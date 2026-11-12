/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: a host environment call, with its own prototype. */
#include <stddef.h>

char *getenv(const char *name);

char *bad_env(void)
{
    return getenv("HOME");
}
