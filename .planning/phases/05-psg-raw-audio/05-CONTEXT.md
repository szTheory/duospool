# Phase 5: PSG Raw Audio - Context

**Gathered:** 2026-11-12
**Status:** Ready for planning
**Method:** Autonomous discuss under the standing fan-out protocol (AGENTS.md): five parallel research advisors (A register model and channel semantics; B time model and PCM synthesis; C sinks, ABI and snapshot; D oracles, judge and ledger;
E scope, plans, process and budgets), then one adversarial reconciliation pass that resolved about 35 cross-cluster conflicts and re-read the load-bearing manual pages as images.
No owner question was needed: no authority gate (licence, visibility, rights, hardware capture, publication, one-way action) is crossed by the decisions below. No hardware capture is needed or requested.
**Evidence caveat (read first):** tags in this file: [MAN img] = the page was read as an IMAGE during this discuss; the recorded ER-1 read (verifier, date) is still plan 05-01's job. [MAN txt] = OCR text only. [COMM] = pcetech.txt, the PCEdev wiki or emulator source read as reading material
(evidence class E4 at best; agreement between emulators is not correctness; nothing copied). [CODE] = this repository. [ASSUMED] = a project modelling choice. [DERIVED] = arithmetic done outside the repository.
Re-read as images by the reconciler at 150 dpi: PSG manual P8-2, P8-3, P8-4, P8-5, P8-6, P8-7, P8-8, P8-9, P8-10, P8-12; HuC6280 hardware manual H8-23 and H8-24. Read as images by advisors only: P8-i, P8-1, P8-11 (advisor A read all 14 PDF pages), H8-18.
The PSG manual labels its pages "P8-n" (the OCR shows "PB" or "PS"); label P8-n is PDF page n+2 of the 14-page copy. Manual text is never copied; facts are stated as short numbers and formulas with page labels (ER-9).

<domain>
## Phase Boundary

The HuC6280 PSG becomes a real device on the existing master-clock bus. Authored audio fixtures write its registers through the Phase 3 bus; the core keeps the register file, wave RAM and counters, emits one decoded record per CPU write to the PSG window (DSPE v1) and point-sampled stereo PCM
at one sample per 192 master clocks (DSPA v1), both through caller-owned sinks. Every audio claim is checked against a literal that DuoSpool did not produce (a manual page, an in-guest computation or an assumed ledger id); an audio hash counts only beside such a literal shown to fail when the
synthesiser is deliberately broken. Nothing here is a hardware claim, and no pitch or timing correctness claim is made while the PSG clock field stays open (criterion 4).

Delivers (requirements AUD-01..AUD-03):

1. Register model: window decode with the 16-byte mirror, R0 channel select, R1..R9 fields and masks, wave RAM and its address counter, DDA latch, noise registers, LFO registers (AUD-01).
2. Time model: a literal PSG tick of master/d (d = the bound psg_tick_divisor), the P8-9 period formulas, one fold `ds_psg_advance` at write stamps and step ends (AUD-01, AUD-02).
3. Synthesis: unsigned 5-bit channel levels, the P8-10 attenuation sum, an assumed Q15 volume curve, a linear stereo mix, an 18-bit noise LFSR, LFO modulation (AUD-01).
4. Raw audio: two new caller-owned sinks, `psg` (DSPE v1, 24-byte records) and `pcm` (DSPA v1, 8-byte frames), canonical streams, a runner `--audio-out`, per-frame sample counts (AUD-02).
5. Evidence: three authored fixtures built in the gate, an audio judge (ROWS, REL, STRUCT, EV), a fault sweep, seven toy mutants, an in-ROM mis-programmed twin, a wrong-divisor negative case and one new gate stage (`audio-check`) (AUD-03).

Not in this phase: host resampling or device playback, WAV output, band-limited synthesis, CD-ROM ADPCM or CD audio, SuperGrafx, the HuC6280A output variant, a public API or installed layout and the `[audio]` run-record table (Phase 6), fuzzing, hardware captures, a profile r2.

Nine waves (every plan ends with `sh scripts/check` green, its goldens regenerated, and a strict-C11 Linux container compile when it touches C):

- **W1 Evidence and plumbing (no behaviour change).** 05-01 evidence data and the CI levers (no C). 05-02 sink plumbing with no producer and the IN-03 refusal. Parallel-safe (disjoint files).
- **W2 Layout.** 05-03: the complete PSG state layout, the one serial/epoch/text bump, seeded-prng-v4, register decode and storage, the always-running noise counters; no synthesis.
- **W3 Tracer.** 05-04: channel 0 write to DSPE record and PCM samples to a hand literal that can go red, through every layer.
- **W4 Judge.** 05-05: audio judge hardening, fault sweep, `audio-check`.
- **W5 Six channels.** 05-06. **W6 Noise and DDA.** 05-07.
- **W7 Checkpoint A.** 05-08. CUT LINE (D-217): if a trigger fires, the LFO plan becomes plan 5.1-01 of an inserted Phase 5.1 and 05-10 closes Phase 5.
- **W8 LFO.** 05-09. **W9 Close-out.** 05-10.

Requirement coverage: AUD-01 05-03 (register side), 05-04, 05-06, 05-07, 05-09 (closes 05-09; at a cut its LFO part closes in 5.1); AUD-02 05-02, 05-03, 05-04 (closes 05-10 with the definition entries); AUD-03 05-04, 05-05, 05-06, 05-07 (closes 05-07).
All three are covered. Sizing: 10 plans in 9 waves; split numbers 05-11..05-15 are reserved for stop-rule splits and 05-16 onward for revision plans; numbers are never reassigned.

</domain>

<decisions>
## Implementation Decisions

Decision ids D-182..D-218 continue Phase 4 (D-135..D-181). Ledger status is `assumed` unless a decision says "E1 candidate", which is allowed only under the evidence rules below. Reqs lists the requirement ids a decision serves. A number or rule with no tag is a [CODE] fact of this repository or an
[ASSUMED] project choice recorded in the ledger entry the decision names. Channels are numbered by the R0 select value 0..5 everywhere in this file (the manual's ch1..ch6 are select 0..5).

### Evidence rules (ER-1..ER-13 stand unchanged; nine are added)

- **ER-14 Criterion-4 hold.** Until the owner settles C1 (decision 10: a primary source the owner accepts, or an owner-approved capture), no ledger entry whose statement states a PSG period, frequency, frequency ratio, noise clock, LFO rate, sample timing or any time correspondence is promoted past
  `assumed`. LED-AUD-0001 keeps its status and the profile field psg_tick_divisor stays OPEN. Rows may still carry MAN tags when their literal comes from the pages (P8-9 with H8-23); the hold is on ledger status and claim words, not on row classes.
- **ER-15 Disputed points are ASSUMED rows only.** `tests/audio/DISPUTED` lists each with its ledger id: attenuation sum exactly 15 (except the R1/R5 byte-zero clause), F = 0, FLF = 0, NFc = 0, R0 select 6 or 7, A3-A0 = $A..$F, an R6 write while chON = 1 and DDA = 0, the key-on counter,
  the D/A latch loaded with chON = 0, an LFO sum that leaves 12 bits. `audio-check` fails a MAN group that touches one (`AUDIO_DISPUTED_POINT`). This mechanises the 04.1 review H-01 suggestion.
- **ER-16 Converses are ASSUMED.** A negated or converse rule (audible below a threshold, no effect without a condition) is MAN only where a page states it. Audible rows are MAN at maximum level and at the P8-10 worked example (sum 4); audible rows at sums 5 to 14 are ASSUMED.
  A strict decrease per register step is MAN (P8-3, P8-4 state it). (Phase 4 gate M-01, H-02; 4.1 H-01, M-01.)
- **ER-17 Class ceilings and rule coverage are data.** `tests/audio/KINDCLASS` maps every row kind to its maximum class (`AEXP_CLASS_INFLATED`); `tests/audio/RULES` lists every clause of every Phase 5 AUD ledger statement and decision, and `audio-check` fails `AUDIO_RULE_NO_ROW` when a clause
  has no group with a kill other than AF0. A new clause cannot ship without a row. (4.1 M-02, the D-160 SB gap.)
- **ER-18 An arithmetic lint is not an oracle.** A static check may recompute a hand literal from formula inputs written in the same row and fail on mismatch; it never writes, supplies or adjusts an expected value, and it is labelled E0 consistency of the expectation file.
  A gate-time generator of expected values (a reference synthesiser, an LFSR sequence generator, a volume-table recompute stage) is a reference model and is NOT committed (ER-7, D-171).
- **ER-19 Audio records are definitions.** DSPE and DSPA fields are project definitions, not hardware claims (ER-13 extended); the runner parses them with hand-written offsets and never includes the core's record header; the PCM value 0 for silence is a format definition.
- **ER-20 An audio hash is a change detector, never a verdict.** ER-11 applies verbatim: a `golden` audio case is valid only beside a `literal` case of the same ROM whose groups the sweep and toys show live, and the evidence prints `pcm_sha256=... psg_sha256=... E0 repeatability`.
- **ER-21 No E2 statement about PSG output exists.** The guest cannot read the PSG (H8-24 lists no read function; reads return the bus latch). GUEST rows cover only the guest's own data and write schedule; every PSG output verdict is a runner-side judge over the DSPE and DSPA streams.
- **ER-22 Promotion needs MAN-tagged guest rows.** ER-2 applies in full, and a `supported` entry also needs, for every clause of its statement, a MAN group in a guest-fixture case (C-level cases alone do not count) and sources with no recorded in-page conflict on that clause (the Phase 4 demotion lesson).
  Same-author decoders and scripts (E0/E2) never back a supported entry alone.

### A. Clock, time base, fold and sample grid (AUD-01, AUD-02)

- **D-182 C1 at the documentary level.** [MAN img P8-9]: fOUT = fmaster / (2 x 32 x F), fmaster "the PSG driving clock frequency (7.16 MHz supplied by the CPU)", F the 12-bit R3:R2 value, F = 0 forbidden. [MAN img H8-23]: the CPU supplies the PSG clock CLKP with f = OSC1 / 3.
  [MAN img P8-12]: the scale table is computed at fmaster = 7.16 MHz; A4 is F = 254 at 440.45 Hz and C1 is F = 3421 at 32.70 Hz, both equal to 7.16 MHz / (64 F) [DERIVED]. So one wave-address step is 2F CLKP periods = 6F master clocks, and a 32-word cycle is 192F master clocks.
  "master/6 with 32F ticks per cycle" (pcetech, the emulators) and "OSC1/3 with 2 x 32F" (the manual) are one formula; the only reading the pages contradict is OSC1/3 with 32F (one octave higher). This is E1 documentary (the model equals the pages), not a measurement. Reqs: AUD-01.
- **D-183 psg_tick_divisor is a literal divisor.** The bound OPEN field d (3|6; r1 frozen; LED-AUD-0001) is the master-clock divisor of the PSG tick: every PSG counter counts ticks of d master clocks, so a channel step is d*Fe, a cycle 32*d*Fe, a noise clock d*64*NFc and an LFO-coupled step d*F1e*FLFe (D-184).
  d = 6 equals P8-9 with fmaster = OSC1/3; d = 3 is the contradicted no-factor-2 reading, implemented because r1 is frozen and criterion 4 keeps the field open. Every audio case binds 6 except the expect_red case of D-211, which binds 3 and must fail the MAN period rows while the order and silence rows stay green.
  No literal anywhere is computed from the bound value. The profile is not edited (D-134); narrowing the field to {6} in an r2 is an owner choice at a milestone review, not a Phase 5 action. Rejected: the documentary-equivalence meaning (3 with a 2F reload), because no fixture could tell the two values apart,
  which makes an OPEN field that every run binds and nothing observes (the D-113 rule that a parameter needs two values a fixture distinguishes). Reqs: AUD-01.
- **D-184 Period table** (master clocks; NFc = 31 - (R7 & $1F), the complement of the 5 bits [MAN img P8-9]; d = 6 values in brackets):
  channel step d*Fe [6Fe], Fe = F, or `psg_f0_as` 4096 when F = 0 (forbidden on P8-9, ASSUMED); cycle 32*d*Fe [192Fe] [MAN img P8-9, P8-12]; noise clock d*64*NFc [384NFc] for NFc >= 1, and d*`psg_noise_nf0_ticks` (32) [192] when NFc = 0 (undefined on P8-9, ASSUMED);
  LFO-coupled select-1 step d*F1e*FLFe [6*F1e*FLFe], FLFe = FLF, or `psg_lfo_f0_as` 256 when FLF = 0 (ASSUMED) [MAN img P8-9: fLFO = fmaster/(2 x 32 x F2 x FLF) is the time to address the 32 words of the modulating channel];
  modulated select-0 value F0' = (F0 + $1000 + (w << s) - (16 << s)) & $FFF, 0 read as 4096, where w is the select-1 wave word at its counter and s = 2*((R9 & 3) - 1) [MAN img P8-6, P8-7, P8-8: the addition, the value w - 16, shifts 0/2/4; the 12-bit mask, the zero rule and the read instant ASSUMED].
  The core computes every PSG period from this table and nothing else. Reqs: AUD-01.
- **D-185 Counter model** (ASSUMED, LED-AUD-0012 unless tagged; the manual is silent on reload timing). Tick instants are k*d (`psg_tick_phase_mclk` 0, the D-136 precedent). At a step event at T the index advances by 1 mod 32 and the next event is T + d*Fe with Fe read at T (F0' for select 0 under an active LFO).
  Writes to R2, R3 or R7 never touch a running count: the new value applies at the next reload. Key-on (chON 0 -> 1 with DDA 0) at stamp w: next = (w - w mod d) + d*Fe, index unchanged (`psg_keyon_widx_step` 0; Mednafen adds 1, a named disagreement). The channel counter is frozen while chON = 0 or DDA = 1,
  and the select-1 counter while LF TRG = 1. The wave counter of select 4 or 5 keeps running under NE = 1 (Mednafen; Mesen2 stops it). Volume and balance writes act at their stamp. Under an active LFO select 1 steps every d*F1e*FLFe instead of d*F1e: one combined counter, as P8-6 describes R8 and the
  ch.2 frequency registers extended into a single divide-by-N counter [MAN img P8-6], not a separate LFO divider. Tie order at one instant: the select-1 step, the channel steps of select 0..5, the noise clocks, then the sample instant; a CPU write stamped w is applied after every event with time <= w
  (D-137, D-111), so a sample at exactly w sees the old value. Fixture rows stay insensitive to these rows: shape rows are rotation-invariant, the first and last plateau of a segment are excluded, and periods are measured between recurrences, never from a write. Reqs: AUD-01.
- **D-186 Fold `ds_psg_advance`** (the D-137 shape). Called (1) at the head of every write to window 2 at its access stamp (no penalty: the offset is >= $800); (2) at the end of every step to machine_time_after - 1, right after `ds_vdc_advance` inside `ds_cpu_step_obs` (VDC then PSG; they share no state).
  Never at reads and never in the interrupt sample (the PSG raises no interrupt). Monotone, composable, inclusive. Representation: PSGT holds for each counter r = (next event time) - (machine_time - 1), a u32 with 1 <= r <= its largest period, and r = 0 for a frozen counter (derivable from the registers);
  no absolute deadline, no next-sample index and no folded-to stamp is stored. Inside a step the point already folded to is a step-local value (the step start, then each window-2 write stamp); between steps it is machine_time - 1 by invariant.
  Catch-up: a channel without the LFO advances in closed form (k steps due, index += k mod 32); the LFO-coupled pair and the LFSRs iterate event by event; every loop is bounded by the folded span over d (the span is at most one instruction, the longest a block transfer of about 4.7 M master clocks).
  Every fold target is a function of the instruction stream alone, so chunking, restore at a stop and an attached sink change nothing (the D-137 argument). Rejected: stored absolute next-event times with per-counter restore windows (cluster B: a larger restore surface and a derived next_sample),
  and a lazy fold at writes only (late tail samples or a stored cursor). Reqs: AUD-01, AUD-02.
- **D-187 Sample grid.** PCM sample s is the point sample of the modelled stereo output at instant s*192 master clocks, s = 0, 1, ... (`psg_mclk_per_sample` 192, `psg_sample_phase_mclk` 0; LED-AUD-0014). The rate is recorded as "1 sample per 192 master clocks"; OSC1/192, about 111,860.795 Hz,
  is a printed label, never core arithmetic. 192 = 32 x 6 is a multiple of both allowed d, so instants lie on the tick grid of either binding. At d = 6 a waveform cycle is exactly F samples and a noise clock exactly 2*NFc samples for every F and NFc, so the MAN period and noise-grid rows are exact.
  Per frame: frame k holds the instants in [k*L, (k+1)*L), L = lines x 1365, count = ceil((k+1)L/192) - ceil(kL/192); 262 lines: 21 frames of 1863 and 11 of 1862 in every 32 (59,605 samples; frame 0 has 1863); 263 lines: 49 of 1870 and 15 of 1869 in every 64 [DERIVED].
  After a run to machine time M > 0 exactly ceil(M/192) samples exist (an independent closed-form literal). Class: ASSUMED:LED-VID-0001 (1365, line count) plus the definition LED-AUD-0014. The grid is a project definition, not the hardware's; point samples alias content above 55.9 kHz (F < 32);
  a better decimation is a new record version or a host concern, never a silent change. Rejected: 450 master clocks (cluster C; no integer noise grid for any NFc <= 31, inexact periods without a K rule, aliasing from 23.9 kHz), 6 (59,605 samples per frame), 384 and 448 (non-integer periods). Reqs: AUD-02.

### B. Register model (AUD-01)

- **D-188 Window, reads and write path.** Physical $1FE800-$1FEBFF; reg = offset & $0F for every access [MAN img H8-24: the PSG decodes A3-A0; the 16-byte mirror is the D-146 inference]; reg 0..9 are R0..R9 and reg 10..15 name no register [MAN img P8-2: A..F shaded] and are logged with NO_TARGET.
  Reads stay exactly as `bus.c` has them: the value is io_buffer, the byte read becomes io_buffer, no fold, not logged (D-104, D-149), no PSG read function; LED-CPU-0013 governs (no page gives a read value). A write: charge_penalty (a no-op, LED-CPU-0017), stamp (D-92), io_buffer = value,
  `ds_psg_advance(stamp)`, `ds_psg_write(reg, value, stamp)` returning sel_before and effect, `ds_obs_psg`, then the existing log_dev (D-119). ST0-ST2 cannot reach the PSG. Block transfers use the per-byte path; fixtures upload wave RAM with TIN to $0806 or an STA loop (TIA alternates R6 and R7).
  Guards that must stay green unchanged: mmu_mpr checks 22 and 29-31 (check 31 writes $6805, which now stores R5 of select 0), spd_speed check 12, cyc_probe m_psg_rd. Reqs: AUD-01.
- **D-189 Select and per-channel writes.** psg_select = R0 & 7 [MAN img P8-3: R0 values 0..5 select ch1..ch6, 6 and 7 shaded]. R2..R6 apply to channel psg_select when it is 0..5 and are dropped at 6 or 7 (`psg_select_67_drop` 1, ASSUMED LED-AUD-0004; Mednafen, Geargrafx and Mesen2 drop, MAME keeps 8 slots).
  R7 applies only at select 4 or 5 [MAN img P8-2, P8-9: R7 exists for ch5 and ch6 only] and is dropped otherwise. R1, R8 and R9 ignore R0 [MAN img P8-2, P8-3]. Stored masks: R0 $07, R3 $0F, R4 $DF, R6 $1F, R7 $9F, R9 $83, R1/R2/R5/R8 $FF [MAN img P8-2..P8-6, P8-9], in one table
  `ds_psg_reg_mask` shared by the write path and the restore decoder (the 04.1 L-02 fix precedent). Reqs: AUD-01.
- **D-190 Wave counter, wave RAM, D/A latch** [MAN img P8-4 mode table, P8-5 procedures (1)-(3)]. Per channel widx 0..31 and dda 0..31. An R4 write stores value & $DF; if the new DDA bit is 1, widx = 0 (the level reading of "when this bit is 1 ... reset"; it is observably equal to the DDA 1 -> 0 edge reading
  of pcetech, MAME and Mednafen, and gives the invariant DDA = 1 implies widx = 0). An R6 write with v = value & $1F: DDA = 1 (either chON): dda = v, wave RAM and widx unchanged ["data in R6 are not changed"; the chON = 0 case is an inference from procedure (3), ASSUMED];
  chON 0 and DDA 0: wave[widx] = v, then widx = (widx + 1) & 31 [MAN]; chON 1 and DDA 0: wave[widx] = v with no increment (`psg_wave_write_when_on` 1, ASSUMED LED-AUD-0004; MAME, Mednafen, Geargrafx and Mesen2 store, pcetech says the write is ignored).
  The counter otherwise advances only in mixing mode at its step events. Reqs: AUD-01.
- **D-191 Output gating.** chON = 0: the channel contributes 0 whatever DDA, NE or the latch hold [MAN img P8-4: the sound output goes off]. chON 1 and DDA 1: the level is dda. chON 1, DDA 0, select 4 or 5 with NE = 1: the noise level (D-193) replaces the wave [MAN img P8-6: noise enabled, sound disabled].
  Otherwise wave[widx]. Only the HuC6280 is modelled (chip_revision huc6280, LED-PROF-0001); HuC6280A gating and output centring are not modelled and are named in LED-AUD-0009. Reqs: AUD-01.
- **D-192 LFO.** psg_lfo_freq = R8 and psg_lfo_ctl = R9 & $83. An R9 write with bit 7 = 1 sets the select-1 widx to 0 and freezes the select-1 counter; while bit 7 = 1 the modulation value is wave[1][0] - 16, held [MAN img P8-6: writing 1 resets the LFO, halts modulation with the data of address 0 and
  resets the wave counter; writing 0 starts it; the hold-while-1 is the level reading, ASSUMED part of LED-AUD-0011]. On TRG 1 -> 0 at stamp w the select-1 counter restarts with next = (w - w mod d) + d*F1e*FLFe (ASSUMED).
  The LFO is active when (lfo_ctl & 3) != 0, bit 7 = 0, and select 0 and select 1 both have chON = 1 [MAN img P8-9 2.2.2 and P8-6 NOTE; "enabled" read as chON, ASSUMED]. While active, select 1 steps every d*F1e*FLFe and select 0's effective value is F0' (D-184), read at each select-0 reload after a
  select-1 step at the same instant; stored registers are never rewritten (unobservable: write-only). Select 1 still sounds at its own level [MAN img P8-6 NOTE]. Until plan 05-09 lands (or after a cut), an R9 write with ctl != 0 sets the DSPE effect bit UNMODELLED and select 0 is rendered unmodulated.
  Reqs: AUD-01.
- **D-193 Noise.** psg_noise[2] = R7 & $9F of select 4 and 5. Each owns an 18-bit LFSR: shift right, the new bit 17 is the XOR of bits 0, 1, 11, 12 and 17 (`psg_lfsr_taps` 137219), the output is bit 0 shifted out, level `psg_noise_high_level` 31 for 1 and 0 for 0; reset value `psg_lfsr_reset` 1 under
  EVERY power-on policy (a register, never drawn: a drawn zero would lock); the period from seed 1 is 131,071 = 2^17 - 1 [DERIVED]. The LFSR clocks at its noise clock whatever NE and chON are, with a noise event at time 0 and every noise period after it (Mednafen, Mesen2, Geargrafx and a community quote;
  MAME clocks it only with NE set: a named disagreement). The page says only "pseudo random noise (square wave)" [MAN img P8-2], so every LFSR detail is ASSUMED LED-AUD-0010. Consequence: the noise counters run from boot, so PSGT is not all zero at boot (the F-11 precedent) and the digest regeneration of
  D-202 needs an independent LFSR computation. Reqs: AUD-01, AUD-03.

### C. Synthesis (AUD-01, AUD-02)

- **D-194 Level, attenuation, silence and mix.** Each channel presents an unsigned 5-bit level (D-191); the D/A rises monotonically with the value 0..31 [MAN img P8-5 figure]. Per side X: att = 2*(15 - MAL_X) + (31 - AL) + 2*(15 - LAL_X), 0..91, in 1.5 dB units [MAN img P8-3, P8-4, P8-10: R1 and R5 3 dB per step, AL 1.5 dB].
  Silence: att >= `psg_silence_att` (30), which equals the P8-10 sum (F-MAL) + (F-AL') + (F-LAL) >= 15 (AL' = AL high 4 bits) for both parities of AL bit 0, and covers the "R1, R4 or R5 is 0" clause under any reading.
  Ruling on the P8-10 conflict [MAN img]: the text says no sound when the sum "exceeds F"; the worked example gives a sum of exactly 15 and says no sound is generated; the paragraph says none is output 45 dB below maximum (15 steps of 3 dB). The model follows the example and the 45 dB sentence.
  The boundary stays disputed (ER-15): MAN silent rows sit at chON = 0, at sums >= 16, and at R1 = $00 or R5 = $00 (the byte-zero clause); a field-zero row is ASSUMED unless its sum is >= 16; a sum-15 row is ASSUMED.
  Curve: contribution = level * VOL[att] for att < 30, else 0; VOL[a] = round(32768 * 10^(-3a/40)), a = 0..29: 32768 27571 23198 19519 16423 13818 11627 9783 8231 6925 5827 4903 4125 3471 2920 2457 2068 1740 1464 1232 1036 872 734 617 519 437 368 309 260 219 [DERIVED, re-checked];
  the page says "about", so the curve is ASSUMED LED-AUD-0009 (Mednafen's 2^(-a/4) and MAME's 48 dB spread are named disagreements). The table is a committed 30-row `psg_vol.def` with the formula in its header. Mix: L and R are the plain sums over the six channels, unsigned, 0..6,094,848 (6 x 31 x 32768).
  No DC removal, filter, resampling or clipping in the core. Reqs: AUD-01, AUD-02.
- **D-195 Integer proof obligations** (each a unit case or `_Static_assert`): u64 times with checked adds; periods fit u32 at d = 6 (channel step <= 24,576, LFO-coupled step <= 6,291,456, noise clock <= 11,904); the closed-form catch-up guards stamp <= UINT64_MAX - period; LFO arithmetic unsigned with s <= 4;
  mix <= 6 x 31 x VOL[0] < 2^31; att <= 91 and VOL indexed only below 30; the LFSR a uint32 masked to $3FFFF; no float, no signed overflow, no right shift of a negative value; the sample loop bounded by span/192 + 1; explicit masks on 5-bit and 12-bit fields. Reqs: AUD-01.

### D. Raw outputs and sinks (AUD-02)

- **D-196 DSPE v1 record** (24 bytes, little-endian, written byte by byte; LED-AUD-0013). One record per CPU write to window 2, block-transfer bytes and undecoded writes included, so DSPE joins `dev` 1:1 on (stamp, pc, value, phys & $F); reads are not events.
  `+00 u64 stamp` (the access stamp, as in dev); `+08 u16 pc`; `+10 u8 reg` (offset & $0F); `+11 u8 value` (unmasked); `+12 u8 sel` (R0 before the write); `+13 u8 chan` (0..5 for R2..R7 targets, $FE for the global R0, R1, R8, R9, $FF for no target);
  `+14 u8 effect`: b0 WAVE_STORE (write mode), b1 DDA_LOAD, b2 WAVE_HELD (stored with no increment while playing), b3 IDX_RESET (R4 with DDA = 1), b4 LFO_RESET (R9 with TRG = 1), b5 NO_TARGET (reg $A..$F, select 6/7 for R2..R7, R7 at select 0..3), b6 UNMODELLED (D-192), b7 zero;
  `+15 u8 wave_idx` (the target's counter after the write, $FF when chan > 5); `+16 u16 post` (the stored value after the write: R0 3 bits, R2/R3 the 12-bit F, R4 & $DF, R6 the stored word or the latch, R7 & $9F, R9 & $83, R1/R5/R8 the byte; $FFFF for NO_TARGET); `+18 u8 version` (1); `+19..+23` zero.
  The flag rules live in one X-table `psg_effects.def` whose rows carry their ledger ids. The EV rows judge the core's own decode (D-208), so the log is a raw output in its own right, not a runner replay. Reqs: AUD-02.
- **D-197 DSPA v1 frame and the `pcm` sink** (LED-AUD-0014). A frame is `u32 L, u32 R`, little-endian, unsigned (8 bytes); 0 means every channel contributes nothing; no sign, no DC offset, no per-channel tap.
  `struct ds_pcm_sink { uint8_t header[32]; uint8_t *frames; uint32_t capacity; /* frames */ }`; the header is written at the start and end of every accepted call and never read: `+00 u32 count`, `+04 u32 lost` (saturating), `+08 u64 seq` (frames offered), `+16 u64 first` (= ceil(T0/192), defined when seq = 0),
  `+24 u32 sample_mclk` (192), `+28 u16 frame_bytes` (8), `+30 u8 version` (1), `+31 u8 format` (1 = 2 x u32le unsigned, L then R). `DS_PCM_ALIGN` 8. A capacity above `DS_PCM_SINK_MAX_FRAMES` = 8,388,608 (64 MiB, about 75 s) is `run.sink_unsupported`.
  Drop-newest in whole frames with the sticky full flag (F-2): lost = seq - count, never a gap inside a call; lost != 0 is `harness_failure` for any case that uses the stream as evidence. Rejected: a 16-byte frame with s32 and a per-channel tap (cluster B: it judges a side channel instead of the claimed artefact,
  lets a mixing bug hide and doubles the size; isolation is done by scene design, D-208) and a 4-byte u16 frame (cluster C: the Q15 curve for six channels does not fit, and scaling it down would round the quietest audible step toward 0 beside the MAN silence rows). Reqs: AUD-02.
- **D-198 Sink ABI and IN-03.** `struct ds_psg_sink { uint8_t header[16]; uint8_t *records; uint32_t capacity; }` has the `dev` shape (count, lost, seq; 24-byte records, align 8, capacity <= DS_SINK_MAX_RECORDS, drop-newest; `ds_obs_chan` reused).
  `struct ds_run_request` gains `struct ds_psg_sink *psg; struct ds_pcm_sink *pcm;` as its LAST members, in one change (plan 05-02); every request local is already zeroed; `run_refuse_struct_size` adapts. The sink table grows from 3 to 5 channels with the unchanged check order (structs memory_invalid,
  capacities sink_unsupported, ranges memory_invalid) and per-channel alignment, wrap and overlap rows (instance, request, result, every other sink). `ds_obs_begin` changes ONCE to `(log, const struct ds_run_request *, uint64_t machine_time)` so later sinks never change it again; the F-3 caller list cascades.
  New hooks `ds_obs_psg` (const instance) and `ds_obs_pcm`; the sample gate is `obs != NULL` and pcm on. Phase 3 core IN-03 is closed here: after the struct_size and reserved checks `ds_run` refuses a request or result that overlaps the instance as memory_invalid, before the sink checks;
  case `run_refuse_call_overlap` (two rows; the test copies a valid request into a malloc'd instance block so effective types stay defined); the instance stays byte-identical. The D-162 note "Phase 5 adds aud" is superseded by the two sinks. Reqs: AUD-02.
- **D-199 Streams, digests and runner output.** Two canonical streams, built by the runner (test code) from the per-call sink contents of one run: DSPE = `"duospool-psg"` NUL, u32 1, the records; DSPA = `"duospool-pcm"` NUL, u32 1, u32 sample_mclk, u16 channels (2), u8 format (1), u8 0, u64 first_frame, the frames.
  No count or length field, so a streaming writer never seeks back; a run started from a restore carries its true first frame. The digest of a stream is SHA-256 of its bytes; in-process equivalence uses memcmp; streams never enter the state digest.
  `ds_runner --cases FILE --audio-out CASE_ID` writes, only for an `audio` case whose fixture is authored or synthetic, `build/gate/audio/CASE_ID.dspe`, `.dspa` and `.tsv` (rows `E` per event, `F` per video frame with first frame and count, `P` run-length groups of identical frames); ids match `[a-z0-9_]+`;
  stdout is three lines (ids, counts, sizes, SHA-256), never sample data. Case `audio_out_rights` mirrors `video_out_rights`; `audio_out_check` in `scripts/conformance-run` recomputes both digests with the host tool. No WAV in Phase 5. Reqs: AUD-02.
- **D-200 Non-interference and the proof set** (closes the 04.1 M-04 class up front). Machine state never depends on a sink: the psg and pcm pointers are read only at the gate and the emit calls; the sample function is const (`ds_psg_sample(const struct ds_instance *, ...)`, a core-boundary anchor);
  without a sink the fold processes every event and skips only sample synthesis. In the plan that lands each audio ROM: `observer_<rom>` (off, large, 4-record and 4-frame drop-newest, poisoned header, capacity 0 and 1, whole and chunked: identical verdict RAM, time, steps, stop sequence, digest;
  pcm seq = the closed form, psg seq cross-counted from dev); `aud_noninterference` (sinks on and off under zero and v4 seeds 1 and 2); `chunk_equiv_audio_<rom>` (schedules 7, 191, 193, 1366 and one frame; concatenated streams memcmp-equal the whole run; first(i+1) = first(i) + seq(i));
  `chunk_equiv_random32_audio`; `snapshot_restore_audio` with a stride (stops at a sample instant, mid-interval, between R2 and R3, between R0 and R4, inside a TIN to $0806, during DDA streaming, with noise running, between LF TRG 1 and 0); `psg_dev_join`; `psg_fold` (random chunks against one jump,
  writes at event time +-1, the D-186 invariant). Reqs: AUD-02, AUD-03.
- **D-201 Phase 6 contract and the run record.** A run's audio artefacts are the two streams; Phase 6 moves the stream writer into a shared runner library without redefining it. Per call a runner sizes pcm capacity >= ceil((until - T0 + ds_max_step_mclk()) / 192) + 1 and psg capacity by the window-write
  bound or "large"; lost != 0 marks an artefact incomplete, never silently accepted. Video origins and PCM instants share the master-clock base, so A/V alignment is arithmetic. A host-chosen rate, per-channel taps or a level-change stream is a NEW sink or record version, never a reinterpretation of DSPA v1.
  No `[audio]` run-record table in Phase 5 (the D-163 precedent); new constants and withheld ids print through `[constants]` and `[withheld]`; the per-frame counts are runner evidence (`F` rows, `pcm_cadence`), which is how the non-integer cadence of criterion 2 is visible. Reqs: AUD-02.

### E. State, snapshot, power-on

- **D-202 Regions and the one bump.** Three regions appended after SATB, one decoder each in the two-pass walker, explicit little-endian, every member in `DS_INSTANCE_TILING` and in exactly one region (D-122, D-165): `PSG0` registers and latches (psg_freq[6] u16, psg_select, psg_main_bal, psg_lfo_freq,
  psg_lfo_ctl, psg_ctl[6], psg_bal[6], psg_widx[6], psg_dda[6], psg_noise[2]: 42 bytes, then an explicit pad member); `PSGW` the wave RAM (psg_wave[192], byte ch*32 + i); `PSGT` the counters (psg_rem[6] u32, psg_nrem[2] u32, psg_lfsr[2] u32: 40 bytes). No region holds audio output.
  `DS_REGION_COUNT` 21 -> 24; `DS_SERIAL_VERSION` 8 -> 9, `DS_BEHAVIOUR_EPOCH` 5 -> 6 and `DS_TEXT_FORMAT_VERSION` 7 -> 8 are bumped ONCE, in plan 05-03, with this complete layout, seeded-prng-v4 and every PSG constants row. Later plans fill behaviour into the same layout; reliance on an unreleased epoch is
  recorded in the phase summary, and a behaviour change after a push bumps the epoch (04.1 I-05). One recorded correction bump is allowed. Regeneration follows D-165 (the red list is exactly the digest_stream cases, the accept goldens and the size pins; literals computed outside the repository with
  `shasum -a 256`; PSGW under v4 from an independent splitmix64; PSGT, including the LFSR advance and the noise distances, from an independent script; the F-3 cascade list applies). Growth about 290 bytes of instance and snapshot. Reqs: AUD-01, AUD-02.
- **D-203 Restore validation** (pass 1, nothing touched until pass 2; refusal field 12, no new code). select <= 7; every freq <= $FFF; ctl & $20 = 0; widx <= 31; dda <= 31; noise & $60 = 0; lfo_ctl & $7C = 0; every wave byte <= 31; every LFSR in 1..$3FFFF.
  Reachable-state invariants (the 04.1 L-02 class): DDA set implies widx = 0; lfo_ctl bit 7 implies widx[1] = 0; rem[i] = 0 exactly when channel i is frozen (chON 0, DDA 1, or i = 1 with TRG 1), else 1 <= rem[i] <= 24,576 (6,291,456 for i = 1); 1 <= nrem <= 11,904;
  every running counter's next event lies on the tick grid ((machine_time - 1 + r) mod d = 0 for the instance's bound d). Ranges are checked before any arithmetic, so a hostile distance costs at most one bounded fold. Hostile corpus: flip every new control field, per-region perturbation of PSG0, PSGW and PSGT,
  out-of-range distances, LFSR 0, plus the restore stops of D-200. Reqs: AUD-02.
- **D-204 Power-on and seeded-prng-v4.** Every PSG register, the select, every channel distance, latch and LFO state is 0 under EVERY policy (the machine is silent at power-on); the noise distances start at 1 (the event at time 0) and the LFSRs at `psg_lfsr_reset`, constants under every policy.
  No page states a PSG reset state (H8-18 lists none, per advisor A; H8-23 only wires RESET). New additive policy `seeded-prng-v4` (policy_version 4) = every v3 draw in the same order with the same bytes, then the fixed domain `PSGW____` (ASCII-packed like `VRAM____`): 192 bytes straight into psg_wave, each & $1F;
  zero, v1, v2 and v3 give zero wave RAM. The allowed set (a core constant; r1 cannot gain a key) becomes `zero|seeded-prng-v1|seeded-prng-v2|seeded-prng-v3|seeded-prng-v4`; LED-PROF-0002 and LED-PROF-0003 are amended. The default guest policy list becomes zero, v4 seed 1, v4 seed 2 (the same run count;
  v4 seeds draw the same VRAM, SATB and palette bytes as v3 seeds, so the Phase 4 guards persist; v3 keeps its powkinds cases). Audio literal cases run zero and v4 seed 1 (D-176 lever (2) applied from the start), and their PCM and DSPE bytes must be identical across the two (the "initialises what it plays" guard);
  the synthetic case `aud_poweron_policy_diff` keys on a channel with no upload and must differ between v4 seeds 1 and 2. Fallback if 05-03 overruns: drop v4 (wave RAM zero under every policy, a fixture rule that every enabled channel gets all 32 words, the synthetic negative control kept), recorded.
  Reqs: AUD-01, AUD-03.
- **D-205 Constants, no model rows.** No model row is added and the D-113 cap (4) is NOT reopened: the only PSG choice with two implemented values a fixture distinguishes is psg_tick_divisor, which is already a profile field. Constants rows (`constants.def`; each lands with the plan that first uses it,
  prints in `[constants]` and joins `[withheld]` through its ledger id; the union stays under 64, now 34): `psg_mclk_per_sample` 192 and `psg_sample_phase_mclk` 0 (LED-AUD-0014), `psg_tick_phase_mclk` 0 (0012), `psg_f0_as` 4096, `psg_lfo_f0_as` 256 and `psg_noise_nf0_ticks` 32 (0007),
  `psg_lfsr_reset` 1, `psg_lfsr_taps` 137219 and `psg_noise_high_level` 31 (0010), `psg_silence_att` 30 (0008), `psg_select_67_drop` 1, `psg_wave_write_when_on` 1 and `psg_keyon_widx_step` 0 (0004): 13 rows, 47 in all. `psg_vol.def` (30 rows, LED-AUD-0009) is a separate table whose ledger id reaches
  `[withheld]`. Manual-stated numbers are plain defines with a cite comment, not rows: 6 channels, 32 words of 5 bits, the 12-bit F, the factors 2 x 32 and 2 x 32 x 2, w - 16, shifts 0/2/4, the register masks. Reqs: AUD-01.

### F. Evidence, fixtures and the judge (AUD-01, AUD-03)

- **D-206 Oracle policy.** Every audio expected value is a hand-written literal in `fixtures/src/NAME.aexp` with its derivation in the row comment (the D-171 pattern), or a MAN/GUEST literal in a DSV1 check. Rows that carry formula inputs (`f=`, `k=`, `nf=`, `f2=`, `flf=`, `shift=`) are recomputed by the
  static `audio-check` arithmetic lint in integer awk from the page formulas (cycle 192F, step 6F, noise clock 384NFc, LFO-coupled step 6 x F2 x FLF master clocks, sample counts over 192); a mismatch is `AEXP_ARITH` (ER-18). Rejected: cluster B's gate-time awk LFSR window and awk volume-table recompute
  stage, a reference synthesiser, an emulator diff, any value from a DuoSpool run. A throwaway script outside the repository may draw up LFSR bits or VOL values; only the committed literal is judged and its comment names the method. `noise_seq` literals are hand-committed and ASSUMED; `psg_vol.def` is
  checked by the C case `psg_vol_table` (strictly decreasing, VOL[0] = 32768, VOL[29] = 219, nothing indexed at 30 or above) and by ASSUMED `levels_exact` rows. Reqs: AUD-03.
- **D-207 The judge.** Case kinds `audio` (forms ROWS, REL, STRUCT, EV) and `audio_fault_sweep` in `tests/conformance/audiokinds.c`, parsing DSPE and DSPA with hand-written offsets. Grammar of `.aexp`: `scene <S> case=<id> role=literal|golden|expect_red [twin=<S>]`;
  `group <S> <gid> form=ROWS|REL|STRUCT|EV kind=<kind> class=MAN:<pages>|GUEST:<model>|ASSUMED:<LED-id> kills=AF..`; `row <S> <gid> <selector> <literal>`. Selectors anchor on the event log with 0-based select values (`anchor=ev:ch=1;reg=R4;val=$9F;nth=1` plus `from=+<mclk>;to=+<mclk>;side=L|R`),
  converted to sample indices with the DSPA header's sample_mclk and first_frame by runner functions, never by core tables; the `.aexp` header's S must equal the stream's sample_mclk (a mismatch is harness_failure). Limits as `.vexp`: 512 rows, 400 characters per line, 16 KiB, fail-closed on unknown keys;
  `fixture-build` writes `NAME.aexp.tsv` (the F-5 channel). ONE new static gate stage `audio-check` (59 stages): grammar, KINDCLASS, DISPUTED, arithmetic lint, RULES map, kill and vacuity rules, toy registry, negative mini-roots under `tests/audio/`. The selftest gains three mutations (remove
  `INV_AUDIO_HASH_ONLY`, remove `AUDIO_GROUP_VACUOUS`, remove the kind-to-class lint), each turning the gate red. Reqs: AUD-03.
- **D-208 Isolation and row kinds.** Channel-specific PCM rows are judged on solo scenes (one channel chON = 1) or pan pairs (one channel hard left, one hard right, the muted side at a sum >= 16, e.g. AL = $1D so AL' = E, never exactly 15); no tap. Kinds and maximum classes (KINDCLASS):
  `ev` value and order GUEST:program; `ev` decode (chan, reg, effect, post) MAN:P8-2+P8-3+P8-4 (the core's decode against a literal written in the row, never a judge-side decode); `ev` at select 6/7, reg >= $A or R6 while playing ASSUMED;
  `period` (K cycles are exactly K*F samples at d = 6) MAN:P8-9+H8-23; `period_ratio` REL MAN:P8-9; `cycle_order` (the 32 interior levels of one cycle are order-isomorphic to the written words up to rotation; a step must span >= 2 samples, so F >= 64) MAN:P8-4+P8-5;
  `cycle_phase`, `cycle_linear`, `levels_exact` ASSUMED; `silent` (every sample of the window on that side is 0) MAN:P8-4 for chON = 0 and MAN:P8-10 for sums >= 16 or R1/R5 byte zero, ASSUMED at 15; `live` (a period or cycle_order property with >= 3 distinct interior levels, never "nonzero");
  `same` REL (off-invariance, `r7_ignored`, `lfo_needs_sel1`) MAN; `atten_order` REL (strict decrease for LAL F, E, D and AL $1F, $1E) MAN:P8-3+P8-4+P8-10; `atten_equal` ASSUMED; `dda_hold`, `dda_order`, `dda_ram_kept` MAN:P8-4+P8-5; `dda_equals_wave`, `dda_level_exact` ASSUMED;
  `two_level`, `run_grid` (every run a multiple of 2*NFc samples), `noise_ratio` MAN:P8-2+P8-6+P8-9+H8-23; `run_min`, `noise_seq` ASSUMED (`noise_seq` names its policy); `lfo_static` (TRG = 1, select-1 address 0 = $18, CTL 1/2/3 give select-0 cycles of 192(F0 + 8), 192(F0 + 32), 192(F0 + 128)) and
  `lfo_needs_sel1` MAN:P8-6..P8-9+H8-23; `lfo_dynamic` (cycle lengths exactly {192(F0 + 8), 192(F0 - 8)} excluding straddling cycles) MAN; LFO switch timing not judged; per-frame counts STRUCT ASSUMED:LED-VID-0001; DSPE and DSPA header fields STRUCT definition (ER-19).
  Lint `AUDIO_SILENCE_UNPAIRED`: every `silent` row needs a `live` row on the same side in the same scene, so a null synthesiser fails. Reqs: AUD-03.
- **D-209 Fault sweep, vacuity and hash roles.** AF0..AF9 applied in memory to the captured streams of one run: AF0 perturb one of the group's literals, AF1 shift the PCM +1 sample against the log, AF2 drop a sample mid-window, AF3 swap L and R, AF4 duplicate every 32nd sample, AF5 zero the window,
  AF6 replace the window with a constant non-silent value, AF7 reverse each 32-step cycle, AF8 drop one event record, AF9 relabel one event's channel k -> k+1 mod 6. The D-172 rules apply: a group no declared fault flips is `AUDIO_GROUP_VACUOUS`, an empty `kills=` is `INV_AUDIO_GROUP_UNKILLED`, an AF0-only
  group names a toy whose evidence shows it red or is listed in `tests/conformance/AUDIO_TOY_EXEMPT` with a reason; the expected kill matrix of advisor D is a planning guide, verified by running. Null-synth controls (no PCM, all zero, a constant) are red; a missing sink is harness_failure.
  Hash roles (ER-20): at most one golden per audio ROM pinning both stream digests (`INV_AUDIO_HASH_ONLY`, runtime refusal `audio-hash-only`); in-process `audio_hash_not_verdict` (an AF7-faulted stream passes its own golden and fails the literal case); golden digests are regenerated on an epoch bump with old
  and new recorded in the `.aexp` comment; noise repeat digests are E0, never goldens. Reqs: AUD-03.
- **D-210 Toy mutants, audio cap 7** (its own cap; D-173 untouched; generated by `scripts/control-mutate`, one anchored statement each; every EXPECT names literal groups, with the F-4 second column for groups that must stay green):
  `psg-emit-mutates` (05-04; red: aud_noninterference, observer), `psg-fold-skipped-at-step-end` (05-04; red: chunk_equiv_audio, observer seq), `psg-chsel-mask` (R0 & 3; 05-06; red: ev decode and the select 4/5 scenes; green: select 0/1 scenes),
  `psg-atten-max` (the largest term instead of the sum; 05-06; red: the P8-10 example and sum-16 silent rows; green: live rows at sum 0), `psg-dda-no-reset` (05-07; red: cycle_phase; green: cycle_order, the symmetry proof),
  `psg-noise-taps` (05-07; red: noise_seq; green: two_level and run_grid, the seed-free proof), `psg-lfo-shift` (shifts 0/2/4 -> 0/1/2; 05-09; red: lfo_static CTL 2 and 3; green: CTL 1; it moves to 5.1 with a cut). The factor-2 period class is covered by the d = 3 expect_red case and the off-by-one
  class by the F + 1 twin (D-211), so there is no period toy. Cost about 7 to 17 s CPU, parallel. Reqs: AUD-03.
- **D-211 Criterion-3 controls.** (1) In-ROM twin: `psg_tone` plays scene T, then T', a byte copy of T's register table except one channel at F + 1 and one channel never keyed on; `.aexp` declares `scene T' role=expect_red twin=T expect_red=<groups>`; the judge requires T all green and, in T', exactly the
  listed groups red and every other group green (anything else is the harness failure `misprog-misattributed`). F + 1 tests the resolution at its minimum (K cycles move by K samples at d = 6). (2) Wrong divisor: the period scene of `psg_tone` bound to psg_tick_divisor = 3 is expect_red on its period
  groups (the cycle halves) while cycle_order and silent stay green. Both run on every gate and use no ROM slot. Rejected: a `-D MISPROG` variant ROM (a slot) and runner-side patching (couples the runner to code offsets). Reqs: AUD-03.
- **D-212 Fixtures and C-level cases.** Three authored ROMs (0BSD, ca65/ld65 at gate time, zero and v4 seeds per D-204, one `mutate_sweep` each, DSV ids 27-29); each writes every register and all 32 words of every channel it enables and judges windows after its own initialisation; CODE 4,096 bytes per ROM.

  - **`psg_tone`** (DSV 27; 05-04, grown in 05-06): power-on silence; the tracer scene on select 0 (P8-5 procedure (2) upload by TIN of an asymmetric 32-word table with a unique maximum and >= 8 distinct values, F a multiple of 64, R1 = R5 = $FF, AL = $1F); three pan-pair scenes over select 0..5
    with distinct asymmetric tables; the P8-10 worked example and sum-16 rows; atten_order; the off-invariance pair; event rows for every R0..R9 and each channel's R2..R6, the ASSUMED select-6 and reg-$A writes and one mirror-alias write; twin T'. If CODE overflows, scenes move to `psg_mod` before a 4th slot.
  - **`psg_mod`** (DSV 28; 05-07): noise solo on select 4 and 5 (two NFRQ values, one with NFc = 7), `r7_ignored` at select 2, a timer-IRQ-paced DDA scene with GUEST schedule rows in DSV1 (N handler runs, N writes, the table walked in order, recomputed differently), dda_hold/dda_order/dda_ram_kept.
  - **`psg_lfo`** (DSV 29; 05-09; moves to 5.1 with a cut): lfo_static CTL 0..3 with select 1 inaudible through R5 = $00 (the MAN byte-zero clause), lfo_needs_sel1, lfo_dynamic with words 0-15 = $18 and 16-31 = $08, F0 + 128 <= $FFF (no wrap).

  C-level cases (no ROM): `psg_decode` (mirrors, select 6/7, R7 at select 0..3, masks), `psg_wave_ptr` (P8-5 procedures (1)-(3), the DDA level reset, one ASSUMED write-while-on row), `psg_lfo_trg`, `psg_poweron` (zero under zero and v1..v3; v4 bytes against an independent splitmix64 literal),
  `psg_pure_function` (closed forms 192F, 6F, 384NFc against literals for F in {1, 254, 3421, 4095} and NFc in {1, 7, 31}; P8-9, P8-12), `psg_vol_table`, `psg_fold`, `psg_random_regs` (32 random register programs including F = 0, NFc = 0, FLF = 0 and F1 = FLF = 1; E0, sanitizers: termination,
  no out-of-range index, lost = 0), `psg_block_flood` (a 65,536-byte TIN to $0806: lost counts, digest unchanged with and without sinks), `aud_poweron_policy_diff`, `pcm_cadence` (262 and 263 lines: each count in {floor, ceil}, the 32- and 64-frame sums, the stream total), `run_refuse_call_overlap`,
  `audio_noise_repeat` (two fresh interleaved instances per policy under zero and v4 seeds 1 and 2: identical PCM and DSPE bytes, printed as E0 repeatability). Rejected: cluster E's `psg_tone_bad` (the twin replaces it) and `psg_reg` (C-level cases plus the event rows replace it); cluster A's
  zero-ROM plan (promotion needs guest rows, ER-22); cluster D's `audio_agree` runner kind (ER-3 circular; `dda_hold` and the GUEST schedule rows cover it). Reqs: AUD-01, AUD-03.
- **D-213 Ledger** (written once in plan 05-01; ids final; area AUD; `profile = "pce-hucard-2btn"`; `pinning_test = "PENDING:5"` until a closing plan sets `TEST:`; `revisit_phase = 5`; each with a `ledger/REQUIRED` line `ID 05-CONTEXT D-nnn`; later plans edit pins and statuses and may amend a statement only to
  correct it). All entries are `assumed` unless marked E1 candidate; promotion only by ER-2 and ER-22 in 05-08 or 05-10.

  - **LED-AUD-0001** (amend). Statement: DuoSpool clocks every PSG counter at master/d, d the bound psg_tick_divisor; d = 6 equals P8-9 with fmaster = OSC1/3 (H8-23), checked against the P8-12 table; d = 3 doubles every PSG frequency relative to those pages; no hardware measurement exists.
    Sources add FACT P8-9, P8-12, H8-23 (img). Disagreement rewritten: the manual's formula carries the factor 2, so "7.16 MHz pin" and "effective master/6" are one formula; what stays unmeasured is the hardware. Status unchanged (ER-14). Withheld: as now plus pitch-accurate. Dec: D-183.
  - **LED-AUD-0002** (amend; E1 candidate). Channels are the R0 select values 0..5 = the manual's ch1..ch6 (P8-3); noise on select 4 and 5 (P8-2, P8-9); select 1 modulates select 0 (P8-2, P8-6). Sources replace the second-hand rows; the disagreement records a naming-base difference only.
    Withheld: any noise or LFO behaviour beyond placement. If the LFO is cut, its placement clause has no guest row and the entry stays assumed until 5.1. Dec: D-189.
  - **LED-AUD-0003** (new; E1 candidate). VL0 register map: A3-A0 select R0..R9; the R0..R9 fields and masks; R1, R8, R9 ignore R0; R7 exists for select 4 and 5 only. Sources FACT P8-2, P8-3, P8-4, P8-6, P8-9, H8-24. Withheld: register read-back, undefined slots, hardware-verified. Dec: D-188, D-189.
  - **LED-AUD-0004** (new). Register edges: the 16-byte mirror (H8-24 inference), select 6/7 drops R2..R7, reads return io_buffer, no access penalty, an R6 write while playing stores with no increment (pcetech disagrees), key-on leaves the counter (Mednafen +1 disagrees), the D/A latch loads with chON = 0.
    Withheld: everything. Dec: D-188..D-190.
  - **LED-AUD-0005** (new; E1 candidate). Wave RAM modes: write mode stores and increments; DDA = 1 resets the address counter; in DDA mode a write goes to the D/A and is held, wave RAM unchanged; chON = 0 turns the output off. Sources FACT P8-4, P8-5. Withheld: playback phase, the pointer while off,
    writes while on. Dec: D-190, D-191.
  - **LED-AUD-0006** (new; ER-14 hold). With d = 6 the channel cycle is 192F master clocks for F >= 1, the noise clock 384NFc for NFc >= 1 and the LFO-coupled step 6 x F2 x FLF (P8-9, H8-23, P8-12). Evidence lists the MAN period rows; status assumed until C1 is settled. Withheld: pitch-accurate,
    any PSG timing claim, cycle-accurate. Dec: D-182..D-184.
  - **LED-AUD-0007** (new). Zero and wrap conventions: F = 0 as 4096, FLF = 0 as 256, NFc = 0 as 32 ticks, F0' masked to 12 bits with 0 as 4096. Sources CODE (four emulators); the pages call these forbidden or undefined. Dec: D-184.
  - **LED-AUD-0008** (new; E1 candidate scoped to the undisputed points). Silent when chON = 0, when R1 or R5 is $00, or when the P8-10 attenuation sum is >= 16; the level strictly decreases per R1, R4 and R5 step (P8-3, P8-4, P8-10). The model also silences at sum 15; that boundary is the recorded
    disagreement ("exceeds F" against the worked example) and is outside the statement. Withheld: the sum-15 boundary, dB values, loudness. Dec: D-194.
  - **LED-AUD-0009** (new). Output model: unsigned 5-bit levels, the Q15 curve 10^(-3a/40), linear stereo sum, no DC removal or filter, HuC6280 not 6280A. Disagreement: Mednafen 2^(-a/4) and its revision split, MAME 48 dB. Withheld: volume-accurate, any loudness or dB claim. Dec: D-191, D-194.
  - **LED-AUD-0010** (new). Noise generator: 18-bit LFSR, taps 0/1/11/12/17, right shift, output bit 0 as 31/0, reset 1 under every policy, always clocking, one per noise channel, NE replaces the wave. Disagreement: MAME clocks only with NE. Withheld: noise sequence, noise spectrum, noise timing. Dec: D-193.
  - **LED-AUD-0011** (new; ER-14 hold). LFO: active when CTL != 0, TRG = 0 and select 0 and 1 both on; TRG = 1 holds address-0 data and resets the select-1 counter; modulation w - 16 shifted by 0/2/4 (P8-6..P8-9); one combined counter; the read instant and the restart are assumed.
    Withheld: LFO-accurate, LFO rate or depth claims. Dec: D-192.
  - **LED-AUD-0012** (new). Counter timing: tick grid phase, reloads, write-to-effect at the next reload, key-on timing, freeze rules, the wave counter under NE (Mednafen versus Mesen2), the tie order, immediate volume. Withheld: any sub-period timing claim. Dec: D-185.
  - **LED-AUD-0013** (new, definition, the LED-VID-0018 pattern). The DSPE v1 record of D-196; a hash of it is E0. Withheld: register read-back, hardware register state. Dec: D-196.
  - **LED-AUD-0014** (new, definition). The DSPA v1 stream of D-197: point samples at s x 192 master clocks, 2 x u32 unsigned, no filtering, resampling or DC removal, the tie order; a hash of it is E0. Withheld: hardware sample rate, analog output, band-limited, sample-accurate. Dec: D-187, D-197.

  Amendments: LED-PROF-0002 (v4 in the allowed set), LED-PROF-0003 (PSG power-on: registers zero, LFSR constant, wave RAM drawn only under v4), LED-CPU-0013 (the PSG read path is unchanged by a live device), LED-CPU-0017 (the PSG window carries no penalty, unchanged). E1 candidates: at most four,
  LED-AUD-0002, -0003, -0005, -0008. Image-verify in 05-01 before any MAN tag: P8-2 to P8-10 and P8-12 (P8-1 and P8-11 are not used by any decision), H8-18, H8-23, H8-24; `cites.txt` gains a `P8-` legend line (label P8-n = PDF page n+2);
  `TOOLS.md` records the PSG manual: https://raw.githubusercontent.com/MiSTer-devel/TurboGrafx16_MiSTer/master/docs/HuC6280%20-%20CMOS%20Programmable%20Sound%20Generator%20Manual.pdf ; 934252 bytes ; sha256 `2f463fd388cd64bc1bef79d259cf456c2c6d4bd8aaba25918b71f2443af6c04b` ; 14 PDF pages.
  Reqs: AUD-01..AUD-03.

### G. Gate, CI, process and plans

- **D-214 Gate cost and budgets.** One new stage, `audio-check` (59 stages, about 2 s like `video-check`). Audio scenes stay short (at most 16 frames each, about 100 to 150 audio frames in the phase). Estimates to be re-measured in 05-04 and 05-08: about 45 to 60 new REQUIRED ids;
  CI critical path +25 to +50 s central; local gate +10 to +20 percent. The per-step PSG fold on every guest run is the largest cost risk (cluster C): measure it in 05-04 (D-176 style). Staging triggers and the D-176 lever order apply unchanged; nothing is weakened to save seconds.

  | Budget | Now | Phase 5 | Rule |
  |---|---|---|---|
  | CI critical path | 562 s (gate-mawk 555 s, 4.1 run) | lever (b) predicts about 440 s; then +25 to +50 s | D-176 job trigger 600 s per job |
  | Local warm gate | the 4.1 close-out run, re-recorded in 05-01 as the baseline | +10 to +20 percent | cut trigger at +25 percent |
  | Guest ROMs | 36 of OX_ROMS_MAX 40 | +3 (39); OX_ROMS_MAX 40 -> 48 in 05-02 as harness capacity | a 4th audio ROM is a recorded deviation |
  | DSV ids | next 27 | 27 psg_tone, 28 psg_mod, 29 psg_lfo | fixed here |
  | Toys | 26 (video cap 10 spent) | audio cap 7 (33 in all; 6 after a cut) | D-173 untouched |
  | Model rows | 4 of 4 | 0 | D-113 not reopened |
  | Constants | 34 of 64 | +13 (47) | D-168 rules |
  | Serial / epoch / text / regions | 8 / 5 / 7 / 21 | 9 / 6 / 8 / 24, once in 05-03 | one correction bump allowed |
  | Policies | default zero, v3 s1, v3 s2 | default zero, v4 s1, v4 s2; audio literal cases zero and v4 s1; noise repeat under all three | D-204 |
  | Ledger E1 candidates | 13 supported entries | at most 4 new | ER-14, ER-22 |
- **D-215 CI levers.** Plan 05-01 applies the two levers recorded in `docs/ci-timings.md` ("Efficiency review of the first run of the three-job shape"), each as its own revertible workflow commit with `scripts/workflow-check` and its aggregator control updated: (a) print `nproc` and the CPU model in the
  tool-versions step of every job; (b) move the clang build and conformance step into a third parallel job that the `gate` aggregator needs. The expected critical path (about 440 s on the 4.1 run) is recorded as a prediction to measure on the first Phase 5 CI run. Lever (c), skipping sanitizers in the
  mawk leg, is rejected (it needs a skip mechanism against the fail-closed rule and drops a distinct draw). The 4.1 review recorded workflow changes as the owner's decision; the owner later authorised the parallel split, so these reversible efficiency edits land locally, and the publish step asks the
  owner once, yes or no, whether to keep them before the push (a no reverts both commits). This is a hand-off at publish time, not a new authority gate.
- **D-216 Process rules (every plan; plan-check verifies each).** Carried: D-177 (a)-(g) and the Phase 3/4 core rules (no malloc, no file I/O, no clocks, no env, no floats, no mutable globals or function statics, defined behaviour only, strict C11, new core files include only stddef/stdint/ds_core.h,
  the core-boundary allow list unchanged). Added from the Phase 4 and 4.1 reviews:
  R1 every expected literal carries its class (MAN:P8-n with a cites.txt img row, GUEST:<model>, ASSUMED:LED-AUD-x) when the row is written, never later (ER-15, ER-16); R2 weakest class wins (ER-10): a literal that uses the volume curve, the LFSR, the tie order or an edge convention is ASSUMED;
  R3 every behaviour rule of this file has a literal row and a named mutant or fault that kills it before its plan closes, or is listed as a named limit (ER-17); R4 every enforcement rule (judge, lint, restore domain, docs) lands with a control or selftest that turns red when the rule line is deleted;
  R5 a toy's EXPECT names the literal group and its evidence shows that group failing; R6 a new fixture lands with chunk, observer, restore and non-interference coverage in the same plan (D-200); R7 no literal, tolerance or count from a DuoSpool run, and an independent script first reproduces a
  committed value; R8 README, validation and ledger text are written from gate output and checked by docs-claims, no "complete" before the validation file is signed; R9 unsigned widening before shifts, bounded snprintf, no POSIX calls; R10 case files designed for PK_LIST_MAX 512 and 400-character
  lines, ASCII, SPDX; R11 a behaviour change after a push bumps the epoch; R12 same-author decoders and scripts are labelled E0/E2.
  Cadence: per task, build plus `sh scripts/check --stage` for the stages it touches, the named cases through `build/gate/ds_runner` and the task's own red control; per plan end, `sh scripts/check`, `--selftest`, `audit-history --range`, and for C plans `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro`;
  05-08 and 05-10 add dash, the second awk and all six Linux parts; one gate per checkout at a time. At most 3 tasks per plan, each plan with a named stop point at a task boundary (stop and split near 60 percent of context after task 1).
  Handbook: 05-01 writes `05-HANDBOOK.md` (target 15-20 KB, cap 25 KB) derived from this file: scope, one line per D-id, the ER rules, the live budget table each SUMMARY updates, the file map, the cadence, C11 footguns, the discrepancy list and these rules. Plans list the handbook plus named code files in
  read_first and cite this file by anchor (`05-CONTEXT D-2nn`); a handbook line that contradicts this file is a defect, and the handbook changes only in 05-08, 05-10 or a recorded deviation.
- **D-217 Plans and the cut line.**

  - **05-01**. Wave: W1. Content: evidence data, no C: the image reads recorded (verifier, date, numbers seen) for the D-213 pages, the `P8-` legend, the TOOLS.md row, 12 new ledger entries and 6 amendments with PENDING:5 pins and REQUIRED lines, docs-check withheld words with its negative control,
    `fixture-lint` `P8-` prefix and `.aexp` class tags, `05-HANDBOOK.md`, the CI levers of D-215 as two commits, the local gate baseline. Reqs: AUD-01..03 (evidence).
  - **05-02**. Wave: W1 (parallel-safe with 05-01). Content: sink plumbing with no producer: both sink structs, the two request members, the 5-channel table, `ds_obs_begin` once, refusal and sink-shape controls, `run_refuse_struct_size`, IN-03 (`run_refuse_call_overlap`), OX_ROMS_MAX 48 with its error text,
    an empty `ds_psg_advance` at both fold points; digests unchanged. Reqs: AUD-02.
  - **05-03**. Wave: W2. Content: the complete layout and the ONE bump: PSG0, PSGW, PSGT, serial 9, epoch 6, text 8, regions 24, seeded-prng-v4, every constants row, `ds_psg_reg_mask`, decoders with every D-203 rule, tiling, boot init of PSGT, register decode and storage (D-188..D-190, D-192 register side),
    the always-running noise counters and LFSRs in the fold (so every existing pin is final here), `psg_decode`, `psg_wave_ptr`, `psg_lfo_trg`, `psg_poweron`, hostile corpus, regenerated digests and goldens, default policy list, powkinds, snapkinds. No synthesis and no record emission. Reqs: AUD-01, AUD-02.
  - **05-04**. Wave: W3. Content: TRACER, the first and only vertical slice before the judge work: channel stepping (closed form, key-on, freeze), synthesis (D-191, D-194), DSPE and DSPA emission, the minimal `audio` kind (EV, period, cycle_order, silent), `--audio-out`, `psg_tone` with power-on silence and the
    select-0 scene, the d = 3 expect_red case, observer, chunk_equiv, snapshot_restore_audio, aud_noninterference, psg_dev_join, psg_fold, pcm_cadence on `psg_tone`, toys `psg-emit-mutates` and `psg-fold-skipped-at-step-end`, the fold cost measurement. It proves: a guest write is stamped, folded,
    decoded into a DSPE record and heard in DSPA samples at the defined grid; a hand literal from P8-4, P8-5, P8-9 and H8-23 passes; the wrong binding and the toys turn it red; sinks change no state. Reqs: AUD-01..03.
  - **05-05**. Wave: W4. Content: judge hardening: the full `.aexp` grammar, REL and STRUCT forms, AF0..AF9, `audio-check` with KINDCLASS, DISPUTED, the arithmetic lint and RULES, negative mini-roots, null-synth and missing-sink controls, the golden role and `audio_hash_not_verdict`, the three selftest mutations,
    AUDIO_TOY_EXEMPT. Reqs: AUD-03.
  - **05-06**. Wave: W5. Content: six channels: pan pairs over select 0..5, event rows for every register, the attenuation and silence rows, atten_order, off-invariance, twin T', `psg_vol_table`, toys `psg-chsel-mask` and `psg-atten-max`. Reqs: AUD-01, AUD-03.
  - **05-07**. Wave: W6. Content: noise output and DDA: NE gating, the LFSR level path, `psg_mod`, the timer-paced DDA scene, noise and DDA rows, `audio_noise_repeat`, `psg_random_regs`, `psg_block_flood`, toys `psg-dda-no-reset` and `psg-noise-taps`, the D-200 cases on `psg_mod`. Reqs: AUD-01, AUD-03.
  - **05-08**. Wave: W7. Content: checkpoint A: all six Linux parts, sh, dash and the second awk, the CI-cost measurement against D-214, a one-off scratch mutant sweep of the PSG code recorded in the SUMMARY (not CI), the ledger promotion pass for covered candidates, THE CUT DECISION. Reqs: all so far.
  - **05-09**. Wave: W8. Content: LFO: the combined counter, TRG, CTL shifts, F', `psg_lfo`, lfo rows, the UNMODELLED path retired for R9, toy `psg-lfo-shift`, the D-200 cases on `psg_lfo`. Reqs: AUD-01.
  - **05-10**. Wave: W9. Content: close-out: ER-2 and ER-22 promotions, README audio paragraph from gate output, pins, validation file from gate output, `docs/ci-timings.md` Phase 5 sections (05-04, 05-08, 05-10), all six Linux parts, verification report, hand-off list. Reqs: AUD-01..03.

  Cut line: at 05-08 the cut is taken if any of: (1) more than two revision plans (05-16+) were needed; (2) either CI work job exceeds 600 s after the D-215 levers and D-176 lever (1); (3) the local warm gate user CPU grew more than 25 percent over the 05-01 baseline on the same machine and load (both recorded);
  (4) the LFO needs a new model row or a discrepancy the pages cannot settle; (5) the LFO fold cannot be bounded per D-186; (6) the owner prefers an earlier pull request. Then 05-09 becomes plan 5.1-01 of an inserted Phase 5.1 (with `psg_lfo` and `psg-lfo-shift`), 05-10 closes Phase 5 with the LFO
  under DSPE UNMODELLED, and the ROADMAP criterion 1 wording gains "LFO registers logged and decoded; LFO modulation in 5.1". Cut order if more must go: LFO first, then the noise repeat case drops to two policies; never DDA and never the period, order or silence rows (criterion 3). The cut is recorded,
  never a gate failure. It is clean because the layout is final in 05-03 and the LFO touches only the select-0/1 pair. Coupling: 05-04 to 05-09 share the PSG code and run strictly in order; only 05-01 and 05-02 run in parallel.
- **D-218 Claims and withheld words.** After Phase 5 the README and ledger may say: "executes the authored PSG diagnostic fixtures and emits a timestamped PSG register-event log (DSPE v1) and PCM (DSPA v1: one point sample per 192 master clocks, 2 x u32 unsigned) before any host resampling";
  "E1 documentary: the entries the README lists as supported equal the cited PSG manual page, not a hardware measurement"; "the channel period rows equal the manual formula under the bound psg_tick_divisor 6; the PSG clock stays an open profile field, so no pitch or timing claim is made";
  "noise output repeats across fresh runs under the same power-on policy (E0)"; a hash only as repeatability evidence. `docs-check` gains these withheld words (whole word and compound, with its negative control): pitch-accurate, sound-accurate, audio-accurate, sample-accurate, noise-accurate, LFO-accurate,
  volume-accurate, band-limited, sounds correct, plays music, authentic sound, audio-compatible (accurate, cycle-accurate, timing-accurate, hardware-verified, compatible and plays games already exist).
  Phase 5 will NOT claim: any pitch in Hz, the PSG clock, any absolute period or frequency ratio as a hardware fact, the sample rate as the hardware's, the noise sequence or spectrum, the volume curve or dB values, DC level or centring, the 6280A, leak, filtering, F = 0, NFc = 0 or FLF = 0 behaviour,
  R6 writes while playing, the key-on phase, LFO timing, the sum-15 boundary, PSG read values, and that any commercial software sounds right. Reqs: AUD-01..03.

### Claude's Discretion

File and symbol names (`psg.c`, `ds_psg.h`, region member names beyond those fixed here), field order beyond the byte layouts fixed here, the pad member of PSG0, the exact wording of ledger statements beyond the tags and facts above, how tasks are cut inside the 10 plans (at most 3 each), the
grouping of conformance case files, the asymmetric wave tables and the F values (multiples of 64), the per-scene frame counts within D-214, how the `psg_vol.def` ledger id reaches `[withheld]`, and the TSV reader versus the awk subset parser for `.aexp`.

</decisions>

<reconciliations>
## Reconciliations (adversarial pass: about 35 cross-cluster conflicts, each resolved by one ruling; the discussion log has the table)

- **PCM cadence.** 192 master clocks (B) over 450 (C): exact periods and noise grids for every F and NFc, less aliasing, negligible cost; neither rate depends on the divisor binding (D-187).
- **PCM frame.** 2 x u32 unsigned, 8 bytes: neither B's 16-byte frame with a per-channel tap nor C's u16 (D-197).
- **Event log.** A new core `psg` sink with decoded DSPE records (A, C) over runner extraction from `dev` (E): EV rows must judge the core's decode, not a same-author replay; `dev` keeps logging and gives a free 1:1 join (D-196, D-198).
- **Fold representation.** Distances from the fold point with nothing derived stored (C) over absolute next-event times with window checks and a stored next_sample (B); the call sites, the tie order and the step-end position were already agreed (D-186).
- **psg_tick_divisor.** Literal (A, B) over documentary equivalence (D, E): an OPEN field must be observable; d = 3 becomes the expect_red negative (D-183, D-211). Absolute period rows are MAN (H8-23 is image-verified), against cluster E's ASSUMED class; the ledger hold is ER-14.
- **What may reach supported.** Register map, numbering, wave RAM and DDA modes, undisputed silence rules (at most 4); the period entry (D's LED-AUD-0005 candidate) and the LFO entry are held by criterion 4 (ER-14, D-213).
- **Silence boundary.** The model silences at sum >= 15 (A, B); MAN rows sit at >= 16, at chON = 0 and at R1/R5 byte zero; 15 is ASSUMED (D, E; D-194). Audible rows at sums 5..14 are ASSUMED (ER-16), narrowing D's MAN range.
- **R6 while playing.** Store at the counter with no increment (A) over WAVE_DROP (C); the flag becomes WAVE_HELD (D-190, D-196).
- **Power-on.** seeded-prng-v4 for wave RAM now, inside the one bump (A, B, C) over deferral (E); registers and LFSR constant under every policy (all) (D-204).
- **ROM count.** Three ROMs (`psg_tone`, `psg_mod`, `psg_lfo`) over 0 (A), 2 (D) and 4 (E): guest rows are needed for promotion, the twin replaces a bad-ROM variant, a separate LFO ROM keeps the cut clean (D-212).
- **OX_ROMS_MAX.** Raised to 48 as harness capacity (E), while the phase budget stays 3 ROMs (D-214).
- **Toys.** One merged list, cap 7 (D, E and C proposed 6, 6 and 2); the factor-2 and off-by-one classes are covered by the d = 3 case and the F + 1 twin (D-210).
- **Oracles.** The arithmetic lint stays (D); B's gate-time LFSR generator and volume-table recompute stage are rejected (ER-18, D-206).
- **Run record.** No `[audio]` table (C, D-163 precedent) over E's table; per-frame counts are runner evidence (D-201).
- **LFO fallback.** E's UNMODELLED flag moves from the PCM record (which has no flags) to the DSPE effect bits (D-192, D-196).
- **LFO counter.** One combined counter (P8-6) over a separate LFO divider and next_lfo (B) (D-185).
- **Regions.** PSG0, PSGW, PSGT: A's split by kind with C's names, not C's per-channel PSGC (D-202).
- **Noise LFSR clocking.** Always (B; three emulators and a community quote) kept despite its cost: PSGT evolves in every run, so the noise counters land in 05-03 with the bump (D-193, D-217).
- **Tracer content.** E's square wave replaced by an asymmetric table (a square cannot show order errors); the tracer follows the layout plan, not the plumbing plan (D-212, D-217).
- **Plan numbering.** Splits 05-11..05-15 and revisions 05-16+ (E proposed 05-15..05-19 and 05-20+, leaving an unexplained gap) (D-217).
- **CI levers.** In 05-01 as two revertible commits, with a yes/no to the owner at publish time (D-215).
- **Policies per audio case.** zero and v4 seed 1 (E's lever (2) adapted to v4) (D-204).
- **audio_agree.** Dropped (D's AD-15 option B is ER-3 circular) (D-212).
- **Chunk schedules.** 7, 191, 193, 1366 and one frame (B's set adapted; C's 449/450/451 follow the rejected rate) (D-200).
- **Naming.** Every advisor's 1-based or "ch2" wording is converted to select values; D's `lfo_needs_ch2` is `lfo_needs_sel1` (D-208).
- **Ledger ids.** The five provisional numberings are replaced by D-213.

### Writer notes (resolved conservatively)

1. P8-10 is internally inconsistent at sum 15 (text "exceeds F" against the worked example and the 45 dB sentence). The model follows the example; rows at 15 are ASSUMED (D-194).
2. P8-8 draws only the low-order 4 bits of the wave word added at each shift, while P8-7 maps the 5-bit word to w - 16 with the MSB as XSGN. The model uses w - 16 (sign extension); the width of that extension is a reading, and no row wraps the 12-bit sum.
3. Manual cross-reference errors, cite by page label only: the noise section is printed 2.3.3 inside 2.2 (P8-9); 2.1.3 refers to 2.3 for frequency (it is 2.2, P8-3); 2.1.5 refers to 2.1.7 for amplitude (it is 2.3, P8-3); 2.1.8 and 2.1.9 refer to 2.3.3 and 2.3.2 (P8-6).
4. OCR errors found by the image reads: the page prefix is P8, not PB or PS; the P8-7 row for 01000 is -8 (the OCR drops the sign); the P8-12 C1 row is R2 = 5D (the OCR shows 50). The image is authoritative.
5. "R1, R4 or R5 of some channel is 0" (P8-10) may mean the byte or a field. MAN rows use the byte reading only (R1 = $00, R5 = $00); field readings are ASSUMED unless the sum is >= 16 anyway. The model silences under both readings.
6. P8-6 says the LFO is active when channels 1 and 2 are "enabled"; the model reads enabled as chON (ASSUMED).
7. There is no 05-RESEARCH yet. If the research step finds an error in this file, it records it as an erratum binding on the planner, in the 04-CONTEXT writer-notes pattern.

</reconciliations>

<discrepancies>
## Discrepancy list (the planner preserves each in the named ledger `disagreement` field)

- **DX-1 C1 clock (LED-AUD-0001).** P8-9 "7.16 MHz supplied by the CPU" with 2 x 32 x F, H8-23 OSC1/3, P8-12 at 7.16 MHz, against the master/6 x 32F wording of pcetech and three emulators: one formula (6F per step). Only OSC1/3 x 32F contradicts the pages; d = 3 implements it.
- **DX-2 Numbering base (LED-AUD-0002).** The manual is 1-based (ch1..ch6), ARCHITECTURE and DuoSpool 0-based (= R0 value); PITFALLS and Mesen2 notes are 1-based.
- **DX-3 Noise LFSR (LED-AUD-0010).** Taps, width, seed and levels come from emulators and a community quote; the page says only "pseudo random noise (square wave)". MAME clocks the LFSR only with NE set.
- **DX-4 LFO arithmetic (LED-AUD-0011).** P8-8 adds 4 bits; P8-7 maps to w - 16; the extension width is a reading. The read instant and the TRG restart instant have no page.
- **DX-5 Silence at sum 15 (LED-AUD-0008).** P8-10 text against its own worked example; Mednafen silences only at its reduction >= 31 on a non-linear table; MAME spreads 48 dB.
- **DX-6 Revision (LED-AUD-0009).** HuC6280 versus HuC6280A gating and centring (Mednafen's revision split); the profile binds huc6280.
- **DX-7 Leak and range (LED-AUD-0009).** P8-10: about 45 dB, a circuit design value, and "a little leak"; no leak is modelled.
- **DX-8 DDA latch with chON = 0 (LED-AUD-0004).** An inference from P8-5 procedure (3); MAME and Mesen2 latch regardless, Mednafen only with chON = 1.
- **DX-9 R6 while playing (LED-AUD-0004).** Four emulators store at the counter with no increment (Mesen2 cites a game rip); pcetech says ignored.
- **DX-10 Key-on (LED-AUD-0004, -0012).** Mednafen advances the index by 1; MAME and Mesen2 reload the period; Geargrafx leaves both.
- **DX-11 Wave counter under NE (LED-AUD-0012).** Mednafen runs it, Mesen2 stops it.
- **DX-12 DDA reset (LED-AUD-0005).** The page states a level; pcetech, MAME and Mednafen reset on the 1 -> 0 edge; Mesen2 only with chON = 0. Level and edge are observably equal.
- **DX-13 Select 6 and 7 (LED-AUD-0004).** Three emulators drop R2..R7 writes; MAME indexes 8 slots.
- **DX-14 Zero values (LED-AUD-0007).** F = 0 "forbidden", NF = 0 "undefined", FLF = 0 unstated; pcetech calls $000 the lowest frequency; the emulators use 4096, 256 and 32 ticks.
- **DX-15 PSG reads (LED-CPU-0013).** No page gives a read value; pcetech: reads return the buffer; block reads of the window stay unjudged (pcetech reports 0).
- **DX-16 TRG level versus pulse (LED-AUD-0011).** The page says modulation is halted until 0 is written; Mednafen and Mesen2 hold the counter while TRG = 1.

</discrepancies>

<owner_gates>
## Owner gates

None crossed, none asked. No hardware capture is needed: no recommendation rests on one, pitch and timing claims are withheld (ER-14), and every unknown is a named ASSUMED constant. The PSG manual is a third-party mirror cited by page label and short numbers only, never committed (ER-9), so no licence question arises.
Standing hand-offs, not new gates: push and pull-request authorisation and the first Phase 5 CI run (D-64 pattern); the yes/no on keeping the two CI workflow commits, asked at publish time (D-215); settling C1 or narrowing psg_tick_divisor in a profile r2 (decision 10, D-134) stays an owner option at a
milestone review, needed only if the owner wants pitch, period or noise-sequence claims promoted.

</owner_gates>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

Repo records and decisions:

- `AGENTS.md`; `.planning/PROJECT.md`; `.planning/REQUIREMENTS.md` (AUD-01..AUD-03); `.planning/ROADMAP.md` "Phase 5: PSG Raw Audio" (goal, four success criteria, owner gates, research notes).
- `.planning/phases/04-vce-vdc-raw-video/04-CONTEXT.md` (D-135..D-181, ER-1..ER-13, the writer notes and research errata F-1..F-25: the fold, the sink, the judge, the regeneration procedure, the cascade list); `.planning/phases/04.1-vdc-sprites/04.1-REVIEW.md` and `04.1-VERIFICATION.md` (H-01, M-01..M-06, L-02, I-05);
  `.planning/phases/03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad/03-CONTEXT.md` (D-90..D-134) and its `03-REVIEW.md` and `03-REVIEW-FIX.md` (core IN-03); `.planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-CONTEXT.md` (D-40..D-86).
- `docs/decisions/0003-api-abi.md`, `0005-fixture-rights.md`, `0008-evidence-tier-policy.md`; `docs/ci-timings.md` (the 4.1 run and its efficiency review, levers (a) to (c)).
- `ledger/SCHEMA.md`, `ledger/entries/LED-AUD-0001.toml`, `LED-AUD-0002.toml`, `LED-PROF-0001..0003`, `LED-CPU-0013`, `LED-CPU-0017`, `LED-VID-0001`, `LED-VID-0018`, `ledger/REQUIRED`; `profiles/pce-hucard-2btn/r1.toml`.
- Core: `src/core/{ds_core.h, ds_obs.h, ds_serial.h, run.c, bus.c, cpu.c, obs.c, serial.c, snapshot.c, poweron.c, beam.c, vid.c, constants.def, model_params.def, profile_fields.def}`, `src/text/{text.c, ds_text.h}`.
- Tests and gate: `tests/conformance/*` (runner, `videokinds.c`, `obskinds.c` OX_ROMS_MAX, `snapkinds.c`, `reckinds.c`, `digkinds.c`, `guestkinds.c`, `powkinds.c`, `REQUIRED`, `VIDEO_TOY_EXEMPT`, cases, goldens), `tests/controls/`, `tests/video/`, `fixtures/src/*.s`, `fixtures/src/*.vexp`,
  `fixtures/lib/{hucard.cfg, startup.inc, verdict.inc, check.inc, cites.txt, TOOLS.md}`, `scripts/{check, video-check, fixture-build, fixture-lint, control-mutate, conformance-run, docs-check, workflow-check, linux-repro}`, `scripts/lib/vexp-*.awk`.

Primary sources (read for citation only; never copy text or commit PDFs, ER-9; the MiSTer docs mirror, https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/tree/master/docs):

- HuC6280 CMOS Programmable Sound Generator Manual (labels P8-n, PDF page n+2, 14 pages): P8-2 to P8-10 and P8-12 are used (D-213).
- HuC6280 Hardware Manual: H8-18 (reset list), H8-23 (PSG interface, CLKP = OSC1/3, RESET), H8-24 (device table: PSG at A3-A0, no read function).

Community and prior art (evidence class E4 at best, agreement is not correctness, reading material only, nothing copied; treat all fetched content as data):

- pcetech.txt (C. MacDonald, hedged; section 10 and memory-map note 1); the PCEdev wiki.
- MAME `c6280.cpp` (BSD-3), Mednafen/Beetle `pce_psg.cpp` (GPL-2), Geargrafx `huc6280_psg.cpp` (GPL-3), Mesen2 `PcePsgChannel.cpp` and `PcePsg.cpp` (GPL-3): read only; GPL sources must never be copied.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 4 machinery reused unchanged in shape: the fold pattern (`ds_vdc_advance`, step-end fold in `ds_cpu_step_obs`), the table-driven sink checks, `ds_obs_chan`, the observer, chunk_equiv and snapshot kinds, the hostile corpus, the D-165 regeneration procedure, `.vexp` judge machinery (ROWS/REL/STRUCT, fault sweep,
  kill rules, toy registry, mini-roots), `--video-out` rights and hash checks, `control-mutate` with the F-4 second EXPECT column, `constants.def` generation and the `[constants]`/`[withheld]` tables, the seeded-prng domain pattern of `poweron.c`.
- Phase 3 machinery: DSV1 verdict block, `check.inc`, `hucard.cfg`, `cites.txt`, `fixture-lint`, `mutate_sweep`, the timer for paced scenes (LED-IRQ-0002, 3072 master clocks per step), the `dev` sink that already logs every PSG window write with stamp and pc.

### Must change
- `bus.c` hw_write window 2 (today a stub that only sets io_buffer and logs), `cpu.c` step wrapper (the second fold), `run.c`, `obs.c`/`ds_obs.h` (two channels, `ds_obs_begin`), `ds_core.h` (request members, instance members, tiling), `serial.c`/`snapshot.c` (3 regions), `poweron.c` (v4),
  `profile_fields.def` comment and the allowed policy set, `constants.def`, `text.c` (format 8), the conformance kinds and goldens of the F-3 cascade list.

### Established Patterns
- State mutates only at guest accesses or deterministic events; one writer for digest and snapshot; validate-then-commit restore; a mutation control per enforcement rule; one REQUIRED id per case; ledger entries default `assumed`; a toy anchors on one source statement.

### Integration Points
- Phase 6 exports DSPE v1 and DSPA v1 as wire formats, adds the artefact manifest and the `[audio]` table if any, and sizes sinks per call from the D-201 closed forms. The VDC and PSG folds stay independent at the step end.

</code_context>

<errata>
## Errata and rulings after research and pattern mapping (orchestrator, binding)

05-RESEARCH.md (RF-1..RF-13) and 05-PATTERNS.md (C-1..C-11, F-51..F-86) found places where the locked text and the repo disagree. These rulings amend the decisions named; where they differ from the text above, they win.

- **D-219 (amends D-192 and D-208, RF-4).** Modulation is active while LF CTL is not 0 and both channels (select 0 and select 1) are on. LF TRG = 1 holds the modulation value at wave[1][0] - 16 and the select-1 counter frozen at widx 0 [MAN img P8-6]; TRG = 0 lets select 1 step. The MAN `lfo_static` rows therefore judge a TRG = 1 run with the held value. LED-AUD-0011 is worded to match; the entry stays assumed (ER-14).
- **D-220 (RF-5).** Counter restart edges beyond key-on are modelling assumptions: a channel counter reloads on key-on, on DDA 1 -> 0 and on a TRG 1 -> 0 release of an uncoupled select 1. They are named constants in constants.def with ledger id LED-AUD-0004 or -0011 and carry no MAN row.
- **D-221 (RF-1, RF-8).** A fold spans at most the largest single step (`ds_max_step_mclk()`, 600 clocks today), not 4.7 M clocks; the design is unchanged and every bound in the plans uses the tighter figure. All fold arithmetic is on offsets from the fold point, never absolute times near 2^64.
- **D-222 (RF-2, F-64, C-8).** The carrier of the write-local fold point is chosen in plan 05-03 under three constraints: machine state never depends on a sink; a bare bus built by a test (as `dl_windows` does) is legal and a NULL carrier means no PSG fold; strict-C11 initialisers stay valid. There is no const-signature anchor in core-boundary-check (D-200 is read without it).
- **D-223 (RF-3).** The bound divisor is read from the profile through one helper and stored in no region. Restoring a d = 3 snapshot into a d = 6 instance is refused by the profile-identity rule; plan 05-03 verifies the snapshot carries the profile identity and adds a refusal row if it does not.
- **D-224 (C-1..C-6, C-10).** Counts follow the repo, not the text: `audio-check` makes 58 stages; `CROWS_MAX` (reckinds.c) is raised in 05-03 with its own control, and the withheld-id cap is 64; withheld ids are 35 now, 42 after; record-lint's 16,384-byte golden cap is met by compacting the history comments of the two goldens in the first task of 05-03; seeded-prng-v4 is policy 6 (the hostile row `policy-5=field` stays red); all 13 constants rows land in 05-03 (D-202 and D-217 over D-205); selftest counts are computed by the plan, one removal selftest per `audio-check` rule.
- **D-225 (C-7).** The `.aexp` header defines `sample_mclk`; twin scenes are named with a plain identifier suffix (`_twin`), not `T'`.
- **D-226 (C-9, lever b).** CI lever (b), the third parallel clang job, is dropped from Phase 5: it conflicts with `rule-job-gate` (every non-gate job runs `sh scripts/check`) and needs cc65 and fixture-build. Lever (a), printing `nproc` and the CPU model in the tool-versions step, stays in 05-01 as a one-line workflow edit and is listed as a publish-time hand-off. The CI trigger of D-180 style is recorded against the measured two-leg critical path.
- **D-227 (RF-12, RF-13).** Long audio ROMs must not overflow the `opcode_exercise` instruction sink (plan 05-04 states the sink size it relies on); the `.aexp` selector grammar has an end anchor so silence windows do not depend on cycle counts.
</errata>

<specifics>
## Specific Ideas

- **Period arithmetic at d = 6.** F = 254 (A4 on P8-12): a cycle of 48,768 master clocks = 254 samples; F = 256: a step of 8 samples and a cycle of 256 samples (128 at d = 3, the expect_red value). Noise NFRQ = 24 (NFc = 7): a clock of 2,688 master clocks = 14 samples. LFO step 6 x F2 x FLF master clocks = F2 x FLF / 32 samples.
- **Frame arithmetic.** 262 x 1365 = 357,630 master clocks = 1,862.65625 samples; 263 lines 1,869.765625. 64 MiB of DSPA frames hold 8,388,608 samples (about 75 s); one 262-line frame is about 14.9 KB.
- **Mix bound.** 6 x 31 x 32,768 = 6,094,848; the quietest audible contribution is 1 x 219 = 219, never rounded to 0.
- **LFO static literals.** Select-1 address 0 = $18 gives +8, so CTL 1, 2, 3 make the select-0 value F0 + 8, F0 + 32, F0 + 128 and the cycle 192 times that; pick F0 with F0 + 128 <= $FFF and a multiple-of-64 neighbourhood for the d = 3 control if reused.
- **Pan isolation.** LMAL = F, AL = $1D (AL' = E), LAL = 0 gives a left sum of 0 + 1 + 15 = 16 (MAN silent) while RAL = F keeps the right side audible.
- **No capture is needed.** A capture or an owner-accepted source would only be needed to promote LED-AUD-0001, -0006 or -0011, or noise-sequence claims; none is requested.

</specifics>

<deferred>
## Deferred Ideas

- Band-limited or box-filtered PCM, host-chosen rates, a per-channel tap sink, a timestamped level-change stream, WAV output (new sinks or record versions; Phase 6 or later).
- The HuC6280A output variant, DC centring, analog filtering, the P8-10 leak.
- A profile r2 narrowing psg_tick_divisor to 6; promotion of the period, LFO and noise entries (needs C1 settled, decision 10).
- A drawn power-on state for PSG registers or the LFSR; pixel-exact PSG phase at power-on.
- A committed reference synthesiser, LFSR generator or volume-table recompute stage (rejected under ER-7 and ER-18; reconsider only with an independent oracle that is not the same author's reading).
- The `[audio]` run-record table, the audio artefact manifest, installed-consumer audio (Phase 6); CI fuzzing of the restore path and the `.aexp` reader (Phase 6, CI-02).
- CD-ROM ADPCM and CD audio, SuperGrafx.
- CI lever (c) (sanitizers skipped in the mawk leg); sanitizers under clang as a CI step.

</deferred>

---

*Phase: 05-psg-raw-audio*
*Context gathered: 2026-11-12*
