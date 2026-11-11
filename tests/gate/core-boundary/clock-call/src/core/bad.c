/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: a host clock call. The prototype is declared here so that only the undefined-symbol rule can catch it. */
#include <stddef.h>

long clock(void);

long bad_clock(void)
{
    return clock();
}
