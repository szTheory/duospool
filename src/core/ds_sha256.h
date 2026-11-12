/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_sha256.h - SHA-256 as specified in FIPS 180-4, written in-house for the content identity of a run (D-47). The digest is
 * taken over the bytes exactly as supplied: no normalisation, no header strip, no byte swap. Any chunk size may be fed to update.
 * A matching digest is evidence of repeatability only; it does not show that the content is genuine.
 * Only stddef.h and stdint.h are included, as everywhere in the core (D-54).
 */
#ifndef DS_SHA256_H
#define DS_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define DS_SHA256_BYTES 32u

struct ds_sha256 {
    uint32_t h[8];   /* the running hash value */
    uint8_t buf[64]; /* the bytes of the block being collected */
    uint64_t bits;   /* the message length so far, counted in BITS (the padding writes it big-endian) */
    uint32_t fill;   /* bytes held in buf, always below 64 between calls */
};

void ds_sha256_init(struct ds_sha256 *s);
/* data may be NULL only when len is 0 */
void ds_sha256_update(struct ds_sha256 *s, const uint8_t *data, size_t len);
/* writes the 32 digest bytes (big-endian words) and leaves s unusable until the next init */
void ds_sha256_final(struct ds_sha256 *s, uint8_t out[DS_SHA256_BYTES]);
/* init, update and final in one call */
void ds_sha256(const uint8_t *data, size_t len, uint8_t out[DS_SHA256_BYTES]);

#endif
