---
phase: 04-vce-vdc-raw-video
plan: 02
subsystem: core-plumbing
tags: [vid-sink, obs, ds_run, fold-skeleton, refusals, observer-matrix, w1]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the dev sink and the obs layer (03-10, 03-19), ds_run refusals and run_refuse kinds (03-35), the timestamped bus (03-21), the interrupt sample (03-22)
provides:
  - struct ds_vid_sink (32-byte header, byte capacity), DS_VID_SINK_MAX_BYTES 64 MiB, DS_VID_MAX_DOTS 682, DS_VIDREC_VERSION 1, DS_VID_MAX_REC_BYTES 2792 (asserted)
  - vid appended last to struct ds_run_request; ds_obs_begin with three sinks; ds_obs_sinks_check table-driven over insn, dev and vid
  - ds_obs_vid, the whole-record drop-newest append with a sticky full flag (F-2)
  - ds_vdc.h (private entry points) and beam.c (inert ds_vdc_advance) with the fold at the three fold points of D-137
  - REQUIRED case vid_chan_append, vid variants of run_refuse_memory_invalid, run_refuse_sink_unsupported and run_refuse_struct_size, the observer matrix with the third sink
affects: [04-03, 04-04, 04-05, 04-06, 04-08, 04-09]

actuals:
  tokens: 13000
  tasks: 3
  commits: 4
plan_head_before: 6acf19ef2a9c33063f38ff6d4f3b3fd06b7a318e
plan_head_after: 48bd0e73657d87ada4c973e80e3f241db14c5c7a

tech-stack:
  added: []
  patterns:
    - "A sink channel is a row of the check table (obj size and alignment, record alignment and unit, limit) plus a running chan in struct ds_obs_log; the vid unit is one byte and its limit is a byte count"
    - "The step-end fold lives inside the public ds_cpu_step_obs around a static step body, so ds_run and ds_cpu_step share it; bus folds take the stamp the bus already computed"

key-files:
  created:
    - src/core/ds_vdc.h
    - src/core/beam.c
    - tests/conformance/cases/vid_chan_append.toml
  modified:
    - src/core/ds_core.h
    - src/core/ds_obs.h
    - src/core/obs.c
    - src/core/run.c
    - src/core/cpu.c
    - src/core/bus.c
    - tests/conformance/obskinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/run_refuse_struct_size.toml
    - tests/conformance/cases/run_refuse_memory_invalid.toml
    - tests/conformance/cases/run_refuse_sink_unsupported.toml

key-decisions:
  - "A malformed vid offer (NULL or empty record) is counted lost, never copied, and does not set the sticky full flag; only a record that does not fit sets it"
  - "The observer matrix gives the vid sink a byte capacity: the closed form of the deadline (lines due times 2792, capped at 16 MiB) for large and poison, the first four record sizes of the large run of the same mode for drop4, 0 and 1 for cap0 and cap1"
  - "ds_vdc_flag, ds_vdc_read, ds_vdc_write, ds_vce_read, ds_vce_write and ds_vdc_poweron are declared only; their defining plans may refine the signatures"

requirements-advanced: [VID-06]
requirements-completed: []  # plumbing only; VID-06 completes in 04-16

duration: 80min
completed: 2026-11-11
---

# Phase 4 Plan 02: Vid sink channel, refusals and the inert fold skeleton Summary

**The third caller-owned sink `vid` (byte capacity, whole-record drop-newest with a sticky full flag), table-driven sink checks over insn, dev and vid, the appended `vid` request member, and an inert `ds_vdc_advance` placed at the three fold points of D-137 with the step-end fold inside `ds_cpu_step_obs`; every digest and golden is unchanged.**

## What was built

- **Task 1 (tracer), commit 58cc1f5.** `struct ds_vid_sink { header[32]; bytes; capacity }` and the constants in ds_obs.h (with `_Static_assert`s that the longest record is 64 + 2728 = 2792 and that 64 MiB fits the address space); `vid` appended LAST to `struct ds_run_request`; `ds_obs_begin` gained
  the third sink (three callers updated); `ds_run` creates the log when insn, dev or vid is present (the anchors at the step and deadline lines are untouched); `ds_obs_end` writes the vid header (count, lost, seq, bytes stored, zero word) only for an attached channel. Because a run must not write
  through an unchecked pointer, `ds_obs_sinks_check` became table-driven over three channels in this task (struct checks, capacities with a per-channel limit, record ranges; the check order is kept). Runner kind `vid_chan_append`, part `attached`: a poisoned header and a 4096-byte guarded buffer,
  header read back by hand as 0/0/0/0/0, buffer untouched, run equal to the run without a sink. The tracer gate was re-run end to end after the commit (verify passed, nothing expanded on a broken base).
- **Task 2 (tdd), commit 55da153.** RED: the part `append` test did not build (no `ds_obs_vid`), and the two compile errors were the only ones. GREEN: `ds_obs_vid` (separate statement from the insn `chan_slot` anchor line). Part `append` has seven hand-derived capacity cases, a NULL/empty-record
  case, an absent-sink case and a NULL-log case. A mutant that dropped the sticky flag was seen red (append case 0, count 3 instead of 2) and restored. Refusals: vid memory_invalid per channel (inside the instance, over the request, own struct inside the instance, misaligned struct and bytes, NULL
  with nonzero capacity, wrap, over the result, and against the insn and dev structs and records, 15 variants), a dev wrap variant, vid sink_unsupported (64 MiB + 1 alone, beside valid insn and dev, an oversized insn beside a valid vid, UINT32_MAX, NULL bytes), the boundary pair (exactly 64 MiB with
  NULL is memory_invalid; with a 64 MiB buffer it is accepted and writes its header), and the OLD-size request on struct_size. The observer matrix now allocates the vid sink and guards, checks a header twin (count, lost = seq - count, bytes within capacity, zero word, a walk over the stored
  records), the prefix rule against the large run, chunk accumulation equality and the stop-run prefix.
- **Task 3, commit 2dc701e.** `ds_vdc.h` (private prototypes with the D-137 contract text), `beam.c` (inert `ds_vdc_advance`), `cpu.c` (static step body; the public `ds_cpu_step_obs` reads `start`, runs the body and folds at `start + total - 1` as one statement on its own line; the sample folds at
  the existing sample stamp before `ds_irq_pending`), `bus.c` (folds at the head of window 0 and 1 reads, the `$1403` read and window 0 and 1 writes, with the stamp computed after `charge_penalty`; `$1402`, `$1400` and `$1401` have none).
- **Fix commit 48bd0e7.** The Linux gcc 13 strict C11 run refused one 200-byte `snprintf` message (`-Wformat-truncation`); the message text was shortened, the check is unchanged.

## Verification

- Task verifies: Task 1 (`--stage build`, `core-boundary`, `conformance`, RESULT vid_chan_append pass), Task 2 (`conformance`, the three RESULT greps, `--stage sanitizers`), Task 3 (static counts 2 in cpu.c and 4 in bus.c, `conformance`, `insn_sink_shapes` pass, `--selftest mutation-control-anchor`): all PASS. At HEAD before Task 3 both fold counts were 0.
- Full `sh scripts/check` PASS after each task (Task 1 on a superset tree, narrow stages re-run on the committed tree; Tasks 2 and 3 on the committed tree). `sh scripts/check --selftest` PASS at the end; `sh scripts/audit-history --range 6acf19e..HEAD` clean.
- **Linux gate-gcc:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` first FAILED on one `-Werror=format-truncation` in obskinds.c (fixed in 48bd0e7), then PASS (gcc 13, cmake 3.28.3, `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`, linux/amd64, one-off local run, not a CI claim).
- Digests and goldens: no committed digest, golden or literal changed (digest_stream_*, digest_region, the two accept goldens, insn_sink_shapes and every pre-existing case pass unchanged), which shows the fold skeleton is inert.
- Evidence class E0: mechanism and non-interference only; every literal in the new checks is a layout figure or hand-derived from the F-2 rule, none from DuoSpool output. No hardware, accuracy or compatibility claim.

## Deviations from Plan

1. **[Rule 3 - Blocking] Table-driven sink checks landed in Task 1, not Task 2.** A tracer that accepts a vid sink must validate it before the obs layer writes its header, or ds_run would write through an unchecked caller pointer. The `ds_obs_sinks_check` change (and `sink_ref_vid`) is therefore in
   58cc1f5; the append function `ds_obs_vid` stayed in Task 2 so the RED/GREEN step is real. Files unchanged against the plan's list.
2. **[Rule 1 - Bug in the plan text] Sticky-full example arithmetic.** The plan's example (64, 2792, 64, 64 into 2900 bytes) cannot show stickiness: after 2856 bytes only 44 bytes remain, so the third record would not fit anyway. The case uses capacity 3000 with 64, 2792, 2792, 64: two records (2856
   bytes) are stored, the third does not fit and sets full, and the fourth (64 bytes, which would fit the 144 left) is lost; count 2, bytes 2856, seq 4, lost 2. Six further capacity cases were added (all fit, large first, exact fit, first record too large, capacity 0, capacity 1).
3. **[Rule 2 - Missing critical behaviour] Malformed offer rule.** A NULL record pointer or an empty record is counted lost without a copy (a copy through NULL is the Phase 3 UBSan finding) and does not set the sticky flag. This makes "capacity 0 stores nothing" hold for every offer.
4. **[Rule 1 - Linux strict C11] `-Wformat-truncation` in the vid header message of the observer matrix** (the defect class the brief names); fixed in 48bd0e7.
5. **[Note] Request-layout change.** `sizeof(struct ds_run_request)` grew by one pointer; no committed literal pins it (the `struct_size` case uses `sizeof`); the OLD-size variant covers the skew.
6. **[Note] Observer vid twin is vacuously exercised until a producer exists.** All vid logs are empty in this plan, so the record walk, the prefix rule and the drop4 capacity computation (0 bytes) run on zero records; the code paths for non-empty logs are first exercised by plan 04-04. The twin of the header (zeros, guard bytes, poison, chunk equality) is live now.

## Known Stubs

- `src/core/beam.c` `ds_vdc_advance`: inert body (no VDC or VCE state exists); resolved by plan 04-03 (grid, frame wrap) and 04-06 (ladder). Recorded in `.planning/WINDOWS.md` (id 4, stub, open).
- `src/core/ds_vdc.h`: `ds_vdc_flag`, `ds_vdc_read`, `ds_vdc_write`, `ds_vce_read`, `ds_vce_write`, `ds_vdc_poweron` are declared and not defined; resolved by plans 04-03 (VCE) and 04-04 (VDC). The windows 0 and 1 read and write stubs of bus.c are unchanged.

## Threat Flags

None. The new surface (the vid sink pointer) is the one in the plan's threat register; T-04-04 to T-04-07 are mitigated and each has a control: per-channel overlap, alignment and wrap variants, the 64 MiB limit pair, the old-size request, and the poisoned-header and guard-byte checks.

## Notes for next plans

- `ds_obs_vid(log, rec, rec_bytes)` is the producer entry point for 04-04: it copies one whole record (built in a stack buffer) and handles the full flag itself; a producer must not pre-check capacity.
- The observer matrix expects records with `rec_bytes` as a u16 at +16 and 64 to 2792 bytes (D-161); a large vid sink that loses a record is `harness_failure`. Its capacity is capped at 16 MiB (`OB_VID_LARGE`); a run whose closed form needs more must raise the cap in obskinds.c.
- `DS_VID_MAX_DOTS` has no `_Static_assert` against `DS_CONST_line_mclk` yet (the constant lands in 04-03).
- `ds_cpu_step_obs` already folds at `start + total - 1`; with `cycle_mclk = 0` style bus walks (dl_windows) the stamp equals machine time and the skeleton must stay a no-op for stamps at or below the folded point (F-29).
- Wave W1 end (after 04-01 and 04-02): `LINUX_REPRO_PARTS="gate-gcc gate-clang core-boundary-gcc" sh scripts/linux-repro` is owed by the orchestrator; only gate-gcc was run here.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: src/core/ds_vdc.h, src/core/beam.c, tests/conformance/cases/vid_chan_append.toml, tests/conformance/REQUIRED line vid_chan_append.
- FOUND commits: 58cc1f5, 55da153, 2dc701e, 48bd0e7 (each an ancestor of HEAD).
