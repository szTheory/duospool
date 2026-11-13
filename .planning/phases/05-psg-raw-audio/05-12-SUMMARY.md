---
phase: 05-psg-raw-audio
plan: 12
subsystem: psg-event-rows-and-aexp-cap
tags: [aud, psg_tone, dspe, ev_decode, ev_edge, mask-probe, aexp-cap, d-233, golden, toys, w5, split-of-05-06]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-06 psg_tone with the pan-pair, attenuation and twin scenes and the split proposal of Task 3b; D-233 (the cap decision)
provides:
  - the .aexp reader cap of 49,152 bytes (AEXP_MAX_BYTES) with a 49,153-byte exit-2 control and an exactly-49,152-byte pass control
  - psg_tone event rows, the ev_decode groups ev_1..ev_5 and ev_probe, the ev_edge group edge and the ev_value group edge_v
  - sc_probe (R8, the R9 pair and the mask probe) after tone_pan01, sc_edge after tone_atten, R7 $78 and R2 rewrites in the tone_pan45 set-up
affects: [05-07, 05-08, 05-10]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-03]

actuals:
  tokens: 8600
  tasks: 2
  commits: 3
plan_head_before: fe94864a2a8e866b6cdf94ed915dc11f3fcfefac
plan_head_after: 10dd0d1c72f98d75ce54b97de457e70bd8d5e669

tech-stack:
  added: []
  patterns:
    - "Decode rows anchor on a G R0 record by value and count, with ev=+K into the set-up block that follows it, so a toy that moves channel writes changes no anchor"
    - "An R2 row is taken only after an R3 the program wrote, so its post never rests on the power-on F (ER-10)"
    - "Disputed edges (select 6 and 7, reg $A, R7 at select 0..3) live in one ev_edge group of ASSUMED rows with explicit mark= items"

key-files:
  created:
    - .planning/phases/05-psg-raw-audio/05-12-PLAN.md
  modified:
    - scripts/audio-check, scripts/check, scripts/lib/aexp-lex.awk, docs/ci-timings.md, .planning/phases/05-psg-raw-audio/05-HANDBOOK.md
    - fixtures/src/psg_tone.s, psg_tone.aexp; fixtures/manifest/psg_tone.toml
    - tests/audio/RULES; tests/controls/psg-chsel-mask/RULE, EXPECT; tests/conformance/cases/audio_tone_pan01.toml, audio_tone_pan45.toml, audio_tone_atten.toml

key-decisions:
  - "05-12: AEXP_MAX_BYTES is 49152 (D-233); the oversize control pads to 49,153 bytes and ok-size-cap to exactly 49,152; the C reader has no byte cap and is unchanged"
  - "05-12: every new R2 decode row follows a program-written R3 (probe R2 $3C, the tone_atten set-up and Y, an R2 rewrite after R7 $00 in tone_pan45), because a first R2 after power-on posts the power-on F high nibble, an ASSUMED input"
  - "05-12: the disputed edges are one ASSUMED ev_edge group in tone_atten with mark= items; $080A is written at select 3 so a reg & 7 mis-decode would name a channel"

duration: about 1 h 30 min wall (gates at load 9 to 19)
completed: 2026-11-13
---

# Phase 5 Plan 12: The psg_tone event rows and the D-233 cap Summary

**The .aexp reader cap rises to 49,152 bytes with both size controls retargeted, and psg_tone's DSPE log now carries hand-written decode rows for R2..R6 of every select 1..5, R7 at selects 4 and 5, R1, R8 and R9 and a mask probe, plus ASSUMED rows for the disputed edges (the mirror, $080A, R7 at select 3, selects 6 and 7). Toy psg-chsel-mask now also fails the select 4 and 5 decode rows.**

## Performance

- Tasks: 2 (plan 05-12 is Task 3b of 05-06-PLAN.md, written as its own plan file first). 15 files changed in fe94864..10dd0d1 (291 insertions, 44 deletions), plus this SUMMARY and the state files.
- psg_tone runs 9,747,960 master clocks to DSV1 (was 9,726,036; about 27.3 frames of 262 lines); until_mclk 12,000,000 still holds. 373 REQUIRED conformance cases, all pass.

## Accomplishments

- Plan file (1e246d3): 05-12-PLAN.md in the shape of 05-06-PLAN.md (frontmatter, objective, stop_rule, two tasks, threat model).
- Task 1 (b6aa6db): D-233. AEXP_MAX_BYTES=49152 in scripts/audio-check. In scripts/check, ok-size-cap is exactly 49,152 bytes and must pass; ac_oversize_control pads to 49,153 bytes and wants exit 2 with an AEXP_FILE_TOO_LARGE line. The z7h comment, aexp-lex.awk, docs/ci-timings.md and a handbook D-233 line follow. No control was removed.
- Task 2 (10dd0d1): the event rows, the scripts that feed them, the RULES moves, the toy evidence, the regenerated golden and the re-pinned manifest.

## The decode rows (D-196, written by hand; fields chan, effect, wave_idx, post)

| Group (scene) | Class | Records (anchor, then ev=+K) and literals |
|---|---|---|
| ev_1 (tone_pan01) | MAN:P8-2+P8-3+P8-4 | 2nd R0 $01: +1 R4 $40 (1, $08, 0, $0040), +2 R4 $00 (1, 0, 0, 0), +3 first R6 (1, $01, 1, $0017), +36 R3 $00 (1, 0, 0, $00C0), +37 R1 $FF ($FE, 0, $FF, $00FF); 4th R0 $01 +3 R5 $0F (1, 0, 0, $000F) |
| ev_probe (tone_pan01) | MAN:P8-2+P8-3+P8-4+P8-6 | 1st R8 $00: R8 ($FE, 0, $FF, 0), R9 $80 ($FE, $10, $FF, $0080), R9 $00 ($FE, 0, $FF, 0), R0 $F9 ($FE, 0, $FF, $0001), R3 $F1 (1, 0, 0, $01C0), R3 $00 (1, 0, 0, $00C0), R2 $3C (1, 0, 0, $003C), R4 $3F (1, 0, 0, $001F), R6 $E5 (1, $01, 1, $0005), R9 $7C ($FE, 0, $FF, 0) |
| ev_4 (tone_pan45) | MAN:P8-2+P8-3+P8-4+P8-6 | 2nd R0 $04: +0 ($FE, 0, $FF, $0004), +1 R4 $40 (4, $08, 0, $0040), +2 R7 $78 (4, 0, 0, $0018), +3 R4 $00, +4 first R6 (4, $01, 1, $0015), +37 R3 $01 (4, 0, 0, $0180), +38 R7 $00, +39 R2 $80 (4, 0, 0, $0180); 4th R0 $04 +3 R5 $F0 (4, 0, 0, $00F0) |
| ev_5 (tone_pan45) | MAN:P8-2+P8-3+P8-4+P8-6 | 2nd R0 $05: +0 ($FE, 0, $FF, $0005), +1 R4 $40, +2 R4 $00, +3 first R6 (5, $01, 1, $001A), +36 R3 $01 (5, 0, 0, $01C0), +37 R7 $00, +38 R2 $C0 (5, 0, 0, $01C0); 4th R0 $05 +3 R5 $0F (5, 0, 0, $000F) |
| ev_2 (tone_atten) | MAN:P8-2+P8-3+P8-4 | 7th R0 $02 (the tone_atten set-up): +1 R4 $40, +2 R4 $00, +3 first R6 (2, $01, 1, $0013), +35 R2 $00 (2, 0, 0, $0100), +36 R3 $01 (2, 0, 0, $0100), +38 R5 $FF (2, 0, 0, $00FF) |
| ev_3 (tone_atten) | MAN:P8-2+P8-3+P8-4 | 7th R0 $03 (Y): +1 R4 $40, +2 R4 $00, +3 first R6 (3, $01, 1, $001C), +35 R2 $40 (3, 0, 0, $0140), +36 R3 $01 (3, 0, 0, $0140), +37 R5 $FF (3, 0, 0, $00FF) |
| edge (tone_atten) | ASSUMED:LED-AUD-0003+LED-AUD-0004 | 8th R0 $03 (sc_edge): +1 R5 $FF via $0815 (3, 0, 0, $00FF), +2 $080A (NO_TARGET; mark reg=RA), +3 R7 at select 3 (NO_TARGET), +4 R0 $06 ($FE, 0, $FF, $0006; sel=6), +5 R2 at select 6 (NO_TARGET; sel=6), +6 R0 $07 ($FE, 0, $FF, $0007; sel=7), +7 R6 at select 7 (NO_TARGET; sel=7); NO_TARGET = ($FF, $20, $FF, $FFFF) |
| edge_v (tone_atten) | GUEST:program | the same anchor: +1 reg R5 value $FF, +2 reg RA value $A5 (reg = offset & $0F) |

- Every literal passed against the core on its first run; no literal was changed after a run.
- Every declared kill flipped: AF0, AF8 and AF9 for seven ev groups, AF0 and AF8 for edge_v. The sweep cases report 54, 55, 54, 48 and 39 declared kills flipped (tone_pan01, tone_pan45, tone_atten, tone_pan23, tone_sel0).
- The tables' word 0 values ($17, $13, $1C, $15, $1A) are the formula values high[0] = 16 + b of the psg_tone.s header, not core output.

## Toy evidence (R5)

| Toy | Red (guest_failure) | Stays pass (F-4) |
|---|---|---|
| psg-chsel-mask (R0 & 3) | audio_tone_pan45: its 10 PCM groups (anchors missing) and, new, ev_4 (row 2: record 290 reads chan 0, the literal is 4) and ev_5 (row 2: record 330 reads chan 1, the literal is 5) | audio_tone_pan01 with ev_1 and ev_probe green; audio_tone_sel0 |
| psg-atten-max | audio_tone_atten (unchanged) | audio_tone_pan01 with ev_1 and ev_probe green; audio_tone_sel0 |

Under psg-chsel-mask the R0 $04 record still posts $0004 (row 1 of ev_4 passes); the rows that turn red are the channel fields of the set-up records.

## The golden (ER-20, D-209)

Regenerated once, after the literal scenes were green, with `ds_runner --cases build/gate/cases.tsv --audio-out audio_tone_sel0` and `shasum -a 256` of the .dspa and .dspe files. The host digests equal the runner's and the audio_tone_golden evidence.
- pcm 9f3e65e7..39e6c -> 0d3aeebfd556684144745a8f28ff539a2cb59ecb730a250c75e2f678e47dca37 (50,771 frames)
- psg 76f6d249..ff7a1 -> bfa5267327746e6df9ae6cf688730b45b92e35da924dea2d62643ff5f21a7869 (626 records)

Both pairs are recorded on the golden comment line of psg_tone.aexp. The manifest sha256 moved from 1c20a012..35e50 to 744fbf6e..09fe8b (size 262,144; repeatability only).

## Gate results (one at a time)

| Run | Result | Wall | User CPU | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check --stage audio-check`, `--selftest mutation-audio-file-too-large` (Task 1 narrow verify) | PASS, PASS | 15.4 s, 0.7 s | | 13.8 |
| `sh scripts/check --timing` before the Task 1 commit | PASS | 513.2 s | 392.9 s | 15.4 to 18.6 |
| `sh scripts/check --stage conformance` (Task 2, before the golden) | only audio_tone_golden guest_failure (expected: the streams changed) | | | |
| `sh scripts/check --stage conformance` (Task 2, after the golden) | PASS, 373 of 373 | | | |
| `sh scripts/check --timing` before the Task 2 commit (the plan-end gate) | PASS, 59 CHECK PASS lines (conformance 98 s, sanitizers 198 s) | 472.8 s | 371.9 s | 10.6 to 10.5 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 205 PASS lines (204 rows and the RESULT line) | 93.0 s | 36.6 s | 8.7 to 9.7 |
| `sh scripts/audit-history --range fe94864..HEAD` | clean (3 commits, 15 blobs, 2,206 blob-path pairs) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | PASS (no C file changed in this plan) | 85 s | | |

- Each task's verify passed as written. Task 2's grep loop and CONTROL psg-chsel-mask RED_OK were read from the plan-end gate's results.txt and controls.txt.
- The user CPU figures were taken at load 9 to 19 and are not comparable with the 05-01 baseline. Cut trigger (3) still needs a quiet run (05-08).

## Live budget table (after 05-12)

| Budget | Value after 05-12 | Phase 5 target |
|---|---|---|
| Gate stages | 58 (done) | 58 |
| Selftest PASS lines | 205 (204 rows) | >= 178 |
| Constants rows / CROWS_MAX | 49 / 64 (done) | 49 / 64 |
| Withheld ids | 42 (done) | 42 |
| .aexp cap | 49,152 bytes (D-233), 512 rows | below the cap |
| .aexp bytes / rows (psg_tone) | 40,664 / 190 (74 groups) | below 49,152 / 512 |
| OX_ROMS_MAX / guest ROMs | 48 / 37 | 48 / 39 |
| DSV ids | 1..27 | 27, 28, 29 |
| Toys | 30 | 33 |
| Conformance REQUIRED cases | 373 | about 379 to 394 |
| RULES clauses (row / limit / later) | 104 (59 / 6 / 39: 23 later:05-07, 16 later:05-09) | every later: moved to row by 05-10 |
| psg_tone run | 9.75 M master clocks (27.3 frames) | about 40 frames |
| Local warm gate user CPU | 371.9 to 392.9 s at load 10 to 19 (not comparable) | trigger (3) above 337.5 s at a comparable load |

## Task Commits

1. Plan file: `1e246d3` (docs)
2. Task 1: D-233, the cap 49,152 and both controls: `b6aa6db` (feat)
3. Task 2: the event rows, probe, edge script, RULES, toy evidence, golden: `10dd0d1` (feat)

## Deviations from Plan

The plan file was written by this executor from 05-06 Task 3b and its split proposal; it was not plan-checked. The items below differ from 05-06 Task 3b and its split proposal. Each stays inside the clauses the task covers, and 05-12-PLAN.md already states them.

**1. [Rule 2 - evidence class, ER-10] R2 rows never rest on the power-on F.**
- The split proposal took the R2 posts $000, $040, $080 and $0C0 from the first set-up writes of selects 2..5.
- Those posts carry the high nibble of the power-on F (D-204, ASSUMED LED-PROF-0003), so a MAN row would overstate its class.
- Instead the R2 rows come from writes that follow a program-written R3:
  - select 1: the probe adds R2 $3C after R3 $00 (post $003C);
  - selects 2 and 3: tone_atten's set-up and Y, whose R2 follows tone_pan23's R3 $01 ($100, $140);
  - selects 4 and 5: the tone_pan45 set-up writes R2 again after R7 $00 ($180, $1C0). F does not change.
- Pre-existing and not changed (out of scope): ev_decode_sel0 (05-04) has an R2 row (post $0000) that rests on the power-on F under a MAN class. Noted for 05-10.

**2. [Rule 2 - completeness within the clauses] Edge rows beyond the plan's three.**
- Added: R7 $9F at select 3 (the converse of "R7 only at select 4 and 5", ASSUMED); R0 $07 with R6 $1A (the select-7 half of aud04-b and d189-b); rows for the R0 $06 and $07 records themselves; and the ev_value group edge_v (reg = offset & $0F for $0815 and $080A).
- The $080A write is made at select 3, not 6. A reg & 7 mis-decode then names channel 3 instead of passing as a dropped select-6 write.

**3. [Scope] No new key-on rows.** R4 $9F records get no new MAN rows, because the key-on is a disputed point (marker keyon). The R4 $40, $00 and $3F rows show R4 decoding.

**4. [Scope] Select 2 and 3 decode rows sit in scene tone_atten**, where their records are, not in tone_pan23. The edge groups are also in tone_atten. No new scene, case or REQUIRED line was needed.

**5. [Files] The handbook changed** (its own rule allows a recorded deviation): a D-233 line and the cap row of the budget table. docs/ci-timings.md and aexp-lex.awk carry the cap figure in comments. All are listed in 05-12-PLAN.md.

**6. [Note] aud12-e** ("volume writes act at their stamp") moves to row with NEED atten_order, as planned. Its TEXT states that the tone_atten windows open 2,000 master clocks after the write, which is a bound, not the exact instant.

Requirement ticks: git diff .planning/REQUIREMENTS.md showed no change after the plan-complete step (requirements.mark-complete was not called), so nothing was reverted; 05-10 completes the AUD ids. state.advance-plan set Current Plan to 8; it was set back to 7 (05-07 is next) and Total Plans in Phase to 11 by hand. .planning/state.json, rewritten by the tooling, was restored to its original bytes and is not committed.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model:
- T-05-12a: the cap is one constant with both controls and the removal selftest.
- T-05-12b: the disputed edges are ASSUMED rows, and the lint refuses a MAN group that carries a marker.
- T-05-12c: no R2 row rests on the power-on F.
- T-05-24: the golden was regenerated once with the host tool, with old and new digests recorded.

## Notes for next plans

- 05-07:
  - psg_tone.aexp is 40,664 bytes and 190 rows (74 groups), so 8,488 bytes remain under the 49,152 cap. 05-07's psg_mod.aexp budget of 32,768 is stricter than the cap and still applies as written.
  - The decode rows anchor on G R0 records by value and count. A new R0 write of one of these values earlier in psg_tone shifts the count:
    - ev_1: the 2nd and 4th R0 $01;
    - ev_4, ev_5: the 2nd and 4th R0 $04 and $05;
    - ev_2: the 7th R0 $02; ev_3: the 7th R0 $03;
    - edge, edge_v: the 8th R0 $03;
    - ev_probe: the 1st R8 $00.
  - The tone_pan45 set-up now writes R7 $78, R7 $00 and R2 again at select 4, and R7 $00 and R2 again at select 5 (NE stays 0). If 05-07 touches tone_pan45, keep those records or re-anchor ev_4 and ev_5.
  - The psg-tone cases keep until_mclk 12,000,000 (DSV1 done at 9,747,960).
- The golden tone_golden is now pcm 0d3aeebf..dca37, psg bfa52673..a7869.
- RULES: 59 row, 6 limit, 23 later:05-07, 16 later:05-09.
- 05-10: review ev_decode_sel0's R2 row (deviation 1).
- Cut trigger (3) still needs a quiet-machine run (05-08).

## Owner items

- None new. The orchestrator decided D-233 before this plan.
- Carried: lever (a), commit 9619efe, awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

The plan file, this SUMMARY and every modified file exist. Commits 1e246d3, b6aa6db and 10dd0d1 are ancestors of HEAD.
