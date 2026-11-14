/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * audiokinds.c - the audio case kind (plan 05-04, grown by 05-05..05-07 and 05-10; the LFO judges come with 05.1-01; D-183, D-187, D-196, D-197, D-199, D-201, D-204, D-206..D-208, D-211, D-230, ER-19, ER-20, ER-21, PR-13, RF-13, RF-14, F-85, F-86). Test code only.
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
 * Plan 05-05 grammar: every kind of tests/audio/KINDCLASS is parsed with its form (an unknown kind or a form that is not the kind's is refused); every kind has its judge here since 05.1-02 (lfo_dynamic, the last), so none is judged from a later plan. A row golden SCENE pcm_sha256=HEX psg_sha256=HEX
 * pins the digests of a golden scene; class DEF:<LED ids> marks a definition (ER-19); selector items stream=dspa|dspe (the STRUCT header rows) and nth=N+M on the end anchor (an offset of M master clocks, PR-13); an expect_red scene (D-211, D-225) carries twin= and expect_red= and the
 * case judging it takes the red list from the scene row. Forms added in 05-05: REL (period_ratio, same, atten_order, atten_equal over the windows of a group's rows in order), STRUCT (header, count) and the ROWS kind live (two rising crossings and at least 3 distinct levels between them).
 * The kind audio_fault_sweep applies AF0..AF9 of D-209 in memory to copies of the captured streams of one run; audio_judge_forms shows every form able to fail (05-05).
 * levels_exact (05-06): the largest sample of the window on that side equals the literal peak (ASSUMED:LED-AUD-0009).
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
#include "ds_psg.h" /* ds_psg_divisor for the coupled restore stop of plan 05.1-02, as psgkinds.c includes it */
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
enum {
    AK_EV_VALUE = 1,
    AK_EV_DECODE,
    AK_EV_EDGE,
    AK_PERIOD,
    AK_PERIOD_RATIO,
    AK_CYCLE_ORDER,
    AK_CYCLE_PHASE,
    AK_LEVELS_EXACT,
    AK_SILENT,
    AK_LIVE,
    AK_SAME,
    AK_ATTEN_ORDER,
    AK_ATTEN_EQUAL,
    AK_DDA_HOLD,
    AK_DDA_ORDER,
    AK_DDA_RAM_KEPT,
    AK_DDA_LEVEL_EXACT,
    AK_TWO_LEVEL,
    AK_RUN_GRID,
    AK_NOISE_RATIO,
    AK_RUN_MIN,
    AK_NOISE_SEQ,
    AK_LFO_STATIC,
    AK_LFO_NEEDS_SEL1,
    AK_LFO_DYNAMIC,
    AK_COUNT,
    AK_HEADER,
    AK_END
};

/* the kinds of tests/audio/KINDCLASS with their form (scripts/audio-check reads the same table as data); every kind is judged here (plan 05-07 judges the noise and DDA kinds and cycle_phase; Phase 5.1 plan 05.1-01 judges lfo_static (the period judge on k and d,
 * d = k x F' checked by the lint) and lfo_needs_sel1 (the same judge), 05.1-02 lfo_dynamic (the run and set rule), so no kind is judged from a later plan any more; the close-out plan 05-10 dropped the two kinds that no plan had a row for) */
static const struct au_kind_def {
    const char *name;
    int form;
} au_kinds[] = {
    {"ev_value", AX_FORM_EV},          {"ev_decode", AX_FORM_EV},      {"ev_edge", AX_FORM_EV},         {"period", AX_FORM_ROWS},       {"period_ratio", AX_FORM_REL},    {"cycle_order", AX_FORM_ROWS}, {"cycle_phase", AX_FORM_ROWS},
    {"levels_exact", AX_FORM_ROWS},    {"silent", AX_FORM_ROWS},       {"live", AX_FORM_ROWS},          {"same", AX_FORM_REL},          {"atten_order", AX_FORM_REL},     {"atten_equal", AX_FORM_REL},  {"dda_hold", AX_FORM_ROWS},
    {"dda_order", AX_FORM_ROWS},       {"dda_ram_kept", AX_FORM_ROWS}, {"dda_level_exact", AX_FORM_ROWS}, {"two_level", AX_FORM_ROWS},  {"run_grid", AX_FORM_ROWS},       {"noise_ratio", AX_FORM_REL},  {"run_min", AX_FORM_ROWS},
    {"noise_seq", AX_FORM_ROWS},       {"lfo_static", AX_FORM_ROWS},   {"lfo_needs_sel1", AX_FORM_REL}, {"lfo_dynamic", AX_FORM_ROWS},  {"count", AX_FORM_STRUCT},        {"header", AX_FORM_STRUCT},
};

struct ax_scene {
    char name[AU_NAME];
    char case_id[72];
    int role;
    char twin[AU_NAME];
    char red[AX_TEXT]; /* the expect_red= list of an expect_red scene (D-211) */
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

struct ax_golden {
    char scene[AU_NAME];
    char pcm[65];
    char psg[65];
};

struct ax_file {
    uint64_t sample_mclk;
    unsigned nh;
    unsigned ns;
    unsigned ng;
    unsigned nr;
    unsigned ngold;
    struct ax_scene sc[AX_MAX];
    struct ax_group gr[AX_MAX];
    struct ax_row rw[AX_MAX];
    struct ax_golden gd[4];
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
    return strncmp(text, "MAN:", 4u) == 0 || strncmp(text, "GUEST:", 6u) == 0 || strncmp(text, "ASSUMED:", 8u) == 0 || strncmp(text, "DEF:", 4u) == 0;
}

static int ax_kind(const char *text)
{
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof au_kinds / sizeof au_kinds[0]); i++) {
        if (strcmp(text, au_kinds[i].name) == 0) {
            return (int)i + 1;
        }
    }
    return 0;
}

static int ax_hex64(const char *s)
{
    return strlen(s) == 64u && strspn(s, "0123456789abcdef") == 64u;
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

            if ((t == NULL || !au_name_ok(t, AU_NAME)) && (r == NULL || strlen(r) == 0u || strlen(r) >= AX_TEXT)) {
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
        for (i = 4u; i < nf; i++) {
            const char *t = ax_val(fld[i], "twin");
            const char *r = ax_val(fld[i], "expect_red");

            if (t != NULL) {
                memcpy(s->twin, t, strlen(t) + 1u);
            }
            if (r != NULL) {
                memcpy(s->red, r, strlen(r) + 1u);
            }
        }
        if (strcmp(b, "literal") == 0) {
            s->role = AX_ROLE_LITERAL;
        } else if (strcmp(b, "golden") == 0) {
            s->role = AX_ROLE_GOLDEN;
        } else if (strcmp(b, "expect_red") == 0) {
            s->role = AX_ROLE_EXPECT_RED;
        } else {
            return "aexp-unknown-key: a scene role is literal, golden or expect_red";
        }
        if ((s->role == AX_ROLE_EXPECT_RED) != (s->twin[0] != '\0' && s->red[0] != '\0') || (s->role != AX_ROLE_EXPECT_RED && (s->twin[0] != '\0' || s->red[0] != '\0'))) {
            return "aexp-malformed: an expect_red scene carries twin=NAME and expect_red=GID,...; no other scene carries either (D-211, D-225)";
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
            return "aexp-unknown-key: a group kind outside tests/audio/KINDCLASS";
        }
        if (au_kinds[g->kind - 1].form != g->form) {
            return "aexp-malformed: the group's form is not the form of its kind (tests/audio/KINDCLASS)";
        }
        if (!ax_class_ok(cl) || ax_kills(kl, &g->kills) != 0) {
            return "aexp-malformed: a group needs class=MAN:L|GUEST:M|ASSUMED:LED|DEF:LED and a non-empty kills list of AF0 to AF9";
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
    } else if (strcmp(fld[0], "golden") == 0) {
        struct ax_golden *gd;

        if (nf != 4u || f->ngold >= 4u || !au_name_ok(fld[1], AU_NAME) || (a = ax_val(fld[2], "pcm_sha256")) == NULL || (b = ax_val(fld[3], "psg_sha256")) == NULL || !ax_hex64(a) || !ax_hex64(b)) {
            return "aexp-malformed: a golden row is: golden SCENE pcm_sha256=HEX psg_sha256=HEX (64 lowercase hex digits each)";
        }
        gd = &f->gd[f->ngold++];
        memcpy(gd->scene, fld[1], strlen(fld[1]) + 1u);
        memcpy(gd->pcm, a, 65u);
        memcpy(gd->psg, b, 65u);
    } else {
        return "aexp-unknown-key: a row kind is aexp, scene, group, row or golden";
    }
    return NULL;
}

static const char *ax_link(struct ax_file *f);

/* build/gate/fixtures/ROM.aexp.tsv into f (cleared first); NULL on success. A missing file is a fault of the harness, never an empty scene list */
static const char *ax_load(const char *rom, struct ax_file *f)
{
    char path[96];
    char *text;
    char *line;
    unsigned nrows = 0u;
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
    return ax_link(f);
}

/* the checks after the rows are read: the header once, a scene, every row's group and every group's scene; NULL on success */
static const char *ax_link(struct ax_file *f)
{
    unsigned i;

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
    uint64_t lf;        /* the video frame length in master clocks (262 or 263 lines of 1365), for the count kind */
    uint64_t hdr_fcount; /* the sum of the pcm headers' count fields over the calls: a fault that drops or adds a frame changes fcount, never this */
    uint64_t hdr_ecount; /* the same for the psg headers */
    uint32_t pfbytes;   /* the pcm header's frame_bytes, version and format as read (au_run refuses others as harness_failure) */
    uint32_t pversion;
    uint32_t pformat;
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

static uint64_t ao_frame_mclk(const struct ds_instance *inst);
static uint64_t ao_frame_count(uint64_t first, uint64_t n, uint64_t lf, uint64_t k, uint64_t *start);

/* the missing-sink rule of D-209: no frame although samples were due (shared with the control of audio_judge_forms) */
static int au_no_frames(const struct au_cap *cp)
{
    return cp->fcount == 0u && cp->machine_time > cp->first * (uint64_t)cp->smclk;
}

/* the D-197 header rule of au_run: frame_bytes 8, version 1, format 1, a sample_mclk above 0; 1 when the header is acceptable (shared with the missing-sink control of audio_judge_forms) */
static int au_pcm_header_ok(const uint8_t *h)
{
    return au_u16(h + AU_H_FBYTES) == AU_F_BYTES && h[AU_H_VERSION] == 1u && h[AU_H_FORMAT] == 1u && au_u32(h + AU_H_SMCLK) != 0u;
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
    if (!au_pcm_header_ok(fsk.header) || (cp->calls > 0u && au_u32(fsk.header + AU_H_SMCLK) != cp->smclk)) {
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
    cp->hdr_ecount += ec;
    cp->hdr_fcount += fc;
    cp->pfbytes = au_u16(fsk.header + AU_H_FBYTES);
    cp->pversion = fsk.header[AU_H_VERSION];
    cp->pformat = fsk.header[AU_H_FORMAT];
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
    cp->lf = ao_frame_mclk(ci);
    release_boot(&b);
    if (c->harness || c->timeout) {
        return -1;
    }
    if (au_no_frames(cp)) {
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
    int stream; /* 0 none, 1 dspa, 2 dspe (the STRUCT header rows) */
    uint64_t to_off; /* the end anchor's offset in master clocks (nth=N+M, PR-13) */
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
            char nb[24];
            const char *plus = strchr(x, '+');
            size_t nlen = plus != NULL ? (size_t)(plus - x) : strlen(x);
            uint64_t off = 0u;

            if (nlen >= sizeof nb || (plus != NULL && (cur != &s->e || au_num(plus + 1, &off) != 0))) {
                return "aexp-malformed: nth=N+M (an offset of M master clocks) is allowed on the end anchor only (PR-13)";
            }
            memcpy(nb, x, nlen);
            nb[nlen] = '\0';
            if (cur == NULL || cur->nth != 0u || au_num(nb, &v) != 0 || v == 0u) {
                return "aexp-malformed: nth=N (N >= 1) follows its anchor once";
            }
            cur->nth = v;
            if (cur == &s->e) {
                s->to_off = off;
            }
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
        } else if ((x = ax_val(item, "stream")) != NULL) {
            if (s->stream != 0 || (strcmp(x, "dspa") != 0 && strcmp(x, "dspe") != 0)) {
                return "aexp-malformed: stream=dspa or stream=dspe, once";
            }
            s->stream = x[3] == 'a' ? 1 : 2;
            cur = NULL;
        } else {
            return "aexp-unknown-key: a selector item is anchor, reg, val, nth, ev, from, to, side or stream";
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

/* the closed literal keys: the judged keys of the 05-04 kinds and the inputs of the 05-05 arithmetic lint (plan check 3 info 5); plan 05-07 adds bits (the noise_seq string) and ords (the dda_hold change ordinals), also in the litkey list of scripts/lib/aexp-rules.awk */
static const char *const au_keys[] = {"f", "k", "d", "nf", "g", "f2", "flf", "step", "w", "w1", "w2", "shift", "set", "m", "n", "r1", "al", "r5", "chon", "p0", "p1", "words", "minrun", "reg", "val", "vals", "chan", "effect", "wave_idx", "post", "sample_mclk", "frame_bytes", "version", "format", "mark", "peak", "bits", "ords"};

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
        to = au_u64(cp->erec + (size_t)ei * AU_E_REC + AU_E_STAMP) + s->to_off;
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
#define AU_LFO_MIN_RUN 40u /* lfo_dynamic (plan 05.1-02): the fewest cycles of an interior run; a half table of psg_lfo's lfo_dynamic lasts 16 x 115,200 / 192 = 9,600 samples, 48 cycles of 200 or about 52 of 184 */

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

/* ---- the noise and DDA helpers of plan 05-07 (D-208): runs, plateaus and the DDA writes of a window ---- */

/* the interior runs of the window [i0, i1) on one side (the runs of equal samples, the first and the last run left out because the window cuts them): their number, the smallest, their greatest common divisor, and whether
 * every one is a multiple of g (g 0: not asked) */
struct au_runs {
    unsigned n;
    uint64_t min;
    uint64_t gcd;
    int off_grid;
    uint64_t bad_at; /* the first sample of the first run off the grid */
    uint64_t bad_len;
};

static uint64_t au_gcd(uint64_t a, uint64_t b)
{
    while (b != 0u) {
        uint64_t t = a % b;

        a = b;
        b = t;
    }
    return a;
}

static void au_runs_of(const struct au_cap *cp, uint64_t i0, uint64_t i1, int side, uint64_t g, struct au_runs *r)
{
    uint64_t start = i0;
    uint64_t i;
    int first = 1;

    memset(r, 0, sizeof *r);
    for (i = i0 + 1u; i <= i1; i++) {
        uint64_t len;

        if (i < i1 && au_sample(cp, i, side) == au_sample(cp, i - 1u, side)) {
            continue;
        }
        len = i - start;
        if (!first && i < i1) { /* an interior run: neither the first nor the one the window end cuts */
            r->min = r->n == 0u || len < r->min ? len : r->min;
            r->gcd = au_gcd(r->gcd, len);
            if (g != 0u && len % g != 0u && !r->off_grid) {
                r->off_grid = 1;
                r->bad_at = start;
                r->bad_len = len;
            }
            r->n++;
        }
        first = 0;
        start = i;
    }
}

/* the run-collapsed levels (plateaus) of [i0, i1) on one side into pl (at most cap; the count is returned, cap + 1 when there are more) */
static unsigned au_plateaus(const struct au_cap *cp, uint64_t i0, uint64_t i1, int side, uint64_t *pl, unsigned cap)
{
    unsigned np = 0u;
    uint64_t i;

    for (i = i0; i < i1; i++) {
        uint64_t x = au_sample(cp, i, side);

        if (np > 0u && np <= cap && pl[np - 1u] == x) {
            continue;
        }
        if (np >= cap) {
            return cap + 1u;
        }
        pl[np++] = x;
    }
    return np;
}

/* the dense ranks of the n plateaus equal those of the n words in order (rot 0), or at some rotation (any 1); 1 when they do */
static int au_ranks_match(const uint64_t *pl, const uint64_t *words, unsigned n, int any, unsigned *rot_out)
{
    unsigned rot;
    unsigned j;

    for (rot = 0u; rot < (any ? n : 1u); rot++) {
        int same = 1;

        for (j = 0u; j < n && same; j++) {
            same = au_rank(pl, n, pl[(j + rot) % n]) == au_rank(words, n, words[j]);
        }
        if (same) {
            *rot_out = rot;
            return 1;
        }
    }
    return 0;
}

#define AU_DDA_MAX 64u

/* the stream indices of the DDA writes (DSPE records with DDA_LOAD) of channel ch whose first sample at or after the stamp lies in [i0, i1), in stream order; the count (at most cap) */
static unsigned au_dda_writes(const struct au_cap *cp, unsigned ch, uint64_t i0, uint64_t i1, uint64_t *at, unsigned cap)
{
    uint32_t i;
    unsigned n = 0u;

    for (i = 0u; i < cp->ecount && n < cap; i++) {
        const uint8_t *r = cp->erec + (size_t)i * AU_E_REC;
        uint64_t a;

        if (r[AU_E_REG] != 6u || r[AU_E_CHAN] != ch || (r[AU_E_EFFECT] & 0x02u) == 0u) {
            continue;
        }
        a = au_index(cp, au_u64(r + AU_E_STAMP));
        if (a >= i0 && a < i1) {
            at[n++] = a;
        }
    }
    return n;
}

static uint32_t au_peak(const struct au_cap *cp, uint64_t i0, uint64_t i1, int side);
static int au_lit_num(const struct au_lit *l, const char *key, uint64_t *v);

/* perturb (AF0 of D-209, the first row of a group only): the judge reads a perturbed literal (silent: every sample 1; period: d + 1; cycle_order: the first two words swapped; levels_exact: peak + 1; the 05-07 kinds as their comments say), so a live comparison must fail */
static int au_rows_row(const struct au_cap *cp, const struct ax_group *g, const struct au_sel *s, const struct au_lit *l, int perturb, char *why, size_t cap)
{
    uint64_t i0 = 0u;
    uint64_t i1 = 0u;
    uint64_t i;
    int w = au_window(cp, s, &i0, &i1, why, cap);

    if (w != AU_PASS) {
        return w;
    }
    if (g->kind == AK_LIVE) {
        /* live (D-208): a period or cycle_order property, never "nonzero": two rising crossings in the window and at least 3 distinct levels between them; AF0 inverts the rule */
        uint64_t cx[2];
        uint64_t lv[3];
        unsigned nl = 0u;
        unsigned j;

        (void)l;
        if (au_crossings(cp, i0, i1, s->side, cx, 2u) < 2u) {
            snprintf(why, cap, "fewer than two rising crossings in the window [%llu, %llu): no live cycle", (unsigned long long)i0, (unsigned long long)i1);
            return perturb ? AU_PASS : AU_FAIL;
        }
        for (i = cx[0]; i < cx[1] && nl < 3u; i++) {
            uint64_t x = au_sample(cp, i, s->side);
            int seen = 0;

            for (j = 0u; j < nl; j++) {
                seen = seen || lv[j] == x;
            }
            if (!seen) {
                lv[nl++] = x;
            }
        }
        snprintf(why, cap, "%s3 distinct levels between the crossings %llu and %llu", nl >= 3u ? "at least " : "fewer than ", (unsigned long long)cx[0], (unsigned long long)cx[1]);
        return (nl >= 3u) != (perturb != 0) ? AU_PASS : AU_FAIL;
    }
    if (g->kind == AK_SILENT) {
        uint32_t want = perturb ? 1u : 0u;

        for (i = i0; i < i1; i++) {
            if (au_sample(cp, i, s->side) != want) {
                snprintf(why, cap, "sample %llu of the window [%llu, %llu) reads %u on side %c, not %u", (unsigned long long)i, (unsigned long long)i0, (unsigned long long)i1, (unsigned)au_sample(cp, i, s->side), s->side == 1 ? 'L' : 'R', (unsigned)want);
                return AU_FAIL;
            }
        }
        snprintf(why, cap, "all %llu samples of the window are 0", (unsigned long long)(i1 - i0));
        return AU_PASS;
    }
    if (g->kind == AK_PERIOD || g->kind == AK_LFO_STATIC) { /* lfo_static (plan 05.1-01, D-208, D-219): the period judge on k and d, with d = k x F' (the lint recomputes F' from f, w and shift) */
        uint64_t cx[AU_CROSS_MAX];
        uint64_t k = 0u;
        uint64_t d = 0u;
        unsigned n;

        if (au_lit_get(l, "k") == NULL || au_lit_get(l, "d") == NULL || au_num(au_lit_get(l, "k"), &k) != 0 || au_num(au_lit_get(l, "d"), &d) != 0 || k == 0u || k >= AU_CROSS_MAX) {
            snprintf(why, cap, "aexp-malformed: a period row needs k (1 to 4095) and d");
            return AU_HARNESS;
        }
        d += perturb ? 1u : 0u;
        n = au_crossings(cp, i0, i1, s->side, cx, AU_CROSS_MAX);
        if ((uint64_t)n < k + 1u) {
            snprintf(why, cap, "%u rising crossings in the window [%llu, %llu), fewer than k + 1 = %llu", n, (unsigned long long)i0, (unsigned long long)i1, (unsigned long long)(k + 1u));
            return AU_FAIL;
        }
        snprintf(why, cap, "crossings %llu and %llu (the first and the %llu-th after it) are %llu samples apart, the literal is %llu", (unsigned long long)cx[0], (unsigned long long)cx[k], (unsigned long long)k, (unsigned long long)(cx[k] - cx[0]), (unsigned long long)d);
        return cx[k] - cx[0] == d ? AU_PASS : AU_FAIL;
    }
    if (g->kind == AK_LEVELS_EXACT) {
        /* levels_exact (plan 05-06; D-194, D-206, ASSUMED:LED-AUD-0009): the largest sample of the window on that side equals the hand literal peak (31 x VOL[att] for a channel whose table holds the word 31) */
        uint64_t want = 0u;
        uint32_t pk;

        if (au_lit_get(l, "peak") == NULL || au_num(au_lit_get(l, "peak"), &want) != 0) {
            snprintf(why, cap, "aexp-malformed: a levels_exact row needs peak");
            return AU_HARNESS;
        }
        want += perturb ? 1u : 0u;
        pk = au_peak(cp, i0, i1, s->side);
        snprintf(why, cap, "the peak of the window [%llu, %llu) on side %c is %u, the literal is %llu", (unsigned long long)i0, (unsigned long long)i1, s->side == 1 ? 'L' : 'R', (unsigned)pk, (unsigned long long)want);
        return (uint64_t)pk == want ? AU_PASS : AU_FAIL;
    }
    if (g->kind == AK_TWO_LEVEL) {
        /* two_level (D-208, MAN:P8-2+P8-6+P8-9+H8-23): the window holds exactly n distinct sample values on that side (n = 2 for noise: the values themselves are ASSUMED and not judged); AF0 reads n + 1 */
        uint64_t want = 0u;
        uint64_t seen[3];
        unsigned ns = 0u;
        unsigned j;

        if (au_lit_num(l, "n", &want) != 0 || want == 0u || want > 2u) {
            snprintf(why, cap, "aexp-malformed: a two_level row needs n (1 or 2)");
            return AU_HARNESS;
        }
        want += perturb ? 1u : 0u;
        for (i = i0; i < i1 && ns < 3u; i++) {
            uint64_t x = au_sample(cp, i, s->side);
            int dup = 0;

            for (j = 0u; j < ns; j++) {
                dup = dup || seen[j] == x;
            }
            if (!dup) {
                seen[ns++] = x;
            }
        }
        snprintf(why, cap, "the window [%llu, %llu) holds %s%u distinct values on side %c, the literal is %llu", (unsigned long long)i0, (unsigned long long)i1, ns >= 3u ? "at least " : "", ns, s->side == 1 ? 'L' : 'R', (unsigned long long)want);
        return (uint64_t)ns == want ? AU_PASS : AU_FAIL;
    }
    if (g->kind == AK_RUN_GRID || g->kind == AK_RUN_MIN) {
        /* run_grid (MAN): every interior run is a multiple of g samples (g = 2 x nf at d = 6: a noise clock of 384 x NFc master clocks is 2 x NFc samples); run_min (ASSUMED:LED-AUD-0010, the sequence decides it): the shortest interior run is exactly minrun samples.
         * A window with no interior run judges nothing and fails. AF0 reads g + 1 or minrun + 1 */
        uint64_t want = 0u;
        struct au_runs rs;

        if (au_lit_num(l, g->kind == AK_RUN_GRID ? "g" : "minrun", &want) != 0 || want == 0u) {
            snprintf(why, cap, "aexp-malformed: a %s row needs %s (above 0)", g->kind == AK_RUN_GRID ? "run_grid" : "run_min", g->kind == AK_RUN_GRID ? "g" : "minrun");
            return AU_HARNESS;
        }
        want += perturb ? 1u : 0u;
        au_runs_of(cp, i0, i1, s->side, g->kind == AK_RUN_GRID ? want : 0u, &rs);
        if (rs.n == 0u) {
            snprintf(why, cap, "the window [%llu, %llu) has no interior run on side %c: nothing to judge", (unsigned long long)i0, (unsigned long long)i1, s->side == 1 ? 'L' : 'R');
            return AU_FAIL;
        }
        if (g->kind == AK_RUN_GRID) {
            if (rs.off_grid) {
                snprintf(why, cap, "the interior run at sample %llu is %llu samples, not a multiple of g = %llu (%u interior runs)", (unsigned long long)rs.bad_at, (unsigned long long)rs.bad_len, (unsigned long long)want, rs.n);
                return AU_FAIL;
            }
            snprintf(why, cap, "all %u interior runs of [%llu, %llu) are multiples of g = %llu (their gcd %llu)", rs.n, (unsigned long long)i0, (unsigned long long)i1, (unsigned long long)want, (unsigned long long)rs.gcd);
            return AU_PASS;
        }
        snprintf(why, cap, "the shortest of %u interior runs of [%llu, %llu) is %llu samples, the literal is %llu", rs.n, (unsigned long long)i0, (unsigned long long)i1, (unsigned long long)rs.min, (unsigned long long)want);
        return rs.min == want ? AU_PASS : AU_FAIL;
    }
    if (g->kind == AK_NOISE_SEQ) {
        /* noise_seq (ASSUMED:LED-AUD-0010): the R7 record of the anchor's channel last before the anchor gives the stamp w; noise events lie at j x p0 up to the first one after w (the reload of the old period, ordinal floor(w / p0) + 2, the event at T = 0 being
         * ordinal 1, RF-11), then every p1. At each event inside the window the sample at the event instant (the D-185 tie order: the noise clock before the sample) is non-zero exactly when the bit of that ordinal in bits is 1. An ordinal past the string is a
         * harness error (fail closed, never a wrap). AF0 inverts the bit of the first judged event */
        const char *bits = au_lit_get(l, "bits");
        uint64_t p0 = 0u;
        uint64_t p1 = 0u;
        uint32_t ai;
        uint32_t ri = UINT32_MAX;
        uint32_t e;
        uint64_t w7;
        uint64_t t;
        uint64_t ord;
        uint64_t nb;
        unsigned judged = 0u;

        if (bits == NULL || strlen(bits) == 0u || strlen(bits) > 256u || strspn(bits, "01") != strlen(bits) || au_lit_num(l, "p0", &p0) != 0 || au_lit_num(l, "p1", &p1) != 0 || p0 == 0u || p1 == 0u || !s->a.set) {
            snprintf(why, cap, "aexp-malformed: a noise_seq row needs a start anchor, p0, p1 (above 0) and bits (1 to 256 characters 0 or 1)");
            return AU_HARNESS;
        }
        nb = strlen(bits);
        ai = au_find(cp, &s->a);
        for (e = 0u; ai != UINT32_MAX && e < ai; e++) {
            const uint8_t *r = cp->erec + (size_t)e * AU_E_REC;

            if (r[AU_E_REG] == 7u && r[AU_E_CHAN] == s->a.ch) {
                ri = e;
            }
        }
        if (ri == UINT32_MAX) {
            snprintf(why, cap, "no R7 record of channel %u before the anchor: no noise period to count the events from", s->a.ch);
            return AU_FAIL;
        }
        w7 = au_u64(cp->erec + (size_t)ri * AU_E_REC + AU_E_STAMP);
        t = (w7 / p0 + 1u) * p0;
        ord = w7 / p0 + 2u;
        while (au_index(cp, t) < i0) {
            t += p1;
            ord++;
        }
        for (; au_index(cp, t) < i1; t += p1, ord++) {
            uint64_t si = au_index(cp, t);
            int want_on;
            int on;

            if (ord > nb) {
                snprintf(why, cap, "the event at %llu master clocks has ordinal %llu, past the %llu bits of the literal: a harness error, never a wrap", (unsigned long long)t, (unsigned long long)ord, (unsigned long long)nb);
                return AU_HARNESS;
            }
            want_on = bits[ord - 1u] == '1';
            if (perturb && judged == 0u) {
                want_on = !want_on;
            }
            on = au_sample(cp, si, s->side) != 0u;
            if (on != want_on) {
                snprintf(why, cap, "the noise event of ordinal %llu (R7 at %llu, the event at %llu, sample %llu) reads %s on side %c, the bit is %c", (unsigned long long)ord, (unsigned long long)w7, (unsigned long long)t, (unsigned long long)si, on ? "non-zero" : "0",
                         s->side == 1 ? 'L' : 'R', want_on ? '1' : '0');
                return AU_FAIL;
            }
            judged++;
        }
        if (judged == 0u) {
            snprintf(why, cap, "no noise event lies inside the window [%llu, %llu): nothing to judge", (unsigned long long)i0, (unsigned long long)i1);
            return AU_FAIL;
        }
        snprintf(why, cap, "the %u noise events of ordinals %llu to %llu (R7 at %llu) match the bits on side %c", judged, (unsigned long long)(ord - judged), (unsigned long long)(ord - 1u), (unsigned long long)w7, s->side == 1 ? 'L' : 'R');
        return AU_PASS;
    }
    if (g->kind == AK_DDA_HOLD) {
        /* dda_hold (MAN:P8-4+P8-5): the DDA writes of the anchor's channel inside the window, in order, are the ordinals 1..N; the level changes exactly at the ordinals of ords and is constant between writes. The change of write j is accepted in the sample at or after its
         * stamp (a_j) or the next one, so the row never judges the tie order of a write at a sample instant (D-185): a_j reads the old or the new level, a_j + 1 .. a_(j+1) - 1 the new one. AF0 moves the first ordinal of ords by one */
        uint64_t ords[AU_WORDS_MAX];
        uint64_t at[AU_DDA_MAX];
        unsigned no = 0u;
        unsigned nw;
        unsigned j;
        unsigned o;
        uint64_t prev;
        uint64_t omax = 0u;

        if (au_lit_get(l, "ords") == NULL || au_numlist(au_lit_get(l, "ords"), ords, AU_WORDS_MAX, &no) != 0 || !s->a.set) {
            snprintf(why, cap, "aexp-malformed: a dda_hold row needs a start anchor and ords (the write ordinals where the level changes)");
            return AU_HARNESS;
        }
        if (perturb) {
            ords[0] += 1u;
        }
        for (o = 0u; o < no; o++) {
            omax = ords[o] > omax ? ords[o] : omax;
        }
        nw = au_dda_writes(cp, s->a.ch, i0, i1, at, AU_DDA_MAX);
        if ((uint64_t)nw < omax || nw == 0u) {
            snprintf(why, cap, "the window [%llu, %llu) holds %u DDA writes of channel %u, fewer than the largest ordinal %llu", (unsigned long long)i0, (unsigned long long)i1, nw, s->a.ch, (unsigned long long)omax);
            return AU_FAIL;
        }
        prev = au_sample(cp, i0, s->side);
        for (i = i0; i < at[0]; i++) {
            if (au_sample(cp, i, s->side) != prev) {
                snprintf(why, cap, "the level changes at sample %llu, before the first DDA write (sample %llu)", (unsigned long long)i, (unsigned long long)at[0]);
                return AU_FAIL;
            }
        }
        for (j = 0u; j < nw; j++) {
            uint64_t end = j + 1u < nw ? at[j + 1u] : i1;
            uint64_t lv;
            int expect = 0;

            if (at[j] + 1u >= end) {
                snprintf(why, cap, "DDA write %u (sample %llu) leaves no held sample before the next write or the window end", j + 1u, (unsigned long long)at[j]);
                return AU_FAIL;
            }
            lv = au_sample(cp, at[j] + 1u, s->side);
            if (au_sample(cp, at[j], s->side) != prev && au_sample(cp, at[j], s->side) != lv) {
                snprintf(why, cap, "sample %llu of DDA write %u reads neither the old nor the new level", (unsigned long long)at[j], j + 1u);
                return AU_FAIL;
            }
            for (i = at[j] + 1u; i < end; i++) {
                if (au_sample(cp, i, s->side) != lv) {
                    snprintf(why, cap, "the level after DDA write %u changes at sample %llu before the next write: not held", j + 1u, (unsigned long long)i);
                    return AU_FAIL;
                }
            }
            for (o = 0u; o < no; o++) {
                expect = expect || ords[o] == (uint64_t)(j + 1u);
            }
            if ((lv != prev) != expect) {
                snprintf(why, cap, "DDA write %u (sample %llu) %s the level (%llu to %llu), the literal says it %s", j + 1u, (unsigned long long)at[j], lv != prev ? "changes" : "keeps", (unsigned long long)prev, (unsigned long long)lv, expect ? "changes" : "keeps");
                return AU_FAIL;
            }
            prev = lv;
        }
        snprintf(why, cap, "the %u DDA writes of channel %u in [%llu, %llu) change the level exactly at the ordinals of the literal and hold it between writes", nw, s->a.ch, (unsigned long long)i0, (unsigned long long)i1);
        return AU_PASS;
    }
    if (g->kind == AK_DDA_ORDER || g->kind == AK_DDA_LEVEL_EXACT || g->kind == AK_CYCLE_PHASE) {
        /* dda_order (MAN:P8-4+P8-5): the plateaus of the window rank as vals, in order; dda_level_exact (ASSUMED:LED-AUD-0009): the plateaus equal vals; cycle_phase (ASSUMED:LED-AUD-0012): the first plateaus from the window start rank as words in order (no rotation).
         * AF0 swaps the first two literal values (dda_level_exact: the first value + 1) */
        uint64_t want[AU_WORDS_MAX];
        uint64_t pl[AU_WORDS_MAX + 1u];
        unsigned nwant = 0u;
        unsigned np;
        unsigned rot = 0u;
        const char *key = g->kind == AK_CYCLE_PHASE ? "words" : "vals";

        if (au_lit_get(l, key) == NULL || au_numlist(au_lit_get(l, key), want, AU_WORDS_MAX, &nwant) != 0 || nwant < 2u) {
            snprintf(why, cap, "aexp-malformed: a %s row needs %s= (2 to 64 numbers)", au_kinds[g->kind - 1].name, key);
            return AU_HARNESS;
        }
        if (perturb && g->kind == AK_DDA_LEVEL_EXACT) {
            want[0] += 1u;
        } else if (perturb) {
            uint64_t t = want[0];

            want[0] = want[1] != want[0] ? want[1] : want[0] + 1u;
            want[1] = t;
        }
        np = au_plateaus(cp, i0, i1, s->side, pl, AU_WORDS_MAX);
        if (g->kind == AK_CYCLE_PHASE ? np < nwant : np != nwant) {
            snprintf(why, cap, "%s%u plateaus in the window [%llu, %llu) on side %c, the literal has %u values", np > AU_WORDS_MAX ? "more than " : "", np > AU_WORDS_MAX ? AU_WORDS_MAX : np, (unsigned long long)i0, (unsigned long long)i1, s->side == 1 ? 'L' : 'R', nwant);
            return AU_FAIL;
        }
        if (g->kind == AK_DDA_LEVEL_EXACT) {
            unsigned j;

            for (j = 0u; j < nwant; j++) {
                if (pl[j] != want[j]) {
                    snprintf(why, cap, "plateau %u reads %llu, the literal is %llu", j + 1u, (unsigned long long)pl[j], (unsigned long long)want[j]);
                    return AU_FAIL;
                }
            }
            snprintf(why, cap, "the %u plateaus of [%llu, %llu) equal the literal levels", nwant, (unsigned long long)i0, (unsigned long long)i1);
            return AU_PASS;
        }
        if (!au_ranks_match(pl, want, nwant, 0, &rot)) {
            snprintf(why, cap, "the ranks of the first %u plateaus of [%llu, %llu) are not those of the literal in order", nwant, (unsigned long long)i0, (unsigned long long)i1);
            return AU_FAIL;
        }
        snprintf(why, cap, "the first %u plateaus of [%llu, %llu) rank as the literal in order", nwant, (unsigned long long)i0, (unsigned long long)i1);
        return AU_PASS;
    }
    if (g->kind == AK_LFO_DYNAMIC) {
        /* lfo_dynamic (plan 05.1-02; D-208, MAN:P8-6+P8-7+P8-8+P8-9+H8-23): the complete cycles of the window (one rising crossing to the next) are grouped into maximal runs of equal length, all taken from the stream: no switch instant is computed, so the row rests
         * on no restart edge (D-220) and judges no switch timing (D-208). Every run length must be one of set=A,B (F'(w1) and F'(w2) samples at shift 0, recomputed by the lint), except a single cycle of another length (the straddling cycle of a switch) that stands
         * between a run of A and a run of B; both lengths must occur; no other cycle may sit at the window's edge; every interior run of A or B (neither the first nor the last) holds at least AU_LFO_MIN_RUN cycles. AF0 reads A + 1 */
        uint64_t cx[AU_CROSS_MAX];
        uint64_t sv[2];
        unsigned nsv = 0u;
        unsigned n;
        unsigned c;
        unsigned nset = 0u;      /* the runs of A or B finished so far */
        unsigned between = 0u;   /* the cycles of another length since the last run of A or B */
        uint64_t other = 0u;     /* the last such length */
        uint64_t last_len = 0u;  /* the last run of A or B */
        unsigned last_cnt = 0u;
        uint64_t run_len = 0u;   /* the run being collected */
        unsigned run_cnt = 0u;
        unsigned straddle = 0u;
        int seen[2] = {0, 0};

        if (au_lit_get(l, "set") == NULL || au_numlist(au_lit_get(l, "set"), sv, 2u, &nsv) != 0 || nsv != 2u || sv[0] == 0u || sv[1] == 0u || sv[0] == sv[1]) {
            snprintf(why, cap, "aexp-malformed: an lfo_dynamic row needs set=A,B (two different cycle lengths in samples)");
            return AU_HARNESS;
        }
        sv[0] += perturb ? 1u : 0u;
        n = au_crossings(cp, i0, i1, s->side, cx, AU_CROSS_MAX);
        if (n > AU_CROSS_MAX) {
            snprintf(why, cap, "more than %u rising crossings in the window [%llu, %llu): past the judge's capacity", AU_CROSS_MAX, (unsigned long long)i0, (unsigned long long)i1);
            return AU_HARNESS;
        }
        if (n < 2u) {
            snprintf(why, cap, "%u rising crossings in the window [%llu, %llu): no complete cycle", n, (unsigned long long)i0, (unsigned long long)i1);
            return AU_FAIL;
        }
        for (c = 0u; c + 1u <= n; c++) {
            uint64_t len = c + 1u < n ? cx[c + 1u] - cx[c] : 0u; /* c = n - 1: the end, which finishes the last run */

            if (c + 1u < n && run_cnt > 0u && len == run_len) {
                run_cnt++;
                continue;
            }
            if (run_cnt > 0u) { /* finish the run (run_len, run_cnt), which ends at the crossing cx[c] */
                int k = run_len == sv[0] ? 0 : run_len == sv[1] ? 1 : -1;

                if (k < 0) {
                    between += run_cnt;
                    other = run_len;
                } else {
                    if (nset == 0u && between > 0u) {
                        snprintf(why, cap, "a cycle of %llu samples before the first run of %llu or %llu (at the window's edge, ending at sample %llu)", (unsigned long long)other, (unsigned long long)sv[0], (unsigned long long)sv[1], (unsigned long long)cx[c]);
                        return AU_FAIL;
                    }
                    if (nset > 0u && between > 1u) {
                        snprintf(why, cap, "%u cycles of another length (the last %llu samples) between a run of %llu and a run of %llu ending at sample %llu: at most one straddling cycle", between, (unsigned long long)other, (unsigned long long)last_len, (unsigned long long)run_len,
                                 (unsigned long long)cx[c]);
                        return AU_FAIL;
                    }
                    if (nset > 0u && between == 1u && last_len == run_len) {
                        snprintf(why, cap, "a cycle of %llu samples inside a run of %llu (ending at sample %llu): a third length is not a straddling cycle", (unsigned long long)other, (unsigned long long)run_len, (unsigned long long)cx[c]);
                        return AU_FAIL;
                    }
                    if (nset >= 2u && last_cnt < AU_LFO_MIN_RUN) {
                        snprintf(why, cap, "an interior run of %u cycles of %llu samples, fewer than %u (a half table holds about 48 to 52)", last_cnt, (unsigned long long)last_len, AU_LFO_MIN_RUN);
                        return AU_FAIL;
                    }
                    straddle += between;
                    between = 0u;
                    seen[k] = 1;
                    nset++;
                    last_len = run_len;
                    last_cnt = run_cnt;
                }
            }
            run_len = len;
            run_cnt = 1u;
        }
        if (between > 0u) {
            snprintf(why, cap, "%u cycle(s) of another length (the last %llu samples) after the last run of %llu or %llu (at the window's edge)", between, (unsigned long long)other, (unsigned long long)sv[0], (unsigned long long)sv[1]);
            return AU_FAIL;
        }
        if (!seen[0] || !seen[1]) {
            snprintf(why, cap, "the %u complete cycles of [%llu, %llu) hold runs of %llu samples only: both %llu and %llu must occur", n - 1u, (unsigned long long)i0, (unsigned long long)i1, (unsigned long long)(seen[0] ? sv[0] : seen[1] ? sv[1] : 0u),
                     (unsigned long long)sv[0], (unsigned long long)sv[1]);
            return AU_FAIL;
        }
        snprintf(why, cap, "the %u complete cycles of [%llu, %llu) form %u runs of %llu or %llu samples (every interior run at least %u cycles) and %u straddling cycle(s), each between a run of one length and one of the other", n - 1u, (unsigned long long)i0,
                 (unsigned long long)i1, nset, (unsigned long long)sv[0], (unsigned long long)sv[1], AU_LFO_MIN_RUN, straddle);
        return AU_PASS;
    }
    if (g->kind != AK_CYCLE_ORDER && g->kind != AK_DDA_RAM_KEPT) {
        snprintf(why, cap, "the kind %s has no ROWS judge in this file (fail closed)", au_kinds[g->kind - 1].name);
        return AU_HARNESS;
    }
    {
        /* cycle_order, and dda_ram_kept (plan 05-07: the wave RAM kept across DDA, MAN:P8-4+P8-5): the plateaus between the first two rising crossings against the word ranks up to rotation (RF-14) */
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
        if (perturb) {
            uint64_t t = words[0];

            words[0] = words[1] != words[0] ? words[1] : words[0] + 1u;
            words[1] = t;
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

/* perturb (AF0): ev_value reads the first value + 1, ev_decode the post + 1 */
static int au_ev_row(const struct au_cap *cp, const struct ax_group *g, const struct au_sel *s, const struct au_lit *l, int perturb, char *why, size_t cap)
{
    uint32_t ai;
    uint64_t at;

    if (!s->a.set || !s->has_ev || s->from_kind != 0 || s->to_kind != 0 || s->side != 0 || s->stream != 0) {
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
        if (perturb) {
            vals[0] = (vals[0] + 1u) & 0xFFu;
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
        want[3] += perturb ? 1u : 0u;
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

/* a number literal of l into *v; 0 on success */
static int au_lit_num(const struct au_lit *l, const char *key, uint64_t *v)
{
    return au_lit_get(l, key) != NULL && au_num(au_lit_get(l, key), v) == 0 ? 0 : -1;
}

/* the STRUCT form (D-207, ER-19): header (the DSPA header fields sample_mclk, frame_bytes, version and format and the header count against the frames delivered; every DSPE record's version and the header count against the records delivered; definitions, DEF) and count (the per-frame sample counts
 * of the frames 0 .. m / L - 1 summed, against n = ceil(m / 192), with the stream on the grid of D-187 up to the run's end; m a whole number of frames of L master clocks, ASSUMED:LED-VID-0001+LED-AUD-0014). perturb (AF0): the first literal + 1 */
static int au_struct_row(const struct au_cap *cp, const struct ax_group *g, const struct au_sel *s, const struct au_lit *l, int perturb, char *why, size_t cap)
{
    uint64_t want[4];
    uint32_t i;

    if (g->kind == AK_HEADER) {
        if (s->stream == 0 || s->a.set || s->e.set || s->from_kind != 0 || s->to_kind != 0 || s->side != 0 || s->has_ev) {
            snprintf(why, cap, "aexp-malformed: a header row selects stream=dspa or stream=dspe and nothing else");
            return AU_HARNESS;
        }
        if (s->stream == 1) {
            if (au_lit_num(l, "sample_mclk", &want[0]) != 0 || au_lit_num(l, "frame_bytes", &want[1]) != 0 || au_lit_num(l, "version", &want[2]) != 0 || au_lit_num(l, "format", &want[3]) != 0) {
                snprintf(why, cap, "aexp-malformed: a dspa header row needs sample_mclk, frame_bytes, version and format");
                return AU_HARNESS;
            }
            want[0] += perturb ? 1u : 0u;
            if ((uint64_t)cp->smclk != want[0] || (uint64_t)cp->pfbytes != want[1] || (uint64_t)cp->pversion != want[2] || (uint64_t)cp->pformat != want[3]) {
                snprintf(why, cap, "the DSPA header reads sample_mclk %u, frame_bytes %u, version %u, format %u, the literal is %llu, %llu, %llu, %llu", (unsigned)cp->smclk, (unsigned)cp->pfbytes, (unsigned)cp->pversion, (unsigned)cp->pformat, (unsigned long long)want[0],
                         (unsigned long long)want[1], (unsigned long long)want[2], (unsigned long long)want[3]);
                return AU_FAIL;
            }
            if (cp->hdr_fcount != (uint64_t)cp->fcount) {
                snprintf(why, cap, "the DSPA header counts %llu frames, the stream holds %u", (unsigned long long)cp->hdr_fcount, (unsigned)cp->fcount);
                return AU_FAIL;
            }
            snprintf(why, cap, "the DSPA header reads the literal and counts the %u frames delivered", (unsigned)cp->fcount);
            return AU_PASS;
        }
        if (au_lit_num(l, "version", &want[0]) != 0) {
            snprintf(why, cap, "aexp-malformed: a dspe header row needs version");
            return AU_HARNESS;
        }
        want[0] += perturb ? 1u : 0u;
        for (i = 0u; i < cp->ecount; i++) {
            if ((uint64_t)cp->erec[(size_t)i * AU_E_REC + AU_E_VERSION] != want[0]) {
                snprintf(why, cap, "DSPE record %u reads version %u, the literal is %llu", (unsigned)i, (unsigned)cp->erec[(size_t)i * AU_E_REC + AU_E_VERSION], (unsigned long long)want[0]);
                return AU_FAIL;
            }
        }
        if (cp->hdr_ecount != (uint64_t)cp->ecount || cp->ecount == 0u) {
            snprintf(why, cap, "the DSPE header counts %llu records, the stream holds %u (none is not a pass)", (unsigned long long)cp->hdr_ecount, (unsigned)cp->ecount);
            return AU_FAIL;
        }
        snprintf(why, cap, "all %u DSPE records read version %llu and the header counts them", (unsigned)cp->ecount, (unsigned long long)want[0]);
        return AU_PASS;
    }
    if (g->kind == AK_COUNT) {
        uint64_t m = 0u;
        uint64_t n = 0u;
        uint64_t k;
        uint64_t sum = 0u;

        if (s->from_kind != 2 || s->to_kind != 2 || s->from != 0u || s->a.set || s->e.set || s->side != 0 || s->stream != 0 || au_lit_num(l, "m", &m) != 0 || au_lit_num(l, "n", &n) != 0 || s->to != m || cp->lf == 0u || m == 0u || m % cp->lf != 0u) {
            snprintf(why, cap, "aexp-malformed: a count row selects from=0;to=M and carries m=M (a whole number of frames) and n");
            return AU_HARNESS;
        }
        n += perturb ? 1u : 0u;
        for (k = 0u; k < m / cp->lf; k++) {
            uint64_t start = 0u;

            sum += ao_frame_count(cp->first, cp->fcount, cp->lf, k, &start);
        }
        if ((uint64_t)cp->fcount + cp->first != cp->machine_time / cp->smclk + (cp->machine_time % cp->smclk != 0u ? 1u : 0u)) {
            snprintf(why, cap, "the stream holds frames %llu to %llu, not every sample instant up to the run's end %llu (the grid of D-187)", (unsigned long long)cp->first, (unsigned long long)(cp->first + cp->fcount), (unsigned long long)cp->machine_time);
            return AU_FAIL;
        }
        snprintf(why, cap, "the frames 0 to %llu of %llu master clocks hold %llu samples, the literal is %llu", (unsigned long long)(m / cp->lf - 1u), (unsigned long long)cp->lf, (unsigned long long)sum, (unsigned long long)n);
        return sum == n ? AU_PASS : AU_FAIL;
    }
    snprintf(why, cap, "the kind %s is not a STRUCT kind", au_kinds[g->kind - 1].name);
    return AU_HARNESS;
}

#define AU_REL_MAX 8u

/* the peak (largest sample) of [i0, i1) on one side */
static uint32_t au_peak(const struct au_cap *cp, uint64_t i0, uint64_t i1, int side)
{
    uint32_t p = 0u;
    uint64_t i;

    for (i = i0; i < i1; i++) {
        uint32_t x = au_sample(cp, i, side);

        p = x > p ? x : p;
    }
    return p;
}

/* the REL form (D-207, D-208): a relation over the windows of the group's rows in row order. period_ratio (two rows, k on each, p0 and p1 on the first): the distance of k cycles of the first window times p1 equals that of the second times p0; noise_ratio (plan 05-07; two rows, p0 and
 * p1 on the first): the run grids of the two windows in the ratio p0 : p1; same (two rows, n on the first): the n samples
 * from the first rising crossing of each window are equal (aligned at the crossing, never at a write, D-185); atten_order (two or more rows): the peaks strictly decrease; atten_equal: the peaks are equal. perturb (AF0): p0 + 1, the second window one sample later, a strict increase of
 * the first pair, the first peak + 1 */
static int au_rel_group(const struct ax_file *f, unsigned gi, const struct au_cap *cp, int perturb, char *why, size_t cap)
{
    const struct ax_group *g = &f->gr[gi];
    uint64_t w0[AU_REL_MAX];
    uint64_t w1[AU_REL_MAX];
    uint64_t kk[AU_REL_MAX];
    int sd[AU_REL_MAX];
    uint64_t p0 = 0u;
    uint64_t p1 = 0u;
    uint64_t cn = 0u;
    unsigned n = 0u;
    unsigned r;
    uint64_t *cx = NULL;
    int out = AU_PASS;

    for (r = 0u; r < f->nr; r++) {
        struct au_sel s;
        struct au_lit l;
        const char *err;
        int v;

        if (f->rw[r].group != gi) {
            continue;
        }
        err = au_sel_parse(f->rw[r].sel, &s);
        if (err == NULL) {
            err = au_lit_parse(f->rw[r].lit, &l);
        }
        if (err != NULL || n >= AU_REL_MAX) {
            snprintf(why, cap, "row %u: %s", n + 1u, err != NULL ? err : "aexp-malformed: a REL group has at most 8 rows");
            return AU_HARNESS;
        }
        v = au_window(cp, &s, &w0[n], &w1[n], why, cap);
        if (v != AU_PASS) {
            return v;
        }
        sd[n] = s.side;
        kk[n] = 0u;
        if (g->kind == AK_PERIOD_RATIO && (au_lit_num(&l, "k", &kk[n]) != 0 || kk[n] == 0u || kk[n] >= AU_CROSS_MAX || (n == 0u && (au_lit_num(&l, "p0", &p0) != 0 || au_lit_num(&l, "p1", &p1) != 0 || p0 == 0u || p1 == 0u)))) {
            snprintf(why, cap, "aexp-malformed: a period_ratio row needs k (1 to 4095), the first also p0 and p1 (above 0)");
            return AU_HARNESS;
        }
        if (g->kind == AK_NOISE_RATIO && n == 0u && (au_lit_num(&l, "p0", &p0) != 0 || au_lit_num(&l, "p1", &p1) != 0 || p0 == 0u || p1 == 0u)) {
            snprintf(why, cap, "aexp-malformed: the first row of a noise_ratio group needs p0 and p1 (above 0)");
            return AU_HARNESS;
        }
        if ((g->kind == AK_SAME || g->kind == AK_LFO_NEEDS_SEL1) && n == 0u && (au_lit_num(&l, "n", &cn) != 0 || cn == 0u)) {
            snprintf(why, cap, "aexp-malformed: the first row of a same group needs n (the samples compared, above 0)");
            return AU_HARNESS;
        }
        n++;
    }
    if (n < 2u || ((g->kind == AK_PERIOD_RATIO || g->kind == AK_SAME || g->kind == AK_LFO_NEEDS_SEL1 || g->kind == AK_NOISE_RATIO) && n != 2u)) {
        snprintf(why, cap, "aexp-malformed: a REL group needs two rows (period_ratio, same, lfo_needs_sel1 and noise_ratio exactly two)");
        return AU_HARNESS;
    }
    if (g->kind == AK_NOISE_RATIO) {
        /* noise_ratio (plan 05-07, MAN:P8-2+P8-6+P8-9+H8-23): the grid of each window, the gcd of its interior runs, in the ratio p0 : p1 (grid 1 x p1 = grid 2 x p0); a window with no interior run fails. AF0 reads p0 + 1 */
        struct au_runs rs[2];

        for (r = 0u; r < 2u; r++) {
            au_runs_of(cp, w0[r], w1[r], sd[r], 0u, &rs[r]);
            if (rs[r].n == 0u) {
                snprintf(why, cap, "window %u has no interior run: no grid to compare", r + 1u);
                return AU_FAIL;
            }
        }
        p0 += perturb ? 1u : 0u;
        snprintf(why, cap, "the grids (the gcd of the interior runs) of the two windows are %llu and %llu samples, the literal ratio is %llu : %llu", (unsigned long long)rs[0].gcd, (unsigned long long)rs[1].gcd, (unsigned long long)p0, (unsigned long long)p1);
        return rs[0].gcd * p1 == rs[1].gcd * p0 ? AU_PASS : AU_FAIL;
    }
    if (g->kind == AK_PERIOD_RATIO || g->kind == AK_SAME || g->kind == AK_LFO_NEEDS_SEL1) { /* lfo_needs_sel1 (plan 05.1-01, D-208, D-219): the same judge, the CTL 0 and CTL 1 windows of select 0 with select 1 off */
        uint64_t dist[2];

        cx = (uint64_t *)malloc((size_t)AU_CROSS_MAX * sizeof *cx);
        if (cx == NULL) {
            snprintf(why, cap, "out of memory for the crossings");
            return AU_HARNESS;
        }
        for (r = 0u; r < 2u; r++) {
            unsigned nc = au_crossings(cp, w0[r], w1[r], sd[r], cx, AU_CROSS_MAX);

            if (g->kind == AK_PERIOD_RATIO) {
                if ((uint64_t)nc < kk[r] + 1u) {
                    snprintf(why, cap, "window %u holds %u rising crossings, fewer than k + 1 = %llu", r + 1u, nc, (unsigned long long)(kk[r] + 1u));
                    out = AU_FAIL;
                    break;
                }
                dist[r] = cx[kk[r]] - cx[0];
            } else {
                if (nc == 0u || cx[0] + cn + (r == 1u && perturb ? 1u : 0u) > w1[r]) {
                    snprintf(why, cap, "window %u has no rising crossing followed by %llu samples inside it", r + 1u, (unsigned long long)cn);
                    out = AU_FAIL;
                    break;
                }
                dist[r] = cx[0] + (r == 1u && perturb ? 1u : 0u);
            }
        }
        if (out == AU_PASS && g->kind == AK_PERIOD_RATIO) {
            p0 += perturb ? 1u : 0u;
            snprintf(why, cap, "the distances of the two windows are %llu and %llu samples, the literal ratio is %llu : %llu", (unsigned long long)dist[0], (unsigned long long)dist[1], (unsigned long long)p0, (unsigned long long)p1);
            out = dist[0] * p1 == dist[1] * p0 ? AU_PASS : AU_FAIL;
        } else if (out == AU_PASS) {
            uint64_t j;

            snprintf(why, cap, "the %llu samples from the first rising crossings (%llu and %llu) are equal", (unsigned long long)cn, (unsigned long long)dist[0], (unsigned long long)dist[1]);
            for (j = 0u; j < cn; j++) {
                if (au_sample(cp, dist[0] + j, sd[0]) != au_sample(cp, dist[1] + j, sd[1])) {
                    snprintf(why, cap, "sample %llu after the first rising crossings differs (%u against %u)", (unsigned long long)j, (unsigned)au_sample(cp, dist[0] + j, sd[0]), (unsigned)au_sample(cp, dist[1] + j, sd[1]));
                    out = AU_FAIL;
                    break;
                }
            }
        }
        free(cx);
        return out;
    }
    {
        uint32_t pk[AU_REL_MAX];

        for (r = 0u; r < n; r++) {
            pk[r] = au_peak(cp, w0[r], w1[r], sd[r]);
        }
        if (g->kind == AK_ATTEN_EQUAL) {
            pk[0] += perturb ? 1u : 0u;
        }
        for (r = 1u; r < n; r++) {
            int ok = g->kind == AK_ATTEN_ORDER ? (perturb && r == 1u ? pk[1] > pk[0] : pk[r] < pk[r - 1u]) : pk[r] == pk[0];

            if (!ok) {
                snprintf(why, cap, "the peak of window %u is %u against %u before it: not %s", r + 1u, (unsigned)pk[r], (unsigned)pk[r - 1u], g->kind == AK_ATTEN_ORDER ? "strictly decreasing" : "equal");
                return AU_FAIL;
            }
        }
        snprintf(why, cap, "the %u peaks (the first %u, the last %u) are %s", n, (unsigned)pk[0], (unsigned)pk[n - 1u], g->kind == AK_ATTEN_ORDER ? "strictly decreasing" : "equal");
        return AU_PASS;
    }
}

/* one group of the scene on a capture: AU_PASS when every row passes; why holds the first failing row's reason (or the last row's). perturb set: the first row reads a perturbed literal (AF0) */
static int au_eval_group(const struct ax_file *f, unsigned gi, const struct au_cap *cp, int perturb, char *why, size_t cap)
{
    const struct ax_group *g = &f->gr[gi];
    unsigned r;
    unsigned nrow = 0u;
    int out = AU_PASS;

    if (g->form == AX_FORM_REL) {
        return au_rel_group(f, gi, cp, perturb, why, cap);
    }
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
        v = g->form == AX_FORM_EV ? au_ev_row(cp, g, &s, &l, perturb && nrow == 1u, rwhy, sizeof rwhy)
            : g->form == AX_FORM_STRUCT ? au_struct_row(cp, g, &s, &l, perturb && nrow == 1u, rwhy, sizeof rwhy)
                                        : au_rows_row(cp, g, &s, &l, perturb && nrow == 1u, rwhy, sizeof rwhy);
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

/* the golden role (ER-20, D-209): NULL when the golden scene si may be judged, else the reason of the refusal audio-hash-only. A golden scene is valid only beside a literal scene of the same ROM (the same file) whose audio_fault_sweep case exists among the cases of this run,
 * and a ROM has at most one golden scene: a stream digest is a change detector and never a verdict */
static const char *au_golden_refusal(const struct ax_file *f, unsigned si)
{
    unsigned i;
    unsigned ngold = 0u;
    int lit = 0;
    int swept = 0;

    for (i = 0u; i < f->ns; i++) {
        ngold += f->sc[i].role == AX_ROLE_GOLDEN ? 1u : 0u;
        lit = lit || f->sc[i].role == AX_ROLE_LITERAL;
    }
    if (f->sc[si].role != AX_ROLE_GOLDEN || ngold != 1u) {
        return "audio-hash-only: a ROM has at most one golden scene (D-209)";
    }
    if (!lit) {
        return "audio-hash-only: a golden scene is valid only beside a literal scene of the same ROM (a digest alone is not a verdict, ER-20)";
    }
    for (i = 0u; i < f->ns && !swept; i++) {
        size_t c;

        if (f->sc[i].role != AX_ROLE_LITERAL) {
            continue;
        }
        for (c = 0u; c < case_count() && !swept; c++) {
            const struct kase *k2 = case_get(c);
            char want[AU_NAME + 8u];
            const char *ex = k2->has[K_EXPECT] ? k2->val[K_EXPECT] : "";
            const char *at;

            snprintf(want, sizeof want, "scene=%s", f->sc[i].name);
            at = strstr(ex, want);
            if (k2->has[K_KIND] && strcmp(k2->val[K_KIND], "audio_fault_sweep") == 0 && at != NULL && (at == ex || at[-1] == ';') && (at[strlen(want)] == '\0' || at[strlen(want)] == ';')) {
                swept = 1;
            }
        }
    }
    return swept ? NULL : "audio-hash-only: the literal scene beside the golden scene has no audio_fault_sweep case, so its groups are not shown live (ER-20)";
}

/* the golden row of scene name, or NULL */
static const struct ax_golden *au_golden_row(const struct ax_file *f, const char *name)
{
    unsigned i;

    for (i = 0u; i < f->ngold; i++) {
        if (strcmp(f->gd[i].scene, name) == 0) {
            return &f->gd[i];
        }
    }
    return NULL;
}

/* the golden case: one run of the ROM under the zero policy, both stream digests against the pinned ones (E0 change detectors) */
static void au_golden_case(const struct kase *k, struct ctx *c, const struct ax_file *f, unsigned si, const struct au_expect *e, uint64_t until, char *got, size_t gotcap)
{
    struct au_cap cp;
    char req[600];
    char pcm_hex[2u * DS_SHA256_BYTES + 1u];
    char psg_hex[2u * DS_SHA256_BYTES + 1u];
    char det[900];
    const char *why = au_golden_refusal(f, si);
    const struct ax_golden *gd = au_golden_row(f, f->sc[si].name);

    memset(&cp, 0, sizeof cp);
    if (why != NULL) {
        harness_error(c, why);
        return;
    }
    if (gd == NULL) {
        harness_error(c, "aexp-malformed: the golden scene has no golden row with its two digests");
        return;
    }
    if (vd_policy_request(0u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
        harness_error(c, "the request of the golden case is too long");
        return;
    }
    if (au_capture(c, e->rom, req, until, e, &cp, 1, got, gotcap) != 0) {
        au_cap_free(&cp);
        return;
    }
    if (au_digests(&cp, pcm_hex, psg_hex) != 0) {
        harness_error(c, "out of memory for the audio streams");
        au_cap_free(&cp);
        return;
    }
    check(c, strcmp(pcm_hex, gd->pcm) == 0, "golden: the DSPA stream digest differs from the pinned pcm_sha256 (a change detector, E0; the literal case of the ROM decides what is right, ER-20)");
    check(c, strcmp(psg_hex, gd->psg) == 0, "golden: the DSPE stream digest differs from the pinned psg_sha256 (a change detector, E0; the literal case of the ROM decides what is right, ER-20)");
    snprintf(det, sizeof det, "pcm_sha256=%s psg_sha256=%s E0 repeatability (a change detector beside the literal scene of the same ROM, never a verdict, ER-20)\naudio_golden: scene %s, pinned pcm_sha256=%s psg_sha256=%s; audio_frames=%u audio_events=%u machine_time=%llu\n", pcm_hex, psg_hex, f->sc[si].name,
             gd->pcm, gd->psg, (unsigned)cp.fcount, (unsigned)cp.ecount, (unsigned long long)cp.machine_time);
    set_evidence_detail(det);
    snprintf(got + strlen(got), gotcap - strlen(got), " golden frames=%u events=%u", (unsigned)cp.fcount, (unsigned)cp.ecount);
    au_cap_free(&cp);
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
    if (f->sc[si].role == AX_ROLE_EXPECT_RED) {
        unsigned t;

        for (t = 0u; t < f->ns && !(strcmp(f->sc[t].name, f->sc[si].twin) == 0 && f->sc[t].role == AX_ROLE_LITERAL); t++) {
        }
        if (t == f->ns || e.nred != 0u || au_red_list(f->sc[si].red, &e) != 0) {
            harness_error(c, "an expect_red scene names a literal scene of its file with twin= and lists its red groups with expect_red=, and its case carries no expect_red of its own (D-211)");
            goto done;
        }
    } else if (f->sc[si].role == AX_ROLE_GOLDEN) {
        au_golden_case(k, c, f, si, &e, until, got, gotcap);
        goto done;
    } else if (f->sc[si].role != AX_ROLE_LITERAL) {
        harness_error(c, "the scene is not a literal, expect_red or golden scene");
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
        verdicts[gi] = au_eval_group(f, gi, &cap0, 0, why, sizeof why);
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

/* ---- the fault sweep of D-209 (plan 05-05): AF0..AF9 applied in memory to copies of the captured streams of one run ---- */

#define AU_AF_N 10u
#define AU_AF6_LEVEL 12345u /* the constant non-silent value of AF6 */

/* a deep copy of the DSPE records and DSPA frames of src into dst, with room for the samples AF4 inserts; 0 on success (the dev records are not copied: no judge reads them) */
static int au_cap_copy(const struct au_cap *src, struct au_cap *dst)
{
    size_t fcap = (size_t)src->fcount + (size_t)src->fcount / 32u + 2u;
    size_t ecap = (size_t)src->ecount + 1u;

    *dst = *src;
    dst->drec = NULL;
    dst->dcount = 0u;
    dst->dcap = 0u;
    dst->erec = (uint8_t *)malloc(ecap * (size_t)AU_E_REC);
    dst->frames = (uint8_t *)malloc(fcap * (size_t)AU_F_BYTES);
    if (dst->erec == NULL || dst->frames == NULL) {
        au_cap_free(dst);
        return -1;
    }
    if (src->ecount > 0u) {
        memcpy(dst->erec, src->erec, (size_t)src->ecount * (size_t)AU_E_REC);
    }
    if (src->fcount > 0u) {
        memcpy(dst->frames, src->frames, (size_t)src->fcount * (size_t)AU_F_BYTES);
    }
    dst->ecap = (uint32_t)ecap;
    dst->fcap = (uint32_t)fcap;
    return 0;
}

/* where the faults of a group act: the window [i0, i1) of its first windowed row (the whole stream when it has none) and the event record of its first row (the start anchor, the anchor plus ev=+K, or the end anchor; UINT32_MAX for none) */
struct au_target {
    int has_win;
    uint64_t i0;
    uint64_t i1;
    uint32_t ev;
};

static void au_target_of(const struct ax_file *f, unsigned gi, const struct au_cap *cp, struct au_target *t)
{
    unsigned r;
    int first = 1;

    memset(t, 0, sizeof *t);
    t->ev = UINT32_MAX;
    for (r = 0u; r < f->nr; r++) {
        struct au_sel s;
        char why[200];
        uint64_t i0 = 0u;
        uint64_t i1 = 0u;

        if (f->rw[r].group != gi || au_sel_parse(f->rw[r].sel, &s) != NULL) {
            continue;
        }
        if (first) {
            uint32_t ai = s.a.set ? au_find(cp, &s.a) : s.e.set ? au_find(cp, &s.e) : UINT32_MAX;

            if (ai != UINT32_MAX && (uint64_t)ai + (s.has_ev ? s.ev_off : 0u) < (uint64_t)cp->ecount) {
                t->ev = (uint32_t)((uint64_t)ai + (s.has_ev ? s.ev_off : 0u));
            }
            first = 0;
        }
        if (!t->has_win && s.side != 0 && s.from_kind != 0 && s.to_kind != 0 && !s.has_ev && au_window(cp, &s, &i0, &i1, why, sizeof why) == AU_PASS) {
            t->has_win = 1;
            t->i0 = i0;
            t->i1 = i1;
        } else if (!t->has_win && s.side == 0 && s.from_kind == 2 && s.to_kind == 2 && s.to > s.from) {
            t->has_win = 1;
            t->i0 = au_index(cp, s.from);
            t->i1 = au_index(cp, s.to);
        }
    }
    if (!t->has_win) {
        t->i0 = 0u;
        t->i1 = cp->fcount;
    }
}

static void au_put_frame(struct au_cap *x, uint64_t i, uint32_t l, uint32_t r)
{
    au_put32(x->frames + (size_t)i * AU_F_BYTES, l);
    au_put32(x->frames + (size_t)i * AU_F_BYTES + 4u, r);
}

/* apply fault AFaf of D-209 to the copy x (AF0 changes no stream: the judge reads a perturbed literal instead) */
static void au_fault(struct au_cap *x, unsigned af, const struct au_target *t)
{
    uint64_t i0 = t->i0 < x->fcount ? t->i0 : x->fcount;
    uint64_t i1 = t->i1 < x->fcount ? t->i1 : x->fcount;
    uint64_t i;

    if (af == 1u) { /* AF1: the PCM one sample later against the log */
        x->first += 1u;
    } else if (af == 2u && i1 > i0) { /* AF2: drop one sample mid-window */
        uint64_t m = i0 + (i1 - i0) / 2u;

        memmove(x->frames + (size_t)m * AU_F_BYTES, x->frames + (size_t)(m + 1u) * AU_F_BYTES, (size_t)(x->fcount - m - 1u) * AU_F_BYTES);
        x->fcount--;
    } else if (af == 3u) { /* AF3: swap L and R */
        for (i = 0u; i < x->fcount; i++) {
            au_put_frame(x, i, au_sample(x, i, 2), au_sample(x, i, 1));
        }
    } else if (af == 4u) { /* AF4: duplicate every 32nd sample of the window */
        for (i = i1; i > i0; i--) {
            uint64_t j = i - 1u;

            if ((j - i0) % 32u == 31u && x->fcount < x->fcap) {
                memmove(x->frames + (size_t)(j + 1u) * AU_F_BYTES, x->frames + (size_t)j * AU_F_BYTES, (size_t)(x->fcount - j) * AU_F_BYTES);
                x->fcount++;
            }
        }
    } else if (af == 5u || af == 6u) { /* AF5: zero the window; AF6: a constant non-silent window */
        for (i = i0; i < i1; i++) {
            au_put_frame(x, i, af == 5u ? 0u : AU_AF6_LEVEL, af == 5u ? 0u : AU_AF6_LEVEL);
        }
    } else if (af == 7u) { /* AF7: reverse each whole 32-sample cycle of the window */
        uint64_t b;

        for (b = i0; b + 32u <= i1; b += 32u) {
            for (i = 0u; i < 16u; i++) {
                uint32_t l = au_sample(x, b + i, 1);
                uint32_t r = au_sample(x, b + i, 2);

                au_put_frame(x, b + i, au_sample(x, b + 31u - i, 1), au_sample(x, b + 31u - i, 2));
                au_put_frame(x, b + 31u - i, l, r);
            }
        }
    } else if (af == 8u && x->ecount > 0u) { /* AF8: drop one event record (the group's own, else the middle one) */
        uint32_t d = t->ev < x->ecount ? t->ev : x->ecount / 2u;

        memmove(x->erec + (size_t)d * AU_E_REC, x->erec + (size_t)(d + 1u) * AU_E_REC, (size_t)(x->ecount - d - 1u) * AU_E_REC);
        x->ecount--;
    } else if (af == 9u) { /* AF9: relabel one event's channel k -> k + 1 mod 6 (the first channel record from the group's own) */
        uint32_t e;

        for (e = t->ev < x->ecount ? t->ev : 0u; e < x->ecount; e++) {
            uint8_t *r = x->erec + (size_t)e * AU_E_REC;

            if (r[AU_E_CHAN] <= 5u) {
                r[AU_E_CHAN] = (uint8_t)((r[AU_E_CHAN] + 1u) % 6u);
                break;
            }
        }
    }
}

static void au_mask_text(unsigned mask, char *out, size_t cap)
{
    unsigned af;
    size_t n = 0u;

    out[0] = '\0';
    for (af = 0u; af < AU_AF_N; af++) {
        if ((mask & (1u << af)) != 0u && n + 5u < cap) {
            n += (size_t)snprintf(out + n, cap - n, "%sAF%u", n > 0u ? "," : "", af);
        }
    }
    if (n == 0u && cap > 1u) {
        out[0] = '-';
        out[1] = '\0';
    }
}

static unsigned au_popcount(unsigned m)
{
    unsigned n = 0u;

    while (m != 0u) {
        n += m & 1u;
        m >>= 1;
    }
    return n;
}

/* the observed kill set of group gi on the capture cp: bit af of *flips set when the group FAILS under AFaf (its judged comparison turned red), bit af of *harness set when it gives a harness outcome instead (the fault made a row malformed or its window empty). A harness outcome
 * is never a kill (D-209; the rule of the video sweep, videokinds.c vk_sweep_group). hwhy gets the reason of the first harness outcome of a declared kill (empty when none); -1 when out of memory */
static int au_kill_set(const struct ax_file *f, unsigned gi, const struct au_cap *cp, unsigned *flips, unsigned *harness, char *hwhy, size_t hcap)
{
    struct au_target t;
    unsigned af;

    *flips = 0u;
    *harness = 0u;
    hwhy[0] = '\0';
    au_target_of(f, gi, cp, &t);
    for (af = 0u; af < AU_AF_N; af++) {
        struct au_cap x;
        char why[320];
        int v;

        if (au_cap_copy(cp, &x) != 0) {
            return -1;
        }
        au_fault(&x, af, &t);
        v = au_eval_group(f, gi, &x, af == 0u, why, sizeof why);
        if (v == AU_FAIL) {
            *flips |= 1u << af;
        } else if (v == AU_HARNESS) {
            if (hwhy[0] == '\0' && (f->gr[gi].kills & (1u << af)) != 0u) {
                snprintf(hwhy, hcap, "AF%u: %.300s", af, why);
            }
            *harness |= 1u << af;
        }
        au_cap_free(&x);
    }
    return 0;
}

/* the sweep verdict of one group (D-209): AU_HARNESS when a declared kill gave a harness outcome (the fault broke a row instead of the judged comparison, so it shows nothing), AU_FAIL when a declared kill left the group green or no declared kill flips it, else AU_PASS */
static int au_sweep_verdict(unsigned kills, unsigned flips, unsigned harness)
{
    if ((kills & harness) != 0u) {
        return AU_HARNESS;
    }
    if ((kills & ~flips) != 0u || (kills & flips) == 0u) {
        return AU_FAIL;
    }
    return AU_PASS;
}

void kind_audio_fault_sweep(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct au_expect e;
    struct ax_file *f = NULL;
    struct au_cap cap0;
    char req[600];
    char detail[4096];
    size_t used = 0u;
    uint64_t until = 0u;
    unsigned si;
    unsigned gi;
    unsigned ngroups = 0u;
    unsigned ndecl = 0u;
    unsigned nflip = 0u;

    memset(&cap0, 0, sizeof cap0);
    if (!k->has[K_EXPECT] || au_parse(k->val[K_EXPECT], &e) != 0 || e.nred != 0u || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an audio_fault_sweep case needs until_mclk and expect = rom=NAME;id=N;checks=N;scene=NAME (every item once, no expect_red)");
        return;
    }
    if (k->has[K_IRQ_STIM] || k->has[K_INPUT]) {
        harness_error(c, "an audio_fault_sweep case declares no irq_stim and no input");
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
    for (si = 0u; si < f->ns && strcmp(f->sc[si].name, e.scene) != 0; si++) {
    }
    if (si == f->ns || f->sc[si].role != AX_ROLE_LITERAL) {
        harness_error(c, "the scene of a fault sweep must be a literal scene of the aexp rows (D-209)");
        goto done;
    }
    if (vd_policy_request(0u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
        harness_error(c, "the request of the audio_fault_sweep case is too long");
        goto done;
    }
    if (au_capture(c, e.rom, req, until, &e, &cap0, 1, got, gotcap) != 0) {
        goto done;
    }
    if (cap0.smclk != f->sample_mclk) {
        harness_error(c, "the aexp header sample_mclk does not equal the DSPA header's sample_mclk (D-207)");
        goto done;
    }
    detail[0] = '\0';
    for (gi = 0u; gi < f->ng; gi++) {
        const struct ax_group *g = &f->gr[gi];
        char why[320];
        char msg[440];
        char line[360];
        char hwhy[320];
        char dtxt[64];
        char ftxt[64];
        char mtxt[64];
        char htxt[64];
        unsigned flips = 0u;
        unsigned harness = 0u;
        int base;

        if (strcmp(g->scene, e.scene) != 0) {
            continue;
        }
        base = au_eval_group(f, gi, &cap0, 0, why, sizeof why);
        if (base == AU_HARNESS) {
            snprintf(msg, sizeof msg, "group %.31s: %.360s", g->gid, why);
            harness_error(c, msg);
            goto done;
        }
        ngroups++;
        if (base != AU_PASS) {
            snprintf(msg, sizeof msg, "group %.31s fails on the unfaulted streams, so no fault can show it live: %.300s", g->gid, why);
            check(c, 0, msg);
            continue;
        }
        if (au_kill_set(f, gi, &cap0, &flips, &harness, hwhy, sizeof hwhy) != 0) {
            harness_error(c, "out of memory for the faulted copies of the streams");
            goto done;
        }
        if (au_sweep_verdict(g->kills, flips, harness) == AU_HARNESS) {
            snprintf(msg, sizeof msg, "group %.31s: a declared kill gave a harness outcome, not a failure of the judged comparison, so it is no kill (D-209): %.280s", g->gid, hwhy);
            harness_error(c, msg);
            goto done;
        }
        ndecl += au_popcount(g->kills);
        nflip += au_popcount(g->kills & flips);
        au_mask_text(g->kills, dtxt, sizeof dtxt);
        au_mask_text(flips, ftxt, sizeof ftxt);
        au_mask_text(g->kills & ~flips, mtxt, sizeof mtxt);
        au_mask_text(harness, htxt, sizeof htxt);
        snprintf(msg, sizeof msg, "group %.31s: the declared kill(s) %.40s do not flip it (declared %.40s, observed %.40s; D-209)", g->gid, mtxt, dtxt, ftxt);
        check(c, (g->kills & ~flips) == 0u, msg);
        snprintf(msg, sizeof msg, "group %.31s: no declared fault flips it (declared %.40s): a vacuous group (D-209)", g->gid, dtxt);
        check(c, (g->kills & flips) != 0u, msg);
        snprintf(line, sizeof line, "audio_sweep %.31s %.95s declared=%.40s observed=%.40s missing=%.40s harness=%.40s\n", g->gid, g->cls, dtxt, ftxt, mtxt, htxt);
        if (used + strlen(line) + 1u < sizeof detail) {
            memcpy(detail + used, line, strlen(line) + 1u);
            used += strlen(line);
        }
    }
    if (ngroups == 0u) {
        harness_error(c, "the scene of the fault sweep has no group");
        goto done;
    }
    {
        char head[300];

        snprintf(head, sizeof head, "audio_sweep: groups=%u declared=%u flipped=%u (E0: each literal group shown live on in-memory faults AF0..AF9 of the captured streams of one run, D-209; observed= the faults that fail it, harness= those that only break a row, never a kill)\n", ngroups,
                 ndecl, nflip);
        if (strlen(head) + used + 1u <= sizeof detail) {
            memmove(detail + strlen(head), detail, used + 1u);
            memcpy(detail, head, strlen(head));
        }
        set_evidence_detail(detail);
    }
    {
        size_t gl = strlen(got);

        snprintf(got + gl, gotcap - gl, " groups=%u flipped=%u/%u", ngroups, nflip, ndecl);
    }
done:
    au_cap_free(&cap0);
    free(f);
}

/* ---- audio_judge_forms (plan 05-05 Task 2; the jf_want pattern of videokinds.c): pass, fail and harness rows of every form and judged kind on a synthetic capture, the AF0 perturbation of each, the null-synth controls on psg_tone's own event stream and the missing-sink rule ---- */

#define AJ_FRAMES 4096u
#define AJ_KEYOFF 2048u /* the synthetic key-off sample: silence from here */

struct aj_ctx {
    struct ctx *c;
    struct ax_file *f;
    unsigned checks;
    char failed[1500];
};

static void aj_want(struct aj_ctx *j, const char *name, int got, int want)
{
    char msg[240];

    snprintf(msg, sizeof msg, "judge forms: %.180s (verdict %d, wanted %d)", name, got, want);
    check(j->c, got == want, msg);
    j->checks++;
    if (got != want && strlen(j->failed) + strlen(msg) + 2u < sizeof j->failed) {
        strcat(j->failed, msg);
        strcat(j->failed, "\n");
    }
}

/* the synthetic word of index w: 0..15 rising, then 31 down to 16, so a cycle has one rising crossing (15 -> 31) and 32 distinct plateaus */
static uint32_t aj_word(unsigned w)
{
    return w < 16u ? w : 47u - w;
}

/* the synthetic capture: key-on (R4 $9F on select 0) at stamp 0 and key-off (R4 $1F) at sample 2048; samples 0..1023 the words x 1000 with 4 samples per word (a cycle of 128), samples 1024..2047 the same x 500, then silence; L = R; the headers of a sound run */
static int aj_stream(struct au_cap *x)
{
    uint32_t i;

    memset(x, 0, sizeof *x);
    x->erec = (uint8_t *)calloc(2u, AU_E_REC);
    x->frames = (uint8_t *)calloc(AJ_FRAMES, AU_F_BYTES);
    if (x->erec == NULL || x->frames == NULL) {
        au_cap_free(x);
        return -1;
    }
    for (i = 0u; i < 2u; i++) {
        uint8_t *r = x->erec + (size_t)i * AU_E_REC;

        au_put64(r + AU_E_STAMP, i == 0u ? 0u : (uint64_t)AJ_KEYOFF * 192u);
        r[AU_E_REG] = 4u;
        r[AU_E_VALUE] = i == 0u ? 0x9Fu : 0x1Fu;
        r[AU_E_CHAN] = 0u;
        r[AU_E_POST] = r[AU_E_VALUE];
        r[AU_E_VERSION] = 1u;
    }
    for (i = 0u; i < AJ_FRAMES; i++) {
        uint32_t v = i < 1024u ? 1000u * aj_word((i / 4u) % 32u) : i < AJ_KEYOFF ? 500u * aj_word((i / 4u) % 32u) : 0u;

        au_put_frame(x, i, v, v);
    }
    x->ecount = 2u;
    x->ecap = 2u;
    x->fcount = AJ_FRAMES;
    x->fcap = AJ_FRAMES;
    x->smclk = 192u;
    x->lf = 357630u;
    x->hdr_fcount = AJ_FRAMES;
    x->hdr_ecount = 2u;
    x->pfbytes = 8u;
    x->pversion = 1u;
    x->pformat = 1u;
    x->machine_time = (uint64_t)AJ_FRAMES * 192u;
    return 0;
}

/* a synthetic capture of whole cycles (plan 05.1-02, the lfo_dynamic rows): one record (R4 $9F on select 0 at stamp 0), 10 samples of 0, then for each pair (length, count) of spec that many cycles of that many samples, each 1000 for its first half and 0 after it, so
 * every cycle starts with its one rising crossing, then 10 samples of 1000 (the crossing that closes the last cycle); L = R. *nf gets the frame count; 0, or -1 when out of memory */
static int aj_cycles(struct au_cap *x, const uint32_t *spec, unsigned npair, uint32_t *nf)
{
    uint32_t total = 20u;
    uint32_t at = 10u;
    uint32_t i;
    unsigned p;

    for (p = 0u; p < npair; p++) {
        total += spec[2u * p] * spec[2u * p + 1u];
    }
    memset(x, 0, sizeof *x);
    x->erec = (uint8_t *)calloc(1u, AU_E_REC);
    x->frames = (uint8_t *)calloc(total, AU_F_BYTES);
    if (x->erec == NULL || x->frames == NULL) {
        au_cap_free(x);
        return -1;
    }
    x->erec[AU_E_REG] = 4u;
    x->erec[AU_E_VALUE] = 0x9Fu;
    x->erec[AU_E_CHAN] = 0u;
    x->erec[AU_E_POST] = 0x9Fu;
    x->erec[AU_E_VERSION] = 1u;
    for (p = 0u; p < npair; p++) {
        uint32_t k;

        for (k = 0u; k < spec[2u * p + 1u]; k++) {
            for (i = 0u; i < spec[2u * p]; i++) {
                uint32_t v = i < spec[2u * p] / 2u ? 1000u : 0u;

                au_put_frame(x, at++, v, v);
            }
        }
    }
    for (i = 0u; i < 10u; i++) {
        au_put_frame(x, at++, 1000u, 1000u);
    }
    x->ecount = 1u;
    x->ecap = 1u;
    x->fcount = total;
    x->fcap = total;
    x->smclk = 192u;
    x->lf = 357630u;
    x->hdr_fcount = total;
    x->hdr_ecount = 1u;
    x->pfbytes = 8u;
    x->pversion = 1u;
    x->pformat = 1u;
    x->machine_time = (uint64_t)total * 192u;
    *nf = total;
    return 0;
}

static int aj_eval(struct aj_ctx *j, const char *form, const char *kind, const char *sel1, const char *lit1, const char *sel2, const char *lit2, const struct au_cap *x, int perturb);

/* one lfo_dynamic row over the whole synthetic capture of spec (window from the record at 0 to the last frame, side L), judged with lit and perturb, against want */
static void aj_dyn(struct aj_ctx *j, const char *name, const uint32_t *spec, unsigned npair, const char *lit, int perturb, int want)
{
    struct au_cap y;
    uint32_t nf = 0u;
    char sel[120];

    if (aj_cycles(&y, spec, npair, &nf) != 0) {
        aj_want(j, name, -1, want); /* out of memory: the check fails */
        return;
    }
    snprintf(sel, sizeof sel, "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+0;to=+%llu;side=L", (unsigned long long)nf * 192u);
    aj_want(j, name, aj_eval(j, "ROWS", "lfo_dynamic", sel, lit, NULL, NULL, &y, perturb), want);
    au_cap_free(&y);
}

#define AJ_DYN "f=192;w1=$18;w2=$08;f2=128;flf=150;step=115200;set=200,184"

/* the lfo_dynamic rule of plan 05.1-02, one row per rule (C-06): a pass (runs of 200 and 184 with one straddling cycle at each switch), the boundary interior run of 40, one length only, a third length inside a run, two straddling cycles between two runs, an interior run of
 * 39, a straddling cycle at the window's edge, AF0 (the first set length + 1) and a literal without set */
static void aj_lfo_dynamic(struct aj_ctx *j)
{
    static const uint32_t pass[] = {200u, 45u, 192u, 1u, 184u, 50u, 191u, 1u, 200u, 10u};
    static const uint32_t run40[] = {200u, 45u, 192u, 1u, 184u, 40u, 192u, 1u, 200u, 45u};
    static const uint32_t one[] = {200u, 100u};
    static const uint32_t third[] = {200u, 45u, 190u, 1u, 200u, 45u, 192u, 1u, 184u, 45u};
    static const uint32_t two[] = {200u, 45u, 192u, 1u, 188u, 1u, 184u, 45u};
    static const uint32_t run39[] = {200u, 45u, 192u, 1u, 184u, 39u, 192u, 1u, 200u, 45u};
    static const uint32_t edge[] = {192u, 1u, 200u, 45u, 184u, 45u};

    aj_dyn(j, "ROWS lfo_dynamic runs of 200 and 184 with one straddling cycle at each switch passes", pass, 5u, AJ_DYN, 0, AU_PASS);
    aj_dyn(j, "ROWS lfo_dynamic with an interior run of 40 cycles passes", run40, 5u, AJ_DYN, 0, AU_PASS);
    aj_dyn(j, "ROWS lfo_dynamic with one length only (200) fails", one, 1u, AJ_DYN, 0, AU_FAIL);
    aj_dyn(j, "ROWS lfo_dynamic with a third length (190) inside a run of 200 fails", third, 5u, AJ_DYN, 0, AU_FAIL);
    aj_dyn(j, "ROWS lfo_dynamic with two straddling cycles (192, 188) between two runs fails", two, 4u, AJ_DYN, 0, AU_FAIL);
    aj_dyn(j, "ROWS lfo_dynamic with an interior run of 39 cycles fails", run39, 5u, AJ_DYN, 0, AU_FAIL);
    aj_dyn(j, "ROWS lfo_dynamic with a cycle of another length at the window's edge fails", edge, 3u, AJ_DYN, 0, AU_FAIL);
    aj_dyn(j, "ROWS lfo_dynamic AF0 (the first set length + 1) fails", pass, 5u, AJ_DYN, 1, AU_FAIL);
    aj_dyn(j, "ROWS lfo_dynamic without set is harness", pass, 5u, "f=192;w1=$18;w2=$08;f2=128;flf=150;step=115200", 0, AU_HARNESS);
}

/* one group of FORM and KIND (class ASSUMED, kills AF0) with up to two rows into j->f as group 0; 0, or -1 when the rows do not parse */
static int aj_load(struct aj_ctx *j, const char *form, const char *kind, const char *sel1, const char *lit1, const char *sel2, const char *lit2)
{
    char lines[5][AX_LINE + 1u];
    unsigned n = 0u;
    unsigned i;
    const char *err = NULL;

    memset(j->f, 0, sizeof *j->f);
    snprintf(lines[n++], sizeof lines[0], "aexp\tsample_mclk=192");
    snprintf(lines[n++], sizeof lines[0], "scene\tt\tcase=audio_t\trole=literal");
    snprintf(lines[n++], sizeof lines[0], "group\tt\tg\tform=%s\tkind=%s\tclass=ASSUMED:LED-AUD-0009\tkills=AF0", form, kind);
    snprintf(lines[n++], sizeof lines[0], "row\tt\tg\t%.180s\t%.180s", sel1, lit1);
    if (sel2 != NULL) {
        snprintf(lines[n++], sizeof lines[0], "row\tt\tg\t%.180s\t%.180s", sel2, lit2);
    }
    for (i = 0u; i < n && err == NULL; i++) {
        char *fld[8];
        unsigned nf = ax_split(lines[i], fld, 8u);

        err = nf > 7u ? "too many fields" : ax_row_parse(j->f, fld, nf);
    }
    if (err == NULL) {
        err = ax_link(j->f);
    }
    return err != NULL ? -1 : 0;
}

/* that group judged on x; the verdict, or AU_HARNESS when the rows do not parse */
static int aj_eval(struct aj_ctx *j, const char *form, const char *kind, const char *sel1, const char *lit1, const char *sel2, const char *lit2, const struct au_cap *x, int perturb)
{
    char why[320];

    if (aj_load(j, form, kind, sel1, lit1, sel2, lit2) != 0) {
        return AU_HARNESS;
    }
    return au_eval_group(j->f, 0u, x, perturb, why, sizeof why);
}

/*
 * The sweep's harness rule (D-209): a silent group whose start anchor is R4 of channel 0 with any value (nth=1: the key-on) and whose window runs from key-on + 393,216 (sample 2048, the key-off) to the key-off + 384,000 (sample 4048). AF8 drops the key-on record, so the start anchor
 * names the key-off and the window [786,432, 777,216) is empty: a harness outcome, which is no kill. AF6 (a constant non-silent window) fails the judged comparison: a kill. A sweep that declares AF8 is a harness failure; one that declares only AF6 passes.
 */
static void aj_sweep_harness(struct aj_ctx *j, const struct au_cap *x)
{
    unsigned flips = 0u;
    unsigned harness = 0u;
    char why[320];
    char hwhy[320];

    if (aj_load(j, "ROWS", "silent", "anchor=ev:ch=0;reg=R4;nth=1;from=+393216;to=ev:ch=0;reg=R4;val=$1F;nth=1+384000;side=R", "chon=0", NULL, NULL) != 0 || au_eval_group(j->f, 0u, x, 0, why, sizeof why) != AU_PASS ||
        au_kill_set(j->f, 0u, x, &flips, &harness, hwhy, sizeof hwhy) != 0) {
        aj_want(j, "sweep: the harness-rule group parses, passes unfaulted and is swept", 0, 1);
        return;
    }
    aj_want(j, "sweep: AF8 that empties the judged window gives a harness outcome and is not counted as a kill", (harness & (1u << 8)) != 0u && (flips & (1u << 8)) == 0u, 1);
    aj_want(j, "sweep: AF6 fails the judged comparison and is counted as a kill", (flips & (1u << 6)) != 0u && (harness & (1u << 6)) == 0u, 1);
    aj_want(j, "sweep: a declared kill seen only as a harness outcome makes the sweep a harness failure", au_sweep_verdict((1u << 6) | (1u << 8), flips, harness), AU_HARNESS);
    aj_want(j, "sweep: declared kills that each fail the judged comparison pass the sweep", au_sweep_verdict(1u << 6, flips, harness), AU_PASS);
}

#define AJ_A "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+0;to=+196608"
#define AJ_B "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+196608;to=+393216"
#define AJ_Q "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+393216;to=+786432"
#define AJ_WORDS "words=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16"
#define AJ_A2 "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+392448;to=+393984" /* samples 2044..2051: four of B's last plateau (word 16 x 500) and four of silence */
#define AJ_C "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+0;to=+24576"        /* samples 0..127: one cycle, its 32 plateaus from word 0 */
#define AJ_VALS "vals=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16"

static void aj_forms(struct aj_ctx *j, const struct au_cap *x)
{
    /* ROWS: period, cycle_order, silent, live, levels_exact (hand-derived from the construction: one rising crossing per 128-sample cycle at samples 64 + 128 m; the word 31 at x 1000 in A and x 500 in B) */
    aj_want(j, "ROWS period k=4 d=512 passes", aj_eval(j, "ROWS", "period", AJ_A ";side=L", "f=64;k=4;d=512", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS period d=511 fails", aj_eval(j, "ROWS", "period", AJ_A ";side=L", "f=64;k=4;d=511", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS period AF0 (d + 1) fails", aj_eval(j, "ROWS", "period", AJ_A ";side=L", "f=64;k=4;d=512", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS period k=0 is harness", aj_eval(j, "ROWS", "period", AJ_A ";side=L", "k=0;d=512", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS period without a side is harness", aj_eval(j, "ROWS", "period", AJ_A, "f=64;k=4;d=512", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS cycle_order of the words passes", aj_eval(j, "ROWS", "cycle_order", AJ_A ";side=R", AJ_WORDS, NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS cycle_order AF0 (two words swapped) fails", aj_eval(j, "ROWS", "cycle_order", AJ_A ";side=R", AJ_WORDS, NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS cycle_order of a rising table fails", aj_eval(j, "ROWS", "cycle_order", AJ_A ";side=R", "words=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS cycle_order without words is harness", aj_eval(j, "ROWS", "cycle_order", AJ_A ";side=R", "r1=$FF", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS silent after the key-off passes", aj_eval(j, "ROWS", "silent", AJ_Q ";side=L", "chon=0", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS silent AF0 (every sample 1) fails", aj_eval(j, "ROWS", "silent", AJ_Q ";side=L", "chon=0", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS silent over the tone fails", aj_eval(j, "ROWS", "silent", AJ_A ";side=L", "chon=0", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS silent with an empty window is harness", aj_eval(j, "ROWS", "silent", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+5;to=+6;side=L", "chon=0", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS silent with an end anchor plus an offset passes", aj_eval(j, "ROWS", "silent", "anchor=ev:ch=0;reg=R4;val=$1F;nth=1;from=+0;to=ev:ch=0;reg=R4;val=$1F;nth=1+384000;side=R", "chon=0", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS live over the tone passes", aj_eval(j, "ROWS", "live", AJ_B ";side=L", "r1=$FF", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS live AF0 (the rule inverted) fails", aj_eval(j, "ROWS", "live", AJ_B ";side=L", "r1=$FF", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS live over silence fails", aj_eval(j, "ROWS", "live", AJ_Q ";side=L", "r1=$FF", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS levels_exact peak 31 x 1000 of A passes", aj_eval(j, "ROWS", "levels_exact", AJ_A ";side=L", "peak=31000", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS levels_exact AF0 (peak + 1) fails", aj_eval(j, "ROWS", "levels_exact", AJ_A ";side=L", "peak=31000", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS levels_exact peak 31 x 1000 of B (31 x 500) fails", aj_eval(j, "ROWS", "levels_exact", AJ_B ";side=L", "peak=31000", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS levels_exact without peak is harness", aj_eval(j, "ROWS", "levels_exact", AJ_A ";side=L", "r1=$FF", NULL, NULL, x, 0), AU_HARNESS);
    /* plan 05-07: the noise and DDA kinds and cycle_phase on the same capture (every plateau is 4 samples; A2 holds the last 4 samples of B and 4 of silence: two values) */
    aj_want(j, "ROWS two_level n=2 over A2 passes", aj_eval(j, "ROWS", "two_level", AJ_A2 ";side=L", "n=2", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS two_level AF0 (n + 1) fails", aj_eval(j, "ROWS", "two_level", AJ_A2 ";side=L", "n=2", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS two_level n=2 over the 32 levels of A fails", aj_eval(j, "ROWS", "two_level", AJ_A ";side=L", "n=2", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS two_level without n is harness", aj_eval(j, "ROWS", "two_level", AJ_A ";side=L", "r1=$FF", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS run_grid g=4 passes", aj_eval(j, "ROWS", "run_grid", AJ_A ";side=L", "nf=2;g=4", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS run_grid g=3 fails", aj_eval(j, "ROWS", "run_grid", AJ_A ";side=L", "g=3", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS run_grid AF0 (g + 1) fails", aj_eval(j, "ROWS", "run_grid", AJ_A ";side=L", "nf=2;g=4", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS run_grid over silence (no interior run) fails", aj_eval(j, "ROWS", "run_grid", AJ_Q ";side=L", "nf=2;g=4", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS run_min minrun=4 passes", aj_eval(j, "ROWS", "run_min", AJ_A ";side=L", "minrun=4", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS run_min AF0 (minrun + 1) fails", aj_eval(j, "ROWS", "run_min", AJ_A ";side=L", "minrun=4", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS noise_seq with no R7 record before the anchor fails", aj_eval(j, "ROWS", "noise_seq", AJ_A ";side=L", "p0=11904;p1=2688;bits=10", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS noise_seq without bits is harness", aj_eval(j, "ROWS", "noise_seq", AJ_A ";side=L", "p0=11904;p1=2688", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS dda_hold with no DDA write in the window fails", aj_eval(j, "ROWS", "dda_hold", AJ_A ";side=L", "ords=1", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS dda_hold without ords is harness", aj_eval(j, "ROWS", "dda_hold", AJ_A ";side=L", "r1=$FF", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS dda_order of one cycle as the words passes", aj_eval(j, "ROWS", "dda_order", AJ_C ";side=L", AJ_VALS, NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS dda_order AF0 (two values swapped) fails", aj_eval(j, "ROWS", "dda_order", AJ_C ";side=L", AJ_VALS, NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS dda_level_exact 0 and 1000 passes", aj_eval(j, "ROWS", "dda_level_exact", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+0;to=+1536;side=L", "vals=0,1000", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS dda_level_exact AF0 (the first value + 1) fails", aj_eval(j, "ROWS", "dda_level_exact", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;from=+0;to=+1536;side=L", "vals=0,1000", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS cycle_phase of the words from word 0 passes", aj_eval(j, "ROWS", "cycle_phase", AJ_C ";side=L", AJ_WORDS, NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS cycle_phase of the words rotated by one fails (cycle_order passes it)", aj_eval(j, "ROWS", "cycle_phase", AJ_C ";side=L", "words=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,0", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS dda_ram_kept of the words passes", aj_eval(j, "ROWS", "dda_ram_kept", AJ_A ";side=R", AJ_WORDS, NULL, NULL, x, 0), AU_PASS);
    /* lfo_static (plan 05.1-01): the period judge on k and d, d = k x F'; here F' = 128 + $10 - 16 = 128 at shift 0, the 128-sample cycle of A, so k = 4 cycles are d = 512 */
    aj_want(j, "ROWS lfo_static f=128 w=$10 shift=0 k=4 d=512 passes", aj_eval(j, "ROWS", "lfo_static", AJ_A ";side=L", "f=128;w=$10;shift=0;k=4;d=512", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "ROWS lfo_static with a wrong d (4 x 130 = 520) fails", aj_eval(j, "ROWS", "lfo_static", AJ_A ";side=L", "f=128;w=$12;shift=0;k=4;d=520", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "ROWS lfo_static AF0 (d + 1) fails", aj_eval(j, "ROWS", "lfo_static", AJ_A ";side=L", "f=128;w=$10;shift=0;k=4;d=512", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "ROWS lfo_static without d is harness", aj_eval(j, "ROWS", "lfo_static", AJ_A ";side=L", "f=128;w=$10;shift=0;k=4", NULL, NULL, x, 0), AU_HARNESS);
    /* lfo_dynamic (plan 05.1-02): the row of a later-judged kind is gone (no kind is judged later); on the synthetic capture's one period a pass of runs is impossible, and the rows of each rule run on captures of whole cycles (aj_lfo_dynamic) */
    aj_want(j, "ROWS lfo_dynamic on one period only (128-sample cycles, set=200,184) fails", aj_eval(j, "ROWS", "lfo_dynamic", AJ_A ";side=L", AJ_DYN, NULL, NULL, x, 0), AU_FAIL);
    aj_lfo_dynamic(j);
    aj_want(j, "ROWS an unknown kind is refused", aj_eval(j, "ROWS", "loud", AJ_A ";side=L", "r1=$FF", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "ROWS the kind period under form REL is refused", aj_eval(j, "REL", "period", AJ_A ";side=L", "f=64;k=4;d=512", NULL, NULL, x, 0), AU_HARNESS);
    /* REL: period_ratio, same, atten_order, atten_equal (window A at x 1000, window B at x 500) */
    aj_want(j, "REL period_ratio 1:1 of A and B passes", aj_eval(j, "REL", "period_ratio", AJ_A ";side=L", "k=4;p0=1;p1=1", AJ_B ";side=L", "k=4", x, 0), AU_PASS);
    aj_want(j, "REL period_ratio 2:1 fails", aj_eval(j, "REL", "period_ratio", AJ_A ";side=L", "k=4;p0=2;p1=1", AJ_B ";side=L", "k=4", x, 0), AU_FAIL);
    aj_want(j, "REL period_ratio AF0 (p0 + 1) fails", aj_eval(j, "REL", "period_ratio", AJ_A ";side=L", "k=4;p0=1;p1=1", AJ_B ";side=L", "k=4", x, 1), AU_FAIL);
    aj_want(j, "REL period_ratio with one row is harness", aj_eval(j, "REL", "period_ratio", AJ_A ";side=L", "k=4;p0=1;p1=1", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "REL same of A on L and on R passes", aj_eval(j, "REL", "same", AJ_A ";side=L", "n=256", AJ_A ";side=R", "n=256", x, 0), AU_PASS);
    aj_want(j, "REL same AF0 (one sample later) fails", aj_eval(j, "REL", "same", AJ_A ";side=L", "n=256", AJ_A ";side=R", "n=256", x, 1), AU_FAIL);
    aj_want(j, "REL same of A and B fails", aj_eval(j, "REL", "same", AJ_A ";side=L", "n=256", AJ_B ";side=L", "n=256", x, 0), AU_FAIL);
    aj_want(j, "REL same without n is harness", aj_eval(j, "REL", "same", AJ_A ";side=L", "r1=$FF", AJ_B ";side=L", "r1=$FF", x, 0), AU_HARNESS);
    aj_want(j, "REL atten_order A then B passes", aj_eval(j, "REL", "atten_order", AJ_A ";side=L", "r1=$FF", AJ_B ";side=L", "r1=$EF", x, 0), AU_PASS);
    aj_want(j, "REL atten_order AF0 (an increase wanted) fails", aj_eval(j, "REL", "atten_order", AJ_A ";side=L", "r1=$FF", AJ_B ";side=L", "r1=$EF", x, 1), AU_FAIL);
    aj_want(j, "REL atten_order B then A fails", aj_eval(j, "REL", "atten_order", AJ_B ";side=L", "r1=$EF", AJ_A ";side=L", "r1=$FF", x, 0), AU_FAIL);
    aj_want(j, "REL atten_equal of A on L and R passes", aj_eval(j, "REL", "atten_equal", AJ_A ";side=L", "r1=$FF", AJ_A ";side=R", "r1=$FF", x, 0), AU_PASS);
    aj_want(j, "REL atten_equal AF0 (the first peak + 1) fails", aj_eval(j, "REL", "atten_equal", AJ_A ";side=L", "r1=$FF", AJ_A ";side=R", "r1=$FF", x, 1), AU_FAIL);
    aj_want(j, "REL atten_equal of A and B fails", aj_eval(j, "REL", "atten_equal", AJ_A ";side=L", "r1=$FF", AJ_B ";side=L", "r1=$FF", x, 0), AU_FAIL);
    aj_want(j, "REL noise_ratio 1:1 of A and B (both on a 4-sample grid) passes", aj_eval(j, "REL", "noise_ratio", AJ_A ";side=L", "p0=1;p1=1", AJ_B ";side=L", "r1=$FF", x, 0), AU_PASS);
    aj_want(j, "REL noise_ratio 1:2 fails", aj_eval(j, "REL", "noise_ratio", AJ_A ";side=L", "p0=1;p1=2", AJ_B ";side=L", "r1=$FF", x, 0), AU_FAIL);
    aj_want(j, "REL noise_ratio AF0 (p0 + 1) fails", aj_eval(j, "REL", "noise_ratio", AJ_A ";side=L", "p0=1;p1=1", AJ_B ";side=L", "r1=$FF", x, 1), AU_FAIL);
    /* lfo_needs_sel1 (plan 05.1-01): the same judge over two windows (no REL kind is judged from a later plan any more); the fail row compares A with a copy of the capture whose B is rewritten at x 1000 with 2 samples per word, a 64-sample cycle: the period alone differs */
    aj_want(j, "REL lfo_needs_sel1 of A on L and on R passes", aj_eval(j, "REL", "lfo_needs_sel1", AJ_A ";side=L", "n=256", AJ_A ";side=R", "r1=$FF", x, 0), AU_PASS);
    aj_want(j, "REL lfo_needs_sel1 AF0 (one sample later) fails", aj_eval(j, "REL", "lfo_needs_sel1", AJ_A ";side=L", "n=256", AJ_A ";side=R", "r1=$FF", x, 1), AU_FAIL);
    aj_want(j, "REL lfo_needs_sel1 with one row is harness", aj_eval(j, "REL", "lfo_needs_sel1", AJ_A ";side=L", "n=256", NULL, NULL, x, 0), AU_HARNESS);
    {
        struct au_cap y = *x;
        uint32_t i;

        y.frames = (uint8_t *)malloc((size_t)x->fcap * AU_F_BYTES);
        if (y.frames == NULL) {
            aj_want(j, "REL lfo_needs_sel1 of two windows of different period: out of memory", AU_HARNESS, AU_FAIL);
        } else {
            memcpy(y.frames, x->frames, (size_t)x->fcap * AU_F_BYTES);
            for (i = 1024u; i < AJ_KEYOFF; i++) {
                au_put_frame(&y, i, 1000u * aj_word((i / 2u) % 32u), 1000u * aj_word((i / 2u) % 32u));
            }
            aj_want(j, "REL lfo_needs_sel1 of two windows of different period (128 and 64 samples) fails", aj_eval(j, "REL", "lfo_needs_sel1", AJ_A ";side=L", "n=256", AJ_B ";side=L", "r1=$FF", &y, 0), AU_FAIL);
            free(y.frames);
        }
    }
    /* STRUCT: count (frame 0 of 357,630 master clocks holds ceil(357630 / 192) = 1863 samples) and header */
    aj_want(j, "STRUCT count m=357630 n=1863 passes", aj_eval(j, "STRUCT", "count", "from=0;to=357630", "m=357630;n=1863", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "STRUCT count n=1862 fails", aj_eval(j, "STRUCT", "count", "from=0;to=357630", "m=357630;n=1862", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "STRUCT count AF0 (n + 1) fails", aj_eval(j, "STRUCT", "count", "from=0;to=357630", "m=357630;n=1863", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "STRUCT count of a part frame is harness", aj_eval(j, "STRUCT", "count", "from=0;to=357631", "m=357631;n=1863", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "STRUCT header dspa passes", aj_eval(j, "STRUCT", "header", "stream=dspa", "sample_mclk=192;frame_bytes=8;version=1;format=1", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "STRUCT header dspa sample_mclk=191 fails", aj_eval(j, "STRUCT", "header", "stream=dspa", "sample_mclk=191;frame_bytes=8;version=1;format=1", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "STRUCT header dspa AF0 fails", aj_eval(j, "STRUCT", "header", "stream=dspa", "sample_mclk=192;frame_bytes=8;version=1;format=1", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "STRUCT header dspe version=1 passes", aj_eval(j, "STRUCT", "header", "stream=dspe", "version=1", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "STRUCT header dspe version=2 fails", aj_eval(j, "STRUCT", "header", "stream=dspe", "version=2", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "STRUCT header with a window is harness", aj_eval(j, "STRUCT", "header", "stream=dspa;from=0;to=10", "sample_mclk=192;frame_bytes=8;version=1;format=1", NULL, NULL, x, 0), AU_HARNESS);
    /* EV: ev_value, ev_decode, ev_edge on the two synthetic records */
    aj_want(j, "EV ev_value passes", aj_eval(j, "EV", "ev_value", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+1", "reg=R4;val=$1F", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "EV ev_value val=$1E fails", aj_eval(j, "EV", "ev_value", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+1", "reg=R4;val=$1E", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "EV ev_value AF0 fails", aj_eval(j, "EV", "ev_value", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+1", "reg=R4;val=$1F", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "EV ev_value without reg is harness", aj_eval(j, "EV", "ev_value", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+1", "val=$1F", NULL, NULL, x, 0), AU_HARNESS);
    aj_want(j, "EV ev_decode passes", aj_eval(j, "EV", "ev_decode", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+0", "chan=0;effect=$00;wave_idx=0;post=$009F", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "EV ev_decode post=$0000 fails", aj_eval(j, "EV", "ev_decode", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+0", "chan=0;effect=$00;wave_idx=0;post=$0000", NULL, NULL, x, 0), AU_FAIL);
    aj_want(j, "EV ev_edge passes", aj_eval(j, "EV", "ev_edge", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+1", "chan=0;effect=$00;wave_idx=0;post=$001F", NULL, NULL, x, 0), AU_PASS);
    aj_want(j, "EV ev_edge AF0 fails", aj_eval(j, "EV", "ev_edge", "anchor=ev:ch=0;reg=R4;val=$9F;nth=1;ev=+1", "chan=0;effect=$00;wave_idx=0;post=$001F", NULL, NULL, x, 1), AU_FAIL);
    aj_want(j, "EV an anchor that is not in the stream fails", aj_eval(j, "EV", "ev_value", "anchor=ev:ch=3;reg=R4;nth=1;ev=+0", "reg=R4;val=$1F", NULL, NULL, x, 0), AU_FAIL);
}

/* the null-synth controls of D-209 on psg_tone's own capture: with no PCM frame, with every frame 0 and with every frame one constant, every period, cycle_order and live group of the scene fails, and every silent group has a failing live partner on its side (D-208) */
static void aj_null(struct aj_ctx *j, const struct ax_file *f, const char *scene, const struct au_cap *cp)
{
    static const char *const nm[3] = {"no PCM frame", "every frame 0", "every frame 12345"};
    unsigned v;

    for (v = 0u; v < 3u; v++) {
        struct au_cap x;
        unsigned gi;
        uint32_t i;

        if (au_cap_copy(cp, &x) != 0) {
            harness_error(j->c, "out of memory for the null-synth copies");
            return;
        }
        if (v == 0u) {
            x.fcount = 0u;
        }
        for (i = 0u; i < x.fcount; i++) {
            au_put_frame(&x, i, v == 1u ? 0u : AU_AF6_LEVEL, v == 1u ? 0u : AU_AF6_LEVEL);
        }
        for (gi = 0u; gi < f->ng; gi++) {
            const struct ax_group *g = &f->gr[gi];
            char why[320];
            char name[200];

            if (strcmp(g->scene, scene) != 0 || g->form == AX_FORM_EV || g->form == AX_FORM_STRUCT) {
                continue;
            }
            if (g->kind == AK_PERIOD || g->kind == AK_CYCLE_ORDER || g->kind == AK_LIVE) {
                snprintf(name, sizeof name, "null synth (%s): the %s group %.31s fails", nm[v], au_kinds[g->kind - 1].name, g->gid);
                aj_want(j, name, au_eval_group(f, gi, &x, 0, why, sizeof why) == AU_PASS ? AU_PASS : AU_FAIL, AU_FAIL);
            } else if (g->kind == AK_SILENT) {
                int sd = 0;
                int partner_red = 0;
                unsigned r;
                unsigned g2;

                for (r = 0u; r < f->nr && sd == 0; r++) {
                    struct au_sel s;

                    if (f->rw[r].group == gi && au_sel_parse(f->rw[r].sel, &s) == NULL) {
                        sd = s.side;
                    }
                }
                for (g2 = 0u; g2 < f->ng; g2++) {
                    const struct ax_group *h = &f->gr[g2];

                    if (strcmp(h->scene, scene) != 0 || !(h->kind == AK_PERIOD || h->kind == AK_CYCLE_ORDER || h->kind == AK_LIVE)) {
                        continue;
                    }
                    for (r = 0u; r < f->nr; r++) {
                        struct au_sel s;

                        if (f->rw[r].group == g2 && au_sel_parse(f->rw[r].sel, &s) == NULL && s.side == sd && sd != 0 && au_eval_group(f, g2, &x, 0, why, sizeof why) != AU_PASS) {
                            partner_red = 1;
                        }
                    }
                }
                snprintf(name, sizeof name, "null synth (%s): the silent group %.31s has a failing live partner on its side", nm[v], g->gid);
                aj_want(j, name, partner_red ? AU_FAIL : AU_PASS, AU_FAIL);
            }
        }
        au_cap_free(&x);
    }
}

void kind_audio_judge_forms(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct au_expect e;
    struct aj_ctx j;
    struct au_cap syn;
    struct au_cap cap0;
    struct ax_file *f = NULL;
    char req[600];
    uint64_t until = 0u;
    unsigned si;

    memset(&j, 0, sizeof j);
    memset(&syn, 0, sizeof syn);
    memset(&cap0, 0, sizeof cap0);
    j.c = c;
    if (!k->has[K_EXPECT] || au_parse(k->val[K_EXPECT], &e) != 0 || e.nred != 0u || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an audio_judge_forms case needs until_mclk and expect = rom=NAME;id=N;checks=N;scene=NAME (the null-synth scene)");
        return;
    }
    j.f = (struct ax_file *)calloc(1u, sizeof *j.f);
    f = (struct ax_file *)calloc(1u, sizeof *f);
    if (j.f == NULL || f == NULL || aj_stream(&syn) != 0) {
        harness_error(c, "out of memory for the judge forms");
        goto done;
    }
    aj_forms(&j, &syn);
    aj_sweep_harness(&j, &syn);
    /* the missing-sink rule (D-209): a pcm header left at zero (no sink written) is refused, and a capture with no frame although samples were due is harness_failure, never a pass */
    {
        uint8_t zh[32];
        struct au_cap none;

        memset(zh, 0, sizeof zh);
        aj_want(&j, "a pcm header left at zero (a missing sink) is refused as harness_failure", au_pcm_header_ok(zh) ? AU_PASS : AU_HARNESS, AU_HARNESS);
        memset(&none, 0, sizeof none);
        none.smclk = 192u;
        none.machine_time = 678936u;
        aj_want(&j, "a capture with no frame after 678,936 master clocks is harness_failure", au_no_frames(&none) ? AU_HARNESS : AU_PASS, AU_HARNESS);
        none.machine_time = 0u;
        aj_want(&j, "a capture with no frame and no sample due is not refused by the missing-sink rule", au_no_frames(&none) ? AU_HARNESS : AU_PASS, AU_PASS);
    }
    {
        const char *err = ax_load(e.rom, f);

        if (err != NULL) {
            harness_error(c, err);
            goto done;
        }
    }
    for (si = 0u; si < f->ns && strcmp(f->sc[si].name, e.scene) != 0; si++) {
    }
    if (si == f->ns || f->sc[si].role != AX_ROLE_LITERAL || vd_policy_request(0u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
        harness_error(c, "the null-synth scene of audio_judge_forms must be a literal scene of the aexp rows");
        goto done;
    }
    if (au_capture(c, e.rom, req, until, &e, &cap0, 1, got, gotcap) != 0) {
        goto done;
    }
    aj_null(&j, f, e.scene, &cap0);
    {
        char det[1800];

        snprintf(det, sizeof det, "audio_judge_forms: %u checks of the ROWS, REL, STRUCT and EV forms (pass, fail, harness and the AF0 perturbation of each), the null-synth controls on %s and the missing-sink rule (E0: the judge is shown able to fail)\n%s", j.checks, e.scene,
                 j.failed[0] != '\0' ? j.failed : "every check holds\n");
        set_evidence_detail(det);
    }
    snprintf(got + strlen(got), gotcap - strlen(got), " judge_checks=%u", j.checks);
done:
    au_cap_free(&syn);
    au_cap_free(&cap0);
    free(j.f);
    free(f);
}

/* ---- audio_hash_not_verdict (plan 05-05 Task 3; ER-20, D-209): a stream digest is a change detector and never a verdict ---- */

/* psg_tone's streams with AF7 (each 32-sample cycle reversed over the window of the scene's first cycle_order group) are hashed as their own golden: the digests of the faulted streams pass that golden, while the literal groups of the scene fail on the same streams;
 * the pinned golden of the ROM sees the change, and the runtime refusal audio-hash-only holds for a golden with no literal scene and for a second golden */
void kind_audio_hash_not_verdict(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct au_expect e;
    struct ax_file *f = NULL;
    struct ax_file *g2 = NULL;
    struct au_cap cap0;
    struct au_cap x;
    char req[600];
    char own_pcm[2u * DS_SHA256_BYTES + 1u];
    char own_psg[2u * DS_SHA256_BYTES + 1u];
    char re_pcm[2u * DS_SHA256_BYTES + 1u];
    char re_psg[2u * DS_SHA256_BYTES + 1u];
    char det[1200];
    char reds[300];
    size_t rl = 0u;
    uint64_t until = 0u;
    unsigned si;
    unsigned gi;
    unsigned gold = AX_MAX;
    unsigned target = AX_MAX;
    unsigned nred = 0u;
    struct au_target t;
    const struct ax_golden *gd = NULL;

    memset(&cap0, 0, sizeof cap0);
    memset(&x, 0, sizeof x);
    reds[0] = '\0';
    if (!k->has[K_EXPECT] || au_parse(k->val[K_EXPECT], &e) != 0 || e.nred != 0u || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an audio_hash_not_verdict case needs until_mclk and expect = rom=NAME;id=N;checks=N;scene=NAME (a literal scene)");
        return;
    }
    f = (struct ax_file *)calloc(1u, sizeof *f);
    g2 = (struct ax_file *)calloc(1u, sizeof *g2);
    if (f == NULL || g2 == NULL) {
        harness_error(c, "out of memory for the aexp rows");
        goto done;
    }
    {
        const char *err = ax_load(e.rom, f);

        if (err != NULL) {
            harness_error(c, err);
            goto done;
        }
    }
    for (si = 0u; si < f->ns && strcmp(f->sc[si].name, e.scene) != 0; si++) {
    }
    for (gi = 0u; gi < f->ns; gi++) {
        gold = f->sc[gi].role == AX_ROLE_GOLDEN && gold == AX_MAX ? gi : gold;
    }
    for (gi = 0u; gi < f->ng; gi++) {
        target = target == AX_MAX && strcmp(f->gr[gi].scene, e.scene) == 0 && f->gr[gi].kind == AK_CYCLE_ORDER ? gi : target;
    }
    if (si == f->ns || f->sc[si].role != AX_ROLE_LITERAL || gold == AX_MAX || target == AX_MAX || (gd = au_golden_row(f, f->sc[gold].name)) == NULL) {
        harness_error(c, "audio_hash_not_verdict needs a literal scene with a cycle_order group and the golden scene of the same ROM");
        goto done;
    }
    if (vd_policy_request(0u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0 || au_capture(c, e.rom, req, until, &e, &cap0, 1, got, gotcap) != 0) {
        if (!c->harness) {
            harness_error(c, "the request of the case is too long");
        }
        goto done;
    }
    au_target_of(f, target, &cap0, &t);
    if (au_cap_copy(&cap0, &x) != 0 || au_digests(&cap0, re_pcm, re_psg) != 0) {
        harness_error(c, "out of memory for the faulted copy");
        goto done;
    }
    au_fault(&x, 7u, &t);
    if (au_digests(&x, own_pcm, own_psg) != 0) {
        harness_error(c, "out of memory for the faulted digests");
        goto done;
    }
    /* the faulted streams pass their own golden: hashed again, the digests equal the ones taken as the golden (a hash only says "unchanged") */
    {
        char a_pcm[2u * DS_SHA256_BYTES + 1u];
        char a_psg[2u * DS_SHA256_BYTES + 1u];

        if (au_digests(&x, a_pcm, a_psg) != 0) {
            harness_error(c, "out of memory for the faulted digests");
            goto done;
        }
        check(c, strcmp(a_pcm, own_pcm) == 0 && strcmp(a_psg, own_psg) == 0, "the AF7-faulted streams do not pass a golden taken from themselves");
    }
    check(c, strcmp(own_pcm, re_pcm) != 0, "AF7 left the DSPA stream digest unchanged: the fault did not reach the stream");
    check(c, strcmp(own_pcm, gd->pcm) != 0, "the pinned golden of the ROM does not see the AF7 change");
    /* the literal groups of the scene on the faulted streams: at least the cycle_order group must fail */
    for (gi = 0u; gi < f->ng; gi++) {
        char why[320];

        if (strcmp(f->gr[gi].scene, e.scene) != 0) {
            continue;
        }
        if (au_eval_group(f, gi, &x, 0, why, sizeof why) != AU_PASS) {
            nred++;
            if (rl + strlen(f->gr[gi].gid) + 2u < sizeof reds) {
                rl += (size_t)snprintf(reds + rl, sizeof reds - rl, "%s%s", rl > 0u ? "," : "", f->gr[gi].gid);
            }
        }
    }
    check(c, nred >= 1u, "no literal group of the scene fails on the AF7-faulted streams, so the literal case would not tell them apart");
    {
        char why[320];

        check(c, au_eval_group(f, target, &x, 0, why, sizeof why) != AU_PASS, "the cycle_order group passes on the AF7-faulted streams");
    }
    /* the runtime refusal audio-hash-only: the golden scene with the literal scenes taken away, and a second golden scene */
    memcpy(g2, f, sizeof *g2);
    for (gi = 0u; gi < g2->ns; gi++) {
        g2->sc[gi].role = g2->sc[gi].role == AX_ROLE_LITERAL ? AX_ROLE_EXPECT_RED : g2->sc[gi].role;
    }
    {
        const char *why = au_golden_refusal(g2, gold);

        check(c, why != NULL && strncmp(why, "audio-hash-only", 15u) == 0, "a golden scene with no literal scene of its ROM was not refused as audio-hash-only (ER-20)");
    }
    memcpy(g2, f, sizeof *g2);
    if (g2->ns < AX_MAX) {
        g2->sc[g2->ns] = g2->sc[gold];
        memcpy(g2->sc[g2->ns].name, "second_golden", 14u);
        g2->ns++;
    }
    {
        const char *why = au_golden_refusal(g2, gold);

        check(c, why != NULL && strncmp(why, "audio-hash-only", 15u) == 0, "a second golden scene of one ROM was not refused as audio-hash-only (D-209)");
    }
    check(c, au_golden_refusal(f, gold) == NULL, "the golden scene of the ROM, beside its literal scene and that scene's sweep case, is refused");
    snprintf(det, sizeof det, "audio_hash_not_verdict: AF7 over the window of %s; the faulted streams hash to pcm_sha256=%s psg_sha256=%s E0 repeatability and pass a golden taken from themselves,\nwhile %u literal group(s) of %s fail on them (%s); the pinned golden pcm_sha256=%s sees the change;"
             " audio-hash-only refuses a golden without a literal scene and a second golden (ER-20)\n", f->gr[target].gid, own_pcm, own_psg, nred, e.scene, reds, gd->pcm);
    set_evidence_detail(det);
    snprintf(got + strlen(got), gotcap - strlen(got), " red=%u", nred);
done:
    au_cap_free(&cap0);
    au_cap_free(&x);
    free(f);
    free(g2);
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

/* the ROMs of an expect of one to four items rom=NAME (aud_noninterference, plan 05-07: psg_tone and psg_mod); the count, or 0 when the expect is malformed */
#define AU_ROMS_MAX 4u

static unsigned au_rom_list(const struct kase *k, char roms[AU_ROMS_MAX][48])
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned n = 0u;

    if (!k->has[K_EXPECT] || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        return 0u;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        const char *v = ax_val(item, "rom");

        if (v == NULL || n >= AU_ROMS_MAX || strlen(v) == 0u || strlen(v) >= 48u || strspn(v, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(v)) {
            return 0u;
        }
        memcpy(roms[n++], v, strlen(v) + 1u);
    }
    return n;
}

/* one ROM of aud_noninterference: run to its DSV1 verdict or until with the psg, pcm and dev sinks on and with no sink, under zero and seeded-prng-v4 seeds 1 and 2 */
static void au_nonint_rom(const struct kase *k, struct ctx *c, const char *rom, uint64_t until, int note, uint64_t *frames, uint64_t *events)
{
    unsigned i;

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
        char msg[240];

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
            snprintf(msg, sizeof msg, "%.40s under %s: the psg, pcm and dev sinks changed the stop reason, machine time, steps, verdict RAM or state digest of the run", rom, vd_policy_name(i));
            check(c, au_end_same(&e0, &e1), msg);
            snprintf(msg, sizeof msg, "%.40s under %s: the sink run produced no DSPE record or no DSPA frame, so the comparison would be vacuous", rom, vd_policy_name(i));
            check(c, cp.ecount > 0u && cp.fcount > 0u, msg);
            *frames += cp.fcount;
            *events += cp.ecount;
            if (i == 0u && note) {
                note_run(c, r1.machine_time, r1.reason);
            }
        }
        au_cap_free(&cp);
        release_boot(&b0);
        release_boot(&b1);
    }
}

/* aud_noninterference (D-200): each ROM of the case (psg_tone; psg_mod from plan 05-07) run to its DSV1 verdict or until_mclk with the psg, pcm and dev sinks on and with no sink, under zero and seeded-prng-v4 seeds 1 and 2: identical stop reason, machine time, steps, verdict RAM
 * and state digest; the sink runs must have produced records and frames, so the comparison is not vacuous. The psg and pcm pointers are read only at the gate and the emits; the toy psg-emit-mutates turns this case red. */
void kind_aud_noninterference(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char roms[AU_ROMS_MAX][48];
    unsigned nroms = au_rom_list(k, roms);
    uint64_t until = 0u;
    uint64_t frames = 0u;
    uint64_t events = 0u;
    unsigned ri;

    if (nroms == 0u || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u) {
        harness_error(c, "an aud_noninterference case needs until_mclk and expect = rom=NAME[;rom=NAME...] (one to four ROMs)");
        return;
    }
    for (ri = 0u; ri < nroms && !c->harness; ri++) {
        au_nonint_rom(k, c, roms[ri], until, ri == 0u, &frames, &events);
    }
    snprintf(got, gotcap, "roms=%u policies=%u frames=%llu events=%llu", nroms, (unsigned)VD_POLICIES, (unsigned long long)frames, (unsigned long long)events);
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

/* the stamp of the n-th DSPE record with the effect DDA_LOAD (an R6 write with DDA 1), UINT64_MAX when there is none */
static uint64_t au_dda_stamp(const struct au_cap *cp, unsigned n)
{
    uint32_t i;
    unsigned seen = 0u;

    for (i = 0u; i < cp->ecount; i++) {
        const uint8_t *r = cp->erec + (size_t)i * AU_E_REC;

        if ((r[AU_E_EFFECT] & 0x02u) != 0u && ++seen == n) {
            return au_u64(r + AU_E_STAMP);
        }
    }
    return UINT64_MAX;
}

/* the stamps of the LFO TRG stops of psg_lfo (plan 05.1-02), all read from the DSPE records of R9 (the global channel) and R6 of the reference run: *t1 the last R9 record with TRG 1 before the first R9 record with TRG 0 (*t0); *up the 10th R6 record after the first R9
 * record with TRG 1 after *t0 (inside the select-1 upload of lfo_dynamic); *rel the first R9 record with TRG 0 and CTL != 0 after it (the release that starts the coupled select-1 steps). 0 when all four exist */
static int au_lfo_stamps(const struct au_cap *cp, uint64_t *t1, uint64_t *t0, uint64_t *up, uint64_t *rel)
{
    uint32_t i;
    unsigned stage = 0u;
    unsigned n6 = 0u;

    *t1 = UINT64_MAX;
    for (i = 0u; i < cp->ecount && stage < 4u; i++) {
        const uint8_t *r = cp->erec + (size_t)i * AU_E_REC;
        uint64_t t = au_u64(r + AU_E_STAMP);
        int trg = (r[AU_E_VALUE] & 0x80u) != 0u;

        if (r[AU_E_REG] == 9u && stage == 0u && trg) {
            *t1 = t;
        } else if (r[AU_E_REG] == 9u && stage == 0u && !trg && *t1 != UINT64_MAX) {
            *t0 = t;
            stage = 1u;
        } else if (r[AU_E_REG] == 9u && stage == 1u && trg) {
            stage = 2u;
        } else if (r[AU_E_REG] == 6u && stage == 2u && ++n6 == 10u) {
            *up = t;
            stage = 3u;
        } else if (r[AU_E_REG] == 9u && stage == 3u && !trg && (r[AU_E_VALUE] & 0x03u) != 0u) {
            *rel = t;
            stage = 4u;
        }
    }
    return stage == 4u ? 0 : -1;
}

#define AU_LFO_STEP 115200u /* the coupled select-1 step of lfo_dynamic: 6 x F1 x FLF = 6 x 128 x 150 (MAN:P8-6+P8-9+H8-23, the step literal of the dyn row of psg_lfo.aexp) */

/* the restore stops of one ROM. Set 0 (psg_tone): at a sample instant (a step boundary on the 192 grid), mid-interval (a boundary 64 to 128 clocks past one), between the R2 and R3 writes, between the R0 and R4 writes of the scene, and inside the TIN to $0806 (after its 10th
 * byte). Set 1 (psg_mod, plan 05-07): between two timer-paced DDA writes (after the 5th DDA_LOAD record, timer write 4, before the 6th) and inside a noise run of mod_noise4 (a mid-interval boundary 100,000 clocks after select 4's key-on, the first R4 $9F, with select 4 keyed on
 * at NE 1). Set 2 (psg_lfo, plan 05.1-02; D-200, D-231, C-09, C-11): trg halfway between an R9 write with TRG 1 and the next with TRG 0 (lfo_release: select 1 keyed on and frozen under TRG 1); upload after the 10th R6 store of lfo_dynamic's select-1 upload (inside the TIN,
 * TRG 1 with widx[1] not 0, accepted, D-231); coupled 1,000 clocks after the first coupled select-1 step of lfo_dynamic (between the first and the second), where rem[1] must exceed d x 4096 (the C-09 bound exercised; harness_failure otherwise). Every stop is placed by DSPE
 * records, never by an absolute time. The prefix run stops there, a snapshot is restored into a poisoned second boot, and the resumed run's DSPE records and DSPA frames appended to the prefix must equal the whole run's streams byte for byte, with first(i+1) = first(i) + seq(i),
 * the same end machine time and the same end digest. The count of restores is added to *done, the reference run's instruction count goes to *insn_n (the run must stay within AU_INSN_CAP) and one line per restore (the stop, its machine time, rem[1]) is appended to det */
static void au_restore_rom(struct ctx *c, const char *rom, const char *req, uint64_t until, int set, int note, unsigned *done, uint32_t *frames, uint32_t *events, uint32_t *insn_n, char *det, size_t detcap)
{
    static const char *const names[3][5] = {{"sample", "mid", "r2r3", "r0r4", "tin"}, {"dda", "noise", "", "", ""}, {"trg", "upload", "coupled", "", ""}};
    unsigned nstops = set == 0 ? 5u : set == 1 ? 2u : 3u;
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
    uint64_t k4 = UINT64_MAX;
    uint64_t t1 = 0u;
    uint64_t t0 = 0u;
    uint64_t up = 0u;
    uint64_t rel = 0u;
    unsigned i;
    const char *err;

    memset(&ref, 0, sizeof ref);
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
    if (note) {
        note_run(c, res.machine_time, res.reason);
    }
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
    *insn_n = ninsn > *insn_n ? ninsn : *insn_n;
    if (au_u32(isk.header + 4) != 0u) {
        harness_error(c, "the instruction sink of the restore kind lost a record");
        goto out;
    }
    /* the stop deadlines: a run stops at the first step end at or after its deadline, so a deadline one clock past a write stamp stops after that write's step */
    if (set == 0) {
        at[0] = au_step_at(insn, ninsn, until / 2u, 0u, 0u);
        at[1] = au_step_at(insn, ninsn, until / 2u, 64u, 128u);
        at[2] = au_stamp_of(&ref, 2u, 256u, 1u);
        at[3] = au_stamp_of(&ref, 0u, 0u, 2u);
        at[4] = au_stamp_of(&ref, 6u, 256u, 10u);
        for (i = 2u; i < 5u; i++) {
            at[i] = at[i] != UINT64_MAX ? at[i] + 1u : UINT64_MAX;
        }
    } else if (set == 1) {
        at[0] = au_dda_stamp(&ref, 5u);
        at[0] = at[0] != UINT64_MAX ? at[0] + 1u : UINT64_MAX;
        k4 = au_stamp_of(&ref, 4u, 0x9Fu, 1u);
        at[1] = k4 != UINT64_MAX ? au_step_at(insn, ninsn, k4 + 100000u, 64u, 128u) : UINT64_MAX;
    } else if (au_lfo_stamps(&ref, &t1, &t0, &up, &rel) == 0) {
        at[0] = t1 + (t0 - t1) / 2u;
        at[1] = up + 1u;
        at[2] = rel + AU_LFO_STEP + 1000u;
    } else {
        at[0] = UINT64_MAX;
    }
    for (i = 0u; i < nstops && !c->harness; i++) {
        struct booted a;
        struct booted t;
        const struct ds_instance *ca = NULL;
        const struct ds_instance *ct = NULL;
        struct au_cap cp;
        struct au_end e;
        int placed;
        char msg[400];

        if (at[i] == UINT64_MAX || at[i] >= until) {
            snprintf(msg, sizeof msg, "the restore stop %s of %.40s has no deadline inside the run (no step boundary or write found)", names[set][i], rom);
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

            if (set == 0) {
                placed = i == 0u ? m % 192u == 0u : i == 1u ? m % 192u != 0u : i == 2u ? m > at[2] - 1u && m <= au_stamp_of(&ref, 3u, 256u, 1u) : i == 3u ? m > at[3] - 1u && m <= au_stamp_of(&ref, 4u, 0x40u, 1u) : ca->blk_active != 0u;
            } else if (set == 1) {
                placed = i == 0u ? m > at[0] - 1u && m <= au_dda_stamp(&ref, 6u) : m % 192u != 0u && m > k4 && m < k4 + 312096u && (ca->psg_ctl[4] & 0x80u) != 0u && (ca->psg_noise[0] & 0x80u) != 0u;
            } else if (i == 0u) { /* trg: under TRG 1 with select 1 keyed on and frozen, before the TRG 0 write */
                placed = m > t1 && m <= t0 && (ca->psg_lfo_ctl & 0x80u) != 0u && (ca->psg_ctl[1] & 0x80u) != 0u && ca->psg_rem[1] == 0u;
            } else if (i == 1u) { /* upload: inside the TIN at select 1, TRG 1, chON 0, widx[1] not 0 (D-231) */
                placed = ca->blk_active != 0u && ca->psg_select == 1u && (ca->psg_lfo_ctl & 0x80u) != 0u && (ca->psg_ctl[1] & 0x80u) == 0u && ca->psg_widx[1] != 0u;
            } else { /* coupled: TRG 0, CTL != 0, one select-1 step after the release, and the C-09 distance */
                uint32_t d = ds_psg_divisor(ca);

                if (ca->psg_rem[1] <= d * 4096u) {
                    snprintf(msg, sizeof msg, "the coupled restore stop of %.40s has the select-1 distance %u, not above d x 4096 = %u: the C-09 bound is not exercised", rom, (unsigned)ca->psg_rem[1], (unsigned)(d * 4096u));
                    harness_error(c, msg);
                    au_cap_free(&cp);
                    release_boot(&a);
                    release_boot(&t);
                    break;
                }
                placed = m > rel + AU_LFO_STEP && (ca->psg_lfo_ctl & 0x80u) == 0u && (ca->psg_lfo_ctl & 0x03u) != 0u && ca->psg_widx[1] == 1u;
            }
            snprintf(msg, sizeof msg, "the restore stop %s of %.40s landed at machine time %llu, not where it was chosen (a sample instant, mid-interval, between the two writes, inside the TIN, between two DDA writes, inside a noise run, under TRG 1, inside the upload, between two coupled steps)", names[set][i], rom, (unsigned long long)m);
            check(c, placed, msg);
            if (ds_snapshot(ca, snap, ds_snapshot_size(), NULL) != (uint32_t)DS_SNAPREF_NONE) {
                check(c, 0, "ds_snapshot at an audio restore stop was refused");
            } else {
                snap_poison((struct ds_instance *)(void *)t.mem, (i & 1u) != 0u ? 0x5Au : 0xA5u);
                if (ds_restore((struct ds_instance *)(void *)t.mem, snap, ds_snapshot_size()) != (uint32_t)DS_SNAPREF_NONE) {
                    check(c, 0, "ds_restore refused the snapshot of an audio restore stop");
                } else if (au_run(c, (struct ds_instance *)(void *)t.mem, until, 0, 0u, &cp, &res) == 0) {
                    au_end_of(&e, ct, &res);
                    snprintf(msg, sizeof msg, "restore of %.40s at %s (machine time %llu): the prefix and the resumed streams do not equal the whole run byte for byte, or first(i+1) != first(i) + seq(i)", rom, names[set][i], (unsigned long long)m);
                    check(c, au_streams_differ(&ref, &cp) == 0 && !cp.chain_bad && cp.calls == 2u, msg);
                    snprintf(msg, sizeof msg, "restore of %.40s at %s: the resumed run ends at another machine time or digest than the whole run", rom, names[set][i]);
                    check(c, e.mclk == eref.mclk && e.digest_ok && eref.digest_ok && memcmp(e.digest, eref.digest, DS_DIGEST_BYTES) == 0, msg);
                    (*done)++;
                    if (strlen(det) + 120u < detcap) {
                        snprintf(det + strlen(det), detcap - strlen(det), "restore %s of %.40s at machine time %llu, rem[1] %u, widx[1] %u\n", names[set][i], rom, (unsigned long long)m, (unsigned)ca->psg_rem[1], (unsigned)ca->psg_widx[1]);
                    }
                }
            }
        }
        au_cap_free(&cp);
        release_boot(&a);
        release_boot(&t);
    }
    *frames += ref.fcount;
    *events += ref.ecount;
out:
    au_cap_free(&ref);
    free(insn);
    free(snap);
}

/* snapshot_restore_audio (D-200): expect = rom=NAME;stops=SET[;rom=NAME;stops=SET] with SET one of sample,mid,r2r3,r0r4,tin (psg_tone), dda,noise (psg_mod) and trg,upload,coupled (psg_lfo, plan 05.1-02): each ROM with its stop set (au_restore_rom), in that order */
void kind_snapshot_restore_audio(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const sets[3] = {"sample,mid,r2r3,r0r4,tin", "dda,noise", "trg,upload,coupled"};
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    char roms[2][48];
    int set[2] = {-1, -1};
    unsigned n = 0u;
    unsigned want_rom = 1u;
    uint64_t until = 0u;
    unsigned done = 0u;
    uint32_t frames = 0u;
    uint32_t events = 0u;
    uint32_t insn_n = 0u;
    char det[1200];
    unsigned i;
    int ok = k->has[K_EXPECT] && pk_copy_text(k->val[K_EXPECT], list, sizeof list) == 0;

    det[0] = '\0';

    while (ok && (item = pk_next_item(&rest, ';')) != NULL) {
        const char *v;

        if (want_rom && n < 2u && (v = ax_val(item, "rom")) != NULL && strlen(v) > 0u && strlen(v) < 48u && strspn(v, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(v)) {
            memcpy(roms[n], v, strlen(v) + 1u);
            want_rom = 0u;
        } else if (!want_rom && (v = ax_val(item, "stops")) != NULL && (strcmp(v, sets[0]) == 0 || strcmp(v, sets[1]) == 0 || strcmp(v, sets[2]) == 0)) {
            set[n] = strcmp(v, sets[0]) == 0 ? 0 : strcmp(v, sets[1]) == 0 ? 1 : 2;
            n++;
            want_rom = 1u;
        } else {
            ok = 0;
        }
    }
    if (!ok || n == 0u || !want_rom || (n == 2u && set[0] == set[1]) || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 400000u) {
        harness_error(c, "a snapshot_restore_audio case needs until_mclk (at least 400,000) and expect = rom=NAME;stops=SET[;rom=NAME;stops=SET] (SET sample,mid,r2r3,r0r4,tin or dda,noise or trg,upload,coupled)");
        return;
    }
    for (i = 0u; i < n && !c->harness; i++) {
        au_restore_rom(c, roms[i], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", until, set[i], i == 0u, &done, &frames, &events, &insn_n, det, sizeof det);
    }
    set_evidence_detail(det);
    snprintf(got, gotcap, "roms=%u restores=%u frames=%u events=%u insn=%u", n, done, (unsigned)frames, (unsigned)events, (unsigned)insn_n);
}

/* ---- audio_noise_repeat and psg_block_flood (plan 05-07 Task 3; D-200, D-201, D-210, D-212, criterion 4) ---- */

#define AU_REPEAT_CHUNK 16384u

/* audio_noise_repeat (D-212, criterion 4; E0 repeatability, never accuracy): for each of zero, seeded-prng-v4 seeds 1 and 2, two fresh instances boot the ROM and run interleaved in calls to the same deadlines (every 16,384 master clocks, A then B) to until_mclk with the psg, pcm
 * and dev sinks; their DSPE and DSPA streams must be byte-identical (memcmp, the comparison of au_streams_differ). The in-process control flips one byte of a copy of B's PCM and then one byte of its DSPE record, and the same comparison must report each difference. */
void kind_audio_noise_repeat(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    char detail[1600];
    size_t used = 0u;
    uint64_t until = 0u;
    unsigned i;
    unsigned same = 0u;

    detail[0] = '\0';
    if (au_rom_expect(k, rom, sizeof rom, NULL, NULL, 0u) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 100000u) {
        harness_error(c, "an audio_noise_repeat case needs until_mclk (at least 100,000) and expect = rom=NAME");
        return;
    }
    for (i = 0u; i < VD_POLICIES && !c->harness; i++) {
        char req[600];
        char line[400];
        char pcm_hex[2u * DS_SHA256_BYTES + 1u];
        char psg_hex[2u * DS_SHA256_BYTES + 1u];
        struct booted ba;
        struct booted bb;
        const struct ds_instance *ca = NULL;
        const struct ds_instance *cb = NULL;
        struct au_cap xa;
        struct au_cap xb;
        struct au_cap x2;
        struct ds_run_result ra;
        struct ds_run_result rb;
        uint64_t pcap;
        uint64_t t;
        const char *err;
        int ok = 1;
        int d;
        char msg[240];

        memset(&xa, 0, sizeof xa);
        memset(&xb, 0, sizeof xb);
        if (vd_policy_request(i, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
            harness_error(c, "the request of the audio_noise_repeat case is too long");
            return;
        }
        err = boot_built_rom(rom, req, &ba, &ca);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        err = boot_built_rom(rom, req, &bb, &cb);
        if (err != NULL) {
            release_boot(&ba);
            harness_error(c, err);
            return;
        }
        pcap = au_pcm_capacity(ca->machine_time, until);
        for (t = AU_REPEAT_CHUNK; ok; t += AU_REPEAT_CHUNK) {
            uint64_t dl = t < until ? t : until;

            ok = au_run(c, (struct ds_instance *)(void *)ba.mem, dl, 0, pcap, &xa, &ra) == 0 && au_run(c, (struct ds_instance *)(void *)bb.mem, dl, 0, pcap, &xb, &rb) == 0;
            if (dl == until) {
                break;
            }
        }
        if (ok) {
            if (i == 0u) {
                note_run(c, ra.machine_time, ra.reason);
            }
            d = au_streams_differ(&xa, &xb);
            snprintf(msg, sizeof msg, "under %s the two interleaved instances made different %s streams: the noise output does not repeat", vd_policy_name(i), d == 1 ? "DSPA" : "DSPE");
            check(c, d == 0, msg);
            snprintf(msg, sizeof msg, "under %s a run produced no DSPE record or no DSPA frame, or the two runs end apart: the comparison would be vacuous", vd_policy_name(i));
            check(c, xa.ecount > 0u && xa.fcount > 0u && ra.machine_time == rb.machine_time && !xa.chain_bad && !xb.chain_bad, msg);
            if (d == 0 && au_cap_copy(&xb, &x2) == 0) {
                x2.frames[(size_t)(x2.fcount / 2u) * AU_F_BYTES] ^= 0x01u;
                check(c, au_streams_differ(&xa, &x2) == 1, "the control: one flipped PCM byte is not reported as a difference, so the comparison could not fail");
                x2.frames[(size_t)(x2.fcount / 2u) * AU_F_BYTES] ^= 0x01u;
                x2.erec[(size_t)(x2.ecount / 2u) * AU_E_REC + AU_E_VALUE] ^= 0x01u;
                check(c, au_streams_differ(&xa, &x2) == 2, "the control: one flipped DSPE byte is not reported as a difference, so the comparison could not fail");
                au_cap_free(&x2);
            } else if (d == 0) {
                harness_error(c, "out of memory for the control copy of the noise repeat streams");
            }
            if (d == 0 && au_digests(&xa, pcm_hex, psg_hex) == 0) {
                same++;
                snprintf(line, sizeof line, "%s: two interleaved instances, %u frames and %u events each, identical streams pcm_sha256=%s psg_sha256=%s (E0 repeatability)\n", vd_policy_name(i), (unsigned)xa.fcount, (unsigned)xa.ecount, pcm_hex, psg_hex);
                if (used + strlen(line) + 1u < sizeof detail) {
                    memcpy(detail + used, line, strlen(line) + 1u);
                    used += strlen(line);
                }
            }
        }
        au_cap_free(&xa);
        au_cap_free(&xb);
        release_boot(&ba);
        release_boot(&bb);
    }
    if (!c->harness) {
        set_evidence_detail(detail);
    }
    snprintf(got, gotcap, "E0 repeatability policies=%u identical=%u", (unsigned)VD_POLICIES, same);
}

/* the flood program (D-212, F-81): MPR0 = $FF, then one TIN of 65,536 bytes (a length of 0) from $E000 to $0806 (R6): every block byte is a window-2 write, 65,536 in all */
#define AU_FLOOD_WRITES 65536u
#define AU_FLOOD_CAP 1024u

static void au_img_flood(unsigned char *img)
{
    size_t at;

    au_img_head(img, &at);
    img[at++] = 0xD3u; /* TIN $E000, $0806, $0000 */
    img[at++] = 0x00u;
    img[at++] = 0xE0u;
    img[at++] = 0x06u;
    img[at++] = 0x08u;
    img[at++] = 0x00u;
    img[at++] = 0x00u;
    au_img_tail(img, at);
}

/* psg_block_flood (D-200, D-201, D-212, T-05-25): the flood program run once with no sink and once with a psg sink of 1,024 records and a pcm sink of the D-201 closed-form capacity. The psg sink keeps the first 1,024 records and counts the rest as lost (drop-newest of whole
 * records): count 1,024, lost 65,536 - 1,024, seq = count + lost; the pcm sink loses nothing; and the stop reason, machine time, steps, verdict RAM and state digest are those of the run with no sink */
void kind_psg_block_flood(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    unsigned char *img;
    uint64_t until = 0u;
    struct booted b0;
    struct booted b1;
    const struct ds_instance *c0 = NULL;
    const struct ds_instance *c1 = NULL;
    struct ds_run_request rq;
    struct ds_run_result r0;
    struct ds_run_result r1;
    struct ds_psg_sink psk;
    struct ds_pcm_sink fsk;
    struct au_end e0;
    struct au_end e1;
    uint8_t *recs = NULL;
    uint8_t *frames = NULL;
    uint64_t pcap;
    const char *err;
    char msg[300];

    if (!k->has[K_EXPECT] || strcmp(k->val[K_EXPECT], "program=tin-65536-r6;psg_cap=1024") != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 4800000u) {
        harness_error(c, "a psg_block_flood case needs until_mclk (at least 4,800,000, past the 65,536-byte TIN) and expect = program=tin-65536-r6;psg_cap=1024");
        return;
    }
    img = (unsigned char *)malloc(RP_IMAGE_BYTES);
    if (img == NULL) {
        harness_error(c, "out of memory for the flood program");
        return;
    }
    au_img_flood(img);
    err = boot_random(img, "", &b0, &c0);
    if (err == NULL) {
        err = boot_random(img, "", &b1, &c1);
        if (err != NULL) {
            release_boot(&b0);
        }
    }
    free(img);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    pcap = au_pcm_capacity(c1->machine_time, until);
    recs = (uint8_t *)calloc((size_t)AU_FLOOD_CAP, (size_t)AU_E_REC);
    frames = (uint8_t *)calloc((size_t)pcap, (size_t)AU_F_BYTES);
    if (recs == NULL || frames == NULL) {
        harness_error(c, "out of memory for the flood sinks");
        goto out;
    }
    memset(&rq, 0, sizeof rq);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    (void)ds_run((struct ds_instance *)(void *)b0.mem, &rq, &r0);
    au_end_of(&e0, c0, &r0);
    memset(&psk, 0, sizeof psk);
    memset(&fsk, 0, sizeof fsk);
    psk.records = recs;
    psk.capacity = AU_FLOOD_CAP;
    fsk.frames = frames;
    fsk.capacity = (uint32_t)pcap;
    rq.psg = &psk;
    rq.pcm = &fsk;
    if (ds_run((struct ds_instance *)(void *)b1.mem, &rq, &r1) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of the flood run with sinks");
        goto out;
    }
    au_end_of(&e1, c1, &r1);
    note_run(c, r1.machine_time, r1.reason);
    check(c, au_end_same(&e0, &e1), "the psg and pcm sinks changed the stop reason, machine time, steps, verdict RAM or state digest of the flood run (D-200)");
    snprintf(msg, sizeof msg, "the psg sink reads count %u, lost %u, seq %llu: the 65,536 window-2 writes of the TIN must give count 1,024, lost 64,512 and seq = count + lost (drop-newest of whole records)", (unsigned)au_u32(psk.header + AU_H_COUNT), (unsigned)au_u32(psk.header + AU_H_LOST),
             (unsigned long long)au_u64(psk.header + AU_H_SEQ));
    check(c, au_u32(psk.header + AU_H_COUNT) == AU_FLOOD_CAP && au_u32(psk.header + AU_H_LOST) == AU_FLOOD_WRITES - AU_FLOOD_CAP && au_u64(psk.header + AU_H_SEQ) == (uint64_t)AU_FLOOD_WRITES, msg);
    snprintf(msg, sizeof msg, "the pcm sink of the D-201 capacity lost %u frames or its seq %llu is not ceil(T1 / 192) - ceil(T0 / 192)", (unsigned)au_u32(fsk.header + AU_H_LOST), (unsigned long long)au_u64(fsk.header + AU_H_SEQ));
    check(c, au_u32(fsk.header + AU_H_LOST) == 0u && au_u64(fsk.header + AU_H_SEQ) == (uint64_t)au_u32(fsk.header + AU_H_COUNT) && au_u64(fsk.header + AU_H_SEQ) == (r1.machine_time + 191u) / 192u - au_u64(fsk.header + AU_H_FIRST), msg);
    check(c, r1.reason == (uint32_t)DS_RUN_REASON_UNTIL, "the flood run did not stop at its deadline");
    snprintf(got, gotcap, "writes=%u count=%u lost=%u frames=%u", (unsigned)AU_FLOOD_WRITES, (unsigned)au_u32(psk.header + AU_H_COUNT), (unsigned)au_u32(psk.header + AU_H_LOST), (unsigned)au_u32(fsk.header + AU_H_COUNT));
out:
    free(recs);
    free(frames);
    release_boot(&b0);
    release_boot(&b1);
}

/* ---- --audio-out and kind audio_out_rights (plan 05-04 Task 3; D-199, F-85, T-05-16; the --video-out shape) ----
 * The runner writes the audio streams of a case only for `--audio-out CASE_ID`, only for a case of kind audio whose fixture is an authored image listed in build/gate/fixtures-authored.used or a synthetic recipe, only to build/gate/audio/ID.dspe and ID.dspa (the canonical streams of D-199)
 * and ID.tsv, paths the runner builds itself from an id of the form [a-z0-9_]+; stdout is exactly three lines of ids, counts, sizes and SHA-256 values, never sample data. The run is the zero-policy run of the case (the same image, request, deadline and DSV1 stop), so the streams are the
 * streams the case judged, and scripts/conformance-run recomputes both digests with a host tool against the pcm_sha256 and psg_sha256 of the case's evidence file. The .tsv holds one header row, then E rows (one per DSPE record: stamp, pc, reg, value, chan, effect, wave_idx, post),
 * F rows (one per video frame of lines x 1365 master clocks with any sample in the stream: the frame, its first stream index and its sample count) and P rows (run-length groups of identical frames: first index, count, L, R). Evidence tier E0. */

#define AO_DIR "build/gate/audio/"
#define AO_ID_MAX 64u

const char *audio_out_id;

int audiokinds_arg(int argc, char **argv, int *a)
{
    if (strcmp(argv[*a], "--audio-out") == 0 && *a + 1 < argc) {
        audio_out_id = argv[*a + 1];
        *a += 1;
        return 1;
    }
    return 0;
}

enum { AO_OK = 0, AO_BAD_ID = 1, AO_NO_CASE = 2, AO_NOT_PERMITTED = 3, AO_NOT_AUDIO = 4, AO_NO_DEADLINE = 5, AO_FAILED = 6 };

static const char *ao_reason(int r)
{
    switch (r) {
    case AO_BAD_ID:
        return "the id is not of the form [a-z0-9_]+ with at most 64 characters";
    case AO_NO_CASE:
        return "no case carries that id";
    case AO_NOT_PERMITTED:
        return "the fixture of the case is neither an authored image listed in build/gate/fixtures-authored.used nor a synthetic recipe, or the expect's rom= is not the authored image that fixture key names";
    case AO_NOT_AUDIO:
        return "the case is not an audio case";
    case AO_NO_DEADLINE:
        return "the case has no until_mclk to run to";
    default:
        return "the audio streams could not be produced";
    }
}

static int ao_id_valid(const char *id)
{
    size_t n = strlen(id);

    return n >= 1u && n <= AO_ID_MAX && strspn(id, "abcdefghijklmnopqrstuvwxyz0123456789_") == n;
}

/* the three paths the runner derives from a valid id: 0 on success */
static int ao_paths(const char *id, char *e, char *p, char *t, size_t cap)
{
    int a;
    int b;
    int d;

    if (!ao_id_valid(id)) {
        return -1;
    }
    a = snprintf(e, cap, AO_DIR "%s.dspe", id);
    b = snprintf(p, cap, AO_DIR "%s.dspa", id);
    d = snprintf(t, cap, AO_DIR "%s.tsv", id);
    return a > 0 && b > 0 && d > 0 && (size_t)a < cap && (size_t)b < cap && (size_t)d < cap ? 0 : -1;
}

/* the case of an id, NULL when none */
static const struct kase *ao_find_case(const char *id)
{
    size_t i;

    for (i = 0u; i < case_count(); i++) {
        if (strcmp(case_get(i)->val[K_ID], id) == 0) {
            return case_get(i);
        }
    }
    return NULL;
}

/*
 * The checks of one case before any file is opened: its fixture (authored and listed, or synthetic), its kind, its deadline, and that its expect's rom= names the image of that fixture key. The image that runs and whose streams are written is
 * build/gate/fixtures/ROM.pce of rom=, so the permission must cover that image: rom= must be the authored image the fixture key names, and listed (review WR-04). An expect with no parsable rom= ties the permission to no image and is refused.
 */
static int ao_case_check(const struct kase *k, uint64_t *until)
{
    struct au_expect e;
    char want[64];
    int n;

    if (!k->has[K_FIXTURE] || !output_fixture_permitted(k->val[K_FIXTURE])) {
        return AO_NOT_PERMITTED;
    }
    if (!k->has[K_KIND] || strcmp(k->val[K_KIND], "audio") != 0) {
        return AO_NOT_AUDIO;
    }
    if (!k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], until) != 0 || *until == 0u) {
        return AO_NO_DEADLINE;
    }
    if (!k->has[K_EXPECT] || au_parse(k->val[K_EXPECT], &e) != 0) {
        return AO_NOT_PERMITTED;
    }
    n = snprintf(want, sizeof want, "authored:%s", e.rom);
    if (n < 0 || (size_t)n >= sizeof want || strcmp(want, k->val[K_FIXTURE]) != 0 || !output_fixture_permitted(want)) {
        return AO_NOT_PERMITTED;
    }
    return AO_OK;
}

/* the checks before any file is opened: the id, the case, then ao_case_check */
static int ao_request(const char *id, const struct kase **kout, uint64_t *until)
{
    const struct kase *k;
    int r;

    if (!ao_id_valid(id)) {
        return AO_BAD_ID;
    }
    k = ao_find_case(id);
    if (k == NULL) {
        return AO_NO_CASE;
    }
    r = ao_case_check(k, until);
    if (r == AO_OK) {
        *kout = k;
    }
    return r;
}

/* the frame length in master clocks of the profile binding of the run: lines_per_frame (262 or 263) x 1365 (ASSUMED LED-VID-0001, LED-VID-0004) */
static uint64_t ao_frame_mclk(const struct ds_instance *inst)
{
    return (strcmp(inst->profile.field[DS_FIELD_lines_per_frame].value, "263") == 0 ? 263u : 262u) * 1365u;
}

/* the sample count of video frame k of length lf over the stream indices [0, n) from first: the samples s with k x lf <= s x 192 < (k + 1) x lf, clipped to the stream (D-187) */
static uint64_t ao_frame_count(uint64_t first, uint64_t n, uint64_t lf, uint64_t k, uint64_t *start)
{
    uint64_t a = (k * lf) / 192u + ((k * lf) % 192u != 0u ? 1u : 0u);
    uint64_t b = ((k + 1u) * lf) / 192u + (((k + 1u) * lf) % 192u != 0u ? 1u : 0u);

    a = a < first ? first : a;
    b = b > first + n ? first + n : b;
    *start = a - first;
    return b > a ? b - a : 0u;
}

static int ao_write_tsv(const char *path, const struct au_cap *cp, uint64_t lf, uint64_t *rows)
{
    FILE *fp = fopen(path, "wb");
    uint32_t i;
    uint64_t k;
    int ok = fp != NULL;

    *rows = 0u;
    if (fp == NULL) {
        return -1;
    }
    ok = ok && fprintf(fp, "row\tstamp_or_frame\tpc_or_first\treg_or_count\tvalue_or_l\tchan_or_r\teffect\twave_idx\tpost\n") > 0;
    for (i = 0u; i < cp->ecount && ok; i++) {
        const uint8_t *r = cp->erec + (size_t)i * AU_E_REC;

        ok = fprintf(fp, "E\t%llu\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n", (unsigned long long)au_u64(r + AU_E_STAMP), (unsigned)au_u16(r + AU_E_PC), (unsigned)r[AU_E_REG], (unsigned)r[AU_E_VALUE], (unsigned)r[AU_E_CHAN], (unsigned)r[AU_E_EFFECT], (unsigned)r[AU_E_WIDX],
                     (unsigned)au_u16(r + AU_E_POST)) > 0;
        (*rows)++;
    }
    for (k = (cp->first * 192u) / lf; ok && cp->fcount > 0u; k++) {
        uint64_t start = 0u;
        uint64_t n = ao_frame_count(cp->first, cp->fcount, lf, k, &start);

        if (n == 0u) {
            break;
        }
        ok = fprintf(fp, "F\t%llu\t%llu\t%llu\n", (unsigned long long)k, (unsigned long long)start, (unsigned long long)n) > 0;
        (*rows)++;
    }
    for (i = 0u; i < cp->fcount && ok;) {
        uint32_t j = i + 1u;

        while (j < cp->fcount && memcmp(cp->frames + (size_t)j * AU_F_BYTES, cp->frames + (size_t)i * AU_F_BYTES, AU_F_BYTES) == 0) {
            j++;
        }
        ok = fprintf(fp, "P\t%u\t%u\t%u\t%u\n", (unsigned)i, (unsigned)(j - i), (unsigned)au_sample(cp, i, 1), (unsigned)au_sample(cp, i, 2)) > 0;
        (*rows)++;
        i = j;
    }
    if (fclose(fp) != 0) {
        ok = 0;
    }
    return ok ? 0 : -1;
}

static void ao_print(const char *id, const char *what, const char *label, uint64_t n, const uint8_t *bytes, size_t len)
{
    uint8_t digest[DS_SHA256_BYTES];
    char hex[2u * DS_SHA256_BYTES + 1u];

    ds_sha256(bytes, len, digest);
    au_hex(digest, hex);
    emit_out("AUDIO %s %s %s=%llu bytes=%llu sha256=%s\n", id, what, label, (unsigned long long)n, (unsigned long long)len, hex);
}

static int ao_write(const char *path, const uint8_t *bytes, size_t len)
{
    FILE *fp = fopen(path, "wb");

    if (fp == NULL) {
        return -1;
    }
    if (fwrite(bytes, 1u, len, fp) != len) {
        fclose(fp);
        return -1;
    }
    return fclose(fp) == 0 ? 0 : -1;
}

/* the whole request: validate, run the case's image (the zero policy, the case's request and deadline, the DSV1 stop) with the audio sinks, write the three files; 0 or an AO_ code (nothing is opened before the request is permitted) */
static int ao_do(const char *id)
{
    const struct kase *k = NULL;
    uint64_t until = 0u;
    char epath[128];
    char ppath[128];
    char tpath[128];
    char req[600];
    struct au_expect e;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct au_cap cp;
    struct ds_run_result res;
    struct ctx sc;
    uint8_t *es = NULL;
    uint8_t *ps = NULL;
    size_t el = 0u;
    size_t pl = 0u;
    uint64_t rows = 0u;
    int r = ao_request(id, &k, &until);

    if (r != AO_OK) {
        return r;
    }
    if (ao_paths(id, epath, ppath, tpath, sizeof epath) != 0) {
        return AO_BAD_ID;
    }
    if (!k->has[K_EXPECT] || au_parse(k->val[K_EXPECT], &e) != 0 || vd_policy_request(0u, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", req, sizeof req) != 0) {
        return AO_FAILED;
    }
    if (boot_built_rom(e.rom, req, &b, &ci) != NULL) {
        return AO_FAILED;
    }
    memset(&cp, 0, sizeof cp);
    memset(&sc, 0, sizeof sc);
    if (au_run(&sc, (struct ds_instance *)(void *)b.mem, until, 1, au_pcm_capacity(ci->machine_time, until), &cp, &res) != 0) {
        r = AO_FAILED;
    } else {
        es = au_psg_stream(&cp, &el);
        ps = au_pcm_stream(&cp, &pl);
        if (es == NULL || ps == NULL || ao_write(epath, es, el) != 0 || ao_write(ppath, ps, pl) != 0 || ao_write_tsv(tpath, &cp, ao_frame_mclk(ci), &rows) != 0) {
            r = AO_FAILED;
        } else {
            char *text = read_file(tpath);

            if (text == NULL) {
                r = AO_FAILED;
            } else {
                ao_print(id, "dspe", "records", cp.ecount, es, el);
                ao_print(id, "dspa", "frames", cp.fcount, ps, pl);
                ao_print(id, "tsv", "rows", rows, (const uint8_t *)text, strlen(text));
                free(text);
            }
        }
    }
    free(es);
    free(ps);
    au_cap_free(&cp);
    release_boot(&b);
    return r;
}

int audio_out_main(void)
{
    int r = ao_do(audio_out_id);

    if (r != AO_OK) {
        emit_err("HARNESS --audio-out refused: %s\n", ao_reason(r));
        return 2;
    }
    return 0;
}

static int ao_file_exists(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

#define AO_KEYS_MAX 8u

/* the values of the allow_fixture items of an expect (at most AO_KEYS_MAX of at most 40 characters; longer items are left to the item loop, which refuses them) */
static void ao_allow_keys(const char *expect, char (*keys)[41], unsigned *nkeys)
{
    char pre[PK_LIST_MAX];
    char *rest = pre;
    char *it;

    *nkeys = 0u;
    if (pk_copy_text(expect, pre, sizeof pre) != 0) {
        return;
    }
    while ((it = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(it, "allow_fixture=", 14u) == 0 && strlen(it + 14) <= 40u && *nkeys < AO_KEYS_MAX) {
            memcpy(keys[*nkeys], it + 14, strlen(it + 14) + 1u);
            (*nkeys)++;
        }
    }
}

/*
 * kind audio_out_rights (the video_out_rights rows, cloned): expect is a list of items separated by ';', each value at most 40 characters, at least one of each of the five kinds:
 *   allow_fixture=KEY  the fixture key is permitted (authored and listed, or a synthetic recipe)      deny_fixture=KEY  it is not (an unlisted authored image, none, a path, a commercial name)
 *   allow_case=ID      the request for the case passes every check before a file is opened, and the paths are build/gate/audio/ID.dspe, .dspa and .tsv with no further slash; and an in-memory copy of the case with its fixture key replaced by each other
 *                      allow_fixture key of the expect (at least one) is refused, because the permission must cover the image its rom= runs (review WR-04)
 *   deny_case=ID       the request is refused (a fixture not permitted, a case that is not an audio case, or no such case) and no file appears        bad_id=ID  the id is refused with no path built and no file at any naive join
 * Nothing is written by this kind: the writing of an allowed request is the --audio-out run of scripts/conformance-run, whose digests it recomputes with a host tool. E0: the rule is a literal one of this plan (T-05-16).
 */
void kind_audio_out_rights(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned classes = 0u;
    unsigned n = 0u;
    char msg[160];
    char keys[AO_KEYS_MAX][41];
    unsigned nkeys = 0u;

    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the audio_out_rights expect list is too long");
        return;
    }
    ao_allow_keys(k->val[K_EXPECT], keys, &nkeys);
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        const char *v;
        char epath[128];
        char ppath[128];
        char tpath[128];

        if (eq == NULL || strlen(eq + 1) > 40u) {
            harness_error(c, "an audio_out_rights item is KEY=VALUE with a value of at most 40 characters");
            return;
        }
        *eq = '\0';
        v = eq + 1;
        n++;
        if (strcmp(item, "allow_fixture") == 0) {
            classes |= 1u;
            snprintf(msg, sizeof msg, "the fixture key %.40s is refused but is authored and listed or a synthetic recipe", v);
            check(c, output_fixture_permitted(v), msg);
        } else if (strcmp(item, "deny_fixture") == 0) {
            classes |= 2u;
            snprintf(msg, sizeof msg, "the fixture key %.40s is permitted but is neither authored and listed nor a synthetic recipe", v);
            check(c, !output_fixture_permitted(v), msg);
        } else if (strcmp(item, "allow_case") == 0) {
            const struct kase *kk = NULL;
            uint64_t until = 0u;
            size_t dl = strlen(AO_DIR);
            int ok;

            classes |= 4u;
            snprintf(msg, sizeof msg, "the audio request for case %.40s is refused before any file is opened", v);
            check(c, ao_request(v, &kk, &until) == AO_OK, msg);
            ok = ao_paths(v, epath, ppath, tpath, sizeof epath) == 0 && strncmp(epath, AO_DIR, dl) == 0 && strncmp(ppath, AO_DIR, dl) == 0 && strncmp(tpath, AO_DIR, dl) == 0 && strchr(epath + dl, '/') == NULL && strchr(ppath + dl, '/') == NULL &&
                 strchr(tpath + dl, '/') == NULL && strstr(epath, "..") == NULL && strstr(ppath, "..") == NULL && strstr(tpath, "..") == NULL;
            snprintf(msg, sizeof msg, "the paths of case %.40s are not directly under build/gate/audio/", v);
            check(c, ok, msg);
            {
                const struct kase *ck = ao_find_case(v);
                unsigned j;
                unsigned swaps = 0u;

                for (j = 0u; ck != NULL && j < nkeys; j++) {
                    struct kase swapped = *ck;

                    if (strcmp(keys[j], ck->val[K_FIXTURE]) == 0) {
                        continue;
                    }
                    swapped.val[K_FIXTURE] = keys[j];
                    swapped.has[K_FIXTURE] = 1;
                    snprintf(msg, sizeof msg, "case %.40s with the fixture key %.40s in its place is not refused (WR-04)", v, keys[j]);
                    check(c, ao_case_check(&swapped, &until) == AO_NOT_PERMITTED, msg);
                    swaps++;
                }
                snprintf(msg, sizeof msg, "case %.40s: no other allow_fixture key of the expect to swap in for its own (WR-04)", v);
                check(c, swaps >= 1u, msg);
            }
        } else if (strcmp(item, "deny_case") == 0) {
            int before;
            int r1;

            classes |= 8u;
            snprintf(epath, sizeof epath, AO_DIR "%.40s.dspe", v);
            snprintf(ppath, sizeof ppath, AO_DIR "%.40s.dspa", v);
            snprintf(tpath, sizeof tpath, AO_DIR "%.40s.tsv", v);
            before = ao_file_exists(epath) || ao_file_exists(ppath) || ao_file_exists(tpath);
            r1 = ao_do(v);
            snprintf(msg, sizeof msg, "the audio request for case %.40s was not refused with the not-permitted, not-audio or no-such-case reason", v);
            check(c, r1 == AO_NOT_PERMITTED || r1 == AO_NOT_AUDIO || r1 == AO_NO_CASE, msg);
            snprintf(msg, sizeof msg, "a refused audio request for case %.40s left a file", v);
            check(c, before || (!ao_file_exists(epath) && !ao_file_exists(ppath) && !ao_file_exists(tpath)), msg);
        } else if (strcmp(item, "bad_id") == 0) {
            char epath2[128];
            int before;

            classes |= 16u;
            snprintf(epath, sizeof epath, AO_DIR "%.40s.dspe", v);
            snprintf(epath2, sizeof epath2, "%.40s.dspe", v);
            snprintf(ppath, sizeof ppath, AO_DIR "%.40s.dspa", v);
            snprintf(tpath, sizeof tpath, AO_DIR "%.40s.tsv", v);
            before = ao_file_exists(epath) || ao_file_exists(epath2) || ao_file_exists(ppath) || ao_file_exists(tpath);
            snprintf(msg, sizeof msg, "the id '%.40s' was accepted or a path was built for it", v);
            check(c, !ao_id_valid(v) && ao_paths(v, epath, ppath, tpath, sizeof epath) != 0 && ao_do(v) == AO_BAD_ID, msg);
            snprintf(epath, sizeof epath, AO_DIR "%.40s.dspe", v);
            snprintf(msg, sizeof msg, "a refused id '%.40s' left a file at a naive join of the id", v);
            check(c, before || (!ao_file_exists(epath) && !ao_file_exists(epath2) && !ao_file_exists(ppath) && !ao_file_exists(tpath)), msg);
        } else {
            harness_error(c, "an audio_out_rights item is allow_fixture, deny_fixture, allow_case, deny_case or bad_id");
            return;
        }
    }
    if (classes != 31u) {
        harness_error(c, "an audio_out_rights case needs at least one item of each of the five kinds");
        return;
    }
    snprintf(got, gotcap, "items=%u", n);
}

/* ---- pcm_cadence (plan 05-04 Task 3; D-187, D-201, criterion 2; ASSUMED:LED-VID-0001 + LED-AUD-0014) ----
 * The ROM of the case is run frame by frame (calls to the deadlines k x L, L = lines x 1365) for 32 frames of 262 lines and, on a second boot bound to 263 lines, 64 frames; the stream must hold ceil(M / 192) - ceil(T0 / 192) frames for the run's M, every call's first and seq must chain,
 * and the frames partitioned at k x L (the samples s with k L <= 192 s < (k + 1) L) must count as the literals below, written by hand from the grid (count(k) = ceil((k + 1) L / 192) - ceil(k L / 192); RESEARCH section 3), each in {floor, ceil} of L / 192. */
static const uint32_t pc_262[32] = {1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u,
                                    1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1863u, 1862u, 1863u, 1862u};
static const uint32_t pc_262_starts[9] = {0u, 1863u, 3726u, 5588u, 7451u, 9314u, 11176u, 13039u, 14902u};
static const uint32_t pc_263[8] = {1870u, 1870u, 1870u, 1870u, 1869u, 1870u, 1870u, 1870u};
#define PC_262_SUM 59605u
#define PC_263_SUM 119665u

/* run frames frames of the case's ROM under request in calls to k x L; the per-frame counts from the stream into counts (at most frames); 0 on success, -1 after a harness error */
static int pc_run(struct ctx *c, const char *rom, const char *request, unsigned frames, uint32_t *counts, uint64_t *total, uint64_t *mclk, int *chain_ok)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct au_cap cp;
    struct ds_run_result res;
    uint64_t lf;
    unsigned f;
    const char *err = boot_built_rom(rom, request, &b, &ci);

    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    memset(&cp, 0, sizeof cp);
    lf = ao_frame_mclk(ci);
    for (f = 1u; f <= frames; f++) {
        if (au_run(c, (struct ds_instance *)(void *)b.mem, lf * f, 0, au_pcm_capacity(0u, lf * frames), &cp, &res) != 0) {
            au_cap_free(&cp);
            release_boot(&b);
            return -1;
        }
    }
    *total = cp.fcount;
    *mclk = cp.machine_time;
    *chain_ok = !cp.chain_bad && cp.first == 0u && cp.calls == frames;
    for (f = 0u; f < frames; f++) {
        uint64_t start = 0u;

        counts[f] = (uint32_t)ao_frame_count(cp.first, cp.fcount, lf, f, &start);
    }
    au_cap_free(&cp);
    release_boot(&b);
    return 0;
}

void kind_pcm_cadence(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    char runs[48];
    const char *req = k->has[K_REQUEST] ? k->val[K_REQUEST] : "";
    char req263[600];
    uint32_t cnt[64];
    uint64_t total = 0u;
    uint64_t mclk = 0u;
    uint64_t sum = 0u;
    int chain = 0;
    unsigned f;
    char msg[240];

    if (au_rom_expect(k, rom, sizeof rom, "runs", runs, sizeof runs) != 0 || strcmp(runs, "262x32,263x64") != 0 || vd_compose_request(req, "bind.lines_per_frame=263", req263, sizeof req263) != 0) {
        harness_error(c, "a pcm_cadence case needs expect = rom=NAME;runs=262x32,263x64");
        return;
    }
    if (pc_run(c, rom, req, 32u, cnt, &total, &mclk, &chain) != 0) {
        return;
    }
    note_run(c, mclk, (uint32_t)DS_RUN_REASON_UNTIL);
    snprintf(msg, sizeof msg, "262 lines: the stream holds %llu frames after a run to %llu, the closed form ceil(M / 192) - ceil(0 / 192) is %llu, or the calls' first and seq do not chain", (unsigned long long)total, (unsigned long long)mclk, (unsigned long long)(mclk / 192u + (mclk % 192u != 0u ? 1u : 0u)));
    check(c, chain && total == mclk / 192u + (mclk % 192u != 0u ? 1u : 0u), msg);
    for (f = 0u; f < 32u; f++) {
        uint64_t start = 0u;

        snprintf(msg, sizeof msg, "262 lines: frame %u holds %u samples, the literal is %u (each count is floor or ceil of 357630 / 192)", f, (unsigned)cnt[f], (unsigned)pc_262[f]);
        check(c, cnt[f] == pc_262[f] && (cnt[f] == 357630u / 192u || cnt[f] == 357630u / 192u + 1u), msg);
        if (f < 9u) {
            (void)ao_frame_count(0u, total, 357630u, f, &start);
            snprintf(msg, sizeof msg, "262 lines: frame %u starts at sample %llu, the literal is %u", f, (unsigned long long)start, (unsigned)pc_262_starts[f]);
            check(c, start == pc_262_starts[f], msg);
        }
        sum += cnt[f];
    }
    snprintf(msg, sizeof msg, "262 lines: the 32 frames hold %llu samples, the literal is 59605 (21 x 1863 + 11 x 1862)", (unsigned long long)sum);
    check(c, sum == PC_262_SUM, msg);
    if (pc_run(c, rom, req263, 64u, cnt, &total, &mclk, &chain) != 0) {
        return;
    }
    snprintf(msg, sizeof msg, "263 lines: the stream holds %llu frames after a run to %llu, not the closed form, or the calls do not chain", (unsigned long long)total, (unsigned long long)mclk);
    check(c, chain && total == mclk / 192u + (mclk % 192u != 0u ? 1u : 0u), msg);
    sum = 0u;
    for (f = 0u; f < 64u; f++) {
        if (f < 8u) {
            snprintf(msg, sizeof msg, "263 lines: frame %u holds %u samples, the literal is %u", f, (unsigned)cnt[f], (unsigned)pc_263[f]);
            check(c, cnt[f] == pc_263[f], msg);
        }
        snprintf(msg, sizeof msg, "263 lines: frame %u holds %u samples, not floor or ceil of 358995 / 192", f, (unsigned)cnt[f]);
        check(c, cnt[f] == 358995u / 192u || cnt[f] == 358995u / 192u + 1u, msg);
        sum += cnt[f];
    }
    snprintf(msg, sizeof msg, "263 lines: the 64 frames hold %llu samples, the literal is 119665 (49 x 1870 + 15 x 1869)", (unsigned long long)sum);
    check(c, sum == PC_263_SUM, msg);
    snprintf(got, gotcap, "frames262=32 frames263=64 sum262=%u sum263=%llu", (unsigned)PC_262_SUM, (unsigned long long)sum);
}
