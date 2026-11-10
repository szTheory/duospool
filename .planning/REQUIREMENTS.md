# Requirements: DuoSpool

**Defined:** 2026-11-09
**Core Value:** For every hardware/content profile DuoSpool claims to support, a fresh, deterministic, headless run can reproduce the machine's behavior and show the evidence — guest progress, machine time, raw video, raw audio — that backs the claim.

Requirements were derived in GSD auto mode from `.planning/bootstrap/PROJECT-SEED.md`
and `.planning/research/` (FEATURES, ARCHITECTURE, PITFALLS, STACK, SUMMARY).
v1 covers the seed's candidate first vertical slice: one named base PC Engine/TG-16
profile, plain HuCard content, original diagnostic fixtures, a headless runner, and
an installed-consumer proof. Where a requirement depends on an open owner decision,
it requires the decision to be made and recorded. It never presumes the answer.

**Evidence tiers** (proposed in research; used in acceptance wording below):
E0 = repeatability only; E1 = documented vector/register behavior from a cited source
asserted literally; E2 = authored diagnostic ROM with an independent verdict;
E3 = hardware measurement with recorded setup/uncertainty (owner gate);
E4 = differential vs another emulator (finds disagreements, decides nothing).

## v1 Requirements

### Profile, Scope & Evidence

- [x] **PROF-01**: The owner-selected first machine profile is recorded as a named, versioned profile record (console model/region, `$1000` bit-6 value, CPU/PSG revision, line count/frame timing, controller type, expansion-port presence, content format). Research's proposed default is Japanese PC Engine / HuC6280 / 2-button pad. It is a proposal until the owner selects it.
- [x] **PROF-02**: Loading content or running with an unsupported or mismatched profile is refused with a typed reason. It never falls back silently.
- [x] **PROF-03**: Any setting that changes guest-visible behavior is part of profile identity and appears in every run's recorded configuration.
- [x] **PROF-04**: A claim ledger lists each supported behavior with its profile, evidence tier (E0–E4), source citation, and fixture/test link. Behaviors with only E0/E4 evidence are not labeled "supported".
- [x] **PROF-05**: Conflicting or unresolved hardware facts the slice depends on (e.g. PSG clock, 262 vs 263 lines, VBlank IRQ position, VCE dot-clock encoding) are recorded as open in the ledger with the chosen modelling assumption, not asserted as fact.

### CPU & Bus (HuC6280)

- [ ] **CPU-01**: The core executes the HuC6280 instruction set (65C02 base plus HuC6280 additions: block transfers, ST0/ST1/ST2, TAM/TMA, SXY/SAX/SAY, CSH/CSL, SET/T-flag, TST, BSR, bit ops) with flag/register effects verified by authored fixtures against the HuC6280 manual (E1/E2).
- [ ] **CPU-02**: The MMU maps logical addresses through MPR0–7 to the 21-bit physical space (256 × 8 KiB banks), including RAM, HuCard banks, I/O page, and a defined unmapped-read value, verified by fixtures.
- [ ] **CPU-03**: The timer (reload, decrement rate tied to the master clock, IRQ) and interrupt controller (disable/status registers, documented priority, sampling point) behave as documented, verified by fixtures that report results in guest RAM.
- [ ] **CPU-04**: High/low CPU speed switching changes instruction timing on the master-clock timeline, and VDC/VCE access penalties are applied as documented.
- [ ] **CPU-05**: Block-transfer instructions are resumable at byte granularity. A bounded run can stop mid-transfer and continue with identical results to an uninterrupted run, and interrupt handling around block transfers follows the recorded model.
- [ ] **CPU-06**: Instruction and cycle claims cite their source. Any cycle count without a primary source or E2/E3 evidence is labeled as a modelling assumption in the ledger.

### Video (HuC6270 VDC / HuC6260 VCE)

- [ ] **VID-01**: The VDC renders background layers from the BAT/VRAM with documented scroll-register latch timing (e.g. BXR/BYR rules), verified by authored display fixtures with literal expected values.
- [ ] **VID-02**: The VDC renders sprites from the SAT with the documented per-line limit and overflow/collision status, verified by fixtures.
- [ ] **VID-03**: The VDC implements VRAM read/write via the register interface, VRAM→SATB DMA, and VRAM-to-VRAM DMA to the documented level, with unverified timing labeled in the ledger.
- [ ] **VID-04**: VDC interrupts (raster-compare and VBlank, status read/acknowledge) fire at the modelled position on the master-clock timeline, verified by a fixture that records interrupt scanlines in guest RAM.
- [ ] **VID-05**: The VCE implements the 512-entry, 9-bit palette (write and readback) and the dot-clock modes, with per-line horizontal geometry derived from the selected dot clock.
- [ ] **VID-06**: Raw video is emitted as per-scanline records (VCE colour indices/9-bit values plus dot-clock/width metadata and frame line count) before any host colour conversion or scaling. Mid-frame dot-clock or geometry changes are represented, not hidden.

### Audio (PSG)

- [ ] **AUD-01**: The PSG implements 6 channels with waveform RAM, frequency, volume/balance, noise (on the two noise-capable channels; channel numbering normalized in the claim ledger), DDA mode, and LFO to the documented level, verified by fixtures that assert register-to-event behavior (E1).
- [ ] **AUD-02**: Raw audio is emitted as a timestamped PSG register-event log and as PCM at a defined, recorded sample rate and format, before any host resampling or device playback.
- [ ] **AUD-03**: Audio checks assert semantic properties (e.g. expected channel frequency/period, waveform content, silence where expected) rather than "buffer is nonzero". Noise generation is deterministic.

### Input

- [ ] **INP-01**: A 2-button pad is emulated through the real SEL/CLR guest scan protocol on the I/O port, so the guest observes input only through its own reads.
- [ ] **INP-02**: An input script format specifies logical button states per port with explicit machine-time stamps, and the runner applies them deterministically.
- [ ] **INP-03**: Each input fixture has negative controls: a no-input run and a wrong-input run must produce guest results different from the correct-input run, and a log of guest pad reads is recorded.
- [ ] **INP-04**: Input state is modelled per port (not a single global mask), so later multitap/6-button support does not require reshaping the core boundary.

### Content Loading & Security

- [x] **CONT-01**: The HuCard loader accepts plain HuCard images of explicitly supported sizes with documented mirroring rules, and refuses all other sizes, special mappers, and unknown layouts with a typed reason.
- [x] **CONT-02**: Copier-header handling and data byte order (PC Engine vs TG-16 order) are explicit, declared inputs or refusals. The loader never guesses by heuristic, CRC, or title string.
- [x] **CONT-03**: Malformed, truncated, zero-length, and oversized content is refused without panicking, without unbounded allocation, and without mutating an existing machine instance.
- [x] **CONT-04**: The core never reads files itself. The host supplies content bytes, and the content's identity (hash, size, declared format) is recorded with each run.

### Determinism, Core Boundary & Runner

- [x] **CORE-01**: Machine state is explicit and per instance. Two instances in one process do not share mutable state, verified by a multi-instance test.
- [x] **CORE-02**: The core has no dependency on host clocks, filesystems, device discovery, threads, UI, or audio/video devices, enforced by a build- or test-time check.
- [x] **CORE-03**: Power-on state is defined by a documented, seeded policy (RAM, MPRs, PSG noise, VDC phase), so a fresh run is reproducible and the policy is recorded with each run.
- [ ] **CORE-04**: Execution is bounded in master-clock time with an explicit stop reason and overshoot. A bounded run may stop mid-frame or mid-instruction-sequence and resume with results identical to an uninterrupted run.
- [ ] **CORE-05**: A snapshot taken at any stop point, restored into a fresh instance and continued, produces outputs identical to continuing the original (state-completeness check, E0).
- [ ] **CORE-06**: A CPU instruction trace and a device-register write log can be enabled without changing machine behavior.
- [ ] **RUN-01**: A headless runner loads content plus a profile and an input script, runs for a bounded machine time, and emits guest-progress markers, machine time, raw video, raw audio, and trace as separate artifacts.
- [x] **RUN-02**: The runner and harness report distinct outcomes: pass, guest failure, timeout, harness failure, skip, cancelled, and missing artifact. No outcome other than pass counts as passing.

### Fixtures & Verification

- [ ] **FIX-01**: Diagnostic fixtures are original, authored HuC6280 assembly sources with project-owned startup code. Their recorded build recipe uses an external, license-checked assembler, with a cross-assembler byte-identity check where practical.
- [x] **FIX-02**: Every fixture has a manifest entry (source, build recipe, toolchain version, byte hash, license, notices, redistribution terms, test meaning, expected verdict source). A manifest check fails closed on any missing field or unlisted fixture.
- [ ] **FIX-03**: Fixture verdicts come from documented behavior or an independent verdict computed in the guest, never from DuoSpool's own recorded output.
- [x] **FIX-04**: The required-test inventory fails closed: a missing, skipped, or evidence-free required check fails the gate, calibrated by a named negative control that is shown to fail.
- [x] **FIX-05**: No commercial ROMs, firmware/system cards, disc images, or private saves are committed, used in CI, or written to CI logs/artifacts, enforced by a repository check.

### Tooling, Packaging & CI

- [x] **TOOL-01**: A language/build-system decision record compares the research-proposed options (Rust/Cargo vs C11/CMake) using a measured spike (build time, installed-consumer proof, fuzz harness, portability target), and the owner-confirmed choice is recorded.
- [ ] **TOOL-02**: One documented local command runs the same checks as the fast PR gate.
- [ ] **PKG-01**: An independent consumer builds against the installed core artifact, with source and build trees unavailable, and reproduces a fixture verdict through the public boundary.
- [x] **CI-01**: CI runs the fast gate on each change, with each job tied to a distinct recurring failure class and its critical-path time recorded.
- [ ] **CI-02**: Fuzzing of the content loader and input-script parser runs as a staged job outside the fast PR path, with crashing inputs replayable in the fast gate as regression cases.
- [ ] **PERF-01**: At least one named workload has a benchmark recording workload identity, build, input, warm-up, measurements, and uncertainty, with output correctness asserted in the same run.

### Integration

- [ ] **INTG-01**: LibFuturo fit is evaluated with a prototype adapter over the real core, or a reproducible case documents each concrete mismatch (e.g. input mask width, sample point, palette/geometry, audio cadence, content spans), without adopting LibFuturo as a dependency.

## v2 Requirements

Deferred. Each needs a named user outcome and enough evidence before it enters a roadmap.

### Profiles & Revisions

- **PROF2-01**: TurboGrafx-16 profile with its own evidence (region bit, default turbo pad, byte-order handling)
- **PROF2-02**: HuC6280A PSG-revision profile (E3-gated: analog capture)
- **PROF2-03**: Per-model profiles (CoreGrafx, Shuttle, GT/LT, Duo variants) where they change behavior

### Input & Peripherals

- **INP2-01**: 6-button pad with scan-counter detection
- **INP2-02**: Multitap (up to 5 ports) and machine-time-driven turbo/autofire
- **INP2-03**: Mouse, XE-1AP, Pachinko and other machine-time-sensitive devices
- **INP2-04**: Memory Base 128

### Content & Persistence

- **CONT2-01**: Street Fighter II' mapper and Populous RAM via explicit mapper declaration and authored mapper fixtures
- **CONT2-02**: Typed, host-owned persistent-store model (internal BRAM, Tennokoe Bank once its unlock behavior is sourced)

### Tooling & UX

- **TOOL2-01**: Profile-bound replay format (input script + start state + profile + build)
- **TOOL2-02**: Per-run evidence report with tiered claims
- **TOOL2-03**: Interactive debugger (breakpoints, VRAM/PSG inspectors) built on trace hooks
- **TOOL2-04**: User-facing, versioned save-state files and rewind
- **TOOL2-05**: Reference/fast dual VDC implementation as an internal differential check
- **INTG2-01**: Frontend adapters (libretro-style or LibFuturo) beyond the prototype, with a public API/ABI evolution policy
- **INTG2-02**: A host frontend with presentation (scaling, colour conversion, audio device output)

### Extended Hardware (rights/oracle gated)

- **HW2-01**: SuperGrafx (second VDC, HuC6202 VPC, 32 KiB RAM)
- **HW2-02**: CD-ROM² / Super CD-ROM² (disc abstraction, SCSI model, CD-DA, ADPCM, user-supplied system card). Needs a rights decision and a legal test boot path first.
- **HW2-03**: Arcade Card (after CD-ROM²)

## Out of Scope

| Feature | Reason |
|---------|--------|
| Committing or CI-using commercial ROMs, BIOS/system cards, disc images, private saves | Rights and security boundary; a hash grants no redistribution right |
| CRC/title-string keyed behavior or auto-detection in the core | Hidden, non-reproducible behavior; replaced by explicit profile/mapper declaration |
| Heuristic auto-detection of HuCard byte order or region | Fail-open; a wrong guess means the wrong machine |
| Behavior-changing "enhancements" in the core (e.g. no sprite limit) | Breaks profile identity and determinism; a distinct profile is required if ever offered |
| Shaders, CRT filters, scaling, composite simulation inside the core | Host presentation, not machine behavior |
| Netplay, run-ahead | Requires mature state/replay determinism first; high maintenance |
| PAL/50 Hz variants, CD+G, LaserActive, HES player | No named user outcome or source yet |
| "Accurate"/"compatible" claims from title screens, game lists, or emulator agreement | Violates Core Value; E0/E4 evidence only |
| Freezing a public API/ABI in v1 | No consumer has proven its shape yet |
| Choosing the project license, repository visibility, or release policy within GSD automation | Owner authority |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| PROF-01 | Phase 1 | Complete |
| PROF-02 | Phase 2 | Complete |
| PROF-03 | Phase 2 | Complete |
| PROF-04 | Phase 1 | Complete |
| PROF-05 | Phase 1 | Complete |
| CPU-01 | Phase 3 | Pending |
| CPU-02 | Phase 3 | Pending |
| CPU-03 | Phase 3 | Pending |
| CPU-04 | Phase 3 | Pending |
| CPU-05 | Phase 3 | Pending |
| CPU-06 | Phase 3 | Pending |
| VID-01 | Phase 4 | Pending |
| VID-02 | Phase 4 | Pending |
| VID-03 | Phase 4 | Pending |
| VID-04 | Phase 4 | Pending |
| VID-05 | Phase 4 | Pending |
| VID-06 | Phase 4 | Pending |
| AUD-01 | Phase 5 | Pending |
| AUD-02 | Phase 5 | Pending |
| AUD-03 | Phase 5 | Pending |
| INP-01 | Phase 3 | Pending |
| INP-02 | Phase 3 | Pending |
| INP-03 | Phase 3 | Pending |
| INP-04 | Phase 3 | Pending |
| CONT-01 | Phase 2 | Complete |
| CONT-02 | Phase 2 | Complete |
| CONT-03 | Phase 2 | Complete |
| CONT-04 | Phase 2 | Complete |
| CORE-01 | Phase 2 | Complete |
| CORE-02 | Phase 2 | Complete |
| CORE-03 | Phase 2 | Complete |
| CORE-04 | Phase 3 | Pending |
| CORE-05 | Phase 3 | Pending |
| CORE-06 | Phase 3 | Pending |
| RUN-01 | Phase 6 | Pending |
| RUN-02 | Phase 2 | Complete |
| FIX-01 | Phase 3 | Pending |
| FIX-02 | Phase 2 | Complete |
| FIX-03 | Phase 3 | Pending |
| FIX-04 | Phase 2 | Complete |
| FIX-05 | Phase 2 | Complete |
| TOOL-01 | Phase 1 | Complete |
| TOOL-02 | Phase 2 | Pending |
| PKG-01 | Phase 6 | Pending |
| CI-01 | Phase 2 | Complete |
| CI-02 | Phase 6 | Pending |
| PERF-01 | Phase 6 | Pending |
| INTG-01 | Phase 6 | Pending |

**Coverage:**
- v1 requirements: 48 total
- Mapped to phases: 48
- Unmapped: 0 ✓

---
*Requirements defined: 2026-11-09*
*Last updated: 2026-11-09 after roadmap creation (traceability filled)*
