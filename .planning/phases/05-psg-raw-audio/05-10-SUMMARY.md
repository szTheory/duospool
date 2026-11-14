---
phase: 05-psg-raw-audio
plan: 10
subsystem: phase-5-close-out
tags: [aud, close-out, d-217, d-234, er-2, er-22, er-10, er-15, er-17, validation, readme, linux-repro, dash, second-awk, w9]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-01..05-08 and 05-12 (the PSG model, psg_tone and psg_mod with their literal scenes, the audio judge and fault sweep, six audio toys, checkpoint A and the cut record)
provides:
  - tests/audio/RULES at the cut, with no later entry; the 17 LFO clauses are limit lines named lfo-in-5.1
  - LED-AUD-0005 clause (c) narrowed to chON 1 and DDA 1, and the entry supported (E1 documentary); 15 supported entries in all
  - the d_hold split (MAN writes 2..9; ASSUMED d_hold1 with mark=latch_off) and ev_decode_sel0_pw (ASSUMED:LED-PROF-0003)
  - the two never-judged kinds dropped; the LFO judge markers name 05.1-01
  - every LED-AUD entry pinned to a TEST: case that names it back
  - the README audio paragraph (D-218 wording, the LFO named as Phase 5.1)
  - docs/ci-timings.md "Phase 5 close-out" (phase-gate set, trigger (3) by instruction count, efficiency review inputs)
  - 05-VALIDATION.md signed (status final, nyquist_compliant true) and the handbook's final state
affects: [05.1-01, 05.1-02, phase 6]

requirements-completed: [AUD-02, AUD-03]
requirements-advanced: [AUD-01]

actuals:
  tokens: 33000
  tasks: 4
  commits: 4
plan_head_before: 3f1706054487b79be22151e5381decbe81a36f68
plan_head_after: 910fa09

tech-stack:
  added: []
  patterns:
    - "A judge group that rests on a disputed point or a power-on value at one ordinal is split: the MAN group re-anchors past that write and renumbers its ordinals, and the one row gets its own ASSUMED group with a mark (ER-10, ER-15)"
    - "At a phase cut, the RULES clauses of the moved plan become limit lines whose NEED names the later phase, because the lint accepts later targets inside the phase only"

key-files:
  created:
    - .planning/phases/05-psg-raw-audio/05-10-SUMMARY.md
  modified:
    - tests/audio/RULES
    - tests/audio/KINDCLASS
    - tests/conformance/audiokinds.c
    - fixtures/src/psg_tone.aexp
    - fixtures/src/psg_mod.aexp
    - ledger/entries/LED-AUD-0001.toml to LED-AUD-0014.toml (13 pins; -0005 promoted; review notes on -0002 and -0003)
    - tests/conformance/cases/ (the pins arrays of 12 cases)
    - README.md
    - docs/ci-timings.md
    - .planning/phases/05-psg-raw-audio/05-VALIDATION.md
    - .planning/phases/05-psg-raw-audio/05-HANDBOOK.md

key-decisions:
  - "05-10: LED-AUD-0005 is supported (E1 documentary) after clause (c) was narrowed to chON 1 and DDA 1 (P8-4). Every clause has a clean MAN group in a guest-fixture case, and the img sources record no in-page conflict. The chON 0 latch is the ASSUMED group d_hold1, outside the statement"
  - "05-10: LED-AUD-0002 stays assumed (its LFO clause has no Phase 5 row), and so does LED-AUD-0003 (its disputed and converse clauses stay in the statement; the guest rows of its mask clause come with psg_lfo in 05.1-01). LED-AUD-0001, -0006 and -0011 stay assumed under ER-14"
  - "05-10: trigger (3) re-measured by instructions retired reads +22.2 percent (gate) and +25.2 percent (sanitize) against 9619efe, equal to checkpoint A. It is recorded, not decided on: the cut is already taken (D-234)"

duration: about 4 h 35 min wall (19:30 to 00:05 local); gates at a host load of 6 to 129 from other projects
completed: 2026-11-14
---

# Phase 5 Plan 10: Phase 5 close-out Summary

**Phase 5 closes at the D-217 cut, with the LFO under DSPE UNMODELLED.**
- tests/audio/RULES has no later entry.
- LED-AUD-0005 is narrowed to what P8-4 states and is now supported (E1 documentary).
- No MAN group rests on the latch_off point or on a power-on value.
- Every audio entry is pinned both ways (the entry names a case, and the case names the entry).
- The README audio paragraph uses only the D-218 wording.
- 05-VALIDATION.md is signed.

AUD-02 and AUD-03 complete with this plan. AUD-01 completes in Phase 5.1.

## Performance

- Tasks: 4 of 4, no split. Commits a3a2238, 62bca83, 632cd58 and 910fa09: 34 files, 297 insertions, 150 deletions before this SUMMARY.
- Other projects' jobs held the host load between 6 and 129, so wall times are long and the user CPU figures are not comparable with earlier runs.

## Gate results

One gate ran at a time in this checkout. The baseline clone is a separate checkout.

| Run | Tree | Result | Wall s | User CPU s | Load (1 min), mean |
|---|---|---|---|---|---|
| Task 1 narrow verify (audio-check, ledger-entries, conformance, the six RESULT greps, the RULES and kind greps, the psg_tone.aexp size) | Task 1 edits | VERIFY1_OK | | | |
| `sh scripts/check --timing` before the Task 1 commit | content of a3a2238 | PASS, 58 stages (59 CHECK PASS lines) | 641.6 | 516.5 | 11.7 |
| `sh scripts/check --timing` in the 9619efe scratch clone (the bracket partner) | 9619efe | PASS, 57 stages | 752.7 | 495.2 | 28.9 (not like for like) |
| Instructions retired by one ds_runner over each stage's case list | a3a2238 against 9619efe | gate 1,013.75 G against 829.37 G (+22.2 percent); sanitize 4,079.08 G against 3,259.19 G (+25.2 percent); every case passes | | | 14 to 41 |
| `sh scripts/linux-repro`, all six parts | a3a2238 | RESULT PASS. default-awk (mawk 1.3.4, gcc 13.3), mawk (clang 18.1.3) and gawk (5.2.1, gcc 13.3): each gate 58 stages, each selftest PASS. gate-gcc, gate-clang and core-boundary-gcc PASS | 9,119 | | 36.1 (12 to 112) |
| `dash scripts/check`, first run | a3a2238 | harness error: conformance stopped at `HARNESS_WALL seconds=300` because of the host load; every earlier stage PASS | 1,473.0 | 551.8 | 80.2 (up to 129) |
| `dash scripts/check`, re-run | a3a2238 | PASS, 58 stages | 726.2 | 514.9 | 19.9 |
| `sh scripts/check --selftest` | a3a2238 | SELFTEST RESULT PASS, 207 PASS lines | 281.7 | 76.4 | 41.2 |
| `sh scripts/audit-history --range f6afede..a3a2238` (the whole Phase 5 range) | | clean: 45 commits, 408 blobs, 2,456 blob-path pairs | 68.2 | | |
| Task 2 verify (ci-timings-controls, docs-claims, grep) | content of 62bca83 | VERIFY2_OK | | | |
| Task 3 narrow verify (ledger-entries, inventory, fixture-lint, audio-check, docs-claims, docs-claims-negatives, greps) | Task 3 edits | all PASS | | | |
| `sh scripts/check --timing` before the Task 3 commit (ledger status, README list) | content of 632cd58 | PASS, 58 stages | 878.4 | 535.9 | 35.2 |
| Task 4 verify | content of 910fa09 | VERIFY4_OK | | | |
| Plan end: `sh scripts/check --timing` | 910fa09 | PASS, 58 stages (59 CHECK PASS lines) | 654.1 | 475.3 | 23.3 |
| Plan end: `sh scripts/check --selftest` | 910fa09 | SELFTEST RESULT PASS, 207 PASS lines | 187.8 | 56.0 | 22.8 |
| Plan end: `dash scripts/check` | 910fa09 | PASS, 58 stages | 646.2 | 474.0 | 21.5 |
| Plan end: `sh scripts/audit-history --range 3f17060..HEAD` | | clean: 4 commits, 35 blobs, 2,250 blob-path pairs | 4.1 | | |
| Plan end: `LINUX_REPRO_PARTS="gate-gcc gate-clang" sh scripts/linux-repro` | 910fa09 | RESULT PASS (strict C11 with -Werror under gcc 13 and clang 18) | 88.6 | | 30.0 |

ROM-SIZE: none of the 255 files added or modified in Phase 5 has a ROM-SIZE shape. psg_tone.aexp is 41,065 bytes with 190 rows and psg_mod.aexp is 13,858 bytes with 35 rows, both under the D-233 cap of 49,152 bytes and 512 rows. The handbook is 25,567 bytes (cap 25,600).

## Task 1: the evidence fixes at the cut (a3a2238)

- **RULES.** The 17 later:05-09 lines are now limit lines with NEED lfo-in-5.1: aud02-c, aud04-g, aud06-c, aud07-b, aud11-a..d, d184-d, d184-e, d192-a, d220-b, d219-a..c, rf4-a and rf5-a.
  Each TEXT ends "(moved to Phase 5.1 at the D-217 cut, D-234)". rf5-a's TEXT keeps that its DDA half has the row d_rel. RULES now holds 80 row lines and 24 limit lines, and no later line. The header comment and scripts/ are unchanged.
- **aud05-c and LED-AUD-0005 clause (c).** Both now read "with chON 1 and DDA 1 an R6 write goes to the D/A and is held until the next write, wave RAM unchanged". The entry's assumption cites P8-4 and names chON 0 with DDA 1 as the disputed latch_off point (LED-AUD-0004, aud04-h). aud04-h's TEXT now names d_hold1.
- **The d_hold split (psg_mod.aexp).**
  - d_hold (MAN:P8-4+P8-5) now anchors on write 1 (the 3rd R6 $05 of select 2) with from=+384, two samples past its stamp. Its window holds writes 2..9 as ordinals 1..8, so ords=2,3,4,5,6,8 (writes 3, 4, 5, 6, 7 and 9).
  - The new group d_hold1 (ASSUMED:LED-AUD-0004, mark=latch_off) judges write 1 alone, from R4 $DF to write 2 (the 4th R6 $05), with ords=1.
  - Judge output: d_hold covers "the 8 DDA writes of channel 2 in [7802, 8834)" and d_hold1 "the 1 DDA writes of channel 2 in [7652, 7927)".
- **ev_decode_sel0_pw (psg_tone.aexp, scene tone_sel0, ASSUMED:LED-PROF-0003).** The R2 $00 row at ev=+35 (post $0000, which carries the power-on F high nibble) moved out of the MAN group into this group.
- **The kinds.**
  - cycle_linear and dda_equals_wave are gone from KINDCLASS, the enum and au_kinds, and no comment names them.
  - The two later-kind rows of audio_judge_forms now use lfo_static (ROWS) and lfo_needs_sel1 (REL), with the same literals and the same harness outcome (judge_checks=109, every check holds).
  - The three LFO markers and the header comments name 05.1-01.
- **No literal value changed.** Sweep kills of the new and moved groups, declared then observed:
  - d_hold: AF0,AF5,AF6,AF8, then AF0,AF2,AF4,AF5,AF6,AF7,AF8,AF9.
  - d_hold1: the same declared and observed sets.
  - ev_decode_sel0_pw: AF0,AF8,AF9, then AF0,AF8,AF9.
  - Sweep totals: mod_dda flipped=34/34 (was 30/30), tone_sel0 42/42.

## Task 2: the phase-gate set and trigger (3) (62bca83)

- docs/ci-timings.md gains "Phase 5 close-out (plan 05-10; local runs, not CI runs)". It holds:
  - the runs above;
  - the 05-01 baseline, 05-04, 05-08 and this run side by side, with stage seconds;
  - the D-214 rows with their final values;
  - the cut and what moved to Phase 5.1;
  - the efficiency review inputs: CI unverified, lever (a), the largest stages, the wall watchdog and the R11 push hazard.
- Trigger (3) by instruction count: +22.2 percent in the gate preset (388 against 334 cases) and +25.2 percent in the sanitize preset (386 against 332), equal to checkpoint A.
  - Between checkpoint A and this run, the counts of both trees moved by 0.1 to 0.2 percent. That is the repeatability of the method.
  - The bracketed user CPU of the same runner work was [+16.8 percent gate, +22.4 percent sanitize] at loads of 14 to 41.
  - The full-gate pair (516.5 s against 495.2 s) is not like for like (mean load 11.7 against 28.9). It is recorded and not used.

## Task 3: pins, the ER-2 and ER-22 pass, the README (632cd58)

Pins: each case's pins array names its entry back (INV_PIN_BACKLINK), and revisit_phase stays 5.

| Entry | pinning_test | Why this case |
|---|---|---|
| LED-AUD-0001 | TEST:audio_tone_sel0_d3 | the d = 3 expect_red case shows the divisor in every period group |
| LED-AUD-0002 | TEST:audio_mod_noise5 | the chan 5 decode and noise on select 5 (n5_ev, n5_two, n5_grid, n5_ratio) |
| LED-AUD-0003 | TEST:psg_decode | every mask, the mirror, the select 6/7 drops and the global R1, R8, R9, at C level |
| LED-AUD-0004 | TEST:psg_wave_ptr | the held write, the key-on start, the freeze, the DDA restart and the latch, at C level |
| LED-AUD-0005 | TEST:audio_mod_dda | d_dec, d_hold, d_ord, d_ram (clauses b and c) |
| LED-AUD-0006 | TEST:audio_tone_sel0 | the period groups of the tracer |
| LED-AUD-0007 | TEST:psg_wave_ptr | F = 0 steps every 24,576 master clocks (Fe 4096) |
| LED-AUD-0009 | TEST:psg_vol_table | the whole curve, and attenuations of 30 or more silent |
| LED-AUD-0010 | TEST:audio_mod_noise4 | n4_seq, n4_two, n4_grid, n4_min |
| LED-AUD-0011 | TEST:psg_lfo_trg | at the cut: the R9 store, the TRG reset, the freeze and UNMODELLED (05.1-02 may move it to audio_lfo_static) |
| LED-AUD-0012 | TEST:psg_fold | fold partitions, the D-186 invariant, an event at a write stamp processed before the write |
| LED-AUD-0013 | TEST:psg_dev_join | one DSPE record per window-2 write, joined to the dev log |
| LED-AUD-0014 | TEST:pcm_cadence | ceil(M / 192) frames and the per-frame counts |

**LED-AUD-0005, promoted.** It gains an E1 evidence row, and its statement now reads "VL0 (E1 documentary) ... equal to the manual's stated rules (P8-4, P8-5)".

| Clause | MAN groups in guest-fixture cases | Declared kills | Other evidence |
|---|---|---|---|
| (a) chON 0, DDA 0: the word is stored at the counter, which then increments | ev_decode_sel0 (R6 WAVE_STORE, wave_idx k & 31; audio_tone_sel0, MAN:P8-2+P8-3+P8-4); cycle_order_L/R (MAN:P8-4+P8-5) | AF0,AF8,AF9; AF0,AF5,AF6,AF7 | mutants W21, W22 (05-08) |
| (b) DDA 1 resets the counter to 0 | the R4 $40 row of ev_decode_sel0; d_dec (R4 $40 after a played index, then R4 $DF; audio_mod_dda, MAN:P8-2+P8-3+P8-4) | AF0,AF8,AF9 | the toy psg-dda-no-reset turns d_dec red; mutant W13 |
| (c) chON 1, DDA 1: each write goes to the D/A and is held until the next one; wave RAM unchanged | d_hold (writes 2..9), d_ord, d_ram (audio_mod_dda, MAN:P8-4+P8-5) | AF0,AF5,AF6,AF8; AF0,AF5,AF6; AF0,AF5,AF6,AF7 | mutants W18, W19, W14 |
| (d) chON 0: the output is off | silent_pre_L/R (audio_tone_sel0, MAN:P8-4) | AF0,AF6 | mutants Y03, Y04 |

ER-2 conditions, all met:
1. P8-4 and P8-5 are img rows of fixtures/lib/cites.txt; FXL_OCR_BACKS_SUPPORTED passed.
2. Every literal of the covering groups is a page rule; none comes from a DuoSpool run.
3. The statement says "equal to the manual's stated rules".
4. withheld now lists hardware-verified, hardware accuracy, timing, game compatibility and the D/A latch loaded with chON 0.
5. The pin is mutual: TEST:audio_mod_dda, whose pins name the entry.

No clause holds a disputed point: the chON 0 latch is d_hold1 (ASSUMED), outside the narrowed statement.
DX-12 records a disagreement with other sources (pcetech, MAME, Mednafen, Mesen2) on when the counter resets; the entry records the two readings as observably equal, and DX-12 is not an in-page conflict. The full gate passed before the commit, and README's supported list equals the ledger's supported set (15 entries).

**Not promoted:**
- LED-AUD-0002: its LFO clause (select 1 modulates select 0) has no Phase 5 row; RULES aud02-c is the limit lfo-in-5.1. 05.1-02 reviews the entry. A comment line in the entry records this.
- LED-AUD-0003: its disputed clauses ($A to $F none; select 6 and 7 none) and its converse (R7 only at select 4 and 5) stay in the statement. The guest rows of its mask clause (R8, R9 bits 1-0, an F of $800 or more) come with psg_lfo in 05.1-01 (05.1-CONTEXT C-05). A comment line in the entry records this.
- LED-AUD-0001, -0006 and -0011: held by ER-14 (they state a period, a frequency or an LFO rate).

README: one Phase 5 paragraph, placed before the supported list, made of the D-218 sentences:
- DuoSpool executes the authored PSG fixtures and emits DSPE v1 and DSPA v1 before any host resampling;
- the supported PSG entries are E1 documentary;
- the period rows equal the manual formula under the bound psg_tick_divisor 6 while the PSG clock stays open, so no pitch or timing claim is made;
- noise repeats under the same power-on policy (E0);
- LFO modulation is Phase 5.1: UNMODELLED is logged and select 0 is not modulated;
- a hash is E0 evidence only;
- the D-218 will-not-claim items are withheld.

The supported paragraph gains one scope sentence for LED-AUD-0005. The README does not call Phase 5 "complete".

## Task 4: the validation file and the handbook (910fa09)

05-VALIDATION.md is signed (2026-11-13):
- frontmatter: status final, nyquist_compliant true, wave_0_complete true;
- a status column on the requirement map (AUD-01 f is Phase 5.1);
- the per-task map all green, with the two 05-12 rows added;
- a "Negative controls seen red" table with evidence lines from the gate;
- a "Close-out results" section;
- the Wave 0 list and the sign-off boxes ticked.

Handbook: a D-234 line, the final budget line and the cut outcome (25,567 bytes).

## Verification-report inputs (for the orchestrator and the verifier)

**Automated versus handed off.** Everything above was automated, and no human UAT is needed. Handed off:
- (a) The push and the first Phase 5 CI run (D-64). The CI critical path stays unverified; the checkpoint A estimate is 525 to 670 s against the 600 s job trigger.
- (b) The owner's yes or no on keeping the lever (a) workflow commit 9619efe, asked once at publish time (D-215, D-226).
- (c) Settling C1, or a profile r2 that narrows psg_tick_divisor, at a milestone review.
- (d) The R11 push hazard of 05.1-CONTEXT C-08, for the push decision. A push of Phase 5 before 05.1-01 runs makes epoch 6 public. 05.1-01's LFO behaviour change would then need an epoch bump, and no 5.1 plan contains one.

**ROADMAP Phase 5 success criteria, with case ids.** Every result is E0 repeatability; row classes are as tagged in the .aexp files.

| Criterion | Cases |
|---|---|
| 1. All six channels and every register, with decoded semantics (LFO modulation in 5.1) | psg_tone ev rows (audio_tone_sel0, -pan01, -pan23, -pan45, -atten: ev_value_sel0, ev_decode_sel0, ev_1..ev_5, ev_probe, edge, edge_v); psg_mod ev rows (audio_mod_noise4, -noise5, -r7, -dda); the LFO register decode at C level (psg_decode, psg_lfo_trg); the modulation half is Phase 5.1 |
| 2. A defined, recorded rate and format, with the per-frame count visible | the DSPA header STRUCT group (header of tone_sel0), pcm_cadence, the count group, the F rows of --audio-out (audio_out_rights, audio_out_check in conformance-run) |
| 3. Period, content and silence checks, which a mis-programmed fixture fails | period_L/R, cycle_order_L/R and silent_pre_L/R of tone_sel0; audio_tone_sel0_d3 (the period groups go red at d = 3); audio_tone_pan01_twin (an exact red set of 7 groups); the nine fault sweeps; the null-synth rows of audio_judge_forms |
| 4. Noise repeats; the clock field stays open; no pitch or timing claim | audio_noise_repeat (zero, v4 s1, v4 s2) and n4_seq; LED-AUD-0001 assumed (ER-14); the docs-claims withheld words (tests/docs/claim-word-audio) |

**Honesty table.**

| Requirement | State | Cases and tier |
|---|---|---|
| AUD-01: the PSG model, including the LFO to the documented level | advanced; completes in Phase 5.1 (05.1-02), because its text names the LFO (D-234) | all but the LFO: psg_decode, psg_wave_ptr, the tone, attenuation, noise and DDA scenes; LED-AUD-0005 and -0008 E1 documentary, the rest assumed; LFO registers logged and decoded (psg_lfo_trg) |
| AUD-02: a timestamped PSG event log and PCM at a recorded format | complete (E0; the formats are definitions, ER-19) | psg_dev_join, pcm_cadence, psg_chan_append, pcm_chan_append, observer_psg_tone, observer_psg_mod, aud_noninterference, chunk_equiv_audio_psg_tone, chunk_equiv_audio_psg_mod, snapshot_restore_audio, psg_fold, audio_out_rights, and the header and count groups |
| AUD-03: semantic audio checks and deterministic noise | complete (E0; MAN rows are E1 documentary where their entry is supported) | the audio kind (ROWS, REL, STRUCT, EV) over 11 scenes, the nine audio_fault_sweep cases, audio_judge_forms, audio_hash_not_verdict, audio_tone_golden, audio_noise_repeat, aud_poweron_policy_diff and the audio-check stage |

**Counts.**
- Audio judge (psg_tone.aexp and psg_mod.aexp): 11 scenes, 97 groups and 225 rows, plus 1 golden row.
- Groups by class: MAN 59, ASSUMED 34, GUEST 3, DEF 1. Rows by class: MAN 168, ASSUMED 43, GUEST 12, DEF 2.
- Groups by form: ROWS 72, EV 16, REL 7, STRUCT 2. Rows by form: EV 111, ROWS 96, REL 15, STRUCT 3.
- Toys: 32 in all, 6 of them audio (psg-lfo-shift is Phase 5.1), with 32 RED_OK lines. AUDIO_TOY_EXEMPT has 0 entries (header comments only).
- REQUIRED cases: 388. RULES: 80 row and 24 limit lines. Ledger: 75 entries, 15 supported.

**Linux part results.** All six parts PASS on a3a2238, the tree after the only C change (table above). At plan end, gate-gcc and gate-clang PASS on 910fa09.

**Findings and deviations of the phase** (from the plan SUMMARYs):
- 05-12 is the stop-rule split of 05-06 Task 3b and brought the D-233 cap.
- The ER-10 fixes left for this plan are done (d_hold ordinal 1, the R2 row).
- The 05-08 mutant sweep left 3 survivors, all equivalent.
- Trigger (3) was borderline at checkpoint A, and the cut was taken (D-234).
- No revision plan (05-16 or later) exists.

**Reliance on the unreleased epoch 6 (R11).** Phase 5 bumped serial 9, epoch 6 and text 8 once, in 05-03. No push has happened, so epoch 6 is unreleased. 05.1-01's LFO behaviour change can land under it only if Phase 5 is not pushed first (C-08).

**Hand-off list.**
- Phase 5.1 (05.1-CONTEXT):
  - 05.1-01 returns the 17 lfo-in-5.1 limit lines to row (C-03), adds lfo_release (C-04) and the guest decode rows for R8, R9 and an F of $800 or more (C-05), judges the three LFO kinds, and replaces the two later-kind rows of audio_judge_forms (C-06).
  - 05.1-02 may move LED-AUD-0011's pin to audio_lfo_static, reviews LED-AUD-0002 and -0003, rewrites the README LFO sentence (C-07), signs 05.1-VALIDATION.md and completes AUD-01.
- Phase 6: DSPE v1 and DSPA v1 as wire formats; the D-201 pcm sink sizing; the [audio] table decision. No API or ABI is frozen.

## Live budget table (after 05-10, final for Phase 5)

| Budget | Final | Phase 5 target |
|---|---|---|
| Gate stages | 58 | 58 |
| Selftest PASS lines | 207 | >= 178 |
| Constants rows / CROWS_MAX | 49 / 64 | 49 / 64 |
| Withheld ids | 42 | 42 |
| Regions; serial / epoch / text | 24; 9 / 6 / 8 | 24; 9 / 6 / 8 |
| OX_ROMS_MAX / guest ROMs | 48 / 38 | 48 / 39 (psg_lfo in 5.1) |
| DSV ids | 27, 28 | 27, 28, 29 (29 in 5.1) |
| Toys | 32 | 33 (32 after a cut) |
| Conformance REQUIRED cases | 388 | about 379 to 394 |
| Supported ledger entries | 15 (13 + LED-AUD-0008 + LED-AUD-0005) | at most 4 new |
| .aexp bytes (psg_tone / psg_mod) | 41,065 / 13,858 | below 49,152 |
| Handbook bytes | 25,567 | at most 25,600 |
| Runner instructions against 9619efe | +22.2 percent gate, +25.2 percent sanitize | trigger (3) over +25 percent (user CPU) |
| CI critical path | unverified; estimate 525 to 670 s | job trigger 600 s |
| Context budget (D-229 stop rule) | finished without a split, under the 80 percent threshold | stop at about 80 percent |

## Task Commits

1. Task 1: the evidence fixes at the cut, `a3a2238` (test)
2. Task 2: the close-out timings and trigger (3), `62bca83` (docs)
3. Task 3: pins, LED-AUD-0005 supported, the README, `632cd58` (feat)
4. Task 4: the validation sign-off and the handbook, `910fa09` (docs)

## Deviations from Plan

**1. [Process] The standing executor brief named by the orchestrator was not on disk.** The orchestrator's message restated its constraints, and the plan's standing rules repeat them; both were followed.

**2. [Rule 3, environment] The first dash run ended in a harness error, not a check failure.** At a host load of 80 to 129, the 300 s wall watchdog that conformance-run sets for the gate preset fired (`HARNESS_WALL seconds=300`). Every earlier stage had passed. The unchanged tree was re-run once the load fell, and it passed. No script or watchdog was changed.

**3. [Evidence] The bracketed user CPU pair is not like for like.** The baseline clone ran at a mean load of 28.9 and this tree at 11.7, so the pair is recorded and not used. Trigger (3) is read from the instruction counts, as the plan prescribes.

**4. [Scope, within files_modified] Notes beyond the plan text.**
- aud04-h's TEXT names d_hold1.
- LED-AUD-0002 and -0003 each carry one comment line that records their blocking clauses.
- The README supported paragraph has one scope sentence for LED-AUD-0005.

**5. [Order] Linux before dash.**
- The six Linux parts ran first, on a3a2238 as the plan requires.
- Dash, the selftest and the audit followed, one at a time.
- The instruction counts are not a gate and do not depend on the load. They ran in scratch directories during the first 13 minutes of the Linux run.

**Note for the verifier.** RULES aud03-f's TEXT still gives "post $0000 after R2" as an example. That row is now the ASSUMED group ev_decode_sel0_pw. The clause stays covered by MAN ev_decode rows (ev_probe: R2 $3C after R3 $00; ev_2..ev_5), so the line was left as it is.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model:
- T-05-34: the README and ledger text come from gate output, and the ER-2 and ER-22 clause table is above. DOCS_SUPPORTED_LIST, DOCS_CLAIM_WORD, INV_PIN_BACKLINK and FXL_OCR_BACKS_SUPPORTED passed in the full gate. The README names the LFO as Phase 5.1.
- T-05-36: the full phase-gate set ran, including dash, the second awk and all six Linux parts. Nothing was weakened.
- T-05-37: each class edit narrows a class or a statement, and no literal value changed. The sweep shows every declared kill of the new groups flipping.

## Self-Check: PASSED

The four task commits (a3a2238, 62bca83, 632cd58, 910fa09) are ancestors of HEAD; RULES, both .aexp files, docs/ci-timings.md, 05-VALIDATION.md and this SUMMARY exist; git rev-list --count 3f17060..910fa09 is 4.
