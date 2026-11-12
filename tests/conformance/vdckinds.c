/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * vdckinds.c - the VDC and VCE C-level kinds (plan 04-05; D-137, D-138, D-162, D-163, D-164, D-175, F-2, F-21, F-29, F-31, ER-7, ER-10). Test code only. Evidence tier E0: the core compared with itself (repeatability, composability, non-interference) plus a few rows whose expected values are written in
 * this file from a rule, not read from the core (the closed form of the line count and the origin grid, class ASSUMED LED-VID-0004 and LED-VID-0001; the dot clock divider of CR 0 and 1, class MAN H6-3). No accuracy, raster, pixel, hardware or compatibility claim; the word pass is not a support claim.
 *   vdc_fold                  expect = rom=NAME;parts=partitions,tie,invariant;partitions=N;frames=N;frame=N;seed=N;tie_line=N (every item once; until_mclk is needed for the invariant part).
 *                             partitions  N seeded random partitions of the stamp range [0, frames * frame] (frame 357630 = 262 * 1365) are folded through direct ds_vdc_advance calls on a booted instance of the fixture and must equal ONE jump to the end in every restorable member (snap_first_diff: the beam
 *                                         bytes among them), in the digest and in the emitted records (memcmp of the stream bytes); odd partitions cut at one clock before, at and one clock after the origin of random lines (the fold target itself sits on the tie, D-137), even ones at random stamps. The number of records is the
 *                                         closed form floor(S / 1365) + 1 of the last stamp S, written here (no code shared with the core).
 *                             tie         a VCE control register write through the bus write path (ds_bus_write with the access stamp of the case, so the fold-then-write order is the production one, D-137): written at origin - 1 of line tie_line the value shows on that line, at origin and at origin + 1 it shows on the next
 *                                         line (the tie rule of D-138, ASSUMED LED-VID-0004, and the divider values of CR 0 and 1, MAN H6-3); line_dirty bit 0 is on exactly the record that carries the change.
 *                             invariant   the fixture runs under a stop callback that is called before the first step and after every step: at every call the pending event time (ds_vdc_pending_time, valid) is at least the machine time (D-137: no event older than machine_time) and the next line index equals the closed
 *                                         form of the lines due at the machine time; the same run is repeated in 64 calls to seeded random deadlines, so the property holds after every ds_run stop. A pure query is made only at a folded point and never ds_irq_pending_query (F-21).
 *   vid_noninterference       expect = rom=NAME;compare=verdict,machine_time,steps,digest (until_mclk needed): for each of the three power-on policies (zero, seeded-prng-v3 seeds 1 and 2) the fixture runs with no vid sink and with a vid sink only in four shapes (large, 4 records drop-newest taken from the sizes of the large
 *                             run, capacity 0 and capacity 1): the machine time, the steps, the verdict RAM and the digest equal those of the run with no sink, the header counts whole records stored, lost = seq - count, bytes is their sum, seq is the closed-form line count, and the bytes past the count are untouched (D-164, F-2).
 *   snapshot_restore_video    expect = rom=NAME;schedules=1364,1366,7;stride=N (until_mclk needed): the fixture is run on each schedule (calls to the deadlines j * size) with a vid sink on every call; at every stop of the schedules 1364 and 1366, which fall at every phase inside a line, and at every stride-th stop and
 *                             at every stop whose machine time is a multiple of 1365 (a pending line event at the machine time itself) of the schedule 7, the machine is snapshotted, restored into a fresh poisoned boot of the same image (member by member equal to the source) and the run goes on with the target. The concatenated
 *                             stream bytes, the digest, the machine time, the steps and the verdict RAM of every schedule equal those of the uninterrupted run (D-137). A case that restored at no stop at a multiple of 1365 is a guest_failure: the tie would be untested.
 * The policy list of this file (zero, seeded-prng-v3 seed 1, seeded-prng-v3 seed 2) is the default list of a guest case (D-167); vd_policy_request appends a fragment to the request of a case as kind_guest does.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_serial.h"
#include "ds_snapshot.h"
#include "ds_vdc.h"
#include "runner.h"

#define VD_GRID 1365u            /* master clocks per line (ASSUMED LED-VID-0004), written in this file */
#define VD_REC_MAX 2792u         /* the longest record (D-161) */
#define VD_REC_MIN 64u
#define VD_O_ORIGIN 0u           /* the layout of D-161 as this file knows it (hand-written, ER-13) */
#define VD_O_REC_BYTES 16u
#define VD_O_DIV 20u
#define VD_O_LINE_DIRTY 32u
#define VD_PART_MAX 256u
#define VD_CUTS_MAX 48u
#define VD_RUN_CHUNKS 64u
#define VD_POISON 0xA5u
#define VD_GUARD 64u
#define VD_DIV_CR0 4u            /* MAN H6-3: the control register content 00h gives the division ratio 4 */
#define VD_DIV_CR1 3u            /* MAN H6-3: the content 01h gives the division ratio 3 */
#define VD_VCE_CR_ADDR 0x0400u   /* logical address of the VCE control register with MPR0 = $FF (the window 1 offset $400, register 0) */

/* ---- the three policies, shared with the observer kind ---- */

static const char *const vd_policies[VD_POLICIES] = {"", "bind.powerup_policy=seeded-prng-v3;seed=1", "bind.powerup_policy=seeded-prng-v3;seed=2"};

const char *vd_policy_name(unsigned i)
{
    return i < VD_POLICIES ? (vd_policies[i][0] == '\0' ? "zero" : vd_policies[i]) : "";
}

/* the request of a case with a policy fragment appended with a semicolon (an empty fragment is the zero policy, an empty base takes the fragment alone); 0 on success, -1 when it does not fit */
int vd_compose_request(const char *base, const char *fragment, char *out, size_t cap)
{
    int n;

    if (base[0] == '\0') {
        n = snprintf(out, cap, "%s", fragment);
    } else if (fragment[0] == '\0') {
        n = snprintf(out, cap, "%s", base);
    } else {
        n = snprintf(out, cap, "%s;%s", base, fragment);
    }
    return n >= 0 && (size_t)n < cap ? 0 : -1;
}

/* the request of a case with the default policy fragment i appended */
int vd_policy_request(unsigned i, const char *base, char *out, size_t cap)
{
    return i < VD_POLICIES ? vd_compose_request(base, vd_policies[i], out, cap) : -1;
}

/* ---- small helpers ---- */

static uint32_t vd_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t vd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t vd_u64(const uint8_t *p)
{
    return (uint64_t)vd_u32(p) | ((uint64_t)vd_u32(p + 4) << 32);
}

/* the number of lines due at the machine time M: floor((M - 1) / 1365) + 1, 0 for M = 0 (D-137, ASSUMED LED-VID-0004 and LED-VID-0001); shares no code with the core */
static uint64_t vd_lines_due(uint64_t m)
{
    return m == 0u ? 0u : (m - 1u) / (uint64_t)VD_GRID + 1u;
}

/* splitmix64 (the generator of the runner's power-on kinds), a fixed seed per case so a partition is a function of the case alone */
static uint64_t vd_mix(uint64_t *state)
{
    uint64_t z;

    *state = *state + UINT64_C(0x9E3779B97F4A7C15);
    z = *state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static int vd_cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;

    return x < y ? -1 : (x > y ? 1 : 0);
}

/* the vid stream of a run: a per-call record memory and the accumulation of what the calls stored */
struct vd_stream {
    uint8_t *call;
    uint8_t *acc;
    uint32_t cap;
    uint64_t bytes;
    uint64_t count;
    uint64_t seq;
    int bad; /* a call lost a record, or its header disagreed with the records it stored */
};

/* the byte capacity of a call that cannot lose a record: the closed form of the lines of a span of at most until clocks, plus a step and the line before it, at the longest record; 0 when it exceeds the sink maximum */
static uint32_t vd_stream_cap(uint64_t until)
{
    uint64_t cap = (until / (uint64_t)VD_GRID + 4u) * (uint64_t)VD_REC_MAX;

    return cap > (uint64_t)DS_VID_SINK_MAX_BYTES ? 0u : (uint32_t)cap;
}

static int vd_stream_make(struct vd_stream *s, uint32_t cap)
{
    memset(s, 0, sizeof *s);
    s->cap = cap;
    s->call = (uint8_t *)malloc((size_t)cap);
    s->acc = (uint8_t *)malloc((size_t)cap);
    if (s->call == NULL || s->acc == NULL) {
        free(s->call);
        free(s->acc);
        memset(s, 0, sizeof *s);
        return -1;
    }
    return 0;
}

static void vd_stream_free(struct vd_stream *s)
{
    free(s->call);
    free(s->acc);
    memset(s, 0, sizeof *s);
}

/* a fresh sink over the call memory */
static void vd_stream_sink(struct vd_stream *s, struct ds_vid_sink *vsk)
{
    memset(vsk, 0, sizeof *vsk);
    vsk->bytes = s->call;
    vsk->capacity = s->cap;
}

/* append what the call stored: the header is read by hand (D-162); a lost record or an inconsistent header sets bad */
static void vd_stream_take(struct vd_stream *s, const struct ds_vid_sink *vsk)
{
    uint32_t cnt = vd_u32(vsk->header);
    uint32_t lost = vd_u32(vsk->header + 4);
    uint64_t seq = vd_u64(vsk->header + 8);
    uint64_t nb = vd_u64(vsk->header + 16);

    if (lost != 0u || (uint64_t)cnt != seq || nb > (uint64_t)s->cap - s->bytes || vd_u64(vsk->header + 24) != 0u) {
        s->bad = 1;
        return;
    }
    if (nb > 0u) { /* a copy of zero bytes still must not get a null pointer */
        memcpy(s->acc + s->bytes, s->call, (size_t)nb);
    }
    s->bytes += nb;
    s->count += (uint64_t)cnt;
    s->seq += seq;
}

/* the records of a stream: pointers to each, by walking the rec_bytes field; 0 when the walk lands exactly on the byte count */
static int vd_walk(const uint8_t *bytes, uint64_t stored, uint64_t count, const uint8_t **rec)
{
    uint64_t off = 0u;
    uint64_t k;

    for (k = 0u; k < count; k++) {
        uint32_t rb;

        if (off + (uint64_t)VD_REC_MIN > stored) {
            return -1;
        }
        rb = vd_u16(bytes + off + VD_O_REC_BYTES);
        if (rb < VD_REC_MIN || rb > VD_REC_MAX || off + (uint64_t)rb > stored) {
            return -1;
        }
        rec[k] = bytes + off;
        off += (uint64_t)rb;
    }
    return off == stored ? 0 : -1;
}

/* ---- kind vdc_fold ---- */

struct vf_expect {
    char rom[48];
    unsigned parts;
    unsigned partitions;
    unsigned frames;
    uint64_t frame;
    uint64_t seed;
    uint64_t tie_line;
    unsigned seen;
};

enum { VF_PART_PARTS = 1, VF_PART_TIE = 2, VF_PART_INV = 4 };
enum { VFI_ROM = 1, VFI_PARTS = 2, VFI_PARTITIONS = 4, VFI_FRAMES = 8, VFI_FRAME = 16, VFI_SEED = 32, VFI_TIE = 64, VFI_ALL = 127 };

static int vf_parse_parts(char *text, unsigned *out)
{
    char *rest = text;
    char *w;

    *out = 0u;
    while ((w = pk_next_item(&rest, ',')) != NULL) {
        unsigned bit = strcmp(w, "partitions") == 0 ? (unsigned)VF_PART_PARTS : strcmp(w, "tie") == 0 ? (unsigned)VF_PART_TIE : strcmp(w, "invariant") == 0 ? (unsigned)VF_PART_INV : 0u;

        if (bit == 0u || (*out & bit) != 0u) {
            return -1;
        }
        *out |= bit;
    }
    return *out != 0u ? 0 : -1;
}

static int vf_parse(const char *text, struct vf_expect *e)
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
        char *val;
        unsigned bit = 0u;
        uint64_t v = 0u;

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        val = eq + 1;
        if (strcmp(item, "rom") == 0) {
            bit = VFI_ROM;
            if (strlen(val) == 0u || strlen(val) >= sizeof e->rom || strspn(val, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(val)) {
                return -1;
            }
            memcpy(e->rom, val, strlen(val) + 1u);
        } else if (strcmp(item, "parts") == 0) {
            bit = VFI_PARTS;
            if (vf_parse_parts(val, &e->parts) != 0) {
                return -1;
            }
        } else if (strcmp(item, "partitions") == 0) {
            bit = VFI_PARTITIONS;
            if (parse_u64(val, &v) != 0 || v < 1u || v > (uint64_t)VD_PART_MAX) {
                return -1;
            }
            e->partitions = (unsigned)v;
        } else if (strcmp(item, "frames") == 0) {
            bit = VFI_FRAMES;
            if (parse_u64(val, &v) != 0 || v < 1u || v > 16u) {
                return -1;
            }
            e->frames = (unsigned)v;
        } else if (strcmp(item, "frame") == 0) {
            bit = VFI_FRAME;
            if (parse_u64(val, &e->frame) != 0 || e->frame < (uint64_t)VD_GRID || e->frame % (uint64_t)VD_GRID != 0u || e->frame > UINT64_C(1000000)) {
                return -1;
            }
        } else if (strcmp(item, "seed") == 0) {
            bit = VFI_SEED;
            if (parse_u64(val, &e->seed) != 0) {
                return -1;
            }
        } else if (strcmp(item, "tie_line") == 0) {
            bit = VFI_TIE;
            if (parse_u64(val, &e->tie_line) != 0 || e->tie_line < 2u || e->tie_line > 200u) {
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
    return e->seen == (unsigned)VFI_ALL ? 0 : -1;
}

/* a booted instance with a direct log over a vid sink of its own: the fold calls of the partitions part are made on it */
struct vf_fold {
    struct booted b;
    struct ds_instance *w;
    const struct ds_instance *ci;
    struct ds_vid_sink vsk;
    struct ds_obs_log log;
    uint8_t *bytes;
    uint32_t cap;
    int live;
};

static int vf_open(struct ctx *c, struct vf_fold *f, const char *rom, uint32_t cap)
{
    const char *err;

    memset(f, 0, sizeof *f);
    f->cap = cap;
    f->bytes = (uint8_t *)malloc((size_t)cap);
    if (f->bytes == NULL) {
        harness_error(c, "out of memory for the sink of the vdc_fold part");
        return -1;
    }
    err = boot_built_rom(rom, "", &f->b, &f->ci);
    if (err != NULL) {
        free(f->bytes);
        f->bytes = NULL;
        harness_error(c, err);
        return -1;
    }
    f->live = 1;
    f->w = (struct ds_instance *)(void *)f->b.mem;
    f->vsk.bytes = f->bytes;
    f->vsk.capacity = cap;
    ds_obs_begin(&f->log, NULL, NULL, &f->vsk);
    return 0;
}

static void vf_close(struct vf_fold *f)
{
    if (f->live) {
        release_boot(&f->b);
    }
    free(f->bytes);
    memset(f, 0, sizeof *f);
}

/* the origin of line k on the grid (ASSUMED LED-VID-0004: raster_phase_mclk 0, line k at k * 1365) */
static uint64_t vf_origin(uint64_t k)
{
    return k * (uint64_t)VD_GRID;
}

/* the stamps of one partition: n cuts in [1, last), sorted, each at most once, then last itself; even partitions draw uniformly, odd ones sit one clock before, on or one clock after the origin of a random line */
static unsigned vf_cuts(uint64_t *st, unsigned n, uint64_t last, int on_grid, uint64_t *rng)
{
    unsigned i;
    unsigned m = 0u;
    uint64_t lines = last / (uint64_t)VD_GRID;

    for (i = 0u; i < n; i++) {
        uint64_t t;

        if (on_grid && lines >= 1u) {
            uint64_t k = 1u + vd_mix(rng) % lines;
            uint64_t d = vd_mix(rng) % 3u;

            t = vf_origin(k) - 1u + d; /* origin - 1, origin, origin + 1 */
        } else {
            t = 1u + vd_mix(rng) % (last - 1u);
        }
        if (t < last) {
            st[m++] = t;
        }
    }
    qsort(st, m, sizeof st[0], vd_cmp_u64);
    st[m++] = last;
    return m;
}

static void vf_partitions(struct ctx *c, const struct vf_expect *e, unsigned *folds)
{
    uint64_t last = (uint64_t)e->frames * e->frame;
    uint32_t cap = vd_stream_cap(last);
    struct vf_fold one;
    uint8_t dig1[DS_DIGEST_BYTES];
    uint64_t rng = e->seed;
    unsigned p;
    uint64_t n_one;
    uint64_t b_one;
    const uint8_t **rec;

    if (cap == 0u) {
        harness_error(c, "the vdc_fold span is too long for one vid sink");
        return;
    }
    if (vf_open(c, &one, e->rom, cap) != 0) {
        return;
    }
    ds_vdc_advance(one.w, last, &one.log);
    ds_obs_end(&one.log);
    n_one = (uint64_t)vd_u32(one.vsk.header);
    b_one = vd_u64(one.vsk.header + 16);
    check(c, vd_u32(one.vsk.header + 4) == 0u && vd_u64(one.vsk.header + 8) == n_one && n_one == vd_lines_due(last + 1u),
          "partitions: one jump to the last stamp S does not hold floor(S / 1365) + 1 records with none lost (the closed form of D-137, ASSUMED LED-VID-0004)");
    rec = (const uint8_t **)calloc((size_t)n_one + 1u, sizeof *rec);
    if (rec == NULL) {
        vf_close(&one);
        harness_error(c, "out of memory for the record list of the vdc_fold part");
        return;
    }
    check(c, vd_walk(one.bytes, b_one, n_one, rec) == 0, "partitions: the records of the one jump do not tile its stored bytes");
    {
        uint64_t k;
        int ok = 1;

        for (k = 0u; k < n_one && ok; k++) {
            ok = vd_u64(rec[k] + VD_O_ORIGIN) == vf_origin(k);
        }
        check(c, ok, "partitions: a record of the one jump does not start at k * 1365 (ASSUMED LED-VID-0004)");
    }
    free((void *)rec);
    check(c, ds_digest(one.ci, dig1) == 0u, "partitions: the digest of the one jump was refused");
    for (p = 0u; p < e->partitions && !c->harness; p++) {
        struct vf_fold f;
        uint64_t st[VD_CUTS_MAX + 1u];
        unsigned m = vf_cuts(st, 1u + (unsigned)(vd_mix(&rng) % (uint64_t)VD_CUTS_MAX), last, (p & 1u) != 0u, &rng);
        unsigned i;
        uint8_t dig[DS_DIGEST_BYTES];
        const char *diff;
        char msg[200];

        if (vf_open(c, &f, e->rom, cap) != 0) {
            break;
        }
        for (i = 0u; i < m; i++) {
            ds_vdc_advance(f.w, st[i], &f.log);
        }
        ds_obs_end(&f.log);
        (*folds)++;
        diff = snap_first_diff(f.ci, one.ci);
        snprintf(msg, sizeof msg, "partitions: partition %u of %u cuts differs from the one jump in the restorable member %s (beam bytes included)", p, m, diff != NULL ? diff : "-");
        check(c, diff == NULL, msg);
        snprintf(msg, sizeof msg, "partitions: the digest of partition %u differs from the one jump", p);
        check(c, ds_digest(f.ci, dig) == 0u && memcmp(dig, dig1, DS_DIGEST_BYTES) == 0, msg);
        snprintf(msg, sizeof msg, "partitions: the records of partition %u (count, lost, offered, byte sum or bytes) differ from the one jump", p);
        check(c, vd_u32(f.vsk.header) == (uint32_t)n_one && vd_u32(f.vsk.header + 4) == 0u && vd_u64(f.vsk.header + 8) == n_one && vd_u64(f.vsk.header + 16) == b_one && vd_u64(f.vsk.header + 24) == 0u
                     && memcmp(f.bytes, one.bytes, (size_t)b_one) == 0,
              msg);
        vf_close(&f);
    }
    vf_close(&one);
}

/* one write of VCE CR = 1 through the bus write path at the access stamp t of line tie_line's origin plus delta, then the fold to the end of line tie_line + 2; the records are judged by the tie rule */
static void vf_tie_one(struct ctx *c, const struct vf_expect *e, int delta)
{
    struct vf_fold f;
    struct ds_bus bus;
    uint64_t o = vf_origin(e->tie_line);
    uint64_t t = delta < 0 ? o - 1u : o + (uint64_t)delta;
    uint64_t end = vf_origin(e->tie_line + 3u) - 1u;
    uint64_t n;
    uint64_t b;
    const uint8_t **rec;
    uint64_t chg; /* the index of the record that must carry the new divider */
    uint64_t r;
    int ok_div = 1;
    int ok_dirty = 1;
    char msg[240];

    if (vf_open(c, &f, e->rom, vd_stream_cap(end)) != 0) {
        return;
    }
    ds_mpr_write(f.w, 0u, 0xFFu); /* MPR0 = $FF: logical $0400 is the VCE control register */
    memset(&bus, 0, sizeof bus);
    bus.start = t; /* cycle_mclk 0: every access of the bus has the stamp start (the stamp of the case), with no penalty cycle added to it */
    bus.obs = &f.log;
    ds_bus_write(f.w, &bus, (uint16_t)VD_VCE_CR_ADDR, 1u);
    ds_vdc_advance(f.w, end, &f.log);
    ds_obs_end(&f.log);
    n = (uint64_t)vd_u32(f.vsk.header);
    b = vd_u64(f.vsk.header + 16);
    rec = (const uint8_t **)calloc((size_t)n + 1u, sizeof *rec);
    if (rec == NULL) {
        vf_close(&f);
        harness_error(c, "out of memory for the record list of the tie part");
        return;
    }
    snprintf(msg, sizeof msg, "tie: a CR write at origin %+d of line %llu: the fold did not give %llu whole records or lost one", delta, (unsigned long long)e->tie_line, (unsigned long long)(e->tie_line + 3u));
    check(c, n == e->tie_line + 3u && vd_u32(f.vsk.header + 4) == 0u && vd_walk(f.bytes, b, n, rec) == 0, msg);
    if (n == e->tie_line + 3u && rec[0] != NULL) {
        chg = delta < 0 ? e->tie_line : e->tie_line + 1u; /* origin - 1: the line itself (the event is after the write); origin or later: the next line (the event precedes the access, D-138, ASSUMED LED-VID-0004) */
        for (r = 0u; r < n; r++) {
            unsigned want = r >= chg ? VD_DIV_CR1 : VD_DIV_CR0; /* MAN H6-3 for the two values; the first line with the new value is the rule above */
            unsigned dirty = (unsigned)rec[r][VD_O_LINE_DIRTY] & 1u;

            ok_div = ok_div && (unsigned)rec[r][VD_O_DIV] == want;
            ok_dirty = ok_dirty && dirty == (r == chg ? 1u : 0u);
        }
        snprintf(msg, sizeof msg, "tie: a CR write at origin %+d of line %llu: the new divider (3, MAN H6-3) is not on exactly the lines from line %llu on (ASSUMED LED-VID-0004, the tie rule of D-138)", delta, (unsigned long long)e->tie_line, (unsigned long long)chg);
        check(c, ok_div, msg);
        snprintf(msg, sizeof msg, "tie: a CR write at origin %+d of line %llu: line_dirty bit 0 is not set on exactly line %llu (ASSUMED LED-VID-0004)", delta, (unsigned long long)e->tie_line, (unsigned long long)chg);
        check(c, ok_dirty, msg);
    }
    free((void *)rec);
    vf_close(&f);
}

struct vf_inv {
    uint64_t calls;
    uint64_t bad_pending;
    uint64_t bad_count;
};

/* the stop callback of the invariant part: it never holds; at every call the next event is not older than the machine time and the next line index is the closed form of the lines due (D-137) */
static int vf_inv_stop(const struct ds_instance *inst, void *ctx)
{
    struct vf_inv *s = (struct vf_inv *)ctx;
    uint64_t t = UINT64_C(0);

    s->calls++;
    if (!ds_vdc_pending_time(&t, inst->line_idx, (uint32_t)inst->stage, (uint32_t)inst->vd_pending, (uint32_t)inst->vd_off, (uint32_t)inst->rcr_off) || t < inst->machine_time) {
        s->bad_pending++;
    }
    if (inst->line_idx != vd_lines_due(inst->machine_time)) {
        s->bad_count++;
    }
    return 0;
}

static void vf_invariant_run(struct ctx *c, const struct vf_expect *e, uint64_t until, int chunked, int with_sink, uint64_t *calls_out)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct vf_inv s;
    struct vd_stream vs;
    struct ds_vid_sink vsk;
    uint64_t rng = e->seed ^ UINT64_C(0x494E564152494E54);
    uint64_t dl[VD_RUN_CHUNKS];
    unsigned n = 1u;
    unsigned i;
    const char *err = boot_built_rom(e->rom, "", &b, &ci);
    char msg[200];

    memset(&s, 0, sizeof s);
    memset(&vs, 0, sizeof vs);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (with_sink && vd_stream_make(&vs, vd_stream_cap(until)) != 0) {
        release_boot(&b);
        harness_error(c, "out of memory for the sink of the invariant part");
        return;
    }
    dl[0] = until;
    if (chunked) {
        for (i = 0u; i + 1u < VD_RUN_CHUNKS; i++) {
            dl[i] = 1u + vd_mix(&rng) % until;
        }
        qsort(dl, VD_RUN_CHUNKS - 1u, sizeof dl[0], vd_cmp_u64);
        dl[VD_RUN_CHUNKS - 1u] = until;
        n = VD_RUN_CHUNKS;
    }
    for (i = 0u; i < n; i++) {
        if (dl[i] <= ci->machine_time && dl[i] < until) {
            continue;
        }
        memset(&rq, 0, sizeof rq);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = dl[i];
        rq.stop = vf_inv_stop;
        rq.stop_ctx = &s;
        if (with_sink) {
            vd_stream_sink(&vs, &vsk);
            rq.vid = &vsk;
        }
        memset(&res, 0, sizeof res);
        if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
            harness_error(c, "ds_run refused a request of the invariant part");
            break;
        }
        if (with_sink) {
            vd_stream_take(&vs, &vsk);
        }
    }
    snprintf(msg, sizeof msg, "invariant (%s, %s): the next event time was older than the machine time or not defined at some stop (D-137)", chunked ? "64 random deadlines" : "one call", with_sink ? "vid sink" : "no sink");
    check(c, s.calls > 100u && s.bad_pending == 0u, msg);
    snprintf(msg, sizeof msg, "invariant (%s, %s): the next line index was not floor((M - 1) / 1365) + 1 of the machine time at some stop (ASSUMED LED-VID-0004)", chunked ? "64 random deadlines" : "one call", with_sink ? "vid sink" : "no sink");
    check(c, s.bad_count == 0u, msg);
    if (with_sink) {
        snprintf(msg, sizeof msg, "invariant (%s): the stream of the run did not hold the closed-form line count with none lost", chunked ? "64 random deadlines" : "one call");
        check(c, !vs.bad && vs.count == vd_lines_due(ci->machine_time) && vs.seq == vs.count, msg);
    }
    *calls_out += s.calls;
    vd_stream_free(&vs);
    release_boot(&b);
}

void kind_vdc_fold(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct vf_expect e;
    uint64_t until = 0u;
    unsigned folds = 0u;
    uint64_t calls = 0u;

    if (!k->has[K_EXPECT] || vf_parse(k->val[K_EXPECT], &e) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u || vd_stream_cap(until) == 0u) {
        harness_error(c, "a vdc_fold case needs until_mclk and expect = rom=NAME;parts=partitions,tie,invariant;partitions=N;frames=N;frame=N;seed=N;tie_line=N (every item once)");
        return;
    }
    if ((e.parts & (unsigned)VF_PART_PARTS) != 0u) {
        vf_partitions(c, &e, &folds);
    }
    if ((e.parts & (unsigned)VF_PART_TIE) != 0u && !c->harness) {
        vf_tie_one(c, &e, -1);
        vf_tie_one(c, &e, 0);
        vf_tie_one(c, &e, 1);
    }
    if ((e.parts & (unsigned)VF_PART_INV) != 0u && !c->harness) {
        vf_invariant_run(c, &e, until, 0, 0, &calls);
        vf_invariant_run(c, &e, until, 1, 0, &calls);
        vf_invariant_run(c, &e, until, 1, 1, &calls);
    }
    snprintf(got, gotcap, "partitions=%u stop_calls=%llu", folds, (unsigned long long)calls);
}

/* ---- kind vid_noninterference ---- */

struct vn_run {
    uint64_t time;
    uint64_t steps;
    uint32_t reason;
    uint8_t verdict[V_SIZE];
    uint8_t digest[DS_DIGEST_BYTES];
    int digest_ok;
    uint8_t *stream; /* the records the sink stored (a copy), NULL when it stored none */
    uint64_t bytes;
    uint32_t count;
    uint32_t lost;
    uint64_t seq;
    int header_ok;   /* the zero word was zero and the bytes past the count were untouched, the guards too */
};

/* the shapes of a vid-only sink: the capacity is in bytes; the large one is the closed form of the deadline at the longest record, the others are set from its run */
enum { VN_OFF = 0, VN_LARGE = 1, VN_DROP4 = 2, VN_CAP0 = 3, VN_CAP1 = 4, VN_SHAPES = 5 };
static const char *const vn_names[VN_SHAPES] = {"off", "large", "drop4", "cap0", "cap1"};

/* one whole run of the fixture under a policy request with the vid sink of a shape (and no insn or dev sink); cap is the byte capacity of the shape; 0 on success */
static int vn_exec(struct ctx *c, const char *rom, const char *request, uint64_t until, int shape, uint32_t cap, struct vn_run *r)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_vid_sink vsk;
    uint8_t *block = NULL;
    size_t blen = (size_t)(cap != 0u ? cap : 1u) + 2u * (size_t)VD_GUARD;
    const char *err = boot_built_rom(rom, request, &b, &ci);

    memset(r, 0, sizeof *r);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    memset(&vsk, shape == VN_OFF ? 0 : (int)VD_POISON, sizeof vsk.header); /* a poisoned header: the core never reads it */
    if (shape != VN_OFF) {
        block = (uint8_t *)malloc(blen);
        if (block == NULL) {
            release_boot(&b);
            harness_error(c, "out of memory for the sink of the vid_noninterference kind");
            return -1;
        }
        memset(block, (int)VD_POISON, blen);
        vsk.bytes = block + VD_GUARD;
        vsk.capacity = cap;
    } else {
        vsk.bytes = NULL;
        vsk.capacity = 0u;
    }
    memset(&rq, 0, sizeof rq);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.vid = shape == VN_OFF ? NULL : &vsk;
    memset(&res, 0, sizeof res);
    r->reason = ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    r->time = res.machine_time;
    r->steps = res.steps;
    memcpy(r->verdict, ci->ram8k + V_OFF, V_SIZE);
    r->digest_ok = ds_digest(ci, r->digest) == 0u;
    if (shape != VN_OFF) {
        uint64_t used = vd_u64(vsk.header + 16);
        size_t j;
        int ok = vd_u64(vsk.header + 24) == 0u && used <= (uint64_t)cap;

        r->count = vd_u32(vsk.header);
        r->lost = vd_u32(vsk.header + 4);
        r->seq = vd_u64(vsk.header + 8);
        r->bytes = used;
        for (j = 0u; j < (size_t)VD_GUARD; j++) { /* the guard bytes either side of the record memory, and the record memory past the stored bytes, still hold the poison */
            ok = ok && block[j] == (uint8_t)VD_POISON && block[(size_t)VD_GUARD + (size_t)(cap != 0u ? cap : 1u) + j] == (uint8_t)VD_POISON;
        }
        for (j = (size_t)used; ok && j < (size_t)(cap != 0u ? cap : 1u); j++) {
            ok = block[(size_t)VD_GUARD + j] == (uint8_t)VD_POISON;
        }
        r->header_ok = ok;
        if (used > 0u && used <= (uint64_t)cap) {
            r->stream = (uint8_t *)malloc((size_t)used);
            if (r->stream == NULL) {
                free(block);
                release_boot(&b);
                harness_error(c, "out of memory for the stream copy of the vid_noninterference kind");
                return -1;
            }
            memcpy(r->stream, block + VD_GUARD, (size_t)used);
        }
    }
    free(block);
    release_boot(&b);
    return 0;
}

/* the whole records of the large stream that fit cap bytes (drop-newest with a sticky full flag, F-2): count and byte sum */
static void vn_fit(const uint8_t *stream, uint64_t count, uint64_t cap, uint32_t *n_out, uint64_t *b_out)
{
    uint64_t used = 0u;
    uint32_t n = 0u;

    while ((uint64_t)n < count) {
        uint64_t rb = (uint64_t)vd_u16(stream + used + VD_O_REC_BYTES);

        if (used + rb > cap) {
            break;
        }
        used += rb;
        n++;
    }
    *n_out = n;
    *b_out = used;
}

void kind_vid_noninterference(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    char rom[48] = "";
    unsigned seen = 0u;
    uint64_t until = 0u;
    unsigned p;
    unsigned runs = 0u;

    if (!k->has[K_EXPECT] || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u || vd_stream_cap(until) == 0u || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a vid_noninterference case needs until_mclk and expect = rom=NAME;compare=verdict,machine_time,steps,digest");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "rom=", 4u) == 0 && (seen & 1u) == 0u && strlen(item + 4) > 0u && strlen(item + 4) < sizeof rom && strspn(item + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 4)) {
            memcpy(rom, item + 4, strlen(item + 4) + 1u);
            seen |= 1u;
        } else if (strcmp(item, "compare=verdict,machine_time,steps,digest") == 0 && (seen & 2u) == 0u) {
            seen |= 2u;
        } else {
            seen = 0u;
            break;
        }
    }
    if (seen != 3u) {
        harness_error(c, "a vid_noninterference case needs until_mclk and expect = rom=NAME;compare=verdict,machine_time,steps,digest");
        return;
    }
    for (p = 0u; p < VD_POLICIES && !c->harness; p++) {
        char request[256];
        struct vn_run run[VN_SHAPES];
        unsigned s;
        uint32_t caps[VN_SHAPES];

        memset(run, 0, sizeof run);
        if (vd_policy_request(p, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", request, sizeof request) != 0) {
            harness_error(c, "the request of the case plus a policy fragment does not fit");
            break;
        }
        caps[VN_OFF] = 0u;
        caps[VN_LARGE] = vd_stream_cap(until);
        caps[VN_DROP4] = 0u;
        caps[VN_CAP0] = 0u;
        caps[VN_CAP1] = 1u;
        for (s = 0u; s < (unsigned)VN_SHAPES && !c->harness; s++) {
            if (s == (unsigned)VN_DROP4) { /* the capacity of the first four records of the large run (F-2): the sum of their rec_bytes */
                uint32_t n4 = run[VN_LARGE].count < 4u ? run[VN_LARGE].count : 4u;
                uint64_t used = 0u;
                uint32_t q;

                for (q = 0u; q < n4 && run[VN_LARGE].stream != NULL; q++) {
                    used += (uint64_t)vd_u16(run[VN_LARGE].stream + used + VD_O_REC_BYTES);
                }
                caps[VN_DROP4] = (uint32_t)used;
            }
            if (vn_exec(c, rom, request, until, (int)s, caps[s], &run[s]) != 0) {
                break;
            }
            runs++;
        }
        if (c->harness) {
            for (s = 0u; s < (unsigned)VN_SHAPES; s++) {
                free(run[s].stream);
            }
            break;
        }
        check(c, run[VN_OFF].reason == (uint32_t)DS_RUN_REASON_UNTIL && run[VN_OFF].digest_ok && run[VN_OFF].steps > 4u, "the run with no sink did not end on its deadline, took four steps or fewer, or its digest was refused");
        if (run[VN_LARGE].lost != 0u) {
            harness_error(c, "the large vid sink lost a record: the stream would be incomplete evidence (D-162)");
        }
        for (s = 1u; s < (unsigned)VN_SHAPES && !c->harness; s++) {
            char msg[240];
            uint32_t want_n = 0u;
            uint64_t want_b = 0u;

            snprintf(msg, sizeof msg, "policy %s, sink %s: the vid sink changed the stop reason, the machine time or the step count of the run (D-164)", vd_policy_name(p), vn_names[s]);
            check(c, run[s].reason == run[VN_OFF].reason && run[s].time == run[VN_OFF].time && run[s].steps == run[VN_OFF].steps, msg);
            snprintf(msg, sizeof msg, "policy %s, sink %s: the vid sink changed the verdict RAM of the run (D-164)", vd_policy_name(p), vn_names[s]);
            check(c, memcmp(run[s].verdict, run[VN_OFF].verdict, V_SIZE) == 0, msg);
            snprintf(msg, sizeof msg, "policy %s, sink %s: the vid sink changed the digest of the instance (D-164)", vd_policy_name(p), vn_names[s]);
            check(c, run[s].digest_ok && memcmp(run[s].digest, run[VN_OFF].digest, DS_DIGEST_BYTES) == 0, msg);
            if (run[VN_LARGE].stream != NULL) {
                vn_fit(run[VN_LARGE].stream, run[VN_LARGE].count, (uint64_t)caps[s], &want_n, &want_b);
            }
            snprintf(msg, sizeof msg, "policy %s, sink %s: the header does not read count = whole records that fit, lost = seq - count, bytes = their sum, seq = the closed-form line count (F-2, D-162)", vd_policy_name(p), vn_names[s]);
            check(c, run[s].seq == vd_lines_due(run[s].time) && run[s].count == want_n && (uint64_t)run[s].lost == run[s].seq - (uint64_t)want_n && run[s].bytes == want_b && run[s].header_ok, msg);
            snprintf(msg, sizeof msg, "policy %s, sink %s: the stored records are not the first records of the large run", vd_policy_name(p), vn_names[s]);
            check(c, run[s].bytes == 0u || (run[s].stream != NULL && run[VN_LARGE].stream != NULL && memcmp(run[s].stream, run[VN_LARGE].stream, (size_t)run[s].bytes) == 0), msg);
        }
        for (s = 0u; s < (unsigned)VN_SHAPES; s++) {
            free(run[s].stream);
        }
    }
    snprintf(got, gotcap, "policies=%u runs=%u", (unsigned)VD_POLICIES, runs);
}

/* ---- kind snapshot_restore_video ---- */

struct sv_sched {
    uint64_t size;
    int every;      /* a restore at every stop (the schedules that fall at every phase of a line) */
};

struct sv_out {
    uint64_t time;
    uint64_t steps;
    uint8_t verdict[V_SIZE];
    uint8_t digest[DS_DIGEST_BYTES];
    int digest_ok;
    struct vd_stream vs;
    int bad;
};

struct sv_stats {
    unsigned restores;
    unsigned at_grid;   /* restores at a stop whose machine time is a multiple of 1365: the next line event is pending at the machine time itself */
    unsigned mid_line;
};

/* run the fixture under the request on a schedule (size 0 = the uninterrupted run: one call to until), restoring as the schedule says; 0 on success, -1 after a harness error */
static int sv_exec(struct ctx *c, const char *rom, const char *request, uint64_t until, const struct sv_sched *sc, unsigned stride, struct sv_out *o, struct sv_stats *st)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_vid_sink vsk;
    uint8_t *snap = NULL;
    uint64_t j = 0u;
    uint64_t d = 0u;
    uint64_t stops = 0u;
    const char *err = boot_built_rom(rom, request, &b, &ci);
    uint32_t cap = vd_stream_cap(until);
    struct booted spare;
    const struct ds_instance *nci = NULL;
    int have_spare = 0;

    memset(&spare, 0, sizeof spare);
    memset(o, 0, sizeof *o);
    if (err != NULL) {
        harness_error(c, err);
        return -1;
    }
    w = (struct ds_instance *)(void *)b.mem;
    if (vd_stream_make(&o->vs, cap) != 0) {
        release_boot(&b);
        harness_error(c, "out of memory for the vid stream of the restore kind");
        return -1;
    }
    if (sc != NULL) {
        snap = (uint8_t *)malloc(ds_snapshot_size());
        if (snap == NULL) {
            vd_stream_free(&o->vs);
            release_boot(&b);
            harness_error(c, "out of memory for the snapshot buffer");
            return -1;
        }
        err = boot_built_rom(rom, request, &spare, &nci); /* the target of the restores: one more boot of the same image under the same request */
        if (err != NULL) {
            free(snap);
            vd_stream_free(&o->vs);
            release_boot(&b);
            harness_error(c, err);
            return -1;
        }
        have_spare = 1;
    }
    while (d < until && !o->bad) {
        j++;
        d = sc == NULL ? until : (sc->size * j < until ? sc->size * j : until);
        if (d <= ci->machine_time && d < until) {
            continue; /* a deadline the previous call overshot takes no step: the next one that is ahead is the next stop */
        }
        memset(&rq, 0, sizeof rq);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = d;
        vd_stream_sink(&o->vs, &vsk);
        rq.vid = &vsk;
        memset(&res, 0, sizeof res);
        if (ds_run(w, &rq, &res) != (uint32_t)DS_RUN_REASON_UNTIL) {
            o->bad = 1;
            break;
        }
        o->steps += res.steps;
        vd_stream_take(&o->vs, &vsk);
        if (o->vs.bad) {
            break;
        }
        if (sc != NULL && d < until) {
            int grid = ci->machine_time % (uint64_t)VD_GRID == 0u;
            int do_restore;

            stops++;
            do_restore = sc->every || grid || (stride != 0u && stops % (uint64_t)stride == 0u);
            if (do_restore) {
                const char *diff;
                struct ds_instance *nw = (struct ds_instance *)(void *)spare.mem;

                if (ds_snapshot(w, snap, ds_snapshot_size(), NULL) != (uint32_t)DS_SNAPREF_NONE) {
                    check(c, 0, "ds_snapshot of the source at a stop of a video schedule was refused");
                    o->bad = 1;
                    break;
                }
                snap_poison(nw, (st->restores & 1u) != 0u ? 0x5Au : 0xA5u); /* the spare is the instance the chain left one restore ago (or the first spare boot): every restorable member of it is poisoned, then the snapshot is restored into it */
                if (ds_restore(nw, snap, ds_snapshot_size()) != (uint32_t)DS_SNAPREF_NONE) {
                    check(c, 0, "ds_restore refused the snapshot that the source just wrote at a stop of a video schedule");
                    o->bad = 1;
                    break;
                }
                st->restores++;
                st->at_grid += grid ? 1u : 0u;
                st->mid_line += grid ? 0u : 1u;
                diff = snap_first_diff(nw, w);
                check(c, diff == NULL, "the restored instance differs from the source in a member (raw compare, ROM excluded)");
                if (diff != NULL) {
                    emit_err("snapshot_restore_video: restore at machine time %llu: member %s differs after the restore\n", (unsigned long long)w->machine_time, diff);
                }
                {
                    struct booted tmp = b; /* the target carries on, the source becomes the spare of the next restore */

                    b = spare;
                    spare = tmp;
                }
                w = (struct ds_instance *)(void *)b.mem;
                ci = w;
            }
        }
    }
    o->time = ci->machine_time;
    memcpy(o->verdict, ci->ram8k + V_OFF, V_SIZE);
    o->digest_ok = ds_digest(ci, o->digest) == 0u;
    release_boot(&b);
    if (have_spare) {
        release_boot(&spare);
    }
    free(snap);
    return 0;
}

void kind_snapshot_restore_video(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    char rom[48] = "";
    unsigned seen = 0u;
    uint64_t until = 0u;
    uint64_t stride = 0u;
    struct sv_sched sched[4];
    unsigned nsched = 0u;
    unsigned p;
    unsigned total = 0u;
    unsigned grid = 0u;
    unsigned mid = 0u;

    if (!k->has[K_EXPECT] || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u || vd_stream_cap(until) == 0u || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a snapshot_restore_video case needs until_mclk and expect = rom=NAME;schedules=1364,1366,7;stride=N");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "rom=", 4u) == 0 && (seen & 1u) == 0u && strlen(item + 4) > 0u && strlen(item + 4) < sizeof rom && strspn(item + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 4)) {
            memcpy(rom, item + 4, strlen(item + 4) + 1u);
            seen |= 1u;
        } else if (strncmp(item, "schedules=", 10u) == 0 && (seen & 2u) == 0u) {
            char *r2 = item + 10;
            char *w;

            while ((w = pk_next_item(&r2, ',')) != NULL) {
                uint64_t v = 0u;

                if (nsched >= 4u || parse_u64(w, &v) != 0 || v == 0u) {
                    nsched = 0u;
                    break;
                }
                sched[nsched].size = v;
                sched[nsched].every = v >= (uint64_t)(VD_GRID - 1u); /* the line-sized schedules restore at every stop; a short one (7) is strided */
                nsched++;
            }
            seen |= nsched > 0u ? 2u : 0u;
        } else if (strncmp(item, "stride=", 7u) == 0 && (seen & 4u) == 0u && parse_u64(item + 7, &stride) == 0 && stride >= 1u && stride <= 100000u) {
            seen |= 4u;
        } else {
            seen = 0u;
            break;
        }
    }
    if (seen != 7u) {
        harness_error(c, "a snapshot_restore_video case needs until_mclk and expect = rom=NAME;schedules=1364,1366,7;stride=N");
        return;
    }
    for (p = 0u; p < VD_POLICIES && !c->harness; p++) {
        char request[256];
        struct sv_out ref;
        struct sv_stats st;
        unsigned s;

        memset(&st, 0, sizeof st);
        if (vd_policy_request(p, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", request, sizeof request) != 0) {
            harness_error(c, "the request of the case plus a policy fragment does not fit");
            break;
        }
        if (sv_exec(c, rom, request, until, NULL, 0u, &ref, &st) != 0) {
            break;
        }
        if (p == 0u) {
            note_run(c, ref.time, (uint32_t)DS_RUN_REASON_UNTIL);
        }
        if (ref.vs.bad || ref.vs.count == 0u) {
            vd_stream_free(&ref.vs);
            harness_error(c, "the uninterrupted run lost a vid record, or its header disagreed with the records it stored, or it stored none: the comparison would be vacuous");
            break;
        }
        check(c, ref.digest_ok && ref.steps > 4u && ref.vs.count == vd_lines_due(ref.time), "the uninterrupted run took four steps or fewer, its digest was refused, or its record count is not the closed-form line count (ASSUMED LED-VID-0004)");
        for (s = 0u; s < nsched && !c->harness; s++) {
            struct sv_out o;
            char msg[240];
            unsigned before = st.restores;
            unsigned grid_before = st.at_grid;

            if (sv_exec(c, rom, request, until, &sched[s], (unsigned)stride, &o, &st) != 0) {
                break;
            }
            if (o.vs.bad) {
                vd_stream_free(&o.vs);
                harness_error(c, "a call of a restored run lost a vid record or its header disagreed with the records it stored");
                break;
            }
            snprintf(msg, sizeof msg, "policy %s, schedule %llu: a call was refused or the run did not end on its deadline", vd_policy_name(p), (unsigned long long)sched[s].size);
            check(c, !o.bad, msg);
            snprintf(msg, sizeof msg, "policy %s, schedule %llu: the machine time or the step count of the restored chain differs from the uninterrupted run", vd_policy_name(p), (unsigned long long)sched[s].size);
            check(c, o.time == ref.time && o.steps == ref.steps, msg);
            snprintf(msg, sizeof msg, "policy %s, schedule %llu: the verdict RAM of the restored chain differs from the uninterrupted run", vd_policy_name(p), (unsigned long long)sched[s].size);
            check(c, memcmp(o.verdict, ref.verdict, V_SIZE) == 0, msg);
            snprintf(msg, sizeof msg, "policy %s, schedule %llu: the digest of the restored chain differs from the uninterrupted run", vd_policy_name(p), (unsigned long long)sched[s].size);
            check(c, o.digest_ok && ref.digest_ok && memcmp(o.digest, ref.digest, DS_DIGEST_BYTES) == 0, msg);
            snprintf(msg, sizeof msg, "policy %s, schedule %llu: the concatenated video stream of the restored chain differs from the uninterrupted run (count, offered, bytes or byte content)", vd_policy_name(p), (unsigned long long)sched[s].size);
            check(c, o.vs.count == ref.vs.count && o.vs.seq == ref.vs.seq && o.vs.bytes == ref.vs.bytes && memcmp(o.vs.acc, ref.vs.acc, (size_t)ref.vs.bytes) == 0, msg);
            snprintf(msg, sizeof msg, "policy %s, schedule %llu: the schedule restored at no stop", vd_policy_name(p), (unsigned long long)sched[s].size);
            check(c, st.restores > before, msg);
            if (sched[s].size < (uint64_t)VD_GRID - 1u) {
                snprintf(msg, sizeof msg, "policy %s, schedule %llu: no restore fell at a stop whose machine time is a multiple of 1365 (a line event pending at the machine time): the tie would be untested", vd_policy_name(p), (unsigned long long)sched[s].size);
                check(c, st.at_grid > grid_before, msg);
            }
            vd_stream_free(&o.vs);
        }
        total += st.restores;
        grid += st.at_grid;
        mid += st.mid_line;
        vd_stream_free(&ref.vs);
    }
    snprintf(got, gotcap, "restores=%u at_grid=%u mid_line=%u", total, grid, mid);
}

/* ---- vdc_vram (plan 04-06; D-148, D-175 (c), T-04-23) ---- */

#define VV_WORDS_MAX 128u
#define VV_ITEM_ROM 1u
#define VV_ITEM_WORDS 2u

struct vv_expect {
    char rom[48];
    unsigned n;
    uint32_t addr[VV_WORDS_MAX];
    uint32_t value[VV_WORDS_MAX];
};

/* words=A:V,A:V,...: word addresses below $8000 (the stored words) and 16-bit values, decimal or 0x hex; at least one, at most VV_WORDS_MAX, no address twice */
static int vv_parse_words(char *list, struct vv_expect *e)
{
    char *rest = list;
    char *w;

    e->n = 0u;
    while ((w = pk_next_item(&rest, ',')) != NULL) {
        char *colon = strchr(w, ':');
        uint64_t a = 0u;
        uint64_t v = 0u;
        unsigned j;

        if (colon == NULL || e->n >= VV_WORDS_MAX) {
            return -1;
        }
        *colon = '\0';
        if (parse_u64(w, &a) != 0 || parse_u64(colon + 1, &v) != 0 || a >= (uint64_t)DS_CONST_vdc_vram_words || v > 0xFFFFu) {
            return -1;
        }
        for (j = 0u; j < e->n; j++) {
            if (e->addr[j] == (uint32_t)a) {
                return -1;
            }
        }
        e->addr[e->n] = (uint32_t)a;
        e->value[e->n] = (uint32_t)v;
        e->n++;
    }
    return e->n > 0u ? 0 : -1;
}

static int vv_parse(const char *text, struct vv_expect *e)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned seen = 0u;

    memset(e, 0, sizeof *e);
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char *eq = strchr(item, '=');
        unsigned bit;

        if (eq == NULL) {
            return -1;
        }
        *eq = '\0';
        if (strcmp(item, "rom") == 0) {
            bit = VV_ITEM_ROM;
            if (strlen(eq + 1) == 0u || strlen(eq + 1) >= sizeof e->rom || strspn(eq + 1, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(eq + 1)) {
                return -1;
            }
            memcpy(e->rom, eq + 1, strlen(eq + 1) + 1u);
        } else if (strcmp(item, "words") == 0) {
            bit = VV_ITEM_WORDS;
            if (vv_parse_words(eq + 1, e) != 0) {
                return -1;
            }
        } else {
            return -1;
        }
        if ((seen & bit) != 0u) {
            return -1;
        }
        seen |= bit;
    }
    return seen == (VV_ITEM_ROM | VV_ITEM_WORDS) ? 0 : -1;
}

/*
 * vdc_vram: the VRAM words the guest wrote, read from the instance bytes (D-175 (c)). The fixture runs to the end of its DSV1 verdict under each of the three default policies; every listed word, assembled here byte by byte from the 65,536 VRAM bytes (word w = byte 2w | byte 2w+1 << 8, no accessor and no
 * port read path of the core is used), must equal the literal of the case, which is restated from the fixture source (a value the guest writes, class MAN H7-5 or the class of its row; never read from a DuoSpool run). A symmetric write/read bug of the port cannot cancel here: a word stored at the wrong address or
 * with the bytes swapped reads wrong from the bytes, whatever the guest reads back. Evidence tier E0; no accuracy or hardware claim.
 */
void kind_vdc_vram(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct vv_expect e;
    uint64_t until = 0u;
    unsigned p;
    unsigned words = 0u;

    if (!k->has[K_EXPECT] || vv_parse(k->val[K_EXPECT], &e) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u) {
        harness_error(c, "a vdc_vram case needs until_mclk and expect = rom=NAME;words=ADDR:VALUE,ADDR:VALUE,... (word addresses below $8000, every address once, at most 128 words)");
        return;
    }
    for (p = 0u; p < (unsigned)VD_POLICIES && !c->harness; p++) {
        char request[256];
        struct booted b;
        const struct ds_instance *ci = NULL;
        struct ds_run_request rq;
        struct ds_run_result res;
        const char *err;
        unsigned j;

        if (vd_policy_request(p, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", request, sizeof request) != 0) {
            harness_error(c, "the request of a vdc_vram case does not fit with a policy fragment");
            return;
        }
        err = boot_built_rom(e.rom, request, &b, &ci);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.stop = dsv1_done;
        if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
            release_boot(&b);
            harness_error(c, "ds_run refused the request of a vdc_vram case");
            return;
        }
        note_run(c, res.machine_time, res.reason);
        if (!dsv1_done(ci, NULL)) {
            release_boot(&b);
            harness_error(c, "the guest of a vdc_vram case did not reach the end of its DSV1 verdict before until_mclk: its words are not final");
            return;
        }
        for (j = 0u; j < e.n; j++) {
            uint32_t w = e.addr[j];
            uint32_t have = (uint32_t)ci->vdc_vram[2u * w] | ((uint32_t)ci->vdc_vram[2u * w + 1u] << 8u);
            char msg[200];

            snprintf(msg, sizeof msg, "policy %s: VRAM word $%04X holds $%04X, the case lists $%04X", vd_policy_name(p), (unsigned)w, (unsigned)have, (unsigned)e.value[j]);
            check(c, have == e.value[j], msg);
            words++;
        }
        release_boot(&b);
    }
    snprintf(got, gotcap, "words=%u policies=%u", words, (unsigned)VD_POLICIES);
}
