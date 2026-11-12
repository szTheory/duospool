/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_obs.h - the observer sinks of the core (plans 03-10 and 03-19; D-119, D-131, WR-02 extended). Private to the core and its tests, like ds_core.h; no public header exists (D-05).
 * A sink is caller memory: the instance holds machine state only and nothing here is part of it. Three sinks exist, all output only; insn and dev share one header and one overflow policy, vid (plan 04-02) has its own, below.
 * The instruction sink (insn) receives one fixed-size record per executed instruction:
 *   record (DS_INSN_RECORD_BYTES = 24, little-endian, written byte by byte from widened unsigned values, the state BEFORE the instruction runs):
 *     +00 u64 start stamp (machine time in master clocks)  +08 u16 pc  +10 u8 opcode  +11 u8 A  +12 u8 X  +13 u8 Y  +14 u8 S  +15 u8 P  +16 u8 speed  +17..+23 zero
 * The device-register sink (dev, plan 03-19) receives one record per logged access to the hardware page: every write to windows 0 to 5 ($0000-$17FF of the page, the ST0 to ST2 physical writes included) and every read and write of
 * window 4 (the $1000 window); reads of the other windows are not logged:
 *   record (DS_DEV_RECORD_BYTES = 24, little-endian): +00 u64 stamp (the stamp of the access in master clocks, since plan 03-21 the access stamp of the bus: the instruction's start stamp plus the cycle of the access inside it times the cycle length, D-92 and D-108; seq orders accesses with one stamp)
 *     +08 u32 phys (physical address, $1FE000 | offset in the page)  +12 u8 value (the byte written, or the byte the read returned)  +13 u8 kind (DS_DEV_WRITE, DS_DEV_READ or DS_DEV_ST)
 *     +14 u8 flags (DS_DEV_FLAG_PENALTY: the access paid the VDC and VCE penalty, DS_DEV_FLAG_PHYS: a physical access that did not consult the mapping registers)  +15 u8 aux (the bus latch io_buffer after the access, D-104)
 *     +16 u16 pc (WR-02: the opcode address of the instruction, and of every step of a block transfer, so a record joins the transfer's single insn record by pc; an interrupt entry's accesses carry the next address the entry pushes)  +18..+23 zero
 * header (DS_INSN_HEADER_BYTES = DS_DEV_HEADER_BYTES = 16, little-endian, in the sink struct): +00 u32 count (records stored)  +04 u32 lost (records dropped, saturating)  +08 u64 seq (records offered in this call = count + lost
 * unless lost saturated). The core overwrites the header at the start of every accepted ds_run call and again at its end, and never reads it back, so a garbage-poisoned header cannot steer a write. The only overflow policy is
 * drop-newest: when count reaches the capacity of the request, later records are dropped and lost counts them. There is no SINK_FULL stop reason (D-95, D-119).
 * The video sink (vid, plan 04-02; D-161, D-162) is a byte buffer for whole raw-video records of 64 to DS_VID_MAX_REC_BYTES bytes (the DSVR v1 layout is written by a later plan; the channel here only stores them). Its capacity is in bytes, at most
 * DS_VID_SINK_MAX_BYTES (a larger one is run.sink_unsupported), and its buffer is DS_VID_ALIGN-aligned. header (DS_VID_HEADER_BYTES = 32, little-endian, in the sink struct): +00 u32 count (whole records stored)  +04 u32 lost (records not stored, saturating)
 * +08 u64 seq (records offered in this call)  +16 u64 bytes (bytes stored: the sum of the stored records)  +24 u64 zero. Written at the start and at the end of every accepted call and never read. The policy is drop-newest in whole records with a sticky full
 * flag (F-2): the first record that does not fit sets full, and that record and every later one of the call count as lost, even a later one that is smaller; lost = seq - count and no record is ever cut or gapped. A record pointer is never used for a zero-length copy.
 * The count of the running call lives in a local of ds_run (struct ds_obs_log), never in the instance and never in caller memory. A sink whose memory overlaps the instance, the request, the result, itself or another sink, a misaligned
 * sink or record pointer, a NULL record pointer with a nonzero capacity, or a record range that wraps the address space is refused as run.memory_invalid and nothing is written (the instance is byte-identical). A capacity above
 * DS_SINK_MAX_RECORDS (insn, dev) or DS_VID_SINK_MAX_BYTES (vid) is refused as run.sink_unsupported. The order of the checks is: the sink structs (memory_invalid), then the capacities (sink_unsupported), then the record ranges (memory_invalid). Hooks are void and take a const
 * instance: they cannot change machine state. The type name ds_sink_fn belongs to the digest stream (ds_serial.h).
 */
#ifndef DS_OBS_H
#define DS_OBS_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_INSN_RECORD_BYTES 24u
#define DS_INSN_HEADER_BYTES 16u
#define DS_INSN_ALIGN 8u /* the record memory is 8-byte aligned so a host can map it as 64-bit words */
#define DS_DEV_RECORD_BYTES 24u
#define DS_DEV_HEADER_BYTES 16u
#define DS_DEV_ALIGN 8u
#define DS_SINK_MAX_RECORDS 1048576u /* the largest capacity of either sink (24 MiB of records): a larger one is run.sink_unsupported, so the byte size of a sink always fits the address space */
_Static_assert((uint64_t)DS_SINK_MAX_RECORDS * (uint64_t)DS_INSN_RECORD_BYTES <= (uint64_t)SIZE_MAX, "the largest sink must fit the address space");
_Static_assert((uint64_t)DS_SINK_MAX_RECORDS * (uint64_t)DS_DEV_RECORD_BYTES <= (uint64_t)SIZE_MAX, "the largest sink must fit the address space");

/* the video sink (plan 04-02; D-161, D-162): the header and the byte capacity are in the struct, the record limits are fixed by the DSVR v1 layout (rec_bytes = 64 + ((4 * active_w + 7) & ~7), active_w at most DS_VID_MAX_DOTS) */
#define DS_VID_HEADER_BYTES 32u
#define DS_VID_ALIGN 8u
#define DS_VID_SINK_MAX_BYTES 67108864u /* 64 MiB: a larger capacity is run.sink_unsupported; it fits the address space on every supported host */
#define DS_VID_MAX_DOTS 682u            /* floor(1365 / 2): the dots of the longest line at the shortest dot clock; the _Static_assert against DS_CONST_line_mclk lands with that constant (plan 04-03) */
#define DS_VID_REC_HEADER_BYTES 64u
#define DS_VID_MAX_REC_BYTES (DS_VID_REC_HEADER_BYTES + ((4u * DS_VID_MAX_DOTS + 7u) & ~7u))
#define DS_VIDREC_VERSION 1u
_Static_assert(DS_VID_MAX_REC_BYTES == 2792u, "the longest DSVR v1 record is 64 + 2728 = 2792 bytes (D-161)");
_Static_assert((uint64_t)DS_VID_SINK_MAX_BYTES <= (uint64_t)SIZE_MAX, "the largest sink must fit the address space");

/* the kind and flag bytes of a dev record */
enum { DS_DEV_WRITE = 1, DS_DEV_READ = 2, DS_DEV_ST = 3 };
enum { DS_DEV_FLAG_PENALTY = 1, DS_DEV_FLAG_PHYS = 2 };

struct ds_insn_sink {
    uint8_t header[DS_INSN_HEADER_BYTES]; /* output only: written by ds_run, never read */
    uint8_t *records;                     /* capacity * DS_INSN_RECORD_BYTES bytes of caller memory, DS_INSN_ALIGN-aligned when capacity is nonzero */
    uint32_t capacity;                    /* records, from the request: nothing in caller memory but this field steers how much is written */
};

struct ds_dev_sink {
    uint8_t header[DS_DEV_HEADER_BYTES]; /* output only: written by ds_run, never read */
    uint8_t *records;                    /* capacity * DS_DEV_RECORD_BYTES bytes of caller memory, DS_DEV_ALIGN-aligned when capacity is nonzero */
    uint32_t capacity;                   /* records, from the request */
};

struct ds_vid_sink {
    uint8_t header[DS_VID_HEADER_BYTES]; /* output only: written by ds_run, never read */
    uint8_t *bytes;                      /* capacity bytes of caller memory, DS_VID_ALIGN-aligned when capacity is nonzero */
    uint32_t capacity;                   /* BYTES, from the request: nothing in caller memory but this field steers how much is written */
};

/* the running state of one sink in one ds_run call: a local of ds_run, written back to the header at the end; on is 0 for a sink the request did not carry */
struct ds_obs_chan {
    int on;
    uint8_t *header;
    uint8_t *records;
    uint32_t capacity;
    uint32_t count;
    uint32_t lost;
    uint64_t seq;
};

/* the running state of the vid sink in one ds_run call (whole records in a byte buffer): on is 0 for a sink the request did not carry; full is the sticky flag of the drop-newest rule */
struct ds_obs_vchan {
    int on;
    int full;
    uint8_t *header;
    uint8_t *bytes;
    uint32_t capacity;
    uint32_t count;
    uint32_t lost;
    uint64_t seq;
    uint64_t stored;
};

struct ds_obs_log {
    struct ds_obs_chan insn;
    struct ds_obs_chan dev;
    struct ds_obs_vchan vid;
};

/* DS_RUNREF_NONE when the sinks of the request may be used by this call, else the refusal (memory_invalid or sink_unsupported) in the order above; a NULL sink is not carried and is always acceptable */
uint32_t ds_obs_sinks_check(const struct ds_run_request *request, const struct ds_instance *inst, const struct ds_run_result *result);
void ds_obs_begin(struct ds_obs_log *log, struct ds_insn_sink *insn, struct ds_dev_sink *dev, struct ds_vid_sink *vid); /* zero the running counts and overwrite the headers of the sinks that are carried */
void ds_obs_insn(struct ds_obs_log *log, const struct ds_instance *inst, uint32_t opcode);     /* one record, or lost++; log NULL or no insn sink does nothing */
/* one dev record for an access at the stamp the bus computed for it, carrying the pc the bus was given (struct ds_bus pc: never read from the instance); the bus calls it after the access, so aux is the latch it left; log NULL or no dev sink does nothing */
void ds_obs_dev(struct ds_obs_log *log, const struct ds_instance *inst, uint64_t stamp, uint32_t pc, uint32_t phys, uint32_t value, uint32_t kind, uint32_t flags);
/* offer one whole video record of rec_bytes bytes at rec (plan 04-02; D-162, F-2): it is stored, or counted lost; the offer is counted in seq either way. A record that does not fit (or any record once one did not fit: the sticky full flag) is lost; a NULL or empty record is
 * malformed, is counted lost and never copied, and does not make the sink full. log NULL or no vid sink does nothing. */
void ds_obs_vid(struct ds_obs_log *log, const uint8_t *rec, uint32_t rec_bytes);
void ds_obs_end(struct ds_obs_log *log); /* overwrite the headers with the final counts; log NULL does nothing */

/* one instruction with the observer hook (cpu.c): ds_cpu_step is ds_cpu_step_obs with no log */
uint32_t ds_cpu_step_obs(struct ds_instance *inst, struct ds_obs_log *log);

#endif
