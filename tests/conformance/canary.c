/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * canary.c - planted defects that a live sanitizer must report (D-55, D-83). It is built only in the sanitize preset and run only by
 * scripts/conformance-run sanitize, which requires a non-zero exit and the sanitizer's own message for every mode:
 *     ds_canary heap      one byte past a small heap array         AddressSanitizer: heap-buffer-overflow
 *     ds_canary overflow  INT_MAX plus a runtime 1 in a signed int  UndefinedBehaviorSanitizer: runtime error, signed integer overflow
 *     ds_canary shift     1 shifted into the sign bit of an int     UndefinedBehaviorSanitizer: runtime error, left shift
 * The operands are volatile so that the optimizer cannot fold them and the compiler cannot warn about a constant defect. The
 * overflow mode can only fire because the sanitize preset carries no -fwrapv (D-83): with signed wrap defined the addition is legal.
 * Each mode prints its result afterwards, which a sanitizer that returned would reach: a dead sanitizer therefore exits 0 here.
 * Test code may allocate; the core may not. This file is never linked with the core.
 * exit : 0 the defect ran unreported (the sanitizer is dead) | 2 usage | non-zero from the sanitizer runtime otherwise
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int canary_heap(void)
{
    unsigned char *p = (unsigned char *)malloc(8u);
    volatile size_t i = 8u; /* one byte past the allocation */
    int r;

    if (p == NULL) {
        return 2;
    }
    memset(p, 0, 8u);
    p[i] = 1u;
    r = (int)p[i];
    free(p);
    printf("canary heap ran unreported value=%d\n", r);
    return 0;
}

static int canary_overflow(void)
{
    volatile int a = INT_MAX;
    volatile int b = 1;
    int r = a + b;

    printf("canary overflow ran unreported value=%d\n", r);
    return 0;
}

static int canary_shift(void)
{
    volatile int v = 1;
    volatile int s = 31;
    int r = v << s;

    printf("canary shift ran unreported value=%d\n", r);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "heap") == 0) {
        return canary_heap();
    }
    if (argc == 2 && strcmp(argv[1], "overflow") == 0) {
        return canary_overflow();
    }
    if (argc == 2 && strcmp(argv[1], "shift") == 0) {
        return canary_shift();
    }
    fprintf(stderr, "usage: ds_canary heap|overflow|shift\n");
    return 2;
}
