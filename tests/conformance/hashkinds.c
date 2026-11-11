/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * hashkinds.c - the case kinds of the content identity: content_id (the id stored by ds_boot, the core's SHA-256 over a fresh copy of the
 * recipe and the content.sha256 line of the record are one value) and hash_literal (NIST FIPS 180-4 example inputs, chunked and whole).
 * Moved out of runner.c by plan 03-03 (D-52 amended, F7) with no change of behaviour; the sweep lengths and main stay in runner.c.
 * Evidence tier E0: a matching hash shows repeatability, not that the content is genuine. Test code only, never installed.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_sha256.h"
#include "ds_text.h"
#include "runner.h"
#include "synth.h"

/* ---- the content identity (D-47): hex form and the kinds content_id and hash_literal ---- */

/* lowercase hex of n bytes into out (2 * n + 1 bytes) */
static void to_hex(const unsigned char *bytes, size_t n, char *out)
{
    static const char digit[] = "0123456789abcdef";
    size_t i;

    for (i = 0u; i < n; i++) {
        out[2u * i] = digit[(bytes[i] >> 4) & 0x0fu];
        out[2u * i + 1u] = digit[bytes[i] & 0x0fu];
    }
    out[2u * n] = '\0';
}

/* 64 characters of 0-9 a-f and nothing else: the one accepted spelling of a digest */
static int is_hex64(const char *s)
{
    size_t i;

    for (i = 0u; i < 64u; i++) {
        if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) {
            return 0;
        }
    }
    return s[64] == '\0';
}

/* SHA-256 of a recipe's bytes through the core, as hex; 0 on success, -1 when the recipe is unknown or memory is short */
int hash_recipe(const char *recipe, char *hex)
{
    size_t n;
    unsigned char *buf;
    unsigned char digest[DS_SHA256_BYTES];

    if (synth_parse(recipe, &n) != 0) {
        return -1;
    }
    buf = (unsigned char *)malloc(n != 0u ? n : 1u);
    if (buf == NULL) {
        return -1;
    }
    if (synth_fill(recipe, buf, n) != 0) {
        free(buf);
        return -1;
    }
    ds_sha256(buf, n, digest);
    to_hex(digest, DS_SHA256_BYTES, hex);
    free(buf);
    return 0;
}

/*
 * kind content_id: boot the request (an accepted one) over the fixture and require that the content id stored by ds_boot, the
 * SHA-256 the core computes over a FRESH copy of the recipe bytes, and the content.sha256 line of the printed record are one and
 * the same value, 64 lowercase hex characters, and that it is the digest of the bytes as supplied (a single flipped bit gives
 * another digest). This is a self-consistency case: the independent comparison with a tool that shares nothing with the core is
 * the sweep in scripts/conformance-run (D-59). A matching hash shows repeatability, not that the content is genuine.
 */
void kind_content_id(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    struct recrow rows[REC_MAX_ROWS];
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const struct recrow *row;
    unsigned char *fresh;
    unsigned char digest[DS_SHA256_BYTES];
    unsigned char flipped[DS_SHA256_BYTES];
    char want[2u * DS_SHA256_BYTES + 1u];
    char have[2u * DS_SHA256_BYTES + 1u];
    size_t len = 0u;
    size_t nrows = 0u;
    size_t i;
    const char *err;

    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    fresh = (unsigned char *)malloc(s.len != 0u ? s.len : 1u);
    if (fresh == NULL || synth_fill(k->val[K_FIXTURE], fresh, s.len) != 0) {
        free(fresh);
        release_boot(&b);
        harness_error(c, "cannot regenerate the recipe bytes");
        return;
    }
    ds_sha256(fresh, s.len, digest);
    to_hex(digest, DS_SHA256_BYTES, want);
    to_hex(inst->content_id, DS_SHA256_BYTES, have);
    check(c, strcmp(want, have) == 0, "the content id stored by ds_boot differs from the SHA-256 of the supplied bytes");
    check(c, is_hex64(have), "the content id is not 64 lowercase hex characters");
    for (i = 0u; i < DS_SHA256_BYTES && inst->content_id[i] == 0u; i++) {
        /* find the first non-zero byte */
    }
    check(c, i < DS_SHA256_BYTES, "the content id is all zero: the placeholder slot was never filled");
    if (s.len != 0u) {
        fresh[s.len / 2u] = (unsigned char)(fresh[s.len / 2u] ^ 1u);
        ds_sha256(fresh, s.len, flipped);
        check(c, memcmp(flipped, digest, DS_SHA256_BYTES) != 0, "a flipped bit of the content leaves the digest unchanged");
    }
    snprintf(got, gotcap, "%.16s", have);

    check(c, ds_text_record(inst, text, sizeof text, &len) == DS_TEXT_OK, "ds_text_record failed on an accepted instance");
    if (len != 0u) {
        err = parse_record(text, rows, &nrows);
        check(c, err == NULL, err != NULL ? err : "layout");
        row = find_row(rows, nrows, "content.sha256");
        check(c, row != NULL && strlen(row->val) == 66u && row->val[0] == '"' && row->val[65] == '"' && strncmp(row->val + 1, want, 64u) == 0,
              "the content.sha256 line of the record differs from the content id");
    }
    free(fresh);
    release_boot(&b);
}

/*
 * kind hash_literal: the request names a NIST FIPS 180-4 example input that is built here, in memory (no file is read), and the
 * expect is the published digest as 64 lowercase hex characters. The input is fed to the core's update in uneven chunks of 1 to 7
 * bytes (so the internal buffering is exercised at every offset) and again in one call; both digests must equal the literal.
 * Inputs: empty, abc, 448bit (the 56-byte message that forces a second padding block), million-a (one million letters a).
 */
void kind_hash_literal(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char msg_448[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const char *name = k->val[K_REQUEST];
    const char *literal = k->val[K_EXPECT];
    unsigned char *buf = NULL;
    size_t n = 0u;
    size_t off = 0u;
    size_t step = 1u;
    struct ds_sha256 st;
    unsigned char digest[DS_SHA256_BYTES];
    char hex[2u * DS_SHA256_BYTES + 1u];

    if (strcmp(k->val[K_FIXTURE], "none") != 0) {
        harness_error(c, "a hash_literal case has no content fixture: its fixture must be none");
        return;
    }
    if (!is_hex64(literal)) {
        harness_error(c, "the expect of a hash_literal case is not 64 lowercase hex characters");
        return;
    }
    if (strcmp(name, "empty") == 0) {
        n = 0u;
    } else if (strcmp(name, "abc") == 0) {
        n = 3u;
    } else if (strcmp(name, "448bit") == 0) {
        n = sizeof msg_448 - 1u;
    } else if (strcmp(name, "million-a") == 0) {
        n = 1000000u;
    } else {
        harness_error(c, "unknown hash_literal input");
        return;
    }
    buf = (unsigned char *)malloc(n != 0u ? n : 1u);
    if (buf == NULL) {
        harness_error(c, "out of memory for the hash input");
        return;
    }
    if (strcmp(name, "abc") == 0) {
        memcpy(buf, "abc", n);
    } else if (strcmp(name, "448bit") == 0) {
        memcpy(buf, msg_448, n);
    } else if (n != 0u) {
        memset(buf, 'a', n);
    }
    check(c, strcmp(name, "448bit") != 0 || n * 8u == 448u, "the 448bit input is not 448 bits long");
    check(c, strcmp(name, "million-a") != 0 || n == 1000000u, "the million-a input is not one million bytes long");

    ds_sha256_init(&st);
    while (off < n) {
        size_t take = n - off < step ? n - off : step;
        ds_sha256_update(&st, buf + off, take);
        off += take;
        step = step % 7u + 1u;
    }
    ds_sha256_final(&st, digest);
    to_hex(digest, DS_SHA256_BYTES, hex);
    check(c, strcmp(hex, literal) == 0, "the digest fed in uneven chunks differs from the NIST literal");
    snprintf(got, gotcap, "%.16s", hex);
    ds_sha256(buf, n, digest);
    to_hex(digest, DS_SHA256_BYTES, hex);
    check(c, strcmp(hex, literal) == 0, "the one-call digest differs from the NIST literal");
    free(buf);
}
