/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * serial.c - the canonical state stream, the state digest and the profile identity hash (CORE-01, PROF-03; D-52, D-27, D-83, D-121).
 * The digest is SHA-256 over a framed stream, never over the bytes of a struct (padding, pointers and layout would leak in, and a
 * field could be added to the struct and silently missed). The stream is the prefix "duospool-state" and a NUL byte, the serializer
 * version as a u32, and then the regions of the table below in order, each framed as its four-byte ASCII tag, a u32 payload length and
 * the payload. Every integer is written explicitly little-endian, byte by byte, from unsigned operands widened before every shift:
 * the sanitize preset has no -fwrapv. The ROM bytes are not in the stream (the content id is the SHA-256 of the copy and stands for
 * them), and no pointer, padding, wall time or path enters it.
 * One region table serves the digest, ds_digest_stream and the ds_region_info test accessor, so a test that walks the accessor walks
 * exactly the regions the digest has. A region that is listed but whose payload the stream does not carry is what the digest_region
 * case and the drop-region toy control exist to catch. A matching digest is evidence of repeatability only, never of correctness.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_serial.h"
#include "ds_sha256.h"

/* ---- the stream writer: u8-sized pieces, little-endian integers, length-prefixed strings ---- */

struct writer {
    ds_sink_fn sink;
    void *ctx;
};

static void w_bytes(struct writer *w, const uint8_t *p, size_t n)
{
    w->sink(w->ctx, p, n);
}

static void w_u8(struct writer *w, uint8_t v)
{
    w_bytes(w, &v, 1u);
}

static void w_u16(struct writer *w, uint16_t v)
{
    uint8_t b[2];

    b[0] = (uint8_t)(v & 0xffu);
    b[1] = (uint8_t)((v >> 8) & 0xffu);
    w_bytes(w, b, sizeof b);
}

static void w_u32(struct writer *w, uint32_t v)
{
    uint8_t b[4];

    b[0] = (uint8_t)(v & 0xffu);
    b[1] = (uint8_t)((v >> 8) & 0xffu);
    b[2] = (uint8_t)((v >> 16) & 0xffu);
    b[3] = (uint8_t)((v >> 24) & 0xffu);
    w_bytes(w, b, sizeof b);
}

static void w_u64(struct writer *w, uint64_t v)
{
    uint8_t b[8];
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        b[i] = (uint8_t)((v >> (8u * i)) & UINT64_C(0xff));
    }
    w_bytes(w, b, sizeof b);
}

/* a string is its length as a u32 and then its bytes without the NUL; the read stops at cap, so an unterminated array cannot be overrun */
static void w_str(struct writer *w, const char *s, uint32_t cap)
{
    uint32_t n = 0u;

    while (n < cap && s[n] != '\0') {
        n++;
    }
    w_u32(w, n);
    w_bytes(w, (const uint8_t *)s, (size_t)n);
}

static void sha_sink(void *ctx, const uint8_t *data, size_t len)
{
    ds_sha256_update((struct ds_sha256 *)ctx, data, len);
}

/* ---- the profile identity hash ---- */

#define DS_PROFILE_HASH_VERSION 1u
#define DS_NAME_CAP 64u

static const char profile_prefix[] = "duospool-profile"; /* the stream writes this and its NUL (sizeof counts the NUL) */

void ds_profile_hash(const struct ds_profile *profile, uint8_t out[DS_DIGEST_BYTES])
{
    struct ds_sha256 st;
    struct writer w;
    uint32_t i;

    if (out == NULL) {
        return;
    }
    if (profile == NULL) {
        for (i = 0u; i < DS_DIGEST_BYTES; i++) {
            out[i] = 0u;
        }
        return;
    }
    ds_sha256_init(&st);
    w.sink = sha_sink;
    w.ctx = &st;
    w_bytes(&w, (const uint8_t *)profile_prefix, sizeof profile_prefix);
    w_u32(&w, DS_PROFILE_HASH_VERSION);
    w_str(&w, profile->id, DS_PROFILE_ID_MAX);
    w_u32(&w, profile->rev);
    w_u32(&w, (uint32_t)DS_FIELD_COUNT);
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        w_str(&w, ds_field_name(i), DS_NAME_CAP);
        w_u32(&w, (uint32_t)profile->field[i].status);
        w_str(&w, profile->field[i].value, DS_FIELD_VALUE_MAX);
        w_u32(&w, (uint32_t)profile->field[i].bound_by);
    }
    /* the unresolved behaviours are a property of the registered (id, rev); their ids are listed in sorted order */
    w_u32(&w, ds_profile_unresolved_count());
    for (i = 0u; i < ds_profile_unresolved_count(); i++) {
        w_str(&w, ds_profile_unresolved(i), DS_NAME_CAP);
    }
    ds_sha256_final(&st, out);
}

/* ---- the region table ---- */

enum region_id { R_PROF, R_CONT, R_POWR, R_TIME, R_RAM8, R_MPRS, R_CPU0, R_BUS0, R_TIMR, R_IRQC, R_IRQS, R_BLKT, R_IOPT, R_SCHD, R_VCE0, R_PALT, R_VDC0, R_BEAM, R_VDMA, R_VRAM, R_SATB, R_COUNT };

struct region {
    char tag[5];            /* four ASCII bytes and a NUL, which is not written */
    uint32_t payload_len;   /* the length written in the frame; the payload must be exactly this long */
    size_t live_off;        /* the live storage whose first byte the payload carries (test accessor) */
    size_t live_len;
};

/* the power-on policy, the has_seed flag and the seed are neighbours in the instance: the accessor spans them as one live range */
_Static_assert(offsetof(struct ds_instance, has_seed) == offsetof(struct ds_instance, powerup_policy) + sizeof(uint32_t),
               "powerup_policy and has_seed are not adjacent");
_Static_assert(offsetof(struct ds_instance, powerup_seed) == offsetof(struct ds_instance, has_seed) + sizeof(uint32_t),
               "has_seed and powerup_seed are not adjacent");

/* the CPU registers are neighbours in the instance (pc, a, x, y, s, p, speed, no padding): the accessor spans them as one live range of eight bytes */
_Static_assert(offsetof(struct ds_instance, a) == offsetof(struct ds_instance, pc) + sizeof(uint16_t), "pc and a are not adjacent");
_Static_assert(offsetof(struct ds_instance, x) == offsetof(struct ds_instance, a) + 1u, "a and x are not adjacent");
_Static_assert(offsetof(struct ds_instance, y) == offsetof(struct ds_instance, x) + 1u, "x and y are not adjacent");
_Static_assert(offsetof(struct ds_instance, s) == offsetof(struct ds_instance, y) + 1u, "y and s are not adjacent");
_Static_assert(offsetof(struct ds_instance, p) == offsetof(struct ds_instance, s) + 1u, "s and p are not adjacent");
_Static_assert(offsetof(struct ds_instance, speed) == offsetof(struct ds_instance, p) + 1u, "p and speed are not adjacent");

/* the bus state is three neighbours in the instance (io_buffer, mpr_latch, tma_zero): the accessor spans them as one live range of three bytes */
_Static_assert(offsetof(struct ds_instance, mpr_latch) == offsetof(struct ds_instance, io_buffer) + 1u, "io_buffer and mpr_latch are not adjacent");
_Static_assert(offsetof(struct ds_instance, tma_zero) == offsetof(struct ds_instance, mpr_latch) + 1u, "mpr_latch and tma_zero are not adjacent");

/*
 * The timer, the interrupt controller and the declared stimulus (plan 03-22; D-121): each region is one live span of the instance, in the order of the declarations of ds_core.h, whose adjacency struct ds_instance asserts. TIMR is timer_origin
 * (u64) and then timer_counter, timer_reload and timer_enabled (11 bytes); IRQC is irq_disable, tiq_latch and the selection of the model parameter irq_i_sample (3 bytes); IRQS is the 16 slots of the declared IRQ1 and IRQ2 stimulus, each the time (u64),
 * the line (0 for an unused slot) and the level (160 bytes; the slots are in struct-of-arrays storage, the stream interleaves them per slot, and an unused slot is all zero by construction of ds_irq_stim_set).
 */
_Static_assert(offsetof(struct ds_instance, timer_enabled) + 1u - offsetof(struct ds_instance, timer_origin) == 11u, "the timer fields are not the 11 bytes of the TIMR payload");
_Static_assert(offsetof(struct ds_instance, irq_i_sample) + 1u - offsetof(struct ds_instance, irq_disable) == 3u, "the controller bytes are not the 3 bytes of the IRQC payload");
_Static_assert(offsetof(struct ds_instance, irq_stim_level) + DS_IRQ_STIM_MAX - offsetof(struct ds_instance, irq_stim_at) == DS_IRQ_STIM_MAX * (sizeof(uint64_t) + 2u), "the stimulus arrays are not the 160 bytes of the IRQS payload");

/*
 * The block transfer in progress (plan 03-25; D-92, D-100, D-121): region BLKT is the 11 bytes blk_active, blk_opcode, blk_toggle, blk_remaining (u32), blk_src (u16) and blk_dst (u16), one live span in the order of the declarations of ds_core.h,
 * whose adjacency struct ds_instance asserts; every field is 0 when no transfer is active. The remaining count is 0 to 65536, so the high byte of its u32 is the 17th bit.
 */
_Static_assert(offsetof(struct ds_instance, blk_dst) + sizeof(uint16_t) - offsetof(struct ds_instance, blk_active) == 11u, "the block transfer fields are not the 11 bytes of the BLKT payload");

/*
 * The I/O port bytes and the declared input schedule (plan 03-29; D-116, D-117, D-121, INP-04): region IOPT is the 6 bytes o_latch and slot_type[0..4] (the latch of port O, then the device on each of the five ports), one live span in the order of the declarations of ds_core.h; region SCHD is the whole schedule, always
 * 16392 bytes: 1024 slots of 16 bytes, each the time (u64), the port (u32) and the button mask (u32), then the event count (u32) and the reserved word (u32, always 0). An unused slot is all zero by construction of ds_input_push, so the payload is a function of the declared list alone. The live span of SCHD is input_ev, input_count and input_reserved,
 * which struct ds_instance asserts are adjacent; the struct has no padding inside an event (a static assert), and the stream writes each field explicitly little-endian.
 */
_Static_assert(offsetof(struct ds_instance, slot_type) + DS_PORT_COUNT - offsetof(struct ds_instance, o_latch) == 6u, "the I/O port bytes are not the 6 bytes of the IOPT payload");
_Static_assert(offsetof(struct ds_instance, input_reserved) + sizeof(uint32_t) - offsetof(struct ds_instance, input_ev) == DS_INPUT_MAX * 16u + 8u, "the input schedule is not the 16392 bytes of the SCHD payload");
_Static_assert(offsetof(struct ds_input_event, at) == 0u && offsetof(struct ds_input_event, port) == 8u && offsetof(struct ds_input_event, mask) == 12u, "the fields of an input event are not at 0, 8 and 12");

/*
 * The VCE and VDC regions (plan 04-03; D-165): each is one live span of the instance in the order of the declarations of ds_core.h, and the instance has no padding (the tiling assert), so a span is its first member to the end of its last. VCE0 is vce_cr (u8) and vce_cta (u16): 3 bytes; PALT the 1,024 bytes of vce_pal; VDC0 is the 20 register slots (u16),
 * the read buffer (u16), the address register, the status and the write latch: 45 bytes; BEAM is 33 bytes (stage, v_state, burst, line_idx, v_togo ... rcr_origin: the stream order of D-165 is written field by field by emit_payload and differs from the order of the members); VDMA is 25 bytes (satb_pending, satb_done_valid, satb_done_at, dma_start, dma_done, dma_word_mclk, dma_state); VRAM the 65,536 bytes of vdc_vram; SATB the 512 bytes of vdc_satb.
 */
_Static_assert(offsetof(struct ds_instance, vce_cta) + sizeof(uint16_t) - offsetof(struct ds_instance, vce_cr) == 3u, "the VCE fields are not the 3 bytes of the VCE0 payload");
_Static_assert(offsetof(struct ds_instance, vdc_wlatch) + 1u - offsetof(struct ds_instance, vdc_reg) == 45u, "the VDC fields are not the 45 bytes of the VDC0 payload");
_Static_assert(offsetof(struct ds_instance, rcr_origin) + 1u - offsetof(struct ds_instance, stage) == 33u, "the beam fields are not the 33 bytes of the BEAM payload");
_Static_assert(offsetof(struct ds_instance, dma_state) + 1u - offsetof(struct ds_instance, satb_pending) == 25u, "the DMA fields are not the 25 bytes of the VDMA payload");
_Static_assert(offsetof(struct ds_instance, vdc_wlatch) + 1u == offsetof(struct ds_instance, stage) && offsetof(struct ds_instance, rcr_origin) + 1u == offsetof(struct ds_instance, satb_pending) && offsetof(struct ds_instance, dma_state) + 1u == offsetof(struct ds_instance, vce_cr),
               "the VDC0, BEAM, VDMA and VCE0 spans are not neighbours");
_Static_assert(offsetof(struct ds_instance, vce_cta) + sizeof(uint16_t) == offsetof(struct ds_instance, vce_pal) && offsetof(struct ds_instance, vce_pal) + DS_PAL_BYTES == offsetof(struct ds_instance, vdc_vram) && offsetof(struct ds_instance, vdc_vram) + DS_VRAM_BYTES == offsetof(struct ds_instance, vdc_satb),
               "the palette, VRAM and SATB arrays are not neighbours");

static const struct region regions[R_COUNT] = {
    [R_PROF] = {"PROF", DS_DIGEST_BYTES, offsetof(struct ds_instance, profile), sizeof(struct ds_profile)},
    [R_CONT] = {"CONT", DS_CONTENT_ID_BYTES + 4u + 4u, offsetof(struct ds_instance, content_id), DS_CONTENT_ID_BYTES},
    [R_POWR] = {"POWR", 4u + 8u + 4u, offsetof(struct ds_instance, powerup_policy),
                offsetof(struct ds_instance, powerup_seed) + sizeof(uint64_t) - offsetof(struct ds_instance, powerup_policy)},
    [R_TIME] = {"TIME", 8u, offsetof(struct ds_instance, machine_time), sizeof(uint64_t)},
    [R_RAM8] = {"RAM8", DS_RAM8K_BYTES, offsetof(struct ds_instance, ram8k), DS_RAM8K_BYTES},
    [R_MPRS] = {"MPRS", DS_MPR_COUNT, offsetof(struct ds_instance, mpr), DS_MPR_COUNT},
    [R_CPU0] = {"CPU0", 8u, offsetof(struct ds_instance, pc), offsetof(struct ds_instance, speed) + 1u - offsetof(struct ds_instance, pc)},
    [R_BUS0] = {"BUS0", 3u, offsetof(struct ds_instance, io_buffer), offsetof(struct ds_instance, tma_zero) + 1u - offsetof(struct ds_instance, io_buffer)},
    [R_TIMR] = {"TIMR", 11u, offsetof(struct ds_instance, timer_origin), offsetof(struct ds_instance, timer_enabled) + 1u - offsetof(struct ds_instance, timer_origin)},
    [R_IRQC] = {"IRQC", 3u, offsetof(struct ds_instance, irq_disable), offsetof(struct ds_instance, irq_i_sample) + 1u - offsetof(struct ds_instance, irq_disable)},
    [R_IRQS] = {"IRQS", DS_IRQ_STIM_MAX * (8u + 2u), offsetof(struct ds_instance, irq_stim_at), offsetof(struct ds_instance, irq_stim_level) + DS_IRQ_STIM_MAX - offsetof(struct ds_instance, irq_stim_at)},
    [R_BLKT] = {"BLKT", 11u, offsetof(struct ds_instance, blk_active), offsetof(struct ds_instance, blk_dst) + sizeof(uint16_t) - offsetof(struct ds_instance, blk_active)},
    [R_IOPT] = {"IOPT", 6u, offsetof(struct ds_instance, o_latch), offsetof(struct ds_instance, slot_type) + DS_PORT_COUNT - offsetof(struct ds_instance, o_latch)},
    [R_SCHD] = {"SCHD", DS_INPUT_MAX * 16u + 8u, offsetof(struct ds_instance, input_ev), offsetof(struct ds_instance, input_reserved) + sizeof(uint32_t) - offsetof(struct ds_instance, input_ev)},
    [R_VCE0] = {"VCE0", 3u, offsetof(struct ds_instance, vce_cr), offsetof(struct ds_instance, vce_cta) + sizeof(uint16_t) - offsetof(struct ds_instance, vce_cr)},
    [R_PALT] = {"PALT", DS_PAL_BYTES, offsetof(struct ds_instance, vce_pal), DS_PAL_BYTES},
    [R_VDC0] = {"VDC0", 45u, offsetof(struct ds_instance, vdc_reg), offsetof(struct ds_instance, vdc_wlatch) + 1u - offsetof(struct ds_instance, vdc_reg)},
    [R_BEAM] = {"BEAM", 33u, offsetof(struct ds_instance, stage), offsetof(struct ds_instance, rcr_origin) + 1u - offsetof(struct ds_instance, stage)},
    [R_VDMA] = {"VDMA", 25u, offsetof(struct ds_instance, satb_pending), offsetof(struct ds_instance, dma_state) + 1u - offsetof(struct ds_instance, satb_pending)},
    [R_VRAM] = {"VRAM", DS_VRAM_BYTES, offsetof(struct ds_instance, vdc_vram), DS_VRAM_BYTES},
    [R_SATB] = {"SATB", DS_SATB_BYTES, offsetof(struct ds_instance, vdc_satb), DS_SATB_BYTES},
};

/* the payload of one region: exactly regions[index].payload_len bytes, one explicit field per call, never a struct */
static void emit_payload(struct writer *w, const struct ds_instance *inst, uint32_t index)
{
    switch (index) {
    case R_PROF: {
        uint8_t h[DS_DIGEST_BYTES];

        ds_profile_hash(&inst->profile, h);
        w_bytes(w, h, sizeof h);
        break;
    }
    case R_CONT:
        w_bytes(w, inst->content_id, DS_CONTENT_ID_BYTES);
        w_u32(w, ds_size_bytes(inst->size_class));
        w_u32(w, inst->size_class);
        break;
    case R_POWR:
        w_u32(w, inst->powerup_policy);
        w_u64(w, inst->powerup_seed);
        w_u32(w, inst->has_seed);
        break;
    case R_TIME:
        w_u64(w, inst->machine_time);
        break;
    case R_RAM8:
        w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);
        break;
    case R_MPRS:
        w_bytes(w, inst->mpr, DS_MPR_COUNT);
        break;
    case R_CPU0: /* the stream order is a x y s p pc speed, one explicit field per line; pc is a u16 little-endian; B is never stored */
        w_u8(w, inst->a);
        w_u8(w, inst->x);
        w_u8(w, inst->y);
        w_u8(w, inst->s);
        w_u8(w, inst->p);
        w_u16(w, inst->pc);
        w_u8(w, inst->speed);
        break;
    case R_BUS0: /* io_buffer, mpr_latch (plan 03-16), then the selected value of the model parameter tma_zero as an index into its allowed list (plan 03-17, D-113, D-121), one explicit field per line */
        w_u8(w, inst->io_buffer);
        w_u8(w, inst->mpr_latch);
        w_u8(w, inst->tma_zero);
        break;
    case R_TIMR: /* the origin of the prescaler phase as a u64 little-endian, then counter, reload and the start bit (plan 03-21, 03-22; D-109, D-121), one explicit field per line */
        w_u64(w, inst->timer_origin);
        w_u8(w, inst->timer_counter);
        w_u8(w, inst->timer_reload);
        w_u8(w, inst->timer_enabled);
        break;
    case R_IRQC: /* the disable mask, the TIQ latch and the selection of the model parameter irq_i_sample as an index into its allowed list (plan 03-22; D-110, D-111, D-113, D-121) */
        w_u8(w, inst->irq_disable);
        w_u8(w, inst->tiq_latch);
        w_u8(w, inst->irq_i_sample);
        break;
    case R_IRQS: { /* the 16 slots of the declared stimulus, each the time (u64), the line and the level; an unused slot is zero (plan 03-22; D-112, D-121) */
        uint32_t slot;

        for (slot = 0u; slot < DS_IRQ_STIM_MAX; slot++) {
            w_u64(w, inst->irq_stim_at[slot]);
            w_u8(w, inst->irq_stim_line[slot]);
            w_u8(w, inst->irq_stim_level[slot]);
        }
        break;
    }
    case R_BLKT: /* the active flag, the opcode, the alternation state, the remaining count as a u32 and the source and destination addresses as u16 (plan 03-25; D-100, D-121), one explicit field per line */
        w_u8(w, inst->blk_active);
        w_u8(w, inst->blk_opcode);
        w_u8(w, inst->blk_toggle);
        w_u32(w, inst->blk_remaining);
        w_u16(w, inst->blk_src);
        w_u16(w, inst->blk_dst);
        break;
    case R_IOPT: { /* the latch of port O, then the device kind of each of the five ports (plan 03-29; D-116, D-121), one explicit field per line */
        uint32_t port;

        w_u8(w, inst->o_latch);
        for (port = 0u; port < DS_PORT_COUNT; port++) {
            w_u8(w, inst->slot_type[port]);
        }
        break;
    }
    case R_SCHD: { /* the 1024 slots of the declared input schedule, each the time (u64), the port (u32) and the mask (u32), then the count and the reserved word (plan 03-29; D-117, D-121) */
        uint32_t slot;

        for (slot = 0u; slot < DS_INPUT_MAX; slot++) {
            w_u64(w, inst->input_ev[slot].at);
            w_u32(w, inst->input_ev[slot].port);
            w_u32(w, inst->input_ev[slot].mask);
        }
        w_u32(w, inst->input_count);
        w_u32(w, inst->input_reserved);
        break;
    }
    case R_VCE0: /* the control register, then the colour table address as a u16 little-endian (plan 04-03; D-165) */
        w_u8(w, inst->vce_cr);
        w_u16(w, inst->vce_cta);
        break;
    case R_PALT:
        w_bytes(w, inst->vce_pal, DS_PAL_BYTES);
        break;
    case R_VDC0: { /* the address register, the status, the 20 register slots as u16, the read buffer as u16 and the write latch (D-165) */
        uint32_t reg;

        w_u8(w, inst->vdc_ar);
        w_u8(w, inst->vdc_status);
        for (reg = 0u; reg < 20u; reg++) {
            w_u16(w, inst->vdc_reg[reg]);
        }
        w_u16(w, inst->vdc_rdbuf);
        w_u8(w, inst->vdc_wlatch);
        break;
    }
    case R_BEAM: /* the stream order of D-165, one explicit field per line */
        w_u64(w, inst->line_idx);
        w_u8(w, inst->stage);
        w_u8(w, inst->v_state);
        w_u16(w, inst->v_togo);
        w_u16(w, inst->display_k);
        w_u16(w, inst->rcr_counter);
        w_u16(w, inst->frame_lines);
        w_u16(w, inst->bg_y);
        w_u16(w, inst->mwr_active);
        w_u8(w, inst->burst);
        w_u8(w, inst->line_dirty);
        w_u8(w, inst->sprite_pend);
        w_u16(w, inst->vd_off);
        w_u16(w, inst->rcr_off);
        w_u8(w, inst->lat_div);
        w_u8(w, inst->vd_pending);
        w_u8(w, inst->vd_irq_phase);
        w_u8(w, inst->rcr_origin);
        break;
    case R_VDMA: /* the SATB copy and the VRAM-to-VRAM transfer (D-165), one explicit field per line */
        w_u8(w, inst->satb_pending);
        w_u8(w, inst->satb_done_valid);
        w_u64(w, inst->satb_done_at);
        w_u8(w, inst->dma_state);
        w_u64(w, inst->dma_start);
        w_u16(w, inst->dma_word_mclk);
        w_u32(w, inst->dma_done);
        break;
    case R_VRAM:
        w_bytes(w, inst->vdc_vram, DS_VRAM_BYTES);
        break;
    case R_SATB:
        w_bytes(w, inst->vdc_satb, DS_SATB_BYTES);
        break;
    default:
        break;
    }
}

static const char stream_prefix[] = "duospool-state"; /* the stream writes this and its NUL */

void ds_digest_stream(const struct ds_instance *inst, ds_sink_fn sink, void *ctx)
{
    struct writer w;
    uint32_t i;

    if (inst == NULL || sink == NULL) {
        return;
    }
    w.sink = sink;
    w.ctx = ctx;
    w_bytes(&w, (const uint8_t *)stream_prefix, sizeof stream_prefix);
    w_u32(&w, DS_SERIAL_VERSION);
    for (i = 0u; i < (uint32_t)R_COUNT; i++) {
        w_bytes(&w, (const uint8_t *)regions[i].tag, 4u);
        w_u32(&w, regions[i].payload_len);
        emit_payload(&w, inst, i);
    }
}

uint32_t ds_digest(const struct ds_instance *inst, uint8_t out[DS_DIGEST_BYTES])
{
    struct ds_sha256 st;

    if (inst == NULL || out == NULL) {
        return 1u;
    }
    ds_sha256_init(&st);
    ds_digest_stream(inst, sha_sink, &st);
    ds_sha256_final(&st, out);
    return DS_OK;
}

_Static_assert((uint32_t)R_COUNT == DS_REGION_COUNT, "DS_REGION_COUNT of ds_serial.h is not the number of rows of the region table");

uint32_t ds_region_frame(uint32_t index, const char **tag, uint32_t *payload_len)
{
    if (tag == NULL || payload_len == NULL || index >= (uint32_t)R_COUNT) {
        return 0u;
    }
    *tag = regions[index].tag;
    *payload_len = regions[index].payload_len;
    return 1u;
}

const char *ds_stream_prefix(uint32_t *len)
{
    if (len != NULL) {
        *len = (uint32_t)sizeof stream_prefix;
    }
    return stream_prefix;
}

uint32_t ds_region_info(struct ds_instance *inst, uint32_t index, const char **tag, uint8_t **ptr, size_t *len)
{
    if (inst == NULL || tag == NULL || ptr == NULL || len == NULL || index >= (uint32_t)R_COUNT) {
        return 0u;
    }
    *tag = regions[index].tag;
    *ptr = (uint8_t *)inst + regions[index].live_off;
    *len = regions[index].live_len;
    return 1u;
}
