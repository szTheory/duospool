/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * obs.c - the instruction sink, device-register sink and video sink writers (plans 03-10, 03-19 and 04-02; D-119, D-131, D-162). The layouts, the header rules and the refusal cases are in ds_obs.h. Records and the header are written byte by byte,
 * little-endian, from widened unsigned values: no struct is written through a cast, so host byte order and alignment never reach the output. Nothing here reads caller memory except the
 * input fields of the sinks (the record pointer and the capacity); the header is overwritten and never read back. No allocation, no global, no clock.
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

/* what the check knows of one sink of the request: where its struct and its record memory are (rec_bytes is 0 until the record range has been computed) */
struct sink_ref {
    int present;
    uintptr_t obj;
    size_t obj_bytes;
    size_t obj_align;
    uint32_t capacity;
    uintptr_t rec;
    size_t rec_bytes;
    size_t rec_align;
    size_t rec_unit;
    uint32_t limit; /* the largest capacity of the sink (records for insn and dev, bytes for vid): a larger one is run.sink_unsupported */
    const uint8_t *records;
};

static void sink_ref_insn(struct sink_ref *r, const struct ds_insn_sink *s)
{
    r->present = s != NULL;
    r->obj = (uintptr_t)s;
    r->obj_bytes = sizeof(struct ds_insn_sink);
    r->obj_align = (size_t)_Alignof(struct ds_insn_sink);
    r->rec_align = (size_t)DS_INSN_ALIGN;
    r->rec_unit = (size_t)DS_INSN_RECORD_BYTES;
    r->limit = (uint32_t)DS_SINK_MAX_RECORDS;
    r->capacity = 0u;
    r->rec = 0u;
    r->rec_bytes = 0u;
    r->records = NULL;
}

static void sink_ref_dev(struct sink_ref *r, const struct ds_dev_sink *s)
{
    r->present = s != NULL;
    r->obj = (uintptr_t)s;
    r->obj_bytes = sizeof(struct ds_dev_sink);
    r->obj_align = (size_t)_Alignof(struct ds_dev_sink);
    r->rec_align = (size_t)DS_DEV_ALIGN;
    r->rec_unit = (size_t)DS_DEV_RECORD_BYTES;
    r->limit = (uint32_t)DS_SINK_MAX_RECORDS;
    r->capacity = 0u;
    r->rec = 0u;
    r->rec_bytes = 0u;
    r->records = NULL;
}

/* the video sink: its capacity is already a byte count, so the unit is one byte */
static void sink_ref_vid(struct sink_ref *r, const struct ds_vid_sink *s)
{
    r->present = s != NULL;
    r->obj = (uintptr_t)s;
    r->obj_bytes = sizeof(struct ds_vid_sink);
    r->obj_align = (size_t)_Alignof(struct ds_vid_sink);
    r->rec_align = (size_t)DS_VID_ALIGN;
    r->rec_unit = (size_t)1u;
    r->limit = (uint32_t)DS_VID_SINK_MAX_BYTES;
    r->capacity = 0u;
    r->rec = 0u;
    r->rec_bytes = 0u;
    r->records = NULL;
}

/* the record range of a sink whose struct has been accepted: 1 when it is usable (a capacity of zero uses no pointer) */
static int record_range_ok(struct sink_ref *r)
{
    if (r->capacity == 0u) {
        return 1;
    }
    if (r->records == NULL || ((uintptr_t)r->records % (uintptr_t)r->rec_align) != 0u) {
        return 0;
    }
    r->rec = (uintptr_t)r->records;
    r->rec_bytes = (size_t)r->capacity * r->rec_unit; /* the capacity is at most the limit of its sink, so this fits (the _Static_asserts of ds_obs.h) */
    return (uintptr_t)r->rec_bytes <= UINTPTR_MAX - r->rec; /* the range would wrap the address space */
}

#define DS_OBS_SINKS 3u /* the channels of the table: insn, dev, vid (Phase 5 adds aud) */

uint32_t ds_obs_sinks_check(const struct ds_run_request *request, const struct ds_instance *inst, const struct ds_run_result *result)
{
    struct sink_ref s[DS_OBS_SINKS];
    unsigned i;
    unsigned j;

    sink_ref_insn(&s[0], request->insn);
    sink_ref_dev(&s[1], request->dev);
    sink_ref_vid(&s[2], request->vid);
    /* 1: the sink structs (nothing of a sink is read before its struct is known to be aligned and clear of the call's memory and of every other struct) */
    for (i = 0u; i < DS_OBS_SINKS; i++) {
        if (s[i].present && ((s[i].obj % (uintptr_t)s[i].obj_align) != 0u || touches_call(s[i].obj, s[i].obj_bytes, inst, request, result))) {
            return (uint32_t)DS_RUNREF_memory_invalid;
        }
    }
    for (i = 0u; i < DS_OBS_SINKS; i++) {
        for (j = i + 1u; j < DS_OBS_SINKS; j++) {
            if (s[i].present && s[j].present && ranges_overlap(s[i].obj, s[i].obj_bytes, s[j].obj, s[j].obj_bytes)) {
                return (uint32_t)DS_RUNREF_memory_invalid;
            }
        }
    }
    /* 2: the capacities: the only fields read from the structs in this step */
    if (s[0].present) {
        s[0].capacity = request->insn->capacity;
        s[0].records = request->insn->records;
    }
    if (s[1].present) {
        s[1].capacity = request->dev->capacity;
        s[1].records = request->dev->records;
    }
    if (s[2].present) {
        s[2].capacity = request->vid->capacity;
        s[2].records = request->vid->bytes;
    }
    for (i = 0u; i < DS_OBS_SINKS; i++) {
        if (s[i].present && s[i].capacity > s[i].limit) {
            return (uint32_t)DS_RUNREF_sink_unsupported;
        }
    }
    /* 3: the record ranges: usable, clear of the call's memory, of the own struct and of the other sinks' structs and records */
    for (i = 0u; i < DS_OBS_SINKS; i++) {
        if (s[i].present && !record_range_ok(&s[i])) {
            return (uint32_t)DS_RUNREF_memory_invalid;
        }
    }
    for (i = 0u; i < DS_OBS_SINKS; i++) {
        if (!s[i].present) {
            continue;
        }
        if (touches_call(s[i].rec, s[i].rec_bytes, inst, request, result)) {
            return (uint32_t)DS_RUNREF_memory_invalid;
        }
        for (j = 0u; j < DS_OBS_SINKS; j++) {
            if (!s[j].present) {
                continue;
            }
            if (ranges_overlap(s[i].rec, s[i].rec_bytes, s[j].obj, s[j].obj_bytes) || (i != j && ranges_overlap(s[i].rec, s[i].rec_bytes, s[j].rec, s[j].rec_bytes))) {
                return (uint32_t)DS_RUNREF_memory_invalid;
            }
        }
    }
    return (uint32_t)DS_RUNREF_NONE;
}

static void write_header(const struct ds_obs_chan *ch)
{
    if (ch->on) {
        put_u32(ch->header, ch->count);
        put_u32(ch->header + 4, ch->lost);
        put_u64(ch->header + 8, ch->seq);
    }
}

/* the video header: +00 u32 count, +04 u32 lost, +08 u64 seq, +16 u64 bytes stored, +24 u64 zero; a channel the request did not carry writes nothing */
static void write_vheader(const struct ds_obs_vchan *ch)
{
    if (ch->on) {
        put_u32(ch->header, ch->count);
        put_u32(ch->header + 4, ch->lost);
        put_u64(ch->header + 8, ch->seq);
        put_u64(ch->header + 16, ch->stored);
        put_u64(ch->header + 24, UINT64_C(0));
    }
}

static void chan_open(struct ds_obs_chan *ch, uint8_t *header, uint8_t *records, uint32_t capacity, int on)
{
    ch->on = on;
    ch->header = header;
    ch->records = records;
    ch->capacity = capacity;
    ch->count = 0u;
    ch->lost = 0u;
    ch->seq = UINT64_C(0);
    write_header(ch);
}

/* offer one record: its bytes in the sink, or NULL when the sink is not carried or full (then lost counts it); the offer is counted in seq either way */
static uint8_t *chan_slot(struct ds_obs_chan *ch, size_t record_bytes)
{
    uint8_t *r;

    if (!ch->on) {
        return NULL;
    }
    ch->seq = ch->seq + UINT64_C(1);
    if (ch->count >= ch->capacity) {
        if (ch->lost < UINT32_MAX) {
            ch->lost = ch->lost + 1u;
        }
        return NULL;
    }
    r = ch->records + (size_t)ch->count * record_bytes;
    ch->count = ch->count + 1u;
    return r;
}

static void vchan_open(struct ds_obs_vchan *ch, struct ds_vid_sink *vid)
{
    ch->on = vid != NULL;
    ch->full = 0;
    ch->header = vid != NULL ? vid->header : NULL;
    ch->bytes = vid != NULL ? vid->bytes : NULL;
    ch->capacity = vid != NULL ? vid->capacity : 0u;
    ch->count = 0u;
    ch->lost = 0u;
    ch->seq = UINT64_C(0);
    ch->stored = UINT64_C(0);
    write_vheader(ch);
}

void ds_obs_begin(struct ds_obs_log *log, struct ds_insn_sink *insn, struct ds_dev_sink *dev, struct ds_vid_sink *vid)
{
    chan_open(&log->insn, insn != NULL ? insn->header : NULL, insn != NULL ? insn->records : NULL, insn != NULL ? insn->capacity : 0u, insn != NULL);
    chan_open(&log->dev, dev != NULL ? dev->header : NULL, dev != NULL ? dev->records : NULL, dev != NULL ? dev->capacity : 0u, dev != NULL);
    vchan_open(&log->vid, vid);
}

void ds_obs_end(struct ds_obs_log *log)
{
    if (log != NULL) {
        write_header(&log->insn);
        write_header(&log->dev);
        write_vheader(&log->vid);
    }
}

void ds_obs_insn(struct ds_obs_log *log, const struct ds_instance *inst, uint32_t opcode)
{
    uint8_t *r;
    uint32_t i;

    if (log == NULL) {
        return;
    }
    r = chan_slot(&log->insn, (size_t)DS_INSN_RECORD_BYTES);
    if (r == NULL) {
        return;
    }
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
}

void ds_obs_dev(struct ds_obs_log *log, const struct ds_instance *inst, uint64_t stamp, uint32_t pc, uint32_t phys, uint32_t value, uint32_t kind, uint32_t flags)
{
    uint8_t *r;
    uint32_t i;

    if (log == NULL) {
        return;
    }
    r = chan_slot(&log->dev, (size_t)DS_DEV_RECORD_BYTES);
    if (r == NULL) {
        return;
    }
    put_u64(r, stamp);
    put_u32(r + 8, phys);
    r[12] = (uint8_t)(value & 0xFFu);
    r[13] = (uint8_t)(kind & 0xFFu);
    r[14] = (uint8_t)(flags & 0xFFu);
    r[15] = inst->io_buffer;
    put_u16(r + 16, pc);
    for (i = 18u; i < DS_DEV_RECORD_BYTES; i++) {
        r[i] = 0u;
    }
}
