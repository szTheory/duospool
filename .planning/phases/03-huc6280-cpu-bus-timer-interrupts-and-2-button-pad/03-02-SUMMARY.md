---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 02
subsystem: core
tags: [ds_run, absolute-deadline, run-api-migration, chunk-sum, interleave, shared-state-toy]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: ds_run with an absolute until_mclk, the minimal HuC6280 core, the guest kind and the ds_proto fixture (plan 03-01)
provides:
  - the placeholder stepping API is deleted (src/core/advance.c, its advance and budgeted-run functions, its per-call cap and the DS_RUN_BUDGET family); ds_run is the only way to advance a machine
  - kinds run_until and run_chunk_sum (stepkinds.c) and REQUIRED cases run_until_abs and run_chunk_sum on the authored ds-proto
  - boot_built_rom, one helper that boots a built authored image, shared by the guest kind and the run kinds
  - budget_spin, spin_forever and every interleave kind migrated to absolute deadlines on the same LCG schedule
  - the shared-state toy re-pointed at the machine-time store in src/core/run.c (red on interleave_ab, interleave_twins, interleave_aba_order and also interleave_destroy_recreate)
affects: [03-03, 03-04, 03-09, 03-16, 03-35, 03-36]

actuals:
  tokens: 18552
  tasks: 2
  commits: 2
plan_head_before: 0168ec3548695ddbae0735c07a51b731d61d049e
plan_head_after: 77abfbabf7bddcb0adcb89c8e7b94c50a0816ef4

tech-stack:
  added: []
  patterns:
    - "every kind that steps a machine runs ds_run to an absolute deadline; a chunk of a schedule moves the deadline, never asks for a relative budget"
    - "a case compares chunked runs with the one-call run at one deadline past the guest's done state (D-95), and says so: E0 repeatability"
    - "the toy control anchors on the one distinct statement that stores machine time in run.c"

key-files:
  created:
    - tests/conformance/cases/run_until_abs.toml
    - tests/conformance/cases/run_chunk_sum.toml
  modified:
    - src/core/run.c
    - src/core/ds_core.h
    - src/core/poweron.c
    - tests/conformance/stepkinds.c
    - tests/conformance/spinkinds.c
    - tests/conformance/ilvkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/powkinds.c
    - tests/conformance/REQUIRED
    - tests/controls/shared-state/RULE
  deleted:
    - src/core/advance.c
    - tests/conformance/cases/advance_rules.toml
    - tests/conformance/cases/run_budget.toml
    - tests/conformance/cases/run_budget_stop.toml

key-decisions:
  - "Amendment record: D-68 and D-75 are superseded by D-91 (also written in the comment header of stepkinds.c). psg_noise and vdc_phase stay in the instance and in the digest until plans 03-03 and 03-36, so no golden, epoch or serial version changes here"
  - "The overshoot of a call that took no step is 0 (the plan truth: a deadline not ahead of machine time returns UNTIL with 0 steps and overshoot 0). Before this plan run.c reported machine_time - until for a past deadline"
  - "The timeout-budget mini-root case file is not edited: its expect budget=N,step=M keeps its meaning as absolute deadlines start+M, start+2M, ... start+N in master clocks"
  - "run_until items use result=until as the default and no over= key: the kind asserts overshoot = machine time - until for every item, so the time literal fixes the overshoot (needed to keep the expect line under the 400-character limit)"

requirements-advanced: [CORE-04]
requirements-completed: []  # none ticked: CORE-04 also needs mid-frame resume, the sink and device stops and the snapshot plans of this phase

duration: 35min
completed: 2026-11-11
---

# Phase 3 Plan 02: Run API migration Summary

**The placeholder stepping API is deleted and every kind that stepped the machine now runs ds_run to absolute deadlines in master clocks; run_until_abs pins the deadline, zero-step and stop-callback boundaries from the ds_proto cycle table, run_chunk_sum shows chunk lists of 1, 7, 168 and 3072 mclk
  reach the same end state as one call, and the shared-state toy still turns its cases red.**

## What was built

- **Task 1 (tracer), commit 8270845.** `kind_run_until` replaces `kind_run_budget`: the first expect item is `rom=NAME`, the others are fresh boots with `until`, `stop`, `result` (default until), `steps`, `time` and `again`. `again=N` makes a second ds_run with a deadline that is not ahead of
  machine time and requires UNTIL, 0 steps, overshoot 0 and a byte-identical instance (a copy of the whole instance block is compared). Every item also asserts overshoot = time - until and overshoot below `ds_max_step_mclk()` (D-94). `kind_run_chunk_sum` runs the reference to one absolute deadline
  past the DSV1 done state (asserted, D-95), then the chunk lists `1,7,168,3072,mix` (mix rotates the four sizes) on fresh boots; machine time, total steps, CPU registers and digest must equal the reference. `boot_built_rom` (spinkinds.c, declared in runner.h) is the one place that reads and boots
  `build/gate/fixtures/NAME.pce`; `kind_guest` now uses it.
- **Cases.** `run_until_abs` (9 rows: deadlines 0, 1, 24, 264 and 421; stops at 0, 100, 120 and 121; again deadlines 0, 3, 24 and 100) and `run_chunk_sum` (deadline 300000 mclk) are REQUIRED. `run_budget`, `run_budget_stop`, `advance_rules` and their REQUIRED lines are gone. Literals follow the
  manual cycle counts of the ds_proto instructions at 12 mclk a low-speed cycle: instruction ends 24, 48, 72, 96, 120, 180 (TAM 5 cycles), 204, 264, 288, 348 (STZ abs,X 5), 372, 420 (taken BNE 2+2), 480; none comes from a run.
- **Task 2, commit 77abfba.** `src/core/advance.c` and the placeholder section of ds_core.h (`DS_ADVANCE_MAX`, `ds_advance`, `enum ds_advance_status`, `enum ds_run_status`, `ds_run_placeholder`) are deleted. `kind_spin`: `spin_forever` loops ds_run with ever growing absolute deadlines (+1000 mclk
  each); `budget_spin` runs deadlines start+step, start+2*step, ... start+N and asserts the end time is the first instruction boundary at or past start+N, less than `ds_max_step_mclk()` past it, then reports timeout. `ilvkinds.c`: the schedule numbers are master clocks that move a per-instance
  absolute deadline (`run_to`, `run_chunked`, `at_boundary` helpers); the arena, canaries, digest and record comparisons are unchanged. `tests/controls/shared-state/RULE` anchors on `inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step(inst);` and adds one master clock on every fifth step
  of the whole process through a hidden static counter.

## Verification

- Task 1 verify as written: `sh scripts/check --stage conformance` and `--stage inventory` pass. Task 2 verify as written: `--stage build`, `--stage conformance`, `--stage named-controls` and `--selftest mutation-control-anchor` (all four RULE files) pass.
- Plan end: `sh scripts/check` PASS (58 s wall), `sh scripts/check --selftest` PASS (39 mutations, 13 s), `dash scripts/check` PASS, `sh scripts/audit-history --range 0168ec3..HEAD` clean (2 commits, 16 blobs) and the full-history audit clean. `grep -rn 'ds_advance\|DS_ADVANCE_MAX\|ds_run_placeholder' src tests scripts` prints nothing and `src/core/advance.c` does not exist.
- Mechanism checks done by hand and reverted (not committed): a wrong step literal in `run_until_abs` (until=24 with steps=2) turns that case guest_failure; a chunk-sum deadline of 100000 (before the guest is done) turns `run_chunk_sum` guest_failure. The toy control output in
  `build/gate/controls/shared-state/results.txt` shows guest_failure for interleave_ab, interleave_twins, interleave_aba_order, interleave_destroy_recreate and liveness_shared_state_selfcheck, `pass` for interleave_canaries (it does not compare digests), and no harness_failure.

## Deviations from Plan

**1. [Rule 1 - Bug] run.c reported a nonzero overshoot for a zero-step call with a past deadline**
- **Found during:** Task 1 (writing `run_until_abs`)
- **Issue:** the plan truth says a deadline not ahead of machine time returns UNTIL with 0 steps and overshoot 0, but plan 03-01's `finish()` returned machine_time - until for until below machine time (for example 21 for until=3 at machine time 24).
- **Fix:** overshoot is machine_time - until only when the call took at least one step and ended past the deadline; the ds_core.h comment says so. Files modified: src/core/run.c, src/core/ds_core.h (neither was in this plan's file list). Commit: 8270845.

**2. [Rule 3 - Blocking] kind_advance, advance_rules and the advance row were removed in Task 1, not Task 2**
- **Issue:** the rewritten stepkinds.c no longer contained the `pstate` helpers, and Task 1's commit has to build; removing them in Task 1 kept every commit green. The plan lists this removal under Task 2.
- **Files:** tests/conformance/stepkinds.c, runner.h, runner.c, REQUIRED, the case file. Commit: 8270845.

**3. [Rule 3 - Blocking] the run_until_abs expect line was cut to fit 400 characters**
- **Issue:** the first draft of the expect text was 680 characters and `scripts/conformance-flatten` refuses lines over 400 (LINE_TOO_LONG).
- **Fix:** `result=until` is the default, the `over=` key was dropped (the kind asserts overshoot = time - until, so the time literal fixes it), and three redundant rows were removed (deadlines 25 and 265, the stop at 100 with a far deadline). The comment in the case file records the remaining overshoot examples. Commit: 8270845.

**4. [Rule 3 - Blocking] boot_built_rom added to spinkinds.c and runner.h (spinkinds.c was a Task 2 file)**
- **Issue:** the run kinds in stepkinds.c need to boot the built ds-proto image; duplicating the read-and-boot code of kind_guest would have copied about 40 lines. The helper is extracted and kind_guest uses it. Commit: 8270845.

**5. Comment-only edits outside the file list:** src/core/poweron.c (one sentence that named the deleted advance.c) and tests/conformance/powkinds.c (the header line that pointed at the removed kinds). Commits 77abfba and 8270845.

## Evidence discipline

E0 only. `run_until_abs` is a mechanism and literal check of the run API from manual cycle counts; `run_chunk_sum` compares runs with each other (repeatability). The 300000 deadline is justified by the done-state assertion in the kind, not by a DuoSpool-derived literal. No ledger status changed, no
ROM or third-party material was added, and no accuracy, compatibility, cycle-accuracy, hardware or boot claim is made. The cycle counts of the opcode rows are still the manual values of plan 03-01, unchecked by the oracle of plan 03-09.

## Known Stubs

None. `psg_noise` and `vdc_phase` still exist and are written only by the power-on policy; nothing evolves them, and plans 03-03 and 03-36 remove them (D-91). The digest still carries them, so no golden moved.

## Threat Flags

None beyond the plan's register. T-03-05 (zero-step boundary, instance compared byte for byte), T-03-06 (every kind but `spin_forever` ends on an absolute deadline; that one lives in the supervised cancel-spin mini-root) and T-03-07 (interleave A/B/solo digest comparison plus the toy proven red) are implemented.

## Notes for the next plans

- The ds_run refusal def and its coverage rows are plan 03-35; `DS_RUNREF_*` and the run enums still live in ds_core.h.
- `ds_max_step_mclk()` is still computed over the 19-opcode subset; plan 03-09 recomputes it over cpu_ops.def and every overshoot assertion (`at_boundary`, `run_until`, `budget_spin`) follows automatically.
- Interleave cases run the synthetic plain-256k and plain-384k images as code: the real core executes their LCG bytes, with undefined opcodes as 2-cycle NOPs. Once plans 03-09 to 03-14 widen the opcode set those cases still compare instances with each other, so they stay valid, but their machine-time digests will change shape; no literal in them depends on the CPU.
- `boot_built_rom` reads `build/gate/fixtures/` relative to the working directory, like `kind_guest`; plan 03-04 moves the guest kind into guestkinds.c and should keep the helper next to it.
- runner.c shrank to about 65.9 KB and stays clear of the 8192-multiple blob shapes that `audit-history` flags (the audit is clean).
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. The cc65 step for CI remains with the plan that owns it.

## Self-Check: PASSED

Created files present: tests/conformance/cases/run_until_abs.toml, tests/conformance/cases/run_chunk_sum.toml. src/core/advance.c, run_budget.toml, run_budget_stop.toml and advance_rules.toml are absent. Commits 8270845 and 77abfba are ancestors of HEAD.
