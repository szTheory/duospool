---
phase: 04-vce-vdc-raw-video
scope: gate, fixtures and evidence (scripts, tests, fixtures, ledger, docs/ci-timings.md, README.md, CMakeLists.txt)
reviewed_range: 925662a..HEAD (HEAD 29edb6e)
reviewed: 2026-11-12T00:00:00Z
depth: deep
status: issues_found
severity:
  blocker: 0
  high: 4
  medium: 6
  low: 6
  info: 4
  total: 20
findings:
  critical: 0
  warning: 10
  info: 10
  total: 20
files_reviewed_list:
  - scripts/video-check
  - scripts/lib/vexp-lex.awk
  - scripts/lib/vexp-rules.awk
  - scripts/conformance-run
  - scripts/check
  - scripts/fixture-build
  - scripts/docs-check
  - CMakeLists.txt
  - tests/conformance/videokinds.c
  - tests/conformance/chunkkinds.c
  - tests/conformance/VIDEO_TOY_EXEMPT
  - tests/conformance/cases (video, fault sweep, vdc_irq_agree, poweron, digest_stream cases)
  - tests/controls (the ten Phase 4 toys)
  - tests/video (all mini-roots)
  - fixtures/src (bg_basic, bg_enab, bg_scroll, vce_dot, vce_pal .vexp; vdc_irq, vdc_dma, vdc_satb, vce_pal .s; .checks of vdc_stat, vdc_dma, vdc_satb, vce_pal, vdc_reg, vdc_irq)
  - fixtures/lib (spin.inc, vdma.inc, cites.txt)
  - ledger/entries (LED-VID-0006, 0008, 0009, 0013, 0014, 0015, 0016, 0017)
  - docs/ci-timings.md
  - README.md
---

# Phase 4: Gate, Fixture and Evidence Review (adversarial, deep)

**Reviewed range:** 925662a..HEAD. **Depth:** deep. **Status:** issues_found (0 blocker, 4 high, 6 medium, 6 low, 4 info).

## Summary

The video judge, fault sweep, golden role and stage-1/stage-2 checker are fail-closed on the paths they cover: sink loss, dev-log overflow, empty windows,
margin violations and missing sinks all become harness_failure, and `sh scripts/check --stage video-check` plus the four video selftests pass. The weak
points are in what the evidence is said to prove. Two toys go red only because the guest DSV1 checks fail, so the video literal groups they are cited as
proving live are never evaluated. One MAN row is self-declared ASSUMED and backs a supported entry; one supported entry claims a timing clause no MAN row or
pin exercises. Several enforcement rules of `vexp-rules.awk` (stage-1 limits, format rules, the ER-10 grid-class rule) can be deleted with every control
and selftest still green, which is the CR-01 lesson in a new place.

Experiments (all in a scratch directory outside the repository, nothing in the repo modified): scratch mini-roots run through `scripts/video-check`; a
scratch copy of `scripts/` with single rule lines deleted; the toy evidence files under `build/gate/controls/`; a scratch harness that boots plain-256k
and dumps the snapshot stream to recompute the digest literals; shasum over `build/gate/video/video_vce_pal.dsvr`; manual page H7-4 re-read as an image.

## Structural Findings (fallow)

None provided.

## Narrative Findings (AI reviewer)

### HIGH

#### H-01: Toy kills of video cases are red for the wrong reason; GUEST border groups are never shown live by a toy

- **Files:** `tests/conformance/videokinds.c:2363-2368`, `scripts/conformance-run:540` (toy_verdict), `tests/controls/vid-palette-mask/EXPECT`, `tests/controls/vid-emit-mutates/EXPECT`, `fixtures/src/vce_pal.vexp:22-23`, `fixtures/src/vce_dot.vexp:14`, `tests/conformance/VIDEO_TOY_EXEMPT:6,8`
- **Issue:** `vk_capture` judges the DSV1 block first and returns at `if (c->harness || c->timeout || c->failed) return -1;`, so a video case whose guest checks fail never evaluates a single video group. `toy_verdict` accepts any guest_failure.
- **Observed:** under vid-palette-mask, `video_vce_pal` shows `assertions=32 got=pass=24 fail=8` with no `video_group` line (the real core gives 38 assertions and six group lines); `video_bg_basic_solid` shows `assertions=6` (its DSV1 count). Under vid-emit-mutates, `video_vce_pal` is `assertions=32 fail=2`, again DSV1 only.
- **Consequence:** the claims "its liveness against a changed palette store comes from the toy vid-palette-mask (it turns video_vce_pal guest_failure on this group)" (vce_pal.vexp, A7) and the vce_dot.vexp and VIDEO_TOY_EXEMPT reasons that lean on it are false: the GUEST border groups of vce_pal, vce_dot and bg_basic have F0 as their only shown kill.
- INV_TOY_NO_LITERAL is satisfied for vid-emit-mutates only by `video_vce_pal`, a literal-role case that is red without any literal group running. Same pattern under vid-rcr-base: `vdc_irq_agree`, `_263` and `_hdw` read `assertions=1 got=guest not a pass under zero` (videokinds.c:4059-4064), so their model-versus-literal comparisons never run under the toy.
- **Failure scenario:** a renderer bug that reads the border from the wrong palette entry while the guest readback still passes would not be caught by any toy, although the docs say a toy covers it.
- **Fix:** let `kind_video` evaluate and print every group even when DSV1 fails (outcome still guest_failure).
- **Fix (cont.):** give EXPECT an optional reason column (for example `video_vce_pal group=border`) that `toy_verdict` checks against the evidence file; or pick a toy that leaves the guest readback intact (mutate the border lookup in vid.c, not the palette store). Correct the A5/A7 texts and the exempt reasons.

#### H-02: vdc_dma check 24 is tagged MAN while its own text says ASSUMED, and it backs a supported E1 clause

- **Files:** `fixtures/src/vdc_dma.s:49`, `fixtures/src/vdc_dma.checks:31`, `ledger/entries/LED-VID-0014.toml:9`, `README.md:15-22`
- **Issue:** row 24 `overrun-no-dv MAN:H7-4 ... (the class of the row is that of the cut, ASSUMED LED-VID-0014)`. Whether the 4096-word transfer is still running at the display start depends on the ASSUMED speed of 4 dots per word (VL1); under a faster speed the copy ends first, DV is set and the literal $00 fails. By ER-10 the row is ASSUMED.
- LED-VID-0014 (supported, listed in the README) states "a transfer cancelled by a display start sets no end status (H7-4)" and cites "a cancelled transfer sets no DV" among its MAN rows. That clause rests on an ASSUMED-class row, and the per-class counts in the evidence file count it as MAN.
- **Fix:** tag row 24 `ASSUMED:LED-VID-0014` (its text already says so) and either drop the cancel clause from the E1 statement into the assumption text or add a row whose premise does not depend on the speed (for example LENR = $FFFF started at the last VBlank line, which no plausible speed finishes before the display start, with that argument written in the row).

#### H-03: LED-VID-0013 claims "starts at the beginning of the following vertical blanking period"; no MAN row or pin exercises it

- **Files:** `ledger/entries/LED-VID-0013.toml:9`, `fixtures/src/vdc_satb.s:26-28,69-84`, `fixtures/src/vdc_satb.checks` rows 1-3, `tests/conformance/cases/vdc_satb_data.toml`
- **Issue:** the MAN rows only show "status $00 9,180 clocks after the DVSSR high-byte write" (row 2) and "VD alone 9,180 clocks after a VBlank following a low-byte write" (row 1). The row that sees the copy (row 3, $28) is ASSUMED. The pin `vdc_satb_data` compares only the final 256 SATB words.
- **Failure scenario:** a model that copies 10 lines after the write (mid-frame) or in the middle of VBlank passes rows 1-3 and `vdc_satb_data`. Rows 1 and 2 also discriminate only because the ASSUMED DS delay (1024 dots x divider = 4,096 clocks) is under the 9,180-clock wait, so by ER-10 they are ASSUMED as well.
- The 04-13 SUMMARY records the pin mismatch as a deviation, but the promotion went ahead, so a README-listed supported statement is stronger than its evidence (ER-2).
- **Fix:** narrow the E1 statement to "256 words, triggered by the DVSSR high byte" (the parts `vdc_satb_data` and a MAN row really show), move "starts at the following vertical blanking" into the assumption text, or add a record-level check that joins the copy instant to the VBlank entry line; re-tag rows 1 and 2 ASSUMED.

#### H-04: Several vexp enforcement rules have no mutation control; the ER-10 grid rule shares a tag and its removal is invisible

- **Files:** `scripts/lib/vexp-rules.awk:28` (claim "one tag for each other rule"), `:160-168` (rule-video-exempt-format, rule-stage1-limits), `:180-256` (rule-format, rule-stage1-row), `:308-309` (grid-class VEXP_CLASS tagged rule-vexp-class-lint), `scripts/check:2647,2676` (selftests), `tests/video/`
- **Issue:** `tests/video` has no mini-root for VEXP_FORMAT, VEXP_NO_SPDX, VEXP_LINE_TOO_LONG, VEXP_NON_ASCII, VEXP_TOO_MANY_ROWS, VEXP_FILE_TOO_LARGE, VIDEO_EXEMPT_FORMAT, or the grid-class rule (class-bad hits only bad cite, bad ledger id, bad GUEST model).
- **Demonstrated (scratch copy):** deleting every line tagged rule-format, rule-stage1-limits, rule-video-exempt-format and rule-stage1-row leaves all ten tests/video controls at EXPECT_OK, and a .vexp with a 401-character line then passes (`STATUS video-check ok`) where the real checker reports VEXP_LINE_TOO_LONG.
- **Demonstrated:** deleting only the grid-class line 309 leaves class-bad at EXPECT_OK and lets `check=count` under `MAN:H6-3` pass. The selftest deletes all lines tagged rule-vexp-class-lint at once, so it cannot see this. ASCII, SPDX and the 16 KiB limit exist only in stage 1 (the runtime re-checks only 400 characters and 512 rows).
- **Fix:** give each code its own tag and a negative mini-root (line-too-long, non-ascii, no-spdx, too-many-rows, file-too-large, format, exempt-format, grid-class), and add them to the z7c selftest list. Correct the line-28 claim.

### MEDIUM

#### M-01: MAN tags on rows that assert the converse "no flag without the enable", which LED-VID-0008 says is not on the page

- **Files:** `fixtures/src/vdc_stat.checks:9,11,24` (rows 1, 3, 16), `fixtures/src/vdc_dma.checks:18` (row 11), `ledger/entries/LED-VID-0008.toml:16`
- **Issue:** H7-4, re-read as an image, says only "If an interrupt enabled by ... occurs, the corresponding bit ... is set". The LED-VID-0008 assumption text agrees ("the converse is the reading of three emulators, outside the E1 statement"), but these four rows assert the converse under `MAN:H7-4`. The E1 statement itself is not inflated; the per-class counts are (ER-4, ER-10).
- **Fix:** re-tag the four rows `ASSUMED:LED-VID-0008` (or a COMM-backed id) and regenerate the class counts.

#### M-02: MAN-class bg rows depend on the ASSUMED row-counter start (display line K shows virtual row K)

- **Files:** `fixtures/src/bg_basic.vexp:10,36-39`, `fixtures/src/bg_enab.vexp:34-37`, versus `fixtures/src/bg_scroll.vexp:43-46` and `ledger/entries/LED-VID-0009.toml:16`
- **Issue:** the `solid` (MAN:H7-18) and `bb_on` (MAN:H7-6) groups assert `line=active+7` shows BAT row 0. If the first display line showed BYR + 1, active+7 would show virtual row 8 (BAT row 1: the asymmetric tiles in bg_basic, all index 15 in bg_enab) and the rows would fail.
- bg_scroll.vexp and LED-VID-0009 call that same counter start an ASSUMED reading (F-12), so by ER-10 these groups are ASSUMED or need rows that do not depend on it.
- **Fix:** keep the MAN groups to display lines 1-6 of BAT row 0, which hold under both readings, and move the active+7 rows into an ASSUMED:LED-VID-0009 group; or re-class the groups.

#### M-03: The class lint binds labels, not content; frame-numbered and write-joined checks pass under MAN and GUEST

- **Files:** `scripts/lib/vexp-rules.awk:101,128,268-270,308-309`
- **Issue:** `gridck` omits `flat_frame` (frame numbering: grid and frame_lines) and `flat_at` (the write-to-line join), although `frame_flags` is listed for the same reason. ROWS `;frame=F` selectors are never class-checked. `GUEST:pal_bijection` may label any check or form. `sym` is self-declared, so ER-12 is bypassed by writing `sym=none`.
- **Demonstrated:** a scratch mini-root with `flat_frame` and `flat_at` under MAN, an `index=` ROWS row joined to a bxr write under `GUEST:pal_bijection`, a `div_window` of the ASSUMED CR encoding 2 under GUEST, and `REL scroll_y_shift` (a direction relation) under `MAN:H7-7 sym=none` all pass `video-check`. The committed files are classed acceptably today; the gate would not stop a regression.
- **Fix:** add `flat_frame` and `flat_at` to `gridck`; require ASSUMED for ROWS rows with `frame=` unless marked frame-invariant; bind each GUEST model to the checks and literal kinds it may label (pal_bijection: value= ROWS and border_after_pal only); require `sym=asym` (hence ASSUMED) for scroll_y_shift, scroll_x_shift with dx not a multiple of the tile period, and wrap selectors.

#### M-04: No inventory link from .vexp scenes to cases, REQUIRED, fault sweeps or the power-on policy list

- **Files:** `scripts/video-check`, `scripts/inventory-check`, `tests/conformance/videokinds.c:2789-2803`, `tests/conformance/cases/vdc_poweron_policy_diff.toml` (expect `roms=`)
- **Issue:** nothing checks that every `scene ... case=X` names an existing REQUIRED case of kind video. Nothing requires that every literal scene has a `video_fault_sweep` case: all ten exist today by convention, and the runtime kills= check runs only inside a sweep. Nothing requires that every .vexp fixture appears in `vdc_poweron_policy_diff` (the 04-13 SUMMARY flags this).
- `vk_golden_judge` accepts a golden when a literal scene ROW exists, not when the literal case runs and passes. A literal case that is deleted or excluded leaves its golden passing, unless a toy EXPECT happens to name it.
- **Fix:** add a video-check (or inventory) rule: every scene case is a REQUIRED case with kind video and the same scene; every literal scene has exactly one REQUIRED video_fault_sweep case for its scene; every stem with a .vexp is listed in `roms=` of vdc_poweron_policy_diff. Give each rule a mini-root.

#### M-05: vdc_poweron_policy_diff asserts frame literals read from a DuoSpool run and says it does not

- **Files:** `tests/conformance/cases/vdc_poweron_policy_diff.toml:7-8,13`, `tests/conformance/vdckinds.c:2554`
- **Issue:** `roms=vce-pal@5,vce-dot@1,bg-basic@2,...` are "a calibration read from its own run" (line 7), and the kind requires each to EQUAL the frame after the last differing record.
- Line 8 then says "no literal comes from a DuoSpool run". The window start is fitted to the observed run, so for the five real fixtures the guard holds by construction. Only the nopal control and a future change of the setup frames can turn it red.
- **Fix:** derive each first judged frame from the fixture source (the frame after the guest's last palette, VRAM or SATB write, joined from the dev log), assert it as an upper bound rather than an equality, and drop the "no literal" sentence or state that the frame numbers are run-derived E0 calibrations.

#### M-06: F0 on frame_flags makes an out-of-domain literal; flag groups have no meaningful kill

- **Files:** `tests/conformance/videokinds.c:1360,1514-1540`, `fixtures/src/bg_enab.vexp:44-52`, `fixtures/src/bg_basic.vexp:53-55`, `tests/conformance/VIDEO_TOY_EXEMPT:9`
- **Issue:** F0 increments the first number of the first row. For `want=1;frame=10;bit=4` that gives `want=2`, which no 1-bit flag can equal, so F0 "flips" the group whatever the records hold. `vk_s_frame_flags` accepts want up to 255.
- The `flags` group of bg_enab (8 rows) and the `flag` group of video_unmodelled_mode declare only F0, and VIDEO_TOY_EXEMPT line 9 cites that F0 as the kill. The remaining rows of those groups are never challenged.
- **Fix:** reject `want > 1` as a harness fault; make F0 on a boolean literal flip it (want ^ 1), or perturb the frame number instead; give each flag group a record fault that clears or sets the flag bit (a new fault, or F7 extended to the flags byte).

### LOW

#### L-01: The ratio tolerance is narrower than its derived bound, justified by an observed run

- **Files:** `fixtures/src/vdc_irq.s:44`, `fixtures/src/vdc_irq.checks:6-7`
- **Issue:** RATIO_TOL = 9, while the comment derives a worst case of 1.33 + 8 x 1.33 = 12 and keeps 9 because "the run measures within 3 ... for the deterministic phase of this machine". That window is chosen from DuoSpool output (D-169). It hides no hypothesis (a wrong line count is off by about 76 iterations), but a correct model change of the interrupt phase could fail a GUEST row.
- **Fix:** use the derived bound 12 and drop the run-based sentence.

#### L-02: The static grammar is weaker than the runtime grammar

- **Files:** `scripts/lib/vexp-rules.awk:315,331-352,199-201,264`, `tests/conformance/videokinds.c:1564`
- **Issue:** ROWS items `0*V` pass the static vacuity lint (`index=0*1,0*2,6*5` gives 3 "distinct" values, a one-colour row; demonstrated), while `vk_expand` rejects `rep == 0` only at run time as harness. Scene names are global across files in stage 2 but per file at run time, so a group or golden in one file can satisfy a scene of another file statically.
- **Fix:** reject COUNT 0 in the awk regex (`[1-9][0-9]*\*`), count distinct values over dots, and key scenes by file.

#### L-03: README says Phase 4 is complete locally while the validation contract is not signed off

- **Files:** `README.md:15`, `.planning/phases/04-vce-vdc-raw-video/04-VALIDATION.md:4-5,139-145`
- **Issue:** README line 15 says "complete locally"; 04-VALIDATION.md is `status: draft`, `nyquist_compliant: false`, sign-off unchecked, "Approval: pending". The 04-13 SUMMARY makes the flip the orchestrator's step, and HEAD has not made it.
- **Fix:** flip the validation file through the close-out step, or soften the README sentence until that happens.

#### L-04: docs/ci-timings.md has broken table rows; D-177(c)'s 400-character rule is broken and not enforced

- **Files:** `docs/ci-timings.md:731,733,556`
- **Issue:** two stray fragments ("with the identical listing in ...|", "the power-on dimension |") sit inside the D-176 levers table and break its markdown.
- D-177(c) says no committed line is over 400 characters, but about 30 changed lines break it, for example ci-timings.md:556 (425), vdc_dma.s:6-7 (407, 415), vdc_dma.checks:5 (430), fixtures/lib/TOOLS.md:32 (627), vexp-rules.awk:21 (623), videokinds.c:1833 (665).
- Only .vexp files are length-checked (rights-check LINELEN is 4,096).
- **Fix:** repair the two table rows; either enforce 400 for docs, fixtures, ledger and scripts in a gate stage with a mini-root, or restate D-177(c) to the files it really covers.

#### L-05: video_hash_not_verdict step (2) rewrites gd[0] instead of the scene's golden row

- **File:** `tests/conformance/videokinds.c:3020`
- **Issue:** step (1) rewrites every golden row of the scene; step (2) writes only `own->gd[0]`. A .vexp with two golden scenes where the judged one is not first would fail step (2) for the wrong reason. Fail-closed, latent today (one golden per file).
- **Fix:** keep the index found in step (1) and write that row.

#### L-06: record-lint controls still carry format_version 1 and behaviour_epoch 1

- **Files:** `tests/gate/record-lint/*` (16 records), `scripts/lib/record-rules.awk:28-29`
- **Issue:** the committed goldens are format 7 and epoch 5. record-lint checks key presence only, and its ok-accept root has none of the Phase 4 `[model_*]` rows (vd_irq_phase, rcr_origin) or constants rows, so the linter's positive control does not resemble a current record.
- **Fix:** regenerate the ok roots at format 7 and epoch 5, or have record-lint compare format_version and behaviour_epoch with the built constants.

### INFO

#### I-01: The status-read offset is stated as 7 cycles in spin.inc and 9 in vdc_irq.s

- **Files:** `fixtures/lib/spin.inc:23`, `fixtures/src/vdc_irq.s:17`
- The handlers read the status after sta (zp),y 7 + tax 2 (h_gen, h_last) or sta zp 4 + tax 2 + sxy 3 (h_end), which is 9 cycles, so vdc_irq.s is right. The cycle sums 68, 44 and 110 match the opcode table, and VDMA_DELAY 255 x 2 = 9,180 clocks and the vce_pal delay of 9,216 clocks check out.

#### I-02: Withheld words in a ledger statement and in case comments

- **Files:** `ledger/entries/LED-VID-0015.toml:5` ("VL2 slot-accurate VRAM access"), 21 Phase 4 case comments ("boots on the core").
- Both are negative or harness usage; consider "slot-level" and "is booted" to keep the withheld words out of claim-bearing text.

#### I-03: Own-line latch rows cannot fail under R1

- **Files:** `fixtures/src/bg_scroll.vexp:33-35,51-53`
- `bxr1+0`, `bxr2+0` and `byr1+0` are guaranteed by line-start sampling for any latch implementation (A3, A4 acknowledge that no renderer mutant exists). They are MAN-classed rows that only record faults can turn red. Recorded limit, no action beyond a note in the evidence text.

#### I-04: Spot checks that hold (for the record)

- **Digest literals:** the snapshot stream of plain-256k seeded-prng-v1 seed 42 has 92,255 bytes and hashes to 588a0c69... Cutting it to the 14 version-7 frames with version 7 reproduces the committed v7 literal a24223b0..., and appending the seven hand-built D-165 frames (BEAM bytes FF FF at 12, 00 40 at 14, 06 01 at 16) reproduces 588a0c69...
- **vce_pal golden:** a33b1f0e... equals shasum -a 256 of the --video-out stream, and the record count 2,565 equals floor((3,500,052 - 1) / 1365) + 1.
- **README:** the supported list equals the ledger supported set (14 ids).
- **CMake lever:** the object-library change keeps one mutated unit per toy with the same flags and definitions as before; no isolation or staleness problem was found. Sink, dev-log and judge caps (VK_DEV_CAP, VK_CRW_MAX, VK_HV_MAX, t_pal) turn overflow into harness_failure, never a silent pass.

---

_Reviewed: 2026-11-12_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
