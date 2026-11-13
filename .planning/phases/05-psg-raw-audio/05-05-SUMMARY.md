---
phase: 05-psg-raw-audio
plan: 05
subsystem: audio-judge-hardening-audio-check-fault-sweep-golden-role
tags: [aud, audio-check, aexp, kindclass, disputed, rules, lint, fault-sweep, judge-forms, golden, hash-not-verdict, selftest, w4]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-04 the audio kind (EV and ROWS forms), psg_tone.aexp (scene tone_sel0), the D-200 proof set, --audio-out
provides:
  - scripts/audio-check, scripts/lib/aexp-lex.awk, scripts/lib/aexp-rules.awk; the 58th gate stage audio-check with 22 mini-roots, generated lex controls, the exit-2 size control and 26 selftest lines
  - tests/audio/KINDCLASS (29 kinds, ceiling and form), DISPUTED (10 points), RULES (104 clauses); tests/conformance/AUDIO_TOY_EXEMPT (header only)
  - audiokinds.c: every KINDCLASS kind parsed, REL and STRUCT forms, live, golden rows and the golden role, kinds audio_fault_sweep, audio_judge_forms, audio_hash_not_verdict
  - psg_tone.aexp: STRUCT header and count groups, the golden scene tone_golden; cases audio_fault_sweep_tone_sel0, audio_judge_forms, audio_tone_golden, audio_hash_not_verdict
affects: [05-06, 05-07, 05-08, 05-09, 05-10]

requirements-completed: []
requirements-advanced: [AUD-03]

actuals:
  tokens: 45200
  tasks: 3
  commits: 4
plan_head_before: ba53c3bd85c22f99c0bb65652c54226a4431bda9
plan_head_after: 2c9dfa02d561b24a87497b950244a37ce40a0c3f

tech-stack:
  added: []
  patterns:
    - "A rule map as data: tests/audio/RULES lists every clause with row (kinds or case:ID), limit (a named reason) or later:05-NN; audio-check fails a row clause whose kinds have no group with a kill other than AF0"
    - "Faults on copies: the sweep copies the captured streams once per fault, acts on the group's own window and event record, and prints the observed kill matrix of all ten faults beside the declared one"
    - "An exit-2 control outside run_controls (the pf_empty_path_control pattern) for a fail-closed size cap, with its own selftest whose alive condition is exit 2"

key-files:
  created:
    - scripts/audio-check
    - scripts/lib/aexp-lex.awk
    - scripts/lib/aexp-rules.awk
    - tests/audio/KINDCLASS
    - tests/audio/DISPUTED
    - tests/audio/RULES
    - tests/audio/ (22 mini-roots)
    - tests/conformance/AUDIO_TOY_EXEMPT
    - tests/conformance/cases/audio_fault_sweep_tone_sel0.toml, audio_judge_forms.toml, audio_tone_golden.toml, audio_hash_not_verdict.toml
  modified:
    - scripts/check (stage, ac_gen_limits, ac_oversize_control, selftests z7e..z7h)
    - scripts/fixture-build (write_aexp passes golden rows; deviation 3)
    - tests/conformance/audiokinds.c, runner.c, runner.h, REQUIRED
    - fixtures/src/psg_tone.aexp
    - docs/ci-timings.md

key-decisions:
  - "05-05: KINDCLASS carries a third column, the form of each kind, so audio-check and the runner refuse a group whose form is not its kind's"
  - "05-05: disputed points are found in the row (f=0, flf=0, nf=0, reg=RA..RF, an R0 value with select 6 or 7, S = 15) or marked by the author with the literal item mark=MARKER (held, keyon, latch_off, lfo_wrap need the mark)"
  - "05-05: a golden scene's digests live in a golden row (golden SCENE pcm_sha256=HEX psg_sha256=HEX), the .vexp precedent; header groups take class DEF:<ledger ids> (ER-19)"
  - "05-05: kinds whose judge lands later (05-06 levels_exact; 05-07 dda, noise and cycle_phase kinds; 05-09 lfo kinds) are parsed and refused as harness_failure when judged, never a pass"

duration: about 2 h 10 min wall (gates at load 9.5 to 19.6)
completed: 2026-11-13
---

# Phase 5 Plan 05: Judge hardening - the audio-check stage, the AF0..AF9 fault sweep, the golden role Summary

**Every .aexp literal is now checked statically by the 58th gate stage, audio-check: its grammar, its class against its kind's ceiling, its disputed points, its own arithmetic, its coverage of the rule map, its kills, its pairing and its hash role.
The sweep shows on the captured streams that every declared kill flips its group (39 of 39), the judge forms and null-synth controls show every form able to fail, and a faulted stream that passes its own golden fails the literal groups.**

## Performance

- Tasks: 3 of 3; no stop-rule split (05-11..05-15 stay free).
- 74 files changed in ba53c3b..2c9dfa0 (2,969 insertions, 42 deletions); 362 conformance cases (358 + 4), all pass.

## Accomplishments

- Task 1 (ddbfcda): scripts/audio-check (the video-check contract, `AEXP_MAX_BYTES=32768` checked before parsing, exit 2), aexp-lex.awk, aexp-rules.awk with 21 tagged rules; KINDCLASS, DISPUTED, RULES; 22 mini-roots (20 negative, ok-audio, ok-inventory); stage_audio_check right after video-check (57 -> 58), ac_gen_limits with ok-size-cap (32,768 bytes, must pass),
  ac_oversize_control (32,769 bytes, exit 2), selftests z7e (3 D-207 mutations), z7f (17 rules), z7g (5 lex lines), z7h (the size cap).
  The kind audio_fault_sweep and its case were pulled into this commit (deviation 1).
- Task 2 (bd6a7a2): the full grammar in the runner; REL (period_ratio, same, atten_order, atten_equal), STRUCT (header, count), ROWS live; audio_judge_forms (81 checks); STRUCT header (DEF) and count (ASSUMED) groups in tone_sel0.
- Task 3 (cfd9b9e): the golden role and audio-hash-only; tone_golden with the host-recomputed digests; audio_hash_not_verdict.
- Fix (2c9dfa0): bounded `%.Ns` widths found by the gcc 13 Linux gate (deviation 5).

## The observed kill matrix (audio_fault_sweep_tone_sel0, E0; RESEARCH section 7 was a guide)

| Group | Declared | Observed (all ten faults) |
|---|---|---|
| ev_value_sel0 | AF0,AF8 | AF0,AF8 |
| ev_decode_sel0 | AF0,AF8,AF9 | AF0,AF8,AF9 |
| period_L, period_R | AF0,AF2,AF4,AF5,AF6 | AF0,AF2,AF4,AF5,AF6,AF7,AF8,AF9 |
| cycle_order_L, cycle_order_R | AF0,AF5,AF6,AF7 | AF0,AF5,AF6,AF7,AF8,AF9 |
| silent_pre_L, silent_pre_R | AF0,AF6 | AF0,AF2,AF6,AF8,AF9 |
| silent_poweron_L, silent_poweron_R | AF0,AF6 | AF0,AF6 |
| header (new) | AF0,AF2,AF4,AF8 | AF0,AF2,AF4,AF8 |
| count (new) | AF0,AF1,AF2,AF4 | AF0,AF1,AF2,AF4 |

No 05-04 declaration was wrong, so none was changed. Unlike the guide, AF7 also flips period (the 32-sample reversal moves the rising crossings of a 256-sample cycle) and AF8 and AF9 flip every anchored window (the anchor record is dropped or relabelled). Declarations stay as written; adding the extra observed kills is left to the authors of later scenes.

## Red evidence

| Control | Result |
|---|---|
| 20 negative mini-roots of tests/audio, each with exactly its code | red as listed (stage audio-check) |
| generated lex controls (line, ASCII, SPDX, 513 rows) and the 32,769-byte control | red; the oversize one exits 2 with AEXP_FILE_TOO_LARGE |
| selftest: each tagged rule deleted from a scratch copy | 26 lines PASS (the deleted rule leaves its control red) |
| INV_AUDIO_HASH_ONLY on a scratch copy of psg_tone.aexp with every tone_sel0 row deleted | `INV_AUDIO_HASH_ONLY` at the tone_golden scene row, exit 1 (shown in Task 3, scratch only) |
| audio_judge_forms null synth (no frame, all 0, constant 12345) | every period and cycle_order group of tone_sel0 red, every silent group has a red partner on its side |
| audio_hash_not_verdict | the AF7 copy passes its own golden; period_L, period_R, cycle_order_L, cycle_order_R red; audio-hash-only refuses a golden with no literal scene and a second golden |

## The RULES later: list (104 clauses: 40 row, 6 limit, 58 later)

- later:05-06 (20): aud03-b, -c, -e, -g, -h, aud04-a, -b, -d, aud08-b, -c, -d, aud09-c, -d, aud12-e, d188-b, d189-b, d189-c, d190-c, d194-a, d194-b.
- later:05-07 (22): aud02-b, aud04-e, -f, -h, aud05-c, aud06-b, aud09-b, aud10-a..-d, aud12-b, -c, -d, d184-c, d185-a, d190-b, d191-c, d191-d, d193-a, d220-a, rf5-a.
- later:05-09 (16): aud02-c, aud04-g, aud06-c, aud07-b, aud11-a..-d, d184-d, d184-e, d192-a, d220-b, d219-a..-c, rf4-a.
- limit (6): aud01-c and d182-b (no-measurement), aud07-a and d184-b (disputed-zero), aud09-e (huc6280-only), aud14-c (no-filter).

## Gate results (one at a time)

| Run | Result | Wall | User CPU | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check --timing` before the Task 1 commit | PASS, 58 stages | 674.6 s | 463.0 s | 19.6 to 10.1 |
| `sh scripts/check --timing` before the Task 2 commit | PASS, 58 stages | 460.8 s | 379.1 s | 9.5 to 9.6 |
| `sh scripts/check --timing` before the Task 3 commit (the plan-end gate) | PASS, 58 stages, CHECK RESULT PASS | 447.9 s | 365.3 s | 17.6 to 15.2 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 203 PASS lines (202 rows and the RESULT line), 0 FAIL | 138.2 s | 49.7 s | |
| `sh scripts/audit-history --range ba53c3b..HEAD` | clean (4 commits, 77 blobs, 2,188 blob-path pairs; re-run after the fix commit) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | FAIL first (format-truncation in audiokinds.c), PASS after the fix 2c9dfa0 | 76 s | | |

After the fix commit: conformance (362 pass) and sanitizers PASS locally. After the plan-end gate only docs/ci-timings.md changed; docs-claims, ci-timings-controls, rights-tree and licences were re-run green.
Task verifies, each as written: VERIFY1_OK, VERIFY2_OK, VERIFY3_OK.

Cut trigger (3): not judged. No quiet-machine run was possible (load 9.5 to 19.6 all afternoon); the lowest-load full run took 379.1 s user CPU at load 9.5, against the 337.5 s threshold set at load 5.7 to 9.1. 05-08 still needs a quiet measurement.

## Live budget table (after 05-05)

| Budget | Value after 05-05 | Phase 5 target |
|---|---|---|
| Gate stages | 58 (done) | 58 |
| Selftest PASS lines | 203 (202 rows) | >= 178 |
| Power-on policies | 5 (done) | 5 |
| Constants rows / CROWS_MAX | 49 / 64 (done) | 49 / 64 |
| Withheld ids | 42 (done) | 42 |
| Record rows (skeleton / 16 events / cap) | 175 / 191 / 192 | 175 / 191 / 192 |
| Regions; serial / epoch / text | 24; 9 / 6 / 8 (done) | 24; 9 / 6 / 8 |
| Golden bytes (256k / 384k) | 8,557 / 8,601 of 16,384 | below 16,384 |
| .aexp bytes / rows (psg_tone) | 14,550 / 76 | below 32,768 / 512 |
| OX_ROMS_MAX / guest ROMs | 48 / 37 | 48 / 39 |
| DSV ids | 1..27 | 27, 28, 29 |
| Toys | 28 | 33 |
| Ledger entries / REQUIRED lines | 75 / 75 | 75 / 75 |
| Conformance REQUIRED cases | 362 (358 + 4) | about 379 to 394 |
| RULES clauses (row / limit / later) | 104 (40 / 6 / 58) | every later: moved to row by 05-10 |
| CI critical path | unverified (no push) | about 587 to 612 s predicted |
| Local warm gate user CPU | 365.3 s at load 15 to 18 (not comparable) | trigger (3) above 337.5 s at a comparable load |

## Task Commits

1. Task 1: the audio-check stage and the sweep kind - `ddbfcda` (feat)
2. Task 2: the full grammar, REL and STRUCT forms, judge forms - `bd6a7a2` (feat)
3. Task 3: the golden role and audio_hash_not_verdict - `cfd9b9e` (feat)
4. Fix: bounded widths for gcc 13 - `2c9dfa0` (fix)

## Deviations from Plan

**1. [Rule 3 - blocking] The sweep kind and its case moved from Task 2 into Task 1.** Task 1's AUDIO_SCENE_NO_SWEEP rule (the inventory link) needs a REQUIRED audio_fault_sweep case for tone_sel0, and the full gate before the Task 1 commit needs that case to pass, so kind_audio_fault_sweep, audio_fault_sweep_tone_sel0 and its REQUIRED line landed in ddbfcda.
The rest of Task 2 (grammar, forms, judge forms, STRUCT groups) stayed in Task 2.

**2. [Rule 2 - design within scope] Grammar additions the plan text did not spell out.** KINDCLASS gains a third column (the form of each kind); a literal key `mark=` carries the author's DISPUTED markers (the word markers held, keyon, latch_off, lfo_wrap have no other place in a row); a row kind `golden SCENE pcm_sha256= psg_sha256=` holds the digests (the .vexp precedent);
header groups take `class=DEF:LED-AUD-0013+LED-AUD-0014`; selector items `stream=dspa|dspe` (header rows) and `nth=N+M` on the end anchor (PR-13's `[+mclk]`). `side` is a selector item only (not a literal key).
Extra codes beyond the plan list: AUDIO_EXEMPT_FORMAT (a malformed AUDIO_TOY_EXEMPT line) and a rule-data-format tag (malformed KINDCLASS, DISPUTED or RULES lines report AEXP_FORMAT), each with its mini-root.

**3. [Rule 3 - blocking] scripts/fixture-build is outside files_modified.** write_aexp shaped only the aexp, scene, group and row kinds, so the golden row of Task 3 was refused; it now passes golden rows of 4 fields (one line and its comment). Commit cfd9b9e; the full gate ran before that commit.

**4. [Scope] Kinds judged later are parsed, not judged.** "Every row kind of KINDCLASS parsed" holds; the judges of levels_exact (05-06), the dda, noise, cycle_phase and cycle_linear kinds (05-07) and the lfo kinds (05-09) land with their rows. Judging one now is harness_failure with the plan named, never a pass (audio_judge_forms checks this for two_level and noise_ratio).
AUDIO_TOY_NO_LITERAL counts a toy of src/core/psg.c as covered when it kills a case of an audio kind (audio, audio_fault_sweep, aud_noninterference, psg_dev_join, snapshot_restore_audio, aud_poweron_policy_diff, psg_fold, pcm_cadence); psg-emit-mutates kills aud_noninterference.

**5. [Rule 1 - bug] gcc 13 -Wformat-truncation in kind_audio_fault_sweep.** The plan-end LINUX_REPRO_PARTS=gate-gcc build failed on the audio_sweep evidence line; fixed with %.95s and %.40s in the separate commit 2c9dfa0 (no history rewritten); gate-gcc PASS after it.

**6. Commit message counts.** ddbfcda's message says "100 clauses" and "25 mini-roots"; the committed tree held 101 clauses and 22 mini-roots (104 clauses after Tasks 2 and 3). History is not rewritten; this SUMMARY carries the true numbers.

**7. README unchanged.** The plan asked to name the stage wherever README documents the stage list or count; README documents neither (it names `sh scripts/check` and its flags only), so nothing changed there; docs/ci-timings.md names the 58th stage.

Requirement ticks: none in the plan commits; any AUD tick made by the plan-complete step is reverted with a file-specific checkout of .planning/REQUIREMENTS.md (only 05-10 completes AUD ids).

## Known Stubs

- The judges of the kinds of 05-06, 05-07 and 05-09 (deviation 4) are parsed placeholders that refuse with harness_failure; each plan adds its judge with its rows. None blocks this plan's goal.

## Threat Flags

None beyond the plan's threat model: T-05-18 (one strict lexer, closed keys, the 32,768-byte cap before parsing, a mini-root per code), T-05-19 (KINDCLASS, DISPUTED, the S rules and the lint, each deletable only with the selftest red), T-05-20 (INV_AUDIO_HASH_ONLY, audio-hash-only, audio_hash_not_verdict, digests recomputed by the host tool),
T-05-21 (the sweep fails a non-flipping kill and a vacuous group; null-synth controls red).

## Notes for next plans

- 05-06: run `sh scripts/check --stage audio-check` before each commit; every new group needs a kind of KINDCLASS with the right form, a class inside the ceiling (P8-10 is in the period and live sets; atten_order allows P8-3+P8-4+P8-10), r1/al/r5 inputs and a side so the lint computes S, and a MAN live/period/cycle_order row only at S = 0 or 4.
  ASSUMED rows at S = 1 (C1, C2) are fine. A sum-15 silent row must be ASSUMED (it is found automatically); mark the select-6 and reg-$A ev_edge rows only if the anchor or literal does not already carry R0 $06 or reg=RA (both are found).
- 05-06: implement the levels_exact judge (ROWS; literal key to choose, add it to au_keys in audiokinds.c and litkey in aexp-rules.awk together). Every literal scene needs exactly one REQUIRED audio_fault_sweep case (AUDIO_SCENE_NO_SWEEP) and every scene case must be a REQUIRED audio case naming the scene; the twin scene is named <scene>_twin with twin= and expect_red= on its scene row
  (the runner reads the red list there; its case carries no expect_red).
- 05-06: when psg_tone's streams change, regenerate tone_golden with `ds_runner --cases <flattened cases> --audio-out audio_tone_sel0` and `shasum -a 256` of build/gate/audio/audio_tone_sel0.dspa and .dspe, and record old and new digests on its comment line. Move each covered RULES clause from later:05-06 to row (audio-check then requires a group of the listed kinds with a kill other than AF0).
- 05-06: the sweep acts on the window of a group's first windowed row and on that row's anchor record; REL groups are judged over all their rows. Its observed column is printed; declare only kills you expect, the sweep fails any declared kill that does not flip.
- psg_tone.aexp: 14,550 bytes and 76 rows after 05-05; the 05-06 projection is about 30.5 KB of 32,768 (one comment line per group).
- Cut trigger (3) still needs a quiet-machine run (05-08). The scripts/check line naming v3 for the guest list (carried from 05-03) was not touched.

## Owner items

- None new. Carried: lever (a) commit 9619efe awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

Files: scripts/audio-check, scripts/lib/aexp-lex.awk, scripts/lib/aexp-rules.awk, tests/audio/KINDCLASS, DISPUTED, RULES, the 22 mini-roots, tests/conformance/AUDIO_TOY_EXEMPT and the four new case files exist. Commits ddbfcda, bd6a7a2, cfd9b9e and 2c9dfa0 are ancestors of HEAD.
