---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 16
subsystem: bus-and-mmu
tags: [bus, bank-class, hardware-page, io-buffer, bus0-region, serial-version-3, vdc-penalty, mmu-mpr, unmapped-value-toy, w3-tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the minimal bus of cpu.c and ds_bus_write_phys (03-01, 03-14), the digest stream of serial version 2 and its goldens (03-03), the DSV1 fixture library and the mutation sweep (03-05, 03-10), the instruction sink and cycle_probe with cyc_probe (03-10, 03-15)
provides:
  - src/core/bus.c: ds_bus_read, ds_bus_write, ds_bus_write_phys, ds_mpr_write moved out of cpu.c; per-access translate, a const bank_class[256] (HuCard, unmapped, RAM x4, hardware page), one switch on offset >> 10 for the hardware page, DS_UNMAPPED_READ, the VDC and VCE stubs, io_buffer, and the access penalty charged before the device sees the access
  - struct ds_bus (one local per step, carries the penalty cycles) passed to every bus call; NULL is accepted for the reset reads and for tests
  - region BUS0 (io_buffer, mpr_latch) in the digest, DS_SERIAL_VERSION 3, digest_stream_prefix and digest_region cases and both goldens recomputed outside the C code
  - fixtures/src/mmu_mpr.s with 48 checks (fixture id 8, 256K), segment BANKS in hucard.cfg for tagged HuCard banks, REQUIRED case mmu_mpr_256k, toy control unmapped-value
  - cyc_probe penalty section (second subroutine, 13th completion check) and REQUIRED case vdc_penalty_assumed (8 items, ASSUMED:LED-CPU-0017 literals)
  - DS_ADDER_VDC_ACCESS in ds_core.h; ds_cpu_op_max_cycles and ds_max_step_mclk carry the penalty (bound 600 master clocks)
affects: [03-17, 03-18, 03-19, 03-20, 03-21, 03-22, 03-23, 03-24, 03-25, 03-27, 03-30, 03-32]

actuals:
  tokens: 15450
  tasks: 3
  commits: 3
plan_head_before: b43ad23d9dcf08e8707d53792a6d40f13a0c3398
plan_head_after: 3750bcd4babb9dc2745ea5b342a30b8df251cb1f

tech-stack:
  added: []
  patterns:
    - "the bank class table is written as runs of a macro (BC_X8, BC_X32, BC_X128), unsized, with a _Static_assert that it holds 256 entries, so a run that is too short or too long fails the build and no GNU range designator is used"
    - "a bus access takes the per-step context (struct ds_bus *bus) as its second argument; the penalty is summed there and the step adds it to the instruction's cycles, so no access-time state lives in the instance and nothing survives a step"
    - "a stateful stub window (io_buffer) is checked by setting the state with one known write (a store to the I/O window) before every read that depends on it, so each check is independent of the ones before it"
    - "a fixture that needs known bytes in the banks above bank 0 writes the optional linker segment BANKS with .repeat ROMBANKS - 1: the first byte of file bank k is k and the last is k xor $A5, so a bank, an offset and a mirror can each be told apart from the $FF of an unmapped read"
    - "a digest literal is recomputed by a perl script that first rebuilds the previous stream version and must reproduce the committed digest before the new region is appended"

key-files:
  created:
    - src/core/bus.c
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - fixtures/manifest/mmu_mpr.toml
    - tests/conformance/cases/mmu_mpr_256k.toml
    - tests/conformance/cases/vdc_penalty_assumed.toml
    - tests/controls/unmapped-value/RULE
    - tests/controls/unmapped-value/EXPECT
  modified:
    - src/core/cpu.c
    - src/core/ds_core.h
    - src/core/serial.c
    - src/core/ds_serial.h
    - src/core/run.c
    - fixtures/src/cyc_probe.s
    - fixtures/src/cyc_probe.checks
    - fixtures/manifest/cyc_probe.toml
    - fixtures/lib/hucard.cfg
    - tests/conformance/digkinds.c
    - tests/conformance/obskinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/digest_stream_prefix.toml
    - tests/conformance/cases/digest_region.toml
    - tests/conformance/cases/cyc_probe.toml
    - tests/conformance/cases/mutate_sweep_cyc_probe.toml
    - tests/conformance/cases/run_overshoot_bound.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml

key-decisions:
  - "io_buffer is a bus latch: a read in $0800-$17FF returns its stub value with the upper bits the window does not drive taken from io_buffer, and the byte read becomes the new io_buffer; a write stores its byte. PSG, I/O and the interrupt addresses $1400 and $1401 read io_buffer whole (open bus); the timer window reads bit 7 of io_buffer over a counter stub of 0; $1402 and $1403 read the upper five bits over three stub bits of 0. VDC and VCE reads and writes and the empty windows ($1800-$1FFF, reading $FF) never touch io_buffer. All of this is ASSUMED:LED-CPU-0012 or -0013, none of it a page fact"
  - "the I/O window stub is open bus (io_buffer) rather than a constant, so that the pad of plan 03-27 replaces a behaviour no committed check depends on beyond checks 42 and 43 of mmu_mpr, which plan 03-27 must revisit"
  - "the penalty is charged per bus access (a read-modify-write to the VDC window costs two), by the offset in the hardware page being below $800, in charge_penalty() of bus.c; the step adds struct ds_bus.penalty to the instruction's cycles, so the cycles are at the speed the instruction started at (D-101, D-107)"
  - "the overshoot bound counts at most one penalty cycle per cycle of the instruction (an access occupies at least one cycle) plus one per cycle of the interrupt entry: ds_max_step_mclk is (17 + 17 + 8 + 8) x 12 = 600, and the case literal run_overshoot_bound.bound is re-derived in the case comment from the manual's 17 and 8 and the assumption"
  - "BUS0 is appended after CPU0 (tag BUS0, 2 bytes: io_buffer, mpr_latch); mpr_latch has no writer until plan 03-17"
  - "no cached bank pointers, no derived table, no function-pointer bus (D-102, D-103): the hardware page is one switch"

requirements-advanced: [CPU-02, CPU-04, CORE-06]
requirements-completed: []  # CPU-02 needs mmu_mpr TAM/TMA edges and the 384K and 1M variants (03-17, 03-20); CPU-04 needs the speed-switch timing (03-23); CORE-06 needs the dev log and observer matrix (03-19)

duration: 140min
completed: 2026-11-11
---

# Phase 3 Plan 16: Bus, MMU, hardware-page windows, BUS0 and the VDC/VCE penalty Summary

**The bus now lives in bus.c with a per-access translate and a 256-entry bank class table, every hardware-page window has a defined stub behaviour with io_buffer as digest state (serial version 3), the VDC and VCE access penalty is one named constant charged in the bus and probed as an assumption, and the 48-check mmu_mpr fixture observes the map from the guest while the unmapped-value toy turns it red.**

## What was built

- **Task 1 (tracer), commit 4c0dcdc.** `bus.c` holds `ds_bus_read`, `ds_bus_write`, `ds_bus_write_phys` and `ds_mpr_write`; `bank_class[256]` is HUCARD for $00-$7F (through `ds_mapper_read`, writes ignored), UNMAPPED for $80-$F7, RAM for $F8-$FB (index `& (DS_RAM8K_BYTES - 1)` behind a `_Static_assert` on the size), UNMAPPED for $FC-$FE and HWPAGE for $FF. The bus calls carry a `struct ds_bus *` (a local of the step); the cpu.c helpers that touch memory take it too and lost their `const` (a read can now change io_buffer). `mmu_mpr` (fixture id 8, 256K) started with 21 checks: unmapped banks and an ignored write, the RAM mirrors, HuCard bank, offset, remap and two-slot reads from tag bytes the fixture writes itself (segment BANKS, added to `hucard.cfg` as an optional segment), bank 0 through two slots, two mirror checks of the 256K image, ROM write ignored, TMA of an unmapped bank value. The tracer verify (`build`, `core-boundary`, `fixtures`, `conformance`) passed, and a scratch edit that made the unmapped return `$00` turned the case guest_failure at check 1 before the commit (the tracer can fail). The task carries no `blocking-human` gate.
- **Task 2, commit 837d943.** The hardware page is one switch on `offset >> 10`: VDC window reads `DS_VDC_STUB_READ` ($00), VCE `DS_VCE_STUB_READ` ($FF), PSG, I/O and $1400/$1401 read io_buffer, the timer window bit 7 of it, $1402 and $1403 its upper five bits, and $1800-$1FFF read $FF with no side effect. `io_buffer` and `mpr_latch` joined `struct ds_instance` (an adjacency `_Static_assert` chain keeps the struct padding-free) and the digest as region BUS0, serial version 3. `digest_stream_prefix` (frames and literal), `digest_region` (tags), both goldens and the `digkinds.c` BUS0 payload check changed in the same commit. mmu_mpr grew to 48 checks (27 hardware-page checks). The unmapped-value toy (RULE anchored on the single `return (uint8_t)DS_UNMAPPED_READ;` of `ds_bus_read`, replacement `return 0u;`) is RED_OK on `mmu_mpr_256k`.
- **Task 3, commit 3750bcd.** `DS_ADDER_VDC_ACCESS` (1 cycle) is charged by `charge_penalty()` in `hw_read` and `hw_write` for offsets below $800, so logical reads and writes through any MPR (zero page and stack included) and the ST0-ST2 physical writes pay it before the window switch; the step adds `bus->penalty` to the instruction's cycles. The penalty joins `ds_cpu_op_max_cycles` and `ds_max_step_mclk`. `cyc_probe.s` gained the second subroutine `penalty` and a 13th completion check; `vdc_penalty_assumed` has 8 items: ST0 5, LDA $0000 (MPR0 = $FF) 6, STA $0400 6, LDA $07FF 6, LDA $0800 5, LDA $0C00 5, LDA zero page with MPR1 = $FF 5, LDA $0000 at high speed 6 cycles of 3 clocks.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `core-boundary`, `fixtures`, `conformance`; Task 2 `--stage conformance`, `record-lint`, `--selftest mutation-control-anchor` (six rules including unmapped-value); Task 3 `--stage fixtures`, `conformance`, `sanitizers` (plus `core-boundary` and `inventory`).
- Plan end: `sh scripts/check` PASS (all stages, the toy controls including unmapped-value and shared-state RED_OK), `sh scripts/check --selftest` PASS, `dash scripts/check` PASS, `sh scripts/audit-history --range b43ad23..HEAD` clean (3 commits, 37 blobs, 1273 blob-path pairs, run before this docs commit).
- Digest literals computed outside the C code: a perl script (scratch directory, not committed) rebuilt the 256K and 384K recipe images from the lcg32 rule, the profile hashes from the field table, the seed-42 RAM from splitmix64 and the CPU0 bytes from the reset vector, reproduced the three committed version 2 digests (eddf37b1..., 31de49dd..., e0ec7c31...) first, then appended BUS0 (two zero bytes) and set the version to 3: af54a963... (digest_stream_prefix), cc8d3b89... (256K zero golden) and 617a12f3... (384K seeded golden). The C output matched on the first run; the stream files hash to the same values under `shasum -a 256`.
- Memory-map facts were read as images of H8-7 to H8-9, H8-24 and H8-8 from a scratch copy of the hardware manual (sha256 40a32e5d..., equal to 03-RESEARCH); nothing was committed or copied. The window bases and widths ($1FE000 VDC, $1FE400 VCE, $1FE800 PSG, $1FEC00 timer, $1FF000 I/O, $1FF400 interrupt registers with $1402 at A1A0 = 10 and $1403 at 11, the rest reserved) and Table 2-2-3 agree with the cites rows.
- Bite shown with scratch edits (each restored byte for byte, nothing committed): the unmapped constant to `$00` turns check 1 red; timer window mask 0x80 to 0xFF turns check 32 red; no io_buffer update for the I/O window turns check 27 red; the empty window updating io_buffer turns check 45 red; the VDC read updating io_buffer turns check 27 red; the penalty range edge changed to $400, $7FF or $C00 turns vdc_penalty_assumed items 3, 4 and 5 red; no penalty on writes turns item 1 (ST0) red; a penalty of 2 turns the case and the overshoot literal red.
- Scratch exhaustive check (not committed): with code in work RAM and MPR0, MPR1 and MPR3 mapped to the hardware page, for all 256 opcodes under four flag settings the extra cycles of one step never exceeded the opcode's table cycles (largest 5, ORA (zz,X) in T mode, against 7), so the overshoot bound above holds for every implemented instruction.

## Deviations from Plan

**1. [Rule 1 - bug found by the new side effects] TST read a fourth byte it does not have.** `ds_cpu_step_obs` read `pc + 3` for every TST, including the zero-page forms (3 bytes). With reads that change io_buffer and cost a cycle in the VDC window that would be an access outside the instruction; the read now happens only for the two absolute forms (`bytes >= 4`). No fixture changed behaviour (the zero-page TST ignores the byte).
**2. [Rule 3 - files outside the plan list] `fixtures/lib/hucard.cfg`** (the optional segment BANKS that mmu_mpr needs for tagged banks; no existing image hash moved), **`tests/conformance/obskinds.c`** (its stepwise reference run read the opcode through `ds_bus_read(w, w->pc)`; it now passes a NULL bus, and its images keep the pc in ROM or RAM where a read changes nothing), **`tests/conformance/digkinds.c`** (the BUS0 payload branch of the digest_stream kind) and **`tests/conformance/cases/run_overshoot_bound.toml`** (its literal `bound=300` became 600, derived in the case comment from the manual's 17 and 8 and the penalty assumption, see key-decisions) **and `tests/conformance/cases/cyc_probe.toml` plus `mutate_sweep_cyc_probe.toml`** (checks 12 to 13). The plan's 21-file list covered the intent; these are the consequences.
**3. [Plan wording] `field_flip.toml` is not edited.** The plan names "the digest_stream_prefix and field_flip literals"; as in plan 03-03 the field_flip kind flips profile fields only and holds no digest literal, so the regenerated literals are digest_stream_prefix, both goldens and the digest_region tag list.
**4. [Plan wording] The access stamps of D-92 and D-108 are not computed yet.** The key_links name an mclk stamp per access and devices caught up to it. No device with time exists until plan 03-21 (the timer), so `struct ds_bus` carries only the penalty now; the stamp (instruction start plus the access offset of D-92 plus the penalty cycles charged so far) is added to the same struct by plan 03-21, which is the first consumer. The order the plan prescribes (penalty charged before the device sees the access, before computing the stamp) is already what `hw_read` and `hw_write` do.
**5. [Count] mmu_mpr has 48 checks, not "about 40"** (21 in the tracer, 27 hardware-page checks in task 2); the DSV1 limit is 96. **cyc_probe gained 13 - 12 = 1 completion check and 8 trace items instead of the 4 the plan names** (the two sides of the range boundary, the zero-page-mapped case and the penalty at high speed were added because D-107 names them and they bound the assumption from both sides).
**6. [Note] No ledger entry is written here.** LED-CPU-0011, -0012, -0013 and -0017 are written by plan 03-20 (its files list); the ASSUMED rows of mmu_mpr and the literals of vdc_penalty_assumed name them already, like the earlier fixtures name ids that plans 03-15 and 03-20 write. The ledger existence check is plan 03-32's.

No Rule 2 or Rule 4 deviation. The shared-state toy control stayed RED_OK throughout, so the standing note was not triggered.

## Evidence discipline

E0 only. mmu_mpr compares the core with the project's own assumptions and with tag bytes the fixture writes itself; the digest literals show that the stream encoding is as documented and repeatable, not that any state is right. The expected literals come from a manual page (MAN: H8-7 for the MPR bank mapping, S8-101 for TMA), an in-guest computation (GUEST: bank 0 through two slots, the end markers) or an assumed ledger id (ASSUMED:LED-CPU-0011, -0012, -0013, LED-LOAD-0003); none comes from DuoSpool output or another emulator. The open-bus behaviour, the x4 RAM mirror, the unmapped constant, the stub values, the io_buffer update rule and the whole VDC/VCE +1 penalty (its scope per access, its order, the zero-page and ST cases) are modelling assumptions, not facts: the manual gives the chip-enable allocation and the register address bits only. vdc_penalty_assumed compares the core's charged time with literals whose excess over the instruction-map base count is the assumption itself; a pass says that the core does what the assumption says. No ledger status changed and none of the eight ER-2 candidates was touched. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

The VDC and VCE windows read constant stubs and drop writes until Phase 4; the timer window, $1402 and $1403 return a counter or register stub of 0 over io_buffer bits until plans 03-21 and 03-22; the I/O window is open bus until plan 03-27; the PSG window reads io_buffer until Phase 5; `mpr_latch` has no writer until plan 03-17. Each is named in `bus.c` and in the mmu_mpr check rows (ASSUMED class). Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-44 (RAM index): `& (DS_RAM8K_BYTES - 1)` with a `_Static_assert` on the size, and `bank_class` is an unsized array asserted to hold 256 entries, indexed by a uint8 MPR value. T-03-45 (ROM index): only banks below $80 reach `ds_mapper_read`, whose bounds refuse indexes outside the image. T-03-46 (uninitialised device state): every stub returns a named constant or io_buffer, and io_buffer is 0 in a zeroed instance under every power-on policy (check 22 reads it before any write).

## Notes for next plans

- **03-17 (TAM/TMA edges, tma_zero):** `mpr_latch` is in the instance and the BUS0 region but nothing writes it; add the writes in TAM and TMA there. mmu_mpr.checks rows 1 to 48 are done; new rows go after 48 (DSV_CHECKS and the LIT table grow; the hardware-page block of checks 22 to 48 sets `MAP SL3, $FF` first and leaves MPR3 at $FF, so new rows that need a RAM or ROM slot map it again). The fixture's `MAP` and `SETIO` macros are in mmu_mpr.s.
- **03-20 (variants, oracle, ledger):** mmu_mpr checks 18 and 19 (banks $20 and $3F as mirrors of banks 0 and $1F) hold for the 256K image only: use `.if ROMBANKS` around their literals for the 384K and 1M variants (the tag segment already scales with ROMBANKS: `.repeat ROMBANKS - 1`). The private accessor for the bank class oracle does not exist; `bank_class` is a file-static in bus.c. Write LED-CPU-0011 (map), -0012 (unmapped, stubs, I/O stub open bus), -0013 (io_buffer rules above), -0017 (penalty: per access, range below $800, zero-page and ST included, ST total 5, high speed one cycle at 3 clocks; RMW not asserted); pins mmu_mpr_256k and vdc_penalty_assumed. The `st_bypass` kind needs the dev sink, which does not exist yet (plan 03-19).
- **03-19 (dev log):** `hw_write` is the one place that sees every write to windows 0 to 5 and the ST physical writes, and `hw_read` every read; the dev record can be written there. `ds_bus_read` is not const and a read can change io_buffer, so an observer must never call it (the matrix peek in obskinds.c passes a NULL bus and relies on a ROM or RAM pc).
- **03-21 (timer):** add the access stamp to `struct ds_bus` (the step's start stamp, the speed's cycle length and the cycles charged so far) and catch the timer up in `hw_read` and `hw_write` for window 3 before the access. mmu_mpr checks 32 to 36 (timer reads with a counter stub of 0, writes only update io_buffer) and 35's second address must be re-judged when the registers exist.
- **03-22 (interrupt registers):** mmu_mpr checks 37 to 41 assert the stubs of $1402 and $1403 (low three bits 0). They change when the disable register and the request register exist.
- **03-25 (block transfers):** each block step makes two data accesses to the bus with the penalty rule above; ds_cpu_op_max_cycles counts the 17 of the table plus at most 17 penalty cycles.
- **03-27 (pad):** mmu_mpr checks 42 and 43 (I/O window write sets io_buffer, I/O read is open bus) must be revisited: the write rule stays, the read becomes the pad value.
- **03-30 and later (snapshot):** BUS0 is two bytes (io_buffer, mpr_latch). `ds_instance` has the new members between `speed` and `rom`; the tiling list must include them.
- **Runner inputs:** no new runner option or case kind was added, so the three runner invocations are unchanged.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/bus.c, fixtures/src/mmu_mpr.s, fixtures/src/mmu_mpr.checks, fixtures/manifest/mmu_mpr.toml, tests/conformance/cases/mmu_mpr_256k.toml, tests/conformance/cases/vdc_penalty_assumed.toml, tests/controls/unmapped-value/RULE, tests/controls/unmapped-value/EXPECT. Commits 4c0dcdc, 837d943 and 3750bcd are ancestors of HEAD.
