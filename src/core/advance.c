/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * advance.c - the placeholder stepping of Phase 2 (D-50, D-68, D-75, D-83). There is no CPU yet, so a stepping API that only counted
 * would let a shared-state bug pass every determinism test; evolving, power-on-seeded state is what gives the later interleave,
 * perturbation and shared-state controls teeth. The rules are fixed and documented, and they are PLACEHOLDERS: no hardware claim is
 * made for them (LED-PROF-0002). Phase 3 replaces this file with real stepping.
 *   machine time   adds the cycles (a placeholder cycle, no hardware unit is claimed)
 *   psg_noise      a 32-bit xorshift register (x ^= x << 13; x ^= x >> 17; x ^= x << 5), stepped once per cycle; "LFSR" in D-68 means a
 *                  fixed placeholder generator, not a hardware LFSR
 *   vdc_phase      a 32-bit counter that adds the cycles and wraps
 * Intended wraparound is unsigned (D-83: the sanitize preset has no -fwrapv).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

/* one step of the placeholder generator (xorshift32): a nonzero register stays nonzero */
static uint32_t noise_step(uint32_t x)
{
    x ^= x << 13u;
    x ^= x >> 17u;
    x ^= x << 5u;
    return x;
}

uint32_t ds_advance(struct ds_instance *inst, uint64_t cycles)
{
    uint64_t i;

    if (inst == NULL || cycles > (uint64_t)DS_ADVANCE_MAX) {
        return (uint32_t)DS_ADVANCE_REFUSED;
    }
    inst->machine_time = inst->machine_time + cycles;
    for (i = 0u; i < cycles; i++) {
        inst->psg_noise = noise_step(inst->psg_noise);
    }
    inst->vdc_phase = inst->vdc_phase + (uint32_t)cycles;
    return (uint32_t)DS_ADVANCE_OK;
}

uint32_t ds_run(struct ds_instance *inst, uint64_t budget, uint64_t step, ds_stop_fn stop, void *ctx)
{
    uint64_t consumed = 0u;

    if (inst == NULL || step == 0u || step > (uint64_t)DS_ADVANCE_MAX) {
        return (uint32_t)DS_RUN_REFUSED;
    }
    if (stop != NULL && stop(inst, ctx) != 0) {
        return (uint32_t)DS_RUN_STOPPED;
    }
    while (consumed < budget) {
        uint64_t left = budget - consumed;
        uint64_t chunk = left < step ? left : step;

        (void)ds_advance(inst, chunk);
        consumed = consumed + chunk;
        if (stop != NULL && stop(inst, ctx) != 0) {
            return (uint32_t)DS_RUN_STOPPED;
        }
    }
    return (uint32_t)DS_RUN_BUDGET;
}
