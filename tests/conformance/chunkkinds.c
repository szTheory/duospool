/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * chunkkinds.c - kind chunk_equiv (plan 03-26; D-92, D-93, D-95, D-119, D-127, CORE-04, ER-7): a fixture (or a random program) is run to one fixed absolute deadline past its done state, once in a single call (the uninterrupted run) and then,
 * on fresh boots, in calls whose deadlines follow each schedule of the case; every schedule must end with the machine time, the step count, the verdict RAM (the DSV1 block), the digest and the sequence of step boundaries of the uninterrupted run.
 * The schedules are named by the case key chunks (a comma list): 1, 7, 168 and 3072 are fixed sizes in master clocks (deadlines j * size), frame is the fixed size 357368 (a frame-sized chunk, labelled a fixed size because the machine has no frame yet),
 * and tick-1 and tick+1 are the deadlines 3072 * j - 1 and 3072 * j + 1, one master clock either side of a timer prescaler step (3072 master clocks); the schedule 3072 is the deadlines on the steps themselves (3072k), so the three ticks are tick-1, 3072 and tick+1.
 * The case key input (plan 03-29) names an input script whose events are pushed to every run before its first step, like the irq_stim list.
 * The step boundary sequence is the stop sequence of the kind: a stop callback that never holds is called before the first step and after every step, and folds the machine time, the registers, the bus latch and the block transfer state at each distinct
 * machine time, so a run that loses or repeats part of a transfer at a stop, or reaches the same end by another path, differs even when its last state is equal. The ordered stop reasons are checked too: every call ends on its deadline (UNTIL), and
 * the calls that take a step and end inside a block transfer (stop point block_byte) are counted per schedule: the case items block-stop (at least one per schedule) and pending-stop (at least one with the timer interrupt and IRQ2 both pending at the stop
 * stamp, from the pure query ds_irq_pending_query, so the stop really is a transfer with interrupts pending) say what a case requires. The case's irq_stim is applied to every boot. A snapshot does not exist yet (plan 03-30), so equality here is of outputs, machine time,
 * stop sequence and digests. Evidence tier E0: the core compared with itself (repeatability and consistency, ER-7); a pass is not a support claim and no literal comes from a DuoSpool run.
 *   expect   compare=verdict,machine_time,stops,digest (the four things compared, each once), then optionally block-stop, pending-stop, random=N (N programs of randprog.c, 1 to 64, instead of a fixture)
 *   fixture  authored:NAME (the built image) or none (with random=N);  until_mclk the deadline;  chunks the schedule list;  irq_stim the declared test stimulus
 * Test code only, never installed.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_serial.h"
#include "runner.h"

#define CS_SCHED_MAX 8u
#define CS_FNV_BASIS UINT64_C(14695981039346656037)
#define CS_FNV_PRIME UINT64_C(1099511628211)
#define CS_PENDING_BOTH 5u /* the request register bits of the timer interrupt (bit 2) and IRQ2 (bit 0) */

struct cs_sched {
    const char *name;
    uint64_t period;
    int delta; /* deadlines are period * j + delta */
};

static const struct cs_sched cs_table[] = {
    {"1", 1u, 0},   {"7", 7u, 0},          {"168", 168u, 0},  {"3072", 3072u, 0}, /* 3072 is also the schedule on the prescaler steps (3072k) */
    {"frame", 357368u, 0},                                                         /* a fixed size of one frame: the machine has no frame yet */
    {"tick-1", 3072u, -1}, {"tick+1", 3072u, 1},
};
#define CS_TABLE_N (sizeof cs_table / sizeof cs_table[0])

/* what the stop callback saw: the step boundary sequence (a fold, and its length) and the first time the guest's done state was seen */
struct cs_seen {
    int have_last;
    uint64_t last;
    uint64_t n;
    uint64_t fold;
    int done;
    uint64_t done_at;
};

static void cs_fold(uint64_t *f, uint64_t v)
{
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        *f = (*f ^ ((v >> (8u * i)) & UINT64_C(0xFF))) * CS_FNV_PRIME;
    }
}

/* the stop callback: it never holds; at each machine time it has not yet seen it folds the state that a chunking could disturb (the time, pc, A, X, Y, S, P, the speed, the bus latch and the whole block transfer state) */
static int cs_stop(const struct ds_instance *inst, void *ctx)
{
    struct cs_seen *s = (struct cs_seen *)ctx;

    if (s->have_last && s->last == inst->machine_time) {
        return 0;
    }
    s->have_last = 1;
    s->last = inst->machine_time;
    s->n++;
    cs_fold(&s->fold, inst->machine_time);
    cs_fold(&s->fold, ((uint64_t)inst->pc << 32) | ((uint64_t)inst->a << 24) | ((uint64_t)inst->x << 16) | ((uint64_t)inst->y << 8) | (uint64_t)inst->s);
    cs_fold(&s->fold, ((uint64_t)inst->p << 16) | ((uint64_t)inst->speed << 8) | (uint64_t)inst->io_buffer);
    cs_fold(&s->fold, ((uint64_t)inst->blk_active << 40) | ((uint64_t)inst->blk_opcode << 32) | ((uint64_t)inst->blk_toggle << 24) | (uint64_t)inst->blk_remaining);
    cs_fold(&s->fold, ((uint64_t)inst->blk_src << 16) | (uint64_t)inst->blk_dst);
    if (!s->done && dsv1_done(inst, NULL)) {
        s->done = 1;
        s->done_at = inst->machine_time;
    }
    return 0;
}

/* where a run's program comes from: a built authored image, or the random program of an index */
struct cs_prog {
    char rom[48];
    int random;
    unsigned index;
    unsigned char *img; /* the image of the random program, built once for all the runs of the program */
};

/* the observable outcome of one run */
struct cs_out {
    uint64_t time;
    uint64_t steps;
    uint64_t calls;
    uint64_t bstops;   /* calls that took a step and ended inside a block transfer */
    uint64_t pstops;   /* those of them with the timer interrupt and IRQ2 both pending at the stop stamp */
    uint64_t stop_n;   /* step boundaries seen */
    uint64_t stop_fold;
    int done;
    uint64_t done_at;
    int bad;           /* a call was refused or did not end on its deadline */
    int digest_ok;
    uint8_t verdict[V_SIZE];
    uint8_t digest[DS_DIGEST_BYTES];
};

/* the deadline of call j (from 1) of a schedule, clipped to the end of the run; a NULL schedule is the one call to the end */
static uint64_t cs_deadline(const struct cs_sched *s, uint64_t j, uint64_t until)
{
    uint64_t d;

    if (s == NULL) {
        return until;
    }
    d = s->period * j;
    d = s->delta < 0 ? d - 1u : d + (uint64_t)s->delta;
    return d < until ? d : until;
}

/* boot the program, apply the case's stimulus, run it on the schedule: 0 on success, -1 after a harness error */
static int cs_exec(struct ctx *c, const struct kase *k, const struct cs_prog *pg, uint64_t until, const struct cs_sched *sc, struct cs_out *o)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct cs_seen seen;
    uint32_t refusal = 0u;
    uint32_t input_refusal = 0u;
    uint64_t j = 0u;
    uint64_t d = 0u;
    const char *err = pg->random ? boot_random(pg->img, k->val[K_REQUEST], &b, &ci) : boot_built_rom(pg->rom, k->val[K_REQUEST], &b, &ci);

    memset(o, 0, sizeof *o);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    w = (struct ds_instance *)(void *)b.mem;
    err = apply_irq_stim(w, k, &refusal);
    if (err != NULL || refusal != 0u) {
        release_boot(&b);
        harness_error(c, err != NULL ? err : "ds_irq_stim_set refused the irq_stim list of the case");
        return -1;
    }
    err = apply_input(w, k, &input_refusal); /* the declared input script of the case, pushed before the first step of every run and every schedule (plan 03-29; D-117, D-118): the input is part of the machine, so the chunking must not move it */
    if (err != NULL || input_refusal != 0u) {
        release_boot(&b);
        harness_error(c, err != NULL ? err : "ds_input_push refused the input script of the case");
        return -1;
    }
    check(c, input_applied(k, w), "the instance does not hold the events of the declared input script: the runs would be compared without the input");
    memset(&seen, 0, sizeof seen);
    seen.fold = CS_FNV_BASIS;
    while (d < until) {
        j++;
        d = cs_deadline(sc, j, until);
        if (d <= ci->machine_time && d < until) {
            continue; /* a deadline the previous call overshot takes no step (the contract of ds_run, checked once after the loop): the next call is the first deadline of the schedule that is ahead, so a chunk of one clock costs one call per step and not one per clock */
        }
        memset(&rq, 0, sizeof rq);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = d;
        rq.stop = cs_stop;
        rq.stop_ctx = &seen;
        memset(&res, 0, sizeof res);
        if (ds_run(w, &rq, &res) != (uint32_t)DS_RUN_REASON_UNTIL || res.reason != (uint32_t)DS_RUN_REASON_UNTIL) {
            o->bad = 1;
            break;
        }
        o->steps += res.steps;
        o->calls++;
        if (res.steps > 0u && res.stop_point == (uint32_t)DS_STOP_BLOCK_BYTE) {
            o->bstops++;
            if (ds_irq_pending_query(ci, ci->machine_time) == CS_PENDING_BOTH) {
                o->pstops++;
            }
        }
    }
    if (!o->bad) { /* a deadline behind the machine time returns UNTIL with zero steps and changes nothing */
        uint64_t before = ci->machine_time;

        memset(&rq, 0, sizeof rq);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until > 0u ? until - 1u : 0u;
        rq.stop = cs_stop;
        rq.stop_ctx = &seen;
        memset(&res, 0, sizeof res);
        if (ds_run(w, &rq, &res) != (uint32_t)DS_RUN_REASON_UNTIL || res.steps != 0u || ci->machine_time != before) {
            o->bad = 1;
        }
    }
    o->time = ci->machine_time;
    o->stop_n = seen.n;
    o->stop_fold = seen.fold;
    o->done = seen.done;
    o->done_at = seen.done_at;
    memcpy(o->verdict, ci->ram8k + V_OFF, V_SIZE);
    o->digest_ok = ds_digest(ci, o->digest) == 0u;
    release_boot(&b);
    return 0;
}

struct cs_items {
    int block_stop;
    int pending_stop;
    unsigned random; /* 0 = a fixture */
};

/* expect = compare=verdict,machine_time,stops,digest, then block-stop, pending-stop and random=N, each at most once; 0 on success */
static int cs_parse_expect(const char *text, struct cs_items *it)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned seen = 0u;
    int first = 1;

    memset(it, 0, sizeof *it);
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        uint64_t v = 0u;

        if (first) {
            char *r2;
            char *w;

            first = 0;
            if (strncmp(item, "compare=", 8u) != 0) {
                return -1;
            }
            r2 = item + 8;
            while ((w = pk_next_item(&r2, ',')) != NULL) {
                unsigned bit = strcmp(w, "verdict") == 0 ? 1u : strcmp(w, "machine_time") == 0 ? 2u : strcmp(w, "stops") == 0 ? 4u : strcmp(w, "digest") == 0 ? 8u : 0u;

                if (bit == 0u || (seen & bit) != 0u) {
                    return -1;
                }
                seen |= bit;
            }
        } else if (strcmp(item, "block-stop") == 0 && !it->block_stop) {
            it->block_stop = 1;
        } else if (strcmp(item, "pending-stop") == 0 && !it->pending_stop) {
            it->pending_stop = 1;
        } else if (strncmp(item, "random=", 7u) == 0 && it->random == 0u && parse_u64(item + 7, &v) == 0 && v >= 1u && v <= 64u) {
            it->random = (unsigned)v;
        } else {
            return -1;
        }
    }
    return seen == 15u ? 0 : -1;
}

/* the chunks key: schedule names from cs_table, each once, at most CS_SCHED_MAX */
static int cs_parse_scheds(const char *text, const struct cs_sched **out, unsigned *n)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    *n = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        size_t i;
        unsigned q;

        for (i = 0u; i < CS_TABLE_N && strcmp(item, cs_table[i].name) != 0; i++) {
        }
        if (i == CS_TABLE_N || *n >= CS_SCHED_MAX) {
            return -1;
        }
        for (q = 0u; q < *n; q++) {
            if (out[q] == &cs_table[i]) {
                return -1;
            }
        }
        out[(*n)++] = &cs_table[i];
    }
    return *n >= 1u ? 0 : -1;
}

/* a schedule's run against the uninterrupted run of the same program */
static void cs_compare(struct ctx *c, const struct cs_out *ref, const struct cs_out *o, const struct cs_sched *sc, const struct cs_items *it, const char *what)
{
    char msg[200];

    snprintf(msg, sizeof msg, "%s: a call of the schedule %s was refused, did not end on its deadline, or the call to a deadline behind the machine time took a step", what, sc->name);
    check(c, !o->bad, msg);
    snprintf(msg, sizeof msg, "%s: the machine time or the step count under the schedule %s differs from the uninterrupted run to the same absolute deadline", what, sc->name);
    check(c, o->time == ref->time && o->steps == ref->steps, msg);
    snprintf(msg, sizeof msg, "%s: the verdict RAM under the schedule %s differs from the uninterrupted run", what, sc->name);
    check(c, memcmp(o->verdict, ref->verdict, V_SIZE) == 0, msg);
    snprintf(msg, sizeof msg, "%s: the digest under the schedule %s differs from the uninterrupted run", what, sc->name);
    check(c, o->digest_ok && ref->digest_ok && memcmp(o->digest, ref->digest, DS_DIGEST_BYTES) == 0, msg);
    snprintf(msg, sizeof msg, "%s: the step boundary sequence under the schedule %s differs from the uninterrupted run (stop sequence)", what, sc->name);
    check(c, o->stop_n == ref->stop_n && o->stop_fold == ref->stop_fold, msg);
    snprintf(msg, sizeof msg, "%s: the schedule %s made one call only: it does not split the run", what, sc->name);
    check(c, o->calls > 1u, msg);
    if (it->block_stop || it->pending_stop) {
        snprintf(msg, sizeof msg, "%s: no call of the schedule %s took a step and ended inside a block transfer (no block byte stop)", what, sc->name);
        check(c, o->bstops > 0u, msg);
    }
    if (it->pending_stop) {
        snprintf(msg, sizeof msg, "%s: no block byte stop of the schedule %s had the timer interrupt and IRQ2 both pending at the stop stamp (ds_irq_pending_query)", what, sc->name);
        check(c, o->pstops > 0u, msg);
    }
}

void kind_chunk_equiv(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct cs_items it;
    const struct cs_sched *sc[CS_SCHED_MAX];
    unsigned nsc = 0u;
    uint64_t until = 0u;
    struct cs_prog pg;
    unsigned nprog;
    unsigned pi;
    unsigned runs = 0u;
    uint64_t blocks = 0u;
    uint64_t pends = 0u;
    uint64_t done_at = 0u;
    int fixture = 0;

    memset(&pg, 0, sizeof pg);
    if (cs_parse_expect(k->val[K_EXPECT], &it) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || !k->has[K_CHUNKS]
        || cs_parse_scheds(k->val[K_CHUNKS], sc, &nsc) != 0) {
        harness_error(c, "a chunk_equiv case needs expect = compare=verdict,machine_time,stops,digest[;block-stop][;pending-stop][;random=N], until_mclk and chunks = a list of 1, 7, 168, 3072, frame, tick-1, tick+1 (each once)");
        return;
    }
    if (it.random != 0u) {
        pg.random = 1;
        nprog = it.random;
    } else if (authored_name(k->val[K_FIXTURE], pg.rom, sizeof pg.rom) == 0) {
        nprog = 1u;
        fixture = 1;
    } else {
        harness_error(c, "a chunk_equiv case needs fixture = authored:NAME, or fixture = none with random=N in its expect");
        return;
    }
    if (pg.random) {
        pg.img = (unsigned char *)malloc(RP_IMAGE_BYTES);
        if (pg.img == NULL) {
            harness_error(c, "out of memory for the random program image");
            return;
        }
    }
    for (pi = 0u; pi < nprog && !c->harness; pi++) {
        struct cs_out ref;
        unsigned si;
        char what[40];

        pg.index = pi;
        if (pg.random) {
            rp_build(pg.img, rp_seed(pi));
            snprintf(what, sizeof what, "random program %u", pi);
        } else {
            snprintf(what, sizeof what, "%s", pg.rom);
        }
        if (cs_exec(c, k, &pg, until, NULL, &ref) != 0) {
            break;
        }
        runs++;
        if (pi == 0u) {
            note_run(c, ref.time, (uint32_t)DS_RUN_REASON_UNTIL);
        }
        check(c, !ref.bad && ref.digest_ok && ref.steps > 4u, "the uninterrupted run took four steps or fewer, ended off its deadline, took a step to a deadline behind the machine time or its digest was refused");
        if (fixture) {
            check(c, ref.done && ref.done_at < until, "the guest is not in its done state before the deadline: the chunk comparison would not be past done (D-95)");
            done_at = ref.done_at;
        }
        for (si = 0u; si < nsc && !c->harness; si++) {
            struct cs_out o;

            if (cs_exec(c, k, &pg, until, sc[si], &o) != 0) {
                break;
            }
            runs++;
            cs_compare(c, &ref, &o, sc[si], &it, what);
            blocks += o.bstops;
            pends += o.pstops;
        }
    }
    free(pg.img);
    snprintf(got, gotcap, "runs=%u bs=%llu ps=%llu done=%llu", runs, (unsigned long long)blocks, (unsigned long long)pends, (unsigned long long)done_at);
}
