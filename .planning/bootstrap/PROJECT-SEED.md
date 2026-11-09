# Project idea: DuoSpool

## What to build

Start DuoSpool, a high-quality emulator project for the NEC / Hudson Soft
PC Engine, known in North America as the TurboGrafx-16. Build an emulator whose
supported hardware and behavior can be understood, reproduced, tested, and
integrated into useful hosts. The owner wants this project to start with
lessons from existing console emulators and LibFuturo, then improve through
focused cross-project reviews at later milestone discussions.

This seed records direction and recommendations. It is not a final feature
matrix, hardware model, language/build choice, public API/ABI, license, or
compatibility promise. Use GSD discovery and reputable, preferably primary,
PC Engine sources to turn it into project-owned requirements and a rolling
roadmap. Label facts, hypotheses, proposed defaults, and owner decisions.

## North Star

- Emulate PC Engine / TurboGrafx-16 behavior accurately for explicitly named
  hardware and content profiles. Tie support claims to evidence instead of an
  unqualified “works” or “accurate” label.
- Keep execution reproducible and observable. Test CPU/device effects, logical
  input, machine time, raw video, raw audio, guest progress, and host behavior
  as separate but connected observations.
- Aim for strong runtime performance without weakening behavior. Benchmark
  identified workloads and optimize measured bottlenecks with source-attributed
  profiles and correct output held constant.
- Keep the emulation core portable and independent of host UI, wall-clock
  scheduling, physical controller discovery, filesystems, and audio/video
  devices.
- Make build, test, debug, package, and consumer feedback fast and repeatable;
  keep dependencies few and well justified.
- Expand verified coverage over time. A first supported profile is a delivery
  sequence, not a permanent limit on the project's ambition.

## Recommended discovery and delivery shape

Evaluate a focused first vertical slice around a named base PC Engine/TG-16
profile and HuCard content, with an original diagnostic fixture, meaningful
input-driven behavior, a headless runner or equivalent consumer, and an
installed-consumer/package proof. Pair machine-level vectors with a small,
rights-cleared homebrew scenario when useful. Keep the first slice broad enough
to exercise real input, timing, video/audio and lifecycle seams, but small
enough to diagnose failures quickly. This is a recommendation, not a selected
region, hardware revision, game, API, or acceptance threshold.

The GSD workflow should explicitly decide how the product relates to these
adjacent system capabilities before turning them into requirements:

- PC Engine and TurboGrafx-16 regional/hardware distinctions, including input
  protocol and content behavior.
- The HuCard path and any bank-switching or special-cartridge hardware.
- CD-ROM² and Super CD-ROM² add-ons, system-card firmware, disc tracks,
  CD-DA/ADPCM audio, and persistent memory.
- SuperGrafx hardware, including its additional video devices and mixing.
- Arcade Card, multitap and other peripherals, if relevant.

These capabilities can change machine architecture, timing, media, rights,
testing, and product cost. Do not silently include or exclude them based on
another emulator's current maturity. Scope is a GSD discovery decision.

## Core, host and LibFuturo

Keep deterministic machine behavior in the core. Let hosts own physical input
mapping, file selection, wall-clock pacing, audio/video devices, UI, and
persistent-storage policy. If multiple consumers are added, prefer one
canonical machine implementation with adapters and independent seam tests.

LibFuturo is an integration candidate, not an accepted dependency or a frozen
contract. Its current experimental C interface offers instance lifecycle,
content loading, logical-button input, bounded execution, software video/audio
buffers, capability/profile queries, and optional persistent-media transfer.
No PC Engine/TG-16 profile or adapter was found in the reviewed contract.
Before adopting it, prove a real DuoSpool core through a public consumer and
check PC Engine timing/input needs, controllers, content and firmware
representation, output format/cadence, audio, saves, failure behavior, and
execution granularity. Its current frame/advance-start-input shape, 32-bit
button mask, single content span, supported pixel/audio formats, and in-process
trust model may not fit all profile promises. Prototype a small adapter or name
the concrete mismatch before proposing a contract change.

## Development and verification intent

- Establish exact source and fixture inventories. Prefer authored diagnostic
  ROMs for the dependable fast suite. Record how each fixture was made, byte
  identity, license, notices, permitted use, and redistribution rights. Do not
  include firmware, system cards, or commercial ROMs without explicit rights.
- Make missing, skipped, cancelled, or evidence-free required checks visible.
  Include malformed/oversized content, boundaries, no-input/wrong-input
  controls, unsupported-profile refusals, lifecycle and state-preservation
  cases as those interfaces appear.
- Use deterministic input sequences with explicit timing and fresh isolated
  machine state. Assert semantic results (guest progress, register/memory
  effects, diagnostic output, or an independent hardware oracle) and preserve
  raw video/audio outputs as regression observations. Hash agreement within a
  shared implementation is not an independent correctness oracle.
- Keep raw emulated video before host scaling/color conversion separate from
  host presentation. Keep raw PSG/CD audio events and PCM separate from device
  playback and perceived quality. A nonzero audio buffer is not by itself an
  adequate signal check.
- Test the public API and the artifact a consumer installs. Add frontend,
  browser, or physical-device recurring CI checks only when that target is
  promised and the check catches a distinct recurring fault.
- Keep a fast local/PR path and stage expensive suite breadth, fuzzing and
  performance tracking according to measured value. Compare CI critical-path,
  queue/setup, emulator throughput, latency, memory and power as separate
  metrics. Add caches or parallel jobs only after a clean comparison supports
  them.
- At each CI run, inspect measured low-risk speed/cost opportunities while
  preserving evidence. At milestone close, keep main green, triage PRs, update
  the rolling roadmap and lessons, and aim to release the exact tested artifact
  whenever rights, qualification and publication authority are ready.

## Owner-selected facts and open choices

Owner-selected: the project name is DuoSpool; the target is a PC Engine /
TurboGrafx-16 emulator; use a source-grounded sibling/libfuturo review at
startup and milestone discussions; automate useful verification; favor small
dependencies; keep CI efficient without reducing evidence; close milestones
cleanly and target frequent releases when ready.

Not selected: product/repository visibility and license; language/build
system; first region, console revision and supported media; whether SuperGrafx,
CD-ROM²/Super CD-ROM², Arcade Card and peripherals are in the supported
product; CPU/video/audio timing fidelity targets; first homebrew/game and
fixture rights; host/platform; core API/ABI and evolution policy; LibFuturo
adoption; save-state/replay/debugger scope; performance budget; package format;
release policy details.

Use reversible defaults for routine implementation details. Ask the owner only
when an answer materially changes product direction, authority, legal rights,
risk, cost, or an irreversible action. Keep licensing, public release, game and
firmware rights, physical hardware qualification, and external publication
explicit. Do not copy a sibling project's license or adopt LibFuturo's
provisional license as DuoSpool's choice.
