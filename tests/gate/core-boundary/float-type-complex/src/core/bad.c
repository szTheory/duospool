/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control (WR-03): _Complex without a float or double token. The declaration is in a skipped block (see float-type-sized). */
#include <stdint.h>

#if 0
_Complex int bad_never_compiled;
#endif
uint32_t ok_zero(void)
{
    return 0u;
}
