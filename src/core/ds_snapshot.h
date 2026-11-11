/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_snapshot.h - the snapshot and restore of a machine (CORE-05; plan 03-30; D-121, D-122, Pitfall 20). Internal to the core and its tests: reached only through the include directory src/core.
 * The snapshot IS the canonical digest stream (ds_serial.h): the same prefix, version and regions in the same order, written by the same emit path (ds_digest_stream), so there is no second layout, the size is fixed (ds_snapshot_size) and the SHA-256 of a snapshot is the digest of the
 * instance. The ROM is not in it (the content id stands for it), nor are sinks or host settings. The stream is machine state only; the three boot-derived members mapper, byte_order and header, which no region carries, are not restored.
 * ds_restore puts a snapshot back onto an instance that was booted from the same profile and the same content (the profile hash and the content id of the stream must be the instance's, or it is refused). The stream is untrusted input: pass 1 walks it in place and checks the length, the prefix, the version,
 * every tag and length and the domain of every field, and writes nothing; pass 2 walks the same bytes again and copies, and cannot fail. A refusal leaves the instance byte-identical (D-121, Pitfall 20). No allocation, no copy of the instance, no pointer into the buffer is kept.
 * A restore needs no earlier state of the target beyond its profile and content: every restorable member is overwritten, so a target booted with another seed, or poisoned, ends byte-identical to the source except for the ROM. A restored machine continues as the original (restore evidence is E0: repeatability, no correctness claim).
 */
#ifndef DS_SNAPSHOT_H
#define DS_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

enum ds_snapshot_refusal {
    DS_SNAPREF_NONE = 0,
#define DS_SNAPREF(name, number) DS_SNAPREF_##name = number,
#include "snapshot_refusals.def"
#undef DS_SNAPREF
    DS_SNAPREF_SLOTS = 16 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "snapshot.NAME" for a refusal number, "ok" for DS_SNAPREF_NONE, NULL for a number that is not a member */
const char *ds_snapshot_refusal_name(uint32_t number);

/* the exact size in bytes of every snapshot: the prefix, the version and the frame (tag, length) and payload of every region, a pure sum of the fixed lengths of the region table (25021 bytes at serial version 7) */
size_t ds_snapshot_size(void);

/* write the snapshot of a booted instance into buf (cap bytes) and the number of bytes written to *written (when not NULL): DS_SNAPREF_NONE, or a refusal (argument, overlap, capacity) with nothing written */
uint32_t ds_snapshot(const struct ds_instance *inst, uint8_t *buf, size_t cap, size_t *written);

/* restore a snapshot of len bytes onto a booted instance of the same profile and content: DS_SNAPREF_NONE, or the first refusal with the instance byte-identical */
uint32_t ds_restore(struct ds_instance *inst, const uint8_t *buf, size_t len);

#endif
