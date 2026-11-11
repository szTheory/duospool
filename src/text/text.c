/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * text.c - the canonical record printer (D-40, D-53, D-69). Printing only: nothing in this file reads text back.
 * No library call is used (no stdio, no string.h): a decimal printer and a bounded appender are all that is needed, and the
 * output is the same on every host. Every narrowing is an explicit cast, because gcc and clang disagree about which implicit
 * conversions -Wconversion reports.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_poweron.h"
#include "ds_serial.h"
#include "ds_text.h"

/* the build id comes from the CMake cache variable DS_BUILD_ID; a build that sets none prints the literal "unset" */
#ifndef DS_BUILD_ID
#define DS_BUILD_ID "unset"
#endif

/* an appender: the first error sticks and every later append becomes a no-op */
struct out {
    char *buf;
    size_t cap;
    size_t len;
    enum ds_text_status err;
};

static void put_ch(struct out *o, char c)
{
    if (o->err != DS_TEXT_OK) {
        return;
    }
    if (o->len + 1u >= o->cap) { /* one byte stays free for the terminator */
        o->err = DS_TEXT_ERR_CAPACITY;
        return;
    }
    o->buf[o->len] = c;
    o->len++;
}

static void put_lit(struct out *o, const char *s)
{
    while (*s != '\0') {
        put_ch(o, *s);
        s++;
    }
}

/* an unsigned decimal of up to 64 bits (20 digits): machine time and the seed are u64 and are printed in full, never truncated */
static void put_u64(struct out *o, uint64_t v)
{
    char tmp[20];
    unsigned n = 0u;

    do {
        tmp[n] = (char)('0' + (unsigned)(v % UINT64_C(10)));
        n++;
        v /= UINT64_C(10);
    } while (v != UINT64_C(0));
    while (n > 0u) {
        n--;
        put_ch(o, tmp[n]);
    }
}

static void put_u32(struct out *o, uint32_t v)
{
    put_u64(o, (uint64_t)v);
}

/*
 * A quoted string value. The source holds at most max bytes including its terminator. Printable ASCII only, and neither a double
 * quote nor a backslash (D-69): a violation is an error and nothing is escaped.
 */
static void put_quoted(struct out *o, const char *s, size_t max)
{
    size_t i;

    if (o->err != DS_TEXT_OK) {
        return;
    }
    if (s == NULL) {
        o->err = DS_TEXT_ERR_ARGUMENT;
        return;
    }
    for (i = 0u; i < max && s[i] != '\0'; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20u || c > 0x7eu || c == (unsigned char)'"' || c == (unsigned char)'\\') {
            o->err = DS_TEXT_ERR_STRING;
            return;
        }
    }
    if (i == max) { /* no terminator inside the field */
        o->err = DS_TEXT_ERR_STRING;
        return;
    }
    put_ch(o, '"');
    put_lit(o, s);
    put_ch(o, '"');
}

static void kv_str(struct out *o, const char *key, const char *value, size_t max)
{
    put_lit(o, key);
    put_lit(o, " = ");
    put_quoted(o, value, max);
    put_ch(o, '\n');
}

static void kv_lit(struct out *o, const char *key, const char *literal)
{
    kv_str(o, key, literal, 64u);
}

static void kv_u32(struct out *o, const char *key, uint32_t value)
{
    put_lit(o, key);
    put_lit(o, " = ");
    put_u32(o, value);
    put_ch(o, '\n');
}

static void kv_u64(struct out *o, const char *key, uint64_t value)
{
    put_lit(o, key);
    put_lit(o, " = ");
    put_u64(o, value);
    put_ch(o, '\n');
}

static void kv_bool(struct out *o, const char *key, int value)
{
    put_lit(o, key);
    put_lit(o, value ? " = true\n" : " = false\n");
}

/* a lowercase hexadecimal string value of n bytes, quoted: 2 * n characters */
static void kv_hex(struct out *o, const char *key, const uint8_t *bytes, size_t n)
{
    static const char digit[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    size_t i;

    put_lit(o, key);
    put_lit(o, " = \"");
    for (i = 0u; i < n; i++) {
        put_ch(o, digit[(bytes[i] >> 4) & 0x0fu]);
        put_ch(o, digit[bytes[i] & 0x0fu]);
    }
    put_lit(o, "\"\n");
}

/* a table header; tables are separated by one blank line, and the first table of a text has none before it */
static void table(struct out *o, const char *prefix, const char *name)
{
    if (o->len > 0u) {
        put_ch(o, '\n');
    }
    put_ch(o, '[');
    put_lit(o, prefix);
    put_lit(o, name);
    put_lit(o, "]\n");
}

static void fail(struct out *o, enum ds_text_status why)
{
    if (o->err == DS_TEXT_OK) {
        o->err = why;
    }
}

/* the named constants (constants.def): name, value and ledger id of every row, in file order; the core reads the same rows as the enumerators DS_CONST_name, so a printed value is the value in use */
struct const_row {
    const char *name;
    uint32_t value;
    const char *ledger;
};

#define DS_CONST(name, value, ledger) {#name, (uint32_t)(value), ledger},
static const struct const_row const_rows[] = {
#include "constants.def"
};
#undef DS_CONST
#define CONST_COUNT ((uint32_t)(sizeof const_rows / sizeof const_rows[0]))

/* the [profile] table and the 13 field tables, key order from the X-table (profile_fields.def) */
static void emit_profile(struct out *o, const struct ds_profile *p)
{
    uint32_t i;
    uint8_t hash[DS_DIGEST_BYTES];

    table(o, "", "profile");
    kv_str(o, "id", p->id, DS_PROFILE_ID_MAX);
    kv_u32(o, "rev", p->rev);
    /* the resolved profile identity: SHA-256 of the canonical profile stream (ds_profile_hash), id, rev and every resolved field */
    ds_profile_hash(p, hash);
    kv_hex(o, "sha256", hash, DS_DIGEST_BYTES);
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        const struct ds_field *f = &p->field[i];
        const char *status = NULL;
        const char *bound = NULL;

        if (f->status == (uint8_t)DS_STATUS_OWNER) {
            status = "owner-selected";
        } else if (f->status == (uint8_t)DS_STATUS_OPEN) {
            status = "open";
        }
        if (f->bound_by == (uint8_t)DS_BOUND_NONE) {
            bound = "none";
        } else if (f->bound_by == (uint8_t)DS_BOUND_CONSTANT) {
            bound = "constant";
        } else if (f->bound_by == (uint8_t)DS_BOUND_RUN_REQUEST) {
            bound = "run-request";
        }
        if (status == NULL || bound == NULL) {
            fail(o, DS_TEXT_ERR_STATE);
            return;
        }
        table(o, "field_", ds_field_name(i));
        kv_lit(o, "status", status);
        kv_str(o, "value", f->value, DS_FIELD_VALUE_MAX);
        kv_lit(o, "bound_by", bound);
    }
}

static void emit_content(struct out *o, const struct ds_instance *inst)
{
    const char *order = NULL;
    const char *header = NULL;

    if (inst->byte_order == 1u) {
        order = "pce";
    } else if (inst->byte_order == 2u) {
        order = "tg16";
    }
    if (inst->header == 1u) {
        header = "none";
    } else if (inst->header == 2u) {
        header = "copier_512";
    }
    if (order == NULL || header == NULL) {
        fail(o, DS_TEXT_ERR_STATE);
        return;
    }
    table(o, "", "content");
    kv_hex(o, "sha256", inst->content_id, DS_CONTENT_ID_BYTES);
    if (ds_size_bytes(inst->size_class) == 0u) {
        fail(o, DS_TEXT_ERR_STATE);
        return;
    }
    kv_u32(o, "size", ds_size_bytes(inst->size_class));
    kv_str(o, "mapper", inst->mapper, DS_MAPPER_NAME_MAX);
    if (o->err == DS_TEXT_OK) {
        /* the mapper rule is named after the mapper: plain is the only mapper of this profile revision */
        if (ds_streq(inst->mapper, "plain")) {
            kv_lit(o, "mapper_rule", "plain-v1");
        } else {
            fail(o, DS_TEXT_ERR_STATE);
            return;
        }
    }
    kv_lit(o, "byte_order", order);
    kv_lit(o, "header_handling", header);
}

/*
 * The withheld ledger ids: the open items that bound this run's evidence. Nothing in the record may read as support for any of them,
 * so the ids are listed in the record itself (D-53), sorted and without duplicates:
 *   - the ledger ids of every field that the run request bound (a run-request binding is no support claim, D-49),
 *   - the unresolved behaviours of the profile (LED-IRQ-0001, LED-VID-0002, LED-VID-0003),
 *   - the load rules that are assumptions: LED-LOAD-0003 (the 384 KiB mirror rule), LED-LOAD-0005 (the bank window and unmapped
 *     reads) and LED-PROF-0002 (the power-on values),
 *   - the ledger id of every row of model_params.def and of every row of constants.def (plan 03-18).
 */
#define WH_MAX 24u    /* a line of the record is limited to 400 characters: the ids line prints each id as 12 characters, two quotes, a comma and a blank, so 24 ids take 7 + 24 * 16 + 1 = 392 */
#define WH_ID_MAX 16u /* an id and its terminator */

struct idset {
    char id[WH_MAX][WH_ID_MAX];
    uint32_t n;
};

/* strcmp without string.h: negative, zero or positive */
static int str_cmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

/* insert the id (len bytes of s) in sorted position unless it is already there; 0 on success, -1 when it does not fit */
static int idset_add(struct idset *set, const char *s, size_t len)
{
    char tmp[WH_ID_MAX];
    uint32_t at;
    uint32_t j;
    size_t k;

    if (len == 0u || len >= WH_ID_MAX) {
        return -1;
    }
    for (k = 0u; k < len; k++) {
        tmp[k] = s[k];
    }
    tmp[len] = '\0';
    for (at = 0u; at < set->n; at++) {
        int c = str_cmp(tmp, set->id[at]);
        if (c == 0) {
            return 0;
        }
        if (c < 0) {
            break;
        }
    }
    if (set->n == WH_MAX) {
        return -1;
    }
    for (j = set->n; j > at; j--) {
        for (k = 0u; k < WH_ID_MAX; k++) {
            set->id[j][k] = set->id[j - 1u][k];
        }
    }
    for (k = 0u; k <= len; k++) {
        set->id[at][k] = tmp[k];
    }
    set->n++;
    return 0;
}

/* add every id of a list separated by '|' (the form of ds_field_ledger); the empty list adds nothing */
static int idset_add_list(struct idset *set, const char *list)
{
    while (*list != '\0') {
        size_t n = 0u;

        while (list[n] != '\0' && list[n] != '|') {
            n++;
        }
        if (idset_add(set, list, n) != 0) {
            return -1;
        }
        list += n;
        if (*list == '|') {
            list++;
        }
    }
    return 0;
}

static void emit_withheld(struct out *o, const struct ds_profile *p)
{
    struct idset set;
    uint32_t i;
    int bad = 0;

    set.n = 0u;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        if (p->field[i].bound_by == (uint8_t)DS_BOUND_RUN_REQUEST) {
            const char *ledger = ds_field_ledger(i);
            bad |= ledger == NULL || idset_add_list(&set, ledger) != 0;
        }
    }
    for (i = 0u; i < ds_profile_unresolved_count(); i++) {
        const char *id = ds_profile_unresolved(i);
        bad |= id == NULL || idset_add_list(&set, id) != 0;
    }
    bad |= idset_add_list(&set, "LED-LOAD-0003|LED-LOAD-0005|LED-PROF-0002") != 0;
    for (i = 0u; i < (uint32_t)DS_MODEL_COUNT; i++) { /* every model row is printed, so its ledger id bounds the run whether the value is the default or was requested (D-113) */
        const char *ledger = ds_model_ledger(i);
        bad |= ledger == NULL || idset_add_list(&set, ledger) != 0;
    }
    for (i = 0u; i < CONST_COUNT; i++) { /* every named constant is in force in every run (D-113) */
        bad |= idset_add_list(&set, const_rows[i].ledger) != 0;
    }
    if (bad) {
        fail(o, DS_TEXT_ERR_STATE);
        return;
    }
    table(o, "", "withheld");
    put_lit(o, "ids = [");
    for (i = 0u; i < set.n; i++) {
        if (i != 0u) {
            put_lit(o, ", ");
        }
        put_quoted(o, set.id[i], WH_ID_MAX);
    }
    put_lit(o, "]\n");
}

/* the power-on policy of the run (D-50): its name, the version of the policy table, and whether a seed was supplied and which */
static void emit_power_on(struct out *o, const struct ds_instance *inst)
{
    const char *policy = ds_policy_name(inst->powerup_policy);

    if (policy == NULL || inst->has_seed > 1u) {
        fail(o, DS_TEXT_ERR_STATE);
        return;
    }
    table(o, "", "power_on");
    kv_lit(o, "policy", policy);
    kv_u32(o, "policy_version", ds_policy_version(inst->powerup_policy));
    kv_bool(o, "seed_present", inst->has_seed == 1u);
    kv_u64(o, "seed", inst->has_seed == 1u ? inst->powerup_seed : UINT64_C(0));
}

/* a model value is a token inside the allowed list and is not NUL-terminated: copy it into a bounded buffer and print it through the checked string path */
static void kv_model_value(struct out *o, const char *key, const char *token, size_t n)
{
    char tmp[32];
    size_t i;

    if (token == NULL || n == 0u || n >= sizeof tmp) {
        fail(o, DS_TEXT_ERR_STATE);
        return;
    }
    for (i = 0u; i < n; i++) {
        tmp[i] = token[i];
    }
    tmp[n] = '\0';
    kv_str(o, key, tmp, sizeof tmp);
}

/*
 * One [model_NAME] table per row of model_params.def, in file order (plan 03-18; D-113, T-03-50): the value the instance holds, the ledger id and the status of the row (assumed or open, never a claim) and who bound the value, default when no run-request key
 * set it and run-request when one did. A value read from an instance whose selection is outside the allowed list is a state error.
 */
static void emit_model(struct out *o, const struct ds_instance *inst, const struct ds_text_run *run)
{
    uint32_t i;

    for (i = 0u; i < (uint32_t)DS_MODEL_COUNT; i++) {
        size_t n = 0u;
        const char *value = ds_model_value(inst, i, &n);
        const char *status = ds_model_status(i);
        const char *ledger = ds_model_ledger(i);
        int bound = run != NULL && i < 32u && ((run->model_bound >> i) & 1u) != 0u;

        if (value == NULL || status == NULL || ledger == NULL) {
            fail(o, DS_TEXT_ERR_STATE);
            return;
        }
        table(o, "model_", ds_model_name(i));
        kv_model_value(o, "value", value, n);
        kv_str(o, "ledger", ledger, 16u);
        kv_str(o, "status", status, 16u);
        kv_lit(o, "bound_by", bound ? "run-request" : "default");
    }
}

/*
 * The [stimulus] table (plan 03-23; D-112, D-53 amended, T-03-65): the declared IRQ1 and IRQ2 test stimulus of the run, copied into the instance before the first step. events is the number of used slots (a slot with line 0 is unused), and
 * event_1 to event_N hold one event each as three decimal fields separated by commas, AT,LINE,LEVEL (the machine time in master clocks, line 1 or 2, level 1 requested or 0): never a dd:dd shape, which the record lint would take for a time. No
 * stimulus declared prints events = 0 and no event line. The list is a test hook (ER-5), not a statement about any device. A slot list that ds_irq_stim_set could not have produced (a used slot after an unused one, a line above 2, a level
 * above 1, times that go back) is a state error: the printer never guesses.
 */
static void emit_stimulus(struct out *o, const struct ds_instance *inst)
{
    uint32_t n = 0u;
    uint32_t i;

    while (n < (uint32_t)DS_IRQ_STIM_MAX && inst->irq_stim_line[n] != 0u) {
        n++;
    }
    for (i = 0u; i < (uint32_t)DS_IRQ_STIM_MAX; i++) {
        int used = i < n;

        if (used ? (inst->irq_stim_line[i] > 2u || inst->irq_stim_level[i] > 1u || (i > 0u && inst->irq_stim_at[i] < inst->irq_stim_at[i - 1u]))
                 : (inst->irq_stim_line[i] != 0u || inst->irq_stim_level[i] != 0u || inst->irq_stim_at[i] != UINT64_C(0))) {
            fail(o, DS_TEXT_ERR_STATE);
            return;
        }
    }
    table(o, "", "stimulus");
    kv_u32(o, "events", n);
    for (i = 0u; i < n; i++) {
        put_lit(o, "event_");
        put_u32(o, i + 1u);
        put_lit(o, " = \"");
        put_u64(o, inst->irq_stim_at[i]);
        put_ch(o, ',');
        put_u32(o, (uint32_t)inst->irq_stim_line[i]);
        put_ch(o, ',');
        put_u32(o, (uint32_t)inst->irq_stim_level[i]);
        put_lit(o, "\"\n");
    }
}

/* the [constants] table: one line per row of constants.def, NAME = "VALUE LEDGER" (the value in decimal, a blank, the ledger id), in file order */
static void emit_constants(struct out *o)
{
    uint32_t i;

    table(o, "", "constants");
    for (i = 0u; i < CONST_COUNT; i++) {
        put_lit(o, const_rows[i].name);
        put_lit(o, " = \"");
        put_u32(o, const_rows[i].value);
        put_ch(o, ' ');
        put_lit(o, const_rows[i].ledger);
        put_lit(o, "\"\n");
    }
}

/*
 * The [run] table (plan 03-18; D-95): how the run named by the caller ended. reason is none for a record of a boot that ran nothing, then until, callback or refused; overshoot is the
 * master clocks the last step ended past the deadline; stop_point is where the run stopped (none, instruction or block_byte); steps counts the steps this call took. A NULL result prints none, 0, none, 0.
 */
static void emit_run(struct out *o, const struct ds_text_run *run)
{
    const struct ds_run_result *r = run != NULL ? run->result : NULL;
    const char *reason = "none";
    const char *stop = "none";

    if (r != NULL) {
        reason = r->reason == (uint32_t)DS_RUN_REASON_UNTIL      ? "until"
                 : r->reason == (uint32_t)DS_RUN_REASON_CALLBACK ? "callback"
                 : r->reason == (uint32_t)DS_RUN_REASON_REFUSED  ? "refused"
                                                                 : NULL;
        stop = r->stop_point == (uint32_t)DS_STOP_INSTRUCTION  ? "instruction"
               : r->stop_point == (uint32_t)DS_STOP_BLOCK_BYTE ? "block_byte"
                                                               : NULL;
        if (reason == NULL || stop == NULL) {
            fail(o, DS_TEXT_ERR_STATE);
            return;
        }
    }
    table(o, "", "run");
    kv_lit(o, "reason", reason);
    kv_u64(o, "overshoot", r != NULL ? r->overshoot : UINT64_C(0));
    kv_lit(o, "stop_point", stop);
    kv_u64(o, "steps", r != NULL ? r->steps : UINT64_C(0));
}

/*
 * The machine table: the machine time with its unit and the state digest. The time is a count of master clocks of OSC1 (D-90, D-91):
 * the unit says so, and no hardware clock accuracy is claimed (LED-PROF-0002). A digest that is equal to another is evidence of
 * repeatability only.
 */
static void emit_machine(struct out *o, const struct ds_instance *inst)
{
    uint8_t digest[DS_DIGEST_BYTES];

    if (ds_digest(inst, digest) != (uint32_t)DS_OK) {
        fail(o, DS_TEXT_ERR_STATE);
        return;
    }
    table(o, "", "machine");
    kv_u64(o, "time", inst->machine_time);
    kv_lit(o, "time_unit", "master-clocks");
    kv_lit(o, "digest_algorithm", "sha256");
    kv_hex(o, "digest", digest, DS_DIGEST_BYTES);
}

/* finish: on success terminate the text, on any error leave an empty string and a zero length */
static enum ds_text_status finish(struct out *o, char *buf, size_t cap, size_t *outlen)
{
    if (o->err == DS_TEXT_OK) {
        buf[o->len] = '\0'; /* put_ch kept one byte free */
        *outlen = o->len;
        return DS_TEXT_OK;
    }
    if (buf != NULL && cap > 0u) {
        /* nothing is emitted: the part of the text written before the error is wiped, not only cut off at the first byte */
        size_t i;
        for (i = 0u; i < o->len && i < cap; i++) {
            buf[i] = '\0';
        }
        buf[0] = '\0';
    }
    if (outlen != NULL) {
        *outlen = 0u;
    }
    return o->err;
}

static enum ds_text_status begin(struct out *o, char *buf, size_t cap, size_t *outlen, const void *subject)
{
    o->buf = buf;
    o->cap = cap;
    o->len = 0u;
    o->err = DS_TEXT_OK;
    if (buf == NULL || cap == 0u || outlen == NULL || subject == NULL) {
        o->err = DS_TEXT_ERR_ARGUMENT;
        return finish(o, buf, cap, outlen);
    }
    return DS_TEXT_OK;
}

enum ds_text_status ds_text_profile(const struct ds_profile *profile, char *buf, size_t cap, size_t *outlen)
{
    struct out o;
    enum ds_text_status st = begin(&o, buf, cap, outlen, profile);

    if (st != DS_TEXT_OK) {
        return st;
    }
    emit_profile(&o, profile);
    return finish(&o, buf, cap, outlen);
}

static void emit_header(struct out *o)
{
    table(o, "", "record");
    kv_u32(o, "format_version", DS_TEXT_FORMAT_VERSION);
    table(o, "", "core");
    kv_str(o, "build_id", DS_BUILD_ID, sizeof DS_BUILD_ID);
    kv_u32(o, "behaviour_epoch", DS_BEHAVIOUR_EPOCH);
}

enum ds_text_status ds_text_record(const struct ds_instance *inst, const struct ds_text_run *run, char *buf, size_t cap, size_t *outlen)
{
    struct out o;
    enum ds_text_status st = begin(&o, buf, cap, outlen, inst);

    if (st != DS_TEXT_OK) {
        return st;
    }
    emit_header(&o);
    emit_profile(&o, &inst->profile);
    emit_withheld(&o, &inst->profile);
    emit_content(&o, inst);
    emit_power_on(&o, inst);
    emit_model(&o, inst, run);
    emit_stimulus(&o, inst);
    emit_constants(&o);
    emit_run(&o, run);
    table(&o, "", "limits");
    kv_u32(&o, "size_cap", (uint32_t)DS_ROM_CAP);
    emit_machine(&o, inst);
    return finish(&o, buf, cap, outlen);
}

enum ds_text_status ds_text_refusal(uint32_t reason, const char *requested_id, uint32_t requested_rev, char *buf, size_t cap,
                                    size_t *outlen)
{
    struct out o;
    enum ds_text_status st = begin(&o, buf, cap, outlen, requested_id);
    const char *name;

    if (st != DS_TEXT_OK) {
        return st;
    }
    name = ds_refusal_name(reason);
    if (reason == (uint32_t)DS_OK || name == NULL) { /* a refusal record needs a refusal: 0 is an accept, and an unknown number is no member */
        fail(&o, DS_TEXT_ERR_STATE);
        return finish(&o, buf, cap, outlen);
    }
    emit_header(&o);
    table(&o, "", "refusal");
    kv_str(&o, "reason", name, DS_TEXT_REASON_MAX);
    kv_str(&o, "requested_id", requested_id, DS_TEXT_REQUEST_ID_MAX);
    kv_u32(&o, "requested_rev", requested_rev);
    return finish(&o, buf, cap, outlen);
}
