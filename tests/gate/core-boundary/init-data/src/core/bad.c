/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control: an initialised non-const table lives in writable data. */
unsigned bad_table[4] = { 1u, 2u, 3u, 4u };

unsigned bad_get(unsigned i)
{
    return bad_table[i & 3u];
}
