/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * psg.c - the PSG (plans 05-02, 05-03; D-183..D-186, D-188..D-193, D-202..D-204, D-220, PR-1, PR-2, PR-7). The fold and its two call sites are described in ds_psg.h. Plan 05-03 adds the state, the register decode and storage (ds_psg_write), the tick divisor,
 * the register mask table, the power-on values and the fold of the
 * counters (the two noise counters and LFSRs clock from time 0 under every policy, a running channel counter advances its wave index in closed form); plan 05-04 adds the synthesis (ds_psg_sample, the volume curve psg_vol.def) and the frame emission on the 192-clock grid
 * (the DSPE record of a write is built in bus.c); plan 05-07 adds the noise level under NE (psg_noise_bit, RF-6). No allocation, no global, no clock.
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
_Static_assert(DS_CONST_psg_tick_phase_mclk == 0 && DS_CONST_psg_sample_phase_mclk == 0,
               "psg.c (the tick grid stamp % d, the sample instants cursor x 192) and obs.c (first = ceil(T0 / 192)) implement only phase 0 of both grids (LED-AUD-0012, LED-AUD-0014): implement a phase before printing another value");

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

/* the output of the last step of an LFSR whose state after that step is post (RF-6): the bit shifted out is the parity of post masked $30C01 (bits 0, 10, 11, 16 and 17 of post are bits 1, 11, 12, 17 and the feedback of the state before), so PSGT stores no output bit */
_Static_assert((((uint32_t)DS_CONST_psg_lfsr_taps >> 1) | (1u << 17)) == 0x30C01u, "the RF-6 output mask $30C01 does not follow from the taps 0, 1, 11, 12 and 17");
static uint32_t psg_noise_bit(uint32_t post)
{
    return (post ^ (post >> 10) ^ (post >> 11) ^ (post >> 16) ^ (post >> 17)) & 1u;
}

/* the noise period of noise channel n (select 4 + n) in master clocks: d x 64 x NFc with NFc = 31 - NFRQ, or d x psg_noise_nf0_ticks when NFc = 0 (D-184) */
static uint64_t noise_period(const struct ds_instance *inst, uint32_t n, uint32_t d)
{
    uint32_t nfc = 31u - ((uint32_t)inst->psg_noise[n] & 0x1Fu);

    return (uint64_t)d * (nfc != 0u ? 64u * nfc : (uint32_t)DS_CONST_psg_noise_nf0_ticks);
}

/* the step period of channel ch in master clocks: d x Fe with Fe = F, or psg_f0_as when F = 0 (D-184); uncoupled for every channel until plan 05.1-01 (Phase 5.1, D-234) */
static uint64_t chan_period(const struct ds_instance *inst, uint32_t ch, uint32_t d)
{
    uint32_t f = (uint32_t)inst->psg_freq[ch] & 0xFFFu;

    return (uint64_t)d * (f != 0u ? f : (uint32_t)DS_CONST_psg_f0_as);
}

/* the volume curve (psg_vol.def, LED-AUD-0009): VOL[a] for a = 0..29, indexed only below psg_silence_att (D-194, D-195) */
static const uint32_t vol[] = {
#define DS_PSG_VOL(a, value) value,
#include "psg_vol.def"
#undef DS_PSG_VOL
};
_Static_assert(sizeof vol / sizeof vol[0] == (size_t)DS_CONST_psg_silence_att, "psg_vol.def must hold exactly psg_silence_att rows: VOL is indexed only below the silence threshold");
_Static_assert(6u * 31u * 32768u < (1u << 31), "the sum of six channels at level 31 and VOL[0] (6,094,848) must stay below 2^31 (D-194, D-195)");

/* every event at an offset <= t from B, with no rebase (PR-1): the noise counters in time order, select 4's counter before select 5's at one instant, each event one LFSR step and a reload with the period in force (D-185, D-193); then every running channel counter in closed form
 * (RESEARCH section 4): with its next event at offset o <= t it steps k = (t - o) / q + 1 times with the period q read at its last reload, and its wave index advances by k mod 32. Called once per sample instant and once at the stamp, so a repeated call with a larger t continues the fold */
static void fold_to(struct ds_instance *inst, uint64_t t, uint32_t d)
{
    uint64_t no[2];
    uint32_t k;

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
    inst->psg_nrem[0] = (uint32_t)no[0];
    inst->psg_nrem[1] = (uint32_t)no[1];
    for (k = 0u; k < 6u; k++) {
        uint64_t o = inst->psg_rem[k];

        if (o != 0u && o <= t) {
            uint64_t q = chan_period(inst, k, d);
            uint64_t steps = (t - o) / q + 1u;

            inst->psg_widx[k] = (uint8_t)(((uint32_t)inst->psg_widx[k] + (uint32_t)(steps & 31u)) & 31u);
            inst->psg_rem[k] = (uint32_t)(o + steps * q);
        }
    }
}

/*
 * The fold (D-186, PR-1). With a pcm sink (obs != NULL and its pcm channel on: the only read of the sink besides the emit, D-200) every sample instant s x 192 from the log's pcm cursor up to the stamp is visited in time order: the events at or before that instant are folded first
 * (channel steps and noise clocks before the sample, D-185 tie order), then ds_psg_sample (const) reads the state and ds_obs_pcm emits the frame. A CPU write stamped exactly at a sample instant is applied after that sample, because the write path folds to its stamp before ds_psg_write.
 * The sample loop runs at most span / 192 + 1 times (D-195). Without a pcm sink every event is folded the same way and only the sampling is skipped (D-200). step_end 1 rebases the distances to the stamp at the end.
 */
void ds_psg_advance(struct ds_instance *inst, uint64_t stamp, uint32_t step_end, struct ds_obs_log *obs)
{
    uint64_t base_t; /* B + 1 = machine_time, the first unfolded instant */
    uint64_t t;      /* the stamp as an offset from B */
    uint32_t d;
    uint32_t k;

    base_t = inst->machine_time;
    if (stamp < base_t) {
        return; /* nothing at or before the stamp is unfolded; a rebase to B itself changes nothing */
    }
    t = stamp - base_t + 1u;
    d = ds_psg_divisor(inst);
    if (obs != NULL && obs->pcm.on) {
        uint64_t lim = (stamp - base_t) / (uint64_t)DS_CONST_psg_mclk_per_sample + 1u;
        uint64_t n;

        for (n = 0u; n < lim && obs->pcm.cursor <= stamp / (uint64_t)DS_CONST_psg_mclk_per_sample; n++) {
            uint64_t si = obs->pcm.cursor * (uint64_t)DS_CONST_psg_mclk_per_sample;
            uint32_t l;
            uint32_t r;

            fold_to(inst, si >= base_t ? si - base_t + 1u : 0u, d); /* the cursor never lags the first unfolded instant inside ds_run (ds_obs_begin sets it to ceil(T0 / 192)); a lagging one would sample the state as it is */
            ds_psg_sample(inst, &l, &r);
            ds_obs_pcm(obs, l, r);
            obs->pcm.cursor = obs->pcm.cursor + 1u;
        }
    }
    fold_to(inst, t, d);
    if (step_end != 0u) { /* the next events stay at stamp + r: a running counter and both noise counters are rebased to the stamp; a frozen channel keeps 0 */
        for (k = 0u; k < 6u; k++) {
            if (inst->psg_rem[k] != 0u) {
                inst->psg_rem[k] = (uint32_t)((uint64_t)inst->psg_rem[k] - t);
            }
        }
        for (k = 0u; k < 2u; k++) {
            inst->psg_nrem[k] = (uint32_t)((uint64_t)inst->psg_nrem[k] - t);
        }
    }
}

/* the attenuation of one side in steps of 1.5 dB (D-194, MAN:P8-3+P8-4+P8-10): 0..91; one statement, the anchor of the toy psg-atten-max (plan 05-06) */
static uint32_t side_att(uint32_t mal, uint32_t al, uint32_t lal)
{
    return 2u * (15u - mal) + (31u - al) + 2u * (15u - lal);
}

void ds_psg_sample(const struct ds_instance *inst, uint32_t *l, uint32_t *r)
{
    uint32_t lmal = ((uint32_t)inst->psg_main_bal >> 4u) & 0xFu;
    uint32_t rmal = (uint32_t)inst->psg_main_bal & 0xFu;
    uint32_t sum_l = 0u;
    uint32_t sum_r = 0u;
    uint32_t ch;

    for (ch = 0u; ch < 6u; ch++) {
        uint32_t ctl = inst->psg_ctl[ch];
        uint32_t bal = inst->psg_bal[ch];
        uint32_t level;
        uint32_t att;

        if ((ctl & 0x80u) == 0u) {
            continue; /* chON 0: the channel contributes 0 whatever DDA, NE or the latch hold (D-191, MAN:P8-4) */
        }
        /* DDA 1: the latch; else, at select 4 or 5 with NE 1, the noise level (psg_noise_high_level for an output bit 1, else 0; D-191, D-193, plan 05-07; the wave counter keeps running, D-185); else the wave word at the wave index */
        if ((ctl & 0x40u) != 0u) {
            level = (uint32_t)inst->psg_dda[ch] & 0x1Fu;
        } else if (ch >= 4u && ((uint32_t)inst->psg_noise[ch - 4u] & 0x80u) != 0u) {
            level = psg_noise_bit(inst->psg_lfsr[ch - 4u]) != 0u ? (uint32_t)DS_CONST_psg_noise_high_level : 0u;
        } else {
            level = (uint32_t)inst->psg_wave[ch * 32u + ((uint32_t)inst->psg_widx[ch] & 31u)] & 0x1Fu;
        }
        att = side_att(lmal, ctl & 0x1Fu, (bal >> 4u) & 0xFu);
        if (att < (uint32_t)DS_CONST_psg_silence_att) {
            sum_l += level * vol[att];
        }
        att = side_att(rmal, ctl & 0x1Fu, bal & 0xFu);
        if (att < (uint32_t)DS_CONST_psg_silence_att) {
            sum_r += level * vol[att];
        }
    }
    *l = sum_l;
    *r = sum_r;
}

/* a channel counter runs while chON is 1 and DDA is 0, and for select 1 while TRG is 0 (D-185, D-192) */
static uint32_t chan_running(const struct ds_instance *inst, uint32_t ch)
{
    uint32_t ctl = inst->psg_ctl[ch];

    return (ctl & 0x80u) != 0u && (ctl & 0x40u) == 0u && !(ch == 1u && (inst->psg_lfo_ctl & 0x80u) != 0u) ? 1u : 0u;
}

/* key-on or a restart at stamp w (D-185, ASSUMED LED-AUD-0012): the next event is (w - w mod d) + d x Fe, stored as its offset from B = machine_time - 1 (the stamp is at offset w - B >= 1), with the wave index unchanged (psg_keyon_widx_step 0) */
static void chan_start(struct ds_instance *inst, uint32_t ch, uint64_t stamp)
{
    uint32_t d = ds_psg_divisor(inst);
    uint64_t w_off = stamp >= inst->machine_time ? stamp - inst->machine_time + 1u : 0u;

    inst->psg_rem[ch] = (uint32_t)(w_off + chan_period(inst, ch, d) - stamp % d);
}

_Static_assert(DS_CONST_psg_select_67_drop == 1 && DS_CONST_psg_keyon_widx_step == 0, "psg.c implements only the conventions select 6/7 dropped and key-on keeps the wave index (LED-AUD-0004)");

uint32_t ds_psg_write(struct ds_instance *inst, uint32_t reg, uint32_t value, uint64_t stamp, struct ds_psg_write_info *info)
{
    uint32_t m = value & ds_psg_reg_mask(reg);
    uint32_t sel = inst->psg_select;
    uint32_t effect = 0u;
    uint32_t chan = 0xFEu;
    uint32_t post = m;

    info->sel = (uint8_t)sel;
    if (reg >= 10u || (reg >= 2u && reg <= 7u && sel > 5u) || (reg == 7u && sel < 4u)) {
        info->chan = 0xFFu; /* reg $A..$F, R2..R7 at select 6 or 7 (psg_select_67_drop), R7 at select 0..3: nothing is stored */
        info->effect = (uint8_t)DS_PSGE_NO_TARGET;
        info->wave_idx = 0xFFu;
        info->post = 0xFFFFu;
        return (uint32_t)DS_PSGE_NO_TARGET;
    }
    if (reg >= 2u && reg <= 7u) {
        chan = sel;
    }
    switch (reg) {
    case 0u:
        inst->psg_select = (uint8_t)(value & ds_psg_reg_mask(0u)); /* the select mask: one statement (toy target, D-210) */
        break;
    case 1u:
        inst->psg_main_bal = (uint8_t)m;
        break;
    case 2u: /* the frequency applies from the next reload (D-185) */
        inst->psg_freq[sel] = (uint16_t)(((uint32_t)inst->psg_freq[sel] & 0xF00u) | m);
        post = inst->psg_freq[sel];
        break;
    case 3u:
        inst->psg_freq[sel] = (uint16_t)(((uint32_t)inst->psg_freq[sel] & 0x0FFu) | (m << 8));
        post = inst->psg_freq[sel];
        break;
    case 4u: {
        uint32_t was = chan_running(inst, sel);
        uint32_t dda_was = (uint32_t)inst->psg_ctl[sel] & 0x40u;

        inst->psg_ctl[sel] = (uint8_t)m;
        if ((m & 0x40u) != 0u) {
            inst->psg_widx[sel] = 0u; /* DDA 1 resets the wave index (D-190) */
            effect |= (uint32_t)DS_PSGE_IDX_RESET;
        }
        if (chan_running(inst, sel) == 0u) {
            inst->psg_rem[sel] = 0u; /* chON 0, DDA 1 or (select 1) TRG 1 freezes the counter */
        } else if (was == 0u && (dda_was == 0u || DS_CONST_psg_dda_release_restart != 0)) {
            chan_start(inst, sel, stamp); /* key-on (chON 0 -> 1 with DDA 0), or a DDA 1 -> 0 release with chON 1 (psg_dda_release_restart, D-220) */
        }
        break;
    }
    case 5u:
        inst->psg_bal[sel] = (uint8_t)m;
        break;
    case 6u: {
        uint32_t ctl = inst->psg_ctl[sel];

        if ((ctl & 0x40u) != 0u) {
            inst->psg_dda[sel] = (uint8_t)m; /* DDA 1: the latch, the wave RAM untouched */
            effect |= (uint32_t)DS_PSGE_DDA_LOAD;
        } else if ((ctl & 0x80u) == 0u) {
            inst->psg_wave[sel * 32u + inst->psg_widx[sel]] = (uint8_t)m; /* chON 0, DDA 0: store and advance */
            inst->psg_widx[sel] = (uint8_t)(((uint32_t)inst->psg_widx[sel] + 1u) & 31u);
            effect |= (uint32_t)DS_PSGE_WAVE_STORE;
        } else if (DS_CONST_psg_wave_write_when_on != 0) {
            inst->psg_wave[sel * 32u + inst->psg_widx[sel]] = (uint8_t)m; /* chON 1, DDA 0: store with no increment (psg_wave_write_when_on) */
            effect |= (uint32_t)DS_PSGE_WAVE_HELD;
        }
        break;
    }
    case 7u: /* select 4 or 5 only; the noise period applies from the next noise reload */
        inst->psg_noise[sel - 4u] = (uint8_t)m;
        break;
    case 8u:
        inst->psg_lfo_freq = (uint8_t)m;
        break;
    default: { /* R9 */
        uint32_t trg_was = (uint32_t)inst->psg_lfo_ctl & 0x80u;
        uint32_t was = chan_running(inst, 1u);

        inst->psg_lfo_ctl = (uint8_t)m;
        if ((m & 0x80u) != 0u) {
            inst->psg_widx[1] = 0u; /* TRG 1: select 1's wave index 0 and its counter frozen (D-192) */
            inst->psg_rem[1] = 0u;
            effect |= (uint32_t)DS_PSGE_LFO_RESET;
        } else if (trg_was != 0u && was == 0u && chan_running(inst, 1u) != 0u && DS_CONST_psg_trg_release_restart != 0) {
            chan_start(inst, 1u, stamp); /* TRG 1 -> 0 with select 1 otherwise running (psg_trg_release_restart, D-220) */
        }
        if ((m & 0x03u) != 0u) {
            effect |= (uint32_t)DS_PSGE_UNMODELLED; /* the modulation is plan 05.1-01's (Phase 5.1, D-234) */
        }
        break;
    }
    }
    info->chan = (uint8_t)chan;
    info->effect = (uint8_t)effect;
    info->wave_idx = chan <= 5u ? inst->psg_widx[chan] : (uint8_t)0xFFu;
    info->post = (uint16_t)post;
    return effect;
}
