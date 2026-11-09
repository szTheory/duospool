---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 03
subsystem: testing
tags: [posix-sh, awk, decision-records, owner-authority, negative-controls, fixture-rights]
status: complete

requires:
  - phase: 01-02
    provides: "decision-check front-matter rules, record 0002, scripts/check decision-negatives stage"
provides:
  - "docs/decisions/0003-0007: owner-recorded API/ABI posture, licence and visibility, fixture rights, CI provider and copied-code provenance principle"
  - "docs/decisions/README.md: record index with status words, checked for drift"
  - "scripts/decision-check: INDEX_MISSING, INDEX_DRIFT, BAD_RECORD_NAME, DUP_RECORD_NUMBER and FIXTURE_RIGHTS_SENTENCE_MISSING rules"
  - "tests/decision/: 17 must-fail controls and 1 must-pass control (ok-protocol-excluded)"
  - ".planning/PROJECT.md Key Decisions pointer rows for records 0002-0008"
affects: [01-04, 01-11, phase-02-ci, fixtures, provenance]

tech-stack:
  added: []
  patterns:
    - "Record is the single source of truth; PROJECT.md holds a pointer plus status word"
    - "Index row status must equal the record front-matter status; the comparison is skipped when the record has no parseable status so one defect gives one reason code"
    - "Fixed-string full-line match (grep -qxF) enforces a rights sentence verbatim"

key-files:
  created:
    - docs/decisions/README.md
    - docs/decisions/0003-api-abi.md
    - docs/decisions/0004-licence-visibility.md
    - docs/decisions/0005-fixture-rights.md
    - docs/decisions/0006-ci-provider.md
    - docs/decisions/0007-copied-code-provenance.md
    - tests/decision/ (17 new case directories)
  modified:
    - scripts/decision-check
    - tests/decision/owner-recorded-no-statement/README.md
    - .planning/PROJECT.md

key-decisions:
  - "Records 0003-0007 use only the CONTEXT.md verbatim owner quotes (D-01, D-02, D-03, D-04, D-05, D-08, D-31); nothing is paraphrased as an owner statement"
  - "0007 marks the D-26 mechanism as Claude's design under the owner's principle, with its own subsection heading"
  - "decision-check now scans every DIR/*.md except README.md and *.protocol.md, so a stray misnamed file is BAD_RECORD_NAME rather than silently ignored"

patterns-established:
  - "Each decision-check rule has a one-defect case directory with record, README.md index and EXPECT"

requirements-completed: [TOOL-01]

actuals:
  tokens: 11950   # chars/4 over the realized git diff
  tasks: 3
  commits: 3
plan_head_before: a51ae50fc3a1e5ef822f940b5b04c540366cc447
plan_head_after: 25a549825329ff4c781bbf72afb6382744363c6e

coverage:
  - id: D1
    description: "Records 0003-0007 exist, each owner-recorded with a verbatim CONTEXT.md owner statement (ROADMAP SC5)"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/decision-check docs/decisions (RECORDS 7, exit 0)"
        status: pass
      - kind: integration
        ref: "sh scripts/check (stage decision-records)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Record 0005 carries the no-GPL sentence and decision-check rejects a 0005 record without it (SC5, D-04)"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "tests/decision/fixture-rights-sentence-missing (EXPECT FIXTURE_RIGHTS_SENTENCE_MISSING)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Index and PROJECT.md pointers match the records, and index drift fails the gate"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "tests/decision/index-drift and tests/decision/index-missing controls"
        status: pass
    human_judgment: false
  - id: D4
    description: "The wording of the records matches the owner's intent (licence choice, rights posture, CI posture)"
    requirement: TOOL-01
    human_judgment: true
    rationale: "The checker verifies structure and quoted text, not that a record states the owner's selection faithfully"
---

# Phase 1 Plan 03: Owner-gated decision records and decision-check hardening Summary

**Five owner-recorded decision records (API/ABI, licence and visibility, fixture rights, CI provider, copied-code provenance) with an indexed README and a decision-check that rejects index drift, bad names, duplicate numbers and a missing no-GPL sentence, each rule with a must-fail control.**

## Performance

- **Duration:** about 15 min (start time was not captured at plan start; estimate)
- **Completed:** 2026-11-09
- **Tasks:** 3 (1 tracer, 1 auto, 1 tdd)
- **Files:** 60 changed (793 insertions, 7 deletions)

## Accomplishments

- Tracer: record 0004 flows through the index and drift check to its PROJECT.md pointer, gate green before any other record was added.
- Records 0003, 0005, 0006 and 0007 written with `owner-statement` quoting the CONTEXT.md option verbatim and citing the 2026-11-09 `/gsd-discuss-phase 1` session. 0005 holds the exact sentence "No GPL or unclear-licence material is admitted without a further owner decision." on its own line.
- PROJECT.md Key Decisions: three existing rows (first profile, API/ABI, fixture rights) and the licence row now point at their records; three rows appended (CI provider, copied-code provenance, evidence-tier policy). "Language and build system" left for plan 01-11.
- `scripts/decision-check` extended; 17 negative cases plus `ok-protocol-excluded` added. `sh scripts/check` and `sh scripts/check --selftest` both pass.

## Task Commits

1. **Task 1: tracer, record 0004, index and drift check** - `687715f` (feat)
2. **Task 2: records 0003, 0005, 0006, 0007 and pointers** - `0f68806` (feat)
3. **Task 3: negative corpus** - `25a5498` (test)

## Deviations from Plan

### Auto-fixed Issues

**1. [Ordering] Task 2 checker rules landed in the Task 1 commit**
- **Found during:** Task 1
- **Issue:** The plan puts BAD_RECORD_NAME, DUP_RECORD_NUMBER and FIXTURE_RIGHTS_SENTENCE_MISSING in Task 2, but they share the same rewritten file-scan loop as the index rules, so they were written together.
- **Fix:** Committed together in `687715f`. Task 2 therefore changed no part of `scripts/decision-check`. Behaviour and acceptance criteria are unaffected.
- **Files modified:** scripts/decision-check
- **Commit:** 687715f

**2. [Scope note] 0007 says the checklist line moves into PROVENANCE.md "when that register ships"**
- **Found during:** Task 2
- **Issue:** The plan says the line lives in PROVENANCE.md "now", but that file ships in plan 01-04 and does not exist yet.
- **Fix:** Worded as "now in this record, and in PROVENANCE.md when that register ships (plan 01-04)". Plan 01-04 should add the line to PROVENANCE.md.
- **Files modified:** docs/decisions/0007-copied-code-provenance.md
- **Commit:** 0f68806

**Total deviations:** 2 (one ordering, one wording to stay accurate). **Impact:** none on acceptance criteria.

## Authentication Gates

None.

## Known Stubs

None.

## Threat Flags

None. No new network, auth or file-access surface; the plan touches documents and a shell checker. T-01-07, T-01-08 and T-01-09 are mitigated by the verbatim-quote rule, the fixed-string sentence check and the index drift check, each with a control.

## Issues Encountered

None.

## Next Phase Readiness

Ready for 01-04. It should add the copied-code PR checklist line to `PROVENANCE.md` and run `scripts/provenance-check` as 0007 describes. Records 0001 (language and build) and the PROJECT.md "Language and build system" row remain for 01-11. The index will need a 0001 row when that record lands.

## Self-Check: PASSED

- Created files exist: docs/decisions/{README.md,0003-api-abi.md,0004-licence-visibility.md,0005-fixture-rights.md,0006-ci-provider.md,0007-copied-code-provenance.md}, 18 directories under tests/decision/.
- Commits 687715f, 0f68806, 25a5498 are ancestors of HEAD; `git rev-list --count` from the plan base gives 3.
- All task acceptance criteria re-run and pass; `sh scripts/check` ends `CHECK RESULT PASS`; `sh scripts/check --selftest` ends `SELFTEST RESULT PASS`; `sh scripts/decision-check docs/decisions` reports RECORDS 7 and exits 0.
