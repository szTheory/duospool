/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: the time header is not on the include allowlist. */
#include <time.h>

int bad_now(void)
{
    return 1;
}
