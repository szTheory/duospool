/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * beam.c - the one fold of the VDC and VCE timeline (plan 04-02 skeleton; D-137, F-9, F-28, F-29). Machine time is u64 master clocks and the fold is a function of an access stamp, the interrupt sample stamp or the end of a step, never of a host clock.
 * The mutation rule of the file: raster, beam, status and DMA state change only inside ds_vdc_advance and the register write handlers of the plans that add them; nothing else writes them, and nothing here allocates, reads a file, a clock or the environment.
 * This plan places the three fold calls (bus.c, cpu.c) and proves them inert: no VDC state exists yet, so the body below consumes its arguments and does nothing, and every digest and golden of the earlier plans is unchanged. Plan 04-03 gives it the grid
 * and the frame wrap, 04-06 the line ladder.
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
