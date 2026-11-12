---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 19
subsystem: observers-dev-log-trace-rights
tags: [dev-sink, observer-matrix, trace-hook-mutates, trace-out, sink-max-records, bus-hook, w3]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the instruction sink, hook and run.memory_invalid (03-10), the bus choke point with hw_read, hw_write and ds_bus_write_phys (03-16), the cyc_probe penalty section (03-16), the run refusal set (03-35), the toy control mechanism (03-02, 03-16)
provides:
  - src/core/ds_obs.h and obs.c: struct ds_dev_sink (header 16 bytes, records pointer, capacity, like the insn sink), the dev record (24 bytes little-endian: u64 stamp, u32 phys, value, kind write/read/ST, flags penalty/physical, aux = the latch after the access, u16 pc), ds_obs_dev, struct ds_obs_chan and struct ds_obs_log with an insn and a dev channel, DS_SINK_MAX_RECORDS = 1048576
  - ds_obs_sinks_check (replaces ds_obs_insn_sink_valid): structs first (memory_invalid), then capacity above DS_SINK_MAX_RECORDS (sink_unsupported), then record ranges against the call's memory, the own struct and the other sink's struct and records (memory_invalid)
  - src/core/bus.c: log_dev, called from hw_write for every write to windows 0-5 (kind write, or ST for ds_bus_write_phys) and from hw_read for reads of window 4; struct ds_bus carries the log pointer; a NULL bus logs nothing
  - run.sink_unsupported re-pointed to an oversized capacity; run.memory_invalid covers the dev sink and the two sinks against each other
  - runner kinds observer (config matrix), dev_log (guest records and the window rule) and trace_out_rights; option --trace-out CASE_ID; REQUIRED cases observer_ds_proto, observer_cyc_probe, dev_log_records, dev_log_windows, trace_out_rights
  - toy control trace-hook-mutates (RED_OK on observer_ds_proto)
  - scripts/conformance-run trace_out_check: explicit --trace-out ds_proto compared with wc and the host sha tool, four refused requests
affects: [03-20, 03-21, 03-22, 03-25, 03-26, 03-27, 03-29, 03-32]

actuals:
  tokens: 32000
  tasks: 3
  commits: 3
plan_head_before: ce6f5f381eed5260b394dea6dc6ddb285674863a
plan_head_after: fc8ed84a2269d9849c00e2374c40782643176850

tech-stack:
  added: []
  patterns:
    - "a sink is a channel of the run's local log: struct ds_obs_log holds an insn and a dev ds_obs_chan, each with on, header pointer, record pointer, capacity and the running count, lost and seq; chan_slot claims the next record or counts the loss, so both hooks share one overflow policy and the toy anchors on one line"
    - "the dev hook is called from the two functions every hardware-page access already passes (hw_read, hw_write), after the access, with the const view of the instance, so aux is the latch the access left and no device ever sees the log"
    - "the stop callback is the observation window of the matrix: it folds machine time, pc, A, X, Y, S, P, speed and the bus latch at every call, so a hook that disturbs any of them for even one instruction is seen at the next call, whatever the guest does with the value later"
    - "refusal order of the sinks: structs (alignment, clear of the call's memory and of each other), then capacities, then record ranges, so no caller field is read before its struct is known to be aligned and clear"

key-files:
  created:
    - tests/conformance/cases/observer_ds_proto.toml
    - tests/conformance/cases/observer_cyc_probe.toml
    - tests/conformance/cases/dev_log_records.toml
    - tests/conformance/cases/dev_log_windows.toml
    - tests/conformance/cases/trace_out_rights.toml
    - tests/controls/trace-hook-mutates/RULE
    - tests/controls/trace-hook-mutates/EXPECT
  modified:
    - src/core/ds_obs.h
    - src/core/obs.c
    - src/core/bus.c
    - src/core/run.c
    - src/core/run_refusals.def
    - src/core/ds_core.h
    - src/core/cpu.c
    - tests/conformance/obskinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/run_refuse_sink_unsupported.toml
    - tests/conformance/cases/run_refuse_memory_invalid.toml
    - scripts/conformance-run

key-decisions:
  - "DS_SINK_MAX_RECORDS is 1048576 records (24 MiB) for either sink. A capacity above it is run.sink_unsupported and is judged before the record range, so an oversized capacity with a NULL pointer is unsupported and exactly the maximum with a NULL pointer is memory_invalid (the boundary is a test variant)"
  - "The dev stamp is the start stamp of the instruction that made the access (machine time is not advanced inside a step), so one instruction can log several accesses with one stamp and seq orders them; the access stamp the timer plan adds to struct ds_bus (plan 03-21) may refine it. The pc field is the instruction's address"
  - "aux is io_buffer after the access (the bus latch, D-104, ASSUMED LED-CPU-0013). The window walk case reads it from the instance itself, not from the rule, so it keeps passing when plans 03-21 and 03-22 change what windows 3 and 5 do to the latch"
  - "Flags: bit 0 the access paid the VDC and VCE penalty (the offset in the page is below $800, D-107), bit 1 a physical access that did not consult the mapping registers (the ST group). The ST kind carries both; the plan's 'ST flag on the ST0 record' is the kind byte 3"
  - "The matrix compares everything per mode against the same mode without a sink (whole, chunk:7, stop:5000) and then across modes: the chunked end state equals the whole one, and the logs accumulated over the chunks equal the whole run's logs. A large sink that loses a record is harness_failure (the log is evidence there)"
  - "The chunked and stopped modes use the same two sinks; per-call headers are checked for consistency (count = min(seq, capacity), lost = seq - count, insn seq = steps of that call). Record memory past the count is checked untouched only in single-call modes, because a later chunk legitimately leaves earlier chunks' records behind"
  - "--trace-out takes a case id, not a path: the runner builds build/gate/traces/ID.insn and .dev itself, never creates the directory (no POSIX call in test code: scripts/conformance-run makes it, a hand run must mkdir first), runs the case's image to its until_mclk with insn capacity DS_SINK_MAX_RECORDS and dev capacity 65536, and prints only record counts, lost counts, sizes and sha256"
  - "trace_out_rights runs only the checks that come before any file is opened, so no gate case writes a trace by itself; the writing path is exercised by the explicit --trace-out step of conformance-run, whose printed sizes and hashes are checked with wc and the host sha tool"

requirements-advanced: [CORE-06]
requirements-completed: []  # plan 03-20's text closes CORE-06 at the end of W3; the observer kind still gets tim_timer, cpu_blk, the random set and pad_2btn (D-119)

duration: 120min
completed: 2026-11-11
---

# Phase 3 Plan 19: Device-register log, observer matrix, trace-hook-mutates toy and --trace-out rights Summary

**The core now has a second caller-owned output sink that logs every hardware-page write, every $1000-window read and every ST physical write; a 3 mode by 5 configuration matrix proves that carrying both sinks changes nothing about a run and a hook that mutates one register makes it go red; and the runner writes traces only for an explicit request on an authored or synthetic case.**

## What was built

- **Task 1 (tracer), commit 34506e7.**
  - Core: `struct ds_dev_sink` and `ds_obs_dev`; `struct ds_obs_log` now holds an insn and a dev `ds_obs_chan` (on, header, records, capacity, count, lost, seq) filled by `ds_obs_begin(log, insn, dev)` and closed by `ds_obs_end`; `chan_slot` is the one place a record is claimed or a loss counted
    (drop-newest, saturating lost, seq counts every offer). `ds_obs_sinks_check` replaces `ds_obs_insn_sink_valid`. `struct ds_bus` gained the `obs` pointer, set by `ds_cpu_step_obs` from its log argument; `bus.c` calls `log_dev` after the access from `hw_write` (windows 0 to 5, kind write, or ST from
    `ds_bus_write_phys`) and `hw_read` (window 4 only). `ds_run` calls the check, then `ds_obs_begin` when either sink is carried. The shared-state toy anchor line in run.c is unchanged.
  - Refusals: `run.sink_unsupported` now means a capacity above `DS_SINK_MAX_RECORDS`; `run_refuse_sink_unsupported` has 7 variants (insn alone, dev alone, each beside a valid other sink, both at 4294967295, an oversized capacity with a NULL pointer, and the boundary: exactly the maximum with a NULL
    pointer is `memory_invalid`). `run_refuse_memory_invalid` gained 9 variants for the dev sink and the two sinks against each other (dev records in the instance or over the request, the dev struct in the instance, misaligned, NULL with a nonzero capacity, dev struct over insn struct, dev records over
    insn records, dev records over the insn struct, the dev struct inside the insn records).
  - `kind observer` (cases `observer_ds_proto`, `observer_cyc_probe`): for each mode (whole, chunk:7, stop:5000) one reference run with no sink and one run per configuration (large, drop4, poison, cap0, cap1) with BOTH sinks, 259 assertions and 18 runs per case, 0.13 s each. Compared with the same
    mode's reference: stop reason, machine time, steps, overshoot, verdict RAM (the DSV1 block), digest and the stop sequence. Also checked: consistent headers, guard bytes and record memory past the count untouched, the records of a limited sink are the first records of the large sink, the chunked
    accumulated logs equal the whole logs, the stop-mode log is a prefix of the whole log.
  - `kind dev_log`: `dev_log_records` runs cyc-probe with both sinks and requires exactly two dev records in order, ST0 at $E182 (kind ST, phys $1FE000, value $22, flags penalty plus physical) and the VCE-window store at $E187 (kind W, phys $1FE400, value A taken from the insn record at that pc, flags
    penalty), each with the stamp of the insn record at its pc; `dev_log_windows` drives every window of the page through the bus choke point (writes of windows 0 to 5, the read of window 4, ST0 to ST2 and a dropped ST target; NULL bus, RAM, ROM and an unmapped bank log nothing) and checks the
    drop-newest arithmetic of a 4-record sink at the hook level.
- **Task 2, commit e37e9fd.** `tests/controls/trace-hook-mutates`: the anchor is `r = chan_slot(&log->insn, (size_t)DS_INSN_RECORD_BYTES);` in obs.c (exactly once), the replacement appends an increment of the A register through a cast away from const, in the generated toy only. `CONTROL
  trace-hook-mutates RED_OK` in the gate; a scratch toy built with replacement equal to the anchor gives `observer_ds_proto pass` (NOT_RED), the mutated toy gives `guest_failure` ("the large sink in mode whole changed the stop reason, the machine time, the steps or the overshoot").
  `mutation-control-anchor` covers the new RULE automatically (PASS).
- **Task 3, commit fc8ed84.** `obskinds_arg` (runner.c gains the call, an early return and a usage line) parses `--trace-out CASE_ID`; `trace_out_do` validates the id (`[a-z][a-z0-9_]*`, at most 64), finds the case, requires its fixture to be an authored image listed in
  `build/gate/fixtures-authored.used` or a synthetic recipe (`synth_size != 0`) and the case to carry `until_mclk`, builds `build/gate/traces/ID.insn` and `.dev` itself, runs and writes them. `kind trace_out_rights` (16 items, 26 assertions) covers permitted and refused fixture keys, the request for
  `ds_proto`, `run_refuse_memory_invalid` (fixture none), no such case, and path-shaped or upper-case ids with no file at any naive join. `scripts/conformance-run` got `trace_out_check`: no `build/gate/traces` after the cases ran, then `--trace-out ds_proto` must print exactly two lines whose sizes
  and sha256 match `wc -c` and the host sha tool (insn: 83447 records, 2002728 bytes, 24 bytes per record, none lost; dev: 0 records for ds-proto), then four refused requests (`../x`, `Ds_proto`, a fixture-none case, no such case) must exit 2 and add no file.

## Verification

- Task verifies: Task 1 `--stage build`, `core-boundary`, `conformance` PASS; Task 2 `--stage conformance` PASS (7 controls RED_OK) and `--selftest mutation-control-anchor` PASS; Task 3 `--stage conformance` and `--stage rights-tree` PASS (also `sanitizers` PASS after the fix below).
- Plan end: `sh scripts/check` PASS (about 86 s wall), `sh scripts/check --selftest` PASS (61 PASS lines, 15 s), `dash scripts/check` PASS, `sh scripts/audit-history --range ce6f5f3..HEAD` clean (3 commits, 26 blobs, 1285 blob-path pairs). `git status` shows nothing tracked under build/gate/traces (build/ is ignored).
- Probes outside the committed tree (scratch edits, files restored byte for byte and re-diffed): the 4-record insn drop test turned red with the count test `>` instead of `>=` (header not a consistent count); a dev hook that incremented an instance field turned `observer_cyc_probe` red only after the
  stop callback fold was widened to registers and latch (see Deviations 2); logging window 4 reads from the wrong window turned `dev_log_windows` red; dropping the ST physical flag turned `dev_log_records` red; `trace_fixture_permit` returning OK always and an id check that accepts `/` and `.` each
  turned `trace_out_rights` red.

## Deviations from Plan

**1. [Rule 3 - blocking] Files outside the plan's list.** `src/core/ds_core.h` (the dev pointer type in the request, the forward declarations, `obs` in `struct ds_bus`), `src/core/cpu.c` (the one-token change `{0u, log}` that gives the bus the log), `tests/conformance/stepkinds.c` (the
sink_unsupported variants and the new memory_invalid variants live there), `tests/conformance/cases/run_refuse_memory_invalid.toml` (its text counts the variants) and `scripts/conformance-run` (the explicit --trace-out step). The plan's 15-file waiver is exceeded by these five; each is the only
place its change can live.
**2. [Rule 2 - sensitivity] The stop sequence folds the registers and the bus latch, not only machine time and pc.** With machine time and pc alone, a dev hook that incremented `io_buffer` was NOT caught: cyc-probe's later timer-window read masks the latch, so the corruption was laundered before the
end state. The fold now includes A, X, Y, S, P, speed and io_buffer at every call of the stop callback; the same probe then turns `observer_cyc_probe` red. The matrix never weakened; a probe that passed showed it was weaker than the plan's claim, and it was strengthened.
**3. [Rule 2 - extra cases] Two cases beyond the plan's list.** `observer_cyc_probe` (ds-proto writes no hardware-page register, so its dev log is empty and the dev sink would never have a record to drop or to keep out of the verdict) and `dev_log_windows` (cyc-probe reaches only two of the logged
accesses, so the window rule and the aux byte of window-4 reads and writes had no witness). Both are REQUIRED and cost 0.13 s and 0.00 s.
**4. [Rule 1 - environment] `trace_out_rights` first failed under the sanitize run on a case-insensitive filesystem:** the naive join of the refused id `Ds_proto` is the same file as `ds_proto.insn` that the gate's explicit request had made. The kind now records whether a file was there before the
refused request and requires the refusal not to create one; in the gate run the directory does not exist when the kind runs (conformance-run removes it first and trace_out_check requires its absence), so the check is not vacuous there.
**5. Task commits were made from trimmed copies of five files.** `obskinds.c`, `runner.c`, `runner.h`, `REQUIRED` and `scripts/conformance-run` hold the Task 3 changes, which were written alongside Task 1; for Task 1 and Task 2 the trace pieces were removed from the working tree, verified,
committed, and restored for Task 3, so each commit builds and passes `--stage conformance` on its own. The Task 1 comment in obskinds.c mentions the toy that Task 2 adds.

No Rule 4 deviation. The shared-state toy control stayed RED_OK (the standing note was not triggered; its anchor line in run.c is unchanged).

## Evidence discipline

E0 only. The matrix compares the core with itself, so it proves non-interference and repeatability of the sinks, not what an instruction or a device access does. The dev_log_records literals are the fixture's immediate ($22, GUEST), the physical addresses of ST0 and the VCE window (H8-8 and H8-9
manual cites), the pcs from the ca65 listing (as in vdc_penalty_assumed), and the accumulator value read from the insn record of that pc, not DuoSpool output. The penalty flag follows ASSUMED:LED-CPU-0017 and the latch follows ASSUMED:LED-CPU-0013; no ledger status changed and none of the eight ER-2
candidates was touched. The trace rule is a literal rule of this plan. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim, no ROM or third-party material, nothing pushed (D-64).

## Known Stubs

None. The dev stamp being the instruction's start stamp is a documented simplification (key-decisions), not a stub.

## Threat Flags

None beyond the plan's register. T-03-51 (trace of non-authored content) is mitigated by the permit rule and the conformance-run step (a fixture-none case is refused); T-03-52 (path traversal) by the id grammar, runner-built paths and the naive-join checks; T-03-53 (observer changing behaviour) by
the matrix with poisoned header and tiny capacities and the liveness toy, which was measured red.

## Notes for next plans

- **Observer kind users (03-21 timer, 03-25 blocks, 03-26 random set, 03-29 pad):** add a case, not code: `kind = "observer"`, `fixture = "authored:NAME"`, `until_mclk`, `expect = "compare=verdict,machine_time,stops,digest"`, `sinks = "large,drop4,poison,cap0,cap1"`, `chunks =
  "whole,chunk:7,stop:5000"`, and list it in REQUIRED. `large` must be among the sinks; whole must be first among the chunks. A guest that runs more than 262144 instructions to its deadline needs `OB_INSN_LARGE` raised (a large sink that loses a record is harness_failure). The case also needs to
  appear in `tests/controls/trace-hook-mutates/EXPECT` only if it should be proven able to fail by that toy (the insn hook mutates A, so any case that executes instructions with a sink goes red).
- **Dev hook placement (03-21, 03-22, 03-27):** `hw_read` and `hw_write` call `log_dev` after the access; `log_dev` takes `offset` and the byte moved. When the timer and interrupt registers get real read side effects, log the value the read returned and keep the call at the exit of the function; the
  logged-window rule (writes of windows 0 to 5, reads of window 4) is D-119 and the table in `dev_log_windows`. `dl_windows` expects window 4 reads to be logged and windows 0 to 3 and 5 reads not; if plan 03-27 changes the I/O window read it keeps the same logging rule.
- **Access stamp (03-21):** `struct ds_bus` still has no stamp; the dev record uses `inst->machine_time` (the instruction's start). When 03-21 adds the access stamp to `struct ds_bus`, change `ds_obs_dev` to take it and update `dev_log_records` (its stamp check compares with the insn record's start stamp and would then need the penalty cycles added).
- **`struct ds_run_request.dev`** is now `struct ds_dev_sink *`; the insn and dev structs have the same 32-byte layout on 64-bit hosts (a `_Static_assert` in stepkinds.c relies on it).
- **--trace-out by hand:** `mkdir -p build/gate/traces` first; `build/gate/ds_runner --cases build/gate/cases.tsv --trace-out ds_proto`. The cases file and the fixtures list must exist (a gate run makes them). The files hold raw records without the header.
- **Timing:** `TIMING conformance 26` (TIMING_WARN, the threshold is 20; a record, not a failure). Measured here: the runner takes 3.4 s for the whole case list and each of the 7 toy runners about 3.3 s (the new toy is one of them), the four new cases cost about 0.3 s of that per runner. The stage
  time before this plan was not recorded at the start, so no delta is claimed. A later plan may let a toy run only the cases of its EXPECT (D-132 re-baseline).
- **Ledger provenance gap for plan 03-32** (standing note, LED-CPU-0015 disagreement column) is untouched here: this plan writes no ledger entry.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/ds_obs.h and obs.c rewritten, tests/conformance/obskinds.c, the five new case files, tests/controls/trace-hook-mutates (RULE, EXPECT). Commits 34506e7, e37e9fd and fc8ed84 are ancestors of HEAD.
