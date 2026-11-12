---
phase: 04-vce-vdc-raw-video
plan: 19
subsystem: vce-palette-fixture-read-logging-restore-window
tags: [vce, fixture, mutate-sweep, toy-control, dev-log, snapshot-window, stop-rule-split, w2]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the VCE window, the fold and the video kind (04-04), seeded-prng-v3 and the restore refusals (04-18)
provides:
  - fixture vce_pal complete, 32 checks (about 28 planned) of classes MAN H6-3 and H6-5, GUEST pal-bijection and ASSUMED LED-VID-0016, identical under zero and two v3 seeds
  - fixtures/lib/vce.inc, the first per-feature include (macros and equates only, never edited later) and the optional RODATA segment of hucard.cfg after CHECKLIT (existing image bytes unchanged)
  - mutate_sweep_vce_pal (REQUIRED, 32 literals flipped) and the toy vid-palette-mask (RED_OK on vce_pal and video_vce_pal)
  - LED-VID-0016 pinned by vce_pal and LED-VID-0018 by video_vce_pal (both mutual, statuses stay assumed)
  - reads of windows 0 and 1 logged to the dev sink (kind READ, value returned, flag PENALTY, the stamp the fold used, io_buffer untouched) with the whole cascade regenerated from rules
  - mmu_mpr checks 25 (empty window reads the unmapped constant) and 26 (VCE colour table readback, MAN H6-5) rewritten, three manifests re-pinned
  - restore window rules of D-166 in snapshot.c (next event, satb_done_at, dma_start) and the REQUIRED case snapshot_refusal_window with 16 rows
affects: [04-05, 04-06, 04-08, 04-10]

requirements-advanced: [VID-05, VID-06]
requirements-completed: []  # VID-05 and VID-06 complete in 04-16

actuals:
  tokens: 18000
  tasks: 2
  commits: 2
plan_head_before: d08d44054031e3ece90cddf8f33105e2e8639049
plan_head_after: 07ddf2e6b2255d9a3914e0c508da98685be2d0d1

tech-stack:
  added: []
  patterns:
    - "A fixture include holds macros and equates only, so including it emits no byte; the macros VCE_CTA, VCE_LO, VCE_HI, VCE_WRITE_P91 and VCE_READ_P91 are expanded where used and the 21 other images kept their hashes"
    - "A GUEST row for a whole table: the guest ORs every difference between a read byte and an independently computed expected byte (an additive running sum against the multiplicative writer) and the check compares that OR with an expected byte of zero"
    - "A dev_log item cycle may end in H for an instruction run at the high speed (3 master clocks per cycle)"

key-files:
  created:
    - fixtures/lib/vce.inc
    - tests/conformance/cases/mutate_sweep_vce_pal.toml
    - tests/conformance/cases/snapshot_refusal_window.toml
    - tests/controls/vid-palette-mask/RULE
    - tests/controls/vid-palette-mask/EXPECT
  modified:
    - fixtures/src/vce_pal.s
    - fixtures/src/vce_pal.checks
    - fixtures/lib/hucard.cfg
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - src/core/bus.c
    - src/core/ds_obs.h
    - src/core/snapshot.c
    - tests/conformance/obskinds.c
    - tests/conformance/snapkinds.c
    - tests/conformance/videokinds.c
    - tests/conformance/reviewkinds.c

key-decisions:
  - "The window rule of dec_beam is judged after the lat_div rule with ds_vdc_pending_time: pending < machine_time or pending - machine_time >= 2730 or no event named is refused as field; the difference is taken only after the order is known, so nothing overflows"
  - "The window labels went to a new REQUIRED case snapshot_refusal_window (the 04-04 notes allowed it); snapshot_refusal_vdc and snapshot_refusal_timeline were not touched"
  - "OB_DEV_LARGE 4096 to 16384: the 32 random programs offer up to 10,980 dev records in a large run (measured once with a counter that was removed); the no-loss rule is unchanged"
  - "The video kind decodes the VCE writes of the dev log by offset & 7 inside the window (MAN H6-2), so writes through the mirror $0444 and $0445 count as palette data writes"

completed: 2026-11-12
---

# Phase 4 Plan 19: vce_pal complete, dev read logging and the fold-window restore rules Summary

**vce_pal now writes all 512 colour table entries by shift-add, reads them back against an additive sum and runs 32 register tests under three power-on policies; a sweep and the toy vid-palette-mask show its literals can fail; reads of the VDC and VCE windows are logged once for the phase; and a restore refuses a next event, SATB end or
  DMA start outside the fold window before any commit.**

## What was built

- **Task 1 (commit 5ca0bd0).** `vce_pal.s` keeps CR $00, a counted delay of 9,216 master clocks, CR $01 and the tail delay of the first slice. Between CR $01 and the tail it reads the unlisted addresses (checks 1 to 7, ASSUMED LED-VID-0016), writes entry i = (91 * i + 17) & 511 by shift and add (VCE_WRITE_P91), reads entries 0 and $100 (checks 8 to 11, MAN H6-5), compares all 512
  entries with the additive sum E(0) = 17, E(i + 1) = (E(i) + 91) & 511 (checks 12 and 13, GUEST pal-bijection, so an 8-bit address alias is caught), and runs the step-after-the-high-byte tests (14 to 17, MAN H6-3), the unused CTR bits (18, 19), independent halves of the data (20 to 23) and of the address (24, 25), the wrap (26, 27), the mirrors $0444 to $040D
  (28, 29) and a TIA block write of four entries (30 to 32), all ASSUMED LED-VID-0016. Entry $100 is never touched after the writer, so the border row of the video case still sees 273. `vce.inc` holds equates and macros only; `hucard.cfg` gains `RODATA` after `CHECKLIT` (the TIA source table is its first user). The toy `vid-palette-mask`
  replaces `& 0x1FFu) >> 8u);` by `& 0xFFu) >> 8u);` in `vce.c`.
- **Task 2 (commit 07ddf2e).** `bus.c` logs a READ for windows 0 and 1 with the stamp of the fold, between the device read and the return; the prose of `ds_obs.h`, `bus.c` and `obskinds.c` is rewritten. `dev_log_records` lists six records (the two old ones plus the reads at $E184, $E18A, $E197 and $E19E with values $00, $FF, $00, $00 from
  the rules); `dev_log_windows` expects the reads of windows 0, 1 and 4.
  `snapshot.c` gains the three window rules and `snapkinds.c` 16 rows (the refused edges before-time, time + 2730, no event, a VD event behind, SATB end before time, + 4097 and max, DMA start ahead; the accepted edges time, time + 2729, a VD event ahead, SATB end at time and + 4096, an idle SATB end far ahead, DMA start at time, an idle
  DMA start far ahead). Each accepted window row also runs 6,000 clocks and snapshots again.

## Verification

- Task 1 verify (`--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, the sweep and toy greps, `--selftest mutation-control-anchor`): PASS. Task 2 verify (`--stage conformance`, the three RESULT greps, `--stage fixtures`, `--stage sanitizers`): PASS. Full `sh scripts/check` PASS (57 stages) on both commits.
- `sh scripts/check --selftest`: PASS. `sh scripts/audit-history --range d08d440..HEAD`: clean (2 commits). **Linux gate-gcc:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, strict C11, linux/amd64, 52 s, one-off local run, not a CI claim). `dash scripts/check` and the second awk are owed at the Phase 4 checkpoint and close-out.
- Literals shown able to fail (ER-11): the toy `vid-palette-mask` is RED_OK, so `vce_pal` and `video_vce_pal` are guest_failure under an 8-bit palette store; the sweep flips all 32 literals of vce_pal one at a time and each flip turns exactly its own check red; a temporary mutant of `snapshot.c` with the three window rules neutralised
  turned `snapshot_refusal_window` guest_failure (restored byte for byte).
- Evidence class E0 throughout: repeatability, with rows from MAN H6-3 and H6-5 (documentary, no ledger promotion), GUEST pal-bijection and ASSUMED LED-VID-0016, LED-VID-0001, LED-VID-0004, LED-CPU-0012 and LED-CPU-0017. No accuracy, hardware or compatibility claim.

## Deviations from Plan

1. **[Rule 3 - Blocking] `videokinds.c` decodes the dev-log writes by `offset & 7` in the VCE window** (Task 1, commit 5ca0bd0). The new fixture writes an entry through the mirrors $0444 and $0445; the core sets line_dirty bit 4 for them, and the old scan by exact physical address would have called those records unexplained. The file is in the plan's Task 2 list, not Task 1's.
2. **[Rule 3 - Blocking] `dl_parse_item` accepts a cycle suffix H** (`obskinds.c`). The sixth cyc-probe record is a read after CSH (3 master clocks per cycle) and the kind judged every stamp with 12; the item `0xE19E:R:0x1FE000:0x00:5H` selects DL_MCLK_HIGH. No existing item changes.
3. **[Rule 1 - Bug] `dev_record_pc` expected 6 records and now sees 9.** The TII of that case runs with the stack mapped to the hardware page, so the three pops of the epilogue are reads of window 0, which D-149 logs now. The expectation is derived from the instruction definition (3 pushes + 3 byte writes + 3 pops), not from the run.
  Files beyond the plan list: `reviewkinds.c`, `dev_record_pc.toml`.
4. **[Rule 1 - Bug] The `time-2-63-1` accept row of snapkinds now also sets line_idx** to ceil(time / 1365). Its purpose (the largest machine time a restore accepts, T-03-84) is kept; with only the time edited the beam was 2^63 clocks behind and the new D-166 rule correctly refused it.
5. **[Rule 3 - Blocking] `until_mclk` 700,000 to 4,000,000** in vce_pal, video_vce_pal and the sweep: the shift-add writer and the readback make the guest run to about 3,500,000 master clocks.
6. **[Note] 32 checks instead of about 28; GUEST tag spelled `pal-bijection`** (the lint requires hyphens). `mmu_mpr_256k.toml` meaning text was updated for the new check classes (beyond the plan list).
7. **[Note] Files in the plan list that were not changed:** `beam.c` (the pure helper `ds_vdc_pending_time` already existed from 04-04), `buskinds.c` (st_bypass only writes), `snapshot_refusal_vdc.toml` and `snapshot_refusal_timeline.toml` (the new rows are in `snapshot_refusal_window`, as the 04-04 notes allowed).
8. **[Note] Plan estimate was 70,000 tokens; the realized diff is about 18,000** by chars/4 (652 lines added, 133 removed over 30 files).

## Known Stubs

None added. The VDC window read is still the stub of 04-02 (status $00 until 04-06); the dev_log_records literals for the VDC reads are that status, ASSUMED LED-VID-0006.

## Threat Flags

None. T-04-13 (palette and CTA arithmetic) is mitigated and now proven live by the toy and the 512-entry GUEST rows; T-04-14 (fold loop) is mitigated by the window rules judged in pass 1 without an overflowing addition.

## Notes for next plans

- `vce.inc` is frozen. Later features create their own includes (vdc.inc in 04-06, vdma.inc, spin.inc, bg.inc, spr.inc). `RODATA` is available and the TIA source table of vce_pal uses 8 bytes of it.
- Reads of windows 0 and 1 are in the dev log: any fixture with many VDC reads (spd_speed, the 04-06 readback loops) adds records. The video kind scans only WRITE records and its VK_DEV_CAP of 16,384 held for vce_pal; the guest kind of 04-06 should watch `lost`.
- 04-06: the six dev_log_records literals read status $00 at the cyc-probe pcs; once the VDC status is real they stay $00 only while no flag is raised, so re-derive them from the rule, never from a run. The $E19E read happens after CSH, so its cycle item ends in H.
- 04-08 and 04-10 must keep live states inside the restore window: after every step the fold has run to the last clock, so the next event is at most one line ahead (stage 1 and 2 events belong to line_idx - 1); a SATB transfer must end within 4,096 clocks of machine_time and a running DMA must have started at or before it, or the new rules refuse a snapshot of a legal run.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: fixtures/lib/vce.inc, tests/controls/vid-palette-mask/RULE, tests/conformance/cases/mutate_sweep_vce_pal.toml, tests/conformance/cases/snapshot_refusal_window.toml, REQUIRED lines mutate_sweep_vce_pal and snapshot_refusal_window.
- FOUND commits 5ca0bd0 and 07ddf2e (ancestors of HEAD).
