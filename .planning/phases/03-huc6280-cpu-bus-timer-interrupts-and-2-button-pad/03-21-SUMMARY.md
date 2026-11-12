---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 21
subsystem: timer-tiq-interrupt-entry-access-stamps
tags: [timer, closed-form, tiq, interrupt-entry, access-stamp, dev-stamp, timer-pure, tim-timer, w4-tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the bus with the hardware-page windows and struct ds_bus (03-16), the dev sink and its stamp (03-19), the DSV1 fixture library, the mutation sweep and the guest kind with its power-on policy list (03-05, 03-10, 03-11), the bank-class oracle and the three mmu_mpr images (03-20)
provides:
  - src/core/timer.c and ds_timer.h: the timer as a closed form on master clocks (ds_timer_write, ds_timer_read, ds_timer_query, ds_timer_tiq, ds_timer_settle); step 3072 master clocks, period reload + 1 steps, state changed only inside guest accesses at their stamps
  - src/core/irq.c: the disable register $1402 (bit 2 TIQD), the request register $1403 (bit 2 TIQ, a write clears TIQ only), ds_irq_read, ds_irq_write, ds_irq_pending (TIQ only, returns the vector low address)
  - cpu.c: the interrupt sample in the last cycle of the instruction with the I flag from before it (irq_i_sample pre, hard-wired until plan 03-22) and the entry in the same step (PCH, PCL, P with B=0 and the current T, I set, D and T cleared, 8 cycles, vector $FFFA through MPR7)
  - struct ds_bus carries the access stamp rule (start, cycle_mclk, last, rmw); the dev record carries the access stamp (ds_obs_dev takes it)
  - fixtures/src/tim_timer.s (DSV id 10, 33 checks), kind timer_pure, REQUIRED cases tim_timer, timer_pure_function and mutate_sweep_tim_timer
affects: [03-22, 03-23, 03-24, 03-25, 03-26, 03-27, 03-29, 03-32, 03-34]

actuals:
  tokens: 27400
  tasks: 3
  commits: 4
plan_head_before: 10503d18c4bf0011f1bf9874948785c93bedeb15
plan_head_after: b9eecf89aad4a8b42cad3f4a3373eced39c4484b

tech-stack:
  added: []
  patterns:
    - "a device with time keeps raw fields that change only inside a guest access at its own stamp (settle, then apply), and answers every other question with a pure query of (fields, stamp): a stamp is computed by the bus from the step's start, the cycle length at the start speed, the index of the last cycle and the penalty so
      far, and one cycle earlier for the read of a read-modify-write instruction"
    - "the interrupt sample and the entry live in the step of the instruction they follow: nothing persists between steps and the entry has its own struct ds_bus (start = end of the instruction, last = 7) so its accesses are stamped inside its 8 cycles"
    - "a C-level proof of a closed form is a naive per-clock reference model inside the test file only, compared at, one clock before and one clock after every boundary, with the manual's numbers re-computed in integers; the case aggregates mismatches into a few assertions and keeps the first message"
    - "an in-guest counter check computes its expectation with the assembler from the instruction counts (the DELAY macro is 6 * k cycles) and a named ratio constant, so the dependence on the community-reported CPU cycle length is one constant (CYC_STEP) and the check is labelled E2"

key-files:
  created:
    - src/core/timer.c
    - src/core/ds_timer.h
    - src/core/irq.c
    - fixtures/src/tim_timer.s
    - fixtures/src/tim_timer.checks
    - fixtures/manifest/tim_timer.toml
    - tests/conformance/cases/tim_timer.toml
    - tests/conformance/cases/timer_pure_function.toml
    - tests/conformance/cases/mutate_sweep_tim_timer.toml
  modified:
    - src/core/ds_core.h
    - src/core/bus.c
    - src/core/cpu.c
    - src/core/obs.c
    - src/core/ds_obs.h
    - tests/conformance/cpukinds.c
    - tests/conformance/obskinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/dev_log_records.toml
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - fixtures/manifest/mmu_mpr.toml
    - fixtures/manifest/mmu_mpr_384k.toml
    - fixtures/manifest/mmu_mpr_1m.toml

key-decisions:
  - "The timer state is seven instance members: timer_origin (u64, before pc so it sits on the 8-byte boundary after the seed) and timer_counter, timer_reload, timer_enabled, tiq_latch, irq_disable after tma_zero, with a _Static_assert chain. They are NOT in the digest yet: the TIMR and IRQC regions and the serial bump are plan
    03-22's, as this plan says. Until then a chunk or restore comparison by digest cannot see the timer, and a raw field comparison can"
  - "Access stamp rule (ASSUMED, LED-CPU-0004; the manual states nothing about the cycle of an access): stamp = start + (last + penalty so far) * cycle_mclk, where last = the opcode's table cycles plus its flag adders minus 1 and penalty includes the access being charged; the read of a CL_RMW instruction is one cycle earlier.
    Every access of a step uses it (operand fetches too), which is irrelevant until code runs from the timer window. A NULL bus uses the machine time"
  - "Timer settle at every register access ($0C00, $0C01, $1403 write); reads, the request register read and the sample are pure. A $0C01 write with the repeated state settles but changes nothing else, so the observable state is that of a no-op. Hold on stop and no $7F window are the constants of D-109; the borrow at a tie (a
    step boundary exactly at the stamp) counts, so an acknowledge at that stamp absorbs it"
  - "Sample stamp = end of the instruction minus the cycle length at the START speed (a CSL or CSH ends at its old speed); the I flag of the sample is the one before the instruction (pre) at the single statement `if (i_before == 0u)`; plan 03-22 turns it into the model parameter. The entry costs DS_INT_ENTRY_CYCLES (8) plus the
    penalty cycles of its own accesses at the speed after the instruction"
  - "ds_obs_dev takes the stamp; the dev record stamp is the access stamp since this plan (it was the instruction's start stamp). dev_log_records items now end in the cycle index of the access (rec=PC:KIND:PHYS:VALUE:CYCLE, 4 for ST0 and 5 for STA abs: page cycles minus 1 plus the penalty cycle), multiplied by 12 in the kind"
  - "timer_pure runs the one-per-reload pure sequences for every reload and the three start phases, and the edges (reload write while running, hold, no-op rewrites, restart phase, acknowledge, settle invariance) for reloads 0, 1, 127 and every eighth, plus a settled variant of the pure sequence for the same reloads: the whole
    kind takes about 0.3 s per runner in the debug build. The model counts clocks and shares no formula with timer.c"
  - "mmu_mpr reads the timer counter and the interrupt registers at checks 32 to 41, so the fixture now starts the timer at reload 0 and stops it seven cycles later before check 32: the counter and the reload register are not reset values (H8-18), and plan 03-24 draws them under seeded-prng-v2. The literals of checks 32 to 41
    did not change; their comments and class rows were re-judged and the three images re-pinned"

requirements-advanced: [CPU-03, CORE-04]
requirements-completed: []  # CPU-03 also needs IRQ1 and IRQ2, the priority, the irq_i_sample parameter and the irq_ctl fixture (03-22); CORE-04 needs the block transfers and chunk cases (W5)

duration: 36min
completed: 2026-11-11
---

# Phase 3 Plan 21: Closed-form timer, TIQ interrupt with same-step entry and the tim_timer fixture Summary

**The HuC6280 timer is a closed form on master clocks (a step every 3072, a period of reload + 1 steps) whose raw state changes only inside guest accesses at their own stamps; a borrow sets TIQ, which the sample in the last cycle of an instruction takes with I clear, the entry pushing PCH, PCL and a status byte with B=0 and
running in the same step; a naive per-clock model proves the step and the period at C level, and a 33-check fixture takes the interrupt and reads the counter as labelled E2 self-consistency.**

## What was built

- **Task 1 (tracer), commit 6b5f903.**
  - `timer.c`: `fold` computes the steps since the origin, the counter after them and the borrows (n <= c: c - n; otherwise m = n - (c + 1), borrows 1 + m / (r + 1), counter r - m mod (r + 1)); `ds_timer_settle` folds them into the fields (origin to the last boundary, counter exact, latch set by any borrow); `ds_timer_write`
    settles, then sets the 7-bit reload ($0C00), loads counter = reload and origin = stamp on bit 0 going 0 to 1, holds the counter on 1 to 0 and does nothing for a repeated state. The reload statement is one distinct line (`uint64_t period = (uint64_t)inst->timer_reload + UINT64_C(1);`) for the toy of plan 03-22.
  - `irq.c`: `ds_irq_read` ($1402: the three disable bits; $1403: bit 2 from the pure TIQ query), `ds_irq_write` ($1402 stores 3 bits; $1403 settles the timer then clears the latch), `ds_irq_pending` (TIQ unless bit 2 of the disable register is set; returns $FFFA).
  - `bus.c`: `access_stamp` and the real timer and interrupt windows; `hw_read` returns the counter at the stamp over bit 7 of the latch, `$1402` and `$1403` the three bits over the upper five; `hw_write` calls the timer and the controller with the stamp; `log_dev` takes the stamp.
  - `cpu.c`: `struct ds_bus` initialised with the start, the cycle length and `last`/`rmw` set once the opcode is known; after the instruction, the sample (`i_before == 0`) and `interrupt_entry` (8 cycles at the speed after the instruction, own bus). Reset clears the timer enable, TIQ and the disable register.
  - `ds_core.h`/`ds_obs.h`/`obs.c`: the seven instance members with their `_Static_assert` chain, the new `struct ds_bus` fields and the irq prototypes; `ds_obs_dev` takes the stamp.
  - `tim_timer.s` first checks (10, DSV id 10): reload 0, `sed`, `cli`, start, bounded wait; the handler (saves A and X, copies the pushed bytes through TSX, PHP/PLA for I and D, reads $1403, stops the timer, acknowledges, reads again, counts, RTI) seen through ten rows: interrupt taken, pushed P (B=0, D=1, I=0), I set and D
    clear in the handler, TIQ set then cleared by the write, pushed return address in the loop, RTI resumed, no second entry. Case `tim_timer`, manifest and REQUIRED line.
  - `mmu_mpr.s` timer init block before check 32, comments and `.checks` text re-judged for checks 32 to 41, three manifests re-pinned; `dev_log_records` re-judged for the access stamp (see Deviations).
- **Task 2, commit 96346d1.** Kind `timer_pure` (cpukinds.c, one runner.h line, one `kinds[]` row) and REQUIRED case `timer_pure_function`: the literals 3 * 1024 = 3072, the 6.992 kHz, 143 usec and 18.3 msec of H8-21 re-computed in integers; for every reload 0 to 127 and the start phases 0, 1 and 3071 the counter (by both read
  addresses), the TIQ bit and the borrow count are compared with the model before, at and after every step boundary and mid-step over three periods; the first and second borrow must fall (reload + 1) and 2 * (reload + 1) steps after the start; edges and a settled variant for reloads 0, 1, 127 and every eighth (340099 samples in all).
- **Task 3, commits 92276e6 and b9eecf8.** `tim_timer` grew to 33 checks: reset state (TIQ clear, disable register 0, no borrow in nearly twelve steps with the timer stopped), counter reads after counted delays (the expectation `reload - cycles / CYC_STEP` written by the assembler from the DELAY counts, GUEST:timer-e2, checks
  14 to 17), the period and the borrow reload (18, 19), start loads the reload (20), stop holds, long hold, repeated 0 (21 to 23), reload write while running (24), repeated 1 (25), bit 7 from io_buffer (26, 27), request latched while masked by TIQD (28), TIQD masks delivery (29), six borrows give one entry (30), I masks
  delivery (31), request latched while I is set (32) and the T bit pushed after `cli; set` (33). `mutate_sweep_tim_timer` flips each of the 33 literals. The second commit makes check 18 read seven steps (see Verification).

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `core-boundary`, `fixtures`, `conformance`; Task 2 `--stage conformance`, `sanitizers`; Task 3 `--stage fixture-lint`, `fixtures`, `conformance`. The tracer gate (auto mode) re-ran the end-to-end verify before the expansion: it passed (`Tracer verified end-to-end - expanding`).
- Plan end: `sh scripts/check` PASS (about 105 s wall, run before each of the four commits and once more after the last), `sh scripts/check --selftest` PASS (63 PASS lines, 15 s), `dash scripts/check` PASS, `sh scripts/audit-history --range 10503d1..HEAD` run before the docs commit (result in the final report). The seven toy
  controls stayed RED_OK, the shared-state toy included (the standing note was not triggered: run.c is untouched).
- Bite shown with scratch edits (each restored byte for byte and `cmp`-checked, nothing committed): the acknowledge not clearing the latch turns tim_timer red at check 1 (the tracer can fail); a period of reload + 2 turns timer_pure_function guest_failure; a stop that reloads the counter turns check 21 red; T not pushed by the
  entry and sampling with I after the instruction each turn check 33 red; a counter rounded instead of floored turns check 14 red; a reload write that reloads the counter turns check 24 red; a period of reload + 2 and reload + 0 each turn check 18 red after the strengthening below. A scratch edit that stamped every access at
  the start of its instruction did NOT turn tim_timer red: start and read use the same offset in every scenario, so the offset cancels; the stamp rule is covered by dev_log_records (cycle 4 and 5) instead.
- Found by the bite probes: check 18 as first written (reload 2, four steps) read 1 for ANY period of 2 or more (m = 1 is below every period), so a period off by one was invisible to the fixture. It now reads seven steps (m = 4), where a period of 2 or 4 reads 2; this is why the plan's later toy `timer-reload-off` (03-22) can turn tim_timer red through check 18.
- Manual facts: H8-12 to H8-16, H8-18, H8-20 to H8-22 and H8-24 are cited from the image reads of earlier plans (cites.txt, `img`); the page images were not re-read here and nothing was copied or committed. The in-guest timer checks are E2 self-consistency: the CPU cycle length (12 master clocks at low speed) is
  community-reported, and every such row depends on the one constant CYC_STEP = 256.

## Deviations from Plan

**1. [Rule 3 - files outside the plan list] The plan's 16-file waiver is exceeded (25 files over four commits).** Added because the stamps and the timer window cannot exist without them: `src/core/obs.c` and `ds_obs.h` (`ds_obs_dev` takes the stamp, as the 03-19 notes and the brief ask), `tests/conformance/obskinds.c` and
`cases/dev_log_records.toml` (the dev record stamp is the access stamp: the kind's items gained a CYCLE field and the dev_log_windows walk initialises the new `struct ds_bus` fields), `fixtures/src/mmu_mpr.s`, `mmu_mpr.checks` and the three mmu_mpr manifests (checks 32 to 41 re-judged, as the 03-20 notes and the brief ask).
**2. [Rule 1 - bug found by the policy list] `.align 16` filled with $FF.** The first tim_timer passed under the zero policy and failed under seeded-prng-v1 (`a DSV1 result byte is not $00, $50 or $46`): the padding in front of the wait loop was $FF, which is BBS7 on this target, executed as an instruction whose branch depends
on a zero-page cell. Found with the trace of the seeded run; fixed with `.align 16, $EA` (the padding is executed, so it is NOP). The policy machinery of D-127 did its job.
**3. [Rule 2 - robustness] mmu_mpr starts and stops the timer before check 32.** Checks 32 to 36 read a counter that was a stub of 0; a real timer's counter and reload are not reset values, and plan 03-24 draws them. The fixture now sets reload 0, starts and stops (7 cycles, no step), so the counter is 0 under every policy.
This changes the binary of the three images (new hashes in the manifests) but adds no check row, so every `checks = 60` literal and the exclude lists are unchanged.
**4. [Plan wording] The edges of timer_pure run for reloads 0, 1, 127 and every eighth, not for every reload.** The plan lists the reload-write, stop and restart behaviours without a coverage rule; the pure comparison (counter, borrows, TIQ bit, both read addresses) runs for all 128 reloads and the three phases. Reason: the
model is a per-clock loop in a debug build and the case runs once per runner (a gate has about 16); the whole kind costs about 0.3 s per runner.
**5. [Note] `ds_irq_pending` returns the vector address, not source bits.** Plan 03-22's must_haves name a pure query `ds_irq_pending_query(inst, stamp)` "returning the pending source bits"; this plan's sample needs the vector of the winner. 03-22 adds the bits query beside it (both can share the helper); the declarations live in ds_core.h.
**6. [Note] The T clear at entry has no in-guest witness.** startup.inc's TIMER vector stub is a JMP, whose fetch clears T anyway, so a handler cannot see whether the entry cleared it; the pushed T is checked (row 33). A C-level check of the cleared bit right after `ds_cpu_step` would do it and is left to plan 03-22 or 03-32.

No Rule 4 deviation. The shared-state toy control stayed RED_OK, so the standing note was not triggered.

## Evidence discipline

E0 for the mechanism: the core is compared with literals written in the case, the fixture or the model. The one E1-capable timer claim is `timer_pure_function` (ER-3): the step of the core equals 3 * 1024 master clocks and the period is (reload + 1) steps, against the manual's numbers (H8-20, H8-21) and a naive model; the
period is a derivation from the 143 usec to 18.3 msec range, not a printed sentence (A1), and the ledger entry LED-IRQ-0002 is not written yet (plan 03-34) and is not promoted. The in-guest counter rows of tim_timer (14 to 27) are E2 self-consistency: they depend on the community-reported CPU cycle length and on the
access-stamp rule of this plan (an assumption of LED-CPU-0004), and can only show that the core agrees with itself. The MAN rows follow register facts of the hardware manual (H8-12, H8-14 to H8-16, H8-18, H8-20 to H8-22); the ASSUMED rows name LED-IRQ-0002, -0003 and -0005 (hold on stop, no-op rewrites, bit 7 from io_buffer,
latch while masked, the pre latency and the pushed T), none of which exists as an entry yet. No ledger status changed, none of the eight ER-2 candidates was touched. No accuracy, timer-accuracy, CPU-speed, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. The timer window and the interrupt registers $1402 and $1403 are real; IRQ1 and IRQ2 (bits 0 and 1 of $1403) read 0 until plan 03-22, the pad behind the I/O window arrives in plan 03-27, the VDC and VCE are stubs until Phase 4 and the PSG until Phase 5, as before. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-57 (64-bit arithmetic): unsigned division and modulo only, `period` is `reload + 1` and never zero, `fold` treats a stamp at or before the origin as no steps, and ds_run's time_wrap refusal keeps the stamps below the guard. T-03-58 (interrupt storm): TIQ is one latch bit, the entry is
bounded at 8 cycles plus penalty (already in `ds_max_step_mclk`), runs are bounded by until_mclk. T-03-59 (circular timer evidence): the in-guest rows are labelled E2 in the checks file, the case and the fixture header, and only the C-level case claims E1.

## Notes for next plans

- **03-22 (interrupt registers, irq_stim, TIMR/IRQC regions):** the timer and controller state is in the instance and NOT in the digest. TIMR is origin (u64), counter, reload, enabled; IRQC is irq_disable (3 bits) and tiq_latch; the domain checks of D-121 are counter <= 127, reload <= 127, enabled and latch 0 or 1, disable < 8.
  The sample uses the I flag from before the instruction at the single statement `if (i_before == 0u)` in `ds_cpu_step_obs`: make it the `irq_i_sample` parameter there (pre default); tim_timer check 33 (T pushed after `cli; set`) is ASSUMED:LED-IRQ-0005 and holds under `pre` only, so tim_timer's case keeps the default.
  `ds_irq_read` reg 3 has IRQ1 and IRQ2 to add in bits 0 and 1; `ds_irq_pending` returns the vector address of the winner (TIQ today). The toy anchor for `timer-reload-off` is the line `uint64_t period = (uint64_t)inst->timer_reload + UINT64_C(1);` in timer.c (exactly once); a period of reload + 2 turns tim_timer red at check
  18 and timer_pure_function red, and a period of reload + 0 divides by zero for reload 0 (a scratch probe only turned it red on arm64 because integer division there does not trap: choose + 2).
- **03-23 (speed):** the access stamp uses `cycle_mclk` at the speed the instruction STARTED at; CSL and CSH switch the speed in `inst->speed` and the entry reads the speed after the instruction. The interrupt sample stamp is `start + total - mclk_per_cycle`, with the start speed's cycle length. If constants.def gains the timer
  hold and no-$7F rows (LED-IRQ-0003), the core must read them (the hold is a branch in `ds_timer_write`, the no-$7F-window is implicit in `fold`).
- **03-24 (power-up v2):** the timer reload and counter are the unspecified registers drawn under v2 (domain `tmr`); the reset in `ds_cpu_reset` clears only timer_enabled, tiq_latch and irq_disable. tim_timer initialises everything it reads (it sets the reload before every start, and reads no counter before a start); mmu_mpr does now too (the init block before check 32).
- **03-25 (block transfers):** each block byte is two data accesses with their own stamps: set `bus->last`/`rmw` (or add a per-access override) for them; the penalty is already summed in `bus->penalty` before the stamp is computed. The entry and the sample after the epilogue use the same helpers.
- **03-26 (chunk and random cases), 03-30 and later (snapshot):** `ds_instance` has `timer_origin` before `pc` and five bytes after `tma_zero`; the tiling list must include them. A raw comparison of those fields is the only chunk-invariance check of the timer until the digest has TIMR.
- **Observer kind users:** `observer_tim_timer` was not added here (the plan does not list it); the matrix is a case file only (`kind = "observer"`, `fixture = "authored:tim-timer"`, `until_mclk`, `sinks`, `chunks`), and the insn hook toy needs the case in `tests/controls/trace-hook-mutates/EXPECT` only if it should be proven red by that toy.
- **dev log users (03-27 pad):** the dev record stamp is the access stamp; `dev_log_records` items end in the access cycle index (page cycles minus 1 plus the penalty); `dev_log_windows` builds its own `struct ds_bus` with `cycle_mclk` 0 so that every access carries the instance's machine time. st_bypass's twelve stamps stay
  strictly increasing (they are 4 cycles into five-cycle instructions).
- **Timing:** `sh scripts/check` about 105 s wall here; the conformance stage is about 37 s (the standing seven toy runners run the whole list each, about 5 s apiece, of which timer_pure is about 0.3 s), sanitizers about 19 s.
- **Ledger:** LED-IRQ-0002, -0003 and -0005 are named by checks of tim_timer but written by plan 03-34; the pins `timer_pure_function` (0002), `tim_timer` (0003) belong there. The standing LED-CPU-0015 provenance gap is untouched (this plan writes no ledger entry).
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/timer.c, src/core/ds_timer.h, src/core/irq.c, fixtures/src/tim_timer.s, fixtures/src/tim_timer.checks, fixtures/manifest/tim_timer.toml, tests/conformance/cases/tim_timer.toml, timer_pure_function.toml, mutate_sweep_tim_timer.toml. Commits 6b5f903, 96346d1, 92276e6 and b9eecf8 are ancestors of HEAD.
