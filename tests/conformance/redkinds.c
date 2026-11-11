/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * redkinds.c - the one output function of the runner and the no-leak kinds (D-47, D-56, T-02-21).
 * Every line the runner prints, on stdout or stderr, goes through emit_out or emit_err, which write the line and also append it to an
 * in-process log (capped; an overflow is remembered and makes the redaction kind fail, never pass). The kinds scan that log, the
 * record text and the evidence text of a synthetic ROM for markers:
 *   redaction               a ROM is generated at run time with markers derived from --marker-seed (scripts/conformance-run passes the
 *                           host clock and pid, so no marker is a constant in any source or any output); it is booted, the record is
 *                           printed, and a byte scanner looks for every marker (as bytes, and as lowercase and uppercase hex) in
 *                           everything the runner printed so far, the record, the evidence text and the observed-value text. Zero hits pass.
 *   redaction_leak_control  the scanner must see a leak: a marker appended to a scratch copy of the log is found (as bytes and as hex),
 *                           and the clean copy is not flagged, so the scan is not vacuous in either direction.
 * The runner never prints bytes, strings or paths found inside content; the markers exist only in memory. Test code only.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_text.h"
#include "runner.h"
#include "synth.h"

/* ---- the one output function and its in-process log ---- */

#define LOG_CAP 262144u
#define LINE_MAX_OUT 1024u

static char out_log[LOG_CAP];
static size_t out_len;
static int out_overflow;

static void log_append(const char *s, size_t n)
{
    if (n > LOG_CAP - out_len) {
        out_overflow = 1;
        n = LOG_CAP - out_len;
    }
    memcpy(out_log + out_len, s, n);
    out_len += n;
}

static void vemit(FILE *fp, const char *fmt, va_list ap)
{
    char line[LINE_MAX_OUT];
    int n = vsnprintf(line, sizeof line, fmt, ap);

    if (n < 0) {
        n = 0;
    }
    if ((size_t)n >= sizeof line) {
        n = (int)sizeof line - 1;
        out_overflow = 1; /* a line that did not fit is cut: the log is no longer complete */
    }
    fwrite(line, 1u, (size_t)n, fp);
    fflush(fp);
    log_append(line, (size_t)n);
}

void emit_out(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vemit(stdout, fmt, ap);
    va_end(ap);
}

void emit_err(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vemit(stderr, fmt, ap);
    va_end(ap);
}

/* ---- markers: run-time unique, from an LCG seeded by --marker-seed; never a constant, never a clock call here ---- */

#define MARKERS 4u
#define MARKER_LEN 16u

static uint32_t seed_state;
static int seed_set;

void set_marker_seed(size_t v)
{
    seed_state = (uint32_t)(v ^ (v >> 32));
    seed_set = 1;
}

static void make_markers(char m[MARKERS][MARKER_LEN + 1u])
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    uint32_t x = seed_state;
    unsigned i;
    unsigned j;

    for (i = 0u; i < MARKERS; i++) {
        for (j = 0u; j < MARKER_LEN; j++) {
            x = x * 1664525u + 1013904223u;
            m[i][j] = alphabet[(x >> 16) % 36u];
        }
        m[i][MARKER_LEN] = '\0';
    }
}

/* ---- the scanner, written for this purpose: occurrences of needle in buf, overlaps counted, bytes compared one at a time ---- */

static unsigned scan_hits(const char *buf, size_t n, const char *needle, size_t m)
{
    size_t i;
    unsigned hits = 0u;

    for (i = 0u; m != 0u && i + m <= n; i++) {
        size_t j = 0u;

        while (j < m && buf[i + j] == needle[j]) {
            j++;
        }
        hits += j == m ? 1u : 0u;
    }
    return hits;
}

/* the marker as bytes, as lowercase hex and as uppercase hex */
static unsigned scan_marker(const char *buf, size_t n, const char *marker)
{
    static const char lower[] = "0123456789abcdef";
    static const char upper[] = "0123456789ABCDEF";
    char hex[2u * MARKER_LEN + 1u];
    unsigned hits = scan_hits(buf, n, marker, MARKER_LEN);
    unsigned i;

    for (i = 0u; i < MARKER_LEN; i++) {
        hex[2u * i] = lower[((unsigned char)marker[i] >> 4) & 0x0fu];
        hex[2u * i + 1u] = lower[(unsigned char)marker[i] & 0x0fu];
    }
    hex[2u * MARKER_LEN] = '\0';
    hits += scan_hits(buf, n, hex, 2u * MARKER_LEN);
    for (i = 0u; i < MARKER_LEN; i++) {
        hex[2u * i] = upper[((unsigned char)marker[i] >> 4) & 0x0fu];
        hex[2u * i + 1u] = upper[(unsigned char)marker[i] & 0x0fu];
    }
    return hits + scan_hits(buf, n, hex, 2u * MARKER_LEN);
}

static unsigned scan_all(const char *buf, size_t n, char m[MARKERS][MARKER_LEN + 1u])
{
    unsigned i;
    unsigned hits = 0u;

    for (i = 0u; i < MARKERS; i++) {
        hits += scan_marker(buf, n, m[i]);
    }
    return hits;
}

/* ---- kind redaction ---- */

void kind_redaction(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const size_t site[MARKERS] = {0x0100u, 0x1F26u, 0x1FD0u, 0x0u}; /* the last one is placed near the end of the image */
    static char text[REC_CAP];
    char marker[MARKERS][MARKER_LEN + 1u];
    char evid[512];
    char *scan;
    struct spec s;
    struct ds_boot_request rq;
    struct ds_boot_result res;
    unsigned char *content;
    unsigned char *mem;
    size_t natural = synth_size(k->val[K_FIXTURE]);
    size_t len = 0u;
    size_t total;
    unsigned i;
    unsigned hits;
    const char *err;

    snprintf(got, gotcap, "markers=%u hits=?", MARKERS);
    if (!seed_set) {
        harness_error(c, "the redaction kind needs --marker-seed (scripts/conformance-run passes it)");
        return;
    }
    err = natural == 0u ? "unknown fixture recipe" : parse_spec(&s, k->val[K_REQUEST], natural);
    if (err == NULL && (s.ptr != PTR_OK || s.len < 0x2000u || s.len > CONTENT_LIMIT)) {
        err = "a redaction case needs ptr=ok and a len of one bank up to the content limit";
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    make_markers(marker);
    content = (unsigned char *)malloc(s.len);
    mem = (unsigned char *)malloc(ds_instance_size());
    if (content == NULL || mem == NULL || synth_fill(k->val[K_FIXTURE], content, s.len) != 0) {
        free(content);
        free(mem);
        harness_error(c, "out of memory or unknown recipe for the synthetic ROM");
        return;
    }
    for (i = 0u; i < MARKERS; i++) {
        size_t at = i == MARKERS - 1u ? s.len - 64u : site[i];

        memcpy(content + at, marker[i], MARKER_LEN);
    }
    fill_request(&rq, &s, content);
    memset(&res, 0xff, sizeof res);
    if (ds_boot(mem, ds_instance_size(), &rq, &res) != 0u || res.instance == NULL) {
        free(content);
        free(mem);
        harness_error(c, "the synthetic ROM was refused: the redaction case needs an accepted boot");
        return;
    }
    if (ds_text_record(res.instance, NULL, text, sizeof text, &len) != DS_TEXT_OK) {
        free(content);
        free(mem);
        harness_error(c, "ds_text_record failed on the synthetic ROM");
        return;
    }
    snprintf(got, gotcap, "markers=%u hits=0", MARKERS);
    evidence_text(evid, sizeof evid, k->stem, k->val[K_KIND], k->val[K_EXPECT], got, c->n);
    /* one buffer: everything printed so far (stdout and stderr), the record, the evidence text and the observed-value text */
    total = out_len + len + strlen(evid) + strlen(got) + 1u;
    scan = (char *)malloc(total);
    if (scan == NULL) {
        free(content);
        free(mem);
        harness_error(c, "out of memory for the scan buffer");
        return;
    }
    memcpy(scan, out_log, out_len);
    memcpy(scan + out_len, text, len);
    memcpy(scan + out_len + len, evid, strlen(evid));
    memcpy(scan + out_len + len + strlen(evid), got, strlen(got) + 1u);
    hits = scan_all(scan, total - 1u, marker);
    check(c, !out_overflow, "the output log overflowed, so the scan cannot see everything that was printed");
    check(c, len > 0u && out_len > 0u, "there is nothing to scan");
    check(c, hits == 0u, "a marker of the synthetic ROM appears in the runner output, the record or the evidence text");
    snprintf(got, gotcap, "markers=%u hits=%u", MARKERS, hits);
    free(scan);
    free(content);
    free(mem);
}

/* ---- kind redaction_leak_control ---- */

void kind_redaction_leak_control(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char lower[] = "0123456789abcdef";
    char marker[MARKERS][MARKER_LEN + 1u];
    char hexform[2u * MARKER_LEN + 1u];
    char *scratch;
    size_t base = out_len;
    unsigned i;

    (void)k;
    snprintf(got, gotcap, "flagged=0");
    if (!seed_set) {
        harness_error(c, "the redaction kinds need --marker-seed (scripts/conformance-run passes it)");
        return;
    }
    make_markers(marker);
    scratch = (char *)malloc(base + 2u * MARKER_LEN + 64u);
    if (scratch == NULL) {
        harness_error(c, "out of memory for the scratch log");
        return;
    }
    /* the clean copy of the log: the scanner must not flag it */
    memcpy(scratch, out_log, base);
    check(c, base > 0u && scan_all(scratch, base, marker) == 0u, "the scanner flags a log that holds no marker");
    /* a marker appended as bytes, then as lowercase hex, to scratch copies only: each must be flagged */
    memcpy(scratch + base, "leak: ", 6u);
    memcpy(scratch + base + 6u, marker[1], MARKER_LEN);
    check(c, scan_all(scratch, base + 6u + MARKER_LEN, marker) >= 1u, "the scanner missed a marker appended to the log as bytes");
    for (i = 0u; i < MARKER_LEN; i++) {
        hexform[2u * i] = lower[((unsigned char)marker[2][i] >> 4) & 0x0fu];
        hexform[2u * i + 1u] = lower[(unsigned char)marker[2][i] & 0x0fu];
    }
    memcpy(scratch + base, "leak: ", 6u);
    memcpy(scratch + base + 6u, hexform, 2u * MARKER_LEN);
    check(c, scan_all(scratch, base + 6u + 2u * MARKER_LEN, marker) >= 1u, "the scanner missed a marker appended to the log as hex");
    snprintf(got, gotcap, "flagged=%u", 2u);
    free(scratch);
}
