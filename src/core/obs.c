/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * obs.c - the instruction sink writer (plan 03-10; D-119, D-131). The layout, the header rules and the refusal cases are in ds_obs.h. Records and the header are written byte by byte,
 * little-endian, from widened unsigned values: no struct is written through a cast, so host byte order and alignment never reach the output. Nothing here reads caller memory except the
 * two input fields of the sink (the record pointer and the capacity); the header is overwritten and never read back. No allocation, no global, no clock.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"

static void put_u16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8u) & 0xFFu);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    put_u16(p, v & 0xFFFFu);
    put_u16(p + 2, (v >> 16u) & 0xFFFFu);
}

static void put_u64(uint8_t *p, uint64_t v)
{
    put_u32(p, (uint32_t)(v & UINT64_C(0xFFFFFFFF)));
    put_u32(p + 4, (uint32_t)((v >> 32u) & UINT64_C(0xFFFFFFFF)));
}

/* the same uintptr_t range test as ranges_overlap in boot.c (a range of zero bytes overlaps nothing) */
static int ranges_overlap(uintptr_t a, size_t an, uintptr_t b, size_t bn)
{
    if (an == 0u || bn == 0u) {
        return 0;
    }
    if (a <= b) {
        return (b - a) < (uintptr_t)an;
    }
    return (a - b) < (uintptr_t)bn;
}

/* a block of caller memory (p, n bytes) that this call must not write into: the instance, the request, the result */
static int touches_call(uintptr_t p, size_t n, const struct ds_instance *inst, const struct ds_run_request *request, const struct ds_run_result *result)
{
    return ranges_overlap(p, n, (uintptr_t)inst, sizeof *inst) || ranges_overlap(p, n, (uintptr_t)request, sizeof *request) ||
           (result != NULL && ranges_overlap(p, n, (uintptr_t)result, sizeof *result));
}

int ds_obs_insn_sink_valid(const struct ds_insn_sink *sink, const struct ds_instance *inst, const struct ds_run_request *request, const struct ds_run_result *result)
{
    uintptr_t s = (uintptr_t)sink;
    uintptr_t r;
    size_t bytes;

    if (sink == NULL || (s % (uintptr_t)_Alignof(struct ds_insn_sink)) != 0u || touches_call(s, sizeof *sink, inst, request, result)) {
        return 0;
    }
    if (sink->capacity == 0u) {
        return 1; /* the record pointer is never used */
    }
    r = (uintptr_t)sink->records;
    if (sink->records == NULL || (r % (uintptr_t)DS_INSN_ALIGN) != 0u || (size_t)sink->capacity > SIZE_MAX / (size_t)DS_INSN_RECORD_BYTES) {
        return 0;
    }
    bytes = (size_t)sink->capacity * (size_t)DS_INSN_RECORD_BYTES;
    if ((uintptr_t)bytes > UINTPTR_MAX - r) {
        return 0; /* the range would wrap the address space */
    }
    return !touches_call(r, bytes, inst, request, result) && !ranges_overlap(r, bytes, s, sizeof *sink);
}

static void write_header(const struct ds_obs_log *log)
{
    put_u32(log->sink->header, log->count);
    put_u32(log->sink->header + 4, log->lost);
    put_u64(log->sink->header + 8, log->seq);
}

void ds_obs_begin(struct ds_obs_log *log, struct ds_insn_sink *sink)
{
    log->sink = sink;
    log->count = 0u;
    log->lost = 0u;
    log->seq = UINT64_C(0);
    write_header(log);
}

void ds_obs_end(struct ds_obs_log *log)
{
    if (log != NULL) {
        write_header(log);
    }
}

void ds_obs_insn(struct ds_obs_log *log, const struct ds_instance *inst, uint32_t opcode)
{
    uint8_t *r;
    uint32_t i;

    if (log == NULL) {
        return;
    }
    log->seq = log->seq + UINT64_C(1);
    if (log->count >= log->sink->capacity) {
        if (log->lost < UINT32_MAX) {
            log->lost = log->lost + 1u;
        }
        return;
    }
    r = log->sink->records + (size_t)log->count * (size_t)DS_INSN_RECORD_BYTES;
    put_u64(r, inst->machine_time);
    put_u16(r + 8, (uint32_t)inst->pc);
    r[10] = (uint8_t)(opcode & 0xFFu);
    r[11] = inst->a;
    r[12] = inst->x;
    r[13] = inst->y;
    r[14] = inst->s;
    r[15] = inst->p;
    r[16] = inst->speed;
    for (i = 17u; i < DS_INSN_RECORD_BYTES; i++) {
        r[i] = 0u;
    }
    log->count = log->count + 1u;
}
