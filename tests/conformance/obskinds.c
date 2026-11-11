/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * obskinds.c - the observer kinds (plan 03-10; D-119, D-127, D-131, T-03-29, T-03-30). Test code only. Evidence class E0: every comparison here is of the core with itself (repeatability and
 * non-interference); none is an oracle, and nothing here is an accuracy, compatibility or hardware claim.
 *   insn_sink_shapes  expect is rom=NAME;caps=N,N,...  (NAME an authored image, each N a record capacity; the case's until_mclk is the absolute deadline). One run of ROM with no sink is the reference:
 *                     machine time, steps, overshoot and the digest of the whole instance. A second boot is stepped one instruction at a time with ds_cpu_step, and the state before each instruction is
 *                     kept. Then, for each capacity N and twice (a clean sink, and a sink whose header and record memory are poisoned with $A5, and, for N = 0, a non-NULL record pointer), the ROM is
 *                     booted again and run with an insn sink of that capacity: the run must give exactly the reference result and digest (the sink changes nothing), the header must read
 *                     count = min(steps, N), lost = steps - count, seq = steps (decoded by hand, little-endian), every stored record must equal the kept pre-instruction state byte for byte (decoded by
 *                     hand, the seven spare bytes zero), and the bytes of the record memory past the count and the guard bytes around it must still hold their poison (the core wrote nothing else, and
 *                     never read the header: a poisoned header gives the same result). A capacity of 0 and of 1, a small capacity that drops (drop-newest, lost counted) and a large one that does not are the shapes.
 *   opcode_exercise   a deferred kind (evaluated after the case loop, the D-86 pattern, so its RESULT line comes before RUNNER_DONE). It runs the ROM of every case of kind guest once (one run per distinct
 *                     ROM and request, to the case's until_mclk or the DSV1 done state, whichever comes first) with an insn sink of 262144 records and collects the set of opcodes the records show as executed.
 *                     The defined opcodes of the core's table (ds_cpu_op_defined: 234 rows, checked against the manual transcription by scripts/opcode-check) that no fixture executed are written to the
 *                     evidence file of the case, sixteen to a line. expect is mode=report (the case passes and records the count: the inventory is a measurement) or mode=require;exclude=0xNN,0xNN,...
 *                     (every unexercised defined opcode not in the exclude list is a guest_failure; plan 03-14 switches the case to require). A sink that lost a record (lost != 0) is harness_failure,
 *                     because the inventory would be incomplete: the log is evidence here (D-119). E0: the inventory says which opcodes the fixtures execute, nothing about what they check.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_serial.h"
#include "runner.h"

#define OBS_GUARD 64u
#define OBS_CAPS_MAX 8u
#define OBS_POISON 0xA5u

/* the state before one instruction, kept by the stepwise run (the independent view of what each record must hold) */
struct pre {
    uint64_t stamp;
    uint16_t pc;
    uint8_t opcode;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t p;
    uint8_t speed;
};

static uint32_t get_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u);
}

static uint32_t get_u32(const uint8_t *p)
{
    return get_u16(p) | (get_u16(p + 2) << 16u);
}

static uint64_t get_u64(const uint8_t *p)
{
    return (uint64_t)get_u32(p) | ((uint64_t)get_u32(p + 4) << 32u);
}

/* the end state of one run: what a sink must not change */
struct end {
    uint32_t reason;
    uint64_t mclk;
    uint64_t steps;
    uint64_t overshoot;
    uint8_t digest[DS_DIGEST_BYTES];
    int digest_ok;
};

static void run_once(struct ds_instance *w, uint64_t until, struct ds_insn_sink *sink, struct end *e)
{
    struct ds_run_request rq;
    struct ds_run_result res;

    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.insn = sink;
    e->reason = ds_run(w, &rq, &res);
    e->mclk = res.machine_time;
    e->steps = res.steps;
    e->overshoot = res.overshoot;
    e->digest_ok = ds_digest(w, e->digest) == 0u;
}

static int same_end(const struct end *a, const struct end *b)
{
    return a->reason == b->reason && a->mclk == b->mclk && a->steps == b->steps && a->overshoot == b->overshoot && a->digest_ok && b->digest_ok && memcmp(a->digest, b->digest, DS_DIGEST_BYTES) == 0;
}

/* the caps list of the expect: rom=NAME;caps=N,N,... (0 on success) */
static int parse_shapes(const char *text, char *rom, size_t romcap, uint64_t *caps, unsigned *ncaps)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have = 0;

    *ncaps = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "rom=", 4u) == 0 && strlen(item + 4) > 0u && strlen(item + 4) < romcap && strspn(item + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 4)) {
            memcpy(rom, item + 4, strlen(item + 4) + 1u);
            have |= 1;
        } else if (strncmp(item, "caps=", 5u) == 0) {
            char *cr = item + 5;
            char *tok;

            while ((tok = pk_next_item(&cr, ',')) != NULL) {
                if (*ncaps >= OBS_CAPS_MAX || parse_u64(tok, &caps[*ncaps]) != 0 || caps[*ncaps] > 1048576u) {
                    return -1;
                }
                (*ncaps)++;
            }
            have |= 2;
        } else {
            return -1;
        }
    }
    return have == 3 && *ncaps >= 1u ? 0 : -1;
}

/* the stepwise view: boot ROM again and keep the state before each of the first limit instructions up to the deadline; the number of instructions taken is returned in *steps */
static const char *step_through(const char *rom, const char *request, uint64_t until, struct pre *pre, uint64_t limit, uint64_t *steps)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    const char *err = boot_built_rom(rom, request, &b, &ci);

    if (err != NULL) {
        return err;
    }
    w = (struct ds_instance *)(void *)b.mem;
    *steps = 0u;
    while (w->machine_time < until) {
        if (*steps < limit) {
            struct pre *p = &pre[*steps];

            p->stamp = w->machine_time;
            p->pc = w->pc;
            p->opcode = ds_bus_read(w, w->pc);
            p->a = w->a;
            p->x = w->x;
            p->y = w->y;
            p->s = w->s;
            p->p = w->p;
            p->speed = w->speed;
        }
        w->machine_time = w->machine_time + (uint64_t)ds_cpu_step(w);
        (*steps)++;
    }
    release_boot(&b);
    return NULL;
}

/* one shape: capacity cap, poisoned or clean; the run must equal the reference and the sink must hold exactly what the stepwise view predicts */
static void one_shape(struct ctx *c, const char *rom, const char *request, uint64_t until, uint64_t cap, int poisoned, const struct end *ref, const struct pre *pre, uint64_t nsteps)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct end e;
    struct ds_insn_sink *sink;
    unsigned char *block;
    unsigned char *rec;
    size_t rec_bytes = (size_t)cap * (size_t)DS_INSN_RECORD_BYTES;
    size_t total = 2u * OBS_GUARD + rec_bytes;
    uint64_t want_count = nsteps < cap ? nsteps : cap;
    uint64_t i;
    size_t j;
    int clean_tail = 1;
    int guards = 1;
    int recs_ok = 1;
    const char *err = boot_built_rom(rom, request, &b, &ci);
    char msg[160];

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    sink = (struct ds_insn_sink *)calloc(1u, sizeof *sink);
    block = (unsigned char *)malloc(total != 0u ? total : 1u);
    if (sink == NULL || block == NULL) {
        free(sink);
        free(block);
        release_boot(&b);
        harness_error(c, "out of memory for the sink");
        return;
    }
    memset(block, poisoned ? (int)OBS_POISON : 0, total);
    rec = block + OBS_GUARD;
    memset(sink->header, poisoned ? (int)OBS_POISON : 0, sizeof sink->header);
    sink->records = (cap == 0u && !poisoned) ? NULL : rec;
    sink->capacity = (uint32_t)cap;
    run_once((struct ds_instance *)(void *)b.mem, until, sink, &e);
    snprintf(msg, sizeof msg, "a sink of capacity %llu (%s) changed the end state or the digest of the run", (unsigned long long)cap, poisoned ? "poisoned" : "clean");
    check(c, same_end(&e, ref), msg);
    snprintf(msg, sizeof msg, "the header of a sink of capacity %llu (%s) does not read count = min(steps, capacity), lost = steps - count, seq = steps", (unsigned long long)cap,
            poisoned ? "poisoned" : "clean");
    check(c, (uint64_t)get_u32(sink->header) == want_count && (uint64_t)get_u32(sink->header + 4) == nsteps - want_count && get_u64(sink->header + 8) == nsteps, msg);
    for (i = 0u; i < want_count; i++) {
        const unsigned char *r = rec + (size_t)i * DS_INSN_RECORD_BYTES;
        const struct pre *p = &pre[i];
        int ok = get_u64(r) == p->stamp && get_u16(r + 8) == (uint32_t)p->pc && r[10] == p->opcode && r[11] == p->a && r[12] == p->x && r[13] == p->y && r[14] == p->s && r[15] == p->p
                 && r[16] == p->speed;

        for (j = 17u; j < DS_INSN_RECORD_BYTES; j++) {
            ok = ok && r[j] == 0u;
        }
        recs_ok = recs_ok && ok;
    }
    snprintf(msg, sizeof msg, "a stored record of the sink of capacity %llu (%s) differs from the state before its instruction", (unsigned long long)cap, poisoned ? "poisoned" : "clean");
    check(c, recs_ok, msg);
    for (j = (size_t)want_count * DS_INSN_RECORD_BYTES; j < rec_bytes; j++) {
        clean_tail = clean_tail && rec[j] == (unsigned char)(poisoned ? OBS_POISON : 0u);
    }
    for (j = 0u; j < OBS_GUARD; j++) {
        guards = guards && block[j] == (unsigned char)(poisoned ? OBS_POISON : 0u) && block[OBS_GUARD + rec_bytes + j] == (unsigned char)(poisoned ? OBS_POISON : 0u);
    }
    snprintf(msg, sizeof msg, "the core wrote into the record memory past the count or into the guard bytes (capacity %llu, %s)", (unsigned long long)cap, poisoned ? "poisoned" : "clean");
    check(c, clean_tail && guards, msg);
    free(sink);
    free(block);
    release_boot(&b);
}

void kind_insn_sink_shapes(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    uint64_t caps[OBS_CAPS_MAX];
    unsigned ncaps = 0u;
    uint64_t until = 0u;
    uint64_t maxcap = 0u;
    uint64_t nsteps = 0u;
    unsigned n;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct end ref;
    struct pre *pre;
    const char *err;

    if (parse_shapes(k->val[K_EXPECT], rom, sizeof rom, caps, &ncaps) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an insn_sink_shapes case needs expect = rom=NAME;caps=N,N,... (at most 8 capacities, each at most 1048576) and an until_mclk");
        return;
    }
    for (n = 0u; n < ncaps; n++) {
        maxcap = caps[n] > maxcap ? caps[n] : maxcap;
    }
    err = boot_built_rom(rom, k->val[K_REQUEST], &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    run_once((struct ds_instance *)(void *)b.mem, until, NULL, &ref);
    note_run(c, ref.mclk, ref.reason);
    release_boot(&b);
    check(c, ref.reason == (uint32_t)DS_RUN_REASON_UNTIL && ref.steps > 4u && ref.digest_ok, "the reference run without a sink did not reach its deadline with more than four steps");
    if (c->failed) {
        return;
    }
    pre = (struct pre *)malloc((size_t)(maxcap != 0u ? maxcap : 1u) * sizeof *pre);
    if (pre == NULL) {
        harness_error(c, "out of memory for the stepwise view");
        return;
    }
    err = step_through(rom, k->val[K_REQUEST], until, pre, maxcap, &nsteps);
    if (err != NULL) {
        free(pre);
        harness_error(c, err);
        return;
    }
    check(c, nsteps == ref.steps, "the stepwise run took a different number of instructions than the run without a sink");
    for (n = 0u; n < ncaps && !c->harness; n++) {
        one_shape(c, rom, k->val[K_REQUEST], until, caps[n], 0, &ref, pre, nsteps);
        one_shape(c, rom, k->val[K_REQUEST], until, caps[n], 1, &ref, pre, nsteps);
    }
    free(pre);
    snprintf(got, gotcap, "steps=%llu shapes=%u", (unsigned long long)nsteps, ncaps * 2u);
}

/* ---- kind opcode_exercise (deferred) ---- */

#define OX_SINK_RECORDS 262144u
#define OX_ROMS_MAX 32u
#define OX_PER_LINE 16u

struct ox_rom {
    char name[48];
    const char *request;
    uint64_t until;
};

/* the distinct (ROM, request) pairs of the guest cases, in case order; the number found is returned, or -1 for more than OX_ROMS_MAX */
static int ox_collect(struct ox_rom *roms)
{
    size_t i;
    unsigned n = 0u;

    for (i = 0u; i < case_count(); i++) {
        const struct kase *g = case_get(i);
        struct ox_rom r;
        unsigned j;
        int dup = 0;

        if (!g->has[K_KIND] || strcmp(g->val[K_KIND], "guest") != 0 || !g->has[K_FIXTURE] || !g->has[K_UNTIL_MCLK] || authored_name(g->val[K_FIXTURE], r.name, sizeof r.name) != 0
            || parse_u64(g->val[K_UNTIL_MCLK], &r.until) != 0) {
            continue;
        }
        r.request = g->has[K_REQUEST] ? g->val[K_REQUEST] : "";
        for (j = 0u; j < n; j++) {
            dup = dup || (strcmp(roms[j].name, r.name) == 0 && strcmp(roms[j].request, r.request) == 0);
        }
        if (dup) {
            continue;
        }
        if (n >= OX_ROMS_MAX) {
            return -1;
        }
        roms[n++] = r;
    }
    return (int)n;
}

/* mode=report | mode=require;exclude=0xNN,... (0 on success); excluded[op] is set for each listed opcode, which must be a defined one */
static int ox_parse(const char *text, int *require, unsigned char *excluded)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have_mode = 0;
    int have_ex = 0;

    memset(excluded, 0, 256u);
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strcmp(item, "mode=report") == 0 && !have_mode) {
            *require = 0;
            have_mode = 1;
        } else if (strcmp(item, "mode=require") == 0 && !have_mode) {
            *require = 1;
            have_mode = 1;
        } else if (strncmp(item, "exclude=", 8u) == 0 && !have_ex) {
            char *er = item + 8;
            char *tok;
            unsigned count = 0u;

            have_ex = 1;
            while ((tok = pk_next_item(&er, ',')) != NULL) {
                uint64_t v = 0u;

                if (parse_u64(tok, &v) != 0 || v > 255u || ds_cpu_op_defined((uint32_t)v) == 0 || excluded[v] != 0u) {
                    return -1;
                }
                excluded[v] = 1u;
                count++;
            }
            if (count == 0u) {
                return -1;
            }
        } else {
            return -1;
        }
    }
    return have_mode && (*require || !have_ex) ? 0 : -1;
}

void kind_opcode_exercise(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct ox_rom roms[OX_ROMS_MAX];
    unsigned char seen[256];
    unsigned char excluded[256];
    static char detail[4096];
    struct ds_insn_sink *sink;
    unsigned char *records;
    int require = 0;
    int nroms;
    int r;
    unsigned op;
    unsigned defined = 0u;
    unsigned exercised = 0u;
    unsigned unexercised = 0u;
    size_t used;
    unsigned col = 0u;

    if (ox_parse(k->val[K_EXPECT], &require, excluded) != 0) {
        harness_error(c, "an opcode_exercise expect is mode=report or mode=require;exclude=0xNN,... (excluded opcodes are defined, listed once, and only in require mode)");
        return;
    }
    nroms = ox_collect(roms);
    if (nroms < 1) {
        harness_error(c, nroms < 0 ? "more than 32 distinct guest ROMs: raise OX_ROMS_MAX" : "no guest case with an authored ROM and an until_mclk was found");
        return;
    }
    sink = (struct ds_insn_sink *)calloc(1u, sizeof *sink);
    records = (unsigned char *)malloc((size_t)OX_SINK_RECORDS * DS_INSN_RECORD_BYTES);
    if (sink == NULL || records == NULL) {
        free(sink);
        free(records);
        harness_error(c, "out of memory for the opcode sink");
        return;
    }
    memset(seen, 0, sizeof seen);
    for (r = 0; r < nroms && !c->harness; r++) {
        struct booted b;
        const struct ds_instance *ci = NULL;
        struct ds_run_request rq;
        struct ds_run_result res;
        const char *err = boot_built_rom(roms[r].name, roms[r].request, &b, &ci);
        uint32_t count;
        uint32_t i;

        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        sink->records = records;
        sink->capacity = OX_SINK_RECORDS;
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = roms[r].until;
        rq.stop = dsv1_done;
        rq.insn = sink;
        if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
            release_boot(&b);
            harness_error(c, "ds_run refused the request of the opcode inventory");
            break;
        }
        release_boot(&b);
        count = get_u32(sink->header);
        check(c, get_u32(sink->header + 4) == 0u, "an instruction sink lost a record: the opcode inventory would be incomplete");
        if (get_u32(sink->header + 4) != 0u) {
            harness_error(c, "an instruction sink of the opcode inventory lost a record (lost != 0)");
            break;
        }
        for (i = 0u; i < count; i++) {
            seen[records[(size_t)i * DS_INSN_RECORD_BYTES + 10u]] = 1u;
        }
    }
    free(sink);
    free(records);
    if (c->harness) {
        return;
    }
    used = (size_t)snprintf(detail, sizeof detail, "rom_runs=%d\nunexercised defined opcodes (hex, sixteen to a line):\n", nroms);
    for (op = 0u; op < 256u; op++) {
        if (ds_cpu_op_defined(op) == 0) {
            continue;
        }
        defined++;
        if (seen[op] != 0u) {
            exercised++;
            continue;
        }
        unexercised++;
        if (used + 8u < sizeof detail) {
            used += (size_t)snprintf(detail + used, sizeof detail - used, "%s0x%02X", col == 0u ? "" : " ", op);
            col++;
            if (col == OX_PER_LINE) {
                used += (size_t)snprintf(detail + used, sizeof detail - used, "\n");
                col = 0u;
            }
        }
        if (require && excluded[op] == 0u) {
            check(c, 0, "a defined opcode is executed by no guest fixture and is not in the exclude list");
        }
    }
    if (col != 0u && used + 2u < sizeof detail) {
        (void)snprintf(detail + used, sizeof detail - used, "\n");
    }
    set_evidence_detail(detail);
    check(c, defined == exercised + unexercised && exercised <= defined, "the opcode counts do not add up");
    check(c, nroms >= 1, "no guest ROM was run");
    snprintf(got, gotcap, "defined=%u exercised=%u unexercised=%u", defined, exercised, unexercised);
}
