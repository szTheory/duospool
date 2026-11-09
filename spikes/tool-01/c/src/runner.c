/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */

/*
 * runner.c - the manifest runner of the C skeleton (protocol W-08, W-09).
 * usage: duospool-runner <fixtures.manifest> <out-dir>
 * It executes every "case" row of the manifest through the public duospool_* API only and prints one transcript line
 * per case, "<id> <observed> PASS|FAIL|CONTROL-REJECTED|CONTROL-BROKEN", then
 *   RUNNER pass=<n> fail=<m> controls_rejected=<k>
 * and exits non-zero when any case FAILs or a control is CONTROL-BROKEN.
 */
#include "duospool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CASES 64
#define FIELD 160
#define CONTENT 65536u

struct manifest_case {
    char id[FIELD];
    char kind[FIELD];
    char arg[FIELD];
    char expect[FIELD];
    int control;
};

static const char *status_name(ds_status st) {
    switch (st) {
    case DS_OK: return "OK";
    case DS_REFUSE_EMPTY: return "REFUSE_EMPTY";
    case DS_REFUSE_TRUNCATED: return "REFUSE_TRUNCATED";
    case DS_REFUSE_BAD_SIZE: return "REFUSE_BAD_SIZE";
    case DS_REFUSE_OVERSIZED: return "REFUSE_OVERSIZED";
    case DS_REFUSE_HEADER: return "REFUSE_HEADER";
    case DS_ERR_NULL_ARG: return "ERR_NULL_ARG";
    case DS_ERR_SHORT_BUFFER: return "ERR_SHORT_BUFFER";
    case DS_ERR_STATE: return "ERR_STATE";
    }
    return "ERR_UNKNOWN";
}

/* Read a whole fixture. Returns 0 on success; *buf is malloc'd (never NULL on success) and *len is the file size. */
static int read_fixture(const char *dir, const char *name, uint8_t **buf, size_t *len) {
    char path[512];
    FILE *fp;
    long size;
    size_t got;
    int n = snprintf(path, sizeof path, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= sizeof path) {
        return 1;
    }
    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 1;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 1;
    }
    *buf = (uint8_t *)malloc((size_t)size + 1u);
    if (*buf == NULL) {
        fclose(fp);
        return 1;
    }
    got = fread(*buf, 1u, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) {
        free(*buf);
        *buf = NULL;
        return 1;
    }
    *len = got;
    return 0;
}

/* load <fixture> into a fresh instance and report the status name. */
static void run_load(const char *dir, const char *fixture, char *observed, size_t cap) {
    uint8_t *buf = NULL;
    size_t len = 0;
    ds_instance *inst = NULL;
    if (read_fixture(dir, fixture, &buf, &len) != 0) {
        snprintf(observed, cap, "FIXTURE_UNREADABLE");
        return;
    }
    if (duospool_create(&inst) != DS_OK) {
        snprintf(observed, cap, "CREATE_FAILED");
    } else {
        snprintf(observed, cap, "%s", status_name(duospool_load(inst, buf, len)));
    }
    duospool_destroy(inst);
    free(buf);
}

static void run_api_null(char *observed, size_t cap) {
    ds_instance *inst = NULL;
    if (duospool_create(&inst) != DS_OK) {
        snprintf(observed, cap, "CREATE_FAILED");
        return;
    }
    snprintf(observed, cap, "%s", status_name(duospool_load(inst, NULL, CONTENT)));
    duospool_destroy(inst);
}

static void run_api_short(char *observed, size_t cap) {
    ds_instance *inst = NULL;
    char out[16];
    if (duospool_create(&inst) != DS_OK) {
        snprintf(observed, cap, "CREATE_FAILED");
        return;
    }
    snprintf(observed, cap, "%s", status_name(duospool_digest_hex(inst, out, sizeof out)));
    duospool_destroy(inst);
}

/* Load lcg-64k.bin, record time and digest, attempt hdr-66048.bin, and require the refusal to change nothing. */
static void run_keeps_state(const char *dir, char *observed, size_t cap) {
    uint8_t *good = NULL;
    uint8_t *bad = NULL;
    size_t good_len = 0;
    size_t bad_len = 0;
    ds_instance *inst = NULL;
    char d1[17] = {0};
    char d2[17] = {0};
    uint64_t t1 = 0;
    uint64_t t2 = 0;
    ds_status refusal;
    if (read_fixture(dir, "lcg-64k.bin", &good, &good_len) != 0 || read_fixture(dir, "hdr-66048.bin", &bad, &bad_len) != 0) {
        snprintf(observed, cap, "FIXTURE_UNREADABLE");
        free(good);
        free(bad);
        return;
    }
    if (duospool_create(&inst) != DS_OK || duospool_load(inst, good, good_len) != DS_OK) {
        snprintf(observed, cap, "SETUP_FAILED");
    } else {
        (void)duospool_step(inst, 7u);
        (void)duospool_time(inst, &t1);
        (void)duospool_digest_hex(inst, d1, sizeof d1);
        refusal = duospool_load(inst, bad, bad_len);
        (void)duospool_time(inst, &t2);
        (void)duospool_digest_hex(inst, d2, sizeof d2);
        snprintf(observed, cap, "%s", (refusal == DS_REFUSE_HEADER && t1 == t2 && strcmp(d1, d2) == 0) ? "UNCHANGED" : "CHANGED");
    }
    duospool_destroy(inst);
    free(good);
    free(bad);
}

/* Load the fixture, apply the W-11 steps 100, 200, 300 and report digest:<hex>. */
static void run_step_digest(const char *dir, const char *fixture, char *observed, size_t cap) {
    uint8_t *buf = NULL;
    size_t len = 0;
    ds_instance *inst = NULL;
    char digest[17];
    ds_status st;
    if (read_fixture(dir, fixture, &buf, &len) != 0) {
        snprintf(observed, cap, "FIXTURE_UNREADABLE");
        return;
    }
    if (duospool_create(&inst) != DS_OK) {
        snprintf(observed, cap, "CREATE_FAILED");
    } else if ((st = duospool_load(inst, buf, len)) != DS_OK) {
        snprintf(observed, cap, "%s", status_name(st));
    } else if (duospool_step(inst, 100u) != DS_OK || duospool_step(inst, 200u) != DS_OK || duospool_step(inst, 300u) != DS_OK ||
               duospool_digest_hex(inst, digest, sizeof digest) != DS_OK) {
        snprintf(observed, cap, "STEP_FAILED");
    } else {
        snprintf(observed, cap, "digest:%s", digest);
    }
    duospool_destroy(inst);
    free(buf);
}

/* W-09: instances A and B load lcg-64k.bin and step 1, 7, 64, 1000 (x100), alternating A and B per step. Both final
 * digests and times must equal a solo instance's. */
static void run_interleave(const char *dir, char *observed, size_t cap) {
    static const uint32_t cycles[4] = {1u, 7u, 64u, 1000u};
    uint8_t *buf = NULL;
    size_t len = 0;
    ds_instance *a = NULL;
    ds_instance *b = NULL;
    ds_instance *solo = NULL;
    char da[17];
    char db[17];
    char ds[17];
    uint64_t ta = 0;
    uint64_t tb = 0;
    uint64_t ts = 0;
    int ok = 1;
    unsigned round;
    unsigned k;
    if (read_fixture(dir, "lcg-64k.bin", &buf, &len) != 0) {
        snprintf(observed, cap, "FIXTURE_UNREADABLE");
        return;
    }
    if (duospool_create(&a) != DS_OK || duospool_create(&b) != DS_OK || duospool_create(&solo) != DS_OK ||
        duospool_load(a, buf, len) != DS_OK || duospool_load(b, buf, len) != DS_OK || duospool_load(solo, buf, len) != DS_OK) {
        ok = 0;
    }
    for (round = 0u; ok != 0 && round < 100u; round++) {
        for (k = 0u; k < 4u; k++) {
            if (duospool_step(a, cycles[k]) != DS_OK || duospool_step(b, cycles[k]) != DS_OK || duospool_step(solo, cycles[k]) != DS_OK) {
                ok = 0;
            }
        }
    }
    if (ok != 0 && duospool_digest_hex(a, da, sizeof da) == DS_OK && duospool_digest_hex(b, db, sizeof db) == DS_OK &&
        duospool_digest_hex(solo, ds, sizeof ds) == DS_OK && duospool_time(a, &ta) == DS_OK && duospool_time(b, &tb) == DS_OK &&
        duospool_time(solo, &ts) == DS_OK) {
        snprintf(observed, cap, "%s", (strcmp(da, ds) == 0 && strcmp(db, ds) == 0 && ta == ts && tb == ts) ? "EQUAL" : "DIFFER");
    } else {
        snprintf(observed, cap, "SETUP_FAILED");
    }
    duospool_destroy(a);
    duospool_destroy(b);
    duospool_destroy(solo);
    free(buf);
}

/* Split a manifest line in place on blanks. Returns the number of fields (at most max). */
static int split_fields(char *line, char **f, int max) {
    int n = 0;
    char *p = line;
    while (*p != '\0' && n < max) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            *p++ = '\0';
        }
        if (*p == '\0') {
            break;
        }
        f[n++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
            p++;
        }
    }
    return n;
}

static int load_cases(const char *path, struct manifest_case *cases) {
    FILE *fp = fopen(path, "r");
    char line[512];
    char *f[8];
    int count = 0;
    int nf;
    if (fp == NULL) {
        return -1;
    }
    while (fgets(line, (int)sizeof line, fp) != NULL) {
        nf = split_fields(line, f, 8);
        if (nf < 5 || strcmp(f[0], "case") != 0) {
            continue;
        }
        if (count >= MAX_CASES) {
            fclose(fp);
            return -1;
        }
        snprintf(cases[count].id, FIELD, "%s", f[1]);
        snprintf(cases[count].kind, FIELD, "%s", f[2]);
        snprintf(cases[count].arg, FIELD, "%s", f[3]);
        snprintf(cases[count].expect, FIELD, "%s", f[4]);
        cases[count].control = (nf >= 6 && strcmp(f[5], "control") == 0) ? 1 : 0;
        count++;
    }
    fclose(fp);
    return count;
}

int main(int argc, char **argv) {
    struct manifest_case cases[MAX_CASES];
    char observed[FIELD];
    const char *verdict;
    int count;
    int i;
    int pass = 0;
    int fail = 0;
    int rejected = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: duospool-runner <fixtures.manifest> <out-dir>\n");
        return 2;
    }
    count = load_cases(argv[1], cases);
    if (count <= 0) {
        fprintf(stderr, "runner: no cases in %s\n", argv[1]);
        return 2;
    }
    for (i = 0; i < count; i++) {
        const struct manifest_case *c = &cases[i];
        if (strcmp(c->kind, "load") == 0) {
            run_load(argv[2], c->arg, observed, sizeof observed);
        } else if (strcmp(c->kind, "api-null") == 0) {
            run_api_null(observed, sizeof observed);
        } else if (strcmp(c->kind, "api-short") == 0) {
            run_api_short(observed, sizeof observed);
        } else if (strcmp(c->kind, "refuse-keeps-state") == 0) {
            run_keeps_state(argv[2], observed, sizeof observed);
        } else if (strcmp(c->kind, "step-digest") == 0) {
            run_step_digest(argv[2], c->arg, observed, sizeof observed);
        } else if (strcmp(c->kind, "interleave") == 0) {
            run_interleave(argv[2], observed, sizeof observed);
        } else {
            snprintf(observed, sizeof observed, "UNKNOWN_KIND");
        }
        if (c->control != 0) {
            if (strcmp(observed, c->expect) == 0) {
                verdict = "CONTROL-BROKEN";
                fail++;
            } else {
                verdict = "CONTROL-REJECTED";
                rejected++;
            }
        } else if (strcmp(observed, c->expect) == 0) {
            verdict = "PASS";
            pass++;
        } else {
            verdict = "FAIL";
            fail++;
        }
        printf("%s %s %s\n", c->id, observed, verdict);
    }
    printf("RUNNER pass=%d fail=%d controls_rejected=%d\n", pass, fail, rejected);
    return fail == 0 ? 0 : 1;
}
