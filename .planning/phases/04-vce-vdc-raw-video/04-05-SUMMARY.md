---
phase: 04-vce-vdc-raw-video
plan: 05
subsystem: tracer-hardening-fold-sink-restore-video-out
tags: [tracer, chunk-equiv, vdc-fold, observer, vid-noninterference, snapshot-restore, toy-control, video-out, cost-spike, w2]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the folded 1365-clock grid, the vid sink and the video kind (04-04), vce_pal complete, dev read logging and the fold-window restore rules (04-19), seeded-prng-v3 (04-18)
provides:
  - chunk_equiv with the video schedules 1364, 1366 and vframe (357,630 = 262 * 1365) and a compare item video that concatenates the vid records of every call and compares the streams with memcmp (REQUIRED case chunk_equiv_video_vce_pal)
  - tests/conformance/vdckinds.c: kinds vdc_fold (64 seeded partitions equal one jump; the tie rule through the bus write path; the next-event invariant at every stop), vid_noninterference (four vid-only sink shapes against no sink under three policies) and snapshot_restore_video (restore at the stops of three schedules, stream compared)
  - the observer matrix on the video fixture (observer_vce_pal) under three power-on policies through a new optional policies key, with a closed-form seq check on every observer case
  - --video-out ID (authored or synthetic fixture of a video case only) writing build/gate/video/ID.dsvr and ID.tsv, the kind video_out_rights and the conformance-run step that recomputes the digest with a host tool
  - toys vid-emit-mutates and fold-skipped-at-step-end (both RED_OK), LED-VID-0004 pinned by vdc_fold, and the Phase 4 measurement spike in docs/ci-timings.md
affects: [04-06, 04-08, 04-10, 04-13, 04-16]

requirements-advanced: [VID-06]
requirements-completed: []  # VID-06 completes in 04-16

actuals:
  tokens: 29000
  tasks: 3
  commits: 3
plan_head_before: 9a3abf9a9e2fbb805df3c55ba6fc24fb933ac3de
plan_head_after: be8cab9af7dc8c0c3babdc02608fcc203ccdae49

tech-stack:
  added: []
  patterns:
    - "A stream accumulator beside the sink: a per-call record memory reused for every call, whose header is read by hand and whose stored bytes are appended to one string, so whole and chunked runs are compared with memcmp and never with hashes"
    - "Direct fold drives on a booted instance: ds_obs_begin over a vid-only log, ds_vdc_advance for the stamps of a partition, snap_first_diff against the one-jump instance; the tie case goes through ds_bus_write with a bus whose cycle_mclk is 0, so the stamp of the access is the stamp of the case and the fold-then-write order is the production one"
    - "Restore without a boot per restore: the target of the next restore is the instance the chain left one restore ago, poisoned again, so a restore costs the snapshot and the compare only"
    - "A file-output option cloned from --trace-out (id charset, authored-and-listed or synthetic fixture, directory made by the harness, stdout only ids and hashes) plus a host-tool recomputation of the digest against the evidence file"

key-files:
  created:
    - tests/conformance/vdckinds.c
    - tests/conformance/cases/chunk_equiv_video_vce_pal.toml
    - tests/conformance/cases/vdc_fold.toml
    - tests/conformance/cases/observer_vce_pal.toml
    - tests/conformance/cases/vid_noninterference.toml
    - tests/conformance/cases/snapshot_restore_video.toml
    - tests/conformance/cases/video_out_rights.toml
    - tests/controls/vid-emit-mutates/RULE
    - tests/controls/vid-emit-mutates/EXPECT
    - tests/controls/fold-skipped-at-step-end/RULE
    - tests/controls/fold-skipped-at-step-end/EXPECT
  modified:
    - tests/conformance/chunkkinds.c
    - tests/conformance/obskinds.c
    - tests/conformance/videokinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - CMakeLists.txt
    - scripts/conformance-run
    - ledger/entries/LED-VID-0004.toml
    - docs/ci-timings.md
    - .planning/phases/04-vce-vdc-raw-video/04-VALIDATION.md

key-decisions:
  - "snapshot_restore_video runs the first 600,000 clocks of vce_pal (about 440 lines) and restores at every stop of the 1364 and 1366 schedules; the schedule 7 is restored at every 251st stop and at every stop whose machine time is a multiple of 1365 (75 such stops over the three policies, a case
    with none is a guest_failure). Over the whole 4,000,000 clocks the first version was 20,352 restores and 83 s"
  - "The closed-form seq check of the observer applies to every observer case, not only to the video one: the fold runs for every program, so the large vid sink of any matrix must have been offered floor((M - 1) / 1365) + 1 records; all existing observer cases still pass"
  - "The policies key (request fragments separated by ||, as in a guest case) is accepted by the observer kind; an absent key keeps the single request of the case, so no earlier observer case changed"
  - "video_vce_pal is not listed in the EXPECT of fold-skipped-at-step-end: under the toy it is harness_failure (every literal row passes, the border window is empty because the closing fold is skipped), so the A5 exception stands and the five guest_failure ids are recorded in 04-VALIDATION.md"
  - "--video-out accepts only a case of kind video (a guest case such as vce_pal is refused as not a video case) beside the fixture rule; the id charset is [a-z0-9_]+ as the plan says (trace ids need a leading letter)"

completed: 2026-11-12
---

# Phase 4 Plan 05: Tracer hardening Summary

**The first video slice is now shown chunk-invariant, restore-invariant and independent of the vid sink: five chunk schedules, 64 random fold partitions, restores at hundreds of stops (ties included) and the three-policy observer matrix all agree with one whole run in stream bytes and digest, two
more toys are red on their cases, `--video-out` writes the canonical stream for an authored fixture only and the gate recomputes its SHA-256 with a host tool, and the cost of the gate is on record against D-176.**

## What was built

- **Task 1, tracer (commit ef70330).** `chunkkinds.c` gains the schedules 1364, 1366 and vframe (357,630 = 262 * 1365; a 263-line frame is 358,995 by binding, written in the file header) beside the untouched historical frame, and the compare item `video`: a vid sink on every call of the whole run
  and of every schedule, the records of each call appended to one accumulator, the two streams compared with `memcmp`. A lost record, a header that disagrees with the records stored or a run that stored none is harness_failure, and the whole run is also held to the closed form floor((M - 1) / 1365)
  + 1, so an empty stream compared with an empty stream cannot pass. Case `chunk_equiv_video_vce_pal`: chunks 7, 1364, 1366, 3072, vframe over 4,000,000 clocks, REQUIRED. The tracer gate was re-run end to end on the committed tree before Task 2 (nothing was expanded on a broken base).
- **Task 2 (commit 33bb17e).** New `vdckinds.c`:
  - `vdc_fold`: 64 seeded partitions of [0, 3 * 357,630] (odd ones cut at origin - 1, origin and origin + 1 of random lines) fold through direct `ds_vdc_advance` calls and equal one jump in every restorable member (`snap_first_diff`, the beam bytes among them), in the digest and in the records; the
    tie part writes the VCE control register through `ds_bus_write` at origin - 1, origin and origin + 1 of line 5 and judges which line carries divider 3 and line_dirty bit 0; the invariant part runs the fixture under a stop callback (one call, 64 random deadlines, with and without a sink) and
    requires at every call a valid pending event time not older than the machine time and a next line index equal to the closed form.
  - `vid_noninterference`: per policy (zero, v3 seed 1, v3 seed 2), no sink against a vid-only sink in four shapes (large, four records, capacity 0, capacity 1): machine time, steps, verdict RAM and digest equal; the header counts whole records, lost = seq - count, seq = the closed form, guard bytes
    and the bytes past the count untouched, stored records equal the first records of the large run.
  - `snapshot_restore_video`: restore at the stops of the schedules 1364, 1366 and 7 (see key decisions); the chain's machine time, steps, verdict RAM, digest and concatenated stream equal the uninterrupted run under three policies.
  - `obskinds.c`: the large vid sink of every observer mode must have been offered the closed-form line count; the optional `policies` key runs the matrix once per request fragment. Case `observer_vce_pal` (sinks large, drop4, poison, cap0, cap1; modes whole, chunk:7, stop:5000; three policies).
    `runner.c`/`runner.h`/`CMakeLists.txt` register the kinds and the source file; four cases and REQUIRED lines.
- **Task 3 (commit be8cab9).**
  - Toys: `vid-emit-mutates` (file `vid.c`; the border store of `ds_vid_render` followed by a cast-away-const increment of the low byte of palette entry $100, so state changes only when a sink is attached; EXPECT video_vce_pal, observer_vce_pal, vid_noninterference) and `fold-skipped-at-step-end`
    (file `cpu.c`; the step-end `ds_vdc_advance` replaced by `(void)start;`; EXPECT chunk_equiv_video_vce_pal, vdc_fold, observer_vce_pal, vid_noninterference, snapshot_restore_video, with a comment line recording why no literal-role case is listed). Both are RED_OK.
  - `--video-out ID`: `videokinds_arg`, `video_out_main`, `kind_video_out_rights` (allow and deny lists cloned from trace_out_rights, plus a guest case as not-a-video-case) in `videokinds.c`; `output_fixture_permitted` exported from `obskinds.c`; the stream text form `.tsv` has one row per record
    after a header row, pixels as run-length groups. `conformance-run` gains `video_out_check` (the directory must be absent after the cases, one explicit request for video_vce_pal, two summary lines, sizes and hashes measured by a host tool, rows = records, the host SHA-256 of the `.dsvr` equal to
    the `video_sha256` of the evidence file, five refused requests add no file) and a `video-out-check BD` mode.
  - LED-VID-0004 `pinning_test = "TEST:vdc_fold"` and the case pin (status stays assumed). `docs/ci-timings.md` has the Phase 4 spike.

## Verification

- Task 1 verify, Task 2 verify (conformance, the four RESULT greps, `--stage sanitizers`), Task 3 verify (conformance, both CONTROL greps, `RESULT video_out_rights pass`, `--stage ci-timings-controls`, `--selftest mutation-control-anchor`): PASS. Full `sh scripts/check` PASS (56 stages) before Task
  1 and Task 3 commits and on the final tree; the Task 2 commit followed a full run whose only failure (a ledger pin placed too early, see deviations) was fixed before it. `sh scripts/check --selftest`: PASS. `sh scripts/audit-history --range 9a3abf9..HEAD`: clean (3 commits).
- **Linux gate-gcc:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, strict C11, linux/amd64, 50 s). **Wave W2 end:** `LINUX_REPRO_PARTS="gate-gcc gate-clang core-boundary-gcc"` PASS (81 s). One-off local runs, not CI claims. `dash scripts/check` and the second awk are owed at the Phase 4 checkpoint and close-out.
- Literals shown able to fail (ER-11): `vid-emit-mutates` is RED_OK on its three cases and `fold-skipped-at-step-end` RED_OK on its five. Temporary mutants, each reverted byte for byte: the tie test `t > stamp` to `t >= stamp` in `beam.c` turned `vdc_fold` and `video_vce_pal` guest_failure
  (chunk_equiv and the observer stayed pass, as expected: the tie rule is a literal, not an invariance); a restore that sets `line_idx` one low for a seventh of the states turned `snapshot_restore_video` guest_failure.
- The stream hash printed by the case is checked outside the core: `shasum -a 256` of `build/gate/video/video_vce_pal.dsvr` equals the `video_sha256` of its evidence file, and the step is shown able to fail (a doctored evidence digest gives `VIDEO_OUT_DIGEST`, a stale directory gives `VIDEO_OUT_UNREQUESTED`).
- Evidence class E0 throughout (repeatability, composability, non-interference). The literal rows are the divider values (MAN H6-3) and the grid, tie rule and closed-form line count (ASSUMED LED-VID-0004 and LED-VID-0001, written in the kinds and sharing no code with the core). No accuracy, hardware or compatibility claim.

## Deviations from Plan

1. **[Rule 3 - Blocking] `snapshot_restore_video` lives in `vdckinds.c`, not `snapkinds.c`, and covers the first 600,000 clocks.** The plan's text restores at every stop of the schedules 1364, 1366 and 7. Measured: a restore costs about 0.7 ms, schedule 7 alone has about 100,000 stops, and the first
   version (4,000,000 clocks, restore at every grid stop and every 97th stop of schedule 7, a boot per restore) took 83 s. Now: no boot per restore (the spare instance is swapped), 600,000 clocks, every stop of 1364 and 1366, every 251st stop and every multiple-of-1365 stop of schedule 7 (2.2 s;
   2,826 restores over three policies, 75 at a line-event tie). The kind shares the stream and policy helpers of `vdckinds.c`; `snapkinds.c` is unchanged.
2. **[Rule 3 - Blocking] The LED-VID-0004 pin moved from Task 2's case to Task 3.** The inventory stage refuses a case pin whose ledger entry does not name the case, so the pin (case `pins` and `pinning_test`) is added together in Task 3, as the plan lists it, and `vdc_fold` carried `pins = []` in the Task 2 commit.
3. **[Note] The anchor of `vid-emit-mutates` is the border store of `ds_vid_render`, not "the emit call statement".** `ds_vid_emit` receives no instance, so no cast-away-const write is possible there; `ds_vid_render` is the code that runs only when a sink is attached (the want_pixels gate), which is
   the property the plan asks for. The effect is as described: the next record's border and the digest change only with a sink.
4. **[Note] `fold-skipped-at-step-end` replaces the fold by `(void)start;`, not an empty statement,** because `start` would be an unused variable under `-Werror`.
5. **[Note] `video_vce_pal` under the fold toy is harness_failure** (all 43 assertions pass, the border window is empty), so it is not listed and the A5 exception row of 04-VALIDATION.md records that and the five observed guest_failure ids.
6. **[Note] Files beyond the plan list:** none outside it except `tests/conformance/cases/observer_vce_pal.toml` use of the new `policies` key (the plan files list names the case). `snapkinds.c` is in the plan list and was not changed (deviation 1). `obskinds.c` is also touched in Task 3 (one
   exported function). `scripts/conformance-run` gained the `video-out-check` mode, used to show the step can fail.
7. **[Note] Duplicated stream accumulator.** `chunkkinds.c` (Task 1) and `vdckinds.c` (Task 2) each hold a small accumulator of the same shape; merging them would reopen the committed tracer and was left for a later cleanup.
8. **[Note] Plan estimate was 95,000 tokens; the realized diff is about 29,000** by chars/4 over the added lines (22 files, 2,062 insertions, 31 deletions).

## Known Stubs

None added. The VDC window read is still the stub of 04-02 (status $00 until 04-06) and `ds_vdc_flag`, `ds_vdc_read` and `ds_vdc_write` are still undefined; records are still header-only. `.planning/WINDOWS.md` id 4 stays open for the vertical machine until 04-08.

## Threat Flags

None. T-04-17 (`--video-out` of a commercial image) is mitigated and judged by `video_out_rights` (fixture ghost, none, a path, a commercial name, a guest case, no such case, bad ids, no file left) and by the harness step (five refused requests add no file). T-04-18 (sink state leaking into machine
state) is mitigated by the observer matrix under three policies, `vid_noninterference` and the live toy `vid-emit-mutates`. T-04-19 (digest recomputed only by the core's hash) is mitigated by the host-tool recomputation in `video_out_check`.

## Notes for next plans

- **Cost.** The spike (`docs/ci-timings.md`, Phase 4 section): one loaded run 224 s wall, 178.6 s user CPU (Phase 3 run A 129 s), `conformance` 45 s (staging trigger 60 s), `sanitizers` 88 s. By the F-8 measure the cut-line trigger (user CPU over 150 s) is over; the decision is 04-11's. Cases of
  this plan cost about 3.6 s plain and 10 s sanitized. Every plan from here adds ROM, sweep and toy cases to the same stages: apply the D-176 levers (a shared toy archive; two policies for video fixtures) before `conformance` crosses 60 s.
- **Observer closed form.** Every observer case now requires that the large vid sink was offered floor((M - 1) / 1365) + 1 records. A later plan that changes the grid or the record count per line (for example a second record per line) must change `ob_lines_due` in `obskinds.c`, `cs_lines_due` in
  `chunkkinds.c`, `vd_lines_due` in `vdckinds.c` and `vk_count_closed_form` in `videokinds.c` on purpose.
- **Video kinds to reuse.** `vd_policy_request` and `vd_compose_request` give the three default policies (declared in `runner.h`); `vd_stream_*` helpers in `vdckinds.c` (a per-call sink and an accumulator) are the shape for any later kind that needs the stream of a chunked or restored run;
  `kind_video`'s stream builder is `vk_stream`; `vo_do` reruns a video case with its own stop callback, so a later video case kind that is judged by something other than `kind_video` needs a branch in `vo_request`.
- **Restore window.** `snapshot_restore_video` only restores states the 04-19 window rules accept; 04-08 and 04-10 must keep live states inside them (see the 04-19 notes) or this case turns guest_failure with a refused snapshot. Its first 600,000 clocks hold the two CR writes and the start of the
  palette writer; a later plan that adds VDC writes to the same fixture, or a new fixture with VDC activity, should add its own `snapshot_restore_video` row over the interesting interval.
- **`--video-out` text form.** The `.tsv` pixels column is empty until 04-09 gives lines an active window; the columns are frame, line, origin, div, vce_cr, border, active_w, line_dirty, flags, pixels. Offsets are hand-written in `videokinds.c` (ER-13).
- **Toys.** The 04-07 `INV_TOY_NO_LITERAL` rule applies to toys of `vce.c`, `vdc.c`, `beam.c` and `vid.c`; `vid-emit-mutates` (vid.c) lists `video_vce_pal`, which is its literal-role case; `fold-skipped-at-step-end` (cpu.c) is exempt by amendment A5.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: tests/conformance/vdckinds.c, tests/conformance/cases/chunk_equiv_video_vce_pal.toml, tests/conformance/cases/vdc_fold.toml, tests/conformance/cases/observer_vce_pal.toml, tests/conformance/cases/vid_noninterference.toml, tests/conformance/cases/snapshot_restore_video.toml,
  tests/conformance/cases/video_out_rights.toml, tests/controls/vid-emit-mutates/RULE, tests/controls/fold-skipped-at-step-end/RULE, REQUIRED lines for all six cases.
- FOUND commits ef70330, 33bb17e and be8cab9 (ancestors of HEAD).
