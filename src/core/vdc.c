/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * vdc.c - the HuC6270 video display controller as the CPU sees it (plan 04-06; D-144, D-146, D-147, D-148, D-149, D-161, F-24, F-27). The window is the hardware page from $1FE000 to $1FE3FF; the register is the offset & 3 (the address pins A1 and A0; A2 to A9 are not decoded, so the window
 * repeats every four bytes) and the access arrives here AFTER the fold of the timeline to the access stamp (bus.c), so a write is seen by the first line event that follows it (D-138). Nothing here touches io_buffer, and the penalty of D-107 is charged by the bus before it gets here.
 *   offset & 3 = 0  write: the address register AR keeps its low five bits ($24 is $04); read: the status (below)
 *   offset & 3 = 1  write: ignored; read: $00 (the high byte of the status word, MAN H7-25)
 *   offset & 3 = 2  data LSB (write: the low byte of the register AR names; read: the low byte of the read buffer)
 *   offset & 3 = 3  data MSB (write: the high byte, with the side effect of the register; read: the high byte of the read buffer, with the read operation when AR is VRR)
 * The registers R00 to R13 are 16-bit values updated byte-wise and at once, then ANDed with ONE static const mask table (MAN H7-2, H7-5 to H7-11 give the field widths). R03, R04 (mask 0) and the AR values $14 to $1F ignore a write and read the read buffer byte, with no side effect (ASSUMED LED-VID-0006).
 * The side effects exist only for the MSB of VWR (the write operation), the MSB of MARR and the MSB read of VRR (the read operation); The MSB of LENR starts the VRAM-to-VRAM transfer and the MSB of DVSSR marks the SATB copy for the next VBlank entry (plan 04-10, D-150, D-151; the progress and the copy are driven by the fold in
 * beam.c). VWR LSB stores the 8-bit write latch, which is never cleared: a second MSB write stores the old LSB again (ASSUMED LED-VID-0006).
 * A write operation stores (msb << 8) | latch at word MAWR through the accessor and then adds the IW increment (1, $20, $40, $80 for CR bits 12 and 11, MAN H7-7) to MAWR, even when the store is dropped. A read operation loads the buffer from VRAM[MARR] and then adds the same increment to MARR; the CPU receives the buffer as it was BEFORE
 * the new operation (MAN H7-5 and H7-25: the high byte write of MARR starts the read, the high byte read of VRR triggers the next word). All address arithmetic is unsigned and wraps modulo 65,536 (D-147).
 * VRAM is 65,536 bytes, word w = byte 2w | byte 2w+1 << 8, and every access goes through the ONE accessor pair below: a read at a word address at or above DS_CONST_vdc_vram_words returns DS_CONST_vdc_oob_read and a write there is dropped (ASSUMED LED-VID-0007). The status read returns the flag bits 0 to 5 with bit 6 (BSY) the named constant
 * and bit 7 zero, and clears bits 0 to 5 (MAN H7-4); ds_vdc_flag is the single status setter, with the enable gate of D-144 (its callers are the events of the ladder in beam.c since plan 04-08: VD and RR, and DS and DV since plan 04-10; OR and CR follow in plan 04-15). A register write sets the line_dirty bits of D-161.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_vdc.h"

/* the VRAM is stored as two bytes per word: the constant row and the array agree */
_Static_assert((uint32_t)DS_CONST_vdc_vram_words * 2u == DS_VRAM_BYTES, "constants.def vdc_vram_words does not match the VRAM byte array");

/* the register slots R00 to R13 (hexadecimal numbering); the DMA registers are SOUR, DESR, LENR and DVSSR (R10 to R13) */
enum { VDC_MAWR = 0x00, VDC_MARR = 0x01, VDC_VWR = 0x02, VDC_CR = 0x05, VDC_BYR = 0x08, VDC_DCR = 0x0F, VDC_SOUR = 0x10, VDC_DESR = 0x11, VDC_LENR = 0x12, VDC_DVSSR = 0x13, VDC_SLOTS = 20 };
/* the vertical state VDW (beam.c numbers it 1): the bus is not free for a transfer there unless the frame is burst; and the sizes of the two transfers */
enum { VDC_VS_VDW = 1, SATB_WORDS = 256 };

/* the value masks of D-146, one per register slot, applied after every byte update (MAN H7-2, H7-5 to H7-11: field widths; H7-2 labels the VCR row VDW, the text and H7-11 say VCR). R03 and R04 are reserved: mask 0 keeps them zero */
static const uint16_t vdc_mask[VDC_SLOTS] = {
    0xFFFFu, 0xFFFFu, 0xFFFFu, 0x0000u, 0x0000u, /* MAWR MARR VWR (R03) (R04) */
    0x1FFFu, 0x03FFu, 0x03FFu, 0x01FFu, 0x00FFu, /* CR RCR BXR BYR MWR */
    0x7F1Fu, 0x7F7Fu, 0xFF1Fu, 0x01FFu, 0x00FFu, /* HSR HDR VPR VDW VCR */
    0x001Fu, 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu, /* DCR SOUR DESR LENR DVSSR */
};

/* the line_dirty bit a write of each register sets (D-161): b1 HSR and HDR, b2 VPR, VDW and VCR, b3 BXR and BYR, b5 CR and MWR; the rest set none */
static const uint8_t vdc_dirty[VDC_SLOTS] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x20u, 0x00u, 0x08u, 0x08u, 0x20u,
    0x02u, 0x02u, 0x04u, 0x04u, 0x04u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
};

/* the one accessor pair of D-147: word addresses at or above DS_CONST_vdc_vram_words are out of range, everything else is two bytes of the array */
uint16_t ds_vdc_vram_read(const struct ds_instance *inst, uint32_t word)
{
    uint32_t i;

    if (word >= (uint32_t)DS_CONST_vdc_vram_words) {
        return (uint16_t)DS_CONST_vdc_oob_read;
    }
    i = word << 1u;
    return (uint16_t)((uint32_t)inst->vdc_vram[i] | ((uint32_t)inst->vdc_vram[i + 1u] << 8u));
}

void ds_vdc_vram_write(struct ds_instance *inst, uint32_t word, uint16_t value)
{
    uint32_t i;

    if (word >= (uint32_t)DS_CONST_vdc_vram_words) {
        return;
    }
    i = word << 1u;
    inst->vdc_vram[i] = (uint8_t)((uint32_t)value & 0xFFu);
    inst->vdc_vram[i + 1u] = (uint8_t)((uint32_t)value >> 8u);
}

/* the address step of VWR and VRR by CR bits 12 and 11 (MAN H7-7): 1, $20, $40, $80, read from the register at the moment of the operation */
static const uint8_t vdc_step[4] = {0x01u, 0x20u, 0x40u, 0x80u};

static uint32_t iw_step(const struct ds_instance *inst)
{
    return (uint32_t)vdc_step[((uint32_t)inst->vdc_reg[VDC_CR] >> 11u) & 3u];
}

/* the read operation (D-148): load the buffer from the word MARR names, then step MARR; the buffer load is one statement */
static void read_operation(struct ds_instance *inst)
{
    inst->vdc_rdbuf = ds_vdc_vram_read(inst, (uint32_t)inst->vdc_reg[VDC_MARR]);
    inst->vdc_reg[VDC_MARR] = (uint16_t)(((uint32_t)inst->vdc_reg[VDC_MARR] + iw_step(inst)) & 0xFFFFu);
}

/* the write operation (D-148): store the word (msb << 8) | latch at MAWR, then step MAWR, even when the store is dropped */
static void write_operation(struct ds_instance *inst, uint32_t msb)
{
    ds_vdc_vram_write(inst, (uint32_t)inst->vdc_reg[VDC_MAWR], (uint16_t)((msb << 8u) | (uint32_t)inst->vdc_wlatch));
    inst->vdc_reg[VDC_MAWR] = (uint16_t)(((uint32_t)inst->vdc_reg[VDC_MAWR] + iw_step(inst)) & 0xFFFFu);
}

/* the dot clock periods of one DMA word in master clocks: DS_CONST_vdc_dma_dots_per_word times the divider. The divider entries and the constant are non-zero, which is what makes every divisor below non-zero (a transfer is never left running with word_mclk 0) */
_Static_assert(DS_CONST_vdc_dma_dots_per_word > 0, "vdc_dma_dots_per_word must be non-zero: it is a divisor of the closed form");

/* one word of the VRAM-to-VRAM transfer (D-151): read SOUR, write DESR through the accessor pair, then step SOUR and DESR by DCR bits 2 and 3 (0 up, 1 down) and LENR down by one; IW never applies and MAWR, MARR, the buffer and the latch are not touched. All steps are unsigned modulo 65,536, so LENR ends at $FFFF */
static void dma_word(struct ds_instance *inst, uint32_t sour_step, uint32_t desr_step)
{
    uint32_t sour = (uint32_t)inst->vdc_reg[VDC_SOUR];
    uint32_t desr = (uint32_t)inst->vdc_reg[VDC_DESR];

    ds_vdc_vram_write(inst, desr, ds_vdc_vram_read(inst, sour));
    inst->vdc_reg[VDC_SOUR] = (uint16_t)((sour + sour_step) & 0xFFFFu);
    inst->vdc_reg[VDC_DESR] = (uint16_t)((desr + desr_step) & 0xFFFFu);
    inst->vdc_reg[VDC_LENR] = (uint16_t)(((uint32_t)inst->vdc_reg[VDC_LENR] + 0xFFFFu) & 0xFFFFu);
}

/* the closed form of the transfer progress (D-151, P3): the words due at time t are (t - dma_start) / dma_word_mclk, less the words done, at most the words that remain (LENR + 1 while running, so at most 65,536); never a loop over clocks, one loop over the due words. The fold calls it before every event and at its target stamp, so a word is
 * applied before every access that follows it (events and words with time <= stamp first). Completion ends the transfer and raises DV through ds_vdc_flag, whose gate reads DCR bit 1 at this instant: no access lies between the completion time and the fold that applies it, so the gate value is the one of the completion instant */
void ds_vdc_dma_apply(struct ds_instance *inst, uint64_t t)
{
    uint64_t due;
    uint64_t n;
    uint32_t remaining;
    uint32_t sour_step;
    uint32_t desr_step;
    uint32_t i;

    if ((uint32_t)inst->dma_state != 2u || t < inst->dma_start || inst->dma_word_mclk == 0u) { /* the guard keeps the divisor non-zero for any state, a hostile restore included (the restore rule is 8, 12 or 16) */
        return;
    }
    due = (t - inst->dma_start) / (uint64_t)inst->dma_word_mclk;
    if (due <= (uint64_t)inst->dma_done) {
        return;
    }
    remaining = (uint32_t)inst->vdc_reg[VDC_LENR] + 1u;
    n = due - (uint64_t)inst->dma_done;
    if (n > (uint64_t)remaining) {
        n = (uint64_t)remaining;
    }
    sour_step = (((uint32_t)inst->vdc_reg[VDC_DCR] >> 2u) & 1u) != 0u ? 0xFFFFu : 1u;
    desr_step = (((uint32_t)inst->vdc_reg[VDC_DCR] >> 3u) & 1u) != 0u ? 0xFFFFu : 1u;
    for (i = 0u; i < (uint32_t)n; i++) {
        dma_word(inst, sour_step, desr_step);
    }
    inst->dma_done = (uint32_t)((uint64_t)inst->dma_done + n);
    if (n == (uint64_t)remaining) { /* the last word moved: the transfer is over */
        inst->dma_state = 0u;
        inst->dma_done = 0u;
        ds_vdc_flag(inst, 4u);
    }
}

/* a LENR MSB write starts a transfer of LENR + 1 words from the live SOUR to the live DESR (D-151, MAN H7-12(a)); a second write restarts from the live registers. It runs from this stamp when the bus is free (the state is not VDW, or the frame is burst) and the divider has been latched, else it is pending and the next EV_LINE that
 * finds the bus free starts it (beam.c). The word time is latched here from the divider, and a pending one holds the smallest legal value until the real start latches it */
static void dma_begin(struct ds_instance *inst, uint64_t stamp)
{
    uint32_t div = (uint32_t)inst->lat_div;
    int free_bus = (uint32_t)inst->v_state != (uint32_t)VDC_VS_VDW || inst->burst != 0u;

    inst->dma_done = 0u;
    inst->dma_word_mclk = (uint16_t)((uint32_t)DS_CONST_vdc_dma_dots_per_word * (div != 0u ? div : 2u));
    if (free_bus && div != 0u) {
        inst->dma_state = 2u;
        inst->dma_start = stamp;
    } else {
        inst->dma_state = 1u;
    }
}

/* the copy of the sprite attribute table (D-150): 256 words from VRAM (DVSSR + i) & $FFFF through the accessor into SATB, at t, the EV_LINE of the VBlank state entry. The words are stored little endian like VRAM; DVSSR is not modified; DS is scheduled 256 * dots-per-word * divider master clocks later (the divider latched at this EV_LINE, so it is non-zero) */
void ds_vdc_satb_copy(struct ds_instance *inst, uint64_t t)
{
    uint32_t src = (uint32_t)inst->vdc_reg[VDC_DVSSR];
    uint32_t i;

    for (i = 0u; i < (uint32_t)SATB_WORDS; i++) {
        uint32_t w = (uint32_t)ds_vdc_vram_read(inst, (src + i) & 0xFFFFu);

        inst->vdc_satb[2u * i] = (uint8_t)(w & 0xFFu);
        inst->vdc_satb[2u * i + 1u] = (uint8_t)(w >> 8u);
    }
    inst->satb_pending = 0u;
    inst->satb_done_valid = 1u;
    inst->satb_done_at = t + (uint64_t)SATB_WORDS * (uint64_t)DS_CONST_vdc_dma_dots_per_word * (uint64_t)inst->lat_div;
}

/* DS is raised when its time has come (the fold, in time order with the events): through ds_vdc_flag, gate DCR bit 0 at this instant (D-144) */
void ds_vdc_satb_due(struct ds_instance *inst, uint64_t t)
{
    if (inst->satb_done_valid != 0u && inst->satb_done_at <= t) {
        inst->satb_done_valid = 0u;
        ds_vdc_flag(inst, 3u);
    }
}

uint8_t ds_vdc_read(struct ds_instance *inst, uint32_t reg, uint64_t stamp)
{
    uint32_t value;

    (void)stamp;
    switch (reg & 3u) {
    case 0u: /* the status: the flag bits 0 to 5, BSY the named constant, bit 7 zero (MAN H7-4, H7-25); the read clears bits 0 to 5 in one statement */
        value = ((uint32_t)inst->vdc_status & 0x3Fu) | (((uint32_t)DS_CONST_vdc_bsy_level & 1u) << 6u);
        inst->vdc_status = (uint8_t)((uint32_t)inst->vdc_status & 0xC0u);
        return (uint8_t)value;
    case 1u: /* the high byte of the status word reads 00h (MAN H7-25) */
        return 0u;
    case 2u: /* the low byte of the read buffer, whatever AR holds, with no side effect */
        return (uint8_t)((uint32_t)inst->vdc_rdbuf & 0xFFu);
    default: /* the high byte of the read buffer as it was before the operation it may trigger; only VRR (AR = 2) triggers the next read (MAN H7-5, H7-25) */
        value = (uint32_t)inst->vdc_rdbuf >> 8u;
        if ((uint32_t)inst->vdc_ar == (uint32_t)VDC_VWR) {
            read_operation(inst);
        }
        return (uint8_t)value;
    }
}

void ds_vdc_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp)
{
    uint32_t ar;
    uint32_t msb;
    uint32_t r;

    switch (reg & 3u) {
    case 0u: /* AR keeps its low five bits */
        inst->vdc_ar = (uint8_t)((uint32_t)value & 0x1Fu);
        return;
    case 1u: /* ignored */
        return;
    default:
        break;
    }
    ar = (uint32_t)inst->vdc_ar;
    if (ar >= (uint32_t)VDC_SLOTS) { /* the AR values $14 to $1F name no register */
        return;
    }
    msb = reg & 1u;
    r = (uint32_t)inst->vdc_reg[ar];
    r = msb != 0u ? ((r & 0x00FFu) | ((uint32_t)value << 8u)) : ((r & 0xFF00u) | (uint32_t)value);
    inst->vdc_reg[ar] = (uint16_t)(r & (uint32_t)vdc_mask[ar]);
    inst->line_dirty = (uint8_t)((uint32_t)inst->line_dirty | (uint32_t)vdc_dirty[ar]);
    if (ar == (uint32_t)VDC_BYR) { /* a write of either BYR byte sets the row counter to the written 9-bit value at its stamp (D-156), so the next display line shows BYR + 1 */
        inst->bg_y = (uint16_t)((uint32_t)inst->vdc_reg[VDC_BYR] & 0x1FFu);
    }
    if (msb == 0u) {
        if (ar == (uint32_t)VDC_VWR) { /* VWR LSB: the write latch, never cleared */
            inst->vdc_wlatch = value;
        }
        return;
    }
    if (ar == (uint32_t)VDC_VWR) { /* VWR MSB: the write operation */
        write_operation(inst, (uint32_t)value);
    } else if (ar == (uint32_t)VDC_MARR) { /* MARR MSB: the read operation */
        read_operation(inst);
    } else if (ar == (uint32_t)VDC_LENR) { /* LENR MSB: the VRAM-to-VRAM transfer starts (D-151); the LSB only stores */
        dma_begin(inst, stamp);
    } else if (ar == (uint32_t)VDC_DVSSR) { /* DVSSR MSB: the SATB copy is due at the next VBlank entry (D-150); the LSB triggers nothing */
        inst->satb_pending = 1u;
    }
}

/* the single status setter (D-144; called by the fold in beam.c): bit 0 CR, 1 OR, 2 RR, 3 DS, 4 DV, 5 VD, each only when its enable bit is set at this instant (CR bits 0 to 3 for CR, OR, RR and VD, DCR bits 0 and 1 for DS and DV, MAN H7-4, H7-6, H7-11); enabling later never creates a flag and disabling later never clears one */
void ds_vdc_flag(struct ds_instance *inst, uint32_t bit)
{
    uint32_t cr = (uint32_t)inst->vdc_reg[VDC_CR];
    uint32_t dcr = (uint32_t)inst->vdc_reg[VDC_DCR];
    uint32_t enabled;

    switch (bit) {
    case 0u:
        enabled = cr & 1u;
        break;
    case 1u:
        enabled = (cr >> 1u) & 1u;
        break;
    case 2u:
        enabled = (cr >> 2u) & 1u;
        break;
    case 3u:
        enabled = dcr & 1u;
        break;
    case 4u:
        enabled = (dcr >> 1u) & 1u;
        break;
    case 5u:
        enabled = (cr >> 3u) & 1u;
        break;
    default:
        enabled = 0u;
        break;
    }
    if (enabled != 0u) {
        inst->vdc_status = (uint8_t)((uint32_t)inst->vdc_status | (1u << bit));
    }
}
