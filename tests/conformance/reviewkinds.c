/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * reviewkinds.c - the kinds added after the Phase 3 code review of 03-REVIEW.md (WR-01 here, WR-02 in the commit after). Test code only. Evidence class E0 (repeatability of the core against literals written in the case); nothing here is an accuracy, compatibility,
 * cycle-accuracy or hardware claim, and the word pass is not a support claim. The kind runs a tiny image that is assembled by hand in this file (the opcodes come from the manual transcription tests/oracle/huc6280-opcodes.tsv, never from the core
 * table): the image is built in memory, never written to a file and never committed, the same way randprog.c builds its images.
 *   brk_irq_sample   BRK with an interrupt pending (WR-01, erratum R-9). The program is CLI then BRK with the IRQ1 level held high by the declared test stimulus of the case (irq_stim). BRK sets I as part of its own entry (S8-30 flag row), so the
 *                    interrupt must wait until the BRK handler has run its first instruction: the handlers append one byte each to a log in work RAM (BRK handler 0x0B, IRQ1 handler 0x01), and the log must begin with the BRK handler's byte. The case's expect
 *                    is log=B0,B1 (the first two bytes, in order). The image is booted and run under BOTH values of the model parameter irq_i_sample (pre and post): the order is the same under both, because the rule does not depend on the parameter.
 *                    The literals: the BRK handler first is the S8-30 flag row (I set) with the H8-12 masking of IRQ1 by I, and the IRQ1 handler second is the level that is still high after the RTI (ASSUMED:LED-IRQ-0005 for the sample after RTI).
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
    char *save = NULL;
    uint64_t v;

    *n = 0u;
    if (strncmp(text, "log=", 4u) != 0 || strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text + 4, strlen(text + 4) + 1u);
    for (tok = strtok_r(buf, ",", &save); tok != NULL; tok = strtok_r(NULL, ",", &save)) {
        char hex[8];

        if (*n >= 8u || strlen(tok) < 1u || strlen(tok) > 2u) {
            return -1;
        }
        snprintf(hex, sizeof hex, "0x%s", tok);
        if (parse_u64(hex, &v) != 0 || v > 0xFFu) {
            return -1;
        }
        want[(*n)++] = (unsigned char)v;
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
