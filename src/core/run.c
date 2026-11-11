/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * run.c - ds_run, the bounded run of the machine in whole instructions to an absolute deadline in master clocks (plan 03-01; D-90, D-92,
 * D-93, D-94, D-95). Steps are indivisible and the deadline is absolute, so run(N) followed by run(M) takes the same steps as run(M) from the start:
 * chunk invariance holds by construction. Nothing but the instance survives a step; there is no local cache of CPU state between steps.
 * Order of a call: refuse a malformed request (NULL instance or request, wrong struct_size, nonzero reserved, a sink, a deadline that would wrap
 * the 64-bit machine time), then return UNTIL with zero steps when the deadline is not ahead, then check the stop callback before the first step
 * and after every step. A stop that holds at the step where the deadline is reached is a stop (CALLBACK), as in the Phase 2 budgeted run.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

uint64_t ds_max_step_mclk(void)
{
    /* the longest instruction of the implemented table plus the interrupt entry, in cycles, at the low-speed cycle length (D-94) */
    return ((uint64_t)ds_cpu_max_cycles() + (uint64_t)DS_INT_ENTRY_CYCLES) * (uint64_t)DS_MCLK_PER_CYCLE_LOW;
}

static uint32_t finish(struct ds_run_result *result, const struct ds_instance *inst, uint32_t reason, uint32_t refusal, uint64_t until,
                       uint64_t steps)
{
    if (result != NULL) {
        uint64_t now = inst != NULL ? inst->machine_time : UINT64_C(0);

        result->reason = reason;
        result->refusal = refusal;
        result->stop_point = (uint32_t)DS_STOP_INSTRUCTION;
        result->pad = 0u;
        result->machine_time = now;
        result->overshoot = reason != (uint32_t)DS_RUN_REASON_REFUSED && steps > UINT64_C(0) && now > until ? now - until : UINT64_C(0);
        result->steps = steps;
    }
    return reason;
}

uint32_t ds_run(struct ds_instance *inst, const struct ds_run_request *request, struct ds_run_result *result)
{
    uint64_t guard = ds_max_step_mclk();
    uint64_t steps = UINT64_C(0);
    uint32_t reason = (uint32_t)DS_RUN_REASON_UNTIL;

    if (inst == NULL) {
        return finish(result, NULL, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_instance_null, UINT64_C(0), steps);
    }
    if (request == NULL) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_request_null, UINT64_C(0), steps);
    }
    if (request->struct_size != (uint32_t)sizeof(struct ds_run_request)) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_struct_size, UINT64_C(0), steps);
    }
    if (request->reserved != 0u) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_reserved, UINT64_C(0), steps);
    }
    if (request->insn != NULL || request->dev != NULL) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_sink, UINT64_C(0), steps);
    }
    /* a step can end up to guard clocks past the deadline: a deadline or a machine time within one step of 2^64 would wrap (Pitfall 20) */
    if (request->until_mclk > UINT64_MAX - guard || inst->machine_time > UINT64_MAX - guard) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_deadline, UINT64_C(0), steps);
    }
    if (request->until_mclk <= inst->machine_time) {
        return finish(result, inst, reason, (uint32_t)DS_RUNREF_NONE, request->until_mclk, steps);
    }
    if (request->stop != NULL && request->stop(inst, request->stop_ctx) != 0) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_CALLBACK, (uint32_t)DS_RUNREF_NONE, request->until_mclk, steps);
    }
    for (;;) {
        inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step(inst);
        steps = steps + UINT64_C(1);
        if (request->stop != NULL && request->stop(inst, request->stop_ctx) != 0) {
            reason = (uint32_t)DS_RUN_REASON_CALLBACK;
            break;
        }
        if (inst->machine_time >= request->until_mclk) {
            break;
        }
    }
    return finish(result, inst, reason, (uint32_t)DS_RUNREF_NONE, request->until_mclk, steps);
}
