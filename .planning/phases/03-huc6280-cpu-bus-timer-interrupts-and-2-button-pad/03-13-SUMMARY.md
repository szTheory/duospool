---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 13
subsystem: cpu-core-and-fixtures
tags: [branches, bit-branches, jumps, subroutines, brk, rti, flag-instructions, nop, cpu-flow, mutation-sweep, w2]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the opcode table and mnemonic switch (03-09), eff_addr and the zero-page helpers (03-11), SEC SED CLV and the ALU group (03-12), the RAM vector slots and DSV1 fixture library (03-05), the mutate_sweep kind (03-10)
provides:
  - cpu.c semantics of BPL BMI BVC BVS BCC BCS BNE BEQ BRA, BBR0-7 and BBS0-7 (third byte is the offset, target PC+3+rr), BSR, JMP abs and (abs,X) beside the existing (abs), JSR, RTS, BRK, RTI, CLI and NOP
  - fixtures/src/cpu_flow.s, cpu_flow.checks and the manifest entry cpu_flow (fixture id 5): 71 checks in ladder order, REQUIRED case cpu_flow
  - 20 more image-read instruction-page cites in fixtures/lib/cites.txt (S8-21 22 23 24 26 27 28 29 31 32 33 35 36 37 38 51 52 57 71 77)
  - REQUIRED case mutate_sweep_cpu_flow (71 of 71 literals flip exactly their own check)
affects: [03-14, 03-15, 03-21, 03-23, 03-24, 03-32]

actuals:
  tokens: 15100
  tasks: 3
  commits: 3
plan_head_before: 6fa9f144b7a5578900553f3b42ada2dbb8dfb22c
plan_head_after: b2453a3a0e5b76c5176f46806fd8b27a6841ff1d

tech-stack:
  added: []
  patterns:
    - "a branch row sets P by PLP to a byte that puts the tested flag in the named state and the other flags in the opposite state (so a core that tests a different flag is caught), runs the branch over a two-instruction marker path ($01 fall through, $02 target) and expects the marker"
    - "the extreme offsets are exercised with filler of one-byte INX ($E8) so a target that is one byte early executes a visible instruction: BRA +127 skips 127 INX bytes, BNE -128 jumps back to an INY at the head of a block that leaves through JMP; the result byte is Y times 16 plus X"
    - "BBR and BBS for one bit use two cells, one with only that bit set and its complement, and four branches accumulating outcome bits into one byte ($06 expected), so a core that tests another bit or inverts a sense differs"
    - "BRK is exercised through the IRQ2 and BRK RAM slot of startup.inc: the fixture stores its handler address, runs BRK, and puts the default (the error-flag handler) back; the handler records the three pushed bytes from the stack page, I and D as the handler sees them (PHP, PLA, mask $0C) and S, and returns with a known X; mode 1 sets bit 5 of the pushed P to test the T load of RTI"
    - "a GUEST check whose expected byte is an address uses an assembler label expression in the guest (lda #<(label+2) into GEXP), so the expectation is computed from the source and never from the core"

key-files:
  created:
    - fixtures/src/cpu_flow.s
    - fixtures/src/cpu_flow.checks
    - fixtures/manifest/cpu_flow.toml
    - tests/conformance/cases/cpu_flow.toml
    - tests/conformance/cases/mutate_sweep_cpu_flow.toml
  modified:
    - src/core/cpu.c
    - fixtures/lib/cites.txt
    - tests/conformance/REQUIRED

key-decisions:
  - "JMP (abs) reads PCL at hhll and PCH at hhll+1 with no 6502 page wrap: the page S8-51 itself prints (hhll+1) and (hhll+X+1), so the row is MAN:S8-51, not an assumption (the earlier comment in cpu.c called it assumed; the page image settles it)"
  - "BBR/BBS: the offset is the third byte and the target is PC+3+rr (S8-20, S8-21). cpu.c takes it through the existing branch_target with the operand chosen by mode (AM_ZPREL uses the third byte)"
  - "BRK pushes (BRK address + 2) high then low, then P with bit 4 (B) = 1 and bit 5 (T) = 0, sets I, clears D, loads PC from $FFF6/$FFF7 through the normal bus (S8-30, H8-17). The pushed bit 5 = 0 is an ASSUMED row (LED-CPU-0009): T is already 0 at BRK because the fetch cleared it, and the manual's flag row T = 0 describes the result, so a pushed T of 0 is the project's reading (D-99)"
  - "RTI pulls P, PCL, PCH and loads every flag including T from the pulled byte (S8-70 flag row RESTORED, D-99). The T load is an ASSUMED row (LED-CPU-0009): a handler sets bit 5 of the pushed byte and the ORA at the return address then runs in T mode"
  - "OPEN CONFLICT NOT DECIDED HERE (orchestrator note, ledger LED-CPU-0009 in 03-CONTEXT section K, which the orchestrator message calls LED-CPU-0012; LED-CPU-0012 in the context is the unmapped-read entry): the T bit pushed at HARDWARE interrupt entry. D-99 says entry pushes the current T then clears it; the H8-4/5 wording and pcetech read differently. This plan touches only BRK (pushes T = 0 per D-99 and the S8-30 flag row) and RTI (loads T); hardware interrupt entry does not exist yet. Plans that implement entry must keep the D-99 constant, one value, and the ledger entry must list both readings in disagreement"
  - "No cycle count and no taken-branch page-cross penalty is asserted anywhere in cpu_flow (LED-CPU-0010 assumption, plan prohibition); the cycle checks arrive with plans 03-21 and 03-23"

requirements-advanced: [CPU-01, CPU-06]
requirements-completed: []  # CPU-01 needs the HuC additions and block fixtures of 03-14 and 03-26 and the oracle cross-check; CPU-06 needs every fixture swept, the ledger entries (03-15) and the later lint rules

duration: 45min
completed: 2026-11-11
---

# Phase 3 Plan 13: Control flow, BRK and RTI, cpu_flow Summary

**cpu.c now executes all eight conditional branches, BRA, BBR0-7, BBS0-7, BSR, the three JMP forms, JSR, RTS, BRK, RTI, CLI and NOP, the authored fixture cpu_flow (71 checks: 60 manual-page literals, 9 in-guest address or stack checks, 2 assumed-class rows) passes under both power-on policies, and every literal of it flips exactly its own check in the sweep.**

## What was built

- **Task 1 (tracer), commit 69f0aa0.** BPL, BMI, BVC, BCC and BCS joined the existing BEQ, BNE, BVS and BRA (flag in the named state, explicit unsigned sign extension of the offset). cpu_flow started with 8 checks: BEQ and BNE taken and not taken by marker path, the two bytes JSR pushes read back from the stack page by the subroutine (expected bytes from the assembler label of the JSR), RTS returns to the next instruction exactly once, and S is balanced. The tracer verify (`fixture-lint`, `fixtures`, `conformance`) passed and, with a scratch change (RTS without the +1, restored byte for byte), the case went guest_failure, so the expansion went ahead; no `blocking-human` gate.
- **Task 2, commit e52f119.** BBR and BBS (one `bit_branch` helper, the third byte as offset), BSR, JMP abs and (abs,X), BRK, RTI, CLI, NOP. cpu_flow grew to 71 checks in ladder order: other conditional branches (12), BRA (1), +127 and -128 offsets, a short backward loop and two flag-preservation rows (5), BBR/BBS for all eight bits (8) plus target, fall-through, flag and backward-loop rows (6), BSR (3), JMP abs, (abs), (abs) at $25FF and (abs,X) with a decoy entry (4), BRK and RTI (12), the seven flag instructions, NOP (5). The code uses 3130 of the 4096 bytes of the code window.
- **Task 3, commit b2453a3.** REQUIRED case `mutate_sweep_cpu_flow`: 71 of 71 literals flip exactly their own check.

## Verification

- Task verifies: Task 1 `fixture-lint`, `fixtures`, `conformance`; Task 2 plus `sanitizers` (and `core-boundary`); Task 3 `fixtures`, `conformance`: all PASS. `RESULT cpu_flow pass assertions=71`, `RESULT mutate_sweep_cpu_flow pass assertions=71`.
- Plan end: `sh scripts/check` PASS (78 s wall, 55 PASS lines, `CONTROL shared-state RED_OK` and `CONTROL adc-carry RED_OK`), `sh scripts/check --selftest` PASS (58 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 6fa9f14..HEAD` clean (3 commits, 15 blobs, 1228 blob-path pairs, run before this docs commit).
- Bite shown with scratch edits of cpu.c (each restored byte for byte, nothing committed; every one turned cpu_flow guest_failure): RTS without the +1, BRK pushing BRK+1, RTI not loading T, BBR/BBS testing the wrong bit, JMP (abs,X) ignoring X, the BBR offset taken from the second byte, BRK not clearing D, BSR pushing PC+2, JMP (abs) wrapping inside the page, BVC with the sense inverted.
- opcode_exercise (mode report): 148 of 234 defined opcodes executed by the fixtures so far (86 listed in the evidence file for plans 03-14 and later).

## Deviations from Plan

**1. [Rule 3 - count] 71 checks, not "about 50".** The branch matrix (eight conditions, taken and not taken, with the other flags in the opposite state), BBR/BBS for all eight bits and the seven flag instructions came to 71. The DSV1 limit (96) and the code window (3130 of 4096 bytes) have room. No manual-page-only fact was dropped.
**2. [Rule 3 - files outside the list] `fixtures/manifest/cpu_flow.toml` and `tests/conformance/cases/cpu_flow.toml`** are named in the plan by their paths; the manifest entry, the case and the REQUIRED lines are the plan's own deliverables, so this is a naming note only. No file outside the plan's list was touched.
**3. [Plan text] "restore the default afterwards".** The BRK handler is installed in the RAM slot and the error-flag handler address is put back as soon as the second BRK has returned; a stray BRK after that sets the DSV1 error flag (T-03-38).
**4. [Ledger id note] The orchestrator message names LED-CPU-0012 for the T-flag push at interrupt entry; 03-CONTEXT section K and the plan 03-15 text assign that conflict to LED-CPU-0009 (interrupt entry and T), and LED-CPU-0012 is the unmapped-read entry.** The two ASSUMED rows use LED-CPU-0009. Nothing is decided; if the orchestrator meant a different entry, only the two class tags in cpu_flow.checks (rows 56 and 59) change.

No Rule 1 bug and no Rule 4 deviation. The standing shared-state toy control stayed `RED_OK`; it needed no change (it is phase-independent since plan 03-12).

## Evidence discipline

E0 only. 60 rows cite instruction pages (S8-20 to S8-24, S8-26 to S8-33, S8-35 to S8-38, S8-51, S8-52, S8-57, S8-70, S8-71, S8-75 to S8-77); the 20 new cites.txt rows and the re-read of S8-20, S8-30 and S8-70 are image reads by this run (flag `img`; no value comes from the OCR layer; page text and images are not committed, the PDFs stayed in a scratch directory outside the repository and their sha256 equals the 03-RESEARCH values). 9 rows are GUEST checks (the pushed bytes of JSR, BSR and BRK against assembler labels, S before and after, S inside the handler). 2 rows are ASSUMED:LED-CPU-0009 (the pushed T of BRK, the T load of RTI). LED-CPU-0009 is not a ledger file yet (plan 03-15 writes it with TEST:cpu_flow and both readings in `disagreement`); no ledger status changed and none of the eight ER-2 candidates was touched. Expected literals come from the page formulas and flag rows read by this run; none comes from DuoSpool output or another emulator. Two page misprints were met and handled by the text and title: the BPL formula lines print N=1 and N=0 swapped (recorded in the S8-28 cite fact), and several branch pages name the wrong mnemonic in their "has no effect" sentence (not used). The policy comparison (zero and seeded-prng-v1 seed 42) shows the fixture does not read power-on RAM; it says nothing about hardware. A pass is not a support claim; no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. The mnemonics of plan 03-14 (RMB, SMB, SAX, SAY, SXY, ST0 to ST2, CSH, CSL, TMA) and the block transfers remain the documented gap in `ds_cpu_step_obs` (advance by table length, charge table cycles). Hardware interrupt entry does not exist yet (plans 03-22 to 03-23); `ds_cpu_step_obs` has no interrupt path.

## Threat Flags

None beyond the plan's register. T-03-37 (stack and PC arithmetic): S wraps through 8-bit masks inside $21xx, PC and every pointer sum wrap by 16-bit masks, the BBR target and the relative target use unsigned sign extension, and every push and pull goes through the bus; the sanitizers stage passes on cpu_flow. T-03-38 (stray vector): the default RAM slot sets the DSV1 error flag and the fixture puts it back after the test BRKs.

## Notes for next plans

- **03-14:** plan 03-14 keeps RMB, SMB, SAX, SAY, SXY, ST0 to ST2, CSH, CSL, TMA and the block-transfer plan; `ds_cpu_step_obs` still lists them in its gap comment. cpu_flow does not use any of them.
- **03-15 (ledger):** write LED-CPU-0009 with both readings of the T flag pushed at hardware interrupt entry (D-99 and H8-4/5 against pcetech) and pin TEST:cpu_flow; the two ASSUMED rows are check 56 (BRK pushes bit 5 = 0, also after SET) and check 59 (RTI loads T from bit 5). LED-CPU-0010 needs no new entry for this plan: JMP (abs) at a $xxFF pointer is MAN:S8-51 (the page prints hhll+1), the page-cross penalty is simply not asserted.
- **03-21 and 03-23 (cycles):** the branch adder (+2 for the eight conditions and BBR/BBS), the BSR, BRK and RTI table counts and the JMP counts are charged by the core but no check asserts them yet; cpu_flow was written so that a cycle probe can reuse its marker paths.
- **03-22 to 03-23 (interrupts):** the BRK handler pattern (slot install, record pushed bytes, return with a known X, restore the default) and the mode-1 trick of editing the pushed P on the stack are reusable for hardware interrupt entry; the entry code must pick one T value per D-99 and keep the other reading in the ledger.
- **cpu_flow code window:** 3130 of 4096 bytes used; a later extension has about 960 bytes, then the second ROMB segment of hucard.cfg is the escape hatch (03-12 note).
- **03-32:** re-read the 20 new cites (R-8). S8-20, S8-30 and S8-70 were re-read for this plan and agree with their rows (BBR is 3 bytes, 6 cycles, 2 more when taken; BRK pushes BRK+2 and sets B to 1 in the pushed byte; RTI pulls P first and its flag row reads RESTORED).
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/src/cpu_flow.s, fixtures/src/cpu_flow.checks, fixtures/manifest/cpu_flow.toml, tests/conformance/cases/cpu_flow.toml, tests/conformance/cases/mutate_sweep_cpu_flow.toml. Commits 69f0aa0, e52f119 and b2453a3 are ancestors of HEAD.
