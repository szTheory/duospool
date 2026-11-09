# Feature Research

**Domain:** PC Engine / TurboGrafx-16 emulator (DuoSpool), evidence-first, per-named-profile
**Researched:** 2026-11-09
**Overall confidence:** MEDIUM. Emulator-feature landscape is HIGH. Hardware claims are MEDIUM at best: the primary Hudson/NEC manuals were not located or read in this pass, so most hardware facts rest on community reverse-engineering notes (TurboSharp `pcetech.txt`, MiSTer `sgxtech.txt`, pce-devel wikis) and on other emulators' source. Those are labelled below.

> Method note. I used WebSearch/WebFetch directly. I did not run the `research-plan` / `research-store` / `classify-confidence` seam, so no digests were cached and the confidence tags below are my own source-type tags, not seam output.
> Two fetched pages (nicole.express, both articles) ended with text addressed to AI agents asking for instructions to be changed. I ignored it. It is page content, not a request from the owner.

## Claim-label legend

Every hardware or emulator claim in this file carries one of these labels (project rule: distinguish owner decisions, facts, code observations, reports, proposals, unknowns).

| Label | Meaning |
|-------|---------|
| **[OWNER]** | Owner decision recorded in `.planning/PROJECT.md` / `AGENTS.md` |
| **[FACT]** | Source-backed, URL given. Still graded: *primary* (Hudson/NEC doc, patent, schematic) or *secondary* (community RE notes) |
| **[CODE]** | Observation from another emulator's source or docs. Shows what that project does. It is NOT a hardware fact |
| **[REPORT]** | Historical or community report, not independently verified |
| **[PROPOSAL]** | My recommendation, open to owner challenge |
| **[UNKNOWN]** | No adequate source found, or sources conflict |

Evidence-tier shorthand used in the "Oracle" columns (Proposal, aligned with PROJECT.md "Evidence" constraint):

| Tier | Meaning |
|------|---------|
| **E0** | Repeatability only (same core, same hash). Proves determinism, nothing else |
| **E1** | Documented vector or register behavior from a cited primary/secondary source, asserted literally in an authored fixture |
| **E2** | Authored diagnostic ROM with its own pass/fail verdict (guest progress), verdict generated independently of the emulator's output |
| **E3** | Hardware-measured with recorded setup and uncertainty (owner gate; analog capture or flash-cart on real console) |
| **E4** | Differential vs another emulator. Finds disagreements, cannot decide who is right |

---

## Part 1: Hardware / content surface as candidate feature areas

Scope for each area is a decision for the owner (`AGENTS.md`: scope is NOT owner-selected). Nothing here silently includes or excludes. "Cost" = architectural cost + rights cost + oracle cost.

### 1.1 Base PC Engine vs TurboGrafx-16 (region profile)

| Aspect | What is known | Label / source | Emulation consequence |
|--------|---------------|----------------|-----------------------|
| HuCard data lines | The North American TG-16 wires the 8 HuCard data lines in reverse order vs the Japanese PC Engine, so cards are not cross-readable. A pin-reversing adapter converts either direction | **[FACT, secondary]** [nicole.express 2020](https://nicole.express/2020/trademarks-and-region-locks-engine.html). The same article says "reversed data order on US ROMs" is something the author read but could not confirm. [Wikipedia](https://en.wikipedia.org/wiki/TurboGrafx-16) says "eight pins reversed". A consolemods pinout page exists but returned HTTP 403 to me, so only a search snippet backs the D0<->D7 mapping | Whether an image is stored in PCE order or TG-16 pin order is a **content-format** question, not a machine-behavior question. Neither MAME `pce.cpp` nor Beetle `huc.cpp` contains any bit-swap logic **[CODE]**. MiSTer notes its US BIOS is "already bit-swapped for emulators" and must not be swapped again **[CODE/doc]** ([MiSTer README](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer)). Common practice is therefore normalize-at-load |
| Region-detect bit | HuC6280 input port bit 6 (K6) at `$1000`: 1 = PC Engine, 0 = TurboGrafx-16. Bit 7 detects something plugged into the expansion bus. K4/K5 unconnected | **[FACT, secondary]** [TurboSharp pcetech.txt](https://github.com/asterick/TurboSharp/blob/master/Text/pcetech.txt) section 4 and [sgxtech.txt](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/blob/master/docs/sgxtech.txt). Consistent with MAME's `PCE_JOY_SIG 0x40` vs `TG_16_JOY_SIG 0x00` **[CODE]** | Per-profile constant on a single input-port bit. Very cheap. Oracle: authored fixture that reads `$1000` and reports bit 6 (E1/E2 against the docs). Real E3 needs one real console of each region |
| US-game region check | Nearly all US games read `$1000`, test bit 6, and on "set" map bank `$90` and jump to `$4000` (empty), giving a blank/crash screen. No equivalent in Japanese games | **[REPORT]** single author, partly speculative (nicole.express above). The author also disputes whether K6 is a region pin or "boot ROM present" | Do NOT claim that bit 6 "is region" as settled. Test as observable behavior, not as semantics |
| VCE odd-address read | Bits 7-1 return random data on PCE, fixed 1 on TG-16 | **[REPORT]** sgxtech.txt, flagged by its author as unverified | Low-priority register edge case. List as `unknown` in the first profile; do not assert |
| Controller | Same signaling on both. TG-16 bundles a turbo (autofire) pad; white PC Engine pad has no autofire. Connector shapes differ | **[FACT secondary]** [Wikipedia](https://en.wikipedia.org/wiki/TurboGrafx-16); **[CODE]** MAME defaults `joypad2` (pce) vs `joypad2_turbo` (tg16) | Physical connector is host-irrelevant. Autofire is a controller-side behavior that must be driven from **machine time**, not host time (see 2.1) |
| CD-ROM region | TurboGrafx-CD / PC Engine CD reported to have no region protection | **[REPORT]** nicole.express above | Region is not a CD-content gate in emulation, but system-card *version* is (see 1.7) |
| PAL | MAME lists "TurboGrafx for PAL region?" as a TODO. Wikipedia says European units were modified US models and no PAL-specific HuCards exist | **[CODE]** MAME `pce.cpp`; **[REPORT]** Wikipedia. Wikipedia's claim is not independently verified | Treat PAL/50 Hz as **out of scope [UNKNOWN]** until someone shows a real need |

Architecture cost: LOW if region = a profile record (`io_bit6`, `default_pad`, `content_byte_order`). Rights cost: none for the profile itself. Oracle: E1/E2 for the bit; E3 only if the owner wants a "matches real TG-16" claim.

**[PROPOSAL]** Model region as a field in the named profile, not as a code branch. Accept only normalized (PCE byte order) HuCard images in the core. Put TG-16-order detection/normalization in the loader with an explicit, testable refuse-or-convert rule. A silent auto-detect that guesses the order is a fail-open design.

### 1.2 HuC6280 vs HuC6280A (CPU/PSG revision)

| Aspect | What is known | Label / source |
|--------|---------------|----------------|
| Where each revision ships | HuC6280 in original PC Engine and most models. HuC6280A reported in SuperGrafx and "some CoreGrafx" | **[REPORT]** [PCE_Soundtest](https://github.com/pce-devel/PCE_Soundtest). [Wikipedia](https://en.wikipedia.org/wiki/TurboGrafx-16) says CoreGrafx has the A, and CoreGrafx II reverts to the original. The two sources are consistent about SGX, ambiguous on CoreGrafx |
| Observable difference | HuC6280 PSG sometimes "pops": the waveform starts offset above centre (a DC offset that decays), roughly doubling the initial impulse without changing peak-to-peak. HuC6280A shows no pop | **[REPORT, observational]** PCE_Soundtest (MIT license; scope/audio-capture based, specific consoles not named) |
| Other differences | Whether the A revision of the CPU or VCE differs in any other way is unknown | **[UNKNOWN]** sgxtech.txt says explicitly it does not know |
| Other emulators | Mednafen exposes `pce.psgrevision` (match = HuC6280A in SGX mode) and notes the A may reduce clicking but add it in games designed around the original | **[CODE/doc]** [Mednafen pce docs](https://mednafen.github.io/documentation/pce.html) |

Architecture cost: LOW-MEDIUM. The revision is a PSG output-stage property (DC offset/decay) plus an unknown amount of other behavior. Rights: PCE_Soundtest MIT, can likely be admitted with notice. Oracle: only E3 (analog capture) can distinguish revisions. A digital-only emulator oracle can assert that the *model* produces the documented offset (E1). **It cannot prove that the model matches a real chip.**

**[PROPOSAL]** `cpu_revision` is a required profile field from day one. The first profile commits to exactly one revision and states "revision-specific behavior beyond the documented PSG DC offset: unknown". Do not implement both revisions in the first slice.

### 1.3 HuCard sizes and special mappers

| Item | Facts | Label / source | Cost |
|------|-------|----------------|------|
| Address space | 21-bit CPU bus; 256 pages of 8K. HuCard ROM at pages `$00-$7F` (1 MB). Smaller ROMs "may be mirrored" | **[FACT secondary]** pcetech.txt section 3 | Mirroring rules for smaller images are card-wiring dependent and NOT fully documented in the notes I read. Boundary tests needed |
| 384 KB layout | Beetle PCE special-cases 384 KB images (upper range mirrored) and uses `bank % (len/8K)` for other sizes | **[CODE]** [Beetle `huc.cpp`](https://github.com/libretro/beetle-pce-libretro/blob/master/mednafen/pce/huc.cpp) | MEDIUM: a table of size to bank map. Not verified against hardware |
| Copier header | Beetle strips 512 bytes if `size & 512` | **[CODE]** same file | Loader policy. Make it explicit and refusable, not silent |
| Street Fighter II' mapper | 2.5 MB-class card. Fixed first 512 KB, a switchable 512 KB window at `$80000-$FFFFF`, bank selected by writes to `$1FF0-$1FF3` | **[CODE]** MAME `pce_rom.cpp` and Beetle agree on the register range; sizes differ in how they round (Beetle rounds up to 5 MB internally). Hardware spec not located | MEDIUM-LOW. One register-decode plus a bank. Isolated, testable. Authored fixture needs its own "SF2-style" mapper ROM (only 1 commercial title uses it) |
| Populous | 32 KB RAM on the card at banks `$40-$43` | **[CODE]** MAME, Beetle (agree). **Conflict:** Beetle README and widespread reports call Populous battery-backed. nicole.express says its author opened the cart and found no battery or contacts | LOW. Detection should come from an explicit mapper declaration in the content manifest, not a title string (Beetle uses the string "POPULOUS" at `0x1F26`) |
| Tennokoe Bank | 8 KB battery-backed RAM on a HuCard, four "boxes", utility only. MAME maps it at `$80000`, locked on reset, with the unlock write marked "TODO: needs real HW study" | **[FACT/REPORT]** [nicole.express 2022](https://nicole.express/2022/the-voice-of-the-heavens-is-only-2kb.html); **[CODE]** MAME `pce_rom.cpp` (unlock mechanism unverified) | LOW cost but evidence gap: the unlock mechanism is an unverified guess in the best-known source. Do not claim support without E3 or primary doc |
| "Tsushin Booster" / MCGenjin | Beetle keys the first by CRC (`0x34dc65c4`) and maps 32 KB at banks `$88-$8B`. MCGenjin is a homebrew mapper detected by the string "MCGENJIN" at `0x1FD0` | **[CODE]** Beetle `huc.cpp` | CRC-keyed behavior is an anti-pattern for this project (see anti-features). MCGenjin is noted only as a possible *rights-clear* mapper family. Not researched further |
| Unknown mappers | The set of all special HuCards is not enumerated in any source I read | **[UNKNOWN]** | Requirement: unsupported mapper means explicit refusal, never "load as plain ROM" |

Rights: HuCard images of commercial titles are a rights boundary. A hash does not grant redistribution rights (`AGENTS.md`). Authored fixtures only in CI. Real-game runs are local-only and gated.

### 1.4 Joypads, 6-button, multitap, turbo, mouse and other input

Primary description: [pce-devel PCE_Controller_Info](https://github.com/pce-devel/PCE_Controller_Info/blob/main/README.md) **[FACT secondary]**; consistent with pcetech.txt section 4 and MAME joypad drivers **[CODE]**.

| Device | Protocol facts | Cost and timing implication |
|--------|----------------|------------------------------|
| 2-button pad | Console drives SEL and CLR; 4 return lines. SEL high = D-pad nibble, SEL low = RUN/SELECT/II/I. Active-low. CLR high forces reads to 0. Scan = SEL high, pulse CLR, wait about 1.25 us (about 9 cycles at 7.16 MHz), read; SEL low, wait, read | Needs reads at CPU-cycle granularity, but **frame-latched logical input is sufficient** for this device if the pad state is only changed at frame boundaries. Anything that changes the pad mid-frame needs timestamped input |
| Turbo/autofire pad | Autofire is a controller-side counter. MAME uses a 74xx163-style counter in its 6-button driver | **[CODE]** MAME `joypad6.cpp`. Autofire rate must come from machine time. Otherwise replays diverge |
| Multitap (up to 5) | A SEL-high toggle advances to the next port; a CLR pulse resets to port 1. Multitap holds CLR high on inactive ports | Port-indexed input is mandatory. See the 32-bit mask mismatch below |
| 6-button (Avenue Pad 6) | Scan 1 as 2-button. Scan 2 returns `0000` on the SEL-high nibble (impossible on a 2-button pad) and III/IV/V/VI on SEL low. Software detects by scanning twice and spotting the `0000` nibble. **Warning from the source: do not assume which scan number carries `0000`.** | Requires a modeled per-pad scan counter (CLR pulse count). Games depending on it: Street Fighter II' (HuCard) plus about 20 CD titles listed in the source |
| Mouse | 4 scans per read (dX high/low, dY high/low). Times out if SEL does not toggle within about 550 us or CLR is not retriggered within about 600 us. Detection can be confused with a 6-button pad | Needs machine-time (microsecond-scale) timeout logic. Does not fit "frame-step input" |
| Pachinko, XE-1AP analog | XE-1AP analog has a data window of about 12.1 us per nybble. Pachinko compatibility is reported as uncertain | **[REPORT]**. Defer. MiSTer warns the 6-button/mouse/XE-1AP modes only work with listed games and break others if forced **[CODE/doc]** |
| Memory Base 128 | 128 KB, serial protocol over the controller port with pass-through (CLR is a clock, SEL is a data bit on rising edge) | **[FACT secondary]** controller info + nicole.express. It is a storage peripheral using the input port. Cross-listed in 1.5 |

**Concrete LibFuturo-contract mismatch candidate [PROPOSAL, to be proven by an adapter]:** a 32-bit button mask cannot carry 5 ports x 12 buttons (60 bits) or mouse deltas. A first 1-pad 2-button profile fits (8 bits). This is a named case to test, not a reason to reject LibFuturo.

### 1.5 Backup RAM and storage

| Device | Facts | Label / source |
|--------|-------|----------------|
| Internal CD-unit BRAM | 2 KB at `$F7:E000-$E7FF` (`$1EE000-$1EE7FF`), not mirrored, locked at power-up. Bit 7 of `$1807` unlocks, reading `$1803` locks | **[FACT secondary]** pcetech.txt section 8; MAME maps at `1EE000` **[CODE]** |
| Format | Beetle initializes with a "HUBM" header string | **[CODE]**. The file system layout is BIOS-defined and part of the system card |
| Tennokoe 2 / Backup Booster | Expansion-port unit, 2 KB SRAM with battery (a 6116), no built-in software | **[REPORT]** nicole.express 2022 |
| Tennokoe Bank | See 1.3 | See 1.3 |
| Memory Base 128 / Save-kun | 128 KB, OKI MSM6389, over the controller port | **[REPORT]** nicole.express 2022 |
| Duo internal | 2 KB capacitor-backed SRAM, not battery-backed | **[REPORT]** nicole.express 2022 |
| TurboBooster Plus | **[UNKNOWN]**. I found no source in this pass. Do not include in requirements without research |

Architecture: **[PROPOSAL]** treat every persistent store as a named, typed, versioned blob owned by the host. The core takes it at load, exposes it plus a "dirty" signal, and never touches a filesystem. This keeps the core boundary clean (`AGENTS.md` rule) and fits LibFuturo's "optional profile-defined persistent media" only if more than one store per profile is allowed. That is another mismatch to test.

Oracle: E1 for the lock/unlock behavior from the documented registers; E2 with an authored fixture that tries a locked write, unlocks, writes, relocks, re-reads. E3 for retention timing (battery claims) is out of scope.

### 1.6 SuperGrafx (second VDC + HuC6202 VPC)

Source: [MiSTer sgxtech.txt](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/blob/master/docs/sgxtech.txt) **[FACT secondary, author caveats]**, plus Wikipedia hardware table.

| Aspect | Facts |
|--------|-------|
| Hardware | Second HuC6270 VDC with its own 64 KB VRAM, 32 KB work RAM (vs 8 KB), a HuC6202 VPC that mixes both VDCs before the VCE |
| VPC | Two 10-bit windows, per-region priority modes (2 bits each), default layer order SP2, BG2, SP2', SP1, BG1, SP1'. The VPC sees a 9-bit bus per VDC (4 pixel bits, 4 palette bits, sprite/background flag) |
| Interrupts | IRQ1 is shared by both VDCs, so the handler must read both status registers |
| Uncertain (author-stated) | VPC priority modes `01b`/`10b` are interpretation-only. Window effect on horizontal overscan unchecked. VDC #2 interrupt details not fully documented. EXT-BUS S-EXP peripheral existence unknown. The author "can't guarantee that everything is 100% accurate" |
| Software | Known SGX-only titles: 5 listed (Ghouls & Ghosts, Grandzort, Battle Ace, Aldynes, 1941 Counter Attack). Darius Plus and Darius Alpha are HuCards that use the second VDC if present. No CD titles target SGX |
| Peripherals | No SGX adapter shape for the IFU-30. NEC's RAU-30 bridges it |

Cost: HIGH. Second VDC instance, VPC pixel pipeline, extra RAM, interrupt merging (MAME lists "SGX IRQ wiring needs input merger?" as a TODO **[CODE]**), a back-panel mode switch affecting VDC register mapping. Oracle: weak. The best documentation is a single RE note with self-declared gaps. Value: a handful of exclusive titles.

**[PROPOSAL, low cost, high value]** Make VDC an *instantiable* component and the VCE input a per-dot pixel-bus record from the start. This is SGX-*ready seams*, not SGX support. It costs almost nothing now and avoids a later rewrite. Do not ship an SGX profile until an owner-selected outcome and a stronger oracle exist.

### 1.7 CD-ROM² / Super CD-ROM² / system cards

| Aspect | Facts | Label / source |
|--------|-------|----------------|
| Components | 64 KB general RAM, 64 KB ADPCM RAM, 2 KB BRAM, base unit, single-speed SCSI-like drive, System Card HuCard | **[FACT secondary]** pcetech.txt section 8. The author hedges many CD register details ("I think", "I'm not sure") |
| Memory map | CD RAM at banks `$80-$87`; Super System Card adds 192 KB at `$68-$7F` (about 256 KB total); CD registers at `$1FF800` | **[FACT secondary]** pcetech.txt; **[CODE]** MAME, Beetle (agree) |
| Drive | SCSI-compatible, commands `00/03/08` plus vendor `D8/D9/DA/DD/DE`; 2048-byte READ blocks; BCD or MSF addressing | **[FACT secondary]** pcetech.txt |
| Audio | CD-DA plus ADPCM (MSM5205). Mednafen exposes separate CD-DA/PSG/ADPCM volumes and an `adpcmextraprec` option outputting the full 12-bit predictor | **[CODE/doc]** Mednafen docs. The ADPCM output stage is modeled differently across emulators |
| System card versions | 1.0/2.0/3.0 (Super System Card). The US "3.0" is internally 3.1 and is used on the US Super System Card and Turbo Duo. Games that depend on a specific version exist (A III jumps into 3.0 code; Altered Beast stalls on non-JP-1.0) | **[REPORT]** nicole.express 2020 |
| Games Express | Unlicensed alt-BIOS card required by CD Bishoujo Pachinko, errors on licensed Super CD-ROM² games | **[REPORT]** nicole.express 2020; Mednafen has a `gecdbios` slot **[CODE/doc]** |
| Duo variants | Duo has built-in System Card 3.0 and 256 KB RAM, uses the PC Engine port | **[FACT secondary]** Wikipedia |
| Save state caveat | MAME notes "CD states are saved but not restored correctly" | **[CODE]** MAME `pce.cpp` |

Architecture cost: VERY HIGH. Needs a disc abstraction (tracks, sector reads, CD-DA, subchannel optional), a SCSI-bus state machine, ADPCM, a CD-DA mixer, and timing of seeks/reads (an oracle-hard, load-latency-observable behavior). A single "content span" cannot carry system card + disc tracks. The host owns file IO, so the core needs a sector-read interface, and the disc identity must be defined (tracks, not just a hash).

Rights cost: HIGHEST. The System Card is firmware and must be user-supplied. It cannot be committed, put in CI, or used as a public test input. Disc images are commercial. A clean-room replacement system card is unresearched. A BIOS method-offset reference exists ([stum.de](https://stum.de/2025/pcenginebiosoffsets/)) but I did not read it. Treat HLE/replacement BIOS as **[UNKNOWN]**.

Oracle: E1 for documented SCSI command responses. E2 would need an authored CD-ROM² diagnostic that runs *without* a commercial System Card, **which may be impossible**, since boot goes through the card's BIOS. **[UNKNOWN]** whether any legally redistributable System-Card-free CD boot path exists. This is the central research flag for CD scope.

### 1.8 Arcade Card

| Aspect | Facts | Label / source |
|--------|-------|----------------|
| Hardware | 2 MB non-memory-mapped RAM. Four port register sets at `$1A00/$1A10/$1A20/$1A30` (data, 24-bit base, 16-bit offset, 16-bit increment, control, manual-offset trigger). 32-bit shift/rotate register at `$1AE0-$1AE5`. Version at `$1AFE`, ID `$51` at `$1AFF`. Data ports mirrored at banks `$40-$43`; block transfers (TIA/TAI) limited to 8 KB | **[FACT secondary]** [pce-devel HuC wiki](https://github.com/pce-devel/huc/wiki/Arcade-Card). The page itself contains a "Recovered Page" section that contradicts the main text (control-bit layout; `$1AE5` rotate vs latch). No timing or edge-case docs |
| Variants | Arcade Card Pro and Duo versions. Sources conflict on whether the Pro carries 256 KB extra RAM plus a 3.1 BIOS, and on whether the Duo version includes any BIOS | **[REPORT, conflicting]** |
| Dependencies | Behaves like an extra mapped device on top of a System Card 3.0 environment. Emulators ship it on by default (Mednafen `pce.arcadecard`) | **[CODE/doc]** Mednafen docs |
| Titles | Fatal Fury Special and Garou Densetsu are the best-known. I did not locate an authoritative title list | **[REPORT]** |

Cost: MEDIUM *given* CD support exists. It has no standalone value without CD-ROM². Oracle: an authored fixture is still impossible if it requires a System Card 3.0 boot path. **Depends entirely on 1.7.**

### 1.9 PC Engine LT / GT / Duo / Duo-R / Duo-RX / Shuttle / CoreGrafx: do they differ for emulation?

Source: [Wikipedia hardware table](https://en.wikipedia.org/wiki/TurboGrafx-16) **[REPORT/secondary]** (CPU revisions for GT, LT and Duo are "not stated").

| Model | HuCard-only emulation difference | CD-related difference |
|-------|----------------------------------|------------------------|
| PC Engine (white) | HuC6280 reference | Expansion port for the CD unit |
| CoreGrafx | HuC6280A reported; composite A/V | same |
| CoreGrafx II | Reverts to HuC6280 | same |
| SuperGrafx | HuC6280A, second VDC, VPC, 32 KB RAM | needs adapter; no SGX CD titles |
| Shuttle | No expansion port; has backup slot | no CD |
| TurboExpress / PC Engine GT | Handheld LCD (colour/geometry/ghosting = presentation, not core) | no CD |
| LT | Built-in screen, no AV out | CD-ROM² works, Super CD-ROM² needs an adapter |
| Duo / Duo-R / Duo-RX | Built-in drive and System Card 3.0, 256 KB RAM; Duo-RX bundles a 6-button pad; Duo-R/RX mostly cosmetic | built-in |

**Conclusion [PROPOSAL]:** for HuCard-only emulation these variants collapse to a small profile record: `{cpu_revision, region_bit, has_expansion_port, default_pad}`, plus **analog output differences that belong to the host, not the core**. The model-level differences that change *machine behavior* are the CPU revision (PSG), the built-in System Card/RAM on Duo, and the presence of the expansion port. Per-model *claims* still need per-model evidence, so profiles must be named per console model, not per "family".

### 1.10 Timing facts needed before any "accuracy" claim

From pcetech.txt (**[FACT secondary]**, author warns the document is "very preliminary", tests "could be flawed"):

- Master clock 21.47727 MHz. CPU = master/3 (7.16 MHz) or master/12 (1.79 MHz), switchable via CSH/CSL. Timer decrements every 1024 CPU clocks. PSG clock reported 3.58 MHz (patents reportedly say 7.16, formulas imply 3.58 per the author). VCE dot clock = master/2, /3 or /4 (10.8, 7.16, 5.36 MHz).
- 263 scanlines per frame (14 top blank, 242 active, 4 bottom blank, 3 sync). VDC/VCE accesses cost an extra CPU cycle.
- The document does not state cycles per frame or per line. Those must be derived and independently confirmed. MAME lists "add 263-line mode" and "implement real pixel clocks" as TODOs **[CODE]**. Beetle claims sub-instruction timing and dot-clock emulation **[CODE/doc]**.
- Resolution can differ per VCE dot clock. This means raw video geometry is not a constant per frame (see 2.5).

**[UNKNOWN]** Primary Hudson/NEC manuals (HuC6280 software manual, HuC6270/HuC6260 datasheets) were referenced by search results but I did not open them. Locating them is the first research task for any hardware-fidelity claim.

---

## Part 2: Emulator-facing features and what comparable emulators ship

### 2.0 Competitor / reference landscape (what these projects provide)

| Capability | Mednafen / Beetle PCE | MAME `pce` | Geargrafx | ares | MiSTer TG16 core |
|------------|-----------------------|------------|-----------|------|------------------|
| HuCard base | yes | yes | yes | yes (`cartridge`, `cpu`, `psg`, `vdp`, `vdp-performance` dirs) | yes |
| SF2 / Populous mappers | yes (README/code) | yes (cart slot options) | not stated on page | not verified | SF2, Populous |
| CD-ROM²/Super CD | yes | yes (`PCE_CD`; CD states not restored correctly) | yes, plus LaserActive LD-ROM² via MMI | `pcd` dir exists, not verified | yes (needs BIOS file) |
| Arcade Card | yes (default on) | yes (`acard_duo`, `acard_pro`) | not stated | not verified | yes |
| SuperGrafx | yes | yes (`sgx` config) | yes, both VDCs in debugger | not verified | yes |
| 6-button/multitap/mouse | yes | joypad variants; expansion port not emulated | 6-btn, 5-player tap, mouse | `controller` dir | yes, plus Pachinko, XE-1AP |
| Backup RAM / MB128 | BRAM, Populous | BRAM | BRAM, Memory Base 128 | not verified | BRAM, Memory Base 128 |
| Save states | yes | partial | yes plus preview | yes (not verified for pce) | yes |
| Rewind | yes | no | yes (not LaserActive) | no | no |
| Debugger | yes | yes (generic) | full (disasm, breakpoints, VRAM viewers, PSG inspector, symbols) plus MCP server | partial | none |
| Headless | CLI | yes | `--headless` only with MCP or TurboLink enabled | not verified | N/A |
| libretro | Beetle PCE / SGX | yes | yes | no | no |
| Accuracy docs | "more accurate than Fast module" | header TODO list | "very accurate", no detail | two video implementations | FPGA |
| Notable gaps (documented) | HuC6280A can add clicking in some games | no 263-line mode, no real pixel clocks, no expansion port, known glitches in Cadash and Fighting Run | not documented | not verified | DDR3 latency glitches without SDRAM |

Sources: [Mednafen docs](https://mednafen.github.io/documentation/pce.html), [Beetle PCE README](https://github.com/libretro/beetle-pce-libretro), [MAME pce.cpp](https://github.com/mamedev/mame/blob/master/src/mame/nec/pce.cpp), [Geargrafx](https://github.com/drhelius/Geargrafx), [ares pce dir](https://github.com/ares-emulator/ares/tree/master/ares/pce), [MiSTer core](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer). All **[CODE/doc]**. Cells marked "not verified" were not confirmed from the pages I could read. Nothing here is an accuracy judgment.

Two precedents worth stealing: ares ships a separate `vdp-performance` implementation beside `vdp` **[CODE]**, and Mednafen splits "accurate" from "fast" modules **[CODE/doc]**. Both are reference-vs-fast pairs that support an *internal* differential oracle. Mednafen's docs also warn that behavior-changing settings (e.g. `nospritelimit`) "cause desyncs in netplay or movies" **[CODE/doc]**. **Any setting that changes guest-visible behavior must be part of profile identity.**

### 2.1 Table stakes for the first evidenced profile

"Table stakes" here = required to make the Core Value true for one profile ("a fresh, deterministic, headless run reproduces the behavior and shows the evidence"), not "what players expect from a general emulator". Players' expectations (frontend, filters, save states) are listed under differentiators and deferred.

| Feature | Why expected / required | Complexity | Dependencies | Evidence / oracle needed to claim it |
|---------|-------------------------|------------|--------------|--------------------------------------|
| Named profile record (region bit, CPU rev, video timing, pad type, expansion port, media type) | Core Value says support is "per named profile" **[OWNER]** | LOW | none | A profile manifest plus a "refuses mismatched content/profile" test (E2) |
| HuC6280 core: 65C02 superset, MMU (8x8K banks, 21-bit space), timer, IRQ controller, I/O port, high/low speed | Everything else depends on it | MEDIUM-HIGH | memory map | Instruction/flag vectors from the HuC6280 software manual (E1). Authored per-opcode fixtures (E2). Cycle-count claims need E3 or a primary doc. **Do not claim cycle accuracy from E4 alone** |
| Bus + memory map (8 KB RAM, HuCard pages, I/O, page `$FF`) | CPU needs it | LOW-MEDIUM | CPU | Documented map (E1). Unmapped read value (`$FF` per pcetech.txt) tested as a boundary |
| HuC6270 VDC: BG, sprites (16/line limit), VRAM, SATB DMA, IRQ | Output for any visual evidence | HIGH | bus, VCE | E2 authored display fixtures with literal tile/sprite expectations. Raster-timed effects need machine-time checks (E1/E3) |
| HuC6260 VCE: 512-colour palette, dot-clock selection | Palette/geometry | MEDIUM | VDC | Palette-write/readback fixtures. Dot-clock widths asserted from docs (E1) |
| PSG (6 channels, waveform RAM, noise, LFO, balance) | Raw audio evidence | MEDIUM | CPU, timer | PSG event log asserted against documented register semantics (E1); PCE_Soundtest-derived analog (E3) only if owner funds hardware. **A nonzero audio buffer is not an adequate check** `AGENTS.md` |
| 2-button pad protocol with SEL/CLR scan | Needed for scripted input | LOW | I/O port | Authored input-echo fixture. Negative control: no input and wrong input must produce different guest results |
| HuCard loader with explicit validation and refusal (size, mapper, header, mirroring rules) | Content is an attack surface **[OWNER]** | MEDIUM | profile | Malformed/oversized/zero-length/unsupported-mapper refusal tests. Exact loader rules are **[PROPOSAL]** |
| Internal backup RAM semantics (even if a plain HuCard never uses it) | Not required for HuCard slice. **Defer** unless the first profile includes the CD unit | n/a | | |
| Deterministic scheduler: bounded machine time, fresh instance, no host clock | Core Value | MEDIUM | all | Same-run repeatability (E0) AND semantic assertions (E2). **Repeatability alone is not support** |
| Scripted logical input with explicit machine-time stamps | Core Value; required for raster/mouse timing later | LOW-MEDIUM | pad | Script format conformance tests; "input consumed" negative control |
| Headless runner emitting observations: guest progress, machine time, raw video, raw audio, CPU/device effects | Core Value | MEDIUM | all above | Runner contract tests; distinct outcomes: pass / guest failure / timeout / harness failure / skip / cancel / missing artifact (fail-closed inventory) |
| Raw video capture before colour conversion/scaling | Core Value separation | MEDIUM | VDC, VCE | See 2.5: format is a **[PROPOSAL]**. Checked against fixtures with literal expected pixel/index values |
| Raw audio capture (PSG events and PCM pre-device) | Core Value separation | MEDIUM | PSG | E1 on register-to-event mapping; PCM compared to an independent reference only where one exists |
| Minimal trace: CPU instruction trace plus device-register write log | Needed to diagnose failures and to assert "CPU/device effects" | LOW-MEDIUM | CPU, bus | Trace schema tests. The trace is a diagnostic, not an oracle |
| Authored diagnostic fixture suite with manifest (source, build recipe, hashes, license, notices, redistribution terms, test meaning) | Fast, rights-clear test suite **[OWNER]** | MEDIUM | assembler choice | Manifest completeness check that fails closed. Toolchain license must be checked (see Rights notes) |
| Installed-consumer proof | Required by project constraints **[OWNER]** | MEDIUM | stable runner | Independent consumer built against the installed artifact with source and build paths unavailable |
| Internal snapshot/restore used as a determinism tool (restore-then-continue equals continue) | Lets the harness fast-forward and checks state completeness. Distinct from a user-facing save file | MEDIUM | explicit per-instance state | State-equality tests. This is an E0 property: it shows state is complete, not that behavior is right |

### 2.2 Differentiators (aligned with Core Value; later milestones)

| Feature | Value proposition | Complexity | Dependencies | Evidence / oracle |
|---------|-------------------|------------|--------------|-------------------|
| Evidence report per run: profile ID, evidence tier per claim, machine time, guest-progress markers | Directly the project's reason to exist. No surveyed emulator emits tiered evidence | MEDIUM | headless runner | Report schema plus a calibrated negative control |
| Machine-time-stamped input (cycle or scanline resolution) | Fixes the frame-latched input limit; enables mouse and mid-frame effect tests | MEDIUM | scheduler | Input-latency/sample-point fixtures (E2) |
| Profile-hash-bound replay (input script + start state + profile + build) | Reproducible bug reports; makes "settings that change behavior" visible | MEDIUM | scripted input, snapshot | Replay equality (E0) plus semantic assertions (E2) |
| Reference/fast dual implementation for VDC (ares-style) as an internal differential oracle | Finds regressions in an optimizing core without weakening behavior. **E4-class evidence only** | HIGH | VDC | Differential harness; never presented as correctness |
| 6-button, multitap (5 ports), turbo/autofire from machine time | Expands the content set (SF2' needs 6-button) | MEDIUM | pad protocol | E1/E2 against the documented protocol; autofire rate needs E3 or a documented rate (**[UNKNOWN]** rate) |
| SF2' mapper, Populous RAM | Small, isolated content gain | LOW-MEDIUM each | HuCard loader | E1 for register decode (`$1FF0-$1FF3`); an authored "mapper-style" fixture so no commercial ROM is needed |
| Internal BRAM, Tennokoe Bank, Memory Base 128 | Persistent-save coverage with typed host-owned blobs | MEDIUM | input port (MB128), mapper | E2 lock/unlock/retention fixtures. Tennokoe unlock is **[UNKNOWN]** hardware behavior, so E3 or a primary doc is required |
| HuC6280A PSG-revision profile | Distinguishes revision-specific behavior | LOW-MEDIUM | PSG | E3 only (analog capture). Otherwise E1 against the documented DC-offset description |
| Full debugger (breakpoints, VRAM/PSG inspectors, symbols) | Strong DX. Geargrafx shows the target bar, including an MCP server | HIGH | trace, memory-peek API | Not an accuracy feature. Test via scripted debugger sessions |
| libretro-style frontend adapter | Widely used integration (Beetle, Geargrafx, Mesen2 all ship as libretro cores) | MEDIUM | stable runner | Installed-consumer proof; **do not freeze the API** |
| LibFuturo adapter | Owner-named integration candidate | MEDIUM | stable runner | Prototype adapter or a named reproducible mismatch (input mask width, multiple stores, frame-latched input, single content span) |
| SuperGrafx profile | Adds a small exclusive set plus Darius Plus/Alpha enhancements | HIGH | instantiable VDC, VPC, 32 KB RAM | Weak (RE notes with author caveats). Needs E3 for VPC priority modes |
| CD-ROM²/Super CD-ROM² profile | The largest content area of the platform (library size not sourced here) | VERY HIGH | disc abstraction, system card, ADPCM, CD-DA, SCSI model | System Card is a rights boundary; E2 may be impossible without it (see 1.7) |
| Arcade Card | Completes CD-era coverage | MEDIUM | CD-ROM² | Register spec has internal conflicts. E1 only until a primary source exists |
| User-facing save-state file | Convenience; replay stepping | MEDIUM | snapshot, versioned format | State-format version and profile-bound. MAME's "CD states not restored correctly" is a warning |
| Rewind | Convenience feature | MEDIUM | fast snapshot | Mednafen notes Arcade Card RAM makes rewind costly on slow CPUs |
| Fast-forward / frame-step / tracing tools | Test authoring | LOW | scheduler | n/a |

### 2.3 Anti-features (deliberately not building, or not in core)

| Feature | Why requested | Why problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Unqualified "accurate" or "compatible" claims, game lists, title-screen screenshots as proof | Marketing norm ("very accurate", "full catalog") | Violates Core Value **[OWNER]**. A boot or title screen is not evidence | Per-profile, per-claim evidence tiers (E0..E4), explicit "unknown" |
| CRC/title-string keyed special-casing in the core (Beetle: "POPULOUS" string, CRC `0x34dc65c4`; Geargrafx: internal ROM database that auto-selects hardware) | Convenience; auto-detect | Hidden behavior; a hash does not establish rights or hardware identity; non-reproducible across dumps | Explicit mapper/profile declared in a manifest. A host-side database may *suggest* a profile, never silently override |
| Auto-detecting HuCard byte order or region and guessing | Fewer user steps | Fail-open; wrong guess = wrong machine | Explicit declare-or-refuse in the loader |
| Behavior-changing enhancements in the core: removing the 16-sprite limit, overscan tweaks, "no sprite limit" | Reduces flicker; popular | Mednafen itself notes it causes glitches (Bloody Wolf, Ninja Ryukenden) and desyncs. Breaks profile identity | Host-level presentation only. If ever offered, it is a distinct, hashed profile variant |
| Bundling or embedding System Cards, BIOS, commercial ROMs, disc images, or private saves in repo/CI | Makes CD "just work" | Rights boundary **[OWNER]**. A hash grants no redistribution right | User-supplied files, local-only, gated; authored fixtures in CI |
| Shaders, CRT filters, scalers, scanline settings, composite/RGB simulation inside the core | Players expect them | Host presentation, not machine behavior. Conflates raw video with presentation | Hosts consume raw video. Presentation lives in the frontend |
| Core-owned filesystem, wall-clock pacing, device discovery, windowing | Convenient for a standalone app | Breaks determinism/portability **[OWNER]** | Host-owned; core takes buffers and machine-time budgets |
| Freezing a public C API/ABI or assuming LibFuturo before a consumer proof | Looks like a "stable product" | Premature contract, locks untested shapes **[OWNER]** | Keep an internal API; prove with an installed consumer; adapter first |
| Netplay / TurboLink-style multi-instance, run-ahead in v1 | Geargrafx ships them | Needs mature save-state and determinism first; high maintenance | Defer; replay determinism is the prerequisite |
| Full cycle-accuracy claims from emulator-to-emulator agreement | "Matches Mednafen" | E4 evidence only. It cannot say who is right | Independent oracle (primary doc, authored diagnostic, E3) |
| CD+G, LaserActive LD-ROM², HES music-ROM player, Physical-drive playback, PAL/50 Hz TG-16 | Seen in other emulators | Each adds media/rights/hardware surface without a named user outcome **[UNKNOWN]** | Revisit only with a named user outcome and a source |
| Mouse/XE-1AP/Pachinko/analog in the first slice | Seen in MiSTer, Mednafen | Need machine-time microsecond behavior and per-game enabling (MiSTer warns forcing them breaks games) | After timestamped input exists |
| Broad fuzzing, big compatibility matrices, noisy perf trends in the fast PR gate | "Quality" | Owner CI rule: staged by measured value **[OWNER]** | Staged jobs, each tied to a distinct failure class |

---

## Feature Dependencies

```
Profile record ──> HuC6280 core ──> Bus/memory map ──> HuCard loader (+ refusal rules)
                       │                  │
                       │                  ├──> VDC ──> VCE ──> Raw video capture
                       │                  │      └─ (instantiable x2 later) ──> SuperGrafx (needs VPC + 32 KB RAM)
                       │                  └──> Input port ($1000) ──> 2-button pad ──> 6-button ──> Multitap ──> Mouse (needs machine-time input)
                       ├──> Timer / IRQ ──> Deterministic scheduler (machine time) ──> Scripted input ──> Headless runner ──> Observations ──> Installed-consumer proof
                       └──> PSG ──> Raw audio capture ──> HuC6280A revision profile (E3 only)

HuCard loader ──> SF2' mapper | Populous RAM | Tennokoe Bank (unlock UNKNOWN)
Input port ──> Memory Base 128 (controller-port serial)
Internal BRAM (needs CD unit) ─┐
System Card (rights) ──> Disc abstraction ──> SCSI model ──> CD-DA + ADPCM ──> CD-ROM²/Super CD-ROM² ──> Arcade Card
Explicit per-instance state ──> Snapshot/restore (determinism tool) ──> Replay ──> User save states ──> Rewind
Trace hooks (core) ──> Debugger (host-side)
Stable runner ──> LibFuturo adapter | libretro adapter
Fixture manifest + toolchain license ──> every authored diagnostic
```

### Dependency Notes

- **CPU, bus, scheduler come first.** Every observation (guest progress, machine time, raw video/audio) is a function of the scheduler, so the scheduler must be cycle-aware from day one even if input is frame-latched initially.
- **Timestamped input conflicts with a frame-only adapter.** Mouse timeouts (about 550/600 us) and 6-button/multitap scans need CPU-cycle ordering. A frame-advance-with-input-at-start API (LibFuturo as reviewed) can express only the frame-latched subset. This is a candidate mismatch.
- **CD-ROM² requires a rights decision before any code.** A System Card is firmware. Without a legal, redistributable System-Card-free boot path there may be no E2 fixture for CD. Arcade Card and the Duo profile inherit this.
- **SuperGrafx requires VDC instantiation but not CD.** No CD titles target SGX (sgxtech.txt), so SGX and CD are independent, and SGX can be sequenced on its own merit.
- **User-facing save states conflict with an immature state format.** Internal snapshot-equals-continue testing is low-risk. A persisted file format with versioning is a separate commitment.
- **CRC-keyed special cases conflict with explicit profiles.** Pick one. This research recommends explicit.
- **Tennokoe Bank and Arcade Card are blocked on documentation quality.** Their only available descriptions contain unverified or self-contradictory sections. They need E1 from a better source or E3.

---

## MVP Definition

### Launch With (first evidenced vertical slice, candidate only, not owner-selected)

Matches the seed's proposed slice (`PROJECT.md` Key Decisions: "base profile + HuCard + original diagnostic fixture + headless runner + installed-consumer proof"). Region and CPU revision are open **[OWNER pending]**.

- [ ] One named profile; I **[PROPOSE]** the Japanese PC Engine / HuC6280 as the reference baseline (region bit = 1) because it is the unmodified reference machine. A TG-16 profile differs only by a bit and a default pad, so it is cheap to add *with its own evidence* after the first one.
- [ ] HuC6280 core, bus/MMU, timer, IRQs, I/O port, high/low speed.
- [ ] Plain HuCard loader with explicit sizes (128 KB-1 MB and the 384 KB case), mirroring rules, and refusal of everything else, including SF2', Populous, Tennokoe, and unknown sizes. Copier header handling explicit.
- [ ] VDC + VCE + PSG at the level needed to evidence raw video and raw audio for authored fixtures.
- [ ] 2-button pad through the real SEL/CLR protocol.
- [ ] Deterministic scheduler, scripted logical input, headless runner, observation emitters, and a trace.
- [ ] Authored diagnostic fixtures with manifests; distinct outcome classes; fail-closed inventory; negative controls.
- [ ] Installed-consumer proof.
- [ ] Internal snapshot-equals-continue test (state completeness).
- [ ] Instantiable component seams (VDC/VCE pixel bus, per-port input, typed persistent stores). Cheap now, costly later.

### Add After Validation (v1.x)

- [ ] 6-button, turbo (machine-time autofire), multitap, input timestamps: trigger = first fixture needing >1 pad or mid-frame input.
- [ ] SF2' mapper, Populous RAM: trigger = a named user outcome for those titles plus an authored mapper fixture.
- [ ] TG-16 profile: trigger = owner wants TG-16 claims; needs its own evidence.
- [ ] HuC6280A PSG-revision profile: trigger = E3 capture available.
- [ ] Replay file format, full tracing, scripted debugger, libretro/LibFuturo adapter prototypes.
- [ ] Typed persistent-store model and Tennokoe Bank, once a better source than a MAME TODO exists.

### Future Consideration (v2+)

- [ ] SuperGrafx: needs an owner outcome and a stronger oracle than one RE note with self-declared uncertainty.
- [ ] CD-ROM²/Super CD-ROM²: needs the System Card rights/legal-path decision, a disc abstraction, SCSI/ADPCM/CD-DA, and a loading-latency oracle.
- [ ] Arcade Card: after CD-ROM².
- [ ] Mouse, XE-1AP, Pachinko, MB128, user-facing save states, rewind, run-ahead, netplay.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Profile record + refusal rules | HIGH | LOW | P1 |
| HuC6280 + bus/MMU + timer/IRQ | HIGH | MEDIUM-HIGH | P1 |
| VDC/VCE/PSG to evidence level | HIGH | HIGH | P1 |
| Scheduler + scripted input + headless runner + observations | HIGH | MEDIUM | P1 |
| Fixture manifest + authored diagnostics | HIGH | MEDIUM | P1 |
| Installed-consumer proof | HIGH | MEDIUM | P1 |
| Trace + internal snapshot test | HIGH | LOW-MEDIUM | P1 |
| 6-button / multitap / turbo | MEDIUM | MEDIUM | P2 |
| Timestamped input | MEDIUM | MEDIUM | P2 |
| SF2' / Populous | MEDIUM | LOW-MEDIUM | P2 |
| TG-16 profile | MEDIUM | LOW | P2 |
| Replay | MEDIUM | MEDIUM | P2 |
| Evidence report per run | HIGH | MEDIUM | P2 |
| libretro/LibFuturo adapter | MEDIUM | MEDIUM | P2 |
| BRAM / Tennokoe Bank / MB128 | MEDIUM | MEDIUM | P2-P3 |
| HuC6280A PSG | LOW-MEDIUM | LOW-MEDIUM | P3 (E3-gated) |
| Interactive debugger | MEDIUM | HIGH | P3 |
| User save states / rewind | MEDIUM | MEDIUM | P3 |
| SuperGrafx | LOW-MEDIUM | HIGH | P3 |
| CD-ROM² / Super CD-ROM² | HIGH (library size unsourced) | VERY HIGH | P3 (rights-gated) |
| Arcade Card | LOW-MEDIUM | MEDIUM (after CD) | P3 |

**Priority key:** P1 must have for the first evidenced profile. P2 add when a trigger appears. P3 future or gated.

## Rights and fixture notes

- **[FACT]** `PCE_Soundtest` is MIT-licensed; the repo disclaims warranty ("at your own risk") ([repo](https://github.com/pce-devel/PCE_Soundtest)). Admitting a derived fixture still needs a notice and a check of the exact hardware setup.
- **[FACT per DNA]** The 240p Test Suite PCE source file is GPL-2.0-or-later (per `.planning/bootstrap/EMULATOR-DNA.md`). Using it as a *fixture binary* in a public repo raises licensing/notice questions that are an owner decision. I did not resolve them.
- **[UNKNOWN]** The license of the `pce-devel/huc` toolchain (HuC, HuCC, PCEAS) was not stated on the pages I read. Check its LICENSE file before choosing an assembler for fixtures. Whether output binaries carry obligations is also unresolved.
- **[PROPOSAL]** Real commercial game runs (SF2', Populous, etc.) are local-only, owner-supplied, and never CI inputs. System Cards are never committed or used in public CI.
- Derived "test meaning" must come from a documented behavior or an independent verdict, not from the emulator's own output.

## Conflicts and uncertainties register

| Claim | Conflict / uncertainty | Where it matters |
|-------|------------------------|------------------|
| HuCard data-line direction | Sources agree lines are reversed; one author could not confirm the exact order. Pinout page inaccessible (HTTP 403) | Loader byte-order rule |
| `$1000` bit 6 meaning | Docs say region (1 = PCE). One source argues "boot ROM present" | Region profile wording |
| HuC6280A in CoreGrafx | PCE_Soundtest: "some CoreGrafx"; Wikipedia: CoreGrafx has A, CoreGrafx II reverts | Per-model profile |
| Populous battery | Reports/README say battery-backed; teardown says no battery | Persistent-store model |
| Tennokoe Bank unlock | Best-known implementation marks it "needs real HW study" | Support claim |
| Arcade Card Pro RAM/BIOS | 256 KB + 3.1 BIOS vs 192 KB; Duo BIOS inclusion unclear; wiki page self-contradicts | Arcade Card scope |
| Cycles per frame/line | Not stated in pcetech.txt; MAME TODO for 263-line mode and real pixel clocks | All timing claims |
| PSG clock | 3.58 MHz per formulas vs 7.16 MHz per patents (author's note) | PSG fidelity |
| SGX VPC modes 01b/10b | Interpretation only (author) | SGX scope |
| TurboBooster Plus | No source found | Peripherals scope |
| Primary manuals | Not located or read in this pass | Every E1 claim |

## Sources

Reputability notes: MiSTer/TurboSharp/pce-devel/Nicole Express are community reverse engineering. They are good leads, not primary. Other emulators are evidence of design, not hardware.

- [TurboSharp pcetech.txt](https://github.com/asterick/TurboSharp/blob/master/Text/pcetech.txt) (MEDIUM; author warns "very preliminary")
- [MiSTer sgxtech.txt](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer/blob/master/docs/sgxtech.txt) (MEDIUM; author-stated uncertainties)
- [pce-devel PCE_Controller_Info](https://github.com/pce-devel/PCE_Controller_Info/blob/main/README.md) (MEDIUM-HIGH)
- [pce-devel HuC wiki: Arcade Card](https://github.com/pce-devel/huc/wiki/Arcade-Card) and [wiki index](https://github.com/pce-devel/huc/wiki) (MEDIUM; page has internal conflicts; the wiki lists patents 4951038 and 5059955 as possible primary sources, unread)
- [pce-devel PCE_Soundtest](https://github.com/pce-devel/PCE_Soundtest) (MEDIUM; observational)
- [Nicole Express: trademarks and region locks](https://nicole.express/2020/trademarks-and-region-locks-engine.html) and [backup RAM](https://nicole.express/2022/the-voice-of-the-heavens-is-only-2kb.html) (MEDIUM, single author, hardware-teardown-based; both pages contained embedded text directed at AI agents, ignored)
- [Wikipedia: TurboGrafx-16](https://en.wikipedia.org/wiki/TurboGrafx-16) (LOW-MEDIUM for model table)
- [MAME `pce.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pce.cpp) and [`pce_rom.cpp`](https://github.com/mamedev/mame/blob/master/src/devices/bus/pce/pce_rom.cpp) (CODE)
- [Beetle PCE `huc.cpp`](https://github.com/libretro/beetle-pce-libretro/blob/master/mednafen/pce/huc.cpp) and [README](https://github.com/libretro/beetle-pce-libretro) (CODE)
- [Mednafen PCE docs](https://mednafen.github.io/documentation/pce.html) (CODE/doc)
- [Geargrafx](https://github.com/drhelius/Geargrafx), [ares `pce`](https://github.com/ares-emulator/ares/tree/master/ares/pce), [MiSTer TurboGrafx16](https://github.com/MiSTer-devel/TurboGrafx16_MiSTer) (CODE/doc)
- Unread leads: [stum.de BIOS method offsets](https://stum.de/2025/pcenginebiosoffsets/), [consolemods TG16 Region Information](https://consolemods.org/wiki/TG16:Region_Information) and [Connector Pinouts](https://consolemods.org/wiki/TG16:Connector_Pinouts) (HTTP 403), primary Hudson HuC6280/HuC6270/HuC6260 manuals (referenced by search results, not located).

---
*Feature research for: PC Engine / TurboGrafx-16 emulator (DuoSpool)*
*Researched: 2026-11-09*
