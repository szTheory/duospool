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
 * aggregate over other rows and so can only be evaluated after the loop (none exist yet). RESULT lines are matched by id.
 * usage: ds_runner --cases FILE --evidence-dir DIR [--profile-tsv FILE] [--rules-fields FILE] [--oracle FILE]
 *   --profile-tsv FILE   the flattened profiles/pce-hucard-2btn/r1.toml (scripts/conformance-flatten), read by the r1 kinds
 *   --rules-fields FILE  the 13 field names of scripts/lib/profile-rules.awk, one per line, read by the names kind
 *   --oracle FILE        the bank-signature rows of tests/conformance/mirror-oracle.awk (SIZE BANK FILEBANK FIRST LAST), read by banksig
 *   --exclude-ids FILE   one case id per line: those cases are not run and print no RESULT line (RUNNER_DONE counts the lines printed).
 *                        Only scripts/conformance-run sanitize passes it, for the exact set of tests/conformance/EXCLUDED_IN_MODE (D-78);
 *                        an id that no case carries is a harness fault (exit 2), so a stale exclusion cannot linger
 *        ds_runner --outcomes   prints the compiled outcome names, one per line, and exits 0 (read by scripts/outcome-sync --names)
 *        ds_runner --sweep-lengths   prints the 21 hash sweep lengths, one per line: the only definition of the list (D-59)
 *        ds_runner --emit RECIPE     writes the bytes of a synthetic recipe to stdout, nothing else (plain-256k, len:N)
 *        ds_runner --hash RECIPE     prints the SHA-256 of a recipe's bytes computed by the core, 64 lowercase hex characters
 *        ds_runner --spec            prints the generator spec string that is part of every recipe
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

#define CONTENT_LIMIT (4u * 1024u * 1024u)
#define CANARY 0xa5u
#define CANARY_TAIL 64u


/* what a case expects: accept (with the content size read from the instance) or one refusal (with the field it names) */
struct expect {
    int accept;
    uint32_t reason;
    int has_field;
    uint32_t field;
    int has_size;
    size_t size;
};


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
static const char *parse_expect(struct expect *e, const char *text)
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
static void fill_request(struct ds_boot_request *rq, const struct spec *s, const unsigned char *content)
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
}

/* boot one case from the fixture recipe and check the outcome against the expectation; got receives the observed reason name */
static unsigned long boot_allocs; /* allocator calls made during the ds_boot call of the last run_boot (alloc_zero_matrix) */

static void run_boot(const char *fixture, const struct spec *s, const struct expect *e, struct ctx *c, char *got, size_t gotcap)
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

/* write the evidence file: ids, reason names, sizes and assertion counts only, never content bytes */
static int write_evidence(const char *path, const char *id, const char *kind, const char *expect, const char *got, unsigned n)
{
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }
    fprintf(fp, "id=%s\nkind=%s\nexpect=%s\ngot=%s\nassertions=%u\n", id, kind, expect, got, n);
    return fclose(fp) == 0 ? 0 : -1;
}

/* ---- the cases TSV (scripts/conformance-flatten): K<TAB>STEM<TAB>TABLE<TAB>KEY<TAB>TYPE<TAB>VALUE rows and a last END row ---- */


/* the ten keys of a case (D-58 nine plus the request key of D-79(a)) and the TSV type of each */
static const struct {
    const char *name;
    char type;
} key_def[NKEYS] = {
    {"id", 's'},      {"kind", 's'},    {"fixture", 's'},         {"request", 's'},  {"expect", 's'},
    {"meaning", 's'}, {"required", 'b'}, {"control", 'b'}, {"expect_outcome", 's'}, {"pins", 'a'},
};

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
            } else if (f[4][0] != key_def[key].type || f[4][1] != '\0') {
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

struct booted {
    unsigned char *mem;
    unsigned char *content;
};

static void release_boot(struct booted *b)
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
static const char *boot_accepted(const char *fixture, const char *request, struct booted *b, struct spec *s,
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
    *inst = res.instance;
    return NULL;
}

/* ---- kind record_lines: the canonical record skeleton, line by line (D-53, D-69) ---- */

#define REC_MAX_ROWS 128u
#define KEY_MAX 72u

struct recrow {
    char key[KEY_MAX]; /* TABLE.KEY */
    const char *val;   /* the text after " = ", quotes kept */
};

/* the keys the skeleton must print, in order: table.key (D-53: record, core, profile, 13 field tables, content with the sha256 first, limits) */
static char exp_key[REC_MAX_ROWS][KEY_MAX];
static size_t n_exp;

static void exp_add(const char *prefix, const char *table, const char *key)
{
    size_t a = strlen(prefix);
    size_t b = strlen(table);
    size_t d = strlen(key);

    if (n_exp < REC_MAX_ROWS && a + b + 1u + d < KEY_MAX) {
        memcpy(exp_key[n_exp], prefix, a);
        memcpy(exp_key[n_exp] + a, table, b);
        exp_key[n_exp][a + b] = '.';
        memcpy(exp_key[n_exp] + a + b + 1u, key, d + 1u);
        n_exp++;
    }
}

static void build_expected_keys(void)
{
    uint32_t i;

    n_exp = 0u;
    exp_add("", "record", "format_version");
    exp_add("", "core", "build_id");
    exp_add("", "core", "behaviour_epoch");
    exp_add("", "profile", "id");
    exp_add("", "profile", "rev");
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        char tab[KEY_MAX];
        size_t n = strlen(ds_field_name(i));

        if (n + 7u >= sizeof tab) {
            break;
        }
        memcpy(tab, "field_", 6u);
        memcpy(tab + 6, ds_field_name(i), n + 1u);
        exp_add("", tab, "status");
        exp_add("", tab, "value");
        exp_add("", tab, "bound_by");
    }
    exp_add("", "content", "sha256");
    exp_add("", "content", "size");
    exp_add("", "content", "mapper");
    exp_add("", "content", "mapper_rule");
    exp_add("", "content", "byte_order");
    exp_add("", "content", "header_handling");
    exp_add("", "limits", "size_cap");
}

/*
 * Split canonical text (in place) into TABLE.KEY rows. Layout rules: every line ends in LF, a table header is `[name]`, a key line
 * is `key = value` inside a table, one blank line sits before every table but the first, and nowhere else. NULL on success,
 * otherwise the first fault.
 */
static const char *parse_record(char *text, struct recrow *rows, size_t *nrows)
{
    char table[48];
    int have_table = 0;
    int prev_blank = 0;
    int first_line = 1;
    char *line = text;

    table[0] = '\0';
    *nrows = 0u;
    while (*line != '\0') {
        char *nl = strchr(line, '\n');
        size_t n;
        if (nl == NULL) {
            return "the text does not end in a newline";
        }
        *nl = '\0';
        n = strlen(line);
        if (n == 0u) {
            if (prev_blank || first_line) {
                return "a blank line where none belongs";
            }
            prev_blank = 1;
        } else if (line[0] == '[') {
            if (line[n - 1u] != ']' || n < 3u || n - 2u >= sizeof table) {
                return "a malformed table header";
            }
            if (!first_line && !prev_blank) {
                return "a table header without a blank line before it";
            }
            memcpy(table, line + 1, n - 2u);
            table[n - 2u] = '\0';
            have_table = 1;
            prev_blank = 0;
        } else {
            char *eq = strstr(line, " = ");
            size_t tl = strlen(table);
            if (eq == NULL || !have_table || prev_blank) {
                return "a key line outside a table or after a blank line";
            }
            *eq = '\0';
            if (*nrows == REC_MAX_ROWS || tl + 1u + strlen(line) >= KEY_MAX) {
                return "too many rows or a key too long";
            }
            memcpy(rows[*nrows].key, table, tl);
            rows[*nrows].key[tl] = '.';
            memcpy(rows[*nrows].key + tl + 1u, line, strlen(line) + 1u);
            rows[*nrows].val = eq + 3;
            (*nrows)++;
            prev_blank = 0;
        }
        first_line = 0;
        line = nl + 1;
    }
    if (prev_blank) {
        return "a blank line at the end";
    }
    return NULL;
}

/* is the raw value text the string want: a quoted string without its quotes, or a bare integer */
static int value_is(const char *raw, const char *want)
{
    size_t n = strlen(raw);

    if (n >= 2u && raw[0] == '"' && raw[n - 1u] == '"') {
        return strlen(want) == n - 2u && memcmp(raw + 1, want, n - 2u) == 0;
    }
    return strcmp(raw, want) == 0;
}

static const struct recrow *find_row(const struct recrow *rows, size_t n, const char *key)
{
    size_t i;

    for (i = 0u; i < n; i++) {
        if (strcmp(rows[i].key, key) == 0) {
            return &rows[i];
        }
    }
    return NULL;
}

/* does the printer refuse (error status, empty text, zero length) the instance copy as it stands; garbage is pre-filled */
static int refuses_with(const struct ds_instance *t, enum ds_text_status want, char *buf, size_t cap)
{
    size_t len = 12345u;
    enum ds_text_status st;

    memset(buf, 0x7f, cap);
    st = ds_text_record(t, buf, cap, &len);
    return st == want && len == 0u && buf[0] == '\0';
}

/*
 * kind record_lines: boot the request (an accepted one), print the record into a buffer (nothing goes to stdout, D-80) and require
 *   the exact table and key order of the skeleton, LF and printable ASCII only, one blank line between tables and nowhere else,
 *   the scalar lines listed in the case expect (TABLE.KEY=VALUE separated by semicolons), the status, value and binder of each of
 *   the 13 field tables from the built-in profile and the request, the profile text as an exact part of the record, and, as
 *   controls on the printer, the refusal of a double quote, a backslash, a non-ASCII byte, an unknown enumeration and a buffer
 *   that is one byte short.
 */
static void kind_record_lines(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    static char work[REC_CAP];
    static char ptext[REC_CAP];
    static char again[REC_CAP];
    static char scratch[REC_CAP];
    struct recrow rows[REC_MAX_ROWS];
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const struct ds_profile *builtin;
    struct ds_instance *tmp;
    char want[160];
    const char *p;
    size_t len = 0u;
    size_t plen = 0u;
    size_t nrows = 0u;
    size_t i;
    uint32_t f;
    const char *err;
    enum ds_text_status st;

    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    builtin = ds_profile_builtin(inst->profile.id, inst->profile.rev);
    check(c, builtin != NULL, "the booted profile is not the built-in r1");
    if (builtin == NULL) {
        release_boot(&b);
        return;
    }
    st = ds_text_record(inst, text, sizeof text, &len);
    check(c, st == DS_TEXT_OK, "ds_text_record failed on an accepted instance");
    if (st != DS_TEXT_OK) {
        release_boot(&b);
        return;
    }
    check(c, len == strlen(text) && len > 0u, "outlen differs from the length of the text");
    for (i = 0u; i < len; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (!(ch == 0x0au || (ch >= 0x20u && ch <= 0x7eu))) {
            break;
        }
    }
    check(c, i == len, "the record holds a byte that is neither LF nor printable ASCII (CR, TAB and non-ASCII are refused)");
    check(c, strchr(text, '\\') == NULL, "the record holds a backslash");

    memcpy(work, text, len + 1u);
    err = parse_record(work, rows, &nrows);
    check(c, err == NULL, err != NULL ? err : "layout");
    build_expected_keys();
    check(c, n_exp == 51u, "the expected skeleton does not hold 51 rows (format_version, 2 core, 2 profile, 39 field, 6 content, 1 limit)");
    check(c, err == NULL && nrows == n_exp, "the number of rows differs from the skeleton");
    for (i = 0u; err == NULL && i < nrows && i < n_exp; i++) {
        if (strcmp(rows[i].key, exp_key[i]) != 0) {
            break;
        }
    }
    check(c, err == NULL && i == n_exp, "a row is missing, extra or out of order against the skeleton");

    /* the scalar lines of the case */
    for (p = k->val[K_EXPECT]; err == NULL && !c->harness && *p != '\0';) {
        const char *end = strchr(p, ';');
        size_t n = end != NULL ? (size_t)(end - p) : strlen(p);
        char *eq;
        const struct recrow *row;
        if (n == 0u || n >= sizeof want) {
            harness_error(c, "an expect item is empty or too long");
            break;
        }
        memcpy(want, p, n);
        want[n] = '\0';
        eq = strchr(want, '=');
        if (eq == NULL) {
            harness_error(c, "an expect item has no =");
            break;
        }
        *eq = '\0';
        row = find_row(rows, nrows, want);
        check(c, row != NULL && value_is(row->val, eq + 1), "a scalar line of the record differs from the case expectation");
        p += n + (end != NULL ? 1u : 0u);
    }

    /* every field table: status, value and binder follow from the built-in profile and the request */
    for (f = 0u; f < (uint32_t)DS_FIELD_COUNT && nrows == n_exp; f++) {
        char key[KEY_MAX];
        const char *name = ds_field_name(f);
        int open = builtin->field[f].status == (uint8_t)DS_STATUS_OPEN;
        const struct recrow *rs;
        const struct recrow *rv;
        const struct recrow *rb;

        snprintf(key, sizeof key, "field_%s.status", name);
        rs = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "field_%s.value", name);
        rv = find_row(rows, nrows, key);
        snprintf(key, sizeof key, "field_%s.bound_by", name);
        rb = find_row(rows, nrows, key);
        check(c, rs != NULL && value_is(rs->val, open ? "open" : "owner-selected"), "a field status line differs");
        check(c, rv != NULL && value_is(rv->val, open ? (s.bind[f] != NULL ? s.bind[f] : "") : builtin->field[f].value),
              "a field value line differs from the constant or the bound value");
        check(c, rb != NULL && value_is(rb->val, open ? "run-request" : "constant"), "a field bound_by line differs");
    }

    /* the profile text is an exact part of the record, followed by the content table */
    st = ds_text_profile(&inst->profile, ptext, sizeof ptext, &plen);
    check(c, st == DS_TEXT_OK && plen == strlen(ptext), "ds_text_profile failed");
    if (st == DS_TEXT_OK) {
        const char *at = strstr(text, ptext);
        check(c, at != NULL && at >= text + 2 && at[-1] == '\n' && at[-2] == '\n' && strncmp(at + plen, "\n[content]\n", 11u) == 0,
              "the profile text is not an exact part of the record, after a blank line and followed by the content table");
        check(c, strncmp(ptext, "[profile]\n", 10u) == 0, "the profile text does not start with the profile table");
    }

    /* printing twice gives the same bytes */
    st = ds_text_record(inst, again, sizeof again, &plen);
    check(c, st == DS_TEXT_OK && plen == len && memcmp(again, text, len + 1u) == 0, "a second print differs from the first");

    /* the capacity edge: len + 1 bytes fit, len bytes do not, and the failed print leaves nothing */
    st = ds_text_record(inst, scratch, len + 1u, &plen);
    check(c, st == DS_TEXT_OK && plen == len && memcmp(scratch, text, len + 1u) == 0, "a buffer of exactly len + 1 bytes did not fit the record");
    check(c, refuses_with(inst, DS_TEXT_ERR_CAPACITY, scratch, len), "a buffer one byte short did not give capacity and an empty text");
    plen = 99u;
    check(c, ds_text_record(inst, NULL, 0u, &plen) == DS_TEXT_ERR_ARGUMENT && plen == 0u, "a NULL buffer is not an argument error");

    /* D-69: a quote, a backslash or a byte outside printable ASCII is refused, never escaped */
    tmp = (struct ds_instance *)malloc(sizeof *tmp);
    if (tmp == NULL) {
        harness_error(c, "out of memory for the instance copy");
    } else {
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_media_slot].value[0] = '"';
        check(c, refuses_with(tmp, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a double quote in a field value was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_lines_per_frame].value[0] = '\\';
        check(c, refuses_with(tmp, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a backslash in a field value was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->mapper[1] = '"';
        check(c, refuses_with(tmp, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a double quote in the mapper name was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.id[2] = '\\';
        check(c, refuses_with(tmp, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a backslash in the profile id was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_vpc].value[0] = (char)0x80;
        check(c, refuses_with(tmp, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "a non-ASCII byte in a field value was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        memset(tmp->mapper, 'x', sizeof tmp->mapper);
        check(c, refuses_with(tmp, DS_TEXT_ERR_STRING, scratch, sizeof scratch), "an unterminated mapper name was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->profile.field[DS_FIELD_chip_revision].status = 9u;
        check(c, refuses_with(tmp, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an unknown field status was not refused");
        memcpy(tmp, inst, sizeof *tmp);
        tmp->byte_order = 7u;
        check(c, refuses_with(tmp, DS_TEXT_ERR_STATE, scratch, sizeof scratch), "an unknown byte order was not refused");
        free(tmp);
    }
    snprintf(got, gotcap, "rows=%u", (unsigned)nrows);
    release_boot(&b);
}

/* ---- the content identity (D-47): hex form, the sweep list and the kind content_id ---- */

/*
 * The lengths of the hash sweep: 0 and 1, the padding boundaries of SHA-256 (55, 56, 57 around the length field, 63, 64, 65 at the
 * block edge, and the same pair of edges one and two blocks on), the bank-size neighbours, and the three supported image sizes.
 * This array is the ONE definition: scripts/conformance-run reads it through --sweep-lengths and plan 17 builds the fixture
 * manifest from the same output, so the two can never disagree.
 */
static const size_t sweep_len[] = {0u,   1u,    54u,   55u,   56u,   57u,   63u,    64u,     65u,      119u,    120u,
                                   121u, 127u,  128u,  129u,  8191u, 8192u, 8193u,  262144u, 393216u, 1048576u};

/* lowercase hex of n bytes into out (2 * n + 1 bytes) */
static void to_hex(const unsigned char *bytes, size_t n, char *out)
{
    static const char digit[] = "0123456789abcdef";
    size_t i;

    for (i = 0u; i < n; i++) {
        out[2u * i] = digit[(bytes[i] >> 4) & 0x0fu];
        out[2u * i + 1u] = digit[bytes[i] & 0x0fu];
    }
    out[2u * n] = '\0';
}

/* 64 characters of 0-9 a-f and nothing else: the one accepted spelling of a digest */
static int is_hex64(const char *s)
{
    size_t i;

    for (i = 0u; i < 64u; i++) {
        if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) {
            return 0;
        }
    }
    return s[64] == '\0';
}

/* SHA-256 of a recipe's bytes through the core, as hex; 0 on success, -1 when the recipe is unknown or memory is short */
static int hash_recipe(const char *recipe, char *hex)
{
    size_t n;
    unsigned char *buf;
    unsigned char digest[DS_SHA256_BYTES];

    if (synth_parse(recipe, &n) != 0) {
        return -1;
    }
    buf = (unsigned char *)malloc(n != 0u ? n : 1u);
    if (buf == NULL) {
        return -1;
    }
    if (synth_fill(recipe, buf, n) != 0) {
        free(buf);
        return -1;
    }
    ds_sha256(buf, n, digest);
    to_hex(digest, DS_SHA256_BYTES, hex);
    free(buf);
    return 0;
}

/*
 * kind content_id: boot the request (an accepted one) over the fixture and require that the content id stored by ds_boot, the
 * SHA-256 the core computes over a FRESH copy of the recipe bytes, and the content.sha256 line of the printed record are one and
 * the same value, 64 lowercase hex characters, and that it is the digest of the bytes as supplied (a single flipped bit gives
 * another digest). This is a self-consistency case: the independent comparison with a tool that shares nothing with the core is
 * the sweep in scripts/conformance-run (D-59). A matching hash shows repeatability, not that the content is genuine.
 */
static void kind_content_id(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static char text[REC_CAP];
    struct recrow rows[REC_MAX_ROWS];
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const struct recrow *row;
    unsigned char *fresh;
    unsigned char digest[DS_SHA256_BYTES];
    unsigned char flipped[DS_SHA256_BYTES];
    char want[2u * DS_SHA256_BYTES + 1u];
    char have[2u * DS_SHA256_BYTES + 1u];
    size_t len = 0u;
    size_t nrows = 0u;
    size_t i;
    const char *err;

    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    fresh = (unsigned char *)malloc(s.len != 0u ? s.len : 1u);
    if (fresh == NULL || synth_fill(k->val[K_FIXTURE], fresh, s.len) != 0) {
        free(fresh);
        release_boot(&b);
        harness_error(c, "cannot regenerate the recipe bytes");
        return;
    }
    ds_sha256(fresh, s.len, digest);
    to_hex(digest, DS_SHA256_BYTES, want);
    to_hex(inst->content_id, DS_SHA256_BYTES, have);
    check(c, strcmp(want, have) == 0, "the content id stored by ds_boot differs from the SHA-256 of the supplied bytes");
    check(c, is_hex64(have), "the content id is not 64 lowercase hex characters");
    for (i = 0u; i < DS_SHA256_BYTES && inst->content_id[i] == 0u; i++) {
        /* find the first non-zero byte */
    }
    check(c, i < DS_SHA256_BYTES, "the content id is all zero: the placeholder slot was never filled");
    if (s.len != 0u) {
        fresh[s.len / 2u] = (unsigned char)(fresh[s.len / 2u] ^ 1u);
        ds_sha256(fresh, s.len, flipped);
        check(c, memcmp(flipped, digest, DS_SHA256_BYTES) != 0, "a flipped bit of the content leaves the digest unchanged");
    }
    snprintf(got, gotcap, "%.16s", have);

    check(c, ds_text_record(inst, text, sizeof text, &len) == DS_TEXT_OK, "ds_text_record failed on an accepted instance");
    if (len != 0u) {
        err = parse_record(text, rows, &nrows);
        check(c, err == NULL, err != NULL ? err : "layout");
        row = find_row(rows, nrows, "content.sha256");
        check(c, row != NULL && strlen(row->val) == 66u && row->val[0] == '"' && row->val[65] == '"' && strncmp(row->val + 1, want, 64u) == 0,
              "the content.sha256 line of the record differs from the content id");
    }
    free(fresh);
    release_boot(&b);
}

/*
 * kind hash_literal: the request names a NIST FIPS 180-4 example input that is built here, in memory (no file is read), and the
 * expect is the published digest as 64 lowercase hex characters. The input is fed to the core's update in uneven chunks of 1 to 7
 * bytes (so the internal buffering is exercised at every offset) and again in one call; both digests must equal the literal.
 * Inputs: empty, abc, 448bit (the 56-byte message that forces a second padding block), million-a (one million letters a).
 */
static void kind_hash_literal(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char msg_448[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const char *name = k->val[K_REQUEST];
    const char *literal = k->val[K_EXPECT];
    unsigned char *buf = NULL;
    size_t n = 0u;
    size_t off = 0u;
    size_t step = 1u;
    struct ds_sha256 st;
    unsigned char digest[DS_SHA256_BYTES];
    char hex[2u * DS_SHA256_BYTES + 1u];

    if (strcmp(k->val[K_FIXTURE], "none") != 0) {
        harness_error(c, "a hash_literal case has no content fixture: its fixture must be none");
        return;
    }
    if (!is_hex64(literal)) {
        harness_error(c, "the expect of a hash_literal case is not 64 lowercase hex characters");
        return;
    }
    if (strcmp(name, "empty") == 0) {
        n = 0u;
    } else if (strcmp(name, "abc") == 0) {
        n = 3u;
    } else if (strcmp(name, "448bit") == 0) {
        n = sizeof msg_448 - 1u;
    } else if (strcmp(name, "million-a") == 0) {
        n = 1000000u;
    } else {
        harness_error(c, "unknown hash_literal input");
        return;
    }
    buf = (unsigned char *)malloc(n != 0u ? n : 1u);
    if (buf == NULL) {
        harness_error(c, "out of memory for the hash input");
        return;
    }
    if (strcmp(name, "abc") == 0) {
        memcpy(buf, "abc", n);
    } else if (strcmp(name, "448bit") == 0) {
        memcpy(buf, msg_448, n);
    } else if (n != 0u) {
        memset(buf, 'a', n);
    }
    check(c, strcmp(name, "448bit") != 0 || n * 8u == 448u, "the 448bit input is not 448 bits long");
    check(c, strcmp(name, "million-a") != 0 || n == 1000000u, "the million-a input is not one million bytes long");

    ds_sha256_init(&st);
    while (off < n) {
        size_t take = n - off < step ? n - off : step;
        ds_sha256_update(&st, buf + off, take);
        off += take;
        step = step % 7u + 1u;
    }
    ds_sha256_final(&st, digest);
    to_hex(digest, DS_SHA256_BYTES, hex);
    check(c, strcmp(hex, literal) == 0, "the digest fed in uneven chunks differs from the NIST literal");
    snprintf(got, gotcap, "%.16s", hex);
    ds_sha256(buf, n, digest);
    to_hex(digest, DS_SHA256_BYTES, hex);
    check(c, strcmp(hex, literal) == 0, "the one-call digest differs from the NIST literal");
    free(buf);
}

/*
 * kind banksig (D-44, D-59): boot the request over the fixture and read byte 0 and byte 8191 of every bank 0 to 127 of the window
 * through ds_mapper_read; each must be an OK read equal to the FIRST and LAST column of the oracle row of that bank. The rows
 * come from --oracle FILE, written by scripts/conformance-run from tests/conformance/mirror-oracle.awk, which shares no code or
 * table with the core or with the generator; the size selecting the rows is the natural size of the fixture recipe, not anything
 * the core reports. Assertions: two per bank, 256 for the 128 banks.
 */
#define ORACLE_BANKS 128u

static const char *oracle_path;

/* the rows of one size (KiB) of the oracle TSV into first/last by bank; NULL on success, otherwise why not */
static const char *load_oracle(size_t kib, unsigned *first, unsigned *last)
{
    char *text = oracle_path != NULL ? read_file(oracle_path) : NULL;
    char *line;
    unsigned seen[ORACLE_BANKS];
    size_t i;
    const char *err = NULL;

    if (text == NULL) {
        return "no oracle file (--oracle FILE) or it cannot be read";
    }
    memset(seen, 0, sizeof seen);
    for (line = text; *line != '\0' && err == NULL;) {
        char *nl = strchr(line, '\n');
        char *f[5];
        size_t sz;
        size_t bank;
        size_t fb;
        size_t fi;
        size_t la;

        if (nl != NULL) {
            *nl = '\0';
        }
        if (split_tab(line, f, 5) != 5 || parse_size(f[0], &sz) != 0 || parse_size(f[1], &bank) != 0 ||
            parse_size(f[2], &fb) != 0 || parse_size(f[3], &fi) != 0 || parse_size(f[4], &la) != 0 || fi > 255u || la > 255u) {
            err = "a malformed oracle row";
        } else if (sz == kib) {
            if (bank >= ORACLE_BANKS || seen[bank] != 0u) {
                err = "an oracle bank is out of range or repeated";
            } else {
                seen[bank] = 1u;
                first[bank] = (unsigned)fi;
                last[bank] = (unsigned)la;
            }
        }
        line = nl != NULL ? nl + 1 : line + strlen(line);
    }
    for (i = 0u; err == NULL && i < ORACLE_BANKS; i++) {
        if (seen[i] == 0u) {
            err = "the oracle has no row for a bank of this size";
        }
    }
    free(text);
    return err;
}

static void kind_banksig(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    unsigned first[ORACLE_BANKS];
    unsigned last[ORACLE_BANKS];
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    size_t natural = synth_size(k->val[K_FIXTURE]);
    const char *err;
    uint32_t bank;

    err = natural % 1024u == 0u && natural != 0u ? load_oracle(natural / 1024u, first, last) : "the fixture has no size in whole KiB";
    if (err == NULL) {
        err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    for (bank = 0u; bank < ORACLE_BANKS; bank++) {
        uint8_t lo = 0u;
        uint8_t hi = 0u;
        uint32_t st_lo = ds_mapper_read(inst, bank, 0u, &lo);
        uint32_t st_hi = ds_mapper_read(inst, bank, DS_BANK_BYTES - 1u, &hi);

        check(c, st_lo == (uint32_t)DS_MAP_OK && (unsigned)lo == first[bank], "the first byte of a bank differs from the oracle");
        check(c, st_hi == (uint32_t)DS_MAP_OK && (unsigned)hi == last[bank], "the last byte of a bank differs from the oracle");
    }
    snprintf(got, gotcap, "banks=%u", (unsigned)ORACLE_BANKS);
    release_boot(&b);
}

/*
 * kind unmapped (D-44, T-02-11): for each of the three sizes (the recipes plain-256k, plain-384k, plain-1m, each booted with the
 * default request) every bank 0x80 to 0xFF must give DS_MAP_UNMAPPED, bank 0xFFFFFFFF must too, an offset of 8192 and the largest
 * offset in a mapped bank must give DS_MAP_BAD_OFFSET, and none of these statuses may write the output byte (it keeps its
 * sentinel), which also shows that the ROM array was not read. The case names no value for an unmapped read: Phase 3 owns it.
 */
static void kind_unmapped(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const recipe[3] = {"plain-256k", "plain-384k", "plain-1m"};
    size_t r;

    (void)k;
    for (r = 0u; r < 3u; r++) {
        struct booted b;
        struct spec s;
        const struct ds_instance *inst = NULL;
        const char *err = boot_accepted(recipe[r], "", &b, &s, &inst);
        uint32_t bank;
        uint8_t v;

        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        for (bank = DS_BANK_LIMIT; bank <= 0xFFu; bank++) {
            v = 0x5au;
            check(c, ds_mapper_read(inst, bank, 0u, &v) == (uint32_t)DS_MAP_UNMAPPED && v == 0x5au,
                  "a bank of 0x80 or above is not unmapped");
        }
        v = 0x5au;
        check(c, ds_mapper_read(inst, 0xFFFFFFFFu, 0u, &v) == (uint32_t)DS_MAP_UNMAPPED && v == 0x5au,
              "bank 0xFFFFFFFF is not unmapped");
        v = 0x5au;
        check(c, ds_mapper_read(inst, 0u, DS_BANK_BYTES, &v) == (uint32_t)DS_MAP_BAD_OFFSET && v == 0x5au,
              "offset 8192 is not a bad offset");
        v = 0x5au;
        check(c, ds_mapper_read(inst, DS_BANK_LIMIT - 1u, 0xFFFFFFFFu, &v) == (uint32_t)DS_MAP_BAD_OFFSET && v == 0x5au,
              "the largest offset is not a bad offset");
        release_boot(&b);
    }
    snprintf(got, gotcap, "sizes=3");
}

static const struct {
    const char *name;
    kind_fn fn;
    int deferred; /* evaluated in pass two, after the loop (D-86) */
} kinds[] = {
    {"refuse", kind_boot, 0},
    {"accept", kind_boot, 0},
    {"resolve", kind_resolve, 0},
    {"record_lines", kind_record_lines, 0},
    {"profile_equal", kind_profile_equal, 0},
    {"names_both_ways", kind_names_both_ways, 0},
    {"field_flip", kind_field_flip, 0},
    {"content_id", kind_content_id, 0},
    {"hash_literal", kind_hash_literal, 0},
    {"banksig", kind_banksig, 0},
    {"unmapped", kind_unmapped, 0},
    {"alloc_zero", kind_alloc_zero, 0},
};

static const char *outcome_of(const struct ctx *c, enum outcome *oc)
{
    if (c->harness) {
        *oc = OUT_harness_failure;
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
    const char *kind = k->has[K_KIND] ? k->val[K_KIND] : "-";
    const char *expect = k->has[K_EXPECT] ? k->val[K_EXPECT] : "-";
    const char *diag;
    enum outcome oc;
    size_t i;
    int j;
    int n;

    memset(&c, 0, sizeof c);
    for (j = 0; j < NKEYS; j++) {
        if (!k->has[j] && k->bad == NULL) {
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
        fprintf(stderr, "case %s: %s\n", k->stem, diag);
    }
    printf("RESULT %s %s assertions=%u evidence=%s\n", k->stem, outcome_name[oc], c.n, evid);
    fflush(stdout);
    return 0;
}

int main(int argc, char **argv)
{
    const char *cases_path = NULL;
    const char *evdir = NULL;
    const char *why = NULL;
    const char *exclude_path = NULL;
    static char excluded[MAX_CASES];
    char *text;
    size_t i;
    size_t printed = 0u;
    int pass;
    int a;

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
        } else {
            cases_path = NULL;
            break;
        }
    }
    if (cases_path == NULL || evdir == NULL) {
        fprintf(stderr, "usage: ds_runner --cases FILE --evidence-dir DIR [--profile-tsv FILE] [--rules-fields FILE]\n"
                        "                 [--oracle FILE] [--exclude-ids FILE]\n"
                        "       ds_runner --sweep-lengths | --spec | --emit RECIPE | --hash RECIPE\n");
        return 2;
    }
    text = read_file(cases_path);
    if (text == NULL) {
        fprintf(stderr, "HARNESS cannot read the cases file\n");
        return 2;
    }
    if (load_cases(text, &why) != 0) {
        fprintf(stderr, "HARNESS cases file: %s\n", why);
        return 2;
    }
    for (i = 0u; i < ncases; i++) {
        if (!valid_id(cases[i].stem)) {
            fprintf(stderr, "HARNESS a case file name is not an id\n");
            return 2;
        }
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
                    fprintf(stderr, "HARNESS the exclusion file names an id that no case carries\n");
                    return 2;
                }
                excluded[i] = 1;
            }
            line = nl != NULL ? nl + 1 : line + strlen(line);
        }
        if (xt == NULL) {
            fprintf(stderr, "HARNESS cannot read the exclusion file\n");
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
                fprintf(stderr, "HARNESS cannot form an evidence path\n");
                return 2;
            }
            printed++;
        }
    }
    printf("RUNNER_DONE %u\n", (unsigned)printed);
    return 0;
}
