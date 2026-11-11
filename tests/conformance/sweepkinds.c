/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * sweepkinds.c - the mutation sweep of the authored fixtures (plan 03-10; D-126, D-95, D-60, ER-7). Test code only. Evidence class E0: the sweep shows that a check can flip, it says nothing
 * about what the check proves.
 *   mutate_sweep  loads the built image build/gate/fixtures/NAME.pce once, finds the DCK1 literal table of fixtures/lib/check.inc at file offset $1000 (magic "DCK1", the count, then one
 *                 entry {check number k, value} of two bytes per check, entry k at $1005 + 2 * (k - 1)), runs the unmutated image and requires the guest to pass, then for every check k that is not
 *                 excluded XORs the value byte of entry k with $FF in a fresh in-memory copy, boots that copy in the same process (no per-mutation assembly and no spawn, D-60, D-126) and requires
 *                 exactly check k to read $46 and every other check $50. A literal whose mutation does not flip its check is vacuous: the check cannot fail. The runs stop at the DSV1 done state
 *                 through the stop callback, a speed aid only (D-95): the case's until_mclk is the hard bound, and a guest that is not done at it is a failure of that mutation.
 *                 expect is id=N (the fixture id of the guest) with the items of runner.h: exclude=IDX,... lists the observe rows (a check whose expected value legitimately differs by case, written
 *                 to NAME.observe by scripts/fixture-build), and it must equal that file exactly: a sweep that skips more, or fewer, than the fixture declares is harness_failure (exclude-mismatch).
 *                 verdict=OUTCOME:REASON makes the case a control of the sweep itself (it passes only on that outcome and reason). One assertion per swept literal; the baseline adds one only on a fault.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

#define SW_TABLE_OFF 0x1000u /* the CHECKLIT segment: bank 0 of the image, logical $F000 (fixtures/lib/hucard.cfg) */
#define SW_ENTRY0 (SW_TABLE_OFF + 5u)

struct sweep_end {
    uint8_t block[V_SIZE];
    int done;
    uint64_t mclk;
    uint32_t reason;
};

/*
 * Boot a copy of the image (patch_at is the file offset whose byte is XORed with $FF, or len for no patch) over the request and the declared irq_stim of the case (plan 03-22) and run it to done or to the absolute deadline until.
 * NULL on success (end is filled), otherwise why not: a harness fault of the case.
 */
static const char *sweep_run(const unsigned char *image, size_t len, const struct kase *kase, size_t patch_at, uint64_t until, struct sweep_end *end)
{
    struct booted b;
    const struct ds_instance *inst = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    uint32_t refusal = 0u;
    const char *err;

    b.mem = NULL;
    b.content = (unsigned char *)malloc(len);
    if (b.content == NULL) {
        return "out of memory for the image copy";
    }
    memcpy(b.content, image, len);
    if (patch_at < len) {
        b.content[patch_at] = (unsigned char)(b.content[patch_at] ^ 0xFFu);
    }
    err = boot_content(kase->val[K_REQUEST], len, &b, &inst);
    if (err != NULL) {
        return err;
    }
    err = apply_irq_stim((struct ds_instance *)(void *)b.mem, kase, &refusal); /* the case's declared stimulus, before the first step: every mutated run sees the one the unmutated run saw (plan 03-22, NW3) */
    if (err == NULL && refusal != 0u) {
        err = "the irq_stim list of the case was refused by ds_irq_stim_set";
    }
    if (err != NULL) {
        release_boot(&b);
        return err;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.stop = dsv1_done;
    end->reason = ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    end->mclk = inst->machine_time;
    end->done = dsv1_done(inst, NULL);
    memcpy(end->block, inst->ram8k + V_OFF, V_SIZE);
    release_boot(&b);
    if (end->reason != (uint32_t)DS_RUN_REASON_UNTIL && end->reason != (uint32_t)DS_RUN_REASON_CALLBACK) {
        return "ds_run refused the request of the sweep";
    }
    return NULL;
}

/* the exclude list of the case equals the observe rows of the built fixture, element by element (both are strictly ascending) */
static int exclude_matches(const struct guest_expect *e, const struct obs_list *ol)
{
    unsigned i;

    if (e->nex != ol->n) {
        return 0;
    }
    for (i = 0u; i < e->nex; i++) {
        if (e->ex[i] != ol->idx[i]) {
            return 0;
        }
    }
    return 1;
}

static int is_excluded(const struct guest_expect *e, unsigned k)
{
    unsigned i;

    for (i = 0u; i < e->nex; i++) {
        if (e->ex[i] == k) {
            return 1;
        }
    }
    return 0;
}

void kind_mutate_sweep(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    struct guest_expect ex;
    struct obs_list ol;
    struct ctx swept;
    struct ctx base;
    struct ctx *t;
    struct sweep_end end;
    unsigned char *image;
    size_t len = 0u;
    uint64_t until = 0u;
    uint64_t checks = 0u;
    unsigned n;
    unsigned mutated = 0u;
    const char *err;

    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0) {
        harness_error(c, "a mutate_sweep case names its ROM as fixture = authored:NAME");
        return;
    }
    if (!k->has[K_UNTIL_MCLK] || !k->has[K_CHECKS] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || parse_u64(k->val[K_CHECKS], &checks) != 0 || checks < 1u
        || checks > V_MAX_CHECKS) {
        harness_error(c, "a mutate_sweep case needs the integer keys until_mclk and checks (checks 1 to 96)");
        return;
    }
    if (parse_guest_expect(k->val[K_EXPECT], &ex) != 0 || ex.mode == MODE_POISON || ex.nobs != 0u) {
        harness_error(c, "a mutate_sweep expect is id=N, optionally with ;exclude=IDX,... and ;verdict=OUTCOME:REASON");
        return;
    }
    memset(&swept, 0, sizeof swept);
    t = ex.mode == MODE_VERDICT ? &swept : c;
    err = read_observe(rom, &ol);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (!exclude_matches(&ex, &ol)) {
        harness_error(t, "exclude-mismatch: the exclude list of the case differs from the observe rows of the fixture (NAME.observe)");
        if (ex.mode == MODE_VERDICT) {
            expect_verdict(c, &swept, &ex, got, gotcap);
        }
        return;
    }
    image = load_image(rom, &len);
    if (image == NULL) {
        harness_error(c, "the built fixture image is missing or unreadable: run scripts/fixture-build first");
        return;
    }
    if (len < SW_ENTRY0 + 2u * (size_t)checks || memcmp(image + SW_TABLE_OFF, "DCK1", 4u) != 0 || (uint64_t)image[SW_TABLE_OFF + 4u] != checks) {
        free(image);
        harness_error(c, "the image has no DCK1 literal table at file offset $1000 with a count equal to the checks key of the case");
        return;
    }
    for (n = 1u; n <= (unsigned)checks; n++) {
        if (image[SW_ENTRY0 + 2u * (n - 1u)] != (unsigned char)n) {
            free(image);
            harness_error(c, "an entry of the DCK1 literal table does not carry its own check number");
            return;
        }
    }
    /* the baseline: the unmutated image must run to done and pass, or a later flip could come from a broken guest */
    err = sweep_run(image, len, k, len, until, &end);
    if (err != NULL) {
        free(image);
        harness_error(c, err);
        return;
    }
    note_run(c, end.mclk, end.reason);
    memset(&base, 0, sizeof base);
    {
        char seen[64];

        judge_dsv1(&base, end.block, (unsigned)ex.id, (unsigned)checks, seen, sizeof seen);
    }
    if (base.harness) {
        free(image);
        harness_error(t, base.why);
        if (ex.mode == MODE_VERDICT) {
            expect_verdict(c, &swept, &ex, got, gotcap);
        }
        return;
    }
    if (base.failed || base.timeout || base.n == 0u) {
        free(image);
        check(t, 0, base.why[0] != '\0' ? base.why : "the unmutated baseline run of the sweep did not pass");
        if (ex.mode == MODE_VERDICT) {
            expect_verdict(c, &swept, &ex, got, gotcap);
        }
        return;
    }
    for (n = 1u; n <= (unsigned)checks; n++) {
        unsigned i;
        char msg[160];
        int flipped;
        int others = 1;
        unsigned disturbed = 0u;

        if (is_excluded(&ex, n)) {
            continue;
        }
        err = sweep_run(image, len, k, SW_ENTRY0 + 2u * (n - 1u) + 1u, until, &end);
        if (err != NULL) {
            harness_error(t, err);
            break;
        }
        mutated++;
        if (!end.done) {
            snprintf(msg, sizeof msg, "the guest did not reach the done state with the literal of check %u mutated", n);
            check(t, 0, msg);
            continue;
        }
        flipped = end.block[V_RES + n - 1u] == R_FAIL;
        for (i = 1u; i <= (unsigned)checks; i++) {
            if (i != n && end.block[V_RES + i - 1u] != R_PASS) {
                others = 0;
                disturbed = i;
            }
        }
        if (!flipped) {
            snprintf(msg, sizeof msg, "mutating the literal of check %u did not flip check %u (result byte $%02x): the check is vacuous", n, n,
                     (unsigned)end.block[V_RES + n - 1u]);
        } else {
            snprintf(msg, sizeof msg, "mutating the literal of check %u also changed check %u (result byte $%02x)", n, disturbed,
                     (unsigned)end.block[V_RES + (disturbed != 0u ? disturbed : 1u) - 1u]);
        }
        check(t, flipped && others, msg);
    }
    free(image);
    snprintf(got, gotcap, "mutations=%u excluded=%u", mutated, ex.nex);
    if (ex.mode == MODE_VERDICT) {
        expect_verdict(c, &swept, &ex, got, gotcap);
    }
}
