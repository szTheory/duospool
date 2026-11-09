---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 01
subsystem: testing
tags: [posix-sh, awk, strict-toml-subset, claim-ledger, decision-records, negative-controls]

requires: []
provides:
  - "sh scripts/check: Phase 1 gate entry point (D-30) with stages ledger-entries, ledger-negatives, ledger-samples, decision-records, decision-negatives and a --selftest flag"
  - "scripts/ledger-check and scripts/lib/{toml-subset,ledger-rules}.awk: two-stage strict-subset ledger checker with END sentinel, --expect code sets, --required, DUP_ID and sorted deterministic output"
  - "scripts/decision-check: decision-record closed front-matter checker (D-25)"
  - "ledger/: SCHEMA.md, REQUIRED, seed entry LED-AUD-0001, 23 negative control directories, 7 must-pass samples"
  - "docs/decisions/0008-evidence-tier-policy.md (proposed-default, D-29)"
affects: [01-02, 01-03, phase-02-gate, ledger, decision-records]

actuals:
  tokens: 24400   # chars/4 over the realized git diff; includes the padded 16 KiB boundary control files
  tasks: 3
  commits: 5
plan_head_before: 7edd4695a05beec1d2dab0fc15321cc7b7809a1d
plan_head_after: 6b31fd242e89d6c86f3b2ee87b0d64794cec41d3

tech-stack:
  added: []
  patterns:
    - "Two-stage checker: stage 1 syntax to flat TAB records ending in an END sentinel, stage 2 schema rules; no pipeline hides a stage failure"
    - "Negative control = directory with an EXPECT file holding the exact reason-code set (D-38)"
    - "Fail-closed gate: silent exit 0 from a checker (CHECKER_DEAD), empty stage 2 (STAGE2_DEAD), zero controls (NO_CONTROLS) and missing sentinel (NO_SENTINEL) all fail"

key-files:
  created:
    - scripts/check
    - scripts/ledger-check
    - scripts/decision-check
    - scripts/lib/toml-subset.awk
    - scripts/lib/ledger-rules.awk
    - ledger/SCHEMA.md
    - ledger/REQUIRED
    - ledger/entries/LED-AUD-0001.toml
    - ledger/negative/
    - ledger/samples/
    - docs/decisions/0008-evidence-tier-policy.md
    - tests/decision/owner-recorded-no-statement/
  modified: []

key-decisions:
  - "Checkers print a marker on a clean exit (EXPECT_OK, STATUS, RECORDS) and scripts/check treats a silent exit 0 as CHECKER_DEAD, because macOS /bin/sh (bash 3.2) exits 0 on a script syntax error"
  - "Control-case files for float-value and wrong-type use status open so each yields exactly one reason code"
  - "Evidence-tier policy 0008 stays proposed-default; the owner was not asked (D-29)"

patterns-established:
  - "Every new script carries the two SPDX header lines (D-01, D-31) after the shebang"
  - "Names reach awk through ENVIRON, never awk -v; no eval, system() or data-named getline"

requirements-completed: [PROF-04]

coverage:
  - id: D1
    description: "A claim-ledger entry marked supported with only E0 and E4 evidence is rejected for exactly SUPPORTED_NO_QUALIFYING_EVIDENCE (ROADMAP SC3)"
    requirement: PROF-04
    verification:
      - kind: integration
        ref: "sh scripts/ledger-check --expect SUPPORTED_NO_QUALIFYING_EVIDENCE ledger/negative/e0-e4/"
        status: pass
      - kind: integration
        ref: "sh scripts/check --selftest (mutation-supported-rule)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The full D-23 negative corpus (E0-only, E4-only, E1 REPORT-only, supported with PENDING, DUP_ID, EMPTY_LEDGER, EMPTY_FILE, NO_SOURCE, DUP_KEY, size boundary and more) fails with exactly its EXPECT code set; samples pass"
    requirement: PROF-04
    verification:
      - kind: integration
        ref: "sh scripts/check (stages ledger-negatives, ledger-samples)"
        status: pass
    human_judgment: false
  - id: D3
    description: "decision-check rejects an owner-recorded record without an owner-statement and accepts record 0008 (proposed-default)"
    requirement: PROF-04
    verification:
      - kind: integration
        ref: "sh scripts/check (stages decision-records, decision-negatives)"
        status: pass
    human_judgment: false
  - id: D4
    description: "scripts/check and the checkers give the same results under gawk and mawk on Linux as under macOS BWK awk"
    requirement: PROF-04
    verification: []
    human_judgment: true
    rationale: "Not run: neither gawk nor mawk is installed on this machine. This is the declared backstop (RESEARCH A1); the scheduled Linux job is Phase 2"

duration: ~60min
completed: 2026-11-09
status: complete
---

# Phase 1 Plan 01: Gate skeleton and claim-ledger checker Summary

**`sh scripts/check` is a fail-closed POSIX sh+awk gate that parses a real PSG-clock ledger entry, rejects an E0/E4-only "supported" control for exactly SUPPORTED_NO_QUALIFYING_EVIDENCE, validates decision-record front matter, and proves itself non-vacuous with `--selftest`.**

## Performance

- **Duration:** about 60 min (the start time was not recorded; this is an estimate)
- **Completed:** 2026-11-09
- **Tasks:** 3 (1 tracer, 1 TDD task, 1 auto)
- **Commits:** 5 task commits, measured from the plan ledger
- **Gate runtime:** about 3 s under macOS `/bin/sh` (budget 5 s)

## Accomplishments

- **Tracer (Task 1):** one command runs the whole slice end to end: stage-1 strict TOML subset reader with END sentinel, stage-2 ledger rules, decision-record front-matter checker, seed entry LED-AUD-0001 (PSG clock, `assumed`, no evidence, worded as a modelling assumption), the e0-e4 control, and record 0008 as `proposed-default`. The tracer was verified end to end before expansion.
- **Hardening (Task 2):** 22 further negative-control directories (23 in all, with e0-e4) and 7 must-pass samples. Both sides of the 16 KiB size boundary are pinned (16384 bytes accepted, 16385 rejected). Added `ledger-samples`, `--required` handling (REQUIRED_MISSING, REQUIRED_ORIGIN_MISMATCH, REQUIRED_BAD_LINE), cross-file DUP_ID, EMPTY_LEDGER, and `ledger/SCHEMA.md` with the "structure, not truth" sentence and the E3 path-shape limit paragraph.
- **Non-vacuity (Task 3):** `decision-negatives` stage and `--selftest`, which mutates a temp copy of `scripts/` (removes the supported rule; silences stage 1) and requires the gate to go red. The tracked scripts are never touched.
- Mutation probes beyond the plan (15 rules and wrapper checks removed in turn from a copy) broke at least one control each, except the EMPTY_LEDGER probe, which exposed the silent-exit-0 hazard fixed in Deviation 1.

## Task Commits

1. **Task 1: tracer** - `6676cb6` (feat)
2. **Task 2 RED/corpus** - `1b2fea7` (test)
3. **Task 2 GREEN/hardening** - `9f7cc66` (feat)
4. **Task 3: decision-negatives and --selftest** - `12668a4` (feat)
5. **Fail-closed fix for dead stage 2 / decision awk** - `6b31fd2` (fix)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Fail-open on a silent exit 0**
- **Found during:** Task 2 (mutation probing)
- **Issue:** macOS `/bin/sh` (bash 3.2) exits 0 when a script has a syntax error, so a broken checker would read as a pass. `dash -n` lint alone does not protect a gate run through `/bin/sh`.
- **Fix:** each checker prints a marker line on a clean exit (EXPECT_OK, STATUS, RECORDS); `scripts/check` treats exit 0 without a marker as CHECKER_DEAD (harness error, exit 2).
- **Files modified:** scripts/check, scripts/ledger-check, scripts/decision-check
- **Commit:** 9f7cc66

**2. [Rule 2 - Missing critical functionality] Dead stage 2 / dead decision awk read as no violations**
- **Found during:** Task 3 (mutation probing)
- **Issue:** a failing stage-2 awk in ledger-check, or a failing awk in decision-check, produced empty output and therefore a pass.
- **Fix:** ledger-check emits STAGE2_DEAD when stage 2 yields nothing; decision-check exits 2 (HARNESS_AWK_FAILED).
- **Files modified:** scripts/ledger-check, scripts/decision-check, ledger/SCHEMA.md
- **Commit:** 6b31fd2

**3. [Rule 1 - Bug] Loop variable aliasing in ledger-rules.awk**
- **Found during:** Task 2 mutation probe (an infinite loop when a rule line was deleted)
- **Issue:** the WRONG_TYPE loop reused `k`, which later served as a numeric count; a string left in `k` made a later `for` condition compare as strings.
- **Fix:** renamed the loop variable to `kk`.
- **Commit:** 9f7cc66

**4. [Rule 2 - Hardening] TAB inside a quoted value rejected (BAD_STRING)**
- **Issue:** a TAB in a value would corrupt the TAB-separated record stream between stages.
- **Fix:** `qstr()` in toml-subset.awk returns failure on TAB. Also suppressed cascading MISSING_FIELD/ASSUMED_* codes for a key that already produced a syntax error, so single-defect controls give a single code.
- **Commit:** 6676cb6

### Plan sequencing

- **Task 2 RED phase collapsed.** The wrapper needed DUP_ID, EMPTY_LEDGER and `--required` to exist for the tracer's gate design, so they were implemented in Task 1. When the Task 2 corpus was committed (`1b2fea7`) it already passed. The RED evidence classifier does not cover shell harnesses. As compensation, each rule was removed in turn from a copy and the matching control was confirmed to break (semantic assessment: every EXPECT fires for the intended reason). I did not fabricate a failing run.
- **`ledger/REQUIRED` and the `--required` hookup** were created in Task 2 as specified. The `ledger-entries` stage uses `--required ledger/REQUIRED` from Task 2 on.

### Branch deviation (needs orchestrator attention)

The working tree was on `main`, which the guard resolves as protected (`git.base-branch --is-protected main` returned true; `git.allow_default_branch_commits` is unset). I did not change configuration. I created and committed on `gsd/phase-1-owner-decisions-claim-ledger-and-toolchain-spike` (the repo's configured phase-branch template shape). `main` is untouched at `7edd469`. The orchestrator should fast-forward or merge this branch, or set `git.allow_default_branch_commits` if it wants later plans to commit on `main`.

## Authentication Gates

None.

## Known Stubs

None. The `PENDING:5` pin on LED-AUD-0001 is the intended state of an `assumed` entry, not a stub.

## Threat Flags

None. Threat register items T-01-01 (size cap, line cap, ASCII, ENVIRON, no eval; pinned by too-big, long-line, float-value), T-01-02 (sentinel, NO_CONTROLS, exit-2 distinction, selftest) and T-01-03 (verbatim owner-statement) are mitigated as planned. Prohibitions held: nothing is marked supported on E0/E4 alone (the only supported entries are labelled controls), and no record is `owner-recorded` (record 0008 stays `proposed-default`).

## Decisions Made

- Fail-closed marker lines on every checker (see Deviation 1).
- Per-case controls use `status = "open"` where the defect concerns an optional key, so each case yields exactly one code.
- `ok-supported-e3` cites `docs/decisions/0002-first-profile.md`; the Phase 1 checker validates only the path shape (stated in SCHEMA.md). Plan 01-02 must create 0002 as `owner-recorded` and assert that the reference resolves.

## Issues / Backstops

- **gawk/mawk not run:** neither is installed here; all code was written to the POSIX-awk rules in RESEARCH Pitfall 2 and run under BWK awk 20200816 and `dash`/`/bin/sh` (bash 3.2). The scheduled Linux job is Phase 2 (RESEARCH A1).
- `scripts/decision-check` has no README-index or `0005` sentence checks; those belong to later plans.

## Next Phase Readiness

Plans 01-02 and 01-03 can append REQUIRED lines and seed entries, add `tests/decision/ok-*` and further negatives without editing `scripts/check`. Phase 2 extends the same `scripts/check`.

## Self-Check: PASSED

- Files found: scripts/check, scripts/ledger-check, scripts/decision-check, scripts/lib/toml-subset.awk, scripts/lib/ledger-rules.awk, ledger/SCHEMA.md, ledger/REQUIRED, ledger/entries/LED-AUD-0001.toml, docs/decisions/0008-evidence-tier-policy.md, tests/decision/owner-recorded-no-statement/EXPECT
- Commits found: 6676cb6, 1b2fea7, 9f7cc66, 12668a4, 6b31fd2
- `sh scripts/check` ends `CHECK RESULT PASS`; `sh scripts/check --selftest` ends `SELFTEST RESULT PASS`
