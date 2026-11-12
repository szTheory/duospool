/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * randprog.c - the in-process random program generator (plan 03-26; D-93, D-119, D-127, ER-7, T-03-71, T-03-72): a 256 KiB HuCard-shaped image built from a seed with a splitmix64 stream, in memory only
 * (never written to a file, never committed, no host randomness, no clock). Every 8 KiB bank is a run of instructions whose opcodes are drawn from the defined rows of the opcode table (ds_cpu_op_defined) and whose
 * operand bytes are drawn at random, laid out by the instruction length of the table (ds_op_meta), so the stream decodes as written when it is entered from its start; a taken branch or jump enters it anywhere, which is the
 * point. Bank 0 ends in a spin loop (BRA to itself at $FFF0) that the four interrupt vectors (IRQ2 $FFF6, IRQ1 $FFF8, the timer $FFFA and NMI $FFFC) point at, and the reset vector ($FFFE) points at the first instruction of
 * bank 0 ($E000, where MPR7 = $00 maps it at reset). The programs have no meaning: they exist to put the instruction set, the bus, the timer, the interrupt controller and the block transfers through paths no authored fixture
 * takes, so that a chunked or observed run can be compared with a whole one. Evidence tier E0 (repeatability and consistency of the core with itself, ER-7); a pass is not a statement about what any hardware does.
 * Test code only, never installed.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

#define RP_BANK_BYTES 0x2000u
#define RP_SPIN_OFF 0x1FF0u /* offset in bank 0 of the spin loop: logical $FFF0 under the reset mapping */
#define RP_BRA_OPCODE 0x80u
#define RP_SEED_BASE UINT64_C(0x5EED000000000003)
#define RP_SEED_STEP UINT64_C(0xD6E8FEB86659FD93) /* an odd constant unrelated to the splitmix gamma, so that the streams of two programs are not shifted copies of one stream */

static uint64_t rp_next(uint64_t *state)
{
    uint64_t z;

    *state = *state + UINT64_C(0x9E3779B97F4A7C15);
    z = *state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* the seed of program index: a fixed function of the index alone, one splitmix64 output of a state that is spread by the index (adjacent indexes give unrelated streams) */
uint64_t rp_seed(unsigned index)
{
    uint64_t state = RP_SEED_BASE + (uint64_t)index * RP_SEED_STEP;

    return rp_next(&state);
}

/* a defined opcode: drawn until the table says the row is an instruction (234 of 256 rows are, so the loop ends at once for every seed; the bound keeps it finite whatever the table says) */
static uint8_t rp_opcode(uint64_t *state)
{
    unsigned tries;

    for (tries = 0u; tries < 4096u; tries++) {
        uint8_t op = (uint8_t)(rp_next(state) & 0xFFu);

        int derail = op == 0x00u || op == 0x20u || op == 0x40u || op == 0x4Cu || op == 0x60u || op == 0x6Cu || op == 0x7Cu; /* BRK, JSR, RTI, JMP (three forms), RTS: one in sixteen is kept, so most programs get past their first instructions */

        if (ds_cpu_op_defined((uint32_t)op) != 0 && (!derail || (rp_next(state) & 15u) == 0u)) {
            return op;
        }
    }
    return 0xEAu; /* NOP, defined in every table */
}

/* the operands of the instructions that decide how far a program gets are shaped, still from the stream: a relative branch (BRA, BSR, the eight conditions, the sixteen bit branches) goes forward by at most 31 bytes, so a program is not caught in a backward
 * loop after a few instructions, and a block transfer moves at most 255 bytes except one in 32, which keeps the length operand 0 (65536 bytes), so a program is not held in one transfer for the whole deadline; every other byte is as drawn */
static void rp_shape(unsigned char *p, size_t start, size_t len, uint8_t op, uint64_t *state)
{
    int branch = (op & 0x1Fu) == 0x10u || op == 0x80u || op == 0x44u || (op & 0x0Fu) == 0x0Fu;
    int block = op == 0x73u || op == 0xC3u || op == 0xD3u || op == 0xE3u || op == 0xF3u;

    if (branch && len >= 2u) {
        p[start + len - 1u] = (unsigned char)(p[start + len - 1u] & 0x1Fu);
    }
    if (block && len == 7u) {
        p[start + 6u] = 0u;
        if ((rp_next(state) & 31u) != 0u) {
            p[start + 5u] = (unsigned char)(1u + (p[start + 5u] % 255u));
        }
    }
}

/* fill the bank (RP_BANK_BYTES at p) with instructions up to `limit` bytes; the tail past the last whole instruction keeps the operand bytes of the last one cut short, which is still defined input */
static void rp_bank(unsigned char *p, size_t limit, uint64_t *state)
{
    size_t at = 0u;

    while (at < limit) {
        uint8_t op = rp_opcode(state);
        uint32_t bytes = 0u;
        uint32_t cycles = 0u;
        uint32_t i;

        ds_op_meta((uint32_t)op, &bytes, &cycles);
        if (bytes == 0u || bytes > 7u) {
            bytes = 1u;
        }
        size_t start = at;

        p[at++] = op;
        for (i = 1u; i < bytes && at < limit; i++) {
            p[at++] = (uint8_t)(rp_next(state) & 0xFFu);
        }
        rp_shape(p, start, at - start, op, state);
    }
}

/* the 256 KiB image of a seed into img (RP_IMAGE_BYTES bytes) */
void rp_build(unsigned char *img, uint64_t seed)
{
    uint64_t state = seed;
    unsigned bank;

    for (bank = 0u; bank < RP_IMAGE_BYTES / RP_BANK_BYTES; bank++) {
        rp_bank(img + (size_t)bank * RP_BANK_BYTES, bank == 0u ? (size_t)RP_SPIN_OFF : (size_t)RP_BANK_BYTES, &state);
    }
    /* the end of bank 0: the spin loop and the vectors; $FFF0 is BRA to itself, and every vector but reset points at it */
    memset(img + RP_SPIN_OFF, 0, RP_BANK_BYTES - RP_SPIN_OFF);
    img[RP_SPIN_OFF] = (unsigned char)RP_BRA_OPCODE;
    img[RP_SPIN_OFF + 1u] = 0xFEu;
    {
        unsigned v;

        for (v = 0u; v < 4u; v++) { /* $FFF6 IRQ2, $FFF8 IRQ1, $FFFA timer, $FFFC NMI */
            img[0x1FF6u + 2u * v] = 0xF0u;
            img[0x1FF7u + 2u * v] = 0xFFu;
        }
        img[0x1FFEu] = 0x00u; /* reset: $E000 */
        img[0x1FFFu] = 0xE0u;
    }
}

/* boot a copy of the image img (RP_IMAGE_BYTES bytes, built once by the caller with rp_build and kept for the several boots of one program) over the request text (the default request when empty): NULL and the accepted instance, or why not (a fault of the case); release_boot frees the copy and the instance */
const char *boot_random(const unsigned char *img, const char *request, struct booted *b, const struct ds_instance **inst)
{
    b->mem = NULL;
    b->content = (unsigned char *)malloc(RP_IMAGE_BYTES);
    if (b->content == NULL) {
        return "out of memory for the random program image";
    }
    memcpy(b->content, img, RP_IMAGE_BYTES);
    return boot_content(request, RP_IMAGE_BYTES, b, inst);
}
