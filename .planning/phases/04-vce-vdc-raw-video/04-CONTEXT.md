# Phase 4: VCE/VDC Raw Video - Context

**Gathered:** 2026-11-11
**Status:** Ready for planning
**Method:** Autonomous discuss under the standing fan-out protocol (AGENTS.md): six parallel research advisors (A VDC register interface, VRAM and DMA; B raster timeline, scroll latch and interrupts; C background and sprite pixels;
D VCE and the raw video record; E fixtures, evidence and gate; F core integration), then one adversarial reconciliation pass that resolved about 28 cross-cluster conflicts and re-read the disputed manual pages as images.
No owner question was needed: no authority gate (licence, visibility, rights, hardware capture, publication, one-way action) is crossed by the decisions below.
**Evidence caveat (read first):** tags in this file: [MAN img] = the page was read as an IMAGE during this discuss (by the reconciler or an advisor); the recorded ER-1 read (verifier, date) is still plan 04-01's job.
[MAN txt] = OCR text only. [COMM] = pcetech.txt, PCEdev wiki or emulator source read as reading material (evidence class E4 at best; agreement between emulators is not correctness; nothing copied). [CODE] = this repository. [ASSUMED] = a project modelling choice.
Re-read as images by the reconciler: HuC6270 H7-4, H7-6, H7-7, H7-10, H7-12, H7-13, H7-14, H7-18, H7-19, H7-20 and HuC6260 H6-3. Read as images by advisors only: H7-2, H7-3, H7-5, H7-8, H7-9, H7-11, H7-15, H7-16, H7-21 to H7-25, H6-1, H6-2, H6-4, H6-5, HuC6280 H8-8, H8-24.
OCR-only so far: H7-1 (table of contents) and H7-17. The HuC6260 copy available ends at H6-5: its table of contents lists the sync generator at H6-6 and that page is not in the file, so NO page held states a line count, a line length or an active width (C2, C5).
The HuC6270 manual contains no cycle counts, no DMA speed and no access-slot timing beyond the character-cycle tables of H7-8, so every counted duration in this phase is [COMM] or [ASSUMED]. Every cite must be image-verified in plan 04-01 before any `MAN:` tag or `supported` promotion (ER-1).

<domain>
## Phase Boundary

The VCE (HuC6260) and the VDC (HuC6270, one VDC, no VPC) become real devices on the existing master-clock bus. Authored display fixtures write the registers, VRAM, SATB and palette through the Phase 3 bus; the core renders one raw record per scanline
(VCE colour values and VDC colour indices, dot-clock divisor, active window, master-clock origin, frame line count) and records interrupt timing that guest code writes into its own RAM. Every claim is checked against a literal that DuoSpool did not produce (a manual page, an in-guest computation
or an assumed ledger id); a frame or stream hash counts only when paired with such a literal that is shown to fail when the renderer is deliberately broken. Nothing here is a hardware claim.

Delivers (requirements VID-01..VID-06):

1. VCE: window decode, CR (dot clock), CTA/CTW/CTR with the 512-entry 9-bit palette, per-line dot-clock divisor latch (VID-05).
2. VDC: register file, VRAM through the port with read-ahead and increments, VRAM-to-SATB DMA, VRAM-to-VRAM DMA, status register, IRQ1 (VID-03, VID-04).
3. Timeline: a fixed 1365-master-clock line grid, vertical cycle, RCR counter, VBlank and raster interrupts at modelled positions, scroll latch rules (VID-01, VID-04).
4. Renderer: background (BAT, CG, scroll, BB/SB/burst) and sprites (SATB, slot limit, overflow, collision), line-sampled (VID-01, VID-02).
5. Raw video: a caller-owned `vid` sink, one variable-length record per line, mid-frame changes represented, never normalised to a fixed width (VID-06).
6. Evidence: twelve authored fixtures built in the gate, a video judge (ROWS, REL, STRUCT), a fault sweep, ten toy mutants, hash-role rules and one new gate stage (`video-check`).

Not in this phase: PSG (Phase 5), CD-ROM, SuperGrafx/VPC, a public API or installed layout (Phase 6), host colour conversion, scaling or windowing, a frame composer, CI fuzzing, VRAM access-slot timing, BSY, CPU stalls, the 4-clock and sprite width modes (VM/SM), burst timing,
DRAM refresh, hardware captures, a profile r2.

Eight waves (every plan ends with `sh scripts/check` green, its goldens regenerated, and a strict-C11 Linux container compile; plans inside a wave run in the listed order unless marked parallel-safe):

- **W1 Evidence and plumbing (no behaviour change).** 04-01 evidence and claim plumbing (data only, no C). 04-02 sink channel table and the `vid` sink shape with no producer. 04-01 and 04-02 are parallel-safe.
- **W2 Tracer.** 04-03: VCE palette write to a blank scanline record to the digest, through every layer, with the complete state layout and the one epoch/serial bump.
- **W3 VDC interface and judge.** 04-04 VDC register file, VRAM and the data path. 04-05 video judge, fault sweep and `video-check`. Parallel-safe.
- **W4 Timeline and DMA.** 04-06 raster timeline, status, IRQ1. 04-07 geometry and dot clock. 04-08 SATB and VRAM-VRAM DMA.
- **W5 Background.** 04-09 background renderer. 04-10 scroll latch, enables, burst.
- **W6 Checkpoint A.** 04-11. CUT LINE: if the checkpoint shows overrun, W7 becomes an inserted Phase 4.1 and 04-11 doubles as the Phase 4 close-out.
- **W7 Sprites.** 04-12 sprite decode and drawing. 04-13 slot limit, overflow, collision.
- **W8 Close-out.** 04-14.

Requirement coverage: VID-01 04-09, 04-10 (closes 04-10); VID-02 04-12, 04-13 (closes 04-13); VID-03 04-04, 04-08 (closes 04-08); VID-04 04-06, 04-08 (closes 04-06; DS and DV rows 04-08; OR and CR rows 04-13);
VID-05 04-03, 04-07 (closes 04-07); VID-06 04-02, 04-03, 04-05, 04-07, 04-10, 04-13 (closes 04-14). All six are covered. Sizing: 14 plans in 8 waves (Phase 3 had 38); numbers 04-15 and 04-16 are spare for review fixes, which are folded into the plan they fix.

</domain>

<decisions>
## Implementation Decisions

Decision ids D-135..D-181 continue Phase 3 (D-90..D-134). Ledger status is `assumed` unless a decision says "E1 candidate", which is allowed only under the evidence rules below. Reqs lists the requirement ids a decision serves. A number or rule with no tag is a [CODE] fact of this repository or an [ASSUMED] project choice recorded in the ledger entry the decision names.

### Evidence rules (ER-1..ER-9 of Phase 3 stand unchanged; four are added)

- **ER-1 Image-verified cites only.** A manual page backs a `supported` entry or a `MAN:` check only after a recorded image read (verifier, date, the numeric fact seen); `fixture-lint` rejects a `MAN:` check backing a `supported` entry unless the cite is `img`. OCR-only cites back only `assumed` entries and ASSUMED checks.
- **ER-2 Default `assumed`.** Promotion to `supported` (E1 documentary) needs ALL of: image-verified cite; literal from the page; statement worded "the DuoSpool table or behaviour equals the manual's stated value"; `withheld` listing hardware accuracy, timing and compatibility; a real `TEST:` pin that names the entry back (INV_PIN_BACKLINK).
  At most eight candidates exist in Phase 4 (D-179); expect fewer.
- **ER-3 Timing evidence is circular.** In-guest interval checks use the CPU cycle length (master/3 or /12), which is community-reported; absolute intervals are E2 and tagged ASSUMED; only ratios are assumption-free.
- **ER-4 Assertion classes.** Checks are MAN, GUEST or ASSUMED; the evidence file splits counts by class and by assertion form (ROWS, REL, STRUCT); the word `pass` is never a support claim.
- **ER-5 Self-consistency.** Fixture and core are one author's work: results are E2 at best; `irq_stim` is a test hook and says nothing about the VDC.
- **ER-6 Withheld words** are extended by D-181.
- **ER-7 Same-author oracles.** Anything re-derived from ledger text by the same author is E0/E2 and labelled so; a reference renderer or geometry oracle is NOT committed (D-171).
- **ER-8 Disagreements** go into ledger `disagreement` fields (listed per entry in D-179).
- **ER-9 Rights.** The manual PDFs come from a third-party mirror with unknown redistribution terms: never commit page images or text; cite page labels and short numeric facts only. No ROM, firmware, disc image or capture is committed.
- **ER-10 Weakest class dominates.** An expected literal that depends on any ASSUMED constant is ASSUMED even when the guest computes it; a GUEST tag is for pure in-guest arithmetic over values the guest itself wrote.
- **ER-11 A video hash is a change detector, never a verdict.** A `golden` video case is valid only if a `literal` case exists for the same scene, every literal group is shown live by the fault sweep and the toys, and the evidence text prints `video_sha256=... E0 repeatability` beside the literal verdict.
- **ER-12 Symmetry split.** Every render scene has an order-insensitive group (solid tiles, per-tile palette block, border, latch point; class MAN where a page states it) and a separate asymmetric group (pixel bit order, plane order, scroll direction, wrap, flips; class ASSUMED because the manual does not print them). One group is never both.
- **ER-13 Records are definitions.** The `vid` record fields are project definitions, not hardware claims; the runner parses records with hand-written offsets and never includes the core's record header; expected values never come from DuoSpool output, another emulator or a golden hash.

### A. Levels, grid, fold and the line ladder (VID-04, VID-05, VID-06)

- **D-135 Levels.** Every VDC/VCE claim starts with its level tag. VL0: the effect happens at the stamp of the triggering access or event and costs no time. VL1: the effect completes after a counted number of dot clocks, the count a named constant, with no VRAM-bus arbitration, no CPU stall and no BSY.
  VL2: slot-accurate fetch contention per the H7-8 character-cycle tables [MAN img: VM=0 CPU slots at 1,3,5,7 of 8 dots; VM=1,2 one slot; VM=3 none] and BSY with CPU wait states: NOT implemented, recorded `deferred` (LED-VID-0015).
  Rendering claims use R1 (all state sampled at the line event, nothing is split inside a line), R2 (a manual counting formula replaces the fetch pipeline), R3 (not modelled and flagged in the record). Ledger statements and README sentences begin with their tag. A level is a property of a claim, not a build switch. Reqs: VID-03, VID-04.
- **D-136 Grid and frame length.** Machine time stays u64 master clocks. Line k (k = 0, 1, ... from boot) starts at `raster_phase_mclk + k * line_mclk`, `line_mclk` = 1365 for every dot-clock mode and `raster_phase_mclk` = 0 [ASSUMED, community figure; 1365 = 6 x 227.5, fclock = 6 fsc is MAN img H6-3, the NTSC line of 227.5 fsc is not from a page held].
  The VCE owns the grid; the VDC is slaved (CR EX bits stored, no effect). frame = k / frame_lines and line_in_frame = k % frame_lines are derived, never stored. `frame_lines` is the bound profile field `lines_per_frame` (262|263), constant for the whole run; VCE CR bit 2 is stored and changes nothing
  (polarity disagrees between emulators: Geargrafx 263 when set, MAME dead code opposite, pcetech calls it blur). The H7-13 sum (VSW+1)+(VDS+2)+(VDW+1)+VCR [MAN img] gives 264 for VPR=$0F02, VDW=239, VCR=4: more than 262 and 263, so the model does NOT follow the sum; the VDC cycle is cut at the frame wrap (D-139).
  No fixture asserts an absolute line or frame number unless it cites ASSUMED:LED-VID-0004; fixtures use differences between events. Dot k of a line spans [k*div, (k+1)*div) master clocks; dots per line are never an integer (341.25, 455, 682.5); the sub-dot remainder is never carried. Reqs: VID-04, VID-05, VID-06.
- **D-137 Fold.** All VDC/VCE time advances through one function `ds_vdc_advance(inst, stamp, obs)`: monotone, composable (advance(a) then advance(b) equals advance(b) when no access lies between), inclusive (every event with time <= stamp is processed, in time order, before the access that carries the stamp).
  It is called (1) at the head of every read or write of hardware windows 0 and 1 and every read of $1403, after the penalty is charged and the stamp computed; (2) in the interrupt sample before the pending query, at the existing sample stamp; (3) at the end of every step with stamp machine_time_after - 1, inside a thin wrapper around `ds_cpu_step_obs`.
  Stored invariant: no event with time < machine_time is pending, i.e. next_event_time >= machine_time, from boot (the line-0 event at time 0 is processed by the first fold inside a `ds_run`, where a sink exists). `until_mclk` and the stop callback never fold; a `ds_run` that returns early does not fold; a NULL bus folds to machine_time without a log.
No `vdc_now` is stored: the next event is derived from (line_idx, stage) and the DMA fields. Pure queries (`ds_irq_pending_query`) document "valid for a folded instance". Only the fold and the register write handlers mutate beam, flag and DMA state. After a run to machine time M (M > 0) exactly
  floor((M - 1 - raster_phase_mclk) / 1365) + 1 line records exist: this closed form is an independent literal.
  Chunk invariance, restore at every stop and sink non-interference follow because every fold target is a function of the instruction stream alone. Reqs: VID-04, VID-06.
- **D-138 Line ladder and latch classes.** Per line three events, processed in time order, ties in this order: EV_LINE at origin + `line_sample_offset_mclk` (0), EV_VD at origin + vd_off, EV_RCR at origin + rcr_off (vd_off and rcr_off are computed at EV_LINE from that line's latched registers and stored; 0xFFFF = none).
  At EV_LINE, in this order: (1) apply DMA words due up to this time (D-151); (2) advance the vertical machine (D-139); (3) step the RCR counter (D-142); (4) latch: HSR, HDR, BXR, the BYR counter rule, MWR CM bit, CR SB and BB, the VCE CR divider, and at display line 0 the burst latch and the
  VRAM-DMA cancel (D-151, D-157); (5) at VBlank state entry latch MWR VM/SM/SCREEN into `mwr_active` and raise VD (phase line_start) and trigger the SATB copy;
(6) evaluate sprites (D-160); (7) render and emit the record when a sink is attached (D-155); (8) clear `line_dirty`. Latch classes: immediate = IE bits at event time, the RCR value at the compare, EX/TE/DR stored with no effect; line = item 4; state entry = VDS and VSW at their states, VDW at VDW
  entry, VCR at VCR entry [MAN img H7-9, H7-10, H7-11: "valid at the beginning of each period"];
VBlank entry = MWR [MAN img H7-8 per advisor: "valid at the beginning of a vertical blanking period"]. Tie rule [ASSUMED, same as D-111 and the timer]: an event with time e <= S precedes an access at stamp S, so a write exactly at an event instant is first seen by the next event of its class. Frame
  0 always renders with the power-on MWR (0) because the first VBlank entry comes after the first display line; fixtures judge frames after one observed VD.
  The line is sampled at EV_LINE (R1): a write, VRAM or palette change inside a line shows from the next line; `line_dirty` (D-161) says it happened. Reqs: VID-01, VID-04, VID-06.
- **D-139 Vertical cycle.** The VDC cycle restarts at frame line 0 in state VDS and runs VDS (VDS+2 lines), VDW (VDW+1 lines, the display lines), VCR (VCR lines, 0 skips), VSW (VSW+1 lines) [MAN img H7-13 Fig 2-2-1 and H7-10: N-2 back porch, N-1 display, N-1 sync], then repeats from VDS until the
  VCE frame wraps and restarts it (an unfinished state is cut). Lengths latch from the registers at the EV_LINE where each state is entered.
  A display line is a line in VDW; `display_k` counts from 0. Sprite line coordinate = display_k + 64 and first sprite dot X = 32 [MAN img H7-13 Fig 2-2-1: (32,64) is the top-left of the display screen]. If the frame wrap cuts VDW no VD is raised that frame (unmodelled, recorded in LED-IRQ-0001; pcetech forces VD at line 261, one source).
  Fixtures that assert per-frame VD, RCR or display-line counts program a vertical sum >= frame_lines (VPR=$0F02, VDW=239, VCR=4 gives 264) so the cycle never repeats inside a frame; exactly one ASSUMED fixture row covers the repeat. pcetech's "VDS+VSW" offset example disagrees with the page by two lines (14 vs 16): the page wins. Reqs: VID-04, VID-06.
- **D-140 Horizontal geometry.** One character is 8 dots [MAN img H7-13 2.3.1(a)]. Per line, measured from the line origin in dots of that line's divider: `active_x = (HDS+1)*8`, `active_w = (HDW+1)*8`, the active window ends at (active_x + active_w) * div master clocks; if that exceeds 1365 the
  window is clipped to the whole dots that fit (possibly 0), the CLIPPED flag is set and nothing is padded.
  HSW and HDE (HSR bits 4-0, HDR bits 14-8) affect only the sprite slot count (D-160) and the record. The origin choice (the end of the sync pulse; manual figure order would add (HSW+1)*8 dots) is [ASSUMED], LED-VID-0005. No fixed width exists anywhere in the core or the record: 320 is a programmed HDW, 341 and 342 are host crop figures. Reqs: VID-05, VID-06.
- **D-141 Dot clock.** The divider is a pure function of VCE CR bits 1:0 through constants `vce_div_cr0..3` = 4, 3, 2, 2: 00 -> /4 and 01 -> /3 are [MAN img H6-3] (LED-VID-0017, E1 candidate); 10 and 11 -> /2 are [COMM] (bit 1 wins in MAME, Geargrafx, Mednafen; pcetech 10 MHz) (LED-VID-0002, assumed).
  The divider is latched at EV_LINE; a CR write during line n applies from line n+1 (two emulators apply it inside the line: recorded as disagreement C11, not modelled, withheld). The record carries the divider actually used per line, so a mid-frame change is visible as differing records and `line_dirty` bit 0 marks the write.
  H7-5 warns never to change the divisor during VRAM access [MAN img]: fixtures change CR only with no VRAM access pending; the hazard is withheld, not modelled. Reqs: VID-05, VID-06.
- **D-142 RCR counter.** State `rcr_counter` (u16): at every EV_LINE it first adds 1 (saturating at 0x7FFF), then is set to 64 on the origin line chosen by model parameter `rcr_origin` {`first_active` default: the first display line; `line_before`: the last VDS line}. The manual text is ambiguous
  [MAN img H7-7: "set to 64 at the previous scanning line of start timing of display period"];
  pcetech, vdcdox and MAME read it as first_active; the two readings differ by one whole line in every split. Power-on value `rcr_counter_poweron` = 16384 (out of the 10-bit range) so nothing matches before the first origin line. At EV_RCR of EVERY line (display or not) the flag RR is raised when `rcr_counter == (RCR & 0x3FF)` and RCR.IE (CR bit 2) is set at that instant.
  EV_RCR is at offset (HDS+HDW+2)*8*div (the end of the active window); if the offset is >= 1365 the event does not occur on that line. Alternatives (MAME at HSYNC fall 1128, Mednafen 4 dots earlier, Geargrafx inside the HDW state) are named disagreements, not parameters. Ledger LED-IRQ-0010 (model row, open). Reqs: VID-04.
- **D-143 VBlank.** VD is raised on the first line after the VDW state (VBlank state entry, D-139) at an offset chosen by model parameter `vd_irq_phase` {`line_start` default: EV_LINE of that line; `hdw_start`: EV_VD at min(active_x*div, 1364)}; gated by CR bit 3 at that instant. The manual says only "a vertical blanking period has started" [MAN img H7-4];
  the placement is the C3 assumption (LED-IRQ-0001): MAME places it at the HDW start of that line, Geargrafx in the HDS event, Mednafen differently. The SATB copy and the MWR latch happen at EV_LINE of that line whatever the phase. No literal VBlank or RCR line number is ever asserted. Reqs: VID-04.
- **D-144 Status and IRQ1.** Flags are set only through `ds_vdc_flag(inst, bit)`, which applies the enable gate at the event instant: CR (bit 0) needs CR.IE bit 0; OR (1) CR bit 1; RR (2) CR bit 2; VD (5) CR bit 3; DS (3) DCR bit 0; DV (4) DCR bit 1 [MAN img H7-4: "if an interrupt enabled by the IE
  bits of CR and DCR occurs, the corresponding bit is set"; H7-6, H7-11 per advisor].
  Three emulators and the pcetech text agree; no source says otherwise; enabling later never creates a flag and disabling later never clears one. A read of the status port (reg 0 of window 0) folds to its stamp, returns `flags & 0x3F` (bit 6 BSY = constant `vdc_bsy_level` = 0, bit 7 = 0) and clears bits 0..5 at that stamp [MAN img H7-4: cleared by reading except BSY].
  The VDC IRQ1 level = (flags != 0), ORed into the IRQ1 bit in `request_bits` together with the Phase 3 `irq_stim` level; delivery stays governed by $1402 and the I flag; the VDC drives no IRQ2 and no NMI. D-112's "Phase 4 replaces it for IRQ1" is read as "stops being the only driver" (reversible); Phase 3 fixtures irq_ctl, brk_irq_sample and cpu_blk_irq keep their stimulus,
  and every video-kind case declares none (the runner asserts events = 0). `$1403` reads and the interrupt sample fold first (D-137). Ledger LED-VID-0008 (flag layout, E1 candidate) and LED-IRQ-0009. Reqs: VID-04.
- **D-145 Contention.** The only CPU/VDC contention is the Phase 3 constant LED-CPU-0017 (+1 CPU cycle per access to $1FE000-$1FE7FF, charged before the device sees the access, unchanged). Raster position never stalls the CPU; CPU VRAM accesses apply at their stamp whatever a DMA is doing; BSY reads 0; there is no peek API.
  Palette access during display repeating the last pixel colour (pcetech) is not modelled. The wiki's /BUSY-to-RDY pinout is recorded as a REPORT source on LED-CPU-0017 (no behaviour change, entry stays `assumed`). Reqs: VID-03.

### B. VDC data path (VID-03)

- **D-146 Port and registers.** Window 0 decodes `reg = offset & 3` (A2..A9 are not decoded; mirrors every 4 bytes): 0 write = AR (low 5 bits kept; $24 is $04) and read = status; 1 reads $00 and ignores writes [MAN img H7-25: high byte reads 00h]; 2 and 3 are data LSB and MSB. The window never touches `io_buffer` (D-104). There is no AR read-back.
  Registers R00..R13 are 16-bit values updated byte-wise and at once by LSB and MSB writes, then ANDed with a mask: CR $1FFF, RCR $03FF, BXR $03FF, BYR $01FF, MWR $00FF, HSR $7F1F, HDR $7F7F, VPR $FF1F, VDW $01FF, VCR $00FF, DCR $001F, the other registers full 16 bits [MAN img H7-2, H7-5..H7-11; H7-2 mislabels VCR as VDW, the text and H7-11 say VCR].
R03, R04 and AR values $14-$1F: writes ignored, reads of the data port return the read buffer byte (LSB for reg 2, MSB for reg 3) with no side effect [MAN img H7-3: "do not use AR=04"; ignore rule COMM and CODE of three emulators]. Side effects exist only for MARR MSB (read operation), VWR MSB
  (write operation), LENR MSB (VRAM DMA start), DVSSR MSB (SATB pending). Ledger LED-VID-0006 (E1 candidate for the register list, AR width, status high byte). Reqs: VID-03.
- **D-147 VRAM.** 32,768 words stored as 65,536 bytes little-endian in the instance (word w = byte 2w | byte 2w+1 << 8) so the live span equals the stream payload on every host. All VRAM access (port, both DMAs, renderer) goes through ONE bounds-checked accessor pair; reads of word addresses $8000-$FFFF return `vdc_oob_read` = 0, writes there are dropped,
and MAWR, MARR, SOUR, DESR and the DMA counters wrap modulo 65536 in explicit unsigned arithmetic [size: COMM; manual register widths allow 64K words; the out-of-range read is a named disagreement: pcetech mirror or corrupt, MAME 0, Mednafen an unwritten array; writes ignored is four-way agreement].
  SATB (256 words, 512 bytes) is written only by the SATB DMA [MAN img H7-20: direct CPU writes impossible].
  An exhaustive 65,536-address accessor sweep runs under the sanitizers. Ledger LED-VID-0007. Reqs: VID-03.
- **D-148 Write and read paths.** VWR LSB write stores the 8-bit write latch. VWR MSB write stores `(msb<<8)|latch` at word MAWR (dropped if >= $8000) and then adds 1, $20, $40 or $80 to MAWR (CR bits 12-11 = 00, 01, 10, 11, as read at that moment) [MAN img H7-7 IW table, H7-5]; the latch is never
  cleared and a second MSB write re-stores the old LSB [COMM]; MAWR increments even when the store is dropped.
  A read operation loads the 16-bit read buffer from VRAM[MARR] and then adds the IW increment to MARR; it is triggered only by an MSB write of MARR and by an MSB read of VRR (AR = $02) [MAN img H7-5, H7-25]; the CPU always receives the buffer as it was before the new operation; LSB reads and MARR LSB writes trigger nothing.
  The ordering difference between MAME and Mednafen (increment moment) is observable only if CR.IW changes between a MARR write and the first VRR read; recorded as a disagreement. All at VL0: the operation completes at the access stamp. Reqs: VID-03.
- **D-149 ST, block transfers, dev log.** ST0, ST1, ST2 reach window 0 as ordinary AR, LSB, MSB writes (A1A0 = 00, 10, 11) [MAN img H8-8, H8-24] through the existing physical bypass; `tia src,$0002,len` is the canonical upload and `tai $0002,dst,len` the readback, both through the unchanged per-byte
  bus path (a block byte touching the VDC costs 7 cycles instead of 6 under D-107).
  The `dev` sink additionally logs READS of windows 0 and 1 (kind READ, value returned, flag PENALTY; stamp = the stamp the fold used) so the runner can join read stamps to records; `$1403` and PSG reads stay unlogged; reads still never touch `io_buffer`. `dev_log_windows` and `buskinds` are amended once, in the tracer. Judged readbacks use LDA loops;
  a judged TAI readback is an ASSUMED-class check only (Phase 3 D-100 left block reads of the I/O page open). Amends LED-CPU-0012 (stub superseded), LED-CPU-0016 (ST reaches a live VDC). Reqs: VID-03.
- **D-150 SATB DMA (VL1).** DVSSR MSB write sets `satb_pending`; DVSSR LSB triggers nothing [MAN img H7-12(b): triggered by the high byte; Mednafen triggers on either byte, MAME on LSB]. At EV_LINE of the VBlank state entry, if `satb_pending` or DCR.DSR (bit 4) is set, 256 words are copied from VRAM `(DVSSR+i)&$FFFF` into SATB
  (the copy instant; `satb_pending` clears; DVSSR is not modified) and DS is scheduled at that time + `256 * vdc_dma_dots_per_word * div` master clocks (4 dots per word = 1024 dots: Mednafen and Geargrafx; pcetech says four lines; MAME 4 hsyncs: named disagreement), raised at that instant if DCR bit 0 is set then.
SATB DMA runs whatever CR.VC says; a DVSSR MSB write after the instant waits for the next VBlank [MAN img H7-12: "will start at the beginning of the following vertical blanking period"; 256 words]. The SATB DMA and the VRAM DMA do not arbitrate (VL1 has no bus model); the Mednafen/Geargrafx priority
  rule is not adopted. Ledger LED-VID-0013 (E1 candidate scoped to size, trigger byte and start). Reqs: VID-03.
- **D-151 VRAM-to-VRAM DMA (VL1).** A LENR MSB write [MAN img H7-12(a)] starts a transfer of LENR+1 words (up to 65,536; LENR = M-1) from SOUR to DESR; each address steps -1 or +1 per DCR bits 2 and 3 (SI/D, DI/D; IW never applies) [MAN img H7-11 per advisor]; one word (read, then write, sequentially so overlapping forward copies propagate) per `4 * div` master clocks,
the divider latched at the start; SOUR, DESR and LENR are live counters, LENR ends at $FFFF; MAWR, MARR, the read buffer and the write latch are never touched; the copy uses the D-147 accessor. Progress is a closed form applied at fold time (words due = (t - start) / word_mclk - done, at most
  65,536), never a per-clock loop. DV is raised at completion if DCR bit 1 is set then.
  The transfer runs only while the bus is free: `bus_free` = (state is not VDW) or `burst`; started while not free it stays pending and starts at the next free EV_LINE. At EV_LINE of the first display line (display_k = 0) the burst latch is evaluated (D-157) and, if not burst, a pending or running transfer is cancelled with no DV flag and the counters left as reached
[MAN img H7-4(e): "when a vertical display cycle starts, the block transfer between VRAMs will be cancelled"; H7-12(a): the rest is not transferred in the next period]; words due at that instant complete first (events <= time). A second LENR MSB write restarts from the live registers (the manual
  says "not normal"; unasserted). Resume-after-display (pcetech) and run-through (Geargrafx) are named disagreements.
  Fixtures that move data run with CR = 0 (burst) so they do not depend on the gating questions; exactly one shared case covers the overrun. Ledger LED-VID-0014 (E1 candidate scoped to trigger byte, M-1, cancel statement, DCR bit meanings). Reqs: VID-03.

### C. VCE (VID-05)

- **D-152 Window and registers.** The VCE decodes `offset & 7` over $1FE400-$1FE7FF [MAN img H6-2: CR write, CTA write, CTW write, CTR read on three address pins]: $0400 CR (write, low byte), $0402 CTA low, $0403 CTA high (bit 0 only), $0404 CTW/CTR low, $0405 CTW/CTR high. CR, CTA and $0401, $0406,
  $0407 read the constant `vce_unused_read` = $FF; writes to $0401, $0406, $0407 are dropped.
  None touches `io_buffer`; the D-107 penalty is unchanged. The manual lists four registers only; unlisted reads are [COMM] (pcetech, three emulators; floating bus is possible). Ledger LED-VID-0016. Reqs: VID-05.
- **D-153 Palette.** State `cta` (u16 <= $1FF) and 512 entries (u16 <= $1FF, stored as 1,024 bytes). Write $0402: cta = (cta & $100) | v. Write $0403: cta = (cta & $0FF) | ((v & 1) << 8). Write $0404: pal[cta] = (pal[cta] & $100) | v. Write $0405: pal[cta] = (pal[cta] & $0FF) | ((v & 1) << 8), then cta = (cta + 1) & $1FF.
  Read $0404: pal[cta] & $FF. Read $0405: ((pal[cta] >> 8) & 1) | `vce_data_hi_unused_or` ($FE), then cta = (cta + 1) & $1FF. CTA increments only after the HIGH byte of a write or read [MAN img H6-3, H6-5]. The table is 512 x 9 bits, G bits 8-6, R 5-3, B 2-0, VD8 = 0 background half, 1 sprite half, VD7-VD4 the block, VD3-VD0 the colour [MAN img H6-3, H6-4].
  The upper-bit read value, wrap at $1FF and independent halves are [COMM] (pcetech "hardware tested", Mednafen, Geargrafx; the manual shades the bits "not available"). The core never decodes G, R or B; no RGB exists anywhere in the core. Round-trip checks compare 9-bit masked values only. Reqs: VID-05.
- **D-154 CR.** CR stores all 8 written bits; bits 1:0 select the divider (D-141); bit 7 (BW) and bit 2 are carried in the record's `vce_cr` byte and have NO effect (BW acts on analog chroma, not on the 9-bit lookup). Power-on CR is 0 under every policy. Reqs: VID-05.

### D. Rendering (VID-01, VID-02)

- **D-155 Render rule.** The record and pixels of line k are a pure function of the machine state at EV_LINE (R1). One `const`-qualified function renders a line into stack locals and an out-parameter; it can write no machine state (enforced by a core-boundary anchor and a toy). The STATE part always
  runs: the BYR counter, sprite evaluation including per-dot sprite opacity (collision depends on it) and the OR/CR detections.
The PIXEL part (BG composition, palette lookup, record bytes) runs only when a `vid` sink is attached, so every sweep, chunk, snapshot and policy run keeps its cost. No tile or pattern cache, no line buffer in the instance or the snapshot (a stack local of about 3 KiB), no per-dot loop unless a sink
  is attached, every mutable render decision is one source statement or one named constant row (so a toy has a unique anchor).
  A record is emitted for every line of every frame (display or not). Reqs: VID-01, VID-02, VID-06.
- **D-156 Background and scroll.** BAT at VRAM word 0, row-major, W x H characters from `mwr_active` SCREEN: 000 32x32, 001 64x32, 010 128x32, 011 128x32, 100 32x64, 101 64x64, 110 128x64, 111 128x64 [MAN img H7-9 per advisor; H7-14: top-left character at address 0, +1 right, +$20 next row for 32 wide]. A BAT word = palette bank bits 15-12, tile code bits 11-0 [MAN img H7-14].
  A tile is 16 words at code*16: words 0-7 hold planes 0 and 1, words 8-15 planes 2 and 3 (CG0 = CH0+CH1, CG1 = CH2+CH3) [MAN img H7-15 per advisor]; which byte of a word is which plane and that the leftmost pixel is bit 7 are NOT printed [COMM, ASSUMED, LED-VID-0010: low byte plane 0, high byte plane 1, bit 7 leftmost].
Index = (bank << 4) | px when px != 0, else 0 [MAN img H7-18: if VD3..VD0 = 0 then VD7..VD4 = 0]. Dot i of the active window takes virtual column (bxr_line + i) mod (8W) and virtual row bg_y mod (8H), where `bxr_line` = BXR as of EV_LINE and `bg_y` is a state (u16): at EV_LINE of display line 0 bg_y
  = BYR (as of that instant), at later display lines bg_y = (bg_y + 1) mod 512,
and a write of either BYR byte sets bg_y = BYR (the written 9-bit value) at its stamp; so a write during line n shows BYR_new + 1 on line n+1 [MAN img H7-7: BXR valid "with the next scanning line"; BYR "(BYR+1)"]. The sub-line latch offset is 0 (Geargrafx latches about 12 dots in: disagreement).
  Ledger LED-VID-0009 (E1 candidate for the next-line statements) and LED-VID-0010. Reqs: VID-01.
- **D-157 Enables, burst, backdrop, border.** SB and BB are sampled at EV_LINE (manual: "from the next horizontal display period") [MAN img H7-6]. `burst` is latched at the EV_LINE of display line 0 as `!(BB || SB)` and persists for the frame (manual: SB = BB = 0 starts burst "with the next frame";
  cleared next frame; the first display line equals "next frame" unless CR is written between them);
  in a burst frame every active dot is index 256 and the record flags BURST. With BB = 0 and not burst, every dot not covered by a sprite takes `bg_off_index` = 0 (pcetech: index 0; Mednafen fills index 256 when only BB is off: named disagreement, a constant, not a parameter). Border and blank colour is index 256 = palette entry $100 [MAN img H7-18: VD8 = 1, VD7-VD0 = 0];
  outside the active window and on non-display lines the colour is the colour of index 256 sampled at EV_LINE (the record's `border`). Mid-line effects and the palette-access glitch are R3 (not modelled). Reqs: VID-01.
- **D-158 Modes.** Phase 4 renders VM = 0 and SM = 0 only. MWR is stored whole; when `mwr_active` has VM != 0 or SM != 0 the line is rendered as VM = 0 / SM = 0 and flag UNMODELLED is set in every record of the frame. One negative fixture case proves the flag. CM is stored and ignored while VM = 0;
  DR (CR bit 10) and TE are stored with no effect. Ledger LED-VID-0015 (deferred), withheld "4-clock mode", "sprite width modes". Reqs: VID-01, VID-02.
- **D-159 Sprite decode.** SATB is machine state (256 words). Sprite s uses words 4s..4s+3 [MAN img H7-20]: Y = w0 & $3FF, X = w1 & $3FF, PC = w2 & $7FF, w3: Y-flip bit 15, CGY bits 13:12, X-flip bit 11, CGX bit 8, SPBG bit 7, palette bits 3:0. Height 16, 32, 64, 64 for CGY 0, 1, 2, 3 (CGY = 2 is
  "not used" on H7-23: modelled as 3 by constant `vid_cgy2_as`; pcetech and Mednafen agree); width 16 or 32.
  Pattern base word = ((PC & ~m) >> 1) << 6 with m = (CGX ? $002 : 0) | (CGY == 1 ? $004 : CGY >= 2 ? $00C : 0) (PC bits 10:1 = VRAM address bits 15:6) [MAN img H7-21, H7-22, H7-23 per advisor]; PC bit 0 ignored with SM = 0. Cell (cx, cy) is at base + cx*$40 + cy*$80 words, plane SGk at +16k, row r at +r; plane bit k of a pixel = SGk; the leftmost pixel is bit 15 [COMM].
X-flip mirrors the whole combined sprite (the two cells of a 32-wide sprite swap), Y-flip mirrors the whole height. Sprite pixel index = 256 | (palette << 4) | px, px = 0 transparent [MAN img H7-24 per advisor]. Dot position dx = X - 32 + c (cell column c), drawn only if 0 <= dx < active_w; line
  coordinate L = display_k + 64, a sprite covers the line iff Y <= L < Y + height (plain integers). Ledger LED-VID-0011. Reqs: VID-02.
- **D-160 Slots, priority, overflow, collision.** With d = HDW+1 and e = HDE+HSW+HDS+3 (characters) taken from HSR and HDR at EV_LINE: SATB entries searched = 64 if d >= 32 else 2d+1; slots = min(16, 2*(e-2)) (0 if e <= 2) [MAN img H7-19, SM = 0; the 256-wide setup HSR $0202, HDR $031F gives e = 10,
  d = 32, 16 slots]. A covering sprite needs 1 slot (width 16) or 2 (width 32). With SB clear at EV_LINE the sprite stage produces nothing (no slots used, no flags). Entries are scanned in index order and the scan never stops early:
take it if used + need <= slots; else if need = 2 and one slot remains take only the left cell and set OVERFLOW; else set OVERFLOW and drop it [partial-half rule COMM, ASSUMED]. OVERFLOW therefore means "at least one covering sprite or half was dropped for lack of slots": a 17th 16-wide sprite sets
  it. H7-19 says "17 or more" and H7-4 says "more than 17" (both [MAN img]): H7-4 is read as the misprint; the threshold is a named disagreement; fixtures assert 16 (none) and 18 (set) as MAN-compatible and 17 as an ASSUMED row.
Off-screen sprites still take slots. Per dot the first opaque pixel in list order wins (lower SATB index first, left cell before right) whatever its SPBG bit; the winner replaces the BG dot if its SPBG = 1 or the BG dot is transparent (px = 0 or BB off) [MAN img H7-21 SPBG: 1 = over BG; H7-19(2):
  highest priority wins]. COLLISION: sprite 0 is in the taken list, its pixel is opaque and another taken entry (a different SATB index; the two cells of one sprite never collide, ASSUMED) is opaque at the same visible dot with SB on [MAN img H7-4(a)].
The detections go to `sprite_pend`, raised as status bits at EV_RCR of the displayed line through `ds_vdc_flag` (gated by CR bits 1 and 0 at that instant); never on non-display lines; a status read clears, a re-detection sets again. The flag position inside the line is not modelled (R1). The record
  carries OVERFLOW, COLLISION (ignoring IE), `slots_used` and `covering` so fixtures never depend on status timing. Ledger LED-VID-0011, LED-VID-0012. Reqs: VID-02.

### E. Raw record and sink (VID-06)

- **D-161 Record layout "DSVR v1" (`DS_VIDREC_VERSION` 1).** One record per line of every frame, in time order, no gaps, little-endian, written byte by byte. rec_bytes = 64 + ((4 * active_w + 7) & ~7). Header (64 bytes, offset type name):
  `+00 u64 origin` (master clocks); `+08 u32 frame`; `+12 u16 line` (0-based in frame); `+14 u16 frame_lines`; `+16 u16 rec_bytes`; `+18 u16 line_mclk` (1365);
  `+20 u8 div` (2, 3 or 4); `+21 u8 vce_cr`; `+22 u16 active_x` (dots, 0 when active_w = 0); `+24 u16 active_w` (dots, 0..682); `+26 u16 border` (9-bit value of palette entry $100);
  `+28 u16 active_k` (display line index, 0xFFFF none); `+30 u8 v_state` (0 VDS, 1 VDW, 2 VCR, 3 VSW);
  `+31 u8 flags` (b0 ACTIVE, b1 BURST, b2 BB, b3 SB, b4 UNMODELLED, b5 CLIPPED, b6 OVERFLOW, b7 COLLISION);
  `+32 u8 line_dirty` (b0 VCE CR, b1 HSR/HDR, b2 VPR/VDW/VCR, b3 BXR/BYR, b4 palette data, b5 CR/MWR written since the previous EV_LINE);
  `+33 u8 slots_used` (0..16); `+34 u8 covering` (0..64); `+35 u8 version` (1); `+36 u16 rcr_counter`; `+38 u16 bxr_eff`; `+40 u16 byr_row` (bg_y used); `+42 u16 hsr`; `+44 u16 hdr`; `+46 u16 vpr`;
  `+48 u16 vdw`; `+50 u16 vcr`; `+52 u16 mwr_eff` (mwr_active); `+54 u16 vdc_cr`; `+56 u64 reserved` (zero).
  Payload at +64: `u16 index[active_w]` (VD8..VD0, bits 15:9 zero), then `u16 colour[active_w]` (9-bit GRB value looked up at EV_LINE, bits 15:9 zero), then zero padding to a multiple of 8. The longest record is 64 + 2728 = 2792 bytes (`DS_VID_MAX_DOTS` = 682 = floor(1365/2), a `_Static_assert`). Non-display lines and blanked windows are header-only (all colour = `border`).
A mid-frame divider, geometry or window change shows as differing `div`, `active_x`, `active_w` between consecutive records and as `line_dirty`; nothing is normalised, no width is asserted, no RGB, no scaling, no gamma, BW is carried not applied. The layout is a project definition (LED-VID-0018),
  versioned by `DS_VIDREC_VERSION` and the per-record `version` byte. Reqs: VID-06.
- **D-162 Sink and run request.** A third caller-owned sink: `struct ds_vid_sink { uint8_t header[32]; uint8_t *bytes; uint32_t capacity; /* bytes */ }` in the private `ds_obs.h`; header written at the start and end of every accepted call and never read: +00 u32 count (records stored), +04 u32 lost, +08 u64 seq (records offered), +16 u64 bytes stored, +24 u64 zero.
Field `vid` is appended as the LAST member of `struct ds_run_request` (struct_size changes; every `struct ds_run_request` local in the repo is zeroed in plan 04-02; `run_refuse_struct_size` adapts). Policy: drop-newest in whole records (when a record does not fit, it and every later record of the
  call count as lost; no gaps), `lost != 0` is `harness_failure` for any case that uses the stream as evidence, no SINK_FULL stop reason,
  `capacity` above `DS_VID_SINK_MAX_BYTES` = 64 MiB is `run.sink_unsupported`, no record-count cap. The sink checks become table-driven over channels (insn, dev, vid; Phase 5 adds aud): alignment 8, wrap, overlap with the instance, request, result and the other sinks give `memory_invalid`, with overlap controls per channel.
  There is NO end-of-run flush rule: the step-end fold (D-137) already processes every line with time < machine_time exactly once whether or not a sink is attached; lines processed without a sink are processed all the same, so a later call never replays history. A host that must not lose lines gives capacity >= (lines due) * 2792 bytes (the closed form of D-137).
  Video file output follows the `--trace-out` rights rule: the option `--video-out ID` is refused unless the fixture manifest kind is authored or synthetic; stdout stays ids, outcomes, sizes, hashes. Reqs: VID-06.
- **D-163 Canonical stream and digest.** The video stream is the bytes `"duospool-video"` NUL, u32 `DS_VIDREC_VERSION`, then the records exactly as in D-161 concatenated across the per-call sink contents of one run; the video digest is SHA-256 of that stream, so `shasum -a 256` of a file holding the
  stream equals it. The runner (test code) builds stream and digest; the core only fills sinks.
In-process equivalence (chunking, observer matrix, restore) compares bytes with memcmp, not hashes; SHA-256 is used only where an external tool must recompute it (committed goldens, the evidence file). The stream is an output, not machine state: it never enters the state digest. A text form exists
  only in the runner (`--video-out`/evidence: one TSV row per line, pixels as run-length groups).
  The run record gets NO `[video]` table in Phase 4 (a table belongs to the Phase 6 artifact); new constants and model rows print through the generic `[constants]` and `[model_*]` tables. Reqs: VID-06.
- **D-164 Non-interference.** Machine state NEVER depends on the sink: the only places the sink pointer is read are the `want_pixels` gate and the emit call; `line_dirty` clears at every EV_LINE with or without a sink; the fold gets `obs` only to pass it to the emitter. Proof: the const render
  function (core-boundary anchor plus a toy), the observer matrix extended to the third sink on a video fixture (configs off, large, 4-record drop-newest, poisoned header, capacity 0 and 1, whole and chunked: identical verdict RAM, machine time, steps, stop sequence and digest;
  header `count = min(seq, cap)`, `lost = seq - count`, `seq` = the closed-form line count; bytes past `count` untouched), and one case that runs a video fixture with the sink on and off under all three policies and compares digests. Reqs: VID-06.

### F. State, snapshot, power-on (CORE-04..06 carried)

- **D-165 Regions and the one bump.** Seven regions are appended after SCHD, one decoder each in the two-pass walker, explicit little-endian, every member in `DS_INSTANCE_TILING` and in exactly one region (D-122): `VCE0` cr u8, cta u16; `PALT` 1,024 bytes; `VDC0` ar u8, status u8, reg[20] u16 (R00..R13 slots, R03/R04 zero), read buffer u16, write latch u8;
  `BEAM` line_idx u64 (the next line whose EV_LINE is pending), stage u8, v_state u8, v_togo u16, display_k u16, rcr_counter u16, frame_lines u16, bg_y u16, mwr_active u16, burst u8, line_dirty u8, sprite_pend u8, vd_off u16, rcr_off u16, lat_div u8, vd_pending u8, and the model selections vd_irq_phase u8, rcr_origin u8;
  `VDMA` satb_pending u8, satb_done_valid u8, satb_done_at u64, dma_state u8 (0 idle, 1 pending, 2 running), dma_start u64, dma_word_mclk u16, dma_done u32; `VRAM` 65,536 bytes; `SATB` 512 bytes. `DS_REGION_COUNT` 14 -> 21; the instance grows by about 67.2 KB (1,073,912 -> about 1.141 MB) and the snapshot from 25,021 bytes to about 92.3 KB (exact figures at the tracer).
  `DS_SERIAL_VERSION` 7 -> 8, `DS_BEHAVIOUR_EPOCH` 4 -> 5 and `DS_TEXT_FORMAT_VERSION` 6 -> 7 are bumped ONCE, in plan 04-03, with the complete layout above; later plans fill behaviour into the same layout and bump the epoch only to correct behaviour a committed pin already depends on. One recorded correction bump is allowed if a field was missed.
Regeneration procedure (mechanical, per bump): change the constants and tables; run conformance and require the red list to be exactly {digest_stream_prefix and the other digest_stream cases, the accept goldens, snapshot size pins, the `ds_snapshot.h` size mention}; for each literal, check the old
  literal equals `shasum -a 256` of the old stream, then, outside the repository and without calling the new core,
append each new frame (tag, u32 length, payload: zero bytes for zero, v1 and v2; for v3 draws from an independent splitmix64) and hash with `shasum -a 256`; extend the case comment with one sentence naming the method (Phase 3 practice); hand-write the epoch literal. Optional cascade cut: keep the
  full 64-hex literal in `digest_stream_prefix` and the two goldens and compare region payloads in the other four. Reqs: VID-06 (and CORE-05 continuity).
- **D-166 Restore validation (pass 1, nothing touched until pass 2).** AR <= 31; status <= $3F; every register inside its mask and R03, R04 zero; cta and every palette entry <= $1FF; stage <= 2; v_state <= 3; v_togo <= 262; display_k is 0xFFFF exactly when v_state is not VDW; rcr_counter <= $7FFF; frame_lines in {262,
  263}; bg_y <= 511; mwr_active inside its mask; line_dirty <= $3F; sprite_pend <= 3; model bytes below the allowed count; dma_state in {0,1,2};
  dma_done <= 65,536; and the timeline rules: line_idx <= machine_time / 1365 + 2 (checked BEFORE any multiplication), the pending event time is >= machine_time and < machine_time + 2 * 1365, vd_off and rcr_off < 1365 or 0xFFFF, satb_done_at is >= machine_time and <= machine_time + 4096 when valid, dma_start <= machine_time while running.
The window rule is a denial-of-service guard (a hostile next-event time far behind machine_time would make the next fold loop for up to 2^63/1365 iterations). VRAM and SATB bytes are any value. The hostile corpus gains a flip of every new control field, per-region perturbation of the 7 regions and
  sampled restore stops: mid-line, at a line-event tie, mid-DMA (both kinds), between the SATB trigger and VBlank entry, and inside a block transfer that targets the VDC data port.
  Hostile-input arithmetic: offsets in u32 with checked bounds (largest (127+127+2)*8*4 = 8192); loops bounded by architectural constants (DMA <= 65,536, SATB scan <= 64, dots <= 682, lines by time). Reqs: VID-06.
- **D-167 Power-on.** A new additive policy `seeded-prng-v3` (policy_version 3) = every v2 draw in the same order and with the same bytes, then new fixed-constant domains `VRAM____`, `SATB____`, `PAL_____`, each its own splitmix64 stream filled straight into the byte arrays (word w = bytes 2w, 2w+1
  little-endian; palette: 1,024 bytes drawn, then bits 1..7 of every odd byte cleared). v1 and v2 are not modified and `zero`, v1 and v2 give zero VRAM, SATB and palette.
  Every VDC and VCE register, the read buffer, the latches, status, beam and DMA state are 0 under EVERY policy (a drawn CR or IE could raise IRQ1 in Phase 3 fixtures that know nothing of the VDC); `rcr_counter_poweron` is a constant, not a draw. The default guest policy list becomes zero, v3 seed 1, v3 seed 2 (same count of runs); v2 stays covered by its own powkinds cases.
The `powerup_policy` allowed set is a core constant (the frozen r1 cannot gain a key): `zero|seeded-prng-v1|seeded-prng-v2|seeded-prng-v3`, ledger LED-PROF-0002 amended, LED-PROF-0003 amended (VDC/VCE power-on). Every video fixture initialises everything it reads and judges a window that starts
  after its own initialisation; identical per-check bytes under zero and two v3 seeds are the guard, plus one negative control that never initialises the palette. Reqs: VID-05, VID-06.
- **D-168 Constants and model rows.** Model rows (cap stays 4, D-113 unchanged: tma_zero, irq_i_sample, vd_irq_phase, rcr_origin): `DS_MODEL(vd_irq_phase, "line_start|hdw_start", "line_start", "LED-IRQ-0001", "open")` and `DS_MODEL(rcr_origin, "first_active|line_before", "first_active",
  "LED-IRQ-0010", "open")`. Their selection bytes live in BEAM from the tracer; both rows exist from 04-03 and the behaviour difference lands in 04-06 together with the case `model_param_vdc`.
Constants rows (`constants.def`, each lands with the plan that uses it, each prints in `[constants]` and joins `[withheld]` through its ledger id; the union must stay under the cap of 64): `line_mclk` 1365 (LED-VID-0001), `raster_phase_mclk` 0 and `line_sample_offset_mclk` 0 (LED-VID-0004),
  `vce_div_cr0` 4 and `vce_div_cr1` 3 (LED-VID-0017), `vce_div_cr2` 2 and `vce_div_cr3` 2 (LED-VID-0002),
  `vce_unused_read` 255 and `vce_data_hi_unused_or` 254 (LED-VID-0016), `vdc_vram_words` 32768 and `vdc_oob_read` 0 (LED-VID-0007), `vdc_bsy_level` 0 (LED-VID-0015), `rcr_counter_poweron` 16384 (LED-IRQ-0010), `vdc_dma_dots_per_word` 4 (LED-VID-0014), `bg_off_index` 0 (LED-VID-0010), `vid_cgy2_as` 3 (LED-VID-0011): 16 rows.
Manual-stated numbers are plain defines with a cite comment, not rows: 8 dots per character, 16 sprite slots, 256 SATB words, sprite origin (32, 64), `DS_VID_MAX_DOTS` 682. The profile r1 is NOT edited (D-134: a field change forces r2 anyway); `lines_per_frame` stays an OPEN field every run binds.
  Parameters considered and rejected: `bg_off_fill`, `vdc_status_when_disabled`, `vdc_dma_outside_vblank`, `rcr_irq_phase`, `spr_limit_17th`, a CR-bit-2 line-count switch, a mid-line dot-clock switch. Reqs: VID-04, VID-05.

### G. Evidence and fixtures (all VID)

- **D-169 Literal sources.** Every check is `MAN:<page>`, `GUEST:<model>` or `ASSUMED:<LED-id>` (ER-10 applies). Forbidden: any value from a DuoSpool run, another emulator, pcetech as an expected value (it is reading material and the source of ASSUMED statements), or a hash of results. Closed-form
  data in fixtures: palette `pal[i] = (i*73 + 19) & 511` (73 odd, so bijective on 0..511),
SAT word `(i * $0101) xor $5A5A`, tile rows from a one-line formula or from hand-packed `.word` tables whose ASCII art is the comment above them (no external art, no font; all 0BSD). Each fixture has a GUEST check that recomputes with different arithmetic than the writer (additive versus
  multiplicative, forward versus reverse). Test points avoid disputed boundaries (16 and 18 sprites, not 17;
  LENR+1 words with a 2-word guard band); the disputed point is an ASSUMED row naming its ledger id. `hucard.cfg` gains one optional segment `RODATA` placed in ROMB after CHECKLIT (CHECKLIT stays at file offset $1000 so the sweep offset constant is unchanged); budgets: CODE 4,096 bytes, RODATA about 3.8 KiB, 96 checks per DSV1.
  A guest measurement wider than a byte is stored as two observe rows (low and high byte) in the DSV1 observed bytes; there is no fixture-private log region (D-124 dropped it). `fixture-lint` accepts the `H7-` and `H6-` cite prefixes and the `.vexp` class tags. Reqs: all.
- **D-170 Fixtures (12 ROMs, built in the gate with ca65/ld65, zero + two v3 seeds, one `mutate_sweep` each).**

- **`vce_pal`**. Plan: 04-03. Content and literal source: CR/CTA/$0401/$0406/$0407 read $FF [ASSUMED LED-VID-0016]; entry-0 round trip by the H6-5 procedure [MAN H6-5]; CTA increments after the high byte only [MAN H6-3, H6-5]; 512-entry write then read, bijection `(i*$5B+$11)&$1FF`, catches 8-bit
  CTA aliasing [GUEST]; CTR high bits 7-1 read 1, wrap, halves, window mirror $0408/$0444, TIA write then LDA readback [ASSUMED]; runner `video` literals: record i origin = i*1365, div 4/3 from CR [MAN H6-3], dots from div [DERIVED], border = the palette word the guest wrote at $100 [GUEST]. About
  28 checks.
- **`vdc_reg`**. Plan: 04-04. Content and literal source: Mirror sweep $0000/$0004/$03FC, AR 5 bits seen through VRAM effects, $0001 reads $00 [MAN H7-25], IW +1/+$20/+$40/+$80 landing addresses [MAN H7-7], MARR/VRR read-ahead [MAN H7-5, H7-25], write-latch persistence, OOB, reserved registers
  [ASSUMED LED-VID-0006, -0007], ST triple equals sta triple also with MPR0 mapped to RAM [MAN H8-8], TIA upload of 32 words and TAI readback [GUEST], runner kind `vdc_vram` comparing `inst` VRAM words with literals restated from the source. About 36 checks.
- **`vdc_stat`**. Plan: 04-06 (+04-08, 04-13). Content and literal source: Flag gating per flag: event with the enable clear leaves status 0 [MAN H7-4], enable set shows the bit, read clears, second read 0, $1403 bit 1 follows the level [ASSUMED LED-IRQ-0009], bits 6 and 7 read 0; RR, VD in 04-06, DS, DV in 04-08, OR, CR in 04-13. About 16 checks.
- **`vdc_irq`**. Plan: 04-06. Content and literal source: Spin-counter measurements (D-174); variants `-D LINES=262` and `263`. About 30 checks.
- **`vce_dot`**. Plan: 04-07. Content and literal source: CR 00 and 01 [MAN H6-3], 02 and 03 [ASSUMED LED-VID-0002], static frames and a mid-frame change at a counted instant; runner literals div, line_mclk, active_x and active_w from the programmed HDS/HDW, border; the write stamp from the `dev` log joined to record origins.
- **`vdc_dma`**. Plan: 04-08. Content and literal source: Burst copy of 64 words (ROM table and runner literals), LENR 0 moves one word [MAN H7-12], SI/D and DI/D reverse [MAN H7-11], DV gating and clear-on-read, IW has no effect, forward overlap, MAWR/MARR/read buffer untouched, retrigger with only
  LENR MSB, duration as an observe row [ASSUMED VL1], the overrun case (first words moved, last word not, DV never set; no count). About 18 checks.
- **`vdc_satb`**. Plan: 04-08. Content and literal source: 256-word pattern, DVSSR MSB trigger, DS after VD, DSR repeat, DVSSR LSB-only leaves SATB unchanged for a frame [MAN H7-12]; runner kind `vdc_satb` compares `inst` SATB to the pattern. About 10 checks.
- **`bg_basic`**. Plan: 04-09. Content and literal source: 32x32 BAT, banks 0, 3, 15, a bank-colour-0 tile, a 1-pixel diagonal; solid group [MAN H7-14, H7-18], asymmetric group [ASSUMED LED-VID-0010]; ROWS literals by hand.
- **`bg_scroll`**. Plan: 04-10. Content and literal source: BXR/BYR written at counted offsets (>= 250 master clocks from both line boundaries), 64x64 wrap (BXR 500, BYR 300), next-line latch [MAN H7-7], direction and wrap [ASSUMED]; REL scroll shifts.
- **`bg_enab`**. Plan: 04-10. Content and literal source: BB/SB matrix, burst frame, mid-frame BB toggle, `bg_off_index` row [ASSUMED LED-VID-0010].
- **`spr_basic`**. Plan: 04-12. Content and literal source: Both widths, three heights, flips, bank, SPBG over opaque and transparent BG, sprite-over-sprite order, pixel 0 transparent, partial clipping; data arrives through SATB DMA (the second witness of D-150).
- **`spr_limit`**. Plan: 04-13. Content and literal source: 16 slots (8 x 32-wide; 16 x 16-wide) no OR; 18 sets OR [MAN H7-19]; 17 [ASSUMED]; narrow display d = 28 searches 57 entries; short blanking e = 8 gives 12 slots [MAN H7-19]; collision cases [MAN H7-4(a)] and the ASSUMED ones; IE gating.

Two ROMs may share a source with `-D` variants (precedent: mmu_mpr). Rights: all sources are authored 0BSD; no copied art. Reqs: all.
- **D-171 The video judge.** Case kind `video`, assertion forms with a class tag per row: `ROWS` (selector `line=active+K x=X0..X1`, literal = a palette ADDRESS list for the structure step or a VALUE list for the palette step; the record carries both index and value so the steps are separable),
  `REL` (a relation id from a closed list: scroll_x_shift, scroll_y_shift, palette_subst, blank_is_border, dot_width_scale; two named runs),
`STRUCT` (records per frame; origins strictly increasing by 1365 with constant frame_lines; origin = (frame*frame_lines + line)*1365 and the record count, both from separate runner functions; every value and index < 512; the display-line count equals min(VDW+1, frame_lines - (VDS+2)); `div` equals
  the table of D-141; the set of widths over a mid-frame run has more than one element).
Data lives in `fixtures/src/NAME.vexp` (hand-derived rows with the derivation in the row comment; at most 512 rows, 400 characters per line, 16 KiB per file, fail-closed on unknown keys, parsed by the awk TOML-subset machinery or a new 40-line TSV reader). The write-to-line join for latch tests uses
  the write's stamp from the `dev` sink and the line origins carried in the records, never the core's tables.
  NO reference renderer, geometry oracle, vid-lint stage or vdata packer is committed (ER-7): a throwaway script may draw up a table, but only the committed literal is judged. Non-vacuity lint: a ROWS selection must contain at least 3 distinct indices and at least one transition per selected line set; an all-border or one-colour expectation is refused. Reqs: VID-06.
- **D-172 Vacuity controls and the stage.** Case roles `literal`, `struct`, `golden`: a `golden` case (one pinned stream digest, E0) is valid only if a `literal` case exists for the same scene (code `INV_VIDEO_HASH_ONLY`, runtime refusal `video-hash-only` as `harness_failure`). Record-fault sweep
  (kind `video_fault_sweep`): run once, then apply each fault of a closed list to the captured records in memory
(F0 perturb one of the group's own literals, F1 shift pixels +1 in x, F2 swap two bits of an index, F3 drop the first active line, F4 duplicate a line, F5 change the divider, F6 widen the active window by one dot, F7 zero every non-zero index, F8 swap two adjacent lines, F9 move an origin by one
  clock) and require the judge to flip exactly the groups that declare the fault in `kills=`;
  a group no declared fault flips is `VIDEO_GROUP_VACUOUS`; an empty `kills=` is `INV_VIDEO_GROUP_UNKILLED`. A toy EXPECT lists at least one `literal` case (`INV_TOY_NO_LITERAL`); every `.vexp` scene is named by a toy EXPECT or by `tests/conformance/VIDEO_TOY_EXEMPT` (one line with a reason; the count is printed in the verification report).
A null-renderer control (a run that emits no records, or all border, or all index 0) is red; a missing sink is `harness_failure`, never a pass. One in-process case `video_hash_not_verdict` hashes a faulted record stream, shows it passes its own golden and fails the literal. ONE new gate stage
  `video-check` (static: `.vexp` grammar, class lint, role and kill rules, with negative mini-roots under `tests/video/`); the selftest gains three mutations
  (remove INV_VIDEO_HASH_ONLY, remove VIDEO_GROUP_VACUOUS, remove the `.vexp` class lint), each turning the gate red. Reqs: VID-06 (success criterion 5).
- **D-173 Toy mutants, cap 10 for the phase** (generated at control time by `scripts/control-mutate` from live sources, one anchored statement each, fail-closed when the anchor is missing; each EXPECT names at least one literal-role case): `vid-palette-mask` (palette store keeps 8 bits; 04-03),
  `vid-emit-mutates` (the emitter writes state; 04-03), `fold-skipped-at-step-end` (04-03; caught by chunk_equiv at line +-1 and the observer matrix),
`vrr-no-readahead` (04-04), `vid-rcr-base` (counter base 64 -> 63; 04-06; the ratio group stays green, the offset group goes red), `status-read-keeps-flags` (04-06), `vid-dot-div` (divider table 4 and 3 swapped; 04-07), `vid-bit-order` (pixel bit order reversed; 04-09; the order-insensitive MAN
  group MUST stay green, the proof of the symmetry split), `vid-scroll-latch` (scroll applied to the current line; 04-10), `vid-spr-limit` (limit 16 -> 17; 04-13).
  A VRAM-DMA control (instant transfer) is a fixture-level expected-red check, not a toy. Cost about 10 x 2.4 s CPU serial, parallel on the gate; the shared-archive lever (D-176) is available. Reqs: all.
- **D-174 Guest-recorded interrupt measurements (criterion 3).** The VDC has no readable scanline counter, so the guest measures time with a fixed-cost foreground loop (no data-dependent branch; a 24-bit add through clc/adc; P <= 12 cycles at high speed, cost from the manual cycle table [MAN img
  H8-25: 3 cycles]) and a handler that samples the counter before reading status; handlers store counter bytes and the status byte into DSV1 observed bytes (two rows per 16-bit value).
Checks: (1) RCR=a+8 follows RCR=a by 8 times the RCR=a to a+1 interval within one loop iteration [GUEST, no assumed constant, MAN H7-7 for the increment-per-scan statement]; (2) the line period (RCR 100 lines apart) and the frame period over 4 frames against 1365 and N*1365 [ASSUMED LED-VID-0001,
  ER-3]; (3) VD falls after the RCR of the last display line and no later than one line after the next [weak ordering, MAN-derived H7-4, H7-7];
(4) the RCR(last display line) to VD gap and the RCR=64+k hit line, which separate `vd_irq_phase` and `rcr_origin` values (245, 341 and 1610 master clocks in the HDS=2, HDW=31, div 4 setup) [ASSUMED LED-IRQ-0001, -0010; tolerance +-1.5 iterations]; (5) status and level semantics and a no-ack
  re-entry count [MAN H7-4, ASSUMED LED-IRQ-0004 level]. The recorded "scanline" is the programmed RCR value and the "phase" is iterations converted by a printed formula; no absolute line is observed.
  Runner kind `vdc_irq_agree` (labelled E0/E2, hardware claim: none) checks that the guest measurements converted to master clocks bracket the model's assert stamp using the `dev` read stamps. Fixtures program HDS, HDW and div themselves; expected values come from those programmed values. Reqs: VID-04.
- **D-175 Data-moved evidence.** VRAM write and read, the ST and TIA paths and VRAM-VRAM DMA are verified by (b) guest readback through VRR against a ROM table computed differently (GUEST) AND (c) runner kinds `vdc_vram` and `vdc_satb` that read `inst` VRAM and SATB bytes (private header, the D-108
  precedent) against literals restated from the fixture source, bypassing the port read path so a symmetric write/read bug cannot cancel.
SATB is not CPU-readable, so (c) is its primary check and a visible sprite is the second witness (04-12). C-level cases without a guest: `vdc_vram_sweep` (65,536 addresses, sanitizers), `vdc_regmask`, `vdc_advance_pure` (a naive per-dot DMA reference against the closed form over random stamp
  partitions, the timer precedent), `vdc_fold` (random chunk sequences against one jump, accesses at event time +-1, the next-event invariant), `vdc_snapshot_mid_dma`;
  the 32-program random differential gains VDC/VCE traffic, plus `vdc_random_regs` (32 random register programs, E0, sanitizers: no crash, no out-of-range index, `lost == 0`, termination). Reqs: VID-03.

### H. Gate, CI, process

- **D-176 Gate and cost.** One new stage, `video-check` (57 stages). Estimate (to be re-measured by plan 04-03 and at 04-11): about 60-70 new REQUIRED ids (12 ROM ids, 12 `mutate_sweep` ids, about 12 video judge cases, 4 fault sweeps, 3 golden or struct cases, `video_hash_not_verdict`, 5
  chunk_equiv, 3 snapshot, 1 observer, 1 agree, `vdc_random_regs`, `vdc_poweron_policy_diff`, 4 sink-shape or refusal cases, 7 unit cases, 2 geometry pins).
Added cost: local gate +35 s central (range +20 to +60), `conformance` 33 s -> about 47 s central (60 s high), CI critical path +55 s central (range +30 to +95) on today's 342-541 s; emulation is cheap (about 1-2 ms per million master clocks) and the cost is spawn-, parse- and compile-bound; BG
  pixels are skipped without a sink (D-155); video chunk schedules are 7, 1364, 1366, 3072 and one frame (schedule 1 dropped for video).
Staging triggers (recorded and applied by the efficiency review, never by failing the gate): if the CI job exceeds 600 s or `conformance` exceeds 60 s, apply in order, each without weakening evidence: (1) link the toy executables against a shared archive of the runner objects and recompile only the
  mutated core unit (CMake only; cuts toy cost by about two thirds); (2) run video fixtures under two policies (zero and one v3 seed) while CPU fixtures keep three,
covered by `vdc_poweron_policy_diff`; (3) drop one chunk schedule from the video cases; (4) as a last resort move the Linux container run and `vdc_random_regs` to a scheduled job. `timeout-minutes` stays 20. The second awk run stays a whole-gate run. After the first CI run of Phase 4 the standing
  efficiency review is applied and recorded in `docs/ci-timings.md` with the per-stage times of 04-03, 04-11 and 04-14. Nothing is weakened to save seconds.
- **D-177 Per-plan process rules.** (a) Every plan that adds or edits C runs `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` (gcc 13, `-std=c11 -pedantic -Werror`, about 80 s emulated) before its closing commit; each wave end runs `gate-gcc gate-clang core-boundary-gcc`; plans 04-11 and 04-14
  run all six parts once; an unavailable container is recorded as "unverified", never as passed.
Footguns in new C: no POSIX calls (`strtok_r`, `strdup`, `getline`, `strcasecmp`), every `snprintf` into a fixed buffer bounds its inputs where the compiler can see them (precision `%.Ns`; `-Wformat-truncation` bit Phase 3 twice), explicit casts and masks before narrowing in 9-bit and 16-bit palette
  arithmetic, unsigned widening before shifts (the sanitizer preset has no `-fwrapv`), no arithmetic on null sink pointers, table-driven record writers with `_Static_assert` on sizes.
(b) Core rules (D-90 onward): no malloc, no file I/O, no clocks, no env, no floats, no mutable globals or function statics (const tables only), no default-order containers, defined behaviour only, new files `src/core/{vdc.c, vce.c, beam.c, vid.c, ds_vdc.h}` (names are discretionary) include only
  stddef/stdint/ds_core.h, the `core-boundary.allow` list is unchanged. (c) No machine-local path (home, temp or working-copy) in any committed file; no line over 400 characters (the ledger schema also demands ASCII and 400); SPDX headers (code MIT OR Apache-2.0; fixtures, ledger, docs 0BSD).
(d) Every commit ends with the standing Co-Authored-By trailer of the executing agent; atomic commits; at most 6 tasks per plan; no plan exists only to add a ledger id, a cite, a toy or a doc except 04-01 (data) and 04-02 (plumbing). (e) The full gate runs in `sh`, `dash` and under the second awk at
  04-11 and 04-14. (f) No new third-party dependency; WLA-DX stays optional verify-only and is not run (no new mnemonics expected). (g) No ROM-like file, manual page text or image, or GPL material is committed (`rights-check`, `licence-check` extended to `.vexp`).
- **D-178 Phase 3 cascade (planned, not accidental).** In 04-02: `run_refuse_struct_size`, every request local zeroed, the observer kind's third channel. In 04-03: `mmu_mpr` checks 25 and 26 (VCE reads $FF) rewritten to empty-window and register-readback literals (checks 27 and 28, VDC/VCE reads
  leave `io_buffer`, stay valid); `DS_VCE_STUB_READ` removed; `dev_log_windows` and `buskinds` (reads of windows 0 and 1 now logged); the five `digest_stream_*` cases and the two goldens recomputed (D-165);
`guestkinds` default policies; `powkinds`, `snapkinds`, `field_flip`, `digest_region` and tiling for the new members; the text record goldens (format 7); `ds_snapshot.h` size text. In 04-04: `mmu_mpr` checks 23 and 24 (VDC reads $00) rewritten, `DS_VDC_STUB_READ` removed; `st_bypass`, `cpu_huc`,
  `cyc_probe`, `spd_speed` and `mmu_mpr` writes to the VDC window now reach real registers (reset values are zero and IE bits 0, so no flag or IRQ arises): each is verified to pass and to read no VDC data, and all are re-run under v3.
  LED-CPU-0012, -0016, -0017, LED-IRQ-0008, LED-PROF-0002, -0003 are amended in 04-01.
- **D-179 Ledger (written once, in plan 04-01; ids final; areas VID and IRQ only: VDC is not an allowed area).** All new entries are `assumed` (0015 is `deferred`), `profile = "pce-hucard-2btn"`, `pinning_test = "PENDING:4"` until a closing plan sets `TEST:`, `revisit_phase = 4`, and each has a
  `ledger/REQUIRED` line `ID 04-CONTEXT D-nnn` (the decision in the table). Later plans edit pins and statuses and may amend a statement only to correct it.

- **LED-VID-0004**. Statement (level tag first): VL0/R1: the VCE/VDC timeline is a fixed grid of 1365 master clocks per line from machine time 0, frame 0 line 0 at time 0; devices fold to each access stamp; line content is the state at the line event; a write at time < event is visible to that line,
  a write at the event goes to the next. Sources and disagreement: FACT H6-3 (fclock 6 fsc); REPORT pcetech, MAME header, Geargrafx (1365); UNKNOWN H6-6 absent. Disagreement: two emulators change the dot clock inside a line (C11); power-on phase unknown. Withheld words: raster-accurate,
  dot-accurate, any sub-line timing, any absolute line or phase claim. Dec: D-136.
- **LED-VID-0005**. Statement (level tag first): R1: vertical cycle order VDS+2, VDW+1, VCR, VSW+1 restarted at each frame start and repeated; characters of 8 dots; active window (HDS+1)*8 dots start, (HDW+1)*8 wide, clipped at the line end; no fixed width; frame length is the profile field, the
  H7-13 sum is not followed. Sources and disagreement: FACT H7-10, H7-13. Disagreement: pcetech VDS+VSW offset; window origin options (manual figure order adds (HSW+1)*8); 320 versus 341 are host crops. Withheld words: visible area, overscan, frame is W x H. Dec: D-139.
- **LED-VID-0006**. Statement (level tag first): VL0: the VDC port decodes offset & 3; AR is 5 bits; registers, masks, VWR/VRR paths, write latch, read-ahead and IW +1/+$20/+$40/+$80 as in D-146, D-148 [E1 candidate scoped to page-stated facts]. Sources and disagreement: FACT H7-2, H7-3, H7-5, H7-7,
  H7-25; REPORT pcetech. Disagreement: increment moment (MAME versus Mednafen), MARR LSB trigger (MAME). Withheld words: hardware behaviour of reserved registers. Dec: D-146.
- **LED-VID-0007**. Statement (level tag first): VL0: VRAM is 32,768 words, reads at $8000-$FFFF return 0, writes dropped; power-on contents by policy (v3 draws), registers zero. Sources and disagreement: REPORT pcetech. Disagreement: out-of-range read (mirror or corrupt, 0, unwritten array). Withheld words: VRAM size, VRAM mirroring, power-on contents. Dec: D-147.
- **LED-VID-0008**. Statement (level tag first): VL0: status layout CR, OR, RR, DS, DV, VD, BSY; a flag is set only when its enable is set at the event instant; read clears 0..5; BSY 0 [E1 candidate for layout and clear-on-read]. Sources and disagreement: FACT H7-4, H7-6, H7-11; CODE three emulators agree on set-if-enabled. Withheld words: interrupt latency, BSY. Dec: D-144.
- **LED-VID-0009**. Statement (level tag first): R1: BXR as of the line start; BYR first display line = BYR, a write makes the next line BYR_new+1; MWR SCREEN valid at VBlank entry, CM next line [E1 candidate for the next-line statements]. Sources and disagreement: FACT H7-7, H7-8. Disagreement:
  sub-line latch offset (Geargrafx about 12 dots). Withheld words: latch position inside the line, scroll-accurate. Dec: D-156.
- **LED-VID-0010**. Statement (level tag first): R1/R2: BG chain (BAT bank/code, CG0/CG1 planes), index rules, border 256, burst latch at display line 0, `bg_off_index` 0; bit order bit 7 leftmost and plane order are COMMUNITY. Sources and disagreement: FACT H7-9, H7-14, H7-15, H7-18, H7-6; REPORT
  pcetech. Disagreement: backdrop when BB = 0 (index 0 versus 256), burst latch instant. Withheld words: tile appearance, backdrop colour, burst timing. Dec: D-157.
- **LED-VID-0011**. Statement (level tag first): R2: sprite decode, pattern addressing, CGY 2 as 3, coordinates (32,64), slots = min(16, 2(e-2)), search 64 or 2d+1 [E1 candidate scoped to origin, 16 per line, the H7-19 table]. Sources and disagreement: FACT H7-13, H7-19, H7-20..H7-24; REPORT
  pcetech. Disagreement: CGY 2, partial-half rule. Withheld words: sprite pattern addressing for CGY 2, sprite width modes. Dec: D-159.
- **LED-VID-0012**. Statement (level tag first): R1: OVERFLOW iff a covering sprite or half is dropped for lack of slots (a 17th sets it); COLLISION rule for sprite 0; flags raised at the end of the active window of the displayed line. Sources and disagreement: FACT H7-4, H7-19. Disagreement: H7-4
  "more than 17" versus H7-19 "17 or more"; transparent-pixel collisions. Withheld words: overflow timing, collision timing, sprite-accurate. Dec: D-160.
- **LED-VID-0013**. Statement (level tag first): VL1: SATB DMA, 256 words, trigger DVSSR MSB, auto-repeat DCR.DSR, copy at VBlank entry, DS at 1024 dots x divider after [E1 candidate scoped to size, trigger, start]. Sources and disagreement: FACT H7-11, H7-12; CODE Mednafen, Geargrafx. Disagreement:
  duration (four lines, 4 hsyncs), trigger byte. Withheld words: any SATB DMA duration claim. Dec: D-150.
- **LED-VID-0014**. Statement (level tag first): VL1: VRAM-VRAM DMA, LENR+1 words, +-1 steps, live counters, 4 dots per word, cancelled at display start with no DV, pending until the bus is free [E1 candidate scoped to trigger, M-1, cancel statement, DCR bits]. Sources and disagreement: FACT H7-4,
  H7-11, H7-12; CODE. Disagreement: overrun (cancel, resume, run-through, instant). Withheld words: any DMA speed claim, DMA-accurate. Dec: D-151.
- **LED-VID-0015**. Statement (level tag first): `deferred`: VL2 access slots, BSY, CPU stalls, VM/SM modes (R3), burst timing, refresh, CM, DR, TE, palette-access artefacts not modelled. Sources and disagreement: FACT H7-8 tables; REPORT wiki /BUSY-RDY. Withheld words: VRAM access timing, 4-clock mode, sprite width modes, BSY. Dec: D-135.
- **LED-VID-0016**. Statement (level tag first): VL0: VCE window decode, registers, 512 x 9-bit table, CTA increments after the high byte, unlisted reads $FF, CTR high byte ORed with $FE, wrap, independent halves, CR bits 7 and 2 no effect [E1 candidate scoped to H6-2..H6-5 facts]. Sources and
  disagreement: FACT H6-2, H6-3, H6-4, H6-5; REPORT pcetech. Disagreement: manual shades the bits, pcetech tested, floating bus. Withheld words: upper-bit reads, colour accuracy. Dec: D-152.
- **LED-VID-0017**. Statement (level tag first): DuoSpool's CR table maps content 00h to master/4 and 01h to master/3, equal to HuC6260 H6-3 [E1 candidate]. Sources and disagreement: FACT H6-3. Withheld words: any other CR value, hardware-verified. Dec: D-141.
- **LED-VID-0018**. Statement (level tag first): The `vid` record (DSVR v1) is a project definition: 9-bit values and indices before colour conversion, one record per line, header fields of D-161; it asserts no width; a hash of it is E0. Sources and disagreement: none (definition). Withheld words: aspect, overscan, visible area, colour accuracy, frame size. Dec: D-161.
- **LED-IRQ-0009**. Statement (level tag first): VL0: IRQ1 level = (VDC flags != 0) ORed with the declared stimulus; status read clears; level, not edge; the VDC drives no IRQ2 or NMI. Sources and disagreement: FACT H7-4; CODE. Withheld words: interrupt latency, IRQ line timing. Dec: D-144.
- **LED-IRQ-0010**. Statement (level tag first): R1: RCR counter 64 on the first display line (model `rcr_origin`), persistent, power-on sentinel, 10-bit compare at every line, RR at the end of the active window (dropped if beyond the line). Sources and disagreement: FACT H7-7 (ambiguous wording
  quoted); REPORT pcetech, vdcdox. Disagreement: first_active versus line_before; RR instant (MAME 1128, Mednafen -4 dots, Geargrafx). Withheld words: any RCR line claim, raster-accurate. Dec: D-142.

Amendments: LED-VID-0001 (frame length is constant for the run; "power-on value" clause removed; CR bit 2 polarity disagreement; H7-13 sum not followed; 1365 derivation; pin `TEST:vid_frame_lines`); LED-VID-0002 (narrowed to the assumed encodings 10 and 11 and the per-line latch; first-hand H6-3
source replaces the second-hand one; the "internally inconsistent" pcetech note to be re-checked and reworded; pin `TEST:vce_dot`);
LED-VID-0003 (no fixed width; (HDW+1)*8 dots from the registers; 320/341 are host crops; pin `TEST:vid_active_window`); LED-IRQ-0001 (placeholder rules: VD at `vd_irq_phase`, no forced VD, RR at the end of the active window; alternatives named; pin `TEST:vdc_irq`; still `assumed`); LED-IRQ-0008 (the
IRQ1 stimulus ORs with the VDC level); LED-CPU-0012 (stub clause superseded); LED-CPU-0016 (ST reaches a live VDC); LED-CPU-0017 (+/BUSY-RDY REPORT source; stays assumed);
LED-PROF-0002 (v3 domains), LED-PROF-0003 (VDC/VCE power-on). Pins are mutual (the case names the id in `pins`). E1 candidates, at most eight: LED-VID-0006, -0008, -0009, -0011, -0013, -0014, -0016, -0017; promoted only by ER-2 in a closing plan (04-11 or 04-14); the rest stay assumed, deferred or
open. Image-verify in 04-01, before any `MAN:` tag: H7-2 to H7-16, H7-18 to H7-25, H6-1 to H6-5, H8-8, H8-24 (H7-17 and H7-1 are not used by any decision);
the executor records verifier and date per page and adds `H7-` and `H6-` legend lines to `cites.txt` (label n = PDF page n+3 for the HuC6270 manual, n+2 for the HuC6260 manual); `TOOLS.md` records both manuals (URL in the MiSTer docs folder, byte size, sha256: HuC6270 copy 1,497,291 bytes, sha256
2bb87866b403632bf1ba1a6801ba82ba6f4a13f15e46835500e368e8da5ac6ab; HuC6260 copy 315,186 bytes, sha256 c6f0d83a75898f3c1513ee5c152e9ce82bac689978e338974a59b9c4db39bd92, ends at H6-5). Reqs: all.
- **D-180 Plans and the cut line.**

- **04-01**. Wave: W1. Content: Evidence plumbing, data only: image-verify and record the cite pages, TOOLS.md, 17 ledger entries and 10 amendments with `PENDING:4` pins, REQUIRED lines, docs-check claim-word extension with its negative control, `fixture-lint` prefixes. No C, no gate cost worth naming. Reqs: VID-01..06 (evidence).
- **04-02**. Wave: W1. Content: Sink channel table (insn, dev, vid), `struct ds_vid_sink` and the appended request field with no producer, every request local zeroed, `ds_vdc_advance` skeleton (empty) at the three fold points, struct_size case, sink-shape and refusal controls; digests unchanged. May fold into 04-03 only if the diff is under about 200 lines. Reqs: VID-06.
- **04-03**. Wave: W2. Content: TRACER (the first vertical slice and the first plan that changes behaviour; the cascade slice, merge nothing into it): complete layout, serial 8, epoch 5, text 7, v3, VCE window/CR/palette, 1365 grid with frame wrap, blank-line records, step-end fold, dev read
  logging, `video` kind (minimal), `vce_pal`, `vdc_fold`, chunk_equiv, observer, snapshot and hostile corpus over 21 regions, goldens, toys `vid-palette-mask`, `vid-emit-mutates`, `fold-skipped-at-step-end`, measurement spike for D-176. Reqs: VID-05, VID-06.
- **04-04**. Wave: W3. Content: VDC window real: registers, masks, VRAM, read-ahead, increments, status read skeleton, `vdc_reg`, `vdc_vram`, vram sweep, regmask, mmu_mpr 23 and 24, toy `vrr-no-readahead`. Reqs: VID-03.
- **04-05**. Wave: W3. Content: Video judge: ROWS/REL/STRUCT, `.vexp` grammar, fault sweep, `video-check` stage with negative mini-roots, null-renderer and missing-sink controls, golden role, `video_hash_not_verdict`, selftest mutations. Reqs: VID-06.
- **04-06**. Wave: W4. Content: Vertical cycle, ladder, status flags, IRQ1 OR, fold before sample, `rcr_origin` and `vd_irq_phase` behaviour with `model_param_vdc`, `vdc_stat` (RR, VD), `vdc_irq`, `vdc_irq_agree`, toys `vid-rcr-base`, `status-read-keeps-flags`. Reqs: VID-04.
- **04-07**. Wave: W4. Content: Horizontal geometry, per-line divider latch, clipping, `line_dirty`, mid-frame dot-clock change, `vce_dot`, `vid_frame_lines`, `vid_active_window` pins, toy `vid-dot-div`. Reqs: VID-05, VID-06.
- **04-08**. Wave: W4. Content: SATB and VRAM-VRAM DMA, closed-form progress, `bus_free`, burst latch, DS and DV flags, `vdc_dma`, `vdc_satb`, `vdc_satb` runner kind, `vdc_advance_pure`, `vdc_snapshot_mid_dma`, DS/DV rows of `vdc_stat`. Reqs: VID-03, VID-04.
- **04-09**. Wave: W5. Content: Background renderer: BAT/CG decode, palette lookup, index rules, record pixels, `bg_basic`, toy `vid-bit-order`. Reqs: VID-01.
- **04-10**. Wave: W5. Content: Scroll latch, BYR counter, BB/SB, burst, backdrop, 64x64 wrap, `bg_scroll`, `bg_enab`, toy `vid-scroll-latch`, chunk_equiv and snapshot on video. Reqs: VID-01, VID-06.
- **04-11**. Wave: W6. Content: Checkpoint A (same gate, recorded): all six Linux parts, CI-cost measurement against D-176, ledger promotion pass for the covered entries, goldens, `vdc_random_regs`, `vdc_poweron_policy_diff`, verification note; the CUT LINE decision. Reqs: all so far.
- **04-12**. Wave: W7. Content: Sprite decode, pattern addressing, drawing, SPBG priority, `spr_basic` (second SATB witness). Reqs: VID-02, VID-06.
- **04-13**. Wave: W7. Content: Slots, overflow, collision, flag raising at EV_RCR, OR/CR rows of `vdc_stat`, `spr_limit`, toy `vid-spr-limit`. Reqs: VID-02, VID-04.
- **04-14**. Wave: W8. Content: Close-out: gate and CI efficiency review, README (levels, honest wording), ledger statuses and pins, E1 promotions under ER-2, all six Linux parts, `sh`/`dash`/second awk, verification report, hand-off list. Reqs: VID-01..06.

Cut line: take it at 04-11 if any of: more than two plans needed a revision plan, the warm quiet gate exceeds 150 s, the CI job exceeds 600 s and lever (1) of D-176 does not recover it, or the owner prefers an earlier pull request. Then 04-11 is the Phase 4 close-out for VID-01, 03, 04, 05, 06 (BG);
04-12..04-14 become plans 4.1-01..4.1-03 of an inserted Phase 4.1 that closes VID-02 and the sprite rows of VID-04 and VID-06.
The cut is clean because sprites touch only the renderer, SATB data already moves in 04-08, and no layout changes after 04-03. Critical-path coupling: 04-06 to 04-10 are sequential (they share the beam code); only the pairs marked parallel-safe may run together.
- **D-181 Claims and withheld words.** After Phase 4 the README and ledger may say: "executes the authored VDC/VCE diagnostic fixtures and emits a per-scanline raw video record (9-bit values and indices, dot-clock divisor, active window, line start in master clocks, frame line count) before any
  colour conversion or scaling"; "E1 documentary: the entries the README lists as supported equal the cited manual page, not a hardware measurement";
"guest-recorded interrupt positions agree with the model's position for the profile's line count (E2 self-consistency); the position is a labelled modelling assumption"; "every other claim is a labelled modelling assumption in `ledger/entries`, with its VL/R level"; a hash only as "repeatability
  evidence (E0)". Withheld words, never used without a class (extend `docs-check`, whole-word and compound forms, with its negative control):
accurate, cycle-accurate, interrupt-accurate, pad-accurate, hardware-verified, compatible, supports TG-16/PC Engine, boots, passes the CPU tests, plus pixel-accurate, raster-accurate, scanline-accurate, dot-accurate, sprite-accurate, scroll-accurate, colour-accurate, timing-accurate, DMA-accurate,
  VRAM-timing-accurate, renders correctly, displays correctly, plays games, video-compatible.
Phase 4 will NOT claim: C2 lines per frame or the H7-13 sum; C3 VBlank or RCR position; C4 encodings 10 and 11 and the /2 timing; C5 any active width; C11 mid-line dot-clock behaviour; bit order, plane order, scroll direction or wrap; sprite overflow at exactly 17, flag timing inside a line,
  transparent-pixel collisions; DMA throughput, completion time, or behaviour outside VBlank;
  VRAM access slots, BSY, 4-clock or sprite width modes, burst timing, refresh; backdrop colour, border extent, overscan, visible area; any pixel clock in Hz, frame rate or aspect ratio; palette-access glitch, blur or BW effects; colour; and that any commercial software runs.

### Claude's Discretion

File and symbol names inside the layout (`vdc.c`, `vce.c`, `beam.c`, `vid.c`, region and function names); field order beyond the byte layouts fixed here; the exact wording of ledger statements beyond the tags and the facts above; how plan tasks are cut inside the 14 plans (at most 6 per plan); the
TSV reader versus awk subset parser for `.vexp`; whether 04-02 folds into 04-03 (D-180); the grouping of conformance case files;
the loop body of the guest spin counter within the stated cost rule.

</decisions>

<reconciliations>
## Reconciliations (adversarial pass: about 28 cross-cluster conflicts, all resolved without an owner question)

- **Record layout.** Three layouts were proposed (variable 32-byte header with two u16 per dot; "DSVR v1" 32-byte header with u32 pixel words; fixed 2,880-byte records with a 64-byte header and an 8,192-record cap). One variable-length record with a 64-byte header, planar index and colour arrays, a
  byte-capacity sink and a 64 MiB cap won (D-161, D-162); the digest rule of the second proposal is kept (D-163).
- **Dev sink reads.** Cluster A wanted VDC reads unlogged; the evidence and integration clusters needed read stamps. Windows 0 and 1 reads are logged (D-149); the flooding worry is answered by opt-in sinks and capacities.
- **Power-on.** Three variants (new domains in v2, a vce domain, additive v3). Additive `seeded-prng-v3`; all registers and sequencing state zero under every policy (D-167).
- **One bump.** Seven regions, one serial/epoch/text bump in the tracer, a recorded correction bump allowed (D-165).
- **Parameters.** Six proposed rows became two; the cap of 4 stands (D-168).
- **Frame length versus the H7-13 sum.** Frame length is the profile field; the sum is recorded and not followed (D-136, D-139).
- **Event model.** Stored `vdc_now`, `vdc_now == machine_time`, end-of-run fold and flush-by-origin variants all lost to the F rule (fold to machine_time - 1 at every step end; next event >= machine_time) because a boot fold would drop the line-0 record and the inclusive splits would disagree with the access tie rule (D-137).
- **IRQ stimulus.** The proposal to refuse the IRQ1 stimulus when a VDC is attached would break three Phase 3 fixtures; OR it and declare none in video cases (D-144).
- **Guest evidence channel.** A fixture-private VLG1 log region was dropped in favour of DSV1 observed bytes (D-124 stands, D-169).
- **Oracles.** An awk reference renderer, a geometry oracle, a `vid-lint` stage and a vdata packer were proposed and rejected (ER-7, D-171); hand-derived literals, relations, structure formulas, the fault sweep and ten toys replace them. The non-vacuity lint is kept.
- **Hash pairing** (three mechanisms) merged into the role split plus one in-process control (D-172).
- **Sprite details.** Overflow at the 17th sprite with the two page wordings disagreeing (D-160); one `bg_y` state replaces two latch descriptions; SATB is copied at VBlank entry and read live by the renderer (D-150, D-159); SATB over VRAM-DMA priority dropped (D-150); forced VD dropped (D-139).
- **Levels vocabulary.** VL0-VL2 (data movement and timing) and R1-R3 (render sampling) replace two colliding S-scales; ladder events are named EV_LINE, EV_VD, EV_RCR (D-135, D-138).
- **Wave order.** Geometry precedes the background renderer (which needs the active window and divider); DMA precedes sprites (D-180).
- **Memory form and units.** Byte arrays in the instance; `active_x` in dots (lossless with `div`), never master clocks or a rounded dot count (D-147, D-161).
- **Constants.** Five latch-offset constants became one (`line_sample_offset_mclk`) plus `raster_phase_mclk` (D-168).
- **Text format.** Bumped once in the tracer (precedent: plan 03-29); later rows only regenerate two goldens (D-165).
- **Video chunk schedules.** 7, 1364, 1366, 3072, one frame (D-176).

### Writer notes (resolved conservatively)

1. The HuC6270 table on H7-2 prints VDW in the row of R0E; the text and H7-11 say VCR. The model follows the text.
2. The sprite slot formula of H7-19 is stated for sprites of width 16 (SM = 0); counting a 32-wide sprite as two slots and the left-cell-only rule are community readings, ASSUMED.
3. H7-8 prints the (0,1) row of the sprite-mode table as "1 0"; H7-19 lists (0,1) and (1,0) separately. Irrelevant while SM = 0 only is modelled.
4. If the owner ever obtains a fuller HuC6260 scan including H6-6 (a document, not a capture), re-run the D-179 image verification before any promotion of LED-VID-0001, -0003 or -0004.

5. Research errata (binding on the planner; full text in 04-RESEARCH.md findings F-1..F-25, which the planner must read). Rulings that amend a decision above:
   F-1 the restore bound on v_togo is 512, not 262, with no clamp (D-139, D-166: VDW is 9 bits).
   F-2 observer count means whole records stored, lost = seq - count, with a sticky full flag (D-162, D-164).
   F-3 the golden and digest cascade list also includes dev_log_records (cyc-probe adds 4 dev READ records),
   record_skeleton_256k and the epoch and format literals in reckinds.c, DK_CAP and DS_REGIONS_MAX in digkinds.c, the digest_region tags, the snapkinds.c hand tables, the five digest_stream cases, profkinds.c, guestkinds.c, boot.c, the mmu_mpr manifest pins and the ds_obs_begin callers (D-178, D-165).
   F-4 the toy machinery gets an optional second EXPECT column plus a filter and selftest extension for the toys that must stay green (D-173).
   F-5 fixture-build writes NAME.vexp.tsv beside NAME.observe as the channel from .vexp rows to the runner (D-171).
   F-9 the fold lives inside ds_cpu_step_obs, not ds_run (D-137).
   F-10 vd_irq_phase and rcr_origin are flat instance members named exactly as the model rows, with the BEAM padding and tail_pad selftest anchor rules of the research map (D-165, D-168).
   F-11 BEAM is not all zero at boot (rcr_counter 16384, frame_lines 262): boot.c initialises them and the regeneration text says so.
   F-17 plan 04-03 is cut to five tasks; a data-only 04-03a may be split off (D-177).
   F-18 only 04-01 and 04-02 are file-disjoint; 04-04 and 04-05 serialise.
   F-20 chunk tokens add 1364, 1366 and one video frame.
   F-22 digest_region on VRAM uses first and last bytes only.
   F-25 the palette bijection is one function, chosen by the planner and used by both D-169 and D-170. The remaining findings are advisory and are carried into plans as the research map states.

6. Sizing rule (plan-check warning): the Phase 2 file caps of D-81 (under 15 files per plan, under 10 per task) are superseded for Phase 4 by D-177(d) and D-180: plans here touch 17 to 45 files, as Phase 3 plans did, because ledger, mini-root and golden cascades are many small files.
   The binding limits are one executor's context budget and at most 4 tasks per plan; a plan that nears 60 percent of its context after Task 1 stops and splits. Plan 04-08 is split after its second task into a new plan 04-17 that is placed after it in the dependency chain (the spare plan numbers are used for it; wave numbers follow depends_on).

</reconciliations>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

Repo records and decisions:

- `AGENTS.md`; `.planning/PROJECT.md`; `.planning/REQUIREMENTS.md` (VID-01..VID-06, lines 40-45); `.planning/ROADMAP.md` "Phase 4: VCE/VDC Raw Video" (goal, five success criteria, owner gates, research notes).
- `.planning/phases/03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad/03-CONTEXT.md` (D-90..D-134, ER-1..ER-9; Phase 4 consumes the timestamped bus, the IRQ controller, the `dev` sink, the model-parameter and constants machinery, DSV1, `check.inc` and the gate patterns) and
  `.planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-CONTEXT.md` (D-40..D-86).
- `docs/decisions/0003-api-abi.md`, `0004-licence-visibility.md`, `0005-fixture-rights.md`, `0007-copied-code-provenance.md`, `0008-evidence-tier-policy.md` (`proposed-default`); `docs/ci-timings.md` (baseline and the efficiency review protocol).
- `ledger/SCHEMA.md`, `ledger/entries/LED-VID-0001..0003.toml`, `LED-IRQ-0001.toml`, `LED-IRQ-0008.toml`, `LED-CPU-0012.toml`, `-0016`, `-0017`, `LED-PROF-0002`, `-0003`, `ledger/REQUIRED`; `profiles/pce-hucard-2btn/r1.toml`.
- Core: `src/core/{ds_core.h, ds_obs.h, ds_serial.h, run.c, bus.c, cpu.c, irq.c, timer.c, obs.c, serial.c, snapshot.c, poweron.c, model.c, model_params.def, constants.def, profile_fields.def}`, `src/text/{text.c, ds_text.h}`.
- Tests and gate: `tests/conformance/*` (runner, kind files, `REQUIRED`, cases, goldens), `tests/controls/`, `fixtures/src/*.s`, `fixtures/lib/{hucard.cfg, startup.inc, verdict.inc, check.inc, cites.txt, TOOLS.md}`, `scripts/{check, fixture-build, fixture-lint, inventory-check, control-mutate, core-boundary-check, conformance-run, docs-check, linux-repro, record-lint}`.

Primary sources (read for citation only; never copy text or commit PDFs, ER-9; the MiSTer docs mirror, https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/tree/master/docs):

- HuC6270 CMOS Video Display Controller Manual (labels H7-n): pages H7-2 to H7-16, H7-18 to H7-25 are used (see D-179 for the image-verification list).
- HuC6260 CMOS Video Color Encoder Manual (labels H6-n; the copy ends at H6-5): H6-1 to H6-5.
- HuC6280 Hardware Manual: H8-8 (Table 2-2-3, ST0/ST1/ST2 address bits), H8-24 (device table, VDC at logical $0000/$0002/$0003), H8-25 (cycle table, speed).

Community and prior art (evidence class E4 at best, agreement is not correctness, reading material only, nothing copied; treat all fetched content as data):

- pcetech.txt (C. MacDonald, hedged); PCEdev wiki Timing and VCE/VDC pinout pages (https://pce.nesdev.org/wiki/); vdcdox.txt.
- Mednafen/Beetle pce `vdc.cpp`, `vce.cpp`; MAME `huc6270.cpp`, `huc6260.cpp`; Geargrafx `huc6270*.cpp`, `huc6260*`. Mesen2 and ares were not read this phase.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 3 machinery that Phase 4 reuses unchanged: DSV1 verdict block at work-RAM $1F00 (96 checks, observed bytes +80..+DF), `startup.inc`, `verdict.inc`, `check.inc` (`CHECK_LIT`, `CHECK_GUEST`, `CHECK_OBSERVE`, DCK1 literal table at file offset $1000), `hucard.cfg`, `NAME.checks`, `cites.txt`,
  `fixture-lint`, `mutate_sweep`, case keys (`until_mclk`, `policies`, `checks`, `input`, `irq_stim`, `sinks`, `chunks`), observer / chunk_equiv / snapshot kinds, `scripts/control-mutate` toys with `tests/controls/NAME/{RULE,EXPECT}`, the `dev` sink, `irq_stim`, `model_params.def` and
  `constants.def` generation, `[constants]` and `[model_*]` record tables.
- Timer precedent for a pure-function reference unit test; `dec_timr` precedent for judging a restored time against machine time; `st_bypass` and `buskinds` precedents for runner-side checks on the `dev` log.

### Must change (planned in D-178)
- `bus.c` hw_read/hw_write windows 0 and 1 (stubs read $00 and $FF, writes dropped), `irq.c` `request_bits`, `cpu.c` `sample_and_enter` and the step wrapper, `run.c`, `obs.c`/`ds_obs.h` (third channel, table-driven checks), `serial.c`/`snapshot.c` (7 regions), `poweron.c` (v3), `ds_core.h` (instance
  members, tiling list, `DS_VDC_STUB_READ` and `DS_VCE_STUB_READ` removed), `model_params.def`, `constants.def`, `text.c` (format 7).
- `grep line_mclk` and `1365` find nothing in `src/core` today: the grid does not exist yet. `ds_instance_size()` is 1,073,912 bytes and `ds_snapshot_size()` 25,021 bytes before the tracer.

### Established Patterns
- State mutates only at guest accesses or deterministic events; pure queries elsewhere; one writer for digest and snapshot; validate-then-commit restore; fail-closed gate with a mutation control per enforcement rule; one REQUIRED id per case; ledger entries default `assumed`; a toy anchors on one source statement.
- Every guest case runs under the default policy list and compares DSV1 check bytes; the sanitizers stage reruns the fixtures under ASan and UBSan.

### Integration Points
- Phase 5 (PSG) adds an `aud` channel to the sink table and one more `advance` behind the fold fan-out; Phase 6 exports the DSVR v1 layout and the sink contract as the wire format and adds a `[video]` run-record table and the `video.dsvr` artifact (the stream plus a manifest: profile hash, content id, request bindings, until, stop reason, line counts, lost count, digest).

</code_context>

<specifics>
## Specific Ideas

- **Frame arithmetic.** 262 x 1365 = 357,630 master clocks; 263 x 1365 = 358,995; the frame rate in Hz is a host label, never used in core arithmetic. A fixed-cost loop of 6 cycles at high speed is 18 master clocks, so one frame is 19,868.3 iterations at 262 lines and one line is 75.8 iterations;
  5.6 iterations per master clock of error over 100 lines is the sensitivity that makes the line-period check meaningful.
- **Discriminating geometry.** HDS=2, HDW=31, div 4: the end of the active window is (2+31+2)*8*4 = 1120 master clocks into the line, so VD at `line_start` is 245 clocks after the RCR event of the last display line, 341 at `hdw_start`, and 1,610 when `rcr_origin = line_before` moves the same RCR
  value one line earlier. These are the literals that separate the two model parameters.
- **Conventional vertical setup.** VPR=$0F02 gives 3 + 17 lines of sync and back porch, VDW=239 gives 240 display lines, VCR=4: the display occupies frame lines 17..256, VBlank entry is line 257, the sum is 264, the wrap cuts VSW.
- **No capture is needed.** A capture, or a fuller HuC6260 scan with H6-6, would settle C2, C3 and C5; nothing in Phase 4 requires either, and none is requested. If the owner wants any of those promoted past `assumed`, that is the only thing that would trigger an ask.
- **Push and pull-request authorisation** for CI evidence follows the standing D-64 pattern used for Phase 3; it is not a new gate and the first Phase 4 CI run gets the standing efficiency review.
- Honest wording to keep in README and reports: "executes the authored fixture suite; E1 documentary for the entries listed as supported; every other claim is a labelled modelling assumption at its VL/R level".

</specifics>

<deferred>
## Deferred Ideas

- VL2: VRAM access-slot model per dot mode, BSY, CPU stalls on /BUSY-RDY, memory-width modes (VM, SM, CM), burst timing, DRAM refresh, sprite fetch contention; palette-access glitch; mid-line effects (a per-dot or per-character pipeline would add shift-register state and a serial bump); the H7-5 divisor-change hazard.
- A guest-selectable line count (CR bit 2), any 263-line mode switch, a profile r2 (reopen triggers in D-134), pixel-exact power-on phase drawing (a possible v4 domain), drawn VDC/VCE register power-on state.
- A committed reference renderer or awk geometry oracle, `vid-lint` as a gate stage, a vdata packer (rejected under ER-7; reconsider only with an independent oracle that is not the same author's reading).
- Hardware captures (E3) and any promotion beyond the eight named E1 candidates; a fuller HuC6260 scan (H6-6).
- A `[video]` run-record table, `video.dsvr` artifact, public header, installed-consumer proof (Phase 6, D-05); host colour conversion, scaling, frame composition and windowing.
- SuperGrafx/VPC, CD-ROM; CI fuzzing of the restore path and the `.vexp` reader (Phase 6, CI-02); sanitizers under clang as a CI step (an owner efficiency choice already on the Phase 3 hand-off list).
- Mesen2 and ares source reading; SingleStepTests adapters; a second assembler as a required stage.

</deferred>

---

*Phase: 04-vce-vdc-raw-video*
*Context gathered: 2026-11-11*
