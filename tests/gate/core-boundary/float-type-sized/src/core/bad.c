/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/* Negative control (WR-03): a sized floating type name. The declaration is in a skipped block so that every compiler accepts the file: the
   scanner is lexical and does not evaluate the preprocessor. */
#include <stdint.h>

#if 0
_Float32 bad_never_compiled;
#endif
uint32_t ok_zero(void)
{
    return 0u;
}
