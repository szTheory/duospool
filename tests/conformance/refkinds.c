/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * refkinds.c - the case kinds of the refusal matrix (plan 11; D-43, D-46, D-82, D-86). They aggregate over the refuse rows or over the
 * check order, so none of them is one boot of one request:
 *   precedence   a table of combined-fault requests, each written REQ>>CODE (REQ a request-override string, CODE the refusal the
 *                EARLIER fault must give); items are separated by a vertical bar and are read from the request key and then from the
 *                expect key, because a case line is limited to 400 characters (scripts/lib/toml-subset.awk). The table must hold at
 *                least one item for every adjacent pair of rows of boot_order.def, so a check added to the list forces a pair.
 *   coverage     DEFERRED (D-86): evaluated in the second pass, after every refuse row has run. The refuse handler records each reason
 *                it observed (observe_name), and the set must equal every enum member except OK, whatever position the row has.
 *   nonmutation  boots a live instance in one block, then boots every refuse row of the cases TSV into a fresh block that is filled
 *                with a fixed byte pattern and carries guard canaries on both sides; the block, the guards and the live block must be
 *                byte-identical afterwards. One assertion per refuse row plus one for the live block.
 * Test code only, never installed. Nothing here prints bytes found inside content; only ids, counts and reason names.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"
#include "synth.h"

/* ---- the set of reasons the refuse rows observed (written by kind_boot, read by kind coverage) ---- */

static unsigned char observed[DS_REFUSAL_SLOTS];

void observe_name(const char *reason_name)
{
    uint32_t n;

    for (n = 1u; n < (uint32_t)DS_REFUSAL_SLOTS; n++) {
        const char *name = ds_refusal_name(n);

        if (name != NULL && strcmp(name, reason_name) == 0) {
            observed[n] = 1u;
        }
    }
}

void kind_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    uint32_t n;
    unsigned members = 0u;
    unsigned seen = 0u;

    (void)k;
    for (n = 1u; n < (uint32_t)DS_REFUSAL_SLOTS; n++) {
        const char *name = ds_refusal_name(n);

        if (name == NULL) {
            continue;
        }
        members++;
        seen += observed[n] != 0u ? 1u : 0u;
        check(c, observed[n] != 0u, "a refusal enum member was never observed by a refuse row");
        if (observed[n] == 0u) {
            fprintf(stderr, "coverage: %s was not observed by any refuse row\n", name);
        }
    }
    check(c, members > 0u, "the refusal enum has no member");
    snprintf(got, gotcap, "members=%u observed=%u", members, seen);
}

/* ---- kind precedence ---- */

/* the number of rows of boot_order.def: the table needs one item per adjacent pair */
#define DS_CHECK(name) +1
enum { BOOT_ROWS = 0
#include "boot_order.def"
};
#undef DS_CHECK

/* run the items of one list (a vertical-bar separated string, read in place); *item counts across lists; *bad keeps the first failed item */
static void run_items(const struct kase *k, char *list, struct ctx *c, unsigned *item, unsigned *bad)
{
    const char *fixture = k->val[K_FIXTURE];
    size_t natural = synth_size(fixture);
    char *p = list;

    while (*p != '\0' && !c->harness) {
        char *end = strchr(p, '|');
        char *arrow;
        struct spec s;
        struct expect e;
        char got[64];
        const char *err;
        int failed_before = c->failed;

        if (end != NULL) {
            *end = '\0';
        }
        arrow = strstr(p, ">>");
        if (arrow == NULL) {
            harness_error(c, "a precedence item has no >>");
            return;
        }
        *arrow = '\0';
        err = parse_spec(&s, p, natural);
        if (err == NULL) {
            err = parse_expect(&e, arrow + 2);
        }
        if (err == NULL && e.accept) {
            err = "a precedence item expects a refusal, not accept";
        }
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        (*item)++;
        run_boot(fixture, &s, &e, c, got, sizeof got);
        if (c->failed && !failed_before && *bad == 0u) {
            *bad = *item;
        }
        if (end == NULL) {
            break;
        }
        p = end + 1;
    }
}

void kind_precedence(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char request[512];
    char expect[512];
    unsigned item = 0u;
    unsigned bad = 0u;
    const char *src[2];
    char *dst[2];
    int i;

    if (synth_size(k->val[K_FIXTURE]) == 0u) {
        harness_error(c, "unknown fixture recipe");
        return;
    }
    src[0] = k->val[K_REQUEST];
    src[1] = k->val[K_EXPECT];
    dst[0] = request;
    dst[1] = expect;
    for (i = 0; i < 2; i++) {
        if (strlen(src[i]) >= sizeof request) {
            harness_error(c, "a precedence list is too long");
            return;
        }
        memcpy(dst[i], src[i], strlen(src[i]) + 1u);
    }
    run_items(k, request, c, &item, &bad);
    run_items(k, expect, c, &item, &bad);
    if (c->harness) {
        return;
    }
    check(c, item + 1u >= (unsigned)BOOT_ROWS, "the table has fewer items than adjacent pairs of rows in boot_order.def");
    if (bad != 0u) {
        snprintf(got, gotcap, "failed_item=%u", bad);
    } else {
        snprintf(got, gotcap, "items=%u rows=%u", item, (unsigned)BOOT_ROWS);
    }
}

/* ---- kind nonmutation ---- */

#define NM_GUARD 64u
#define NM_SLACK 64u
#define NM_PRE 0xc3u
#define NM_POST 0x3cu

/* a fixed, non-uniform pattern: a write of any single value into the block changes at least some bytes of it */
static unsigned char nm_pattern(size_t i)
{
    return (unsigned char)((i * 131u + 7u) & 0xffu);
}

/*
 * Boot one refuse row into a patterned block between guards. 0 when the row cannot be set up (harness error in c); otherwise 1 and
 * *same tells whether the refusal was the expected one with the whole region byte-identical afterwards.
 */
static int nonmutation_row(const struct kase *r, struct ctx *c, int *same)
{
    static const unsigned char dummy[1] = {0u};
    struct spec s;
    struct expect e;
    struct ds_boot_request rq;
    struct ds_boot_result res;
    size_t isize = ds_instance_size();
    size_t total = NM_GUARD + isize + NM_SLACK + NM_GUARD;
    unsigned char *region;
    unsigned char *snap;
    unsigned char *content = NULL;
    const unsigned char *cptr = NULL;
    const char *err;
    size_t natural;
    size_t i;
    void *mem;
    size_t cap;
    uint32_t ret;

    if (r->bad != NULL || !r->has[K_FIXTURE] || !r->has[K_REQUEST] || !r->has[K_EXPECT]) {
        harness_error(c, "a refuse row is malformed");
        return 0;
    }
    natural = synth_size(r->val[K_FIXTURE]);
    err = natural == 0u ? "unknown fixture recipe" : parse_spec(&s, r->val[K_REQUEST], natural);
    if (err == NULL) {
        err = parse_expect(&e, r->val[K_EXPECT]);
    }
    if (err == NULL && s.ptr == PTR_OK && s.len > CONTENT_LIMIT) {
        err = "len above the content limit needs ptr=dummy or ptr=null";
    }
    if (err != NULL) {
        harness_error(c, err);
        return 0;
    }
    region = (unsigned char *)malloc(total);
    snap = (unsigned char *)malloc(total);
    if (s.ptr == PTR_OK) {
        content = (unsigned char *)malloc(s.len != 0u ? s.len : 1u);
    }
    if (region == NULL || snap == NULL || (s.ptr == PTR_OK && content == NULL) ||
        (content != NULL && synth_fill(r->val[K_FIXTURE], content, s.len) != 0)) {
        free(content);
        free(snap);
        free(region);
        harness_error(c, "out of memory or unknown recipe in the non-mutation matrix");
        return 0;
    }
    cptr = s.ptr == PTR_OK ? content : (s.ptr == PTR_DUMMY ? dummy : NULL);
    memset(region, (int)NM_PRE, NM_GUARD);
    for (i = 0u; i < isize + NM_SLACK; i++) {
        region[NM_GUARD + i] = nm_pattern(i);
    }
    memset(region + NM_GUARD + isize + NM_SLACK, (int)NM_POST, NM_GUARD);
    memcpy(snap, region, total);

    fill_request(&rq, &s, cptr);
    memset(&res, 0xff, sizeof res);
    mem = s.mem == MEM_NULL ? NULL : (s.mem == MEM_MISALIGNED ? (void *)(region + NM_GUARD + 1u) : (void *)(region + NM_GUARD));
    cap = s.cap == CAP_SHORT ? isize - 1u : isize;
    ret = ds_boot(mem, cap, s.req == REQ_NULL ? NULL : &rq, &res);
    *same = ret == e.reason && !e.accept && res.instance == NULL && memcmp(region, snap, total) == 0;
    free(content);
    free(snap);
    free(region);
    return 1;
}

void kind_nonmutation(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted live;
    struct spec s;
    const struct ds_instance *inst = NULL;
    unsigned char *snap;
    const char *err;
    size_t isize = ds_instance_size();
    size_t i;
    unsigned rows = 0u;

    (void)k;
    snprintf(got, gotcap, "rows=0");
    err = boot_accepted("plain-256k", "", &live, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    snap = (unsigned char *)malloc(isize);
    if (snap == NULL) {
        release_boot(&live);
        harness_error(c, "out of memory for the live block snapshot");
        return;
    }
    memcpy(snap, live.mem, isize);
    for (i = 0u; i < case_count(); i++) {
        const struct kase *r = case_get(i);
        int same = 0;

        if (!r->has[K_KIND] || strcmp(r->val[K_KIND], "refuse") != 0) {
            continue;
        }
        if (!nonmutation_row(r, c, &same)) {
            free(snap);
            release_boot(&live);
            return;
        }
        check(c, same, "a refuse row was not refused as expected, or it changed its block or a guard canary");
        if (!same) {
            fprintf(stderr, "nonmutation_matrix: row %s changed memory or was not refused as expected\n", r->stem);
        }
        rows++;
    }
    check(c, memcmp(live.mem, snap, isize) == 0, "the live instance block changed while refuse rows ran");
    snprintf(got, gotcap, "rows=%u", rows);
    if (rows == 0u) {
        check(c, 0, "the cases hold no refuse row");
    }
    free(snap);
    release_boot(&live);
}
