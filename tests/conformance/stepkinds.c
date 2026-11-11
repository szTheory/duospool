/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * stepkinds.c - the case kinds of the run API (plan 03-02; D-93, D-94, D-95). Test code only. Evidence tier E0: repeatability of the run API and
 * the boundary behaviour of its absolute deadline; nothing here is an accuracy, compatibility or hardware claim.
 * Amendment record (plan 03-02): D-68 and D-75 are SUPERSEDED by D-91. The placeholder stepping API (ds_advance, ds_run_placeholder, DS_ADVANCE_MAX,
 * src/core/advance.c) and the Phase 2 kinds that used it (advance, run_budget) are gone: every kind that steps the machine runs ds_run to an
 * absolute until_mclk, never a relative budget, so run(N) then run(M) is run(N+M) by construction. psg_noise and vdc_phase stay in the instance and in
 * the digest until plans 03-03 and 03-36 remove them, so no golden changes in this plan. The timeout-budget mini-root keeps its expect text
 * budget=N,step=M: it now means absolute deadlines in chunks of step master clocks (spinkinds.c).
 *   run_until       expect is a list of items separated by '|'. The first item is rom=NAME (a built authored image, build/gate/fixtures/NAME.pce);
 *                   every other item runs on its own fresh boot and has comma-separated keys until (absolute deadline in master clocks, required),
 *                   stop (- never, N: the stop callback holds once machine time is at least N), result (until, the default, or callback), steps (required),
 *                   time (machine time after, optional) and again (a second ds_run with this deadline,
 *                   which is not ahead of machine time, so it must return until with 0 steps, overshoot 0 and leave the whole instance unchanged).
 *                   Every item also checks overshoot = machine time - until when a step was taken and positive, and overshoot below ds_max_step_mclk() (D-94).
 *                   The literals follow from the ds_proto instruction cycle table and the 12 master clocks of a low-speed cycle, never from a run.
 *   run_chunk_sum   expect is rom=NAME;until=N;chunks=LIST where LIST is comma-separated chunk sizes in master clocks and the word mix (the sizes
 *                   1, 7, 168 and 3072 in rotation). The reference runs once to the absolute deadline until, which must lie past the guest's DSV1
 *                   done state (D-95). Every list then runs on a fresh boot to deadlines d1 = c, d2 = d1 + c, ... capped at until, and must end with the
 *                   same machine time, total steps, CPU registers and digest as the reference. This compares runs with each other: E0 repeatability.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_serial.h"
#include "runner.h"

/* the stop callback: never, or once machine time is at least at (checked before the first step and after every step) */
struct stop_ctx {
    int never;
    uint64_t at;
};

static int stop_at(const struct ds_instance *inst, void *ctx)
{
    const struct stop_ctx *sc = (const struct stop_ctx *)ctx;

    return !sc->never && inst->machine_time >= sc->at;
}

/* one ds_run to an absolute deadline with an optional stop; the reason, which is REFUSED when the call itself fails */
static uint32_t run_abs(struct ds_instance *inst, uint64_t until, struct stop_ctx *sc, struct ds_run_result *out)
{
    struct ds_run_request rq;

    memset(&rq, 0, sizeof rq);
    memset(out, 0, sizeof *out);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    if (sc != NULL) {
        rq.stop = stop_at;
        rq.stop_ctx = sc;
    }
    return ds_run(inst, &rq, out);
}

/* ---- kind run_until ---- */

struct item {
    uint64_t until;
    uint64_t steps;
    uint64_t time;
    uint64_t again;
    uint32_t result;
    int have_time;
    int have_again;
    struct stop_ctx sc;
};

static int item_read(struct item *it, char *text)
{
    char *rest = text;
    char *kv;
    int have = 0;

    memset(it, 0, sizeof *it);
    it->sc.never = 1;
    it->result = (uint32_t)DS_RUN_REASON_UNTIL;
    while ((kv = pk_next_item(&rest, ',')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(kv, "until") == 0 && parse_u64(eq + 1, &it->until) == 0) {
            have |= 1;
        } else if (strcmp(kv, "steps") == 0 && parse_u64(eq + 1, &it->steps) == 0) {
            have |= 2;
        } else if (strcmp(kv, "result") == 0) {
            it->result = strcmp(eq + 1, "until") == 0 ? (uint32_t)DS_RUN_REASON_UNTIL : strcmp(eq + 1, "callback") == 0 ? (uint32_t)DS_RUN_REASON_CALLBACK : 0u;
            if (it->result == 0u) {
                return -1;
            }
        } else if (strcmp(kv, "stop") == 0) {
            it->sc.never = strcmp(eq + 1, "-") == 0;
            if (!it->sc.never && parse_u64(eq + 1, &it->sc.at) != 0) {
                return -1;
            }
        } else if (strcmp(kv, "time") == 0 && parse_u64(eq + 1, &it->time) == 0) {
            it->have_time = 1;
        } else if (strcmp(kv, "again") == 0 && parse_u64(eq + 1, &it->again) == 0) {
            it->have_again = 1;
        } else {
            return -1;
        }
    }
    return have == 3 ? 0 : -1;
}

/* run one item on a fresh boot of the image; 0 on success, -1 on a harness fault (already reported on c) */
static int item_run(const char *rom, const char *request, const struct item *it, struct ctx *c)
{
    struct booted b;
    const struct ds_instance *inst = NULL;
    struct ds_instance *w;
    struct ds_run_result r;
    struct stop_ctx sc = it->sc;
    uint32_t reason;
    uint64_t guard = ds_max_step_mclk();
    const char *err = boot_built_rom(rom, request, &b, &inst);

    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    w = (struct ds_instance *)(void *)b.mem;
    reason = run_abs(w, it->until, &sc, &r);
    check(c, reason == it->result, "ds_run returned a different reason than expected");
    check(c, r.steps == it->steps, "ds_run took a different number of steps than the instruction cycle table gives");
    check(c, !it->have_time || (inst->machine_time == it->time && r.machine_time == it->time), "machine time after ds_run differs from the expected value");
    check(c, r.overshoot == (r.machine_time > it->until && r.steps > 0u ? r.machine_time - it->until : UINT64_C(0)), "the overshoot is not machine time minus the deadline");
    check(c, r.overshoot < guard, "the overshoot reached ds_max_step_mclk()");
    if (it->have_again) {
        unsigned char *copy = (unsigned char *)malloc(ds_instance_size());

        if (copy == NULL) {
            release_boot(&b);
            harness_error(c, "out of memory for the instance copy");
            return -1;
        }
        memcpy(copy, b.mem, ds_instance_size());
        reason = run_abs(w, it->again, NULL, &r);
        check(c, it->again <= inst->machine_time, "the case deadline of the second call is ahead of machine time");
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && r.steps == 0u && r.overshoot == 0u, "a deadline not ahead of machine time did not return until with 0 steps and overshoot 0");
        check(c, memcmp(copy, b.mem, ds_instance_size()) == 0, "a zero-step ds_run changed the instance (machine time, RAM or CPU state)");
        free(copy);
    }
    release_boot(&b);
    return 0;
}

void kind_run_until(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char rom[48];
    char *rest = list;
    char *text;
    unsigned n = 0u;

    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the expect list is too long");
        return;
    }
    text = pk_next_item(&rest, '|');
    if (text == NULL || strncmp(text, "rom=", 4u) != 0 || strlen(text + 4) == 0u || strlen(text + 4) >= sizeof rom
        || strspn(text + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(text + 4)) {
        harness_error(c, "the first item of a run_until expect is rom=NAME");
        return;
    }
    memcpy(rom, text + 4, strlen(text + 4) + 1u);
    while ((text = pk_next_item(&rest, '|')) != NULL && !c->harness) {
        struct item it;

        if (item_read(&it, text) != 0) {
            harness_error(c, "a run_until item is malformed");
            return;
        }
        if (item_run(rom, k->val[K_REQUEST], &it, c) != 0) {
            return;
        }
        n++;
    }
    check(c, n >= 1u, "the run_until case lists no item");
    snprintf(got, gotcap, "items=%u", n);
}

/* ---- kind run_chunk_sum ---- */

#define CS_LISTS_MAX 8u
#define CS_DONE_STATE_OFF 0x1F04u /* the DSV1 state byte of a guest (spinkinds.c) */

struct endstate {
    uint64_t time;
    uint64_t steps;
    uint64_t calls;
    uint8_t cpu[8];
    uint8_t digest[DS_DIGEST_BYTES];
    int done;
    int ok;
};

/* the chunk size of call number i of a list: a fixed size, or the rotation 1, 7, 168, 3072 (size 0 in the list means mix) */
static uint64_t chunk_size(uint64_t fixed, uint64_t i)
{
    static const uint64_t rot[4] = {1u, 7u, 168u, 3072u};

    return fixed != 0u ? fixed : rot[i % 4u];
}

/* boot a fresh instance and run it to until in calls of chunk (0 = mix); chunk == until runs in one call */
static const char *chunk_run(const char *rom, const char *request, uint64_t until, uint64_t chunk, int mix, struct endstate *e)
{
    struct booted b;
    const struct ds_instance *inst = NULL;
    struct ds_instance *w;
    struct ds_run_result r;
    uint64_t d = 0u;
    uint64_t i = 0u;
    const char *err = boot_built_rom(rom, request, &b, &inst);

    memset(e, 0, sizeof *e);
    if (err != NULL) {
        return err;
    }
    w = (struct ds_instance *)(void *)b.mem;
    while (d < until) {
        uint64_t step = chunk_size(mix ? 0u : chunk, i);

        d = until - d > step ? d + step : until;
        if (run_abs(w, d, NULL, &r) != (uint32_t)DS_RUN_REASON_UNTIL) {
            release_boot(&b);
            return "ds_run refused a chunk of the run";
        }
        e->steps += r.steps;
        e->calls++;
        i++;
    }
    e->time = inst->machine_time;
    e->cpu[0] = (uint8_t)(inst->pc & 0xFFu);
    e->cpu[1] = (uint8_t)(inst->pc >> 8);
    e->cpu[2] = inst->a;
    e->cpu[3] = inst->x;
    e->cpu[4] = inst->y;
    e->cpu[5] = inst->s;
    e->cpu[6] = inst->p;
    e->cpu[7] = inst->speed;
    e->done = inst->ram8k[CS_DONE_STATE_OFF] == 0xA5u;
    e->ok = ds_digest(inst, e->digest) == 0u;
    release_boot(&b);
    return NULL;
}

static int end_same(const struct endstate *a, const struct endstate *b)
{
    return a->ok && b->ok && a->time == b->time && a->steps == b->steps && memcmp(a->cpu, b->cpu, sizeof a->cpu) == 0
           && memcmp(a->digest, b->digest, DS_DIGEST_BYTES) == 0;
}

void kind_run_chunk_sum(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char rom[48];
    char chunks[PK_LIST_MAX];
    char *rest = list;
    char *kv;
    uint64_t until = 0u;
    int have = 0;
    struct endstate ref;
    unsigned n = 0u;
    const char *err;

    rom[0] = '\0';
    chunks[0] = '\0';
    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the expect list is too long");
        return;
    }
    while ((kv = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            harness_error(c, "a run_chunk_sum expect is rom=NAME;until=N;chunks=LIST");
            return;
        }
        *eq = '\0';
        if (strcmp(kv, "rom") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof rom
            && strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(eq + 1)) {
            memcpy(rom, eq + 1, strlen(eq + 1) + 1u);
            have |= 1;
        } else if (strcmp(kv, "until") == 0 && parse_u64(eq + 1, &until) == 0 && until > 0u) {
            have |= 2;
        } else if (strcmp(kv, "chunks") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof chunks) {
            memcpy(chunks, eq + 1, strlen(eq + 1) + 1u);
            have |= 4;
        } else {
            harness_error(c, "a run_chunk_sum expect is rom=NAME;until=N;chunks=LIST");
            return;
        }
    }
    if (have != 7) {
        harness_error(c, "a run_chunk_sum expect is rom=NAME;until=N;chunks=LIST");
        return;
    }
    err = chunk_run(rom, k->val[K_REQUEST], until, until, 0, &ref);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    check(c, ref.ok && ref.steps > 0u, "the reference run took no step or its digest was refused");
    check(c, ref.done, "the guest is not in its done state at the deadline: the chunk comparison would not be past done (D-95)");
    rest = chunks;
    while ((kv = pk_next_item(&rest, ',')) != NULL && !c->harness) {
        struct endstate e;
        uint64_t size = 0u;
        int mix = strcmp(kv, "mix") == 0;

        if (n >= CS_LISTS_MAX || (!mix && (parse_u64(kv, &size) != 0 || size == 0u))) {
            harness_error(c, "a run_chunk_sum chunk list item is not a size above 0 or mix, or there are too many");
            return;
        }
        err = chunk_run(rom, k->val[K_REQUEST], until, size, mix, &e);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        check(c, e.time == ref.time, "the machine time after the chunked run differs from the one-call run to the same absolute deadline");
        check(c, e.steps == ref.steps, "the total steps of the chunked run differ from the one-call run");
        check(c, end_same(&e, &ref), "the CPU registers or the digest after the chunked run differ from the one-call run");
        check(c, mix || size >= until || e.calls > 1u, "a chunked run made one call only: the list does not split the run");
        n++;
    }
    check(c, n >= 1u, "the run_chunk_sum case lists no chunk size");
    snprintf(got, gotcap, "lists=%u", n);
}
