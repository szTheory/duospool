/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_timer.h - the private entry points of the timer (plan 03-21; D-109, D-108, I16). Private to the core and its tests like ds_core.h; no public header exists (D-05).
 * The timer is a closed form on machine time (timer.c): a prescaler step every DS_TIMER_STEP_MCLK master clocks from the origin stamp (OSC1 / 3 / 1024, H8-20 and H8-21), a 7-bit down counter and a 7-bit reload register.
 * A borrow happens when the counter is 0 and a step arrives; the counter then holds the reload value, so the period between borrows is (reload + 1) steps (a derivation from the 143 usec to 18.3 msec range of H8-21, assumption A1 of the research).
 * Timer state changes ONLY inside a guest access at its stamp (ds_timer_write, and the acknowledge of ds_irq_write, which fold the elapsed time first); every other use is a pure query of the stored fields and a stamp, so the raw fields
 * are the same for every chunking of a run (D-109). Nothing here peeks the bus, reads a clock or keeps a cache.
 * The stamps are master clocks of the machine timeline; the bus computes the stamp of each access (struct ds_bus in ds_core.h).
 */
#ifndef DS_TIMER_H
#define DS_TIMER_H

#include <stdint.h>

#include "ds_core.h"

/* master clocks per prescaler step: OSC1 / 3 / 1024 (H8-20, H8-21; LED-IRQ-0002). The step is NOT derived from the CPU cycle length, which is what keeps it independent of the speed bit (D-90) */
#define DS_TIMER_STEP_MCLK 3072u

/*
 * A register write of the timer window at the access stamp: reg 0 ($0C00) sets the 7-bit reload register and leaves the counter alone; reg 1 ($0C01) starts the timer on bit 0 going 0 to 1 (counter = reload, origin = stamp) and stops
 * it on bit 0 going 1 to 0 (the counter is held); a write that repeats the state changes nothing. The register is selected by bit 0 of reg only. The elapsed time is folded into the fields first.
 */
void ds_timer_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp);

/* the 7-bit counter value a read of either timer address returns at the stamp (the eighth bit comes from the bus latch in the bus, D-104); pure: nothing is changed, whatever the stamp */
uint8_t ds_timer_read(const struct ds_instance *inst, uint32_t reg, uint64_t stamp);

/* the pure query: the counter value and the number of borrows since the stored origin at the stamp (a stamp before the origin counts as the origin; a stopped timer has no borrows) */
void ds_timer_query(const struct ds_instance *inst, uint64_t stamp, uint8_t *counter, uint64_t *borrows);

/* 1 when TIQ is requested at the stamp: the latch is set or a borrow is due since the origin (pure) */
uint32_t ds_timer_tiq(const struct ds_instance *inst, uint64_t stamp);

/* fold the elapsed time into the fields at the stamp (origin to the last step boundary, counter to its exact value, the latch set by any borrow); the guest accesses of the timer and of the request register call it */
void ds_timer_settle(struct ds_instance *inst, uint64_t stamp);

#endif
