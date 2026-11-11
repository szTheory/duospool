# Phase 3: HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad - Research

**Researched:** 2026-11-10
**Domain:** C11 freestanding HuC6280 core (bounded run, bus/MMU, timer, interrupts, pad, snapshot), ca65/ld65 authored fixtures, gate and ledger extensions
**Confidence:** HIGH for tool facts and manual facts read from page images this session; MEDIUM for the architecture patterns, which are proposals [P]

Provenance tags (project vocabulary): [SF] source-backed fact with URL; [CO] observed in this session (command, file or page image); [HR] historical or community report; [P] proposal; [U] unknown.
Mapping to the agent vocabulary: [SF] is CITED or VERIFIED, [CO] is VERIFIED by observation, [P] and [U] are ASSUMED and are collected in the Assumptions Log.

<user_constraints>
## User Constraints (from 03-CONTEXT.md)

The discuss step is done. Everything below is locked and binding exactly as written in `.planning/phases/03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad/03-CONTEXT.md` (lines 46-273 hold the decision text). This file lists ids and titles only, to stay short; read the CONTEXT text before planning.

### Locked Decisions
- Evidence rules ER-1 image-verified cites only; ER-2 default `assumed` (eight E1 candidates: LED-CPU-0001, -0002, -0005, -0007, -0014, -0016, LED-IRQ-0002, -0004); ER-3 timer evidence is circular; ER-4 assertion classes MAN/GUEST/ASSUMED; ER-5 pad and stimulus are self-consistency; ER-6 withheld
  claim words; ER-7 other watch points; ER-8 disagreements carried into ledger; ER-9 rights (never commit manual pages or text).
- A. D-90 unit is u64 master clocks (`mclk`); D-91 migration (placeholder removed, epoch and serial 1 to 2); D-92 instruction-atomic steps and in-instruction access offsets; D-93 `ds_run(inst, &request, &result)` with absolute `until_mclk`; D-94 overshoot bound derived from the table; D-95 outcomes.
- B. D-96 256-row `cpu_ops.def`; D-97 independent opcode TSV oracle; D-98 cycle rules; D-99 edge assumptions; D-100 block transfer; D-101 speed switching.
- C. D-102 per-access translate through `mpr[8]`; D-103 memory map; D-104 unmapped and stubs; D-105 TAM/TMA; D-106 ST0-ST2 bypass (cite H8-8 table 2-2-3); D-107 VDC/VCE +1 cycle; D-108 access stamps.
- D. D-109 closed-form timer; D-110 interrupt controller; D-111 sampling in the last bus cycle, entry in the same step, `irq_i_sample`; D-112 `irq_stim`.
- E. D-113 `model_params.def` (two rows); D-114 power-up and `seeded-prng-v2`.
- F. D-115 pad protocol at $1000; D-116 port model; D-117 input script and `scripts/input-check`; D-118 pad evidence.
- G. D-119 sinks `insn` and `dev`; D-120 trace rights; D-121 snapshot = digest stream; D-122 restore evidence.
- H. D-123 build fixtures in the gate; D-124 DSV1 verdict block; D-125 expected-value classes; D-126 mutation control; D-127 granularity and inventory.
- I. D-128 ca65/ld65 V2.19; D-129 encoding oracle; D-130 tool provenance. J. D-131 stages; D-132 time; D-133 rights. K. 32 new ledger ids. L. D-134 no profile r2.
- The 8 waves W1..W8, the Phase 2 amendment list (D-68, D-75, D-53, D-52, D-50, D-57, D-79, D-58, D-59, D-60, D-61, D-62, D-85, D-55) and the unchanged list (D-42, D-44, D-45..D-49, D-51, D-54, D-64, D-77, D-78, D-83, D-81).

### Claude's Discretion
- File and symbol names inside the layout; exact record field order beyond what is named; region tag names; wording of ledger statements beyond those listed; how the 28-32 plans are cut, within D-81 and the wave order; generation grouping of `.def` tables and per-fixture files (groups under 10 per task).

### Deferred Ideas (OUT OF SCOPE)
- SingleStepTests adapters and `scripts/vector-fetch`; WLA-DX as a required gate stage; SINK_FULL, drop-oldest and STEP_LIMIT; per-byte block trace records; the block-read-of-I/O rule.
- Model parameters beyond `irq_i_sample` and `tma_zero`; batching or fast paths for block transfers; cached bank pointers; late input push, per-frame feeding, multitap, 6-button, mouse, turbo, `input_topology`, `last_write_t`.
- Snapshot awk layout lint, staged long mutation set, fuzz targets for restore and the TSV reader, a real peek API and LibFuturo adapter; hardware captures and promotion beyond the eight E1 candidates; a profile r2.
- Public `duospool.h`, install rules, installed-consumer proof; branch ruleset on main.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CPU-01 | HuC6280 instruction set with flag and register effects verified by authored fixtures | Opcode map H8-25 and SW per-instruction pages image-read; ca65 encodings probed (section Code Examples); pitfalls on T flag, decimal, TST, BBR, TSB |
| CPU-02 | MPR0-7 to 21-bit physical map, defined unmapped-read value | H8-7, H8-8, H8-9, H8-10, H8-11, H8-24 image-verified; bank-class oracle pattern; $80-$F7 and RAM x4 remain [HR] |
| CPU-03 | Timer and interrupt controller verified by guest fixtures | H8-12..H8-16, H8-20..H8-22, H8-24 image-verified; closed-form timer formulas with settle-at-access; H8-14 wording conflict recorded |
| CPU-04 | Speed switching on the master-clock timeline, VDC/VCE penalties | H8-18 low speed at reset and H8-25 CSL/CSH 3 cycles image-verified; no manual text for switching or penalty (ASSUMED class) |
| CPU-05 | Resumable block transfers at byte granularity | H8-6, S8-88, S8-89 image-verified (17+6x, length 0 = 65,536, push/pop of A, X, Y); BLKT state design |
| CPU-06 | Cycle claims cite a source or are labelled assumptions | `cites.txt` img/ocr seed table from this research; ledger schema rules; image-verified page table |
| INP-01 | 2-button pad through SEL/CLR reads | CD BIOS p. 28 image-verified; H8-18 and H8-19 support port O reset high and the $1000 window; wire order stays [HR] |
| INP-02 | Per-port input script in machine time | `toml-subset.awk` constraints (table names, 400-char lines); `input-check` and flatten pattern |
| INP-03 | No-input and wrong-input negative controls plus pad-read log | Three scripts from one ROM; `dev` sink padlog; recommendation to also store raw reads in DSV1 observed bytes |
| INP-04 | Input modelled per port | `IOPT` region with `slot_type[5]`; empty-port refusal tests |
| CORE-04 | Bounded run with stop reason, overshoot, resume identity | Absolute-deadline step/stop model; chunk-invariance argument; `DS_MAX_STEP_MCLK` by table scan |
| CORE-05 | Snapshot and restore equivalence | Snapshot = digest stream; validate-then-commit; tiling assert; hostile-input threat classes |
| CORE-06 | Trace and device-write log without behaviour change | Sink design with output-only header; observer matrix; liveness toy |
| FIX-01 | Authored fixtures, licence-checked external assembler, cross-assembler check where practical | Recipe confirmed: Homebrew bottle, apt 2.19-1 and source build give identical bytes; preflight fails closed; WLA optional |
| FIX-03 | Verdicts from documented behaviour or in-guest computation | DSV1 design, literal table for `mutate_sweep`, lint class rules, ca65 syntax constraints on explicit sizing |
</phase_requirements>

## Summary

The locked design is implementable with no new product dependency. This session verified the tool story on this Mac and in an Ubuntu 24.04 container: the Homebrew bottle (cc65 2.19, arm64_tahoe), Ubuntu apt `cc65` 2.19-1 and a source build of the V2.19 tag all assemble and link the same one-source
256K prototype image to the same SHA-256 (`70801a2f...1c`), and all print `V2.18` as their version [CO]. The local Mac has no ca65, so W1 must add `ca65` and `ld65` to the preflight list and document `brew install cc65`; CI needs `sudo apt-get install -y cc65` (6 s update plus 3 s install in a
container here [CO]).

Manual verification (ER-1) was done from page images, not OCR, for 37 pages (23 hardware, 13 software, 1 BIOS; listed in the verification section). Every fact the locked decisions rest on was confirmed, with five corrections the planner must carry: the ST0-ST2 page is H8-8 (not H8-3); the mirrored hardware manual has no H8-26 (28
pages, ends at H8-25); the H8-25 footnote itself reads "17+16z", which is a printed typo, not an OCR error, so the block-transfer cycle cite must be H8-6 plus S8-89; CSL and CSH appear only as map entries on H8-25 (no software-manual page); and H8-14 words the TIQ latch conditionally while H8-15 and
H8-21 say a borrow sets it. Two further findings refine the CONTEXT without contradicting it: the power-on latch $FF has a manual basis (H8-18 and H8-19 say port O resets high), and ca65 cannot take `z:`/`a:` inside parentheses or as the BBR operand, so lint rule D-125 needs the wording "explicit
size hint: `z:`, `a:` or `<`".

The main planning risks are mechanical, not conceptual: `tests/conformance/runner.c` is already 65,876 bytes and holds the kind table; the case schema has exactly ten keys enforced by `scripts/lib/inventory-rules.awk` and the runner key table; `scripts/check` (104 KB) and
`tests/conformance/REQUIRED` are touched by every wave; toy mutants need an anchor that occurs exactly once in a `src/core/*.c` file; and every new core header must be named `ds_*.h` to pass the include rule. The baseline gate was measured at 75.7 s wall, 48 CHECK lines, on this machine today [CO].

**Primary recommendation:** Execute the eight waves as locked; start W1 by making runner headroom (move the record and hash kinds out of `runner.c`), add the assembler to preflight, and build the DSV1 plus literal-table check macros before any CPU work, so every later fixture is born with a mutation sweep.

## Architectural Responsibility Map

This is a library plus a gate, so the tiers are the repository layers.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Machine time, `ds_run`, CPU, bus, timer, IRQ, pad, snapshot | Core `src/core` (freestanding C11) | - | Deterministic behaviour lives in the core (CLAUDE.md core boundary); no host clock, file or allocator |
| Printing run record, `[model_*]`, `[input]`, `[stimulus]`, `[run]` | Printer `src/text` | Gate `record-lint` | `ds_text` prints only (D-40, D-51); lint enforces shape |
| Reading case files, input scripts, fixture ROMs, sinks to files | Runner `tests/conformance` | Gate scripts | Host concerns (files, `--trace-out`) stay out of the core |
| Parsing hand-edited data (cases, input script, irq_stim) | `scripts/lib/toml-subset.awk` via `conformance-flatten` and `input-check` | - | One parser (D-51); the C runner reads TSV only |
| Fixture sources, startup, DSV1 macros, literal tables | `fixtures/src`, `fixtures/lib` (ca65, 0BSD) | `scripts/fixture-build`, `fixture-lint` | Authored, built in the gate, never committed as ROMs (D-123) |
| Independent oracles (opcode TSV, bank-class awk, assembler matrix) | `tests/oracle`, `tests/conformance/*.awk`, `scripts/xasm-check` | - | Independence from `cpu_ops.def` is the evidence mechanism (D-97, D-129) |
| Claims, tiers, withheld lists | `ledger/entries`, `ledger/REQUIRED` | `inventory-check` pins | ER-2: `assumed` by default, `supported` only for eight candidates |
| Tool identity, CI install step | `fixtures/lib/TOOLS.md`, `.github/workflows/gate.yml` | `workflow-check` | Identity by behaviour, never `--version` (D-128) |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cc65 (ca65 + ld65) | 2.19 [CO]; tag V2.19 commit 555282497c3ecf8b313d87d5973093af19c35bd5 (CONTEXT) | Fixture assembler and linker, `--cpu huc6280` | Zlib licence; complete HuC6280 table; 1 s for 12 builds (advisor measurement); identical bytes across three distributions [CO] |
| C11 compiler (gcc, clang) | Apple clang 21.0.0 local [CO]; CI gcc and clang | Core, text, runner, toys | Existing presets with `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror` |
| CMake + Ninja | CMake 4.4.3, Ninja 1.13.2 [CO] | Build, presets `gate`, `sanitize`, `gate-gcc`, `gate-clang` | Existing; `src/core/*.c` is globbed with CONFIGURE_DEPENDS, runner sources are an explicit list |
| POSIX sh + awk | BWK awk 20200816 local [CO]; gawk and mawk in CI | Gate, flatten, oracles, input-check | Existing one-parser rule (D-51) |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| WLA-DX | 10.7 Homebrew bottle contains `wla-huc6280` and `wlalink` [CO] | Optional verify-only cross-check | GPL-2.0-or-later tool, never committed; run when on PATH, print `optional-absent` otherwise (D-129) |
| poppler `pdftoppm` | 26.04.0 [CO] | Render manual pages to images for ER-1 reads | Verifier tool only, not a gate dependency; output goes to a scratch directory outside the repo |
| Docker | 29.5.2, `ubuntu:24.04` cached [CO] | One-off Linux reproduction (`scripts/linux-repro`) | Labelled local run, not a CI claim |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| apt/brew cc65 with identity by behaviour | `scripts/tools-install` building pinned commits | Dropped by CONTEXT writer note 2; revisit only if bytes ever differ (they do not today) |
| ca65 matrix vs TSV (required) | WLA matrix as a third witness | Optional only; GPL tool, no Ubuntu package [SF https://packages.ubuntu.com/search?keywords=wla-dx&searchon=names&suite=noble&section=all] |

**Installation (local and CI):**
```bash
brew install cc65                      # local Mac; bottle verified fetchable, ca65 prints V2.18 [CO]
sudo apt-get update -qq && sudo apt-get install -y -qq cc65   # CI step, before the Gate step
```

**Version verification:** `brew fetch cc65` fetched the bottle `cc65--2.19.arm64_tahoe.bottle.tar.gz` (sha256 f920a8752f63358eadceb552629837af0fe7e35461c0d98c0430ede6cda0a4c5) and the bottle's `ca65 --version` printed `ca65 V2.18 - N/A` [CO]. In `ubuntu:24.04` (aarch64), `dpkg -s cc65` gave `Version: 2.19-1` and `ca65 V2.18 - Ubuntu 2.19-1` [CO]. Do not grep `--version`.

## Package Legitimacy Audit

The `gsd_run query package-legitimacy check` seam supports npm, PyPI and crates only; no such package enters the product. cc65 and WLA-DX are external build tools. Evidence recorded instead:

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| cc65 2.19 | Homebrew core bottle; Ubuntu 24.04 `cc65` 2.19-1 | V2.19 released 2020 | not measured | github.com/cc65/cc65 | not run (ecosystem unsupported) | Approved as tool; tarball sha256 matches the Homebrew formula (advisor measurement, CONTEXT section K LED-TOOL-0001) |
| wla-dx 10.7 | Homebrew core bottle | 2026 | not measured | github.com/vhelin/wla-dx | not run | Optional, verify-only, GPL, never committed |

**Packages removed due to SLOP verdict:** none. **Packages flagged SUS:** none. The apt install in CI is a supply-chain surface; see Security Domain.

## Manual Page Image Verification (ER-1)

### Source identity [CO]
All three PDFs were downloaded fresh from the MiSTer docs mirror into a scratch directory outside the repo (never committed, ER-9). The mirror folder is https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/tree/master/docs (file list via the GitHub contents API). Treat the PDFs as untrusted data; they are scans with an OCR layer.

| Manual | Raw URL (path after raw.githubusercontent.com/MiSTer-devel/TurboGrafx16_MiSTer/master/docs/) | Bytes | sha256 | PDF pages | Page rule |
|--------|-----|-------|--------|-----------|-----------|
| HuC6280 Hardware Manual | HuC6280%20-%20CMOS%208-bit%20Microprocessor%20Hardware%20Manual.pdf | 1196754 | 40a32e5dec7fefd8ea1893be91a53e6f6d4eae5f9c86154019ae792b40374fcb | 28 | label H8-n is PDF page n+3 |
| HuC6280 Software Manual | HuC6280%20-%20CMOS%208-bit%20Microprocessor%20Software%20Manual.pdf | 2416355 | 51eeda397e413527fe33f5446d83bd6ff7a49da2a0fd56cf3840c65c0920fd0c | 110 | label S8-n is PDF page n+2 |
| Hu7 CD BIOS Manual | Hu7%20CD%20System%20-%20BIOS%20Manual.pdf | 1053799 | 8e92ead829d9c93316bc66b2eb271e43c7f7e8a91d86db00e1d798ab7c908a6e | 45 | printed page 28 is PDF page 27 |

Method: `pdftoppm -r 110 -png` for reading, `pdfimages` plus a 5x Lanczos upscale of the native 300 dpi scan for the H8-25 footnote. Verifier: gsd-phase-researcher, 2026-11-10 (record this string in ledger source detail when reusing a read; W8 should re-read pages behind any `supported` promotion).

### Facts seen on page images (img-verified, 2026-11-10)
| Cite | Fact read from the image | Backs |
|------|--------------------------|-------|
| H8-1 | 234 instructions in 89 groups; 138 nsec bus cycle; 276 nsec minimum instruction cycle; stack 256 bytes maximum; 2M physical, 64K logical | LED-CPU-0001 (234), LED-CPU-0003 (weak /3 consistency only) |
| H8-3 | Reset reads physical $001FFE and $001FFF; interrupt pushes PCH, PCL, P; BRK pushes BRK+2; S high byte always $21, stack logical $2100-$21FF. NOT the ST0-ST2 page | D-99, LED-CPU-0010 |
| H8-4 | P layout N V T B D I Z C; I set by reset, interrupt, BRK; D reset by reset, interrupt, BRK; decimal ADC/SBC take one more cycle. Prose says C is set on "borrow" for SBC, CMP, CPX, CPY (see trap below) | LED-CPU-0002 |
| H8-5 | B pushed 0 for IRQ2 interrupt and 1 for BRK; B reads 1 with PHP or PLA; T set by SET, reset in the fetch cycle of the next instruction, saved on the stack when an interrupt occurs, reads 0 with PHP or PLA, saved set to 1 only for a "special interrupt"; BIT, TRB, TSB, TST put memory bit 7 in N and bit 6 in V | LED-CPU-0009, LED-CPU-0010 |
| H8-6 | Block transfer saves A, X, Y, loads (SH,X) (DH,Y) (LH,ACC); length 0 = 65536; six bus cycles per byte; cycles 17 + 6x with x the length; A, X, Y restored; uses 3 stack bytes; Table 2-1-1: TII inc/inc, TIN inc/fixed, TIA inc/alternate, TAI alternate/inc, TDD dec/dec, "post-increment precedes post-decrement" | LED-CPU-0007, -0008 |
| H8-7 | Table 2-2-1: top three logical address bits select MPR0-MPR7; the MPR byte becomes physical A13-A20 | LED-CPU-0011 |
| H8-8 | Reset sets MPR7 to $00 and does not initialise MPR0-MPR6; TAMi/TMAi second byte has one bit per MPR (Table 2-2-2, first row misprinted "MPR 1" for 00000001); ST0, ST1, ST2 drive A20 high and A1,A0 = 00, 10, 11 (Table 2-2-3). THE ST PAGE | LED-CPU-0014, -0016 |
| H8-9 | Chip-enable map: CE7 $1FE000-$1FE3FF (VDC), CEK $1FE400-$1FE7FF (VCE), CEP $1FE800-$1FEBFF (PSG), CET $1FEC00-$1FEFFF (timer), CEIO $1FF000-$1FF3FF, CECG $1FF400-$1FF7FF, CER $1F0000-$1F7FFF; zero page and stack drawn at $1F0000 and $1F0100 when MPR1=$F8 | LED-CPU-0011 |
| H8-10, H8-11 | 64K logical, 2M physical, 256 blocks $00-$FF; example MPR1=$F8 puts $1F0000-$1F1FFF at logical $2000-$3FFF | LED-CPU-0011 |
| H8-12 | Interrupt inputs sampled in the last bus cycle of an instruction; NMI is edge-sensed (H to L), vector $FFFC/$FFFD; entry pushes PCH, PCL, P, S post-decremented, B pushed 0, I set, D reset | LED-IRQ-0004, -0006 |
| H8-13 | Priority RESET > NMI > BRK > TIMER > IRQ1 > IRQ2; vectors FFF6/7 IRQ2 and BRK, FFF8/9 IRQ1, FFFA/B timer, FFFC/D NMI, $001FFE/F reset; IRQ1 and IRQ2 are level-sensed and masked by I | LED-IRQ-0004 |
| H8-14 | Timer interrupt vector $FFFA/$FFFB; TIQ reset by a write cycle to the interrupt request register. Wording: "if a timer borrow occurs with I reset and TIQD at 0, TIQ is set" (conditional) | LED-IRQ-0003 disagreement |
| H8-15 | Request register: bit 0 IRQ2, bit 1 IRQ1, bit 2 TIQ; IRQ1 and IRQ2 read the inverted pin level; TIQ read-only, set by a timer borrow, reset by a write; reset value 0; addresses $1FF400-$1FF7FF with A1,A0 = 1,1 | LED-IRQ-0004 |
| H8-16 | Disable register: bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD, 1 = disabled, all 0 after reset; A1,A0 = 1,0 | LED-IRQ-0004 |
| H8-17 | BRK: vector $FFF6/$FFF7, B pushed 1, I set, D reset, return address BRK+2 | LED-CPU-0009 |
| H8-18 | Reset: I set, D reset, MPR7 $00, timer stopped, disable register reset, TIQ reset, low speed, port O driven H, T reset, interrupt circuit initialised | D-114, LED-CPU-0005, LED-PROF-0003 |
| H8-19 | Read cycle at $1FF000-$1FF3FF reads port K; write cycle outputs port O | LED-CPU-0011, LED-IO-0003 |
| H8-20 | 7-bit downcounter, 7-bit reload, start/stop flag, prescaler: OSC1 / 3 = PPS3, then / 1,024; reload write bits 6-0; control bit 0; counter read bits 6-0; reload and counter invalid after reset | LED-IRQ-0002 |
| H8-21 | Borrow sets TIQ and CPU is requested if TIQD=0 and I=0; counter reloads from the reload register; reload can change while counting; writing 0 stops the timer and resets the prescaler; 21.48 MHz / 3 / 1024 = 6.992 kHz; interval 143 usec to 18.3 msec; counter reads can be invalid, read twice | LED-IRQ-0002 |
| H8-22 | Fig 2-7-1 and 2-7-2: control register bit 0 start/stop, downcounter loaded when it changes 0 to 1 | LED-IRQ-0002 |
| H8-24 | Device table: VDC A1-A0 with ST0/ST1/ST2 at logical 0000/0002/0003; VCE A2-A0; PSG A3-A0; timer control A0=1 write only, reload write and counter read A0=0; port O write / port K read; $1402 disable (A1A0=10), $1403 request (A1A0=11, TIQ reset at write); RAM $1F0000-$1F7FFF via MPR1=$F8-$FB | LED-CPU-0011, LED-IRQ-0004 |
| H8-25 | The only instruction map page (gray-code grid, bytes and cycles per cell); CSL (opcode 54) and CSH (D4) show 1 byte, 3 cycles; ST0/ST1/ST2 2 bytes 4 cycles; TAMi 2 bytes 5; TMAi 2 bytes 4; BSR 2 bytes 8; BRK 1 byte 8; RTI 1 byte 7; footnote reads "17+16z" | LED-CPU-0001, -0005 |
| S8-11 | Zero page high byte "always contains the logical address 20(16)", also for ZP,X and ZP,Y (X is added to the low byte) | D-99 |
| S8-17 | ADC IMM 2 bytes 2 cycles; ZP 4; ZP,X 4; (IND) 7; (IND,X) 7; (IND),Y 7; ABS 5; ABS,X 5; ABS,Y 5; T=1 adds 3 cycles, M(x) <- M + M(x) + C; D=1 adds 1 cycle, V unaffected; flags N V 0 - - - Z C | LED-CPU-0002 |
| S8-20 | BBRi 3 bytes 6 cycles, 2 more when taken; machine code 10i+F | LED-CPU-0002 |
| S8-30 | BRK $00, 1 byte, 8 cycles; pushes PCH, PCL, P; B pushed 1; flags T=0 B=1 D=0 I=1 | LED-CPU-0002, -0009 |
| S8-70 | RTI $40, 1 byte, 7 cycles; P pulled first, then PCL, PCH | LED-CPU-0002 |
| S8-74 | SBC: A <- A - M - not-C; same cycle table as ADC; D=1 adds 1 and leaves V unaffected | LED-CPU-0002 |
| S8-80 | ST0 $03 nn, 2 bytes, 4 cycles; CE7 low, (A1,A0) = (0,0); the CE7 assertion is stated directly | LED-CPU-0016 |
| S8-88, S8-89 | TAI flowchart (Y, A, X saved; first byte post-increment, second post-decrement; length decrement with borrow) and table: $F3, 7 bytes, 17+6x cycles, T=0; X, A, Y popped at the end | LED-CPU-0007 |
| S8-90, S8-101 | TAMi $53 operand 2^i, 2 bytes, 5 cycles; TMAi $43, 2 bytes, 4 cycles | LED-CPU-0014 |
| S8-103 | TSB $04 zp 6 cycles, $0C abs 7 cycles; N from memory bit 7, V from memory bit 6, Z | LED-CPU-0010 |
| S8-104 | TST $83, $A3 (3 bytes, 7 cycles), $93, $B3 (4 bytes, 8 cycles); operand order opcode, immediate, address; N=M7, V=M6, Z | LED-CPU-0002 |
| CD BIOS p. 28 | EX_JOYSNS output byte: bit 0 ONE, 1 TWO, 2 SELECT, 3 RUN, 4 UP, 5 RIGHT, 6 DOWN, 7 LEFT, 1 = on | D-116 mask layout |

### Mismatches and corrections (planner must carry these)
1. **ST0-ST2 page.** ROADMAP "H8-3" is wrong; the page is H8-8 Table 2-2-3, with the CE7/A1/A0 statement also on S8-80. H8-8 states A20 high and the A1,A0 code; the "$1FE000|n" physical address combines it with H8-9 and H8-24. Word LED-CPU-0016 as the manual's statement ("ST0, ST1, ST2 present A1,A0
   = 00, 10, 11 to the VDC with CE7 asserted by the instruction"), not as "equals $1FE00n through translation".
2. **No H8-26.** The mirrored hardware manual has 28 PDF pages and ends at H8-25; the table of contents lists "4. Instruction Set Summary ... H8-26", but that page is absent [CO]. Use H8-25 plus SW Tables 2-3-1 to 2-3-3 (S8-7..S8-9, read from the OCR layer only: re-read the images; alphabetical list with flags and a page column) and the
   per-instruction pages for flags. Fix the cite "H8-25/26" in `cites.txt`, TSV header and ledger.
3. **"17+16z" is not an OCR error.** The scan of H8-25 itself prints "17+16z" (a misprint of 17+6x). H8-6 says "(17 + 6x), where x is the length" and S8-89 says "17+6x". Cite H8-6 and S8-88/89 for LED-CPU-0007, put the H8-25 footnote in the entry's `disagreement` field, and do not take the formula
   from H8-25. The CONTEXT evidence caveat that lists "17+16z" as an OCR artifact is therefore incorrect.
4. **CSL and CSH have no text.** The software manual has no CSL or CSH page and no occurrence of either mnemonic [CO grep of the OCR layer]; the hardware manual shows them only as map cells (1 byte, 3 cycles) and in the reset list ("low speed mode is set"). LED-CPU-0005 can promote only "1 byte, 3
   cycles in the manual's map" and "low speed at reset"; everything about switching stays LED-CPU-0006.
5. **TIQ wording.** H8-14 (1) conditions "TIQ is set" on I reset and TIQD 0; H8-15 ("set by a timer borrow") and H8-21 ("If a borrow occurs ... TIQ is set. The CPU is requested ... if TIQD is 0 and I is 0") read as unconditional. D-109's latch-always constant matches two of three pages; record H8-14 in `disagreement` of LED-IRQ-0003.
6. **Carry on subtract.** H8-4 prose says C is set "if a borrow occurs" for SBC, CMP, CPX, CPY; S8-74 gives A - M - not-C. Use the formula pages, never the prose, for flags.
7. **Printing typos not to transcribe:** H8-8 Table 2-2-2 first row says "MPR 1" for 00000001 (MPR0); S8-74 shows "BSC (ZZ),Y"; the OCR layer misreads TDD as "TOO" and TII as "Til".
8. **Port O reset high.** H8-18 and H8-19 make the power-on latch $FF (all bits H) a manual statement for the port O output; ER-5 and LED-IO-0003 currently class it as community evidence. The SEL = bit 0 and CLR = bit 1 mapping stays [HR]. Optional improvement, not a contradiction.
9. **TSB and TRB flags.** S8-103 and H8-5 say N and V come from memory bits 7 and 6; pcetech (HR) says "of result". Carry both in LED-CPU-0010; fixtures must not assert N and V of TSB/TRB beyond a class label.
10. **138 ns** (H8-1) is a rating ("as high as"); 21.48 MHz / 3 = 139.7 ns. It is a consistency hint for /3, nothing more. The /12 low-speed ratio is in no manual page read [CO].

## Architecture Patterns

### System Architecture Diagram

```
fixtures/src/NAME.s + fixtures/lib/{hucard.cfg,startup.inc,verdict.inc,check.inc}
        | scripts/fixture-build (ca65 --cpu huc6280 -D ROMBANKS, ld65 -C hucard.cfg -D ROMBANKS)
        v
build/gate/fixtures/NAME.pce (ignored)  ->  build/gate/fixtures.used (NAME SIZE SHA256) -> fixture-check vs manifest
        |                                   (preflight fails closed if ca65 or ld65 is missing)
        v
tests/conformance/cases/*.toml --conformance-flatten--> cases.tsv     input script TOML --input-check--> INPUT rows
        |                                                                    |
        v                                                                    v
 ds_runner (guest, mutate_sweep, chunk_equiv, observer, snapshot kinds)
   ds_boot(rom, profile, policy) -> reset vector via translate -> state at mclk 0
   loop: ds_run(inst, {until_mclk, stop_cb, insn_sink, dev_sink}, &result)
        step = one instruction (+ interrupt entry) | block prologue | one block byte | block epilogue
          |-- fetch/read/write -> bus(stamp) -> translate(mpr[a>>13]) -> bank_class[256]
          |        ROM bank ($00-$7F): mapper | RAM $F8-$FB x4 | $FF hardware page window switch
          |        VDC/VCE +1 cycle charge -> stub | timer | io $1000 (pad: schedule at stamp) | irq regs | PSG stub
          |-- after instruction: sample IRQ at last-cycle stamp (pure queries: timer, irq_stim, irq regs)
   stop: UNTIL | CALLBACK | REFUSED, overshoot, stop_point
   verdict: runner reads inst->ram8k[$1F00..$1FFF] (DSV1), recomputes summary -> RESULT line
   evidence: digest = sha256(snapshot stream); sinks -> padlog.tsv / --trace-out (authored fixtures only)
```

### Recommended Project Structure (files new or changed; names are discretion)
```
src/core/
  ds_core.h  ds_run.h  ds_cpu.h  ds_bus.h  ds_timer.h  ds_irq.h  ds_io.h  ds_obs.h  ds_snapshot.h   (all named ds_*.h)
  cpu_ops.def  model_params.def  (refusals.def, boot_order.def, profile_fields.def extended)
  run.c  cpu.c  bus.c  timer.c  irq.c  io.c  obs.c  snapshot.c  (advance.c removed; serial.c, poweron.c, boot.c edited)
src/text/text.c               [machine] unit, [model_*], [input], [stimulus], [run]
tests/conformance/
  runner.c (shrink first)  runner.h  guestkinds.c  sweepkinds.c  chunkkinds.c  obskinds.c  snapkinds.c  fixload.c  hashkinds.c
  cases/*.toml  REQUIRED  golden/*.record.toml  bankclass-oracle.awk
tests/oracle/huc6280-opcodes.tsv    tests/gate/<mini-roots>/    tests/controls/<toy>/RULE + EXPECT
fixtures/src/*.s *.checks   fixtures/lib/{hucard.cfg,startup.inc,verdict.inc,check.inc,variants.txt,cites.txt,TOOLS.md}
fixtures/manifest/*.toml (authored entries)
scripts/{fixture-build,fixture-lint,xasm-check,input-check} + scripts/lib/*.awk (+ edits to check, preflight, conformance-run, conformance-ingest, determinism-run, outcome-sync, workflow-check, docs-check)
```

### Pattern 1: Opcode table as data, X-macro, no string in the core
**What:** One row per opcode in `src/core/cpu_ops.def`; C expands only the columns it needs; awk reads the same file for the two-way check against the TSV, so the cite column never enters the core.
**When:** D-96. **Example [P]:**
```c
/* cpu_ops.def: DS_OP(opcode, MNEMONIC, MODE, bytes, base_cycles, access_class, "cite") */
DS_OP(0x00, BRK, IMP, 1, 8, STK, "S8-30")
DS_OP(0x53, TAM, IMM, 2, 5, MPR, "S8-90")
/* cpu.c */
static const struct ds_op ops[256] = {
#define DS_OP(op, mn, mode, bytes, cyc, cls, cite) [op] = {MN_##mn, AM_##mode, bytes, cyc, CL_##cls},
#include "cpu_ops.def"
#undef DS_OP
};
```
Dispatch with `switch (op->mn)` over an enum and no `default`, so `-Wswitch` (in `-Wall`) fails the build when a mnemonic has no handler. No computed goto. `static const` only: the boundary check rejects any writable symbol [CO scripts/core-boundary-check header: "a global, a function-local static
and an initialised table all fail, and a static const table does not"]. The compile uses `-fno-pie`, so pointer tables are legal, but avoid them anyway.
`DS_MAX_STEP_MCLK` (D-94): compute by scanning `ops[]` in a function plus the adders and the interrupt entry at low speed; the test asserts observed overshoot below it.

### Pattern 2: Step and stop model with absolute deadline
**What:** `ds_run` loops `while (machine_time < until) { if (stop && stop(inst)) break; step(); }`. Every piece of state lives in the instance; no local survives a step. Chunking cannot change the step sequence because steps are indivisible and the deadline is absolute.
**Detail:** `until <= machine_time` returns UNTIL with 0 steps (D-93). Guard `machine_time` near UINT64_MAX: a hostile snapshot can set TIME high; refuse in restore (time above 2^63) or return REFUSED from `ds_run` when `machine_time + DS_MAX_STEP_MCLK` would wrap. Intended unsigned wrap everywhere else; signed overflow is UB in the sanitize preset (no `-fwrapv`, D-83).

### Pattern 3: One bus choke point with stamps and the access-offset rule
**What:** `uint8_t ds_bus_read(inst, logical, ctx)` and `ds_bus_write` translate `phys = ((uint32_t)mpr[a >> 13] << 13) | (a & 0x1FFFu)` (widen before shifting; MPR is uint8_t and `mpr << 13` is fine in int, `<< 24` would not be), index `bank_class[256]`, then `switch`. A hardware-page access first
charges the VDC/VCE +1 cycle when `(phys & 0x1FFFFF) < 0x1FE800` and `>= 0x1FE000` (VDC and VCE windows), then stamps the access `insn_start + (offset + extra_cycles) * cycle_len`. Offsets: single data access at cycles-1 (last cycle), read-modify-write read at cycles-2 and write at cycles-1, no
dummy accesses (D-92). ST0-ST2 call `ds_bus_write_phys` with the physical address `0x1FE000 | n`, n = 0, 2, 3, never touching `mpr[]`.
**Anti-pattern:** caching translated pointers (D-102 defers it); special-casing zero page or stack (they go through MPR1 like any access, H8-3 and H8-9 figure).

### Pattern 4: Closed-form timer with settle-at-access
**What:** State `{origin, counter, reload, enabled, tiq_latch}`; step = 3072 mclk from `origin`. Pure query at stamp t when enabled: `n = (t - origin) / 3072`; if `n <= counter` value is `counter - n` with 0 borrows; else `m = n - (counter + 1)`, borrows `1 + m / (reload + 1)`, value `reload - (m %
(reload + 1))`. Guest writes (`$0C00`, `$0C01`, `$1403`) first call `timer_settle(t)`, which folds elapsed time into `(counter, origin, latch)` in canonical form (origin moves to the last step boundary, counter to the exact value, latch set if any borrow elapsed), then applies the write. Settle runs
only at guest accesses at deterministic stamps, so raw fields are identical for every chunking (D-109, X13). The `(reload + 1)` period is a derivation from the 143 usec to 18.3 msec range on H8-21, not a printed sentence [SF H8-21]. A per-clock reference model is a unit test only. Constants: hold on
stop, no $7F window (D-109).

### Pattern 5: Interrupt sample and same-step entry
**What:** After the instruction, `sample_stamp = t_end - cycle_len_of_last_cycle`; pending = timer (pure query, latch OR borrows since ack) > IRQ1 level (stimulus at stamp, not disabled) > IRQ2 level. Entry (8 cycles) pushes PCH, PCL, P with B=0 and the current T bit, sets I, clears D and T, fetches
the vector through MPR7. `irq_i_sample`: under `pre` the I value before the instruction is used after CLI, SEI, PLP, RTI; under `post` the new I is used (D-111). No latch state persists between steps.
**Pitfall:** a CSL/CSH instruction's last cycle runs at the OLD speed; compute the stamp before switching.

### Pattern 6: Block transfer as four step kinds
**What:** State `BLKT {active, opcode, src, dst, remaining(u32), alt_toggle, latched[7]}`. Prologue (14 cycles) latches the seven bytes, pushes Y, A, X to the stack through the bus, advances `pc`; each byte step (6 cycles) reads source at offset 4 and writes destination at 6, steps 16-bit addresses
with explicit masks, decrements `remaining` (0 on entry means 65,536); epilogue (3 cycles) pops X, A, Y from memory (a destination that clobbers the stack is faithful), clears `active`, then samples IRQ (D-100). Stop point after each byte is BLOCK_BYTE. S8-88 draws S decremented only twice before
the X push; since A, X, Y are popped from memory and S is restored, the net result is the same.

### Pattern 7: Sinks are output-only records in caller memory
**What:** Request holds `{ptr, capacity_records}` per sink; the core overwrites the header `{count, lost, seq}` at the start of every `ds_run` call and never reads it back, so a garbage-poisoned header cannot steer a write (the observer matrix requires identical results with a poisoned header,
D-119). Drop-newest with `lost++`. Records are written byte by byte, little-endian, from widened unsigned values. Sink overlap with instance, request or content is refused `memory_invalid` with the same `uintptr_t` range test as `ranges_overlap` in `src/core/boot.c`. The name `ds_sink_fn` already
exists in `ds_serial.h` for the digest stream; use another type name.

### Pattern 8: Snapshot = digest stream, restore = validate then commit
**What:** Extend `regions[]` and `emit_payload` in `src/core/serial.c` (tags CPU0, BLKT, BUS0, TIMR, IRQC, IOPT, SCHD, IRQS; drop PSGN and VDCP); the snapshot writer is `ds_digest_stream`; `ds_snapshot_size()` sums fixed payload lengths (schedule and stimulus regions are fixed-size, unused slots
zero). `ds_restore` validates in place from the caller buffer (prefix, version, tag order, lengths, every field domain, reserved bits, trailing bytes), then copies; the instance is over 1 MiB so no staging copy on the stack. A hand-written tiling list with `_Static_assert(sum == sizeof(struct
ds_instance))` is a separate independent check (D-122); keep explicit padding members so the assert is honest on gcc and clang.

### Pattern 9: Input schedule evaluated at read stamp
**What:** Immutable copy-in array `{at, port, mask}` (1024 x 16 B). Level of port p at stamp t is the mask of the last event with `at <= t` and that port (default 0 = nothing held). Linear or binary search both give the same value; no cursor, so chunking and restore cannot disagree. `ds_input_push` re-validates and is legal only at `machine_time == 0` with no step run (D-117).

### Pattern 10: Fixture checks with a literal table (mutation by patching)
**What:** `check.inc` macros compare an actual value with a literal stored in a ROM table at a fixed file offset; the runner patches one literal in an in-memory copy and expects exactly that check to flip to $46 (D-126). Table layout [P]: magic "DCK1", count, then fixed-size entries `{id, expected
bytes}` in a ld65 segment placed at a fixed address in bank 0; fixture-lint requires every `CHECK` to use the table. Each literal comes from the page or a GUEST model, never from the table's own mutation partner. Sweeps end early through the done-polling stop callback (speed aid only, D-95) so each
mutated run stops at done.

### Anti-Patterns to Avoid
- **Per-fixture, per-mutation assemblies** (0.05 s x ~400 checks): use the in-process literal patch.
- **A guessed overshoot constant:** derive it from the table.
- **Hidden state in the core:** no function-local statics (CB_WRITABLE), no globals, no `static` caches of the timer.
- **Using the C struct layout for the snapshot:** the stream writer is explicit little-endian field by field.
- **Reading the sink header back,** or trusting `count` from caller memory.
- **Treating `pass` as support:** ASSUMED-class checks pass in the gate (ER-4); never quote one total.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Assembling fixtures | A mini assembler or hand-typed byte arrays | ca65/ld65 (`--cpu huc6280`) | A self-written encoder shares errors with the decoder; ca65 vs the manual TSV is the encoding oracle |
| Parsing hand-edited data | A C TOML or script parser | `scripts/lib/toml-subset.awk` via `conformance-flatten` and `input-check` | One parser rule (D-51); stage 1 already enforces 400-char lines and ASCII |
| Hashing the state | A second digest | `src/core/sha256.c` and host `shasum -a 256` for independence | Snapshot sha256 equals the digest by construction |
| Opcode metadata twice | Two hand-typed tables in the core | One `cpu_ops.def` plus the independent TSV | The two-way awk check is the CPU-06 mechanism |
| Cross-assembler agreement | A custom byte differ | `cmp` on assembled matrices (`xasm-check`) | Small, fail-closed, selftest-mutatable |
| Toy mutants | Committed broken copies of core files | `scripts/control-mutate` rules (`tests/controls/NAME/RULE`) | D-85: generated from live sources; the anchor must occur exactly once |
| Randomness for programs | libc `rand` | `ds_splitmix64_next` in `src/core/poweron.c` | Deterministic and already tested (seed 0 vector in D-50) |
| Float or time anywhere in the core | `float`, `clock()` | integer mclk arithmetic | CB_FLOAT and CB_UNDEF_SYMBOL fail the gate |

**Key insight:** every evidence mechanism in this phase works only because two things are written independently (manual to TSV, ledger text to bank-class awk, assembler to table, in-guest model to core). Reusing the core's tables in a check silently collapses that independence.

## Runtime State Inventory

Included because W1 migrates the machine-time unit and removes placeholder fields.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | Two goldens in `tests/conformance/golden/` print `placeholder-cycles`, epoch 1 and digests; cases with literal digests or psg_noise: `advance_rules`, `run_budget`, `interleave_*`, `digest_*`, `poweron_*`, `nonmutation_*` | Migrate literals in the same plan from independent derivations, never from core output; epoch and serial 1 to 2 |
| Live service config | `.github/workflows/gate.yml` (no assembler install); `scripts/lib/workflow-rules.awk` (no rule about tool-install steps) | Code edit: add the apt step before `Gate`; teach `workflow-check` with a negative control |
| OS-registered state | None found: no launchd, cron or Task Scheduler entries in the repo | None |
| Secrets and env vars | None: CI has no secrets (workflow rule WF_FORBIDDEN_KEY forbids them) | None |
| Build artifacts | `build/gate`, `build/sanitize`, `build/gate-gcc`, `build/gate-clang` (ignored), stale `ds_runner` and `cases.tsv` after the schema change; Homebrew cc65 absent on this Mac | Gate stages rebuild; first W1 task documents `brew install cc65`; `.gitignore` comment about a Phase 3 `!` rule is rewritten (D-133) |

## Common Pitfalls

### Pitfall 1: Runner file headroom and the kind table
**What goes wrong:** `tests/conformance/runner.c` is 65,876 bytes (`wc -c`) and holds `kinds[]`, `run_case` and `main` [CO]; Phase 2 plans used 65,536 bytes as an acceptance limit, and no gate rule enforces it [CO: grep of `scripts/rights-check` finds only the SHAPE rule for non-text files].
**How to avoid:** first W1 task moves `build_expected_keys`, `parse_record`, `kind_record_lines` (lines about 953-1290) and the hash kinds (about 1293-1475) out to `reckinds.c` and a new `hashkinds.c`, about 20 KB. Register new `.c` files in `DS_RUNNER_SOURCES` in `CMakeLists.txt`; the toy runners reuse that list.

### Pitfall 2: Case schema has exactly ten keys
**What goes wrong:** `scripts/lib/inventory-rules.awk` rejects a case without all ten keys or with another key (`INV_CASE_FIELD`); the runner marks an unknown key `bad` (`key_def[NKEYS]`).
**How to avoid:** add the optional keys (`until_mclk`, `policies`, `checks`, `input`, `irq_stim`, `sinks`, `chunks`) in one plan that edits `runner.h` (`K_*` enum), the runner key table, `inventory-rules.awk` and its mini-roots under `tests/gate/inventory`, together. TSV type `a` is only an array of strings.

### Pitfall 3: Core header names and writable symbols
**What goes wrong:** the include rule accepts only `<stddef.h> <stdint.h> <stdbool.h>`, quoted `ds_*.h` and quoted `*.def` (`core-boundary-check` scanner) [CO]. A header named `cpu.h` fails CB_INCLUDE. A function-local static, a global or an initialised non-const table fails CB_WRITABLE. A float
token (also `1.5` or `1e3`) fails CB_FLOAT. Allowed undefined symbols are only memcpy, memmove, memset, memcmp (bzero and `__chkstk_darwin` on Darwin) [CO scripts/core-boundary.allow]: a large local array can still pull `__chkstk_darwin`, allowed on Darwin only.

### Pitfall 4: Toy anchors
**What goes wrong:** a `control-mutate` rule needs `file=src/core/X.c` (a file in `DS_CORE_SOURCES`, so not a `.def` or header) and an `anchor` that occurs exactly once, on one line [CO `tests/controls/*/RULE`]. Constants in headers (`DS_UNMAPPED_READ`) cannot be mutated.
**How to avoid:** write each of the 8 mutation targets as one distinct statement in a `.c` file (unmapped return in `bus.c`, reload compare in `timer.c`, priority chain in `irq.c`, T-flag serialisation line in `serial.c`, BLKT payload line, block progress store, ADC carry line, the hook call).
The Phase 2 anchor in `advance.c` (toy `shared-state`) vanishes and must be re-pointed; the `drop-region` anchor survives only if the RAM8 emit line in `serial.c` is kept verbatim.

### Pitfall 5: ca65 sizing syntax and the lint rule
**What goes wrong:** `lda $0020` still assembles as zero page (`A5 20`) and `lda $20` likewise [CO]; zero page on the HuC6280 is logical $2000-$20FF, so a literal below $100 means $2000+n. `z:` and `a:` are rejected inside parentheses (`lda (z:$20)`, `jmp (a:$1234)` give "Syntax error") and as the first BBR/BBS operand (`bbr0 z:$20, l0` gives "Syntax error") [CO].
**How to avoid:** lint rule "every operand below $100 carries an explicit size hint: `z:`, `a:` or `<`"; write `lda (<$20)`, `bbr0 <$20, label`, `jmp ($1234)`, `tst #$80, z:$20`. Parenthesise macro parameters (`.byte (n)`); `.byte $FF - n` with n = `i+1` failed [advisor measurement].

### Pitfall 6: TAM/TMA edge writing
**What goes wrong:** ca65 rejects `tma #3` and `tam #3` ("Argument to TAM must be a power of two") but accepts `tam #0` and `tma #0` (`53 00`, `43 00`) [CO]. D-105's `.byte` plus `; RAW:` is needed only for multi-bit operands. Range error above 255.

### Pitfall 7: T flag and SET ordering
**What goes wrong:** T is reset in the fetch cycle of the next instruction (H8-5); only ADC, AND, EOR, ORA change behaviour, memory-to-memory on `$2000+X`, +3 cycles (S8-17); `SED` between `SET` and `ADC` clears T, so BCD needs `SED` before `SET` [HR pcetech section 2.3]. There is no `CLT` mnemonic
(ca65: "':' expected") [CO]. At step start capture `t_active = P.T` then clear T; PHP and BRK push bit 5 = 0, an interrupt after SET pushes bit 5 = 1 and RTI or PLP load it (D-99, [HR]).

### Pitfall 8: Decimal mode and flag sources
**What goes wrong:** decimal ADC/SBC add 1 cycle and leave V unchanged (S8-17, S8-74). N, Z, C of a decimal result are an assumption (D-99). Subtract carry uses A - M - not-C, not the H8-4 prose. Combined T mode plus decimal adds both adders by plain reading (+4); no page states the combination, label it ASSUMED.

### Pitfall 9: BRK, IRQ and B
**What goes wrong:** B is not a stored latch: PHP and BRK push B=1, hardware interrupts push B=0 (H8-5, S8-30, H8-17), PLP ignores it. BRK pushes PC+2 and its vector shares IRQ2 at $FFF6. D is cleared at every interrupt entry and BRK (H8-4, S8-30). S wraps inside $21xx, zero page index wraps inside $20xx (S8-11).

### Pitfall 10: TST, BBR and addressing-byte order
**What goes wrong:** TST encodes opcode, immediate, then address (S8-104): `83 nn zz`, 3 bytes, 7 cycles; abs forms 4 bytes, 8 cycles. BBRi/BBSi are 3 bytes, 6 cycles, +2 when taken (S8-20); the relative target is computed from PC+3. JMP ($xxFF) reads the high byte at the next address, not the 6502
wrap [HR pcetech]. Taken branches do not pay a page-cross cycle [HR pcetech], a ledger assumption (LED-CPU-0010).

### Pitfall 11: Block transfer details
**What goes wrong:** length 0 is 65,536 (H8-6), so `remaining` must be uint32_t or a flag; 17 + 6 x 65,536 = 393,233 CPU cycles, about 4.72 million mclk at low speed [P arithmetic]. TAI and TIA alternate source or destination add then subtract, not bit-0 inversion [HR pcetech]. `$FFFF` wrap of
16-bit addresses must be explicit. A destination write into the stack area is faithful and must not be "fixed" because the epilogue pops from memory.

### Pitfall 12: Timer prescaler phase
**What goes wrong:** starting (0 to 1) loads the counter and sets `origin = now`, which resets the prescaler phase; stopping resets the prescaler too (H8-21). A reload write while running changes only the NEXT reload, not the current counter (H8-21). Re-basing after such a write needs the settle
step in Pattern 4, or the first period after the write is wrong. Counter reads return bits 6-0 and take bit 7 from `io_buffer` (D-104).

### Pitfall 13: Sampling edge cases
**What goes wrong:** sample stamp is the last cycle of the instruction, ties inclusive (D-111); a `$1403` write in that cycle takes effect first. With `irq_i_sample = pre`, CLI/PLP/RTI delay by one instruction; with `post` there is no delay; the distinguishing fixture records the handler-ran observable under both. TIQ coalesces many borrows into one bit.

### Pitfall 14: Unsigned, shifts and sanitizers
**What goes wrong:** `-fwrapv` is only on `gate`, `gate-gcc`, `gate-clang`; the `sanitize` preset has `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer` and no `-fwrapv` [CO CMakePresets.json], so signed overflow and bad shifts abort. All presets use `-std=c11 -pedantic
-Wall -Wextra -Wconversion -Wshadow -Werror` [CO]. `uint8_t` operands promote to int: `(uint8_t)(a + 1u)` casts are required by `-Wconversion`; widen before `<< 24`; use `1u << n`; relative branch offsets need explicit unsigned sign extension, not `(int8_t)` casts; V is `((a ^ r) & (m ^ r) &
0x80u)`. `-Wshadow` fires on locals named like outer variables (avoid `index`, `time`, `len` as inner names).

### Pitfall 15: Preflight control portability and optional tools
**What goes wrong:** a "PATH without ca65" control must not assume where ca65 lives (`/usr/bin` on Ubuntu). `env PATH=<empty dir> /bin/sh scripts/preflight --tools 'ca65 ld65'` prints `PREFLIGHT_MISSING ca65` and `ld65`, exit 2, because `command -v` is a builtin [CO]. Do not add `wla-huc6280` and `wlalink` to the required list (optional, D-129).

### Pitfall 16: Record lint patterns
**What goes wrong:** `record-rules.awk` rejects any value with a slash or backslash (REC_PATH), `0x` plus a hex digit (REC_POINTER) and a time-like `dd:dd` or date-like `dddd-dd-dd` (REC_TIMESTAMP) [CO header]. A stimulus printed as `3072:12:1` contains `72:12` and fails. Print decimal numbers with
commas, and add the new tables to `keys[]` and the sequence in `record-rules.awk` plus `reckinds.c` expected keys. The golden set changes (`time_unit`, new tables, `behaviour_epoch`).

### Pitfall 17: Fixture stage ordering
**What goes wrong:** `stage_fixtures` does `rm -f build/gate/fixtures.used` and then `conformance-run fixtures` rewrites it from `ds_runner --fixtures` (synthetic recipes only) and demands the used names equal the manifest stems [CO scripts/check]. Authored ROM lines from `fixture-build` would be deleted or flagged FIX_UNLISTED.
**How to avoid:** W1 makes `conformance-run fixtures` append the `fixture-build` list (or `fixture-build` writes a second file merged before `fixture-check`), registers `fixture-build` before `fixtures`, and keeps "stage inputs are produced in the same run". The sanitizers stage reads built ROMs from `build/gate/fixtures` like it already requires `build/gate/results.txt`.

### Pitfall 18: Image size and bank 0
**What goes wrong:** the boot path accepts only 256K, 384K and 1M plain images (`ds_size_classify`, D-44), so every fixture is at least 262,144 bytes; bank 0 holds code and the vector at file offset $1FFE (MPR7=$00 at reset, H8-8). With 256K, bank $20 mirrors bank 0 (LED-LOAD-0003). The 384K mirror is `b < 0x40 ? (b & 0x1F) : (0x20 + (b & 0x0F))` [CO `src/core/mapper.c`].

### Pitfall 19: DSV1 and MPR remaps
**What goes wrong:** the verdict block at work-RAM offset $1F00 is logical $3F00 only while MPR1=$F8; `mmu_mpr` remaps MPRs. Write verdicts through a dedicated slot the fixture never remaps (for example MPR2=$F9 giving $5F00). Every fixture must zero what it reads (policy runs zero, v1, v2).

### Pitfall 20: Hostile snapshot time and values
**What goes wrong:** a restored TIME near UINT64_MAX makes `machine_time + step` wrap; a field outside its domain (speed 2, timer counter 128, remaining 0 while active, 65,537) must be refused in pass 1 (D-121). Accepted inputs must re-serialise to identical bytes.

## Code Examples

### ca65 syntax verified against the Homebrew bottle (listing bytes) [CO]
| Source | Bytes |
|--------|-------|
| `tst #$80, z:$20` | 83 80 20 |
| `tst #$82, a:$2000` | 93 82 00 20 |
| `bbr0 <$20, l0` | 0F 20 rel |
| `rmb3 z:$22`, `smb4 z:$23` | 37 22, C7 23 |
| `tam #$01`, `tma #$80`, `tam #0` | 53 01, 43 80, 53 00 |
| `lda (<$20)`, `lda (<$20, x)`, `lda (<$20), y` | B2 20, A1 20, B1 20 |
| `jmp ($1234)`, `jmp ($1234, x)` | 6C 34 12, 7C 34 12 |
| `st0 #$11`, `st1`, `st2` | 03 11, 13 22, 23 33 |
| `tii/tai/tia/tdd/tin src, dst, len` | 73, F3, E3, C3, D3 then three 16-bit words |
| `sxy`, `sax`, `say`, `cla`, `clx`, `cly` | 02, 22, 42, 62, 82, C2 |
| `csl`, `csh`, `set`, `bsr`, `bra` | 54, D4, F4, 44 rr, 80 rr |
| `stz a:$2000`, `inc a`, `dec a`, `bit #$12` | 9C 00 20, 1A, 3A, 89 12 |

Build commands (flags measured; `-D` goes to both tools):
```sh
ca65 --cpu huc6280 -D ROMBANKS=32 -I fixtures/lib -o build/gate/fx/NAME.o fixtures/src/NAME.s
ld65 -C fixtures/lib/hucard.cfg -D ROMBANKS=32 -o build/gate/fixtures/NAME.pce build/gate/fx/NAME.o build/gate/fx/banktags.o
```
Config shape that worked (one source gives 256K, 384K, 1M by `ROMBANKS` 32, 48, 128) [CO from advisor prototype, re-run here with the bottle]:
```
MEMORY { ROM0: start=$E000, size=$1FF6, fill=yes, fillval=$FF, file="%O";
         VEC:  start=$FFF6, size=10,    fill=yes, fillval=$FF, file="%O";
         EXT:  start=$0000, size=(ROMBANKS - 1) * $2000, fill=yes, fillval=$FF, file="%O"; }
SEGMENTS { CODE: load=ROM0, type=ro; VECTORS: load=VEC, type=ro; TAGS: load=EXT, type=ro; }
```
The project config adds the check-literal segment at a fixed address inside ROM0 [P]. Vectors are five words at $FFF6: IRQ2/BRK, IRQ1, TIMER, NMI, RESET (H8-13). Memory areas at `start=$0000` are file layout only; HuC6280 zero page symbols must be offsets, never $20xx.

### One-off reproduction of the identity measurement [CO]
```sh
ca65 --cpu huc6280 -o t.o t.s && ca65 --cpu huc6280 -o bt.o bt.s && ld65 -C t32.cfg -o img.bin t.o bt.o
shasum -a 256 img.bin   # 70801a2f751733279bd6b7b7f604a45a9d434b6d17fc011c1d115b414bbdb41c on the bottle, apt 2.19-1 and the source build
```

### Optional WLA invocation [CO bottle, 8192-byte test image]
```sh
wla-huc6280 -o t.o t.s && wlalink -r l.lnk t.bin      # l.lnk: [objects] then t.o
```

### Assembler-matrix check shape [P]
`scripts/xasm-check` reads `tests/oracle/huc6280-opcodes.tsv`, emits one neutral-syntax line per opcode and mode with explicit size hints, assembles with ca65, extracts bytes from the listing, and compares each row's encoding bytes (opcode plus operand placeholders) to the TSV. Exit 1
`XASM_MISMATCH`; exit 2 `PREFLIGHT_MISSING`. Multi-bit TAM/TMA rows are `.byte`. Controls: empty PATH, a shim `ca65` that flips a byte, a TSV with one byte changed.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Placeholder cycles with xorshift state (Phase 2) | u64 master clocks with real CPU | Phase 3 W1 | Goldens, epoch and serial change in one plan |
| Relative budget `ds_run(budget, step)` | Absolute `until_mclk` with overshoot | Phase 3 W1 | run(N)+run(M) equals run(N+M) by construction |
| Per-clock timer tick | Closed form on mclk | Phase 3 W4 | Chunk-invariant over a 393k-cycle block transfer |
| Hash of results as a pass signal | DSV1 verdict plus literal-table mutation sweep | Phase 3 W1/W2 | A check that cannot flip is vacuous and fails the sweep |

**Deprecated or removed:** `ds_advance`, `advance.c`, `psg_noise`, `vdc_phase`, regions PSGN and VDCP, the `max_cycles`-style budget keys.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `(reload + 1)` steps per period is the right reading of the 143 usec to 18.3 msec range | Pattern 4 | Off-by-one in every timer fixture; flagged E1 only for C-level test |
| A2 | Apt `cc65` install on the GitHub ubuntu-24.04 image costs about 10-25 s and needs no extra repository | Standard Stack | CI timing; measure on first run (the container measurement was 9 s) |
| A3 | Output bytes are identical on the x86-64 CI runner (only aarch64 was measured here for apt) | Standard Stack | Hash pins in the manifest could differ on first CI; confirm after the first run |
| A4 | Per-mutation runs are cheap if fixtures stay under about 200k CPU cycles and sweeps stop at done | Pattern 10, Validation | Sweep cost in sanitize mode could reach tens of seconds; measure after W2 |
| A5 | Combined T plus decimal costs +4 cycles | Pitfall 8 | Only ASSUMED-class checks touch it |
| A6 | Toys cost about 3-6 s CPU each (9 toys over four CI invocations) | Validation, gate time | May require "one compiler only" per D-132 |
| A7 | The file names and the ids of runner cases listed in the Validation section | Validation | Cosmetic; planner owns names |
| A8 | Keeping pad raw read bytes in DSV1 observed bytes satisfies ROADMAP "a guest pad-read log is recorded" | Validation SC2 | If the owner reads SC2 as a separate log region, a $1E00 region returns (dropped by CONTEXT) |

## Open Questions (RESOLVED)

1. **Guest pad-read log wording (SC2).** (RESOLVED: CONTEXT errata R-7; plans 27 and 29 store raw $1000 reads in the DSV1 observed bytes as well as the runner pad log.)
   - Known: D-118 makes `<id>.padlog.tsv` a runner artifact from the `dev` sink; the $1E00 guest log was dropped.
   - Unclear: ROADMAP says "a guest pad-read log is recorded for all three".
   - Recommendation: have `pad_2btn` also store each raw $1000 read in DSV1 observed bytes (+80..+DF, up to 96 bytes) so a guest-side log literally exists, and cite both in the verification report.
2. **Number of REQUIRED ids.** (RESOLVED: the planned count follows the enumeration; one sweep id per fixture is kept per D-126.) CONTEXT says about 50; the enumeration below reaches about 64 with one sweep id per fixture and eight pin cases. Fold sweeps into one id per wave if review cost matters.
3. **First CI evidence.** (RESOLVED: D-64 holds; plan 33 records CI as unverified, pending owner push authorisation.) D-64 authorisation for a push is still pending; the apt step and the timing review stay "unverified-pending-push" until then.
4. **ER-1 recording.** (RESOLVED: CONTEXT errata R-8; each ledger entry records verifier, date and fact; plan 32 re-reads the pages behind the eight E1 candidates.) This research read the pages; ledger entries still need the verifier, date and fact in source detail when written; W8 should repeat the read for the eight candidates.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| ca65, ld65 | fixture-build, xasm, gate preflight | No (not installed) | bottle 2.19 fetchable [CO] | None: fail closed; `brew install cc65` |
| wla-huc6280, wlalink | optional verify-only | No | bottle 10.7 usable from a scratch extract [CO] | Print `optional-absent`; no cross-assembler claim |
| cmake, ninja | build | Yes | 4.4.3, 1.13.2 | - |
| clang (as gcc and clang), awk | gate | Yes | Apple clang 21.0.0, BWK awk 20200816 | CI has gcc, clang, gawk, mawk |
| Docker, ubuntu:24.04 | `scripts/linux-repro`, apt reproduction | Yes | 29.5.2 | Report Linux verification as unverified |
| pdftoppm, qpdf, tesseract | page-image verification | Yes (verifier machine only) | poppler 26.04.0 | Not a gate dependency |

**Missing with no fallback:** ca65 and ld65 on this Mac until `brew install cc65` (a reversible local tool install, not an authority gate).
**Missing with fallback:** WLA-DX.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Project gate: sh/awk stages in `scripts/check` plus the C conformance runner `build/gate/ds_runner` (RESULT lines judged by `conformance-ingest`); no third-party test framework |
| Config file | `CMakePresets.json`, `tests/conformance/REQUIRED`, `tests/conformance/cases/*.toml`, `tests/conformance/EXCLUDED_IN_MODE`, `fixtures/manifest/*.toml` |
| Quick run command | `sh scripts/check --stage conformance` (add `--stage fixture-build`, `fixture-lint`, `xasm`, `input-check` for the new stages) |
| Full suite command | `sh scripts/check` (single local command; CI runs it plus gcc, clang and a second awk) |
| Selftest | `sh scripts/check --selftest [NAME]`: each new checker needs a removal mutation that turns its control red |
| Baseline | 75.7 s wall, 48 CHECK lines on this Mac today; slowest `sanitizers` 10 s, `conformance` 10 s, `named-controls` 7 s, `audit-synthetic` 6 s [CO `--timing` run] |

### Phase Requirements to Test Map
Ids are proposals [P]. "G" means the case is a guest fixture judged from DSV1.

| Req ID | Check (case or stage) | Runs where | Wave | Failure signal |
|--------|----------------------|-----------|------|----------------|
| CPU-01 | `opcode_table_manual`, `cycle_adders_manual`, G `cpu_ldst`, `cpu_alu`, `cpu_flow`, `cpu_huc`, `cpu_blk`; stages `opcode-oracle`, `xasm` | conformance (gate and sanitize), stages | W2, W5 | `guest_failure` with failing check ids in the evidence file; `OPC_*`; `XASM_MISMATCH` |
| CPU-02 | G `mmu_mpr_256k`, `_384k`, `_1m`; `bankclass_oracle`; `tam_tma_manual`; `st_bypass` | conformance | W3 | `guest_failure`; evidence names the bank and offset that differ |
| CPU-03 | `timer_pure_function` (C-level), G `tim_timer`, `irq_ctl_pre`, `irq_ctl_post`; `irq_regs_manual` | conformance | W4 | `guest_failure`; DSV1 error flag when a vector handler runs unexpectedly |
| CPU-04 | G `spd_speed`; `speed_manual`; penalty checks in `spd_speed` classed ASSUMED | conformance | W3 (penalty), W4 (speed) | `guest_failure` |
| CPU-05 | G `cpu_blk`; `block_cycles_manual`; `chunk_equiv_cpu_blk`, `chunk_equiv_cpu_blk_len0`; `chunk_equiv_random32` | conformance | W5 | digest mismatch between chunk schedules reported per schedule |
| CPU-06 | `ledger-check`, `inventory-check`, `fixture-lint` cite rules, `opcode-oracle`, record `[model_*]` golden, `model_param_refusal` | static stages, conformance | W2-W8 | `LED`/`INV_*`/`FIX_UNCITED` codes; refusal outcome |
| INP-01 | G `pad_2btn_correct`, `input_exact_edges` (C-level at-1, at, at+1) | conformance | W6 | `guest_failure` |
| INP-02 | stage `input-check` (about 15 negatives, one per INP_* code), `input_schedule_chunking` | static stage, conformance | W6 | `INP_*` code set differs |
| INP-03 | G `pad_2btn_none`, `pad_2btn_wrong`, `pad_2btn_late`, `pad_2btn_glitch`; padlog | conformance | W6 | runner order assertion; identical verdict bytes between none and correct |
| INP-04 | `input_port_refusal` (event for empty port 2-5 refused), `IOPT`/`SCHD` region presence | conformance | W6 | refusal code mismatch |
| CORE-04 | `run_until_abs`, `run_overshoot_bound`, `chunk_equiv_*` | conformance | W1-W5 | digest or machine-time mismatch |
| CORE-05 | `snapshot_restore_sampled`, `snapshot_restore_boundaries`, `snapshot_tiling`, `snapshot_hostile` | conformance (also sanitize) | W7 | memcmp or lockstep difference; sanitizer abort on hostile input |
| CORE-06 | `observer_ds_proto`, `observer_cpu_blk`, `observer_tim_timer`, `observer_pad`, `observer_random`; `trace_out_rights` | conformance | W3, extended W5/W6 | verdict, time, stop or digest differs between sink configs; `lost != 0` is `harness_failure` |
| FIX-01 | stages `fixture-build`, `xasm`, preflight; manifest `kind = "authored"` entries | static stages | W1, W2, W8 | `PREFLIGHT_MISSING`, `FIX_HASH_MISMATCH`, `XASM_MISMATCH` |
| FIX-03 | G `ds_proto` plus controls; `mutate_sweep_*`; `fixture-lint` class rules | conformance, static | W1, W8 | each unflipped check fails its sweep; `FIX_*` codes |

### Negative controls and independent oracles
| Req ID | Control that must go red | Independent oracle (class) | Ledger |
|--------|--------------------------|----------------------------|--------|
| CPU-01 | Toy ADC carry; TSV with one byte changed; shim ca65 flipping a byte; mutate sweep flip of a literal | HW H8-25 and SW pages as a separate TSV transcription (E1 documentary after img verification); GUEST reference models for ADC/SBC and shifts (E2) | LED-CPU-0001, -0002, -0009, -0010 |
| CPU-02 | Toy unmapped value; `mmu_mpr_policy_diff` negative control (needs W4 v2); wrong ST literal | Manual literals H8-7, H8-8, H8-11; awk bank-class oracle re-derived from ledger text (E0/E2 label, ER-7); in-guest write-through-mirror reads | LED-CPU-0011-0017 |
| CPU-03 | Toy timer reload off by one; toy IRQ priority swap; `irq_i_sample` pre versus post must differ | H8-20/21 C-level step and period (E1); H8-12..16 and H8-24 register facts; in-guest timer checks are E2 (circular on CPU speed, ER-3) | LED-IRQ-0002..0008 |
| CPU-04 | Mutate-sweep flips on every `spd_speed` literal | H8-18 (low speed at reset) and H8-25 (3 cycles) E1; switch behaviour and penalty ASSUMED, no oracle | LED-CPU-0005, -0006, -0017 |
| CPU-05 | Toy block progress lost on stop; toy length 0 read as 0 bytes (chunk case at length 0) | H8-6 and S8-88/89 (E1: 17+6x, 0 = 65,536); in-guest data model | LED-CPU-0007, -0008, LED-IRQ-0007 |
| CPU-06 | `FIX_UNCITED`; ledger negative `assumed-no-assumption`; removed REQUIRED id; model value outside allowed set | `cites.txt` img flags; manual TSV two-way check | LED-PROF-0004 |
| INP-01..03 | none-input and wrong-input scripts must give `guest_failure` with the declared check set; shared-input toy; late and glitch scripts | CD BIOS p. 28 mask layout (img); wire protocol HR; E2 self-consistency (ER-5) | LED-IO-0002..0005 |
| INP-02 | each `input-check` negative; selftest mutation removing a rule | Schema, not hardware | LED-IO-0004 |
| CORE-04 | toy block progress lost; chunk schedules 1, 7, 168, 3072, frame-sized, event-tick | None beyond internal consistency (E0) | LED-CPU-0004 |
| CORE-05 | Toys: drop BLKT, drop T flag, skip a field in restore | Host `shasum -a 256` of the snapshot equals the digest; raw memcmp excluding ROM (E0) | none (gate property) |
| CORE-06 | Liveness toy where the hook mutates state; `--trace-out` on a synthetic or non-authored case refused | None (E0 structural) | none |
| FIX-01 | Empty-PATH preflight; shim ca65; TSV change; manifest hash change | Two independent encoders (ca65 and the manual transcription); WLA optional | LED-TOOL-0001 |
| FIX-03 | `ds_proto` predone, nodone, badsum, wrongid; wrong-literal mini-root; `verdict_source = duospool-output` still rejected by fixture-check | Manual literals and in-guest models; expected values never from DuoSpool output | none |

### ROADMAP Phase 3 success criteria
| SC | Check | Where | Wave | Notes |
|----|-------|-------|------|-------|
| 1 | Authored fixtures for instructions, MPR, timer, IRQ, speed with cited literals; a wrong literal fails; cross-assembler identity "where practical" | fixture-build, G cases, sweeps, `xasm` (ca65 vs manual TSV required; WLA optional) | W1-W5, W8 | State exactly what ran: encodings equal the manual transcription; no second-assembler claim when WLA is absent |
| 2 | Pad fixture with three scripts; per-port script in mclk; differing verdicts; guest log | `pad_2btn_*`, `input-check` | W6 | See Open Question 1 for the guest log |
| 3 | Chunked runs equal uninterrupted; stops mid-frame and mid-block incl. length 0; snapshot restore continues identically | `chunk_equiv_*`, `snapshot_*` | W5, W7 | Mid-frame has no frame yet: use fixed mclk chunk sizes including prime sizes and timer-event ticks |
| 4 | Trace and device-write log do not change verdicts, time or digest | `observer_*` matrix, liveness toy | W3, W5, W6 | `dev` sink needs hardware-window writes; add pad and timer registers later |
| 5 | Every cycle count and penalty has a ledger entry or an assumption label; parameters named | ledger-check, inventory pins, `[model_*]` record, `[withheld]` | W2-W8 | VDC/VCE penalty is a constant with LED-CPU-0017; speed switch LED-CPU-0006 |

### Sampling Rate
- **Per task commit:** the narrowest relevant stage, for example `sh scripts/check --stage conformance` or `--stage input-check` (D-81: per-task verify at stage granularity).
- **Per wave merge:** `sh scripts/check` green with goldens regenerated and that wave's ledger entries written.
- **Phase gate:** full gate green on gate and sanitize presets, one `linux-repro` run, W8 verification report.

### Wave 0 Gaps (must exist before the code that needs them)
- [ ] `ca65` and `ld65` on PATH locally (`brew install cc65`); preflight entries; CI apt step; tool record `fixtures/lib/TOOLS.md` (licence Zlib, commit, "prints V2.18", package versions, PDF hashes above).
- [ ] Runner headroom move (Pitfall 1) and registration of new kind files in `CMakeLists.txt`.
- [ ] Case-key extension plan (Pitfall 2) with inventory-rules negatives.
- [ ] `tests/oracle/huc6280-opcodes.tsv` produced by a pass that does not read `cpu_ops.def`, from the page images (H8-25 plus S8-7..S8-9 plus instruction pages), with the printing typos of mismatch 7 handled; PROVENANCE note that it is numeric data (ER-9).
- [ ] `fixtures/lib/cites.txt` seeded from the img-verified table above, each row flagged `img` or `ocr`.
- [ ] DSV1 macros and the literal table (`verdict.inc`, `check.inc`) before any CPU fixture.
- [ ] `tests/gate` mini-roots for `fixture-lint`, `xasm`, `input-check`, authored fixture-check, and the preflight empty-PATH control.
- [ ] Anchored statements for the 8 toys written as single lines (Pitfall 4).

### Fixture inventory and check counts as planned
Counts are the CONTEXT and advisor estimates; DSV1 allows up to 96 checks per ROM.

| Fixture (ROM) | Variants and REQUIRED ids [P] | Checks | Wave |
|---------------|--------------------------------|--------|------|
| ds_proto | `ds_proto`, `_predone`, `_nodone`, `_badsum`, `_wrongid` | 8 | W1 |
| cpu_ldst, cpu_alu, cpu_flow, cpu_huc | one id each | about 60, 60, 50, 50 | W2 |
| cpu_blk | `cpu_blk` | about 40 | W5 |
| mmu_mpr | `mmu_mpr_256k`, `_384k`, `_1m`, `_policy_diff` | about 40 each | W3 (policy diff W4) |
| tim_timer | `tim_timer` | about 30 | W4 |
| irq_ctl | `irq_ctl_pre`, `irq_ctl_post` | about 30 | W4 |
| spd_speed | `spd_speed` | about 12 (penalty as ASSUMED class) | W4 |
| pad_2btn | `_correct`, `_none`, `_wrong`, `_late`, `_glitch` | about 12 | W6 |

Runner cases: eight pin cases (`opcode_table_manual`, `cycle_adders_manual`, `speed_manual`, `block_cycles_manual`, `tam_tma_manual`, `st_bypass`, `timer_pure_function`, `irq_regs_manual`), `run_until_abs`, `run_overshoot_bound`, `bankclass_oracle`, `model_param_refusal`, `trace_out_rights`,
`chunk_equiv_*` (4), `chunk_equiv_random32`, `observer_*` (5), `input_exact_edges`, `input_port_refusal`, `input_schedule_chunking`, `snapshot_*` (4), `poweron_v2_*` (2), and `mutate_sweep_<fixture>` per fixture (11). Total about 64 new ids against 104 existing ids
[CO: `tests/conformance/REQUIRED` has 109 lines, 104 of them ids; CONTEXT counts lines].

### Gate-time and CI implications
- D-60/D-132: record per-stage time with `--timing`; baseline above; target 120 s warm quiet local; stages over 20 s warn; time never fails the gate. Measure after W1 and W8.
- Fixture builds are about 1 s total (advisor: 12 builds 0.78 s); `fixture-lint` and `xasm` are small awk passes; `input-check` with 15 negatives adds a few seconds of awk.
- Runner additions at -O0: the 65,536-byte transfer is 65,536 byte steps, cheap; sweeps are ms per run if fixtures stay short [A4]; the sanitizers stage re-runs everything (about 3x slower).
- Toy mutants: each toy rebuilds the whole runner plus core (`ds_toy_NAME` in `CMakeLists.txt`); with 8 toys and 4 CI invocations (Gate, gcc, clang, second awk) this multiplies. First CI run was 100 s critical path against a 10-minute timeout [CO `docs/ci-timings.md`]; if growth nears half the budget, run toys and sweeps under one compiler (D-132).
- CI install: apt update 6 s and install 3 s in a container here [CO]; place the step before `Gate`; no new action, no secrets (D-62).

## Per-wave file touch map (for overlap detection)

Hot files touched by most waves: `scripts/check` (stages and selftest), `tests/conformance/REQUIRED`, `src/core/ds_core.h`, `src/core/serial.c`, `CMakeLists.txt`, `tests/conformance/runner.c` and `runner.h`, `scripts/conformance-run`. Give each wave one plan that owns gate wiring and REQUIRED edits, and make same-wave plans that touch those files sequential.

- **W1:** core `ds_core.h`, `advance.c` (delete), new `run.c`, `bus.c`, `cpu.c`, `cpu_ops.def` skeleton, `serial.c`, `boot.c`, `poweron.c`, `refusals.def`; `src/text/text.c`, `ds_text.h`; runner `runner.c`, `runner.h`, `stepkinds.c`, `spinkinds.c`, `ilvkinds.c`, `powkinds.c`, `digkinds.c`,
  `reckinds.c`, new `guestkinds.c`, `fixload.c`, `hashkinds.c`; about 20 case files plus 2 goldens; `CMakeLists.txt`; controls `shared-state`, `drop-region`; gate mini-roots `cancel-spin`, `timeout-budget`, `wrong-literal`; scripts `check`, `preflight`, `conformance-run`, `conformance-ingest`,
  `determinism-run`, `outcome-sync`, `record-lint`, `workflow-check` rules, `inventory-rules.awk`, new `fixture-build`, `fixture-lint`; `fixtures/lib/*`, `fixtures/src/ds_proto.*`, manifest entry; `.github/workflows/gate.yml`, `.gitignore`, `README.md`, `docs-check`; ledger LED-PROF-0002 edit,
  LED-TOOL-0001, LED-CPU-0003, -0004. Cut into at least 5 plans: (1) unit and run API, (2) printer, record and goldens, (3) runner headroom and case migration, (4) fixture toolchain and `ds_proto`, (5) CI, docs and ledger.
- **W2:** `cpu_ops.def`, `cpu.c`, `obs.c` (insn sink), `ds_obs.h`; `tests/oracle/huc6280-opcodes.tsv`, `scripts/xasm-check`, `scripts/lib/opcode-rules.awk`; fixtures `cpu_ldst`, `cpu_alu`, `cpu_flow`, `cpu_huc`; `sweepkinds.c`; first toys (`adc-carry`); ledger LED-CPU-0001, -0002, -0009, -0010.
- **W3:** `bus.c`, `ds_bus.h`, `model_params.def`, `obs.c` (dev sink), `text.c` (`[model_*]`), `record-rules.awk`; fixture `mmu_mpr`; `bankclass-oracle.awk`; `obskinds.c`; ledger LED-CPU-0011 to -0017, LED-PROF-0004, edit LED-LOAD-0005.
- **W4:** `timer.c`, `irq.c`, `ds_timer.h`, `ds_irq.h`, `poweron.c` (v2), `boot.c` (seed required for v2), `profile_fields.def` (allowed set), `ds_poweron.h`, `text.c` (policy_version per policy), `conformance-flatten` (irq_stim); fixtures `tim_timer`, `irq_ctl`, `spd_speed`; toys `timer-reload`, `irq-priority`; ledger LED-IRQ-0002..0008, LED-PROF-0003.
- **W5:** `cpu.c` (BLKT), `serial.c` (BLKT region), `chunkkinds.c`; fixture `cpu_blk`; random-program generator in the runner; toys `block-progress`, `trace-hook-mutates`.
- **W6:** `io.c`, `ds_io.h`, `serial.c` (IOPT, SCHD), `text.c` (`[input]`); `scripts/input-check`, `scripts/lib/input-rules.awk`, `conformance-flatten`; fixture `pad_2btn`; shared-input toy; ledger LED-IO-0002..0006, edit LED-IO-0001.
- **W7:** `snapshot.c`, `ds_snapshot.h`, `serial.c`, `snapkinds.c`; toys `snapshot-drop-t`, `snapshot-drop-blkt`; tiling asserts in `ds_core.h`.
- **W8:** ledger finalisation, `ledger/REQUIRED`, `tests/conformance/REQUIRED`, `README.md`, `scripts/docs-check`, `fixtures/lib/cites.txt`, verification report; no core code.

## Security Domain

### Applicable ASVS Categories (level 1, `security_enforcement` enabled)

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V1 Architecture | Yes | Core boundary check (`core-boundary-check`), no allocator, caller memory |
| V2 Authentication, V3 Session, V4 Access Control | No | No users, sessions or network service |
| V5 Input Validation | Yes | Validate-then-commit restore; input script schema and caps; sink and request validation; refusals instead of guesses |
| V6 Cryptography | Yes (integrity only) | Existing SHA-256; no custom crypto |
| V7 Error Handling and Logging | Yes | Stdout carries ids, outcomes, sizes, hashes only; trace files only with `--trace-out` for authored fixtures (D-120) |
| V8 Data Protection | Yes | No ROM, firmware or manual page content committed or logged (ER-9, D-56) |
| V10 Malicious Code | Yes | External assembler install in CI; pinned action only (D-62); no vendored GPL code |
| V12 Files and Resources | Yes | Fixture ROMs and traces under `build/`; path handling in runner options; 64 KiB and 1024-event caps |
| V14 Configuration | Yes | Workflow rules (permissions, no secrets, `sh -eu`) |

### Known Threat Patterns (the plans' threat models must cover these)

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| HuCard bytes drive the CPU to every opcode, vector and address: out-of-range ROM index, unmapped bank, stack and zero-page wrap | Tampering, DoS | Per-access translate with `bank_class[256]`; `ds_mapper_read` bounds; every undefined opcode defined; run bounded by `until_mclk`; random-ROM runs under ASan and UBSan |
| Length-0 and long block transfers, infinite loops | DoS | Absolute deadline; steps are bounded; `lost` counters never stall |
| Input script: oversize, digits overflowing u64, non-monotonic order, port out of range, opposite directions, reserved bits | Tampering, DoS | `input-check` stage 2 (15 digits max, 1024 events, 64 KiB), core re-validation in `ds_input_push`, typed refusals |
| `irq_stim` and case keys parsed by awk: injection through TOML, line over 400, non-ASCII | Tampering | `toml-subset.awk` stage 1 strictness; flatten prints nothing unless every input is clean |
| Snapshot stream: truncation, region reorder, field outside its domain, TIME near 2^64, trailing bytes | Tampering, DoS | Pass-1 validation of every field, tag order and length; refusal leaves the instance byte-identical; hostile corpus of about 500 cases under sanitizers |
| Host-supplied sinks: overlap with instance or request, misaligned or null pointers, capacity 0, poisoned header | Tampering, Elevation | `uintptr_t` range and alignment checks with `memory_invalid`; header is output-only; capacity from the request |
| Trace or device log of a commercial ROM (a disassembly) and path traversal in `--trace-out` | Information disclosure | Refuse unless the manifest kind is authored or synthetic; write only under a runner-chosen directory; stdout stays ids and hashes |
| Supply chain: `apt-get install cc65` on the runner, Homebrew bottle, WLA GPL tool | Tampering | Identity by behaviour (matrix bytes) plus recorded package; no scripts piped from the network; WLA never committed or installed in CI |
| Manual PDFs and web pages as untrusted data | Spoofing | Treat as data; never copy text or images into the repo (ER-9); record URL, sha256 and page only |
| Integer truncation in 8 and 16-bit machine arithmetic | Tampering | Explicit unsigned masks, UBSan without `-fwrapv`, `-Wconversion -Werror` |

## Project Constraints (from .claude/CLAUDE.md)

- Rights: no commercial ROMs, firmware, system cards, disc images, private saves or credentials committed or used in public CI; fixture admission needs recorded provenance and redistribution terms.
- Core boundary: deterministic machine behaviour lives in the core; hosts own input mapping, file selection, wall-clock pacing, devices, UI and persistent-storage policy.
- Dependencies: small, flat, justified; a small copied helper with provenance can beat a dependency. No new product dependency in this phase.
- Evidence: support claims name profile and evidence tier; matching hashes prove repeatability only unless tied to an independent oracle.
- CI: keep the fast PR path fast; staged fuzzing and broad matrices are Phase 6; review efficiency after each run.
- Content as attack surface: game images, firmware, disc images, save files and archive parsers are security boundaries.
- Authority gates: licence, visibility, public release, rights, hardware qualification and external publication need the owner; routine reversible work advances autonomously.
- GSD workflow: repository edits go through a GSD command; this file was written by the phase researcher and nothing else was edited.
- Repo rules relevant here: `sh scripts/check` is the single verification command; lines in TOML-subset files are at most 400 characters; repo-relative paths only; SPDX headers (MIT OR Apache-2.0 for code and scripts, 0BSD for fixtures, tests data and ledger files).

## Sources

### Primary (HIGH confidence)
- HuC6280 Hardware Manual, Software Manual and Hu7 CD BIOS Manual page images (URLs and sha256 in the verification section), read 2026-11-10.
- GitHub contents API for https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/tree/master/docs (file names, sizes, raw URLs).
- Repository files read this session: `AGENTS.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md` (Phase 3), `02-CONTEXT.md` (D-50..D-86), `03-CONTEXT.md`, `CMakeLists.txt`, `CMakePresets.json`, `src/core/*`, `src/text/text.c`,
  `tests/conformance/{runner.c,runner.h,outcomes.def,REQUIRED,EXCLUDED_IN_MODE}`, `tests/controls/*/RULE`, `scripts/{check,preflight,conformance-flatten,conformance-run,core-boundary-check,control-mutate,fixture-check}`, `scripts/lib/{toml-subset,record-rules,workflow-rules,inventory-rules}.awk`,
  `ledger/SCHEMA.md`, `.github/workflows/gate.yml`, `docs/ci-timings.md`.
- Local runs: `brew fetch cc65` and `wla-dx` (bottles extracted to a scratch directory), ca65 probes and listings, `ubuntu:24.04` container with apt `cc65` 2.19-1, `sh scripts/check --timing`.

### Secondary (MEDIUM confidence)
- Advisor reports clusterA..E and synthesis (session scratch, not committed): build measurements (12 builds 0.78 s, WLA 1.3 s build) and the prototype configs.
- pcetech.txt (author-hedged "very preliminary", [HR]): T flag, B flag, JMP indirect, branch page-cross, block I/O read rule. https://github.com/asterick/TurboSharp/blob/master/Text/pcetech.txt
- PCE_Controller_Info README for SEL and CLR behaviour [HR]: https://github.com/pce-devel/PCE_Controller_Info/blob/main/README.md

### Tertiary (LOW confidence)
- Emulator sources named in CONTEXT are reading material only; nothing here depends on them and none were copied.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH, measured on the bottle, apt in a container and a source build.
- Manual facts: HIGH for the pages listed (image read); the absence claims (no CSL text, no H8-26, no /12 ratio) rest on the 28-page and 110-page documents read here and the OCR layer [CO].
- Architecture: MEDIUM, proposals consistent with the locked decisions and the existing code.
- Pitfalls: MEDIUM to HIGH; ca65 and gate facts are observed, timing estimates are [P].

**Research date:** 2026-11-10
**Valid until:** 2026-12-10 for tool versions and gate timing; manual facts do not expire.
