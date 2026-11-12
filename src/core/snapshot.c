/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * snapshot.c - ds_snapshot_size, ds_snapshot and ds_restore (CORE-05; plan 03-30; D-121, D-122, Pitfall 20, T-03-83 to T-03-85).
 * The snapshot is the canonical digest stream: ds_snapshot writes it with ds_digest_stream into the caller's buffer, so there is no second serializer and the SHA-256 of a snapshot is the digest of the instance. The size is the sum of the fixed lengths of the one region table of serial.c
 * (the prefix, the version and the frame and payload of every region). The ROM and the three boot-derived members (mapper, byte_order, header) are not in the stream and are not restored.
 * ds_restore is validate-then-commit over the caller's buffer in place, with no allocation and no copy of the instance. Pass 1 refuses a bad argument, an overlap of the buffer with the instance, a length other than the fixed size (truncated, trailing), a wrong prefix or version, and then, region by
 * region in stream order, a wrong tag or frame length and every field outside its domain; it writes nothing. Pass 2 walks the same bytes and copies every field into the instance; it checks nothing and so cannot fail. Each region has ONE decoder that does both: it reads its fields in stream order, and
 * either (commit) stores them or (validate) judges them, so the two passes cannot disagree about where a field is. The decoders are the mirror of emit_payload in serial.c: the round trip (restore, then snapshot, equal bytes) and the poisoned-target comparison of the runner (snapkinds.c) are what catch a drift.
 * Field domains (the instance can reach all of them and only them): the tests of the decoders below. A power-on policy must be the one the instance's profile binds, a block transfer is all zero unless active, a stimulus or schedule list is a valid declared list followed by all-zero slots, and the machine time is below 2^63 so that
 * no step or deadline can wrap it (Pitfall 20; ds_run refuses a deadline within one step of 2^64 on top of that).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_poweron.h"
#include "ds_serial.h"
#include "ds_snapshot.h"

/* the refusal names, generated from snapshot_refusals.def */
#define DS_SNAPREF(name, number) [number] = "snapshot." #name,
static const char *const ds_snapshot_refusal_names[DS_SNAPREF_SLOTS] = {
    [0] = "ok",
#include "snapshot_refusals.def"
};
#undef DS_SNAPREF

/* the slot ceiling (F4): the rows are numbered 1..N without a gap or a reuse, so N is the row count and must leave a slot below DS_SNAPREF_SLOTS */
#define DS_SNAPREF(name, number) +1
enum { DS_SNAPREF_ROWS = 0
#include "snapshot_refusals.def"
};
#undef DS_SNAPREF
_Static_assert(DS_SNAPREF_ROWS < (int)DS_SNAPREF_SLOTS, "snapshot_refusals.def has more rows than DS_SNAPREF_SLOTS slots");

const char *ds_snapshot_refusal_name(uint32_t number)
{
    return number < (uint32_t)DS_SNAPREF_SLOTS ? ds_snapshot_refusal_names[number] : NULL;
}

/* ---- the size and the writer ---- */

size_t ds_snapshot_size(void)
{
    uint32_t prefix = 0u;
    size_t total;
    uint32_t i;

    (void)ds_stream_prefix(&prefix);
    total = (size_t)prefix + sizeof(uint32_t); /* the prefix and the version */
    for (i = 0u; i < DS_REGION_COUNT; i++) {
        const char *tag = NULL;
        uint32_t len = 0u;

        if (ds_region_frame(i, &tag, &len) != 0u) {
            total += 4u + sizeof(uint32_t) + (size_t)len; /* the tag, the length and the payload */
        }
    }
    return total;
}

/* do [a, a + an) and [b, b + bn) share a byte? Integer addresses and differences only, so no sum can wrap (the same test as ranges_overlap of boot.c); an empty range shares nothing */
static int ranges_overlap(uintptr_t a, size_t an, uintptr_t b, size_t bn)
{
    if (an == 0u || bn == 0u) {
        return 0;
    }
    if (a <= b) {
        return (b - a) < (uintptr_t)an;
    }
    return (a - b) < (uintptr_t)bn;
}

struct snap_out {
    uint8_t *buf;
    size_t cap;
    size_t pos;
};

static void snap_sink(void *ctx, const uint8_t *data, size_t len)
{
    struct snap_out *o = (struct snap_out *)ctx;
    size_t i;

    for (i = 0u; i < len && o->pos < o->cap; i++) { /* the capacity was checked before the stream starts, so the bound never drops a byte; it keeps the writer inside the buffer whatever the stream does */
        o->buf[o->pos] = data[i];
        o->pos++;
    }
}

uint32_t ds_snapshot(const struct ds_instance *inst, uint8_t *buf, size_t cap, size_t *written)
{
    struct snap_out o;
    size_t size = ds_snapshot_size();

    if (inst == NULL || buf == NULL) {
        return (uint32_t)DS_SNAPREF_argument;
    }
    if (ranges_overlap((uintptr_t)inst, sizeof *inst, (uintptr_t)buf, cap)) {
        return (uint32_t)DS_SNAPREF_overlap;
    }
    if (cap < size) {
        return (uint32_t)DS_SNAPREF_capacity;
    }
    o.buf = buf;
    o.cap = size;
    o.pos = 0u;
    ds_digest_stream(inst, snap_sink, &o);
    if (written != NULL) {
        *written = o.pos;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

/* ---- the reader: u8-sized pieces, little-endian integers, from the caller's buffer; the buffer is exactly the fixed size, checked before any read ---- */

struct rd {
    const uint8_t *p;
};

static uint8_t r_u8(struct rd *r)
{
    uint8_t v = r->p[0];

    r->p += 1;
    return v;
}

static uint16_t r_u16(struct rd *r)
{
    uint16_t v = (uint16_t)((uint32_t)r->p[0] | ((uint32_t)r->p[1] << 8));

    r->p += 2;
    return v;
}

static uint32_t r_u32(struct rd *r)
{
    uint32_t v = (uint32_t)r->p[0] | ((uint32_t)r->p[1] << 8) | ((uint32_t)r->p[2] << 16) | ((uint32_t)r->p[3] << 24);

    r->p += 4;
    return v;
}

static uint64_t r_u64(struct rd *r)
{
    uint64_t v = UINT64_C(0);
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        v |= (uint64_t)r->p[i] << (8u * i);
    }
    r->p += 8;
    return v;
}

/* ---- the field domains ---- */

#define SNAP_TIME_LIMIT (UINT64_C(1) << 63)  /* a machine time at or above this is refused (Pitfall 20) */
#define SNAP_P_B_BIT 0x10u                   /* the B bit of P: pushed by BRK and PHP, never stored (H8-12) */
#define SNAP_IRQ_REG_MASK 0x07u              /* the three bits of the interrupt disable register */
#define SNAP_BLK_MAX 65536u                  /* the most bytes a block transfer has left (the length operand 0 is 65536, H8-6) */
#define SNAP_PAD_BITS 0xFFu                  /* the eight buttons of the two-button pad; every bit above is reserved */
#define SNAP_UP_DOWN 0x50u
#define SNAP_LEFT_RIGHT 0xA0u
#define SNAP_VDC_AR_MAX 31u                  /* the address register keeps its low 5 bits (D-146) */
#define SNAP_VDC_STATUS_MAX 0x3Fu            /* status flags 0 to 5; BSY (bit 6) and bit 7 read 0 (D-144) */
#define SNAP_VCE_ENTRY_MAX 0x1FFu            /* a colour table address or entry is 9 bits (D-153) */
#define SNAP_V_TOGO_MAX 512u                 /* VDW is a 9-bit register, so a state can have 512 lines left; no clamp (F-1) */
#define SNAP_BEAM_NONE 0xFFFFu               /* display_k outside the display state, and vd_off or rcr_off for "no event" */
#define SNAP_BEAM_LINE_MAX 0x7FFFu           /* the RCR counter saturates here (D-142) */
#define SNAP_DMA_WORDS_MAX 65536u            /* LENR + 1 words at most (D-151) */

/* the number of values of model parameter index: its allowed list is values separated by a vertical bar */
static uint32_t model_values(uint32_t index)
{
    const char *allowed = ds_model_allowed(index);
    uint32_t n = 1u;

    if (allowed == NULL) {
        return 0u;
    }
    while (*allowed != '\0') {
        if (*allowed == '|') {
            n++;
        }
        allowed++;
    }
    return n;
}

/* what pass 1 learns from one region that a later region is judged against: the device kind of each port (IOPT, judged before SCHD) and the machine time of the stream (TIME, judged before TIMR: the timer origin may not lie after it) */
struct vctx {
    uint8_t slot_type[DS_PORT_COUNT];
    uint64_t time;
};

/* a decoder reads its region from r in stream order and, when commit is nonzero, stores every field into inst and returns DS_SNAPREF_NONE; otherwise it writes nothing and returns the refusal of the first field outside its domain */
typedef uint32_t (*dec_fn)(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit);

static uint32_t dec_prof(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t want[DS_DIGEST_BYTES];
    uint32_t i;
    uint32_t same = 1u;

    (void)v;
    if (commit) {
        r->p += DS_DIGEST_BYTES; /* the profile hash names the profile the instance already holds: nothing to store */
        return (uint32_t)DS_SNAPREF_NONE;
    }
    ds_profile_hash(&inst->profile, want);
    for (i = 0u; i < DS_DIGEST_BYTES; i++) {
        if (r_u8(r) != want[i]) {
            same = 0u;
        }
    }
    return same != 0u ? (uint32_t)DS_SNAPREF_NONE : (uint32_t)DS_SNAPREF_profile;
}

static uint32_t dec_cont(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t same = 1u;
    uint32_t bytes;
    uint32_t size_class;
    uint32_t i;

    (void)v;
    for (i = 0u; i < DS_CONTENT_ID_BYTES; i++) {
        if (r_u8(r) != inst->content_id[i]) {
            same = 0u;
        }
    }
    bytes = r_u32(r);
    size_class = r_u32(r);
    if (commit) {
        return (uint32_t)DS_SNAPREF_NONE; /* the content identity is the instance's own: nothing to store */
    }
    if (same == 0u || size_class != inst->size_class || bytes != ds_size_bytes(inst->size_class)) {
        return (uint32_t)DS_SNAPREF_content;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_powr(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t policy = r_u32(r);
    uint64_t seed = r_u64(r);
    uint32_t has_seed = r_u32(r);
    uint32_t bound;

    (void)v;
    if (commit) {
        inst->powerup_policy = policy;
        inst->powerup_seed = seed;
        inst->has_seed = has_seed;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    bound = ds_policy_of(inst->profile.field[DS_FIELD_powerup_policy].value);
    if (policy < (uint32_t)DS_POLICY_ZERO || policy > (uint32_t)DS_POLICY_SEEDED_V2) {
        return (uint32_t)DS_SNAPREF_field; /* a policy outside 1 to 3 */
    }
    if (policy != bound) {
        return (uint32_t)DS_SNAPREF_profile; /* a valid policy that is not the one the instance's profile binds: the snapshot of another profile */
    }
    if (has_seed > 1u || (has_seed == 0u && seed != UINT64_C(0)) || (ds_policy_is_seeded(policy) != 0u) != (has_seed != 0u)) {
        return (uint32_t)DS_SNAPREF_field; /* the seed is present exactly under the two seeded policies, and is 0 when absent (D-79(b)) */
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_time(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint64_t t = r_u64(r);

    if (commit) {
        inst->machine_time = t;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    v->time = t; /* kept for dec_timr: pass 1 only, the commit pass judges nothing */
    return t >= SNAP_TIME_LIMIT ? (uint32_t)DS_SNAPREF_time : (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_ram8(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t i;

    (void)v;
    for (i = 0u; i < DS_RAM8K_BYTES; i++) {
        uint8_t b = r_u8(r);

        if (commit) {
            inst->ram8k[i] = b;
        }
    }
    return (uint32_t)DS_SNAPREF_NONE; /* every byte value is a legal RAM byte */
}

static uint32_t dec_mprs(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t i;

    (void)v;
    for (i = 0u; i < DS_MPR_COUNT; i++) {
        uint8_t b = r_u8(r);

        if (commit) {
            inst->mpr[i] = b;
        }
    }
    return (uint32_t)DS_SNAPREF_NONE; /* every byte value is a legal mapping register */
}

static uint32_t dec_cpu0(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t a = r_u8(r);
    uint8_t x = r_u8(r);
    uint8_t y = r_u8(r);
    uint8_t s = r_u8(r);
    uint8_t p = r_u8(r);
    uint16_t pc = r_u16(r);
    uint8_t speed = r_u8(r);

    (void)v;
    if (commit) {
        inst->a = a;
        inst->x = x;
        inst->y = y;
        inst->s = s;
        inst->p = p;
        inst->pc = pc;
        inst->speed = speed;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    if (((uint32_t)p & SNAP_P_B_BIT) != 0u) {
        return (uint32_t)DS_SNAPREF_reserved;
    }
    return speed > 1u ? (uint32_t)DS_SNAPREF_field : (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_bus0(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t io_buffer = r_u8(r);
    uint8_t mpr_latch = r_u8(r);
    uint8_t tma_zero = r_u8(r);

    (void)v;
    if (commit) {
        inst->io_buffer = io_buffer;
        inst->mpr_latch = mpr_latch;
        inst->tma_zero = tma_zero;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    return (uint32_t)tma_zero >= model_values((uint32_t)DS_MODEL_tma_zero) ? (uint32_t)DS_SNAPREF_field : (uint32_t)DS_SNAPREF_NONE; /* the selection is an index into the allowed list of the model parameter */
}

static uint32_t dec_timr(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint64_t origin = r_u64(r);
    uint8_t counter = r_u8(r);
    uint8_t reload = r_u8(r);
    uint8_t enabled = r_u8(r);

    if (commit) {
        inst->timer_origin = origin;
        inst->timer_counter = counter;
        inst->timer_reload = reload;
        inst->timer_enabled = enabled;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    /* no origin can wrap the closed form (it only subtracts the origin from a later stamp when that is above it), but a reachable origin is never after the machine time: every write that sets it uses a stamp inside the instruction that
     * did the write and ds_timer_settle only moves it to a step boundary at or below the stamp it is given, so between steps origin <= machine time, enabled or not (a stopped timer keeps its stale origin). An origin after the time of the
     * same stream is a machine that never counts or borrows and that no guest can reach: refused as a field outside its domain (WR-03) */
    return counter > 127u || reload > 127u || enabled > 1u || origin > v->time ? (uint32_t)DS_SNAPREF_field : (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_irqc(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t disable = r_u8(r);
    uint8_t tiq = r_u8(r);
    uint8_t sample = r_u8(r);

    (void)v;
    if (commit) {
        inst->irq_disable = disable;
        inst->tiq_latch = tiq;
        inst->irq_i_sample = sample;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    if (((uint32_t)disable & ~SNAP_IRQ_REG_MASK) != 0u || tiq > 1u || (uint32_t)sample >= model_values((uint32_t)DS_MODEL_irq_i_sample)) {
        return (uint32_t)DS_SNAPREF_field;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_irqs(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t slot;
    uint64_t prev = UINT64_C(0);
    uint32_t unused_seen = 0u;
    uint32_t used = 0u;

    (void)v;
    for (slot = 0u; slot < DS_IRQ_STIM_MAX; slot++) {
        uint64_t at = r_u64(r);
        uint8_t line = r_u8(r);
        uint8_t level = r_u8(r);

        if (commit) {
            inst->irq_stim_at[slot] = at;
            inst->irq_stim_line[slot] = line;
            inst->irq_stim_level[slot] = level;
            continue;
        }
        if (line == 0u) { /* an unused slot is all zero, and no used slot follows one */
            if (at != UINT64_C(0) || level != 0u) {
                return (uint32_t)DS_SNAPREF_field;
            }
            unused_seen = 1u;
            continue;
        }
        if (unused_seen != 0u || line > 2u || level > 1u || (used != 0u && at < prev)) { /* a declared list: lines 1 and 2, levels 0 and 1, times that do not decrease */
            return (uint32_t)DS_SNAPREF_field;
        }
        prev = at;
        used++;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_blkt(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t active = r_u8(r);
    uint8_t opcode = r_u8(r);
    uint8_t toggle = r_u8(r);
    uint32_t remaining = r_u32(r);
    uint16_t src = r_u16(r);
    uint16_t dst = r_u16(r);

    (void)v;
    if (commit) {
        inst->blk_active = active;
        inst->blk_opcode = opcode;
        inst->blk_toggle = toggle;
        inst->blk_remaining = remaining;
        inst->blk_src = src;
        inst->blk_dst = dst;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    if (active > 1u) {
        return (uint32_t)DS_SNAPREF_field;
    }
    if (active == 0u) { /* no transfer: every field is 0 */
        return opcode != 0u || toggle != 0u || remaining != 0u || src != 0u || dst != 0u ? (uint32_t)DS_SNAPREF_field : (uint32_t)DS_SNAPREF_NONE;
    }
    /* a transfer in progress: TII, TDD, TIN, TIA or TAI; the alternation state is 0 or 1 and is always 0 for the three that do not alternate; 0 bytes left is legal (the epilogue is next), 65536 is the most */
    if ((opcode != 0x73u && opcode != 0xC3u && opcode != 0xD3u && opcode != 0xE3u && opcode != 0xF3u) || toggle > 1u || (toggle != 0u && opcode != 0xE3u && opcode != 0xF3u) || remaining > SNAP_BLK_MAX) {
        return (uint32_t)DS_SNAPREF_field;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_iopt(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t latch = r_u8(r);
    uint8_t kind[DS_PORT_COUNT];
    uint32_t port;
    uint8_t port1;

    for (port = 0u; port < DS_PORT_COUNT; port++) {
        kind[port] = r_u8(r);
    }
    if (commit) {
        inst->o_latch = latch;
        for (port = 0u; port < DS_PORT_COUNT; port++) {
            inst->slot_type[port] = kind[port];
        }
        return (uint32_t)DS_SNAPREF_NONE;
    }
    /* port 1 holds the pad that the profile names and the other four ports are empty, as ds_io_poweron leaves them (r1 populates port 1 only); nothing changes a slot after boot */
    port1 = ds_streq(inst->profile.field[DS_FIELD_input_port1].value, "pad2") ? (uint8_t)DS_SLOT_PAD2 : (uint8_t)DS_SLOT_NONE;
    if (kind[0] != port1) {
        return (uint32_t)DS_SNAPREF_field;
    }
    for (port = 1u; port < DS_PORT_COUNT; port++) {
        if (kind[port] != (uint8_t)DS_SLOT_NONE) {
            return (uint32_t)DS_SNAPREF_field;
        }
    }
    for (port = 0u; port < DS_PORT_COUNT; port++) {
        v->slot_type[port] = kind[port];
    }
    return (uint32_t)DS_SNAPREF_NONE; /* the latch of port O is any byte */
}

static uint32_t dec_schd(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    struct rd tail;
    uint32_t count;
    uint32_t reserved;
    uint32_t slot;
    uint64_t prev_at = UINT64_C(0);
    uint32_t prev_port = 0u;

    tail.p = r->p + (size_t)DS_INPUT_MAX * 16u; /* the count and the reserved word follow the slots; the validation needs the count first */
    count = r_u32(&tail);
    reserved = r_u32(&tail);
    if (!commit) {
        if (count > DS_INPUT_MAX) {
            return (uint32_t)DS_SNAPREF_field;
        }
        if (reserved != 0u) {
            return (uint32_t)DS_SNAPREF_reserved;
        }
    }
    for (slot = 0u; slot < DS_INPUT_MAX; slot++) {
        uint64_t at = r_u64(r);
        uint32_t port = r_u32(r);
        uint32_t mask = r_u32(r);

        if (commit) {
            inst->input_ev[slot].at = at;
            inst->input_ev[slot].port = port;
            inst->input_ev[slot].mask = mask;
            continue;
        }
        if (slot >= count) { /* an unused slot is all zero */
            if (at != UINT64_C(0) || port != 0u || mask != 0u) {
                return (uint32_t)DS_SNAPREF_field;
            }
            continue;
        }
        /* a declared event: a port with a pad behind it (judged against the device kinds of the same stream), a mask of the two-button pad with no opposite directions, and the order of ds_input_push (times do not decrease; an equal time needs a greater port) */
        if (port < 1u || port > DS_PORT_COUNT || v->slot_type[port - 1u] != (uint8_t)DS_SLOT_PAD2) {
            return (uint32_t)DS_SNAPREF_field;
        }
        if ((mask & ~(uint32_t)SNAP_PAD_BITS) != 0u || (mask & SNAP_UP_DOWN) == SNAP_UP_DOWN || (mask & SNAP_LEFT_RIGHT) == SNAP_LEFT_RIGHT) {
            return (uint32_t)DS_SNAPREF_field;
        }
        if (slot > 0u && (at < prev_at || (at == prev_at && port <= prev_port))) {
            return (uint32_t)DS_SNAPREF_field;
        }
        prev_at = at;
        prev_port = port;
    }
    r->p += 8; /* the count and the reserved word, read above through the second reader */
    if (commit) {
        inst->input_count = count;
        inst->input_reserved = reserved;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

/* the register masks of the VDC (D-146, in the order of R00 to R13): a stored register has every bit outside its mask 0; R03 and R04 are not registers and stay 0 */
static const uint16_t vdc_reg_mask[20] = {0xFFFFu, 0xFFFFu, 0xFFFFu, 0x0000u, 0x0000u, 0x1FFFu, 0x03FFu, 0x03FFu, 0x01FFu, 0x00FFu, 0x7F1Fu, 0x7F7Fu, 0xFF1Fu, 0x01FFu, 0x00FFu, 0x001Fu, 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu};

static uint32_t dec_vce0(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t cr = r_u8(r);
    uint16_t cta = r_u16(r);

    (void)v;
    if (commit) {
        inst->vce_cr = cr;
        inst->vce_cta = cta;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    return (uint32_t)cta > SNAP_VCE_ENTRY_MAX ? (uint32_t)DS_SNAPREF_field : (uint32_t)DS_SNAPREF_NONE; /* the control register is any byte */
}

static uint32_t dec_palt(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t i;

    (void)v;
    for (i = 0u; i < DS_PAL_BYTES; i++) {
        uint8_t b = r_u8(r);

        if (commit) {
            inst->vce_pal[i] = b;
        } else if ((i & 1u) != 0u && (uint32_t)b > (SNAP_VCE_ENTRY_MAX >> 8)) { /* entry i / 2 is the u16 of bytes 2i and 2i + 1: at most 9 bits, so the odd byte is 0 or 1 */
            return (uint32_t)DS_SNAPREF_field;
        }
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_vdc0(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t ar = r_u8(r);
    uint8_t status = r_u8(r);
    uint16_t reg[20];
    uint16_t rdbuf;
    uint8_t wlatch;
    uint32_t i;

    (void)v;
    for (i = 0u; i < 20u; i++) {
        reg[i] = r_u16(r);
    }
    rdbuf = r_u16(r);
    wlatch = r_u8(r);
    if (commit) {
        inst->vdc_ar = ar;
        inst->vdc_status = status;
        for (i = 0u; i < 20u; i++) {
            inst->vdc_reg[i] = reg[i];
        }
        inst->vdc_rdbuf = rdbuf;
        inst->vdc_wlatch = wlatch;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    if ((uint32_t)ar > SNAP_VDC_AR_MAX || (uint32_t)status > SNAP_VDC_STATUS_MAX) {
        return (uint32_t)DS_SNAPREF_field;
    }
    for (i = 0u; i < 20u; i++) {
        if (((uint32_t)reg[i] & ~(uint32_t)vdc_reg_mask[i]) != 0u) {
            return (uint32_t)DS_SNAPREF_field;
        }
    }
    return (uint32_t)DS_SNAPREF_NONE; /* the read buffer and the write latch are any value */
}

static uint32_t dec_beam(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint64_t line_idx = r_u64(r);
    uint8_t stage = r_u8(r);
    uint8_t v_state = r_u8(r);
    uint16_t v_togo = r_u16(r);
    uint16_t display_k = r_u16(r);
    uint16_t rcr_counter = r_u16(r);
    uint16_t frame_lines = r_u16(r);
    uint16_t bg_y = r_u16(r);
    uint16_t mwr_active = r_u16(r);
    uint8_t burst = r_u8(r);
    uint8_t line_dirty = r_u8(r);
    uint8_t sprite_pend = r_u8(r);
    uint16_t vd_off = r_u16(r);
    uint16_t rcr_off = r_u16(r);
    uint8_t lat_div = r_u8(r);
    uint8_t vd_pending = r_u8(r);
    uint8_t vd_irq_phase = r_u8(r);
    uint8_t rcr_origin = r_u8(r);

    if (commit) {
        inst->line_idx = line_idx;
        inst->stage = stage;
        inst->v_state = v_state;
        inst->v_togo = v_togo;
        inst->display_k = display_k;
        inst->rcr_counter = rcr_counter;
        inst->frame_lines = frame_lines;
        inst->bg_y = bg_y;
        inst->mwr_active = mwr_active;
        inst->burst = burst;
        inst->line_dirty = line_dirty;
        inst->sprite_pend = sprite_pend;
        inst->vd_off = vd_off;
        inst->rcr_off = rcr_off;
        inst->lat_div = lat_div;
        inst->vd_pending = vd_pending;
        inst->vd_irq_phase = vd_irq_phase;
        inst->rcr_origin = rcr_origin;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    /* the line count is judged against the machine time of the same stream (v->time is set by dec_time in pass 1 only, F-34a), before any multiplication by the line length (D-166: a count far ahead of the time would make the next fold loop for as many lines) */
    if (line_idx > v->time / (uint64_t)DS_CONST_line_mclk + UINT64_C(2)) {
        return (uint32_t)DS_SNAPREF_field;
    }
    if ((uint32_t)stage > 2u || (uint32_t)v_state > 3u || (uint32_t)v_togo > SNAP_V_TOGO_MAX) {
        return (uint32_t)DS_SNAPREF_field;
    }
    if (v_state == 1u ? (uint32_t)display_k >= 512u : (uint32_t)display_k != SNAP_BEAM_NONE) { /* a display line inside VDW, 0xFFFF everywhere else */
        return (uint32_t)DS_SNAPREF_field;
    }
    if ((uint32_t)rcr_counter > SNAP_BEAM_LINE_MAX || (frame_lines != 262u && frame_lines != 263u) || (uint32_t)bg_y > 511u || (uint32_t)mwr_active > 0xFFu) {
        return (uint32_t)DS_SNAPREF_field;
    }
    if (burst > 1u || (uint32_t)line_dirty > 0x3Fu || (uint32_t)sprite_pend > 3u || vd_pending > 1u) {
        return (uint32_t)DS_SNAPREF_field;
    }
    if (((uint32_t)vd_off >= (uint32_t)DS_CONST_line_mclk && (uint32_t)vd_off != SNAP_BEAM_NONE) || ((uint32_t)rcr_off >= (uint32_t)DS_CONST_line_mclk && (uint32_t)rcr_off != SNAP_BEAM_NONE)) {
        return (uint32_t)DS_SNAPREF_field;
    }
    /* the divider latched at EV_LINE multiplies vd_off, rcr_off and dma_word_mclk: 2, 3 or 4, or 0 before the first line event (line_idx 0), so no restored state reaches a zero divisor */
    if (lat_div == 0u ? line_idx != UINT64_C(0) : (lat_div < 2u || lat_div > 4u)) {
        return (uint32_t)DS_SNAPREF_field;
    }
    if ((uint32_t)vd_irq_phase >= model_values((uint32_t)DS_MODEL_vd_irq_phase) || (uint32_t)rcr_origin >= model_values((uint32_t)DS_MODEL_rcr_origin)) {
        return (uint32_t)DS_SNAPREF_field;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_vdma(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint8_t satb_pending = r_u8(r);
    uint8_t satb_done_valid = r_u8(r);
    uint64_t satb_done_at = r_u64(r);
    uint8_t dma_state = r_u8(r);
    uint64_t dma_start = r_u64(r);
    uint16_t dma_word_mclk = r_u16(r);
    uint32_t dma_done = r_u32(r);

    (void)v;
    if (commit) {
        inst->satb_pending = satb_pending;
        inst->satb_done_valid = satb_done_valid;
        inst->satb_done_at = satb_done_at;
        inst->dma_state = dma_state;
        inst->dma_start = dma_start;
        inst->dma_word_mclk = dma_word_mclk;
        inst->dma_done = dma_done;
        return (uint32_t)DS_SNAPREF_NONE;
    }
    if (satb_pending > 1u || satb_done_valid > 1u || dma_state > 2u || dma_done > SNAP_DMA_WORDS_MAX) {
        return (uint32_t)DS_SNAPREF_field;
    }
    /* the master clocks per word are 4 dots of the divider latched at the start (2, 3 or 4): the divisor of the closed form of the transfer, so a transfer in progress never has any other (D-151); the time windows of satb_done_at and dma_start land with the fold (plan 04-04) */
    if (dma_state != 0u && dma_word_mclk != 8u && dma_word_mclk != 12u && dma_word_mclk != 16u) {
        return (uint32_t)DS_SNAPREF_field;
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_vram(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t i;

    (void)v;
    if (!commit) {
        r->p += DS_VRAM_BYTES; /* every byte value is a legal VRAM byte */
        return (uint32_t)DS_SNAPREF_NONE;
    }
    for (i = 0u; i < DS_VRAM_BYTES; i++) {
        inst->vdc_vram[i] = r_u8(r);
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

static uint32_t dec_satb(struct ds_instance *inst, struct vctx *v, struct rd *r, int commit)
{
    uint32_t i;

    (void)v;
    if (!commit) {
        r->p += DS_SATB_BYTES; /* every byte value is a legal SATB byte */
        return (uint32_t)DS_SNAPREF_NONE;
    }
    for (i = 0u; i < DS_SATB_BYTES; i++) {
        inst->vdc_satb[i] = r_u8(r);
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

/* the decoders in the order of the region table of serial.c; the tag is checked against that table at every step, so a table that drifts from the stream fails closed */
static const struct {
    const char tag[5];
    dec_fn fn;
} decoders[DS_REGION_COUNT] = {
    {"PROF", dec_prof}, {"CONT", dec_cont}, {"POWR", dec_powr}, {"TIME", dec_time}, {"RAM8", dec_ram8}, {"MPRS", dec_mprs}, {"CPU0", dec_cpu0},
    {"BUS0", dec_bus0}, {"TIMR", dec_timr}, {"IRQC", dec_irqc}, {"IRQS", dec_irqs}, {"BLKT", dec_blkt}, {"IOPT", dec_iopt}, {"SCHD", dec_schd},
    {"VCE0", dec_vce0}, {"PALT", dec_palt}, {"VDC0", dec_vdc0}, {"BEAM", dec_beam}, {"VDMA", dec_vdma}, {"VRAM", dec_vram}, {"SATB", dec_satb},
};

/* one walk over the stream: pass 1 (commit 0) checks the prefix, the version, every frame and every field; pass 2 (commit 1) repeats the walk and stores. The frame checks are made in both passes, but in pass 2 they cannot fail: pass 1 has accepted these same bytes. */
static uint32_t walk(struct ds_instance *inst, const uint8_t *buf, int commit)
{
    struct rd r;
    struct vctx v;
    uint32_t prefix_len = 0u;
    const char *prefix = ds_stream_prefix(&prefix_len);
    uint32_t i;
    uint32_t k;

    r.p = buf;
    for (k = 0u; k < DS_PORT_COUNT; k++) {
        v.slot_type[k] = 0u;
    }
    v.time = 0u;
    for (i = 0u; i < prefix_len; i++) {
        if (r_u8(&r) != (uint8_t)prefix[i]) {
            return (uint32_t)DS_SNAPREF_prefix;
        }
    }
    if (r_u32(&r) != DS_SERIAL_VERSION) {
        return (uint32_t)DS_SNAPREF_version;
    }
    for (i = 0u; i < DS_REGION_COUNT; i++) {
        const char *tag = NULL;
        uint32_t want_len = 0u;
        uint32_t code;

        if (ds_region_frame(i, &tag, &want_len) == 0u) {
            return (uint32_t)DS_SNAPREF_tag;
        }
        for (k = 0u; k < 4u; k++) {
            if (r_u8(&r) != (uint8_t)tag[k] || (uint8_t)decoders[i].tag[k] != (uint8_t)tag[k]) {
                return (uint32_t)DS_SNAPREF_tag;
            }
        }
        if (r_u32(&r) != want_len) {
            return (uint32_t)DS_SNAPREF_region_length;
        }
        code = decoders[i].fn(inst, &v, &r, commit);
        if (code != (uint32_t)DS_SNAPREF_NONE) {
            return code;
        }
    }
    return (uint32_t)DS_SNAPREF_NONE;
}

uint32_t ds_restore(struct ds_instance *inst, const uint8_t *buf, size_t len)
{
    size_t size;
    uint32_t code;

    if (inst == NULL || buf == NULL) {
        return (uint32_t)DS_SNAPREF_argument;
    }
    if (ranges_overlap((uintptr_t)inst, sizeof *inst, (uintptr_t)buf, len)) {
        return (uint32_t)DS_SNAPREF_overlap;
    }
    size = ds_snapshot_size();
    if (len < size) {
        return (uint32_t)DS_SNAPREF_truncated;
    }
    if (len > size) {
        return (uint32_t)DS_SNAPREF_trailing;
    }
    code = walk(inst, buf, 0); /* pass 1: validate every byte, write nothing */
    if (code != (uint32_t)DS_SNAPREF_NONE) {
        return code;
    }
    (void)walk(inst, buf, 1); /* pass 2: commit; the same bytes that pass 1 accepted */
    return (uint32_t)DS_SNAPREF_NONE;
}
