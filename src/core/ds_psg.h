/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_psg.h - the private entry point of the PSG (plan 05-02; D-186, D-222, PR-1). Private to the core and its tests like ds_core.h; no public header exists (D-05).
 * All PSG time advances through ONE function, ds_psg_advance (psg.c), called from exactly two places and from nowhere else: (1) at the head of every write to hardware window 2 ($1FE800-$1FEBFF), after the stamp and io_buffer and before the write is decoded,
 * with step_end 0 (bus.c); (2) at the end of every step, directly after the VDC fold, with the stamp start + total - 1 and step_end 1 (ds_cpu_step_obs, cpu.c). Never at a read (the PSG has no read function, H8-24) and never in the interrupt sample.
 * PR-1 (the D-222 carrier, no struct ds_bus member): inside a step machine_time is the step start, so the stored PSG distances stay relative to machine_time - 1 for the whole step; a step_end 0 call processes every event with time <= stamp without rebasing,
 * and the step_end 1 call folds to the step's last clock and rebases the distances to it. An out-of-step bus write (a test with no bus) folds at its stamp with no rebase. obs is the observer log or NULL, used for emission only and read for nothing else.
 * Plan 05-02 defines the inert ds_psg_advance (no PSG state exists yet); plan 05-03 adds the state, the decode and the counters, plan 05-04 the synthesis and the emission.
 */
#ifndef DS_PSG_H
#define DS_PSG_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"

/* fold the PSG to the stamp (inclusive): step_end is 0 at a window-2 write and 1 at the step end, where the distances are rebased to the stamp (PR-1) */
void ds_psg_advance(struct ds_instance *inst, uint64_t stamp, uint32_t step_end, struct ds_obs_log *obs);

#endif
