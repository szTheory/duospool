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
 *                   ;poison-t0               the runner writes the DSV1 magic and the done state into work RAM after boot and before the first step; the
 *                                            t=0 precondition must then give harness_failure ("before the first step"), and the guest is not run at all
 *                 Observe rows (plan 03-10): the optional fifth column of NAME.checks, the word observe, marks a check whose expected value legitimately differs by case; scripts/fixture-build lists
 *                 such indexes in build/gate/fixtures/NAME.observe, the guest writes its observed byte and result $50, and the case judges the byte through one item observe=IDX:VALUE per index
 *                 (id=1;observe=3:0x16). An index without an item is harness_failure (observe-missing), an item for a check that is not an observe row too (observe-unlisted).
 *                 Power-on policies (plan 03-11; D-127): the ROM is booted and run once per policy of the list (the optional key policies, request fragments separated by ||, or the built-in default of zero and
 *                 seeded-prng-v1 seed 42), and the DSV1 result bytes +10..+6F and observed bytes +80..+DF of every run must equal those of the first run; the first difference is a guest_failure naming the check.
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
    if (strncmp(item, "observe=", 8u) == 0) {
        unsigned idx = 0u;
        unsigned i;
        uint64_t value = 0u;
        int differs;

        item += 8;
        colon = strchr(item, ':');
        if (colon == NULL || e->nobs >= GE_ITEMS_MAX) {
            return -1;
        }
        *colon = '\0';
        differs = strcmp(colon + 1, "differs") == 0;
        if (parse_idx(item, &idx) != 0 || (!differs && (parse_u64(colon + 1, &value) != 0 || value > 255u))) {
            return -1;
        }
        for (i = 0u; i < e->nobs; i++) {
            if (e->obs[i].idx == idx) {
                return -1;
            }
        }
        e->obs[e->nobs].idx = idx;
        e->obs[e->nobs].differs = differs;
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
            if (e->obs[j].differs) {
                harness_error(t, "observe-differs: observe=IDX:differs is judged only in policy-diff mode, which this case does not use");
                continue;
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
 * policies (request fragments separated by ||, an empty fragment is the default zero policy) or, when the key is absent, the built-in default below. Plan 03-24 switches the default to zero plus two seeded-prng-v2 seeds, so cases never
 * repeat the list. Each fragment is appended to the case's own request with a semicolon. The poison-t0 control does not run the guest, so it uses the first policy only.
 */
static const char *const default_policies[] = {"", "bind.powerup_policy=seeded-prng-v1;seed=42"};
#define GP_DEFAULT_COUNT 2u
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

struct gstate {
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
    const struct guest_expect *ex = g->ex;

    err = boot_built_rom(g->rom, g->request, &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
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
    reason = ds_run((struct ds_instance *)(void *)b.mem, &run, &out);
    note_run(c, inst->machine_time, reason);
    if (reason != (uint32_t)DS_RUN_REASON_UNTIL) {
        harness_error(c, "ds_run refused the request or stopped for another reason than the deadline");
        release_boot(&b);
        return 0;
    }
    judge_dsv1(judged, inst->ram8k + V_OFF, (unsigned)ex->id, (unsigned)g->checks, got, gotcap);
    if (!judged->harness && !judged->failed && !judged->timeout) {
        observe_judge(judged, inst->ram8k + V_OFF, ex, g->ol);
    }
    memcpy(blk, inst->ram8k + V_OFF, V_SIZE);
    release_boot(&b);
    return 1;
}

/* the first check whose result or observed byte differs between two blocks (1-based), 0 when none */
static unsigned first_block_difference(const uint8_t *a, const uint8_t *b)
{
    unsigned i;

    for (i = 0u; i < V_MAX_CHECKS; i++) {
        if (a[V_RES + i] != b[V_RES + i] || a[V_OBS + i] != b[V_OBS + i]) {
            return i + 1u;
        }
    }
    return 0u;
}

void kind_guest(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
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
    np = policy_list(k, pbuf, sizeof pbuf, plist);
    if (np == 0u) {
        harness_error(c, "the policies key of a guest case is request fragments separated by || (at most 8)");
        return;
    }
    if (ex.mode == MODE_POISON) {
        np = 1u; /* the planted-block control never runs the guest */
    }
    memset(&judged, 0, sizeof judged);
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
    g.rom = rom;
    g.until = until;
    g.checks = checks;
    g.ex = &ex;
    g.ol = &ol;
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
        if (i == 0u) {
            ran = guest_once(&g, c, ex.mode == MODE_PLAIN ? c : &judged, blk0, got, gotcap);
            if (!ran) {
                return; /* a harness fault, or the poison control: its outcome is already set */
            }
            outcome0 = ctx_outcome(ex.mode == MODE_VERDICT ? &judged : c);
            if (ex.mode == MODE_VERDICT) {
                expect_verdict(c, &judged, &ex, got, gotcap);
            }
            continue;
        }
        memset(&sc, 0, sizeof sc);
        memset(&sj, 0, sizeof sj);
        ran = guest_once(&g, &sc, ex.mode == MODE_VERDICT ? &sj : &sc, blk, seen, sizeof seen);
        if (!ran) {
            snprintf(msg, sizeof msg, "policy %s: the guest did not run to the deadline (%s)", policy_name(plist[i]), sc.why);
            check(c, 0, msg);
            continue;
        }
        if (strcmp(ctx_outcome(ex.mode == MODE_VERDICT ? &sj : &sc), outcome0) != 0) {
            snprintf(msg, sizeof msg, "policy %s gave the outcome %s, policy %s gave %s: the guest depends on the power-on state", policy_name(plist[i]),
                     ctx_outcome(ex.mode == MODE_VERDICT ? &sj : &sc), policy_name(plist[0]), outcome0);
            check(c, 0, msg);
            continue;
        }
        diff = first_block_difference(blk0, blk);
        if (diff != 0u) {
            snprintf(msg, sizeof msg, "policy %s differs from policy %s at check %u (result or observed byte): the guest reads power-on state", policy_name(plist[i]), policy_name(plist[0]), diff);
            check(c, 0, msg);
        }
    }
}
