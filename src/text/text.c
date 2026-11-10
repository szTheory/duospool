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

static void put_u32(struct out *o, uint32_t v)
{
    char tmp[10];
    unsigned n = 0u;

    do {
        tmp[n] = (char)('0' + (v % 10u));
        n++;
        v /= 10u;
    } while (v != 0u);
    while (n > 0u) {
        n--;
        put_ch(o, tmp[n]);
    }
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

/* the [profile] table and the 13 field tables, key order from the X-table (profile_fields.def) */
static void emit_profile(struct out *o, const struct ds_profile *p)
{
    uint32_t i;

    table(o, "", "profile");
    kv_str(o, "id", p->id, DS_PROFILE_ID_MAX);
    kv_u32(o, "rev", p->rev);
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
    kv_u32(o, "size", inst->content_size);
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

/* finish: on success terminate the text, on any error leave an empty string and a zero length */
static enum ds_text_status finish(struct out *o, char *buf, size_t cap, size_t *outlen)
{
    if (o->err == DS_TEXT_OK) {
        buf[o->len] = '\0'; /* put_ch kept one byte free */
        *outlen = o->len;
        return DS_TEXT_OK;
    }
    if (buf != NULL && cap > 0u) {
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

enum ds_text_status ds_text_record(const struct ds_instance *inst, char *buf, size_t cap, size_t *outlen)
{
    struct out o;
    enum ds_text_status st = begin(&o, buf, cap, outlen, inst);

    if (st != DS_TEXT_OK) {
        return st;
    }
    table(&o, "", "record");
    kv_u32(&o, "format_version", DS_TEXT_FORMAT_VERSION);
    table(&o, "", "core");
    kv_str(&o, "build_id", DS_BUILD_ID, sizeof DS_BUILD_ID);
    kv_u32(&o, "behaviour_epoch", DS_BEHAVIOUR_EPOCH);
    emit_profile(&o, &inst->profile);
    emit_content(&o, inst);
    table(&o, "", "limits");
    kv_u32(&o, "size_cap", (uint32_t)DS_ROM_CAP);
    return finish(&o, buf, cap, outlen);
}
