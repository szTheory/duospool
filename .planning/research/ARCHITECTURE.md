# Architecture Research: PC Engine / TurboGrafx-16 Emulator (DuoSpool)

**Domain:** Console emulator core (HuC6280 + HuC6270 VDC + HuC6260 VCE + integrated PSG), with a deterministic headless-first host boundary
**Researched:** 2026-11-09
**Overall confidence:** MEDIUM-HIGH for the base-system hardware model and the scheduling recommendation (primary NEC/Hudson manuals read directly, plus four independent emulator codebases). MEDIUM for SuperGrafx. LOW-MEDIUM for CD-ROM2/Arcade Card timing (only secondary sources read). The host-boundary and LibFuturo-fit sections are proposals; no LibFuturo source was available or consulted (AGENTS.md forbids relying on other private workspaces), so the LibFuturo description is the one in PROJECT.md / EMULATOR-DNA.md.

## How to read the labels

Every claim carries one label:

| Label | Meaning |
|-------|---------|
| **[SB]** | Source-backed fact: stated by a primary or reputable document I read (source key in brackets, e.g. [SB:S1 p.H8-13]) |
| **[CO]** | Code observation: what a named emulator's source does. Evidence of one implementation's choice, **not** a hardware fact |
| **[HR]** | Historical/secondary report: forum posts, author-caveated notes, search snippets I could not verify |
| **[P]** | Proposal: my recommendation for DuoSpool; needs the stated evidence before it is a decision |
| **[U]** | Unknown / not resolved by this research |

Source keys (S1...S16) are listed at the end. Primary manuals S1-S3 were read as page images (they are NEC/Hudson manuals mirrored in the MiSTer repo; study them, do not vendor them into this repo; redistribution rights are unknown). The PSG manual (S4) was downloaded but **not read**; PSG facts below come from the HuC6280 manual's PSG interface page, the pce-devel wiki, MacDonald's notes and emulator code.

---

## 1. Hardware model (what the hardware requires of the architecture)

### 1.1 Clocks and frame structure

| Fact | Label | Source |
|------|-------|--------|
| Master clock = 6 x NTSC colour burst = 315/88 x 6 MHz = **21.47727... MHz** | [SB] | S7 (Timing page), S1 (OSC1 "21.48 MHz") |
| CPU **high speed = master/3 (7.159 MHz)**, **low speed = master/12 (1.790 MHz)**, selected by CSH/CSL; CPU powers up in low speed | [SB] speeds (S7, S8); [SB-author-caveated] power-up speed and "CSL/CSH take 3 cycles, switch latency unknown" (S5) | S5, S7, S8 |
| Timer prescaler = master/3, then /1024 -> ~6.992 kHz tick (period ~143 us); 7-bit down counter | [SB] | S1 p.H8-20/21 |
| VCE dot clocks: master/4 (~5.37 MHz), master/3 (~7.16 MHz), master/2 (~10.74 MHz) | [SB] divisors (S7); S3 documents **only** DCC=0 (/4) and DCC=1 (/3) in its register table, the /2 mode appears in S5, S10, S11, S12 | S3 p.H6-3, S5, S7 |
| A scanline is **1365 master clocks** (= 227.5 colour-burst cycles); lines per frame **262 or 263** | [CO] three codebases agree on 1365 (S10 `HUC6260_LINE_LENGTH`, S11 `1365` screen width, S12 `WPF = 1365`); [SB] 262.5-line-like NTSC arithmetic is consistent but the manual pages I read do not state line counts | S10, S11, S12 |
| Resulting frame rate: 262 lines = **60.054 Hz** (357,630 master clocks/frame); 263 lines = **59.826 Hz** (358,995) | derived arithmetic from the above [P-derived] | computed |
| PSG sample/clock rate: **master/6 = 3.5795 MHz** in emulator practice, although the CPU manual lists the PSG clock input CLKP as OSC1/3 | [SB] CLKP = OSC1/3 (S1 p.H8-23); [CO] master/6 in S10 `Audio::ClockSources` and S11 (`colorburst` rate); [HR] S5: "patents say 7.16 MHz but the frequency formulas effectively double the result" | **CONFLICT**, see 1.9 |

**Architectural consequence [P]:** one monotonic `u64` machine-time counter in **master clocks** is the only clock. Everything else is a derived divisor (CPU /3 or /12, timer /3072, dot /2-/4, PSG /6). Integer ratios throughout; no floats and no host time in the core.

### 1.2 HuC6280 CPU

| Fact | Label | Source |
|------|-------|--------|
| 65C02 derivative; 234 instructions; 7-bit timer, 8-bit input port K, 8-bit output port O, PSG on one chip | [SB] | S1 p.H8-1 |
| 8 MPR registers; the top 3 bits of the 16-bit logical address select an MPR; its 8-bit value becomes physical address bits A13-A20; physical space = 256 x 8 KiB banks = 2 MiB | [SB] | S1 p.H8-7, H8-10/11 |
| Reset vector read from physical $001FFE/$001FFF (MPR7 is $00 at power-up so this is logical $FFFE/F). MPR7 = 0 at power-up, other MPRs/A/X/Y/S random | [SB] reset vector (S1 p.H8-3); [HR] power-up randomness (S5) | S1, S5 |
| Stack lives at logical $2100-$21FF; ST0/ST1/ST2 write VDC registers directly, bypassing MPR mapping | [SB] stack (S1 p.H8-3); [HR] ST0-2 (S5) | S1, S5 |
| T flag ("memory operation"): set by SET, cleared at next fetch, redirects ADC/AND/ORA/EOR to zero page via X; saved across interrupts | [SB] | S1 p.H8-5 |
| Block transfers (TII/TIN/TIA/TAI/TDD) save A, X, Y on the stack, move one byte per iteration; **length $0000 = 65,536 bytes**; execution takes **17 + 6x cycles** (x = length) | [SB] | S1 p.H8-6 |
| 17 + 6 x 65,536 = 393,233 CPU cycles = **~54.9 ms at high speed = ~3.3 frames** (~13 ms x 4 at low speed) | derived [P-derived] | computed |
| Interrupt inputs are sampled in the **last bus cycle of an instruction**; priority **RESET > NMI > BRK > TIMER > IRQ1 > IRQ2**; vectors: IRQ2/BRK $FFF6, IRQ1 $FFF8, TIMER $FFFA, NMI $FFFC, RESET phys $1FFE | [SB] | S1 p.H8-12/13 |
| IRQ1/IRQ2 are **level-sensed**; NMI is edge-sensed; on interrupt: push PCH, PCL, P; set I; clear D; B pushed as 0 | [SB] | S1 p.H8-12..14 |
| 4 of 4 emulators read (Mednafen, Geargrafx, Ares, MAME) check TIQ first, then IRQ1, then IRQ2 | [CO] consistent with the manual | S9, S10, S11, S12 |
| Block transfers are **not interruptible mid-instruction**: a pending IRQ fires after the whole instruction finishes. (Geargrafx executes block transfers one byte per internal step, but only dispatches IRQ when `m_transfer_state == 0`) | [HR] S5 hardware tests; [CO] S10; consistent with S1 "sampled in last bus cycle" | S1, S5, S10 |
| Differences from stock 65C02 observed by one author: no extra cycle on page-crossing branches, no dummy RMW read/write, illegal opcodes = 2-cycle NOP (clears T), decimal ADC/SBC does not touch V | [HR] author-caveated | S5 |
| VDC and VCE register accesses cost **one extra CPU cycle** (MAME: `check_vdc_vce_penalty` = 1; Geargrafx `InjectCycles(1)`, 2 for a VDC write during a block transfer) | [HR] S5; [CO] S10, S12 | S5, S10, S12 |
| Revisions: HuC6280 vs HuC6280A (PSG output centring differs; Mednafen exposes a `psgrevision` setting and defaults to HuC6280A for SuperGrafx). Whether CPU timing differs between revisions | [CO]/[HR] for the PSG difference (S9 option text, EMULATOR-DNA PCE_Soundtest lead); **[U]** for any CPU difference | S9 |

### 1.3 Memory map (physical, bank numbers = 8 KiB units)

| Banks | Contents | Label | Source |
|-------|----------|-------|--------|
| $00-$7F | HuCard ROM (small ROMs mirrored) | [SB] | S5, S7 |
| $80-$F7 | Unmapped on base system (reads $FF) -- **CD-ROM2: $80-$87 = 64 KiB RAM**; Super System Card adds 192 KiB at $68-$7F | [HR] S5; [SB-wiki] S7 for CD RAM | S5, S7 |
| $F7 | First 2 KiB = backup RAM (BRAM, 2 KiB at $1EE000-$1EE7FF) on CD systems, lock/unlock via $1803/$1807 | [HR] | S5 |
| $F8-$FB | Work RAM: **8 KiB mirrored x4 on PC Engine/TG-16; 32 KiB (flat) on SuperGrafx** | [SB-wiki] | S7, S6 |
| $FC-$FE | Unmapped | [SB] | S5 |
| $FF | Hardware page: $0000-$03FF VDC (4-byte mirror), $0400-$07FF VCE (8-byte mirror), $0800-$0BFF PSG, $0C00-$0FFF timer (2-byte), $1000-$13FF I/O port, $1400-$17FF interrupt registers (4-byte), $1800-$1FFF CD interface (or $FF) | [SB] S1 p.H8-24 (addresses), [SB] S5/S7 (mirroring) | S1, S5, S7 |
| Hardware page **I/O buffer**: reads from unmapped/partially-driven hardware-page locations return the last value on an 8-bit bus latch with register-specific bit masks | [HR] author-tested; edge cases self-described as unclear | S5 |

**Architectural consequence [P]:** the memory system is a **256-entry bank table** (physical bank -> read/write handler + backing store), plus a hardware-page decoder. This is exactly the shape every emulator read converged on (Geargrafx `m_memory_map[256]` + mapper override for banks < $80 + `ReadHardware/WriteHardware` hook, S10). This is the seam for HuCard mappers, CD RAM, Super System Card RAM and Arcade Card; it is **not** a generic bus abstraction.

### 1.4 Interrupt registers and timer ([SB] S1 p.H8-14..22 unless stated)

- `$1402` (A1:A0 = 1:0), read/write: **interrupt disable** mask, bit0 IRQ2, bit1 IRQ1, bit2 TIQ; 1 = disabled.
- `$1403` (1:1): read = request status (inverted pin levels for IRQ1/IRQ2, latched TIQ); **any write acknowledges TIQ**.
- Timer: write `$0C00` = 7-bit reload; write `$0C01` bit0 = start/stop; 0->1 loads counter from reload and resets the prescaler; on borrow, TIQ is set and counter reloads from the latch. Reading the counter can return invalid values (manual advises double-reading).
- Disabling an interrupt does not drop the line; the status register still shows it ([HR] S5).
- Exact "underflow reads $7F" and read-while-reload edge behaviour is emulator-specific (Geargrafx reads `$7F` when counter==0 and within 15 master clocks of reload): [CO] S10; **[U]** hardware truth.

### 1.5 I/O port and controllers

| Fact | Label | Source |
|------|-------|--------|
| `$1000` write: D0 = SEL, D1 = CLR; read: D3-D0 = pad data (active low), **D6 = region (1 = PC Engine, 0 = TurboGrafx-16)**, **D7 = CD-ROM sense (1 = not attached)**, D5/D4 = 1 | [HR] S5; [CO] S10 (`m_pce_jap` -> bit 6, `!m_cdrom` -> bit 7) | S5, S10 |
| 2-button pad: SEL=0 -> {Run, Select, II, I} on D3..D0; SEL=1 -> {Left, Right, Down, Up}; CLR high forces D3-D0 = 0 | [HR] | S5 |
| Multitap (5 players): internal counter advances on SEL 0->1 while CLR low; CLR 0->1 with SEL high resets it | [HR] | S5 |
| 6-button (Avenue Pad 6), mouse, turbo/autofire devices | **[U]** no primary protocol source read; Geargrafx implements Avenue Pad 3/6, mouse, MB128 (S10) | -- |
| Controller state is a **stateful protocol machine** (SEL/CLR/counter) that the CPU reads live; the host only supplies the buttons' level state | [CO] S10 `Input::WriteO/ReadK`, S9 `INPUT_Frame` | S9, S10 |

### 1.6 HuC6270 VDC ([SB] S2 unless noted)

- Registers (via address register + data port at $0000/$0002/$0003; ST0/ST1/ST2 shortcuts): R00 MAWR, R01 MARR, R02 VWR/VRR, R05 CR, R06 RCR, R07 BXR, R08 BYR, R09 MWR, R0A HSR (HDS, HSW), R0B HDR (HDW, HDE), R0C VPR (VDS, VSW), R0D VDW, R0E VCR, R0F DCR, R10 SOUR, R11 DESR, R12 LENR, R13 DVSSR. (p.H7-2)
- Status register bits: BSY, VD, DV, DS, RR, OR, CR (p.H7-2/3). Interrupt enables in CR bits 0-3 (collision, overflow, raster, VBlank). Status read acknowledges (and VDC IRQ line is shared into **IRQ1**) [HR S5, CO S12].
- VRAM: 64 KiB present (32K x 16-bit words); addressable 128 KiB with the upper half mirrored/undefined [HR S5; CO S9 default `pce.vramsize` 32768 words].
- **Background:** 8x8 character, BAT virtual size 32/64/128 x 32/64 chars via MWR SCREEN field; 16 palettes x 16 colours. **Sprites:** 64 in the internal SATB (filled only by VRAM->SATB DMA), 16x16 base cells combinable to 16/32 x 16/32/64, **max 16 sprite slots per scanline** ([HR] S5; over-limit sets OR flag).
- **VRAM access scheduling [SB] (p.H7-8):** the VDC divides each 8-dot "character cycle" into fetch slots. In 1-dot mode the CPU gets slots 1,3,5,7 (BAT in 2, CG in 6,8); in 2-dot mode the CPU gets one slot per 8 dots (slot 3-4), in 4-dot mode **no CPU slot during active display**. Sprite generator fetches happen in horizontal blanking with similar width modes. **This is the source of the "VDC access wait state"** and of BSY in the status register.
- **Register latch timing [SB] (p.H7-6..10):** BXR becomes valid on the **next scan line**; BYR written mid-display becomes valid with the next line (BYR+1); MWR VRAM-width becomes valid **after the next VBlank**; sprite width at the start of VBlank; HSW/HDS valid at the start of the next horizontal period; VDW at start of the vertical display period; VCR at the start of the front porch; SB/BB (sprite/BG enable) valid from the next horizontal display period.
- **Raster compare [SB] (p.H7-7):** internal line counter is set to **64** at the line before display start and incremented each line; interrupt when counter == RCR. Valid RCR is therefore $40..~$146 ([HR] S5). Real-HW placement of the IRQ within the line: Mednafen fires it **4 VDC clocks before HDW end** [CO S9], MAME "near the end of the HDW period" and notes "confirmed on real HW" for a related VSW-latch quirk [CO S12]. Exact hardware position **[U]**.
- **DMA [SB-partial]:** VRAM->VRAM (SOUR/DESR/LENR, runs only outside active display; 2 VDC clocks/word in Mednafen [CO]) and VRAM->SATB (completes ~4 lines after active display; `DS` flag) [HR S5]. MAME marks both DMA timings "incorrect" in its own source [CO S12]. **Dot-clock-dependent DMA speed [U]**.
- **VBlank IRQ placement:** "set on first line after active area" [HR S5]; MAME: "VBlank IRQ happens at the beginning of HDW period after VDW ends" [CO S12]; Covell reports Mednafen's VSync interrupt fires late versus hardware [HR S13]. **Conflicting/unsettled.**

### 1.7 HuC6260 VCE

| Fact | Label | Source |
|------|-------|--------|
| 512 colour-table entries x 9 bits (G3 R3 B3); 256 background + 256 sprite; index = VD8 (sprite flag) : VD7-4 palette : VD3-0 pattern colour | [SB] | S3 p.H6-3/4 |
| CPU writes palette via CTA (address; auto-increments after the **high byte**) and CTW; CPU read via CTR | [SB] | S3 p.H6-3/5 |
| CR: DCC (dot clock divisor); bit 7 = greyscale, bit 2 = blur ("some games"), and (per emulators) a bit that selects 262 vs 263 lines | [SB] DCC (S3); [HR] bits 7,2 (S5); [CO] 262/263 select (S10 `k_huc6260_total_lines`, S11 `extraLine`, S12 comment "Select screen height 0=262,1=263") | S3, S5, S10-S12 |
| Palette CPU access during active display "streaks": VCE repeats the previous pixel's colour | [HR] S5 + forum repost; not modelled by any emulator I read | S5 |
| Overscan/blank colour: sprite palette 0 colour 0 (and BG-off/sprites-off interior) | [HR] | S5 |
| Active picture widths vary by VDC HDW and VCE dot clock: e.g. 256 (5 MHz mode), 320 (7 MHz), 512 (10 MHz) typical; overscan modes wider | [HR] S5 display table; [CO] S10 `k_huc6260_line_width` {256,341,512} (+overscan), full line {342,455,683} dots | S5, S10 |
| **Width can change mid-frame** (per-line width table in Mednafen `LineWidths`; Ares emits fixed 1365 master-clock-wide lines with pixel replication) | [CO] S9, S11 | S9, S11 |

### 1.8 PSG (HuC6280-integrated)

- 6 channels; each plays a **32-sample x 5-bit unsigned waveform**; channels 0-1 support frequency modulation (LFO; channel 1 modulates channel 0 and is itself muted); **channels 4-5 support noise** [SB-wiki S8 for 6 channels/waveform/DDA/noise; the "noise only on 4-5" detail from [CO] S11 `index >= 4 && noiseEnable`; pairings in S8 are unspecific].
- Per channel: 12-bit frequency, enable, DDA ("direct D/A") bit, volume, L/R balance; global L/R volume; waveform RAM writable only while the channel is disabled and DDA clear; waveform index resets on DDA 1->0 [HR S5].
- Output revision difference HuC6280 vs HuC6280A [CO S9; HR]. LFO register details **[U]** here (manual S4 unread).
- **Determinism hazard [CO]:** Ares seeds noise from `nall::random()` (S11 `psg/channel.cpp`); the real LFSR/seed behaviour is **[U]**. A reproducible core needs a defined deterministic noise generator (documented as an approximation until measured).

### 1.9 Conflicting or unsettled sources (explicit list)

| Topic | Source A | Source B | Handling |
|-------|----------|----------|----------|
| PSG clock | S1: PSG clock input = OSC1/3 (7.16 MHz) | S5/S10/S11: effective rate master/6 (3.58 MHz) | Define PSG tick = master/6 as a **profile parameter**, flag as [U] until an independent measurement (PCE_Soundtest-type capture) settles it |
| Dot clock modes | S3: DCC 0 = /4, 1 = /3 only | S5/S10/S11/S12: three modes incl. /2 (10.74 MHz; CR value 2 and 3 identical) | Implement three; manual is the incomplete one |
| VCE 10 MHz value | S5 prose "10.8 MHz" | S7 "master/2" = 10.74 MHz | Use master/2 |
| Line count | S5: 263 fixed | S10/S11: 262 default, 263 selectable (CR bit) | Profile parameter; first profile choice is an owner/measurement decision |
| Chip numbering in S5 | S5 section headings swap "HuC6260 VDC / HuC6270 VCE" (HuC6270 = VDC, HuC6260 = VCE per S2/S3) | -- | Typographical error in S5 (as fetched); treat facts by content |
| VBlank IRQ timing | S5: first line after active area | S12: beginning of HDW after VDW; S13: Mednafen too late | **[U]** needs a hardware oracle (owner gate) |
| Interrupt priority | S1: TIMER > IRQ1 > IRQ2 | all four emulators agree | No conflict (S1 resolves) |
| Redistribution | S5 asks not to be linked/redistributed | -- | Cite for facts; do not vendor |

### 1.10 Profile dimensions that change behaviour (feeds the "first profile" decision)

[P] A **profile** is an immutable struct fixed at instance creation: `{ region_bit, cpu_rev, psg_rev, lines_per_frame (262|263), vdc_count (1|2), vpc_present, workram_layout, hardware_page_decode, input_ports, media_slots }`. Known axes: region bit in `$1000` (S5), HuC6280 vs 6280A PSG (S9), 262/263 lines (S10/S11), SuperGrafx (S6), CD present bit (S5). TG-16 vs PCE **HuCard data-line/bit-order and physical differences**: **[U]** (PROJECT.md says "reported signal and content-protection behaviour"; I found no source read in this pass that pins the data-line swap; treat as a research item before the first content-validation phase).

---

## 2. Timing and scheduling

### 2.1 What granularity does known raster-sensitive behaviour need?

| Behaviour | Needs | Evidence |
|-----------|-------|----------|
| Timer tick/IRQ | Master-clock timestamps; IRQ checked at instruction boundary (last bus cycle) | [SB] S1 |
| Per-instruction cycle counts | **Cycle-count tables** per opcode x speed; a one-instruction-per-scanline test shows drift as a slanted raster split | [HR] S13 (Covell CPU test) |
| Block transfers (and their VRAM interaction) | **Per-byte** stepping with VDC slot availability; whole instruction can span >3 frames | [SB] 17+6x (S1); [HR] S13: VRAM transfer timing depends on CPU/VDC bus interleave; Mednafen "always uses minimal VRAM access time" |
| VDC CPU access slots / BSY | **Dot/slot-level** (8-dot character cycle, access-width mode) | [SB] S2 p.H7-8; [CO] S10 `k_huc6270_vram_read_delay`/`write_delay` per dot-clock mode; [HR] forum report that X-cycle tests need hblank slot modelling |
| BXR/BYR latch vs mid-line write | **Dot-phase** latch points inside HDS | [SB] S2 p.H7-7; [CO] S9 `TimeFromHDSStartToBYRLatch` |
| Raster IRQ position | Dot-phase within line | [CO] S9, S12 |
| Sprite-per-line budget | Line-level with a fetch-cycle budget (truncation when HBlank cycles run out) | [CO] S9 `sprite_cg_fetch_counter`; [HR] S5 16/line |
| Palette write mid-line streaking | Dot-level colour-lookup stall | [HR] S5; no emulator models it -> **[U]**, defer |
| PSG register writes | Master-clock timestamp per write; synthesis tick /6 | [CO] S10 (`WritePSG` calls `ClockPendingCycles` first), S9 |
| Joypad polling | Live protocol reads; level state sample is enough for once-per-frame pollers | [CO] S9/S10 |

**Conclusion [P]:** frame- or scanline-only scheduling cannot be the contract. The minimum viable timing model is **instruction-granular CPU with per-access master-clock timestamps, a device catch-up before every hardware-page access, and a dot-phase (character-cycle) VDC state machine**. Per-dot pixel rendering is not required for the first slice; **line rendering at a defined phase point with latched registers** (as Mednafen does) is the right first approximation. The evidence plan is a set of authored raster-alignment fixtures (Section 8).

### 2.2 How existing emulators schedule (code observations, all [CO])

| Emulator | CPU/device model | Notes relevant to DuoSpool | Source |
|----------|------------------|----------------------------|--------|
| **Mednafen `pce`** | One master-clock `timestamp` in the CPU. `ADDCYC` adds `(cycles*3) << speed_shift` (slow = x4) and triggers `HappySync` when `next_event <= 0`. The VCE is the event scheduler: it runs the CPU to the next event (hsync/vsync/VDC child event/CD event), then advances the VDC(s) in dot-clock chunks. The VDC is a **horizontal phase machine** (`HPHASE_HSW/HDS/HDS_PART2/HDS_PART3/HDW/HDW_FINAL/HDE`) with cached `CR/HDS/HDW/HDE` at line start, BYR/BXR latch points inside HDS, RCR increment at `HDW` end, and **draws the whole BG/sprite line at HDW start**. Input is latched by `INPUT_Frame()` at the start of each `RunPartial`. Audio: PSG updated by timestamp; hi-res integration buffer at master/12, then resampled | Lazy catch-up, event-driven, sub-line latch accuracy without per-dot rendering. A separate `pce_fast` core exists (speed-oriented) | S9 |
| **Geargrafx** | `RunToVBlank(...)` loops `RunInstruction()`; each instruction returns master cycles; `ClockHardware(cycles)` advances timer, `HuC6260::Clock` (VCE drives VDC/line state), CD, audio. **Sub-instruction sync:** the CPU calls `ClockPendingCycles()` before PSG writes and `InjectCycles()` for VDC/VCE access wait states; CPU-VRAM accesses are queued and processed in <= 3-master-clock steps while pending. Block transfers execute **one byte per step**. `template<is_cdrom,is_sgx,is_laseractive>` specialisation of the hot loop. One `m_master_clock_cycles` u64. Audio: PSG ticks at master/6; samples are taken with a fixed-rate fractional accumulator (`cycles * sample_rate` vs master rate), **per-source buffers** (PSG, ADPCM, CD-DA). Tests: `tests/` runs SingleStepTests/65x02 JSON | The closest structural match to what DuoSpool wants (explicit state, one counter, template profiles). GPL-3.0 | S10 |
| **Ares `pce`** | Cooperative threads (`CPU::main`, `VDP::main<supergrafx>`, `PSG::main`), all stepping at `colorburst*6` (21.477 MHz) with `synchronize()` calls: CPU `step()` synchronizes PSG (on timer tick), VDP and PCD every step. VDP loop is **per-dot** (`hclock` per VDC, VPC bus, VCE colour lookup, pixel replicated by `vce.clock()` into a **1365 x 263 master-clock-resolution frame**). PSG: accuracy profile runs 1 sample per 3.58 MHz tick; performance profile decimates x64. The faster scanline VDP "performance" implementation exists but `option()` forces accurate mode: "scanline renderer is too buggy" | Evidence that a second, lower-accuracy renderer drifts and gets abandoned. Per-dot is simple to reason about, costlier. Output at master-clock resolution is a lossless raw-video representation | S11 |
| **MAME `pce`** | Standard MAME device scheduler. `H6280` runs in cycles with `m_clocks_per_cycle` (fast 1, slow 4) over a `MAIN_CLOCK/3` device clock; timer counted in those clocks; VCE/VDC/VPC at `MAIN_CLOCK`. Screen `set_raw(MAIN_CLOCK, WPF=1365, ..., LPF=263, 18..260)`; VDC steps per pixel, horizontal state every 8 pixels. VDC HSYNC/VSYNC are driven by the VCE. PSG is internal to the CPU device. TODOs: DMA timing "incorrect", sprite precaching, "Add 263 line mode", "properly implement the pixel clocks", "test sprite collision and overflow interrupts", expansion port | Mature framework; not a hardware oracle (its own TODOs say so) | S12 |
| **MiSTer TurboGrafx16** | FPGA RTL (`HUC6280/`, `huc6270.vhd`, `huc6260.vhd`, `huc6202.vhd`, `cd/`), mirrors the primary manuals under `docs/` | CPU-test results: ran block transfers too fast and code from DDR3 too slow (memory latency specific to FPGA, not a model for software) [HR S13]. Useful as a doc source (S2/S3/S6), not as a timing structure to copy | S6, S13 |

### 2.3 Recommended scheduler [P]

```
state: machine_time: u64   // master clocks, monotonic across advance() calls, never reset

cpu.step():
    t0 = cpu.time
    fetch opcode (access at t0)
    for each bus access in the instruction (index i):
        ts = t0 + cycles_so_far(i) * divisor(speed)        // divisor 3 or 12
        if address is hardware page:
            catch_up_devices(ts)                             // VDC, VCE, timer, PSG, pad, CD
            perform access; add wait-state (VDC/VCE +1 cycle) and VDC-slot stall
        else: perform access
    cpu.time = t0 + total_cycles * divisor + stalls
    check IRQ lines at the last bus cycle (not after full instruction), vectors per S1
    block transfers: 1 byte per iteration as a resumable micro-step (see 3.2)

catch_up_devices(ts):  // each device exposes run_to(ts) and next_event_time()
    timer.run_to(ts); vce.run_to(ts) -> sync pulses -> vdc.run_to(ts); psg.run_to(ts); ...
```

Why this and not the alternatives:

- **Lockstep per instruction with a device chunk after each instruction** is simpler but cannot place a register write inside the instruction that performs it (the Covell test slant). Geargrafx added `ClockPendingCycles`/`InjectCycles` to patch exactly that, which signals that "catch-up at access time" is the natural primitive.
- **Cooperative threads (Ares)** give clean per-chip code but put chip state on coroutine stacks; explicit state machines serialise and diff more easily for a save-state/negative-control-heavy project [P]. Not an anti-pattern, a cost.
- **Event-driven lazy (Mednafen)** is the fastest and what I recommend as the *optimised* form: same semantics, devices only run when asked or when `next_event_time` is due. Start with "always catch up to the access timestamp and to the instruction end"; optimise to lazy only after the fixtures pass and a profile shows the cost (AGENTS.md: profile before optimising).
- **One timeline per instance**; no wall clock; no `rand()`; power-on randomness from a seeded generator carried in state [P].

[U] Speed-switch (CSL/CSH) timing details; exact VDC slot phase relative to the CPU clock at power-on (Covell reports dependence on "when the VDC starts up" [HR S13]); exact hardware IRQ line sampling vs a VDC status write in the same instruction. **These are the first things a timing fixture must pin down.**

---

## 3. Core/host boundary

### 3.1 Principle (from AGENTS.md / PROJECT.md, restated as constraints)

The core owns: machine state, the master-clock timeline, input protocol state machines, VDC/VCE/PSG/CPU behaviour, cartridge/mapper behaviour. The host owns: files, paths, wall-clock pacing, device I/O, colour conversion to display formats, audio filters/resampling to a device rate, persistent-storage policy, and UI. The core exposes **observations**; it does not own presentation.

### 3.2 Bounded execution

[P] The primitive is `advance(instance, until, input_events, limits)` where `until` is one of: *machine time T*, *end of the current frame (VCE vsync)*, *N instructions*, *N master clocks*. It always stops at an **instruction boundary or a resumable block-transfer byte boundary**, never inside an instruction's atomic bus cycle. Why this matters:

- One block-transfer instruction can last ~3.3 frames at high speed (S1 17+6x; derived). "Run one frame" must therefore be allowed to return mid-transfer, with the transfer counters in saved state (Geargrafx keeps `m_transfer_state/count/length/source/dest` in state, S10). Otherwise `advance` must tolerate a multi-frame overshoot and stay deterministic.
- Overshoot is bounded and reported: result = `{stop_reason, machine_time_reached, frame_index, line, dot_phase}`.
- A frame-level wrapper (`run_frame`) is a thin convenience over the same primitive, so frame stepping and sub-frame stepping have one implementation.

### 3.3 Input

[P] Input is an **ordered list of `(machine_time_offset, port, device_state)` events**; a frame-level caller supplies exactly one event at offset 0 per port. Reasons:

- The pad is a protocol machine read live through `$1000`, but game code overwhelmingly polls once per frame, so frame-start sampling is correct for those games and for first-slice fixtures [CO S9: input latched per frame].
- Timestamped events let tests prove *when* an input was consumed (negative controls, mid-frame polling, latency checks) without changing the core.
- Autofire/turbo is a host (or script) concern; Geargrafx advances turbo counters in `EndFrame()` inside the input component (S10), which ties core behaviour to frame-count and should not be copied [P].
- **Do not model input as a 32-bit mask internally.** 5-player multitap with 2-button pads = 40 bits; with 6-button pads = 60; mice carry relative deltas (S5, S10) [P/derived]. Per-port, per-device-type state, with a 32-bit-mask adapter for the single-pad case.

### 3.4 Content

[P] Content is a **typed list**, not one span: `[ {slot: hucard, bytes}, {slot: system_card, bytes}, {slot: disc, sector_provider}, {slot: backup_ram, bytes}, ... ]`. v1 uses only `hucard`. Validation in the core's load path: size bounds, copier-header handling, mirroring rules, refusal of unsupported profile/mapper, no mutation of a running instance on refusal (EMULATOR-DNA lifecycle lessons). The HuCard header/size conventions and which special mappers exist are **[U]** and must be sourced in the content-loading phase. Disc content needs random-access sector reads; to stay deterministic the host must answer sector requests synchronously from the core's viewpoint (host latency never enters machine time) [P].

### 3.5 Raw video

[P] Define raw video before any host conversion:

```
frame_record:
    lines_total:  262 | 263
    per line:     dot_clock_divisor (4|3|2), active_dots, first_active_dot_offset, u16 pixels[active_dots]
    pixel word:   9-bit VCE colour value (G3R3B3) | grey flag | (optional debug: pre-palette VDC value, sprite/BG origin)
    master-clock origin of each line (for alignment with trace/PSG events)
```

Reasons: the VCE outputs 512 distinct 9-bit colours (S3) so gray8/RGB24/index8 either lose the palette semantics (index8 needs 512) or force a 3-bit->8-bit expansion policy that belongs to the host; width varies 256/341/512+ and can change per line (S9 `LineWidths`, S11 fixed 1365-clock line buffer); the 262/263 count and dot-clock-dependent pixel aspect (S5, S10) must be visible. Ares' 1365 x 263 master-clock-resolution representation (S11) is the simplest lossless form: any host resamples from it; a compact per-line form is the same information. **Which representation to pick is a first-slice decision**; either is fine if the semantics above are preserved.

### 3.6 Raw audio

[P] Two layers, both deterministic, neither device-shaped:

1. **PSG event log:** `(master_clock_timestamp, register, value)` plus an initial full-state snapshot; this is what a regression test asserts and what a VGM-style exporter needs (Geargrafx's VGM recorder works from register writes, S10).
2. **PCM at a defined integer-ratio rate:** per-source (PSG, later ADPCM, CD-DA) buffers like Geargrafx, produced by an exact fractional accumulator against the master clock (S10), without analogue filter emulation in the raw path (Ares applies a 20 Hz high-pass in `PSG::load`; Mednafen applies low/high-pass shifts in the integrator; those are presentation choices [CO S9, S11]).

Cadence is non-integer: ~734.33 samples/frame at 44.1 kHz on 262 lines, ~737.14 on 263, ~799.27 at 48 kHz (derived). Any caller-owned fixed-capacity buffer contract must allow variable counts per frame and report them.

### 3.7 Save state

[P] State shape: one versioned blob **per instance**, with its own version independent of API and behaviour revision. Contents: master-clock counter; CPU registers, MPR0-7, speed, T/I/D flags, pending IRQ lines, disable mask, timer (counter, reload, enable, prescaler phase), block-transfer micro-state, I/O buffer; VCE (CR, CTA, 512-entry palette, hcounter/vcounter, line-width table for the frame in progress); each VDC (all registers, VRAM 32K words, SATB 256 words, read/write buffers, RCR counter, h/v phase counters, cached latched values, DMA state, status/IRQ line); PSG (per-channel regs, waveform RAM, phases, noise state, balance, LFO); pad protocol state; mapper/battery RAM; seeded RNG state. **Excluded:** ROM bytes (identity hash instead), host settings, trace buffers. Geargrafx's state includes `m_master_clock_cycles` and the CPU transfer state (S10 `huc6280.cpp`); Ares serialises per component (S11 `serialization.cpp`). Mid-frame save is allowed only at an `advance` stop point (instruction/byte boundary) [P].

### 3.8 How a LibFuturo-style C interface would or would not fit

LibFuturo shape (per PROJECT.md/EMULATOR-DNA.md only): opaque instance; content + config id; 32-bit logical-button mask; bounded frame/tick advance with input sampled at advance start; caller-owned gray8/RGB24/index8 video and native-endian interleaved S16 audio; capability/profile queries; optional persistent media.

| LibFuturo feature | Fit for a first single-HuCard, single-2-button-pad profile | Mismatch to test with a real consumer (all [P]/[U]) |
|-------------------|-----------------------------------------------------------|------------------------------------------------------|
| Opaque instance, per-instance state | **Fits** (explicit per-instance state is the design) | Prove independence with two interleaved instances |
| Content + config id, single span | Fits HuCard | Does **not** fit system card + disc + backup RAM; config id must carry the profile (revision, region, 262/263) |
| 32-bit button mask, sampled at advance start | Fits one pad, frame-granular | Multitap/6-button/mouse exceed 32 bits (derived); mid-frame polling tests need timestamped events; tick-sized advance must still stop at instruction boundaries |
| Bounded frame/tick advance | Fits `run_frame`; tick = master clocks maps naturally | Block transfers can exceed a frame (S1): define overshoot/return behaviour; frame length is 357,630 or 358,995 master clocks (variable) |
| Video gray8/RGB24/index8 | RGB24 works as a **converted** view; not raw | 512-colour palette, per-line width changes and 262/263 variability need either a geometry query per frame or an extension; index8 cannot carry 512 indices; colour expansion policy is a host/adapter choice |
| Audio S16 interleaved | Fine for the *rendered* PCM view | Raw PSG event log has no slot; non-integer samples/frame; CD-DA/ADPCM mixing policy |
| Capabilities/profile queries | Needed (profile axes in 1.10) | Profile vocabulary is DuoSpool's, not LibFuturo's |
| Persistent media | BRAM 2 KiB (S5), HuCard battery RAM: small, single store | Multiple stores (BRAM + Memory Base 128 + Arcade) need a typed list |
| In-process trust model | Acceptable for a local runner | Content is untrusted (AGENTS.md); a parser fault is in-process |

**Recommendation [P]:** do not freeze anything. Build DuoSpool's own internal `advance(until, events)` first, write a **frame-level adapter** shaped like LibFuturo as one experiment, and record each concrete mismatch as a reproducible case. A frame-level wrapper is plausibly sufficient for a first limited profile (EMULATOR-DNA says to confirm, not assume).

---

## 4. Recommended architecture

### 4.1 System overview

```
 HOST (outside the core)                    |  CORE (one instance = all machine state)
 ------------------------------------------ | -------------------------------------------------------------
  runner / frontend / LibFuturo adapter     |
   - files, paths, CI, wall clock           |   profile (immutable)           content list (typed slots)
   - input script/device mapping            |        |                             |
   - colour + audio presentation            |        v                             v
        |            ^                      |   +----------------+      +---------------------+
        | advance()  | observations         |   | Scheduler /    |<---->| Cartridge / mapper  |
        | input evts | (video, audio,       |   | master clock   |      | (bank handlers,     |
        v            | trace, state)        |   | u64 timeline   |      |  battery RAM)       |
  +-------------+    |                      |   +--------+-------+      +----------+----------+
  | core API    |----+--------------------> |            |                          |
  | (internal,  |                           |            v                          v
  | unfrozen)   |                           |   +----------------+   +-----------------------+
  +-------------+                           |   | HuC6280 CPU    |-->| Memory map (256x8KiB   |
                                            |   | + MMU, timer,  |   | bank table) +          |
                                            |   | IRQ ctrl, I/O  |   | hardware-page decode   |
                                            |   +---+---+---+----+   +-----------------------+
                                            |       |   |   |
                       IRQ1/IRQ2 lines <-----+-------+   |   +--> Input port / pad protocol machines
                                            |           v
                                            |   +----------------+       +-----------+
                                            |   | VCE (HuC6260)  |<------|  HuC6270  |  (x1 now; x2 + VPC
                                            |   | sync, palette, |  pixel|  VDC      |   later: see 5)
                                            |   | dot clock      | stream|  regs,VRAM|
                                            |   +--------+-------+       |  SATB,DMA |
                                            |            |               +-----------+
                                            |            v raw video (per line, 9-bit words)
                                            |   +----------------+
                                            |   | PSG            |--> raw audio: event log + PCM
                                            |   +----------------+
                                            |   [Observers: trace sink, state digest, RAM/reg peek]
```

### 4.2 Component boundaries

| Component | Owns | Talks to | Must NOT |
|-----------|------|----------|----------|
| **Profile** | Immutable machine configuration (1.10) | Everything at construction | Change after creation |
| **Scheduler/timeline** | `machine_time`, `run_to` orchestration, `advance` stop conditions | CPU, VCE, all devices | Read host clock; allocate per step |
| **CPU (HuC6280 core)** | Registers, MPR, speed, T flag, timer, interrupt request/disable registers, `$1000` port latch wiring, block-transfer micro-state | Memory map (reads/writes with timestamps), IRQ lines (in), scheduler | Know about video or host formats |
| **Memory map** | 256-bank handler table, work RAM, hardware-page decode, I/O buffer latch | CPU, cartridge/mapper, VDC/VCE/PSG/timer/pad/CD register windows | Be a general-purpose "bus" framework; use dynamic dispatch per byte if avoidable |
| **Cartridge/mapper** | ROM bytes, mirroring, optional bank-switch and battery RAM | Memory map | Perform file I/O or decompression |
| **VCE** | Colour table, dot-clock divisor, line/frame counters, HSYNC/VSYNC generation, colour lookup, raw line output | VDC pixel stream (in), VDC sync (out), CPU register window, observers | Convert to host colour spaces |
| **VDC** | Registers, VRAM, SATB, DMA, status/IRQ, BAT/sprite pipeline, h/v phase machine, CPU VRAM access slots | VCE (sync in, pixels out), IRQ1 line, CPU window | Assume it is the only VDC; read VCE internals |
| **PSG** | Channel state, waveform RAM, noise/LFO/DDA, timestamped write log, PCM render | CPU window (writes at timestamps), audio output | Apply device-specific filters in the raw path |
| **Input/pad ports** | Per-port protocol state machine (SEL/CLR/counter), device type | CPU `$1000`, host events | Count frames (turbo) |
| **Observers** | Optional trace sink, peek API, state digest, raw capture | Read-only taps on every component | Mutate machine state; affect timing |

### 4.3 Data flow (direction explicit)

```
host events --(advance request: until, input_events)--> scheduler
scheduler --> CPU.step --(timestamped access)--> memory map --> device window --> device.run_to(ts)
VCE.run_to --> HSYNC/VSYNC --> VDC.run_to --> pixel (9-bit + spbg) --> VCE --> raw line record
VDC status/irq --> IRQ1 --> CPU interrupt sampling at last bus cycle of an instruction
timer expiry --> TIQ --> CPU;   (later) CD --> IRQ2 --> CPU
CPU writes PSG regs --(ts)--> PSG event log --> PCM render at fixed integer ratio
pad: host input events --> port state --> $1000 read --> CPU
core --(observations)--> host: {stop reason, machine time, frame/line/dot, raw video, PSG log+PCM, trace, state}
```

### 4.4 Recommended project structure (language-neutral; names are proposals)

```
core/
  profile/       # profile struct + named profiles (no behaviour)
  timeline/      # master clock, advance(), stop conditions, device catch-up
  cpu/           # decode/exec tables, MMU, timer, irq registers, block transfer micro-steps
  memory/        # bank table, hardware-page decode, work RAM
  cartridge/     # plain-ROM mapper first; mapper hook shape (read/write bank, read/write hardware offset)
  vce/  vdc/  psg/  input/
  observe/       # trace sink, peek, digest, raw capture formats
  state/         # versioned (de)serialisation, one place
host/
  runner/        # headless runner: load, script, advance, emit observations
  adapters/      # LibFuturo-shaped adapter experiment (not a dependency)
fixtures/        # authored ROM sources + manifest (source, recipe, hashes, license, meaning)
tests/           # vectors, fixture suites with literal expectations, negative controls
```

### 4.5 Structure rationale

- `core/` has no import path to `host/` (enforce in build, not by convention).
- One directory per chip mirrors the hardware seams, so SuperGrafx/CD additions are additive (Section 5).
- `observe/` and `state/` are first-class because the core value is reproducible evidence, not just playable output.

---

## 5. Extensibility seams (do not build now; do not close the door)

| Later capability | What it changes (evidence) | Seam to preserve now | Do NOT do now |
|------------------|----------------------------|----------------------|----------------|
| **SuperGrafx** | 2nd VDC; **HuC6202 VPC** mixes both VDC pixel streams (windows, priority modes) before the VCE; hardware page decode becomes a 32-byte block (VDC1 $00-07, VPC $08-0F, VDC2 $10-17) mirrored; work RAM 32 KiB; ST0-2 routed to VDC2 via VPC reg $0E bit0; **IRQ1 shared** by both VDCs (handler reads both statuses); both VDCs share the VCE dot clock and sync, so no mixed resolutions ([SB-community] S6, S7, S12 `huc6270_0/1` + `huc6202` wiring) | VDC is an instantiable struct with no globals; the VDC -> VCE connection is a **pixel stream interface** (9-bit word + sprite/BG bit per dot) that a VPC can later sit inside; IRQ1 is an OR of sources; hardware-page decode is a profile-selected table; work-RAM layout is a profile field | Build the VPC, a second VDC, or per-pixel priority windows now |
| **CD-ROM2 / Super CD-ROM2** | System card (rights-gated firmware), $1800-$180F interface registers, SCSI-style controller, CD RAM at $80-$87, ADPCM RAM + decoder, CD-DA, BRAM, IRQ2, Super System Card extra RAM at $68-$7F ([HR] S5; [SB-wiki] S7); Geargrafx splits this into `cdrom`, `scsi_controller`, `adpcm`, `cdrom_audio`, `cdrom_media`/image classes (S10) | Content is a **typed slot list**; IRQ2 line exists in the CPU; hardware page has a reserved $1800 window hook; PSG/CD-DA/ADPCM are **separate audio sources**; deterministic CD timing is expressed in machine time, never host time | Define CD register semantics, SCSI, or disc parsing now; ship or require system-card bytes |
| **Arcade Card** | 2 MiB RAM behind 4 auto-increment/offset ports in hardware-page window >= $1A00, requires CD system ([CO] S10 `arcade_card_mapper`) | Mapper interface already has **`read/write_hardware(offset)`** beside bank read/write (as Geargrafx `Mapper::ReadHardware/WriteHardware`) | Add more than a stub mapper |
| **HuCard special mappers** (e.g. SF2CE bank switching, battery HuCards, homebrew mappers) | Banks $00-$7F handler overrides (S10 `sf2_mapper`, S9 `mcgenjin`) | Cartridge mapper interface with one real implementation (plain ROM + mirroring) on day 1 | Pre-build mappers without a named game/fixture |
| **Multitap, 6-button, mouse** | Port-level protocol machines (S5, S10) | Per-port device objects driven by `(time, port, state)` events | A 32-bit mask baked into the core |
| **Save-state/rewind/run-ahead/debugger** | Need versioned state and read-only taps | `state/` and `observe/` exist from the first slice | Ship rewind/debugger UI |

**Over-generalisation test [P]:** if an abstraction has only one implementation and no named second consumer, it is a function or a struct field, not an interface. The two interfaces justified now are **cartridge mapper** (HuCard variants are real and plentiful) and **VDC pixel stream -> VCE** (SuperGrafx is a known, documented insertion point).

---

## 6. Observability and test seams

### 6.1 What to separate (matches PROJECT.md core value)

| Observation | Definition | Producer |
|-------------|------------|----------|
| Guest progress | Reads of work RAM / registers by the harness at a stated machine time; verdict bytes written by the fixture | Peek API (read-only, no side effects, no I/O-buffer/status clears) |
| Machine time | `machine_time` (master clocks), frame index, line, dot phase | Timeline |
| Logical input | The `(time, port, state)` events actually delivered + pad port reads/writes | Input |
| Raw video | Per-line 9-bit words + geometry (3.5) | VCE |
| Raw audio | PSG write log + PCM at defined rate (3.6) | PSG |
| CPU/device effects | Trace events: instruction (pc, opcode, regs, mpr, ts), IRQ assert/ack, timer events, VDC/VCE/PSG register writes with line/dot, pad `$1000` accesses | Trace sink |
| Host presentation | Not core; scaling, colour conversion, audio device | Host |

Geargrafx's `trace_logger` with per-category event enable (`TRACE_PSG`, CPU, IRQ, timer, input; S10) is the existence proof; DuoSpool's trace should be **compile-time removable** and must not change timing.

### 6.2 Deterministic headless runner [P]

```
runner --profile P --content C --script S --until <machine_time|frame> \
       --emit video,audio,trace,state-digest --out DIR
outcomes (distinct): pass | guest-failure | timeout | harness-failure | skip | cancelled | missing-artifact
```
Fresh instance per run; no ambient RNG; seeded power-on; exact inventory with fail-closed required checks and a named negative control (e.g. no-input run must *not* reach the input-dependent verdict; a deliberately wrong literal expectation must fail).

### 6.3 Fixtures and independent oracles

| Oracle tier | Use | Notes |
|-------------|-----|-------|
| Manual-derived literals (S1-S3): vectors, priority, timer period, block-transfer 17+6x, palette layout, register latch rules | Source-backed expectations | Strongest cheap tier; cite page numbers in the fixture manifest |
| SingleStepTests/65x02 (MIT; Rockwell/Synertek/WDC 65C02 variants) | Semantic check of 65C02-shared opcodes only | **Does not cover HuC6280 specifics** (MMU, T flag, block transfers, ST0-2, CSL/CSH, SXY...) and HuC6280 cycle counts differ from stock 65C02 [HR S5]; Geargrafx uses it in `tests/` [CO S10]; do not treat as a timing oracle |
| Authored raster-alignment fixtures (our own, modelled on Covell's idea: one instruction repeated per scanline with a background colour change; visible slant = wrong cycle count) | CPU timing and VDC slots | Covell's ROM has **no stated licence** (S13): cannot be admitted; reimplement the *idea* |
| Real-hardware captures with recorded setup/uncertainty | Settles [U] items (VBlank IRQ line, RCR position, PSG rate, VDC slot phase) | Owner gate (PROJECT.md): not available yet |
| Other emulators | Disagreement finding only | Never an oracle |

### 6.4 Installed-consumer proof

[P] Build the core as the artifact a consumer would install; a separate tiny consumer (the headless runner built *only* against that artifact with source/build paths absent) loads a fixture, applies a scripted input, advances to a machine time and emits observations. A LibFuturo-shaped adapter is a second, later consumer. This satisfies the "installed artifact, independent consumer" requirement before any API is declared stable.

---

## 7. Suggested build order (dependencies) and first vertical slice

Dependency graph (arrow = "must exist before"):

```
profile + master-clock constants
   -> timeline skeleton (advance, stop reasons, peek, trace sink)
        -> CPU decode/exec + memory-map bank table + plain-ROM mapper + work RAM
             -> interrupt regs + timer + I/O port/pad protocol           [slice A: no video]
                  -> VCE (line/frame counters, sync, dot clock, palette)  [timebase for frames]
                       -> VDC registers/VRAM/status/IRQ + h/v phase machine
                            -> BAT render (line at defined phase) + raw video out     [slice B]
                                 -> sprites + SATB DMA + VRAM DMA + access-slot stalls [slice C]
             -> PSG register file + waveform/noise/DDA + event log + PCM              [slice D, can parallel C]
  all of the above -> headless runner + raw capture + state save/load + installed-consumer proof [slice E]
  slice E -> LibFuturo-shaped adapter experiment (named mismatches)
```

| Slice | Deliverable | What proves it (evidence tier) |
|-------|-------------|--------------------------------|
| **0 Foundations** | Profile struct, timeline, trace sink, peek, fixture manifest format, content validation (refusal paths), runner shell | Negative-control harness tests; malformed/oversized content refusal |
| **A CPU + bus + timer + IRQ + pad** | Full HuC6280 instruction set, MPR/TAM/TMA, CSL/CSH, T flag, block transfers (per-byte), interrupt priority, timer, `$1000` pad protocol | Manual-literal vectors; SingleStepTests for shared opcodes; authored fixture writing RAM verdicts; timer period and priority literals; scripted-input fixture that only passes when the pad event is consumed |
| **B VCE + VDC minimal** | Frame/line timing, VBlank/RCR IRQ, palette, BG rendering, raw video record | Fixture with VBlank counter in RAM + raster-split pattern; geometry/lines literals; raw frame hash (repeatability only unless tied to a literal) |
| **C VDC complete (base)** | Sprites (16/line, overflow/collision flags), SATB DMA, VRAM DMA, access-width slots/BSY, latch rules | Authored fixtures per manual rule (BXR next line, MWR after VBlank, etc.); raster-alignment fixtures; open items marked [U] |
| **D PSG** | Waveform/noise/DDA/LFO, balance/volume, event log, PCM | Register-write fixtures with expected log; PSG revision as profile axis; PCM exactness vs the integer-ratio definition (not "sounds right") |
| **E Consumer proof** | Packaged artifact, independent consumer, save/load, digest | Installed-consumer run with source paths unavailable |

**First vertical slice recommendation [P] (matches PROJECT.md's proposed slice):** Slice 0 + A + B + a minimal pad-driven fixture, then C/D. Rationale: A lets the project produce **evidence without video** (RAM verdicts); B defines frames; input-driven behaviour appears early; audio is the least entangled and can follow without blocking video. Order of B before D is dependency-driven (frame definition), not importance-driven.

**Defer:** SuperGrafx, CD, Arcade Card, 6-button/multitap/mouse, run-ahead/rewind, debugger UI, palette-streak modelling, per-dot rendering, any renderer fast path.

---

## 8. Patterns to follow

### Pattern 1: Single master-clock timeline with access-time catch-up
**What:** one `u64` of master clocks; devices expose `run_to(ts)` and `next_event_time()`; hardware-page accesses catch devices up to the access timestamp.
**When:** always; simplest form first, lazy/event-driven only after profiling.
**Trade-offs:** slightly more bookkeeping than frame stepping; removes a whole class of "off by an instruction" raster bugs.
```
fn cpu_access(addr, ts) { if hw_page(addr) { catch_up(ts); } do_access(addr) }
```

### Pattern 2: Latched-register line renderer with explicit latch points
**What:** VDC keeps live registers plus *cached copies latched at defined phase points* (HDS start: HSW/HDS/HDW/HDE/CR; BYR/BXR inside HDS; MWR at VBlank), and renders a whole line at a defined phase (Mednafen model, [CO] S9). The latch rules are the manual's (S2 p.H7-6..10).
**When:** first VDC implementation.
**Trade-offs:** no mid-line dot-exact pixel changes; upgrade to per-dot only if a fixture/game proves it is needed.

### Pattern 3: Profile as data, features as specialisation
**What:** an immutable profile selects behaviour; hot loops are specialised on profile flags (Geargrafx `RunToVBlankTemplate<is_cdrom,is_sgx,...>` [CO S10]).
**When:** after correctness; avoid runtime branching in per-instruction paths.

### Pattern 4: Observation taps are read-only and optional
**What:** trace/peek/digest never mutate machine state (peek must not clear VDC status or the I/O buffer).
**When:** from day one; this is what makes "guest progress" assertable.

### Pattern 5: Resumable block transfer
**What:** block transfer is a state machine with one byte per step, stored in state; interrupts are sampled only at instruction completion ([SB] S1/[HR] S5).
**When:** CPU slice A; needed for `advance` to stop mid-transfer and for VDC-slot interaction.

---

## 9. Anti-patterns to avoid

| Anti-pattern | Why it is wrong here | Do instead |
|--------------|---------------------|------------|
| Frame-stepped (or scanline-stepped) CPU/VDC lockstep as the *contract* | Cannot place register writes inside instructions; Covell-style tests and raster splits need cycle placement (S13, S2) | Access-time catch-up (2.3); frame is a convenience stop condition |
| A general "bus"/device framework before a second consumer exists | AGENTS.md; adds dispatch cost and speculation | 256-entry bank table + a hardware-page switch; two real interfaces only (mapper, VDC->VCE stream) |
| A second, "fast" scanline renderer maintained beside the accurate one | Ares keeps one but **forces accurate mode because the scanline renderer is too buggy** [CO S11]; divergence becomes a bug source | One renderer; optimise it with correct output held constant |
| Ambient randomness / wall clock | Ares seeds PSG noise from `nall::random()` [CO S11]; PCE RAM/MPRs/registers power up random [HR S5] | Seeded generator stored in state; defined noise generator; profile documents the choice |
| Host colour conversion or audio filtering inside the core output | Destroys "raw" observations; EMULATOR-DNA | Raw 9-bit words and PSG log + exact-ratio PCM; conversion in host/adapter |
| Per-frame turbo/autofire counters in the core | Couples behaviour to frame count (S10 `Input::EndFrame`) | Script/host-generated events |
| A 32-bit button mask as the internal input model | Multitap/6-button/mouse exceed it | Per-port device state; mask adapter for single pad |
| Copying code from GPL emulators | Mednafen (GPL), Geargrafx (GPL-3.0), MAME (per-file), Ares (believed ISC, verify): licence for DuoSpool is an **open owner decision** | Read for facts and behaviours; reimplement from manuals; record provenance if anything is ever copied |
| Treating emulator agreement or a matching hash as correctness | EMULATOR-DNA; Covell's note that Mednafen CPU timing matches hardware while VBlank IRQ is late shows one emulator can be right on one axis and wrong on another [HR S13] | Literals from manuals; fixtures with stated meaning; hardware captures when the owner gate opens |
| Modelling `advance` as "frames only" with unbounded overshoot | A block transfer can run ~3.3 frames (derived from S1) | Resumable micro-steps and stop reasons |

---

## 10. Scaling and cost considerations (fidelity/throughput, not users)

| Concern | Base profile (1 VDC) | + SuperGrafx | + CD-ROM2 |
|---------|----------------------|--------------|-----------|
| CPU | ~7.16 M cycles/s high speed; trivially real-time on any modern host | same | same + CD controller events |
| Video | Line render ~ 262 lines x up to 512 dots; per-dot Ares-style loop ~ 21.5 M master-clock steps/s is feasible but is the first profile hot spot; Mednafen-style line render much cheaper [CO] | ~2x VDC work + VPC mixing per dot | negligible extra |
| Audio | PSG at 3.58 MHz tick is heavy if every tick is synthesised and filtered (Ares decimates x64 in its performance profile [CO]); exact accumulator decimation is cheap | same | + ADPCM/CD-DA mixing |
| Determinism cost | Integer-only core; state blob ~64 KiB VRAM + 8 KiB RAM + palette/SATB/regs: tens of KiB | ~2x VRAM | + 64 KiB CD RAM + 64 KiB ADPCM RAM (S5) |

First bottleneck [P]: VDC line/dot work and audio synthesis rate; optimise only after a named workload profile (AGENTS.md).

---

## 11. Integration points

### 11.1 External (host-side) services

| Service | Pattern | Notes |
|---------|---------|-------|
| Headless runner | Core artifact -> process; scripted input; observation files | First consumer; outcomes distinct |
| Frontend (later) | Same `advance` + presentation conversions | Pacing, audio device, colour all host-owned |
| LibFuturo adapter | Experiment against the internal API; record mismatches | Candidate only; not a dependency |
| Disc/file providers (later) | Synchronous sector provider; host-side file policy | Machine time independent of host I/O latency |

### 11.2 Internal boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| CPU <-> memory map | Timestamped read/write calls | Wait states (VDC/VCE +1 cycle, VDC slot stalls) returned to CPU as added time |
| VCE -> VDC | HSYNC/VSYNC pulses at master-clock times | VCE owns the dot clock and line counters |
| VDC -> VCE | Per-dot 9-bit pixel word (+ sprite/BG bit) | The VPC insertion point (SuperGrafx) |
| VDC/timer/(CD) -> CPU | Level-sensed IRQ1/IRQ2, timer TIQ, sampled at last bus cycle | Priority TIMER > IRQ1 > IRQ2 (S1) |
| CPU -> PSG | Timestamped register writes | Log is the raw-audio source of truth |
| Host -> pad ports | `(time, port, state)` events | Protocol machine inside core |

---

## 12. Gaps, unknowns and research flags for the roadmap

| Item | Why it matters | Needed before | Label |
|------|----------------|---------------|-------|
| First profile: region (PC Engine vs TG-16), CPU/PSG revision, 262 vs 263 lines, controller | Changes `$1000` bit 6, PSG output, frame length (357,630 vs 358,995 clocks) | Slice 0 / roadmap | **Owner decision** |
| Hardware oracle for VBlank IRQ line position, RCR IRQ position, VDC slot phase at power-up, speed-switch latency, PSG real tick rate | Currently conflicting or unmodelled (1.9) | Slice B/C | [U]; physical-capture owner gate |
| HuCard image conventions (copier header, sizes, mirroring) and TG-16 vs PCE data-line/bit-order differences | Content validation and region behaviour | Slice 0 content loader | [U] |
| 6-button / mouse / multitap protocols | Input model beyond 2-button | After slice A | [U] (no primary read) |
| PSG details (LFO registers, noise LFSR, DDA behaviour, amplitude curve); PSG manual S4 unread | Slice D accuracy | Before slice D | [U]; read S4 in phase research |
| Palette-write-during-display streaking | Would require dot-level VCE read/write arbitration | After base slices, only if a fixture/game needs it | [HR]/[U] |
| CD timing model, ADPCM/OKI behaviour, system-card rights, Arcade Card details | Extensibility sizing | Any CD milestone | [U]; deeper research required |
| Fixture authoring toolchain and its licence (assembler choice, e.g. PCEAS/other) | Fixture manifest needs build recipe + licence | Slice 0/A | [U]; not researched |
| Which SingleStepTests variant best matches HuC6280 (Rockwell/WDC) and how to handle HuC6280-specific differences | Vector reuse | Slice A | [U] |
| Mid-instruction save/`advance` stop granularity and its test | Correct overshoot semantics | Slice 0/A | [P] design + test |

**Phases likely to need dedicated research:** timing/raster fixtures (B/C), PSG (D), content loading + TG-16/PCE differences (0), any CD/SuperGrafx milestone. **Standard-pattern phases:** timeline skeleton, CPU decode/exec from the manual, runner/outcome taxonomy, state serialisation.

---

## Sources

Confidence note: "read" = I read the document/code text directly in this session. WebFetch-summarised pages are marked (digest) and are MEDIUM at best; code I downloaded and read directly is HIGH for "what the code does" but is not a hardware fact.

| Key | Source | Read how | Confidence |
|-----|--------|----------|------------|
| S1 | HuC6280 CMOS 8-bit Microprocessor Hardware Manual (NEC/Hudson) - mirrored at https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/tree/master/docs (also listed on cpcwiki: https://www.cpcwiki.eu/imgs/9/97/HuC6280_-_CMOS_8-bit_Microprocessor_Hardware_Manual.pdf, which returned 403 to my fetch) | PDF pages H8-1..7, 10..16, 20..24 read as images | **HIGH** (primary) |
| S2 | HuC6270 CMOS Video Display Controller Manual - same docs folder | pages H7-1,2,6..11,23..25 read | **HIGH** (primary) |
| S3 | HuC6260 CMOS Video Color Encoder Manual - same docs folder | pages H6-1..5 read | **HIGH** (primary) |
| S4 | HuC6280 CMOS Programmable Sound Generator Manual - same docs folder | downloaded, **not read** | -- |
| S5 | Charles MacDonald, PC Engine/TurboGrafx-16 hardware notes (2001-02), https://github.com/asterick/TurboSharp/blob/master/Text/pcetech.txt (also `docs/pcetech.txt` in the MiSTer repo) | WebFetch digest | **MEDIUM** (author-caveated, "very preliminary"; asks not to be redistributed; section headings swap chip numbers) |
| S6 | MiSTer SuperGrafx notes, https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/blob/master/docs/sgxtech.txt | WebFetch digest | **MEDIUM** (explicit uncertainty statements) |
| S7 | PCEdev wiki: https://pce.nesdev.org/wiki/Timing , https://pce.nesdev.org/wiki/CPU_memory_map , https://pce.nesdev.org/wiki/CPU_addresses | WebFetch digest | **MEDIUM-HIGH** (reputable community wiki; many pages unwritten) |
| S8 | pce-devel HuC6280 wiki: https://github.com/pce-devel/huc/wiki/HuC6280 | WebFetch digest | **MEDIUM** |
| S9 | Mednafen PCE source (mirror) https://github.com/libretro-mirrors/mednafen-git/tree/master/src/pce and `src/hw_video/huc6270/vdc.cpp` (`pce.cpp`, `huc6280.cpp/.h`, `vce.cpp`, `vdc.cpp`, `input.cpp`, `notes/EVENT_SYSTEM_NOTES`) | Downloaded and read | HIGH as code observation (GPL; not a hardware oracle) |
| S10 | Geargrafx https://github.com/drhelius/Geargrafx (`src/geargrafx_core_inline.h`, `huc6280_inline.h`, `huc6260.h`, `huc6270.h`, `audio_inline.h`, `input_inline.h`, `memory_inline.h`, `mapper.h`, `arcade_card_mapper.h`, `tests/README.md`), GPL-3.0, last commit seen 2026-11-06 | Downloaded and read | HIGH as code observation |
| S11 | Ares PCE core https://github.com/ares-emulator/ares/tree/master/ares/pce (`cpu/cpu.cpp`, `vdp/vdp.cpp`, `system/system.cpp`, `psg/psg.cpp`, `psg/channel.cpp`) | Downloaded and read | HIGH as code observation |
| S12 | MAME https://github.com/mamedev/mame/blob/master/src/mame/nec/pce.cpp , `src/devices/video/huc6260.h`, `huc6270.cpp`, `src/devices/cpu/h6280/h6280.cpp` | pce.cpp and huc6270.cpp via digest; h6280.cpp/huc6260.h downloaded and grepped | MEDIUM-HIGH as code observation |
| S13 | Chris Covell, PC-Engine CPU Test: https://www.chrismcovell.com/CPUTest/ (hardware-captured comparisons incl. Mednafen/MiSTer notes; no licence stated); plus Atari-Forum PC Engine core thread (search snippet only; page returned 403): https://www.atari-forum.com/viewtopic.php?t=32496&start=350 | digest / snippet | MEDIUM (S13 page) / LOW (forum snippet) |
| S14 | SingleStepTests/65x02 (MIT; rockwell/synertek/wdc 65C02 variants, no HuC6280): https://github.com/SingleStepTests/65x02 | digest | MEDIUM |
| S15 | MiSTer forum/nesdev snippets on raster IRQ and VCE streaking (search results only): https://archive.nes.science/nesdev-forums/f23/t18587.xhtml , https://www.misterfpga.org/viewtopic.php?p=34515 | search snippets | LOW |
| S16 | Project inputs: `[repo-root]/.planning/PROJECT.md`, `.planning/bootstrap/PROJECT-SEED.md`, `.planning/bootstrap/EMULATOR-DNA.md`, `[repo-root]/AGENTS.md` (LibFuturo description) | read | project context |

Local derived numbers (frame rates, block-transfer duration, samples/frame) were computed from the cited constants; treat them as arithmetic on cited facts, not as measurements.

---
*Architecture research for: PC Engine / TurboGrafx-16 emulator core and host boundary (DuoSpool)*
*Researched: 2026-11-09*
