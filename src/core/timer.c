/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * timer.c - the HuC6280 timer as a closed form on machine time (plan 03-21; D-109, H8-20, H8-21, H8-22, ER-3).
 * State, all in the instance: origin (the stamp of the last step boundary the state was folded to, or of the start write), counter (the 7-bit value at the origin), reload, enabled and the TIQ latch (the last two
 * of them are written here and in irq.c). A prescaler step arrives every DS_TIMER_STEP_MCLK master clocks from the origin; each step counts the counter down, and a step that arrives with the counter at 0 is a borrow: the
 * counter then holds the reload value. After n steps from a start with counter c and reload r: n <= c gives counter c - n and no borrow; otherwise m = n - (c + 1) steps have passed since the first borrow, there have been 1 + m / (r + 1)
 * borrows and the counter is r - m mod (r + 1). The period between borrows is therefore (r + 1) steps (a derivation from the 143 usec to 18.3 msec interval of H8-21, assumption A1; the one claim of this file that a C-level test judges
 * against a naive per-clock model, timer_pure_function).
 * The mutation rule of this file: the state changes ONLY inside ds_timer_write and ds_timer_settle, which a guest access calls with its own stamp. Every other use (a counter read, the interrupt sample, the request register) is the
 * pure query below, so the stored fields are identical for every chunking of a run (D-109). No tick, no scheduler, no float, no clock: the arithmetic is unsigned 64-bit division and modulo, and the period is never zero (T-03-57).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_timer.h"

/* the steps elapsed since the origin at the stamp, the counter value after them and the borrows among them; a stopped timer, or a stamp that is not after the origin, has no steps */
static void fold(const struct ds_instance *inst, uint64_t stamp, uint64_t *steps, uint8_t *value, uint64_t *borrows)
{
    uint64_t n = UINT64_C(0);
    uint64_t b = UINT64_C(0);
    uint64_t v = (uint64_t)inst->timer_counter;

    if (inst->timer_enabled != 0u && stamp > inst->timer_origin) {
        n = (stamp - inst->timer_origin) / (uint64_t)DS_TIMER_STEP_MCLK;
        if (n <= v) {
            v = v - n;
        } else {
            uint64_t m = n - (v + UINT64_C(1));
            uint64_t period = (uint64_t)inst->timer_reload + UINT64_C(1);

            b = UINT64_C(1) + m / period;
            v = (uint64_t)inst->timer_reload - m % period;
        }
    }
    *steps = n;
    *value = (uint8_t)(v & UINT64_C(0x7F));
    *borrows = b;
}

void ds_timer_query(const struct ds_instance *inst, uint64_t stamp, uint8_t *counter, uint64_t *borrows)
{
    uint64_t n;
    uint8_t v;
    uint64_t b;

    fold(inst, stamp, &n, &v, &b);
    if (counter != NULL) {
        *counter = v;
    }
    if (borrows != NULL) {
        *borrows = b;
    }
}

uint8_t ds_timer_read(const struct ds_instance *inst, uint32_t reg, uint64_t stamp)
{
    uint8_t v;

    (void)reg; /* both addresses of the window read the counter (D-109) */
    ds_timer_query(inst, stamp, &v, NULL);
    return v;
}

uint32_t ds_timer_tiq(const struct ds_instance *inst, uint64_t stamp)
{
    uint64_t b;

    ds_timer_query(inst, stamp, NULL, &b);
    return (inst->tiq_latch != 0u || b != UINT64_C(0)) ? 1u : 0u;
}

void ds_timer_settle(struct ds_instance *inst, uint64_t stamp)
{
    uint64_t n;
    uint8_t v;
    uint64_t b;

    fold(inst, stamp, &n, &v, &b);
    if (b != UINT64_C(0)) {
        inst->tiq_latch = 1u; /* the latch is set at every borrow, masked or not, and many borrows are one bit (D-109) */
    }
    inst->timer_counter = v;
    inst->timer_origin = inst->timer_origin + n * (uint64_t)DS_TIMER_STEP_MCLK;
}

void ds_timer_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp)
{
    ds_timer_settle(inst, stamp);
    if ((reg & 1u) == 0u) {
        inst->timer_reload = (uint8_t)((uint32_t)value & 0x7Fu); /* $0C00: the reload register, seven bits; the counter and the running period are untouched (H8-21) */
    } else if (((uint32_t)value & 1u) != 0u) {
        if (inst->timer_enabled == 0u) { /* bit 0 goes 0 to 1: the counter is loaded and the prescaler phase restarts (H8-22) */
            inst->timer_enabled = 1u;
            inst->timer_counter = inst->timer_reload;
            inst->timer_origin = stamp;
        }
    } else if (inst->timer_enabled != 0u) { /* bit 0 goes 1 to 0: the timer stops with the counter held (D-109 constant: hold, ASSUMED LED-IRQ-0003) */
        inst->timer_enabled = 0u;
    }
}
