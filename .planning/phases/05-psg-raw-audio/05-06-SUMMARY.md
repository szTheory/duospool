---
phase: 05-psg-raw-audio
plan: 06
subsystem: psg-six-channels-pan-pairs-attenuation-twin-toys
tags: [aud, psg_tone, pan-pairs, attenuation, levels_exact, psg_vol_table, psg_pure_function, twin, toys, golden, w5, split]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-05 the audio-check stage, the AF0..AF9 sweep, the REL and STRUCT forms, the golden role
provides:
  - psg_tone grown with a script player, six channel tables and the scenes tone_pan01, tone_pan23, tone_pan45, tone_atten and the in-ROM twin tone_pan01_twin; DSV checks 5..10
  - the levels_exact judge (literal key peak) in audiokinds.c and aexp-rules.awk, with audio_judge_forms rows
  - the C-level kinds psg_vol_table and psg_pure_function (psgkinds.c)
  - the toys psg-atten-max and psg-chsel-mask
affects: [05-12, 05-07, 05-08, 05-10]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-03]

actuals:
  tokens: 25900
  tasks: 3
  commits: 3
plan_head_before: 8074e8d12a84124fe5b61470df62efc2b74860de
plan_head_after: d5427e2

tech-stack:
  added: []
  patterns:
    - "A fixture script player: register scripts in RODATA (an offset and a value, a delay of n units, a TIN upload, the end), so a scene is a byte table and its twin a byte copy with named changes"
    - "Anchors with val= count only the records of one value, so a toy that moves unrelated writes between channels changes no count (the F-4 pass column of psg-chsel-mask)"
    - "Side pairs with one literal share one group of two rows, and group ids are short (per, ord, sil, lev, ao, ae), to keep a grown .aexp under the D-230 cap"

key-files:
  created:
    - tests/conformance/cases/audio_tone_pan01.toml, audio_tone_pan23.toml, audio_tone_pan45.toml and their three audio_fault_sweep cases
    - tests/conformance/cases/audio_tone_atten.toml, audio_fault_sweep_tone_atten.toml, audio_tone_pan01_twin.toml, psg_vol_table.toml, psg_pure_function.toml
    - tests/controls/psg-atten-max/RULE and EXPECT, tests/controls/psg-chsel-mask/RULE and EXPECT
  modified:
    - fixtures/src/psg_tone.s, psg_tone.checks, psg_tone.aexp; fixtures/manifest/psg_tone.toml
    - tests/conformance/audiokinds.c, psgkinds.c, runner.c, runner.h, REQUIRED; scripts/lib/aexp-rules.awk; tests/audio/RULES
    - the psg-tone cases audio_tone_sel0, audio_tone_sel0_d3, audio_fault_sweep_tone_sel0, audio_tone_golden, audio_judge_forms, audio_hash_not_verdict, psg_tone, mutate_sweep_psg_tone, chunk_equiv_audio_psg_tone

key-decisions:
  - "05-06: every pan, attenuation and twin anchor carries val=: the toy psg-chsel-mask moves the R4 $00 writes of (b) at selects 4 and 5 to selects 0 and 1, which would shift a plain nth count of tone_pan01"
  - "05-06: side pairs with one literal share a group of two rows and group ids are short (D-230); the twin's red list is 7 merged groups, the same groups as the plan's 10 per-side ids"
  - "05-06: the event rows, the mask probe and the ev_edge rows (Task 3b) are split to 05-12; psg_tone.aexp holds 32,406 of 32,768 bytes and they need about 4.3 KB, so 05-12 needs a D-230 decision first"

duration: about 2 h 15 min wall (gates at load 8 to 17)
completed: 2026-11-13
---

# Phase 5 Plan 06: Six channels, attenuation and the in-ROM twin Summary

**psg_tone now plays all six channels, each with its own 32-word table and F (128 to 448), solo at S = 0 and in single and double hard pans. A scene changes R1, R5 and AL on one playing channel to cover the P8-10 attenuation and silence rules. A mis-programmed copy of tone_pan01 fails exactly the groups its two changes predict.
Two toys show the attenuation sum and the select mask can be caught. The event rows for every register (Task 3b) do not fit the D-230 cap and move to the split plan 05-12.**

## Performance

- Tasks: 3 (Tasks 1 and 2 whole; Task 3 as 3a, with 3b split to 05-12). 35 files changed in 8074e8d..d5427e2 (794 insertions, 54 deletions). 373 REQUIRED conformance cases (362 + 11), all pass.
- psg_tone runs 9,726,036 master clocks to its DSV1 verdict (about 27.2 frames of 262 lines): tone_sel0 1.4, tone_pan01 3.4, tone_pan23 5.4, tone_pan45 7.5, tone_atten 4.1, the twin 3.4 frames, well inside the 140-frame insn-sink headroom (RF-12). Each scene stays under 16 frames (D-214).

## Accomplishments

- Task 1 (accca1d): a script player and six channel tables in psg_tone.s; the scenes tone_pan01, tone_pan23 and tone_pan45, each with sub-phases A, B (solo, S = 0), C1, C2 (one channel hard left or right) and C3 (both). Each scene has a literal case and a sweep case.
  DSV checks 5..10 sum each table to 496 in two ways. The golden is regenerated, the manifest re-pinned, and RULES aud08-c moves to row.
- Task 2 (202d5d9): tone_atten, the levels_exact judge, psg_vol_table, psg_pure_function and the toy psg-atten-max. RULES aud08-b, aud08-d, aud09-c, d194-a and d194-b move to row.
- Task 3a (d5427e2): the twin tone_pan01_twin, the toy psg-chsel-mask and the golden regenerated again. RULES aud04-d and d190-c move to row through case:psg_wave_ptr.

## Tables and the check method

Table c (select c) = high[i] = 16 + ((a x i + b) mod 16) for words 0..15 and low[i] = (e x i + g) mod 16 for words 16..31. The (a, b, e, g) values are (3, 0, 5, 15), (5, 7, 3, 2), (7, 3, 11, 9), (9, 12, 13, 4), (11, 5, 7, 14) and (13, 10, 9, 6).
A throwaway script outside the repository checked the seven tables (these six and tone_tab):
- no rotation, reversal, complement or reversed complement of a table is a rotation of itself or of another table;
- cyclically adjacent words differ;
- the only rising crossing of the threshold 15.5 is word 31 -> word 0;
- each table is a permutation of 0..31 summing to 496.
The formula and the method are in the psg_tone.s header. The words are formula values, never core output. Every period, order, level and silence literal was hand-derived; every one passed against the core on the first run, so no literal was changed after a run.

## The twin red set (D-211)

The twin is a byte copy of the tone_pan01 script with two changes: R2 $81 (select 0 at F + 1 = 129) and every select-1 key-on written with bit 7 clear ($9F -> $1F in B, $9D -> $1D in C2 and C3).
- Red (expect_red): per_0_A, per_0_C1_L, per_0_C3_L. With F = 129, four cycles take 516 samples, so the fifth crossing falls past the window and each window holds 4 crossings, not 5.
- Red (expect_red): per_1_B, ord_1_B, per_1_C2_R, per_1_C3_R. Select 1 never sounds, so these windows have no crossing.
- Green: ord_0_A (the order holds at 4 or 5 samples a step), sil_C1_R and sil_C2_L.
audio_tone_pan01 is all green on the same ROM. The run gave exactly this set.

## Toy evidence (R5)

| Toy | Red (guest_failure) | Stays pass (F-4) |
|---|---|---|
| psg-atten-max (the largest attenuation term instead of the sum) | audio_tone_atten: lev_P2 (peak 360,437 for 255,161), sil_P2_R (90,520 on R), sil_P3_R (31,020 on R); the other 11 groups pass | audio_tone_sel0, audio_tone_pan01 |
| psg-chsel-mask (R0 & 3) | audio_tone_pan45: all 10 groups (no record names channel 4 or 5) | audio_tone_pan01, audio_tone_sel0 |

## Observed kill matrices (E0)

- tone_pan01, tone_pan23, tone_pan45: 48 of 48 declared kills flip. AF3 kills every C group. AF7, AF8 and AF9 also flip the anchored period groups; these are observed, not declared.
- tone_atten: 44 of 44 declared kills flip. The observed matrix adds AF3 on the live, lev_P2 and ao groups, and AF8 and AF9 on most groups.

## Gate results (one at a time)

| Run | Result | Wall | User CPU | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check --timing` before Task 1 (first) | FAIL: rights-tree (a 423-character manifest line); chunk_equiv_audio_psg_tone (until 1,000,000 ends before DSV1) | | | 14.3 |
| `sh scripts/check --timing` before the Task 1 commit | PASS | 402.8 s | 331.5 s | 11.6 to 9.6 |
| `sh scripts/check --timing` before the Task 2 commit | PASS | 429.5 s | 354.7 s | 13.4 to 17.4 |
| `sh scripts/check --timing` before the Task 3a commit (the plan-end gate) | PASS, 58 stages (conformance 107 s, sanitizers 227 s) | 644.8 s | 450.2 s | 8.3 to 15.5 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 205 PASS lines (204 rows and the RESULT line) | 221.8 s | 67.0 s | 13.1 |
| `sh scripts/audit-history --range 8074e8d..HEAD` | clean (3 commits, 45 blobs, 2,205 blob-path pairs) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | PASS | 119 s | | |

Each task's verify passed as written: VERIFY1_OK, VERIFY2_OK, VERIFY3_OK. Cut trigger (3) was not judged: no quiet machine was available (load 8 to 17), so 05-08 still needs a quiet run.

## Live budget table (after 05-06)

| Budget | Value after 05-06 | Phase 5 target |
|---|---|---|
| Gate stages | 58 (done) | 58 |
| Selftest PASS lines | 205 (204 rows) | >= 178 |
| Constants rows / CROWS_MAX | 49 / 64 (done) | 49 / 64 |
| Withheld ids | 42 (done) | 42 |
| Golden bytes (256k / 384k) | 8,557 / 8,601 of 16,384 (unchanged) | below 16,384 |
| .aexp bytes / rows (psg_tone) | 32,406 / 136 (66 groups) | below 32,768 / 512 |
| OX_ROMS_MAX / guest ROMs | 48 / 37 | 48 / 39 |
| DSV ids | 1..27 | 27, 28, 29 |
| Toys | 30 | 33 |
| Conformance REQUIRED cases | 373 | about 379 to 394 |
| RULES clauses (row / limit / later) | 104 (48 / 6 / 50: 11 later:05-06 for 05-12, 23 later:05-07, 16 later:05-09) | every later: moved to row by 05-10 |
| psg_tone run | 9.73 M master clocks (27.2 frames) | about 40 frames |
| Local warm gate user CPU | 331.5 to 450.2 s at load 8 to 17 (not comparable) | trigger (3) above 337.5 s at a comparable load |

## Task Commits

1. Task 1: three pan-pair scenes, sweeps, golden: `accca1d` (feat)
2. Task 2: tone_atten, levels_exact, psg_vol_table, psg_pure_function, psg-atten-max: `202d5d9` (feat)
3. Task 3a: the twin, psg-chsel-mask, golden: `d5427e2` (feat)

## Deviations from Plan

**1. [Rule 3, blocking] D-230 byte cap: Task 3 is split, and the .aexp layout changed.**
- At plan time the .aexp was projected at about 30.5 KB. The rows as specified come to about 37 KB.
- To fit, these changes were made:
  - one group of two rows for each side pair that shares a literal (A and B per and ord, lev_P1, sil_P4, sil_P5);
  - short group ids;
  - a compacted header, with the derivations of the scripts and tables moved to the psg_tone.s header;
  - the twin's A and B groups carry the L row only.
- The twin's red list therefore names 7 merged groups. They are the same groups as the plan's 10 per-side ids.
- The event rows still did not fit, so Task 3b moves to 05-12 (see the split proposal below). This covers the decode rows for selects 1..5 R2..R6, R7 at selects 4 and 5 including R7 $78, R8, the R9 pair, the mask probe and the three ev_edge rows.
- Because of this, toy psg-chsel-mask names no ev_decode group yet. Its red evidence is every PCM group of tone_pan45.

**2. [Rule 1, a plan fact] Anchors carry val=; the plan said not to use it.**
- The plan assumed that tone_pan01 is judged before any select-4 or select-5 write. That is not so: step (b) writes R0 = 4 and 5 with R4 $00 at reset.
- Under psg-chsel-mask those writes land on selects 0 and 1. That would shift a plain nth count of R4 records, and the F-4 pass column of tone_pan01 would fail.
- With val=, the counts include only R4 records of one value. The twin's anchors count the twin's own bytes (select 1 R4 $1F nth 4, $1D nth 6, 7 and 9), so every twin anchor still exists.

**3. [Rule 3] Files outside files_modified.**
- These existing psg-tone cases now carry checks=10 and until_mclk 12,000,000, because the grown program needs about 9.7 M master clocks: audio_tone_sel0, audio_tone_sel0_d3, audio_fault_sweep_tone_sel0, audio_tone_golden, audio_judge_forms, audio_hash_not_verdict, psg_tone and mutate_sweep_psg_tone.
- chunk_equiv_audio_psg_tone gets the same until_mclk. With 1,000,000 its runs ended before DSV1 done, which the first gate showed as guest_failure.
- audiokinds.c and aexp-rules.awk were changed in Task 2 for the levels_exact judge and the peak key, as the binding note of 05-05 requires. The judge also gained four audio_judge_forms rows.
- The mutate sweep moved to 10 checks in Task 1 (the plan had it in Task 3), because checks 5..10 landed in Task 1.

**4. [Rule 1] Each sub-phase opens with R4 $40.** The plan's period window, +(192F + 2000) to +(1152F + 1000), holds c_2..c_6 only if the wave index is 0 at the key-on. A key-on after an earlier key-off would otherwise start at an arbitrary index, so every sub-phase resets the index first (D-190).

**5. [Scope] tone_atten is one continuous play.**
- Phases 6, 9 and 9a of the plan have the same register state as P1, so their atten_order and atten_equal rows reuse P1's window.
- P1 carries per and lev on both sides, but no ord rows: select 2's order at S = 0 is judged by ord_2_A of tone_pan23, which uses the same table, F and S.
- The live kind is used at P2. Every window anchors on the last write of its phase.

**6. RULES retargets.**
- aud09-d (a linear sum over six channels) moves to later:05-07. 05-06 judges one channel per side only; a same-side sum of two DDA levels fits 05-07.
- aud04-d and d190-c (R6 held) move to row through case:psg_wave_ptr. That case has the C-level WAVE_HELD row, and no fixture row plays a held write.
- 11 clauses stay later:05-06 for 05-12: aud03-b, aud03-c, aud03-e, aud03-g, aud03-h, aud04-a, aud04-b, aud12-e, d188-b, d189-b and d189-c. audio-check accepts later: only for 05-06..05-10.

**7. [Rule 1] The manifest meaning line was 423 characters.** rights-tree refused it in the first gate, so it was shortened to 365 characters (371 after Task 3a).

The plan-complete step's requirement ticks were checked with git diff .planning/REQUIREMENTS.md; any AUD tick it made was reverted with a file-specific checkout (05-10 completes the AUD ids).

## Split proposal: 05-12 (Task 3b, depends_on 05-06, before 05-07)

- Precondition: an orchestrator or owner decision on D-230. psg_tone.aexp has 362 bytes left, and 3b needs about 4.3 KB. Options:
  - (a) Raise the .aexp source cap, for example to 49,152. That touches the AEXP_MAX_BYTES of scripts/audio-check, the 32,769-byte exit-2 control and its selftest z7h, and needs a D-230 erratum. The runner reads the comment-free TSV, which stays near 26 KB.
  - (b) Move the event rows into a fixture whose .aexp has room. psg_mod in 05-07 is the candidate.
  - (c) Shorten the 05-04 and 05-05 comments. This is not recommended: they are derivations.
- Content once the decision is made: a probe script after tone_pan01 with these writes and their (chan, effect, wave_idx, post) literals:
  - R8 $00 ($FE, $00, $FF, $0000)
  - R9 $80 ($FE, $10, $FF, $0080), then R9 $00 ($FE, $00, $FF, $0000)
  - R0 $F9 ($FE, $00, $FF, $0001)
  - R3 $F1 (1, $00, 0, $01C0), then R3 $00 (1, $00, 0, $00C0)
  - R4 $3F (1, $00, 0, $001F)
  - R6 $E5 (1, $01, 1, $0005)
  - R9 $7C ($FE, $00, $FF, $0000)
- Also R7 $78 right after select 4's R4 $40 in the tone_pan45 set-up (4, $00, 0, $0018).
- ev_decode rows for selects 1..5 R2..R6. Select 1's R3, R4 and R6 come from the probe. The first R6 of each upload stores word 0 (wave_idx 1). R2 and R3 post F: select 2 $000 then $100, select 3 $040 and $140, select 4 $080 and $180, select 5 $0C0 and $1C0. Plus R7 $00 at select 5.
- An ev_edge script after tone_atten: R0 3 and R5 $FF through $0815 (3, $00, 0, $00FF; select 3's index is 0 after Y); R0 $06 and R2 $12 ($FF, $20, $FF, $FFFF); a write to $080A (same fields).
- Then: move the 11 later:05-06 clauses to row, regenerate the golden, and add the ev groups to the psg-chsel-mask evidence.

## Known Stubs

- None in the code. The event rows of Task 3b are deferred to 05-12 (see above), not stubbed.

## Threat Flags

None beyond the plan's threat model:
- T-05-22: MAN rows only at S = 0, 4, >= 16 and byte zero; S = 15 is ASSUMED with mark=sum=15; the pan audible sides are ASSUMED; the lint enforced each.
- T-05-23: the exact expect_red set, with any other red set reported as misprog-misattributed.
- T-05-24: the golden was regenerated three times with the host shasum, with old and new recorded beside the live literal scenes.

## Notes for next plans

- 05-12 first (see the split proposal). psg_tone.aexp is at 32,406 bytes; any change to it needs the D-230 decision or a relocation.
- 05-07:
  - aud09-d (a same-side linear sum) is now later:05-07. Two DDA latches on one side at att 0 give a hand literal of 2 x 31 x 32768.
  - The psg-tone cases need until_mclk at least 12,000,000 (DSV1 done at 9.73 M). Keep that bound if psg_tone grows.
  - Write new anchors with val= wherever a toy can move writes between channels.
- The golden tone_golden is now pcm 9f3e65e7..39e6c, psg 76f6d249..ff7a1. 05-12 regenerates it once more.
- The script player of psg_tone.s, through its PAN_SCENE macro and the commands $00..$3F, $80 + n, $C0 + c and $FF, is the cheapest way to add scenes; delays are units of at least 18,432 master clocks.
- Cut trigger (3) still needs a quiet-machine run (05-08).

## Owner items

- The D-230 decision for 05-12 (cap or relocation). This is an orchestrator item, or an owner item if the cap counts as a gate-rule decision.
- Carried: lever (a), commit 9619efe, awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

The created case files, the two toy directories and the modified fixture files exist. Commits accca1d, 202d5d9 and d5427e2 are ancestors of HEAD.
