# Phase 5: PSG Raw Audio - Discussion Log

> Human reference only (audits, retrospectives). Not consumed by downstream agents; 05-CONTEXT.md is canonical.

**Date:** 2026-11-12
**Mode:** autonomous discuss under the standing fan-out protocol (AGENTS.md); no owner question asked.

## Method

- Five parallel advisors, one per cluster: A register model and channel semantics; B time model and PCM synthesis; C sinks, ABI and snapshot; D oracles, judge and ledger; E scope, plans, process and budgets.
- One adversarial reconciliation pass over the five reports (about 35 cross-cluster conflicts), each resolved by one ruling with a reason, not by averaging.
- The reconciler re-read the load-bearing pages as 150 dpi images: PSG manual P8-2 to P8-10 and P8-12, HuC6280 H8-23 and H8-24. Facts confirmed: fOUT = fmaster / (2 x 32 x F) with fmaster 7.16 MHz supplied by the CPU (P8-9); CLKP = OSC1 / 3 (H8-23);
  the P8-12 table matches 7.16 MHz / (64 F) (A4 F 254 at 440.45, C1 F 3421 at 32.70); R0 values 0..5 select ch1..ch6, 6 and 7 shaded (P8-3); the P8-10 text ("exceeds F") contradicts its worked example (sum 15 silent); LF CTL shifts 0/2/4 (P8-8), w - 16 (P8-7), TRG reset and halt rules (P8-6);
  the PSG decodes A3-A0 with no read function (H8-24).

## Advisor summary

| Cluster | Scope | Key recommendations | Outcome |
|---------|-------|---------------------|---------|
| A register model | decode, select, wave pointer, DDA, LFO and noise registers, power-on, regions | `& $0F` mirror; reads unchanged; select 6/7 drop; level DDA reset; R6 while playing stores, no increment; TRG level hold; v4 wave RAM; PSG0/PWAV/PSGT; numbering page-stated; C1 one formula | Adopted almost whole (D-188..D-193, D-202, D-204) |
| B time and synthesis | divisor, periods, counters, fold, sample grid, mix, noise, LFO | literal divisor, d = 3 negative; 192-clock point samples; absolute-time fold; s32 frame with a tap; Q15 curve; always-clocking 18-bit LFSR; awk recompute stages | Divisor, grid, periods, curve, LFSR, proof obligations adopted; fold form, tap, awk stages rejected |
| C sinks and ABI | event log container, PCM rate and format, sinks, streams, IN-03, snapshot, Phase 6 contract | new `psg` sink DSPE v1; 450-clock rate; u16 frames; step-end fold with nothing derived stored; two sinks appended once; IN-03 closed; v4 | Sink, ABI, streams, IN-03, fold representation, Phase 6 contract adopted; rate and u16 format rejected |
| D oracles and judge | literals, `.aexp`, `audio-check`, isolation, row classes, sweep, toys, twin, ledger | hand literals plus arithmetic lint; solo and pan scenes; order rows; MAN silence at >= 16; AF0..AF9; in-ROM twin; KINDCLASS, DISPUTED, RULES; 2 ROMs; period E1 | Judge, lints, sweep, twin, isolation adopted; ROM count, `audio_agree`, period promotion overruled |
| E scope and process | scope, tracer, plans, cut, handbook, cadence, plan rules, budgets, CI levers, claims | full scope with an LFO cut; 10 plans; dev-sink log; 4 ROMs; v4 deferred; `[audio]` table; audio toy cap 6; CI levers in plan 1 | Plans, cut, handbook, cadence, rules, claims, levers adopted; dev-sink log, ROM count, v4 deferral and the table overruled |

## Conflict rulings

| Conflict | Positions | Ruling | Reason |
|----------|-----------|--------|--------|
| PCM cadence | B 192 master clocks; C 450 | 192 | At d = 6 a cycle is exactly F samples and a noise clock 2 NFc samples for every value; at 450 no noise grid is integral; less aliasing; cost negligible; both are multiples of 3 and 6 |
| PCM frame | B 16 bytes (s32 L/R plus tap); C 4 bytes (u16) | 2 x u32 unsigned, 8 bytes, format 1 | The tap judges a side channel and lets mixing bugs hide; u16 cannot hold the Q15 mix without rounding the quietest step toward 0; scenes isolate channels |
| Event log | E dev-sink extraction; A, C core sink with decoded fields | New `psg` sink, DSPE v1, 24 bytes, decoded chan, sel, effect, post | EV rows must judge the core's decode; a runner replay is same-author E0; dev keeps logging and gives a 1:1 join |
| ABI shape | C two sinks; old note "one aud sink" | `psg` and `pcm` appended last in one change; `ds_obs_begin` changed once; IN-03 refusal in the same plan | One cascade; the uneven overlap contract closes while the request is touched anyway |
| Fold design | B absolute next-event times with window checks and next_sample; C distances, nothing derived | Distances from the fold point (u32, 0 = frozen), step-local fold point inside a step, no next_sample | Range-only restore checks, no u64 window rule per counter, nothing derived in state; the chunk argument is the same for both |
| Snapshot | A PSG0/PWAV/PSGT; C PSG0/PSGC/PSGW | PSG0, PSGW, PSGT; 21 -> 24 regions; serial 9, epoch 6, text 8 once in 05-03; v4 yes | A's split by kind, C's names; one bump with the complete layout |
| Divisor meaning and C1 | A, B literal; D, E documentary equivalence | Literal; d = 3 is an expect_red negative; LED-AUD-0001 stays not supported | An OPEN field must be observable; d = 3 represents the original rival reading |
| What reaches supported | D period entry as E1; B several; criterion 4 | At most 4 E1 candidates: numbering, register map, wave and DDA modes, undisputed silence; period, LFO, noise clock held (ER-14) | Criterion 4 forbids pitch or timing correctness claims until C1 is settled; rows may still be MAN |
| Silence boundary | text "exceeds F"; example sum 15 silent | Model silent at >= 15; MAN rows at >= 16, chON 0, R1/R5 byte zero; 15 ASSUMED | Follow the example and the 45 dB sentence; keep the disputed point out of MAN rows |
| Audible rows | D MAN at <= 14; B ASSUMED near the threshold | MAN only at maximum and the sum-4 example; 5..14 ASSUMED | Converse rule (Phase 4 and 4.1 review recurrences) |
| Volume curve | B Q15 10^(-3a/40) plus an awk recompute stage | Q15 table committed, ASSUMED; no recompute stage; C case for monotonicity and endpoints | The page says "about"; a gate-time recompute is a same-author model |
| Noise LFSR | B 18-bit, taps 0/1/11/12/17, always clocking; MAME only with NE | B's model, ASSUMED, MAME disagreement recorded; noise counters land in 05-03 | Three emulators and a community quote; landing it with the bump keeps every existing pin final |
| Zero values | all clusters: 4096, 256, 32 ticks as ASSUMED | Modelled now as constants; never in MAN rows | Defined behaviour for hostile input; the pages say forbidden or undefined |
| R6 while playing | A store without increment; C WAVE_DROP; pcetech ignore | Store without increment; effect bit WAVE_HELD | Four emulators and a game observation against one 2001 note; ASSUMED either way |
| Oracles | D hand literals plus arithmetic lint; B gate-time LFSR window and volume recompute | Literals plus the lint only | ER-7 and D-171; the lint can only fail and never supplies a value (ER-18) |
| ROM count | A 0; B 3; D 2; E 4 | 3: psg_tone, psg_mod, psg_lfo; OX_ROMS_MAX 48 as capacity | Promotion needs guest rows; the twin replaces a bad-ROM variant; a separate LFO ROM keeps the cut clean |
| Toys | D 6; E 6; C 2 | Cap 7, one merged list | The non-interference and fold toys are proven patterns; factor-2 and off-by-one are covered by the d = 3 case and the twin |
| Twin and negatives | D in-ROM twin; E bad ROM; B pad-selected path | In-ROM twin T' (F + 1, one channel never keyed on) plus the d = 3 expect_red case | No ROM slot; localised, attributable red |
| Scope and LFO | E full scope with a PCM UNMODELLED flag and an LFO cut | Full scope, LFO last; UNMODELLED moves to the DSPE effect bits; cut at 05-08 with six triggers | The PCM frame has no flags; the log can mark unmodelled writes |
| Plan shape | E 10 plans, 9 waves, splits 05-15..19, revisions 05-20+ | 10 plans in 9 waves; splits 05-11..05-15, revisions 05-16+; noise counters and register storage move into 05-03 | Remove the numbering gap; existing fixtures write R5, so pins must be final at the bump |
| Tracer | E square wave on channel 0 | Asymmetric table on select 0, period, order, silence and EV rows, d = 3 negative, full proof set | A square cannot show order errors; mvp needs one red-capable end-to-end slice |
| Run record | E `[audio]` table; C none | None in Phase 5; per-frame counts are runner evidence | D-163 precedent; the table belongs to the Phase 6 artefact |
| Policies | E zero plus one v3 seed; others v4 | Default zero, v4 s1, v4 s2; audio literal cases zero and v4 s1; noise repeat under all three | v4 needs a v4 seed to guard wave RAM; lever (2) cost discipline |
| audio_agree | D runner kind | Dropped | ER-3 circular; dda_hold and GUEST schedule rows cover it |
| Chunk schedules | B 7/191/193/3072/frame; C 1/7/449/450/451/1366/frame | 7, 191, 193, 1366, one frame | Sample period +-1, a line, a frame; C's set followed the rejected rate |
| CI levers | E in plan 1; 4.1 record says workflow edits are the owner's | In 05-01 as two revertible commits; yes/no to the owner at publish time; lever (c) rejected | Reversible efficiency work proceeds; the recorded owner preference is honoured before anything is pushed |
| Handbook | E 05-HANDBOOK.md | Adopted, cap 25 KB, contradictions are defects | Phase 4 tracers split on reading cost, not diff size |
| Naming | mixed 1-based and "ch2" wording | 0-based select values everywhere | The R0 value is what the guest writes and the log records |

## Owner gates

None crossed and none asked. No hardware capture is needed. Hand-offs: push and PR authorisation (D-64 pattern); keep or revert the two CI workflow commits, asked once at publish time (D-215); settling C1 or a profile r2 stays an owner option at a milestone review.

## Deferred ideas

See the deferred section of 05-CONTEXT.md.
