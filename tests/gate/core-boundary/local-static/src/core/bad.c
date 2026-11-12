/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: a non-const function-local static, which is a hidden global. */
unsigned bad_bump(void)
{
    static unsigned n;
    n = n + 1u;
    return n;
}
