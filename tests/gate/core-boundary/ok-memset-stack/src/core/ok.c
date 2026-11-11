/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Positive control: the memory builtins and a 20 KB stack array are allowed, and nothing here is writable data. */
#include <stddef.h>
#include <stdint.h>

uint8_t ok_fill(uint8_t value, size_t index)
{
    uint8_t buf[20480];
    uint8_t copy[16];
    __builtin_memset(buf, value, sizeof buf);
    __builtin_memcpy(copy, buf, sizeof copy);
    return (uint8_t)(copy[index & 15u] ^ buf[index % sizeof buf]);
}
