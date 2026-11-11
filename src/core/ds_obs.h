/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_obs.h - the observer sinks of the core (plan 03-10; D-119, D-131, WR-02 extended). Private to the core and its tests, like ds_core.h; no public header exists (D-05).
 * A sink is caller memory: the instance holds machine state only and nothing here is part of it. The instruction sink (insn) receives one fixed-size record per executed instruction:
 *   record (DS_INSN_RECORD_BYTES = 24, little-endian, written byte by byte from widened unsigned values, the state BEFORE the instruction runs):
 *     +00 u64 start stamp (machine time in master clocks)  +08 u16 pc  +10 u8 opcode  +11 u8 A  +12 u8 X  +13 u8 Y  +14 u8 S  +15 u8 P  +16 u8 speed  +17..+23 zero
 *   header (DS_INSN_HEADER_BYTES = 16, little-endian, in the sink struct): +00 u32 count (records stored)  +04 u32 lost (records dropped, saturating)  +08 u64 seq (instructions offered
 *   in this call = count + lost unless lost saturated). The core overwrites the header at the start of every accepted ds_run call and again at its end, and never reads it back, so a
 *   garbage-poisoned header cannot steer a write. The only overflow policy is drop-newest: when count reaches the capacity of the request, later records are dropped and lost counts them.
 *   The count of the running call lives in a local of ds_run (struct ds_obs_log), never in the instance and never in caller memory. A sink whose memory overlaps the instance, the request,
 *   the result or itself, a misaligned sink or record pointer, a NULL record pointer with a nonzero capacity, or a capacity whose byte size does not fit the address space is refused
 *   as run.memory_invalid and nothing is written (the instance is byte-identical). Hooks are void and take a const instance: they cannot change machine state.
 * The device-register sink (dev) is not built yet (plan 03-19): a non-NULL request->dev is run.sink_unsupported. The type name ds_sink_fn belongs to the digest stream (ds_serial.h).
 */
#ifndef DS_OBS_H
#define DS_OBS_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_INSN_RECORD_BYTES 24u
#define DS_INSN_HEADER_BYTES 16u
#define DS_INSN_ALIGN 8u /* the record memory is 8-byte aligned so a host can map it as 64-bit words */

struct ds_insn_sink {
    uint8_t header[DS_INSN_HEADER_BYTES]; /* output only: written by ds_run, never read */
    uint8_t *records;                     /* capacity * DS_INSN_RECORD_BYTES bytes of caller memory, DS_INSN_ALIGN-aligned when capacity is nonzero */
    uint32_t capacity;                    /* records, from the request: nothing in caller memory but this field steers how much is written */
};

/* the running state of one ds_run call with an insn sink: a local of ds_run, written back to the header at the end */
struct ds_obs_log {
    struct ds_insn_sink *sink;
    uint32_t count;
    uint32_t lost;
    uint64_t seq;
};

/* 1 when the sink may be used by this call: aligned, non-overlapping with the instance, the request, the result (when not NULL) and itself, a valid record range */
int ds_obs_insn_sink_valid(const struct ds_insn_sink *sink, const struct ds_instance *inst, const struct ds_run_request *request, const struct ds_run_result *result);
void ds_obs_begin(struct ds_obs_log *log, struct ds_insn_sink *sink); /* zero the running counts and overwrite the header with them */
void ds_obs_insn(struct ds_obs_log *log, const struct ds_instance *inst, uint32_t opcode); /* one record, or lost++; log NULL does nothing */
void ds_obs_end(struct ds_obs_log *log);                                                   /* overwrite the header with the final counts; log NULL does nothing */

/* one instruction with the observer hook (cpu.c): ds_cpu_step is ds_cpu_step_obs with no log */
uint32_t ds_cpu_step_obs(struct ds_instance *inst, struct ds_obs_log *log);

#endif
