# DuoSpool

DuoSpool is an emulator project for the NEC / Hudson Soft PC Engine, sold in North America as the TurboGrafx-16. Its
support claims are tied to named hardware and content profiles and to recorded evidence. It makes no accuracy or
compatibility claim today.

DuoSpool is a working name and is not trademark-cleared.

## Status

Phases 1 to 3 are complete locally. The core is a deterministic machine shell that accepts or refuses plain HuCard bytes under a named
profile, and it executes the authored fixture suite through a bounded run measured in master clocks. Every outcome comes from a
fresh, headless, repeatable run, and a matching hash proves repeatability only.

Phase 4 (raw video from the VDC and the VCE) is complete locally for its background, interrupt, DMA, palette and dot-clock scope. Sprites
were split off at the cut line into Phase 4.1, described in the next paragraph. DuoSpool now executes the authored VDC/VCE diagnostic fixtures and emits a per-scanline raw video record (9-bit values and indices,
dot-clock divisor, active window, line start in master clocks, frame line count) before any colour conversion or scaling. A hash of that record is repeatability evidence (E0) and never a verdict; every expected literal in a video case
comes from a manual page, from an in-guest computation or from a named ledger entry, and each literal group is turned red by a fault
sweep, a mutation sweep or a toy control.

Phase 4.1 (sprites) is complete locally for the authored sprite fixtures. DuoSpool draws sprites from the sprite attribute table into the
same per-scanline raw record: both widths, three heights, flips, the palette bank, priority against the background and against other
sprites, clipping, a per-line limit of 16 sprites (the slot count follows the display width), and the overflow and collision flags, which
are written into the record and, through the status register, into guest RAM. Every sprite check is E0 repeatability and no sprite entry is
supported: the sprite statements are labelled assumptions in LED-VID-0011 (decode, addressing and slots, level R2) and LED-VID-0012
(overflow and collision, level R1), each with its page disagreement recorded. Rows taken from manual pages (origin, index, plane offsets,
priority, the 16 and 18 sprite lines) are tagged as such in the fixtures; the leftmost-pixel bit order, flips, the CGY field value 2, the
17th sprite, the two-slot count, the narrow-display counts and every collision row are assumptions. Withheld for sprites: overflow timing,
collision timing, collision of transparent pixels, the sprite width modes (SM and VM other than 0) and sprite pattern addressing for CGY 2.

Supported entries (evidence tier E1, documentary): LED-CPU-0001, LED-CPU-0002, LED-CPU-0005, LED-CPU-0014, LED-CPU-0016,
LED-IRQ-0002, LED-IRQ-0004 (part a only), LED-VID-0006, LED-VID-0008, LED-VID-0009, LED-VID-0014, LED-VID-0016 and
LED-VID-0017. Supported here means the model equals the cited manual page: a documentary comparison, not a hardware measurement.
Each Phase 4 entry is scoped by its statement to the page-stated part only: LED-VID-0008 covers the RR, DV and VD flags, their
enables and the clear on read (the OR and CR flags are raised by the sprite model, a labelled assumption outside the E1 statement), LED-VID-0009 the next-line statements for BXR and
BYR, LED-VID-0014 the trigger, the length and the DCR bits of the VRAM copy (the cancel at a display start is a modelling reading), and
LED-VID-0017 exactly the control register contents 00h and 01h.

Guest-recorded interrupt positions agree with the model's position for the profile's line count (E2 self-consistency); the position
is a labelled modelling assumption.

The block-transfer cost entry, LED-CPU-0007, stays assumed. Every other claim is a labelled modelling assumption in
`ledger/entries`, and the gate makes no second-assembler claim (the optional WLA-DX cross-check is a local one-off recorded in
`docs/ci-timings.md`).

Withheld claim words: "accurate", "cycle-accurate", "interrupt-accurate", "pad-accurate", "hardware-verified", "compatible", "supports TG-16/PC Engine", "boots", "passes the CPU tests". None of them is used here without an evidence class, and the gate rejects each one anywhere else in this file.

Withheld claim words: "pixel-accurate", "raster-accurate", "scanline-accurate", "dot-accurate", "sprite-accurate", "scroll-accurate", "colour-accurate", "timing-accurate", "DMA-accurate", "VRAM-timing-accurate", "renders correctly", "displays correctly", "plays games", "video-compatible". None of them is used here without an evidence class.

## Local gate

```
sh scripts/check
```

This is the one documented local command. It runs every gate stage and ends with `CHECK RESULT PASS` or
`CHECK RESULT FAIL`; run it before every commit.

CI runs the same command: sh scripts/check.

The gate prints one line per stage (`CHECK <stage> PASS|FAIL|ERROR`) and writes the same verdicts to `build/gate/results.tsv`.

Flags (usage: `sh scripts/check [--selftest [NAME] | --stage NAME] [--timing]`):

- `--selftest [NAME]` mutates a temporary copy of `scripts/` and proves that a removed rule turns the gate red; with `NAME`
  it runs only the mutation printed as `SELFTEST NAME`.
- `--stage NAME` runs only the stage `NAME` and exits with that stage's verdict.
- `--timing` prints the seconds each stage took, and a warning for a stage over 20 s. Time is a record, never a failure.

Exit codes: 0 every stage passed, 1 a stage failed or a control was broken, 2 a harness error (an unknown stage or
mutation name, a dead checker, a missing tool).

Tool prerequisites: a POSIX `sh`, cmake, ninja, a C11 compiler reachable as `cc` (clang or gcc), `nm`, perl, iconv, awk,
git, a sha256 tool (`sha256sum` or `shasum`), and the cc65 assembler tools `ca65` and `ld65` (`brew install cc65` locally,
`apt-get install cc65` in CI; they build the authored fixtures, which are never committed as binaries). The stage `preflight` reports a missing one by name. The `sanitizers` stage
also links the compiler's AddressSanitizer and UndefinedBehaviorSanitizer runtimes: gcc ships them with its libraries, while
Ubuntu's clang needs the separate package `libclang-rt-18-dev` (a link error naming `libclang_rt.asan` means it is missing).

## Where things are

- `docs/decisions/` holds the decision records and their index.
- `ledger/` holds the claim ledger and its schema.
- `PROVENANCE.md` is the register of any code copied from another project.

## Licensing

Licensed under either of Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE)) or MIT license
([LICENSE-MIT](LICENSE-MIT)) at your option.

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in DuoSpool by you, as
defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions.

Authored fixture directories are 0BSD ([LICENSE-0BSD](LICENSE-0BSD)), declared per directory.
Fixture manifests under `fixtures/manifest/` declare 0BSD per entry; no ROM-like binary is committed, and each later fixture directory declares 0BSD in its own directory.

## Rights

No commercial ROMs, firmware, system cards, disc images or private saves are accepted (decision record 0005).
