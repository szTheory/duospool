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
 *   observer          (plan 03-19) the non-interference matrix: fixture = authored:NAME, until_mclk, expect = compare=verdict,machine_time,stops,digest, sinks = the configurations besides off (large, drop4, poison, cap0, cap1; large is
 *                     required: it is the log every other configuration is compared with), chunks = the run modes (whole first; chunk:N runs N calls to equal parts of the deadline; stop:K holds the stop callback at its K-th call).
 *                     In each mode the machine is run once with no sink (the reference) and once per configuration with BOTH sinks (insn and dev); every run must give the reference stop reason, machine time, steps, overshoot,
 *                     verdict RAM (the DSV1 block), digest and stop sequence (the count and an FNV-1a fold of the machine time, pc, registers and bus latch at every call of the stop callback). The sinks themselves are checked too: headers
 *                     consistent (count = min(seq, capacity), lost = seq - count, insn seq = steps), the guard bytes and the record memory past the count untouched (a poisoned header never read), the records of a limited sink the
 *                     first records of the large sink, and, in a chunked run, the logs accumulated over the chunks equal the logs of the whole run. A large sink that lost a record is harness_failure (the log is evidence here).
 *                     E0: the core is compared with itself (non-interference), nothing here is an oracle. A hook that changes machine state turns this red (the trace-hook-mutates toy).
 *   dev_log           (plan 03-19) expect = mode=guest;rom=NAME;rec=PC:KIND:PHYS:VALUE:CYCLE;rec=... (until_mclk required; CYCLE is the zero-based cycle of the access inside its instruction, plan 03-21): the built ROM runs to the deadline with an insn sink and a dev sink; the dev log must hold exactly the listed records in order,
 *                     each with the stamp of the insn record at that pc, the pc, kind (W, R, ST), physical address, value (a number, or A for the A register in that insn record), the flags the address implies and zero spare bytes; or
 *                     mode=windows;rom=NAME: the accesses of every window driven through the bus choke point, which logs the writes of windows 0 to 5, the reads of window 4 and the ST physical writes and nothing else, and
 *                     the drop-newest arithmetic of the hook. E0, with literals from the case (PC, physical address and immediates from the fixture source and the hardware manual), none from DuoSpool output.
 *   trace_out_rights  (plan 03-19) the rule of the --trace-out option, below.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_serial.h"
#include "ds_sha256.h"
#include "runner.h"
#include "synth.h"

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
            p->opcode = ds_bus_read(w, NULL, w->pc); /* the pc of these images is in ROM or work RAM, where a read changes nothing (a hardware-page read would update io_buffer, D-104) */
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

/* ---- kind observer: the non-interference matrix (plan 03-19; D-119, D-131, CORE-06, T-03-53) ---- */

#define OB_INSN_LARGE 262144u /* records: more than any run of the matrix executes, so a large sink never drops */
#define OB_DEV_LARGE 4096u
#define OB_CHUNKS_MAX 64u
#define OB_MODES_MAX 6u
#define OB_CFG_MAX 6u
#define OB_FNV_BASIS UINT64_C(14695981039346656037)
#define OB_FNV_PRIME UINT64_C(1099511628211)

/* a sink configuration: capacities of the two sinks, whether header and record memory start poisoned with $A5; "off" carries no sink at all */
struct ob_cfg {
    const char *name;
    int on;
    uint32_t icap;
    uint32_t dcap;
    int poison;
    int large; /* 1 for the two configurations that never drop (large and poison): the kind lowers their instruction capacity to what the deadline can execute, see ob_cfg_fit */
};

static const struct ob_cfg ob_cfgs[OB_CFG_MAX] = {
    {"off", 0, 0u, 0u, 0, 0},
    {"large", 1, OB_INSN_LARGE, OB_DEV_LARGE, 0, 1},
    {"drop4", 1, 4u, 4u, 0, 0},
    {"poison", 1, OB_INSN_LARGE, OB_DEV_LARGE, 1, 1},
    {"cap0", 1, 0u, 0u, 1, 0}, /* a non-NULL record pointer and a poisoned header with capacity 0 */
    {"cap1", 1, 1u, 1u, 1, 0},
};

/* a run mode: one call to the deadline (whole), N calls to equal parts of it (chunk) or one call whose stop callback holds at its K-th invocation (stop) */
enum ob_mode_kind { OBM_WHOLE, OBM_CHUNK, OBM_STOP };

struct ob_mode {
    enum ob_mode_kind kind;
    uint64_t n;
    char name[24];
};

/* the stop callback of every run: it never changes anything, counts its invocations, folds the machine time, pc, A, X, Y, S, P, speed, bus latch and block transfer state of each into an FNV-1a value (so a hook that disturbs a register or the latch for even one
 * instruction is seen at the next call, whatever the guest does with it later), and holds at invocation `at` (0 = never) */
struct ob_stopper {
    uint64_t calls;
    uint64_t fold;
    uint64_t at;
    uint64_t last;     /* the machine time of the previous invocation (0 before the first: a boot is at machine time 0) */
    uint64_t cont;     /* the steps that continued a block transfer (the byte steps and the epilogue): a step that began with blk_active set; the instruction hook is offered once per instruction, so these are steps without an offer (plan 03-26) */
    uint8_t prev_blk;  /* blk_active at the previous invocation */
};

static int ob_stop(const struct ds_instance *inst, void *ctx)
{
    struct ob_stopper *s = (struct ob_stopper *)ctx;
    uint64_t t = inst->machine_time;
    unsigned i;

    s->calls = s->calls + 1u;
    if (t != s->last) { /* a step has completed since the previous invocation (the first invocation of a call comes before its first step, at the same machine time) */
        s->cont = s->cont + (s->prev_blk != 0u ? 1u : 0u);
        s->last = t;
        s->prev_blk = inst->blk_active;
    }
    for (i = 0u; i < 8u; i++) {
        s->fold = (s->fold ^ ((t >> (8u * i)) & UINT64_C(0xFF))) * OB_FNV_PRIME;
    }
    s->fold = (s->fold ^ (uint64_t)inst->pc) * OB_FNV_PRIME;
    s->fold = (s->fold ^ (((uint64_t)inst->a << 8u) | (uint64_t)inst->x)) * OB_FNV_PRIME;
    s->fold = (s->fold ^ (((uint64_t)inst->y << 8u) | (uint64_t)inst->s)) * OB_FNV_PRIME;
    s->fold = (s->fold ^ (((uint64_t)inst->p << 16u) | ((uint64_t)inst->speed << 8u) | (uint64_t)inst->io_buffer)) * OB_FNV_PRIME;
    s->fold = (s->fold ^ (((uint64_t)inst->blk_active << 40u) | ((uint64_t)inst->blk_opcode << 32u) | ((uint64_t)inst->blk_toggle << 24u) | (uint64_t)inst->blk_remaining)) * OB_FNV_PRIME;
    s->fold = (s->fold ^ (((uint64_t)inst->blk_src << 16u) | (uint64_t)inst->blk_dst)) * OB_FNV_PRIME;
    return s->at != 0u && s->calls >= s->at;
}

/* a copy of what a sink held after a run: count records of 24 bytes each (NULL for none) and the header numbers */
struct ob_log {
    unsigned char *rec;
    uint32_t count;
    uint32_t lost;
    uint64_t seq;
};

/* where the program of a run comes from (plan 03-26): a built authored image (rom), or the in-process random program of an index (randprog.c); k is the case, whose request is booted over and whose irq_stim is declared to every boot */
struct ob_src {
    const char *rom;
    int random;
    unsigned index;
    const unsigned char *img; /* the image of the random program, built once for the runs of the whole matrix */
    const struct kase *k;
};

/* the observable outcome of one run: everything a sink must not change, plus the two logs; bstops counts the calls that took a step and ended inside a block transfer and pstops those of them with the timer interrupt and IRQ2 both pending at the stop stamp */
struct ob_out {
    struct end e;
    uint8_t verdict[V_SIZE];
    uint64_t scalls;
    uint64_t sfold;
    uint64_t bstops;
    uint64_t pstops;
    uint64_t cont; /* steps that continued a block transfer: executed steps without an instruction offer to the instruction sink */
    struct ob_log ilog;
    struct ob_log dlog;
};

static void ob_out_free(struct ob_out *o)
{
    free(o->ilog.rec);
    free(o->dlog.rec);
    o->ilog.rec = NULL;
    o->dlog.rec = NULL;
}

/* the memory of the sinks of one run: the two sink structs and the record blocks with guard bytes either side, all poisoned or zero as the configuration says */
struct ob_mem {
    struct ds_insn_sink *isk;
    struct ds_dev_sink *dsk;
    unsigned char *iblock;
    unsigned char *dblock;
    size_t ibytes;
    size_t dbytes;
    int poison;
};

static void ob_mem_free(struct ob_mem *m)
{
    free(m->isk);
    free(m->dsk);
    free(m->iblock);
    free(m->dblock);
    memset(m, 0, sizeof *m);
}

/* 0 on success; a zero capacity still gets one record of memory behind its non-NULL pointer, so a stray write has somewhere to land and be seen */
static int ob_mem_make(struct ob_mem *m, const struct ob_cfg *cf)
{
    unsigned char fill = cf->poison ? (unsigned char)OBS_POISON : 0u;

    memset(m, 0, sizeof *m);
    m->poison = cf->poison;
    if (!cf->on) {
        return 0;
    }
    m->ibytes = (size_t)(cf->icap != 0u ? cf->icap : 1u) * (size_t)DS_INSN_RECORD_BYTES;
    m->dbytes = (size_t)(cf->dcap != 0u ? cf->dcap : 1u) * (size_t)DS_DEV_RECORD_BYTES;
    m->isk = (struct ds_insn_sink *)calloc(1u, sizeof *m->isk);
    m->dsk = (struct ds_dev_sink *)calloc(1u, sizeof *m->dsk);
    m->iblock = (unsigned char *)malloc(m->ibytes + 2u * OBS_GUARD);
    m->dblock = (unsigned char *)malloc(m->dbytes + 2u * OBS_GUARD);
    if (m->isk == NULL || m->dsk == NULL || m->iblock == NULL || m->dblock == NULL) {
        ob_mem_free(m);
        return -1;
    }
    memset(m->iblock, fill, m->ibytes + 2u * OBS_GUARD);
    memset(m->dblock, fill, m->dbytes + 2u * OBS_GUARD);
    memset(m->isk->header, fill, sizeof m->isk->header);
    memset(m->dsk->header, fill, sizeof m->dsk->header);
    m->isk->records = m->iblock + OBS_GUARD;
    m->isk->capacity = cf->icap;
    m->dsk->records = m->dblock + OBS_GUARD;
    m->dsk->capacity = cf->dcap;
    return 0;
}

/* the guard bytes either side of a record block still hold the fill */
static int ob_guards_ok(const unsigned char *block, size_t bytes, int poison)
{
    unsigned char want = poison ? (unsigned char)OBS_POISON : 0u;
    size_t j;
    int ok = 1;

    for (j = 0u; j < OBS_GUARD; j++) {
        ok = ok && block[j] == want && block[OBS_GUARD + bytes + j] == want;
    }
    return ok;
}

/* the record bytes from index `from` to the end of the block still hold the fill (single-call runs only: a chunked run leaves earlier chunks' records behind) */
static int ob_tail_ok(const unsigned char *rec, size_t bytes, size_t from, int poison)
{
    unsigned char want = poison ? (unsigned char)OBS_POISON : 0u;
    size_t j;
    int ok = 1;

    for (j = from; j < bytes; j++) {
        ok = ok && rec[j] == want;
    }
    return ok;
}

static void ob_log_header(struct ob_log *l, const unsigned char *header)
{
    l->count = get_u32(header);
    l->lost = get_u32(header + 4);
    l->seq = get_u64(header + 8);
}

/* append the records of one call (count of them at rec) to an accumulated log of at most cap records: 0 on success */
static int ob_log_append(struct ob_log *acc, uint32_t cap, size_t unit, const unsigned char *rec, uint32_t count)
{
    if (acc->rec == NULL) {
        acc->rec = (unsigned char *)malloc((size_t)cap * unit);
        if (acc->rec == NULL) {
            return -1;
        }
    }
    if ((uint64_t)acc->count + (uint64_t)count > (uint64_t)cap) {
        return -1;
    }
    memcpy(acc->rec + (size_t)acc->count * unit, rec, (size_t)count * unit);
    acc->count += count;
    return 0;
}

/* what one run of one call gave: fill the end state from the run result and the instance */
static void ob_end_of(struct ob_out *o, struct ds_instance *w, uint32_t reason, const struct ds_run_result *res, const struct ob_stopper *st)
{
    o->e.reason = reason;
    o->e.mclk = res->machine_time;
    o->e.steps = o->e.steps + res->steps;
    o->e.overshoot = res->overshoot;
    o->e.digest_ok = ds_digest(w, o->e.digest) == 0u;
    memcpy(o->verdict, w->ram8k + V_OFF, V_SIZE);
    o->scalls = st->calls;
    o->sfold = st->fold;
    o->cont = st->cont;
}

/* the per-call header of a sink must be a consistent count of what that call offered: count = min(seq, capacity), lost = seq - count; the offered number is returned in *seq */
static int ob_header_consistent(const unsigned char *header, uint32_t cap, uint64_t *seq)
{
    uint32_t count = get_u32(header);
    uint32_t lost = get_u32(header + 4);
    uint64_t s = get_u64(header + 8);
    uint64_t want = s < (uint64_t)cap ? s : (uint64_t)cap;

    *seq = s;
    return (uint64_t)count == want && (uint64_t)lost == s - want;
}

/* one run of the machine under one configuration and mode: 0 on success (checks made on c), -1 after a harness error. A whole or stop run is one ds_run call; a chunk run is o->n calls to equal parts of the deadline. */
static int ob_exec(struct ctx *c, const struct ob_src *src, uint64_t until, const struct ob_cfg *cf, const struct ob_mode *md, struct ob_out *o)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    struct ob_mem m;
    struct ob_stopper st;
    struct ds_run_request rq;
    struct ds_run_result res;
    uint32_t reason = 0u;
    uint64_t calls = md->kind == OBM_CHUNK ? md->n : 1u;
    uint64_t j;
    uint64_t cont0 = 0u;
    uint32_t refusal = 0u;
    char msg[200];
    const char *err = src->random ? boot_random(src->img, src->k->val[K_REQUEST], &b, &ci) : boot_built_rom(src->rom, src->k->val[K_REQUEST], &b, &ci);
    int rc = 0;

    memset(o, 0, sizeof *o);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    w = (struct ds_instance *)(void *)b.mem;
    err = apply_irq_stim(w, src->k, &refusal); /* the declared stimulus of the case (plan 03-26, D-112): a test hook, applied to every run of the matrix */
    if (err != NULL || refusal != 0u) {
        release_boot(&b);
        harness_error(c, err != NULL ? err : "ds_irq_stim_set refused the irq_stim list of the case");
        return -1;
    }
    if (ob_mem_make(&m, cf) != 0) {
        release_boot(&b);
        harness_error(c, "out of memory for the observer sinks");
        return -1;
    }
    st.calls = 0u;
    st.fold = OB_FNV_BASIS;
    st.at = md->kind == OBM_STOP ? md->n : 0u;
    st.last = 0u;
    st.cont = 0u;
    st.prev_blk = 0u;
    memset(&res, 0, sizeof res);
    for (j = 1u; j <= calls && rc == 0; j++) {
        memset(&rq, 0, sizeof rq);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = md->kind == OBM_CHUNK ? until * j / md->n : until;
        rq.stop = ob_stop;
        rq.stop_ctx = &st;
        rq.insn = m.isk;
        rq.dev = m.dsk;
        cont0 = st.cont;
        reason = ds_run(w, &rq, &res);
        if (reason == (uint32_t)DS_RUN_REASON_REFUSED) {
            harness_error(c, "ds_run refused a request of the observer matrix");
            rc = -1;
            break;
        }
        if (res.steps > 0u && res.stop_point == (uint32_t)DS_STOP_BLOCK_BYTE) {
            o->bstops++;
            if (ds_irq_pending_query(ci, ci->machine_time) == 5u) {
                o->pstops++;
            }
        }
        if (cf->on && m.isk != NULL) {
            uint64_t iseq = 0u;
            uint64_t dseq = 0u;
            const unsigned char *irec = m.iblock + OBS_GUARD;
            const unsigned char *drec = m.dblock + OBS_GUARD;

            snprintf(msg, sizeof msg, "the insn header of the %s sink in mode %s is not a consistent count (count = min(seq, capacity), lost = seq - count)", cf->name, md->name);
            check(c, ob_header_consistent(m.isk->header, cf->icap, &iseq), msg);
            snprintf(msg, sizeof msg, "the dev header of the %s sink in mode %s is not a consistent count (count = min(seq, capacity), lost = seq - count)", cf->name, md->name);
            check(c, ob_header_consistent(m.dsk->header, cf->dcap, &dseq), msg);
            snprintf(msg, sizeof msg, "the insn header of the %s sink in mode %s does not count one offer per executed instruction of the call (steps that continue a block transfer are not instructions)", cf->name, md->name);
            check(c, iseq == res.steps - (st.cont - cont0), msg);
            if (md->kind == OBM_CHUNK && cf->large) { /* the large and the poisoned sink: their accumulated logs are compared in the kind */
                if (ob_log_append(&o->ilog, cf->icap, DS_INSN_RECORD_BYTES, irec, get_u32(m.isk->header)) != 0 || ob_log_append(&o->dlog, cf->dcap, DS_DEV_RECORD_BYTES, drec, get_u32(m.dsk->header)) != 0) {
                    harness_error(c, "the accumulated log of a chunked run overflowed or could not be allocated");
                    rc = -1;
                }
                o->ilog.seq += iseq;
                o->dlog.seq += dseq;
                o->ilog.lost += get_u32(m.isk->header + 4);
                o->dlog.lost += get_u32(m.dsk->header + 4);
            }
        }
        ob_end_of(o, w, reason, &res, &st);
    }
    if (rc == 0 && cf->on) {
        snprintf(msg, sizeof msg, "the core wrote outside the record memory of the %s sink in mode %s (guard bytes changed)", cf->name, md->name);
        check(c, ob_guards_ok(m.iblock, m.ibytes, m.poison) && ob_guards_ok(m.dblock, m.dbytes, m.poison), msg);
        if (md->kind != OBM_CHUNK) {
            uint32_t ic = get_u32(m.isk->header);
            uint32_t dc = get_u32(m.dsk->header);

            snprintf(msg, sizeof msg, "the core wrote into the record memory past the count of the %s sink in mode %s", cf->name, md->name);
            check(c, ob_tail_ok(m.iblock + OBS_GUARD, m.ibytes, (size_t)ic * DS_INSN_RECORD_BYTES, m.poison) && ob_tail_ok(m.dblock + OBS_GUARD, m.dbytes, (size_t)dc * DS_DEV_RECORD_BYTES, m.poison), msg);
            ob_log_header(&o->ilog, m.isk->header);
            ob_log_header(&o->dlog, m.dsk->header);
            if (ic > 0u) {
                o->ilog.rec = (unsigned char *)malloc((size_t)ic * DS_INSN_RECORD_BYTES);
            }
            if (dc > 0u) {
                o->dlog.rec = (unsigned char *)malloc((size_t)dc * DS_DEV_RECORD_BYTES);
            }
            if ((ic > 0u && o->ilog.rec == NULL) || (dc > 0u && o->dlog.rec == NULL)) {
                harness_error(c, "out of memory for the log copy of the observer matrix");
                rc = -1;
            } else {
                memcpy(o->ilog.rec, m.iblock + OBS_GUARD, (size_t)ic * DS_INSN_RECORD_BYTES);
                memcpy(o->dlog.rec, m.dblock + OBS_GUARD, (size_t)dc * DS_DEV_RECORD_BYTES);
                o->ilog.count = ic;
                o->dlog.count = dc;
            }
        }
    }
    ob_mem_free(&m);
    release_boot(&b);
    return rc;
}

/* the end state of a sink run equals the end state of the same mode without a sink: what a sink must not change */
static void ob_same_as(struct ctx *c, const struct ob_out *ref, const struct ob_out *o, const char *cfg, const char *mode)
{
    char msg[200];

    snprintf(msg, sizeof msg, "the %s sink in mode %s changed the stop reason, the machine time, the steps or the overshoot of the run", cfg, mode);
    check(c, o->e.reason == ref->e.reason && o->e.mclk == ref->e.mclk && o->e.steps == ref->e.steps && o->e.overshoot == ref->e.overshoot, msg);
    snprintf(msg, sizeof msg, "the %s sink in mode %s changed the verdict RAM of the run", cfg, mode);
    check(c, memcmp(o->verdict, ref->verdict, V_SIZE) == 0, msg);
    snprintf(msg, sizeof msg, "the %s sink in mode %s changed the digest of the instance", cfg, mode);
    check(c, o->e.digest_ok && ref->e.digest_ok && memcmp(o->e.digest, ref->e.digest, DS_DIGEST_BYTES) == 0, msg);
    snprintf(msg, sizeof msg, "the %s sink in mode %s changed the stop sequence (the stop callback was called at other times)", cfg, mode);
    check(c, o->scalls == ref->scalls && o->sfold == ref->sfold && o->bstops == ref->bstops && o->pstops == ref->pstops, msg);
}

/* two accumulated or copied logs hold the same records and the same offers */
static int ob_logs_equal(const struct ob_out *a, const struct ob_out *b)
{
    return a->ilog.count == b->ilog.count && a->dlog.count == b->dlog.count && a->ilog.seq == b->ilog.seq && a->dlog.seq == b->dlog.seq
           && (a->ilog.count == 0u || (a->ilog.rec != NULL && b->ilog.rec != NULL && memcmp(a->ilog.rec, b->ilog.rec, (size_t)a->ilog.count * DS_INSN_RECORD_BYTES) == 0))
           && (a->dlog.count == 0u || (a->dlog.rec != NULL && b->dlog.rec != NULL && memcmp(a->dlog.rec, b->dlog.rec, (size_t)a->dlog.count * DS_DEV_RECORD_BYTES) == 0));
}

/* the records a limited sink holds are the first count records of the large sink of the same mode, and its counts follow from what the large sink saw offered */
static void ob_prefix_of(struct ctx *c, const struct ob_out *large, const struct ob_out *o, const struct ob_cfg *cf, const char *mode)
{
    char msg[200];
    uint64_t want_i = large->ilog.seq < (uint64_t)cf->icap ? large->ilog.seq : (uint64_t)cf->icap;
    uint64_t want_d = large->dlog.seq < (uint64_t)cf->dcap ? large->dlog.seq : (uint64_t)cf->dcap;

    snprintf(msg, sizeof msg, "the insn header of the %s sink in mode %s differs from the offers the large sink saw", cf->name, mode);
    check(c, o->ilog.seq == large->ilog.seq && (uint64_t)o->ilog.count == want_i && (uint64_t)o->ilog.lost == large->ilog.seq - want_i, msg);
    snprintf(msg, sizeof msg, "the dev header of the %s sink in mode %s differs from the offers the large sink saw", cf->name, mode);
    check(c, o->dlog.seq == large->dlog.seq && (uint64_t)o->dlog.count == want_d && (uint64_t)o->dlog.lost == large->dlog.seq - want_d, msg);
    snprintf(msg, sizeof msg, "the insn records of the %s sink in mode %s are not the first records of the large sink", cf->name, mode);
    check(c, o->ilog.count == 0u || (large->ilog.rec != NULL && o->ilog.rec != NULL && memcmp(o->ilog.rec, large->ilog.rec, (size_t)o->ilog.count * DS_INSN_RECORD_BYTES) == 0), msg);
    snprintf(msg, sizeof msg, "the dev records of the %s sink in mode %s are not the first records of the large sink", cf->name, mode);
    check(c, o->dlog.count == 0u || (large->dlog.rec != NULL && o->dlog.rec != NULL && memcmp(o->dlog.rec, large->dlog.rec, (size_t)o->dlog.count * DS_DEV_RECORD_BYTES) == 0), msg);
}

/* the grammar of the keys of an observer case: expect = compare=verdict,machine_time,stops,digest (the four things compared, each once), then optionally pending-stop (plan 03-26: in every mode but whole the reference run must end at least one call inside a block transfer with the timer interrupt and IRQ2
 * both pending at the stop stamp) and random=N (N in-process random programs, randprog.c, 1 to 64, with fixture = none); sinks = the configurations besides off, from ob_cfgs, large among them;
 * chunks = the modes, whole first: whole, chunk:N (2 to 64 calls), stop:K (the stop callback holds at its K-th invocation) */
static int ob_parse_expect(const char *text, int *pending, unsigned *random)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned seen = 0u;
    int first = 1;

    *pending = 0;
    *random = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0 || strncmp(text, "compare=", 8u) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        uint64_t v = 0u;

        if (first) {
            char *r2 = item + 8;
            char *w;

            first = 0;
            while ((w = pk_next_item(&r2, ',')) != NULL) {
                unsigned bit = strcmp(w, "verdict") == 0 ? 1u : strcmp(w, "machine_time") == 0 ? 2u : strcmp(w, "stops") == 0 ? 4u : strcmp(w, "digest") == 0 ? 8u : 0u;

                if (bit == 0u || (seen & bit) != 0u) {
                    return -1;
                }
                seen |= bit;
            }
        } else if (strcmp(item, "pending-stop") == 0 && !*pending) {
            *pending = 1;
        } else if (strncmp(item, "random=", 7u) == 0 && *random == 0u && parse_u64(item + 7, &v) == 0 && v >= 1u && v <= 64u) {
            *random = (unsigned)v;
        } else {
            return -1;
        }
    }
    return seen == 15u ? 0 : -1;
}

static int ob_parse_sinks(const char *text, int *want)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned i;
    unsigned n = 0u;

    memset(want, 0, OB_CFG_MAX * sizeof *want);
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        for (i = 1u; i < OB_CFG_MAX && strcmp(item, ob_cfgs[i].name) != 0; i++) {
        }
        if (i == OB_CFG_MAX || want[i]) {
            return -1;
        }
        want[i] = 1;
        n++;
    }
    return n >= 1u && want[1] ? 0 : -1;
}

static int ob_parse_modes(const char *text, struct ob_mode *modes, unsigned *nmodes)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    *nmodes = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        struct ob_mode *m = &modes[*nmodes];
        uint64_t v = 0u;

        if (*nmodes >= OB_MODES_MAX) {
            return -1;
        }
        if (strcmp(item, "whole") == 0 && *nmodes == 0u) {
            m->kind = OBM_WHOLE;
            m->n = 0u;
        } else if (strncmp(item, "chunk:", 6u) == 0 && parse_u64(item + 6, &v) == 0 && v >= 2u && v <= OB_CHUNKS_MAX) {
            m->kind = OBM_CHUNK;
            m->n = v;
        } else if (strncmp(item, "stop:", 5u) == 0 && parse_u64(item + 5, &v) == 0 && v >= 2u && v <= 1000000u) {
            m->kind = OBM_STOP;
            m->n = v;
        } else {
            return -1;
        }
        if (strlen(item) >= sizeof m->name) {
            return -1;
        }
        memcpy(m->name, item, strlen(item) + 1u);
        (*nmodes)++;
    }
    return *nmodes >= 1u && modes[0].kind == OBM_WHOLE ? 0 : -1;
}

/* the whole matrix for one program: every mode, each with the no-sink reference and every configuration; *runs counts the machine runs */
static void ob_matrix(struct ctx *c, const struct ob_src *src, uint64_t until, const int *want, const struct ob_mode *modes, unsigned nmodes, int pending, unsigned *runs)
{
    struct ob_out whole_large;
    struct ob_cfg cfg[OB_CFG_MAX];
    int have_whole = 0;
    unsigned mi;
    unsigned q;

    memset(&whole_large, 0, sizeof whole_large);
    for (q = 0u; q < OB_CFG_MAX; q++) {
        cfg[q] = ob_cfgs[q];
        if (cfg[q].large) {
            uint64_t fit = until / 6u + 16u; /* an instruction takes at least two cycles of at least three master clocks, so a deadline cannot execute more instructions than this; a large sink that still loses a record is harness_failure below */

            cfg[q].icap = fit < (uint64_t)OB_INSN_LARGE ? (uint32_t)fit : (uint32_t)OB_INSN_LARGE;
        }
    }
    for (mi = 0u; mi < nmodes && !c->harness; mi++) {
        struct ob_out ref;
        struct ob_out large;
        unsigned ci;

        memset(&ref, 0, sizeof ref);
        memset(&large, 0, sizeof large);
        if (ob_exec(c, src, until, &cfg[0], &modes[mi], &ref) != 0) {
            break;
        }
        (*runs)++;
        if (mi == 0u) {
            if (!src->random || src->index == 0u) {
                note_run(c, ref.e.mclk, ref.e.reason);
            }
            check(c, ref.e.digest_ok && ref.e.steps > 4u, "the reference run without a sink took four steps or fewer, or its digest could not be taken");
        }
        if (pending && modes[mi].kind != OBM_WHOLE) {
            char msg[200];

            snprintf(msg, sizeof msg, "no call of the reference run in mode %s ended inside a block transfer with the timer interrupt and IRQ2 both pending at the stop stamp (ds_irq_pending_query)", modes[mi].name);
            check(c, ref.pstops > 0u, msg);
        }
        if (ob_exec(c, src, until, &cfg[1], &modes[mi], &large) != 0) {
            ob_out_free(&ref);
            break;
        }
        (*runs)++;
        ob_same_as(c, &ref, &large, cfg[1].name, modes[mi].name);
        if (large.ilog.lost != 0u || large.dlog.lost != 0u) {
            harness_error(c, "the large sink of the observer matrix lost a record: the log is evidence here and would be incomplete");
        }
        if (modes[mi].kind == OBM_STOP) {
            check(c, ref.e.reason == (uint32_t)DS_RUN_REASON_CALLBACK && ref.scalls == modes[mi].n, "a stop mode run did not end on the stop callback at its K-th invocation");
        }
        for (ci = 2u; ci < OB_CFG_MAX && !c->harness; ci++) {
            struct ob_out o;

            if (!want[ci]) {
                continue;
            }
            if (ob_exec(c, src, until, &cfg[ci], &modes[mi], &o) != 0) {
                break;
            }
            (*runs)++;
            ob_same_as(c, &ref, &o, cfg[ci].name, modes[mi].name);
            if (modes[mi].kind != OBM_CHUNK) {
                ob_prefix_of(c, &large, &o, &cfg[ci], modes[mi].name);
            } else if (cfg[ci].large) {
                char msg[200];

                snprintf(msg, sizeof msg, "the logs accumulated over the %s run with the %s sink differ from those of the large sink", modes[mi].name, cfg[ci].name);
                check(c, ob_logs_equal(&large, &o), msg);
            }
            ob_out_free(&o);
        }
        if (mi == 0u) {
            whole_large = large;
            have_whole = 1;
            memset(&large, 0, sizeof large);
            check(c, whole_large.ilog.seq == ref.e.steps - ref.cont, "the large insn sink of the whole run did not offer one record per executed instruction (steps that continue a block transfer are not instructions)");
        } else if (have_whole && modes[mi].kind == OBM_CHUNK) {
            char msg[200];

            snprintf(msg, sizeof msg, "the end state of the %s run differs from the whole run (chunk invariance of the machine)", modes[mi].name);
            check(c, ref.e.mclk == whole_large.e.mclk && ref.e.steps == whole_large.e.steps && memcmp(ref.verdict, whole_large.verdict, V_SIZE) == 0 && memcmp(ref.e.digest, whole_large.e.digest, DS_DIGEST_BYTES) == 0, msg);
            snprintf(msg, sizeof msg, "the logs accumulated over the %s run differ from the logs of the whole run (chunk invariance of the sinks)", modes[mi].name);
            check(c, ob_logs_equal(&large, &whole_large), msg);
        } else if (have_whole && modes[mi].kind == OBM_STOP) {
            char msg[200];

            snprintf(msg, sizeof msg, "the insn records of the %s run are not the first records of the whole run", modes[mi].name);
            check(c, large.ilog.count <= whole_large.ilog.count && (large.ilog.count == 0u || memcmp(large.ilog.rec, whole_large.ilog.rec, (size_t)large.ilog.count * DS_INSN_RECORD_BYTES) == 0), msg);
        }
        ob_out_free(&ref);
        ob_out_free(&large);
    }
    ob_out_free(&whole_large);
}

void kind_observer(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    uint64_t until = 0u;
    int want[OB_CFG_MAX];
    struct ob_mode modes[OB_MODES_MAX];
    unsigned nmodes = 0u;
    int pending = 0;
    unsigned nrandom = 0u;
    unsigned runs = 0u;
    unsigned pi;
    unsigned nprog = 1u;
    unsigned char *img = NULL;
    struct ob_src src;

    rom[0] = '\0';
    if (!k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || until > UINT64_C(0xFFFFFFFF) || !k->has[K_SINKS] || !k->has[K_CHUNKS] || ob_parse_expect(k->val[K_EXPECT], &pending, &nrandom) != 0
        || ob_parse_sinks(k->val[K_SINKS], want) != 0 || ob_parse_modes(k->val[K_CHUNKS], modes, &nmodes) != 0 || (nrandom == 0u && authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0)) {
        harness_error(c, "an observer case needs fixture = authored:NAME (or fixture = none with random=N in its expect), until_mclk, expect = compare=verdict,machine_time,stops,digest[;pending-stop][;random=N], sinks = a list of off-less configurations with large (large, drop4, poison, cap0, cap1) and chunks = whole first, then chunk:N or stop:K");
        return;
    }
    memset(&src, 0, sizeof src);
    src.k = k;
    src.rom = rom;
    if (nrandom != 0u) {
        img = (unsigned char *)malloc(RP_IMAGE_BYTES);
        if (img == NULL) {
            harness_error(c, "out of memory for the random program image");
            return;
        }
        src.random = 1;
        src.img = img;
        nprog = nrandom;
    }
    for (pi = 0u; pi < nprog && !c->harness; pi++) {
        src.index = pi;
        if (img != NULL) {
            rp_build(img, rp_seed(pi));
        }
        ob_matrix(c, &src, until, want, modes, nmodes, pending, &runs);
    }
    free(img);
    snprintf(got, gotcap, "programs=%u modes=%u runs=%u", nprog, nmodes, runs);
}

/* ---- kind dev_log: the device-register log (plan 03-19; D-119, CORE-06) ---- */

#define DL_ITEMS_MAX 8u
#define DL_DEV_CAP 64u
#define DL_INSN_CAP 262144u
#define DL_WIN_CAP 4u
#define DL_MCLK_LOW 12u /* master clocks per CPU cycle at the power-up low speed (D-90), a literal of the judge: the two stores of the cyc-probe penalty section run before its CSH */

/* one expected record of a guest log: the pc of the instruction, kind (W write, R read, ST), the physical address and the byte (a number, or A for the A register in the insn record of that instruction) */
struct dl_item {
    uint32_t pc;
    uint32_t kind;
    uint32_t phys;
    uint32_t value;
    int value_is_a;
    uint32_t cycle; /* the zero-based cycle of the instruction in which the access happens (plan 03-21, D-92): its page cycle count minus one plus the VDC and VCE penalty cycles of the assumption */
};

static int dl_parse_item(const char *text, struct dl_item *it)
{
    char buf[64];
    char *rest = buf;
    char *f[5];
    unsigned n = 0u;
    uint64_t v = 0u;
    char *tok;

    if (strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((tok = pk_next_item(&rest, ':')) != NULL) {
        if (n >= 5u) {
            return -1;
        }
        f[n++] = tok;
    }
    if (n != 5u || parse_u64(f[4], &v) != 0 || v > 63u) {
        return -1;
    }
    it->cycle = (uint32_t)v;
    if (parse_u64(f[0], &v) != 0 || v > 0xFFFFu) {
        return -1;
    }
    it->pc = (uint32_t)v;
    if (strcmp(f[1], "W") == 0) {
        it->kind = (uint32_t)DS_DEV_WRITE;
    } else if (strcmp(f[1], "R") == 0) {
        it->kind = (uint32_t)DS_DEV_READ;
    } else if (strcmp(f[1], "ST") == 0) {
        it->kind = (uint32_t)DS_DEV_ST;
    } else {
        return -1;
    }
    if (parse_u64(f[2], &v) != 0 || v < 0x1FE000u || v > 0x1FFFFFu) {
        return -1;
    }
    it->phys = (uint32_t)v;
    it->value_is_a = strcmp(f[3], "A") == 0;
    if (!it->value_is_a) {
        if (parse_u64(f[3], &v) != 0 || v > 0xFFu) {
            return -1;
        }
        it->value = (uint32_t)v;
    } else {
        it->value = 0u;
    }
    return 0;
}

/* mode=guest;rom=NAME;rec=PC:KIND:PHYS:VALUE:CYCLE... or mode=windows;rom=NAME (0 on success) */
static int dl_parse(const char *text, int *windows, char *rom, size_t romcap, struct dl_item *items, unsigned *nitems)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have_mode = 0;
    int have_rom = 0;

    *nitems = 0u;
    *windows = 0;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strcmp(item, "mode=guest") == 0 && !have_mode) {
            have_mode = 1;
        } else if (strcmp(item, "mode=windows") == 0 && !have_mode) {
            have_mode = 1;
            *windows = 1;
        } else if (strncmp(item, "rom=", 4u) == 0 && !have_rom && strlen(item + 4) > 0u && strlen(item + 4) < romcap && strspn(item + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 4)) {
            memcpy(rom, item + 4, strlen(item + 4) + 1u);
            have_rom = 1;
        } else if (strncmp(item, "rec=", 4u) == 0 && *nitems < DL_ITEMS_MAX && dl_parse_item(item + 4, &items[*nitems]) == 0) {
            (*nitems)++;
        } else {
            return -1;
        }
    }
    return have_mode && have_rom && (*windows ? *nitems == 0u : *nitems >= 1u) ? 0 : -1;
}

/* the dev record of index i of a log */
static const unsigned char *dl_rec(const unsigned char *log, uint32_t i)
{
    return log + (size_t)i * DS_DEV_RECORD_BYTES;
}

/* the flags a dev record of this physical address and kind must carry: the penalty bit for the VDC and VCE windows (offset in the page below $800, D-107) and the physical bit for the ST writes */
static unsigned dl_flags(uint32_t phys, uint32_t kind)
{
    return ((phys & 0x1FFFu) < 0x800u ? (unsigned)DS_DEV_FLAG_PENALTY : 0u) | (kind == (uint32_t)DS_DEV_ST ? (unsigned)DS_DEV_FLAG_PHYS : 0u);
}

static void dl_guest(struct ctx *c, const char *rom, const char *request, uint64_t until, const struct dl_item *items, unsigned nitems, char *got, size_t gotcap)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_insn_sink *isk = (struct ds_insn_sink *)calloc(1u, sizeof *isk);
    struct ds_dev_sink *dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    unsigned char *irec = (unsigned char *)malloc((size_t)DL_INSN_CAP * DS_INSN_RECORD_BYTES);
    unsigned char *drec = (unsigned char *)calloc((size_t)DL_DEV_CAP, DS_DEV_RECORD_BYTES);
    const char *err;
    uint32_t icount;
    uint32_t dcount;
    unsigned i;
    char msg[160];

    if (isk == NULL || dsk == NULL || irec == NULL || drec == NULL) {
        free(isk);
        free(dsk);
        free(irec);
        free(drec);
        harness_error(c, "out of memory for the dev log sinks");
        return;
    }
    err = boot_built_rom(rom, request, &b, &ci);
    if (err != NULL) {
        free(isk);
        free(dsk);
        free(irec);
        free(drec);
        harness_error(c, err);
        return;
    }
    isk->records = irec;
    isk->capacity = DL_INSN_CAP;
    dsk->records = drec;
    dsk->capacity = DL_DEV_CAP;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.insn = isk;
    rq.dev = dsk;
    (void)ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    note_run(c, res.machine_time, res.reason);
    release_boot(&b);
    icount = get_u32(isk->header);
    dcount = get_u32(dsk->header);
    if (res.reason != (uint32_t)DS_RUN_REASON_UNTIL || get_u32(isk->header + 4) != 0u || get_u32(dsk->header + 4) != 0u) {
        harness_error(c, "the dev log run did not reach its deadline or a sink lost a record: the log is evidence here and would be incomplete");
    } else {
        snprintf(msg, sizeof msg, "the dev log holds %u records and offered %llu, the case lists %u", (unsigned)dcount, (unsigned long long)get_u64(dsk->header + 8), nitems);
        check(c, dcount == nitems && get_u64(dsk->header + 8) == (uint64_t)nitems, msg);
        for (i = 0u; i < nitems && i < dcount; i++) {
            const unsigned char *r = dl_rec(drec, i);
            uint32_t at;
            uint32_t hits = 0u;
            uint64_t stamp = 0u;
            uint32_t a_reg = 0u;
            unsigned j;
            int zeros = 1;

            for (at = 0u; at < icount; at++) {
                const unsigned char *ir = irec + (size_t)at * DS_INSN_RECORD_BYTES;

                if (get_u16(ir + 8) == items[i].pc) {
                    hits++;
                    stamp = get_u64(ir);
                    a_reg = ir[11];
                }
            }
            snprintf(msg, sizeof msg, "dev record %u: the instruction at pc %04X is not in the insn log exactly once", i, (unsigned)items[i].pc);
            check(c, hits == 1u, msg);
            snprintf(msg, sizeof msg, "dev record %u: the stamp is not the start stamp of the instruction at pc %04X in the insn log plus %u cycles of %u master clocks (the access stamp, D-92)", i, (unsigned)items[i].pc, (unsigned)items[i].cycle, DL_MCLK_LOW);
            check(c, get_u64(r) == stamp + (uint64_t)items[i].cycle * (uint64_t)DL_MCLK_LOW, msg);
            snprintf(msg, sizeof msg, "dev record %u: the pc field is not %04X", i, (unsigned)items[i].pc);
            check(c, get_u16(r + 16) == items[i].pc, msg);
            snprintf(msg, sizeof msg, "dev record %u: the kind byte is not %u", i, (unsigned)items[i].kind);
            check(c, (uint32_t)r[13] == items[i].kind, msg);
            snprintf(msg, sizeof msg, "dev record %u: the physical address is not %06X", i, (unsigned)items[i].phys);
            check(c, get_u32(r + 8) == items[i].phys, msg);
            snprintf(msg, sizeof msg, "dev record %u: the value is not %02X", i, (unsigned)(items[i].value_is_a ? a_reg : items[i].value));
            check(c, (uint32_t)r[12] == (items[i].value_is_a ? a_reg : items[i].value), msg);
            snprintf(msg, sizeof msg, "dev record %u: the flags are not %u (penalty bit below $800 in the page, physical bit for the ST writes)", i, dl_flags(items[i].phys, items[i].kind));
            check(c, (unsigned)r[14] == dl_flags(items[i].phys, items[i].kind), msg);
            for (j = 18u; j < DS_DEV_RECORD_BYTES; j++) {
                zeros = zeros && r[j] == 0u;
            }
            snprintf(msg, sizeof msg, "dev record %u: the spare bytes are not zero", i);
            check(c, zeros, msg);
            if (i > 0u) {
                snprintf(msg, sizeof msg, "dev record %u: the stamps of the log are not in order", i);
                check(c, get_u64(r) >= get_u64(dl_rec(drec, i - 1u)), msg);
            }
        }
        check(c, ob_tail_ok(drec, (size_t)DL_DEV_CAP * DS_DEV_RECORD_BYTES, (size_t)dcount * DS_DEV_RECORD_BYTES, 0), "the core wrote into the dev record memory past the count");
    }
    free(isk);
    free(dsk);
    free(irec);
    free(drec);
    snprintf(got, gotcap, "records=%u", (unsigned)dcount);
}

/* the bus-level view of the window rule (D-119): one instance, one dev sink, accesses driven through the choke point with MPR0 mapped to the hardware page. The expected records are written from the rule itself
 * (writes to windows 0 to 5 and reads of window 4 are logged, everything else is not), the aux byte from the instance's own latch after each access. A NULL bus logs nothing. */
struct dl_exp {
    uint32_t phys;
    uint32_t value;
    uint32_t kind;
    uint32_t aux;
};

static void dl_windows(struct ctx *c, const char *rom, const char *request, char *got, size_t gotcap)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    struct dl_exp want[16];
    unsigned nexp = 0u;
    union {
        uint64_t align;
        unsigned char b[DL_WIN_CAP * 4u * DS_DEV_RECORD_BYTES];
    } big;
    struct ds_dev_sink dsk;
    struct ds_obs_log log;
    struct ds_bus bus;
    struct ds_bus bus_off;
    unsigned k;
    unsigned i;
    unsigned nrec;
    char msg[160];
    const char *err = boot_built_rom(rom, request, &b, &ci);

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    memset(&big, 0, sizeof big);
    memset(&dsk, 0, sizeof dsk);
    dsk.records = big.b;
    dsk.capacity = DL_WIN_CAP * 4u;
    ds_obs_begin(&log, NULL, &dsk);
    memset(&bus, 0, sizeof bus);
    memset(&bus_off, 0, sizeof bus_off);
    bus.obs = &log;
    bus.start = w->machine_time; /* no instruction runs in the walk: a cycle length of 0 stamps every access at the instance's machine time */
    bus_off.start = w->machine_time;
    ds_mpr_write(w, 0u, 0xFFu); /* logical $0000-$1FFF is the hardware page */
    /* a write to every window: offsets $000 + k * $400 + 5 */
    for (k = 0u; k < 8u; k++) {
        uint32_t off = k * 0x400u + 5u;

        ds_bus_write(w, &bus, (uint16_t)off, (uint8_t)(0x10u + k));
        if (k <= 5u) {
            want[nexp].phys = 0x1FE000u | off;
            want[nexp].value = 0x10u + k;
            want[nexp].kind = (uint32_t)DS_DEV_WRITE;
            want[nexp].aux = (uint32_t)w->io_buffer;
            nexp++;
        }
    }
    /* a read of every window: only window 4 is logged; the value it returns and the latch after it are what the record must hold */
    for (k = 0u; k < 8u; k++) {
        uint32_t off = k * 0x400u + 1u;
        uint8_t v = ds_bus_read(w, &bus, (uint16_t)off);

        if (k == 4u) {
            want[nexp].phys = 0x1FE000u | off;
            want[nexp].value = (uint32_t)v;
            want[nexp].kind = (uint32_t)DS_DEV_READ;
            want[nexp].aux = (uint32_t)w->io_buffer;
            nexp++;
        }
    }
    /* accesses that must not log: a NULL bus, RAM, ROM and an unmapped bank through other MPRs */
    ds_bus_write(w, &bus_off, 0x0405u, 0x77u);
    (void)ds_bus_read(w, &bus_off, 0x1001u);
    ds_mpr_write(w, 1u, 0xF8u);
    ds_mpr_write(w, 2u, 0x00u);
    ds_mpr_write(w, 3u, 0x80u);
    ds_bus_write(w, &bus, 0x2010u, 0x55u);
    (void)ds_bus_read(w, &bus, 0x2010u);
    ds_bus_write(w, &bus, 0x4010u, 0x55u);
    (void)ds_bus_read(w, &bus, 0x4010u);
    ds_bus_write(w, &bus, 0x6010u, 0x55u);
    (void)ds_bus_read(w, &bus, 0x6010u);
    /* the physical writes of ST0 to ST2 are logged as ST with the penalty and physical flags; any other target is dropped and logs nothing */
    for (k = 0u; k < 4u; k++) {
        static const uint32_t target[4] = {0x1FE000u, 0x1FE002u, 0x1FE003u, 0x1FE001u};

        ds_bus_write_phys(w, &bus, target[k], (uint8_t)(0x20u + k));
        if (k < 3u) {
            want[nexp].phys = target[k];
            want[nexp].value = 0x20u + k;
            want[nexp].kind = (uint32_t)DS_DEV_ST;
            want[nexp].aux = (uint32_t)w->io_buffer;
            nexp++;
        }
    }
    ds_obs_end(&log);
    nrec = get_u32(dsk.header);
    snprintf(msg, sizeof msg, "the window walk offered %llu records and stored %u, the rule predicts %u (the sink holds %u)", (unsigned long long)get_u64(dsk.header + 8), nrec, nexp, DL_WIN_CAP * 4u);
    check(c, get_u64(dsk.header + 8) == (uint64_t)nexp && nexp <= DL_WIN_CAP * 4u && nrec == nexp && get_u32(dsk.header + 4) == 0u, msg);
    for (i = 0u; i < nexp && i < nrec; i++) {
        const unsigned char *r = dl_rec(big.b, i);
        unsigned j;
        int zeros = 1;

        snprintf(msg, sizeof msg, "window walk record %u: physical address, value, kind, flags or aux differ from the rule", i);
        check(c, get_u32(r + 8) == want[i].phys && (uint32_t)r[12] == want[i].value && (uint32_t)r[13] == want[i].kind && (unsigned)r[14] == dl_flags(want[i].phys, want[i].kind) && (uint32_t)r[15] == want[i].aux, msg);
        snprintf(msg, sizeof msg, "window walk record %u: the stamp or the pc is not the instance's (no instruction runs here)", i);
        check(c, get_u64(r) == w->machine_time && get_u16(r + 16) == (uint32_t)w->pc, msg);
        for (j = 18u; j < DS_DEV_RECORD_BYTES; j++) {
            zeros = zeros && r[j] == 0u;
        }
        check(c, zeros, "a window walk record has a nonzero spare byte");
    }
    /* the same walk offered to a sink of 4 records: drop-newest, the first 4 records kept, lost counted, seq the full count (the hook level of the policy; ds_run level is the observer matrix) */
    {
        union {
            uint64_t align;
            unsigned char b[4u * DS_DEV_RECORD_BYTES];
        } small;
        struct ds_dev_sink ssk;
        struct ds_obs_log slog;
        struct ds_bus sbus;
        unsigned n;

        memset(&small, 0xA5, sizeof small);
        memset(&ssk, 0xA5, sizeof ssk);
        ssk.records = small.b;
        ssk.capacity = 4u;
        ds_obs_begin(&slog, NULL, &ssk);
        memset(&sbus, 0, sizeof sbus);
        sbus.obs = &slog;
        for (n = 0u; n < 10u; n++) {
            ds_obs_dev(sbus.obs, w, w->machine_time, 0x1FE000u + n, n, (uint32_t)DS_DEV_WRITE, 0u);
        }
        ds_obs_end(&slog);
        check(c, get_u32(ssk.header) == 4u && get_u32(ssk.header + 4) == 6u && get_u64(ssk.header + 8) == 10u, "a 4-record dev sink offered 10 records does not read count 4, lost 6, seq 10");
        for (n = 0u; n < 4u; n++) {
            check(c, get_u32(dl_rec(small.b, n) + 8) == 0x1FE000u + n, "a 4-record dev sink does not hold the first four records (drop-newest)");
        }
    }
    release_boot(&b);
    snprintf(got, gotcap, "records=%u", nrec);
}

void kind_dev_log(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    struct dl_item items[DL_ITEMS_MAX];
    unsigned nitems = 0u;
    int windows = 0;
    uint64_t until = 0u;

    if (dl_parse(k->val[K_EXPECT], &windows, rom, sizeof rom, items, &nitems) != 0 || (!windows && (!k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u))) {
        harness_error(c, "a dev_log case needs until_mclk and expect = mode=guest;rom=NAME;rec=PC:KIND:PHYS:VALUE:CYCLE;rec=... (KIND W, R or ST; VALUE a number or A; CYCLE 0 to 63) or mode=windows;rom=NAME");
        return;
    }
    if (windows) {
        dl_windows(c, rom, k->val[K_REQUEST], got, gotcap);
    } else {
        dl_guest(c, rom, k->val[K_REQUEST], until, items, nitems, got, gotcap);
    }
}

/* ---- --trace-out and kind trace_out_rights (plan 03-19; D-120, D-56, D-80, T-03-51, T-03-52) ----
 * The runner writes an instruction trace and a device log only for `--trace-out CASE_ID`, only for a case whose fixture is an authored image listed in build/gate/fixtures-authored.used (written by scripts/fixture-build in the same
 * gate run) or a synthetic recipe, and only to build/gate/traces/ID.insn and ID.dev, a path the runner builds itself from an id of the form [a-z][a-z0-9_]*. Nothing reaches stdout but ids, record counts, sizes and sha256 values. */

#define TRACE_DIR "build/gate/traces/"
#define TRACE_USED "build/gate/fixtures-authored.used"
#define TRACE_ID_MAX 64u
#define TRACE_DEV_CAP 65536u

const char *trace_out_id;

int obskinds_arg(int argc, char **argv, int *a)
{
    if (strcmp(argv[*a], "--trace-out") == 0 && *a + 1 < argc) {
        trace_out_id = argv[*a + 1];
        *a += 1;
        return 1;
    }
    return 0;
}

enum { TR_OK = 0, TR_BAD_ID = 1, TR_NO_CASE = 2, TR_NOT_PERMITTED = 3, TR_NO_DEADLINE = 4, TR_FAILED = 5 };

static const char *trace_reason(int r)
{
    switch (r) {
    case TR_BAD_ID:
        return "the id is not of the form [a-z][a-z0-9_]* with at most 64 characters";
    case TR_NO_CASE:
        return "no case carries that id";
    case TR_NOT_PERMITTED:
        return "the fixture of the case is neither an authored image listed in build/gate/fixtures-authored.used nor a synthetic recipe";
    case TR_NO_DEADLINE:
        return "the case has no until_mclk to run to";
    default:
        return "the trace could not be produced";
    }
}

static int trace_id_valid(const char *id)
{
    size_t n = strlen(id);

    return n >= 1u && n <= TRACE_ID_MAX && id[0] >= 'a' && id[0] <= 'z' && strspn(id, "abcdefghijklmnopqrstuvwxyz0123456789_") == n;
}

/* 1 when NAME is the first token of a line of the authored used list (a missing or unreadable list lists nothing: the rule fails closed) */
static int trace_authored_listed(const char *name)
{
    char *text = read_file(TRACE_USED);
    char *line = text;
    size_t n = strlen(name);
    int found = 0;

    while (text != NULL && *line != '\0' && !found) {
        char *nl = strchr(line, '\n');

        found = strncmp(line, name, n) == 0 && line[n] == ' ';
        line = nl != NULL ? nl + 1 : line + strlen(line);
    }
    free(text);
    return found;
}

static int trace_fixture_permit(const char *fixture)
{
    char name[48];

    if (authored_name(fixture, name, sizeof name) == 0) {
        return trace_authored_listed(name) ? TR_OK : TR_NOT_PERMITTED;
    }
    return synth_size(fixture) != 0u ? TR_OK : TR_NOT_PERMITTED;
}

/* the paths the runner derives from a valid id: 0 on success */
static int trace_paths(const char *id, char *insn, char *dev, size_t cap)
{
    int a;
    int b;

    if (!trace_id_valid(id)) {
        return -1;
    }
    a = snprintf(insn, cap, TRACE_DIR "%s.insn", id);
    b = snprintf(dev, cap, TRACE_DIR "%s.dev", id);
    return a > 0 && b > 0 && (size_t)a < cap && (size_t)b < cap ? 0 : -1;
}

/* the checks that come before any file is opened: the id, the case, its fixture and its deadline */
static int trace_request(const char *id, const struct kase **kout, uint64_t *until)
{
    size_t i;

    if (!trace_id_valid(id)) {
        return TR_BAD_ID;
    }
    for (i = 0u; i < case_count(); i++) {
        const struct kase *k = case_get(i);

        if (strcmp(k->val[K_ID], id) == 0) {
            if (!k->has[K_FIXTURE] || trace_fixture_permit(k->val[K_FIXTURE]) != TR_OK) {
                return TR_NOT_PERMITTED;
            }
            if (!k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], until) != 0 || *until == 0u) {
                return TR_NO_DEADLINE;
            }
            *kout = k;
            return TR_OK;
        }
    }
    return TR_NO_CASE;
}

static void trace_hex(const uint8_t *d, size_t n, char *out)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0u; i < n; i++) {
        out[2u * i] = digits[(d[i] >> 4u) & 0xFu];
        out[2u * i + 1u] = digits[d[i] & 0xFu];
    }
    out[2u * n] = '\0';
}

/* write bytes to path and print the line of that file: record count, lost count, size and sha256 (0 on success) */
static int trace_file(const char *id, const char *what, const char *path, const unsigned char *rec, uint32_t count, uint32_t lost, size_t unit)
{
    size_t bytes = (size_t)count * unit;
    FILE *fp = fopen(path, "wb");
    uint8_t digest[DS_SHA256_BYTES];
    char hex[2u * DS_SHA256_BYTES + 1u];
    int ok;

    if (fp == NULL) {
        return -1;
    }
    ok = (bytes == 0u || fwrite(rec, 1u, bytes, fp) == bytes) && fclose(fp) == 0;
    if (!ok) {
        return -1;
    }
    ds_sha256(rec != NULL ? rec : (const uint8_t *)"", bytes, digest);
    trace_hex(digest, DS_SHA256_BYTES, hex);
    emit_out("TRACE %s %s records=%u lost=%u bytes=%llu sha256=%s\n", id, what, (unsigned)count, (unsigned)lost, (unsigned long long)bytes, hex);
    return 0;
}

/* the whole request: validate, run the case's image to its deadline with both sinks, write the two files; 0 or a TR_ code (nothing is opened before the request is permitted) */
static int trace_out_do(const char *id)
{
    const struct kase *k = NULL;
    uint64_t until = 0u;
    char ipath[128];
    char dpath[128];
    char name[48];
    struct booted b;
    struct spec s;
    const struct ds_instance *ci = NULL;
    struct ds_insn_sink *isk;
    struct ds_dev_sink *dsk;
    unsigned char *irec;
    unsigned char *drec;
    struct ds_run_request rq;
    struct ds_run_result res;
    const char *err;
    int r = trace_request(id, &k, &until);

    if (r != TR_OK) {
        return r;
    }
    if (trace_paths(id, ipath, dpath, sizeof ipath) != 0) {
        return TR_BAD_ID;
    }
    if (authored_name(k->val[K_FIXTURE], name, sizeof name) == 0) {
        err = boot_built_rom(name, k->val[K_REQUEST], &b, &ci);
    } else {
        err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &ci);
    }
    if (err != NULL) {
        return TR_FAILED;
    }
    isk = (struct ds_insn_sink *)calloc(1u, sizeof *isk);
    dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    irec = (unsigned char *)malloc((size_t)DS_SINK_MAX_RECORDS * DS_INSN_RECORD_BYTES);
    drec = (unsigned char *)malloc((size_t)TRACE_DEV_CAP * DS_DEV_RECORD_BYTES);
    if (isk == NULL || dsk == NULL || irec == NULL || drec == NULL) {
        r = TR_FAILED;
    } else {
        isk->records = irec;
        isk->capacity = DS_SINK_MAX_RECORDS;
        dsk->records = drec;
        dsk->capacity = TRACE_DEV_CAP;
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.insn = isk;
        rq.dev = dsk;
        if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED
            || trace_file(id, "insn", ipath, irec, get_u32(isk->header), get_u32(isk->header + 4), DS_INSN_RECORD_BYTES) != 0
            || trace_file(id, "dev", dpath, drec, get_u32(dsk->header), get_u32(dsk->header + 4), DS_DEV_RECORD_BYTES) != 0) {
            r = TR_FAILED;
        }
    }
    free(isk);
    free(dsk);
    free(irec);
    free(drec);
    release_boot(&b);
    return r;
}

int trace_out_main(void)
{
    int r = trace_out_do(trace_out_id);

    if (r != TR_OK) {
        emit_err("HARNESS --trace-out refused: %s\n", trace_reason(r));
        return 2;
    }
    return 0;
}

/* a file at path exists and can be opened for reading */
static int file_exists(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/*
 * kind trace_out_rights: expect is a list of items separated by ';', each at most 40 characters, at least one of each of the first four:
 *   allow_fixture=KEY  the fixture key is permitted (authored and listed, or a synthetic recipe)      deny_fixture=KEY  it is not (an unlisted authored image, none, a path, a commercial name)
 *   allow_case=ID      the request for the case passes every check that comes before a file is opened, and the paths are TRACE_DIR/ID.insn and .dev with no further slash
 *   deny_case=ID       the request is refused (a case whose fixture is not permitted, or no such case) and no file appears        bad_id=ID  the id is not of the form [a-z][a-z0-9_]* and is refused with no path built and no file at any naive join
 * Nothing is written by this kind: the writing of an allowed request is the --trace-out run of scripts/conformance-run. E0: the rule is a literal one of this plan.
 */
void kind_trace_out_rights(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned classes = 0u;
    unsigned n = 0u;
    char msg[160];

    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the trace_out_rights expect list is too long");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        const char *v;
        char ipath[128];
        char dpath[128];

        if (eq == NULL || strlen(eq + 1) > 40u) {
            harness_error(c, "a trace_out_rights item is KEY=VALUE with a value of at most 40 characters");
            return;
        }
        *eq = '\0';
        v = eq + 1;
        n++;
        if (strcmp(item, "allow_fixture") == 0) {
            classes |= 1u;
            snprintf(msg, sizeof msg, "the fixture key %s is refused but is authored and listed or a synthetic recipe", v);
            check(c, trace_fixture_permit(v) == TR_OK, msg);
        } else if (strcmp(item, "deny_fixture") == 0) {
            classes |= 2u;
            snprintf(msg, sizeof msg, "the fixture key %s is permitted but is neither authored and listed nor a synthetic recipe", v);
            check(c, trace_fixture_permit(v) == TR_NOT_PERMITTED, msg);
        } else if (strcmp(item, "allow_case") == 0) {
            const struct kase *kk = NULL;
            uint64_t until = 0u;
            int ok;

            classes |= 4u;
            snprintf(msg, sizeof msg, "the trace request for case %s is refused before any file is opened", v);
            check(c, trace_request(v, &kk, &until) == TR_OK, msg);
            ok = trace_paths(v, ipath, dpath, sizeof ipath) == 0 && strncmp(ipath, TRACE_DIR, strlen(TRACE_DIR)) == 0 && strncmp(dpath, TRACE_DIR, strlen(TRACE_DIR)) == 0 && strchr(ipath + strlen(TRACE_DIR), '/') == NULL
                 && strchr(dpath + strlen(TRACE_DIR), '/') == NULL && strstr(ipath, "..") == NULL && strstr(dpath, "..") == NULL;
            snprintf(msg, sizeof msg, "the paths of case %s are not directly under build/gate/traces/", v);
            check(c, ok, msg);
        } else if (strcmp(item, "deny_case") == 0) {
            int before;

            classes |= 8u;
            snprintf(ipath, sizeof ipath, TRACE_DIR "%s.insn", v);
            snprintf(dpath, sizeof dpath, TRACE_DIR "%s.dev", v);
            before = file_exists(ipath) || file_exists(dpath); /* a file of an earlier explicit request (or of a case-folded name on a case-insensitive filesystem) may be there: the refusal must not create one */
            snprintf(msg, sizeof msg, "the trace request for case %s was not refused with the not-permitted or no-such-case reason", v);
            check(c, trace_out_do(v) == TR_NOT_PERMITTED || trace_out_do(v) == TR_NO_CASE, msg);
            snprintf(msg, sizeof msg, "a refused trace request for case %s left a file", v);
            check(c, before || (!file_exists(ipath) && !file_exists(dpath)), msg);
        } else if (strcmp(item, "bad_id") == 0) {
            char ipath2[128];
            int before;

            classes |= 16u;
            snprintf(ipath, sizeof ipath, TRACE_DIR "%s.insn", v);
            snprintf(ipath2, sizeof ipath2, "%s.insn", v);
            snprintf(dpath, sizeof dpath, TRACE_DIR "%s.dev", v);
            before = file_exists(ipath) || file_exists(ipath2) || file_exists(dpath);
            snprintf(msg, sizeof msg, "the id '%s' was accepted or a path was built for it", v);
            check(c, !trace_id_valid(v) && trace_paths(v, ipath, dpath, sizeof ipath) != 0 && trace_out_do(v) == TR_BAD_ID, msg);
            snprintf(ipath, sizeof ipath, TRACE_DIR "%s.insn", v);
            snprintf(msg, sizeof msg, "a refused id '%s' left a file at a naive join of the id", v);
            check(c, before || (!file_exists(ipath) && !file_exists(ipath2) && !file_exists(dpath)), msg);
        } else {
            harness_error(c, "a trace_out_rights item is allow_fixture, deny_fixture, allow_case, deny_case or bad_id");
            return;
        }
    }
    if (classes != 31u) {
        harness_error(c, "a trace_out_rights case needs at least one item of each of the five kinds");
        return;
    }
    snprintf(got, gotcap, "items=%u", n);
}
