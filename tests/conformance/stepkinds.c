/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * stepkinds.c - the case kinds of the run API (plan 03-02; D-93, D-94, D-95). Test code only. Evidence tier E0: repeatability of the run API and
 * the boundary behaviour of its absolute deadline; nothing here is an accuracy, compatibility or hardware claim.
 * Amendment record (plan 03-02): D-68 and D-75 are SUPERSEDED by D-91. The placeholder stepping API (its advance and budgeted-run functions, its per-call cap and
 * the file src/core/advance.c) and the Phase 2 kinds that used it (advance, run_budget) are gone: every kind that steps the machine runs ds_run to an
 * absolute until_mclk, never a relative budget, so run(N) then run(M) is run(N+M) by construction. the two placeholder registers of the
 * first phases left the digest in plan 03-03 and the instance in plan 03-36. The timeout-budget mini-root keeps its expect text
 * budget=N,step=M: it now means absolute deadlines in chunks of step master clocks (guestkinds.c).
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
 *   run_refuse      expect is rom=NAME;refuse=CODE where CODE is a name of run_refusals.def (plan 03-35; D-93, F4). On a fresh boot of the built image that has
 *                   run to a nonzero machine time, ds_run is called with each malformed request of that CODE (a table of variants in this file). Every call must
 *                   return REFUSED with refusal run.CODE, take 0 steps, report overshoot 0 and leave the whole instance, digest and raw bytes, unchanged; a
 *                   second call with a NULL result must return REFUSED too. The codes every row observed are kept for run_refusal_coverage.
 *   run_overshoot   expect is roms=LIST;calls=N;span=N;seed=N;bound=N (plan 03-09; D-94, CORE-04). LIST is comma-separated: an authored image name (ds-proto) or a synthetic recipe (plain-256k, plain-384k).
 *                   Each ROM is booted once and run by N calls; call i goes to the absolute deadline machine time + 1 (an even call, which takes exactly one instruction) or machine time + 1 +
 *                   (r mod span) (an odd call, r from a fixed generator seeded with seed). Every call must end on an instruction boundary at or past its deadline with overshoot = time - until,
 *                   a single step no longer than ds_max_step_mclk(), and every overshoot below it; the largest overshoot seen must be above 0 (a case that never overshoots proves nothing). bound=N is
 *                   the value ds_max_step_mclk() must have: a literal from the manual (17 cycles of a block transfer plus the 8 of an interrupt entry, times 12), so that a change of the derivation shows here.
 *   run_refusal_coverage  DEFERRED (D-86 pattern, evaluated after the case loop): every member of run_refusals.def must have been observed by a run_refuse row.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
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
    note_run(c, inst->machine_time, reason);
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

/* ---- kind run_overshoot ---- */

#define OV_ROMS_MAX 4u
#define OV_CALLS_MAX 256u

/* a fixed 64-bit generator for the deadlines (test code only; the values carry no meaning) */
static uint64_t ov_next(uint64_t *x)
{
    *x = *x * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
    return *x >> 33;
}

/* boot one ROM: a synthetic recipe (plain-*) through boot_accepted, an authored image through boot_built_rom */
static const char *ov_boot(const char *rom, const char *request, struct booted *b, const struct ds_instance **inst)
{
    struct spec s;

    if (strncmp(rom, "plain-", 6u) == 0) {
        return boot_accepted(rom, request, b, &s, inst);
    }
    return boot_built_rom(rom, request, b, inst);
}

static void ov_rom(const char *rom, const char *request, uint64_t calls, uint64_t span, uint64_t seed, uint64_t guard, uint64_t *maxo, struct ctx *c)
{
    struct booted b;
    const struct ds_instance *inst = NULL;
    struct ds_instance *w;
    struct ds_run_result r;
    uint64_t x = seed;
    uint64_t i;
    const char *err = ov_boot(rom, request, &b, &inst);

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    for (i = 0u; i < calls; i++) {
        uint64_t before = inst->machine_time;
        uint64_t until = before + UINT64_C(1) + ((i & 1u) != 0u ? ov_next(&x) % span : UINT64_C(0));
        uint32_t reason = run_abs(w, until, NULL, &r);

        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && r.steps >= 1u, "ds_run did not take a step to a deadline ahead of machine time");
        check(c, inst->machine_time >= until && r.machine_time == inst->machine_time, "ds_run ended before its deadline");
        check(c, r.overshoot == inst->machine_time - until, "the overshoot is not machine time minus the deadline");
        check(c, r.overshoot < guard, "the overshoot reached ds_max_step_mclk()");
        if ((i & 1u) == 0u) {
            check(c, r.steps == 1u && inst->machine_time - before <= guard, "a single step took longer than ds_max_step_mclk()");
        }
        if (r.overshoot > *maxo) {
            *maxo = r.overshoot;
        }
    }
    note_run(c, inst->machine_time, (uint32_t)DS_RUN_REASON_UNTIL);
    release_boot(&b);
}

void kind_run_overshoot(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char roms[PK_LIST_MAX];
    char *rest = list;
    char *kv;
    uint64_t calls = 0u;
    uint64_t span = 0u;
    uint64_t seed = 0u;
    uint64_t bound = 0u;
    uint64_t maxo = 0u;
    uint64_t guard = ds_max_step_mclk();
    int have = 0;
    unsigned n = 0u;

    roms[0] = '\0';
    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the expect list is too long");
        return;
    }
    while ((kv = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            harness_error(c, "a run_overshoot expect is roms=LIST;calls=N;span=N;seed=N;bound=N");
            return;
        }
        *eq = '\0';
        if (strcmp(kv, "roms") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof roms) {
            memcpy(roms, eq + 1, strlen(eq + 1) + 1u);
            have |= 1;
        } else if (strcmp(kv, "calls") == 0 && parse_u64(eq + 1, &calls) == 0 && calls > 0u && calls <= OV_CALLS_MAX) {
            have |= 2;
        } else if (strcmp(kv, "span") == 0 && parse_u64(eq + 1, &span) == 0 && span > 0u) {
            have |= 4;
        } else if (strcmp(kv, "seed") == 0 && parse_u64(eq + 1, &seed) == 0) {
            have |= 8;
        } else if (strcmp(kv, "bound") == 0 && parse_u64(eq + 1, &bound) == 0 && bound > 0u) {
            have |= 16;
        } else {
            harness_error(c, "a run_overshoot expect is roms=LIST;calls=N;span=N;seed=N;bound=N");
            return;
        }
    }
    if (have != 31) {
        harness_error(c, "a run_overshoot expect is roms=LIST;calls=N;span=N;seed=N;bound=N");
        return;
    }
    check(c, guard == bound, "ds_max_step_mclk() is not the value the manual gives for the longest opcode plus the interrupt entry");
    rest = roms;
    while ((kv = pk_next_item(&rest, ',')) != NULL && !c->harness) {
        if (n >= OV_ROMS_MAX || strlen(kv) == 0u || strspn(kv, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(kv)) {
            harness_error(c, "a run_overshoot rom name is [a-z0-9-] and there are at most four");
            return;
        }
        ov_rom(kv, k->val[K_REQUEST], calls, span, seed + (uint64_t)n, guard, &maxo, c);
        n++;
    }
    check(c, n >= 3u, "the run_overshoot case names fewer than three ROMs");
    check(c, maxo > 0u, "no call overshot its deadline: the case proves nothing about the bound");
    snprintf(got, gotcap, "roms=%u maxo=%llu", n, (unsigned long long)maxo);
}

/* ---- kind run_chunk_sum ---- */

#define CS_LISTS_MAX 8u
#define CS_DONE_STATE_OFF 0x1F04u /* the DSV1 state byte of a guest (guestkinds.c) */

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
    note_run(c, ref.time, (uint32_t)DS_RUN_REASON_UNTIL);
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

/* ---- kind run_refuse ---- */

#define RR_SINK_BYTES 64u
#define RR_REC_RECORDS 4u
#define RR_MACHINE_TIME_FIRST 600u /* the boot is run to this deadline first, so the instance is not at its boot state */

static unsigned char rr_observed[DS_RUNREF_SLOTS];
static unsigned char rr_sink_a[RR_SINK_BYTES]; /* valid buffers that overlap nothing: the refusal must not depend on a bad pointer (W6) */
static unsigned char rr_sink_b[RR_SINK_BYTES];
/* a valid instruction sink (plan 03-10): the struct and 8-byte aligned record memory of RR_REC_RECORDS records, overlapping nothing */
static struct ds_insn_sink rr_insn;
static union {
    uint64_t align;
    unsigned char b[RR_REC_RECORDS * DS_INSN_RECORD_BYTES];
} rr_rec;

/*
 * One refused call on instance arg (the live block w, or NULL) with request rq (or NULL): the refusal, the result fields and the unchanged instance. res is the result struct of the first call;
 * with again set the call is repeated with a NULL result (a refusal that does not depend on the result struct does not change).
 */
static void rr_call_ex(struct ctx *c, struct ds_instance *w, const struct ds_instance *inst, struct ds_instance *arg, const struct ds_run_request *rq, uint32_t code, struct ds_run_result *rp,
                       int again)
{
    struct ds_run_result res_local;
    struct ds_run_result *res = rp != NULL ? rp : &res_local;
    uint8_t before[DS_DIGEST_BYTES];
    uint8_t after[DS_DIGEST_BYTES];
    size_t size = ds_instance_size();
    unsigned char *copy = (unsigned char *)malloc(size);
    uint64_t now = inst->machine_time;
    const char *name = ds_run_refusal_name(code);
    uint32_t reason;

    if (copy == NULL) {
        harness_error(c, "out of memory for the instance copy");
        return;
    }
    memcpy(copy, w, size);
    check(c, ds_digest(inst, before) == 0u, "the digest of the instance before the refused call was refused");
    memset(res, 0xAA, sizeof *res);
    reason = ds_run(arg, rq, res);
    check(c, reason == (uint32_t)DS_RUN_REASON_REFUSED && res->reason == (uint32_t)DS_RUN_REASON_REFUSED, "a malformed run request was not refused");
    check(c, res->refusal == code, "the run refusal code differs from the one the row names");
    check(c, name != NULL && strncmp(name, "run.", 4u) == 0, "a run refusal does not print as run.NAME");
    check(c, res->steps == 0u && res->overshoot == 0u && res->pad == 0u && res->stop_point == (uint32_t)DS_STOP_INSTRUCTION, "a refused call reported steps, overshoot or a stop point");
    check(c, res->machine_time == (arg != NULL ? now : UINT64_C(0)), "a refused call reported a machine time other than the instance's");
    if (res->refusal < (uint32_t)DS_RUNREF_SLOTS) {
        rr_observed[res->refusal] = 1u;
    }
    if (again) {
        check(c, ds_run(arg, rq, NULL) == (uint32_t)DS_RUN_REASON_REFUSED, "a refused call with a NULL result did not return REFUSED");
    }
    check(c, memcmp(copy, w, size) == 0, "a refused ds_run changed the instance bytes");
    check(c, ds_digest(inst, after) == 0u && memcmp(before, after, DS_DIGEST_BYTES) == 0, "a refused ds_run changed the instance digest");
    free(copy);
}

static void rr_call(struct ctx *c, struct ds_instance *w, const struct ds_instance *inst, struct ds_instance *arg, const struct ds_run_request *rq, uint32_t code)
{
    rr_call_ex(c, w, inst, arg, rq, code, NULL, 1);
}

static void rr_base_request(struct ds_run_request *rq, uint64_t until)
{
    memset(rq, 0, sizeof *rq);
    rq->struct_size = (uint32_t)sizeof *rq;
    rq->until_mclk = until;
}

/* a call that must NOT be refused (the edge of the time_wrap rule): the reason and 0 steps, and the instance unchanged */
static void rr_accept(struct ctx *c, struct ds_instance *w, const struct ds_instance *inst, const struct ds_run_request *rq, uint32_t reason)
{
    struct ds_run_result res;
    size_t size = ds_instance_size();
    unsigned char *copy = (unsigned char *)malloc(size);

    if (copy == NULL) {
        harness_error(c, "out of memory for the instance copy");
        return;
    }
    memcpy(copy, w, size);
    memset(&res, 0xAA, sizeof res);
    check(c, ds_run(w, rq, &res) == reason && res.reason == reason && res.refusal == (uint32_t)DS_RUNREF_NONE, "a request at the edge of the wrap rule got the wrong reason or was refused");
    check(c, res.steps == 0u && res.overshoot == 0u, "a request at the edge of the wrap rule took a step");
    check(c, memcmp(copy, w, size) == 0 && res.machine_time == inst->machine_time, "a zero-step call at the edge of the wrap rule changed the instance");
    free(copy);
}

/* the variants of one code; the base request is a valid one that would run the machine if it were accepted */
static void rr_variants(struct ctx *c, const char *code_name, struct ds_instance *w, const struct ds_instance *inst, unsigned *calls)
{
    struct ds_run_request rq;
    uint64_t until = inst->machine_time + UINT64_C(600);

    if (strcmp(code_name, "struct_size") == 0) {
        const uint32_t size[5] = {(uint32_t)sizeof rq - 1u, (uint32_t)sizeof rq + 1u, (uint32_t)sizeof rq + 4u, 0u, 0xFFFFFFFFu};
        unsigned i;

        for (i = 0u; i < 5u; i++) {
            rr_base_request(&rq, until);
            rq.struct_size = size[i];
            rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_struct_size);
            (*calls)++;
        }
    } else if (strcmp(code_name, "instance_null") == 0) {
        rr_base_request(&rq, until);
        rr_call(c, w, inst, NULL, &rq, (uint32_t)DS_RUNREF_instance_null);
        (*calls)++;
    } else if (strcmp(code_name, "request_null") == 0) {
        rr_call(c, w, inst, w, NULL, (uint32_t)DS_RUNREF_request_null);
        (*calls)++;
    } else if (strcmp(code_name, "reserved") == 0) {
        const uint32_t value[3] = {1u, 0x80000000u, 0xFFFFFFFFu};
        unsigned i;

        for (i = 0u; i < 3u; i++) {
            rr_base_request(&rq, until);
            rq.reserved = value[i];
            rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_reserved);
            (*calls)++;
        }
    } else if (strcmp(code_name, "sink_unsupported") == 0) {
        unsigned i;

        /* the dev sink alone, and the dev sink beside a valid insn sink (W6): each points at a valid buffer that overlaps neither the instance nor the request. The insn sink alone is accepted since plan 03-10. */
        rr_insn.records = rr_rec.b;
        rr_insn.capacity = RR_REC_RECORDS;
        for (i = 0u; i < 2u; i++) {
            rr_base_request(&rq, until);
            rq.insn = i != 0u ? &rr_insn : NULL;
            rq.dev = (void *)rr_sink_b;
            rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_sink_unsupported);
            (*calls)++;
        }
        (void)rr_sink_a;
    } else if (strcmp(code_name, "memory_invalid") == 0) {
        /* an insn sink whose memory the call owns or cannot use is refused with the instance byte-identical (plan 03-10; D-93, D-119, I4) */
        struct ds_insn_sink sink;
        struct ds_run_result own;
        uintptr_t in_ram = ((uintptr_t)w->ram8k + (uintptr_t)(DS_INSN_ALIGN - 1u)) & ~(uintptr_t)(DS_INSN_ALIGN - 1u);
        uintptr_t in_rom = ((uintptr_t)w->rom + (uintptr_t)(DS_INSN_ALIGN - 1u)) & ~(uintptr_t)(DS_INSN_ALIGN - 1u);
        unsigned char saved[sizeof sink];

        /* 1: record memory inside the instance (the work RAM), 2: inside the instance's copy of the content (the machine's ROM image: the host content buffer itself is not known to ds_run), 3: over the request */
        memset(&sink, 0, sizeof sink);
        sink.capacity = 4u;
        sink.records = (uint8_t *)in_ram;
        rr_base_request(&rq, until);
        rq.insn = &sink;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        sink.records = (uint8_t *)in_rom;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        sink.records = (uint8_t *)(void *)&rq;
        sink.capacity = 2u;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        /* 4: the record memory is the sink struct itself (header overlap), 5: the sink struct lies inside the instance (its bytes are restored afterwards) */
        sink.records = (uint8_t *)(void *)&sink;
        sink.capacity = 1u;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        memcpy(saved, w->ram8k + (in_ram - (uintptr_t)w->ram8k), sizeof saved);
        sink.records = rr_rec.b;
        sink.capacity = RR_REC_RECORDS;
        memcpy(w->ram8k + (in_ram - (uintptr_t)w->ram8k), &sink, sizeof sink);
        rq.insn = (struct ds_insn_sink *)in_ram;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        memcpy(w->ram8k + (in_ram - (uintptr_t)w->ram8k), saved, sizeof saved);
        rq.insn = &sink;
        /* 6: a misaligned record pointer, 7: a NULL record pointer with a nonzero capacity, 8: a record range that would wrap the address space */
        sink.records = rr_rec.b + 4;
        sink.capacity = 1u;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        sink.records = NULL;
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        sink.records = (uint8_t *)(UINTPTR_MAX & ~(uintptr_t)(DS_INSN_ALIGN - 1u));
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid);
        /* 9: record memory over the result struct (a call with a result, so the second call with a NULL result is skipped) */
        sink.records = (uint8_t *)(void *)&own;
        sink.capacity = 2u;
        rr_call_ex(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_memory_invalid, &own, 0);
        (*calls) += 9u;
    } else if (strcmp(code_name, "time_wrap") == 0) {
        uint64_t guard = ds_max_step_mclk();
        uint64_t saved = w->machine_time;
        struct stop_ctx now = {0, 0u};

        /* a deadline within one maximum step of 2^64 (a step could end past it and wrap machine time: Pitfall 20) */
        rr_base_request(&rq, UINT64_MAX);
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_time_wrap);
        rr_base_request(&rq, UINT64_MAX - guard + UINT64_C(1));
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_time_wrap);
        /* ... and machine time within one maximum step of 2^64, with an ordinary deadline */
        w->machine_time = UINT64_MAX - guard + UINT64_C(1);
        rr_base_request(&rq, 0u);
        rr_call(c, w, inst, w, &rq, (uint32_t)DS_RUNREF_time_wrap);
        (*calls) += 3u;
        /* the edge itself is not refused: a deadline of 2^64 - 1 - guard with a stop that holds returns CALLBACK before any step, and a machine time of 2^64 - 1 - guard with a deadline of 0 returns UNTIL */
        w->machine_time = saved;
        rr_base_request(&rq, UINT64_MAX - guard);
        rq.stop = stop_at;
        rq.stop_ctx = &now;
        rr_accept(c, w, inst, &rq, (uint32_t)DS_RUN_REASON_CALLBACK);
        w->machine_time = UINT64_MAX - guard;
        rr_base_request(&rq, 0u);
        rr_accept(c, w, inst, &rq, (uint32_t)DS_RUN_REASON_UNTIL);
        w->machine_time = saved;
    } else {
        harness_error(c, "the run_refuse code is not one of run_refusals.def");
    }
}

void kind_run_refuse(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char rom[48];
    char code[32];
    char *rest = list;
    char *kv;
    int have = 0;
    struct booted b;
    const struct ds_instance *inst = NULL;
    struct ds_run_result r;
    unsigned calls = 0u;
    const char *err;

    rom[0] = '\0';
    code[0] = '\0';
    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the expect list is too long");
        return;
    }
    while ((kv = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            harness_error(c, "a run_refuse expect is rom=NAME;refuse=CODE");
            return;
        }
        *eq = '\0';
        if (strcmp(kv, "rom") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof rom
            && strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(eq + 1)) {
            memcpy(rom, eq + 1, strlen(eq + 1) + 1u);
            have |= 1;
        } else if (strcmp(kv, "refuse") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof code
                   && strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789_") == strlen(eq + 1)) {
            memcpy(code, eq + 1, strlen(eq + 1) + 1u);
            have |= 2;
        } else {
            harness_error(c, "a run_refuse expect is rom=NAME;refuse=CODE");
            return;
        }
    }
    if (have != 3) {
        harness_error(c, "a run_refuse expect is rom=NAME;refuse=CODE");
        return;
    }
    err = boot_built_rom(rom, k->val[K_REQUEST], &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    memset(&r, 0, sizeof r);
    {
        struct ds_run_request first;
        uint32_t reason;

        rr_base_request(&first, RR_MACHINE_TIME_FIRST);
        reason = ds_run((struct ds_instance *)(void *)b.mem, &first, &r);
        note_run(c, inst->machine_time, reason);
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && inst->machine_time >= RR_MACHINE_TIME_FIRST,
              "the valid first run of the run_refuse boot did not reach its deadline");
    }
    rr_variants(c, code, (struct ds_instance *)(void *)b.mem, inst, &calls);
    check(c, calls >= 1u, "the run_refuse case made no refused call");
    release_boot(&b);
    snprintf(got, gotcap, "calls=%u", calls);
}

/* ---- kind run_refusal_coverage (deferred, D-86 pattern) ---- */

void kind_run_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    uint32_t n;
    unsigned members = 0u;
    unsigned seen = 0u;

    (void)k;
    for (n = 1u; n < (uint32_t)DS_RUNREF_SLOTS; n++) {
        const char *name = ds_run_refusal_name(n);

        if (name == NULL) {
            continue;
        }
        members++;
        seen += rr_observed[n] != 0u ? 1u : 0u;
        check(c, rr_observed[n] != 0u, "a run refusal code was never observed by a run_refuse row");
        if (rr_observed[n] == 0u) {
            emit_err("run_refusal_coverage: %s was not observed by any run_refuse row\n", name);
        }
    }
    check(c, members > 0u, "run_refusals.def has no member");
    snprintf(got, gotcap, "members=%u observed=%u", members, seen);
}
