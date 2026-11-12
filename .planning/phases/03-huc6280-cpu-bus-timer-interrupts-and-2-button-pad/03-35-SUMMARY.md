---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 35
subsystem: core
tags: [ds_run, refusals, run_refusals-def, coverage, non-mutation]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: ds_run on absolute deadlines, boot_built_rom and the ds-proto fixture (plans 03-01 and 03-02)
provides:
  - src/core/run_refusals.def, the closed set of ds_run refusal codes (DS_RUNREF rows, printed run.NAME, slot ceiling with _Static_assert), separate from refusals.def (F4)
  - ds_run_refusal_name() for the run.NAME strings
  - kind run_refuse and REQUIRED rows run_refuse_instance_null, request_null, struct_size, reserved, sink_unsupported, time_wrap
  - deferred kind run_refusal_coverage (D-86 pattern), REQUIRED
affects: [03-10, 03-19, 03-36]

actuals:
  tokens: 7300
  tasks: 2
  commits: 2
plan_head_before: 51cce19a587ba572b94e638ce8f6fb9dd9c5e5ba
plan_head_after: ccf0574ff85dbcf3a48d7037f8e293ecce60f42c

tech-stack:
  added: []
  patterns:
    - "a def file per refusal family with its own prefix and name table; coverage kinds read the def, never a literal list"
    - "a refusal row boots, runs the machine to a nonzero time, calls the refused request in several variants and compares raw bytes and digest before and after"

key-files:
  created:
    - src/core/run_refusals.def
    - tests/conformance/cases/run_refuse_struct_size.toml
    - tests/conformance/cases/run_refuse_instance_null.toml
    - tests/conformance/cases/run_refuse_request_null.toml
    - tests/conformance/cases/run_refuse_reserved.toml
    - tests/conformance/cases/run_refuse_sink_unsupported.toml
    - tests/conformance/cases/run_refuse_time_wrap.toml
    - tests/conformance/cases/run_refusal_coverage.toml
  modified:
    - src/core/ds_core.h
    - src/core/run.c
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED

key-decisions:
  - "Codes sink and deadline of plan 03-01 are renamed sink_unsupported and time_wrap (the names the plan fixes); their numbers 5 and 6 are unchanged and the numbers 1..6 are not reused"
  - "The slot ceiling is DS_RUNREF_SLOTS = 16, checked twice: the designated-initialiser name table fails to compile for a number at or above it, and an enum row count plus _Static_assert covers a def with too many rows"
  - "Variants per refusal live in the kind (struct_size five sizes, reserved three values, sink three pointer combinations, time_wrap three refused and two accepted edge cases), one REQUIRED row per code as the plan says"

requirements-advanced: [CORE-04]
requirements-completed: []  # none ticked: CORE-04 also needs mid-frame resume, the sink and device stops and the snapshot plans of this phase

duration: 25min
completed: 2026-11-11
---

# Phase 3 Plan 35: ds_run refusal set Summary

**ds_run refusals now come from their own run_refusals.def (run.NAME), each of the six codes is produced by one REQUIRED row that checks the instance bytes and digest are unchanged, and a deferred coverage row fails when a def member is unobserved.**

## What was built

- **Task 1 (tracer), commit bf1c7c6.** `src/core/run_refusals.def` holds `DS_RUNREF(name, number)` rows 1..6. `ds_core.h` expands it into `enum ds_run_refusal` (`DS_RUNREF_NONE = 0`, `DS_RUNREF_SLOTS = 16`) and declares `ds_run_refusal_name()`; `run.c` owns the name table, the row-count `_Static_assert` and the name function, and uses the renamed codes. `kind_run_refuse` (stepkinds.c; one runner.h line, one `kinds[]` row) takes `expect = "rom=NAME;refuse=CODE"`, boots the built ds-proto image, runs it past 600 master clocks so the instance is not at its boot state, then calls ds_run with each malformed request of that code. Every call must return REFUSED with `run.CODE`, 0 steps, overshoot 0, stop point INSTRUCTION, pad 0, the instance's machine time (0 for a NULL instance); a second call with a NULL result must also return REFUSED; the raw instance bytes and the digest must be identical before and after. `run_refuse_struct_size` is REQUIRED.
- **Task 2, commit ccf0574.** Variants for the five other codes: `instance_null` (a separate live instance is untouched), `request_null`, `reserved` (1, 0x80000000, 0xFFFFFFFF), `sink_unsupported` (dev only, insn only, both, each a valid non-overlapping buffer, W6: until plan 03-10 every sink is unsupported, from plan 03-10 the insn sink is accepted but the dev sink is still refused, and plan 03-19 re-points the row to an oversized capacity) and `time_wrap` (until = 2^64 - 1, until = 2^64 - guard, and machine time = 2^64 - guard with deadline 0, where guard is `ds_max_step_mclk()`; plus the edge itself accepted: deadline 2^64 - 1 - guard with a stop that holds gives CALLBACK with 0 steps, and machine time 2^64 - 1 - guard with deadline 0 gives UNTIL with 0 steps). `kind_run_refusal_coverage` is registered as a deferred kind and fails when any def member was not observed. Six case files, all appended to REQUIRED.

## Verification

- Task 1 verify as written: `--stage build`, `--stage core-boundary`, `--stage conformance` pass. Task 2 verify as written: `--stage conformance`, `--stage sanitizers` pass.
- Plan end: `sh scripts/check` PASS, `sh scripts/check --selftest` PASS, `sh scripts/audit-history --range 51cce19..HEAD` clean (2 commits, 18 blobs). `dash scripts/check` is optional (see the final report for its result).
- Mechanism checks done by hand and reverted (not committed): a one-line mutation in run.c that changes `pc` before the struct_size refusal turns `run_refuse_struct_size` guest_failure (47 assertions, tracer proven able to fail). A scratch cases TSV in the scratchpad directory without the `run_refuse_time_wrap` rows (END count adjusted) makes `run_refusal_coverage` guest_failure with `run.time_wrap was not observed by any run_refuse row` (recorded, not committed).

## Deviations from Plan

**1. [Rule 3 - Blocking] the case file comment for run_refuse_time_wrap was split over several lines**
- **Issue:** `scripts/conformance-flatten` refused a comment line over 400 characters (LINE_TOO_LONG).
- **Fix:** the comment is wrapped; no content change. Commit: ccf0574.

**2. Plan text wording:** the plan says "replace the plan 03-01 enum in ds_core.h"; the enum there already used `DS_RUNREF_sink` and `DS_RUNREF_deadline`, so those two were renamed (run.c and ds_core.h only; no test referenced them). No deviation in behaviour.

## Evidence discipline

E0 only. The refusal names, numbers and the non-mutation rule are the specification of this plan (D-93, F4), not derived from a DuoSpool output; the time_wrap edge values follow from the guard definition of D-94. No ledger status changed, no ROM or third-party material was added, and no accuracy, compatibility, cycle-accuracy, hardware or boot claim is made.

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-94 (refusal path tampering) is mitigated: every refusal row checks digest and raw bytes before and after, and the coverage row fails on an unobserved code.

## Notes for the next plans

- A new ds_run refusal (for example plan 03-10 or later device-sink codes) takes the next number in `run_refusals.def` (7 onward, 16 slots), needs a `rr_variants` branch in stepkinds.c and a REQUIRED `run_refuse_NAME` case; `run_refusal_coverage` then fails until it exists.
- Plan 03-10 changes what `sink_unsupported` means for the insn sink: the row's insn-only variant must then be dropped or re-pointed (the dev-only variant stays). Plan 03-19 re-points the row to an oversized capacity.
- runner.c is 66016 bytes (net +100); stepkinds.c is 25229 bytes. `audit-history` is clean, so no 8192-multiple shape was hit.
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation.

## Self-Check: PASSED

Created files present: src/core/run_refusals.def and the seven case files under tests/conformance/cases/. Commits bf1c7c6 and ccf0574 are ancestors of HEAD.
