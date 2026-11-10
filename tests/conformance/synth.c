/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * synth.c - synthetic image recipes (D-59). Integer LCG only, no floating point and no clock: a recipe gives the same bytes in
 * every process on every host.
 *   plain-256k  262144 bytes, the LCG stream  x = x * 1664525 + 1013904223 mod 2^32  from x = 12345, byte = (x >> 16) and 255.
 * The LCG fill is the whole recipe for now; plan 08 adds the unique tag byte at the start and the end of every 8 KiB bank.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "synth.h"

#define PLAIN_256K_BYTES 262144u

size_t synth_size(const char *recipe)
{
    if (strcmp(recipe, "plain-256k") == 0) {
        return PLAIN_256K_BYTES;
    }
    return 0u;
}

int synth_fill(const char *recipe, unsigned char *buf, size_t n)
{
    uint32_t x = 12345u;
    size_t i;

    if (synth_size(recipe) == 0u) {
        return -1;
    }
    for (i = 0u; i < n; i++) {
        x = x * 1664525u + 1013904223u;
        buf[i] = (unsigned char)((x >> 16) & 255u);
    }
    return 0;
}
