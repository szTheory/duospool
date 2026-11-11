/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control (WR-03): a floating constant written with an exponent only (no decimal point). */
#include <stdint.h>

uint32_t bad_scale(uint32_t n)
{
    return (uint32_t)(n * 1e3);
}
