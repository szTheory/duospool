/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * runner.c - the conformance runner (D-57, D-60, D-80). ISO C only (fopen, fread, snprintf), so it builds under strict -std=c11.
 * It runs every case in ONE process and prints one line per case:
 *     RESULT <id> <outcome> assertions=<n> evidence=<relpath>
 * then  RUNNER_DONE <n>. In conformance mode stdout carries nothing else (D-80). The runner never aggregates and never decides
 * required versus optional: scripts/conformance-ingest, run by the sh gate, is the only judge. Outcome names come from
 * outcomes.def, never from string literals. Diagnostics go to stderr and never contain content bytes.
 * Cases come from the TSV that scripts/conformance-flatten prints (D-51): the one parser of hand-edited data stays in sh and
 * awk, and this reader refuses a TSV without its closing END row, because a truncated file must never read as a smaller suite.
 * Cases run in two passes (D-86): pass one runs every ordinary case in TSV order; pass two runs the deferred kinds, which
 * aggregate over other rows and so can only be evaluated after the loop (kind coverage). RESULT lines are matched by id.
 * usage: ds_runner --cases FILE --evidence-dir DIR [--profile-tsv FILE] [--rules-fields FILE] [--oracle FILE]
 *   --profile-tsv FILE   the flattened profiles/pce-hucard-2btn/r1.toml (scripts/conformance-flatten), read by the r1 kinds
 *   --rules-fields FILE  the 13 field names of scripts/lib/profile-rules.awk, one per line, read by the names kind
 *   --oracle FILE        the bank-signature rows of tests/conformance/mirror-oracle.awk (SIZE BANK FILEBANK FIRST LAST), read by banksig
 *   --bank-oracle FILE   the 256 rows (BANK CLASS) of tests/conformance/bankclass-oracle.awk, read by bankclass_oracle (buskinds.c, plan 03-20)
 *   --opcode-tsv FILE    the manual opcode table tests/oracle/huc6280-opcodes.tsv, read by the opcode_table kind (cpukinds.c)
 *   --inputs FILE        the flattened input scripts of tests/conformance/inputs (conformance-flatten --inputs: SCRIPT and INPUT rows), read by the guest kind for a case with an input key (inputkinds.c, plan 03-27)
 *   --marker-seed N      the seed of the run-time unique markers of the redaction kinds (D-56); scripts/conformance-run passes the host
 *                        clock and pid. The runner never calls a clock and no marker is a constant in any source
 *   --records DIR        every case of kind accept, record_lines, record_golden or record_refusal writes DIR/ID.record.toml (record text only);
 *                        no other kind writes anything and nothing else is created in DIR (plan 14, D-86; the directory must exist)
 *   --golden-dir DIR     the directory of the committed goldens (tests/conformance/golden) read by the record_golden kind
 *   --only ID            run exactly one case (the precedence mutation of scripts/check uses it); an id no case carries is a harness fault
 *   --exclude-ids FILE   one case id per line: those cases are not run and print no RESULT line (RUNNER_DONE counts the lines printed);
 *                        only conformance-run sanitize passes it, for EXCLUDED_IN_MODE (D-78); an id no case carries is a harness fault (exit 2)
 *        ds_runner --outcomes   prints the compiled outcome names, one per line, and exits 0 (read by scripts/outcome-sync --names)
 *        ds_runner --sweep-lengths   prints the 21 hash sweep lengths, one per line: the only definition of the list (D-59)
 *        ds_runner --emit RECIPE     writes the bytes of a synthetic recipe to stdout, nothing else (plain-256k, len:N)
 *        ds_runner --hash RECIPE     prints the SHA-256 of a recipe's bytes computed by the core, 64 lowercase hex characters
 *        ds_runner --spec            prints the generator spec string that is part of every recipe
 *        ds_runner --cases FILE --fixtures   prints NAME SIZE for every recipe the gate emits, cases and sweep (fixlist.c, plan 17)
 *        ds_runner --cases FILE --trace-out CASE_ID   writes build/gate/traces/CASE_ID.insn and .dev (the instruction trace and the device log of the case's image, run to its deadline) and prints only record counts, sizes and sha256 values;
 *                     refused (exit 2) unless the case's fixture is an authored image listed in build/gate/fixtures-authored.used or a synthetic recipe, or when CASE_ID is not [a-z][a-z0-9_]* (obskinds.c, plan 03-19; D-120); the directory must exist
 *   scripts/conformance-run compares --hash with the host sha tool over --emit for every sweep length (HASH_SWEEP_MISMATCH).
 * exit : 0 every case ran and RUNNER_DONE was printed (the verdict is the ingest's), 2 usage or harness error (no case ran)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_sha256.h"
#include "ds_text.h"
#include "runner.h"
#include "synth.h"

enum outcome {
#define DS_OUTCOME(n) OUT_##n,
#include "outcomes.def"
#undef DS_OUTCOME
    OUT_COUNT
};

static const char *const outcome_name[OUT_COUNT] = {
#define DS_OUTCOME(n) #n,
#include "outcomes.def"
#undef DS_OUTCOME
};

#define CANARY 0xa5u
#define CANARY_TAIL 64u

void check(struct ctx *c, int cond, const char *what)
{
    c->n++;
    if (!cond && !c->failed) {
        c->failed = 1;
        snprintf(c->why, sizeof c->why, "%s", what);
    }
}

void harness_error(struct ctx *c, const char *what)
{
    if (!c->harness) {
        c->harness = 1;
        snprintf(c->why, sizeof c->why, "%s", what);
    }
}

/* the default bindings of every run request (D-67); an owner-selected field is never bound */
static const struct {
    const char *name;
    const char *value;
} default_binding[] = {
    {"io_1000_d6_value", "1"}, {"io_1000_d7_value", "1"},      {"lines_per_frame", "262"},  {"psg_tick_divisor", "6"},
    {"content_byte_order", "pce"}, {"content_copier_header", "refuse"}, {"powerup_policy", "zero"},
};

static int field_by_name(const char *name)
{
    uint32_t i;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        if (strcmp(ds_field_name(i), name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int parse_size(const char *v, size_t *out)
{
    size_t acc = 0u;
    if (*v == '\0') {
        return -1;
    }
    for (; *v != '\0'; v++) {
        unsigned d;
        if (*v < '0' || *v > '9') {
            return -1;
        }
        d = (unsigned)(*v - '0');
        if (acc > (SIZE_MAX - d) / 10u) {
            return -1;
        }
        acc = acc * 10u + d;
    }
    *out = acc;
    return 0;
}

int parse_u64(const char *v, uint64_t *out)
{
    uint64_t acc = 0u;
    unsigned base = 10u;

    if (v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
        base = 16u;
        v += 2;
    }
    if (*v == '\0') {
        return -1;
    }
    for (; *v != '\0'; v++) {
        unsigned d;
        if (*v >= '0' && *v <= '9') {
            d = (unsigned)(*v - '0');
        } else if (base == 16u && *v >= 'a' && *v <= 'f') {
            d = (unsigned)(*v - 'a') + 10u;
        } else if (base == 16u && *v >= 'A' && *v <= 'F') {
            d = (unsigned)(*v - 'A') + 10u;
        } else {
            return -1;
        }
        if (acc > (UINT64_MAX - d) / base) {
            return -1;
        }
        acc = acc * base + d;
    }
    *out = acc;
    return 0;
}

static int parse_u32(const char *v, uint32_t *out)
{
    size_t n;
    uint32_t r;
    if (parse_size(v, &n) != 0) {
        return -1;
    }
    r = (uint32_t)n;
    if ((size_t)r != n) {
        return -1;
    }
    *out = r;
    return 0;
}

/* apply one k=v pair; NULL on success, otherwise the reason */
static const char *apply_pair(struct spec *s, const char *key, const char *val)
{
    if (strcmp(key, "profile") == 0) {
        s->profile = val;
    } else if (strcmp(key, "rev") == 0) {
        if (parse_u32(val, &s->rev) != 0) {
            return "rev is not a number";
        }
    } else if (strcmp(key, "mapper") == 0) {
        s->mapper = strcmp(val, "-") == 0 ? NULL : val;
    } else if (strcmp(key, "order") == 0) {
        if (strcmp(val, "-") == 0) {
            s->order = 0u;
        } else if (strcmp(val, "pce") == 0) {
            s->order = 1u;
        } else if (strcmp(val, "tg16") == 0) {
            s->order = 2u;
        } else {
            return "order is not - or pce or tg16";
        }
    } else if (strcmp(key, "header") == 0) {
        if (strcmp(val, "-") == 0) {
            s->header = 0u;
        } else if (strcmp(val, "none") == 0) {
            s->header = 1u;
        } else if (strcmp(val, "copier_512") == 0) {
            s->header = 2u;
        } else {
            return "header is not - or none or copier_512";
        }
    } else if (strcmp(key, "len") == 0) {
        if (strcmp(val, "size_max") == 0) {
            s->len = SIZE_MAX;
        } else if (parse_size(val, &s->len) != 0) {
            return "len is not a number or size_max";
        }
    } else if (strcmp(key, "ptr") == 0) {
        if (strcmp(val, "ok") == 0) {
            s->ptr = PTR_OK;
        } else if (strcmp(val, "null") == 0) {
            s->ptr = PTR_NULL;
        } else if (strcmp(val, "dummy") == 0) {
            s->ptr = PTR_DUMMY;
        } else {
            return "ptr is not ok or null or dummy";
        }
    } else if (strcmp(key, "mem") == 0) {
        if (strcmp(val, "ok") == 0) {
            s->mem = MEM_OK;
        } else if (strcmp(val, "null") == 0) {
            s->mem = MEM_NULL;
        } else if (strcmp(val, "misaligned") == 0) {
            s->mem = MEM_MISALIGNED;
        } else {
            return "mem is not ok or null or misaligned";
        }
    } else if (strcmp(key, "cap") == 0) {
        if (strcmp(val, "ok") == 0) {
            s->cap = CAP_OK;
        } else if (strcmp(val, "short") == 0) {
            s->cap = CAP_SHORT;
        } else {
            return "cap is not ok or short";
        }
    } else if (strcmp(key, "req") == 0) {
        if (strcmp(val, "ok") == 0) {
            s->req = REQ_OK;
        } else if (strcmp(val, "null") == 0) {
            s->req = REQ_NULL;
        } else {
            return "req is not ok or null";
        }
    } else if (strcmp(key, "struct_size") == 0) {
        if (strcmp(val, "ok") == 0) {
            s->bad_size = 0;
        } else if (strcmp(val, "bad") == 0) {
            s->bad_size = 1;
        } else {
            return "struct_size is not ok or bad";
        }
    } else if (strcmp(key, "reserved") == 0) {
        if (strcmp(val, "0") == 0) {
            s->reserved = 0u;
        } else if (strcmp(val, "1") == 0) {
            s->reserved = 1u;
        } else {
            return "reserved is not 0 or 1";
        }
    } else if (strcmp(key, "seed") == 0) {
        if (parse_u64(val, &s->seed) != 0) {
            return "seed is not a number";
        }
        s->has_seed = 1;
    } else if (strncmp(key, "model.", 6) == 0) {
        if (s->nmodel >= SPEC_MODEL_MAX || key[6] == '\0') {
            return "model. needs a name and at most 4 model keys";
        }
        s->model_name[s->nmodel] = key + 6;
        s->model_value[s->nmodel] = val;
        s->nmodel++;
    } else if (strncmp(key, "bind.", 5) == 0) {
        int f = field_by_name(key + 5);
        if (f < 0) {
            return "bind. names no profile field";
        }
        s->bind[f] = strcmp(val, "-") == 0 ? NULL : val;
    } else {
        return "unknown request key";
    }
    return NULL;
}

/*
 * The value prefix @hex: (plan 14): the rest of the value is lowercase hex, decoded in place to the bytes it names, so that an ASCII case
 * file can carry a hostile string (a double quote, a backslash, a non-ASCII byte). A NUL byte cannot be carried in a C string and is a
 * fault of the case, as is an odd length or a character that is not 0-9 a-f. NULL on success, otherwise the reason.
 */
static const char *decode_hex_value(char *v)
{
    size_t n = strlen(v + 5);
    size_t i;
    char *in = v + 5;

    if (n % 2u != 0u) {
        return "@hex: value has an odd number of digits";
    }
    for (i = 0u; i < n / 2u; i++) {
        int d[2];
        size_t j;
        for (j = 0u; j < 2u; j++) {
            char ch = in[2u * i + j];
            if (ch >= '0' && ch <= '9') {
                d[j] = ch - '0';
            } else if (ch >= 'a' && ch <= 'f') {
                d[j] = ch - 'a' + 10;
            } else {
                return "@hex: value has a character that is not 0-9 a-f";
            }
        }
        if (d[0] == 0 && d[1] == 0) {
            return "@hex: value holds a NUL byte";
        }
        v[i] = (char)(d[0] * 16 + d[1]);
    }
    v[n / 2u] = '\0';
    return NULL;
}

/* fill s from a request-override string; NULL on success, otherwise the reason */
const char *parse_spec(struct spec *s, const char *text, size_t default_len)
{
    char *p;
    size_t i;

    memset(s, 0, sizeof *s);
    s->profile = "pce-hucard-2btn";
    s->rev = 1u;
    s->mapper = "plain";
    s->order = 1u;
    s->header = 1u;
    s->len = default_len;
    s->ptr = PTR_OK;
    for (i = 0u; i < sizeof default_binding / sizeof default_binding[0]; i++) {
        s->bind[field_by_name(default_binding[i].name)] = default_binding[i].value;
    }
    if (strlen(text) >= sizeof s->text) {
        return "request string too long";
    }
    memcpy(s->text, text, strlen(text) + 1u);
    p = s->text;
    while (*p != '\0') {
        char *end = strchr(p, ';');
        char *eq;
        const char *err;
        if (end != NULL) {
            *end = '\0';
        }
        eq = strchr(p, '=');
        if (eq == NULL) {
            return "request pair without =";
        }
        *eq = '\0';
        err = strncmp(eq + 1, "@hex:", 5u) == 0 ? decode_hex_value(eq + 1) : NULL;
        if (err != NULL) {
            return err;
        }
        err = apply_pair(s, p, eq + 1);
        if (err != NULL) {
            return err;
        }
        if (end == NULL) {
            break;
        }
        p = end + 1;
    }
    return NULL;
}

/* expectation: accept[;size=N] or load.NAME[;field=NAME]; a load.NAME that is not a refusal never matches */
const char *parse_expect(struct expect *e, const char *text)
{
    char buf[160];
    char *p;
    int first = 1;

    memset(e, 0, sizeof *e);
    if (strlen(text) >= sizeof buf) {
        return "expect string too long";
    }
    memcpy(buf, text, strlen(text) + 1u);
    p = buf;
    for (;;) {
        char *end = strchr(p, ';');
        if (end != NULL) {
            *end = '\0';
        }
        if (first) {
            first = 0;
            if (strcmp(p, "accept") == 0) {
                e->accept = 1;
            } else {
                uint32_t n;
                for (n = 1u; n < (uint32_t)DS_REFUSAL_SLOTS; n++) {
                    const char *name = ds_refusal_name(n);
                    if (name != NULL && strcmp(p, name) == 0) {
                        e->reason = n;
                        break;
                    }
                }
                if (e->reason == 0u) {
                    if (strncmp(p, "load.", 5) != 0) {
                        return "expect is neither accept nor a load.NAME";
                    }
                    /* a load.NAME that no refusal carries is a wrong literal: it can never match, so the case is a
                     * guest_failure and not a harness fault (a mistyped expectation must show up as a red case) */
                    e->reason = 0xffffffffu;
                }
            }
        } else if (strncmp(p, "field=", 6) == 0) {
            int f = field_by_name(p + 6);
            if (f < 0) {
                return "expect field names no profile field";
            }
            e->has_field = 1;
            e->field = (uint32_t)f;
        } else if (strncmp(p, "size=", 5) == 0) {
            if (parse_size(p + 5, &e->size) != 0) {
                return "expect size is not a number";
            }
            e->has_size = 1;
        } else {
            return "unknown expect token";
        }
        if (end == NULL) {
            break;
        }
        p = end + 1;
    }
    return NULL;
}

static int canary_intact(const unsigned char *p, size_t n)
{
    size_t i;
    for (i = 0u; i < n; i++) {
        if (p[i] != CANARY) {
            return 0;
        }
    }
    return 1;
}

/* every open field of a resolved profile records the run request as its binder, every owner-selected field the constant (D-49) */
static int bound_by_ok(const struct ds_profile *p)
{
    uint32_t i;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        uint8_t want = p->field[i].status == DS_STATUS_OPEN ? (uint8_t)DS_BOUND_RUN_REQUEST : (uint8_t)DS_BOUND_CONSTANT;
        if (p->field[i].bound_by != want) {
            return 0;
        }
    }
    return 1;
}

/* the boot request of a spec over a content pointer */
void fill_request(struct ds_boot_request *rq, const struct spec *s, const unsigned char *content)
{
    uint32_t i;

    memset(rq, 0, sizeof *rq);
    rq->struct_size = (uint32_t)sizeof *rq + (s->bad_size ? 1u : 0u);
    rq->reserved = s->reserved;
    rq->profile_id = s->profile;
    rq->profile_rev = s->rev;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        rq->bindings.value[i] = s->bind[i];
    }
    rq->mapper = s->mapper;
    rq->byte_order = s->order;
    rq->header = s->header;
    rq->content = content;
    rq->len = s->len;
    rq->has_seed = s->has_seed ? 1u : 0u;
    rq->powerup_seed = s->has_seed ? s->seed : 0u;
}

/* boot one case from the fixture recipe and check the outcome against the expectation; got receives the observed reason name */
static unsigned long boot_allocs; /* allocator calls made during the ds_boot call of the last run_boot (alloc_zero_matrix) */

void run_boot(const char *fixture, const struct spec *s, const struct expect *e, struct ctx *c, char *got, size_t gotcap)
{
    static const unsigned char dummy[1] = {0u};
    struct ds_boot_request rq;
    struct ds_boot_result res;
    size_t isize = ds_instance_size();
    size_t total = isize + CANARY_TAIL;
    unsigned char *base = (unsigned char *)malloc(total);
    unsigned char *buf = NULL;
    const unsigned char *cptr = NULL;
    void *mem;
    size_t cap;
    uint32_t ret;

    if (base == NULL) {
        harness_error(c, "out of memory for the instance buffer");
        return;
    }
    if (s->nmodel != 0u) {
        free(base);
        harness_error(c, "a model.NAME key applies to a kind that runs the machine, not to a boot row");
        return;
    }
    memset(base, (int)CANARY, total);
    if (s->ptr == PTR_OK) {
        if (s->len > CONTENT_LIMIT) {
            free(base);
            harness_error(c, "len above the content limit needs ptr=dummy or ptr=null");
            return;
        }
        buf = (unsigned char *)malloc(s->len != 0u ? s->len : 1u);
        if (buf == NULL) {
            free(base);
            harness_error(c, "out of memory for the content buffer");
            return;
        }
        if (synth_fill(fixture, buf, s->len) != 0) {
            free(buf);
            free(base);
            harness_error(c, "unknown fixture recipe");
            return;
        }
        cptr = buf;
    } else if (s->ptr == PTR_DUMMY) {
        cptr = dummy;
    }
    fill_request(&rq, s, cptr);
    memset(&res, 0xff, sizeof res);
    mem = s->mem == MEM_NULL ? NULL : (s->mem == MEM_MISALIGNED ? (void *)(base + 1) : (void *)base);
    cap = s->cap == CAP_SHORT ? isize - 1u : isize;

    boot_allocs = ds_shim_calls();
    ret = ds_boot(mem, cap, s->req == REQ_NULL ? NULL : &rq, &res);
    boot_allocs = ds_shim_calls() - boot_allocs;

    if (ds_refusal_name(ret) != NULL) {
        snprintf(got, gotcap, "%s", ret == 0u ? "accept" : ds_refusal_name(ret));
    } else {
        snprintf(got, gotcap, "unknown_%u", (unsigned)ret);
    }
    check(c, res.reason == ret, "result.reason differs from the return value");
    if (e->accept) {
        check(c, ret == 0u, "expected accept, got a refusal");
        if (ret == 0u) {
            const struct ds_instance *inst = res.instance;
            check(c, res.instance == (struct ds_instance *)mem && inst != NULL, "result.instance is not the caller memory");
            check(c, res.field_index == DS_NO_FIELD, "an accept names a field");
            if (inst != NULL) {
                check(c, !e->has_size || (size_t)ds_size_bytes(inst->size_class) == e->size, "content size read from the instance differs");
                check(c, buf == NULL || memcmp(inst->rom, buf, s->len) == 0, "the ROM copy differs from the supplied bytes");
                check(c, strcmp(inst->profile.id, "pce-hucard-2btn") == 0 && inst->profile.rev == 1u,
                      "the resolved profile is not recorded");
                check(c, inst->machine_time == 0u, "machine time is not zero at power-on");
                check(c, bound_by_ok(&inst->profile), "a resolved field has the wrong bound_by (open: run-request, owner: constant)");
            }
            check(c, canary_intact(base + isize, CANARY_TAIL), "the core wrote past the instance");
        }
    } else {
        check(c, ret == e->reason, "refusal differs from the expected one");
        check(c, res.instance == NULL, "a refusal returned an instance");
        check(c, !e->has_field || res.field_index == e->field, "the named field differs");
        check(c, canary_intact(base, total), "a refusal wrote to caller memory");
    }
    free(buf);
    free(base);
}


/*
 * For kind alloc_zero (allockinds.c): the cases, and the allocator calls made inside ds_boot when the request of a refuse row is
 * booted again with fresh test-allocated memory. On a row that cannot be booted c gets a harness error and *allocs is left alone.
 */
void boot_row(const struct kase *r, struct ctx *c, unsigned long *allocs)
{
    struct spec s;
    struct expect e;
    struct ctx sub;
    char got[64];
    const char *err;
    size_t natural;

    if (r->bad != NULL || !r->has[K_FIXTURE] || !r->has[K_REQUEST] || !r->has[K_EXPECT]) {
        harness_error(c, "a refuse row is malformed");
        return;
    }
    natural = synth_size(r->val[K_FIXTURE]);
    err = natural == 0u ? "unknown fixture recipe" : parse_spec(&s, r->val[K_REQUEST], natural);
    if (err == NULL) {
        err = parse_expect(&e, r->val[K_EXPECT]);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    memset(&sub, 0, sizeof sub);
    run_boot(r->val[K_FIXTURE], &s, &e, &sub, got, sizeof got);
    if (sub.harness) {
        harness_error(c, sub.why);
        return;
    }
    *allocs = boot_allocs;
}

/* the text of an evidence file: ids, reason names, sizes and assertion counts only, never content bytes */
void evidence_text(char *buf, size_t cap, const char *id, const char *kind, const char *expect, const char *got, unsigned n)
{
    snprintf(buf, cap, "id=%s\nkind=%s\nexpect=%s\ngot=%s\nassertions=%u\n", id, kind, expect, got, n);
}

/* the evidence directory of the case that is running (runner.h): set by run_case before the kind runs */
const char *run_evidence_dir;

/* text a kind wants after the standard lines of its evidence file (the refusal record); cleared before every case */
static char ev_detail[4096];

/* the per-class counts of a guest case (plan 03-32; ER-4): the evidence file of a case that set them carries one  classes: man=N guest=N assumed=N  line, so no total stands alone as support; cleared before every case */
static int ev_classes_set;
static unsigned ev_man;
static unsigned ev_guest;
static unsigned ev_assumed;
static int ev_control;

void set_evidence_detail(const char *text)
{
    snprintf(ev_detail, sizeof ev_detail, "%s", text);
}

void set_evidence_classes(unsigned man, unsigned guest, unsigned assumed, int control)
{
    ev_classes_set = 1;
    ev_control = control;
    ev_man = man;
    ev_guest = guest;
    ev_assumed = assumed;
}

static int write_evidence(const char *path, const char *id, const char *kind, const char *expect, const char *got, unsigned n)
{
    char text[1024];
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }
    evidence_text(text, sizeof text, id, kind, expect, got, n);
    fputs(text, fp);
    if (ev_classes_set && ev_control) {
        /* a protocol control (the runner judges the outcome of a planted or broken verdict block, not the rows of the fixture): its assertions are the runner's own comparisons, counted apart from every class */
        fprintf(fp, "classes: man=0 guest=0 assumed=0 control=%u\n", n);
    } else if (ev_classes_set) {
        fprintf(fp, "classes: man=%u guest=%u assumed=%u\n", ev_man, ev_guest, ev_assumed);
    }
    if (ev_detail[0] != '\0') {
        fputs("record:\n", fp);
        fputs(ev_detail, fp);
    }
    return fclose(fp) == 0 ? 0 : -1;
}

/* ---- the cases TSV (scripts/conformance-flatten): K<TAB>STEM<TAB>TABLE<TAB>KEY<TAB>TYPE<TAB>VALUE rows and a last END row ---- */


/* the keys of a case (runner.h) and the TSV type of each; the keys of the second row are optional (K_OPTIONAL_MASK) */
static const struct {
    const char *name;
    char type;
} key_def[NKEYS] = {
    {"id", 's'},      {"kind", 's'},    {"fixture", 's'},         {"request", 's'},  {"expect", 's'},
    {"meaning", 's'}, {"required", 'b'}, {"control", 'b'}, {"expect_outcome", 's'}, {"pins", 'a'},
    {"until_mclk", 'i'}, {"policies", 's'}, {"checks", 'i'}, {"input", 's'}, {"irq_stim", 'a'}, {"sinks", 's'}, {"chunks", 's'},
};

/* an integer key is an unsigned decimal of one to nineteen digits (T-03-11), so it always fits a uint64_t */
static int is_uint_text(const char *v)
{
    size_t n = strlen(v);

    return n >= 1u && n <= 19u && strspn(v, "0123456789") == n;
}

#define MAX_CASES 1024u


static struct kase cases[MAX_CASES];
static size_t ncases;

size_t case_count(void)
{
    return ncases;
}

const struct kase *case_get(size_t i)
{
    return &cases[i];
}

/* read a whole file into a NUL-terminated heap buffer; NULL on any failure */
char *read_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    char *buf;
    long size;
    size_t got;

    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    buf = (char *)malloc((size_t)size + 1u);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }
    got = fread(buf, 1u, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) {
        free(buf);
        return NULL;
    }
    buf[got] = '\0';
    return buf;
}

static int valid_id(const char *s)
{
    if (s[0] < 'a' || s[0] > 'z') {
        return 0;
    }
    for (; *s != '\0'; s++) {
        if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_')) {
            return 0;
        }
    }
    return 1;
}

/* split a line in place on TAB into at most max fields, empty fields kept; returns the field count */
int split_tab(char *line, char **f, int max)
{
    int n = 0;
    for (;;) {
        char *t;
        f[n++] = line;
        t = strchr(line, '\t');
        if (t == NULL || n == max) {
            return n;
        }
        *t = '\0';
        line = t + 1;
    }
}

/*
 * Parse the TSV into cases[]. Returns 0, or -1 with the reason in *why. A TSV whose last row is not END, whose END count is not
 * the number of cases, or that holds no case is a harness fault: the runner must never run a smaller suite than the gate asked.
 */
static int load_cases(char *text, const char **why)
{
    char *line = text;
    int seen_end = 0;
    size_t end_count = 0u;

    while (*line != '\0') {
        char *nl = strchr(line, '\n');
        char *f[6];
        int nf;
        if (nl == NULL) {
            *why = "the last line has no newline";
            return -1;
        }
        *nl = '\0';
        if (seen_end) {
            *why = "a row follows the END row";
            return -1;
        }
        nf = split_tab(line, f, 6);
        if (nf == 2 && strcmp(f[0], "END") == 0) {
            if (parse_size(f[1], &end_count) != 0) {
                *why = "malformed END row";
                return -1;
            }
            seen_end = 1;
        } else if (nf == 6 && strcmp(f[0], "K") == 0) {
            struct kase *k = NULL;
            size_t i;
            int key = -1;
            int j;
            for (i = 0u; i < ncases; i++) {
                if (strcmp(cases[i].stem, f[1]) == 0) {
                    k = &cases[i];
                    break;
                }
            }
            if (k == NULL) {
                if (ncases == MAX_CASES) {
                    *why = "too many cases";
                    return -1;
                }
                k = &cases[ncases++];
                k->stem = f[1];
            }
            for (j = 0; j < NKEYS; j++) {
                if (strcmp(f[3], key_def[j].name) == 0) {
                    key = j;
                }
            }
            if (f[2][0] != '\0') {
                k->bad = k->bad != NULL ? k->bad : "a key inside a table";
            } else if (key < 0) {
                k->bad = k->bad != NULL ? k->bad : "unknown key";
            } else if (k->has[key]) {
                k->bad = k->bad != NULL ? k->bad : "duplicate key";
            } else if (f[4][0] != key_def[key].type || f[4][1] != '\0' || (key_def[key].type == 'i' && !is_uint_text(f[5]))) {
                k->bad = k->bad != NULL ? k->bad : "key has the wrong type";
            } else {
                k->has[key] = 1;
                k->val[key] = f[5];
            }
        } else {
            *why = "unexpected row";
            return -1;
        }
        line = nl + 1;
    }
    if (!seen_end) {
        *why = "the END row is missing";
        return -1;
    }
    if (ncases == 0u) {
        *why = "the TSV holds no case";
        return -1;
    }
    if (end_count != ncases) {
        *why = "the END count differs from the number of cases";
        return -1;
    }
    return 0;
}

/* ---- case kinds ---- */

typedef void (*kind_fn)(const struct kase *k, struct ctx *c, char *got, size_t gotcap);

/* kinds refuse and accept: boot the request and compare with the expectation */
static void kind_boot(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct spec s;
    struct expect e;
    const char *err;
    const char *fixture = k->val[K_FIXTURE];
    size_t natural = synth_size(fixture);

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
    if (e.accept != (strcmp(k->val[K_KIND], "accept") == 0)) {
        harness_error(c, "the kind and the expectation disagree about accept");
        return;
    }
    run_boot(fixture, &s, &e, c, got, gotcap);
    if (!e.accept) {
        observe_name(got); /* the reasons seen by refuse rows, for the deferred coverage kind (D-86) */
    } else if (!c->harness) {
        record_for_accept(k, c); /* --records: the record of the accepted boot is one file of the run */
    }
}

/*
 * kind resolve: no content and no boot; the profile id, rev and bindings of the request go to ds_resolve and the result code
 * (and the field it names) is compared with the expectation. accept means the profile resolved: every open field then carries
 * its bound value and the run request as binder.
 */
static void kind_resolve(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct spec s;
    struct expect e;
    struct ds_bindings b;
    struct ds_profile out;
    const char *err;
    uint32_t field = 0xfffffff0u;
    uint32_t ret;
    uint32_t i;

    if (strcmp(k->val[K_FIXTURE], "none") != 0) {
        harness_error(c, "a resolve case has no content: its fixture must be none");
        return;
    }
    err = parse_spec(&s, k->val[K_REQUEST], 0u);
    if (err == NULL) {
        err = parse_expect(&e, k->val[K_EXPECT]);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        b.value[i] = s.bind[i];
    }
    memset(&out, 0, sizeof out);
    ret = ds_resolve(s.profile, s.rev, &b, &out, &field);
    snprintf(got, gotcap, "%s", ds_refusal_name(ret) != NULL ? (ret == 0u ? "accept" : ds_refusal_name(ret)) : "unknown");
    if (e.accept) {
        check(c, ret == 0u, "expected the profile to resolve, got a refusal");
        if (ret == 0u) {
            check(c, field == DS_NO_FIELD, "a resolved profile names a field");
            check(c, bound_by_ok(&out), "a resolved field has the wrong bound_by (open: run-request, owner: constant)");
            for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
                if (out.field[i].status == DS_STATUS_OPEN) {
                    check(c, s.bind[i] != NULL && strcmp(out.field[i].value, s.bind[i]) == 0,
                          "an open field does not carry the bound value");
                }
            }
        }
    } else {
        check(c, ret == e.reason, "resolve result differs from the expected one");
        check(c, !e.has_field || field == e.field, "the named field differs");
    }
}

/* ---- booted instance for the kinds that print or compare after an accepted boot ---- */

void release_boot(struct booted *b)
{
    free(b->content);
    free(b->mem);
    b->content = NULL;
    b->mem = NULL;
}

/*
 * Boot the request text over the fixture's natural size and hand back the accepted instance and the parsed request. NULL on
 * success, otherwise why not: a kind that uses this is about what follows an accepted boot, so a refused request is a harness
 * fault of the case, never a verdict on the guest.
 */
const char *boot_accepted(const char *fixture, const char *request, struct booted *b, struct spec *s,
                          const struct ds_instance **inst)
{
    struct ds_boot_request rq;
    struct ds_boot_result res;
    size_t natural = synth_size(fixture);
    const char *err;

    b->mem = NULL;
    b->content = NULL;
    if (natural == 0u) {
        return "unknown fixture recipe";
    }
    err = parse_spec(s, request, natural);
    if (err != NULL) {
        return err;
    }
    if (s->ptr != PTR_OK || s->len > CONTENT_LIMIT) {
        return "a booted-instance case needs ptr=ok and a len within the content limit";
    }
    b->mem = (unsigned char *)malloc(ds_instance_size());
    b->content = (unsigned char *)malloc(s->len != 0u ? s->len : 1u);
    if (b->mem == NULL || b->content == NULL) {
        release_boot(b);
        return "out of memory for the instance or the content";
    }
    if (synth_fill(fixture, b->content, s->len) != 0) {
        release_boot(b);
        return "unknown fixture recipe";
    }
    fill_request(&rq, s, b->content);
    memset(&res, 0xff, sizeof res);
    if (ds_boot(b->mem, ds_instance_size(), &rq, &res) != 0u || res.instance == NULL) {
        release_boot(b);
        return "the request was refused: this kind needs an accepted boot";
    }
    err = apply_model(res.instance, s);
    if (err != NULL) {
        release_boot(b);
        return err;
    }
    *inst = res.instance;
    return NULL;
}

/* ---- the hash sweep lengths (D-59): the kinds of the content identity live in hashkinds.c, the record kinds in reckinds.c ---- */

/*
 * The lengths of the hash sweep: 0 and 1, the padding boundaries of SHA-256 (55, 56, 57 around the length field, 63, 64, 65 at the
 * block edge, and the same pair of edges one and two blocks on), the bank-size neighbours, and the three supported image sizes.
 * This array is the ONE definition: scripts/conformance-run reads it through --sweep-lengths and plan 17 builds the fixture
 * manifest from the same output, so the two can never disagree.
 */
static const size_t sweep_len[] = {0u,   1u,    54u,   55u,   56u,   57u,   63u,    64u,     65u,      119u,    120u,
                                   121u, 127u,  128u,  129u,  8191u, 8192u, 8193u,  262144u, 393216u, 1048576u};

static const struct {
    const char *name;
    kind_fn fn;
    int deferred; /* evaluated in pass two, after the loop (D-86) */
} kinds[] = {
    {"refuse", kind_boot, 0},
    {"accept", kind_boot, 0},
    {"resolve", kind_resolve, 0},
    {"record_lines", kind_record_lines, 0},
    {"record_run", kind_record_run, 0},
    {"record_constants", kind_record_constants, 0},
    {"profile_equal", kind_profile_equal, 0},
    {"names_both_ways", kind_names_both_ways, 0},
    {"field_flip", kind_field_flip, 0},
    {"content_id", kind_content_id, 0},
    {"hash_literal", kind_hash_literal, 0},
    {"banksig", kind_banksig, 0},
    {"bankclass_oracle", kind_bankclass_oracle, 0},
    {"st_bypass", kind_st_bypass, 0},
    {"unmapped", kind_unmapped, 0},
    {"alloc_zero", kind_alloc_zero, 0},
    {"precedence", kind_precedence, 0},
    {"nonmutation", kind_nonmutation, 0},
    {"noheur", kind_noheur, 0},
    {"copy_in", kind_copy_in, 0},
    {"mem_overlap", kind_mem_overlap, 0},
    {"redaction", kind_redaction, 0},
    {"redaction_leak_control", kind_redaction_leak_control, 0},
    {"splitmix", kind_splitmix, 0},
    {"poweron", kind_poweron, 0},
    {"run_until", kind_run_until, 0},
    {"run_chunk_sum", kind_run_chunk_sum, 0},
    {"run_overshoot", kind_run_overshoot, 0},
    {"run_refuse", kind_run_refuse, 0},
    {"budget_spin", kind_spin, 0},
    {"spin_forever", kind_spin, 0},
    {"guest", kind_guest, 0},
    {"video", kind_video, 0},
    {"vdc_fold", kind_vdc_fold, 0},
    {"vid_noninterference", kind_vid_noninterference, 0},
    {"snapshot_restore_video", kind_snapshot_restore_video, 0},
    {"model_refuse", kind_model_refuse, 0},
    {"mutate_sweep", kind_mutate_sweep, 0},
    {"insn_sink_shapes", kind_insn_sink_shapes, 0},
    {"observer", kind_observer, 0},
    {"chunk_equiv", kind_chunk_equiv, 0},
    {"dev_log", kind_dev_log, 0},
    {"vid_chan_append", kind_vid_chan_append, 0},
    {"trace_out_rights", kind_trace_out_rights, 0},
    {"opcode_table", kind_opcode_table, 0},
    {"cycle_probe", kind_cycle_probe, 0},
    {"timer_pure", kind_timer_pure, 0},
    {"snapshot_hash", kind_snapshot_hash, 0},
    {"snapshot_restore", kind_snapshot_restore, 0},
    {"snapshot_refuse", kind_snapshot_refuse, 0},
    {"snapshot_hostile", kind_snapshot_hostile, 0},
    {"opcode_exercise", kind_opcode_exercise, 1},
    {"poweron_domain", kind_poweron_domain, 0},
    {"digest_stream", kind_digest_stream, 0},
    {"digest_pair", kind_digest_pair, 0},
    {"digest_region", kind_digest_region, 0},
    {"profile_hash", kind_profile_hash, 0},
    {"nonmutation_digest", kind_nonmutation_digest, 0},
    {"record_golden", kind_record_golden, 0},
    {"record_refusal", kind_record_refusal, 0},
    {"record_refuses_quote", kind_record_refuses_quote, 0},
    {"interleave", kind_interleave, 0},
    {"interleave_twins", kind_interleave_twins, 0},
    {"interleave_destroy_recreate", kind_interleave_destroy_recreate, 0},
    {"interleave_aba_order", kind_interleave_aba_order, 0},
    {"interleave_pad", kind_interleave_pad, 0},
    {"interleave_canaries", kind_interleave_canaries, 0},
    {"liveness_shared_state_selfcheck", kind_liveness_shared_state_selfcheck, 0},
    {"coverage", kind_coverage, 1},
    {"run_refusal_coverage", kind_run_refusal_coverage, 1},
    {"model_refusal_coverage", kind_model_refusal_coverage, 1},
    {"stim_refusal_coverage", kind_stim_refusal_coverage, 1},
    {"input_refusal_coverage", kind_input_refusal_coverage, 1},
    {"snapshot_refusal_coverage", kind_snapshot_refusal_coverage, 1},
    {"brk_irq_sample", kind_brk_irq_sample, 0},
    {"dev_record_pc", kind_dev_record_pc, 0},
};

static const char *outcome_of(const struct ctx *c, enum outcome *oc)
{
    if (c->harness) {
        *oc = OUT_harness_failure;
    } else if (c->timeout && !c->failed) {
        *oc = OUT_timeout;
    } else if (c->failed || c->n == 0u) {
        *oc = OUT_guest_failure;
    } else {
        *oc = OUT_pass;
    }
    return *oc == OUT_pass ? NULL : (c->why[0] != '\0' ? c->why : "no assertion was made");
}

/* run one case and print its RESULT line; returns 0, or -1 when the line could not be produced */
static int run_case(const struct kase *k, const char *evdir)
{
    struct ctx c;
    char got[64] = "";
    char evid[256];
    char tail[80] = "";
    const char *kind = k->has[K_KIND] ? k->val[K_KIND] : "-";
    const char *expect = k->has[K_EXPECT] ? k->val[K_EXPECT] : "-";
    const char *diag;
    enum outcome oc;
    size_t i;
    int j;
    int n;

    memset(&c, 0, sizeof c);
    ev_detail[0] = '\0';
    ev_classes_set = 0;
    run_evidence_dir = evdir;
    for (j = 0; j < NKEYS; j++) {
        if (!k->has[j] && ((K_OPTIONAL_MASK >> j) & 1u) == 0u && k->bad == NULL) {
            harness_error(&c, "a key is missing");
        }
    }
    if (k->bad != NULL) {
        harness_error(&c, k->bad);
    }
    if (!c.harness && strcmp(k->val[K_ID], k->stem) != 0) {
        harness_error(&c, "the id key differs from the file name");
    }
    if (!c.harness) {
        for (i = 0u; i < sizeof kinds / sizeof kinds[0]; i++) {
            if (strcmp(kinds[i].name, kind) == 0) {
                kinds[i].fn(k, &c, got, sizeof got);
                break;
            }
        }
        if (i == sizeof kinds / sizeof kinds[0]) {
            harness_error(&c, "unknown kind");
        }
    }
    n = snprintf(evid, sizeof evid, "%s/%s.txt", evdir, k->stem);
    if (n < 0 || (size_t)n >= sizeof evid || write_evidence(evid, k->stem, kind, expect, got, c.n) != 0) {
        harness_error(&c, "cannot write the evidence file");
        n = snprintf(evid, sizeof evid, "%s/%s.txt", evdir, k->stem);
        if (n < 0 || (size_t)n >= sizeof evid) {
            return -1;
        }
    }
    diag = outcome_of(&c, &oc);
    if (diag != NULL) {
        emit_err("case %s: %s\n", k->stem, diag);
    }
    if (c.has_run) {
        snprintf(tail, sizeof tail, " machine_time=%llu stop=%s", (unsigned long long)c.run_mclk, c.run_stop);
    }
    emit_out("RESULT %s %s assertions=%u evidence=%s%s\n", k->stem, outcome_name[oc], c.n, evid, tail);
    return 0;
}

int main(int argc, char **argv)
{
    const char *cases_path = NULL;
    const char *evdir = NULL;
    const char *why = NULL;
    const char *exclude_path = NULL;
    const char *only_id = NULL;
    static char excluded[MAX_CASES];
    char *text;
    size_t i;
    size_t printed = 0u;
    int pass;
    int a;
    int list_fx = 0;

    if (argc == 2 && strcmp(argv[1], "--outcomes") == 0) {
        /* the outcome names compiled into this binary from outcomes.def, one per line: scripts/outcome-sync compares them with the
         * def file itself, so a binary built from another list is caught (D-57) */
        size_t o;
        for (o = 0u; o < (size_t)OUT_COUNT; o++) {
            printf("%s\n", outcome_name[o]);
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--sweep-lengths") == 0) {
        size_t n;
        for (n = 0u; n < sizeof sweep_len / sizeof sweep_len[0]; n++) {
            printf("%lu\n", (unsigned long)sweep_len[n]);
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--spec") == 0) {
        printf("%s\n", synth_spec());
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--hash") == 0) {
        char hex[2u * DS_SHA256_BYTES + 1u];
        if (hash_recipe(argv[2], hex) != 0) {
            fprintf(stderr, "HARNESS unknown recipe\n");
            return 2;
        }
        printf("%s\n", hex);
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--emit") == 0) {
        /* the only way an image ever leaves memory: stdout, for the host hash tool; nothing is written to a file */
        size_t n;
        unsigned char *buf;
        int ok;
        if (synth_parse(argv[2], &n) != 0) {
            fprintf(stderr, "HARNESS unknown recipe\n");
            return 2;
        }
        buf = (unsigned char *)malloc(n != 0u ? n : 1u);
        if (buf == NULL || synth_fill(argv[2], buf, n) != 0) {
            free(buf);
            fprintf(stderr, "HARNESS cannot generate the recipe\n");
            return 2;
        }
        ok = fwrite(buf, 1u, n, stdout) == n && fflush(stdout) == 0;
        free(buf);
        return ok ? 0 : 2;
    }
    for (a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--exclude-ids") == 0 && a + 1 < argc) {
            exclude_path = argv[++a];
        } else if (strcmp(argv[a], "--only") == 0 && a + 1 < argc) {
            only_id = argv[++a];
        } else if (strcmp(argv[a], "--marker-seed") == 0 && a + 1 < argc) {
            size_t seed;
            if (parse_size(argv[a + 1], &seed) != 0) {
                emit_err("HARNESS --marker-seed is not a number\n");
                return 2;
            }
            set_marker_seed(seed);
            a++;
        } else if (strcmp(argv[a], "--records") == 0 && a + 1 < argc) {
            records_dir = argv[++a];
        } else if (strcmp(argv[a], "--golden-dir") == 0 && a + 1 < argc) {
            golden_dir = argv[++a];
        } else if (strcmp(argv[a], "--fixtures") == 0) {
            list_fx = 1;
        } else if (strcmp(argv[a], "--cases") == 0 && a + 1 < argc) {
            cases_path = argv[++a];
        } else if (strcmp(argv[a], "--evidence-dir") == 0 && a + 1 < argc) {
            evdir = argv[++a];
        } else if (strcmp(argv[a], "--profile-tsv") == 0 && a + 1 < argc) {
            profile_tsv_path = argv[++a];
        } else if (strcmp(argv[a], "--rules-fields") == 0 && a + 1 < argc) {
            rules_fields_path = argv[++a];
        } else if (strcmp(argv[a], "--oracle") == 0 && a + 1 < argc) {
            oracle_path = argv[++a];
        } else if (strcmp(argv[a], "--bank-oracle") == 0 && a + 1 < argc) {
            bank_oracle_path = argv[++a];
        } else if (cpukinds_arg(argc, argv, &a) || obskinds_arg(argc, argv, &a) || inputkinds_arg(argc, argv, &a)) {
            continue;
        } else {
            cases_path = NULL;
            break;
        }
    }
    if (cases_path == NULL || (evdir == NULL && !list_fx && trace_out_id == NULL)) {
        emit_err("usage: ds_runner --cases FILE --evidence-dir DIR [--profile-tsv FILE] [--rules-fields FILE]\n"
                        "                 [--oracle FILE] [--bank-oracle FILE] [--opcode-tsv FILE] [--inputs FILE] [--exclude-ids FILE] [--only ID] [--marker-seed N] [--records DIR] [--golden-dir DIR]\n"
                        "       ds_runner --cases FILE --trace-out CASE_ID   (writes build/gate/traces/CASE_ID.insn and .dev for an authored or synthetic case)\n"
                        "       ds_runner --sweep-lengths | --spec | --emit RECIPE | --hash RECIPE\n");
        return 2;
    }
    text = read_file(cases_path);
    if (text == NULL) {
        emit_err("HARNESS cannot read the cases file\n");
        return 2;
    }
    if (load_cases(text, &why) != 0) {
        emit_err("HARNESS cases file: %s\n", why);
        return 2;
    }
    for (i = 0u; i < ncases; i++) {
        if (!valid_id(cases[i].stem)) {
            emit_err("HARNESS a case file name is not an id\n");
            return 2;
        }
    }
    if (list_fx) {
        return list_fixtures(sweep_len, sizeof sweep_len / sizeof sweep_len[0]);
    }
    if (trace_out_id != NULL) {
        return trace_out_main();
    }
    if (exclude_path != NULL) {
        char *xt = read_file(exclude_path);
        char *line = xt;
        while (xt != NULL && *line != '\0') {
            char *nl = strchr(line, '\n');
            if (nl != NULL) {
                *nl = '\0';
            }
            if (*line != '\0') {
                for (i = 0u; i < ncases && strcmp(cases[i].stem, line) != 0; i++) {
                }
                if (i == ncases) {
                    free(xt);
                    emit_err("HARNESS the exclusion file names an id that no case carries\n");
                    return 2;
                }
                excluded[i] = 1;
            }
            line = nl != NULL ? nl + 1 : line + strlen(line);
        }
        if (xt == NULL) {
            emit_err("HARNESS cannot read the exclusion file\n");
            return 2;
        }
        /* the ids are consumed (excluded[] holds indexes, not pointers into the text): release the buffer, which a leak-detecting
         * sanitizer (clang with LeakSanitizer on Linux) reports as a direct leak once the pointer is out of its slot */
        free(xt);
    }
    if (only_id != NULL) {
        size_t hit = 0u;
        for (i = 0u; i < ncases; i++) {
            excluded[i] = strcmp(cases[i].stem, only_id) != 0;
            hit += excluded[i] ? 0u : 1u;
        }
        if (hit != 1u) {
            emit_err("HARNESS --only names an id that no case carries\n");
            return 2;
        }
    }
    for (pass = 0; pass < 2; pass++) {
        for (i = 0u; i < ncases; i++) {
            const struct kase *k = &cases[i];
            int deferred = 0;
            size_t j;
            if (excluded[i]) {
                continue;
            }
            for (j = 0u; j < sizeof kinds / sizeof kinds[0]; j++) {
                if (k->has[K_KIND] && strcmp(kinds[j].name, k->val[K_KIND]) == 0) {
                    deferred = kinds[j].deferred;
                }
            }
            if (deferred != pass) {
                continue;
            }
            if (run_case(k, evdir) != 0) {
                emit_err("HARNESS cannot form an evidence path\n");
                return 2;
            }
            printed++;
        }
    }
    emit_out("RUNNER_DONE %u\n", (unsigned)printed);
    return 0;
}
