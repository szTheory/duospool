# Phase 4: VCE/VDC Raw Video - Research

**Researched:** 2026-11-11
**Domain:** C11 freestanding VCE/VDC devices on the existing master-clock bus; a third caller-owned `vid` sink with variable-length records; sh+awk gate, ca65/ld65 authored display fixtures, a video judge
**Confidence:** HIGH for the manual facts (page images read this session) and for the code map (files read this session); MEDIUM for cost and plan-feasibility figures, which are measured on a loaded machine or estimated

Provenance tags used here: [MAN img] = a manual page read as an IMAGE this session (second read, a cross-check; the recorded ER-1 read stays plan 04-01's job); [CODE] = a file of this repository read this session, path and line given; [CO] = measured or observed by a command this session; [P] = a proposal of this research; [ASSUMED] = not verified, listed in the Assumptions Log.
Mapping to the agent vocabulary: [MAN img], [CODE] and [CO] are VERIFIED by observation; [P] and [ASSUMED] need the planner's or owner's confirmation. No manual text or image is copied here (ER-9): page labels and short numeric facts only. No path outside the repository is named.

<user_constraints>
## User Constraints (from 04-CONTEXT.md)

The discuss step is done. Everything below is locked and binding exactly as written in `.planning/phases/04-vce-vdc-raw-video/04-CONTEXT.md` (decision text on its lines 49-402). This file lists ids and titles only, to stay short (the Phase 3 precedent); read the CONTEXT text before planning.

### Locked Decisions
- Evidence rules: ER-1..ER-9 of Phase 3 stand; ER-10 weakest class dominates; ER-11 a video hash is a change detector, never a verdict; ER-12 symmetry split (order-insensitive group vs asymmetric ASSUMED group); ER-13 records are definitions.
- A. D-135 levels VL0-VL2 and R1-R3; D-136 fixed 1365-clock grid; D-137 one fold `ds_vdc_advance` at three fold points, closed-form line count; D-138 line ladder EV_LINE, EV_VD, EV_RCR; D-139 vertical cycle VDS+2, VDW+1, VCR, VSW+1; D-140 horizontal geometry; D-141 dot clock 4, 3, 2, 2; D-142 RCR counter and `rcr_origin`; D-143 VBlank and `vd_irq_phase`; D-144 status and IRQ1; D-145 contention.
- B. D-146 port and registers; D-147 VRAM 32K words, one accessor pair; D-148 write and read paths; D-149 ST, block transfers, dev reads logged; D-150 SATB DMA; D-151 VRAM-VRAM DMA.
- C. D-152 VCE window; D-153 palette; D-154 CR. D. D-155 render rule R1; D-156 background and scroll; D-157 enables, burst, border; D-158 VM/SM modes not modelled; D-159 sprite decode; D-160 slots, priority, overflow, collision.
- E. D-161 record DSVR v1; D-162 sink and run request; D-163 canonical stream and digest; D-164 non-interference. F. D-165 seven regions and the one bump; D-166 restore validation; D-167 `seeded-prng-v3`; D-168 two model rows and 16 constants rows.
- G. D-169 literal sources; D-170 twelve fixtures; D-171 video judge; D-172 vacuity controls and `video-check`; D-173 ten toys; D-174 guest-recorded interrupt measurements; D-175 data-moved evidence. H. D-176 gate and cost; D-177 per-plan process rules; D-178 Phase 3 cascade; D-179 ledger (17 entries, 10 amendments, 8 E1
  candidates); D-180 14 plans, 8 waves, cut line at 04-11; D-181 claims and withheld words.

### Claude's Discretion
- File and symbol names inside the layout (`vdc.c`, `vce.c`, `beam.c`, `vid.c`, region and function names); field order beyond the byte layouts fixed in the CONTEXT; wording of ledger statements beyond the tags and facts; how tasks are cut inside the 14 plans (at most 6 per plan); TSV reader versus awk subset parser for
  `.vexp`; whether 04-02 folds into 04-03; grouping of conformance case files; the loop body of the guest spin counter within the stated cost rule.

### Deferred Ideas (OUT OF SCOPE)
- VL2 access slots, BSY, CPU stalls, VM/SM/CM modes, burst timing, refresh, palette-access glitch, mid-line effects, the H7-5 divisor-change hazard; a guest-selectable line count, a profile r2, drawn VDC/VCE power-on state.
- A committed reference renderer or geometry oracle, `vid-lint`, a vdata packer; hardware captures and any E1 promotion beyond the eight named candidates; a fuller HuC6260 scan.
- A `[video]` run-record table, `video.dsvr` artifact, public header, installed-consumer proof (Phase 6); host colour conversion, scaling, windowing; SuperGrafx/VPC, CD-ROM; CI fuzzing; Mesen2 and ares reading; SingleStepTests adapters.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| VID-01 | Background from BAT/VRAM, documented scroll-register latch, literal fixtures | H7-7, H7-9, H7-14, H7-15, H7-18 image-read; render and latch map in the integration section; ER-12 split; `bg_*` validation rows |
| VID-02 | Sprites from the SAT, per-line limit, overflow and collision status | H7-4, H7-19..H7-24 image-read (CGY, CGX, SG layout, slot table); two findings on classification (F-13) and the 17th sprite wording |
| VID-03 | VRAM port, VRAM-SATB DMA, VRAM-VRAM DMA, unverified timing labelled | H7-5, H7-7, H7-11, H7-12, H7-25 image-read; accessor, fold and DMA closed form map; `vdc_*` validation rows |
| VID-04 | Raster-compare and VBlank interrupts at the modelled position, status read acknowledges, guest-recorded | H7-4, H7-6, H7-7, H7-10, H7-13 image-read; fold-before-sample integration; spin-counter evidence plan; findings F-1, F-9, F-12 |
| VID-05 | 512x9-bit palette, dot clock modes, per-line geometry | H6-2..H6-5, H7-9, H7-10 image-read; VCE window decode map; divider latch; geometry and clip rules |
| VID-06 | Raw per-scanline records before colour conversion; mid-frame changes represented | D-161..D-164 consistency checked against the code; sink table design; findings F-2, F-5, F-6; validation rows |
</phase_requirements>

## Summary

The locked design is implementable with no new dependency and no change of tool. The code was read and measured this session: the instance is 1,073,912 bytes, a snapshot is 25,021 bytes in 14 regions, the gate has 56 stages, one full runner pass over 236 cases takes 13.9 s of CPU on a loaded machine, and emulation itself is
cheap (a 6M-clock guest case takes 0.03 s). The seven new regions add 67,178 payload bytes, so the snapshot becomes 92,255 bytes in 21 regions [CO]. Every cite behind the eight E1 candidate entries was re-read as an image: all cited literals are on the cited pages, with refinements the ledger text must carry (F-12, F-13, F-23 and the candidate table) (section
Manual Page Image Verification).

The main planning risks are mechanical. The Phase 3 cascade list in D-178 is incomplete: `dev_log_records` will see four more dev records from `cyc_probe`, `digkinds.c` has a 32,768-byte capture buffer and a 16-region cap, `snapkinds.c` hard-codes the 14-frame layout, `record_skeleton_256k` and `reckinds.c` carry the epoch and
format literals, `profkinds.c` and `guestkinds.c` carry the policy list, and `boot.c` must initialise the two non-zero beam fields. The toy machinery can only assert `guest_failure`, so the two "stays green" toys of D-173 need a small extension. The `.vexp` data has no channel to the runner in the CONTEXT; the fixture-build
`.observe` precedent gives a channel with no new runner argument. The cut-line trigger "warm quiet gate over 150 s" will probably fire at 04-11 on cost alone, so 04-11 must be a viable close-out.

Two decision-level inconsistencies need a ruling before 04-03: the restore bound `v_togo <= 262` of D-166 refuses reachable states (VDW+1 reaches 512), and the observer rule `count = min(seq, cap)` of D-164 mixes records and bytes. All findings are numbered F-1 to F-25 below with the decision id; none changes a locked decision silently.

**Primary recommendation:** Keep the 14 plans and 8 waves. Make 04-03 the only plan that edits layout, serial, epoch, text and the whole red list; rehearse the regeneration procedure once outside the repository first. Fold the VDC inside `ds_cpu_step_obs` itself, not in `ds_run`. Carry `.vexp` data to the runner as
`build/gate/fixtures/NAME.vexp.tsv` written by `fixture-build`. Resolve F-1, F-2 and F-4 in 04-01 or 04-02 text before any C is written.

## Findings: inconsistencies, impossibilities and omissions (numbered)

Severity: B = the plan named cannot be done as written until ruled; A = advisory, a trap or a correction. No decision is changed here; each finding gives a recommended ruling.

- **F-1 (B, 04-03 and 04-06; D-166, D-139).** The restore bound `v_togo <= 262` refuses reachable states. `v_togo` is the remaining lines of the current state, latched at state entry from the registers: VDW is 9 bits (H7-10 [MAN img]) so the VDW state can be 512 lines, VDS+2 up to 257, VCR up to 255. D-139 cuts an unfinished
  state at the frame wrap but does not clamp `v_togo` at entry. `vdc_random_regs` and `snapshot_hostile` would hit this. Ruling: do not clamp; bound `v_togo` at 512 and `display_k` below 512 in `dec_beam`; the cut stays a consequence of the frame restart.
- **F-2 (B, 04-02 and 04-03; D-164 vs D-162).** D-164 says header `count = min(seq, cap)`, but D-162 makes `capacity` a byte count and records are variable length (64 to 2792 bytes). The relation is: `count` = whole records stored, `bytes` = their sum, `lost = seq - count`, and after the first record that does not fit every
  later record is lost (a sticky `full` flag in the chan, because a later header-only record is smaller and would otherwise fit). The "4-record" observer configuration must take its byte capacity from the first four record sizes of the large run.
- **F-3 (B, 04-03; D-178, D-165).** The cascade list is incomplete. Add: `dev_log_records` (its ROM `cyc-probe` reads window 0 and 1 four times; the kind demands the log hold exactly the listed records, `obskinds.c:1323-1325`); `record_skeleton_256k` (literals `record.format_version=6`, and the epoch) and `reckinds.c:247`
  (literal `behaviour_epoch = 4`); `digkinds.c:40` `DK_CAP 32768` (the stream becomes 92,255 bytes) and `digkinds.c:147` `DS_REGIONS_MAX 16` (21 regions); `digest_region.toml` tags list (14 tags); `snapkinds.c:32-45` (`SK_VERSION 7`, `SK_FRAMES 14`, `SK_EXPECTED_SIZE 25021`, `sk_frames[]`, the `RG_*` enum); the five
  `digest_stream_*` cases (`version=7`, `frames=`); `profkinds.c:188` (hard-coded allowed list of `powerup_policy`); `guestkinds.c:840` (`default_policies`); `boot.c:345-362` (not in the CONTEXT "Must change" list); the three `mmu_mpr` manifest pins; `obskinds.c:1416,1508` (`ds_obs_begin` gains a parameter). D-165's "red list
  is exactly {...}" must read "at least".
- **F-4 (B, 04-06, 04-09, 04-10; D-173).** The toy machinery asserts only `guest_failure` for the case ids of `EXPECT` (`scripts/conformance-run` controls_mode, lines 534-550 [CODE]). Two toys need a case that STAYS `pass` under the mutant: `vid-rcr-base` (ratio group stays green) and `vid-bit-order` (order-insensitive MAN
  group stays green). Ruling: let an `EXPECT` line carry an optional second column `pass` (the id column is already read with `awk '{ print $1 }'`), include those ids in `filter_cases`, check them in the same loop, and extend selftest `mutation-toy-filter`. Each such scene is judged as two cases (one per group) so a group's
  outcome is a case outcome.
- **F-5 (B, 04-05; D-171).** `.vexp` has no channel to the C runner (the runner reads flattened TSV, never TOML). Passing a new `--vexp` argument touches six call sites (conformance-run run, sanitize, controls_mode, control mode; determinism-run argument list and its required-input list; selftest scratch lists in
  `scripts/check`). Ruling: `scripts/fixture-build` writes `build/gate/fixtures/NAME.vexp.tsv` from the validated `fixtures/src/NAME.vexp`, next to `NAME.observe` and `NAME.classes`, which the guest judge already reads from the working directory.
- **F-6 (A, 04-05; D-172).** The codes `INV_VIDEO_HASH_ONLY`, `INV_VIDEO_GROUP_UNKILLED`, `INV_TOY_NO_LITERAL` carry the inventory-check prefix but D-172 hosts them in the new `video-check` stage. Ruling: `scripts/video-check` prints them verbatim (own awk rules file, own negative mini-roots under `tests/video/`);
  `inventory-check` is not touched; the selftest mutations name the rule tags in the new files.
- **F-7 (A, 04-03; D-155).** "Enforced by a core-boundary anchor" has no counterpart: `scripts/core-boundary-check` has include, compile, undefined-symbol, writable-symbol and float rules only [CODE]. The `const` parameter of the render function is compiler-enforced; the dynamic proof is toy `vid-emit-mutates` (control-mutate anchor). Do not add a core-boundary rule.
- **F-8 (A, 04-11; D-180, D-176).** The cut-line trigger "warm quiet gate over 150 s" will likely fire: the Phase 3 local baseline was 193 s (loaded, load 15) with 129 s of user CPU, against a 120 s target that was already missed (`docs/ci-timings.md`, W8 section), and D-176 adds +35 s central. Ruling: define "quiet" as the
  measured user CPU of one `sh scripts/check --timing` run, record it at 04-03 and 04-11, and plan 04-11 as a complete close-out for VID-01, 03, 04, 05, 06 (as D-180 already says).
- **F-9 (B, 04-02; D-137).** "A thin wrapper around `ds_cpu_step_obs`" must live INSIDE `ds_cpu_step_obs` (rename the current body to a static function; the public function folds at `start + total - 1` and returns total). `ds_cpu_step` (NULL log) calls it, so the stepwise twin of `obskinds.c:169` (`w->machine_time +=
  ds_cpu_step(w)`) equals `ds_run` once VDC state exists. A wrapper in `run.c` would break `insn_sink_shapes`.
- **F-10 (B, 04-03; D-165, D-168).** Layout constraints the CONTEXT does not state: (1) `model.c:104-108` writes `inst->name` for each model row, so `vd_irq_phase` and `rcr_origin` must be flat `uint8_t` members of `struct ds_instance` with exactly those names (no nested beam struct); (2) the BEAM stream order of D-165 puts a
  `u8` before `u16` fields, which would give implicit padding in a struct in the same order; the struct order may differ from the stream order but each region's live span must be contiguous and fully carried (`ds_region_info` accessor, `digest_region`); implicit padding breaks the tiling assert; (3) `rom` sits at offset 25,330
  (not 8-aligned) so `u64` members need explicit pads or must follow `tail_pad`; (4) selftest `mutation-tiling-entry` anchors on the exact line `    uint8_t tail_pad[6];` in `ds_core.h` (scripts/check, selftest `mutation-tiling-entry`, about line 2840), keep that line or change the selftest in the same commit; (5) any new
  explicit pad member must be added to `sk_is_identity` in `snapkinds.c:81` or be restored.
- **F-11 (A, 04-03; D-165 procedure, D-167).** The regeneration text says appended frames are "zero bytes for zero, v1 and v2". The BEAM payload is not zero at boot: `rcr_counter` is the constant 16384 and `frame_lines` is bound from the profile (262 by default, `runner.c:89`). The independent re-derivation must write those two
  fields at their stream offsets. `boot.c` must set them after `memset` (line 345) because "every beam field is 0 under every policy" cannot hold for these two; `vd_off` and `rcr_off` may stay 0 at boot because EV_LINE recomputes them before use.
- **F-12 (A, 04-01; D-179, LED-VID-0009).** The FACT list of LED-VID-0009 (H7-7, H7-8) omits H7-9, the page that gives the SCREEN field "valid at the beginning of a vertical blanking period" and the CM field "next scanning line"; H7-8 gives only VM and SM. Also "BYR on the first display line = BYR" is not on any page (H7-7 only
  says a mid-display update shows from the next line as BYR+1): it is an assumed clause and must stay outside the E1 sentence.
- **F-13 (A, 04-09, 04-12; ER-12, D-156, D-159).** Plane significance IS page-stated: H7-18 prints VD3 to VD0 as CH3 to CH0 and H7-24 prints VD3 to VD0 as SG3 to SG0 [MAN img]. What no page prints is which byte of a word is which plane and which pixel is leftmost. Classing "plane order" wholesale as ASSUMED is conservative and allowed; if a check isolates plane significance it may be MAN.
- **F-14 (A, 04-01; D-181).** The docs-check regex (`scripts/docs-check:125`) already matches `accurate` and `compatible` as whole words, and a hyphen is a boundary, so `pixel-accurate`, `video-compatible` and the other hyphenated compounds are already caught. Only the multi-word items need regex additions: `renders correctly`,
  `displays correctly`, `plays games`. Keep one `rule-claim-word` tag line (the selftest deletes tagged lines).
- **F-15 (A, 04-01, 04-05; D-177(g)).** `licence-check` already requires the 0BSD SPDX pair in the first five lines of every file under `fixtures/` (`scripts/licence-check:123-127`), so `.vexp` needs a header and no code change; new files under `scripts/` need the MIT-or-Apache pair; `tests/conformance/VIDEO_TOY_EXEMPT` is not scanned (not .c, .h or .def).
- **F-16 (A, 04-02; D-178).** "Every `struct ds_run_request` local is zeroed in 04-02" is already true: 34 locals in 14 test files, all zeroed by `memset` or `rr_base_request` [CO]. `run_refuse_struct_size` already uses `sizeof`. The work is assigning `vid` where a case wants it, plus one new variant (a request of the OLD size, `sizeof - sizeof(void *)`) as an ABI-skew control.
- **F-17 (B, 04-03; D-177(d)).** 04-03 as listed (layout, bump, goldens, VCE, grid, records, fold, dev read logging, `video` kind, `vce_pal`, `vdc_fold`, chunk, observer, snapshot, hostile corpus, three toys, a measurement spike) cannot fit in 6 tasks without very large tasks. Ruling: five tasks, each one atomic commit set: (1)
  layout, serial, epoch, text, v3, boot init and the whole red list with goldens; (2) VCE window, grid, step-end fold, record emission, dev read logging; (3) runner `video` kind, `vce_pal`, manifest, cites; (4) `vdc_fold`, chunk_equiv, observer, snapshot and hostile corpus over 21 regions; (5) three toys and the measurement
  spike. If this is still too large, split off task 1 as a data-only plan `04-03a` using spare number 04-15 (the bump stays in one plan).
- **F-18 (A, W3; D-180).** The pair 04-04 and 04-05 is logically independent but not file-disjoint: both append to `tests/conformance/REQUIRED`, `runner.c` `kinds[]` (line 1007 table) and `runner.h`. Only 04-01 and 04-02 are file-disjoint. Ruling: run W3 sequentially or give 04-05 the shared hot files; file creation is disjoint
  if `videokinds.c` is created in 04-03, `vdckinds.c` in 04-04 and 04-05 only edits `videokinds.c` (the explicit `DS_RUNNER_SOURCES` list in `CMakeLists.txt:63-69` is then edited by one plan per file).
- **F-19 (A, 04-03 onward; D-170).** `opcode_exercise` runs EVERY guest case ROM with an insn sink of 262,144 records and makes a lost record a `harness_failure` (`obskinds.c` header comment). A long spin-counter ROM at high speed executes roughly 10 master clocks per instruction [P], so 2M clocks is about 200,000 instructions:
  stay under the cap by short `until_mclk` and `DSV1 done`, or raise the case capacity (the sink maximum is 1,048,576 records).
- **F-20 (A, 04-03; D-176).** The chunk token `frame` is the historical constant 357,368 (`chunkkinds.c:43`), not 262 x 1365 = 357,630. Add new tokens (`1364`, `1366`, `vframe` = 357,630 at 262 lines) rather than reuse `frame`; `cs_table` has 7 rows and `CS_SCHED_MAX` is 8 per case, a video case lists 5.
- **F-21 (A, 04-06; D-144, D-137).** After the VDC level is ORed in, `request_bits` is no longer a pure function of `(inst, stamp)`: the VDC part reads the folded `vdc_status` and ignores the stamp. `ds_irq_pending_query` is called by the stop callbacks of `chunkkinds.c:184` and `obskinds.c:804` at `ci->machine_time` (the fold
  reached `machine_time - 1`): the timer is inclusive at that stamp, the VDC is not. Harmless for the existing uses; document "valid for a folded instance" in `ds_core.h` and never use the query at a stamp past the fold in a new test.
- **F-22 (A, 04-03; D-165, D-166).** `digest_region` with `TAG:all` flips every byte and re-hashes: for VRAM that is 65,536 hashes of 92 KB (about 6 GB). Use the default first-and-last flip for `VRAM`, `SATB` and `PALT` (`PALT:all` at 1,024 flips is about 94 MB and acceptable); the hostile corpus samples VRAM bytes, it does not perturb all of them.
- **F-23 (A, 04-07; D-141, LED-VID-0017).** H6-3 gives the divider for the CONTENT of CR (00h and 01h), a whole byte; D-141 reads bits 1:0 of a byte whose other bits (BW, bit 2) are carried and ignored. The E1 sentence of LED-VID-0017 is exactly "content 00h maps to master/4 and 01h to master/3", which is page-true; it must not be widened to "bits 1:0".
- **F-25 (A, 04-03; D-169 vs D-170).** Two different closed-form palette bijections are named: D-169 says `pal[i] = (i*73 + 19) & 511`, D-170 `vce_pal` says `(i*$5B+$11)&$1FF` (that is 91 and 17). Both are bijective on 0..511 (73 and 91 are odd). Pick ONE constant pair for the fixture, record it in the `.checks` row and the
  `.vexp` comment, and make the GUEST check recompute it with different arithmetic (running additive sum versus multiplicative writer).
- **F-24 (A, 04-02; D-137(1)).** The three fold points need the VDC fold at the head of `hw_read` for windows 0, 1 and for `$1403` (bus.c lines 119-147), at the head of `hw_write` for windows 0 and 1 (lines 154-156, including the ST path `ds_bus_write_phys`), and in `sample_and_enter` before `ds_irq_pending` (cpu.c:566). Reads
  and writes of `$1402` and `$1400` need no fold (the mask only gates delivery).

## Architectural Responsibility Map

This is a library plus a gate, so the tiers are the repository layers.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| VCE and VDC state, registers, VRAM, SATB, palette, line grid, fold, DMA, flags | Core `src/core` (new `vdc.c`, `vce.c`, `beam.c`) | - | Deterministic machine behaviour lives in the core (CLAUDE.md core boundary); no clock, file or allocator |
| Line render (BG, sprites) and the DSVR record bytes | Core `src/core/vid.c` (const render function, byte-by-byte writer) | - | R1: the record is a pure function of state at EV_LINE; the sink is output only |
| Sink channels and run request | Core `obs.c`, `ds_obs.h`, `run.c` | Runner kinds | Same mechanism as insn and dev (D-119); caller owns memory |
| Interrupt level and delivery | Core `irq.c`, `cpu.c` | - | IRQ1 is a level ORed with the declared stimulus; delivery stays under `$1402` and the I flag |
| Printing constants, model rows, format 7 | `src/text` | `record-lint` | The printer prints, nothing parses (D-40) |
| Stream, digest and canonical video file | Runner `tests/conformance` (new `videokinds.c`) | Gate scripts | Host concerns; the core only fills sinks (D-163) |
| Literal data of the judge | `fixtures/src/NAME.vexp` via `fixture-build` | `scripts/video-check` | Hand-derived literals; one validated parser; the C runner reads TSV only |
| Fixtures, DSV1 verdicts, spin counters | `fixtures/src`, `fixtures/lib` (ca65, 0BSD) | `fixture-lint`, `fixture-build` | Authored, built in the gate, never committed as ROMs |
| Claims and tiers | `ledger/entries`, `ledger/REQUIRED` | `inventory-check`, `docs-check`, README | ER-2: `assumed` by default, at most eight E1 candidates |

## Manual Page Image Verification (cross-check of the E1 candidate cites)

Method: both PDFs were opened from a session scratch directory (outside the repository, never committed). Byte size and SHA-256 equal the CONTEXT: HuC6270 copy 1,497,291 bytes, `2bb87866...ac6ab`; HuC6260 copy 315,186 bytes, `c6f0d83a...d92` [CO shasum]. Page n of the HuC6270 manual is PDF page n+3 (28 pages); label n of the
HuC6260 manual is PDF page n+2 (7 pages: cover, contents, H6-1 to H6-5). The HuC6260 contents page lists the sync generator at H6-6, which is not in the file [MAN img]. Each page below was rendered at 100 dpi and read as an image on 2026-11-11 (verifier: gsd-phase-researcher). Plan 04-01 still records its own ER-1 read in
`cites.txt`.

### Facts seen, page by page

| Page | Numeric facts seen on the image | Backs |
|------|----------------------------------|-------|
| H7-2 | Register list R00 to R13 with symbols; AR and SR rows; R03 and R04 shown as reserved; SR bits BSY 6, VD 5, DV 4, DS 3, RR 2, OR 1, CR 0; CR fields IW, DR, TE, BB, SB, EX, IE; MWR fields CM, SCREEN, SM, VM; the R0E row is named VCR but its data cell reads VDW (misprint) | -0006 |
| H7-3 | AR is write-only, bits 4 to 0 (5 bits), registers R00 to R13; a warning not to use AR = 04 | -0006 |
| H7-4 | Status bits as H7-2; read clears all but BSY; a status bit is set if an interrupt enabled by the IE bits of CR and DCR occurs; CR flag is sprite 0 against sprites 1 to 63; OR text says "more than 17"; DV is cancelled when a vertical display cycle starts and then no end status; VD means a vertical blanking period has started; BSY is the inverse of the /BUSY pin | -0008, -0014, -0012 |
| H7-5 | MARR high-byte write starts the read into the read register and increments MARR when done; VWR high-byte write starts the write and increments MAWR; VRR high-byte read triggers the next word; do not change the VCE clock division during VRAM access; CR layout (IW 12-11, DR 10, TE 9-8, BB 7, SB 6, EX 5-4, IE 3-0); EX, TE, DR valid at once | -0006 |
| H7-6 | IE bits 0 to 3 = CC, OC, RC, VC; EX table; SB and BB valid from the next horizontal display period; SB = BB = 0 starts burst with the next frame and burst clears in the next frame after SB or BB is 1; VRAM-VRAM transfer is always possible in burst | -0008, -0010 |
| H7-7 | IW table +1, +20h, +40h, +80h for IW 00, 01, 10, 11; RCR counter "set to 64 at the previous scanning line of start timing of display period" and incremented each scan; BXR valid with the next scanning line; BYR update during display shows with the next line as BYR+1; widths RCR 9..0, BXR 9..0, BYR 8..0 | -0006, -0009 |
| H7-8 | MWR bits CM 7, SCREEN 6-4, SM 3-2, VM 1-0; VM and SM valid at the beginning of a vertical blanking period; VM table: width 1 gives CPU slots at character-cycle positions 1, 3, 5, 7, width 2 one CPU slot, width 4 none | -0009 |
| H7-9 | SCREEN table 000 32x32, 001 64x32, 010 128x32, 011 128x32, 100 32x64, 101 64x64, 110 128x64, 111 128x64, valid at the beginning of a vertical blanking period; CM valid with the next scanning line; HSR HDS bits 14-8 and HSW bits 4-0 valid at the beginning of each period; the "N-1" rules | -0009, -0005 |
| H7-10 | HDR HDE bits 14-8, HDW bits 6-0; VPR VDS bits 15-8, VSW bits 4-0, VDS = N-2 for a back porch of N; VDW 9 bits, N-1, valid at the beginning of a vertical display period | -0005 |
| H7-11 | VCR 8 bits, valid at the beginning of a vertical front porch; DCR DSR bit 4, DI/D bit 3, SI/D bit 2, DVC bit 1, DSC bit 0; 0 increments and 1 decrements; SOUR | -0008, -0013, -0014 |
| H7-12 | DESR; LENR up to 64K words with LENR = M-1; DVSSR; VRAM-VRAM transfer in vertical blanking or burst, the rest is not transferred in the next period, triggered by the LENR high byte; VRAM-SATB 256 words at the start of vertical blanking, triggered by the DVSSR high byte, a set register starts at the beginning of the following period | -0013, -0014 |
| H7-13 | Fig 2-2-1: widths HSW+1, HDS+1, HDW+1, HDE+1; heights VDS+2, VDW+1, VCR, VSW+1 in that top-to-bottom order; (32,64) sprite coordinates at the display top-left; character 8x8 dots; 4,096 characters; 16 of 256 colours per character | -0005, -0011 |
| H7-14 | BAT at VRAM address 0, 32x32 shown with +1 right and +20h next row; BAT word = CG COLOR bits 15-12, character code bits 11-0 | -0010 |
| H7-15 | CG: 16 words per character, CH0 and CH1 are CG0 (8 words), CH2 and CH3 are CG1 (8 words); CH0 start address has 4 low zero bits; the figure draws CH1 left of CH0 in a word but prints no bit numbers | -0010 |
| H7-16 | BG display control flow; two words (CH0-CH1, CH2-CH3) per 16-colour character; no numeric fact used | context only |
| H7-18 | BG video data VD8 = 0, VD7-VD4 = CG COLOR, VD3-VD0 = CH3-CH0; all four low bits 0 forces VD7-VD4 to 0; border VD8 = 1 and VD7-VD0 = 0; VD8 = SPBG | -0010 |
| H7-19 | Up to 16 sprites per line; table for SM (0,0): entries searched 64 when d >= 32 else 2d+1, taken from SG 2(e-2), max 16; d = HDW+1, e = HDE+HSW+HDS+3; "17 or more ... the 16 of higher priority are displayed"; SATB 64 sprites | -0011, -0012 |
| H7-20 | SATB is not CPU-writable; 4 words per sprite, 256 words; word layout Y bits 9-0, X bits 9-0, pattern code bits 10-0, Y-flip 15, CGY 13-12, X-flip 11, CGX 8, SPBG 7, colour 3-0; priority follows address | -0011 |
| H7-21 | (32,64) top-left; pattern code high 10 bits equal the high 10 bits of the VRAM address; SPBG 0 background, 1 sprite priority | -0011 |
| H7-22 | X flips right-left, Y flips up-down; CGX pairs two sprites left to right, the one with PC bit 1 = 0 first; X flip inverts the pair | -0011 |
| H7-23 | CGY table 00 normal, 01 2CGY, 10 not used, 11 4CGY; 2CGY combines patterns with PC3,PC2 = (0,0),(0,1) or (1,0),(1,1); 4CGY all four | -0011 |
| H7-24 | SG is 4 areas of 16x16, 16 words each, SG0 aligned to 64 words, SG1 at +16, SG2 at +32, SG3 at +48; sprite VD8 = 1, VD7-VD4 = colour, VD3-VD0 = SG3-SG0 | -0011 |
| H7-25 | Status high byte reads 00h; write sequence AR, low, high (the page prints A0 = 0 for both data steps: a misprint); MAWR incremented after the high VWR write; MARR incremented after the high VRR read | -0006 |
| H6-1 | 512 colours; no register fact | context only |
| H6-2 | Registers by pins A1, A2: CR (0,0) write, CTA (1,0) write, CTW (0,1) write, CTR (0,1) read; CR is the field DCC in D7-D0; CTA 9 bits; CTW and CTR are G, R, B of 3 bits each over D8-D0 | -0016 |
| H6-3 | CR content 00h = division ratio 4, fCLOCK 6fsc, fck 1.5fsc; 01h = ratio 3, 6fsc, 2fsc; CTA increments after the HIGH byte access; G bits 8-6, R 5-3, B 2-0 | -0016, -0017 |
| H6-4 | Colour table is 512 addresses x 9 bits, two 256-address halves (VD8 = 0 background, 1 sprite), 16 blocks of 16 | -0016 |
| H6-5 | CTA write low then high; data write low then high, CTA incremented after the high byte; read the same, CTA incremented after the high byte | -0016 |

### Candidate ledger entries against the pages (is each cited literal on the cited page?)

| Entry | Cited pages | Verdict | Refinement the ledger text must carry |
|-------|-------------|---------|---------------------------------------|
| LED-VID-0006 | H7-2, H7-3, H7-5, H7-7, H7-25 | All cited literals are on the pages | Field widths behind the masks are on H7-5 (CR), H7-7, H7-8, H7-9, H7-10, H7-11; add them if the masks are in the E1 sentence. No register is readable on this interface, so a "mask" is a field-width fact, not a read-back fact. Write latch, increment moment and reserved reads are not page facts |
| LED-VID-0008 | H7-4, H7-6, H7-11 | Layout, clear-on-read, IE bits 0-3 and DCR bits 0-1 are on the pages | The page says an interrupt enabled by IE sets its bit; "a flag is set only when its enable is set at the event instant" is a reading, keep it out of the E1 sentence. BSY = 0 is a modelling constant |
| LED-VID-0009 | H7-7, H7-8 | BXR and BYR statements on H7-7; VM and SM valid at VBlank on H7-8 | Add H7-9 (SCREEN at VBlank, CM next line). "BYR = BYR on the first display line" is not on a page (F-12) |
| LED-VID-0011 | H7-13, H7-19, H7-20..24 | Origin (32,64), 16 per line, the slot table, SAT layout, flips, CGX, CGY table, SG layout, VD bits are all on the pages | CGY = 2 as 3, the partial-half rule and the 2-slot rule for 32-wide sprites are not page facts; the H7-19 table is printed for the sprite-width mode SM = 0 only |
| LED-VID-0013 | H7-11, H7-12 | 256 words, DVSSR high-byte trigger, start at the next vertical blanking, DSR repeat, DSC bit are on the pages | The 1,024-dot duration and the copy instant are assumed |
| LED-VID-0014 | H7-4, H7-11, H7-12 | Trigger byte, M-1, 64K words, cancel at display start without end status, SI/D and DI/D, burst or blanking only are on the pages | "IW never applies", 4 dots per word and "pending until the bus is free" are not page facts |
| LED-VID-0016 | H6-2..H6-5 | Pins and four registers, 9-bit entries, G R B bit positions, 512 x 9, CTA increment after the high byte are on the pages | Unlisted reads ($FF), the upper bits of a CTR high read, the wrap at $1FF and independent halves are not page facts |
| LED-VID-0017 | H6-3 | Content 00h gives 4 and 01h gives 3, fCLOCK 6fsc | Exactly "content 00h and 01h"; nothing for other values (F-23) |

Page errata to record in `cites.txt` facts: H7-2 prints VDW in the data cell of the VCR row; H7-4 says "more than 17" while H7-19 says "17 or more"; H7-25 prints A0 = 0 for the high-byte step. The CONTEXT writer notes (VCR label, the SM (0,1) row) are confirmed on the images.

## Standard Stack

No new product dependency, no new tool (D-177(f)).

| Item | Version here | Role |
|------|--------------|------|
| C11 core and runner | `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror` (gate-base preset); `-fwrapv` on the gate preset only, the sanitize preset has none | Everything under `src` and `tests/conformance` [CODE CMakePresets.json] |
| cmake, ninja | 4.4.3 and 1.13.2 here; the Linux container has 3.28.3 | Presets `gate`, `gate-gcc`, `gate-clang`, `sanitize` |
| ca65, ld65 (cc65) | V2.19 (prints V2.18) | Fixtures; `hucard.cfg` gains the optional `RODATA` segment |
| awk | BWK 20200816 here; gawk and mawk on CI and in `linux-repro` | POSIX awk only: no `gensub`, `asort`, `\y`, `IGNORECASE`, regex intervals |
| shasum or sha256sum | host tool | External recomputation of goldens and the video digest |
| docker | 29.5.2 present | `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` (gcc 13, about 80 s emulated) |
| pdftoppm (poppler) | present | Page images for plan 04-01 (not a gate tool) |

### Package Legitimacy Audit

Not applicable: this phase installs no external package. `gsd-tools package-legitimacy` was not run because there is nothing to check.

## Code-level integration map (what changes, where, and what must be mirrored)

Line numbers are of the files as read this session (before Phase 4). "Plan" is the plan of D-180 that owns the edit.

### Core headers

| File | Change | Plan |
|------|--------|------|
| `src/core/ds_core.h` | `DS_BEHAVIOUR_EPOCH 4u` (line 22) to 5; remove `DS_VDC_STUB_READ`, `DS_VCE_STUB_READ` (lines 290-292, used only in `bus.c:123,125`); `struct ds_run_request` (317-325) gets `struct ds_vid_sink *vid` LAST, add the forward declaration beside lines 312-314 | 04-02 (request), 04-03 (rest) |
| `src/core/ds_core.h` (continued) | `struct ds_instance` gets the new block (see layout notes); `DS_INSTANCE_TILING` (223-229) and the offsetof asserts (190-216, 231-232) updated; document that `ds_irq_pending_query` needs a folded instance | 04-03 |
| `src/core/ds_obs.h` | `struct ds_vid_sink` (header[32], `uint8_t *bytes`, `uint32_t capacity` bytes), `DS_VID_SINK_MAX_BYTES` 64 MiB, `DS_VID_MAX_DOTS` 682 with a `_Static_assert` against `DS_CONST_line_mclk`, `DS_VIDREC_VERSION`; `struct ds_obs_log` gets a `vid` chan; `ds_obs_begin` gets a third sink parameter; new `ds_obs_vid`-style byte emitter | 04-02 |
| new `src/core/ds_vdc.h` | private prototypes: `ds_vdc_advance(inst, stamp, obs)`, `ds_vdc_flag(inst, bit)`, `ds_vdc_read/write`, `ds_vce_read/write`, `ds_vdc_poweron`; includes only stddef, stdint, `ds_core.h` | 04-02 skeleton, filled later |
| `src/core/ds_serial.h` | `DS_SERIAL_VERSION 7u` to 8 (line 22); `DS_REGION_COUNT 14u` to 21 (line 41); the comment listing the regions (line 7) | 04-03 |
| `src/core/ds_snapshot.h` | the size mention "(25021 bytes at serial version 7)" (line 30) | 04-03 |
| `src/core/ds_poweron.h` | `DS_POLICY_SEEDED_V3 = 4`; three new `ds_domain` rows before `DS_DOMAIN_COUNT` (array size of `domain_const[]`); `ds_policy_of`, `ds_policy_is_seeded`, `ds_policy_name`, `ds_policy_version` (v3 returns 3) | 04-03 |
| `src/core/profile_fields.def` | allowed set of `powerup_policy` (line 31) gets `seeded-prng-v3`; not part of the profile hash | 04-03 |
| `src/text/ds_text.h` | `DS_TEXT_FORMAT_VERSION 6u` to 7 (the comment lists the history); header text | 04-03 |

### Core sources

| File | Function and lines | Change |
|------|--------------------|--------|
| `bus.c` | `hw_read` 113-147 | Windows 0 and 1 (lines 122-125) call fold then the device read; `$1403` branch (136) folds first; windows 0 and 1 log a READ (D-149) through `log_dev`, today only window 4 logs (143-145); neither touches `io_buffer` |
| `bus.c` | `hw_write` 150-180 | `default:` (174) splits into VDC and VCE cases that fold then write; the log call at 177 already covers windows 0-5 |
| `bus.c` | `ds_bus_write_phys` 224-229 | unchanged path; comment at 227 ("a stub that drops the byte") |
| `irq.c` | `request_bits` 97-100 | OR in the VDC level `(vdc_status & 0x3F) != 0` as bit 1; `ds_irq_read` and `ds_irq_pending_query` stay const (F-21); the file header comment (lines 8-9) |
| `cpu.c` | `sample_and_enter` 561-572 | `ds_vdc_advance(inst, sample_stamp, log)` before `ds_irq_pending` at line 566 (only in the `i_sample == 0` branch) |
| `cpu.c` | `ds_cpu_step_obs` 684-1091, `ds_cpu_step` 1093 | rename the body to a static function; the public function folds at `start + total - 1` (F-9); `struct ds_bus` initialisers at 513, 675, 690 are positional, add no field |
| `run.c` | `ds_run` 74-129 | line 106 (the test that creates the log when an insn or a dev sink is present) also tests `request->vid`; line 108 passes the third sink; refusals unchanged |
| `obs.c` | `ds_obs_sinks_check` 107-161, `sink_ref_*` 52-91, `chan_*` 163-216 | table-driven over channels (the arrays `s[2]` and the `i < 2u` loops become `s[3]`); keep the check ORDER (structs, capacities, record ranges) so the existing refusal cases stay valid |
| `obs.c` (continued) | - | a variable-length append with a sticky `full` flag (F-2); header 32 bytes for vid vs 16 for the others; never `memcpy` through a NULL record pointer (the UBSan finding of Phase 3 at `obskinds.c:854`) |
| `serial.c` | `region_id` 134, `regions[]` 185-201, `emit_payload` 204-300 | seven rows and seven cases appended after SCHD; `_Static_assert` on each live span; PALT, VRAM, SATB are byte arrays (explicit little-endian for the u16 palette); `R_COUNT == DS_REGION_COUNT` assert at 336 |
| `snapshot.c` | `vctx` (about 150), `dec_*` 57-410, `decoders[]` 553-557, `walk` 420 | seven decoders; `dec_beam` reads `v->time` (set by `dec_time` in pass 1) and checks `line_idx <= time / 1365 + 2` BEFORE any multiplication; `model_values` for the new rows; `dec_powr` accepts policy 4; F-1 bound |
| `poweron.c` | `domain_const[]` 32-37, `ds_policy_*` 77-112, `ds_poweron_apply` 143-155 | three ASCII-packed constants (`VRAM____`, `SATB____`, `PAL_____`), a v3 apply after v2 (every v2 draw first, same order and bytes), palette fill 1,024 bytes then clear bits 1..7 of every odd byte |
| `boot.c` | after line 357-362 | `ds_vdc_poweron(inst)`: `rcr_counter = 16384`, `frame_lines` from `ctx.profile.field[DS_FIELD_lines_per_frame].value` ("262" or "263"); keep `ds_cpu_reset` order; `boot_order.def` unchanged |
| `model_params.def` | 2 rows today (`tma_zero`, `irq_i_sample`) | `vd_irq_phase` and `rcr_origin`; total 4 equals the cap `_Static_assert(DS_MODEL_COUNT <= 4u)` at `ds_core.h:452` |
| `constants.def` | 18 rows today | 16 rows (D-168); each needs an existing ledger entry first (`INV_CONST_LEDGER`), so 04-01 writes the ids; the record prints them and joins `[withheld]` |
| `text.c` | `emit_withheld` 355-390, `emit_constants` 571, `emit_model` 432, header 686 | arithmetic only; `[withheld]` is 26 ids today and becomes about 35 (cap `WH_MAX` 64, `record-lint` rule at most 64) [CO golden]; the record text is 11,370 bytes against `REC_CAP` 16,384, about 12.6 KB after |
| new `vdc.c`, `vce.c`, `beam.c`, `vid.c` | - | names discretionary; no malloc, no globals, no function statics, const tables only, include only stddef, stdint, `ds_core.h`, `ds_obs.h`, `ds_vdc.h`; `core-boundary.allow` unchanged (memset, memcpy, memcmp, memmove only) |

### Instance layout and region notes (F-10)

- Measured: `sizeof(struct ds_instance)` 1,073,912, alignment 8, `slot_type` at 25,325, `rom` at 25,330, `tail_pad` at 1,073,906 [CO compiled probe]. Region payloads add 3 (VCE0) + 1,024 (PALT) + 45 (VDC0) + 33 (BEAM) + 25 (VDMA) + 65,536 (VRAM) + 512 (SATB) = 67,178 bytes; plus 7 frames of 8 bytes the stream grows by 67,234 to 92,255.
- Safest placement for the new block: after `tail_pad` (offset 1,073,912 is 8-aligned), `u64` members first, then `u32`, `u16`, `u8`, then a new explicit trailing pad so the sum is a multiple of 8 (67,178 mod 8 = 2, so a 6-byte pad if no other padding). Keep `rom` followed by `tail_pad[6]` untouched so the `ds_core.h:232` assert and selftest anchor stay valid.
- Flat names, one region each, every member in `DS_INSTANCE_TILING`; the two model bytes are named `vd_irq_phase` and `rcr_origin`.
- BEAM payload (33 bytes): `line_idx` u64, stage, v_state, `v_togo` u16, `display_k` u16, `rcr_counter` u16, `frame_lines` u16, `bg_y` u16, `mwr_active` u16, burst, `line_dirty`, `sprite_pend`, `vd_off` u16, `rcr_off` u16, `lat_div`, `vd_pending`, `vd_irq_phase`, `rcr_origin`. VDMA payload 25 bytes. Totals checked by hand against D-165.
- Event model check: within a line the pair (stage, `vd_pending`) plus the offsets determines the next event: stage 1 means EV_VD is next and EV_RCR is still outstanding iff `rcr_off >= vd_off` (tie goes to EV_VD, D-138); stage 2 means EV_RCR is next. No extra field is needed. DS and DV completions are two more event kinds
  derived from `satb_done_at` and the DMA closed form, so the next-event minimum has five candidates.

### Runner and tests (`tests/conformance`)

| File | Lines and symbols | Change |
|------|-------------------|--------|
| `runner.c` | `kinds[]` 1007-1077; `default_binding[]` 89-92 | one row per new kind: `video`, `video_fault_sweep`, `video_hash_not_verdict`, `vdc_irq_agree`, `vdc_vram`, `vdc_satb`, `vdc_vram_sweep`, `vdc_regmask`, `vdc_advance_pure`, `vdc_fold`, `vdc_snapshot_mid_dma`, `vdc_random_regs`, `vdc_poweron_policy_diff`, `vid_sink_shapes` (names discretionary) |
| `runner.c` (continued) | - | `lines_per_frame` default is "262"; the 263 variants bind `bind.lines_per_frame=263` in the case request. Size 49,673 bytes, headroom is not a problem |
| `runner.h` | key enum (`K_ID` to `K_CHUNKS`), `K_OPTIONAL_MASK` | NO new case key: carry the scene, role and groups in `expect` (precedent `compare=...;restore=N`), so `inventory-rules.awk` (want table, lines 28-34), the key table and `conformance-flatten` are untouched. Declarations of new kinds |
| new `videokinds.c` | - | parses records with hand-written offsets (ER-13), never includes a core record header; builds the stream `"duospool-video" NUL, u32 version, records` and its SHA-256 with `ds_sha256.h` (as `digkinds.c` does); judge ROWS, REL, STRUCT |
| `videokinds.c` (continued) | - | fault sweep F0-F9 on captured bytes; prints `video_sha256=... E0 repeatability` beside the verdict through `set_evidence_detail`; `--video-out ID` refused unless the manifest kind is authored or synthetic (copy the rule of `kind_trace_out_rights`) |
| new `vdckinds.c` | - | the C-level cases of D-175; `vdc_vram` and `vdc_satb` read `inst->vram` and `inst->satb` bytes directly (the D-108 precedent: no peek API) against literals restated from the fixture source |
| `guestkinds.c` | `default_policies` 840; `guest_once` 1023-1110 | default list becomes zero, v3 seed 1, v3 seed 2 (the three strings only); a video case runs with a `vid` sink (and a `dev` sink) in the first run; a lost vid record is `harness_failure` |
| `chunkkinds.c` | `cs_table` 40-45; usage text 482; `CS_SCHED_MAX 8` | tokens `1364`, `1366`, `vframe` (F-20); a video case needs the stream concatenated across calls and compared with `memcmp`, not hashes (D-163) |
| `obskinds.c` | `dl_windows` 1386-1520; `dl_guest` 1262-1375; observer kind; `ds_obs_begin` at 1416 and 1508 | window rule: reads of windows 0 and 1 now log (the read loop at about 1436 keeps only `k == 4u`); expected `aux` for those reads is the latch UNCHANGED; the sink-shape matrix gains the vid channel; `insn_sink_shapes` twin at 169 needs the F-9 placement |
| `stepkinds.c` | `rr_variants` 567-700, `rr_base_request` 541 | vid overlap controls per channel (memory_invalid), `sink_unsupported` for a capacity above 64 MiB, the OLD-size variant (F-16); the `_Static_assert(sizeof dev == sizeof insn)` stays for those two only because the vid struct is larger |
| `snapkinds.c` | 32-45, 81, 848; `snap_poison` 99; hostile corpus | frame table 14 to 21 rows, `SK_VERSION 8`, `SK_EXPECTED_SIZE` 92,255, `RG_*` enum, `SK_FRAMES`; `sk_is_identity` if a pad member is added; flip of each new control field and per-region perturbation by sampling |
| `digkinds.c` | `DK_CAP` 40, `DS_REGIONS_MAX` 147, `kind_digest_stream`, `kind_digest_region` 461 | 32,768 to at least 131,072; 16 to at least 24; `digest_region.toml` tags list, `VRAM` and `SATB` first-and-last only (F-22) |
| `reckinds.c`, `record_skeleton_256k.toml` | 247 and `expect` | `behaviour_epoch = 5`, `record.format_version=7` |
| `powkinds.c`, `profkinds.c` | `profkinds.c:188` | v3 draw-order and domain-independence cases (clone the v2 cases), the allowed string of `powerup_policy` |
| `randprog.c` | 139 lines | VDC and VCE traffic for the 32-program differential: only through the hardware page with MPR0 = $FF; never feed `$1403` timing assumptions |
| goldens | `golden/accept_256k_zero.record.toml`, `accept_384k_seeded.record.toml` | regenerate: `format_version`, `behaviour_epoch`, `[withheld]`, `[constants]`, two more `[model_*]` tables, the state digest |

### Scripts, fixtures, ledger

| File | Change | Plan |
|------|--------|------|
| `scripts/check` | `run_stage video-check stage_video_check` after `fixture-lint-negatives` (it needs no build); a stage function plus a `run_controls` call over `tests/video`; stage count 56 to 57 | 04-05 |
| `scripts/check` (selftest) | three mutations in the style of `mutation-docs-claim-word` (delete the tagged rule lines, the control must stop being met): `rule-video-hash-only`, `rule-video-group-vacuous`, `rule-vexp-class-lint`; the Phase 3 selftest printed 113 SELFTEST lines and gains 3 | 04-05 |
| new `scripts/video-check`, `scripts/lib/vexp-rules.awk` (+ lex) | POSIX awk, the `--expect` and sentinel conventions of the other checkers; `.vexp` grammar fail-closed on unknown keys; at most 512 rows, 400 characters per line, 16 KiB per file | 04-05 |
| `scripts/fixture-build` | variants lines for the two `vdc_irq` images (`-D LINES=262`, `-D LINES=263`, as `mmu_mpr_1m` precedent, `ROMBANKS` 32); write `NAME.vexp.tsv` beside `.observe` and `.classes` (F-5) | 04-03 and later |
| `scripts/lib/fixture-lint-rules.awk` | `islabel` regex, line 56: add `H7-[0-9]+` and `H6-[0-9]+` | 04-01 |
| `scripts/lib/fixture-rules.awk` | line 62: the `manual-cite:` pattern lists the prefixes H8, S8 and BIOS and must also accept H7 and H6 for `verdict_source` of manifest entries (not in the CONTEXT list) | 04-01 |
| `scripts/docs-check` | regex line 125 (F-14); README policy line; `DOCS_SUPPORTED_LIST` requires the README "Supported entries" paragraph to equal the set of `supported` entries at every promotion | 04-01, 04-11, 04-14 |
| `scripts/conformance-run` | `controls_mode` 430-560: optional second column of `EXPECT` (F-4), `filter_cases` includes those ids | 04-06 |
| `fixtures/lib/hucard.cfg` | add `RODATA: load = ROMB, type = ro, optional = yes;` after `CHECKLIT`; adding an optional segment does not change the bytes of images that do not use it, so the existing 21 manifest pins (21 authored images) are unchanged | 04-03 |
| `fixtures/lib/check.inc`, `startup.inc`, `verdict.inc` | do NOT edit: any emitted byte changes every authored image hash. Put spin loops, 24-bit counters and VDC helper routines in a NEW include (for example `video.inc`) that only the video fixtures include | all fixtures |
| `fixtures/manifest/*.toml`, `fixtures/lib/cites.txt`, `TOOLS.md` | one manifest entry per image (12 sources, 13 images with the two `vdc_irq` variants: size and sha256 pins); H7 and H6 legend rows; both manual records (URL, bytes, sha256) | 04-01, each fixture plan |
| `fixtures/src/mmu_mpr.s` and `.checks` | checks 23-26 text and literals (04-03 for 25-26, 04-04 for 23-24); the checks still pass numerically ($00 status, $FF unused VCE reads) but their class text names the superseded stub clause; three manifest pins change twice | 04-03, 04-04 |
| `ledger/entries`, `ledger/REQUIRED` | 17 new files (areas VID and IRQ only), 10 amendments, one REQUIRED line each `ID 04-CONTEXT D-nnn` (the entry `origin` must equal that text) | 04-01 |
| `tests/gate/record-lint` | 16 mini-roots (3 ok, 13 negative), all `format_version = 1` hand-held records: NOT regenerated by the bump; `record-rules.awk` checks key names, table sequence, `NUMBER LEDGER-ID` shape and at most 64 withheld ids, none of which changes [CO] | none |

## Architecture Patterns

### System architecture diagram

```
 guest ROM (ca65) --CPU step--> ds_cpu_step_obs (cpu.c)
   |                              |  start = machine_time
   |  bus accesses                |  total = step core
   v                              |  ds_vdc_advance(inst, start+total-1, log)   <-- fold point 3
 bus.c hw_read / hw_write --------+
   | windows 0,1 and $1403: stamp = start + (last+penalty)*cycle ; ds_vdc_advance(inst, stamp)  <-- fold point 1
   |      |
   |      +--> VDC port (regs, VRAM accessor, DMA state)   VCE window (cr, cta, palette)
   |      +--> dev sink: READ/WRITE records with the fold stamp (opt-in)
   |
 sample_and_enter (cpu.c): ds_vdc_advance(inst, sample_stamp) then ds_irq_pending   <-- fold point 2
   ^                               request_bits = stimulus | timer | (vdc_status != 0)

 ds_vdc_advance(inst, S): loop { e = min(next ladder event, SATB-done, DMA-completion); if e > S break;
                                 process e in time order (ties: EV_LINE, EV_VD, EV_RCR) }
   EV_LINE(k): DMA words due -> vertical machine -> RCR counter -> latch -> VBlank entry (VD, SATB copy)
               -> sprite evaluation (STATE) -> render line (const fn, PIXEL only with a vid sink) -> emit record
   EV_VD / EV_RCR / DS / DV: ds_vdc_flag(inst, bit) applies the enable gate at the event instant
 vid sink (caller memory): header[32] + whole records, drop-newest, sticky full

 runner (tests): video kind -> stream "duospool-video" + records -> ROWS / REL / STRUCT vs build/gate/fixtures/NAME.vexp.tsv
                 fault sweep F0-F9 on the captured bytes; golden digest is E0 and only beside a literal case
```

### Recommended file structure

```
src/core/{vdc.c, vce.c, beam.c, vid.c, ds_vdc.h}   new (names discretionary)
tests/conformance/{videokinds.c, vdckinds.c}        new kinds
fixtures/src/{vce_pal,vdc_reg,vdc_stat,vdc_irq,vce_dot,vdc_dma,vdc_satb,bg_basic,bg_scroll,bg_enab,spr_basic,spr_limit}.{s,checks,vexp}
fixtures/lib/video.inc                              new shared video helpers
scripts/{video-check, lib/vexp-rules.awk}           new stage
tests/video/                                        negative mini-roots of video-check
tests/controls/{10 toys}/{RULE,EXPECT}              D-173
```

### Pattern 1: one fold, three callers, stamps from the instruction stream only
**What:** all VDC and VCE time advances in `ds_vdc_advance`; its target is always a function of the instruction stream (an access stamp, the sample stamp, or `machine_time_after - 1`). **When:** every access to windows 0 and 1, `$1403` and the interrupt sample. **Why it is sound here:** access stamps inside one step are
non-decreasing (a read-modify-write reads one cycle before it writes; a block byte reads in cycle 4 and writes in cycle 6; penalties only add), and the sample stamp (`start + total - irq_sample_cycles_before_end * mclk`, the constant is 1) equals the last data-access stamp, which the tie rule "event time <= stamp comes first"
already orders [CODE cpu.c:561-572, bus.c:88-100, constants.def].
```c
/* Source: [P] shape only; names are discretionary */
uint32_t ds_cpu_step_obs(struct ds_instance *inst, struct ds_obs_log *log)
{
    uint64_t start = inst->machine_time;
    uint32_t total = cpu_step_core(inst, log);
    ds_vdc_advance(inst, start + (uint64_t)total - UINT64_C(1), log);
    return total;
}
```
The stored invariant `next_event_time >= machine_time` holds after every step, so a restore at any stop and chunked runs equal one run by construction (D-137).

### Pattern 2: R1 render at EV_LINE with a const function
The record of a line is a pure function of the state at its EV_LINE. One function takes `const struct ds_instance *`, writes a stack record (about 3 KiB) and an out-parameter, and cannot write machine state; the STATE part (BYR counter, sprite evaluation with per-dot opacity, OR and CR detection) always runs, the PIXEL part
only with a vid sink attached. `line_dirty` clears at every EV_LINE with or without a sink (D-164).

### Pattern 3: variable-length sink with a sticky full flag
Per channel: header written at the start and end of an accepted call and never read; `count`, `lost`, `seq`, `bytes`; after the first record that does not fit every later record is counted lost (F-2). The check order in `ds_obs_sinks_check` is kept (structs, capacities, record ranges) and becomes table-driven.

### Pattern 4: restore as validate-then-commit with a time-window guard
Seven decoders, each reading its fields in stream order and either judging (pass 1) or storing (pass 2). The time guard is checked before any multiplication: `line_idx <= time / 1365 + 2`, the pending event time within `[machine_time, machine_time + 2 * 1365)`, `satb_done_at` within `[machine_time, machine_time + 4096]` when
valid, `dma_start <= machine_time` while running; this prevents a hostile snapshot from making the next fold loop for up to 2^63/1365 iterations.

### Pattern 5: judge roles and the fault sweep
`literal` (hand-derived rows), `struct` (formulas from separate functions), `golden` (one pinned digest, E0, valid only beside a `literal` case of the scene). The fault sweep mutates the CAPTURED records in memory (F0-F9) and requires exactly the groups that declare the fault in `kills=` to flip.

### Pattern 6: guest measurement with a fixed-cost loop
A foreground loop with no data-dependent branch (a 24-bit add through clc/adc, 6 cycles per iteration at high speed = 18 master clocks), a handler that samples the counter before it reads status, results stored as two observe rows per 16-bit value in DSV1 (+80..+DF, 96 rows). The "scanline" the guest records is the programmed
RCR value; the phase is iterations converted by a printed formula; no absolute line is observed (D-174).

### Anti-Patterns to Avoid
- **Editing `check.inc`, `startup.inc` or `verdict.inc`:** changes every image hash; use a new include.
- **A new runner argument for `.vexp`:** six call sites; use the `.observe` channel (F-5).
- **Hashing as evidence:** a video hash is E0 and valid only beside a literal case that the fault sweep and the toys show live (ER-11).
- **Rendering inside the run loop per dot without a sink:** every sweep, chunk and snapshot run would pay for it; the PIXEL part is gated on the sink pointer only.
- **A reference renderer, geometry oracle or vdata packer in the repository:** forbidden by ER-7 and D-171.

## Don't Hand-Roll

| Problem | Do not build | Use instead | Why |
|---------|--------------|-------------|-----|
| SHA-256 of the stream and of goldens | a second hash | `ds_sha256.h` in the runner; `shasum -a 256` outside | one implementation, one external check |
| Power-on draws | a new PRNG | `ds_stream_init` and `ds_stream_fill` (splitmix64, `poweron.c`) with three new domain constants | same derivation as v1 and v2; additive |
| Data file parsing | a C parser for hand-edited text | `toml-subset.awk` machinery through `fixture-build`/`video-check`, TSV into the runner | one parser of hand-edited data (D-51) |
| Check macros and literal table | new verdict plumbing | `CHECK_LIT`, `CHECK_GUEST`, `CHECK_OBSERVE`, `LIT`, DSV1 | the sweep and class lint already understand them |
| Sink overlap and alignment tests | per-sink copies | the table-driven check in `obs.c` (`ranges_overlap`, alignment 8) | one rule for three channels |
| Fixture mutation | a video-specific sweep for guest checks | `mutate_sweep` for DSV1 literals; the record fault sweep for judge groups | each covers its own layer |

**Key insight:** every piece of machinery this phase needs exists in the repository; the work is wiring a third channel and a seven-region state through it, with evidence rules that make each wire fail visibly.

## Common Pitfalls

### Pitfall 1: the golden and digest churn cascade
**What goes wrong:** the bump of serial 7 to 8, epoch 4 to 5 and text 6 to 7 turns a long list red at once, and the list in D-165 is shorter than the real one (F-3). **The real red list (measured):** the five `digest_stream_*` cases (`version=7`, 14 frames, literal digests), `digest_region`, `snapshot_equals_digest`,
`snapshot_hostile` and `snapkinds.c` constants, `record_skeleton_256k`, the two accept goldens, `reckinds.c:247`, `dev_log_*`, the `DK_CAP` and `DS_REGIONS_MAX` capacities, `profkinds.c:188`, `guestkinds.c:840` and the `ds_snapshot.h` size text.
**How to avoid:** (1) rehearse the regeneration once outside the repository before 04-03 (a throwaway script is allowed, ER-7: only the committed literal is judged); (2) for the policy-v1 digest cases the new frames are VCE0 3 bytes, PALT 1,024, VDC0 45, BEAM 33, VDMA 25, VRAM 65,536, SATB 512 bytes of zeros EXCEPT BEAM, which
holds `rcr_counter` 16384 and `frame_lines` 262 (F-11); (3) check that each OLD literal equals `shasum -a 256` of the OLD stream before computing the new one; (4) the optional cascade cut of D-165 (full 64-hex literal in `digest_stream_prefix` and the two goldens, region-payload compare in the other four) is allowed; (5) a
second (correction) bump costs the same cascade, so the layout review of 04-03 task 1 is the cheap place to catch a missed field.
**Warning signs:** a red case outside the list, a `harness_failure` in `digest_stream` about a capture overrun (`DK_CAP`), `regions=21` against a 14-tag list.

### Pitfall 2: the +1 VDC/VCE penalty and the stamp the fold sees
**What goes wrong:** the stamp of an access is `start + (last + penalty) * cycle_mclk` and `penalty` already includes the access being made (`charge_penalty` runs before `access_stamp`, `bus.c:119-120`, `154-155`); computing the fold target before charging puts it one cycle early (12 master clocks at low speed). A block byte
that touches the VDC costs 7 cycles, not 6, and its read (cycle 4) and write (cycle 6) are two stamps one instruction apart. A read-modify-write on a VDC address reads one cycle before it writes.
**How to avoid:** take the stamp from `access_stamp` after `charge_penalty`, never recompute it; the sample stamp is the start of the last cycle (`irq_sample_cycles_before_end` = 1) and equals the last data-access stamp, so the tie rule orders them; fixtures that judge latch points keep their writes at least 250 master clocks
from both line boundaries (D-170), and the chunk schedules 1364 and 1366 put a stop one clock either side of a line event.
**Warning signs:** `chunk_equiv_video_*` red only at schedule 1364 or 1366; a status read that clears a flag set one event later.

### Pitfall 3: existing fixtures that touch the VDC and VCE
**What goes wrong:** the stubs disappear and previously inert writes become register writes. Measured uses [CODE fixtures/src]: `mmu_mpr` checks 23-28 read `$6000`, `$63FF`, `$6400`, `$67FF` (status 0, MSB data of the read buffer 0, VCE CR and the unused read both $FF: the numbers survive, the class text "no device yet" does
not); `mmu_mpr` also writes `$6001` (ignored) and `ST0 #$22` (AR = 2); `cpu_huc` checks 54-56 run `ST0 #$A5`, `ST1 #$A6`, `ST2 #$A7`, which now select AR = 5 and write CR = `$07A6` (IE bits OR and RR on, BB on, VD off, SB off), then `ST0/1/2 #$E8` (BYR = `$0E8`) and `ST0 #$00`; with all vertical registers zero the cycle is 4
lines and the RCR counter is 64 to 67 against RCR = 0, so no flag and no IRQ1 arises; `cyc_probe` reads `$0000`, `$07FF` and a zero-page-mapped window and writes `$0400`; `spd_speed` makes 256 and 1,024 reads of `$0000` and `$0400` at both speeds; `st_bypass` only writes; `irq_ctl`, `tim_timer`, `pad_2btn` read `$1403` (VDC
level 0).
**How to avoid:** D-178 plan: verify each passes and reads no VDC data, re-run under v3; keep every VDC and VCE register and the beam state zero under every policy (a drawn CR or IE could raise IRQ1 in fixtures that know nothing of the VDC).
**Warning signs:** an interrupt in `irq_ctl_*` that only appears under a v3 seed.

### Pitfall 4: `dev` sink cascade and the exact-list kind
**What goes wrong:** D-149 logs READS of windows 0 and 1; `dev_log_records` requires `dcount == nitems` for the listed records (`obskinds.c:1325`), and `cyc-probe` makes four such reads (`m_vdc_rd`, `m_vce_end`, `m_zp_rd`, `m_vdc_h`), so the case lists four more `rec=...:R:...` items with literal returned values (status $00,
VCE unused $FF, zero-page-mapped status $00, status $00); `dev_log_windows` expects only window-4 reads (`obskinds.c:1436`) and its `aux` for the new records is the UNCHANGED latch.
**How to avoid:** amend both in the tracer with literals written from the rule, not from a run (E0 stays E0).

### Pitfall 5: layout, padding and tiling
**What goes wrong:** see F-10. A u8 followed by a u16 inside a region gives implicit padding; a pad member outside every region is poisoned by `snap_poison` and not restored, so `snap_first_diff` reports it; `tail_pad[6]` is an anchor of a selftest.
**How to avoid:** group by size, one region per contiguous live span, explicit pad members named like `tail_pad` and listed in `sk_is_identity`; run `sh scripts/check --selftest mutation-tiling-entry` after any layout edit.

### Pitfall 6: strict C11 under gcc 13, and sanitizers
**What goes wrong (Phase 3 findings):** `strtok_r` is not declared in `-std=c11` (the build failed on the first hosted run); `snprintf` into a small buffer with an unbounded `%s` fails `-Werror=format-truncation` even when the length was checked elsewhere (bound the precision where the compiler sees it: `%.2s`); `memcpy(dst,
NULL, 0)` is a UBSan finding on Linux gcc (`obskinds.c:854`); the sanitize preset has no `-fwrapv`, so every shift and every 9-bit or 16-bit wrap must use unsigned operands widened first and explicit masks; `-Wconversion` needs a cast on every narrowing assignment; `-Wshadow` flags locals that reuse a name; no `0b` literals, no
statement expressions, no zero-length arrays under `-pedantic`.
**How to avoid:** every plan that edits C runs `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` before its closing commit (D-177(a)); an unavailable container is "unverified", never passed.

### Pitfall 7: cost multiplier of every new case
**What goes wrong:** a case runs far more often than once. Per gate run it executes in `conformance`, in `sanitizers` (ASan and UBSan, slower), and twice in `determinism` (the runner is run twice with `--records`); CI runs the gate twice (gawk, then mawk) and builds and runs `conformance` again under gcc and clang: about 10 executions per job [CO scripts/check, determinism-run, ci-timings].
**Measured here (loaded machine, load about 13):** one runner pass over 236 cases 13.9 s CPU; the random cases `observer_random` and `chunk_equiv_random32` 2.0 s each; sweeps 0.3-0.75 s; `snapshot_hostile` 0.13 s for 2,034 assertions at 25 KB snapshots (expect about 3.7 times for 92 KB); a 6M-clock guest case 0.03 s; one digest of 92 KB is about 0.4 ms [P].
**How to avoid:** apply the D-176 staging levers in order only when a trigger fires; measure at 04-03, 04-11 and 04-14 with `sh scripts/check --timing`.

### Pitfall 8: toy anchors
**What goes wrong:** a toy rule needs `file=` a `src/core/*.c` file and an anchor that occurs exactly once; `.def` files and headers cannot be mutated (`CMakeLists.txt:79-99`). The tables named in D-173 (divider 4 and 3, counter base 64, slot limit 16) must therefore be one statement each in a `.c` file, not constants of a header or `.def`.
**How to avoid:** write each mutable render decision as one distinct statement (D-155): the divider select array in `vce.c`, the RCR set in `beam.c`, the slot `min` in `vid.c`, the palette store in `vce.c`, the status clear in `vdc.c`, the fold call line in `cpu.c`. Ten new toys make 26 in all; the toy cases run only their `EXPECT` ids.

### Pitfall 9: the hucard.cfg budget and shared includes
**What goes wrong:** ROMB is `$F000-$FFF5` = 4,086 bytes shared by CHECKLIT (5 + 2n bytes, 197 for 96 checks) and the new RODATA, so a 96-check fixture has 3,889 bytes for tables, not "about 3.8 KiB" of 3,891; CODE is 4,096 bytes. A change to `check.inc`, `startup.inc` or `verdict.inc` that emits any byte changes all 21 existing image hashes.
**How to avoid:** new routines go in `video.inc`; budgets are checked by `ld65` itself (an overflow is a link error, fail-closed).

### Pitfall 10: guest evidence width and the first fold
**What goes wrong:** DSV1 has 96 observed bytes, so a 16-bit measurement is two rows; a fixture cannot keep a private log (D-124). The line-0 record at time 0 is produced by the FIRST fold inside the first `ds_run`; a sink attached in a later call never sees earlier lines (D-162); a fold run by a test helper with a bus whose
log has no vid sink (as `dl_windows` does at machine time 0) consumes line 0 silently.
**How to avoid:** evidence runs attach the vid sink in the first call and count records against the closed form `floor((M - 1 - phase) / 1365) + 1`; fixtures judge a window that starts after their own initialisation.

### Pitfall 11: `.vexp` and awk portability
**What goes wrong:** the checker runs under BWK awk (macOS), gawk and mawk (CI, `linux-repro`); the existing scripts are POSIX awk only. At most 512 rows, 400 characters per line, 16 KiB per file; unknown keys fail closed; a ROWS selection must hold at least 3 distinct indices and a transition per line set (non-vacuity lint).
**How to avoid:** clone the conventions of `fixture-lint-lex.awk` and `input-rules.awk` (sentinel line, `--expect`, `STAGE2_DEAD`), test the negatives under the second awk in the selftest.

### Pitfall 12: chunk tokens and the `frame` name
See F-20. A video "frame" chunk must be a multiple of the line length to be a frame; the Phase 3 token is not.

### Pitfall 13: the policy list and the v3 guard
**What goes wrong:** the default guest policies change to zero, v3 seed 1, v3 seed 2; v3 repeats every v2 draw first, so Phase 3 fixtures see the same CPU, MPR and timer start states plus random VRAM, SATB and palette they never read. A video fixture that reads an uninitialised palette entry passes under zero and would fail under v3.
**How to avoid:** identical per-check bytes under zero and two v3 seeds are the guard; `vdc_poweron_policy_diff` needs one negative control that never initialises the palette and must DIFFER between zero and v3.

## Code Examples

Shape of the closed forms the fold relies on (all unsigned, no per-clock loop) [P]:
```c
/* lines recorded after a run to machine time m (m > 0): independent literal of D-137 */
static uint64_t lines_after(uint64_t m, uint64_t phase)
{
    return m > phase ? (m - UINT64_C(1) - phase) / UINT64_C(1365) + UINT64_C(1) : UINT64_C(0);
}
/* VRAM-VRAM words due at time t: never a loop over dots; at most 65,536 */
static uint32_t dma_words_due(uint64_t t, uint64_t start, uint32_t word_mclk, uint32_t done)
{
    uint64_t due = t >= start ? (t - start) / (uint64_t)word_mclk : UINT64_C(0);
    return due > UINT64_C(65536) ? UINT32_C(65536) - done : (uint32_t)due - done;
}
```
The fixture spin loop (6 cycles at high speed, no data-dependent branch) and the handler that samples before it reads status follow D-174; their literal conversion (iterations to master clocks) is `iterations * 18` and is printed in the evidence file, never asserted as an absolute line.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Project gate: sh and awk stages in `scripts/check` plus the C conformance runner `build/gate/ds_runner` whose RESULT lines `conformance-ingest` judges; no third-party framework |
| Config | `CMakePresets.json`, `tests/conformance/REQUIRED`, `tests/conformance/cases/*.toml`, `fixtures/manifest/*.toml`, `fixtures/src/*.{s,checks,vexp}`, `tests/controls/*/{RULE,EXPECT}` |
| Quick run (one case, about 0.03-0.5 s) | after `cmake --build --preset gate`: `build/gate/ds_runner --cases build/gate/cases.tsv --evidence-dir build/gate/ev-quick --profile-tsv build/gate/r1.tsv --rules-fields build/gate/rules-fields.txt --oracle build/gate/oracle.tsv --bank-oracle build/gate/bankclass.tsv --inputs build/gate/inputs.tsv` |
| Quick run, continued | add `--opcode-tsv tests/oracle/huc6280-opcodes.tsv --marker-seed 7 --golden-dir tests/conformance/golden --only ID`; the `.tsv` files exist after one `sh scripts/check --stage conformance`; fixtures need `sh scripts/fixture-build` first |
| Quick run (stages) | `sh scripts/check --stage conformance` (about 33 s CI, 31 s local before Phase 4), `--stage video-check`, `--stage fixture-build`, `--stage fixture-lint`, `--stage fixtures`, `--stage inventory`, `--stage docs-claims` |
| Full suite | `sh scripts/check` (single local command, CI mirrors it), then `sh scripts/check --selftest` for every new rule, `dash scripts/check` and the second awk at 04-11 and 04-14 |
| Strict C11 and Linux | `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` per C plan; `gate-gcc gate-clang core-boundary-gcc` per wave end; all six parts at 04-11 and 04-14 |
| Baseline | Phase 3 close: 56 stages; local run A 193 s wall, 129 s user (loaded machine); CI job 342-344 s (gate 119 s, second awk 104 s) [CODE docs/ci-timings.md]; one runner pass 13.9 s [CO] |

### Phase Requirements to Test Map

Case ids are the proposals of D-170 to D-176; "literal" names the independent source and its class (ER-4, ER-10).

| Req | Behaviour proved | Automated check | Literal source (class) | Control that makes it fail | Wave |
|-----|------------------|-----------------|------------------------|----------------------------|------|
| VID-01 | BG tile decode, BAT bank and code, palette lookup, border index 256 | `bg_basic` (video, role literal; ROWS solid group; separate asymmetric group) | solid group MAN H7-14, H7-18 (hand-derived ROWS in `.vexp`); asymmetric group ASSUMED LED-VID-0010 | fault sweep F0-F2, F7; toy `vid-bit-order` (asymmetric case red, solid case stays pass: needs F-4) | W5 |
| VID-01 | Scroll write lands on the next line; wrap; BYR+1 | `bg_scroll` (ROWS, REL scroll shifts) | MAN H7-7 next-line statements; direction and wrap ASSUMED | toy `vid-scroll-latch` (scroll applied to the current line) | W5 |
| VID-01 | BB, SB, burst frame, backdrop | `bg_enab` | MAN H7-6 (burst, next frame); `bg_off_index` ASSUMED LED-VID-0010 | fault sweep F7; `video_hash_not_verdict` | W5 |
| VID-02 | Both widths, three heights, flips, bank, SPBG, order, clipping, data via SATB DMA | `spr_basic` | MAN H7-20 to H7-24 (layout, CGY table, SG +16, VD bits); leftmost-bit ASSUMED | fault sweep F1, F2; toy `vid-spr-limit` | W7 |
| VID-02 | 16 slots, overflow, collision, IE gating, flags into guest RAM | `spr_limit`; OR and CR rows of `vdc_stat` | MAN H7-19 (16 and 18 sprites, `2(e-2)`: e = 8 gives 12, `2d+1`: d = 28 searches 57), H7-4(a); 17th and transparent collisions ASSUMED | toy `vid-spr-limit` (16 to 17); status reads in guest RAM | W7 |
| VID-03 | Port decode, AR 5 bits, IW increments, read-ahead, write latch, OOB, ST triple, TIA upload and TAI readback | `vdc_reg` (guest, about 36 checks); `vdc_vram` (runner reads `inst->vram`) | MAN H7-3, H7-5, H7-7, H7-25, H8-8; GUEST ROM tables computed differently; ASSUMED LED-VID-0006, -0007 | toy `vrr-no-readahead`; mutate sweep of `vdc_reg` | W3 |
| VID-03 | Accessor total over 65,536 addresses; register masks | `vdc_vram_sweep` (sanitizers), `vdc_regmask` | field widths MAN H7-5..H7-11 | sanitizer abort; mask literal flip | W3 |
| VID-03 | VRAM-VRAM DMA: data moved, SI/D and DI/D, M-1, overlap, cancel, DV | `vdc_dma` (about 18 checks), `vdc_advance_pure` (naive per-dot reference vs closed form), `vdc_snapshot_mid_dma` | MAN H7-11, H7-12, H7-4(e); duration VL1 ASSUMED LED-VID-0014 | fixture-level expected-red "instant transfer"; sweep | W4 |
| VID-03 | VRAM-SATB DMA: 256 words, DVSSR MSB trigger, DS after VD | `vdc_satb` (about 10 checks) + runner kind `vdc_satb` | MAN H7-12(b), H7-20; copy instant ASSUMED LED-VID-0013 | sweep; second witness is `spr_basic` | W4 |
| VID-04 | Status layout, read clears, flag gating, IRQ1 level, `$1403` bit 1 | `vdc_stat` (about 16 checks) | MAN H7-4, H7-6, H7-11; level ASSUMED LED-IRQ-0009 | toy `status-read-keeps-flags` | W4 |
| VID-04 | VBlank and RCR positions recorded by the guest at the programmed RCR; ratios | `vdc_irq` (262 and 263 variants), `vdc_irq_agree` (E0/E2), `model_param_vdc` | ratio RCR=a+8 vs a to a+1: GUEST (no assumed constant); period 1365 and N*1365: ASSUMED LED-VID-0001, ER-3 | toy `vid-rcr-base` (offset group red, ratio case stays pass: F-4) | W4 |
| VID-04 | (cont.) the VD gap and the RCR hit line separate `vd_irq_phase` and `rcr_origin` | the same cases | gaps 245, 341 and 1,610 clocks at HDS=2, HDW=31, div 4: ASSUMED LED-IRQ-0001, -0010 | tolerance +-1.5 iterations of 18 clocks | W4 |
| VID-04 | Line count closed form; frame length constant | `vid_frame_lines`; STRUCT rows | `floor((M-1-phase)/1365)+1` and `(frame*frame_lines+line)*1365` from separate runner functions (E2) | toy `fold-skipped-at-step-end` (chunk_equiv at 1364/1366) | W2, W4 |
| VID-05 | 512 x 9-bit entries round trip, CTA increment after the high byte, windows, divider per CR | `vce_pal` (about 28 checks), `vce_dot` | MAN H6-3 (00h /4, 01h /3), H6-5; bijection GUEST (F-25); 02 and 03 ASSUMED LED-VID-0002 | toy `vid-palette-mask` (palette store keeps 8 bits), toy `vid-dot-div` | W2, W4 |
| VID-05 | Geometry from HDS and HDW, clip, mid-frame change visible | `vid_active_window`, `vce_dot` STRUCT rows | programmed values (GUEST) and ASSUMED origin LED-VID-0005 | fault sweep F5, F6; STRUCT "more than one width" | W4 |
| VID-06 | One record per line, origins strictly increasing by 1365, mid-frame change represented, never normalised | `video` kind: STRUCT rows in every video case | closed forms from separate runner functions (E2) | fault sweep F3, F4, F8, F9; null-renderer control | W2 onward |
| VID-06 | Sink drop-newest in whole records, header count, lost, seq, bytes; poisoned header; refusals | `vid_sink_shapes`, extended `observer`, `stepkinds` vid variants | rules written in the case (E0) | `trace-hook-mutates`-style toy `vid-emit-mutates` | W1, W2 |
| VID-06 | Chunk invariance, restore at stops, snapshot of mid-line and mid-DMA states, hostile corpus | `chunk_equiv_video_*` (5 schedules), `snapshot_*` video cases, `snapshot_hostile` | equality of outputs (E0) | toy `fold-skipped-at-step-end`; selftest `mutation-restore-skip-field` stays alive | W2, W5 |
| VID-06 | A hash never stands alone | `video_hash_not_verdict`, role rules of `video-check` | a faulted stream passes its own golden and fails the literal | selftest mutations removing the three rules | W3 |

### Negative controls and independent oracles

| Item | What must go red | Independent oracle |
|------|------------------|--------------------|
| Renderer broken | null renderer, all-border, all-index-0 (D-172); the ten toys | hand-derived `.vexp` literals (ER-7: no reference renderer) |
| Judge vacuity | `VIDEO_GROUP_VACUOUS`, `INV_VIDEO_GROUP_UNKILLED`, `INV_VIDEO_HASH_ONLY`, `INV_TOY_NO_LITERAL`; the three selftest mutations | the fault sweep F0-F9 on captured records |
| Guest checks | `mutate_sweep` flips every DSV1 literal; policy-diff control | manual pages (MAN), in-guest arithmetic (GUEST), ledger ids (ASSUMED) |
| Power-on | `vdc_poweron_policy_diff` with a never-initialised palette | zero versus v3 seeds |
| Fold and restore | `vdc_fold` (random chunk sequences against one jump, accesses at event time +-1), `vdc_advance_pure` | naive per-dot DMA reference; closed line count |

### ROADMAP Phase 4 success criteria

| SC | Check | Notes |
|----|-------|-------|
| 1 | `bg_basic`, `bg_scroll`, STRUCT rows (record fields), next-line latch ROWS | the record carries values, divider, active window, origin, frame line count; literals come from the fixture's own data |
| 2 | `spr_limit`, `vdc_stat` OR and CR rows, `vdc_reg`, `vdc_satb`, `vdc_dma` | sprite half moves to Phase 4.1 if the cut line is taken at 04-11; the DMA and register half closes in Phase 4 |
| 3 | `vdc_irq` x2, `vdc_irq_agree` | labelled modelling assumption (LED-IRQ-0001, -0010, LED-VID-0001); no absolute line observed |
| 4 | `vce_pal` (all 512 entries), `vce_dot` mid-frame change | differing `div`, `active_x`, `active_w` between consecutive records |
| 5 | `video_hash_not_verdict`, role rules, fault sweep, toys | ER-11 |

### Sampling Rate

- **Per task commit:** the inner-loop `ds_runner --only ID` for the case under work plus the narrowest stage (`video-check`, `fixture-lint`, `conformance`).
- **Per plan close:** `sh scripts/check` green with goldens regenerated and the plan's ledger pins edited; `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` for C plans.
- **Per wave end:** `gate-gcc gate-clang core-boundary-gcc`.
- **Phase gate (04-11, 04-14):** all six Linux parts, `sh`, `dash` and the second awk, the selftest, the timing record, the verification report.

### Wave 0 Gaps (must exist before the code that needs them)

- [ ] `H7-` and `H6-` labels accepted by `fixture-lint-rules.awk:56` and `fixture-rules.awk:62`, rows in `cites.txt`, both manuals in `TOOLS.md` (04-01).
- [ ] `docs-check` claim words and policy line (F-14) with its negative control (04-01).
- [ ] 17 ledger entries and 10 amendments, each constants row's id present before `constants.def` lands (04-01).
- [ ] Sink channel table with a `vid` channel and no producer; `ds_cpu_step_obs` fold placement (04-02).
- [ ] Capacities `DK_CAP`, `DS_REGIONS_MAX`, the snapshot hand tables, the two goldens (04-03).
- [ ] `EXPECT` second column in `controls_mode` for stays-green toys (before 04-06).
- [ ] `.vexp.tsv` channel in `fixture-build` and the `video-check` skeleton (04-05).
- [ ] `RODATA` segment in `hucard.cfg` and `fixtures/lib/video.inc` (04-03).

### Fixture inventory as planned (D-170)

| ROM | Plan | Checks (about) | Judge |
|-----|------|----------------|-------|
| `vce_pal` | 04-03 | 28 | guest + runner `video` literals |
| `vdc_reg` | 04-04 | 36 | guest + `vdc_vram` |
| `vdc_stat` | 04-06 (+04-08, 04-13) | 16 | guest |
| `vdc_irq` (262 and 263 images) | 04-06 | 30 | guest spin counters + `vdc_irq_agree` |
| `vce_dot` | 04-07 | not stated | `video` ROWS and STRUCT |
| `vdc_dma` | 04-08 | 18 | guest |
| `vdc_satb` | 04-08 | 10 | guest + `vdc_satb` |
| `bg_basic`, `bg_scroll`, `bg_enab` | 04-09, 04-10 | not stated | `video` ROWS and REL |
| `spr_basic`, `spr_limit` | 04-12, 04-13 | not stated | `video` ROWS and guest status reads |

Each ROM also needs a manifest entry, a `.checks` file with class tags, one `mutate_sweep_*` case and a REQUIRED line; about 60-70 new ids in all (D-176).

## Per-plan decomposition feasibility (14 plans, 8 waves)

Hot files touched by many plans: `tests/conformance/REQUIRED`, `runner.c` (`kinds[]`), `runner.h`, `CMakeLists.txt` (source list), `scripts/check`, `ledger/entries/*` pins, `fixtures/manifest`.

| Plan | Main files | Shares hot files with | Parallel claim | Risk |
|------|-----------|-----------------------|----------------|------|
| 04-01 | `ledger/*`, `fixtures/lib/{cites.txt,TOOLS.md}`, `scripts/lib/{fixture-lint-rules,fixture-rules}.awk`, `scripts/docs-check`, README, `tests/docs`, `tests/gate/fixture-lint` | none of 04-02 | parallel-safe with 04-02: TRUE (disjoint) | low; the `fixture-rules.awk` regex is missing from the CONTEXT list |
| 04-02 | `ds_core.h`, `ds_obs.h`, `obs.c`, `run.c`, `cpu.c`, `bus.c`, `ds_vdc.h` skeleton, `stepkinds.c`, `obskinds.c`, new sink cases, `REQUIRED` | none of 04-01 | TRUE | moderate: `ds_obs_sinks_check` refactor must keep refusal order and codes; digests unchanged proves the skeleton is inert |
| 04-03 | everything of layout, serial, snapshot, poweron, boot, text, VCE, grid, records, fold, runner kinds, goldens, `vce_pal`, 3 toys | all | sequential; owns the whole red list | HIGH: far over 6 tasks as listed (F-17); the single bump lands here |
| 04-04 | `vdc.c`, `bus.c` window 0, `vdckinds.c`, `vdc_reg`, `mmu_mpr` checks 23-24, `REQUIRED`, `runner.c` | 04-05 on `REQUIRED`, `runner.c`, `runner.h` | "parallel-safe" is FALSE at file level (F-18) | moderate |
| 04-05 | `videokinds.c`, `scripts/video-check`, `scripts/lib/vexp-rules.awk`, `scripts/check`, `tests/video`, selftest mutations, `fixture-build` | 04-04 as above | as above | moderate: the `.vexp` channel decision (F-5) |
| 04-06 | `beam.c` ladder, `irq.c`, `cpu.c` sample fold, models, `vdc_stat`, `vdc_irq`, `conformance-run` EXPECT column | 04-07, 04-08 (beam code) | sequential | HIGH on spin-counter fixture design; F-1, F-4, F-21 |
| 04-07 | beam geometry, divider latch, `vce_dot` | 04-06, 04-08 | sequential | moderate |
| 04-08 | DMA closed form, `bus_free`, `vdc_dma`, `vdc_satb`, `vdc_advance_pure` | 04-06, 04-07 | sequential | moderate: tie order with EV_LINE and the cancel at display line 0 |
| 04-09 | `vid.c` BG, `bg_basic`, toy `vid-bit-order` | 04-10 | sequential | moderate: the stays-green case (F-4) |
| 04-10 | scroll latch, BB/SB, burst, `bg_scroll`, `bg_enab`, chunk and snapshot on video | 04-09 | sequential | moderate |
| 04-11 | measurement, goldens, promotion pass, `vdc_random_regs`, `vdc_poweron_policy_diff`, six Linux parts | all | sequential | cut-line decision (F-8) |
| 04-12, 04-13 | `vid.c` sprites, `beam.c` flags, `spr_*`, toy `vid-spr-limit` | each other | sequential | moderate; the cut is clean at the code level (no layout change after 04-03; SATB data already moves in 04-08) |
| 04-14 | README, ledger statuses and pins, E1 promotions, verification report | all | sequential | `DOCS_SUPPORTED_LIST` must equal the promoted set |

**Risk of the one bump landing in the tracer.** The 33-byte BEAM, 25-byte VDMA and the scalar fields cover every behaviour decision I traced (D-138 to D-160): per-line latched values are read at EV_LINE or kept as `vd_off`, `rcr_off`, `lat_div`; the next event follows from (stage, `vd_pending`, offsets) and the SATB and DMA
fields. I found no missing field. The residual risks are layout mechanics (F-10, F-11) and the decision-level issue F-1, not a missing behaviour field. Mitigation: one task-1 review that walks each of D-138 to D-160 and lists the state its next event reads that is not a register; a scripted dry run of the regeneration; the
recorded correction bump stays allowed (D-165).

**Cut line.** Sprites touch only the renderer, `sprite_pend` and the record bytes that stay zero until 04-12 and 04-13; the layout is final after 04-03. If 04-11 is the close-out, VID-02 and the OR and CR rows of VID-04 and VID-06 move to Phase 4.1, which also changes ROADMAP success criterion 2 (sprite half) and the README wording; E1 candidate LED-VID-0011 cannot be promoted at the cut.

## Security Domain

### Applicable ASVS categories (level 1)

| Category | Applies | Control |
|----------|---------|---------|
| V1 Architecture | yes | `core-boundary-check` (no allocator, no float, no writable symbol), const render function |
| V2-V4 Authentication, Session, Access | no | no users or network |
| V5 Input validation | yes | validate-then-commit restore with the time-window guard; table-driven sink checks; `.vexp` grammar fail-closed; refusals instead of guesses |
| V6 Cryptography | integrity only | existing SHA-256, no custom crypto |
| V7 Error handling and logging | yes | stdout stays ids, outcomes, sizes, hashes; video files only under `--video-out` for authored or synthetic fixtures |
| V8 Data protection | yes | no manual page text or image, ROM or capture committed (ER-9, rights-check) |
| V12 Files and resources | yes | sink maximum 64 MiB, 1,024-event and 64 KiB script caps stay; `.vexp` 16 KiB and 512 rows |
| V14 Configuration | yes | workflow rules unchanged; no new tool |

### Known threat patterns

| Pattern | STRIDE | Mitigation |
|---------|--------|------------|
| Hostile snapshot with a far-behind next-event time makes the next fold loop for up to 2^63/1365 iterations | DoS | pass-1 window rule before any multiplication; hostile corpus flips every new control field and samples stops |
| A restore whose `v_togo`, `display_k`, offsets or counters are outside their domain | Tampering | per-field domain checks (with the F-1 bound) |
| Guest-controlled VRAM, SATB and DMA addresses and lengths | Tampering, DoS | one bounds-checked accessor pair, explicit unsigned wrap, DMA at most 65,536 words per transfer, SATB scan at most 64, dots at most 682, sanitizer sweep of all 65,536 addresses |
| Host sink memory overlapping the instance, request, result or another sink; misaligned or NULL records; wrapping range | Tampering, Elevation | `memory_invalid` per channel, header output-only and never read back, overlap controls per channel |
| Video file output of a commercial image | Information disclosure | `--video-out` refused unless the manifest kind is authored or synthetic |
| `.vexp` or `expect` text injection into awk or TOML | Tampering | one strict parser, 400-character lines, unknown keys refused, no shell expansion of data |
| Integer truncation in 9-bit and 16-bit palette and coordinate arithmetic | Tampering | explicit masks and unsigned widening, UBSan without `-fwrapv`, `-Wconversion -Werror` |

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| ca65, ld65 | fixtures | yes | V2.19 (prints V2.18) | none: missing is exit 2, never a skip |
| cmake, ninja | build, presets | yes | 4.4.3, 1.13.2 | none |
| C compiler | build | yes | Apple clang 21 (`gcc` and `clang` are both it here) | gcc 13 and clang 18 only through `linux-repro` |
| docker | strict gcc 13 build, container gate | yes (client and daemon answered) | 29.5.2 | record "unverified", never passed |
| awk | gate | BWK 20200816 only; gawk and mawk absent locally | - | container parts `gawk` and `mawk` |
| poppler `pdftoppm` | ER-1 image reads (04-01) | yes | present | none needed for the gate |
| the two manual PDFs | ER-1 reads | outside the repository (session scratch) | sizes and hashes as in `TOOLS.md` | re-download from the MiSTer docs mirror; never committed |
| Missing with no fallback | none | | | |

## Project Constraints (from .claude/CLAUDE.md and AGENTS.md)

- Language and build: decision record 0001 chose C11 with CMake and Ninja; the Rust text in the "Technology Stack" section of CLAUDE.md is an unadopted proposal and does not govern.
- Rights: no commercial ROM, firmware, system card, disc image, private save or credential committed or used in public CI; no manual page text or image in the repository (ER-9); fixtures are authored 0BSD with provenance.
- Core boundary: deterministic machine behaviour in the core (no file, clock, environment, allocator, float, mutable global); hosts own files, pacing and presentation; no public API or ABI before a consumer proof (the DSVR layout is a project definition, not an API).
- Dependencies: small and flat; none added; a copied helper needs recorded provenance (none is expected).
- Evidence: a support claim names its profile and tier; a matching hash proves repeatability only unless tied to an independent oracle; the word `pass` is never a support claim.
- CI: keep the fast path fast; review efficiency after every run and record it in `docs/ci-timings.md`; nothing is weakened to save seconds.
- Content as attack surface: sinks, snapshots and `.vexp` data are untrusted input and get validate-then-commit and fail-closed handling.
- Authority gates: licence, visibility, rights, hardware captures and publication stay with the owner; nothing in Phase 4 crosses one (no capture is needed).
- Repository rules: `sh scripts/check` is the single verification command; lines in TOML-subset files at most 400 characters; ASCII; SPDX headers (code MIT OR Apache-2.0, fixtures, ledger and docs 0BSD); repository-relative paths only; every commit ends with the standing Co-Authored-By trailer.
- GSD workflow: repository edits go through a GSD command; this file is the only file the phase researcher wrote.
- Runtime State Inventory: not applicable (new devices, no rename or migration). State of the Art: not applicable (no external framework).

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | About 10 master clocks per instruction at high speed, so a 2M-clock spin ROM executes about 200,000 instructions (F-19) | Findings | `opcode_exercise` could still lose records; raise the case capacity |
| A2 | Hashing cost 0.4 ms per 92 KB digest and `snapshot_hostile` growth of about 3.7 times | Pitfall 7 | conformance time slightly off; re-measure at 04-03 |
| A3 | CI cost multiplier of about 10 executions per case per job | Pitfall 7 | staging levers fire earlier or later |
| A4 | A new include `video.inc` keeps the 21 existing image hashes unchanged | Pitfall 9 | pins would need regeneration; verify with `sh scripts/check --stage fixtures` |
| A5 | Placing the new block after `tail_pad` keeps the `tail_pad[6]` anchor valid | F-10 | the selftest anchor must be updated in the same commit |
| A6 | The pair (stage, `vd_pending`) plus offsets fully encodes the next event | Integration map | a correction bump (allowed once) |
| A7 | An `EXPECT` second column is acceptable to the owner as a gate-machinery extension (reversible, routine) | F-4 | the two stays-green toys cannot be expressed as toys |

## Open Questions (RESOLVED)

1. **Does the executor accept the F-1 ruling (bound 512, no clamp)?** Known: VDW is 9 bits (H7-10) and D-139 cuts at the wrap only. Unclear: whether the owner wants a clamp. Recommendation: no clamp, bound 512; the context allows it without an owner question. RESOLVED: bound 512, no clamp (CONTEXT errata F-1; plan 04-03 Task 1 and Task 4).
2. **Where do the `INV_VIDEO_*` codes live (F-6)?** Recommendation: `scripts/video-check`, names verbatim. RESOLVED: the codes are printed by `scripts/video-check` (plan 04-07 Task 2); `inventory-check` is not edited.
3. **Is the first fixture plan allowed a 5-task cut of 04-03 (F-17) or a data-only `04-03a`?** Recommendation: five tasks first; `04-03a` only if the first task alone exceeds a plan. RESOLVED: neither; 04-03 is split three ways into 04-03 (data-only layout and bump), 04-04 (tracer) and 04-05 (hardening), renumbering later plans (04-PLAN set).
4. **Which palette bijection constants (F-25)?** Recommendation: the D-170 pair `$5B`, `$11` (the fixture text is the authority) and fix the D-169 sentence. RESOLVED: P(i) = (91*i + 17) & 511 (the D-170 pair $5B, $11) in every fixture; the D-169 sentence with 73 and 19 is superseded (plans 04-04 onward).

## Sources

### Primary (HIGH confidence)
- HuC6270 and HuC6260 manual page images H7-2 to H7-16, H7-18 to H7-25 and H6-1 to H6-5, read as images 2026-11-11 from the copies whose sizes and SHA-256 are in `04-CONTEXT.md` (D-179); the HuC6260 contents page.
- Repository files read this session: `AGENTS.md`, `.claude/CLAUDE.md`, `.planning/{REQUIREMENTS,ROADMAP,STATE}.md`, `04-CONTEXT.md`, `03-RESEARCH.md`, `src/core/{ds_core.h,ds_obs.h,ds_serial.h,ds_snapshot.h,ds_poweron.h,run.c,bus.c,irq.c,cpu.c (lines 496-700,
  1060-1096),obs.c,serial.c,snapshot.c,poweron.c,model.c,boot.c,constants.def,model_params.def,profile_fields.def,run_refusals.def,snapshot_refusals.def}`, `src/text/{text.c,ds_text.h}`, `tests/conformance/{runner.h,runner.c,guestkinds.c,obskinds.c,chunkkinds.c,snapkinds.c,digkinds.c,stepkinds.c,sweepkinds.c,REQUIRED,EXCLUDED_I
  _MODE,outcomes.def}`, `scripts/{check,conformance-run,control-mutate,fixture-build,fixture-lint,inventory-check,docs-check,licence-check,core-boundary-check,determinism-run,outcome-sync,record-lint,linux-repro}`, `scripts/lib/*.awk` (rule lines cited), `fixtures/lib/*`, `fixtures/src/{mmu_mpr,cyc_probe}.*` and the grep of all
  fixtures, `CMakeLists.txt`, `CMakePresets.json`, `docs/ci-timings.md`, `ledger/SCHEMA.md` and entries.
- Commands run: compiled layout probe (`sizeof`, `offsetof`), `ds_runner` timings per case and for the whole list, tool version probes, `shasum` of both PDFs, `pdftoppm` renders.

### Secondary (MEDIUM confidence)
- `docs/ci-timings.md` measurements of Phase 3 (loaded machine; CI row for run 43122980214).

### Tertiary (LOW confidence)
- None used. Emulator sources and pcetech were not re-read this session; their claims stay as the CONTEXT labels them ([COMM], ASSUMED).

## Metadata

**Confidence breakdown:**
- Manual facts: HIGH - every cited literal of the eight candidates was read on the page image.
- Code map: HIGH - files and line numbers read this session; the layout probe was compiled.
- Findings: HIGH for F-1, F-2, F-3, F-4, F-9, F-10 (traced in code); MEDIUM for F-8, F-17, F-19 (estimates).
- Costs: MEDIUM - measured on a loaded machine; the CI multiplier is an estimate.

**Research date:** 2026-11-11
**Valid until:** the first change to the files named in the integration map (the line numbers move), otherwise 30 days.
