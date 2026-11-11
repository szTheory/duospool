/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Positive control: const tables, external, static and function-local, are read-only data and are not flagged on either platform. */
#include <stdint.h>

const uint8_t ok_tab[4] = { 1, 2, 3, 4 };
static const uint16_t ok_tab2[2] = { 100, 200 };
static const char *const ok_names[2] = { "a", "b" };

uint32_t ok_get(uint32_t i)
{
    static const uint8_t local[3] = { 7, 8, 9 };
    return (uint32_t)ok_tab[i & 3u] + ok_tab2[i & 1u] + local[i % 3u] + (uint32_t)ok_names[i & 1u][0];
}
