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
#include "ds_obs.h"
#include "ds_timer.h"
#include "ds_vdc.h"

/* the 8 KiB work RAM is mirrored four times in banks $F8-$FB with & $1FFF (D-103): the mask is only right for this size */
_Static_assert(DS_RAM8K_BYTES == 0x2000u, "the work RAM mirror mask assumes 8 KiB");
_Static_assert(DS_MPR_COUNT == 8u, "the translate assumes eight mapping registers");

/* the class names of the table are the values of enum ds_bank_class (ds_core.h), which the accessor ds_bank_class below returns */
#define BC_UNMAPPED DS_BANK_CLASS_UNMAPPED
#define BC_HUCARD DS_BANK_CLASS_HUCARD
#define BC_RAM DS_BANK_CLASS_RAM
#define BC_HWPAGE DS_BANK_CLASS_HWPAGE

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

/* the class of one bank, for the bank-class case of the conformance runner (plan 03-20; D-103): a bank above 255 is DS_BANK_CLASS_NONE. It reads the same const table as the bus and nothing else. */
uint32_t ds_bank_class(uint32_t bank)
{
    return bank < 256u ? (uint32_t)bank_class[bank] : (uint32_t)DS_BANK_CLASS_NONE;
}

/* the bank index and the offset in the bank of a logical address: the only translation, computed per access from the MPRs */
static uint32_t bank_of(const struct ds_instance *inst, uint16_t addr)
{
    return (uint32_t)inst->mpr[(uint32_t)addr >> 13u];
}

static uint32_t offset_of(uint16_t addr)
{
    return (uint32_t)addr & 0x1FFFu;
}

/*
 * The hardware page, bank $FF: six windows of $400 bytes at physical $1FE000 (H8-9, H8-24), decoded by the offset in the page shifted right by 10 in one switch (D-103, no function-pointer bus).
 * Window 0 is the VDC (register A1-A0), 1 the VCE (A2-A0), 2 the PSG (A3-A0), 3 the timer (A0), 4 the I/O port (one register for the whole window), 5 the interrupt registers (A1-A0: 2 is $1402, 3 is $1403),
 * and windows 6 and 7 ($1800-$1FFF) have no device. The timer, the interrupt controller (plans 03-21 and 03-22) and the pad (plan 03-27, io.c) are devices; the VDC and VCE are stubs until Phase 4 and the PSG until Phase 5:
 *   VDC window reads DS_VDC_STUB_READ and VCE window DS_VCE_STUB_READ (D-104); the windows without a device read the unmapped constant and change nothing.
 *   io_buffer (D-104, ASSUMED LED-CPU-0013) is the byte of the last read or write in $0800-$17FF (windows 2 to 5): a read returns its stub value with the upper bits of the byte the window does not drive taken from io_buffer
 *   (the PSG window all eight bits, the timer bit 7 over its 7-bit counter, $1402 and $1403 the upper five bits over three register bits, the interrupt registers $1400 and $1401 all eight, the I/O window nothing: the pad drives all eight lines), and the byte read becomes the new io_buffer;
 *   a write in those windows stores its byte in io_buffer (the I/O window's byte is also the latch of port O). Reads and writes in the VDC and VCE windows and in the empty windows do not touch io_buffer.
 */
enum hw_window { HW_VDC = 0, HW_VCE = 1, HW_PSG = 2, HW_TIMER = 3, HW_IO = 4, HW_IRQ = 5 };

/*
 * The VDC and VCE access penalty (D-107, ASSUMED LED-CPU-0017): any access to physical $1FE000-$1FE7FF, that is the offset in the page below $800 (windows 0 and 1), costs DS_ADDER_VDC_ACCESS more CPU cycles. It is charged here, in the bus,
 * before the device sees the access, for every route to the page (a logical address through any MPR, so the zero page and the stack included, and the physical writes of ST0 to ST2), and the step adds the sum to the instruction (cpu.c).
 * A NULL bus (the reset reads, a test) charges nothing. The cycle length it is multiplied by is the speed the instruction started at (D-101).
 */
static void charge_penalty(struct ds_bus *bus, uint32_t offset)
{
    if (bus != NULL && offset < 0x800u) {
        bus->penalty = bus->penalty + (uint32_t)DS_ADDER_VDC_ACCESS;
    }
}

/*
 * The stamp of an access in master clocks (plan 03-21; D-92, D-108): start + (last + penalty) * cycle_mclk, where start is the machine time at the start of the step, last the index of the last cycle of the instruction before the penalty
 * and penalty the cycles this step's accesses have been charged so far (this access included, since the penalty is charged first). A single data access is in the last cycle; the read of a read-modify-write instruction is one cycle earlier (bus->rmw).
 * No dummy accesses and no other in-instruction offsets are modelled (the default rule of D-92, ASSUMED LED-CPU-0004). A NULL bus (the reset reads, a test) has no step: the stamp is the machine time.
 */
static uint64_t access_stamp(const struct ds_instance *inst, const struct ds_bus *bus, int is_read)
{
    uint32_t idx;

    if (bus == NULL) {
        return inst->machine_time;
    }
    idx = bus->last + bus->penalty;
    if (is_read && bus->rmw != 0u && idx > 0u) {
        idx = idx - 1u;
    }
    return bus->start + (uint64_t)idx * (uint64_t)bus->cycle_mclk;
}

/* the base of the hardware page: physical $1FE000 (H8-9, H8-24) */
#define HW_PAGE_PHYS 0x1FE000u

/* one dev record for an access to the page at offset: a NULL bus (the reset reads, a test) logs nothing, like it charges nothing */
static void log_dev(const struct ds_instance *inst, const struct ds_bus *bus, uint64_t stamp, uint32_t offset, uint32_t value, uint32_t kind)
{
    if (bus != NULL) {
        ds_obs_dev(bus->obs, inst, stamp, bus->pc, HW_PAGE_PHYS | offset, value, kind, (offset < 0x800u ? (uint32_t)DS_DEV_FLAG_PENALTY : 0u) | (kind == (uint32_t)DS_DEV_ST ? (uint32_t)DS_DEV_FLAG_PHYS : 0u));
    }
}

static uint8_t hw_read(struct ds_instance *inst, struct ds_bus *bus, uint32_t offset)
{
    uint32_t reg = offset & 0x3FFu;
    uint8_t value;
    uint64_t stamp;

    charge_penalty(bus, offset);
    stamp = access_stamp(inst, bus, 1);
    switch ((enum hw_window)(offset >> 10u)) {
    case HW_VDC: /* the fold goes first with the stamp computed above, never recomputed (D-137 (1), F-24, Pitfall 2); the windows stay stubs until plans 04-03 and 04-04 */
        ds_vdc_advance(inst, stamp, bus != NULL ? bus->obs : NULL);
        return (uint8_t)DS_VDC_STUB_READ;
    case HW_VCE:
        ds_vdc_advance(inst, stamp, bus != NULL ? bus->obs : NULL);
        return (uint8_t)DS_VCE_STUB_READ;
    case HW_PSG:
        value = inst->io_buffer;
        break;
    case HW_IO: /* the pad protocol of io.c (plan 03-27): a pure function of the latch, the input schedule and the stamp; like every read in $0800-$17FF the byte read becomes io_buffer */
        value = ds_io_read(inst, stamp);
        break;
    case HW_TIMER: /* the counter at the access stamp over bit 7 of the bus latch; both addresses read the same (D-109) */
        value = (uint8_t)((uint32_t)(inst->io_buffer & 0x80u) | (uint32_t)ds_timer_read(inst, reg, stamp));
        break;
    case HW_IRQ: /* $1402 and $1403: the three register bits over the upper five of the latch (H8-24); $1400 and $1401 read the latch whole */
        if ((reg & 3u) == 3u) { /* a read of $1403 folds the VDC first (D-137 (1)); $1402 and $1400/$1401 do not: the mask only gates delivery (F-24) */
            ds_vdc_advance(inst, stamp, bus != NULL ? bus->obs : NULL);
        }
        value = (reg & 3u) >= 2u ? (uint8_t)((uint32_t)(inst->io_buffer & 0xF8u) | ds_irq_read(inst, reg, stamp)) : inst->io_buffer;
        break;
    default: /* windows 6 and 7: no device, the unmapped constant, no side effect */
        value = (uint8_t)DS_UNMAPPED_READ;
        return value;
    }
    inst->io_buffer = value;
    if ((enum hw_window)(offset >> 10u) == HW_IO) { /* the $1000 window is the one whose reads are logged (D-119) */
        log_dev(inst, bus, stamp, offset, value, (uint32_t)DS_DEV_READ);
    }
    return value;
}

/* kind is DS_DEV_WRITE for a write through the mapping registers and DS_DEV_ST for the physical write of ST0 to ST2 */
static void hw_write(struct ds_instance *inst, struct ds_bus *bus, uint32_t offset, uint8_t value, uint32_t kind)
{
    uint64_t stamp;

    charge_penalty(bus, offset);
    stamp = access_stamp(inst, bus, 0);
    switch ((enum hw_window)(offset >> 10u)) {
    case HW_VDC: /* the VDC and VCE windows fold to the access stamp and drop the byte (stubs until plans 04-03 and 04-04); the ST physical writes reach here too (D-137 (1), F-24) */
    case HW_VCE:
        ds_vdc_advance(inst, stamp, bus != NULL ? bus->obs : NULL);
        break;
    case HW_TIMER:
        inst->io_buffer = value;
        ds_timer_write(inst, offset & 0x3FFu, value, stamp);
        break;
    case HW_IRQ:
        inst->io_buffer = value;
        if (((offset & 0x3FFu) & 3u) >= 2u) { /* $1402 the disable register, $1403 the TIQ acknowledge; $1400 and $1401 name no register */
            ds_irq_write(inst, offset & 0x3FFu, value, stamp);
        }
        break;
    case HW_PSG:
        inst->io_buffer = value;
        break;
    case HW_IO: /* the byte is the latch of port O (bit 0 SEL, bit 1 CLR) and, as for every write in $0800-$17FF, the new io_buffer */
        inst->io_buffer = value;
        ds_io_write(inst, value);
        break;
    default: /* the empty windows */
        break;
    }
    if ((offset >> 10u) <= (uint32_t)HW_IRQ) { /* every write to windows 0 to 5 is logged, a device or not (D-119); windows 6 and 7 have none */
        log_dev(inst, bus, stamp, offset, value, kind);
    }
}

uint8_t ds_bus_read(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr)
{
    uint32_t bank = bank_of(inst, addr);
    uint32_t offset = offset_of(addr);
    uint8_t value = 0u;

    switch ((enum ds_bank_class)bank_class[bank]) {
    case BC_HUCARD:
        return ds_mapper_read(inst, bank, offset, &value) == (uint32_t)DS_MAP_OK ? value : (uint8_t)DS_UNMAPPED_READ;
    case BC_RAM:
        return inst->ram8k[offset & (DS_RAM8K_BYTES - 1u)];
    case BC_HWPAGE:
        return hw_read(inst, bus, offset);
    case BC_UNMAPPED:
    default:
        break;
    }
    return (uint8_t)DS_UNMAPPED_READ;
}

void ds_bus_write(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr, uint8_t value)
{
    uint32_t bank = bank_of(inst, addr);

    switch ((enum ds_bank_class)bank_class[bank]) {
    case BC_RAM:
        inst->ram8k[offset_of(addr) & (DS_RAM8K_BYTES - 1u)] = value;
        break;
    case BC_HWPAGE:
        hw_write(inst, bus, offset_of(addr), value, (uint32_t)DS_DEV_WRITE);
        break;
    case BC_HUCARD: /* ROM and every unmapped bank ignore a write (D-103, D-104) */
    case BC_UNMAPPED:
    default:
        break;
    }
}

/*
 * The physical write path of ST0, ST1 and ST2 (plan 03-14; D-106, H8-8 Table 2-2-3, S8-80 to S8-82): the address is physical $1FE000 | n with n in {0, 2, 3} and the mapping registers are never consulted.
 * Its only caller is the ST group of ds_cpu_step_obs, and a target outside the three is dropped (T-03-40).
 */
void ds_bus_write_phys(struct ds_instance *inst, struct ds_bus *bus, uint32_t phys, uint8_t value)
{
    if (phys == 0x1FE000u || phys == 0x1FE002u || phys == 0x1FE003u) {
        hw_write(inst, bus, phys & 0x1FFFu, value, (uint32_t)DS_DEV_ST); /* the VDC window: a stub that drops the byte until Phase 4 */
    }
}

void ds_mpr_write(struct ds_instance *inst, uint32_t index, uint8_t value)
{
    if (index < (uint32_t)DS_MPR_COUNT) {
        inst->mpr[index] = value;
    }
}
