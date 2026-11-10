/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: the compile-time stamp macro makes the object differ between builds. */
const char *bad_stamp(void)
{
    return __TIME__;
}
