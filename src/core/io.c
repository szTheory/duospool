/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * io.c - the I/O port of the HuC6280 and the two-button pad behind it, and the input schedule that drives the pad (plan 03-27; D-115, D-116, D-117, R-5, R-7; H8-18, H8-19, CDBIOS-p28).
 * The hardware manual states the port: eight input lines K0 to K7 read by a read cycle at physical $1FF000 to $1FF3FF and eight output lines O0 to O7 driven from a latch by a write cycle there, the output high after reset (H8-19, H8-18).
 * Everything above that is a modelling assumption of this project and is labelled so (ASSUMED LED-IO-0002 to LED-IO-0005; evidence class E2, the pad core and the pad fixture are written by one party, ER-5): bit 0 of the latch is SEL and bit 1 is CLR,
 * a read returns $30 | d6 << 6 | d7 << 7 | nibble, the nibble is 0 while CLR is 1 and otherwise the inverted direction bits (SEL = 1: D0 UP, D1 RIGHT, D2 DOWN, D3 LEFT) or the inverted button bits (SEL = 0: D0 I, D1 II, D2 SELECT, D3 RUN), a pressed button reading 0.
 * One report orders the direction nibble differently (RIGHT and DOWN swapped against this table, pcetech.txt, ER-8); the table here follows the other three sources of the decision text and the disagreement is carried into the ledger, not decided by the code.
 * d6 and d7 come from the run request (the profile fields io_1000_d6_value and io_1000_d7_value, ASSUMED LED-IO-0001, open). There is no settling-time model: the level a read sees is the level at its own access stamp, and a fixture that reads without the
 * nine-cycle wait after its write is not labelled hardware-valid (D-115).
 * The logical button mask of an event is in the CD BIOS layout, the one fact of the pad that a manual page states (CDBIOS-p28, the EX_JOYSNS output byte: I 0, II 1, SELECT 2, RUN 3, UP 4, RIGHT 5, DOWN 6, LEFT 7, 1 = pressed).
 * The schedule is an immutable copy of a declared list inside the instance (D-117): the level of a port at a stamp is the mask of the last event of that port at or before the stamp (ties inclusive, a port with no event yet holds nothing pressed), a pure function of the
 * instance members, so no chunking of a run, no observer and no restore can make two reads at one stamp differ. The core never reads a file, a clock or the environment; the host supplies the list.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define PAD_BITS_MASK 0x000000FFu     /* the eight bits of the two-button pad; every bit above is reserved (III to VI, D-116) */
#define PAD_DIR_SHIFT 4u              /* the direction bits UP RIGHT DOWN LEFT are bits 4 to 7 of the mask */
#define PAD_UP_DOWN 0x50u             /* UP (bit 4) and DOWN (bit 6): pressed together they are opposite directions */
#define PAD_LEFT_RIGHT 0xA0u          /* RIGHT (bit 5) and LEFT (bit 7) */
#define LATCH_SEL 0x01u               /* bit 0 of the latch: 1 selects the directions, 0 the buttons (LED-IO-0002, assumed) */
#define LATCH_CLR 0x02u               /* bit 1 of the latch: 1 reads the nibble as 0 (LED-IO-0003, assumed) */
#define READ_BASE 0x30u               /* bits 4 and 5 of a read (LED-IO-0003, assumed) */

/* the refusal names of ds_input_push, generated from input_refusals.def */
#define DS_INPUTREF(name, number) [number] = "input." #name,
static const char *const ds_input_refusal_names[DS_INPUTREF_SLOTS] = {
    [0] = "ok",
#include "input_refusals.def"
};
#undef DS_INPUTREF

/* the slot ceiling (F4): the rows are numbered 1..N without a gap or a reuse, so N is the row count and must leave a slot below DS_INPUTREF_SLOTS */
#define DS_INPUTREF(name, number) +1
enum { DS_INPUTREF_ROWS = 0
#include "input_refusals.def"
};
#undef DS_INPUTREF
_Static_assert(DS_INPUTREF_ROWS < (int)DS_INPUTREF_SLOTS, "input_refusals.def has more rows than DS_INPUTREF_SLOTS slots");

/* the late check reads "no step was taken" as machine time 0, which holds only while a boot charges no time (constants.def, ASSUMED LED-CPU-0004) */
_Static_assert((uint64_t)DS_CONST_reset_cost_mclk == UINT64_C(0), "ds_input_push tests machine_time == 0 as the state before the first step: a nonzero reset cost needs a separate step counter");

const char *ds_input_refusal_name(uint32_t number)
{
    return number < (uint32_t)DS_INPUTREF_SLOTS ? ds_input_refusal_names[number] : NULL;
}

/* power-on state of the port (D-114, R-5): the latch of port O drives a high level after reset (H8-18, H8-19), so it is $FF under every policy, and port 1 holds the pad that the profile names (r1: pad2); ports 2 to 5 stay empty */
void ds_io_poweron(struct ds_instance *inst)
{
    inst->o_latch = 0xFFu;
    inst->slot_type[0] = ds_streq(inst->profile.field[DS_FIELD_input_port1].value, "pad2") ? (uint8_t)DS_SLOT_PAD2 : (uint8_t)DS_SLOT_NONE;
}

void ds_io_write(struct ds_instance *inst, uint8_t value)
{
    inst->o_latch = value;
}

/* the buttons held on port (1-based) at the stamp: the mask of the last event of that port whose time is at or before the stamp, 0 before the first; the events are in non-decreasing time order, so the scan ends at the first event after the stamp */
static uint32_t level_at(const struct ds_instance *inst, uint32_t port, uint64_t stamp)
{
    uint32_t held = 0u;
    uint32_t i;

    for (i = 0u; i < inst->input_count && i < DS_INPUT_MAX && inst->input_ev[i].at <= stamp; i++) {
        if (inst->input_ev[i].port == port) {
            held = inst->input_ev[i].mask;
        }
    }
    return held;
}

uint8_t ds_io_read(const struct ds_instance *inst, uint64_t stamp)
{
    uint32_t d6 = inst->profile.field[DS_FIELD_io_1000_d6_value].value[0] == '1' ? 1u : 0u;
    uint32_t d7 = inst->profile.field[DS_FIELD_io_1000_d7_value].value[0] == '1' ? 1u : 0u;
    uint32_t nibble = 0u;
    uint32_t held;

    if (((uint32_t)inst->o_latch & LATCH_CLR) == 0u) {
        held = inst->slot_type[0] == (uint8_t)DS_SLOT_PAD2 ? level_at(inst, 1u, stamp) : 0u; /* no pad on port 1: the lines idle high, nothing pressed */
        held = ((uint32_t)inst->o_latch & LATCH_SEL) != 0u ? (held >> PAD_DIR_SHIFT) & 0x0Fu : held & 0x0Fu;
        nibble = ~held & 0x0Fu; /* a pressed button reads 0 */
    }
    return (uint8_t)(READ_BASE | (d6 << 6u) | (d7 << 7u) | nibble);
}

uint32_t ds_input_push(struct ds_instance *inst, const struct ds_input_event *events, uint32_t count)
{
    uint32_t i;

    if (count > DS_INPUT_MAX || (events == NULL && count != 0u)) {
        return (uint32_t)DS_INPUTREF_queue_full;
    }
    for (i = 0u; i < count; i++) {
        /* the port must be one of the five and, on a real instance, have a pad behind it (a NULL instance is the state refusal below and has no slots to read) */
        if (events[i].port < 1u || events[i].port > DS_PORT_COUNT) {
            return (uint32_t)DS_INPUTREF_port;
        }
        if (inst != NULL && inst->slot_type[events[i].port - 1u] != (uint8_t)DS_SLOT_PAD2) {
            return (uint32_t)DS_INPUTREF_port;
        }
        if ((events[i].mask & ~PAD_BITS_MASK) != 0u || (events[i].mask & PAD_UP_DOWN) == PAD_UP_DOWN || (events[i].mask & PAD_LEFT_RIGHT) == PAD_LEFT_RIGHT) {
            return (uint32_t)DS_INPUTREF_bits;
        }
    }
    for (i = 1u; i < count; i++) {
        if (events[i].at < events[i - 1u].at || (events[i].at == events[i - 1u].at && events[i].port <= events[i - 1u].port)) {
            return (uint32_t)DS_INPUTREF_order;
        }
    }
    if (inst == NULL) {
        return (uint32_t)DS_INPUTREF_state;
    }
    if (inst->machine_time != UINT64_C(0)) {
        return (uint32_t)DS_INPUTREF_late;
    }
    if (inst->input_count != 0u) {
        return (uint32_t)DS_INPUTREF_state;
    }
    for (i = 0u; i < DS_INPUT_MAX; i++) { /* every slot is written: the unused ones are zero, so the schedule is a function of the declared list alone */
        inst->input_ev[i].at = i < count ? events[i].at : UINT64_C(0);
        inst->input_ev[i].port = i < count ? events[i].port : 0u;
        inst->input_ev[i].mask = i < count ? events[i].mask : 0u;
    }
    inst->input_count = count;
    return (uint32_t)DS_INPUTREF_NONE;
}
