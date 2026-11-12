/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * vid.c - the raw video line record "DSVR v1" (plan 04-04; D-155, D-161, D-164, ER-13). One record per line of every frame, little-endian, written byte by byte from widened unsigned values; the header is 64 bytes:
 *   +00 u64 origin  +08 u32 frame  +12 u16 line  +14 u16 frame_lines  +16 u16 rec_bytes  +18 u16 line_mclk  +20 u8 div  +21 u8 vce_cr  +22 u16 active_x  +24 u16 active_w  +26 u16 border  +28 u16 active_k  +30 u8 v_state  +31 u8 flags
 *   +32 u8 line_dirty  +33 u8 slots_used  +34 u8 covering  +35 u8 version  +36 u16 rcr_counter  +38 u16 bxr_eff  +40 u16 byr_row  +42 u16 hsr  +44 u16 hdr  +46 u16 vpr  +48 u16 vdw  +50 u16 vcr  +52 u16 mwr_eff  +54 u16 vdc_cr  +56 u64 reserved (zero)
 * The payload (two u16 arrays of active_w entries, then zero padding to a multiple of 8) belongs to the plans that give the line an active window; a record of this plan is the header alone (rec_bytes 64, active_w 0, every colour of the line the border). Plan 04-08 fills v_state, active_k (the display line index, 0xFFFF off display), rcr_counter and the ACTIVE flag from the vertical machine.
 * Plan 04-08 fills v_state, active_k (the display line index inside VDW, 0xFFFF off display), rcr_counter and the ACTIVE flag from the vertical machine; the render still reads no VRAM and gives the line no active window.
 * The render function takes a CONST instance and a stack record and can write no machine state: the record is a pure function of the state at the line event (R1), and what the fold does with the record (emit or drop) changes nothing in the instance (D-164).
 * The layout is a project definition, not a hardware claim (LED-VID-0018, ER-13). The header offsets are asserted below.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_vdc.h"

/* the header offsets of D-161 and the width of each field; every field starts where the one before it ends, and the last ends at the header size */
#define OFF_ORIGIN 0u
#define OFF_FRAME 8u
#define OFF_LINE 12u
#define OFF_FRAME_LINES 14u
#define OFF_REC_BYTES 16u
#define OFF_LINE_MCLK 18u
#define OFF_DIV 20u
#define OFF_VCE_CR 21u
#define OFF_ACTIVE_X 22u
#define OFF_ACTIVE_W 24u
#define OFF_BORDER 26u
#define OFF_ACTIVE_K 28u
#define OFF_V_STATE 30u
#define OFF_FLAGS 31u
#define OFF_LINE_DIRTY 32u
#define OFF_SLOTS 33u
#define OFF_COVERING 34u
#define OFF_VERSION 35u
#define OFF_RCR 36u
#define OFF_BXR 38u
#define OFF_BYR 40u
#define OFF_HSR 42u
#define OFF_HDR 44u
#define OFF_VPR 46u
#define OFF_VDW 48u
#define OFF_VCR 50u
#define OFF_MWR 52u
#define OFF_VDC_CR 54u
#define OFF_RESERVED 56u
_Static_assert(OFF_FRAME == OFF_ORIGIN + 8u && OFF_LINE == OFF_FRAME + 4u && OFF_FRAME_LINES == OFF_LINE + 2u && OFF_REC_BYTES == OFF_FRAME_LINES + 2u && OFF_LINE_MCLK == OFF_REC_BYTES + 2u, "DSVR v1 header offsets (D-161)");
_Static_assert(OFF_DIV == OFF_LINE_MCLK + 2u && OFF_VCE_CR == OFF_DIV + 1u && OFF_ACTIVE_X == OFF_VCE_CR + 1u && OFF_ACTIVE_W == OFF_ACTIVE_X + 2u && OFF_BORDER == OFF_ACTIVE_W + 2u && OFF_ACTIVE_K == OFF_BORDER + 2u, "DSVR v1 header offsets (D-161)");
_Static_assert(OFF_V_STATE == OFF_ACTIVE_K + 2u && OFF_FLAGS == OFF_V_STATE + 1u && OFF_LINE_DIRTY == OFF_FLAGS + 1u && OFF_SLOTS == OFF_LINE_DIRTY + 1u && OFF_COVERING == OFF_SLOTS + 1u && OFF_VERSION == OFF_COVERING + 1u, "DSVR v1 header offsets (D-161)");
_Static_assert(OFF_RCR == OFF_VERSION + 1u && OFF_BXR == OFF_RCR + 2u && OFF_BYR == OFF_BXR + 2u && OFF_HSR == OFF_BYR + 2u && OFF_HDR == OFF_HSR + 2u && OFF_VPR == OFF_HDR + 2u, "DSVR v1 header offsets (D-161)");
_Static_assert(OFF_VDW == OFF_VPR + 2u && OFF_VCR == OFF_VDW + 2u && OFF_MWR == OFF_VCR + 2u && OFF_VDC_CR == OFF_MWR + 2u && OFF_RESERVED == OFF_VDC_CR + 2u && OFF_RESERVED + 8u == DS_VID_REC_HEADER_BYTES, "DSVR v1 header offsets (D-161)");

/* the record flag bits of D-161 that this plan sets */
#define FLAG_ACTIVE 0x01u
#define FLAG_BURST 0x02u

/* the vertical state VDW: a line in it is a display line (D-139) */
#define V_STATE_VDW 1u

/* the VDC register slots the header carries (D-146: the slot is the register number) */
#define REG_CR 5u
#define REG_BXR 7u
#define REG_HSR 10u
#define REG_HDR 11u
#define REG_VPR 12u
#define REG_VDW 13u
#define REG_VCR 14u

/* the palette entry that is the border (D-161: entry $100, the first colour of the sprite half) */
#define BORDER_ENTRY 0x100u

/* one little-endian store of n bytes of v at rec + off, byte by byte */
static void put_le(uint8_t *rec, uint32_t off, uint64_t v, uint32_t n)
{
    uint32_t i;

    for (i = 0u; i < n; i++) {
        rec[off + i] = (uint8_t)((v >> (8u * i)) & UINT64_C(0xFF));
    }
}

/* the 9-bit value of palette entry idx from its two stored bytes */
static uint32_t pal_entry(const struct ds_instance *inst, uint32_t idx)
{
    return ((uint32_t)inst->vce_pal[(idx << 1u) & (DS_PAL_BYTES - 1u)] | ((uint32_t)inst->vce_pal[((idx << 1u) + 1u) & (DS_PAL_BYTES - 1u)] << 8u)) & 0x1FFu;
}

uint32_t ds_vid_render(const struct ds_instance *inst, uint64_t origin, uint8_t *rec)
{
    uint64_t k = (origin - (uint64_t)DS_CONST_raster_phase_mclk) / (uint64_t)DS_CONST_line_mclk;
    uint64_t fl = (uint64_t)inst->frame_lines;
    const uint16_t *reg = inst->vdc_reg;
    int display = (uint32_t)inst->v_state == V_STATE_VDW;

    put_le(rec, OFF_ORIGIN, origin, 8u);
    put_le(rec, OFF_FRAME, fl != UINT64_C(0) ? k / fl : UINT64_C(0), 4u); /* the frame and the line in it are derived from the line index and the bound frame length, never stored (D-136) */
    put_le(rec, OFF_LINE, fl != UINT64_C(0) ? k % fl : UINT64_C(0), 2u);
    put_le(rec, OFF_FRAME_LINES, fl, 2u);
    put_le(rec, OFF_REC_BYTES, (uint64_t)DS_VID_REC_HEADER_BYTES, 2u); /* header only: no active window yet */
    put_le(rec, OFF_LINE_MCLK, (uint64_t)DS_CONST_line_mclk, 2u);
    put_le(rec, OFF_DIV, (uint64_t)inst->lat_div, 1u);
    put_le(rec, OFF_VCE_CR, (uint64_t)inst->vce_cr, 1u);
    put_le(rec, OFF_ACTIVE_X, UINT64_C(0), 2u);
    put_le(rec, OFF_ACTIVE_W, UINT64_C(0), 2u);
    put_le(rec, OFF_BORDER, (uint64_t)pal_entry(inst, BORDER_ENTRY), 2u);
    put_le(rec, OFF_ACTIVE_K, display ? (uint64_t)inst->display_k : (uint64_t)0xFFFFu, 2u); /* the display line index inside VDW, 0xFFFF on every other line (plan 04-08) */
    put_le(rec, OFF_V_STATE, (uint64_t)inst->v_state, 1u);
    put_le(rec, OFF_FLAGS, (display ? (uint64_t)FLAG_ACTIVE : UINT64_C(0)) | (inst->burst != 0u ? (uint64_t)FLAG_BURST : UINT64_C(0)), 1u);
    put_le(rec, OFF_LINE_DIRTY, (uint64_t)inst->line_dirty, 1u);
    put_le(rec, OFF_SLOTS, UINT64_C(0), 1u);
    put_le(rec, OFF_COVERING, UINT64_C(0), 1u);
    put_le(rec, OFF_VERSION, (uint64_t)DS_VIDREC_VERSION, 1u);
    put_le(rec, OFF_RCR, (uint64_t)inst->rcr_counter, 2u);
    put_le(rec, OFF_BXR, (uint64_t)reg[REG_BXR], 2u);
    put_le(rec, OFF_BYR, (uint64_t)inst->bg_y, 2u);
    put_le(rec, OFF_HSR, (uint64_t)reg[REG_HSR], 2u);
    put_le(rec, OFF_HDR, (uint64_t)reg[REG_HDR], 2u);
    put_le(rec, OFF_VPR, (uint64_t)reg[REG_VPR], 2u);
    put_le(rec, OFF_VDW, (uint64_t)reg[REG_VDW], 2u);
    put_le(rec, OFF_VCR, (uint64_t)reg[REG_VCR], 2u);
    put_le(rec, OFF_MWR, (uint64_t)inst->mwr_active, 2u);
    put_le(rec, OFF_VDC_CR, (uint64_t)reg[REG_CR], 2u);
    put_le(rec, OFF_RESERVED, UINT64_C(0), 8u);
    return DS_VID_REC_HEADER_BYTES;
}

/* the one place besides the want_pixels gate of the fold where the sink is touched (D-164): the whole record is offered, and the sink stores it or counts it lost */
void ds_vid_emit(struct ds_obs_log *obs, const uint8_t *rec, uint32_t rec_bytes)
{
    ds_obs_vid(obs, rec, rec_bytes);
}
