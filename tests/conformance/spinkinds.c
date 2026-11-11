/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * spinkinds.c - the two case kinds that give the gate live producers of the outcomes timeout and cancelled (plan 18; D-57, D-58, D-77).
 * Neither kind is in the real inventory: they exist only in the control mini-roots tests/gate/timeout-budget and tests/gate/cancel-spin,
 * which scripts/conformance-run control and control-signal run through this very runner. Test code only, never installed.
 *   budget_spin   boots the request over the fixture, then calls ds_run with absolute deadlines start+M, start+2M, ... and finally start+N, with a stop
 *                 function that never fires (plan 03-02: the relative budget of Phase 2 is gone, D-91; the expect text keeps its meaning). expect is
 *                 "budget=N,step=M". The deadline is machine time in master clocks, never the host clock, so the outcome is deterministic: when
 *                 every call ends with UNTIL the case ends with the outcome timeout (D-57, D-95). Machine time after the run must be the first
 *                 instruction boundary at or past start+N, less than ds_max_step_mclk() past it (an assertion; a wrong value is a guest_failure,
 *                 which wins over the timeout).
 *   spin_forever  boots the request, then calls ds_run with ever growing absolute deadlines without end and never returns, so no RESULT line is ever printed for it.
 *                 The supervisor (conformance-run supervise) is signalled with TERM, kills this process and writes the cancelled line.
 *                 The host wall watchdog of the supervisor is the only other way out.
 * Plan 03-01 adds the first guest kind here (plan 03-04 moves it into guestkinds.c):
 *   guest         boots a ROM that scripts/fixture-build assembled (build/gate/fixtures/NAME.pce), runs it with ds_run to an absolute deadline
 *                 and judges the DSV1 verdict block that the guest wrote into work RAM, read from raw inst->ram8k (D-108: no peek API).
 *                 expect is "rom=NAME;id=N;checks=N;until=N" (until is machine time in master clocks, absolute from boot).
 *                 Outcome mapping, one reading of D-124 (the judge below and ds_proto.s carry the same layout):
 *                   harness_failure  a harness-side precondition fails: the ROM is unreadable, the expect is malformed, a DSV1 block is already
 *                                    present before the first step, ds_run refuses the request, the block reports zero checks, or its check
 *                                    count or fixture id differs from the case
 *                   guest_failure    the guest wrote a fault: any result byte other than $50 (so $46 and $00 too), an invalid state or complement,
 *                                    a bad version, trailer, summary, progress or checksum, nonzero error flags or reserved bytes
 *                   timeout          the run ended at the deadline and the block is absent or not done (and no guest fault is visible)
 *                   pass             a consistent done block in which all n_checks result bytes are $50; assertions = n_checks
 *                 DSV1 layout: +00 "DSV1", +04 state ($00 idle, $01 running, $A5 done), +05 complement, +06 version 1, +07 n_checks, +08 fixture id,
 *                 +09 pass, +0A fail, +0B notrun, +0C progress, +0D error flags, +10..+6F result per check ($00 not run, $50 pass, $46 fail),
 *                 +80..+DF observed byte per check, +E0 +E1 16-bit little-endian sum of +06..+DF, +FE +FF trailer $5A $A5, every other byte zero.
 *                 The block is at work-RAM offset $1F00 (logical $3F00 while MPR1 = $F8). Evidence class E0: the verdict is what the authored guest wrote.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

/* read "budget=N,step=M" into the two numbers; 0 on success */
static int parse_budget(const char *text, uint64_t *budget, uint64_t *step)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *kv;
    int have_budget = 0;
    int have_step = 0;

    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((kv = pk_next_item(&rest, ',')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(kv, "budget") == 0 && parse_u64(eq + 1, budget) == 0) {
            have_budget = 1;
        } else if (strcmp(kv, "step") == 0 && parse_u64(eq + 1, step) == 0) {
            have_step = 1;
        } else {
            return -1;
        }
    }
    return have_budget && have_step ? 0 : -1;
}

/* one ds_run to an absolute deadline with no stop function: true when it ended on the deadline */
static int run_deadline(struct ds_instance *inst, uint64_t until)
{
    struct ds_run_request rq;
    struct ds_run_result out;

    memset(&rq, 0, sizeof rq);
    memset(&out, 0, sizeof out);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    return ds_run(inst, &rq, &out) == (uint32_t)DS_RUN_REASON_UNTIL;
}

void kind_spin(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    struct ds_instance *w;
    uint64_t budget = 0u;
    uint64_t step = 0u;
    uint64_t start;
    uint64_t at;
    int ended = 1;
    const char *err;
    int forever = strcmp(k->val[K_KIND], "spin_forever") == 0;

    if (!forever && (parse_budget(k->val[K_EXPECT], &budget, &step) != 0 || step == 0u)) {
        harness_error(c, "a budget_spin expect is not budget=N,step=M with M above 0");
        return;
    }
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    if (forever) {
        uint64_t until = 0u;

        for (;;) {
            until += UINT64_C(1000);
            (void)run_deadline(w, until);
        }
    }
    start = inst->machine_time;
    at = start;
    while (at - start < budget && ended) {
        at = budget - (at - start) > step ? at + step : start + budget;
        ended = run_deadline(w, at);
    }
    check(c, ended, "ds_run did not end on an absolute deadline of the budget");
    check(c, inst->machine_time >= start + budget && inst->machine_time - (start + budget) < ds_max_step_mclk(),
          "machine time after the last deadline is not the first instruction boundary at or past the start plus the budget");
    if (ended && !c->failed) {
        c->timeout = 1;
        snprintf(c->why, sizeof c->why, "the machine-time deadline of %llu master clocks was reached without a stop", (unsigned long long)budget);
    }
    snprintf(got, gotcap, "time=%llu", (unsigned long long)inst->machine_time);
    release_boot(&b);
}

/* ---- kind guest (plan 03-01): the DSV1 verdict of an authored ca65 fixture ---- */

#define V_OFF 0x1F00u
#define V_SIZE 0x100u
#define V_MAX_CHECKS 96u
#define V_RES 0x10u
#define V_OBS 0x80u
#define R_PASS 0x50u
#define R_FAIL 0x46u

/* a block is present when its magic is there, or when it is in the done state with a valid complement (a precondition fault before the first step) */
static int has_magic(const uint8_t *b)
{
    return b[0] == 0x44u && b[1] == 0x53u && b[2] == 0x56u && b[3] == 0x31u;
}

static int block_present(const uint8_t *b)
{
    return has_magic(b) || (b[4] == 0xA5u && b[5] == 0x5Au);
}

/* one guest-written fault: it counts as one failed assertion and keeps the first reason */
static void fault(struct ctx *c, const char *what)
{
    check(c, 0, what);
}

/* the verdict block that the run left in b (256 bytes); want_id and want_checks come from the case. Sets exactly one outcome on c. */
static void judge_dsv1(struct ctx *c, const uint8_t *b, unsigned want_id, unsigned want_checks, char *got, size_t gotcap)
{
    unsigned state = b[4];
    unsigned nchk = b[7];
    unsigned i;
    unsigned npass = 0u;
    unsigned nfail = 0u;
    unsigned nnot = 0u;
    unsigned sum = 0u;
    int any_fail = 0;
    char msg[160];

    if (!has_magic(b)) {
        c->timeout = 1;
        snprintf(c->why, sizeof c->why, "the run ended at the deadline and the guest wrote no DSV1 block");
        snprintf(got, gotcap, "block=absent");
        return;
    }
    snprintf(got, gotcap, "state=%02x n=%u", state, nchk);
    if (!((state == 0x00u && b[5] == 0xFFu) || (state == 0x01u && b[5] == 0xFEu) || (state == 0xA5u && b[5] == 0x5Au))) {
        fault(c, "the DSV1 state or its complement is invalid");
        return;
    }
    for (i = 0u; i < V_MAX_CHECKS; i++) {
        any_fail = any_fail || b[V_RES + i] == R_FAIL;
    }
    if (state != 0xA5u) {
        /* not done: a fault the guest already wrote outranks the timeout (outcome_of lets failed win over timeout) */
        if (b[0x0D] != 0u) {
            fault(c, "the guest set an error flag before it finished");
        } else if (any_fail) {
            fault(c, "a check result is fail and the guest did not finish");
        } else {
            c->timeout = 1;
            snprintf(c->why, sizeof c->why, "the run ended at the deadline before the guest set the DSV1 done state");
        }
        return;
    }
    if (nchk == 0u) {
        harness_error(c, "the DSV1 block reports zero checks, so no assertion could be made");
        return;
    }
    if (nchk != want_checks || b[8] != want_id) {
        harness_error(c, "the check count or the fixture id of the DSV1 block differs from the case");
        return;
    }
    if (b[6] != 1u) {
        fault(c, "the DSV1 version is not 1");
        return;
    }
    if (nchk > V_MAX_CHECKS) {
        fault(c, "the DSV1 check count is above 96");
        return;
    }
    if (b[0xFE] != 0x5Au || b[0xFF] != 0xA5u) {
        fault(c, "the DSV1 trailer is wrong");
        return;
    }
    for (i = 0u; i < V_SIZE; i++) {
        int reserved = i == 0x0Eu || i == 0x0Fu || (i >= 0x70u && i <= 0x7Fu) || (i >= 0xE2u && i <= 0xFDu);
        int unused = i >= V_RES + nchk && i < V_RES + V_MAX_CHECKS;
        int unused_obs = i >= V_OBS + nchk && i < V_OBS + V_MAX_CHECKS;

        if ((reserved || unused || unused_obs) && b[i] != 0u) {
            fault(c, "a DSV1 byte that must be zero is not");
            return;
        }
        if (i >= 6u && i <= 0xDFu) {
            sum += b[i];
        }
    }
    for (i = 0u; i < nchk; i++) {
        unsigned r = b[V_RES + i];

        npass += r == R_PASS;
        nfail += r == R_FAIL;
        nnot += r == 0u;
        if (r != R_PASS && r != R_FAIL && r != 0u) {
            fault(c, "a DSV1 result byte is not $00, $50 or $46");
            return;
        }
    }
    if (b[9] != npass || b[0x0A] != nfail || b[0x0B] != nnot || b[0x0C] != nchk) {
        fault(c, "the DSV1 summary or progress differs from the one recomputed from the result bytes");
        return;
    }
    if (b[0x0D] != 0u) {
        fault(c, "the DSV1 error flags are not zero");
        return;
    }
    if ((sum & 0xFFFFu) != (unsigned)(b[0xE0] | (b[0xE1] << 8))) {
        fault(c, "the DSV1 checksum differs from the sum recomputed from raw RAM");
        return;
    }
    snprintf(got, gotcap, "pass=%u fail=%u notrun=%u", npass, nfail, nnot);
    for (i = 0u; i < nchk; i++) {
        snprintf(msg, sizeof msg, "check %u of the guest did not pass (result byte $%02x)", i + 1u, (unsigned)b[V_RES + i]);
        check(c, b[V_RES + i] == R_PASS, msg);
    }
}

/* read a whole file into a heap buffer of at most cap bytes; NULL on any failure */
static unsigned char *read_rom(const char *path, size_t cap, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    unsigned char *buf;
    long size;

    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) <= 0 || (size_t)size > cap || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    buf = (unsigned char *)malloc((size_t)size);
    if (buf == NULL || fread(buf, 1u, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *len = (size_t)size;
    return buf;
}

/* rom=NAME;id=N;checks=N;until=N into the four fields; 0 on success. NAME is lowercase letters, digits and hyphens only (it becomes a file name). */
static int parse_guest(const char *text, char *rom, size_t romcap, uint64_t *id, uint64_t *checks, uint64_t *until)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *kv;
    int have = 0;

    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((kv = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(kv, '=');

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(kv, "rom") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < romcap && strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(eq + 1)) {
            memcpy(rom, eq + 1, strlen(eq + 1) + 1u);
            have |= 1;
        } else if (strcmp(kv, "id") == 0 && parse_u64(eq + 1, id) == 0 && *id <= 255u) {
            have |= 2;
        } else if (strcmp(kv, "checks") == 0 && parse_u64(eq + 1, checks) == 0 && *checks <= 255u) {
            have |= 4;
        } else if (strcmp(kv, "until") == 0 && parse_u64(eq + 1, until) == 0) {
            have |= 8;
        } else {
            return -1;
        }
    }
    return have == 15 ? 0 : -1;
}

/*
 * Boot the built authored image build/gate/fixtures/ROM.pce over the request text (the case's request key; empty gives the defaults of parse_spec).
 * NULL and b and the accepted instance on success, otherwise why not: a fault here is a harness fault of the case, never a verdict on the guest.
 * Shared by the guest kind and the run kinds of stepkinds.c; release_boot frees what it allocated.
 */
const char *boot_built_rom(const char *rom, const char *request, struct booted *b, const struct ds_instance **inst)
{
    char path[96];
    struct spec s;
    struct ds_boot_request rq;
    struct ds_boot_result res;
    const char *err;
    size_t len = 0u;

    snprintf(path, sizeof path, "build/gate/fixtures/%s.pce", rom);
    b->mem = NULL;
    b->content = read_rom(path, (size_t)DS_ROM_CAP, &len);
    if (b->content == NULL) {
        return "the built fixture image is missing or unreadable: run scripts/fixture-build first";
    }
    err = parse_spec(&s, request, len);
    if (err != NULL || s.ptr != PTR_OK) {
        release_boot(b);
        return err != NULL ? err : "a guest case needs ptr=ok";
    }
    b->mem = (unsigned char *)malloc(ds_instance_size());
    if (b->mem == NULL) {
        release_boot(b);
        return "out of memory for the instance";
    }
    fill_request(&rq, &s, b->content);
    memset(&res, 0xff, sizeof res);
    if (ds_boot(b->mem, ds_instance_size(), &rq, &res) != 0u || res.instance == NULL) {
        release_boot(b);
        return "the built fixture image was refused at boot";
    }
    *inst = res.instance;
    return NULL;
}

void kind_guest(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    uint64_t id = 0u;
    uint64_t checks = 0u;
    uint64_t until = 0u;
    struct booted b;
    struct ds_run_request run;
    struct ds_run_result out;
    const struct ds_instance *inst = NULL;
    const char *err;

    if (parse_guest(k->val[K_EXPECT], rom, sizeof rom, &id, &checks, &until) != 0) {
        harness_error(c, "a guest expect is not rom=NAME;id=N;checks=N;until=N");
        return;
    }
    err = boot_built_rom(rom, k->val[K_REQUEST], &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    /* the precondition of D-124: no verdict block is present before the first step */
    if (block_present(inst->ram8k + V_OFF)) {
        harness_error(c, "a DSV1 block is present in work RAM before the first step");
        release_boot(&b);
        return;
    }
    memset(&run, 0, sizeof run);
    memset(&out, 0, sizeof out);
    run.struct_size = (uint32_t)sizeof run;
    run.until_mclk = until;
    if (ds_run((struct ds_instance *)(void *)b.mem, &run, &out) != (uint32_t)DS_RUN_REASON_UNTIL) {
        harness_error(c, "ds_run refused the request or stopped for another reason than the deadline");
        release_boot(&b);
        return;
    }
    judge_dsv1(c, inst->ram8k + V_OFF, (unsigned)id, (unsigned)checks, got, gotcap);
    release_boot(&b);
}
