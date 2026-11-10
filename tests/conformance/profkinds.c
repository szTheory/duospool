/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * profkinds.c - the case kinds that compare the built-in r1 profile of the core with the frozen profiles/pce-hucard-2btn/r1.toml:
 * profile_equal, names_both_ways and field_flip (D-48, D-53). Split out of runner.c to keep each test source small. ISO C only.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_text.h"
#include "runner.h"

/* ---- the frozen r1 record, flattened by scripts/conformance-flatten (--profile-tsv) and the field-name list of the profile rules
 * (--rules-fields, one name per line, cut out of scripts/lib/profile-rules.awk by conformance-run). C never parses TOML (D-40,
 * D-51): both files are line formats printed by the sh gate. ---- */

#define MAX_PROWS 512u
#define MAX_RULES 64u

struct prow {
    const char *table; /* empty for a root key */
    const char *key;
    const char *type; /* s, i, b or a */
    char *value;      /* an array has its elements joined with '|' (flatten uses the unit separator) */
};

const char *profile_tsv_path;
const char *rules_fields_path;
static struct prow prow[MAX_PROWS];
static size_t nprow;
static int ptsv_state; /* 0 not read, 1 read, -1 failed */
static const char *ptsv_why;
static const char *rules_name[MAX_RULES];
static size_t nrules;
static int rules_state;
static const char *rules_why;

/* read the flattened r1 once; NULL on success, otherwise why not (a harness fault of the run, never a verdict) */
static const char *load_profile_tsv(void)
{
    char *text;
    char *line;
    int seen_end = 0;
    size_t end_count = 0u;

    if (ptsv_state != 0) {
        return ptsv_state > 0 ? NULL : ptsv_why;
    }
    ptsv_state = -1;
    if (profile_tsv_path == NULL) {
        ptsv_why = "this case needs --profile-tsv";
        return ptsv_why;
    }
    text = read_file(profile_tsv_path);
    if (text == NULL) {
        ptsv_why = "cannot read the profile TSV";
        return ptsv_why;
    }
    for (line = text; *line != '\0';) {
        char *nl = strchr(line, '\n');
        char *f[6];
        int nf;
        if (nl == NULL) {
            ptsv_why = "the profile TSV has a last line without a newline";
            return ptsv_why;
        }
        *nl = '\0';
        if (seen_end) {
            ptsv_why = "a row follows the END row of the profile TSV";
            return ptsv_why;
        }
        nf = split_tab(line, f, 6);
        if (nf == 2 && strcmp(f[0], "END") == 0) {
            if (parse_size(f[1], &end_count) != 0) {
                ptsv_why = "malformed END row in the profile TSV";
                return ptsv_why;
            }
            seen_end = 1;
        } else if (nf == 6 && strcmp(f[0], "K") == 0 && strcmp(f[1], "r1") == 0 && nprow < MAX_PROWS) {
            char *v;
            prow[nprow].table = f[2];
            prow[nprow].key = f[3];
            prow[nprow].type = f[4];
            prow[nprow].value = f[5];
            if (f[4][0] == 'a') {
                for (v = f[5]; *v != '\0'; v++) {
                    if (*v == '\037') {
                        *v = '|';
                    }
                }
            }
            nprow++;
        } else {
            ptsv_why = "an unexpected row in the profile TSV (only K rows of r1 and one END row are expected)";
            return ptsv_why;
        }
        line = nl + 1;
    }
    if (!seen_end || end_count != 1u || nprow == 0u) {
        ptsv_why = "the profile TSV has no END row, an END count other than 1, or no row";
        return ptsv_why;
    }
    ptsv_state = 1;
    return NULL;
}

static const struct prow *ptsv_get(const char *table, const char *key)
{
    size_t i;

    for (i = 0u; i < nprow; i++) {
        if (strcmp(prow[i].table, table) == 0 && strcmp(prow[i].key, key) == 0) {
            return &prow[i];
        }
    }
    return NULL;
}

/* read the field-name list of the profile rules once: one non-empty name per line */
static const char *load_rules_fields(void)
{
    char *text;
    char *line;

    if (rules_state != 0) {
        return rules_state > 0 ? NULL : rules_why;
    }
    rules_state = -1;
    if (rules_fields_path == NULL) {
        rules_why = "this case needs --rules-fields";
        return rules_why;
    }
    text = read_file(rules_fields_path);
    if (text == NULL) {
        rules_why = "cannot read the rules field list";
        return rules_why;
    }
    for (line = text; *line != '\0';) {
        char *nl = strchr(line, '\n');
        if (nl == NULL) {
            rules_why = "the rules field list has a last line without a newline";
            return rules_why;
        }
        *nl = '\0';
        if (*line == '\0' || nrules == MAX_RULES) {
            rules_why = "the rules field list holds an empty line or too many names";
            return rules_why;
        }
        rules_name[nrules++] = line;
        line = nl + 1;
    }
    if (nrules == 0u) {
        rules_why = "the rules field list is empty";
        return rules_why;
    }
    rules_state = 1;
    return NULL;
}

/* is v exactly one of the values of the '|'-separated set */
static int list_has(const char *set, const char *v)
{
    size_t n = strlen(v);

    while (*set != '\0') {
        const char *bar = strchr(set, '|');
        size_t m = bar != NULL ? (size_t)(bar - set) : strlen(set);
        if (m == n && memcmp(set, v, n) == 0) {
            return 1;
        }
        if (bar == NULL) {
            break;
        }
        set = bar + 1;
    }
    return 0;
}

/*
 * Where the frozen r1 states no allowed list, the core's own list must be this one: nothing for vdc_count, vpc and input_port1
 * (owner-selected, never bound), and for powerup_policy the D-67 constant, because the frozen record cannot gain the key.
 */
static const char *core_allowed_without_r1(const char *field)
{
    return strcmp(field, "powerup_policy") == 0 ? "zero|seeded-prng-v1" : "";
}

/*
 * kind profile_equal: the built-in r1 constant against the flattened r1.toml, field by field (D-48, T-02-48). Compared per field:
 * status, the value of an owner-selected field, the allowed list where r1 has one (and the documented core list where it has
 * none), the absence of any value on an open field, and that the proposal of an open field is one of the allowed values. Not
 * compared: label, decision and ledger keys, which the core does not carry. Root keys: profile_id, rev and the
 * unresolved_behaviours ids, which must be exactly the ones the constant lists.
 */
void kind_profile_equal(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    const char *why = load_profile_tsv();
    const struct ds_profile *b;
    const struct prow *r;
    uint32_t f;
    unsigned n_open = 0u;
    unsigned n_owner = 0u;
    unsigned core_open = 0u;

    (void)k;
    if (why != NULL) {
        harness_error(c, why);
        return;
    }
    b = ds_profile_builtin("pce-hucard-2btn", 1u);
    check(c, b != NULL, "the built-in profile pce-hucard-2btn rev 1 is not registered");
    if (b == NULL) {
        return;
    }
    r = ptsv_get("", "profile_id");
    check(c, r != NULL && strcmp(r->value, b->id) == 0, "the profile id differs from r1");
    r = ptsv_get("", "rev");
    check(c, r != NULL && r->type[0] == 'i' && strcmp(r->value, "1") == 0 && b->rev == 1u, "the revision differs from r1");

    /* unresolved_behaviours: every id of r1 is in the constant and the constant has no other */
    r = ptsv_get("", "unresolved_behaviours");
    check(c, r != NULL && r->type[0] == 'a', "r1 has no unresolved_behaviours array");
    if (r != NULL) {
        uint32_t u;
        unsigned n_r1 = 0u;
        const char *p = r->value;
        while (*p != '\0') {
            const char *bar = strchr(p, '|');
            size_t m = bar != NULL ? (size_t)(bar - p) : strlen(p);
            int found = 0;
            for (u = 0u; u < ds_profile_unresolved_count(); u++) {
                const char *id = ds_profile_unresolved(u);
                if (id != NULL && strlen(id) == m && memcmp(id, p, m) == 0) {
                    found = 1;
                }
            }
            check(c, found, "an unresolved behaviour id of r1 is missing from the constant");
            n_r1++;
            if (bar == NULL) {
                break;
            }
            p = bar + 1;
        }
        check(c, n_r1 == ds_profile_unresolved_count(), "the constant lists a different number of unresolved behaviours than r1");
    }

    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
        const char *name = ds_field_name(f);
        const struct prow *st = ptsv_get(name, "status");
        const struct prow *val = ptsv_get(name, "value");
        const struct prow *al = ptsv_get(name, "allowed");
        const struct prow *pr = ptsv_get(name, "proposal");
        const char *core_allowed = ds_field_allowed(f);
        char msg[160];
        int owner;

        snprintf(msg, sizeof msg, "field %s: r1 has no status", name);
        check(c, st != NULL, msg);
        if (st == NULL) {
            continue;
        }
        owner = strcmp(st->value, "owner-selected") == 0;
        if (b->field[f].status == (uint8_t)DS_STATUS_OPEN) {
            core_open++;
        }
        if (owner) {
            n_owner++;
        } else if (strcmp(st->value, "open") == 0) {
            n_open++;
        }
        snprintf(msg, sizeof msg, "field %s: the status differs from r1 (owner-selected or open)", name);
        check(c, owner ? b->field[f].status == (uint8_t)DS_STATUS_OWNER : b->field[f].status == (uint8_t)DS_STATUS_OPEN, msg);
        if (owner) {
            snprintf(msg, sizeof msg, "field %s: the value differs from the owner-selected value of r1", name);
            check(c, val != NULL && strcmp(val->value, b->field[f].value) == 0, msg);
            snprintf(msg, sizeof msg, "field %s: an owner-selected field is not bound_by constant in the constant", name);
            check(c, b->field[f].bound_by == (uint8_t)DS_BOUND_CONSTANT, msg);
        } else {
            snprintf(msg, sizeof msg, "field %s: r1 gives an open field a value, or the constant does", name);
            check(c, val == NULL && b->field[f].value[0] == '\0', msg);
            snprintf(msg, sizeof msg, "field %s: an open field of the constant is already bound", name);
            check(c, b->field[f].bound_by == (uint8_t)DS_BOUND_NONE, msg);
            snprintf(msg, sizeof msg, "field %s: the r1 proposal is not one of the allowed values", name);
            check(c, pr != NULL && list_has(core_allowed, pr->value), msg);
        }
        snprintf(msg, sizeof msg, "field %s: the allowed list differs from r1 (or from the documented core list where r1 has none)",
                 name);
        check(c, strcmp(core_allowed, al != NULL ? al->value : core_allowed_without_r1(name)) == 0, msg);
    }
    check(c, n_owner == 6u && n_open == 7u && core_open == 7u, "r1 and the constant do not both have 6 owner-selected and 7 open fields");
    snprintf(got, gotcap, "fields=%u", (unsigned)DS_FIELD_COUNT);
}

/*
 * kind names_both_ways: the 13 field names of the X-list, the table names of the flattened r1 and the field list of the profile
 * rules are the same set, each checked against each in both directions, and a name appears once.
 */
void kind_names_both_ways(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    const char *why = load_profile_tsv();
    const char *why2 = why != NULL ? why : load_rules_fields();
    uint32_t f;
    size_t i;
    size_t n_tables = 0u;

    (void)k;
    if (why2 != NULL) {
        harness_error(c, why2);
        return;
    }
    check(c, ds_field_name((uint32_t)DS_FIELD_COUNT) == NULL, "the name table has a name past the last field");
    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
        const char *name = ds_field_name(f);
        char msg[160];
        int in_rules = 0;
        snprintf(msg, sizeof msg, "field %s of the X-list is not a table of r1", name);
        check(c, ptsv_get(name, "status") != NULL, msg);
        for (i = 0u; i < nrules; i++) {
            in_rules += strcmp(rules_name[i], name) == 0;
        }
        snprintf(msg, sizeof msg, "field %s of the X-list is not exactly once in the list of scripts/lib/profile-rules.awk", name);
        check(c, in_rules == 1, msg);
    }
    /* the other direction: every table of r1 is an X-list field, every name of the rules is one too */
    for (i = 0u; i < nprow; i++) {
        int dup = 0;
        size_t j;
        if (prow[i].table[0] == '\0') {
            continue;
        }
        for (j = 0u; j < i; j++) {
            dup += strcmp(prow[j].table, prow[i].table) == 0;
        }
        if (dup == 0) {
            char msg[160];
            int known = 0;
            n_tables++;
            for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
                known += strcmp(ds_field_name(f), prow[i].table) == 0;
            }
            snprintf(msg, sizeof msg, "table %s of r1 is not a field of the X-list", prow[i].table);
            check(c, known == 1, msg);
        }
    }
    for (i = 0u; i < nrules; i++) {
        char msg[160];
        int known = 0;
        for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
            known += strcmp(ds_field_name(f), rules_name[i]) == 0;
        }
        snprintf(msg, sizeof msg, "name %s of the profile rules is not a field of the X-list", rules_name[i]);
        check(c, known == 1, msg);
    }
    check(c, n_tables == (size_t)DS_FIELD_COUNT && nrules == (size_t)DS_FIELD_COUNT && DS_FIELD_COUNT == 13,
          "the three field sets do not each hold exactly 13 names");
    snprintf(got, gotcap, "names=%u", (unsigned)DS_FIELD_COUNT);
}

/*
 * kind field_flip: for each of the 13 fields, change the resolved profile (its value, its status, its binder) in a copy and print
 * the profile identity text again: each of the three changes must change the text, or the field is not part of the identity
 * (T-02-48). One assertion per field. Label text is not part of the core, so it cannot be part of this text. The case expect
 * carries fields=N, the number of fields the case was written for; a different number is a fault of the case.
 */
void kind_field_flip(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char t0[REC_CAP];
    static char t1[REC_CAP];
    struct spec s;
    struct ds_bindings bnd;
    struct ds_profile base;
    struct ds_profile work;
    const char *err = parse_spec(&s, k->val[K_REQUEST], 0u);
    const char *at = strstr(k->val[K_EXPECT], "fields=");
    size_t n0 = 0u;
    size_t n1 = 0u;
    uint32_t field = 0u;
    uint32_t f;

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (strcmp(k->val[K_FIXTURE], "none") != 0 || at == NULL || strcmp(at + 7, "13") != 0 || DS_FIELD_COUNT != 13) {
        harness_error(c, "a field_flip case has fixture none and expect fields=13, and the profile has 13 fields");
        return;
    }
    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
        bnd.value[f] = s.bind[f];
    }
    memset(&base, 0, sizeof base);
    if (ds_resolve(s.profile, s.rev, &bnd, &base, &field) != 0u) {
        harness_error(c, "the request of a field_flip case must resolve");
        return;
    }
    if (ds_text_profile(&base, t0, sizeof t0, &n0) != DS_TEXT_OK) {
        harness_error(c, "the resolved profile does not print");
        return;
    }
    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT; f++) {
        char msg[160];
        int ok_value;
        int ok_status;
        int ok_bound;

        work = base;
        work.field[f].value[0] = work.field[f].value[0] == 'x' ? 'y' : 'x';
        ok_value = ds_text_profile(&work, t1, sizeof t1, &n1) == DS_TEXT_OK && strcmp(t0, t1) != 0;
        work = base;
        work.field[f].status = work.field[f].status == (uint8_t)DS_STATUS_OWNER ? (uint8_t)DS_STATUS_OPEN : (uint8_t)DS_STATUS_OWNER;
        ok_status = ds_text_profile(&work, t1, sizeof t1, &n1) == DS_TEXT_OK && strcmp(t0, t1) != 0;
        work = base;
        work.field[f].bound_by = work.field[f].bound_by == (uint8_t)DS_BOUND_CONSTANT ? (uint8_t)DS_BOUND_RUN_REQUEST : (uint8_t)DS_BOUND_CONSTANT;
        ok_bound = ds_text_profile(&work, t1, sizeof t1, &n1) == DS_TEXT_OK && strcmp(t0, t1) != 0;
        snprintf(msg, sizeof msg, "field %s: a change of its value, status or binder leaves the printed identity unchanged", ds_field_name(f));
        check(c, ok_value && ok_status && ok_bound, msg);
    }
    snprintf(got, gotcap, "assertions=%u", (unsigned)DS_FIELD_COUNT);
}

