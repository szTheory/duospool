/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * spinkinds.c - the two case kinds that give the gate live producers of the outcomes timeout and cancelled (plan 18; D-57, D-58, D-77).
 * Neither kind is in the real inventory: they exist only in the control mini-roots tests/gate/timeout-budget and tests/gate/cancel-spin,
 * which scripts/conformance-run control and control-signal run through this very runner. Test code only, never installed.
 *   budget_spin   boots the request over the fixture, then calls ds_run with absolute deadlines start+M, start+2M, ... and finally start+N, with a stop
 *                 function that never fires (plan 03-02: the relative budget of Phase 2 is gone, D-91; the expect text keeps its meaning). expect is
 *                 "budget=N,step=M". The deadline is machine time in master clocks, never the host clock, so the outcome is deterministic: when
 *                 every call ends with UNTIL the case ends with the outcome timeout (D-57, D-95). Machine time after the run must be the first
 *                 instruction boundary at or past start+N, less than ds_max_step_mclk() past it (an assertion; a wrong value is a guest_failure,
 *                 which wins over the timeout).
 *   spin_forever  boots the request, then calls ds_run with ever growing absolute deadlines without end and never returns, so no RESULT line is ever printed for it.
 *                 The supervisor (conformance-run supervise) is signalled with TERM, kills this process and writes the cancelled line.
 *                 The host wall watchdog of the supervisor is the only other way out.
 * The guest kind that plan 03-01 first put here lives in guestkinds.c since plan 03-04.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

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

/* one ds_run to an absolute deadline with no stop function: true when it ended on the deadline */
static int run_deadline(struct ds_instance *inst, uint64_t until)
{
    struct ds_run_request rq;
    struct ds_run_result out;

    memset(&rq, 0, sizeof rq);
    memset(&out, 0, sizeof out);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    return ds_run(inst, &rq, &out) == (uint32_t)DS_RUN_REASON_UNTIL;
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
    uint64_t at;
    int ended = 1;
    const char *err;
    int forever = strcmp(k->val[K_KIND], "spin_forever") == 0;

    if (!forever && (parse_budget(k->val[K_EXPECT], &budget, &step) != 0 || step == 0u)) {
        harness_error(c, "a budget_spin expect is not budget=N,step=M with M above 0");
        return;
    }
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    if (forever) {
        uint64_t until = 0u;

        for (;;) {
            until += UINT64_C(1000);
            (void)run_deadline(w, until);
        }
    }
    start = inst->machine_time;
    at = start;
    while (at - start < budget && ended) {
        at = budget - (at - start) > step ? at + step : start + budget;
        ended = run_deadline(w, at);
    }
    check(c, ended, "ds_run did not end on an absolute deadline of the budget");
    check(c, inst->machine_time >= start + budget && inst->machine_time - (start + budget) < ds_max_step_mclk(),
          "machine time after the last deadline is not the first instruction boundary at or past the start plus the budget");
    if (ended && !c->failed) {
        c->timeout = 1;
        snprintf(c->why, sizeof c->why, "the machine-time deadline of %llu master clocks was reached without a stop", (unsigned long long)budget);
    }
    snprintf(got, gotcap, "time=%llu", (unsigned long long)inst->machine_time);
    release_boot(&b);
}
