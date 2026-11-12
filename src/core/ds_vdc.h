/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_vdc.h - the private entry points of the VCE and the VDC (plan 04-02; D-137, F-9, F-21, F-24, F-28, F-29). Private to the core and its tests like ds_core.h; no public header exists (D-05).
 * All VDC and VCE time advances through ONE function, ds_vdc_advance (beam.c): monotone, composable (advance(a) then advance(b) equals advance(b) when no access lies between) and inclusive (every event with time <= stamp is processed, in time order,
 * before the access that carries the stamp). A stamp at or below the folded point does nothing and never rewinds (F-29); a NULL obs folds without a log, and a bus-less access (stamp = machine time) is the same call (F-29).
 * It is called from three places and from nowhere else: (1) at the head of every read and write of hardware windows 0 and 1 and every read of $1403, with the stamp the bus already computed after charging the penalty (bus.c, F-24); (2) in the interrupt sample, before the
 * pending query, at the existing sample stamp (cpu.c); (3) at the end of every step with the stamp machine_time_after - 1, inside ds_cpu_step_obs (cpu.c, F-9, F-28). Every fold target is therefore a function of the instruction stream alone, so chunking, restore at a stop
 * and an attached sink cannot change what is folded.
 * VCE and VDC state changes ONLY inside the fold and the register write handlers; every other function of the machine reads state. The pure queries (ds_irq_pending_query among them) are valid for a FOLDED instance: they read the stored fields and not the stamp (F-21).
 * Plan 04-02 declares the entry points and defines only the inert ds_vdc_advance (no VDC state exists yet); the others are defined by the plans that own their state (VCE 04-03, VDC 04-04 on) and their signatures may be refined there.
 */
#ifndef DS_VDC_H
#define DS_VDC_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"

/* fold the VDC and VCE to the stamp (inclusive): process every event with time <= stamp in time order; obs is the observer log the line records are emitted to, or NULL (D-137, D-164: it is passed to the emitter and read for nothing else) */
void ds_vdc_advance(struct ds_instance *inst, uint64_t stamp, struct ds_obs_log *obs);

/* raise one VDC status flag (the fold and the write handlers only) */
void ds_vdc_flag(struct ds_instance *inst, uint32_t bit);

/* a register read or write of the VDC window (reg = A1-A0) and of the VCE window (reg = A2-A0) at the access stamp, after the fold; a read of a window register may change the status flags, a write changes registers */
uint8_t ds_vdc_read(struct ds_instance *inst, uint32_t reg, uint64_t stamp);
void ds_vdc_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp);
uint8_t ds_vce_read(struct ds_instance *inst, uint32_t reg, uint64_t stamp);
void ds_vce_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp);

/* the power-on state of the VDC and the VCE (D-167) */
void ds_vdc_poweron(struct ds_instance *inst);

#endif
