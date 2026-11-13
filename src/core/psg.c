/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * psg.c - the PSG (plans 05-02, 05-03; D-183..D-186, D-193, D-202..D-204, PR-1). The fold and its two call sites are described in ds_psg.h. Plan 05-03 adds the state: the tick divisor, the register mask table, the power-on values and the fold of the
 * counters (the two noise counters and LFSRs clock from time 0 under every policy, a running channel counter advances its wave index in closed form); plan 05-04 adds the synthesis and the record and frame emission. No allocation, no global, no clock.
 * In-fold times are offsets from the base B = machine_time - 1 (D-221, RF-8): an event at offset o is at time B + o, the stamp is at offset t = stamp - B, and no absolute time near 2^64 is ever formed.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_psg.h"

_Static_assert(6u * (uint32_t)DS_CONST_psg_f0_as == DS_PSG_CHAN_REM_MAX, "the largest channel step at d = 6 is not 24,576");
_Static_assert(6u * (uint32_t)DS_CONST_psg_f0_as * (uint32_t)DS_CONST_psg_lfo_f0_as == DS_PSG_COUPLED_REM_MAX, "the largest coupled step at d = 6 is not 6,291,456");
_Static_assert(6u * 64u * 31u == DS_PSG_NOISE_REM_MAX && 6u * (uint32_t)DS_CONST_psg_noise_nf0_ticks < DS_PSG_NOISE_REM_MAX, "the largest noise period at d = 6 is not 11,904");
_Static_assert(600u + DS_PSG_COUPLED_REM_MAX < (1u << 23), "a fold offset does not stay below 2^23 (D-195, D-221)");
_Static_assert((uint32_t)DS_CONST_psg_lfsr_taps == ((1u << 0) | (1u << 1) | (1u << 11) | (1u << 12) | (1u << 17)), "the taps constant does not name bits 0, 1, 11, 12 and 17");
_Static_assert(DS_CONST_reset_cost_mclk == 0, "the PSG power-on state is the state at machine time 0: the boot must charge no reset cost (RF-15)");

uint32_t ds_psg_divisor(const struct ds_instance *inst)
{
    return ds_streq(inst->profile.field[DS_FIELD_psg_tick_divisor].value, "3") ? 3u : 6u;
}

/* D-189 (MAN:P8-2..P8-9): the storage mask of each register; reg 10..15 store nothing */
static const uint8_t reg_mask[16] = {0x07u, 0xFFu, 0xFFu, 0x0Fu, 0xDFu, 0xFFu, 0x1Fu, 0x9Fu, 0xFFu, 0x83u, 0u, 0u, 0u, 0u, 0u, 0u};

uint32_t ds_psg_reg_mask(uint32_t reg)
{
    return reg < 16u ? (uint32_t)reg_mask[reg] : 0u;
}

void ds_psg_poweron(struct ds_instance *inst)
{
    uint32_t k;

    for (k = 0u; k < 6u; k++) {
        inst->psg_freq[k] = 0u;
        inst->psg_ctl[k] = 0u;
        inst->psg_bal[k] = 0u;
        inst->psg_widx[k] = 0u;
        inst->psg_dda[k] = 0u;
        inst->psg_rem[k] = 0u;
    }
    inst->psg_select = 0u;
    inst->psg_main_bal = 0u;
    inst->psg_lfo_freq = 0u;
    inst->psg_lfo_ctl = 0u;
    for (k = 0u; k < 2u; k++) {
        inst->psg_noise[k] = 0u;
        inst->psg_nrem[k] = 1u; /* the first noise event is at machine time 0 (RF-11) */
        inst->psg_lfsr[k] = (uint32_t)DS_CONST_psg_lfsr_reset;
    }
}

/* one step of an 18-bit noise LFSR (D-193, ASSUMED LED-AUD-0010): the feedback is the parity of bits 0, 1, 11, 12 and 17 (psg_lfsr_taps) and enters at bit 17 */
static uint32_t lfsr_step(uint32_t s)
{
    uint32_t b = (s ^ (s >> 1) ^ (s >> 11) ^ (s >> 12) ^ (s >> 17)) & 1u;

    return ((s >> 1) | (b << 17)) & DS_PSG_LFSR_MASK;
}

/* the noise period of noise channel n (select 4 + n) in master clocks: d x 64 x NFc with NFc = 31 - NFRQ, or d x psg_noise_nf0_ticks when NFc = 0 (D-184) */
static uint64_t noise_period(const struct ds_instance *inst, uint32_t n, uint32_t d)
{
    uint32_t nfc = 31u - ((uint32_t)inst->psg_noise[n] & 0x1Fu);

    return (uint64_t)d * (nfc != 0u ? 64u * nfc : (uint32_t)DS_CONST_psg_noise_nf0_ticks);
}

/* the step period of channel ch in master clocks: d x Fe with Fe = F, or psg_f0_as when F = 0 (D-184); uncoupled for every channel until plan 05-09 */
static uint64_t chan_period(const struct ds_instance *inst, uint32_t ch, uint32_t d)
{
    uint32_t f = (uint32_t)inst->psg_freq[ch] & 0xFFFu;

    return (uint64_t)d * (f != 0u ? f : (uint32_t)DS_CONST_psg_f0_as);
}

void ds_psg_advance(struct ds_instance *inst, uint64_t stamp, uint32_t step_end, struct ds_obs_log *obs)
{
    uint64_t base_t; /* B + 1 = machine_time, the first unfolded instant */
    uint64_t t;      /* the stamp as an offset from B */
    uint64_t no[2];
    uint32_t d;
    uint32_t k;

    (void)obs; /* emission lands in plan 05-04 */
    base_t = inst->machine_time;
    if (stamp < base_t) {
        return; /* nothing at or before the stamp is unfolded; a rebase to B itself changes nothing */
    }
    t = stamp - base_t + 1u;
    d = ds_psg_divisor(inst);
    /* the noise counters: every event at an offset <= t in time order, select 4's counter before select 5's at one instant; each event is one LFSR step and reloads with the period in force (D-185, D-193) */
    no[0] = inst->psg_nrem[0];
    no[1] = inst->psg_nrem[1];
    for (;;) {
        uint32_t n = no[1] < no[0] ? 1u : 0u;

        if (no[n] > t) {
            break;
        }
        inst->psg_lfsr[n] = lfsr_step(inst->psg_lfsr[n]);
        no[n] += noise_period(inst, n, d);
    }
    /* the channel counters in closed form (RESEARCH section 4): a running counter with its next event at offset o <= t steps k = (t - o) / q + 1 times with the period q read at its last reload, and its wave index advances by k mod 32 */
    for (k = 0u; k < 6u; k++) {
        uint64_t o = inst->psg_rem[k];

        if (o != 0u && o <= t) {
            uint64_t q = chan_period(inst, k, d);
            uint64_t steps = (t - o) / q + 1u;

            inst->psg_widx[k] = (uint8_t)(((uint32_t)inst->psg_widx[k] + (uint32_t)(steps & 31u)) & 31u);
            o += steps * q;
        }
        /* step_end 1 rebases a running counter to the stamp (its next event stays at stamp + r); step_end 0 keeps it relative to B */
        inst->psg_rem[k] = o == 0u ? 0u : (uint32_t)(step_end != 0u ? o - t : o);
    }
    for (k = 0u; k < 2u; k++) {
        inst->psg_nrem[k] = (uint32_t)(step_end != 0u ? no[k] - t : no[k]);
    }
}
