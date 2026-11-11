---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 37
subsystem: fixtures
tags: [fixture-lint, cites-registry, expected-value-classes, size-hints, dead-stage-guard, awk]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the fixture library and the .checks format (03-05)
provides:
  - fixtures/lib/cites.txt, the registry of manual page cites with an img or ocr flag
  - scripts/fixture-lint, a two-stage checker of fixtures/src against D-125 (R-3 wording) with eight FXL_* codes and one red control per code
  - stages fixture-lint (static, before the build) and fixture-lint-negatives, selftests mutation-fixture-lint-orphan and mutation-fixture-lint-dead-stage
affects: [03-08, 03-32, 03-06]

actuals:
  tokens: 6000
  tasks: 2
  commits: 2
plan_head_before: ab4dc4004ec328c56eaa628add1d89d53df0986b
plan_head_after: 3c7ccf1ce51cc2d3241bdfc425e6d49173585b32

tech-stack:
  added: []
  patterns:
    - "stage 1 (scripts/lib/fixture-lint-lex.awk) writes records to a file and ends with an END sentinel; stage 2 (fixture-lint-rules.awk) exits 2 and prints X FXL_STAGE_DEAD without it; the shell tests both awk statuses and requires exactly one N summary record; no awk is on the left of a pipe"
    - "one tagged statement per code, so a selftest can delete it: rule-orphan, rule-unlisted, rule-bad-class, rule-count, rule-include-path, rule-size-hint, rule-raw-uncited, rule-unknown-cite"

key-files:
  created:
    - fixtures/lib/cites.txt
    - scripts/fixture-lint
    - scripts/lib/fixture-lint-lex.awk
    - scripts/lib/fixture-lint-rules.awk
    - tests/gate/fixture-lint/ (nine mini-roots: ok-fixture plus one per code, each fixtures/src/t.s, t.checks and an EXPECT)
  modified:
    - scripts/check

key-decisions:
  - "Pairing: a CHECK_LIT or CHECK_GUEST with no .checks row is FXL_UNLISTED_CHECK (a number used twice counts its second use); a .checks row with no CHECK, or a second row for an index, is FXL_ORPHAN_CHECK. FXL_COUNT is only the headers: id= and checks= present and checks= equal to the row count"
  - "A RAW line is any source line whose comment holds the word RAW; it must read RAW:<page> and the page must be in cites.txt. Plain .byte data tables carry no marker and are not RAW lines"
  - "The size-hint rule is evaluated only for instruction mnemonics (a built-in lower-case HuC6280 list), on each comma-separated operand piece after parentheses are stripped; immediates, the block transfers (TAI TDD TIA TII TIN) and non-literal operands are not judged. z:, a: and < are the accepted hints (R-3)"
  - "cites.txt uses the label CDBIOS-p28 for the CD BIOS manual printed page 28 (a label with a space would not fit a MAN: class tag); the research rows that name two pages (H8-10 and H8-11, S8-88 and S8-89, S8-90 and S8-101) are one row per page"

requirements-advanced: [FIX-03, CPU-06]
requirements-completed: []  # FIX-03 needs the mutation sweep and CPU-06 needs the ledger-side cite rules (plan 03-32); neither is complete

duration: 35min
completed: 2026-11-11
---

# Phase 3 Plan 37: Fixture linter and cite registry Summary

**scripts/fixture-lint now rejects an orphan or unlisted check, a bad class tag, a wrong count, a foreign include, an unhinted zero-page operand, an uncited RAW line and an unknown manual cite, each with its own red control, and a dead stage 1 reads as FXL_STAGE_DEAD (exit 2) rather than a pass.**

## What was built

- **Task 1 (tracer), commit 17d5453.** `fixtures/lib/cites.txt` (0BSD header, ER-9 note, 40 rows, label TAB flag TAB verifier TAB date TAB fact) seeded from the 03-RESEARCH image-verified table with the corrected cites: ST0 to ST2 on H8-8, no H8-26, block transfer cycles on H8-6 plus S8-88 and S8-89; S8-7, S8-8 and S8-9 (read from the OCR layer only) are the three `ocr` rows. `scripts/fixture-lint` reads `fixtures/src/*.s` and `*.checks` through `scripts/lib/fixture-lint-lex.awk` (stage 1, ending with an END sentinel) and `fixture-lint-rules.awk` (stage 2), with `--root`, `--expect` and `--cites FILE` (default `fixtures/lib/cites.txt`), and prints `FIXTURE_LINT fixtures=N checks=M raw=R` and `STATUS fixture-lint ok`. A cite registry that is empty, malformed or has a duplicate label is a harness error (exit 2). Stages `fixture-lint` and `fixture-lint-negatives` are registered after `inventory-negatives`, before the build; selftests `mutation-fixture-lint-orphan` (deletes the rule-orphan lines of a scratch copy and requires the orphan-check control to flip) and `mutation-fixture-lint-dead-stage` (empties the scratch stage 1 and requires exit 2 with FXL_STAGE_DEAD, after showing the unmutated copy passes). This commit carried only the orphan rule and the ok-fixture and orphan-check mini-roots.
- **Task 2, commit 3c7ccf1.** The other seven rules and seven mini-roots, one per code: unlisted-check, bad-class (class `EMU:other-emulator`), count (`checks=3` with two rows), include-path (`.include "../elsewhere/other.inc"`), size-hint (`lda $20`), raw-uncited (`; RAW ...` without a page) and unknown-cite (`MAN:H8-99`). With the Task 1 rules the seven controls were seen BROKEN (EXPECT_MISMATCH, no code reported) before the rules went in, then green. The real tree (`ds_proto`, 8 checks, 0 RAW lines) is clean. A scratch probe outside the repo also showed `lda ($20),y`, `sta $0020,x`, `ldx %101`, `lda ($20,x)`, `bbr3 $20, l` and `tst #$80, $21` flagged for the size hint, a block transfer with small numbers not flagged, `RAW:H8-99` an unknown cite and `ASSUMED:LED-CPU-001` a bad class.
- **Extra proof of CR-01, in `selftest awk-exit-status`.** Two cases (`fixture-lint-stage1`, `fixture-lint-stage2`) use the existing awk shim to force each awk program to die and require exit 2 and no OK line.

## Verification

- Task verifies (`--stage fixture-lint`, `--stage fixture-lint-negatives`, `--selftest mutation-fixture-lint-orphan`, `--selftest mutation-fixture-lint-dead-stage`): PASS.
- Plan end: `sh scripts/check` PASS (all 50 stages, 52 s), `sh scripts/check --selftest` PASS, the two new stages also PASS under `dash`, `sh scripts/audit-history --range ab4dc40..HEAD` clean (2 commits, 32 blobs).

## Deviations from Plan

**1. [Rule 3 - structure] A third script file, `scripts/lib/fixture-lint-lex.awk`, holds stage 1.** The plan lists only `fixture-lint-rules.awk` and `scripts/fixture-lint`. The dead-stage selftest has to empty stage 1 in a scratch copy, which is a one-line replacement when stage 1 is its own file (the `missing-sentinel` precedent) and a fragile text edit when it is inline. No other file outside the plan list changed except the two `as_case` additions inside `scripts/check` (itself in the list).

**2. FXL_COUNT is limited to the headers.** The plan says "counts equal `checks=`". `checks=` is compared with the number of rows only; comparing it with the CHECK uses too would make the unlisted-check control give two codes. A CHECK without a row is FXL_UNLISTED_CHECK instead, so the two counts cannot drift without one code or the other.

## Evidence discipline

No ledger status changed; no E-tier claim. The linter checks the shape of the expected-value classes (ER-4) and that a cited page is in the registry; it does not check that a page says what a check claims, and the `img` or `ocr` flag is recorded but not yet enforced (plan 03-32). cites.txt holds page labels and short numeric facts in the registrar's words only (ER-9, T-03-15): no manual text or image. The `img` rows carry the 2026-11-10 read of the research table; they are not re-read here, and plan 03-32 must re-read the pages behind any supported promotion (R-8). No accuracy, compatibility, cycle-accuracy, hardware or boot claim.

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-15 (cites.txt), T-03-17 (FXL_BAD_CLASS, FXL_UNKNOWN_CITE) and T-03-96 (sentinel, tested awk statuses, FXL_STAGE_DEAD with a selftest) are mitigated.

## Notes for next plans

- Rules still to add where their inputs appear: mnemonics a subset of the opcode TSV (plan 03-08; the lint rules file has a header comment naming it, and a built-in mnemonic list is used for the size-hint rule until then), a MAN: cite behind a supported ledger entry must be `img`, and an ASSUMED: id must name a ledger entry (plan 03-32). Each new rule needs one tagged statement and a mini-root under `tests/gate/fixture-lint/`.
- A new fixture needs fixtures/src/NAME.s and NAME.checks (headers `id=N` and `checks=N`, rows `index TAB slug TAB class TAB meaning`); every CHECK_LIT or CHECK_GUEST number needs a row; use `z:`, `a:` or `<` on any literal operand below $100; `.include "startup.inc"` (bare name) or `fixtures/lib/NAME` or `build/PATH`; mark raw instruction bytes with `; RAW:<page>`.
- A bare `.include NAME` is judged only by its syntax (no slash), not by whether the file exists in fixtures/lib.
- New MAN: pages must be added to `fixtures/lib/cites.txt` first (use `ocr` unless the page image was read, and record verifier and date).
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/lib/cites.txt, scripts/fixture-lint, scripts/lib/fixture-lint-lex.awk, scripts/lib/fixture-lint-rules.awk, the nine tests/gate/fixture-lint mini-roots. Commits 17d5453 and 3c7ccf1 are ancestors of HEAD.
