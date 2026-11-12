---
phase: 04-vce-vdc-raw-video
plan: 13
subsystem: checkpoint-a-cut-line
tags: [vid, checkpoint, cut-line, random-regs, poweron-policy, er-2, promotion, readme, d-176, linux-repro, w6]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the background renderer, scroll latch and enables (04-09 to 04-12), the video judge and fault sweeps (04-05, 04-07), the interrupt measurements (04-17), the vid sink, the v3 power-on policy and the restore rules (04-02, 04-18, 04-19)
provides:
  - tests/conformance/vdckinds.c - the kinds vdc_random_regs (32 seeded programs of random VDC, VCE, DMA-trigger and block-transfer traffic through the hardware page, bounded and range-checked under the gate and sanitizer presets) and vdc_poweron_policy_diff (five video fixtures equal under zero and two v3 seeds from their first judged frame, and the vce-pal-nopal control that
    must differ)
  - tests/conformance/randprog.c - rp_build maps the hardware page first and carries VDC and VCE accesses one slot in sixteen; rp_build_vdc builds the vdc_random_regs programs; the new case chunk_equiv_random32_video compares the raw stream of the random programs across chunk schedules
  - fixtures/manifest/vce_pal_nopal.toml and the NOPAL variant of fixtures/src/vce_pal.s - the control image whose palette is never initialised (the base vce_pal image is byte for byte unchanged)
  - CMakeLists.txt - lever (1) of D-176: the toys link shared object libraries (cold build of the 25 toys 47.1 s to 3.3 s user CPU)
  - ledger - LED-VID-0006, -0008, -0009, -0013, -0014, -0016 and -0017 promoted to supported (E1 documentary) under ER-2, each scoped by its statement to the page-stated part; the README supported list equals the ledger set
  - docs/ci-timings.md - the checkpoint A section: runs, stage table, the D-176 levers, the cut-line record, the Linux, dash, selftest and audit results and the efficiency review inputs
  - 04-VALIDATION.md - per-task rows for plans 04-01 to 04-12, 04-17 to 04-19 and 04-13, Wave 0 ticks, checkpoint A inputs
affects: [04-14, 04-15, 04-16]

requirements-completed: []
requirements-advanced: [VID-01, VID-03, VID-04, VID-05, VID-06]

actuals:
  tokens: 20900
  tasks: 3
  commits: 5
plan_head_before: 48699efda13283bf42d69a402556dddbae7211b2
plan_head_after: 676e4a41e4b87c17265a10492563d41d57b6c6e8

tech-stack:
  added: []
  patterns:
    - "A policy guard shown able to fail: the same fixture without its palette initialisation gives a border that differs between the zero policy and each v3 seed, so a guard that compared equal on every input would not pass the control"
    - "The judged window of the policy case is the frames after the last record that differs between policies, asserted to be exactly the declared first judged frame, so a later window cannot hide a difference and an earlier one is a difference"
    - "Toy executables link shared object libraries of the runner and of each core unit and compile only the generated unit"

key-files:
  created:
    - fixtures/manifest/vce_pal_nopal.toml
    - tests/conformance/cases/vdc_random_regs.toml
    - tests/conformance/cases/vdc_poweron_policy_diff.toml
    - tests/conformance/cases/chunk_equiv_random32_video.toml
  modified:
    - tests/conformance/vdckinds.c
    - tests/conformance/randprog.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - fixtures/src/vce_pal.s
    - scripts/fixture-build
    - CMakeLists.txt
    - ledger/entries/LED-VID-0006.toml
    - ledger/entries/LED-VID-0008.toml
    - ledger/entries/LED-VID-0009.toml
    - ledger/entries/LED-VID-0013.toml
    - ledger/entries/LED-VID-0014.toml
    - ledger/entries/LED-VID-0016.toml
    - ledger/entries/LED-VID-0017.toml
    - README.md
    - docs/ci-timings.md
    - .planning/phases/04-vce-vdc-raw-video/04-VALIDATION.md

key-decisions:
  - "The first judged frame of each fixture in vdc_poweron_policy_diff is the first frame after the last record that differs between policies (vce-pal 5, vce-dot 1, bg-basic, bg-scroll and bg-enab 2); the differences are the border of the setup frames, before the palette entry the border reads is written. The case asserts that equality, so the calibration cannot drift either way"
  - "The random differentials map the hardware page at the first instruction of bank 0 (MPR0 = $FF) and plant the video accesses as instructions; the stream comparison is a separate case (chunk_equiv_random32_video) because chunk_equiv refuses a restore combined with video"
  - "Lever (1) of D-176 is applied (trigger fired: conformance over 60 s); levers (2) to (4) are recorded and not applied: they remove runs and none brings the stage under 60 s alone"
  - "ER-2 promotions are scoped by the statement to the part a pinned MAN row exercises: the converse readings (a flag only when enabled, BSY, DS, OR, CR) and the modelling clauses moved to assumption text; LED-VID-0011 and every other entry stay assumed"
  - "The cut is recommended (the quiet-gate trigger fired); nyquist_compliant is left false here and is flipped by whichever plan closes the phase"

completed: 2026-11-12
---

# Phase 4 Plan 13: Checkpoint A, the cut line Summary

**Random VDC and VCE register programs survive the gate and sanitizer presets, the power-on policy guard is shown able to fail by a never-initialised palette image, seven page-scoped entries are promoted to supported under ER-2, and the cut-line triggers are on record: the gate read 262 to 374 s of user CPU against 150 s (fired, load-dependent), no revision plan was used and
the CI job time is unverified.**

## What was built

- **Task 1 (commit 2872067).** `vce_pal.s` wraps only the palette-initialisation block in `.ifndef NOPAL` (the base image sha256 is unchanged), `scripts/fixture-build` has one variants line `vce_pal vce-pal-nopal 32 -D NOPAL` and `fixtures/manifest/vce_pal_nopal.toml` pins it (a control image, no DSV1 judgement). `rp_build` now begins bank 0 with `LDA #$FF; TAM #$01` and, one
  instruction slot in sixteen, emits a VDC register write (AR, low, high), a VCE window write or a VDC status or data read, all through the hardware page; `rp_build_vdc` builds the programs of the
  new kind `vdc_random_regs`: 32 programs from a fixed seed, each a stream of random-register VDC writes (one in four from the registers that start a transfer or change the geometry), VCE window writes, status reads, 64-byte TIA block transfers into the VDC data port or the VCE window and counted waits, run to 1,200,000 master clocks with a vid sink of the closed-form
  capacity. Each program must stop at the deadline (reason until, within 16,384 clocks), lose no record, give the count floor((M - 1) / 1365) + 1 and hold only whole records with border, indices and colours below 512; the digest of every program differs from the one before (a vacuity guard). The kind `vdc_poweron_policy_diff` runs vce-pal, vce-dot, bg-basic, bg-scroll and
  bg-enab under zero and v3 seeds 1 and 2 and requires equal verdict RAM and
  byte-equal records from the first judged frame; the window must hold at least one frame and its start must equal the frame after the last differing record. For vce-pal-nopal the border of some record in the window must differ between zero and each seed (observed: 4,396 of 4,396 records for both seeds). The new case `chunk_equiv_random32_video` compares the stream bytes of
  the 32 random programs, which now write the video devices, across the schedules 1364, 1366, 3072 and one frame.
- **Lever commit (d831680).** `CMakeLists.txt`: the runner, shim and text sources and each core source are compiled once into object libraries; a toy links them and compiles only its generated unit. Cold build of the 25 toys: 8.4 s wall and 47.1 s user CPU before, 0.8 s and 3.3 s after; all 25 controls RED_OK with the identical `controls.txt` listing.
- **Task 3 (commit a9f690d).** ER-2 promotions (below), README, and the 04-VALIDATION.md per-task map (41 rows), Wave 0 ticks and checkpoint A inputs.
- **Task 2 (commits 8f2ac96 and 676e4a4).** The checkpoint A section of `docs/ci-timings.md`.

### ER-2 promotion pass

All seven covered E1 candidates were promoted; LED-VID-0011 (sprites) and every other entry stay assumed. For each, the page or pages named below were re-read as page images in this plan (verifier gsd-executor, 2026-11-12) from a scratch copy of the HuC6270 and HuC6260 manuals (sha256 as recorded in `fixtures/lib/TOOLS.md`, never committed): H7-3, H7-4, H7-5, H7-6, H7-7,
H7-11, H7-12, H7-25, H6-2, H6-3 and H6-5; the literals are on the pages; the statement reads "equals the manual's stated values" for the page-true part; withheld lists hardware accuracy, timing and compatibility; the pin is a passing TEST that names the entry. What each statement covers and what was left outside it:

| Entry | Pin | The E1 statement covers | Left in assumption text |
|-------|-----|-------------------------|-------------------------|
| LED-VID-0006 | TEST:vdc_reg | AR bits 4-0, VWR and VRR high-byte triggers, IW steps, MARR start, status high byte 00h | write latch, port decode, field widths, reserved reads, increment moment |
| LED-VID-0008 | TEST:vdc_stat | RR bit 2, DV bit 4, VD bit 5, their enables (CR bits 2 and 3, DCR bit 1), clear on read | the converse (no flag when not enabled), BSY reads 0, CR, OR and DS positions (no MAN row raises them) |
| LED-VID-0009 | TEST:video_bg_scroll | the next-line statements for BXR and BYR (H7-7) | R1 line-start reading, SCREEN and CM validity, first display line of a BYR write, direction, wrap |
| LED-VID-0013 | TEST:vdc_satb_data | 256 words, high-byte trigger of DVSSR, start at the following vertical blanking | duration, DS instant, DSR repeat |
| LED-VID-0014 | TEST:vdc_dma | high-byte trigger of LENR, LENR + 1 words, DCR SI/D and DI/D, DCR bit 1 enable of DV, no end status when cancelled | 4 dots per word, bus wait, overlap, IW, retrigger, overrun cut |
| LED-VID-0016 | TEST:vce_pal | 512 x 9-bit table, 9-bit CTA, low then high byte writes and reads, CTA steps after the high byte | window decode and mirrors, unlisted reads, CTR high bits, wrap, block-transfer alternation |
| LED-VID-0017 | TEST:video_vce_dot | content 00h gives ratio 4 and 01h ratio 3, nothing else (F-23) | the master clock identified with 6fsc, bits 1:0 alone |

LED-VID-0013 note: the pin vdc_satb_data compares the 256 copied words; its trigger and start rows are the MAN H7-12 rows of the sibling case vdc_satb, which is not the entry's pin (INV_PIN_BACKLINK allows one pin case). Both cases are listed in the entry's evidence field. Moving the pin to vdc_satb would touch two case files outside this plan; the close-out can do it if the
owner wants the pin to be the case that holds the trigger rows.

## Cut-line triggers (D-180), for the orchestrator

| Trigger | Measure | Result |
|---------|---------|--------|
| more than two plans needed a revision plan | 0 revision plans (04-20 and 04-21 unused; 04-17, 04-18 and 04-19 are planned and stop-rule splits, which do not count) | not fired |
| warm quiet gate over 150 s | user CPU of one `sh scripts/check --timing` run: 262.4 s (run D, load 7, quietest), 301.3 s (run A, load 21 to 15), 374.4 s (run B, load 11 to 14); wall 321 s, 397 s and 470 s; the 04-05 spike had 178.6 s | fired (load-dependent, not a quiet-machine figure) |
| CI job over 600 s with lever (1) not recovering it | no push authorised (D-64) | unmeasured |
| owner prefers an earlier pull request | for the orchestrator | unmeasured here |

**Recommendation: take the cut.** If it is taken: 04-14 to 04-16 become 4.1-01 to 4.1-03; the README and 04-VALIDATION.md text of this plan are the Phase 4 close-out for the background scope; the orchestrator sets `nyquist_compliant: true` in 04-VALIDATION.md, runs the standard mark-complete for VID-01, VID-03, VID-04 and VID-05 after moving the sprite plans (VID-06 stays open
for the sprite rows), and adds to the README Status paragraph that sprites move to Phase 4.1 (plan-check info 11). LED-VID-0008 is already worded to what is exercised (RR, DV and VD, their enables and the clear on read; OR and CR are raised by no event yet).

## Verification (verification-report inputs)

- **Gate:** `sh scripts/check` PASS (57 stages) at every commit and at the end (final run D: 321 s wall, 262 s user CPU, load 7; conformance 62 s, sanitizers 146 s, determinism 52 s). `dash scripts/check` PASS twice (389 s and 479 s wall). `sh scripts/check --selftest` PASS (145 PASS, no FAIL) at the first commit and at the end. `sh scripts/audit-history --range 925662a..HEAD`
  clean (62 commits, 617 blobs, at the last commit; 59 commits at the first).
- **Linux (one-off local container runs, linux/amd64 emulated, not a CI claim), all six parts PASS:** `default-awk` (mawk 1.3.4, gcc 13: gate 57 stages in 1,489 s and selftest 145), `mawk` (mawk, clang 18: 960 s and selftest), `gawk` (gawk 5.2.1, gcc 13: 805 s and selftest), `gate-gcc` (-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror), `gate-clang`,
  `core-boundary-gcc`; 4,572 s in all, on the tree with every C and CMake change of this plan.
- **Controls shown able to fail (ER-11):** the nopal image (border differs in all 4,396 records under both seeds, so vdc_poweron_policy_diff would fail on a guard that passed everything); all 25 toy controls RED_OK before and after the CMake change (identical listing); the vacuity guards of the two kinds (a digest equal to the previous program's, a window under one frame, a
  window start that is not the frame after the last difference).
- **Automated vs handed off:** everything above was automated here. Handed off: a push and a CI run for the job time (D-64, owner authority), and the owner's preference on an earlier pull request. Nothing else needs a human.
- **Counts:** 315 REQUIRED ids (3 added here) and 315 RESULT lines per run. Video judge: 12 scenes in 5 fixtures with .vexp files (10 literal role, 2 golden role), 47 groups, 98 rows by class MAN 20, GUEST 5, ASSUMED 73 and by form ROWS 30, REL 3, STRUCT 65 (the video evidence files print the same split); toy controls 25; VIDEO_TOY_EXEMPT 6; hash roles: 10 literal scenes carry
  `video_sha256 ... E0 repeatability` beside their literal verdict and 2 golden scenes are change detectors only.
- **Cost of the new cases (gate preset, `--only`):** vdc_poweron_policy_diff 1.1 s user, vdc_random_regs 0.3 s, chunk_equiv_random32_video 1.5 s; chunk_equiv_random32 3.0 s and observer_random 4.1 s with the new video traffic in the random programs.
- **Evidence class:** E0 throughout for the new cases (no oracle; the core shown survivable, bounded and policy-independent); E1 documentary for the seven promoted entries only. No accuracy, compatibility or hardware claim anywhere.

## Requirement honesty table

| Requirement | State at this checkpoint | Cases that show it (E0 repeatability; E1 documentary only for the promoted entries) |
|-------------|--------------------------|-------------------------------------------------------------------------------------|
| VID-01 background | complete for the background scope (only the close-out tick remains) | video_bg_basic_solid, video_bg_basic_asym, video_bg_scroll, video_bg_scroll_asym, video_bg_enab, their sweeps, toys vid-bit-order and vid-scroll-latch, chunk_equiv_video_bg_scroll, snapshot_restore_video_bg |
| VID-02 sprites | not started (plans 04-14 and 04-15) | none; LED-VID-0011 stays assumed |
| VID-03 VRAM, SATB and VRAM-VRAM data movement | complete | vdc_reg, vdc_vram, vdc_vram_sweep, vdc_regmask, vdc_satb, vdc_satb_data, vdc_dma, vdc_dma_vram, vdc_advance_pure, vdc_snapshot_mid_dma, vdc_random_regs |
| VID-04 status and interrupts (background scope) | complete for RR, DV, VD, DS and the IRQ1 path; OR and CR are raised by no event until sprites | vdc_stat, vdc_irq, vdc_irq_ratio, vdc_irq_agree and its variants, toys vid-rcr-base and status-read-keeps-flags |
| VID-05 VCE and geometry | complete | vce_pal, video_vce_pal, vce_dot, video_vce_dot, vid_frame_lines, vid_active_window, vdc_fold, vdc_poweron_policy_diff, toys vid-palette-mask and vid-dot-div |
| VID-06 raw video record and evidence | complete only at close-out: its sprite rows remain | the video judge and sweeps, video_hash_not_verdict, vid_noninterference, the observer and chunk cases, snapshot_restore_video, video_out_rights |

No requirement checkbox was ticked by this plan: the sibling plans 04-14 to 04-16 still list every VID id, so the ready-ids gate holds them.

ROADMAP criterion 5 is shown live by toys for the palette, divider, background (bit order) and scroll (latch) paths, while some scenes are live only by record faults of the fault sweeps (the BXR rows, the 64 x 64 wrap, the enable matrix: toy cap of D-173). An owner reading criterion 5 literally may want one more renderer mutant (plan-check info 9).

## Deviations from Plan

1. **[Rule 3 - Blocking, by the plan's own rule] `CMakeLists.txt` changed** (not in files_modified): lever (1) of D-176, whose trigger fired (`conformance` over 60 s), in its own commit d831680 as Task 2 requires. No evidence weakened (25 controls RED_OK, identical listing).
2. **[Note] A new case `chunk_equiv_random32_video`** (and its REQUIRED line) is not in the file list: it is how the random differential carries the raw stream of the programs that now write the VDC and VCE, because `chunk_equiv` refuses a restore combined with video, so the existing `chunk_equiv_random32` could not take the `video` item.
3. **[Note] Task 3 was committed before Task 2.** The six Linux parts include the second-awk whole-gate runs, which judge the new README and ledger texts, so they had to run on the tree with the promotions; Task 2's section records their results and was written after them. The task verifies were each run as written on the tree of their commit.
4. **[Note] The Task 2 runs were made at different tree states**, all with identical code: `sh scripts/check --timing` runs A, B and D, dash at the first and third commits, selftest at the first and last commits (and three times inside the container), audit-history at the first and last commits. Runs A and B are the F-8 measure's loaded samples; run D, the last, is the
   quietest (load 7). No quiet machine was available and none was waited for.
5. **[Note] `rp_build` changed for every random program** (the hardware page mapped first and video accesses one slot in sixteen): `chunk_equiv_random32` and `observer_random` still pass with their existing equality checks, and neither has a golden.
6. **[Note] LED-VID-0013 is promoted although its pin (vdc_satb_data) is not the case that holds the trigger and start rows** (vdc_satb); see the promotion section. LED-VID-0008 keeps the clear-on-read and the three raised flags only; the plan text named DS as exercised, but its rows are ASSUMED LED-VID-0013, so DS stays outside the E1 sentence.
7. **[Note] Wave 0 ticks:** nine of ten items are ticked; the last (includes) is unticked because `spr.inc` is created by plan 04-14.
8. **[Note] Plan estimate was 90,000 tokens; the realized diff is about 20,900** by chars/4 over the added lines (22 files, 935 insertions, 85 deletions). The stop rule did not fire.

## Known Stubs

None. No fixture register, table or data in this plan is a placeholder. The NOPAL image is a control, not a stub: its palette block is left out on purpose.

## Threat Flags

None. T-04-44 (random register programs): 32 programs under ASan and UBSan with the termination, loss and range checks; T-04-45 (power-on seeds leaking into judged output): the case over five fixtures and the nopal control that must differ; T-04-46 (claim inflation): ER-2 all-conditions rule with the pages re-read as images, statements scoped to the exercised page-stated part,
the README list equal to the ledger set and the D-181 wording only.

## Notes for next plans

- **A new video fixture with a `.vexp` file must be added to the `roms=` list of `vdc_poweron_policy_diff`** (with its first judged frame: run the case once, read the failure message, which prints the last differing frame; the case asserts the window start equals that frame plus one). There is no inventory check yet that every `.vexp` fixture is listed; the sprite fixtures of
  04-14 and 04-15 (or 4.1) need the line.
- **The random programs now write VDC and VCE registers:** a core change that makes the random differentials fail is a real finding, not a test fault. `rp_build_vdc` has its own constants at the top of `randprog.c`; the kind caps are 256 programs and the 64 MiB sink.
- **`CMakeLists.txt` toys:** adding a core source adds one object library (`ds_toy_core_NAME`) automatically; a toy rule still names exactly one core file.
- **Cost:** conformance reads 62 to 84 s (trigger 60 s) with `ds_runner` alone about 51 s; the remaining D-176 levers (2) to (4) are recorded and cost evidence runs; the efficiency review after the first CI run should decide them with the CI job time in hand.
- **If the cut is taken,** the orchestrator inserts Phase 4.1, flips `nyquist_compliant`, runs the mark-complete for VID-01, VID-03, VID-04 and VID-05 and extends the README Status paragraph (see the cut-line section).
- `.planning/state.json` was left untouched and uncommitted.

## Close-out note on requirement ticks

The standard plan-complete step (`requirements.mark-complete VID-01 VID-03 VID-04 VID-05 VID-06`) ticked all five checkboxes and traceability rows. This plan completes none of them: 04-14 to 04-16 still list every VID id and have no SUMMARY, so the ticks were reverted with a file-specific `git checkout` of .planning/REQUIREMENTS.md after checking that nothing else in that file
was modified (the diff was the ten tick lines only); REQUIREMENTS.md equals HEAD. If the cut is taken, the orchestrator runs the mark-complete for VID-01, VID-03, VID-04 and VID-05 after moving the sprite plans; otherwise the normal step of 04-16 ticks them.

## Self-Check: PASSED

- FOUND: fixtures/manifest/vce_pal_nopal.toml, tests/conformance/cases/vdc_random_regs.toml, vdc_poweron_policy_diff.toml and chunk_equiv_random32_video.toml, tests/conformance/vdckinds.c, CMakeLists.txt, docs/ci-timings.md, the seven promoted ledger entries (status supported), the REQUIRED lines of the three new ids.
- FOUND commits 2872067, d831680, a9f690d, 8f2ac96 and 676e4a4 (ancestors of HEAD).
