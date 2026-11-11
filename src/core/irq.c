/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * irq.c - the interrupt controller of the HuC6280: the three sources, the two registers, the priority and the declared test stimulus (plans 03-21 and 03-22; D-110, D-111, D-112; H8-12 to H8-16, H8-21, H8-24).
 * Two registers live in window 5 of the hardware page (bank $FF): the disable register at $1402 (three bits, 1 = disabled, bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD, reset 0, H8-16) and the request register at $1403 (bit 0 the IRQ2 level, bit 1 the
 * IRQ1 level, bit 2 TIQ, read-only, TIQ set by a timer borrow and reset by a write cycle, H8-15). The TIQ request bit is a latch that every borrow sets, masked or not, and that many borrows set once (D-109); a write to $1403 clears it and nothing else.
 * The latch is the stored bit or a borrow that is due at the stamp: both are folded into one pure query of the timer, so a read of the request register has no side effect (D-110).
 * IRQ1 and IRQ2 are levels (H8-13: driven by level sense). Nothing drives them yet (the VDC arrives in Phase 4): a case may declare a stimulus (ds_irq_stim_set, the arrays irq_stim_* of the instance), a list of events (at, line, level) whose levels are a pure
 * function of the stamp. It is a test hook and says nothing about the VDC or any other hardware source (ER-5, D-112); with no stimulus both levels are 0 at every stamp.
 * Delivery: a source that is pending and not disabled is taken at the sample point when I is clear (the caller applies I, cpu.c); the vector is the logical address of its low byte, read through MPR7 like any access (H8-13: the timer vector is $FFFA/$FFFB, IRQ1 $FFF8/$FFF9,
 * IRQ2 and BRK $FFF6/$FFF7; the priority is timer, IRQ1, IRQ2). A disable bit does not stop the latch or the level, only the delivery. NMI has no driver (D-110).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_timer.h"

#define IRQ_BIT_IRQ2 0x01u   /* bit 0 of both registers (H8-15, H8-16) */
#define IRQ_BIT_IRQ1 0x02u   /* bit 1 */
#define IRQ_BIT_TIQ 0x04u    /* bit 2 */
#define IRQ_REG_MASK 0x07u   /* the three register bits (H8-24: the interrupt registers are three bits wide) */
#define VEC_TIMER_LOW 0xFFFAu /* H8-13, H8-14 */
#define VEC_IRQ1_LOW 0xFFF8u
#define VEC_IRQ2_LOW 0xFFF6u

/* the refusal names of ds_irq_stim_set, generated from stim_refusals.def */
#define DS_STIMREF(name, number) [number] = "stim." #name,
static const char *const ds_stim_refusal_names[DS_STIMREF_SLOTS] = {
    [0] = "ok",
#include "stim_refusals.def"
};
#undef DS_STIMREF

/* the slot ceiling (F4): the rows are numbered 1..N without a gap or a reuse, so N is the row count and must leave a slot below DS_STIMREF_SLOTS */
#define DS_STIMREF(name, number) +1
enum { DS_STIMREF_ROWS = 0
#include "stim_refusals.def"
};
#undef DS_STIMREF
_Static_assert(DS_STIMREF_ROWS < (int)DS_STIMREF_SLOTS, "stim_refusals.def has more rows than DS_STIMREF_SLOTS slots");

/* the state check below reads "no step was taken" as machine time 0, which holds only while a boot charges no time (constants.def, ASSUMED LED-CPU-0004) */
_Static_assert((uint64_t)DS_CONST_reset_cost_mclk == UINT64_C(0), "ds_irq_stim_set tests machine_time == 0 as the state before the first step: a nonzero reset cost needs a separate step counter");

const char *ds_stim_refusal_name(uint32_t number)
{
    return number < (uint32_t)DS_STIMREF_SLOTS ? ds_stim_refusal_names[number] : NULL;
}

uint32_t ds_irq_stim_set(struct ds_instance *inst, const struct ds_irq_event *events, uint32_t count)
{
    uint32_t i;

    if (count > DS_IRQ_STIM_MAX || (events == NULL && count != 0u)) {
        return (uint32_t)DS_STIMREF_count;
    }
    for (i = 0u; i < count; i++) {
        if (events[i].line != 1u && events[i].line != 2u) {
            return (uint32_t)DS_STIMREF_line;
        }
        if (events[i].level > 1u) {
            return (uint32_t)DS_STIMREF_level;
        }
    }
    for (i = 1u; i < count; i++) {
        if (events[i].at < events[i - 1u].at) {
            return (uint32_t)DS_STIMREF_order;
        }
    }
    if (inst == NULL || inst->machine_time != UINT64_C(0)) {
        return (uint32_t)DS_STIMREF_state;
    }
    for (i = 0u; i < DS_IRQ_STIM_MAX; i++) { /* every slot is written: the unused ones are zero, so the region IRQS is a function of the declared list alone */
        inst->irq_stim_at[i] = i < count ? events[i].at : UINT64_C(0);
        inst->irq_stim_line[i] = i < count ? (uint8_t)events[i].line : 0u;
        inst->irq_stim_level[i] = i < count ? (uint8_t)events[i].level : 0u;
    }
    return (uint32_t)DS_STIMREF_NONE;
}

/* the level of line (1 or 2) at the stamp: the level of the last event of that line at or before the stamp (ties inclusive), 0 before the first; the slots are in non-decreasing time order, so a later slot of the line wins, and an unused slot (line 0) ends the list */
static uint32_t stim_level(const struct ds_instance *inst, uint32_t line, uint64_t stamp)
{
    uint32_t level = 0u;
    uint32_t i;

    for (i = 0u; i < DS_IRQ_STIM_MAX && inst->irq_stim_line[i] != 0u; i++) {
        if ((uint32_t)inst->irq_stim_line[i] == line && inst->irq_stim_at[i] <= stamp) {
            level = (uint32_t)inst->irq_stim_level[i] & 1u;
        }
    }
    return level;
}

/* the three request bits at the stamp, before any mask: bit 0 the IRQ2 level, bit 1 the IRQ1 level, bit 2 the TIQ request (the register layout of H8-15) */
static uint32_t request_bits(const struct ds_instance *inst, uint64_t stamp)
{
    return (stim_level(inst, 2u, stamp) != 0u ? IRQ_BIT_IRQ2 : 0u) | (stim_level(inst, 1u, stamp) != 0u ? IRQ_BIT_IRQ1 : 0u) | (ds_timer_tiq(inst, stamp) != 0u ? IRQ_BIT_TIQ : 0u);
}

uint32_t ds_irq_read(const struct ds_instance *inst, uint32_t reg, uint64_t stamp)
{
    if ((reg & 3u) == 2u) {
        return (uint32_t)inst->irq_disable & IRQ_REG_MASK; /* $1402 */
    }
    return request_bits(inst, stamp); /* $1403: IRQ2, IRQ1 and TIQ; a read changes nothing (D-110) */
}

void ds_irq_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp)
{
    if ((reg & 3u) == 2u) {
        inst->irq_disable = (uint8_t)((uint32_t)value & IRQ_REG_MASK);
    } else {
        ds_timer_settle(inst, stamp); /* a borrow due up to this stamp is requested first, then the write clears the request: a write in the sample cycle takes effect before the sample (D-111) */
        inst->tiq_latch = 0u;
    }
}

uint32_t ds_irq_pending_query(const struct ds_instance *inst, uint64_t stamp)
{
    return request_bits(inst, stamp) & ~(uint32_t)inst->irq_disable & IRQ_REG_MASK;
}

uint32_t ds_irq_pending(const struct ds_instance *inst, uint64_t stamp)
{
    uint32_t bits = ds_irq_pending_query(inst, stamp);

    return (bits & IRQ_BIT_TIQ) != 0u ? VEC_TIMER_LOW : (bits & IRQ_BIT_IRQ1) != 0u ? VEC_IRQ1_LOW : (bits & IRQ_BIT_IRQ2) != 0u ? VEC_IRQ2_LOW : 0u;
}
