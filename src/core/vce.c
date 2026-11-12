/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * vce.c - the HuC6260 colour encoder as the CPU sees it (plan 04-04; D-141, D-152, D-153, D-154, F-25). The window is the hardware page from $1FE400 to $1FE7FF; the register is selected by the offset & 7 (the address pins A2 to A0, MAN H6-2) and the access arrives
 * here AFTER the fold of the timeline to the access stamp (bus.c), so a write is seen by the first line event that follows it (D-138).
 *   $0400 CR (write: all eight bits are stored; bits 1:0 choose the dot clock divider, bit 7 and bit 2 are only carried into the line record)
 *   $0402 CTA low (write: bits 7:0 of the 9-bit colour table address)   $0403 CTA high (write: bit 0 only)
 *   $0404 CTW and CTR low (write and read: bits 7:0 of the entry at CTA)  $0405 CTW and CTR high (write and read: bit 8 of the entry; the address is then incremented, mod 512)
 * Every other offset and every read of CR, CTA, $0401, $0406 and $0407 returns DS_CONST_vce_unused_read; their writes are dropped. A read of $0405 returns bit 8 of the entry under DS_CONST_vce_data_hi_unused_or. The two unused-bit values and the wrap at 511 are
 * reports of other emulators (ASSUMED LED-VID-0016), the increment after the HIGH byte is MAN H6-3 and H6-5. The palette is 512 entries of 9 bits kept as 1,024 little-endian bytes (entry i is byte 2i | byte 2i+1 << 8, bits 15:9 always 0), so the live span equals its stream
 * payload on every host. The core never decodes green, red or blue: no colour conversion exists anywhere (D-154). None of the accesses touches io_buffer, and the penalty of D-107 is charged by the bus before it gets here.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_vdc.h"

/* the dot clock divider by the two low bits of CR (D-141): the one table, a constant row of constants.def per entry (the first two entries share a line: that line is the one anchor of the toy vid-dot-div of plan 04-09); the divider the fold latches at EV_LINE is read from here and from nowhere else */
static const uint8_t vce_divider[4] = {
    (uint8_t)DS_CONST_vce_div_cr0, (uint8_t)DS_CONST_vce_div_cr1,
    (uint8_t)DS_CONST_vce_div_cr2,
    (uint8_t)DS_CONST_vce_div_cr3,
};

uint32_t ds_vce_divider(uint32_t cr)
{
    return (uint32_t)vce_divider[cr & 3u];
}

/* the palette bytes of the entry the colour table address names: the address is kept at most $1FF, and the mask is applied again where it indexes */
static uint32_t pal_index(const struct ds_instance *inst)
{
    return (uint32_t)inst->vce_cta & 0x1FFu;
}

static void cta_step(struct ds_instance *inst)
{
    inst->vce_cta = (uint16_t)(((uint32_t)inst->vce_cta + 1u) & 0x1FFu);
}

uint8_t ds_vce_read(struct ds_instance *inst, uint32_t reg, uint64_t stamp)
{
    uint32_t idx = pal_index(inst);
    uint32_t value;

    (void)stamp;
    switch (reg & 7u) {
    case 4u: /* CTR low: bits 7:0 of the entry at CTA */
        return inst->vce_pal[idx << 1u];
    case 5u: /* CTR high: bit 8 of the entry over the unused bits, then the address steps */
        value = ((uint32_t)inst->vce_pal[(idx << 1u) + 1u] & 1u) | (uint32_t)DS_CONST_vce_data_hi_unused_or;
        cta_step(inst);
        return (uint8_t)value;
    default: /* CR, CTA, $0401, $0406 and $0407 have no read */
        return (uint8_t)DS_CONST_vce_unused_read;
    }
}

void ds_vce_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp)
{
    uint32_t idx = pal_index(inst);

    (void)stamp;
    switch (reg & 7u) {
    case 0u: /* CR: all eight bits are stored; bit 0 of line_dirty says a line record follows a CR write (D-161) */
        inst->vce_cr = value;
        inst->line_dirty = (uint8_t)((uint32_t)inst->line_dirty | 0x01u);
        break;
    case 2u: /* CTA low */
        inst->vce_cta = (uint16_t)(((uint32_t)inst->vce_cta & 0x100u) | (uint32_t)value);
        break;
    case 3u: /* CTA high: bit 0 only */
        inst->vce_cta = (uint16_t)(((uint32_t)inst->vce_cta & 0x0FFu) | (((uint32_t)value & 1u) << 8u));
        break;
    case 4u: /* CTW low: bits 7:0 of the entry at CTA */
        inst->vce_pal[idx << 1u] = value;
        inst->line_dirty = (uint8_t)((uint32_t)inst->line_dirty | 0x10u);
        break;
    case 5u: /* CTW high: bit 8 of the entry, then the address steps (the one statement of the nine-bit store: the entry is the low byte already held with the written byte above it, kept to 9 bits) */
        inst->vce_pal[(idx << 1u) + 1u] = (uint8_t)((((uint32_t)inst->vce_pal[idx << 1u] | ((uint32_t)value << 8u)) & 0x1FFu) >> 8u);
        inst->line_dirty = (uint8_t)((uint32_t)inst->line_dirty | 0x10u);
        cta_step(inst);
        break;
    default: /* $0401, $0406 and $0407 drop the write */
        break;
    }
}
