/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * poweron.c - power-on state (CORE-03; D-50, D-75, D-79(b), D-83).
 * Two policies: zero (RAM zero, psg_noise 1, vdc_phase 0) and seeded-prng-v1. The seeded policy derives one stream per domain:
 *     sub    = splitmix64 step started from state (seed XOR DOMAIN_CONST)      (one step; a bare XOR with no step is not the algorithm)
 *     stream = a splitmix64 generator whose initial state is sub
 *     draws  = successive 64-bit outputs of the stream, written as little-endian bytes
 * ram8k draws 8192 bytes (1024 draws); psg_noise takes the low 32 bits of its first output (forced to 1 when that is 0, because the
 * placeholder generator that the register once fed would stay at 0 forever); vdc_phase takes the low 32 bits of its first output. The domain constants are
 * fixed 64-bit values (the ASCII-packed domain names), never enum ordinals, so reordering the enum cannot change a draw. The eight mapping
 * registers are an all-zero documented constant with an open ledger item for Phase 3.
 * Everything here is a modelling assumption of this project (LED-PROF-0002): no value or evolution rule is a hardware claim. Intended
 * wraparound is unsigned: the sanitize preset has no -fwrapv, so a signed or implicit wrap here would be a defect (D-83).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_poweron.h"

/* ASCII-packed names, most significant byte first: "RAM8K___", "PSGNOISE", "VDCPHASE" */
static const uint64_t domain_const[DS_DOMAIN_COUNT] = {
    UINT64_C(0x52414D384B5F5F5F), /* ram8k */
    UINT64_C(0x5053474E4F495345), /* psg_noise */
    UINT64_C(0x5644435048415345), /* vdc_phase */
};

uint64_t ds_splitmix64_next(uint64_t *state)
{
    uint64_t z;

    *state = *state + UINT64_C(0x9E3779B97F4A7C15);
    z = *state;
    z = (z ^ (z >> 30u)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27u)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31u);
}

uint64_t ds_domain_const(uint32_t domain)
{
    return domain < (uint32_t)DS_DOMAIN_COUNT ? domain_const[domain] : UINT64_C(0);
}

void ds_stream_init(struct ds_stream *s, uint64_t seed, uint32_t domain)
{
    uint64_t start = seed ^ ds_domain_const(domain);

    s->state = ds_splitmix64_next(&start);
}

uint64_t ds_stream_next(struct ds_stream *s)
{
    return ds_splitmix64_next(&s->state);
}

void ds_stream_fill(struct ds_stream *s, uint8_t *out, size_t n)
{
    size_t i = 0u;

    while (i < n) {
        uint64_t word = ds_stream_next(s);
        unsigned shift;

        for (shift = 0u; shift < 64u && i < n; shift += 8u) {
            out[i] = (uint8_t)(word >> shift);
            i++;
        }
    }
}

uint32_t ds_policy_of(const char *value)
{
    if (ds_streq(value, "zero")) {
        return (uint32_t)DS_POLICY_ZERO;
    }
    return ds_streq(value, "seeded-prng-v1") ? (uint32_t)DS_POLICY_SEEDED : (uint32_t)DS_POLICY_NONE;
}

void ds_poweron_apply(struct ds_instance *inst)
{
    /* the instance was zeroed by the caller: RAM, the mapping registers (the documented all-zero constant) and vdc_phase are 0 already */
    inst->psg_noise = 1u;
    if (inst->powerup_policy == (uint32_t)DS_POLICY_SEEDED) {
        struct ds_stream ram;
        struct ds_stream noise;
        struct ds_stream phase;
        uint32_t first;

        ds_stream_init(&ram, inst->powerup_seed, (uint32_t)DS_DOMAIN_RAM8K);
        ds_stream_fill(&ram, inst->ram8k, (size_t)DS_RAM8K_BYTES);
        ds_stream_init(&noise, inst->powerup_seed, (uint32_t)DS_DOMAIN_PSG_NOISE);
        first = (uint32_t)ds_stream_next(&noise);
        inst->psg_noise = first != 0u ? first : 1u;
        ds_stream_init(&phase, inst->powerup_seed, (uint32_t)DS_DOMAIN_VDC_PHASE);
        inst->vdc_phase = (uint32_t)ds_stream_next(&phase);
    }
}
