/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * bus.c - the one bus choke point and the memory map behind it (plan 03-16; D-102, D-103, D-104, D-106, D-107, D-108). It moved here from cpu.c, where plans 03-01 to 03-14 kept a minimal version.
 * Every CPU access translates per access from the eight mapping registers: phys = (mpr[a >> 13] << 13) | (a & $1FFF), a 21-bit address, and the 8-bit bank index mpr[a >> 13] selects a class in the const
 * table bank_class[256]: $00-$7F the HuCard through ds_mapper_read (writes ignored), $80-$F7 unmapped, $F8-$FB the 8 KiB work RAM mirrored four times (& $1FFF), $FC-$FE unmapped, $FF the hardware page.
 * No translated pointer and no derived table is cached (D-102): the table is a constant and the MPRs are the only state. Unmapped banks read the one constant DS_UNMAPPED_READ and ignore writes (D-104).
 * The memory-map facts are the chip-enable allocation of the hardware manual (H8-9, H8-24: VDC at $1FE000, VCE at $1FE400, PSG at $1FE800, timer at $1FEC00, I/O at $1FF000, interrupt registers at
 * $1FF400, work RAM behind CER at $1F0000-$1F7FFF); the x4 mirroring of the 8 KiB RAM, the reads of unmapped banks and of the windows without a device, and the bus latch are modelling assumptions of this project
 * (LED-CPU-0011 to LED-CPU-0013), not hardware facts. Nothing here peeks: observers never read through the bus (D-108).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"

/* the 8 KiB work RAM is mirrored four times in banks $F8-$FB with & $1FFF (D-103): the mask is only right for this size */
_Static_assert(DS_RAM8K_BYTES == 0x2000u, "the work RAM mirror mask assumes 8 KiB");
_Static_assert(DS_MPR_COUNT == 8u, "the translate assumes eight mapping registers");

enum bank_cls { BC_UNMAPPED = 0, BC_HUCARD = 1, BC_RAM = 2, BC_HWPAGE = 3 };

/* the class of each of the 256 banks, written as runs (range designators are a GNU extension); the array is unsized and its size is asserted below, so a run that is too short or too long fails the build */
#define BC_X8(c) (c), (c), (c), (c), (c), (c), (c), (c)
#define BC_X32(c) BC_X8(c), BC_X8(c), BC_X8(c), BC_X8(c)
#define BC_X128(c) BC_X32(c), BC_X32(c), BC_X32(c), BC_X32(c)
static const uint8_t bank_class[] = {
    BC_X128(BC_HUCARD),                                                                                          /* $00-$7F */
    BC_X32(BC_UNMAPPED), BC_X32(BC_UNMAPPED), BC_X32(BC_UNMAPPED), BC_X8(BC_UNMAPPED), BC_X8(BC_UNMAPPED), BC_X8(BC_UNMAPPED), /* $80-$F7: 120 banks */
    BC_RAM, BC_RAM, BC_RAM, BC_RAM,                                                                              /* $F8-$FB */
    BC_UNMAPPED, BC_UNMAPPED, BC_UNMAPPED,                                                                       /* $FC-$FE */
    BC_HWPAGE,                                                                                                   /* $FF */
};
_Static_assert(sizeof bank_class == 256u, "bank_class must hold exactly one class per bank");

/* the bank index and the offset in the bank of a logical address: the only translation, computed per access from the MPRs */
static uint32_t bank_of(const struct ds_instance *inst, uint16_t addr)
{
    return (uint32_t)inst->mpr[(uint32_t)addr >> 13u];
}

static uint32_t offset_of(uint16_t addr)
{
    return (uint32_t)addr & 0x1FFFu;
}

uint8_t ds_bus_read(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr)
{
    uint32_t bank = bank_of(inst, addr);
    uint32_t offset = offset_of(addr);
    uint8_t value = 0u;

    (void)bus; /* the penalty joins in task 3 of plan 03-16 */
    switch ((enum bank_cls)bank_class[bank]) {
    case BC_HUCARD:
        return ds_mapper_read(inst, bank, offset, &value) == (uint32_t)DS_MAP_OK ? value : (uint8_t)DS_UNMAPPED_READ;
    case BC_RAM:
        return inst->ram8k[offset & (DS_RAM8K_BYTES - 1u)];
    case BC_HWPAGE: /* the windows of the hardware page land in task 2 */
    case BC_UNMAPPED:
    default:
        break;
    }
    return (uint8_t)DS_UNMAPPED_READ;
}

void ds_bus_write(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr, uint8_t value)
{
    uint32_t bank = bank_of(inst, addr);

    (void)bus;
    if (bank_class[bank] == (uint8_t)BC_RAM) {
        inst->ram8k[offset_of(addr) & (DS_RAM8K_BYTES - 1u)] = value;
    }
    /* ROM and every unmapped bank ignore a write (D-103, D-104) */
}

/*
 * The physical write path of ST0, ST1 and ST2 (plan 03-14; D-106, H8-8 Table 2-2-3, S8-80 to S8-82): the address is physical $1FE000 | n with n in {0, 2, 3} and the mapping registers are never consulted.
 * Its only caller is the ST group of ds_cpu_step_obs, and a target outside the three is dropped (T-03-40).
 */
void ds_bus_write_phys(struct ds_instance *inst, struct ds_bus *bus, uint32_t phys, uint8_t value)
{
    (void)inst;
    (void)bus;
    (void)value;
    if (phys == 0x1FE000u || phys == 0x1FE002u || phys == 0x1FE003u) {
        /* the VDC stub lands in task 2 */
    }
}

void ds_mpr_write(struct ds_instance *inst, uint32_t index, uint8_t value)
{
    if (index < (uint32_t)DS_MPR_COUNT) {
        inst->mpr[index] = value;
    }
}
