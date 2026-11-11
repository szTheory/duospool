/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * cpu.c - the HuC6280 core of the tracer slice (plan 03-01; D-90, D-92, D-96, D-114); the bus choke point moved to bus.c in plan 03-16.
 * Plan 03-09 replaced the hand-written subset by the 256-row table of cpu_ops.def (opcode, mnemonic, mode, bytes, cycles, access class, cite) expanded into static const metadata and a
 * mnemonic switch. A mnemonic whose semantics land in a later plan (the block transfers, plan 03-25) advances PC by the table length and charges the table cycles with no other effect until then (the case labels name the plan).
 * One step is one whole instruction: all state lives in the instance, no local survives a step (D-92). The step returns the master clocks it took; ds_run adds them to the machine time.
 * A cycle is 12 master clocks at low speed and 3 at high speed (D-90). The 22 undefined opcodes are one byte, two cycles, T cleared (the length is an assumption of this project, LED-CPU-0010).
 * Bus: every CPU access goes through ds_bus_read and ds_bus_write of bus.c (D-102 to D-104): the translate, the bank classes and the hardware page live there, not here.
 * Nothing here peeks: observers never read through the bus (D-108). All 8 and 16 bit machine arithmetic is explicit unsigned masking: the sanitize preset has no -fwrapv (D-83, Pitfall 14).
 * The table's bytes and cycles are checked against an independent transcription of the manual by scripts/opcode-check (stage opcode-oracle); no expected value anywhere is derived from this core.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_obs.h"

/* processor status bits (H8-4): N V T B D I Z C; B exists only on the stack, so it has no mask here */
#define P_N 0x80u
#define P_V 0x40u
#define P_T 0x20u
#define P_D 0x08u
#define P_I 0x04u
#define P_Z 0x02u
#define P_C 0x01u


_Static_assert(DS_MPR_COUNT == 8u, "the translate assumes eight mapping registers");

/*
 * The opcode table (D-96): src/core/cpu_ops.def has one DS_OP row per opcode, expanded here into static const metadata. The mnemonic list below names each mnemonic once; the dispatch
 * in ds_cpu_step is a switch over it with no default, so -Wswitch fails the build when a mnemonic has no handler. The enum order is the order of this list; no value is stored anywhere.
 */
#define DS_MNEMONICS(X)                                                                                                                                                            \
    X(UNDEF) X(ADC) X(AND) X(ASL) X(BBR0) X(BBR1) X(BBR2) X(BBR3) X(BBR4) X(BBR5) X(BBR6) X(BBR7) X(BBS0) X(BBS1) X(BBS2) X(BBS3) X(BBS4) X(BBS5) X(BBS6) X(BBS7)                  \
    X(BCC) X(BCS) X(BEQ) X(BIT) X(BMI) X(BNE) X(BPL) X(BRA) X(BRK) X(BSR) X(BVC) X(BVS) X(CLA) X(CLC) X(CLD) X(CLI) X(CLV) X(CLX) X(CLY) X(CMP) X(CPX) X(CPY) X(CSH) X(CSL)          \
    X(DEC) X(DEX) X(DEY) X(EOR) X(INC) X(INX) X(INY) X(JMP) X(JSR) X(LDA) X(LDX) X(LDY) X(LSR) X(NOP) X(ORA) X(PHA) X(PHP) X(PHX) X(PHY) X(PLA) X(PLP) X(PLX) X(PLY)              \
    X(RMB0) X(RMB1) X(RMB2) X(RMB3) X(RMB4) X(RMB5) X(RMB6) X(RMB7) X(ROL) X(ROR) X(RTI) X(RTS) X(SAX) X(SAY) X(SBC) X(SEC) X(SED) X(SEI) X(SET)                                \
    X(SMB0) X(SMB1) X(SMB2) X(SMB3) X(SMB4) X(SMB5) X(SMB6) X(SMB7) X(ST0) X(ST1) X(ST2) X(STA) X(STX) X(STY) X(STZ) X(SXY) X(TAI) X(TAM) X(TAX) X(TAY) X(TDD) X(TIA) X(TII)       \
    X(TIN) X(TMA) X(TRB) X(TSB) X(TST) X(TSX) X(TXA) X(TXS) X(TYA)

enum ds_mn {
#define DS_MN(name) MN_##name,
    DS_MNEMONICS(DS_MN)
#undef DS_MN
};

/* addressing modes, the names of the opcode table; the access classes of D-92 are carried as metadata for the bus offset rule of the bus plan */
enum ds_mode {
    AM_IMPLIED, AM_ACC, AM_IMM, AM_ZP, AM_ZPX, AM_ZPY, AM_ABS, AM_ABSX, AM_ABSY, AM_IND, AM_INDX, AM_INDY, AM_ABSIND, AM_ABSINDX, AM_REL, AM_ZPREL, AM_BLK, AM_MASK,
    AM_IMMZP, AM_IMMZPX, AM_IMMABS, AM_IMMABSX, AM_UNDEF
};
enum ds_class { CL_NONE, CL_READ, CL_WRITE, CL_RMW, CL_STACK, CL_MPR, CL_BLOCK };

struct ds_op {
    uint8_t mn;     /* enum ds_mn */
    uint8_t mode;   /* enum ds_mode */
    uint8_t bytes;  /* instruction length */
    uint8_t cycles; /* the manual's base count; a block transfer holds its 17 */
    uint8_t cls;    /* enum ds_class */
};

static const struct ds_op ops[256] = {
#define DS_OP(opcode, mn, mode, nbytes, ncycles, cls, cite) [opcode] = {(uint8_t)MN_##mn, (uint8_t)AM_##mode, nbytes, ncycles, (uint8_t)CL_##cls},
#include "cpu_ops.def"
#undef DS_OP
};

/* the def holds exactly one row per opcode: a row count other than 256 fails the build (an absent row would be a zeroed entry); the opcode order and the content are checked by scripts/opcode-check */
#define DS_OP(opcode, mn, mode, nbytes, ncycles, cls, cite) +1
enum { DS_OP_ROWS = 0
#include "cpu_ops.def"
};
#undef DS_OP
_Static_assert(DS_OP_ROWS == 256, "cpu_ops.def must hold exactly 256 rows");

/* the adders of D-98, each one cited in the ledger (LED-CPU-0002): they are added to the table's base count and never stored in it */
static uint32_t taken_adder(uint32_t mn)
{
    switch ((enum ds_mn)mn) {
    case MN_BPL: case MN_BMI: case MN_BVC: case MN_BVS: case MN_BCC: case MN_BCS: case MN_BNE: case MN_BEQ:
    case MN_BBR0: case MN_BBR1: case MN_BBR2: case MN_BBR3: case MN_BBR4: case MN_BBR5: case MN_BBR6: case MN_BBR7:
    case MN_BBS0: case MN_BBS1: case MN_BBS2: case MN_BBS3: case MN_BBS4: case MN_BBS5: case MN_BBS6: case MN_BBS7:
        return DS_ADDER_BRANCH_TAKEN; /* BRA, BSR and the others are fixed counts in the table */
    default:
        return 0u;
    }
}

/* T mode (memory-to-memory through $2000+X) and decimal mode add cycles to these mnemonics only (S8-17, S8-18, S8-47, S8-58, S8-74). The two predicates are the one place that says which: the worst case below and the adders of one execution use them. */
static int t_mode_mnemonic(uint32_t mn)
{
    return mn == (uint32_t)MN_ADC || mn == (uint32_t)MN_AND || mn == (uint32_t)MN_EOR || mn == (uint32_t)MN_ORA;
}

static int decimal_mnemonic(uint32_t mn)
{
    return mn == (uint32_t)MN_ADC || mn == (uint32_t)MN_SBC;
}

/* the cycles the active flags add to one execution: T mode +3 and decimal +1 apply together on ADC (the sum +4 is the project's reading: no page states the combination, ASSUMED, LED-CPU-0010) */
static uint32_t flag_cycles(uint32_t mn, int t_active, uint32_t p)
{
    uint32_t extra = 0u;

    if (t_active != 0 && t_mode_mnemonic(mn) != 0) {
        extra += DS_ADDER_T_MODE;
    }
    if ((p & P_D) != 0u && decimal_mnemonic(mn) != 0) {
        extra += DS_ADDER_DECIMAL;
    }
    return extra;
}

/* the worst case of flag_cycles: both flags active */
static uint32_t flag_adders(uint32_t mn)
{
    return flag_cycles(mn, 1, P_D);
}

/* the longest this opcode can take in cycles: the table count plus every adder that can apply to it (D-94); the VDC and VCE penalty is one cycle per bus access and an access occupies at least one cycle, so at most as many accesses as the table count can be charged (D-107). A block transfer counts its 17 here (the table value). */
uint32_t ds_cpu_op_max_cycles(uint32_t opcode)
{
    const struct ds_op *op = &ops[opcode & 0xFFu];

    return (uint32_t)op->cycles + taken_adder(op->mn) + flag_adders(op->mn) + (uint32_t)op->cycles * (uint32_t)DS_ADDER_VDC_ACCESS;
}

/* ---- the opcode table accessors ---- */

int ds_cpu_op_defined(uint32_t opcode)
{
    return ops[opcode & 0xFFu].mn != (uint32_t)MN_UNDEF;
}

/* the table's byte count and base cycle count of one opcode, read back for the manual comparison (plan 03-15; D-97): a private accessor, no machine state; opcode is masked to 8 bits, a NULL out-pointer is skipped */
void ds_op_meta(uint32_t opcode, uint32_t *bytes, uint32_t *cycles)
{
    const struct ds_op *op = &ops[opcode & 0xFFu];

    if (bytes != NULL) {
        *bytes = (uint32_t)op->bytes;
    }
    if (cycles != NULL) {
        *cycles = (uint32_t)op->cycles;
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
    lo = ds_bus_read(inst, NULL, 0xFFFEu); /* the reset costs no time (D-92): no bus accounting */
    hi = ds_bus_read(inst, NULL, 0xFFFFu);
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

/* CMP, CPX, CPY (S8-41 to S8-43): reg - M for N Z C only, V untouched; C is set when reg >= M (the carry flag of the subtraction, never the H8-4 prose about borrows: research mismatch 6) */
static void compare(struct ds_instance *inst, uint32_t reg, uint32_t m)
{
    uint32_t diff = (reg - m) & 0xFFu;
    uint32_t p = (uint32_t)inst->p & ~P_C;

    if (reg >= m) {
        p |= P_C;
    }
    inst->p = (uint8_t)p;
    set_nz(inst, diff);
}

/* the stack is logical $2100 + S through the normal bus (S8-11: it wraps inside $21xx); a push writes then decrements, a pull increments then reads */
static void push(struct ds_instance *inst, struct ds_bus *bus, uint32_t value)
{
    ds_bus_write(inst, bus, (uint16_t)(0x2100u | (uint32_t)inst->s), (uint8_t)value);
    inst->s = (uint8_t)(((uint32_t)inst->s - 1u) & 0xFFu);
}

static uint32_t pull(struct ds_instance *inst, struct ds_bus *bus)
{
    inst->s = (uint8_t)(((uint32_t)inst->s + 1u) & 0xFFu);
    return ds_bus_read(inst, bus, (uint16_t)(0x2100u | (uint32_t)inst->s));
}

/* the branch target: the address after the instruction plus the signed offset, with the sign extension done on unsigned values */
static uint16_t branch_target(uint32_t next, uint32_t rel)
{
    return (uint16_t)(next + rel - ((rel & 0x80u) != 0u ? 0x100u : 0u));
}

/* the modes that name a memory cell: the nine of the load and store family (plan 03-11) */
static int has_ea(uint32_t mode)
{
    return mode == (uint32_t)AM_ZP || mode == (uint32_t)AM_ZPX || mode == (uint32_t)AM_ZPY || mode == (uint32_t)AM_IND || mode == (uint32_t)AM_INDX || mode == (uint32_t)AM_INDY
           || mode == (uint32_t)AM_ABS || mode == (uint32_t)AM_ABSX || mode == (uint32_t)AM_ABSY;
}

/*
 * The zero page is logical $2000-$20FF, so a zero-page address is $2000 | zz, and an indexed zero-page address is $2000 | ((zz + index) & $FF): the high byte is always $20 (S8-11, D-99), never a carry into
 * $21xx. A zero-page pointer takes its high byte from $2000 | ((zz + 1) & $FF), wrapping inside the page like the index (the project's reading, LED-CPU-0010). The 16-bit sums wrap at $FFFF (masks, no signed arithmetic).
 * Every cell goes through the bus (ds_bus_read translates through the MPRs, D-102); the pointer bytes are read here, so a mode that is not executed never touches memory.
 */
static uint16_t zp_addr(uint32_t zz)
{
    return (uint16_t)(0x2000u | (zz & 0xFFu));
}

static uint16_t zp_pointer(struct ds_instance *inst, struct ds_bus *bus, uint32_t zz)
{
    uint32_t lo = ds_bus_read(inst, bus, zp_addr(zz));
    uint32_t hi = ds_bus_read(inst, bus, zp_addr(zz + 1u));

    return (uint16_t)(lo | (hi << 8u));
}

static uint16_t eff_addr(struct ds_instance *inst, struct ds_bus *bus, uint32_t mode, uint32_t op1, uint32_t abs_addr)
{
    switch ((enum ds_mode)mode) {
    case AM_ZPX:
        return zp_addr(op1 + (uint32_t)inst->x);
    case AM_ZPY:
        return zp_addr(op1 + (uint32_t)inst->y);
    case AM_IND:
        return zp_pointer(inst, bus, op1);
    case AM_INDX:
        return zp_pointer(inst, bus, (op1 + (uint32_t)inst->x) & 0xFFu);
    case AM_INDY:
        return (uint16_t)((uint32_t)zp_pointer(inst, bus, op1) + (uint32_t)inst->y);
    case AM_ABSX:
        return (uint16_t)(abs_addr + (uint32_t)inst->x);
    case AM_ABSY:
        return (uint16_t)(abs_addr + (uint32_t)inst->y);
    case AM_ABS:
        return (uint16_t)abs_addr;
    default: /* AM_ZP, and the modes that never reach here (has_ea is checked first) */
        return zp_addr(op1);
    }
}

/* BBR0-7 and BBS0-7 (S8-20, S8-21): the branch is taken when bit i of the zero-page cell ($2000 | zz) is 0 (BBR) or 1 (BBS); the enum keeps BBR0 to BBR7 and BBS0 to BBS7 in this order */
static int bit_branch(struct ds_instance *inst, struct ds_bus *bus, uint32_t mn, uint32_t zz)
{
    uint32_t cell = ds_bus_read(inst, bus, zp_addr(zz));
    uint32_t bit = mn >= (uint32_t)MN_BBS0 ? mn - (uint32_t)MN_BBS0 : mn - (uint32_t)MN_BBR0;
    uint32_t set = (cell >> bit) & 1u;

    return mn >= (uint32_t)MN_BBS0 ? set != 0u : set == 0u;
}

/* the value of an operand: immediate, or a read of the effective address; 0 for a mode that holds no operand (those land with their group, plans 03-12 to 03-14, and have no effect until then) */
static int operand_value(struct ds_instance *inst, struct ds_bus *bus, uint32_t mode, uint32_t op1, uint32_t abs_addr, uint32_t *value)
{
    if (mode == (uint32_t)AM_IMM) {
        *value = op1;
        return 1;
    }
    if (has_ea(mode)) {
        *value = ds_bus_read(inst, bus, eff_addr(inst, bus, mode, op1, abs_addr));
        return 1;
    }
    return 0;
}

/* a store: one write at the effective address (flags are never touched by a store, S8-83 to S8-86) */
static void store_operand(struct ds_instance *inst, struct ds_bus *bus, uint32_t mode, uint32_t op1, uint32_t abs_addr, uint32_t value)
{
    if (has_ea(mode)) {
        ds_bus_write(inst, bus, eff_addr(inst, bus, mode, op1, abs_addr), (uint8_t)value);
    }
}

/*
 * The accumulator-class operations (S8-17, S8-18, S8-47, S8-58): ADC, AND, EOR and ORA work on A, except right after SET, when they work on the zero-page cell named by X, logical $2000 | X, and A is left alone.
 * acc_in reads the operand the operation changes (A, or that cell); acc_out writes the result back to it. t_mode is the value of T before this instruction (H8-5: T is cleared by the next instruction's fetch).
 */
static uint16_t t_cell(const struct ds_instance *inst)
{
    return zp_addr((uint32_t)inst->x);
}

static uint32_t acc_in(struct ds_instance *inst, struct ds_bus *bus, int t_mode)
{
    return t_mode != 0 ? (uint32_t)ds_bus_read(inst, bus, t_cell(inst)) : (uint32_t)inst->a;
}

static void acc_out(struct ds_instance *inst, struct ds_bus *bus, int t_mode, uint32_t value)
{
    if (t_mode != 0) {
        ds_bus_write(inst, bus, t_cell(inst), (uint8_t)value);
    } else {
        inst->a = (uint8_t)value;
    }
}

/* AND, ORA, EOR: N and Z from the result, no other flag (S8-18, S8-47, S8-58) */
static void logic_op(struct ds_instance *inst, struct ds_bus *bus, uint32_t mn, int t_mode, uint32_t m)
{
    uint32_t acc = acc_in(inst, bus, t_mode);
    uint32_t res;

    switch ((enum ds_mn)mn) {
    case MN_AND:
        res = acc & m;
        break;
    case MN_ORA:
        res = acc | m;
        break;
    default: /* MN_EOR */
        res = acc ^ m;
        break;
    }
    acc_out(inst, bus, t_mode, res);
    set_nz(inst, res);
}

/* binary add (S8-17): a + m + C, flags N V Z C; the 9-bit sum keeps the carry out, V is the signed overflow ((a ^ r) & (m ^ r) & $80, Pitfall 14). SBC reaches this with m complemented (A - M - not C, S8-74). */
static uint32_t add_binary(struct ds_instance *inst, uint32_t a, uint32_t m)
{
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
    set_nz(inst, res);
    return res;
}

/*
 * Decimal add and subtract on valid BCD (S8-17, S8-74): the cycle count grows by one and V is unaffected (both from the pages); the result is the packed BCD sum or difference of the two digits, each digit adjusted by 6 (add) or by -6
 * (subtract). N, Z and C are taken from the adjusted result and the decimal carry (D-99, an assumption, LED-CPU-0010); what a decimal operation does with a digit above 9 is not defined here and is not tested.
 */
static uint32_t add_decimal(struct ds_instance *inst, uint32_t a, uint32_t m)
{
    uint32_t lo = (a & 0x0Fu) + (m & 0x0Fu) + ((uint32_t)inst->p & P_C);
    uint32_t hi;
    uint32_t res;
    uint32_t p = (uint32_t)inst->p & ~(P_N | P_Z | P_C);

    if (lo > 9u) {
        lo += 6u;
    }
    hi = (a >> 4u) + (m >> 4u) + (lo >> 4u);
    if (hi > 9u) {
        hi += 6u;
        p |= P_C;
    }
    res = ((hi & 0x0Fu) << 4u) | (lo & 0x0Fu);
    inst->p = (uint8_t)p;
    set_nz(inst, res);
    return res;
}

static uint32_t sub_decimal(struct ds_instance *inst, uint32_t a, uint32_t m)
{
    uint32_t borrow_in = 1u - ((uint32_t)inst->p & P_C); /* A - M - not C (S8-74) */
    uint32_t lo = (a & 0x0Fu) + 16u - (m & 0x0Fu) - borrow_in;
    uint32_t hi;
    uint32_t res;
    uint32_t p = (uint32_t)inst->p & ~(P_N | P_Z | P_C);
    uint32_t borrow_lo = lo < 16u ? 1u : 0u;

    lo = borrow_lo != 0u ? ((lo + 10u) & 0x0Fu) : (lo & 0x0Fu); /* a borrowed digit is adjusted by -6, which is +10 modulo 16 */
    hi = (a >> 4u) + 16u - (m >> 4u) - borrow_lo;
    if (hi >= 16u) {
        p |= P_C; /* no borrow out */
        hi &= 0x0Fu;
    } else {
        hi = (hi + 10u) & 0x0Fu;
    }
    res = (hi << 4u) | lo;
    inst->p = (uint8_t)p;
    set_nz(inst, res);
    return res;
}

/* ADC: the T-mode cell or A is the first addend; D selects the decimal add (T mode with D is the project's reading of a combination no page states, LED-CPU-0010) */
static void adc_op(struct ds_instance *inst, struct ds_bus *bus, int t_mode, uint32_t m)
{
    uint32_t acc = acc_in(inst, bus, t_mode);
    uint32_t res = ((uint32_t)inst->p & P_D) != 0u ? add_decimal(inst, acc, m) : add_binary(inst, acc, m);

    acc_out(inst, bus, t_mode, res);
}

/* SBC (S8-74): A - M - not C; binary mode adds the complement of M, so the same adder serves; there is no T-mode form */
static void sbc_op(struct ds_instance *inst, uint32_t m)
{
    uint32_t a = inst->a;

    inst->a = (uint8_t)(((uint32_t)inst->p & P_D) != 0u ? sub_decimal(inst, a, m) : add_binary(inst, a, m ^ 0xFFu));
}

/* ASL, LSR, ROL, ROR, INC, DEC on a value (S8-19, S8-56, S8-68, S8-69, S8-48, S8-44): the shifts set N Z C (C is the bit shifted out), INC and DEC set N and Z only */
static uint32_t rmw_value(struct ds_instance *inst, uint32_t mn, uint32_t v)
{
    uint32_t cin = (uint32_t)inst->p & P_C;
    uint32_t cout = 0u;
    uint32_t res;
    uint32_t p = (uint32_t)inst->p & ~P_C;

    switch ((enum ds_mn)mn) {
    case MN_ASL:
        res = (v << 1u) & 0xFFu;
        cout = (v >> 7u) & 1u;
        break;
    case MN_LSR:
        res = v >> 1u;
        cout = v & 1u;
        break;
    case MN_ROL:
        res = ((v << 1u) | cin) & 0xFFu;
        cout = (v >> 7u) & 1u;
        break;
    case MN_ROR:
        res = (v >> 1u) | (cin << 7u);
        cout = v & 1u;
        break;
    case MN_INC:
        res = (v + 1u) & 0xFFu;
        cout = cin; /* C is not in the flag row of INC and DEC: it stays as it was */
        break;
    default: /* MN_DEC */
        res = (v - 1u) & 0xFFu;
        cout = cin;
        break;
    }
    if (cout != 0u) {
        p |= P_C;
    }
    inst->p = (uint8_t)p;
    set_nz(inst, res);
    return res;
}

/* BIT, TSB, TRB, TST: Z from first AND v; N and V from bits 7 and 6 of the memory (or, for BIT immediate, the operand) byte v (S8-25, S8-102 to S8-104, H8-5); C is untouched. The N and V of TSB and TRB are the page's reading (research mismatch 9). */
static void bit_test(struct ds_instance *inst, uint32_t first, uint32_t v)
{
    uint32_t p = (uint32_t)inst->p & ~(P_N | P_V | P_Z);

    if ((v & 0x80u) != 0u) {
        p |= P_N;
    }
    if ((v & 0x40u) != 0u) {
        p |= P_V;
    }
    if ((first & v) == 0u) {
        p |= P_Z;
    }
    inst->p = (uint8_t)p;
}

/* the memory cell of TST (S8-104): the encoding is opcode, immediate, then the address, so zero page or the low byte come second and the high byte fourth */
static uint16_t tst_addr(const struct ds_instance *inst, uint32_t mode, uint32_t op2, uint32_t op3)
{
    switch ((enum ds_mode)mode) {
    case AM_IMMZPX:
        return zp_addr(op2 + (uint32_t)inst->x);
    case AM_IMMABS:
        return (uint16_t)(op2 | (op3 << 8u));
    case AM_IMMABSX:
        return (uint16_t)((op2 | (op3 << 8u)) + (uint32_t)inst->x);
    default: /* AM_IMMZP */
        return zp_addr(op2);
    }
}

/* one instruction; log is the instruction sink writer of the running ds_run call or NULL (plan 03-10): the hook is called once per executed instruction with the state before it runs (D-119) */
uint32_t ds_cpu_step_obs(struct ds_instance *inst, struct ds_obs_log *log)
{
    struct ds_bus bus_state = {0u}; /* what this instruction's bus accesses add to it: a local of the step, nothing survives it (D-92) */
    struct ds_bus *bus = &bus_state;
    uint32_t pc = inst->pc;
    uint8_t opcode = ds_bus_read(inst, bus, inst->pc);
    const struct ds_op *op = &ops[opcode];
    uint32_t mn = op->mn;
    uint32_t mode = op->mode;
    uint32_t bytes = op->bytes; /* the 22 undefined rows hold one byte and two cycles: the D-96 assumption (LED-CPU-0010) lives in the def */
    uint32_t cycles = op->cycles;
    uint32_t op1 = bytes >= 2u ? ds_bus_read(inst, bus, (uint16_t)(pc + 1u)) : 0u;
    uint32_t op2 = bytes >= 3u ? ds_bus_read(inst, bus, (uint16_t)(pc + 2u)) : 0u;
    uint32_t next = (pc + bytes) & 0xFFFFu;
    uint32_t abs_addr = op1 | (op2 << 8u);
    uint32_t v = 0u;
    uint16_t ea;
    uint32_t lo;
    uint32_t hi;
    int taken = 0;
    uint32_t i;
    int t_active = ((uint32_t)inst->p & P_T) != 0u; /* T as the instruction before this one left it: only ADC, AND, EOR and ORA read it (S8-17) */
    uint32_t mclk_per_cycle = inst->speed != 0u ? DS_MCLK_PER_CYCLE_HIGH : DS_MCLK_PER_CYCLE_LOW; /* the speed this instruction starts at: CSL and CSH are charged at the old speed, the new one applies from the next instruction (D-101) */

    ds_obs_insn(log, inst, opcode); /* a void hook over a const instance: the observer cannot change the machine (D-119) */
    inst->p = (uint8_t)((uint32_t)inst->p & ~P_T); /* T is reset in the fetch cycle of every instruction (H8-5) */
    switch ((enum ds_mn)mn) {
    case MN_SEI:
        inst->p = (uint8_t)((uint32_t)inst->p | P_I);
        break;
    case MN_CLD:
        inst->p = (uint8_t)((uint32_t)inst->p & ~P_D);
        break;
    case MN_CLC:
        inst->p = (uint8_t)((uint32_t)inst->p & ~P_C);
        break;
    case MN_SEC: /* C <- 1, no other flag (S8-75); SEC, SED and CLV are plan 03-13's flag instructions, done here because this group's fixture needs them */
        inst->p = (uint8_t)((uint32_t)inst->p | P_C);
        break;
    case MN_SED: /* D <- 1 (S8-76) */
        inst->p = (uint8_t)((uint32_t)inst->p | P_D);
        break;
    case MN_CLV: /* V <- 0 */
        inst->p = (uint8_t)((uint32_t)inst->p & ~P_V);
        break;
    case MN_SET: /* T <- 1 after the fetch-time clear: the next ADC, AND, EOR or ORA works on the cell named by X (S8-78) */
        inst->p = (uint8_t)((uint32_t)inst->p | P_T);
        break;
    case MN_LDA:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            inst->a = (uint8_t)v;
            set_nz(inst, v);
        }
        break;
    case MN_LDX:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            inst->x = (uint8_t)v;
            set_nz(inst, v);
        }
        break;
    case MN_LDY:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            inst->y = (uint8_t)v;
            set_nz(inst, v);
        }
        break;
    case MN_STA: /* stores take no flags (S8-83 to S8-86) */
        store_operand(inst, bus, mode, op1, abs_addr, inst->a);
        break;
    case MN_STX:
        store_operand(inst, bus, mode, op1, abs_addr, inst->x);
        break;
    case MN_STY:
        store_operand(inst, bus, mode, op1, abs_addr, inst->y);
        break;
    case MN_STZ:
        store_operand(inst, bus, mode, op1, abs_addr, 0u);
        break;
    case MN_TAX: /* N and Z from the moved value (S8-91) */
        inst->x = inst->a;
        set_nz(inst, inst->x);
        break;
    case MN_TAY:
        inst->y = inst->a;
        set_nz(inst, inst->y);
        break;
    case MN_TXA:
        inst->a = inst->x;
        set_nz(inst, inst->a);
        break;
    case MN_TYA:
        inst->a = inst->y;
        set_nz(inst, inst->a);
        break;
    case MN_TSX: /* N and Z from S (S8-105) */
        inst->x = inst->s;
        set_nz(inst, inst->x);
        break;
    case MN_TXS: /* no flags (S8-107) */
        inst->s = inst->x;
        break;
    case MN_CLA: /* the register clears take no flags (S8-34, S8-39, S8-40) */
        inst->a = 0u;
        break;
    case MN_CLX:
        inst->x = 0u;
        break;
    case MN_CLY:
        inst->y = 0u;
        break;
    case MN_SXY: /* X and Y exchange, no flag changes (S8-87); SAX and SAY exchange A with X or Y, no flag changes (S8-72, S8-73) */
        v = inst->x;
        inst->x = inst->y;
        inst->y = (uint8_t)v;
        break;
    case MN_SAX:
        v = inst->a;
        inst->a = inst->x;
        inst->x = (uint8_t)v;
        break;
    case MN_SAY:
        v = inst->a;
        inst->a = inst->y;
        inst->y = (uint8_t)v;
        break;
    case MN_RMB0: case MN_RMB1: case MN_RMB2: case MN_RMB3: case MN_RMB4: case MN_RMB5: case MN_RMB6: case MN_RMB7: /* the specified bit of a zero-page cell is cleared, no flag changes (S8-67): one read, one write (LED-CPU-0010) */
        ea = zp_addr(op1);
        ds_bus_write(inst, bus, ea, (uint8_t)((uint32_t)ds_bus_read(inst, bus, ea) & ~(1u << (mn - (uint32_t)MN_RMB0))));
        break;
    case MN_SMB0: case MN_SMB1: case MN_SMB2: case MN_SMB3: case MN_SMB4: case MN_SMB5: case MN_SMB6: case MN_SMB7: /* the specified bit is set (S8-79) */
        ea = zp_addr(op1);
        ds_bus_write(inst, bus, ea, (uint8_t)((uint32_t)ds_bus_read(inst, bus, ea) | (1u << (mn - (uint32_t)MN_SMB0))));
        break;
    case MN_TMA: /* A <- the selected MPR, no flag changes (S8-101): the one-hot operand is the documented use; a multi-bit operand returns the highest selected index (D-105); operand 0 leaves A alone until plan 03-17 adds tma_zero */
        for (i = 0u; i < (uint32_t)DS_MPR_COUNT; i++) {
            if ((op1 & (1u << i)) != 0u) {
                inst->a = inst->mpr[i];
            }
        }
        break;
    case MN_ST0: /* the immediate goes to physical $1FE000, $1FE002 or $1FE003 whatever the MPRs hold (H8-8 Table 2-2-3, S8-80 to S8-82, D-106); no flag changes */
        ds_bus_write_phys(inst, bus, 0x1FE000u, (uint8_t)op1);
        break;
    case MN_ST1:
        ds_bus_write_phys(inst, bus, 0x1FE002u, (uint8_t)op1);
        break;
    case MN_ST2:
        ds_bus_write_phys(inst, bus, 0x1FE003u, (uint8_t)op1);
        break;
    case MN_CSL: /* the speed bit is state (D-101): CSL selects low and CSH high, 1 byte and 3 cycles in the H8-25 map, no software-manual page; the cycles of this instruction are charged at the old speed below */
        inst->speed = 0u;
        break;
    case MN_CSH:
        inst->speed = 1u;
        break;
    case MN_PHX:
        push(inst, bus, inst->x);
        break;
    case MN_PHY:
        push(inst, bus, inst->y);
        break;
    case MN_PLX: /* N and Z */
        inst->x = (uint8_t)pull(inst, bus);
        set_nz(inst, inst->x);
        break;
    case MN_PLY:
        inst->y = (uint8_t)pull(inst, bus);
        set_nz(inst, inst->y);
        break;
    case MN_INX:
        inst->x = (uint8_t)(((uint32_t)inst->x + 1u) & 0xFFu);
        set_nz(inst, inst->x);
        break;
    case MN_INY:
        inst->y = (uint8_t)(((uint32_t)inst->y + 1u) & 0xFFu);
        set_nz(inst, inst->y);
        break;
    case MN_DEX:
        inst->x = (uint8_t)(((uint32_t)inst->x - 1u) & 0xFFu);
        set_nz(inst, inst->x);
        break;
    case MN_DEY:
        inst->y = (uint8_t)(((uint32_t)inst->y - 1u) & 0xFFu);
        set_nz(inst, inst->y);
        break;
    case MN_SBC:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            sbc_op(inst, v);
        }
        break;
    case MN_ADC: /* binary or decimal by D, memory-to-memory by T */
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            adc_op(inst, bus, t_active, v);
        }
        break;
    case MN_AND: case MN_ORA:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            logic_op(inst, bus, mn, t_active, v);
        }
        break;
    case MN_CMP:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            compare(inst, inst->a, v);
        }
        break;
    case MN_CPX:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            compare(inst, inst->x, v);
        }
        break;
    case MN_CPY:
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            compare(inst, inst->y, v);
        }
        break;
    case MN_BIT: /* N and V from the operand, Z from A AND the operand; BIT immediate follows the page's flag row too */
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            bit_test(inst, inst->a, v);
        }
        break;
    case MN_TSB: /* M <- A OR M (S8-103) */
        if (has_ea(mode)) {
            ea = eff_addr(inst, bus, mode, op1, abs_addr);
            v = ds_bus_read(inst, bus, ea);
            bit_test(inst, inst->a, v);
            ds_bus_write(inst, bus, ea, (uint8_t)(v | (uint32_t)inst->a));
        }
        break;
    case MN_TRB: /* M <- not A AND M (the formula of S8-102; the prose of that page says the result is not stored: the stored reading is the project's, LED-CPU-0010) */
        if (has_ea(mode)) {
            ea = eff_addr(inst, bus, mode, op1, abs_addr);
            v = ds_bus_read(inst, bus, ea);
            bit_test(inst, inst->a, v);
            ds_bus_write(inst, bus, ea, (uint8_t)(v & ~(uint32_t)inst->a));
        }
        break;
    case MN_TST: /* immediate AND memory: Z from the two, N and V from the memory byte; nothing is stored */
        v = ds_bus_read(inst, bus, tst_addr(inst, mode, op2, bytes >= 4u ? (uint32_t)ds_bus_read(inst, bus, (uint16_t)(pc + 3u)) : 0u)); /* the fourth byte is read only by the two absolute forms: a read of the byte after a 3-byte form would be an access that is not part of the instruction */
        bit_test(inst, op1, v);
        break;
    case MN_ASL: case MN_LSR: case MN_ROL: case MN_ROR: case MN_INC: case MN_DEC: /* the accumulator and the memory forms: one read then one write, N and Z from the result */
        if (mode == (uint32_t)AM_ACC) {
            inst->a = (uint8_t)rmw_value(inst, mn, inst->a);
        } else if (has_ea(mode)) {
            ea = eff_addr(inst, bus, mode, op1, abs_addr);
            ds_bus_write(inst, bus, ea, (uint8_t)rmw_value(inst, mn, ds_bus_read(inst, bus, ea)));
        }
        break;
    case MN_EOR: /* N and Z */
        if (operand_value(inst, bus, mode, op1, abs_addr, &v) != 0) {
            logic_op(inst, bus, mn, t_active, v);
        }
        break;
    case MN_BNE: /* the eight conditional branches (S8-22 to S8-28, S8-32, S8-33): taken when the flag is in the named state; no flag changes */
        taken = ((uint32_t)inst->p & P_Z) == 0u;
        break;
    case MN_BEQ:
        taken = ((uint32_t)inst->p & P_Z) != 0u;
        break;
    case MN_BVS:
        taken = ((uint32_t)inst->p & P_V) != 0u;
        break;
    case MN_BVC:
        taken = ((uint32_t)inst->p & P_V) == 0u;
        break;
    case MN_BPL:
        taken = ((uint32_t)inst->p & P_N) == 0u;
        break;
    case MN_BMI:
        taken = ((uint32_t)inst->p & P_N) != 0u;
        break;
    case MN_BCC:
        taken = ((uint32_t)inst->p & P_C) == 0u;
        break;
    case MN_BCS:
        taken = ((uint32_t)inst->p & P_C) != 0u;
        break;
    case MN_BRA: /* the table's 4 cycles already include the branch: no adder (S8-29) */
        taken = 1;
        break;
    case MN_TAM: /* every selected MPR gets A (D-105: one-hot is the documented use) */
        for (i = 0u; i < (uint32_t)DS_MPR_COUNT; i++) {
            if ((op1 & (1u << i)) != 0u) {
                ds_mpr_write(inst, i, inst->a);
            }
        }
        break;
    case MN_JSR: /* pushes the address of the last byte of the instruction, high byte first */
        push(inst, bus, ((pc + 2u) >> 8u) & 0xFFu);
        push(inst, bus, (pc + 2u) & 0xFFu);
        next = abs_addr;
        break;
    case MN_RTS: /* pulls the low byte then the high byte and adds one */
        lo = pull(inst, bus);
        hi = pull(inst, bus);
        next = ((hi << 8u) | lo) + 1u;
        next &= 0xFFFFu;
        break;
    case MN_PHA:
        push(inst, bus, inst->a);
        break;
    case MN_PLA: /* N and Z */
        inst->a = (uint8_t)pull(inst, bus);
        set_nz(inst, inst->a);
        break;
    case MN_PHP: /* B reads 1 and T reads 0 on the stack (H8-5) */
        push(inst, bus, ((uint32_t)inst->p & ~P_T) | 0x10u);
        break;
    case MN_PLP: /* every flag is loaded except B, which has no stored bit (LED-CPU-0009: PLP loads T) */
        inst->p = (uint8_t)(pull(inst, bus) & ~0x10u);
        break;
    case MN_JMP: /* S8-51: abs sets PC; (abs) reads PCL at hhll and PCH at hhll+1 (no 6502 page wrap: the page prints hhll+1); (abs,X) adds X to the pointer address first; 16-bit wrap by masks */
        if (mode == (uint32_t)AM_ABSIND) {
            lo = ds_bus_read(inst, bus, (uint16_t)abs_addr);
            hi = ds_bus_read(inst, bus, (uint16_t)(abs_addr + 1u));
            next = (hi << 8u) | lo;
        } else if (mode == (uint32_t)AM_ABSINDX) {
            lo = ds_bus_read(inst, bus, (uint16_t)(abs_addr + (uint32_t)inst->x));
            hi = ds_bus_read(inst, bus, (uint16_t)(abs_addr + (uint32_t)inst->x + 1u));
            next = (hi << 8u) | lo;
        } else {
            next = abs_addr;
        }
        break;
    case MN_BBR0: case MN_BBR1: case MN_BBR2: case MN_BBR3: case MN_BBR4: case MN_BBR5: case MN_BBR6: case MN_BBR7:
    case MN_BBS0: case MN_BBS1: case MN_BBS2: case MN_BBS3: case MN_BBS4: case MN_BBS5: case MN_BBS6: case MN_BBS7:
        taken = bit_branch(inst, bus, mn, op1); /* three bytes: the target is PC+3+rr, with rr the third byte (S8-20, S8-21) */
        break;
    case MN_BSR: /* pushes the address of the last byte of the BSR (pc+1), PCH first, then branches to PC+2+rr (S8-31); the count is the table's 8, with no taken adder */
        push(inst, bus, ((pc + 1u) >> 8u) & 0xFFu);
        push(inst, bus, (pc + 1u) & 0xFFu);
        taken = 1;
        break;
    case MN_BRK: /* S8-30, H8-17: pushes BRK+2 (PCH, PCL) and P with B = 1 and T = 0, sets I, clears D, then loads PC from $FFF6/$FFF7 through the bus (the IRQ2 vector); T already reads 0 here (fetch-time clear, H8-5); a pushed T of 0 is the project's reading (LED-CPU-0009) */
        push(inst, bus, ((pc + 2u) >> 8u) & 0xFFu);
        push(inst, bus, (pc + 2u) & 0xFFu);
        push(inst, bus, ((uint32_t)inst->p & ~P_T) | 0x10u);
        inst->p = (uint8_t)(((uint32_t)inst->p | P_I) & ~P_D);
        lo = ds_bus_read(inst, bus, 0xFFF6u);
        hi = ds_bus_read(inst, bus, 0xFFF7u);
        next = (hi << 8u) | lo;
        break;
    case MN_RTI: /* S8-70: P is pulled first, then PCL, then PCH; every flag including T is loaded from the pulled byte (D-99), and B has no stored bit */
        inst->p = (uint8_t)(pull(inst, bus) & ~0x10u);
        lo = pull(inst, bus);
        hi = pull(inst, bus);
        next = (hi << 8u) | lo;
        break;
    case MN_CLI: /* I <- 0 (S8-37) */
        inst->p = (uint8_t)((uint32_t)inst->p & ~P_I);
        break;
    case MN_NOP: /* PC <- PC+1, no register or flag (S8-57) */
        break;
    /* ---- the functionality gap: the undefined opcodes and the block transfers advance PC by the table length and charge the table cycles, with no register, flag or memory effect, until the block-transfer plan lands ---- */
    case MN_UNDEF: /* D-96: one byte, two cycles, T cleared */
        break;
    case MN_TAI: case MN_TDD: case MN_TIA: case MN_TII: case MN_TIN:
        break; /* the block-transfer plan (D-100); one step charges the table's 17 until then */
    }
    if (taken) {
        next = branch_target(next, mode == (uint32_t)AM_ZPREL ? op2 : op1);
        cycles += taken_adder(mn);
    }
    cycles += flag_cycles(mn, t_active, inst->p);
    cycles += bus->penalty; /* the VDC and VCE accesses this instruction made, charged by the bus (D-107) */
    inst->pc = (uint16_t)next;
    return cycles * mclk_per_cycle;
}

uint32_t ds_cpu_step(struct ds_instance *inst)
{
    return ds_cpu_step_obs(inst, NULL);
}
