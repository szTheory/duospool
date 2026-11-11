/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * cpu.c - the HuC6280 core of the tracer slice (plan 03-01; D-90, D-92, D-96, D-102..D-104, D-114) and, until plan 03-16 moves it into bus.c,
 * the single bus choke point. This is the minimal subset that the authored fixture ds_proto uses; plans 03-09 to 03-14 replace the table below
 * with the 256-row cpu_ops.def. Every opcode that is not in the table executes as the D-96 undefined-opcode behaviour: one byte, two cycles,
 * T cleared, nothing else changed (the length is an assumption of this project, LED-CPU-0009 and LED-CPU-0010 to come).
 * One step is one whole instruction: all state lives in the instance, no local survives a step (D-92). The step returns the master clocks it
 * took; ds_run adds them to the machine time. A cycle is 12 master clocks at low speed and 3 at high speed (D-90).
 * Bus: every CPU access translates through the eight mapping registers, phys = (mpr[a >> 13] << 13) | (a & $1FFF) (D-102). Banks below $80 read the
 * HuCard through ds_mapper_read and ignore writes; banks $F8-$FB are the 8 KiB work RAM mirrored four times; everything else reads $FF and
 * ignores writes until the hardware page lands in plan 03-16 (D-103, D-104). Nothing here peeks: observers never read through the bus (D-108).
 * All 8 and 16 bit machine arithmetic is explicit unsigned masking: the sanitize preset has no -fwrapv (D-83, Pitfall 14).
 * Cycle counts and bytes come from the HuC6280 software manual (S8-17 ADC, S8-20 and the instruction map H8-25); the rows below are the ones
 * ds_proto needs. No expected value anywhere is derived from this core.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"

/* processor status bits (H8-4): N V T B D I Z C; B exists only on the stack, so it has no mask here */
#define P_N 0x80u
#define P_V 0x40u
#define P_T 0x20u
#define P_D 0x08u
#define P_I 0x04u
#define P_Z 0x02u
#define P_C 0x01u

#define UNDEFINED_BYTES 1u
#define UNDEFINED_CYCLES 2u
#define BRANCH_TAKEN_CYCLES 2u /* D-98: a taken branch costs two more cycles, BRA included */

/* the 8 KiB work RAM is mirrored four times in banks $F8-$FB with & $1FFF (D-103): the mask is only right for this size */
_Static_assert(DS_RAM8K_BYTES == 0x2000u, "the work RAM mirror mask assumes 8 KiB");
_Static_assert(DS_MPR_COUNT == 8u, "the translate assumes eight mapping registers");

/* one row per implemented opcode: bytes, base cycles and the adder when a branch is taken (the manual's values, S8-17, S8-20, H8-25) */
struct op_row {
    uint8_t opcode;
    uint8_t bytes;
    uint8_t cycles;
    uint8_t taken;
};

static const struct op_row op_table[] = {
    {0x78u, 1u, 2u, 0u}, /* SEI */
    {0xD8u, 1u, 2u, 0u}, /* CLD */
    {0x18u, 1u, 2u, 0u}, /* CLC */
    {0xA9u, 2u, 2u, 0u}, /* LDA #imm */
    {0xADu, 3u, 5u, 0u}, /* LDA abs */
    {0xBDu, 3u, 5u, 0u}, /* LDA abs,X */
    {0x8Du, 3u, 5u, 0u}, /* STA abs */
    {0x9Eu, 3u, 5u, 0u}, /* STZ abs,X */
    {0xA2u, 2u, 2u, 0u}, /* LDX #imm */
    {0x9Au, 1u, 2u, 0u}, /* TXS */
    {0xE8u, 1u, 2u, 0u}, /* INX */
    {0x69u, 2u, 2u, 0u}, /* ADC #imm */
    {0x7Du, 3u, 5u, 0u}, /* ADC abs,X */
    {0xC9u, 2u, 2u, 0u}, /* CMP #imm */
    {0xD0u, 2u, 2u, BRANCH_TAKEN_CYCLES}, /* BNE */
    {0xF0u, 2u, 2u, BRANCH_TAKEN_CYCLES}, /* BEQ */
    {0x70u, 2u, 2u, BRANCH_TAKEN_CYCLES}, /* BVS */
    {0x80u, 2u, 2u, BRANCH_TAKEN_CYCLES}, /* BRA */
    {0x53u, 2u, 5u, 0u},                  /* TAM #mask */
};

#define OP_COUNT (sizeof op_table / sizeof op_table[0])

static const struct op_row *find_row(uint8_t opcode)
{
    size_t i;

    for (i = 0u; i < OP_COUNT; i++) {
        if (op_table[i].opcode == opcode) {
            return &op_table[i];
        }
    }
    return NULL;
}

uint32_t ds_cpu_max_cycles(void)
{
    uint32_t most = UNDEFINED_CYCLES;
    size_t i;

    for (i = 0u; i < OP_COUNT; i++) {
        uint32_t len = (uint32_t)op_table[i].cycles + (uint32_t)op_table[i].taken;

        if (len > most) {
            most = len;
        }
    }
    return most;
}

/* ---- the bus (D-102, D-103, D-104) ---- */

static uint32_t translate(const struct ds_instance *inst, uint16_t addr)
{
    return ((uint32_t)inst->mpr[(uint32_t)addr >> 13u] << 13u) | ((uint32_t)addr & 0x1FFFu);
}

uint8_t ds_bus_read(const struct ds_instance *inst, uint16_t addr)
{
    uint32_t phys = translate(inst, addr);
    uint32_t bank = phys >> 13u;
    uint32_t offset = phys & 0x1FFFu;
    uint8_t value = 0u;

    if (bank < DS_BANK_LIMIT) {
        return ds_mapper_read(inst, bank, offset, &value) == (uint32_t)DS_MAP_OK ? value : (uint8_t)DS_UNMAPPED_READ;
    }
    if (bank >= 0xF8u && bank <= 0xFBu) {
        return inst->ram8k[offset];
    }
    return (uint8_t)DS_UNMAPPED_READ;
}

void ds_bus_write(struct ds_instance *inst, uint16_t addr, uint8_t value)
{
    uint32_t phys = translate(inst, addr);
    uint32_t bank = phys >> 13u;

    if (bank >= 0xF8u && bank <= 0xFBu) {
        inst->ram8k[phys & 0x1FFFu] = value;
    }
    /* ROM and every unmapped bank ignore a write (D-103, D-104) */
}

void ds_mpr_write(struct ds_instance *inst, uint32_t index, uint8_t value)
{
    if (index < (uint32_t)DS_MPR_COUNT) {
        inst->mpr[index] = value;
    }
}

/* ---- reset ---- */

void ds_cpu_reset(struct ds_instance *inst)
{
    uint32_t lo;
    uint32_t hi;

    /* the facts of H8-18 and H8-8: I set, D and T clear, low speed, MPR7 = $00; the registers the manual leaves open are 0 under every policy of this plan */
    inst->a = 0u;
    inst->x = 0u;
    inst->y = 0u;
    inst->s = 0u;
    inst->p = (uint8_t)P_I;
    inst->speed = 0u;
    ds_mpr_write(inst, 7u, 0u);
    lo = ds_bus_read(inst, 0xFFFEu);
    hi = ds_bus_read(inst, 0xFFFFu);
    inst->pc = (uint16_t)(lo | (hi << 8u));
}

/* ---- the step ---- */

/* N and Z from an 8-bit result */
static void set_nz(struct ds_instance *inst, uint32_t value)
{
    uint32_t p = (uint32_t)inst->p & ~(P_N | P_Z);

    if ((value & 0x80u) != 0u) {
        p |= P_N;
    }
    if ((value & 0xFFu) == 0u) {
        p |= P_Z;
    }
    inst->p = (uint8_t)p;
}

/* binary ADC (S8-17): A + M + C, flags N V Z C. Decimal mode needs SED, which this subset does not execute, so D is never set here. */
static void adc(struct ds_instance *inst, uint32_t m)
{
    uint32_t a = inst->a;
    uint32_t sum = a + m + ((uint32_t)inst->p & P_C);
    uint32_t res = sum & 0xFFu;
    uint32_t p = (uint32_t)inst->p & ~(P_N | P_V | P_Z | P_C);

    if (sum > 0xFFu) {
        p |= P_C;
    }
    if (((a ^ res) & (m ^ res) & 0x80u) != 0u) {
        p |= P_V;
    }
    inst->p = (uint8_t)p;
    inst->a = (uint8_t)res;
    set_nz(inst, res);
}

/* CMP: A - M for N Z C only; C is set when A >= M */
static void cmp(struct ds_instance *inst, uint32_t m)
{
    uint32_t a = inst->a;
    uint32_t diff = (a - m) & 0xFFu;
    uint32_t p = (uint32_t)inst->p & ~P_C;

    if (a >= m) {
        p |= P_C;
    }
    inst->p = (uint8_t)p;
    set_nz(inst, diff);
}

/* the branch target: the address after the instruction plus the signed offset, with the sign extension done on unsigned values */
static uint16_t branch_target(uint32_t next, uint32_t rel)
{
    return (uint16_t)(next + rel - ((rel & 0x80u) != 0u ? 0x100u : 0u));
}

uint32_t ds_cpu_step(struct ds_instance *inst)
{
    uint32_t pc = inst->pc;
    uint8_t opcode = ds_bus_read(inst, inst->pc);
    const struct op_row *row = find_row(opcode);
    uint32_t bytes = row != NULL ? (uint32_t)row->bytes : UNDEFINED_BYTES;
    uint32_t cycles = row != NULL ? (uint32_t)row->cycles : UNDEFINED_CYCLES;
    uint32_t op1 = bytes >= 2u ? ds_bus_read(inst, (uint16_t)(pc + 1u)) : 0u;
    uint32_t op2 = bytes >= 3u ? ds_bus_read(inst, (uint16_t)(pc + 2u)) : 0u;
    uint32_t next = (pc + bytes) & 0xFFFFu;
    uint16_t abs_addr = (uint16_t)(op1 | (op2 << 8u));
    uint16_t abs_x = (uint16_t)((uint32_t)abs_addr + (uint32_t)inst->x);
    int taken = 0;
    uint32_t i;

    inst->p = (uint8_t)((uint32_t)inst->p & ~P_T); /* T is reset in the fetch cycle of every instruction (H8-5) */
    switch (opcode) {
    case 0x78u: /* SEI */
        inst->p = (uint8_t)((uint32_t)inst->p | P_I);
        break;
    case 0xD8u: /* CLD */
        inst->p = (uint8_t)((uint32_t)inst->p & ~P_D);
        break;
    case 0x18u: /* CLC */
        inst->p = (uint8_t)((uint32_t)inst->p & ~P_C);
        break;
    case 0xA9u: /* LDA #imm */
        inst->a = (uint8_t)op1;
        set_nz(inst, op1);
        break;
    case 0xADu: /* LDA abs */
        inst->a = ds_bus_read(inst, abs_addr);
        set_nz(inst, inst->a);
        break;
    case 0xBDu: /* LDA abs,X */
        inst->a = ds_bus_read(inst, abs_x);
        set_nz(inst, inst->a);
        break;
    case 0x8Du: /* STA abs */
        ds_bus_write(inst, abs_addr, inst->a);
        break;
    case 0x9Eu: /* STZ abs,X */
        ds_bus_write(inst, abs_x, 0u);
        break;
    case 0xA2u: /* LDX #imm */
        inst->x = (uint8_t)op1;
        set_nz(inst, op1);
        break;
    case 0x9Au: /* TXS: no flags */
        inst->s = inst->x;
        break;
    case 0xE8u: /* INX */
        inst->x = (uint8_t)(((uint32_t)inst->x + 1u) & 0xFFu);
        set_nz(inst, inst->x);
        break;
    case 0x69u: /* ADC #imm */
        adc(inst, op1);
        break;
    case 0x7Du: /* ADC abs,X */
        adc(inst, ds_bus_read(inst, abs_x));
        break;
    case 0xC9u: /* CMP #imm */
        cmp(inst, op1);
        break;
    case 0xD0u: /* BNE */
        taken = ((uint32_t)inst->p & P_Z) == 0u;
        break;
    case 0xF0u: /* BEQ */
        taken = ((uint32_t)inst->p & P_Z) != 0u;
        break;
    case 0x70u: /* BVS */
        taken = ((uint32_t)inst->p & P_V) != 0u;
        break;
    case 0x80u: /* BRA */
        taken = 1;
        break;
    case 0x53u: /* TAM #mask: every selected MPR gets A (D-105: one-hot is the documented use) */
        for (i = 0u; i < (uint32_t)DS_MPR_COUNT; i++) {
            if ((op1 & (1u << i)) != 0u) {
                ds_mpr_write(inst, i, inst->a);
            }
        }
        break;
    default: /* D-96: undefined and not-yet-implemented opcodes are one byte, two cycles, T cleared */
        break;
    }
    if (taken) {
        next = branch_target(next, op1);
        cycles += row != NULL ? (uint32_t)row->taken : 0u;
    }
    inst->pc = (uint16_t)next;
    return cycles * (inst->speed != 0u ? DS_MCLK_PER_CYCLE_HIGH : DS_MCLK_PER_CYCLE_LOW);
}
