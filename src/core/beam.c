/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * beam.c - the one fold of the VDC and VCE timeline (plan 04-02 skeleton; D-137, F-9, F-28, F-29). Machine time is u64 master clocks and the fold is a function of an access stamp, the interrupt sample stamp or the end of a step, never of a host clock.
 * The mutation rule of the file: raster, beam, status and DMA state change only inside ds_vdc_advance and the register write handlers of the plans that add them; nothing else writes them, and nothing here allocates, reads a file, a clock or the environment.
 * This plan places the three fold calls (bus.c, cpu.c) and proves them inert: no VDC state exists yet, so the body below consumes its arguments and does nothing, and every digest and golden of the earlier plans is unchanged. Plan 04-03 gives it the grid
 * and the frame wrap, 04-06 the line ladder.
 * Plan 04-03 adds the power-on state (ds_vdc_poweron) and the state layout of the instance (ds_core.h); the fold stays inert.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_vdc.h"

void ds_vdc_advance(struct ds_instance *inst, uint64_t stamp, struct ds_obs_log *obs)
{
    (void)inst;
    (void)stamp;
    (void)obs;
}

/* power-on state of the VCE, the VDC and the beam (plan 04-03; D-165, D-167, F-11, F-48): ds_boot has zeroed the instance, so every register, latch, status, DMA and beam byte is already 0 under every policy; this sets the three non-zero bytes. The RCR counter is a constant (it is not a draw), the frame length is the bound
 * lines_per_frame of the profile (an open field every run binds: ds_resolve refuses an unbound one, so the value is 262 or 263 and 262 is the only other case) and display_k is 0xFFFF outside the display state (v_state 0, VDS, at boot). The VRAM, SATB and palette bytes are the business of the power-on policy (poweron.c). */
void ds_vdc_poweron(struct ds_instance *inst)
{
    inst->rcr_counter = (uint16_t)DS_CONST_rcr_counter_poweron;
    inst->frame_lines = ds_streq(inst->profile.field[DS_FIELD_lines_per_frame].value, "263") ? (uint16_t)263u : (uint16_t)262u;
    inst->display_k = (uint16_t)0xFFFFu;
}
