# Phase 5: PSG Raw Audio - Research

**Researched:** 2026-11-12
**Domain:** C11 freestanding PSG device on the existing master-clock bus; two caller-owned sinks (`psg` events, `pcm` frames); sh+awk gate, ca65/ld65 authored audio fixtures, an audio judge
**Confidence:** HIGH for the manual facts (13 page images read this session), the code map (files read this session) and the integer arithmetic (exact-integer recomputation this session); MEDIUM for cost figures; LOW nowhere load-bearing

Provenance tags used here: [MAN img] = a manual page read as an IMAGE this session (150 dpi, a second read and cross-check; the recorded ER-1 read stays plan 05-01's job); [CODE] = a file of this repository read this session, path and line given, values quoted verbatim; [CO] = measured or observed by a command this session;
[DERIVED] = integer arithmetic recomputed this session by a throwaway exact-integer script outside the repository (never committed, ER-18: only a committed literal is ever judged); [P] = a proposal of this research; [ASSUMED] = not verified, listed in the Assumptions Log.
Mapping to the agent vocabulary: [MAN img], [CODE], [CO] and [DERIVED] are VERIFIED by observation; [P] and [ASSUMED] need the planner's confirmation. No manual text or image is copied (ER-9): page labels, numbers and formulas only. No path outside the repository is named. The discuss advisors' notes were used as raw material and are not cited.

<user_constraints>
## User Constraints (from 05-CONTEXT.md)

The discuss step is done. Everything below is locked and binding exactly as written in `.planning/phases/05-psg-raw-audio/05-CONTEXT.md` (decision text on its lines 45-330). This file lists ids and titles only (the Phase 3 and 4 precedent); read the CONTEXT text before planning. Nothing here re-opens a decision; where this research
found a gap or an internal tension it is a numbered finding (RF-n) with a recommended ruling, never a silent change.

### Locked Decisions
- Evidence rules: ER-1..ER-13 stand; ER-14 criterion-4 hold; ER-15 disputed points are ASSUMED rows only; ER-16 converses are ASSUMED; ER-17 class ceilings and rule coverage are data; ER-18 an arithmetic lint is not an oracle; ER-19 audio records are definitions; ER-20 an audio hash is a change detector; ER-21 no E2 statement about PSG output; ER-22 promotion needs MAN-tagged guest rows.
- A. D-182 C1 at the documentary level; D-183 psg_tick_divisor is a literal divisor; D-184 period table; D-185 counter model; D-186 fold `ds_psg_advance`; D-187 sample grid.
- B. D-188 window, reads and write path; D-189 select and per-channel writes; D-190 wave counter, wave RAM, D/A latch; D-191 output gating; D-192 LFO; D-193 noise.
- C. D-194 level, attenuation, silence and mix; D-195 integer proof obligations. D. D-196 DSPE v1 record; D-197 DSPA v1 frame and the `pcm` sink; D-198 sink ABI and IN-03; D-199 streams, digests, runner output; D-200 non-interference and the proof set; D-201 Phase 6 contract.
- E. D-202 regions and the one bump; D-203 restore validation; D-204 power-on and seeded-prng-v4; D-205 constants, no model rows. F. D-206 oracle policy; D-207 the judge; D-208 isolation and row kinds; D-209 fault sweep, vacuity, hash roles; D-210 toy mutants (audio cap 7); D-211 criterion-3 controls; D-212 fixtures and C-level cases; D-213 ledger.
- G. D-214 gate cost and budgets; D-215 CI levers; D-216 process rules R1..R12; D-217 plans and the cut line; D-218 claims and withheld words.

### Claude's Discretion (verbatim)
File and symbol names (`psg.c`, `ds_psg.h`, region member names beyond those fixed here), field order beyond the byte layouts fixed here, the pad member of PSG0, the exact wording of ledger statements beyond the tags and facts above, how tasks are cut inside the 10 plans (at most 3 each), the
grouping of conformance case files, the asymmetric wave tables and the F values (multiples of 64), the per-scene frame counts within D-214, how the `psg_vol.def` ledger id reaches `[withheld]`, and the TSV reader versus the awk subset parser for `.aexp`.

### Deferred Ideas (OUT OF SCOPE, verbatim)
- Band-limited or box-filtered PCM, host-chosen rates, a per-channel tap sink, a timestamped level-change stream, WAV output (new sinks or record versions; Phase 6 or later).
- The HuC6280A output variant, DC centring, analog filtering, the P8-10 leak.
- A profile r2 narrowing psg_tick_divisor to 6; promotion of the period, LFO and noise entries (needs C1 settled, decision 10).
- A drawn power-on state for PSG registers or the LFSR; pixel-exact PSG phase at power-on.
- A committed reference synthesiser, LFSR generator or volume-table recompute stage (rejected under ER-7 and ER-18; reconsider only with an independent oracle that is not the same author's reading).
- The `[audio]` run-record table, the audio artefact manifest, installed-consumer audio (Phase 6); CI fuzzing of the restore path and the `.aexp` reader (Phase 6, CI-02).
- CD-ROM ADPCM and CD audio, SuperGrafx.
- CI lever (c) (sanitizers skipped in the mawk leg); sanitizers under clang as a CI step.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description (REQUIREMENTS.md) | Research Support |
|----|-------------------------------|------------------|
| AUD-01 | 6 channels with waveform RAM, frequency, volume/balance, noise on the two noise-capable channels, DDA, LFO, verified by fixtures that assert register-to-event behaviour (E1) | Register table (section 1, P8-2..P8-6, P8-9, H8-24 image-read); period, noise and LFO formulas (section 2); RF-4 and RF-5 close two gaps in the counter rules; EV row examples (section 6) |
| AUD-02 | Timestamped PSG register-event log and PCM at a defined, recorded rate and format, before host resampling | Sample-grid arithmetic and per-frame counts (section 3); DSPE and DSPA byte layouts, ABI and runner options (section 5); RF-2 (step-local fold point), RF-8 (overflow-free offsets), RF-18 (ds_obs_begin callers) |
| AUD-03 | Semantic checks (period, waveform content, silence), never "buffer is nonzero"; noise deterministic | Worked examples of every check kind with hand-derived literals (section 6); judge grammar and kill matrix (section 7); LFSR derivation and the boot closed form (RF-6, RF-11) |
</phase_requirements>

## Summary

The locked design is implementable with no new dependency and no new tool. The manual pages behind every MAN class were re-read as images and agree with the CONTEXT facts: fOUT = fmaster / (2 x 32 x F) with fmaster 7.16 MHz supplied by the CPU (P8-9), CLKP = OSC1 / 3 (H8-23), so a wave step is 6F master clocks and a cycle 192F;
the P8-12 table checks numerically (A4 F = 254 at 440.45 Hz, C1 F = 3421 at 32.70 Hz) [MAN img, DERIVED]. All integer literals the plans need are computed below: the 30-entry Q15 volume table with an exact integer rounding rule, the first 20 LFSR outputs with the states, the per-frame sample counts for 262 and 263 lines, and the crossing indices of the tracer scene.

The code was read and probed this session. `sizeof(struct ds_instance)` is 1,141,096 bytes, `vid_pad` is the last member at 1,141,090, `struct ds_run_request` is 56 bytes and `ds_max_step_mclk()` returns 600 [CO probe linked against the built core]. A block transfer runs one byte per step (block_byte, cpu.c:620-653), so every PSG fold
spans at most 600 master clocks, not the 4.7 M of the D-186 loop-bound sentence (RF-1, a tightening). The PSG window today only sets io_buffer (`case HW_PSG: inst->io_buffer = value; break;`, bus.c:186-188).

Four findings need a ruling before 05-03 or 05-04: the step-local fold point of D-186 has no carrier in the code (RF-2, recommended: a pointer member of `struct ds_bus`); the bound divisor is in no region (RF-3); D-192 defines "active" with TRG = 0 while D-208 judges held modulation under TRG = 1 (RF-4, an internal tension of the CONTEXT);
and the frozen-to-running transitions other than key-on are unspecified (RF-5). None changes a locked decision silently; each carries a recommended ruling the orchestrator can adopt as an erratum.

**Primary recommendation:** keep the 10 plans and 9 waves. Settle RF-2..RF-5 as errata in 05-01 text before any C. Implement the fold in offsets from the first unfolded instant (RF-8) so no absolute time is ever added near 2^64. Use the tracer table and the literals of section 6 verbatim as the 05-04 hand literals.

## Findings: gaps, tensions and corrections (numbered)

Severity: B = the plan named cannot be done as written until ruled; A = advisory, a trap or a correction. A finding marked CONTRADICTS CONTEXT is reported, not applied.

- **RF-1 (A; D-186, D-195; CONTRADICTS CONTEXT text, tightening only).** D-186 says the folded span "is at most one instruction, the longest a block transfer of about 4.7 M master clocks". In this code base a block transfer is a prologue step, one step per byte and an epilogue step (cpu.c:530-534; `block_byte` reads once and writes once,
  cpu.c:620-653), and `ds_max_step_mclk()` = 600 [CO probe; formula run.c:38-55]. Every fold call between steps or at a write spans <= 600 master clocks. Every loop bound below uses 600. The design is unchanged.
- **RF-2 (B; D-186).** The "step-local" fold point has no carrier. `struct ds_bus` is a positional local of `interrupt_entry` (cpu.c:514), `block_step` (cpu.c:679) and `cpu_step_body` (cpu.c:694); `ds_cpu_step_obs` sees only `start` and `total` (cpu.c:1099-1105). The PSG stores distances relative to the fold point and PCM emission needs the first unfolded
  instant, so the point a write fold reached must flow to the step-end fold. Recommended ruling: add `uint64_t *psg_at` as the LAST member of `struct ds_bus` (ds_core.h:428-437); `ds_cpu_step_obs` owns `uint64_t psg_at = start` (the first unfolded instant), passes `&psg_at` down, and folds `[psg_at, start + total - 1]` after the VDC fold.
  hw_write HW_PSG folds `[*bus->psg_at, stamp]` and stores `stamp + 1`. All three positional initialisers must list the new member: `-Wextra` enables `-Wmissing-field-initializers` under gcc and clang, and `-Werror` turns an omitted trailing member into a build failure. NULL bus (reset reads, tests): fold to `stamp - 1` only, then apply the write,
  because folding through `machine_time` would break the between-steps invariant (stored distances are relative to `machine_time - 1`); document it as the test-only path. A step holds at most one PSG-window write today (one data write per instruction or block byte; stack and zero page live in window 0), but the design must not rely on it.
- **RF-3 (B; D-183, D-202).** The bound divisor d is in no region: D-202 fixes PSG0 at 42 bytes with no divisor member, and the fold needs d. Recommended: no new member; a const helper `ds_psg_divisor(inst)` maps `inst->profile.field[DS_FIELD_psg_tick_divisor].value` ("3" or "6") to 3 or 6, the `ds_vdc_frame_lines` shape (beam.c:297-300) without a cached copy, read once
  per fold. Consequence: a snapshot does not identify the binding; D-203's grid rule uses the restoring instance's d, so a d = 3 snapshot whose counters all sit on the 6-grid restores into a d = 6 instance. Open question 1 asks whether that is acceptable or a cached byte (refused as `profile`, the frame_lines precedent at snapshot.c:706) should take the PSG0 pad position.
- **RF-4 (B; D-192, D-208, LED-AUD-0011; CONTRADICTS CONTEXT internally).** D-192 defines the LFO as active only with TRG = 0 and gives F0' "while active", yet says the modulation value is held while TRG = 1, and D-208 `lfo_static` judges select-0 cycles of 192(F0 + 8), 192(F0 + 32), 192(F0 + 128) under TRG = 1. Read literally, a model with TRG = 1 leaves select 0
  unmodulated and the MAN `lfo_static` rows go red on a correct implementation. P8-6 [MAN img]: writing 1 halts modulation with the data from address 0 of ch2 and resets its wave counter; P8-9 [MAN img]: the LFO is effective when LF CTL is 1, 2 or 3 (no TRG term). Recommended ruling: two terms. Modulation applies when (R9 & 3) != 0 and select 0 and 1 both have chON = 1;
  F0' uses wave[1][widx1]. Coupled stepping (select 1 steps every d x F1e x FLFe) needs modulation AND TRG = 0. TRG = 1 freezes select 1 at widx 0, so F0' uses wave[1][0]. Reword LED-AUD-0011 in 05-01 ("modulation applies when CTL != 0 and select 0 and 1 are on; select 1 steps at the LFO rate when also TRG = 0"). D-203's freeze invariant is unchanged.
- **RF-5 (A; D-185, D-192).** Unspecified transitions: (a) DDA 1 -> 0 while chON = 1; (b) TRG 1 -> 0 when stepping is not coupled (D-192 gives d x F1e x FLFe unconditionally); (c) a change of coupling (CTL, select-0 chON) while select 1 runs; (d) select 1 keyed on while TRG = 1. Recommended (ASSUMED, LED-AUD-0012): every frozen -> running transition at stamp w
  sets next = (w - w mod d) + the current period (coupled if coupled after the write, else d x F1e); a coupling change on a running counter applies at its next reload (the R2/R3 rule of D-185); (d) stays frozen until TRG = 0. The D-203 invariant "rem = 0 exactly when frozen" holds under this rule.
- **RF-6 (A; D-193, D-202).** The noise output needs no stored bit: the bit shifted out equals the parity of bits 0, 10, 11, 16 and 17 of the post-shift LFSR (mask $30C01), because bit 0 is a tap. Proof: post = (pre >> 1) | (b << 17) with b = pre0 ^ pre1 ^ pre11 ^ pre12 ^ pre17; post bits 0..16 are pre bits 1..17 and post17 = b,
  so pre0 = post17 ^ post0 ^ post10 ^ post11 ^ post16. Checked over 200,000 steps from seed 1 [DERIVED]. PSGT (40 bytes) is therefore complete.
- **RF-7 (A; D-187, descriptive aside).** "point samples alias content above 55.9 kHz (F < 32)": the fundamental (111,860.8 / F Hz) passes Nyquist only at F = 1; the 32-step staircase rate (3,579,545.45 / F Hz) passes it for F < 64. No rule depends on the parenthesis.
- **RF-8 (A; D-186, D-195).** Compute every in-fold time as an offset from E (the first unfolded instant), never as an absolute next-event time. A restored `machine_time` may sit just below `UINT64_MAX - 600` (ds_run refuses only above that), where `machine_time + 6,291,456` wraps. With offsets every intermediate is <= 600 + 6,291,456 < 2^23, so D-195's
  "stamp <= UINT64_MAX - period" guard is met by construction; keep a `_Static_assert` on the offset bound.
- **RF-9 (A; D-200).** Chunk tokens 191 and 193 do not exist: `cs_table` holds `"1"`, `"7"`, `"168"`, `"3072"`, `"frame"`, `"tick-1"`, `"tick+1"`, `"1364"`, `"1366"`, `"vframe"` [CODE tests/conformance/chunkkinds.c:47-52]. Add 191 and 193 (the F-20 precedent); "one frame" is `vframe` (357,630). The chunk_equiv expect grammar needs an audio compare token for the concatenated streams.
- **RF-10 (A; D-202, layout).** Place PSGT (10 x u32, 40 bytes) at the 8-aligned end of `vid_pad` (offset 1,141,096 [CO]), then `psg_freq[6]` (12), the 30 single bytes, `psg_wave[192]`, then `psg_pad[6]`: 40 + 12 + 30 + 192 + 6 = 280, no implicit padding. Instance 1,141,096 -> 1,141,376; snapshot 92,255 -> 92,553 (15 + 4 + 24 x 8 + 92,068 + 274).
  The assert `"vid_pad is not the last member of the instance"` (ds_core.h:300) moves to `psg_pad`; `psg_pad` joins `sk_is_identity`; `DS_REGIONS_MAX 24u` (digkinds.c:147) is exactly enough; `DK_CAP 131072u` (digkinds.c:40) is enough; `SK_VERSION 8u`, `SK_FRAMES 21u`, `SK_EXPECTED_SIZE 92255u` (snapkinds.c:32-34) become 9, 24, 92553.
- **RF-11 (A; D-202 regeneration).** Every digest changes at the bump, also for ROMs that never write the PSG, because the noise counters run from boot. Independent closed form when no PSG write happened (R7 = 0, so NFc = 31 and the noise period at d = 6 is 11,904): events n = 0 for M = 0, else floor((M - 1) / 11904) + 1; both LFSRs = step^n(1);
  both nrem = n x 11904 - (M - 1), and 1 at M = 0; every rem = 0; PSG0 zero; PSGW zero (v4: the drawn bytes). Examples [DERIVED]: M = 1 -> n = 1, LFSR $20000, nrem 11,904; M = 11,905 -> n = 2, LFSR $30000; M = 357,630 -> n = 31, LFSR $107EF, nrem 11,395. `mmu_mpr` writes `$6805` (R5 of select 0, stored in psg_bal[0]) and the random programs may reach the window.
- **RF-12 (A; D-212, the F-19 class).** `opcode_exercise` runs every guest ROM with `OX_SINK_RECORDS 1048576u` (obskinds.c:304). A tight wait loop at high speed is about 29,800 instructions per 262-line frame (4 cycles x 3 master clocks), so a ROM may wait about 35 frames at high speed (about 140 at low speed) before records are lost (harness_failure). Budget it per fixture.
- **RF-13 (A; D-207 grammar).** A silence window that ends at a later event (for example the key-on) cannot be written as `from=+mclk;to=+mclk` of one anchor without a cycle count read from a run (R7). Add an end anchor (`to=ev:<selector>` with an optional offset) to the selector grammar.
- **RF-14 (A; D-208, D-211).** `cycle_order` must segment plateaus by run-collapsing the samples between two recurrences, not by a step length computed from F: D-211 requires `cycle_order` green under d = 3, where a step is F/64 samples instead of F/32. This needs cyclically adjacent words to differ; the recommended table has 32 distinct words.
- **RF-15 (A; D-204).** "Noise distances start at 1 (the event at time 0)" relies on `DS_CONST(reset_cost_mclk, 0, "LED-CPU-0004")` (constants.def:43) and `inst->machine_time = (uint64_t)DS_CONST_reset_cost_mclk` (boot.c:364). Add `_Static_assert(DS_CONST_reset_cost_mclk == 0, ...)` beside the PSG boot init, or derive nrem from the boot time with grid alignment.
- **RF-16 (A; D-198, IN-03).** `ranges_overlap` is static in obs.c (obs.c:33) and boot.c; the IN-03 check in run.c needs its own copy or one shared internal helper; no libc call is involved, so the core-boundary allow list is unchanged.
- **RF-17 (A; D-198).** `ds_run` creates the log only when `request->insn != NULL || request->dev != NULL || request->vid != NULL` (run.c:106); add `psg` and `pcm`. `ds_obs_begin` has seven callers: run.c:108, vdckinds.c:359, obskinds.c:1633, 1725, 2237, 2272, 2281 [CO grep]; the request grows from 56 to 72 bytes on LP64 (the F-16 old-size control is 56).

## Architectural Responsibility Map

This is a library plus a gate, so the tiers are the repository layers.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| PSG registers, wave RAM, counters, LFSRs, fold, synthesis | Core `src/core` (new `psg.c`, `ds_psg.h`; names discretionary) | - | Deterministic machine behaviour lives in the core: no clock, file, allocator, float or mutable global |
| Sample value at one instant | Core, a const function `ds_psg_sample(const struct ds_instance *, ...)` | - | Non-interference by signature (D-200) |
| Sink channels `psg` and `pcm`, run request, IN-03 | Core `obs.c`, `ds_obs.h`, `run.c` | Runner kinds | Same mechanism as insn, dev, vid; caller owns memory |
| Write decode and the bus hook | Core `bus.c` HW_PSG write case | - | One choke point; reads unchanged (D-188) |
| Constants, `psg_vol.def`, format 8 text | `src/core/constants.def`, `src/text` | `record-lint` | The printer prints, nothing parses (D-40) |
| Streams DSPE and DSPA, digests, `--audio-out` | Runner `tests/conformance` (new `audiokinds.c`) | `scripts/conformance-run` | Host concerns; the core only fills sinks (D-199) |
| Literal data of the judge | `fixtures/src/NAME.aexp` via `fixture-build` (`NAME.aexp.tsv`) | `scripts/audio-check` | Hand-derived literals; one validated parser; the runner reads TSV only |
| Fixtures and DSV1 guest schedule rows | `fixtures/src`, `fixtures/lib` (ca65, 0BSD), a new `audio.inc` | `fixture-lint`, `fixture-build` | Authored, built in the gate, never committed as ROMs |
| Claims and tiers | `ledger/entries`, `ledger/REQUIRED` | `inventory-check`, `docs-check`, README | ER-2, ER-14, ER-22 |

## Manual Page Image Verification (this session)

Method: both PDFs are the copies whose size and SHA-256 are recorded: PSG manual 934,252 bytes, `2f463fd388cd...6c04b`, 14 pages (equal to D-213); HuC6280 hardware manual 1,196,754 bytes, `40a32e5dec7f...74fcb` (equal to `fixtures/lib/TOOLS.md:51`) [CO shasum]. Rendered with `pdftoppm -r 150` into a session scratch directory outside the repository.
Read as images: P8-2 to P8-10 (PDF 4 to 12), P8-12 (PDF 14), H8-18 (PDF 21), H8-23 (PDF 26), H8-24 (PDF 27). P8-11 (PDF 13) was rendered and not read: no decision uses it. Label P8-n is PDF page n+2; H8-n is PDF page n+3. Verifier: gsd-phase-researcher, 2026-11-12.

| Page | Numeric facts seen on the image | Backs |
|------|----------------------------------|-------|
| P8-2 | Channels ch1-ch6 have waveform generators, noise on ch5 and ch6, LFO uses ch2 to modulate ch1; register map R0..R9 with bit fields; A3-A0 0..9 select R0..R9, A..F shaded; R0, R1, R8, R9 one per PSG (R0 not effective); R7 row shaded for ch1-ch4 | -0002, -0003 |
| P8-3 | R0 bits 2-0 ch SELECT, 7-3 shaded; R0 values 0..5 = ch1..ch6, 6 and 7 shaded; example R2 of ch3 = R0 2, A 2; R1 LMAL 7-4 RMAL 3-0, F max, about 3 dB per step; R2 8 bits; R3 bits 3-0; R4 chON 7, DDA 6, 5 shaded, AL 4-0 | -0002, -0003 |
| P8-4 | Mode table (chON, DDA): 00 write data, increment after a write; 01 reset counter; 10 mixing, increment at the frequency timing; 11 reset, each write to the D/A; AL $1F max, about 1.5 dB per step; R5 LAL 7-4 RAL 3-0, 3 dB per step; R6 bits 4-0, 32 addresses 0..$1F | -0003, -0005 |
| P8-5 | D/A staircase over data 0..31 (monotone, drawn linear); procedures (1) normal writing, (2) chON 0 DDA 1 then 0 sets the counter to 0, 32 words at 0..$1F, (3) DDA: write $00 or AL 0 first, then chON 1 DDA 1, held until the next write, R6 data not changed | -0005 |
| P8-6 | R7 NE bit 7 (noise on, sound off), NOISE FRQ 4-0; R8 with ch2 R2/R3 one divide-by-N counter; R9 LF TRG bit 7: writing 1 resets the LFO, halts modulation with ch2 address-0 data, resets the wave counter; writing 0 starts it; LF CTL 1-0; MSB of R6 is XSGN; NOTE: active only when ch1 and ch2 enabled | -0011 |
| P8-7 | Modulation value = waveform value - $10: $1F +F, $18 +8, $10 0, $0F -1, $08 -8, $00 -10 | -0011 |
| P8-8 | LF CTL 0 disabled, 1 shift 0, 2 shift left 2, 3 shift left 4; the figure adds the low-order 4 bits | -0011 |
| P8-9 | fOUT = fmaster / (2 x 32 x F), fmaster 7.16 MHz supplied by the CPU, F = R3[3:0]:R2, F = 0 forbidden; fLFO = fmaster / (2 x 32 x F2 x FLF), effective for LF CTL 1, 2, 3, 20-bit combined counter, time to address 32 words; fNCL = fmaster / (2 x 32 x 2 x NF), NF the complement of the 5 bits, NF = 0 undefined; R7 for ch5 and ch6 only | -0001, -0006, -0011 |
| P8-10 | R1 and R5 3 dB steps, R4 1.5 dB; range about 45 dB, none output 45 dB below maximum, a little leak; silent if R1, R4 or R5 is 0 or the sum (F-MAL) + (F-AL') + (F-LAL) "exceeds F"; example R1 $C8, AL' E, R5 $F8: left 3 + 1 + 0 = 4 (12 dB), right 7 + 1 + 7 = 15 (45 dB, no sound) | -0008 |
| P8-12 | Scale table at fmaster 7.16 MHz: C1 F 3,421 (R3 D, R2 5D) 32.70 Hz; A4 F 254 (R3 0, R2 FE) 440.45 Hz; C8 F 27 4,143.5 Hz; A2 F 1,017 110.00 Hz | -0001, -0006 |
| H8-18 | Reset state list: I set, D reset, MPR7 $00, timer stopped, interrupt disable reset, TIQ reset, low speed, port O high; no PSG entry | -0003 (power-on is a project choice) |
| H8-23 | Signals CPU to PSG: D0-D7, RD, WR, CEP, A0-A3, CLKP f = OSC1 / 3, RESET, SX | -0001, -0006 |
| H8-24 | PSG at physical $1FE800-$1FEBFF (logical $0800-$080F with MPR0 $FF), decode A3-A0, no read or bit function listed | -0003, LED-CPU-0013 |

Arithmetic check of P8-12 [DERIVED]: 7,160,000 / (64 x 254) = 440.4528 and 7,160,000 / (64 x 3421) = 32.7024, the printed values; $D5D = 3,421 and $0FE = 254. The OCR errata of the CONTEXT writer notes (P8 prefix, P8-7 row 01000 = -8, P8-12 C1 R2 = 5D) are confirmed on the images.

## 1. Register-by-register table (as locked; masks in one table `ds_psg_reg_mask`)

Window: physical $1FE800-$1FEBFF; reg = offset & $0F for every access (D-188). "Per ch" = the channel `psg_select` names at the write stamp (0..5; 6 and 7 drop R2..R7). Classes: MAN = page-stated [MAN img]; ASSUMED with ledger id; DEF = project definition.

| reg | Name | Scope | Bits (D7..D0) | Stored mask | Reset | Write rule | Class |
|-----|------|-------|---------------|-------------|-------|------------|-------|
| 0 | R0 ch SELECT | global | 2-0 select | $07 | 0 | psg_select = v & 7 | MAN P8-2, P8-3 (6/7 name no channel: MAN shaded; the drop rule ASSUMED LED-AUD-0004) |
| 1 | R1 LMAL/RMAL | global, R0 ignored | 7-4 LMAL, 3-0 RMAL | $FF | 0 | stored; acts at the stamp | MAN P8-2, P8-3, P8-10 |
| 2 | R2 FRQ LOW | per ch | 7-0 F7..F0 | $FF | 0 | F low byte; applies at the next reload | layout MAN P8-3, P8-9; reload timing ASSUMED LED-AUD-0012 |
| 3 | R3 FRQ HIGH | per ch | 3-0 F11..F8 | $0F | 0 | F high nibble; next reload | layout MAN; F = 0 as 4096 ASSUMED LED-AUD-0007 |
| 4 | R4 chON/DDA/AL | per ch | 7 chON, 6 DDA, 4-0 AL | $DF | 0 | DDA = 1 sets widx = 0 (level); chON 0 -> 1 with DDA 0 is key-on (RF-5) | modes MAN P8-4, P8-5; key-on phase ASSUMED LED-AUD-0004, -0012 |
| 5 | R5 LAL/RAL | per ch | 7-4 LAL, 3-0 RAL | $FF | 0 | stored; acts at the stamp | MAN P8-4, P8-10 |
| 6 | R6 WAVE DATA | per ch | 4-0 data | $1F (word or latch) | wave RAM zero (v4: drawn) | DDA 1: dda = v; chON 0 DDA 0: wave[widx] = v, widx + 1; chON 1 DDA 0: wave[widx] = v, no increment | MAN P8-4, P8-5; latch at chON 0 inferred, write-while-on ASSUMED LED-AUD-0004 |
| 7 | R7 NE/NOISE FRQ | select 4, 5 only | 7 NE, 4-0 NFRQ | $9F | 0 | dropped at select 0..3, 6, 7; NFc = 31 - (v & $1F); next noise reload | MAN P8-6, P8-9; NFc 0 as 32 ticks ASSUMED LED-AUD-0007 |
| 8 | R8 LFO FRQ | global | 7-0 FLF | $FF | 0 | next select-1 reload | MAN P8-6, P8-9; FLF 0 as 256 ASSUMED |
| 9 | R9 LF TRG/CTL | global | 7 TRG, 1-0 CTL | $83 | 0 | TRG 1: widx[1] = 0 at the write, select 1 frozen; later R6 writes advance widx[1] (D-231) (RF-4, RF-5) | MAN P8-6, P8-8; hold-while-1 ASSUMED LED-AUD-0011 |
| A-F | none | - | - | - | - | logged with NO_TARGET, io_buffer set, nothing stored | MAN P8-2 (shaded) |

Reads: unchanged (io_buffer, not folded, not logged; D-188, LED-CPU-0013). Mirror: offset & $0F over the whole $400 window (H8-24 decodes A3-A0; the mirror is the D-146-style inference). Power-on: every register, select, latch, widx and rem 0 under every policy; nrem 1; LFSR `psg_lfsr_reset` 1 (D-204).
Writes that are never dropped: R0, R1, R8, R9 at any select. Writes dropped at select 6 or 7: R2..R6 (and R7). Writes dropped at select 0..3: R7.

## 2. Formulas in master clocks with worked integer examples

All periods are master clocks for the bound divisor d (D-183, D-184); the d = 6 value equals P8-9 with fmaster = OSC1 / 3 (H8-23). Fe = F, or 4096 when F = 0 (ASSUMED); FLFe = FLF, or 256 when FLF = 0 (ASSUMED); NFc = 31 - (R7 & $1F).

| Quantity | Formula | d = 6 examples [DERIVED] |
|----------|---------|---------------------------|
| Channel step (one word) | d x Fe | F 1: 6; F 254: 1,524; F 256: 1,536; F 3,421: 20,526; F 4,095: 24,570; F 0 (ASSUMED 4096): 24,576 |
| Channel cycle (32 words) | 32 x d x Fe | F 254: 48,768 = 254 samples; F 256: 49,152 = 256 samples; F 3,421: 656,832 = 3,421 samples (d = 3 halves each) |
| Noise clock (one LFSR step) | d x 64 x NFc (NFc >= 1); d x 32 when NFc = 0 (ASSUMED) | R7 low $00: NFc 31, 11,904 = 62 samples; $11: NFc 14, 5,376 = 28 samples; $18: NFc 7, 2,688 = 14 samples; $1F: 192 = 1 sample (ASSUMED) |
| Coupled select-1 step | d x F1e x FLFe | F1 128, FLF 150: 115,200 = 600 samples; maximum 6 x 4096 x 256 = 6,291,456 |
| Modulated select-0 value | F0' = (F0 + $1000 + (w << s) - (16 << s)) & $FFF, 0 read as 4096; s = 2 x ((R9 & 3) - 1) | F0 512, w $18: s 0 -> 520; s 2 -> 544; s 4 -> 640; w $08, s 0 -> 504; F0 16, w 0, s 0 -> 0 -> 4096 (ASSUMED) |

Integer range of the LFO sum before the mask [DERIVED]: minimum 0 + 4096 + 0 - 256 = 3,840, maximum 4095 + 4096 + 496 - 256 = 8,431, so the unsigned expression never wraps (s <= 4, w <= 31).

### Attenuation, silence and the mix (D-194)

Per side X: att = 2 x (15 - MAL_X) + (31 - AL) + 2 x (15 - LAL_X), range 0..91. With S = the P8-10 sum (15 - MAL) + (15 - AL') + (15 - LAL) and AL = 2 x AL' + b: att = 2S + 1 - b. Hence att >= 30 <=> S >= 15 and att <= 29 <=> S <= 14, for b = 0 and b = 1 [DERIVED proof]. R1 = $00 or R5 = $00 gives S >= 15 under any AL; R4 = $00 has chON = 0.
Contribution = level x VOL[att] for att <= 29, else 0; L and R are plain unsigned sums over the six channels. Examples: the P8-10 left side (R1 $C8, AL $1C or $1D, R5 $F8): S 4, att 9 or 8, VOL 6,925 or 8,231; the right side S 15, att 31 or 30: silent in the model (ASSUMED row, disputed).
Pan isolation of the CONTEXT specifics (LMAL F, AL $1D, LAL 0, RAL F): left S 16 (MAN silent), right att 2, VOL 23,198.

### The Q15 volume table and its integer rounding rule

VOL[a] = round-half-up(32768 x 10^(-3a/40)) for a = 0..29 (1.5 dB per unit). Exact integer characterisation used to compute it: VOL[a] is the unique integer v with (2v - 1)^40 x 10^(3a) <= 2^640 < (2v + 1)^40 x 10^(3a) (from v - 1/2 <= 2^15 x 10^(-3a/40) < v + 1/2, doubled and raised to the 40th power).
No tie is possible (for a >= 1 the left side is a multiple of 5, for a = 0 it is odd). Big-integer evaluation this session gives [DERIVED], equal to the D-194 list:

| a | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|----|----|----|----|----|
| VOL | 32768 | 27571 | 23198 | 19519 | 16423 | 13818 | 11627 | 9783 | 8231 | 6925 | 5827 | 4903 | 4125 | 3471 | 2920 |

| a | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 |
|---|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
| VOL | 2457 | 2068 | 1740 | 1464 | 1232 | 1036 | 872 | 734 | 617 | 519 | 437 | 368 | 309 | 260 | 219 |

Strictly decreasing; VOL[0] = 32768, VOL[29] = 219 (the `psg_vol_table` endpoints). The closest any fractional part comes to one half is 0.0018, so a double-precision `round` gives the same table; `psg_vol.def` carries the integer rule in its header so a reviewer can re-check one row by hand. Mix bound: 6 x 31 x 32768 = 6,094,848 < 2^23.

### The noise LFSR (D-193, ASSUMED LED-AUD-0010)

Step: b = (s ^ (s >> 1) ^ (s >> 11) ^ (s >> 12) ^ (s >> 17)) & 1; s = (s >> 1) | (b << 17); s stays in 1..$3FFFF. Taps 0, 1, 11, 12, 17: 1 + 2 + 2048 + 4096 + 131072 = 137,219 = `psg_lfsr_taps`. The output of a step is the bit shifted out (old bit 0): level 31 for 1, 0 for 0; between steps the level is that of the last step (RF-6 gives it from the stored state).
Period from seed 1: 131,071 = 2^17 - 1 [DERIVED]. First 20 steps from state 1 (hand-checkable):

| step k | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|--------|---|---|---|---|---|---|---|---|---|----|
| state before | $00001 | $20000 | $30000 | $38000 | $3C000 | $3E000 | $3F000 | $1F800 | $0FC00 | $07E00 |
| output bit | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

| step k | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
|--------|----|----|----|----|----|----|----|----|----|----|
| state before | $03F00 | $01F80 | $00FC0 | $207E0 | $303F0 | $381F8 | $3C0FC | $3E07E | $1F03F | $2F81F |
| output bit | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |

Output string of steps 1..20: `10000000000000000011`; state after step 20: $37C0F. Steps 1..40: `1000000000000000001111110000001111101111`. Hand check of step 13 -> 14: $00FC0 has bits 6..11 set; b = bit0 0 ^ bit1 0 ^ bit11 1 ^ bit12 0 ^ bit17 0 = 1, so $00FC0 >> 1 = $007E0, plus $20000 = $207E0.

## 3. The sample grid (D-187, D-197)

- Sample s is the point sample at instant s x 192 master clocks, s = 0, 1, ...; sample s sees every event with time <= s x 192 and no CPU write stamped s x 192 (D-185 tie order). 192 = 32 x 6 is a multiple of 3 and 6, so every instant lies on either tick grid.
- After a run to machine time M > 0 the samples are s = 0 .. floor((M - 1) / 192); count = ceil(M / 192). Examples [DERIVED]: M 1, 191, 192 -> 1 sample; M 193 -> 2; M 357,630 -> 1,863 (last instant 357,504); M 1,000,000 -> 5,209.
- One call from T0 to T1 (machine times at its start and end) emits first = ceil(T0 / 192) and seq = ceil(T1 / 192) - ceil(T0 / 192); the next call's first = first + seq. Per-call capacity (D-201): ceil((until - T0 + 600) / 192) + 1; one 262-line frame: ceil(358,230 / 192) + 1 = 1,867.
- Per frame k of L = lines x 1365 master clocks: count(k) = ceil((k + 1) L / 192) - ceil(k L / 192); frame k starts at sample ceil(k L / 192).

| Lines | L | L / 192 | Period | Counts [DERIVED] |
|-------|---|---------|--------|------------------|
| 262 | 357,630 | 1,862 + 21/32 | 32 frames = 59,605 samples (21 x 1,863 + 11 x 1,862) | frames 0..31: 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1863 1862 1863 1862 |
| 263 | 358,995 | 1,869 + 49/64 | 64 frames = 119,665 samples (49 x 1,870 + 15 x 1,869) | frames 0..7: 1870 1870 1870 1870 1869 1870 1870 1870; frame starts 0, 1870, 3740, 5610, 7480, 9349, 11219, 13089 |

262-line frame starts: 0, 1863, 3726, 5588, 7451, 9314, 11176, 13039, 14902. Rate label: OSC1 / 192 = (236,250,000 / 11) / 192 = 4,921,875 / 44 Hz, about 111,860.795 Hz (OSC1 is the NTSC 6 fsc figure, a label and never core arithmetic). Class of the count rows: ASSUMED:LED-VID-0001 plus the definition LED-AUD-0014.

## 4. The fold `ds_psg_advance` (D-185, D-186, D-195, RF-1, RF-2, RF-4, RF-5, RF-8)

Representation (locked): for counter i the stored r_i = next_i - (P), P the last folded instant; between steps P = machine_time - 1, so with E = P + 1 (the first unfolded instant) next_i = E + r_i - 1. r = 0 means frozen (channels only; noise never freezes). Implement with E (no underflow at machine_time 0) and offsets o_i = r_i - 1 from E.

```
ds_psg_advance(inst, E, S, obs)        /* fold every event with time in [E, S]; afterwards P = S; S < E is a no-op */
  d    = ds_psg_divisor(inst)          /* 3 or 6 from the bound profile field (RF-3) */
  span = S - E                         /* <= 599 between steps or at a write (RF-1) */
  want = obs != NULL && obs->pcm.on    /* the only read of the pcm sink */
  mod  = modulation_applies(inst)      /* CTL != 0, select 0 and 1 chON (RF-4) */
  cpl  = mod && TRG == 0               /* select 1 steps at d * F1e * FLFe */
  t = 0                                /* offset of the next instant not yet processed */
  loop:
    u = span + 1                                           /* sentinel: past S */
    if cpl or mod: u = min(u, o[1] if select 1 running, o[0] if select 0 running)
    for n in 4, 5: u = min(u, on[n])
    if want: u = min(u, first sample offset >= t)          /* (192 - (E + t) mod 192) mod 192 + t */
    if u > span: break
    /* instant u, tie order: select 1, select 0, (other channels lazily), noise 4, 5, sample */
    if cpl and o[1] == u: widx[1] = (widx[1] + 1) & 31; o[1] += d * F1e * FLFe
    if mod and o[0] == u: widx[0] = (widx[0] + 1) & 31; o[0] += d * F0e'      /* F0' read now, after the select-1 step */
    for n in 4, 5: if on[n] == u: lfsr[n] = step(lfsr[n]); on[n] += noise_period(n)
    if want and (E + u) mod 192 == 0:
        catch_up_closed_form(every running channel not handled above, to offset u)
        ds_psg_sample(inst, &l, &r); emit(obs, l, r)        /* const: reads state only */
    t = u + 1
  catch_up_closed_form(every running channel not handled above, to offset span)
  store r_i = o_i - span for every running counter (>= 1), r = 0 for frozen ones
```

Closed form for a running channel without coupling (period Q, pending offset o <= target T): k = (T - o) / Q + 1; widx = (widx + (k & 31)) & 31; o = o + k x Q (k x Q <= T - o + Q, so the result <= T + Q). It is exact because a register can change only at a write, which is a fold boundary, and the pending event keeps the period it was scheduled with (D-185).
When modulation applies but stepping is not coupled (TRG = 1), select 0 still reloads with F0' from wave[1][0]: handle it in the event loop (its period changes only at writes, so closed form between writes is also exact). Optional speed-up: between two select-1 steps F0' is constant, so select 0 may be caught up in closed form to (next select-1 step - 1).

Write path at stamp w (bus.c HW_PSG, D-188): `charge_penalty` (no-op) -> stamp -> `io_buffer = value` -> `ds_psg_advance(inst, *bus->psg_at, w, obs)` -> `*bus->psg_at = w + 1` -> `ds_psg_write(inst, reg, value, w)` returns sel_before and effect -> `ds_obs_psg` -> existing `log_dev`. Step end (cpu.c, after the VDC fold): `ds_psg_advance(inst, psg_at, start + total - 1, log)`.
In `ds_psg_write`, offsets are relative to E' = w + 1: a frozen -> running transition sets o = (w - w mod d) + period - (w + 1), which is in [0, period - 1] (RF-5); running -> frozen sets r = 0.

### Loop bounds and integer proofs (D-195)

- Span: <= 599 offsets per call (RF-1); a test that folds a long span directly (`psg_fold` "one jump") is linear in it.
- Events per call at the hostile extreme (d = 3, every F and FLF = 1): select 1 <= span / 3 + 1 = 200, select 0 <= 200, noise <= 2 x (span / 96 + 1) = 14, samples <= span / 192 + 1 = 4; total about 420. Typical: 0 to 4.
- Offsets: every o and r <= 600 + 6,291,456 < 2^23 (uint32_t); u64 is never added near 2^64 (RF-8). Periods: channel <= 24,576, coupled <= 6,291,456, noise <= 11,904, all u32 (`_Static_assert` on the d = 6 maxima).
- LFO sum in 3,840..8,431 (unsigned, no wrap); att <= 91; VOL indexed only when att <= 29; per side <= 6,094,848 < 2^31; LFSR uint32_t masked to $3FFFF; every shift on a widened unsigned operand; no float, no signed overflow, no right shift of a negative value.

### Snapshot regions (D-202) and restore rules (D-203)

| Region | Payload | Stream order (little-endian, field by field) |
|--------|---------|-----------------------------------------------|
| PSG0 | 42 | psg_freq[6] u16, psg_select, psg_main_bal, psg_lfo_freq, psg_lfo_ctl, psg_ctl[6], psg_bal[6], psg_widx[6], psg_dda[6], psg_noise[2] |
| PSGW | 192 | psg_wave[192], byte ch x 32 + i |
| PSGT | 40 | psg_rem[6] u32, psg_nrem[2] u32, psg_lfsr[2] u32 |

Pass-1 checks, every failure `snapshot.field` (12), nothing written until pass 2: select <= 7; every freq <= $FFF; ctl & $20 = 0; widx <= 31; dda <= 31; noise & $60 = 0; lfo_ctl & $7C = 0; every wave byte <= 31; every LFSR in 1..$3FFFF; DDA set => widx = 0; (lfo_ctl bit 7 => widx[1] = 0 is dropped by D-231: R6 writes under TRG 1 advance widx[1]);
rem[i] = 0 exactly when channel i is frozen (chON 0, DDA 1, or i = 1 with TRG 1), else 1 <= rem[i] <= 24,576 (6,291,456 for i = 1); 1 <= nrem <= 11,904; every running counter on the grid: (machine_time + r - 1) mod d = 0 (written so machine_time 0 cannot underflow). The PSGT decoder reads the PSG0 values from the walker context (the `dec_beam` reads `v->time` precedent).
Hostile corpus: flip every control field; per-region perturbation of PSG0, PSGW, PSGT; out-of-range r; r off the grid; LFSR 0 and $40000; the restore stops of D-200.

## 5. Records, sinks, ABI and runner (D-196..D-201)

DSPE v1 record (24 bytes, little-endian, written byte by byte; one per CPU write to window 2, block bytes and undecoded writes included; joins `dev` 1:1 on stamp, pc, value, phys & $F):

| Off | Type | Field | Rule |
|-----|------|-------|------|
| +00 | u64 | stamp | the access stamp, as in dev |
| +08 | u16 | pc | WR-02 rule |
| +10 | u8 | reg | offset & $0F |
| +11 | u8 | value | unmasked |
| +12 | u8 | sel | R0 before the write |
| +13 | u8 | chan | 0..5 for R2..R7 targets; $FE for R0, R1, R8, R9; $FF no target |
| +14 | u8 | effect | b0 WAVE_STORE, b1 DDA_LOAD, b2 WAVE_HELD, b3 IDX_RESET, b4 LFO_RESET, b5 NO_TARGET, b6 UNMODELLED, b7 0 |
| +15 | u8 | wave_idx | target counter after the write; $FF when chan > 5 |
| +16 | u16 | post | stored value after the write (R0 3 bits, R2/R3 the 12-bit F, R4 & $DF, R6 word or latch, R7 & $9F, R9 & $83, R1/R5/R8 byte); $FFFF for NO_TARGET |
| +18 | u8 | version | 1 |
| +19..+23 | - | zero | - |

Effect rows of `psg_effects.def`: WAVE_STORE = R6 with chON 0 and DDA 0; DDA_LOAD = R6 with DDA 1; WAVE_HELD = R6 with chON 1 and DDA 0; IDX_RESET = R4 whose new value has DDA 1; LFO_RESET = R9 with bit 7 = 1; NO_TARGET = reg $A..$F, R2..R7 at select 6/7, R7 at select 0..3; UNMODELLED = R9 with CTL != 0 before 05-09 lands (or after a cut).

DSPA v1: frame = `u32 L, u32 R` unsigned little-endian (8 bytes), 0 = nothing contributes. `struct ds_pcm_sink { uint8_t header[32]; uint8_t *frames; uint32_t capacity; }` (capacity in frames).
Header, written at the start and end of every accepted call and never read: +00 u32 count, +04 u32 lost (saturating), +08 u64 seq, +16 u64 first (= ceil(T0 / 192), defined when seq = 0), +24 u32 sample_mclk (192), +28 u16 frame_bytes (8), +30 u8 version (1), +31 u8 format (1).
`DS_PCM_ALIGN` 8; capacity above `DS_PCM_SINK_MAX_FRAMES` 8,388,608 (64 MiB, about 75.0 s) is `run.sink_unsupported`. Drop-newest in whole frames, sticky full; lost = seq - count; lost != 0 is harness_failure for any evidence case.
`struct ds_psg_sink { uint8_t header[16]; uint8_t *records; uint32_t capacity; }`: the dev shape (count, lost, seq), 24-byte records, align 8, capacity <= `DS_SINK_MAX_RECORDS` (1,048,576), drop-newest, `ds_obs_chan` reused.

ABI changes (plan 05-02, one change): `struct ds_run_request` gains `struct ds_psg_sink *psg; struct ds_pcm_sink *pcm;` after `struct ds_vid_sink *vid;` (ds_core.h:391), size 56 -> 72 on LP64. `ds_obs_sinks_check` grows from 3 to 5 channels, same order (structs memory_invalid, capacities sink_unsupported, ranges memory_invalid) with per-channel alignment, wrap and overlap rows.
`ds_obs_begin(struct ds_obs_log *, const struct ds_run_request *, uint64_t machine_time)` replaces the three-pointer form; seven callers (RF-17). New hooks `ds_obs_psg` (const instance) and `ds_obs_pcm`.
IN-03: after the struct_size and reserved checks, `ds_run` refuses a request or result overlapping the instance as `memory_invalid`, before the sink checks (`run_refuse_call_overlap`, two rows, the request copied into a malloc'd instance block).

Streams (runner-built, never core): DSPE = `"duospool-psg"` NUL, u32 1, records. DSPA = `"duospool-pcm"` NUL, u32 1, u32 sample_mclk, u16 channels 2, u8 format 1, u8 0, u64 first_frame, frames. No count or length field. Digest = SHA-256 of the stream bytes; in-process equivalence by memcmp; never in the state digest.
Runner: `ds_runner --cases FILE --audio-out CASE_ID` writes `build/gate/audio/CASE_ID.dspe`, `.dspa`, `.tsv` (E per event, F per video frame with first frame and count, P run-length groups) only for an `audio` case whose fixture is authored (listed in `build/gate/fixtures-authored.used`) or synthetic; ids `[a-z0-9_]+`; stdout is three lines.
Rights: case `audio_out_rights` mirrors `video_out_rights` row for row (allow and deny fixtures, allow and deny cases, bad ids); `audio_out_check` in `scripts/conformance-run` mirrors `video_out_check` (no directory before the request, exactly three lines, host `shasum -a 256` of both files equals the evidence).

## 6. Worked examples of every check kind (hand literals for 05-04 onward)

### The tracer scene (select 0, d = 6, F = 256)

Table (32 distinct words, a high half >= 16 then a low half <= 15, unique maximum 31 at word 5; no rotation, reversal, complement or reversed complement of it is a rotation of it [DERIVED], so AF7 and a polarity bug both fail `cycle_order`):
`$10 $13 $16 $19 $1C $1F $1D $1A $17 $14 $11 $12 $15 $18 $1B $1E $0F $0B $07 $03 $00 $04 $08 $0C $0E $0A $06 $02 $01 $05 $09 $0D` (decimal 16 19 22 25 28 31 29 26 23 20 17 18 21 24 27 30 15 11 7 3 0 4 8 12 14 10 6 2 1 5 9 13). Closed form for the comment: words 0..5 = 16 + 3i, 6..10 = 29 - 3(i - 6), 11..15 = 18 + 3(i - 11).
Write sequence and the DSPE rows it yields (chan, effect, wave_idx, post): R0 $00 ($FE, $00, $FF, $0000); R4 $40 (0, $08, 0, $0040); R4 $00 (0, $00, 0, $0000); 32 x R6 by `tin` to $0806, write k = 1..32 (0, $01, k & 31, word k - 1); R2 $00 (0, $00, 0, $0000); R3 $01 (0, $00, 0, $0100); R1 $FF ($FE, $00, $FF, $00FF); R5 $FF (0, $00, 0, $00FF); R4 $9F key-on (0, $00, 0, $009F).
Value and order of these rows are GUEST:program; the decode fields are MAN:P8-2+P8-3+P8-4. att = 0 on both sides, so a sample is 32768 x word (the exact value is ASSUMED, `levels_exact`; order rows do not use it).

**Period crossing distance (MAN:P8-9+H8-23).** Key-on at stamp w; w' = w - (w mod 6); widx = 0 at key-on (procedure (2) leaves the counter at 0 after 32 writes). Step j (j >= 1) happens at w' + 1536 j and sets widx = j mod 32. The threshold (min + max) / 2 of a cycle lies between 15 and 16 words, so the only rising crossing of a cycle is word 31 -> word 0, at steps j = 32k.
The crossing sample is the first sample at or after the event (events precede the sample at one instant): c_k = ceil((w' + 49,152 k) / 192) = ceil(w' / 192) + 256 k, because 49,152 = 256 x 192. The distance over K cycles is 256 K whatever w is. The first cycle starts at the key-on write and is excluded (D-185): measure between recurrences.
Literal row: window from = +50,000, to = +300,000 master clocks after the key-on event; the first crossing inside the window is c_2 (the event of c_1, at w' + 49,152, is before w + 50,000) and the fifth is c_6 (its event at w' + 294,912 is before w + 300,000); `f=256;k=4;d=1024` (the lint recomputes d = k x f).
Numeric instance for the comment (w = 100,003 chosen for illustration, never read from a run): w' = 100,002, ceil(w' / 192) = 521, crossings 777, 1033, 1289, 1545, 1801, 2057; window samples 782..2083; inside: 1033, 1289, 1545, 1801, 2057; distance 2057 - 1033 = 1024.
Expect-red controls: d = 3 (cycle 24,576 = 128 samples): crossings inside the same window are c_3..c_7 of the 128 grid, distance 512, red. Twin F + 1 = 257 (cycle 49,344 = 257 samples): distance 1,028, red. Both change no other group.

**Waveform order-isomorphism (MAN:P8-4+P8-5).** Between two consecutive rising crossings (for example samples 1033 and 1289 in the instance above) run-collapse the samples into plateaus: 32 runs of 8 samples at d = 6 (4 at d = 3), values 32768 x word. Because the table is a permutation of 0..31, the rank of a plateau equals its word, so the literal is the table itself:
`cycle_order words=16,19,22,25,28,31,29,26,23,20,17,18,21,24,27,30,15,11,7,3,0,4,8,12,14,10,6,2,1,5,9,13` and the check is "the rank sequence of the 32 plateaus equals a rotation of this list" (here rotation 0, because the window starts at word 0). AF7 (reverse each cycle) gives the reversed list, not a rotation: red. Plateau starts at d = 6: 1033 + 8i.

**Silence paired with a live row (MAN:P8-4).** Window from the R1 $FF write to the key-on write (two anchors, RF-13), both sides: every sample = 0 (chON = 0 everywhere; the value 0 is a format definition, ER-19). Paired live rows in the same scene and side: the `period` and `cycle_order` rows above (L and R). `AUDIO_SILENCE_UNPAIRED` fails a scene whose silent row has no live partner.
P8-10 rows for 05-06: R1 $C8, AL $1D, R5 $F8: left live (S 4); right S 15 silent in the model, ASSUMED (DISPUTED); with RMAL 7 (R1 $C7) the right sum is 16: MAN silent. Pan isolation: LMAL F, AL $1D, LAL 0: left S 16 MAN silent; RAL F: right live.

**Noise two-level run-length grid (MAN:P8-2+P8-6+P8-9+H8-23).** `psg_mod`, select 4 solo, R7 = $98 (NE 1, NFRQ 24, NFc 7) written at stamp w_n. Noise events run from boot every 11,904 (= 62 x 192) until the first reload after w_n, then every 2,688 (= 14 x 192); every event lies on a sample instant, and the level changes only at events.
Literal rows for a window starting 11,904 after w_n (past the pending old-period event): `two_level count=2` (exactly two distinct values; the values 0 and 31 x VOL[att] are ASSUMED) and `run_grid nf=7;g=14` (every interior run length is a multiple of 14 samples; the lint recomputes g = 384 x nf / 192). `noise_ratio`: NFRQ 17 (NFc 14) gives g = 28.
`noise_seq` (ASSUMED, names its policy): the output bits after the n-th event since boot, from the LFSR table of section 2 (n counts events at 11,904 until the reload, then 2,688; a throwaway script may draw the bits, the committed literal is judged).

**DDA literal list (MAN:P8-4+P8-5; schedule rows GUEST).** Select 2 (example), procedure (3): chON 0 DDA 1, R6 $00, then R4 = $DF (chON 1, DDA 1, AL $1F). Timer-paced writes of the list 5, 5, 20, 3, 31, 0, 12, 12, 27 (one per timer interrupt; a prescaler step is 3,072 = 16 samples, so a reload R gives 16 (R + 1) samples between writes plus handler jitter).
`dda_hold`: the level changes exactly at write ordinals 1, 3, 4, 5, 6, 7, 9 (writes 2 and 8 repeat the value), in the first sample after each write stamp (a sample at the stamp sees the old level), and is constant between writes. `dda_order`: the held levels 5, 20, 3, 31, 0, 12, 27 are order-isomorphic to the written values.
`dda_ram_kept`: after leaving DDA (chON 0 DDA 1, then DDA 0, then chON 1), `cycle_order` of the playback equals the table uploaded before DDA.

**LFO TRG probe (`lfo_static`, MAN:P8-6..P8-9+H8-23; 05-09).** Select 1: wave word 0 = $18 (+8), chON 1, R5 = $00 (silent by the byte-zero clause, MAN), R9 TRG = 1 (widx[1] = 0, frozen). Select 0: F0 = 512, solo. Under RF-4's ruling: CTL 1 -> F0' 520 -> cycle 520 samples; CTL 2 -> 544; CTL 3 -> 640; CTL 0 -> 512.
Rows `f=512;w=$18;shift=0|2|4;k=2;d=1040|1088|1280` (the lint recomputes F0' and d = k x F0'). d = 3: 260, 272, 320 per cycle (expect-red). `lfo_needs_sel1`: select 1 chON 0 with CTL 1 gives 512 (`same` as CTL 0). `lfo_dynamic` example: select-1 words 0-15 = $18, 16-31 = $08, F0 = 192, F1 = 128, FLF = 150, TRG 0, CTL 1:
cycle lengths in {200, 184} samples, a select-1 step every 115,200 master clocks = 600 samples (3 select-0 cycles), a half table every 9,600 samples (about 5.2 frames); cycles that straddle a switch are excluded.

## 7. Judge grammar sketch, fault sweep and kill matrix (D-207..D-211)

`.aexp` (TAB separated like `.vexp`, keys closed and in fixed order, 512 rows, 400 characters per line, 16 KiB, SPDX 0BSD header, ASCII):
```
header  sample_mclk=192                                         (must equal the DSPA header; a mismatch is harness_failure)
scene   <S>  case=<id>  role=literal|golden|expect_red  [twin=<S>]  [expect_red=<gid,...>]
group   <S>  <gid>  form=ROWS|REL|STRUCT|EV  kind=<kind>  class=MAN:<labels>|GUEST:<model>|ASSUMED:<LED-id>  kills=AF0,...
row     <S>  <gid>  <selector>  <literal>
selector: anchor=ev:ch=<0..5|G>;reg=R<n>;val=$<hh>;nth=<n>;from=+<mclk>;to=+<mclk>|to=ev:<...>[+<mclk>];side=L|R
```
Codes (each with its own tag and its own negative mini-root under `tests/audio/`, the gate H-04 lesson): AEXP_FORMAT, AEXP_UNKNOWN_KEY, AEXP_LINE_TOO_LONG, AEXP_NON_ASCII, AEXP_NO_SPDX, AEXP_TOO_MANY_ROWS, AEXP_FILE_TOO_LARGE, AEXP_ARITH, AEXP_CLASS_INFLATED (KINDCLASS), AUDIO_DISPUTED_POINT (DISPUTED), AUDIO_RULE_NO_ROW (RULES), AUDIO_SILENCE_UNPAIRED,
AUDIO_GROUP_VACUOUS, INV_AUDIO_GROUP_UNKILLED, INV_AUDIO_HASH_ONLY; runtime `audio-hash-only`, `misprog-misattributed`. Selftest mutations: remove INV_AUDIO_HASH_ONLY, remove AUDIO_GROUP_VACUOUS, remove the kind-to-class lint. Add an inventory link (gate M-04 lesson): every scene case is a REQUIRED `audio` case and every literal scene has one `audio_fault_sweep` case.

Fault sweep AF0..AF9 (locked list) and the expected kill matrix (a planning guide, verified by running; a group no declared fault flips is vacuous):

| Kind | Expected kills | Why the others cannot |
|------|----------------|-----------------------|
| period, lfo_static | AF0, AF2, AF4, AF5, AF6 | AF1 shifts both crossings; AF7 keeps the period |
| period_ratio (REL) | AF0, AF4 on one window | - |
| cycle_order | AF0, AF5, AF6, AF7 | AF2 moves a plateau by one sample, ranks unchanged |
| cycle_phase (ASSUMED) | AF0, AF1, AF7 | - |
| levels_exact (ASSUMED) | AF0, AF5, AF6 | - |
| silent | AF0, AF6; AF3 on a pan pair | AF5 keeps zero |
| live | AF0, AF5, AF6 | - |
| same (REL) | AF0, AF2 on one run | - |
| two_level | AF0, AF5, AF6 | - |
| run_grid | AF0, AF2, AF4 | AF5 and AF6 leave no interior run: declare them only if vacuity is a red |
| dda_hold | AF0, AF1, AF8 | - |
| ev | AF0, AF8, AF9 | - |
| atten_order (REL) | AF0, AF5, AF6 | - |

## Standard Stack

No new product dependency, no new tool (D-216). Language: C11 with CMake and Ninja (decision record `docs/decisions/0001-language-build.md`); the Rust text in CLAUDE.md is an unadopted proposal.

| Item | Version here | Role |
|------|--------------|------|
| C11 core and runner | `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`; `-fwrapv` only on the gate preset, none on sanitize | Everything under `src` and `tests/conformance` |
| cmake, ninja | 4.4.3, 1.13.2 [CO] | Presets gate, gate-gcc, gate-clang, sanitize |
| ca65, ld65 | prints V2.18 [CO] (tag V2.19) | Fixtures, a new `fixtures/lib/audio.inc` |
| awk | BWK 20200816 here; gawk and mawk through `linux-repro` | `audio-check` (POSIX awk only) |
| shasum | 6.02 [CO] | External recomputation of goldens and stream digests |
| docker | 29.5.2, daemon answering [CO] | `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` (gcc 13) |

### Package Legitimacy Audit

Not applicable: this phase installs no external package. `gsd-tools package-legitimacy` was not run because there is nothing to check.

## Code-level integration map

Code facts read this session (verbatim): `#define DS_SERIAL_VERSION 8u` (ds_serial.h:22); `#define DS_REGION_COUNT 21u` (ds_serial.h:41); `#define DS_BEHAVIOUR_EPOCH 5u` (ds_core.h:22); `#define DS_TEXT_FORMAT_VERSION 7u` (ds_text.h:36); `DS_FIELD(psg_tick_divisor, OPEN, "", "3|6", "LED-AUD-0001")` (profile_fields.def:29);
`DS_CONST(cycle_mclk_low, 12, ...)` and `DS_CONST(cycle_mclk_high, 3, ...)` (constants.def:41-42), 34 `DS_CONST` rows (lines 41-74); `#define OX_ROMS_MAX 40u` (obskinds.c:305); `static const char *const default_policies[] = {"", "bind.powerup_policy=seeded-prng-v3;seed=1", "bind.powerup_policy=seeded-prng-v3;seed=2"};` (guestkinds.c:867).

| File | Change | Plan |
|------|--------|------|
| `src/core/ds_core.h` | request members `psg`, `pcm` after `vid` (383-392); `struct ds_bus` gains `uint64_t *psg_at` last (428-437, RF-2); epoch 5 -> 6; PSG block after `vid_pad` (RF-10), tiling list (284-295), last-member assert (300) | 05-02 (request), 05-03 (rest), 05-04 (bus member) |
| `src/core/ds_obs.h`, `obs.c` | two sink structs, constants, five-channel check table, `ds_obs_begin` new form, `ds_obs_psg`, `ds_obs_pcm` | 05-02 |
| `src/core/run.c` | IN-03 overlap refusal after the reserved check; log gate (106); `ds_obs_begin` call (108) | 05-02 |
| `src/core/bus.c` | HW_PSG write case (186-188): fold, write, emit, then the existing `log_dev`; reads untouched (134-136) | 05-02 (empty fold), 05-03, 05-04 |
| `src/core/cpu.c` | `ds_cpu_step_obs` (1099-1105): `psg_at`, the PSG fold after the VDC fold; three `struct ds_bus` initialisers (514, 679, 694) | 05-02 (empty fold), 05-04 |
| new `src/core/psg.c`, `ds_psg.h` | registers, write decode, fold, const sample, divisor helper, poweron; include only stddef, stdint, `ds_core.h`, `ds_obs.h` | 05-02 skeleton, 05-03, 05-04 onward |
| `src/core/ds_serial.h`, `serial.c`, `snapshot.c` | serial 8 -> 9, regions 21 -> 24, three regions and decoders with every D-203 rule | 05-03 |
| `src/core/poweron.c`, `ds_poweron.h`, `profile_fields.def` comment | seeded-prng-v4, domain `PSGW____`, allowed set | 05-03 |
| `src/core/constants.def`, new `psg_vol.def`, `psg_effects.def` | 13 rows (34 -> 47), 30-row table, effect X-table; rows land with their first user | 05-03..05-09 |
| `src/text/ds_text.h`, `text.c` | format 7 -> 8 | 05-03 |
| `tests/conformance/{snapkinds,digkinds,chunkkinds,guestkinds,obskinds,stepkinds,powkinds,profkinds,reckinds}.c` | SK constants (RF-10), tokens 191 and 193 (RF-9), policy list v4, OX_ROMS_MAX 40 -> 48, sink matrices, old-size request, v4 draw cases | 05-02, 05-03, 05-04 |
| new `tests/conformance/audiokinds.c` | `audio` and `audio_fault_sweep` kinds, streams, `--audio-out`, hand-written offsets | 05-04, 05-05 |
| `scripts/check`, new `scripts/audio-check`, `scripts/lib/aexp-*.awk`, `tests/audio/` | stage 59, selftest mutations, mini-roots | 05-05 |
| `scripts/fixture-build`, `fixture-lint` rules, `fixtures/lib/cites.txt`, `TOOLS.md` | `NAME.aexp.tsv`, `P8-` prefix, PSG manual row | 05-01, 05-04 |

## Architecture Patterns

### System architecture diagram

```
 guest ROM (ca65) --step--> ds_cpu_step_obs: start = machine_time; psg_at = start
   |                             total = cpu_step_body(inst, log, &psg_at)
   | bus write to window 2       ds_vdc_advance(start + total - 1)
   v                             ds_psg_advance(psg_at, start + total - 1)      <-- fold point 2
 bus.c hw_write HW_PSG: stamp w; io_buffer = v; ds_psg_advance(*psg_at, w); *psg_at = w + 1   <-- fold point 1
   |   ds_psg_write(reg, v, w) -> sel_before, effect --> ds_obs_psg (DSPE record) --> log_dev (dev record)
   v
 ds_psg_advance [E, S]: instants in time order; ties: select-1 step, select-0 step, noise 4, 5, sample
   channels without coupling: closed form at each sample and at S; coupled pair and LFSRs: event by event
   sample instant s*192 with a pcm sink: ds_psg_sample (const) -> L, R -> pcm sink (drop-newest, sticky full)
 runner: DSPE and DSPA streams -> audio judge (ROWS, REL, STRUCT, EV) vs build/gate/fixtures/NAME.aexp.tsv; fault sweep AF0..AF9
```

### Pattern 1: one fold, two call sites, targets from the instruction stream only
Every fold target is an access stamp or `start + total - 1`, so chunking, restore at a stop and an attached sink change nothing (the D-137 argument). The fold is inclusive and composable: fold(a) then fold(b) equals fold(b) when no write lies between.

### Pattern 2: distances, not deadlines
State holds r per counter relative to the fold point; restore checks are pure ranges plus the grid rule; nothing derived (no next-sample index, no folded-to stamp) is stored.

### Pattern 3: a const sample function behind a sink gate
Without a pcm sink the fold processes every event and skips only `ds_psg_sample`; the pcm pointer is read only at the gate and the emit.

### Anti-Patterns to Avoid
- **Computing an absolute next time:** wraps near 2^64 after a hostile restore; use offsets (RF-8).
- **Deriving any literal from the bound divisor or from a run:** a d = 3 case that expects MAN period rows to pass is the bug (D-183, R7).
- **Measuring a period from a write:** the first plateau after key-on depends on ASSUMED phase rules; measure between recurrences.
- **Editing `check.inc`, `startup.inc` or `verdict.inc`:** changes every authored image hash; put PSG helpers in `fixtures/lib/audio.inc`.
- **Uploading wave RAM with TIA:** TIA alternates the destination ($0806, $0807); use TIN or an STA loop.

## Don't Hand-Roll

| Problem | Do not build | Use instead | Why |
|---------|--------------|-------------|-----|
| Stream and golden digests | a second hash | `ds_sha256.h` in the runner, `shasum -a 256` outside | one implementation, one external check |
| v4 power-on draws | a new PRNG | `ds_stream_init`/`ds_stream_fill` (splitmix64, poweron.c) with domain `PSGW____` | additive policy, same derivation as v1..v3 |
| `.aexp` parsing | a C parser | the awk subset machinery through `fixture-build` and `audio-check`, TSV into the runner | one parser of hand-edited data (D-51) |
| Sink checks | per-sink copies | the table-driven `ds_obs_sinks_check` (ranges_overlap, alignment 8) | one rule for five channels |
| Expected values | a reference synthesiser, LFSR generator or volume recompute stage | hand literals plus the static arithmetic lint | ER-7, ER-18 |

**Key insight:** every mechanism exists; the work is a fourth and fifth sink, a 274-byte state and a judge whose every rule turns red when deleted.

## Common Pitfalls (from the Phase 3, 4 and 4.1 reviews, applied to audio)

### Pitfall 1: evidence class inflated at write time (gate M-01, H-02; 4.1 H-01, M-01: three recurrences)
**What goes wrong:** a converse ("silent below a threshold", "no modulation without TRG") or a disputed boundary (sum 15) tagged MAN. **How to avoid:** class every row when it is written (D-216 R1); KINDCLASS and DISPUTED are data the gate enforces; audible rows at sums 5..14 are ASSUMED (ER-16); the field-zero reading of P8-10 is ASSUMED unless the sum is >= 16.
**Warning signs:** a MAN row whose comment says "reading", "assumed" or "the model".

### Pitfall 2: rules without rows (4.1 M-02, I-02, the D-160 SB gap)
**What goes wrong:** a decision clause no fixture exercises, with a surviving mutant. **How to avoid:** `tests/audio/RULES` lists every clause of every AUD statement and decision (including the RF rulings adopted); AUDIO_RULE_NO_ROW fails a clause whose only kill is AF0; the 05-08 scratch mutant sweep of `psg.c` lists survivors in the SUMMARY.

### Pitfall 3: claims outrunning fixtures (gate H-03, L-03; 4.1 M-05, M-06)
**What goes wrong:** an E1 statement wider than its MAN rows, or a README or validation file written ahead of gate output. **How to avoid:** narrow each E1 sentence to the clauses a MAN guest row shows (ER-22); write README and VALIDATION text from printed class counts (D-216 R8); keep period, LFO and noise-clock entries `assumed` (ER-14).

### Pitfall 4: toys red for the wrong reason (gate H-01; 4.1 M-03)
**What goes wrong:** a toy kills a case through its DSV1 verdict, never evaluating the audio group it claims. **How to avoid:** the audio kind evaluates and prints every group even when DSV1 fails; EXPECT names the group; evidence must show that group failing (D-216 R5).

### Pitfall 5: enforcement rules deletable with controls green (gate H-04)
**How to avoid:** one tag and one mini-root per `audio-check` code (section 7); the selftest deletes one rule at a time.

### Pitfall 6: a fixture without chunk, restore, observer and non-interference coverage (4.1 M-04)
**How to avoid:** D-200's proof set lands with each ROM (D-216 R6); schedules 7, 191, 193, 1366, vframe (RF-9).

### Pitfall 7: strict C11 on Linux gcc 13
**What goes wrong (recorded):** `-Werror=format-truncation` on bounded buffers (03-33 SUMMARY: five sites; 04-02 SUMMARY: one more); `strtok_r` undeclared under `-std=c11`; `memcpy(dst, NULL, 0)` flagged by UBSan on Linux; new here: `-Wmissing-field-initializers` on the positional `struct ds_bus` initialisers (RF-2); `-Wconversion` on every u32 -> u8/u16 store of a PSG field.
**How to avoid:** precision-bounded `%.Ns` in every `snprintf`; no POSIX calls; widen to unsigned before shifts (the sanitize preset has no `-fwrapv`); `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` before every C plan closes; an unavailable container is "unverified", never passed.

### Pitfall 8: the digest cascade of the one bump
**What goes wrong:** every digest moves (the noise counters run from boot, RF-11), not only the PSG-writing ROMs. **How to avoid:** rehearse the regeneration outside the repository first; the independent PSGT closed form of RF-11; check each old literal against `shasum -a 256` of the old stream first; the F-3 cascade list plus the RF-10 snapkinds constants.

### Pitfall 9: a behaviour change after a push without an epoch bump (4.1 I-05)
**How to avoid:** D-216 R11; the phase summary records reliance on the unreleased epoch 6 until the first push.

### Pitfall 10: literals or tolerances read from a run (gate M-05, L-01)
**How to avoid:** selectors anchor on events and offsets written from the formulas; the illustrative stamp of section 6 is never committed as a literal; any throwaway script first reproduces a committed value (D-216 R7).

### Pitfall 11: long audio ROMs and the insn sink (RF-12)
**How to avoid:** keep each ROM under about 35 frames of high-speed waiting or wait at low speed; scenes at most 16 frames (D-214).

### Pitfall 12: lines over 400 characters and case-list limits (4.1 L-04; the 04-18 deviation)
**How to avoid:** a 32-word table literal is about 90 characters; keep `.aexp` rows, case `expect` strings (PK_LIST_MAX 512) and ledger lines under the limits; `audio-check` enforces 400.

## Code Examples

```c
/* the LFSR step and the output level from the stored state (D-193, RF-6); s is in 1..0x3FFFF */
static uint32_t psg_lfsr_step(uint32_t s)
{
    uint32_t b = (s ^ (s >> 1u) ^ (s >> 11u) ^ (s >> 12u) ^ (s >> 17u)) & 1u;
    return ((s >> 1u) | (b << 17u)) & 0x3FFFFu;
}
static uint32_t psg_noise_bit(uint32_t post) /* the bit shifted out by the last step */
{
    return (post ^ (post >> 10u) ^ (post >> 11u) ^ (post >> 16u) ^ (post >> 17u)) & 1u;
}
/* closed-form catch-up of one uncoupled channel: o and t are offsets from E (RF-8), q the period */
static void psg_catch_up(uint8_t *widx, uint32_t *o, uint32_t t, uint32_t q)
{
    if (*o <= t) {
        uint32_t k = (t - *o) / q + 1u;
        *widx = (uint8_t)(((uint32_t)*widx + (k & 31u)) & 31u);
        *o = *o + k * q; /* k * q <= t - o + q < 2^24 */
    }
}
```

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Project gate: sh and awk stages in `scripts/check` plus the C runner `build/gate/ds_runner` whose RESULT lines `conformance-ingest` judges; no third-party framework |
| Config | `CMakePresets.json`, `tests/conformance/REQUIRED` (339 lines), `tests/conformance/cases/*.toml` (334 files), `fixtures/manifest/*.toml`, `fixtures/src/*.{s,checks,aexp}`, `tests/controls/*/{RULE,EXPECT}` (26 toys) [CO] |
| Quick run (one case) | after `cmake --build --preset gate` and one `sh scripts/check --stage conformance`: `build/gate/ds_runner --cases build/gate/cases.tsv --evidence-dir build/gate/ev-quick --profile-tsv build/gate/r1.tsv --rules-fields build/gate/rules-fields.txt --oracle build/gate/oracle.tsv` |
| Quick run, continued | `--bank-oracle build/gate/bankclass.tsv --inputs build/gate/inputs.tsv --opcode-tsv tests/oracle/huc6280-opcodes.tsv --marker-seed 7 --golden-dir tests/conformance/golden --only ID` |
| Quick run (stages) | `sh scripts/check --stage conformance`, `--stage audio-check` (new), `--stage fixture-build`, `--stage fixture-lint`, `--stage inventory`, `--stage docs-claims` |
| Full suite | `sh scripts/check`, then `sh scripts/check --selftest`; at 05-08 and 05-10 also `dash scripts/check`, the second awk and all six Linux parts |
| Strict C11 and Linux | `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` per C plan |
| Baseline | 57 `run_stage` lines in `scripts/check` this session (the CONTEXT budget counts 58; `audio-check` adds one either way); local warm gate re-recorded in 05-01 (D-214) |

### Phase Requirements to Test Map

| Req / SC | Behaviour proved | Automated check (case or stage) | Literal source (class) | Control that makes it fail | Plan |
|----------|------------------|----------------------------------|------------------------|----------------------------|------|
| AUD-01 | Decode, mirrors, select 6/7, R7 at select 0..3, masks | `psg_decode` (C level); `ev` rows of `psg_tone` | MAN P8-2, P8-3, H8-24; drop rules ASSUMED LED-AUD-0004 | toy `psg-chsel-mask`; AF8, AF9 | 05-03, 05-06 |
| AUD-01 | Wave pointer procedures (1)-(3), DDA level reset, latch | `psg_wave_ptr`, `dda_*` rows of `psg_mod` | MAN P8-4, P8-5; write-while-on ASSUMED | toy `psg-dda-no-reset` (cycle_phase red, cycle_order green) | 05-03, 05-07 |
| AUD-01 | Period formula 192F under d = 6 | `audio_psg_tone` period rows; `psg_pure_function` (F 1, 254, 3421, 4095; NFc 1, 7, 31) | MAN P8-9 + H8-23 (+ P8-12) | d = 3 expect_red case; twin F + 1 | 05-04, 05-06 |
| AUD-01 | Volume, balance, silence rule | `atten_order`, `silent`, P8-10 example rows; `psg_vol_table` | MAN P8-3, P8-4, P8-10 (sums >= 16, byte zero); sum 15 ASSUMED | toy `psg-atten-max` | 05-06 |
| AUD-01 | Noise on select 4 and 5, NE gating, clock grid | `two_level`, `run_grid`, `noise_ratio`, `r7_ignored` | MAN P8-2, P8-6, P8-9, H8-23; LFSR ASSUMED | toy `psg-noise-taps` (noise_seq red, run_grid green) | 05-07 |
| AUD-01 | LFO static, needs select 1, dynamic | `lfo_static`, `lfo_needs_sel1`, `lfo_dynamic`, `psg_lfo_trg` | MAN P8-6..P8-9 + H8-23 (after RF-4's ruling) | toy `psg-lfo-shift` | 05-09 (or 5.1) |
| AUD-02 | DSPE one record per write, joins dev 1:1 | `psg_dev_join`, `ev` rows, STRUCT header rows | definitions LED-AUD-0013 (ER-19) | AF8; toy `psg-emit-mutates` | 05-04 |
| AUD-02 | DSPA grid, count closed form, per-frame counts | `pcm_cadence` (262 and 263), observer seq rows | section 3 literals (ASSUMED LED-VID-0001 + LED-AUD-0014) | toy `psg-fold-skipped-at-step-end` | 05-04 |
| AUD-02 | Sinks, refusals, IN-03, non-interference, chunks, restore | `run_refuse_*`, `run_refuse_call_overlap`, `observer_<rom>`, `aud_noninterference`, `chunk_equiv_audio_<rom>`, `snapshot_restore_audio`, `psg_fold`, `audio_out_rights` + `audio_out_check` | rules written in the cases (E0) | toys above; hostile corpus | 05-02, 05-04 |
| AUD-03 | Semantic rows, never nonzero; null-synth and missing-sink controls | `audio` kind ROWS/REL/STRUCT/EV; `audio_fault_sweep_*`; `audio-check` stage | hand literals (D-206) | AF0..AF9; null synth red; selftest mutations | 05-04, 05-05 |
| AUD-03 | Noise deterministic | `audio_noise_repeat` (zero, v4 s1, v4 s2; two interleaved fresh instances) | E0 repeatability | flip-a-byte negative of the comparison | 05-07 |
| SC1 | All six channels, every register, decoded semantics | `ev` rows over R0..R9 and each channel's R2..R6 in `psg_tone` | GUEST:program + MAN P8-2..P8-4 | AF8, AF9, `psg-chsel-mask` | 05-06 |
| SC2 | Recorded rate and format; non-integer cadence visible | DSPA STRUCT header rows; `pcm_cadence`; TSV `F` rows | definition + ASSUMED | AF2 on cadence | 05-04 |
| SC3 | Period, content, silence; mis-programmed fixture fails | tracer rows; twin T'; d = 3 expect_red | MAN rows of section 6 | `misprog-misattributed` if any other group flips | 05-04, 05-06 |
| SC4 | Noise repeats; clock field stays open | `audio_noise_repeat`; docs-check withheld words; LED-AUD-0001 status `assumed` | E0; ER-14 | docs-check negative control | 05-01, 05-07 |

### Sampling Rate
- **Per task commit:** build, `ds_runner --only ID` for the case under work, the narrowest stage, the task's own red control.
- **Per plan close:** `sh scripts/check`, `--selftest`, `audit-history --range`, and for C plans `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro`.
- **Phase gate (05-08, 05-10):** all six Linux parts, `dash`, the second awk, the timing record, the verification report.

### Wave 0 Gaps (must exist before the code that needs them)
- [ ] `P8-` prefix in `fixture-lint` rules and `fixture-rules.awk` manual-cite pattern, `cites.txt` legend and image-read rows, PSG manual row in `TOOLS.md` (05-01).
- [ ] 12 ledger entries and 6 amendments with PENDING:5 pins, each constants row's id present before `constants.def` grows; LED-AUD-0011 wording per RF-4 (05-01).
- [ ] docs-check withheld words with a negative control (05-01).
- [ ] Errata text for RF-2..RF-5 in the handbook (05-01), so 05-02 can place the `psg_at` carrier and the empty fold.
- [ ] Sink table with `psg` and `pcm` and no producer; `ds_obs_begin` new form; OX_ROMS_MAX 48 (05-02).
- [ ] Chunk tokens 191 and 193 and an audio compare token (RF-9) before the first `chunk_equiv_audio_*` (05-04).
- [ ] `NAME.aexp.tsv` channel in `fixture-build` and a minimal `audio` kind (05-04); the full `audio-check` stage (05-05).

## Security Domain

### Applicable ASVS categories (level 1)

| Category | Applies | Control |
|----------|---------|---------|
| V1 Architecture | yes | `core-boundary-check`; const sample function; sinks output only |
| V2-V4 Authentication, Session, Access | no | no users or network |
| V5 Input validation | yes | validate-then-commit restore (D-203 plus the grid rule); five-channel sink checks; `.aexp` grammar fail-closed |
| V6 Cryptography | integrity only | existing SHA-256 |
| V7 Error handling and logging | yes | stdout ids, counts, sizes, hashes only; audio files only under `--audio-out` |
| V8 Data protection | yes | no manual text or image, ROM or capture committed (ER-9, rights-check) |
| V12 Files and resources | yes | PCM sink cap 64 MiB, event sink cap 1,048,576 records, `.aexp` 16 KiB and 512 rows |

### Known threat patterns

| Pattern | STRIDE | Mitigation |
|---------|--------|------------|
| Hostile snapshot with far distances or LFSR 0 | DoS, Tampering | range checks before any arithmetic; LFSR 1..$3FFFF; offsets bounded by 600 + max period (RF-8) |
| Hostile ROM: F = 1, FLF = 1, NFc 0, d = 3 | DoS | per-step fold bounded (about 420 events, section 4); defined zero conventions (no division by zero) |
| A 65,536-byte TIN flood to $0806 | DoS | drop-newest in whole records, `psg_block_flood` asserts lost counts and an unchanged digest |
| Sink memory overlapping the instance, request, result or another sink; request or result overlapping the instance (IN-03) | Tampering | `memory_invalid` per channel and for the call |
| Audio file of a commercial image | Information disclosure | `--audio-out` only for authored or synthetic fixtures |
| `.aexp` text injection into awk | Tampering | one strict parser, closed keys, 400-character lines, no shell expansion |

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| ca65, ld65 | fixtures | yes | prints V2.18 | none: missing is exit 2 |
| cmake, ninja | build | yes | 4.4.3, 1.13.2 | none |
| C compiler | build | yes | Apple clang (gcc 13 and clang 18 through `linux-repro`) | - |
| docker | strict gcc 13 build, container parts | yes, daemon answering | 29.5.2 | record "unverified" |
| awk | gate | BWK 20200816; gawk, mawk absent locally | - | container parts |
| pdftoppm | ER-1 image reads (05-01) | yes | present | - |
| PSG and HuC6280 manual PDFs | ER-1 reads | outside the repository (session scratch) | sizes and hashes as recorded | re-download from the MiSTer docs mirror; never committed |

Missing with no fallback: none.

## Project Constraints (from .claude/CLAUDE.md and AGENTS.md)

- Language and build: C11, CMake, Ninja (decision record 0001); the Rust proposal text in CLAUDE.md is not adopted.
- Rights: no commercial ROM, firmware, disc image, save or credential committed or used in CI; no manual page text or image in the repository (ER-9); fixtures authored, 0BSD, with provenance.
- Core boundary: deterministic behaviour in the core; no file, clock, environment, allocator, float or mutable global; hosts own resampling, playback and presentation; no public API or ABI before a consumer proof (DSPE and DSPA are project definitions).
- Dependencies: none added. Evidence: a claim names profile and tier; a hash proves repeatability only unless tied to an independent oracle. CI: the fast path stays fast; nothing weakened to save seconds.
- Content as attack surface: sinks, snapshots and `.aexp` data are untrusted input. Authority gates: none crossed (no capture needed).
- Repository rules: `sh scripts/check` is the single verification command; ASCII; lines at most 400 characters; SPDX headers; repository-relative paths; GSD workflow for edits. Runtime State Inventory: not applicable (no rename or migration). State of the Art: not applicable (no external framework).

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | At most one PSG-window write per step in today's instruction set (stack and zero page in window 0) | RF-2 | none if the design follows RF-2 (it does not rely on it) |
| A2 | `-Wmissing-field-initializers` fires on the positional `struct ds_bus` initialisers once a member is added | RF-2, Pitfall 7 | a missed member would be zero-initialised silently; listing it is required either way |
| A3 | A tight wait loop costs 4 cycles (about 29,800 instructions per frame at high speed) | RF-12 | the frame budget per ROM shifts; re-measure in 05-04 |
| A4 | Accepting a d = 3 snapshot into a d = 6 instance when every counter is on the 6-grid is harmless | RF-3 | a profile-mismatched restore passes; Open question 1 |
| A5 | The kill matrix of section 7 (beyond the locked AF list) | 7 | groups found vacuous at 05-05; verified by running |
| A6 | `ds_resolve` refuses an unbound OPEN field, so the divisor string is always "3" or "6" (beam.c comment at 295-296 for the line count) | RF-3 | a third value would need a refusal in the helper |

## Open Questions (RESOLVED: 1 by D-223, 2 by D-219, 3 by D-222)

1. **Divisor and restore (RF-3). RESOLVED by D-223.** Known: D-202 has no divisor member; the grid rule uses the restoring instance's d. Unclear: whether a cross-binding restore must be refused. Recommendation: accept for Phase 5 (no layout change), name it in LED-AUD-0012's withheld text; a cached byte refused as `profile` is the alternative and changes D-202's byte count.
2. **RF-4 TRG ruling. RESOLVED by D-219.** Needs the orchestrator to adopt it as an erratum (it reconciles D-192 with D-208 and P8-6); without it `lfo_static` cannot be MAN and green at once.
3. **RF-2 carrier. RESOLVED by D-222 (plan-time ruling PR-1: the F-64 reading).** A pointer member of `struct ds_bus` is the smallest change; an out-parameter of `cpu_step_body` is equivalent. Either must reach `block_step` and `interrupt_entry`.

## Sources

### Primary (HIGH confidence)
- PSG manual page images P8-2 to P8-10 and P8-12; HuC6280 hardware manual images H8-18, H8-23, H8-24; both copies hash-checked against the recorded values.
- Repository files read this session: `05-CONTEXT.md`, `05-DISCUSSION-LOG.md`, `04-RESEARCH.md`, `04.1-REVIEW.md` (summary), `04-REVIEW-gate.md` (findings), `03-REVIEW.md` (IN-03), `.planning/{ROADMAP,REQUIREMENTS}.md`, `src/core/{ds_core.h,ds_obs.h,ds_serial.h,ds_vdc.h,bus.c,cpu.c,run.c,obs.c,beam.c,snapshot.c,boot.c,constants.def,profile_fields.def,snapshot_refusals.def,run_refusals.def}`,
  `src/text/ds_text.h`, `tests/conformance/{chunkkinds,obskinds,snapkinds,digkinds,guestkinds}.c` (lines cited), `tests/conformance/cases/video_out_rights.toml`, `fixtures/src/{spr_basic.vexp,mmu_mpr.s,cyc_probe.s,spd_speed.s}`, `fixtures/lib/TOOLS.md`, `scripts/conformance-run`, `scripts/check` (stage lines), `docs/decisions/0001-language-build.md`.
- Commands: compiled layout probe against `build/gate/libds_core.a` (`sizeof`, `offsetof`, `ds_max_step_mclk`); exact-integer recomputation of VOL, the LFSR, frame counts, crossings and LFO values; `shasum`, `pdftoppm`, tool version probes.

### Secondary (MEDIUM confidence)
- Phase 3 and 4 SUMMARY records of the gcc 13 findings (03-33, 04-02).

### Tertiary (LOW confidence)
- None used. Emulator sources and pcetech were not re-read this session; their claims stay as the CONTEXT labels them ([COMM], ASSUMED).

## Metadata

**Confidence breakdown:**
- Manual facts: HIGH - every cited literal was read on the page image this session.
- Code map: HIGH - files and lines read this session; layout and step bound probed.
- Arithmetic: HIGH - exact-integer recomputation (VOL by a 40th-power inequality, LFSR period and outputs, cadence, crossings).
- Findings: HIGH for RF-1, RF-2, RF-6, RF-9, RF-10 (traced or computed); MEDIUM for RF-12 (estimate).
- Costs: MEDIUM - the per-step fold cost is still to be measured in 05-04 (D-214).

**Research date:** 2026-11-12
**Valid until:** the first change to the files named in the integration map (line numbers move), otherwise 30 days.
