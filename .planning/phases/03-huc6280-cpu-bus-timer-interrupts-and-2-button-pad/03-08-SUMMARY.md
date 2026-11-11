---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 08
subsystem: fixtures
tags: [opcode-oracle, manual-transcription, page-images, xasm, ca65, fixture-lint, licence-check, w2-tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: ca65 and ld65 on the preflight list and the tool record (03-06), the CI cc65 step (03-07), the fixture linter and cite registry (03-37), the W1 close (03-38)
provides:
  - tests/oracle/huc6280-opcodes.tsv, 256 rows (234 defined, 22 UNDEF) transcribed from the H8-25 map and the S8-17 to S8-108 instruction pages read as images, flags from S8-7 to S8-9
  - scripts/xasm-check and scripts/lib/xasm-matrix.awk, the encoding oracle (stage xasm, controls in stage xasm-negatives), with the optional WLA-DX path printing optional-absent
  - fixture-lint rule FXL_UNKNOWN_MNEMONIC reading the TSV mnemonic column, licence-check coverage of tests/oracle
  - LED-TOOL-0001 with E0 matrix evidence, a FACT source for the image read, pinning_test NONE (gate stage xasm)
affects: [03-09, 03-32, 03-33]

actuals:
  tokens: 25000
  tasks: 3
  commits: 3
plan_head_before: 85c97906357eef1d73322a1307cd4a0697fcabca
plan_head_after: 602615155fd8fc9f15db5277464ebd449cd0944c

tech-stack:
  added: []
  patterns:
    - "xasm-check stage 1 (xasm-matrix.awk -v stage=1) writes S, R and V records ending in an END sentinel to a file; stage 2 (-v stage=2) requires the sentinel (XASM_STAGE_DEAD, exit 2) and compares an 8-byte slot per row (padded FF) against the assembled image; no awk on the left of a pipe"
    - "no external command runs before the tool check (case, command -v and cd/pwd only), so an empty PATH gives PREFLIGHT_MISSING ca65 and exit 2 instead of a shell error"
    - "relative targets are labels placed after the instruction (ca65 evaluates * inside BBR/BBS operands at the wrong address, so bbr0 <$20, *+3 gives offset 2)"

key-files:
  created:
    - tests/oracle/huc6280-opcodes.tsv
    - scripts/xasm-check
    - scripts/lib/xasm-matrix.awk
    - tests/gate/xasm/ (ok-tsv, tsv-byte-changed, tsv-bad-shape)
    - tests/gate/fixture-lint/unknown-mnemonic/
    - tests/licence/oracle-no-spdx/
    - tests/licence/ok-oracle/
  modified:
    - scripts/check
    - scripts/fixture-lint
    - scripts/lib/fixture-lint-rules.awk
    - scripts/licence-check
    - fixtures/lib/TOOLS.md
    - ledger/entries/LED-TOOL-0001.toml

key-decisions:
  - "Where the H8-25 grid and an instruction page disagree, the machine code printed on the instruction page decides; every disagreement is listed in the TSV header (opcodes 34 to 37, B4, B5 and the 7D mode label)"
  - "Mode vocabulary of the TSV: IMPLIED ACC IMM ZP ZPX ZPY ABS ABSX ABSY IND INDX INDY ABSIND ABSINDX REL ZPREL BLK MASK IMMZP IMMZPX IMMABS IMMABSX and - for UNDEF; TAM and TMA are MASK (the manual labels them IMPLIED but they take one operand byte, a single set bit)"
  - "Block-transfer cycles are written 17+6x (H8-6, S8-89), never the 17+16z misprint of the H8-25 footnote (R-1); CSL and CSH, which have no flag-table row and no instruction page, carry the flag value unlisted and the single cite H8-25"
  - "The mnemonic set of the fixture linter now comes from the TSV, replacing the built-in list, so the size-hint rule and the unknown-mnemonic rule cannot drift from the manual transcription"

requirements-advanced: [FIX-01, CPU-01, CPU-06]
requirements-completed: []  # FIX-01 also closes in 03-15, 03-32 and 03-33; CPU-01 needs the core table and fixtures; CPU-06 needs the ledger-side cite rules (03-32)

duration: 30min
completed: 2026-11-11
---

# Phase 3 Plan 08: Manual-derived opcode oracle and assembler encoding matrix Summary

**tests/oracle/huc6280-opcodes.tsv holds all 256 opcodes as read from the manual page images, and the new gate stage xasm assembles one line per defined row with ca65 and requires every byte to equal the transcription; five controls (a changed TSV byte, a bad row, a shim assembler that rewrites an instruction, a shim linker that fails, an empty PATH) prove the stage goes red.**

## What was built

- **Task 1 (tracer), commit 5e98006.** `scripts/xasm-check` (MIT OR Apache-2.0, POSIX sh; `--root`, `--expect`, `--ca65`, `--ld65`, `--rows`, `--work`) plus `scripts/lib/xasm-matrix.awk`. A five-row table (ST0, BBR0, TAM, TST, LDA immediate) went end to end: `STATUS xasm ok rows=5 wla=optional-absent`, exit 0; PATH set to an empty directory gave `PREFLIGHT_MISSING ca65` (and ld65, awk, od, tr, sort, grep, cut, mktemp) with exit 2. The stage `xasm` is registered before the build. Tracer gate: `<verify>` re-run after the commit, passed, so expansion went ahead (no `blocking-human` gate).
- **Task 2, commit ac897f6.** The full table. Method: PDFs fetched to a scratch directory outside the repository (sha256 equal to the 03-RESEARCH values for the hardware and software manuals), H8-25 rendered at 300 dpi and read cell by cell as an image (16 rows by 16 columns, gray-code order), every instruction page S8-17 to S8-108 rendered at 110 dpi, the table region cropped and read as an image (page numbers taken from the footers, which all agree with PDF page minus 2), Tables 2-3-1 to 2-3-3 (S8-7 to S8-9) read as images. No OCR text was used for any value. The two reads were compared by script: the 232 opcodes that have an instruction page agree in mnemonic, mode, bytes and cycles except the printing errors below; CSL (54) and CSH (D4) exist only in the map. The 22 empty map cells are 0B 1B 2B 33 3B 4B 5B 5C 63 6B 7B 8B 9B AB BB CB DB DC E2 EB FB FC and are the exact complement of the 234 defined opcodes. The stage now runs with `--rows 234`: all 234 defined rows assemble to the transcribed bytes on the first full run (`STATUS xasm ok rows=234 wla=optional-absent`). TOOLS.md gained the numeric-data note and the matrix measurement; `LED-TOOL-0001` got `pinning_test = "NONE:gate stage xasm (scripts/xasm-check) is the evidence, not a conformance case"`, an E0 evidence item, a FACT source for the image read (verifier gsd-executor, 2026-11-11) and an updated assumption; status stays assumed.
- **Task 3 (TDD), commit 6026151.** Stage `xasm-negatives`: mini-roots `ok-tsv`, `tsv-byte-changed` (EXPECT XASM_MISMATCH), `tsv-bad-shape` (EXPECT XASM_TSV_SHAPE), then four run-time probes: a shim `ca65` generated into a scratch directory (never committed) that turns `lda #$11` into `lda #$12` before calling the real assembler (XASM_MISMATCH), a shim `ld65` that fails (XASM_ASSEMBLE), PATH set to an empty directory (exit 2, `PREFLIGHT_MISSING ca65`, no STATUS line) and `--rows 234` against the five-row table (XASM_TSV_SHAPE). Selftests `mutation-xasm-compare` (deletes the rule-compare line of a scratch copy, requires the tsv-byte-changed control to flip), `mutation-xasm-dead-stage` (empties the awk program, requires XASM_STAGE_DEAD and exit 2 after showing the unmutated copy passes) and two `awk-exit-status` cases (`xasm-stage1`, `xasm-stage2`, each forcing one awk call to die). `fixture-lint` gained `--tsv` and `FXL_UNKNOWN_MNEMONIC` (mini-root `unknown-mnemonic`, selftest `mutation-fixture-lint-mnemonic`); `licence-check` now requires 0BSD in the first five lines of every file under `tests/oracle` (mini-roots `oracle-no-spdx` and `ok-oracle`).

## Printing errors found on the page images

Recorded in the TSV header, none copied as text. The H8-25 grid cells for opcodes 34 to 37 read AND ZP,X / RMB3 ZP / ROL (IND) / ROL ZP,X (a shifted run with one spurious cell), where S8-25, S8-18, S8-68 and S8-67 give BIT ZP,X, AND ZP,X, ROL ZP,X and RMB3; the cells for B4 and B5 read LDA ZP,X and LDA ZP, where S8-55 and S8-53 give LDY ZP,X and LDA ZP,X; 7D prints its mode as (ABS,X); the letter O stands for the digit 0 in several mnemonics; ADS, RT1, TRAi, NCP and DY stand for ADC, RTI, TMAi, NOP and DEY. On the instruction pages S8-74 prints BSC at F1, S8-47 prints AND at 5D and S8-96 prints TAI at E3. The reference-page column of Tables 2-3-x lists pages that differ from the footers (for example SET at S8-88 where the page is S8-78), so cites use the footer page that was read. These are new facts for the ledger and for plan 03-09: the printed H8-25 grid alone would give a wrong table at six opcodes.

## Verification

- Task verifies: `sh scripts/check --stage xasm` (Task 1), `--stage xasm` and `--stage ledger-entries` (Task 2), `--stage xasm-negatives`, `--stage fixture-lint-negatives`, `--stage licence-negatives`, `--selftest mutation-xasm-compare`, `--selftest mutation-xasm-dead-stage` (Task 3): PASS.
- Plan end: `sh scripts/check` PASS (53 stages, about 51 s wall; `xasm` under 1 s, `xasm-negatives` about 1 s), `sh scripts/check --selftest` PASS (50 PASS lines, 13 s), `dash scripts/check` PASS, `sh scripts/audit-history --range 85c9790..HEAD` clean (3 commits, 22 blobs), no PDF, PNG or JPG tracked.

## Deviations from Plan

**1. [Rule 2 - missing control] Extra mini-roots, probes and one selftest.** The plan lists controls for a changed TSV byte, a shim ca65 and an empty PATH, and for the unknown mnemonic and a missing oracle SPDX line. A malformed row (XASM_TSV_SHAPE), the pinned row count (`--rows 234`), a failing linker (XASM_ASSEMBLE), a positive oracle licence case (`ok-oracle`) and a selftest for the unknown-mnemonic rule had no control, and a check without a control can pass vacuously, so I added them: `tests/gate/xasm/tsv-bad-shape`, `tests/licence/ok-oracle`, two inline probes in `stage_xasm_negatives` and `mutation-fixture-lint-mnemonic`.

**2. [Rule 3 - blocking] `--tsv` for fixture-lint and edits to the existing call sites.** The scratch copies of `scripts/` that the selftests run cannot see the repository TSV through the default path (the root of a copy is its parent), exactly as with `--cites`, so eight existing invocations in `scripts/check` (stages and selftests) now pass `--tsv tests/oracle/huc6280-opcodes.tsv`. Behaviour of the other rules is unchanged. The built-in mnemonic list of the size-hint rule was replaced by the TSV column.

**3. [Rule 3 - blocking] LED-TOOL-0001 lines are capped at 399 characters.** `ledger-check` rejects lines of 400 characters or more; the sources and assumption lines were shortened to fit (the substance is kept: the FACT source and the matrix result are there).

**4. Task 1 carried a five-row table.** The full table was transcribed first (it had to be, to find the page rules), then a five-row subset was committed as the Task 1 tracer and the complete file restored for Task 2, so the commits show the tracer shape the plan describes. The thin file's header says it is a tracer.

**5. TDD order.** The new controls were written together with their rules, not seen red before each rule existed. Their red state is proven by the mutation selftests (`mutation-xasm-compare`, `mutation-xasm-dead-stage`, `mutation-fixture-lint-mnemonic`, the shim and empty-PATH probes), each of which shows the control alive and then dead when the rule is removed.

No Rule 1 or Rule 4 deviation.

## Evidence discipline

E0 only. The TSV is documentary transcription (what the page images print); it is not promoted, no ledger status changed and none of the eight ER-2 candidates was touched. The matrix is a byte-level identity check of ca65 (Homebrew bottle, prints V2.18) against the transcription: it says nothing about cycles, flags or behaviour, and says nothing about the apt package. A matching result shows two encoders agree, one of which is a human-style page read, and the page read has now been done once by one verifier; plan 03-32 must still re-read the pages behind any supported promotion (R-8). The cycles column is base cycles as printed; adders (taken branch, T mode, decimal) live in LED-CPU-0002 rows. No accuracy, compatibility, cycle-accuracy, hardware-verified or boot claim. No manual text or image is committed (ER-9, T-03-24); the header describes errors in my own words and the page cites are labels only. WLA-DX was absent (`wla=optional-absent`), so no cross-assembler claim is made.

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-23 (PDF sha256 checked before use, content treated as data, renders in a scratch directory), T-03-24 (numbers and page labels only), T-03-25 (the shim, changed-byte and mutation controls show the comparison bites) and T-03-103 (HTTPS fetch from the recorded mirror path, hash-checked; WLA optional and not installed) are mitigated as planned.

## Notes for next plans

- **03-09 (second table, two-way checker):** the TSV is the oracle. Do not read it while transcribing the core table. Compare opcode by opcode on mnemonic, mode class, bytes and cycles; expect the six printed-grid errors above to be a trap for a table built from the H8-25 grid alone (opcodes 34 to 37, B4, B5). Mode names are mine (see the header); BLK cycles are the string `17+6x`; UNDEF rows carry `-`; CSL and CSH flags are `unlisted`. `scripts/xasm-check --rows 234` pins 234 defined and 22 UNDEF rows.
- **fixture-lint:** every lower-case first word of a fixture source line must be a mnemonic of the TSV (an assignment `name = value` is not judged); lower-case macro names would be flagged, so keep macros upper case as `CHECK_LIT` is.
- **cites.txt (03-32):** the rows S8-7, S8-8 and S8-9 are still flagged `ocr`; this plan read them as images (verifier gsd-executor, 2026-11-11) and the file is not in its list, so the flags were left. H8-25 and the instruction pages S8-17 to S8-108 were also read as images; the TIN flowchart S8-99 and the flowchart text pages were not read. Flip flags in the plan that owns promotion, keeping verifier, date and fact (R-8).
- **WLA path:** written from the WLA-DX documentation and never run; if an owner puts WLA-DX on PATH and the stage reports `XASM_WLA_FAILED` (exit 2) or a wla mismatch, fix the template in `scripts/lib/xasm-matrix.awk` (`wlasrc`), do not trust a wla result until it has run once. Nothing was installed.
- **ca65 quirk worth keeping:** inside BBR and BBS operands `*` evaluates after the opcode and zero page byte, so a literal `*+3` gives offset 2; labels are correct.
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. The new stage adds about one second locally. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: tests/oracle/huc6280-opcodes.tsv, scripts/xasm-check, scripts/lib/xasm-matrix.awk, tests/gate/xasm (three mini-roots), tests/gate/fixture-lint/unknown-mnemonic, tests/licence/oracle-no-spdx, tests/licence/ok-oracle. Commits 5e98006, ac897f6 and 6026151 are ancestors of HEAD.
