/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * runner.h - what the conformance runner and its case-kind files share: the assertion counter, the parsed case and request
 * structures and the few helpers every kind uses. The runner itself (runner.c) keeps the TSV reader, the boot and record kinds and
 * main; profkinds.c holds the kinds that compare the built-in profile with the frozen r1 record. Test code only, never installed.
 */
#ifndef DS_RUNNER_H
#define DS_RUNNER_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define REC_CAP 16384u

/* the ten keys of a case (D-58 nine plus the request key of D-79(a)) */
enum { K_ID, K_KIND, K_FIXTURE, K_REQUEST, K_EXPECT, K_MEANING, K_REQUIRED, K_CONTROL, K_EXPECT_OUTCOME, K_PINS, NKEYS };

struct kase {
    const char *stem;
    const char *val[NKEYS];
    int has[NKEYS];
    const char *bad; /* the first structural fault of the case, NULL when none */
};

/* assertion counter: a case passes only when it made at least one assertion and none failed */
struct ctx {
    unsigned n;
    int failed;
    int harness;
    char why[160];
};

enum { PTR_OK, PTR_NULL, PTR_DUMMY };
enum { MEM_OK, MEM_NULL, MEM_MISALIGNED };
enum { CAP_OK, CAP_SHORT };
enum { REQ_OK, REQ_NULL };

/* the request of one case, filled from a request-override string (semicolon-separated k=v) */
struct spec {
    char text[512];
    const char *profile;
    uint32_t rev;
    const char *mapper; /* NULL = undeclared */
    uint32_t order;
    uint32_t header;
    size_t len;
    int ptr;
    int mem;
    int cap;
    int req;
    int bad_size;
    uint32_t reserved;
    const char *bind[DS_FIELD_COUNT];
};

#define CONTENT_LIMIT (4u * 1024u * 1024u) /* the largest content buffer a case may allocate; a longer row uses ptr=dummy or ptr=null */

/* what a case expects: accept (with the content size read from the instance) or one refusal (with the field it names) */
struct expect {
    int accept;
    uint32_t reason;
    int has_field;
    uint32_t field;
    int has_size;
    size_t size;
};

/* a booted instance and the content it was booted from (boot_accepted fills it, release_boot frees both) */
struct booted {
    unsigned char *mem;
    unsigned char *content;
};

void check(struct ctx *c, int cond, const char *what);
void harness_error(struct ctx *c, const char *what);
int parse_size(const char *v, size_t *out);
char *read_file(const char *path);
int split_tab(char *line, char **f, int max);
const char *parse_spec(struct spec *s, const char *text, size_t default_len);
const char *parse_expect(struct expect *e, const char *text);
void fill_request(struct ds_boot_request *rq, const struct spec *s, const unsigned char *content);

/* boot a request over a recipe and check the outcome against the expectation (got receives the observed reason name) */
void run_boot(const char *fixture, const struct spec *s, const struct expect *e, struct ctx *c, char *got, size_t gotcap);

/* boot the request text over the fixture's natural size: NULL and the accepted instance, or why not (a refusal is a fault of the case) */
const char *boot_accepted(const char *fixture, const char *request, struct booted *b, struct spec *s, const struct ds_instance **inst);
void release_boot(struct booted *b);

/* the inputs of the profile kinds, set by main from --profile-tsv and --rules-fields */
extern const char *profile_tsv_path;
extern const char *rules_fields_path;

/* what the allocation kind (allockinds.c) needs from the runner: the cases and the allocator calls made inside ds_boot for a refuse row */
size_t case_count(void);
const struct kase *case_get(size_t i);
void boot_row(const struct kase *r, struct ctx *c, unsigned long *allocs);

/* the allocation counter (allocshim.c in the gate presets, a counter that never moves otherwise) and the kind that reads it */
unsigned long ds_shim_calls(void);
void kind_alloc_zero(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the mapper kinds (mapkinds.c) and the oracle file they read (--oracle) */
extern const char *oracle_path;
void kind_banksig(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_unmapped(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the refusal-matrix kinds (refkinds.c); observe_name records the reason a refuse row returned, for the deferred kind coverage */
void observe_name(const char *reason_name);
void kind_precedence(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_nonmutation(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the profile kinds (profkinds.c); each gets the case, the counter, and a buffer for the observed value */
void kind_profile_equal(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_names_both_ways(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_field_flip(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

#endif
