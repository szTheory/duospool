/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_text.h - the canonical printer of the run record (D-40, D-53, D-69). It prints and nothing else: there is no parser in C
 * (D-40, D-51), and the core never includes this header, so dependencies point runner -> text -> core only.
 * The text is canonical so that two runs can be compared byte for byte:
 *   - flat one-level tables, `[name]` then `key = value` lines, one blank line between tables, key order fixed by the X-table
 *     (profile_fields.def) and by the table layout in text.c, LF only, no trailing blank line
 *   - integers are decimal, strings are printable ASCII between double quotes
 *   - no path, pointer, wall time or process id ever appears
 * D-69: a string that holds a double quote, a backslash or a byte outside printable ASCII is never escaped. The printer returns
 * DS_TEXT_ERR_STRING and emits nothing. Any error leaves buf as an empty string (buf[0] is NUL when cap is at least 1) and
 * *outlen as 0, so a failed print can never be read as a short record.
 * On success the text is NUL-terminated inside cap and *outlen is its length without the terminator.
 * This skeleton prints the tables record, core, profile, field_NAME (13), content and limits. The content table starts with the
 * content sha256 (64 lowercase hex characters, D-47); the profile sha256, the withheld ids, the power-on table and the machine table come with later plans.
 */
#ifndef DS_TEXT_H
#define DS_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

/* version of the record layout: bumped whenever a table or a key is added, removed or reordered */
#define DS_TEXT_FORMAT_VERSION 1u

enum ds_text_status {
    DS_TEXT_OK = 0,
    DS_TEXT_ERR_ARGUMENT = 1, /* a NULL pointer, or a zero capacity */
    DS_TEXT_ERR_CAPACITY = 2, /* the text and its terminator do not fit in cap */
    DS_TEXT_ERR_STRING = 3,   /* a value holds a double quote, a backslash or a byte outside printable ASCII (D-69) */
    DS_TEXT_ERR_STATE = 4     /* an enumerated value of the instance is not a member of its set */
};

/* the whole record skeleton of a booted instance */
enum ds_text_status ds_text_record(const struct ds_instance *inst, char *buf, size_t cap, size_t *outlen);

/*
 * The profile identity text: the [profile] table and the 13 [field_NAME] tables exactly as ds_text_record prints them (the
 * same text is a substring of the record). Label text is not part of the core, so it is not part of this text.
 */
enum ds_text_status ds_text_profile(const struct ds_profile *profile, char *buf, size_t cap, size_t *outlen);

#endif
