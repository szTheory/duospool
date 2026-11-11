/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_serial.h - the canonical state digest and the profile identity hash (CORE-01, PROF-03; D-52, D-27). Internal to the core and its tests:
 * reached only through the include directory src/core.
 * The digest is SHA-256 over a framed stream, never over raw struct bytes (padding, pointers and layout would leak in): the prefix
 * "duospool-state" and a NUL byte, the serializer version as a u32, then the regions of the table in serial.c (version 7: PROF CONT POWR TIME RAM8 MPRS
 * CPU0 BUS0 TIMR IRQC IRQS BLKT IOPT SCHD), each framed as a four-byte ASCII tag, a u32 payload length and the payload. Every integer is written explicitly little-endian. The ROM bytes are not in the stream: the content
 * id (SHA-256 of the copy) stands for them. A matching digest is evidence of repeatability only, never of correctness.
 * Only stddef.h and stdint.h are reached from here, through ds_core.h (D-54).
 */
#ifndef DS_SERIAL_H
#define DS_SERIAL_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_DIGEST_BYTES 32u
/* the serializer version written into every stream: bumped by any change of the stream layout or of the set of regions */
#define DS_SERIAL_VERSION 7u

/* where the stream bytes go: one call per piece, in order; ctx is the caller's */
typedef void (*ds_sink_fn)(void *ctx, const uint8_t *data, size_t len);

/*
 * Write the canonical state stream of a booted instance to sink, byte for byte what ds_digest hashes (the test hook of D-52). The sink
 * may be called with any chunk size. A NULL instance or sink writes nothing.
 */
void ds_digest_stream(const struct ds_instance *inst, ds_sink_fn sink, void *ctx);

/* SHA-256 of that stream into out; returns DS_OK, or 1 and writes nothing for a NULL instance or output */
uint32_t ds_digest(const struct ds_instance *inst, uint8_t out[DS_DIGEST_BYTES]);

/*
 * Test accessor over the region table that the digest itself walks. For region index (0 up to the region count minus one): the
 * four-character tag (a static NUL-terminated string), a pointer into the LIVE storage of the instance whose first byte the payload of
 * the region carries, and the length of that live span. Returns 1 for a valid index and 0 otherwise (outputs untouched).
 */
uint32_t ds_region_info(struct ds_instance *inst, uint32_t index, const char **tag, uint8_t **ptr, size_t *len);

/*
 * The profile identity hash: SHA-256 over the stream "duospool-profile" NUL, the version u32 (1), the id as a string, the revision u32,
 * the field count u32, then for each field in table order its name (string), status (u32), value (string) and bound_by (u32), then the
 * number of unresolved behaviours u32 and their ids in sorted order (strings). A string is its length as a u32 and then its bytes (no
 * NUL). Label text is never part of it (D-27): identity is (id, rev) plus the canonical resolved text.
 */
void ds_profile_hash(const struct ds_profile *profile, uint8_t out[DS_DIGEST_BYTES]);

#endif
