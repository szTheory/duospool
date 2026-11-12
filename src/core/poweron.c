/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * poweron.c - power-on state (CORE-03, CPU-06; D-50, D-75, D-79(b), D-83, D-114, D-134).
 * Four policies: zero (everything the manual leaves open is 0), seeded-prng-v1 (RAM only), seeded-prng-v2 (RAM and the open registers) and seeded-prng-v3 (v2 and the VRAM, SATB and palette bytes). A seeded policy derives one stream per domain:
 *     sub    = splitmix64 step started from state (seed XOR DOMAIN_CONST)      (one step; a bare XOR with no step is not the algorithm)
 *     stream = a splitmix64 generator whose initial state is sub
 *     draws  = successive 64-bit outputs of the stream, written as little-endian bytes
 * ram8k draws 8192 bytes (1024 draws). The domain constants are fixed 64-bit values (the ASCII-packed domain names), never enum ordinals,
 * so reordering the enum cannot change a draw. The two placeholder-register domains of the first phases are gone (D-91,
 * D-114): each domain has its own stream, so the RAM draws did not move.
 * seeded-prng-v1 draws the ram8k domain only; the mapping registers, the CPU registers and the timer registers are 0 under zero and v1 (v1 is frozen byte for byte, D-50).
 * seeded-prng-v2 draws the v1 domain unchanged and then, each from its own stream and in this order (little-endian draws, byte k is byte k of the stream):
 *     cpu  5 bytes: A, X, Y, S, then the byte whose bits 7, 6, 1 and 0 are N, V, Z and C of the status register (bits 5, 4, 3 and 2 of that byte are not used)
 *     mpr  7 bytes: MPR0 to MPR6, all eight bits each
 *     tmr  2 bytes: the timer reload register (the byte and $7F), then the timer counter (the byte and $7F); both are 7-bit registers that H8-20 calls invalid after reset
 * What stays constant under every policy, because the manual states it (H8-18, H8-8): I set, D and T clear, low speed, MPR7 = $00, the timer stopped, the disable register 0 and TIQ clear
 * (cpu.c sets them at reset). io_buffer and mpr_latch have no domain and stay 0. The domain constants are the ASCII-packed names padded with underscores: "CPU_____", "MPR_____", "TMR_____".
 * seeded-prng-v3 (plan 04-18; D-167) draws every v2 domain unchanged and then, each from its own stream and in this order, straight into the byte arrays (word w is bytes 2w and 2w+1, little-endian, which is byte k of the stream):
 *     vram 65,536 bytes of vdc_vram ("VRAM____"), satb 512 bytes of vdc_satb ("SATB____"), pal 1,024 bytes of vce_pal ("PAL_____"), then bits 1..7 of every odd palette byte are cleared (a colour entry has 9 bits)
 * zero, v1 and v2 leave VRAM, SATB and the palette 0. No VDC or VCE register, latch, status, beam or DMA byte is drawn under any policy: a drawn control register could raise IRQ1 under a Phase 3 guest that knows nothing of the VDC.
 * Everything here is a modelling assumption of this project (LED-PROF-0002): no value or evolution rule is a hardware claim. Intended
 * wraparound is unsigned: the sanitize preset has no -fwrapv, so a signed or implicit wrap here would be a defect (D-83).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_poweron.h"

/* ASCII-packed names, most significant byte first: "RAM8K___" */
static const uint64_t domain_const[DS_DOMAIN_COUNT] = {
    UINT64_C(0x52414D384B5F5F5F), /* ram8k */
    UINT64_C(0x4350555F5F5F5F5F), /* cpu: "CPU_____" */
    UINT64_C(0x4D50525F5F5F5F5F), /* mpr: "MPR_____" */
    UINT64_C(0x544D525F5F5F5F5F), /* tmr: "TMR_____" */
    UINT64_C(0x5652414D5F5F5F5F), /* vram: "VRAM____" */
    UINT64_C(0x534154425F5F5F5F), /* satb: "SATB____" */
    UINT64_C(0x50414C5F5F5F5F5F), /* pal: "PAL_____" */
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
    if (ds_streq(value, "seeded-prng-v1")) {
        return (uint32_t)DS_POLICY_SEEDED;
    }
    if (ds_streq(value, "seeded-prng-v2")) {
        return (uint32_t)DS_POLICY_SEEDED_V2;
    }
    return ds_streq(value, "seeded-prng-v3") ? (uint32_t)DS_POLICY_SEEDED_V3 : (uint32_t)DS_POLICY_NONE;
}

uint32_t ds_policy_is_seeded(uint32_t policy)
{
    return policy == (uint32_t)DS_POLICY_SEEDED || policy == (uint32_t)DS_POLICY_SEEDED_V2 || policy == (uint32_t)DS_POLICY_SEEDED_V3 ? 1u : 0u;
}

const char *ds_policy_name(uint32_t policy)
{
    if (policy == (uint32_t)DS_POLICY_ZERO) {
        return "zero";
    }
    if (policy == (uint32_t)DS_POLICY_SEEDED) {
        return "seeded-prng-v1";
    }
    if (policy == (uint32_t)DS_POLICY_SEEDED_V2) {
        return "seeded-prng-v2";
    }
    return policy == (uint32_t)DS_POLICY_SEEDED_V3 ? "seeded-prng-v3" : NULL;
}

uint32_t ds_policy_version(uint32_t policy)
{
    if (policy == (uint32_t)DS_POLICY_ZERO || policy == (uint32_t)DS_POLICY_SEEDED) {
        return 1u;
    }
    if (policy == (uint32_t)DS_POLICY_SEEDED_V2) {
        return 2u;
    }
    return policy == (uint32_t)DS_POLICY_SEEDED_V3 ? 3u : 0u;
}

/* the P bits that seeded-prng-v2 draws: N, V, Z and C (I, D and T are reset facts, B is never stored) */
#define POWERON_P_DRAWN 0xC3u
#define POWERON_REG7 0x7Fu

/* the v2 domains cpu, mpr and tmr (see the header comment for the layout of each) */
static void apply_v2(struct ds_instance *inst)
{
    struct ds_stream st;
    uint8_t buf[DS_MPR_COUNT];
    uint32_t i;

    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_CPU);
    ds_stream_fill(&st, buf, 5u);
    inst->a = buf[0];
    inst->x = buf[1];
    inst->y = buf[2];
    inst->s = buf[3];
    inst->p = (uint8_t)((uint32_t)buf[4] & POWERON_P_DRAWN);
    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_MPR);
    ds_stream_fill(&st, buf, 7u);
    for (i = 0u; i < 7u; i++) {
        inst->mpr[i] = buf[i];
    }
    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_TMR);
    ds_stream_fill(&st, buf, 2u);
    inst->timer_reload = (uint8_t)((uint32_t)buf[0] & POWERON_REG7);
    inst->timer_counter = (uint8_t)((uint32_t)buf[1] & POWERON_REG7);
}

/* the v3 domains: the three byte arrays of the video state, each straight from its own stream; the odd palette bytes keep bit 0 only (a colour entry is 9 bits) */
static void apply_v3(struct ds_instance *inst)
{
    struct ds_stream st;
    size_t i;

    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_VRAM);
    ds_stream_fill(&st, inst->vdc_vram, sizeof inst->vdc_vram);
    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_SATB);
    ds_stream_fill(&st, inst->vdc_satb, sizeof inst->vdc_satb);
    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_PAL);
    ds_stream_fill(&st, inst->vce_pal, sizeof inst->vce_pal);
    for (i = 1u; i < sizeof inst->vce_pal; i += 2u) {
        inst->vce_pal[i] = (uint8_t)((uint32_t)inst->vce_pal[i] & 0x01u);
    }
}

void ds_poweron_apply(struct ds_instance *inst)
{
    /* the instance was zeroed by the caller: RAM, the mapping registers, the CPU registers and the timer registers are 0 already, which is the whole of the zero policy */
    if (ds_policy_is_seeded(inst->powerup_policy) != 0u) {
        struct ds_stream ram;

        ds_stream_init(&ram, inst->powerup_seed, (uint32_t)DS_DOMAIN_RAM8K);
        ds_stream_fill(&ram, inst->ram8k, (size_t)DS_RAM8K_BYTES);
    }
    if (inst->powerup_policy >= (uint32_t)DS_POLICY_SEEDED_V2) {
        apply_v2(inst); /* v3 draws every v2 domain unchanged */
    }
    if (inst->powerup_policy >= (uint32_t)DS_POLICY_SEEDED_V3) {
        apply_v3(inst);
    }
}
