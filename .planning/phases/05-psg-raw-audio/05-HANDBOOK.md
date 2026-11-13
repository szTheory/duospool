# Phase 5 Handbook: PSG Raw Audio

Written by plan 05-01 from 05-CONTEXT (D-182..D-232, D-233 added by 05-12; errata win), 05-RESEARCH (RF-1..RF-17) and 05-PATTERNS (C-1..C-11, F-51..F-86). Plans from 05-03 on read this file, their plan and the previous SUMMARY, and open the sources only at cited anchors.
A line here that contradicts 05-CONTEXT is a defect; this file changes only in 05-08, 05-10 or a recorded deviation.
Channels are R0 select values 0..5 (the manual's ch1..ch6). Page labels: P8-n = PSG manual PDF page n+2; H8-n = hardware manual PDF page n+3 (image rows in fixtures/lib/cites.txt, read 2026-11-13).

## 1. Scope and boundary

Deliverables (AUD-01..03): (1) register model (mirror, select, R1..R9 masks, wave RAM, DDA latch, noise and LFO registers); (2) time model (tick master/d, P8-9 periods, one fold ds_psg_advance); (3) synthesis (5-bit levels, P8-10 sum, assumed Q15 curve, linear mix, LFSR, LFO);
(4) raw audio (sinks psg DSPE v1 and pcm DSPA v1, streams, --audio-out, per-frame counts); (5) evidence (three ROMs, the audio judge, fault sweep, seven toys, in-ROM twin, wrong-divisor control, stage audio-check).
Not in phase: resampling, playback, WAV, band-limited synthesis, CD audio, SuperGrafx, HuC6280A, public API, [audio] table (Phase 6), fuzzing, captures, a profile r2. No pitch or timing claim while the PSG clock is open.

## 2. Decisions (one line each)

- D-182 C1 documentary: P8-9 fOUT = fmaster/(2 x 32 x F), fmaster 7.16 MHz from the CPU; H8-23 CLKP = OSC1/3; a step is 6F master clocks, a cycle 192F (E1 documentary, not a measurement).
- D-183 psg_tick_divisor d (3|6, OPEN) divides every PSG counter; every audio case binds 6 except the d = 3 expect_red case; no literal uses the bound value; the profile is not edited.
- D-184 Period table: step d x Fe (Fe = F, 0 -> 4096), cycle 32 x d x Fe, noise d x 64 x NFc (NFc = 31 - (R7 & $1F); 0 -> 32 ticks), coupled select-1 step d x F1e x FLFe (FLF 0 -> 256), F0' = (F0 + $1000 + (w << s) - (16 << s)) & $FFF, 0 -> 4096, s = 2((R9 & 3) - 1).
- D-185 Counter model (ASSUMED LED-AUD-0012): ticks at k x d; R2/R3/R7 apply at the next reload; key-on next = (w - w mod d) + d x Fe, index unchanged; frozen while chON 0 or DDA 1 (select 1 also TRG 1); NE keeps the wave counter running; tie order select 1, select 0..5, noise, sample, then the write.
- D-186 Fold ds_psg_advance at window-2 writes and at step end after ds_vdc_advance, never at reads; PSGT stores distances r (0 = frozen); closed form for uncoupled channels, an event loop for the LFO pair and LFSRs.
- D-187 Sample s at s x 192 master clocks (psg_mclk_per_sample 192, psg_sample_phase_mclk 0); ceil(M/192) samples after a run to M; a definition (LED-AUD-0014); counts in section 5.
- D-188 Window $1FE800-$1FEBFF, reg = offset & $0F, $A..$F NO_TARGET; reads unchanged; write order: penalty no-op, stamp, io_buffer, advance, ds_psg_write, ds_obs_psg, log_dev; the mmu_mpr, spd_speed and cyc_probe guards stay green.
- D-189 psg_select = R0 & 7; R2..R6 go to the selected channel, dropped at 6 or 7; R7 only at select 4 or 5; R1, R8, R9 ignore R0; masks in one table ds_psg_reg_mask (section 5).
- D-190 R4 with DDA 1 sets widx 0 (level reading); R6: DDA 1 loads dda; chON 0 DDA 0 stores and increments; chON 1 DDA 0 stores with no increment (psg_wave_write_when_on 1).
- D-191 Output gating: chON 0 gives 0; DDA 1 gives dda; NE at select 4/5 gives the noise level; else wave[widx]; HuC6280 only.
- D-192 LFO (amended by D-219, D-231): CTL != 0 with select 0 and 1 on; TRG 1 holds wave[1][0] - 16; select 1 steps at d x F1e x FLFe; until 05-09 an R9 write with CTL != 0 sets UNMODELLED.
- D-193 Noise: two 18-bit LFSRs (section 5), output bit 0 as 31/0, reset 1 under every policy, clocking always from time 0; period 131,071 (ASSUMED LED-AUD-0010).
- D-194 att = 2(15 - MAL) + (31 - AL) + 2(15 - LAL), 0..91; silent at att >= 30; contribution level x VOL[att]; L and R plain unsigned sums, 0..6,094,848; no DC removal, filter or clip.
- D-195 Integer obligations: u64 times, u32 periods (step <= 24,576, coupled <= 6,291,456, noise <= 11,904), mix < 2^31, VOL indexed below 30 only, LFSR & $3FFFF, no float, no signed overflow.
- D-196 DSPE v1: one 24-byte LE record per window-2 write (section 5); effect bits in psg_effects.def.
- D-197 DSPA v1 (section 5); struct ds_pcm_sink {header[32]; frames; capacity}; DS_PCM_SINK_MAX_FRAMES 8,388,608; drop-newest in whole frames.
- D-198 struct ds_psg_sink {header[16]; records; capacity} (dev shape); request gains psg, pcm as LAST members (05-02); 5 sink channels; ds_obs_begin(log, request, machine_time) once; IN-03 refusal.
- D-199 Runner streams: DSPE "duospool-psg" NUL, u32 1, records; DSPA "duospool-pcm" NUL, u32 1, u32 sample_mclk, u16 2, u8 1, u8 0, u64 first, frames; --audio-out writes .dspe, .dspa, .tsv.
- D-200 Proof set per ROM: observer, aud_noninterference, chunk_equiv_audio, chunk_equiv_random32_audio, snapshot_restore_audio, psg_dev_join, psg_fold; the sample function is const.
- D-201 Phase 6 contract: two streams; pcm capacity >= ceil((until - T0 + ds_max_step_mclk())/192) + 1; lost != 0 is incomplete; no [audio] table in Phase 5.
- D-202 Regions PSG0 (42 bytes + pad), PSGW (192), PSGT (40); 21 -> 24 regions; serial 9, epoch 6, text 8 once in 05-03 (one correction bump allowed); regeneration per D-165.
- D-203 Restore pass 1 (field 12): field ranges, wave <= 31, LFSR 1..$3FFFF, DDA implies widx 0, rem 0 exactly when frozen, distance ranges, tick grid ((machine_time - 1 + r) mod d = 0); the TRG rule is dropped by D-231.
- D-204 Power-on: PSG state 0 under every policy, noise distances 1, LFSRs psg_lfsr_reset; seeded-prng-v4 = v3 draws then PSGW____ (192 bytes & $1F); guest policies zero, v4 s1, v4 s2; audio literal cases zero and v4 s1.
- D-205 No model row (D-113 cap stays 4); PSG constants rows land in constants.def (now 15, D-228); psg_vol.def is a separate 30-row table (LED-AUD-0009); manual numbers are plain defines with a cite.
- D-206 Oracles: hand literals in fixtures/src/NAME.aexp with a derivation comment; the arithmetic lint recomputes rows with formula inputs (AEXP_ARITH); no reference synthesiser, generator or emulator diff.
- D-207 Judge kinds audio (ROWS, REL, STRUCT, EV) and audio_fault_sweep in audiokinds.c; .aexp scene/group/row; selectors anchor on DSPE events; one static stage audio-check.
- D-208 Isolation by solo scenes and pan pairs (muted side sum >= 16); KINDCLASS caps each kind; AUDIO_SILENCE_UNPAIRED: a silent row needs a live row on its side and scene.
- D-209 Faults AF0..AF9 on captured streams; AUDIO_GROUP_VACUOUS, INV_AUDIO_GROUP_UNKILLED; at most one golden per audio ROM (INV_AUDIO_HASH_ONLY); audio_hash_not_verdict; null-synth controls red.
- D-210 Audio toys, cap 7: psg-emit-mutates, psg-fold-skipped-at-step-end (05-04), psg-chsel-mask, psg-atten-max (05-06), psg-dda-no-reset, psg-noise-taps (05-07), psg-lfo-shift (05-09).
- D-211 Criterion-3 controls: in-ROM twin (F + 1 and one channel never keyed on, exact red set, else misprog-misattributed) and the period scene bound to d = 3 as expect_red.
- D-212 ROMs psg_tone (DSV 27, 05-04/05-06), psg_mod (28, 05-07), psg_lfo (29, 05-09), 0BSD, CODE 4,096 bytes each, writing every register and all 32 words they enable; plus C-level cases (05-CONTEXT D-212).
- D-213 Ledger (05-01): LED-AUD-0003..0014 new; -0001, -0002, PROF-0002, -0003, CPU-0013, -0017 amended; all assumed; E1 candidates -0002, -0003, -0005, -0008 only.
- D-214 Budgets (section 9); one stage; scenes at most 16 frames; the per-step fold is the largest cost risk, measured in 05-04.
- D-215 CI levers: (a) nproc and CPU model in every tool-versions step (applied in 05-01); (b) dropped by D-226; (c) rejected; the owner says yes or no at publish time.
- D-216 Process rules R1..R12 and the cadence (section 7, section 12); the handbook (this file).
- D-217 Ten plans in nine waves; cut line at 05-08 with six triggers (section 11).
- D-218 Claims allowed after Phase 5 and the withheld words (docs-check and the README line, 05-01): pitch-, sound-, audio-, sample-, noise-, LFO-, volume-accurate, band-limited, sounds correct, plays music, authentic sound, audio-compatible.
- D-219 (amends D-192, D-208) Modulation while CTL != 0 and select 0 and 1 on; TRG 1 holds wave[1][0] - 16 and freezes select 1; TRG 0 lets it step; lfo_static rows judge a TRG = 1 run.
- D-220 (RF-5) Restart edges beyond key-on (DDA 1 -> 0, TRG 1 -> 0 of an uncoupled select 1) are constants with ledger LED-AUD-0004 and carry no MAN row.
- D-221 (RF-1, RF-8) A fold spans at most ds_max_step_mclk() (600), not 4.7 M; arithmetic on offsets from the fold point, never absolute times near 2^64.
- D-222 (RF-2, F-64) The write-local fold carrier: no state depends on a sink; a bare test bus is legal; strict-C11 initialisers stay valid. PR-1 rules it.
- D-223 (RF-3) d is read from the profile by one helper and stored in no region; a d = 3 snapshot into a d = 6 instance is refused by profile identity; 05-03 verifies and adds a refusal row if needed.
- D-224 (C-1..C-10) Counts follow the repo: 58 stages; CROWS_MAX raised in 05-03 with a control; withheld 35 -> 42; golden comments compacted in 05-03 Task 1; all constants rows in 05-03.
- D-225 (C-7) The .aexp header defines sample_mclk; twin scenes are named <scene>_twin.
- D-226 (C-9) Lever (b) dropped (it conflicts with rule-job-gate, needs cc65 and fixture-build); lever (a) only, a publish-time hand-off.
- D-227 (RF-12, RF-13) Long audio ROMs must not overflow the opcode_exercise insn sink (05-04 states the size); the selector grammar has an end anchor.
- D-228 (corrects D-224) seeded-prng-v4 is policy enum value 5 with no gap; the hostile row moves to policy-6=field; 15 PSG constants rows, 49 in all, record bound 191 of 192.
- D-229 (corrects D-216) Stop rule at about 80 percent of the context budget; 05-03 and 05-04 expect splits (05-11, 05-12); a sixth split goes to the orchestrator.
- D-230 (amends D-207) .aexp reader cap 32,768 bytes per file (one named constant, a 32,769-byte control refused fail-closed); .vexp keeps 16,384; byte budget and wc -c check per .aexp.
- D-231 (amends D-203) "TRG implies widx[1] = 0" is dropped: R6 writes under TRG 1 advance widx[1] legally; the held value is wave[1][0] - 16 whatever the index; 05-03 adds an accepted row.
- D-232 The D-230 oversized-.aexp control is an exit-2 control in the pf_empty_path_control pattern; run_controls keeps exit 0/1 for the others.
- D-233 (amends D-230; added by plan 05-12 as a recorded deviation) .aexp reader cap 49,152 bytes per file (one named constant; a 49,153-byte exit-2 control and an exactly-49,152-byte pass control); .vexp keeps 16,384; one comment line per group still holds.

## 3. Evidence rules

ER-1 image-verified cites only; ER-2 default assumed; ER-3 timing evidence is circular; ER-4 assertion classes; ER-5 self-consistency; ER-6 withheld words; ER-7 same-author oracles; ER-8 disagreements; ER-9 rights; ER-10 weakest class dominates; ER-11 a hash is a change detector; ER-12 symmetry split; ER-13 records are definitions.
- ER-14 Criterion-4 hold: no entry stating a PSG period, frequency, noise clock, LFO rate or sample timing passes assumed until the owner settles C1; MAN row classes are still allowed.
- ER-15 Disputed points (sum 15, zero values, select 6/7, reg $A..$F, R6 while playing, key-on, latch with chON 0, LFO wrap) are ASSUMED rows only, listed in tests/audio/DISPUTED; a MAN group touching one fails AUDIO_DISPUTED_POINT.
- ER-16 Converses are ASSUMED unless a page states them; audible rows are MAN at maximum level and at the P8-10 example (sum 4), ASSUMED at sums 5..14; a strict decrease per step is MAN.
- ER-17 tests/audio/KINDCLASS caps each kind's class (AEXP_CLASS_INFLATED); tests/audio/RULES lists every clause; a clause with no non-AF0 kill fails AUDIO_RULE_NO_ROW.
- ER-18 An arithmetic lint is E0 consistency, never an oracle; no gate-time generator of expected values is committed.
- ER-19 DSPE and DSPA fields are definitions; the runner parses them with its own offsets; PCM 0 for silence is a format definition.
- ER-20 An audio hash is a change detector: a golden case is valid only beside a literal case of the same ROM; evidence prints pcm_sha256 and psg_sha256 as E0.
- ER-21 No E2 statement about PSG output: the guest cannot read the PSG; every output verdict is a runner-side judge.
- ER-22 A supported entry needs, per clause, a MAN group in a guest-fixture case and no recorded in-page conflict; same-author decoders never back it alone.

## 4. Plan-time rulings (PR-1..PR-19)

- PR-1 (D-222 carrier, F-64): no struct ds_bus member. Stored PSGT distances stay relative to inst->machine_time - 1 for the whole step (inside a step machine_time is the step start). ds_psg_advance(inst, stamp, step_end, obs) is the one function at both fold points: the write path passes step_end 0 and processes every event with time <= stamp without rebasing;
  the step-end call passes step_end 1, folds to start + total - 1 and rebases to it. The PCM cursor lives in the pcm channel of struct ds_obs_log, set by ds_obs_begin to ceil(T0/192). Out-of-step bus writes (dl_windows, vf_tie_one) fold at the stamp with no rebase; D-203 ranges hold only between steps. RF-2 is a fallback only as a recorded 05-03 deviation.
- PR-2 The D-185 counter bookkeeping (key-on, freeze, DDA and TRG restarts, closed-form reload and widx catch-up) lands in 05-03 with register storage; 05-04 adds synthesis, emission and the judge (a recorded move of "channel stepping" from 05-04 to 05-03).
- PR-3 Two more constants rows psg_dda_release_restart 1 and psg_trg_release_restart 1, ledger LED-AUD-0004: 15 PSG rows, 34 -> 49, all in 05-03; CROWS_MAX 40 -> 64.
- PR-4 05-02 writes the pcm header count, lost, seq, frame_bytes 8, version 1, format 1; first and sample_mclk from 05-04 on.
- PR-5 fixtures/lib/psg.inc is created in 05-04 (macros and equates, never edited later); later shared code goes in a new include (psgmod.inc 05-07, psglfo.inc 05-09) or the fixture.
- PR-6 ".aexp class tags" of 05-01 means the P8 label predicate; fixture-lint does not parse .aexp; the .aexp class grammar belongs to audio-check (05-05) and the 05-04 runner refusal (F-86).
- PR-7 psg_effects.def lands in 05-03 (ds_psg_write returns the effect bits); DSPE emission in 05-04.
- PR-8 LED-AUD-0009 joins the fixed withheld list (text.c and reckinds.c withheld_expected) in the 05-03 bump; psg_vol.def lands in 05-04.
- PR-9 seeded-prng-v4 lands in 05-03 Task 3 as enum value 5; the hostile row policy-5=field becomes policy-6=field.
- PR-10 The const sample function is ds_psg_sample(const struct ds_instance *, uint32_t *l, uint32_t *r); no core-boundary rule (the compiler and toy psg-emit-mutates enforce it, C-8).
- PR-11 Chunk tokens "191" and "193" join cs_table in 05-04; one frame is vframe.
- PR-12 Fixture ids: manifest psg_tone, fixture authored:psg-tone, ROM build/gate/fixtures/psg-tone.pce, literals build/gate/fixtures/psg-tone.aexp.tsv; the same shape for psg-mod and psg-lfo.
- PR-13 .aexp additions: header row "aexp sample_mclk=192"; twin scenes <scene>_twin; end anchor to=ev:<selector>[+<mclk>]; case-level expect_red=<gid,...> in the case expect string.
- PR-14 vd_policies (vdckinds.c) switches from v3 to v4 seeds with the guest list (video draws unchanged: v4 draws the v3 bytes first).
- PR-15 Audio toys are registered and watched by audio-check (psg.c, obs.c, cpu.c anchors); their EXPECT names audio cases only (F-72).
- PR-16 Only revision plans 05-16+ count toward cut trigger (1).
- PR-17 05-01..05-09 list AUD ids under requirements_advanced; 05-10 completes them (AUD-01 stays advanced at a cut).
- PR-18 Stage count 57 -> 58 (C-1); the selftest count is computed from the 05-05 run (C-10).
- PR-19 CI lever (b) dropped (D-226); lever (a) only, a publish-time hand-off.

## 5. Core facts

Registers (offset & $0F): 0 R0 select bits 2-0 ($07); 1 R1 LMAL 7-4 RMAL 3-0 ($FF, global); 2 R2 F low ($FF); 3 R3 F high 3-0 ($0F); 4 R4 chON 7, DDA 6, AL 4-0 ($DF); 5 R5 LAL 7-4 RAL 3-0 ($FF); 6 R6 wave data 4-0 ($1F); 7 R7 NE 7, NFRQ 4-0 ($9F, select 4/5 only); 8 R8 FLF ($FF, global); 9 R9 TRG 7, CTL 1-0 ($83, global); $A..$F none.
Never dropped: R0, R1, R8, R9. Dropped at select 6/7: R2..R7. R7 dropped at select 0..3. Power-on: all 0, nrem 1, LFSR 1.

Periods at d = 6 (master clocks; samples = /192): step 6F (F 256: 1,536 = 8 samples); cycle 192F (F 254: 48,768 = 254 samples; F 256: 256 samples, 128 at d = 3); noise 384 x NFc (NFRQ $00 -> NFc 31: 11,904 = 62 samples; $18 -> NFc 7: 2,688 = 14); coupled step 6 x F1 x FLF (F1 128, FLF 150: 115,200 = 600 samples; max 6,291,456).
F0' examples: F0 512, w $18: CTL 1 -> 520, CTL 2 -> 544, CTL 3 -> 640; w $08, CTL 1 -> 504. LFO sum before the mask 3,840..8,431.

Grid (D-187): M 1, 191, 192 -> 1 sample; 193 -> 2; 357,630 -> 1,863. 262 lines (L 357,630 = 1,862 + 21/32): 32 frames = 59,605 samples (21 x 1,863 + 11 x 1,862), frame 0 1,863, starts 0, 1863, 3726, 5588, 7451. 263 lines (L 358,995): 64 frames = 119,665 (49 x 1,870 + 15 x 1,869). Per-call first = ceil(T0/192), seq = ceil(T1/192) - ceil(T0/192).

DSPE v1: +00 u64 stamp, +08 u16 pc, +10 reg, +11 value, +12 sel, +13 chan (0..5, $FE global, $FF none), +14 effect (b0 WAVE_STORE, b1 DDA_LOAD, b2 WAVE_HELD, b3 IDX_RESET, b4 LFO_RESET, b5 NO_TARGET, b6 UNMODELLED), +15 wave_idx ($FF if chan > 5), +16 u16 post ($FFFF NO_TARGET), +18 version 1, +19..+23 zero.
DSPA v1: frame u32 L, u32 R; header +00 count, +04 lost, +08 u64 seq, +16 u64 first, +24 u32 sample_mclk, +28 u16 frame_bytes, +30 version, +31 format.

VOL[a] = round-half-up(32768 x 10^(-3a/40)), a = 0..29; integer rule: v with (2v - 1)^40 x 10^(3a) <= 2^640 < (2v + 1)^40 x 10^(3a); no tie exists.
32768 27571 23198 19519 16423 13818 11627 9783 8231 6925 5827 4903 4125 3471 2920 2457 2068 1740 1464 1232 1036 872 734 617 519 437 368 309 260 219.
att = 2S + 1 - b with S the P8-10 sum and b = AL bit 0, so att >= 30 iff S >= 15. P8-10 example: R1 $C8, AL $1C/$1D, R5 $F8: left S 4, att 9/8; right S 15 (ASSUMED silent). Pan pair LMAL F, AL $1D, LAL 0: left S 16 (MAN silent).

LFSR step: b = (s ^ s>>1 ^ s>>11 ^ s>>12 ^ s>>17) & 1; s = (s >> 1) | (b << 17); taps 137,219. Output (old bit 0) = parity of post-shift bits under mask $30C01 (RF-6), so PSGT needs no stored bit.
States before steps 1..20: $00001 $20000 $30000 $38000 $3C000 $3E000 $3F000 $1F800 $0FC00 $07E00 $03F00 $01F80 $00FC0 $207E0 $303F0 $381F8 $3C0FC $3E07E $1F03F $2F81F; outputs 10000000000000000011; after step 20 $37C0F.
RF-11 boot closed form (no PSG write, NFc 31, period 11,904): n = 0 at M = 0, else floor((M - 1)/11904) + 1; LFSRs = step^n(1); nrem = n x 11904 - (M - 1) (1 at M = 0). Examples: M 1 -> n 1, LFSR $20000, nrem 11,904; M 11,905 -> n 2, $30000; M 357,630 -> n 31, $107EF, nrem 11,395.

## 6. File map (plan)

src/core: ds_psg.h (05-02); psg.c (05-02 inert, 05-03 decode and counters, 05-04 synthesis, 05-06/07/09); psg_effects.def (05-03); psg_vol.def (05-04); obs.c, ds_obs.h, run.c (05-02, 05-04); ds_core.h, bus.c, cpu.c (05-02, 05-03); serial, snapshot, poweron, boot, constants.def, text (05-03).
tests/conformance: audiokinds.c (05-04 create, 05-05..05-09); amended kinds and goldens (05-02, 05-03); runner registration (each kind plan). fixtures: psg.inc (05-04); psg_tone, psg_mod, psg_lfo .s, .checks, .aexp, manifests (05-04, 05-07, 05-09).
scripts: audio-check, aexp-lex.awk, aexp-rules.awk, tests/audio/*, AUDIO_TOY_EXEMPT, check stage and selftests (05-05); fixture-build write_aexp, conformance-run audio_out_check (05-04). Toys 05-04..05-09. 05-01: cites, TOOLS, fixture-lint, ledger, docs-check, README, gate.yml, this file.

## 7. Cadence and the per-plan gate

Per task: build, sh scripts/check --stage for the touched stages, the named cases through build/gate/ds_runner and the task's red control; commit when green. Full gate before a task commit only for a golden, digest literal, version, ledger status, README supported list, workflow or gate rule change.
Per plan end, once: sh scripts/check, sh scripts/check --selftest, sh scripts/audit-history --range <start>..HEAD, and for C plans LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro. 05-08 and 05-10 add dash, the second awk and all six Linux parts. One gate per checkout at a time; an end-of-plan failure is fixed in a separate commit.
Carried: D-177 (a)-(g); core: no malloc, file I/O, clocks, env, floats or mutable statics; defined behaviour, strict C11; new core files include only stddef/stdint/ds_core.h. At most 3 tasks per plan.

## 8. C11 and gate footguns

- gcc 13 strict C11: bounded snprintf with %.Ns (-Wformat-truncation), no strtok_r or other POSIX calls, no memcpy from NULL, -Wmissing-field-initializers on positional initialisers, -Wconversion on every u32 to u8/u16 store; widen to unsigned before shifts (the sanitize preset has no -fwrapv).
- F-80 toy anchors stay unique (control-mutate fails on n != 1) in bus.c, run.c, obs.c, serial.c, snapshot.c (no PSG line may contain "inst->p = p;"), cpu.c (the ds_vdc_advance line) and boot.c; 05-PATTERNS F-80 lists them.
- F-56: record-lint refuses a golden over 16,384 bytes; compact history comments in 05-03; never exactly 16,384.
- ROM-SIZE shapes (audit-history): no tracked blob of 4,096 bytes or more that is a multiple of 8,192 or 8,192 + 512; wc -c every new or grown file.
- PK_LIST_MAX 512 per case list; 400-character lines everywhere; ASCII; SPDX (scripts MIT OR Apache-2.0, data 0BSD).
- mawk printf %d fails above 2^31 - 1: print large integers with %.0f or as strings; POSIX awk only (no \b, no IGNORECASE); no awk on the left of a pipe.

## 9. Live budget table (each SUMMARY repeats it with its own values)

| Budget | Before Phase 5 | Phase 5 target | Rule |
|---|---|---|---|
| Gate stages | 57 | 58 (audio-check, 05-05) | C-1, PR-18 |
| Power-on policies | 4 | 5 (v4 = enum 5) | D-228 |
| .aexp reader cap | none | 49,152 bytes (D-233) and 512 rows per file; .vexp 16,384 | D-230, D-233 |
| Constants rows / CROWS_MAX | 34 / 40 | 49 / 64 | D-224, D-228, PR-3 |
| Withheld ids | 35 | 42 | D-224 |
| Record rows (skeleton / 16 events / cap) | 153 / 169 / 192 | 175 / 191 / 192 | D-228 |
| Regions; serial / epoch / text | 21; 8 / 5 / 7 | 24; 9 / 6 / 8 (once, 05-03) | D-202 |
| Snapshot / instance bytes | 92,255 / 1,141,096 | 92,553 / 1,141,376 | RF-10 |
| Run request bytes (LP64) | 56 | 72 | RF-17 |
| OX_ROMS_MAX / guest ROMs | 40 / 36 | 48 / 39 | D-214 |
| DSV ids | 1..26 | 27 psg_tone, 28 psg_mod, 29 psg_lfo | D-212 |
| Toys | 26 | 33 (32 after a cut) | D-210 |
| Ledger entries / REQUIRED lines | 63 / 63 | 75 / 75 | D-213 |
| Conformance REQUIRED cases | 334 | about 379 to 394 | D-214 |
| CI critical path | 562 s (4.1 run) | about 587 to 612 s; job trigger 600 s | D-214, D-226 |
| Local warm gate user CPU | 270.0 s (05-01, load 5.7-9.1) | +10 to +20 percent; trigger (3) over +25 percent | D-214, D-217 |

Checkpoint A (05-08): stages 58, constants 49/64, withheld 42, regions 24, ROMs 38/48, DSV 27-28, toys 32, REQUIRED 388, supported 14; user CPU 317-358 s: +20.0 / +32.4 percent in two same-hour pairs (load 6-18); runner instructions +22 / +25 percent; CI unverified (525-670 s).

## 10. Discrepancy list (ledger disagreement field)

DX-1 (LED-AUD-0001); DX-2 (-0002); DX-3 (-0010); DX-4, DX-16 (-0011); DX-5 (-0008); DX-6, DX-7 (-0009); DX-8, DX-9, DX-10, DX-13 (-0004; DX-10 also -0012); DX-11 (-0012); DX-12 (-0005); DX-14 (-0007); DX-15 (LED-CPU-0013). The texts are in those ledger fields.

## 11. Cut line (D-217)

Cut at 05-08 if any of: (1) more than two revision plans (05-16+); (2) a CI work job over 600 s after levers (a) and D-176 (1); (3) local warm user CPU over +25 percent of the 05-01 baseline (same machine, load); (4) the LFO needs a new model row or an unsettled discrepancy;
(5) an unbounded LFO fold; (6) an owner preference for an earlier pull request.
Then 05-09 becomes plan 5.1-01 of an inserted Phase 5.1 (psg_lfo, psg-lfo-shift), 05-10 closes Phase 5 with the LFO under DSPE UNMODELLED, and ROADMAP criterion 1 gains "LFO registers logged and decoded; LFO modulation in 5.1". Cut order: LFO first, then the noise repeat case to two policies; never DDA, period, order or silence rows.
05-08 record (docs/ci-timings.md): (1) 0; (2) no CI run; (3) borderline (+20.0 / +32.4 percent); (4)-(6) no. Recommendation: cut (LFO to 5.1); the orchestrator decides.

## 12. Process rules and the stop rule

R1 class each literal when written (MAN:P8-n with an img row, GUEST:<model>, ASSUMED:LED-AUD-x); R2 weakest class wins (curve, LFSR, tie order, tick phase, zero conventions are ASSUMED); R3 every behaviour rule has a literal row and a named killing mutant or fault, or is a named limit;
R4 every enforcement rule lands with a control red when the rule line is deleted; R5 a toy EXPECT names a literal group shown failing; R6 a new fixture brings chunk, observer, restore and non-interference coverage; R7 no literal from a DuoSpool run; R8 README, validation and ledger text from gate output;
R9 unsigned widening before shifts, bounded snprintf, no POSIX; R10 PK_LIST_MAX 512, 400-character lines, ASCII, SPDX; R11 a behaviour change after a push bumps the epoch; R12 same-author decoders and scripts are E0/E2.
Stop rule (D-229 supersedes D-216's 60 percent): stop only on concrete evidence at about 80 percent of the context budget; commit, write a partial SUMMARY and propose the split. 05-03 and 05-04 expect splits into 05-11 and 05-12; later splits take 05-13..05-15; a sixth split stops the run and goes to the orchestrator.
