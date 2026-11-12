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
 * STRUCT rows (selector check=NAME), the checks written here: count, origins, frame_lines, records_per_frame, rec_bytes, values_below_512, div_table, div_window, window (plan 04-09), div_first, border_after_pal, line_dirty, display_lines, width_set.
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
#define VK_O_ACTIVE_X 22u
#define VK_O_ACTIVE_W 24u
#define VK_O_BORDER 26u
#define VK_O_FLAGS 31u
#define VK_O_LINE_DIRTY 32u
#define VK_FLAG_ACTIVE 0x01u
#define VK_FLAG_CLIPPED 0x20u
#define VK_O_VERSION 35u
#define VK_O_RESERVED 56u
#define VK_MAX_DOTS 682u
#define VK_MAX_REC (VK_HDR + 4u * VK_MAX_DOTS + 8u)
#define VK_GRID 1365u
#define VK_DEV_CAP 16384u
#define VK_CRW_MAX 12u
#define VK_HV_MAX 16u /* the HSR and HDR register writes the runner records (plan 04-09) */
#define VK_PHYS_WIN 0x1FE400u /* the base of the VCE window; the register of an access is the offset & 7 (MAN H6-2), so $1FE408 is a mirror of CR and $1FE444 a mirror of CTW low */
#define VK_PHYS_WIN_END 0x1FE800u
#define VK_PHYS_VDC 0x1FE000u /* the VDC window: the register of an access is the offset & 3 (A1 and A0, MAN H8-24): 0 = AR, 2 = data LSB, 3 = data MSB */
#define VK_PHYS_VDC_END 0x1FE400u
#define VK_VDC_AR_HSR 10u
#define VK_VDC_AR_HDR 11u
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

static const char *vx_resolve(struct vx_file *f);

/* the file build/gate/fixtures/ROM.vexp.tsv into f (cleared first); NULL on success. A missing file is a fault of the harness, never an empty scene list: the video kind is judged by these rows only */
static const char *vx_load(const char *rom, struct vx_file *f)
{
    char path[96];
    char *text;
    char *line;
    unsigned nrows = 0u;
    const char *err = NULL;

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
    return vx_resolve(f);
}

/* every row names a group and every group a scene; the group index of each row is set. NULL on success */
static const char *vx_resolve(struct vx_file *f)
{
    unsigned i;

    if (f->ns == 0u) {
        return "vexp-malformed: the vexp rows hold no scene row";
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
    uint64_t t_hv[VK_HV_MAX]; /* the stamps of the data MSB writes of HSR and HDR (the write that completes a register write), in order; the VDC window is joined by AR as the guest set it */
    unsigned n_hv;
    uint64_t m; /* the machine time the run reached */
};

/* the CR writes, the palette data writes and the HSR and HDR register writes of the dev log, found by the register of the physical address (VCE window: offset & 7, MAN H6-2, mirrors included; VDC window: offset & 3, MAN H8-24, with AR as the guest last set it) and by kind (the stamps are the join points of the rows); 0 when the log is well formed. The physical writes of ST0 to ST2 are logged as DS_DEV_ST, the others as DS_DEV_WRITE */
static int vk_scan_dev(const uint8_t *recs, uint32_t count, struct vk_log *lg)
{
    uint32_t i;
    unsigned ar = 0u;

    memset(lg, 0, sizeof *lg);
    for (i = 0u; i < count; i++) {
        const uint8_t *r = recs + (size_t)i * VK_DEV_REC;
        uint32_t phys = vk_u32(r + 8);

        if ((unsigned)r[13] != (unsigned)DS_DEV_WRITE && (unsigned)r[13] != (unsigned)DS_DEV_ST) {
            continue;
        }
        if (phys >= (uint32_t)VK_PHYS_VDC && phys < (uint32_t)VK_PHYS_VDC_END) {
            if ((phys & 3u) == 0u) {
                ar = (unsigned)r[12] & 0x1Fu;
            } else if ((phys & 3u) == 3u && (ar == VK_VDC_AR_HSR || ar == VK_VDC_AR_HDR)) {
                if (lg->n_hv >= VK_HV_MAX) {
                    return -1;
                }
                lg->t_hv[lg->n_hv++] = vk_u64(r);
            }
            continue;
        }
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
#define VK_KV_MAX 12u

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

static uint32_t vk_dot(const uint8_t *rec, uint32_t j, int which);

/* the record with origin o lies wholly inside the window of CR write j: at least one line after the stamp of write j and, when write j + 1 exists, ending a line before it (origin + 2 lines <= the next stamp), so it depends neither on the grid phase nor on the tie rule and never on the power-on CR */
static int vk_in_window(const struct vk_rc *rc, unsigned j, uint64_t o)
{
    if (o < rc->lg->t_cr[j] + (uint64_t)VK_GRID) {
        return 0;
    }
    return !(j + 1u < rc->lg->n_cr && o + 2u * (uint64_t)VK_GRID > rc->lg->t_cr[j + 1u]);
}

/* the CR writes of the log are exactly the values cw[0..nc) and at least gap clocks apart (an empty window is a fault of the case, not a pass); 0 when they are, else the harness verdict is already set */
static int vk_crw_match(struct vk_rc *rc, const unsigned *cw, unsigned nc, uint64_t gap)
{
    unsigned j;

    if (rc->lg->n_cr != nc) {
        return vk_bad(rc, "the dev log does not hold exactly the CR writes the row lists");
    }
    for (j = 0u; j < nc; j++) {
        if (rc->lg->v_cr[j] != cw[j] || (j > 0u && rc->lg->t_cr[j] - rc->lg->t_cr[j - 1u] < gap)) {
            return vk_bad(rc, "a CR write of the guest has another value than the row lists, or two CR writes are closer than its gap (the window of judged records could be empty)");
        }
    }
    return 0;
}

/* the divider of the records wholly inside the window of each CR write j whose bit is set in judge (MAN H6-3 for the contents 00h and 01h, ASSUMED LED-VID-0002 for 02h and 03h: the class of the group says which); the window of a judged write must hold a record */
static int vk_div_windows(struct vk_rc *rc, const unsigned *dv, const unsigned *cw, unsigned nc, unsigned judge)
{
    unsigned j;

    for (j = 0u; j < nc; j++) {
        uint64_t k;
        uint64_t judged = 0u;

        if (((judge >> j) & 1u) == 0u) {
            continue;
        }
        for (k = 0u; k < rc->w->n; k++) {
            if (!vk_in_window(rc, j, vk_u64(rc->w->rec[k] + VK_O_ORIGIN))) {
                continue;
            }
            if (judged == 0u) {
                vk_focus(rc, k);
            }
            judged++;
            if ((unsigned)rc->w->rec[k][VK_O_DIV] != dv[j]) {
                snprintf(rc->why, rc->cap, "div: record %llu, at least one line after CR write %u (value %u) and before the next one, does not carry divider %u", (unsigned long long)k, j + 1u, cw[j], dv[j]);
                return VG_FAIL;
            }
        }
        if (judged == 0u) {
            return vk_bad(rc, "the window of a CR write holds no record: the case cannot judge its divider");
        }
    }
    return VG_PASS;
}

/* STRUCT div_table (MAN H6-3): the records wholly inside the window of every CR write j carry the divider div[j]; the log must hold exactly the CR writes crw lists, at least gap clocks apart */
static int vk_s_div_table(struct vk_rc *rc)
{
    static const char *const keys[] = {"div", "crw", "gap"};
    unsigned dv[VK_CRW_MAX];
    unsigned cw[VK_CRW_MAX];
    unsigned nd = 0u;
    unsigned nc = 0u;
    uint64_t gap = 0u;

    if (vk_need(rc, keys, 3u) != 0 || vk_lst(rc, "div", dv, VK_CRW_MAX, &nd) != 0 || vk_lst(rc, "crw", cw, VK_CRW_MAX, &nc) != 0 || nd != nc || vk_num(rc, "gap", &gap) != 0) {
        return vk_bad(rc, "STRUCT div_table needs the items div=D,D, crw=V,V (the same count) and gap=N");
    }
    if (vk_crw_match(rc, cw, nc, gap) != 0) {
        return VG_HARNESS;
    }
    return vk_div_windows(rc, dv, cw, nc, (1u << nc) - 1u);
}

/* STRUCT div_window: as div_table, but only the windows of the CR writes listed in judge=J,J are judged (the others are still checked to be the writes the row lists): the contents 00h and 01h of one group are MAN H6-3, the contents 02h and 03h of another are ASSUMED LED-VID-0002 */
static int vk_s_div_window(struct vk_rc *rc)
{
    static const char *const keys[] = {"div", "crw", "gap", "judge"};
    unsigned dv[VK_CRW_MAX];
    unsigned cw[VK_CRW_MAX];
    unsigned jl[VK_CRW_MAX];
    unsigned nd = 0u;
    unsigned nc = 0u;
    unsigned nj = 0u;
    unsigned mask = 0u;
    unsigned j;
    uint64_t gap = 0u;

    if (vk_need(rc, keys, 4u) != 0 || vk_lst(rc, "div", dv, VK_CRW_MAX, &nd) != 0 || vk_lst(rc, "crw", cw, VK_CRW_MAX, &nc) != 0 || nd != nc || vk_lst(rc, "judge", jl, VK_CRW_MAX, &nj) != 0 || vk_num(rc, "gap", &gap) != 0) {
        return vk_bad(rc, "STRUCT div_window needs the items div=D,D, crw=V,V (the same count), gap=N and judge=J,J");
    }
    for (j = 0u; j < nj; j++) {
        if (jl[j] >= nc) {
            return vk_bad(rc, "STRUCT div_window judges a CR write the row does not list");
        }
        mask |= 1u << jl[j];
    }
    if (vk_crw_match(rc, cw, nc, gap) != 0) {
        return VG_HARNESS;
    }
    return vk_div_windows(rc, dv, cw, nc, mask);
}

/* STRUCT window (ASSUMED LED-VID-0005, D-140, D-161): inside the window of CR write crw every display line (flag ACTIVE) carries the divider div, active_x, active_w, rec_bytes, the CLIPPED flag clipped and a payload of active_w index words equal to index and active_w colour words equal to the border of its own record, and every other line is header-only (active_w 0, active_x 0, rec_bytes 64, no CLIPPED). At least one display line must be judged */
static int vk_s_window(struct vk_rc *rc)
{
    static const char *const keys[] = {"crw", "div", "active_x", "active_w", "rec_bytes", "clipped", "index"};
    uint64_t v[7];
    uint64_t k;
    uint64_t judged = 0u;
    unsigned i;

    for (i = 0u; i < 7u; i++) {
        if (vk_need(rc, keys, 7u) != 0 || vk_num(rc, keys[i], &v[i]) != 0) {
            return vk_bad(rc, "STRUCT window needs the items crw, div, active_x, active_w, rec_bytes, clipped and index (decimal)");
        }
    }
    if (v[0] >= (uint64_t)rc->lg->n_cr || v[5] > 1u || v[4] != (uint64_t)VK_HDR + ((4u * v[3] + 7u) & ~(uint64_t)7u)) {
        return vk_bad(rc, "STRUCT window names a CR write the log does not hold, a clipped other than 0 or 1, or a rec_bytes that is not 64 + ((4 * active_w + 7) & ~7)");
    }
    for (k = 0u; k < rc->w->n; k++) {
        const uint8_t *r = rc->w->rec[k];
        uint32_t aw = vk_u16(r + VK_O_ACTIVE_W);
        uint32_t j;
        int ok;

        if (!vk_in_window(rc, (unsigned)v[0], vk_u64(r + VK_O_ORIGIN))) {
            continue;
        }
        if ((r[VK_O_FLAGS] & VK_FLAG_ACTIVE) == 0u) { /* a blanking line is header-only */
            if (aw != 0u || vk_u16(r + VK_O_ACTIVE_X) != 0u || vk_u16(r + VK_O_REC_BYTES) != VK_HDR || (r[VK_O_FLAGS] & VK_FLAG_CLIPPED) != 0u) {
                snprintf(rc->why, rc->cap, "window (ASSUMED LED-VID-0005): record %llu, a line outside the display, is not header-only (active_w, active_x, rec_bytes 64, no CLIPPED)", (unsigned long long)k);
                vk_focus(rc, k);
                return VG_FAIL;
            }
            continue;
        }
        if (judged == 0u) {
            vk_focus(rc, k);
        }
        judged++;
        ok = (uint64_t)r[VK_O_DIV] == v[1] && (uint64_t)vk_u16(r + VK_O_ACTIVE_X) == v[2] && (uint64_t)aw == v[3] && (uint64_t)vk_u16(r + VK_O_REC_BYTES) == v[4] && (uint64_t)((r[VK_O_FLAGS] & VK_FLAG_CLIPPED) != 0u ? 1u : 0u) == v[5];
        for (j = 0u; j < aw && ok; j++) {
            ok = (uint64_t)vk_dot(r, j, 0) == v[6] && vk_dot(r, j, 1) == vk_u16(r + VK_O_BORDER);
        }
        if (!ok) {
            snprintf(rc->why, rc->cap, "window (ASSUMED LED-VID-0005): display record %llu, inside the window of CR write %llu, does not carry div %llu, active_x %llu, active_w %llu, rec_bytes %llu, CLIPPED %llu and a blank payload (index %llu, colour = border)", (unsigned long long)k, (unsigned long long)v[0] + 1u, (unsigned long long)v[1], (unsigned long long)v[2], (unsigned long long)v[3], (unsigned long long)v[4], (unsigned long long)v[5], (unsigned long long)v[6]);
            return VG_FAIL;
        }
    }
    return judged >= 1u ? VG_PASS : vk_bad(rc, "the window of the CR write holds no display line: the case cannot judge the geometry");
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

/* STRUCT display_lines (ASSUMED LED-VID-0005, D-143): every frame but the last has min(vdw + 1, frame_lines - (vds + 2)) records with an active window */
static int vk_s_display_lines(struct vk_rc *rc)
{
    static const char *const keys[] = {"vdw", "vds", "frame_lines"};
    uint64_t vdw = 0u;
    uint64_t vds = 0u;
    uint64_t fl = 0u;
    uint64_t want;
    uint64_t k;
    uint64_t run = 0u;
    uint64_t complete = 0u;
    uint32_t cur;

    if (vk_need(rc, keys, 3u) != 0 || vk_num(rc, "vdw", &vdw) != 0 || vk_num(rc, "vds", &vds) != 0 || vk_num(rc, "frame_lines", &fl) != 0 || fl == 0u || fl > 1000u || vdw > 1000u || vds > 1000u) {
        return vk_bad(rc, "STRUCT display_lines needs the items vdw=N, vds=N and frame_lines=N");
    }
    want = vds + 2u >= fl ? 0u : fl - (vds + 2u);
    if (vdw + 1u < want) {
        want = vdw + 1u;
    }
    vk_focus(rc, rc->w->n / 2u);
    cur = vk_u32(rc->w->rec[0] + VK_O_FRAME);
    for (k = 0u; k < rc->w->n; k++) {
        uint32_t f = vk_u32(rc->w->rec[k] + VK_O_FRAME);

        if (f != cur) {
            if (run != want) {
                snprintf(rc->why, rc->cap, "STRUCT display_lines: frame %u holds %llu active lines, min(VDW + 1, frame_lines - (VDS + 2)) is %llu", (unsigned)cur, (unsigned long long)run, (unsigned long long)want);
                return VG_FAIL;
            }
            complete++;
            cur = f;
            run = 0u;
        }
        if (vk_u16(rc->w->rec[k] + VK_O_ACTIVE_W) > 0u) {
            run++;
        }
    }
    return complete >= 1u ? VG_PASS : vk_bad(rc, "STRUCT display_lines: the stream holds no complete frame");
}

/* STRUCT width_set: the set of active widths over the run has at least min_distinct elements (a mid-frame change of the window shows as a second width) */
static int vk_s_width_set(struct vk_rc *rc)
{
    static const char *const keys[] = {"min_distinct"};
    uint64_t want = 0u;
    uint32_t seen[16];
    unsigned ns = 0u;
    uint64_t k;

    if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "min_distinct", &want) != 0 || want == 0u || want > 16u) {
        return vk_bad(rc, "STRUCT width_set needs the single item min_distinct=N (1 to 16)");
    }
    vk_focus(rc, rc->w->n / 2u);
    for (k = 0u; k < rc->w->n; k++) {
        uint32_t aw = vk_u16(rc->w->rec[k] + VK_O_ACTIVE_W);
        unsigned j;

        for (j = 0u; j < ns && seen[j] != aw; j++) {
        }
        if (j == ns && ns < 16u) {
            seen[ns++] = aw;
        }
    }
    if ((uint64_t)ns < want) {
        snprintf(rc->why, rc->cap, "STRUCT width_set: the run holds %u distinct active widths, at least %llu are expected", ns, (unsigned long long)want);
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
    {"div_window", vk_s_div_window},
    {"window", vk_s_window},
    {"div_first", vk_s_div_first},
    {"border_after_pal", vk_s_border},
    {"line_dirty", vk_s_line_dirty},
    {"display_lines", vk_s_display_lines},
    {"width_set", vk_s_width_set},
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

/* ---- ROWS (D-171): the dots X0 to X1 of the K-th active line, a list of palette addresses (index) or of colour values (value) ---- */

/* the list of items V or COUNT*V separated by commas into out (at most cap values, each below 65536), the count in *n; 0 on success */
static int vk_expand(const char *text, uint32_t *out, unsigned cap, unsigned *n)
{
    char buf[VX_LIT];
    char *rest = buf;
    char *item;

    *n = 0u;
    if (strlen(text) >= sizeof buf) {
        return -1;
    }
    memcpy(buf, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        uint64_t rep = 1u;
        uint64_t val = 0u;
        uint64_t r;
        char *star = strchr(item, '*');

        if (star != NULL) {
            *star = '\0';
            if (strlen(item) == 0u || strlen(item) > 5u || strspn(item, "0123456789") != strlen(item) || parse_u64(item, &rep) != 0 || rep == 0u || rep > (uint64_t)cap) {
                return -1;
            }
            item = star + 1;
        }
        if (strlen(item) == 0u || strlen(item) > 5u || strspn(item, "0123456789") != strlen(item) || parse_u64(item, &val) != 0 || val > 65535u || rep > (uint64_t)(cap - *n)) {
            return -1;
        }
        for (r = 0u; r < rep; r++) {
            out[(*n)++] = (uint32_t)val;
        }
    }
    return *n >= 1u ? 0 : -1;
}

/* the active lines of a run: the records with an active window in the first frame that has one, in order; the count (at most cap) */
static unsigned vk_active_lines(const struct vk_walk *w, uint64_t *out, unsigned cap)
{
    uint64_t k;
    unsigned n = 0u;
    uint32_t frame = 0u;
    int have = 0;

    for (k = 0u; k < w->n; k++) {
        if (vk_u16(w->rec[k] + VK_O_ACTIVE_W) == 0u) {
            continue;
        }
        if (!have) {
            have = 1;
            frame = vk_u32(w->rec[k] + VK_O_FRAME);
        }
        if (vk_u32(w->rec[k] + VK_O_FRAME) != frame || n >= cap) {
            break;
        }
        out[n++] = k;
    }
    return n;
}

/* the index (which = 0) or the colour value (which = 1) of dot j of a record */
static uint32_t vk_dot(const uint8_t *rec, uint32_t j, int which)
{
    uint32_t aw = vk_u16(rec + VK_O_ACTIVE_W);

    return vk_u16(rec + VK_HDR + (which != 0 ? 2u * aw : 0u) + 2u * j);
}

/* the value after the prefix, or NULL, and the number it starts with (up to 9 digits), the end of the digits in *end */
static int vk_scan_u(const char *p, uint64_t *out, const char **end)
{
    size_t n = strspn(p, "0123456789");

    if (n == 0u || n > 9u) {
        return -1;
    }
    *out = 0u;
    while (n-- > 0u) {
        *out = *out * 10u + (uint64_t)(*p++ - '0');
    }
    *end = p;
    return 0;
}

static int vk_rows_row(struct vk_rc *rc, const struct vx_row *r, int perturb)
{
    uint64_t kk = 0u;
    uint64_t x0 = 0u;
    uint64_t x1 = 0u;
    const char *p = r->sel;
    const char *lv;
    uint32_t want[VK_MAX_DOTS];
    uint32_t dseen[8];
    unsigned nw = 0u;
    unsigned ndist = 0u;
    unsigned ntr = 0u;
    unsigned j;
    unsigned which;
    uint64_t lines[512];
    unsigned nl;
    const uint8_t *rec;
    uint32_t aw;

    if (strncmp(p, "line=active+", 12u) != 0 || vk_scan_u(p + 12, &kk, &p) != 0 || strncmp(p, ";x=", 3u) != 0 || vk_scan_u(p + 3, &x0, &p) != 0 || strncmp(p, "..", 2u) != 0 || vk_scan_u(p + 2, &x1, &p) != 0 || *p != '\0') {
        return vk_bad(rc, "a ROWS row has the selector line=active+K;x=X0..X1");
    }
    if (strncmp(r->lit, "index=", 6u) == 0) {
        which = 0u;
        lv = r->lit + 6;
    } else if (strncmp(r->lit, "value=", 6u) == 0) {
        which = 1u;
        lv = r->lit + 6;
    } else {
        return vk_bad(rc, "a ROWS row has the literal index=LIST or value=LIST");
    }
    if (vk_expand(lv, want, VK_MAX_DOTS, &nw) != 0 || x1 < x0 || x1 >= (uint64_t)VK_MAX_DOTS || (uint64_t)nw != x1 - x0 + 1u) {
        return vk_bad(rc, "a ROWS literal is a list of V or COUNT*V items with one value for each dot of its x range");
    }
    for (j = 0u; j < nw; j++) { /* the non-vacuity of D-171: at least 3 distinct values and a transition, or an all-border or one-colour expectation would pass a null renderer */
        unsigned m;

        for (m = 0u; m < ndist && dseen[m] != want[j]; m++) {
        }
        if (m == ndist && ndist < 8u) {
            dseen[ndist++] = want[j];
        }
        if (j > 0u && want[j] != want[j - 1u]) {
            ntr++;
        }
    }
    if (ndist < 3u || ntr < 1u) {
        return vk_bad(rc, "VIDEO_ROWS_VACUOUS: a ROWS literal needs at least 3 distinct values and one transition");
    }
    if (perturb) {
        want[0]++; /* F0: the first expected value of the row is made wrong */
    }
    nl = vk_active_lines(rc->w, lines, 512u);
    if (kk >= (uint64_t)nl) {
        snprintf(rc->why, rc->cap, "ROWS: the run has %u active lines and the row asks for active+%llu", nl, (unsigned long long)kk);
        vk_focus(rc, rc->w->n / 2u);
        return VG_FAIL;
    }
    vk_focus(rc, lines[kk]);
    rec = rc->w->rec[lines[kk]];
    aw = vk_u16(rec + VK_O_ACTIVE_W);
    if (x1 >= (uint64_t)aw) {
        snprintf(rc->why, rc->cap, "ROWS: the dots %llu to %llu are not all inside the active window of active+%llu (width %u)", (unsigned long long)x0, (unsigned long long)x1, (unsigned long long)kk, (unsigned)aw);
        return VG_FAIL;
    }
    for (j = 0u; j < nw; j++) {
        if (vk_dot(rec, (uint32_t)x0 + j, (int)which) != want[j]) {
            snprintf(rc->why, rc->cap, "ROWS: dot %llu of active+%llu has the %s %u, the row says %u", (unsigned long long)x0 + j, (unsigned long long)kk, which == 0u ? "index" : "value", (unsigned)vk_dot(rec, (uint32_t)x0 + j, (int)which), (unsigned)want[j]);
            return VG_FAIL;
        }
    }
    return VG_PASS;
}

/* ---- REL (D-171): a relation between two named runs, A the reference and B the variant ---- */

static const struct vk_run *vk_run_named(const struct vk_run *runs, unsigned nruns, const char *name)
{
    unsigned i;

    for (i = 0u; i < nruns; i++) {
        if (strcmp(runs[i].name, name) == 0) {
            return &runs[i];
        }
    }
    return NULL;
}

/* two rows of two runs differ at some dot (index or value) */
static int vk_row_differs(const uint8_t *ra, uint32_t xa, const uint8_t *rb, uint32_t xb)
{
    return vk_dot(ra, xa, 0) != vk_dot(rb, xb, 0) || vk_dot(ra, xa, 1) != vk_dot(rb, xb, 1);
}

static int vk_fail_rel(struct vk_rc *rc, const char *rel, const char *what)
{
    snprintf(rc->why, rc->cap, "REL %s: %s", rel, what);
    return VG_FAIL;
}

static int vk_rel_row(struct vk_rc *rc, const struct vx_row *r, const struct vk_run *runs, unsigned nruns, int perturb)
{
    char lit[VX_LIT + 8u];
    char rel[24];
    char names[72];
    char *comma;
    const struct vk_run *ra;
    const struct vk_run *rb;
    uint64_t la[512];
    uint64_t lb[512];
    unsigned na;
    unsigned nb;
    unsigned i;
    uint64_t v1 = 0u;
    uint64_t v2 = 0u;
    int sel;

    if (vk_kv_parse(r->sel, rc) != 0 || rc->nkv != 2u || strcmp(rc->kv[0].k, "rel") != 0 || strcmp(rc->kv[1].k, "runs") != 0 || strlen(rc->kv[0].v) >= sizeof rel) {
        return vk_bad(rc, "a REL row has the selector rel=NAME;runs=A,B");
    }
    memcpy(rel, rc->kv[0].v, strlen(rc->kv[0].v) + 1u);
    memcpy(names, rc->kv[1].v, strlen(rc->kv[1].v) + 1u);
    comma = strchr(names, ',');
    if (comma == NULL) {
        return vk_bad(rc, "a REL row names two runs: runs=A,B");
    }
    *comma = '\0';
    ra = vk_run_named(runs, nruns, names);
    rb = vk_run_named(runs, nruns, comma + 1);
    if (ra == NULL || rb == NULL) {
        return vk_bad(rc, "a REL row names a run that the case does not have");
    }
    memcpy(lit, r->lit, strlen(r->lit) + 1u);
    if (perturb && vk_perturb(lit, sizeof lit) != 0) {
        return vk_bad(rc, "the literal of a REL row holds no number to perturb");
    }
    if (vk_kv_parse(lit, rc) != 0) {
        return vk_bad(rc, "the literal of a REL row is not key=value items separated by semicolons");
    }
    na = vk_active_lines(ra->w, la, 512u);
    nb = vk_active_lines(rb->w, lb, 512u);
    if (na == 0u || nb == 0u) {
        snprintf(rc->why, rc->cap, "REL %s: a run holds no active line (a null renderer never passes)", rel);
        return VG_FAIL;
    }
    vk_focus(rc, lb[0]);
    sel = strcmp(rel, "scroll_x_shift") == 0 ? 1 : strcmp(rel, "scroll_y_shift") == 0 ? 2 : strcmp(rel, "palette_subst") == 0 ? 3 : strcmp(rel, "blank_is_border") == 0 ? 4 : strcmp(rel, "dot_width_scale") == 0 ? 5 : 0;
    if (sel == 1) { /* B shows the row of A moved left by dx dots: B[x] = A[x + dx] for every dot that stays inside */
        static const char *const keys[] = {"dx"};
        unsigned observable = 0u;

        if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "dx", &v1) != 0 || v1 == 0u || na != nb) {
            return vk_bad(rc, "REL scroll_x_shift needs dx=N (at least 1) and runs with the same number of active lines");
        }
        for (i = 0u; i < na; i++) {
            uint32_t aw = vk_u16(ra->w->rec[la[i]] + VK_O_ACTIVE_W);
            uint32_t x;

            if (vk_u16(rb->w->rec[lb[i]] + VK_O_ACTIVE_W) != aw || v1 >= (uint64_t)aw) {
                snprintf(rc->why, rc->cap, "REL scroll_x_shift: active line %u has another width in B than in A, or dx is not below the width", i);
                return VG_FAIL;
            }
            for (x = 0u; x + (uint32_t)v1 < aw; x++) {
                if (vk_row_differs(rb->w->rec[lb[i]], x, ra->w->rec[la[i]], x + (uint32_t)v1)) {
                    snprintf(rc->why, rc->cap, "REL scroll_x_shift: dot %u of active line %u in B is not dot %u of A", x, i, x + (unsigned)v1);
                    return VG_FAIL;
                }
                if (vk_row_differs(ra->w->rec[la[i]], x + (uint32_t)v1, ra->w->rec[la[i]], x)) {
                    observable = 1u;
                }
            }
        }
        return observable ? VG_PASS : vk_bad(rc, "REL scroll_x_shift: run A is constant along x, so a shift cannot be observed");
    }
    if (sel == 2) { /* B line L shows A line L + dy */
        static const char *const keys[] = {"dy"};
        unsigned observable = 0u;

        if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "dy", &v1) != 0 || v1 == 0u || na != nb || v1 >= (uint64_t)na) {
            return vk_bad(rc, "REL scroll_y_shift needs dy=N (at least 1, below the active lines) and runs with the same number of active lines");
        }
        for (i = 0u; i + (unsigned)v1 < na; i++) {
            uint32_t aw = vk_u16(rb->w->rec[lb[i]] + VK_O_ACTIVE_W);
            uint32_t x;

            if (vk_u16(ra->w->rec[la[i + (unsigned)v1]] + VK_O_ACTIVE_W) != aw) {
                snprintf(rc->why, rc->cap, "REL scroll_y_shift: active line %u of B has another width than active line %u of A", i, i + (unsigned)v1);
                return VG_FAIL;
            }
            for (x = 0u; x < aw; x++) {
                if (vk_row_differs(rb->w->rec[lb[i]], x, ra->w->rec[la[i + (unsigned)v1]], x)) {
                    snprintf(rc->why, rc->cap, "REL scroll_y_shift: dot %u of active line %u in B is not that of active line %u in A", x, i, i + (unsigned)v1);
                    return VG_FAIL;
                }
                if (vk_row_differs(ra->w->rec[la[i + (unsigned)v1]], x, ra->w->rec[la[i]], x)) {
                    observable = 1u;
                }
            }
        }
        return observable ? VG_PASS : vk_bad(rc, "REL scroll_y_shift: the lines of run A are all alike, so a shift cannot be observed");
    }
    if (sel == 3) { /* B is A with the colour value from replaced by to, every index unchanged */
        static const char *const keys[] = {"from", "to"};
        unsigned hit = 0u;

        if (vk_need(rc, keys, 2u) != 0 || vk_num(rc, "from", &v1) != 0 || vk_num(rc, "to", &v2) != 0 || na != nb || v1 == v2) {
            return vk_bad(rc, "REL palette_subst needs from=V and to=W (different) and runs with the same number of active lines");
        }
        for (i = 0u; i < na; i++) {
            uint32_t aw = vk_u16(ra->w->rec[la[i]] + VK_O_ACTIVE_W);
            uint32_t x;

            if (vk_u16(rb->w->rec[lb[i]] + VK_O_ACTIVE_W) != aw) {
                return vk_fail_rel(rc, "palette_subst", "a line has another width in B than in A");
            }
            for (x = 0u; x < aw; x++) {
                uint32_t ca = vk_dot(ra->w->rec[la[i]], x, 1);
                uint32_t cb = vk_dot(rb->w->rec[lb[i]], x, 1);

                if (vk_dot(ra->w->rec[la[i]], x, 0) != vk_dot(rb->w->rec[lb[i]], x, 0) || cb != (ca == (uint32_t)v1 ? (uint32_t)v2 : ca)) {
                    return vk_fail_rel(rc, "palette_subst", "a dot of B is not that of A with the colour replaced (or an index changed)");
                }
                if (ca == (uint32_t)v1) {
                    hit = 1u;
                }
            }
        }
        return hit ? VG_PASS : vk_bad(rc, "REL palette_subst: run A holds no dot of the colour from, so the substitution cannot be observed");
    }
    if (sel == 4) { /* B is blank: every active dot has the index and shows the border; A is not blank */
        static const char *const keys[] = {"index"};
        unsigned shown = 0u;

        if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "index", &v1) != 0) {
            return vk_bad(rc, "REL blank_is_border needs the single item index=N");
        }
        for (i = 0u; i < nb; i++) {
            uint32_t aw = vk_u16(rb->w->rec[lb[i]] + VK_O_ACTIVE_W);
            uint32_t x;

            for (x = 0u; x < aw; x++) {
                if ((uint64_t)vk_dot(rb->w->rec[lb[i]], x, 0) != v1 || vk_dot(rb->w->rec[lb[i]], x, 1) != vk_u16(rb->w->rec[lb[i]] + VK_O_BORDER)) {
                    return vk_fail_rel(rc, "blank_is_border", "a dot of B does not have the blank index and the border colour");
                }
            }
        }
        for (i = 0u; i < na; i++) {
            uint32_t aw = vk_u16(ra->w->rec[la[i]] + VK_O_ACTIVE_W);
            uint32_t x;

            for (x = 0u; x < aw; x++) {
                if (vk_dot(ra->w->rec[la[i]], x, 1) != vk_u16(ra->w->rec[la[i]] + VK_O_BORDER)) {
                    shown = 1u;
                }
            }
        }
        return shown ? VG_PASS : vk_bad(rc, "REL blank_is_border: run A shows nothing but the border, so a blank cannot be told from it");
    }
    if (sel == 5) { /* the width in master clocks (active_w * div) is the same, the divider differs: the window is a count of dots at another dot clock */
        static const char *const keys[] = {"lines"};
        unsigned pairs = na < nb ? na : nb;

        if (vk_need(rc, keys, 1u) != 0 || vk_num(rc, "lines", &v1) != 0 || v1 == 0u) {
            return vk_bad(rc, "REL dot_width_scale needs the single item lines=N (at least 1)");
        }
        if ((uint64_t)pairs < v1) {
            snprintf(rc->why, rc->cap, "REL dot_width_scale: the runs have %u corresponding active lines, the row wants %llu", pairs, (unsigned long long)v1);
            return VG_FAIL;
        }
        for (i = 0u; i < pairs; i++) {
            uint64_t ca = (uint64_t)vk_u16(ra->w->rec[la[i]] + VK_O_ACTIVE_W) * (uint64_t)ra->w->rec[la[i]][VK_O_DIV];
            uint64_t cb = (uint64_t)vk_u16(rb->w->rec[lb[i]] + VK_O_ACTIVE_W) * (uint64_t)rb->w->rec[lb[i]][VK_O_DIV];

            if (ca != cb || ra->w->rec[la[i]][VK_O_DIV] == rb->w->rec[lb[i]][VK_O_DIV]) {
                return vk_fail_rel(rc, "dot_width_scale", "a line of B does not span the same master clocks as in A, or has the same divider");
            }
        }
        return VG_PASS;
    }
    return vk_bad(rc, "a REL row names a relation that is not in the closed list");
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
    if (g->form == VX_FORM_ROWS) {
        v = vk_rows_row(&rc, r, perturb);
        *focus = rc.focus;
        return v;
    }
    if (g->form == VX_FORM_REL) {
        v = vk_rel_row(&rc, r, runs, nruns, perturb);
        *focus = rc.focus;
        return v;
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

/* a vid sink that was never written: no record and no offered record although lines were due by the closed form of the machine time the run reached. A missing sink is a fault of the harness, never a pass (D-172) */
static int vk_sink_missing(uint32_t count, uint64_t seq, uint64_t m)
{
    return count == 0u && seq == 0u && vk_count_closed_form(m) >= 1u;
}

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
    if (vk_sink_missing(cp->vcount, vk_u64(cp->vsk->header + 8), res.machine_time)) {
        harness_error(c, "the vid sink holds no record and was offered none although lines were due: a missing sink is a fault of the harness, never a pass (D-172)");
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

static int vk_golden_judge(struct ctx *c, const struct vx_file *f, const char *scene, const char *hex);

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
        uint8_t gdig[DS_SHA256_BYTES];
        char ghex[2u * DS_SHA256_BYTES + 1u];
        char gdet[700];
        uint8_t *gstream = vk_stream(cp.vbuf, cp.vbytes);
        struct vx_file *own = (struct vx_file *)calloc(1u, sizeof *own);
        struct ctx sub;
        unsigned s2;

        if (gstream == NULL || own == NULL) {
            free(gstream);
            free(own);
            harness_error(c, "out of memory for the golden role");
            goto done;
        }
        ds_sha256(gstream, (size_t)(19u + cp.vbytes), gdig);
        free(gstream);
        vk_hex(gdig, ghex);
        (void)vk_golden_judge(c, cp.vx, cp.e.scene, ghex);
        /* the in-process sub-check of ER-11: the same case pointed at a scene with no literal case (the literal rows of the scene made struct-role rows in a copy) is refused as video-hash-only, a harness_failure */
        memcpy(own, cp.vx, sizeof *own);
        for (s2 = 0u; s2 < own->ns; s2++) {
            if (strcmp(own->sc[s2].name, cp.e.scene) == 0 && own->sc[s2].role == VX_ROLE_LITERAL) {
                own->sc[s2].role = VX_ROLE_STRUCT;
            }
        }
        memset(&sub, 0, sizeof sub);
        (void)vk_golden_judge(&sub, own, cp.e.scene, ghex);
        free(own);
        check(c, sub.harness && !sub.failed && strncmp(sub.why, "video-hash-only", 15u) == 0, "a golden scene with no literal scene was not refused as video-hash-only (ER-11)");
        snprintf(gdet, sizeof gdet, "video_sha256=%s E0 repeatability (a change detector, never a verdict, ER-11)\nvideo_golden: the digest is compared with the pinned digest of the scene; it is valid only beside a literal case of the same scene (video_vce_pal for vce_pal); a golden scene with no literal scene is refused video-hash-only\nvideo_records=%llu video_bytes=%llu machine_time=%llu\n", ghex,
                 (unsigned long long)cp.w.n, (unsigned long long)cp.vbytes, (unsigned long long)cp.lg.m);
        set_evidence_detail(gdet);
        snprintf(got + strlen(got), gotcap - strlen(got), " records=%llu", (unsigned long long)cp.w.n);
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


/* ---- the record faults F0 to F9 (D-172) and the sweep ---- */

#define VK_RUNS_MAX 4u

static const char *const vk_fault_name[10] = {"F0 perturb one of the group's own literals", "F1 shift pixels +1 in x", "F2 swap two bits of an index", "F3 drop the first active line", "F4 duplicate a line", "F5 change the divider", "F6 widen the active window by one dot", "F7 zero every non-zero index", "F8 swap two adjacent lines", "F9 move an origin by one clock"};

static void vk_put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void vk_put64(uint8_t *p, uint64_t v)
{
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        p[i] = (uint8_t)((v >> (8u * i)) & 0xFFu);
    }
}

/* the first record at or after from with an active window; w->n when there is none */
static uint64_t vk_next_active(const struct vk_walk *w, uint64_t from)
{
    uint64_t k;

    for (k = from; k < w->n; k++) {
        if (vk_u16(w->rec[k] + VK_O_ACTIVE_W) > 0u) {
            return k;
        }
    }
    return w->n;
}

/*
 * One fault of D-172 applied to a copy of the captured records, at the record the group looks at (focus, the record of its first row). src is never changed: a changed record is a copy in scr[0] (the pointer of the copy replaces the original in dst) and a dropped, duplicated or swapped line changes only the pointer array dst (capacity src->n + 1). Returns 0 when the fault was applied
 * and -1 when it cannot act on this stream (for example a pixel fault on a record with no active window): such a fault flips nothing, so a group that declares it is vacuous (VIDEO_GROUP_VACUOUS), never silently green. F0 is a fault of the literal and is applied by the row evaluator (perturb), not here.
 *   F1 dot x of the first active record at or after the focus takes dot x - 1 (dot 0 stays), index and colour       F2 bits 0 and 1 of every index of that record are swapped       F3 that record is dropped       F4 a copy of the record before the focus is inserted at the focus
 *   F5 the divider of the focus record changes (4 to 3, any other value to 4)       F6 the window of the focus record is one dot wider (index 0, the border colour)       F7 every index of the first active record at or after the focus is 0       F8 the focus record and the one before it swap (the first two when the focus is 0)       F9 the origin of the focus record grows by one clock
 */
static int vk_fault_apply(const struct vk_walk *src, struct vk_walk *dst, unsigned f, uint64_t focus, uint8_t (*scr)[VK_MAX_REC])
{
    uint64_t n = src->n;
    uint64_t p;
    const uint8_t *rec;
    uint32_t aw;
    uint32_t j;

    if (n == 0u || focus >= n || f == 0u || f > 9u) {
        return -1;
    }
    memcpy((void *)dst->rec, (const void *)src->rec, (size_t)n * sizeof *src->rec);
    dst->n = n;
    dst->ok = 1;
    switch (f) {
    case 1u:
    case 2u:
    case 7u:
        p = vk_next_active(src, focus);
        if (p >= n) {
            return -1;
        }
        rec = src->rec[p];
        aw = vk_u16(rec + VK_O_ACTIVE_W);
        memcpy(scr[0], rec, (size_t)vk_u16(rec + VK_O_REC_BYTES));
        for (j = 0u; j < aw; j++) {
            uint32_t jj = f == 1u ? aw - 1u - j : j; /* F1 walks from the right so that each dot takes the one on its left */

            if (f == 1u) {
                if (jj > 0u) {
                    vk_put16(scr[0] + VK_HDR + 2u * jj, vk_u16(rec + VK_HDR + 2u * (jj - 1u)));
                    vk_put16(scr[0] + VK_HDR + 2u * aw + 2u * jj, vk_u16(rec + VK_HDR + 2u * aw + 2u * (jj - 1u)));
                }
            } else if (f == 2u) {
                uint32_t v = vk_u16(rec + VK_HDR + 2u * jj);

                if (((v >> 1) ^ v) & 1u) {
                    v ^= 3u;
                }
                vk_put16(scr[0] + VK_HDR + 2u * jj, v);
            } else {
                vk_put16(scr[0] + VK_HDR + 2u * jj, 0u);
            }
        }
        dst->rec[p] = scr[0];
        return 0;
    case 3u:
        p = vk_next_active(src, focus);
        if (p >= n) {
            return -1;
        }
        memmove((void *)(dst->rec + p), (const void *)(dst->rec + p + 1u), (size_t)(n - p - 1u) * sizeof *dst->rec);
        dst->n = n - 1u;
        return 0;
    case 4u:
        memmove((void *)(dst->rec + focus + 1u), (const void *)(dst->rec + focus), (size_t)(n - focus) * sizeof *dst->rec);
        dst->rec[focus] = src->rec[focus == 0u ? 0u : focus - 1u];
        dst->n = n + 1u;
        return 0;
    case 5u:
    case 9u:
        rec = src->rec[focus];
        memcpy(scr[0], rec, (size_t)vk_u16(rec + VK_O_REC_BYTES));
        if (f == 5u) {
            scr[0][VK_O_DIV] = (uint8_t)(rec[VK_O_DIV] == 4u ? 3u : 4u);
        } else {
            vk_put64(scr[0] + VK_O_ORIGIN, vk_u64(rec + VK_O_ORIGIN) + 1u);
        }
        dst->rec[focus] = scr[0];
        return 0;
    case 6u:
        rec = src->rec[focus];
        aw = vk_u16(rec + VK_O_ACTIVE_W);
        if (aw >= VK_MAX_DOTS) {
            return -1;
        }
        memset(scr[0], 0, VK_MAX_REC);
        memcpy(scr[0], rec, VK_HDR);
        for (j = 0u; j < aw; j++) {
            vk_put16(scr[0] + VK_HDR + 2u * j, vk_u16(rec + VK_HDR + 2u * j));
            vk_put16(scr[0] + VK_HDR + 2u * (aw + 1u) + 2u * j, vk_u16(rec + VK_HDR + 2u * aw + 2u * j));
        }
        vk_put16(scr[0] + VK_HDR + 2u * aw, 0u);
        vk_put16(scr[0] + VK_HDR + 2u * (aw + 1u) + 2u * aw, vk_u16(rec + VK_O_BORDER));
        vk_put16(scr[0] + VK_O_ACTIVE_W, aw + 1u);
        vk_put16(scr[0] + VK_O_REC_BYTES, VK_HDR + ((4u * (aw + 1u) + 7u) & ~7u));
        dst->rec[focus] = scr[0];
        return 0;
    default: /* F8 */
        if (n < 2u) {
            return -1;
        }
        p = focus == 0u ? 1u : focus;
        dst->rec[p] = src->rec[p - 1u];
        dst->rec[p - 1u] = src->rec[p];
        return 0;
    }
}

/* the result of the sweep of one group */
struct vk_sweep {
    unsigned declared; /* faults the group declares in kills= */
    unsigned flipped;  /* of them, turned into a failure */
    unsigned vacuous;  /* of them, left the group green or could not act (VIDEO_GROUP_VACUOUS) */
    unsigned harness;  /* of them, gave a fault of the case instead of a verdict */
    int baseline;      /* the verdict of the group on the unfaulted run (it must pass for a sweep to mean anything) */
    char first[400];   /* the text of the first vacuous or harness fault, or of the failed baseline */
    char line[96]; /* one line for the evidence text: the faults that flipped it */
};

/*
 * The sweep of one group of the scene on the runs of a case (D-172): the group must pass unfaulted; then each fault of its kills= is applied (F0 to the literal of its first row, the others to a copy of the LAST run at the record the group looks at first) and the group must come out as a failure. A group that a declared fault leaves green, or on which the fault cannot act, is vacuous.
 * dstrec is a pointer array of at least n + 1 records and scr two record images.
 */
static void vk_sweep_group(const struct vx_file *f, unsigned gi, const struct vk_run *runs, unsigned nruns, const uint8_t **dstrec, uint8_t (*scr)[VK_MAX_REC], struct vk_sweep *sw)
{
    unsigned bit;
    uint64_t focus = VK_NOFOCUS;
    uint64_t fo2;
    char why[240];
    size_t used = 0u;

    memset(sw, 0, sizeof *sw);
    sw->baseline = vk_eval_group(f, gi, runs, nruns, 0, &focus, why, sizeof why);
    if (sw->baseline != VG_PASS) {
        snprintf(sw->first, sizeof sw->first, "the group %s does not pass unfaulted (%s)", f->gr[gi].gid, why);
        return;
    }
    if (focus == VK_NOFOCUS || focus >= runs[nruns - 1u].w->n) {
        focus = runs[nruns - 1u].w->n / 2u;
    }
    for (bit = 0u; bit < 10u; bit++) {
        struct vk_run fr[VK_RUNS_MAX];
        struct vk_walk fw;
        unsigned i;
        int applied = 0;
        int v;

        if ((f->gr[gi].kills & (1u << bit)) == 0u) {
            continue;
        }
        sw->declared++;
        for (i = 0u; i < nruns && i < VK_RUNS_MAX; i++) {
            fr[i] = runs[i];
        }
        fw.rec = dstrec;
        fw.n = 0u;
        fw.ok = 0;
        if (bit == 0u) {
            v = vk_eval_group(f, gi, runs, nruns, 1, &fo2, why, sizeof why);
            applied = 1;
        } else if (vk_fault_apply(runs[nruns - 1u].w, &fw, bit, focus, scr) == 0) {
            fr[nruns - 1u].w = &fw;
            v = vk_eval_group(f, gi, fr, nruns, 0, &fo2, why, sizeof why);
            applied = 1;
        } else {
            v = VG_PASS;
        }
        if (applied && v == VG_FAIL) {
            sw->flipped++;
            if (used + 4u < sizeof sw->line) {
                used += (size_t)snprintf(sw->line + used, sizeof sw->line - used, "%sF%u", used == 0u ? "" : ",", bit);
            }
        } else if (applied && v == VG_HARNESS) {
            sw->harness++;
            if (sw->first[0] == '\0') {
                snprintf(sw->first, sizeof sw->first, "%s on group %s gave a fault of the case (%s)", vk_fault_name[bit], f->gr[gi].gid, why);
            }
        } else {
            sw->vacuous++;
            if (sw->first[0] == '\0') {
                snprintf(sw->first, sizeof sw->first, "VIDEO_GROUP_VACUOUS: %s does not flip group %s%s", vk_fault_name[bit], f->gr[gi].gid, applied ? "" : " (it cannot act on this stream)");
            }
        }
    }
}

/* the stream of D-163 of a walk (the records of the walk, in its order, behind the header); a heap block of 19 + bytes, its length in *len, NULL when out of memory */
static uint8_t *vk_stream_of_walk(const struct vk_walk *w, uint64_t *len)
{
    uint64_t total = 0u;
    uint64_t k;
    uint8_t *stream;
    uint64_t off = 19u;

    for (k = 0u; k < w->n; k++) {
        total += (uint64_t)vk_u16(w->rec[k] + VK_O_REC_BYTES);
    }
    stream = (uint8_t *)malloc((size_t)(19u + total));
    if (stream == NULL) {
        return NULL;
    }
    memcpy(stream, "duospool-video", 14u);
    stream[14] = 0u;
    stream[15] = 1u;
    stream[16] = 0u;
    stream[17] = 0u;
    stream[18] = 0u;
    for (k = 0u; k < w->n; k++) {
        uint32_t rb = vk_u16(w->rec[k] + VK_O_REC_BYTES);

        memcpy(stream + off, w->rec[k], (size_t)rb);
        off += (uint64_t)rb;
    }
    *len = off;
    return stream;
}

/* the golden role (ER-11, D-172): a stream digest is a change detector and never a verdict, so it is judged only beside a literal scene of the same NAME. No literal scene row of that name is refused as video-hash-only, a harness_failure, before the digest is looked at; the digest then
 * must equal the pinned digest of the scene (one assertion). Returns 1 when the digest was compared */
static int vk_golden_judge(struct ctx *c, const struct vx_file *f, const char *scene, const char *hex)
{
    unsigned i;
    int lit = 0;
    const struct vx_golden *gd = NULL;

    for (i = 0u; i < f->ns; i++) {
        if (strcmp(f->sc[i].name, scene) == 0 && f->sc[i].role == VX_ROLE_LITERAL) {
            lit = 1;
        }
    }
    if (!lit) {
        harness_error(c, "video-hash-only: a golden scene is valid only beside a literal case of the same scene (a digest alone is not a verdict, ER-11)");
        return 0;
    }
    for (i = 0u; i < f->nd; i++) {
        if (strcmp(f->gd[i].scene, scene) == 0) {
            gd = &f->gd[i];
        }
    }
    if (gd == NULL) {
        harness_error(c, "the vexp rows hold no golden row for the scene");
        return 0;
    }
    check(c, strcmp(gd->sha, hex) == 0, "golden: the stream digest differs from the pinned golden digest (a change detector, E0 repeatability; the literal case of the scene decides what is right, ER-11)");
    return 1;
}

/* the groups of a scene on one run: the number of groups judged, and 1 in *bad when one failed or is a fault of the case (scratch contexts of the judge-forms and the hash-not-verdict kinds read the verdict) */
static unsigned vk_scene_groups(const struct vx_file *f, const char *scene, const struct vk_run *runs, unsigned nruns, unsigned *bad)
{
    unsigned gi;
    unsigned n = 0u;

    *bad = 0u;
    for (gi = 0u; gi < f->ng; gi++) {
        char why[240];
        uint64_t fo;

        if (strcmp(f->gr[gi].scene, scene) != 0) {
            continue;
        }
        n++;
        if (vk_eval_group(f, gi, runs, nruns, 0, &fo, why, sizeof why) != VG_PASS) {
            (*bad)++;
        }
    }
    return n;
}

/* the scene has a scene row of the role literal or struct: the sweep and the hash-not-verdict kind need the literal groups of the scene */
static int vk_scene_has_literal(const struct vx_file *f, const char *scene)
{
    unsigned i;

    for (i = 0u; i < f->ns; i++) {
        if (strcmp(f->sc[i].name, scene) == 0 && (f->sc[i].role == VX_ROLE_LITERAL || f->sc[i].role == VX_ROLE_STRUCT)) {
            return 1;
        }
    }
    return 0;
}

/*
 * kind video_fault_sweep (D-172, ER-11): run the guest once, capture the records, then for every group of the scene apply every fault the group declares in kills= to the captured records in memory and require the group to flip (see vk_sweep_group). A group that no declared fault flips is a failure naming VIDEO_GROUP_VACUOUS; a group that
 * does not pass unfaulted, or a fault that gives a fault of the case, is reported. expect is that of the video kind (rom=NAME;id=N;scene=NAME). E0: the sweep shows that each literal comparison is live against these record faults, a mechanism and not a claim about any video.
 */
void kind_video_fault_sweep(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct vk_cap cp;
    const uint8_t **dstrec = NULL;
    uint8_t (*scr)[VK_MAX_REC] = NULL;
    unsigned gi;
    unsigned ngroups = 0u;
    unsigned declared = 0u;
    unsigned flipped = 0u;
    char detail[2048];
    size_t used = 0u;

    if (vk_capture(k, c, &cp, got, gotcap) != 0) {
        goto done;
    }
    if (!vk_scene_has_literal(cp.vx, cp.e.scene)) {
        harness_error(c, "the scene of a fault sweep needs a literal or struct scene row");
        goto done;
    }
    dstrec = (const uint8_t **)calloc((size_t)cp.w.n + 2u, sizeof *dstrec);
    scr = (uint8_t(*)[VK_MAX_REC])calloc(2u, VK_MAX_REC);
    if (dstrec == NULL || scr == NULL) {
        harness_error(c, "out of memory for the fault sweep");
        goto done;
    }
    detail[0] = '\0';
    for (gi = 0u; gi < cp.vx->ng; gi++) {
        struct vk_run run;
        struct vk_sweep sw;
        char msg[440];
        char gline[320];

        if (strcmp(cp.vx->gr[gi].scene, cp.e.scene) != 0) {
            continue;
        }
        run.name = "main";
        run.w = &cp.w;
        run.lg = &cp.lg;
        vk_sweep_group(cp.vx, gi, &run, 1u, dstrec, scr, &sw);
        ngroups++;
        declared += sw.declared;
        flipped += sw.flipped;
        if (sw.baseline != VG_PASS) {
            snprintf(msg, sizeof msg, "sweep: %s", sw.first);
            check(c, 0, msg);
        } else if (sw.harness > 0u) {
            snprintf(msg, sizeof msg, "sweep: %s", sw.first);
            harness_error(c, msg);
            goto done;
        } else {
            snprintf(msg, sizeof msg, "sweep: %s", sw.vacuous > 0u ? sw.first : "every declared fault flips the group");
            check(c, sw.vacuous == 0u && sw.declared >= 1u, msg);
        }
        snprintf(gline, sizeof gline, "video_sweep %s declared=%u flipped=%u vacuous=%u flips=%s\n", cp.vx->gr[gi].gid, sw.declared, sw.flipped, sw.vacuous, sw.line[0] != '\0' ? sw.line : "-");
        if (used + strlen(gline) + 1u < sizeof detail) {
            memcpy(detail + used, gline, strlen(gline) + 1u);
            used += strlen(gline);
        }
    }
    if (ngroups == 0u) {
        harness_error(c, "the scene of the fault sweep has no group");
        goto done;
    }
    {
        char head[200];

        snprintf(head, sizeof head, "video_sweep: groups=%u declared=%u flipped=%u (E0: each literal comparison shown live against the record faults it declares, ER-11, D-172)\n", ngroups, declared, flipped);
        if (strlen(head) + used + 1u <= sizeof detail) {
            memmove(detail + strlen(head), detail, used + 1u);
            memcpy(detail, head, strlen(head));
        }
        set_evidence_detail(detail);
    }
    snprintf(got + strlen(got), gotcap - strlen(got), " groups=%u flipped=%u/%u", ngroups, flipped, declared);
done:
    free((void *)dstrec);
    free(scr);
    vk_cap_free(&cp);
}

/*
 * kind video_hash_not_verdict (ER-11, D-172, T-04-25): a video hash is a change detector and never a verdict. The guest runs once; then an F5-faulted copy of the captured records (the divider of the record the div group looks at is changed) is hashed in memory. The faulted stream (1) passes a golden computed for that faulted stream,
 * the very comparison the golden role makes, (2) fails the golden of the unfaulted stream, so a golden does detect a change, and (3) fails the literal div group of the scene, so a passing golden proves nothing about the content. expect is that of the video kind; the scene must hold a group with the id div and one golden row.
 */
void kind_video_hash_not_verdict(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct vk_cap cp;
    const uint8_t **dstrec = NULL;
    uint8_t (*scr)[VK_MAX_REC] = NULL;
    struct vk_run run;
    struct vk_walk fw;
    struct vx_file *own = NULL;
    uint8_t digest[DS_SHA256_BYTES];
    char hex_ok[2u * DS_SHA256_BYTES + 1u];
    char hex_f[2u * DS_SHA256_BYTES + 1u];
    uint8_t *stream = NULL;
    uint64_t slen = 0u;
    uint64_t focus = VK_NOFOCUS;
    unsigned gi;
    unsigned div_gi = 0u;
    int have_div = 0;
    char why[240];
    struct ctx sub;

    if (vk_capture(k, c, &cp, got, gotcap) != 0) {
        goto done;
    }
    for (gi = 0u; gi < cp.vx->ng; gi++) {
        if (strcmp(cp.vx->gr[gi].scene, cp.e.scene) == 0 && strcmp(cp.vx->gr[gi].gid, "div") == 0) {
            div_gi = gi;
            have_div = 1;
        }
    }
    if (!have_div || !vk_scene_has_literal(cp.vx, cp.e.scene)) {
        harness_error(c, "the scene of video_hash_not_verdict needs a literal scene row and a group with the id div");
        goto done;
    }
    run.name = "main";
    run.w = &cp.w;
    run.lg = &cp.lg;
    check(c, vk_eval_group(cp.vx, div_gi, &run, 1u, 0, &focus, why, sizeof why) == VG_PASS, "the div group does not pass on the unfaulted stream: the case has nothing to show");
    dstrec = (const uint8_t **)calloc((size_t)cp.w.n + 2u, sizeof *dstrec);
    scr = (uint8_t(*)[VK_MAX_REC])calloc(2u, VK_MAX_REC);
    own = (struct vx_file *)calloc(1u, sizeof *own);
    if (dstrec == NULL || scr == NULL || own == NULL) {
        harness_error(c, "out of memory for video_hash_not_verdict");
        goto done;
    }
    fw.rec = dstrec;
    fw.n = 0u;
    fw.ok = 0;
    if (focus == VK_NOFOCUS || vk_fault_apply(&cp.w, &fw, 5u, focus, scr) != 0) {
        harness_error(c, "the F5 fault could not be applied to the stream");
        goto done;
    }
    /* the digest of the unfaulted stream and of the faulted one */
    stream = vk_stream_of_walk(&cp.w, &slen);
    if (stream == NULL) {
        harness_error(c, "out of memory for the video stream");
        goto done;
    }
    ds_sha256(stream, (size_t)slen, digest);
    free(stream);
    vk_hex(digest, hex_ok);
    stream = vk_stream_of_walk(&fw, &slen);
    if (stream == NULL) {
        harness_error(c, "out of memory for the video stream");
        goto done;
    }
    ds_sha256(stream, (size_t)slen, digest);
    free(stream);
    stream = NULL;
    vk_hex(digest, hex_f);
    check(c, strcmp(hex_ok, hex_f) != 0, "the F5-faulted stream has the digest of the unfaulted one: the fault changed nothing");
    /* (1) the faulted stream passes a golden computed for it: a copy of the rows whose golden digest for the scene is that of the faulted stream */
    memcpy(own, cp.vx, sizeof *own);
    for (gi = 0u; gi < own->nd; gi++) {
        if (strcmp(own->gd[gi].scene, cp.e.scene) == 0) {
            memcpy(own->gd[gi].sha, hex_f, sizeof own->gd[gi].sha);
        }
    }
    memset(&sub, 0, sizeof sub);
    check(c, vk_golden_judge(&sub, own, cp.e.scene, hex_f) == 1 && !sub.failed && !sub.harness && sub.n == 1u, "the faulted stream does not pass a golden computed for that faulted stream");
    /* (2) it fails the golden of the unfaulted stream */
    memcpy(own->gd[0].sha, hex_ok, sizeof own->gd[0].sha);
    memset(&sub, 0, sizeof sub);
    (void)vk_golden_judge(&sub, own, cp.e.scene, hex_f);
    check(c, sub.failed && !sub.harness, "the faulted stream passes the golden of the unfaulted stream: a golden would not detect the change");
    /* (3) and it fails the literal div group, which the golden of the faulted stream cannot know */
    run.w = &fw;
    check(c, vk_eval_group(cp.vx, div_gi, &run, 1u, 0, &focus, why, sizeof why) == VG_FAIL, "the F5-faulted stream passes the literal div group: the literal is not live against a changed divider");
    {
        char detail[600];

        snprintf(detail, sizeof detail, "video_hash_not_verdict: unfaulted video_sha256=%s, F5-faulted video_sha256=%s\nthe faulted stream passes its own golden (E0 repeatability), fails the golden of the unfaulted stream and fails the literal group div: a hash is a change detector, never a verdict (ER-11)\n", hex_ok, hex_f);
        set_evidence_detail(detail);
    }
done:
    free(stream);
    free((void *)dstrec);
    free(scr);
    free(own);
    vk_cap_free(&cp);
}

/* ---- kind video_judge_forms (D-171, D-172): the forms of the judge on hand-built records ---- */

/*
 * The records of this kind are built by TEST CODE from the layout of D-161 written in this file: no renderer, no core, no emulator and no hash produces an expected value (ER-7, ER-13). The rows are parsed by the same parser that reads a .vexp.tsv file, so the grammar of the file is exercised too. One scene and one group per check, the verdict of the group compared with the verdict wanted:
 *   ROWS    index and value lists (plain and COUNT*V items), a second active line, a wrong index, a wrong value, dots outside the active window, a line that does not exist, the non-vacuity rule of D-171 (fewer than 3 distinct values, no transition: a fault of the case) and F0 on the first row
 *   REL     each relation id (scroll_x_shift, scroll_y_shift, palette_subst, blank_is_border, dot_width_scale) on two named runs: a pair that has the relation passes, a pair that does not fails, and F0 on the literal fails the pair that has it
 *   STRUCT  each closed check (count, origins, frame_lines, records_per_frame, rec_bytes, values_below_512, div_table, div_first, border_after_pal, line_dirty, display_lines, width_set): the literal that is right passes and a literal (or a stream) that is wrong fails
 *   null renderers: a run with no record, a run of all-border dots and a run of all index 0 never pass any form (D-172); a sink that was never written is a fault of the harness
 *   sweep   the faults F0 F1 F2 F3 F4 F7 F8 flip a ROWS group that declares them, a STRUCT group that declares a pixel fault is reported vacuous, and F6 widens a record by one dot and keeps the stream well formed
 * Class of every row: ASSUMED (the rows are test data of this kind, not a statement about a video). E0.
 */

#define JF_MAXREC 16u
#define JF_RECBYTES 128u

struct jf_stream {
    uint8_t buf[JF_MAXREC * JF_RECBYTES];
    const uint8_t *rec[JF_MAXREC + 2u];
    struct vk_walk w;
    unsigned used;
};

struct jf_line {
    uint32_t aw;
    uint32_t border;
    uint32_t div;
    uint32_t dirty;
    uint32_t idx[8];
    uint32_t col[8];
};

static void jf_init(struct jf_stream *s)
{
    memset(s, 0, sizeof *s);
    s->w.rec = s->rec;
    s->w.ok = 1;
}

static void jf_add(struct jf_stream *s, uint64_t origin, uint32_t frame, uint32_t line, uint32_t fl, const struct jf_line *l)
{
    uint8_t *p = s->buf + s->used;
    uint32_t rb = VK_HDR + ((4u * l->aw + 7u) & ~7u);
    uint32_t j;

    memset(p, 0, rb);
    vk_put64(p + VK_O_ORIGIN, origin);
    vk_put16(p + VK_O_FRAME, frame & 0xFFFFu);
    vk_put16(p + VK_O_FRAME + 2, frame >> 16);
    vk_put16(p + VK_O_LINE, line);
    vk_put16(p + VK_O_FRAME_LINES, fl);
    vk_put16(p + VK_O_REC_BYTES, rb);
    vk_put16(p + VK_O_LINE_MCLK, VK_GRID);
    p[VK_O_DIV] = (uint8_t)l->div;
    vk_put16(p + VK_O_ACTIVE_W, l->aw);
    vk_put16(p + VK_O_BORDER, l->border);
    p[VK_O_LINE_DIRTY] = (uint8_t)l->dirty;
    p[VK_O_VERSION] = 1u;
    for (j = 0u; j < l->aw; j++) {
        vk_put16(p + VK_HDR + 2u * j, l->idx[j]);
        vk_put16(p + VK_HDR + 2u * l->aw + 2u * j, l->col[j]);
    }
    s->rec[s->w.n] = p;
    s->w.n++;
    s->used += rb;
}

/* the base stream of the STRUCT checks: 10 lines, frame_lines 4 (frames 0 and 1 complete, frame 2 holds 2 lines), lines 1 and 2 of each frame active with 4 dots (index (3k + 2x) % 7 + 1, colour 100 + index), border 5, divider 4 before record 8 and 3 from it, line_dirty bit 0 on records 1 and 8 and bit 4 on record 2.
 * variant 1 moves the origin of record 4 by one clock; variant 2 gives every active dot the index idxv (a null renderer: 256 is all-border, 0 is all index 0) and the colour 5 of the border and of palette entry 0 */
static void jf_base(struct jf_stream *s, int variant, uint32_t idxv)
{
    uint32_t k;

    jf_init(s);
    for (k = 0u; k < 10u; k++) {
        struct jf_line l;
        uint32_t x;

        memset(&l, 0, sizeof l);
        l.border = 5u;
        l.div = k < 8u ? 4u : 3u;
        l.dirty = k == 1u || k == 8u ? 1u : k == 2u ? 16u : 0u;
        if (k % 4u == 1u || k % 4u == 2u) {
            l.aw = 4u;
            for (x = 0u; x < 4u; x++) {
                l.idx[x] = variant == 2 ? idxv : (3u * k + 2u * x) % 7u + 1u;
                l.col[x] = variant == 2 ? 5u : 100u + l.idx[x];
            }
        }
        jf_add(s, (uint64_t)k * VK_GRID + (variant == 1 && k == 4u ? 1u : 0u), k / 4u, k % 4u, 4u, &l);
    }
}

/* the dev log of the base stream: CR writes $00 at 100 and $01 at 9655, one palette write at 2000, the machine time 9 * 1365 + 1 */
static void jf_log(struct vk_log *lg)
{
    memset(lg, 0, sizeof *lg);
    lg->n_cr = 2u;
    lg->t_cr[0] = 100u;
    lg->v_cr[0] = 0u;
    lg->t_cr[1] = 9655u;
    lg->v_cr[1] = 1u;
    lg->n_pal = 1u;
    lg->t_pal[0] = 2000u;
    lg->t_pal_last = 2000u;
    lg->m = 9u * (uint64_t)VK_GRID + 1u;
}

/* a stream of 3 active lines (records 0 to 2 of frame 0) of the given width, divider and border with the index rows idx and the colours col */
static void jf_rows_stream(struct jf_stream *s, uint32_t aw, uint32_t div, uint32_t border, const uint32_t idx[3][8], const uint32_t col[3][8])
{
    uint32_t k;

    jf_init(s);
    for (k = 0u; k < 3u; k++) {
        struct jf_line l;
        uint32_t x;

        memset(&l, 0, sizeof l);
        l.aw = aw;
        l.div = div;
        l.border = border;
        for (x = 0u; x < aw; x++) {
            l.idx[x] = idx[k][x];
            l.col[x] = col[k][x];
        }
        jf_add(s, (uint64_t)k * VK_GRID, 0u, k, 4u, &l);
    }
}

/* the rows of one scene of one group from their text, parsed by the parser of the .vexp.tsv reader; 0 on success */
static int jf_parse(struct vx_file *f, const char *form, const char *cls, const char *kills, const char *sel, const char *lit)
{
    char text[3][480];
    char *fld[8];
    unsigned i;
    unsigned nf;
    const char *err;

    memset(f, 0, sizeof *f);
    snprintf(text[0], sizeof text[0], "scene\tjf\tcase=jf_case\trole=literal");
    snprintf(text[1], sizeof text[1], "group\tjf\tg\tform=%s\tclass=%s\tsym=none\tkills=%s", form, cls, kills);
    snprintf(text[2], sizeof text[2], "row\tjf\tg\t%s\t%s", sel, lit);
    for (i = 0u; i < 3u; i++) {
        nf = vx_split(text[i], fld, 8u);
        err = nf > 8u ? "too many fields" : vx_row_parse(f, fld, nf);
        if (err != NULL) {
            return -1;
        }
    }
    return vx_resolve(f) == NULL ? 0 : -1;
}

struct jf_ctx {
    struct vx_file *vx;
    struct ctx *c;
    unsigned checks;
    char failed[1500]; /* the text of every check that failed, one per line, for the evidence file */
};

/* one row on the runs: the verdict of its group (F0 when perturb) */
static int jf_eval(struct jf_ctx *jf, const char *form, const char *kills, const char *sel, const char *lit, const struct vk_run *runs, unsigned nruns, int perturb)
{
    char why[240];
    uint64_t fo;

    if (jf_parse(jf->vx, form, "ASSUMED:LED-VID-0004", kills, sel, lit) != 0) {
        return VG_HARNESS + 1;
    }
    return vk_eval_group(jf->vx, 0u, runs, nruns, perturb, &fo, why, sizeof why);
}

static void jf_want(struct jf_ctx *jf, const char *name, int got, int want)
{
    char msg[200];

    snprintf(msg, sizeof msg, "judge forms: %s (verdict %d, wanted %d)", name, got, want);
    check(jf->c, got == want, msg);
    jf->checks++;
    if (got != want && strlen(jf->failed) + strlen(msg) + 2u < sizeof jf->failed) {
        strcat(jf->failed, msg);
        strcat(jf->failed, "\n");
    }
}

static void jf_run1(struct vk_run *r, const struct jf_stream *s, const struct vk_log *lg)
{
    r->name = "main";
    r->w = &s->w;
    r->lg = lg;
}

/* the STRUCT checks on the base stream: the right literal passes, the wrong one fails */
static void jf_struct(struct jf_ctx *jf, const struct jf_stream *s, const struct jf_stream *skew, const struct vk_log *lg)
{
    static const struct {
        const char *check;
        const char *ok;
        const char *bad;
    } t[] = {
        {"count", "grid=1365", "grid=1366"},
        {"origins", "step=1365", "step=1366"},
        {"frame_lines", "value=4;step=1365", "value=5;step=1365"},
        {"records_per_frame", "value=4", "value=5"},
        {"rec_bytes", "header=64;version=1;max_w=682", "header=65;version=1;max_w=682"},
        {"values_below_512", "limit=512", "limit=2"},
        {"div_table", "div=4,3;crw=0,1;gap=5460", "div=4,4;crw=0,1;gap=5460"},
        {"div_first", "div=3;prev=4;crw=1", "div=3;prev=3;crw=1"},
        {"border_after_pal", "value=5", "value=6"},
        {"line_dirty", "cr_bit=0;pal_bit=4", "cr_bit=1;pal_bit=4"},
        {"display_lines", "vds=0;vdw=1;frame_lines=4", "vds=0;vdw=0;frame_lines=4"},
        {"width_set", "min_distinct=2", "min_distinct=3"},
    };
    struct vk_run run;
    unsigned i;

    jf_run1(&run, s, lg);
    for (i = 0u; i < (unsigned)(sizeof t / sizeof t[0]); i++) {
        char sel[48];
        char name[80];

        snprintf(sel, sizeof sel, "check=%s", t[i].check);
        snprintf(name, sizeof name, "STRUCT %s with the right literal passes", t[i].check);
        jf_want(jf, name, jf_eval(jf, "STRUCT", "F0", sel, t[i].ok, &run, 1u, 0), VG_PASS);
        snprintf(name, sizeof name, "STRUCT %s with a wrong literal fails", t[i].check);
        jf_want(jf, name, jf_eval(jf, "STRUCT", "F0", sel, t[i].bad, &run, 1u, 0), VG_FAIL);
        if (strcmp(t[i].check, "values_below_512") != 0) { /* F0 raises the limit 512 to 513, which every value below 512 still satisfies: this check is live through its wrong literal above, not through F0 */
            snprintf(name, sizeof name, "STRUCT %s with F0 on the right literal fails", t[i].check);
            jf_want(jf, name, jf_eval(jf, "STRUCT", "F0", sel, t[i].ok, &run, 1u, 1), VG_FAIL);
        }
    }
    jf_run1(&run, skew, lg); /* a stream whose origin of record 4 is one clock off: the origins check fails and the count check does not */
    jf_want(jf, "STRUCT origins on a stream with one origin moved fails", jf_eval(jf, "STRUCT", "F0", "check=origins", "step=1365", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "STRUCT count on a stream with one origin moved still passes (it counts records)", jf_eval(jf, "STRUCT", "F0", "check=count", "grid=1365", &run, 1u, 0), VG_PASS);
}

/* the ROWS forms on the base stream (active+0 is record 1, active+1 is record 2) */
static void jf_rows(struct jf_ctx *jf, const struct jf_stream *s, const struct vk_log *lg)
{
    struct vk_run run;

    jf_run1(&run, s, lg);
    jf_want(jf, "ROWS an index list passes", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1,3", &run, 1u, 0), VG_PASS);
    jf_want(jf, "ROWS a value list passes", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "value=104,106,101,103", &run, 1u, 0), VG_PASS);
    jf_want(jf, "ROWS COUNT*V items pass", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=1*4,1*6,1*1,1*3", &run, 1u, 0), VG_PASS);
    jf_want(jf, "ROWS the second active line passes", jf_eval(jf, "ROWS", "F0", "line=active+1;x=0..3", "index=7,2,4,6", &run, 1u, 0), VG_PASS);
    jf_want(jf, "ROWS a partial x range passes", jf_eval(jf, "ROWS", "F0", "line=active+0;x=1..3", "index=6,1,3", &run, 1u, 0), VG_PASS);
    jf_want(jf, "ROWS a wrong index fails", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1,4", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "ROWS a wrong value fails", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "value=104,106,101,104", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "ROWS dots outside the active window fail", jf_eval(jf, "ROWS", "F0", "line=active+0;x=2..5", "index=1,3,5,7", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "ROWS a line that does not exist fails", jf_eval(jf, "ROWS", "F0", "line=active+9;x=0..3", "index=4,6,1,3", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "ROWS F0 on the first expected value fails", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1,3", &run, 1u, 1), VG_FAIL);
    jf_want(jf, "ROWS one distinct value is a fault of the case (D-171 non-vacuity)", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,4,4,4", &run, 1u, 0), VG_HARNESS);
    jf_want(jf, "ROWS two distinct values are a fault of the case (D-171 non-vacuity)", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,4,6,6", &run, 1u, 0), VG_HARNESS);
    jf_want(jf, "ROWS a list of the wrong length is a fault of the case", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1", &run, 1u, 0), VG_HARNESS);
}

/* the five relations on pairs of runs of three active lines: the pair that has the relation passes, the pair that does not fails */
static void jf_rel(struct jf_ctx *jf, const struct vk_log *lg)
{
    static const uint32_t ia[3][8] = {{1, 2, 3, 4, 5, 6}, {2, 3, 4, 5, 6, 7}, {3, 4, 5, 6, 7, 8}};
    static const uint32_t ca[3][8] = {{201, 202, 203, 204, 205, 206}, {202, 203, 204, 205, 206, 207}, {203, 204, 205, 206, 207, 208}};
    static const uint32_t ib_x[3][8] = {{2, 3, 4, 5, 6, 9}, {3, 4, 5, 6, 7, 9}, {4, 5, 6, 7, 8, 9}};
    static const uint32_t cb_x[3][8] = {{202, 203, 204, 205, 206, 209}, {203, 204, 205, 206, 207, 209}, {204, 205, 206, 207, 208, 209}};
    static const uint32_t ib_y[3][8] = {{2, 3, 4, 5, 6, 7}, {3, 4, 5, 6, 7, 8}, {3, 4, 5, 6, 7, 8}};
    static const uint32_t cb_y[3][8] = {{202, 203, 204, 205, 206, 207}, {203, 204, 205, 206, 207, 208}, {203, 204, 205, 206, 207, 208}};
    static const uint32_t cb_p[3][8] = {{201, 202, 300, 204, 205, 206}, {202, 300, 204, 205, 206, 207}, {300, 204, 205, 206, 207, 208}};
    static const uint32_t ib_b[3][8] = {{256, 256, 256, 256, 256, 256}, {256, 256, 256, 256, 256, 256}, {256, 256, 256, 256, 256, 256}};
    static const uint32_t cb_b[3][8] = {{7, 7, 7, 7, 7, 7}, {7, 7, 7, 7, 7, 7}, {7, 7, 7, 7, 7, 7}};
    static const uint32_t ib_b2[3][8] = {{256, 256, 256, 256, 256, 256}, {256, 256, 256, 256, 256, 1}, {256, 256, 256, 256, 256, 256}};
    static const uint32_t ib_w[3][8] = {{1, 2, 3, 4, 5, 6, 7, 8}, {2, 3, 4, 5, 6, 7, 8, 9}, {3, 4, 5, 6, 7, 8, 9, 1}};
    static const uint32_t cb_w[3][8] = {{1, 2, 3, 4, 5, 6, 7, 8}, {2, 3, 4, 5, 6, 7, 8, 9}, {3, 4, 5, 6, 7, 8, 9, 1}};
    struct jf_stream a;
    struct jf_stream b;
    struct vk_run runs[2];

    jf_rows_stream(&a, 6u, 4u, 7u, ia, ca);
    runs[0].name = "a";
    runs[0].w = &a.w;
    runs[0].lg = lg;
    runs[1].name = "b";
    runs[1].w = &b.w;
    runs[1].lg = lg;
    jf_rows_stream(&b, 6u, 4u, 7u, ib_x, cb_x);
    jf_want(jf, "REL scroll_x_shift on a pair shifted by one dot passes", jf_eval(jf, "REL", "F0", "rel=scroll_x_shift;runs=a,b", "dx=1", runs, 2u, 0), VG_PASS);
    jf_want(jf, "REL scroll_x_shift with F0 on dx fails the pair", jf_eval(jf, "REL", "F0", "rel=scroll_x_shift;runs=a,b", "dx=1", runs, 2u, 1), VG_FAIL);
    jf_rows_stream(&b, 6u, 4u, 7u, ia, ca);
    jf_want(jf, "REL scroll_x_shift on a pair with no shift fails", jf_eval(jf, "REL", "F0", "rel=scroll_x_shift;runs=a,b", "dx=1", runs, 2u, 0), VG_FAIL);
    jf_rows_stream(&b, 6u, 4u, 7u, ib_y, cb_y);
    jf_want(jf, "REL scroll_y_shift on a pair shifted by one line passes", jf_eval(jf, "REL", "F0", "rel=scroll_y_shift;runs=a,b", "dy=1", runs, 2u, 0), VG_PASS);
    jf_rows_stream(&b, 6u, 4u, 7u, ia, ca);
    jf_want(jf, "REL scroll_y_shift on a pair with no shift fails", jf_eval(jf, "REL", "F0", "rel=scroll_y_shift;runs=a,b", "dy=1", runs, 2u, 0), VG_FAIL);
    jf_rows_stream(&b, 6u, 4u, 7u, ia, cb_p);
    jf_want(jf, "REL palette_subst on a pair with one colour replaced passes", jf_eval(jf, "REL", "F0", "rel=palette_subst;runs=a,b", "from=203;to=300", runs, 2u, 0), VG_PASS);
    jf_rows_stream(&b, 6u, 4u, 7u, ia, ca);
    jf_want(jf, "REL palette_subst on a pair with no replacement fails", jf_eval(jf, "REL", "F0", "rel=palette_subst;runs=a,b", "from=203;to=300", runs, 2u, 0), VG_FAIL);
    jf_rows_stream(&b, 6u, 4u, 7u, ib_b, cb_b);
    jf_want(jf, "REL blank_is_border on a blank run passes", jf_eval(jf, "REL", "F0", "rel=blank_is_border;runs=a,b", "index=256", runs, 2u, 0), VG_PASS);
    jf_rows_stream(&b, 6u, 4u, 7u, ib_b2, cb_b);
    jf_want(jf, "REL blank_is_border on a run with one dot that is not blank fails", jf_eval(jf, "REL", "F0", "rel=blank_is_border;runs=a,b", "index=256", runs, 2u, 0), VG_FAIL);
    jf_rows_stream(&b, 8u, 3u, 7u, ib_w, cb_w);
    jf_want(jf, "REL dot_width_scale on widths 6 at divider 4 and 8 at divider 3 passes", jf_eval(jf, "REL", "F0", "rel=dot_width_scale;runs=a,b", "lines=3", runs, 2u, 0), VG_PASS);
    jf_want(jf, "REL dot_width_scale with F0 on lines fails the pair", jf_eval(jf, "REL", "F0", "rel=dot_width_scale;runs=a,b", "lines=3", runs, 2u, 1), VG_FAIL);
    jf_rows_stream(&b, 7u, 3u, 7u, ib_w, cb_w);
    jf_want(jf, "REL dot_width_scale on widths 6 at divider 4 and 7 at divider 3 fails", jf_eval(jf, "REL", "F0", "rel=dot_width_scale;runs=a,b", "lines=3", runs, 2u, 0), VG_FAIL);
    jf_init(&b); /* a run with no record: no relation holds */
    jf_want(jf, "REL a run with no record fails", jf_eval(jf, "REL", "F0", "rel=scroll_x_shift;runs=a,b", "dx=1", runs, 2u, 0), VG_FAIL);
    jf_rows_stream(&b, 6u, 4u, 7u, ia, ca);
    jf_want(jf, "REL a run that the case does not have is a fault of the case", jf_eval(jf, "REL", "F0", "rel=scroll_x_shift;runs=a,c", "dx=1", runs, 2u, 0), VG_HARNESS);
}

/* the null renderers: no record, all border, all index 0 never pass a form (D-172) */
static void jf_null(struct jf_ctx *jf, const struct vk_log *lg)
{
    struct jf_stream none;
    struct jf_stream border;
    struct jf_stream zero;
    struct vk_run run;

    jf_init(&none);
    jf_base(&border, 2, 256u);
    jf_base(&zero, 2, 0u);
    jf_run1(&run, &none, lg);
    jf_want(jf, "null renderer: a run with no record fails a ROWS row", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1,3", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "null renderer: a run with no record fails a STRUCT row", jf_eval(jf, "STRUCT", "F0", "check=count", "grid=1365", &run, 1u, 0), VG_FAIL);
    jf_run1(&run, &border, lg);
    jf_want(jf, "null renderer: an all-border run fails a ROWS index row", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1,3", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "null renderer: an all-border run fails a ROWS value row", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "value=104,106,101,103", &run, 1u, 0), VG_FAIL);
    jf_run1(&run, &zero, lg);
    jf_want(jf, "null renderer: an all-index-0 run fails a ROWS index row", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "index=4,6,1,3", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "null renderer: an all-index-0 run fails a ROWS value row", jf_eval(jf, "ROWS", "F0", "line=active+0;x=0..3", "value=104,106,101,103", &run, 1u, 0), VG_FAIL);
    jf_want(jf, "a sink that was never written is a fault of the harness", vk_sink_missing(0u, 0u, 12286u) == 1 && vk_sink_missing(10u, 10u, 12286u) == 0 && vk_sink_missing(0u, 0u, 0u) == 0, 1);
}

/* the sweep of the faults on hand-built records: a ROWS group that declares F0 F1 F2 F3 F4 F7 F8 is flipped by each; a STRUCT group that declares a pixel fault is vacuous; F6 widens a record by one dot and the stream stays well formed */
static void jf_sweep(struct jf_ctx *jf, const struct jf_stream *s, const struct vk_log *lg)
{
    struct vk_run run;
    struct vk_sweep sw;
    const uint8_t *dst[JF_MAXREC + 2u];
    uint8_t scr[2][VK_MAX_REC];
    struct vk_walk fw;
    uint64_t slen = 0u;
    uint8_t *stream;
    uint64_t total = 0u;
    uint64_t k;

    jf_run1(&run, s, lg);
    if (jf_parse(jf->vx, "ROWS", "ASSUMED:LED-VID-0010", "F0,F1,F2,F3,F4,F7,F8", "line=active+1;x=0..3", "index=7,2,4,6") != 0) {
        harness_error(jf->c, "the sweep group of video_judge_forms did not parse");
        return;
    }
    vk_sweep_group(jf->vx, 0u, &run, 1u, dst, scr, &sw);
    jf_want(jf, "sweep: a ROWS group is flipped by every fault F0 F1 F2 F3 F4 F7 F8 it declares", sw.baseline == VG_PASS && sw.declared == 7u && sw.flipped == 7u && sw.vacuous == 0u && sw.harness == 0u, 1);
    if (jf_parse(jf->vx, "STRUCT", "ASSUMED:LED-VID-0004", "F1,F4", "check=count", "grid=1365") != 0) {
        harness_error(jf->c, "the vacuous group of video_judge_forms did not parse");
        return;
    }
    vk_sweep_group(jf->vx, 0u, &run, 1u, dst, scr, &sw);
    jf_want(jf, "sweep: a group that a declared fault (F1 on a count row) leaves green is vacuous", sw.baseline == VG_PASS && sw.declared == 2u && sw.vacuous == 1u && sw.flipped == 1u, 1);
    fw.rec = dst;
    fw.n = 0u;
    fw.ok = 0;
    jf_want(jf, "sweep: F6 widens the record by one dot", vk_fault_apply(&s->w, &fw, 6u, 1u, scr) == 0 && vk_u16(fw.rec[1] + VK_O_ACTIVE_W) == 5u && vk_u16(fw.rec[1] + VK_O_REC_BYTES) == 88u && fw.n == s->w.n, 1);
    stream = vk_stream_of_walk(&fw, &slen);
    for (k = 0u; k < fw.n; k++) {
        total += (uint64_t)vk_u16(fw.rec[k] + VK_O_REC_BYTES);
    }
    jf_want(jf, "sweep: the widened stream keeps the records back to back (stream length 19 + the sum of rec_bytes)", stream != NULL && slen == 19u + total, 1);
    free(stream);
    {
        struct jf_stream ho;
        struct jf_line hl;

        jf_init(&ho);
        memset(&hl, 0, sizeof hl);
        hl.div = 4u;
        jf_add(&ho, 0u, 0u, 0u, 4u, &hl);
        jf_add(&ho, (uint64_t)VK_GRID, 0u, 1u, 4u, &hl);
        jf_want(jf, "sweep: a pixel fault cannot act on header-only records (F1 finds no active window)", vk_fault_apply(&ho.w, &fw, 1u, 0u, scr) == -1, 1);
    }
}

void kind_video_judge_forms(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct jf_ctx jf;
    struct jf_stream base;
    struct jf_stream skew;
    struct vk_log lg;
    struct vk_run run;
    unsigned bad = 0u;

    (void)k;
    jf.vx = (struct vx_file *)calloc(1u, sizeof *jf.vx);
    jf.c = c;
    jf.checks = 0u;
    jf.failed[0] = '\0';
    if (jf.vx == NULL) {
        harness_error(c, "out of memory for video_judge_forms");
        return;
    }
    jf_log(&lg);
    jf_base(&base, 0, 0u);
    jf_base(&skew, 1, 0u);
    jf_struct(&jf, &base, &skew, &lg);
    jf_rows(&jf, &base, &lg);
    jf_rel(&jf, &lg);
    jf_null(&jf, &lg);
    jf_sweep(&jf, &base, &lg);
    /* a whole scene on the base stream through the parser of the file: every STRUCT group passes (the groups are ASSUMED test data) */
    jf_run1(&run, &base, &lg);
    if (jf_parse(jf.vx, "STRUCT", "ASSUMED:LED-VID-0004", "F4", "check=count", "grid=1365") == 0) {
        jf_want(&jf, "a scene parsed from text passes through vk_scene_groups", vk_scene_groups(jf.vx, "jf", &run, 1u, &bad) == 1u && bad == 0u, 1);
    }
    snprintf(got, gotcap, "forms checks=%u", jf.checks);
    if (jf.failed[0] != '\0') {
        set_evidence_detail(jf.failed);
    }
    free(jf.vx);
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

/*
 * ---- vdc_irq_agree (plan 04-17; D-174, D-136, ER-3, ER-10, F-41; ASSUMED LED-IRQ-0001, LED-IRQ-0010, LED-VID-0001, LED-CPU-0009, LED-CPU-0017) ----
 *   vdc_irq_agree   expect = rom=NAME;id=N;gap=N;hit=N;next=N;tol=N;brk=N;row=N, request = the model binding of the case ("" or model.NAME=VALUE), keys until_mclk and checks.
 * The guest (fixtures/src/vdc_irq.s) records in guest RAM, as observe rows ROW to ROW + 3, the iterations (6 CPU cycles = 18 master clocks each) from the sample of the handler of the last display line RCR to that of the VBlank handler in two frames (rows ROW and ROW + 1) and from the VD sample of frame 0 to the
 * sample of the first RCR = 64 of frame 1 (rows ROW + 2, ROW + 3, low and high byte). This kind runs the ROM under the three default power-on policies, requires the DSV1 block to be a consistent pass (all binding-independent guest checks) and the same bytes under the three policies, takes the status reads of the VDC from the dev log of the first run
 * (the handler read: phys $1FE000, kind READ, with its access stamp; the read is 9 cycles into every measured handler, so two read stamps differ by the difference of the handler boundaries) and judges, in master clocks and never as an absolute line:
 *   - the model against the literals of the case: gap is the hand-derived time from the RCR event of the last display line to the VD event (245 with the default rows, 341 with vd_irq_phase = hdw_start, 1610 with rcr_origin = line_before: (2 + 31 + 2) * 8 * 4 = 1120 and 1365 - 1120 = 245, plus 24 * 4, plus one line) and hit is the time from the VD event to the RCR = 64
     event of the next frame; the VD read stamp minus the last-display-line read stamp
 *     is within brk of gap (the wait from an event to the instruction boundary that takes it is at least 1 and under 5 cycles, so two waits differ by under 4 cycles = 12 master clocks) and the stamp of the read of the first RCR = 64 handler of frame 1 (next reads after the VD read) minus the VD read stamp is within brk of hit;
 *   - the guest against the dev log: 18 * iterations is within brk of the read stamp difference (the difference of two handler boundaries is 6 * g cycles plus 0 or plus or minus 4, spin.inc), the instrument bracket;
 *   - the guest against the literals: 18 * iterations is within tol of gap or hit, tol = 27 master clocks = 1.5 iterations (the plan window of D-174 check 4), which is the sum of the two bounds above, 24, with a margin.
 * Evidence E0/E2: the model compared with hand-derived literals and with the guest's own count of machine time; the literals are modelling assumptions and not hardware facts. Hardware claim: none.
 * The guest not finishing, or any of its checks failing, is guest_failure (a toy that moves the RCR origin must turn this red); a lost dev record or a ROM that cannot be booted is harness_failure.
 */
#define IA_DEV_CAP 16384u
#define IA_PHYS_STATUS 0x1FE000u
#define IA_POLICIES 3u
#define IA_ITER_CLK 18u

struct ia_expect {
    char rom[64];
    uint64_t id;
    uint64_t gap;
    uint64_t hit;
    uint64_t next;
    uint64_t tol;
    uint64_t brk;
    uint64_t row;
    unsigned seen;
};

static int ia_parse(const char *text, struct ia_expect *e)
{
    char list[256];
    char *rest = list;
    char *item;
    static const char *const names[] = {"id", "gap", "hit", "next", "tol", "brk", "row"};
    uint64_t *slot[7];
    unsigned i;

    memset(e, 0, sizeof *e);
    slot[0] = &e->id;
    slot[1] = &e->gap;
    slot[2] = &e->hit;
    slot[3] = &e->next;
    slot[4] = &e->tol;
    slot[5] = &e->brk;
    slot[6] = &e->row;
    if (strlen(text) >= sizeof list) {
        return -1;
    }
    memcpy(list, text, strlen(text) + 1u);
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        int matched = 0;

        if (strncmp(item, "rom=", 4u) == 0) {
            if ((e->seen & 256u) != 0u || strlen(item + 4) == 0u || strlen(item + 4) >= sizeof e->rom) {
                return -1;
            }
            memcpy(e->rom, item + 4, strlen(item + 4) + 1u);
            e->seen |= 256u;
            continue;
        }
        for (i = 0u; i < 7u; i++) {
            size_t n = strlen(names[i]);

            if (strncmp(item, names[i], n) == 0 && item[n] == '=') {
                if ((e->seen & (1u << i)) != 0u || parse_u64(item + n + 1, slot[i]) != 0 || *slot[i] > UINT64_C(100000)) {
                    return -1;
                }
                e->seen |= 1u << i;
                matched = 1;
            }
        }
        if (!matched) {
            return -1;
        }
    }
    return e->seen == 383u && e->row >= 1u && e->row + 3u <= 96u && e->next >= 1u && e->next <= 255u && e->tol >= 1u && e->tol <= 54u && e->brk >= 1u && e->brk <= 24u ? 0 : -1;
}

/* run the ROM once over the request; dsk is the dev sink of the first run (NULL for the others); the DSV1 block is copied to blk; NULL on success, else a harness text */
static const char *ia_exec(struct ctx *c, const char *rom, const char *request, uint64_t until, struct ds_dev_sink *dsk, uint8_t *blk)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    const char *err = boot_built_rom(rom, request, &b, &ci);

    if (err != NULL) {
        return err;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.dev = dsk;
    (void)ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    note_run(c, res.machine_time, res.reason);
    if (res.reason != (uint32_t)DS_RUN_REASON_UNTIL) {
        release_boot(&b);
        return "a vdc_irq_agree run did not reach its deadline: ds_run refused the request or stopped for another reason";
    }
    memcpy(blk, ci->ram8k + V_OFF, V_SIZE);
    release_boot(&b);
    return NULL;
}

static int64_t ia_diff(uint64_t a, uint64_t b)
{
    return a >= b ? (int64_t)(a - b) : -(int64_t)(b - a);
}

static int64_t ia_abs(int64_t v)
{
    return v < 0 ? -v : v;
}

void kind_vdc_irq_agree(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct ia_expect e;
    uint64_t until = 0u;
    uint64_t checks = 0u;
    uint8_t blk[IA_POLICIES][V_SIZE];
    struct ds_dev_sink *dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    unsigned char *drec = (unsigned char *)calloc((size_t)IA_DEV_CAP, VK_DEV_REC);
    uint64_t *rst = (uint64_t *)calloc((size_t)IA_DEV_CAP, sizeof *rst);
    uint8_t *rval = (uint8_t *)calloc((size_t)IA_DEV_CAP, 1u);
    char req[256];
    char msg[360];
    const char *err = NULL;
    uint32_t count;
    uint32_t i;
    uint32_t nrd = 0u;
    uint32_t vd[2] = {0u, 0u};
    unsigned nvd = 0u;
    unsigned p;
    int64_t d1[2];
    int64_t d2;
    uint64_t g1[2];
    uint64_t g2;

    if (dsk == NULL || drec == NULL || rst == NULL || rval == NULL) {
        err = "out of memory for the dev sink of a vdc_irq_agree run";
    } else if (!k->has[K_EXPECT] || !k->has[K_UNTIL_MCLK] || !k->has[K_CHECKS] || !k->has[K_REQUEST] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || parse_u64(k->val[K_CHECKS], &checks) != 0 || ia_parse(k->val[K_EXPECT], &e) != 0 || checks > 96u) {
        err = "a vdc_irq_agree case needs until_mclk, checks, request and expect = rom=NAME;id=N;gap=N;hit=N;next=N;tol=N;brk=N;row=N (every item once, tol at most 54, brk at most 24)";
    }
    if (err == NULL) {
        dsk->records = drec;
        dsk->capacity = IA_DEV_CAP;
        for (p = 0u; p < IA_POLICIES && err == NULL; p++) {
            struct ctx s;
            char gotj[96];

            if (vd_policy_request(p, k->val[K_REQUEST], req, sizeof req) != 0) {
                err = "the request of a vdc_irq_agree case with a power-on policy does not fit";
                break;
            }
            err = ia_exec(c, e.rom, req, until, p == 0u ? dsk : NULL, blk[p]);
            if (err != NULL) {
                break;
            }
            memset(&s, 0, sizeof s);
            judge_dsv1(&s, blk[p], (unsigned)e.id, (unsigned)checks, gotj, sizeof gotj);
            if (s.harness) {
                err = "the DSV1 block of a vdc_irq_agree run is not consistent with the case (id or checks)";
            } else if (s.failed || s.timeout) {
                snprintf(msg, sizeof msg, "the guest under policy %s: %s", vd_policy_name(p), s.why);
                check(c, 0, msg);
                snprintf(got, gotcap, "guest not a pass under %s", vd_policy_name(p));
                err = "";
            }
        }
    }
    if (err != NULL && err[0] != '\0') {
        free(dsk);
        free(drec);
        free(rst);
        free(rval);
        harness_error(c, err);
        return;
    }
    if (err != NULL) { /* the guest itself failed: guest_failure, nothing more to judge */
        free(dsk);
        free(drec);
        free(rst);
        free(rval);
        return;
    }
    count = vk_u32(dsk->header);
    if (vk_u32(dsk->header + 4) != 0u) {
        free(dsk);
        free(drec);
        free(rst);
        free(rval);
        harness_error(c, "the dev sink of a vdc_irq_agree run lost a record: the status read stamps are the evidence here and would be incomplete");
        return;
    }
    for (i = 0u; i < count && i < IA_DEV_CAP; i++) {
        const unsigned char *r = drec + (size_t)i * VK_DEV_REC;

        if ((uint32_t)r[13] == (uint32_t)DS_DEV_READ && vk_u32(r + 8) == IA_PHYS_STATUS) {
            rst[nrd] = vk_u64(r);
            rval[nrd] = r[12];
            if ((r[12] & 0x20u) != 0u && nvd < 2u) {
                vd[nvd++] = nrd;
            }
            nrd++;
        }
    }
    free(dsk);
    free(drec);
    for (p = 1u; p < IA_POLICIES; p++) {
        snprintf(msg, sizeof msg, "the DSV1 block under policy %s differs from the one under zero: the measurement reads power-on state", vd_policy_name(p));
        check(c, memcmp(blk[0], blk[p], V_SIZE) == 0, msg);
    }
    check(c, nvd == 2u && vd[0] >= 1u && vd[1] >= 1u && (uint64_t)vd[0] + e.next < nrd, "the dev log has fewer than two status reads with the VD bit set, or no read after the first of them: the stamps cannot be taken");
    if (c->failed != 0) {
        free(rst);
        free(rval);
        snprintf(got, gotcap, "dev stamps missing");
        return;
    }
    d1[0] = ia_diff(rst[vd[0]], rst[vd[0] - 1u]);
    d1[1] = ia_diff(rst[vd[1]], rst[vd[1] - 1u]);
    d2 = ia_diff(rst[vd[0] + (uint32_t)e.next], rst[vd[0]]);
    g1[0] = blk[0][V_OBS + e.row - 1u];
    g1[1] = blk[0][V_OBS + e.row];
    g2 = (uint64_t)blk[0][V_OBS + e.row + 1u] | ((uint64_t)blk[0][V_OBS + e.row + 2u] << 8u);
    for (i = 0u; i < 2u; i++) {
        int64_t gc = (int64_t)(g1[i] * IA_ITER_CLK);

        snprintf(msg, sizeof msg, "frame %u: the model's last-display-line RCR to VD read stamp difference is %lld master clocks, the case literal is %llu within +-%llu (ASSUMED LED-IRQ-0001, LED-IRQ-0010)", (unsigned)i, (long long)d1[i], (unsigned long long)e.gap, (unsigned long long)e.brk);
        check(c, ia_abs(d1[i] - (int64_t)e.gap) <= (int64_t)e.brk, msg);
        snprintf(msg, sizeof msg, "frame %u: the guest counted %llu iterations (%lld master clocks) but the dev log says %lld, outside the instrument bracket of +-%llu", (unsigned)i, (unsigned long long)g1[i], (long long)gc, (long long)d1[i], (unsigned long long)e.brk);
        check(c, ia_abs(gc - d1[i]) <= (int64_t)e.brk, msg);
        snprintf(msg, sizeof msg, "frame %u: the guest counted %llu iterations (%lld master clocks) from the last-display-line RCR to VD, the window is %llu +-%llu", (unsigned)i, (unsigned long long)g1[i], (long long)gc, (unsigned long long)e.gap, (unsigned long long)e.tol);
        check(c, ia_abs(gc - (int64_t)e.gap) <= (int64_t)e.tol, msg);
    }
    {
        int64_t gc = (int64_t)(g2 * IA_ITER_CLK);

        snprintf(msg, sizeof msg, "the model's VD to first RCR = 64 read stamp difference of the next frame is %lld master clocks, the case literal is %llu within +-%llu (ASSUMED LED-IRQ-0010, LED-VID-0001)", (long long)d2, (unsigned long long)e.hit, (unsigned long long)e.brk);
        check(c, ia_abs(d2 - (int64_t)e.hit) <= (int64_t)e.brk, msg);
        snprintf(msg, sizeof msg, "the guest counted %llu iterations (%lld master clocks) from VD to the RCR = 64 of the next frame but the dev log says %lld, outside the instrument bracket of +-%llu", (unsigned long long)g2, (long long)gc, (long long)d2, (unsigned long long)e.brk);
        check(c, ia_abs(gc - d2) <= (int64_t)e.brk, msg);
        snprintf(msg, sizeof msg, "the guest counted %llu iterations (%lld master clocks) from VD to the RCR = 64 of the next frame, the window is %llu +-%llu", (unsigned long long)g2, (long long)gc, (unsigned long long)e.hit, (unsigned long long)e.tol);
        check(c, ia_abs(gc - (int64_t)e.hit) <= (int64_t)e.tol, msg);
    }
    snprintf(got, gotcap, "g1=%llu,%llu g2=%llu d1=%lld,%lld d2=%lld", (unsigned long long)g1[0], (unsigned long long)g1[1], (unsigned long long)g2, (long long)d1[0], (long long)d1[1], (long long)d2);
    free(rst);
    free(rval);
}
