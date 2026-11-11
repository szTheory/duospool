/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * runner.h - what the conformance runner and its case-kind files share: the assertion counter, the parsed case and request
 * structures and the few helpers every kind uses. The runner itself (runner.c) keeps the TSV reader, the boot kinds and
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
    int timeout; /* a machine-time budget ended the case (outcome timeout, kinds budget_spin) */
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
    int has_seed;      /* seed=N present (the policy itself comes from bind.powerup_policy) */
    uint64_t seed;
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
int parse_u64(const char *v, uint64_t *out); /* decimal, or 0x and hex digits; 0 on success */
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

void kind_noheur(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_copy_in(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_mem_overlap(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the one output function (redkinds.c): every line of stdout and stderr goes through it and into an in-process log that the no-leak
 * kinds scan; evidence_text is the text of an evidence file (runner.c), set_marker_seed takes --marker-seed */
void emit_out(const char *fmt, ...);
void emit_err(const char *fmt, ...);
void evidence_text(char *buf, size_t cap, const char *id, const char *kind, const char *expect, const char *got, unsigned n);
void set_marker_seed(size_t v);
void kind_redaction(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_redaction_leak_control(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the power-on kinds (powkinds.c, plan 12) and the run kinds (stepkinds.c, plan 03-02) */
#define PK_LIST_MAX 512u
int pk_copy_text(const char *src, char *dst, size_t cap);   /* copy a key's text into a work buffer, 0 on success */
char *pk_next_item(char **rest, char sep);                  /* the next separator-delimited item of a list, split in place */
void kind_splitmix(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_poweron(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_run_until(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_run_chunk_sum(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_run_refuse(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_run_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_poweron_domain(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the digest and profile-hash kinds (digkinds.c, plan 13) */
void kind_digest_stream(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_digest_pair(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_digest_region(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_profile_hash(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_nonmutation_digest(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the record kinds and the --records / --golden-dir inputs (reckinds.c, plan 14): record_emit writes DIR/ID.record.toml when --records was
 * given, record_for_accept does so for kind accept, set_evidence_detail puts text after the standard lines of the case's evidence file */
extern const char *records_dir;
extern const char *golden_dir;
void record_emit(const struct kase *k, struct ctx *c, const char *text, size_t len);
void record_for_accept(const struct kase *k, struct ctx *c);
void set_evidence_detail(const char *text);
void kind_record_golden(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_record_refusal(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_record_refuses_quote(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* moved from runner.c by plan 03-03: the record_lines kind, the record parser it shares with content_id, and the row type of both */
#define REC_MAX_ROWS 128u
#define KEY_MAX 72u

struct recrow {
    char key[KEY_MAX]; /* TABLE.KEY */
    const char *val;   /* the text after " = ", quotes kept */
};
const char *parse_record(char *text, struct recrow *rows, size_t *nrows);
const struct recrow *find_row(const struct recrow *rows, size_t n, const char *key);
void kind_record_lines(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the content identity kinds (hashkinds.c, moved from runner.c by plan 03-03); hash_recipe is also read by main for --hash */
int hash_recipe(const char *recipe, char *hex);
void kind_content_id(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_hash_literal(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the interleave kinds (ilvkinds.c, plan 16) */
void kind_interleave(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_interleave_twins(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_interleave_destroy_recreate(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_interleave_aba_order(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_interleave_canaries(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_liveness_shared_state_selfcheck(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the profile kinds (profkinds.c); each gets the case, the counter, and a buffer for the observed value */
void kind_profile_equal(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_names_both_ways(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_field_flip(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the kinds that produce the outcomes timeout and cancelled (spinkinds.c, plan 18): one function for budget_spin and spin_forever */
void kind_spin(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* the guest kind (spinkinds.c until plan 03-04): boots a built authored ROM, runs it with ds_run and judges its DSV1 verdict block from raw RAM */
void kind_guest(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* boot build/gate/fixtures/ROM.pce over the request text: NULL and the accepted instance, or why not (a harness fault of the case); release_boot frees it */
const char *boot_built_rom(const char *rom, const char *request, struct booted *b, const struct ds_instance **inst);

/* the --fixtures list (fixlist.c, plan 17): NAME SIZE for every recipe a case materialises and every sweep length; 0, or 2 on a fault */
int list_fixtures(const size_t *sweep, size_t nsweep);

#endif
