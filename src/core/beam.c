/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * beam.c - the one fold of the VDC and VCE timeline (plan 04-02 skeleton, plan 04-04 grid; D-136, D-137, D-138, D-166, F-9, F-28, F-29). Machine time is u64 master clocks and the fold is a function of an access stamp, the interrupt sample stamp or the end of a step, never of a host clock.
 * The mutation rule of the file: raster, beam, status and DMA state change only inside ds_vdc_advance and the register write handlers of the plans that add them; nothing else writes them, and nothing here allocates, reads a file, a clock or the environment.
 * The grid (plan 04-04): line k starts at DS_CONST_raster_phase_mclk + k * DS_CONST_line_mclk and its EV_LINE is DS_CONST_line_sample_offset_mclk after that; the stored next event is derived from (line_idx, stage, vd_off, rcr_off), no clock value is stored (D-137). The fold processes every event with time <= stamp in time order, an event at the
 * stamp of an access BEFORE the access (the tie rule of D-138, ASSUMED LED-VID-0004), and never rewinds: a stamp below the next event does nothing. At EV_LINE the divider is latched from the control register, the record of the line is rendered from the state and offered to the sink when one is attached, the dirty flags are cleared with or without a sink (D-164),
 * and the line index advances. Plan 04-08 adds the vertical machine, the RCR counter and the two interrupt events of the ladder: at EV_LINE, in the order of D-138, the vertical cycle advances (D-139), the RCR counter steps (D-142), the divider is latched and the two event offsets are computed, and the VBlank state entry latches MWR and raises VD (D-143);
 * EV_VD and EV_RCR of the line follow at their offsets and raise VD and RR through ds_vdc_flag, whose enable gate is read at that instant (D-144). Plan 04-10 adds the DMA: the closed form of the VRAM-to-VRAM transfer and the DS time are applied in time order before every event and at the target stamp (item 1 of D-138), the burst
 * latch, the cancel at display start and the start of a pending transfer are at EV_LINE (dma_line), and the SATB copy is at the VBlank state entry. Plan 04-09 gives a display line its active window in the record from HSR, HDR and the divider latched here (vid.c, D-140). The positions are modelling assumptions (ASSUMED LED-
 * IRQ-0001, LED-IRQ-0010, LED-VID-0004), not hardware measurements.
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
/* the vertical states of D-139 (v_state) and the register slots the ladder reads (the slot is the register number, vdc.c) */
enum { VS_VDS = 0, VS_VDW = 1, VS_VCR = 2, VS_VSW = 3 };
enum { SLOT_CR = 5, SLOT_RCR = 6, SLOT_MWR = 9, SLOT_HSR = 10, SLOT_HDR = 11, SLOT_VPR = 12, SLOT_VDW = 13, SLOT_VCR = 14, SLOT_DCR = 15 };
/* CR bits 7 (BB, background) and 6 (SB, sprites): both clear makes a frame burst (D-157), and DCR bit 4 repeats the SATB copy at every VBlank entry (D-150) */
enum { CR_BB_SB = 0xC0, DCR_DSR = 0x10 };
/* the register slot of BYR: the background row counter of this file's ev_line steps it (D-156) */
enum { SLOT_BYR = 8 };
/* the status flag numbers of ds_vdc_flag (D-144): RR is bit 2 and VD bit 5 */
enum { FLAG_RR = 2, FLAG_VD = 5 };

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

/* the length in lines of vertical state st, read from the registers at the EV_LINE where the state is entered (D-139; MAN H7-10, H7-11, H7-13): VDS+2 (VPR bits 15-8), VDW+1 (9 bits), VCR (8 bits, 0 skips the state) and VSW+1 (VPR bits 4-0). At most 512, so v_togo never needs a clamp (F-1) */
static uint32_t state_len(const struct ds_instance *inst, uint32_t st)
{
    uint32_t vpr = (uint32_t)inst->vdc_reg[SLOT_VPR];

    switch (st) {
    case (uint32_t)VS_VDS:
        return ((vpr >> 8u) & 0xFFu) + 2u;
    case (uint32_t)VS_VDW:
        return ((uint32_t)inst->vdc_reg[SLOT_VDW] & 0x1FFu) + 1u;
    case (uint32_t)VS_VCR:
        return (uint32_t)inst->vdc_reg[SLOT_VCR] & 0xFFu;
    default:
        return (vpr & 0x1Fu) + 1u;
    }
}

/* enter vertical state st on this line (its first line): v_togo counts the lines of the state still to come after this one, and display_k counts the display lines from 0 inside VDW and is 0xFFFF in every other state */
static void enter_state(struct ds_instance *inst, uint32_t st, uint32_t len)
{
    inst->v_state = (uint8_t)st;
    inst->v_togo = (uint16_t)(len - 1u);
    inst->display_k = st == (uint32_t)VS_VDW ? (uint16_t)0u : (uint16_t)0xFFFFu;
}

/* item 2 of D-138, the vertical machine (D-139): the cycle restarts in VDS at frame line 0 (an unfinished state is cut, and no VD is raised for a VDW the wrap cut, LED-IRQ-0001) and otherwise runs VDS, VDW, VCR and VSW round again. A state whose length is 0 (only VCR can be) is skipped. Returns 1 when this line is the VBlank state entry, the first line after VDW.
 * When VDW completes on the last line of the frame (VDW with no line to come, so (VDS+2) + (VDW+1) lines fill the frame exactly), the first line after it is frame line 0: that wrap line is the VBlank state entry (review fix M-02, ASSUMED LED-IRQ-0001 and LED-VID-0005), so VD, the SATB copy and the MWR latch happen at the wrap instant and then
 * the vertical cycle restarts in VDS on the same line. A VDW that still has lines to come at the wrap is cut, with no VD */
static int vertical_step(struct ds_instance *inst, uint64_t k)
{
    uint32_t prev = (uint32_t)inst->v_state;
    uint32_t st = prev;
    uint32_t len = 0u;
    uint32_t tries;

    if (inst->frame_lines != 0u && k % (uint64_t)inst->frame_lines == UINT64_C(0)) {
        int vdw_done = prev == (uint32_t)VS_VDW && inst->v_togo == 0u && k != UINT64_C(0);

        enter_state(inst, (uint32_t)VS_VDS, state_len(inst, (uint32_t)VS_VDS));
        return vdw_done;
    }
    if (inst->v_togo != 0u) { /* a later line of the same state */
        inst->v_togo = (uint16_t)((uint32_t)inst->v_togo - 1u);
        if (prev == (uint32_t)VS_VDW) {
            inst->display_k = (uint16_t)(((uint32_t)inst->display_k + 1u) & 0xFFFFu);
        }
        return 0;
    }
    for (tries = 0u; tries < 4u && len == 0u; tries++) { /* the next state with at least one line; only VCR can have none, so the loop ends within two steps */
        st = (st + 1u) & 3u;
        len = state_len(inst, st);
    }
    if (len == 0u) { /* unreachable: keeps the loop bound a visible fact and not a proof about three registers */
        len = 1u;
    }
    enter_state(inst, st, len);
    return prev == (uint32_t)VS_VDW ? 1 : 0;
}

/* item 3 of D-138, the RCR counter (D-142): add 1 saturating at $7FFF, then set it to 64 on the origin line chosen by the model parameter rcr_origin (0 first_active: the first display line, 1 line_before: the last line of VDS). The set-to-64 is ONE statement, the anchor of the toy vid-rcr-base */
static void rcr_step(struct ds_instance *inst)
{
    uint32_t origin;

    if (inst->rcr_counter < (uint16_t)0x7FFFu) {
        inst->rcr_counter = (uint16_t)((uint32_t)inst->rcr_counter + 1u);
    }
    if (inst->rcr_origin == 0u) {
        origin = (uint32_t)inst->v_state == (uint32_t)VS_VDW && inst->display_k == (uint16_t)0u ? 1u : 0u;
    } else {
        origin = (uint32_t)inst->v_state == (uint32_t)VS_VDS && inst->v_togo == 0u ? 1u : 0u;
    }
    if (origin != 0u) {
        inst->rcr_counter = (uint16_t)64u;
    }
}

/* the background row counter bg_y (D-156), stepped at the EV_LINE of every display line after the vertical machine: the first display line (VDW, display_k 0) takes BYR as it stands, every later display line the previous value plus one modulo 512; a line that is not a display line leaves it alone. A write of either BYR byte sets it to the written value at its stamp (vdc.c), so a write during line n shows BYR + 1 on line n + 1 (MAN H7-7, ASSUMED LED-VID-0009) */
static void bg_step(struct ds_instance *inst)
{
    if ((uint32_t)inst->v_state != (uint32_t)VS_VDW) {
        return;
    }
    if (inst->display_k == (uint16_t)0u) {
        inst->bg_y = (uint16_t)((uint32_t)inst->vdc_reg[SLOT_BYR] & 0x1FFu);
    } else {
        inst->bg_y = (uint16_t)(((uint32_t)inst->bg_y + 1u) & 0x1FFu);
    }
}

/* item 4 of D-138 for the two offsets (D-140, D-142, D-143, T-04-28): the horizontal registers are read at this EV_LINE; the active window starts at (HDS+1)*8 dots and ends at (HDS+HDW+2)*8 dots, each times the divider latched for the line, in u32 (at most (127+127+2)*8*4 = 8192, no overflow).
 * EV_RCR is at the window end and does not occur on a line where that is 1365 or more; EV_VD (phase hdw_start) is at the window start, at most 1364. When EV_RCR exists the window start is before it, so the stage order VD then RCR is also the time order */
static void schedule_offsets(struct ds_instance *inst)
{
    uint32_t hds = ((uint32_t)inst->vdc_reg[SLOT_HSR] >> 8u) & 0x7Fu;
    uint32_t hdw = (uint32_t)inst->vdc_reg[SLOT_HDR] & 0x7Fu;
    uint32_t div = (uint32_t)inst->lat_div;
    uint32_t rcr_off = (hds + hdw + 2u) * 8u * div;
    uint32_t vd_off = (hds + 1u) * 8u * div;

    inst->rcr_off = rcr_off >= (uint32_t)DS_CONST_line_mclk ? (uint16_t)EV_OFF_NONE : (uint16_t)rcr_off;
    if (inst->vd_pending != 0u) {
        inst->vd_off = (uint16_t)(vd_off < (uint32_t)DS_CONST_line_mclk - 1u ? vd_off : (uint32_t)DS_CONST_line_mclk - 1u);
    }
}

/* item 4 of D-138 for the VRAM-to-VRAM transfer (D-151, D-157), at the EV_LINE of time t after the vertical step and the divider latch: at the first display line (VDW, display_k 0) the burst latch is evaluated as !(BB || SB) from CR as it stands now and persists for the frame, and a frame that is not burst cancels a pending or running
 * transfer with no DV, the counters as reached (the words due at this instant were applied before the event); then a pending transfer starts here when the bus is free (not VDW, or burst) and the divider is latched: its first word is due one word time after t, and the word time is latched from this divider. A transfer started
 * during a display period of a frame that is not burst stays pending until a later line finds the bus free */
static void dma_line(struct ds_instance *inst, uint64_t t)
{
    if ((uint32_t)inst->v_state == (uint32_t)VS_VDW && inst->display_k == (uint16_t)0u) {
        inst->burst = ((uint32_t)inst->vdc_reg[SLOT_CR] & (uint32_t)CR_BB_SB) == 0u ? (uint8_t)1u : (uint8_t)0u;
        if (inst->burst == 0u && inst->dma_state != 0u) {
            inst->dma_state = 0u;
            inst->dma_done = 0u;
        }
    }
    if (inst->dma_state == 1u && inst->lat_div != 0u && ((uint32_t)inst->v_state != (uint32_t)VS_VDW || inst->burst != 0u)) {
        inst->dma_state = 2u;
        inst->dma_start = t;
        inst->dma_done = 0u;
        inst->dma_word_mclk = (uint16_t)((uint32_t)DS_CONST_vdc_dma_dots_per_word * (uint32_t)inst->lat_div);
    }
}

/* EV_LINE of line line_idx, the items of D-138 this plan has, in order: (2) the vertical machine, (3) the RCR counter, (4) the divider latch and the event offsets, (5) at the VBlank state entry the MWR latch and VD, (7) render and offer the record when a sink is attached (the want_pixels gate of D-164), (8) clear the dirty flags; then the line advances.
 * Item 1 (DMA words due) is applied by the fold before this function runs, item 4 for the transfer is dma_line, and the other latches belong to later plans. VD of phase line_start is raised here, at this EV_LINE; phase hdw_start leaves it outstanding (vd_pending) for the EV_VD of this line. The divider is latched before the offsets are computed from it */
static void ev_line(struct ds_instance *inst, struct ds_obs_log *obs)
{
    uint64_t k = inst->line_idx;
    int vblank = vertical_step(inst, k);

    rcr_step(inst);
    bg_step(inst);
    inst->lat_div = (uint8_t)ds_vce_divider((uint32_t)inst->vce_cr);
    dma_line(inst, line_origin(k) + (uint64_t)DS_CONST_line_sample_offset_mclk);
    inst->vd_off = (uint16_t)EV_OFF_NONE;
    inst->vd_pending = 0u;
    if (vblank != 0) {
        inst->mwr_active = (uint16_t)((uint32_t)inst->vdc_reg[SLOT_MWR] & 0xFFu);
        if (inst->satb_pending != 0u || ((uint32_t)inst->vdc_reg[SLOT_DCR] & (uint32_t)DCR_DSR) != 0u) { /* the SATB copy instant, whatever CR.VC says (D-150); no arbitration with a VRAM transfer (VL1) */
            ds_vdc_satb_copy(inst, line_origin(k) + (uint64_t)DS_CONST_line_sample_offset_mclk);
        }
        if (inst->vd_irq_phase == 0u) {
            ds_vdc_flag(inst, (uint32_t)FLAG_VD);
        } else {
            inst->vd_pending = 1u;
        }
    }
    schedule_offsets(inst);
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
        ds_vdc_dma_apply(inst, t); /* the words due and the DS time that come up to this event are applied before it, in time order (item 1 of D-138) */
        ds_vdc_satb_due(inst, t);
        switch ((uint32_t)inst->stage) {
        case (uint32_t)STAGE_VD: /* the VBlank flag of phase hdw_start, gated by CR bit 3 at this instant (D-143, D-144) */
            inst->vd_pending = 0u;
            inst->vd_off = (uint16_t)EV_OFF_NONE;
            ds_vdc_flag(inst, (uint32_t)FLAG_VD);
            inst->stage = next_stage(inst, (uint32_t)STAGE_VD);
            break;
        case (uint32_t)STAGE_RCR: /* the raster flag of EVERY line, display or not: the counter equals RCR (10 bits) and CR bit 2 is set at this instant (D-142, D-144) */
            inst->rcr_off = (uint16_t)EV_OFF_NONE;
            if ((uint32_t)inst->rcr_counter == ((uint32_t)inst->vdc_reg[SLOT_RCR] & 0x3FFu)) {
                ds_vdc_flag(inst, (uint32_t)FLAG_RR);
            }
            inst->stage = next_stage(inst, (uint32_t)STAGE_RCR);
            break;
        default:
            ev_line(inst, obs);
            break;
        }
    }
    ds_vdc_dma_apply(inst, stamp); /* then the words and the DS time up to the target stamp, before the access that carries it (never a rewind: a stamp below the folded point finds nothing due) */
    ds_vdc_satb_due(inst, stamp);
}

/* power-on state of the VCE, the VDC and the beam (plan 04-03; D-165, D-167, F-11, F-48): ds_boot has zeroed the instance, so every register, latch, status, DMA and beam byte is already 0 under every policy; this sets the three non-zero bytes. The RCR counter is a constant (it is not a draw), the frame length is the bound
 * lines_per_frame of the profile (an open field every run binds: ds_resolve refuses an unbound one, so the value is 262 or 263 and 262 is the only other case) and display_k is 0xFFFF outside the display state (v_state 0, VDS, at boot). The VRAM, SATB and palette bytes are the business of the power-on policy (poweron.c). */
uint32_t ds_vdc_frame_lines(const struct ds_instance *inst)
{
    return ds_streq(inst->profile.field[DS_FIELD_lines_per_frame].value, "263") ? 263u : 262u;
}

void ds_vdc_poweron(struct ds_instance *inst)
{
    inst->rcr_counter = (uint16_t)DS_CONST_rcr_counter_poweron;
    inst->frame_lines = (uint16_t)ds_vdc_frame_lines(inst);
    inst->display_k = (uint16_t)0xFFFFu;
}
