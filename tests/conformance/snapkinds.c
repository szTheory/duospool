/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * snapkinds.c - the snapshot and restore kinds (plan 03-30; CORE-05, D-121, D-122, Pitfall 20, T-03-83 to T-03-85, ER-7). Evidence tier E0 throughout: the core compared with itself (repeatability and consistency); a pass is not a support claim and no literal comes from a DuoSpool run.
 * The evidence is built WITHOUT the serializer's own region table, which is what restore is written against:
 *   - the stream layout of this file (sk_frames, the prefix and the version) is a hand-kept copy, so the snapshot is parsed and edited here with offsets that the core did not supply;
 *   - the instance is compared RAW, member by member over the tiling list of ds_core.h (DS_INSTANCE_TILING, a hand-written list whose sum is a static assert against sizeof(struct ds_instance)), ROM excluded, never through the digest or a region;
 *   - the target of a restore is a fresh instance booted with another seed and whose every restorable member is then poisoned (0xA5 and again 0x5A), so a field that restore forgets stays poison and a field that equals the poison in the source cannot equal both;
 *   - after a restore the target is run in lockstep with an uninterrupted reference run, one step at a time: machine time, pc, opcode and a fold of the instruction and device records of every step (the dev records carry the pad reads), over a bounded window, and, at the lead stop of each class, to the end of the run.
 * kinds:
 *   snapshot_hash      expect = equals-digest[;host-file]: the snapshot of a mid-run stop has the fixed size, the hand-kept frames, the digest as its SHA-256, excludes the ROM, and (host-file) is written beside the evidence text with its digest, for the host tool of conformance-run.
 *   snapshot_restore   expect = stops=CLASS,...[;window=N][;finish][;perturb] or boundaries[;window=N] (see snapshot_restore below), fixture authored:NAME, request, until_mclk, and the case's input and irq_stim keys.
 *   snapshot_refuse    expect = ROW=CODE|ROW=CODE|... : each hostile edit of a snapshot (or API misuse) is refused with exactly CODE (a name of snapshot_refusals.def, without the prefix) and the target instance is byte-identical afterwards, or accept (the edit is a legal state and re-serialises identically).
 *   snapshot_refusal_coverage (deferred): every member of snapshot_refusals.def was observed by a refuse row.
 * Test code only, never installed.
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

/* ---- the hand-kept stream layout (independent of serial.c): the prefix and its NUL, the version, then 21 frames of tag, u32 length and payload ---- */

#define SK_PREFIX_BYTES 15u
#define SK_VERSION 8u
#define SK_FRAMES 21u
#define SK_EXPECTED_SIZE 92255u /* 15 + 4 + 21 * 8 + 92068 payload bytes: the 24890 of serial version 7 and the 67178 of the seven Phase 4 regions (3 + 1024 + 45 + 33 + 25 + 65536 + 512), derived by hand (plan 04-03) */

static const struct {
    const char *tag;
    uint32_t len;
} sk_frames[SK_FRAMES] = {
    {"PROF", 32u}, {"CONT", 40u}, {"POWR", 16u}, {"TIME", 8u}, {"RAM8", 8192u}, {"MPRS", 8u}, {"CPU0", 8u},
    {"BUS0", 3u},  {"TIMR", 11u}, {"IRQC", 3u},  {"IRQS", 160u}, {"BLKT", 11u}, {"IOPT", 6u}, {"SCHD", 16392u},
    {"VCE0", 3u},  {"PALT", 1024u}, {"VDC0", 45u}, {"BEAM", 33u},  {"VDMA", 25u}, {"VRAM", 65536u}, {"SATB", 512u},
};
enum { RG_PROF, RG_CONT, RG_POWR, RG_TIME, RG_RAM8, RG_MPRS, RG_CPU0, RG_BUS0, RG_TIMR, RG_IRQC, RG_IRQS, RG_BLKT, RG_IOPT, RG_SCHD, RG_VCE0, RG_PALT, RG_VDC0, RG_BEAM, RG_VDMA, RG_VRAM, RG_SATB };

/* the offset of the frame (the tag) of region r, and of its payload */
static size_t sk_frame_off(unsigned r)
{
    size_t off = (size_t)SK_PREFIX_BYTES + 4u;
    unsigned j;

    for (j = 0u; j < r; j++) {
        off += 8u + (size_t)sk_frames[j].len;
    }
    return off;
}

static size_t sk_payload_off(unsigned r)
{
    return sk_frame_off(r) + 8u;
}

static size_t sk_stream_size(void)
{
    return sk_frame_off(SK_FRAMES - 1u) + 8u + (size_t)sk_frames[SK_FRAMES - 1u].len;
}

/* ---- the tiling list: every member of the instance, by offset and size ---- */

struct sk_tile {
    const char *name;
    size_t off;
    size_t size;
};
#define SK_TILE(m) {#m, offsetof(struct ds_instance, m), sizeof(((struct ds_instance *)0)->m)},
static const struct sk_tile sk_tiles[] = {DS_INSTANCE_TILING(SK_TILE)};
#define SK_TILE_N (sizeof sk_tiles / sizeof sk_tiles[0])

/* the members that a restore does not write because they are not machine state of the stream: the identity of the instance (profile, content id, size class) is checked by pass 1, mapper, byte order and header are derived at boot, the ROM is excluded and the tail is padding */
static int sk_is_identity(const char *name)
{
    static const char *const ids[] = {"profile", "content_id", "mapper", "size_class", "byte_order", "header", "rom", "tail_pad", "vid_pad"};
    size_t i;

    for (i = 0u; i < sizeof ids / sizeof ids[0]; i++) {
        if (strcmp(ids[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* the members follow one another without a gap and fill the struct: the property that the static assert of ds_core.h proves by sum, here by offset */
static int sk_tiling_ok(void)
{
    size_t at = 0u;
    size_t i;

    for (i = 0u; i < SK_TILE_N; i++) {
        if (sk_tiles[i].off != at) {
            return 0;
        }
        at += sk_tiles[i].size;
    }
    return at == sizeof(struct ds_instance);
}

void snap_poison(struct ds_instance *t, uint8_t byte)
{
    size_t i;

    for (i = 0u; i < SK_TILE_N; i++) {
        if (!sk_is_identity(sk_tiles[i].name)) {
            memset((unsigned char *)t + sk_tiles[i].off, byte, sk_tiles[i].size);
        }
    }
}

/* the name of the first member whose bytes differ between two instances, ROM excluded; NULL when they are equal member by member */
const char *snap_first_diff(const struct ds_instance *a, const struct ds_instance *b)
{
    size_t i;

    for (i = 0u; i < SK_TILE_N; i++) {
        if (strcmp(sk_tiles[i].name, "rom") == 0) {
            continue;
        }
        if (memcmp((const unsigned char *)a + sk_tiles[i].off, (const unsigned char *)b + sk_tiles[i].off, sk_tiles[i].size) != 0) {
            return sk_tiles[i].name;
        }
    }
    return NULL;
}

/* ---- booting ---- */

/* the request of the target: the seed replaced (7, or 8 when the source's is 7) and the model.NAME items dropped, so the target starts with a different power-on state and the defaults of the model parameters: the restore must carry both. 0 on success */
static int sk_twin_request(const char *req, char *out, size_t cap)
{
    char work[PK_LIST_MAX];
    char *rest = work;
    char *item;
    size_t at = 0u;

    if (pk_copy_text(req, work, sizeof work) != 0) {
        return -1;
    }
    out[0] = '\0';
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        char one[64];
        int n;

        if (strncmp(item, "model.", 6u) == 0) {
            continue;
        }
        if (strncmp(item, "seed=", 5u) == 0) {
            uint64_t v = 0u;

            if (parse_u64(item + 5, &v) != 0) {
                return -1;
            }
            n = snprintf(one, sizeof one, "seed=%d", v == 7u ? 8 : 7);
        } else {
            n = snprintf(one, sizeof one, "%s", item);
        }
        if (n < 0 || (size_t)n >= sizeof one || at + (size_t)n + 2u > cap) {
            return -1;
        }
        at += (size_t)snprintf(out + at, cap - at, "%s%s", at != 0u ? ";" : "", one);
    }
    return 0;
}

/* boot the built image over the request; with apply the case's irq_stim list and input script are declared too (before the first step). NULL on a harness fault (reported) */
static struct ds_instance *sk_boot(struct ctx *c, const struct kase *k, const char *rom, const char *request, int apply, struct booted *b)
{
    const struct ds_instance *ci = NULL;
    struct ds_instance *w;
    uint32_t refusal = 0u;
    const char *err = boot_built_rom(rom, request, b, &ci);

    if (err != NULL) {
        harness_error(c, err);
        return NULL;
    }
    w = (struct ds_instance *)(void *)b->mem;
    if (apply) {
        err = apply_irq_stim(w, k, &refusal);
        if (err != NULL || refusal != 0u) {
            release_boot(b);
            harness_error(c, err != NULL ? err : "ds_irq_stim_set refused the irq_stim list of the case");
            return NULL;
        }
        err = apply_input(w, k, &refusal);
        if (err != NULL || refusal != 0u) {
            release_boot(b);
            harness_error(c, err != NULL ? err : "ds_input_push refused the input script of the case");
            return NULL;
        }
    }
    return w;
}

static void sk_hex(const uint8_t *d, size_t n, char *out)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0u; i < n; i++) {
        out[2u * i] = digits[(d[i] >> 4) & 0x0Fu];
        out[2u * i + 1u] = digits[d[i] & 0x0Fu];
    }
    out[2u * n] = '\0';
}

static void sk_put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void sk_put64(uint8_t *p, uint64_t v)
{
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        p[i] = (uint8_t)((v >> (8u * i)) & UINT64_C(0xFF));
    }
}

static uint64_t sk_get64(const uint8_t *p)
{
    uint64_t v = UINT64_C(0);
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

/* ---- snapshot_hash ---- */

/*
 * snapshot_hash: the snapshot of a mid-run stop of the fixture. Checked here: the size is the hand-kept 92255 bytes and equals ds_snapshot_size(); the call writes exactly that many bytes and not one more (a canary after the buffer); the prefix, the version, every tag and every length are the hand-kept ones;
 * the TIME, CPU0 and RAM8 payloads equal the instance's own members read raw; the SHA-256 of the snapshot equals ds_digest of the instance; the snapshot has the same bytes after a byte of the ROM is flipped (the ROM is not in it); and the guest has not finished at the stop (it is mid-run).
 * host-file: the snapshot bytes are written to ID.snapshot beside the evidence text, and the evidence text gets the lines digest=HEX (the core's digest), snapshot_bytes=N and snapshot_file=ID.snapshot. scripts/conformance-run then hashes the file with sha256sum or shasum -a 256 and fails the stage with SNAPSHOT_HOST_HASH_MISMATCH when that differs from the digest line:
 * the host tool shares no code, language or author with the core (D-121). The snapshot of an authored fixture only is written (D-120: the state of a commercial image is not for the evidence directory).
 */
void kind_snapshot_hash(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct ds_instance *w;
    char rom[48];
    uint64_t until = 0u;
    int host_file = 0;
    int have_equals = 0;
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    struct ds_run_request rq;
    struct ds_run_result res;
    uint8_t *buf;
    uint8_t *aux;
    uint8_t d1[DS_DIGEST_BYTES];
    uint8_t d2[DS_DIGEST_BYTES];
    size_t size;
    size_t written = 0u;
    size_t r;
    unsigned j;
    uint32_t rc;
    char hex[2u * DS_DIGEST_BYTES + 1u];
    char detail[256];

    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a snapshot_hash case needs fixture = authored:NAME, until_mclk and expect = equals-digest[;host-file]");
        return;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strcmp(item, "equals-digest") == 0 && !have_equals) {
            have_equals = 1;
        } else if (strcmp(item, "host-file") == 0 && !host_file) {
            host_file = 1;
        } else {
            harness_error(c, "the expect of a snapshot_hash case is equals-digest[;host-file], each once");
            return;
        }
    }
    if (!have_equals) {
        harness_error(c, "the expect of a snapshot_hash case needs equals-digest");
        return;
    }
    check(c, sk_tiling_ok(), "the tiling list of ds_core.h does not tile struct ds_instance member by member");
    check(c, sk_stream_size() == (size_t)SK_EXPECTED_SIZE, "the hand-kept frames of this file do not add up to the 92255 bytes of the stream");
    w = sk_boot(c, k, rom, k->val[K_REQUEST], 1, &b);
    if (w == NULL) {
        return;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rc = ds_run(w, &rq, &res);
    note_run(c, w->machine_time, rc);
    check(c, rc == (uint32_t)DS_RUN_REASON_UNTIL && res.steps > 4u, "the run to the snapshot stop took four steps or fewer");
    check(c, !dsv1_done(w, NULL), "the guest is already in its done state at the snapshot stop: the stop is not mid-run");
    size = ds_snapshot_size();
    check(c, size == (size_t)SK_EXPECTED_SIZE && size == sk_stream_size(), "ds_snapshot_size() is not the hand-kept size of the stream");
    buf = (uint8_t *)malloc(size + 16u);
    aux = (uint8_t *)malloc(size + 16u);
    if (buf == NULL || aux == NULL) {
        free(buf);
        free(aux);
        release_boot(&b);
        harness_error(c, "out of memory for the snapshot buffers");
        return;
    }
    memset(buf, 0xC3, size + 16u);
    rc = ds_snapshot(w, buf, size + 16u, &written);
    check(c, rc == (uint32_t)DS_SNAPREF_NONE && written == size, "ds_snapshot did not write exactly ds_snapshot_size() bytes");
    for (j = 0u; j < 16u; j++) {
        check(c, buf[size + j] == 0xC3u, "ds_snapshot wrote past ds_snapshot_size() bytes");
    }
    /* the hand-kept frames */
    check(c, memcmp(buf, "duospool-state", 15u) == 0, "the snapshot does not start with the prefix and its NUL");
    check(c, buf[15] == (uint8_t)SK_VERSION && buf[16] == 0u && buf[17] == 0u && buf[18] == 0u, "the serializer version of the snapshot is not the hand-kept version 8 (a layout change needs this file's table and version updated)");
    for (r = 0u; r < SK_FRAMES; r++) {
        const uint8_t *f = buf + sk_frame_off((unsigned)r);
        uint32_t len = (uint32_t)f[4] | ((uint32_t)f[5] << 8) | ((uint32_t)f[6] << 16) | ((uint32_t)f[7] << 24);

        check(c, memcmp(f, sk_frames[r].tag, 4u) == 0 && len == sk_frames[r].len, "a frame of the snapshot has a tag or a length other than the hand-kept one");
    }
    /* the payloads that name instance members read raw */
    check(c, sk_get64(buf + sk_payload_off(RG_TIME)) == w->machine_time, "the TIME payload is not the machine time of the instance");
    {
        const uint8_t *p = buf + sk_payload_off(RG_CPU0);

        check(c, p[0] == w->a && p[1] == w->x && p[2] == w->y && p[3] == w->s && p[4] == w->p && p[5] == (uint8_t)(w->pc & 0xFFu) && p[6] == (uint8_t)(w->pc >> 8) && p[7] == w->speed, "the CPU0 payload is not a x y s p pc speed of the instance");
        check(c, memcmp(buf + sk_payload_off(RG_RAM8), w->ram8k, DS_RAM8K_BYTES) == 0, "the RAM8 payload is not the work RAM of the instance");
    }
    /* the digest is the SHA-256 of the snapshot */
    ds_sha256(buf, size, d1);
    check(c, ds_digest(w, d2) == (uint32_t)DS_OK && memcmp(d1, d2, DS_DIGEST_BYTES) == 0, "the SHA-256 of the snapshot is not the digest of the instance");
    /* the ROM is not in the snapshot */
    w->rom[100] ^= 0xFFu;
    rc = ds_snapshot(w, aux, size, &written);
    check(c, rc == (uint32_t)DS_SNAPREF_NONE && written == size && memcmp(buf, aux, size) == 0, "a changed ROM byte changed the snapshot: the ROM must be excluded");
    w->rom[100] ^= 0xFFu;
    sk_hex(d1, DS_DIGEST_BYTES, hex);
    if (host_file) {
        char path[256];
        FILE *fp;
        int n;
        int ok = 0;

        if (run_evidence_dir != NULL) {
            n = snprintf(path, sizeof path, "%s/%s.snapshot", run_evidence_dir, k->stem);
            if (n > 0 && (size_t)n < sizeof path) {
                fp = fopen(path, "wb");
                if (fp != NULL) {
                    ok = fwrite(buf, 1u, size, fp) == size;
                    ok = fclose(fp) == 0 && ok;
                }
            }
        }
        if (!ok) {
            harness_error(c, "cannot write the snapshot file into the evidence directory");
        }
        snprintf(detail, sizeof detail, "digest=%s\nsnapshot_bytes=%u\nsnapshot_file=%s.snapshot\nhost_tool=sha256sum or shasum -a 256 (scripts/conformance-run)\n", hex, (unsigned)size, k->stem);
        set_evidence_detail(detail);
    }
    snprintf(got, gotcap, "bytes=%u %.16s", (unsigned)size, hex);
    free(buf);
    free(aux);
    release_boot(&b);
}

/* ---- the step observer ---- */

#define SK_INS_CAP 4u
#define SK_DEV_CAP 32u
#define SK_FNV_BASIS UINT64_C(14695981039346656037)
#define SK_FNV_PRIME UINT64_C(1099511628211)
#define SK_PAD_LO 0x1FF000u
#define SK_PAD_HI 0x1FF3FFu

struct sk_sinks {
    struct ds_insn_sink ins;
    struct ds_dev_sink dev;
};

static int sk_sinks_make(struct sk_sinks *s)
{
    memset(s, 0, sizeof *s);
    s->ins.records = (uint8_t *)malloc((size_t)SK_INS_CAP * DS_INSN_RECORD_BYTES);
    s->dev.records = (uint8_t *)malloc((size_t)SK_DEV_CAP * DS_DEV_RECORD_BYTES);
    s->ins.capacity = SK_INS_CAP;
    s->dev.capacity = SK_DEV_CAP;
    if (s->ins.records == NULL || s->dev.records == NULL) {
        free(s->ins.records);
        free(s->dev.records);
        return -1;
    }
    return 0;
}

static void sk_sinks_free(struct sk_sinks *s)
{
    free(s->ins.records);
    free(s->dev.records);
}

/* what one step showed: the machine time and pc after it, the opcode it executed (0x100 for a step that is no instruction, a block byte or the like), a fold of every instruction and device record it produced, and whether it touched the pad window with a write or a read */
struct sk_obs {
    uint64_t time;
    uint64_t fold;
    uint16_t pc;
    uint16_t opcode;
    uint8_t padw;
    uint8_t padr;
};

static uint32_t sk_get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void sk_fold(uint64_t *f, uint8_t b)
{
    *f = (*f ^ (uint64_t)b) * SK_FNV_PRIME;
}

/* one step of the machine through ds_run (a deadline one master clock ahead takes exactly one step) with both sinks attached; 0 on success, -1 when the call did not take exactly one step */
static int sk_step(struct ds_instance *w, struct sk_sinks *s, struct sk_obs *o)
{
    struct ds_run_request rq;
    struct ds_run_result res;
    uint32_t ni;
    uint32_t nd;
    uint32_t i;
    size_t j;

    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = w->machine_time + 1u;
    rq.insn = &s->ins;
    rq.dev = &s->dev;
    if (ds_run(w, &rq, &res) != (uint32_t)DS_RUN_REASON_UNTIL || res.steps != 1u) {
        return -1;
    }
    ni = sk_get32(s->ins.header);
    nd = sk_get32(s->dev.header);
    if (ni > SK_INS_CAP || nd > SK_DEV_CAP) {
        return -1;
    }
    o->time = w->machine_time;
    o->pc = w->pc;
    o->opcode = ni >= 1u ? (uint16_t)s->ins.records[10] : 0x100u;
    o->padw = 0u;
    o->padr = 0u;
    o->fold = SK_FNV_BASIS;
    sk_fold(&o->fold, (uint8_t)ni);
    sk_fold(&o->fold, (uint8_t)nd);
    sk_fold(&o->fold, (uint8_t)sk_get32(s->ins.header + 4));
    sk_fold(&o->fold, (uint8_t)sk_get32(s->dev.header + 4));
    for (j = 0u; j < (size_t)ni * DS_INSN_RECORD_BYTES; j++) {
        sk_fold(&o->fold, s->ins.records[j]);
    }
    for (i = 0u; i < nd; i++) {
        const uint8_t *r = s->dev.records + (size_t)i * DS_DEV_RECORD_BYTES;
        uint32_t phys = sk_get32(r + 8);

        for (j = 0u; j < DS_DEV_RECORD_BYTES; j++) {
            sk_fold(&o->fold, r[j]);
        }
        if (phys >= SK_PAD_LO && phys <= SK_PAD_HI) {
            if (r[13] == (uint8_t)DS_DEV_WRITE) {
                o->padw = 1u;
            } else if (r[13] == (uint8_t)DS_DEV_READ) {
                o->padr = 1u;
            }
        }
    }
    return 0;
}

/* one step of the source run without sinks (a deadline one master clock ahead): the machine time and pc after it are returned for the comparison with the reference trace; 0 on success */
static int sk_step_plain(struct ds_instance *w, uint64_t *time, uint16_t *pc)
{
    struct ds_run_request rq;
    struct ds_run_result res;

    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = w->machine_time + 1u;
    if (ds_run(w, &rq, &res) != (uint32_t)DS_RUN_REASON_UNTIL || res.steps != 1u) {
        return -1;
    }
    *time = w->machine_time;
    *pc = w->pc;
    return 0;
}

/* ---- the reference trace ---- */

#define SK_F_T 1u      /* the T flag is set: right after SET, before the next instruction clears it */
#define SK_F_BLK 4u    /* a block transfer is in progress */
#define SK_F_BLK0 8u   /* ... with no byte left (the epilogue is next) */
#define SK_F_SPEED1 16u
#define SK_F_PADW 32u  /* the step that led here wrote the pad window ($1000) */
#define SK_F_PADR 64u  /* ... or read it */
#define SK_P_T 0x20u
#define SK_OP_SET 0xF4u
#define SK_OP_CLI 0x58u
#define SK_OP_SEI 0x78u
#define SK_OP_PLP 0x28u
#define SK_OP_RTI 0x40u
#define SK_EXT 256u    /* steps recorded past the deadline, for the window of the last stops */
#define SK_ROWS_LIMIT 4000000u

struct sk_row {
    uint64_t time;
    uint64_t fold;
    uint16_t pc;
    uint16_t opcode;
    uint8_t flags;
};

struct sk_trace {
    struct sk_row *row;
    size_t n;
    size_t cap;
    size_t steps_end; /* the boundary at the deadline: row i is the state after i steps */
};

static int sk_push(struct sk_trace *tr, const struct ds_instance *w, const struct sk_obs *o)
{
    struct sk_row *r;
    unsigned flags = 0u;

    if (tr->n >= tr->cap) {
        size_t cap = tr->cap != 0u ? tr->cap * 2u : 4096u;
        struct sk_row *bigger;

        if (cap > SK_ROWS_LIMIT) {
            return -1;
        }
        bigger = (struct sk_row *)realloc(tr->row, cap * sizeof *bigger);
        if (bigger == NULL) {
            return -1;
        }
        tr->row = bigger;
        tr->cap = cap;
    }
    r = &tr->row[tr->n];
    if (((unsigned)w->p & SK_P_T) != 0u) {
        flags |= SK_F_T;
    }
    if (w->blk_active != 0u) {
        flags |= SK_F_BLK;
        if (w->blk_remaining == 0u) {
            flags |= SK_F_BLK0;
        }
    }
    if (w->speed != 0u) {
        flags |= SK_F_SPEED1;
    }
    if (o != NULL) {
        flags |= o->padw != 0u ? SK_F_PADW : 0u;
        flags |= o->padr != 0u ? SK_F_PADR : 0u;
    }
    r->time = w->machine_time;
    r->pc = w->pc;
    r->opcode = o != NULL ? o->opcode : 0u;
    r->fold = o != NULL ? o->fold : 0u;
    r->flags = (uint8_t)flags;
    tr->n++;
    return 0;
}

/* ---- the case items of snapshot_restore ---- */

enum { CL_FIRST = 1u, CL_LAST = 2u, CL_KTH = 4u, CL_BLOCK = 8u, CL_SET = 16u, CL_CLI = 32u, CL_SPEED = 64u, CL_PAD = 128u, CL_PLP = 256u, CL_RTI = 512u, CL_SEI = 1024u };
#define SK_STOP_MAX 1024u
#define SK_WINDOW_DEFAULT 32u
#define SK_WINDOW_MAX 256u

struct sk_items {
    unsigned cls;
    unsigned kth;
    unsigned window;
    int finish;
    int perturb;
    int all; /* boundaries: every instruction boundary of the run */
};

static int sk_parse_items(const char *text, struct sk_items *it)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have_window = 0;

    memset(it, 0, sizeof *it);
    it->window = SK_WINDOW_DEFAULT;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        uint64_t v = 0u;

        if (strncmp(item, "stops=", 6u) == 0 && it->cls == 0u && !it->all) {
            char *r2 = item + 6;
            char *w;

            while ((w = pk_next_item(&r2, ',')) != NULL) {
                unsigned bit = strcmp(w, "first") == 0 ? CL_FIRST : strcmp(w, "last") == 0 ? CL_LAST : strcmp(w, "mid-block") == 0 ? CL_BLOCK : strcmp(w, "after-set") == 0 ? CL_SET : strcmp(w, "after-cli") == 0 ? CL_CLI : strcmp(w, "after-plp") == 0 ? CL_PLP : strcmp(w, "after-rti") == 0 ? CL_RTI : strcmp(w, "after-sei") == 0 ? CL_SEI : strcmp(w, "speed") == 0 ? CL_SPEED : strcmp(w, "pad-write") == 0 ? CL_PAD : 0u;

                if (strncmp(w, "kth:", 4u) == 0 && parse_u64(w + 4, &v) == 0 && v >= 1u && v <= 1000000u && (it->cls & CL_KTH) == 0u) {
                    it->kth = (unsigned)v;
                    bit = CL_KTH;
                }
                if (bit == 0u || (it->cls & bit) != 0u) {
                    return -1;
                }
                it->cls |= bit;
            }
        } else if (strcmp(item, "boundaries") == 0 && it->cls == 0u && !it->all) {
            it->all = 1;
        } else if (strncmp(item, "window=", 7u) == 0 && !have_window && parse_u64(item + 7, &v) == 0 && v >= 1u && v <= SK_WINDOW_MAX) {
            it->window = (unsigned)v;
            have_window = 1;
        } else if (strcmp(item, "finish") == 0 && !it->finish) {
            it->finish = 1;
        } else if (strcmp(item, "perturb") == 0 && !it->perturb) {
            it->perturb = 1;
        } else {
            return -1;
        }
    }
    return it->cls != 0u || it->all ? 0 : -1;
}

struct sk_stop {
    size_t idx;
    unsigned cls;
    int lead; /* the first stop selected for some class: it also gets the perturbation and the finish run */
};

struct sk_plan {
    struct sk_stop s[SK_STOP_MAX];
    unsigned n;
    unsigned lead_seen; /* classes that already have a lead stop */
    int overflow;
};

static void sk_add(struct sk_plan *p, size_t idx, unsigned cls)
{
    unsigned i;
    int lead = (p->lead_seen & cls) == 0u;

    p->lead_seen |= cls;
    for (i = 0u; i < p->n; i++) {
        if (p->s[i].idx == idx) {
            p->s[i].cls |= cls;
            p->s[i].lead = p->s[i].lead || lead;
            return;
        }
    }
    if (p->n >= SK_STOP_MAX) {
        p->overflow = 1;
        return;
    }
    p->s[p->n].idx = idx;
    p->s[p->n].cls = cls;
    p->s[p->n].lead = lead;
    p->n++;
}

static int sk_stop_cmp(const void *a, const void *b)
{
    size_t x = ((const struct sk_stop *)a)->idx;
    size_t y = ((const struct sk_stop *)b)->idx;

    return x < y ? -1 : x > y ? 1 : 0;
}

/* the boundaries 0 to steps_end whose flag is set (want 1) or clear (want 0), at most cap of them in order; returns how many (all of them are counted, only cap are stored) */
static size_t sk_pick(const struct sk_trace *tr, unsigned flag, int want, size_t *out, size_t cap)
{
    size_t n = 0u;
    size_t i;

    for (i = 0u; i <= tr->steps_end; i++) {
        if (((tr->row[i].flags & flag) != 0u) == (want != 0)) {
            if (n < cap) {
                out[n] = i;
            }
            n++;
        }
    }
    return n;
}

#define SK_LIST_CAP 262144u

/* the boundaries 1 to steps_end reached by a step that executed opcode op, at most cap of them in order; returns how many */
static size_t sk_pick_op(const struct sk_trace *tr, unsigned op, size_t *out, size_t cap)
{
    size_t n = 0u;
    size_t i;

    for (i = 1u; i <= tr->steps_end; i++) {
        if ((unsigned)tr->row[i].opcode == op) {
            if (n < cap) {
                out[n] = i;
            }
            n++;
        }
    }
    return n;
}

/* add up to m boundaries spread evenly over the list of n (n at least 1) */
static void sk_spread(struct sk_plan *p, const size_t *list, size_t n, size_t m, unsigned cls)
{
    size_t j;

    if (m > n) {
        m = n;
    }
    for (j = 0u; j < m; j++) {
        sk_add(p, list[j * n / m], cls);
    }
}

/* build the stop list from the classes of the case; a class that selects no boundary is a failure of the case (it would make the evidence vacuous) */
static void sk_plan_make(struct ctx *c, const struct sk_trace *tr, const struct sk_items *it, struct sk_plan *p)
{
    size_t *list = (size_t *)malloc(SK_LIST_CAP * sizeof *list);
    size_t n;
    size_t i;
    unsigned sp;

    memset(p, 0, sizeof *p);
    if (list == NULL) {
        harness_error(c, "out of memory for the stop list");
        return;
    }
    if ((it->cls & CL_FIRST) != 0u) {
        sk_add(p, 0u, CL_FIRST);
    }
    if ((it->cls & CL_LAST) != 0u) {
        sk_add(p, tr->steps_end, CL_LAST);
    }
    if ((it->cls & CL_KTH) != 0u) {
        unsigned cnt = 0u;

        for (i = it->kth; i < tr->steps_end && cnt < 256u; i += it->kth) {
            sk_add(p, i, CL_KTH);
            cnt++;
        }
        check(c, cnt > 0u, "the kth class selected no boundary: k is not below the number of steps");
    }
    if ((it->cls & CL_BLOCK) != 0u) {
        n = sk_pick(tr, SK_F_BLK, 1, list, SK_LIST_CAP);
        check(c, n > 0u, "the mid-block class selected no boundary: the run has no block transfer");
        if (n > 0u && n <= SK_LIST_CAP) {
            sk_add(p, list[0], CL_BLOCK);          /* just after the prologue: all bytes still to come */
            sk_add(p, list[n / 2u], CL_BLOCK);     /* in the middle of the transfer */
            sk_add(p, list[n - 1u], CL_BLOCK);     /* the last boundary of the last active stretch */
        } else if (n > SK_LIST_CAP) {
            check(c, 0, "the run has more block transfer boundaries than the stop list holds");
        }
        n = sk_pick(tr, SK_F_BLK0, 1, list, SK_LIST_CAP);
        check(c, n > 0u, "no boundary of the run has an active block transfer with no byte left (the epilogue next)");
        if (n > 0u) {
            sk_add(p, list[0], CL_BLOCK);
        }
    }
    if ((it->cls & CL_SET) != 0u) {
        n = sk_pick_op(tr, SK_OP_SET, list, SK_LIST_CAP);
        check(c, n > 0u && n <= SK_LIST_CAP, "the after-set class selected no boundary: the run never executes SET");
        if (n > 0u && n <= SK_LIST_CAP) {
            sk_spread(p, list, n, 6u, CL_SET);
            for (i = 0u; i < p->n; i++) {
                if ((p->s[i].cls & CL_SET) != 0u) {
                    check(c, (tr->row[p->s[i].idx].flags & SK_F_T) != 0u, "a boundary after SET does not have the T flag set: the stop is not between SET and the next instruction");
                }
            }
        }
    }
    for (sp = 0u; sp < 4u; sp++) { /* after CLI, PLP, RTI and SEI: the I flag of the restored instance is what the instruction left (and, under irq_i_sample, what the sample then sees) */
        static const struct {
            unsigned cls;
            unsigned op;
        } iops[4] = {{CL_CLI, SK_OP_CLI}, {CL_PLP, SK_OP_PLP}, {CL_RTI, SK_OP_RTI}, {CL_SEI, SK_OP_SEI}};

        if ((it->cls & iops[sp].cls) != 0u) {
            n = sk_pick_op(tr, iops[sp].op, list, SK_LIST_CAP);
            check(c, n > 0u && n <= SK_LIST_CAP, "an after-cli, after-plp, after-rti or after-sei class selected no boundary: the run never executes that instruction");
            if (n > 0u && n <= SK_LIST_CAP) {
                sk_spread(p, list, n, 3u, iops[sp].cls);
            }
        }
    }
    if ((it->cls & CL_SPEED) != 0u) {
        for (sp = 0u; sp < 2u; sp++) {
            n = sk_pick(tr, SK_F_SPEED1, (int)sp, list, SK_LIST_CAP);
            check(c, n > 0u, "the speed class selected no boundary at one of the two speeds");
            if (n > 0u) {
                sk_add(p, list[(n < SK_LIST_CAP ? n : SK_LIST_CAP) / 2u], CL_SPEED);
            }
        }
    }
    if ((it->cls & CL_PAD) != 0u) {
        n = sk_pick(tr, SK_F_PADW, 1, list, SK_LIST_CAP);
        check(c, n > 0u, "the pad-write class selected no boundary: no step wrote the $1000 window");
        if (n > 0u) {
            sk_spread(p, list, n < SK_LIST_CAP ? n : SK_LIST_CAP, 12u, CL_PAD);
        }
    }
    check(c, !p->overflow, "the stop list is longer than the plan holds");
    qsort(p->s, p->n, sizeof p->s[0], sk_stop_cmp);
    free(list);
}

/* ---- the per-region perturbation ---- */

/*
 * Every region of the digest is reached through the accessor of the table the digest walks, on the RESTORED instance: a byte of its live span is flipped, the digest must change, and the byte put back must put the digest back. Spans of at most 16 bytes are flipped byte by byte (so A, X, Y, S, the block transfer state, the timer and the I/O port bytes are each seen
 * changing the digest at a mid-run stop, which the boot state cannot show: there A, X, Y and S are all zero), the others at their first and last byte (PROF at its first byte only: the tail of the last value of the profile is padding that the hash does not take). Returns the number of flips made.
 */
static unsigned sk_perturb(struct ctx *c, struct ds_instance *t)
{
    uint8_t d0[DS_DIGEST_BYTES];
    uint8_t d1[DS_DIGEST_BYTES];
    unsigned flips = 0u;
    uint32_t r;

    if (ds_digest(t, d0) != (uint32_t)DS_OK) {
        check(c, 0, "the digest of the restored instance was refused");
        return 0u;
    }
    for (r = 0u; r < DS_REGION_COUNT; r++) {
        const char *tag = NULL;
        uint8_t *ptr = NULL;
        size_t len = 0u;
        size_t pos[16];
        size_t np = 0u;
        size_t q;

        if (ds_region_info(t, r, &tag, &ptr, &len) == 0u || len == 0u) {
            check(c, 0, "a region of the table has no live span");
            continue;
        }
        if (len <= 16u) {
            for (q = 0u; q < len; q++) {
                pos[np++] = q;
            }
        } else {
            pos[np++] = 0u;
            if (strcmp(tag, "PROF") != 0) {
                pos[np++] = len - 1u;
            }
        }
        for (q = 0u; q < np; q++) {
            ptr[pos[q]] ^= 0xFFu;
            check(c, ds_digest(t, d1) == (uint32_t)DS_OK && memcmp(d0, d1, DS_DIGEST_BYTES) != 0, "flipping a byte of a region of the restored instance did not change the digest");
            ptr[pos[q]] ^= 0xFFu;
            check(c, ds_digest(t, d1) == (uint32_t)DS_OK && memcmp(d0, d1, DS_DIGEST_BYTES) == 0, "putting the byte back did not put the digest back");
            flips++;
        }
    }
    return flips;
}

/* ---- snapshot_restore ---- */

struct sk_env {
    struct ctx *c;
    const struct sk_items *it;
    struct ds_instance *t;
    struct sk_sinks *sinks;
    const struct sk_trace *tr;
    const struct ds_instance *rend;
    uint64_t until;
    uint8_t *snap;
    uint8_t *snap2;
    size_t size;
    unsigned restores;
    unsigned distinct;
    unsigned perturbed;
    unsigned finished;
    unsigned long steps;
    unsigned padlooks;
};

/* the lockstep window after a restore at boundary i: the target is stepped win steps and each must show the machine time, pc, opcode and record fold of the uninterrupted reference. 0 when all agree */
static int sk_lockstep(struct sk_env *e, size_t i, size_t win)
{
    size_t j;

    for (j = 1u; j <= win; j++) {
        struct sk_obs o;
        const struct sk_row *ref = &e->tr->row[i + j];

        if (sk_step(e->t, e->sinks, &o) != 0) {
            return -1;
        }
        e->steps++;
        if (o.time != ref->time || o.pc != ref->pc || o.opcode != ref->opcode || o.fold != ref->fold) {
            return -1;
        }
    }
    return 0;
}

/* one stop: snapshot the source (which sits at boundary i), restore into the poisoned target (two poison bytes, or one alternating by boundary), compare raw, by digest and by re-serialising, then run the window and, at a lead stop, the rest of the run */
static void sk_at_stop(struct sk_env *e, struct ds_instance *s, size_t i, unsigned cls, int lead, int alt)
{
    struct ctx *c = e->c;
    size_t written = 0u;
    uint32_t rc;
    unsigned pass;
    unsigned npass = alt ? 1u : 2u;
    size_t win = e->it->window;
    int stop_ok = 1;

    rc = ds_snapshot(s, e->snap, e->size, &written);
    check(c, rc == (uint32_t)DS_SNAPREF_NONE && written == e->size, "ds_snapshot of the source did not write the whole snapshot");
    if (rc != (uint32_t)DS_SNAPREF_NONE) {
        return;
    }
    if (!alt) {
        uint8_t d1[DS_DIGEST_BYTES];
        uint8_t d2[DS_DIGEST_BYTES];

        ds_sha256(e->snap, e->size, d1);
        check(c, ds_digest(s, d2) == (uint32_t)DS_OK && memcmp(d1, d2, DS_DIGEST_BYTES) == 0, "the SHA-256 of the snapshot is not the digest of the source at this stop");
    }
    if (s->a != s->x && s->a != s->y && s->a != s->s && s->x != s->y && s->x != s->s && s->y != s->s) {
        e->distinct++;
    }
    for (pass = 0u; pass < npass; pass++) {
        uint8_t poison = alt ? ((i & 1u) != 0u ? 0x5Au : 0xA5u) : (pass == 0u ? 0xA5u : 0x5Au);
        const char *diff;
        uint8_t d1[DS_DIGEST_BYTES];
        uint8_t d2[DS_DIGEST_BYTES];

        snap_poison(e->t, poison);
        rc = ds_restore(e->t, e->snap, e->size);
        e->restores++;
        check(c, rc == (uint32_t)DS_SNAPREF_NONE, "ds_restore refused a snapshot that the source just wrote");
        if (rc != (uint32_t)DS_SNAPREF_NONE) {
            emit_err("snapshot_restore: boundary %lu: %s\n", (unsigned long)i, ds_snapshot_refusal_name(rc));
            return;
        }
        diff = snap_first_diff(e->t, s);
        check(c, diff == NULL, "the restored instance differs from the source in a member (raw compare, ROM excluded)");
        if (diff != NULL) {
            emit_err("snapshot_restore: boundary %lu: member %s differs after the restore (poison %02X)\n", (unsigned long)i, diff, (unsigned)poison);
            stop_ok = 0;
        }
        if (!alt) {
            check(c, ds_digest(e->t, d1) == (uint32_t)DS_OK && ds_digest(s, d2) == (uint32_t)DS_OK && memcmp(d1, d2, DS_DIGEST_BYTES) == 0, "the digest of the restored instance is not the digest of the source");
        }
    }
    if (!alt) {
        rc = ds_snapshot(e->t, e->snap2, e->size, &written);
        check(c, rc == (uint32_t)DS_SNAPREF_NONE && written == e->size && memcmp(e->snap, e->snap2, e->size) == 0, "the restored instance does not re-serialise to the snapshot it was restored from");
    }
    if (lead && e->it->perturb && !alt && i > 0u && e->perturbed == 0u) { /* once per case, at the first lead stop past the boot state (where A, X, Y and S differ and the block, timer and port state are live) */
        e->perturbed += sk_perturb(c, e->t);
    }
    if (!stop_ok) {
        return;
    }
    if ((cls & CL_PAD) != 0u) { /* the window runs through the first read of the pad window that follows the write: the read that needs the schedule, which only the snapshot gave the target */
        size_t j;
        size_t found = 0u;

        for (j = 1u; j <= SK_WINDOW_MAX && i + j < e->tr->n; j++) {
            if ((e->tr->row[i + j].flags & SK_F_PADR) != 0u) {
                found = j;
                break;
            }
        }
        check(c, found != 0u, "no read of the pad window follows the pad write of this stop within the window");
        if (found != 0u) {
            win = found + 1u;
            e->padlooks++;
        }
    }
    if (i + win >= e->tr->n) {
        win = e->tr->n - 1u - i;
    }
    if (sk_lockstep(e, i, win) != 0) {
        check(c, 0, "the restored instance left the uninterrupted run in lockstep (machine time, pc, opcode or the record fold of a step)");
        emit_err("snapshot_restore: boundary %lu: lockstep broke inside the window of %lu steps\n", (unsigned long)i, (unsigned long)win);
        return;
    }
    check(c, 1, "lockstep");
    if (lead && e->it->finish && i + win <= e->tr->steps_end) {
        struct ds_run_request rq;
        struct ds_run_result res;
        uint8_t d1[DS_DIGEST_BYTES];
        uint8_t d2[DS_DIGEST_BYTES];
        const char *diff;

        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = e->until;
        rc = ds_run(e->t, &rq, &res);
        check(c, rc == (uint32_t)DS_RUN_REASON_UNTIL, "the restored instance could not be run to the deadline");
        check(c, e->t->machine_time == e->rend->machine_time, "the restored instance ends at another machine time than the uninterrupted run");
        check(c, memcmp(e->t->ram8k + V_OFF, e->rend->ram8k + V_OFF, V_SIZE) == 0, "the verdict RAM of the restored instance differs from the uninterrupted run at the end");
        check(c, ds_digest(e->t, d1) == (uint32_t)DS_OK && ds_digest(e->rend, d2) == (uint32_t)DS_OK && memcmp(d1, d2, DS_DIGEST_BYTES) == 0, "the digest of the restored instance differs from the uninterrupted run at the end");
        diff = snap_first_diff(e->t, e->rend);
        check(c, diff == NULL, "the restored instance differs from the uninterrupted run at the end in a member (raw compare, ROM excluded)");
        if (diff != NULL) {
            emit_err("snapshot_restore: boundary %lu: member %s differs at the end of the run\n", (unsigned long)i, diff);
        }
        e->finished++;
    }
}

/*
 * snapshot_restore (D-121, D-122): the reference run of the fixture is made first, one step at a time, and its trace kept (the machine time, pc, opcode and record fold of every step, and the flags of every boundary); a copy of the machine at the deadline is kept too. Then a second source run goes boundary by boundary,
 * and at each selected stop it is snapshotted and the snapshot restored into the target (booted with the twin request, no schedule, no stimulus, the defaults of the model parameters, then poisoned). See sk_at_stop. The classes of stops (expect stops=LIST):
 *   first, last   the boot state and the state at the deadline;  kth:K  every K-th boundary (at most 256);
 *   mid-block     the first boundary of a block transfer, its middle, the last one and the first with no byte left (the epilogue next);  after-set  up to six boundaries right after a SET (the T flag is set there, which is asserted: between SET and the next instruction);
 *   after-cli, after-plp, after-rti, after-sei   up to three boundaries after each such instruction (a class whose instruction never runs fails the case): the I flag of the restored instance is the one the instruction left;  speed  the median boundary at each of the two speeds;
 *   pad-write     boundaries right after a step that wrote the $1000 window, with the window run through the next read of it: the input schedule and the pad latch are the restored state's.
 * boundaries    every instruction boundary of the run (one poison byte per boundary, alternating); window=N the lockstep steps after each restore (default 32); finish: at the lead stop of each class also run to the deadline and compare the end (time, verdict RAM, digest, raw members);
 * perturb: at the lead stop of each class flip one byte of every region of the restored instance (sk_perturb). A class that selects no boundary fails the case. At least one stop must have A, X, Y and S pairwise different, or a swap of those registers by restore could not show.
 */
void kind_snapshot_restore(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct sk_items it;
    struct sk_trace tr;
    struct sk_sinks sinks;
    struct sk_plan *plan;
    struct sk_env e;
    struct booted br;
    struct booted bs;
    struct booted bt;
    struct ds_instance *r = NULL;
    struct ds_instance *s = NULL;
    struct ds_instance *t = NULL;
    struct ds_instance *rend = NULL;
    char rom[48];
    char twin[PK_LIST_MAX];
    uint64_t until = 0u;
    size_t i;
    size_t stop_at = 0u;
    unsigned nstops = 0u;
    int sinks_ok = 0;
    struct sk_obs o;
    unsigned x;

    memset(&tr, 0, sizeof tr);
    memset(&br, 0, sizeof br);
    memset(&bs, 0, sizeof bs);
    memset(&bt, 0, sizeof bt);
    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || sk_parse_items(k->val[K_EXPECT], &it) != 0
        || sk_twin_request(k->val[K_REQUEST], twin, sizeof twin) != 0) {
        harness_error(c, "a snapshot_restore case needs fixture = authored:NAME, until_mclk and expect = stops=first,last,kth:K,mid-block,after-set,after-cli,after-plp,after-rti,after-sei,speed,pad-write (any of them, each once) or boundaries, then optionally window=N, finish, perturb");
        return;
    }
    check(c, sk_tiling_ok(), "the tiling list of ds_core.h does not tile struct ds_instance member by member");
    plan = (struct sk_plan *)malloc(sizeof *plan);
    if (plan == NULL || sk_sinks_make(&sinks) != 0) {
        free(plan);
        harness_error(c, "out of memory for the restore evidence");
        return;
    }
    sinks_ok = 1;
    memset(&e, 0, sizeof e);
    e.c = c;
    e.it = &it;
    e.sinks = &sinks;
    e.tr = &tr;
    e.until = until;
    e.size = ds_snapshot_size();
    e.snap = (uint8_t *)malloc(e.size);
    e.snap2 = (uint8_t *)malloc(e.size);
    rend = (struct ds_instance *)malloc(ds_instance_size());
    if (e.snap == NULL || e.snap2 == NULL || rend == NULL) {
        harness_error(c, "out of memory for the snapshot buffers");
        goto done;
    }
    e.rend = rend;

    /* pass A: the uninterrupted reference run, one step at a time, with the trace; the copy at the deadline; then the steps past it that the window of the last stops needs */
    r = sk_boot(c, k, rom, k->val[K_REQUEST], 1, &br);
    if (r == NULL) {
        goto done;
    }
    check(c, input_applied(k, r), "the reference instance does not hold the events of the declared input script");
    if (sk_push(&tr, r, NULL) != 0) {
        harness_error(c, "out of memory or too many steps for the reference trace");
        goto done;
    }
    while (r->machine_time < until) {
        if (sk_step(r, &sinks, &o) != 0 || sk_push(&tr, r, &o) != 0) {
            harness_error(c, "a reference step was refused, took more than one step, or the trace is too long");
            goto done;
        }
    }
    tr.steps_end = tr.n - 1u;
    memcpy(rend, r, ds_instance_size());
    check(c, tr.steps_end > 4u, "the reference run took four steps or fewer");
    check(c, dsv1_done(rend, NULL), "the guest is not in its done state at the deadline: the end of the run would not be the verdict");
    note_run(c, rend->machine_time, (uint32_t)DS_RUN_REASON_UNTIL);
    for (x = 0u; x < SK_EXT; x++) {
        if (sk_step(r, &sinks, &o) != 0 || sk_push(&tr, r, &o) != 0) {
            harness_error(c, "a reference step past the deadline was refused");
            goto done;
        }
    }
    release_boot(&br);
    r = NULL;

    /* the target: another seed, defaults of the model parameters, no schedule, no stimulus */
    t = sk_boot(c, k, rom, twin, 0, &bt);
    if (t == NULL) {
        goto done;
    }
    check(c, t->input_count == 0u && t->irq_stim_line[0] == 0u, "the target instance is not booted with an empty schedule and no stimulus");
    e.t = t;
    if (it.all) {
        plan->n = 0u;
    } else {
        sk_plan_make(c, &tr, &it, plan);
        if (c->harness) {
            goto done;
        }
    }

    /* pass B: the source, boundary by boundary */
    s = sk_boot(c, k, rom, k->val[K_REQUEST], 1, &bs);
    if (s == NULL) {
        goto done;
    }
    for (i = 0u; i <= tr.steps_end && !c->harness; i++) {
        if (it.all) {
            sk_at_stop(&e, s, i, 0u, 0, 1);
            nstops++;
        } else if (stop_at < plan->n && plan->s[stop_at].idx == i) {
            sk_at_stop(&e, s, i, plan->s[stop_at].cls, plan->s[stop_at].lead, 0);
            stop_at++;
            nstops++;
        }
        if (i < tr.steps_end) {
            uint64_t st_time = 0u;
            uint16_t st_pc = 0u;

            if (sk_step_plain(s, &st_time, &st_pc) != 0 || st_time != tr.row[i + 1u].time || st_pc != tr.row[i + 1u].pc) {
                check(c, 0, "the source run does not follow the reference run step for step: the machine is not deterministic, or the run with no sink differs from the run with sinks");
                break;
            }
        }
    }
    check(c, it.all || stop_at == plan->n, "a planned stop was not reached");
    check(c, e.distinct > 0u, "no restored stop had A, X, Y and S pairwise different: a swap of those registers by restore could not show");
    check(c, nstops > 0u && e.restores > 0u, "no snapshot was restored");
    if (it.perturb) {
        check(c, e.perturbed > 0u, "the perturbation was requested but flipped no byte");
    }
    if (it.finish) {
        check(c, e.finished > 0u, "finish was requested but no stop could be run to the deadline");
    }
    if ((it.cls & CL_PAD) != 0u) {
        check(c, e.padlooks > 0u, "no pad-write stop ran through a read of the pad window");
        check(c, input_applied(k, s) && s->input_count > 0u, "the pad case's source holds no input events: the schedule would not be carried by the snapshot");
    }
    snprintf(got, gotcap, "stops=%u restores=%u steps=%lu fin=%u win=%u", nstops, e.restores, (unsigned long)tr.steps_end, e.finished, (unsigned)it.window);
done:
    if (sinks_ok) {
        sk_sinks_free(&sinks);
    }
    release_boot(&br);
    release_boot(&bs);
    release_boot(&bt);
    free(rend);
    free(e.snap);
    free(e.snap2);
    free(tr.row);
    free(plan);
}

/* ---- snapshot_refuse ---- */

static unsigned char sk_observed[DS_SNAPREF_SLOTS];

enum { M_EDIT, M_SHORT, M_LONG, M_NULL, M_OVERLAP, M_SNAP_CAPACITY, M_SNAP_OVERLAP };

/* the edits: each takes the snapshot copy (exactly the size of the stream) and changes it the way the row names; offsets come from the hand-kept layout */
static void ed_none(uint8_t *b)
{
    (void)b;
}
static void ed_profile(uint8_t *b)
{
    b[sk_payload_off(RG_PROF)] ^= 0x01u;
}
static void ed_content(uint8_t *b)
{
    b[sk_payload_off(RG_CONT)] ^= 0x01u;
}
static void ed_prefix(uint8_t *b)
{
    b[0] ^= 0x01u;
}
static void ed_version(uint8_t *b)
{
    sk_put32(b + SK_PREFIX_BYTES, SK_VERSION - 1u);
}
static void ed_tags(uint8_t *b)
{
    uint8_t t[4];

    memcpy(t, b + sk_frame_off(RG_CONT), 4u);
    memcpy(b + sk_frame_off(RG_CONT), b + sk_frame_off(RG_POWR), 4u);
    memcpy(b + sk_frame_off(RG_POWR), t, 4u);
}
static void ed_region_length(uint8_t *b)
{
    sk_put32(b + sk_frame_off(RG_RAM8) + 4u, DS_RAM8K_BYTES - 1u);
}
static void ed_speed2(uint8_t *b)
{
    b[sk_payload_off(RG_CPU0) + 7u] = 2u;
}
static void ed_timer128(uint8_t *b)
{
    b[sk_payload_off(RG_TIMR) + 8u] = 128u;
}
static void ed_reload128(uint8_t *b)
{
    b[sk_payload_off(RG_TIMR) + 9u] = 128u;
}
static void ed_timer_start2(uint8_t *b)
{
    b[sk_payload_off(RG_TIMR) + 10u] = 2u;
}
/* the timer origin against the machine time of the same stream (WR-03): one after it is a machine no guest can reach, one equal to it is a boundary a reachable machine has. The rows read the TIME payload of the stream they edit, never the core. */
static void ed_timer_origin_ahead(uint8_t *b)
{
    sk_put64(b + sk_payload_off(RG_TIMR), sk_get64(b + sk_payload_off(RG_TIME)) + 1u);
}
static void ed_timer_origin_far(uint8_t *b)
{
    sk_put64(b + sk_payload_off(RG_TIMR), UINT64_C(1) << 62);
    b[sk_payload_off(RG_TIMR) + 10u] = 1u; /* enabled, so the timer is the stuck one of the review: it never counts and never borrows */
}
static void ed_timer_origin_at_time(uint8_t *b)
{
    sk_put64(b + sk_payload_off(RG_TIMR), sk_get64(b + sk_payload_off(RG_TIME)));
}
static void ed_blk_rem_inactive(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    sk_put32(p + 3u, 1u);
}
static void ed_blk_rem_65537(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    p[0] = 1u;
    p[1] = 0x73u;
    sk_put32(p + 3u, 65537u);
}
static void ed_blk_rem_65536(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    p[0] = 1u;
    p[1] = 0x73u;
    sk_put32(p + 3u, 65536u);
}
static void ed_blk_rem_zero_active(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    p[0] = 1u;
    p[1] = 0xC3u;
}
static void ed_blk_opcode(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    p[0] = 1u;
    p[1] = 0x74u;
    sk_put32(p + 3u, 4u);
}
static void ed_blk_toggle_tii(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    p[0] = 1u;
    p[1] = 0x73u;
    p[2] = 1u;
    sk_put32(p + 3u, 4u);
}
static void ed_blk_toggle_tia(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_BLKT);

    memset(p, 0, 11u);
    p[0] = 1u;
    p[1] = 0xE3u;
    p[2] = 1u;
    sk_put32(p + 3u, 4u);
}
static void ed_time_2_63(uint8_t *b)
{
    sk_put64(b + sk_payload_off(RG_TIME), UINT64_C(1) << 63);
}
static void ed_time_max(uint8_t *b)
{
    sk_put64(b + sk_payload_off(RG_TIME), UINT64_MAX);
}
static void ed_time_below(uint8_t *b)
{
    sk_put64(b + sk_payload_off(RG_TIME), (UINT64_C(1) << 63) - 1u);
    /* the fold window rule of plan 04-19 (D-166) refuses a beam whose next event is behind the machine time, so the beam follows the time: the next line event is the first one at or after it (ceiling of time / 1365), which the line-limit rule also allows */
    sk_put64(b + sk_payload_off(RG_BEAM), ((UINT64_C(1) << 63) - UINT64_C(1) + UINT64_C(1364)) / UINT64_C(1365));
}
static void ed_p_b_bit(uint8_t *b)
{
    b[sk_payload_off(RG_CPU0) + 4u] |= 0x10u;
}
static void ed_policy5(uint8_t *b)
{
    sk_put32(b + sk_payload_off(RG_POWR), 5u); /* policy 5 is the first unknown policy since seeded-prng-v3 took 4 (plan 04-18) */
}
static void ed_policy_other(uint8_t *b)
{
    sk_put32(b + sk_payload_off(RG_POWR), 2u); /* the request of the case binds seeded-prng-v2 (3) */
}
static void ed_seed_no_flag(uint8_t *b)
{
    sk_put32(b + sk_payload_off(RG_POWR) + 12u, 0u);
}
static void ed_tma2(uint8_t *b)
{
    b[sk_payload_off(RG_BUS0) + 2u] = 2u;
}
static void ed_sample2(uint8_t *b)
{
    b[sk_payload_off(RG_IRQC) + 2u] = 2u;
}
static void ed_disable8(uint8_t *b)
{
    b[sk_payload_off(RG_IRQC)] = 8u;
}
static void ed_tiq2(uint8_t *b)
{
    b[sk_payload_off(RG_IRQC) + 1u] = 2u;
}
static void ed_stim_order(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_IRQS);

    sk_put64(p, 100u);
    p[8] = 1u;
    p[9] = 1u;
    sk_put64(p + 10u, 50u);
    p[18] = 2u;
    p[19] = 0u;
}
static void ed_stim_gap(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_IRQS);

    memset(p, 0, 10u);
    sk_put64(p + 10u, 5u);
    p[18] = 1u;
    p[19] = 1u;
}
static void ed_stim_line3(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_IRQS);

    sk_put64(p, 5u);
    p[8] = 3u;
    p[9] = 0u;
}
static void ed_stim_valid(uint8_t *b)
{
    uint8_t *p = b + sk_payload_off(RG_IRQS);
    unsigned i;

    memset(p, 0, 160u);
    for (i = 0u; i < 3u; i++) {
        sk_put64(p + 10u * i, 1000u * (i + 1u));
        p[10u * i + 8u] = (uint8_t)(1u + (i & 1u));
        p[10u * i + 9u] = (uint8_t)(1u - (i & 1u));
    }
}
static void ed_schd_count(uint8_t *b)
{
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 1025u);
}
static void ed_schd_reserved(uint8_t *b)
{
    sk_put32(b + sk_payload_off(RG_SCHD) + 16388u, 1u);
}
/* one event in slot i of the schedule: time, port, mask */
static void sk_slot(uint8_t *b, unsigned i, uint64_t at, uint32_t port, uint32_t mask)
{
    uint8_t *p = b + sk_payload_off(RG_SCHD) + 16u * i;

    sk_put64(p, at);
    sk_put32(p + 8u, port);
    sk_put32(p + 12u, mask);
}
static void sk_schd_clear(uint8_t *b)
{
    memset(b + sk_payload_off(RG_SCHD), 0, 16392u);
}
static void ed_schd_port2(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 0u, 2u, 1u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 1u);
}
static void ed_schd_port0(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 0u, 0u, 1u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 1u);
}
static void ed_schd_bit8(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 0u, 1u, 0x100u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 1u);
}
static void ed_schd_opposite(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 0u, 1u, 0x50u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 1u);
}
static void ed_schd_order(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 10u, 1u, 1u);
    sk_slot(b, 1u, 5u, 1u, 2u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 2u);
}
static void ed_schd_equal_port(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 10u, 1u, 1u);
    sk_slot(b, 1u, 10u, 1u, 2u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 2u);
}
static void ed_schd_unused(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 1023u, 0u, 0u, 1u);
}
static void ed_schd_valid(uint8_t *b)
{
    sk_schd_clear(b);
    sk_slot(b, 0u, 100u, 1u, 0x11u);
    sk_put32(b + sk_payload_off(RG_SCHD) + 16384u, 1u);
}
static void ed_iopt_port2(uint8_t *b)
{
    b[sk_payload_off(RG_IOPT) + 2u] = 1u;
}
static void ed_iopt_port1(uint8_t *b)
{
    b[sk_payload_off(RG_IOPT) + 1u] = 0u; /* the profile names a pad on port 1 */
}
static void ed_iopt_kind2(uint8_t *b)
{
    b[sk_payload_off(RG_IOPT) + 1u] = 2u;
}
static void ed_latch(uint8_t *b)
{
    b[sk_payload_off(RG_IOPT)] = 0x00u;
}

/*
 * The edits of the seven Phase 4 regions (plan 04-18; D-166, F-1, F-34, T-04-08, T-04-09). The offsets are payload offsets in the stream order of the decoders, written by hand from the frame layout and not from the struct:
 *   VCE0 3 bytes: cr 0, cta 1 (u16).  PALT 1,024 bytes.  VDC0 45 bytes: ar 0, status 1, the 20 registers from 2 (register n at 2 + 2n), read buffer 42, write latch 44.
 *   BEAM 33 bytes: line_idx 0 (u64), stage 8, v_state 9, v_togo 10, display_k 12, rcr_counter 14, frame_lines 16, bg_y 18, mwr_active 20 (u16 each), burst 22, line_dirty 23, sprite_pend 24, vd_off 25, rcr_off 27 (u16), lat_div 29, vd_pending 30, vd_irq_phase 31, rcr_origin 32.
 *   VDMA 25 bytes: satb_pending 0, satb_done_valid 1, satb_done_at 2 (u64), dma_state 10, dma_start 11 (u64), dma_word_mclk 19 (u16), dma_done 21 (u32).
 * A row that sets a field to its last legal value is declared accept (the boundary pair of its refusal): the accepted snapshot must re-serialise to the same bytes.
 */
static void sk_put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}
#define ED_U8(fn, rg, off, val) static void fn(uint8_t *b) { b[sk_payload_off(rg) + (off)] = (uint8_t)(val); }
#define ED_U16(fn, rg, off, val) static void fn(uint8_t *b) { sk_put16(b + sk_payload_off(rg) + (off), (uint32_t)(val)); }
#define ED_U32(fn, rg, off, val) static void fn(uint8_t *b) { sk_put32(b + sk_payload_off(rg) + (off), (uint32_t)(val)); }
#define BM_AT(b, off) ((b) + sk_payload_off(RG_BEAM) + (off))
#define VDC_REG(n) (2u + 2u * (n))

ED_U16(ed_cta_200, RG_VCE0, 1u, 0x200u)
ED_U16(ed_cta_1ff, RG_VCE0, 1u, 0x1FFu)
ED_U8(ed_pal_odd2, RG_PALT, 1u, 2u)
ED_U8(ed_pal_odd1, RG_PALT, 1u, 1u)
ED_U8(ed_pal_last_ff, RG_PALT, 1023u, 0xFFu)
ED_U8(ed_ar_32, RG_VDC0, 0u, 32u)
ED_U8(ed_ar_31, RG_VDC0, 0u, 31u)
ED_U8(ed_st_40, RG_VDC0, 1u, 0x40u)
ED_U8(ed_st_3f, RG_VDC0, 1u, 0x3Fu)
ED_U16(ed_r03, RG_VDC0, VDC_REG(3u), 1u)
ED_U16(ed_r04, RG_VDC0, VDC_REG(4u), 1u)
ED_U16(ed_r05, RG_VDC0, VDC_REG(5u), 0x2000u)
ED_U16(ed_r06, RG_VDC0, VDC_REG(6u), 0x0400u)
ED_U16(ed_r07, RG_VDC0, VDC_REG(7u), 0x0400u)
ED_U16(ed_r08, RG_VDC0, VDC_REG(8u), 0x0200u)
ED_U16(ed_r09, RG_VDC0, VDC_REG(9u), 0x0100u)
ED_U16(ed_r0a, RG_VDC0, VDC_REG(10u), 0x0080u)
ED_U16(ed_r0b, RG_VDC0, VDC_REG(11u), 0x0080u)
ED_U16(ed_r0c, RG_VDC0, VDC_REG(12u), 0x0020u)
ED_U16(ed_r0d, RG_VDC0, VDC_REG(13u), 0x0200u)
ED_U16(ed_r0d_1ff, RG_VDC0, VDC_REG(13u), 0x01FFu)
ED_U16(ed_r0e, RG_VDC0, VDC_REG(14u), 0x0100u)
ED_U16(ed_r0f, RG_VDC0, VDC_REG(15u), 0x0020u)
ED_U8(ed_stage_3, RG_BEAM, 8u, 3u)
ED_U8(ed_vstate_4, RG_BEAM, 9u, 4u)
ED_U16(ed_vtogo_513, RG_BEAM, 10u, 513u)
ED_U16(ed_vtogo_512, RG_BEAM, 10u, 512u)
ED_U16(ed_dk_0_vds, RG_BEAM, 12u, 0u)
ED_U16(ed_rcr_8000, RG_BEAM, 14u, 0x8000u)
ED_U16(ed_rcr_7fff, RG_BEAM, 14u, 0x7FFFu)
ED_U16(ed_fl_261, RG_BEAM, 16u, 261u)
ED_U16(ed_fl_264, RG_BEAM, 16u, 264u)
ED_U16(ed_fl_263, RG_BEAM, 16u, 263u)
ED_U16(ed_fl_262, RG_BEAM, 16u, 262u)
ED_U16(ed_bgy_512, RG_BEAM, 18u, 512u)
ED_U16(ed_bgy_511, RG_BEAM, 18u, 511u)
ED_U16(ed_mwr_100, RG_BEAM, 20u, 0x100u)
ED_U8(ed_burst_2, RG_BEAM, 22u, 2u)
ED_U8(ed_dirty_40, RG_BEAM, 23u, 0x40u)
ED_U8(ed_sprite_4, RG_BEAM, 24u, 4u)
ED_U16(ed_vdoff_555, RG_BEAM, 25u, 1365u)
ED_U16(ed_vdoff_554, RG_BEAM, 25u, 1364u)
ED_U16(ed_vdoff_ffff, RG_BEAM, 25u, 0xFFFFu)
ED_U16(ed_rcroff_555, RG_BEAM, 27u, 1365u)
ED_U8(ed_ld_5, RG_BEAM, 29u, 5u)
ED_U8(ed_ld_1, RG_BEAM, 29u, 1u)
ED_U8(ed_ld_4, RG_BEAM, 29u, 4u)
ED_U8(ed_ld_2, RG_BEAM, 29u, 2u)
ED_U8(ed_vdpend_2, RG_BEAM, 30u, 2u)
ED_U8(ed_vph_2, RG_BEAM, 31u, 2u)
ED_U8(ed_vph_1, RG_BEAM, 31u, 1u)
ED_U8(ed_rorg_2, RG_BEAM, 32u, 2u)
ED_U8(ed_satbp_2, RG_VDMA, 0u, 2u)
ED_U8(ed_satbv_2, RG_VDMA, 1u, 2u)
ED_U8(ed_dst_3, RG_VDMA, 10u, 3u)
ED_U32(ed_dd_10001, RG_VDMA, 21u, 65537u)
ED_U32(ed_dd_10000, RG_VDMA, 21u, 65536u)
ED_U32(ed_dd_1, RG_VDMA, 21u, 1u)
ED_U32(ed_dd_0, RG_VDMA, 21u, 0u)
ED_U16(ed_dw_9_idle, RG_VDMA, 19u, 9u)

/* the display line is judged against the vertical state: inside VDW (v_state 1) below 512, anywhere else exactly 0xFFFF (F-1) */
static void ed_dk_511_vdw(uint8_t *b)
{
    b[sk_payload_off(RG_BEAM) + 9u] = 1u;
    sk_put16(BM_AT(b, 12u), 511u);
}
static void ed_dk_512_vdw(uint8_t *b)
{
    b[sk_payload_off(RG_BEAM) + 9u] = 1u;
    sk_put16(BM_AT(b, 12u), 512u);
}
static void ed_dk_ffff_vdw(uint8_t *b)
{
    b[sk_payload_off(RG_BEAM) + 9u] = 1u; /* display_k stays 0xFFFF, which is not a display line inside VDW */
}
/* line_idx is judged against the machine time of the same stream before any multiplication; lat_div is 2 in these rows so that the divider rule is not what refuses them */
static void ed_li_far(uint8_t *b)
{
    sk_put64(BM_AT(b, 0u), sk_get64(b + sk_payload_off(RG_TIME)) / UINT64_C(1365) + UINT64_C(3));
    b[sk_payload_off(RG_BEAM) + 29u] = 2u;
}
static void ed_li_limit(uint8_t *b)
{
    sk_put64(BM_AT(b, 0u), sk_get64(b + sk_payload_off(RG_TIME)) / UINT64_C(1365) + UINT64_C(2));
    b[sk_payload_off(RG_BEAM) + 29u] = 2u;
}
static void ed_li_2_63(uint8_t *b)
{
    sk_put64(BM_AT(b, 0u), UINT64_C(1) << 63);
    b[sk_payload_off(RG_BEAM) + 29u] = 2u;
}
static void ed_li_max(uint8_t *b)
{
    sk_put64(BM_AT(b, 0u), UINT64_MAX);
    b[sk_payload_off(RG_BEAM) + 29u] = 2u;
}
/* a divider of 0 is the state before the first line event only: after line 0 it would be a zero divisor */
static void ed_ld_0_late(uint8_t *b)
{
    sk_put64(BM_AT(b, 0u), UINT64_C(5));
    b[sk_payload_off(RG_BEAM) + 29u] = 0u;
}
/* the fold window rules (D-166, T-04-14; plan 04-19): the next event the BEAM members name must lie in [machine_time, machine_time + 2730), judged after the divider rule, which is 2 in these rows so that it is not what refuses them. The rows that need a machine time set it to a multiple of 1365 plus a delta, and the
 * line index to a count that the line-limit rule accepts (at most time / 1365 + 2); stage 0 names the EV_LINE of line_idx at line_idx * 1365 (raster_phase_mclk 0, ASSUMED LED-VID-0004), stage 1 the EV_VD of line_idx - 1 at its origin plus vd_off */
static void win_set(uint8_t *b, uint64_t time, uint64_t line_idx, uint8_t stage)
{
    sk_put64(b + sk_payload_off(RG_TIME), time);
    sk_put64(BM_AT(b, 0u), line_idx);
    b[sk_payload_off(RG_BEAM) + 8u] = stage;
    b[sk_payload_off(RG_BEAM) + 29u] = 2u;
}
static void ed_ev_before(uint8_t *b)
{
    sk_put64(BM_AT(b, 0u), UINT64_C(1)); /* the event of line 1 is at 1365, far behind the machine time of the snapshot */
    b[sk_payload_off(RG_BEAM) + 8u] = 0u;
    b[sk_payload_off(RG_BEAM) + 29u] = 2u;
}
static void ed_ev_far(uint8_t *b)
{
    win_set(b, UINT64_C(74) * UINT64_C(1365), UINT64_C(76), 0u); /* the event is at 76 * 1365 = time + 2730 exactly */
}
static void ed_ev_2729(uint8_t *b)
{
    win_set(b, UINT64_C(74) * UINT64_C(1365) + UINT64_C(1), UINT64_C(76), 0u); /* the event is time + 2729, the last clock the window holds */
}
static void ed_ev_now(uint8_t *b)
{
    win_set(b, UINT64_C(74) * UINT64_C(1365), UINT64_C(74), 0u); /* the event is at the machine time itself */
}
static void ed_ev_none(uint8_t *b)
{
    win_set(b, sk_get64(b + sk_payload_off(RG_TIME)), UINT64_C(5), 1u); /* stage 1 with no VD flag outstanding names no event */
    b[sk_payload_off(RG_BEAM) + 30u] = 0u;
}
/* an outstanding VD event as a run leaves it (review fix L-01): the phase hdw_start (vd_irq_phase 1) and no RCR event of the line still to come (rcr_off none), so that only the window rule is what judges the row */
static void vd_set(uint8_t *b, uint64_t line_idx)
{
    win_set(b, sk_get64(b + sk_payload_off(RG_TIME)), line_idx, 1u);
    b[sk_payload_off(RG_BEAM) + 30u] = 1u;
    b[sk_payload_off(RG_BEAM) + 31u] = 1u;
    sk_put16(BM_AT(b, 25u), 100u);
    sk_put16(BM_AT(b, 27u), 0xFFFFu);
}
static void ed_ev_vd_before(uint8_t *b)
{
    vd_set(b, UINT64_C(1)); /* the EV_VD of line 0 at 100, behind the machine time */
}
static void ed_ev_vd_ahead(uint8_t *b)
{
    vd_set(b, sk_get64(b + sk_payload_off(RG_TIME)) / UINT64_C(1365) + UINT64_C(2)); /* the EV_VD of the line before the last allowed index: its origin plus 100 is less than one line ahead */
}
/* the event order rules (review fix L-01), each from the accepted edge event-vd-ahead with one member changed: VD outstanding under the phase line_start (which raises VD at EV_LINE and never leaves it outstanding), VD outstanding at the stage RCR (EV_VD always comes first), and an RCR event of the same line at or before the VD event (the window
 * end is after the window start, so a run never has it); an RCR event after the VD event is accepted */
static void ed_vd_line_start(uint8_t *b)
{
    ed_ev_vd_ahead(b);
    b[sk_payload_off(RG_BEAM) + 31u] = 0u;
}
static void ed_vd_stage_rcr(uint8_t *b)
{
    ed_ev_vd_ahead(b);
    b[sk_payload_off(RG_BEAM) + 8u] = 2u;
    sk_put16(BM_AT(b, 27u), 200u);
}
static void ed_vd_after_rcr(uint8_t *b)
{
    ed_ev_vd_ahead(b);
    sk_put16(BM_AT(b, 27u), 50u);
}
static void ed_vd_rcr_equal(uint8_t *b)
{
    ed_ev_vd_ahead(b);
    sk_put16(BM_AT(b, 27u), 100u);
}
static void ed_vd_before_rcr(uint8_t *b)
{
    ed_ev_vd_ahead(b);
    sk_put16(BM_AT(b, 27u), 101u);
}
/* satb_done_at is judged only when valid, in [machine_time, machine_time + 4096]; dma_start only while a transfer runs, at or before the machine time */
static void sat_set(uint8_t *b, uint8_t valid, uint64_t at)
{
    b[sk_payload_off(RG_VDMA) + 1u] = valid;
    sk_put64(b + sk_payload_off(RG_VDMA) + 2u, at);
}
static void ed_sat_before(uint8_t *b)
{
    sat_set(b, 1u, sk_get64(b + sk_payload_off(RG_TIME)) - UINT64_C(1));
}
static void ed_sat_now(uint8_t *b)
{
    sat_set(b, 1u, sk_get64(b + sk_payload_off(RG_TIME)));
}
static void ed_sat_4096(uint8_t *b)
{
    sat_set(b, 1u, sk_get64(b + sk_payload_off(RG_TIME)) + UINT64_C(4096));
}
static void ed_sat_4097(uint8_t *b)
{
    sat_set(b, 1u, sk_get64(b + sk_payload_off(RG_TIME)) + UINT64_C(4097));
}
static void ed_sat_max(uint8_t *b)
{
    sat_set(b, 1u, UINT64_MAX);
}
static void ed_sat_idle_far(uint8_t *b)
{
    sat_set(b, 0u, UINT64_MAX);
}
static void dst_set(uint8_t *b, uint8_t state, uint64_t start)
{
    b[sk_payload_off(RG_VDMA) + 10u] = state;
    sk_put64(b + sk_payload_off(RG_VDMA) + 11u, start);
    sk_put16(b + sk_payload_off(RG_VDMA) + 19u, 8u);
}
static void ed_dst_ahead(uint8_t *b)
{
    dst_set(b, 1u, sk_get64(b + sk_payload_off(RG_TIME)) + UINT64_C(1));
}
static void ed_dst_now(uint8_t *b)
{
    dst_set(b, 1u, sk_get64(b + sk_payload_off(RG_TIME)));
}
static void ed_dst_idle_far(uint8_t *b)
{
    dst_set(b, 0u, UINT64_MAX);
}
/* a transfer in progress has 8, 12 or 16 master clocks per word, never 0 (the closed form divides by it) */
static void ed_dma_word_0(uint8_t *b)
{
    b[sk_payload_off(RG_VDMA) + 10u] = 2u;
    sk_put16(b + sk_payload_off(RG_VDMA) + 19u, 0u);
}
static void ed_dw_9_run(uint8_t *b)
{
    b[sk_payload_off(RG_VDMA) + 10u] = 1u;
    sk_put16(b + sk_payload_off(RG_VDMA) + 19u, 9u);
}
static void ed_dw_8_run(uint8_t *b)
{
    b[sk_payload_off(RG_VDMA) + 10u] = 1u;
    sk_put16(b + sk_payload_off(RG_VDMA) + 19u, 8u);
}
static void ed_dw_12_run(uint8_t *b)
{
    b[sk_payload_off(RG_VDMA) + 10u] = 2u;
    sk_put16(b + sk_payload_off(RG_VDMA) + 19u, 12u);
}
static void ed_dw_16_run(uint8_t *b)
{
    b[sk_payload_off(RG_VDMA) + 10u] = 2u;
    sk_put16(b + sk_payload_off(RG_VDMA) + 19u, 16u);
}

static const struct {
    const char *name;
    int mode;
    void (*edit)(uint8_t *);
} sk_rows[] = {
    {"unchanged", M_EDIT, ed_none},
    {"wrong-profile", M_EDIT, ed_profile},
    {"wrong-content", M_EDIT, ed_content},
    {"bad-prefix", M_EDIT, ed_prefix},
    {"bad-version", M_EDIT, ed_version},
    {"swapped-tags", M_EDIT, ed_tags},
    {"bad-region-length", M_EDIT, ed_region_length},
    {"short", M_SHORT, ed_none},
    {"trailing", M_LONG, ed_none},
    {"speed-2", M_EDIT, ed_speed2},
    {"timer-128", M_EDIT, ed_timer128},
    {"reload-128", M_EDIT, ed_reload128},
    {"timer-start-2", M_EDIT, ed_timer_start2},
    {"timer-origin-ahead", M_EDIT, ed_timer_origin_ahead},
    {"timer-origin-far", M_EDIT, ed_timer_origin_far},
    {"timer-origin-at-time", M_EDIT, ed_timer_origin_at_time},
    {"blk-rem-inactive", M_EDIT, ed_blk_rem_inactive},
    {"blk-rem-65537", M_EDIT, ed_blk_rem_65537},
    {"blk-rem-65536", M_EDIT, ed_blk_rem_65536},
    {"blk-rem-0-active", M_EDIT, ed_blk_rem_zero_active},
    {"blk-opcode", M_EDIT, ed_blk_opcode},
    {"blk-toggle-tii", M_EDIT, ed_blk_toggle_tii},
    {"blk-toggle-tia", M_EDIT, ed_blk_toggle_tia},
    {"time-2-63", M_EDIT, ed_time_2_63},
    {"time-max", M_EDIT, ed_time_max},
    {"time-2-63-1", M_EDIT, ed_time_below},
    {"p-b-bit", M_EDIT, ed_p_b_bit},
    {"policy-5", M_EDIT, ed_policy5},
    {"policy-other", M_EDIT, ed_policy_other},
    {"seed-no-flag", M_EDIT, ed_seed_no_flag},
    {"tma-zero-2", M_EDIT, ed_tma2},
    {"irq-sample-2", M_EDIT, ed_sample2},
    {"irq-disable-8", M_EDIT, ed_disable8},
    {"tiq-2", M_EDIT, ed_tiq2},
    {"stim-order", M_EDIT, ed_stim_order},
    {"stim-gap", M_EDIT, ed_stim_gap},
    {"stim-line-3", M_EDIT, ed_stim_line3},
    {"stim-valid", M_EDIT, ed_stim_valid},
    {"schd-count-1025", M_EDIT, ed_schd_count},
    {"schd-reserved", M_EDIT, ed_schd_reserved},
    {"schd-port-2", M_EDIT, ed_schd_port2},
    {"schd-port-0", M_EDIT, ed_schd_port0},
    {"schd-bit-8", M_EDIT, ed_schd_bit8},
    {"schd-opposite", M_EDIT, ed_schd_opposite},
    {"schd-order", M_EDIT, ed_schd_order},
    {"schd-equal-port", M_EDIT, ed_schd_equal_port},
    {"schd-unused-slot", M_EDIT, ed_schd_unused},
    {"schd-valid", M_EDIT, ed_schd_valid},
    {"iopt-port-2", M_EDIT, ed_iopt_port2},
    {"iopt-port-1", M_EDIT, ed_iopt_port1},
    {"iopt-kind-2", M_EDIT, ed_iopt_kind2},
    {"latch-any", M_EDIT, ed_latch},
    {"cta-200", M_EDIT, ed_cta_200},
    {"cta-1ff", M_EDIT, ed_cta_1ff},
    {"pal-odd-2", M_EDIT, ed_pal_odd2},
    {"pal-odd-1", M_EDIT, ed_pal_odd1},
    {"pal-last-ff", M_EDIT, ed_pal_last_ff},
    {"ar-32", M_EDIT, ed_ar_32},
    {"ar-31", M_EDIT, ed_ar_31},
    {"status-40", M_EDIT, ed_st_40},
    {"status-3f", M_EDIT, ed_st_3f},
    {"r03-1", M_EDIT, ed_r03},
    {"r04-1", M_EDIT, ed_r04},
    {"r05-mask", M_EDIT, ed_r05},
    {"r06-mask", M_EDIT, ed_r06},
    {"r07-mask", M_EDIT, ed_r07},
    {"r08-mask", M_EDIT, ed_r08},
    {"r09-mask", M_EDIT, ed_r09},
    {"r0a-mask", M_EDIT, ed_r0a},
    {"r0b-mask", M_EDIT, ed_r0b},
    {"r0c-mask", M_EDIT, ed_r0c},
    {"r0d-mask", M_EDIT, ed_r0d},
    {"r0d-1ff", M_EDIT, ed_r0d_1ff},
    {"r0e-mask", M_EDIT, ed_r0e},
    {"r0f-mask", M_EDIT, ed_r0f},
    {"stage-3", M_EDIT, ed_stage_3},
    {"v-state-4", M_EDIT, ed_vstate_4},
    {"v-togo-513", M_EDIT, ed_vtogo_513},
    {"v-togo-512", M_EDIT, ed_vtogo_512},
    {"disp-511-vdw", M_EDIT, ed_dk_511_vdw},
    {"disp-512-vdw", M_EDIT, ed_dk_512_vdw},
    {"disp-ffff-vdw", M_EDIT, ed_dk_ffff_vdw},
    {"disp-0-vds", M_EDIT, ed_dk_0_vds},
    {"rcr-8000", M_EDIT, ed_rcr_8000},
    {"rcr-7fff", M_EDIT, ed_rcr_7fff},
    {"frame-261", M_EDIT, ed_fl_261},
    {"frame-264", M_EDIT, ed_fl_264},
    {"frame-263", M_EDIT, ed_fl_263},
    {"frame-262", M_EDIT, ed_fl_262},
    {"bg-y-512", M_EDIT, ed_bgy_512},
    {"bg-y-511", M_EDIT, ed_bgy_511},
    {"mwr-100", M_EDIT, ed_mwr_100},
    {"burst-2", M_EDIT, ed_burst_2},
    {"dirty-40", M_EDIT, ed_dirty_40},
    {"sprite-4", M_EDIT, ed_sprite_4},
    {"vd-pending-2", M_EDIT, ed_vdpend_2},
    {"vd-off-1365", M_EDIT, ed_vdoff_555},
    {"vd-off-1364", M_EDIT, ed_vdoff_554},
    {"vd-off-ffff", M_EDIT, ed_vdoff_ffff},
    {"rcr-off-1365", M_EDIT, ed_rcroff_555},
    {"line-far", M_EDIT, ed_li_far},
    {"line-limit", M_EDIT, ed_li_limit},
    {"line-2-63", M_EDIT, ed_li_2_63},
    {"line-max", M_EDIT, ed_li_max},
    {"lat-div-5", M_EDIT, ed_ld_5},
    {"lat-div-1", M_EDIT, ed_ld_1},
    {"lat-div-0-late", M_EDIT, ed_ld_0_late},
    {"lat-div-4", M_EDIT, ed_ld_4},
    {"lat-div-2", M_EDIT, ed_ld_2},
    {"vd-phase-2", M_EDIT, ed_vph_2},
    {"vd-phase-1", M_EDIT, ed_vph_1},
    {"rcr-origin-2", M_EDIT, ed_rorg_2},
    {"satb-pend-2", M_EDIT, ed_satbp_2},
    {"satb-valid-2", M_EDIT, ed_satbv_2},
    {"dma-state-3", M_EDIT, ed_dst_3},
    {"dma-done-65537", M_EDIT, ed_dd_10001},
    {"dma-done-65536", M_EDIT, ed_dd_10000},
    {"dma-done-1", M_EDIT, ed_dd_1},
    {"dma-done-0", M_EDIT, ed_dd_0},
    {"dma-word-0", M_EDIT, ed_dma_word_0},
    {"dma-word-9", M_EDIT, ed_dw_9_run},
    {"dma-word-8", M_EDIT, ed_dw_8_run},
    {"dma-word-12", M_EDIT, ed_dw_12_run},
    {"dma-word-16", M_EDIT, ed_dw_16_run},
    {"dma-word-9-idle", M_EDIT, ed_dw_9_idle},
    {"event-before-time", M_EDIT, ed_ev_before},
    {"event-too-far", M_EDIT, ed_ev_far},
    {"event-2729", M_EDIT, ed_ev_2729},
    {"event-at-time", M_EDIT, ed_ev_now},
    {"event-none", M_EDIT, ed_ev_none},
    {"event-vd-before", M_EDIT, ed_ev_vd_before},
    {"event-vd-ahead", M_EDIT, ed_ev_vd_ahead},
    {"vd-pend-line-start", M_EDIT, ed_vd_line_start},
    {"vd-pend-stage-rcr", M_EDIT, ed_vd_stage_rcr},
    {"vd-after-rcr", M_EDIT, ed_vd_after_rcr},
    {"vd-rcr-equal", M_EDIT, ed_vd_rcr_equal},
    {"vd-before-rcr", M_EDIT, ed_vd_before_rcr},
    {"satb-done-before", M_EDIT, ed_sat_before},
    {"satb-done-now", M_EDIT, ed_sat_now},
    {"satb-done-4096", M_EDIT, ed_sat_4096},
    {"satb-done-4097", M_EDIT, ed_sat_4097},
    {"satb-done-max", M_EDIT, ed_sat_max},
    {"satb-done-idle-far", M_EDIT, ed_sat_idle_far},
    {"dma-start-ahead", M_EDIT, ed_dst_ahead},
    {"dma-start-now", M_EDIT, ed_dst_now},
    {"dma-start-idle-far", M_EDIT, ed_dst_idle_far},
    {"null-buffer", M_NULL, ed_none},
    {"buffer-in-instance", M_OVERLAP, ed_none},
    {"snapshot-capacity", M_SNAP_CAPACITY, ed_none},
    {"snapshot-overlap", M_SNAP_OVERLAP, ed_none},
};
#define SK_ROWS_N (sizeof sk_rows / sizeof sk_rows[0])

/* the number of a refusal by its short name (without the snapshot. prefix), 0 for none */
static uint32_t sk_code_of(const char *name)
{
    uint32_t n;

    for (n = 1u; n < (uint32_t)DS_SNAPREF_SLOTS; n++) {
        const char *full = ds_snapshot_refusal_name(n);

        if (full != NULL && strncmp(full, "snapshot.", 9u) == 0 && strcmp(full + 9, name) == 0) {
            return n;
        }
    }
    return 0u;
}

/*
 * snapshot_refuse (D-121, T-03-83, T-03-85, Pitfall 20): the source instance runs to the stop of the case and is snapshotted; for each row of the expect the named edit (or misuse of the API) is made to a copy of the snapshot, and ds_restore (or ds_snapshot for the snapshot-* rows) must return exactly the declared refusal, with the TARGET byte-identical afterwards:
 * the target is a twin booted with another seed whose restorable members are poisoned, copied before the call, and compared byte for byte over the whole instance (ROM included) and by digest. A row declared accept must be accepted and the restored target must re-serialise to exactly the edited snapshot (accepted inputs re-serialise identically). The codes observed go to the deferred kind snapshot_refusal_coverage.
 */
void kind_snapshot_refuse(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted bs;
    struct booted bt;
    struct ds_instance *s;
    struct ds_instance *t = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    char rom[48];
    char twin[PK_LIST_MAX];
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    uint64_t until = 0u;
    uint8_t *good = NULL;
    uint8_t *edit = NULL;
    uint8_t *check_buf = NULL;
    unsigned char *base = NULL;
    size_t size;
    unsigned rows = 0u;
    unsigned accepted = 0u;
    unsigned refused = 0u;
    uint32_t rc;

    memset(&bs, 0, sizeof bs);
    memset(&bt, 0, sizeof bt);
    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || sk_twin_request(k->val[K_REQUEST], twin, sizeof twin) != 0
        || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, "a snapshot_refuse case needs fixture = authored:NAME, until_mclk, a seeded request and expect = ROW=CODE|ROW=CODE|... (CODE a name of snapshot_refusals.def without the prefix, or accept)");
        return;
    }
    s = sk_boot(c, k, rom, k->val[K_REQUEST], 1, &bs);
    if (s == NULL) {
        return;
    }
    t = sk_boot(c, k, rom, twin, 0, &bt);
    if (t == NULL) {
        release_boot(&bs);
        return;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rc = ds_run(s, &rq, &res);
    note_run(c, s->machine_time, rc);
    check(c, rc == (uint32_t)DS_RUN_REASON_UNTIL && res.steps > 4u, "the run to the snapshot stop took four steps or fewer");
    size = ds_snapshot_size();
    good = (uint8_t *)malloc(size + 1u);
    edit = (uint8_t *)malloc(size + 1u);
    check_buf = (uint8_t *)malloc(size + 1u);
    base = (unsigned char *)malloc(ds_instance_size());
    if (good == NULL || edit == NULL || check_buf == NULL || base == NULL) {
        harness_error(c, "out of memory for the refusal rows");
        goto done;
    }
    check(c, ds_snapshot(s, good, size, NULL) == (uint32_t)DS_SNAPREF_NONE, "ds_snapshot of the source was refused");
    check(c, sk_stream_size() == size, "the hand-kept frames of this file do not add up to ds_snapshot_size()");
    while ((item = pk_next_item(&rest, '|')) != NULL) {
        char *eq = strchr(item, '=');
        size_t r;
        uint32_t want = 0u;
        int want_accept;
        uint32_t got_code = 0u;
        size_t call_len = size;
        const uint8_t *call_buf = edit;

        if (eq == NULL) {
            harness_error(c, "a row of a snapshot_refuse expect is ROW=CODE");
            goto done;
        }
        *eq = '\0';
        for (r = 0u; r < SK_ROWS_N && strcmp(sk_rows[r].name, item) != 0; r++) {
        }
        want_accept = strcmp(eq + 1, "accept") == 0;
        want = want_accept ? 0u : sk_code_of(eq + 1);
        if (r == SK_ROWS_N || (!want_accept && want == 0u)) {
            harness_error(c, "a row of a snapshot_refuse expect names no known row or no refusal code");
            goto done;
        }
        rows++;
        memcpy(edit, good, size);
        edit[size] = 0x5Eu;
        sk_rows[r].edit(edit);
        snap_poison(t, 0xA5u);
        memcpy(base, t, ds_instance_size());
        switch (sk_rows[r].mode) {
        case M_SHORT:
            call_len = size - 1u;
            break;
        case M_LONG:
            call_len = size + 1u;
            break;
        case M_NULL:
            call_buf = NULL;
            break;
        case M_OVERLAP:
            call_buf = (const uint8_t *)t + 16u;
            break;
        default:
            break;
        }
        if (sk_rows[r].mode == M_SNAP_CAPACITY) {
            got_code = ds_snapshot(s, check_buf, size - 1u, NULL);
        } else if (sk_rows[r].mode == M_SNAP_OVERLAP) {
            got_code = ds_snapshot(s, (uint8_t *)s + 100u, size, NULL);
        } else {
            got_code = ds_restore(t, call_buf, call_len);
        }
        if (got_code != 0u && got_code < (uint32_t)DS_SNAPREF_SLOTS) {
            sk_observed[got_code] = 1u;
        }
        if (want_accept) {
            check(c, got_code == (uint32_t)DS_SNAPREF_NONE, "a row declared accept was refused");
            if (got_code == (uint32_t)DS_SNAPREF_NONE) {
                size_t w2 = 0u;

                accepted++;
                check(c, ds_snapshot(t, check_buf, size, &w2) == (uint32_t)DS_SNAPREF_NONE && w2 == size && memcmp(check_buf, edit, size) == 0, "an accepted snapshot does not re-serialise to the same bytes");
                if (strncmp(item, "event-", 6u) == 0 || strncmp(item, "satb-done-", 10u) == 0 || strncmp(item, "dma-start-", 10u) == 0) { /* plan 04-19: an accepted edge of a fold window still runs defined: a bounded run from it (the fold loops over a few lines at most) ends for a declared reason and its state snapshots again */
                    struct ds_run_request rq3;
                    struct ds_run_result res3;

                    memset(&rq3, 0, sizeof rq3);
                    memset(&res3, 0, sizeof res3);
                    rq3.struct_size = (uint32_t)sizeof rq3;
                    rq3.until_mclk = t->machine_time + UINT64_C(6000);
                    rc = ds_run(t, &rq3, &res3);
                    check(c, rc == (uint32_t)DS_RUN_REASON_UNTIL || rc == (uint32_t)DS_RUN_REASON_REFUSED, "a bounded run from an accepted edge of a fold window ended for no declared reason");
                    check(c, ds_snapshot(t, check_buf, size, NULL) == (uint32_t)DS_SNAPREF_NONE, "the snapshot of a state reached by a run from an accepted edge of a fold window was refused");
                }
                if (strcmp(item, "time-2-63-1") == 0) { /* T-03-84: the largest machine time that restore accepts must not be one that a run can wrap: a deadline near 2^64 is refused, and a short run from there takes steps and ends below 2^64 */
                    struct ds_run_request rq2;
                    struct ds_run_result res2;
                    uint64_t at = t->machine_time;

                    memset(&rq2, 0, sizeof rq2);
                    memset(&res2, 0, sizeof res2);
                    rq2.struct_size = (uint32_t)sizeof rq2;
                    rq2.until_mclk = UINT64_MAX;
                    check(c, ds_run(t, &rq2, &res2) == (uint32_t)DS_RUN_REASON_REFUSED && res2.refusal == (uint32_t)DS_RUNREF_time_wrap && t->machine_time == at, "a run to a deadline of 2^64 - 1 from a restored time of 2^63 - 1 was not refused as time_wrap with the instance unchanged");
                    memset(&res2, 0, sizeof res2);
                    rq2.until_mclk = at + 200u;
                    check(c, ds_run(t, &rq2, &res2) == (uint32_t)DS_RUN_REASON_UNTIL && res2.steps > 0u && t->machine_time > at && t->machine_time < UINT64_MAX, "a short run from a restored time of 2^63 - 1 did not take steps or wrapped the machine time");
                }
            } else {
                emit_err("snapshot_refuse: row %s: refused as %s\n", item, ds_snapshot_refusal_name(got_code));
            }
        } else {
            check(c, got_code == want, "a row was refused with another code than the one it declares, or was accepted");
            if (got_code != want) {
                emit_err("snapshot_refuse: row %s: got %s, wanted snapshot.%s\n", item, ds_snapshot_refusal_name(got_code) != NULL ? ds_snapshot_refusal_name(got_code) : "?", eq + 1);
            }
            refused++;
            check(c, memcmp(base, t, ds_instance_size()) == 0, "a refusal changed the target instance: it is not byte-identical afterwards");
            check(c, snap_first_diff(t, (const struct ds_instance *)(const void *)base) == NULL, "a refusal changed a member of the target instance");
        }
    }
    check(c, rows >= 1u, "the refuse case has no row");
    snprintf(got, gotcap, "rows=%u refused=%u accepted=%u", rows, refused, accepted);
done:
    free(good);
    free(edit);
    free(check_buf);
    free(base);
    release_boot(&bs);
    release_boot(&bt);
}

/*
 * snapshot_refusal_coverage (DEFERRED, D-86 pattern): every member of snapshot_refusals.def was observed by a refuse row, the table has no gap and every name prints as snapshot.NAME.
 */
void kind_snapshot_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    uint32_t n;
    unsigned members = 0u;
    unsigned seen = 0u;

    (void)k;
    for (n = 1u; n < (uint32_t)DS_SNAPREF_SLOTS; n++) {
        const char *name = ds_snapshot_refusal_name(n);

        if (name == NULL) {
            continue;
        }
        members++;
        seen += sk_observed[n] != 0u ? 1u : 0u;
        check(c, strncmp(name, "snapshot.", 9u) == 0, "a snapshot refusal name does not start with snapshot.");
        check(c, sk_observed[n] != 0u, "a snapshot refusal code was never observed by a refuse row");
        if (sk_observed[n] == 0u) {
            emit_err("snapshot_refusal_coverage: %s was not observed by any refuse row\n", name);
        }
    }
    check(c, members > 0u, "snapshot_refusals.def has no member");
    snprintf(got, gotcap, "members=%u observed=%u", members, seen);
}

/* ---- snapshot_hostile ---- */

/*
 * snapshot_hostile (plan 03-31; D-122, D-133, T-03-86, Pitfall 20): the bounded hostile corpus of snapshot bytes, GENERATED IN MEMORY from one good snapshot of the fixture's mid-run state and a fixed splitmix64 stream: no file is read or committed and the generator takes no host randomness, so every run makes the same inputs.
 * Every input goes to ds_restore on a poisoned twin of the source. The verdict of each is one of two and both are checked, with nothing else allowed: REFUSED (the code is a member of the closed set and the target is byte-identical to its copy from before the call, ROM included) or ACCEPTED (the input has exactly the length of the stream, the accepted bytes re-serialise to
 * identical bytes, the SHA-256 of the input is the digest of the target, a bounded run of the restored machine takes no undefined step, and a snapshot taken after that run restores again). The layout used to make the inputs is the hand-kept one of this file, so the generator does not use the table the core's restore is written against.
 * Classes (expect = a list of them, each once, then optionally mid-block, which requires the source to sit inside a block transfer so that the BLKT bytes are live):
 *   boundary  the length of the buffer at every region boundary and one byte either side (the start and the end of the prefix, the version, each frame and each payload, the end of the stream): shorter and longer than the stream is refused, the exact length is accepted
 *   length    the length word of every frame set to 0, length - 1, length + 1 and 2^32 - 1: refused
 *   frame     every byte of the prefix, of the version and of every tag with its low bit flipped, and the versions 0 and 2^32 - 1: refused
 *   sample    64 truncations at lengths drawn from the stream below the length of the stream: refused
 *   flip      the low bit flipped in every byte of the control fields (profile hash, content id, power-on policy, time, P and the speed, the bus selection, timer, interrupt controller, the first three stimulus slots, the block transfer state, the port bytes, the schedule count and reserved word and its first three events): refused or accepted,
 *             whichever the field domain says, never anything else
 *   data      the low bit (and the high bit of RAM) flipped in the bytes that every value of is a legal state (work RAM, the mapping registers, A, X, Y, S, pc, the bus latches): accepted, which proves that the accepted path is taken and is clean
 * Counts by class go to the evidence text. Evidence tier E0: the core compared with its own layout; a pass says that no hostile input of these classes was mishandled, not that the format is right.
 */
enum { HC_BOUNDARY, HC_LENGTH, HC_FRAME, HC_SAMPLE, HC_FLIP, HC_DATA, HC_N };
enum { HW_REFUSE, HW_ACCEPT, HW_EITHER };
#define HS_PAD 80u
#define HS_RUN_CLOCKS 3000u
#define HS_SEED UINT64_C(0x484F5354494C4531)

static const char *const hc_names[HC_N] = {"boundary", "length", "frame", "sample", "flip", "data"};

struct hs_env {
    struct ctx *c;
    struct ds_instance *t;
    uint8_t *good;
    uint8_t *edit;
    uint8_t *again;
    unsigned char *base;
    size_t size;
    unsigned cls;
    unsigned total;
    unsigned n[HC_N];
    unsigned refused[HC_N];
    unsigned accepted[HC_N];
    unsigned ran;
};

static uint64_t hs_next(uint64_t *state)
{
    uint64_t z;

    *state = *state + UINT64_C(0x9E3779B97F4A7C15);
    z = *state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* the edit buffer is the good snapshot followed by a canary, so a read past the stream finds a known byte and never one outside the buffer */
static void hs_fresh(struct hs_env *e)
{
    memcpy(e->edit, e->good, e->size);
    memset(e->edit + e->size, 0x5E, HS_PAD);
}

/* one hostile input: the first len bytes of the edit buffer go to ds_restore on the poisoned target; want says what the generator knows from the layout (refused, accepted, or only that it is one of the two) */
static void hs_try(struct hs_env *e, size_t len, int want, unsigned idx)
{
    struct ctx *c = e->c;
    uint32_t rc;

    snap_poison(e->t, (e->total & 1u) != 0u ? 0x5Au : 0xA5u);
    e->total++;
    memcpy(e->base, e->t, ds_instance_size());
    rc = ds_restore(e->t, e->edit, len);
    e->n[e->cls]++;
    if (rc != (uint32_t)DS_SNAPREF_NONE) {
        int known = rc < (uint32_t)DS_SNAPREF_SLOTS && ds_snapshot_refusal_name(rc) != NULL;

        check(c, known, "ds_restore returned a code that is not a member of the closed set of snapshot refusals");
        check(c, want != HW_ACCEPT, "a hostile input that is a legal state (data class) was refused");
        check(c, memcmp(e->base, e->t, ds_instance_size()) == 0, "a refused hostile input changed the target: it is not byte-identical afterwards");
        if (want == HW_ACCEPT || memcmp(e->base, e->t, ds_instance_size()) != 0) {
            emit_err("snapshot_hostile: %s %u: refused as %s (length %lu)\n", hc_names[e->cls], idx, known ? ds_snapshot_refusal_name(rc) : "?", (unsigned long)len);
        }
        e->refused[e->cls]++;
    } else {
        size_t w2 = 0u;
        int again_ok;

        check(c, want != HW_REFUSE, "a hostile input that must be refused (a wrong length, tag, frame length, prefix or version) was accepted");
        check(c, len == e->size, "an input of another length than the stream was accepted");
        if (want == HW_REFUSE || len != e->size) {
            emit_err("snapshot_hostile: %s %u: accepted (length %lu)\n", hc_names[e->cls], idx, (unsigned long)len);
        }
        e->accepted[e->cls]++;
        if (len == e->size) {
            uint8_t d1[DS_DIGEST_BYTES];
            uint8_t d2[DS_DIGEST_BYTES];
            struct ds_run_request rq;
            struct ds_run_result res;
            uint32_t rr;

            again_ok = ds_snapshot(e->t, e->again, e->size, &w2) == (uint32_t)DS_SNAPREF_NONE && w2 == e->size && memcmp(e->again, e->edit, e->size) == 0;
            check(c, again_ok, "an accepted hostile input does not re-serialise to the same bytes");
            ds_sha256(e->edit, e->size, d1);
            check(c, ds_digest(e->t, d2) == (uint32_t)DS_OK && memcmp(d1, d2, DS_DIGEST_BYTES) == 0, "the SHA-256 of an accepted hostile input is not the digest of the restored instance");
            if (!again_ok) {
                emit_err("snapshot_hostile: %s %u: the accepted input does not re-serialise to itself\n", hc_names[e->cls], idx);
            }
            /* the restored machine is run for a bounded time: whatever the field values were (any address of a block transfer, any mapping, any stamp), the step is defined (the sanitizer stage judges it) */
            memset(&rq, 0, sizeof rq);
            memset(&res, 0, sizeof res);
            rq.struct_size = (uint32_t)sizeof rq;
            rq.until_mclk = e->t->machine_time + (uint64_t)HS_RUN_CLOCKS;
            rr = ds_run(e->t, &rq, &res);
            check(c, rr == (uint32_t)DS_RUN_REASON_UNTIL || rr == (uint32_t)DS_RUN_REASON_REFUSED, "a bounded run of an accepted hostile state ended for no declared reason");
            e->ran++;
            /* a state that a run reached is a state restore must take (legal at every run stop): the snapshot of it restores back, member for member */
            memcpy(e->base, e->t, ds_instance_size());
            check(c, ds_snapshot(e->t, e->again, e->size, NULL) == (uint32_t)DS_SNAPREF_NONE, "the snapshot of a state reached by a run from an accepted hostile state was refused");
            snap_poison(e->t, 0x5Au);
            rc = ds_restore(e->t, e->again, e->size);
            check(c, rc == (uint32_t)DS_SNAPREF_NONE, "the snapshot of a state reached by a run from an accepted hostile state was not restorable");
            if (rc == (uint32_t)DS_SNAPREF_NONE) {
                check(c, snap_first_diff(e->t, (const struct ds_instance *)(const void *)e->base) == NULL, "restoring the snapshot of a run-reached state did not give the same state");
            } else {
                emit_err("snapshot_hostile: %s %u: a run-reached state was refused as %s\n", hc_names[e->cls], idx, ds_snapshot_refusal_name(rc) != NULL ? ds_snapshot_refusal_name(rc) : "?");
            }
        }
    }
}

/* the region boundaries of the stream, ascending and distinct */
static unsigned hs_bounds(size_t *b, unsigned cap)
{
    unsigned n = 0u;
    unsigned r;
    unsigned i;
    unsigned j;
    size_t cand[3u * SK_FRAMES + 3u];
    unsigned nc = 0u;

    cand[nc++] = 0u;
    cand[nc++] = (size_t)SK_PREFIX_BYTES;
    cand[nc++] = (size_t)SK_PREFIX_BYTES + 4u;
    for (r = 0u; r < SK_FRAMES; r++) {
        cand[nc++] = sk_frame_off(r);
        cand[nc++] = sk_payload_off(r);
        cand[nc++] = sk_payload_off(r) + (size_t)sk_frames[r].len;
    }
    for (i = 0u; i < nc; i++) {
        int dup = 0;

        for (j = 0u; j < n; j++) {
            dup = dup || b[j] == cand[i];
        }
        if (!dup && n < cap) {
            b[n++] = cand[i];
        }
    }
    for (i = 1u; i < n; i++) { /* insertion sort: n is at most 45 */
        size_t v = b[i];

        for (j = i; j > 0u && b[j - 1u] > v; j--) {
            b[j] = b[j - 1u];
        }
        b[j] = v;
    }
    return n;
}

static void hs_boundary(struct hs_env *e)
{
    size_t b[3u * SK_FRAMES + 3u];
    unsigned n = hs_bounds(b, (unsigned)(sizeof b / sizeof b[0]));
    unsigned i;
    int d;

    e->cls = HC_BOUNDARY;
    for (i = 0u; i < n; i++) {
        for (d = -1; d <= 1; d++) {
            size_t len;

            if (b[i] == 0u && d < 0) {
                continue;
            }
            len = (size_t)((long)b[i] + d);
            hs_fresh(e);
            hs_try(e, len, len == e->size ? HW_ACCEPT : HW_REFUSE, (unsigned)(3u * i + (unsigned)(d + 1)));
        }
    }
}

static void hs_length(struct hs_env *e)
{
    unsigned r;
    unsigned q;

    e->cls = HC_LENGTH;
    for (r = 0u; r < SK_FRAMES; r++) {
        const uint32_t v[4] = {0u, sk_frames[r].len - 1u, sk_frames[r].len + 1u, 0xFFFFFFFFu};

        for (q = 0u; q < 4u; q++) {
            hs_fresh(e);
            sk_put32(e->edit + sk_frame_off(r) + 4u, v[q]);
            hs_try(e, e->size, HW_REFUSE, 4u * r + q);
        }
    }
}

static void hs_frame(struct hs_env *e)
{
    unsigned i;
    unsigned r;
    unsigned q;
    unsigned idx = 0u;

    e->cls = HC_FRAME;
    for (i = 0u; i < SK_PREFIX_BYTES + 4u; i++) { /* the prefix, its NUL and the version */
        hs_fresh(e);
        e->edit[i] ^= 0x01u;
        hs_try(e, e->size, HW_REFUSE, idx++);
    }
    for (q = 0u; q < 2u; q++) {
        hs_fresh(e);
        sk_put32(e->edit + SK_PREFIX_BYTES, q == 0u ? 0u : 0xFFFFFFFFu);
        hs_try(e, e->size, HW_REFUSE, idx++);
    }
    for (r = 0u; r < SK_FRAMES; r++) {
        for (q = 0u; q < 4u; q++) {
            hs_fresh(e);
            e->edit[sk_frame_off(r) + q] ^= 0x01u;
            hs_try(e, e->size, HW_REFUSE, idx++);
        }
    }
}

static void hs_sample(struct hs_env *e)
{
    uint64_t st = HS_SEED;
    unsigned i;

    e->cls = HC_SAMPLE;
    for (i = 0u; i < 64u; i++) {
        size_t len = (size_t)(hs_next(&st) % (uint64_t)e->size); /* 0 to size - 1: always shorter than the stream */

        hs_fresh(e);
        hs_try(e, len, HW_REFUSE, i);
    }
}

/* the control fields: payload byte ranges (from, to) of a region; the flip is the low bit of every byte */
static const struct {
    unsigned rg;
    size_t from;
    size_t to;
} hs_ctl[] = {
    {RG_PROF, 0u, 32u},   {RG_CONT, 0u, 40u},   {RG_POWR, 0u, 16u},  {RG_TIME, 0u, 8u},     {RG_CPU0, 4u, 5u},      {RG_CPU0, 7u, 8u},     {RG_BUS0, 2u, 3u},
    {RG_TIMR, 0u, 11u},   {RG_IRQC, 0u, 3u},    {RG_IRQS, 0u, 30u},  {RG_BLKT, 0u, 11u},    {RG_IOPT, 0u, 6u},      {RG_SCHD, 0u, 48u},    {RG_SCHD, 16384u, 16392u},
    /* plan 04-18 (D-166): every control byte of the four Phase 4 control regions, VCE0 3, VDC0 45, BEAM 33 and VDMA 25 bytes */
    {RG_VCE0, 0u, 3u},    {RG_VDC0, 0u, 45u},   {RG_BEAM, 0u, 33u},  {RG_VDMA, 0u, 25u},
};

static void hs_flip(struct hs_env *e)
{
    unsigned i;
    unsigned idx = 0u;

    e->cls = HC_FLIP;
    for (i = 0u; i < (unsigned)(sizeof hs_ctl / sizeof hs_ctl[0]); i++) {
        size_t at;

        for (at = hs_ctl[i].from; at < hs_ctl[i].to; at++) {
            hs_fresh(e);
            e->edit[sk_payload_off(hs_ctl[i].rg) + at] ^= 0x01u;
            hs_try(e, e->size, HW_EITHER, idx++);
        }
    }
    /* plan 04-18 (D-166, F-22): the palette, by sampling, and a bit that no palette entry has: bit 1 of an odd palette byte (entries have 9 bits) is refused */
    for (i = 0u; i < 4u; i++) {
        static const size_t odd[4] = {1u, 3u, 511u, 1023u};

        hs_fresh(e);
        e->edit[sk_payload_off(RG_PALT) + odd[i]] ^= 0x02u;
        hs_try(e, e->size, HW_REFUSE, idx++);
    }
}

static void hs_data(struct hs_env *e)
{
    static const size_t ram[4] = {0u, 1u, 4095u, 8191u};
    unsigned i;
    unsigned idx = 0u;

    e->cls = HC_DATA;
    for (i = 0u; i < 8u; i++) { /* work RAM: the first, second and last two bytes, the low bit and the high bit */
        hs_fresh(e);
        e->edit[sk_payload_off(RG_RAM8) + ram[i >> 1]] ^= (i & 1u) != 0u ? 0x80u : 0x01u;
        hs_try(e, e->size, HW_ACCEPT, idx++);
    }
    for (i = 0u; i < DS_MPR_COUNT; i++) { /* every byte value is a legal mapping register */
        hs_fresh(e);
        e->edit[sk_payload_off(RG_MPRS) + i] ^= 0x01u;
        hs_try(e, e->size, HW_ACCEPT, idx++);
    }
    for (i = 0u; i < 6u; i++) { /* A, X, Y, S and the two bytes of pc */
        hs_fresh(e);
        e->edit[sk_payload_off(RG_CPU0) + (i < 4u ? i : i + 1u)] ^= 0x01u;
        hs_try(e, e->size, HW_ACCEPT, idx++);
    }
    for (i = 0u; i < 2u; i++) { /* the data bus latch and the mapping latch */
        hs_fresh(e);
        e->edit[sk_payload_off(RG_BUS0) + i] ^= 0x01u;
        hs_try(e, e->size, HW_ACCEPT, idx++);
    }
    /* plan 04-18 (D-166, F-22): PALT, VRAM and SATB bytes by sampling, never all 65,536 VRAM bytes: the first, second and last bytes and the bytes around the middle, the low bit and the high bit (the odd palette bytes keep bit 0 only, so the high bit of an odd one is a refusal row of the flip class) */
    {
        static const size_t pal[4] = {0u, 2u, 1022u, 1u};
        static const size_t vram[8] = {0u, 1u, 255u, 256u, 32767u, 32768u, 65534u, 65535u};
        static const size_t satb[6] = {0u, 1u, 255u, 256u, 510u, 511u};

        for (i = 0u; i < 8u; i++) {
            hs_fresh(e);
            e->edit[sk_payload_off(RG_PALT) + pal[i >> 1]] ^= (i & 1u) != 0u && (pal[i >> 1] & 1u) != 0u ? 0x01u : (i & 1u) != 0u ? 0x80u : 0x01u;
            hs_try(e, e->size, HW_ACCEPT, idx++);
        }
        for (i = 0u; i < 16u; i++) {
            hs_fresh(e);
            e->edit[sk_payload_off(RG_VRAM) + vram[i >> 1]] ^= (i & 1u) != 0u ? 0x80u : 0x01u;
            hs_try(e, e->size, HW_ACCEPT, idx++);
        }
        for (i = 0u; i < 12u; i++) {
            hs_fresh(e);
            e->edit[sk_payload_off(RG_SATB) + satb[i >> 1]] ^= (i & 1u) != 0u ? 0x80u : 0x01u;
            hs_try(e, e->size, HW_ACCEPT, idx++);
        }
    }
}

/*
 * vdc-tia (plan 04-06; D-166, D-175): sampled restore stops INSIDE a block transfer that targets the VDC data port. The authored vdc_reg image (the TIA upload of 32 words to the data pair and the TAI readback of them) is run under a seeded-prng-v3 request to four stops chosen by the block transfer state of the instance, never by a time
 * taken from a run: the TIA with 63, 32 and 1 bytes left (opcode $E3, the destination $0002 or $0003; an odd and an even byte, the first and the last) and the TAI with 33 bytes left (opcode $F3, the source $0002 or $0003). At each stop the machine is snapshotted and restored into a fresh boot with another seed whose every restorable member is poisoned, the restored instance equals the source
 * in every member (VRAM, the VDC registers, the read buffer and write latch, the block transfer bytes among them), and the target is run on to the deadline of the case: its machine time, step count, verdict RAM, digest and every restorable member equal those of one uninterrupted run. Evidence tier E0 (the core compared with itself); no hardware claim.
 */
struct hv_spec {
    uint32_t opcode;
    uint32_t remaining;
};

static int hv_stop(const struct ds_instance *inst, void *ctx)
{
    const struct hv_spec *sp = (const struct hv_spec *)ctx;
    uint32_t port = sp->opcode == 0xE3u ? (uint32_t)inst->blk_dst : (uint32_t)inst->blk_src;

    return inst->blk_active == 1u && (uint32_t)inst->blk_opcode == sp->opcode && inst->blk_remaining == sp->remaining && (port & 0xFFFEu) == 2u;
}

static void hs_vdc_block(struct ctx *c, const char *rom, uint64_t until, unsigned *stops)
{
    static const struct hv_spec spec[4] = {{0xE3u, 63u}, {0xE3u, 32u}, {0xE3u, 1u}, {0xF3u, 33u}};
    static const char *const req_src = "bind.powerup_policy=seeded-prng-v3;seed=5";
    static const char *const req_twin = "bind.powerup_policy=seeded-prng-v3;seed=6";
    struct booted br;
    const struct ds_instance *ref = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    uint64_t ref_steps = 0u;
    uint8_t ref_digest[DS_DIGEST_BYTES];
    uint8_t *snap = (uint8_t *)malloc(ds_snapshot_size());
    unsigned i;
    const char *err;

    if (snap == NULL) {
        harness_error(c, "out of memory for the snapshot of the vdc-tia stops");
        return;
    }
    err = boot_built_rom(rom, req_src, &br, &ref);
    if (err != NULL) {
        free(snap);
        harness_error(c, err);
        return;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    check(c, ds_run((struct ds_instance *)(void *)br.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_UNTIL, "the uninterrupted run of the vdc-tia part did not end on its deadline");
    ref_steps = res.steps;
    check(c, ds_digest(ref, ref_digest) == 0u, "the digest of the uninterrupted run of the vdc-tia part was refused");
    for (i = 0u; i < 4u && !c->harness; i++) {
        struct booted bs;
        struct booted bt;
        const struct ds_instance *cs = NULL;
        const struct ds_instance *ct = NULL;
        struct ds_instance *ws;
        struct ds_instance *wt;
        struct hv_spec sp = spec[i];
        uint64_t steps1;
        uint8_t dig[DS_DIGEST_BYTES];
        const char *diff;
        char msg[200];

        err = boot_built_rom(rom, req_src, &bs, &cs);
        if (err != NULL) {
            harness_error(c, err);
            break;
        }
        err = boot_built_rom(rom, req_twin, &bt, &ct);
        if (err != NULL) {
            release_boot(&bs);
            harness_error(c, err);
            break;
        }
        ws = (struct ds_instance *)(void *)bs.mem;
        wt = (struct ds_instance *)(void *)bt.mem;
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        rq.stop = hv_stop;
        rq.stop_ctx = &sp;
        snprintf(msg, sizeof msg, "vdc-tia stop %u (opcode $%02X, %u bytes left): the run did not stop inside the block transfer of the VDC data port", i, (unsigned)sp.opcode, (unsigned)sp.remaining);
        check(c, ds_run(ws, &rq, &res) == (uint32_t)DS_RUN_REASON_CALLBACK, msg);
        steps1 = res.steps;
        if (!c->failed) {
            snprintf(msg, sizeof msg, "vdc-tia stop %u: ds_snapshot of the source was refused", i);
            check(c, ds_snapshot(ws, snap, ds_snapshot_size(), NULL) == (uint32_t)DS_SNAPREF_NONE, msg);
            snap_poison(wt, 0xA5u);
            snprintf(msg, sizeof msg, "vdc-tia stop %u: ds_restore refused the snapshot of a machine inside a block transfer of the VDC data port", i);
            check(c, ds_restore(wt, snap, ds_snapshot_size()) == (uint32_t)DS_SNAPREF_NONE, msg);
            diff = snap_first_diff(ct, cs);
            snprintf(msg, sizeof msg, "vdc-tia stop %u: the restored instance differs from the source in the member %s", i, diff != NULL ? diff : "-");
            check(c, diff == NULL, msg);
            memset(&rq, 0, sizeof rq);
            memset(&res, 0, sizeof res);
            rq.struct_size = (uint32_t)sizeof rq;
            rq.until_mclk = until;
            snprintf(msg, sizeof msg, "vdc-tia stop %u: the restored target did not run on to the deadline", i);
            check(c, ds_run(wt, &rq, &res) == (uint32_t)DS_RUN_REASON_UNTIL, msg);
            snprintf(msg, sizeof msg, "vdc-tia stop %u: the machine time or the step count of the restored chain differs from the uninterrupted run", i);
            check(c, ct->machine_time == ref->machine_time && steps1 + res.steps == ref_steps, msg);
            snprintf(msg, sizeof msg, "vdc-tia stop %u: the verdict RAM or the digest of the restored chain differs from the uninterrupted run", i);
            check(c, memcmp(ct->ram8k + V_OFF, ref->ram8k + V_OFF, V_SIZE) == 0 && ds_digest(ct, dig) == 0u && memcmp(dig, ref_digest, DS_DIGEST_BYTES) == 0, msg);
            diff = snap_first_diff(ct, ref);
            snprintf(msg, sizeof msg, "vdc-tia stop %u: the member %s of the restored chain differs from the uninterrupted run (VRAM and the VDC state included)", i, diff != NULL ? diff : "-");
            check(c, diff == NULL, msg);
            (*stops)++;
        }
        release_boot(&bs);
        release_boot(&bt);
    }
    release_boot(&br);
    free(snap);
}

void kind_snapshot_hostile(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct hs_env e;
    struct booted bs;
    struct booted bt;
    struct ds_instance *s;
    struct ds_instance *t = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    char rom[48];
    char twin[PK_LIST_MAX];
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    uint64_t until = 0u;
    unsigned want_cls = 0u;
    int want_block = 0;
    int want_vdc = 0;
    char vdc_rom[48];
    unsigned vdc_stops = 0u;
    int first = 1;
    unsigned q;
    unsigned total = 0u;
    unsigned refused = 0u;
    unsigned accepted = 0u;
    int ok = 1;
    uint32_t rc;
    int off = 0;
    char detail[1024];

    memset(&bs, 0, sizeof bs);
    memset(&bt, 0, sizeof bt);
    memset(&e, 0, sizeof e);
    detail[0] = '\0';
    if (authored_name(k->val[K_FIXTURE], rom, sizeof rom) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || sk_twin_request(k->val[K_REQUEST], twin, sizeof twin) != 0
        || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        ok = 0;
    }
    while (ok && (item = pk_next_item(&rest, ';')) != NULL) {
        if (first) {
            char *r2 = item;
            char *w;

            first = 0;
            while ((w = pk_next_item(&r2, ',')) != NULL) {
                for (q = 0u; q < HC_N && strcmp(w, hc_names[q]) != 0; q++) {
                }
                if (q == HC_N || (want_cls & (1u << q)) != 0u) {
                    ok = 0;
                    break;
                }
                want_cls |= 1u << q;
            }
        } else if (strcmp(item, "mid-block") == 0 && !want_block) {
            want_block = 1;
        } else if (strncmp(item, "vdc-tia=", 8u) == 0 && !want_vdc && strspn(item + 8, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 8) && strlen(item + 8) > 0u && strlen(item + 8) < sizeof vdc_rom) {
            want_vdc = 1;
            memcpy(vdc_rom, item + 8, strlen(item + 8) + 1u);
        } else {
            ok = 0;
        }
    }
    if (!ok || want_cls == 0u) {
        harness_error(c, "a snapshot_hostile case needs fixture = authored:NAME, until_mclk, a seeded request and expect = a list of boundary, length, frame, sample, flip, data (each once)[;mid-block][;vdc-tia=NAME]");
        return;
    }
    s = sk_boot(c, k, rom, k->val[K_REQUEST], 1, &bs);
    if (s == NULL) {
        return;
    }
    t = sk_boot(c, k, rom, twin, 0, &bt);
    if (t == NULL) {
        release_boot(&bs);
        return;
    }
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rc = ds_run(s, &rq, &res);
    note_run(c, s->machine_time, rc);
    check(c, rc == (uint32_t)DS_RUN_REASON_UNTIL && res.steps > 4u, "the run to the snapshot stop took four steps or fewer");
    if (want_block) {
        check(c, s->blk_active == 1u && s->blk_remaining > 0u && s->blk_remaining < 65536u, "the source is not inside a block transfer at the stop: the BLKT bytes of the corpus would not be live");
    }
    e.c = c;
    e.t = t;
    e.size = ds_snapshot_size();
    e.good = (uint8_t *)malloc(e.size + HS_PAD);
    e.edit = (uint8_t *)malloc(e.size + HS_PAD);
    e.again = (uint8_t *)malloc(e.size + HS_PAD);
    e.base = (unsigned char *)malloc(ds_instance_size());
    if (e.good == NULL || e.edit == NULL || e.again == NULL || e.base == NULL) {
        harness_error(c, "out of memory for the hostile corpus");
        goto done;
    }
    check(c, ds_snapshot(s, e.good, e.size, NULL) == (uint32_t)DS_SNAPREF_NONE, "ds_snapshot of the source was refused");
    check(c, sk_stream_size() == e.size, "the hand-kept frames of this file do not add up to ds_snapshot_size()");
    if ((want_cls & (1u << HC_BOUNDARY)) != 0u) {
        hs_boundary(&e);
    }
    if ((want_cls & (1u << HC_LENGTH)) != 0u) {
        hs_length(&e);
    }
    if ((want_cls & (1u << HC_FRAME)) != 0u) {
        hs_frame(&e);
    }
    if ((want_cls & (1u << HC_SAMPLE)) != 0u) {
        hs_sample(&e);
    }
    if ((want_cls & (1u << HC_FLIP)) != 0u) {
        hs_flip(&e);
    }
    if ((want_cls & (1u << HC_DATA)) != 0u) {
        hs_data(&e);
    }
    for (q = 0u; q < HC_N; q++) {
        if ((want_cls & (1u << q)) != 0u) {
            check(c, e.n[q] > 0u, "a requested class of the corpus generated no input");
        }
        total += e.n[q];
        refused += e.refused[q];
        accepted += e.accepted[q];
    }
    check(c, refused > 0u && accepted > 0u, "the corpus did not take both paths: it has no refused or no accepted input");
    check(c, e.ran == accepted, "an accepted input was not run");
    if (want_vdc) {
        hs_vdc_block(c, vdc_rom, until, &vdc_stops);
        check(c, vdc_stops == 4u || c->harness, "the vdc-tia part did not restore at its four stops");
    }
    snprintf(got, gotcap, "cases=%u refused=%u accepted=%u", total, refused, accepted);
    for (q = 0u; q < HC_N; q++) { /* the counts by class go to the evidence text */
        if ((want_cls & (1u << q)) != 0u && off >= 0 && (size_t)off < sizeof detail) {
            off += snprintf(detail + off, sizeof detail - (size_t)off, "class=%s cases=%u refused=%u accepted=%u\n", hc_names[q], e.n[q], e.refused[q], e.accepted[q]);
        }
    }
    if (want_vdc && off >= 0 && (size_t)off < sizeof detail) {
        off += snprintf(detail + off, sizeof detail - (size_t)off, "class=vdc-tia stops=%u (restore inside a block transfer of the VDC data port, continued to the end, E0)\n", vdc_stops);
    }
    set_evidence_detail(detail);
done:
    free(e.good);
    free(e.edit);
    free(e.again);
    free(e.base);
    release_boot(&bs);
    release_boot(&bt);
}
