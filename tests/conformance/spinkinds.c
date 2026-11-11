/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * spinkinds.c - the two case kinds that give the gate live producers of the outcomes timeout and cancelled (plan 18; D-57, D-58, D-77).
 * Neither kind is in the real inventory: they exist only in the control mini-roots tests/gate/timeout-budget and tests/gate/cancel-spin,
 * which scripts/conformance-run control and control-signal run through this very runner. Test code only, never installed.
 *   budget_spin   boots the request over the fixture, then calls ds_run with a stop function that never fires. expect is "budget=N,step=M".
 *                 The budget is machine time, never the host clock, so the outcome is deterministic: when ds_run reports that the budget
 *                 was consumed (DS_RUN_BUDGET, what D-57 calls timeout) the case ends with the outcome timeout. Machine time after the
 *                 run must equal the start plus the budget (an assertion; a wrong value is a guest_failure, which wins over the timeout).
 *   spin_forever  boots the request, then steps ds_advance(1) without end and never returns, so no RESULT line is ever printed for it.
 *                 The supervisor (conformance-run supervise) is signalled with TERM, kills this process and writes the cancelled line.
 *                 The host wall watchdog of the supervisor is the only other way out.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

static int never_stop(const struct ds_instance *inst, void *ctx)
{
    (void)inst;
    (void)ctx;
    return 0;
}

/* read "budget=N,step=M" into the two numbers; 0 on success */
static int parse_budget(const char *text, uint64_t *budget, uint64_t *step)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *kv;
    int have_budget = 0;
    int have_step = 0;

    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((kv = pk_next_item(&rest, ',')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(kv, "budget") == 0 && parse_u64(eq + 1, budget) == 0) {
            have_budget = 1;
        } else if (strcmp(kv, "step") == 0 && parse_u64(eq + 1, step) == 0) {
            have_step = 1;
        } else {
            return -1;
        }
    }
    return have_budget && have_step ? 0 : -1;
}

void kind_spin(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    struct ds_instance *w;
    uint64_t budget = 0u;
    uint64_t step = 0u;
    uint64_t start;
    uint32_t status;
    const char *err;
    int forever = strcmp(k->val[K_KIND], "spin_forever") == 0;

    if (!forever && parse_budget(k->val[K_EXPECT], &budget, &step) != 0) {
        harness_error(c, "a budget_spin expect is not budget=N,step=M");
        return;
    }
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    if (forever) {
        for (;;) {
            (void)ds_advance(w, 1u);
        }
    }
    start = inst->machine_time;
    status = ds_run(w, budget, step, never_stop, NULL);
    check(c, status == (uint32_t)DS_RUN_BUDGET, "ds_run did not end on the exhausted budget");
    check(c, inst->machine_time == start + budget, "machine time after the exhausted budget is not the start plus the budget");
    if (status == (uint32_t)DS_RUN_BUDGET && !c->failed) {
        c->timeout = 1;
        snprintf(c->why, sizeof c->why, "the machine-time budget of %llu cycles was exhausted without a stop", (unsigned long long)budget);
    }
    snprintf(got, gotcap, "time=%llu", (unsigned long long)inst->machine_time);
    release_boot(&b);
}
