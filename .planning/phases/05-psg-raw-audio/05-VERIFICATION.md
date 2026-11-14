---
phase: 05-psg-raw-audio
verified: 2026-11-14T06:36:55Z
status: passed
score: 79/79 must-haves verified
covered_files:
  - ".github/workflows/gate.yml"
  - ".planning/phases/05-psg-raw-audio/05-01-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-01-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-02-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-02-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-03-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-03-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-04-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-04-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-05-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-05-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-06-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-06-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-07-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-07-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-08-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-08-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-10-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-10-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-12-PLAN.md"
  - ".planning/phases/05-psg-raw-audio/05-12-SUMMARY.md"
  - ".planning/phases/05-psg-raw-audio/05-VALIDATION.md"
  - "CMakeLists.txt"
  - "README.md"
  - "docs/ci-timings.md"
  - "fixtures/lib/TOOLS.md"
  - "fixtures/lib/cites.txt"
  - "fixtures/lib/psg.inc"
  - "fixtures/manifest/psg_mod.toml"
  - "fixtures/manifest/psg_tone.toml"
  - "fixtures/src/psg_mod.aexp"
  - "fixtures/src/psg_mod.checks"
  - "fixtures/src/psg_mod.s"
  - "fixtures/src/psg_tone.aexp"
  - "fixtures/src/psg_tone.checks"
  - "fixtures/src/psg_tone.s"
  - "ledger/REQUIRED"
  - "ledger/entries/LED-AUD-0001.toml"
  - "ledger/entries/LED-AUD-0002.toml"
  - "ledger/entries/LED-AUD-0003.toml"
  - "ledger/entries/LED-AUD-0004.toml"
  - "ledger/entries/LED-AUD-0005.toml"
  - "ledger/entries/LED-AUD-0006.toml"
  - "ledger/entries/LED-AUD-0007.toml"
  - "ledger/entries/LED-AUD-0008.toml"
  - "ledger/entries/LED-AUD-0009.toml"
  - "ledger/entries/LED-AUD-0010.toml"
  - "ledger/entries/LED-AUD-0011.toml"
  - "ledger/entries/LED-AUD-0012.toml"
  - "ledger/entries/LED-AUD-0013.toml"
  - "ledger/entries/LED-AUD-0014.toml"
  - "ledger/entries/LED-CPU-0013.toml"
  - "ledger/entries/LED-CPU-0017.toml"
  - "ledger/entries/LED-PROF-0002.toml"
  - "ledger/entries/LED-PROF-0003.toml"
  - "scripts/audio-check"
  - "scripts/check"
  - "scripts/conformance-run"
  - "scripts/docs-check"
  - "scripts/fixture-build"
  - "scripts/lib/aexp-lex.awk"
  - "scripts/lib/aexp-rules.awk"
  - "scripts/lib/fixture-lint-rules.awk"
  - "scripts/lib/fixture-rules.awk"
  - "src/core/boot.c"
  - "src/core/bus.c"
  - "src/core/constants.def"
  - "src/core/cpu.c"
  - "src/core/ds_core.h"
  - "src/core/ds_obs.h"
  - "src/core/ds_poweron.h"
  - "src/core/ds_psg.h"
  - "src/core/ds_serial.h"
  - "src/core/ds_snapshot.h"
  - "src/core/obs.c"
  - "src/core/poweron.c"
  - "src/core/profile_fields.def"
  - "src/core/psg.c"
  - "src/core/psg_effects.def"
  - "src/core/psg_vol.def"
  - "src/core/run.c"
  - "src/core/serial.c"
  - "src/core/snapshot.c"
  - "src/text/ds_text.h"
  - "src/text/text.c"
  - "tests/audio/DISPUTED"
  - "tests/audio/KINDCLASS"
  - "tests/audio/RULES"
  - "tests/audio/arith/EXPECT"
  - "tests/audio/arith/fixtures/src/t.aexp"
  - "tests/audio/class-inflated/EXPECT"
  - "tests/audio/class-inflated/fixtures/src/t.aexp"
  - "tests/audio/class-label/EXPECT"
  - "tests/audio/class-label/fixtures/src/t.aexp"
  - "tests/audio/data-format/EXPECT"
  - "tests/audio/data-format/fixtures/src/t.aexp"
  - "tests/audio/data-format/tests/audio/RULES"
  - "tests/audio/disputed-mark/EXPECT"
  - "tests/audio/disputed-mark/fixtures/src/t.aexp"
  - "tests/audio/disputed-sum/EXPECT"
  - "tests/audio/disputed-sum/fixtures/src/t.aexp"
  - "tests/audio/exempt-format/EXPECT"
  - "tests/audio/exempt-format/fixtures/src/t.aexp"
  - "tests/audio/exempt-format/tests/conformance/AUDIO_TOY_EXEMPT"
  - "tests/audio/format/EXPECT"
  - "tests/audio/format/fixtures/src/t.aexp"
  - "tests/audio/group-af0/EXPECT"
  - "tests/audio/group-af0/fixtures/src/t.aexp"
  - "tests/audio/group-unkilled/EXPECT"
  - "tests/audio/group-unkilled/fixtures/src/t.aexp"
  - "tests/audio/group-vacuous/EXPECT"
  - "tests/audio/group-vacuous/fixtures/src/t.aexp"
  - "tests/audio/hash-only/EXPECT"
  - "tests/audio/hash-only/fixtures/src/t.aexp"
  - "tests/audio/live-class/EXPECT"
  - "tests/audio/live-class/fixtures/src/t.aexp"
  - "tests/audio/ok-audio/fixtures/src/t.aexp"
  - "tests/audio/ok-inventory/fixtures/src/t.aexp"
  - "tests/audio/ok-inventory/tests/conformance/REQUIRED"
  - "tests/audio/ok-inventory/tests/conformance/cases/audio_fault_sweep_t.toml"
  - "tests/audio/ok-inventory/tests/conformance/cases/audio_t.toml"
  - "tests/audio/rule-no-row/EXPECT"
  - "tests/audio/rule-no-row/fixtures/src/t.aexp"
  - "tests/audio/rule-no-row/tests/audio/RULES"
  - "tests/audio/scene-no-case/EXPECT"
  - "tests/audio/scene-no-case/fixtures/src/t.aexp"
  - "tests/audio/scene-no-case/tests/conformance/REQUIRED"
  - "tests/audio/scene-no-case/tests/conformance/cases/audio_fault_sweep_t.toml"
  - "tests/audio/scene-no-case/tests/conformance/cases/audio_t.toml"
  - "tests/audio/scene-no-sweep/EXPECT"
  - "tests/audio/scene-no-sweep/fixtures/src/t.aexp"
  - "tests/audio/scene-no-sweep/tests/conformance/REQUIRED"
  - "tests/audio/scene-no-sweep/tests/conformance/cases/audio_t.toml"
  - "tests/audio/silence-unpaired/EXPECT"
  - "tests/audio/silence-unpaired/fixtures/src/t.aexp"
  - "tests/audio/silent-class/EXPECT"
  - "tests/audio/silent-class/fixtures/src/t.aexp"
  - "tests/audio/toy-no-literal/EXPECT"
  - "tests/audio/toy-no-literal/fixtures/src/t.aexp"
  - "tests/audio/toy-no-literal/tests/controls/toy/EXPECT"
  - "tests/audio/toy-no-literal/tests/controls/toy/RULE"
  - "tests/audio/unknown-key/EXPECT"
  - "tests/audio/unknown-key/fixtures/src/t.aexp"
  - "tests/conformance/AUDIO_TOY_EXEMPT"
  - "tests/conformance/REQUIRED"
  - "tests/conformance/audiokinds.c"
  - "tests/conformance/cases/aud_noninterference.toml"
  - "tests/conformance/cases/aud_poweron_policy_diff.toml"
  - "tests/conformance/cases/audio_fault_sweep_mod_dda.toml"
  - "tests/conformance/cases/audio_fault_sweep_mod_noise4.toml"
  - "tests/conformance/cases/audio_fault_sweep_mod_noise5.toml"
  - "tests/conformance/cases/audio_fault_sweep_mod_r7.toml"
  - "tests/conformance/cases/audio_fault_sweep_tone_atten.toml"
  - "tests/conformance/cases/audio_fault_sweep_tone_pan01.toml"
  - "tests/conformance/cases/audio_fault_sweep_tone_pan23.toml"
  - "tests/conformance/cases/audio_fault_sweep_tone_pan45.toml"
  - "tests/conformance/cases/audio_fault_sweep_tone_sel0.toml"
  - "tests/conformance/cases/audio_hash_not_verdict.toml"
  - "tests/conformance/cases/audio_judge_forms.toml"
  - "tests/conformance/cases/audio_mod_dda.toml"
  - "tests/conformance/cases/audio_mod_noise4.toml"
  - "tests/conformance/cases/audio_mod_noise5.toml"
  - "tests/conformance/cases/audio_mod_r7.toml"
  - "tests/conformance/cases/audio_noise_repeat.toml"
  - "tests/conformance/cases/audio_out_rights.toml"
  - "tests/conformance/cases/audio_tone_atten.toml"
  - "tests/conformance/cases/audio_tone_golden.toml"
  - "tests/conformance/cases/audio_tone_pan01.toml"
  - "tests/conformance/cases/audio_tone_pan01_twin.toml"
  - "tests/conformance/cases/audio_tone_pan23.toml"
  - "tests/conformance/cases/audio_tone_pan45.toml"
  - "tests/conformance/cases/audio_tone_sel0.toml"
  - "tests/conformance/cases/audio_tone_sel0_d3.toml"
  - "tests/conformance/cases/chunk_equiv_audio_psg_mod.toml"
  - "tests/conformance/cases/chunk_equiv_audio_psg_tone.toml"
  - "tests/conformance/cases/chunk_equiv_random32_audio.toml"
  - "tests/conformance/cases/digest_region.toml"
  - "tests/conformance/cases/digest_stream_blkt.toml"
  - "tests/conformance/cases/digest_stream_input.toml"
  - "tests/conformance/cases/digest_stream_irq.toml"
  - "tests/conformance/cases/digest_stream_model.toml"
  - "tests/conformance/cases/digest_stream_prefix.toml"
  - "tests/conformance/cases/mutate_sweep_psg_mod.toml"
  - "tests/conformance/cases/mutate_sweep_psg_tone.toml"
  - "tests/conformance/cases/observer_psg_mod.toml"
  - "tests/conformance/cases/observer_psg_tone.toml"
  - "tests/conformance/cases/pcm_cadence.toml"
  - "tests/conformance/cases/pcm_chan_append.toml"
  - "tests/conformance/cases/psg_block_flood.toml"
  - "tests/conformance/cases/psg_chan_append.toml"
  - "tests/conformance/cases/psg_decode.toml"
  - "tests/conformance/cases/psg_dev_join.toml"
  - "tests/conformance/cases/psg_fold.toml"
  - "tests/conformance/cases/psg_lfo_trg.toml"
  - "tests/conformance/cases/psg_mod.toml"
  - "tests/conformance/cases/psg_poweron.toml"
  - "tests/conformance/cases/psg_pure_function.toml"
  - "tests/conformance/cases/psg_random_regs.toml"
  - "tests/conformance/cases/psg_restore_divisor.toml"
  - "tests/conformance/cases/psg_tone.toml"
  - "tests/conformance/cases/psg_vol_table.toml"
  - "tests/conformance/cases/psg_wave_ptr.toml"
  - "tests/conformance/cases/record_skeleton_256k.toml"
  - "tests/conformance/cases/run_refuse_call_overlap.toml"
  - "tests/conformance/cases/run_refuse_memory_invalid.toml"
  - "tests/conformance/cases/run_refuse_sink_unsupported.toml"
  - "tests/conformance/cases/run_refuse_struct_size.toml"
  - "tests/conformance/cases/snapshot_equals_digest.toml"
  - "tests/conformance/cases/snapshot_refusal_fields.toml"
  - "tests/conformance/cases/snapshot_refusal_psg.toml"
  - "tests/conformance/cases/snapshot_refusal_psgt.toml"
  - "tests/conformance/cases/snapshot_restore_audio.toml"
  - "tests/conformance/cases/video_out_rights.toml"
  - "tests/conformance/chunkkinds.c"
  - "tests/conformance/digkinds.c"
  - "tests/conformance/golden/accept_256k_zero.record.toml"
  - "tests/conformance/golden/accept_384k_seeded.record.toml"
  - "tests/conformance/guestkinds.c"
  - "tests/conformance/obskinds.c"
  - "tests/conformance/powkinds.c"
  - "tests/conformance/profkinds.c"
  - "tests/conformance/psgkinds.c"
  - "tests/conformance/reckinds.c"
  - "tests/conformance/runner.c"
  - "tests/conformance/runner.h"
  - "tests/conformance/snapkinds.c"
  - "tests/conformance/stepkinds.c"
  - "tests/conformance/vdckinds.c"
  - "tests/conformance/videokinds.c"
  - "tests/controls/psg-atten-max/EXPECT"
  - "tests/controls/psg-atten-max/RULE"
  - "tests/controls/psg-chsel-mask/EXPECT"
  - "tests/controls/psg-chsel-mask/RULE"
  - "tests/controls/psg-dda-no-reset/EXPECT"
  - "tests/controls/psg-dda-no-reset/RULE"
  - "tests/controls/psg-emit-mutates/EXPECT"
  - "tests/controls/psg-emit-mutates/RULE"
  - "tests/controls/psg-fold-skipped-at-step-end/EXPECT"
  - "tests/controls/psg-fold-skipped-at-step-end/RULE"
  - "tests/controls/psg-noise-taps/EXPECT"
  - "tests/controls/psg-noise-taps/RULE"
  - "tests/docs/claim-word-audio-authentic-sound/AGENTS.md"
  - "tests/docs/claim-word-audio-authentic-sound/EXPECT"
  - "tests/docs/claim-word-audio-authentic-sound/README.md"
  - "tests/docs/claim-word-audio-authentic-sound/scripts/check"
  - "tests/docs/claim-word-audio-band-limited/AGENTS.md"
  - "tests/docs/claim-word-audio-band-limited/EXPECT"
  - "tests/docs/claim-word-audio-band-limited/README.md"
  - "tests/docs/claim-word-audio-band-limited/scripts/check"
  - "tests/docs/claim-word-audio-plays-music/AGENTS.md"
  - "tests/docs/claim-word-audio-plays-music/EXPECT"
  - "tests/docs/claim-word-audio-plays-music/README.md"
  - "tests/docs/claim-word-audio-plays-music/scripts/check"
  - "tests/docs/claim-word-audio-sounds-correct/AGENTS.md"
  - "tests/docs/claim-word-audio-sounds-correct/EXPECT"
  - "tests/docs/claim-word-audio-sounds-correct/README.md"
  - "tests/docs/claim-word-audio-sounds-correct/scripts/check"
  - "tests/docs/claim-word-audio/AGENTS.md"
  - "tests/docs/claim-word-audio/EXPECT"
  - "tests/docs/claim-word-audio/README.md"
  - "tests/docs/claim-word-audio/scripts/check"
  - "tests/gate/fixture-lint/ok-p8-label/fixtures/lib/cites.txt"
  - "tests/gate/fixture-lint/ok-p8-label/fixtures/src/t.checks"
  - "tests/gate/fixture-lint/ok-p8-label/fixtures/src/t.s"
  - "tests/gate/fixture-lint/ok-p8-label/ledger/entries/LED-AUD-0003.toml"
  - "tests/gate/fixture-lint/p8-fact-no-img/EXPECT"
  - "tests/gate/fixture-lint/p8-fact-no-img/fixtures/lib/cites.txt"
  - "tests/gate/fixture-lint/p8-fact-no-img/fixtures/src/t.checks"
  - "tests/gate/fixture-lint/p8-fact-no-img/fixtures/src/t.s"
  - "tests/gate/fixture-lint/p8-fact-no-img/ledger/entries/LED-AUD-0003.toml"
  - "tests/gate/fixture-lint/p8-label-no-img/EXPECT"
  - "tests/gate/fixture-lint/p8-label-no-img/fixtures/src/t.checks"
  - "tests/gate/fixture-lint/p8-label-no-img/fixtures/src/t.s"
  - "tests/workflow/ok-gate/.github/workflows/gate.yml"
covered_digest: "v4:sha256:734765a59d4939e76f71d30e94a8edc8b2515e85fac1cb6997b31da8b2079551"
behavior_unverified: 0
overrides_applied: 0
deferred:
  - truth: "AUD-01: the PSG implements the LFO to the documented level, verified by fixtures that assert register-to-event behaviour (E1); ROADMAP criterion 1's LFO modulation half"
    addressed_in: "Phase 05.1"
    evidence: "Phase 05.1 goal: 'Authored audio fixtures show select 1 modulating select 0 through the LFO exactly as the PSG pages state (held, absent and running modulation in the select-0 period), judged by hand literals'; REQUIREMENTS.md traceability AUD-01 -> Phase 5.1; ROADMAP criterion 1 text 'LFO modulation in 5.1 (D-234)'"
human_verification: []
---

# Phase 5: PSG Raw Audio Verification Report

**Phase Goal:** Authored audio fixtures produce a timestamped PSG event log and PCM at a recorded format. Semantic properties are checked, and no pitch or timing claim goes beyond the available evidence. (Cut at checkpoint 05-08: LFO modulation is Phase 05.1, D-234.)
**Scope verified:** plans 05-01 to 05-08, 05-10 and 05-12, and the review-fix commits, range `f6afede..7106a7f` (65 commits; the last code commit is `4c89b15`, and the two commits after it change only .planning). ROADMAP Phase 5 criteria 1 to 4; requirements AUD-02 and AUD-03 (completed here) and AUD-01 (advanced); the 71 plan must_haves; D-182 to D-235 with the errata winning (D-219 to D-235).
**Verified:** 2026-11-14 (local macOS machine, Apple clang 21; load average 13 to 29 during the gate run)
**Status:** passed
**Re-verification:** No, initial verification
**Evidence tier of everything below:** E0 repeatability, plus E1 documentary for the two supported entries LED-AUD-0005 and LED-AUD-0008. MAN rows equal a cited, image-read page (P8-n, H8-n); GUEST rows are the guest's own program and schedule; ASSUMED rows name a ledger entry. Nothing here claims hardware accuracy, a pitch, a PSG clock, or that any software sounds right.

## Verdict in one paragraph

All four ROADMAP criteria hold against the tree, with the LFO modulation half of criterion 1 deferred to Phase 05.1 as the roadmap now states. I ran `sh scripts/check --timing` once, alone: CHECK RESULT PASS, 58 stages (59 PASS lines), exit 0, 575 s of stage time (sanitizers 266, conformance 107, determinism 84). No HARNESS_WALL fired, so no re-run was needed.
build/gate/results.txt has 388 RESULT lines, all `pass`, equal one for one to the 388 ids in `tests/conformance/REQUIRED`. build/sanitize/results.txt has 386, all `pass` (the 2 missing ids are the EXCLUDED_IN_MODE alloc-shim cases). build/gate/controls.txt has 32 `RED_OK` lines, the six PSG toys among them. `sh scripts/check --stage audio-check` alone is PASS (12 s).
I read the PSG code paths directly (psg.c, ds_psg.h, bus.c window 2, cpu.c step end, obs.c sinks, snapshot.c PSGT decoder) and re-derived the load-bearing literals outside the repository. These are the volume table, the LFSR period, the frame cadence, the period distances at d = 6 and d = 3, the noise grids, levels_exact and the DDA schedule sum. All match the committed values.
The host `shasum -a 256` of the gate's own `--audio-out` files equals the pinned golden digests. The README supported list equals the ledger supported set (15 ids). The two PSG entries in it are E1 documentary. Every other LED-AUD entry is assumed.

## Observable Truths

### ROADMAP Phase 5 success criteria

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| SC1 | Fixtures program all 6 channels (wave RAM, frequency, volume/balance, noise on the two noise channels, DDA); the DSPE log entries match the fixture's literal writes and the documented semantics; channel numbering follows the ledger's normalised scheme; LFO registers logged and decoded, LFO modulation in 5.1 | VERIFIED (LFO modulation deferred) | detail 1 |
| SC2 | PCM at a defined, recorded rate and format before any host resampling; the per-frame sample count is reported, so a non-integer cadence is visible | VERIFIED | detail 2 |
| SC3 | Audio checks assert period, waveform content and silence; a deliberately mis-programmed fixture (wrong frequency or a silent channel) fails them; "buffer is nonzero" is never accepted | VERIFIED | detail 3 |
| SC4 | Noise output identical across repeated fresh runs under the same power-on policy; PSG clock and revision stay open profile parameters; no PSG pitch or timing correctness claim | VERIFIED | detail 4 |

### Truths named by the orchestrator brief

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| B1 | Raw PSG audio exists as specified: six channels, noise, DDA, wave RAM, volume curve, 192-clock sampling cadence, the DSPE psg sink and the DSPA pcm sink, `--audio-out` | VERIFIED | detail 5 |
| B2 | The audio-check stage exists with its controls (mini-roots, size cap, selftest mutations) | VERIFIED | detail 6 |
| B3 | LFO correctly out of scope: R8/R9 decoded and logged, R9 CTL != 0 flagged UNMODELLED, select 0 unmodulated, tracked in Phase 5.1 | VERIFIED | detail 7 |
| B4 | Evidence discipline: every audio check is E0 or a hand literal classed MAN, GUEST or ASSUMED; a hash is never a verdict; no withheld claim word; LED-AUD-0005 and -0008 supported at E1 documentary with the README equal to the ledger; the other AUD entries assumed | VERIFIED | detail 8 |

### Plan must_haves (abridged; each checked against code and this run's gate output)

| # | Plan | Truth (abridged) | Status | Evidence |
|---|------|------------------|--------|----------|
| 1 | 01 | cites.txt img rows P8-2..P8-10, P8-12, H8-23 (and H8-18, H8-24); P8 legend; no manual text or image committed | VERIFIED | 10 P8 img rows plus H8-18/23/24 read; `git ls-files` holds no PDF, image or ROM |
| 2 | 01 | fixture-lint accepts P8 labels only with an img row; negative mini-roots | VERIFIED | tests/gate/fixture-lint/p8-*; CHECK fixture-lint-negatives PASS |
| 3 | 01 | 12 new and 6 amended ledger entries, DX disagreements, REQUIRED lines | VERIFIED | 14 LED-AUD entries; ledger-entries and inventory PASS. Pins are now final (truth 61) |
| 4 | 01 | LED-AUD-0011 worded per D-219/D-231; LED-AUD-0004 names the D-220 restart edges | VERIFIED (as corrected by WR-06 at the cut) | 0011 now states what runs (decode, TRG freeze, UNMODELLED), and the D-219 clauses are marked planned for 05.1-01; 0004 statement names key-on, DDA 1 -> 0, TRG 1 -> 0 |
| 5 | 01 | docs-check rejects the four new D-218 words; claim-word-audio control red | VERIFIED | docs-claims-negatives PASS; five audio mini-roots (WR-07 added one per word) |
| 6 | 01 | 05-HANDBOOK.md 15,000..25,600 bytes, lines under 400, names D-182..D-232 | VERIFIED | 25,567 bytes, 0 lines over 399, no id missing |
| 7 | 01 | Every gate job prints nproc and the CPU model (lever (a)); local baseline recorded | VERIFIED | gate.yml 2 nproc steps; docs/ci-timings.md "Phase 5 baseline" |
| 8 | 02 | ds_run_request gains psg and pcm as last members; run_refuse_struct_size keeps old sizes | VERIFIED | run_refuse_struct_size pass |
| 9 | 02 | ds_obs_sinks_check covers five channels in the unchanged order with alignment, wrap and overlap rows | VERIFIED | obs.c DS_OBS_SINKS 5, s[3] psg, s[4] pcm; run_refuse_memory_invalid and run_refuse_sink_unsupported pass |
| 10 | 02 | ds_obs_begin changed once to (log, request, machine_time) | VERIFIED | obs.c:333-345 |
| 11 | 02 | IN-03: a request or result overlapping the instance is memory_invalid before the sink checks | VERIFIED | run.c order comment and check; run_refuse_call_overlap pass (9 assertions) |
| 12 | 02 | psg sink 24-byte drop-newest records; pcm sink 8-byte frames, sticky full, header at start and end | VERIFIED | obs.c ds_obs_psg/ds_obs_pcm/write_pheader; psg_chan_append, pcm_chan_append pass |
| 13 | 02 | ds_psg_advance called at the window-2 write head (step_end 0) and after the VDC fold (step_end 1) | VERIFIED | bus.c:220, cpu.c:1105-1106. "Does nothing yet" was the 05-02 state, filled by 05-03/05-04 by design |
| 14 | 02 | OX_ROMS_MAX 48 | VERIFIED | obskinds.c:305 |
| 15 | 03 | PSG0 42 bytes + pad, PSGW 192, PSGT 40; no implicit padding | VERIFIED | snapkinds.c SK_EXPECTED_SIZE 92553 (274 PSG bytes, derived by hand); snapshot_equals_digest pass |
| 16 | 03 | Serial 9, epoch 6, text 8, regions 24, bumped once with 15 PSG constants rows | VERIFIED | ds_serial.h:22,41; ds_core.h:22; ds_text.h:36; digest_stream_* pass |
| 17 | 03 | Restore pass 1 refuses every D-203 violation (field), including distances out of range and off the tick grid | VERIFIED (D-235 tightening) | dec_psg0/psgw/psgt refuse every listed violation; select 1 now bounded by d x 4096 (D-235 amends D-203); snapshot_refusal_psg 18, _psgt 17, psg_restore_divisor 9 rows pass |
| 18 | 03 | Boot: registers, latches, distances 0, noise distances 1, LFSRs 1 under every policy; noise clocks from time 0 | VERIFIED | psg.c:39-60; psg_poweron pass (25 assertions) |
| 19 | 03 | Window-2 decode reg = offset & $0F through one mask table; select 6/7 drop; R7 only at select 4/5; effects per psg_effects.def | VERIFIED | psg.c:32-37, 244-342; psg_decode pass (93 assertions) |
| 20 | 03 | Counter bookkeeping: key-on next = (w - w mod d) + d x Fe, freeze rules, DDA and TRG restarts, closed-form widx advance | VERIFIED | psg.c:104-133, 226-240, 278-293, 319-334; psg_wave_ptr, psg_lfo_trg, psg_fold pass |
| 21 | 03 | seeded-prng-v4 is enum 5, PSGW____ domain; default policies zero, v4 s1, v4 s2; hostile row policy-6 refused | VERIFIED | psg_poweron pass; snapshot_refusal_fields pass |
| 22 | 04 | ca65 tracer by TIN, F = 256; one DSPE record per window-2 write joined 1:1 with dev; one DSPA frame per 192 clocks | VERIFIED | psg_dev_join pass; audio_tone_sel0 pass (13 groups) |
| 23 | 04 | Period rows c2..c6 1,024 samples apart; cycle_order ranks up to rotation | VERIFIED | evidence: "crossings 946 and 1970 ... are 1024 samples apart"; "rank as the words at rotation 0"; 1,024 = 4 x 32 x 6 x 256 / 192 re-derived |
| 24 | 04 | Silent rows before key-on (MAN:P8-4) and at power-on (ASSUMED), each with a live partner | VERIFIED | silent_pre_L/R and silent_poweron_L/R pass; AUDIO_SILENCE_UNPAIRED lint |
| 25 | 04 | d = 3 binding fails exactly the period groups (512 against 1,024) | VERIFIED | audio_tone_sel0_d3: "512 samples apart, the literal is 1024" on period_L/R only |
| 26 | 04 | D-200 proof set on psg_tone: observer, non-interference, chunks, restore, dev join, fold | VERIFIED | observer_psg_tone (976), aud_noninterference, chunk_equiv_audio_psg_tone, chunk_equiv_random32_audio (1376), snapshot_restore_audio, psg_dev_join, psg_fold pass |
| 27 | 04 | aud_poweron_policy_diff: an un-uploaded channel differs between v4 seeds and between zero and v4 | VERIFIED | pass (4 assertions) |
| 28 | 04 | Toys psg-emit-mutates and psg-fold-skipped-at-step-end red; --audio-out writes only for authored/synthetic audio cases; audio_out_check re-hashes | VERIFIED | both RED_OK; audio_out_rights pass; build/gate/audio/ holds exactly the audio_tone_sel0 files |
| 29 | 04 | pcm_cadence: ceil(M/192) - ceil(T0/192) frames; 21 x 1863 + 11 x 1862 = 59,605 per 32; 263-line counts and 119,665 per 64 | VERIFIED | pcm_cadence pass (117 assertions); every count re-derived |
| 30 | 05 | 58 stages; audio-check checks grammar, limits, KINDCLASS, DISPUTED, lint, RULES, kills, toys, inventory | VERIFIED (cap per D-233) | CHECK audio-check PASS; `AUDIO_CHECK files=2 scenes=11 groups=97 rows=225 clauses=104`; cap 49,152 (D-233 amends the 32,768 of D-230) |
| 31 | 05 | Every audio-check code has a red mini-root; selftest deletes each tagged rule | VERIFIED (selftest reported) | 22 mini-roots run in the stage; my spot run of six gives exit 1 with exactly the EXPECT code; selftest 207 PASS lines reported at 4c89b15 |
| 32 | 05 | Arithmetic lint recomputes d = k x f, g = 2 x nf, S, att, n = ceil(m/192); never writes a value | VERIFIED | aexp-rules.awk rule-aexp-arith; tests/audio/arith red |
| 33 | 05 | MAN silent needs S >= 16, chON 0 or a byte zero; S = 15 disputed; MAN live at S = 0 or 4; silence paired | VERIFIED | disputed-sum, silent-class, live-class, silence-unpaired mini-roots red |
| 34 | 05 | audio_fault_sweep AF0..AF9; every declared kill flips; null-synth controls red; missing sink is harness_failure | VERIFIED | nine sweeps all flipped (detail 3); audio_judge_forms 123 assertions pass |
| 35 | 05 | Golden role at most one per ROM beside a literal scene; audio_hash_not_verdict | VERIFIED | INV_AUDIO_HASH_ONLY static, audio-hash-only runtime; hash_not_verdict red=4 |
| 36 | 06 | Each select 0..5 plays its own asymmetric table at F = 128..448 with solo period and cycle order rows | VERIFIED | psg_tone.s header tables (a, e odd: permutations of 16..31 and 0..15); audio_tone_pan01/23/45 pass |
| 37 | 06 | Pan sub-phases: muted side S = 16 all zero (MAN); audible side ASSUMED | VERIFIED | sil_C1_R, sil_C2_L classes MAN:P8-10; pan per_* groups ASSUMED:LED-AUD-0008 |
| 38 | 06 | DSPE decode rows for every R0..R9 and each channel's R2..R6; edge writes ASSUMED | VERIFIED | ev_decode_sel0, ev_1..ev_5, ev_probe (R8, R9), edge/edge_v rows pass |
| 39 | 06 | P8-10 worked example, S = 15 ASSUMED, S = 16 MAN, byte-zero MAN, strict decrease rows; levels_exact 1,015,808 and 255,161 | VERIFIED | audio_tone_atten pass; sil_P3_R re-derived (8 + 1 + 7 = 16, att 32); 31 x 8231 = 255,161 |
| 40 | 06 | Off-invariance (same, REL) | VERIFIED | same_Y MAN:P8-4 pass |
| 41 | 06 | Twin fails exactly the predicted red set; tone_pan01 green | VERIFIED (7 merged groups, recorded) | 7 expect_red groups fail, 3 pass; the plan's 10 per-side ids merged into 7 groups (05-06 key decision) |
| 42 | 06 | Toys psg-chsel-mask and psg-atten-max red with F-4 greens; psg_vol_table | VERIFIED | both RED_OK, EXPECT files carry pass columns; psg_vol_table pass; VOL rows re-derived by the integer rule |
| 43 | 07 | NE level 31/0 from the bit shifted out (parity mask $30C01); wave counter runs under NE | VERIFIED | psg.c:70-75, 207-208; static assert ties $30C01 to the taps; output-bit derivation re-checked by hand |
| 44 | 07 | psg_mod noise solo: two levels, runs multiples of 14 and 28, ratio 2, noise_seq ASSUMED | VERIFIED | audio_mod_noise4/5 pass; 6 x 64 x 7 / 192 = 14 and x 14 = 28 re-derived |
| 45 | 07 | R7 at select 2 dropped and PCM unchanged (r7_ignored) | VERIFIED | audio_mod_r7 pass |
| 46 | 07 | Timer-paced DDA: level changes at ordinals 1, 3..7, 9, holds, order-isomorphic; GUEST 9 runs, sum 115 | VERIFIED | audio_mod_dda pass; psg_mod.s dda_h handler via VEC_TIMER; sum 115 re-derived |
| 47 | 07 | After DDA the cycle order equals the earlier upload (MAN) and phase starts at word 0 (ASSUMED); toy psg-dda-no-reset | VERIFIED | d_ram, d_phase; toy RED_OK with greens |
| 48 | 07 | Toy psg-noise-taps red on noise_seq only; audio_noise_repeat identical under three policies, fails on a one-byte flip | VERIFIED | RED_OK; `got=E0 repeatability policies=3 identical=3` |
| 49 | 07 | psg_mod proof set; psg_random_regs; psg_block_flood | VERIFIED | observer_psg_mod (956), chunk_equiv_audio_psg_mod, psg_random_regs (128), psg_block_flood pass, also under sanitizers |
| 50 | 08 | sh and dash gate, selftest, audit, six Linux parts recorded | VERIFIED (reported) | 05-08-SUMMARY; not re-run here by instruction; my sh gate PASS supports it |
| 51 | 08 | docs/ci-timings.md per-stage seconds, D-214 rows, CI estimate unverified | VERIFIED | docs/ci-timings.md Phase 5 sections |
| 52 | 08 | Scratch mutant sweep of psg.c with every survivor disposed | VERIFIED (record) | 05-08-SUMMARY: 108 mutants, 102 killed, 3 equivalent survivors (K12, P02, F01), 3 stillborn replaced |
| 53 | 08 | E1 promotion only under ER-2 and ER-22 clause by clause; README equals ledger | VERIFIED | detail 8 |
| 54 | 08 | Cut decision recorded against the six triggers; handbook updated | VERIFIED | docs/ci-timings.md "Cut-line record (D-217)"; D-234 in 05-CONTEXT |
| 55 | 10 | Cut taken: nothing of psg_lfo, psg-lfo-shift or modulation lands; 32 toys | VERIFIED | no psg_lfo files; 32 RED_OK; no tests/controls/psg-lfo-shift |
| 56 | 10 | RULES has no later entry; 17 LFO clauses are limit lfo-in-5.1 | VERIFIED | RULES: 80 row, 24 limit, 0 later |
| 57 | 10 | LED-AUD-0005 clause (c) narrowed; d_hold1 ASSUMED with mark=latch_off | VERIFIED | entry statement and assumption; psg_mod.aexp d_hold1 |
| 58 | 10 | ev_decode_sel0's R2 row is ASSUMED:LED-PROF-0003 | VERIFIED | ev_decode_sel0_pw pass |
| 59 | 10 | cycle_linear and dda_equals_wave dropped; LFO markers name 05.1-01 | VERIFIED | KINDCLASS has neither |
| 60 | 10 | Phase-gate set (sh, dash, selftest, audit, six Linux parts) with trigger (3) by instruction count | VERIFIED (reported) | 05-10-SUMMARY gate table; REVIEW-FIX end runs at 4c89b15 (gate, selftest, audit, gate-gcc); my sh gate PASS here |
| 61 | 10 | No PENDING:5 pin; every LED-AUD entry pinned both ways; supported only under ER-2/ER-22 | VERIFIED | 14 TEST: pins (listed in detail 8); INV_PIN_BACKLINK in ledger-entries PASS |
| 62 | 10 | README audio paragraph in D-218 wording, LFO named as 5.1, will-not-claim items withheld | VERIFIED | README lines 33-41; docs-claims PASS |
| 63 | 10 | 05-VALIDATION.md signed, nyquist_compliant true | VERIFIED | frontmatter status final, nyquist_compliant true |
| 64 | 10 | SUMMARY carries verification inputs, hand-offs, honesty table, counts | VERIFIED | 05-10-SUMMARY sections; counts re-checked (97 groups: MAN 59, ASSUMED 34, GUEST 3, DEF 1) |
| 65 | 12 | .aexp cap 49,152 (AEXP_MAX_BYTES); 49,153 refused exit 2; exactly 49,152 passes | VERIFIED | audio-check:25; the stage's oversize control and ok-size-cap; both .aexp files under the cap (41,065 and 13,934) |
| 66 | 12 | DSPE rows for R2..R6 of selects 1..5, R7 at 4/5, R1, R8, R9 | VERIFIED | ev_1..ev_5, ev_probe pass |
| 67 | 12 | Mask probe literals after tone_pan01 | VERIFIED | ev_probe rows (R9 $80 effect $10, R3 $F1 post $1C0, R6 $E5 WAVE_STORE index 1, R9 $7C post 0) |
| 68 | 12 | Every R2 row's high nibble comes from a program-written R3 | VERIFIED | the one exception is ev_decode_sel0_pw, classed ASSUMED (truth 58) |
| 69 | 12 | Edge writes are ASSUMED rows (mirror, $080A, R7 at select 3, select 6/7) | VERIFIED | edge/edge_v classes; DISPUTED reg/sel tokens |
| 70 | 12 | Toy psg-chsel-mask turns tone_pan45 red, tone_pan01 and tone_sel0 green | VERIFIED | EXPECT with F-4 pass column; RED_OK |
| 71 | 12 | 11 RULES clauses moved to row; golden regenerated once with old and new digests | VERIFIED | RULES has no later line; golden comment line; host shasum of this run's --audio-out files equals the pinned digests |

**Score:** 79/79 verified (4 criteria, 4 brief truths, 71 plan truths). Behaviour-dependent truths (sinks change no state, restore refusal, fold partitions, the non-interference of the fold, noise repeat) each rest on a passing gate case that a toy, a fault, a hostile row or a scratch mutant shows going red. None rests on code presence alone.

**Detail 1 (SC1).** psg_tone.s writes every select 0..5. Its three pan scenes upload a distinct asymmetric table, F, R1, R4 and R5 to each channel, and it adds the attenuation scene and the twin. psg_mod.s covers noise on select 4 and 5 (R7 $98 and $91), R7 at select 2 and the timer-paced DDA scene on select 2.
The DSPE log is judged two ways. GUEST ev_value rows check the program's own writes in order. MAN ev_decode rows check chan, effect, wave_idx and post, written by hand from P8-2..P8-4 and P8-6 (ev_decode_sel0, ev_1..ev_5, ev_probe, the noise and DDA decode rows).
The decode is the core's own (bus.c emit_psg builds the record from the `ds_psg_write` info), not a runner replay. Channels are the R0 select value 0..5 (LED-AUD-0002, DX-2). R8 and R9 are written by the guest in sc_probe (R8 $00, R9 $80, R9 $00, R9 $7C) and decoded in ev_probe.
R9 with CTL != 0 (UNMODELLED) is judged at C level by psg_lfo_trg. No guest R9 write has CTL != 0, so select 0 is never modulated in a fixture. The "(E1)" of the criterion is met by the MAN rows, which are page literals (documentary).
Of the entries that state these semantics, LED-AUD-0005 and -0008 are supported. LED-AUD-0003 stays assumed because its disputed and converse clauses are still in its statement, and the guest rows of its LFO mask clause come with psg_lfo in 05.1-01. The LFO modulation half is deferred to Phase 05.1.

**Detail 2 (SC2).** obs.c writes the pcm header with sample_mclk 192, frame_bytes 8, version 1 and format 1 (2 x u32le unsigned, L then R). first = ceil(T0 / 192) is computed without forming T0 + 191, and static asserts pin the sample phase to 0 (WR-03).
The judge's STRUCT header group checks those fields (row `stream=dspa sample_mclk=192;frame_bytes=8;version=1;format=1`). I read the canonical stream of this run's `--audio-out`: it starts `duospool-pcm` NUL, u32 1, u32 192, u16 2, u8 1, u8 0, u64 first 0, and holds 50,771 frames in 406,201 bytes.
Its `.tsv` F rows read 1863, 1863, 1862, 1863, ... (18 x 1863, 9 x 1862, one partial frame of 479), so the non-integer cadence is visible. pcm_cadence asserts the 262- and 263-line counts. I re-derived them: frame starts 0, 1863, 3726, 5588, ..., 14902; 21 x 1863 + 11 x 1862 = 59,605; and 49 x 1870 + 15 x 1869 = 119,665.
The core has no resampling, filter or DC removal (psg.c ds_psg_sample is a plain unsigned sum).

**Detail 3 (SC3).** The literal groups of tone_sel0 are period_L/R (MAN:P8-9+H8-23, 1,024 samples between c2 and c6), cycle_order_L/R (MAN:P8-4+P8-5) and silent_pre_L/R (MAN:P8-4, chON 0). The pan, attenuation, noise and DDA scenes add their own period, order, silence, attenuation-order and noise-grid groups.
Mis-programmed controls in every gate run:
- `audio_tone_sel0_d3`, the wrong divisor: period_L/R fail with "512 samples apart, the literal is 1024" and every other group stays green.
- `audio_tone_pan01_twin`, F + 1 on select 0 and select 1 never keyed on: exactly the 7 predicted groups fail and the other 3 pass. Any other red set would be the harness failure misprog-misattributed.

"Nonzero" is never a verdict. `live` needs two rising crossings and at least 3 distinct levels between them (audiokinds.c:1404-1428). Every `silent` row needs a `live` partner (AUDIO_SILENCE_UNPAIRED). AF5 (zero window) and AF6 (constant non-silent window) are declared kills.
Fault sweeps of this run, declared kills all flipped: tone_sel0 42/42, tone_pan01 54/54, tone_pan23 48/48, tone_pan45 54/54, tone_atten 55/55, mod_noise4 22/22, mod_noise5 14/14, mod_r7 4/4, mod_dda 33/33. mod_dda is 33 after WR-01 dropped one vacuous AF8 kill; 05-VALIDATION's 34/34 predates that fix.

**Detail 4 (SC4).** `audio_noise_repeat`: `got=E0 repeatability policies=3 identical=3`, two interleaved fresh instances of psg_mod with 13,021 frames and 241 events each, byte-identical under zero, v4 seed 1 and v4 seed 2. The comparison is shown to fail on a one-byte flip.
The LFSR is a register reset to 1 under every policy, never drawn (psg.c:58). I recomputed its period from seed 1 independently: 131,071 = 2^17 - 1, as stated in D-193.
Profile r1 keeps `psg_tick_divisor` with status open, allowed 3 and 6 (LED-AUD-0001, assumed). `chip_revision` is huc6280, owner-selected as a scope choice, and LED-PROF-0001 stays assumed. Neither is promoted, and `git log` shows the profile untouched since Phase 1.
LED-AUD-0001, -0006 and -0011 (period, frequency, LFO rate) stay assumed under ER-14. The README says "no pitch or timing claim is made" and lists Hz, the PSG clock, absolute periods and the sample rate as withheld.

**Detail 5 (B1).** The fold is `ds_psg_advance`, called at exactly two sites: bus.c:220 at the window-2 write (step_end 0) and cpu.c:1106 after the VDC fold (step_end 1). It visits every 192-clock sample instant up to the stamp only when a pcm sink is on, folds events first, then calls the const `ds_psg_sample` and `ds_obs_pcm`.
Without a sink it folds the same events and skips only the sampling (non-interference: aud_noninterference pass, toy psg-emit-mutates red). Every window-2 write is decoded by `ds_psg_write`, emitted as a 24-byte DSPE v1 record through `ds_obs_psg` (bus.c:163-188, 216-225), and logged to dev afterwards.
The pieces of the model are:
- Six channels with wave RAM (psg_wave[192]), the write mode, the DDA latch and the index reset.
- Noise on select 4 and 5 (18-bit LFSR, taps 0/1/11/12/17, output from the shifted-out bit).
- The volume curve psg_vol.def (30 rows, VOL[a] = round(32768 x 10^(-3a/40)); I checked every row with the integer rule in its header, and the table is strictly decreasing from 32768 to 219).
- Silence at att >= 30.
`--audio-out` writes `.dspe`, `.dspa` and `.tsv` only for an audio case of an authored or synthetic fixture. The rights check also covers the image that boots (WR-04). This run's files hash (host shasum) to pcm `0d3aeebf...` and psg `bfa52673...`, equal to the golden row of psg_tone.aexp.

**Detail 6 (B2).** `scripts/audio-check` is stage 32 of 58, after video-check. On the repository it prints `AUDIO_CHECK files=2 scenes=11 groups=97 rows=225 exempt=0 clauses=104` and `STATUS audio-check ok n=2`. The stage also runs:
- the 22 mini-roots of tests/audio (2 ok-, 20 must-fail);
- the generated lex-limit controls;
- the exit-2 control for a 49,153-byte file.
I ran six mini-roots directly (hash-only, group-vacuous, silence-unpaired, class-inflated, disputed-sum, rule-no-row). Each exits 1 with exactly the code in its EXPECT. The selftest mutations (hash-only, group-vacuous, class lint, the other tagged rules, the lex limits, the size cap) are reported PASS at 4c89b15 (207 PASS lines) and were not re-run here by instruction.

**Detail 7 (B3).** psg.c stores R8 and R9 & $83. R9 with TRG 1 sets widx[1] = 0 and freezes select 1 (LFO_RESET). A TRG release restarts an uncoupled select 1 (D-220). R9 with CTL != 0 sets effect bit UNMODELLED (psg.c:331-333). `chan_period` is uncoupled for every channel, so select 0 is never modulated.
The restore bound for select 1 is the uncoupled d x 4096 (D-235). LED-AUD-0011 says "decodes and logs the LFO registers but does not model modulation until Phase 5.1", and LED-AUD-0006 and -0007 mark the LFO conventions as not modelled.
RULES carries 17 `limit` lines named lfo-in-5.1. The README says "LFO modulation is Phase 5.1: an R9 write with CTL not 0 is logged with the UNMODELLED effect bit, and select 0 is not modulated". Phase 05.1 exists in ROADMAP with the LFO goal, and 05.1-CONTEXT C-09 and C-10 carry the restore bound and the ledger clauses back.

**Detail 8 (B4).** Every group of both .aexp files carries a class: MAN:P8-n/H8-n 59, ASSUMED:LED-... 34, GUEST:program/dda-schedule 3, DEF 1. No other label occurs. KINDCLASS caps each kind's class, and DISPUTED forces the disputed points to ASSUMED.
The only digests are the golden row (printed as "E0 repeatability (a change detector beside the literal scene of the same ROM, never a verdict, ER-20)") and the noise-repeat digests (E0). INV_AUDIO_HASH_ONLY and the runtime refusal `audio-hash-only` keep a golden beside a swept literal scene, and audio_hash_not_verdict shows an AF7-faulted stream failing 4 literal groups.
A search of README.md, docs/ and ledger/entries for the D-218 words finds them only in the README policy line and in `withheld` lists. docs-claims and docs-claims-negatives PASS, with one control per new word (WR-07).
The ledger has 15 `supported` entries, equal to the README list: LED-CPU-0001/0002/0005/0014/0016, IRQ-0002/0004, VID-0006/0008/0009/0014/0016/0017, AUD-0005 and AUD-0008. LED-AUD-0001..0004, 0006, 0007 and 0009..0014 are `assumed`.
LED-AUD-0005: each clause has a MAN guest group (ev_decode_sel0 and cycle_order; ev_decode_sel0's R4 $40 row and d_dec; d_hold, d_ord and d_ram; silent_pre). Clause (c) is narrowed to chON 1 and DDA 1, and the disputed chON 0 latch is the ASSUMED group d_hold1, outside the statement.
LED-AUD-0008 after WR-05: silence at chON 0, at R1 or R5 $00 and at a sum of 16 or more, plus a strict decrease only while the sum stays at most 4. The covering rows are sil_P3_R (8 + 1 + 7 = 16, att 32), sil_P4, sil_P5, same_Y, silent_pre, and ao_r1, ao_al, ao_lal at sums 0..2. The sum-15 point stays outside the statement.
Both entries cite img rows P8-3, P8-4, P8-5 and P8-10, say "equal to the manual's stated rules", withhold hardware accuracy, timing and compatibility, and pin a case that names them back.
The README scope sentences for both match the narrowed statements. Pins of the assumed entries: 0001 audio_tone_sel0_d3, 0002 audio_mod_noise5, 0003 psg_decode, 0004 and 0007 psg_wave_ptr, 0006 audio_tone_sel0, 0009 psg_vol_table, 0010 audio_mod_noise4, 0011 psg_lfo_trg, 0012 psg_fold, 0013 psg_dev_join, 0014 pcm_cadence.

### Deferred Items

| # | Item | Addressed In | Evidence |
|---|------|-------------|----------|
| 1 | AUD-01's LFO to the documented level (criterion 1's modulation half): psg_lfo, lfo_static, lfo_needs_sel1, lfo_dynamic, toy psg-lfo-shift, the coupled fold and restore bound | Phase 05.1 | ROADMAP Phase 05.1 goal (select 1 modulating select 0 judged by hand literals); REQUIREMENTS traceability AUD-01 -> Phase 5.1; D-234 |

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/core/psg.c`, `ds_psg.h`, `psg_effects.def`, `psg_vol.def` | PSG model, fold, decode, synthesis | VERIFIED | 342 + 72 lines, substantive; called from bus.c and cpu.c |
| `src/core/obs.c`, `ds_obs.h` | psg and pcm sinks | VERIFIED | five-channel table, two emit hooks with producers |
| `src/core/snapshot.c`, `serial.c` | PSG0, PSGW, PSGT regions and the restore domain | VERIFIED | three decoders, per-d bounds |
| `fixtures/src/psg_tone.{s,aexp,checks}`, `psg_mod.{s,aexp,checks}` | authored audio fixtures and literals | VERIFIED | built by ca65 in the gate; manifests carry their image hashes |
| `scripts/audio-check`, `scripts/lib/aexp-*.awk`, `tests/audio/*` | the static audio judge and its controls | VERIFIED | stage PASS; mini-roots red |
| `tests/conformance/audiokinds.c`, `psgkinds.c` | runner judge, sweeps, --audio-out, C-level PSG cases | VERIFIED | 4,722 + 1,157 lines; every audio case passes |
| `tests/controls/psg-*` (6) | audio toys | VERIFIED | 6 RED_OK |
| `ledger/entries/LED-AUD-0001..0014` | claims ledger | VERIFIED | 2 supported, 12 assumed, all pinned both ways |

`verify.artifacts` over the 10 plans: 27/27 artifacts present and substantive.

## Key Link Verification

The plan key_links are prose, so `verify.key-links` cannot parse them (21 of 23 "source not found"). I checked each link in the code:

| From | To | Via | Status |
|------|----|-----|--------|
| bus.c HW_PSG write | psg.c ds_psg_advance, ds_psg_write | bus.c:216-225 | WIRED |
| cpu.c ds_cpu_step_obs | ds_psg_advance (step_end 1) | cpu.c:1106, after ds_vdc_advance | WIRED |
| bus.c emit_psg | obs.c ds_obs_psg | bus.c:187 | WIRED |
| ds_psg_advance | ds_psg_sample (const) and ds_obs_pcm | psg.c:162-165 | WIRED |
| run.c ds_run | obs.c ds_obs_sinks_check | run.c:96 | WIRED |
| snapshot.c dec_psgt | struct vctx PSG0 values, ds_psg_divisor | snapshot.c:951-967 | WIRED |
| audiokinds.c | build/gate/fixtures/NAME.aexp.tsv | audiokinds.c:261, 560 (written by fixture-build) | WIRED |
| scripts/check stage_audio_check | scripts/audio-check and tests/audio mini-roots | check:1063-1076 | WIRED |
| psg_tone.aexp twin scene | audiokinds.c expect_red rule | the twin case passes only with the exact red set | WIRED |
| psg_mod.s timer handler | R6 writes of select 2 in DDA mode | dda_h installed into VEC_TIMER | WIRED |
| tests/controls/psg-* EXPECT | literal groups (with F-4 greens) | named-controls stage | WIRED |
| README supported list | ledger supported set | DOCS_SUPPORTED_LIST in docs-claims | WIRED |
| LED-AUD-* pinning_test | case pins | INV_PIN_BACKLINK in ledger-entries | WIRED |

## Data-Flow Trace (Level 4)

| Artifact | Data | Source | Produces real data | Status |
|----------|------|--------|--------------------|--------|
| DSPA frames | L, R sums | ds_psg_sample over instance state, which guest writes change through ds_psg_write and the fold advances | yes: 50,771 frames for psg_tone, judged by period and order rows | FLOWING |
| DSPE records | decoded fields | `ds_psg_write` info of each guest write | yes: 626 records for psg_tone, joined 1:1 to dev | FLOWING |
| --audio-out files | the two streams | runner concatenation of per-call sink contents | yes: host shasum equals the golden | FLOWING |
| pcm header | sample_mclk, format | constants and ceil(T0 / 192) | yes: judged by the header STRUCT row | FLOWING |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full gate | `sh scripts/check --timing` (once, alone) | CHECK RESULT PASS, 58 stages, exit 0, 575 s stage time | PASS |
| Audio judge stage | `sh scripts/check --stage audio-check` | CHECK audio-check PASS, 12 s | PASS |
| Audio-check controls go red | `sh scripts/audio-check --root tests/audio/<6 cases> ...` | each exit 1 with exactly its EXPECT code | PASS |
| Six PSG toys | build/gate/controls.txt of this run | 6 of 32 RED_OK lines are psg-* | PASS |
| --audio-out bytes | `shasum -a 256 build/gate/audio/audio_tone_sel0.{dspa,dspe}`; `od` of the header; F rows of the .tsv | equal to the golden; header `duospool-pcm`, 192, 2, 1; counts 1863/1862 | PASS |
| VOL table | integer rule (2v - 1)^40 x 10^(3a) <= 2^640 < (2v + 1)^40 x 10^(3a), scratch Python | all 30 rows hold, strictly decreasing | PASS |
| LFSR period | scratch Python step of taps 0/1/11/12/17 from seed 1 | 131,071 | PASS |
| Cadence and noise grids | scratch arithmetic | 59,605; 119,665; frame starts; 14 and 28 samples; 255,161; sum 115 | PASS |

## Probe Execution

Step 7c: not applicable. No `scripts/*/tests/probe-*.sh` exists, and no plan or SUMMARY declares a probe.

## Requirements Coverage

| Requirement | Source Plans | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| AUD-02 | 05-02, 05-03, 05-04, 05-05, 05-10 | Timestamped PSG register-event log and PCM at a defined, recorded rate and format before host resampling | SATISFIED (E0; the formats are definitions, ER-19) | DSPE v1 and DSPA v1 sinks, psg_dev_join, pcm_cadence, the header and count groups, observer, non-interference, chunk and restore cases, audio_out_rights, --audio-out |
| AUD-03 | 05-04, 05-05, 05-06, 05-07, 05-10 | Semantic audio checks, never "buffer is nonzero"; deterministic noise | SATISFIED (E0; MAN rows E1 documentary where the entry is supported) | audio kind over 11 scenes, nine fault sweeps, d = 3 and twin controls, audio_judge_forms null-synth rows, audio_hash_not_verdict, audio_noise_repeat, audio-check |
| AUD-01 | 05-03, 05-04, 05-06, 05-07, 05-08, 05-10 | Six channels with wave RAM, frequency, volume/balance, noise, DDA and LFO to the documented level (E1) | ADVANCED; LFO part deferred to Phase 05.1 | everything but the LFO is implemented and judged; LFO registers decoded and logged (UNMODELLED); REQUIREMENTS.md keeps AUD-01 open, mapped to Phase 5.1 |

REQUIREMENTS.md maps AUD-02 and AUD-03 to Phase 5 and AUD-01 to Phase 5.1, so no requirement is orphaned. AUD-02 and AUD-03 are ticked `[x]` in REQUIREMENTS.md (by ffe30bd, the 05-10 completion commit).

## Gate run (this verification)

| Command | Result | Time |
|---------|--------|------|
| `sh scripts/check --timing` (once, alone, main checkout at 7106a7f) | CHECK RESULT PASS, exit 0, 58 stages; 388/388 REQUIRED `pass`; 32/32 toys RED_OK (6 PSG); sanitize 386 pass; no HARNESS_WALL | 575 s of stage time (sanitizers 266, conformance 107, determinism 84); load 13 to 29 |
| `sh scripts/check --stage audio-check` (after the full gate, alone) | CHECK audio-check PASS | 12 s |
| `sh scripts/audio-check` on the repository and on six mini-roots | STATUS ok; six controls red with their codes | seconds |
| `verify.artifacts` over 10 plans | 27/27 | - |

Not re-run by me (orchestrator instruction): `--selftest`, `dash scripts/check` and the Linux parts. These are reported as follows:
- At a3a2238: all six Linux parts PASS.
- At 910fa09: dash, selftest, gate-gcc and gate-clang PASS.
- At 4c89b15: the gate, selftest (207 PASS lines), audit-history `59d8eed..HEAD` and gate-gcc PASS.

Only .planning changed between 4c89b15 and 7106a7f (05-CONTEXT D-235 and 05-REVIEW-FIX.md). My full sh gate at 7106a7f passes. `.planning/state.json` is untracked and was not touched (sha256 `418f05b2...` before and after).

## Test Quality Audit

| Test file | Linked req | Skipped | Circular | Assertion level | Verdict |
|-----------|-----------|---------|----------|-----------------|---------|
| tests/conformance/audiokinds.c (audio, sweeps, judge forms, golden, noise repeat, cadence, rights) | AUD-02, AUD-03 | 0 | no: literals are hand-written in .aexp with derivations; the golden is generated by the system and is labelled an E0 change detector that never decides a verdict | value and behavioural (period distances, rank order, exact red sets) | sound |
| tests/conformance/psgkinds.c (decode, wave pointer, TRG, power-on, pure function, vol table, fold, random regs, flood, restore divisor) | AUD-01, AUD-02 | 0 | no: closed forms and splitmix64 bytes from independent computations (per case comments) | value | sound |
| tests/audio mini-roots, tests/controls/psg-* | AUD-03 | 0 | n/a | must-fail controls | sound |

Disabled tests on requirements: 0. Circular patterns: 0 used as a verdict. Insufficient assertions: 0. Provenance: I re-derived the period, cadence, noise-grid, level, DDA-sum, VOL and LFSR literals outside the repository, and each one matches.

## Decision Coverage

The `check.decision-coverage-verify` gate could not parse 05-CONTEXT's decision format (`- **D-182 C1 at the documentary level.**`), so it was not evaluated and gives no counts. By a manual search:
- D-182 to D-234 each appear in the shipped code, tests, ledger, docs or the SUMMARYs.
- D-214, D-215, D-226 and D-229 (budgets, CI levers, stop rule) appear only in planning documents; they are process decisions.
- D-235 is implemented in snapshot.c `dec_psgt` (per-d bounds) and is cited there as "review WR-02", not by its id.
This is a warning only.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| 253 files changed outside .planning | - | TBD, FIXME, XXX (debt markers) | none found | - |
| same | - | TODO, HACK, PLACEHOLDER | none found | - |
| src/text/ds_text.h | 34 | comment line of 475 characters (grown from 344 by 2575af4, 05-03) against the 400-character convention of the handbook | Info | cosmetic; no gate rule covers headers |
| tests/conformance/psgkinds.c:339, cases/psg_lfo_trg.toml:5, cases/snapshot_restore_audio.toml:7 | - | stale "until plan 05-09" / "05-09 adds" comments (WR-06 fixed src/core only) | Info | D-234 says every 05-09 reference reads as 05.1-01 |
| home or temp paths added outside .planning | - | none | - | - |
| stubs | - | none: sampling, decode, emission, judge and sweeps are live (toys, faults, mutants) | - | - |

## Warnings (non-blocking)

1. **Skipped review info items stay open** (05-REVIEW-FIX, with reasons):
   - IN-01 (the ER-16 audible-row lint covers three kinds; every current row is at S = 0);
   - IN-02 (three constants rows could break the core with no build error; a WR-03-style static assert is a cheap follow-up);
   - IN-04 (impossible-state fallbacks compute rather than fail closed);
   - IN-06 (RULES coverage is per kind, not per clause, so ER-17 is coarser than its wording);
   - IN-07 (--audio-out truncates and follows existing files; direct runner use only).
2. **IN-09 needs a ruling.** Every run record lists LED-AUD-0008 under `[withheld]` (through the constants row psg_silence_att), while README lists it as supported. The same pattern predates Phase 5 (LED-VID-0014). The README matches the ledger. The record's `[withheld]` meaning (D-53) is an owner or orchestrator decision.
3. **WR-02 and WR-05 were flagged "requires human verification" by the fixer. I checked both and no human is needed.**
   - WR-02: after the step-end rebase a running distance lies in 1..d x Fe <= d x 4096 and a noise distance in 1..d x 64 x 31, because fold_to runs before the rebase. So the tightened `dec_psgt` refuses no reachable state. snapshot_restore_audio passes under it. D-235 records the ruling, and 05.1-CONTEXT C-09 restores the coupled bound with the coupled fold.
   - WR-05 narrows LED-AUD-0008 and widens nothing (detail 8).
4. **05-SECURITY.md T-05-09** still cites the old select-1 bound (6,291,456) and old snapshot.c line numbers. The code is now stricter (D-235). It is an audited record and was left as it is.
5. **05-VALIDATION.md** quotes mod_dda 34/34 and LED-AUD-0008's pre-WR-05 wording. Both are records at 632cd58; the current figures are above.
6. **LED-AUD-0002** still states "select 1 modulates select 0 through the LFO" although modulation is not modelled. The entry is assumed, its clause is the limit aud02-c lfo-in-5.1, and 05.1-02 reviews it. WR-06 corrected 0006, 0007 and 0011 only.
7. **MVP-mode guard.** ROADMAP marks the phase `mode: mvp`, but the goal is not a user story (`user-story.validate` returns false). As in Phases 1 to 4.1, this report verifies against the ROADMAP criteria, with no User Flow Coverage table.
8. **Process notes.** 05-12-PLAN.md was written by the executor and not plan-checked (05-12-SUMMARY). The plan key_links are prose. snapshot_restore_audio covers 7 restore stops (the D-200 list without the LFO TRG stop, which is 05.1's).

## Recorded hand-offs (owner authority, not verifications)

- Push and pull-request authorisation, and the first Phase 5 CI run (D-64). The CI critical path is unverified; the checkpoint A estimate is 525 to 670 s against the 600 s job trigger.
- Yes or no on keeping the lever (a) workflow commit 9619efe, asked once at publish time (D-215, D-226).
- The R11 push hazard (05.1-CONTEXT C-08): epoch 6 is unreleased. A push of Phase 5 before 05.1-01 runs makes it public, and 05.1-01's LFO behaviour change would then need an epoch bump that no 5.1 plan contains.
- Settling C1, or a profile r2 that narrows psg_tick_divisor, at a milestone review (decision 10). It is needed only for pitch, period or noise-sequence promotions.
- The IN-09 `[withheld]` ruling, and optionally the IN-01, IN-02, IN-04, IN-06 and IN-07 follow-ups.

## Human Verification Required

None. Every behaviour of the phase is checked by the gate, and the project wants zero human UAT. No listening test is needed because no claim about how audio sounds is made: "sounds correct", "plays music" and "authentic sound" are withheld words that the gate rejects.

## Gaps Summary

No blocking gaps. Authored fixtures write every register of all six channels through the bus. The core emits one decoded, timestamped DSPE v1 record per window-2 write and point-sampled DSPA v1 PCM at one sample per 192 master clocks, with the per-frame count visible.
Period, waveform order, silence, attenuation order, noise grid and DDA checks are hand literals classed MAN, GUEST or ASSUMED. Both mis-programmed controls and the fault sweeps show those literals can fail, and the one golden is an E0 change detector beside them.
Noise repeats byte for byte under three power-on policies. The PSG clock stays an open profile field, and no pitch or timing claim is made. LFO modulation is deferred to Phase 05.1 by the recorded cut (D-234). Warnings 1 to 8 are recorded limits, stale records or rulings for the owner.

---

_Verified: 2026-11-14T06:36:55Z_
_Verifier: Claude (gsd-verifier)_
