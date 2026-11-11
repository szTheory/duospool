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

/*
 * the keys of a case: the ten mandatory ones (D-58 nine plus the request key of D-79(a)) and the seven optional ones of plan 03-04 (D-57, D-58, D-79
 * amended): until_mclk and checks are integers, the others are strings whose grammar belongs to the kind that reads them. Three places list the keys:
 * this enum, the key table in runner.c and the want table of scripts/lib/inventory-rules.awk.
 */
enum {
    K_ID, K_KIND, K_FIXTURE, K_REQUEST, K_EXPECT, K_MEANING, K_REQUIRED, K_CONTROL, K_EXPECT_OUTCOME, K_PINS,
    K_UNTIL_MCLK, K_POLICIES, K_CHECKS, K_INPUT, K_IRQ_STIM, K_SINKS, K_CHUNKS, NKEYS
};

/* the keys a case may omit: one bit per key, so run_case refuses a missing key only when its bit is clear */
#define K_OPTIONAL_MASK                                                                                                                                       \
    ((1u << K_UNTIL_MCLK) | (1u << K_POLICIES) | (1u << K_CHECKS) | (1u << K_INPUT) | (1u << K_IRQ_STIM) | (1u << K_SINKS) | (1u << K_CHUNKS))

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
    int has_run; /* a kind ran the machine: the RESULT line then ends machine_time=N stop=REASON (note_run, guestkinds.c; D-95) */
    uint64_t run_mclk;
    const char *run_stop;
    char why[160];
};

enum { PTR_OK, PTR_NULL, PTR_DUMMY };
enum { MEM_OK, MEM_NULL, MEM_MISALIGNED };
enum { CAP_OK, CAP_SHORT };
enum { REQ_OK, REQ_NULL };

#define SPEC_MODEL_MAX 4u /* at most the 4 rows of model_params.def (D-113) */

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
    unsigned nmodel;    /* model.NAME=VALUE pairs of the request (plan 03-17): applied with ds_model_set after the boot and before the first step; the texts point into text */
    const char *model_name[SPEC_MODEL_MAX];
    const char *model_value[SPEC_MODEL_MAX];
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
/* apply the model.NAME=VALUE pairs of a parsed request to a booted instance (ds_model_set, before the first step; guestkinds.c): NULL on success, otherwise why not */
const char *apply_model(struct ds_instance *inst, const struct spec *s);

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

/* the bus kinds (buskinds.c, plan 03-20): st_bypass and bankclass_oracle, which compares the class of every bank with the rows of tests/conformance/bankclass-oracle.awk (--bank-oracle FILE) */
extern const char *bank_oracle_path;
void kind_bankclass_oracle(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_st_bypass(const struct kase *k, struct ctx *c, char *got, size_t gotcap); /* the device-register log of ST0 to ST2 under several MPR0 settings, and the guest verdict of fixtures/src/st_bypass.s */

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
void kind_run_overshoot(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
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
void kind_record_run(const struct kase *k, struct ctx *c, char *got, size_t gotcap);       /* plan 03-18: the [run] table after a real ds_run */
void kind_record_constants(const struct kase *k, struct ctx *c, char *got, size_t gotcap); /* plan 03-18: the [constants] table against src/core/constants.def */

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

/* the guest kind (guestkinds.c since plan 03-04): boots a built authored ROM, runs it with ds_run and judges its DSV1 verdict block from raw RAM */
void kind_guest(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* the model parameter kinds (guestkinds.c, plan 03-17): ds_model_set refusals with the instance unchanged, and the deferred coverage of model_refusals.def and the rows of model_params.def */
void kind_model_refuse(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_model_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* the declared IRQ1 and IRQ2 stimulus of a case (plan 03-22; D-112, T-03-60): the case key irq_stim is an array of strings "AT,LINE,LEVEL" (decimal or 0x hex); irq_stim_parse reads it into events (at most cap, a longer list is a fault of the case),
 * declared is 1 when the key is present; apply_irq_stim declares it to a booted instance through ds_irq_stim_set before the first step (no key: nothing is applied). NULL on success, otherwise why not (a fault of the case); *refusal receives the
 * code of stim_refusals.def that ds_irq_stim_set returned (0 for none) and a refusal is also a fault of the case for every caller but the refusal rows. The deferred kind stim_refusal_coverage needs every member of stim_refusals.def observed by a refuse row. */
#define IRQ_STIM_PARSE_MAX 32u
const char *irq_stim_parse(const struct kase *k, struct ds_irq_event *ev, unsigned cap, unsigned *n, int *declared);
const char *apply_irq_stim(struct ds_instance *inst, const struct kase *k, uint32_t *refusal);
void kind_stim_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/*
 * The input schedule of a case (inputkinds.c, plan 03-27; D-51, D-116, D-117, ER-5): the case key input names an input script; scripts/conformance-flatten --inputs parsed it and wrote its rows to the file named by --inputs (inputkinds_arg parses that option for
 * main: 1 when it consumed argv[*a] and its value), which every runner invocation of conformance-run and determinism-run passes, the toy runners included. input_events reads the events of the named script (at most cap; a longer script, a missing script, a missing
 * rows file and a malformed row are faults of the case); declared is 1 when the key is present. apply_input gives the events to a booted instance through ds_input_push before the first step (no key: nothing is applied); *refusal receives the code of
 * input_refusals.def that the core returned (0 for none) and a refusal is a fault of the case for every caller but the refusal rows. input_refuse runs the refusal rows of a refused=input.NAME expect; the deferred kind input_refusal_coverage needs every member observed.
 */
#define INPUT_PARSE_MAX 1100u
struct guest_expect;
extern const char *inputs_tsv_path;
int inputkinds_arg(int argc, char **argv, int *a);
const char *input_events(const struct kase *k, struct ds_input_event *ev, unsigned cap, unsigned *n, int *declared);
const char *apply_input(struct ds_instance *inst, const struct kase *k, uint32_t *refusal);
void input_refuse(const struct kase *k, struct ctx *c, const struct guest_expect *ex, const char *rom, uint64_t until, char *got, size_t gotcap);
void kind_input_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* boot build/gate/fixtures/ROM.pce over the request text: NULL and the accepted instance, or why not (a harness fault of the case); release_boot frees it */
const char *boot_built_rom(const char *rom, const char *request, struct booted *b, const struct ds_instance **inst);
/* the two halves of boot_built_rom (plan 03-10): load_image reads build/gate/fixtures/ROM.pce into a heap buffer (NULL and *len 0 when it cannot); boot_content boots the buffer
 * b->content (heap memory the caller set, len bytes, freed by release_boot) over the request text. The mutation sweep patches a copy of the image between the two. */
unsigned char *load_image(const char *rom, size_t *len);
const char *boot_content(const char *request, size_t len, struct booted *b, const struct ds_instance **inst);

/* the DSV1 verdict block (guestkinds.c has the layout): its offset in work RAM, its size, the result and observed byte bases and the two result values */
#define V_OFF 0x1F00u
#define V_SIZE 0x100u
#define V_MAX_CHECKS 96u
#define V_RES 0x10u
#define V_OBS 0x80u
#define R_PASS 0x50u
#define R_FAIL 0x46u

/*
 * The expect of a guest or mutate_sweep case (plan 03-10): id=N, then items separated by semicolons, in any order, each at most once:
 *   verdict=OUTCOME:REASON  a protocol control (the judge must give exactly this outcome and a reason containing the word)
 *   poison-t0               the t=0 precondition control (guest only)
 *   refused=stim.NAME       a stimulus refusal control (plan 03-22, guest only): the case's irq_stim list must be refused by ds_irq_stim_set with exactly the code NAME of src/core/stim_refusals.def, the instance unchanged, and no guest is run (observe items are not allowed)
 *   refused=input.NAME[,NAME...]  (plan 03-27, guest only) an input refusal control: synthetic events that bypass every script are pushed to ds_input_push and each variant of each named code of src/core/input_refusals.def must come back as exactly that code, the instance
 *                           byte-identical (inputkinds.c); no guest is run except for the late code; observe items are not allowed
 *   observe=IDX:VALUE       the observed byte of observe row IDX must equal VALUE (decimal or 0x hex, at most 255)
 *   observe=IDX:policy      (plan 03-24) the byte is recorded and compared with no value, and it is exempt from the cross-policy equality of the guest kind: the row legitimately reads power-on state (the power-up reads of mmu_mpr)
 *   observe=IDX:differs     (plan 03-24) only in a case that carries policy-diff=IDX for the same IDX: the byte of every policy after the first must differ from the byte of the first one
 *   policy-diff=IDX         (plan 03-24, guest only) the policy-diff negative control: the case must also carry observe=IDX:differs, and every other observe row of the case is observe=IDX:policy or has a value. A fixture that initialised the row before observing
 *                           it makes the case guest_failure, which shows that the comparison across policies can see a difference
 *   exclude=IDX,IDX,...     the checks a mutation sweep skips (strictly ascending), which must equal the observe rows of the fixture (mutate_sweep only)
 */
enum guest_mode { MODE_PLAIN, MODE_VERDICT, MODE_POISON, MODE_REFUSED, MODE_REFUSED_INPUT };
#define GE_ITEMS_MAX 32u

struct guest_expect {
    uint64_t id;
    enum guest_mode mode;
    char outcome[24];
    char reason[48];
    unsigned nobs;
    struct {
        unsigned idx;
        int differs; /* observe=IDX:differs */
        int policy;  /* observe=IDX:policy */
        unsigned value;
    } obs[GE_ITEMS_MAX];
    unsigned pdiff; /* policy-diff=IDX (0 when the case has no such item) */
    unsigned nex;
    unsigned ex[GE_ITEMS_MAX];
};

/* the observe rows of a built fixture (build/gate/fixtures/ROM.observe, one decimal check index per line, ascending): NULL on success, otherwise why not */
struct obs_list {
    unsigned n;
    unsigned idx[GE_ITEMS_MAX];
};
const char *read_observe(const char *rom, struct obs_list *out);
int parse_guest_expect(const char *text, struct guest_expect *e); /* 0 on success */
/* judge the DSV1 block b (256 bytes of raw work RAM) on c: exactly one outcome; got receives the observed summary */
void judge_dsv1(struct ctx *c, const uint8_t *b, unsigned want_id, unsigned want_checks, char *got, size_t gotcap);
/* a protocol-control case: s is the scratch context the judge filled; c passes only on the declared outcome and a reason containing the declared word */
void expect_verdict(struct ctx *c, const struct ctx *s, const struct guest_expect *e, char *got, size_t gotcap);
/* every observe row of the fixture needs an observe=IDX:VALUE item and every item an observe row: harness_failure (observe-missing, observe-unlisted) on t otherwise */
void observe_cover(struct ctx *t, const struct guest_expect *e, const struct obs_list *ol, unsigned nchk);
/* the observed byte of each observe row against its item (only when the block is judged pass) */
void observe_judge(struct ctx *t, const uint8_t *b, const struct guest_expect *e, const struct obs_list *ol);

/* the observer kinds (obskinds.c, plan 03-10) */
void kind_insn_sink_shapes(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

void kind_opcode_exercise(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* plan 03-19: the observer non-interference matrix, the device-register log and the --trace-out rule (obskinds.c). trace_out_id is the value of --trace-out (NULL when not given), parsed by obskinds_arg beside the other
 * options (1 when it consumed argv[*a] and its value); trace_out_main runs the request and returns the exit status of the runner (0, or 2 for a refusal or a fault) */
extern const char *trace_out_id;
int obskinds_arg(int argc, char **argv, int *a);
int trace_out_main(void);
void kind_observer(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_dev_log(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_trace_out_rights(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* the stop callback that holds once the guest has set the DSV1 done state with its complement (a speed aid only, D-95: the deadline stays the hard bound) */
int dsv1_done(const struct ds_instance *inst, void *ctx);

/* the chunk invariance kind (chunkkinds.c, plan 03-26) and the in-process random programs it and the observer kind run (randprog.c): rp_seed is the seed of program index, rp_build writes its RP_IMAGE_BYTES image,
 * boot_random boots a copy of an image built by rp_build over the request text (NULL and the accepted instance, or why not; release_boot frees the copy); programs are built in memory only, from a splitmix64 stream, and no file or host randomness is involved */
#define RP_IMAGE_BYTES 262144u
void kind_chunk_equiv(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
uint64_t rp_seed(unsigned index);
void rp_build(unsigned char *img, uint64_t seed);
const char *boot_random(const unsigned char *img, const char *request, struct booted *b, const struct ds_instance **inst);

/* the mutation sweep (sweepkinds.c, plan 03-10): every literal of the DCK1 table of a built image is flipped in an in-memory copy and must flip exactly its own check */
void kind_mutate_sweep(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
/* the NAME of a fixture key authored:NAME (0 on success; NAME is [a-z0-9-]) and the recording of a run for the machine_time and stop tail of a RESULT line */
int authored_name(const char *fixture, char *out, size_t cap);
void note_run(struct ctx *c, uint64_t machine_time, uint32_t reason);

/* the CPU table kinds (cpukinds.c, plan 03-15) and their input: opcode_table reads the manual TSV named by --opcode-tsv; cpukinds_arg parses that option for main (1 when it consumed argv[*a] and its value) */
extern const char *opcode_tsv_path;
int cpukinds_arg(int argc, char **argv, int *a);
void kind_opcode_table(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_cycle_probe(const struct kase *k, struct ctx *c, char *got, size_t gotcap);
void kind_timer_pure(const struct kase *k, struct ctx *c, char *got, size_t gotcap); /* plan 03-21: the closed-form timer against a naive per-clock model (ds_timer.h entry points, E1 for the step and the period only, ER-3) */

/* the --fixtures list (fixlist.c, plan 17): NAME SIZE for every recipe a case materialises and every sweep length; 0, or 2 on a fault */
int list_fixtures(const size_t *sweep, size_t nsweep);

#endif
