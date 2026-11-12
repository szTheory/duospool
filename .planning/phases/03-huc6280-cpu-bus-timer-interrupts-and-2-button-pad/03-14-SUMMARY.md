---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 14
subsystem: cpu-core-and-fixtures
tags: [huc6280-additions, sxy, sax, say, rmb, smb, tam, tma, csl, csh, st0-st2, cpu-huc, mutation-sweep, opcode-exercise, w2]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the opcode table and mnemonic switch (03-09), the mutate_sweep kind and the opcode_exercise inventory in report mode (03-10), zero-page helpers and CLA CLX CLY TAM (03-11), the DSV1 fixture library (03-05), the cpu_flow fixture pattern (03-13)
provides:
  - cpu.c semantics of SXY, SAX, SAY, RMB0-7, SMB0-7, TMA (one-hot, and the D-105 highest-index rule for a multi-bit operand), CSL and CSH (the speed bit is state, the instruction is charged at the old speed) and ST0-ST2 through the new ds_bus_write_phys entry that never consults the MPRs
  - fixtures/src/cpu_huc.s, cpu_huc.checks and the manifest entry cpu_huc (fixture id 6): 76 checks in ladder order, REQUIRED case cpu_huc
  - 7 more image-read instruction-page cites in fixtures/lib/cites.txt (S8-67 72 73 79 81 82 87)
  - REQUIRED case mutate_sweep_cpu_huc (76 of 76 literals flip exactly their own check)
  - opcode_exercise in mode=require with exclude=0x73,0xC3,0xD3,0xE3,0xF3: 229 of 234 defined opcodes are executed by the REQUIRED fixtures, only the five block transfers are not
affects: [03-15, 03-16, 03-17, 03-21, 03-23, 03-25, 03-26, 03-32]

actuals:
  tokens: 12700
  tasks: 3
  commits: 3
plan_head_before: 1e1cdcf955ac79d9fcccd7418d528e8243be5d42
plan_head_after: 25a64e7d83a791075c51e503af4ecc1c7a9fc056

tech-stack:
  added: []
  patterns:
    - "a register-swap or clear flag row loads the registers, then pushes the wanted P byte, then loads the register values, then PLP: PLP is the last instruction before the instruction under test so the register loads cannot disturb N and Z"
    - "TAM and TMA are checked two ways that do not share an index: TMA reads back seven distinct values written by TAM (MPR1 and MPR2 take the RAM mirror values $FB and $FA so the zero page and the verdict block stay reachable), and TAM is checked by translation: MPR3 to MPR6 and MPR0 set to $F8 must read the work-RAM marker through logical $6000, $8000, $A000, $C000 and $0000"
    - "ST0-ST2 are checked against a GUEST byte: with MPR0 mapped to work RAM the guest writes RAM[0], RAM[2] and RAM[3], executes the ST, and reads the cell back at logical $0000, $0002, $0003; a core that routed ST through the MPRs would overwrite it"
    - "an addressing-mode check chains the new modes into one expected byte from operand cells that each hold one bit, and repeats the first mode at the end so ORA, EOR and ADC cannot be confused with each other (the expected byte is a hand sum of the cell table, never an observed value)"
    - "opcode_exercise exclude lists are 0xNN hex opcodes, not mnemonics (the runner parses numbers)"

key-files:
  created:
    - fixtures/src/cpu_huc.s
    - fixtures/src/cpu_huc.checks
    - fixtures/manifest/cpu_huc.toml
    - tests/conformance/cases/cpu_huc.toml
    - tests/conformance/cases/mutate_sweep_cpu_huc.toml
  modified:
    - src/core/cpu.c
    - src/core/ds_core.h
    - fixtures/lib/cites.txt
    - tests/conformance/REQUIRED
    - tests/conformance/cases/opcode_exercise.toml

key-decisions:
  - "ST0, ST1 and ST2 call ds_bus_write_phys with $1FE000, $1FE002 and $1FE003 (H8-8 Table 2-2-3, S8-80 to S8-82, D-106). The function drops any other address (T-03-40) and, until plan 03-16 puts the VDC stub behind it, accepts the three legal ones with no effect. The only observable difference from an MPR-routed write is that RAM does not change; the physical effect is asserted when plan 03-16 adds the device"
  - "CSL and CSH set inst->speed to 0 and 1 and the step charges the instruction at the speed it started with (D-101: the 3 cycles are at the old speed). No committed check reads the speed bit or a machine time that depends on it: no instruction reads it and the plan prohibits speed-switch timing here (plans 03-16, 03-23). The two checks assert that each is one byte and that execution continues. A scratch mutation that made CSH take an extra byte turned two checks red"
  - "TMA with a multi-bit operand returns the highest selected index (D-105 constant) and TMA #0 leaves A unchanged; neither is asserted (plan 03-17 adds tma_zero and mpr_latch). TAM with a multi-bit operand already wrote every selected MPR"
  - "RMBn and SMBn are one read and one write of the zero-page cell $2000|zz (LED-CPU-0010 RMW rule), flags untouched (the flag row of S8-67 and S8-79 is T = 0 and dashes)"
  - "The TRB stored-result row (check 75, trb-abs) is ASSUMED:LED-CPU-0010, the same reading as cpu_alu: the formula of S8-102 stores not A AND M while the prose says the result is not stored"

requirements-advanced: [CPU-01, CPU-02, CPU-04]
requirements-completed: []  # CPU-01 still needs the block-transfer fixtures (03-26) and the oracle cross-check; CPU-02 the full bus and MMU cases (03-16, 03-17); CPU-04 the speed-switch timing (03-23)

duration: 30min
completed: 2026-11-11
---

# Phase 3 Plan 14: HuC6280 additions, cpu_huc, opcode coverage as a requirement Summary

**cpu.c now executes SXY, SAX, SAY, RMB0-7, SMB0-7, TMA, CSL, CSH and ST0-ST2 (through a physical write path that ignores the MPRs), the authored fixture cpu_huc (76 checks: 72 manual-page literals, 3 in-guest bytes, 1 assumed-class row) passes under both power-on policies, every literal flips exactly its own check in the sweep, and opcode_exercise is a requirement: 229 of 234 defined opcodes are executed by REQUIRED fixtures and only the five block transfers are excluded.**

## What was built

- **Task 1 (tracer), commit 3942b5e.** SXY, SAX and SAY (exchange, no flag change) beside the existing CLA, CLX and CLY. cpu_huc started with 14 checks: each swap (both registers), the three clears, and flag rows where the register values would change N and Z if the core updated flags. The tracer verify (`fixture-lint`, `fixtures`, `conformance`) passed; with a scratch change (SAX as a no-op, restored byte for byte) the case went guest_failure with two checks red, so the expansion went ahead. The task carries no `blocking-human` gate.
- **Task 2, commit f481041.** RMBn and SMBn, TMA, CSL, CSH, ST0 to ST2 and `ds_bus_write_phys` (declared in ds_core.h). cpu_huc grew to 58 checks: RMB0-7 and SMB0-7 (16, one per bit) plus flag and neighbour-cell rows (4), TMA of the reset MPR7 ($00) and a seven-value TAM/TMA round trip for MPR0 to MPR6 (8), TAM translation reads for MPR0, MPR3 to MPR6 (5), flag rows of TAM and TMA (3), CSH and CSL length rows (3), ST0 to ST2 bypass rows, a length row and a flag row (5). Verify `fixture-lint`, `fixtures`, `conformance`, `sanitizers` (and `core-boundary`) passed. Six scratch mutations of cpu.c (ST0 or ST2 through the normal bus, TMA reading the wrong MPR, SMB shifting the bit, CSH with a wrong length, TAM writing the wrong MPR) each turned cpu_huc guest_failure; every one was restored byte for byte, nothing committed.
- **Task 3, commit 25a64e7.** REQUIRED case `mutate_sweep_cpu_huc` (76 of 76 literals flip exactly their own check). opcode_exercise switched to `mode=require;exclude=0x73,0xC3,0xD3,0xE3,0xF3`. With the task-2 fixture 56 non-block opcodes were still unexercised (the report-mode evidence file listed 86 before this plan: 5 block transfers, 25 HuC opcodes that tasks 1 and 2 cover, and these 56 of the arithmetic, logic, shift and test group), so cpu_huc gained 18 mode-coverage checks (59 to 76) that run the 56 addressing-mode opcodes of ORA, AND, EOR, ADC, SBC, CMP, CPX, CPY, ASL, ROL, LSR, ROR, DEC, INC, BIT, TRB and TST that no other fixture reached. Result: `defined=234 exercised=229 unexercised=5` (the five excluded block transfers). With the exclude list shortened by one opcode (scratch, restored) the case went guest_failure, so the requirement bites.

## Verification

- Task verifies exactly as written: Task 1 `fixture-lint`, `fixtures`, `conformance`; Task 2 plus `sanitizers`; Task 3 `conformance`: all PASS. `cpu_huc` pass=76 fail=0 notrun=0, `mutate_sweep_cpu_huc` mutations=76 excluded=0.
- Plan end: `sh scripts/check` PASS (55 PASS lines, the toy controls including shared-state stayed red as expected: stage named-controls PASS), `sh scripts/check --selftest` PASS (58 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 1e1cdcf..HEAD` clean (3 commits, 20 blobs, 1239 blob-path pairs, run before this docs commit).
- The code window of cpu_huc is 2819 of 4096 bytes (CODE segment), the literal table 157 bytes, 76 of the 96 DSV1 slots.

## Deviations from Plan

**1. [Rule 3 - count] 76 checks, not "about 50".** 58 are the HuC additions (the 16 RMB/SMB bit rows and the flag, neighbour and round-trip rows came to more than the plan's estimate) and 18 are the mode-coverage rows of deviation 2. The DSV1 limit (96) has room.
**2. [Rule 3 - owner fixture full] The unexercised opcodes of the arithmetic, logic, shift and test group are covered in cpu_huc, not in their owning fixture.** The plan says to add such checks to the fixture that owns the group; cpu_alu is at the 96-check DSV1 limit (its 4 KiB window and 96 slots are both used), cpu_ldst has 14 free slots and is the wrong group, and a new fixture would have been a larger scope change than a section of cpu_huc. The section is labelled in the file header and each check names its mode and page. The exclude list was not widened.
**3. [Plan text] The exclude list is `0x73,0xC3,0xD3,0xE3,0xF3`, not `TII,TDD,TIN,TIA,TAI`.** The opcode_exercise parser (tests/conformance/obskinds.c) takes hex opcodes and rejects names; the case comment names the mnemonics.
**4. [Rule 3 - files outside the list] `src/core/ds_core.h` (the declaration of `ds_bus_write_phys`, named by the plan text), `fixtures/manifest/cpu_huc.toml` and `tests/conformance/cases/cpu_huc.toml` by path** are the plan's own deliverables; no other file was touched.
**5. [Rule 1 - own literal] The first run of the mode-coverage section failed check 59 (ora-modes).** The hand sum was wrong: it included a cell ($10) that belongs to the (zz),Y mode, which is not one of the six ORA modes of the chain. The expected literal was recomputed from the cell table ($04 + $01 + $08 + $02 + $40 + $20 = $6F) and the check meaning was corrected; the core was not changed. The failing index was found with a scratch harness outside the repository.
**6. [Scope note] CSL and CSH have no committed assertion of the speed bit itself.** No guest instruction reads it and a machine-time check would assert the speed-switch timing that plans 03-16 and 03-23 own. The core charges the instruction at the old speed and keeps the new one as state (D-101); the record CPU0 region and the insn sink record carry it. A cpu.c change that left CSH a no-op would not turn any committed check red in this plan. See "Notes for next plans".

No Rule 2 or Rule 4 deviation. The standing shared-state toy control needed no change.

## Evidence discipline

E0 only. 72 rows cite instruction pages (S8-17 18 19 25 34 39 40 41 42 43 44 47 48 56 58 67 68 69 72 73 74 79 80 87 90 101 104, H8-8, H8-25); 7 cites.txt rows are new (S8-67 72 73 79 81 82 87, flag `img`, read by this run as page images; no value comes from the OCR layer; page text and images are not committed and the PDFs stayed in a scratch directory outside the repository, their sha256 equal to the 03-RESEARCH values). S8-80, S8-90, S8-101, H8-8 and H8-25 were re-read as images for this plan and agree with their rows; H8-8's Table 2-2-2 prints "MPR 1" twice (the first row is MPR0, a printing error; the operand bit pattern 00000001 is MPR0 by S8-90's `2^i`), which does not change a literal. 3 rows are GUEST (the ST bypass bytes, computed from the guest's own earlier writes). 1 row is ASSUMED:LED-CPU-0010 (the TRB stored result). The LED-CPU ids named here (0005 CSL and CSH, 0014 TAM/TMA one-hot, 0016 ST bypass, 0010 TRB and RMW) are not ledger files yet (plan 03-15); no ledger status changed and none of the eight ER-2 candidates was touched. Expected literals come from the page function lines and flag rows read by this run or from hand sums of cells the fixture writes; none comes from DuoSpool output or another emulator. The policy comparison (zero and seeded-prng-v1 seed 42) shows the fixture does not read power-on RAM or power-on MPR0 to MPR6 (every MPR is written before it is read; MPR7 reads the reset value $00, H8-8); it says nothing about hardware. A pass is not a support claim; no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

`ds_bus_write_phys` accepts the three legal ST targets and does nothing with them: no device exists until plan 03-16 (the VDC stub, the +1 penalty and the `dev` log sit behind this entry). `ds_cpu_step_obs` still lists the five block transfers in its gap comment (plan 03-25). Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-39 (TAM): `ds_mpr_write` is still the only writer of `mpr[]`, values are 8-bit and the translation is computed per access; TMA only reads. T-03-40 (ST physical write): `ds_bus_write_phys` has the ST step as its only caller and drops every address other than $1FE000, $1FE002 and $1FE003.

## Notes for next plans

- **03-15 (ledger and pin cases):** write LED-CPU-0005 (CSL/CSH 1 byte 3 cycles, H8-25 map cells only, R-6), LED-CPU-0014 (TAM/TMA one-hot; pin TEST:cpu_huc), LED-CPU-0016 (ST bypass, H8-8 Table 2-2-3; the in-guest check is cpu_huc st0-bypass to st2-bypass), LED-CPU-0010 (add the TRB stored-result row of cpu_huc check 75 beside the cpu_alu rows). The tag counts of cpu_huc: 72 MAN, 3 GUEST, 1 ASSUMED.
- **03-16 (bus):** put the VDC stub behind `ds_bus_write_phys` (cpu.c, declared in ds_core.h); ST0 to ST2 then become observable at the device. The cpu_huc ST rows keep asserting the MPR bypass (RAM unchanged with MPR0 mapped to RAM); the runner-judged `st_bypass` case of D-106 is that plan's. The +1 penalty and `dev` log are not implemented; `DS_ADDER_*` has no ST entry yet.
- **03-17 (mmu_mpr):** TMA with a multi-bit operand returns the highest selected index now (D-105); TMA #0 leaves A unchanged until `tma_zero` and `mpr_latch` exist. cpu_huc does not assert either. cpu_huc never reads a power-on MPR0 to MPR6, so the seeded power-on policies of that plan cannot disturb it.
- **03-21 and 03-23 (cycles, speed):** `ds_cpu_step_obs` captures the clock length at the start of the step, so CSH and CSL are charged at the old speed and the next instruction at the new one. No committed check asserts this or reads the speed bit; plan 03-23 should add one (for example the insn record speed byte or a machine-time bound across a CSH) and then a CSH no-op mutation will bite. The RMB, SMB, SXY, SAX, SAY, ST, TAM and TMA table counts (7, 7, 3, 3, 3, 4, 5, 4) are charged but not asserted.
- **03-25:** remove `exclude=0x73,0xC3,0xD3,0xE3,0xF3` from opcode_exercise when the block transfers land; the case comment says so.
- **cpu_alu, cpu_ldst, cpu_flow, cpu_huc code windows:** cpu_huc uses 2819 of 4096 bytes and 76 of 96 slots; cpu_alu is at 96 of 96, so a later arithmetic row cannot go into cpu_alu without a second fixture.
- **03-32:** re-read the 7 new cites (R-8). The row texts are numeric and structural only.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/src/cpu_huc.s, fixtures/src/cpu_huc.checks, fixtures/manifest/cpu_huc.toml, tests/conformance/cases/cpu_huc.toml, tests/conformance/cases/mutate_sweep_cpu_huc.toml. Commits 3942b5e, f481041 and 25a64e7 are ancestors of HEAD.
