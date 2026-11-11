---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 12
subsystem: cpu-core-and-fixtures
tags: [alu, adc, sbc, decimal, t-flag, shifts, compares, bit-tests, cpu-alu, toy-control, mutation-sweep, w2]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the opcode table and mnemonic switch (03-09), the memory-mode helpers eff_addr, operand_value and the cpu_ldst fixture (03-11), the mutate_sweep kind (03-10), the toy control machinery (02-16, 03-02), the fixture library and cites registry (03-05, 03-37)
provides:
  - cpu.c semantics of AND ORA EOR ADC SBC (binary, decimal on valid BCD, T-mode cell form), ASL LSR ROL ROR INC DEC (accumulator and memory), INX INY DEX DEY, CMP CPX CPY, BIT TSB TRB TST, SET, and SEC SED CLV
  - T-mode and decimal cycle adders charged from the named constants through one pair of predicates shared with the worst-case table scan (ds_max_step_mclk)
  - fixtures/src/cpu_alu.s, cpu_alu.checks and the manifest entry cpu_alu: 96 checks (the DSV1 limit) in ladder order, REQUIRED case cpu_alu
  - 21 more image-read instruction-page cites in fixtures/lib/cites.txt (S8-18 19 25 41 to 50 56 58 68 69 75 76 78 102)
  - the adc-carry toy control (REQUIRED run shows CONTROL adc-carry RED_OK), all toys built in one cmake --build call, REQUIRED case mutate_sweep_cpu_alu (96 of 96 literals flip exactly their own check)
  - the shared-state toy made phase-independent
affects: [03-13, 03-14, 03-15, 03-23, 03-24, 03-32]

actuals:
  tokens: 21500
  tasks: 3
  commits: 3
plan_head_before: 974b7cd2647f1336c9a6685922789735bc496deb
plan_head_after: af90c5ae1758c04cb5a97c5191846d5ff409e206

tech-stack:
  added: []
  patterns:
    - "an aggregated GUEST check runs an in-guest model against the real instruction over 32 edge sums or shifts, ORs every difference into one byte and expects zero (GEXP = 0, the sweep patches the XOR mask); the adder model is bit-serial from AND, ORA, EOR and a table of the eight bit masks, the shifter model moves one bit at a time through the same table, so no adder or shifter instruction models itself"
    - "P is read as PHP, PLA and a mask ($C3 for N V Z C, $83 for N Z C, one bit for a single flag): B, I and T never enter a literal; P is set from a byte by PLP (SETP) so that a flag the instruction must leave alone is set to a known value first"
    - "D is cleared before the check macro runs: the tally code adds in binary"
    - "a toy control that must be red for every starting state is built on a strictly increasing hidden counter, not on a periodic one"

key-files:
  created:
    - fixtures/src/cpu_alu.s
    - fixtures/src/cpu_alu.checks
    - fixtures/manifest/cpu_alu.toml
    - tests/conformance/cases/cpu_alu.toml
    - tests/conformance/cases/mutate_sweep_cpu_alu.toml
    - tests/controls/adc-carry/RULE
    - tests/controls/adc-carry/EXPECT
  modified:
    - src/core/cpu.c
    - fixtures/lib/cites.txt
    - scripts/conformance-run
    - tests/conformance/REQUIRED
    - tests/controls/shared-state/RULE

key-decisions:
  - "Subtract is A - M - not C from the S8-74 formula (binary: add the complement of M with C as carry in; decimal: digit-wise borrow). The H8-4 prose about borrows is not used (research mismatch 6). CMP, CPX, CPY set C when the register is greater than or equal to M."
  - "Decimal mode (valid BCD only): the result byte is the packed BCD sum or difference and V is unchanged (S8-17, S8-74, MAN class). N, Z and C of a decimal result, the decimal borrow value of a subtract that borrows out, and ADC with both T and D are ASSUMED:LED-CPU-0010 rows (D-99). Invalid BCD is not exercised. T plus decimal costs +4 cycles on ADC by plain reading of the two adders (assumed)."
  - "T mode: T is captured before the per-instruction clear (H8-5); only ADC, AND, EOR and ORA read it (S8-17, S8-18, S8-47, S8-58). The zero-page cell $2000 | X receives the result and A is unchanged; flags come from the cell result. SET sets T after the clear, so the next instruction sees it and the one after does not. SED or CLC between SET and ADC ends T mode (S8-17 'immediately before')."
  - "TSB and TRB: N and V come from memory bits 7 and 6 (S8-103, H8-5), Z from A AND the old memory byte; the fixture asserts only the memory result and Z, and Z and the TRB stored result are ASSUMED rows because the pages do not say where Z comes from and the TRB page's prose ('the result is not stored') contradicts its own formula (M <- not A and M). N and V of TSB and TRB are not asserted (research mismatch 9)."
  - "BIT immediate takes N and V from the operand byte (the flag row of S8-25 reads M7 and M6 for every mode); TST #imm,mem takes Z from imm AND mem and N, V from the memory byte; the encoding is opcode, immediate, address (S8-104)."
  - "DEX: the page's function line prints Y <- X - 1 while its title, opcode CA and mnemonic are X; X is decremented (recorded in the cite fact and the check meaning)."
  - "SEC, SED and CLV are implemented here although plan 03-13 lists them (the fixture's carry-in rows, decimal rows and flag setup need them); 03-13 keeps CLI, NOP and the flow instructions."

requirements-advanced: [CPU-01, CPU-06, FIX-03]
requirements-completed: []  # CPU-01 needs the flow, HuC and block fixtures of 03-13, 03-14 and 03-26 and the oracle cross-check; CPU-06 and FIX-03 need every fixture swept and the later lint rules

duration: 30min
completed: 2026-11-11
---

# Phase 3 Plan 12: ALU, decimal mode, T flag, cpu_alu and the adc-carry toy Summary

**cpu.c now executes the arithmetic, logic, shift, compare and bit-test group including decimal mode and the T-mode memory forms, the authored fixture cpu_alu (96 checks: 75 manual-page literals, 12 in-guest model checks, 9 assumed-class rows) passes under both power-on policies and every literal of it flips exactly its own check in the sweep, and the first fast-gate toy mutant (a binary adder that forgets the carry in) turns cpu_alu red.**

## What was built

- **Task 1 (tracer), commit 0f7f277.** cpu.c gains the accumulator-class helpers (`acc_in`, `acc_out`, `logic_op`, `add_binary`, `adc_op`), captures T before the fetch-time clear, charges the T-mode and decimal adders from the existing constants through `t_mode_mnemonic`, `decimal_mnemonic` and `flag_cycles` (the worst case `flag_adders` is `flag_cycles` with both flags on, so `ds_max_step_mclk` and the step cannot drift), and adds SEC and INY. cpu_alu starts with 25 checks: ten logic rungs (literals, N and Z rows that also show V and C untouched, three addressing modes), five binary ADC literals (carry in, carry out, both overflow directions), the in-guest adder model over 32 sums, and ADC in eight addressing modes (two of them were dropped again for the check limit, deviation 1). The tracer verify (`fixture-lint`, `fixtures`, `conformance`) passed, so expansion went ahead; no `blocking-human` gate.
- **Task 2, commit fe848e6.** SBC, decimal add and subtract on valid BCD, the six shift/rotate/increment/decrement operations in accumulator and memory forms (one `rmw_value` helper, one read and one write), DEX, DEY, CMP, CPX, CPY (one `compare`), BIT, TSB, TRB, TST (one `bit_test`, `tst_addr` for the immediate-first encoding), SET, SED, CLV. cpu_alu ends at 96 checks in ladder order: logic (10), ADC (13), SBC (6), decimal (11), T mode (8), shifts (15, four of them aggregated against an in-guest shifter), INC and DEC (12), compares (9), bit tests (12). The shared-state toy was made phase-independent in the same commit (deviation 4).
- **Task 3, commit af90c5a.** `tests/controls/adc-carry` (rule `rule-adc-carry` on the single `uint32_t sum = a + m + ((uint32_t)inst->p & P_C);` line of cpu.c, replacement without the carry in, EXPECT cpu_alu), `controls_mode` of conformance-run now generates all toy sources, builds every `ds_toy_NAME` in one `cmake --build --target ...` call and then runs each toy, and the REQUIRED case `mutate_sweep_cpu_alu`.

## Verification

- Task verifies: Task 1 `fixture-lint`, `fixtures`, `conformance`; Task 2 plus `sanitizers`; Task 3 `conformance` and `--selftest mutation-control-anchor` (five PASS lines, adc-carry included): all PASS.
- Plan end: `sh scripts/check` PASS (79 s wall, all stages), `dash scripts/check` PASS, `sh scripts/check --selftest` PASS (58 PASS lines), `sh scripts/audit-history --range 974b7cd..HEAD` clean (3 commits, 18 blobs, 1223 blob-path pairs, run before this docs commit).
- Bite shown with scratch edits of cpu.c (restored byte for byte, nothing committed): decimal digit adjust removed fails check 31, SBC using M instead of its complement fails 24, T mode forced off fails 41, ROL ignoring the carry in fails 56, CMP setting C only for A greater than M fails 77, BIT taking V from bit 5 fails 85, DEC clearing C fails 67, TST zero page,X ignoring X fails 95. The adc-carry toy: `CONTROL adc-carry RED_OK` in the gate run; with the replacement equal to the anchor (scratch, reverted) it prints `NOT_RED` with `cpu_alu:pass`. The mutation sweep: 96 of 96 literals flip exactly their own check.
- The toy build batch: `build/gate/controls/build.txt` shows one cmake invocation building the five toys.

## Deviations from Plan

**1. [Rule 3 - limit] 96 checks, not "about 60", and not more.** The first complete fixture had 124 checks; the DSV1 result region holds 96 (the runner refuses a larger count: "the DSV1 check count is above 96"), and the 4 KiB code window of hucard.cfg was exceeded by 1198 bytes. The aggregated model loops were folded into two shared subroutines (`agg_addsub`, `agg_shift`) and 28 lower-value rows were dropped (redundant addressing modes of SBC and ADC, second flag rows of INX and DEY, EQ rows of CPX and CPY, the second digit-carry row, and similar). The code window ends 22 bytes short of full. Every instruction of the group still has at least one row; the dropped rows are listed in the scratch generator only, and none was a manual-page-only fact.
**2. [Rule 3 - plan 03-13 mnemonics] SEC, SED and CLV are implemented here.** The carry-in rows, the decimal rows (SED) and the flag setup of the compare rows need them; plan 03-13 still owns CLI, NOP, the branches, BRK, RTI and JMP.
**3. [Rule 2 - SET and INY and DEX and DEY] listed in the plan's gap comment only as SET and INX/INY/DEX/DEY under "plan 03-12"; all implemented and checked.**
**4. [Rule 1 - standing note, tests/controls/shared-state/RULE] the shared-state toy went NOT_RED a third time** (interleave_twins and interleave_aba_order passed on the toy) because the new fixture shifted the process-wide step count. Per the standing note it was not retuned by eye: the replacement now adds a strictly increasing hidden counter (`shared_n = shared_n + 1u; ... + shared_n`) to every step of every instance, and the RULE comment records the argument (an instance stepped after another instance has stepped in between sees every counter value larger than the solo run, and a sum of strictly larger terms is strictly larger, so the interleaved-versus-solo comparison cannot agree for any starting counter). The three cases of EXPECT are red from a standalone run (counter starting at 0) and inside the matrix; the real core still passes them. The file is outside the plan's file list.
**5. [Files outside the plan list]** tests/controls/shared-state/RULE (deviation 4) and fixtures/manifest/cpu_alu.toml and tests/conformance/cases/cpu_alu.toml (which the plan lists by name only as "manifest entry" and "cases cpu_alu").
**6. [Plan text "about 16 literal edge vectors per group"]** the models run 16 edge pairs or values with C clear and set (32 runs per aggregated check); the literal vectors are one row each.

No Rule 4 deviation.

## Evidence discipline

E0 only. 75 rows cite instruction pages (S8-17, 18, 19, 25, 41 to 50, 56, 58, 68, 69, 74, 102 to 104; the 21 new cites.txt rows are image reads by this run, flag `img`; no value comes from the OCR layer); 12 rows are GUEST model checks; 9 rows are ASSUMED:LED-CPU-0010 (decimal ADC C and Z, decimal SBC borrow value and C, ADC with T and D, TSB Z set and clear, TRB stored result, TRB Z set). LED-CPU-0010 is not a ledger file yet (the ledger plans write it, 03-32 adds the existence check); no ledger status changed and none of the eight ER-2 candidates was touched. Expected literals were computed by hand from the page formulas (a scratch generator used as a calculator); none comes from DuoSpool output or another emulator. The policy comparison (zero and seeded-prng-v1 seed 42) shows the fixture does not read power-on RAM; it says nothing about hardware. A pass is not a support claim; no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64). Cycle counts of the adders are charged by the core but not asserted here (the cycle checks arrive in plan 03-23).

## Known Stubs

None added. The mnemonics of plans 03-13 (branches, BRK, RTI, CLI, NOP, JMP forms, BSR) and 03-14 (RMB, SMB, SAX, SAY, SXY, ST0 to ST2, CSH, CSL, TMA) and the block transfers remain the documented gap in `ds_cpu_step_obs` (advance by table length, charge table cycles).

## Threat Flags

None beyond the plan's register. T-03-34 (8-bit arithmetic): every sum, difference, shift and wrap is explicit unsigned arithmetic with masks, decimal subtract is biased by 16 to stay unsigned, and the sanitizers stage passes on cpu_alu. T-03-35 (self-referential literals): the adder and shifter models use only AND, ORA, EOR, branches and a mask table, and the sweep flips every row. T-03-36 (toy anchor drift): the anchor occurs once and `mutation-control-anchor` covers the new rule.

## Notes for next plans

- **03-13:** SEC, SED, CLV exist already (S8-75, S8-76; CLV has no page read yet, it is the V clear of the table); the gap comment names what is left. `cpu_alu` uses BEQ, BNE and BRA only; BCC, BCS, BMI and BPL do not exist yet and the fixture does not need them. The fixture assumes `CLD` at reset and clears D itself before each check.
- **DSV1 limit:** a fixture holds at most 96 checks; cpu_flow and cpu_huc should plan for it. The 4 KiB code window held 82 checks of cpu_ldst and 96 of cpu_alu with shared model subroutines; a fixture that needs more should add a second segment in ROMB behind the literal table (hucard.cfg change) rather than drop rows.
- **Aggregated model checks:** `agg_addsub` and `agg_shift` (KIND cell) in cpu_alu.s are reusable; a second check of a pair reads MDF without rerunning the loop.
- **03-15:** write LED-CPU-0010 entries for the nine ASSUMED rows: decimal N, Z, C on valid BCD, the decimal borrow value, T plus decimal (+4 cycles), TSB and TRB Z source, TRB stored result against its own prose, and invalid BCD untested.
- **03-23 (timing):** `flag_cycles(mn, t_active, p)` already charges +3 (T mode on ADC AND EOR ORA) and +1 (D on ADC SBC); the cycle rows still have to be asserted.
- **03-24:** the policy list change does not touch cpu_alu (every cell is written before it is read).
- **03-32:** re-read the 21 new cites (R-8). TST, TSB and S8-74 rows reuse earlier researcher reads; S8-104 and S8-103 were re-read for this plan and agree with their rows.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/src/cpu_alu.s, fixtures/src/cpu_alu.checks, fixtures/manifest/cpu_alu.toml, tests/conformance/cases/cpu_alu.toml, tests/conformance/cases/mutate_sweep_cpu_alu.toml, tests/controls/adc-carry/RULE, tests/controls/adc-carry/EXPECT. Commits 0f7f277, fe848e6 and af90c5a are ancestors of HEAD.
