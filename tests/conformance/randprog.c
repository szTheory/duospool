/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * randprog.c - the in-process random program generator (plan 03-26; D-93, D-119, D-127, ER-7, T-03-71, T-03-72): a 256 KiB HuCard-shaped image built from a seed with a splitmix64 stream, in memory only
 * (never written to a file, never committed, no host randomness, no clock). Every 8 KiB bank is a run of instructions whose opcodes are drawn from the defined rows of the opcode table (ds_cpu_op_defined) and whose
 * operand bytes are drawn at random, laid out by the instruction length of the table (ds_op_meta), so the stream decodes as written when it is entered from its start; a taken branch or jump enters it anywhere, which is the
 * point. Bank 0 ends in a spin loop (BRA to itself at $FFF0) that the four interrupt vectors (IRQ2 $FFF6, IRQ1 $FFF8, the timer $FFFA and NMI $FFFC) point at, and the reset vector ($FFFE) points at the first instruction of
 * bank 0 ($E000, where MPR7 = $00 maps it at reset). The programs have no meaning: they exist to put the instruction set, the bus, the timer, the interrupt controller and the block transfers through paths no authored fixture
 * takes, so that a chunked or observed run can be compared with a whole one. Evidence tier E0 (repeatability and consistency of the core with itself, ER-7); a pass is not a statement about what any hardware does.
 * Plan 04-13 (D-175, T-04-44) adds two things. The programs of rp_build map the hardware page at logical $0000 (LDA #$FF, TAM #$01 at the first instruction of bank 0, MPR0 = $FF) and, one instruction slot in sixteen, carry a short VDC or VCE access
 * through that page (a register write through AR and the data ports, a VCE window write, a status or data read of the VDC), so the chunked, restored and observed runs compare a machine whose video devices have been written; no access reads $1403 or depends on its timing. rp_build_vdc builds the programs
 * of the vdc_random_regs kind: nothing but such accesses, block transfers into the data ports and counted waits, the register of the VDC drawn over every address of AR and the data of every access drawn at random, so the geometry, the DMA registers, the enables and the VCE window are all driven to values no authored fixture writes.
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

/* the video accesses of the programs (D-175): every one goes through the hardware page that the first instruction of bank 0 maps at logical $0000 (MPR0 = $FF), so the VDC is at $0000 (AR), $0002 and $0003 (the data port, low and high) and the VCE window at $0400 to $0405. Opcodes written here: LDA #imm A9,
 * LDA abs AD, STA abs 8D, LDX #imm A2, DEX CA, BNE D0, TIA E3, TAM 53. Each emitter writes at most RP_HW_MAX bytes at p + at and moves at past them. */
#define RP_HW_MAX 24u
#define RP_OP_LDA_IMM 0xA9u
#define RP_OP_LDA_ABS 0xADu
#define RP_OP_STA_ABS 0x8Du
#define RP_OP_LDX_IMM 0xA2u
#define RP_OP_DEX 0xCAu
#define RP_OP_BNE 0xD0u
#define RP_OP_TIA 0xE3u
#define RP_OP_TAM 0x53u
#define RP_VDC_AR 0x0000u
#define RP_VDC_LSB 0x0002u
#define RP_VDC_MSB 0x0003u
#define RP_VCE_BASE 0x0400u

static void rp_put3(unsigned char *p, size_t *at, unsigned op, unsigned addr)
{
    p[(*at)++] = (unsigned char)op;
    p[(*at)++] = (unsigned char)(addr & 0xFFu);
    p[(*at)++] = (unsigned char)((addr >> 8) & 0xFFu);
}

static void rp_put2(unsigned char *p, size_t *at, unsigned op, unsigned imm)
{
    p[(*at)++] = (unsigned char)op;
    p[(*at)++] = (unsigned char)(imm & 0xFFu);
}

/* LDA #imm, STA addr */
static void rp_store(unsigned char *p, size_t *at, unsigned imm, unsigned addr)
{
    rp_put2(p, at, RP_OP_LDA_IMM, imm);
    rp_put3(p, at, RP_OP_STA_ABS, addr);
}

/* a VDC register write through AR and the data port: AR = reg, then the low and the high byte (the high byte write completes the data write) */
static void rp_vdc_write(unsigned char *p, size_t *at, unsigned reg, unsigned data)
{
    rp_store(p, at, reg, RP_VDC_AR);
    rp_store(p, at, data & 0xFFu, RP_VDC_LSB);
    rp_store(p, at, (data >> 8) & 0xFFu, RP_VDC_MSB);
}

/* a VCE window write: CR (register 0), CTA low or high, CTW low or high */
static void rp_vce_write(unsigned char *p, size_t *at, unsigned pick, unsigned value)
{
    rp_store(p, at, value & 0xFFu, RP_VCE_BASE + (pick == 0u ? 0u : 1u + pick));
}

/* the hardware access of one slot of rp_bank: a VDC register write, a VCE window write, or a read of the VDC status or data port; 15 bytes at most */
static void rp_hw_access(unsigned char *p, size_t *at, uint64_t *state)
{
    uint64_t r = rp_next(state);
    unsigned kind = (unsigned)(r & 7u);
    unsigned a = (unsigned)((r >> 8) & 0xFFFFu);

    if (kind < 4u) {
        rp_vdc_write(p, at, (unsigned)((r >> 24) & 0x1Fu), a);
    } else if (kind < 6u) {
        rp_vce_write(p, at, (unsigned)((r >> 24) & 3u) + (unsigned)((r >> 26) & 1u), a); /* a pick of 0 to 4 */
    } else {
        rp_put3(p, at, RP_OP_LDA_ABS, kind == 6u ? RP_VDC_AR : RP_VDC_LSB + (unsigned)((r >> 24) & 1u)); /* the status, or a data port read */
    }
}

/* fill the bank (RP_BANK_BYTES at p) with instructions up to `limit` bytes; the tail past the last whole instruction keeps the operand bytes of the last one cut short, which is still defined input */
static void rp_bank(unsigned char *p, size_t limit, uint64_t *state)
{
    size_t at = 0u;

    while (at < limit) {
        uint8_t op;

        if (at + RP_HW_MAX <= limit && (rp_next(state) & 15u) == 0u) { /* one slot in sixteen: a video access through the hardware page (D-175) */
            rp_hw_access(p, &at, state);
            continue;
        }
        op = rp_opcode(state);
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

/* the end of bank 0: the spin loop and the vectors; $FFF0 is BRA to itself, and every vector but reset points at it */
static void rp_tail(unsigned char *img)
{
    unsigned v;

    memset(img + RP_SPIN_OFF, 0, RP_BANK_BYTES - RP_SPIN_OFF);
    img[RP_SPIN_OFF] = (unsigned char)RP_BRA_OPCODE;
    img[RP_SPIN_OFF + 1u] = 0xFEu;
    for (v = 0u; v < 4u; v++) { /* $FFF6 IRQ2, $FFF8 IRQ1, $FFFA timer, $FFFC NMI */
        img[0x1FF6u + 2u * v] = 0xF0u;
        img[0x1FF7u + 2u * v] = 0xFFu;
    }
    img[0x1FFEu] = 0x00u; /* reset: $E000 */
    img[0x1FFFu] = 0xE0u;
}

/* the 256 KiB image of a seed into img (RP_IMAGE_BYTES bytes) */
void rp_build(unsigned char *img, uint64_t seed)
{
    uint64_t state = seed;
    unsigned bank;

    for (bank = 0u; bank < RP_IMAGE_BYTES / RP_BANK_BYTES; bank++) {
        if (bank == 0u) { /* the first instructions of bank 0 map the hardware page at logical $0000 (MPR0 = $FF), so the video accesses of rp_bank reach the VDC and the VCE (D-175) */
            size_t at = 0u;

            rp_put2(img, &at, RP_OP_LDA_IMM, 0xFFu);
            rp_put2(img, &at, RP_OP_TAM, 0x01u);
            rp_bank(img + at, (size_t)RP_SPIN_OFF - at, &state);
        } else {
            rp_bank(img + (size_t)bank * RP_BANK_BYTES, (size_t)RP_BANK_BYTES, &state);
        }
    }
    rp_tail(img);
}

/* the vdc_random_regs program of a seed (plan 04-13, D-175, T-04-44): bank 0 only (the other banks are zero and never entered), the hardware page mapped first, then operations drawn from the stream until the bank is full: a VDC register write (the register drawn over all 32 values of AR, one in four from the registers
 * that start a transfer or change the geometry: CR 5, MWR 9, VPR to VCR 12 to 14, DCR 15, SOUR 16, DESR 17, LENR 18, SATB 19), a VCE window write, a status or data port read, a block transfer of at most 64 bytes of the bank into the VDC data port or the VCE data window (TIA, which alternates the two destination addresses),
 * and a counted wait of up to 64 passes (LDX #n, DEX, BNE: 6 CPU cycles per pass). No interrupt is enabled (the reset state masks them), so a program ends only at its deadline. */
void rp_build_vdc(unsigned char *img, uint64_t seed)
{
    static const unsigned hot[] = {5u, 9u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u};
    uint64_t state = seed;
    size_t at = 0u;
    size_t limit = (size_t)RP_SPIN_OFF - (size_t)RP_HW_MAX;

    memset(img, 0, RP_IMAGE_BYTES);
    rp_put2(img, &at, RP_OP_LDA_IMM, 0xFFu);
    rp_put2(img, &at, RP_OP_TAM, 0x01u);
    while (at < limit) {
        uint64_t r = rp_next(&state);
        unsigned kind = (unsigned)(r & 15u);
        unsigned a = (unsigned)((r >> 8) & 0xFFFFu);
        unsigned b = (unsigned)((r >> 24) & 0xFFu);

        if (kind < 7u) { /* a VDC register write */
            unsigned reg = (b & 3u) == 0u ? (unsigned)(sizeof hot / sizeof hot[0]) : 0u;

            reg = reg != 0u ? hot[(b >> 2) % reg] : (b >> 2) & 0x1Fu;
            rp_vdc_write(img, &at, reg, a);
        } else if (kind < 10u) { /* a VCE window write */
            rp_vce_write(img, &at, b % 5u, a);
        } else if (kind < 11u) { /* a read of the status or of a data port */
            rp_put3(img, &at, RP_OP_LDA_ABS, (b & 1u) == 0u ? RP_VDC_AR : RP_VDC_LSB + ((b >> 1) & 1u));
        } else if (kind < 12u) { /* TIA source, destination, length: the source is in the bank (logical $E000 to $E7FF), the destination the VDC data port or the VCE data window, the length 1 to 64 */
            unsigned src = 0xE000u + (a & 0x07FFu);
            unsigned dst = (b & 1u) == 0u ? RP_VDC_LSB : RP_VCE_BASE + 4u;
            unsigned len = 1u + ((b >> 1) & 63u);

            img[at++] = (unsigned char)RP_OP_TIA;
            img[at++] = (unsigned char)(src & 0xFFu);
            img[at++] = (unsigned char)(src >> 8);
            img[at++] = (unsigned char)(dst & 0xFFu);
            img[at++] = (unsigned char)(dst >> 8);
            img[at++] = (unsigned char)(len & 0xFFu);
            img[at++] = 0u;
        } else { /* a counted wait */
            rp_put2(img, &at, RP_OP_LDX_IMM, 1u + (b % 64u));
            img[at++] = (unsigned char)RP_OP_DEX;
            img[at++] = (unsigned char)RP_OP_BNE;
            img[at++] = 0xFDu; /* back to the DEX */
        }
    }
    rp_tail(img);
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
