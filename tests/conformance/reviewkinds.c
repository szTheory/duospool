/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * reviewkinds.c - the two kinds added after the Phase 3 code review (WR-01 and WR-02 of 03-REVIEW.md). Test code only. Evidence class E0 (repeatability of the core against literals written in the case); nothing here is an accuracy, compatibility,
 * cycle-accuracy or hardware claim, and the word pass is not a support claim. Both kinds run a tiny image that is assembled by hand in this file (the opcodes come from the manual transcription tests/oracle/huc6280-opcodes.tsv, never from the core
 * table): the image is built in memory, never written to a file and never committed, the same way randprog.c builds its images.
 *   brk_irq_sample   BRK with an interrupt pending (WR-01, erratum R-9). The program is CLI then BRK with the IRQ1 level held high by the declared test stimulus of the case (irq_stim). BRK sets I as part of its own entry (S8-30 flag row), so the
 *                    interrupt must wait until the BRK handler has run its first instruction: the handlers append one byte each to a log in work RAM (BRK handler 0x0B, IRQ1 handler 0x01), and the log must begin with the BRK handler's byte. The case's expect
 *                    is log=B0,B1 (the first two bytes, in order). The image is booted and run under BOTH values of the model parameter irq_i_sample (pre and post): the order is the same under both, because the rule does not depend on the parameter.
 *                    The literals: the BRK handler first is the S8-30 flag row (I set) with the H8-12 masking of IRQ1 by I, and the IRQ1 handler second is the level that is still high after the RTI (ASSUMED:LED-IRQ-0005 for the sample after RTI).
 *   dev_record_pc    The pc field of the device-register records of a block transfer (WR-02). The program is LDA #$FF, TAM #2 (MPR1 = the hardware page, so the zero page and the stack at logical $2000-$21FF reach the page), then a TII of three bytes from
 *                    $E000 to $2002 (the VDC window) at $E004. Every dev record of the transfer, the three pushes of the prologue and the three byte writes, must carry the address of the TII ($E004, hand-assembled here and written in the case as pc=0xE004)
 *                    and the same address as the one insn record of the transfer. The expect is pc=0xNNNN;records=N (the count, 3 pushes + 3 byte writes, from the instruction definition: Y, A and X are pushed and three bytes move).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "runner.h"

#define RV_BANK0_VECTORS 0x1FF6u /* offset in bank 0 of the IRQ2 and BRK vector ($FFF6): IRQ2/BRK, IRQ1, timer, NMI, reset follow in words */
#define RV_SPIN 0x00F0u          /* a BRA to itself at $E0F0: the unused vectors point here */
#define RV_LOG 0x10u             /* the log of the BRK image starts at RAM offset $10 (logical $2010 under MPR1 = $F8), its count at the zero page byte 0 */

static uint32_t rv_u16(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u);
}

static uint64_t rv_u64(const unsigned char *p)
{
    uint64_t v = 0u;
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

/* an all-zero image with the unused vectors and reset set: the vectors point at the spin loop, reset at $E000; the caller puts the program and the handlers in place */
static unsigned char *rv_image(void)
{
    unsigned char *img = (unsigned char *)calloc(1u, RP_IMAGE_BYTES);
    unsigned v;

    if (img == NULL) {
        return NULL;
    }
    img[RV_SPIN] = 0x80u; /* BRA */
    img[RV_SPIN + 1u] = 0xFEu;
    for (v = 0u; v < 4u; v++) {
        img[RV_BANK0_VECTORS + 2u * v] = (unsigned char)(0xE000u + RV_SPIN) & 0xFFu;
        img[RV_BANK0_VECTORS + 2u * v + 1u] = 0xE0u;
    }
    img[0x1FFEu] = 0x00u; /* reset: $E000 */
    img[0x1FFFu] = 0xE0u;
    return img;
}

/* boot the image (ownership of img passes to the booted block): NULL on success */
static const char *rv_boot(unsigned char *img, const char *request, struct booted *b, const struct ds_instance **inst)
{
    b->mem = NULL;
    b->content = img;
    return boot_content(request, RP_IMAGE_BYTES, b, inst);
}

/* ---- brk_irq_sample ---- */

static const unsigned char rv_brk_main[] = {
    0xA9, 0xF8, /* E000 LDA #$F8   MPR1 value for the work RAM */
    0x53, 0x02, /* E002 TAM #2     MPR1 = $F8: logical $2000-$3FFF is the work RAM, the zero page is RAM offset 0 */
    0x58,       /* E004 CLI        I = 0 */
    0x00,       /* E005 BRK        the signature byte follows; the return address is E007 */
    0xEA,       /* E006 NOP */
    0x80, 0xFE, /* E007 BRA *      the main loop */
};

/* a handler that appends one byte to the log: LDX $00 (the count), LDA #mark, STA $2010,X, INC $00, RTI */
static void rv_handler(unsigned char *at, unsigned char mark)
{
    static const unsigned char body[] = {0xA6, 0x00, 0xA9, 0x00, 0x9D, 0x10, 0x20, 0xE6, 0x00, 0x40};

    memcpy(at, body, sizeof body);
    at[3] = mark;
}

static int rv_parse_log(const char *text, unsigned char *want, unsigned *n)
{
    char buf[64];
    char *tok;
    char *comma;
    uint64_t v;

    *n = 0u;
    if (strncmp(text, "log=", 4u) != 0 || strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text + 4, strlen(text + 4) + 1u);
    /* Split on commas by hand: strtok_r is POSIX, not ISO C11, so a strict -std=c11 build (gcc on Linux) does not declare it. */
    tok = buf;
    for (;;) {
        char hex[8];

        comma = strchr(tok, ',');
        if (comma != NULL) {
            *comma = '\0';
        }
        if (*n >= 8u || strlen(tok) < 1u || strlen(tok) > 2u) {
            return -1;
        }
        snprintf(hex, sizeof hex, "0x%.2s", tok);
        if (parse_u64(hex, &v) != 0 || v > 0xFFu) {
            return -1;
        }
        want[(*n)++] = (unsigned char)v;
        if (comma == NULL) {
            break;
        }
        tok = comma + 1;
    }
    return *n >= 2u ? 0 : -1;
}

static void rv_brk_one(const struct kase *k, struct ctx *c, const char *param, uint64_t until, const unsigned char *want, unsigned nwant)
{
    char request[48];
    char msg[192];
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    struct ds_run_request rq;
    struct ds_run_result res;
    uint32_t refusal = 0u;
    unsigned char *img = rv_image();
    const char *err;
    unsigned i;

    if (img == NULL) {
        harness_error(c, "out of memory for the BRK image");
        return;
    }
    memcpy(img, rv_brk_main, sizeof rv_brk_main);
    rv_handler(img + 0x100u, 0x0Bu); /* $E100, the BRK and IRQ2 vector */
    rv_handler(img + 0x200u, 0x01u); /* $E200, the IRQ1 vector */
    img[RV_BANK0_VECTORS] = 0x00u;
    img[RV_BANK0_VECTORS + 1u] = 0xE1u;
    img[RV_BANK0_VECTORS + 2u] = 0x00u;
    img[RV_BANK0_VECTORS + 3u] = 0xE2u;
    snprintf(request, sizeof request, "model.irq_i_sample=%s", param);
    err = rv_boot(img, request, &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    err = apply_irq_stim(w, k, &refusal);
    if (err != NULL || refusal != 0u) {
        release_boot(&b);
        harness_error(c, err != NULL ? err : "the irq_stim list of the case was refused");
        return;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    (void)ds_run(w, &rq, &res);
    note_run(c, res.machine_time, res.reason);
    snprintf(msg, sizeof msg, "irq_i_sample=%s: the run did not reach its deadline", param);
    check(c, res.reason == (uint32_t)DS_RUN_REASON_UNTIL, msg);
    snprintf(msg, sizeof msg, "irq_i_sample=%s: the handlers ran %u times, the case lists at least %u log bytes", param, (unsigned)w->ram8k[0], nwant);
    check(c, (unsigned)w->ram8k[0] >= nwant && (unsigned)w->ram8k[0] < 0x80u, msg);
    for (i = 0u; i < nwant && (unsigned)w->ram8k[0] > i; i++) {
        snprintf(msg, sizeof msg, "irq_i_sample=%s: log byte %u is %02X, the case says %02X (a pending interrupt ran before the first instruction of the BRK handler when the first byte is the interrupt's)", param, i, (unsigned)w->ram8k[RV_LOG + i], (unsigned)want[i]);
        check(c, w->ram8k[RV_LOG + i] == want[i], msg);
    }
    release_boot(&b);
}

void kind_brk_irq_sample(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    unsigned char want[8];
    unsigned nwant = 0u;
    uint64_t until = 0u;

    if (rv_parse_log(k->val[K_EXPECT], want, &nwant) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || !k->has[K_IRQ_STIM]) {
        harness_error(c, "a brk_irq_sample case needs until_mclk, an irq_stim list and expect = log=B0,B1[,...] (hex bytes of the handler log, at least two)");
        return;
    }
    rv_brk_one(k, c, "pre", until, want, nwant);
    rv_brk_one(k, c, "post", until, want, nwant);
    snprintf(got, gotcap, "values=pre,post log=%u", nwant);
}

/* ---- dev_record_pc ---- */

static const unsigned char rv_tii_main[] = {
    0xA9, 0xFF,                                     /* E000 LDA #$FF   MPR1 value for the hardware page */
    0x53, 0x02,                                     /* E002 TAM #2     MPR1 = $FF: logical $2000-$3FFF is the hardware page */
    0x73, 0x00, 0xE0, 0x02, 0x20, 0x03, 0x00,       /* E004 TII $E000,$2002,3: source, destination (the VDC data window) and length, little endian */
    0x80, 0xFE,                                     /* E00B BRA *      the loop after the transfer */
};

static int rv_parse_pc(const char *text, uint32_t *pc, uint32_t *records)
{
    char buf[64];
    char *semi;
    uint64_t v;

    if (strncmp(text, "pc=", 3u) != 0 || strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text + 3, strlen(text + 3) + 1u);
    semi = strchr(buf, ';');
    if (semi == NULL || strncmp(semi + 1, "records=", 8u) != 0) {
        return -1;
    }
    *semi = '\0';
    if (parse_u64(buf, &v) != 0 || v > 0xFFFFu) {
        return -1;
    }
    *pc = (uint32_t)v;
    if (parse_u64(semi + 9, &v) != 0 || v < 1u || v > 64u) {
        return -1;
    }
    *records = (uint32_t)v;
    return 0;
}

void kind_dev_record_pc(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    enum { ICAP = 64, DCAP = 64 };
    uint32_t want_pc = 0u;
    uint32_t want_n = 0u;
    uint64_t until = 0u;
    unsigned char *img;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_insn_sink isk;
    struct ds_dev_sink dsk;
    union {
        uint64_t align;
        unsigned char b[ICAP * DS_INSN_RECORD_BYTES];
    } iu;
    union {
        uint64_t align;
        unsigned char b[DCAP * DS_DEV_RECORD_BYTES];
    } du;
    const char *err;
    uint32_t icount;
    uint32_t dcount;
    uint32_t i;
    uint32_t tii = 0u;
    uint32_t tii_pc = 0u;
    char msg[192];

    if (rv_parse_pc(k->val[K_EXPECT], &want_pc, &want_n) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "a dev_record_pc case needs until_mclk and expect = pc=0xNNNN;records=N");
        return;
    }
    img = rv_image();
    if (img == NULL) {
        harness_error(c, "out of memory for the TII image");
        return;
    }
    memcpy(img, rv_tii_main, sizeof rv_tii_main);
    err = rv_boot(img, "", &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    memset(&isk, 0, sizeof isk);
    memset(&dsk, 0, sizeof dsk);
    memset(&iu, 0, sizeof iu);
    memset(&du, 0, sizeof du);
    isk.records = iu.b;
    isk.capacity = ICAP;
    dsk.records = du.b;
    dsk.capacity = DCAP;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.insn = &isk;
    rq.dev = &dsk;
    (void)ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    note_run(c, res.machine_time, res.reason);
    release_boot(&b);
    icount = (uint32_t)isk.header[0] | ((uint32_t)isk.header[1] << 8u) | ((uint32_t)isk.header[2] << 16u) | ((uint32_t)isk.header[3] << 24u);
    dcount = (uint32_t)dsk.header[0] | ((uint32_t)dsk.header[1] << 8u) | ((uint32_t)dsk.header[2] << 16u) | ((uint32_t)dsk.header[3] << 24u);
    if (res.reason != (uint32_t)DS_RUN_REASON_UNTIL || isk.header[4] != 0u || dsk.header[4] != 0u) {
        harness_error(c, "the run did not reach its deadline or a sink lost a record: the log is evidence here and would be incomplete");
        return;
    }
    /* the one insn record of the transfer: the opcode 0x73 (TII), found by opcode, not by address */
    for (i = 0u; i < icount; i++) {
        const unsigned char *ir = iu.b + (size_t)i * DS_INSN_RECORD_BYTES;

        if (ir[10] == 0x73u) {
            tii++;
            tii_pc = rv_u16(ir + 8);
        }
    }
    snprintf(msg, sizeof msg, "the insn log holds %u records of opcode 73 (TII), wanted exactly one (the transfer is one record, D-100)", (unsigned)tii);
    check(c, tii == 1u, msg);
    snprintf(msg, sizeof msg, "the insn record of the TII has pc %04X, the hand-assembled image puts it at %04X", (unsigned)tii_pc, (unsigned)want_pc);
    check(c, tii_pc == want_pc, msg);
    snprintf(msg, sizeof msg, "the dev log holds %u records, the transfer makes %u (three pushes and three byte writes)", (unsigned)dcount, (unsigned)want_n);
    check(c, dcount == want_n, msg);
    for (i = 0u; i < dcount; i++) {
        const unsigned char *r = du.b + (size_t)i * DS_DEV_RECORD_BYTES;

        snprintf(msg, sizeof msg, "dev record %u carries pc %04X, the address of the TII is %04X (prologue, byte and epilogue records all belong to the one instruction)", (unsigned)i, (unsigned)rv_u16(r + 16), (unsigned)want_pc);
        check(c, rv_u16(r + 16) == want_pc && rv_u16(r + 16) == tii_pc, msg);
        if (i > 0u) {
            check(c, rv_u64(r) >= rv_u64(r - DS_DEV_RECORD_BYTES), "the stamps of the dev log are not in order");
        }
    }
    snprintf(got, gotcap, "records=%u pc=%04X", (unsigned)dcount, (unsigned)tii_pc);
}
