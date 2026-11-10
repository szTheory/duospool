/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * runner.c - the conformance runner (D-57, D-60, D-80). ISO C only (fopen, fread, snprintf), so it builds under strict -std=c11.
 * It runs every case in ONE process and prints one line per case:
 *     RESULT <id> <outcome> assertions=<n> evidence=<relpath>
 * then  RUNNER_DONE <n>. In conformance mode stdout carries nothing else (D-80). The runner never aggregates and never decides
 * required versus optional: scripts/conformance-ingest, run by the sh gate, is the only judge. Outcome names come from
 * outcomes.def, never from string literals. Diagnostics go to stderr and never contain content bytes.
 * Tracer version (plan 05 task 1): --tracer runs two built-in cases; the TSV reader replaces it in task 2.
 * usage: ds_runner --tracer [--evidence-dir DIR]
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"

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

#define FIXTURE_BYTES 262144u
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
static const char *parse_spec(struct spec *s, const char *text)
{
    char *p;
    size_t i;

    memset(s, 0, sizeof *s);
    s->profile = "pce-hucard-2btn";
    s->rev = 1u;
    s->mapper = "plain";
    s->order = 1u;
    s->header = 1u;
    s->len = FIXTURE_BYTES;
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

/* expectation: accept[;size=N] or load.NAME[;field=NAME] */
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
                    return "expect is neither accept nor a known load.NAME";
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

/* integer LCG fill: x = x * 1664525 + 1013904223 mod 2^32, byte = (x >> 16) and 255; task 2 moves it to synth.c */
static void lcg_fill(unsigned char *p, size_t n)
{
    uint32_t x = 12345u;
    size_t i;
    for (i = 0u; i < n; i++) {
        x = x * 1664525u + 1013904223u;
        p[i] = (unsigned char)((x >> 16) & 255u);
    }
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

/* boot one case and check the outcome against the expectation; got receives the observed reason name for the evidence */
static void run_boot(const struct spec *s, const struct expect *e, struct ctx *c, char *got, size_t gotcap)
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
        lcg_fill(buf, s->len);
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
static int write_evidence(const char *path, const char *id, const char *expect, const char *got, unsigned n)
{
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }
    fprintf(fp, "id=%s\nexpect=%s\ngot=%s\nassertions=%u\n", id, expect, got, n);
    return fclose(fp) == 0 ? 0 : -1;
}

struct builtin {
    const char *id;
    const char *request;
    const char *expect;
};

static const struct builtin tracer_cases[] = {
    {"ref_profile_unknown", "profile=nope", "load.profile_unknown"},
    {"accept_tracer_256k", "", "accept;size=262144"},
};

static int run_tracer(const char *evdir)
{
    size_t i;
    size_t done = 0u;
    for (i = 0u; i < sizeof tracer_cases / sizeof tracer_cases[0]; i++) {
        const struct builtin *b = &tracer_cases[i];
        struct spec s;
        struct expect e;
        struct ctx c;
        char got[64] = "";
        char evid[256];
        const char *err;
        enum outcome oc;

        memset(&c, 0, sizeof c);
        err = parse_spec(&s, b->request);
        if (err != NULL) {
            harness_error(&c, err);
        } else if ((err = parse_expect(&e, b->expect)) != NULL) {
            harness_error(&c, err);
        } else {
            run_boot(&s, &e, &c, got, sizeof got);
        }
        if (evdir != NULL) {
            int n = snprintf(evid, sizeof evid, "%s/%s.txt", evdir, b->id);
            if (n < 0 || (size_t)n >= sizeof evid || write_evidence(evid, b->id, b->expect, got, c.n) != 0) {
                harness_error(&c, "cannot write the evidence file");
            }
        } else {
            snprintf(evid, sizeof evid, "tracer/%s.txt", b->id);
        }
        oc = c.harness ? OUT_harness_failure : (c.failed || c.n == 0u ? OUT_guest_failure : OUT_pass);
        if (oc != OUT_pass) {
            fprintf(stderr, "case %s: %s\n", b->id, c.why[0] != '\0' ? c.why : "no assertion was made");
        }
        printf("RESULT %s %s assertions=%u evidence=%s\n", b->id, outcome_name[oc], c.n, evid);
        done++;
    }
    printf("RUNNER_DONE %u\n", (unsigned)done);
    return 0;
}

int main(int argc, char **argv)
{
    const char *evdir = NULL;
    int tracer = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tracer") == 0) {
            tracer = 1;
        } else if (strcmp(argv[i], "--evidence-dir") == 0 && i + 1 < argc) {
            evdir = argv[++i];
        } else {
            tracer = 0;
            break;
        }
    }
    if (!tracer) {
        fprintf(stderr, "usage: ds_runner --tracer [--evidence-dir DIR]\n");
        return 2;
    }
    return run_tracer(evdir);
}
