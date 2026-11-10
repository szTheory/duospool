/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: limits.h resolves on clang only, and is banned everywhere by the include rule. */
#include <limits.h>

int bad_limit(void)
{
    return 1;
}
