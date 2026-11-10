/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: a writable global shared by every instance. */
unsigned bad_counter;

unsigned bad_next(void)
{
    bad_counter = bad_counter + 1u;
    return bad_counter;
}
