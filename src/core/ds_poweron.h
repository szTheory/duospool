/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_poweron.h - the power-on policies and the deterministic streams behind them (D-50, D-75, D-79(b)). Internal to the core and its
 * tests: reached only through the include directory src/core. Every value here is a modelling assumption of the project (LED-PROF-0002),
 * not a hardware observation.
 * Policy names: zero, seeded-prng-v1, seeded-prng-v2 and seeded-prng-v3. The v1 suffix freezes the algorithm (splitmix64), the per-domain derivation
 * sub = splitmix64(seed XOR DOMAIN_CONST), the domain set and the draw order. seeded-prng-v2 (plan 03-24; D-114, D-134) is additive:
 * it draws the v1 domain unchanged and then the domains cpu, mpr and tmr (poweron.c says which byte goes where); every reset fact
 * (H8-18) stays a constant under all four policies, and io_buffer and mpr_latch have no domain and stay 0. seeded-prng-v3 (plan 04-18; D-167) is additive again: every v2 draw in the same order and with the same bytes, then the domains
 * VRAM____, SATB____ and PAL_____ filled straight into the VRAM, SATB and palette byte arrays; every VDC and VCE register, latch, status, beam and DMA byte stays 0 under every policy.
 */
#ifndef DS_POWERON_H
#define DS_POWERON_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

enum ds_policy { DS_POLICY_NONE = 0, DS_POLICY_ZERO = 1, DS_POLICY_SEEDED = 2, DS_POLICY_SEEDED_V2 = 3, DS_POLICY_SEEDED_V3 = 4 };

/* the domains: ram8k is the whole of seeded-prng-v1 (RAM only since plan 03-36); cpu, mpr and tmr are added by seeded-prng-v2 (plan 03-24); vram, satb and pal by seeded-prng-v3 (plan 04-18). The order here is not the derivation (the derivation uses the fixed constants of poweron.c) */
enum ds_domain { DS_DOMAIN_RAM8K = 0, DS_DOMAIN_CPU, DS_DOMAIN_MPR, DS_DOMAIN_TMR, DS_DOMAIN_VRAM, DS_DOMAIN_SATB, DS_DOMAIN_PAL, DS_DOMAIN_COUNT };

/* one domain stream: a splitmix64 generator whose state starts at the domain's sub-seed */
struct ds_stream {
    uint64_t state;
};

/* one splitmix64 step: advances *state and returns the output (unsigned arithmetic only) */
uint64_t ds_splitmix64_next(uint64_t *state);

/* the fixed 64-bit constant of a domain (ASCII-packed name), 0 for a value that is not a domain */
uint64_t ds_domain_const(uint32_t domain);

/* start the stream of a domain for a seed: state = the output of one splitmix64 step started from (seed XOR DOMAIN_CONST) */
void ds_stream_init(struct ds_stream *s, uint64_t seed, uint32_t domain);

/* the next 64-bit draw of a stream */
uint64_t ds_stream_next(struct ds_stream *s);

/* n bytes from successive draws, each draw written as little-endian bytes; a final partial draw is truncated, never carried over */
void ds_stream_fill(struct ds_stream *s, uint8_t *out, size_t n);

/* the policy named by a bound profile value: enum ds_policy, DS_POLICY_NONE for any other text */
uint32_t ds_policy_of(const char *value);

/* 1 for the three seeded policies (they need a seed, D-79(b)), 0 for zero and for DS_POLICY_NONE or any other value */
uint32_t ds_policy_is_seeded(uint32_t policy);

/* the name of a policy as bound in the profile field powerup_policy, NULL for DS_POLICY_NONE or any other value */
const char *ds_policy_name(uint32_t policy);

/* the version of the policy table that the run record prints next to the name (D-50, D-134): 1 for zero and seeded-prng-v1, 2 for seeded-prng-v2, 3 for seeded-prng-v3, 0 for DS_POLICY_NONE or any other value */
uint32_t ds_policy_version(uint32_t policy);

/* set the power-on state of a zeroed instance from its recorded policy, has_seed and powerup_seed */
void ds_poweron_apply(struct ds_instance *inst);

#endif
