/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * stepkinds.c - the case kinds of the placeholder stepping (plan 12; D-50, D-57, D-68). Every expected value in a case is a literal
 * derived independently of the C code (a throwaway perl script outside the repo applying the written rules). Evidence tier E0
 * (repeatability and a literal): the stepping rules are placeholders and claim nothing about hardware (LED-PROF-0002). Test code only.
 *   advance         boots the request, then applies the expect items in order, separated by ';': advance=N (ds_advance must accept),
 *                   refuse=N (ds_advance must refuse and change nothing), set_noise=0xN | set_vdc=0xN (write the placeholder state),
 *                   time=N | psg_noise=0xN | vdc_phase=0xN (check). replay=K replays every advance=N on a second identical boot in
 *                   chunks of K cycles and requires the same final state (the result must not depend on how the cycles are split)
 *   run_budget      expect is a list of items separated by '|', each on its own fresh boot, with comma-separated keys budget, step, stop
 *                   (- never, N stops once machine time is at least N), result (stopped, budget or refused) and time (machine time after)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_poweron.h"
#include "runner.h"

/* ---- kind advance ---- */

struct pstate {
    uint64_t time;
    uint32_t noise;
    uint32_t vdc;
};

static struct pstate state_of(const struct ds_instance *inst)
{
    struct pstate p;

    p.time = inst->machine_time;
    p.noise = inst->psg_noise;
    p.vdc = inst->vdc_phase;
    return p;
}

static int same_state(const struct pstate *a, const struct pstate *b)
{
    return a->time == b->time && a->noise == b->noise && a->vdc == b->vdc;
}

/*
 * Apply the item list to an instance. chunk 0 is the checked pass: every advance=N is one call and every check item is asserted.
 * chunk K is the replay: advance=N is made in calls of K cycles and nothing is asserted. 0 on success, -1 on a malformed item.
 */
static int apply_items(struct ds_instance *inst, const char *text, size_t chunk, struct ctx *c)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        uint64_t num;

        if (eq == NULL || parse_u64(eq + 1, &num) != 0) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(item, "advance") == 0) {
            uint64_t left = num;

            do {
                uint64_t step = chunk != 0u && left > chunk ? (uint64_t)chunk : left;
                uint32_t st = ds_advance(inst, step);

                if (chunk == 0u) {
                    check(c, st == (uint32_t)DS_ADVANCE_OK, "ds_advance refused a call within DS_ADVANCE_MAX");
                }
                left -= step;
            } while (left > 0u);
        } else if (strcmp(item, "refuse") == 0) {
            if (chunk == 0u) {
                struct pstate before = state_of(inst);
                uint32_t st = ds_advance(inst, num);
                struct pstate after = state_of(inst);

                check(c, st == (uint32_t)DS_ADVANCE_REFUSED, "ds_advance accepted a call above DS_ADVANCE_MAX");
                check(c, same_state(&before, &after), "a refused ds_advance changed the state");
            }
        } else if (strcmp(item, "set_noise") == 0) {
            inst->psg_noise = (uint32_t)num;
        } else if (strcmp(item, "set_vdc") == 0) {
            inst->vdc_phase = (uint32_t)num;
        } else if (strcmp(item, "time") == 0) {
            if (chunk == 0u) {
                check(c, inst->machine_time == num, "machine time differs from the independently derived literal");
            }
        } else if (strcmp(item, "psg_noise") == 0) {
            if (chunk == 0u) {
                check(c, (uint64_t)inst->psg_noise == num, "psg_noise differs from the independently derived literal");
            }
        } else if (strcmp(item, "vdc_phase") == 0) {
            if (chunk == 0u) {
                check(c, (uint64_t)inst->vdc_phase == num, "vdc_phase differs from the independently derived literal");
            }
        } else if (strcmp(item, "replay") != 0) {
            return -1;
        }
    }
    return 0;
}

void kind_advance(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    const char *rp = strstr(k->val[K_EXPECT], "replay=");
    struct ds_instance *w;
    struct pstate first;

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    if (apply_items(w, k->val[K_EXPECT], 0u, c) != 0) {
        release_boot(&b);
        harness_error(c, "an advance expect item is malformed");
        return;
    }
    first = state_of(w);
    snprintf(got, gotcap, "time=%llu", (unsigned long long)first.time);
    if (rp != NULL) {
        struct booted b2;
        struct spec s2;
        const struct ds_instance *inst2 = NULL;
        uint64_t chunk = 0u;

        err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b2, &s2, &inst2);
        if (err != NULL) {
            release_boot(&b);
            harness_error(c, err);
            return;
        }
        {
            char num[24];
            size_t len = strcspn(rp + 7, ";");

            if (len > 0u && len < sizeof num) {
                memcpy(num, rp + 7, len);
                num[len] = '\0';
                if (parse_u64(num, &chunk) != 0) {
                    chunk = 0u;
                }
            }
        }
        if (chunk == 0u || apply_items((struct ds_instance *)(void *)b2.mem, k->val[K_EXPECT], (size_t)chunk, c) != 0) {
            release_boot(&b2);
            release_boot(&b);
            harness_error(c, "the replay chunk is 0 or an item is malformed");
            return;
        }
        {
            struct pstate second = state_of((const struct ds_instance *)(void *)b2.mem);

            check(c, same_state(&first, &second), "the final state depends on how the cycles are split across calls");
        }
        release_boot(&b2);
    }
    release_boot(&b);
}

/* ---- kind run_budget ---- */

struct stop_ctx {
    int never;
    uint64_t at;
};

static int stop_at(const struct ds_instance *inst, void *ctx)
{
    const struct stop_ctx *sc = (const struct stop_ctx *)ctx;

    return !sc->never && inst->machine_time >= sc->at;
}

static int run_item(const struct kase *k, char *text, struct ctx *c)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err;
    struct stop_ctx sc;
    uint64_t budget = 0u;
    uint64_t step = 0u;
    uint64_t want_time = 0u;
    uint32_t want_status = 0u;
    int have_time = 0;
    char *rest = text;
    char *kv;
    uint32_t status;

    sc.never = 1;
    sc.at = 0u;
    while ((kv = pk_next_item(&rest, ',')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(kv, "budget") == 0) {
            if (parse_u64(eq + 1, &budget) != 0) {
                return -1;
            }
        } else if (strcmp(kv, "step") == 0) {
            if (parse_u64(eq + 1, &step) != 0) {
                return -1;
            }
        } else if (strcmp(kv, "stop") == 0) {
            sc.never = strcmp(eq + 1, "-") == 0;
            if (!sc.never && parse_u64(eq + 1, &sc.at) != 0) {
                return -1;
            }
        } else if (strcmp(kv, "time") == 0) {
            have_time = 1;
            if (parse_u64(eq + 1, &want_time) != 0) {
                return -1;
            }
        } else if (strcmp(kv, "result") == 0) {
            want_status = strcmp(eq + 1, "stopped") == 0 ? (uint32_t)DS_RUN_STOPPED
                          : strcmp(eq + 1, "budget") == 0 ? (uint32_t)DS_RUN_BUDGET
                          : strcmp(eq + 1, "refused") == 0 ? (uint32_t)DS_RUN_REFUSED : 0u;
        } else {
            return -1;
        }
    }
    if (want_status == 0u) {
        return -1;
    }
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return 0;
    }
    status = ds_run_placeholder((struct ds_instance *)(void *)b.mem, budget, step, stop_at, &sc);
    check(c, status == want_status, "ds_run_placeholder returned a different status than expected");
    check(c, !have_time || inst->machine_time == want_time, "machine time after ds_run_placeholder differs from the expected value");
    check(c, status == (uint32_t)DS_RUN_REFUSED || inst->machine_time <= budget, "machine time went past the budget");
    release_boot(&b);
    return 0;
}

void kind_run_budget(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned n = 0u;

    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the expect list is too long");
        return;
    }
    while ((item = pk_next_item(&rest, '|')) != NULL && !c->harness) {
        if (run_item(k, item, c) != 0) {
            harness_error(c, "a run_budget item is malformed");
            return;
        }
        n++;
    }
    check(c, n >= 1u, "the run_budget case lists no item");
    snprintf(got, gotcap, "items=%u", n);
}
