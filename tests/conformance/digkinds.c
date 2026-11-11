/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * digkinds.c - the case kinds of the canonical state digest and the profile identity hash (plan 13; D-52, D-43, D-55, D-27). Evidence tier E0:
 * a digest shows that a state is the same state, never that the state is right. The framing and the literals below are independent of the
 * code under test: the stream is parsed here byte by byte without calling ds_digest, and the literals come from a throwaway perl script that
 * writes the stream of the written rules (Digest::SHA and Math::BigInt, outside the repo). Test code only, never installed.
 *   digest_stream   boots the request, captures ds_digest_stream, and checks the prefix, the NUL, the version and the frame of each region in
 *                   order, every payload against the instance, the stream length, and then that SHA-256 of the captured stream equals
 *                   ds_digest. expect items, separated by ';': version=N, frames=TAG:LEN,TAG:LEN,..., size=N, enum=N, digest=HEX64
 *   digest_pair     request is two requests separated by "||"; the first is booted twice, into blocks prefilled with different bytes, and
 *                   the second once; the two boots of the first must agree, and expect is same (the second agrees too) or differ
 *   digest_region   boots the request, and for every region reached through ds_region_info flips one byte of live state, the first and the
 *                   last byte of its live span: each flip must change the digest, and restoring the byte must restore it. One assertion per
 *                   region. expect tags=A,B,...: the region count and order; TAG:first says that only the first byte of the live span is
 *                   text of the stream (the PROF span is the whole profile struct, whose tail is the padding of the last value, not hashed)
 *   profile_hash    expect fields: each of the 13 fields flipped (value, status, bound_by) changes ds_profile_hash, one assertion per field;
 *                   expect identity;unbound=HEX;resolved=HEX: literals, id and revision flips, bytes after a NUL are not hashed, the sorted
 *                   unresolved ids; the resolved literal is the profile of the request bound as the default request binds
 *   nonmutation_digest   boots a live instance, then boots every refuse row of the cases into the memory of that live instance and takes the
 *                   digest before and after each: one assertion per refuse row plus one for the whole run
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_serial.h"
#include "ds_sha256.h"
#include "runner.h"
#include "synth.h"

#define DK_CAP 16384u
#define DK_LIST 400u

/* the captured stream */
struct capture {
    uint8_t buf[DK_CAP];
    size_t n;
    int over;
};

static void cap_sink(void *ctx, const uint8_t *data, size_t len)
{
    struct capture *cp = (struct capture *)ctx;

    if (len > DK_CAP - cp->n) {
        cp->over = 1;
        return;
    }
    memcpy(cp->buf + cp->n, data, len);
    cp->n += len;
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t rd64(const uint8_t *p)
{
    return (uint64_t)rd32(p) | ((uint64_t)rd32(p + 4) << 32);
}

static void to_hex(const uint8_t *d, size_t n, char *out)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0u; i < n; i++) {
        out[2u * i] = digits[d[i] >> 4];
        out[2u * i + 1u] = digits[d[i] & 0x0fu];
    }
    out[2u * n] = '\0';
}

/* a booted instance in a block that was filled with one byte first, so that a digest cannot depend on what the block held before */
struct block {
    unsigned char *mem;
    unsigned char *content;
};

static void block_free(struct block *b)
{
    free(b->content);
    free(b->mem);
    b->content = NULL;
    b->mem = NULL;
}

static const char *block_boot(const char *fixture, const char *request, unsigned char fill, struct block *b,
                              const struct ds_instance **inst)
{
    struct spec s;
    struct ds_boot_request rq;
    struct ds_boot_result res;
    size_t natural = synth_size(fixture);
    const char *err;

    b->mem = NULL;
    b->content = NULL;
    if (natural == 0u) {
        return "unknown fixture recipe";
    }
    err = parse_spec(&s, request, natural);
    if (err != NULL) {
        return err;
    }
    if (s.ptr != PTR_OK || s.len > CONTENT_LIMIT) {
        return "a digest case needs ptr=ok and a len within the content limit";
    }
    b->mem = (unsigned char *)malloc(ds_instance_size());
    b->content = (unsigned char *)malloc(s.len != 0u ? s.len : 1u);
    if (b->mem == NULL || b->content == NULL) {
        block_free(b);
        return "out of memory for the instance or the content";
    }
    memset(b->mem, (int)fill, ds_instance_size());
    if (synth_fill(fixture, b->content, s.len) != 0) {
        block_free(b);
        return "unknown fixture recipe";
    }
    fill_request(&rq, &s, b->content);
    memset(&res, 0xff, sizeof res);
    if (ds_boot(b->mem, ds_instance_size(), &rq, &res) != 0u || res.instance == NULL) {
        block_free(b);
        return "the request was refused: this kind needs an accepted boot";
    }
    *inst = res.instance;
    return NULL;
}

/* ---- kind digest_stream ---- */

#define DS_REGIONS_MAX 8u

struct frame_want {
    char tag[5];
    uint32_t len;
};

/* parse "TAG:LEN,TAG:LEN,..." into f[]; the count, or -1 when malformed or more than max */
static int parse_frames(char *list, struct frame_want *f, unsigned max)
{
    char *rest = list;
    char *item;
    unsigned n = 0u;

    while ((item = pk_next_item(&rest, ',')) != NULL) {
        char *colon = strchr(item, ':');
        size_t len;

        if (colon == NULL || colon - item != 4 || n == max || parse_size(colon + 1, &len) != 0 || len > DK_CAP) {
            return -1;
        }
        memcpy(f[n].tag, item, 4u);
        f[n].tag[4] = '\0';
        f[n].len = (uint32_t)len;
        n++;
    }
    return (int)n;
}

void kind_digest_stream(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static struct capture cp;
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    char list[PK_LIST_MAX];
    char frames_text[DK_LIST];
    char *rest = list;
    char *item;
    struct frame_want fw[DS_REGIONS_MAX];
    int nframes = 0;
    int have_frames = 0;
    uint32_t want_version = 0u;
    int have_version = 0;
    size_t want_size = 0u;
    uint64_t want_enum = 0u;
    char want_digest[65];
    const uint8_t *p;
    size_t left;
    int i;
    uint8_t ph[DS_DIGEST_BYTES];
    uint8_t dg[DS_DIGEST_BYTES];
    struct ds_sha256 st;
    uint8_t viasha[DS_SHA256_BYTES];

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    want_digest[0] = '\0';
    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        release_boot(&b);
        harness_error(c, "the expect list is too long");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        uint64_t num;

        if (strncmp(item, "version=", 8) == 0 && parse_u64(item + 8, &num) == 0) {
            want_version = (uint32_t)num;
            have_version = 1;
        } else if (strncmp(item, "frames=", 7) == 0 && pk_copy_text(item + 7, frames_text, sizeof frames_text) == 0) {
            nframes = parse_frames(frames_text, fw, DS_REGIONS_MAX);
            have_frames = nframes > 0;
        } else if (strncmp(item, "size=", 5) == 0 && parse_size(item + 5, &want_size) == 0) {
            continue;
        } else if (strncmp(item, "enum=", 5) == 0 && parse_u64(item + 5, &want_enum) == 0) {
            continue;
        } else if (strncmp(item, "digest=", 7) == 0 && strlen(item + 7) == 64u) {
            memcpy(want_digest, item + 7, 65u);
        } else {
            release_boot(&b);
            harness_error(c, "a digest_stream expect item is malformed");
            return;
        }
    }
    if (!have_version || !have_frames || want_size == 0u || want_enum == 0u || want_digest[0] == '\0') {
        release_boot(&b);
        harness_error(c, "a digest_stream expect needs version, frames, size, enum and digest");
        return;
    }

    memset(&cp, 0, sizeof cp);
    ds_digest_stream(inst, cap_sink, &cp);
    check(c, !cp.over, "the captured stream is longer than the capture buffer");

    /* the prefix, the NUL and the version: parsed here, not by the serializer */
    check(c, cp.n >= 19u && memcmp(cp.buf, "duospool-state\0", 15u) == 0, "the stream does not start with the prefix duospool-state and a NUL byte");
    check(c, cp.n >= 19u && rd32(cp.buf + 15) == want_version, "the serializer version after the prefix differs");
    p = cp.buf + (cp.n >= 19u ? 19u : cp.n);
    left = cp.n >= 19u ? cp.n - 19u : 0u;

    ph[0] = 0u;
    ds_profile_hash(&inst->profile, ph);
    for (i = 0; i < nframes; i++) {
        char msg[160];
        const uint8_t *pl;
        int frame_ok = left >= 8u && memcmp(p, fw[i].tag, 4u) == 0 && rd32(p + 4) == fw[i].len && left - 8u >= fw[i].len;

        snprintf(msg, sizeof msg, "frame %d is not tag %s with length %u at its place in the stream", i, fw[i].tag, (unsigned)fw[i].len);
        check(c, frame_ok, msg);
        if (!frame_ok) {
            left = 0u;
            break;
        }
        pl = p + 8;
        snprintf(msg, sizeof msg, "the payload of region %s differs from the instance", fw[i].tag);
        if (strcmp(fw[i].tag, "PROF") == 0) {
            check(c, fw[i].len == 32u && memcmp(pl, ph, 32u) == 0, msg);
        } else if (strcmp(fw[i].tag, "CONT") == 0) {
            check(c, fw[i].len == 40u && memcmp(pl, inst->content_id, 32u) == 0 && (size_t)rd32(pl + 32) == want_size &&
                         (uint64_t)rd32(pl + 36) == want_enum && inst->size_class == (uint32_t)want_enum,
                  msg);
        } else if (strcmp(fw[i].tag, "POWR") == 0) {
            check(c, fw[i].len == 16u && rd32(pl) == inst->powerup_policy && rd64(pl + 4) == inst->powerup_seed &&
                         rd32(pl + 12) == inst->has_seed,
                  msg);
        } else if (strcmp(fw[i].tag, "TIME") == 0) {
            check(c, fw[i].len == 8u && rd64(pl) == inst->machine_time, msg);
        } else if (strcmp(fw[i].tag, "RAM8") == 0) {
            check(c, fw[i].len == DS_RAM8K_BYTES && memcmp(pl, inst->ram8k, DS_RAM8K_BYTES) == 0, msg);
        } else if (strcmp(fw[i].tag, "MPRS") == 0) {
            check(c, fw[i].len == DS_MPR_COUNT && memcmp(pl, inst->mpr, DS_MPR_COUNT) == 0, msg);
        } else if (strcmp(fw[i].tag, "CPU0") == 0) {
            /* a x y s p, then pc as a u16 little-endian, then speed: read byte by byte, in the stream order of the documented rule */
            check(c, fw[i].len == 8u && pl[0] == inst->a && pl[1] == inst->x && pl[2] == inst->y && pl[3] == inst->s && pl[4] == inst->p &&
                         (unsigned)(pl[5] | ((unsigned)pl[6] << 8)) == (unsigned)inst->pc && pl[7] == inst->speed,
                  msg);
        } else {
            harness_error(c, "a digest_stream frame names a tag that the case does not know");
            break;
        }
        p += 8u + fw[i].len;
        left -= 8u + fw[i].len;
    }
    check(c, left == 0u, "bytes are left over after the last region, or the regions did not add up to the stream");

    /* the digest is exactly SHA-256 of that stream, and equals the literal derived outside the code */
    ds_sha256_init(&st);
    ds_sha256_update(&st, cp.buf, cp.n);
    ds_sha256_final(&st, viasha);
    check(c, ds_digest(inst, dg) == 0u && memcmp(dg, viasha, DS_SHA256_BYTES) == 0, "ds_digest is not the SHA-256 of the captured stream");
    {
        char hex[65];

        to_hex(dg, DS_DIGEST_BYTES, hex);
        check(c, strcmp(hex, want_digest) == 0, "the digest differs from the independently derived literal");
        snprintf(got, gotcap, "%.16s", hex);
    }
    release_boot(&b);
}

/* ---- kind digest_pair ---- */

void kind_digest_pair(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char text[PK_LIST_MAX];
    char *bar;
    const char *req_a;
    const char *req_b;
    struct block ba1;
    struct block ba2;
    struct block bb;
    const struct ds_instance *ia1 = NULL;
    const struct ds_instance *ia2 = NULL;
    const struct ds_instance *ib = NULL;
    uint8_t da1[DS_DIGEST_BYTES];
    uint8_t da2[DS_DIGEST_BYTES];
    uint8_t db[DS_DIGEST_BYTES];
    const char *err;
    int want_same;
    char hex[65];

    if (strcmp(k->val[K_EXPECT], "same") != 0 && strcmp(k->val[K_EXPECT], "differ") != 0) {
        harness_error(c, "a digest_pair expect is same or differ");
        return;
    }
    want_same = strcmp(k->val[K_EXPECT], "same") == 0;
    if (pk_copy_text(k->val[K_REQUEST], text, sizeof text) != 0 || (bar = strstr(text, "||")) == NULL) {
        harness_error(c, "a digest_pair request is two requests separated by ||");
        return;
    }
    *bar = '\0';
    req_a = text;
    req_b = bar + 2;
    err = block_boot(k->val[K_FIXTURE], req_a, 0xaau, &ba1, &ia1);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    err = block_boot(k->val[K_FIXTURE], req_a, 0x55u, &ba2, &ia2);
    if (err != NULL) {
        block_free(&ba1);
        harness_error(c, err);
        return;
    }
    err = block_boot(k->val[K_FIXTURE], req_b, 0x00u, &bb, &ib);
    if (err != NULL) {
        block_free(&ba1);
        block_free(&ba2);
        harness_error(c, err);
        return;
    }
    check(c, ds_digest(ia1, da1) == 0u && ds_digest(ia2, da2) == 0u && ds_digest(ib, db) == 0u, "ds_digest refused a booted instance");
    /* the same request is the same digest whatever the block held before the boot (two blocks, two different fills) */
    check(c, memcmp(da1, da2, DS_DIGEST_BYTES) == 0, "the same request gave two different digests in two blocks");
    if (want_same) {
        check(c, memcmp(da1, db, DS_DIGEST_BYTES) == 0, "the two requests of a same pair gave different digests");
    } else {
        check(c, memcmp(da1, db, DS_DIGEST_BYTES) != 0, "the two requests of a differ pair gave the same digest");
    }
    to_hex(da1, DS_DIGEST_BYTES, hex);
    snprintf(got, gotcap, "%.16s", hex);
    block_free(&ba1);
    block_free(&ba2);
    block_free(&bb);
}

/* ---- kind digest_region ---- */

void kind_digest_region(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *cinst = NULL;
    const char *err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &cinst);
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    const char *want[DS_REGIONS_MAX];
    unsigned nwant = 0u;
    unsigned index;
    uint8_t before[DS_DIGEST_BYTES];

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (strncmp(k->val[K_EXPECT], "tags=", 5) != 0 || pk_copy_text(k->val[K_EXPECT] + 5, list, sizeof list) != 0) {
        release_boot(&b);
        harness_error(c, "a digest_region expect is tags=A,B,...");
        return;
    }
    while ((item = pk_next_item(&rest, ',')) != NULL && nwant < DS_REGIONS_MAX) {
        want[nwant++] = item;
    }
    if (nwant == 0u) {
        release_boot(&b);
        harness_error(c, "a digest_region expect lists no tag");
        return;
    }
    /* the instance is the caller's own writable block: the accessor takes it non-const, so a test can reach the live storage */
    {
        struct ds_instance *inst = (struct ds_instance *)b.mem;
        unsigned count = 0u;
        const char *tag;
        uint8_t *ptr;
        size_t len;

        while (ds_region_info(inst, count, &tag, &ptr, &len) != 0u) {
            count++;
        }
        if (count != nwant) {
            /* the region count is the number of assertions: a different number fails the case here, once */
            check(c, 0, "the region table does not have the number of regions the case lists");
            release_boot(&b);
            snprintf(got, gotcap, "regions=%u", count);
            return;
        }
        if (ds_digest(inst, before) != 0u) {
            check(c, 0, "ds_digest refused a booted instance");
            release_boot(&b);
            return;
        }
        for (index = 0u; index < count; index++) {
            uint8_t flipped[DS_DIGEST_BYTES];
            uint8_t restored[DS_DIGEST_BYTES];
            const char *why = NULL;
            int in_block = 0;
            int first_only = 0;
            char want_tag[5];
            size_t wl;
            unsigned pass;

            if (ds_region_info(inst, index, &tag, &ptr, &len) == 0u || tag == NULL || ptr == NULL) {
                check(c, 0, "ds_region_info does not answer for an index below the count");
                break;
            }
            in_block = len > 0u && ptr >= b.mem && len <= ds_instance_size() && (size_t)(ptr - b.mem) <= ds_instance_size() - len;
            wl = strlen(want[index]);
            memset(want_tag, 0, sizeof want_tag);
            memcpy(want_tag, want[index], wl < 4u ? wl : 4u);
            first_only = wl == 10u && strcmp(want[index] + 4, ":first") == 0;
            if ((wl != 4u && !first_only) || strcmp(tag, want_tag) != 0) {
                why = "a region has another tag than the case lists at its place";
            } else if (!in_block) {
                why = "the live span of a region is empty or outside the instance";
            }
            for (pass = 0u; why == NULL && pass < (first_only ? 1u : 2u); pass++) {
                uint8_t *at = pass == 0u ? ptr : ptr + (len - 1u);
                uint8_t saved = *at;

                *at = (uint8_t)(saved ^ 0x01u);
                (void)ds_digest(inst, flipped);
                *at = saved;
                (void)ds_digest(inst, restored);
                if (memcmp(flipped, before, DS_DIGEST_BYTES) == 0) {
                    why = pass == 0u ? "flipping the first byte of the live state of a region did not change the digest"
                                     : "flipping the last byte of the live state of a region did not change the digest";
                } else if (memcmp(restored, before, DS_DIGEST_BYTES) != 0) {
                    why = "restoring the flipped byte did not restore the digest";
                }
            }
            check(c, why == NULL, why != NULL ? why : "");
            if (why != NULL) {
                emit_err("digest_region: region %u (%s): %s\n", index, tag, why);
            }
        }
        snprintf(got, gotcap, "regions=%u", count);
    }
    release_boot(&b);
}

/* ---- kind profile_hash ---- */

static int hash_differs(const struct ds_profile *a, const uint8_t base[DS_DIGEST_BYTES])
{
    uint8_t h[DS_DIGEST_BYTES];

    ds_profile_hash(a, h);
    return memcmp(h, base, DS_DIGEST_BYTES) != 0;
}

static void profile_hash_fields(struct ctx *c, char *got, size_t gotcap)
{
    const struct ds_profile *base = ds_profile_builtin("pce-hucard-2btn", 1u);
    uint8_t h0[DS_DIGEST_BYTES];
    uint32_t f;

    if (base == NULL) {
        check(c, 0, "the built-in profile is not registered");
        return;
    }
    ds_profile_hash(base, h0);
    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
        struct ds_profile p;
        int value_changes;
        int status_changes;
        int bound_changes;
        char msg[160];

        p = *base;
        p.field[f].value[0] = (char)(p.field[f].value[0] ^ 0x01);
        value_changes = hash_differs(&p, h0);
        p = *base;
        p.field[f].status = (uint8_t)(p.field[f].status ^ 0x03u);
        status_changes = hash_differs(&p, h0);
        p = *base;
        p.field[f].bound_by = (uint8_t)(p.field[f].bound_by ^ 0x01u);
        bound_changes = hash_differs(&p, h0);
        snprintf(msg, sizeof msg, "flipping the value, the status or bound_by of field %s did not change the profile hash", ds_field_name(f));
        check(c, value_changes && status_changes && bound_changes, msg);
    }
    snprintf(got, gotcap, "fields=%u", (unsigned)DS_FIELD_COUNT);
}

static void profile_hash_identity(const char *expect, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    char want_unbound[65];
    char want_resolved[65];
    const struct ds_profile *base = ds_profile_builtin("pce-hucard-2btn", 1u);
    struct ds_profile resolved;
    struct ds_bindings bind;
    struct spec s;
    uint8_t h[DS_DIGEST_BYTES];
    uint8_t h2[DS_DIGEST_BYTES];
    char hex[65];
    uint32_t i;
    uint32_t field = 0u;
    const char *err;

    want_unbound[0] = '\0';
    want_resolved[0] = '\0';
    if (base == NULL || pk_copy_text(expect, list, sizeof list) != 0) {
        harness_error(c, "the built-in profile is not registered, or the expect list is too long");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "unbound=", 8) == 0 && strlen(item + 8) == 64u) {
            memcpy(want_unbound, item + 8, 65u);
        } else if (strncmp(item, "resolved=", 9) == 0 && strlen(item + 9) == 64u) {
            memcpy(want_resolved, item + 9, 65u);
        } else if (strcmp(item, "identity") != 0) {
            harness_error(c, "a profile_hash identity expect item is malformed");
            return;
        }
    }
    if (want_unbound[0] == '\0' || want_resolved[0] == '\0') {
        harness_error(c, "a profile_hash identity expect needs unbound= and resolved=");
        return;
    }
    err = parse_spec(&s, "", 0u);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        bind.value[i] = s.bind[i];
    }
    if (ds_resolve("pce-hucard-2btn", 1u, &bind, &resolved, &field) != DS_OK) {
        harness_error(c, "the default request does not resolve");
        return;
    }

    /* literals from the perl derivation of the written stream: the unbound constant and the default-bound profile (policy zero) */
    ds_profile_hash(base, h);
    to_hex(h, DS_DIGEST_BYTES, hex);
    check(c, strcmp(hex, want_unbound) == 0, "the hash of the unbound r1 constant differs from the independently derived literal");
    ds_profile_hash(&resolved, h);
    to_hex(h, DS_DIGEST_BYTES, hex);
    check(c, strcmp(hex, want_resolved) == 0, "the hash of the default-bound profile differs from the independently derived literal");
    snprintf(got, gotcap, "%.16s", hex);
    ds_profile_hash(&resolved, h2);
    check(c, memcmp(h, h2, DS_DIGEST_BYTES) == 0, "the same profile gave two different hashes");

    {
        struct ds_profile p = resolved;

        p.id[0] = (char)(p.id[0] ^ 0x01);
        check(c, hash_differs(&p, h), "flipping the profile id did not change the hash");
        p = resolved;
        p.rev = p.rev + 1u;
        check(c, hash_differs(&p, h), "changing the profile revision did not change the hash");
        /* bytes after the NUL of a string are not text: the identity is the text, not the array */
        p = resolved;
        p.id[20] = 'x';
        p.field[DS_FIELD_media_slot].value[20] = 'x';
        check(c, !hash_differs(&p, h), "a byte after the NUL of a string changed the hash: the array, not the text, was hashed");
    }
    /* the unresolved behaviour ids are listed in strictly increasing order (the hash covers them in this order) */
    check(c, ds_profile_unresolved_count() > 0u, "the profile lists no unresolved behaviour");
    for (i = 1u; i < ds_profile_unresolved_count(); i++) {
        check(c, strcmp(ds_profile_unresolved(i - 1u), ds_profile_unresolved(i)) < 0, "the unresolved behaviour ids are not in strictly increasing order");
    }
}

void kind_profile_hash(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    if (strcmp(k->val[K_EXPECT], "fields") == 0) {
        profile_hash_fields(c, got, gotcap);
    } else if (strncmp(k->val[K_EXPECT], "identity;", 9) == 0) {
        profile_hash_identity(k->val[K_EXPECT], c, got, gotcap);
    } else {
        harness_error(c, "a profile_hash expect is fields or identity;unbound=HEX;resolved=HEX");
    }
}

/* ---- kind nonmutation_digest ---- */

void kind_nonmutation_digest(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted live;
    struct spec s0;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted("plain-256k", k->val[K_REQUEST], &live, &s0, &inst);
    uint8_t first[DS_DIGEST_BYTES];
    size_t isize = ds_instance_size();
    size_t i;
    unsigned rows = 0u;

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    snprintf(got, gotcap, "rows=0");
    if (ds_digest(inst, first) != 0u) {
        check(c, 0, "ds_digest refused a booted instance");
        release_boot(&live);
        return;
    }
    for (i = 0u; i < case_count(); i++) {
        const struct kase *r = case_get(i);
        static const unsigned char dummy[1] = {0u};
        struct spec s;
        struct expect e;
        struct ds_boot_request rq;
        struct ds_boot_result res;
        uint8_t d0[DS_DIGEST_BYTES];
        uint8_t d1[DS_DIGEST_BYTES];
        unsigned char *content = NULL;
        const unsigned char *cptr = NULL;
        size_t natural;
        void *mem;
        size_t cap;
        uint32_t ret;
        const char *rerr;

        if (!r->has[K_KIND] || strcmp(r->val[K_KIND], "refuse") != 0) {
            continue;
        }
        if (r->bad != NULL || !r->has[K_FIXTURE] || !r->has[K_REQUEST] || !r->has[K_EXPECT]) {
            harness_error(c, "a refuse row is malformed");
            break;
        }
        natural = synth_size(r->val[K_FIXTURE]);
        rerr = natural == 0u ? "unknown fixture recipe" : parse_spec(&s, r->val[K_REQUEST], natural);
        if (rerr == NULL) {
            rerr = parse_expect(&e, r->val[K_EXPECT]);
        }
        if (rerr == NULL && s.ptr == PTR_OK && s.len > CONTENT_LIMIT) {
            rerr = "len above the content limit needs ptr=dummy or ptr=null";
        }
        if (rerr != NULL) {
            harness_error(c, rerr);
            break;
        }
        if (s.ptr == PTR_OK) {
            content = (unsigned char *)malloc(s.len != 0u ? s.len : 1u);
            if (content == NULL || synth_fill(r->val[K_FIXTURE], content, s.len) != 0) {
                free(content);
                harness_error(c, "out of memory or unknown recipe in the digest non-mutation matrix");
                break;
            }
        }
        cptr = s.ptr == PTR_OK ? content : (s.ptr == PTR_DUMMY ? dummy : NULL);
        fill_request(&rq, &s, cptr);
        memset(&res, 0xff, sizeof res);
        /* the refused boot is aimed at the memory of the live instance: a refusal must leave a live machine untouched */
        mem = s.mem == MEM_NULL ? NULL : (s.mem == MEM_MISALIGNED ? (void *)(live.mem + 1) : (void *)live.mem);
        cap = s.cap == CAP_SHORT ? isize - 1u : isize;
        (void)ds_digest(inst, d0);
        ret = ds_boot(mem, cap, s.req == REQ_NULL ? NULL : &rq, &res);
        (void)ds_digest(inst, d1);
        free(content);
        check(c, ret == e.reason && !e.accept && memcmp(d0, d1, DS_DIGEST_BYTES) == 0,
              "a refuse row was not refused as expected, or it changed the digest of a live instance");
        if (ret != e.reason || memcmp(d0, d1, DS_DIGEST_BYTES) != 0) {
            emit_err("nonmutation_digest: row %s changed the live digest or was not refused as expected\n", r->stem);
        }
        rows++;
    }
    if (!c->harness) {
        uint8_t last[DS_DIGEST_BYTES];

        (void)ds_digest(inst, last);
        check(c, memcmp(first, last, DS_DIGEST_BYTES) == 0, "the digest of the live instance differs after the whole matrix");
        if (rows == 0u) {
            check(c, 0, "the cases hold no refuse row");
        }
        snprintf(got, gotcap, "rows=%u", rows);
    }
    release_boot(&live);
}
