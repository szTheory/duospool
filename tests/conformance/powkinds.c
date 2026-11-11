/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * powkinds.c - the case kinds of power-on state (plan 12; D-50, D-75; the placeholder registers and their domains left in plan 03-36, D-91). Every expected value in a
 * case is a literal derived independently of the C code (a throwaway perl script applying the D-75 derivation with arbitrary-precision
 * integers; the method is noted in the meaning key of each case). Evidence tier E0 (repeatability and a literal): these cases show the
 * implementation agrees with the project's own written rules, and claim nothing about hardware (LED-PROF-0002).
 *   splitmix        request seed=N; expect a comma-separated list of 0x outputs of successive splitmix64 steps started from state N
 *   poweron         boots the request (fixture and request key) and checks the instance against the expect items, separated by ';':
 *                   ram=zero | ram@OFFSET=HEXBYTES | mpr=zero | time=N | policy=NAME | seed=N|none
 *   poweron_domain  request seed=N; expect extra=A,B: the first 16 bytes of the ram8k stream are unchanged when another stream (the one
 *                   of a value that names no domain, whose constant is 0) draws A or B values in between, that other stream's bytes
 *                   differ from the ram8k bytes, and a boot reproduces the ram8k stream
 * (the run kinds run_until and run_chunk_sum are in stepkinds.c; the stepping that this file once covered is gone, D-91.) Test code only, never installed.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_poweron.h"
#include "runner.h"

/* the text of a key, copied into a work buffer; 0 on success */
int pk_copy_text(const char *src, char *dst, size_t cap)
{
    if (strlen(src) >= cap) {
        return -1;
    }
    memcpy(dst, src, strlen(src) + 1u);
    return 0;
}

/* the next item of a list in place: the item, with *rest left at the following one (NULL at the end) */
char *pk_next_item(char **rest, char sep)
{
    char *item = *rest;
    char *end;

    if (item == NULL || *item == '\0') {
        return NULL;
    }
    end = strchr(item, sep);
    if (end != NULL) {
        *end = '\0';
        *rest = end + 1;
    } else {
        *rest = NULL;
    }
    return item;
}

static int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
}

/* HEX digits to bytes; the count, or -1 for an odd length, a non-digit or an overflow of cap */
static long hex_bytes(const char *h, unsigned char *out, size_t cap)
{
    size_t n = strlen(h);
    size_t i;

    if (n == 0u || n % 2u != 0u || n / 2u > cap) {
        return -1;
    }
    for (i = 0u; i < n / 2u; i++) {
        int hi = hex_digit(h[2u * i]);
        int lo = hex_digit(h[2u * i + 1u]);

        if (hi < 0 || lo < 0) {
            return -1;
        }
        out[i] = (unsigned char)(((unsigned)hi << 4) | (unsigned)lo);
    }
    return (long)(n / 2u);
}

/* ---- kind splitmix ---- */

void kind_splitmix(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct spec s;
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    const char *err = parse_spec(&s, k->val[K_REQUEST], 0u);
    uint64_t state;
    unsigned draws = 0u;

    if (err == NULL && !s.has_seed) {
        err = "a splitmix case needs seed=N";
    }
    if (err != NULL || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, err != NULL ? err : "the expect list is too long");
        return;
    }
    state = s.seed;
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        uint64_t want;

        if (parse_u64(item, &want) != 0) {
            harness_error(c, "a splitmix literal is not a 0x number");
            return;
        }
        draws++;
        check(c, ds_splitmix64_next(&state) == want, "a splitmix64 output differs from the independently derived literal");
    }
    check(c, draws >= 1u, "the splitmix case lists no literal");
    snprintf(got, gotcap, "draws=%u", draws);
}

/* ---- kind poweron ---- */

static int is_all_zero(const unsigned char *p, size_t n)
{
    size_t i;

    for (i = 0u; i < n; i++) {
        if (p[i] != 0u) {
            return 0;
        }
    }
    return 1;
}

void kind_poweron(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        release_boot(&b);
        harness_error(c, "the expect list is too long");
        return;
    }
    snprintf(got, gotcap, "booted");
    while ((item = pk_next_item(&rest, ';')) != NULL && !c->harness) {
        uint64_t num;

        if (strcmp(item, "ram=zero") == 0) {
            check(c, is_all_zero(inst->ram8k, sizeof inst->ram8k), "the work RAM is not all zero");
        } else if (strcmp(item, "mpr=zero") == 0) {
            check(c, is_all_zero(inst->mpr, sizeof inst->mpr), "the mapping registers are not the all-zero constant");
        } else if (strncmp(item, "ram@", 4) == 0) {
            char *eq = strchr(item, '=');
            unsigned char want[64];
            size_t off;
            long n;

            if (eq == NULL) {
                harness_error(c, "a ram@ item has no =");
                break;
            }
            *eq = '\0';
            n = hex_bytes(eq + 1, want, sizeof want);
            if (parse_size(item + 4, &off) != 0 || n < 0 || off > sizeof inst->ram8k || (size_t)n > sizeof inst->ram8k - off) {
                harness_error(c, "a ram@ item is malformed or outside the 8 KiB RAM");
                break;
            }
            check(c, memcmp(inst->ram8k + off, want, (size_t)n) == 0, "the work RAM differs from the independently derived literal");
        } else if (strncmp(item, "time=", 5) == 0) {
            char *eq = strchr(item, '=');

            if (parse_u64(eq + 1, &num) != 0) {
                harness_error(c, "a numeric expect item is not a number");
                break;
            }
            check(c, inst->machine_time == num, "machine time is not the expected value");
        } else if (strncmp(item, "policy=", 7) == 0) {
            check(c, strcmp(inst->profile.field[DS_FIELD_powerup_policy].value, item + 7) == 0,
                  "the recorded profile value of powerup_policy differs");
            check(c, inst->powerup_policy == ds_policy_of(item + 7) && inst->powerup_policy != 0u, "the recorded policy code differs");
        } else if (strncmp(item, "seed=", 5) == 0) {
            if (strcmp(item + 5, "none") == 0) {
                check(c, inst->has_seed == 0u && inst->powerup_seed == 0u, "a seedless boot recorded a seed");
            } else if (parse_u64(item + 5, &num) != 0) {
                harness_error(c, "the seed item is not a number or none");
            } else {
                check(c, inst->has_seed == 1u && inst->powerup_seed == num, "the recorded seed differs");
            }
        } else {
            harness_error(c, "unknown poweron expect item");
        }
    }
    release_boot(&b);
}

/* ---- kind poweron_domain ---- */

#define PD_BYTES 16u
/* a value that names no domain: ds_domain_const gives it the constant 0, so its stream is a second, independent stream of the same seed */
#define PD_OTHER ((uint32_t)DS_DOMAIN_COUNT)

void kind_poweron_domain(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct spec s;
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    const char *err = parse_spec(&s, k->val[K_REQUEST], 0u);
    unsigned char base[PD_BYTES];
    unsigned char other16[PD_BYTES];
    struct ds_stream st;
    uint64_t bare;
    unsigned runs = 0u;

    if (err == NULL && !s.has_seed) {
        err = "a poweron_domain case needs seed=N";
    }
    if (err != NULL || strncmp(k->val[K_EXPECT], "extra=", 6) != 0 || pk_copy_text(k->val[K_EXPECT] + 6, list, sizeof list) != 0) {
        harness_error(c, err != NULL ? err : "the expect key is not extra=A,B");
        return;
    }
    check(c, ds_domain_const(PD_OTHER) == UINT64_C(0), "a value that names no domain has a nonzero constant");
    ds_stream_init(&st, s.seed, (uint32_t)DS_DOMAIN_RAM8K);
    ds_stream_fill(&st, base, sizeof base);
    ds_stream_init(&st, s.seed, PD_OTHER);
    ds_stream_fill(&st, other16, sizeof other16);
    check(c, memcmp(base, other16, sizeof base) != 0, "the ram8k domain and another stream drew the same bytes");
    /* the derivation is a splitmix64 step started from (seed XOR constant): a stream that starts at the bare XOR is another stream */
    ds_stream_init(&st, s.seed, (uint32_t)DS_DOMAIN_RAM8K);
    bare = s.seed ^ ds_domain_const((uint32_t)DS_DOMAIN_RAM8K);
    check(c, st.state != bare, "the stream starts at the bare seed XOR constant: the splitmix step of D-75 is missing");

    while ((item = pk_next_item(&rest, ',')) != NULL) {
        uint64_t extra;
        struct ds_stream ram;
        struct ds_stream other;
        unsigned char mixed[PD_BYTES];
        uint64_t i;

        if (parse_u64(item, &extra) != 0) {
            harness_error(c, "an extra count is not a number");
            return;
        }
        ds_stream_init(&ram, s.seed, (uint32_t)DS_DOMAIN_RAM8K);
        ds_stream_init(&other, s.seed, PD_OTHER);
        ds_stream_fill(&ram, mixed, 8u);
        for (i = 0u; i < extra; i++) {
            (void)ds_stream_next(&other);
        }
        ds_stream_fill(&ram, mixed + 8, 8u);
        check(c, memcmp(mixed, base, sizeof base) == 0, "the ram8k bytes changed when another stream drew extra values");
        runs++;
    }
    check(c, runs >= 1u, "the poweron_domain case lists no extra count");
    {
        /* a boot with the same seed lays down exactly the ram8k stream */
        struct booted b;
        struct spec bs;
        const struct ds_instance *inst = NULL;
        char req[96];

        snprintf(req, sizeof req, "bind.powerup_policy=seeded-prng-v1;seed=%llu", (unsigned long long)s.seed);
        err = boot_accepted("plain-256k", req, &b, &bs, &inst);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        check(c, memcmp(inst->ram8k, base, sizeof base) == 0, "a boot with this seed does not lay down the ram8k stream");
        release_boot(&b);
    }
    snprintf(got, gotcap, "runs=%u", runs);
}
