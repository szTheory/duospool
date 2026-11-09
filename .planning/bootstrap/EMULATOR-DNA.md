# Emulator engineering DNA for DuoSpool

Prepared 2026-11-09 as self-contained input to DuoSpool's GSD kickoff. It
combines transferable development lessons from multiple in-progress emulator
projects and LibFuturo with PC Engine/TG-16-specific research leads. It is not a
PC Engine specification, settled architecture, acceptance checklist, or
implementation result. None of the sibling project builds, tests, CI runs, or
hardware were run during this review.

## Transferable engineering lessons

### Core boundaries and API evolution

- Keep machine state explicit and per instance. Define content ownership, input
  sampling, execution bounds, output-buffer lifetime, reset/unload, error
  behavior, and concurrency before promising them in a public API. A
  context-shaped interface does not itself prove reentrancy or independence.
- Keep file access, host clocks, device enumeration, windows, audio/video
  presentation, and persistent-storage policy outside deterministic machine
  behavior. Build a headless consumer early so a frontend is not required to
  reproduce a core failure.
- Test API/ABI, behavior revision, state format, save/media format, content
  identity, and hardware profile as separate compatibility dimensions only as
  required. Opaque instances, size-tagged structs, caller-owned output buffers,
  explicit failure effects, and append-only evolution are useful patterns to
  evaluate; do not copy another project's field layouts, status numbers, or
  version policy.
- Avoid freezing a broad generic bus or speculative API before an actual
  consumer demonstrates the need. Small vertical proof can expose integration
  friction faster than a large unvalidated interface.
- Test the installed package through an independent consumer with source/build
  paths unavailable before making package or integration claims. A successful
  source-tree build proves less than the shipped boundary.

### Accuracy, workload profiling and performance

- Name hardware profiles and quality tiers precisely. Separate source/spec
  expectations, vector results, differential findings, deterministic
  repeatability, game behavior, and physical-hardware comparisons. Emulator
  agreement finds disagreements but cannot decide which implementation is right.
- Use independent oracles where possible: documented vectors, hardware
  measurements with setup and uncertainty recorded, original diagnostics with
  a separate verdict, or a truly independent implementation. Don't treat a
  game list, boot, title screen, test-card screenshot, or hash as a blanket
  accuracy claim.
- Profile a named workload and configuration before optimizing. Preserve exact
  workload identity, representative input, build, source-attributed profile
  data, warm-up, measurements, uncertainty, and output correctness. Track
  emulated throughput, end-to-end latency, resource use and CI wall time
  separately.
- Stop performance proposals when the evidence cannot attribute cost to the
  suspected code. More samples of the wrong or noisy metric do not improve
  the decision.

### Deterministic testing and fixture rights

- Start with small, authored diagnostic ROMs and literal expected results;
  keep fixture generation separate from the verdict. A fixture manifest should
  record source, build recipe, hashes, license, notices, use/redistribution
  terms, and exact test meaning.
- Use deterministic input recordings, fresh machine/save state, and an exact
  test inventory. Include negative controls proving that input is consumed and
  the harness rejects impossible or deliberately broken expectations.
- Record machine progress/time, input effects, raw framebuffer, audio events or
  samples, and host presentation separately. Tie output digests to semantic
  assertions. Tests that share the same core can prove route consistency and
  repeatability without proving hardware correctness.
- Exercise refusal and lifecycle paths: malformed or oversized media,
  unsupported profiles, null/short output buffers, capacity/work limits, reset,
  unload, multiple instances, failed media/state load, and no-mutation or
  continuation behavior where promised.
- Keep guest failure, timeout, harness failure, skip, cancellation, and missing
  artifact as distinct outcomes. Make required test inventories fail closed;
  calibrate the gate with a named negative control.

### CI, development velocity and releases

- Start with one documented local command that matches the fast PR gate.
  Allocate each CI job to a distinct failure class. Stage slow suites, broad
  matrices, fuzzing, and performance trends when they do not add useful PR-time
  feedback. Do not import mature-project machinery wholesale into a new repo.
- Measure CI setup/fetch/compile/test critical-path costs before changing
  concurrency or adding caches. Avoid duplicate builds and repeated generation
  when a measured reduction preserves required evidence and clean invalidation.
- Keep one clear project-owned rolling roadmap. At milestone discussion,
  re-read the live project state and only the sibling/API evidence that could
  change the current choice. Fan out on the decision across relevant system,
  CPU/bus, graphics/audio, API, security, performance, testing and DevOps
  specialties; compare viable alternatives and failure modes; use primary
  sources when they could change the result; challenge the recommendation and
  stop at diminishing returns. Recommend one coherent path; avoid repeated
  consensus rounds without new evidence.
- Close milestones with main green, PR triage, refreshed roadmap/lessons, and a
  release target. Publish only the exact tested artifact when its rights,
  provenance, qualification, package contents and authority are ready. GSD
  milestone numbers and software versions are separate.
- Automate verification when the result is useful. Reserve human checks for
  subjective judgment, physical/external evidence, and decisions tools cannot
  make. Autonomous progress does not silently accept a material risk, rights
  issue, or external action.

## PC Engine / TurboGrafx-16 research leads

These are dated entry points for GSD research, not a closed source survey.
Read the relevant sections directly and check licenses, scope and current
revisions before relying on them.

| Source | Useful entry point | Limits and cautions |
|---|---|---|
| [MAME PC Engine/TurboGrafx-16 driver](https://github.com/mamedev/mame/blob/master/src/mame/nec/pce.cpp) | Separates PC Engine, TurboGrafx-16 and SuperGrafx configurations and shows current device wiring. The source labels sound imperfect and lists unfinished rendering, expansion-port, scanline-mode and interrupt items. | Active emulator code, not a hardware specification or accuracy oracle. Its TODO list is evidence that this implementation still has gaps, not a complete issue list for DuoSpool. |
| [MiSTer TurboGrafx-16 / SuperGrafx technical notes](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/blob/master/docs/sgxtech.txt) | A detailed reverse-engineering lead on SuperGrafx's second HuC6270 VDC, HuC6202 VPC mixing, interrupts and edge cases. | FPGA project documentation; some statements are explicitly uncertain. SuperGrafx support is a separate scope/architecture decision, not implied by PC Engine/TG-16. |
| [TurboSharp PC Engine technical notes](https://github.com/asterick/TurboSharp/blob/master/Text/pcetech.txt) | CPU timing, bus/memory mapping and hardware notes; the document says its content includes console-tested measurements. | The author warns that methods or interpretations may be wrong. Treat as hypotheses to triangulate with manuals, other implementations, and tests. |
| [PCE_Soundtest](https://github.com/pce-devel/PCE_Soundtest) | Small MIT-licensed diagnostic targeting reported sound differences between HuC6280 and HuC6280A revisions. | The repository reports an analog capture/observational test. Check the exact hardware setup and expected waveform before admitting a derived regression fixture; it is not a complete PSG oracle. |
| [240p Test Suite PCE source](https://github.com/ArtemioUrbina/240pTestSuite/blob/master/240psuite/PCE/240pSuite.c) | Existing PC Engine/TG-16 display-oriented test program with patterns and mode checks; code has conditional CD-ROM and SuperGrafx paths. | GPL-2.0-or-later according to this source file. Its display calibration intent is not a comprehensive CPU/video conformance suite. |

GSD should look for primary HuC6280/HuC6270/HuC6260 manuals, test programs with
explicit fixture rights, and hardware measurements whose setup and uncertainty
are documented. Community-compiled reference pages are useful indices, not
automatically authoritative.

### Domain-specific questions to resolve early

The PC Engine family can widen significantly beyond base HuCard play. GSD
should establish product boundaries, not infer them from an emulator's feature
list:

1. Which exact base profile is first: region/console variant, CPU revision,
   video timing, controller signaling and content format?
2. Does the North Star include only the base PC Engine/TG-16, or also
   SuperGrafx? The latter adds another video controller and a priority/mixing
   controller.
3. Are CD-ROM² and Super CD-ROM² in the intended product, now or later? That
   adds system-card firmware, disc/container/track identity, CD audio, ADPCM,
   RAM, loading latency, and rights boundaries. Arcade Card and persistent
   memory add further cases.
4. At what temporal boundary are inputs sampled? Is frame-stepping sufficient
   for promised raster/timing behavior, or do tests/game behavior require
   sub-frame execution and timestamped input?
5. How does the core expose color output, geometry, variable timing and audio
   cadence without hiding PC Engine behavior behind host conversion?
6. What are the oracle tiers for CPU vectors, device/register behavior, raster
   effects, PSG/variant behavior, controller protocols, CD audio/timing, and
   selected game progress?
7. Which independent consumer proves the installed core's behavior? Which
   stages can be deterministic and headless, and which require analog
   display/audio capture or real hardware?
8. Which sources and test ROMs can be redistributed? A public repository,
   public CI log, or checksum does not establish legal permission.

An early HuCard profile is a plausible way to bound a first proof; it is a
recommendation for sequencing, not an owner-selected scope or a permanent
ceiling. CD or SuperGrafx support should earn roadmap placement through a named
user outcome and enough source/test evidence to make it testable.

## LibFuturo integration assessment

The reviewed LibFuturo API is experimental, in-process, and has no PC Engine /
TG-16 profile, adapter or test. Treat it as an option to test, not as DuoSpool's
contract. Its current C surface includes opaque instances, content plus a
configuration ID, a 32-bit logical-button mask, bounded frame/tick advancement,
caller-owned video and audio buffers, profile/capability queries, and optional
profile-defined persistent media. It has a defined advance-start input sample
point, a single content span per load, gray8/RGB24/index8 video, and
native-endian interleaved S16 audio.

For a real core/consumer proof, check:

- whether input sampling and bounded work preserve the chosen profile's timing;
- whether player counts, pad/turbo/multitap controls fit an honest input profile;
- whether one content span represents media plus any selected firmware/system
  card;
- whether CD tracks and multiple persistent stores need a richer model;
- whether output geometry, palette/color, variable timing and audio cadence fit
  the contract;
- whether refused loads/capacity failures preserve the previous machine state
  and buffers;
- whether the host handles persistence and external device ownership without
  being mislabeled as a core guarantee.

A frame-level wrapper may be sufficient for a first limited profile, but
confirm that with a real consumer and timing/input tests. If there is a mismatch,
describe it with a reproducible case before proposing an interface change. A
successful adapter build alone does not prove hardware accuracy, game
compatibility, browser support, or package qualification.
