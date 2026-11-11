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
 *                 expect is "id=N" (the fixture id that the guest writes into its DSV1 block), optionally followed by one protocol-control mode (plan 03-05, D-124) and observe items (the grammar is in runner.h):
 *                   ;verdict=OUTCOME:REASON  the guest is a deliberate protocol fault. The case passes only when the judge gives exactly the outcome OUTCOME
 *                                            (pass, guest_failure, harness_failure or timeout) and its reason text contains the word REASON (lowercase letters,
 *                                            digits, hyphens and underscores); any other verdict, or the right outcome for another reason, is a guest_failure
 *                   ;refused=stim.NAME       a stimulus refusal control (plan 03-22, D-112): the case's irq_stim list must be refused by ds_irq_stim_set with exactly that code, the instance unchanged; the guest is never run
 *                   ;refused=input.NAME[,NAME...]  an input refusal control (plan 03-27, D-117): synthetic events bypassing every script are pushed to ds_input_push and must be refused with exactly the named codes of src/core/input_refusals.def, the instance unchanged (inputkinds.c)
 *                   ;fail-set=IDX,...        (plan 03-29, D-118) a control-shaped case: exactly the listed checks must read $46 and every other check $50 in a consistent done block (the pad negative controls); any other set is a failure of the case
 *                   ;exact-edges             (plan 03-29, D-117, D-118) the level of the declared input script is read with ds_io_read at at - 1, at and at + 1 of every event after time 0 and compared with the protocol byte computed in the test; no guest is run (inputkinds.c)
 *                   ;padlog                  (plan 03-29, D-118, R-7) the first run of the case carries the dev sink and the pad-window records are written as the evidence file ID.padlog.tsv; a lost record is harness_failure
 *                   ;pad-order=NAME,NAME     (plan 03-29, D-118) the same ROM is run with the two named input scripts first; the three verdicts must be pairwise different and each dev log must hold a SEL = 1 and a SEL = 0 read, or the case is harness_failure before correct is trusted
 *                   ;poison-t0               the runner writes the DSV1 magic and the done state into work RAM after boot and before the first step; the
 *                                            t=0 precondition must then give harness_failure ("before the first step"), and the guest is not run at all
 *                 Observe rows (plan 03-10): the optional fifth column of NAME.checks, the word observe, marks a check whose expected value legitimately differs by case; scripts/fixture-build lists
 *                 such indexes in build/gate/fixtures/NAME.observe, the guest writes its observed byte and result $50, and the case judges the byte through one item observe=IDX:VALUE per index
 *                 (id=1;observe=3:0x16). An index without an item is harness_failure (observe-missing), an item for a check that is not an observe row too (observe-unlisted).
 *                 Plan 03-24 (D-127): observe=IDX:policy marks a row whose byte legitimately reads power-on state (recorded, exempt from the cross-policy equality, compared with no value), and the mode policy-diff=IDX with observe=IDX:differs is the
 *                 negative control of that equality: the byte of every policy after the first must DIFFER from the first one, so a fixture that stopped reading the power-on state turns the case guest_failure (policy-diff).
 *                 Power-on policies (plan 03-11; D-127): the ROM is booted and run once per policy of the list (the optional key policies, request fragments separated by ||, or the built-in default of zero, seeded-prng-v2 seed 1 and
 *                 seeded-prng-v2 seed 2, plan 03-24), and the DSV1 result bytes +10..+6F and observed bytes +80..+DF of every run must equal those of the first run; the first difference is a guest_failure naming the check.
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
#include "ds_obs.h"
#include "ds_serial.h"
#include "runner.h"

/* ---- the DSV1 judge ---- */

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
void judge_dsv1(struct ctx *c, const uint8_t *b, unsigned want_id, unsigned want_checks, char *got, size_t gotcap)
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

/* one decimal check index 1..255 at s (the whole text), 0 on success */
static int parse_idx(const char *s, unsigned *out)
{
    uint64_t v = 0u;

    if (strlen(s) == 0u || strlen(s) > 3u || strspn(s, "0123456789") != strlen(s) || parse_u64(s, &v) != 0 || v < 1u || v > 255u) {
        return -1;
    }
    *out = (unsigned)v;
    return 0;
}

/* one item of the expect after id=N; each kind of item at most once for the modes, observe rows by index (0 on success) */
static int parse_item(char *item, struct guest_expect *e, int *seen_mode, int *seen_exclude)
{
    char *colon;

    if (strcmp(item, "poison-t0") == 0) {
        if (*seen_mode) {
            return -1;
        }
        *seen_mode = 1;
        e->mode = MODE_POISON;
        memcpy(e->outcome, "harness_failure", 16u);
        memcpy(e->reason, "before the first step", 22u);
        return 0;
    }
    if (strncmp(item, "verdict=", 8u) == 0) {
        item += 8;
        colon = strchr(item, ':');
        if (*seen_mode || colon == NULL || colon == item || (size_t)(colon - item) >= sizeof e->outcome || strlen(colon + 1) == 0u || strlen(colon + 1) >= sizeof e->reason
            || strspn(colon + 1, "abcdefghijklmnopqrstuvwxyz0123456789-_") != strlen(colon + 1)) {
            return -1;
        }
        *seen_mode = 1;
        memcpy(e->outcome, item, (size_t)(colon - item));
        e->outcome[colon - item] = '\0';
        memcpy(e->reason, colon + 1, strlen(colon + 1) + 1u);
        e->mode = MODE_VERDICT;
        return 0;
    }
    if (strncmp(item, "policy-diff=", 12u) == 0) {
        unsigned idx = 0u;

        if (*seen_mode || parse_idx(item + 12, &idx) != 0) {
            return -1;
        }
        *seen_mode = 1;
        e->pdiff = idx; /* the mode stays MODE_PLAIN: the guest is judged as usual and the diff row is compared after the runs */
        return 0;
    }
    if (strncmp(item, "refused=stim.", 13u) == 0) {
        item += 13;
        if (*seen_mode || strlen(item) == 0u || strlen(item) >= sizeof e->reason || strspn(item, "abcdefghijklmnopqrstuvwxyz") != strlen(item)) {
            return -1;
        }
        *seen_mode = 1;
        memcpy(e->reason, item, strlen(item) + 1u);
        e->mode = MODE_REFUSED;
        return 0;
    }
    if (strncmp(item, "refused=input.", 14u) == 0) {
        item += 14;
        if (*seen_mode || strlen(item) == 0u || strlen(item) >= sizeof e->reason || strspn(item, "abcdefghijklmnopqrstuvwxyz_,") != strlen(item)) {
            return -1;
        }
        *seen_mode = 1;
        memcpy(e->reason, item, strlen(item) + 1u);
        e->mode = MODE_REFUSED_INPUT;
        return 0;
    }
    if (strncmp(item, "fail-set=", 9u) == 0) {
        char *rest = item + 9;
        char *tok;

        if (*seen_mode) {
            return -1;
        }
        *seen_mode = 1;
        e->mode = MODE_FAILSET;
        while ((tok = pk_next_item(&rest, ',')) != NULL) {
            unsigned idx = 0u;

            if (e->nfail >= GE_ITEMS_MAX || parse_idx(tok, &idx) != 0 || (e->nfail > 0u && idx <= e->fail[e->nfail - 1u])) {
                return -1;
            }
            e->fail[e->nfail++] = idx;
        }
        return e->nfail > 0u ? 0 : -1;
    }
    if (strcmp(item, "exact-edges") == 0) {
        if (*seen_mode) {
            return -1;
        }
        *seen_mode = 1;
        e->mode = MODE_EDGES;
        return 0;
    }
    if (strcmp(item, "padlog") == 0) {
        if (e->padlog) {
            return -1;
        }
        e->padlog = 1;
        return 0;
    }
    if (strncmp(item, "pad-order=", 10u) == 0) {
        char *comma = strchr(item + 10, ',');
        size_t la;
        size_t lb;

        if (e->order_a[0] != '\0' || comma == NULL) {
            return -1;
        }
        la = (size_t)(comma - (item + 10));
        lb = strlen(comma + 1);
        if (la == 0u || lb == 0u || la >= sizeof e->order_a || lb >= sizeof e->order_b || strchr(comma + 1, ',') != NULL || item[10] < 'a' || item[10] > 'z' || comma[1] < 'a' || comma[1] > 'z'
            || strspn(item + 10, "abcdefghijklmnopqrstuvwxyz0123456789_,") != la + 1u + lb) {
            return -1;
        }
        memcpy(e->order_a, item + 10, la);
        e->order_a[la] = '\0';
        memcpy(e->order_b, comma + 1, lb + 1u);
        return strcmp(e->order_a, e->order_b) != 0 ? 0 : -1;
    }
    if (strncmp(item, "observe=", 8u) == 0) {
        unsigned idx = 0u;
        unsigned i;
        uint64_t value = 0u;
        int differs;
        int policy;

        item += 8;
        colon = strchr(item, ':');
        if (colon == NULL || e->nobs >= GE_ITEMS_MAX) {
            return -1;
        }
        *colon = '\0';
        differs = strcmp(colon + 1, "differs") == 0;
        policy = strcmp(colon + 1, "policy") == 0;
        if (parse_idx(item, &idx) != 0 || (!differs && !policy && (parse_u64(colon + 1, &value) != 0 || value > 255u))) {
            return -1;
        }
        for (i = 0u; i < e->nobs; i++) {
            if (e->obs[i].idx == idx) {
                return -1;
            }
        }
        e->obs[e->nobs].idx = idx;
        e->obs[e->nobs].differs = differs;
        e->obs[e->nobs].policy = policy;
        e->obs[e->nobs].value = (unsigned)value;
        e->nobs++;
        return 0;
    }
    if (strncmp(item, "exclude=", 8u) == 0) {
        char *rest = item + 8;
        char *tok;

        if (*seen_exclude) {
            return -1;
        }
        *seen_exclude = 1;
        while ((tok = pk_next_item(&rest, ',')) != NULL) {
            unsigned idx = 0u;

            if (e->nex >= GE_ITEMS_MAX || parse_idx(tok, &idx) != 0 || (e->nex > 0u && idx <= e->ex[e->nex - 1u])) {
                return -1;
            }
            e->ex[e->nex++] = idx;
        }
        return e->nex > 0u ? 0 : -1;
    }
    return -1;
}

/* the expect of a guest or mutate_sweep case: id=N, then items (see runner.h); 0 on success */
int parse_guest_expect(const char *text, struct guest_expect *e)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int seen_mode = 0;
    int seen_exclude = 0;
    size_t n;

    memset(e, 0, sizeof *e);
    e->mode = MODE_PLAIN;
    if (pk_copy_text(text, list, sizeof list) != 0 || strncmp(text, "id=", 3u) != 0) {
        return -1;
    }
    item = pk_next_item(&rest, ';');
    if (item == NULL) {
        return -1;
    }
    n = strspn(item + 3, "0123456789");
    if (n == 0u || n > 3u || item[3u + n] != '\0' || parse_u64(item + 3, &e->id) != 0 || e->id > 255u) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (parse_item(item, e, &seen_mode, &seen_exclude) != 0) {
            return -1;
        }
    }
    return 0;
}

/* the outcome name of a context, by the same precedence as outcome_of in runner.c (harness, then timeout unless failed, then failure or no assertion) */
static const char *ctx_outcome(const struct ctx *c)
{
    if (c->harness) {
        return "harness_failure";
    }
    if (c->timeout && !c->failed) {
        return "timeout";
    }
    if (c->failed || c->n == 0u) {
        return "guest_failure";
    }
    return "pass";
}

/* a protocol-control case: the scratch context s holds what the judge (or the t=0 precondition) concluded; the case passes only on the declared outcome and reason */
void expect_verdict(struct ctx *c, const struct ctx *s, const struct guest_expect *e, char *got, size_t gotcap)
{
    const char *seen = ctx_outcome(s);
    char msg[160];

    snprintf(got, gotcap, "verdict=%s", seen);
    snprintf(msg, sizeof msg, "the protocol fault gave the outcome %s, not the declared %s", seen, e->outcome);
    check(c, strcmp(seen, e->outcome) == 0, msg);
    snprintf(msg, sizeof msg, "the outcome is the declared one but its reason does not contain '%s'", e->reason);
    check(c, strstr(s->why, e->reason) != NULL, msg);
}

/*
 * A fail-set control (plan 03-29; D-118): the scratch context s holds what the judge concluded about the block blk of the run, and the case passes only when the block is a consistent done block in which exactly the checks of the declared set read $46 and every
 * other check $50. The judge reports a structural fault (a bad trailer, a bad checksum, an error flag) before it looks at any check result, so the first reason of a block whose only fault is failed checks starts with "check ": any other reason means the block itself
 * is wrong and the declared set is not the fact under test. A harness fault of the run stays a harness fault.
 */
static void expect_failset(struct ctx *c, const struct ctx *s, const uint8_t *blk, const struct guest_expect *e, unsigned nchk, char *got, size_t gotcap)
{
    unsigned i;
    unsigned nf = 0u;
    int equal = 1;
    size_t at;

    if (s->harness) {
        harness_error(c, s->why);
        return;
    }
    snprintf(got, gotcap, "failset=");
    at = strlen(got);
    if (s->timeout || !s->failed || strncmp(s->why, "check ", 6u) != 0) {
        check(c, 0, "fail-set: the run did not end in a consistent done block with failed checks (a timeout, a structural fault of the block or no failed check)");
        return;
    }
    for (i = 0u; i < nchk && i < V_MAX_CHECKS; i++) {
        unsigned r = blk[V_RES + i];

        if (r != R_PASS && r != R_FAIL) {
            check(c, 0, "fail-set: a result byte of the block is neither $50 nor $46");
            return;
        }
        if (r == R_FAIL) {
            if (nf >= e->nfail || e->fail[nf] != i + 1u) {
                equal = 0;
            }
            nf++;
            if (at + 5u < gotcap) {
                at += (size_t)snprintf(got + at, gotcap - at, "%s%u", nf > 1u ? "," : "", i + 1u);
            }
        }
    }
    check(c, equal && nf == e->nfail, "fail-set: the checks that failed are not exactly the set the case declares");
}

int dsv1_done(const struct ds_instance *inst, void *ctx)
{
    (void)ctx;
    return inst->ram8k[V_OFF + 4u] == 0xA5u && inst->ram8k[V_OFF + 5u] == 0x5Au;
}

/* the observe rows of a built fixture: build/gate/fixtures/ROM.observe, one decimal check index per line, ascending (scripts/fixture-build writes it from the fifth column of ROM's .checks) */
const char *read_observe(const char *rom, struct obs_list *out)
{
    char path[96];
    char *text;
    char *line;

    out->n = 0u;
    snprintf(path, sizeof path, "build/gate/fixtures/%s.observe", rom);
    text = read_file(path);
    if (text == NULL) {
        return "the built observe list is missing: run scripts/fixture-build first";
    }
    line = text;
    while (*line != '\0') {
        char *nl = strchr(line, '\n');
        unsigned idx = 0u;

        if (nl == NULL) {
            free(text);
            return "the built observe list does not end in a newline";
        }
        *nl = '\0';
        if (parse_idx(line, &idx) != 0 || out->n >= GE_ITEMS_MAX || (out->n > 0u && idx <= out->idx[out->n - 1u])) {
            free(text);
            return "the built observe list holds a line that is not an ascending check index";
        }
        out->idx[out->n++] = idx;
        line = nl + 1;
    }
    free(text);
    return NULL;
}

/* every observe row of the fixture has an observe=IDX:VALUE item, every item names an observe row of the fixture (and a check of the case); harness_failure on t otherwise */
void observe_cover(struct ctx *t, const struct guest_expect *e, const struct obs_list *ol, unsigned nchk)
{
    unsigned i;
    unsigned j;

    for (i = 0u; i < ol->n; i++) {
        int found = 0;

        for (j = 0u; j < e->nobs; j++) {
            found = found || e->obs[j].idx == ol->idx[i];
        }
        if (!found) {
            harness_error(t, "observe-missing: an observe row of the fixture has no observe=IDX:VALUE item in the case expect");
        }
        if (ol->idx[i] > nchk) {
            harness_error(t, "observe-missing: an observe row of the fixture is above the checks count of the case");
        }
    }
    for (j = 0u; j < e->nobs; j++) {
        int found = 0;

        for (i = 0u; i < ol->n; i++) {
            found = found || e->obs[j].idx == ol->idx[i];
        }
        if (!found) {
            harness_error(t, "observe-unlisted: an observe=IDX:VALUE item names a check that is not an observe row of the fixture");
        }
        if (e->obs[j].differs && (e->pdiff == 0u || e->obs[j].idx != e->pdiff)) {
            harness_error(t, "observe-differs: observe=IDX:differs is judged only in a case that carries policy-diff=IDX for the same row");
        }
    }
    if (e->pdiff != 0u) {
        int has_differs = 0;

        for (j = 0u; j < e->nobs; j++) {
            has_differs = has_differs || (e->obs[j].idx == e->pdiff && e->obs[j].differs);
        }
        if (!has_differs) {
            harness_error(t, "policy-diff: a policy-diff=IDX case needs the item observe=IDX:differs for the same row");
        }
    }
}

/* the observed byte of each observe row against its item; the block is a judged pass, so every result byte is $50 already */
void observe_judge(struct ctx *t, const uint8_t *b, const struct guest_expect *e, const struct obs_list *ol)
{
    unsigned i;
    unsigned j;
    char msg[160];

    for (i = 0u; i < ol->n; i++) {
        for (j = 0u; j < e->nobs; j++) {
            if (e->obs[j].idx != ol->idx[i]) {
                continue;
            }
            if (e->obs[j].differs || e->obs[j].policy) {
                continue; /* judged across the runs of the policy list by kind_guest, not against a value (plan 03-24) */
            }
            snprintf(msg, sizeof msg, "the observed byte of observe row %u differs from the observe=%u:%u item of the case", ol->idx[i], ol->idx[i], e->obs[j].value);
            check(t, b[V_OBS + ol->idx[i] - 1u] == (uint8_t)e->obs[j].value, msg);
        }
    }
}

/* the built image build/gate/fixtures/ROM.pce in a heap buffer of at most DS_ROM_CAP bytes (NULL, with *len 0, when it is missing or unreadable) */
unsigned char *load_image(const char *rom, size_t *len)
{
    char path[96];

    *len = 0u;
    snprintf(path, sizeof path, "build/gate/fixtures/%s.pce", rom);
    return read_rom(path, (size_t)DS_ROM_CAP, len);
}

/* the model.NAME=VALUE pairs of a parsed request into the booted instance through ds_model_set, before the first step (plan 03-17); NULL on success, otherwise why not (a fault of the case: a refused setting is never a verdict on the guest) */
const char *apply_model(struct ds_instance *inst, const struct spec *s)
{
    unsigned i;

    for (i = 0u; i < s->nmodel; i++) {
        if (ds_model_set(inst, s->model_name[i], s->model_value[i]) != (uint32_t)DS_MODELREF_NONE) {
            return "a model.NAME setting of the request was refused by ds_model_set";
        }
    }
    return NULL;
}

/*
 * The declared IRQ1 and IRQ2 stimulus of a case (plan 03-22; D-112, T-03-60, ER-5). The case key irq_stim is an array of strings AT,LINE,LEVEL (the flattened array reaches the runner as the elements joined by the unit separator, octal 037),
 * each field decimal or 0x hex. This is the one reader: the core validates the list again (ds_irq_stim_set), so a bad list is a refusal there and never undefined behaviour here. The key is a TEST stimulus (E0/E2 for controller logic), never
 * evidence about the VDC or any hardware source (ER-5). A list longer than the parse buffer, a malformed element or a field above 32 bits is a fault of the case.
 */
const char *irq_stim_parse(const struct kase *k, struct ds_irq_event *ev, unsigned cap, unsigned *n, int *declared)
{
    char buf[PK_LIST_MAX * 2u];
    const char *p;
    unsigned count = 0u;

    *n = 0u;
    *declared = 0;
    if (!k->has[K_IRQ_STIM]) {
        return NULL;
    }
    *declared = 1;
    if (strlen(k->val[K_IRQ_STIM]) >= sizeof buf) {
        return "the irq_stim list is too long";
    }
    memcpy(buf, k->val[K_IRQ_STIM], strlen(k->val[K_IRQ_STIM]) + 1u);
    if (buf[0] == '\0') {
        return NULL; /* an empty array declares no events */
    }
    p = buf;
    for (;;) {
        char *sep = strchr(p, '\037');
        char elem[64];
        char *c1;
        char *c2;
        uint64_t at = 0u;
        uint64_t line = 0u;
        uint64_t level = 0u;
        size_t len = sep != NULL ? (size_t)(sep - p) : strlen(p);

        if (count >= cap || len == 0u || len >= sizeof elem) {
            return "an irq_stim list holds more elements than the parse buffer, or an empty or overlong element";
        }
        memcpy(elem, p, len);
        elem[len] = '\0';
        c1 = strchr(elem, ',');
        c2 = c1 != NULL ? strchr(c1 + 1, ',') : NULL;
        if (c1 == NULL || c2 == NULL || strchr(c2 + 1, ',') != NULL) {
            return "an irq_stim element is AT,LINE,LEVEL";
        }
        *c1 = '\0';
        *c2 = '\0';
        if (parse_u64(elem, &at) != 0 || parse_u64(c1 + 1, &line) != 0 || parse_u64(c2 + 1, &level) != 0 || line > 0xFFFFFFFFu || level > 0xFFFFFFFFu) {
            return "an irq_stim field is not an unsigned number (line and level at most 32 bits)";
        }
        ev[count].at = at;
        ev[count].line = (uint32_t)line;
        ev[count].level = (uint32_t)level;
        count++;
        if (sep == NULL) {
            break;
        }
        p = sep + 1;
    }
    *n = count;
    return NULL;
}

const char *apply_irq_stim(struct ds_instance *inst, const struct kase *k, uint32_t *refusal)
{
    struct ds_irq_event ev[IRQ_STIM_PARSE_MAX];
    unsigned n = 0u;
    int declared = 0;
    const char *err;

    *refusal = 0u;
    err = irq_stim_parse(k, ev, IRQ_STIM_PARSE_MAX, &n, &declared);
    if (err != NULL || !declared) {
        return err;
    }
    *refusal = ds_irq_stim_set(inst, ev, (uint32_t)n);
    return NULL;
}

/*
 * Boot the image in b->content (len bytes, heap memory the caller set; release_boot frees it) over the request text (the case's request key; empty gives the defaults of parse_spec).
 * NULL and b and the accepted instance on success, otherwise why not: a fault here is a harness fault of the case, never a verdict on the guest. On a fault b is released.
 */
const char *boot_content(const char *request, size_t len, struct booted *b, const struct ds_instance **inst)
{
    struct spec s;
    struct ds_boot_request rq;
    struct ds_boot_result res;
    const char *err;

    b->mem = NULL;
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
    err = apply_model(res.instance, &s);
    if (err != NULL) {
        release_boot(b);
        return err;
    }
    *inst = res.instance;
    return NULL;
}

/* Boot the built authored image build/gate/fixtures/ROM.pce: load_image, then boot_content. Shared by the guest kind and the run kinds of stepkinds.c; release_boot frees what it allocated. */
const char *boot_built_rom(const char *rom, const char *request, struct booted *b, const struct ds_instance **inst)
{
    size_t len = 0u;

    b->mem = NULL;
    b->content = load_image(rom, &len);
    if (b->content == NULL) {
        return "the built fixture image is missing or unreadable: run scripts/fixture-build first";
    }
    return boot_content(request, len, b, inst);
}

/* the two integer keys of the case (until_mclk and checks, both at most the limit), 0 on success */
static int read_int_keys(const struct kase *k, uint64_t *until, uint64_t *checks)
{
    return k->has[K_UNTIL_MCLK] && k->has[K_CHECKS] && parse_u64(k->val[K_UNTIL_MCLK], until) == 0 && parse_u64(k->val[K_CHECKS], checks) == 0 && *checks <= 255u ? 0 : -1;
}

/*
 * The power-on policy list of a guest case (plan 03-11; D-127, T-03-33). A guest ROM must initialise everything it reads, so the same ROM is booted and run once per policy of the list and the DSV1 result bytes (+10..+6F) and observed
 * bytes (+80..+DF) of every run must equal those of the first run: a check that read power-on state differs between a zero and a seeded power-on, and the case is guest_failure naming the check. The list is the optional case key
 * policies (request fragments separated by ||, an empty fragment is the default zero policy) or, when the key is absent, the built-in default below: zero plus two seeded-prng-v2 seeds since plan 03-24 (D-114, D-127), so cases never
 * repeat the list, and a REQUIRED case with an explicit key needs a meaning that starts with policies-override: and names the reason (the guard of kind_guest, control guest_policies_guard). Each fragment is appended to the case's own request with a semicolon. The poison-t0 control does not run the guest, so it uses the first policy only.
 */
static const char *const default_policies[] = {"", "bind.powerup_policy=seeded-prng-v2;seed=1", "bind.powerup_policy=seeded-prng-v2;seed=2"};
#define GP_DEFAULT_COUNT 3u
#define GP_MAX 8u

/* the policy fragments of a case into list (NULL entries never occur); returns the count, 0 when the key text is malformed */
static unsigned policy_list(const struct kase *k, char *buf, size_t cap, const char **list)
{
    unsigned n = 0u;
    char *p;

    if (!k->has[K_POLICIES]) {
        for (n = 0u; n < GP_DEFAULT_COUNT; n++) {
            list[n] = default_policies[n];
        }
        return n;
    }
    if (strlen(k->val[K_POLICIES]) >= cap) {
        return 0u;
    }
    memcpy(buf, k->val[K_POLICIES], strlen(k->val[K_POLICIES]) + 1u);
    p = buf;
    for (;;) {
        char *sep = strstr(p, "||");

        if (n >= GP_MAX) {
            return 0u;
        }
        list[n++] = p;
        if (sep == NULL) {
            break;
        }
        *sep = '\0';
        p = sep + 2;
    }
    return n;
}

static const char *policy_name(const char *fragment)
{
    return fragment[0] == '\0' ? "zero" : fragment;
}

/* the case's request with the policy fragment appended; 0 on success, -1 when it does not fit the request text of a spec */
static int compose_request(const char *base, const char *fragment, char *out, size_t cap)
{
    int n;

    if (base[0] == '\0') {
        n = snprintf(out, cap, "%s", fragment);
    } else if (fragment[0] == '\0') {
        n = snprintf(out, cap, "%s", base);
    } else {
        n = snprintf(out, cap, "%s;%s", base, fragment);
    }
    return n >= 0 && (size_t)n < cap ? 0 : -1;
}

/* the dev sink of a pad run (plan 03-29; D-118, D-119): the sink struct and its record memory are heap blocks of the harness (the records are 8-byte aligned by calloc); the count and the lost counter are read from the header after the run */
#define PL_CAP 512u
struct padrun {
    struct ds_dev_sink *sink;
    unsigned char *rec;
    uint32_t count;
    uint32_t lost;
};

static int padrun_new(struct padrun *p)
{
    memset(p, 0, sizeof *p);
    p->sink = (struct ds_dev_sink *)calloc(1u, sizeof *p->sink);
    p->rec = (unsigned char *)calloc((size_t)PL_CAP, DS_DEV_RECORD_BYTES);
    if (p->sink == NULL || p->rec == NULL) {
        free(p->sink);
        free(p->rec);
        p->sink = NULL;
        p->rec = NULL;
        return -1;
    }
    p->sink->records = p->rec;
    p->sink->capacity = PL_CAP;
    return 0;
}

static void padrun_free(struct padrun *p)
{
    free(p->sink);
    free(p->rec);
    p->sink = NULL;
    p->rec = NULL;
}

static uint32_t pl_u32(const unsigned char *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static uint64_t pl_u64(const unsigned char *b)
{
    return (uint64_t)pl_u32(b) | ((uint64_t)pl_u32(b + 4) << 32);
}

/* a record is a pad access when its physical address lies in the $1000 window of the hardware page ($1FF000 to $1FF3FF, H8-19) */
static int pl_is_pad(const unsigned char *r)
{
    return (pl_u32(r + 8) & ~UINT32_C(0x3FF)) == UINT32_C(0x1FF000);
}

/* the number of reads with CLR low and SEL high, and with CLR low and SEL low, in the pad window of the log; the latch of a read is the value of the last write to the window, and $FF before any write (port O is high after reset, H8-18) */
static void padlog_scan(const struct padrun *p, unsigned *sel1, unsigned *sel0)
{
    unsigned latch = 0xFFu;
    uint32_t i;

    *sel1 = 0u;
    *sel0 = 0u;
    for (i = 0u; i < p->count && i < PL_CAP; i++) {
        const unsigned char *r = p->rec + (size_t)i * DS_DEV_RECORD_BYTES;

        if (!pl_is_pad(r)) {
            continue;
        }
        if (r[13] == (unsigned char)DS_DEV_WRITE) {
            latch = r[12];
        } else if (r[13] == (unsigned char)DS_DEV_READ && (latch & 2u) == 0u) {
            if ((latch & 1u) != 0u) {
                (*sel1)++;
            } else {
                (*sel0)++;
            }
        }
    }
}

/* write ID.padlog.tsv into the evidence directory: a header row and one row per pad-window record, stamp (master clocks, decimal), phys (hex), value (hex), kind (write or read) and the latch (hex; for a write the byte written, for a read the latch it saw). 0 on success */
static int padlog_write(const struct kase *k, const struct padrun *p)
{
    char path[256];
    FILE *fp;
    unsigned latch = 0xFFu;
    uint32_t i;
    int n;

    if (run_evidence_dir == NULL) {
        return -1;
    }
    n = snprintf(path, sizeof path, "%s/%s.padlog.tsv", run_evidence_dir, k->stem);
    if (n < 0 || (size_t)n >= sizeof path) {
        return -1;
    }
    fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }
    fputs("stamp\tphys\tvalue\tkind\tlatch\n", fp);
    for (i = 0u; i < p->count && i < PL_CAP; i++) {
        const unsigned char *r = p->rec + (size_t)i * DS_DEV_RECORD_BYTES;

        if (!pl_is_pad(r)) {
            continue;
        }
        if (r[13] == (unsigned char)DS_DEV_WRITE) {
            latch = r[12];
        }
        fprintf(fp, "%llu\t%06X\t%02X\t%s\t%02X\n", (unsigned long long)pl_u64(r), (unsigned)pl_u32(r + 8), (unsigned)r[12], r[13] == (unsigned char)DS_DEV_WRITE ? "write" : "read", latch);
    }
    return fclose(fp) == 0 ? 0 : -1;
}

struct gstate {
    const struct kase *kase; /* the case: its irq_stim key is declared to every booted instance (plan 03-22) */
    struct padrun *pad;      /* the dev sink of this run (plan 03-29), NULL for none */
    const char *rom;
    const char *request;
    uint64_t until;
    uint64_t checks;
    const struct guest_expect *ex;
    const struct obs_list *ol;
};

/*
 * One boot and run of the guest over request, judged as the case says. c collects harness faults and the machine-time note; in the plain mode the verdict is judged into c itself (judged == c), in the verdict mode into the scratch
 * context judged, which expect_verdict then compares with the declaration. blk receives the 256 bytes of the block after the run. Returns 1 when the machine ran to the deadline and the block was judged, 0 otherwise.
 */
static int guest_once(const struct gstate *g, struct ctx *c, struct ctx *judged, uint8_t *blk, char *got, size_t gotcap)
{
    struct booted b;
    struct ds_run_request run;
    struct ds_run_result out;
    const struct ds_instance *inst = NULL;
    const char *err;
    uint32_t reason;
    uint32_t stim_refusal = 0u;
    uint32_t input_refusal = 0u;
    const struct guest_expect *ex = g->ex;

    err = boot_built_rom(g->rom, g->request, &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return 0;
    }
    err = apply_irq_stim((struct ds_instance *)(void *)b.mem, g->kase, &stim_refusal);
    if (err == NULL && stim_refusal != 0u) {
        err = "the irq_stim list of the case was refused by ds_irq_stim_set";
    }
    if (err == NULL) {
        /* the input script of the case (plan 03-27; D-117): its events are pushed after the boot and before the first step, like the stimulus; the core re-validates them */
        err = apply_input((struct ds_instance *)(void *)b.mem, g->kase, &input_refusal);
        if (err == NULL && input_refusal != 0u) {
            err = "the input script of the case was refused by ds_input_push";
        }
    }
    if (err != NULL) {
        harness_error(c, err);
        release_boot(&b);
        return 0;
    }
    if (ex->mode == MODE_POISON) {
        /* the runner-side control of the t=0 precondition (D-124): plant the magic and the done state, as a stale or hostile block would be */
        uint8_t *ram = ((struct ds_instance *)(void *)b.mem)->ram8k;

        ram[V_OFF + 0u] = 0x44u;
        ram[V_OFF + 1u] = 0x53u;
        ram[V_OFF + 2u] = 0x56u;
        ram[V_OFF + 3u] = 0x31u;
        ram[V_OFF + 4u] = 0xA5u;
        ram[V_OFF + 5u] = 0x5Au;
    }
    /* the precondition of D-124: no verdict block is present before the first step */
    if (block_present(inst->ram8k + V_OFF)) {
        harness_error(ex->mode == MODE_POISON ? judged : c, "a DSV1 block is present in work RAM before the first step");
        if (ex->mode == MODE_POISON) {
            expect_verdict(c, judged, ex, got, gotcap);
        }
        release_boot(&b);
        return 0;
    }
    if (ex->mode == MODE_POISON) {
        check(c, 0, "the t=0 precondition did not see the planted DSV1 block");
        release_boot(&b);
        return 0;
    }
    memset(&run, 0, sizeof run);
    memset(&out, 0, sizeof out);
    run.struct_size = (uint32_t)sizeof run;
    run.until_mclk = g->until;
    run.dev = g->pad != NULL ? g->pad->sink : NULL;
    reason = ds_run((struct ds_instance *)(void *)b.mem, &run, &out);
    note_run(c, inst->machine_time, reason);
    if (reason != (uint32_t)DS_RUN_REASON_UNTIL) {
        harness_error(c, "ds_run refused the request or stopped for another reason than the deadline");
        release_boot(&b);
        return 0;
    }
    if (g->pad != NULL) {
        g->pad->count = pl_u32(g->pad->sink->header);
        g->pad->lost = pl_u32(g->pad->sink->header + 4);
        if (g->pad->lost != 0u) {
            harness_error(c, "pad-log: the dev sink lost records, so the pad log would be incomplete evidence (D-119)");
            release_boot(&b);
            return 0;
        }
    }
    judge_dsv1(judged, inst->ram8k + V_OFF, (unsigned)ex->id, (unsigned)g->checks, got, gotcap);
    if (!judged->harness && !judged->failed && !judged->timeout) {
        observe_judge(judged, inst->ram8k + V_OFF, ex, g->ol);
    }
    memcpy(blk, inst->ram8k + V_OFF, V_SIZE);
    release_boot(&b);
    return 1;
}

/* 1 when check idx (1-based) is an observe row that the case marks observe=IDX:policy or observe=IDX:differs: its observed byte is exempt from the cross-policy equality (plan 03-24) */
static int row_exempt(const struct guest_expect *ex, unsigned idx)
{
    unsigned j;

    for (j = 0u; j < ex->nobs; j++) {
        if (ex->obs[j].idx == idx && (ex->obs[j].policy || ex->obs[j].differs)) {
            return 1;
        }
    }
    return 0;
}

/* the first check whose result or observed byte differs between two blocks (1-based), 0 when none; the observed byte of an exempt row is not compared, its result byte still is */
static unsigned first_block_difference(const struct guest_expect *ex, const uint8_t *a, const uint8_t *b)
{
    unsigned i;

    for (i = 0u; i < V_MAX_CHECKS; i++) {
        if (a[V_RES + i] != b[V_RES + i] || (a[V_OBS + i] != b[V_OBS + i] && !row_exempt(ex, i + 1u))) {
            return i + 1u;
        }
    }
    return 0u;
}

/* 1 when the case is a REQUIRED one: the key required = true */
static int case_required(const struct kase *k)
{
    return k->has[K_REQUIRED] && strcmp(k->val[K_REQUIRED], "true") == 0;
}

/* the meaning of a case that overrides the policy list starts with policies-override: and a reason (at least one character that is not a blank after the colon) */
static int policies_override_ok(const char *meaning)
{
    static const char tag[] = "policies-override:";

    return strncmp(meaning, tag, sizeof tag - 1u) == 0 && strspn(meaning + sizeof tag - 1u, " ") < strlen(meaning + sizeof tag - 1u);
}

/* ---- the stimulus refusal rows of kind guest (plan 03-22; D-112, T-03-60, F4) ---- */

/*
 * expect id=N;refused=stim.NAME: the built authored image is booted (no guest is run, except for the state refusal), the case's irq_stim list is given to ds_irq_stim_set and must come back as exactly the refusal NAME of src/core/stim_refusals.def,
 * with the instance byte-identical afterwards and the name printing as stim.NAME. The variants of NAME below add calls the key list cannot express (a NULL list with a count, a line or level above 8 bits, the NULL instance). Before the refusals a valid list
 * is proven accepted and cleared again (so a core that refused everything would fail here), and the codes observed are kept for the deferred kind stim_refusal_coverage.
 */
static unsigned char sr_observed[DS_STIMREF_SLOTS];

/* one call that must be refused with want: the instance (inst, live in memory of ds_instance_size() bytes) must be byte-identical afterwards */
static void sr_refused(struct ctx *c, struct ds_instance *inst, const struct ds_irq_event *ev, uint32_t count, uint32_t want, unsigned *calls)
{
    size_t size = ds_instance_size();
    unsigned char *copy = (unsigned char *)malloc(size);
    uint32_t got;

    if (copy == NULL) {
        harness_error(c, "out of memory for the instance copy");
        return;
    }
    if (inst != NULL) {
        memcpy(copy, inst, size);
    }
    got = ds_irq_stim_set(inst, ev, count);
    check(c, got == want, "ds_irq_stim_set returned a code other than the one the case names");
    check(c, ds_stim_refusal_name(got) != NULL && (got == 0u || strncmp(ds_stim_refusal_name(got), "stim.", 5u) == 0), "a stimulus refusal does not print as stim.NAME");
    if (inst != NULL) {
        check(c, memcmp(copy, inst, size) == 0, "a refused ds_irq_stim_set changed the instance");
    }
    if (got < (uint32_t)DS_STIMREF_SLOTS) {
        sr_observed[got] = 1u;
    }
    (*calls)++;
    free(copy);
}

static void stim_refuse(const struct kase *k, struct ctx *c, const struct guest_expect *ex, const char *rom, uint64_t until, char *got, size_t gotcap)
{
    struct ds_irq_event ev[IRQ_STIM_PARSE_MAX];
    struct ds_irq_event good[2] = {{1u, 1u, 1u}, {2u, 2u, 0u}};
    struct ds_irq_event tmp[2];
    struct booted b;
    const struct ds_instance *inst = NULL;
    struct ds_instance *wr;
    unsigned n = 0u;
    int declared = 0;
    unsigned calls = 0u;
    uint32_t want = 0u;
    uint32_t i;
    const char *err;
    char name[24];

    snprintf(name, sizeof name, "stim.%s", ex->reason);
    for (i = 1u; i < (uint32_t)DS_STIMREF_SLOTS; i++) {
        if (ds_stim_refusal_name(i) != NULL && strcmp(ds_stim_refusal_name(i), name) == 0 && i != 0u) {
            want = i;
        }
    }
    if (want == 0u) {
        harness_error(c, "a refused=stim.NAME expect names a code that stim_refusals.def does not have");
        return;
    }
    err = irq_stim_parse(k, ev, IRQ_STIM_PARSE_MAX, &n, &declared);
    if (err != NULL || !declared || n == 0u) {
        harness_error(c, err != NULL ? err : "a refused=stim.NAME case needs an irq_stim list with at least one event");
        return;
    }
    err = boot_built_rom(rom, k->val[K_REQUEST], &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    wr = (struct ds_instance *)(void *)b.mem;
    /* before the first step a valid list is accepted and a count of 0 clears it again (a core that refused everything must fail here) */
    check(c, ds_irq_stim_set(wr, good, 2u) == (uint32_t)DS_STIMREF_NONE && wr->irq_stim_line[0] == 1u && wr->irq_stim_line[1] == 2u && wr->irq_stim_level[1] == 0u && wr->irq_stim_at[1] == 2u, "a valid irq_stim list before the first step was not accepted");
    check(c, ds_irq_stim_set(wr, NULL, 0u) == (uint32_t)DS_STIMREF_NONE && wr->irq_stim_line[0] == 0u && wr->irq_stim_line[1] == 0u && wr->irq_stim_at[1] == 0u, "a count of 0 did not clear the declared stimulus");
    check(c, ds_irq_stim_set(wr, good, (uint32_t)DS_IRQ_STIM_MAX + 1u) == (uint32_t)DS_STIMREF_count && wr->irq_stim_line[0] == 0u, "a list one longer than the maximum was not refused as stim.count");
    if (want == (uint32_t)DS_STIMREF_state) {
        struct ds_run_request rq;
        struct ds_run_result res;
        uint32_t reason;

        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        reason = ds_run(wr, &rq, &res);
        note_run(c, inst->machine_time, reason);
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && res.steps >= 1u && inst->machine_time != 0u, "the run before the state refusal took no step");
        sr_refused(c, wr, ev, n, want, &calls);
        sr_refused(c, wr, good, 2u, want, &calls); /* a valid list is refused too: the state is not a comparison */
        sr_refused(c, wr, NULL, 0u, want, &calls);
        check(c, ds_irq_stim_set(NULL, good, 2u) == want, "a NULL instance was not refused as stim.state");
        sr_observed[want] = 1u;
        calls++;
    } else {
        sr_refused(c, wr, ev, n, want, &calls);
        if (want == (uint32_t)DS_STIMREF_count) {
            sr_refused(c, wr, NULL, 1u, want, &calls); /* a NULL list with a nonzero count */
            sr_refused(c, wr, ev, 0xFFFFFFFFu, want, &calls);
        } else if (want == (uint32_t)DS_STIMREF_line) {
            for (i = 0u; i < 4u; i++) {
                static const uint32_t bad_lines[4] = {0u, 3u, 257u, 0xFFFFFFFFu}; /* 257 is 1 in the low byte: the full word is judged */
                tmp[0] = good[0];
                tmp[1] = good[1];
                tmp[i & 1u].line = bad_lines[i];
                sr_refused(c, wr, tmp, 2u, want, &calls);
            }
        } else if (want == (uint32_t)DS_STIMREF_level) {
            for (i = 0u; i < 3u; i++) {
                static const uint32_t bad_levels[3] = {2u, 256u, 0xFFFFFFFFu}; /* 256 is 0 in the low byte */
                tmp[0] = good[0];
                tmp[1] = good[1];
                tmp[i & 1u].level = bad_levels[i];
                sr_refused(c, wr, tmp, 2u, want, &calls);
            }
        } else if (want == (uint32_t)DS_STIMREF_order) {
            tmp[0].at = 100u;
            tmp[0].line = 1u;
            tmp[0].level = 1u;
            tmp[1] = tmp[0];
            tmp[1].at = 99u; /* one earlier than the event before it, the other line */
            tmp[1].line = 2u;
            sr_refused(c, wr, tmp, 2u, want, &calls);
        }
        /* equal times are not an order fault: the list below is accepted, and then cleared */
        tmp[0].at = 100u;
        tmp[0].line = 1u;
        tmp[0].level = 1u;
        tmp[1] = tmp[0];
        tmp[1].level = 0u;
        check(c, ds_irq_stim_set(wr, tmp, 2u) == (uint32_t)DS_STIMREF_NONE, "two events at the same time were refused");
        check(c, ds_irq_stim_set(wr, NULL, 0u) == (uint32_t)DS_STIMREF_NONE, "the stimulus could not be cleared");
    }
    check(c, calls >= 1u, "the refuse case made no refused call");
    release_boot(&b);
    snprintf(got, gotcap, "calls=%u", calls);
}

/*
 * stim_refusal_coverage (DEFERRED, D-86 pattern): every member of stim_refusals.def was observed by a refused=stim.NAME row, the table has no gap and every name prints as stim.NAME, and the stimulus maximum is the 16 events of D-112.
 */
void kind_stim_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    uint32_t n;
    unsigned members = 0u;
    unsigned seen = 0u;

    (void)k;
    for (n = 1u; n < (uint32_t)DS_STIMREF_SLOTS; n++) {
        const char *name = ds_stim_refusal_name(n);

        if (name == NULL) {
            continue;
        }
        members++;
        seen += sr_observed[n] != 0u ? 1u : 0u;
        check(c, strncmp(name, "stim.", 5u) == 0, "a stimulus refusal name does not start with stim.");
        check(c, sr_observed[n] != 0u, "a stimulus refusal code was never observed by a refused=stim.NAME row");
        if (sr_observed[n] == 0u) {
            emit_err("stim_refusal_coverage: %s was not observed by any refuse row\n", name);
        }
    }
    check(c, members > 0u, "stim_refusals.def has no member");
    check(c, (unsigned)DS_IRQ_STIM_MAX == 16u, "the stimulus maximum is not the 16 events of D-112");
    snprintf(got, gotcap, "members=%u observed=%u", members, seen);
}

/* the pad runs of one guest case (plan 03-29): the dev log of the case's own first run and the two extra runs of a pad-order item with the verdict block of each */
struct padset {
    struct padrun main;
    struct padrun a;
    struct padrun b;
    uint8_t blk_a[V_SIZE];
    uint8_t blk_b[V_SIZE];
};

/* 1 when two verdict blocks carry the same result bytes and the same observed bytes (the verdict of a pad run, R-7) */
static int pad_same(const uint8_t *x, const uint8_t *y)
{
    return memcmp(x + V_RES, y + V_RES, V_MAX_CHECKS) == 0 && memcmp(x + V_OBS, y + V_OBS, V_MAX_CHECKS) == 0;
}

/* the dev log of a run holds at least one read with CLR low and SEL high and one with CLR low and SEL low: the guest scanned both halves of the pad (D-118) */
static int pad_log_scans(const struct padrun *p)
{
    unsigned s1 = 0u;
    unsigned s0 = 0u;

    padlog_scan(p, &s1, &s0);
    return s1 != 0u && s0 != 0u;
}

static void guest_case(const struct kase *k, struct ctx *c, char *got, size_t gotcap, struct padset *ps)
{
    char rom[48];
    struct guest_expect ex;
    uint64_t checks = 0u;
    uint64_t until = 0u;
    struct ctx judged;
    struct obs_list ol;
    struct gstate g;
    char pbuf[PK_LIST_MAX];
    const char *plist[GP_MAX];
    unsigned np;
    unsigned i;
    uint8_t blk0[V_SIZE];
    const char *err;
    const char *outcome0 = NULL;
    char seen[96];
    int scratch;

    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0) {
        harness_error(c, "a guest case names its ROM as fixture = authored:NAME");
        return;
    }
    if (read_int_keys(k, &until, &checks) != 0) {
        harness_error(c, "a guest case needs the integer keys until_mclk and checks (checks at most 255)");
        return;
    }
    if (parse_guest_expect(k->val[K_EXPECT], &ex) != 0) {
        harness_error(c, "a guest expect is id=N, optionally followed by ;verdict=OUTCOME:REASON or ;poison-t0, and ;observe=IDX:VALUE items");
        return;
    }
    if (ex.nex != 0u) {
        harness_error(c, "a guest expect takes no exclude= item (that is a mutate_sweep item)");
        return;
    }
    if (((ex.padlog || ex.order_a[0] != '\0') && ex.mode != MODE_PLAIN && ex.mode != MODE_FAILSET) || (ex.order_a[0] != '\0' && (!ex.padlog || !k->has[K_INPUT]))) {
        harness_error(c, "a padlog or pad-order item belongs to a plain or fail-set guest case, and pad-order needs padlog and an input key");
        return;
    }
    memset(&judged, 0, sizeof judged);
    if (k->has[K_POLICIES] && case_required(k) && !policies_override_ok(k->val[K_MEANING])) {
        /* the guard (plan 03-24, D-127): the three-policy run is the default of every guest case, so a REQUIRED case may not narrow it silently; the control guest_policies_guard declares the verdict */
        harness_error(ex.mode == MODE_VERDICT ? &judged : c,
                      "policies-key: a REQUIRED guest case with an explicit policies key needs a meaning that starts with policies-override: and names the reason");
        if (ex.mode == MODE_VERDICT) {
            expect_verdict(c, &judged, &ex, got, gotcap);
        }
        return;
    }
    if (ex.mode == MODE_REFUSED) {
        if (ex.nobs != 0u) {
            harness_error(c, "a refused=stim.NAME case takes no observe= item");
            return;
        }
        stim_refuse(k, c, &ex, rom, until, got, gotcap);
        return;
    }
    if (ex.mode == MODE_REFUSED_INPUT) {
        if (ex.nobs != 0u) {
            harness_error(c, "a refused=input.NAME case takes no observe= item");
            return;
        }
        input_refuse(k, c, &ex, rom, until, got, gotcap);
        return;
    }
    if (ex.mode == MODE_EDGES) {
        if (ex.nobs != 0u) {
            harness_error(c, "an exact-edges case takes no observe= item");
            return;
        }
        input_edges(k, c, rom, got, gotcap);
        return;
    }
    np = policy_list(k, pbuf, sizeof pbuf, plist);
    if (np == 0u) {
        harness_error(c, "the policies key of a guest case is request fragments separated by || (at most 8)");
        return;
    }
    if (ex.pdiff != 0u && np < 2u) {
        harness_error(c, "policy-diff: a policy-diff=IDX case needs at least two policies in its list");
        return;
    }
    if (ex.mode == MODE_POISON) {
        np = 1u; /* the planted-block control never runs the guest */
    }
    err = read_observe(rom, &ol);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (ex.mode != MODE_POISON) {
        /* an observe row of the fixture without an expectation is a fault of the case (observe-missing), before the guest runs; a verdict control may declare it */
        observe_cover(ex.mode == MODE_VERDICT ? &judged : c, &ex, &ol, (unsigned)checks);
        if (ex.mode == MODE_VERDICT && judged.harness) {
            expect_verdict(c, &judged, &ex, got, gotcap);
            return;
        }
        if (c->harness) {
            return;
        }
    }
    g.kase = k;
    g.pad = NULL;
    g.rom = rom;
    g.until = until;
    g.checks = checks;
    g.ex = &ex;
    g.ol = &ol;
    scratch = ex.mode == MODE_VERDICT || ex.mode == MODE_FAILSET;
    if (ex.padlog && padrun_new(&ps->main) != 0) {
        harness_error(c, "out of memory for the pad log");
        return;
    }
    if (ex.order_a[0] != '\0') {
        /* the order triple (plan 03-29; D-118): the same ROM with the two other scripts, under the first policy, before the case's own run; their verdict blocks and dev logs are kept for the comparison below. Their outcomes are not judged here: a failing guest is what a negative script is for */
        const char *names[2];
        struct padrun *prs[2];
        uint8_t *blks[2];
        unsigned j;

        names[0] = ex.order_a;
        names[1] = ex.order_b;
        prs[0] = &ps->a;
        prs[1] = &ps->b;
        blks[0] = ps->blk_a;
        blks[1] = ps->blk_b;
        for (j = 0u; j < 2u; j++) {
            struct kase kx = *k;
            struct gstate gx = g;
            struct ctx xc;
            struct ctx xj;
            char xreq[512];
            char xgot[96];

            memset(&xc, 0, sizeof xc);
            memset(&xj, 0, sizeof xj);
            kx.val[K_INPUT] = names[j];
            kx.has[K_INPUT] = 1;
            if (padrun_new(prs[j]) != 0) {
                harness_error(c, "out of memory for the pad log");
                return;
            }
            if (compose_request(k->val[K_REQUEST], plist[0], xreq, sizeof xreq) != 0) {
                harness_error(c, "the request of the case with the policy fragment does not fit the request text");
                return;
            }
            gx.kase = &kx;
            gx.pad = prs[j];
            gx.request = xreq;
            if (!guest_once(&gx, &xc, &xj, blks[j], xgot, sizeof xgot) || xj.harness) {
                harness_error(c, xc.harness ? xc.why : (xj.harness ? xj.why : "pad-order: a run of the order triple did not complete"));
                return;
            }
        }
    }
    for (i = 0u; i < np; i++) {
        char req[512];
        struct ctx sc;
        struct ctx sj;
        uint8_t blk[V_SIZE];
        unsigned diff;
        char msg[200];
        int ran;

        if (compose_request(k->val[K_REQUEST], plist[i], req, sizeof req) != 0) {
            harness_error(c, "the request of the case with the policy fragment does not fit the request text");
            return;
        }
        g.request = req;
        g.pad = i == 0u && ex.padlog ? &ps->main : NULL;
        if (i == 0u) {
            ran = guest_once(&g, c, ex.mode == MODE_PLAIN ? c : &judged, blk0, got, gotcap);
            if (!ran) {
                return; /* a harness fault, or the poison control: its outcome is already set */
            }
            outcome0 = ctx_outcome(scratch ? &judged : c);
            if (ex.mode == MODE_VERDICT) {
                expect_verdict(c, &judged, &ex, got, gotcap);
            } else if (ex.mode == MODE_FAILSET) {
                expect_failset(c, &judged, blk0, &ex, (unsigned)checks, got, gotcap);
            }
            if (ex.padlog) {
                /* the pad log is an E0 runner artifact (D-118): written beside the evidence text, and the run must show the guest scanning both halves of the pad before it is trusted */
                if (padlog_write(k, &ps->main) != 0) {
                    harness_error(c, "pad-log: cannot write the pad log into the evidence directory");
                    return;
                }
                if (!pad_log_scans(&ps->main) || (ex.order_a[0] != '\0' && (!pad_log_scans(&ps->a) || !pad_log_scans(&ps->b)))) {
                    harness_error(c, "pad-log: the dev log of a run of the pad triple holds no read with SEL = 1 or none with SEL = 0 (CLR low), so it does not show the guest scanning both halves of the pad");
                    return;
                }
            }
            if (ex.order_a[0] != '\0') {
                int same_ac = pad_same(blk0, ps->blk_a);
                int same_bc = pad_same(blk0, ps->blk_b);
                int same_ab = pad_same(ps->blk_a, ps->blk_b);
                size_t gl = strlen(got);

                if (gl + 16u < gotcap) {
                    snprintf(got + gl, gotcap - gl, " same=%d%d%d", same_ac, same_bc, same_ab);
                }
                if (same_ac || same_bc || same_ab) { harness_error(c, "pad-order: two of the three runs (the case, and the two scripts of the pad-order item) have the same verdict bytes, so the pad does not decide the verdict"); return; } /* rule-pad-order */
            }
            continue;
        }
        memset(&sc, 0, sizeof sc);
        memset(&sj, 0, sizeof sj);
        ran = guest_once(&g, &sc, scratch ? &sj : &sc, blk, seen, sizeof seen);
        if (!ran) {
            snprintf(msg, sizeof msg, "policy %s: the guest did not run to the deadline (%s)", policy_name(plist[i]), sc.why);
            check(c, 0, msg);
            continue;
        }
        if (strcmp(ctx_outcome(scratch ? &sj : &sc), outcome0) != 0) {
            snprintf(msg, sizeof msg, "policy %s gave the outcome %s, policy %s gave %s: the guest depends on the power-on state", policy_name(plist[i]),
                     ctx_outcome(scratch ? &sj : &sc), policy_name(plist[0]), outcome0);
            check(c, 0, msg);
            continue;
        }
        diff = first_block_difference(&ex, blk0, blk);
        if (diff != 0u) {
            snprintf(msg, sizeof msg, "policy %s differs from policy %s at check %u (result or observed byte): the guest reads power-on state", policy_name(plist[i]), policy_name(plist[0]), diff);
            check(c, 0, msg);
        }
        if (ex.pdiff != 0u && ex.mode == MODE_PLAIN) {
            /* the policy-diff control: the row that reads power-on state must read a DIFFERENT byte under every policy after the first, or the comparison above could not see a dependence */
            snprintf(msg, sizeof msg, "policy-diff: observe row %u reads the same byte under policy %s and policy %s: the control cannot see a power-on dependence", ex.pdiff, policy_name(plist[0]), policy_name(plist[i]));
            check(c, blk[V_OBS + ex.pdiff - 1u] != blk0[V_OBS + ex.pdiff - 1u], msg);
        }
    }
}

void kind_guest(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct padset ps;

    memset(&ps, 0, sizeof ps);
    guest_case(k, c, got, gotcap, &ps);
    padrun_free(&ps.main);
    padrun_free(&ps.a);
    padrun_free(&ps.b);
}

/* ---- kinds model_refuse and model_refusal_coverage (plan 03-17; D-113, T-03-47, F4) ---- */

/*
 * model_refuse: expect is rom=NAME;refuse=CODE, CODE a name of src/core/model_refusals.def. The built authored image NAME is booted and every call of ds_model_set that the variants of CODE (a table below) name must return that
 * code, write nothing (the whole instance, raw bytes and digest, is the same before and after) and leave the selected value of the parameter where it was. The state variants first prove that a valid selection IS accepted before the
 * first step (so a core that refused everything would not pass the control), then run the machine and expect the refusal. The codes observed are kept for model_refusal_coverage.
 */
static unsigned char mr_observed[DS_MODELREF_SLOTS];

/* one call that must be refused with want: the instance (inst, live in mem of ds_instance_size() bytes) must be byte-identical and its digest the same afterwards */
static void mr_refused(struct ctx *c, struct ds_instance *inst, const char *name, const char *value, uint32_t want, unsigned *calls)
{
    size_t size = ds_instance_size();
    unsigned char *copy = (unsigned char *)malloc(size);
    uint8_t before[DS_DIGEST_BYTES];
    uint8_t after[DS_DIGEST_BYTES];
    uint32_t got;

    if (copy == NULL) {
        harness_error(c, "out of memory for the instance copy");
        return;
    }
    memcpy(copy, inst, size);
    check(c, ds_digest(inst, before) == 0u, "the digest of the instance before the refused ds_model_set was refused");
    got = ds_model_set(inst, name, value);
    check(c, got == want, "ds_model_set returned a code other than the one the row names");
    check(c, ds_model_refusal_name(got) != NULL && (got == 0u || strncmp(ds_model_refusal_name(got), "model.", 6u) == 0), "a model refusal does not print as model.NAME");
    check(c, memcmp(copy, inst, size) == 0, "a refused ds_model_set changed the instance");
    check(c, ds_digest(inst, after) == 0u && memcmp(before, after, DS_DIGEST_BYTES) == 0, "a refused ds_model_set changed the digest");
    if (got < (uint32_t)DS_MODELREF_SLOTS) {
        mr_observed[got] = 1u;
    }
    (*calls)++;
    free(copy);
}

/* the selected value of row 0 as text, compared with want */
static int mr_value_is(const struct ds_instance *inst, const char *want)
{
    size_t n = 0u;
    const char *v = ds_model_value(inst, 0u, &n);

    return v != NULL && n == strlen(want) && memcmp(v, want, n) == 0;
}

static void mr_variants(struct ctx *c, const char *code, struct ds_instance *inst, unsigned *calls)
{
    static const char *const bad_names[] = {NULL, "", "tma_zer", "tma_zero_", "TMA_ZERO", " tma_zero", "tma_zero ", "tma_zero|", "irq_i_sampl", "irq_i_sample ", "IRQ_I_SAMPLE", "irq_i_sample|tma_zero", "model.tma_zero"};
    static const char *const bad_values[] = {NULL, "", "bus_latc", "bus_latch_", "BUS_LATCH", "bus_latch|a_unchanged", "a_unchanged|", "|", " bus_latch", "bus_latch ", "0", "1", "a_unchange"};
    unsigned i;

    if (strcmp(code, "unknown_name") == 0) {
        for (i = 0u; i < sizeof bad_names / sizeof bad_names[0]; i++) {
            mr_refused(c, inst, bad_names[i], "bus_latch", (uint32_t)DS_MODELREF_unknown_name, calls);
        }
        mr_refused(c, inst, NULL, NULL, (uint32_t)DS_MODELREF_unknown_name, calls); /* the name is checked before the value */
    } else if (strcmp(code, "value_not_allowed") == 0) {
        for (i = 0u; i < sizeof bad_values / sizeof bad_values[0]; i++) {
            mr_refused(c, inst, "tma_zero", bad_values[i], (uint32_t)DS_MODELREF_value_not_allowed, calls);
        }
        /* the second row (plan 03-22): its values are pre and post, and the values of the other row are not its values */
        mr_refused(c, inst, "irq_i_sample", "bus_latch", (uint32_t)DS_MODELREF_value_not_allowed, calls);
        mr_refused(c, inst, "irq_i_sample", "a_unchanged", (uint32_t)DS_MODELREF_value_not_allowed, calls);
        mr_refused(c, inst, "irq_i_sample", "PRE", (uint32_t)DS_MODELREF_value_not_allowed, calls);
        mr_refused(c, inst, "irq_i_sample", "pre|post", (uint32_t)DS_MODELREF_value_not_allowed, calls);
        mr_refused(c, inst, "irq_i_sample", " post", (uint32_t)DS_MODELREF_value_not_allowed, calls);
        mr_refused(c, inst, "tma_zero", "post", (uint32_t)DS_MODELREF_value_not_allowed, calls);
    } else if (strcmp(code, "state") == 0) {
        struct ds_run_request rq;
        struct ds_run_result res;
        uint32_t reason;

        /* before the first step a valid selection is accepted, also twice and back, and the instance then holds it (a core that refused everything must fail here) */
        check(c, mr_value_is(inst, "bus_latch"), "a fresh instance does not hold the default value of tma_zero");
        check(c, ds_model_set(inst, "tma_zero", "a_unchanged") == (uint32_t)DS_MODELREF_NONE && mr_value_is(inst, "a_unchanged"), "a valid selection before the first step was not accepted");
        check(c, ds_model_set(inst, "tma_zero", "bus_latch") == (uint32_t)DS_MODELREF_NONE && mr_value_is(inst, "bus_latch"), "the default selection before the first step was not accepted");
        check(c, inst->irq_i_sample == (uint8_t)DS_IRQ_I_SAMPLE_PRE, "a fresh instance does not hold the default value of irq_i_sample");
        check(c, ds_model_set(inst, "irq_i_sample", "post") == (uint32_t)DS_MODELREF_NONE && inst->irq_i_sample == (uint8_t)DS_IRQ_I_SAMPLE_POST, "irq_i_sample = post before the first step was not accepted");
        check(c, ds_model_set(inst, "irq_i_sample", "pre") == (uint32_t)DS_MODELREF_NONE && inst->irq_i_sample == (uint8_t)DS_IRQ_I_SAMPLE_PRE, "irq_i_sample = pre before the first step was not accepted");
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = 0u; /* a deadline that is not ahead takes no step: the machine is still before its first step */
        reason = ds_run(inst, &rq, &res);
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && res.steps == 0u && inst->machine_time == 0u, "a zero-step run moved the machine");
        check(c, ds_model_set(inst, "tma_zero", "a_unchanged") == (uint32_t)DS_MODELREF_NONE && mr_value_is(inst, "a_unchanged"), "a selection after a run that took no step was refused");
        check(c, ds_model_set(inst, "tma_zero", "bus_latch") == (uint32_t)DS_MODELREF_NONE, "the selection could not be put back");
        rq.until_mclk = 600u;
        reason = ds_run(inst, &rq, &res);
        note_run(c, inst->machine_time, reason);
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && res.steps >= 1u && inst->machine_time != 0u, "the first run of the state variants took no step");
        mr_refused(c, inst, "tma_zero", "a_unchanged", (uint32_t)DS_MODELREF_state, calls);
        mr_refused(c, inst, "tma_zero", "bus_latch", (uint32_t)DS_MODELREF_state, calls); /* the value it already holds is refused too: the state is not a comparison */
        mr_refused(c, inst, "irq_i_sample", "post", (uint32_t)DS_MODELREF_state, calls);
        check(c, mr_value_is(inst, "bus_latch"), "a refused selection changed the value");
        /* the arguments are judged before the state: a bad name or value after the first step keeps its own code */
        mr_refused(c, inst, "tma_zer", "bus_latch", (uint32_t)DS_MODELREF_unknown_name, calls);
        mr_refused(c, inst, "tma_zero", "bogus", (uint32_t)DS_MODELREF_value_not_allowed, calls);
        /* a NULL instance is the state refusal */
        check(c, ds_model_set(NULL, "tma_zero", "bus_latch") == (uint32_t)DS_MODELREF_state, "a NULL instance was not refused as model.state");
        mr_observed[DS_MODELREF_state] = 1u;
        (*calls)++;
    } else {
        harness_error(c, "a model_refuse row names a code that model_refusals.def does not have");
    }
}

void kind_model_refuse(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char rom[48];
    char code[32];
    char *rest = list;
    char *kv;
    int have = 0;
    struct booted b;
    const struct ds_instance *inst = NULL;
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
            harness_error(c, "a model_refuse expect is rom=NAME;refuse=CODE");
            return;
        }
        *eq = '\0';
        if (strcmp(kv, "rom") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof rom && strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(eq + 1)) {
            memcpy(rom, eq + 1, strlen(eq + 1) + 1u);
            have |= 1;
        } else if (strcmp(kv, "refuse") == 0 && strlen(eq + 1) > 0u && strlen(eq + 1) < sizeof code && strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789_") == strlen(eq + 1)) {
            memcpy(code, eq + 1, strlen(eq + 1) + 1u);
            have |= 2;
        } else {
            harness_error(c, "a model_refuse expect is rom=NAME;refuse=CODE");
            return;
        }
    }
    if (have != 3) {
        harness_error(c, "a model_refuse expect is rom=NAME;refuse=CODE");
        return;
    }
    err = boot_built_rom(rom, k->val[K_REQUEST], &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    mr_variants(c, code, (struct ds_instance *)(void *)b.mem, &calls);
    check(c, calls >= 1u, "the model_refuse case made no refused call");
    release_boot(&b);
    snprintf(got, gotcap, "calls=%u", calls);
}

/*
 * model_refusal_coverage (DEFERRED, D-86 pattern): every member of model_refusals.def was observed by a model_refuse row, and the rows of model_params.def are well formed: at most 4, at least two allowed values each (a row exists only because
 * two values are implemented, D-113), the default is the first allowed value (a zeroed instance holds index 0), the ledger id has the LED-AREA-NNNN shape and the status is assumed or open (never supported, ER-2).
 */
void kind_model_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    uint32_t n;
    uint32_t r;
    unsigned members = 0u;
    unsigned seen = 0u;

    (void)k;
    for (n = 1u; n < (uint32_t)DS_MODELREF_SLOTS; n++) {
        const char *name = ds_model_refusal_name(n);

        if (name == NULL) {
            continue;
        }
        members++;
        seen += mr_observed[n] != 0u ? 1u : 0u;
        check(c, mr_observed[n] != 0u, "a model refusal code was never observed by a model_refuse row");
        if (mr_observed[n] == 0u) {
            emit_err("model_refusal_coverage: %s was not observed by any model_refuse row\n", name);
        }
    }
    check(c, members > 0u, "model_refusals.def has no member");
    check(c, (unsigned)DS_MODEL_COUNT >= 1u && (unsigned)DS_MODEL_COUNT <= 4u, "model_params.def has no row or more than the 4 rows of D-113");
    for (r = 0u; r < (uint32_t)DS_MODEL_COUNT; r++) {
        const char *allowed = ds_model_allowed(r);
        const char *def = ds_model_default(r);
        const char *led = ds_model_ledger(r);
        const char *status = ds_model_status(r);
        size_t first = allowed != NULL ? strcspn(allowed, "|") : 0u;

        check(c, ds_model_name(r) != NULL && strlen(ds_model_name(r)) > 0u, "a model parameter has no name");
        check(c, allowed != NULL && allowed[first] == '|', "a model parameter has fewer than two allowed values");
        check(c, def != NULL && allowed != NULL && strlen(def) == first && strncmp(def, allowed, first) == 0, "the default of a model parameter is not the first allowed value");
        check(c, led != NULL && strlen(led) == 12u && strncmp(led, "LED-", 4u) == 0, "the ledger id of a model parameter is not LED-AREA-NNNN");
        check(c, status != NULL && (strcmp(status, "assumed") == 0 || strcmp(status, "open") == 0), "the status of a model parameter is neither assumed nor open");
    }
    snprintf(got, gotcap, "members=%u observed=%u rows=%u", members, seen, (unsigned)DS_MODEL_COUNT);
}
