---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 09
subsystem: cpu-core
tags: [opcode-table, x-macro, opcode-oracle, two-way-check, overshoot-bound, w2-tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: tests/oracle/huc6280-opcodes.tsv, the independent manual transcription (03-08); the run API and ds_max_step_mclk (03-01, 03-02)
provides:
  - src/core/cpu_ops.def, 256 rows (234 defined, 22 UNDEF) read from the H8-25 map and S8 instruction page images, expanded in cpu.c into a static const table and a mnemonic switch with no default
  - scripts/opcode-check and scripts/lib/opcode-rules.awk, the two-way comparison with codes OPC_MISSING_ROW, OPC_EXTRA_ROW, OPC_FIELD, OPC_UNDEF_COUNT, OPC_SHAPE and the 234/22 cross-check (stage opcode-oracle, controls in stage opcode-oracle-negatives)
  - ds_max_step_mclk() scanned from the table plus named adders (DS_ADDER_BRANCH_TAKEN 2, DS_ADDER_T_MODE 3, DS_ADDER_DECIMAL 1) plus the interrupt entry 8; kind run_overshoot and the REQUIRED case run_overshoot_bound
affects: [03-10, 03-11, 03-12, 03-13, 03-14, 03-15, 03-16, 03-32]

actuals:
  tokens: 57000
  tasks: 3
  commits: 3
plan_head_before: 07f38ed2054218f36881ed969fcb47c8059c4a6b
plan_head_after: f2d097588b56d53ced1b6f866316c78febf9b3f8

tech-stack:
  added: []
  patterns:
    - "opcode-check runs awk three times (stage 1 for the def, stage 1 for the tsv, stage 2), each writing to a file with its status tested; each stage 1 ends in an END sentinel that the shell and stage 2 both require (OPC_STAGE_DEAD, exit 2); no awk is on the left of a pipe"
    - "the mnemonic list of cpu.c (DS_MNEMONICS) names each mnemonic once; the dispatch is switch over enum ds_mn with no default, so -Wswitch fails the build when a mnemonic has no case; a row-count enum plus _Static_assert pins 256 rows"
    - "taken-branch, T-mode and decimal adders are functions of the mnemonic in cpu.c and constants in ds_core.h; the table keeps the manual's base count (BRA is 4 and has no adder)"

key-files:
  created:
    - src/core/cpu_ops.def
    - scripts/opcode-check
    - scripts/lib/opcode-rules.awk
    - tests/gate/opcode-oracle/ (ok-tree, cycles-differ, def-row-missing, tsv-extra-row, undef-count)
    - tests/conformance/cases/run_overshoot_bound.toml
  modified:
    - src/core/cpu.c
    - src/core/ds_core.h
    - src/core/run.c
    - scripts/check
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.h
    - tests/conformance/runner.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/interleave_aba_order.toml

key-decisions:
  - "Block transfers hold their base 17 in the def; opcode-check compares it as the manual's 17+6x (the 6 per byte is the block step of D-100); a BLK row with any other base fails as OPC_FIELD"
  - "BRA keeps the manual's 4 cycles and takes no taken-adder (the adder applies to the eight conditional branches and to BBR0-7 and BBS0-7 only); this gives the same total as the earlier 2+2"
  - "Opcodes whose group lands in plans 03-11 to 03-14 (and the block plan) advance PC by the table length and charge the table cycles with no other effect; each case label names the plan"
  - "A block transfer counts its base 17 in the overshoot bound, so ds_max_step_mclk() is (17 + 8) x 12 = 300 master clocks; run_overshoot_bound pins that literal, so a later change of the derivation must change the case in the same commit"

requirements-advanced: [CPU-01, CPU-06, CORE-04]
requirements-completed: []  # CPU-01 needs the fixtures (03-11 to 03-14), CPU-06 the ledger-side cite rules (03-15, 03-32), CORE-04 the chunk-equivalence cases of later waves

duration: 70min
completed: 2026-11-11
---

# Phase 3 Plan 09: Opcode table as data, two-way oracle check and derived overshoot bound Summary

**cpu_ops.def holds all 256 opcodes as read from the manual page images and drives the core's dispatch; scripts/opcode-check compares it with the independent TSV of plan 03-08 row by row and both ways on mnemonic, mode, bytes and base cycles (234 defined, 22 undefined), and ds_max_step_mclk() is now scanned from that table (300 master clocks) and pinned by run_overshoot_bound.**

## What was built

- **Task 1 (tracer), commit f08fc45.** `cpu_ops.def` with the 32 rows that the authored fixtures use; `cpu.c` expands it into `static const struct ds_op ops[256]` (`mn`, `mode`, `bytes`, `cycles`, `cls`) and dispatches with `switch ((enum ds_mn)mn)` and no default. Every mnemonic has a case already (gap cases carry a comment naming the plan); a row the thin def lacked ran as the undefined opcode, as before. `scripts/opcode-check` with `--partial` compared the 32 rows against the TSV: no difference. Stage `opcode-oracle` registered before the build. `ds_cpu_max_cycles()` was replaced by `ds_cpu_op_max_cycles(opcode)` and the adders became named constants in `ds_core.h`; `ds_max_step_mclk()` in `run.c` now scans the 256 opcodes. Tracer verify (`--stage build`, `conformance`, `core-boundary`) passed before the commit, so expansion went ahead (no `blocking-human` gate).
- **Task 2, commit a447346.** The full def (256 rows in opcode order, 234 defined and 22 undefined), `--partial` removed, the tracer fallback removed, a row-count `_Static_assert` of 256 added. `sh scripts/check --stage opcode-oracle` passes with `STATUS opcode-oracle ok defined=234 undefined=22`.
- **Task 3, commit f2d0975.** Stage `opcode-oracle-negatives` with five mini-roots; selftests `mutation-opcode-oracle-field` and `mutation-opcode-oracle-dead-stage` plus three `awk-exit-status` cases (`opcode-oracle-stage1-def`, `-stage1-tsv`, `-stage2`); kind `run_overshoot` (stepkinds.c, one runner.h line, one `kinds[]` row) and the REQUIRED case `run_overshoot_bound`. The `ds_max_step_mclk()` replacement itself was done in Task 1 together with the table, because the old function scanned the table that Task 1 removed.

## How independence was kept

The def was transcribed from page images rendered by this run from the verified PDFs (sha256 of the hardware and software manuals equal the 03-RESEARCH values, copied from the plan 03-08 scratch download, which is allowed by T-03-104). H8-25 (PDF page 28) was rendered at 300 dpi and read cell by cell in four crops, 16 rows by 16 columns in gray-code order. The instruction pages were read as images for: ADC S8-17, AND S8-18, BBR S8-20, BBS S8-21, BIT S8-25, BRA S8-29, BRK S8-30, BSR S8-31, DEC S8-44, INC S8-48, JMP S8-51, JSR S8-52, LDA S8-53, LDY S8-55, NOP S8-57, RMB S8-67, ROL S8-68, ROR S8-69, SAX S8-72, ST0 S8-80, STZ S8-86, SXY S8-87, TAI S8-89, TAM S8-90, TDD S8-94, TIA S8-96, TII S8-98, TIN S8-100, TMA S8-101, TRB S8-102, TSB S8-103, TST S8-104. Rows whose instruction page was not read as an image carry only the cite H8-25. No OCR text was used for any value (the text layer was used only to find which page holds which instruction). The TSV data rows were not opened while transcribing; the cpu.c hand-written switch was not copied from. Two limits to state plainly: the executor read the first twenty comment lines of the TSV (its header, no data row) while taking xasm-check as a template, and the 03-08 SUMMARY (which this plan was told to read) names the six printing errors of the H8-25 grid, so those six places were known in advance; each was then verified on its instruction page by this run (S8-25, S8-18, S8-68, S8-67, S8-55, S8-53, S8-17) rather than taken from that note.

**Result of the first comparison.** Zero rows disagreed: all 256 rows (234 defined, 22 UNDEF) matched the TSV in mnemonic, mode, bytes and base cycles on the first run of `opcode-check`, so there was no row to resolve by a page re-read, and neither table was edited to match the other. A result with no difference at all deserves a second look, so the checker was shown to bite before it was trusted: a scratch def with B5 changed to 7 cycles and TII to 16 gave `OPC_FIELD` for both, and the committed controls (below) and the two mutation selftests prove the field comparison, the count rule and the sentinel are alive. Where the printed map and the instruction pages disagree, the page was used: 34 BIT ZP,X, 35 AND ZP,X, 36 ROL ZP,X, 37 RMB3 (the map prints a shifted run with ROL (IND) and no BIT ZP,X), B4 LDY ZP,X and B5 LDA ZP,X (the map prints LDA ZP and LDA ZP,X with 2 7 for B5), 7D ADC with the mode label (ABS,X) misprinted as (ABS.X) in parentheses; the S8-96 page prints TAI at E3 where the map and the sequence say TIA; the block-transfer cycle formula is 17+6x on every page, never the 17+16z of the map footnote.

## Verification

- Task verifies: Task 1 `--stage build`, `conformance`, `core-boundary`; Task 2 `--stage opcode-oracle`, `build`, `conformance` (and `core-boundary`, `sanitizers` also run); Task 3 `--stage opcode-oracle-negatives`, `conformance`, `--selftest mutation-opcode-oracle-field`, `--selftest mutation-opcode-oracle-dead-stage`: PASS.
- Plan end: `sh scripts/check` PASS (55 PASS lines, about 56 s wall), `sh scripts/check --selftest` PASS (55 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 07f38ed..HEAD` clean (3 commits, 28 blobs, 1199 blob-path pairs).
- The overshoot case was shown to bite: with `+ DS_INT_ENTRY_CYCLES` removed from ds_max_step_mclk in a scratch edit (restored byte for byte) `run_overshoot_bound` failed on the bound literal; on the real tree the largest observed overshoot over 64 deadlines on ds-proto, plain-256k and plain-384k is 162 master clocks, below 300.

## Deviations from Plan

**1. [Rule 3 - blocking] interleave_aba_order retuned.** With the full table the random stream of plain-256k now executes instructions of 2 to 17 cycles instead of mostly 2, and the shared-state toy control (tests/controls/shared-state) came out `NOT_RED` on `interleave_aba_order` (the toy adds one clock every fifth step; that case is red only for some counter phases, as its own comment says). The control is not weakened: the case's call sizes (stimulus, not a golden) were re-measured. A search over random sizes, each judged on the toy behind a filler case of 25 and then 50 different lengths (75 phases, all red) picked 26660, 4332 and 15976 (was 9000, 6000, 11000, which was red for 16 of 25 phases now); the real core passes them. The file comment records this and warns against retuning by eye. Files: `tests/conformance/cases/interleave_aba_order.toml` (not in the plan's file list).
**2. [Rule 2 - missing control] Extra controls and codes.** The plan names four mini-roots; `ok-tree` was added as the positive control the runner requires, and `OPC_SHAPE` (a malformed row) joined the four codes so a bad row is a violation rather than a silent skip. `def-row-missing` expects `OPC_MISSING_ROW,OPC_UNDEF_COUNT`: removing a def row also leaves 233 defined opcodes, which is a true second finding, so the exact-set control names both. Three `awk-exit-status` cases (CR-01) cover the three awk runs.
**3. Task order of the case labels.** The plan puts "case labels for every mnemonic" in Task 2; all labels were written in Task 1 because the mnemonic switch has no default and must compile as a whole. Task 2 added only rows, the row-count assert and the removal of the tracer scaffolding.
**4. Behaviour change that follows from the plan.** The opcodes of later groups used to run as the undefined opcode (1 byte, 2 cycles); they now advance by the table length and charge the table cycles (the plan's gap behaviour). Fixtures that only use the already implemented opcodes are unaffected (conformance and every fixture stage stay green). Operand modes of the implemented mnemonics are the immediate and three absolute modes through one helper, so LDA, LDX, LDY, ADC, CMP and EOR also work in `abs` and `abs,Y` forms they did not before, and STA, STZ, INC in the three absolute modes; decimal mode and the T flag are still not implemented for ADC (plan 03-12).

No Rule 1 or Rule 4 deviation.

## Evidence discipline

E0 only. The def and the TSV are two page-image transcriptions by the same model family, made one after the other in different runs; their agreement shows the transcription is consistent, not that the manual is right, and says nothing about hardware. No ledger status changed and none of the eight ER-2 candidates was touched. `run_overshoot_bound` is E0 (repeatability and a literal): bound=300 comes from the manual cites S8-89 and S8-30 and the 12 mclk of D-90, never from a run. The 22 undefined rows (1 byte, 2 cycles, T cleared) remain an assumption (LED-CPU-0010) and are compared by membership only, as the plan says (I11). No accuracy, compatibility, cycle-accuracy, hardware-verified or boot claim. No manual text or image is committed (ER-9, T-03-24: cites are page labels). The PDFs and renders stay in a scratch directory outside the repository.

## Known Stubs

The functionality gap named in the plan: the mnemonics of plans 03-11 to 03-14 and the block transfers advance PC by the table length and charge the table cycles with no register, flag or memory effect (the case labels in `ds_cpu_step` name the plan). This is intentional and each is resolved by the named plan. No other stub.

## Threat Flags

None beyond the plan's register. T-03-26 (all 256 rows defined, 256 asserted at compile time, switch without default), T-03-27 (independence recorded above; no side edited to match the other), T-03-28 (table-derived bound, REQUIRED case, `time_wrap` uses the same function) and T-03-104 (the PDFs were already in scratch from plan 03-08 and their sha256 was checked equal to the 03-RESEARCH values before use; nothing fetched, nothing committed) are mitigated as planned.

## Notes for next plans

- **03-10 to 03-14:** replace a gap case in `ds_cpu_step` by its semantics; the case label list names the plan. Use `operand_value`/`has_ea` for the modes already handled and extend them for ZP, indexed and indirect modes (the access class column `cls` is carried but not used yet; the bus offset rule of D-92 uses it).
- **03-12:** `flag_adders()` in cpu.c already names which mnemonics take the T-mode (ADC, AND, EOR, ORA) and decimal (ADC, SBC) adders for the bound; the execution of those adders is still to be written in the step. If a mnemonic gains an adder, change the function and, if the maximum moves, the literal `bound=300` of `run_overshoot_bound` in the same commit.
- **03-14 block plan:** a block transfer is charged its table 17 in one step for now; when it is split into prologue, byte and epilogue steps the maximum single step shrinks but the bound stays table-derived (base 17), so `bound=300` remains conservative; update the doc comment, not the literal, unless the derivation changes.
- **03-16:** the VDC and VCE penalty slots join the adders in `ds_core.h` and `ds_cpu_op_max_cycles`; `bound=300` will change then.
- **03-15 and 03-32:** cites in the def are page labels only; rows whose instruction page was not read as an image carry H8-25 alone, so the 03-32 re-read (R-8) should cover the rows without an S8 cite. The TSV and the def agree on all 256 rows, so the re-read compares both with the page, not with each other.
- **interleave_*:** the other two shared-state-toy cases (`interleave_ab`, `interleave_twins`) are still green, but any later change to the cycle table or to opcode semantics can move step boundaries again; re-measure the phases (see the comment in `interleave_aba_order.toml`) instead of retuning by eye.
- CI: `.github/workflows/gate.yml` is unchanged, nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. The new stages add about one second locally. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/cpu_ops.def, scripts/opcode-check, scripts/lib/opcode-rules.awk, tests/gate/opcode-oracle (five mini-roots), tests/conformance/cases/run_overshoot_bound.toml. Commits f08fc45, a447346 and f2d0975 are ancestors of HEAD.
