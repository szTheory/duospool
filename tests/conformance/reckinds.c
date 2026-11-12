/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * reckinds.c - the case kinds of the run record as a byte-comparable artifact (plan 14; D-53, D-69, D-80, D-86). Evidence tier E0: a
 * golden shows that the record is the same bytes as an earlier run of the same inputs, which is repeatability. It never shows that the
 * machine behaviour is right. Test code only, never installed.
 *   record_golden        boots the request, renders the record into a buffer (nothing goes to stdout, D-80) and compares it byte for byte
 *                        with tests/conformance/golden/NAME.record.toml after the golden's leading comment block (the lines that start with
 *                        #); expect is NAME. build_id is the default constant and behaviour_epoch is part of the bytes: the golden never
 *                        carries a git build id (D-53)
 *   record_refusal       boots a request that must be refused (expect is the refusal name) and renders the refusal record: record, core and
 *                        refusal tables and nothing after, so no machine table. The record text is also printed into the case evidence
 *   record_refuses_quote the request is two or more requests separated by "||", each naming a hostile profile id built with the @hex:
 *                        value prefix (a double quote, a backslash, a non-ASCII byte); each is refused by the core and the printer then
 *                        returns an error status, an empty text and a zero length, and nothing is written to DIR (D-69). A clean id
 *                        printed first shows that the error comes from the id and not from the request
 * The --records DIR contract (D-86, one statement): every case of kind accept, record_lines, record_golden or record_refusal that
 * produces a record writes exactly one file DIR/ID.record.toml holding the record text only, ID being the case id; no other kind writes
 * anything, record_refuses_quote included, and the runner creates nothing else in DIR.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_sha256.h"
#include "ds_text.h"
#include "runner.h"
#include "synth.h"

/* what the record says about the request that made the instance: the result of the last run (NULL: none) and the model rows the request bound with model.NAME (bit i is row i of model_params.def) */
static struct ds_text_run run_of(const struct spec *s, const struct ds_run_result *result)
{
    struct ds_text_run run;
    unsigned i;
    uint32_t row;

    run.result = result;
    run.model_bound = 0u;
    for (i = 0u; i < s->nmodel; i++) {
        for (row = 0u; row < (uint32_t)DS_MODEL_COUNT; row++) {
            if (strcmp(s->model_name[i], ds_model_name(row)) == 0) {
                run.model_bound |= UINT32_C(1) << row;
            }
        }
    }
    return run;
}

const char *records_dir = NULL;
const char *golden_dir = NULL;

/* write DIR/ID.record.toml (when --records was given); a failure is a harness fault of the run, never a verdict on the guest */
void record_emit(const struct kase *k, struct ctx *c, const char *text, size_t len)
{
    char path[512];
    FILE *fp;
    int n;

    if (records_dir == NULL) {
        return;
    }
    n = snprintf(path, sizeof path, "%s/%s.record.toml", records_dir, k->stem);
    if (n < 0 || (size_t)n >= sizeof path) {
        harness_error(c, "the record path is too long");
        return;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        harness_error(c, "cannot create the record file");
        return;
    }
    if (fwrite(text, 1u, len, fp) != len || fclose(fp) != 0) {
        harness_error(c, "cannot write the record file");
    }
}

/* kind accept: the record of the accepted boot (the boot itself was judged by kind_boot) */
void record_for_accept(const struct kase *k, struct ctx *c)
{
    static char text[REC_CAP];
    struct booted b;
    struct spec s;
    struct ds_text_run run;
    const struct ds_instance *inst = NULL;
    size_t len = 0u;
    const char *err;

    if (records_dir == NULL) {
        return;
    }
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    run = run_of(&s, NULL);
    if (ds_text_record(inst, &run, text, sizeof text, &len) != DS_TEXT_OK) {
        harness_error(c, "ds_text_record failed on an accepted instance");
    } else {
        record_emit(k, c, text, len);
    }
    release_boot(&b);
}

/* a golden or reason name: lowercase letters, digits and underscores, so that no path part can come from a case file */
static int plain_name(const char *s)
{
    size_t i;

    if (s[0] == '\0' || strlen(s) > 80u) {
        return 0;
    }
    for (i = 0u; s[i] != '\0'; i++) {
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '_')) {
            return 0;
        }
    }
    return 1;
}

/* the first byte after the leading comment block: every line that starts with # belongs to it */
static const char *golden_body(const char *file)
{
    const char *p = file;

    while (*p == '#') {
        while (*p != '\0' && *p != '\n') {
            p++;
        }
        if (*p == '\n') {
            p++;
        }
    }
    return p;
}

void kind_record_golden(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    static char again[REC_CAP];
    char path[512];
    struct booted b;
    struct spec s;
    struct ds_text_run run;
    const struct ds_instance *inst = NULL;
    const char *name = k->val[K_EXPECT];
    const char *err;
    char *golden;
    const char *body;
    size_t len = 0u;
    size_t alen = 0u;
    size_t blen;
    size_t at;
    int n;

    if (!plain_name(name)) {
        harness_error(c, "the expect of a record_golden case is not a plain golden name");
        return;
    }
    if (golden_dir == NULL) {
        harness_error(c, "a record_golden case needs --golden-dir");
        return;
    }
    n = snprintf(path, sizeof path, "%s/%s.record.toml", golden_dir, name);
    if (n < 0 || (size_t)n >= sizeof path) {
        harness_error(c, "the golden path is too long");
        return;
    }
    golden = read_file(path);
    if (golden == NULL) {
        harness_error(c, "the golden file cannot be read");
        return;
    }
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        free(golden);
        harness_error(c, err);
        return;
    }
    run = run_of(&s, NULL);
    check(c, ds_text_record(inst, &run, text, sizeof text, &len) == DS_TEXT_OK && len == strlen(text) && len > 0u,
          "ds_text_record failed on an accepted instance");
    check(c, ds_text_record(inst, &run, again, sizeof again, &alen) == DS_TEXT_OK && alen == len && memcmp(again, text, len + 1u) == 0,
          "a second print differs from the first");
    body = golden_body(golden);
    blen = strlen(body);
    check(c, blen > 0u, "the golden holds nothing after its comment block");
    for (at = 0u; at < len && at < blen && text[at] == body[at]; at++) {
    }
    snprintf(got, gotcap, at == len && at == blen ? "equal bytes=%u" : "diff_at=%u", (unsigned)at);
    check(c, at == len && at == blen, "the printed record differs from the golden byte for byte");
    check(c, len == 0u || text[len - 1u] == '\n', "the record does not end in a newline");
    record_emit(k, c, text, len);
    free(golden);
    release_boot(&b);
}

/* the exact tail of a refusal record, built here from the three values the case names */
static void refusal_tail(char *out, size_t cap, const char *reason, const char *id, uint32_t rev)
{
    snprintf(out, cap, "\n[refusal]\nreason = \"%s\"\nrequested_id = \"%s\"\nrequested_rev = %u\n", reason, id, (unsigned)rev);
}

void kind_record_refusal(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    static char scratch[REC_CAP];
    char tail[256];
    struct spec s;
    struct expect e;
    size_t natural = synth_size(k->val[K_FIXTURE]);
    size_t len = 0u;
    size_t tlen;
    const char *err;
    const char *reason;
    enum ds_text_status st;

    if (natural == 0u) {
        harness_error(c, "unknown fixture recipe");
        return;
    }
    err = parse_spec(&s, k->val[K_REQUEST], natural);
    if (err == NULL) {
        err = parse_expect(&e, k->val[K_EXPECT]);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (e.accept) {
        harness_error(c, "a record_refusal case expects a refusal");
        return;
    }
    run_boot(k->val[K_FIXTURE], &s, &e, c, got, gotcap); /* the request is refused as the case says, and the instance memory is untouched */
    if (c->harness) {
        return;
    }
    reason = ds_refusal_name(e.reason);
    st = ds_text_refusal(e.reason, s.profile, s.rev, text, sizeof text, &len);
    check(c, st == DS_TEXT_OK && reason != NULL && len == strlen(text), "ds_text_refusal failed on a refusal");
    if (st != DS_TEXT_OK || reason == NULL) {
        return;
    }
    check(c, strncmp(text, "[record]\nformat_version = 7\n\n[core]\nbuild_id = \"", 47u) == 0, "the refusal record does not start with the record and core tables");
    check(c, strstr(text, "\nbehaviour_epoch = 5\n") != NULL, "the refusal record has no behaviour_epoch line");
    refusal_tail(tail, sizeof tail, reason, s.profile, s.rev);
    tlen = strlen(tail);
    check(c, len > tlen && strcmp(text + len - tlen, tail) == 0, "the record does not end in the refusal table with reason, requested id and rev");
    check(c, strstr(text, "[refusal]") != NULL && strstr(text, "[machine]") == NULL, "a refusal record holds a machine table or no refusal table");
    check(c, strstr(text, "[profile]") == NULL && strstr(text, "[content]") == NULL && strstr(text, "[power_on]") == NULL,
          "a refusal record holds a table of an accepted run");
    /* the capacity edge and the argument checks, as for the accept record */
    check(c, ds_text_refusal(e.reason, s.profile, s.rev, scratch, len + 1u, &tlen) == DS_TEXT_OK && tlen == len && strcmp(scratch, text) == 0,
          "a buffer of exactly len + 1 bytes did not fit the refusal record");
    memset(scratch, 0x7f, sizeof scratch);
    check(c, ds_text_refusal(e.reason, s.profile, s.rev, scratch, len, &tlen) == DS_TEXT_ERR_CAPACITY && tlen == 0u && scratch[0] == '\0',
          "a buffer one byte short did not give capacity and an empty text");
    check(c, ds_text_refusal(0u, s.profile, s.rev, scratch, sizeof scratch, &tlen) == DS_TEXT_ERR_STATE && tlen == 0u,
          "reason 0 (an accept) was printed as a refusal");
    check(c, ds_text_refusal(900u, s.profile, s.rev, scratch, sizeof scratch, &tlen) == DS_TEXT_ERR_STATE && tlen == 0u,
          "a reason that is not a member of the refusal set was printed");
    check(c, ds_text_refusal(e.reason, NULL, s.rev, scratch, sizeof scratch, &tlen) == DS_TEXT_ERR_ARGUMENT && tlen == 0u,
          "a NULL requested id was not an argument error");
    set_evidence_detail(text);
    record_emit(k, c, text, len);
}

void kind_record_refuses_quote(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    char work[512];
    char *rest;
    char *part;
    struct spec s;
    struct expect e;
    size_t natural = synth_size(k->val[K_FIXTURE]);
    size_t len;
    unsigned seen = 0u;
    const char *err;
    size_t plen = 0u;

    if (natural == 0u) {
        harness_error(c, "unknown fixture recipe");
        return;
    }
    err = parse_expect(&e, k->val[K_EXPECT]);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (e.accept) {
        harness_error(c, "a record_refuses_quote case expects a refusal");
        return;
    }
    /* the control: a clean requested id prints, so that the refusals below come from the id and from nothing else */
    memset(text, 0x7f, sizeof text);
    check(c, ds_text_refusal(e.reason, "nope", 1u, text, sizeof text, &len) == DS_TEXT_OK && strstr(text, "requested_id = \"nope\"\n") != NULL,
          "a clean requested id was not printed");
    if (pk_copy_text(k->val[K_REQUEST], work, sizeof work) != 0) {
        harness_error(c, "the request list is too long");
        return;
    }
    rest = work;
    while (!c->harness && (part = pk_next_item(&rest, '|')) != NULL) {
        char two[512];

        /* the list separator is two bars: an item that starts with a bar is the empty piece between them */
        if (*part == '\0') {
            continue;
        }
        if (pk_copy_text(part, two, sizeof two) != 0) {
            harness_error(c, "a request of the list is too long");
            break;
        }
        err = parse_spec(&s, two, natural);
        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        run_boot(k->val[K_FIXTURE], &s, &e, c, got, gotcap);
        if (c->harness) {
            break;
        }
        memset(text, 0x7f, sizeof text);
        len = 12345u;
        check(c, ds_text_refusal(e.reason, s.profile, s.rev, text, sizeof text, &len) == DS_TEXT_ERR_STRING,
              "a requested id with a double quote, a backslash or a non-ASCII byte did not give the string error");
        check(c, len == 0u && text[0] == '\0', "the printer emitted text for a hostile requested id");
        check(c, memchr(text, '[', sizeof text) == NULL, "a table header of the failed print was left in the buffer");
        seen++;
    }
    /* a requested id that is 64 characters long does not fit its field: the same error, never a cut-off value */
    {
        char longid[DS_TEXT_REQUEST_ID_MAX + 1u];
        memset(longid, 'a', sizeof longid - 1u);
        longid[sizeof longid - 1u] = '\0';
        check(c, ds_text_refusal(e.reason, longid, 1u, text, sizeof text, &plen) == DS_TEXT_ERR_STRING && plen == 0u,
              "an over-long requested id was not refused");
    }
    check(c, seen >= 2u, "the case names fewer than two hostile requests");
    snprintf(got, gotcap, "hostile=%u", seen);
}

/* ---- kind record_lines: the canonical record skeleton, line by line (D-53, D-69); moved here from runner.c by plan 03-03 ---- */

/* the keys the record must print, in order: table.key (D-53: record, core, profile with its sha256, 13 field tables, withheld, content with the
 * sha256 first, power_on, one model_NAME table per row of model_params.def, stimulus (the key events: this skeleton declares no stimulus, so no event line; plan 03-23), input (events and schedule_sha256; plan 03-28), constants (one key per row of constants.def), run, limits, machine).
 * The count below is pinned: a plan that adds a key or a table edits this list and the count in one commit */
/* the number of rows of constants.def */
#define DS_CONST(name, value, ledger) +1
enum { CONST_ROWS = 0
#include "constants.def"
};
#undef DS_CONST

static char exp_key[REC_MAX_ROWS][KEY_MAX];
static size_t n_exp;

static void exp_add(const char *prefix, const char *table, const char *key)
{
    size_t a = strlen(prefix);
    size_t b = strlen(table);
    size_t d = strlen(key);

    if (n_exp < REC_MAX_ROWS && a + b + 1u + d < KEY_MAX) {
        memcpy(exp_key[n_exp], prefix, a);
        memcpy(exp_key[n_exp] + a, table, b);
        exp_key[n_exp][a + b] = '.';
        memcpy(exp_key[n_exp] + a + b + 1u, key, d + 1u);
        n_exp++;
    }
}

/* the ledger id of every row of constants.def, in file order (read from the def by the preprocessor, not from the printed record) */
#define DS_CONST(name, value, ledger) ledger,
static const char *const const_ledger[CONST_ROWS] = {
#include "constants.def"
};
#undef DS_CONST

/* the sorted, duplicate-free list of ledger ids that the [withheld] table must hold for a request (plan 03-29): the ids of the fields the request bound, the unresolved behaviours of the profile, the three load rules that are assumptions, and the ledger id of every model row and every constants row.
 * It is rebuilt here from the defs and the request, not from the printer, so the table is checked against the sources and not against itself. Returns the count, or 0 when it does not fit. */
#define WH_EXP_MAX 64u
static char wh_exp[WH_EXP_MAX][16];

static int wh_add(unsigned *n, const char *s, size_t len)
{
    unsigned i;
    unsigned at;

    if (len == 0u || len >= sizeof wh_exp[0]) {
        return -1;
    }
    for (at = 0u; at < *n; at++) {
        int cmp = strncmp(s, wh_exp[at], len);

        if (cmp == 0 && wh_exp[at][len] == '\0') {
            return 0;
        }
        if (cmp < 0 || (cmp == 0 && wh_exp[at][len] != '\0')) {
            break;
        }
    }
    if (*n == WH_EXP_MAX) {
        return -1;
    }
    for (i = *n; i > at; i--) {
        memcpy(wh_exp[i], wh_exp[i - 1u], sizeof wh_exp[0]);
    }
    memcpy(wh_exp[at], s, len);
    wh_exp[at][len] = '\0';
    (*n)++;
    return 0;
}

static int wh_add_list(unsigned *n, const char *list)
{
    while (*list != '\0') {
        size_t len = strcspn(list, "|");

        if (wh_add(n, list, len) != 0) {
            return -1;
        }
        list += len;
        if (*list == '|') {
            list++;
        }
    }
    return 0;
}

static unsigned withheld_expected(const struct spec *s)
{
    unsigned n = 0u;
    uint32_t i;
    int bad = 0;

    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        if (s->bind[i] != NULL) {
            bad |= wh_add_list(&n, ds_field_ledger(i)) != 0;
        }
    }
    for (i = 0u; i < ds_profile_unresolved_count(); i++) {
        bad |= wh_add_list(&n, ds_profile_unresolved(i)) != 0;
    }
    bad |= wh_add_list(&n, "LED-LOAD-0003|LED-LOAD-0005|LED-PROF-0002") != 0;
    for (i = 0u; i < (uint32_t)DS_MODEL_COUNT; i++) {
        bad |= wh_add_list(&n, ds_model_ledger(i)) != 0;
    }
    for (i = 0u; i < (uint32_t)CONST_ROWS; i++) {
        bad |= wh_add_list(&n, const_ledger[i]) != 0;
    }
    return bad ? 0u : n;
}

static int value_is(const char *raw, const char *want);

/* 1 when one of the rows withheld.id_N holds the ledger id (one id per row since format version 6) */
static int withheld_has(const struct recrow *rows, size_t n, const char *id)
{
    size_t i;

    for (i = 0u; i < n; i++) {
        if (strncmp(rows[i].key, "withheld.id_", 12u) == 0 && value_is(rows[i].val, id)) {
            return 1;
        }
    }
    return 0;
}

static unsigned exp_events; /* the events the case declares: the skeleton holds one event_N line each after stimulus.events (plan 03-23) */

static void build_expected_keys(unsigned nids)
{
    uint32_t i;

    n_exp = 0u;
    exp_add("", "record", "format_version");
    exp_add("", "core", "build_id");
    exp_add("", "core", "behaviour_epoch");
    exp_add("", "profile", "id");
    exp_add("", "profile", "rev");
    exp_add("", "profile", "sha256");
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        char tab[KEY_MAX];
        size_t n = strlen(ds_field_name(i));

        if (n + 7u >= sizeof tab) {
            break;
        }
        memcpy(tab, "field_", 6u);
        memcpy(tab + 6, ds_field_name(i), n + 1u);
        exp_add("", tab, "status");
        exp_add("", tab, "value");
        exp_add("", tab, "bound_by");
    }
    exp_add("", "withheld", "count");
    for (i = 1u; i <= nids; i++) { /* one row id_N per withheld ledger id (format version 6) */
        char name[16];

        snprintf(name, sizeof name, "id_%u", (unsigned)i);
        exp_add("", "withheld", name);
    }
    exp_add("", "content", "sha256");
    exp_add("", "content", "size");
    exp_add("", "content", "mapper");
    exp_add("", "content", "mapper_rule");
    exp_add("", "content", "byte_order");
    exp_add("", "content", "header_handling");
    exp_add("", "power_on", "policy");
    exp_add("", "power_on", "policy_version");
    exp_add("", "power_on", "seed_present");
    exp_add("", "power_on", "seed");
    for (i = 0u; i < (uint32_t)DS_MODEL_COUNT; i++) { /* one table per row of model_params.def, in file order */
        char tab[KEY_MAX];
        size_t n = strlen(ds_model_name(i));

        if (n + 7u >= sizeof tab) {
            break;
        }
        memcpy(tab, "model_", 6u);
        memcpy(tab + 6, ds_model_name(i), n + 1u);
        exp_add("", tab, "value");
        exp_add("", tab, "ledger");
        exp_add("", tab, "status");
        exp_add("", tab, "bound_by");
    }
    exp_add("", "stimulus", "events");
    for (i = 0u; i < exp_events; i++) {
        char evkey[24];

        snprintf(evkey, sizeof evkey, "event_%u", (unsigned)(i + 1u));
        exp_add("", "stimulus", evkey);
    }
    exp_add("", "input", "events");
    exp_add("", "input", "schedule_sha256");
#define DS_CONST(name, value, ledger) exp_add("", "constants", #name);
#include "constants.def"
#undef DS_CONST
    exp_add("", "run", "reason");
    exp_add("", "run", "overshoot");
    exp_add("", "run", "stop_point");
    exp_add("", "run", "steps");
    exp_add("", "limits", "size_cap");
    exp_add("", "machine", "time");
    exp_add("", "machine", "time_unit");
    exp_add("", "machine", "digest_algorithm");
    exp_add("", "machine", "digest");
}

/*
 * Split canonical text (in place) into TABLE.KEY rows. Layout rules: every line ends in LF, a table header is `[name]`, a key line
 * is `key = value` inside a table, one blank line sits before every table but the first, and nowhere else. NULL on success,
 * otherwise the first fault.
 */
const char *parse_record(char *text, struct recrow *rows, size_t *nrows)
{
    char table[48];
    int have_table = 0;
    int prev_blank = 0;
    int first_line = 1;
    char *line = text;

    table[0] = '\0';
    *nrows = 0u;
    while (*line != '\0') {
        char *nl = strchr(line, '\n');
        size_t n;
        if (nl == NULL) {
            return "the text does not end in a newline";
        }
        *nl = '\0';
        n = strlen(line);
        if (n == 0u) {
            if (prev_blank || first_line) {
                return "a blank line where none belongs";
            }
            prev_blank = 1;
        } else if (line[0] == '[') {
            if (line[n - 1u] != ']' || n < 3u || n - 2u >= sizeof table) {
                return "a malformed table header";
            }
            if (!first_line && !prev_blank) {
                return "a table header without a blank line before it";
            }
            memcpy(table, line + 1, n - 2u);
            table[n - 2u] = '\0';
            have_table = 1;
            prev_blank = 0;
        } else {
            char *eq = strstr(line, " = ");
            size_t tl = strlen(table);
            if (eq == NULL || !have_table || prev_blank) {
                return "a key line outside a table or after a blank line";
            }
            *eq = '\0';
            if (*nrows == REC_MAX_ROWS || tl + 1u + strlen(line) >= KEY_MAX) {
                return "too many rows or a key too long";
            }
            memcpy(rows[*nrows].key, table, tl);
            rows[*nrows].key[tl] = '.';
            memcpy(rows[*nrows].key + tl + 1u, line, strlen(line) + 1u);
            rows[*nrows].val = eq + 3;
            (*nrows)++;
            prev_blank = 0;
        }
        first_line = 0;
        line = nl + 1;
    }
    if (prev_blank) {
        return "a blank line at the end";
    }
    return NULL;
}

/* is the raw value text the string want: a quoted string without its quotes, or a bare integer */
static int value_is(const char *raw, const char *want)
{
    size_t n = strlen(raw);

    if (n >= 2u && raw[0] == '"' && raw[n - 1u] == '"') {
        return strlen(want) == n - 2u && memcmp(raw + 1, want, n - 2u) == 0;
    }
    return strcmp(raw, want) == 0;
}

const struct recrow *find_row(const struct recrow *rows, size_t n, const char *key)
{
    size_t i;

    for (i = 0u; i < n; i++) {
        if (strcmp(rows[i].key, key) == 0) {
            return &rows[i];
        }
    }
    return NULL;
}

/* does the printer refuse (error status, empty text, zero length) the instance copy as it stands; garbage is pre-filled */
static int refuses_with(const struct ds_instance *t, const struct ds_text_run *run, enum ds_text_status want, char *buf, size_t cap)
{
    size_t len = 12345u;
    enum ds_text_status st;

    memset(buf, 0x7f, cap);
    st = ds_text_record(t, run, buf, cap, &len);
    return st == want && len == 0u && buf[0] == '\0';
}

/* the TABLE.KEY=VALUE items of an expect, separated by semicolons: each names a row of the record and its value (the literals come from the case, never from the printer) */
static void check_items(struct ctx *c, const struct recrow *rows, size_t nrows, const char *expect)
{
    char want[256];
    const char *p;

    for (p = expect; !c->harness && *p != '\0';) {
        const char *end = strchr(p, ';');
        size_t n = end != NULL ? (size_t)(end - p) : strlen(p);
        char *eq;
        const struct recrow *row;

        if (n == 0u || n >= sizeof want) {
            harness_error(c, "an expect item is empty or too long");
            break;
        }
        memcpy(want, p, n);
        want[n] = '\0';
        eq = strchr(want, '=');
        if (eq == NULL) {
            harness_error(c, "an expect item has no =");
            break;
        }
        *eq = '\0';
        row = find_row(rows, nrows, want);
        check(c, row != NULL && value_is(row->val, eq + 1), "a scalar line of the record differs from the case expectation");
        p += n + (end != NULL ? 1u : 0u);
    }
}

/*
 * The [stimulus] table against the case's own irq_stim list (plan 03-23; D-112, T-03-65), used by kind record_lines: events is the number of declared events, event_I holds the I-th as AT,LINE,LEVEL in decimal fields separated by commas (the case's numbers, parsed by the one reader of the
 * runner, printed here from the parsed values and not from the record), no event_N+1 line exists, every line of the table is one of those, and no value holds a colon (a dd:dd shape would be taken for a time by the record lint). A case with no irq_stim key must print events = 0.
 */
static void check_stimulus_rows(struct ctx *c, const struct recrow *rows, size_t nrows, const struct kase *k)
{
    struct ds_irq_event ev[IRQ_STIM_PARSE_MAX];
    unsigned n = 0u;
    int declared = 0;
    unsigned i;
    size_t r;
    uint64_t events = 0u;
    char key[KEY_MAX];
    char want[64];
    const char *err = irq_stim_parse(k, ev, IRQ_STIM_PARSE_MAX, &n, &declared);

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    check(c, find_row(rows, nrows, "stimulus.events") != NULL && parse_u64(find_row(rows, nrows, "stimulus.events")->val, &events) == 0 && events == (uint64_t)n, "the stimulus table does not hold the number of events the case declares");
    for (i = 0u; i < n; i++) {
        const struct recrow *row;

        snprintf(key, sizeof key, "stimulus.event_%u", i + 1u);
        snprintf(want, sizeof want, "%llu,%u,%u", (unsigned long long)ev[i].at, (unsigned)ev[i].line, (unsigned)ev[i].level);
        row = find_row(rows, nrows, key);
        check(c, row != NULL && value_is(row->val, want), "a stimulus event line differs from the event the case declares (AT,LINE,LEVEL in decimal)");
    }
    snprintf(key, sizeof key, "stimulus.event_%u", n + 1u);
    check(c, find_row(rows, nrows, key) == NULL, "the stimulus table holds more event lines than the case declares");
    for (r = 0u; r < nrows; r++) {
        if (strncmp(rows[r].key, "stimulus.", 9u) == 0) {
            check(c, strchr(rows[r].val, ':') == NULL && strchr(rows[r].val, '/') == NULL && strchr(rows[r].val, '\\') == NULL, "a stimulus value holds a colon, a slash or a backslash");
        }
    }
}

/*
 * The [input] table against the case's own input script (plan 03-28; D-117, T-03-79), used by kind record_lines: events is the number of events of the script the key input names (read by the one reader of the runner from the rows that scripts/conformance-flatten --inputs
 * wrote; a case with no input key declares an explicit empty script and must print events = 0), and schedule_sha256 is the sha256 of the canonical encoding written here with snprintf, one line AT,PORT,MASK in decimal and one LF per event, from the parsed values and not from the record:
 * a formatting path other than the printer's. Same hash code as the printer; that code is checked against the host tool by the hash kinds, and the case also pins a literal computed with the host sha256 tool. The table prints no colon, slash or backslash.
 */
static void check_input_rows(struct ctx *c, const struct recrow *rows, size_t nrows, const struct kase *k)
{
    struct ds_input_event ev[INPUT_PARSE_MAX];
    unsigned n = 0u;
    int declared = 0;
    unsigned i;
    size_t r;
    uint64_t events = 0u;
    struct ds_sha256 h;
    uint8_t dig[DS_SHA256_BYTES];
    char hex[2u * DS_SHA256_BYTES + 3u];
    const struct recrow *ri;
    const char *err = input_events(k, ev, INPUT_PARSE_MAX, &n, &declared);

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    check(c, find_row(rows, nrows, "input.events") != NULL && parse_u64(find_row(rows, nrows, "input.events")->val, &events) == 0 && events == (uint64_t)n, "the input table does not hold the number of events of the script the case names");
    ds_sha256_init(&h);
    for (i = 0u; i < n; i++) {
        char line[64];
        int len = snprintf(line, sizeof line, "%llu,%u,%u\n", (unsigned long long)ev[i].at, (unsigned)ev[i].port, (unsigned)ev[i].mask);

        ds_sha256_update(&h, (const uint8_t *)line, (size_t)len);
    }
    ds_sha256_final(&h, dig);
    for (i = 0u; i < DS_SHA256_BYTES; i++) {
        snprintf(hex + 2u * i, 3u, "%02x", (unsigned)dig[i]);
    }
    ri = find_row(rows, nrows, "input.schedule_sha256");
    check(c, ri != NULL && value_is(ri->val, hex), "the input identity is not the sha256 of the canonical encoding of the script's events");
    for (r = 0u; r < nrows; r++) {
        if (strncmp(rows[r].key, "input.", 6u) == 0) {
            check(c, strchr(rows[r].val, ':') == NULL && strchr(rows[r].val, '/') == NULL && strchr(rows[r].val, '\\') == NULL, "an input value holds a colon, a slash or a backslash");
        }
    }
}

/* the [withheld] table against the list rebuilt from the defs and the request (plan 03-29): count is the number of ids, and the rows id_1 to id_N hold exactly those ids in sorted order, so a dropped, extra, duplicated or misordered id is noticed */
static void check_withheld_rows(struct ctx *c, const struct recrow *rows, size_t nrows, unsigned nids)
{
    const struct recrow *rc = find_row(rows, nrows, "withheld.count");
    unsigned i;
    char key[KEY_MAX];

    check(c, rc != NULL && strtoul(rc->val, NULL, 10) == (unsigned long)nids, "withheld.count is not the number of ledger ids that the defs and the request give");
    for (i = 1u; i <= nids; i++) {
        const struct recrow *ri;

        snprintf(key, sizeof key, "withheld.id_%u", i);
        ri = find_row(rows, nrows, key);
        check(c, ri != NULL && value_is(ri->val, wh_exp[i - 1u]), "a withheld id row differs from the sorted list of ledger ids of the defs and the request");
    }
}

/*
 * kind record_lines: boot the request (an accepted one), print the record into a buffer (nothing goes to stdout, D-80) and require
 *   the exact table and key order of the skeleton, LF and printable ASCII only, one blank line between tables and nowhere else,
 *   the scalar lines listed in the case expect (TABLE.KEY=VALUE separated by semicolons), the status, value and binder of each of
 *   the 13 field tables from the built-in profile and the request, the profile text as an exact part of the record, and, as
 *   controls on the printer, the refusal of a double quote, a backslash, a non-ASCII byte, an unknown enumeration and a buffer
 *   that is one byte short.
 */
void kind_record_lines(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    static char work[REC_CAP];
    static char ptext[REC_CAP];
    static char again[REC_CAP];
    static char scratch[REC_CAP];
    struct recrow rows[REC_MAX_ROWS];
    struct booted b;
    struct spec s;
    struct ds_text_run run;
    const struct ds_instance *inst = NULL;
    const struct ds_profile *builtin;
    struct ds_instance *tmp;
    size_t len = 0u;
    size_t plen = 0u;
    size_t nrows = 0u;
    size_t i;
    uint32_t f;
    unsigned nids;
    const char *err;
    enum ds_text_status st;

    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    {
        struct ds_irq_event ev[IRQ_STIM_PARSE_MAX];
        unsigned nev = 0u;
        int declared = 0;
        uint32_t stim_refusal = 0u;

        err = irq_stim_parse(k, ev, IRQ_STIM_PARSE_MAX, &nev, &declared);
        if (err == NULL) {
            err = apply_irq_stim((struct ds_instance *)(void *)b.mem, k, &stim_refusal); /* the case's declared stimulus, before the first step: the record prints it (plan 03-23) */
        }
        if (err != NULL) {
            harness_error(c, err);
            release_boot(&b);
            return;
        }
        exp_events = nev;
    }
    {
        uint32_t input_refusal = 0u;

        err = apply_input((struct ds_instance *)(void *)b.mem, k, &input_refusal); /* the case's declared input script, before the first step: the record names it (plan 03-28) */
        if (err == NULL && input_refusal != 0u) {
            err = "ds_input_push refused the input script of a record_lines case";
        }
        if (err != NULL) {
            harness_error(c, err);
            release_boot(&b);
            return;
        }
    }
    builtin = ds_profile_builtin(inst->profile.id, inst->profile.rev);
    check(c, builtin != NULL, "the booted profile is not the built-in r1");
    if (builtin == NULL) {
        release_boot(&b);
        return;
    }
    run = run_of(&s, NULL);
    st = ds_text_record(inst, &run, text, sizeof text, &len);
    check(c, st == DS_TEXT_OK, "ds_text_record failed on an accepted instance");
    if (st != DS_TEXT_OK) {
        release_boot(&b);
        return;
    }
    check(c, len == strlen(text) && len > 0u, "outlen differs from the length of the text");
    for (i = 0u; i < len; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (!(ch == 0x0au || (ch >= 0x20u && ch <= 0x7eu))) {
            break;
        }
    }
    check(c, i == len, "the record holds a byte that is neither LF nor printable ASCII (CR, TAB and non-ASCII are refused)");
    check(c, strchr(text, '\\') == NULL, "the record holds a backslash");

    memcpy(work, text, len + 1u);
    err = parse_record(work, rows, &nrows);
    check(c, err == NULL, err != NULL ? err : "layout");
    nids = withheld_expected(&s);
    check(c, nids > 0u, "the expected list of withheld ledger ids is empty or does not fit");
    build_expected_keys(nids);
    /* 61 rows before plan 03-18 (format_version, 2 core, 3 profile, 39 field, 1 withheld, 6 content, 4 power_on, 1 limit, 4 machine), plus the 4 rows of the run table, the 1 row of the stimulus table of a record with no event (plan 03-23), the 2 rows of the input table (plan 03-28) and, from the defs, 4 rows per model parameter and 1 per constant */
    check(c, n_exp == 60u + 1u + nids + 4u + 1u + 2u + exp_events + 4u * (unsigned)DS_MODEL_COUNT + (unsigned)CONST_ROWS, "the expected record does not hold 68 rows plus 1 per withheld ledger id, 1 per declared stimulus event, 4 per model parameter and 1 per constant");
    check(c, err == NULL && nrows == n_exp, "the number of rows differs from the skeleton");
    for (i = 0u; err == NULL && i < nrows && i < n_exp; i++) {
        if (strcmp(rows[i].key, exp_key[i]) != 0) {
            break;
        }
    }
    check(c, err == NULL && i == n_exp, "a row is missing, extra or out of order against the skeleton");

    /* the scalar lines of the case */
    if (err == NULL) {
        check_items(c, rows, nrows, k->val[K_EXPECT]);
        check_stimulus_rows(c, rows, nrows, k);
        check_input_rows(c, rows, nrows, k);
        check_withheld_rows(c, rows, nrows, nids);
    }

    /* every field table: status, value and binder follow from the built-in profile and the request */
    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT && nrows == n_exp; f++) {
        char key[KEY_MAX];
        const char *name = ds_field_name(f);
        int open = builtin->field[f].status == (uint8_t)DS_STATUS_OPEN;
        const struct recrow *rs;
        const struct recrow *rv;
        const struct recrow *rb;

        snprintf(key, sizeof key, "field_%s.status", name);
        rs = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "field_%s.value", name);
        rv = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "field_%s.bound_by", name);
        rb = find_row(rows, nrows, key);
        check(c, rs != NULL && value_is(rs->val, open ? "open" : "owner-selected"), "a field status line differs");
        check(c, rv != NULL && value_is(rv->val, open ? (s.bind[f] != NULL ? s.bind[f] : "") : builtin->field[f].value),
              "a field value line differs from the constant or the bound value");
        check(c, rb != NULL && value_is(rb->val, open ? "run-request" : "constant"), "a field bound_by line differs");
    }

    /* every model table: the value is the request's own when it named model.NAME, else the first allowed value; the binder follows; ledger and status are the def's own words */
    for (f = 0u; f < (uint32_t)DS_MODEL_COUNT && nrows == n_exp; f++) {
        char key[KEY_MAX];
        const char *name = ds_model_name(f);
        const char *want_value = ds_model_default(f);
        int bound = 0;
        unsigned m;
        const struct recrow *rv;
        const struct recrow *rl;
        const struct recrow *rs;
        const struct recrow *rb;

        for (m = 0u; m < s.nmodel; m++) {
            if (strcmp(s.model_name[m], name) == 0) {
                want_value = s.model_value[m];
                bound = 1;
            }
        }
        snprintf(key, sizeof key, "model_%s.value", name);
        rv = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "model_%s.ledger", name);
        rl = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "model_%s.status", name);
        rs = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "model_%s.bound_by", name);
        rb = find_row(rows, nrows, key);
        check(c, rv != NULL && value_is(rv->val, want_value), "a model value line differs from the request or the default");
        check(c, rl != NULL && value_is(rl->val, ds_model_ledger(f)), "a model ledger line differs from the row of model_params.def");
        check(c, rs != NULL && value_is(rs->val, ds_model_status(f)), "a model status line differs from the row of model_params.def");
        check(c, rb != NULL && value_is(rb->val, bound ? "run-request" : "default"), "a model bound_by line differs from the request");
        check(c, withheld_has(rows, nrows, ds_model_ledger(f)), "the ledger id of a model row is not in the withheld list");
    }

    /* the profile text is an exact part of the record, followed by the content table */
    st = ds_text_profile(&inst->profile, ptext, sizeof ptext, &plen);
    check(c, st == DS_TEXT_OK && plen == strlen(ptext), "ds_text_profile failed");
    if (st == DS_TEXT_OK) {
        const char *at = strstr(text, ptext);
        check(c, at != NULL && at >= text + 2 && at[-1] == '\n' && at[-2] == '\n' && strncmp(at + plen, "\n[withheld]\n", 12u) == 0,
              "the profile text is not an exact part of the record, after a blank line and followed by the withheld table");
        check(c, strncmp(ptext, "[profile]\n", 10u) == 0, "the profile text does not start with the profile table");
    }

    /* printing twice gives the same bytes */
    st = ds_text_record(inst, &run, again, sizeof again, &plen);
    check(c, st == DS_TEXT_OK && plen == len && memcmp(again, text, len + 1u) == 0, "a second print differs from the first");

    /* the capacity edge: len + 1 bytes fit, len bytes do not, and the failed print leaves nothing */
    st = ds_text_record(inst, &run, scratch, len + 1u, &plen);
    check(c, st == DS_TEXT_OK && plen == len && memcmp(scratch, text, len + 1u) == 0, "a buffer of exactly len + 1 bytes did not fit the record");
    check(c, refuses_with(inst, &run, DS_TEXT_ERR_CAPACITY, scratch, len), "a buffer one byte short did not give capacity and an empty text");
    plen = 99u;
    check(c, ds_text_record(inst, &run, NULL, 0u, &plen) == DS_TEXT_ERR_ARGUMENT && plen == 0u, "a NULL buffer is not an argument error");

    /* D-69: a quote, a backslash or a byte outside printable ASCII is refused, never escaped */
    tmp = (struct ds_instance *)malloc(sizeof *tmp);
    if (tmp == NULL) {
        harness_error(c, "out of memory for the instance copy");
    } else {
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_media_slot].value[0] = '"';
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a double quote in a field value was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_lines_per_frame].value[0] = '\\';
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a backslash in a field value was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->mapper[1] = '"';
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a double quote in the mapper name was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.id[2] = '\\';
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a backslash in the profile id was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_vpc].value[0] = (char)0x80;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a non-ASCII byte in a field value was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        memset(tmp->mapper, 'x', sizeof tmp->mapper);
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "an unterminated mapper name was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_chip_revision].status = 9u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an unknown field status was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->byte_order = 7u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an unknown byte order was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->tma_zero = 9u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a model selection outside its allowed list was not refused");
        /* the stimulus slots: a list that ds_irq_stim_set could not have produced is a state error, never printed (plan 03-23); each control starts from an instance with no stimulus */
        memcpy(tmp, inst, sizeof *tmp);
        memset(tmp->irq_stim_at, 0, sizeof tmp->irq_stim_at);
        memset(tmp->irq_stim_line, 0, sizeof tmp->irq_stim_line);
        memset(tmp->irq_stim_level, 0, sizeof tmp->irq_stim_level);
        check(c, !refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an instance with no stimulus was refused");
        tmp->irq_stim_line[0] = 3u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a stimulus line above 2 was not refused");
        tmp->irq_stim_line[0] = 1u;
        tmp->irq_stim_level[0] = 2u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a stimulus level above 1 was not refused");
        tmp->irq_stim_level[0] = 0u;
        tmp->irq_stim_line[2] = 1u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a used stimulus slot after an unused one was not refused");
        tmp->irq_stim_line[2] = 0u;
        tmp->irq_stim_line[1] = 2u;
        tmp->irq_stim_at[0] = UINT64_C(9);
        tmp->irq_stim_at[1] = UINT64_C(8);
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "stimulus times that go back were not refused");
        tmp->irq_stim_line[0] = 0u;
        tmp->irq_stim_line[1] = 0u;
        tmp->irq_stim_at[0] = UINT64_C(0);
        tmp->irq_stim_at[1] = UINT64_C(0);
        tmp->irq_stim_at[5] = UINT64_C(1);
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a nonzero time in an unused stimulus slot was not refused");
        /* the input slots (plan 03-28): a schedule that ds_input_push could not have produced is a state error, never printed; each control starts from a valid two-event schedule */
        memcpy(tmp, inst, sizeof *tmp);
        memset(tmp->input_ev, 0, sizeof tmp->input_ev);
        tmp->input_count = 2u;
        tmp->input_ev[0].at = UINT64_C(10);
        tmp->input_ev[0].port = 1u;
        tmp->input_ev[0].mask = 0x08u;
        tmp->input_ev[1].at = UINT64_C(10);
        tmp->input_ev[1].port = 2u;
        tmp->input_ev[1].mask = 0x00u;
        check(c, !refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a valid input schedule was refused");
        tmp->input_ev[1].port = 1u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an equal input time with a port that does not increase was not refused");
        tmp->input_ev[1].port = 2u;
        tmp->input_ev[1].at = UINT64_C(9);
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "input times that go back were not refused");
        tmp->input_ev[1].at = UINT64_C(11);
        tmp->input_ev[1].port = 6u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an input port above 5 was not refused");
        tmp->input_ev[1].port = 0u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an input port of 0 in a used slot was not refused");
        tmp->input_ev[1].port = 2u;
        tmp->input_ev[1].mask = 0x100u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an input mask with a bit above bit 7 was not refused");
        tmp->input_ev[1].mask = 0x50u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "UP and DOWN held together were not refused");
        tmp->input_ev[1].mask = 0xa0u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "LEFT and RIGHT held together were not refused");
        tmp->input_ev[1].mask = 0x00u;
        tmp->input_ev[5].at = UINT64_C(1);
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a nonzero time in an unused input slot was not refused");
        tmp->input_ev[5].at = UINT64_C(0);
        tmp->input_reserved = 1u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "a nonzero reserved word of the input schedule was not refused");
        tmp->input_reserved = 0u;
        tmp->input_count = (uint32_t)DS_INPUT_MAX + 1u;
        check(c, refuses_with(tmp, &run, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an input count above the cap was not refused");
        free(tmp);
    }
    snprintf(got, gotcap, "rows=%u", (unsigned)nrows);
    record_emit(k, c, text, len); /* --records: the record of this case, text only */
    release_boot(&b);
}

/* ---- kinds record_run and record_constants (plan 03-18; D-53 amended, D-95, D-113) ---- */

/* print the record of inst with the run description into text, copy it to work and split it into rows; 0 on success, else a harness fault has been raised */
static int print_rows(struct ctx *c, const struct ds_instance *inst, const struct ds_text_run *tr, char *text, char *work, struct recrow *rows, size_t *nrows)
{
    size_t len = 0u;
    const char *err;

    if (ds_text_record(inst, tr, text, REC_CAP, &len) != DS_TEXT_OK || len != strlen(text)) {
        harness_error(c, "ds_text_record failed on an accepted instance");
        return -1;
    }
    memcpy(work, text, len + 1u);
    err = parse_record(work, rows, nrows);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    return 0;
}

/* the decimal value of a row of the record, 0 on success */
static int row_u64(const struct recrow *rows, size_t n, const char *key, uint64_t *out)
{
    const struct recrow *r = find_row(rows, n, key);

    return r != NULL && parse_u64(r->val, out) == 0 ? 0 : -1;
}

static int stop_at_once(const struct ds_instance *inst, void *ctx)
{
    (void)inst;
    (void)ctx;
    return 1;
}

/*
 * kind record_run: boot the built authored image NAME (fixture = authored:NAME, request as given), run it with ds_run to the absolute deadline of the key until_mclk and print the record with the run result. expect is TABLE.KEY=VALUE items
 * (run.reason=until;run.stop_point=instruction), the literals of the case. Structural assertions that name no DuoSpool-derived number: [run] steps is above 0, overshoot is below ds_max_step_mclk() and is the machine time past the deadline,
 * machine.time is at or past the deadline. The same instance is then printed with no run named (reason none, 0, none, 0), after a run that a stop callback ended at once (callback, 0 steps) and after a refused run (refused): the table
 * follows the result it is given and nothing else. Evidence tier E0: the record repeats what the run reported; it says nothing about the machine. No file is written (D-86).
 */
void kind_record_run(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    static char work[REC_CAP];
    struct recrow rows[REC_MAX_ROWS];
    struct booted b;
    struct spec s;
    struct ds_text_run tr;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_run_result res2;
    const struct ds_instance *inst = NULL;
    struct ds_instance *w;
    char rom[64];
    uint64_t until = 0u;
    uint64_t steps = 0u;
    uint64_t over = 0u;
    uint64_t when = 0u;
    size_t nrows = 0u;
    const char *err;

    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0) {
        harness_error(c, "a record_run case names its ROM as fixture = authored:NAME");
        return;
    }
    if (!k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "a record_run case needs the integer key until_mclk above 0");
        return;
    }
    err = parse_spec(&s, k->val[K_REQUEST], 262144u);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    err = boot_built_rom(rom, k->val[K_REQUEST], &b, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    w = (struct ds_instance *)(void *)b.mem;
    {
        uint32_t input_refusal = 0u;

        err = apply_input(w, k, &input_refusal); /* the case's declared input script, before the first step: the record names it (plan 03-29; D-117) */
        if (err == NULL && input_refusal != 0u) {
            err = "ds_input_push refused the input script of a record_run case";
        }
        if (err != NULL) {
            release_boot(&b);
            harness_error(c, err);
            return;
        }
    }
    memset(&rq, 0, sizeof rq);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    memset(&res, 0xff, sizeof res);
    check(c, ds_run(w, &rq, &res) == (uint32_t)DS_RUN_REASON_UNTIL && res.reason == (uint32_t)DS_RUN_REASON_UNTIL, "the run to the deadline did not end with the until reason");
    note_run(c, w->machine_time, res.reason);
    tr = run_of(&s, &res);
    if (print_rows(c, inst, &tr, text, work, rows, &nrows) == 0) {
        check_items(c, rows, nrows, k->val[K_EXPECT]);
        check(c, row_u64(rows, nrows, "run.steps", &steps) == 0 && steps > 0u, "the run table shows no step after a run to a deadline");
        check(c, row_u64(rows, nrows, "run.overshoot", &over) == 0 && over < ds_max_step_mclk(), "the run table shows an overshoot that is not below ds_max_step_mclk");
        check(c, row_u64(rows, nrows, "machine.time", &when) == 0 && when >= until, "the machine time of the record is below the deadline");
        check(c, when >= until && over == when - until, "the overshoot of the record is not the machine time past the deadline");
        check(c, steps == res.steps && over == res.overshoot && when == res.machine_time, "the run table differs from the result the run returned");
        snprintf(got, gotcap, "steps=%llu", (unsigned long long)steps);
    }
    /* no run named: the table says so, whatever the machine did before */
    tr = run_of(&s, NULL);
    if (print_rows(c, inst, &tr, text, work, rows, &nrows) == 0) {
        check_items(c, rows, nrows, "run.reason=none;run.overshoot=0;run.stop_point=none;run.steps=0");
    }
    /* a stop callback that holds before the first step: callback, no step, no overshoot */
    rq.until_mclk = w->machine_time + 1000000u;
    rq.stop = stop_at_once;
    memset(&res2, 0xff, sizeof res2);
    check(c, ds_run(w, &rq, &res2) == (uint32_t)DS_RUN_REASON_CALLBACK && res2.steps == 0u, "a stop callback that holds at once did not end the run with the callback reason");
    tr = run_of(&s, &res2);
    if (print_rows(c, inst, &tr, text, work, rows, &nrows) == 0) {
        check_items(c, rows, nrows, "run.reason=callback;run.overshoot=0;run.stop_point=instruction;run.steps=0");
    }
    /* a refused run: the reason says so */
    rq.stop = NULL;
    rq.struct_size = 0u;
    memset(&res2, 0xff, sizeof res2);
    check(c, ds_run(w, &rq, &res2) == (uint32_t)DS_RUN_REASON_REFUSED, "a run request with a wrong struct_size was not refused");
    tr = run_of(&s, &res2);
    if (print_rows(c, inst, &tr, text, work, rows, &nrows) == 0) {
        check_items(c, rows, nrows, "run.reason=refused;run.steps=0");
    }
    /* a result that holds a reason or a stop point outside the sets is a state error, never a guess */
    res2.reason = 9u;
    tr = run_of(&s, &res2);
    check(c, ds_text_record(inst, &tr, text, REC_CAP, &nrows) == DS_TEXT_ERR_STATE && nrows == 0u && text[0] == '\0', "a run reason outside the set was printed");
    release_boot(&b);
}

/* one row of constants.def read as text: NAME, VALUE and LEDGER as the strings between the separators */
struct crow {
    char name[48];
    char value[16];
    char ledger[16];
};
#define CROWS_MAX 40u

/* copy the field starting at p and ending at the first stop character into out (cap bytes, terminated); the end pointer, or NULL when it does not fit or no stop character follows */
static const char *take_field(const char *p, char stop, char *out, size_t cap)
{
    size_t n = 0u;

    while (p[n] != '\0' && p[n] != stop && p[n] != '\n') {
        n++;
    }
    if (p[n] != stop || n == 0u || n >= cap) {
        return NULL;
    }
    memcpy(out, p, n);
    out[n] = '\0';
    return p + n;
}

/* the rows of constants.def: every line that starts in column 0 with DS_CONST( is NAME, VALUE, "LEDGER"); returns the count, or -1 when a row is malformed or there are too many */
static int parse_constants_def(const char *text, struct crow *rows, unsigned max)
{
    const char *p = text;
    unsigned n = 0u;

    while (*p != '\0') {
        const char *nl = strchr(p, '\n');

        if (strncmp(p, "DS_CONST(", 9u) == 0) {
            const char *q;

            if (n == max) {
                return -1;
            }
            q = take_field(p + 9, ',', rows[n].name, sizeof rows[n].name);
            if (q == NULL || strncmp(q, ", ", 2u) != 0) {
                return -1;
            }
            q = take_field(q + 2, ',', rows[n].value, sizeof rows[n].value);
            if (q == NULL || strncmp(q, ", \"", 3u) != 0 || strspn(rows[n].value, "0123456789") != strlen(rows[n].value)) {
                return -1;
            }
            q = take_field(q + 3, '"', rows[n].ledger, sizeof rows[n].ledger);
            if (q == NULL || q[1] != ')') { /* q is at the closing quote */
                return -1;
            }
            n++;
        }
        if (nl == NULL) {
            break;
        }
        p = nl + 1;
    }
    return (int)n;
}

/* do the lines of the [constants] table equal the rows, in order and in full: one NAME = "VALUE LEDGER" line per row and no other line */
static int constants_match(const struct recrow *rows, size_t nrows, const struct crow *def, unsigned ndef)
{
    unsigned seen = 0u;
    size_t i;

    for (i = 0u; i < nrows; i++) {
        if (strncmp(rows[i].key, "constants.", 10u) != 0) {
            continue;
        }
        if (seen >= ndef) {
            return 0;
        }
        {
            char want_key[KEY_MAX];
            char want_val[48];

            snprintf(want_key, sizeof want_key, "constants.%s", def[seen].name);
            snprintf(want_val, sizeof want_val, "%s %s", def[seen].value, def[seen].ledger);
            if (strcmp(rows[i].key, want_key) != 0 || !value_is(rows[i].val, want_val)) {
                return 0;
            }
        }
        seen++;
    }
    return seen == ndef;
}

/* the value of the named row, 0 when absent (a harness fault of the case follows from a missing name) */
static const struct crow *crow_named(const struct crow *def, unsigned ndef, const char *name)
{
    unsigned i;

    for (i = 0u; i < ndef; i++) {
        if (strcmp(def[i].name, name) == 0) {
            return &def[i];
        }
    }
    return NULL;
}

/*
 * kind record_constants: the [constants] table of the record of the accepted request (fixture a synthetic recipe, request as given) equals src/core/constants.def read as text: the same names in the same order, each as "VALUE LEDGER", and every
 * ledger id is in the [withheld] list (so are the ids of the model rows). Controls: a def with a changed ledger id, a changed value, a dropped row and an added row each fail the comparison (a comparison that cannot fail proves nothing). The values in use
 * are checked against the def as well: the header macros DS_MCLK_PER_CYCLE_LOW and _HIGH, DS_UNMAPPED_READ and DS_ADDER_VDC_ACCESS, the machine time of a fresh boot (the reset cost) and the bytes and cycles of every undefined opcode row of cpu_ops.def.
 * expect names the def path and is not otherwise read. Evidence tier E0: the printed value is the value in use; whether the value is right is the ledger's question. No file is written (D-86).
 */
void kind_record_constants(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    static char work[REC_CAP];
    struct recrow rows[REC_MAX_ROWS];
    struct crow def[CROWS_MAX];
    struct crow mut[CROWS_MAX];
    struct booted b;
    struct spec s;
    struct ds_text_run tr;
    const struct ds_instance *inst = NULL;
    const struct recrow *wh;
    const struct crow *cr;
    char *defs;
    size_t nrows = 0u;
    int ndef;
    unsigned n;
    unsigned i;
    unsigned undefined = 0u;
    uint32_t op;
    const char *err;

    if (strcmp(k->val[K_EXPECT], "src/core/constants.def") != 0) {
        harness_error(c, "the expect of a record_constants case is the path src/core/constants.def");
        return;
    }
    defs = read_file(k->val[K_EXPECT]);
    if (defs == NULL) {
        harness_error(c, "src/core/constants.def cannot be read");
        return;
    }
    ndef = parse_constants_def(defs, def, CROWS_MAX);
    free(defs);
    if (ndef < 1) {
        harness_error(c, "src/core/constants.def holds no row or a row that is not DS_CONST(name, number, \"LEDGER\")");
        return;
    }
    n = (unsigned)ndef;
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    tr = run_of(&s, NULL);
    if (print_rows(c, inst, &tr, text, work, rows, &nrows) == 0) {
        check(c, constants_match(rows, nrows, def, n), "the [constants] table differs from src/core/constants.def");
        wh = find_row(rows, nrows, "withheld.count");
        check(c, wh != NULL, "the record has no withheld list");
        for (i = 0u; wh != NULL && i < n; i++) {
            check(c, withheld_has(rows, nrows, def[i].ledger), "the ledger id of a constants row is not in the withheld list");
        }
        for (i = 0u; wh != NULL && i < (unsigned)DS_MODEL_COUNT; i++) {
            check(c, withheld_has(rows, nrows, ds_model_ledger(i)), "the ledger id of a model row is not in the withheld list");
        }
        /* the controls: the comparison fails for each kind of drift */
        memcpy(mut, def, n * sizeof def[0]);
        mut[0].ledger[strlen(mut[0].ledger) - 1u] = mut[0].ledger[strlen(mut[0].ledger) - 1u] == '9' ? '8' : '9';
        check(c, !constants_match(rows, nrows, mut, n), "a changed ledger id in constants.def was not noticed");
        memcpy(mut, def, n * sizeof def[0]);
        mut[n - 1u].value[0] = mut[n - 1u].value[0] == '7' ? '6' : '7';
        check(c, !constants_match(rows, nrows, mut, n), "a changed value in constants.def was not noticed");
        check(c, !constants_match(rows, nrows, def, n - 1u), "a dropped row of constants.def was not noticed");
        memcpy(mut, def, n * sizeof def[0]);
        memcpy(&mut[n], &def[0], sizeof def[0]);
        check(c, n + 1u <= CROWS_MAX && !constants_match(rows, nrows, mut, n + 1u), "an added row of constants.def was not noticed");
    }
    /* the values in use are the values of the def */
    cr = crow_named(def, n, "cycle_mclk_low");
    check(c, cr != NULL && strtoul(cr->value, NULL, 10) == (unsigned long)DS_MCLK_PER_CYCLE_LOW, "cycle_mclk_low is not the value DS_MCLK_PER_CYCLE_LOW has");
    cr = crow_named(def, n, "cycle_mclk_high");
    check(c, cr != NULL && strtoul(cr->value, NULL, 10) == (unsigned long)DS_MCLK_PER_CYCLE_HIGH, "cycle_mclk_high is not the value DS_MCLK_PER_CYCLE_HIGH has");
    cr = crow_named(def, n, "unmapped_read");
    check(c, cr != NULL && strtoul(cr->value, NULL, 10) == (unsigned long)DS_UNMAPPED_READ, "unmapped_read is not the value DS_UNMAPPED_READ has");
    cr = crow_named(def, n, "vdc_vce_penalty_cycles");
    check(c, cr != NULL && strtoul(cr->value, NULL, 10) == (unsigned long)DS_ADDER_VDC_ACCESS, "vdc_vce_penalty_cycles is not the value DS_ADDER_VDC_ACCESS has");
    cr = crow_named(def, n, "reset_cost_mclk");
    check(c, cr != NULL && strtoull(cr->value, NULL, 10) == (unsigned long long)inst->machine_time, "reset_cost_mclk is not the machine time of a fresh boot");
    {
        const struct crow *nb = crow_named(def, n, "undefined_opcode_bytes");
        const struct crow *nc = crow_named(def, n, "undefined_opcode_cycles");

        check(c, nb != NULL && nc != NULL, "constants.def has no undefined_opcode_bytes or undefined_opcode_cycles row");
        for (op = 0u; nb != NULL && nc != NULL && op < 256u; op++) {
            uint32_t bytes = 0u;
            uint32_t cycles = 0u;

            if (ds_cpu_op_defined(op)) {
                continue;
            }
            ds_op_meta(op, &bytes, &cycles);
            undefined++;
            check(c, bytes == (uint32_t)strtoul(nb->value, NULL, 10) && cycles == (uint32_t)strtoul(nc->value, NULL, 10), "an undefined opcode row of cpu_ops.def differs from the undefined opcode constants");
        }
        check(c, undefined == 22u, "the opcode table does not hold 22 undefined opcodes");
    }
    snprintf(got, gotcap, "rows=%u", n);
    release_boot(&b);
}
