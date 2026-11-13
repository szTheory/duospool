# Phase 5 plan check 1 (gsd-plan-checker, round 1 of 3)

Phase: 05-psg-raw-audio. Plans checked: 05-01 .. 05-10 (10 files at fe97155). Inputs: 05-CONTEXT (D-182..D-228, ER-14..ER-22, errata D-219..D-228, which win over the text above them), 05-RESEARCH (RF-1..RF-17, sections 1-7), 05-PATTERNS (C-1..C-11, F-51..F-86), ROADMAP Phase 5 (four success
criteria), REQUIREMENTS AUD-01..03, AGENTS.md, .claude/CLAUDE.md, the three Phase 4 plan-check reports (format), the Phase 4 SUMMARYs (actuals) and the repository at HEAD.

Result: ISSUES FOUND, verdict REVISE. 9 blockers, 8 warnings, 9 info. The architecture is sound (tracer first, judge second, layout final in 05-03, cut line at 05-08), every numeric literal recomputed below is right, and the F-64 carrier ruling (PR-1) meets D-222. The blockers are one
locked-decision contradiction carried through two plans and the ROADMAP (D-228), one missing negative control (D-204), four fixture or row defects that cannot pass or are class-inflated as written (05-04 silent_pre, 05-06 pan phase C, 05-07 noise scenes, 05-09 lfo_dynamic), one verify that can never
pass, the stop-rule threshold against D-216, and the missing validation strategy file.

## Mechanical checks run

- verify.plan-structure on 10 plans: valid true, errors [], warnings []; tasks 3 in nine plans, 2 in 05-02; 29 tasks, each with files, action, verify and done. No plan exceeds 3 tasks.
- Waves: 1:[01,02] 2:[03] 3:[04] 4:[05] 5:[06] 6:[07] 7:[08] 8:[09] 9:[10]; every wave equals max(depends_on)+1; no cycle, no forward or missing reference. The only same-wave pair is 05-01/05-02: files_modified intersection is empty (05-01 touches fixtures/lib, scripts/lib, scripts/docs-check, ledger,
  README, handbook, workflow, docs/ci-timings.md; 05-02 touches src/core and tests/conformance). No undeclared shared mutable resource (05-02 reads PR-1/PR-4 as plan text, not as an artefact).
- files_modified equals the union of task <files> in 10 of 10 plans (script, 0 differences).
- Stage and selftest names: every --stage name exists in scripts/check except audio-check, and every --selftest name except mutation-audio-hash-only, mutation-audio-group-vacuous and mutation-aexp-class-lint; all four are created by 05-05 Task 1 and first used there.
- Case ids in verify commands: every id exists at HEAD or is created by the same or an earlier plan, except `mmu_mpr` in 05-03 Task 3 (blocker 7). Every CONTROL name is a toy the same plan creates.
- Count and match gates run against HEAD (read only): none passes at HEAD except the 05-08/05-10 guards that LED-AUD-0001/-0006/-0011 are not supported and the 05-10 "no PENDING:5" guard, which are invariants made meaningful by 05-01, not measurements.
- 26 <automated> blocks; 17 span two lines, each broken after `&&` (valid sh continuation; info 5). No `2>/dev/null`, `|| true` or `|| echo`; no <fails_when> (carried, info 9).
- 10 of 10 plans: 0 lines of 400 or more characters, 0 non-ASCII bytes; absolute home and temp paths appear only in the rule sentence that forbids them and in grep guards.
- estimate-check --calibrated (budget 100,000, confidence low, no calibration): over 05-03 150k (1.5x), 05-04 160k (1.6x), 05-05 130k, 05-07 130k, 05-06 120k, 05-09 115k; under 05-01 95k, 05-02 85k, 05-10 80k, 05-08 75k. Advisory (warning 1).
- Repository counts measured: 57 run_stage lines (58 after audio-check, C-1); 334 REQUIRED case ids (339 lines with the header); 63 ledger/REQUIRED lines; 34 DS_CONST rows; CROWS_MAX 40u; REC_MAX_ROWS 192u; OX_ROMS_MAX 40u with 36 distinct authored (ROM, request) guest pairs (39 after psg_tone,
  psg_mod, psg_lfo, of 48); 26 toys; 7 ds_obs_begin callers (run.c:108, obskinds.c 1633/1725/2237/2272/2281, vdckinds.c:359); enum ds_policy ends at DS_POLICY_SEEDED_V3 = 4; snapshot_refusal_fields carries `policy-5=field`; goldens 16,065 and 16,099 bytes (cap 16,384, so the 05-03 Task 1 compaction is
  needed).
- No 05-VALIDATION.md in the phase directory (blocker 9). RESEARCH "## Open Questions" has three items without a RESOLVED marker (warning 8).

## Numeric re-derivation (all match the plan text)

- Tracer (05-04): step 6 x 256 = 1,536; cycle 49,152 = 256 x 192, so c_k = ceil(w'/192) + 256k. With w - 5 <= w' <= w: c1's event w' + 49,152 < w + 50,000; c2's w' + 98,304 > w + 50,000; c6's w' + 294,912 < w + 300,000; c7's w' + 344,064 > w + 300,000. c2..c6 give k = 4, d = 1,024. At d = 3 the cycle
  is 24,576 = 128 samples; crossings m = 3..12 fall in the window and the first-to-fifth distance is 512 (expect_red). The table is a permutation of 0..31 (sum 496), words 0..15 >= 16 and 16..31 <= 15, so the only rising crossing of the 15.5-level threshold is word 31 -> 0; hex and closed form agree.
- Event rows (05-04): R4 $40 -> (0, $08, 0, $0040); R6 k -> (0, $01, k & 31, word k-1); R3 $01 after R2 $00 -> post $0100; R1/R5 $FF -> $00FF; R4 $9F -> $009F. Correct under D-196.
- Per-frame counts: 262 lines, frames 0..31 = 1863 1863 1862 (repeating) ... 1863 1862, 21 x 1863 + 11 x 1862 = 59,605, starts 0, 1863, 3726, 5588, 7451, 9314, 11176, 13039, 14902; 263 lines, frames 0..7 = 1870 1870 1870 1870 1869 1870 1870 1870, 49 x 1870 + 15 x 1869 = 119,665 per 64.
- LFSR from seed 1 (taps 0/1/11/12/17 = 137,219): states $20000, $30000, $38000, $3C000 ...; state after 31 steps $107EF; output bits 1000000000000000001111110000001111101111 (the RESEARCH prefix); period 131,071; RF-6 mask $30C01 recovers the shifted-out bit. Boot rows (05-03): T = 0 -> $20000, nrem
  11,904; T = 11,904 -> $30000, 11,904; T = 357,629 -> 31 events, $107EF, nrem 369,024 - 357,629 = 11,395.
- Counter rows (05-03): 100,003 mod 6 = 1, w' = 100,002; (107,681 - w') / 1,536 = 4.9993 -> widx 4; 107,682 -> exactly 5 steps -> widx 5. The pair catches an off-by-one in either direction.
- Q15 (VOL[a] = round-half-up(32768 x 10^(-3a/40))): all 30 values recomputed exactly; the integer rule (2v-1)^40 x 10^(3a) <= 2^640 < (2v+1)^40 x 10^(3a) holds for every row. Mix bound 6 x 31 x 32,768 = 6,094,848; levels_exact 31 x 32,768 = 1,015,808 and 31 x VOL[8] = 31 x 8,231 = 255,161.
- P8-10 rows (05-06): R1 $C8, AL $1D, R5 $F8: left S = 3 + 1 + 0 = 4, att 8; right S = 7 + 1 + 7 = 15, att 30; R1 $C7: right S = 16, att 32. Pan: R1 $FF, AL $1D, R5 $F0 gives left S = 1, right S = 16 (the CONTEXT specifics example). Twin F + 1 = 129: 4 cycles 516 against 512.
- Noise (05-07): R7 $98 -> NFRQ 24, NFc 7, clock 2,688 = 14 x 192; R7 $91 -> NFc 14, 5,376 = 28 x 192; window start 11,904 + 192 = 12,096; 11,904 = 62 x 192, and because 2,688 is a multiple of 192 the run grid is exact whatever the event phase. DDA sum 115.
- LFO (05-09): F0' = 520, 544, 640 (w $18, s 0/2/4), 504 (w $08), 16 + 4096 + 0 - 16 = 4096 -> 0 -> 4096; two-cycle distances 1,024/1,040/1,088/1,280; step 6 x 128 x 150 = 115,200 = 600 samples; FLF 0 -> 196,608; sum range 3,840..8,431; first reload after an R9 write within 6 x 640 = 3,840, and the
  next crossing within 192F0' + 720 of the write, before the window start 192F0' + 2,000.
- Layout and stream: PSG0 42 + psg_pad 6 + PSGW 192 + PSGT 40 = 280 (PSGT at block offset 240, 4-aligned; 1,141,376 = 8 x 142,672, no tail padding); instance 1,141,096 -> 1,141,376; snapshot 92,255 + 3 x 8 + 274 = 92,553; request 56 -> 72. Record rows 60 + 1 + nids + 4 + 1 + 2 + events + 4 x models +
  consts: 153 today (35, 34), 175 skeleton and 191 with 16 stimulus events after (42, 49), of 192. Constants 34 + 15 = 49; withheld 35 + 6 constants ids + LED-AUD-0009 = 42. PSGW____ = $505347575F5F5F5F. Toys 26 + 7 = 33 (32 at a cut). Periods: 6 x 4096 = 24,576; 6 x 4096 x 256 = 6,291,456; 6 x 64 x
  31 = 11,904.

## Locked-decision coverage (check 1)

Covered by at least one task: D-182 (01), D-183 (01, 03, 04), D-184 (03, 07, 09), D-185 (03, 04, 09), D-186 (02, 03, 04), D-187 (04), D-188..D-190 (03), D-191 (04, 07), D-192 (03, 09), D-193 (03, 07), D-194 (04, 06), D-195 (03, 04, 09), D-196 (03, 04), D-197..D-199 (02, 04), D-200 (04, 07, 09),
D-201 (04), D-202 (03), D-203 (03), D-205 (03, as amended to 15 rows), D-206..D-211 (04..07, 09), D-212 (except below), D-213 (01, 08, 10), D-214 (01, 04, 08, 10), D-215/D-226 (01), D-217 (all; PR-2 deviation recorded), D-218 (01, 10), D-219..D-225 and D-227 (01, 03, 04, 05, 09).

Not covered or contradicted: D-228 is cited by no plan and its policy ruling is contradicted (blocker 1; its constants half is delivered through PR-3). D-204/D-212 `aud_poweron_policy_diff` has no plan (blocker 2). D-216's stop threshold is contradicted (blocker 8). No deferred idea is implemented; no scope-reduction wording (v1, static, placeholder) was found.

## Blockers (these properties must hold before execution)

1. [context_compliance] seeded-prng-v4 is enum value 5 with no gap, and the Phase 4 hostile row moves to `policy-6=field` (D-228, which corrects D-224).
   Evidence: the plans implement the superseded D-224 reading (v4 = 6, value 5 refused, new `policy-7=field`). Every place: 05-01 Task 2 line 170 ("policy 6 internally is a 05-03 detail"); 05-01 Task 3 line 211 (PR-9: "enum value 6 (D-224): value 5 stays unassigned and refused, so the hostile row
   policy-5=field stays unchanged and a new row policy-7=field ..."); 05-03 truth line 75; 05-03 Task 3 line 246 ("DS_POLICY_SEEDED_V4 = 6 (5 unassigned, refused)") and line 247 ("dec_powr accepts 1..4 and 6, refuses 5 and > 6"); 05-03 Task 3 line 250 ("plus policy-7=field (the existing policy-5=field
   row stays unchanged ...)"); 05-03 Task 3 verify line 258 (`grep -q 'DS_POLICY_SEEDED_V4 = 6'`). The handbook will not carry D-228 either: 05-01 truth line 58 and section (2) line 193 say D-182..D-227, and the Task 3 verify loop (line 224) is `seq 182 227`. Outside the plans: ROADMAP.md line 240 says
   "seeded-prng-v4 (policy 6)" (orchestrator file).
   Example fix (non-binding): v4 = 5; dec_powr accepts 1..5 and refuses above 5; snapshot_refusal_fields `policy-5=field` becomes `policy-6=field`; drop the policy-7 row; PR-9 and the LED-PROF-0002 note say 5; verify greps `= 5`; handbook covers D-182..D-228 (`seq 182 228`).

2. [requirement_coverage / R4] The zero-versus-v4 identity rule of the audio kind has a negative control that can go red: `aud_poweron_policy_diff`, a channel with no upload whose output differs between v4 seeds 1 and 2 (D-204, D-212 C-level list).
   Evidence: the case appears only in the 05-03 stop-rule fallback text (line 111, "kept as a control"); no task creates it, no files_modified lists it, no REQUIRED line. Every audio literal case from 05-04 on asserts identical PCM/DSPE under zero and v4 seed 1, and nothing shows that assertion can fail.
   Example fix (non-binding): 05-04 Task 1 or 2 adds a synthetic `aud_poweron_policy_diff` case (key on a channel whose wave RAM is never written; require different DSPA bytes under v4 seeds 1 and 2), with its case file and REQUIRED line.

3. [verification_derivation / ER-15, ER-16, R3] A MAN silent row's silence rests only on its stated basis, and the tracer's chON-gate scratch mutant can turn it red.
   Evidence: 05-04 Task 1 step (c) writes R4 $40 then R4 $00, so throughout `silent_pre` (R1 $FF to the R4 $9F key-on) channel 0 has AL = 0: att = 0 + 31 + 0 = 31 >= 30 and the P8-10 sum is exactly 15, a DISPUTED point. With chON ignored the window is still silent, so the <done> demonstration
   "silent_pre_L red under a chON-gate mutant" cannot succeed, and a MAN:P8-4 group rests on the sum-15 boundary that the lint cannot see (the row carries only chon=0).
   Example fix (non-binding): make the second R4 write $1F (chON 0, DDA 0, AL $1F) so channel 0 sits at S = 0 with chON = 0; its ev_decode row becomes (0, $00, 0, $001F); give the silent rows r1/al/r5 inputs too.

4. [verification_derivation] Every row of a pan-pair phase can pass on a correct synthesiser (05-06 Task 1, truth line 49).
   Evidence: phase C keys both channels: a hard left (R5 $F0) and b hard right (R5 $0F). L then carries a at S = 1 and R carries b at S = 1, so neither side is all zero; the planned "silent rows on each muted side (MAN:P8-10, S = 16, from the later key-on + 2,000 to the first key-off)" are red on a
   correct core. The twin derivation (Task 3, "the muted-side silent rows stay green") inherits the error. RESEARCH section 6 pans ONE channel ("LMAL F, AL $1D, LAL 0: left S 16 MAN silent; RAL F: right live").
   Example fix (non-binding): C1 a alone hard left (R silent MAN:P8-10 at S = 16, L period ASSUMED at S = 1), C2 b alone hard right (L silent MAN), C3 both (isolation rows only, ASSUMED); re-derive the twin's expect_red set.

5. [context_compliance D-212, D-204] Every channel a fixture enables gets all 32 words and every register before it can sound, so the zero and v4 seed 1 streams are identical.
   Evidence: 05-07 Task 1 keys select 4 on ("R1 $FF, R5 $FF, AL $1F, chON 1, DDA 0") and only then writes R7 = $98; select 4's wave RAM and R2/R3 are never written (F = 0, a DISPUTED value). Between the R4 and R7 stamps (about 84 master clocks for sta abs + lda # + sta abs at low speed) a sample
   instant falls with probability about 0.44 and reads drawn wave RAM under v4, so audio_mod_noise4 and audio_mod_noise5 fail their identity check. D-212 requires the upload regardless.
   Example fix (non-binding): for selects 4 and 5 upload 32 words by procedure (2), write R2/R3 with F a multiple of 64, write R7 (NE 1) before the key-on.

6. [ER-10 / R2 / D-208] A MAN group never depends on the tie order or a counter-restart edge, and LFO switch timing is not judged.
   Evidence: 05-09 Task 2 `lfo_dynamic` (MAN) excludes cycles that contain "anchor R9 $01 plus 16 x 115,200 x j" and requires every other complete cycle to be 200 or 184. Those instants are true only under the ASSUMED TRG-release restart (D-192 "(ASSUMED)", D-220 "no MAN row"); under another restart
   the switches move by up to 600 samples (three select-0 cycles), a mixed cycle is judged, and the row fails. So the verdict rests on an edge convention (R2) and on the switch instant (D-208 "LFO switch timing not judged").
   Example fix (non-binding): take the straddling cycles from the stream (maximal runs of equal cycle length; values in {200, 184}; at most one cycle strictly between them per switch; both present; runs of at least 40), or class the group ASSUMED:LED-AUD-0011.

7. [task_completeness] Every `auto` task's verify can pass on a correct tree.
   Evidence: 05-03 Task 3 verify (line 257) requires `RESULT mmu_mpr pass`; no case `mmu_mpr` exists (the fixture mmu_mpr runs as mmu_mpr_256k; also mmu_mpr_384k, mmu_mpr_1m). The loop prints "red mmu_mpr" and exits 1 on every run.
   Example fix (non-binding): name mmu_mpr_256k (the case whose check 31 writes $6805), plus the 384k and 1m variants if wanted.

8. [context_compliance D-216 / scope_sanity] Each plan stops and splits near 60 percent of context at its named task boundary (D-216).
   Evidence: all ten stop_rule blocks and the standing "Stop rule calibration" line say "stop only on concrete evidence ... (context above about 80 percent, repetition, truncated tool output)". No erratum amends D-216. Phase 4 actuals: 04-03 (the bump, est. 60k) and 04-04 (the tracer, est. 70k) both
   stopped after their first large task at the 60 percent rule and split into 04-18 and 04-19; 05-03 (150k) and 05-04 (160k) carry more than either. At 80 percent an executor enters 05-03 Task 3 or 05-04 Task 2 with too little room and has no stop inside a task.
   Example fix (non-binding): restore "after Task N is committed, stop and split if context is at or above about 60 percent" in every stop_rule and standing rule, or record an erratum amending D-216 first.

9. [nyquist 8e] A validation strategy file exists before execution.
   Evidence: no 05-VALIDATION.md; config nyquist_validation true; 05-RESEARCH has "## Validation Architecture" (line 524). Phase 4 wrote 04-VALIDATION.md at research time (df20aa5) and signed it at close. 05-10 Task 3 creates the file only at close-out.
   Example fix (non-binding): the orchestrator writes 05-VALIDATION.md (draft: per-task automated map of the 10 plans, Wave 0 list, sampling) from RESEARCH before execution; 05-10 Task 3 updates and signs it. No planner edit beyond that wording.

## Warnings (these properties should hold)

1. [scope_sanity] Six plans exceed the 100k budget (05-03 1.5x, 05-04 1.6x, 05-05/05-07 1.3x, 05-06 1.2x, 05-09 1.15x). 05-03 Task 2 is 04-03 Tasks 1+2 plus the noise fold and the D-203 rules; Task 3 is 04-18 plus decode, counters, effects and five kinds; 05-04 Task 1 is 04-04 Task 1 (about 60 percent
   alone) plus synthesis, both emissions, the .aexp channel and both stream builders. Expect 05-11 and 05-12. Five spare numbers serve six over-budget plans; say what a sixth split does. 05-03 Task 1 (golden compaction, CROWS_MAX) touches no 05-01 file and could move into 05-02.
2. [cross-plan contract, dimension 9] 05-02 Task 1 makes pcm_chan_append assert header `first` and `sample_mclk` = 0; 05-04 Task 1 makes ds_obs_begin write sample_mclk 192, so a REQUIRED case goes red under the Task 1 verify. Example fix: 05-04 Task 1 lists obskinds.c and updates that assertion (sample_mclk 192, first = ceil(T0/192)).
3. [task_completeness] 05-06 Tasks 1 and 2 change psg_tone while audio_tone_golden (REQUIRED, created in 05-05) pins its streams; --stage conformance stays red until Task 3 regenerates. Example fix: regenerate (old and new recorded) in each task that changes the ROM.
4. [audio-check data] KINDCLASS page sets disagree with later rows: `same` lacks P8-2 (05-07 r7_ignored is MAN:P8-2+P8-9); `live` and `period` lack P8-10 (05-06 S = 4 rows are MAN:P8-10+P8-9+H8-23). If AEXP_CLASS_INFLATED compares page sets, audio-check fails in 05-06/05-07. Example fix: widen the sets or define the rule as rank-only in 05-05.
5. [ER-17, ER-22] LED-AUD-0008's clause "strictly decreases per R1 step" has no planned row (tone_atten covers LAL F/E/D and AL $1F/$1E only). Example fix: an R1 pair (R1 $FF against $EF on the left, AL $1F, R5 $FF) as atten_order MAN in 05-06 Task 2.
6. [ER-22] LED-AUD-0003's "fields and masks" clause has no MAN guest row: no guest write sets a masked bit (tracer R4 $9F, R6 <= $1F; 05-06 unspecified), and C-level psg_decode does not count. Example fix: 05-06 ev rows writing don't-care bits, e.g. R0 $F9 (post $0001), R3 $F1 (F $1xx), R4 with bit 5 set, R6 $E5 (word 5), R7 $78 at select 4 (post $0018), R9 $7C (post $0000).
7. [R2] 05-07 dda_hold asserts the change "in the first sample after the write stamp (a sample at the stamp sees the old level)", which is the tie order whenever a stamp is a sample instant. The timer period is 16(R+1) samples, so all nine writes share one phase mod 192 (stamps are multiples of 12,
   about a 1 in 16 chance). Example fix: state that phase from a hand cycle count (non-zero), or accept the sample at the stamp or the next.
8. [research_resolution] RESEARCH "## Open Questions" (RF-3 restore, RF-4 TRG, RF-2 carrier) lacks RESOLVED; resolved in substance by D-223, D-219 and D-222. Example fix: mark the heading and point at the errata.

## Info

1. ER-18 ruling on noise_seq (asked): it stays inside ER-18. The only expected values are the committed 256 bits, drawn outside the repository by a script that first reproduces the RESEARCH 40-bit prefix (R7). The judge computes positions only (event ordinals from the row inputs p0=11904;p1=2688 and
   the R7 anchor), the same selector arithmetic D-207 allows for master clocks to sample indices. It is runner code that shares nothing with the core, and the group is ASSUMED (boot event phase and reload rule). Conditions: an ordinal beyond 256 fails closed (never wraps), and the row comment states
   the largest ordinal its window needs (about floor(w/11,904) + 1 + 112 here).
2. F-64 carrier (PR-1) against D-222: it holds. Machine state never reads the obs log; the PCM cursor lives in the pcm channel and is set from T0 at begin, so first(i+1) = first(i) + seq(i) by construction. No ds_bus member means no positional initialiser changes and no NULL dereference on dl_windows'
   zeroed bus. "A NULL carrier means no PSG fold" is vacuous: bare-bus writes fold at stamp = machine_time, harmless (at most the event at E), and F-64 says psg_fold must not assert ranges after them. Direct-call C cases (stamps up to 107,682 on a fresh instance) stay far below the u32 offset limit.
3. Planner flags accepted: pan audible sides at S = 1 ASSUMED with MAN rows on two S = 0 solo phases (ER-16 strict reading; the CONTEXT specifics example itself gives S = 1); counter bookkeeping in 05-03 (PR-2; D-217 also puts every D-203 rule in 05-03, and "rem = 0 exactly when frozen" needs key-on
   bookkeeping); D-220's two extra rows (49, bound 191 of 192, D-228); psg_pure_function in 05-06 Task 2 (D-217 gives it no plan; literals 6, 1,524, 20,526, 24,570 and 192, 48,768, 656,832, 786,240 check).
4. 05-03 wording: Task 2 defines next = E + nrem - 1, Task 3 says "offset = next - E" for key-on. The 107,681/107,682 rows catch an off-by-one; say "stored distance = next - E + 1".
5. 17 of 26 <automated> blocks span two lines after `&&` (valid sh); Phase 4 kept them on one line.
6. 05-10 maps criterion 2 to "DSPA header rows", but no .aexp declares a STRUCT header or count group (the evidence exists as observer_psg_tone's header twin, pcm_chan_append and pcm_cadence). Add a STRUCT group in 05-05 or change the mapping.
7. 05-05 RULES scope (D-182..D-197, D-220) omits the behaviour decisions D-198..D-204 (sinks, restore domain, power-on) that already have case rows; audio_judge_forms has no expect_red misattribution row; D-203 accepts rem[1] up to 6,291,456 when select 1 is uncoupled (unreachable, bounded).
8. If 05-03 splits after Task 2, v4 lands in 05-11 under the same version numbers (the 04-03/04-18 precedent); record it as the D-202 reading.
9. Carried from Phase 4: no {FAILING_DIRECTIONS} or {VERIFY_PATHS} probe was supplied, so check 8f is not scored.

## Evidence discipline (check 4) and threat models (check 8)

- Classes at write time: every literal row in 05-03..05-09 names MAN, GUEST or ASSUMED in the plan text (R1). ER-10 holds except blockers 3 and 6 and warning 7.
- ER-14: 05-08 and 05-10 prohibit and grep against support of LED-AUD-0001/-0006/-0011; promotions are limited to -0002/-0003/-0005/-0008 by files_modified and action text.
- ER-16: MAN live/period/order rows sit at S = 0 or S = 4 only; the S = 15 row is ASSUMED with the DISPUTED marker; atten_order is MAN as ER-16 allows.
- ER-17: RULES with row/limit/later statuses, AUDIO_RULE_NO_ROW and no later: left at 05-10 (gaps: warning 5, info 7).
- ER-11/ER-20: one golden per ROM beside a literal scene, INV_AUDIO_HASH_ONLY plus audio-hash-only, audio_hash_not_verdict, digests printed as E0.
- ER-22: per-clause MAN guest tables in 05-08 and 05-10, sources with no in-page conflict (gaps: warnings 5 and 6).
- Deletion controls (R4): P8 label (two mini-roots), docs words (claim-word-audio), sink refusals and IN-03 (cases plus a scratch deletion), restore rules (hostile rows plus a deletion matrix), CROWS_MAX, one audio-check mini-root and selftest per code, seven toys, AF sweeps, null-synth controls. Missing: the identity rule's negative (blocker 2).
- Gate time: each fixture is held to at most 16 frames per scene and about 40 frames per ROM; fold cost measured in 05-04, budgets in 05-08 and 05-10.
- Strict-C11 Linux part (gate-gcc) at the end of every C plan (02..07, 09); all six parts in 05-08 and 05-10; the REQUIREMENTS tick-revert rule in every plan; no forbidden claim word is introduced.
- Threat models cover untrusted guests: sink floods (T-05-06 pcm cap, T-05-25 psg_block_flood with drop-newest), restore domain (T-05-08, -09, -12; 05-09 boundary rem[1]), fold amplification (T-05-14 sample loop, T-05-25 F = 1 and NFc = 0, T-05-31 coupled loop of about 400 events per 600-clock span).

## Structured issues

```yaml
issues:
  - {plan: "05-01,05-03", dimension: context_compliance, severity: blocker, required_property: "seeded-prng-v4 is policy 5 and the hostile row is policy-6=field (D-228); the handbook covers D-228", description: "policy 6 / policy-7 / D-182..D-227 at 05-01 L58, L170, L193, L211, L224 and 05-03 L75, L246-250, L258", fix_hint: "v4 = 5; policy-5 row -> policy-6; seq 182 228"}
  - {plan: "05-04", dimension: requirement_coverage, severity: blocker, required_property: "aud_poweron_policy_diff exists and is red-capable (D-204, D-212)", description: "named only in the 05-03 fallback; no task, file or REQUIRED line", fix_hint: "synthetic case keyed on an unuploaded channel, v4 seeds 1 and 2 must differ"}
  - {plan: "05-04", task: 1, dimension: verification_derivation, severity: blocker, required_property: "a MAN silent row is silent only for its stated basis", description: "R4 $00 leaves AL = 0 (att 31, P8-10 sum 15) in silent_pre; chON-gate mutant stays green", fix_hint: "second R4 write $1F; ev row post $001F"}
  - {plan: "05-06", task: 1, dimension: verification_derivation, severity: blocker, required_property: "every planned row passes on a correct synthesiser", description: "phase C keys both channels, so neither side is silent", fix_hint: "single-channel pan sub-phases for the MAN silent rows; re-derive the twin set"}
  - {plan: "05-07", task: 1, dimension: context_compliance, severity: blocker, required_property: "every enabled channel gets all 32 words and every register before it sounds (D-212, D-204 identity)", description: "select 4/5 keyed on before R7 with no wave upload and F = 0", fix_hint: "upload, write F, write R7 NE 1 before key-on"}
  - {plan: "05-09", task: 2, dimension: verification_derivation, severity: blocker, required_property: "a MAN group does not depend on an ASSUMED restart edge or switch timing (R2, D-208)", description: "lfo_dynamic exclusion windows come from the assumed TRG-release restart", fix_hint: "stream-derived exclusion or ASSUMED class"}
  - {plan: "05-03", task: 3, dimension: task_completeness, severity: blocker, required_property: "every verify can pass on a correct tree", description: "RESULT mmu_mpr: no such case", fix_hint: "mmu_mpr_256k"}
  - {plan: "all", dimension: context_compliance, severity: blocker, required_property: "stop and split near 60 percent at the named boundary (D-216)", description: "stop rules say about 80 percent; Phase 4 analogs split at 60", fix_hint: "restore 60 percent or record an erratum"}
  - {plan: "phase", dimension: nyquist_compliance, severity: blocker, required_property: "05-VALIDATION.md exists before execution", description: "absent; 05-10 creates it at close", fix_hint: "orchestrator drafts it from RESEARCH; 05-10 signs it"}
  - {plan: "05-03,05-04,05-05,05-06,05-07,05-09", dimension: scope_sanity, severity: warning, required_property: "each plan fits one executor context", description: "1.15x to 1.6x over budget; Phase 4 analogs split", fix_hint: "expect 05-11/05-12; rule for a sixth split; move 05-03 T1 to 05-02"}
  - {plan: "05-02,05-04", dimension: cross_plan_data_contracts, severity: warning, required_property: "a later producer updates the earlier assertion it changes", description: "pcm_chan_append asserts sample_mclk 0; 05-04 writes 192", fix_hint: "05-04 T1 edits obskinds.c"}
  - {plan: "05-06", task: "1,2", dimension: task_completeness, severity: warning, required_property: "each task verify can go green", description: "audio_tone_golden red until T3", fix_hint: "regenerate per ROM change"}
  - {plan: "05-05", task: 1, dimension: verification_derivation, severity: warning, required_property: "KINDCLASS ceilings admit the classes later plans write", description: "same lacks P8-2; live/period lack P8-10", fix_hint: "widen or rank-only"}
  - {plan: "05-06", task: 2, dimension: requirement_coverage, severity: warning, required_property: "every LED-AUD-0008 clause has a row (ER-17)", description: "no R1-step atten_order row", fix_hint: "R1 $FF vs $EF pair"}
  - {plan: "05-06", task: 1, dimension: requirement_coverage, severity: warning, required_property: "LED-AUD-0003 masks have a MAN guest row (ER-22)", description: "no guest write sets a masked bit", fix_hint: "ev rows with don't-care bits"}
  - {plan: "05-07", task: 2, dimension: verification_derivation, severity: warning, required_property: "MAN dda_hold does not judge the tie order (R2)", description: "first sample after the stamp is the tie order when the stamp is a sample instant", fix_hint: "state the phase or accept either sample"}
  - {plan: "phase", dimension: research_resolution, severity: warning, required_property: "RESEARCH has no unresolved open question", description: "three items unmarked, resolved by D-219/D-222/D-223", fix_hint: "mark RESOLVED"}
```

## Dimension summary

| Dimension | Result |
|---|---|
| 1 Requirement coverage | AUD-01..03 each claimed (AUD-01 03/04/06/07/09, AUD-02 02/03/04, AUD-03 04..07; 05-10 completes); blocker 2; warnings 5, 6 |
| 2 Task completeness | 29 of 29 structurally complete; blocker 7; warning 3 |
| 3 / 3b Dependencies, coupling | pass |
| 4 Key links | pass (bus -> write -> DSPE; fold -> const sample -> pcm; judge -> .aexp.tsv) |
| 5 Scope sanity | warning 1; blocker 8 |
| 6 Verification derivation | literals recomputed and correct; blockers 3, 4, 6; warnings 4, 7 |
| 7 / 7b Context compliance, scope reduction | blockers 1, 5, 8; no reduction wording; no deferred idea |
| 7c Tier compliance | pass (core/runner split; runner never includes the core record header) |
| 8 Nyquist | blocker 9; automated verify on every task |
| 9 Data contracts | warning 2 |
| 10 CLAUDE.md / AGENTS.md | pass (no push, no dependency, rights, attribution line, GSD entry points) |
| 11 Research resolution | warning 8 |
| 12 Pattern compliance | pass (F-64 ruled, F-80 anchors kept, analogs cited) |
| Verify format / paths | blocker 7; info 5 |

## Recommendation

REVISE. Blockers 1, 3, 4, 5, 6 and 7 are local plan edits (one line to one paragraph each); blocker 2 adds one case to 05-04; blocker 8 is a wording change in ten stop rules (or an erratum); blocker 9 is an orchestrator artefact. Apply warnings 2, 3, 5 and 6 in the same pass: they fail closed but would force mid-execution deviations.
