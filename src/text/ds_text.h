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
 * The accept record prints, in order, the tables record, core, profile (id, rev and the resolved sha256 from ds_profile_hash), the 13
 * field_NAME tables, withheld (the sorted ledger ids that bound the run), content, power_on, limits and machine (the machine time in
 * the unit master-clocks, and the state digest from ds_digest). The refusal record prints record, core and refusal (reason,
 * requested_id, requested_rev) and nothing after: a run that was refused has no machine. Every value is read from the instance through
 * the core's accessors; there is no struct that repackages them. A digest and a profile hash are evidence of repeatability only.
 */
#ifndef DS_TEXT_H
#define DS_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

/* the longest requested id (with its terminator) and refusal name the refusal record prints; a longer one is DS_TEXT_ERR_STRING */
#define DS_TEXT_REQUEST_ID_MAX 64u
#define DS_TEXT_REASON_MAX 40u

/* version of the record layout: bumped whenever a table or a key is added, removed or reordered */
#define DS_TEXT_FORMAT_VERSION 2u

enum ds_text_status {
    DS_TEXT_OK = 0,
    DS_TEXT_ERR_ARGUMENT = 1, /* a NULL pointer, or a zero capacity */
    DS_TEXT_ERR_CAPACITY = 2, /* the text and its terminator do not fit in cap */
    DS_TEXT_ERR_STRING = 3,   /* a value holds a double quote, a backslash or a byte outside printable ASCII (D-69) */
    DS_TEXT_ERR_STATE = 4     /* an enumerated value of the instance is not a member of its set */
};

/* the whole accept record of a booted instance */
enum ds_text_status ds_text_record(const struct ds_instance *inst, char *buf, size_t cap, size_t *outlen);

/*
 * The refusal record of a boot that was refused: reason is a refusal number (not 0, and a member of the refusal set, else
 * DS_TEXT_ERR_STATE), requested_id the profile id the request named (NUL-terminated, at most DS_TEXT_REQUEST_ID_MAX - 1 printable
 * characters without a double quote or a backslash, else DS_TEXT_ERR_STRING and nothing is emitted, D-69) and requested_rev its revision.
 */
enum ds_text_status ds_text_refusal(uint32_t reason, const char *requested_id, uint32_t requested_rev, char *buf, size_t cap,
                                    size_t *outlen);

/*
 * The profile identity text: the [profile] table (with its sha256) and the 13 [field_NAME] tables exactly as ds_text_record prints them (the
 * same text is a substring of the record). Label text is not part of the core, so it is not part of this text.
 */
enum ds_text_status ds_text_profile(const struct ds_profile *profile, char *buf, size_t cap, size_t *outlen);

#endif
