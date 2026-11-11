/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * synth.c - synthetic image recipes (D-59). Integer LCG only, no floating point and no clock: a recipe gives the same bytes in
 * every process on every host. The generator is the one named by SYNTH_SPEC, and that string is part of every recipe: a change
 * of seed, multiplier, increment or byte selection is a change of recipe, so the fixture manifest hashes the spec with the bytes.
 *   plain-256k  262144 bytes of the LCG stream (32 file banks), then the bank tags below.
 *   plain-384k  393216 bytes (48 file banks) and plain-1m 1048576 bytes (128 file banks): the same stream and the same tags.
 *   len:N       N bytes of the same stream, N written in decimal without a leading zero (len:0 is the empty image); used by the
 *               hash sweep, whose lengths straddle the padding boundaries of SHA-256.
 *   plain-*:N   the first N bytes of a plain-* image (N above its natural size continues the stream; the bank tags stay those of the
 *               natural size). This is what a case with a len=N override materialises (plan 17): synth_name gives the recipe name of
 *               what a case emits, so the fixture manifest has one entry for every byte string the gate generates.
 * Bank tags (plain-* recipes only; len:N is the pure stream): after the LCG fill, the first byte of file bank k (the 8 KiB block
 * k of the image) is overwritten with (k*37+11) mod 256 and its last byte with (k*91+5) mod 256, so every file bank of every
 * supported size can be told from its neighbours. tests/conformance/mirror-oracle.awk carries its own copy of this formula: the
 * expected bank signatures never come from this file or from the core.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "synth.h"

/* the plain-* recipes: name and natural size in bytes */
static const struct {
    const char *name;
    size_t bytes;
} plain_recipe[] = {
    {"plain-256k", 262144u},
    {"plain-384k", 393216u},
    {"plain-1m", 1048576u},
};

#define BANK_BYTES 8192u
#define LEN_RECIPE_MAX 16777216u /* a len:N recipe above 16 MiB is refused, so --emit cannot be asked for an unbounded stream */

const char *synth_spec(void)
{
    return "lcg32 seed=12345 x=x*1664525+1013904223 mod 2^32 byte=(x>>16)&255";
}

/* the index in plain_recipe of the base name of a recipe (the part before a colon, or the whole name), -1 when it is not a plain-* name */
static int plain_base(const char *recipe)
{
    size_t len = strcspn(recipe, ":");
    size_t i;

    for (i = 0u; i < sizeof plain_recipe / sizeof plain_recipe[0]; i++) {
        if (strlen(plain_recipe[i].name) == len && strncmp(plain_recipe[i].name, recipe, len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* a decimal length without a leading zero (one spelling per length) and not above LEN_RECIPE_MAX; 0 on success */
static int parse_dec(const char *s, size_t *out)
{
    size_t v = 0u;
    size_t i;

    if (s[0] == '\0' || (s[0] == '0' && s[1] != '\0')) {
        return -1;
    }
    for (i = 0u; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return -1;
        }
        v = v * 10u + (size_t)(s[i] - '0');
        if (v > LEN_RECIPE_MAX) {
            return -1;
        }
    }
    *out = v;
    return 0;
}

int synth_parse(const char *recipe, size_t *n)
{
    int b = plain_base(recipe);

    if (b >= 0) {
        const char *rest = recipe + strlen(plain_recipe[b].name);

        if (*rest == '\0') {
            *n = plain_recipe[b].bytes;
            return 0;
        }
        /* BASE:N, the first N bytes of a plain-* image (N above the natural size continues the stream); N equal to the natural size
         * is the base name itself, so every byte string has one spelling */
        if (parse_dec(rest + 1, n) != 0 || *n == plain_recipe[b].bytes) {
            return -1;
        }
        return 0;
    }
    if (strncmp(recipe, "len:", 4u) != 0) {
        return -1;
    }
    return parse_dec(recipe + 4, n);
}

size_t synth_size(const char *recipe)
{
    size_t n = 0u;

    return synth_parse(recipe, &n) == 0 ? n : 0u;
}

int synth_name(const char *fixture, size_t len, char *out, size_t cap)
{
    size_t natural;
    int b = plain_base(fixture);
    int w;

    if (synth_parse(fixture, &natural) != 0) {
        return -1;
    }
    if (natural == len) {
        w = snprintf(out, cap, "%s", fixture);
    } else if (b >= 0 && plain_recipe[b].bytes == len) {
        w = snprintf(out, cap, "%s", plain_recipe[b].name);
    } else if (b >= 0) {
        w = snprintf(out, cap, "%s:%lu", plain_recipe[b].name, (unsigned long)len);
    } else {
        w = snprintf(out, cap, "len:%lu", (unsigned long)len);
    }
    return (w < 0 || (size_t)w >= cap) ? -1 : 0;
}

/* the tag bytes of a plain-* image: only bytes that lie inside the n requested are written, only banks inside the natural size */
static void tag_banks(unsigned char *buf, size_t n, size_t natural)
{
    size_t k;

    for (k = 0u; k < natural / BANK_BYTES; k++) {
        size_t first = k * BANK_BYTES;
        size_t last = first + BANK_BYTES - 1u;

        if (first < n) {
            buf[first] = (unsigned char)((k * 37u + 11u) % 256u);
        }
        if (last < n) {
            buf[last] = (unsigned char)((k * 91u + 5u) % 256u);
        }
    }
}

int synth_fill(const char *recipe, unsigned char *buf, size_t n)
{
    uint32_t x = 12345u;
    size_t parsed;
    int b = plain_base(recipe);
    size_t i;

    if (synth_parse(recipe, &parsed) != 0) {
        return -1;
    }
    for (i = 0u; i < n; i++) {
        x = x * 1664525u + 1013904223u;
        buf[i] = (unsigned char)((x >> 16) & 255u);
    }
    if (b >= 0) {
        tag_banks(buf, n, plain_recipe[b].bytes); /* the tags follow the base image, not the requested length */
    }
    return 0;
}
