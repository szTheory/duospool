/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * allocshim.c - the allocation counter of the gate presets (D-42, D-46). It defines malloc, calloc, realloc and free in the runner
 * executable, so that every call made by an object linked into it (the core, the text printer, the runner) reaches these functions
 * and is counted, then forwards to the system allocator. ds_shim_calls() returns the number of allocating calls so far: the runner
 * reads it before and after a ds_boot call, and the difference must be zero for a refusal (kind alloc_zero). Calls made inside the
 * system library (printf and fopen buffers) bind to the library's own allocator on macOS and are not counted; the runner only reads
 * the counter around ds_boot, where nothing but core code runs.
 *
 * It is linked only when DS_ALLOC_SHIM is ON (the gate presets). The sanitizer runtime owns the allocator in the sanitize preset, so
 * the shim cannot work there and is not built; the two alloc_zero cases are excluded from that mode only (tests/conformance/
 * EXCLUDED_IN_MODE, D-78). A dead shim cannot pass: the case alloc_shim_selfcheck requires one allocator call from the test to move
 * the counter by exactly one, and the matrix requires the same before it reads a single row.
 *
 * Forwarding is chosen by platform. macOS: the default malloc zone (tested). glibc: __libc_malloc and friends (UNTESTED here: Linux
 * interposition normally also redirects calls made by libc itself, which the counting window avoids by reading the counter only
 * around ds_boot). Any other platform: nothing is defined, the counter never moves and the self-check fails, which is the intended
 * fail-closed result. The nm check of a later plan (no allocator symbol in the core) is the primary proof of "the core never
 * allocates"; this shim is a second witness with a self-check of its own.
 * The counter is not atomic: the runner is single-threaded. Test code only, never installed, never linked with the sanitizers.
 */
#include <stddef.h>
#include <stdlib.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#define DS_SHIM_PLATFORM 1
#elif defined(__GLIBC__)
extern void *__libc_malloc(size_t n);
extern void *__libc_calloc(size_t a, size_t b);
extern void *__libc_realloc(void *p, size_t n);
extern void __libc_free(void *p);
#define DS_SHIM_PLATFORM 1
#else
#define DS_SHIM_PLATFORM 0
#endif

static unsigned long shim_calls;

unsigned long ds_shim_calls(void);

unsigned long ds_shim_calls(void)
{
    return shim_calls;
}

#if DS_SHIM_PLATFORM

void *malloc(size_t n)
{
    shim_calls++;
#if defined(__APPLE__)
    return malloc_zone_malloc(malloc_default_zone(), n);
#else
    return __libc_malloc(n);
#endif
}

void *calloc(size_t a, size_t b)
{
    shim_calls++;
#if defined(__APPLE__)
    return malloc_zone_calloc(malloc_default_zone(), a, b);
#else
    return __libc_calloc(a, b);
#endif
}

void *realloc(void *p, size_t n)
{
    shim_calls++;
#if defined(__APPLE__)
    if (p == NULL) {
        return malloc_zone_malloc(malloc_default_zone(), n);
    }
    return malloc_zone_realloc(malloc_zone_from_ptr(p), p, n);
#else
    return __libc_realloc(p, n);
#endif
}

void free(void *p)
{
    if (p == NULL) {
        return;
    }
#if defined(__APPLE__)
    {
        malloc_zone_t *zone = malloc_zone_from_ptr(p);
        if (zone != NULL) {
            malloc_zone_free(zone, p);
        }
    }
#else
    __libc_free(p);
#endif
}

#endif
