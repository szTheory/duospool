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
#include "ds_serial.h"
#include "ds_sha256.h"
#include "ds_snapshot.h"
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

/* the streams of one run, accumulated over its ds_run calls (a chunked run or a restore appends): records, frames and dev records back to back, the first frame of the first call, and whether every later call continued the chain first(i+1) = first(i) + seq(i) */
struct au_cap {
    uint8_t *erec; /* the DSPE records */
    uint32_t ecount;
    uint32_t ecap;
    uint8_t *frames; /* the DSPA frames */
    uint32_t fcount;
    uint32_t fcap;
    uint64_t first;
    uint32_t smclk;
    uint8_t *drec; /* the dev records */
    uint32_t dcount;
    uint32_t dcap;
    uint32_t calls;
    int chain_bad;
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

/* the pcm capacity of D-201 for a run from t0 to the deadline until: ceil((until - t0 + ds_max_step_mclk()) / 192) + 1 frames */
static uint64_t au_pcm_capacity(uint64_t t0, uint64_t until)
{
    uint64_t span = (until > t0 ? until - t0 : 0u) + ds_max_step_mclk();

    return span / 192u + (span % 192u != 0u ? 1u : 0u) + 1u;
}

/* one ds_run call on w to until (with the DSV1 stop when dsv1 is set) whose psg, pcm and dev sinks point at the free tails of cp (allocated on first use: AU_PSG_CAP records, pcap frames, AU_DEV_CAP records); the call's records and frames are appended. A lost record or frame,
 * an inconsistent header or a refused request is a harness error on c (-1); 0 otherwise. res receives the result of the call. */
static int au_run(struct ctx *c, struct ds_instance *w, uint64_t until, int dsv1, uint64_t pcap, struct au_cap *cp, struct ds_run_result *res)
{
    struct ds_run_request rq;
    struct ds_psg_sink psk;
    struct ds_pcm_sink fsk;
    struct ds_dev_sink dsk;
    uint32_t ec;
    uint32_t fc;
    uint32_t dc;
    uint64_t first;

    if (cp->erec == NULL) {
        if (pcap == 0u || pcap > (uint64_t)DS_PCM_SINK_MAX_FRAMES) {
            harness_error(c, "the audio run is too long for one pcm sink (more than 8,388,608 frames)");
            return -1;
        }
        cp->erec = (uint8_t *)calloc((size_t)AU_PSG_CAP, (size_t)AU_E_REC);
        cp->frames = (uint8_t *)calloc((size_t)pcap, (size_t)AU_F_BYTES);
        cp->drec = (uint8_t *)calloc((size_t)AU_DEV_CAP, (size_t)AU_D_REC);
        cp->ecap = AU_PSG_CAP;
        cp->fcap = (uint32_t)pcap;
        cp->dcap = AU_DEV_CAP;
        if (cp->erec == NULL || cp->frames == NULL || cp->drec == NULL) {
            harness_error(c, "out of memory for the sinks of an audio run");
            return -1;
        }
    }
    memset(&psk, 0, sizeof psk);
    memset(&fsk, 0, sizeof fsk);
    memset(&dsk, 0, sizeof dsk);
    psk.records = cp->erec + (size_t)cp->ecount * AU_E_REC;
    psk.capacity = cp->ecap - cp->ecount;
    fsk.frames = cp->frames + (size_t)cp->fcount * AU_F_BYTES;
    fsk.capacity = cp->fcap - cp->fcount;
    dsk.records = cp->drec + (size_t)cp->dcount * AU_D_REC;
    dsk.capacity = cp->dcap - cp->dcount;
    memset(&rq, 0, sizeof rq);
    memset(res, 0, sizeof *res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.stop = dsv1 ? dsv1_done : NULL; /* a speed aid only (D-95): until_mclk is the hard bound */
    rq.psg = &psk;
    rq.pcm = &fsk;
    rq.dev = &dsk;
    if (ds_run(w, &rq, res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of an audio run");
        return -1;
    }
    ec = au_u32(psk.header + AU_H_COUNT);
    fc = au_u32(fsk.header + AU_H_COUNT);
    dc = au_u32(dsk.header + AU_H_COUNT);
    first = au_u64(fsk.header + AU_H_FIRST);
    if (au_u32(psk.header + AU_H_LOST) != 0u || au_u32(fsk.header + AU_H_LOST) != 0u || au_u32(dsk.header + AU_H_LOST) != 0u || au_u64(psk.header + AU_H_SEQ) != (uint64_t)ec || au_u64(fsk.header + AU_H_SEQ) != (uint64_t)fc ||
        au_u64(dsk.header + AU_H_SEQ) != (uint64_t)dc || ec > psk.capacity || fc > fsk.capacity || dc > dsk.capacity) {
        harness_error(c, "a sink of the audio run lost a record or a frame, or its header is inconsistent: the streams would be incomplete evidence (D-201)");
        return -1;
    }
    if (au_u16(fsk.header + AU_H_FBYTES) != AU_F_BYTES || fsk.header[AU_H_VERSION] != 1u || fsk.header[AU_H_FORMAT] != 1u || au_u32(fsk.header + AU_H_SMCLK) == 0u || (cp->calls > 0u && au_u32(fsk.header + AU_H_SMCLK) != cp->smclk)) {
        harness_error(c, "the pcm header does not read frame_bytes 8, version 1, format 1 and one sample_mclk above 0 (D-197)");
        return -1;
    }
    if (cp->calls == 0u) {
        cp->first = first;
        cp->smclk = au_u32(fsk.header + AU_H_SMCLK);
    } else if (first != cp->first + (uint64_t)cp->fcount) {
        cp->chain_bad = 1; /* first(i+1) = first(i) + seq(i) does not hold (D-200) */
    }
    cp->ecount += ec;
    cp->fcount += fc;
    cp->dcount += dc;
    cp->calls++;
    cp->machine_time = res->machine_time;
    cp->reason = res->reason;
    return 0;
}

/* boot ROM over request, run it to its DSV1 verdict or until with the three sinks, judge the DSV1 block on c (verdict text into got) and keep the streams in cp; 0 when the capture is usable, -1 after a harness error on c */
static int au_capture(struct ctx *c, const char *rom, const char *request, uint64_t until, const struct au_expect *e, struct au_cap *cp, int note, char *got, size_t gotcap)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_result res;
    const char *err;

    memset(cp, 0, sizeof *cp);
    err = boot_built_rom(rom, request, &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    if (au_run(c, (struct ds_instance *)(void *)b.mem, until, 1, au_pcm_capacity(ci->machine_time, until), cp, &res) != 0) {
        release_boot(&b);
        return -1;
    }
    if (note) {
        note_run(c, res.machine_time, res.reason);
    }
    judge_dsv1(c, ci->ram8k + V_OFF, (unsigned)e->id, (unsigned)e->checks, got, gotcap);
    release_boot(&b);
    if (c->harness || c->timeout) {
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

/* ---- the D-200 proof set kinds of the audio file (plan 05-04 Task 2): aud_noninterference, psg_dev_join, snapshot_restore_audio, aud_poweron_policy_diff ---- */

/* the expect rom=NAME of a proof-set case (the built image build/gate/fixtures/NAME.pce), with an optional second item key=VALUE copied into extra (NULL when the case needs none); 0 on success */
static int au_rom_expect(const struct kase *k, char *rom, size_t romcap, const char *key, char *extra, size_t xcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have_rom = 0;
    int have_extra = key == NULL;

    if (!k->has[K_EXPECT] || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        const char *v;

        if ((v = ax_val(item, "rom")) != NULL && !have_rom && strlen(v) > 0u && strlen(v) < romcap && strspn(v, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(v)) {
            memcpy(rom, v, strlen(v) + 1u);
            have_rom = 1;
        } else if (key != NULL && (v = ax_val(item, key)) != NULL && !have_extra && strlen(v) > 0u && strlen(v) < xcap) {
            memcpy(extra, v, strlen(v) + 1u);
            have_extra = 1;
        } else {
            return -1;
        }
    }
    return have_rom && have_extra ? 0 : -1;
}

/* the end state of a run that a sink must not change (D-200): reason, machine time, steps, verdict RAM and state digest */
struct au_end {
    uint32_t reason;
    uint64_t mclk;
    uint64_t steps;
    uint8_t verdict[V_SIZE];
    uint8_t digest[DS_DIGEST_BYTES];
    int digest_ok;
};

static void au_end_of(struct au_end *e, const struct ds_instance *w, const struct ds_run_result *res)
{
    e->reason = res->reason;
    e->mclk = res->machine_time;
    e->steps = res->steps;
    memcpy(e->verdict, w->ram8k + V_OFF, V_SIZE);
    e->digest_ok = ds_digest(w, e->digest) == 0u;
}

static int au_end_same(const struct au_end *a, const struct au_end *b)
{
    return a->reason == b->reason && a->mclk == b->mclk && a->steps == b->steps && memcmp(a->verdict, b->verdict, V_SIZE) == 0 && a->digest_ok && b->digest_ok && memcmp(a->digest, b->digest, DS_DIGEST_BYTES) == 0;
}

/* aud_noninterference (D-200): psg_tone run to its DSV1 verdict with the psg, pcm and dev sinks on and with no sink, under zero and seeded-prng-v4 seeds 1 and 2: identical stop reason, machine time, steps, verdict RAM and state digest; the sink runs must have produced
 * records and frames, so the comparison is not vacuous. The psg and pcm pointers are read only at the gate and the emits; the toy psg-emit-mutates turns this case red. */
void kind_aud_noninterference(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    uint64_t until = 0u;
    unsigned i;
    uint64_t frames = 0u;
    uint64_t events = 0u;

    if (au_rom_expect(k, rom, sizeof rom, NULL, NULL, 0u) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an aud_noninterference case needs until_mclk and expect = rom=NAME");
        return;
    }
    for (i = 0u; i < VD_POLICIES && !c->harness; i++) {
        char req[600];
        struct booted b0;
        struct booted b1;
        const struct ds_instance *c0 = NULL;
        const struct ds_instance *c1 = NULL;
        struct ds_run_request rq;
        struct ds_run_result r0;
        struct ds_run_result r1;
        struct au_end e0;
        struct au_end e1;
        struct au_cap cp;
        const char *err;
        char msg[200];

        memset(&cp, 0, sizeof cp);
        if (vd_policy_request(i, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
            harness_error(c, "the request of the aud_noninterference case is too long");
            return;
        }
        err = boot_built_rom(rom, req, &b0, &c0);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        err = boot_built_rom(rom, req, &b1, &c1);
        if (err != NULL) {
            release_boot(&b0);
            harness_error(c, err);
            return;
        }
        memset(&rq, 0, sizeof rq);
        memset(&r0, 0, sizeof r0);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.stop = dsv1_done;
        (void)ds_run((struct ds_instance *)(void *)b0.mem, &rq, &r0);
        au_end_of(&e0, c0, &r0);
        if (au_run(c, (struct ds_instance *)(void *)b1.mem, until, 1, au_pcm_capacity(c1->machine_time, until), &cp, &r1) == 0) {
            au_end_of(&e1, c1, &r1);
            snprintf(msg, sizeof msg, "under %s the psg, pcm and dev sinks changed the stop reason, machine time, steps, verdict RAM or state digest of the run", vd_policy_name(i));
            check(c, au_end_same(&e0, &e1), msg);
            snprintf(msg, sizeof msg, "under %s the sink run produced no DSPE record or no DSPA frame: the comparison would be vacuous", vd_policy_name(i));
            check(c, cp.ecount > 0u && cp.fcount > 0u, msg);
            frames += cp.fcount;
            events += cp.ecount;
            if (i == 0u) {
                note_run(c, r1.machine_time, r1.reason);
            }
        }
        au_cap_free(&cp);
        release_boot(&b0);
        release_boot(&b1);
    }
    snprintf(got, gotcap, "policies=%u frames=%llu events=%llu", (unsigned)VD_POLICIES, (unsigned long long)frames, (unsigned long long)events);
}

/* the DSPE records of cp joined 1:1 and in order with the dev records of window-2 writes on (stamp, pc, value, phys & $F) (D-196, F-66); 0 when the join holds, else the index of the first mismatch + 1 into *at */
static int au_join(const struct au_cap *cp, uint32_t *window_writes, uint32_t *at)
{
    uint32_t i;
    uint32_t j = 0u;

    *window_writes = 0u;
    *at = 0u;
    for (i = 0u; i < cp->dcount; i++) {
        const uint8_t *d = cp->drec + (size_t)i * AU_D_REC;
        uint32_t phys = au_u32(d + 8);
        const uint8_t *e;

        if (phys < 0x1FE800u || phys >= 0x1FEC00u || d[13] == 2u) { /* not window 2, or a read (kind 2): a read of the PSG window is not logged and makes no DSPE record */
            continue;
        }
        (*window_writes)++;
        if (j >= cp->ecount) {
            *at = i + 1u;
            return -1;
        }
        e = cp->erec + (size_t)j * AU_E_REC;
        if (au_u64(e + AU_E_STAMP) != au_u64(d) || au_u16(e + AU_E_PC) != au_u16(d + 16) || e[AU_E_VALUE] != d[12] || e[AU_E_REG] != (phys & 0x0Fu)) {
            *at = i + 1u;
            return -1;
        }
        j++;
    }
    if (j != cp->ecount) {
        *at = cp->dcount + 1u;
        return -1;
    }
    return 0;
}

/* ---- a synthetic image built in test code (the randprog.c layout: bank 0 at logical $E000 under the reset mapping, the reset vector $FFFE -> $E000, every other vector and the end of the program at a spin loop at $FFF0) ---- */

#define AU_IMG_BANK 0x2000u
#define AU_IMG_SPIN 0x1FF0u

static void au_img_tail(unsigned char *img, size_t at)
{
    unsigned v;

    img[at] = 0x4Cu; /* JMP $FFF0 */
    img[at + 1u] = 0xF0u;
    img[at + 2u] = 0xFFu;
    img[AU_IMG_SPIN] = 0x80u; /* BRA to itself */
    img[AU_IMG_SPIN + 1u] = 0xFEu;
    for (v = 0u; v < 4u; v++) {
        img[0x1FF6u + 2u * v] = 0xF0u;
        img[0x1FF7u + 2u * v] = 0xFFu;
    }
    img[0x1FFEu] = 0x00u;
    img[0x1FFFu] = 0xE0u;
}

/* LDA #v; STA $08rr (a PSG register write at logical $0800 + off while MPR0 = $FF) */
static void au_img_write(unsigned char *img, size_t *at, unsigned off, unsigned v)
{
    img[(*at)++] = 0xA9u;
    img[(*at)++] = (unsigned char)(v & 0xFFu);
    img[(*at)++] = 0x8Du;
    img[(*at)++] = (unsigned char)(off & 0xFFu);
    img[(*at)++] = 0x08u;
}

static void au_img_head(unsigned char *img, size_t *at)
{
    memset(img, 0, RP_IMAGE_BYTES);
    *at = 0u;
    img[(*at)++] = 0xA9u; /* LDA #$FF; TAM #$01: MPR0 = $FF, the hardware page at logical $0000 */
    img[(*at)++] = 0xFFu;
    img[(*at)++] = 0x53u;
    img[(*at)++] = 0x01u;
}

/* the join program (GUEST:program, counted by hand): 2 reads of the window, then 7 writes (R0 = 0; R7 at select 0, reg $A and reg $F: no target; $0810, the mirror of R0, = 6; R2 at select 6: no target; R0 = 1), a TII of 8 bytes from $E100 to $0806..$080D (8 writes:
 * R6, R7 at select 1, R8, R9, reg $A..$D) and one more read: 15 window-2 writes, so 15 DSPE records, and no record for the 3 reads */
#define AU_JOIN_WRITES 15u

static void au_img_join(unsigned char *img)
{
    size_t at;
    unsigned i;

    au_img_head(img, &at);
    img[at++] = 0xADu; /* LDA $0800 */
    img[at++] = 0x00u;
    img[at++] = 0x08u;
    img[at++] = 0xADu; /* LDA $0809 */
    img[at++] = 0x09u;
    img[at++] = 0x08u;
    au_img_write(img, &at, 0x00u, 0x00u);
    au_img_write(img, &at, 0x07u, 0x55u);
    au_img_write(img, &at, 0x0Au, 0x11u);
    au_img_write(img, &at, 0x0Fu, 0x22u);
    au_img_write(img, &at, 0x10u, 0x06u);
    au_img_write(img, &at, 0x02u, 0x33u);
    au_img_write(img, &at, 0x00u, 0x01u);
    img[at++] = 0x73u; /* TII $E100, $0806, 8 */
    img[at++] = 0x00u;
    img[at++] = 0xE1u;
    img[at++] = 0x06u;
    img[at++] = 0x08u;
    img[at++] = 0x08u;
    img[at++] = 0x00u;
    img[at++] = 0xADu; /* LDA $0806 */
    img[at++] = 0x06u;
    img[at++] = 0x08u;
    au_img_tail(img, at);
    for (i = 0u; i < 8u; i++) {
        img[0x0100u + i] = (unsigned char)(0x41u + 0x11u * i);
    }
}

/* psg_dev_join (D-196, D-200, F-66): DSPE joins dev 1:1 on (stamp, pc, value, phys & $F) for the ROM of the case, and for the in-memory join program, whose 15 window-2 writes (block bytes and undecoded writes included) must give exactly 15 DSPE records and whose reads give none */
void kind_psg_dev_join(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    uint64_t until = 0u;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct au_cap cp;
    struct ds_run_result res;
    unsigned char *img;
    uint32_t ww = 0u;
    uint32_t at = 0u;
    uint32_t rom_events = 0u;
    const char *err;
    char msg[200];

    if (au_rom_expect(k, rom, sizeof rom, NULL, NULL, 0u) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "a psg_dev_join case needs until_mclk and expect = rom=NAME");
        return;
    }
    memset(&cp, 0, sizeof cp);
    err = boot_built_rom(rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (au_run(c, (struct ds_instance *)(void *)b.mem, until, 1, au_pcm_capacity(ci->machine_time, until), &cp, &res) == 0) {
        note_run(c, res.machine_time, res.reason);
        snprintf(msg, sizeof msg, "the DSPE records of %.40s do not join the dev records of window-2 writes 1:1 on stamp, pc, value and register (first mismatch near dev record %u)", rom, (unsigned)at);
        check(c, au_join(&cp, &ww, &at) == 0, msg);
        check(c, ww == cp.ecount && ww > 0u, "the ROM made no window-2 write, or the DSPE count is not the count of its window-2 writes");
        rom_events = cp.ecount;
    }
    au_cap_free(&cp);
    release_boot(&b);
    if (c->harness) {
        return;
    }
    img = (unsigned char *)malloc(RP_IMAGE_BYTES);
    if (img == NULL) {
        harness_error(c, "out of memory for the join program");
        return;
    }
    au_img_join(img);
    err = boot_random(img, "", &b, &ci);
    free(img);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (au_run(c, (struct ds_instance *)(void *)b.mem, 20000u, 0, au_pcm_capacity(ci->machine_time, 20000u), &cp, &res) == 0) {
        check(c, au_join(&cp, &ww, &at) == 0, "the DSPE records of the join program do not join its dev records of window-2 writes 1:1");
        snprintf(msg, sizeof msg, "the join program makes %u DSPE records and %u logged window-2 writes; its 15 writes (7 plain, 8 by TII, the undecoded ones included) must give exactly 15 and its 3 reads none", (unsigned)cp.ecount, (unsigned)ww);
        check(c, cp.ecount == AU_JOIN_WRITES && ww == AU_JOIN_WRITES, msg);
    }
    au_cap_free(&cp);
    release_boot(&b);
    snprintf(got, gotcap, "rom_events=%u program_events=%u", (unsigned)rom_events, (unsigned)AU_JOIN_WRITES);
}

/* the policy-diff program (D-204, D-212): select 3, F = 256, R1 $FF, R5 $FF, key-on with AL $1F, and its wave RAM never written, so what it plays is the power-on wave RAM of the policy */
static void au_img_poweron(unsigned char *img)
{
    size_t at;

    au_img_head(img, &at);
    au_img_write(img, &at, 0x00u, 0x03u);
    au_img_write(img, &at, 0x02u, 0x00u);
    au_img_write(img, &at, 0x03u, 0x01u);
    au_img_write(img, &at, 0x01u, 0xFFu);
    au_img_write(img, &at, 0x05u, 0xFFu);
    au_img_write(img, &at, 0x04u, 0x9Fu);
    au_img_tail(img, at);
}

/* aud_poweron_policy_diff (D-204, D-212, R4): the in-memory program plays a channel whose wave RAM it never wrote. Its DSPA stream must differ between seeded-prng-v4 seeds 1 and 2 and between zero and v4 seed 1, and must be identical between zero and seeded-prng-v3 seed 1
 * (v3 draws no wave). The differences are reported by au_streams_differ, the identity check every audio literal case relies on, so that check is shown able to fail. */
void kind_aud_poweron_policy_diff(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const pol[4] = {"", "bind.powerup_policy=seeded-prng-v4;seed=1", "bind.powerup_policy=seeded-prng-v4;seed=2", "bind.powerup_policy=seeded-prng-v3;seed=1"};
    struct au_cap cp[4];
    unsigned char *img;
    uint64_t until = 0u;
    unsigned i;
    int d01;
    int d12;
    int d03;

    memset(cp, 0, sizeof cp);
    if (!k->has[K_EXPECT] || strcmp(k->val[K_EXPECT], "program=sel3-keyon-no-wave") != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 100000u) {
        harness_error(c, "an aud_poweron_policy_diff case needs until_mclk (at least 100,000) and expect = program=sel3-keyon-no-wave");
        return;
    }
    img = (unsigned char *)malloc(RP_IMAGE_BYTES);
    if (img == NULL) {
        harness_error(c, "out of memory for the policy-diff program");
        return;
    }
    au_img_poweron(img);
    for (i = 0u; i < 4u && !c->harness; i++) {
        struct booted b;
        const struct ds_instance *ci = NULL;
        struct ds_run_result res;
        const char *err = boot_random(img, pol[i], &b, &ci);

        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        (void)au_run(c, (struct ds_instance *)(void *)b.mem, until, 0, au_pcm_capacity(ci->machine_time, until), &cp[i], &res);
        release_boot(&b);
    }
    free(img);
    if (!c->harness) {
        d12 = au_streams_differ(&cp[1], &cp[2]);
        d01 = au_streams_differ(&cp[0], &cp[1]);
        d03 = au_streams_differ(&cp[0], &cp[3]);
        check(c, d12 == 1, "the DSPA streams of seeded-prng-v4 seeds 1 and 2 do not differ: the identity check of the audio kind could not see a played power-on wave");
        check(c, d01 == 1, "the zero-versus-v4 identity check of the audio kind does not report a difference between zero and seeded-prng-v4 seed 1 for a channel keyed on without an upload (D-204)");
        check(c, d03 == 0, "the DSPA streams of zero and seeded-prng-v3 seed 1 differ: v3 draws no wave RAM");
        check(c, cp[0].fcount > 0u && cp[1].fcount == cp[0].fcount, "the policy runs produced no frames or different frame counts");
        snprintf(got, gotcap, "v4s1_v4s2=%s zero_v4s1=%s zero_v3s1=%s frames=%u", d12 == 1 ? "differ" : "same", d01 == 1 ? "differ" : "same", d03 == 0 ? "same" : "differ", (unsigned)cp[0].fcount);
    }
    for (i = 0u; i < 4u; i++) {
        au_cap_free(&cp[i]);
    }
}

/* ---- snapshot_restore_audio (D-200; the kind_snapshot_restore_video shape) ---- */

#define AU_INSN_CAP 262144u

/* the stamp of the n-th DSPE record of register reg (any channel), with value val when val <= 255; UINT64_MAX when there is none */
static uint64_t au_stamp_of(const struct au_cap *cp, unsigned reg, unsigned val, unsigned n)
{
    uint32_t i;
    unsigned seen = 0u;

    for (i = 0u; i < cp->ecount; i++) {
        const uint8_t *r = cp->erec + (size_t)i * AU_E_REC;

        if (r[AU_E_REG] == reg && (val > 255u || r[AU_E_VALUE] == val) && ++seen == n) {
            return au_u64(r + AU_E_STAMP);
        }
    }
    return UINT64_MAX;
}

/* the first instruction start (a step boundary) at or after t whose residue mod 192 is rlo..rhi, from the insn records of the reference run; UINT64_MAX when there is none */
static uint64_t au_step_at(const uint8_t *insn, uint32_t n, uint64_t t, uint64_t rlo, uint64_t rhi)
{
    uint32_t i;

    for (i = 0u; i < n; i++) {
        uint64_t s = au_u64(insn + (size_t)i * DS_INSN_RECORD_BYTES);

        if (s >= t && s % 192u >= rlo && s % 192u <= rhi) {
            return s;
        }
    }
    return UINT64_MAX;
}

/* restore at stops chosen by machine time on the ROM of the case: at a sample instant (a step boundary on the 192 grid), mid-interval (a boundary 64 to 128 clocks past one), between the R2 and R3 writes, between the R0 and R4 writes of the scene, and inside the TIN to $0806
 * (after its 10th byte). The prefix run stops there, a snapshot is restored into a poisoned second boot, and the resumed run's DSPE records and DSPA frames appended to the prefix must equal the whole run's streams byte for byte, with first(i+1) = first(i) + seq(i), the same end
 * machine time and the same end digest. 05-07 and 05-09 add the DDA, noise and LFO TRG stops. */
void kind_snapshot_restore_audio(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const names[5] = {"sample", "mid", "r2r3", "r0r4", "tin"};
    char rom[48];
    char stops[96];
    uint64_t until = 0u;
    const char *req = k->has[K_REQUEST] ? k->val[K_REQUEST] : "";
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_insn_sink isk;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct au_cap ref;
    struct au_end eref;
    uint8_t *insn = NULL;
    uint8_t *snap = NULL;
    uint32_t ninsn = 0u;
    uint64_t at[5];
    unsigned i;
    unsigned done = 0u;
    const char *err;

    memset(&ref, 0, sizeof ref);
    if (au_rom_expect(k, rom, sizeof rom, "stops", stops, sizeof stops) != 0 || strcmp(stops, "sample,mid,r2r3,r0r4,tin") != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 400000u) {
        harness_error(c, "a snapshot_restore_audio case needs until_mclk (at least 400,000) and expect = rom=NAME;stops=sample,mid,r2r3,r0r4,tin");
        return;
    }
    insn = (uint8_t *)calloc((size_t)AU_INSN_CAP, (size_t)DS_INSN_RECORD_BYTES);
    snap = (uint8_t *)malloc(ds_snapshot_size());
    if (insn == NULL || snap == NULL) {
        free(insn);
        free(snap);
        harness_error(c, "out of memory for the restore kind");
        return;
    }
    /* the reference: the whole run with the audio sinks, and once more with an instruction sink for the step boundaries */
    err = boot_built_rom(rom, req, &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        goto out;
    }
    if (au_run(c, (struct ds_instance *)(void *)b.mem, until, 0, au_pcm_capacity(ci->machine_time, until), &ref, &res) != 0) {
        release_boot(&b);
        goto out;
    }
    au_end_of(&eref, ci, &res);
    note_run(c, res.machine_time, res.reason);
    release_boot(&b);
    err = boot_built_rom(rom, req, &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        goto out;
    }
    memset(&isk, 0, sizeof isk);
    isk.records = insn;
    isk.capacity = AU_INSN_CAP;
    memset(&rq, 0, sizeof rq);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.insn = &isk;
    (void)ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    release_boot(&b);
    ninsn = au_u32(isk.header);
    if (au_u32(isk.header + 4) != 0u) {
        harness_error(c, "the instruction sink of the restore kind lost a record");
        goto out;
    }
    /* the stop deadlines: a run stops at the first step end at or after its deadline, so a deadline one clock past a write stamp stops after that write's step */
    at[0] = au_step_at(insn, ninsn, until / 2u, 0u, 0u);
    at[1] = au_step_at(insn, ninsn, until / 2u, 64u, 128u);
    at[2] = au_stamp_of(&ref, 2u, 256u, 1u);
    at[3] = au_stamp_of(&ref, 0u, 0u, 2u);
    at[4] = au_stamp_of(&ref, 6u, 256u, 10u);
    for (i = 2u; i < 5u; i++) {
        at[i] = at[i] != UINT64_MAX ? at[i] + 1u : UINT64_MAX;
    }
    for (i = 0u; i < 5u && !c->harness; i++) {
        struct booted a;
        struct booted t;
        const struct ds_instance *ca = NULL;
        const struct ds_instance *ct = NULL;
        struct au_cap cp;
        struct au_end e;
        int placed;
        char msg[240];

        if (at[i] == UINT64_MAX || at[i] >= until) {
            snprintf(msg, sizeof msg, "the restore stop %s has no deadline inside the run (no step boundary or write found)", names[i]);
            harness_error(c, msg);
            break;
        }
        memset(&cp, 0, sizeof cp);
        err = boot_built_rom(rom, req, &a, &ca);
        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        err = boot_built_rom(rom, req, &t, &ct);
        if (err != NULL) {
            release_boot(&a);
            harness_error(c, err);
            break;
        }
        if (au_run(c, (struct ds_instance *)(void *)a.mem, at[i], 0, au_pcm_capacity(ca->machine_time, until), &cp, &res) == 0) {
            uint64_t m = ca->machine_time;

            placed = i == 0u ? m % 192u == 0u : i == 1u ? m % 192u != 0u : i == 2u ? m > at[2] - 1u && m <= au_stamp_of(&ref, 3u, 256u, 1u) : i == 3u ? m > at[3] - 1u && m <= au_stamp_of(&ref, 4u, 0x40u, 1u) : ca->blk_active != 0u;
            snprintf(msg, sizeof msg, "the restore stop %s landed at machine time %llu, not where it was chosen (a sample instant, mid-interval, between the two writes, inside the TIN)", names[i], (unsigned long long)m);
            check(c, placed, msg);
            if (ds_snapshot(ca, snap, ds_snapshot_size(), NULL) != (uint32_t)DS_SNAPREF_NONE) {
                check(c, 0, "ds_snapshot at an audio restore stop was refused");
            } else {
                snap_poison((struct ds_instance *)(void *)t.mem, (i & 1u) != 0u ? 0x5Au : 0xA5u);
                if (ds_restore((struct ds_instance *)(void *)t.mem, snap, ds_snapshot_size()) != (uint32_t)DS_SNAPREF_NONE) {
                    check(c, 0, "ds_restore refused the snapshot of an audio restore stop");
                } else if (au_run(c, (struct ds_instance *)(void *)t.mem, until, 0, 0u, &cp, &res) == 0) {
                    au_end_of(&e, ct, &res);
                    snprintf(msg, sizeof msg, "restore at %s (machine time %llu): the prefix and the resumed streams do not equal the whole run byte for byte, or first(i+1) != first(i) + seq(i)", names[i], (unsigned long long)m);
                    check(c, au_streams_differ(&ref, &cp) == 0 && !cp.chain_bad && cp.calls == 2u, msg);
                    snprintf(msg, sizeof msg, "restore at %s: the resumed run ends at another machine time or digest than the whole run", names[i]);
                    check(c, e.mclk == eref.mclk && e.digest_ok && eref.digest_ok && memcmp(e.digest, eref.digest, DS_DIGEST_BYTES) == 0, msg);
                    done++;
                }
            }
        }
        au_cap_free(&cp);
        release_boot(&a);
        release_boot(&t);
    }
    snprintf(got, gotcap, "restores=%u frames=%u events=%u", done, (unsigned)ref.fcount, (unsigned)ref.ecount);
out:
    au_cap_free(&ref);
    free(insn);
    free(snap);
}
