/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * audiokinds.c - the audio case kind (plan 05-04, grown by 05-05..05-09; D-183, D-187, D-196, D-197, D-199, D-201, D-204, D-206..D-208, D-211, D-230, ER-19, ER-20, ER-21, PR-13, RF-13, RF-14, F-85, F-86). Test code only.
 *   audio   boots a ROM that scripts/fixture-build assembled, attaches a psg sink (16,384 records), a pcm sink (ceil((until - T0 + ds_max_step_mclk()) / 192) + 1 frames, D-201) and a dev sink, runs it with ds_run to the end of its DSV1 verdict (rq.stop = dsv1_done, until_mclk the hard
 *           bound), judges the DSV1 block as the guest kind does, then reads the DSPE records and DSPA frames the core wrote and judges the scene by GROUP. Records and frames are parsed with the OFFSETS OF D-196 AND D-197 WRITTEN IN THIS FILE: the runner never includes a core
 *           record writer, and no expected value comes from the core, another emulator or a hash (ER-19, R7). The run is made under the zero policy and under seeded-prng-v4 seed 1, and the DSPE and DSPA streams of the two must be byte-identical (D-204: the fixture
 *           initialises every word it plays); the groups are judged on the zero run. Every group is evaluated and printed even when the DSV1 verdict fails (Pitfall 4); the outcome then stays guest_failure.
 *   expect  rom=NAME;id=N;checks=N;scene=NAME[;expect_red=GID,GID...]  (every item once, any order): the built image build/gate/fixtures/NAME.pce, the fixture id and check count of its DSV1 block, the scene of the .aexp file, and for an expect-red binding (D-211) the groups that
 *           must fail. A scene row names one case; another case may judge the same scene only with expect_red (the wrong-divisor control: the same literals, a different request). With expect_red the case passes only when exactly the listed groups fail and every other group
 *           passes; anything else is the harness failure misprog-misattributed (D-211).
 * The literals are NOT in the case: they live in fixtures/src/NAME.aexp and reach this file only as build/gate/fixtures/NAME.aexp.tsv, written by scripts/fixture-build (F-73, PR-12). The grammar, the class lint and the kill rules are judged statically by scripts/audio-check
 * (plan 05-05); until then this file is the only reader and refuses an unknown row kind, an unknown key or a malformed row itself (F-86). Rows, TAB separated, keys closed:
 *   aexp    sample_mclk=N                                                     the header, exactly once: N must equal the DSPA header's sample_mclk (a mismatch is harness_failure)
 *   scene   NAME  case=ID  role=literal|golden|expect_red  [twin=NAME]  [expect_red=GID,...]
 *   group   SCENE GID  form=ROWS|REL|STRUCT|EV  kind=KIND  class=MAN:<labels>|GUEST:<model>|ASSUMED:<LED-id>  kills=AF0,...
 *   row     SCENE GID  SELECTOR  LITERAL
 * SELECTOR items (semicolons): anchor=ev:ch=<0..5|G>;reg=R<n>;[val=$hh;]nth=N (the N-th DSPE record of that channel, G the global chan $FE, register and value); ev=+K (the K-th record after the anchor, form EV); from=+M and to=+M (master clocks after the anchor) or from=M and to=M
 * (absolute, no anchor); to=ev:ch=..;reg=..;[val=..;]nth=N (an end anchor, RF-13: the reg, val and nth items after it belong to it); side=L|R. A window [from, to) holds the samples s with from <= s x sample_mclk < to (D-187), converted to stream indices with the DSPA header's
 * sample_mclk and first frame. LITERAL items (semicolons) are closed keys: the judged ones of each kind and the inputs of the 05-05 arithmetic lint (accepted, not judged here).
 * Kinds judged here (05-04): ev_value (reg and val, or reg and vals for consecutive records), ev_decode (chan, effect, wave_idx, post of one record: the core's decode against the row, never a judge-side decode), period (the rising crossings of the window: threshold (min + max) / 2
 * of the window on that side, a crossing is the first sample with 2x >= min + max after one with 2x < min + max; fewer than k + 1 crossings is a fail; the distance from the first to the (k+1)-th must be d), cycle_order (the samples between the first two crossings run-collapsed
 * into plateaus, ranked, and compared with the ranks of words up to rotation; RF-14) and silent (every sample of the window on that side is 0, ER-19). An anchor that is not found or a window past the end of the stream fails its row; an empty window is harness_failure.
 * The evidence text carries pcm_sha256=HEX psg_sha256=HEX E0 repeatability: the SHA-256 of the streams of D-199 (DSPE: "duospool-psg" NUL, u32 1, the records; DSPA: "duospool-pcm" NUL, u32 1, u32 sample_mclk, u16 2, u8 1, u8 0, u64 first frame, the frames).
 * A hash is a change detector and never a verdict (ER-20): the literal groups decide the case. A lost record or frame of any sink, a missing sink and an inconsistent header are harness_failure.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_sha256.h"
#include "runner.h"

/* the DSPE v1 record of D-196 as this file knows it (hand-written, ER-19) */
#define AU_E_REC 24u
#define AU_E_STAMP 0u
#define AU_E_PC 8u
#define AU_E_REG 10u
#define AU_E_VALUE 11u
#define AU_E_SEL 12u
#define AU_E_CHAN 13u
#define AU_E_EFFECT 14u
#define AU_E_WIDX 15u
#define AU_E_POST 16u
#define AU_E_VERSION 18u
#define AU_CHAN_GLOBAL 0xFEu
/* the DSPA v1 header and frame of D-197 */
#define AU_F_BYTES 8u
#define AU_H_COUNT 0u
#define AU_H_LOST 4u
#define AU_H_SEQ 8u
#define AU_H_FIRST 16u
#define AU_H_SMCLK 24u
#define AU_H_FBYTES 28u
#define AU_H_VERSION 30u
#define AU_H_FORMAT 31u
/* the dev record (ds_obs.h layout, hand-written here) */
#define AU_D_REC 24u
#define AU_PSG_CAP 16384u /* the "large" psg sink (F-81): far above the window-2 writes of an authored scene */
#define AU_DEV_CAP 16384u
#define AU_PSG_STREAM_HDR 17u /* "duospool-psg" NUL (13) and u32 1 */
#define AU_PCM_STREAM_HDR 33u /* "duospool-pcm" NUL (13), u32 1, u32 sample_mclk, u16 2, u8 1, u8 0, u64 first frame */
#define AU_NAME 32u

static uint32_t au_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t au_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t au_u64(const uint8_t *p)
{
    return (uint64_t)au_u32(p) | ((uint64_t)au_u32(p + 4) << 32);
}

static void au_put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void au_put64(uint8_t *p, uint64_t v)
{
    au_put32(p, (uint32_t)(v & UINT64_C(0xFFFFFFFF)));
    au_put32(p + 4, (uint32_t)((v >> 32) & UINT64_C(0xFFFFFFFF)));
}

/* a scene name, a group id or a case id: [a-z][a-z0-9_]* of at least one and fewer than cap characters */
static int au_name_ok(const char *s, size_t cap)
{
    size_t n = strlen(s);

    return n >= 1u && n < cap && s[0] >= 'a' && s[0] <= 'z' && strspn(s, "abcdefghijklmnopqrstuvwxyz0123456789_") == n;
}

/* a group id: [a-z][a-zA-Z0-9_]*, fewer than AU_NAME characters (the side suffixes _L and _R of the plan's group ids, period_L, cycle_order_R) */
static int au_gid_ok(const char *s)
{
    size_t n = strlen(s);

    return n >= 1u && n < AU_NAME && s[0] >= 'a' && s[0] <= 'z' && strspn(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == n;
}

/* a number of a row: decimal digits, or $ and hex digits (at most 16); 0 on success */
static int au_num(const char *s, uint64_t *out)
{
    uint64_t v = 0u;
    size_t i;

    if (s[0] == '$') {
        if (strlen(s) < 2u || strlen(s) > 17u || strspn(s + 1, "0123456789abcdefABCDEF") != strlen(s + 1)) {
            return -1;
        }
        for (i = 1u; s[i] != '\0'; i++) {
            char ch = s[i];
            uint64_t dgt = ch >= '0' && ch <= '9' ? (uint64_t)(ch - '0') : ch >= 'a' && ch <= 'f' ? (uint64_t)(ch - 'a' + 10) : (uint64_t)(ch - 'A' + 10);

            v = (v << 4) | dgt;
        }
        *out = v;
        return 0;
    }
    if (strlen(s) == 0u || strlen(s) > 19u || strspn(s, "0123456789") != strlen(s)) {
        return -1;
    }
    return parse_u64(s, out);
}

/* a register name R0..R9 or RA..RF into 0..15; 0 on success */
static int au_reg(const char *s, unsigned *out)
{
    if (s[0] != 'R' || s[1] == '\0' || s[2] != '\0') {
        return -1;
    }
    if (s[1] >= '0' && s[1] <= '9') {
        *out = (unsigned)(s[1] - '0');
    } else if (s[1] >= 'A' && s[1] <= 'F') {
        *out = (unsigned)(s[1] - 'A' + 10);
    } else {
        return -1;
    }
    return 0;
}

/* ---- the expect of an audio case ---- */

#define AU_RED_MAX 16u

struct au_expect {
    char rom[48];
    uint64_t id;
    uint64_t checks;
    char scene[AU_NAME];
    unsigned nred;
    char red[AU_RED_MAX][AU_NAME];
    unsigned seen;
};

enum { AUI_ROM = 1, AUI_ID = 2, AUI_CHECKS = 4, AUI_SCENE = 8, AUI_RED = 16, AUI_ALL = 15 };

/* a list of group ids separated by commas into e->red; 0 on success */
static int au_red_list(const char *text, struct au_expect *e)
{
    char buf[PK_LIST_MAX];
    char *rest = buf;
    char *item;

    if (pk_copy_text(text, buf, sizeof buf) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        unsigned i;

        if (e->nred >= AU_RED_MAX || !au_gid_ok(item)) {
            return -1;
        }
        for (i = 0u; i < e->nred; i++) {
            if (strcmp(e->red[i], item) == 0) {
                return -1;
            }
        }
        memcpy(e->red[e->nred++], item, strlen(item) + 1u);
    }
    return e->nred >= 1u ? 0 : -1;
}

static int au_parse(const char *text, struct au_expect *e)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    memset(e, 0, sizeof *e);
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        const char *val;
        unsigned bit;

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        val = eq + 1;
        if (strcmp(item, "rom") == 0) {
            bit = AUI_ROM;
            if (strlen(val) == 0u || strlen(val) >= sizeof e->rom || strspn(val, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(val)) {
                return -1;
            }
            memcpy(e->rom, val, strlen(val) + 1u);
        } else if (strcmp(item, "id") == 0) {
            bit = AUI_ID;
            if (parse_u64(val, &e->id) != 0 || e->id == 0u || e->id > 255u) {
                return -1;
            }
        } else if (strcmp(item, "checks") == 0) {
            bit = AUI_CHECKS;
            if (parse_u64(val, &e->checks) != 0 || e->checks == 0u || e->checks > 255u) {
                return -1;
            }
        } else if (strcmp(item, "scene") == 0) {
            bit = AUI_SCENE;
            if (!au_name_ok(val, sizeof e->scene)) {
                return -1;
            }
            memcpy(e->scene, val, strlen(val) + 1u);
        } else if (strcmp(item, "expect_red") == 0) {
            bit = AUI_RED;
            if (au_red_list(val, e) != 0) {
                return -1;
            }
        } else {
            return -1;
        }
        if ((e->seen & bit) != 0u) {
            return -1;
        }
        e->seen |= bit;
    }
    return (e->seen & (unsigned)AUI_ALL) == (unsigned)AUI_ALL ? 0 : -1;
}

/* ---- the .aexp.tsv channel (F-73, F-86, D-230) ---- */

#define AX_MAX 512u /* rows of one file (D-230: the audio-check reader refuses more) */
#define AX_LINE 400u
#define AX_TEXT 400u
#define AX_CLASS 96u

enum { AX_ROLE_LITERAL = 1, AX_ROLE_GOLDEN = 2, AX_ROLE_EXPECT_RED = 3 };
enum { AX_FORM_ROWS = 1, AX_FORM_REL = 2, AX_FORM_STRUCT = 3, AX_FORM_EV = 4 };
enum { AK_EV_VALUE = 1, AK_EV_DECODE = 2, AK_PERIOD = 3, AK_CYCLE_ORDER = 4, AK_SILENT = 5 };

struct ax_scene {
    char name[AU_NAME];
    char case_id[72];
    int role;
};

struct ax_group {
    char scene[AU_NAME];
    char gid[AU_NAME];
    int form;
    int kind;
    char cls[AX_CLASS];
    unsigned kills; /* bit n: the fault AFn is declared to flip the group (judged from plan 05-05) */
};

struct ax_row {
    char scene[AU_NAME];
    char gid[AU_NAME];
    char sel[AX_TEXT];
    char lit[AX_TEXT];
    unsigned group;
};

struct ax_file {
    uint64_t sample_mclk;
    unsigned nh;
    unsigned ns;
    unsigned ng;
    unsigned nr;
    struct ax_scene sc[AX_MAX];
    struct ax_group gr[AX_MAX];
    struct ax_row rw[AX_MAX];
};

static unsigned ax_split(char *line, char **f, unsigned cap)
{
    unsigned n = 0u;
    char *p = line;

    f[n++] = p;
    while (*p != '\0') {
        if (*p == '\t') {
            *p = '\0';
            if (n >= cap) {
                return cap + 1u;
            }
            f[n++] = p + 1;
        }
        p++;
    }
    return n;
}

static const char *ax_val(const char *field, const char *key)
{
    size_t n = strlen(key);

    return strncmp(field, key, n) == 0 && field[n] == '=' ? field + n + 1u : NULL;
}

static int ax_kills(const char *text, unsigned *mask)
{
    char buf[64];
    char *rest = buf;
    char *item;

    *mask = 0u;
    if (strlen(text) == 0u || strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        if (item[0] != 'A' || item[1] != 'F' || item[2] < '0' || item[2] > '9' || item[3] != '\0') {
            return -1;
        }
        *mask |= 1u << (unsigned)(item[2] - '0');
    }
    return *mask != 0u ? 0 : -1;
}

static int ax_class_ok(const char *text)
{
    const char *colon = strchr(text, ':');

    if (colon == NULL || colon[1] == '\0' || strlen(text) >= AX_CLASS) {
        return 0;
    }
    return strncmp(text, "MAN:", 4u) == 0 || strncmp(text, "GUEST:", 6u) == 0 || strncmp(text, "ASSUMED:", 8u) == 0;
}

static int ax_kind(const char *text)
{
    if (strcmp(text, "ev_value") == 0) {
        return AK_EV_VALUE;
    }
    if (strcmp(text, "ev_decode") == 0) {
        return AK_EV_DECODE;
    }
    if (strcmp(text, "period") == 0) {
        return AK_PERIOD;
    }
    if (strcmp(text, "cycle_order") == 0) {
        return AK_CYCLE_ORDER;
    }
    if (strcmp(text, "silent") == 0) {
        return AK_SILENT;
    }
    return 0;
}

static const char *ax_row_parse(struct ax_file *f, char **fld, unsigned nf)
{
    const char *a = NULL;
    const char *b = NULL;
    unsigned i;

    if (strcmp(fld[0], "aexp") == 0) {
        if (nf != 2u || (a = ax_val(fld[1], "sample_mclk")) == NULL || au_num(a, &f->sample_mclk) != 0 || f->sample_mclk == 0u || f->nh != 0u) {
            return "aexp-malformed: the header row is: aexp sample_mclk=N, exactly once";
        }
        f->nh = 1u;
    } else if (strcmp(fld[0], "scene") == 0) {
        struct ax_scene *s;

        if (nf < 4u || nf > 6u || f->ns >= AX_MAX || !au_name_ok(fld[1], AU_NAME) || (a = ax_val(fld[2], "case")) == NULL || (b = ax_val(fld[3], "role")) == NULL || !au_name_ok(a, 72u)) {
            return "aexp-malformed: a scene row is: scene NAME case=ID role=literal|golden|expect_red [twin=NAME] [expect_red=GID,...]";
        }
        for (i = 4u; i < nf; i++) {
            const char *t = ax_val(fld[i], "twin");
            const char *r = ax_val(fld[i], "expect_red");

            if ((t == NULL || !au_name_ok(t, AU_NAME)) && (r == NULL || strlen(r) == 0u)) {
                return "aexp-unknown-key: the optional scene keys are twin=NAME and expect_red=GID,...";
            }
        }
        for (i = 0u; i < f->ns; i++) {
            if (strcmp(f->sc[i].case_id, a) == 0 || strcmp(f->sc[i].name, fld[1]) == 0) {
                return "aexp-malformed: two scene rows name the same case or the same scene";
            }
        }
        s = &f->sc[f->ns++];
        memcpy(s->name, fld[1], strlen(fld[1]) + 1u);
        memcpy(s->case_id, a, strlen(a) + 1u);
        if (strcmp(b, "literal") == 0) {
            s->role = AX_ROLE_LITERAL;
        } else if (strcmp(b, "golden") == 0) {
            s->role = AX_ROLE_GOLDEN;
        } else if (strcmp(b, "expect_red") == 0) {
            s->role = AX_ROLE_EXPECT_RED;
        } else {
            return "aexp-unknown-key: a scene role is literal, golden or expect_red";
        }
    } else if (strcmp(fld[0], "group") == 0) {
        struct ax_group *g;
        const char *fm;
        const char *kd;
        const char *cl;
        const char *kl;

        if (nf != 7u || f->ng >= AX_MAX || !au_name_ok(fld[1], AU_NAME) || !au_gid_ok(fld[2]) || (fm = ax_val(fld[3], "form")) == NULL || (kd = ax_val(fld[4], "kind")) == NULL || (cl = ax_val(fld[5], "class")) == NULL ||
            (kl = ax_val(fld[6], "kills")) == NULL) {
            return "aexp-malformed: a group row is: group SCENE GID form= kind= class= kills=";
        }
        g = &f->gr[f->ng++];
        memcpy(g->scene, fld[1], strlen(fld[1]) + 1u);
        memcpy(g->gid, fld[2], strlen(fld[2]) + 1u);
        if (strcmp(fm, "ROWS") == 0) {
            g->form = AX_FORM_ROWS;
        } else if (strcmp(fm, "REL") == 0) {
            g->form = AX_FORM_REL;
        } else if (strcmp(fm, "STRUCT") == 0) {
            g->form = AX_FORM_STRUCT;
        } else if (strcmp(fm, "EV") == 0) {
            g->form = AX_FORM_EV;
        } else {
            return "aexp-unknown-key: a group form is ROWS, REL, STRUCT or EV";
        }
        g->kind = ax_kind(kd);
        if (g->kind == 0) {
            return "aexp-unknown-key: a group kind this runner does not judge (05-04: ev_value, ev_decode, period, cycle_order, silent)";
        }
        if ((g->kind == AK_EV_VALUE || g->kind == AK_EV_DECODE) != (g->form == AX_FORM_EV) || (g->form != AX_FORM_EV && g->form != AX_FORM_ROWS)) {
            return "aexp-malformed: the ev kinds take form EV and period, cycle_order and silent form ROWS";
        }
        if (!ax_class_ok(cl) || ax_kills(kl, &g->kills) != 0) {
            return "aexp-malformed: a group needs class=MAN:L|GUEST:M|ASSUMED:LED and a non-empty kills list of AF0 to AF9";
        }
        memcpy(g->cls, cl, strlen(cl) + 1u);
        for (i = 0u; i + 1u < f->ng; i++) {
            if (strcmp(f->gr[i].scene, g->scene) == 0 && strcmp(f->gr[i].gid, g->gid) == 0) {
                return "aexp-malformed: two group rows have the same scene and id";
            }
        }
    } else if (strcmp(fld[0], "row") == 0) {
        struct ax_row *r;

        if (nf != 5u || f->nr >= AX_MAX || !au_name_ok(fld[1], AU_NAME) || !au_gid_ok(fld[2]) || strlen(fld[3]) == 0u || strlen(fld[3]) >= AX_TEXT || strlen(fld[4]) == 0u || strlen(fld[4]) >= AX_TEXT) {
            return "aexp-malformed: a row row is: row SCENE GID SELECTOR LITERAL";
        }
        r = &f->rw[f->nr++];
        memcpy(r->scene, fld[1], strlen(fld[1]) + 1u);
        memcpy(r->gid, fld[2], strlen(fld[2]) + 1u);
        memcpy(r->sel, fld[3], strlen(fld[3]) + 1u);
        memcpy(r->lit, fld[4], strlen(fld[4]) + 1u);
    } else {
        return "aexp-unknown-key: a row kind is aexp, scene, group or row";
    }
    return NULL;
}

/* build/gate/fixtures/ROM.aexp.tsv into f (cleared first); NULL on success. A missing file is a fault of the harness, never an empty scene list */
static const char *ax_load(const char *rom, struct ax_file *f)
{
    char path[96];
    char *text;
    char *line;
    unsigned nrows = 0u;
    unsigned i;
    const char *err = NULL;

    memset(f, 0, sizeof *f);
    snprintf(path, sizeof path, "build/gate/fixtures/%.48s.aexp.tsv", rom);
    text = read_file(path);
    if (text == NULL) {
        return "aexp-missing: the built aexp rows are missing: run scripts/fixture-build first (the scenes of an audio case live in fixtures/src/NAME.aexp)";
    }
    line = text;
    while (*line != '\0' && err == NULL) {
        char *nl = strchr(line, '\n');
        char *fld[8];
        unsigned nf;

        if (nl == NULL) {
            err = "aexp-malformed: the built aexp rows do not end in a newline";
            break;
        }
        *nl = '\0';
        if (strlen(line) > AX_LINE || ++nrows > AX_MAX) {
            err = "aexp-malformed: a row over 400 characters, or more than 512 rows";
            break;
        }
        nf = ax_split(line, fld, 8u);
        if (nf > 7u) {
            err = "aexp-malformed: a row with more than 7 fields";
            break;
        }
        err = ax_row_parse(f, fld, nf);
        line = nl + 1;
    }
    free(text);
    if (err != NULL) {
        return err;
    }
    if (f->nh != 1u || f->ns == 0u) {
        return "aexp-malformed: the aexp rows need the header row aexp sample_mclk=N and at least one scene row";
    }
    for (i = 0u; i < f->nr; i++) {
        unsigned g;

        for (g = 0u; g < f->ng; g++) {
            if (strcmp(f->gr[g].scene, f->rw[i].scene) == 0 && strcmp(f->gr[g].gid, f->rw[i].gid) == 0) {
                break;
            }
        }
        if (g == f->ng) {
            return "aexp-malformed: a row names a group that no group row defines";
        }
        f->rw[i].group = g;
    }
    for (i = 0u; i < f->ng; i++) {
        unsigned s;

        for (s = 0u; s < f->ns; s++) {
            if (strcmp(f->sc[s].name, f->gr[i].scene) == 0) {
                break;
            }
        }
        if (s == f->ns) {
            return "aexp-malformed: a group names a scene that no scene row defines";
        }
    }
    return NULL;
}

/* ---- the capture of one run: the streams the judge reads ---- */

struct au_cap {
    uint8_t *erec; /* the DSPE records */
    uint32_t ecount;
    uint8_t *frames; /* the DSPA frames */
    uint32_t fcount;
    uint64_t first;
    uint32_t smclk;
    uint8_t *drec; /* the dev records */
    uint32_t dcount;
    uint64_t machine_time;
    uint32_t reason;
};

static void au_cap_free(struct au_cap *cp)
{
    free(cp->erec);
    free(cp->frames);
    free(cp->drec);
    memset(cp, 0, sizeof *cp);
}

/* the pcm capacity of D-201 for a run from t0 to the deadline until: ceil((until - t0 + ds_max_step_mclk()) / 192) + 1 frames (0 when it does not fit a sink) */
static uint64_t au_pcm_capacity(uint64_t t0, uint64_t until)
{
    uint64_t span = (until > t0 ? until - t0 : 0u) + ds_max_step_mclk();

    return span / 192u + (span % 192u != 0u ? 1u : 0u) + 1u;
}

/* boot ROM over request, run it to its DSV1 verdict or until with the three sinks, judge the DSV1 block on c (verdict text into got) and keep the streams in cp; 0 when the capture is usable, -1 after a harness error on c */
static int au_capture(struct ctx *c, const char *rom, const char *request, uint64_t until, const struct au_expect *e, struct au_cap *cp, int note, char *got, size_t gotcap)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_psg_sink psk;
    struct ds_pcm_sink fsk;
    struct ds_dev_sink dsk;
    uint64_t pcap;
    const char *err;

    memset(cp, 0, sizeof *cp);
    err = boot_built_rom(rom, request, &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    pcap = au_pcm_capacity(ci->machine_time, until);
    if (pcap > (uint64_t)DS_PCM_SINK_MAX_FRAMES) {
        release_boot(&b);
        harness_error(c, "the audio case is too long for one pcm sink (until_mclk needs more than 8,388,608 frames)");
        return -1;
    }
    cp->erec = (uint8_t *)calloc((size_t)AU_PSG_CAP, (size_t)AU_E_REC);
    cp->frames = (uint8_t *)calloc((size_t)pcap, (size_t)AU_F_BYTES);
    cp->drec = (uint8_t *)calloc((size_t)AU_DEV_CAP, (size_t)AU_D_REC);
    if (cp->erec == NULL || cp->frames == NULL || cp->drec == NULL) {
        release_boot(&b);
        harness_error(c, "out of memory for the sinks of an audio case");
        return -1;
    }
    memset(&psk, 0, sizeof psk);
    memset(&fsk, 0, sizeof fsk);
    memset(&dsk, 0, sizeof dsk);
    psk.records = cp->erec;
    psk.capacity = AU_PSG_CAP;
    fsk.frames = cp->frames;
    fsk.capacity = (uint32_t)pcap;
    dsk.records = cp->drec;
    dsk.capacity = AU_DEV_CAP;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.stop = dsv1_done; /* a speed aid only (D-95): until_mclk is the hard bound */
    rq.psg = &psk;
    rq.pcm = &fsk;
    rq.dev = &dsk;
    if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        release_boot(&b);
        harness_error(c, "ds_run refused the request of an audio case");
        return -1;
    }
    if (note) {
        note_run(c, res.machine_time, res.reason);
    }
    cp->machine_time = res.machine_time;
    cp->reason = res.reason;
    judge_dsv1(c, ci->ram8k + V_OFF, (unsigned)e->id, (unsigned)e->checks, got, gotcap);
    release_boot(&b);
    if (c->harness || c->timeout) {
        return -1;
    }
    cp->ecount = au_u32(psk.header + AU_H_COUNT);
    cp->fcount = au_u32(fsk.header + AU_H_COUNT);
    cp->dcount = au_u32(dsk.header + AU_H_COUNT);
    cp->first = au_u64(fsk.header + AU_H_FIRST);
    cp->smclk = au_u32(fsk.header + AU_H_SMCLK);
    if (au_u32(psk.header + AU_H_LOST) != 0u || au_u32(fsk.header + AU_H_LOST) != 0u || au_u32(dsk.header + AU_H_LOST) != 0u || au_u64(psk.header + AU_H_SEQ) != (uint64_t)cp->ecount ||
        au_u64(fsk.header + AU_H_SEQ) != (uint64_t)cp->fcount || cp->ecount > AU_PSG_CAP || (uint64_t)cp->fcount > pcap) {
        harness_error(c, "a sink of the audio case lost a record or a frame, or its header is inconsistent: the streams would be incomplete evidence (D-201)");
        return -1;
    }
    if (au_u16(fsk.header + AU_H_FBYTES) != AU_F_BYTES || fsk.header[AU_H_VERSION] != 1u || fsk.header[AU_H_FORMAT] != 1u || cp->smclk == 0u) {
        harness_error(c, "the pcm header does not read frame_bytes 8, version 1, format 1 and a sample_mclk above 0 (D-197)");
        return -1;
    }
    if (cp->fcount == 0u && cp->machine_time > cp->first * (uint64_t)cp->smclk) {
        harness_error(c, "the pcm sink holds no frame although samples were due: a missing sink is a fault of the harness, never a pass (D-209)");
        return -1;
    }
    return 0;
}

/* the two streams of D-199, built here from the sink contents (test code, never the core); heap blocks freed by the caller, NULL when out of memory */
static uint8_t *au_psg_stream(const struct au_cap *cp, size_t *len)
{
    size_t n = (size_t)AU_PSG_STREAM_HDR + (size_t)cp->ecount * (size_t)AU_E_REC;
    uint8_t *s = (uint8_t *)malloc(n);

    if (s == NULL) {
        return NULL;
    }
    memcpy(s, "duospool-psg", 12u);
    s[12] = 0u;
    au_put32(s + 13, 1u);
    if (cp->ecount > 0u) {
        memcpy(s + AU_PSG_STREAM_HDR, cp->erec, (size_t)cp->ecount * (size_t)AU_E_REC);
    }
    *len = n;
    return s;
}

static uint8_t *au_pcm_stream(const struct au_cap *cp, size_t *len)
{
    size_t n = (size_t)AU_PCM_STREAM_HDR + (size_t)cp->fcount * (size_t)AU_F_BYTES;
    uint8_t *s = (uint8_t *)malloc(n);

    if (s == NULL) {
        return NULL;
    }
    memcpy(s, "duospool-pcm", 12u);
    s[12] = 0u;
    au_put32(s + 13, 1u);
    au_put32(s + 17, cp->smclk);
    s[21] = 2u;
    s[22] = 0u;
    s[23] = 1u;
    s[24] = 0u;
    au_put64(s + 25, cp->first);
    if (cp->fcount > 0u) {
        memcpy(s + AU_PCM_STREAM_HDR, cp->frames, (size_t)cp->fcount * (size_t)AU_F_BYTES);
    }
    *len = n;
    return s;
}

static void au_hex(const uint8_t *d, char *out)
{
    static const char digits[] = "0123456789abcdef";
    unsigned i;

    for (i = 0u; i < DS_SHA256_BYTES; i++) {
        out[2u * i] = digits[(d[i] >> 4) & 0xFu];
        out[2u * i + 1u] = digits[d[i] & 0xFu];
    }
    out[2u * DS_SHA256_BYTES] = '\0';
}

/* the SHA-256 of both streams as hex; 0 on success */
static int au_digests(const struct au_cap *cp, char *pcm_hex, char *psg_hex)
{
    uint8_t d[DS_SHA256_BYTES];
    size_t n = 0u;
    uint8_t *s = au_pcm_stream(cp, &n);

    if (s == NULL) {
        return -1;
    }
    ds_sha256(s, n, d);
    free(s);
    au_hex(d, pcm_hex);
    s = au_psg_stream(cp, &n);
    if (s == NULL) {
        return -1;
    }
    ds_sha256(s, n, d);
    free(s);
    au_hex(d, psg_hex);
    return 0;
}

/* the zero-versus-v4 identity rule of D-204: 0 when both streams of a and b are byte-identical, 1 when the DSPA streams differ, 2 when only the DSPE streams differ, -1 when out of memory. Shared with aud_poweron_policy_diff, which shows that it can report a difference */
static int au_streams_differ(const struct au_cap *a, const struct au_cap *b)
{
    size_t na = 0u;
    size_t nb = 0u;
    uint8_t *sa = au_pcm_stream(a, &na);
    uint8_t *sb = au_pcm_stream(b, &nb);
    int r = 0;

    if (sa == NULL || sb == NULL) {
        r = -1;
    } else if (na != nb || memcmp(sa, sb, na) != 0) {
        r = 1;
    }
    free(sa);
    free(sb);
    if (r != 0) {
        return r;
    }
    sa = au_psg_stream(a, &na);
    sb = au_psg_stream(b, &nb);
    if (sa == NULL || sb == NULL) {
        r = -1;
    } else if (na != nb || memcmp(sa, sb, na) != 0) {
        r = 2;
    }
    free(sa);
    free(sb);
    return r;
}

/* ---- selectors and literals ---- */

struct au_ev {
    int set;
    unsigned ch; /* 0..5, or AU_CHAN_GLOBAL for G */
    int has_reg;
    unsigned reg;
    int has_val;
    unsigned val;
    uint64_t nth;
};

struct au_sel {
    struct au_ev a; /* the start anchor */
    struct au_ev e; /* the end anchor (to=ev:...) */
    int has_ev;
    uint64_t ev_off;
    int from_kind; /* 0 none, 1 relative to the anchor, 2 absolute */
    uint64_t from;
    int to_kind; /* 0 none, 1 relative, 2 absolute, 3 the end anchor */
    uint64_t to;
    int side; /* 0 none, 1 L, 2 R */
};

/* one selector into s; NULL on success, else why not (a fault of the literal file: harness_failure) */
static const char *au_sel_parse(const char *text, struct au_sel *s)
{
    char buf[AX_TEXT];
    char *rest = buf;
    char *item;
    struct au_ev *cur = NULL;

    memset(s, 0, sizeof *s);
    if (strlen(text) >= sizeof buf) {
        return "aexp-malformed: a selector is too long";
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        uint64_t v = 0u;
        const char *x;

        if ((x = ax_val(item, "anchor")) != NULL || ((x = ax_val(item, "to")) != NULL && strncmp(x, "ev:", 3u) == 0)) {
            struct au_ev *ev = item[0] == 'a' ? &s->a : &s->e;

            if (strncmp(x, "ev:ch=", 6u) != 0 || ev->set || (item[0] == 'a' && s->from_kind != 0) || (item[0] == 't' && s->to_kind != 0)) {
                return "aexp-malformed: an anchor is anchor=ev:ch=<0..5|G> or to=ev:ch=<0..5|G>, once each, the start anchor before from=";
            }
            if (strcmp(x + 6, "G") == 0) {
                ev->ch = AU_CHAN_GLOBAL;
            } else if (au_num(x + 6, &v) == 0 && v <= 5u && x[6] != '$') {
                ev->ch = (unsigned)v;
            } else {
                return "aexp-malformed: an anchor channel is 0..5 or G";
            }
            ev->set = 1;
            cur = ev;
            if (item[0] == 't') {
                s->to_kind = 3;
            }
        } else if ((x = ax_val(item, "reg")) != NULL) {
            if (cur == NULL || cur->has_reg || au_reg(x, &cur->reg) != 0) {
                return "aexp-malformed: reg=R<n> follows its anchor once";
            }
            cur->has_reg = 1;
        } else if ((x = ax_val(item, "val")) != NULL) {
            if (cur == NULL || cur->has_val || au_num(x, &v) != 0 || v > 255u) {
                return "aexp-malformed: val=$hh follows its anchor once";
            }
            cur->has_val = 1;
            cur->val = (unsigned)v;
        } else if ((x = ax_val(item, "nth")) != NULL) {
            if (cur == NULL || cur->nth != 0u || au_num(x, &v) != 0 || v == 0u) {
                return "aexp-malformed: nth=N (N >= 1) follows its anchor once";
            }
            cur->nth = v;
        } else if ((x = ax_val(item, "ev")) != NULL) {
            if (!s->a.set || s->has_ev || x[0] != '+' || au_num(x + 1, &v) != 0) {
                return "aexp-malformed: ev=+K follows the start anchor once";
            }
            s->has_ev = 1;
            s->ev_off = v;
            cur = NULL;
        } else if ((x = ax_val(item, "from")) != NULL || (x = ax_val(item, "to")) != NULL) {
            int is_from = item[0] == 'f';
            int rel = x[0] == '+';

            if (au_num(rel ? x + 1 : x, &v) != 0 || rel != s->a.set || (is_from ? s->from_kind : s->to_kind) != 0) {
                return "aexp-malformed: from= and to= are +M after a start anchor or M without one, once each";
            }
            if (is_from) {
                s->from_kind = rel ? 1 : 2;
                s->from = v;
            } else {
                s->to_kind = rel ? 1 : 2;
                s->to = v;
            }
            cur = NULL;
        } else if ((x = ax_val(item, "side")) != NULL) {
            if (s->side != 0 || (strcmp(x, "L") != 0 && strcmp(x, "R") != 0)) {
                return "aexp-malformed: side=L or side=R, once";
            }
            s->side = x[0] == 'L' ? 1 : 2;
            cur = NULL;
        } else {
            return "aexp-unknown-key: a selector item is anchor, reg, val, nth, ev, from, to or side";
        }
    }
    if ((s->a.set && (!s->a.has_reg || s->a.nth == 0u)) || (s->e.set && (!s->e.has_reg || s->e.nth == 0u))) {
        return "aexp-malformed: an anchor needs reg= and nth=";
    }
    return NULL;
}

#define AU_KV_MAX 16u
#define AU_WORDS_MAX 64u

struct au_lit {
    unsigned n;
    char key[AU_KV_MAX][16];
    char val[AU_KV_MAX][AX_TEXT];
};

/* the closed literal keys: the judged keys of the 05-04 kinds and the inputs of the 05-05 arithmetic lint (plan check 3 info 5) */
static const char *const au_keys[] = {"f", "k", "d", "nf", "g", "f2", "flf", "step", "w", "w1", "w2", "shift", "set", "m", "n", "r1", "al", "r5", "chon", "p0", "p1", "words", "minrun", "reg", "val", "vals", "chan", "effect", "wave_idx", "post"};

static const char *au_lit_parse(const char *text, struct au_lit *l)
{
    char buf[AX_TEXT];
    char *rest = buf;
    char *item;

    memset(l, 0, sizeof *l);
    if (strlen(text) >= sizeof buf) {
        return "aexp-malformed: a literal is too long";
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        unsigned i;
        int known = 0;

        if (eq == NULL || eq == item || eq[1] == '\0' || l->n >= AU_KV_MAX || (size_t)(eq - item) >= sizeof l->key[0]) {
            return "aexp-malformed: a literal is key=value items separated by semicolons";
        }
        *eq = '\0';
        for (i = 0u; i < (unsigned)(sizeof au_keys / sizeof au_keys[0]); i++) {
            known = known || strcmp(item, au_keys[i]) == 0;
        }
        for (i = 0u; i < l->n; i++) {
            if (strcmp(l->key[i], item) == 0) {
                return "aexp-malformed: a literal key appears twice";
            }
        }
        if (!known) {
            return "aexp-unknown-key: a literal key outside the closed list";
        }
        memcpy(l->key[l->n], item, strlen(item) + 1u);
        memcpy(l->val[l->n], eq + 1, strlen(eq + 1) + 1u);
        l->n++;
    }
    return NULL;
}

static const char *au_lit_get(const struct au_lit *l, const char *key)
{
    unsigned i;

    for (i = 0u; i < l->n; i++) {
        if (strcmp(l->key[i], key) == 0) {
            return l->val[i];
        }
    }
    return NULL;
}

/* a list of numbers separated by commas (decimal or $hex) into out (at most cap); 0 on success */
static int au_numlist(const char *text, uint64_t *out, unsigned cap, unsigned *n)
{
    char buf[AX_TEXT];
    char *rest = buf;
    char *item;

    *n = 0u;
    if (strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        if (*n >= cap || au_num(item, &out[*n]) != 0) {
            return -1;
        }
        (*n)++;
    }
    return *n >= 1u ? 0 : -1;
}

/* ---- the judge ---- */

enum { AU_PASS = 0, AU_FAIL = 1, AU_HARNESS = 2 };

/* the stream index of the n-th DSPE record that matches the anchor, or UINT32_MAX when there is none */
static uint32_t au_find(const struct au_cap *cp, const struct au_ev *ev)
{
    uint32_t i;
    uint64_t seen = 0u;

    for (i = 0u; i < cp->ecount; i++) {
        const uint8_t *r = cp->erec + (size_t)i * AU_E_REC;

        if (r[AU_E_CHAN] == ev->ch && r[AU_E_REG] == ev->reg && (!ev->has_val || r[AU_E_VALUE] == ev->val)) {
            seen++;
            if (seen == ev->nth) {
                return i;
            }
        }
    }
    return UINT32_MAX;
}

/* the first stream index of a sample at or after master clock t: ceil(t / sample_mclk) - first, saturated at 0 */
static uint64_t au_index(const struct au_cap *cp, uint64_t t)
{
    uint64_t s = t / cp->smclk + (t % cp->smclk != 0u ? 1u : 0u);

    return s > cp->first ? s - cp->first : 0u;
}

static uint32_t au_sample(const struct au_cap *cp, uint64_t i, int side)
{
    return au_u32(cp->frames + (size_t)i * AU_F_BYTES + (side == 2 ? 4u : 0u));
}

/* the window of a ROWS selector as stream indices [*i0, *i1); AU_PASS, AU_FAIL (an anchor not found, a window past the stream) or AU_HARNESS (a malformed or empty window), why set */
static int au_window(const struct au_cap *cp, const struct au_sel *s, uint64_t *i0, uint64_t *i1, char *why, size_t cap)
{
    uint64_t base = 0u;
    uint64_t from;
    uint64_t to;

    if (s->side == 0 || s->from_kind == 0 || s->to_kind == 0 || s->has_ev) {
        snprintf(why, cap, "aexp-malformed: a window needs side=, from= and to= and no ev=");
        return AU_HARNESS;
    }
    if (s->a.set) {
        uint32_t ai = au_find(cp, &s->a);

        if (ai == UINT32_MAX) {
            snprintf(why, cap, "the start anchor record is not in the DSPE stream");
            return AU_FAIL;
        }
        base = au_u64(cp->erec + (size_t)ai * AU_E_REC + AU_E_STAMP);
    }
    from = s->from_kind == 1 ? base + s->from : s->from;
    if (s->to_kind == 3) {
        uint32_t ei = au_find(cp, &s->e);

        if (ei == UINT32_MAX) {
            snprintf(why, cap, "the end anchor record is not in the DSPE stream");
            return AU_FAIL;
        }
        to = au_u64(cp->erec + (size_t)ei * AU_E_REC + AU_E_STAMP);
    } else {
        to = s->to_kind == 1 ? base + s->to : s->to;
    }
    *i0 = au_index(cp, from);
    *i1 = au_index(cp, to);
    if (*i0 >= *i1) {
        snprintf(why, cap, "the window [%llu, %llu) master clocks holds no sample: an empty judged window is a fault of the literal, never a pass", (unsigned long long)from, (unsigned long long)to);
        return AU_HARNESS;
    }
    if (*i1 > (uint64_t)cp->fcount) {
        snprintf(why, cap, "the window ends at sample %llu, past the %u frames of the stream", (unsigned long long)*i1, (unsigned)cp->fcount);
        return AU_FAIL;
    }
    return AU_PASS;
}

#define AU_CROSS_MAX 4096u

/* the rising crossings of the window on one side (threshold (min + max) / 2 written as 2x against min + max); the count, the first cap of them in cx */
static unsigned au_crossings(const struct au_cap *cp, uint64_t i0, uint64_t i1, int side, uint64_t *cx, unsigned cap)
{
    uint64_t lo = UINT64_MAX;
    uint64_t hi = 0u;
    uint64_t i;
    unsigned n = 0u;

    for (i = i0; i < i1; i++) {
        uint64_t x = au_sample(cp, i, side);

        lo = x < lo ? x : lo;
        hi = x > hi ? x : hi;
    }
    for (i = i0 + 1u; i < i1; i++) {
        uint64_t x = au_sample(cp, i, side);
        uint64_t p = au_sample(cp, i - 1u, side);

        if (2u * x >= lo + hi && 2u * p < lo + hi) {
            if (n < cap) {
                cx[n] = i;
            }
            n++;
        }
    }
    return n;
}

/* the dense rank of v among the n values of a (the number of distinct values below v) */
static unsigned au_rank(const uint64_t *a, unsigned n, uint64_t v)
{
    unsigned i;
    unsigned j;
    unsigned r = 0u;

    for (i = 0u; i < n; i++) {
        int dup = 0;

        if (a[i] >= v) {
            continue;
        }
        for (j = 0u; j < i; j++) {
            dup = dup || a[j] == a[i];
        }
        r += dup ? 0u : 1u;
    }
    return r;
}

static int au_rows_row(const struct au_cap *cp, const struct ax_group *g, const struct au_sel *s, const struct au_lit *l, char *why, size_t cap)
{
    uint64_t i0 = 0u;
    uint64_t i1 = 0u;
    uint64_t i;
    int w = au_window(cp, s, &i0, &i1, why, cap);

    if (w != AU_PASS) {
        return w;
    }
    if (g->kind == AK_SILENT) {
        for (i = i0; i < i1; i++) {
            if (au_sample(cp, i, s->side) != 0u) {
                snprintf(why, cap, "sample %llu of the window [%llu, %llu) reads %u on side %c, not 0", (unsigned long long)i, (unsigned long long)i0, (unsigned long long)i1, (unsigned)au_sample(cp, i, s->side), s->side == 1 ? 'L' : 'R');
                return AU_FAIL;
            }
        }
        snprintf(why, cap, "all %llu samples of the window are 0", (unsigned long long)(i1 - i0));
        return AU_PASS;
    }
    if (g->kind == AK_PERIOD) {
        uint64_t cx[AU_CROSS_MAX];
        uint64_t k = 0u;
        uint64_t d = 0u;
        unsigned n;

        if (au_lit_get(l, "k") == NULL || au_lit_get(l, "d") == NULL || au_num(au_lit_get(l, "k"), &k) != 0 || au_num(au_lit_get(l, "d"), &d) != 0 || k == 0u || k >= AU_CROSS_MAX) {
            snprintf(why, cap, "aexp-malformed: a period row needs k (1 to 4095) and d");
            return AU_HARNESS;
        }
        n = au_crossings(cp, i0, i1, s->side, cx, AU_CROSS_MAX);
        if ((uint64_t)n < k + 1u) {
            snprintf(why, cap, "%u rising crossings in the window [%llu, %llu), fewer than k + 1 = %llu", n, (unsigned long long)i0, (unsigned long long)i1, (unsigned long long)(k + 1u));
            return AU_FAIL;
        }
        snprintf(why, cap, "crossings %llu and %llu (the first and the %llu-th after it) are %llu samples apart, the literal is %llu", (unsigned long long)cx[0], (unsigned long long)cx[k], (unsigned long long)k, (unsigned long long)(cx[k] - cx[0]), (unsigned long long)d);
        return cx[k] - cx[0] == d ? AU_PASS : AU_FAIL;
    }
    {
        /* cycle_order: the plateaus between the first two rising crossings against the word ranks up to rotation (RF-14) */
        uint64_t cx[2];
        uint64_t words[AU_WORDS_MAX];
        uint64_t pl[AU_WORDS_MAX];
        unsigned nw = 0u;
        unsigned np = 0u;
        unsigned rot;
        unsigned j;

        if (au_lit_get(l, "words") == NULL || au_numlist(au_lit_get(l, "words"), words, AU_WORDS_MAX, &nw) != 0 || nw < 2u) {
            snprintf(why, cap, "aexp-malformed: a cycle_order row needs words= (2 to 64 numbers)");
            return AU_HARNESS;
        }
        if (au_crossings(cp, i0, i1, s->side, cx, 2u) < 2u) {
            snprintf(why, cap, "fewer than two rising crossings in the window [%llu, %llu): no whole cycle to order", (unsigned long long)i0, (unsigned long long)i1);
            return AU_FAIL;
        }
        for (i = cx[0]; i < cx[1]; i++) {
            uint64_t x = au_sample(cp, i, s->side);

            if (np > 0u && pl[np - 1u] == x) {
                continue;
            }
            if (np >= AU_WORDS_MAX) {
                snprintf(why, cap, "more than %u plateaus between the crossings %llu and %llu, the literal has %u words", AU_WORDS_MAX, (unsigned long long)cx[0], (unsigned long long)cx[1], nw);
                return AU_FAIL;
            }
            pl[np++] = x;
        }
        if (np != nw) {
            snprintf(why, cap, "%u plateaus between the crossings %llu and %llu, the literal has %u words", np, (unsigned long long)cx[0], (unsigned long long)cx[1], nw);
            return AU_FAIL;
        }
        for (rot = 0u; rot < np; rot++) {
            int same = 1;

            for (j = 0u; j < np && same; j++) {
                same = au_rank(pl, np, pl[(j + rot) % np]) == au_rank(words, nw, words[j]);
            }
            if (same) {
                snprintf(why, cap, "the %u plateaus between the crossings %llu and %llu rank as the words at rotation %u", np, (unsigned long long)cx[0], (unsigned long long)cx[1], rot);
                return AU_PASS;
            }
        }
        snprintf(why, cap, "the ranks of the %u plateaus between the crossings %llu and %llu are no rotation of the ranks of the words", np, (unsigned long long)cx[0], (unsigned long long)cx[1]);
        return AU_FAIL;
    }
}

static int au_ev_row(const struct au_cap *cp, const struct ax_group *g, const struct au_sel *s, const struct au_lit *l, char *why, size_t cap)
{
    uint32_t ai;
    uint64_t at;

    if (!s->a.set || !s->has_ev || s->from_kind != 0 || s->to_kind != 0 || s->side != 0) {
        snprintf(why, cap, "aexp-malformed: an EV row selects anchor=ev:...;ev=+K and nothing else");
        return AU_HARNESS;
    }
    ai = au_find(cp, &s->a);
    if (ai == UINT32_MAX) {
        snprintf(why, cap, "the anchor record is not in the DSPE stream");
        return AU_FAIL;
    }
    at = (uint64_t)ai + s->ev_off;
    if (g->kind == AK_EV_VALUE) {
        uint64_t vals[AU_WORDS_MAX];
        unsigned nv = 0u;
        unsigned reg = 0u;
        unsigned j;
        const char *rv = au_lit_get(l, "reg");
        const char *v1 = au_lit_get(l, "val");
        const char *vn = au_lit_get(l, "vals");

        if (rv == NULL || au_reg(rv, &reg) != 0 || (v1 == NULL) == (vn == NULL) || (v1 != NULL && (au_num(v1, &vals[0]) != 0 || vals[0] > 255u)) || (vn != NULL && au_numlist(vn, vals, AU_WORDS_MAX, &nv) != 0)) {
            snprintf(why, cap, "aexp-malformed: an ev_value row needs reg=R<n> and one of val=$hh or vals=$hh,...");
            return AU_HARNESS;
        }
        if (v1 != NULL) {
            nv = 1u;
        }
        for (j = 0u; j < nv; j++) {
            const uint8_t *r;

            if (at + j >= (uint64_t)cp->ecount) {
                snprintf(why, cap, "record %llu is past the %u DSPE records", (unsigned long long)(at + j), (unsigned)cp->ecount);
                return AU_FAIL;
            }
            r = cp->erec + (size_t)(at + j) * AU_E_REC;
            if (r[AU_E_REG] != reg || (uint64_t)r[AU_E_VALUE] != vals[j]) {
                snprintf(why, cap, "record %llu is reg %u value $%02X, the literal is reg %u value $%02X", (unsigned long long)(at + j), (unsigned)r[AU_E_REG], (unsigned)r[AU_E_VALUE], reg, (unsigned)vals[j]);
                return AU_FAIL;
            }
        }
        snprintf(why, cap, "%u record(s) from %llu match", nv, (unsigned long long)at);
        return AU_PASS;
    }
    {
        static const char *const keys[4] = {"chan", "effect", "wave_idx", "post"};
        uint64_t want[4];
        uint64_t got[4];
        const uint8_t *r;
        unsigned j;

        for (j = 0u; j < 4u; j++) {
            if (au_lit_get(l, keys[j]) == NULL || au_num(au_lit_get(l, keys[j]), &want[j]) != 0) {
                snprintf(why, cap, "aexp-malformed: an ev_decode row needs chan, effect, wave_idx and post");
                return AU_HARNESS;
            }
        }
        if (at >= (uint64_t)cp->ecount) {
            snprintf(why, cap, "record %llu is past the %u DSPE records", (unsigned long long)at, (unsigned)cp->ecount);
            return AU_FAIL;
        }
        r = cp->erec + (size_t)at * AU_E_REC;
        got[0] = r[AU_E_CHAN];
        got[1] = r[AU_E_EFFECT];
        got[2] = r[AU_E_WIDX];
        got[3] = au_u16(r + AU_E_POST);
        for (j = 0u; j < 4u; j++) {
            if (got[j] != want[j]) {
                snprintf(why, cap, "record %llu reads %s %llu, the literal is %llu", (unsigned long long)at, keys[j], (unsigned long long)got[j], (unsigned long long)want[j]);
                return AU_FAIL;
            }
        }
        if (r[AU_E_VERSION] != 1u) {
            snprintf(why, cap, "record %llu is not DSPE version 1", (unsigned long long)at);
            return AU_FAIL;
        }
        snprintf(why, cap, "record %llu decodes as the literal", (unsigned long long)at);
        return AU_PASS;
    }
}

/* one group of the scene on a capture: AU_PASS when every row passes; why holds the first failing row's reason (or the last row's) */
static int au_eval_group(const struct ax_file *f, unsigned gi, const struct au_cap *cp, char *why, size_t cap)
{
    const struct ax_group *g = &f->gr[gi];
    unsigned r;
    unsigned nrow = 0u;
    int out = AU_PASS;

    snprintf(why, cap, "no row");
    for (r = 0u; r < f->nr; r++) {
        const struct ax_row *row = &f->rw[r];
        struct au_sel s;
        struct au_lit l;
        const char *err;
        char rwhy[300];
        int v;

        if (row->group != gi) {
            continue;
        }
        nrow++;
        err = au_sel_parse(row->sel, &s);
        if (err == NULL) {
            err = au_lit_parse(row->lit, &l);
        }
        if (err != NULL) {
            snprintf(why, cap, "row %u: %s", nrow, err);
            return AU_HARNESS;
        }
        v = g->form == AX_FORM_EV ? au_ev_row(cp, g, &s, &l, rwhy, sizeof rwhy) : au_rows_row(cp, g, &s, &l, rwhy, sizeof rwhy);
        if (v == AU_HARNESS) {
            snprintf(why, cap, "row %u: %s", nrow, rwhy);
            return AU_HARNESS;
        }
        if (out == AU_PASS) {
            snprintf(why, cap, "row %u: %s", nrow, rwhy);
        }
        if (v == AU_FAIL) {
            out = AU_FAIL;
        }
    }
    if (nrow == 0u) {
        snprintf(why, cap, "aexp-malformed: the group has no row");
        return AU_HARNESS;
    }
    return out;
}

void kind_audio(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct au_expect e;
    struct ax_file *f = NULL;
    struct au_cap cap0;
    struct au_cap cap1;
    struct ctx s1;
    char req[600];
    char verdict1[64];
    char detail[4096];
    char pcm_hex[2u * DS_SHA256_BYTES + 1u];
    char psg_hex[2u * DS_SHA256_BYTES + 1u];
    uint64_t until = 0u;
    unsigned si;
    unsigned gi;
    unsigned ngroups = 0u;
    unsigned nred_seen = 0u;
    unsigned red_ok = 1u;
    int verdicts[AX_MAX];
    size_t used;
    int diff;

    memset(&cap0, 0, sizeof cap0);
    memset(&cap1, 0, sizeof cap1);
    if (!k->has[K_EXPECT] || au_parse(k->val[K_EXPECT], &e) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an audio case needs until_mclk and expect = rom=NAME;id=N;checks=N;scene=NAME[;expect_red=GID,...] (every item once)");
        return;
    }
    if (k->has[K_IRQ_STIM] || k->has[K_INPUT]) {
        harness_error(c, "an audio case declares no irq_stim and no input: neither is part of an audio scene");
        return;
    }
    f = (struct ax_file *)calloc(1u, sizeof *f);
    if (f == NULL) {
        harness_error(c, "out of memory for the aexp rows");
        return;
    }
    {
        const char *err = ax_load(e.rom, f);

        if (err != NULL) {
            harness_error(c, err);
            goto done;
        }
    }
    for (si = 0u; si < f->ns; si++) {
        if (strcmp(f->sc[si].name, e.scene) == 0) {
            break;
        }
    }
    if (si == f->ns || (strcmp(f->sc[si].case_id, k->val[K_ID]) != 0 && e.nred == 0u) || (strcmp(f->sc[si].case_id, k->val[K_ID]) == 0 && e.nred != 0u)) {
        harness_error(c, "the scene of the expect has no scene row, or its scene row names another case (only an expect_red binding may judge the scene of another case)");
        goto done;
    }
    if (f->sc[si].role != AX_ROLE_LITERAL) {
        harness_error(c, "the scene is not a literal scene: plan 05-04 judges literal scenes only (05-05 adds the golden role)");
        goto done;
    }
    /* the two policy runs of D-204: zero (judged) and seeded-prng-v4 seed 1 (identity) */
    if (vd_policy_request(0u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
        harness_error(c, "the request of the audio case is too long");
        goto done;
    }
    if (au_capture(c, e.rom, req, until, &e, &cap0, 1, got, gotcap) != 0) {
        goto done;
    }
    if (cap0.smclk != f->sample_mclk) {
        harness_error(c, "the aexp header sample_mclk does not equal the DSPA header's sample_mclk (D-207)");
        goto done;
    }
    if (vd_policy_request(1u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
        harness_error(c, "the request of the audio case is too long");
        goto done;
    }
    memset(&s1, 0, sizeof s1);
    if (au_capture(&s1, e.rom, req, until, &e, &cap1, 0, verdict1, sizeof verdict1) != 0) {
        harness_error(c, s1.why[0] != '\0' ? s1.why : "the seeded-prng-v4 seed 1 run of the audio case could not be captured");
        goto done;
    }
    check(c, !s1.failed, "the DSV1 verdict under seeded-prng-v4 seed 1 does not pass");
    diff = au_streams_differ(&cap0, &cap1);
    if (diff < 0) {
        harness_error(c, "out of memory for the audio streams");
        goto done;
    }
    check(c, diff == 0, diff == 1 ? "the DSPA stream under seeded-prng-v4 seed 1 differs from the zero policy's: the fixture plays a word it never wrote (D-204)" : "the DSPE stream under seeded-prng-v4 seed 1 differs from the zero policy's (D-204)");
    for (gi = 0u; gi < e.nred; gi++) {
        unsigned g2;
        int found = 0;

        for (g2 = 0u; g2 < f->ng; g2++) {
            found = found || (strcmp(f->gr[g2].scene, e.scene) == 0 && strcmp(f->gr[g2].gid, e.red[gi]) == 0);
        }
        if (!found) {
            harness_error(c, "an expect_red group is not a group of the scene");
            goto done;
        }
    }
    /* every group of the scene, evaluated and printed whatever the DSV1 verdict (Pitfall 4) */
    detail[0] = '\0';
    used = 0u;
    for (gi = 0u; gi < f->ng; gi++) {
        const struct ax_group *g = &f->gr[gi];
        char why[320];
        char line[600];
        unsigned r;
        int listed = 0;

        verdicts[gi] = AU_PASS;
        if (strcmp(g->scene, e.scene) != 0) {
            continue;
        }
        for (r = 0u; r < e.nred; r++) {
            listed = listed || strcmp(e.red[r], g->gid) == 0;
        }
        verdicts[gi] = au_eval_group(f, gi, &cap0, why, sizeof why);
        if (verdicts[gi] == AU_HARNESS) {
            char msg[420];

            snprintf(msg, sizeof msg, "group %.31s: %.360s", g->gid, why);
            harness_error(c, msg);
            goto done;
        }
        ngroups++;
        if (listed) {
            nred_seen++;
        }
        if ((verdicts[gi] == AU_FAIL) != listed) {
            red_ok = 0u;
        }
        snprintf(line, sizeof line, "audio_group %.31s %.95s %s%s: %.300s\n", g->gid, g->cls, verdicts[gi] == AU_PASS ? "pass" : "fail", listed ? " (expect_red)" : "", why);
        if (used + strlen(line) + 1u < sizeof detail) {
            memcpy(detail + used, line, strlen(line) + 1u);
            used += strlen(line);
        }
    }
    if (ngroups == 0u) {
        harness_error(c, "the scene of the case has no group: a literal case without a literal group never passes");
        goto done;
    }
    if (e.nred != 0u) {
        if (!red_ok || nred_seen != e.nred) {
            harness_error(c, "misprog-misattributed: the set of failing groups is not exactly the expect_red list (D-211)");
            set_evidence_detail(detail);
            goto done;
        }
        for (gi = 0u; gi < f->ng; gi++) {
            if (strcmp(f->gr[gi].scene, e.scene) == 0) {
                char msg[200];

                snprintf(msg, sizeof msg, "group %.31s %s as the expect_red binding requires", f->gr[gi].gid, verdicts[gi] == AU_FAIL ? "fails" : "passes");
                check(c, 1, msg);
            }
        }
    } else {
        for (gi = 0u; gi < f->ng; gi++) {
            if (strcmp(f->gr[gi].scene, e.scene) == 0) {
                char msg[200];

                snprintf(msg, sizeof msg, "group %.31s (%.95s) fails", f->gr[gi].gid, f->gr[gi].cls);
                check(c, verdicts[gi] == AU_PASS, msg);
            }
        }
    }
    if (au_digests(&cap0, pcm_hex, psg_hex) != 0) {
        harness_error(c, "out of memory for the audio streams");
        goto done;
    }
    {
        char head[700];

        snprintf(head, sizeof head, "pcm_sha256=%s psg_sha256=%s E0 repeatability (change detectors beside the literal groups, never a verdict, ER-20)\naudio_frames=%u audio_first=%llu audio_events=%u sample_mclk=%u machine_time=%llu policies=zero,seeded-prng-v4 seed 1 streams %s\n",
                 pcm_hex, psg_hex, (unsigned)cap0.fcount, (unsigned long long)cap0.first, (unsigned)cap0.ecount, (unsigned)cap0.smclk, (unsigned long long)cap0.machine_time, diff == 0 ? "identical" : "differ");
        if (strlen(head) + used + 1u <= sizeof detail) {
            memmove(detail + strlen(head), detail, used + 1u);
            memcpy(detail, head, strlen(head));
        }
        set_evidence_detail(detail);
    }
    {
        size_t gl = strlen(got);

        snprintf(got + gl, gotcap - gl, " frames=%u events=%u groups=%u", (unsigned)cap0.fcount, (unsigned)cap0.ecount, ngroups);
    }
done:
    au_cap_free(&cap0);
    au_cap_free(&cap1);
    free(f);
}
