/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ilvkinds.c - the interleave case kinds (plan 16, migrated to ds_run by plan 03-02; D-55, D-91, D-93, CORE-01): several instances live in one process,
 * are run in a fixed-seed pseudo-random order to absolute deadlines in master clocks, and every one of them must end in the state (digest and printed
 * record) that the same inputs give when the instance runs on its own. A hidden global in the core would couple the machines and move those digests.
 * A call that runs an instance to the next absolute deadline of its own schedule is one ds_run; the schedule numbers are master clocks, and a
 * deadline already behind the machine time (the previous call overshot) takes no step. Evidence tier E0: isolation and repeatability of ds_run, nothing about hardware. The blocks are allocated here, in test code, with
 * canary bytes around every instance; the core allocates nothing. Test code only, never installed.
 *   interleave                       request is A||B (two requests; A is the case fixture, B is the same fixture with its own len and
 *                                    seed); expect lcg=N;steps=N. A and B are stepped alternately on the LCG schedule, each beside a
 *                                    solo copy (A0, B0) that receives only its own calls; digests are compared at every eighth step and
 *                                    at the end, and the printed records at the end
 *   interleave_twins                 request is one request; expect lcg=N;total=N. Two instances with identical inputs are stepped
 *                                    interleaved, each in calls of different size but the same total, and must equal each other and a
 *                                    third instance that took the total in one call
 *   interleave_destroy_recreate      request A||B; expect lcg=N;steps=N;at=N. At step AT the block of B is filled with a stale pattern and
 *                                    B is booted again into it; B must equal its fresh digest, and after its earlier calls are applied
 *                                    again must equal the solo copy; the run then continues to the end
 *   interleave_aba_order             request A||B; expect lcg=N;t1=N;t2=N;t3=N. A takes t1, B takes t2, A takes t3; a reference A booted
 *                                    afterwards takes t1+t3 in other call sizes and must equal A, and a reference B equals B
 *   interleave_canaries              request A||B; expect lcg=N;steps=N. Three adjacent instances (A, B, A again) separated by canary
 *                                    bytes: no canary changes through boot, step, the discard of the neighbour and its reboot, and the
 *                                    canary check itself is shown to see a changed guard byte and a changed padding byte
 *   liveness_shared_state_selfcheck  as interleave; the same helper is run with the memory of B aliased to the memory of A and must
 *                                    report a difference, and without the alias must report none
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_serial.h"
#include "ds_text.h"
#include "runner.h"
#include "synth.h"

#define IL_GUARD 64u        /* bytes of canary on each side of an instance block (rounded up to the instance alignment) */
#define IL_SLOTS_MAX 4u
#define IL_STEPS_MAX 256u
#define IL_CHUNK_MOD 4093u  /* a call moves the deadline of an instance by 1 + (r mod IL_CHUNK_MOD) master clocks */

/* ---- the arena: aligned instance blocks separated by guard bytes ---- */

struct arena {
    unsigned char *raw;
    unsigned char *slot[IL_SLOTS_MAX];
    size_t n;
    size_t isz;   /* ds_instance_size() */
    size_t span;  /* the block of one instance: isz rounded up to the instance alignment; the tail past isz is padding and is canary too */
    size_t guard;
};

static size_t round_up(size_t v, size_t a)
{
    return ((v + a - 1u) / a) * a;
}

static unsigned char canary_at(size_t k, size_t region)
{
    return (unsigned char)(0xc3u ^ (unsigned)(k * 37u) ^ (unsigned)(region * 11u));
}

/* a region of the arena that no instance may write: guard G (0 .. n) or the tail padding of slot S (n + 1 + S) */
static unsigned char *region_ptr(const struct arena *a, size_t region, size_t *len)
{
    if (region <= a->n) {
        *len = a->guard;
        return region == 0u ? a->slot[0] - a->guard : a->slot[region - 1u] + a->span;
    }
    *len = a->span - a->isz;
    return a->slot[region - a->n - 1u] + a->isz;
}

static size_t region_count(const struct arena *a)
{
    return (a->n + 1u) + a->n;
}

static void arena_seal(const struct arena *a)
{
    size_t r;
    size_t k;

    for (r = 0u; r < region_count(a); r++) {
        size_t len;
        unsigned char *p = region_ptr(a, r, &len);

        for (k = 0u; k < len; k++) {
            p[k] = canary_at(k, r);
        }
    }
}

static int arena_intact(const struct arena *a)
{
    size_t r;
    size_t k;

    for (r = 0u; r < region_count(a); r++) {
        size_t len;
        const unsigned char *p = region_ptr(a, r, &len);

        for (k = 0u; k < len; k++) {
            if (p[k] != canary_at(k, r)) {
                return 0;
            }
        }
    }
    return 1;
}

static void arena_free(struct arena *a)
{
    free(a->raw);
    a->raw = NULL;
}

/* NULL on success, otherwise why not */
static const char *arena_new(struct arena *a, size_t n)
{
    size_t align = ds_instance_align();
    size_t total;
    size_t adj;
    size_t i;
    unsigned char *base;

    memset(a, 0, sizeof *a);
    if (align == 0u || n == 0u || n > IL_SLOTS_MAX) {
        return "the instance alignment is 0 or the slot count is out of range";
    }
    a->n = n;
    a->isz = ds_instance_size();
    a->span = round_up(a->isz, align);
    a->guard = round_up(IL_GUARD, align);
    total = n * a->span + (n + 1u) * a->guard + align;
    a->raw = (unsigned char *)malloc(total);
    if (a->raw == NULL) {
        return "out of memory for the instance arena";
    }
    adj = (align - ((size_t)(uintptr_t)a->raw % align)) % align;
    base = a->raw + adj;
    for (i = 0u; i < n; i++) {
        a->slot[i] = base + a->guard + i * (a->span + a->guard);
    }
    arena_seal(a);
    return NULL;
}

/* ---- the inputs of one instance and the boot into a slot ---- */

struct input {
    struct spec s;
    unsigned char *content;
};

static void input_free(struct input *in)
{
    free(in->content);
    in->content = NULL;
}

static const char *input_make(struct input *in, const char *fixture, const char *request)
{
    size_t natural = synth_size(fixture);
    const char *err;

    in->content = NULL;
    if (natural == 0u) {
        return "unknown fixture recipe";
    }
    err = parse_spec(&in->s, request, natural);
    if (err != NULL) {
        return err;
    }
    if (in->s.ptr != PTR_OK || in->s.len > CONTENT_LIMIT) {
        return "an interleave case needs ptr=ok and a len within the content limit";
    }
    in->content = (unsigned char *)malloc(in->s.len != 0u ? in->s.len : 1u);
    if (in->content == NULL) {
        return "out of memory for the content";
    }
    if (synth_fill(fixture, in->content, in->s.len) != 0) {
        input_free(in);
        return "unknown fixture recipe";
    }
    return NULL;
}

/* boot the inputs into slot i; fill >= 0 sets every byte of the block first, fill < 0 boots over whatever the block holds (stale state) */
static const char *slot_boot(const struct arena *a, void *mem, int fill, const struct input *in, struct ds_instance **out)
{
    struct ds_boot_request rq;
    struct ds_boot_result res;

    if (fill >= 0) {
        memset(mem, fill, a->isz);
    }
    fill_request(&rq, &in->s, in->content);
    memset(&res, 0xff, sizeof res);
    if (ds_boot(mem, a->isz, &rq, &res) != 0u || res.instance == NULL) {
        return "an interleave request was refused: the case needs accepted boots";
    }
    *out = res.instance;
    return NULL;
}

/* ---- the schedule and the observations ---- */

static uint64_t lcg_next(uint64_t *x)
{
    *x = *x * 6364136223846793005u + 1442695040888963407u;
    return *x >> 33;
}

static uint64_t chunk_of(uint64_t r)
{
    return 1u + (r >> 1) % IL_CHUNK_MOD;
}

struct dig {
    uint8_t d[DS_DIGEST_BYTES];
};

static int dig_of(const struct ds_instance *inst, struct dig *out)
{
    return ds_digest(inst, out->d) == 0u;
}

static int dig_eq(const struct dig *a, const struct dig *b)
{
    return memcmp(a->d, b->d, DS_DIGEST_BYTES) == 0;
}

/* the digests of two instances are equal (and both were read) */
static int same_digest(const struct ds_instance *x, const struct ds_instance *y)
{
    struct dig a;
    struct dig b;

    return dig_of(x, &a) && dig_of(y, &b) && dig_eq(&a, &b);
}

/* the printed records of two instances are equal byte for byte (and both were printed) */
static int same_record(const struct ds_instance *x, const struct ds_instance *y)
{
    char bx[REC_CAP];
    char by[REC_CAP];
    size_t lx = 0u;
    size_t ly = 0u;

    if (ds_text_record(x, bx, sizeof bx, &lx) != DS_TEXT_OK || ds_text_record(y, by, sizeof by, &ly) != DS_TEXT_OK) {
        return 0;
    }
    return lx == ly && lx > 0u && memcmp(bx, by, lx) == 0;
}

/* the params of an expect text: key=N items separated by ';'; 0 on success, -1 on a malformed item or a missing wanted key */
struct params {
    uint64_t lcg;
    uint64_t steps;
    uint64_t total;
    uint64_t at;
    uint64_t t1;
    uint64_t t2;
    uint64_t t3;
    int has_at;
};

static int params_read(const char *text, struct params *p)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    memset(p, 0, sizeof *p);
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        uint64_t v;

        if (eq == NULL || parse_u64(eq + 1, &v) != 0) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(item, "lcg") == 0) {
            p->lcg = v;
        } else if (strcmp(item, "steps") == 0) {
            p->steps = v;
        } else if (strcmp(item, "total") == 0) {
            p->total = v;
        } else if (strcmp(item, "at") == 0) {
            p->at = v;
            p->has_at = 1;
        } else if (strcmp(item, "t1") == 0) {
            p->t1 = v;
        } else if (strcmp(item, "t2") == 0) {
            p->t2 = v;
        } else if (strcmp(item, "t3") == 0) {
            p->t3 = v;
        } else {
            return -1;
        }
    }
    return 0;
}

/* split the request text A||B in two work buffers; NULL on success, otherwise why not */
static const char *request_pair(const char *text, char *a, char *b, size_t cap)
{
    const char *bar = strstr(text, "||");

    if (bar == NULL || (size_t)(bar - text) >= cap || strlen(bar + 2) >= cap) {
        return "an interleave request is two requests separated by ||";
    }
    memcpy(a, text, (size_t)(bar - text));
    a[bar - text] = '\0';
    memcpy(b, bar + 2, strlen(bar + 2) + 1u);
    return NULL;
}

/* the fixed-seed schedule: who steps (0 = A, 1 = B) and how far the call moves that instance's deadline, in master clocks, for n steps */
struct step {
    unsigned who;
    uint64_t mclk; /* how far this call moves the deadline of its instance, in master clocks */
};

static void schedule_make(struct step *s, unsigned n, uint64_t seed)
{
    uint64_t x = seed;
    unsigned i;

    for (i = 0u; i < n; i++) {
        uint64_t r = lcg_next(&x);

        s[i].who = (unsigned)(r & 1u);
        s[i].mclk = chunk_of(r);
    }
}

/* run an instance to one absolute deadline: true when ds_run took the request and ended on the deadline (one assertion's worth of fact) */
static int run_to(struct ds_instance *inst, uint64_t until)
{
    struct ds_run_request rq;
    struct ds_run_result out;

    memset(&rq, 0, sizeof rq);
    memset(&out, 0, sizeof out);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    return ds_run(inst, &rq, &out) == (uint32_t)DS_RUN_REASON_UNTIL;
}

/* the machine time is the first instruction boundary at or past the deadline: not before it, and less than one maximal step after it (D-94) */
static int at_boundary(const struct ds_instance *inst, uint64_t until)
{
    return inst->machine_time >= until && inst->machine_time - until < ds_max_step_mclk();
}

/* run an instance from the deadline from to the deadline to in calls of LCG-chosen size, each to its own absolute deadline; the number of calls (0 when a call was refused) */
static unsigned run_chunked(struct ds_instance *inst, uint64_t from, uint64_t to, uint64_t *x)
{
    uint64_t at = from;
    unsigned calls = 0u;

    while (at < to) {
        uint64_t n = chunk_of(lcg_next(x));

        if (n > to - at) {
            n = to - at;
        }
        at += n;
        if (!run_to(inst, at)) {
            return 0u;
        }
        calls++;
    }
    return calls;
}

/* ---- the A / B interleave against solo copies (kinds interleave and liveness_shared_state_selfcheck) ---- */

/*
 * Boot A and B (slots 0 and 1; with alias the B boot goes into the memory of A, so that the two pointers are equal) and the solo copies A0
 * and B0 (slots 2 and 3), then step. Every comparison is one assertion on c. NULL on success, otherwise a harness fault.
 */
static const char *ab_run(struct ctx *c, const char *fixture, const char *req_text, const struct params *p, int alias)
{
    char ra[PK_LIST_MAX];
    char rb[PK_LIST_MAX];
    struct input ia;
    struct input ib;
    struct arena ar;
    struct step sched[IL_STEPS_MAX];
    struct ds_instance *a = NULL;
    struct ds_instance *b = NULL;
    struct ds_instance *a0 = NULL;
    struct ds_instance *b0 = NULL;
    struct dig first_a;
    struct dig first_b;
    struct dig now;
    uint64_t tgt[2] = {0u, 0u};
    const char *err;
    unsigned i;

    ia.content = NULL;
    ib.content = NULL;
    if (p->steps == 0u || p->steps > IL_STEPS_MAX) {
        return "steps is 0 or above the schedule limit";
    }
    err = request_pair(req_text, ra, rb, sizeof ra);
    if (err == NULL) {
        err = input_make(&ia, fixture, ra);
    }
    if (err == NULL) {
        err = input_make(&ib, fixture, rb);
    }
    if (err == NULL) {
        err = arena_new(&ar, IL_SLOTS_MAX);
    }
    if (err != NULL) {
        input_free(&ia);
        input_free(&ib);
        return err;
    }
    err = slot_boot(&ar, ar.slot[0], 0x00, &ia, &a);
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[alias ? 0 : 1], 0xff, &ib, &b);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[2], 0x5a, &ia, &a0);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[3], 0xa5, &ib, &b0);
    }
    if (err != NULL) {
        arena_free(&ar);
        input_free(&ia);
        input_free(&ib);
        return err;
    }
    /* the two machines differ (content, seed), and each equals its solo copy at power-on */
    check(c, dig_of(a, &first_a) && dig_of(b, &first_b), "ds_digest refused a booted instance");
    check(c, !dig_eq(&first_a, &first_b), "A and B have the same digest at power-on: the case does not tell the machines apart");
    check(c, same_digest(a, a0) && same_digest(b, b0), "an instance differs from its solo copy at power-on");

    schedule_make(sched, (unsigned)p->steps, p->lcg);
    for (i = 0u; i < (unsigned)p->steps; i++) {
        struct ds_instance *live = sched[i].who == 0u ? a : b;
        struct ds_instance *solo = sched[i].who == 0u ? a0 : b0;

        tgt[sched[i].who] += sched[i].mclk;
        check(c, run_to(live, tgt[sched[i].who]) && run_to(solo, tgt[sched[i].who]), "ds_run did not end on the absolute deadline of the call");
        if (i % 8u == 7u) {
            check(c, same_digest(a, a0), "the digest of A differs from its solo run at a checkpoint");
            check(c, same_digest(b, b0), "the digest of B differs from its solo run at a checkpoint");
        }
    }
    check(c, same_digest(a, a0), "the final digest of A differs from its solo run");
    check(c, same_digest(b, b0), "the final digest of B differs from its solo run");
    check(c, same_record(a, a0), "the printed record of A differs from the record of its solo run");
    check(c, same_record(b, b0), "the printed record of B differs from the record of its solo run");
    check(c, at_boundary(a0, tgt[0]) && at_boundary(b0, tgt[1]), "a solo copy's machine time is not the first instruction boundary at or past its last deadline");
    check(c, dig_of(a0, &now) && !dig_eq(&now, &first_a), "the run did not change the digest of A: the schedule is vacuous");
    check(c, arena_intact(&ar), "a canary byte around the instances changed");
    arena_free(&ar);
    input_free(&ia);
    input_free(&ib);
    return NULL;
}

void kind_interleave(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct params p;
    const char *err;

    if (params_read(k->val[K_EXPECT], &p) != 0) {
        harness_error(c, "an interleave expect is lcg=N;steps=N");
        return;
    }
    err = ab_run(c, k->val[K_FIXTURE], k->val[K_REQUEST], &p, 0);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    snprintf(got, gotcap, "steps=%llu", (unsigned long long)p.steps);
}

void kind_liveness_shared_state_selfcheck(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct params p;
    struct ctx clean;
    struct ctx aliased;
    const char *err;

    if (params_read(k->val[K_EXPECT], &p) != 0) {
        harness_error(c, "a liveness expect is lcg=N;steps=N");
        return;
    }
    memset(&clean, 0, sizeof clean);
    memset(&aliased, 0, sizeof aliased);
    err = ab_run(&clean, k->val[K_FIXTURE], k->val[K_REQUEST], &p, 0);
    if (err == NULL) {
        err = ab_run(&aliased, k->val[K_FIXTURE], k->val[K_REQUEST], &p, 1);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    /* without the alias the helper sees no difference; with the second pointer equal to the first it must see one */
    check(c, clean.n > 0u && !clean.failed, "the interleave helper reported a difference for two separate instances");
    check(c, aliased.n > 0u && aliased.failed, "the interleave helper did not see two instances that share their memory");
    snprintf(got, gotcap, "aliased=%s", aliased.failed ? "seen" : "missed");
}

/* ---- twins ---- */

void kind_interleave_twins(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct params p;
    struct input in;
    struct arena ar;
    struct ds_instance *t[2] = {NULL, NULL};
    struct ds_instance *solo = NULL;
    struct dig fresh;
    struct dig now;
    uint64_t left[2];
    unsigned calls[2] = {0u, 0u};
    uint64_t x;
    const char *err;

    if (params_read(k->val[K_EXPECT], &p) != 0 || p.total == 0u || p.total > UINT64_C(0xFFFFFFFF)) {
        harness_error(c, "an interleave_twins expect is lcg=N;total=N with total from 1 to 2^32-1 master clocks");
        return;
    }
    err = input_make(&in, k->val[K_FIXTURE], k->val[K_REQUEST]);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    err = arena_new(&ar, 3u);
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[0], 0x11, &in, &t[0]);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[1], 0xee, &in, &t[1]);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[2], 0x00, &in, &solo);
    }
    if (err != NULL) {
        harness_error(c, err);
        arena_free(&ar);
        input_free(&in);
        return;
    }
    check(c, dig_of(t[0], &fresh) && same_digest(t[0], t[1]) && same_digest(t[0], solo), "identical inputs gave different digests at power-on");
    left[0] = p.total;
    left[1] = p.total;
    x = p.lcg;
    while (left[0] + left[1] > 0u) {
        uint64_t r = lcg_next(&x);
        unsigned who = (unsigned)(r & 1u);
        uint64_t n = chunk_of(r);

        if (left[who] == 0u) {
            who ^= 1u;
        }
        if (n > left[who]) {
            n = left[who];
        }
        left[who] -= n;
        check(c, run_to(t[who], p.total - left[who]), "ds_run did not end on the absolute deadline of a twin's call");
        calls[who]++;
    }
    check(c, run_to(solo, p.total), "ds_run did not end on the absolute deadline of the one-call solo run");
    check(c, calls[0] != calls[1], "the twins took the same number of calls: the call sizes do not differ");
    check(c, at_boundary(t[0], p.total) && at_boundary(t[1], p.total) && at_boundary(solo, p.total), "a twin's machine time is not the first instruction boundary at or past the total");
    check(c, same_digest(t[0], t[1]), "the two twins ended with different digests");
    check(c, same_digest(t[0], solo), "a twin differs from the instance that took the total in one call");
    check(c, same_record(t[0], t[1]) && same_record(t[0], solo), "the printed records of the twins differ");
    check(c, dig_of(solo, &now) && !dig_eq(&now, &fresh), "the run did not change the digest: the run is vacuous");
    check(c, arena_intact(&ar), "a canary byte around the instances changed");
    snprintf(got, gotcap, "calls=%u,%u", calls[0], calls[1]);
    arena_free(&ar);
    input_free(&in);
}

/* ---- destroy and recreate ---- */

void kind_interleave_destroy_recreate(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char ra[PK_LIST_MAX];
    char rb[PK_LIST_MAX];
    struct params p;
    struct input ia;
    struct input ib;
    struct arena ar;
    struct step sched[IL_STEPS_MAX];
    struct ds_instance *a = NULL;
    struct ds_instance *b = NULL;
    struct ds_instance *a0 = NULL;
    struct ds_instance *b0 = NULL;
    struct dig fresh_b;
    uint64_t tgt[2] = {0u, 0u};
    struct dig now;
    const char *err;
    unsigned i;
    unsigned at;
    size_t j;

    if (params_read(k->val[K_EXPECT], &p) != 0 || p.steps < 2u || p.steps > IL_STEPS_MAX || !p.has_at || p.at == 0u || p.at >= p.steps) {
        harness_error(c, "an interleave_destroy_recreate expect is lcg=N;steps=N;at=N with 0 < at < steps");
        return;
    }
    at = (unsigned)p.at;
    ia.content = NULL;
    ib.content = NULL;
    err = request_pair(k->val[K_REQUEST], ra, rb, sizeof ra);
    if (err == NULL) {
        err = input_make(&ia, k->val[K_FIXTURE], ra);
    }
    if (err == NULL) {
        err = input_make(&ib, k->val[K_FIXTURE], rb);
    }
    if (err == NULL) {
        err = arena_new(&ar, IL_SLOTS_MAX);
    }
    if (err != NULL) {
        input_free(&ia);
        input_free(&ib);
        harness_error(c, err);
        return;
    }
    err = slot_boot(&ar, ar.slot[0], 0x00, &ia, &a);
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[1], 0xff, &ib, &b);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[2], 0x5a, &ia, &a0);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[3], 0xa5, &ib, &b0);
    }
    if (err != NULL) {
        arena_free(&ar);
        input_free(&ia);
        input_free(&ib);
        harness_error(c, err);
        return;
    }
    check(c, dig_of(b, &fresh_b) && same_digest(a, a0) && same_digest(b, b0), "an instance differs from its solo copy at power-on");
    schedule_make(sched, (unsigned)p.steps, p.lcg);
    for (i = 0u; i < (unsigned)p.steps; i++) {
        struct ds_instance *live = sched[i].who == 0u ? a : b;
        struct ds_instance *solo = sched[i].who == 0u ? a0 : b0;

        if (i == at) {
            unsigned q;
            uint64_t tb = 0u;

            check(c, same_digest(b, b0), "B differs from its solo copy before it is discarded");
            /* the discard: the block of B takes a stale pattern, then B is booted again over it */
            for (j = 0u; j < ar.isz; j++) {
                ar.slot[1][j] = (unsigned char)(0xdbu ^ (unsigned)(j * 131u));
            }
            check(c, same_digest(a, a0), "A changed when its neighbour was discarded");
            err = slot_boot(&ar, ar.slot[1], -1, &ib, &b);
            if (err != NULL) {
                harness_error(c, err);
                break;
            }
            check(c, dig_of(b, &now) && dig_eq(&now, &fresh_b), "B booted over stale bytes differs from its fresh boot");
            for (q = 0u; q < at; q++) {
                if (sched[q].who == 1u) {
                    tb += sched[q].mclk;
                    check(c, run_to(b, tb), "ds_run did not end on the absolute deadline of a replayed call");
                }
            }
            check(c, same_digest(b, b0), "B differs from its solo copy after its earlier calls were applied again");
        }
        tgt[sched[i].who] += sched[i].mclk;
        check(c, run_to(live, tgt[sched[i].who]) && run_to(solo, tgt[sched[i].who]), "ds_run did not end on the absolute deadline of the call");
    }
    if (!c->harness) {
        check(c, same_digest(a, a0), "the final digest of A differs from an uninterrupted run");
        check(c, same_digest(b, b0), "the final digest of B differs from an uninterrupted run");
        check(c, same_record(a, a0) && same_record(b, b0), "a printed record differs from the record of the uninterrupted run");
        check(c, arena_intact(&ar), "a canary byte around the instances changed");
    }
    snprintf(got, gotcap, "at=%u", at);
    arena_free(&ar);
    input_free(&ia);
    input_free(&ib);
}

/* ---- A, then B, then A again ---- */

void kind_interleave_aba_order(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char ra[PK_LIST_MAX];
    char rb[PK_LIST_MAX];
    struct params p;
    struct input ia;
    struct input ib;
    struct arena ar;
    struct ds_instance *a = NULL;
    struct ds_instance *b = NULL;
    struct ds_instance *a_ref = NULL;
    struct ds_instance *b_ref = NULL;
    uint64_t x;
    const char *err;
    unsigned calls = 0u;

    if (params_read(k->val[K_EXPECT], &p) != 0 || p.t1 == 0u || p.t2 == 0u || p.t3 == 0u) {
        harness_error(c, "an interleave_aba_order expect is lcg=N;t1=N;t2=N;t3=N");
        return;
    }
    ia.content = NULL;
    ib.content = NULL;
    err = request_pair(k->val[K_REQUEST], ra, rb, sizeof ra);
    if (err == NULL) {
        err = input_make(&ia, k->val[K_FIXTURE], ra);
    }
    if (err == NULL) {
        err = input_make(&ib, k->val[K_FIXTURE], rb);
    }
    if (err == NULL) {
        err = arena_new(&ar, IL_SLOTS_MAX);
    }
    if (err != NULL) {
        input_free(&ia);
        input_free(&ib);
        harness_error(c, err);
        return;
    }
    x = p.lcg;
    /* the order: A takes t1, then B is booted and takes t2, then A takes t3; only afterwards are the references booted and run */
    err = slot_boot(&ar, ar.slot[0], 0x00, &ia, &a);
    if (err == NULL) {
        calls += run_chunked(a, 0u, p.t1, &x);
        err = slot_boot(&ar, ar.slot[1], 0xff, &ib, &b);
    }
    if (err == NULL) {
        calls += run_chunked(b, 0u, p.t2, &x);
        calls += run_chunked(a, p.t1, p.t1 + p.t3, &x);
        err = slot_boot(&ar, ar.slot[2], 0x5a, &ia, &a_ref);
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[3], 0xa5, &ib, &b_ref);
    }
    if (err != NULL) {
        arena_free(&ar);
        input_free(&ia);
        input_free(&ib);
        harness_error(c, err);
        return;
    }
    x = p.lcg ^ 0x9e3779b97f4a7c15u; /* the references reach the same deadlines in other call sizes */
    check(c, run_chunked(a_ref, 0u, p.t1 + p.t3, &x) > 0u, "ds_run did not end on the absolute deadline of a call of the reference A");
    check(c, run_chunked(b_ref, 0u, p.t2, &x) > 0u, "ds_run did not end on the absolute deadline of a call of the reference B");
    check(c, at_boundary(a, p.t1 + p.t3) && at_boundary(b, p.t2), "a machine time is not the first instruction boundary at or past the last deadline of the instance");
    check(c, same_digest(a, a_ref), "A after A, B, A differs from an uninterrupted A");
    check(c, same_digest(b, b_ref), "B differs from the B that ran after everything else");
    check(c, same_record(a, a_ref) && same_record(b, b_ref), "a printed record differs from the record of the reference run");
    check(c, !same_digest(a, b), "A and B end with the same digest: the case does not tell them apart");
    check(c, calls > 3u, "the order case made too few calls to be an interleave");
    check(c, arena_intact(&ar), "a canary byte around the instances changed");
    snprintf(got, gotcap, "calls=%u", calls);
    arena_free(&ar);
    input_free(&ia);
    input_free(&ib);
}

/* ---- canaries ---- */

void kind_interleave_canaries(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char ra[PK_LIST_MAX];
    char rb[PK_LIST_MAX];
    struct params p;
    struct input ia;
    struct input ib;
    struct arena ar;
    struct step sched[IL_STEPS_MAX];
    struct ds_instance *inst[3] = {NULL, NULL, NULL};
    struct dig before;
    struct dig after;
    uint64_t tgt[3] = {0u, 0u, 0u};
    const char *err;
    unsigned i;
    size_t j;
    size_t len;
    unsigned char *g;
    unsigned char keep;

    if (params_read(k->val[K_EXPECT], &p) != 0 || p.steps == 0u || p.steps > IL_STEPS_MAX) {
        harness_error(c, "an interleave_canaries expect is lcg=N;steps=N");
        return;
    }
    ia.content = NULL;
    ib.content = NULL;
    err = request_pair(k->val[K_REQUEST], ra, rb, sizeof ra);
    if (err == NULL) {
        err = input_make(&ia, k->val[K_FIXTURE], ra);
    }
    if (err == NULL) {
        err = input_make(&ib, k->val[K_FIXTURE], rb);
    }
    if (err == NULL) {
        err = arena_new(&ar, 3u);
    }
    if (err != NULL) {
        input_free(&ia);
        input_free(&ib);
        harness_error(c, err);
        return;
    }
    check(c, arena_intact(&ar), "the arena is not intact before any instance is booted");
    /* the canary check sees a changed guard byte and a changed padding byte, then agrees again when they are put back */
    g = region_ptr(&ar, 1u, &len);
    keep = g[len - 1u];
    g[len - 1u] = (unsigned char)(keep ^ 0x01u);
    check(c, !arena_intact(&ar), "the canary check did not see a changed guard byte");
    g[len - 1u] = keep;
    check(c, arena_intact(&ar), "the canary check disagrees after the guard byte was put back");
    if (ar.span > ar.isz) {
        g = region_ptr(&ar, ar.n + 1u, &len);
        keep = g[0];
        g[0] = (unsigned char)(keep ^ 0x80u);
        check(c, !arena_intact(&ar), "the canary check did not see a changed padding byte");
        g[0] = keep;
    }
    err = slot_boot(&ar, ar.slot[0], 0x00, &ia, &inst[0]);
    check(c, err == NULL && arena_intact(&ar), "a canary byte changed in the boot of the first instance");
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[1], 0xff, &ib, &inst[1]);
        check(c, err == NULL && arena_intact(&ar), "a canary byte changed in the boot of the second instance");
    }
    if (err == NULL) {
        err = slot_boot(&ar, ar.slot[2], 0x33, &ia, &inst[2]);
        check(c, err == NULL && arena_intact(&ar), "a canary byte changed in the boot of the third instance");
    }
    if (err != NULL) {
        arena_free(&ar);
        input_free(&ia);
        input_free(&ib);
        harness_error(c, err);
        return;
    }
    schedule_make(sched, (unsigned)p.steps, p.lcg);
    for (i = 0u; i < (unsigned)p.steps; i++) {
        unsigned which = sched[i].who == 0u ? 0u : (i % 3u == 0u ? 2u : 1u); /* the middle block is stepped from both sides */

        tgt[which] += sched[i].mclk;
        check(c, run_to(inst[which], tgt[which]), "ds_run did not end on the absolute deadline of the call");
        if (i % 8u == 7u) {
            check(c, arena_intact(&ar), "a canary byte changed while the instances were stepped");
        }
    }
    check(c, arena_intact(&ar), "a canary byte changed through the runs");
    /* the discard of the middle neighbour: its block takes a stale pattern, the others and the canaries must not move, and a reboot is clean too */
    check(c, dig_of(inst[0], &before), "ds_digest refused the first instance");
    for (j = 0u; j < ar.isz; j++) {
        ar.slot[1][j] = (unsigned char)(0xdbu ^ (unsigned)(j * 131u));
    }
    check(c, arena_intact(&ar), "a canary byte changed when the neighbour was discarded");
    check(c, dig_of(inst[0], &after) && dig_eq(&before, &after), "the state of the first instance changed when its neighbour was discarded");
    err = slot_boot(&ar, ar.slot[1], -1, &ib, &inst[1]);
    check(c, err == NULL && arena_intact(&ar), "a canary byte changed in the reboot of the discarded instance");
    check(c, dig_of(inst[0], &after) && dig_eq(&before, &after), "the state of the first instance changed in the reboot of its neighbour");
    snprintf(got, gotcap, "regions=%u", (unsigned)region_count(&ar));
    arena_free(&ar);
    input_free(&ia);
    input_free(&ib);
}
