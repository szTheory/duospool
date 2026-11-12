/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * vid.c - the raw video line record "DSVR v1" (plan 04-04; D-155, D-161, D-164, ER-13). One record per line of every frame, little-endian, written byte by byte from widened unsigned values; the header is 64 bytes:
 *   +00 u64 origin  +08 u32 frame  +12 u16 line  +14 u16 frame_lines  +16 u16 rec_bytes  +18 u16 line_mclk  +20 u8 div  +21 u8 vce_cr  +22 u16 active_x  +24 u16 active_w  +26 u16 border  +28 u16 active_k  +30 u8 v_state  +31 u8 flags
 *   +32 u8 line_dirty  +33 u8 slots_used  +34 u8 covering  +35 u8 version  +36 u16 rcr_counter  +38 u16 bxr_eff  +40 u16 byr_row  +42 u16 hsr  +44 u16 hdr  +46 u16 vpr  +48 u16 vdw  +50 u16 vcr  +52 u16 mwr_eff  +54 u16 vdc_cr  +56 u64 reserved (zero)
 * The payload is two u16 arrays of active_w entries (the index, then the colour) and zero padding to a multiple of 8; a non-display line is the header alone (rec_bytes 64, active_w 0, active_x 0). Plan 04-08 fills v_state, active_k (the display line index inside VDW, 0xFFFF off display), rcr_counter and the ACTIVE flag from the vertical machine.
 * Plan 04-09 gives a display line its active window from the registers HSR and HDR and the divider latched at EV_LINE (D-140): active_x = (HDS+1)*8 and active_w = (HDW+1)*8 dots of that line's divider; a window that would end past the 1365 master clocks of the line is clipped to the whole dots that fit (possibly none, then active_x is 0) and the CLIPPED flag is set; nothing
 * is padded and no width is fixed anywhere.
 * Plan 04-11 adds the background (D-156, D-157, D-158). A dot of the active window takes its index from the frame state at the line event: in a burst frame (the latch of D-157, taken at display line 0 as !(BB || SB) and kept for the frame; BURST flag b1 on its display lines) every dot is index 256, the blank index; with BB = 0 and no burst it is DS_CONST_bg_off_index
 * (a sprite dot, below, replaces it); with BB = 1 it is the background: the BAT word of the dot (W x H characters from the SCREEN bits of the MWR latched at VBlank entry, static const table, row-major from VRAM word 0), the tile at word code * 16 with the planes of CG0 and CG1, px 0 giving index 0 whatever the bank and else (bank << 4) | px. The colour is the
 * palette entry of the index as it stands at the line event.
 * Pixel order is ASSUMED LED-VID-0010: low byte plane 0 or 2, high byte plane 1 or 3, the leftmost dot of a tile row is bit 7 (the one statement marked below); which plane carries which significance is page-stated (H7-18). Dot i takes virtual column (BXR + i) mod 8W and virtual row bg_y mod 8H, bg_y being the counter of beam.c and vdc.c (D-156). The flags b2 BB and b3 SB are
 * CR bits 7 and 6 at the line event; b4 UNMODELLED is set on every record while the latched MWR has VM or SM non-zero, whose
 * line is rendered as VM = 0 and SM = 0 (D-158). Every VRAM word is read through the bounds-checked accessor (an address out of range reads 0, T-04-38); the function reads the instance and writes the record only, and holds no table or line buffer of its own.
 * Plan 04.1-01 adds the sprites (D-155, D-159, D-160 priority part; MAN H7-13, H7-20 to H7-24). ds_vid_sprites is the STATE part of a display line with SB set: it scans the 64 SATB entries in index order (word 0 Y bits 9-0, word 1 X bits 9-0, word 2 pattern code bits 10-0, word 3 Y-flip 15, CGY 13-12, X-flip 11, CGX 8, SPBG 7, palette 3-0), counts the entries whose range
 * Y <= display_k + 64 < Y + height covers the line (height 16, 32, 64 for CGY 0, 1, 3 and DS_CONST_vid_cgy2_as for CGY 2) and puts for every visible dot the first opaque sprite pixel in SATB order into a stack array. A sprite pixel has the index 256 | palette << 4 | px, px 0 being transparent (H7-24; plane k is SGk at +16k words, H7-24), the pattern base word is
 * ((PC & ~m) >> 1) << 6 with m = CGX ? 2 : 0 | CGY 1 ? 4 : CGY 2 or 3 ? 12 : 0 (PC bits 10-1 are VRAM address bits 15-6, H7-21), cell (cx, cy) is at base + cx * $40 + cy * $80 and row r of a cell is at +r (H7-22, H7-23), and the leftmost pixel of a row is bit 15 (ASSUMED LED-VID-0011, the one statement marked below). A flip mirrors the whole
 * combined sprite. The dot X - 32 + t is drawn only if it lies in the active window (H7-13: the origin (32, 64) is the display top-left). The composition in ds_vid_render lets the sprite dot replace the background dot when its SPBG bit is 1 or the background dot is transparent (px 0, or BB off) (H7-21). Every VRAM word is read through the bounds-checked accessor; no slot
 * limit, overflow or collision exists yet (plan 04.1-02).
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
#define FLAG_BB 0x04u
#define FLAG_SB 0x08u
#define FLAG_UNMODELLED 0x10u
#define FLAG_CLIPPED 0x20u

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

/* the index of every dot of a blank active window (D-157: palette entry $100 is VD8 = 1, VD7-VD0 = 0) */
#define BLANK_INDEX 0x100u

/* the BAT size in characters from the SCREEN bits of the MWR (D-156; MAN H7-9): 000 32x32, 001 64x32, 010 and 011 128x32, 100 32x64, 101 64x64, 110 and 111 128x64. Every width and height is a power of two, so the wrap of a virtual column or row is a mask */
static const uint8_t bat_chars_w[8] = {32u, 64u, 128u, 128u, 32u, 64u, 128u, 128u};
static const uint8_t bat_chars_h[8] = {32u, 32u, 32u, 32u, 64u, 64u, 64u, 64u};

/* CR bits 7 (BB) and 6 (SB) and the MWR bits that make a mode this plan does not render (VM bits 1-0, SM bits 3-2) */
#define CR_BB 0x80u
#define CR_SB 0x40u
#define MWR_VM_SM 0x0Fu

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

/* the background index (VD8..VD0, VD8 = 0) of the dot at virtual column col and virtual row row of a BAT of w characters across (D-156): the BAT word at row / 8 * w + col / 8 holds the bank in bits 15-12 and the tile code in bits 11-0; the tile is 16 words at code * 16, CG0 in words 0 to 7 (planes 0 and 1) and CG1 in words 8 to 15 (planes 2 and 3), the row of the tile
 * being row & 7. ASSUMED LED-VID-0010: the low byte of a word is plane 0 (or 2), the high byte plane 1 (or 3), and the leftmost dot is bit 7. The index is 0 for px 0 whatever the bank and (bank << 4) | px otherwise (MAN H7-18). u32 arithmetic throughout; the accessor bounds every word (a tile code above 2047 addresses past the VRAM and reads 0) */
static uint32_t bg_index(const struct ds_instance *inst, uint32_t w, uint32_t col, uint32_t row)
{
    uint32_t bat = (uint32_t)ds_vdc_vram_read(inst, (row >> 3u) * w + (col >> 3u));
    uint32_t base = ((bat & 0x0FFFu) << 4u) + (row & 7u);
    uint32_t cg0 = (uint32_t)ds_vdc_vram_read(inst, base);
    uint32_t cg1 = (uint32_t)ds_vdc_vram_read(inst, base + 8u);
    uint32_t bit = 7u - (col & 7u);
    uint32_t px = ((cg0 >> bit) & 1u) | (((cg0 >> (bit + 8u)) & 1u) << 1u) | (((cg1 >> bit) & 1u) << 2u) | (((cg1 >> (bit + 8u)) & 1u) << 3u);

    return px == 0u ? 0u : (((bat >> 12u) & 0xFu) << 4u) | px;
}

/* the active window of a display line (D-140, D-141): x and w in dots of the divider div, from the HDS and HDW fields of HSR and HDR as they stand at EV_LINE. The window ends at (x + w) * div master clocks; when that is past the line it is clipped to the dots that fit whole, floor(line / div) - x of them or none, the clip is reported in *clipped, and a window with no dot has
 * x 0 (D-161). u32 throughout: at most (1024 + 1024) * 4 */
static void active_window(const struct ds_instance *inst, uint32_t *x, uint32_t *w, uint32_t *clipped)
{
    uint32_t hds = ((uint32_t)inst->vdc_reg[REG_HSR] >> 8u) & 0x7Fu;
    uint32_t hdw = (uint32_t)inst->vdc_reg[REG_HDR] & 0x7Fu;
    uint32_t div = (uint32_t)inst->lat_div != 0u ? (uint32_t)inst->lat_div : 1u;
    uint32_t ax = (hds + 1u) * 8u;
    uint32_t aw = (hdw + 1u) * 8u;
    uint32_t fit = (uint32_t)DS_CONST_line_mclk / div;

    *clipped = 0u;
    if ((ax + aw) * div > (uint32_t)DS_CONST_line_mclk) {
        *clipped = 1u;
        aw = fit > ax ? fit - ax : 0u;
    }
    if (aw > (uint32_t)DS_VID_MAX_DOTS) { /* the static bound of the record (T-04-31): unreachable after the clip, kept as a visible fact */
        aw = (uint32_t)DS_VID_MAX_DOTS;
    }
    *x = aw != 0u ? ax : 0u;
    *w = aw;
}

/* the CR bit 6 (SB) and the SATB geometry: 64 entries of 4 words; the sprite height in lines of a CGY class (MAN H7-23: 00 is 16, 01 is 32, 11 is 64; the class 2 is never looked up because a CGY field of 2 is replaced by DS_CONST_vid_cgy2_as first) */
#define SPR_ENTRIES 64u
#define SPR_ORIGIN_Y 64u
#define SPR_ORIGIN_X 32u
static const uint8_t spr_height[4] = {16u, 32u, 64u, 64u};

/* SATB word w of the 256 (byte 2w low, byte 2w+1 high; D-159) */
static uint32_t satb_word(const struct ds_instance *inst, uint32_t w)
{
    uint32_t b = (w & 0xFFu) << 1u;

    return (uint32_t)inst->vdc_satb[b] | ((uint32_t)inst->vdc_satb[b + 1u] << 8u);
}

void ds_vid_sprites(const struct ds_instance *inst, struct ds_vid_spr *spr)
{
    uint32_t ax = 0u;
    uint32_t aw = 0u;
    uint32_t clipped = 0u;
    uint32_t line = (uint32_t)inst->display_k + SPR_ORIGIN_Y;
    uint32_t s;
    uint32_t j;

    spr->covering = 0u;
    spr->on = 0u;
    if ((uint32_t)inst->v_state != V_STATE_VDW || ((uint32_t)inst->vdc_reg[REG_CR] & CR_SB) == 0u) { /* with SB clear, or off a display line, the sprite stage produces nothing (D-160) */
        return;
    }
    spr->on = 1u;
    active_window(inst, &ax, &aw, &clipped);
    for (j = 0u; j < aw; j++) {
        spr->dot[j] = 0u;
    }
    for (s = 0u; s < SPR_ENTRIES; s++) {
        uint32_t y = satb_word(inst, 4u * s) & 0x3FFu;
        uint32_t x = satb_word(inst, 4u * s + 1u) & 0x3FFu;
        uint32_t pc = satb_word(inst, 4u * s + 2u) & 0x7FFu;
        uint32_t attr = satb_word(inst, 4u * s + 3u);
        uint32_t cgy = (attr >> 12u) & 3u;
        uint32_t cgx = (attr >> 8u) & 1u;
        uint32_t yflip = (attr >> 15u) & 1u;
        uint32_t xflip = (attr >> 11u) & 1u;
        uint32_t spbg = (attr >> 7u) & 1u;
        uint32_t pal = attr & 0xFu;
        uint32_t cls = cgy == 2u ? ((uint32_t)DS_CONST_vid_cgy2_as & 3u) : cgy;
        uint32_t h = (uint32_t)spr_height[cls];
        uint32_t m = (cgx != 0u ? 0x002u : 0u) | (cls == 1u ? 0x004u : (cls >= 2u ? 0x00Cu : 0u));
        uint32_t base = ((pc & ~m) >> 1u) << 6u;
        uint32_t width = cgx != 0u ? 32u : 16u;
        uint32_t row;
        uint32_t pw[4] = {0u, 0u, 0u, 0u};
        uint32_t sc;

        if (line < y || line >= y + h) { /* plain integers: Y <= L < Y + height (D-159) */
            continue;
        }
        spr->covering = (uint8_t)((uint32_t)spr->covering + 1u);
        row = yflip != 0u ? h - 1u - (line - y) : line - y;
        for (sc = 0u; sc < width; sc++) { /* sc is the column in the sprite's own pattern; X-flip mirrors the whole combined sprite, so the dot is the mirrored column */
            uint32_t t = xflip != 0u ? width - 1u - sc : sc;
            uint32_t dx = x + t;
            uint32_t k;
            uint32_t sbit;
            uint32_t px;

            if ((sc & 15u) == 0u) { /* the four plane words of this cell's row: plane SGk at +16k words, cell (cx, cy) at base + cx * $40 + cy * $80 (H7-22, H7-23, H7-24) */
                uint32_t cell = base + (sc >> 4u) * 0x40u + (row >> 4u) * 0x80u + (row & 15u);

                for (k = 0u; k < 4u; k++) {
                    pw[k] = (uint32_t)ds_vdc_vram_read(inst, cell + 16u * k);
                }
            }
            if (dx < SPR_ORIGIN_X || dx - SPR_ORIGIN_X >= aw) { /* drawn only inside the active window (D-159) */
                continue;
            }
            sbit = 15u - (sc & 15u); /* ASSUMED LED-VID-0011: the leftmost pixel of a pattern row is bit 15 (the one statement of the sprite bit order) */
            px = ((pw[0] >> sbit) & 1u) | (((pw[1] >> sbit) & 1u) << 1u) | (((pw[2] >> sbit) & 1u) << 2u) | (((pw[3] >> sbit) & 1u) << 3u);
            if (px != 0u && spr->dot[dx - SPR_ORIGIN_X] == 0u) { /* px 0 is transparent; the first opaque pixel in SATB order wins (D-160, H7-19) */
                spr->dot[dx - SPR_ORIGIN_X] = (uint16_t)((spbg << 15u) | 0x100u | (pal << 4u) | px);
            }
        }
    }
}

uint32_t ds_vid_render(const struct ds_instance *inst, uint64_t origin, const struct ds_vid_spr *spr, uint8_t *rec)
{
    uint64_t k = (origin - (uint64_t)DS_CONST_raster_phase_mclk) / (uint64_t)DS_CONST_line_mclk;
    uint64_t fl = (uint64_t)inst->frame_lines;
    const uint16_t *reg = inst->vdc_reg;
    int display = (uint32_t)inst->v_state == V_STATE_VDW;
    uint32_t ax = 0u;
    uint32_t aw = 0u;
    uint32_t clipped = 0u;
    uint32_t cr = (uint32_t)reg[REG_CR];
    uint32_t bb = (cr & CR_BB) != 0u ? 1u : 0u;
    uint32_t sb = (cr & CR_SB) != 0u ? 1u : 0u;
    uint32_t unmodelled = ((uint32_t)inst->mwr_active & MWR_VM_SM) != 0u ? 1u : 0u;
    uint32_t screen = ((uint32_t)inst->mwr_active >> 4u) & 7u;
    uint32_t bat_w = (uint32_t)bat_chars_w[screen];
    uint32_t col_mask = bat_w * 8u - 1u;
    uint32_t row = (uint32_t)inst->bg_y & ((uint32_t)bat_chars_h[screen] * 8u - 1u);
    uint32_t bxr_line = (uint32_t)reg[REG_BXR] & 0x3FFu;
    uint32_t burst = inst->burst != 0u ? 1u : 0u;
    uint32_t idx;
    uint32_t rec_bytes;
    uint32_t j;

    if (display) { /* a non-display line is header-only: active_w 0 and active_x 0 */
        active_window(inst, &ax, &aw, &clipped);
    }
    rec_bytes = (uint32_t)DS_VID_REC_HEADER_BYTES + ((4u * aw + 7u) & ~7u);
    put_le(rec, OFF_ORIGIN, origin, 8u);
    put_le(rec, OFF_FRAME, fl != UINT64_C(0) ? k / fl : UINT64_C(0), 4u); /* the frame and the line in it are derived from the line index and the bound frame length, never stored (D-136) */
    put_le(rec, OFF_LINE, fl != UINT64_C(0) ? k % fl : UINT64_C(0), 2u);
    put_le(rec, OFF_FRAME_LINES, fl, 2u);
    put_le(rec, OFF_REC_BYTES, (uint64_t)rec_bytes, 2u);
    put_le(rec, OFF_LINE_MCLK, (uint64_t)DS_CONST_line_mclk, 2u);
    put_le(rec, OFF_DIV, (uint64_t)inst->lat_div, 1u);
    put_le(rec, OFF_VCE_CR, (uint64_t)inst->vce_cr, 1u);
    put_le(rec, OFF_ACTIVE_X, (uint64_t)ax, 2u);
    put_le(rec, OFF_ACTIVE_W, (uint64_t)aw, 2u);
    put_le(rec, OFF_BORDER, (uint64_t)pal_entry(inst, BORDER_ENTRY), 2u);
    put_le(rec, OFF_ACTIVE_K, display ? (uint64_t)inst->display_k : (uint64_t)0xFFFFu, 2u); /* the display line index inside VDW, 0xFFFF on every other line (plan 04-08) */
    put_le(rec, OFF_V_STATE, (uint64_t)inst->v_state, 1u);
    put_le(rec, OFF_FLAGS, (display ? (uint64_t)FLAG_ACTIVE : UINT64_C(0)) | (display && burst != 0u ? (uint64_t)FLAG_BURST : UINT64_C(0)) | (bb != 0u ? (uint64_t)FLAG_BB : UINT64_C(0)) | (sb != 0u ? (uint64_t)FLAG_SB : UINT64_C(0)) | (unmodelled != 0u ? (uint64_t)FLAG_UNMODELLED : UINT64_C(0)) | (clipped != 0u ? (uint64_t)FLAG_CLIPPED : UINT64_C(0)), 1u);
    put_le(rec, OFF_LINE_DIRTY, (uint64_t)inst->line_dirty, 1u);
    put_le(rec, OFF_SLOTS, UINT64_C(0), 1u);
    put_le(rec, OFF_COVERING, (uint64_t)spr->on != UINT64_C(0) ? (uint64_t)spr->covering : UINT64_C(0), 1u); /* the SATB entries that cover this line, counted by ds_vid_sprites (D-161); 0 with SB clear */
    put_le(rec, OFF_VERSION, (uint64_t)DS_VIDREC_VERSION, 1u);
    put_le(rec, OFF_RCR, (uint64_t)inst->rcr_counter, 2u);
    put_le(rec, OFF_BXR, (uint64_t)bxr_line, 2u);
    put_le(rec, OFF_BYR, (uint64_t)inst->bg_y, 2u);
    put_le(rec, OFF_HSR, (uint64_t)reg[REG_HSR], 2u);
    put_le(rec, OFF_HDR, (uint64_t)reg[REG_HDR], 2u);
    put_le(rec, OFF_VPR, (uint64_t)reg[REG_VPR], 2u);
    put_le(rec, OFF_VDW, (uint64_t)reg[REG_VDW], 2u);
    put_le(rec, OFF_VCR, (uint64_t)reg[REG_VCR], 2u);
    put_le(rec, OFF_MWR, (uint64_t)inst->mwr_active, 2u);
    put_le(rec, OFF_VDC_CR, (uint64_t)reg[REG_CR], 2u);
    put_le(rec, OFF_RESERVED, UINT64_C(0), 8u);
    for (j = 0u; j < aw; j++) { /* the payload: u16 index[active_w], then u16 colour[active_w] (the colour of the palette entry of that index), then zero padding to a multiple of 8 */
        if (burst != 0u) { /* a burst frame: every active dot is the blank index (D-157) */
            idx = (uint32_t)BLANK_INDEX;
        } else {
            if (bb == 0u) { /* BB = 0 and not burst: the background is off and the dot takes bg_off_index unless a sprite covers it (D-157) */
                idx = (uint32_t)DS_CONST_bg_off_index;
            } else {
                idx = bg_index(inst, bat_w, (bxr_line + j) & col_mask, row);
            }
            if (spr->on != 0u && spr->dot[j] != 0u && ((uint32_t)(spr->dot[j] >> 15u) != 0u || bb == 0u || (idx & 0xFu) == 0u)) { /* the winning sprite pixel replaces the dot when its SPBG bit is 1 or the background dot is transparent (px 0, or BB off) (MAN H7-21) */
                idx = (uint32_t)spr->dot[j] & 0x1FFu;
            }
        }
        put_le(rec, (uint32_t)DS_VID_REC_HEADER_BYTES + 2u * j, (uint64_t)idx, 2u);
        put_le(rec, (uint32_t)DS_VID_REC_HEADER_BYTES + 2u * aw + 2u * j, (uint64_t)pal_entry(inst, idx), 2u);
    }
    for (j = (uint32_t)DS_VID_REC_HEADER_BYTES + 4u * aw; j < rec_bytes; j++) {
        rec[j] = 0u;
    }
    return rec_bytes;
}

/* the one place besides the want_pixels gate of the fold where the sink is touched (D-164): the whole record is offered, and the sink stores it or counts it lost */
void ds_vid_emit(struct ds_obs_log *obs, const uint8_t *rec, uint32_t rec_bytes)
{
    ds_obs_vid(obs, rec, rec_bytes);
}
