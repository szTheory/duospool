/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * synth.c - synthetic image recipes (D-59). Integer LCG only, no floating point and no clock: a recipe gives the same bytes in
 * every process on every host. The generator is the one named by SYNTH_SPEC, and that string is part of every recipe: a change
 * of seed, multiplier, increment or byte selection is a change of recipe, so the fixture manifest hashes the spec with the bytes.
 *   plain-256k  262144 bytes of the LCG stream, then the bank tags below.
 *   len:N       N bytes of the same stream, N written in decimal without a leading zero (len:0 is the empty image); used by the
 *               hash sweep, whose lengths straddle the padding boundaries of SHA-256.
 * Bank tags (plain-* recipes only; len:N is the pure stream): after the LCG fill, the first byte of file bank k (the 8 KiB block
 * k of the image) is overwritten with (k*37+11) mod 256 and its last byte with (k*91+5) mod 256, so every file bank of every
 * supported size can be told from its neighbours. tests/conformance/mirror-oracle.awk carries its own copy of this formula: the
 * expected bank signatures never come from this file or from the core.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "synth.h"

#define PLAIN_256K_BYTES 262144u
#define BANK_BYTES 8192u
#define LEN_RECIPE_MAX 16777216u /* a len:N recipe above 16 MiB is refused, so --emit cannot be asked for an unbounded stream */

const char *synth_spec(void)
{
    return "lcg32 seed=12345 x=x*1664525+1013904223 mod 2^32 byte=(x>>16)&255";
}

int synth_parse(const char *recipe, size_t *n)
{
    size_t v = 0u;
    size_t i;

    if (strcmp(recipe, "plain-256k") == 0) {
        *n = PLAIN_256K_BYTES;
        return 0;
    }
    if (strncmp(recipe, "len:", 4u) != 0 || recipe[4] == '\0') {
        return -1;
    }
    if (recipe[4] == '0' && recipe[5] != '\0') {
        return -1; /* no leading zero: one spelling per length */
    }
    for (i = 4u; recipe[i] != '\0'; i++) {
        if (recipe[i] < '0' || recipe[i] > '9') {
            return -1;
        }
        v = v * 10u + (size_t)(recipe[i] - '0');
        if (v > LEN_RECIPE_MAX) {
            return -1;
        }
    }
    *n = v;
    return 0;
}

size_t synth_size(const char *recipe)
{
    size_t n = 0u;

    return synth_parse(recipe, &n) == 0 ? n : 0u;
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
    size_t natural;
    size_t i;

    if (synth_parse(recipe, &natural) != 0) {
        return -1;
    }
    for (i = 0u; i < n; i++) {
        x = x * 1664525u + 1013904223u;
        buf[i] = (unsigned char)((x >> 16) & 255u);
    }
    if (strncmp(recipe, "plain-", 6u) == 0) {
        tag_banks(buf, n, natural);
    }
    return 0;
}
