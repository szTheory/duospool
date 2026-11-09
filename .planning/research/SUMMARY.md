# Project Research Summary

**Project:** DuoSpool (owner-selected working name; not trademark-cleared)
**Domain:** PC Engine / TurboGrafx-16 emulator core: deterministic, headless-first, evidence-first, one named profile at a time
**Researched:** 2026-11-09
**Confidence:** MEDIUM overall

Inputs: `.planning/research/STACK.md`, `FEATURES.md`, `ARCHITECTURE.md`, `PITFALLS.md`, and `.planning/PROJECT.md` (context and open decisions). The bootstrap pack was not re-read for this synthesis. LibFuturo is described only as PROJECT.md describes it.

---

## How to Read This Summary

**Claim labels.** The four researchers used different tag sets. This summary normalizes them to six categories and keeps each claim's meaning. Use the mapping to trace any claim back to its file.

| Label here | Meaning | STACK | FEATURES | ARCHITECTURE | PITFALLS |
|---|---|---|---|---|---|
| [FACT] | Source-backed fact, graded primary or secondary by its researcher | [SF] | [FACT] | [SB] | [SRC] |
| [CODE] | Code observation: what one implementation does. Not a hardware fact | [CO] | [CODE] | [CO] | [CODE] |
| [REPORT] | Historical or community report, not independently verified | [HR] | [REPORT] | [HR] | [HIST] |
| [PROPOSAL] | Recommendation or inference. Not a decision | [P] | [PROPOSAL] | [P] | [PROP] |
| [UNKNOWN] | No adequate source, or sources conflict | [U] | [UNKNOWN] | [U] | [UNK] |
| [OWNER] | Owner decision, recorded or pending | n/a | [OWNER] | n/a | n/a |

[FACT, derived] marks arithmetic on cited constants (ARCHITECTURE's [P-derived]). It is not a measurement.

**Proposed default** means a research recommendation the owner may overrule. It is not a decision. Every item in the owner-gated section stays open even where a default is proposed.

**Evidence tiers.** FEATURES (E0-E4) and PITFALLS (named tiers) describe the same ladder. This summary uses the FEATURES numbering.

| Tier | Meaning | Can back a correctness claim? |
|---|---|---|
| E0 | Repeatability only: same build, same input, same hash | No |
| E1 | Documented vector or register behavior, asserted literally | Yes, for that cited behavior |
| E2 | Authored diagnostic ROM with its own verdict, computed independently of emulator output | Yes, for the behavior it tests |
| E3 | Hardware-measured, with setup and uncertainty recorded (owner gate) | Yes, within the measured setup |
| E4 | Differential against another emulator | No. Triage only (conflict C12) |

**Source weight.** pcetech.txt, one author's preliminary notes, is labelled three different ways: [FACT, secondary] by FEATURES, [SRC] by PITFALLS, [REPORT] by ARCHITECTURE. This summary treats its content as a community report unless a primary manual page or a measurement confirms it. The same applies to the two Nicole Express articles (one author, teardown-based) and to the MiSTer sgxtech notes (author-caveated).

---

## Executive Summary

DuoSpool is a machine core, not a frontend. The target is the PC Engine family as a set of named profiles: a HuC6280 CPU (a 65C02 derivative with an MMU, timer, I/O port and integrated PSG), a HuC6270 video display controller (VDC), a HuC6260 video colour encoder (VCE), and HuCard, CD-ROM² and expansion content. The surveyed emulators (Mednafen/Beetle PCE, MAME, Geargrafx, Ares, the MiSTer FPGA core) converge on a few structures: one master-clock counter, devices caught up at the moment the CPU touches them, a 256-entry bank table for memory, per-line video, and a register-level PSG. The research files agree on the architecture that follows. It is one u64 master-clock timeline, an instruction-granular CPU with timestamped hardware-page accesses, per-line raw video in 9-bit VCE words, PSG as a timestamped event log plus PCM at an exact integer ratio, per-port input protocol machines fed by timestamped events, typed content slots, and per-instance state with no host clock, filesystem or RNG in the core. ARCHITECTURE, FEATURES and PITFALLS each reject frame-stepped execution as the contract, and STACK's determinism rules are consistent with that.

The recommended approach is a first vertical slice of one named Japanese PC Engine profile (proposed: HuC6280, region field set to PC Engine, 2-button pad, HuCard only). The slice includes a HuCard loader that refuses everything it does not explicitly support; the HuC6280 core with timer, interrupts and the SEL/CLR pad protocol; VCE, VDC and PSG brought to the level needed for authored fixtures; a deterministic scheduler and headless runner; authored diagnostic fixtures with manifests; and an installed-consumer proof. The proposed language is Rust, with a zero-dependency core and a thin C-ABI facade. A one-day Rust-versus-C spike should confirm it (MEDIUM; the ranking is single-source). Fixtures are written in ca65/ld65 (Zlib) from original startup code. SuperGrafx, CD-ROM² and Super CD-ROM², Arcade Card, multitap, 6-button and mouse, a TG-16 profile, persisted saves, replay and a debugger stay out of the first slice. Their seams (an instantiable VDC, typed content slots, mapper hooks, per-port input) are built in now. Nothing is frozen as a public API before an independent consumer uses it.

The main risk is that the hardware-timing facts a first profile depends on are thin and partly contradictory. The sources disagree on the PSG clock (3.58 vs 7.16 MHz), the line count (262 vs 263), the VBlank and raster IRQ positions, the dot-clock register encoding, the 7 MHz active width (320 vs 341) and the HuCard data-line order. No hardware-verified, licence-clear CPU or VDC test suite is known. The oracle therefore has to be authored fixtures with literal expectations, plus owner-gated hardware captures for anything the manuals do not settle. The second risk is rights. The HuC/PCEAS toolchain licence is unclear, the 240p Test Suite is GPL-2.0-or-later, and System Card firmware cannot be committed or run in CI. The mitigations the research proposes are a claim ledger before the first golden hash, an evidence tier on every claim, negative controls (no input, wrong input, deliberately wrong literal) on every test, a fail-closed inventory, a manifest gate that rejects unknown licences, and explicit profile parameters for every unresolved value, so an unknown stays visible instead of being guessed.

---

## Owner-Gated Decisions

Research does not decide these. Routine, reversible work (manifests, harness, the spike, documentation) can proceed. These items should be surfaced to the owner rather than assumed. Several of them gate a phase boundary.

| # | Decision | Status | Proposed default ([PROPOSAL], not a decision) | What is needed to close it |
|---|---|---|---|---|
| 1 | Language and build system | Open [OWNER] | Rust stable, pinned 1.99.1, edition 2024, Cargo; zero-dependency core; thin C-ABI facade. C11 + CMake + Ninja is the close second | One-day skeleton spike of both (STACK's list), then owner weighting |
| 2 | First profile: region field, console model, CPU revision, line count, video timing, controller | Open [OWNER] | Japanese PC Engine; HuC6280 only (no 6280A in slice 1); region field set to PC Engine; 2-button pad; HuCard only. Line count and PSG tick stay explicit parameters marked [UNKNOWN] | Owner choice. Line count and PSG rate need measurement (C1, C2) |
| 3 | SuperGrafx scope | Open [OWNER] | Out of v1. Keep the VDC instantiable and the VDC-to-VCE pixel stream as a seam | A named user outcome and a stronger oracle than one RE note |
| 4 | CD-ROM² / Super CD-ROM² / Arcade Card scope | Open [OWNER] | Out of v1. Typed content slots and mapper hooks only | System Card rights path (no redistributable, System-Card-free boot path is known [UNKNOWN]); disc abstraction design |
| 5 | Multitap, 6-button, mouse, other peripherals | Open [OWNER] | After timestamped input exists, i.e. at the first fixture that needs more than one pad or mid-frame input | A named outcome; read PCE_Controller_Info directly |
| 6 | Core API/ABI and evolution policy | Open [OWNER] | No frozen ABI. Internal advance API first; hand-written header only after a consumer proves the shape | Owner agreement |
| 7 | LibFuturo adoption | Open [OWNER] | Candidate only. Build a frame-level adapter experiment and record each named mismatch. No dependency | Owner to supply or authorize access to the current LibFuturo contract. No researcher read its source |
| 8 | Licence and repository visibility | Open [OWNER] (owner authority) | No recommendation from research. This decision gates fixture admission (see rights gates below) | Owner licence decision |
| 9 | First homebrew or game; fixture rights | Open [OWNER] | Authored fixtures only in the fast suite | Owner decision on any GPL or unclear-licence material |
| 10 | Physical-hardware captures (E3) | Open owner gate | None assumed. Captures are the only route to settle the PSG rate, the VBlank and raster IRQ positions, and PSG revision | Owner decides whether to fund a recorded capture setup |
| 11 | Timing fidelity target and performance budget | Open [OWNER] | Instruction-granular CPU with access-time catch-up. Per-dot rendering and palette-write streaking not required in the first slice | Owner target. Performance measured only after correctness |
| 12 | Host/platform targets and package format | Open [OWNER] | Static library, hand-written header and pkg-config file (STACK) | Decide when the installed-consumer proof is planned |
| 13 | Whether a wasm target is promised | Open [OWNER] | Not promised; staged later | Owner decision (it decides when wasm CI is worth its cost) |
| 14 | CI provider | Not selected | Not chosen by any researcher | Owner |
| 15 | Save-state, replay and debugger scope | Open [OWNER] | In-memory snapshot for tests only. No persisted format in slice 1 (conflict C13) | Owner |
| 16 | PAL / 50 Hz scope | Open [UNKNOWN] | Out of scope until a named need exists (FEATURES) | Owner confirmation |
| 17 | Evidence policy: differential output is triage only | Proposed default | E4 never backs a correctness claim (FEATURES, ARCHITECTURE, AGENTS.md). PITFALLS C1 reads differently (conflict C12) | Owner confirmation |
| 18 | Release policy details | Open [OWNER] | None from research | Owner |

**Rights gates (apply to every phase).** No GPL or unclear-licence fixture is admitted before decision 8 is made. No commercial ROM, BIOS, System Card, disc image or private save is committed or used in public CI. A hash does not grant redistribution rights. Reference emulators are read for behaviour only. No code is copied from them until the licence decision and a provenance record exist.

---

## Key Findings

### Recommended Stack (STACK.md, confidence MEDIUM)

The stack is a Rust-first proposal with a deliberately empty dependency list for the core. The C path is kept as a real alternative. Tool versions and licences were read from official release APIs and documentation on 2026-11-09 and are rated HIGH by STACK. The language ranking is judgement (MEDIUM). It is the only language evidence in the set, because ARCHITECTURE and PITFALLS are language-neutral by design.

| Technology | Verified version or status (2026-11-09) | Role | Why (proposed) |
|---|---|---|---|
| Rust stable + Cargo, pinned with `rust-toolchain.toml` | 1.99.1, released 2026-10-16 [FACT] | Core, facade, runner, tests | Tier 1 on the three host targets STACK checked (aarch64 macOS, x86_64 Linux glibc, x86_64 Windows MSVC) [FACT]. Makes whole classes of memory defects unrepresentable in safe code [PROPOSAL] |
| Rust edition 2024 | Stable since 1.85.0 [FACT] | Language level | No migration debt in a greenfield project |
| Core crate: `#![forbid(unsafe_code)]`, `no_std`-capable, zero third-party runtime dependencies | n/a | Machine core | Flat dependency tree by construction. Enforced by `cargo-deny` [PROPOSAL] |
| C ABI facade crate with hand-written `duospool.h` | n/a | The only crate with `unsafe` | Keeps the ABI small and unfrozen. `panic = "abort"` plus an explicit status code on every entry point. Since Rust 1.81 a panic escaping a plain `extern "C"` function aborts rather than being undefined behaviour [FACT] |
| ca65 / ld65 (cc65) | Tag V2.19 (2020-11-20); `master` active [FACT] | Fixture assembler (primary) | Zlib licence. Its HuC6280 instruction table covers block transfers, ST0-2, TAM/TMA, SXY/SAX/SAY, CSH/CSL, SET, TST, BSR [CODE]. Write a project-owned linker config rather than the stock PCE target [PROPOSAL] |
| WLA-DX | v10.7 (2026-06-28) [FACT]; GPL-2.0-or-later | External cross-assembler for a byte-identity check only | Independent encoder. Not vendored. Whether output binaries carry GPL obligations is [UNKNOWN] |
| `cargo-deny` | 0.20.2 (2026-07-09); Apache-2.0 [FACT] | Fast gate: licence allow-list, advisories, banned non-workspace crates in core and facade | Enforces "does not constrain the future licence" [PROPOSAL] |
| `cargo-fuzz` (libFuzzer) | 0.13.2 (2026-06-09) [FACT] | Scheduled fuzzing of the loader and bounded execution, in a separate crate outside the workspace | Needs a nightly toolchain; Unix-like x86-64 and aarch64 only [FACT] |
| Miri | Nightly component [FACT] | UB checks on the facade crate only | Staged, not in the PR path [PROPOSAL] |
| `cargo test` + project-owned conformance runner | Bundled | Unit tests; manifest-driven runner with a closed outcome enum | Built-in test tools cannot model the distinct outcome taxonomy [PROPOSAL] |
| `wasmtime` | v50.0.1 (2026-10-29); Apache-2.0 [FACT] | Staged wasm repeatability check | Only once wasm is promised |

**Not recommended (with reasons from the files).** Zig: pre-1.0 and its 0.17 release removed `@cImport` and broke tooling [FACT]. cbindgen as the public contract: it freezes whatever the code happens to look like; revisit after a consumer proves the shape [PROPOSAL]. AFL++: AGPL-3.0-or-later, external tool use only [FACT]. PCEAS/HuC as fixture tools: licence unclear [FACT]. Macroassembler AS: a third assembler without a demonstrated need [PROPOSAL]. Default-hasher maps, wall-clock time, threads and floating point in core machine behaviour [PROPOSAL].

**Alternative (C11, CMake 4.4.5, Ninja 1.13.2, LLVM 23.1.5; clang 21 is present locally) [FACT, CODE].** C11 is ranked second because it wins under four conditions: the owner prioritises a toolchain already installed (locally clang, CMake and Ninja are present, while rustc, cargo and zig are not [CODE]); direct reuse of C code or a LibFuturo adapter; C-only contributors; or sanitizers and libFuzzer without a nightly compiler. On this machine Apple clang 21 does not ship the libFuzzer runtime [CODE]. Homebrew llvm@21 is the likely fix and was not tested [UNKNOWN].

**Phase 1 spike [PROPOSAL].** At most one day. Build both skeletons with the same fixture manifest: an opaque create/step/destroy instance, a status-code path, a 64 KiB HuCard loader with refusal cases, a C consumer built against an installed prefix with source paths removed, a 60-second loader fuzz run, and a wasm build. Record clean and incremental build time, test time, lines of glue and tools required.

**Determinism rules for the core [PROPOSAL, from STACK].** No `std::time`, threads, files, environment or randomness in the core. No default-hasher `HashMap` or `HashSet`. No floating point in machine behaviour; host colour conversion belongs to the consumer. Explicit wrapping or checked arithmetic on 8- and 16-bit machine values, with overflow checks on in tests. One owned state struct per instance, and tests that run two instances interleaved to catch hidden sharing.

**Fixture toolchain [PROPOSAL, licence-dependent].** Primary: ca65/ld65 at V2.19 or a named `master` commit, built from source with a recorded hash, plus project-authored startup code, reset vector, MPR setup and register constants. Cross-check: WLA-DX v10.7 on the common subset, requiring byte identity. The dual-assembler check was not run in research [UNKNOWN]. Not admitted: PCEAS/HuC runtime or startup code, and a self-written assembler as the only source of fixture bytes (its errors would correlate with the emulator's decoder).

**Fast gate.** One local command (proposed `scripts/check`) runs fmt, clippy, unit tests, loader refusals, the conformance inventory with negative controls, `cargo-deny`, and the C consumer proof [PROPOSAL, from STACK].

### Hardware Baseline the First Profile Depends On

Each row shows the label and the basis. Rows marked CONFLICT are carried forward in the conflicts table below.

| Fact | Label | Basis and notes |
|---|---|---|
| Master clock 21.47727 MHz (6 times the NTSC colour burst). CPU high speed = master/3 (7.16 MHz), low speed = master/12 (1.79 MHz). CPU powers up in low speed | [FACT] for the speeds. Power-up speed is author-caveated | Manual pages per ARCHITECTURE (S1, S7, S8). Power-up speed also in pcetech [REPORT] |
| Timer: master/3 then /1024, about 6.99 kHz (about 143 us); 7-bit down counter; write-to-start and reload rules | [FACT] (manual pp. H8-20/21 per ARCHITECTURE). Enable-edge reload behaviour [CODE] (MAME, Mednafen) | Underflow read value and read-while-reload edge cases are emulator-specific [UNKNOWN] |
| Interrupt priority RESET > NMI > BRK > TIMER > IRQ1 > IRQ2. Vectors IRQ2/BRK $FFF6, IRQ1 $FFF8, TIMER $FFFA, NMI $FFFC. Inputs sampled at the last bus cycle. IRQ1 and IRQ2 are level-sensed; NMI is edge-sensed | [FACT] (manual pp. H8-12..14 per ARCHITECTURE) | All four emulators check TIQ, then IRQ1, then IRQ2 [CODE], consistent with the manual |
| Block transfers (TII/TIN/TIA/TAI/TDD): length 0 means 65,536 bytes; 17 + 6 x length cycles, i.e. about 393k cycles, about 54.9 ms, about 3.3 frames at high speed [FACT, derived] | Length and cycle formula [FACT] (manual p. H8-6 per ARCHITECTURE; MAME table [CODE]). "Not interruptible mid-instruction" is the working rule [REPORT], consistent with the manual sampling rule. Exact interrupt latency around block transfers and after CLI/RTI [UNKNOWN] | Drives the "advance can overshoot a frame" design |
| MMU: 8 MPR registers select 8 KiB physical banks; 2 MiB physical space; reset vector at physical $1FFE; MPR7 = 0 at power-up, other MPRs and registers random | [FACT] (manual pp. H8-3, H8-7, H8-10/11 per ARCHITECTURE). Random power-up is [REPORT]. MPR values beyond MPR7 [UNKNOWN] | TAM #0 and TMA #0 behaviour [UNKNOWN] |
| T flag: set by SET, cleared at the next fetch; ADC, AND, ORA and EOR redirect to zero page via X; saved across interrupts | [FACT] (manual p. H8-5 per ARCHITECTURE). Title-screen dependency in MAME's history [REPORT] | Cycle counts for T-mode operations: MAME adds cycles [CODE] |
| VDC and VCE register accesses cost one extra CPU cycle | [REPORT] (pcetech), corroborated by [CODE] (MAME, Geargrafx) | Geargrafx adds more during block transfers [CODE] |
| VCE palette: 512 entries of 9 bits (G3 R3 B3); 256 background and 256 sprite entries; index = sprite flag : palette : pixel | [FACT] (manual pp. H6-3/4 per ARCHITECTURE) | Palette-write streaking during active display [REPORT], not modelled by any emulator read [UNKNOWN] |
| VCE dot clocks: master/4 and master/3 documented in the manual page read; master/2 (10.74 MHz) implemented by several codebases | /4 and /3 [FACT]. /2 [CODE]. Register encoding [UNKNOWN] (C4) | pcetech's register section gives 7 and 5 MHz, which is internally inconsistent [REPORT] |
| Active line = 1365 master clocks (about 227.5 colour-burst cycles) | [CODE] (three codebases) | Not stated on the manual pages read |
| Lines per frame: 262 or 263 | CONFLICT (C2) | Frame length 357,630 (262) or 358,995 (263) master clocks; about 60.05 Hz or 59.83 Hz [FACT, derived] |
| VDC: 64 KiB VRAM (32K x 16-bit words); 16 sprites per line; 64 sprites in the SATB; overflow sets a status flag | [REPORT] (pcetech) and [CODE] (MAME, Mednafen) | Upper VRAM address half open or undefined [CODE] |
| VDC register latch rules: BXR and BYR take effect on the next line; MWR width after VBlank; VDW at the start of the display period | [FACT] (manual pp. H7-6..10 per ARCHITECTURE) | These are the rules a latch-point implementation must follow |
| VRAM CPU access slots per dot-clock mode (1-, 2- and 4-dot), BSY status | Slot model [FACT] (manual p. H7-8 per ARCHITECTURE). BSY meaning [REPORT] | No emulator read models DMA timing faithfully: MAME marks it incorrect [CODE] |
| Raster compare: internal counter starts at $40; valid RCR roughly $40 to $146 | [REPORT] (pcetech). A MAME comment places the last valid RCR at $130 with 240 active lines, marked as confirmed on hardware [CODE] | Exact IRQ position within the line: CONFLICT (C3) |
| I/O port $1000. Write: D0 = SEL, D1 = CLR. Read: D3-D0 pad data (active low), D6 = country (1 = PC Engine), D7 = CD sense (1 = absent) | [REPORT] (pcetech), [CODE] (Geargrafx) | Meaning of D6 disputed (C6) |
| 2-button pad: SEL high reads directions, SEL low reads buttons; CLR high forces D3-D0 to zero; scan about 9 cycles after SEL | [REPORT] (pcetech); secondary [FACT] (PCE_Controller_Info, per FEATURES) | 6-button, multitap and mouse protocols: secondary sources only; the manual was not read for them [UNKNOWN] |
| PSG: 6 channels; 32-sample x 5-bit waveform RAM; noise on channels 4-5 (0-based); LFO on channels 0-1 (1 modulates 0) | Waveform and channel count [FACT] (wiki per ARCHITECTURE). Noise and LFO placement [CODE]. LFO and noise details [UNKNOWN]. PSG manual S4 downloaded but unread | Other files number channels 1-based; normalize before writing fixtures |
| PSG effective tick: master/6 (3.58 MHz) in codebases, versus OSC1/3 (7.16 MHz) on the manual's clock pin | CONFLICT (C1) | Do not claim a PSG pitch or timing result until measured |
| HuC6280 vs HuC6280A: the PSG output differs (a DC offset that decays, audible as a pop) | [REPORT] (observational; PCE_Soundtest, MIT). Which models ship which revision: CONFLICT (C14) | Only analog capture can distinguish the two. E3 |
| SuperGrafx: a second VDC, a HuC6202 VPC, 32 KiB work RAM, shared IRQ1 | [REPORT] (MiSTer sgxtech, author-caveated). Work RAM size also [CODE] | VPC priority modes 01b and 10b are interpretation only |
| CD-ROM²: 64 KiB CD RAM at banks $80-$87; Super System Card adds 192 KiB at $68-$7F; CD registers at $1FF800; 2 KiB BRAM | [REPORT] (pcetech), [CODE] (MAME, Beetle). Many CD register details hedged by the author | Deferred. System Card is firmware |

### Expected Features (FEATURES.md, confidence MEDIUM)

FEATURES defines "table stakes" as what is required for the core value to hold for one profile ("a fresh, deterministic, headless run reproduces the behaviour and shows the evidence"). It does not mean what a general emulator user expects. The first-profile set it proposes [PROPOSAL]:

- **Named profile record**: region field, CPU revision, video timing, pad type, expansion port, media type. Low complexity. Values are [OWNER]; the shape is [PROPOSAL].
- **HuC6280 core**: 65C02 superset, MMU, timer, IRQ controller, I/O port, speed switching. Medium-high complexity. Oracle: manual vectors (E1), authored per-opcode fixtures (E2). Cycle-count claims need E3 or a primary document. Do not claim cycle accuracy from E4 alone.
- **Bus and memory map**: 8 KB work RAM, HuCard pages, I/O, the hardware page. Unmapped reads return $FF [REPORT], tested as a boundary.
- **VDC** (background, 16 sprites per line, VRAM, SATB DMA, IRQ) and **VCE** (512-colour palette, dot clock). High and medium complexity. Oracle: authored display fixtures with literal tile and sprite expectations.
- **PSG** (6 channels, waveform RAM, noise, LFO, balance) to evidence level. Oracle: register-to-event log against documented semantics (E1). A non-zero audio buffer is not an adequate check.
- **2-button pad** through the real SEL/CLR protocol. Negative control: no input and wrong input must produce different guest results.
- **HuCard loader** with explicit validation and refusal (size, mapper, header, mirroring). Malformed, oversized, zero-length and unsupported-mapper refusal tests.
- **Deterministic scheduler** with bounded machine time and a fresh instance per run; **scripted logical input** with machine-time stamps.
- **Headless runner** emitting observations with distinct outcomes (pass, guest failure, timeout, harness failure, skip, cancelled, missing artifact).
- **Raw video and raw audio capture** before any colour conversion, scaling or device output.
- **Minimal trace** (CPU instruction plus device-register write log). A diagnostic, not an oracle.
- **Authored diagnostic fixture suite** with a manifest (source, build recipe, hashes, licence, notices, redistribution terms, test meaning).
- **Installed-consumer proof**, and an **internal snapshot/restore** that tests state completeness (restore-then-continue equals continue). This is in-memory only (conflict C13).
- **Instantiable component seams** (VDC, VCE pixel bus, per-port input, typed stores). Cheap now, costly to retrofit.

**Differentiators (later, each with a trigger).** Evidence report per run; machine-time-stamped input; profile-hash-bound replay; a reference and fast VDC pair as an internal differential (E4-class only); 6-button, multitap and host-side turbo; the Street Fighter II' mapper (2.5 MB) and Populous RAM; BRAM, Tennokoe Bank and Memory Base 128; a HuC6280A PSG profile (E3 only); a debugger; a libretro or LibFuturo adapter; SuperGrafx; CD-ROM² and Super CD-ROM²; Arcade Card; a user-facing save-state file; rewind.

**Anti-features (deliberately not built, or not in the core) [PROPOSAL].**
- Unqualified "accurate" or "compatible" claims, game lists, title-screen screenshots as proof, or emulator agreement presented as correctness.
- CRC- or title-string special-casing in the core. A host-side database may suggest a profile but never override it silently.
- Auto-detecting HuCard byte order or region and guessing. Fail-open: a wrong guess means a wrong machine.
- Behaviour-changing enhancements in the core, such as removing the 16-sprite limit. Mednafen's own notes say this causes glitches and desyncs. If ever offered, it is a distinct, hashed profile variant.
- Bundling system cards, BIOS, commercial ROMs, disc images or private saves in the repository or CI.
- Shaders, CRT filters, scalers and composite simulation in the core. Presentation belongs to the host.
- A core-owned filesystem, wall-clock pacing, device discovery or windowing.
- A frozen public ABI, or a LibFuturo dependency, before a consumer proof.
- Netplay, TurboLink-style multi-instance and run-ahead in v1.
- CD+G, LaserActive, the HES music player, PAL/50 Hz TG-16, and mouse, XE-1AP or Pachinko input in the first slice. No named user outcome or source yet [UNKNOWN].
- Broad fuzzing, large compatibility matrices and noisy performance trends in the fast PR gate.

**Oracle ladder [FEATURES and ARCHITECTURE, consistent].** Manual-derived literals are the strongest cheap tier and should be cited by page in the manifest. SingleStepTests/65x02 (MIT) checks shared 65C02 opcodes only, not HuC6280 cycle counts. Authored raster-alignment fixtures (the idea behind Covell's test, reimplemented) are the route for CPU and VDC timing. Covell's own ROM has no stated licence and cannot be admitted. Hardware captures with recorded setup are the owner-gated route to settle [UNKNOWN] items. Other emulators are disagreement finders only.

### Architecture Approach (ARCHITECTURE.md, confidence MEDIUM-HIGH for base hardware)

One instance equals one machine. The core owns machine state and the master-clock timeline, the input protocol machines, the VDC, VCE, PSG and CPU behaviour, and the cartridge and mapper behaviour. The host owns files, paths, wall-clock pacing, device I/O, colour conversion, audio resampling, persistent-storage policy and UI. Time, input and content enter through one primitive, `advance(until, input_events, limits)`. It stops only at an instruction boundary or a resumable block-transfer byte boundary, and it returns a stop reason, the machine time reached, and the frame, line and dot phase. A frame wrapper (`run_frame`) is a thin convenience over the same primitive [PROPOSAL].

| Component | Owns (proposed) | Must not |
|---|---|---|
| Profile | Immutable machine configuration, fixed at instance creation | Change after creation |
| Scheduler / timeline | The u64 master-clock counter, `advance` stop conditions, device catch-up | Read the host clock; allocate per step |
| CPU (HuC6280) | Registers, MPR, speed, T flag, timer, interrupt request and disable registers, the `$1000` port wiring, block-transfer micro-state | Know about video or host formats |
| Memory map | 256-entry bank table, work RAM, hardware-page decode, I/O buffer latch | Become a general bus framework or dispatch per byte |
| Cartridge / mapper | ROM bytes, mirroring, optional bank switching and battery RAM, with read/write hooks for hardware offsets | Perform file I/O or decompression |
| VCE | Colour table, dot-clock divisor, line and frame counters, HSYNC/VSYNC generation, raw line output | Convert to host colour spaces |
| VDC | Registers, VRAM, SATB, DMA, status and IRQ, background and sprite pipeline, phase machine, CPU access slots | Assume it is the only VDC, or read VCE internals |
| PSG | Channel state, waveform RAM, noise, LFO, DDA, timestamped write log, PCM render | Apply device filters in the raw path |
| Input / pad ports | Per-port protocol state machine (SEL, CLR, counter) and device type | Count frames (turbo belongs to the host) |
| Observers | Trace sink, peek, state digest, raw capture | Mutate state or affect timing |
| State | One versioned blob per instance, in-memory first | Carry ROM bytes (identity hash instead) |
| Host (outside core) | Runner, frontend, LibFuturo-shaped adapter, presentation | Be imported by the core |

**Patterns (proposed).** (1) Single master-clock timeline with access-time catch-up: a hardware-page access first advances the devices to that timestamp. (2) Latched-register line renderer with explicit latch points (the Mednafen model). (3) Profile as data, with features as specialization. (4) Read-only observation taps that never clear status or the I/O latch. (5) Block transfer as a resumable state machine stored in state.

**Anti-patterns to avoid (proposed).** Frame- or scanline-stepped lockstep as the contract. A general bus framework before a second consumer exists. A second, faster scanline renderer beside the accurate one (Ares forces accurate mode because its scanline renderer is too buggy [CODE]). Ambient randomness or wall-clock time. Host colour conversion or filtering inside the core output. Per-frame turbo counters in the core. A 32-bit button mask as the internal input model. Copying code from GPL emulators. Treating emulator agreement or a matching hash as correctness. Frames-only advance with unbounded overshoot.

**Over-generalization test (proposed).** An abstraction with one implementation and no named second consumer is a function or a struct field, not an interface. Two interfaces are justified now: the cartridge mapper (HuCard variants are real and plentiful) and the VDC-to-VCE pixel stream (SuperGrafx is a documented insertion point).

**Scheduling recommendation (proposed).** Start with "always catch up to the access timestamp and to the instruction end." Move to lazy, event-driven scheduling only after the fixtures pass and a profile shows the cost. Power-on randomness comes from a seeded generator carried in state.

**Raw video and audio (proposed).** Raw video is per line: 9-bit VCE words, active-window geometry, dot-clock divisor, and a master-clock origin for each line. Frame composition is a derived view. Raw audio is a PSG event log (timestamp, register, value, plus an initial snapshot) and PCM from a per-source exact fractional accumulator at a defined integer-ratio rate. Non-integer samples per frame (about 734.3 at 44.1 kHz on 262 lines, about 737.1 on 263) must be reported per frame.

**LibFuturo fit (PROJECT.md describes the shape; no researcher read its source, per AGENTS.md).** Shape: opaque instance; content with a config id; a 32-bit logical-button mask; bounded advance with input sampled at advance start; caller-owned gray8, RGB24 and index8 video; interleaved S16 audio; capability queries; optional persistent media.

- Fits: opaque per-instance state; bounded advance, provided it stops at instruction boundaries; a frame wrapper over the internal primitive.
- Mismatches to prove with a real adapter, not assume: a 32-bit mask cannot carry 5 ports of 12 buttons (60 bits) or mouse deltas, though it fits one 2-button pad (8 bits); input sampled at advance start cannot express mid-frame scans; a single content span cannot carry a system card, a disc and backup RAM; block transfers can exceed one frame; index8 cannot carry a 512-colour palette; per-line width and 262/263 variation need a geometry query; the PSG event log has no slot; samples per frame are not an integer; several persistent stores need a typed list; the profile vocabulary is DuoSpool's, not LibFuturo's.

**Build order and slices (ARCHITECTURE section 7, adopted by FEATURES' P1 list).** Slice 0 is foundations: profile, timeline, trace, peek, manifest format, refusal paths, runner shell. Slice A is the CPU, bus, timer, IRQ and pad, which produces evidence without video. Slice B is the minimal VCE and VDC with frame timing, VBlank and RCR IRQs and background rendering. Slice C is the complete base VDC. Slice D is the PSG, which can run alongside C. Slice E is the packaged artifact and independent consumer.

### Critical Pitfalls (PITFALLS.md, confidence MEDIUM)

PITFALLS maps each pitfall to a phase and lists its warning signs. The highest-impact items for the roadmap:

| # | Pitfall | Prevention (proposed) | Phase |
|---|---|---|---|
| C1 | Hash agreement, title screens or emulator agreement recorded as correctness. Evidence: the reference emulators disagree with each other and with hardware (Cadash, Burai II, Geargrafx and MAME issues) | Evidence tier on every record. A golden hash must come with a semantic assertion that fails if the hash was regenerated from a broken core. Never regenerate goldens with an update flag in the same change as logic | Harness, Fixtures |
| C2 | Scripted input the guest never consumes. Vacuous passes when input and no-input hash the same | No-input, wrong-input and correct-input runs with distinct semantic outcomes, asserted in that order. Log guest pad reads in the observation stream. Express input in machine time, not host frames | Input, Harness |
| C3 | Frame-level execution hides raster behaviour; geometry is not constant (mid-frame resolution changes in real games) | Per-line records. Master-clock scheduling. No `FRAME_WIDTH` constant. Scheduler can stop at a named line | Video, Scope |
| C4 / C5 | Community timing documents used as the spec; HuC6280 quirks (T flag, block transfers, IRQ latency, timer, speed switch, wait states) ported as expectations | A claim ledger: statement, sources, disagreement, status (spec, measured, inferred, guessed) and the test that pins it. Quirks as named profile parameters marked [UNKNOWN] until measured. Two independent sources before any behaviour is marked measured | Scope, Core CPU |
| C6 | VDC treated as "registers plus frame renderer" | State machine on the master-clock timeline with access-slot and DMA hooks. Declare a simplification level (L0 no stalls, L1 fixed stalls, L2 slot model) and label every claim with its level | Video |
| C7 | Rights: vendored test ROMs with unclear terms; HuC-built fixtures linking an unspecified runtime; commercial strings in logs or artifacts | Manifest gate that fails closed on any fixture with licence unknown. No GPL fixture before the owner's decision. Log redaction test. Private-corpus runs emit only pass/fail class and opaque IDs, upload nothing, and never run from forks | Fixtures/Rights, CI |
| C8 / C9 | Non-deterministic harness and undefined power-on state; skipped or missing checks look green | Power-on seed policy, with each fixture passing under at least two policies. Double-run and reordered-run equality. Required inventory with a closed outcome enum where only `pass` is green. Negative controls that prove the gate fails when a required test is removed | Harness, Core CPU, CI |
| C10 | Heuristic loader identification (size, bytes, CRC); unbounded reads | Typed loader result (ok, refused with reason, unsupported). Refusal reasons enumerated and tested. Declared orientation and mapper; ambiguity is a refusal. Size cap before allocation. Short reads are failures | Content/Security |
| C11 / C12 | Scope creep and implicit profiles; a generic Bus trait or public ABI frozen before a consumer exists | Profile is a first-class field in every record. SuperGrafx and CD code unreachable from the base profile. Concrete machine interface first; abstract after two consumers exist. No public header before the first consumer | Scope, Packaging |
| M1 / M2 | PSG modelled as a wavetable; joypad as a latched byte; multitap and 6-button shortcuts | Register-event log and literal waveform expectations. CLR/SEL state machine per peripheral. A "known hardware incompatibility" note class so a failing title is not counted as a bug without evidence | Audio, Input |
| M6 | Copying another emulator's structure, licence or comments (including stale "FIXME" guesses) | Clean-room rule. Provenance record (file, commit, licence, notice, maintenance cost) for any copied helper | Scope, all |

The other pitfalls (M3 mappers, M4 colour conversion, M5 oracle gap, M7 save data, M8 performance, M9 CI bloat, M10 consumer proof, and the minor items) are in PITFALLS.md with their phase mapping. Game-specific traps (After Burner, Ankuku Densetsu, Crude Buster, Mizubaku Daibouken, Cadash, Crest of Wolf, Dai Makaimura, Sorcerian and others) are hypothesis cases only. Use them after the underlying primitive has a fixture, never as requirements.

---

## Cross-Source Conflicts (carried forward, not resolved)

Each conflict is listed with the competing positions and labels, and with how the roadmap should treat it. None is resolved by this synthesis.

| ID | Topic | Competing positions (source, label) | Roadmap treatment |
|---|---|---|---|
| C1 | PSG clock | Effective tick master/6 = 3.58 MHz (pcetech [REPORT]; Geargrafx, Mednafen, Ares code [CODE]). Versus OSC1/3 = 7.16 MHz on the manual's clock pin (manual p. H8-23 per ARCHITECTURE [FACT]); patents cited by pcetech say 7.16 MHz, and pcetech's own formulas imply 3.58 [REPORT] | Profile parameter. Placeholder master/6 marked [UNKNOWN]. No PSG timing or pitch claim until a measurement exists (owner gate, decision 10) |
| C2 | Lines per frame | 263 fixed (pcetech [REPORT]; FEATURES; PITFALLS; MAME TODO "add 263-line mode" [CODE]). Versus 262 default with 263 selectable by a VCE control-register bit (Mednafen, Geargrafx, Ares code [CODE]). MAME's own screen configuration uses 263 [CODE] while its TODO says the 263 mode is missing. The manual pages read do not state a line count | Profile field lines_per_frame in {262, 263}. First-profile choice is an owner or measurement decision. Frame length and frame rate claims are withheld until chosen |
| C3 | VBlank and raster IRQ position | VBlank: first line after the active area (pcetech [REPORT]). Versus the beginning of the HDW period after VDW ends (MAME [CODE]). Covell reports Mednafen's VSync interrupt fires late compared with hardware [REPORT]. RCR IRQ: 4 VDC clocks before HDW end (Mednafen [CODE]) versus near the end of HDW (MAME [CODE]). Exact hardware position [UNKNOWN] | Fixtures record the IRQ scanline and phase into RAM and assert no literal line number until a hardware capture or primary-manual page settles it (owner gate) |
| C4 | Dot-clock modes and register encoding | Manual page read documents DCC 0 (/4) and 1 (/3) only [FACT]. Codebases implement /2 as well [CODE]. pcetech's register section says 7 and 5 MHz [REPORT] | Implement three modes, as ARCHITECTURE recommends. Register encoding [UNKNOWN]. Verify against the HuC6260 page before the first VCE fixture |
| C5 | Active width in the 7 MHz class | 320 pixels (pcetech display table [REPORT]; ARCHITECTURE). Versus 341 (Geargrafx `k_huc6260_line_width` table and Mednafen's overscan comments, 341.3 [CODE]; PITFALLS) | Unresolved. Raw output is the full line plus active-window coordinates, not a fixed width |
| C6 | Meaning of $1000 bit 6 | Country or region: 1 = PC Engine (pcetech [REPORT]; code [CODE]). Versus a "boot ROM present" reading (one teardown author, disputed [REPORT]). FEATURES records both | Name the bit by observed behaviour in fixtures. Do not write "region" as settled. Test the behaviour, not the semantics |
| C7 | HuCard data-line order (TG-16 vs PC Engine) | Reversed wiring on the North American card (secondary articles, Wikipedia, FEATURES [REPORT]). ARCHITECTURE did not pin it [UNKNOWN]. MAME applies a bit reversal when a byte at offset $1FFF is below $E0, with no explanation of the threshold [CODE]. MiSTer notes its US BIOS is already bit-swapped [CODE] | Loader requires a declared orientation. Ambiguity is a refusal. No silent swap and no auto-detection (all four files agree on the rule even though the facts are open) |
| C8 | Copier header detection | Beetle strips 512 bytes when the size has bit 9 set [CODE] (FEATURES). MAME detects a header only when length mod 16 KB equals 512 [CODE] (PITFALLS). The two heuristics disagree | Header is declared in the manifest. A header-prefixed image is a refusal and a negative-control fixture |
| C9 | Block-transfer interrupt latency | Not interruptible mid-instruction (manual or pcetech [FACT or REPORT]; Geargrafx [CODE]). Versus exact latency around block transfers and after CLI/RTI [UNKNOWN] (PITFALLS). MAME's history records a delayed-interrupt fix for a pending instruction [REPORT] | "Not interruptible" is the working rule. Latency is a named parameter with a fixture. The matching game title is a hypothesis case only |
| C10 | ST0-ST2 bypassing the MMU | ARCHITECTURE: write directly to the VDC, bypassing the MPR mapping (manual p. H8-3 [FACT]). PITFALLS: same, from a forum summary [REPORT, low] | Verify on the cited manual page at the start of the CPU phase. Low stakes, since both files agree |
| C11 | Mid-frame pixel-clock and width changes in Mednafen | Mednafen's VCE code marks mid-frame pixel-clock changes as known-wrong [CODE] (PITFALLS), while its per-line width table handles them [CODE] (ARCHITECTURE) | Mid-frame width change is in the raw record, but its correctness is [UNKNOWN]. No fixture asserts it before a hardware check |
| C12 | Evidence-tier semantics | PITFALLS C1 says only the last three tiers (differential, authored diagnostic, hardware) can back a correctness claim. That would admit emulator differential and exclude documented vectors. FEATURES and ARCHITECTURE treat E1 documented vectors as valid and E4 as triage only. AGENTS.md treats emulator agreement as not correctness | Adopt the stricter FEATURES and ARCHITECTURE reading: E4 never backs correctness. Owner confirms (decision 17) |
| C13 | Save-state scope in slice 1 | FEATURES and ARCHITECTURE: an in-memory snapshot in the first slice, with state/ as a first-class module. PITFALLS M7: no save-state format in the first slice | Compatible if "format" means a persisted file. Keep the in-memory snapshot in the slice; defer any on-disk versioned format. Owner confirms (decision 15) |
| C14 | Which models have the HuC6280A | "Some CoreGrafx" (PCE_Soundtest [REPORT]). Versus CoreGrafx has the A and CoreGrafx II reverts (Wikipedia [REPORT]) | Per-model profile claims need per-model evidence. Deferred |
| C15 | PCE_Soundtest admissibility | MIT licence, admissible with notice (FEATURES, ARCHITECTURE). Versus MIT, but its startup code is HuC-generated, which inherits HuC's unclear status, and it is an analog capture with no waveform oracle; not for the fast suite without a rights review (STACK) | Not admitted. STACK's stricter reading governs. Owner rights decision |
| C16 | Fixture assembler | PITFALLS suggests PCEAS "or equivalent" with no HuC runtime. STACK rejects PCEAS (unclear licence, rolling tag) and proposes ca65 (Zlib) | ca65 is the proposed default, pending the licence decision. PCEAS is not admitted |
| C17 | HuC toolchain licence | STACK read the LICENSE file directly: the original authors did not specify conditions, the legal status is hard to state, GitHub reports no licence [FACT]. PITFALLS worked from a summary, not the file [REPORT, low]. FEATURES and ARCHITECTURE did not check [UNKNOWN] | Treat as unclear and not admissible. STACK's direct reading governs. Owner or counsel decision |
| C18 | Emulator licences | Geargrafx: GPL-3.0 (ARCHITECTURE read the repository [CODE]); PITFALLS lists it as unknown. Ares: believed ISC, to verify (ARCHITECTURE). Mesen2: unknown (PITFALLS). Mednafen-derived code: GPL-2.0-or-later [CODE]. MAME PCE files: BSD-3 per file [CODE] | Read for behaviour only. No copying before the licence decision and a provenance record |
| C19 | Manual access and coverage | ARCHITECTURE read manual pages from S1-S3 as images, cited by page. FEATURES states the primary manuals were not located or read. PITFALLS did not read them | ARCHITECTURE's page citations are the only manual-backed hardware facts in the set. Re-read each cited page when its phase starts. The PDF mirror on cpcwiki returned an error to the researchers |
| C20 | Source labelling for pcetech.txt | [FACT, secondary] (FEATURES). [SRC] (PITFALLS). [REPORT] (ARCHITECTURE) | Treat as community report (see How to Read) |
| C21 | Deferred-scope conflicts (no slot in v1) | Populous battery: battery-backed (Beetle README and reports [REPORT]) versus no battery (teardown [REPORT]). Arcade Card Pro RAM and BIOS: 256 KB with 3.1 BIOS versus 192 KB; the wiki contradicts itself. Tennokoe Bank unlock: a guess in the best-known source [CODE]. Street Fighter II' mapper rounding: Beetle rounds to 5 MB internally, MAME detects at 2.5 MB [CODE] | Deferred. Mappers are declared in the manifest, never detected by title string. Each needs its own fixture and source before any claim |

---

## Implications for Roadmap

The structure below is a PROPOSAL. It is the dependency-ordered structure the four files support. The roadmapper makes the final phase decisions. The first vertical slice (Phases 2 to 8) matches PROJECT.md's seed slice: base profile, HuCard, original diagnostic fixture, headless runner and installed-consumer proof. Phase 1 is a gate phase whose exit criteria are owner decisions and records, not code.

### Phase 1: Decisions, Claim Ledger and Toolchain Spike (gate)

**Rationale:** Language, first profile, licence and scope gate everything after them. The spike is cheap and settles the language question with data. A claim ledger must exist before the first golden hash (PITFALLS C1, C4).

**Delivers:** Decision records for owner decisions 1, 2, 6, 8 and 9 (as far as the owner is ready). A claim ledger skeleton (statement, sources, disagreement, status, pinning test). The fixture manifest schema (source, recipe, hashes, licence, notices, redistribution, test meaning, oracle tier). The closed outcome enum. The provenance rule for copied code. The Rust-versus-C spike (STACK's list), using the same fixture manifest. No fixture admitted until the licence decision is made.

**Addresses:** Named profile record (the shape, with values [OWNER]).

**Avoids:** C4 (community docs as oracle), C7 (rights before any fixture), C11 (implicit profile), C12 (premature API), M6 (copying).

**Research flag:** None beyond the spike, which has a fixed plan. Owner decisions are the real work.

### Phase 2: Deterministic Foundations and Fail-Closed Harness

**Rationale:** Every observation is a function of the scheduler, so the timeline must be cycle-aware from the start (FEATURES, ARCHITECTURE). Negative controls and the inventory must exist before any test is accepted (PITFALLS C1, C9).

**Delivers:** Immutable profile struct. Master-clock timeline with `advance(until, events, limits)` and stop reasons. Read-only peek and trace sinks. Outcome taxonomy and required-inventory check. Negative-control fixtures (deliberately wrong literal, no-input run, truncated image). Determinism tests: double run, fresh process, reordered run, and two power-on policies. The fast gate (`scripts/check`). `cargo-deny` in the gate. A minimal runner shell that can run RAM-verdict fixtures.

**Addresses:** Deterministic scheduler; trace; runner shell; fixture manifest enforcement.

**Avoids:** C1, C8, C9, C12.

**Research flag:** None. Standard patterns.

### Phase 3: HuCard Loader and Content Refusal Matrix

**Rationale:** Content is an attack surface (PROJECT.md, PITFALLS C10). The loader can be built in parallel with Phase 4 once Phase 2 exists. Fixtures need a loader that refuses what it does not support.

**Delivers:** Typed loader result: ok (profile, orientation, mapper, size), refused (reason), unsupported. Plain-ROM mapper with mirroring for 128 KB to 1 MB and the 384 KB case. Header handling that is declared, never guessed. Refusals for empty, truncated, header-only, 513-byte, oversized, non-multiple-of-8 KB, and unknown-mapper images (SF2', Populous, Tennokoe refused explicitly). No mutation of a running instance on refusal. Size-sweep fixtures that read a bank signature through each mirror. Fuzz-finding corpus replay in the fast gate. Scheduled loader fuzzing begins.

**Addresses:** HuCard loader with explicit validation and refusal.

**Avoids:** C10, M3 (mappers as afterthoughts), the security table (unbounded reads, short reads, header arithmetic).

**Research flag:** YES. Byte order (C7), header rule (C8) and mirroring for non-power-of-two sizes are [UNKNOWN] or conflicting. Research the rules and record the chosen policy before the refusal matrix is final.

### Phase 4: HuC6280 Core, Bus, Timer, Interrupts and 2-Button Pad (Slice A)

**Rationale:** The CPU, bus, timer, IRQ and pad produce evidence without video, through RAM verdicts. This is the earliest slice that can show a guest-observable result.

**Delivers:** Full HuC6280 instruction set, including the T flag, TAM/TMA, CSL/CSH, and block transfers as per-byte resumable micro-steps. Interrupt priority with last-bus-cycle sampling. Timer with enable-edge reload. Bank-table memory with work-RAM mirroring. The SEL/CLR protocol for a 2-button pad, with a guest-visible pad-read log. First scripted-input fixture, which passes only when the pad event is consumed, with no-input and wrong-input controls. Manual-literal vectors. SingleStepTests subset for shared 65C02 opcodes, fetched to an uncommitted cache. In-memory snapshot-equals-continue test.

**Addresses:** HuC6280 core; bus and memory map; 2-button pad; scripted logical input; internal snapshot.

**Avoids:** C2 (input never consumed), C5 (HuC6280 quirks), C8, M2 (pad shortcuts).

**Research flag:** YES for targeted timing edges that are [UNKNOWN]: interrupt latency around block transfers and after CLI/RTI, speed-switch cycle count, TAM #0 and TMA #0, MPR power-up values beyond MPR7, and ST0-2 (manual p. H8-3). Opcode tables themselves are a standard pattern taken from the manual.

### Phase 5: VCE and VDC Minimal Raw Video (Slice B)

**Rationale:** The VCE owns the dot clock and the line and frame counters, so frame timing is defined here. Mednafen's model of latching registers at defined phase points is the recommended first approximation.

**Delivers:** Lines-per-frame as a profile parameter. Line and frame counters, sync generation, dot clock. Palette (512 by 9 bits). Background rendering at a defined phase point with latched registers. VBlank and RCR IRQs routed into the interrupt controller. Per-line raw record (9-bit words, active window, divisor, master-clock origin). CPU VRAM access with the one-cycle penalty. Fixtures that record status reads and IRQ scanline and phase into RAM with literal expectations. A raw-frame hash only together with a literal.

**Addresses:** VCE; VDC (background); raw video capture.

**Avoids:** C3 (frame-level hides raster), C6 (VDC as renderer), M4 (colour conversion in core), C1 (hash as oracle).

**Research flag:** YES. VBlank and RCR position (C3), dot-clock encoding (C4), 7 MHz width (C5), mid-frame width change (C11), and the BG phase model. Owner gate for any hardware capture.

### Phase 6: VDC Completeness and Raster Alignment (Slice C)

**Rationale:** Sprites, DMA, access slots and latch rules are where games depend on exact timing. They come after the frame exists, and they are the largest unknowns in the VDC.

**Delivers:** Sprites with the 16-per-line limit, overflow and collision flags, and the 32-pixel double count. SATB DMA with its end interrupt about four lines after the active period. VRAM-to-VRAM DMA. Access-slot stalls and BSY. Latch rules (BXR and BYR next line, MWR after VBlank). Raster-alignment fixtures: an authored version of the one-instruction-per-line idea. The simplification level is declared on every claim.

**Addresses:** VDC (complete for base).

**Avoids:** C6, C5 (wait states), M8 (no per-dot renderer optimization).

**Research flag:** YES. The VRAM slot model per dot mode, DMA timing (MAME marks it incorrect), burst mode, and sprite overflow semantics are [UNKNOWN] or guesswork across emulators. Re-read manual pp. H7-6 to H7-11 before planning.

### Phase 7: PSG and Raw Audio (Slice D; may run alongside Phase 6)

**Rationale:** Audio is the least entangled with the frame, so it can run in parallel with VDC completeness. It depends on the timer and the CPU's timestamped writes.

**Delivers:** Six channels, waveform RAM, noise (channels 4-5), LFO (channels 0-1), DDA, balance. Timestamped register-write log. PCM rendered at an exact integer-ratio accumulator. PSG revision as a profile field. No DC-offset or pop claim without a capture.

**Addresses:** PSG to evidence level; raw audio capture.

**Avoids:** M1 (wavetable shortcut, non-zero audio check), C1.

**Research flag:** YES. Read the PSG manual (S4, downloaded but unread by ARCHITECTURE) before planning. Resolve the clock conflict (C1). LFO registers, the noise generator and seed, and the wave-pointer behaviour when playback is off are [UNKNOWN] (MAME's own open items). Ares seeds noise from its generic random helper [CODE], so a defined generator is needed.

### Phase 8: Headless Runner, Observations and Installed-Consumer Proof (Slice E)

**Rationale:** Completes the seed slice. The consumer proof is a project requirement for the first slice, not a later nice-to-have.

**Delivers:** Runner invocation with profile, content, script, stop condition and output directory. Observations: stop reason, machine time, frame, line and dot, raw video, PSG log and PCM, trace, state digest. Distinct outcomes. Packaged artifact (static library, hand-written header, pkg-config file; decision 12). An independent consumer, in a different language from the core, built in a separate directory and process with source and build paths absent. Package-contents test. Optional LibFuturo-shaped adapter experiment that records named mismatches (not a dependency).

**Addresses:** Headless runner; installed-consumer proof; evidence report per run (first form).

**Avoids:** M10 (consumer built from the source tree), C12 (premature contract), C9.

**Research flag:** None for the mechanics. The host and package format are owner decisions (12).

### Phase 9 and Later: Deferred Scope (each item needs an owner outcome and evidence)

| Item | Trigger | Research flag |
|---|---|---|
| Timestamped input, 6-button (two-scan detection), multitap, host-side turbo | First fixture needing more than one pad or mid-frame input (decision 5) | YES. Read PCE_Controller_Info directly. Mouse timing is [UNKNOWN] |
| Street Fighter II' mapper, Populous RAM, typed persistent stores | Named outcome plus an authored mapper fixture | YES per mapper. Populous battery conflict (C21) |
| TG-16 profile | Owner wants TG-16 claims; needs its own evidence | YES. Byte order (C7), bit 6 meaning (C6) |
| HuC6280A PSG profile | E3 capture available | YES. E3 only |
| SuperGrafx | Owner outcome (decision 3) and a stronger oracle (VPC modes need E3) | YES |
| CD-ROM² / Super CD-ROM² and Arcade Card | System Card rights path (decision 4), disc abstraction, SCSI, ADPCM and CD-DA model, loading-latency oracle | YES, heavy. Architecture confidence LOW-MEDIUM for these |
| Persisted save states, replay, rewind, debugger | Versioned state format; a named outcome (decision 15) | Moderate. Save-state corruption tests (M7) |
| CI scale-out: nightly fuzz for bounded execution and the facade; Miri; fixture reproducibility; wasm digest; performance benchmarks | Phases as listed in the CI table below | Per item |

### CI Staging (cross-cutting, proposed)

Each job maps to one failure class. The fast path stays fast. Stage by measured value, and record setup, fetch, compile and test times after each run before adding caches (AGENTS.md).

| Job | Starts | Failure class | Path |
|---|---|---|---|
| `scripts/check` (fmt, clippy, unit tests, loader refusals, inventory with negative controls, cargo-deny, C consumer proof) | Phase 2 | Logic, inventory, packaging | Fast PR gate |
| Fuzz-finding corpus replay through `cargo test` | Phase 3 | Loader regressions | Fast PR gate (stable toolchain) |
| Loader fuzzing (nightly, time-boxed) | Phase 3 | Memory and UB in loader | Scheduled |
| Fixture reproducibility (rebuild with pinned assembler, compare bytes) | Phase 4 | Fixture provenance drift | Separate job, not PR-blocking |
| Bounded-execution fuzz and facade API fuzz; Miri on facade | Phase 4 onward (execution); Phase 8 (facade) | Memory and UB across the boundary | Scheduled |
| Benchmarks with workload identity | After Phase 6 correctness | Throughput regression | Scheduled or manual; non-blocking |
| wasm digest comparison | Only if wasm is promised | Host portability drift | Scheduled |

Corpus-related jobs never upload commercial-derived output. Any job that touches a private corpus is not triggered from forks and reports `skipped (corpus absent)` as a distinct non-green status on the release gate (PITFALLS C7, C9).

### Phase Ordering Rationale

- The timeline comes before the CPU because every observation depends on the scheduler. Retrofitting access-time catch-up is a rewrite (PITFALLS C3 recovery: HIGH).
- Negative controls and the inventory come before any golden hash. This is the only way to keep a green gate meaningful.
- CPU, bus and interrupts come before video. Slice A produces RAM-verdict evidence with no video dependency.
- The VCE comes before the VDC because it defines the frame timebase that VBlank and RCR interrupts need.
- PSG follows the timer and interrupts. It is the least entangled with the frame, so it can run alongside VDC completeness.
- The loader is built early because every fixture depends on it refusing what it does not support.
- The consumer proof is last in the slice, but a minimal runner exists from Phase 2 so the consumer can be exercised early.
- No fixture is admitted before the licence decision. This keeps Phases 1 to 3 free of rights risk.

### Research Flags

**Needs deeper research during planning (`/gsd-plan-phase --research-phase`):**
- **Phase 3:** byte order, header rule and non-power-of-two mirroring (C7, C8).
- **Phase 4:** interrupt latency around block transfers and CLI/RTI; speed-switch cycle count; TAM/TMA #0; power-up MPR values; ST0-2 (C9, C10).
- **Phase 5:** VBlank and RCR IRQ positions; dot-clock encoding; 7 MHz width; mid-frame width change (C2, C3, C4, C5, C11).
- **Phase 6:** VRAM slot model per dot mode; DMA timing; burst mode; sprite overflow and collision semantics.
- **Phase 7:** PSG manual (S4, unread); PSG clock (C1); LFO, noise generator and seed; wave-pointer behaviour.
- **Phase 9 and later:** every item in the deferred table.

**Standard patterns (skip research-phase):**
- **Phase 1:** decision records, the manifest schema and the spike, which has a fixed plan.
- **Phase 2:** timeline, peek and trace sinks, outcome taxonomy, inventory, fast gate.
- **Phase 4:** instruction-table structure and the opcode vectors from the manual (the timing edges above are the exception).
- **Phase 8:** runner, outcome reporting and packaging mechanics. The host and package format are owner decisions.

---

## Confidence Assessment

| Area | Confidence | Basis |
|---|---|---|
| Stack | MEDIUM | Tool versions and licences HIGH (read from release APIs and licence files on 2026-11-09). Language ranking MEDIUM and single-source. The dual-assembler byte-identity check and the libFuzzer link on macOS were not run |
| Features | MEDIUM | Emulator feature landscape HIGH (code and docs read). Hardware claims MEDIUM at best. Primary manuals were not located or read by this researcher. Many claims rest on pcetech, sgxtech, one teardown author and Wikipedia |
| Architecture | MEDIUM-HIGH for base hardware; MEDIUM for SuperGrafx; LOW-MEDIUM for CD-ROM² and Arcade Card | The only file that read primary NEC/Hudson manual pages, cited by page. Emulator code read directly (Mednafen, Geargrafx, Ares, MAME). PSG manual downloaded but unread |
| Pitfalls | MEDIUM | Code evidence HIGH where read directly. Several hardware-timing items rest on forum summaries or a licence summary, not the file itself |
| Overall | MEDIUM | Architecture-level decisions are consistent across all four files and robust: one master-clock timeline, access-time catch-up, per-line raw video, per-port input, typed content, and rights gates. Individual hardware values are not yet robust enough to back an accuracy claim. Each one is labelled and must be re-verified against its page or measurement when its phase starts |

---

## Gaps to Address

Each gap is listed with how to handle it during planning or execution.

1. **Manual coverage is uneven.** ARCHITECTURE read the HuC6280, HuC6270 and HuC6260 pages it cites. The PSG manual (S4) was not read. FEATURES and PITFALLS did not read the manuals. **Handle:** re-read each cited page when its phase starts, and record the page in the fixture manifest.
2. **No hardware oracle for the timing values that matter most.** VBlank and RCR position, PSG rate, VDC slot phase at power-up, speed-switch latency. **Handle:** owner decision 10. Until a capture exists, carry these as profile parameters marked [UNKNOWN], and label the fixtures as repeatability or semantic only.
3. **Fixture toolchain not proven.** The dual-assembler byte-identity check was not run, because building downloaded toolchain code was blocked in research. The common syntax subset between ca65 and WLA-DX is unknown. **Handle:** run it in Phase 1 or early Phase 4, and record hashes.
4. **Language decision has no data yet.** The spike was not run. **Handle:** Phase 1 spike, then owner decision 1.
5. **libFuzzer on macOS.** Apple clang 21 does not ship the runtime. Linking Homebrew llvm@21 is untested. **Handle:** test it in the spike. If the C path is chosen, fuzz on Linux CI.
6. **Controller protocols are secondary-source only** for 6-button, mouse, multitap and turbo. **Handle:** read PCE_Controller_Info directly before any Phase 9 input work.
7. **Byte order, header and mirroring rules have conflicting code and no pinned primary source.** **Handle:** the loader refuses until a policy is chosen and recorded (Phase 3).
8. **LibFuturo source was never read,** by design (AGENTS.md). Only PROJECT.md describes it. **Handle:** the adapter experiment needs the owner to supply or authorize the current contract (decision 7).
9. **Channel and state numbering differs across files.** PSG channels are 0-based in ARCHITECTURE and 1-based in PITFALLS and the Mesen2 notes. Some other numbering differences exist. **Handle:** normalize in the claim ledger before writing fixtures.
10. **Licences unclear or unchecked** for HuC/PCEAS (C17), Covell's CPU test (no licence stated; not admissible), rgalland's test ROMs (no licence visible; not admissible), the ccovell splitres test (location and licence not found), Ares (believed ISC, to verify), Mesen2 (unknown). **Handle:** none is admitted or copied until the licence decision and review.
11. **Model-level claims** (which console has which CPU revision; CoreGrafx variants; Arcade Card variants) rest on conflicting or secondary sources. **Handle:** per-model evidence before any per-model claim.
12. **Source hygiene.** Two pages fetched by FEATURES' researcher contained text addressed to AI agents asking for changes. The researcher ignored it, and it is not a request from the owner. All fetched text is treated as data in this synthesis. The two Nicole Express articles are single-author teardown notes and are treated as reports.
13. **Scope conflicts:** the in-memory snapshot (C13) and the 262/263 default (C2) are decisions for the owner or the roadmapper, not for research.

---

## Sources

Grouped by the file that read them. The labels in the body apply. Source-type grades are each researcher's own.

**Primary NEC/Hudson manuals (ARCHITECTURE, page-cited; HIGH as primary, redistribution unknown; cite, do not vendor):** HuC6280 CMOS 8-bit Microprocessor Hardware Manual, HuC6270 Video Display Controller Manual, HuC6260 Video Colour Encoder Manual, mirrored in the docs folder of the MiSTer TurboGrafx16 repository (`https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/tree/master/docs`). PSG manual downloaded but not read.

**Toolchain and standards (STACK, HIGH for versions and licences as read on 2026-11-09):** cc65 repository (ca65 documentation, instruction table, `pce.cfg`, V2.19 tag, Zlib) `https://github.com/cc65/cc65`; WLA-DX (v10.7, GPL-2.0-or-later, `legal.rst`) `https://github.com/vhelin/wla-dx`; pce-devel/huc repository (LICENSE text read directly; legal status unclear) `https://github.com/pce-devel/huc`; SingleStepTests/65x02 (MIT) `https://github.com/SingleStepTests/65x02`; Rust platform support, edition guide, 1.81 release note and panic reference; cargo-fuzz and LLVM libFuzzer documentation; CMake presets manual; cargo-deny, cargo-nextest, cargo-c, cbindgen, wasmtime, wasm-bindgen, Catch2 and Unity release metadata; AFL++ README (AGPL-3.0-or-later); Zig download page and 0.17.0 release notes.

**Emulator source (CODE, read directly; not hardware facts):** MAME `pce.cpp`, `pce_rom.cpp`, `pce_slot.cpp`, `h6280.cpp`, `huc6270.cpp`, `huc6260.h`, `c6280.cpp`, `joypad6.cpp` `https://github.com/mamedev/mame`. Mednafen/Beetle PCE `huc6280.cpp`, `vce.cpp`, `pce.cpp`, `huc.cpp` `https://github.com/libretro/beetle-pce-libretro`; Mednafen PCE documentation `https://mednafen.github.io/documentation/pce.html`. Geargrafx (GPL-3.0) `https://github.com/drhelius/Geargrafx`. Ares PCE core `https://github.com/ares-emulator/ares/tree/master/ares/pce`. Mesen2 VRAM-delay commit `https://github.com/SourMesen/Mesen2/commit/b84d4f23cff067209531ab652e3f6b33438a1163`.

**Community reverse engineering and documentation (REPORT; author-caveated; not oracles):** pcetech.txt, by Charles MacDonald, via TurboSharp `https://github.com/asterick/TurboSharp/blob/master/Text/pcetech.txt`. MiSTer sgxtech.txt `https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/blob/master/docs/sgxtech.txt`. pce-devel PCE_Controller_Info `https://github.com/pce-devel/PCE_Controller_Info`. pce-devel PCE_Soundtest (MIT; analog capture) `https://github.com/pce-devel/PCE_Soundtest`. pce-devel HuC wiki (Arcade Card, HuC6280 pages; self-contradictory in places) `https://github.com/pce-devel/huc/wiki`. PCE wiki pages on timing, CPU memory map and addresses `https://pce.nesdev.org/wiki/Timing`. Chris Covell's PC-Engine CPU test `https://www.chrismcovell.com/CPUTest/` (no licence; not admissible). Nicole Express articles (2020 and 2022; single author). Wikipedia TurboGrafx-16 (model table only). TASVideos timing thread `https://tasvideos.org/Forum/Posts/331259`. Geargrafx issue tracker (cited by number).

**Rights and tooling references:** 240p Test Suite PCE source header (GPL-2.0-or-later) `https://github.com/ArtemioUrbina/240pTestSuite`. rgalland/PCEngine_TestRoms (no licence visible) `https://github.com/rgalland/PCEngine_TestRoms`. pce-devel mednafenPceDev releases `https://github.com/pce-devel/mednafenPceDev/releases`. CVE-2025-9809 (libretro-common cue parser; secondary) `https://vuldb.com/cve/CVE-2025-9809`.

**Unread leads (not used as sources):** stum.de BIOS method offsets; consolemods TG-16 region and pinout pages (access refused to the researchers); Atari-Forum PC Engine thread (search snippet only).

**Project inputs:** `.planning/PROJECT.md` (read for this synthesis); `AGENTS.md`; `.planning/bootstrap/PROJECT-SEED.md` and `EMULATOR-DNA.md` (cited by the researchers, not re-read here).

---
*Research completed: 2026-11-09*
*Ready for roadmap: yes, with the 18 owner-gated decisions surfaced and the 21 cross-source conflicts carried as profile parameters or fixture constraints, not as settled facts.*