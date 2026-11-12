---
phase: 04-vce-vdc-raw-video
plan: 04
subsystem: tracer-vce-grid-records
tags: [tracer, vce, fold, vid-sink, video-kind, stop-rule-split, w2]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the vid sink and the fold call sites (04-02), the seven regions and the one bump (04-03), seeded-prng-v3 and the restore refusals (04-18)
provides:
  - vce.c: the VCE window (CR, CTA, CTW and CTR with the 9-bit palette as 1,024 little-endian bytes, unused reads 255, one static const divider table indexed by cr & 3)
  - vid.c: the const DSVR v1 header render (rec_bytes 64, active_w 0) with offsets asserted, and ds_vid_emit, the only caller of ds_obs_vid
  - beam.c: the EV_LINE grid fold (line k at raster_phase_mclk + k * 1365), divider latch, record emit behind the want_pixels gate, line_dirty cleared with or without a sink, ds_vdc_pending_time (the one pure helper of the pending event time)
  - bus.c window 1 folds then reads and writes the VCE; DS_VCE_STUB_READ removed
  - eight constants rows (raster_phase_mclk, line_sample_offset_mclk, vce_div_cr0..3, vce_unused_read, vce_data_hi_unused_or); both accept goldens and the record skeleton count (142 rows) written by hand from the def
  - runner kind video (tests/conformance/videokinds.c): DSV1 verdict, dev-log joins, STRUCT, div, first-change, line_dirty and border rows with hand-written record offsets, stream SHA-256 printed as E0 beside the literals
  - fixture vce_pal (first slice, 4 MAN checks, id 15), cases vce_pal (guest, three policies) and video_vce_pal (video, role literal), both REQUIRED
affects: [04-05, 04-06, 04-08, 04-09]

actuals:
  tokens: 23000
  tasks: 1
  commits: 1
plan_head_before: a72df99077ef5fa7ee2a91a72fd66d1e08100d75
plan_head_after: a76be7d8ab71a5232f6021ecf60b04b5deeff40e

tech-stack:
  added: []
  patterns:
    - "The fold's stored next event is derived by one pure helper (ds_vdc_pending_time) that returns validity, so the restore rule and the fold read the time from the same place and a hostile state cannot make it overflow"
    - "A video case joins the dev-log stamps of its writes to the records by the grid rule written in the runner (first record after a stamp = t / 1365 + 1); value rows are judged only on records a whole line away from the write that sets them"
    - "The render is a const function over the instance into a stack record; the emit is the single place beside the want_pixels gate (obs != NULL && obs->vid.on) that touches the sink"

key-files:
  created:
    - src/core/vce.c
    - src/core/vid.c
    - tests/conformance/videokinds.c
    - tests/conformance/cases/vce_pal.toml
    - tests/conformance/cases/video_vce_pal.toml
    - fixtures/src/vce_pal.s
    - fixtures/src/vce_pal.checks
    - fixtures/manifest/vce_pal.toml
  modified:
    - src/core/beam.c
    - src/core/bus.c
    - src/core/ds_core.h
    - src/core/ds_vdc.h
    - src/core/constants.def
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/obskinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/cases/vid_chan_append.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - CMakeLists.txt

key-decisions:
  - "line_idx is advanced at EV_LINE, so the events after it (EV_VD at stage 1, EV_RCR at stage 2) belong to line line_idx - 1; ds_vdc_pending_time encodes this and returns 0 (no event) for stage 1 or 2 at line_idx 0, a stage 1 without vd_pending or with vd_off none, a stage 2 with rcr_off none, or a time beyond 2^64"
  - "The fold loop bound is 3 * (span / 1365) + 6 events (three events per line plus the tail of the line before the span); the plan's (stamp - machine_time) / 1365 + 3 counted line events only"
  - "The role of a video case (literal) is an item of its expect, not a new case key, so scripts/conformance-flatten and the ingest are untouched"
  - "The video kind runs the ROM once under the request of the case (the default policy zero), not under the three-policy list: the early records legitimately differ by power-on palette, and the guest case vce_pal is the one that compares the three policies"

requirements-advanced: [VID-05, VID-06]
requirements-completed: []  # VID-05 and VID-06 complete in 04-16

duration: 200min
completed: 2026-11-12
---

# Phase 4 Plan 04: Tracer (Task 1 only; Tasks 2 and 3 are a new plan) Summary

**A ca65-built guest writes VCE CR and the 512 palette entries through the bus; the folded 1365-clock grid emits one header-only DSVR v1 record per line into the caller's vid sink; the runner kind video judges count, origin, frame and line, divider 4 then 3, line_dirty bits and border 273 by literal rows with hand-written offsets, and prints the stream SHA-256 as E0
  repeatability beside them. Stop rule fired after Task 1; Tasks 2 and 3 were not started.**

## Status and the split proposal (stop rule)

The plan's stop_rule says: after Task 1 is committed, if context is at or above about 60 percent, stop, write a partial SUMMARY naming the remaining tasks and report a split. The executor judged its context to be past that point (the reading list, the fold, the new runner kind and the cascade were large), committed Task 1 (a76be7d, tracer gate re-run end to end and
  green) and stopped. Tasks 2 and 3 are untouched.

**Proposed split:** a new plan with the next spare number (04-19), `depends_on: ["04-04"]`, wave 2/3 as the planner sees fit, holding Task 2 and Task 3 of 04-04-PLAN.md verbatim (their text, read_first, behavior, action and verify are unchanged and still correct, with the notes below). Its files are the Task 2 and Task 3 file lists of the plan. Everything Task 1
  leaves for them is listed under "Notes for next plans". 04-05 (tracer hardening) should depend on the new plan if it needs the toy or the sweep; it does not need the dev read logging.

## What was built (Task 1, commit a76be7d)

- **Core.** `vce.c` decodes `offset & 7` (CR write stores all eight bits and sets line_dirty bit 0; CTA low and high; CTW and CTR with the D-153 formulas; the colour table high-byte store is ONE statement; CTA steps mod 512 after the high byte only; every other read returns 255 and every other write is dropped). `vid.c` writes the 64-byte header through a table of
  asserted offsets, byte by byte, from a const instance. `beam.c`: `ds_vdc_advance` loops `ds_vdc_pending_time` while the next event is at or before the stamp (EV_LINE latches `lat_div` from the divider table, sets `vd_off = rcr_off = 0xFFFF`, renders and emits only when `obs->vid.on`, clears `line_dirty`, advances `line_idx`); stages 1 and 2 only retire what a
  restored state names (no VD flag, no RCR compare yet). `bus.c`: window 1 folds to the access stamp, then calls `ds_vce_read` or `ds_vce_write`; no `io_buffer` is touched. Eight constants rows with their ledger ids.
- **Runner.** `videokinds.c` (kind `video`, in `DS_RUNNER_SOURCES`): boots the built ROM with a dev sink (16,384 records) and a vid sink of (lines due + 1) * 2,792 bytes, runs with `rq.stop = dsv1_done`, judges DSV1 as the guest kind does, then walks the records with offsets written in the file (no core header is included). Rows, each in its own function: count =
  floor((M - 1) / 1365) + 1; origin k * 1365; frame and line; shape (rec_bytes formula, line_mclk 1365, version 1, zero reserved word); values below 512; div per CR write on records a whole line away from both stamps (MAN H6-3); the first change at the stamp of CR write 2 (ASSUMED LED-VID-0004); line_dirty bit 0 and bit 4 on exactly the first record after each write
  (ASSUMED LED-VID-0004); border 273 a whole line after the last palette write (GUEST). A lost record, a missing sink, an irq_stim key, a gap below the case's `gap`, a CR value other than the case lists and an empty judged window are harness_failure. The evidence text carries `video_sha256=HEX E0 repeatability`, the record and byte counts and the per-class row counts
  (man 2, guest 1, assumed 9).
- **Fixture and cases.** `vce_pal.s` (0BSD): MPR0 = $FF, CR $00, a counted delay of 768 CPU cycles (LDX 2, 127 passes of DEX 2 + taken BNE 4, a last pass of 4; 9,216 master clocks at the low speed, comment states the sum from the opcode table), CR $01, the 512 entries P(i) = (91 * i + 17) & 511 by the additive running sum through the H6-5 procedure, a readback of
  entries 0 and $100, and a tail delay of 18,432 clocks. Four MAN:H6-5 checks. Cases `vce_pal` (guest; the three default policies give identical check bytes) and `video_vce_pal`; manifest pinned (repeatability only).
- **Cascade.** Both accept goldens (28 constants rows, 30 withheld ids: LED-VID-0004, -0016, -0017 joined) and the record skeleton count (142 rows) were written by hand from the def, never from printer output; the digest, format version and epoch lines did not change. The `attached` part of `vid_chan_append` and its case text now expect the producer's records (count
  by the closed form, 64 bytes each, origin i * 1365) instead of an empty header.

## Verification

- Task 1 verify (`--stage fixture-lint`, `--stage core-boundary`, `--stage conformance`, the two RESULT greps and the `video_sha256=` grep): PASS, run twice (before the commit and on the committed tree: the tracer gate re-run end to end, nothing expanded on a broken base).
- Full `sh scripts/check`: 57 stages PASS before the commit. `sh scripts/audit-history --range a72df99..HEAD`: clean. `sh scripts/check --selftest`: see the closing report (run after the commit). **Linux gate-gcc:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`, linux/amd64, 53 s,
  one-off local run, not a CI claim). `dash scripts/check` and the second awk are owed at the Phase 4 checkpoint and close-out.
- Literals shown able to fail (ER-11), by temporary mutants of the live sources, each reverted: `vce_div_cr1` 3 to 4 turned `video_vce_pal` guest_failure (the div rows); the palette store mask `0x1FF` to `0xFF` turned `vce_pal` and `video_vce_pal` guest_failure (the round trip and the border row); a `line_dirty` that is never cleared turned `video_vce_pal`
  guest_failure (the dirty rows). The toy `vid-palette-mask` itself is Task 2.
- Evidence class E0 throughout: repeatability, with literal rows from MAN H6-3 and H6-5 (documentary, no ledger promotion), GUEST pal_bijection and ASSUMED LED-VID-0004 and LED-VID-0001 (the grid, the line length and the tie rule). No accuracy, hardware or compatibility claim.

## Deviations from Plan

1. **[Rule 3 - Blocking] `vid_chan_append` rewritten for the first producer.** Its `attached` part asserted an empty vid header after a run ("no producer yet"), which Task 1 makes false (04-02 noted this). The part now expects count = floor((M - 1) / 1365) + 1, lost 0, bytes = 64 * count, records back to back with origin i * 1365 and rec_bytes 64, poison untouched
  past them, and the unchanged run equivalence. Files beyond the plan's list: `tests/conformance/obskinds.c` and `tests/conformance/cases/vid_chan_append.toml`.
2. **[Note] `role=literal` is an expect item,** not a case key (adding a key means editing the flatten and ingest scripts, outside this plan). D-172's `golden` and `struct` roles can extend the same item.
3. **[Note] Tail delay in the fixture.** The border row needs a record a whole line after the last palette write; the guest would seal its block within a line of it, so the fixture ends with an 18,432-clock delay. Part of the first slice; Task 2 keeps it.
4. **[Note] First slice writes the palette with the additive running sum.** Task 2 replaces the writer by the shift-add multiply and adds the second computation as the check, as the plan's behavior list says; the manifest is re-pinned there.
5. **[Note] `until_mclk` 700,000 for both cases** (the guest seals at about 505,000 clocks; the plan gave no value). The video sink is sized from it (about 1.4 MB).
6. **[Note] Signatures chosen where the plan left them open:** `ds_vce_divider(cr)`, `ds_vdc_pending_time(&t, line_idx, stage, vd_pending, vd_off, rcr_off)`, `ds_vid_render(inst, origin, rec)` and `ds_vid_emit(obs, rec, n)` in `ds_vdc.h`. The D-166 helper the plan puts in Task 3 already exists (see below).

## Known Stubs

- `src/core/beam.c` `ds_vdc_advance`: the grid and EV_LINE are live; the vertical machine, the RCR counter, VD and the DMA closed forms are not (04-08 and later). `.planning/WINDOWS.md` id 4 stays open for that reason; it is resolved by 04-08.
- The VDC window (0) is still the stub of 04-02; `ds_vdc_flag`, `ds_vdc_read` and `ds_vdc_write` are declared and undefined (04-06). Records are header-only (active_w 0, border from entry $100, flags only the burst bit).

## Threat Flags

None. T-04-13 (palette and CTA arithmetic) is mitigated in code (explicit masks, CTA mod 512, the one-statement 9-bit store) and by the 512-entry GUEST check, which is Task 2. T-04-14 (fold loop) is mitigated by the loop bound; the restore window rule is Task 3. T-04-15 (record writer) holds by the whole-record append of 04-02 and the asserted header offsets. T-04-16
  holds: the digest is printed only beside literal rows, and the mutants above show the rows can fail.

## Notes for next plans (the Task 2 and Task 3 plan, and 04-05)

- **Task 2.** `vce_pal.checks` is id 15 with 4 checks; the case keys `checks = 4` in `vce_pal.toml` and `video_vce_pal.toml` and the manifest pin change with the fixture. The video case's `expect` items (crw, div, border, gap, frame_lines) stay valid; the fixture must keep CR $00, a delay of at least 5,460 clocks, CR $01 in that order and the tail delay.
  `ds_vce_write` keeps the palette high-byte store as one statement for the toy (`& 0x1FFu) >> 8u` is the unique text; the replacement is `& 0xFFu`), and the mutants above show the toy would be red on `vce_pal` and `video_vce_pal`. A guest case runs under zero, v3 seed 1 and v3 seed 2: read nothing that was not written first. Dev-log capacity of the video kind is
  16,384 records: Task 2's readback loops and Task 3's logged reads of windows 0 and 1 add records, so raise `VK_DEV_CAP` in videokinds.c if a `lost` appears (it is harness_failure, never silently weaker).
- **Task 3.** snapshot.c should call `ds_vdc_pending_time` (declared in `ds_vdc.h`; it returns 0 when the members name no event, so refuse that as `field` too) and judge `t >= machine_time && t - machine_time < 2730` with no addition that can overflow, in `dec_beam` AFTER the `lat_div` rule so `lat-div-0-late` (line_idx 5, lat_div 0) still proves its own rule; the
  state a fixture run reaches now has a live `line_idx`, so the existing `line-limit` accept row (line_idx = time / 1365 + 2) is only accepted while the machine time is not a multiple of 1365 (the run of 100,020 clocks is not). The `event-too-far` row needs the time edited to a multiple of 1365 (then line_idx = time / 1365 + 2 gives exactly time + 2730); add accept
  rows at the edges (pending = time, pending = time + 2729, `satb_done_at` = time and time + 4096, `dma_start` = time while running). `snapshot_refusal_timeline` has about 70 characters of room under the 511-character expect cap, so a new case (for example `snapshot_refusal_window`, REQUIRED) is likely needed. `satb_done_at` and `dma_start` are judged in `dec_vdma`
  against `v->time`. The hostile corpus ran clean with the live fold (accepted states run 3,000 clocks and restore again), but add a check that every accepted hostile input still runs defined after the new rules.
- **Dev read logging (Task 3).** `hw_read` for windows 0 and 1 returns early; the plan puts one `log_dev(..., DS_DEV_READ)` between the device read and the return, with the same stamp. The video kind scans only WRITE records, so it is unaffected. `mmu_mpr` checks 25 and 26 and the `cyc_probe` reads pass unchanged today (the VCE offsets they read return 255 or a
  palette byte the guest writes first); the plan's rewrite of check 25 and 26 is still required for the stale class text.
- **04-05 (tracer hardening)** extends the observer matrix and the sink on/off comparison to the video fixture; `kind_video` runs one policy only and takes its sinks from the case.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED (Task 1)

- FOUND: src/core/vce.c, src/core/vid.c, tests/conformance/videokinds.c, fixtures/src/vce_pal.s, fixtures/manifest/vce_pal.toml, tests/conformance/cases/vce_pal.toml, tests/conformance/cases/video_vce_pal.toml, REQUIRED lines vce_pal and video_vce_pal.
- FOUND commit a76be7d (an ancestor of HEAD).
