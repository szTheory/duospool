/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: floating point is banned from machine behaviour. */
float bad_twice(float x)
{
    return x + x;
}
