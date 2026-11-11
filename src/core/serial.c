/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * serial.c - the canonical state stream, the state digest and the profile identity hash (CORE-01, PROF-03; D-52, D-27, D-83).
 * The digest is SHA-256 over a framed stream, never over the bytes of a struct (padding, pointers and layout would leak in, and a
 * field could be added to the struct and silently missed). The stream is the prefix "duospool-state" and a NUL byte, the serializer
 * version as a u32, and then the regions of the table below in order, each framed as its four-byte ASCII tag, a u32 payload length and
 * the payload. Every integer is written explicitly little-endian, byte by byte, from unsigned operands widened before every shift:
 * the sanitize preset has no -fwrapv. The ROM bytes are not in the stream (the content id is the SHA-256 of the copy and stands for
 * them), and no pointer, padding, wall time or path enters it.
 * One region table serves the digest, ds_digest_stream and the ds_region_info test accessor, so a test that walks the accessor walks
 * exactly the regions the digest has. A region that is listed but whose payload the stream does not carry is what the digest_region
 * case and the drop-region toy control exist to catch. A matching digest is evidence of repeatability only, never of correctness.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_serial.h"
#include "ds_sha256.h"

/* ---- the stream writer: u8-sized pieces, little-endian integers, length-prefixed strings ---- */

struct writer {
    ds_sink_fn sink;
    void *ctx;
};

static void w_bytes(struct writer *w, const uint8_t *p, size_t n)
{
    w->sink(w->ctx, p, n);
}

static void w_u32(struct writer *w, uint32_t v)
{
    uint8_t b[4];

    b[0] = (uint8_t)(v & 0xffu);
    b[1] = (uint8_t)((v >> 8) & 0xffu);
    b[2] = (uint8_t)((v >> 16) & 0xffu);
    b[3] = (uint8_t)((v >> 24) & 0xffu);
    w_bytes(w, b, sizeof b);
}

static void w_u64(struct writer *w, uint64_t v)
{
    uint8_t b[8];
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        b[i] = (uint8_t)((v >> (8u * i)) & UINT64_C(0xff));
    }
    w_bytes(w, b, sizeof b);
}

/* a string is its length as a u32 and then its bytes without the NUL; the read stops at cap, so an unterminated array cannot be overrun */
static void w_str(struct writer *w, const char *s, uint32_t cap)
{
    uint32_t n = 0u;

    while (n < cap && s[n] != '\0') {
        n++;
    }
    w_u32(w, n);
    w_bytes(w, (const uint8_t *)s, (size_t)n);
}

static void sha_sink(void *ctx, const uint8_t *data, size_t len)
{
    ds_sha256_update((struct ds_sha256 *)ctx, data, len);
}

/* ---- the profile identity hash ---- */

#define DS_PROFILE_HASH_VERSION 1u
#define DS_NAME_CAP 64u

static const char profile_prefix[] = "duospool-profile"; /* the stream writes this and its NUL (sizeof counts the NUL) */

void ds_profile_hash(const struct ds_profile *profile, uint8_t out[DS_DIGEST_BYTES])
{
    struct ds_sha256 st;
    struct writer w;
    uint32_t i;

    if (out == NULL) {
        return;
    }
    if (profile == NULL) {
        for (i = 0u; i < DS_DIGEST_BYTES; i++) {
            out[i] = 0u;
        }
        return;
    }
    ds_sha256_init(&st);
    w.sink = sha_sink;
    w.ctx = &st;
    w_bytes(&w, (const uint8_t *)profile_prefix, sizeof profile_prefix);
    w_u32(&w, DS_PROFILE_HASH_VERSION);
    w_str(&w, profile->id, DS_PROFILE_ID_MAX);
    w_u32(&w, profile->rev);
    w_u32(&w, (uint32_t)DS_FIELD_COUNT);
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        w_str(&w, ds_field_name(i), DS_NAME_CAP);
        w_u32(&w, (uint32_t)profile->field[i].status);
        w_str(&w, profile->field[i].value, DS_FIELD_VALUE_MAX);
        w_u32(&w, (uint32_t)profile->field[i].bound_by);
    }
    /* the unresolved behaviours are a property of the registered (id, rev); their ids are listed in sorted order */
    w_u32(&w, ds_profile_unresolved_count());
    for (i = 0u; i < ds_profile_unresolved_count(); i++) {
        w_str(&w, ds_profile_unresolved(i), DS_NAME_CAP);
    }
    ds_sha256_final(&st, out);
}

/* ---- the region table ---- */

enum region_id { R_PROF, R_CONT, R_POWR, R_TIME, R_RAM8, R_MPRS, R_PSGN, R_VDCP, R_COUNT };

struct region {
    char tag[5];            /* four ASCII bytes and a NUL, which is not written */
    uint32_t payload_len;   /* the length written in the frame; the payload must be exactly this long */
    size_t live_off;        /* the live storage whose first byte the payload carries (test accessor) */
    size_t live_len;
};

/* the power-on policy, the has_seed flag and the seed are neighbours in the instance: the accessor spans them as one live range */
_Static_assert(offsetof(struct ds_instance, has_seed) == offsetof(struct ds_instance, powerup_policy) + sizeof(uint32_t),
               "powerup_policy and has_seed are not adjacent");
_Static_assert(offsetof(struct ds_instance, powerup_seed) == offsetof(struct ds_instance, has_seed) + sizeof(uint32_t),
               "has_seed and powerup_seed are not adjacent");

static const struct region regions[R_COUNT] = {
    [R_PROF] = {"PROF", DS_DIGEST_BYTES, offsetof(struct ds_instance, profile), sizeof(struct ds_profile)},
    [R_CONT] = {"CONT", DS_CONTENT_ID_BYTES + 4u + 4u, offsetof(struct ds_instance, content_id), DS_CONTENT_ID_BYTES},
    [R_POWR] = {"POWR", 4u + 8u + 4u, offsetof(struct ds_instance, powerup_policy),
                offsetof(struct ds_instance, powerup_seed) + sizeof(uint64_t) - offsetof(struct ds_instance, powerup_policy)},
    [R_TIME] = {"TIME", 8u, offsetof(struct ds_instance, machine_time), sizeof(uint64_t)},
    [R_RAM8] = {"RAM8", DS_RAM8K_BYTES, offsetof(struct ds_instance, ram8k), DS_RAM8K_BYTES},
    [R_MPRS] = {"MPRS", DS_MPR_COUNT, offsetof(struct ds_instance, mpr), DS_MPR_COUNT},
    [R_PSGN] = {"PSGN", 4u, offsetof(struct ds_instance, psg_noise), sizeof(uint32_t)},
    [R_VDCP] = {"VDCP", 4u, offsetof(struct ds_instance, vdc_phase), sizeof(uint32_t)},
};

/* the payload of one region: exactly regions[index].payload_len bytes, one explicit field per call, never a struct */
static void emit_payload(struct writer *w, const struct ds_instance *inst, uint32_t index)
{
    switch (index) {
    case R_PROF: {
        uint8_t h[DS_DIGEST_BYTES];

        ds_profile_hash(&inst->profile, h);
        w_bytes(w, h, sizeof h);
        break;
    }
    case R_CONT:
        w_bytes(w, inst->content_id, DS_CONTENT_ID_BYTES);
        w_u32(w, ds_size_bytes(inst->size_class));
        w_u32(w, inst->size_class);
        break;
    case R_POWR:
        w_u32(w, inst->powerup_policy);
        w_u64(w, inst->powerup_seed);
        w_u32(w, inst->has_seed);
        break;
    case R_TIME:
        w_u64(w, inst->machine_time);
        break;
    case R_RAM8:
        w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);
        break;
    case R_MPRS:
        w_bytes(w, inst->mpr, DS_MPR_COUNT);
        break;
    case R_PSGN:
        w_u32(w, inst->psg_noise);
        break;
    case R_VDCP:
        w_u32(w, inst->vdc_phase);
        break;
    default:
        break;
    }
}

static const char stream_prefix[] = "duospool-state"; /* the stream writes this and its NUL */

void ds_digest_stream(const struct ds_instance *inst, ds_sink_fn sink, void *ctx)
{
    struct writer w;
    uint32_t i;

    if (inst == NULL || sink == NULL) {
        return;
    }
    w.sink = sink;
    w.ctx = ctx;
    w_bytes(&w, (const uint8_t *)stream_prefix, sizeof stream_prefix);
    w_u32(&w, DS_SERIAL_VERSION);
    for (i = 0u; i < (uint32_t)R_COUNT; i++) {
        w_bytes(&w, (const uint8_t *)regions[i].tag, 4u);
        w_u32(&w, regions[i].payload_len);
        emit_payload(&w, inst, i);
    }
}

uint32_t ds_digest(const struct ds_instance *inst, uint8_t out[DS_DIGEST_BYTES])
{
    struct ds_sha256 st;

    if (inst == NULL || out == NULL) {
        return 1u;
    }
    ds_sha256_init(&st);
    ds_digest_stream(inst, sha_sink, &st);
    ds_sha256_final(&st, out);
    return DS_OK;
}

uint32_t ds_region_info(struct ds_instance *inst, uint32_t index, const char **tag, uint8_t **ptr, size_t *len)
{
    if (inst == NULL || tag == NULL || ptr == NULL || len == NULL || index >= (uint32_t)R_COUNT) {
        return 0u;
    }
    *tag = regions[index].tag;
    *ptr = (uint8_t *)inst + regions[index].live_off;
    *len = regions[index].live_len;
    return 1u;
}
