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
 * The side effects exist only for the MSB of VWR (the write operation), the MSB of MARR and the MSB read of VRR (the read operation); LENR and DVSSR are stored and their MSB effects (the DMAs) arrive in plan 04-10. VWR LSB stores the 8-bit write latch, which is never cleared: a second MSB write stores the old LSB again (ASSUMED LED-VID-0006).
 * A write operation stores (msb << 8) | latch at word MAWR through the accessor and then adds the IW increment (1, $20, $40, $80 for CR bits 12 and 11, MAN H7-7) to MAWR, even when the store is dropped. A read operation loads the buffer from VRAM[MARR] and then adds the same increment to MARR; the CPU receives the buffer as it was BEFORE
 * the new operation (MAN H7-5 and H7-25: the high byte write of MARR starts the read, the high byte read of VRR triggers the next word). All address arithmetic is unsigned and wraps modulo 65,536 (D-147).
 * VRAM is 65,536 bytes, word w = byte 2w | byte 2w+1 << 8, and every access goes through the ONE accessor pair below: a read at a word address at or above DS_CONST_vdc_vram_words returns DS_CONST_vdc_oob_read and a write there is dropped (ASSUMED LED-VID-0007). The status read returns the flag bits 0 to 5 with bit 6 (BSY) the named constant
 * and bit 7 zero, and clears bits 0 to 5 (MAN H7-4); ds_vdc_flag is the single status setter, with the enable gate of D-144 (its callers arrive in plan 04-08, so no flag is ever set by an event yet). A register write sets the line_dirty bits of D-161.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_vdc.h"

/* the VRAM is stored as two bytes per word: the constant row and the array agree */
_Static_assert((uint32_t)DS_CONST_vdc_vram_words * 2u == DS_VRAM_BYTES, "constants.def vdc_vram_words does not match the VRAM byte array");

/* the register slots R00 to R13 (hexadecimal numbering) */
enum { VDC_MAWR = 0x00, VDC_MARR = 0x01, VDC_VWR = 0x02, VDC_CR = 0x05, VDC_DCR = 0x0F, VDC_SLOTS = 20 };

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

    (void)stamp;
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
    }
}

/* the single status setter (D-144): bit 0 CR, 1 OR, 2 RR, 3 DS, 4 DV, 5 VD, each only when its enable bit is set at this instant (CR bits 0 to 3 for CR, OR, RR and VD, DCR bits 0 and 1 for DS and DV, MAN H7-4, H7-6, H7-11); enabling later never creates a flag and disabling later never clears one */
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
