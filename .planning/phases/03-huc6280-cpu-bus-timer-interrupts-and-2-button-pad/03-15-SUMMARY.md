---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 15
subsystem: conformance-and-ledger
tags: [opcode-table, cycle-adders, cyc-probe, insn-sink, ledger, assumed, w2-close]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the manual opcode transcription tests/oracle/huc6280-opcodes.tsv (03-08), the compiled opcode table and ds_cpu_op_defined (03-09), the insn sink and the mutate_sweep kind (03-10), the cpu_ldst, cpu_alu, cpu_flow and cpu_huc fixtures with their ASSUMED:LED-CPU-0010 and LED-CPU-0009 rows (03-11 to 03-14), the DSV1 fixture library (03-05)
provides:
  - ds_op_meta(opcode, &bytes, &cycles), a private accessor of the static const opcode table, and the runner option --opcode-tsv (passed in both conformance-run invocations and in determinism-run)
  - tests/conformance/cpukinds.c with kind opcode_table (256 assertions, TSV against the compiled table, 7-column fixed-width reader) and kind cycle_probe (insn-sink start-stamp deltas against page literals, lost != 0 is harness_failure)
  - fixtures/src/cyc_probe.s, cyc_probe.checks and the manifest entry cyc_probe (fixture id 7): one fixed instruction sequence at $E100 and 12 DSV1 completion checks
  - REQUIRED cases opcode_table_manual, cyc_probe, cycle_adders_manual (18 single-instruction cycle counts), mutate_sweep_cyc_probe (12 of 12 literals), selftest mutation-opcode-table-tsv
  - ledger entries LED-CPU-0001, LED-CPU-0002, LED-CPU-0009 and LED-CPU-0010, all assumed, with TEST pins opcode_table_manual, cycle_adders_manual, cpu_flow and cpu_alu (the pins arrays of the four cases carry the ids)
affects: [03-16, 03-17, 03-20, 03-21, 03-23, 03-25, 03-32, 03-34]

actuals:
  tokens: 13900
  tasks: 3
  commits: 3
plan_head_before: a911efdee6d2bece3b170a3dd4904a3b2ffd1c77
plan_head_after: 46eb2edee0b8052ec0127fc17ea42ad30eabda7a

tech-stack:
  added: []
  patterns:
    - "a kind that needs a new input file takes it through one helper (cpukinds_arg) called from the argument loop of main, so runner.c grows by one else-if, one kinds[] row per kind and one usage word (F7)"
    - "a cycle count is judged from the trace as (start stamp of the next record - start stamp of this record) divided by the master clocks of the speed in force, with the record found by pc and opcode from the assembler listing (order and addresses are independent of the core, ER-7) and the speed stated in the case item, not read from the core"
    - "a fixture whose probe sequence must sit at a fixed address pads the code window with .res $100 - (* - reset) and pins the start with an .assert; .align would make ld65 warn that the CODE segment is not aligned"
    - "the sweep of a trace-judged fixture covers its DSV1 completion checks; the trace literals live in the case, not in the image"

key-files:
  created:
    - tests/conformance/cpukinds.c
    - fixtures/src/cyc_probe.s
    - fixtures/src/cyc_probe.checks
    - fixtures/manifest/cyc_probe.toml
    - tests/conformance/cases/opcode_table_manual.toml
    - tests/conformance/cases/cyc_probe.toml
    - tests/conformance/cases/cycle_adders_manual.toml
    - tests/conformance/cases/mutate_sweep_cyc_probe.toml
    - ledger/entries/LED-CPU-0001.toml
    - ledger/entries/LED-CPU-0002.toml
    - ledger/entries/LED-CPU-0009.toml
    - ledger/entries/LED-CPU-0010.toml
  modified:
    - src/core/cpu.c
    - src/core/ds_core.h
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - CMakeLists.txt
    - scripts/conformance-run
    - scripts/determinism-run
    - scripts/check
    - tests/conformance/REQUIRED
    - tests/conformance/cases/cpu_flow.toml
    - tests/conformance/cases/cpu_alu.toml
    - ledger/REQUIRED

key-decisions:
  - "opcode_table compares only bytes and base cycles of the 234 defined rows and the defined/undefined membership of the 22 UNDEF rows; a block transfer prints 17+6x in the TSV and the table holds the base 17. The expect literals defined=234;undefined=22 are the manual's counts, written in the case; a TSV that holds other counts is harness_failure"
  - "cycle_probe items are PC:OPCODE:CYCLES:SPEED with the pc and opcode taken from the ca65 listing of the built image and the cycles from the pages (S8-17, S8-20, S8-24, S8-30, S8-49, S8-70, S8-74, S8-76, S8-78, S8-90, S8-101, H8-25). Two assertions per item: the record's speed byte equals the stated speed (CSH itself at L, the next INX at H, D-101) and the delta equals CYCLES times 12 (L) or 3 (H), literals of the judge"
  - "the measured quantity is the core's own charged time, so cycle_adders_manual compares the core with page literals; no timer or in-guest clock is involved, and nothing is labelled E2 because nothing circular is asserted (the timer arrives in plan 03-21)"
  - "LED-CPU-0001 and -0002 list an E1 documentary item beside an E0 item but stay assumed: the E1 item is stated as counted only after the plan 03-32 re-read (ER-1, R-8); no entry moved to supported"
  - "LED-CPU-0009 carries both readings of the T bit at hardware interrupt entry (H8-4/5 wording versus pcetech) and the D-99 constant; LED-CPU-0010 names the TRB stored-result row of cpu_huc (check 75), the cpu_ldst pointer-wrap rows and the nine cpu_alu rows in its header comment"
  - "LED-CPU-0005, -0014 and -0016 are not written here: plan 03-20 owns -0014 and -0016 and plan 03-34 owns -0005 (the 03-14 note that 03-15 would write them was an assumption; the plan text and the later plans decide)"

requirements-advanced: [CPU-01, CPU-06, FIX-01]
requirements-completed: []  # CPU-01 still needs the block-transfer fixtures (03-26) and the page re-read (03-32); CPU-06 needs the W3 to W8 entries and 03-32; FIX-01 needs the W8 close-out

duration: 75min
completed: 2026-11-11
---

# Phase 3 Plan 15: Opcode and cycle pins, cyc_probe and the W2 ledger entries Summary

**The compiled opcode table is compared with the manual transcription for all 256 opcodes inside the runner (256 assertions), 18 single-instruction cycle counts measured from the instruction trace of the new cyc_probe fixture equal page literals written in the case, cyc_probe's 12 completion checks are swept, and LED-CPU-0001, -0002, -0009 and -0010 exist as assumed entries with real pins.**

## What was built

- **Task 1 (tracer), commit 26a2e5d.** `ds_op_meta` in cpu.c (declared in ds_core.h) reads the static const table. `tests/conformance/cpukinds.c` holds `kind_opcode_table` and `cpukinds_arg`, which parses `--opcode-tsv` for main with one call; runner.c got one else-if, one `kinds[]` row and the usage word. The TSV reader is fixed-width: seven TAB columns per data row, an opcode of exactly two uppercase hex digits, each opcode once, 256 rows, one or two digit numbers, `17+6x` only on a 7-byte BLK row, `-` only on UNDEF rows; any fault is harness_failure (T-03-41). The option was added to both runner invocations of conformance-run and to determinism-run (see deviation 1). REQUIRED case `opcode_table_manual` (256 assertions) and selftest `mutation-opcode-table-tsv` (a scratch TSV copy with the cycle count of opcode 69 changed to 3 makes the live runner give guest_failure; the unmutated copy passes first). Tracer verify (`build`, `conformance`, selftest) passed; the task carries no `blocking-human` gate.
- **Task 2, commit 8ccf6ac.** `fixtures/src/cyc_probe.s` (fixture id 7): the probe subroutine starts at logical $E100 (the code window is padded and an `.assert` pins it) and runs BEQ not taken and taken, BBR0 not taken, BBR1 taken, SET and ADC, SED and ADC and SBC, TAM, TMA, BRK into a one-INC handler, RTI, CSH, INX, CSL, INX. Twelve DSV1 checks (11 MAN, 1 GUEST) show that the sequence completed and took the defined paths; the sweep case flips all 12 literals. `kind_cycle_probe` runs the ROM with an insn sink (8192 records, stop at DSV1 done) and, per item `PC:OPCODE:CYCLES:SPEED`, finds the record by pc and opcode in trace order, asserts its speed byte, and compares the start-stamp delta to the next record with CYCLES x 12 (L) or x 3 (H). The case `cycle_adders_manual` has 18 items (54 assertions): ADC imm 2, BEQ 2 and 4, BBR 6 and 8, SET 2, T-mode ADC 5, SED 2, decimal ADC 3 and SBC 3, TAM 5, TMA 4, BRK 8, RTI 7, CSH 3 at L, INX 2 at H, CSL 3 at H, INX 2 at L, each page named in the case comment. lost != 0 is harness_failure.
- **Task 3, commit 46eb2ed.** LED-CPU-0001 (pin opcode_table_manual), LED-CPU-0002 (pin cycle_adders_manual; the H8-25 "17+16z" misprint of 17+6x is in `disagreement` with H8-6, S8-88, S8-89 and R-1), LED-CPU-0009 (pin cpu_flow; both T-at-entry readings), LED-CPU-0010 (pin cpu_alu; TSB/TRB N and V per S8-103 and H8-5 versus pcetech, the TRB stored result, decimal flags, undefined opcodes, wraps, no page-cross penalty, RMW). Each has every schema key, the four standard withheld items plus one specific, assumption, revisit_phase 3, origin and its ledger/REQUIRED line; the four cases list the ids in `pins`.

## Verification

- Task verifies exactly as written, all PASS: Task 1 `--stage build`, `--stage conformance`, `--selftest mutation-opcode-table-tsv`; Task 2 `--stage fixture-lint`, `fixtures`, `conformance`; Task 3 `--stage ledger-entries`, `inventory`, `ledger-negatives`.
- Plan end: `sh scripts/check` PASS (about 80 s wall, 55 PASS lines, sanitizers and core-boundary included, the toy controls shared-state and adc-carry stayed red), `sh scripts/check --selftest` PASS (59 PASS lines, `mutation-opcode-table-tsv` among them), `dash scripts/check` PASS, `sh scripts/audit-history --range a911efd..HEAD` clean (3 commits, 30 blobs, 1249 blob-path pairs, run before this docs commit).
- Bite shown with scratch edits (each restored byte for byte, nothing committed): DS_ADDER_T_MODE 3 to 2 makes cycle_adders_manual red at item 7; DS_ADDER_BRANCH_TAKEN 2 to 1 at item 3; TAM 5 to 6 in cpu_ops.def at item 11 and opcode_table_manual red at 0x53; charging the step at the speed after the instruction (CSH at the new speed) at item 15; a sink capacity of 16 turns the case harness_failure (lost != 0). The mutation sweep flips 12 of 12 literals.
- `cyc_probe` runs under the default power-on policies (zero and seeded-prng-v1 seed 42) with identical results; `opcode_exercise` (require mode) still passes with the new ROM in its set.

## Deviations from Plan

**1. [Rule 3 - blocking, files outside the plan list] `scripts/determinism-run` also passes `--opcode-tsv`.** The plan names the two conformance-run invocations (F5), but determinism-run is a third runner invocation over the same cases and would have given opcode_table_manual harness_failure in the determinism stage. The two long command lines (conformance-run, determinism-run) got a shell line continuation before the new option so no line is longer than 400 characters (the toy line of conformance-run was already 443 characters before this plan and is now split).
**2. [Plan text "fixed .org"] The probe address is fixed by padding, not by an ca65 `.org` or `.align`.** `.org` is not available in the relocatable build and `.align 256` makes ld65 warn that the CODE segment is not aligned. The code window is padded with `.res $100 - (* - reset)` and `.assert * - reset = $100` fails the build if the library code ever outgrows the offset. The addresses in the case ($E10F and the others) come from the ca65 listing of the built image, as the plan's ER-7 note says (order and addresses are independent of the core).
**3. [Plan text] The "cycle_probe measured cycle sums come from the timer" clause does not apply.** The timer does not exist yet (plan 03-21); the plan text uses insn-sink start stamps, which is what this plan does. Nothing is labelled E2 circular: the expected literals are page values and the measured quantity is the core's charged time.
**4. [Note] LED-CPU-0005, -0014 and -0016 are not written.** The 03-14 SUMMARY expected plan 03-15 to write them; this plan's text, files list and the later plans (03-20 for -0014 and -0016, 03-34 for -0005) say otherwise. The ASSUMED and MAN rows that name those ids in cpu_huc.checks stay dangling until those plans; the ledger existence check is plan 03-32's.
**5. [Note] LED-CPU-0010 pins cpu_alu only (F9: one entry, one pin).** The ASSUMED rows in cpu_ldst and cpu_huc name the same entry; the header comment of the entry says so.

No Rule 1, Rule 2 or Rule 4 deviation. The standing shared-state toy control stayed red and needed no change.

## Evidence discipline

E0 for what the runner measures. opcode_table_manual compares the core table with a page-image transcription made by the same project (plan 03-08, a different run than the def of plan 03-09): agreement shows consistency, not that the manual is right, and says nothing about hardware. cycle_adders_manual compares the core's charged time with page literals taken from instruction pages and the H8-25 map cells read as images by earlier plans (cites.txt rows, flag `img`); plan 03-15 re-read no page. No expected literal comes from cpu_ops.def, a DuoSpool run, or another emulator; the pc and opcode of each item come from the ca65 listing. The ledger entries are all `assumed`: LED-CPU-0001 and -0002 list an E1 documentary item that is stated as counted only after the plan 03-32 re-read (ER-1, R-8), so none of the eight ER-2 candidates moved. The H8-25 "17+16z" misprint is recorded as a disagreement footnote, not an OCR error (R-1); the block-transfer cost is not asserted anywhere in this plan. The 22 undefined opcodes (1 byte 2 cycles) are an assumption (LED-CPU-0010) and are compared by membership only. A pass is not a support claim; no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. The block transfers and the VDC penalty are still absent from the core (plans 03-16, 03-25); cycle_adders_manual asserts neither. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-41 (TSV reader): fixed-width seven-column parse, opcode range and duplicates checked, 256 rows required, the path comes only from conformance-run and determinism-run, every fault is harness_failure. T-03-42 (ledger overclaim): all four entries are assumed, ledger-check passes. T-03-43 (circular cycle evidence): the literals are in the case from the pages, the lost counter is enforced and shown to bite.

## Notes for next plans

- **03-16 (bus, VDC penalty):** extend cyc_probe.s with the penalty section after `probe` (the plan text says a second section); the probe address is pinned by `.assert * - reset = $100`, so new code goes after `probe_h` and `fixture_main`, and `DSV_CHECKS` and the LIT table grow with it. The `cycle_probe` kind takes any number of items (limit 32) per case; a second case can use the same ROM with other items. Changing the bound of `ds_max_step_mclk` is that plan's job.
- **03-21 and 03-23 (timer, speed):** `cycle_adders_manual` already asserts CSH at the old speed and the next INX at the new one from the trace (items 15 to 18); the speed bit is read from the insn record. An in-guest timer measurement of the same sequence would be E2 self-consistency (ER-3) and should be labelled so.
- **03-25 (block transfers):** the opcode_table kind treats `17+6x` as base 17; the per-byte 6 is not compared. A `block_cycles_manual` case can use cycle_probe with a TII record and a length literal.
- **03-32 (promotion):** re-read H8-25, S8-17, S8-20, S8-24, S8-30, S8-70, S8-74, S8-90, S8-101, S8-103 and H8-4, H8-5 for LED-CPU-0001, -0002, -0009 and -0010 (R-8). The TSV and the def agree with each other; compare both with the pages, not with each other. Only -0001 and -0002 are candidates; -0009 and -0010 stay assumed.
- **03-34:** LED-CPU-0005 (CSL and CSH, H8-25 map cells only, R-6) is written there; cycle_adders_manual items 15 to 18 can be listed as evidence.
- **Runner inputs:** any later runner option goes through the same three invocations (conformance-run run mode, controls mode, determinism-run).
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: tests/conformance/cpukinds.c, fixtures/src/cyc_probe.s, fixtures/src/cyc_probe.checks, fixtures/manifest/cyc_probe.toml, tests/conformance/cases/opcode_table_manual.toml, cyc_probe.toml, cycle_adders_manual.toml, mutate_sweep_cyc_probe.toml, ledger/entries/LED-CPU-0001.toml, -0002, -0009, -0010. Commits 26a2e5d, 8ccf6ac and 46eb2ed are ancestors of HEAD.
