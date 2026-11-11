/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: an allocator call, with its own prototype. */
#include <stddef.h>

void *malloc(size_t size);

void *bad_alloc(void)
{
    return malloc(16);
}
