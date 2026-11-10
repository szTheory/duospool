---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 02
subsystem: testing
tags: [sh, awk, gate, conformance, outcomes, result-lines, toml-tsv, negative-controls, selftest-mutation]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: run_controls helper, --stage NAME and --selftest NAME in scripts/check (plan 02-01)
provides:
  - tests/conformance/outcomes.def, the seven outcomes defined once as X-macro rows
  - scripts/conformance-ingest, a separable RESULT-file judge (only pass is green) with a code-to-class table
  - scripts/conformance-flatten, hand-edited TOML to a validated TSV over toml-subset.awk, nothing on stdout unless every input is clean
  - stages ingest-controls and flatten-controls, selftests mutation-ingest-skip-red and mutation-flatten-sentinel
  - 18 fabricated-results controls under tests/gate/ingest and 6 flatten controls under tests/gate/flatten
affects: [02-03, 02-04, 02-05, later plans that emit RESULT lines, read the case TSV or add REQUIRED and inventory files]

actuals:
  tokens: 12900    # chars/4 over the realized diff (git diff d33de3b..HEAD, 88 files, 683 added lines)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD
plan_head_before: d33de3b6087560cee188f4d944559677d0302d35
plan_head_after: de72eda4b0597e9fd253324a04292036c94ff3eb
commits: 3

tech-stack:
  added: []
  patterns:
    - "outcome names are read from outcomes.def by awk; a harness error (exit 2) if the file is empty, has a malformed or duplicated row, or lacks the three names the judge relies on"
    - "one awk array holds the code-to-class table: OUTCOME_<NAME> rows are generated from outcomes.def, the ING_ structural codes are listed once and all map to harness_failure"
    - "a rule whose removal must flip a control carries a single-line trailing marker token, and the selftest assembles the token at run time so the checking script never contains it"
    - "a checker that prints a stream on success (flatten) puts its STATUS marker on stderr, so scripts/check's checker() marker rule holds without a validate-only mode"

key-files:
  created:
    - tests/conformance/outcomes.def
    - scripts/conformance-ingest
    - scripts/conformance-flatten
    - tests/gate/ingest/ (18 case directories: results.txt, tests/conformance/REQUIRED, tests/conformance/cases/*.toml, EXPECT)
    - tests/gate/flatten/ (6 case directories)
  modified:
    - scripts/check

key-decisions:
  - "conformance-ingest defaults --outcomes to the outcomes.def of the repository the script lives in, not ROOT/tests/conformance: a mini-root control supplies its own REQUIRED and cases but judges against the one real outcome set"
  - "a REQUIRED line that is not a valid id is kept as written and reported as OUTCOME_MISSING_ARTIFACT (no RESULT line can carry it), so the code table needs no extra code and nothing is skipped silently"
  - "evidence is optional in the line parser and mandatory in the judge: an absent evidence field and an empty one are both ING_NO_EVIDENCE; a malformed ref (leading slash, dot-dot or empty segment, unknown signal source) is ING_BAD_LINE"
  - "conformance-flatten writes its violations to stderr and prints STATUS flatten ok N on stderr when clean; stdout carries only the TSV or, under --expect, EXPECT_OK"

patterns-established:
  - "Fabricated-results mini-roots: results.txt plus tests/conformance/{REQUIRED,cases/} plus EXPECT, one expected code set per case, so judging needs no C and no build"
  - "A control that run_controls cannot express (mixed good/bad input must print zero stdout bytes) is a run-time probe inside the same stage, built under $tmp"

requirements-completed: [RUN-02, FIX-04]

coverage:
  - id: D1
    description: "Exactly seven outcomes exist and are defined once; only pass is green"
    requirement: RUN-02
    verification:
      - kind: unit
        ref: "grep -c 'DS_OUTCOME(' tests/conformance/outcomes.def = 7; sh scripts/check --stage ingest-controls (one red control per non-pass outcome, six in all)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The judge recomputes the verdict from RESULT lines and turns red on empty input, a missing or wrong RUNNER_DONE, an unknown or upper-case token, a duplicate id, an unlisted id, a pass line with assertions=0, empty evidence, a bad line, an empty REQUIRED and a REQUIRED id with no line"
    requirement: FIX-04
    verification:
      - kind: unit
        ref: "sh scripts/check --stage ingest-controls (18 cases, exact code sets) -> CHECK ingest-controls PASS"
        status: pass
    human_judgment: false
  - id: D3
    description: "Skip is red because of one rule, mutation-proven"
    requirement: RUN-02
    verification:
      - kind: unit
        ref: "sh scripts/check --selftest mutation-ingest-skip-red -> SELFTEST mutation-ingest-skip-red PASS (mutated judge prints STATUS conformance ok on a skip)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Ingest violation codes map to outcome classes per D-79(c)"
    requirement: RUN-02
    verification:
      - kind: unit
        ref: "raw lines: unknown-token class=harness_failure, required-no-line class=missing_artifact, skip class=skip; every ING_ code is harness_failure class"
        status: pass
    human_judgment: false
  - id: D5
    description: "conformance-flatten releases a TSV only when every input is clean, ending in END plus the file count"
    requirement: FIX-04
    verification:
      - kind: unit
        ref: "sh scripts/check --stage flatten-controls (5 red controls, ok case, mixed good/bad directory prints 0 stdout bytes, TAB in name refused) -> PASS; sh scripts/check --selftest mutation-flatten-sentinel -> PASS"
        status: pass
    human_judgment: false

duration: 5 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 02: Outcome model, RESULT-line judge and TOML flattener Summary

**The seven-outcome table (`outcomes.def`), a fail-closed RESULT-file judge (`conformance-ingest`, only `pass` is green) and the single-parser TOML-to-TSV wrapper (`conformance-flatten`) now exist in sh and awk before any C does, each with fabricated-input controls shown red and two mutation selftests.**

## Performance

- **Duration:** 5 min (2026-11-10T03:40:28Z to 03:46:20Z)
- **Tasks:** 3 of 3
- **Files:** 88 changed (3 scripts and outcomes.def created, `scripts/check` modified, 24 control case directories)
- **Gate cost:** the two new stages measure 1.14 s (ingest-controls, 18 cases) and 0.41 s (flatten-controls) on this machine; the full `sh scripts/check` went from about 10 s to about 16 s in one measurement. An earlier run of the same gate took 31 s on a loaded machine, so treat absolute numbers as noisy.

## Accomplishments

- `tests/conformance/outcomes.def` holds the seven rows `pass guest_failure timeout harness_failure skip cancelled missing_artifact`. `conformance-ingest` reads the closed set from it by awk and keeps no literal list of outcome names; an empty, malformed or duplicated def file is a harness error (exit 2), because guessing the set could turn an unknown token into a pass.
- `conformance-ingest --results FILE --required FILE --inventory DIR [--outcomes FILE] [--root DIR] [--expect CODES]` parses each RESULT line strictly (single spaces, lower-case id, `assertions=N`, `evidence=REF`, optional `machine_time=N` then `stop=REASON`), checks `RUNNER_DONE N` against the count of RESULT lines, and cross-checks REQUIRED. Violations print as `CODE<TAB>class=NAME<TAB>detail`, taken from one code-to-class table. Marker `STATUS conformance ok` only on a clean run; exit 0, 1 or 2.
- 18 fabricated-results controls, one per non-pass outcome (6) and per structural rule (12), each with an exact-set EXPECT: guest failure, timeout, harness failure, skip, cancelled (assertions=0 with evidence `signal:TERM` yields `OUTCOME_CANCELLED` only), missing artifact, duplicate id, unlisted id, zero tests (`ING_EMPTY` plus `OUTCOME_MISSING_ARTIFACT`, no `ING_NO_DONE`), no done marker, wrong done count, pass with assertions=0, empty evidence, bad line, empty REQUIRED, unknown upper-case token, REQUIRED id with no line, plus the one passing sample.
- `mutation-ingest-skip-red`: deleting the single `rule-non-pass` line from a scratch copy makes the same judge print `STATUS conformance ok` on a skip (and `EXPECT_MISMATCH wanted 'OUTCOME_SKIP' got '<none>'` under the control's expectation), so the redness of skip depends on exactly that rule.
- `conformance-flatten [--expect CODES] PATH...` runs `toml-subset.awk` per file without a pipeline, classifies every stage 1 row (ERR rows pass their code through; the END row must occur once and last, else `FLAT_NO_SENTINEL`; a row of unexpected shape is `FLAT_BAD_ROW`), and releases the TSV only after every input is clean: `K<TAB>stem<TAB>table<TAB>key<TAB>type<TAB>value` rows and a final `END<TAB>n`. On any error stdout is empty and exit is 1. `profiles/pce-hucard-2btn/r1.toml` yields 13 distinct non-empty table names.
- `mutation-flatten-sentinel`: with `toml-subset.awk` replaced by a program that prints nothing, flattening `ok-two-files` exits 1, prints zero bytes and reports `FLAT_NO_SENTINEL`; the unmutated copy exits 0 and ends in `END`.

## Task Commits

1. **Task 1 (tracer): outcomes.def, conformance-ingest, ingest-controls stage with four controls** - `748ae6f` (feat)
2. **Task 2: the remaining 14 ingest controls and mutation-ingest-skip-red** - `03f97ea` (test)
3. **Task 3: conformance-flatten, its controls and probes, mutation-flatten-sentinel** - `de72eda` (feat)

## Verification Results

Tracer gate (auto mode): after the Task 1 commit `sh scripts/check --stage ingest-controls` was re-run and passed; log line `Tracer verified end-to-end - expanding`, then Tasks 2 and 3.

Per-task `<automated>` commands (stage granularity, D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check --stage ingest-controls` (Task 1, four cases) | exit 0, `CHECK ingest-controls PASS` |
| `sh scripts/check --stage ingest-controls && sh scripts/check --selftest mutation-ingest-skip-red` (Task 2, 18 cases) | exit 0, `CHECK ingest-controls PASS`, `SELFTEST mutation-ingest-skip-red PASS` (also under `dash`) |
| `sh scripts/check --stage flatten-controls && sh scripts/check --selftest mutation-flatten-sentinel` (Task 3) | exit 0, `CHECK flatten-controls PASS`, `SELFTEST mutation-flatten-sentinel PASS` |

Acceptance criteria, all run: ok-all-pass prints `STATUS conformance ok`, exit 0; the skip control exits 0 only with `--expect OUTCOME_SKIP` and exits 1 without it, its raw line carries `class=skip`; unknown-token raw line `class=harness_failure`; required-no-line raw line `class=missing_artifact`; `grep -c 'DS_OUTCOME('` = 7; `grep -c DS_OUTCOME scripts/conformance-ingest` = 4; SPDX count 1 for outcomes.def, conformance-ingest and conformance-flatten; `ls tests/gate/ingest | wc -l` = 18 and every EXPECT (17 negatives) non-empty; zero-tests raw output has `ING_EMPTY` and `OUTCOME_MISSING_ARTIFACT` and no `ING_NO_DONE`; flatten `ok-two-files` ends with `END	2`, exit 0; flatten `bad-syntax` prints 0 stdout bytes, exit 1.

Plan-end (wave-end) commands:

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 20 stage PASS lines (18 earlier plus ingest-controls and flatten-controls), `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | eight `SELFTEST ... PASS` lines (the six before, plus the two new), `SELFTEST RESULT PASS`, exit 0 |
| `dash scripts/check` | `CHECK RESULT PASS`, exit 0 |
| each non-pass outcome in outcomes.def has a fabricated control judged red | guest_failure, timeout, harness_failure, skip, cancelled and missing_artifact each map to their own `OUTCOME_*` line with `class=<name>` |

Control sanity beyond the plan: a copy of `conformance-flatten` that prints the rows before judging makes `flatten-controls` report `CONTROL BROKEN mixed-good-and-bad (exit 1, 162 stdout bytes ...)`, so the probe is not vacuous. Only the macOS BWK awk (20200816) is installed here; gawk and mawk were not available to test.

## Files Created/Modified

- `tests/conformance/outcomes.def` - seven outcome rows with a C-style SPDX header (MIT OR Apache-2.0).
- `scripts/conformance-ingest` - the judge, with the single `rule-non-pass` line.
- `scripts/conformance-flatten` - the flattener.
- `scripts/check` - `stage_ingest_controls`, `stage_flatten_controls`, two stages registered after results-negatives, selftest mutations (g) and (h).
- `tests/gate/ingest/<18 cases>/` and `tests/gate/flatten/<6 cases>/` - controls; case TOML stubs and REQUIRED files carry a 0BSD header.

## Decisions Made

See `key-decisions` above. The plan fixed the CLI, codes and markers; the choices left open were where the outcomes default points for a mini-root, how a malformed REQUIRED id is reported, and how flatten satisfies the gate's marker rule.

## Deviations from Plan

### Auto-fixed Issues

None. No bug, missing critical functionality or blocker needed a Rule 1-3 fix.

### Additions and interpretation choices (inside the plan's intent)

1. **Flatten marker on stderr.** `checker()` in `scripts/check` treats an exit 0 without `EXPECT_OK`, `STATUS ` or `RECORDS ` as a dead checker, and flatten's clean output is a TSV. Rather than weaken `checker()` or add a validate-only mode, flatten prints `STATUS flatten ok N` to stderr on a clean run (checker merges stderr). stdout stays TSV-only, which is what the C runner reads.
2. **`FLAT_BAD_ROW` and `BAD_FILENAME` codes in flatten.** The plan lists `FLAT_NO_INPUT`, `FLAT_NO_SENTINEL` plus pass-through codes. Flatten also refuses a stage 1 row of unexpected shape (`FLAT_BAD_ROW`) and a TAB or newline in a path (`BAD_FILENAME`, as in the other checkers), because a TAB in a stem would forge a TSV row. The TAB-in-name case is exercised by a run-time probe inside `flatten-controls`; `FLAT_BAD_ROW` is defensive and has no control (stage 1 cannot currently produce it).
3. **Extra probes in `stage_flatten_controls`.** Besides the mixed-directory probe the plan names, the stage checks that `ok-two-files` prints K rows and ends with `END<TAB>2`, and that a TAB-named file gives `BAD_FILENAME`.
4. **Ingest harness errors.** Missing results, REQUIRED, inventory directory or outcomes file, a malformed outcomes.def, or a TAB or newline in an inventory file name exit 2 (`HARNESS_NO_FILE`, `HARNESS_BAD_OUTCOMES`, `HARNESS_BAD_FILENAME`); none has a control because the plan's code list is closed and the 18-case count is an acceptance criterion.
5. **A line after `RUNNER_DONE` is `ING_BAD_LINE`, and extra `RUNNER_DONE` lines are `ING_DONE_COUNT`.** The grammar says RUNNER_DONE is last; no dedicated control (also within the fixed 18).
6. **Field order.** `machine_time=` must precede `stop=`; a RESULT line with them swapped is `ING_BAD_LINE`.
7. **`assertions-zero` and the cancelled control** follow the plan literally: `ING_ASSERTIONS_ZERO` only for pass lines, the cancelled line carries `assertions=0`.

**Total deviations:** 0 auto-fixed, 7 additive or interpretive notes. **Impact:** none on scope; the judge and flattener are slightly stricter than specified.

## Issues Encountered

- The repository branch name still reads `gsd/phase-1-...` while the work is Phase 2; the executor instructions said to stay on the current branch, so no branch was changed.
- Pre-existing `.planning/state.json` (untracked) was left alone.

## Known Stubs

None. (The case TOML stubs under `tests/gate/ingest/*/tests/conformance/cases/` hold only an `id` key by design; they exist to give the judge an inventory, not to run.)

## Threat Flags

None. No new network endpoint or auth path. The new file-access patterns are read-only scans of a mini-root and `mktemp -d` scratch files. T-02-03 and T-02-04 are mitigated by the 18 ingest controls and the skip mutation; T-02-05 by the mixed-directory zero-byte probe and `mutation-flatten-sentinel`; T-02-SC accepted (no installs).

## Next Phase Readiness

Plan 02-03 can feed real runner stdout to `sh scripts/conformance-ingest --results FILE` (the default REQUIRED and cases paths are `tests/conformance/REQUIRED` and `tests/conformance/cases`, created by plan 05) and add the `cancelled` producer; later plans read case data with `sh scripts/conformance-flatten tests/conformance/cases`. A real `tests/conformance/REQUIRED` with zero id lines is `ING_REQUIRED_EMPTY`, and any non-`pass` line stays red.

## Self-Check: PASSED

- FOUND: `/Users/jon/projects/duospool/tests/conformance/outcomes.def`, `/Users/jon/projects/duospool/scripts/conformance-ingest`, `/Users/jon/projects/duospool/scripts/conformance-flatten`, `/Users/jon/projects/duospool/tests/gate/ingest` (18 directories), `/Users/jon/projects/duospool/tests/gate/flatten` (6 directories)
- FOUND commits (ancestors of HEAD): 748ae6f, 03f97ea, de72eda
- `git rev-list --count d33de3b6087560cee188f4d944559677d0302d35..de72eda4b0597e9fd253324a04292036c94ff3eb` = 3
