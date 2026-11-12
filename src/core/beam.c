/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * beam.c - the one fold of the VDC and VCE timeline (plan 04-02 skeleton, plan 04-04 grid; D-136, D-137, D-138, D-166, F-9, F-28, F-29). Machine time is u64 master clocks and the fold is a function of an access stamp, the interrupt sample stamp or the end of a step, never of a host clock.
 * The mutation rule of the file: raster, beam, status and DMA state change only inside ds_vdc_advance and the register write handlers of the plans that add them; nothing else writes them, and nothing here allocates, reads a file, a clock or the environment.
 * The grid (plan 04-04): line k starts at DS_CONST_raster_phase_mclk + k * DS_CONST_line_mclk and its EV_LINE is DS_CONST_line_sample_offset_mclk after that; the stored next event is derived from (line_idx, stage, vd_off, rcr_off), no clock value is stored (D-137). The fold processes every event with time <= stamp in time order, an event at the
 * stamp of an access BEFORE the access (the tie rule of D-138, ASSUMED LED-VID-0004), and never rewinds: a stamp below the next event does nothing. At EV_LINE the divider is latched from the control register, the record of the line is rendered from the state and offered to the sink when one is attached, the dirty flags are cleared with or without a sink (D-164),
 * and the line index advances. The vertical machine, the RCR counter, VD and the DMA closed forms are plans 04-08 and later: until then EV_LINE schedules no EV_VD and no EV_RCR (both offsets are none), the stages 1 and 2 only retire what a restored state names, and every record is the header alone.
 * Plan 04-03 added the power-on state (ds_vdc_poweron) and the state layout of the instance (ds_core.h).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_vdc.h"

/* the offset value that means "no such event this line" (D-138) */
#define EV_OFF_NONE 0xFFFFu
/* the stages: which event of the line is next (BEAM region, ds_core.h) */
enum { STAGE_LINE = 0, STAGE_VD = 1, STAGE_RCR = 2 };

/* the origin of line k on the grid; k is bounded by the caller (the fold from its own count, the restore rule from machine_time / line_mclk + 2), so the product stays inside 64 bits for every machine time a run can reach */
static uint64_t line_origin(uint64_t k)
{
    return (uint64_t)DS_CONST_raster_phase_mclk + k * (uint64_t)DS_CONST_line_mclk;
}

int ds_vdc_pending_time(uint64_t *time, uint64_t line_idx, uint32_t stage, uint32_t vd_pending, uint32_t vd_off, uint32_t rcr_off)
{
    uint64_t base;

    if (line_idx > (UINT64_MAX - (uint64_t)DS_CONST_raster_phase_mclk - (uint64_t)DS_CONST_line_mclk - (uint64_t)DS_CONST_line_sample_offset_mclk) / (uint64_t)DS_CONST_line_mclk) {
        return 0; /* the product would leave 64 bits: no such time */
    }
    if (stage == (uint32_t)STAGE_LINE) {
        *time = line_origin(line_idx) + (uint64_t)DS_CONST_line_sample_offset_mclk;
        return 1;
    }
    if (line_idx == UINT64_C(0) || (stage != (uint32_t)STAGE_VD && stage != (uint32_t)STAGE_RCR)) {
        return 0; /* the events after EV_LINE belong to the line whose EV_LINE advanced line_idx */
    }
    base = line_origin(line_idx - UINT64_C(1));
    if (stage == (uint32_t)STAGE_VD) {
        if (vd_pending == 0u || vd_off == (uint32_t)EV_OFF_NONE) {
            return 0;
        }
        *time = base + (uint64_t)vd_off;
        return 1;
    }
    if (rcr_off == (uint32_t)EV_OFF_NONE) {
        return 0;
    }
    *time = base + (uint64_t)rcr_off;
    return 1;
}

/* the stage that follows the one just retired: the EV_VD of the line when it is outstanding, else its EV_RCR when it has one, else the EV_LINE of the next line (ties in this order: EV_LINE, EV_VD, EV_RCR, D-138) */
static uint8_t next_stage(const struct ds_instance *inst, uint32_t after)
{
    if (after < (uint32_t)STAGE_VD && inst->vd_pending != 0u && (uint32_t)inst->vd_off != (uint32_t)EV_OFF_NONE) {
        return (uint8_t)STAGE_VD;
    }
    if (after < (uint32_t)STAGE_RCR && (uint32_t)inst->rcr_off != (uint32_t)EV_OFF_NONE) {
        return (uint8_t)STAGE_RCR;
    }
    return (uint8_t)STAGE_LINE;
}

/* EV_LINE of line line_idx (items 4, 7 and 8 of D-138 as far as this plan has them): latch the divider, schedule nothing else, render and offer the record when a sink is attached (the want_pixels gate of D-164), clear the dirty flags, advance the line */
static void ev_line(struct ds_instance *inst, struct ds_obs_log *obs)
{
    uint64_t k = inst->line_idx;

    inst->lat_div = (uint8_t)ds_vce_divider((uint32_t)inst->vce_cr);
    inst->vd_off = (uint16_t)EV_OFF_NONE;
    inst->rcr_off = (uint16_t)EV_OFF_NONE;
    inst->vd_pending = 0u;
    if (obs != NULL && obs->vid.on) { /* the want_pixels gate: the record is built and offered only when a vid sink is attached; nothing the line does to the instance depends on it */
        uint8_t rec[DS_VID_MAX_REC_BYTES];
        uint32_t n = ds_vid_render(inst, line_origin(k), rec);

        ds_vid_emit(obs, rec, n);
    }
    inst->line_dirty = 0u;
    inst->line_idx = k + UINT64_C(1);
    inst->stage = next_stage(inst, (uint32_t)STAGE_LINE);
}

void ds_vdc_advance(struct ds_instance *inst, uint64_t stamp, struct ds_obs_log *obs)
{
    uint64_t span = stamp > inst->machine_time ? stamp - inst->machine_time : UINT64_C(0);
    /* the loop bound (D-137, D-166, T-04-14): the stored next event is never before machine_time (boot, the step-end fold and the restore window rule keep it so), so a fold to a stamp at most span clocks ahead meets at most three events per line that starts inside the span plus the tail of the line before it. A legal state never reaches the bound; it caps a hostile one */
    uint64_t bound = (span / (uint64_t)DS_CONST_line_mclk) * UINT64_C(3) + UINT64_C(6);
    uint64_t n;

    for (n = UINT64_C(0); n < bound; n++) {
        uint64_t t = UINT64_C(0);

        if (!ds_vdc_pending_time(&t, inst->line_idx, (uint32_t)inst->stage, (uint32_t)inst->vd_pending, (uint32_t)inst->vd_off, (uint32_t)inst->rcr_off) || t > stamp) {
            break;
        }
        switch ((uint32_t)inst->stage) {
        case (uint32_t)STAGE_VD: /* no VD flag exists yet (plan 04-08): the event is retired */
            inst->vd_pending = 0u;
            inst->vd_off = (uint16_t)EV_OFF_NONE;
            inst->stage = next_stage(inst, (uint32_t)STAGE_VD);
            break;
        case (uint32_t)STAGE_RCR: /* no RCR compare exists yet (plan 04-08): the event is retired */
            inst->rcr_off = (uint16_t)EV_OFF_NONE;
            inst->stage = next_stage(inst, (uint32_t)STAGE_RCR);
            break;
        default:
            ev_line(inst, obs);
            break;
        }
    }
}

/* power-on state of the VCE, the VDC and the beam (plan 04-03; D-165, D-167, F-11, F-48): ds_boot has zeroed the instance, so every register, latch, status, DMA and beam byte is already 0 under every policy; this sets the three non-zero bytes. The RCR counter is a constant (it is not a draw), the frame length is the bound
 * lines_per_frame of the profile (an open field every run binds: ds_resolve refuses an unbound one, so the value is 262 or 263 and 262 is the only other case) and display_k is 0xFFFF outside the display state (v_state 0, VDS, at boot). The VRAM, SATB and palette bytes are the business of the power-on policy (poweron.c). */
void ds_vdc_poweron(struct ds_instance *inst)
{
    inst->rcr_counter = (uint16_t)DS_CONST_rcr_counter_poweron;
    inst->frame_lines = ds_streq(inst->profile.field[DS_FIELD_lines_per_frame].value, "263") ? (uint16_t)263u : (uint16_t)262u;
    inst->display_k = (uint16_t)0xFFFFu;
}
