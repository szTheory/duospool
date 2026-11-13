/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * psg.c - the PSG (plan 05-02; D-186, PR-1). The fold and its two call sites are described in ds_psg.h. Plan 05-02 defines the fold as inert: no PSG state exists yet, so nothing is folded and every state digest, golden and record is unchanged.
 * Plan 05-03 fills it with the register storage and the counters, plan 05-04 with the synthesis and the record and frame emission. No allocation, no global, no clock.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_psg.h"

void ds_psg_advance(struct ds_instance *inst, uint64_t stamp, uint32_t step_end, struct ds_obs_log *obs)
{
    /* inert until plan 05-03 fills it */
    (void)inst;
    (void)stamp;
    (void)step_end;
    (void)obs;
}
