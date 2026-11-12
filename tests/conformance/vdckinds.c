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
 *   vid_noninterference       expect = rom=NAME;compare=verdict,machine_time,steps,digest (until_mclk needed): for each of the two power-on policies of VD_POLICIES_VIDEO (zero and seeded-prng-v3 seed 1; lever (2) of D-176, plan 04.1-01) the fixture runs with no vid sink and with a vid sink only in four shapes (large, 4 records drop-newest taken from the sizes of the large
 *                             run, capacity 0 and capacity 1): the machine time, the steps, the verdict RAM and the digest equal those of the run with no sink, the header counts whole records stored, lost = seq - count, bytes is their sum, seq is the closed-form line count, and the bytes past the count are untouched (D-164, F-2).
 *   snapshot_restore_video    expect = rom=NAME;schedules=1364,1366,7;stride=N (until_mclk needed): the fixture is run on each schedule (calls to the deadlines j * size) with a vid sink on every call; at every stop of the schedules 1364 and 1366, which fall at every phase inside a line, and at every stride-th stop and
 *                             at every stop whose machine time is a multiple of 1365 (a pending line event at the machine time itself) of the schedule 7, the machine is snapshotted, restored into a fresh poisoned boot of the same image (member by member equal to the source) and the run goes on with the target. The concatenated
 *                             stream bytes, the digest, the machine time, the steps and the verdict RAM of every schedule equal those of the uninterrupted run (D-137). A case that restored at no stop at a multiple of 1365 is a guest_failure: the tie would be untested.
 *   vdc_random_regs           expect = programs=N;seed=N (until_mclk needed; plan 04-13, D-175, T-04-44): N in-process programs (rp_build_vdc in randprog.c) that write random values to every VDC register through AR and the data port,
 *                             to the VCE window, to the DMA triggers and through block transfers, with random waits, each run to the deadline with a vid sink of the closed-form capacity: ds_run stops at the deadline, no record is lost, the count equals
 *                             floor((M - 1) / 1365) + 1, every record is whole and every border, index and colour value is below 512. Meant for the sanitizer preset as much as the gate preset; E0 (no crash, no out-of-range value, termination), nothing is
 *                             compared with any oracle.
 *   vdc_poweron_policy_diff   expect = roms=NAME@FRAME,...;control=NAME@FRAME (until_mclk needed; plan 04-13, D-167, Pitfall 13, T-04-45): each named fixture runs under the three policies with a vid sink; the verdict RAM and every
 *                             record from the first judged frame FRAME on must be byte for byte equal (the frames before it are setup frames and may differ); the control image (vce-pal-nopal, whose palette is never initialised) must show a different border in
 *                             the window between the zero policy and each v3 seed, which shows the guard can fail. E0.
 * The policy list of this file (zero, seeded-prng-v3 seed 1, seeded-prng-v3 seed 2) is the default list of a guest case (D-167); vd_policy_request appends a fragment to the request of a case as kind_guest does.
 */
#include <stddef.h>
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
    for (p = 0u; p < VD_POLICIES_VIDEO && !c->harness; p++) {
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
    snprintf(got, gotcap, "policies=%u runs=%u", (unsigned)VD_POLICIES_VIDEO, runs);
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
    for (p = 0u; p < VD_POLICIES_VIDEO && !c->harness; p++) {
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
#define VV_ITEM_TIA 4u

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

/* tia=ADDR:W0:STEP:COUNT: COUNT more words at ADDR, ADDR + 1, ...; word i is (W0 + STEP * i) & $FFFF, the multiplication the assembler of the fixture forms its upload table with (a different arithmetic from the additive running sum of the guest); the addresses must be new */
static int vv_parse_tia(char *list, struct vv_expect *e)
{
    uint64_t f[4];
    unsigned i;
    unsigned j;
    char *rest = list;
    char *part;

    for (i = 0u; i < 4u; i++) {
        part = pk_next_item(&rest, ':');
        if (part == NULL || parse_u64(part, &f[i]) != 0) {
            return -1;
        }
    }
    if (rest != NULL || f[0] + f[3] > (uint64_t)DS_CONST_vdc_vram_words || f[1] > 0xFFFFu || f[2] > 0xFFFFu || f[3] < 1u || f[3] + (uint64_t)e->n > (uint64_t)VV_WORDS_MAX) {
        return -1;
    }
    for (i = 0u; i < (unsigned)f[3]; i++) {
        uint32_t a = (uint32_t)f[0] + i;

        for (j = 0u; j < e->n; j++) {
            if (e->addr[j] == a) {
                return -1;
            }
        }
        e->addr[e->n] = a;
        e->value[e->n] = (uint32_t)(((uint32_t)f[1] + (uint32_t)f[2] * i) & 0xFFFFu);
        e->n++;
    }
    return 0;
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
        } else if (strcmp(item, "tia") == 0) {
            bit = VV_ITEM_TIA;
            if ((seen & VV_ITEM_WORDS) == 0u || vv_parse_tia(eq + 1, e) != 0) { /* the explicit words come first, the pattern adds to them */
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
    return (seen & (VV_ITEM_ROM | VV_ITEM_WORDS)) == (VV_ITEM_ROM | VV_ITEM_WORDS) ? 0 : -1;
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
        harness_error(c, "a vdc_vram case needs until_mclk and expect = rom=NAME;words=ADDR:VALUE,ADDR:VALUE,...[;tia=ADDR:W0:STEP:COUNT] (word addresses below $8000, every address once, at most 128 words)");
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

/* ---- vdc_vram_sweep and vdc_regmask (plan 04-06; D-146, D-147, D-148, D-161, D-175, T-04-20, T-04-21) ---- */

#define VS_WORDS 65536u         /* every 16-bit word address (the address registers are 16 bits wide) */
#define VS_STORED 32768u        /* the words that exist (ASSUMED LED-VID-0007, written here; the core's constant is DS_CONST_vdc_vram_words) */
#define VS_PART_ACCESSOR 1u
#define VS_PART_PORT 2u
#define VS_PART_MASKS 1u
#define VS_PART_AR 2u
#define VS_PART_STATUS 4u

/* the word the sweeps store at address w: a function of w written here, so a word stored at the wrong address or with swapped bytes cannot match */
static uint32_t vs_pattern(uint32_t w, uint32_t salt)
{
    return ((w * 0x9E37u) ^ (w >> 3u) ^ salt) & 0xFFFFu;
}

/* the parts list of a case, expect = parts=NAME,NAME,...: a comma list of the names in names[], each once, as bits; 0 on a bad list */
static unsigned vs_parts(const char *text, const char *const *names, unsigned n)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *w;
    unsigned out = 0u;

    if (strncmp(text, "parts=", 6u) != 0 || pk_copy_text(text + 6, list, sizeof list) != 0) {
        return 0u;
    }
    while ((w = pk_next_item(&rest, ',')) != NULL) {
        unsigned q;

        for (q = 0u; q < n && strcmp(w, names[q]) != 0; q++) {
        }
        if (q == n || (out & (1u << q)) != 0u) {
            return 0u;
        }
        out |= 1u << q;
    }
    return out;
}

/* a booted synthetic instance of the case's fixture (plain-256k) with no content judged: the VDC is the subject, the cartridge is not touched */
static struct ds_instance *vs_boot(struct ctx *c, const struct kase *k, struct booted *b)
{
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted(k->val[K_FIXTURE], k->has[K_REQUEST] ? k->val[K_REQUEST] : "", b, &s, &inst);

    if (err != NULL) {
        harness_error(c, err);
        return NULL;
    }
    return (struct ds_instance *)(void *)b->mem;
}

/* one port write of a data word: AR = reg, then the low byte, then the high byte */
static void vs_port_word(struct ds_instance *w, uint32_t reg, uint32_t value)
{
    ds_vdc_write(w, 0u, (uint8_t)reg, 0u);
    ds_vdc_write(w, 2u, (uint8_t)(value & 0xFFu), 0u);
    ds_vdc_write(w, 3u, (uint8_t)(value >> 8u), 0u);
}

/* the 65,536 bytes of VRAM against the words the sweep stored (a pattern below $8000, nothing above: the array ends there); 0 when equal */
static int vs_bytes_equal(const struct ds_instance *w, uint32_t salt)
{
    uint32_t i;

    for (i = 0u; i < VS_STORED; i++) {
        uint32_t v = vs_pattern(i, salt);

        if (w->vdc_vram[2u * i] != (uint8_t)(v & 0xFFu) || w->vdc_vram[2u * i + 1u] != (uint8_t)(v >> 8u)) {
            return -1;
        }
    }
    return 0;
}

/*
 * vdc_vram_sweep (D-147, T-04-20; E0): expect = parts=accessor,port (each once). Every one of the 65,536 word addresses goes through the one accessor pair (accessor part) and through the port path (port part: MAWR and MARR written, VWR written, VRR read) on a booted instance, under the sanitizers of the sanitize preset (the gate runs the case there too).
 * accessor  every word below $8000 is written and read back (a pattern of the address, written in this file); every word address from $8000 to $FFFF and three larger ones ($10000, $7FFFFFFF, $FFFFFFFF) reads 0, and a write there leaves every byte of the instance unchanged (a copy taken before, compared outside the VRAM bytes,
 *           so a store past the array is seen); the bytes then equal the pattern word by word.
 * port      for each word address w: MAWR = w, VWR gets the word of w; the instance MAWR is w + 1 mod 65,536 (the step is applied also when the store is dropped); then for each w: MARR = w, the buffer is loaded, the high byte read of VRR returns the word of w (low byte read first) and steps MARR to w + 1 mod 65,536;
 *           the words below $8000 equal the pattern and above read 0, and the bytes equal the pattern in the end.
 * The literals are the rule of D-147 (ASSUMED LED-VID-0007: size, out-of-range read 0, writes dropped) and the steps of MAN H7-5 and H7-7 (IW 00 = +1, CR is 0); none comes from a run. No accuracy or hardware claim.
 */
void kind_vdc_vram_sweep(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const names[2] = {"accessor", "port"};
    struct booted b;
    struct ds_instance *w;
    unsigned parts = k->has[K_EXPECT] ? vs_parts(k->val[K_EXPECT], names, 2u) : 0u;
    uint32_t i;
    uint32_t bad = 0u;
    uint32_t calls = 0u;
    unsigned char *before;
    int outside_bad;

    if (parts == 0u) {
        harness_error(c, "a vdc_vram_sweep case needs expect = parts=accessor,port (a list of the parts, each once) and a booting synthetic fixture");
        return;
    }
    w = vs_boot(c, k, &b);
    if (w == NULL) {
        return;
    }
    if ((parts & VS_PART_ACCESSOR) != 0u) {
        static const uint32_t big[3] = {0x10000u, 0x7FFFFFFFu, 0xFFFFFFFFu};

        for (i = 0u; i < VS_STORED; i++) {
            ds_vdc_vram_write(w, i, (uint16_t)vs_pattern(i, 0x1111u));
            calls++;
        }
        for (i = 0u; i < VS_WORDS; i++) {
            uint32_t want = i < VS_STORED ? vs_pattern(i, 0x1111u) : 0u;

            bad += (uint32_t)ds_vdc_vram_read(w, i) != want;
            calls++;
        }
        before = (unsigned char *)malloc(sizeof *w);
        if (before == NULL) {
            release_boot(&b);
            harness_error(c, "out of memory for the instance copy of the vdc_vram_sweep kind");
            return;
        }
        memcpy(before, w, sizeof *w);
        for (i = VS_STORED; i < VS_WORDS; i++) {
            ds_vdc_vram_write(w, i, 0xFFFFu);
            calls++;
        }
        for (i = 0u; i < 3u; i++) {
            ds_vdc_vram_write(w, big[i], 0xFFFFu);
            bad += (uint32_t)ds_vdc_vram_read(w, big[i]) != 0u;
            calls += 2u;
        }
        {
            const unsigned char *now = (const unsigned char *)(const void *)w;
            size_t v0 = offsetof(struct ds_instance, vdc_vram);
            size_t v1 = v0 + (size_t)DS_VRAM_BYTES;

            outside_bad = memcmp(before, now, v0) != 0 || memcmp(before + v1, now + v1, sizeof *w - v1) != 0;
        }
        free(before);
        check(c, !outside_bad, "accessor: a write at a word address at or above $8000 changed a byte of the instance outside the 65,536 VRAM bytes (the satb, the beam, the registers)");
        check(c, bad == 0u, "accessor: a read of a word address returned another word than the pattern written (below $8000) or a nonzero word (at and above $8000, ASSUMED LED-VID-0007)");
        check(c, vs_bytes_equal(w, 0x1111u) == 0, "accessor: a write at a word address at or above $8000 changed a VRAM byte, or a stored word moved");
    }
    if ((parts & VS_PART_PORT) != 0u) {
        uint32_t step_bad = 0u;
        uint32_t read_bad = 0u;

        bad = 0u;
        ds_vdc_write(w, 0u, 5u, 0u); /* CR = 0: IW +1 */
        ds_vdc_write(w, 2u, 0u, 0u);
        ds_vdc_write(w, 3u, 0u, 0u);
        for (i = 0u; i < VS_WORDS; i++) {
            vs_port_word(w, 0u, i); /* MAWR = i */
            ds_vdc_write(w, 0u, 2u, 0u);
            ds_vdc_write(w, 2u, (uint8_t)(vs_pattern(i, 0x2222u) & 0xFFu), 0u);
            ds_vdc_write(w, 3u, (uint8_t)(vs_pattern(i, 0x2222u) >> 8u), 0u);
            step_bad += (uint32_t)w->vdc_reg[0] != ((i + 1u) & 0xFFFFu);
            calls += 7u;
        }
        for (i = 0u; i < VS_WORDS; i++) {
            uint32_t want = i < VS_STORED ? vs_pattern(i, 0x2222u) : 0u;
            uint32_t lo;
            uint32_t hi;

            vs_port_word(w, 1u, i); /* MARR = i: the high byte write loads the buffer */
            ds_vdc_write(w, 0u, 2u, 0u);
            lo = (uint32_t)ds_vdc_read(w, 2u, 0u);
            hi = (uint32_t)ds_vdc_read(w, 3u, 0u);
            read_bad += (lo | (hi << 8u)) != want;
            step_bad += (uint32_t)w->vdc_reg[1] != ((i + 2u) & 0xFFFFu); /* the write started one read (MARR = i + 1) and the high byte read of VRR another (i + 2) */
            calls += 9u;
        }
        check(c, step_bad == 0u, "port: MAWR or MARR is not the address plus the step (1) modulo 65,536 after an operation, also for the dropped words (MAN H7-5, H7-7)");
        check(c, read_bad == 0u, "port: a word read through VRR is not the word written through VWR (below $8000) or not 0 (at and above $8000, ASSUMED LED-VID-0007)");
        check(c, vs_bytes_equal(w, 0x2222u) == 0, "port: the VRAM bytes do not equal the words written through VWR (word address, byte order or a dropped write that was stored)");
    }
    release_boot(&b);
    snprintf(got, gotcap, "addresses=%u accesses=%u", (unsigned)VS_WORDS, (unsigned)calls);
}

/* the field widths of the registers R00 to R13 (MAN H7-2, H7-5 to H7-11; H7-2 prints VDW in the R0E row, the text and H7-11 say VCR), written here: the masks of D-146; R03 and R04 are reserved */
static const uint16_t vm_mask[20] = {
    0xFFFFu, 0xFFFFu, 0xFFFFu, 0x0000u, 0x0000u, 0x1FFFu, 0x03FFu, 0x03FFu, 0x01FFu, 0x00FFu, 0x7F1Fu, 0x7F7Fu, 0xFF1Fu, 0x01FFu, 0x00FFu, 0x001Fu, 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu,
};

/* the line_dirty bit a write of each register sets (D-161: b1 HSR and HDR, b2 VPR VDW VCR, b3 BXR and BYR, b5 CR and MWR), written here */
static uint32_t vm_dirty(uint32_t reg)
{
    switch (reg) {
    case 0x0Au:
    case 0x0Bu:
        return 0x02u;
    case 0x0Cu:
    case 0x0Du:
    case 0x0Eu:
        return 0x04u;
    case 0x07u:
    case 0x08u:
        return 0x08u;
    case 0x05u:
    case 0x09u:
        return 0x20u;
    default:
        return 0u;
    }
}

/* the status through the port, and a flag raised by the single setter */
static uint32_t vm_status(struct ds_instance *w)
{
    return (uint32_t)ds_vdc_read(w, 0u, 0u);
}

/*
 * vdc_regmask (D-146, D-144, D-161, T-04-21; E0): expect = parts=masks,ar,status (each once) on a booted synthetic instance.
 * masks   for each of R00 to R13: a write of $FF to the low byte reads back (instance bytes) the low byte of its mask, then a write of $FF to the high byte the whole mask (the field widths of MAN H7-2 and H7-5 to H7-11, written here); R03 and R04 stay 0; the line_dirty bit of the write is the one of D-161. MARR is judged after its read operation: written $FFFF it
 *         is stepped to $0000 by the IW step of MAN H7-5 (the mask of MARR is $FFFF); a register with side effects keeps its value, nothing else changes.
 * ar      AR keeps its low five bits ($E2 acts as $02, $24 as $04); at each AR from $14 to $1F a data write changes no register, the buffer, the latch, the line_dirty bits and no VRAM byte (a reserved register ignores a write, ASSUMED LED-VID-0006).
 * status  a flag raised with its enable clear leaves the status 0; with the enable set (CR bits 0 to 3 for CR, OR, RR and VD, DCR bits 0 and 1 for DS and DV, MAN H7-4) the read returns the bit pattern and the next read 0 (a read clears bits 0 to 5); bit 6 (BSY) is the constant level 0 (ASSUMED LED-VID-0015) and bit 7 is 0;
 *         disabling later does not clear a raised flag; a flag number above 5 changes nothing.
 * The literals are the manual field widths, the bit layout of H7-4 and the rules of D-144 and D-146; none comes from a run. A mask is a field-width fact, not a read-back claim. No accuracy or hardware claim.
 */
void kind_vdc_regmask(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const names[3] = {"masks", "ar", "status"};
    struct booted b;
    struct ds_instance *w;
    unsigned parts = k->has[K_EXPECT] ? vs_parts(k->val[K_EXPECT], names, 3u) : 0u;
    unsigned checks = 0u;
    uint32_t n;
    char msg[200];

    if (parts == 0u) {
        harness_error(c, "a vdc_regmask case needs expect = parts=masks,ar,status (a list of the parts, each once) and a booting synthetic fixture");
        return;
    }
    w = vs_boot(c, k, &b);
    if (w == NULL) {
        return;
    }
    if ((parts & VS_PART_MASKS) != 0u) {
        for (n = 0u; n < 20u; n++) {
            uint32_t m = (uint32_t)vm_mask[n];
            uint32_t want_low = 0x00FFu & m;
            uint32_t want_full = n == 1u ? 0x0000u : m; /* MARR: the MSB write starts a read operation that steps it (+1: $FFFF becomes $0000) */

            w->line_dirty = 0u;
            ds_vdc_write(w, 0u, (uint8_t)n, 0u);
            ds_vdc_write(w, 2u, 0xFFu, 0u);
            snprintf(msg, sizeof msg, "R%02X: a low byte write of $FF reads back $%04X, not $%04X (the low byte of the field mask of D-146)", (unsigned)n, (unsigned)w->vdc_reg[n], (unsigned)want_low);
            check(c, (uint32_t)w->vdc_reg[n] == want_low, msg);
            snprintf(msg, sizeof msg, "R%02X: line_dirty is $%02X after a write, not $%02X (D-161)", (unsigned)n, (unsigned)w->line_dirty, (unsigned)vm_dirty(n));
            check(c, (uint32_t)w->line_dirty == vm_dirty(n), msg);
            ds_vdc_write(w, 3u, 0xFFu, 0u);
            snprintf(msg, sizeof msg, "R%02X: a write of $FFFF reads back $%04X, not $%04X (the field mask of D-146, MAN H7-2 and H7-5 to H7-11)", (unsigned)n, (unsigned)w->vdc_reg[n], (unsigned)want_full);
            check(c, (uint32_t)w->vdc_reg[n] == want_full, msg);
            checks += 3u;
        }
        check(c, w->vdc_reg[3] == 0u && w->vdc_reg[4] == 0u, "R03 and R04 are not 0 after writes of $FFFF");
        ds_vdc_write(w, 0u, 5u, 0u); /* CR = 0 again so that no later part sees an IW step or an enable */
        ds_vdc_write(w, 2u, 0u, 0u);
        ds_vdc_write(w, 3u, 0u, 0u);
    }
    if ((parts & VS_PART_AR) != 0u) {
        uint32_t ar;
        uint16_t regs[20];
        uint32_t sum_before = 0u;
        uint32_t sum_after = 0u;
        uint32_t i;

        ds_vdc_write(w, 0u, 0xE2u, 0u);
        check(c, w->vdc_ar == 0x02u, "AR written $E2 does not keep its low five bits ($02, MAN H7-3: AR is bits 4-0)");
        ds_vdc_write(w, 0u, 0x24u, 0u);
        check(c, w->vdc_ar == 0x04u, "AR written $24 does not read $04 (five bits)");
        for (ar = 0x14u; ar <= 0x1Fu; ar++) {
            memcpy(regs, w->vdc_reg, sizeof regs);
            sum_before = 0u;
            sum_after = 0u;
            for (i = 0u; i < (uint32_t)DS_VRAM_BYTES; i++) {
                sum_before += (uint32_t)w->vdc_vram[i] * (i + 1u);
            }
            w->line_dirty = 0u;
            ds_vdc_write(w, 0u, (uint8_t)ar, 0u);
            ds_vdc_write(w, 2u, 0xFFu, 0u);
            ds_vdc_write(w, 3u, 0xFFu, 0u);
            for (i = 0u; i < (uint32_t)DS_VRAM_BYTES; i++) {
                sum_after += (uint32_t)w->vdc_vram[i] * (i + 1u);
            }
            snprintf(msg, sizeof msg, "AR $%02X: a data write changed a register, the line_dirty bits or the VRAM bytes (a reserved register ignores a write, ASSUMED LED-VID-0006)", (unsigned)ar);
            check(c, memcmp(regs, w->vdc_reg, sizeof regs) == 0 && w->line_dirty == 0u && sum_before == sum_after && w->vdc_ar == (uint8_t)ar, msg);
            checks++;
        }
        ds_vdc_write(w, 0u, 0u, 0u);
    }
    if ((parts & VS_PART_STATUS) != 0u) {
        uint32_t bit;

        w->vdc_status = 0u;
        vs_port_word(w, 5u, 0x0000u); /* CR and DCR clear: the enables of the first step (the masks part leaves DCR at its mask) */
        vs_port_word(w, 0x0Fu, 0x0000u);
        for (bit = 0u; bit < 8u; bit++) { /* the enables are clear (CR and DCR are 0): no flag is raised */
            ds_vdc_flag(w, bit);
        }
        ds_vdc_flag(w, 31u);
        check(c, vm_status(w) == 0u, "status: a flag raised with its enable clear left a bit in the status (MAN H7-4: a bit is set when an interrupt enabled by CR or DCR occurs)");
        vs_port_word(w, 5u, 0x000Fu); /* CR: IE = all four */
        vs_port_word(w, 0x0Fu, 0x0003u); /* DCR: DSC and DVC */
        for (bit = 0u; bit < 6u; bit++) {
            ds_vdc_flag(w, bit);
        }
        ds_vdc_flag(w, 6u);
        ds_vdc_flag(w, 7u);
        ds_vdc_flag(w, 31u);
        n = vm_status(w);
        snprintf(msg, sizeof msg, "status: the six flags raised with their enables set read $%02X, not $3F (CR 0, OR 1, RR 2, DS 3, DV 4, VD 5, MAN H7-4; BSY 6 and bit 7 read 0, ASSUMED LED-VID-0015)", (unsigned)n);
        check(c, n == 0x3Fu, msg);
        check(c, vm_status(w) == 0u, "status: a second read is not 0 (a read clears bits 0 to 5, MAN H7-4)");
        ds_vdc_flag(w, 5u);
        ds_vdc_flag(w, 2u);
        vs_port_word(w, 5u, 0x0000u); /* disabling later does not clear a raised flag */
        vs_port_word(w, 0x0Fu, 0x0000u);
        n = vm_status(w);
        check(c, n == 0x24u, "status: disabling the enables after the flags were raised did not leave the raised flags (VD and RR, $24) for the read");
        ds_vdc_flag(w, 0u); /* enabling later never creates a flag */
        vs_port_word(w, 5u, 0x000Fu);
        check(c, vm_status(w) == 0u, "status: a flag raised with its enable clear appeared when the enable was set later (D-144)");
        vs_port_word(w, 5u, 0x0000u);
        checks += 5u;
    }
    release_boot(&b);
    snprintf(got, gotcap, "assertions=%u", checks);
}

/* ---- model_param_vdc (plan 04-08; D-142, D-143, D-168, ER-10, ASSUMED LED-IRQ-0001 and LED-IRQ-0010) ----
 *   model_param_vdc   expect = rom=NAME;vd_shift=N;rr_shift=N;tol=N, request = "A||B" (until_mclk needed). The ROM (the vdc_stat guest) is run three times with the dev sink: with the default request, with A (model.vd_irq_phase=hdw_start) and with B (model.rcr_origin=line_before).
 *   In each run the status reads of the VDC window ($1FE000, kind R, with the stamp of the access) are taken from the dev log: the VD stamp is the stamp of the LAST read whose value has bit 5 set (the read in the IRQ1 handler of the VBlank interrupt; the foreground read before it also has bit 5), the RR stamp that of the FIRST read whose value has bit 2 set (the read in the
 *   handler of the RCR interrupt).
 *   The stamps are judged from the dev log and the literals of the case, never from a literal copied out of the core: the VD stamp of run A minus that of the default run is vd_shift (+96: the window start of HDS = 2 at divider 4 is (2 + 1) * 8 * 4 master clocks after the line start, ASSUMED LED-IRQ-0001) and the RR stamp of run B minus that of the default run is rr_shift (-1365: the
 *   origin line is one line earlier, so the same RCR value matches one line sooner, ASSUMED LED-IRQ-0010), each within plus or minus tol master clocks. tol is the longest instruction of the foreground wait loop of the fixture, lda abs of 5 cycles at 3 master clocks (15): an interrupt is taken at the first instruction boundary after the event, so the stamp of the handler read
 * moves by the shift
 *   less than one instruction. The two cross rows hold the other parameter fixed: the VD stamp of run B and the RR stamp of run A equal those of the default run within tol (each parameter moves its own event only). Evidence E0: the model compared with the case's own literals; no hardware or accuracy claim.
 */
#define MV_DEV_CAP 512u
#define MV_PHYS_STATUS 0x1FE000u

struct mv_stamps {
    uint64_t vd;
    uint64_t rr;
    int have_vd;
    int have_rr;
};

/* run the ROM over the request with a dev sink and take the two status read stamps; 0 on success, else a harness text in err */
static const char *mv_run(const char *rom, const char *request, uint64_t until, struct ctx *c, struct mv_stamps *out)
{
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_dev_sink *dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    unsigned char *drec = (unsigned char *)calloc((size_t)MV_DEV_CAP, DS_DEV_RECORD_BYTES);
    const char *err;
    uint32_t count;
    uint32_t i;

    memset(out, 0, sizeof *out);
    if (dsk == NULL || drec == NULL) {
        free(dsk);
        free(drec);
        return "out of memory for the dev sink of a model_param_vdc run";
    }
    err = boot_built_rom(rom, request, &b, &ci);
    if (err != NULL) {
        free(dsk);
        free(drec);
        return err;
    }
    dsk->records = drec;
    dsk->capacity = MV_DEV_CAP;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.dev = dsk;
    (void)ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
    note_run(c, res.machine_time, res.reason);
    release_boot(&b);
    count = vd_u32(dsk->header);
    if (res.reason != (uint32_t)DS_RUN_REASON_UNTIL || vd_u32(dsk->header + 4) != 0u) {
        free(dsk);
        free(drec);
        return "a model_param_vdc run did not reach its deadline or the dev sink lost a record: the log is the evidence here and would be incomplete";
    }
    for (i = 0u; i < count; i++) {
        const unsigned char *r = drec + (size_t)i * DS_DEV_RECORD_BYTES;

        if ((uint32_t)r[13] == (uint32_t)DS_DEV_READ && vd_u32(r + 8) == MV_PHYS_STATUS) {
            uint64_t stamp = vd_u64(r);

            if ((r[12] & 0x20u) != 0u) {
                out->vd = stamp;
                out->have_vd = 1;
            }
            if ((r[12] & 0x04u) != 0u && !out->have_rr) {
                out->rr = stamp;
                out->have_rr = 1;
            }
        }
    }
    free(dsk);
    free(drec);
    return NULL;
}

/* the signed difference a - b as int64 without overflow (the stamps are far below 2^62) */
static int64_t mv_diff(uint64_t a, uint64_t b)
{
    return a >= b ? (int64_t)(a - b) : -(int64_t)(b - a);
}

static int64_t mv_abs(int64_t v)
{
    return v < 0 ? -v : v;
}

/* one signed decimal number: an optional minus and digits; 0 on success */
static int mv_parse_signed(const char *v, int64_t *out)
{
    uint64_t u = 0u;
    int neg = v[0] == '-';

    if (parse_u64(v + (neg ? 1 : 0), &u) != 0 || u > UINT64_C(1000000000)) {
        return -1;
    }
    *out = neg ? -(int64_t)u : (int64_t)u;
    return 0;
}

void kind_model_param_vdc(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[64];
    char req_a[128];
    char req_b[128];
    const char *sep;
    const char *p;
    uint64_t until = 0u;
    uint64_t tol_u = 0u;
    int64_t vd_shift = 0;
    int64_t rr_shift = 0;
    int have_rom = 0;
    int have_vd = 0;
    int have_rr = 0;
    int have_tol = 0;
    struct mv_stamps base;
    struct mv_stamps run_a;
    struct mv_stamps run_b;
    const char *err;
    char msg[320];
    int64_t tol;
    int64_t d;

    if (!k->has[K_EXPECT] || !k->has[K_REQUEST] || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u) {
        harness_error(c, "a model_param_vdc case needs until_mclk, request = A||B and expect = rom=NAME;vd_shift=N;rr_shift=N;tol=N");
        return;
    }
    sep = strstr(k->val[K_REQUEST], "||");
    if (sep == NULL || (size_t)(sep - k->val[K_REQUEST]) >= sizeof req_a || strlen(sep + 2) >= sizeof req_b || strstr(sep + 2, "||") != NULL) {
        harness_error(c, "the request of a model_param_vdc case is exactly two requests separated by ||");
        return;
    }
    memcpy(req_a, k->val[K_REQUEST], (size_t)(sep - k->val[K_REQUEST]));
    req_a[sep - k->val[K_REQUEST]] = '\0';
    memcpy(req_b, sep + 2, strlen(sep + 2) + 1u);
    for (p = k->val[K_EXPECT]; *p != '\0';) {
        const char *end = strchr(p, ';');
        size_t n = end != NULL ? (size_t)(end - p) : strlen(p);
        char item[80];

        if (n == 0u || n >= sizeof item) {
            harness_error(c, "a model_param_vdc expect item is empty or too long");
            return;
        }
        memcpy(item, p, n);
        item[n] = '\0';
        if (strncmp(item, "rom=", 4u) == 0 && !have_rom && n - 4u < sizeof rom) {
            memcpy(rom, item + 4, n - 4u + 1u);
            have_rom = 1;
        } else if (strncmp(item, "vd_shift=", 9u) == 0 && !have_vd && mv_parse_signed(item + 9, &vd_shift) == 0) {
            have_vd = 1;
        } else if (strncmp(item, "rr_shift=", 9u) == 0 && !have_rr && mv_parse_signed(item + 9, &rr_shift) == 0) {
            have_rr = 1;
        } else if (strncmp(item, "tol=", 4u) == 0 && !have_tol && parse_u64(item + 4, &tol_u) == 0 && tol_u <= 21u) {
            have_tol = 1;
        } else {
            harness_error(c, "a model_param_vdc expect item is unknown, repeated or out of range (tol is at most 21 master clocks)");
            return;
        }
        p = end != NULL ? end + 1 : p + n;
    }
    if (!have_rom || !have_vd || !have_rr || !have_tol) {
        harness_error(c, "a model_param_vdc expect needs rom, vd_shift, rr_shift and tol");
        return;
    }
    tol = (int64_t)tol_u;
    err = mv_run(rom, "", until, c, &base);
    if (err == NULL) {
        err = mv_run(rom, req_a, until, c, &run_a);
    }
    if (err == NULL) {
        err = mv_run(rom, req_b, until, c, &run_b);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    check(c, base.have_vd && base.have_rr && run_a.have_vd && run_a.have_rr && run_b.have_vd && run_b.have_rr, "a run of the guest has no status read in the dev log with bit 5 (VD) or bit 2 (RR) set: the stamps cannot be taken");
    if (c->failed != 0) {
        snprintf(got, gotcap, "stamps missing");
        return;
    }
    d = mv_diff(run_a.vd, base.vd);
    snprintf(msg, sizeof msg, "vd_irq_phase=hdw_start moved the VD status-read stamp by %lld master clocks against the default run, the case says %lld within +-%lld (ASSUMED LED-IRQ-0001)", (long long)d, (long long)vd_shift, (long long)tol);
    check(c, mv_abs(d - vd_shift) <= tol, msg);
    d = mv_diff(run_b.rr, base.rr);
    snprintf(msg, sizeof msg, "rcr_origin=line_before moved the RR status-read stamp by %lld master clocks against the default run, the case says %lld within +-%lld (ASSUMED LED-IRQ-0010)", (long long)d, (long long)rr_shift, (long long)tol);
    check(c, mv_abs(d - rr_shift) <= tol, msg);
    d = mv_diff(run_b.vd, base.vd);
    snprintf(msg, sizeof msg, "rcr_origin=line_before moved the VD status-read stamp by %lld master clocks: the other parameter must leave it within +-%lld", (long long)d, (long long)tol);
    check(c, mv_abs(d) <= tol, msg);
    d = mv_diff(run_a.rr, base.rr);
    snprintf(msg, sizeof msg, "vd_irq_phase=hdw_start moved the RR status-read stamp by %lld master clocks: the other parameter must leave it within +-%lld", (long long)d, (long long)tol);
    check(c, mv_abs(d) <= tol, msg);
    snprintf(got, gotcap, "vd_shift=%lld rr_shift=%lld tol=%lld", (long long)mv_diff(run_a.vd, base.vd), (long long)mv_diff(run_b.rr, base.rr), (long long)tol);
}

/* ---- vdc_advance_pure (plan 04-10; D-151, D-157, D-175; E0) ---- */

#define AP_HORIZON 36000u   /* master clocks of one scenario: the display starts of the model's vertical cycle fall at 6825 and 34125 */
#define AP_OPS_MAX 5u
#define AP_SAMPLES_MAX 90u
#define AP_WORDS 32768u     /* the VRAM words that exist (ASSUMED LED-VID-0007); a word address above reads 0 and a write there is dropped, written here */
#define AP_REG_SOUR 0x10u
#define AP_REG_DESR 0x11u
#define AP_REG_LENR 0x12u

/*
 * The model of this kind is a naive reference: it steps ONE master clock at a time with counters and shares no formula, table or code with vdc.c or beam.c. The vertical setup is VPR = $0302 (VDS 3: 5 lines, VSW 2: 3 lines), VDW = 9 (10 lines), VCR = 2, a cycle of 20 lines that restarts at frame line 0 (frame 262 lines): line l is a display line when
 * (l mod 262) mod 20 is in 5..14 and its display start when it is 5 (D-139, written here as arithmetic, ASSUMED LED-VID-0001). The transfer: a LENR high byte write starts it at once when the bus is free (not a display line, or burst) and else leaves it pending until a line event finds the bus free; one word per 4 * divider clocks (the dots per word is 4, ASSUMED
 * LED-VID-0014, and the divider of the VCE control register 0 to 3 is 4, 3, 2, 2, MAN H6-3 and ASSUMED LED-VID-0002, both written here); at a display start the burst latch is !(CR bits 7 and 6) and a frame that is not burst cancels a pending or running transfer; the counters are those reached, LENR ends at $FFFF and DV is raised then when DCR bit 1 is set.
 */
struct ap_model {
    uint8_t vram[65536];
    uint32_t sour;
    uint32_t desr;
    uint32_t lenr;
    uint32_t dcr;
    uint32_t cr;
    uint32_t state; /* 0 idle, 1 pending, 2 running */
    uint32_t phase; /* master clocks since the start or the last word */
    uint32_t word;  /* master clocks per word */
    uint32_t moved; /* words moved since the start */
    int burst;
    int dv;
    uint64_t line;
    uint32_t line_clk;
    uint64_t now;
    unsigned cancels;
    unsigned completes;
    unsigned pendings;
};

struct ap_stat {
    unsigned scenarios;
    unsigned samples;
    unsigned cancels;
    unsigned completes;
    unsigned pendings;
    unsigned bad;
    char first[200];
};

static uint32_t ap_rd(const struct ap_model *m, uint32_t w)
{
    return w >= AP_WORDS ? 0u : (uint32_t)m->vram[2u * w] | ((uint32_t)m->vram[2u * w + 1u] << 8u);
}

static void ap_wr(struct ap_model *m, uint32_t w, uint32_t v)
{
    if (w < AP_WORDS) {
        m->vram[2u * w] = (uint8_t)(v & 0xFFu);
        m->vram[2u * w + 1u] = (uint8_t)(v >> 8u);
    }
}

static int ap_display(uint64_t line)
{
    uint32_t p = (uint32_t)((line % 262u) % 20u);

    return p >= 5u && p < 15u;
}

static int ap_dstart(uint64_t line)
{
    return (line % 262u) % 20u == 5u;
}

/* the line event of m->line at the time m->now */
static void ap_line(struct ap_model *m)
{
    if (ap_dstart(m->line)) {
        m->burst = (m->cr & 0xC0u) == 0u;
        if (!m->burst && m->state != 0u) {
            m->state = 0u;
            m->cancels++;
        }
    }
    if (m->state == 1u && (!ap_display(m->line) || m->burst)) {
        m->state = 2u;
        m->phase = 0u;
    }
}

/* one master clock: a word due at this clock first, then the line event of this clock (the words due at an event time complete before it) */
static void ap_tick(struct ap_model *m)
{
    m->now++;
    if (m->state == 2u) {
        m->phase++;
        if (m->phase == m->word) {
            m->phase = 0u;
            ap_wr(m, m->desr, ap_rd(m, m->sour));
            m->sour = (m->sour + ((m->dcr & 4u) != 0u ? 0xFFFFu : 1u)) & 0xFFFFu;
            m->desr = (m->desr + ((m->dcr & 8u) != 0u ? 0xFFFFu : 1u)) & 0xFFFFu;
            m->lenr = (m->lenr + 0xFFFFu) & 0xFFFFu;
            m->moved++;
            if (m->lenr == 0xFFFFu) {
                m->state = 0u;
                m->completes++;
                if ((m->dcr & 2u) != 0u) {
                    m->dv = 1;
                }
            }
        }
    }
    m->line_clk++;
    if (m->line_clk == 1365u) {
        m->line_clk = 0u;
        m->line++;
        ap_line(m);
    }
}

/* one data word to the register ar through the port at the stamp (the AR, the low byte, the high byte, which triggers) */
static void ap_port(struct ds_instance *w, uint32_t ar, uint32_t v, uint64_t stamp)
{
    ds_vdc_write(w, 0u, (uint8_t)ar, stamp);
    ds_vdc_write(w, 2u, (uint8_t)(v & 0xFFu), stamp);
    ds_vdc_write(w, 3u, (uint8_t)(v >> 8u), stamp);
}

static void ap_note(struct ap_stat *st, const char *what, uint32_t a, uint32_t b, uint64_t t)
{
    st->bad++;
    if (st->first[0] == '\0') {
        snprintf(st->first, sizeof st->first, "%s: model %u, core %u at stamp %llu", what, (unsigned)a, (unsigned)b, (unsigned long long)t);
    }
}

/* the core at stamp t against the model at the same clock: the three counters, the state, DV and every VRAM byte */
static void ap_compare(struct ap_stat *st, const struct ap_model *m, const struct ds_instance *w, uint64_t t)
{
    st->samples++;
    if ((uint32_t)w->vdc_reg[AP_REG_SOUR] != m->sour) {
        ap_note(st, "SOUR", m->sour, (uint32_t)w->vdc_reg[AP_REG_SOUR], t);
    } else if ((uint32_t)w->vdc_reg[AP_REG_DESR] != m->desr) {
        ap_note(st, "DESR", m->desr, (uint32_t)w->vdc_reg[AP_REG_DESR], t);
    } else if ((uint32_t)w->vdc_reg[AP_REG_LENR] != m->lenr) {
        ap_note(st, "LENR", m->lenr, (uint32_t)w->vdc_reg[AP_REG_LENR], t);
    } else if ((uint32_t)w->dma_state != m->state) {
        ap_note(st, "state", m->state, (uint32_t)w->dma_state, t);
    } else if ((((uint32_t)w->vdc_status >> 4u) & 1u) != (m->dv ? 1u : 0u)) {
        ap_note(st, "DV", m->dv ? 1u : 0u, ((uint32_t)w->vdc_status >> 4u) & 1u, t);
    } else if (memcmp(m->vram, w->vdc_vram, sizeof m->vram) != 0) {
        ap_note(st, "VRAM bytes", 0u, 1u, t);
    }
}

/* a lookahead sample: the stamps at, one before and one after a word boundary of the first words and the last words, and at, before and after a display start */
static int ap_wants(const struct ap_model *m)
{
    int near_word = m->state == 2u && (m->moved < 3u || m->lenr < 3u) && (m->phase + 1u == m->word || m->phase == 0u || m->phase == 1u);
    int near_start = (m->line_clk == 0u && ap_dstart(m->line)) || (m->line_clk == 1364u && ap_dstart(m->line + 1u)) || (m->line_clk == 1u && ap_dstart(m->line));

    return near_word || near_start;
}

static void ap_scenario(struct ctx *c, const struct kase *k, uint64_t *rng, struct ap_stat *st)
{
    static const uint32_t cr_values[4] = {0x0000u, 0x0040u, 0x0080u, 0x00C0u};
    static const uint32_t word_for_cr[4] = {16u, 12u, 8u, 8u}; /* 4 dots of the divider 4, 3, 2, 2 */
    static struct ap_model m;
    struct booted b;
    struct ds_instance *w;
    uint64_t op_at[AP_OPS_MAX];
    uint32_t op_kind[AP_OPS_MAX]; /* 0 start, 1 CR write */
    uint32_t op_val[AP_OPS_MAX][4];
    unsigned nops = 1u + (unsigned)(vd_mix(rng) % (uint64_t)AP_OPS_MAX);
    unsigned oi = 0u;
    unsigned i;
    unsigned j;
    uint32_t vce = (uint32_t)(vd_mix(rng) % 4u);
    uint32_t cr0 = cr_values[vd_mix(rng) % 4u];
    uint64_t t;
    unsigned samples_here = 0u;
    uint64_t stops[12];

    memset(&m, 0, sizeof m);
    for (i = 0u; i < nops; i++) {
        uint64_t at = 1u + vd_mix(rng) % (uint64_t)(AP_HORIZON - 2u);

        for (j = 0u; j < i; j++) { /* distinct stamps; the list is sorted below */
            if (op_at[j] == at) {
                at++;
            }
        }
        op_at[i] = at;
    }
    for (i = 0u; i < nops; i++) { /* sort ascending, with the kinds drawn after the order is fixed */
        for (j = i + 1u; j < nops; j++) {
            if (op_at[j] < op_at[i]) {
                uint64_t x = op_at[i];

                op_at[i] = op_at[j];
                op_at[j] = x;
            }
        }
    }
    for (i = 0u; i < nops; i++) {
        uint64_t r = vd_mix(rng);

        op_kind[i] = (i == 0u || (r & 3u) != 0u) ? 0u : 1u;
        op_val[i][0] = (uint32_t)((r >> 8u) % 4u) == 0u ? 0x7FF0u + (uint32_t)((r >> 16u) % 32u) : (uint32_t)((r >> 16u) % 6000u); /* SOUR: sometimes at the end of VRAM, so a copy crosses the out-of-range words */
        op_val[i][1] = (uint32_t)((r >> 24u) % 5u) == 0u ? 0xFFF0u + (uint32_t)((r >> 32u) % 16u) : (uint32_t)((r >> 32u) % 8000u);        /* DESR: sometimes at the top of the address space, so a decrement wraps and an increment wraps */
        op_val[i][2] = (uint32_t)((r >> 40u) % 3u) == 0u ? (uint32_t)(vd_mix(rng) % 3000u) : (uint32_t)(vd_mix(rng) % 300u);          /* LENR: M - 1 */
        op_val[i][3] = (uint32_t)(vd_mix(rng) & 0x0Eu);                                                                              /* DCR bits 1 to 3; for a CR write, the CR value index */
    }
    for (i = 0u; i < 12u; i++) {
        stops[i] = 1u + vd_mix(rng) % (uint64_t)AP_HORIZON;
    }
    w = vs_boot(c, k, &b);
    if (w == NULL) {
        return;
    }
    for (i = 0u; i < (uint32_t)sizeof m.vram; i++) { /* the same pseudo-random bytes in the instance and in the model */
        uint8_t v = (uint8_t)(vd_mix(rng) & 0xFFu);

        m.vram[i] = v;
        w->vdc_vram[i] = v;
    }
    w->vce_cr = (uint8_t)vce;
    m.cr = cr0;
    m.word = word_for_cr[vce];
    ap_port(w, 12u, 0x0302u, 0u); /* VPR */
    ap_port(w, 13u, 9u, 0u);      /* VDW */
    ap_port(w, 14u, 2u, 0u);      /* VCR */
    ap_port(w, 5u, cr0, 0u);      /* CR */
    ap_line(&m);                  /* the line event of line 0 at time 0: the core folds it at its first advance */
    for (t = 1u; t <= (uint64_t)AP_HORIZON; t++) {
        int want;
        int is_op = oi < nops && op_at[oi] == t;

        ap_tick(&m);
        want = is_op || ap_wants(&m);
        for (i = 0u; i < 12u && !want; i++) {
            want = stops[i] == t;
        }
        if ((want && samples_here < AP_SAMPLES_MAX) || t == (uint64_t)AP_HORIZON || is_op) {
            ds_vdc_advance(w, t, NULL);
            ap_compare(st, &m, w, t);
            samples_here++;
        }
        if (is_op) { /* the access after the fold of its own stamp, on the core and on the model */
            if (op_kind[oi] == 0u) {
                uint32_t sour = op_val[oi][0];
                uint32_t desr = op_val[oi][1];
                uint32_t lenr = op_val[oi][2];
                uint32_t dcr = op_val[oi][3];
                int free_bus = !ap_display(m.line) || m.burst;

                ap_port(w, 15u, dcr, t);
                ap_port(w, AP_REG_SOUR, sour, t);
                ap_port(w, AP_REG_DESR, desr, t);
                ds_vdc_write(w, 0u, (uint8_t)AP_REG_LENR, t);
                ds_vdc_write(w, 2u, (uint8_t)(lenr & 0xFFu), t); /* the low byte only stores */
                ds_vdc_write(w, 3u, (uint8_t)(lenr >> 8u), t);   /* the high byte starts the transfer */
                m.dcr = dcr;
                m.sour = sour;
                m.desr = desr;
                m.lenr = lenr;
                m.state = free_bus ? 2u : 1u;
                m.phase = 0u;
                m.moved = 0u;
                if (!free_bus) {
                    m.pendings++;
                }
            } else {
                uint32_t cr = cr_values[(op_val[oi][3] >> 1u) & 3u];

                ap_port(w, 5u, cr, t);
                m.cr = cr;
            }
            ap_compare(st, &m, w, t);
            oi++;
        }
    }
    st->cancels += m.cancels;
    st->completes += m.completes;
    st->pendings += m.pendings;
    st->scenarios++;
    release_boot(&b);
}

/* the divisor guard: a transfer started before any line event (the divider not latched yet) stays pending with a legal word time, and starts at the first line event with the word time of the divider */
static void ap_guard(struct ctx *c, const struct kase *k)
{
    struct booted b;
    struct ds_instance *w = vs_boot(c, k, &b);

    if (w == NULL) {
        return;
    }
    ap_port(w, AP_REG_SOUR, 0x0100u, 0u);
    ap_port(w, AP_REG_DESR, 0x0200u, 0u);
    ap_port(w, AP_REG_LENR, 0x0003u, 0u);
    check(c, w->dma_state == 1u && (w->dma_word_mclk == 8u || w->dma_word_mclk == 12u || w->dma_word_mclk == 16u), "a transfer started before the first line event is not pending with a word time of 8, 12 or 16 master clocks (the divisor guard of 04-18)");
    ds_vdc_advance(w, 0u, NULL);
    check(c, w->dma_state == 2u && w->dma_start == 0u && w->dma_word_mclk == 16u, "the pending transfer did not start at the first line event with the word time 16 of the divider 4 (MAN H6-3, ASSUMED LED-VID-0014)");
    release_boot(&b);
}

/*
 * vdc_advance_pure (D-151, D-157, D-175; E0): expect = scenarios=N;seed=N. N seeded random scenarios, each a booted synthetic instance whose VRAM holds pseudo-random bytes, with up to five accesses at random stamps (transfer starts through the port with random SOUR, DESR, LENR, DCR bits 1 to 3 and CR writes that make the frame burst or not),
 * advanced by direct ds_vdc_advance calls to a random partition of the stamps and to the stamps at, before and after the first word boundaries, the last words and every display start. After every call the counters SOUR, DESR and LENR, the state, DV and all 65,536 VRAM bytes of the core equal those of the naive per-clock model above. The case needs a
 * minimum number of cancels at display start, completions and pending starts so that it cannot pass without crossing the cases it names; and the divisor guard (a start before the first line event). Evidence tier E0: the closed form against a model of the same rule; no accuracy or hardware claim.
 */
void kind_vdc_advance_pure(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct ap_stat st;
    unsigned long long scen = 0u;
    unsigned long long seed = 0u;
    char msg[320];
    unsigned i;
    uint64_t rng;
    const char *e = k->has[K_EXPECT] ? k->val[K_EXPECT] : "";

    memset(&st, 0, sizeof st);
    if (sscanf(e, "scenarios=%llu;seed=%llu", &scen, &seed) != 2 || scen < 1u || scen > 4096u) {
        harness_error(c, "a vdc_advance_pure case needs expect = scenarios=N;seed=N (N from 1 to 4096) and a booting synthetic fixture");
        return;
    }
    rng = (uint64_t)seed;
    ap_guard(c, k);
    for (i = 0u; i < (unsigned)scen && !c->harness; i++) {
        ap_scenario(c, k, &rng, &st);
    }
    if (c->harness) {
        return;
    }
    snprintf(msg, sizeof msg, "the closed form differs from the per-clock model in %u comparisons of %u; the first: %s", st.bad, st.samples, st.first);
    check(c, st.bad == 0u, msg);
    check(c, st.cancels >= (unsigned)(scen / 16u) && st.completes >= (unsigned)(scen / 4u) && st.pendings >= (unsigned)(scen / 16u), "the scenarios crossed too few cancels, completions or pending starts: the case could pass without the display-start rules");
    snprintf(got, gotcap, "n=%u smp=%u cancel=%u done=%u pend=%u", st.scenarios, st.samples, st.cancels, st.completes, st.pendings);
}

/* ---- vdc_satb_data (plan 04-10; D-150, D-175 (c), T-04-36; E0) ---- */

/*
 * vdc_satb_data: the SATB words the VRAM-to-SATB copy stored, read from the instance bytes (D-175 (c)). The SATB is not CPU-readable (MAN H7-20), so the guest judges only the DS flag and this kind reads the 512 SATB bytes: expect = rom=NAME;satb=W0:STEP:COUNT, the words (W0 + STEP * i) & $FFFF for i below COUNT (256), the multiplication the assembler of the
 * fixture forms its pattern with (restated from fixtures/src/vdc_satb.s; the guest's own check is the DS flag). The fixture runs to the end of its DSV1 verdict under each of the three default power-on policies and every word, assembled here byte by byte (word i = byte 2i | byte 2i+1 << 8, no accessor and no port path), must equal the pattern: a copy that did not run, ran from
 * the wrong address, stored the bytes swapped or from the first pattern instead of the rewritten source fails. Evidence tier E0; no accuracy or hardware claim.
 * Plan 04.1-01 adds the second form expect = rom=NAME;satb=vram:ADDR:256 for a guest whose 256 SATB source words are sprite entries, not an arithmetic progression (the spr_basic fixture): the expected word j is the VRAM word ADDR + j of the instance at the end of the verdict, assembled here byte by byte from the instance's VRAM bytes (no accessor, no port path),
 * so the SATB must equal the source block the guest copied from, in every power-on policy of the list. The guest's own checks 3 and 4 compare that source block, read back through VRR, with the ROM table it was uploaded from (GUEST), so the chain ROM table, VRAM source block, SATB has two links read by two different paths. The block must hold at least 16 distinct words, so a
 * copy of an all-equal region cannot pass.
 */
void kind_vdc_satb_data(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    unsigned long long w0 = 0u;
    unsigned long long step = 0u;
    unsigned long long count = 0u;
    unsigned long long vaddr = 0u;
    int from_vram = 0;
    uint64_t until = 0u;
    unsigned p;
    unsigned words = 0u;
    const char *e = k->has[K_EXPECT] ? k->val[K_EXPECT] : "";

    int parsed = k->has[K_EXPECT];

    if (parsed && strstr(e, ";satb=vram:") != NULL) { /* the second form: the source block in the instance's VRAM (plan 04.1-01) */
        parsed = sscanf(e, "rom=%47[a-z0-9-];satb=vram:%llu:%llu", rom, &vaddr, &count) == 3 && count == 256u && vaddr + count <= 0x8000u;
        from_vram = parsed;
    } else if (parsed) {
        parsed = sscanf(e, "rom=%47[a-z0-9-];satb=%llu:%llu:%llu", rom, &w0, &step, &count) == 4 && w0 <= 0xFFFFu && step <= 0xFFFFu && count == 256u;
    }
    if (!parsed || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u) {
        harness_error(c, "a vdc_satb_data case needs until_mclk and expect = rom=NAME;satb=W0:STEP:256 (decimal numbers: the first word, the step and the word count) or rom=NAME;satb=vram:ADDR:256 (the source block at the VRAM word ADDR, below $8000)");
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
            harness_error(c, "the request of a vdc_satb_data case does not fit with a policy fragment");
            return;
        }
        err = boot_built_rom(rom, request, &b, &ci);
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
            harness_error(c, "ds_run refused the request of a vdc_satb_data case");
            return;
        }
        note_run(c, res.machine_time, res.reason);
        if (!dsv1_done(ci, NULL)) {
            release_boot(&b);
            harness_error(c, "the guest of a vdc_satb_data case did not reach the end of its DSV1 verdict before until_mclk: its SATB is not final");
            return;
        }
        if (from_vram) { /* the block must not be a copy of an all-equal region: at least 16 distinct words (a vacuity guard) */
            unsigned distinct = 0u;

            for (j = 0u; j < (unsigned)count; j++) {
                uint32_t wj = (uint32_t)ci->vdc_vram[2u * (vaddr + j)] | ((uint32_t)ci->vdc_vram[2u * (vaddr + j) + 1u] << 8u);
                unsigned m;
                int seen = 0;

                for (m = 0u; m < j && !seen; m++) {
                    seen = wj == ((uint32_t)ci->vdc_vram[2u * (vaddr + m)] | ((uint32_t)ci->vdc_vram[2u * (vaddr + m) + 1u] << 8u));
                }
                distinct += seen ? 0u : 1u;
            }
            check(c, distinct >= 16u, "the source block of a vdc_satb_data case in the second form holds fewer than 16 distinct words: the case could pass on an empty copy");
        }
        for (j = 0u; j < (unsigned)count; j++) {
            uint32_t want = from_vram ? ((uint32_t)ci->vdc_vram[2u * (vaddr + j)] | ((uint32_t)ci->vdc_vram[2u * (vaddr + j) + 1u] << 8u)) : (uint32_t)((w0 + step * (unsigned long long)j) & 0xFFFFu);
            uint32_t have = (uint32_t)ci->vdc_satb[2u * j] | ((uint32_t)ci->vdc_satb[2u * j + 1u] << 8u);
            char msg[200];

            snprintf(msg, sizeof msg, "policy %s: SATB word %u holds $%04X, the pattern has $%04X", vd_policy_name(p), j, (unsigned)have, (unsigned)want);
            check(c, have == want, msg);
            words++;
        }
        release_boot(&b);
    }
    snprintf(got, gotcap, "words=%u policies=%u", words, (unsigned)VD_POLICIES);
}

/* ---- vdc_snapshot_mid_dma (plan 04-10; D-151, D-150, D-166, D-175; E0) ---- */

/*
 * vdc_snapshot_mid_dma: restores taken in the middle of a VRAM-to-VRAM transfer (forward and reverse), in the wait for DS after a SATB copy, and between the DVSSR high byte write and the VBlank that carries it out continue to the same end as the uninterrupted run. expect = stops=ROM:KIND,ROM:KIND,... with the KINDs fwd (a transfer
 * running with 8 to 40 words done and DCR bits 2 and 3 clear), rev (a transfer running with 1 to 3 words done and DCR bit 2 or 3 set), ds (a SATB copy done and DS not yet raised) and arm (the DVSSR high byte written, the copy not yet made). Each stop is chosen by machine state, not by time. For each stop the machine is run to it under a seeded-prng-v3 request, snapshotted,
 * restored into a twin booted under another seed and poisoned first, the restored instance must equal the source in every restorable member (VRAM and SATB among them), and the target run to the deadline must equal the uninterrupted run in machine time, steps, verdict RAM, digest and every member (D-166 pass 1 validation is what this relies on: dma_done, the word time, the time windows).
 */
struct sm_stop {
    char rom[48];
    char kind[8];
    int seen;       /* a transfer of the stop's direction was running at the previous call */
    uint32_t lenr0; /* LENR at the first call that saw it running: the words done since are LENR0 - LENR (LENR steps down by one per word, D-151) */
};

/* the words done are counted from LENR, not from the core's progress member: since the review fix H-01 the closed form is re-based at every apply, so dma_done is 0 between folds and says nothing about the progress */
static int sm_stop_cb(const struct ds_instance *inst, void *ctx)
{
    struct sm_stop *sp = (struct sm_stop *)ctx;
    uint32_t dcr = (uint32_t)inst->vdc_reg[15];

    if (strcmp(sp->kind, "fwd") == 0 || strcmp(sp->kind, "rev") == 0) {
        int want_rev = sp->kind[0] == 'r';
        int is_rev = (dcr & 0x0Cu) != 0u;
        uint32_t done;

        if (inst->dma_state != 2u || is_rev != want_rev) {
            sp->seen = 0;
            return 0;
        }
        if (!sp->seen) {
            sp->seen = 1;
            sp->lenr0 = (uint32_t)inst->vdc_reg[18];
            return 0;
        }
        done = (sp->lenr0 - (uint32_t)inst->vdc_reg[18]) & 0xFFFFu;
        return want_rev ? done >= 1u && done <= 3u : done >= 8u && done <= 40u;
    }
    if (strcmp(sp->kind, "ds") == 0) {
        return inst->satb_done_valid == 1u;
    }
    return inst->satb_pending == 1u;
}

static int sm_parse(const char *text, struct sm_stop *out, unsigned cap, unsigned *n)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    *n = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0 || strncmp(list, "stops=", 6u) != 0) {
        return -1;
    }
    rest = list + 6;
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        char *colon = strchr(item, ':');

        if (colon == NULL || *n >= cap || (size_t)(colon - item) >= sizeof out[0].rom || strlen(colon + 1) >= sizeof out[0].kind || strspn(item, "abcdefghijklmnopqrstuvwxyz0123456789-:") != strlen(item)) {
            return -1;
        }
        *colon = '\0';
        memcpy(out[*n].rom, item, strlen(item) + 1u);
        memcpy(out[*n].kind, colon + 1, strlen(colon + 1) + 1u);
        out[*n].seen = 0;
        out[*n].lenr0 = 0u;
        if (strcmp(out[*n].kind, "fwd") != 0 && strcmp(out[*n].kind, "rev") != 0 && strcmp(out[*n].kind, "ds") != 0 && strcmp(out[*n].kind, "arm") != 0) {
            return -1;
        }
        (*n)++;
    }
    return *n > 0u ? 0 : -1;
}

void kind_vdc_snapshot_mid_dma(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const req_src = "bind.powerup_policy=seeded-prng-v3;seed=5";
    static const char *const req_twin = "bind.powerup_policy=seeded-prng-v3;seed=6";
    struct sm_stop stops[8];
    unsigned n = 0u;
    unsigned i;
    unsigned restored = 0u;
    uint64_t until = 0u;
    uint8_t *snap;

    if (!k->has[K_EXPECT] || sm_parse(k->val[K_EXPECT], stops, 8u, &n) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u) {
        harness_error(c, "a vdc_snapshot_mid_dma case needs until_mclk and expect = stops=ROM:KIND,... with KIND fwd, rev, ds or arm (at most 8)");
        return;
    }
    snap = (uint8_t *)malloc(ds_snapshot_size());
    if (snap == NULL) {
        harness_error(c, "out of memory for the snapshot of the vdc_snapshot_mid_dma stops");
        return;
    }
    for (i = 0u; i < n && !c->harness; i++) {
        struct booted br;
        struct booted bs;
        struct booted bt;
        const struct ds_instance *ref = NULL;
        const struct ds_instance *cs = NULL;
        const struct ds_instance *ct = NULL;
        struct ds_run_request rq;
        struct ds_run_result res;
        uint64_t ref_steps;
        uint64_t steps1;
        uint8_t ref_digest[DS_DIGEST_BYTES];
        uint8_t dig[DS_DIGEST_BYTES];
        const char *err;
        const char *diff;
        char msg[240];

        err = boot_built_rom(stops[i].rom, req_src, &br, &ref);
        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        snprintf(msg, sizeof msg, "stop %u (%s %s): the uninterrupted run did not end on its deadline", i, stops[i].rom, stops[i].kind);
        check(c, ds_run((struct ds_instance *)(void *)br.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_UNTIL, msg);
        ref_steps = res.steps;
        check(c, ds_digest(ref, ref_digest) == 0u, "the digest of an uninterrupted run was refused");
        err = boot_built_rom(stops[i].rom, req_src, &bs, &cs);
        if (err == NULL) {
            err = boot_built_rom(stops[i].rom, req_twin, &bt, &ct);
            if (err != NULL) {
                release_boot(&bs);
            }
        }
        if (err != NULL) {
            release_boot(&br);
            harness_error(c, err);
            break;
        }
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.stop = sm_stop_cb;
        rq.stop_ctx = &stops[i];
        snprintf(msg, sizeof msg, "stop %u (%s %s): the run did not stop at the machine state the stop names", i, stops[i].rom, stops[i].kind);
        check(c, ds_run((struct ds_instance *)(void *)bs.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_CALLBACK, msg);
        steps1 = res.steps;
        if (!c->failed) {
            snprintf(msg, sizeof msg, "stop %u (%s %s): ds_snapshot of the source was refused", i, stops[i].rom, stops[i].kind);
            check(c, ds_snapshot((struct ds_instance *)(void *)bs.mem, snap, ds_snapshot_size(), NULL) == (uint32_t)DS_SNAPREF_NONE, msg);
            snap_poison((struct ds_instance *)(void *)bt.mem, 0xA5u);
            snprintf(msg, sizeof msg, "stop %u (%s %s): ds_restore refused the snapshot of a machine in the state the stop names", i, stops[i].rom, stops[i].kind);
            check(c, ds_restore((struct ds_instance *)(void *)bt.mem, snap, ds_snapshot_size()) == (uint32_t)DS_SNAPREF_NONE, msg);
            diff = snap_first_diff(ct, cs);
            snprintf(msg, sizeof msg, "stop %u (%s %s): the restored instance differs from the source in the member %s", i, stops[i].rom, stops[i].kind, diff != NULL ? diff : "-");
            check(c, diff == NULL, msg);
            memset(&rq, 0, sizeof rq);
            memset(&res, 0, sizeof res);
            rq.struct_size = (uint32_t)sizeof rq;
            rq.until_mclk = until;
            snprintf(msg, sizeof msg, "stop %u (%s %s): the restored target did not run on to the deadline", i, stops[i].rom, stops[i].kind);
            check(c, ds_run((struct ds_instance *)(void *)bt.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_UNTIL, msg);
            snprintf(msg, sizeof msg, "stop %u (%s %s): the machine time or the step count of the restored chain differs from the uninterrupted run", i, stops[i].rom, stops[i].kind);
            check(c, ct->machine_time == ref->machine_time && steps1 + res.steps == ref_steps, msg);
            snprintf(msg, sizeof msg, "stop %u (%s %s): the verdict RAM or the digest of the restored chain differs from the uninterrupted run", i, stops[i].rom, stops[i].kind);
            check(c, memcmp(ct->ram8k + V_OFF, ref->ram8k + V_OFF, V_SIZE) == 0 && ds_digest(ct, dig) == 0u && memcmp(dig, ref_digest, DS_DIGEST_BYTES) == 0, msg);
            diff = snap_first_diff(ct, ref);
            snprintf(msg, sizeof msg, "stop %u (%s %s): the member %s of the restored chain differs from the uninterrupted run (VRAM, SATB and the DMA state included)", i, stops[i].rom, stops[i].kind, diff != NULL ? diff : "-");
            check(c, diff == NULL, msg);
            restored++;
        }
        release_boot(&bs);
        release_boot(&bt);
        release_boot(&br);
    }
    free(snap);
    snprintf(got, gotcap, "stops=%u restored=%u", n, restored);
}

/* ---- vdc_dma_lenr_live (review fix H-01; D-151, D-137, D-166; E0) ---- */

/*
 * vdc_dma_lenr_live: a VRAM-to-VRAM transfer whose LENR low byte is rewritten while it runs (LENR is a live counter, D-151, and the low byte only stores) restores at every stop and continues to the same end. expect = first=N;lsb=V;then=N;keep=N;every=N (decimal). A booted image whose guest never touches the VDC (cpu-alu) is run with ds_run;
 * between runs the VDC is driven directly at the machine time (the fold to that time first, then the port writes, the production order of D-137): SOUR = DESR = 0 and LENR = $FFFF start the transfer (CR is 0, so every frame is burst and nothing cancels it, D-157); after `first` words (word time 16 master clocks: 4 dots, ASSUMED
 * LED-VID-0014, times the divider 4 of CR 0, MAN H6-3, written here) the low byte is written V; after `then` more words (the review's repro 60,000 / $FF / 5,600: the transfer is then past 65,536 words since its start) the machine stops; then the low byte is written V every 2,048 master clocks (128 words) `keep` times, which keeps the transfer alive
 * past three times 65,536 words, and the machine stops after every `every`-th write; then it runs to until_mclk with no write and the transfer ends (LENR $FFFF, idle). At every stop: while a transfer runs the progress member dma_done is 0 and dma_start is at most one word time behind the machine time (the progress is bounded by construction,
 * so no member counts the words since the start and nothing can wrap at 2^32 words however long a guest keeps the transfer alive; a 2^32-word run itself is about 4.3e9 word moves and is not run), and the machine is snapshotted, restored into a twin booted under another power-on seed and poisoned first (member by member equal to the source), and the
 * run goes on with the twin. The chain must equal the uninterrupted run of the same script in machine time, steps, digest and every restorable member. Before the fix the restore past 65,536 words was refused (the review's H-01). E0: the core against itself, no accuracy or hardware claim.
 */
#define LL_WORD_MCLK 16u   /* master clocks per word: DS_CONST_vdc_dma_dots_per_word 4 (ASSUMED LED-VID-0014) times the divider 4 of CR 0 (MAN H6-3), written here */
#define LL_KEEP_MCLK 2048u /* 128 words between two low-byte writes of the keep phase: LENR stays between $xx7F and $xxFF */
#define LL_START_MCLK 1000u

struct ll_param {
    unsigned first;
    unsigned lsb;
    unsigned then;
    unsigned keep;
    unsigned every;
};

struct ll_run {
    struct ds_instance *cur;
    struct ds_instance *spare; /* NULL: the uninterrupted run, no restore */
    uint8_t *snap;
    uint64_t steps;
    uint64_t t0;
    unsigned stops;
    unsigned restores;
};

/* the fold to the machine time, then AR = ar and the low byte, and the high byte when msb is set, all at the machine time (the order of a bus access, D-137) */
static void ll_port(struct ds_instance *w, uint32_t ar, uint32_t value, int msb)
{
    uint64_t t = w->machine_time;

    ds_vdc_advance(w, t, NULL);
    ds_vdc_write(w, 0u, (uint8_t)ar, t);
    ds_vdc_write(w, 2u, (uint8_t)(value & 0xFFu), t);
    if (msb) {
        ds_vdc_write(w, 3u, (uint8_t)((value >> 8u) & 0xFFu), t);
    }
}

static void ll_to(struct ctx *c, struct ll_run *r, uint64_t until)
{
    struct ds_run_request rq;
    struct ds_run_result res;

    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    check(c, ds_run(r->cur, &rq, &res) == (uint32_t)DS_RUN_REASON_UNTIL, "a run of the vdc_dma_lenr_live script did not end on its deadline");
    r->steps += res.steps;
}

static void ll_stop(struct ctx *c, struct ll_run *r, const char *where)
{
    const struct ds_instance *s = r->cur;
    const char *diff;
    char msg[240];

    r->stops++;
    if (s->dma_state == 2u) {
        snprintf(msg, sizeof msg, "%s: a running transfer has dma_done %u or a start more than one word time behind the machine time (the progress must stay bounded, H-01)", where, (unsigned)s->dma_done);
        check(c, s->dma_done == 0u && s->dma_start <= s->machine_time && s->machine_time - s->dma_start <= (uint64_t)s->dma_word_mclk, msg);
    }
    if (r->spare == NULL || c->failed) {
        return;
    }
    snprintf(msg, sizeof msg, "%s: ds_snapshot of the source was refused", where);
    check(c, ds_snapshot(r->cur, r->snap, ds_snapshot_size(), NULL) == (uint32_t)DS_SNAPREF_NONE, msg);
    snap_poison(r->spare, 0xA5u);
    snprintf(msg, sizeof msg, "%s: ds_restore refused the snapshot of a running transfer whose LENR low byte was rewritten (H-01)", where);
    check(c, ds_restore(r->spare, r->snap, ds_snapshot_size()) == (uint32_t)DS_SNAPREF_NONE, msg);
    diff = snap_first_diff(r->spare, r->cur);
    snprintf(msg, sizeof msg, "%s: the restored instance differs from the source in the member %s", where, diff != NULL ? diff : "-");
    check(c, diff == NULL, msg);
    if (!c->failed) {
        struct ds_instance *t = r->cur;

        r->cur = r->spare;
        r->spare = t;
        r->restores++;
    }
}

/* the script of the case on r->cur; the words since the start are (machine time - t0) / 16 while the transfer runs */
static void ll_script(struct ctx *c, const struct ll_param *p, struct ll_run *r, uint64_t until)
{
    unsigned j;

    ll_to(c, r, LL_START_MCLK);
    ll_port(r->cur, 0x10u, 0u, 1);
    ll_port(r->cur, 0x11u, 0u, 1);
    ll_port(r->cur, 0x12u, 0xFFFFu, 1);
    r->t0 = r->cur->machine_time;
    check(c, r->cur->dma_state == 2u, "the LENR high byte write did not start the transfer at once (CR 0: the frame is burst, the bus is free, D-151, D-157)");
    ll_to(c, r, r->t0 + (uint64_t)p->first * LL_WORD_MCLK);
    ll_stop(c, r, "after the first words");
    check(c, r->cur->dma_state == 2u, "the transfer was not running after the first words");
    ll_port(r->cur, 0x12u, p->lsb, 0);
    ll_to(c, r, r->cur->machine_time + (uint64_t)p->then * LL_WORD_MCLK);
    check(c, r->cur->dma_state == 2u && (r->cur->machine_time - r->t0) / LL_WORD_MCLK > UINT64_C(65536), "the transfer was not running past 65,536 words since its start (the repro of H-01)");
    ll_stop(c, r, "past 65,536 words");
    for (j = 1u; j <= p->keep && !c->failed; j++) {
        ll_port(r->cur, 0x12u, p->lsb, 0);
        ll_to(c, r, r->cur->machine_time + LL_KEEP_MCLK);
        if (j % p->every == 0u) {
            ll_stop(c, r, "keep-alive");
        }
    }
    check(c, r->cur->dma_state == 2u && (r->cur->machine_time - r->t0) / LL_WORD_MCLK > UINT64_C(3) * UINT64_C(65536), "the keep phase did not keep the transfer running past three times 65,536 words");
    ll_to(c, r, until);
    ll_stop(c, r, "after the end");
    check(c, r->cur->dma_state == 0u && r->cur->vdc_reg[0x12] == 0xFFFFu, "the transfer did not end with LENR $FFFF once the low byte was no longer rewritten");
}

void kind_vdc_dma_lenr_live(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const req_src = "bind.powerup_policy=seeded-prng-v3;seed=5";
    static const char *const req_twin = "bind.powerup_policy=seeded-prng-v3;seed=6";
    struct ll_param p;
    struct ll_run ref;
    struct ll_run chain;
    struct booted br;
    struct booted bs;
    struct booted bt;
    const struct ds_instance *ci = NULL;
    char rom[48];
    char tail = '\0';
    uint64_t until = 0u;
    uint8_t dig_ref[DS_DIGEST_BYTES];
    uint8_t dig[DS_DIGEST_BYTES];
    const char *err;
    const char *diff;
    char msg[240];

    memset(&p, 0, sizeof p);
    if (!k->has[K_EXPECT] || sscanf(k->val[K_EXPECT], "first=%u;lsb=%u;then=%u;keep=%u;every=%u%c", &p.first, &p.lsb, &p.then, &p.keep, &p.every, &tail) != 5 || p.first == 0u || p.first > 65000u || p.lsb > 0xFFu || p.then == 0u || p.keep == 0u || p.every == 0u
        || authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 1000u) {
        harness_error(c, "a vdc_dma_lenr_live case needs fixture = authored:NAME, until_mclk and expect = first=N;lsb=V;then=N;keep=N;every=N (decimal; first at most 65,000, V at most 255)");
        return;
    }
    err = boot_built_rom(rom, req_src, &br, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    memset(&ref, 0, sizeof ref);
    ref.cur = (struct ds_instance *)(void *)br.mem;
    ll_script(c, &p, &ref, until);
    check(c, ds_digest(ref.cur, dig_ref) == 0u, "the digest of the uninterrupted run was refused");
    err = boot_built_rom(rom, req_src, &bs, &ci);
    if (err == NULL) {
        err = boot_built_rom(rom, req_twin, &bt, &ci);
        if (err != NULL) {
            release_boot(&bs);
        }
    }
    if (err != NULL) {
        release_boot(&br);
        harness_error(c, err);
        return;
    }
    memset(&chain, 0, sizeof chain);
    chain.cur = (struct ds_instance *)(void *)bs.mem;
    chain.spare = (struct ds_instance *)(void *)bt.mem;
    chain.snap = (uint8_t *)malloc(ds_snapshot_size());
    if (chain.snap == NULL) {
        harness_error(c, "out of memory for the snapshot of the vdc_dma_lenr_live stops");
    } else if (!c->failed) {
        ll_script(c, &p, &chain, until);
        snprintf(msg, sizeof msg, "the restored chain (%u restores) differs from the uninterrupted run in machine time or steps", chain.restores);
        check(c, chain.cur->machine_time == ref.cur->machine_time && chain.steps == ref.steps, msg);
        check(c, ds_digest(chain.cur, dig) == 0u && memcmp(dig, dig_ref, DS_DIGEST_BYTES) == 0, "the digest of the restored chain differs from the uninterrupted run");
        diff = snap_first_diff(chain.cur, ref.cur);
        snprintf(msg, sizeof msg, "the member %s of the restored chain differs from the uninterrupted run (VRAM and the DMA state included)", diff != NULL ? diff : "-");
        check(c, diff == NULL, msg);
        check(c, chain.restores == chain.stops && chain.restores >= 3u, "the chain did not restore at every stop");
    }
    free(chain.snap);
    release_boot(&bs);
    release_boot(&bt);
    release_boot(&br);
    snprintf(got, gotcap, "stops=%u restored=%u", chain.stops, chain.restores);
}

/* ---- vdc_vblank_wrap (review fix M-02; D-139, D-143, D-150; ASSUMED LED-IRQ-0001, LED-VID-0005; E0) ---- */

/*
 * vdc_vblank_wrap: the VBlank state entry when the display state ends on the last line of the frame. expect = rows=VDS:WANT,...;frames=N with WANT each (VD in every frame from frame 1 on, the SATB copy made once, DS raised once, MWR latched) or none (no VD, no copy, no DS, no latch) (decimal VDS, 0 to 255).
 * For each row and for each of the two VD phases (the default line_start and model.vd_irq_phase=hdw_start) a synthetic image of the case's fixture is booted (262 lines per frame, the fixture's binding) and the VDC is driven without a guest: VPR = VDS << 8 | 2, VDW = 239 (240 display lines), VCR = 3, CR = $08 (VD enabled), DCR = $01 (DS enabled),
 * MWR = $10 and the DVSSR high byte written once (the copy armed). The timeline is folded frame by frame with ds_vdc_advance to the last clock of the frame (262 * 1365 master clocks, ASSUMED LED-VID-0001) and the status is read there (the read clears the flags, MAN H7-4).
 * With VDS 19 the states VDS (21 lines) and VDW (240) end one line before the frame wraps; with VDS 20 they fill the frame exactly (22 + 240 = 262) and VDW ends on its last line; with VDS 21 the wrap cuts VDW (D-139: no VD). The literal for VDS 20 is the decision of the review fix M-02 (the VBlank state entry is the frame-wrap line when VDW
 * completes on the last line of the frame: VD, the SATB copy and the MWR latch happen there, then the vertical cycle restarts), class ASSUMED LED-IRQ-0001 and LED-VID-0005; the bit positions of VD and DS are MAN H7-4. E0: no accuracy or hardware claim.
 */
#define VW_ROWS_MAX 4u
#define VW_FRAME_MCLK (262u * 1365u) /* lines per frame (the bound field) times the line length (ASSUMED LED-VID-0001), written here */
#define VW_MWR 0x10u

struct vw_row {
    unsigned vds;
    int each;
};

static int vw_parse(const char *text, struct vw_row *rows, unsigned *n, unsigned *frames)
{
    char list[PK_LIST_MAX];
    char *rest;
    char *item;
    char *semi;

    *n = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0 || strncmp(list, "rows=", 5u) != 0 || (semi = strchr(list, ';')) == NULL || sscanf(semi + 1, "frames=%u", frames) != 1 || *frames < 3u || *frames > 16u) {
        return -1;
    }
    *semi = '\0';
    rest = list + 5;
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        char want[8];

        if (*n >= VW_ROWS_MAX || sscanf(item, "%u:%7[a-z]", &rows[*n].vds, want) != 2 || rows[*n].vds > 255u || (strcmp(want, "each") != 0 && strcmp(want, "none") != 0)) {
            return -1;
        }
        rows[*n].each = strcmp(want, "each") == 0;
        (*n)++;
    }
    return *n > 0u ? 0 : -1;
}

void kind_vdc_vblank_wrap(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const phases[2] = {"", "model.vd_irq_phase=hdw_start"};
    struct vw_row rows[VW_ROWS_MAX];
    unsigned n = 0u;
    unsigned frames = 0u;
    unsigned i;
    unsigned runs = 0u;

    if (!k->has[K_EXPECT] || vw_parse(k->val[K_EXPECT], rows, &n, &frames) != 0) {
        harness_error(c, "a vdc_vblank_wrap case needs expect = rows=VDS:each|none,...;frames=N (at most 4 rows, 3 to 16 frames)");
        return;
    }
    for (i = 0u; i < n && !c->harness; i++) {
        unsigned ph;

        for (ph = 0u; ph < 2u && !c->harness; ph++) {
            struct spec s;
            struct booted b;
            const struct ds_instance *ci = NULL;
            struct ds_instance *w;
            const char *err = boot_accepted(k->val[K_FIXTURE], phases[ph], &b, &s, &ci);
            unsigned f;
            unsigned vd_frames = 0u;
            unsigned ds_count = 0u;
            char msg[240];

            if (err != NULL) {
                harness_error(c, err);
                return;
            }
            w = (struct ds_instance *)(void *)b.mem;
            snprintf(msg, sizeof msg, "the fixture of the case does not bind 262 lines per frame (frame_lines %u)", (unsigned)w->frame_lines);
            check(c, w->frame_lines == 262u, msg);
            vs_port_word(w, 0x0Cu, (rows[i].vds << 8u) | 2u);
            vs_port_word(w, 0x0Du, 239u);
            vs_port_word(w, 0x0Eu, 3u);
            vs_port_word(w, 0x05u, 0x08u);
            vs_port_word(w, 0x0Fu, 0x01u);
            vs_port_word(w, 0x09u, VW_MWR);
            vs_port_word(w, 0x13u, 0x0100u);
            for (f = 0u; f < frames; f++) {
                uint64_t end = (uint64_t)(f + 1u) * (uint64_t)VW_FRAME_MCLK - UINT64_C(1);
                uint32_t st;

                ds_vdc_advance(w, end, NULL);
                st = (uint32_t)ds_vdc_read(w, 0u, end);
                if (f > 0u && (st & 0x20u) != 0u) {
                    vd_frames++;
                }
                if ((st & 0x08u) != 0u) {
                    ds_count++;
                }
            }
            if (rows[i].each) {
                snprintf(msg, sizeof msg, "VDS %u (%s): VD in %u of frames 1 to %u, the SATB copy pending %u, DS %u times, MWR latch $%02X (each frame wanted, one copy, one DS, latch $%02X)", rows[i].vds, ph == 0u ? "line_start" : "hdw_start", vd_frames, frames - 1u, (unsigned)w->satb_pending, ds_count,
                         (unsigned)w->mwr_active, VW_MWR);
                check(c, vd_frames == frames - 1u && w->satb_pending == 0u && ds_count == 1u && w->mwr_active == (uint16_t)VW_MWR, msg);
            } else {
                snprintf(msg, sizeof msg, "VDS %u (%s): VD in %u frames, the SATB copy pending %u, DS %u times, MWR latch $%02X (none wanted: the wrap cuts VDW)", rows[i].vds, ph == 0u ? "line_start" : "hdw_start", vd_frames, (unsigned)w->satb_pending, ds_count, (unsigned)w->mwr_active);
                check(c, vd_frames == 0u && w->satb_pending == 1u && ds_count == 0u && w->mwr_active == 0u, msg);
            }
            runs++;
            release_boot(&b);
        }
    }
    snprintf(got, gotcap, "rows=%u runs=%u", n, runs);
}

/* ---- kind vdc_poweron_policy_diff (plan 04-13; D-167, Pitfall 13, T-04-45) ---- */

#define VP_ITEMS_MAX 8u
#define VP_O_FRAME 8u    /* the layout of D-161 as this file knows it (hand-written, ER-13) */
#define VP_O_ACTIVE_W 24u
#define VP_O_BORDER 26u
#define VP_MIN_WINDOW 262u /* a window of fewer records than one frame of 262 lines would prove too little */

struct vp_item {
    char rom[48];
    uint64_t from; /* the first frame of the judged window: the setup frames before it may differ between policies, every record from it on must not */
};

static int vp_item(char *text, struct vp_item *it)
{
    char *at = strchr(text, '@');

    if (at == NULL || at == text || (size_t)(at - text) >= sizeof it->rom) {
        return -1;
    }
    *at = '\0';
    if (strspn(text, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(text) || parse_u64(at + 1, &it->from) != 0) {
        return -1;
    }
    memcpy(it->rom, text, strlen(text) + 1u);
    return 0;
}

/* the records of two streams, walked; wa and wb are the record pointer arrays (count entries each). Out: the number of records at or after frame `from` that are not byte for byte equal (nd), the number of those whose border differs (nb), the records in the window (nw), the last frame holding any difference and the number of records anywhere that differ (nall) */
static void vp_compare(const uint8_t *const *ra, const uint8_t *const *rb, uint32_t count, uint64_t from, uint64_t *nd, uint64_t *nb, uint64_t *nw, uint64_t *last, uint64_t *nall)
{
    uint32_t k;

    *nall = 0u;
    *nd = 0u;
    *nb = 0u;
    *nw = 0u;
    *last = 0u;
    for (k = 0u; k < count; k++) {
        uint32_t la = vd_u16(ra[k] + VD_O_REC_BYTES);
        uint32_t lb = vd_u16(rb[k] + VD_O_REC_BYTES);
        uint64_t frame = (uint64_t)vd_u32(ra[k] + VP_O_FRAME);
        int same = la == lb && memcmp(ra[k], rb[k], (size_t)la) == 0;

        if (frame >= from) {
            (*nw)++;
            if (!same) {
                (*nd)++;
            }
            if (vd_u16(ra[k] + VP_O_BORDER) != vd_u16(rb[k] + VP_O_BORDER)) {
                (*nb)++;
            }
        }
        if (!same) {
            *last = frame;
            (*nall)++;
        }
    }
}

/* one fixture under the three policies; control = 0: the verdict RAM and every record of the window must be equal under the three policies; control = 1: the border of some record of the window must differ between the zero policy and each seed. 0 on success, -1 on a harness error */
static int vp_one(struct ctx *c, const struct vp_item *it, int control, uint64_t until, uint64_t *window_out, uint64_t *border_out, uint64_t *last_out)
{
    struct vn_run run[VD_POLICIES];
    const uint8_t **rec[VD_POLICIES];
    unsigned p;
    int rc = 0;
    uint64_t cap = vd_stream_cap(until);
    uint64_t maxlast = 0u;
    uint64_t anyall = 0u;

    memset(run, 0, sizeof run);
    memset(rec, 0, sizeof rec);
    for (p = 0u; p < VD_POLICIES && rc == 0; p++) {
        char request[256];

        if (vd_policy_request(p, "", request, sizeof request) != 0 || vn_exec(c, it->rom, request, until, VN_LARGE, (uint32_t)cap, &run[p]) != 0) {
            rc = -1;
            break;
        }
        if (run[p].lost != 0u || run[p].stream == NULL || !run[p].header_ok || run[p].count == 0u) {
            harness_error(c, "a run of the vdc_poweron_policy_diff kind lost a record, stored none, or its sink header disagreed: the comparison would be incomplete (D-162)");
            rc = -1;
            break;
        }
        rec[p] = (const uint8_t **)malloc((size_t)run[p].count * sizeof *rec[p]);
        if (rec[p] == NULL) {
            harness_error(c, "out of memory for the record table of the vdc_poweron_policy_diff kind");
            rc = -1;
            break;
        }
        if (vd_walk(run[p].stream, run[p].bytes, (uint64_t)run[p].count, (const uint8_t **)rec[p]) != 0) {
            harness_error(c, "a stream of the vdc_poweron_policy_diff kind does not walk to its byte count: the record lengths disagree (D-161)");
            rc = -1;
            break;
        }
    }
    for (p = 1u; p < VD_POLICIES && rc == 0; p++) {
        uint64_t nd = 0u;
        uint64_t nb = 0u;
        uint64_t nw = 0u;
        uint64_t last = 0u;
        uint64_t nall = 0u;
        char msg[300];

        snprintf(msg, sizeof msg, "%s under %s: the machine time, the step count or the number of records differs from the zero policy run, so the two runs are not comparable record by record", it->rom, vd_policy_name(p));
        check(c, run[p].time == run[0].time && run[p].steps == run[0].steps && run[p].count == run[0].count && run[p].bytes == run[0].bytes, msg);
        if (run[p].count != run[0].count) {
            continue;
        }
        vp_compare((const uint8_t *const *)rec[0], (const uint8_t *const *)rec[p], run[0].count, it->from, &nd, &nb, &nw, &last, &nall);
        if (nall > 0u && last >= maxlast) {
            maxlast = last;
        }
        anyall += nall;
        snprintf(msg, sizeof msg, "%s: the window from frame %llu holds %llu records, fewer than one frame of %u lines: it would prove too little", it->rom, (unsigned long long)it->from, (unsigned long long)nw, VP_MIN_WINDOW);
        check(c, nw >= (uint64_t)VP_MIN_WINDOW, msg);
        if (nw > *window_out) {
            *window_out = nw;
        }
        if (last > *last_out) {
            *last_out = last;
        }
        if (control == 0) {
            snprintf(msg, sizeof msg, "%s under %s: the verdict RAM (the DSV1 bytes) differs from the zero policy run (D-167: a guest result must not depend on the power-on policy)", it->rom, vd_policy_name(p));
            check(c, memcmp(run[p].verdict, run[0].verdict, V_SIZE) == 0, msg);
            snprintf(msg, sizeof msg, "%s under %s: %llu records at or after frame %llu differ from the zero policy run (the last frame holding any difference is %llu): the power-on policy reaches the judged window (D-167)", it->rom, vd_policy_name(p), (unsigned long long)nd, (unsigned long long)it->from, (unsigned long long)last);
            check(c, nd == 0u, msg);
        } else {
            snprintf(msg, sizeof msg, "%s under %s: the border of no record at or after frame %llu differs from the zero policy run, so the control cannot show the guard fail: the palette entry the border reads is initialised or the policies do not differ", it->rom, vd_policy_name(p), (unsigned long long)it->from);
            check(c, nb > 0u, msg);
            *border_out += nb;
        }
    }
    if (rc == 0 && control == 0) {
        char msg[300];

        snprintf(msg, sizeof msg, "%s: the first judged frame %llu is not the first frame after the last record that differs between policies (%s): a later one would hide "
            "records from the guard, an earlier one is a record that differs", it->rom, (unsigned long long)it->from, anyall > 0u ? "a record differs up to a frame that the window must follow" : "no record differs, so the window starts at frame 0");
        check(c, it->from == (anyall > 0u ? maxlast + 1u : 0u), msg);
    }
    for (p = 0u; p < VD_POLICIES; p++) {
        free((void *)rec[p]);
        free(run[p].stream);
    }
    return rc;
}

void kind_vdc_poweron_policy_diff(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    struct vp_item roms[VP_ITEMS_MAX];
    struct vp_item ctl;
    unsigned nroms = 0u;
    unsigned seen = 0u;
    unsigned i;
    uint64_t until = 0u;
    uint64_t window = 0u;
    uint64_t borders = 0u;
    uint64_t last = 0u;
    uint64_t clast = 0u;
    uint64_t cwindow = 0u;

    memset(&ctl, 0, sizeof ctl);
    if (!k->has[K_EXPECT] || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 100000u || vd_stream_cap(until) == 0u || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a vdc_poweron_policy_diff case needs until_mclk and expect = roms=NAME@FRAME,NAME@FRAME;control=NAME@FRAME");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "roms=", 5u) == 0 && (seen & 1u) == 0u) {
            char *r2 = item + 5;
            char *w;
            int bad = 0;

            while (!bad && (w = pk_next_item(&r2, ',')) != NULL) {
                bad = nroms >= VP_ITEMS_MAX || vp_item(w, &roms[nroms]) != 0;
                nroms += bad ? 0u : 1u;
            }
            if (bad || nroms == 0u) {
                seen = 0u;
                break;
            }
            seen |= 1u;
        } else if (strncmp(item, "control=", 8u) == 0 && (seen & 2u) == 0u && vp_item(item + 8, &ctl) == 0) {
            seen |= 2u;
        } else {
            seen = 0u;
            break;
        }
    }
    if (seen != 3u || nroms == 0u) {
        harness_error(c, "a vdc_poweron_policy_diff case needs until_mclk and expect = roms=NAME@FRAME,NAME@FRAME;control=NAME@FRAME (each name once)");
        return;
    }
    for (i = 0u; i < nroms && !c->harness; i++) {
        if (vp_one(c, &roms[i], 0, until, &window, &borders, &last) != 0) {
            return;
        }
    }
    if (!c->harness && vp_one(c, &ctl, 1, until, &cwindow, &borders, &clast) != 0) {
        return;
    }
    snprintf(got, gotcap, "roms=%u window=%llu last=%llu border=%llu", nroms, (unsigned long long)window, (unsigned long long)last, (unsigned long long)borders);
}

/* ---- kind vdc_random_regs (plan 04-13; D-175, T-04-44) ---- */

#define VR_REC_AW_MAX 682u /* the widest active window, in dots (D-161) */

struct vr_stat {
    unsigned programs;
    unsigned with_active;   /* programs that emitted at least one record with the active flag */
    unsigned with_burst;
    uint64_t records;
    uint64_t values;
};

/* the records of one stream: every record is whole, its border, indices and colour values are below 512, its active width is within the longest and its length holds its arrays; the stream walks to the byte count */
static int vr_check_stream(struct ctx *c, unsigned index, const uint8_t *bytes, uint64_t stored, uint64_t count, struct vr_stat *st)
{
    const uint8_t *rec;
    uint64_t off = 0u;
    uint64_t k;
    int active = 0;
    int burst = 0;
    char msg[240];

    for (k = 0u; k < count; k++) {
        uint32_t rb;
        uint32_t aw;
        uint32_t j;
        int ok;

        if (off + (uint64_t)VD_REC_MIN > stored) {
            snprintf(msg, sizeof msg, "program %u: record %llu starts past the stored bytes", index, (unsigned long long)k);
            check(c, 0, msg);
            return -1;
        }
        rec = bytes + off;
        rb = vd_u16(rec + VD_O_REC_BYTES);
        aw = vd_u16(rec + VP_O_ACTIVE_W);
        ok = rb >= VD_REC_MIN && rb <= VD_REC_MAX && off + (uint64_t)rb <= stored && aw <= VR_REC_AW_MAX && 64u + 4u * aw <= rb && vd_u16(rec + VP_O_BORDER) < 512u;
        for (j = 0u; ok && j < 2u * aw; j++) { /* the index array, then the colour array */
            ok = vd_u16(rec + 64u + 2u * j) < 512u;
            st->values++;
        }
        snprintf(msg, sizeof msg, "program %u: record %llu has a length, an active width, a border, an index or a colour value out of range (a value of 512 or more, or arrays that do not fit the record)", index, (unsigned long long)k);
        check(c, ok, msg);
        if (!ok) {
            return -1;
        }
        active = active || aw > 0u;
        burst = burst || (rec[31] & 0x02u) != 0u; /* the burst flag (bit 1 of flags, D-161) */
        off += (uint64_t)rb;
    }
    snprintf(msg, sizeof msg, "program %u: the records do not add up to the stored byte count", index);
    check(c, off == stored, msg);
    st->records += count;
    st->with_active += active ? 1u : 0u;
    st->with_burst += burst ? 1u : 0u;
    return 0;
}

void kind_vdc_random_regs(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    unsigned seen = 0u;
    unsigned programs = 0u;
    uint64_t seed0 = 0u;
    uint64_t until = 0u;
    uint64_t cap;
    unsigned char *img;
    unsigned pi;
    struct vr_stat st;
    uint8_t prev[DS_DIGEST_BYTES];

    memset(&st, 0, sizeof st);
    memset(prev, 0, sizeof prev);
    if (!k->has[K_EXPECT] || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until < 100000u || vd_stream_cap(until) == 0u || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a vdc_random_regs case needs until_mclk and expect = programs=N;seed=N");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        uint64_t v = 0u;

        if (strncmp(item, "programs=", 9u) == 0 && (seen & 1u) == 0u && parse_u64(item + 9, &v) == 0 && v >= 1u && v <= 256u) {
            programs = (unsigned)v;
            seen |= 1u;
        } else if (strncmp(item, "seed=", 5u) == 0 && (seen & 2u) == 0u && parse_u64(item + 5, &seed0) == 0) {
            seen |= 2u;
        } else {
            seen = 0u;
            break;
        }
    }
    if (seen != 3u) {
        harness_error(c, "a vdc_random_regs case needs until_mclk and expect = programs=N;seed=N");
        return;
    }
    cap = vd_stream_cap(until);
    img = (unsigned char *)malloc(RP_IMAGE_BYTES);
    if (img == NULL) {
        harness_error(c, "out of memory for the random program image");
        return;
    }
    for (pi = 0u; pi < programs && !c->harness; pi++) {
        struct booted b;
        const struct ds_instance *ci = NULL;
        struct ds_run_request rq;
        struct ds_run_result res;
        struct ds_vid_sink vsk;
        uint8_t *block;
        uint64_t sx = seed0 + (uint64_t)pi * UINT64_C(0xD6E8FEB86659FD93);
        uint64_t seed = vd_mix(&sx);
        const char *err;
        char msg[260];
        uint8_t dig[DS_DIGEST_BYTES];
        uint32_t reason;

        rp_build_vdc(img, seed);
        err = boot_random(img, "", &b, &ci);
        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        block = (uint8_t *)malloc((size_t)cap);
        if (block == NULL) {
            release_boot(&b);
            harness_error(c, "out of memory for the vid sink of the vdc_random_regs kind");
            break;
        }
        memset(&vsk, 0, sizeof vsk);
        vsk.bytes = block;
        vsk.capacity = (uint32_t)cap;
        memset(&rq, 0, sizeof rq);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.vid = &vsk;
        memset(&res, 0, sizeof res);
        reason = ds_run((struct ds_instance *)(void *)b.mem, &rq, &res);
        snprintf(msg, sizeof msg, "program %u: ds_run did not stop at the deadline (a stop reason other than until) or stopped more than 16384 master clocks past it (the longest step of these programs is a 64-byte block transfer)", pi);
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && res.machine_time >= until && res.machine_time - until < 16384u, msg);
        snprintf(msg, sizeof msg, "program %u: the digest of the instance was refused", pi);
        check(c, ds_digest(ci, dig) == 0u, msg);
        snprintf(msg, sizeof msg, "program %u: the digest of the instance equals that of the program before it, so the programs do not reach distinct machine states (a vacuous run)", pi);
        check(c, pi == 0u || memcmp(dig, prev, DS_DIGEST_BYTES) != 0, msg);
        memcpy(prev, dig, DS_DIGEST_BYTES);
        {
            uint32_t cnt = vd_u32(vsk.header);
            uint32_t lost = vd_u32(vsk.header + 4);
            uint64_t seq = vd_u64(vsk.header + 8);
            uint64_t nb = vd_u64(vsk.header + 16);

            snprintf(msg, sizeof msg, "program %u: the vid sink lost a record or its header disagrees with the closed form of the line count (lost %u, count %u, seq %llu, lines due %llu)", pi, lost, cnt, (unsigned long long)seq, (unsigned long long)vd_lines_due(res.machine_time));
            check(c, lost == 0u && (uint64_t)cnt == seq && seq == vd_lines_due(res.machine_time) && nb <= cap && vd_u64(vsk.header + 24) == 0u, msg);
            if (lost == 0u && nb <= cap) {
                (void)vr_check_stream(c, pi, block, nb, (uint64_t)cnt, &st);
            }
        }
        st.programs++;
        free(block);
        release_boot(&b);
    }
    free(img);
    snprintf(got, gotcap, "programs=%u records=%llu active=%u burst=%u", st.programs, (unsigned long long)st.records, st.with_active, st.with_burst);
}
