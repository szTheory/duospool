---
phase: 05-psg-raw-audio
plan: 04
subsystem: psg-synthesis-emission-audio-judge-and-proof-set
tags: [aud, psg, tracer, dspe, dspa, pcm, aexp, audio-judge, d200, toys, audio-out, w3]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-02 sink plumbing (psg and pcm channels, IN-03); 05-03 PSG layout, decode, counter bookkeeping, effect bits, seeded-prng-v4
provides:
  - ds_psg_sample (const), src/core/psg_vol.def (30 rows, LED-AUD-0009), the sample loop of ds_psg_advance on the 192-clock grid, the pcm cursor and header first/sample_mclk (PR-4), DSPE v1 records built in bus.c
  - fixtures/lib/psg.inc, fixtures/src/psg_tone.s (DSV 27), psg_tone.checks, psg_tone.aexp (scene tone_sel0), the manifest; fixture-build write_aexp
  - tests/conformance/audiokinds.c: kind audio (EV and ROWS forms, expect_red binding, zero-versus-v4 identity), aud_noninterference, psg_dev_join, snapshot_restore_audio, aud_poweron_policy_diff, --audio-out, audio_out_rights, pcm_cadence
  - psg_fold (psgkinds.c), the audio compare item and schedules 191/193 (chunkkinds.c), psg and pcm sinks in the observer matrix (obskinds.c), audio_out_check (conformance-run)
  - toys psg-emit-mutates and psg-fold-skipped-at-step-end; the 05-04 fold-cost section of docs/ci-timings.md
affects: [05-05, 05-06, 05-07, 05-09, 05-10]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-02, AUD-03]

actuals:
  tokens: 54873
  tasks: 3
  commits: 4
plan_head_before: 741a402a4a22955f46f398e17ac436b2f5fad548
plan_head_after: 167f2c833b5608679b22346d695638fac59e035d

tech-stack:
  added: []
  patterns:
    - "The fold visits every sample instant cursor x 192 <= stamp in time order, folding the events of that instant first (fold_to), then samples; a write stamped at a sample instant is applied after the sample"
    - "A runner capture appends per call: the sinks of each ds_run call point at the free tails of one accumulator, so chunked and restored runs build the D-199 streams directly and the first-frame chain is checked per call"
    - "An expect_red case judges the scene of another case with the same literals and requires exactly the listed groups red (D-211)"

key-files:
  created:
    - src/core/psg_vol.def
    - fixtures/lib/psg.inc
    - fixtures/src/psg_tone.s
    - fixtures/src/psg_tone.checks
    - fixtures/src/psg_tone.aexp
    - fixtures/manifest/psg_tone.toml
    - tests/conformance/audiokinds.c
    - tests/conformance/cases (14 new: psg_tone, audio_tone_sel0, audio_tone_sel0_d3, aud_noninterference, psg_dev_join, snapshot_restore_audio, aud_poweron_policy_diff, psg_fold, chunk_equiv_audio_psg_tone, chunk_equiv_random32_audio, observer_psg_tone, audio_out_rights, pcm_cadence, mutate_sweep_psg_tone)
    - tests/controls/psg-emit-mutates/{RULE,EXPECT}, tests/controls/psg-fold-skipped-at-step-end/{RULE,EXPECT}
  modified:
    - src/core/psg.c, ds_psg.h, obs.c, ds_obs.h, bus.c
    - tests/conformance/obskinds.c, chunkkinds.c, psgkinds.c, runner.c, runner.h, REQUIRED, cases/pcm_chan_append.toml
    - scripts/fixture-build, scripts/conformance-run, CMakeLists.txt, docs/ci-timings.md

key-decisions:
  - "05-04: group ids keep the plan's side suffixes (period_L, cycle_order_R); the runner accepts [a-z][a-zA-Z0-9_]* for group ids and [a-z][a-z0-9_]* for scene names and case ids"
  - "05-04: a scene row names one case; another case may judge that scene only with expect_red (the d = 3 control audio_tone_sel0_d3 has no scene row of its own)"
  - "05-04: selector items are flat (semicolons); reg, val and nth bind to the most recent anchor (anchor=ev:... or the end anchor to=ev:...); from=+M/to=+M after a start anchor, from=M/to=M without one; window [from, to) holds the samples with from <= 192 s < to"
  - "05-04: in chunk_equiv an audio header off the closed form is a counted check, a lost frame or an inconsistent count stays harness_failure, so the fold toy turns the case guest_failure"

duration: about 3 h 10 min wall (gates at load 10 to 23)
completed: 2026-11-13
---

# Phase 5 Plan 04: the PSG tracer - synthesis, DSPE and DSPA, the audio judge, the D-200 proof set Summary

**A guest PSG write sequence on the bus now becomes one DSPE record per window-2 write and one DSPA frame per 192 master clocks, and the select-0 scene of psg_tone passes every hand-literal group (event values and decode, period 1,024 samples over four cycles, plateau order, silence before key-on and at power-on). The d = 3 binding fails exactly the period groups.
Sinks change no state, chunking and restores reproduce the streams byte for byte, and two toys turn their cases red.**

## Performance

- Tasks: 3 of 3; no stop-rule split was needed (05-12 stays free). The named stop point inside Task 1 was used as two commits (1a core, 1b fixture and judge).
- Files: 41 changed in 741a402..HEAD (4,103 insertions, 68 deletions); 358 conformance cases (344 + 14), all pass.

## Accomplishments

- Task 1a (e955bf7): psg_vol.def; ds_psg_sample (chON gate, DDA latch or wave word, one-statement side attenuation, VOL below 30 only, plain sums with a _Static_assert below 2^31);
  ds_psg_advance folds each sample instant's events first, then samples (bounded by span / 192 + 1); ds_obs_begin sets the pcm cursor and header first to ceil(T0/192) and sample_mclk 192; bus.c builds the DSPE record byte by byte after ds_psg_write, exactly when log_dev emits.
pcm_chan_append now asserts first for T0 = 0, 1, 191, 192, 193 (0, 1, 1, 1, 2).
- Task 1b (d28d834): write_aexp (aexp 2, scene 4 to 6, group 7, row 5 fields); psg.inc; psg_tone.s (power-on silence 4,608 clocks, every channel chON 0, the TIN upload, F = 256, R1 $FF, R5 $FF, key-on after 36,864 clocks, a 444,096-clock tone, key-off, 4 GUEST table checks); psg_tone.aexp; audiokinds.c kind audio; cases psg_tone, audio_tone_sel0, audio_tone_sel0_d3.
- Task 2 (df87b3e): observer psg/pcm columns and the pcm header twin; chunk_equiv audio item and schedules 191/193; aud_noninterference; psg_dev_join; snapshot_restore_audio (5 stops); aud_poweron_policy_diff; psg_fold.
- Task 3 (167f2c8): --audio-out, audio_out_rights, audio_out_check, pcm_cadence, two toys, mutate_sweep_psg_tone, the fold-cost record.

## The tracer evidence (E0; documentary under the bound divisor 6, no pitch claim, ER-14)

| Group | Class | audio_tone_sel0 | audio_tone_sel0_d3 |
|---|---|---|---|
| ev_value_sel0, ev_decode_sel0 | GUEST:program, MAN:P8-2+P8-3+P8-4 | pass (the key-on is record 51 of 53) | pass |
| period_L, period_R | MAN:P8-9+H8-23 | pass: crossings 946 and 1970, 1,024 apart | fail as listed: crossings 818 and 1330, 512 apart |
| cycle_order_L, cycle_order_R | MAN:P8-4+P8-5 | pass: 32 plateaus at rotation 0 | pass (4 samples per word) |
| silent_pre_L, silent_pre_R | MAN:P8-4 | pass: 192 samples | pass |
| silent_poweron_L, silent_poweron_R | ASSUMED:LED-PROF-0003 | pass: 219 samples | pass |

Streams under zero and seeded-prng-v4 seed 1 are byte-identical. pcm_sha256=3e20cc72... psg_sha256=f85f0369... (E0 change detectors, never a verdict); 3,537 frames and 53 events to DSV1 done at 678,936 master clocks. The host re-hash of --audio-out matches both.

## Red evidence (R3, R5): scratch mutants of psg.c (never committed), the d = 3 binding and the toys

| Fault | Red groups or cases | Green |
|---|---|---|
| step period F -> F + 1 (chan_period) | period_L, period_R of audio_tone_sel0 | all other groups |
| wave index read reversed (31 - widx) | cycle_order_L, cycle_order_R; audio_tone_sel0_d3 harness_failure misprog-misattributed | period, silent, ev |
| chON gate removed (ds_psg_sample) | silent_pre_L, silent_pre_R (sample 242 reads 524,288 = 16 x 32768) | silent_poweron (power-on AL 0 gives att >= 30) |
| bind.psg_tick_divisor=3 | period_L, period_R (expect_red) | cycle_order, silent, ev |
| toy psg-emit-mutates | aud_noninterference, observer_psg_tone guest_failure: CONTROL RED_OK | |
| toy psg-fold-skipped-at-step-end | chunk_equiv_audio_psg_tone, observer_psg_tone guest_failure: CONTROL RED_OK | |
| aud_poweron_policy_diff | v4 seeds 1 and 2 differ; zero and v4 seed 1 differ (the identity check reports it); zero and v3 seed 1 same | |

## .aexp byte budget (D-230)

psg_tone.aexp is 12,532 bytes after this plan: 10 groups and 57 rows of tone_sel0 (a row about 115 bytes, a group 92, a comment line about 200; the 40 ev_decode rows are 4,500 bytes).
Projection: +2.7 KB in 05-05 (STRUCT groups, golden scene), +16 KB in 05-06 (three pan scenes about 2.5 KB each, tone_atten 2 KB, event rows 3.6 KB, probe 0.8 KB, twin 2.5 KB): about 31 KB of 32,768 and about 250 of 512 rows. Within the cap with a small margin; the measurement and the projection are in the file's header.

## Gate results (end of plan, one at a time)

| Run | Result | Wall | User CPU | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check --timing` (after all code; only docs/ci-timings.md changed after it, and docs-claims, ci-timings-controls, rights-tree and licences were re-run green) | PASS, CHECK RESULT PASS | 657.5 s | 501.2 s | 22.5 at start, 11.9 at end |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 177 PASS lines (176 rows and the RESULT line), 0 FAIL | 97 s | 41.4 s | |
| `sh scripts/audit-history --range 741a402..HEAD` | clean (4 commits, 51 blobs, 2,126 blob-path pairs) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | LINUX_REPRO gate-gcc PASS | 132 s | | |

Task verifies, each run exactly as written before its commit: VERIFY1_OK (1b), the 1a narrow verify (core-boundary and conformance, 344 pass), VERIFY2_OK (conformance and sanitizers), VERIFY3_OK.

## Fold cost (D-214) and the insn-sink budget (RF-12, D-227)

- The machine was not quiet (load 10 to 23 all afternoon), so the clean-machine measurement asked for by 05-03 could not be taken; cut trigger (3) stays unjudged (501.2 s user CPU against 337.5 s at two to three times the baseline load). Conformance alone: 97.1 s user CPU (358 cases) against 74.6 s at 05-03 (344 cases).
- The sample loop runs only with a pcm sink (at most 4 instants per step); chunk_equiv_random32_audio with every audio sink took 2.39 s user CPU, about 0.6 microseconds per step all in.
- psg_tone relies on OX_SINK_RECORDS 1,048,576, waits at the power-up low speed, reaches DSV1 done at 678,936 master clocks (under 2 frames of 262 lines) in under 20,000 instructions; about 140 frames of 4-cycle waiting would fit.

## Live budget table (after 05-04)

| Budget | Value after 05-04 | Phase 5 target |
|---|---|---|
| Gate stages | 57 | 58 |
| Selftest PASS lines | 177 (176 rows) | >= 178 |
| Power-on policies | 5 (done) | 5 |
| Constants rows / CROWS_MAX | 49 / 64 (done) | 49 / 64 |
| Withheld ids | 42 (done) | 42 |
| Record rows (skeleton / 16 events / cap) | 175 / 191 / 192 | 175 / 191 / 192 |
| Regions; serial / epoch / text | 24; 9 / 6 / 8 (done) | 24; 9 / 6 / 8 |
| Snapshot / instance / request bytes | 92,553 / 1,141,376 / 72 (done) | same |
| Golden bytes (256k / 384k) | 8,557 / 8,601 of 16,384 | below 16,384 |
| .aexp bytes / rows (psg_tone) | 12,532 / 57 | below 32,768 / 512 |
| OX_ROMS_MAX / guest ROMs | 48 / 37 | 48 / 39 |
| DSV ids | 1..27 | 27, 28, 29 |
| Toys | 28 | 33 |
| Ledger entries / REQUIRED lines | 75 / 75 | 75 / 75 |
| Conformance REQUIRED cases | 358 (344 + 14) | about 379 to 394 |
| CI critical path | unverified (no push) | about 587 to 612 s predicted |
| Local warm gate user CPU | 501.2 s at load 22.5 to 11.9 (not comparable) | trigger (3) above 337.5 s at a comparable load |

## Task Commits

1. Task 1a: synthesis, volume curve, DSPE and DSPA emission, pcm cursor - `e955bf7` (feat)
2. Task 1b: psg_tone, the .aexp channel, the audio judge, the d = 3 control - `d28d834` (feat)
3. Task 2: the D-200 proof set - `df87b3e` (test)
4. Task 3: --audio-out, pcm_cadence, toys, mutate sweep, fold cost - `167f2c8` (feat)

## Deviations from Plan

**1. [Rule 3 - blocking] Upper-case side suffixes in group ids.** The plan's group ids (period_L, cycle_order_R, expect_red=period_L,period_R) are outside [a-z0-9_]. The runner accepts [a-z][a-zA-Z0-9_]* for group ids only; scene names and case ids stay lower case. 05-05's lexer must accept the same (its "names [a-z0-9_]+" covers scenes). Commit d28d834.

**2. [Rule 1 - bug] psg.inc register-write macro.** `sta a:(reg)` is parsed by ca65 as an indirect operand; the macro writes `sta a:reg`. Found by the first build, fixed before the commit.

**3. [Rule 2 - design within scope] The expect_red binding of a scene.** The d = 3 case has no scene row (two scene rows may not name one case and a scene); a scene row names one case, and another case may judge that scene only with expect_red. Commit d28d834.

**4. [Rule 2 - correctness] psg_dev_join gains an in-memory program.**
The fixture psg_tone has no undecoded write and no read of the window, so the plan's "block bytes and undecoded writes included, no read produces one" is shown on a 15-write program (2 reads first, R7 at select 0, reg $A and $F, R2 at select 6, the $0810 mirror, a TII of 8 bytes to $0806..$080D, one more read): exactly 15 DSPE records, joined 1:1.
Commit df87b3e.

**5. [Rule 1 - Task 3 needs] chunkkinds.c changed again in Task 3.** In Task 2 a per-call pcm header off the closed form was harness_failure; under the toy psg-fold-skipped-at-step-end that made the case harness_failure, not guest_failure.
The closed form and the first-frame chain are now a counted check (cs_compare and the uninterrupted run), a lost frame or an inconsistent count stays harness_failure. Commit 167f2c8 (the file is in the plan's files_modified).

**6. Scope notes, no file outside files_modified.** The observer matrix shares ob_cfgs, so every observer case (observer_vce_pal and the others) now also carries the psg and pcm sinks and their checks (all green). The audio kind runs zero and v4 seed 1 by itself (no policies key) and reads checks from its expect (as the plan writes it).
The selector end anchor has no [+mclk] offset yet (tone_sel0 needs none); an offset is refused as malformed. ev_value_sel0 declares kills AF0,AF8 (AF9 relabels a channel, which an ev_value row does not read); the fault sweep of 05-05 verifies every declared kill.

**7. Commit trailer.** All four commits end with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`, as the standing brief requires.

Requirement ticks: none in the plan commits; any AUD tick made by the plan-complete step is reverted with a file-specific checkout of .planning/REQUIREMENTS.md (only 05-10 completes AUD ids).

## Known Stubs

- The NE noise replacement in ds_psg_sample lands in 05-07 and the LFO modulation in 05-09 (the plan's prohibitions); the DDA level line is in place and is judged by the dda rows of 05-07.
- The audio kind judges literal scenes only; the golden role, STRUCT groups, the fault sweep and audio-check land in 05-05. None of these blocks this plan's goal.

## Threat Flags

None beyond the plan's threat model: T-05-13 (byte-by-byte writers, whole records and frames, VOL below 30, the mix bound asserted), T-05-14 (sample loop bounded, psg_fold under sanitizers), T-05-15 (digests beside literal groups; d = 3 and the scratch mutants red), T-05-16 (audio_out_rights, authored or synthetic only), T-05-17 (write_aexp field ranges, the runner refuses unknown rows and keys).

## Notes for next plans

- 05-05: the .aexp grammar the runner reads (audiokinds.c header): rows aexp, scene, group, row; selector items anchor=ev:ch=<0..5|G>, reg=R<n>, val=$hh (optional), nth=N, ev=+K, from=+M|M, to=+M|M|ev:ch=.., side=L|R, with reg/val/nth bound to the most recent anchor; closed literal keys: the lint inputs of the plan plus reg, val, vals, chan, effect, wave_idx, post. Group ids may end _L/_R.
- 05-05: kinds judged so far: ev_value, ev_decode (form EV), period, cycle_order, silent (form ROWS); any other kind is refused as aexp-unknown-key until 05-05 adds it. au_streams_differ is the zero-versus-v4 identity check (aud_poweron_policy_diff shows it reports a difference).
- 05-05: the evidence line `pcm_sha256=HEX psg_sha256=HEX E0 ...` is parsed by audio_out_check with a sed pattern; keep its form. The declared kills of tone_sel0 are in psg_tone.aexp; the sweep must verify each.
- 05-05 and 05-06: psg_tone.aexp is 12,532 bytes and projected at about 31 KB after 05-06; write one comment line per group, never per row.
- 05-06: psg.inc is frozen (PR-5); psg_tone's scratch cells are $2430..$2439; the guest reaches DSV1 done at 678,936 master clocks today.
- Cost: cut trigger (3) still needs a quiet-machine re-run (05-05 or 05-08). scripts/check line 2261 still names v3 for the guest list (carried from 05-03, 05-05 owns scripts/check).

## Owner items

- None new. Carried: lever (a) commit 9619efe awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

Files: src/core/psg_vol.def, fixtures/lib/psg.inc, fixtures/src/psg_tone.{s,checks,aexp}, fixtures/manifest/psg_tone.toml, tests/conformance/audiokinds.c, the 14 new case files and the two toy directories exist. Commits e955bf7, d28d834, df87b3e and 167f2c8 are ancestors of HEAD.
