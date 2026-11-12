/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * fixlist.c - the runner's --fixtures list (plan 17, FIX-02, D-59). It prints one line  NAME SIZE  for every synthetic recipe that the
 * gate can emit, de-duplicated and sorted in byte order:
 *   - the recipe of every case that has a fixture key other than none and authored:NAME (an authored ROM is built, not synthesised), for each request of the case (the text before and after each
 *     "||"), when the request materialises bytes: ptr is ok and len is within the content limit (ptr=null, ptr=dummy and
 *     len=size_max materialise none). The recipe is the one synth_name gives for the fixture and the effective len, so a len=N
 *     override is named plain-256k:N (or len:N over a len:N fixture) and not mistaken for a pure stream;
 *   - every recipe len:N of the hash sweep (the lengths come from the caller, the one definition in runner.c).
 * The case rows are read with parse_spec and synth_size, the functions that the kinds use, so the list cannot drift from what the
 * runner generates. Every kind takes its request as one spec or as a || list, except precedence, whose table of REQ>>CODE items is
 * read here the way refkinds.c reads it. Images that are not recipes (case-local mutations applied after generation, and the run-time image of the redaction
 * kinds) are not in the list by design. A case that cannot be resolved is a harness fault (exit 2): a list that quietly leaves it out
 * would make the manifest claim more than it covers.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runner.h"
#include "synth.h"

#define FIX_MAX 512u
#define FIX_NAME 48u

static char names[FIX_MAX][FIX_NAME];
static size_t nnames;

static int add_name(const char *name)
{
    size_t i;

    for (i = 0u; i < nnames; i++) {
        if (strcmp(names[i], name) == 0) {
            return 0;
        }
    }
    if (nnames == FIX_MAX || strlen(name) >= FIX_NAME) {
        return -1;
    }
    memcpy(names[nnames++], name, strlen(name) + 1u);
    return 0;
}

static int by_name(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

/* the recipe that one request of a case materialises; 0 on success, -1 on a fault of the case (message on stderr) */
static int add_request(const struct kase *k, const char *text, size_t natural)
{
    struct spec s;
    char name[FIX_NAME];
    const char *err = parse_spec(&s, text, natural);

    if (err != NULL) {
        fprintf(stderr, "HARNESS fixtures: case %s: %s\n", k->stem, err);
        return -1;
    }
    if (s.ptr != PTR_OK || s.len > CONTENT_LIMIT) {
        return 0;
    }
    if (synth_name(k->val[K_FIXTURE], s.len, name, sizeof name) != 0 || add_name(name) != 0) {
        fprintf(stderr, "HARNESS fixtures: case %s: no recipe name for its image\n", k->stem);
        return -1;
    }
    return 0;
}

int list_fixtures(const size_t *sweep, size_t nsweep)
{
    size_t i;

    for (i = 0u; i < case_count(); i++) {
        const struct kase *k = case_get(i);
        char text[512];
        char *part;
        size_t natural;

        /* none has no image; authored:NAME is built by scripts/fixture-build and listed in the authored used list, never a synthetic recipe (plan 03-04, F3) */
        if (!k->has[K_FIXTURE] || strcmp(k->val[K_FIXTURE], "none") == 0 || strncmp(k->val[K_FIXTURE], "authored:", 9u) == 0) {
            continue;
        }
        natural = synth_size(k->val[K_FIXTURE]);
        if (natural == 0u && strcmp(k->val[K_FIXTURE], "len:0") != 0) {
            fprintf(stderr, "HARNESS fixtures: case %s: unknown fixture recipe\n", k->stem);
            return 2;
        }
        if (k->has[K_KIND] && strcmp(k->val[K_KIND], "precedence") == 0) {
            /* the one kind with a request grammar of its own: items REQ>>CODE separated by a bar, in the request key and then in the expect key
             * (refkinds.c run_items); each REQ is one request over the fixture */
            size_t w;

            for (w = 0u; w < 2u; w++) {
                const char *src = w == 0u ? k->val[K_REQUEST] : k->val[K_EXPECT];

                if (strlen(src) >= sizeof text) {
                    fprintf(stderr, "HARNESS fixtures: case %s: table too long\n", k->stem);
                    return 2;
                }
                strcpy(text, src);
                for (part = text; *part != '\0';) {
                    char *end = strchr(part, '|');
                    char *arrow;

                    if (end != NULL) {
                        *end = '\0';
                    }
                    arrow = strstr(part, ">>");
                    if (arrow == NULL) {
                        fprintf(stderr, "HARNESS fixtures: case %s: a precedence item has no >>\n", k->stem);
                        return 2;
                    }
                    *arrow = '\0';
                    if (add_request(k, part, natural) != 0) {
                        return 2;
                    }
                    if (end == NULL) {
                        break;
                    }
                    part = end + 1;
                }
            }
            continue;
        }
        if (strlen(k->has[K_REQUEST] ? k->val[K_REQUEST] : "") >= sizeof text) {
            fprintf(stderr, "HARNESS fixtures: case %s: request too long\n", k->stem);
            return 2;
        }
        strcpy(text, k->has[K_REQUEST] ? k->val[K_REQUEST] : "");
        for (part = text;;) {
            char *bar = strstr(part, "||"); /* a request is one or more requests separated by || (the interleave and digest_pair kinds) */

            if (bar != NULL) {
                *bar = '\0';
            }
            if (add_request(k, part, natural) != 0) {
                return 2;
            }
            if (bar == NULL) {
                break;
            }
            part = bar + 2;
        }
    }
    for (i = 0u; i < nsweep; i++) {
        char name[FIX_NAME];

        if (snprintf(name, sizeof name, "len:%lu", (unsigned long)sweep[i]) < 0 || add_name(name) != 0) {
            return 2;
        }
    }
    if (nnames == 0u) {
        fprintf(stderr, "HARNESS fixtures: the list is empty\n");
        return 2;
    }
    qsort(names, nnames, FIX_NAME, by_name);
    for (i = 0u; i < nnames; i++) {
        printf("%s %lu\n", names[i], (unsigned long)synth_size(names[i]));
    }
    return 0;
}
