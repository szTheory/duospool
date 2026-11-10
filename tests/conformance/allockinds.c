/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * allockinds.c - the case kind alloc_zero (D-42, D-46), the allocation witness of the gate presets. It reads the counter of
 * tests/conformance/allocshim.c, which is linked when DS_ALLOC_SHIM is 1. In a build without the shim (DS_ALLOC_SHIM 0, the sanitize
 * preset) the counter below never moves, so both forms fail there as guest_failure and scripts/conformance-run excludes the two cases
 * from that mode only, through tests/conformance/EXCLUDED_IN_MODE (D-78). The request of the case names the form:
 *   selfcheck  the test calls the allocator once and the counter must read exactly 1, so a dead shim cannot pass
 *   matrix     every refuse-kind row of the cases TSV is booted again with fresh test-allocated memory (boot_row in runner.c) and the
 *              counter read around ds_boot must be 0. One assertion per row, so the assertion count is the number of rows run; the
 *              shim must be live first (that probe is not an assertion: a dead shim is one failed assertion and no row runs)
 * The allocator is called through a volatile pointer so that the compiler cannot drop the probe pair. Test code only.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runner.h"

#if !DS_ALLOC_SHIM
unsigned long ds_shim_calls(void)
{
    return 0ul;
}
#endif

static void *(*volatile alloc_probe)(size_t) = malloc;

/* one allocator call from here; how far the counter moved */
static unsigned long probe_moved(void)
{
    unsigned long before = ds_shim_calls();
    void *p = alloc_probe(16u);
    unsigned long moved = ds_shim_calls() - before;

    free(p);
    return moved;
}

void kind_alloc_zero(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    const char *form = k->val[K_REQUEST];

    if (strcmp(form, "selfcheck") == 0) {
        unsigned long moved = probe_moved();

        snprintf(got, gotcap, "calls=%lu", moved);
        check(c, moved == 1ul, "one allocator call from the test did not move the counter by exactly one");
    } else if (strcmp(form, "matrix") == 0) {
        size_t i;
        unsigned rows = 0u;

        snprintf(got, gotcap, "rows=0");
        if (probe_moved() != 1ul) {
            check(c, 0, "the allocation shim is not live in this build");
            return;
        }
        for (i = 0u; i < case_count(); i++) {
            const struct kase *r = case_get(i);
            unsigned long allocs = 0ul;

            if (!r->has[K_KIND] || strcmp(r->val[K_KIND], "refuse") != 0) {
                continue;
            }
            boot_row(r, c, &allocs);
            if (c->harness) {
                return;
            }
            check(c, allocs == 0ul, "ds_boot allocated on a refuse row");
            if (allocs != 0ul) {
                emit_err("alloc_zero_matrix: row %s made %lu allocator calls\n", r->stem, allocs);
            }
            rows++;
        }
        snprintf(got, gotcap, "rows=%u", rows);
        if (rows == 0u) {
            check(c, 0, "the cases hold no refuse row");
        }
    } else {
        harness_error(c, "alloc_zero request is neither selfcheck nor matrix");
    }
}
