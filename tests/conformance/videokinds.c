/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * videokinds.c - the video case kind (plan 04-04; D-137, D-138, D-141, D-161, D-163, D-169, D-171 first slice, ER-10, ER-11, ER-13, F-37). Test code only.
 *   video   boots a ROM that scripts/fixture-build assembled, attaches a vid sink and a dev sink, runs it with ds_run to the end of its DSV1 verdict (rq.stop = dsv1_done, until_mclk is the hard bound), judges the DSV1 block as the guest kind does, then reads the raw video records
 *           the core wrote and judges them. The records are parsed with the OFFSETS OF D-161 WRITTEN IN THIS FILE: the runner never includes a core record header and no expected value comes from the core, another emulator or a hash (ER-13, ER-11).
 *   expect  rom=NAME;id=N;role=literal;frame_lines=N;crw=V,V;div=D,D;border=N;gap=N  (every item once, in any order):
 *             rom, id        the built image build/gate/fixtures/NAME.pce and the fixture id its DSV1 block carries; the case also needs the keys until_mclk and checks (as the guest kind does)
 *             role           literal (the only role of the first slice; D-172 adds struct and golden): the case holds literal rows from the fixture source beside the digest
 *             frame_lines    the frame length the request binds (262), restated from the profile default, class ASSUMED LED-VID-0001
 *             crw            the values the guest writes to the VCE control register, in order (restated from the fixture source); the writes are found in the dev log by the register (offset & 7) of a physical address in the VCE window and their stamps are the join points
 *             div            the dot clock divider each CR value must give (MAN H6-3: content 00h division ratio 4, 01h division ratio 3), one per value of crw
 *             border         the 9-bit value of palette entry $100 the guest left (GUEST: pure in-guest arithmetic, 91 * 256 + 17 = 23313 = 45 * 512 + 273)
 *             gap            the least number of master clocks the guest leaves between two CR writes (the fixture's counted delay, at least four lines): a case whose measured gap is shorter is harness_failure (the window of judged records could be empty)
 *   Rows (ER-10: the weakest class of the inputs of each row):
 *     STRUCT, class ASSUMED LED-VID-0004 and LED-VID-0001 (every row uses the 1365-clock grid or the frame length): the record count is floor((M - 1) / 1365) + 1 for the machine time M the run reached; origin_k = k * 1365; the frame and the line are those of (frame * frame_lines + line) * 1365 = origin;
 *       frame_lines, line_mclk, version, rec_bytes (64 + ((4 * active_w + 7) & ~7)) and the zero reserved word hold on every record; every colour value is below 512.
 *     div, class MAN H6-3: every record that lies wholly at least one line after the stamp of CR write j and, when another CR write follows, at least one line before it (origin >= t_j + 1365 and origin + 2 * 1365 <= t_(j+1)) carries the divider of crw j. Such a record does not depend on the grid phase or on the tie rule; the window of each
 *       write must hold at least one record (the case is harness_failure if it is empty).
 *     first change, class ASSUMED LED-VID-0004 (the tie rule of D-138: an event at or before the stamp of an access precedes it): the first record whose origin is after the stamp of the second CR write carries the new divider and the record before it the old one.
 *     line_dirty, class ASSUMED LED-VID-0004 (the join of a write stamp to the grid): bit 0 is set on exactly the first record after each CR write and on no other record, bit 4 on exactly the first record after each palette data write (register 4 or 5 of the window, mirrors included) and on no other record.
 *     border, class GUEST (pal_bijection): every record whose origin is at least one line after the last palette write carries the border the case lists; the window must hold at least one record.
 *   The evidence text carries video_sha256=HEX E0 repeatability: the SHA-256 of the stream "duospool-video" NUL, u32 1 (little-endian), then the records back to back (D-163). A hash is a change detector and never a verdict (ER-11): the literal rows above decide the case.
 *   A lost record of either sink, a missing sink, an irq_stim key, a measured gap below the case's gap and an empty judged window are harness_failure.
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
    int role_literal;
    uint64_t frame_lines;
    unsigned ncrw;
    unsigned crw[VK_CRW_MAX];
    unsigned ndiv;
    unsigned div[VK_CRW_MAX];
    uint64_t border;
    uint64_t gap;
    unsigned seen;
};

enum { VKI_ROM = 1, VKI_ID = 2, VKI_ROLE = 4, VKI_FL = 8, VKI_CRW = 16, VKI_DIV = 32, VKI_BORDER = 64, VKI_GAP = 128, VKI_ALL = 255 };

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

        if (*n >= cap || parse_u64(item, &v) != 0 || v > 255u) {
            return -1;
        }
        out[(*n)++] = (unsigned)v;
    }
    return *n >= 1u ? 0 : -1;
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
        } else if (strcmp(item, "role") == 0) {
            bit = VKI_ROLE;
            if (strcmp(val, "literal") != 0) {
                return -1;
            }
            e->role_literal = 1;
        } else if (strcmp(item, "frame_lines") == 0) {
            bit = VKI_FL;
            if (parse_u64(val, &e->frame_lines) != 0 || e->frame_lines == 0u || e->frame_lines > 1000u) {
                return -1;
            }
        } else if (strcmp(item, "crw") == 0) {
            bit = VKI_CRW;
            if (vk_list(val, e->crw, VK_CRW_MAX, &e->ncrw) != 0) {
                return -1;
            }
        } else if (strcmp(item, "div") == 0) {
            bit = VKI_DIV;
            if (vk_list(val, e->div, VK_CRW_MAX, &e->ndiv) != 0) {
                return -1;
            }
        } else if (strcmp(item, "border") == 0) {
            bit = VKI_BORDER;
            if (parse_u64(val, &e->border) != 0 || e->border > 511u) {
                return -1;
            }
        } else if (strcmp(item, "gap") == 0) {
            bit = VKI_GAP;
            if (parse_u64(val, &e->gap) != 0 || e->gap == 0u) {
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
    return e->seen == (unsigned)VKI_ALL && e->ncrw == e->ndiv && e->ncrw >= 2u ? 0 : -1;
}

/* ---- the rows, each its own function (F-37) ---- */

/* the number of lines whose event lies at or before machine time M - 1: the closed form of D-137 (ASSUMED LED-VID-0004, LED-VID-0001), written here and nowhere else */
static uint64_t vk_count_closed_form(uint64_t m)
{
    return m == 0u ? 0u : (m - 1u) / (uint64_t)VK_GRID + 1u;
}

/* the index of the first line whose origin is after the stamp t, on the grid k * 1365 (ASSUMED LED-VID-0004: an event at or before the stamp precedes the access) */
static uint64_t vk_first_after(uint64_t t)
{
    return t / (uint64_t)VK_GRID + 1u;
}

/* the origin of record k by the rule (ASSUMED LED-VID-0004) */
static uint64_t vk_origin_rule(uint64_t k)
{
    return k * (uint64_t)VK_GRID;
}

struct vk_log {
    uint64_t t_cr[VK_CRW_MAX];
    unsigned v_cr[VK_CRW_MAX];
    unsigned n_cr;
    uint64_t t_pal[4096];
    unsigned n_pal;
    uint64_t t_pal_last;
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

static void vk_row_count(struct ctx *c, uint64_t n, uint64_t m)
{
    char msg[200];

    snprintf(msg, sizeof msg, "STRUCT count: the sink holds %llu records, the closed form floor((M - 1) / 1365) + 1 of the machine time %llu is %llu", (unsigned long long)n, (unsigned long long)m, (unsigned long long)vk_count_closed_form(m));
    check(c, n == vk_count_closed_form(m), msg);
}

static void vk_row_origin(struct ctx *c, const struct vk_walk *w)
{
    uint64_t k;
    uint64_t bad = w->n;

    for (k = 0u; k < w->n; k++) {
        if (vk_u64(w->rec[k] + VK_O_ORIGIN) != vk_origin_rule(k)) {
            bad = k;
            break;
        }
    }
    check(c, bad == w->n, "STRUCT origin: a record does not start at k * 1365 (first bad index is the one the case aborts on)");
}

static void vk_row_frame_line(struct ctx *c, const struct vk_walk *w, uint64_t fl)
{
    uint64_t k;
    int ok = 1;

    for (k = 0u; k < w->n && ok; k++) {
        const uint8_t *r = w->rec[k];
        uint64_t frame = (uint64_t)vk_u32(r + VK_O_FRAME);
        uint64_t line = (uint64_t)vk_u16(r + VK_O_LINE);

        ok = (uint64_t)vk_u16(r + VK_O_FRAME_LINES) == fl && line < fl && (frame * fl + line) * (uint64_t)VK_GRID == vk_u64(r + VK_O_ORIGIN);
    }
    check(c, ok, "STRUCT frame and line: a record has another frame_lines than the case, a line outside its frame, or (frame * frame_lines + line) * 1365 differs from its origin");
}

static void vk_row_shape(struct ctx *c, const struct vk_walk *w)
{
    uint64_t k;
    int ok = 1;
    int zero = 1;

    for (k = 0u; k < w->n; k++) {
        const uint8_t *r = w->rec[k];
        uint32_t aw = vk_u16(r + VK_O_ACTIVE_W);

        ok = ok && aw <= VK_MAX_DOTS && vk_u16(r + VK_O_REC_BYTES) == VK_HDR + ((4u * aw + 7u) & ~7u) && vk_u16(r + VK_O_LINE_MCLK) == VK_GRID && (unsigned)r[VK_O_VERSION] == 1u;
        zero = zero && vk_u64(r + VK_O_RESERVED) == 0u;
    }
    check(c, ok, "STRUCT shape: a record has rec_bytes other than 64 + ((4 * active_w + 7) & ~7), line_mclk other than 1365, version other than 1, or active_w above 682");
    check(c, zero, "STRUCT reserved: a record has a nonzero reserved word");
}

static void vk_row_values(struct ctx *c, const struct vk_walk *w)
{
    uint64_t k;
    int ok = 1;

    for (k = 0u; k < w->n; k++) {
        const uint8_t *r = w->rec[k];
        uint32_t aw = vk_u16(r + VK_O_ACTIVE_W);
        uint32_t j;

        ok = ok && vk_u16(r + VK_O_BORDER) < 512u;
        for (j = 0u; j < 2u * aw && ok; j++) { /* the index array and the colour array: every entry is a 9-bit value */
            ok = vk_u16(r + VK_HDR + 2u * j) < 512u;
        }
    }
    check(c, ok, "STRUCT values: a border, index or colour value is not below 512");
}

/* div rows (MAN H6-3): the records wholly inside the window of CR write j carry the divider of that value; returns the number of records judged */
static uint64_t vk_row_div(struct ctx *c, const struct vk_walk *w, const struct vk_log *lg, const struct vk_expect *e, unsigned j)
{
    uint64_t k;
    uint64_t judged = 0u;
    int ok = 1;
    char msg[160];

    for (k = 0u; k < w->n; k++) {
        uint64_t o = vk_u64(w->rec[k] + VK_O_ORIGIN);

        if (o < lg->t_cr[j] + (uint64_t)VK_GRID) {
            continue;
        }
        if (j + 1u < e->ncrw && o + 2u * (uint64_t)VK_GRID > lg->t_cr[j + 1u]) {
            continue;
        }
        judged++;
        ok = ok && (unsigned)w->rec[k][VK_O_DIV] == e->div[j];
    }
    snprintf(msg, sizeof msg, "div (MAN H6-3): a record at least one line after CR write %u (value %u) and before the next one does not carry divider %u", j + 1u, e->crw[j], e->div[j]);
    if (judged >= 1u) { /* an empty window is a fault of the case, reported by the caller as harness_failure */
        check(c, ok, msg);
    }
    return judged;
}

/* the first change (ASSUMED LED-VID-0004, tie rule D-138): the first record after the stamp of CR write j carries divider j, the record before it divider j - 1 */
static void vk_row_first_change(struct ctx *c, const struct vk_walk *w, const struct vk_log *lg, const struct vk_expect *e, unsigned j)
{
    uint64_t r1 = vk_first_after(lg->t_cr[j]);
    char msg[200];

    snprintf(msg, sizeof msg, "first change (ASSUMED LED-VID-0004): the first record after CR write %u does not carry divider %u, or the record before it does not carry %u", j + 1u, e->div[j], e->div[j - 1u]);
    check(c, r1 >= 1u && r1 < w->n && (unsigned)w->rec[r1][VK_O_DIV] == e->div[j] && (unsigned)w->rec[r1 - 1u][VK_O_DIV] == e->div[j - 1u], msg);
}

/* the line_dirty rows (ASSUMED LED-VID-0004): bit 0 on exactly the first record after each CR write, bit 4 on exactly the first record after each palette data write */
static void vk_row_dirty(struct ctx *c, const struct vk_walk *w, const struct vk_log *lg)
{
    uint64_t k;
    uint64_t j;
    int ok0 = 1;
    int ok4 = 1;
    uint64_t *want0 = (uint64_t *)calloc((size_t)(w->n / 64u + 1u), sizeof(uint64_t));
    uint64_t *want4 = (uint64_t *)calloc((size_t)(w->n / 64u + 1u), sizeof(uint64_t));

    if (want0 == NULL || want4 == NULL) {
        free(want0);
        free(want4);
        harness_error(c, "out of memory for the line_dirty rows");
        return;
    }
    for (j = 0u; j < (uint64_t)lg->n_cr; j++) {
        uint64_t r = vk_first_after(lg->t_cr[j]);

        if (r < w->n) {
            want0[r / 64u] |= UINT64_C(1) << (r % 64u);
        }
    }
    for (j = 0u; j < (uint64_t)lg->n_pal; j++) {
        uint64_t r = vk_first_after(lg->t_pal[j]);

        if (r < w->n) {
            want4[r / 64u] |= UINT64_C(1) << (r % 64u);
        }
    }
    for (k = 0u; k < w->n; k++) {
        unsigned d = (unsigned)w->rec[k][VK_O_LINE_DIRTY];
        unsigned w0 = (unsigned)((want0[k / 64u] >> (k % 64u)) & 1u);
        unsigned w4 = (unsigned)((want4[k / 64u] >> (k % 64u)) & 1u);

        ok0 = ok0 && (d & 1u) == w0;
        ok4 = ok4 && ((d >> 4) & 1u) == w4;
    }
    free(want0);
    free(want4);
    check(c, ok0, "line_dirty bit 0 (ASSUMED LED-VID-0004): it is not set on exactly the first record after each CR write");
    check(c, ok4, "line_dirty bit 4 (ASSUMED LED-VID-0004): it is not set on exactly the first record after each palette data write");
}

/* border (GUEST pal_bijection): every record at least one line after the last palette write; returns the number of records judged */
static uint64_t vk_row_border(struct ctx *c, const struct vk_walk *w, const struct vk_log *lg, uint64_t want)
{
    uint64_t k;
    uint64_t judged = 0u;
    int ok = 1;

    for (k = 0u; k < w->n; k++) {
        if (vk_u64(w->rec[k] + VK_O_ORIGIN) >= lg->t_pal_last + (uint64_t)VK_GRID) {
            judged++;
            ok = ok && (uint64_t)vk_u16(w->rec[k] + VK_O_BORDER) == want;
        }
    }
    if (judged >= 1u) {
        check(c, ok, "border (GUEST pal_bijection): a record at least one line after the last palette write does not carry the border the guest left (the case lists 91 * 256 + 17 mod 512)");
    }
    return judged;
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

void kind_video(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct vk_expect e;
    uint64_t until = 0u;
    uint64_t checks = 0u;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_dev_sink *dsk = NULL;
    struct ds_vid_sink *vsk = NULL;
    uint8_t *drec = NULL;
    uint8_t *vbuf = NULL;
    const uint8_t **recp = NULL;
    uint64_t lines_due;
    uint64_t vcap;
    const char *err;
    char verdict[64] = "";
    struct vk_walk w;
    struct vk_log lg;

    if (!k->has[K_EXPECT] || vk_parse(k->val[K_EXPECT], &e) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || !k->has[K_CHECKS] || parse_u64(k->val[K_CHECKS], &checks) != 0 || checks == 0u || checks > 255u) {
        harness_error(c, "a video case needs until_mclk, checks and expect = rom=NAME;id=N;role=literal;frame_lines=N;crw=V,V;div=D,D;border=N;gap=N (every item once, at least two CR writes)");
        return;
    }
    if (k->has[K_IRQ_STIM]) {
        harness_error(c, "a video case declares no irq_stim: the interrupt test hook is not part of a video scene");
        return;
    }
    /* the sink sizes from the closed form of the deadline: one line more than the lines due at until_mclk, at the longest record (D-162) */
    lines_due = vk_count_closed_form(until) + 1u;
    vcap = lines_due * (uint64_t)VK_MAX_REC;
    if (vcap > (uint64_t)DS_VID_SINK_MAX_BYTES) {
        harness_error(c, "the video case is too long for one vid sink (until_mclk needs more than 64 MiB)");
        return;
    }
    dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    vsk = (struct ds_vid_sink *)calloc(1u, sizeof *vsk);
    drec = (uint8_t *)calloc((size_t)VK_DEV_CAP, (size_t)VK_DEV_REC);
    vbuf = (uint8_t *)calloc((size_t)vcap, 1u);
    recp = (const uint8_t **)calloc((size_t)lines_due, sizeof *recp);
    if (dsk == NULL || vsk == NULL || drec == NULL || vbuf == NULL || recp == NULL) {
        harness_error(c, "out of memory for the sinks of a video case");
        goto done;
    }
    err = boot_built_rom(e.rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        goto done;
    }
    dsk->records = drec;
    dsk->capacity = VK_DEV_CAP;
    vsk->bytes = vbuf;
    vsk->capacity = (uint32_t)vcap;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.stop = dsv1_done; /* a speed aid only (D-95): until_mclk is the hard bound */
    rq.dev = dsk;
    rq.vid = vsk;
    if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of a video case");
        release_boot(&b);
        goto done;
    }
    note_run(c, res.machine_time, res.reason);
    judge_dsv1(c, ci->ram8k + V_OFF, (unsigned)e.id, (unsigned)checks, verdict, sizeof verdict);
    release_boot(&b);
    snprintf(got, gotcap, "%s", verdict);
    if (c->harness || c->timeout || c->failed) {
        goto done;
    }
    {
        uint32_t vcount = vk_u32(vsk->header);
        uint32_t vlost = vk_u32(vsk->header + 4);
        uint64_t vseq = vk_u64(vsk->header + 8);
        uint64_t vbytes = vk_u64(vsk->header + 16);
        uint32_t dcount = vk_u32(dsk->header);
        uint32_t dlost = vk_u32(dsk->header + 4);
        uint8_t digest[DS_SHA256_BYTES];
        char hex[2u * DS_SHA256_BYTES + 1u];
        char detail[1024];
        uint8_t *stream;
        uint64_t judged_border;
        unsigned j;
        unsigned man = 0u;
        unsigned guest = 0u;
        unsigned assumed = 0u;

        if (vlost != 0u || dlost != 0u || vseq != (uint64_t)vcount || vbytes > vcap) {
            harness_error(c, "a sink of the video case lost a record or its header is inconsistent: the stream would be incomplete evidence (D-162)");
            goto done;
        }
        if ((uint64_t)vcount > lines_due) {
            check(c, 0, "STRUCT count: the vid sink holds more records than the lines due at the deadline plus one");
            goto done;
        }
        w.rec = recp;
        vk_walk_records(vbuf, vbytes, (uint64_t)vcount, &w);
        if (!w.ok) {
            check(c, 0, "STRUCT walk: the records of the vid sink do not tile the stored bytes (a rec_bytes below 64, above the longest record or past the end)");
            goto done;
        }
        if (vk_scan_dev(drec, dcount, &lg) != 0 || lg.n_cr != e.ncrw) {
            harness_error(c, "the dev log does not hold exactly the CR writes the case lists, or too many palette writes");
            goto done;
        }
        for (j = 0u; j < e.ncrw; j++) {
            if (lg.v_cr[j] != e.crw[j] || (j > 0u && lg.t_cr[j] - lg.t_cr[j - 1u] < e.gap)) {
                harness_error(c, "a CR write of the guest has another value than the case lists, or two CR writes are closer than the gap of the case (the window of judged records could be empty)");
                goto done;
            }
        }
        /* STRUCT rows, ASSUMED LED-VID-0004 and LED-VID-0001 */
        vk_row_count(c, w.n, res.machine_time);
        vk_row_origin(c, &w);
        vk_row_frame_line(c, &w, e.frame_lines);
        vk_row_shape(c, &w);
        vk_row_values(c, &w);
        assumed += 6u;
        /* div rows, MAN H6-3, and the first change, ASSUMED LED-VID-0004 */
        for (j = 0u; j < e.ncrw; j++) {
            uint64_t judged = vk_row_div(c, &w, &lg, &e, j);

            if (judged == 0u) {
                harness_error(c, "the window of a CR write holds no record: the case cannot judge its divider");
                goto done;
            }
            man++;
        }
        for (j = 1u; j < e.ncrw; j++) {
            if (e.div[j] != e.div[j - 1u]) {
                vk_row_first_change(c, &w, &lg, &e, j);
                assumed++;
            }
        }
        vk_row_dirty(c, &w, &lg);
        assumed += 2u;
        judged_border = vk_row_border(c, &w, &lg, e.border);
        if (judged_border == 0u) {
            harness_error(c, "no record lies a whole line after the last palette write: the case cannot judge the border");
            goto done;
        }
        guest++;
        /* the stream of D-163 and its digest: E0 repeatability, printed beside the literal verdict and never a verdict itself (ER-11) */
        stream = (uint8_t *)malloc((size_t)(19u + vbytes));
        if (stream == NULL) {
            harness_error(c, "out of memory for the video stream");
            goto done;
        }
        memcpy(stream, "duospool-video", 14u);
        stream[14] = 0u;
        stream[15] = 1u;
        stream[16] = 0u;
        stream[17] = 0u;
        stream[18] = 0u;
        memcpy(stream + 19, vbuf, (size_t)vbytes);
        ds_sha256(stream, (size_t)(19u + vbytes), digest);
        free(stream);
        vk_hex(digest, hex);
        snprintf(detail, sizeof detail, "video_sha256=%s E0 repeatability (a change detector, never a verdict, ER-11)\nvideo_records=%llu video_bytes=%llu machine_time=%llu\nvideo_rows: man=%u guest=%u assumed=%u (class of each row in the case comment, ER-10)\n", hex,
                 (unsigned long long)w.n, (unsigned long long)vbytes, (unsigned long long)res.machine_time, man, guest, assumed);
        set_evidence_detail(detail);
        snprintf(got + strlen(got), gotcap - strlen(got), " records=%llu", (unsigned long long)w.n);
    }
done:
    free(dsk);
    free(vsk);
    free(drec);
    free(vbuf);
    free((void *)recp);
}
