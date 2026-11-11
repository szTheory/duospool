---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 38
subsystem: docs
tags: [readme, docs-check, ledger, timing-baseline, w1-close]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: ca65 and ld65 on the preflight list (03-06), cc65 CI install step (03-07), run_until_abs and run_chunk_sum cases (03-02)
provides:
  - README Phase 3 status in withheld-claim wording and cc65 (ca65, ld65) in the tool prerequisites
  - docs-check rule DOCS_TOOL_MISSING (rule-tool-named) with the red control tests/docs/missing-tool
  - LED-CPU-0003 and LED-CPU-0004 as assumed entries pinned to run_until_abs and run_chunk_sum, with REQUIRED lines and pins back-links
  - LED-PROF-0002 unit and mapping register clauses marked superseded (D-91, D-114)
  - W1 local timing baseline in docs/ci-timings.md, CI recorded as unverified
affects: [03-08, 03-32]

actuals:
  tokens: 3100
  tasks: 2
  commits: 2
plan_head_before: 581916faab3f5df357a0865d82abbffbaf128360
plan_head_after: b67730c629bea0dddd529a7730905e825b82c62e

tech-stack:
  added: []
  patterns:
    - "docs-check reads the default tool list from the tools=\"...\" line of ROOT/scripts/preflight and requires each entry (any alternative of a|b) to appear in the README as a whole word, so cc is not found inside cc65; a root without scripts/preflight skips the rule"

key-files:
  created:
    - tests/docs/missing-tool/
    - ledger/entries/LED-CPU-0003.toml
    - ledger/entries/LED-CPU-0004.toml
  modified:
    - README.md
    - scripts/docs-check
    - ledger/entries/LED-PROF-0002.toml
    - ledger/REQUIRED
    - tests/conformance/cases/run_until_abs.toml
    - tests/conformance/cases/run_chunk_sum.toml
    - docs/ci-timings.md

key-decisions:
  - "The tool-named rule is skipped for a root with no scripts/preflight, so the five older docs mini-roots stay unchanged; the real repository has the file and its preflight stage fails without it"
  - "LED-CPU-0003 and LED-CPU-0004 name in their assumption text what the pinned case does and does not exercise: run_until_abs pins the low-speed 12 only, run_chunk_sum pins chunk invariance only"

requirements-advanced: [FIX-01, CORE-04, CPU-06]
requirements-completed: []  # FIX-01 needs the encoding-matrix oracle (03-08); CORE-04 and CPU-06 continue in later waves

duration: 25min
completed: 2026-11-11
---

# Phase 3 Plan 38: README, docs-check tool rule, W1 ledger entries and timing baseline Summary

**The README now names the assembler it requires and a docs-check rule with a red control keeps the prerequisite list in step with the preflight tool list; W1's time and step model sits in the ledger as two assumed entries with real pins, and the W1 local gate time (62 s warm, 50 stages) is recorded against the 120 s target with CI marked unverified.**

## What was built

- **Task 1 (tracer), commit e2d181d.** README Status rewritten: Phase 3 in progress, an authored-fixture bounded run in master clocks, no accuracy, compatibility or boot claim, a matching hash proves repeatability only. Tool prerequisites add `ca65` and `ld65` (`brew install cc65` locally, `apt-get install cc65` in CI). `scripts/docs-check` gained `DOCS_TOOL_MISSING` (tag `rule-tool-named`). Control first: with the mini-root `tests/docs/missing-tool` added and no rule, `--stage docs-claims-negatives` showed `CONTROL BROKEN missing-tool` (`EXPECT_MISMATCH wanted DOCS_TOOL_MISSING got <none>`); after the rule, both docs stages are green and the real README passes. The mini-root is the ok-docs shape plus a stub `scripts/preflight` holding the real `tools=` line, with a README that names everything except `ca65`.
- **Task 2, commit b67730c.** `LED-CPU-0003` (master clock unit, /3 high, /12 low, timer /3072, frequency a label only) and `LED-CPU-0004` (instruction-atomic steps, stop points, in-instruction offset rule, zero reset cost), both assumed, every schema key, four-plus withheld items, assumption, revisit_phase 3, origin `03-CONTEXT D-90` and `D-92`, REQUIRED lines, pins `TEST:run_until_abs` and `TEST:run_chunk_sum`, back-linked in the cases' `pins`. LED-CPU-0003 sources: FACT H8-20 and H8-1 each with verifier (gsd-phase-researcher), date (2026-11-10) and the fact, taken from 03-RESEARCH and stated as not re-read here (R-8); REPORT for the /12 ratio, with no manual page claimed. `LED-PROF-0002`: statement and assumption mark the unit clause superseded by D-91 and the mapping register clause by D-114; id, origin, REQUIRED line and pin unchanged, `updated` bumped. `docs/ci-timings.md`: new section "Local baseline after W1" with the 50-row stage table, the 120 s target verdict, a short efficiency review and the line `CI for the W1 changes: unverified, pending owner push authorisation (D-64)`.

## Verification

- Task 1 verify (`--stage docs-claims`, `--stage docs-claims-negatives`): PASS. Task 2 verify (`--stage ledger-entries`, `--stage inventory`, `--timing`): PASS; `ledger-negatives` and `inventory-negatives` also PASS.
- `sh scripts/check --timing`: PASS, 62 s wall, 50 stages, no `TIMING_WARN`, load average about 10 to 13 on a shared machine. `sh scripts/check --selftest`: PASS, 14 s. Final plan-end runs (gate, selftest, audit-history) are listed in the final report.

## Deviations from Plan

None - plan executed as written. Notes: the docs-check tool rule skips roots without `scripts/preflight` (the plan names only the new mini-root as carrying one); I did not add a selftest mutation for `rule-tool-named`, because `scripts/check` is not in the plan's file list; the red control `missing-tool` is the proof the rule is alive.

## Evidence discipline

E0 only. Both new entries are assumed, with E0 evidence and a pin that exercises only part of the statement (said in each assumption). No ledger status was promoted and none of the eight ER-2 candidates was touched. The H8-20 and H8-1 facts are recorded from the research's image read and are not re-read by this plan; the /12 low-speed ratio has no manual basis and is labelled community-reported. The timing table is a local figure on a loaded machine and not a CI measurement; no CI row exists for W1. No accuracy, compatibility, cycle-accuracy, hardware or boot claim.

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-22 (README wording with a red control) and T-03-97 (both entries assumed with real pins; `ledger-check` rejects supported without E1 FACT) are mitigated as planned.

## Notes for next plans

- Stale text: the LED-PROF-0002 source row "hardware PSG noise and VDC phase behaviour" still says Phase 2 evolves them with placeholder rules; the placeholders are gone from src, but source rows are history and were left unchanged.
- LED-CPU-0003 gets its high-speed 3 and timer step pins in later cases; update `pinning_test` only if a better single case lands (one entry pins one case, F9).
- The first CI run after an authorised push adds the `apt-get install cc65` step to the critical path; add its row and review to `docs/ci-timings.md` then. Pending owner gate: push authorisation (D-64).
- `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: tests/docs/missing-tool (README.md, AGENTS.md, EXPECT, scripts/check, scripts/preflight), ledger/entries/LED-CPU-0003.toml, ledger/entries/LED-CPU-0004.toml. Commits e2d181d and b67730c are ancestors of HEAD.
