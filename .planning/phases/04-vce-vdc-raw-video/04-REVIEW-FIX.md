---
phase: 04-vce-vdc-raw-video
fixed_at: 2026-11-12T00:00:00Z
review_path: .planning/phases/04-vce-vdc-raw-video/04-REVIEW.md, .planning/phases/04-vce-vdc-raw-video/04-REVIEW-gate.md
iteration: 1
findings_in_scope: 29
fixed: 23
skipped: 0
status: partial
---

# Phase 4: Code Review Fix Report (core review and gate review)

**Fixed at:** 2026-11-12
**Source reviews:** 04-REVIEW.md (core: H-01, M-01, M-02, L-01, L-02, I-01 to I-04) and 04-REVIEW-gate.md (gate and evidence: H-01 to H-04, M-01 to M-06, L-01 to L-06, I-01 to I-04)
**Iteration:** 1
**Scope:** every finding of both reports, under the orchestrator rulings. Status `partial` because the rulings close some findings by a
recorded limit or leave them to the orchestrator (core I-01 to I-03, gate M-06, L-03, L-06, I-03, I-04), not because a fix was skipped.

**Summary:**
- Findings: 29 (core 9, gate 20)
- Fixed: 23 (one of them, core L-01, with a documented residual)
- Documented or left by ruling: 6 (core I-01, I-02, I-03; gate L-03, L-06, I-03), plus gate M-06 fixed as a recorded limit and gate I-04 (spot checks, no action)

Commits (all on the current branch, no push):
- `55cbc7d` fix(04): core review fixes H-01, M-01, M-02, L-01, L-02, I-04
- `6be9f6d` fix(04): gate review H-02, H-03, M-01 evidence classes
- `2cb8cdf` fix(04): gate review H-01, H-04, M-02, M-03, M-04, L-02, L-05 video judge and checker
- `f870401` fix(04): gate review L-01, L-04, M-05, I-01, I-02 wording and tolerance
- docs(04): review fix report (this file)

## Core review (04-REVIEW.md)

| Finding | Status | Commit | Evidence |
|---------|--------|--------|----------|
| H-01 dma_done unbounded, restore refused | fixed | 55cbc7d | new case vdc_dma_lenr_live: red before the fix (first stop: dma_done 59999), pass after (37 stops, 37 restores, 2406 assertions) |
| M-01 frame_lines not judged against the profile | fixed | 55cbc7d | snapshot_refusal_reach: frame-263=profile, frame-262=accept |
| M-02 no VBlank when VDS and VDW fill the frame | fixed (decision recorded, assumed) | 55cbc7d | new case vdc_vblank_wrap: VDS 19 and 20 VD every frame, one copy, one DS, MWR latched; VDS 21 none; both VD phases. No golden changed |
| L-01 unreachable beam states accepted | fixed, residual documented | 55cbc7d | snapshot_refusal_reach: vd-pend-line-start, vd-pend-stage-rcr, vd-after-rcr, vd-rcr-equal refused, vd-before-rcr accepted |
| L-02 restore rules restate core constants | fixed | 55cbc7d | the refusal cases of the masks, dividers, word times and the SATB window pass unchanged on the derived rules |
| I-01 unreachable zero-divider branches | documented | - | no change (ruling) |
| I-02 line_sample_offset shifts EV_LINE only | documented | - | no change (ruling); latent while the constant is 0 |
| I-03 narrow counters | documented | - | no change (ruling) |
| I-04 stale plan references | fixed | 55cbc7d | vid.c and vdc.c name plans 04.1-01 and 04.1-02 of Phase 4.1 |

**H-01.** `ds_vdc_dma_apply` (src/core/vdc.c) re-bases the closed form after every apply: `dma_start` moves on by the words applied and
`dma_done` returns to 0, so the progress is the distance from `dma_start` and never accumulates. Chunk invariance holds because
floor((t - start - n*w) / w) = floor((t - start) / w) - n. The restore rule now requires `dma_done == 0` (the rows of
snapshot_refusal_vdma: dma-done-65536 is refused now; snapshot_refusal_reach adds dma-done-1 refused and dma-done-0 accepted). The
regression case vdc_dma_lenr_live (tests/conformance/vdckinds.c) drives the VDC directly between `ds_run` calls on the cpu-alu image
(whose guest never touches the VDC): SOUR = DESR = 0, LENR = $FFFF, then the review's repro (60,000 words, LENR low byte $FF, 5,600
more words: past 65,536 since the start), then 1,100 low-byte writes 128 words apart (the transfer stays alive past 3 x 65,536 words),
a restore into a poisoned twin booted under another seed at every stop, and an end where the transfer completes. At every stop with a
running transfer it asserts `dma_done == 0` and `machine_time - dma_start <= dma_word_mclk`. Deviation: the 2^32 wrap is shown
structurally, not by a run: no member counts the words since the start any more, so nothing can wrap however long a guest keeps the
transfer alive; a run of 2^32 words is about 4.3e9 word moves, too slow for the fast gate. vdc_snapshot_mid_dma now counts its
progress stops from LENR (the progress member is 0 between folds).

**M-01.** `ds_vdc_frame_lines` (src/core/beam.c) is the one derivation of the frame length from the bound profile, used by the power-on
and by `dec_beam`, which keeps the {262, 263} domain rule (field) and then refuses a value that differs from the profile as
`DS_SNAPREF_profile`.

**M-02.** `vertical_step` (src/core/beam.c): when VDW completes on the last line of the frame (VDW with no line to come at the wrap),
the wrap line is the VBlank state entry, so VD (or the outstanding VD of the phase hdw_start), the SATB copy and the MWR latch happen at
the wrap instant and the vertical cycle restarts in VDS on that line. A VDW that still has lines to come is cut with no VD, as before.
The decision is in the assumption and disagreement text of LED-IRQ-0001 and LED-VID-0005 (both stay assumed). No video fixture sits
on this edge: the vce_pal and bg_basic goldens passed unchanged on the full gate, so no digest needed a repin.

**L-01.** `dec_beam` refuses an outstanding VD under the phase line_start, an outstanding VD outside the stage VD, and an RCR event of
the same line at or before the outstanding VD event. The two event rows of snapshot_refusal_window that set an outstanding VD now set
the phase hdw_start and no RCR event, so the window rule alone judges them. Residual (documented): the offsets are not recomputed
from HSR, HDR and the latched divider, because a register write after the EV_LINE legally changes the registers and not the offsets
of that line; and `vd_off` is not required to be none when no VD is outstanding, because the power-on state holds 0 there.

**L-02.** `dec_vdc0` reads the mask table of vdc.c through `ds_vdc_reg_mask`; the divider rule (`lat_div`) and the word time of a running
transfer are judged against the divider table of vce.c through `ds_vce_divider` and `DS_CONST_vdc_dma_dots_per_word`; the SATB window
is 256 words of the largest divider. The second mask table in snapshot.c is gone.

## Gate review (04-REVIEW-gate.md)

| Finding | Status | Commit | Evidence |
|---------|--------|--------|----------|
| H-01 toys red for the wrong reason | fixed | 2cb8cdf | toy evidence: `video_group border GUEST fail` for video_vce_pal, video_vce_dot, video_bg_basic_solid under vid-palette-mask and for video_vce_pal under vid-emit-mutates |
| H-02 vdc_dma check 24 tagged MAN | fixed (entry narrowed, stays supported) | 6be9f6d | fixture-lint, ledger-entries, docs-claims PASS; vdc_dma 24 checks pass |
| H-03 LED-VID-0013 start clause unexercised | fixed (entry narrowed, stays supported; see the note) | 6be9f6d | as above; vdc_satb and vdc_satb_data pass |
| H-04 vexp rules without mutation controls | fixed | 2cb8cdf | selftests mutation-video-rules (14 controls) and mutation-video-lex (6 controls) PASS |
| M-01 four MAN rows assert the converse | fixed | 6be9f6d | rows retagged ASSUMED:LED-VID-0008 |
| M-02 MAN bg groups rest on the row counter start | fixed | 2cb8cdf | groups solid7 and bb_on7 (ASSUMED LED-VID-0009); fault sweeps of bg_basic and bg_enab pass |
| M-03 class lint binds labels, not content | fixed, two limits recorded | 2cb8cdf | mini-roots grid-class, frame-class, guest-bind, rel-asym |
| M-04 no inventory link scene to case, sweep, policy | fixed | 2cb8cdf | mini-roots scene-no-case, scene-no-sweep, fixture-no-policy, ok-inventory |
| M-05 policy_diff comment says no literal from a run | fixed | f870401 | comment states the frames are E0 calibration values |
| M-06 F0 on frame_flags | recorded limit | 2cb8cdf | 04-VALIDATION.md, VIDEO_TOY_EXEMPT, the bg_enab.vexp comment; no second fault kind |
| L-01 RATIO_TOL below its derived bound | fixed | f870401 | RATIO_TOL = 12 with the arithmetic; vdc_irq and vdc_irq_ratio pass |
| L-02 static grammar weaker than runtime | fixed | 2cb8cdf | mini-roots rows-count-zero and scene-other-file |
| L-03 README complete vs draft validation | left (ruling) | - | the orchestrator sets the validation flags |
| L-04 broken table rows, lines over 400 | fixed | f870401 (and the other commits for their own files) | no line over 400 characters among the lines changed since 925662a outside .planning |
| L-05 gd[0] in video_hash_not_verdict | fixed | 2cb8cdf | the scene's own golden rows are rewritten; video_hash_not_verdict passes |
| L-06 record-lint controls at format 1 | documented | - | hand-held mini-roots (info) |
| I-01 spin.inc cycle count | fixed | f870401 | 9 cycles, as in vdc_irq.s |
| I-02 withheld words | fixed | 6be9f6d (LED-VID-0015), f870401 (21 case comments) | "slot-level", "is booted on the core" |
| I-03 own-line latch rows | documented | - | no change |
| I-04 spot checks | no action | - | - |

**H-01.** `vk_capture` (tests/conformance/videokinds.c) takes a flag: the video kind keeps judging its records after a failed guest, so
every group of the scene is evaluated and printed as a `video_group` line, and the outcome stays guest_failure (c->failed is never
cleared); a golden-role case stops at the failed guest, and the fault sweep and video_hash_not_verdict keep the old early return. The toy
evidence now shows the groups: under vid-palette-mask, video_vce_pal shows `border GUEST fail` with every other group passing,
video_bg_basic_solid shows `solidval GUEST fail` and `border GUEST fail`, and video_vce_dot (added to the toy EXPECT) shows `border GUEST
fail` with every other group passing; under vid-emit-mutates, video_vce_pal shows `border GUEST fail`, so INV_TOY_NO_LITERAL is now met by
a literal group that runs and fails. The claims of vce_pal.vexp, vce_dot.vexp, bg_basic.vexp, VIDEO_TOY_EXEMPT and 04-VALIDATION.md (A5
and A7 note) say what the evidence shows. Residual (documented): vdc_irq_agree, vdc_irq_agree_263 and vdc_irq_agree_hdw (kind
vdc_irq_agree) still stop at the failed guest under vid-rcr-base; their model-versus-literal comparisons do not run under the toy, and
the toy's direct kill is the guest case vdc_irq in the same EXPECT.

**H-02.** vdc_dma check 24 is `ASSUMED:LED-VID-0014` in vdc_dma.checks and vdc_dma.s (the transfer is still running at the display start
only under the modelled speed). LED-VID-0014 drops the cancel clause from its E1 statement and keeps it, with the reason, in the
assumption text; the trigger, the M-1 length, the SI/D and DI/D steps and DV with DCR bit 1 keep MAN rows (vdc_dma 4 to 10, 12, 13, 19),
so the entry stays supported. README scopes LED-VID-0014 to the trigger, the length and the DCR bits; the supported list is unchanged and
equals the ledger set (docs-claims PASS).

**H-03.** LED-VID-0013 drops "starts at the beginning of the following vertical blanking period" from its E1 statement into the
assumption text. vdc_satb rows 1 and 2 are `ASSUMED:LED-VID-0013` per ER-10, and so is row 5 (copy-once): beyond the ruling, row 5
discriminates only through the same modelled DS delay, so by ER-10 it is ASSUMED too. The statement is now "moves 256 words and is
triggered by a write to the high byte of DVSSR (H7-12)". **Note for the orchestrator:** the remaining E1 content is exercised by the pin
TEST:vdc_satb_data (the page count of 256 words, copied after a DVSSR high-byte write, equal to the guest's pattern), not by a MAN-tagged
row of vdc_satb.checks: none of those is left for LED-VID-0013. The entry stays supported on that pin; if the ruling's "MAN-exercised"
means a MAN-tagged guest row, LED-VID-0013 should be demoted to assumed (its status line, the README supported list and the README
scope sentence, kept equal by docs-check).

**H-04.** Every code of the video checker has its own tag and control. New tags: rule-vexp-grid-class (the grid rule, split from
rule-vexp-class-lint), rule-vexp-frame-class, rule-vexp-guest-bind, rule-vexp-rel-asym, rule-video-scene-case, rule-video-scene-sweep,
rule-video-fixture-policy, and rule-lex-line-too-long, rule-lex-non-ascii, rule-lex-no-spdx, rule-lex-too-many-rows,
rule-lex-file-too-large in vexp-lex.awk. New committed mini-roots under tests/video: format, exempt-format, grid-class, frame-class,
guest-bind, rel-asym, rows-count-zero, scene-other-file, scene-no-case, scene-no-sweep, fixture-no-policy, ok-inventory. The five stage 1
limits are generated controls (`vc_gen_limits` in scripts/check, run by the video-check stage and by the new selftest
mutation-video-lex), because no committed file may hold a line over 400 characters or a non-ASCII byte. The z7c list of
mutation-video-rules gains format, exempt-format, grid-class, frame-class, guest-bind, rel-asym, scene-no-case, scene-no-sweep and
fixture-no-policy; mutation-video-lex deletes each rule-lex tag from vexp-lex.awk and the rule-stage1-limits line from vexp-rules.awk.
The line-28 claim of vexp-rules.awk lists the tags and their controls. rule-stage1-row has no file control (stage 1 never prints the
row it catches); the RULES sentinel and VIDEO_STAGE_DEAD guard that boundary.

**M-01.** vdc_stat rows 1, 3 and 16 and vdc_dma row 11 are `ASSUMED:LED-VID-0008` (the converse "no flag without the enable" is the
reading of three emulators, outside the E1 statement). LED-VID-0008 keeps MAN rows for VD, RR, DV and the clear on read (vdc_stat 5, 6,
9, 10, 17, 18, 21; vdc_dma 12, 13); its text is unchanged.

**M-02.** bg_basic.vexp: the MAN group solid keeps display line 0 (BAT row 0 under both readings of the row counter start); the row at
display line 7 is the new group solid7, ASSUMED LED-VID-0009, kills F0, F1, F7. bg_enab.vexp: likewise bb_on and bb_on7. The fault
sweeps flip both new groups with their declared faults.

**M-03.** In vexp-rules.awk: flat_frame and flat_at are grid-dependent checks (ASSUMED only); a ROWS row with `;frame=F` under MAN or
GUEST is VEXP_CLASS, and `;held=F` (accepted by the runtime parser as the same selection) is the author's mark that the literal does not
rest on the frame number; the MAN and GUEST rows of bg_basic and bg_enab use it, the ASSUMED rows keep `;frame=F`. A GUEST model is
bound to what its arithmetic gives (pal_bijection: border_after_pal and value= ROWS rows on line=active+K only). A REL scroll_y_shift,
or a scroll_x_shift whose dx is not a multiple of 8, under a class other than ASSUMED is VEXP_ASYM_CLASS. Recorded limits: sym= and
held= stay self-declared, and a wrap selector is not recognised statically.

**M-04.** video-check reads the case inventory where the root carries tests/conformance/cases (VC and PR facts) and vexp-rules.awk
requires: each scene case is a REQUIRED case of kind video whose expect names the scene (VIDEO_SCENE_NO_CASE); each literal scene has
exactly one REQUIRED video_fault_sweep case of its scene (VIDEO_SCENE_NO_SWEEP); each .vexp ROM is in the roms= list of
vdc_poweron_policy_diff (VIDEO_FIXTURE_NO_POLICY). The repository passes all three. vk_golden_judge itself is unchanged: a deleted
literal case is now caught by VIDEO_SCENE_NO_CASE.

**M-06.** Recorded limit: the flag groups of video_unmodelled_mode and video_bg_enab declare only F0, which proves the comparison live
and no more; no second fault kind was added (not cheap: rejecting want > 1 would leave the flag groups with no declared kill).

**L-01.** RATIO_TOL = 12 in vdc_irq.s with the arithmetic in the comment (1.33 + 8 x 1.33 = 12); the run-based sentence is gone and
the vdc_irq case comments say 12. Deviation: D-174 counted only the quantisation (8 + 1 = 9); the source now says why 12 is the bound.
The ROM bytes of vdc-irq and vdc-irq-263 change (one immediate operand), so their manifest pins were remeasured with shasum -a 256 in the same commit; no video golden or snapshot digest covers those images.

**L-04.** The two stray fragments of the D-176 levers table in docs/ci-timings.md are removed (each repeated the end of the row above).
Every line over 400 characters among the lines changed since 925662a (outside .planning) is wrapped: comments in src/core (vid.c,
ds_core.h, serial.c, beam.c), the test kinds, runner.h, four toy RULE files (comment lines only; the anchors are unchanged),
fixtures/lib/TOOLS.md, vdc_dma.s, vdc_dma.checks, fixture-build, vexp-rules.awk and ci-timings.md:556; three long string literals in
videokinds.c and vdckinds.c are split by concatenation. No enforcement stage was added; the generated stage 1 controls are not committed.

**L-06.** The record-lint controls are hand-held mini-roots at format_version 1 and behaviour_epoch 1; record-lint checks key presence
only, so they still prove what they test. Regenerating them at format 7 and epoch 5 is left for a later plan.

## Verification

All gates ran in the main checkout (workflow.use_worktrees is false; no worktree was created).

- `sh scripts/check`: CHECK RESULT PASS on the core group before `55cbc7d` (8 min 53 s). On the tree of the three gate groups the first
  run was red in one stage, `fixtures` (FIX_HASH_MISMATCH for vdc-irq and vdc-irq-263: the L-01 tolerance change altered one immediate
  operand); the two image pins were remeasured with `shasum -a 256` (fixtures/manifest/vdc_irq.toml and vdc_irq_263.toml, with a repin
  note) and the static stages that read manifests passed. The final run at HEAD `f870401` is CHECK RESULT PASS (6 min 26 s).
  Deviation: the three gate commits were gated together on their combined tree, not one by one (one green tree split by file).
- `sh scripts/check --selftest` at `f870401`: SELFTEST RESULT PASS (160 PASS lines, among them the 14 controls of mutation-video-rules
  and the 6 of mutation-video-lex).
- `sh scripts/audit-history --range 29edb6e..HEAD` at `f870401`: clean (4 commits, 110 blobs).
- `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro`: PASS after the core group (86 s). At the end, with the gate commits,
  `LINUX_REPRO_PARTS="gate-gcc gate-clang core-boundary-gcc"`: gate-gcc PASS, gate-clang PASS, core-boundary-gcc PASS (125 s; a local
  one-off container run, not a CI claim).
- behaviour_epoch was not bumped: H-01 changes the dma_start values of a running transfer and M-02 changes behaviour only on the edge it
  fixes; both are inside the unreleased Phase 4 epoch 5, and no committed golden or digest covers either.

---

_Fixed: 2026-11-12_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
