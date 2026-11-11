/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * guestkinds.c - the guest case kind and the shared authored-ROM loader (plan 03-04; D-57, D-58, D-79 amended, D-95, D-123, D-124; F1, F3, F11).
 * Test code only. Evidence class E0: the verdict is what the authored guest wrote, judged from raw work RAM; nothing here is an accuracy,
 * compatibility or hardware claim. The kind and its judge moved here from spinkinds.c (plan 03-01 put them there).
 *   guest         boots a ROM that scripts/fixture-build assembled (build/gate/fixtures/NAME.pce), runs it with ds_run to an absolute deadline
 *                 and judges the DSV1 verdict block that the guest wrote into work RAM, read from raw inst->ram8k (D-108: no peek API).
 *                 The ROM is named by the fixture key as authored:NAME (NAME is lowercase letters, digits and hyphens; the loader reads only
 *                 build/gate/fixtures/NAME.pce and never a path from the case, T-03-12). The deadline is the integer key until_mclk (absolute
 *                 from boot, master clocks) and the check count the integer key checks; both are optional for other kinds but required here.
 *                 expect is "id=N" (the fixture id that the guest writes into its DSV1 block).
 *                 The RESULT line of a guest case ends machine_time=N stop=REASON (D-95, F11).
 *                 Outcome mapping, one reading of D-124 (the judge below and ds_proto.s carry the same layout):
 *                   harness_failure  a harness-side precondition fails: the ROM is unreadable, a key or the expect is malformed, a DSV1 block is
 *                                    already present before the first step, ds_run refuses the request, the block reports zero checks, or its
 *                                    check count or fixture id differs from the case
 *                   guest_failure    the guest wrote a fault: any result byte other than $50 (so $46 and $00 too), an invalid state or complement,
 *                                    a bad version, trailer, summary, progress or checksum, nonzero error flags or reserved bytes
 *                   timeout          the run ended at the deadline and the block is absent or not done (and no guest fault is visible)
 *                   pass             a consistent done block in which all n_checks result bytes are $50; assertions = n_checks
 *                 DSV1 layout: +00 "DSV1", +04 state ($00 idle, $01 running, $A5 done), +05 complement, +06 version 1, +07 n_checks, +08 fixture id,
 *                 +09 pass, +0A fail, +0B notrun, +0C progress, +0D error flags, +10..+6F result per check ($00 not run, $50 pass, $46 fail),
 *                 +80..+DF observed byte per check, +E0 +E1 16-bit little-endian sum of +06..+DF, +FE +FF trailer $5A $A5, every other byte zero.
 *                 The block is at work-RAM offset $1F00 (logical $3F00 while MPR1 = $F8).
 * Shared with the run kinds of stepkinds.c: boot_built_rom (the loader) and note_run (the machine time and stop reason of a RESULT line).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

/* ---- the DSV1 judge ---- */

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

/* the NAME of a fixture key authored:NAME into out; 0 on success. NAME is lowercase letters, digits and hyphens only (it becomes a file name, T-03-12). */
int authored_name(const char *fixture, char *out, size_t cap)
{
    const char *name;
    size_t n;

    if (strncmp(fixture, "authored:", 9u) != 0) {
        return -1;
    }
    name = fixture + 9;
    n = strlen(name);
    if (n == 0u || n >= cap || strspn(name, "abcdefghijklmnopqrstuvwxyz0123456789-") != n) {
        return -1;
    }
    memcpy(out, name, n + 1u);
    return 0;
}

/* the stop reason of a RESULT line: the name of the ds_run reason (the ingest accepts [a-z][a-z0-9_:-]*) */
static const char *stop_name(uint32_t reason)
{
    return reason == (uint32_t)DS_RUN_REASON_UNTIL ? "until" : reason == (uint32_t)DS_RUN_REASON_CALLBACK ? "callback" : "refused";
}

/* record the machine time after a run and why it stopped, for the machine_time=N stop=REASON tail of the RESULT line (D-95); the last call wins */
void note_run(struct ctx *c, uint64_t machine_time, uint32_t reason)
{
    c->has_run = 1;
    c->run_mclk = machine_time;
    c->run_stop = stop_name(reason);
}

/* id=N into the fixture id; 0 on success */
static int parse_id(const char *text, uint64_t *id)
{
    return strncmp(text, "id=", 3u) == 0 && parse_u64(text + 3, id) == 0 && *id <= 255u ? 0 : -1;
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

/* the two integer keys of the case (until_mclk and checks, both at most the limit), 0 on success */
static int read_int_keys(const struct kase *k, uint64_t *until, uint64_t *checks)
{
    return k->has[K_UNTIL_MCLK] && k->has[K_CHECKS] && parse_u64(k->val[K_UNTIL_MCLK], until) == 0 && parse_u64(k->val[K_CHECKS], checks) == 0 && *checks <= 255u ? 0 : -1;
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
    uint32_t reason;

    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0) {
        harness_error(c, "a guest case names its ROM as fixture = authored:NAME");
        return;
    }
    if (read_int_keys(k, &until, &checks) != 0) {
        harness_error(c, "a guest case needs the integer keys until_mclk and checks (checks at most 255)");
        return;
    }
    if (parse_id(k->val[K_EXPECT], &id) != 0) {
        harness_error(c, "a guest expect is id=N");
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
    reason = ds_run((struct ds_instance *)(void *)b.mem, &run, &out);
    note_run(c, inst->machine_time, reason);
    if (reason != (uint32_t)DS_RUN_REASON_UNTIL) {
        harness_error(c, "ds_run refused the request or stopped for another reason than the deadline");
        release_boot(&b);
        return;
    }
    judge_dsv1(c, inst->ram8k + V_OFF, (unsigned)id, (unsigned)checks, got, gotcap);
    release_boot(&b);
}
