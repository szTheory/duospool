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
 * usage: ds_runner --cases FILE --evidence-dir DIR [--profile-tsv FILE]
 *   --profile-tsv FILE is accepted and not read yet: plan 06 uses it for the r1 equality kinds.
 * exit : 0 every case ran and RUNNER_DONE was printed (the verdict is the ingest's), 2 usage or harness error (no case ran)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
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

/* what a case expects: accept (with the content size read from the instance) or one refusal (with the field it names) */
struct expect {
    int accept;
    uint32_t reason;
    int has_field;
    uint32_t field;
    int has_size;
    size_t size;
};

/* assertion counter: a case passes only when it made at least one assertion and none failed */
struct ctx {
    unsigned n;
    int failed;
    int harness;
    char why[160];
};

static void check(struct ctx *c, int cond, const char *what)
{
    c->n++;
    if (!cond && !c->failed) {
        c->failed = 1;
        snprintf(c->why, sizeof c->why, "%s", what);
    }
}

static void harness_error(struct ctx *c, const char *what)
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

static int parse_size(const char *v, size_t *out)
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
static const char *parse_spec(struct spec *s, const char *text, size_t default_len)
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

/* boot one case from the fixture recipe and check the outcome against the expectation; got receives the observed reason name */
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
    uint32_t i;

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
    memset(&rq, 0, sizeof rq);
    rq.struct_size = (uint32_t)sizeof rq + (s->bad_size ? 1u : 0u);
    rq.reserved = s->reserved;
    rq.profile_id = s->profile;
    rq.profile_rev = s->rev;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        rq.bindings.value[i] = s->bind[i];
    }
    rq.mapper = s->mapper;
    rq.byte_order = s->order;
    rq.header = s->header;
    rq.content = cptr;
    rq.len = s->len;
    memset(&res, 0xff, sizeof res);
    mem = s->mem == MEM_NULL ? NULL : (s->mem == MEM_MISALIGNED ? (void *)(base + 1) : (void *)base);
    cap = s->cap == CAP_SHORT ? isize - 1u : isize;

    ret = ds_boot(mem, cap, s->req == REQ_NULL ? NULL : &rq, &res);

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
                check(c, !e->has_size || (size_t)inst->content_size == e->size, "content size read from the instance differs");
                check(c, buf == NULL || memcmp(inst->rom, buf, s->len) == 0, "the ROM copy differs from the supplied bytes");
                check(c, strcmp(inst->profile.id, "pce-hucard-2btn") == 0 && inst->profile.rev == 1u,
                      "the resolved profile is not recorded");
                check(c, inst->machine_time == 0u, "machine time is not zero at power-on");
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

enum { K_ID, K_KIND, K_FIXTURE, K_REQUEST, K_EXPECT, K_MEANING, K_REQUIRED, K_CONTROL, K_EXPECT_OUTCOME, K_PINS, NKEYS };

/* the ten keys of a case (D-58 nine plus the request key of D-79(a)) and the TSV type of each */
static const struct {
    const char *name;
    char type;
} key_def[NKEYS] = {
    {"id", 's'},      {"kind", 's'},    {"fixture", 's'},         {"request", 's'},  {"expect", 's'},
    {"meaning", 's'}, {"required", 'b'}, {"control", 'b'}, {"expect_outcome", 's'}, {"pins", 'a'},
};

#define MAX_CASES 1024u

struct kase {
    const char *stem;
    const char *val[NKEYS];
    int has[NKEYS];
    const char *bad; /* the first structural fault of the case, NULL when none */
};

static struct kase cases[MAX_CASES];
static size_t ncases;

/* read a whole file into a NUL-terminated heap buffer; NULL on any failure */
static char *read_file(const char *path)
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
static int split_tab(char *line, char **f, int max)
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

static const struct {
    const char *name;
    kind_fn fn;
    int deferred; /* evaluated in pass two, after the loop (D-86) */
} kinds[] = {
    {"refuse", kind_boot, 0},
    {"accept", kind_boot, 0},
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
    char *text;
    size_t i;
    size_t printed = 0u;
    int pass;
    int a;

    for (a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--cases") == 0 && a + 1 < argc) {
            cases_path = argv[++a];
        } else if (strcmp(argv[a], "--evidence-dir") == 0 && a + 1 < argc) {
            evdir = argv[++a];
        } else if (strcmp(argv[a], "--profile-tsv") == 0 && a + 1 < argc) {
            a++; /* reserved for plan 06 */
        } else {
            cases_path = NULL;
            break;
        }
    }
    if (cases_path == NULL || evdir == NULL) {
        fprintf(stderr, "usage: ds_runner --cases FILE --evidence-dir DIR [--profile-tsv FILE]\n");
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
    for (pass = 0; pass < 2; pass++) {
        for (i = 0u; i < ncases; i++) {
            const struct kase *k = &cases[i];
            int deferred = 0;
            size_t j;
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
