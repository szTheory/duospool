---
phase: "05"
slug: "psg-raw-audio"
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-11-13"
---

# Phase 5 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution, drafted before execution from `05-RESEARCH.md` section "Validation Architecture" and the plans `05-01-PLAN.md` to `05-10-PLAN.md`. The plans are authoritative for stage names, case ids and verify commands; on any mismatch the plan wins and the closing plan corrects this file.

> `nyquist_compliant` stays false until execution proves the map: every requirement and criterion row green, every negative control seen red, the full gate, `--selftest`, the dash run, the second awk, all six Linux parts and audit-history clean. Plan 05-10 Task 3 completes and signs this file; nothing flips it earlier.

> **Evidence discipline.** Every green here is E0 repeatability. An audio hash counts only beside a literal group shown to fail (ER-20). Manual-page literals are E1 documentary only after the ER-1 image read (05-01) and only for entries promoted under ER-2 and ER-22. No pitch or timing correctness claim is made while psg_tick_divisor stays open (ER-14). No withheld claim word is used.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None added. The sh+awk gate (`scripts/check`) is the only judge; the C runner `build/gate/ds_runner` prints RESULT lines judged by `conformance-ingest`; toys print CONTROL lines into `build/gate/controls.txt` |
| **Config files** | `CMakePresets.json`, `tests/conformance/cases/*.toml`, `tests/conformance/REQUIRED`, `fixtures/manifest/*.toml`, `fixtures/src/*.{s,checks,aexp}`, `tests/audio/{KINDCLASS,RULES,DISPUTED}`, `tests/conformance/AUDIO_TOY_EXEMPT`, `ledger/REQUIRED` |
| **Quick run** | `sh scripts/check --stage conformance` then `grep '^RESULT <case> pass' build/gate/results.txt`; narrow stages `audio-check` (from 05-05), `fixture-build`, `fixture-lint`, `inventory`, `docs-claims`, `ledger-entries`, `record-lint` |
| **Full suite** | `sh scripts/check` (57 stages now, 58 after 05-05), then `sh scripts/check --selftest`, then `sh scripts/audit-history --range <start>..HEAD` |
| **Linux strict C11** | `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` at the end of every plan that edits C (05-02..05-07, 05-09); all six parts (`sh scripts/linux-repro`, which includes the second awk) and `dash scripts/check` in 05-08 and 05-10 |
| **Runtime baseline** | Recorded by 05-01 in `docs/ci-timings.md` ("Phase 5 baseline"); fold cost re-measured after 05-03 and in 05-04; budgets of D-214 checked in 05-08 and 05-10; cut trigger (3) at +25 percent user CPU over the 05-01 baseline |

---

## Sampling Rate

- **After every task commit:** the task's own verify from its plan (narrow stages plus the RESULT and CONTROL lines it names)
- **After every plan:** `sh scripts/check`, `--selftest`, audit-history over the plan's range, and the gate-gcc Linux part for C plans
- **Before 05-08 and 05-10 sign their checkpoint:** dash, the second awk and all six Linux parts
- **Full gate before a commit (D-216 exceptions):** goldens, digest literals, versions, ledger statuses, the README supported list, the workflow, a gate stage or rule
- **Max feedback latency:** the narrow verify of a task stays under about 2 minutes (one conformance stage run)

---

## Requirement and Success-Criterion Map

| Row | Behaviour proved | Automated check (case or stage) | Command | Literal class | Control that turns it red | Plan |
|-----|------------------|----------------------------------|---------|---------------|---------------------------|------|
| AUD-01 a | Decode, mirrors, select 6/7, R7 at select 0..3, masks | `psg_decode`; ev_decode and ev_edge rows of psg_tone (mask probe) | `sh scripts/check --stage conformance; RESULT psg_decode, audio_tone_pan01` | MAN P8-2..P8-6, P8-9; drops and mirror ASSUMED LED-AUD-0004 | toy psg-chsel-mask; AF8, AF9 | 05-03, 05-06 |
| AUD-01 b | Wave pointer procedures (1)-(3), DDA level reset, latch, counters | `psg_wave_ptr`, `psg_lfo_trg`; dda rows of psg_mod | `--stage conformance; RESULT psg_wave_ptr, audio_mod_dda` | MAN P8-4, P8-5; write-while-on and phase ASSUMED | toy psg-dda-no-reset; scratch mutants of 05-03 | 05-03, 05-07 |
| AUD-01 c | Period 192F at d = 6, all six channels | period rows of tone_sel0 and the pan scenes; `psg_pure_function` | `--stage conformance; RESULT audio_tone_sel0, psg_pure_function` | MAN P8-9 + H8-23 (+ P8-12) | `audio_tone_sel0_d3` (d = 3 expect_red); twin F + 1 | 05-04, 05-06 |
| AUD-01 d | Volume, balance, silence | `audio_tone_atten`, pan silent rows, `psg_vol_table` | `--stage audio-check; --stage conformance; RESULT audio_tone_atten, psg_vol_table` | MAN P8-3, P8-4, P8-10 (S >= 16, byte zero, S = 4); S = 15 and curve ASSUMED | toy psg-atten-max | 05-06 |
| AUD-01 e | Noise on select 4 and 5, NE gating, clock grid | two_level, run_grid, noise_ratio, noise_seq, r7_ignored | `--stage conformance; RESULT audio_mod_noise4, audio_mod_noise5, audio_mod_r7` | MAN P8-2, P8-6, P8-9, H8-23; LFSR ASSUMED | toy psg-noise-taps | 05-07 |
| AUD-01 f | LFO static, needs select 1, dynamic | lfo_static, lfo_needs_sel1, lfo_dynamic; `psg_lfo_trg` | `--stage conformance; RESULT audio_lfo_static, audio_lfo_needs_sel1, audio_lfo_dynamic` | MAN P8-6..P8-9 + H8-23 (D-219) | toy psg-lfo-shift | 05-09 (5.1-01 at a cut) |
| AUD-02 a | DSPE one record per window-2 write, joined 1:1 with dev | `psg_dev_join`, ev rows, DSPE STRUCT header | `--stage conformance; RESULT psg_dev_join` | definition LED-AUD-0013 (ER-19) | AF8; toy psg-emit-mutates | 05-04, 05-05 |
| AUD-02 b | DSPA grid, count closed form, per-frame counts | `pcm_cadence`, STRUCT count and header groups, observer seq rows | `--stage conformance; RESULT pcm_cadence, observer_psg_tone` | ASSUMED LED-VID-0001 + definition LED-AUD-0014 | toy psg-fold-skipped-at-step-end; AF2 | 05-04, 05-05 |
| AUD-02 c | Sinks, IN-03, non-interference, chunks, restore | `run_refuse_*` (with call_overlap), `*_chan_append`, `observer_<rom>`, `aud_noninterference`, `chunk_equiv_audio_<rom>`, `snapshot_restore_audio`, `psg_fold`, `audio_out_rights` | `RESULT` of each | rules in the cases (E0) | 05-02 scratch deletions; hostile rows; toys | 05-02, 05-04, 05-07, 05-09 |
| AUD-03 a | Semantic rows, never nonzero; liveness | audio kind ROWS/REL/STRUCT/EV; `audio_fault_sweep_*`; `audio-check` | `--stage audio-check; --stage conformance; RESULT audio_fault_sweep_tone_sel0` | hand literals (D-206) | AF0..AF9; null-synth rows of `audio_judge_forms`; selftest mutations | 05-04, 05-05 |
| AUD-03 b | Noise deterministic | `audio_noise_repeat` (zero, v4 s1, v4 s2) | `--stage conformance; RESULT audio_noise_repeat` | E0 repeatability | one-byte flip of the comparison | 05-07 |
| AUD-03 c | Zero and v4 identity of audio literal cases can fail | `aud_poweron_policy_diff` | `--stage conformance; RESULT aud_poweron_policy_diff` | ASSUMED LED-PROF-0003 | itself is the negative (streams must differ) | 05-04 |
| SC1 | All six channels, every register, decoded semantics | ev rows of psg_tone, psg_mod, psg_lfo | `--stage conformance; RESULT audio_tone_pan01, audio_tone_pan23, audio_tone_pan45` | GUEST:program + MAN P8-2..P8-4 | AF8, AF9, psg-chsel-mask | 05-06, 05-07, 05-09 |
| SC2 | Recorded rate and format; non-integer cadence visible | DSPA STRUCT header group, `pcm_cadence`, TSV F rows of `--audio-out` | `--stage conformance; RESULT pcm_cadence, audio_out_rights` | definition + ASSUMED | AF2; audio_out_check in conformance-run | 05-04, 05-05 |
| SC3 | Period, content, silence; a mis-programmed fixture fails | tone_sel0 period, cycle_order, silent rows; `audio_tone_sel0_d3`; `audio_tone_pan01_twin` | `--stage conformance; RESULT audio_tone_sel0_d3, audio_tone_pan01_twin` | MAN rows of RESEARCH section 6 | misprog-misattributed if any other group flips | 05-04, 05-06 |
| SC4 | Noise repeats; clock field stays open | `audio_noise_repeat`; docs-check withheld words; LED-AUD-0001 assumed | `--stage conformance; --stage docs-claims; --stage ledger-entries` | E0; ER-14 | tests/docs/claim-word-audio | 05-01, 05-07, 05-10 |

---

## Per-Task Verification Map

Drafted from the plans; executors keep it current through their SUMMARYs and 05-10 completes it. Commands abbreviate each task's verify (stage list; RESULT and CONTROL lines named). File Exists: W0 means a Wave 0 item below must land first.

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 05-01-T1 | 05-01 | W1 | AUD-01..03 | T-05-01 | ER-1 reads, P8 legend and label rules with two mini-roots, TOOLS row | gate | `stage fixture-lint, fixture-lint-negatives, rights-tree; 10 P8 img rows` | W0 | pending |
| 05-01-T2 | 05-01 | W1 | AUD-01..03 | T-05-02 | 12 new and 6 amended ledger entries, REQUIRED lines, withheld words | gate | `stage ledger-entries, ledger-negatives, inventory, docs-claims, docs-claims-negatives` | W0 | pending |
| 05-01-T3 | 05-01 | W1 | AUD-01..03 | T-05-03 | handbook, lever (a), local baseline | gate | `handbook size and id loop; stage workflow, workflow-negatives, ci-timings-controls` | W0 | pending |
| 05-02-T1 | 05-02 | W1 | AUD-02 | T-05-04, T-05-07 | psg and pcm sinks, request members, ds_obs_begin once, inert fold | conformance | `stage core-boundary, conformance; RESULT psg_chan_append, pcm_chan_append, digest_stream_prefix` | W0 | pending |
| 05-02-T2 | 05-02 | W1 | AUD-02 | T-05-04, T-05-05, T-05-06 | five-channel refusals, old sizes, IN-03, OX_ROMS_MAX 48 | conformance | `stage conformance; RESULT run_refuse_call_overlap, run_refuse_struct_size, run_refuse_memory_invalid` | W0 | pending |
| 05-03-T1 | 05-03 | W2 | AUD-01, AUD-02 | T-05-11 | golden compaction, CROWS_MAX 64 | gate | `stage record-lint, record-lint-negatives, conformance; RESULT record_golden_256k_zero, record_constants_shown` | yes | pending |
| 05-03-T2 | 05-03 | W2 | AUD-01, AUD-02 | T-05-08, T-05-09, T-05-11 | layout and the one bump, decoders, noise counters, regenerated literals | conformance | `stage core-boundary, record-lint, conformance; RESULT digest_stream_*, record_golden_*` | yes | pending |
| 05-03-T3 | 05-03 | W2 | AUD-01, AUD-02 | T-05-08, T-05-10, T-05-12 | decode, counters, effects, v4 (enum 5, D-228), C-level cases, hostile rows | conformance | `stage conformance; RESULT psg_decode, psg_wave_ptr, psg_lfo_trg, psg_poweron, psg_restore_divisor, mmu_mpr_256k` | yes | pending |
| 05-04-T1 | 05-04 | W3 | AUD-01..03 | T-05-13, T-05-15, T-05-17 | tracer: DSPE and DSPA judged by hand literals, d = 3 red | conformance | `stage fixture-lint, fixture-build, core-boundary, conformance; RESULT audio_tone_sel0, audio_tone_sel0_d3, pcm_chan_append` | W0 | pending |
| 05-04-T2 | 05-04 | W3 | AUD-02, AUD-03 | T-05-14 | D-200 proof set and aud_poweron_policy_diff | conformance | `stage conformance, sanitizers; RESULT observer_psg_tone, aud_noninterference, aud_poweron_policy_diff, chunk_equiv_audio_psg_tone, snapshot_restore_audio` | yes | pending |
| 05-04-T3 | 05-04 | W3 | AUD-02, AUD-03 | T-05-16 | --audio-out, pcm_cadence, two toys, mutate sweep, fold cost | conformance | `stage conformance; RESULT audio_out_rights, pcm_cadence; CONTROL psg-emit-mutates, psg-fold-skipped-at-step-end` | yes | pending |
| 05-05-T1 | 05-05 | W4 | AUD-03 | T-05-18, T-05-19 | audio-check stage 58, data files, mini-roots, selftests | gate | `stage audio-check; selftest mutation-audio-hash-only, mutation-audio-group-vacuous, mutation-aexp-class-lint` | W0 | pending |
| 05-05-T2 | 05-05 | W4 | AUD-03 | T-05-21 | full grammar, sweep kind, null-synth, STRUCT groups | conformance | `stage audio-check, conformance; RESULT audio_fault_sweep_tone_sel0, audio_judge_forms` | yes | pending |
| 05-05-T3 | 05-05 | W4 | AUD-03 | T-05-20 | golden role, audio_hash_not_verdict | conformance | `stage audio-check, conformance; RESULT audio_tone_golden, audio_hash_not_verdict` | yes | pending |
| 05-06-T1 | 05-06 | W5 | AUD-01, AUD-03 | T-05-22 | pan scenes with solo and single-channel pan sub-phases, event rows, mask probe | conformance | `stage fixture-lint, audio-check, conformance; RESULT audio_tone_pan01, audio_tone_pan23, audio_tone_pan45, audio_tone_golden` | yes | pending |
| 05-06-T2 | 05-06 | W5 | AUD-01, AUD-03 | T-05-22 | attenuation and silence, psg_vol_table, psg_pure_function, toy | conformance | `stage audio-check, conformance; RESULT audio_tone_atten, psg_vol_table, psg_pure_function; CONTROL psg-atten-max` | yes | pending |
| 05-06-T3 | 05-06 | W5 | AUD-01, AUD-03 | T-05-23, T-05-24 | twin, toy, golden regenerated | conformance | `stage audio-check, conformance; RESULT audio_tone_pan01_twin, audio_tone_golden; CONTROL psg-chsel-mask` | yes | pending |
| 05-07-T1 | 05-07 | W6 | AUD-01, AUD-03 | T-05-26 | NE noise path, noise and r7_ignored scenes | conformance | `stage fixture-lint, audio-check, conformance; RESULT audio_mod_noise4, audio_mod_noise5, audio_mod_r7` | yes | pending |
| 05-07-T2 | 05-07 | W6 | AUD-01, AUD-03 | T-05-27 | timer-paced DDA scene, GUEST schedule, toy | conformance | `stage audio-check, conformance; RESULT audio_mod_dda, mutate_sweep_psg_mod; CONTROL psg-dda-no-reset` | yes | pending |
| 05-07-T3 | 05-07 | W6 | AUD-01, AUD-03 | T-05-25, T-05-28 | psg_mod proof set, noise repeat, random regs, flood, toy | conformance | `stage conformance, sanitizers; RESULT audio_noise_repeat, psg_random_regs, psg_block_flood; CONTROL psg-noise-taps` | yes | pending |
| 05-08-T1 | 05-08 | W7 | AUD-01..03 | T-05-30 | phase-gate set and cost record | gate | `stage ci-timings-controls, docs-claims; dash; all six Linux parts` | yes | pending |
| 05-08-T2 | 05-08 | W7 | AUD-01..03 | T-05-29 | scratch mutant sweep, promotions under ER-2 and ER-22 | gate | `stage ledger-entries, inventory, fixture-lint, docs-claims, docs-claims-negatives` | yes | pending |
| 05-08-T3 | 05-08 | W7 | AUD-01..03 | T-05-30 | cut decision, handbook update | gate | `stage ci-timings-controls, docs-claims; handbook size` | yes | pending |
| 05-09-T1 | 05-09 | W8 | AUD-01 | T-05-31, T-05-32 | combined LFO counter, F0', coupled fold | conformance | `stage core-boundary, conformance; RESULT psg_lfo_trg, psg_fold` | yes | pending |
| 05-09-T2 | 05-09 | W8 | AUD-01 | T-05-33 | psg_lfo scenes, toy | conformance | `stage fixture-lint, audio-check, conformance; RESULT audio_lfo_static, audio_lfo_dynamic; CONTROL psg-lfo-shift` | yes | pending |
| 05-09-T3 | 05-09 | W8 | AUD-01 | T-05-31 | psg_lfo proof set | conformance | `stage audio-check, conformance, sanitizers; RESULT observer_psg_lfo, chunk_equiv_audio_psg_lfo` | yes | pending |
| 05-10-T1 | 05-10 | W9 | AUD-01..03 | T-05-36 | phase-gate set and close-out timing | gate | `stage ci-timings-controls, docs-claims; dash; all six Linux parts` | yes | pending |
| 05-10-T2 | 05-10 | W9 | AUD-01..03 | T-05-34 | pins, promotions, RULES, README | gate | `stage ledger-entries, inventory, fixture-lint, audio-check, docs-claims, docs-claims-negatives` | yes | pending |
| 05-10-T3 | 05-10 | W9 | AUD-01..03 | T-05-35 | this file signed off, verification-report inputs | gate | `nyquist_compliant true, status final; stage docs-claims, ledger-entries` | yes | pending |

*Status: pending, green, red, flaky*

---

## Wave 0 Requirements (gaps that must close before the code that needs them)

- [ ] `P8-` label in `fixture-lint-rules.awk` and `fixture-rules.awk`, the `cites.txt` legend and img rows, the PSG manual row in `TOOLS.md` (05-01)
- [ ] 12 ledger entries and 6 amendments; every constants row's ledger id present before `constants.def` grows (05-01)
- [ ] docs-check withheld words with the negative mini-root `tests/docs/claim-word-audio` (05-01)
- [ ] `05-HANDBOOK.md` with the plan-time rulings PR-1..PR-19, D-228 and D-229 (05-01)
- [ ] psg and pcm sink channels with no producer, `ds_obs_begin` new form, OX_ROMS_MAX 48 (05-02)
- [ ] golden headroom and CROWS_MAX 64 before the bump (05-03 Task 1)
- [ ] `psgkinds.c` for C-level PSG cases (05-03)
- [ ] chunk tokens 191 and 193 and the audio compare word (05-04)
- [ ] `NAME.aexp.tsv` channel in `fixture-build`, `audiokinds.c` with the minimal audio kind, `fixtures/lib/psg.inc` (05-04)
- [ ] `audio-check` stage with KINDCLASS, DISPUTED, RULES and mini-roots (05-05)

---

## Manual-Only Verifications

None expected: every behaviour is checked by the gate (AGENTS.md "Verify phases": zero human UAT). Hand-offs, not verifications: the push and the first Phase 5 CI run (D-64); the owner's yes or no on keeping the lever (a) workflow commit, asked at publish time (D-215, D-226); settling C1 or a profile r2 at a milestone review. No hardware capture is needed or requested.

---

## Validation Sign-Off

- [ ] Every task has an automated verify or a Wave 0 dependency
- [ ] Sampling continuity: no three consecutive tasks without an automated verify
- [ ] Every negative control seen red (seven audio toys, or six after a cut; every audio_fault_sweep; audio_tone_sel0_d3; the twin; null-synth rows; aud_poweron_policy_diff; audio-check mini-roots and selftests; hostile restore rows; docs and fixture-lint controls)
- [ ] No watch-mode flags
- [ ] `nyquist_compliant: true` set only by plan 05-10

**Approval:** pending
