/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * videokinds.c - the video case kind (plan 04-04, 04-07; D-137, D-138, D-141, D-161, D-163, D-169, D-171, D-172, ER-10, ER-11, ER-12, ER-13, F-5, F-37, F-38). Test code only.
 *   video   boots a ROM that scripts/fixture-build assembled, attaches a vid sink and a dev sink, runs it with ds_run to the end of its DSV1 verdict (rq.stop = dsv1_done, until_mclk is the hard bound), judges the DSV1 block as the guest kind does, then reads the raw video records
 *           the core wrote and judges them by GROUP. The records are parsed with the OFFSETS OF D-161 WRITTEN IN THIS FILE: the runner never includes a core record header and no expected value comes from the core, another emulator or a hash (ER-13, ER-11).
 *   expect  rom=NAME;id=N;scene=NAME  (every item once, any order): the built image build/gate/fixtures/NAME.pce, the fixture id its DSV1 block carries (the case also needs the keys until_mclk and checks, as the guest kind does) and the scene of the .vexp file whose rows judge the case.
 * The literals are NOT in the case: they live in fixtures/src/NAME.vexp (hand-derived rows, the derivation in comments) and reach this file only as build/gate/fixtures/NAME.vexp.tsv, written by scripts/fixture-build (F-5, F-38); the runner takes no argument for it. The grammar, the class lint (MAN, GUEST, ASSUMED) and the role and kill rules are judged statically by scripts/video-check;
 * this file refuses an unknown row, an unknown key or a malformed row again (T-04-24). Four row kinds, TAB separated, keys closed:
 *   scene   NAME  case=CASE_ID  role=literal|struct|golden        the case CASE_ID judges the scene NAME in the given role
 *   group   SCENE GID  form=ROWS|REL|STRUCT  class=MAN:L|GUEST:M|ASSUMED:LED  sym=order|asym|none  kills=F0,F5,...      a group; the outcome of a case is the conjunction of its groups (D-171, ER-4)
 *   row     SCENE GID  SELECTOR  LITERAL                           one assertion of the group (SELECTOR and LITERAL are key=value items separated by semicolons)
 *   golden  SCENE sha256=HEX                                       the pinned stream digest of a golden scene (E0 repeatability, never a verdict, ER-11)
 * STRUCT rows (selector check=NAME), the checks written here: count, origins, frame_lines, records_per_frame, rec_bytes, values_below_512, div_table, div_first, border_after_pal, line_dirty.
 * Their classes (ER-10: the weakest class of the inputs wins) are in the .vexp file: a row that uses the 1365-clock grid, the frame length or the join of a write stamp to the grid is ASSUMED (LED-VID-0004, LED-VID-0001);
 * a div row is MAN H6-3 and is judged only on records at least one whole line away from the write that sets it; the border row is GUEST pal_bijection (pure in-guest arithmetic over the values the guest wrote).
 * The evidence text carries video_sha256=HEX E0 repeatability: the SHA-256 of the stream "duospool-video" NUL, u32 1 (little-endian), then the records back to back (D-163). A hash is a change detector and never a verdict (ER-11): the literal groups decide the case.
 * A lost record of either sink, a missing sink, an irq_stim key, an empty judged window, an unknown or malformed row and a CR write other than the row lists are harness_failure; a stream with no record fails every group.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_sha256.h"
#include "runner.h"

/* the layout of D-161 as this file knows it (hand-written, ER-13) */
#define VK_HDR 64u
#define VK_O_ORIGIN 0u
#define VK_O_FRAME 8u
#define VK_O_LINE 12u
#define VK_O_FRAME_LINES 14u
#define VK_O_REC_BYTES 16u
#define VK_O_LINE_MCLK 18u
#define VK_O_DIV 20u
#define VK_O_ACTIVE_W 24u
#define VK_O_BORDER 26u
#define VK_O_LINE_DIRTY 32u
#define VK_O_VERSION 35u
#define VK_O_RESERVED 56u
#define VK_MAX_DOTS 682u
#define VK_MAX_REC (VK_HDR + 4u * VK_MAX_DOTS + 8u)
#define VK_GRID 1365u
#define VK_DEV_CAP 16384u
#define VK_CRW_MAX 4u
#define VK_PHYS_WIN 0x1FE400u /* the base of the VCE window; the register of an access is the offset & 7 (MAN H6-2), so $1FE408 is a mirror of CR and $1FE444 a mirror of CTW low */
#define VK_PHYS_WIN_END 0x1FE800u
#define VK_REG_CR 0u
#define VK_REG_CTW_LO 4u
#define VK_REG_CTW_HI 5u
#define VK_DEV_REC 24u

struct vk_expect {
    char rom[48];
    uint64_t id;
    char scene[32];
    unsigned seen;
};

enum { VKI_ROM = 1, VKI_ID = 2, VKI_SCENE = 4, VKI_ALL = 7 };

static uint32_t vk_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t vk_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t vk_u64(const uint8_t *p)
{
    return (uint64_t)vk_u32(p) | ((uint64_t)vk_u32(p + 4) << 32);
}

/* a list of small decimal numbers separated by commas into out (at most cap), the count in *n; 0 on success */
static int vk_list(const char *text, unsigned *out, unsigned cap, unsigned *n)
{
    char buf[64];
    char *rest = buf;
    char *item;

    *n = 0u;
    if (strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        uint64_t v = 0u;

        if (*n >= cap || strspn(item, "0123456789") != strlen(item) || parse_u64(item, &v) != 0 || v > 255u) {
            return -1;
        }
        out[(*n)++] = (unsigned)v;
    }
    return *n >= 1u ? 0 : -1;
}

/* a scene name or a case id: [a-z][a-z0-9_]* of at least one and fewer than cap characters */
static int vk_name_ok(const char *s, size_t cap)
{
    size_t n = strlen(s);

    return n >= 1u && n < cap && s[0] >= 'a' && s[0] <= 'z' && strspn(s, "abcdefghijklmnopqrstuvwxyz0123456789_") == n;
}

/* the expect of a video case; 0 on success */
static int vk_parse(const char *text, struct vk_expect *e)
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
        unsigned bit = 0u;

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        val = eq + 1;
        if (strcmp(item, "rom") == 0) {
            bit = VKI_ROM;
            if (strlen(val) == 0u || strlen(val) >= sizeof e->rom || strspn(val, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(val)) {
                return -1;
            }
            memcpy(e->rom, val, strlen(val) + 1u);
        } else if (strcmp(item, "id") == 0) {
            bit = VKI_ID;
            if (parse_u64(val, &e->id) != 0 || e->id == 0u || e->id > 255u) {
                return -1;
            }
        } else if (strcmp(item, "scene") == 0) {
            bit = VKI_SCENE;
            if (!vk_name_ok(val, sizeof e->scene)) {
                return -1;
            }
            memcpy(e->scene, val, strlen(val) + 1u);
        } else {
            return -1;
        }
        if ((e->seen & bit) != 0u) {
            return -1;
        }
        e->seen |= bit;
    }
    return e->seen == (unsigned)VKI_ALL ? 0 : -1;
}

/* ---- the .vexp.tsv channel: the rows of the scenes of one fixture (D-171, F-5, F-38, T-04-24) ---- */

#define VX_NAME 32u
#define VX_CASE 72u
#define VX_MAX 512u /* rows of one file (the lexer of scripts/video-check refuses more) */
#define VX_SEL 192u
#define VX_LIT 192u
#define VX_LINE 400u

enum { VX_ROLE_LITERAL = 1, VX_ROLE_STRUCT = 2, VX_ROLE_GOLDEN = 3 };
enum { VX_FORM_ROWS = 1, VX_FORM_REL = 2, VX_FORM_STRUCT = 3 };
enum { VX_CLS_MAN = 0, VX_CLS_GUEST = 1, VX_CLS_ASSUMED = 2 };

struct vx_scene {
    char name[VX_NAME];
    char case_id[VX_CASE];
    int role;
};

struct vx_group {
    char scene[VX_NAME];
    char gid[VX_NAME];
    int form;
    int cls;
    unsigned kills; /* bit n set: the fault Fn is declared to flip the group */
};

struct vx_row {
    char scene[VX_NAME];
    char gid[VX_NAME];
    char sel[VX_SEL];
    char lit[VX_LIT];
    unsigned group;
};

struct vx_golden {
    char scene[VX_NAME];
    char sha[2u * DS_SHA256_BYTES + 1u];
};

struct vx_file {
    unsigned ns;
    unsigned ng;
    unsigned nr;
    unsigned nd;
    struct vx_scene sc[VX_MAX];
    struct vx_group gr[VX_MAX];
    struct vx_row rw[VX_MAX];
    struct vx_golden gd[VX_MAX];
};

/* the fields of a TAB separated line in place: the count (more than cap means too many fields) */
static unsigned vx_split(char *line, char **f, unsigned cap)
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

/* the value after "KEY=" of a field, NULL when the field is another key */
static const char *vx_val(const char *field, const char *key)
{
    size_t n = strlen(key);

    return strncmp(field, key, n) == 0 && field[n] == '=' ? field + n + 1u : NULL;
}

static int vx_kills(const char *text, unsigned *mask)
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
        if (item[0] != 'F' || item[1] < '0' || item[1] > '9' || item[2] != '\0') {
            return -1;
        }
        *mask |= 1u << (unsigned)(item[1] - '0');
    }
    return *mask != 0u ? 0 : -1;
}

static int vx_class(const char *text, int *cls)
{
    const char *colon = strchr(text, ':');

    if (colon == NULL || colon[1] == '\0') {
        return -1;
    }
    if (strncmp(text, "MAN:", 4u) == 0) {
        *cls = VX_CLS_MAN;
    } else if (strncmp(text, "GUEST:", 6u) == 0) {
        *cls = VX_CLS_GUEST;
    } else if (strncmp(text, "ASSUMED:", 8u) == 0) {
        *cls = VX_CLS_ASSUMED;
    } else {
        return -1;
    }
    return 0;
}

/* one row of the file into the tables; NULL on success, else why not */
static const char *vx_row_parse(struct vx_file *f, char **fld, unsigned nf)
{
    const char *a = NULL;
    const char *b = NULL;
    const char *c = NULL;
    const char *d = NULL;
    unsigned i;

    if (strcmp(fld[0], "scene") == 0) {
        struct vx_scene *s;

        if (nf != 4u || f->ns >= VX_MAX || !vk_name_ok(fld[1], VX_NAME) || (a = vx_val(fld[2], "case")) == NULL || (b = vx_val(fld[3], "role")) == NULL || !vk_name_ok(a, VX_CASE)) {
            return "vexp-malformed: a scene row is: scene NAME case=ID role=literal|struct|golden";
        }
        for (i = 0u; i < f->ns; i++) {
            if (strcmp(f->sc[i].case_id, a) == 0) {
                return "vexp-malformed: two scene rows name the same case";
            }
        }
        s = &f->sc[f->ns++];
        memcpy(s->name, fld[1], strlen(fld[1]) + 1u);
        memcpy(s->case_id, a, strlen(a) + 1u);
        if (strcmp(b, "literal") == 0) {
            s->role = VX_ROLE_LITERAL;
        } else if (strcmp(b, "struct") == 0) {
            s->role = VX_ROLE_STRUCT;
        } else if (strcmp(b, "golden") == 0) {
            s->role = VX_ROLE_GOLDEN;
        } else {
            return "vexp-unknown-key: a scene role is literal, struct or golden";
        }
    } else if (strcmp(fld[0], "group") == 0) {
        struct vx_group *g;

        if (nf != 7u || f->ng >= VX_MAX || !vk_name_ok(fld[1], VX_NAME) || !vk_name_ok(fld[2], VX_NAME) || (a = vx_val(fld[3], "form")) == NULL || (b = vx_val(fld[4], "class")) == NULL || (c = vx_val(fld[5], "sym")) == NULL || (d = vx_val(fld[6], "kills")) == NULL) {
            return "vexp-malformed: a group row is: group SCENE GID form= class= sym= kills=";
        }
        g = &f->gr[f->ng++];
        memcpy(g->scene, fld[1], strlen(fld[1]) + 1u);
        memcpy(g->gid, fld[2], strlen(fld[2]) + 1u);
        if (strcmp(a, "ROWS") == 0) {
            g->form = VX_FORM_ROWS;
        } else if (strcmp(a, "REL") == 0) {
            g->form = VX_FORM_REL;
        } else if (strcmp(a, "STRUCT") == 0) {
            g->form = VX_FORM_STRUCT;
        } else {
            return "vexp-unknown-key: a group form is ROWS, REL or STRUCT";
        }
        if (vx_class(b, &g->cls) != 0 || (strcmp(c, "order") != 0 && strcmp(c, "asym") != 0 && strcmp(c, "none") != 0) || vx_kills(d, &g->kills) != 0) {
            return "vexp-malformed: a group needs class=MAN:L|GUEST:M|ASSUMED:LED, sym=order|asym|none and a non-empty kills list of F0 to F9";
        }
        for (i = 0u; i + 1u < f->ng; i++) {
            if (strcmp(f->gr[i].scene, g->scene) == 0 && strcmp(f->gr[i].gid, g->gid) == 0) {
                return "vexp-malformed: two group rows have the same scene and id";
            }
        }
    } else if (strcmp(fld[0], "row") == 0) {
        struct vx_row *r;

        if (nf != 5u || f->nr >= VX_MAX || !vk_name_ok(fld[1], VX_NAME) || !vk_name_ok(fld[2], VX_NAME) || strlen(fld[3]) == 0u || strlen(fld[3]) >= VX_SEL || strlen(fld[4]) == 0u || strlen(fld[4]) >= VX_LIT) {
            return "vexp-malformed: a row row is: row SCENE GID SELECTOR LITERAL (selector and literal within their caps)";
        }
        r = &f->rw[f->nr++];
        memcpy(r->scene, fld[1], strlen(fld[1]) + 1u);
        memcpy(r->gid, fld[2], strlen(fld[2]) + 1u);
        memcpy(r->sel, fld[3], strlen(fld[3]) + 1u);
        memcpy(r->lit, fld[4], strlen(fld[4]) + 1u);
    } else if (strcmp(fld[0], "golden") == 0) {
        struct vx_golden *g;

        if (nf != 3u || f->nd >= VX_MAX || !vk_name_ok(fld[1], VX_NAME) || (a = vx_val(fld[2], "sha256")) == NULL || strlen(a) != 2u * DS_SHA256_BYTES || strspn(a, "0123456789abcdef") != strlen(a)) {
            return "vexp-malformed: a golden row is: golden SCENE sha256=64 lowercase hex digits";
        }
        g = &f->gd[f->nd++];
        memcpy(g->scene, fld[1], strlen(fld[1]) + 1u);
        memcpy(g->sha, a, strlen(a) + 1u);
    } else {
        return "vexp-unknown-key: a row kind is scene, group, row or golden";
    }
    return NULL;
}

/* the file build/gate/fixtures/ROM.vexp.tsv into f (cleared first); NULL on success. A missing file is a fault of the harness, never an empty scene list: the video kind is judged by these rows only */
static const char *vx_load(const char *rom, struct vx_file *f)
{
    char path[96];
    char *text;
    char *line;
    unsigned nrows = 0u;
    const char *err = NULL;
    unsigned i;

    memset(f, 0, sizeof *f);
    snprintf(path, sizeof path, "build/gate/fixtures/%s.vexp.tsv", rom);
    text = read_file(path);
    if (text == NULL) {
        return "vexp-missing: the built vexp rows are missing: run scripts/fixture-build first (the scene of a video case lives in fixtures/src/NAME.vexp)";
    }
    line = text;
    while (*line != '\0' && err == NULL) {
        char *nl = strchr(line, '\n');
        char *fld[8];
        unsigned nf;

        if (nl == NULL) {
            err = "vexp-malformed: the built vexp rows do not end in a newline";
            break;
        }
        *nl = '\0';
        if (strlen(line) > VX_LINE || ++nrows > VX_MAX) {
            err = "vexp-malformed: a row over 400 characters, or more than 512 rows";
            break;
        }
        nf = vx_split(line, fld, 8u);
        if (nf > 8u) {
            err = "vexp-malformed: a row with more than 8 fields";
            break;
        }
        err = vx_row_parse(f, fld, nf);
        line = nl + 1;
    }
    free(text);
    if (err != NULL) {
        return err;
    }
    if (f->ns == 0u) {
        return "vexp-malformed: the built vexp rows hold no scene row";
    }
    for (i = 0u; i < f->nr; i++) { /* every row names a group, every group a scene */
        unsigned g;

        for (g = 0u; g < f->ng; g++) {
            if (strcmp(f->gr[g].scene, f->rw[i].scene) == 0 && strcmp(f->gr[g].gid, f->rw[i].gid) == 0) {
                break;
            }
        }
        if (g == f->ng) {
            return "vexp-malformed: a row names a group that no group row defines";
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
            return "vexp-malformed: a group names a scene that no scene row defines";
        }
    }
    return NULL;
}

/* the index of the scene row of a case, or f->ns when no scene row names it */
static unsigned vx_case_scene(const struct vx_file *f, const char *case_id)
{
    unsigned i;

    for (i = 0u; i < f->ns; i++) {
        if (strcmp(f->sc[i].case_id, case_id) == 0) {
            return i;
        }
    }
    return f->ns;
}

/* ---- the records and the evidence they are judged against ---- */

/* the number of lines whose event lies at or before machine time M - 1: the closed form of D-137 (ASSUMED LED-VID-0004, LED-VID-0001), written here for the sink sizes and for the --video-out recipe; the STRUCT count row takes its grid from the .vexp literal */
static uint64_t vk_count_closed_form(uint64_t m)
{
    return m == 0u ? 0u : (m - 1u) / (uint64_t)VK_GRID + 1u;
}

/* the index of the first line whose origin is after the stamp t, on the grid k * 1365 (ASSUMED LED-VID-0004: an event at or before the stamp precedes the access) */
static uint64_t vk_first_after(uint64_t t)
{
    return t / (uint64_t)VK_GRID + 1u;
}

struct vk_log {
    uint64_t t_cr[VK_CRW_MAX];
    unsigned v_cr[VK_CRW_MAX];
    unsigned n_cr;
    uint64_t t_pal[4096];
    unsigned n_pal;
    uint64_t t_pal_last;
    uint64_t m; /* the machine time the run reached */
};

/* the CR writes and the palette data writes of the dev log, found by the register of the physical address in the VCE window (offset & 7, MAN H6-2, mirrors included) and by kind (the stamps are the join points of the rows); 0 when the log is well formed */
static int vk_scan_dev(const uint8_t *recs, uint32_t count, struct vk_log *lg)
{
    uint32_t i;

    memset(lg, 0, sizeof *lg);
    for (i = 0u; i < count; i++) {
        const uint8_t *r = recs + (size_t)i * VK_DEV_REC;
        uint32_t phys = vk_u32(r + 8);

        if ((unsigned)r[13] != (unsigned)DS_DEV_WRITE) {
            continue;
        }
        if (phys < (uint32_t)VK_PHYS_WIN || phys >= (uint32_t)VK_PHYS_WIN_END) {
            continue;
        }
        if ((phys & 7u) == (uint32_t)VK_REG_CR) {
            if (lg->n_cr >= VK_CRW_MAX) {
                return -1;
            }
            lg->t_cr[lg->n_cr] = vk_u64(r);
            lg->v_cr[lg->n_cr] = (unsigned)r[12];
            lg->n_cr++;
        } else if ((phys & 7u) == (uint32_t)VK_REG_CTW_LO || (phys & 7u) == (uint32_t)VK_REG_CTW_HI) {
            if (lg->n_pal >= (unsigned)(sizeof lg->t_pal / sizeof lg->t_pal[0])) {
                return -1;
            }
            lg->t_pal[lg->n_pal++] = vk_u64(r);
            lg->t_pal_last = vk_u64(r);
        }
    }
    return 0;
}

/* record k: the header at the start of the k-th record of the walk */
struct vk_walk {
    const uint8_t **rec;
    uint64_t n;
    int ok;
};

static void vk_walk_records(const uint8_t *bytes, uint64_t stored, uint64_t count, struct vk_walk *w)
{
    uint64_t off = 0u;
    uint64_t k;

    w->ok = 1;
    w->n = 0u;
    for (k = 0u; k < count; k++) {
        uint32_t rb;

        if (off + (uint64_t)VK_HDR > stored) {
            w->ok = 0;
            return;
        }
        rb = vk_u16(bytes + off + VK_O_REC_BYTES);
        if (rb < VK_HDR || rb > VK_MAX_REC || off + (uint64_t)rb > stored) {
            w->ok = 0;
            return;
        }
        w->rec[k] = bytes + off;
        off += (uint64_t)rb;
        w->n++;
    }
    if (off != stored) {
        w->ok = 0;
    }
}

/* ---- the judge: a row is evaluated on one run of records, a group is the conjunction of its rows (D-171) ---- */

enum { VG_PASS = 0, VG_FAIL = 1, VG_HARNESS = 2 };
#define VK_NOFOCUS UINT64_MAX
#define VK_KV_MAX 6u

struct vk_run {
    const char *name;
    const struct vk_walk *w;
    const struct vk_log *lg;
};

struct vk_kv {
    char k[16];
    char v[72];
};

/* the working state of one row: the run it is judged on, the key=value items of its literal, the record the row looks at first (the focus of the fault sweep) and the text of a failure */
struct vk_rc {
    const struct vk_walk *w;
    const struct vk_log *lg;
    struct vk_kv kv[VK_KV_MAX];
    unsigned nkv;
    uint64_t focus;
    char *why;
    size_t cap;
};

/* the items KEY=VALUE separated by semicolons of text into rc->kv (at least one, none empty, no key twice); 0 on success */
static int vk_kv_parse(const char *text, struct vk_rc *rc)
{
    char buf[VX_LIT];
    char *rest = buf;
    char *item;
    size_t len = strlen(text);

    rc->nkv = 0u;
    if (len == 0u || len >= sizeof buf || text[len - 1u] == ';') {
        return -1;
    }
    memcpy(buf, text, len + 1u);
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        unsigned i;

        if (eq == NULL || eq == item || eq[1] == '\0' || rc->nkv >= VK_KV_MAX) {
            return -1;
        }
        *eq = '\0';
        if (strlen(item) >= sizeof rc->kv[0].k || strlen(eq + 1) >= sizeof rc->kv[0].v) {
            return -1;
        }
        for (i = 0u; i < rc->nkv; i++) {
            if (strcmp(rc->kv[i].k, item) == 0) {
                return -1;
            }
        }
        memcpy(rc->kv[rc->nkv].k, item, strlen(item) + 1u);
        memcpy(rc->kv[rc->nkv].v, eq + 1, strlen(eq + 1) + 1u);
        rc->nkv++;
    }
    return rc->nkv >= 1u ? 0 : -1;
}

static int vk_kv_find(const struct vk_rc *rc, const char *key)
{
    unsigned i;

    for (i = 0u; i < rc->nkv; i++) {
        if (strcmp(rc->kv[i].k, key) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* the literal holds exactly the keys of the list, each once; 0 when it does */
static int vk_need(const struct vk_rc *rc, const char *const *keys, unsigned nk)
{
    unsigned i;

    if (rc->nkv != nk) {
        return -1;
    }
    for (i = 0u; i < nk; i++) {
        if (vk_kv_find(rc, keys[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

/* a decimal item of the literal; 0 on success */
static int vk_num(const struct vk_rc *rc, const char *key, uint64_t *out)
{
    int i = vk_kv_find(rc, key);
    const char *v;

    if (i < 0) {
        return -1;
    }
    v = rc->kv[i].v;
    if (strlen(v) > 18u || strspn(v, "0123456789") != strlen(v)) {
        return -1;
    }
    return parse_u64(v, out);
}

static int vk_lst(const struct vk_rc *rc, const char *key, unsigned *out, unsigned cap, unsigned *n)
{
    int i = vk_kv_find(rc, key);

    return i < 0 ? -1 : vk_list(rc->kv[i].v, out, cap, n);
}

static int vk_bad(struct vk_rc *rc, const char *what)
{
    snprintf(rc->why, rc->cap, "%s", what);
    return VG_HARNESS;
}

static void vk_focus(struct vk_rc *rc, uint64_t idx)
{
    if (rc->focus == VK_NOFOCUS) {
        rc->focus = idx;
    }
}

/* STRUCT count: the number of records is floor((M - 1) / grid) + 1 for the machine time M the run reached (ASSUMED LED-VID-0004) */
static int vk_s_count(struct vk_rc *rc)
{
    static const char *const keys[] = {"grid"};
    uint64_t grid = 0u;
    uint64_t m = rc->lg->m;
    uint64_t want;

    if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "grid", &grid) != 0 || grid == 0u) {
        return vk_bad(rc, "STRUCT count needs the single item grid=N");
    }
    want = m == 0u ? 0u : (m - 1u) / grid + 1u;
    vk_focus(rc, rc->w->n / 2u);
    if (rc->w->n != want) {
        snprintf(rc->why, rc->cap, "STRUCT count: the sink holds %llu records, the closed form floor((M - 1) / %llu) + 1 of the machine time %llu is %llu", (unsigned long long)rc->w->n, (unsigned long long)grid, (unsigned long long)m, (unsigned long long)want);
        return VG_FAIL;
    }
    return VG_PASS;
}

/* STRUCT origins: record k starts at k * step and says line_mclk = step (ASSUMED LED-VID-0004) */
static int vk_s_origins(struct vk_rc *rc)
{
    static const char *const keys[] = {"step"};
    uint64_t step = 0u;
    uint64_t k;

    if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "step", &step) != 0 || step == 0u || step > 1000000u) {
        return vk_bad(rc, "STRUCT origins needs the single item step=N");
    }
    vk_focus(rc, rc->w->n / 2u);
    for (k = 0u; k < rc->w->n; k++) {
        if (vk_u64(rc->w->rec[k] + VK_O_ORIGIN) != k * step || (uint64_t)vk_u16(rc->w->rec[k] + VK_O_LINE_MCLK) != step) {
            snprintf(rc->why, rc->cap, "STRUCT origins: record %llu does not start at k * %llu or does not say line_mclk = %llu", (unsigned long long)k, (unsigned long long)step, (unsigned long long)step);
            return VG_FAIL;
        }
    }
    return VG_PASS;
}

/* STRUCT frame_lines: every record says frame_lines = value, a line below it, and (frame * value + line) * step = its origin (ASSUMED LED-VID-0001) */
static int vk_s_frame_lines(struct vk_rc *rc)
{
    static const char *const keys[] = {"value", "step"};
    uint64_t fl = 0u;
    uint64_t step = 0u;
    uint64_t k;

    if (vk_need(rc, keys, 2u) != 0 || vk_num(rc, "value", &fl) != 0 || vk_num(rc, "step", &step) != 0 || fl == 0u || fl > 1000u || step == 0u || step > 1000000u) {
        return vk_bad(rc, "STRUCT frame_lines needs the items value=N (up to 1000) and step=N");
    }
    vk_focus(rc, rc->w->n / 2u);
    for (k = 0u; k < rc->w->n; k++) {
        const uint8_t *r = rc->w->rec[k];
        uint64_t frame = (uint64_t)vk_u32(r + VK_O_FRAME);
        uint64_t line = (uint64_t)vk_u16(r + VK_O_LINE);

        if ((uint64_t)vk_u16(r + VK_O_FRAME_LINES) != fl || line >= fl || (frame * fl + line) * step != vk_u64(r + VK_O_ORIGIN)) {
            snprintf(rc->why, rc->cap, "STRUCT frame_lines: record %llu has another frame_lines than %llu, a line outside its frame, or (frame * frame_lines + line) * %llu differs from its origin", (unsigned long long)k, (unsigned long long)fl, (unsigned long long)step);
            return VG_FAIL;
        }
    }
    return VG_PASS;
}

/* STRUCT records_per_frame: every frame but the last holds exactly value records (ASSUMED LED-VID-0001); the last frame is partial and is not judged, and a stream with no complete frame is a fault of the case */
static int vk_s_records_per_frame(struct vk_rc *rc)
{
    static const char *const keys[] = {"value"};
    uint64_t want = 0u;
    uint64_t k;
    uint64_t run = 0u;
    uint64_t complete = 0u;
    uint32_t cur;

    if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "value", &want) != 0 || want == 0u) {
        return vk_bad(rc, "STRUCT records_per_frame needs the single item value=N");
    }
    vk_focus(rc, rc->w->n / 2u);
    cur = vk_u32(rc->w->rec[0] + VK_O_FRAME);
    for (k = 0u; k < rc->w->n; k++) {
        uint32_t f = vk_u32(rc->w->rec[k] + VK_O_FRAME);

        if (f != cur) {
            if (run != want) {
                snprintf(rc->why, rc->cap, "STRUCT records_per_frame: frame %u holds %llu records, not %llu", (unsigned)cur, (unsigned long long)run, (unsigned long long)want);
                return VG_FAIL;
            }
            complete++;
            cur = f;
            run = 0u;
        }
        run++;
    }
    return complete >= 1u ? VG_PASS : vk_bad(rc, "STRUCT records_per_frame: the stream holds no complete frame");
}

/* STRUCT rec_bytes: every record says rec_bytes = header + ((4 * active_w + 7) & ~7), a width up to max_w, the version and a zero reserved word (ASSUMED LED-VID-0004: the layout is the project definition of D-161) */
static int vk_s_rec_bytes(struct vk_rc *rc)
{
    static const char *const keys[] = {"header", "version", "max_w"};
    uint64_t hdr = 0u;
    uint64_t ver = 0u;
    uint64_t maxw = 0u;
    uint64_t k;

    if (vk_need(rc, keys, 3u) != 0 || vk_num(rc, "header", &hdr) != 0 || vk_num(rc, "version", &ver) != 0 || vk_num(rc, "max_w", &maxw) != 0 || hdr > 4096u || maxw > 4096u) {
        return vk_bad(rc, "STRUCT rec_bytes needs the items header=N, version=N and max_w=N");
    }
    vk_focus(rc, rc->w->n / 2u);
    for (k = 0u; k < rc->w->n; k++) {
        const uint8_t *r = rc->w->rec[k];
        uint64_t aw = (uint64_t)vk_u16(r + VK_O_ACTIVE_W);

        if (aw > maxw || (uint64_t)vk_u16(r + VK_O_REC_BYTES) != hdr + ((4u * aw + 7u) & ~(uint64_t)7u) || (uint64_t)r[VK_O_VERSION] != ver || vk_u64(r + VK_O_RESERVED) != 0u) {
            snprintf(rc->why, rc->cap, "STRUCT rec_bytes: record %llu has a rec_bytes other than %llu + ((4 * active_w + 7) & ~7), a width above %llu, another version than %llu, or a nonzero reserved word", (unsigned long long)k, (unsigned long long)hdr, (unsigned long long)maxw, (unsigned long long)ver);
            return VG_FAIL;
        }
    }
    return VG_PASS;
}

/* STRUCT values_below_512: the border, every index and every colour value of every record is below limit (the values are 9 bits, D-161) */
static int vk_s_values(struct vk_rc *rc)
{
    static const char *const keys[] = {"limit"};
    uint64_t lim = 0u;
    uint64_t k;

    if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "limit", &lim) != 0 || lim == 0u) {
        return vk_bad(rc, "STRUCT values_below_512 needs the single item limit=N");
    }
    vk_focus(rc, rc->w->n / 2u);
    for (k = 0u; k < rc->w->n; k++) {
        const uint8_t *r = rc->w->rec[k];
        uint32_t aw = vk_u16(r + VK_O_ACTIVE_W);
        uint32_t j;
        int ok = (uint64_t)vk_u16(r + VK_O_BORDER) < lim;

        for (j = 0u; j < 2u * aw && ok; j++) { /* the index array and the colour array */
            ok = (uint64_t)vk_u16(r + VK_HDR + 2u * j) < lim;
        }
        if (!ok) {
            snprintf(rc->why, rc->cap, "STRUCT values_below_512: record %llu holds a border, index or colour value of %llu or more", (unsigned long long)k, (unsigned long long)lim);
            return VG_FAIL;
        }
    }
    return VG_PASS;
}

/* STRUCT div_table (MAN H6-3): the records wholly inside the window of CR write j carry the divider div[j]. A record lies wholly in the window when its origin is at least one line after the stamp of write j and, when write j + 1 exists, ends a line before it (origin + 2 lines <= the next stamp), so it depends neither on the grid phase nor on the tie rule and never on the power-on CR.
 * The log must hold exactly the CR writes crw lists, at least gap clocks apart (an empty window is a fault of the case, not a pass) */
static int vk_s_div_table(struct vk_rc *rc)
{
    static const char *const keys[] = {"div", "crw", "gap"};
    unsigned dv[VK_CRW_MAX];
    unsigned cw[VK_CRW_MAX];
    unsigned nd = 0u;
    unsigned nc = 0u;
    uint64_t gap = 0u;
    unsigned j;

    if (vk_need(rc, keys, 3u) != 0 || vk_lst(rc, "div", dv, VK_CRW_MAX, &nd) != 0 || vk_lst(rc, "crw", cw, VK_CRW_MAX, &nc) != 0 || nd != nc || vk_num(rc, "gap", &gap) != 0) {
        return vk_bad(rc, "STRUCT div_table needs the items div=D,D, crw=V,V (the same count) and gap=N");
    }
    if (rc->lg->n_cr != nc) {
        return vk_bad(rc, "the dev log does not hold exactly the CR writes the div_table row lists");
    }
    for (j = 0u; j < nc; j++) {
        if (rc->lg->v_cr[j] != cw[j] || (j > 0u && rc->lg->t_cr[j] - rc->lg->t_cr[j - 1u] < gap)) {
            return vk_bad(rc, "a CR write of the guest has another value than the div_table row lists, or two CR writes are closer than its gap (the window of judged records could be empty)");
        }
    }
    for (j = 0u; j < nc; j++) {
        uint64_t k;
        uint64_t judged = 0u;

        for (k = 0u; k < rc->w->n; k++) {
            uint64_t o = vk_u64(rc->w->rec[k] + VK_O_ORIGIN);

            if (o < rc->lg->t_cr[j] + (uint64_t)VK_GRID) {
                continue;
            }
            if (j + 1u < nc && o + 2u * (uint64_t)VK_GRID > rc->lg->t_cr[j + 1u]) {
                continue;
            }
            if (j == 0u && judged == 0u) {
                vk_focus(rc, k);
            }
            judged++;
            if ((unsigned)rc->w->rec[k][VK_O_DIV] != dv[j]) {
                snprintf(rc->why, rc->cap, "div (MAN H6-3): record %llu, at least one line after CR write %u (value %u) and before the next one, does not carry divider %u", (unsigned long long)k, j + 1u, cw[j], dv[j]);
                return VG_FAIL;
            }
        }
        if (judged == 0u) {
            return vk_bad(rc, "the window of a CR write holds no record: the case cannot judge its divider");
        }
    }
    return VG_PASS;
}

/* STRUCT div_first (ASSUMED LED-VID-0004, the tie rule of D-138): the first record whose origin is after the stamp of CR write crw carries divider div and the record before it divider prev */
static int vk_s_div_first(struct vk_rc *rc)
{
    static const char *const keys[] = {"div", "prev", "crw"};
    uint64_t dv = 0u;
    uint64_t pv = 0u;
    uint64_t cw = 0u;
    uint64_t r1;

    if (vk_need(rc, keys, 3u) != 0 || vk_num(rc, "div", &dv) != 0 || vk_num(rc, "prev", &pv) != 0 || vk_num(rc, "crw", &cw) != 0 || cw >= (uint64_t)VK_CRW_MAX) {
        return vk_bad(rc, "STRUCT div_first needs the items div=D, prev=D and crw=J");
    }
    if (cw >= (uint64_t)rc->lg->n_cr) {
        return vk_bad(rc, "the dev log holds no CR write number crw");
    }
    r1 = vk_first_after(rc->lg->t_cr[cw]);
    vk_focus(rc, r1 < rc->w->n ? r1 : rc->w->n / 2u);
    if (!(r1 >= 1u && r1 < rc->w->n && (uint64_t)rc->w->rec[r1][VK_O_DIV] == dv && (uint64_t)rc->w->rec[r1 - 1u][VK_O_DIV] == pv)) {
        snprintf(rc->why, rc->cap, "first change (ASSUMED LED-VID-0004): the first record after CR write %llu does not carry divider %llu, or the record before it does not carry %llu", (unsigned long long)cw + 1u, (unsigned long long)dv, (unsigned long long)pv);
        return VG_FAIL;
    }
    return VG_PASS;
}

/* STRUCT border_after_pal (GUEST pal_bijection): every record at least one line after the last palette data write carries the border value; the window must hold a record */
static int vk_s_border(struct vk_rc *rc)
{
    static const char *const keys[] = {"value"};
    uint64_t want = 0u;
    uint64_t k;
    uint64_t judged = 0u;

    if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "value", &want) != 0 || want > 511u) {
        return vk_bad(rc, "STRUCT border_after_pal needs the single item value=N (a 9-bit value)");
    }
    if (rc->lg->n_pal == 0u) {
        return vk_bad(rc, "the dev log holds no palette data write");
    }
    for (k = 0u; k < rc->w->n; k++) {
        if (vk_u64(rc->w->rec[k] + VK_O_ORIGIN) >= rc->lg->t_pal_last + (uint64_t)VK_GRID) {
            if (judged == 0u) {
                vk_focus(rc, k);
            }
            judged++;
            if ((uint64_t)vk_u16(rc->w->rec[k] + VK_O_BORDER) != want) {
                snprintf(rc->why, rc->cap, "border (GUEST pal_bijection): record %llu, at least one line after the last palette write, does not carry the border %llu the guest left", (unsigned long long)k, (unsigned long long)want);
                return VG_FAIL;
            }
        }
    }
    return judged >= 1u ? VG_PASS : vk_bad(rc, "no record lies a whole line after the last palette write: the case cannot judge the border");
}

/* STRUCT line_dirty (ASSUMED LED-VID-0004, the join of a write stamp to the grid): bit cr_bit is set on exactly the first record after each CR write and on no other record, bit pal_bit on exactly the first record after each palette data write */
static int vk_s_line_dirty(struct vk_rc *rc)
{
    static const char *const keys[] = {"cr_bit", "pal_bit"};
    uint64_t cb = 0u;
    uint64_t pb = 0u;
    uint64_t k;
    uint64_t j;
    int ok0 = 1;
    int ok4 = 1;
    uint64_t *want0;
    uint64_t *want4;

    if (vk_need(rc, keys, 2u) != 0 || vk_num(rc, "cr_bit", &cb) != 0 || vk_num(rc, "pal_bit", &pb) != 0 || cb > 7u || pb > 7u) {
        return vk_bad(rc, "STRUCT line_dirty needs the items cr_bit=N and pal_bit=N (bits 0 to 7)");
    }
    want0 = (uint64_t *)calloc((size_t)(rc->w->n / 64u + 1u), sizeof(uint64_t));
    want4 = (uint64_t *)calloc((size_t)(rc->w->n / 64u + 1u), sizeof(uint64_t));
    if (want0 == NULL || want4 == NULL) {
        free(want0);
        free(want4);
        return vk_bad(rc, "out of memory for the line_dirty rows");
    }
    vk_focus(rc, rc->lg->n_cr > 0u && vk_first_after(rc->lg->t_cr[0]) < rc->w->n ? vk_first_after(rc->lg->t_cr[0]) : rc->w->n / 2u);
    for (j = 0u; j < (uint64_t)rc->lg->n_cr; j++) {
        uint64_t r = vk_first_after(rc->lg->t_cr[j]);

        if (r < rc->w->n) {
            want0[r / 64u] |= UINT64_C(1) << (r % 64u);
        }
    }
    for (j = 0u; j < (uint64_t)rc->lg->n_pal; j++) {
        uint64_t r = vk_first_after(rc->lg->t_pal[j]);

        if (r < rc->w->n) {
            want4[r / 64u] |= UINT64_C(1) << (r % 64u);
        }
    }
    for (k = 0u; k < rc->w->n; k++) {
        unsigned d = (unsigned)rc->w->rec[k][VK_O_LINE_DIRTY];
        unsigned w0 = (unsigned)((want0[k / 64u] >> (k % 64u)) & 1u);
        unsigned w4 = (unsigned)((want4[k / 64u] >> (k % 64u)) & 1u);

        ok0 = ok0 && ((d >> (unsigned)cb) & 1u) == w0;
        ok4 = ok4 && ((d >> (unsigned)pb) & 1u) == w4;
    }
    free(want0);
    free(want4);
    if (!ok0 || !ok4) {
        snprintf(rc->why, rc->cap, "line_dirty (ASSUMED LED-VID-0004): bit %llu is not set on exactly the first record after each CR write, or bit %llu not on exactly the first record after each palette data write", (unsigned long long)cb, (unsigned long long)pb);
        return VG_FAIL;
    }
    return VG_PASS;
}

struct vk_chk {
    const char *name;
    int (*fn)(struct vk_rc *);
};

static const struct vk_chk vk_checks[] = {
    {"count", vk_s_count},
    {"origins", vk_s_origins},
    {"frame_lines", vk_s_frame_lines},
    {"records_per_frame", vk_s_records_per_frame},
    {"rec_bytes", vk_s_rec_bytes},
    {"values_below_512", vk_s_values},
    {"div_table", vk_s_div_table},
    {"div_first", vk_s_div_first},
    {"border_after_pal", vk_s_border},
    {"line_dirty", vk_s_line_dirty},
};

/* F0 (D-172): the first decimal number of a literal grows by one, so the row is made to claim something its own source does not say; 0 when the text holds a number */
static int vk_perturb(char *lit, size_t cap)
{
    size_t i = 0u;
    size_t j;
    char tail[VX_LIT];
    uint64_t v = 0u;
    int n;

    while (lit[i] != '\0' && (lit[i] < '0' || lit[i] > '9')) {
        i++;
    }
    if (lit[i] == '\0') {
        return -1;
    }
    for (j = i; lit[j] >= '0' && lit[j] <= '9'; j++) {
        if (j - i >= 15u) {
            return -1;
        }
        v = v * 10u + (uint64_t)(lit[j] - '0');
    }
    if (strlen(lit + j) >= sizeof tail) {
        return -1;
    }
    memcpy(tail, lit + j, strlen(lit + j) + 1u);
    n = snprintf(lit + i, cap - i, "%llu%s", (unsigned long long)(v + 1u), tail);
    return n > 0 && (size_t)n < cap - i ? 0 : -1;
}

/* one row on the runs of a case: the verdict, the text of a failure and the record it looked at first. perturb is F0 (the literal of this row is changed before it is read) */
static int vk_eval_row(const struct vx_group *g, const struct vx_row *r, const struct vk_run *runs, unsigned nruns, int perturb, uint64_t *focus, char *why, size_t cap)
{
    struct vk_rc rc;
    char lit[VX_LIT + 8u];
    unsigned i;
    int v;

    memset(&rc, 0, sizeof rc);
    rc.why = why;
    rc.cap = cap;
    rc.focus = VK_NOFOCUS;
    why[0] = '\0';
    if (nruns < 1u) {
        return VG_HARNESS;
    }
    rc.w = runs[nruns - 1u].w; /* the run a fault is applied to is the last one (the variant of a REL pair) */
    rc.lg = runs[nruns - 1u].lg;
    if (g->form != VX_FORM_STRUCT) {
        snprintf(why, cap, "the %s form of group %s is not judged by this version of the runner", g->form == VX_FORM_ROWS ? "ROWS" : "REL", g->gid);
        return VG_HARNESS;
    }
    if (vk_kv_parse(r->sel, &rc) != 0 || rc.nkv != 1u || strcmp(rc.kv[0].k, "check") != 0) {
        snprintf(why, cap, "a STRUCT row of group %s has a selector other than check=NAME", g->gid);
        return VG_HARNESS;
    }
    for (i = 0u; i < (unsigned)(sizeof vk_checks / sizeof vk_checks[0]); i++) {
        if (strcmp(vk_checks[i].name, rc.kv[0].v) == 0) {
            break;
        }
    }
    if (i == (unsigned)(sizeof vk_checks / sizeof vk_checks[0])) {
        snprintf(why, cap, "a STRUCT row of group %s names the unknown check %s", g->gid, rc.kv[0].v);
        return VG_HARNESS;
    }
    memcpy(lit, r->lit, strlen(r->lit) + 1u);
    if (perturb && vk_perturb(lit, sizeof lit) != 0) {
        snprintf(why, cap, "the literal of a STRUCT row of group %s holds no number to perturb", g->gid);
        return VG_HARNESS;
    }
    if (vk_kv_parse(lit, &rc) != 0) {
        snprintf(why, cap, "the literal of a STRUCT row of group %s is not key=value items separated by semicolons", g->gid);
        return VG_HARNESS;
    }
    v = vk_checks[i].fn(&rc);
    *focus = rc.focus;
    return v;
}

/* a group on the runs of a case: the worst verdict of its rows (a harness fault, then a failure, then a pass). A run with no record fails every group (a null renderer never passes, D-172). focus is the record the first row looked at; perturb is F0 applied to the first row of the group */
static int vk_eval_group(const struct vx_file *f, unsigned gi, const struct vk_run *runs, unsigned nruns, int perturb, uint64_t *focus, char *why, size_t cap)
{
    unsigned i;
    unsigned r;
    int worst = VG_PASS;
    int first = 1;

    why[0] = '\0';
    *focus = VK_NOFOCUS;
    for (r = 0u; r < nruns; r++) {
        if (runs[r].w->n == 0u) {
            snprintf(why, cap, "the run %s holds no record", runs[r].name);
            return VG_FAIL;
        }
    }
    for (i = 0u; i < f->nr; i++) {
        char w1[240];
        uint64_t fo = VK_NOFOCUS;
        int v;

        if (f->rw[i].group != gi) {
            continue;
        }
        v = vk_eval_row(&f->gr[gi], &f->rw[i], runs, nruns, perturb && first, &fo, w1, sizeof w1);
        first = 0;
        if (*focus == VK_NOFOCUS) {
            *focus = fo;
        }
        if (v > worst) {
            worst = v;
            snprintf(why, cap, "%s", w1);
        }
    }
    if (first) {
        snprintf(why, cap, "the group %s has no row", f->gr[gi].gid);
        return VG_HARNESS;
    }
    return worst;
}

static void vk_hex(const uint8_t *d, char *out)
{
    static const char digits[] = "0123456789abcdef";
    unsigned i;

    for (i = 0u; i < DS_SHA256_BYTES; i++) {
        out[2u * i] = digits[(d[i] >> 4) & 0xFu];
        out[2u * i + 1u] = digits[d[i] & 0xFu];
    }
    out[2u * DS_SHA256_BYTES] = '\0';
}

/* the canonical stream of D-163: "duospool-video" NUL, u32 1 (little-endian), then the records back to back; a heap block of 19 + vbytes bytes (NULL when out of memory), freed by the caller */
static uint8_t *vk_stream(const uint8_t *vbuf, uint64_t vbytes)
{
    uint8_t *stream = (uint8_t *)malloc((size_t)(19u + vbytes));

    if (stream == NULL) {
        return NULL;
    }
    memcpy(stream, "duospool-video", 14u);
    stream[14] = 0u;
    stream[15] = 1u;
    stream[16] = 0u;
    stream[17] = 0u;
    stream[18] = 0u;
    if (vbytes > 0u) { /* a copy of zero bytes still must not get a null pointer */
        memcpy(stream + 19, vbuf, (size_t)vbytes);
    }
    return stream;
}

/* ---- the capture of a video run: the boot, the run to the end of the DSV1 verdict with a vid and a dev sink, the records walked and the dev log scanned ---- */

struct vk_cap {
    struct vk_expect e;
    struct vx_file *vx;
    uint64_t until;
    uint64_t checks;
    struct ds_dev_sink *dsk;
    struct ds_vid_sink *vsk;
    uint8_t *drec;
    uint8_t *vbuf;
    const uint8_t **recp;
    uint64_t lines_due;
    uint64_t vcap;
    uint64_t vbytes;
    uint32_t vcount;
    uint32_t dcount;
    struct vk_walk w;
    struct vk_log lg;
    char verdict[64];
};

static void vk_cap_free(struct vk_cap *cp)
{
    free(cp->vx);
    free(cp->dsk);
    free(cp->vsk);
    free(cp->drec);
    free(cp->vbuf);
    free((void *)cp->recp);
    memset(cp, 0, sizeof *cp);
}

/* the whole capture of a video case; 0 when the stream and the dev log are well formed and the guest verdict passed, else non-zero (the outcome is already set in c, and got holds the verdict text when there is one). cp is cleared first and is always to be freed with vk_cap_free */
static int vk_capture(const struct kase *k, struct ctx *c, struct vk_cap *cp, char *got, size_t gotcap)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    const char *err;

    memset(cp, 0, sizeof *cp);
    if (!k->has[K_EXPECT] || vk_parse(k->val[K_EXPECT], &cp->e) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &cp->until) != 0 || cp->until == 0u || !k->has[K_CHECKS] || parse_u64(k->val[K_CHECKS], &cp->checks) != 0 || cp->checks == 0u || cp->checks > 255u) {
        harness_error(c, "a video case needs until_mclk, checks and expect = rom=NAME;id=N;scene=NAME (every item once)");
        return -1;
    }
    if (k->has[K_IRQ_STIM]) {
        harness_error(c, "a video case declares no irq_stim: the interrupt test hook is not part of a video scene");
        return -1;
    }
    /* the sink sizes from the closed form of the deadline: one line more than the lines due at until_mclk, at the longest record (D-162) */
    cp->lines_due = vk_count_closed_form(cp->until) + 1u;
    cp->vcap = cp->lines_due * (uint64_t)VK_MAX_REC;
    if (cp->vcap > (uint64_t)DS_VID_SINK_MAX_BYTES) {
        harness_error(c, "the video case is too long for one vid sink (until_mclk needs more than 64 MiB)");
        return -1;
    }
    cp->vx = (struct vx_file *)calloc(1u, sizeof *cp->vx);
    cp->dsk = (struct ds_dev_sink *)calloc(1u, sizeof *cp->dsk);
    cp->vsk = (struct ds_vid_sink *)calloc(1u, sizeof *cp->vsk);
    cp->drec = (uint8_t *)calloc((size_t)VK_DEV_CAP, (size_t)VK_DEV_REC);
    cp->vbuf = (uint8_t *)calloc((size_t)cp->vcap, 1u);
    cp->recp = (const uint8_t **)calloc((size_t)cp->lines_due, sizeof *cp->recp);
    if (cp->vx == NULL || cp->dsk == NULL || cp->vsk == NULL || cp->drec == NULL || cp->vbuf == NULL || cp->recp == NULL) {
        harness_error(c, "out of memory for the sinks of a video case");
        return -1;
    }
    err = vx_load(cp->e.rom, cp->vx);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    err = boot_built_rom(cp->e.rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    cp->dsk->records = cp->drec;
    cp->dsk->capacity = VK_DEV_CAP;
    cp->vsk->bytes = cp->vbuf;
    cp->vsk->capacity = (uint32_t)cp->vcap;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = cp->until;
    rq.stop = dsv1_done; /* a speed aid only (D-95): until_mclk is the hard bound */
    rq.dev = cp->dsk;
    rq.vid = cp->vsk;
    if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of a video case");
        release_boot(&b);
        return -1;
    }
    note_run(c, res.machine_time, res.reason);
    judge_dsv1(c, ci->ram8k + V_OFF, (unsigned)cp->e.id, (unsigned)cp->checks, cp->verdict, sizeof cp->verdict);
    release_boot(&b);
    snprintf(got, gotcap, "%s", cp->verdict);
    if (c->harness || c->timeout || c->failed) {
        return -1;
    }
    cp->vcount = vk_u32(cp->vsk->header);
    cp->dcount = vk_u32(cp->dsk->header);
    cp->vbytes = vk_u64(cp->vsk->header + 16);
    if (vk_u32(cp->vsk->header + 4) != 0u || vk_u32(cp->dsk->header + 4) != 0u || vk_u64(cp->vsk->header + 8) != (uint64_t)cp->vcount || cp->vbytes > cp->vcap) {
        harness_error(c, "a sink of the video case lost a record or its header is inconsistent: the stream would be incomplete evidence (D-162)");
        return -1;
    }
    if ((uint64_t)cp->vcount > cp->lines_due) {
        check(c, 0, "STRUCT count: the vid sink holds more records than the lines due at the deadline plus one");
        return -1;
    }
    cp->w.rec = cp->recp;
    vk_walk_records(cp->vbuf, cp->vbytes, (uint64_t)cp->vcount, &cp->w);
    if (!cp->w.ok) {
        check(c, 0, "STRUCT walk: the records of the vid sink do not tile the stored bytes (a rec_bytes below 64, above the longest record or past the end)");
        return -1;
    }
    if (vk_scan_dev(cp->drec, cp->dcount, &cp->lg) != 0) {
        harness_error(c, "the dev log holds more CR or palette writes than the runner records");
        return -1;
    }
    cp->lg.m = res.machine_time;
    return 0;
}

void kind_video(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct vk_cap cp;
    unsigned si;
    unsigned gi;
    unsigned man = 0u;
    unsigned guest = 0u;
    unsigned assumed = 0u;
    unsigned nrows[3] = {0u, 0u, 0u};
    unsigned ngroups = 0u;
    char detail[2048];
    char gline[256];
    size_t used = 0u;

    if (vk_capture(k, c, &cp, got, gotcap) != 0) {
        goto done;
    }
    si = vx_case_scene(cp.vx, k->val[K_ID]);
    if (si == cp.vx->ns || strcmp(cp.vx->sc[si].name, cp.e.scene) != 0) {
        harness_error(c, "no scene row of the vexp rows names this case with the scene of its expect");
        goto done;
    }
    if (cp.vx->sc[si].role == VX_ROLE_GOLDEN) {
        harness_error(c, "the golden role is not judged by this version of the runner");
        goto done;
    }
    {
        struct vk_run run;

        run.name = "main";
        run.w = &cp.w;
        run.lg = &cp.lg;
        detail[0] = '\0';
        for (gi = 0u; gi < cp.vx->ng; gi++) {
            const struct vx_group *g = &cp.vx->gr[gi];
            char why[240];
            char msg[320];
            uint64_t fo;
            unsigned r;
            int v;

            if (strcmp(g->scene, cp.e.scene) != 0) {
                continue;
            }
            if (cp.vx->sc[si].role == VX_ROLE_STRUCT && g->form != VX_FORM_STRUCT) {
                harness_error(c, "a struct-role case judges STRUCT groups only");
                goto done;
            }
            v = vk_eval_group(cp.vx, gi, &run, 1u, 0, &fo, why, sizeof why);
            snprintf(msg, sizeof msg, "group %s (%s): %s", g->gid, g->cls == VX_CLS_MAN ? "MAN" : g->cls == VX_CLS_GUEST ? "GUEST" : "ASSUMED", why);
            if (v == VG_HARNESS) {
                harness_error(c, msg);
                goto done;
            }
            check(c, v == VG_PASS, msg);
            for (r = 0u; r < cp.vx->nr; r++) {
                if (cp.vx->rw[r].group == gi) {
                    if (g->cls == VX_CLS_MAN) {
                        man++;
                    } else if (g->cls == VX_CLS_GUEST) {
                        guest++;
                    } else {
                        assumed++;
                    }
                    nrows[g->form - 1]++;
                }
            }
            ngroups++;
            snprintf(gline, sizeof gline, "video_group %s %s %s\n", g->gid, g->cls == VX_CLS_MAN ? "MAN" : g->cls == VX_CLS_GUEST ? "GUEST" : "ASSUMED", v == VG_PASS ? "pass" : "fail");
            if (used + strlen(gline) + 1u < sizeof detail) {
                memcpy(detail + used, gline, strlen(gline) + 1u);
                used += strlen(gline);
            }
        }
        if (ngroups == 0u) {
            harness_error(c, "the scene of the case has no group: a literal case without a literal group never passes");
            goto done;
        }
    }
    {
        uint8_t digest[DS_SHA256_BYTES];
        char hex[2u * DS_SHA256_BYTES + 1u];
        char head[600];
        uint8_t *stream = vk_stream(cp.vbuf, cp.vbytes);

        if (stream == NULL) {
            harness_error(c, "out of memory for the video stream");
            goto done;
        }
        /* the stream of D-163 and its digest: E0 repeatability, printed beside the literal verdict and never a verdict itself (ER-11) */
        ds_sha256(stream, (size_t)(19u + cp.vbytes), digest);
        free(stream);
        vk_hex(digest, hex);
        snprintf(head, sizeof head, "video_sha256=%s E0 repeatability (a change detector, never a verdict, ER-11)\nvideo_records=%llu video_bytes=%llu machine_time=%llu\nvideo_rows: man=%u guest=%u assumed=%u (class of each group in the .vexp file, ER-10); forms: rows=%u rel=%u struct=%u; groups=%u\n", hex, (unsigned long long)cp.w.n, (unsigned long long)cp.vbytes,
                 (unsigned long long)cp.lg.m, man, guest, assumed, nrows[VX_FORM_ROWS - 1], nrows[VX_FORM_REL - 1], nrows[VX_FORM_STRUCT - 1], ngroups);
        if (strlen(head) + used + 1u <= sizeof detail) {
            memmove(detail + strlen(head), detail, used + 1u);
            memcpy(detail, head, strlen(head));
        }
        set_evidence_detail(detail);
        snprintf(got + strlen(got), gotcap - strlen(got), " records=%llu", (unsigned long long)cp.w.n);
    }
done:
    vk_cap_free(&cp);
}


/* ---- --video-out and kind video_out_rights (plan 04-05; D-162, D-163, T-04-17, T-04-19) ----
 * The runner writes the video stream of a case only for `--video-out CASE_ID`, only for a case of kind video whose fixture is an authored image listed in build/gate/fixtures-authored.used (written by scripts/fixture-build in the same gate run) or a synthetic recipe, only to build/gate/video/ID.dsvr (the canonical stream of D-163) and ID.tsv (one row per line, pixels as run-length groups), paths
 * the runner builds itself from an id of the form [a-z0-9_]+; nothing reaches stdout but ids, counts, sizes and sha256 values. The run is the run of the case (the same image, request, deadline and stop callback, with the vid sink alone: the sink never changes machine state, D-164), so the stream is the stream the case judged. scripts/conformance-run recomputes the SHA-256 of the written .dsvr
 * file with a host tool and compares it with the video_sha256 of the evidence file of the case (the external recomputation of T-04-19). Evidence tier E0. */

#define VO_DIR "build/gate/video/"
#define VO_ID_MAX 64u

const char *video_out_id;

int videokinds_arg(int argc, char **argv, int *a)
{
    if (strcmp(argv[*a], "--video-out") == 0 && *a + 1 < argc) {
        video_out_id = argv[*a + 1];
        *a += 1;
        return 1;
    }
    return 0;
}

enum { VO_OK = 0, VO_BAD_ID = 1, VO_NO_CASE = 2, VO_NOT_PERMITTED = 3, VO_NOT_VIDEO = 4, VO_NO_DEADLINE = 5, VO_FAILED = 6 };

static const char *vo_reason(int r)
{
    switch (r) {
    case VO_BAD_ID:
        return "the id is not of the form [a-z0-9_]+ with at most 64 characters";
    case VO_NO_CASE:
        return "no case carries that id";
    case VO_NOT_PERMITTED:
        return "the fixture of the case is neither an authored image listed in build/gate/fixtures-authored.used nor a synthetic recipe";
    case VO_NOT_VIDEO:
        return "the case is not a video case";
    case VO_NO_DEADLINE:
        return "the case has no until_mclk to run to";
    default:
        return "the video stream could not be produced";
    }
}

static int vo_id_valid(const char *id)
{
    size_t n = strlen(id);

    return n >= 1u && n <= VO_ID_MAX && strspn(id, "abcdefghijklmnopqrstuvwxyz0123456789_") == n;
}

/* the paths the runner derives from a valid id: 0 on success */
static int vo_paths(const char *id, char *dsvr, char *tsv, size_t cap)
{
    int a;
    int b;

    if (!vo_id_valid(id)) {
        return -1;
    }
    a = snprintf(dsvr, cap, VO_DIR "%s.dsvr", id);
    b = snprintf(tsv, cap, VO_DIR "%s.tsv", id);
    return a > 0 && b > 0 && (size_t)a < cap && (size_t)b < cap ? 0 : -1;
}

/* the checks that come before any file is opened: the id, the case, its fixture (T-04-17: authored and listed, or synthetic), its kind and its deadline */
static int vo_request(const char *id, const struct kase **kout, uint64_t *until)
{
    size_t i;

    if (!vo_id_valid(id)) {
        return VO_BAD_ID;
    }
    for (i = 0u; i < case_count(); i++) {
        const struct kase *k = case_get(i);

        if (strcmp(k->val[K_ID], id) == 0) {
            if (!k->has[K_FIXTURE] || !output_fixture_permitted(k->val[K_FIXTURE])) {
                return VO_NOT_PERMITTED;
            }
            if (!k->has[K_KIND] || strcmp(k->val[K_KIND], "video") != 0) {
                return VO_NOT_VIDEO;
            }
            if (!k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], until) != 0 || *until == 0u) {
                return VO_NO_DEADLINE;
            }
            *kout = k;
            return VO_OK;
        }
    }
    return VO_NO_CASE;
}

/* the text form of the stream (D-163): one tab-separated row per record, after one header row of the column names; the pixels of a row are run-length groups COUNT*INDEX/COLOUR (the index array and the colour array of the record, equal neighbours grouped), empty for a record with no active window. Offsets are those of this file. 0 on success */
static int vo_write_tsv(const char *path, const uint8_t *vbuf, uint64_t vbytes, uint64_t count, uint64_t *rows)
{
    FILE *fp = fopen(path, "wb");
    uint64_t off = 0u;
    uint64_t k;
    int ok = fp != NULL;

    *rows = 0u;
    if (fp == NULL) {
        return -1;
    }
    ok = fprintf(fp, "frame\tline\torigin\tdiv\tvce_cr\tborder\tactive_w\tline_dirty\tflags\tpixels\n") > 0;
    for (k = 0u; k < count && ok; k++) {
        const uint8_t *r = vbuf + off;
        uint32_t rb;
        uint32_t aw;
        uint32_t d;
        int first = 1;

        if (off + (uint64_t)VK_HDR > vbytes) {
            ok = 0;
            break;
        }
        rb = vk_u16(r + VK_O_REC_BYTES);
        aw = vk_u16(r + VK_O_ACTIVE_W);
        if (rb < VK_HDR || rb > VK_MAX_REC || off + (uint64_t)rb > vbytes || aw > VK_MAX_DOTS || VK_HDR + 4u * aw > rb) {
            ok = 0;
            break;
        }
        ok = fprintf(fp, "%u\t%u\t%llu\t%u\t%u\t%u\t%u\t%u\t%u\t", (unsigned)vk_u32(r + VK_O_FRAME), (unsigned)vk_u16(r + VK_O_LINE), (unsigned long long)vk_u64(r + VK_O_ORIGIN), (unsigned)r[VK_O_DIV], (unsigned)r[21], (unsigned)vk_u16(r + VK_O_BORDER), (unsigned)aw,
                     (unsigned)r[VK_O_LINE_DIRTY], (unsigned)r[31])
             > 0;
        d = 0u;
        while (d < aw && ok) {
            uint32_t idx = vk_u16(r + VK_HDR + 2u * d);
            uint32_t col = vk_u16(r + VK_HDR + 2u * aw + 2u * d);
            uint32_t run = 1u;

            while (d + run < aw && vk_u16(r + VK_HDR + 2u * (d + run)) == idx && vk_u16(r + VK_HDR + 2u * aw + 2u * (d + run)) == col) {
                run++;
            }
            ok = fprintf(fp, "%s%u*%u/%u", first ? "" : ",", (unsigned)run, (unsigned)idx, (unsigned)col) > 0;
            first = 0;
            d += run;
        }
        ok = ok && fputc('\n', fp) != EOF;
        off += (uint64_t)rb;
        (*rows)++;
    }
    ok = ok && off == vbytes;
    return fclose(fp) == 0 && ok ? 0 : -1;
}

/* the line of one written file: its count (records or rows), size and the SHA-256 of its bytes (data, len) */
static void vo_print(const char *id, const char *what, const char *label, uint64_t n, const uint8_t *data, size_t len)
{
    uint8_t digest[DS_SHA256_BYTES];
    char hex[2u * DS_SHA256_BYTES + 1u];

    ds_sha256(data, len, digest);
    vk_hex(digest, hex);
    emit_out("VIDEO %s %s %s=%llu bytes=%llu sha256=%s\n", id, what, label, (unsigned long long)n, (unsigned long long)len, hex);
}

/* the text file at path (no NUL byte in it): the line of its rows; 0 on success */
static int vo_print_tsv(const char *id, const char *path, uint64_t rows)
{
    char *text = read_file(path);

    if (text == NULL) {
        return -1;
    }
    vo_print(id, "tsv", "rows", rows, (const uint8_t *)text, strlen(text));
    free(text);
    return 0;
}

/* the whole request: validate, run the case's image to its deadline with the vid sink, write the two files; 0 or a VO_ code (nothing is opened before the request is permitted) */
static int vo_do(const char *id)
{
    const struct kase *k = NULL;
    uint64_t until = 0u;
    char dpath[128];
    char tpath[128];
    struct vk_expect e;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_vid_sink *vsk = NULL;
    uint8_t *vbuf = NULL;
    uint8_t *stream = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    uint64_t vcap;
    uint64_t vbytes = 0u;
    uint64_t count = 0u;
    uint64_t rows = 0u;
    const char *err;
    int r = vo_request(id, &k, &until);

    if (r != VO_OK) {
        return r;
    }
    if (vo_paths(id, dpath, tpath, sizeof dpath) != 0) {
        return VO_BAD_ID;
    }
    if (!k->has[K_EXPECT] || vk_parse(k->val[K_EXPECT], &e) != 0) {
        return VO_FAILED;
    }
    vcap = (vk_count_closed_form(until) + 1u) * (uint64_t)VK_MAX_REC;
    if (vcap > (uint64_t)DS_VID_SINK_MAX_BYTES) {
        return VO_FAILED;
    }
    err = boot_built_rom(e.rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        return VO_FAILED;
    }
    vsk = (struct ds_vid_sink *)calloc(1u, sizeof *vsk);
    vbuf = (uint8_t *)calloc((size_t)vcap, 1u);
    if (vsk == NULL || vbuf == NULL) {
        r = VO_FAILED;
    } else {
        vsk->bytes = vbuf;
        vsk->capacity = (uint32_t)vcap;
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.stop = dsv1_done; /* the stop of the case run: the same run, so the same stream */
        rq.vid = vsk;
        if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED || vk_u32(vsk->header + 4) != 0u) {
            r = VO_FAILED;
        } else {
            count = (uint64_t)vk_u32(vsk->header);
            vbytes = vk_u64(vsk->header + 16);
            stream = vk_stream(vbuf, vbytes);
            if (stream == NULL || vbytes > vcap) {
                r = VO_FAILED;
            } else {
                FILE *fp = fopen(dpath, "wb");

                if (fp == NULL || fwrite(stream, 1u, (size_t)(19u + vbytes), fp) != (size_t)(19u + vbytes) || fclose(fp) != 0 || vo_write_tsv(tpath, vbuf, vbytes, count, &rows) != 0) {
                    r = VO_FAILED;
                } else {
                    vo_print(id, "dsvr", "records", count, stream, (size_t)(19u + vbytes));
                    if (vo_print_tsv(id, tpath, rows) != 0) {
                        r = VO_FAILED;
                    }
                }
            }
        }
    }
    free(stream);
    free(vsk);
    free(vbuf);
    release_boot(&b);
    return r;
}

int video_out_main(void)
{
    int r = vo_do(video_out_id);

    if (r != VO_OK) {
        emit_err("HARNESS --video-out refused: %s\n", vo_reason(r));
        return 2;
    }
    return 0;
}

/* a file at path exists and can be opened for reading */
static int vo_file_exists(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/*
 * kind video_out_rights: expect is a list of items separated by ';', each value at most 40 characters, at least one of each of the five kinds (the lists of trace_out_rights, cloned):
 *   allow_fixture=KEY  the fixture key is permitted (authored and listed, or a synthetic recipe)      deny_fixture=KEY  it is not (an unlisted authored image, none, a path, a commercial name)
 *   allow_case=ID      the request for the case passes every check that comes before a file is opened, and the paths are build/gate/video/ID.dsvr and .tsv with no further slash
 *   deny_case=ID       the request is refused (a case whose fixture is not permitted, a case that is not a video case, or no such case) and no file appears        bad_id=ID  the id is not of the form [a-z0-9_]+ and is refused with no path built and no file at any naive join
 * Nothing is written by this kind: the writing of an allowed request is the --video-out run of scripts/conformance-run, whose digest it recomputes with a host tool. E0: the rule is a literal one of this plan (T-04-17).
 */
void kind_video_out_rights(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned classes = 0u;
    unsigned n = 0u;
    char msg[160];

    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "the video_out_rights expect list is too long");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        const char *v;
        char dpath[128];
        char tpath[128];

        if (eq == NULL || strlen(eq + 1) > 40u) {
            harness_error(c, "a video_out_rights item is KEY=VALUE with a value of at most 40 characters");
            return;
        }
        *eq = '\0';
        v = eq + 1;
        n++;
        if (strcmp(item, "allow_fixture") == 0) {
            classes |= 1u;
            snprintf(msg, sizeof msg, "the fixture key %s is refused but is authored and listed or a synthetic recipe", v);
            check(c, output_fixture_permitted(v), msg);
        } else if (strcmp(item, "deny_fixture") == 0) {
            classes |= 2u;
            snprintf(msg, sizeof msg, "the fixture key %s is permitted but is neither authored and listed nor a synthetic recipe", v);
            check(c, !output_fixture_permitted(v), msg);
        } else if (strcmp(item, "allow_case") == 0) {
            const struct kase *kk = NULL;
            uint64_t until = 0u;
            int ok;

            classes |= 4u;
            snprintf(msg, sizeof msg, "the video request for case %s is refused before any file is opened", v);
            check(c, vo_request(v, &kk, &until) == VO_OK, msg);
            ok = vo_paths(v, dpath, tpath, sizeof dpath) == 0 && strncmp(dpath, VO_DIR, strlen(VO_DIR)) == 0 && strncmp(tpath, VO_DIR, strlen(VO_DIR)) == 0 && strchr(dpath + strlen(VO_DIR), '/') == NULL && strchr(tpath + strlen(VO_DIR), '/') == NULL
                 && strstr(dpath, "..") == NULL && strstr(tpath, "..") == NULL;
            snprintf(msg, sizeof msg, "the paths of case %s are not directly under build/gate/video/", v);
            check(c, ok, msg);
        } else if (strcmp(item, "deny_case") == 0) {
            int before;
            int r1;

            classes |= 8u;
            snprintf(dpath, sizeof dpath, VO_DIR "%s.dsvr", v);
            snprintf(tpath, sizeof tpath, VO_DIR "%s.tsv", v);
            before = vo_file_exists(dpath) || vo_file_exists(tpath); /* a file of an earlier explicit request (or of a case-folded name on a case-insensitive filesystem) may be there: the refusal must not create one */
            r1 = vo_do(v);
            snprintf(msg, sizeof msg, "the video request for case %s was not refused with the not-permitted, not-video or no-such-case reason", v);
            check(c, r1 == VO_NOT_PERMITTED || r1 == VO_NOT_VIDEO || r1 == VO_NO_CASE, msg);
            snprintf(msg, sizeof msg, "a refused video request for case %s left a file", v);
            check(c, before || (!vo_file_exists(dpath) && !vo_file_exists(tpath)), msg);
        } else if (strcmp(item, "bad_id") == 0) {
            char dpath2[128];
            int before;

            classes |= 16u;
            snprintf(dpath, sizeof dpath, VO_DIR "%s.dsvr", v);
            snprintf(dpath2, sizeof dpath2, "%s.dsvr", v);
            snprintf(tpath, sizeof tpath, VO_DIR "%s.tsv", v);
            before = vo_file_exists(dpath) || vo_file_exists(dpath2) || vo_file_exists(tpath);
            snprintf(msg, sizeof msg, "the id '%s' was accepted or a path was built for it", v);
            check(c, !vo_id_valid(v) && vo_paths(v, dpath, tpath, sizeof dpath) != 0 && vo_do(v) == VO_BAD_ID, msg);
            snprintf(dpath, sizeof dpath, VO_DIR "%s.dsvr", v);
            snprintf(msg, sizeof msg, "a refused id '%s' left a file at a naive join of the id", v);
            check(c, before || (!vo_file_exists(dpath) && !vo_file_exists(dpath2) && !vo_file_exists(tpath)), msg);
        } else {
            harness_error(c, "a video_out_rights item is allow_fixture, deny_fixture, allow_case, deny_case or bad_id");
            return;
        }
    }
    if (classes != 31u) {
        harness_error(c, "a video_out_rights case needs at least one item of each of the five kinds");
        return;
    }
    snprintf(got, gotcap, "items=%u", n);
}
