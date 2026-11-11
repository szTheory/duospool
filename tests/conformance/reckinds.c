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
#include "ds_text.h"
#include "runner.h"
#include "synth.h"

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
    if (ds_text_record(inst, text, sizeof text, &len) != DS_TEXT_OK) {
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
    check(c, ds_text_record(inst, text, sizeof text, &len) == DS_TEXT_OK && len == strlen(text) && len > 0u,
          "ds_text_record failed on an accepted instance");
    check(c, ds_text_record(inst, again, sizeof again, &alen) == DS_TEXT_OK && alen == len && memcmp(again, text, len + 1u) == 0,
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
    check(c, strncmp(text, "[record]\nformat_version = 1\n\n[core]\nbuild_id = \"", 47u) == 0, "the refusal record does not start with the record and core tables");
    check(c, strstr(text, "\nbehaviour_epoch = 1\n") != NULL, "the refusal record has no behaviour_epoch line");
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
