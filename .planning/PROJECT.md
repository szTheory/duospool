# DuoSpool

## What This Is

DuoSpool is an emulator for the NEC / Hudson Soft PC Engine, sold in North
America as the TurboGrafx-16. It aims to emulate explicitly named hardware and
content profiles whose behavior can be understood, reproduced, tested, and
integrated into useful hosts. It is for the owner first, and for developers or
players who want an emulator whose support claims are tied to evidence rather
than an unqualified "works" or "accurate" label.

The name "DuoSpool" is the owner-selected working name. It does not imply
trademark clearance.

## Core Value

For every hardware/content profile DuoSpool claims to support, a fresh,
deterministic, headless run can reproduce the machine's behavior and show the
evidence — guest progress, machine time, raw video, raw audio — that backs the
claim.

If everything else fails, this must hold. A feature that cannot be reproduced
and evidenced is not supported, regardless of how it looks in a frontend.

## Requirements

### Validated

- ✓ HuCard content loading with explicit validation and refusal of malformed,
  oversized, or unsupported content — Phase 2 (typed refusals, 22-member enum;
  evidence tier E0, repeatability only; no accuracy claim)
- ✓ A test harness that keeps outcomes distinct (pass, guest failure, timeout,
  harness failure, skip, cancellation, missing artifact) and fails closed on
  missing required checks, calibrated with negative controls — Phase 2
- ✓ One documented local command (`sh scripts/check`) that matches the fast PR
  gate, plus CI that runs the same gate — Phase 2 (first green CI run on
  PR 1; gcc/clang/mawk/gawk evidence is CI plus a one-off local container run)

### Active

These are hypotheses until shipped and validated. Detailed, ID'd requirements
live in `.planning/REQUIREMENTS.md`.

- [ ] A portable emulation core with explicit per-instance machine state. The
      core does not depend on host UI, wall-clock scheduling, filesystems,
      physical controller discovery, or audio/video devices.
- [ ] A first named base PC Engine/TG-16 profile (region, console revision, CPU
      revision, video timing, controller signaling, content format) with
      documented evidence for each behavior claimed.
- [ ] Deterministic execution: scripted logical input with explicit timing, fresh
      isolated machine state, bounded execution, and repeatable outputs.
- [ ] Separate observations of CPU/device effects, logical input, machine time,
      raw emulated video (before host scaling/colour conversion), raw audio
      (PSG events and PCM, before device playback), and guest progress.
- [ ] A headless runner (or equivalent consumer) that can load content, apply a
      scripted input, run for a bounded machine time, and emit observations.
- [ ] Original, rights-cleared diagnostic fixtures with a manifest (source,
      build recipe, hashes, license, notices, redistribution terms, test
      meaning) as the dependable fast test suite.
- [ ] An installed-consumer/package proof: an independent consumer built
      against the installed artifact with source/build paths unavailable.
- [ ] Measured, workload-identified performance tracking, with correct output
      held constant while optimizing.

### Out of Scope

Exclusions for the current milestone. Most are deferrals, not permanent
exclusions. Each needs a named user outcome and enough source and test
evidence before it gets a roadmap slot.

- Commercial ROMs, BIOS/system-card firmware, disc images, or private saves in
  the repository or CI — rights boundary; a hash does not grant redistribution
  rights.
- Unqualified "accuracy" or "compatibility" claims (e.g. game lists,
  title-screen screenshots, emulator-to-emulator hash agreement presented as
  correctness) — violates the core value; evidence must be tied to an
  independent oracle or labeled as repeatability only.
- Freezing a public API/ABI before a real consumer proves it — premature
  contracts lock in untested shapes.
- Adopting LibFuturo as a dependency or contract before a real DuoSpool
  core/consumer proof — it is an integration candidate only.
- Copying a sibling project's license, CI machinery, release policy, or field
  layouts — sibling projects are engineering evidence, not inherited policy.
- Physical-hardware qualification claims without recorded setup and
  uncertainty — not available yet; requires explicit owner gate.

## Context

**Domain (source-backed, to be refined by research).** The PC Engine family is
built around the HuC6280 CPU (a 65C02 derivative with an MMU, timer, I/O port
and integrated PSG), the HuC6270 VDC (video display controller) and the HuC6260
VCE (video colour encoder). HuCards are the base content format; some special
cartridges carry extra hardware. The family extends to CD-ROM² and Super CD-ROM²
(system-card firmware, disc tracks, CD-DA and ADPCM audio, extra RAM, persistent
backup memory), Arcade Card, SuperGrafx (a second HuC6270 plus a HuC6202 VPC
for priority/mixing), multitap and other peripherals. Regional variants
(PC Engine vs TurboGrafx-16) differ at least in physical form and in reported
signal and content-protection behavior; the exact differences that matter for
emulation are a research item.

**Engineering lessons (from `.planning/bootstrap/EMULATOR-DNA.md`).** These
practices transfer from sibling emulator projects and LibFuturo: explicit
per-instance state, early headless consumer, installed-package proof, named
profiles, independent oracles, fixture manifests, distinct test outcomes,
fail-closed inventories, staged CI, and source-attributed profiling. These are
engineering evidence, not PC Engine hardware facts.

**Research leads (dated 2026-11-09, not authoritative):** MAME `pce.cpp` driver
(active code with its own TODOs), MiSTer TurboGrafx16 `sgxtech.txt`
(SuperGrafx reverse engineering), TurboSharp `pcetech.txt` (console-measured
timing notes with author caveats), PCE_Soundtest (MIT, HuC6280 vs HuC6280A PSG
differences), 240p Test Suite PCE source (GPL-2.0-or-later display tests).
Primary HuC6280/HuC6270/HuC6260 manuals and test programs with explicit fixture
rights are still to be located.

**LibFuturo.** The reviewed C interface is experimental and in-process. It
offers opaque instances, content plus a config ID, a 32-bit logical-button
mask, bounded frame/tick advance with input sampled at advance start,
caller-owned video (gray8/RGB24/index8) and S16 interleaved audio buffers,
profile/capability queries, and optional persistent media. It has no PC Engine
profile or adapter. Potential mismatches to test: input sample point vs
raster-timed behavior, multitap/turbo input, single content span vs HuCard +
system card + CD tracks, PC Engine palette/geometry/variable timing, audio
cadence.

**Project state.** Greenfield. At initialization the repository contained only
planning configuration, the bootstrap pack, and `AGENTS.md`. After Phase 1 it holds the claim ledger (`ledger/`), the first-profile record (`profiles/`), decision records (`docs/decisions/`), licence files, and a fail-closed shell gate (`scripts/check`). The core language is C11 with CMake and Ninja (record 0001). After Phase 2 it also holds a C11 core shell (`src/core`: profile resolve, HuCard boot/refusal, mapper boundary, power-on policy, state digest), a run-record printer, a conformance runner with 104 required cases, and a 47-stage fail-closed gate. No CPU, video or audio exists yet. The repository is public at github.com/szTheory/duospool; PR 1 (Phase 2 branch) is open and unmerged.

## Constraints

- **Rights**: No commercial ROMs, firmware, system cards, disc images, private
  saves, or credentials committed or used in public CI. Fixture admission
  requires recorded provenance and redistribution terms. Why: legal/security
  boundary that a hash or public mirror does not resolve.
- **Core boundary**: Deterministic machine behavior lives in the core; hosts
  own physical input mapping, file selection, wall-clock pacing, audio/video
  devices, UI, and persistent-storage policy. Why: reproducibility and
  portability.
- **Dependencies**: Small, flat, well-justified dependency tree; a small copied
  helper with recorded provenance can beat a new dependency. Why: owner
  preference; lower maintenance and supply-chain cost.
- **Evidence**: Support claims must name the profile and the evidence tier.
  Matching hashes prove repeatability only, unless tied to an independent
  oracle. Why: core value.
- **CI**: The fast PR path stays fast. Fuzzing, broad compatibility matrices,
  and noisy performance tests are staged by measured value. Why: owner
  preference for efficient CI without weakening evidence.
- **Content as attack surface**: Game images, firmware, disc images, save files,
  and archive parsers are security boundaries. Why: untrusted input.
- **Authority gates**: Licensing, repository visibility, public release, game
  and firmware rights, physical-hardware qualification, and external
  publication require explicit owner decisions. Routine reversible work
  advances autonomously.

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Project name is DuoSpool (working name, not trademark-cleared) | Owner-selected | ✓ Owner decision |
| Target is a PC Engine / TurboGrafx-16 emulator | Owner-selected | ✓ Owner decision |
| Run a source-grounded sibling/LibFuturo review at startup and at each milestone discussion | Owner-selected | ✓ Owner decision |
| Automate useful verification; favor small dependencies; keep CI efficient without reducing evidence | Owner-selected | ✓ Owner decision |
| Close milestones cleanly; target frequent releases when rights, qualification and authority permit | Owner-selected | ✓ Owner decision |
| First vertical slice: base PC Engine/TG-16 profile + HuCard + original diagnostic fixture + headless runner + installed-consumer proof | Seed recommendation; small enough to diagnose, broad enough to exercise input/timing/video/audio/lifecycle seams | — Proposed (candidate, not owner-selected) |
| Exact first profile (region, console revision, CPU revision HuC6280 vs HuC6280A, video timing) | Requires research and owner input | → docs/decisions/0002-first-profile.md (owner-recorded scope; disputed fields open) |
| SuperGrafx, CD-ROM²/Super CD-ROM², Arcade Card, multitap/peripherals in product scope | Each changes architecture, timing, media, rights, and testing cost; must earn a roadmap slot via a named user outcome | — Open (not in v1; not excluded from the North Star) |
| Language and build system | Research recommendation pending; decide at Phase 1 discussion | → docs/decisions/0001-language-build.md (owner-recorded) |
| Host/platform targets, package format | Decide when the installed-consumer proof is planned | — Open |
| Core API/ABI shape and evolution policy | Do not freeze before a real consumer proof | → docs/decisions/0003-api-abi.md (owner-recorded) |
| LibFuturo adoption | Candidate only; requires a prototype adapter or a named concrete mismatch | — Open |
| License and repository visibility | Owner authority; do not copy a sibling's or LibFuturo's provisional license | → docs/decisions/0004-licence-visibility.md (owner-recorded) |
| CPU/video/audio timing fidelity targets, performance budget | Requires research and measurement | — Open |
| Save-state, replay, debugger scope | Not selected | — Open |
| First homebrew/game and fixture rights | Owner/rights decision; authored fixtures preferred for the fast suite | → docs/decisions/0005-fixture-rights.md (owner-recorded) |
| Release policy details | Owner authority | — Open |
| CI provider | Thin wrapper around scripts/check; workflow is Phase 2 | → docs/decisions/0006-ci-provider.md (owner-recorded) |
| Copied-code provenance rule | Owner principle: small flat dependency tree, copy with provenance over a new dep | → docs/decisions/0007-copied-code-provenance.md (owner-recorded principle) |
| Evidence-tier policy (E4 triage only) | Emulator agreement is not correctness | → docs/decisions/0008-evidence-tier-policy.md (proposed-default) |
| GSD config: existing `.planning/config.json` preserved; `granularity: standard` added because it was unset (consistent with `nyquist_validation: true`) | Routine, reversible | ✓ Applied at init |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state
5. Refresh only the sibling/LibFuturo evidence that could change the live decision

---
*Last updated: 2026-11-10 after Phase 2 (deterministic machine shell, HuCard boundary and fail-closed gate; E0 evidence only)*
