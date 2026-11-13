---
phase: 05-psg-raw-audio
plan: 07
subsystem: psg-noise-dda-psg_mod-proof-set
tags: [aud, psg_mod, noise, lfsr, ne, dda, timer-irq, noise_seq, dda_hold, cycle_phase, toys, d-200, criterion-4, w6]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-06 and 05-12 (psg_tone with six channels and the event rows, the D-233 cap), 05-05 (the audio-check stage, the AF0..AF9 sweep)
provides:
  - the NE noise level in ds_psg_sample (psg_noise_bit, the RF-6 parity of the post state masked $30C01)
  - the authored fixture psg_mod (DSV 28) with the scenes mod_noise4, mod_noise5, mod_r7 and mod_dda, 8 GUEST checks, its .aexp and manifest
  - the judges two_level, run_grid, run_min, noise_seq, noise_ratio, dda_hold, dda_order, dda_level_exact, dda_ram_kept and cycle_phase, and the literal keys bits and ords
  - psg_mod's D-200 proof set (observer, chunk equivalence, restore stops dda and noise, non-interference), audio_noise_repeat, psg_random_regs, psg_block_flood
  - the toys psg-dda-no-reset and psg-noise-taps (32 toys)
affects: [05-08, 05-09, 05-10]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-03]

actuals:
  tokens: 28000
  tasks: 3
  commits: 3
plan_head_before: 977fea899346be9a9cd0280c2f78a5d498d76157
plan_head_after: aa4e0b74fb51dd6be0119d5fa025f58cf772f95e

tech-stack:
  added: []
  patterns:
    - "noise_seq ordinals come from the observed R7 stamp of the anchor's channel and the row inputs p0 and p1, never from an LFSR model; an ordinal past the committed string is a harness error"
    - "dda_hold accepts a write's change in the sample at or after its stamp or in the next one, so a MAN row never judges the tie order of a write at a sample instant"
    - "A seed-free scene (mod_noise5) gives a sequence toy a case-level F-4 pass column; the first row of an EV group is one whose record has no identical twin under the same nth, so AF8 and AF9 move a row"

key-files:
  created:
    - fixtures/src/psg_mod.s, psg_mod.checks, psg_mod.aexp; fixtures/manifest/psg_mod.toml
    - tests/conformance/cases psg_mod, audio_mod_noise4, audio_mod_noise5, audio_mod_r7, audio_mod_dda, their four audio_fault_sweep cases, mutate_sweep_psg_mod, observer_psg_mod, chunk_equiv_audio_psg_mod, audio_noise_repeat, psg_random_regs, psg_block_flood
    - tests/controls/psg-dda-no-reset/RULE and EXPECT, tests/controls/psg-noise-taps/RULE and EXPECT
  modified:
    - src/core/psg.c; tests/conformance/audiokinds.c, psgkinds.c, runner.c, runner.h, REQUIRED; scripts/lib/aexp-rules.awk; tests/audio/RULES
    - tests/conformance/cases/aud_noninterference.toml, snapshot_restore_audio.toml

key-decisions:
  - "05-07: the noise level is recomputed from the stored LFSR state by the RF-6 parity (mask $30C01, tied to the taps by a static assert); PSGT stores no output bit, and the wave counter keeps running under NE"
  - "05-07: the 256-bit noise_seq string is committed in psg_mod.aexp; a throwaway script outside the repository drew it after reproducing the 40-bit RESEARCH string and the step-20 state $37C0F, and it agrees with the RF-6 parity; there is no gate-time generator"
  - "05-07: aud12-d (the wave counter under NE) becomes a named limit, because the wave is unheard while NE is 1 and no Phase 5 scene releases NE while playing; rf5-a goes to later:05-09 for its TRG half"
  - "05-07: cycle_linear and dda_equals_wave have no row in any plan; their judge marker moves from 05-07 to 05-10, which implements or drops them"

duration: about 1 h 0 min wall (gates at load 10 to 20)
completed: 2026-11-13
---

# Phase 5 Plan 07: Noise, DDA and the psg_mod proof set Summary

**Selects 4 and 5 with NE 1 now play a two-level noise from their 18-bit LFSRs. The new authored ROM psg_mod judges it in two ways:**
- **MAN rows:** two levels on a grid of 14 and 28 samples, in the ratio 2.
- **ASSUMED rows:** the level at each noise event follows a committed 256-bit seed-1 string.

**An R7 write at select 2 changes nothing. In a timer-paced DDA scene each written value holds until the next write, the wave RAM survives DDA and playback restarts at word 0; DSV1 checks the guest's own write schedule.**

**psg_mod carries the full D-200 proof set. Its noise output repeats byte for byte across fresh interleaved runs under three policies (E0). Hostile register programs and a 65,536-byte flood stay bounded.**

## Performance

- Tasks: 3 of 3, no split. 33 files changed in 977fea8..aa4e0b7 (1,741 insertions, 101 deletions), plus this SUMMARY and the state files.
- 388 REQUIRED conformance cases (373 + 15), all pass.
- psg_mod reaches DSV1 done at 2,427,012 master clocks, about 6.8 frames of 262 lines. CODE is 849 of 4,096 bytes and RODATA 289 bytes. Every scene stays under 16 frames (D-214) and the whole ROM is far inside 40 frames (RF-12).

## Accomplishments

- **Task 1 (ae9a270): the noise path and the first three scenes.**
  - The core: NE gating and psg_noise_bit in psg.c. The tap expression stays one statement.
  - The fixture: psg_mod.s with mod_noise4, mod_noise5 and mod_r7, and three table-sum GUEST checks.
  - The judges: two_level, run_grid, run_min, noise_seq, noise_ratio, dda_hold, dda_order, dda_level_exact, dda_ram_kept and cycle_phase, all in audiokinds.c because Task 2 lists no C file. The keys bits and ords are in au_keys and in litkey, and the lint's numeric loop skips them. audio_judge_forms has rows for each new kind.
  - Cases, sweeps and REQUIRED lines. RULES: ten noise clauses move to row and aud12-d becomes a limit.
- **Task 2 (e925a98): the DDA scene.**
  - The timer-paced DDA scene (a)..(f) and checks 4..8.
  - mod_dda with 9 groups, the toy psg-dda-no-reset, audio_mod_dda, its sweep and mutate_sweep_psg_mod.
  - RULES: eleven DDA and counter clauses move to row and rf5-a to later:05-09. The commit message says twelve; eleven is the right count.
- **Task 3 (aa4e0b7): the proof set and the robustness cases.**
  - observer_psg_mod and chunk_equiv_audio_psg_mod.
  - snapshot_restore_audio now runs per ROM: psg_tone with its 5 stops, psg_mod with the stops dda and noise, 7 restores in all.
  - aud_noninterference over psg_tone and psg_mod.
  - audio_noise_repeat, psg_random_regs (32 programs, 1,307 writes), psg_block_flood, and the toy psg-noise-taps.

## The literal scenes (every literal passed against the core on its first run; none was changed after a run)

| Scene | Group (kind, class) | Literal |
|---|---|---|
| mod_noise4 | n4_ev (ev_decode MAN) | R7 $98 at select 4: chan 4, effect 0, index 0, post $0098 |
| | n4_two (two_level MAN:P8-2+P8-6+P8-9+H8-23), L and R | n=2 over key-on + 12,096 .. + 312,096 |
| | n4_lev (levels_exact ASSUMED:LED-AUD-0009+0010) | peak 31 x 32768 = 1,015,808 |
| | n4_grid (run_grid MAN), L and R | nf=7;g=14 (384 x 7 / 192) |
| | n4_min (run_min ASSUMED:LED-AUD-0010) | minrun=14 |
| | n4_seq (noise_seq ASSUMED:LED-AUD-0010, every policy) | p0=11904;p1=2688;bits = the 256-bit string; observed: R7 at 50,028, 111 events of ordinals 8..118 (hand bound 123) |
| mod_noise5 | n5_ev, n5_two, n5_grid (nf=14;g=28), n5_ratio (noise_ratio MAN, p0=1;p1=2) | grids 14 and 28; no sequence row (seed-free) |
| mod_r7 | r7_same (same MAN:P8-2+P8-9) | 768 samples from the first rising crossing of each window equal |
| | r7_drop (ev_edge ASSUMED:LED-AUD-0004) | chan $FF, effect $20, wave_idx $FF, post $FFFF |
| mod_dda | d_ev (ev_value GUEST:program) | R6 $05,$05,$14,$03,$1F,$00,$0C,$0C,$1B, consecutive |
| | d_dec (ev_decode MAN) | DDA writes 1 and 9: DDA_LOAD at index 0, post 5 and 27; R4 $40 and R4 $DF: IDX_RESET, index 0 |
| | d_hold (dda_hold MAN:P8-4+P8-5), L and R | ords=1,3,4,5,6,7,9 |
| | d_ord (dda_order MAN), L and R | vals=5,20,3,31,0,12,27 |
| | d_lev (dda_level_exact ASSUMED:LED-AUD-0009) | 163,840, 655,360, 98,304, 1,015,808, 0, 393,216, 884,736 |
| | d_ram (dda_ram_kept MAN), L and R | T2 up to rotation (observed rotation 0) |
| | d_phase (cycle_phase ASSUMED:LED-AUD-0012) | T2 from word 0 after the (d) key-on |
| | d_rel (cycle_phase ASSUMED:LED-AUD-0004+0012) | T2 from word 0 after the (f) release of DDA with chON 1 (D-220) |
| | d_sum (levels_exact ASSUMED:LED-AUD-0009, aud09-d) | 2 x 31 x 32768 = 2,031,616 |

The 256-bit string and its method:
- A throwaway Python script outside the repository stepped the LFSR of D-193 from 1 (taps 0, 1, 11, 12 and 17, right shift, output the old bit 0).
- It first reproduced the 20- and 40-bit strings of RESEARCH section 2 and the state $37C0F after step 20.
- It then printed steps 1..256 and computed them a second way, by the RF-6 parity of each post state, with the same result.
- The string has 124 ones. A second check over every ordinal range the windows can take confirmed two levels, a shortest interior run of one event, and an event-run gcd of 1. This is a property of the string, used for the ASSUMED run_min literal and as a sanity check of the MAN rows.

## Kill matrices (E0, observed by the sweep cases)

| Scene | Declared kills flipped | Observed beyond the declared |
|---|---|---|
| mod_noise4 | 22 of 22 | AF7, AF8 and AF9 on most windowed groups |
| mod_noise5 | 14 of 14 | AF5..AF9 on n5_ratio |
| mod_r7 | 4 of 4 | AF4..AF9 on r7_same |
| mod_dda | 30 of 30 | d_hold also AF2, AF4, AF7, AF9; AF1 flips no group, so no write stamp sits on a sample instant |

## Toy and mutant evidence (R5)

| Toy or mutant | Red | Stays green |
|---|---|---|
| scratch mutant, NE ignored (not committed) | audio_mod_noise4 (n4_two, n4_grid, n4_min, n4_seq) and audio_mod_noise5 (n5_two, n5_grid, n5_ratio) | n4_ev, n4_lev, n5_ev |
| psg-dda-no-reset (the reset statement replaced by an empty statement) | audio_mod_dda: d_phase, d_rel, d_dec (record 185 reads index 28) | d_ram, d_hold, d_ord, d_lev, d_sum, d_ev; audio_mod_r7 and audio_mod_noise4 pass (F-4) |
| psg-noise-taps (tap 12 dropped) | audio_mod_noise4: n4_seq only (the sequence differs from ordinal 7) | n4_two, n4_grid, n4_min, n4_lev, n4_ev; audio_mod_noise5 and audio_tone_sel0 pass (F-4) |

The toy count is 32 after this plan (33 after 05-09).

## Gate results (one at a time)

| Run | Result | Wall | User CPU | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check --timing` before the Task 1 commit (aexp-rules.awk and RULES changed) | PASS, 59 CHECK PASS lines | 368.1 s | 310.9 s | 20.4 to 10.9 |
| Task 1 verify as written | VERIFY1_OK | | | |
| `sh scripts/check --timing` before the Task 2 commit (RULES, a new toy) | PASS (conformance 83 s, sanitizers 180 s) | 433.7 s | 344.6 s | 17.4 to 19.3 |
| Task 2 verify conditions (audio-check, conformance, results, CONTROL psg-dda-no-reset RED_OK, wc -c) | all met | | | |
| Task 3 verify as written (conformance, results, CONTROL psg-noise-taps RED_OK, `--stage sanitizers`) | VERIFY3_OK, sanitizers PASS | sanitizers 182.0 s | 173.8 s | |
| `sh scripts/check --timing` at plan end | PASS, 59 CHECK PASS lines (conformance 85 s, sanitizers 167 s) | 415.0 s | 334.4 s | 13.8 to 9.5 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 207 PASS lines (206 rows and the RESULT line) | 78.8 s | 31.0 s | 13.6 to 9.8 |
| `sh scripts/audit-history --range 977fea8..HEAD` | clean (3 commits, 48 blobs, 2,236 blob-path pairs) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | PASS (gcc 13, strict C11) | 51.4 s | | |

- The first sweep of mod_dda failed on d_dec (AF8 and AF9 declared but not flipped). The row order was fixed before the Task 2 commit (deviation 3), so every commit is green.
- User CPU was taken at load 10 to 20, so it is not comparable with the 05-01 baseline (270.0 s). Cut trigger (3) still needs a quiet run in 05-08.

## Live budget table (after 05-07)

| Budget | Value after 05-07 | Phase 5 target |
|---|---|---|
| Gate stages | 58 (done) | 58 |
| Selftest PASS lines | 207 (206 rows) | >= 178 |
| Constants rows / CROWS_MAX | 49 / 64 (done) | 49 / 64 |
| Withheld ids | 42 (done) | 42 |
| .aexp cap | 49,152 bytes (D-233), 512 rows | below the cap |
| .aexp bytes / rows (psg_mod) | 13,105 / 33 (21 groups); projection after 05-08: unchanged | below 32,768 (plan budget) / 512 |
| .aexp bytes / rows (psg_tone) | 40,664 / 190 (unchanged) | below 49,152 / 512 |
| OX_ROMS_MAX / guest ROMs | 48 / 38 | 48 / 39 |
| DSV ids | 1..28 | 27, 28, 29 |
| Toys | 32 | 33 |
| Conformance REQUIRED cases | 388 | about 379 to 394 |
| RULES clauses (row / limit / later) | 104 (80 / 7 / 17 later:05-09) | every later: moved to row by 05-10 |
| psg_mod run | 2.43 M master clocks (6.8 frames) | about 40 frames |
| Local warm gate user CPU | 310.9 to 344.6 s at load 10 to 20 (not comparable) | trigger (3) above 337.5 s at a comparable load |

## Task Commits

1. Task 1: the NE noise level, psg_mod's noise and r7 scenes, the judges: `ae9a270` (feat)
2. Task 2: the timer-paced DDA scene, the GUEST checks, the DDA rows, toy psg-dda-no-reset: `e925a98` (feat)
3. Task 3: the proof set, audio_noise_repeat, psg_random_regs, psg_block_flood, toy psg-noise-taps: `aa4e0b7` (test)

## Deviations from Plan

**1. [Rule 3, files] Files outside files_modified.**
- scripts/lib/aexp-rules.awk gained the literal keys bits and ords, and its numeric loop skips them. The orchestrator's binding note requires each new judge key in both lists.
- audiokinds.c changed in Task 1 for every new judge, including the DDA ones, because Task 2 lists no C file.
- The psg-tone cases, psg_dev_join and psg_fold are untouched.
- The psg-mod cases moved from checks=3 to checks=8 in Task 2, because checks 4..8 land with the DDA scene.

**2. [Rule 2, completeness] Rows beyond the plan's list, all within its clauses.**
- n4_lev: levels_exact, 31 x 32768, ASSUMED, for aud10-a "level 31 or 0".
- n4_min: run_min, ASSUMED; without it a run grid of 28 would also pass g = 14.
- R7 decode rows n4_ev and n5_ev (MAN, post $0098 and $0091).
- d_ev: the GUEST write schedule read from the stream.
- d_dec: decode rows.
- d_rel: a cycle_phase row after a DDA 1 -> 0 release with chON 1, phase (f). It covers D-220, aud04-f and the "frozen while DDA 1" clause, which (d) alone, released with chON 0, cannot.
- d_sum: the aud09-d hand literal 2 x 31 x 32768 on selects 2 and 3, phase (e).
- noise_seq is on mod_noise4 only, so that mod_noise5 is the seed-free F-4 pass column of psg-noise-taps.

**3. [Rule 1] d_dec row order.**
- The first sweep showed AF8 and AF9 not flipping d_dec. Its first row was (b)'s R4 $40, and the next R4 $40 of select 2, in (d), decodes the same under the same nth.
- The DDA write rows now come first: a dropped or relabelled write 1 moves the ev=+8 row onto (d)'s R4 $40. The literals are unchanged.

**4. [Plan text] two_level uses n=2, not count=2.** The kind count is a STRUCT kind name and n is an existing literal key, so no new key was needed.

**5. [Plan text, plan check 1 warning 7] The stamp phase mod 192 of the DDA writes is not stated from a hand cycle count.**
- Each write lands at a timer borrow plus the interrupt latency plus the handler's cycles. The borrow phase (LED-IRQ-0002) and the latency are ASSUMED, so a hand number would be an ASSUMED figure the row does not use.
- The row accepts the change in the sample at or after the stamp or in the next one, the second fix warning 7 offers. Its comment says so.
- The sweep observes AF1 flipping nothing, which is consistent with no stamp on a sample instant. That is an observation, not a literal.

**6. [Note, a known dependency] dda_hold ordinal 1 needs a level other than 5 before write 1.**
- That level is the latch 0 of R6 $00, written with chON 0. That is procedure (3), the disputed latch_off point.
- Under the other reading the latch keeps its power-on 0 (LED-PROF-0003). The plan prescribes this sequence; the row stays MAN as planned.

**7. [Scope] RULES.**
- aud12-d (the select 4 and 5 wave counters run under NE) has no row, because the wave is unheard while NE is 1. It is now a named limit, ne-wave-unheard, with that reason; the ledger entry stays assumed.
- rf5-a is later:05-09: d_rel covers its DDA half, and its TRG half belongs to 05-09.
- Partial clauses moved to row carry a note of what is not judged: aud04-e, aud04-f, aud12-b, aud12-c, d185-a. These notes cover the restart instants, R2/R3 at a reload, and chON 0 freezing.
- aud10-c and aud12-b now need noise_seq, since its ordinals count from boot and keep the old period to the reload.

**8. [Scope] The kind table.**
- cycle_linear and dda_equals_wave were marked "judged from plan 05-07" but no plan has a row for them. Their marker is now 05-10.
- The two audio_judge_forms rows that tested "a later kind is harness" now use these two kinds instead of two_level and noise_ratio, so the check is kept.

**9. [Scope] The proof-set cases grow by expect items, not new files.**
- aud_noninterference takes several rom= items.
- snapshot_restore_audio takes rom/stops pairs. Its until_mclk rose to 2,500,000 for both ROMs; it was 1,000,000 and 700,000 for psg_tone.
- psg_tone's restore stops sample and mid now sit at 1.25 M instead of 0.35 M master clocks.

State and requirements after the plan-complete step: git diff .planning/REQUIREMENTS.md showed no change, so nothing was reverted (05-10 completes the AUD ids); STATE.md reads Current Plan 8 and Total Plans in Phase 11; .planning/state.json was restored to its original bytes and is not committed.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model:
- T-05-25: psg_random_regs and psg_block_flood run in the sanitizers stage. The flood counts drop-newest records exactly.
- T-05-26: noise_seq is ASSUMED with the committed string. The MAN rows are only two_level, run_grid and noise_ratio. psg-noise-taps turns only n4_seq red.
- T-05-27: the DDA MAN rows are hold, order and RAM kept. The latch values are ASSUMED, and the GUEST rows cover only the guest's own schedule.
- T-05-28: aud_noninterference, the observer and the flood digest cover psg_mod.

## Notes for next plans

- **05-08, the checkpoint and the cut decision:**
  - Cut trigger (1): no revision plan (05-16+) so far, and 05-07 needed no split.
  - Trigger (3) still needs a quiet-machine run. This plan's gates ran at load 10 to 20 (user CPU 310.9 to 344.6 s) and are not comparable with the 05-01 baseline of 270.0 s.
  - The conformance stage took 85 s wall at the plan-end gate with 388 cases (05-12: 98 s with 373, at another load); the new cost was not measured on a quiet machine.
  - Triggers (4) and (5), the LFO, are untouched.
  - psg_mod.aexp is 13,105 bytes, far below both caps.
  - The full gate passed at every commit, so there is no outstanding red for the checkpoint.
- **05-08, the ledger:**
  - LED-AUD-0010 (noise) and LED-AUD-0012 (counter timing) stay assumed (criterion-4 hold, ER-14).
  - The noise and DDA MAN rows exist now for the E1 review: two_level, run_grid, noise_ratio, dda_hold, dda_order, dda_ram_kept and the r7 same.
- **05-09:**
  - The restore kind has a per-ROM stop-set helper (au_restore_rom); add the LFO TRG stop as a new set.
  - aud_noninterference takes one to four rom= items; add psg-lfo.
  - rf5-a is later:05-09.
  - The toy count reaches 33 with psg-lfo-shift.
- **05-10:**
  - Implement or drop cycle_linear and dda_equals_wave.
  - aud12-d is a limit; review it if a scene releasing NE while playing is wanted.
  - The partial RULES notes listed in deviation 7.
  - ev_decode_sel0's R2 row (a 05-12 note).

## Owner items

- None new.
- Carried: lever (a), commit 9619efe, awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

The created fixture, manifest, case and toy files and this SUMMARY exist. Commits ae9a270, e925a98 and aa4e0b7 are ancestors of HEAD.
