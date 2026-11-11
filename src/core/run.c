/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * run.c - ds_run, the bounded run of the machine in whole instructions to an absolute deadline in master clocks (plan 03-01; D-90, D-92,
 * D-93, D-94, D-95). Steps are indivisible and the deadline is absolute, so run(N) followed by run(M) takes the same steps as run(M) from the start:
 * chunk invariance holds by construction. Nothing but the instance survives a step; there is no local cache of CPU state between steps.
 * Order of a call: refuse a malformed request (NULL instance or request, wrong struct_size, nonzero reserved, a sink that is oversized or not valid memory (plans 03-10 and 03-19: ds_obs_sinks_check), a deadline that would wrap
 * the 64-bit machine time), then (with a sink) overwrite its header, return UNTIL with zero steps when the deadline is not ahead, then check the stop callback before the first step
 * and after every step. The insn hook sees each instruction once, before it runs, the dev hook each logged hardware-page access, and neither can change the instance (ds_obs.h). A stop that holds at the step where the deadline is reached is a stop (CALLBACK), as in the Phase 2 budgeted run.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"

#define DS_RUNREF(name, number) [number] = "run." #name,
static const char *const ds_run_refusal_names[DS_RUNREF_SLOTS] = {
    [0] = "ok",
#include "run_refusals.def"
};
#undef DS_RUNREF

/* the slot ceiling (F4): the rows are numbered 1..N without a gap or a reuse, so N is the row count and must leave a slot below DS_RUNREF_SLOTS */
#define DS_RUNREF(name, number) +1
enum { DS_RUNREF_ROWS = 0
#include "run_refusals.def"
};
#undef DS_RUNREF
_Static_assert(DS_RUNREF_ROWS < (int)DS_RUNREF_SLOTS, "run_refusals.def has more rows than DS_RUNREF_SLOTS slots");

const char *ds_run_refusal_name(uint32_t number)
{
    return number < (uint32_t)DS_RUNREF_SLOTS ? ds_run_refusal_names[number] : NULL;
}

uint64_t ds_max_step_mclk(void)
{
    /* the maximum over the 256 table rows of the base count plus the adders that can apply (the VDC and VCE access penalty included), plus the interrupt entry with its own penalty accesses (at most one per cycle), in cycles, at the low-speed cycle length (D-94, D-107) */
    uint32_t most = 0u;
    uint32_t op;

    for (op = 0u; op < 256u; op++) {
        uint32_t cycles = ds_cpu_op_max_cycles(op);

        if (op == 0x54u || op == 0xD4u) { /* CSL and CSH: a switch that changes the speed may cost extra cycles (constants.def speed_switch_extra_cycles, LED-CPU-0006: none) */
            cycles += (uint32_t)DS_CONST_speed_switch_extra_cycles;
        }
        if (cycles > most) {
            most = cycles;
        }
    }
    return ((uint64_t)most + (uint64_t)DS_INT_ENTRY_CYCLES + (uint64_t)DS_INT_ENTRY_CYCLES * (uint64_t)DS_ADDER_VDC_ACCESS) * (uint64_t)DS_MCLK_PER_CYCLE_LOW;
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
    struct ds_obs_log log;
    struct ds_obs_log *obs = NULL;
    uint32_t refusal;

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
    refusal = ds_obs_sinks_check(request, inst, result);
    if (refusal != (uint32_t)DS_RUNREF_NONE) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, refusal, UINT64_C(0), steps);
    }
    /* a step can end up to guard clocks past the deadline: a deadline or a machine time within one step of 2^64 would wrap (Pitfall 20) */
    if (request->until_mclk > UINT64_MAX - guard || inst->machine_time > UINT64_MAX - guard) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_REFUSED, (uint32_t)DS_RUNREF_time_wrap, UINT64_C(0), steps);
    }
    if (request->insn != NULL || request->dev != NULL) {
        obs = &log;
        ds_obs_begin(obs, request->insn, request->dev);
    }
    if (request->until_mclk <= inst->machine_time) {
        return finish(result, inst, reason, (uint32_t)DS_RUNREF_NONE, request->until_mclk, steps);
    }
    if (request->stop != NULL && request->stop(inst, request->stop_ctx) != 0) {
        return finish(result, inst, (uint32_t)DS_RUN_REASON_CALLBACK, (uint32_t)DS_RUNREF_NONE, request->until_mclk, steps);
    }
    for (;;) {
        inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step_obs(inst, obs);
        steps = steps + UINT64_C(1);
        if (request->stop != NULL && request->stop(inst, request->stop_ctx) != 0) {
            reason = (uint32_t)DS_RUN_REASON_CALLBACK;
            break;
        }
        if (inst->machine_time >= request->until_mclk) {
            break;
        }
    }
    ds_obs_end(obs);
    return finish(result, inst, reason, (uint32_t)DS_RUNREF_NONE, request->until_mclk, steps);
}
