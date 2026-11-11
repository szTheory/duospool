# Roadmap: DuoSpool

## Overview

Milestone v1 delivers the candidate first vertical slice: one named base PC Engine/TG-16 profile, plain HuCard content, original authored diagnostic fixtures, a headless runner, and an installed-consumer proof. Each support claim in the slice is backed by evidence that a fresh, deterministic, headless run can reproduce. The work starts with a gate phase in which the owner records decisions and the claim ledger is set up. Next comes a deterministic machine shell with a fail-closed gate and a HuCard content boundary that refuses what it does not support. Next is the HuC6280 CPU, bus, timer, interrupts, and 2-button pad, which produce guest-RAM verdicts before any video exists. Video (VCE/VDC) and audio (PSG) follow and can run in parallel. The slice closes with the runner, the installed-consumer proof, and staged verification.

Owner-gated decisions are not decided here. Phases that depend on one name it and gate on its record. Deferred scope stays out of v1 phases: SuperGrafx, CD-ROM²/Super CD-ROM², Arcade Card, TG-16 profile, HuC6280A profile, multitap/6-button/mouse, special mappers, persistent saves, replay, and debugger (see REQUIREMENTS.md v2). No GPL or unclear-licence fixture, and no commercial ROM, firmware, system card, disc image, or private save, is assumed in any phase.

**Granularity note:** Config granularity is `standard` (4-6 phases). The research proposed 8 phases. This roadmap folds the loader into the deterministic-shell phase and both VDC slices into a single video phase, which gives 6 phases.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Owner Decisions, Claim Ledger and Toolchain Spike** - Gate phase. Measured language spike, a first-profile record, and decision records for the owner to confirm. The claim ledger holds every open hardware fact (completed 2026-11-09)
- [x] **Phase 2: Deterministic Machine Shell, HuCard Boundary and Fail-Closed Gate** - Typed accept/refuse of HuCard bytes into fresh, isolated, seeded instances. One local command and CI fail closed (completed 2026-11-10)
- [ ] **Phase 3: HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad** - Authored fixtures report guest-RAM verdicts for the CPU, MMU, timer, IRQ, and SEL/CLR pad, with negative controls and resumable bounded runs
- [ ] **Phase 4: VCE/VDC Raw Video** - Per-scanline raw video and guest-recorded interrupt timing, checked against literal expectations for background, sprites, DMA, palette, and dot clock
- [ ] **Phase 5: PSG Raw Audio** - Timestamped PSG event log and PCM at a recorded format, checked by semantic properties. Can run in parallel with Phase 4
- [ ] **Phase 6: Headless Runner, Installed-Consumer Proof and Staged Verification** - Separate observation artifacts, an independent consumer against the installed artifact, staged fuzzing, a benchmark, and a LibFuturo fit evaluation

## Phase Details

### Phase 1: Owner Decisions, Claim Ledger and Toolchain Spike
**Goal**: The owner can confirm the decisions that gate the slice from recorded evidence. Before any code depends on a hardware fact the slice uses, that fact is in a claim ledger with its evidence tier and open/closed status.
**Mode:** mvp
**Depends on**: Nothing (first phase)
**Requirements**: TOOL-01, PROF-01, PROF-04, PROF-05
**Owner gates**: Language/build (decision 1), first-profile values (2), API/ABI posture (6), licence and repository visibility (8), fixture rights (9), and CI provider (14). This phase produces the records. It does not choose the values. Any decision still open at phase end is marked open, together with the later phase it blocks.
**Success Criteria** (what must be TRUE):
  1. A language/build decision record shows measured results from both spike skeletons (Rust/Cargo and C11/CMake), using the same fixture manifest for both: clean and incremental build time, test time, lines of glue, tools required, an installed-consumer build with source paths removed, a short loader fuzz run, and portability notes. Either the owner's choice is recorded with a date, or the record says the decision is open and Phase 2 is blocked.
  2. A versioned first-profile record names every field: console model/region, `$1000` bit-6 value, CPU/PSG revision, line count/frame timing, controller type, expansion-port presence, and content format. Each value is marked either owner-selected or open. Research defaults appear only as labelled proposals.
  3. The claim ledger exists with these fields: statement, profile, evidence tier (E0-E4), source citation, disagreement, status, and pinning test. A ledger check rejects a deliberately mislabelled sample entry that is marked "supported" on E0/E4 evidence only.
  4. Each conflict the slice depends on is an open ledger entry with its chosen modelling assumption, not stated as fact. The list includes at least: PSG clock, 262 vs 263 lines, VBlank/RCR IRQ position, dot-clock encoding, 7 MHz active width, `$1000` bit 6, HuCard byte order, copier-header rule, and PSG channel numbering.
  5. Decision records exist for licence/visibility, fixture rights, API/ABI posture, CI provider, and the copied-code provenance rule. Each one is either owner-recorded or explicitly open. The fixture-rights record states that no GPL or unclear-licence material is admitted until the owner decides.
**Plans:** 12/12 plans complete

Plans:
**Wave 1**
- [x] 01-01-PLAN.md — Walking-skeleton gate: scripts/check, the ledger and decision checkers, the E0/E4 negative control, record 0008, the full ledger negative corpus and --selftest

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 01-02-PLAN.md — Seed ledger (9 mandated conflicts + C14), first-profile record r1 with profile-check, record 0002 with the criterion-2 mapping

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 01-03-PLAN.md — Decision records 0003-0007, the README index with its drift check, PROJECT.md pointers, the decision negative corpus

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 01-04-PLAN.md — PROVENANCE.md with provenance-check, licence files with licence-check, README, .gitignore

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 01-05-PLAN.md — Pre-registered spike protocol for 0001 (rubric, arithmetic, D-36 toolchain rule), score.awk with D-07 controls, protocol-order-check

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 01-06-PLAN.md — Spike harness and the C11/CMake core path (written first): fixtures, manifest, consumer, core and runner, hard gates G1-G4

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 01-07-PLAN.md — C fuzz gate G5 with liveness, M08 sanitizer row, C bench hooks and LINES.map

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 01-08-PLAN.md — Rust/Cargo core path on stable 1.99.1: safe core, capi facade, safe runner, hard gates G1-G4

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 01-09-PLAN.md — Rust fuzz gate G5 with ASan canary, Miri on the facade (D-36 nightly), wasm row, Rust bench hooks and LINES.map

**Wave 10** *(blocked on Wave 9 completion)*
- [x] 01-10-PLAN.md — Pre-registered measurements for both options, run once, with check-results and its negative controls

**Wave 11** *(blocked on Wave 10 completion)*
- [x] 01-11-PLAN.md — Scores, record 0001 by the executable D-07 rule, evidence tag, spike removed from main

**Wave 12** *(blocked on Wave 11 completion)*
- [x] 01-12-PLAN.md — Full-history audit and gitleaks, the owner checkpoint, then the public repository with main and the evidence tag pushed
**Research**: Not needed beyond the spike, which has a fixed plan (SUMMARY: standard pattern). The real work is the owner decisions.

### Phase 2: Deterministic Machine Shell, HuCard Boundary and Fail-Closed Gate
**Goal**: A host passes DuoSpool content bytes and a named profile. It gets back either a typed refusal or a fresh, isolated, reproducibly initialised machine instance whose identity is recorded. One local command, mirrored in CI, fails closed when any required check is missing or skipped, has no evidence, or violates the rights boundary.
**Mode:** mvp
**Depends on**: Phase 1 (the language/build decision must be recorded; the first-profile record must exist, even with open values)
**Requirements**: PROF-02, PROF-03, CONT-01, CONT-02, CONT-03, CONT-04, CORE-01, CORE-02, CORE-03, RUN-02, FIX-02, FIX-04, FIX-05, TOOL-02, CI-01
**Owner gates**: CI provider (decision 14) for CI-01. HuCard byte-order and copier-header policy are declared inputs, never guessed. If a policy is not yet chosen, that case is refused.
**Success Criteria** (what must be TRUE):
  1. A refusal matrix in the fast gate covers these inputs: empty, truncated, header-prefixed, 513-byte, oversized, non-multiple-of-8 KiB, undeclared byte order or header, explicitly unsupported special mappers (e.g. SF2', Populous, Tennokoe), and mismatched profile. Each one returns its own typed reason. None panics or allocates past the size cap before validation. The state digest of an existing instance is the same before and after the refusal.
  2. Each supported HuCard size reads its expected bank signature through every documented mirror, via the mapper boundary. The core never opens a file: the host supplies the bytes, and the run record contains the content hash, size, declared format, byte order, and header handling.
  3. Every run record contains the full profile identity (every guest-visible setting) and the power-on policy and seed. Two instances run interleaved in one process produce the same digests as each instance run alone. The same inputs in a fresh process produce identical records. A build- or test-time check rejects core use of host clocks, filesystem, threads, environment, device, or UI APIs, and a deliberately violating negative control is shown to fail it.
  4. One documented local command runs the fast gate. Pass, guest failure, timeout, harness failure, skip, cancelled, and missing artifact are reported as distinct outcomes, and only pass is green. Each named negative control fails the gate: a removed required test, a missing manifest field, an unlisted fixture, a deliberately wrong literal, and an unlisted ROM/firmware-like binary added to the tree.
  5. CI on the owner-selected provider runs the same fast gate on every change. Each job is tied to a named failure class, and its critical-path time is recorded. If the CI provider is still undecided, this criterion is reported as blocked, not satisfied.
**Plans**: 22/22 plans complete
- [x] 02-01-PLAN.md
- [x] 02-02-PLAN.md
- [x] 02-03-PLAN.md
- [x] 02-04-PLAN.md
- [x] 02-05-PLAN.md
- [x] 02-06-PLAN.md
- [x] 02-07-PLAN.md
- [x] 02-08-PLAN.md
- [x] 02-09-PLAN.md
- [x] 02-10-PLAN.md
- [x] 02-11-PLAN.md
- [x] 02-12-PLAN.md
- [x] 02-13-PLAN.md
- [x] 02-14-PLAN.md
- [x] 02-15-PLAN.md
- [x] 02-16-PLAN.md
- [x] 02-17-PLAN.md
- [x] 02-18-PLAN.md
- [x] 02-19-PLAN.md
- [x] 02-20-PLAN.md
- [x] 02-21-PLAN.md
- [x] 02-22-PLAN.md
**Research**: Needed. HuCard byte order (C7), copier-header rule (C8), and mirroring for non-power-of-two sizes (e.g. 384 KiB) are unknown or conflicting. The chosen policy must be recorded in the ledger before the refusal matrix is final.

### Phase 3: HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad
**Goal**: Authored diagnostic fixtures run on the HuC6280 core and write their own verdicts into guest RAM. The verdicts cover instructions, MMU mapping, timer, interrupts, speed switching, and pad input. Every fixture has negative controls. A bounded run can stop at any allowed point and resume with identical results.
**Mode:** mvp
**Depends on**: Phase 2; Phase 1 licence and fixture-rights records (no fixture is admitted before the licence decision)
**Requirements**: CPU-01, CPU-02, CPU-03, CPU-04, CPU-05, CPU-06, INP-01, INP-02, INP-03, INP-04, CORE-04, CORE-05, CORE-06, FIX-01, FIX-03
**Owner gates**: Licence (decision 8) and fixture rights (9) before any fixture is admitted. The fixture assembler is admitted only after its licence check.
**Success Criteria** (what must be TRUE):
  1. Authored HuC6280 fixtures are built from original sources with project-owned startup code, using the recorded licence-checked assembler. A cross-assembler byte-identity check runs where practical. The fixtures write pass/fail verdicts into guest RAM for: the instruction set, including the HuC6280 additions; MPR mapping, including the defined unmapped-read value; the timer; interrupt priority, disable, and status; and high/low speed switching. Expected values cite a manual page or come from an in-guest computation, never from DuoSpool output. A deliberately wrong expected literal makes the fixture fail.
  2. A pad fixture observes the 2-button pad only through SEL/CLR reads on the I/O port. The input script is per port and stamped in machine time. The correct-input run passes, while the no-input and wrong-input runs produce different guest verdicts, and a guest pad-read log is recorded for all three.
  3. A fixture run in bounded chunks of varying size produces outputs identical to an uninterrupted run. This includes stops mid-frame and mid-block-transfer, including a length-0 (65,536-byte) transfer. A snapshot taken at any stop point and restored into a fresh instance continues identically.
  4. Turning on the CPU instruction trace and the device-register write log leaves verdicts, machine time, and state digests unchanged.
  5. Each cycle count and access penalty in use has a ledger entry that cites its source or labels it a modelling assumption. This covers speed switching and VDC/VCE access penalties. Unresolved edges are named profile or model parameters, not silent choices: interrupt latency around block transfers and after CLI/RTI, TAM/TMA #0, power-up MPR values, and ST0-ST2 MMU bypass.
**Plans**: 36/38 plans executed in 38 sequential waves, listed in execution order (context waves W1-W8 of 03-CONTEXT; W6-W7 are the Phase 3.1 cut line; ids 03-34..03-38 were added by the round-1 plan-check revision)

Plans:
- [x] 03-01-PLAN.md — W1 tracer: ca65 ds_proto through fixture-build, absolute-deadline ds_run in master clocks, minimal CPU and bus, DSV1 judged from guest RAM
- [x] 03-02-PLAN.md — W1: placeholder run API deleted; run_until_abs and run_chunk_sum; spin and interleave kinds on ds_run
- [x] 03-35-PLAN.md — W1: run refusals def with full coverage rows
- [x] 03-03-PLAN.md — W1: CPU0 region, PSGN/VDCP out, serial 2, goldens; runner headroom
- [x] 03-36-PLAN.md — W1: placeholder state removed; epoch 2, master-clocks record, goldens
- [x] 03-04-PLAN.md — W1: optional case keys in three places, guestkinds.c, authored fixture key
- [x] 03-05-PLAN.md — W1: fixture library (startup with RAM vectors, verdict, DCK1 literal table), DSV1 protocol controls, t=0 control
- [x] 03-37-PLAN.md — W1: cites.txt and fixture-lint with dead-stage control
- [x] 03-06-PLAN.md — W1: authored manifest entries, merged used list, assembler preflight, TOOLS.md, LED-TOOL-0001
- [x] 03-07-PLAN.md — W1: CI apt cc65 step with workflow-check and regenerated mini-roots
- [x] 03-38-PLAN.md — W1: README and docs-check, W1 ledger entries and timing
- [x] 03-08-PLAN.md — W2 tracer: manual opcode TSV and the ca65 encoding oracle (xasm), WLA optional
- [x] 03-09-PLAN.md — W2: 256-row cpu_ops.def, two-way opcode oracle, derived overshoot bound
- [x] 03-10-PLAN.md — W2: mutate_sweep, insn sink, opcode-exercise inventory
- [x] 03-11-PLAN.md — W2: load, store, transfer, stack semantics; cpu_ldst; default policies list
- [x] 03-12-PLAN.md — W2: ALU, decimal and T flag; cpu_alu; adc-carry toy
- [x] 03-13-PLAN.md — W2: control flow, BRK, RTI; cpu_flow
- [x] 03-14-PLAN.md — W2: HuC6280 additions except block transfers; cpu_huc; opcode coverage required
- [x] 03-15-PLAN.md — W2: opcode and cycle pin cases, cyc_probe sweep, W2 ledger entries
- [x] 03-16-PLAN.md — W3 tracer: full bus and MMU, windows, stubs, io_buffer, VDC/VCE penalty; mmu_mpr; unmapped toy
- [x] 03-17-PLAN.md — W3: model_params.def with tma_zero, TAM/TMA edges
- [x] 03-18-PLAN.md — W3: [model_*], [run] and [constants] record tables, LED-PROF-0004, LED-CPU-0015
- [x] 03-19-PLAN.md — W3: dev sink, observer matrix, trace-hook toy, --trace-out rights
- [x] 03-20-PLAN.md — W3: bank-class oracle, mmu_mpr size variants, st_bypass, W3 ledger entries
- [x] 03-21-PLAN.md — W4 tracer: closed-form timer, TIQ interrupt path, timer_pure_function, tim_timer
- [x] 03-22-PLAN.md — W4: interrupt controller, irq_stim, irq_i_sample, TIMR/IRQC/IRQS regions, two toys
- [x] 03-23-PLAN.md — W4: speed switching, spd_speed, [stimulus] table, W4 constants rows
- [x] 03-34-PLAN.md — W4: W4 ledger entries
- [x] 03-24-PLAN.md — W4: seeded-prng-v2, three-policy fixture runs, policy-diff control
- [x] 03-25-PLAN.md — W5 tracer: resumable block transfers, cpu_blk, length 0 and interrupt-pending variants, BLKT region, block-progress toy
- [x] 03-26-PLAN.md — W5: chunk_equiv (including mid-block with interrupts pending), 32-program random differential, W5 ledger entries
- [x] 03-27-PLAN.md — W6 tracer: $1000 pad protocol, input schedule, ds_input_push, pad_2btn correct
- [x] 03-28-PLAN.md — W6: input-check with one negative per code, [input] table
- [x] 03-29-PLAN.md — W6: pad negative controls, exact edges, pad logs, pad sweep and chunk case, shared-input toy, IOPT/SCHD, W6 ledger
- [x] 03-30-PLAN.md — W7 tracer: snapshot = digest stream, validate-then-commit restore, host hash check, sampled restores, tiling assert
- [x] 03-31-PLAN.md — W7: hostile snapshot corpus, restore at random stops, snapshot toys
- [ ] 03-32-PLAN.md — W8: page re-reads, ledger finalisation under ER-2, evidence split by class, constants ledger rule, no PENDING pins
- [ ] 03-33-PLAN.md — W8: README wording with docs-check, timing re-baseline, optional WLA run, CI unverified record
**Research**: Needed for targeted timing edges (C9, C10): block-transfer and CLI/RTI interrupt latency, speed-switch cycle count, TAM/TMA #0, power-up MPRs beyond MPR7, and ST0-ST2 (manual p. H8-3). Opcode tables come from the manual and follow a standard pattern.

### Phase 4: VCE/VDC Raw Video
**Goal**: Authored display fixtures produce per-scanline raw video and record interrupt timing in guest RAM. Background, sprite, DMA, palette, and dot-clock behaviour are checked against literal expectations, never against a frame hash alone.
**Mode:** mvp
**Depends on**: Phase 3 (interrupt controller, timestamped CPU device access)
**Requirements**: VID-01, VID-02, VID-03, VID-04, VID-05, VID-06
**Owner gates**: Lines-per-frame value from the first-profile record (decision 2). Any hardware capture that would settle IRQ position or geometry needs the owner's capture decision (10). None is assumed.
**Success Criteria** (what must be TRUE):
  1. Background fixtures draw known tiles through the BAT with scroll writes. The per-scanline raw record contains VCE colour values, dot-clock divisor, active window, master-clock origin, and frame line count, and it matches literals derived from the fixture's own data. A scroll write takes effect at the documented latch point (BXR/BYR on the next line).
  2. Sprite fixtures write the per-line limit and the overflow and collision status flags into guest RAM. Fixtures for VRAM register read/write, VRAM-to-SATB DMA, and VRAM-to-VRAM DMA verify the data moved. Each claim states its VDC simplification level, and any DMA or access-slot timing without a source is labelled in the ledger.
  3. A fixture writes the scanline and phase of raster-compare and VBlank interrupts into guest RAM. The recorded values agree with the modelled position for the profile's line count. They are labelled a modelling assumption, not a hardware claim, until a primary manual page or an owner-approved capture settles C3.
  4. All 512 9-bit palette entries round-trip through write and readback in guest RAM. A dot-clock change, including one mid-frame, shows up as changed per-line geometry in the raw record and is never normalised to a fixed width.
  5. A raw-video hash counts as a verdict only when paired with a semantic literal assertion, and that assertion is shown to fail when the renderer is deliberately broken.
**Plans**: TBD
**Research**: Needed (heavy). VBlank and RCR IRQ position (C3), dot-clock register encoding (C4), 7 MHz active width (C5), and mid-frame width change (C11). For the VDC: VRAM slot model per dot mode, DMA timing, burst mode, and sprite overflow/collision semantics. Re-read manual pp. H7-6 to H7-11 and the cited HuC6260 pages before planning. If the phase grows too large, planning may split background/IRQ from sprites/DMA into separate plans or an inserted decimal phase.

### Phase 5: PSG Raw Audio
**Goal**: Authored audio fixtures produce a timestamped PSG event log and PCM at a recorded format. Semantic properties are checked, and no pitch or timing claim goes beyond the available evidence.
**Mode:** mvp
**Depends on**: Phase 3 (timer, timestamped CPU writes). Can run in parallel with Phase 4.
**Requirements**: AUD-01, AUD-02, AUD-03
**Owner gates**: PSG revision comes from the first-profile record (decision 2). Settling the PSG clock conflict (C1) needs a primary source or an owner-approved capture (decision 10). None is assumed.
**Success Criteria** (what must be TRUE):
  1. Fixtures program all 6 channels: waveform RAM, frequency, volume/balance, noise on the two noise-capable channels, DDA, and LFO. The resulting entries in the timestamped register-event log match the fixture's literal expected writes and the documented semantics (E1). Channel numbering follows the ledger's normalised scheme.
  2. PCM is emitted at a defined, recorded sample rate and format before any host resampling. The per-frame sample count is reported, so a non-integer cadence is visible.
  3. Audio checks assert semantic properties: a channel period that matches the programmed frequency under the current clock assumption, the expected waveform content, and silence where a channel is off. A deliberately mis-programmed fixture (wrong frequency or a silent channel) fails them. "Buffer is nonzero" is never accepted.
  4. Noise output is identical across repeated fresh runs under the same power-on policy. The PSG clock and revision are profile parameters whose ledger status stays open, and no PSG pitch or timing correctness claim is made until C1 is settled.
**Plans**: TBD
**Research**: Needed. Read the PSG manual (S4, downloaded but unread) before planning. Also: clock conflict C1; LFO registers; the noise generator and its seed; wave-pointer behaviour when playback is off.

### Phase 6: Headless Runner, Installed-Consumer Proof and Staged Verification
**Goal**: A user outside the source tree installs the core, runs a fixture through the headless runner or an independent consumer, and gets a reproducible verdict, with every observation in its own artifact. Staged fuzzing, a benchmark, and the LibFuturo fit evaluation run without slowing the fast gate.
**Mode:** mvp
**Depends on**: Phase 4 and Phase 5 (raw video and audio observations)
**Requirements**: RUN-01, PKG-01, CI-02, PERF-01, INTG-01
**Owner gates**: Host/platform targets and package format (decision 12). API/ABI posture (6): nothing is frozen. LibFuturo (7): the owner must supply or authorize access to the current contract, and adoption is not assumed. Release policy (18) and publication authority are outside this phase.
**Success Criteria** (what must be TRUE):
  1. The headless runner takes content, profile, input script, and a machine-time bound. It writes these as separate artifacts: guest-progress markers, machine time with stop reason and overshoot, raw video records, PSG event log, PCM, and trace. When an expected artifact is absent, the outcome is "missing artifact", not pass.
  2. An independent consumer is built in a separate directory and process against the installed artifact, with the source and build trees unavailable. It reproduces a fixture verdict through the public boundary. The package follows the owner's recorded host/package decision, and no API/ABI is declared frozen.
  3. Fuzzing of the content loader and the input-script parser runs as a staged job outside the fast PR path. An input saved from a crash replays in the fast gate as a regression case.
  4. A named-workload benchmark records workload identity, build, input, warm-up, measurements, and uncertainty. The same run asserts output correctness, and the benchmark fails when that assertion fails.
  5. A LibFuturo fit evaluation exists as one of two forms: a prototype adapter over the real core, or reproducible cases for each concrete mismatch (input mask width, sample point, palette/geometry, audio cadence, content spans). No LibFuturo dependency is added. If the owner has not authorized access to the contract, this criterion is reported as blocked.
**Plans**: TBD
**Research**: Not needed for runner, outcome, and packaging mechanics (standard patterns). Host, package format, and LibFuturo access are owner decisions.

## Owner-Gated Decisions Referenced by Phases

These decisions are open unless a record says otherwise. Research defaults are proposals, not decisions (see `.planning/research/SUMMARY.md`, "Owner-Gated Decisions").

| # | Decision | Gates |
|---|----------|-------|
| 1 | Language and build system | Phase 2 onward (hard gate) |
| 2 | First-profile values (region, model, CPU/PSG revision, line count, controller) | Phases 2-5 (open values stay explicit parameters) |
| 6 | Core API/ABI and evolution policy | Phase 6 (no freeze) |
| 7 | LibFuturo adoption / contract access | Phase 6 (INTG-01) |
| 8 | Licence and repository visibility | Phase 3 (fixture admission) |
| 9 | Fixture rights; GPL/unclear-licence material | Phase 3 onward |
| 10 | Physical-hardware captures (E3) | Phases 4-5 (only to settle open timing facts) |
| 12 | Host/platform targets, package format | Phase 6 |
| 14 | CI provider | Phase 2 (CI-01) |

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6. Phases 4 and 5 depend only on Phase 3, so they may run in parallel.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Owner Decisions, Claim Ledger and Toolchain Spike | 12/12 | Complete    | 2026-11-09 |
| 2. Deterministic Machine Shell, HuCard Boundary and Fail-Closed Gate | 22/22 | Complete    | 2026-11-10 |
| 3. HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad | 36/38 | In Progress | - |
| 4. VCE/VDC Raw Video | 0/TBD | Not started | - |
| 5. PSG Raw Audio | 0/TBD | Not started | - |
| 6. Headless Runner, Installed-Consumer Proof and Staged Verification | 0/TBD | Not started | - |
