/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Positive control (WR-03): integer constants and names that contain an e, a p or a point but are not floating constants. */
#include <stdint.h>

struct ok_pair { uint32_t a; uint32_t b1e3; };
static const struct ok_pair ok_pairs[2] = { { 1u, 2u }, { 0x1e3u, 0xE1u } };

uint32_t ok_mix(uint32_t i)
{
    uint32_t x1e3 = 0xFFFFu + 100UL + 07u + 0x1eu;  /* 1.5 and 2e9 and 0x1p3 are only comment text */
    const char *s = "1.5 2e9 0x1p3";
    return ok_pairs[i & 1u].b1e3 + x1e3 + ok_pairs[0].a + (uint32_t)s[0] + (uint32_t)(i - 1u) + (uint32_t)('e');
}
