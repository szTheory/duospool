---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 23
subsystem: speed-switch-fixture-stimulus-record-table-w4-constants
tags: [speed-switch, csl, csh, spd-speed, timer-as-clock, stimulus-table, record-format-4, constants, observer, w4]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: CSL and CSH charged at the old speed with the speed bit as state (03-14), cycle_probe and cyc_probe (03-15), the VDC and VCE penalty (03-16), the record printer, constants table and goldens (03-18), the closed-form timer and access stamps (03-21), the declared irq_stim (03-22)
provides:
  - fixtures/src/spd_speed.s (DSV id 12, 13 checks) - the speed switch and the VDC and VCE penalty measured in-guest with the timer as the clock, plus a probe sequence at the fixed address $E100 for the trace case
  - cases spd_speed, mutate_sweep_spd_speed, spd_speed_trace (kind cycle_probe, 9 items including the first instruction at the power-up low speed), observer_tim_timer (the observer matrix over the timer fixture), record_stimulus_shown
  - the run record table [stimulus] (events = N, then event_I = "AT,LINE,LEVEL" in decimal), DS_TEXT_FORMAT_VERSION 4, record-rules.awk, reckinds.c, both goldens and the record-lint and determinism mini-roots updated; kind record_lines applies the case irq_stim, so the lint and the determinism stage read a real stimulus table
  - four constants rows read by the core: timer_hold_on_stop (LED-IRQ-0003), irq_sample_cycles_before_end (LED-IRQ-0006), interrupt_entry_cycles (LED-CPU-0009), speed_switch_extra_cycles (LED-CPU-0006); their ledger ids joined [withheld] (22 of the 24 the line allows)
affects: [03-24, 03-25, 03-26, 03-29, 03-30, 03-32, 03-34]

actuals:
  tokens: 21400
  tasks: 2
  commits: 3
plan_head_before: b07fd9794ad3c929e4fb5e8f3b4ad1f5f5f55cca
plan_head_after: a2078f134f0985fadd4ba6b4a37efa058657eefe

tech-stack:
  added: []
  patterns:
    - "a speed or penalty is measured in-guest by a fixed loop and the timer counter delta: the expected counter is written by the assembler from the instruction counts (E2 self-consistency, the clocks per cycle are inputs)"
    - "which speed a switching instruction is charged at is made visible by tuning the measured interval to end a few master clocks past a step boundary under one reading and a few before it under the other (12 and 6 clocks here), so the two readings and a no-op switch read three different counters"
    - "a penalty of N cycles per access is compared against a base loop over a work-RAM cell with exactly step / (penalty * clocks per cycle) accesses: the difference in counter steps is an exact integer for every timer phase, so the check needs no tuning"
    - "a trace item list for a fixture whose long loops would overflow the instruction sink is stopped by until_mclk right after the probe (the probe sits first, at a fixed padded address) so lost stays 0"

key-files:
  created:
    - fixtures/src/spd_speed.s
    - fixtures/src/spd_speed.checks
    - fixtures/manifest/spd_speed.toml
    - tests/conformance/cases/spd_speed.toml
    - tests/conformance/cases/mutate_sweep_spd_speed.toml
    - tests/conformance/cases/spd_speed_trace.toml
    - tests/conformance/cases/observer_tim_timer.toml
    - tests/conformance/cases/record_stimulus_shown.toml
    - tests/gate/record-lint/missing-stimulus-table/record.toml
    - tests/gate/record-lint/missing-stimulus-table/EXPECT
    - tests/gate/record-lint/stimulus-count-mismatch/record.toml
    - tests/gate/record-lint/stimulus-count-mismatch/EXPECT
    - tests/gate/record-lint/ok-stimulus-events/record.toml
  modified:
    - src/core/constants.def
    - src/core/cpu.c
    - src/core/timer.c
    - src/core/run.c
    - src/core/ds_core.h
    - src/text/text.c
    - src/text/ds_text.h
    - scripts/lib/record-rules.awk
    - tests/conformance/reckinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - tests/gate/record-lint (7 accept-style records)
    - tests/gate/determinism (4 records)

key-decisions:
- "The speed switch itself was already implemented in plan 03-14 (CSL and CSH charged at the speed the instruction started at, the speed bit state, the sample stamp at the old cycle length), so Task 1 changed no core code: the plan's deliverable is the evidence. The new committed checks that turn red on a no-op CSH are spd_speed
    checks 2, 3, 5, 6 and 7, the spd_speed_trace items 3, 5 and 7 (speed byte) and, since plan 03-15, cycle_adders_manual items 15 to 18"
  - "spd_speed uses the power-up speed in two places, so a core that powers up at the high speed is red twice: check 1 (the fixed loop before any switch reads the low-speed count, MAN:H8-18) and the first item of spd_speed_trace (the SEI at the reset address has speed byte 0)"
- "[stimulus] is events = N followed by event_1 to event_N, one line per event, each as AT,LINE,LEVEL in decimal fields separated by commas. One line per event (not one list value) because 16 events of 20-digit times would break the 400-character line rule; the lint rule checks the key sequence against the events value (at most
    16) and the three-field shape with a line of 1 or 2 and a level of 0 or 1. The printer refuses a slot list that ds_irq_stim_set could not have produced (a state error)"
- "The four constants rows: timer_hold_on_stop 1 (the no-$7F-window of the timer stays implicit in the closed form and is named in the row comment), irq_sample_cycles_before_end 1 (the sample stamp is the end minus this many cycles at the starting speed; 1 or 2 by a static assert), interrupt_entry_cycles 8 (replaces the
    DS_INT_ENTRY_CYCLES literal), speed_switch_extra_cycles 0 (an extra cost of a switch that actually changes the speed, charged at the old speed; also in the ds_max_step_mclk bound). Each has a static assert range and each bites: a changed value turns a committed case red"
  - "No record format layout change was needed for the [withheld] ids line: 18 plus the four ids is 22 ids against the cap of 24, the longest record line is 362 characters. The next plan that adds ids (03-25 and 03-29 were expected to) has room for two; LED-IRQ-0008 (the stimulus) is not in the list and would be the 23rd"
- "record_stimulus_shown is a kind record_lines case (a synthetic boot with the case irq_stim applied), so its record is written to the records directory, read by the real record lint and reproduced by the determinism stage. A first version made kind record_run a record producer (three kind lists in scripts changed); the
    determinism stage then went red because its second process runs in another working directory where the authored images are not found, so the version was replaced by commit 3 and the scripts are unchanged"

requirements-advanced: [CPU-06, CORE-06, CPU-03]
requirements-completed: [CPU-04]  # CPU-04: speed switching changes the timing on the master-clock timeline (spd_speed in-guest at E2, spd_speed_trace and cycle_adders_manual from the trace against page literals) and the VDC and VCE penalty is applied at the speed in force (vdc_penalty_assumed, spd_speed checks 9 to 12, ASSUMED
    LED-CPU-0017); the entries LED-CPU-0006 and LED-CPU-0017 are still written by plan 03-34 and CPU-06 still needs them

duration: 70min
completed: 2026-11-11
---

# Phase 3 Plan 23: Speed switch fixture, [stimulus] record table and the W4 constants Summary

**The CPU speed switch is now measured from inside the guest with the timer as the clock (13 checks: power-up low speed, the same loop at both speeds, a repeated switch, CSH and CSL charged at the OLD speed by two intervals tuned to a step boundary, and the one-cycle VDC and VCE penalty at the speed in force), the instruction
    trace shows the speed byte and the machine time of the switching instructions against page literals, the run record prints the declared IRQ stimulus in a `[stimulus]` table (record format 4), and four more named modelling constants (timer hold on stop, sample stamp, interrupt entry cycles, speed-switch cost) are printed with
    their ledger ids and read by the core.**

## What was built

- **Task 1 (tracer), commit 6c8eb2d.**
- `fixtures/src/spd_speed.s` (DSV id 12, 13 checks, manifest pinned at 256 KiB): check 1 measures a fixed loop (2435 CPU cycles from the timer start access to the counter read) before any switch and expects nine steps of 3072 master clocks (counter 118, MAN:H8-18 for the low speed, the step count E2); 2 and 3 repeat it after CSH
    (125) and CSL (118); 4 relates the two in the guest (steps at low = 4 * steps at high + the remainder of the loop length over 256, GUEST); 5 and 6 are CSH at the high speed and CSL at the low speed (no change); 7 and 8 are the tuned intervals (3084 master clocks for CSH, one step; 3066 for CSL, no step), which say the
    switching instruction is charged at the old speed (a new-speed charge or a no-op reads differently); 9 to 12 compare a loop of 256 (low) or 1024 (high) reads of the VDC, VCE and PSG windows with the same loop over a RAM cell, the difference being exactly one step for the VDC and VCE (penalty of one cycle at the speed in
    force) and none for the PSG window (outside the range); 13 shows the probe ran.
- The subroutine `probe` at the fixed logical address $E100 (padded and pinned by an `.assert`) runs CSH, INX, CSH, INX, CSL, INX, CSL, INX; case `spd_speed_trace` (kind cycle_probe, until_mclk 100000 so the sink holds the early records only) judges 9 items: the SEI at the reset address at L (2 cycles, S8-77), CSH at L, INX at
    H, CSH at H, INX at H, CSL at H, INX at L, CSL at L, INX at L, each with its speed byte and 3 or 2 cycles of the speed in force (H8-25, S8-49).
  - Cases `spd_speed`, `mutate_sweep_spd_speed` (13 of 13 literals flip exactly their own check) and `observer_tim_timer` (the five sink configurations, whole, 7 chunks and a stop callback over the timer fixture: 259 assertions), all added to `REQUIRED`.
- **Task 2, commit 553c634.**
  - `text.c`: `emit_stimulus` after the `[model_*]` tables, `DS_TEXT_FORMAT_VERSION` 4. The goldens were edited by hand from the defs (format_version 4, `[stimulus] events = 0`, four constants rows, four ids in `[withheld]`); the digest, profile and content hashes did not change because no state was added.
  - `constants.def` rows `timer_hold_on_stop`, `irq_sample_cycles_before_end`, `interrupt_entry_cycles`, `speed_switch_extra_cycles`; `timer.c` (the stop branch reads the hold constant), `cpu.c` (the sample stamp, the entry cycles through `DS_INT_ENTRY_CYCLES`, the extra cycles of an effective switch), `run.c` (`ds_max_step_mclk`
    includes the switch cost) and `ds_core.h` read them; static asserts bound the ranges.
  - `record-rules.awk`: the stimulus table is in the sequence after the model tables, its keys must be `events` then `event_1` to `event_N` with N the value of events (at most 16), every event value three decimal fields with a line of 1 or 2 and a level of 0 or 1 (a colon is never printed). `reckinds.c`: the expected keys and the
    row count (one more row), the refusal-prefix literal, five printer controls (a stimulus line above 2, a level above 1, a used slot after an unused one, times that go back, a nonzero unused slot are each a state error), `check_stimulus_rows` (the table against the case's own parsed irq_stim) and `kind_record_lines` applying the
    case's irq_stim to its boot and expecting one `event_N` line per declared event (commit a2078f1).
  - Case `record_stimulus_shown` (five events, the last time above 2^32), mini-roots `missing-stimulus-table` and `stimulus-count-mismatch` (REC_TABLES each) and `ok-stimulus-events`; the seven accept-style record-lint records and the four determinism records gained `[stimulus] events = 0`.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage fixture-lint`, `fixtures`, `conformance`; Task 2 `--stage conformance`, `record-lint`, `record-lint-negatives`, `determinism-negatives`. The tracer gate (auto mode) re-ran the end-to-end verify before the expansion: it passed (`Tracer verified end-to-end - expanding`).
- Plan end: `sh scripts/check` PASS (the first full run after commit 553c634 was RED in the determinism stage, deviation 4; it is green after the fix commit, wall time over 170 s here and not timed exactly), `sh scripts/check --selftest` PASS (65 PASS lines, 25 s), `dash scripts/check` PASS, `sh scripts/audit-history --range
    b07fd97..HEAD` clean (3 commits, 40 blobs, 1375 blob-path pairs, run before this docs commit). The nine toy controls stayed RED_OK (named-controls PASS), `shared-state` included.
- Bite shown with scratch edits (each restored byte for byte and `cmp`-checked, nothing committed): CSH made a no-op turns spd_speed check 2, spd_speed_trace item 3, cycle_adders_manual item 16 and vdc_penalty_assumed red; CSL and CSH charged at the new speed turn spd_speed check 7 and spd_speed_trace item 2 red (the tuned
    interval works); a power-up speed of high turns spd_speed check 1, cycle_adders_manual item 1 and the digests red; each of the four new constants changed in the def (hold 0, sample cycles 2, entry cycles 9, switch extra 1) turns a committed case red (tim_timer and timer_pure_function; irq_ctl_pre check 33;
    run_overshoot_bound; cycle_adders_manual and spd_speed_trace) besides the two goldens; the stimulus printed with a colon turns record_stimulus_shown red.
- Manual facts: the hardware manual copy in a scratch directory outside the repo (sha256 40a32e5d... equals 03-RESEARCH) was read as a page image for H8-18 (the reset sets the low speed mode); H8-25 (CSL and CSH one byte, three cycles) is cited from the earlier image reads of the registry (`img`) and was not re-read. Nothing was copied or committed.

## Deviations from Plan

**1. [Plan wording] Task 1 changed no core code.** The plan's action says "Make CSL/CSH charge 3 cycles at the old speed and switch the speed bit after their last cycle; the sample stamp uses the old cycle length"; that is what plan 03-14 and 03-21 already do, and cycle_adders_manual (03-15) already asserted CSH at L and the INX
    behind it at H. The evidence added is spd_speed and the trace case; the brief's concern (a no-op CSH turns no committed check red) was already false after 03-15 and is now covered in three ways.
**2. [Rule 3 - files outside the plan list] The plan's 27-file waiver is exceeded (37 files over three commits).** Added because the work needs them: `tests/conformance/cases/spd_speed_trace.toml` (the committed speed-byte and machine-time check the brief asks for), `src/core/timer.c`, `src/core/run.c` and `src/core/ds_core.h` (the constants rows must be read by the core, 03-21 note),
    `tests/gate/record-lint/missing-machine-table` and `missing-model-table` (they are accept-style records and would otherwise fail for two reasons), and the mini-roots `stimulus-count-mismatch` and `ok-stimulus-events`.
**3. [Plan wording] `record_stimulus_shown` declares five events, not two.** The plan text says "events = 2" for an irq_ctl_pre boot; the case declares its own list (five events, one tie, one time above 2^32, both lines and both levels), which covers more of the printer and the lint, and the expect literals are that list.
**4. [Rule 1 - found by the full gate] The first version of the stimulus case broke the determinism stage.** "record-lint accepts it" needs a record that holds events, and no record-producing case declared a stimulus. Commit 553c634 made kind `record_run` (authored fixture) a record producer and added it to the three kind lists
    of `scripts/check`, `conformance-run` and `determinism-run`; the full gate then failed in the determinism stage, because its second process runs in another working directory where `build/gate/fixtures` is not found (the authored cases already harness-fail silently there and only the records are compared). Commit a2078f1
    turns `record_stimulus_shown` into a kind `record_lines` case (a synthetic boot, no authored image) and restores the three scripts and `kind_record_run`; `kind_record_lines` applies the case irq_stim and expects one `event_N` line per declared event.
**5. [Design] The no-$7F-window of the timer is not a separate row.** The plan lists "timer hold on stop and no $7F window (LED-IRQ-0003)" as one of four rows; the row is `timer_hold_on_stop`, and the no-$7F-window (the borrow loads the reload value at once) stays implicit in the closed form and is named in the row comment,
    because the four rows of the plan name four ledger ids and a second implementation of the window would be a model parameter (D-113: not in Phase 3).

No Rule 4 deviation. The shared-state toy control stayed RED_OK (the standing note was not triggered: run.c gained only the speed-switch cost in the bound, not the machine-time store the control anchors on).

## Evidence discipline

E0 for the mechanism. Every in-guest timer row of spd_speed is E2 self-consistency (ER-3): the expected counter is the assembler's computation from the instruction counts and three inputs, the community-reported 12 master clocks per CPU cycle at the low speed, the manual's prescaler giving 3 at the high speed (LED-CPU-0003) and
    the access stamp rule (LED-CPU-0004); a core can only show that it agrees with itself. The facts that are the manual's are the power-up low speed (H8-18, check 1; its step count is E2) and the one-byte length of CSL and CSH (H8-25, check 13). The ratio, the cost of a switch (zero extra cycles) and the charge at the old speed
    (rows 7 and 8, also the trace case) are modelling assumptions, ASSUMED LED-CPU-0006 (open; alternative: the last cycle of the instruction already at the new speed), and the VDC and VCE penalty is ASSUMED LED-CPU-0017; neither entry exists as a ledger file yet (plan 03-34 writes them) and none was promoted, none of the eight
    ER-2 candidates was touched. The trace case compares the core's charged time with page literals (SEI S8-77, INX S8-49, CSH and CSL H8-25 map cells); the record_stimulus_shown case compares the printed table with the list the case declared (E0). The stimulus is a test hook (ER-5, D-112), not evidence about the VDC. No
    accuracy, CPU-speed, cycle-accuracy, compatibility, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. The VDC, VCE and PSG windows remain stubs until Phases 4 and 5 (the penalty is charged by the bus, the windows read their stub values), the pad is plan 03-27. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-63 (cycle length selection): the speed bit is read once at the start of the step, and `ds_max_step_mclk` takes the longest row (with the switch cost) at the low-speed cycle length; run_overshoot_bound still passes. T-03-64 (overclaim): every speed and penalty row is ASSUMED class and the
    rows name LED-CPU-0006 (open) and LED-CPU-0017; the four constants rows name their ledger ids in the record. T-03-65 (record values): the stimulus is printed as decimal fields with commas, the lint refuses a colon, a slash, 0x and a date shape in any value, and the printer refuses a slot list the core could not have
    produced.

## Notes for next plans

- **03-24 (power-up v2):** nothing new in the digest from this plan (no instance state was added). spd_speed initialises every cell it reads and reads the timer only after starting it (the reload is written before every start); it does not read power-up RAM, MPR0 (set to $FF and back to $F8), or the io_buffer (bit 7 of every counter read is masked).
- **03-25 (block transfers) and 03-26:** the guest kind applies irq_stim, and so does kind record_lines now (a record case with a stimulus is a record_lines case on a synthetic boot, which the determinism stage can reproduce; an authored-fixture record case cannot be, see deviation 4). The record's `[withheld]` ids line holds 22
    ids against the cap of 24 (the longest line is 362 characters): two ids of room. The cheapest next id sources are the new model rows (tma_zero exists) and constants rows; if more
    than two ids are needed, change the layout in the same task as a record format bump (standing note), writing the golden blocks from the defs. `speed_switch_extra_cycles`: a block transfer that changes the speed is not a case (only CSL and CSH change it).
- **03-29 and 03-30 (snapshot):** `inst->speed` is in the CPU0 region already; the speed bit is state (D-101) and a restore must refuse a value above 1 (the domain checks of D-121 list it).
- **03-32 (promotion) and 03-34 (ledger):** write LED-CPU-0006 (speed switch: three cycles at the old speed, effect from the next cycle, no grid re-alignment, no extra cost; open; alternative `last_cycle`, the instruction's last cycle already at the new speed; pin spd_speed or spd_speed_trace; checks 2 to 8 are E2 and 7 and 8
    depend on the stamp rule), LED-CPU-0005 (CSL and CSH one byte, three cycles, H8-25 map cells only, power-up low speed H8-18; pins cycle_adders_manual and spd_speed_trace), LED-CPU-0017 (the penalty; pin vdc_penalty_assumed; spd_speed checks 9 to 12 are E2 and show the range and the speed in force), LED-IRQ-0003 (the hold on
    stop constant), LED-IRQ-0006 (the sample stamp constant), LED-CPU-0009 (the entry cycles constant; only run_overshoot_bound reads it, no in-guest check measures the 8 cycles). The record prints all four ids already.
- **Constants rows and their readers:** `DS_INT_ENTRY_CYCLES` is now the constants row; the sample stamp reads `irq_sample_cycles_before_end`; `ds_timer_write` reads `timer_hold_on_stop`; CSL and CSH add `speed_switch_extra_cycles` when the speed changes. A later plan that changes a value bumps `DS_BEHAVIOUR_EPOCH` (the header comment of the def says so).
- **Record layout (F10):** `[stimulus]` is one table with a variable number of lines, so `reckinds.c` expects `stimulus.events` only for the skeleton (no stimulus), `check_stimulus_rows` compares the lines of a record_lines case with its own irq_stim, and `record-rules.awk` checks the key sequence against the events value. The
    `[input]` table of the pad plans goes after `[stimulus]` and before `[constants]` (D-53 order: model tables, stimulus, input, constants, run): add `input` to `seq_accept` between them, to `build_expected_keys` in `reckinds.c`, and bump `DS_TEXT_FORMAT_VERSION`.
- **Timing:** the spd_speed fixture runs in about 533000 master clocks (the sweep runs it 14 times in the conformance stage, a few seconds); the trace case stops at 100000. `sh scripts/check` wall time is in the final report. CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified,
    pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/src/spd_speed.s and spd_speed.checks, fixtures/manifest/spd_speed.toml, the cases spd_speed, mutate_sweep_spd_speed, spd_speed_trace, observer_tim_timer and record_stimulus_shown, and the mini-roots missing-stimulus-table, stimulus-count-mismatch and ok-stimulus-events. Commits 6c8eb2d, 553c634 and a2078f1 are ancestors of HEAD.
