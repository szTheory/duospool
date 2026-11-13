/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * psgkinds.c - the C-level PSG case kinds (plan 05-03; D-185, D-188..D-193, D-204, D-212, D-220, D-223, D-231, PR-2). Every expected value is a literal written in this file from a manual page (MAN:P8-n), an assumed ledger convention (ASSUMED:LED-x) or an
 * independent script outside the repository; never a value read back from the core first. The class of each row is named in the case comment. Evidence tier E0: these kinds show the core follows the project's written rules, nothing about hardware.
 *   psg_decode           expect rows=all: the mirror (offsets r, r + $10, r + $3F0 through the bus), the storage masks, reg $A..$F, R2..R7 at select 6/7, R7 at select 0..3, R1/R8/R9 at any select, sel_before
 *   psg_wave_ptr         expect rows=all: the R6 wave pointer and DDA latch procedures and the channel counter bookkeeping (key-on, freeze, F = 0, DDA release)
 *   psg_lfo_trg          expect rows=all: R9 TRG reset and freeze, the TRG release restart, UNMODELLED, the D-231 TRG state with widx[1] = 5 restored
 *   psg_poweron          expect rows=all: the PSG power-on state under zero, v1, v2, v3 and v4 (seeds 1, 2), the v4 wave draw, the noise counters from time 0
 *   psg_restore_divisor  expect rows=all: a psg_tick_divisor=3 snapshot is refused by a default instance as snapshot.profile and restored by a d = 3 one (D-223)
 *   psg_vol_table        expect rows=all: the 30 rows of psg_vol.def strictly decrease from 32768 to 219, and att 29, 30 and 91 contribute 31 x 219, 0 and 0 (plan 05-06)
 *   psg_pure_function    expect rows=all: widx(T + 6F) = widx(T) + 1 and widx(T + 192F) = widx(T) for four F, the noise distance periodic with 384 x NFc for three NFc (plan 05-06)
 * (later plans add psg_fold and others to this file.) Test code only, never installed. Includes the core's private ds_psg.h as vdckinds.c includes ds_vdc.h.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_psg.h"
#include "ds_serial.h"
#include "ds_sha256.h"
#include "ds_snapshot.h"
#include "runner.h"

/* the storage masks of D-189 written here by hand (MAN:P8-2, P8-3, P8-4, P8-6, P8-9), not read from psg.c */
static const uint8_t pk_mask[10] = {0x07u, 0xFFu, 0xFFu, 0x0Fu, 0xDFu, 0xFFu, 0x1Fu, 0x9Fu, 0xFFu, 0x83u};

static int pk_rows_all(const struct kase *k, struct ctx *c, const char *kind)
{
    char msg[160];

    if (!k->has[K_EXPECT] || strcmp(k->val[K_EXPECT], "rows=all") != 0) {
        snprintf(msg, sizeof msg, "a %s case needs expect = rows=all and a booting synthetic fixture", kind);
        harness_error(c, msg);
        return 0;
    }
    return 1;
}

static struct ds_instance *pk_boot(struct ctx *c, const char *fixture, const char *request, struct booted *b)
{
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted(fixture, request, b, &s, &inst);

    if (err != NULL) {
        harness_error(c, err);
        return NULL;
    }
    return (struct ds_instance *)(void *)b->mem;
}

/* the PSG block, psg_freq to the end of psg_lfsr */
#define PK_BLOCK_OFF offsetof(struct ds_instance, psg_freq)
#define PK_BLOCK_LEN (sizeof(struct ds_instance) - offsetof(struct ds_instance, psg_freq))

/* the PSG state at power-on with a zero wave RAM, at machine time 0 */
static void pk_reset(struct ds_instance *w)
{
    ds_psg_poweron(w);
    memset(w->psg_wave, 0, sizeof w->psg_wave);
    w->machine_time = 0u;
}

/* a write through the bus with no step (stamp = machine time) to window-2 offset off; MPR0 maps the hardware page at logical $0000 */
static void pk_bus(struct ds_instance *w, uint32_t off, uint32_t value)
{
    ds_mpr_write(w, 0u, 0xFFu);
    ds_bus_write(w, NULL, (uint16_t)(0x0800u + off), (uint8_t)value);
}

/* the register a write to reg stored, as the byte that the masks speak of (R2 and R3 the low and high byte of F of the selected channel; R6 the word at wave index 0 of the selected channel) */
static uint32_t pk_stored(const struct ds_instance *w, uint32_t reg)
{
    uint32_t sel = w->psg_select;

    switch (reg) {
    case 0u:
        return w->psg_select;
    case 1u:
        return w->psg_main_bal;
    case 2u:
        return (uint32_t)w->psg_freq[sel] & 0xFFu;
    case 3u:
        return (uint32_t)w->psg_freq[sel] >> 8;
    case 4u:
        return w->psg_ctl[sel];
    case 5u:
        return w->psg_bal[sel];
    case 6u:
        return w->psg_wave[sel * 32u];
    case 7u:
        return w->psg_noise[sel - 4u];
    case 8u:
        return w->psg_lfo_freq;
    default:
        return w->psg_lfo_ctl;
    }
}

void kind_psg_decode(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const uint32_t mirror[3] = {0x000u, 0x010u, 0x3F0u};
    struct booted b;
    struct ds_instance *w;
    uint8_t before[PK_BLOCK_LEN];
    struct ds_psg_write_info info;
    uint32_t reg;
    uint32_t m;
    uint32_t sel;
    uint32_t e;
    unsigned rows = 0u;
    char msg[200];

    if (!pk_rows_all(k, c, "psg_decode")) {
        return;
    }
    w = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b);
    if (w == NULL) {
        return;
    }
    /* the masks and the mirror: $FF at offsets r, r + $10 and r + $3F0 stores the same masked value; R2..R6 at select 0, R7 at select 4 */
    for (reg = 0u; reg < 10u; reg++) {
        snprintf(msg, sizeof msg, "ds_psg_reg_mask(%u) is $%02X, not the mask $%02X of D-189", (unsigned)reg, (unsigned)ds_psg_reg_mask(reg), (unsigned)pk_mask[reg]);
        check(c, ds_psg_reg_mask(reg) == (uint32_t)pk_mask[reg], msg);
        for (m = 0u; m < 3u; m++) {
            pk_reset(w);
            pk_bus(w, 0u, reg == 7u ? 4u : 0u);
            pk_bus(w, mirror[m] + reg, 0xFFu);
            snprintf(msg, sizeof msg, "R%u written $FF at window offset $%03X stored $%02X, not $%02X (mask D-189, mirror reg = offset & $0F)", (unsigned)reg, (unsigned)(mirror[m] + reg), (unsigned)pk_stored(w, reg), (unsigned)pk_mask[reg]);
            check(c, pk_stored(w, reg) == (uint32_t)pk_mask[reg], msg);
            rows++;
        }
    }
    /* reg $A..$F store nothing (MAN:P8-2) and return NO_TARGET */
    for (reg = 10u; reg < 16u; reg++) {
        pk_reset(w);
        ds_psg_advance(w, 0u, 0u, NULL); /* the noise events due at time 0 are folded first: the row judges the write alone */
        memcpy(before, (const uint8_t *)w + PK_BLOCK_OFF, PK_BLOCK_LEN);
        pk_bus(w, 0x3F0u + reg, 0xFFu);
        snprintf(msg, sizeof msg, "a write to reg $%X changed the PSG state", (unsigned)reg);
        check(c, memcmp(before, (const uint8_t *)w + PK_BLOCK_OFF, PK_BLOCK_LEN) == 0, msg);
        e = ds_psg_write(w, reg, 0xFFu, 0u, &info);
        check(c, e == (uint32_t)DS_PSGE_NO_TARGET && info.chan == 0xFFu && info.wave_idx == 0xFFu && info.post == 0xFFFFu && info.effect == (uint8_t)DS_PSGE_NO_TARGET, "a write to reg $A..$F is not NO_TARGET with chan $FF, wave_idx $FF and post $FFFF");
        rows++;
    }
    /* R2..R7 at select 6 and 7 store nothing (ASSUMED:LED-AUD-0004 psg_select_67_drop); R7 at select 0..3 is dropped (ASSUMED:LED-AUD-0004) */
    for (sel = 0u; sel < 8u; sel++) {
        for (reg = 2u; reg < 8u; reg++) {
            int dropped = sel > 5u || (reg == 7u && sel < 4u);

            if (!dropped) {
                continue;
            }
            pk_reset(w);
            pk_bus(w, 0u, sel);
            memcpy(before, (const uint8_t *)w + PK_BLOCK_OFF, PK_BLOCK_LEN);
            pk_bus(w, reg, 0xFFu);
            snprintf(msg, sizeof msg, "R%u at select %u changed the PSG state", (unsigned)reg, (unsigned)sel);
            check(c, memcmp(before, (const uint8_t *)w + PK_BLOCK_OFF, PK_BLOCK_LEN) == 0, msg);
            e = ds_psg_write(w, reg, 0xFFu, 0u, &info);
            snprintf(msg, sizeof msg, "R%u at select %u is not NO_TARGET with chan $FF and post $FFFF, or sel is not %u", (unsigned)reg, (unsigned)sel, (unsigned)sel);
            check(c, e == (uint32_t)DS_PSGE_NO_TARGET && info.chan == 0xFFu && info.post == 0xFFFFu && info.sel == (uint8_t)sel, msg);
            rows++;
        }
    }
    /* R1, R8 and R9 ignore R0 (MAN:P8-2, P8-3): stored at select 6 and 7, chan $FE */
    for (sel = 6u; sel < 8u; sel++) {
        static const uint32_t glob[3] = {1u, 8u, 9u};
        uint32_t g;

        for (g = 0u; g < 3u; g++) {
            pk_reset(w);
            pk_bus(w, 0u, sel);
            e = ds_psg_write(w, glob[g], 0xFDu, 0u, &info);
            snprintf(msg, sizeof msg, "R%u at select %u stored $%02X, not $%02X, or chan is not $FE", (unsigned)glob[g], (unsigned)sel, (unsigned)pk_stored(w, glob[g]), (unsigned)(0xFDu & pk_mask[glob[g]]));
            check(c, pk_stored(w, glob[g]) == (0xFDu & (uint32_t)pk_mask[glob[g]]) && info.chan == 0xFEu && info.wave_idx == 0xFFu && (e & (uint32_t)DS_PSGE_NO_TARGET) == 0u, msg);
            rows++;
        }
    }
    /* sel_before is R0 before the write; post of R0 is the 3 bits stored, of R2/R3 the 12-bit F */
    pk_reset(w);
    (void)ds_psg_write(w, 0u, 0x0Bu, 0u, &info);
    check(c, info.sel == 0u && w->psg_select == 3u && info.post == 3u && info.chan == 0xFEu, "R0 $0B: sel_before is not 0, the select is not 3 or post is not 3");
    (void)ds_psg_write(w, 0u, 0x05u, 0u, &info);
    check(c, info.sel == 3u && w->psg_select == 5u, "R0 $05 after R0 $0B: sel_before is not 3");
    (void)ds_psg_write(w, 3u, 0xF2u, 0u, &info);
    (void)ds_psg_write(w, 2u, 0x34u, 0u, &info);
    check(c, w->psg_freq[5] == 0x234u && info.post == 0x234u && info.chan == 5u && info.sel == 5u, "R3 $F2 then R2 $34 at select 5: F is not $234 or post is not the 12-bit F");
    rows += 3u;
    snprintf(got, gotcap, "rows=%u", rows);
    release_boot(&b);
}

/* the counter row helpers: fold to t (no rebase), then write */
static uint32_t pk_w(struct ds_instance *w, uint32_t reg, uint32_t value, uint64_t t, struct ds_psg_write_info *info)
{
    ds_psg_advance(w, t, 0u, NULL);
    return ds_psg_write(w, reg, value, t, info);
}

void kind_psg_wave_ptr(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct ds_instance *w;
    struct ds_psg_write_info info;
    uint32_t i;
    uint32_t e;
    unsigned bad = 0u;
    char msg[200];

    if (!pk_rows_all(k, c, "psg_wave_ptr")) {
        return;
    }
    w = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b);
    if (w == NULL) {
        return;
    }
    /* (1) 32 R6 writes with chON 0 and DDA 0 fill wave[0..31] and the index wraps to 0, WAVE_STORE each (MAN:P8-4, P8-5) */
    pk_reset(w);
    (void)ds_psg_write(w, 0u, 2u, 0u, &info);
    for (i = 0u; i < 32u; i++) {
        e = ds_psg_write(w, 6u, 0xE0u | i, 0u, &info);
        bad += e != (uint32_t)DS_PSGE_WAVE_STORE || info.chan != 2u || info.wave_idx != (uint8_t)((i + 1u) & 31u) || info.post != i || w->psg_wave[64u + i] != (uint8_t)i ? 1u : 0u;
    }
    check(c, bad == 0u && w->psg_widx[2] == 0u, "32 R6 writes with chON 0 and DDA 0 did not fill wave[0..31] with the 5-bit words, step the index each time and wrap it to 0 with WAVE_STORE");
    /* (2) R4 $40 sets the index 0 (IDX_RESET), R4 $00 keeps it (MAN:P8-4, P8-5) */
    for (i = 0u; i < 3u; i++) {
        (void)ds_psg_write(w, 6u, 1u, 0u, &info);
    }
    e = ds_psg_write(w, 4u, 0x40u, 0u, &info);
    check(c, w->psg_widx[2] == 0u && e == (uint32_t)DS_PSGE_IDX_RESET && info.post == 0x40u, "R4 $40 after three R6 writes did not set the wave index 0 with IDX_RESET");
    e = ds_psg_write(w, 4u, 0x00u, 0u, &info);
    check(c, w->psg_widx[2] == 0u && e == 0u, "R4 $00 after R4 $40 changed the wave index or set an effect");
    /* (3) R6 $00, R4 $DF, R6 v: the latch is v & $1F, the wave RAM unchanged, the index 0, DDA_LOAD (MAN:P8-4, P8-5) */
    (void)ds_psg_write(w, 6u, 0x00u, 0u, &info);
    e = ds_psg_write(w, 4u, 0xDFu, 0u, &info);
    check(c, w->psg_widx[2] == 0u && e == (uint32_t)DS_PSGE_IDX_RESET && w->psg_ctl[2] == 0xDFu, "R4 $DF did not store $DF and set the wave index 0");
    {
        uint8_t wave[192];

        memcpy(wave, w->psg_wave, sizeof wave);
        e = ds_psg_write(w, 6u, 0x35u, 0u, &info);
        check(c, w->psg_dda[2] == 0x15u && memcmp(wave, w->psg_wave, sizeof wave) == 0 && w->psg_widx[2] == 0u && e == (uint32_t)DS_PSGE_DDA_LOAD && info.post == 0x15u, "R6 $35 with DDA 1 did not load the latch $15 alone with DDA_LOAD");
    }
    /* R6 with chON 0 and DDA 1 loads the latch (ASSUMED:LED-AUD-0004, DX-8) */
    (void)ds_psg_write(w, 4u, 0x40u, 0u, &info);
    e = ds_psg_write(w, 6u, 0x0Au, 0u, &info);
    check(c, w->psg_dda[2] == 0x0Au && e == (uint32_t)DS_PSGE_DDA_LOAD, "R6 with chON 0 and DDA 1 did not load the latch");
    /* R6 with chON 1 and DDA 0 stores wave[widx] with no increment, WAVE_HELD (ASSUMED:LED-AUD-0004, DX-9) */
    pk_reset(w);
    (void)ds_psg_write(w, 0u, 2u, 0u, &info);
    (void)ds_psg_write(w, 6u, 0x01u, 0u, &info);
    (void)ds_psg_write(w, 6u, 0x02u, 0u, &info);
    (void)ds_psg_write(w, 4u, 0x80u, 0u, &info);
    e = ds_psg_write(w, 6u, 0x1Eu, 0u, &info);
    check(c, w->psg_wave[66] == 0x1Eu && w->psg_wave[65] == 0x02u && w->psg_widx[2] == 2u && e == (uint32_t)DS_PSGE_WAVE_HELD && info.wave_idx == 2u, "R6 with chON 1 and DDA 0 did not store wave[2] = $1E with no increment and WAVE_HELD");

    /* the counter rows (PR-2; d = 6, a fresh state at machine time 0, offsets from B = -1). Select 0, F = 256 (R2 $00, R3 $01), key-on R4 $9F at stamp 100,003: the tick before it is w' = 100,002, the events at w' + 1,536 j (6F MAN:P8-9 + H8-23;
     * the tick phase and the key-on phase ASSUMED:LED-AUD-0012, so the rows are ASSUMED) */
    pk_reset(w);
    (void)pk_w(w, 0u, 0u, 10u, &info);
    (void)pk_w(w, 2u, 0x00u, 20u, &info);
    (void)pk_w(w, 3u, 0x01u, 30u, &info);
    (void)pk_w(w, 4u, 0x9Fu, 100003u, &info);
    snprintf(msg, sizeof msg, "key-on at 100,003: the distance is %u, not 101,539 (next event 101,538 = 100,002 + 1,536)", (unsigned)w->psg_rem[0]);
    check(c, w->psg_rem[0] == 101539u && w->psg_widx[0] == 0u, msg);
    ds_psg_advance(w, 107681u, 0u, NULL);
    snprintf(msg, sizeof msg, "folded to 107,681 the wave index is %u, not 4", (unsigned)w->psg_widx[0]);
    check(c, w->psg_widx[0] == 4u, msg);
    ds_psg_advance(w, 107682u, 0u, NULL);
    snprintf(msg, sizeof msg, "folded to 107,682 the wave index is %u, not 5", (unsigned)w->psg_widx[0]);
    check(c, w->psg_widx[0] == 5u, msg);
    /* R4 $1F (chON 0) freezes: distance 0, index unchanged; nothing moves afterwards */
    (void)pk_w(w, 4u, 0x1Fu, 107700u, &info);
    ds_psg_advance(w, 200000u, 0u, NULL);
    check(c, w->psg_rem[0] == 0u && w->psg_widx[0] == 5u, "R4 $1F did not freeze select 0 (distance 0, wave index 5 kept)");
    /* F = 0 steps every 24,576 (Fe 4096, ASSUMED:LED-AUD-0007): select 3 keyed on at 200,004 (a tick), next events 224,580 and 249,156 */
    (void)pk_w(w, 0u, 3u, 200000u, &info);
    (void)pk_w(w, 4u, 0x80u, 200004u, &info);
    ds_psg_advance(w, 224579u, 0u, NULL);
    check(c, w->psg_widx[3] == 0u, "F = 0: select 3 stepped before 224,580");
    ds_psg_advance(w, 224580u, 0u, NULL);
    check(c, w->psg_widx[3] == 1u, "F = 0: select 3 did not step at 224,580");
    ds_psg_advance(w, 249156u, 0u, NULL);
    check(c, w->psg_widx[3] == 2u, "F = 0: select 3 did not step again at 249,156 (period 24,576)");
    /* a DDA 1 -> 0 release with chON 1 at w2 = 300,005 restarts at (w2 - w2 mod 6) + 1,536 = 301,536 (ASSUMED:LED-AUD-0004 psg_dda_release_restart) */
    (void)pk_w(w, 0u, 4u, 300000u, &info);
    (void)pk_w(w, 3u, 0x01u, 300001u, &info);
    (void)pk_w(w, 4u, 0xC0u, 300002u, &info);
    check(c, w->psg_rem[4] == 0u && w->psg_widx[4] == 0u, "chON 1 with DDA 1 is not frozen at index 0");
    (void)pk_w(w, 4u, 0x80u, 300005u, &info);
    ds_psg_advance(w, 301535u, 0u, NULL);
    check(c, w->psg_widx[4] == 0u, "DDA release: select 4 stepped before 301,536");
    ds_psg_advance(w, 301536u, 0u, NULL);
    check(c, w->psg_widx[4] == 1u, "DDA release at 300,005: select 4 did not restart with its first step at 301,536");
    snprintf(got, gotcap, "rows=13");
    release_boot(&b);
}

void kind_psg_lfo_trg(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct ds_instance *w;
    struct ds_psg_write_info info;
    uint32_t e;
    uint32_t i;
    uint8_t *snap = NULL;
    size_t size;

    if (!pk_rows_all(k, c, "psg_lfo_trg")) {
        return;
    }
    w = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b);
    if (w == NULL) {
        return;
    }
    /* R9 $80 sets widx[1] = 0 and freezes select 1, LFO_RESET (MAN:P8-6) */
    pk_reset(w);
    (void)pk_w(w, 0u, 1u, 10u, &info);
    (void)pk_w(w, 3u, 0x01u, 20u, &info);
    (void)pk_w(w, 4u, 0x9Fu, 1000u, &info);
    ds_psg_advance(w, 3000u, 0u, NULL);
    check(c, w->psg_rem[1] != 0u && w->psg_widx[1] == 1u, "select 1 keyed on at 1,000 with F = 256 is not running with index 1 at 3,000");
    e = pk_w(w, 9u, 0x80u, 3000u, &info);
    check(c, w->psg_widx[1] == 0u && w->psg_rem[1] == 0u && e == (uint32_t)DS_PSGE_LFO_RESET && info.chan == 0xFEu && info.post == 0x80u && w->psg_lfo_ctl == 0x80u, "R9 $80 did not set widx[1] 0, freeze select 1 and return LFO_RESET");
    ds_psg_advance(w, 5002u, 0u, NULL);
    check(c, w->psg_widx[1] == 0u, "select 1 stepped under TRG 1");
    /* R9 $00 with select 1 chON 1 at w = 5,003 restarts at (w - w mod 6) + 6 x 256 = 6,534 (ASSUMED:LED-AUD-0004 psg_trg_release_restart) */
    e = pk_w(w, 9u, 0x00u, 5003u, &info);
    check(c, e == 0u && w->psg_rem[1] == 6535u, "R9 $00 at 5,003 did not restart select 1 with its next event at 6,534 (distance 6,535 from B = -1)");
    ds_psg_advance(w, 6533u, 0u, NULL);
    check(c, w->psg_widx[1] == 0u, "TRG release: select 1 stepped before 6,534");
    ds_psg_advance(w, 6534u, 0u, NULL);
    check(c, w->psg_widx[1] == 1u, "TRG release: select 1 did not step at 6,534");
    /* R9 with CTL != 0 sets UNMODELLED and stores value & $83 (D-192, until plan 05-09) */
    e = pk_w(w, 9u, 0x7Du, 7000u, &info);
    check(c, e == (uint32_t)DS_PSGE_UNMODELLED && w->psg_lfo_ctl == 0x01u && info.post == 0x01u, "R9 $7D did not store $01 with UNMODELLED");
    e = pk_w(w, 9u, 0xFFu, 7001u, &info);
    check(c, e == ((uint32_t)DS_PSGE_UNMODELLED | (uint32_t)DS_PSGE_LFO_RESET) && w->psg_lfo_ctl == 0x83u, "R9 $FF did not store $83 with LFO_RESET and UNMODELLED");
    /* D-231: R9 $80, R0 1, R4 $00 and five R6 writes leave widx[1] = 5 under TRG 1; the state is reachable, and its snapshot restores */
    pk_reset(w);
    (void)ds_psg_write(w, 9u, 0x80u, 0u, &info);
    (void)ds_psg_write(w, 0u, 0x01u, 0u, &info);
    (void)ds_psg_write(w, 4u, 0x00u, 0u, &info);
    for (i = 0u; i < 5u; i++) {
        (void)ds_psg_write(w, 6u, 0x10u + i, 0u, &info);
    }
    check(c, w->psg_widx[1] == 5u && w->psg_rem[1] == 0u && w->psg_lfo_ctl == 0x80u && w->psg_wave[32] == 0x10u, "R6 writes under TRG 1 did not store and advance widx[1] to 5 (D-190, D-231)");
    size = ds_snapshot_size();
    snap = (uint8_t *)malloc(size);
    if (snap == NULL) {
        harness_error(c, "out of memory for the snapshot");
        release_boot(&b);
        return;
    }
    check(c, ds_snapshot(w, snap, size, NULL) == (uint32_t)DS_SNAPREF_NONE && ds_restore(w, snap, size) == (uint32_t)DS_SNAPREF_NONE && w->psg_widx[1] == 5u, "the TRG state with widx[1] = 5 is not restored (D-231 dropped the rule TRG implies widx[1] = 0)");
    free(snap);
    snprintf(got, gotcap, "rows=10");
    release_boot(&b);
}

/* the v4 seed 1 wave RAM: bytes 0..15 and the SHA-256 of all 192, from an independent splitmix64 script outside the repository (ASSUMED:LED-PROF-0003; method in the case comment) */
static const uint8_t pk_v4s1_head[16] = {0x08u, 0x0Du, 0x15u, 0x0Cu, 0x1Bu, 0x19u, 0x03u, 0x02u, 0x0Eu, 0x0Au, 0x1Bu, 0x19u, 0x0Au, 0x14u, 0x11u, 0x03u};
static const char pk_v4s1_sha[] = "0a36d3ccd3e9eeeff9b4ba6851d0934ede245ecc0b53b3714df2d832f7ebcec4";

static int pk_psg_constants(const struct ds_instance *w)
{
    unsigned regs = (unsigned)w->psg_select | w->psg_main_bal | w->psg_lfo_freq | w->psg_lfo_ctl | w->psg_noise[0] | w->psg_noise[1];
    unsigned i;

    for (i = 0u; i < 6u; i++) {
        regs |= (unsigned)w->psg_freq[i] | w->psg_ctl[i] | w->psg_bal[i] | w->psg_widx[i] | w->psg_dda[i] | (w->psg_rem[i] != 0u ? 1u : 0u);
    }
    for (i = 0u; i < 6u; i++) {
        regs |= w->psg_pad[i];
    }
    return regs == 0u && w->psg_nrem[0] == 1u && w->psg_nrem[1] == 1u && w->psg_lfsr[0] == 1u && w->psg_lfsr[1] == 1u;
}

void kind_psg_poweron(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const req[10] = {"", "bind.powerup_policy=seeded-prng-v1;seed=1", "bind.powerup_policy=seeded-prng-v1;seed=2", "bind.powerup_policy=seeded-prng-v2;seed=1", "bind.powerup_policy=seeded-prng-v2;seed=2",
                                        "bind.powerup_policy=seeded-prng-v3;seed=1", "bind.powerup_policy=seeded-prng-v3;seed=2", "bind.powerup_policy=seeded-prng-v4;seed=1", "bind.powerup_policy=seeded-prng-v4;seed=2", "bind.powerup_policy=zero"};
    static const uint64_t fold_t[3] = {0u, 11904u, 357629u};
    static const uint32_t want_lfsr[3] = {0x20000u, 0x30000u, 0x107EFu};
    static const uint32_t want_nrem[3] = {11904u, 11904u, 11395u};
    struct booted b;
    struct booted b3;
    struct ds_instance *w;
    struct ds_instance *w3;
    uint8_t h[32];
    char hex[65];
    unsigned i;
    char msg[200];

    if (!pk_rows_all(k, c, "psg_poweron")) {
        return;
    }
    for (i = 0u; i < 10u; i++) {
        int is_v4 = i == 7u || i == 8u;

        w = pk_boot(c, k->val[K_FIXTURE], req[i], &b);
        if (w == NULL) {
            return;
        }
        snprintf(msg, sizeof msg, "policy row %u: a PSG register, index, latch or distance is not 0, or a noise distance or LFSR is not 1 (D-193, D-204)", i);
        check(c, pk_psg_constants(w), msg);
        if (!is_v4) {
            unsigned j;
            unsigned any = 0u;

            for (j = 0u; j < sizeof w->psg_wave; j++) {
                any |= w->psg_wave[j];
            }
            snprintf(msg, sizeof msg, "policy row %u: the wave RAM is not zero (only seeded-prng-v4 draws it)", i);
            check(c, any == 0u, msg);
        }
        if (i == 7u) {
            ds_sha256(w->psg_wave, sizeof w->psg_wave, h);
            for (unsigned j = 0u; j < 32u; j++) {
                snprintf(hex + 2u * j, 3u, "%02x", (unsigned)h[j]);
            }
            check(c, memcmp(w->psg_wave, pk_v4s1_head, sizeof pk_v4s1_head) == 0, "v4 seed 1: wave bytes 0..15 differ from the independently derived literal");
            check(c, strcmp(hex, pk_v4s1_sha) == 0, "v4 seed 1: the SHA-256 of the 192 wave bytes differs from the independently derived literal");
            w3 = pk_boot(c, k->val[K_FIXTURE], req[5], &b3);
            if (w3 != NULL) {
                check(c, memcmp(w->vdc_vram, w3->vdc_vram, sizeof w->vdc_vram) == 0 && memcmp(w->vdc_satb, w3->vdc_satb, sizeof w->vdc_satb) == 0 && memcmp(w->vce_pal, w3->vce_pal, sizeof w->vce_pal) == 0 && memcmp(w->ram8k, w3->ram8k, sizeof w->ram8k) == 0,
                      "v4 seed 1: the RAM, VRAM, SATB or palette bytes differ from v3 seed 1 (v4 draws every v3 domain first)");
                release_boot(&b3);
            }
        }
        if (i == 8u) {
            unsigned j;
            unsigned high = 0u;
            unsigned any = 0u;

            for (j = 0u; j < sizeof w->psg_wave; j++) {
                high |= (unsigned)w->psg_wave[j] & 0xE0u;
                any |= w->psg_wave[j];
            }
            check(c, high == 0u && any != 0u, "v4 seed 2: the wave RAM is all zero or holds a byte above 31");
        }
        release_boot(&b);
    }
    /* the noise counters from time 0 on a fresh instance (no PSG write, NFc 31, period 11,904), folded with step_end 1 to T: RF-11 closed form, ASSUMED:LED-AUD-0010 + LED-AUD-0012 */
    for (i = 0u; i < 3u; i++) {
        w = pk_boot(c, k->val[K_FIXTURE], "", &b);
        if (w == NULL) {
            return;
        }
        ds_psg_advance(w, fold_t[i], 1u, NULL);
        snprintf(msg, sizeof msg, "folded to %u: LFSRs $%05X/$%05X and noise distances %u/%u, not $%05X and %u", (unsigned)fold_t[i], (unsigned)w->psg_lfsr[0], (unsigned)w->psg_lfsr[1], (unsigned)w->psg_nrem[0], (unsigned)w->psg_nrem[1], (unsigned)want_lfsr[i],
                 (unsigned)want_nrem[i]);
        check(c, w->psg_lfsr[0] == want_lfsr[i] && w->psg_lfsr[1] == want_lfsr[i] && w->psg_nrem[0] == want_nrem[i] && w->psg_nrem[1] == want_nrem[i], msg);
        release_boot(&b);
    }
    snprintf(got, gotcap, "rows=13");
}

void kind_psg_restore_divisor(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b3;
    struct booted b6;
    struct booted bt;
    struct ds_instance *w3;
    struct ds_instance *w6;
    struct ds_instance *t3;
    uint8_t *snap;
    size_t size = ds_snapshot_size();

    if (!pk_rows_all(k, c, "psg_restore_divisor")) {
        return;
    }
    w3 = pk_boot(c, k->val[K_FIXTURE], "bind.psg_tick_divisor=3", &b3);
    if (w3 == NULL) {
        return;
    }
    w6 = pk_boot(c, k->val[K_FIXTURE], "", &b6);
    if (w6 == NULL) {
        release_boot(&b3);
        return;
    }
    t3 = pk_boot(c, k->val[K_FIXTURE], "bind.psg_tick_divisor=3", &bt);
    snap = (uint8_t *)malloc(size);
    if (t3 == NULL || snap == NULL) {
        harness_error(c, t3 == NULL ? "the d = 3 twin did not boot" : "out of memory for the snapshot");
        free(snap);
        release_boot(&b3);
        release_boot(&b6);
        if (t3 != NULL) {
            release_boot(&bt);
        }
        return;
    }
    check(c, ds_psg_divisor(w3) == 3u && ds_psg_divisor(w6) == 6u, "ds_psg_divisor does not read 3 and 6 from the two bindings");
    w3->psg_nrem[0] = 4u; /* the next noise events at machine time 3: on the tick grid of d = 3, off the grid of d = 6 */
    w3->psg_nrem[1] = 4u;
    check(c, ds_snapshot(w3, snap, size, NULL) == (uint32_t)DS_SNAPREF_NONE, "ds_snapshot of the d = 3 instance was refused");
    check(c, ds_restore(w6, snap, size) == (uint32_t)DS_SNAPREF_profile, "a d = 3 snapshot restored into a default (d = 6) instance is not refused as snapshot.profile (D-223)");
    check(c, ds_restore(t3, snap, size) == (uint32_t)DS_SNAPREF_NONE && t3->psg_nrem[0] == 4u, "a d = 3 snapshot is not restored by a d = 3 instance");
    free(snap);
    release_boot(&b3);
    release_boot(&b6);
    release_boot(&bt);
    snprintf(got, gotcap, "rows=3");
}

/* ---- psg_fold (plan 05-04 Task 2; D-185, D-186, D-195, D-200, D-221, F-64; the kind_vdc_fold shape) ----
 * expect seed=N;partitions=P. A program of register writes drawn from a splitmix64 stream of the seed (E0: the stream only chooses the program) is folded on a fresh instance in one jump (machine time 0, every fold step_end 0 to the next write, then to the end) and again in P random
 * step partitions (steps of 1 to 600 master clocks, D-221: each step's writes folded with step_end 0, then a step_end 1 fold to the step's last clock and the machine time moved past it, as ds_cpu_step_obs does). Both carry a pcm channel. The PSG registers, wave RAM, indices and LFSRs
 * must be equal, every next event at the same absolute time, and the samples byte-identical; after every step_end fold the D-186 invariant holds (1 <= r <= the largest period, r = 0 exactly when frozen, (machine_time - 1 + r) mod d = 0); out-of-step folds are not asserted (F-64).
 * The tie order of D-185 at a write: an R6 write to a running channel at its next step event E minus 1 stores at the old index, at E and at E + 1 at the new one (the event at the write stamp is processed first; E = (w - w mod d) + d x F after a key-on at w, the formula written here);
 * a key-on stamped at a sample instant is heard from the next sample on (the sample at the stamp sees the state before the write). */

static uint32_t pf_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

#define PF_END 240000u
#define PF_FRAMES (PF_END / 192u + 2u)

static uint64_t pf_next(uint64_t *s)
{
    uint64_t z;

    *s += UINT64_C(0x9E3779B97F4A7C15);
    z = *s;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

struct pf_w {
    uint64_t t;
    uint32_t reg;
    uint32_t val;
};

#define PF_WMAX 160u

/* the program of a seed: every channel set up (select, F of 8 to 71, R5 $FF, a few wave words, key-on with or without DDA), R1 $FF, then writes of any register at random later stamps */
static unsigned pf_program(uint64_t seed, struct pf_w *w)
{
    uint64_t s = seed;
    uint64_t t = 16u;
    unsigned n = 0u;
    unsigned ch;

    w[n++] = (struct pf_w){t, 1u, 0xFFu};
    for (ch = 0u; ch < 6u; ch++) {
        unsigned j;

        w[n++] = (struct pf_w){t += 30u, 0u, ch};
        w[n++] = (struct pf_w){t += 30u, 2u, (uint32_t)(8u + pf_next(&s) % 64u)};
        w[n++] = (struct pf_w){t += 30u, 3u, 0u};
        w[n++] = (struct pf_w){t += 30u, 5u, 0xFFu};
        for (j = 0u; j < 4u; j++) {
            w[n++] = (struct pf_w){t += 30u, 6u, (uint32_t)(pf_next(&s) & 0x1Fu)};
        }
        w[n++] = (struct pf_w){t += 30u, 7u, (uint32_t)(pf_next(&s) & 0x9Fu)};
        w[n++] = (struct pf_w){t += 30u, 4u, (pf_next(&s) & 3u) == 0u ? 0xDFu : 0x9Fu};
    }
    while (n < PF_WMAX) {
        uint64_t r = pf_next(&s);

        t += 1u + r % 2900u;
        if (t >= (uint64_t)PF_END - 2u) {
            break;
        }
        w[n++] = (struct pf_w){t, (uint32_t)((r >> 16) % 10u), (uint32_t)((r >> 24) & 0xFFu)};
    }
    return n;
}

/* the frozen rule of D-185: chON 0, DDA 1, or select 1 with TRG 1 */
static int pf_frozen(const struct ds_instance *w, uint32_t ch)
{
    return (w->psg_ctl[ch] & 0x80u) == 0u || (w->psg_ctl[ch] & 0x40u) != 0u || (ch == 1u && (w->psg_lfo_ctl & 0x80u) != 0u);
}

/* the D-186 invariant between steps: 0 when it holds */
static int pf_invariant(const struct ds_instance *w, uint32_t d)
{
    uint32_t k;

    for (k = 0u; k < 6u; k++) {
        uint32_t r = w->psg_rem[k];

        if (pf_frozen(w, k) ? r != 0u : (r == 0u || r > 6u * 4096u || (w->machine_time - 1u + r) % d != 0u)) {
            return -1;
        }
    }
    for (k = 0u; k < 2u; k++) {
        uint32_t r = w->psg_nrem[k];

        if (r == 0u || r > 6u * 64u * 31u || (w->machine_time - 1u + r) % d != 0u) {
            return -1;
        }
    }
    return 0;
}

struct pf_pcm {
    struct ds_pcm_sink fsk;
    struct ds_run_request rq;
    struct ds_obs_log log;
    uint8_t frames[PF_FRAMES * 8u];
};

static void pf_pcm_open(struct pf_pcm *p)
{
    memset(p, 0, sizeof *p);
    p->fsk.frames = p->frames;
    p->fsk.capacity = PF_FRAMES;
    p->rq.pcm = &p->fsk;
    ds_obs_begin(&p->log, &p->rq, UINT64_C(0));
}

/* the absolute time of a counter's next event: B + r with B = machine_time - 1 (D-186); 0 for a frozen channel */
static uint64_t pf_abs(const struct ds_instance *w, uint32_t r)
{
    return r == 0u ? 0u : w->machine_time - 1u + (uint64_t)r;
}

void kind_psg_fold(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    uint64_t seed = 0u;
    uint64_t parts = 0u;
    struct pf_w prog[PF_WMAX];
    unsigned np;
    struct booted b1;
    struct booted b2;
    struct ds_instance *w1;
    struct ds_instance *w2;
    struct pf_pcm *p1 = NULL;
    struct pf_pcm *p2 = NULL;
    struct ds_psg_write_info info;
    uint64_t steps = 0u;
    unsigned bad_inv = 0u;
    uint64_t part;
    uint32_t d;
    unsigned i;

    if (!k->has[K_EXPECT] || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a psg_fold case needs expect = seed=N;partitions=P");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "seed=", 5u) == 0 && parse_u64(item + 5, &seed) == 0) {
            continue;
        }
        if (strncmp(item, "partitions=", 11u) == 0 && parse_u64(item + 11, &parts) == 0 && parts >= 1u && parts <= 64u) {
            continue;
        }
        harness_error(c, "a psg_fold case needs expect = seed=N;partitions=P (P 1 to 64)");
        return;
    }
    if (parts == 0u) {
        harness_error(c, "a psg_fold case needs expect = seed=N;partitions=P (P 1 to 64)");
        return;
    }
    np = pf_program(seed, prog);
    w1 = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b1);
    if (w1 == NULL) {
        return;
    }
    w2 = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b2);
    p1 = (struct pf_pcm *)malloc(sizeof *p1);
    p2 = (struct pf_pcm *)malloc(sizeof *p2);
    if (w2 == NULL || p1 == NULL || p2 == NULL) {
        if (w2 != NULL) {
            release_boot(&b2);
        }
        release_boot(&b1);
        free(p1);
        free(p2);
        if (!c->harness) {
            harness_error(c, "out of memory for the psg_fold kind");
        }
        return;
    }
    d = ds_psg_divisor(w1);
    /* the one jump */
    pk_reset(w1);
    pf_pcm_open(p1);
    for (i = 0u; i < np; i++) {
        ds_psg_advance(w1, prog[i].t, 0u, &p1->log);
        (void)ds_psg_write(w1, prog[i].reg, prog[i].val, prog[i].t, &info);
    }
    ds_psg_advance(w1, (uint64_t)PF_END - 1u, 0u, &p1->log);
    ds_obs_end(&p1->log);
    /* the partitions: P runs with different step lengths, each against the one jump */
    for (part = 0u; part < parts; part++) {
        uint64_t s = seed ^ (UINT64_C(0xA5A5A5A5) * (part + 1u));
        uint64_t m = 0u;
        unsigned wi = 0u;
        int same = 1;
        uint32_t ch;
        char msg[200];

        pk_reset(w2);
        pf_pcm_open(p2);
        while (m < (uint64_t)PF_END) {
            uint64_t len = 1u + pf_next(&s) % 600u;

            if (len > (uint64_t)PF_END - m) {
                len = (uint64_t)PF_END - m;
            }
            while (wi < np && prog[wi].t < m + len) {
                ds_psg_advance(w2, prog[wi].t, 0u, &p2->log);
                (void)ds_psg_write(w2, prog[wi].reg, prog[wi].val, prog[wi].t, &info);
                wi++;
            }
            ds_psg_advance(w2, m + len - 1u, 1u, &p2->log);
            m += len;
            w2->machine_time = m;
            steps++;
            if (pf_invariant(w2, d) != 0) {
                bad_inv++;
            }
        }
        ds_obs_end(&p2->log);
        same = memcmp(w1->psg_freq, w2->psg_freq, sizeof w1->psg_freq) == 0 && memcmp(w1->psg_ctl, w2->psg_ctl, sizeof w1->psg_ctl) == 0 && memcmp(w1->psg_bal, w2->psg_bal, sizeof w1->psg_bal) == 0 &&
               memcmp(w1->psg_widx, w2->psg_widx, sizeof w1->psg_widx) == 0 && memcmp(w1->psg_dda, w2->psg_dda, sizeof w1->psg_dda) == 0 && memcmp(w1->psg_wave, w2->psg_wave, sizeof w1->psg_wave) == 0 &&
               memcmp(w1->psg_noise, w2->psg_noise, sizeof w1->psg_noise) == 0 && memcmp(w1->psg_lfsr, w2->psg_lfsr, sizeof w1->psg_lfsr) == 0 && w1->psg_select == w2->psg_select && w1->psg_main_bal == w2->psg_main_bal &&
               w1->psg_lfo_freq == w2->psg_lfo_freq && w1->psg_lfo_ctl == w2->psg_lfo_ctl;
        for (ch = 0u; ch < 6u; ch++) {
            same = same && pf_abs(w1, w1->psg_rem[ch]) == pf_abs(w2, w2->psg_rem[ch]);
        }
        for (ch = 0u; ch < 2u; ch++) {
            same = same && pf_abs(w1, w1->psg_nrem[ch]) == pf_abs(w2, w2->psg_nrem[ch]);
        }
        snprintf(msg, sizeof msg, "partition %llu: the PSG state after the stepped folds differs from the one jump (registers, wave RAM, indices, LFSRs or the absolute time of a next event)", (unsigned long long)part);
        check(c, same, msg);
        snprintf(msg, sizeof msg, "partition %llu: the samples of the stepped folds differ from the one jump (count %u against %u)", (unsigned long long)part, (unsigned)pf_u32(p2->fsk.header), (unsigned)pf_u32(p1->fsk.header));
        check(c, pf_u32(p1->fsk.header) == PF_END / 192u && pf_u32(p2->fsk.header) == pf_u32(p1->fsk.header) && pf_u32(p1->fsk.header + 4) == 0u && memcmp(p1->frames, p2->frames, (size_t)pf_u32(p1->fsk.header) * 8u) == 0, msg);
    }
    check(c, bad_inv == 0u, "the D-186 invariant (1 <= r <= the largest period, r = 0 exactly when frozen, the tick grid) failed after a step_end fold");
    /* the tie order of D-185 at an R6 write to a running channel: select 0, F = 10, key-on at w = 1003; E = (w - w mod d) + d x 10 */
    {
        static const int64_t dts[3] = {-1, 0, 1};
        static const uint8_t want[3] = {0u, 1u, 1u}; /* the wave index after the held store: the old index before the event, the new one at and after it */
        uint64_t kw = 1003u;
        uint64_t e = kw - kw % d + (uint64_t)d * 10u;
        unsigned j;

        for (j = 0u; j < 3u; j++) {
            uint64_t t = (uint64_t)((int64_t)e + dts[j]);
            char msg[160];

            pk_reset(w2);
            (void)pk_w(w2, 0u, 0u, 10u, &info);
            (void)pk_w(w2, 2u, 10u, 20u, &info);
            (void)pk_w(w2, 3u, 0u, 30u, &info);
            (void)pk_w(w2, 4u, 0x9Fu, kw, &info);
            (void)pk_w(w2, 6u, 0x15u, t, &info);
            snprintf(msg, sizeof msg, "an R6 write at the next step event %s gives wave index %u, the tie order of D-185 gives %u", j == 0u ? "minus 1" : j == 1u ? "itself" : "plus 1", (unsigned)info.wave_idx, (unsigned)want[j]);
            check(c, info.wave_idx == want[j] && (info.effect & (uint8_t)DS_PSGE_WAVE_HELD) != 0u, msg);
        }
    }
    /* a key-on stamped at the sample instant 20 x 192 = 3840: sample 20 sees the state before the write (0), sample 21 hears the channel */
    {
        uint32_t j;

        pk_reset(w2);
        pf_pcm_open(p2);
        (void)pk_w(w2, 1u, 0xFFu, 10u, &info);
        (void)pk_w(w2, 0u, 0u, 20u, &info);
        for (j = 0u; j < 32u; j++) {
            (void)pk_w(w2, 6u, 31u, 30u + j, &info);
        }
        (void)pk_w(w2, 5u, 0xFFu, 100u, &info);
        (void)pk_w(w2, 2u, 64u, 110u, &info);
        ds_psg_advance(w2, 3840u, 0u, &p2->log);
        (void)ds_psg_write(w2, 4u, 0x9Fu, 3840u, &info);
        ds_psg_advance(w2, 3840u + 192u, 0u, &p2->log);
        ds_obs_end(&p2->log);
        check(c, pf_u32(p2->fsk.header) == 22u && pf_u32(p2->frames + 20u * 8u) == 0u && pf_u32(p2->frames + 21u * 8u) != 0u,
              "a key-on stamped at the sample instant 3840 is heard in sample 20, or not in sample 21: the sample at the stamp must see the state before the write");
    }
    release_boot(&b1);
    release_boot(&b2);
    free(p1);
    free(p2);
    snprintf(got, gotcap, "writes=%u partitions=%llu steps=%llu d=%u", np, (unsigned long long)parts, (unsigned long long)steps, (unsigned)d);
}

/* ---- plan 05-06: psg_vol_table and psg_pure_function ---- */

/* the volume curve as the core holds it, through the X-macro of src/core/psg_vol.def (never re-derived here; LED-AUD-0009) */
static const uint32_t pk_vol[] = {
#define DS_PSG_VOL(a, value) (uint32_t)(value),
#include "psg_vol.def"
#undef DS_PSG_VOL
};

/*
 * kind psg_vol_table (plan 05-06; D-194, D-195, D-205, D-206; ASSUMED:LED-AUD-0009): expect rows=all. The 30 rows of psg_vol.def strictly decrease from VOL[0] = 32768 to VOL[29] = 219 (the handbook literals), the table holds exactly
 * psg_silence_att = 30 rows, and through ds_psg_sample a select-2 channel with DDA 1 and the latch 31 contributes 31 x 219 = 6,789 at att 29 (R1 $FF, R5 $FF, AL 2) and 0 at att 30 (AL 1) and at att 91 (R1 $00, R5 $00, AL 0): no index of 30 or more is read.
 */
void kind_psg_vol_table(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct ds_instance *w;
    struct ds_psg_write_info info;
    uint32_t i;
    uint32_t l = 0u;
    uint32_t r = 0u;
    unsigned bad = 0u;
    char msg[200];

    if (!pk_rows_all(k, c, "psg_vol_table")) {
        return;
    }
    w = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b);
    if (w == NULL) {
        return;
    }
    check(c, sizeof pk_vol / sizeof pk_vol[0] == 30u && (uint32_t)DS_CONST_psg_silence_att == 30u, "psg_vol.def does not hold exactly psg_silence_att = 30 rows");
    for (i = 1u; i < (uint32_t)(sizeof pk_vol / sizeof pk_vol[0]); i++) {
        bad += pk_vol[i] < pk_vol[i - 1u] ? 0u : 1u;
    }
    snprintf(msg, sizeof msg, "the volume curve is not strictly decreasing (%u rows out of order)", bad);
    check(c, bad == 0u, msg);
    check(c, pk_vol[0] == 32768u && pk_vol[29] == 219u, "VOL[0] is not 32768 or VOL[29] is not 219 (the handbook literals of LED-AUD-0009)");
    pk_reset(w);
    (void)ds_psg_write(w, 0u, 2u, 0u, &info);
    (void)ds_psg_write(w, 4u, 0xC2u, 0u, &info);
    (void)ds_psg_write(w, 6u, 0x1Fu, 0u, &info);
    (void)ds_psg_write(w, 1u, 0xFFu, 0u, &info);
    (void)ds_psg_write(w, 5u, 0xFFu, 0u, &info);
    ds_psg_sample(w, &l, &r);
    snprintf(msg, sizeof msg, "att 29 (AL 2) reads L %u R %u, not 31 x 219 = 6,789 on both sides", (unsigned)l, (unsigned)r);
    check(c, l == 6789u && r == 6789u, msg);
    (void)ds_psg_write(w, 4u, 0xC1u, 0u, &info);
    ds_psg_sample(w, &l, &r);
    snprintf(msg, sizeof msg, "att 30 (AL 1) reads L %u R %u, not 0: an index of 30 was read", (unsigned)l, (unsigned)r);
    check(c, l == 0u && r == 0u, msg);
    (void)ds_psg_write(w, 4u, 0xC0u, 0u, &info);
    (void)ds_psg_write(w, 1u, 0x00u, 0u, &info);
    (void)ds_psg_write(w, 5u, 0x00u, 0u, &info);
    ds_psg_sample(w, &l, &r);
    check(c, l == 0u && r == 0u, "att 91 (R1 $00, R5 $00, AL 0) does not read 0");
    snprintf(got, gotcap, "rows=6");
    release_boot(&b);
}

/*
 * kind psg_pure_function (plan 05-06; D-212; MAN:P8-9+P8-12+H8-23 for 6F, 192F and 384 x NFc, the tick phase ASSUMED:LED-AUD-0012 does not enter): expect rows=all. On a fresh d = 6 state at machine time 0 driven by direct ds_psg_write and ds_psg_advance calls,
 * for F in {1, 254 (P8-12 A4), 3421 (P8-12 C1), 4095} select 0 keyed on at 1,000 shows widx(T + 6F) = widx(T) + 1 mod 32 and widx(T + 192F) = widx(T) at three T after its first step (6F = 6, 1,524, 20,526, 24,570; 192F = 192, 48,768, 656,832, 786,240),
 * and for NFc in {1, 7, 31} (R7 $1E, $18, $00 at select 4) the distance to the next noise event is periodic with period 384 x NFc (384, 2,688, 11,904). Relations only: no key-on phase, no LFSR model.
 */
void kind_psg_pure_function(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const uint32_t fs[4] = {1u, 254u, 3421u, 4095u};
    static const uint64_t step[4] = {6u, 1524u, 20526u, 24570u};
    static const uint64_t cyc[4] = {192u, 48768u, 656832u, 786240u};
    static const uint32_t r7[3] = {0x1Eu, 0x18u, 0x00u};
    static const uint64_t nper[3] = {384u, 2688u, 11904u};
    struct booted b;
    struct ds_instance *w;
    struct ds_psg_write_info info;
    unsigned i;
    unsigned j;
    unsigned bad = 0u;
    char msg[240];

    if (!pk_rows_all(k, c, "psg_pure_function")) {
        return;
    }
    w = pk_boot(c, k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b);
    if (w == NULL) {
        return;
    }
    for (i = 0u; i < 4u; i++) {
        uint64_t t = 1000u + step[i] + 7u; /* past the first step (at most 1,000 + 6F) */

        pk_reset(w);
        (void)pk_w(w, 0u, 0u, 10u, &info);
        (void)pk_w(w, 2u, fs[i] & 0xFFu, 20u, &info);
        (void)pk_w(w, 3u, fs[i] >> 8, 30u, &info);
        (void)pk_w(w, 4u, 0x9Fu, 1000u, &info);
        for (j = 0u; j < 3u; j++) {
            uint32_t a;
            uint32_t b1;
            uint32_t b2;

            ds_psg_advance(w, t, 0u, NULL);
            a = w->psg_widx[0];
            ds_psg_advance(w, t + step[i], 0u, NULL);
            b1 = w->psg_widx[0];
            ds_psg_advance(w, t + cyc[i], 0u, NULL);
            b2 = w->psg_widx[0];
            if (b1 != ((a + 1u) & 31u) || b2 != a) {
                bad++;
                snprintf(msg, sizeof msg, "F = %u at T = %llu: widx(T) %u, widx(T + 6F) %u, widx(T + 192F) %u", (unsigned)fs[i], (unsigned long long)t, (unsigned)a, (unsigned)b1, (unsigned)b2);
                check(c, 0, msg);
            }
            t += cyc[i] + 3u * (uint64_t)(j + 1u);
        }
    }
    check(c, bad == 0u, "a wave index relation widx(T + 6F) = widx(T) + 1, widx(T + 192F) = widx(T) failed");
    for (i = 0u; i < 3u; i++) {
        uint64_t t = 30000u;

        pk_reset(w);
        (void)pk_w(w, 0u, 4u, 10u, &info);
        (void)pk_w(w, 7u, r7[i], 20u, &info);
        for (j = 0u; j < 3u; j++) {
            uint64_t d1;
            uint64_t d2;

            ds_psg_advance(w, t, 0u, NULL);
            d1 = (uint64_t)w->psg_nrem[0] - 1u - t;
            ds_psg_advance(w, t + nper[i], 0u, NULL);
            d2 = (uint64_t)w->psg_nrem[0] - 1u - (t + nper[i]);
            snprintf(msg, sizeof msg, "NFc %u: the distance to the next noise event is %llu at T = %llu and %llu at T + %llu", (unsigned)(nper[i] / 384u), (unsigned long long)d1, (unsigned long long)t, (unsigned long long)d2, (unsigned long long)nper[i]);
            check(c, d1 == d2 && d1 < nper[i], msg);
            t += nper[i] + 1001u;
        }
    }
    snprintf(got, gotcap, "rows=21");
    release_boot(&b);
}
