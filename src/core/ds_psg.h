/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_psg.h - the private entry points of the PSG (plans 05-02, 05-03; D-186, D-202, D-203, D-222, PR-1). Private to the core and its tests like ds_core.h; no public header exists (D-05).
 * All PSG time advances through ONE function, ds_psg_advance (psg.c), called from exactly two places and from nowhere else: (1) at the head of every write to hardware window 2 ($1FE800-$1FEBFF), after the stamp and io_buffer and before the write is decoded,
 * with step_end 0 (bus.c); (2) at the end of every step, directly after the VDC fold, with the stamp start + total - 1 and step_end 1 (ds_cpu_step_obs, cpu.c). Never at a read (the PSG has no read function, H8-24) and never in the interrupt sample.
 * PR-1 (the D-222 carrier, no struct ds_bus member): inside a step machine_time is the step start, so the stored PSG distances stay relative to machine_time - 1 for the whole step; a step_end 0 call processes every event with time <= stamp without rebasing,
 * and the step_end 1 call folds to the step's last clock and rebases the distances to it. An out-of-step bus write (a test with no bus) folds at its stamp with no rebase. obs is the observer log or NULL, used for emission only and read for nothing else.
 * A distance r of PSGT names the next event at machine_time + r - 1 (D-186); r = 0 is a frozen channel counter. Between steps the D-203 ranges hold: a running channel 1..24,576 (1..6,291,456 for select 1 once the LFO couples it, 05-09), a noise counter
 * 1..11,904, and every next event on the tick grid of d (psg_tick_divisor, 3 or 6, read from the profile by ds_psg_divisor and stored nowhere, D-183, D-223).
 */
#ifndef DS_PSG_H
#define DS_PSG_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"

/* the largest distances at d = 6 (D-195): a channel step 6 x 4096, a coupled select-1 step 6 x 4096 x 256, a noise period 6 x 64 x 31 */
#define DS_PSG_CHAN_REM_MAX 24576u
#define DS_PSG_COUPLED_REM_MAX 6291456u
#define DS_PSG_NOISE_REM_MAX 11904u
#define DS_PSG_LFSR_MASK 0x3FFFFu

/* fold the PSG to the stamp (inclusive): step_end is 0 at a window-2 write and 1 at the step end, where the distances are rebased to the stamp (PR-1) */
void ds_psg_advance(struct ds_instance *inst, uint64_t stamp, uint32_t step_end, struct ds_obs_log *obs);

/* the tick divisor d of the instance's profile binding: 3 when psg_tick_divisor is "3", else 6 (D-183, F-70) */
uint32_t ds_psg_divisor(const struct ds_instance *inst);

/* the storage mask of register reg (0..15) of the window (D-189): R0 $07, R1 $FF, R2 $FF, R3 $0F, R4 $DF, R5 $FF, R6 $1F, R7 $9F, R8 $FF, R9 $83, reg 10..15 0; one table shared with the restore decoder (snapshot.c) */
uint32_t ds_psg_reg_mask(uint32_t reg);

/* the power-on PSG state under every policy (D-193, D-204): registers, latches, indices and channel distances 0, both noise distances 1, both LFSRs psg_lfsr_reset; the wave RAM is left to the policy (zero, or drawn by seeded-prng-v4) */
void ds_psg_poweron(struct ds_instance *inst);

#endif
