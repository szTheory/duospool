/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * irq.c - the interrupt controller of the HuC6280, the TIQ source only (plan 03-21; D-110, D-111; H8-12 to H8-16, H8-21, H8-24). Plan 03-22 adds IRQ1 and IRQ2 (the stimulus levels), the sample parameter and the serialised regions.
 * Two registers live in window 5 of the hardware page (bank $FF): the disable register at $1402 (three bits, 1 = disabled, bit 2 TIQD, reset 0, H8-16) and the request register at $1403 (bit 2 TIQ, read-only, set by a timer
 * borrow, reset by a write cycle, H8-15). The TIQ request bit is a latch that every borrow sets, masked or not, and that many borrows set once (D-109); a write to $1403 clears it and nothing else. The latch is the stored bit or a borrow that is
 * due at the stamp: both are folded into one pure query of the timer, so a read of the request register has no side effect (D-110).
 * Delivery: a source that is pending and not disabled is taken at the sample point when I is clear (the caller applies I, cpu.c); the vector is the logical address of its low byte, read through MPR7 like any access (H8-13: the timer
 * vector is $FFFA/$FFFB, IRQ1 $FFF8/$FFF9, IRQ2 and BRK $FFF6/$FFF7; the priority is timer, IRQ1, IRQ2). The disable bit does not stop the latch, only the delivery.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_timer.h"

#define IRQ_BIT_TIQ 0x04u    /* bit 2 of both registers (H8-15, H8-16) */
#define IRQ_REG_MASK 0x07u   /* the three register bits (H8-24: the interrupt registers are three bits wide) */
#define VEC_TIMER_LOW 0xFFFAu /* H8-13, H8-14 */

uint32_t ds_irq_read(const struct ds_instance *inst, uint32_t reg, uint64_t stamp)
{
    if ((reg & 3u) == 2u) {
        return (uint32_t)inst->irq_disable & IRQ_REG_MASK; /* $1402 */
    }
    return ds_timer_tiq(inst, stamp) != 0u ? IRQ_BIT_TIQ : 0u; /* $1403: TIQ in bit 2; IRQ1 and IRQ2 have no driver yet (plan 03-22) */
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

uint32_t ds_irq_pending(const struct ds_instance *inst, uint64_t stamp)
{
    if (((uint32_t)inst->irq_disable & IRQ_BIT_TIQ) == 0u && ds_timer_tiq(inst, stamp) != 0u) {
        return VEC_TIMER_LOW;
    }
    return 0u;
}
