---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 09
subsystem: gate
tags: [toy-controls, control-mutate, wrong-mirror, size-refusal, conformance-gate, ledger, cmake]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: the plain mapper with the size enum and the one-line 384K rule (02-08); the runner, the refuse kind and the len=N request override (02-06, 02-07); conformance-run run mode and the ds_core glob (02-05)
provides:
  - scripts/control-mutate, the generator of a toy control source from the live file and a rule-tagged literal substitution (exit 2 and CONTROL_ANCHOR_COUNT TAG n=N unless the anchor occurs exactly once)
  - CMake discovery of tests/controls/NAME/RULE, one EXCLUDE_FROM_ALL target ds_toy_NAME per rule, built from the ds_core sources with the one named file replaced by the generated variant
  - conformance-run controls mode (CONTROL NAME RED_OK or NOT_RED, field-wise id and outcome match, exit 2 on a missing or empty EXPECT) inside the conformance stage
  - selftest mutation-control-anchor (SELFTEST mutation-control-anchor NAME PASS per rule)
  - the toy tests/controls/wrong-mirror (384K as b and 0x3F), shown red by accept_384k_banksig
  - cases ref_size_8192 and ref_size_512k (22 required cases in all) and the open ledger entry LED-LOAD-0004
affects: [02-10 (sanitizer stage reruns the cases), 02-11 (ref_size_2m_too_large and the rest of the refusal matrix), the next three toys (heuristic-reverse, drop-region, shared-state) reuse the mechanism with a RULE and an EXPECT only, 02-17 (fixture manifest)]

actuals:
  tokens: 6198     # chars/4 over the realized diff (git diff plan_head_before..HEAD excluding .planning, 24793 chars, 11 files, 402 added and 1 removed line)
  tasks: 2
  commits: 2       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 90f8f8d5ff5340824ca83d3862363eeef8dba8de
plan_head_after: 29dcb2d268665023d30b39dda6194370330dac92
commits: 2

tech-stack:
  added: []
  patterns:
    - "a control is a generated, never committed, one-line variant of one live file: tests/controls/NAME holds only RULE and EXPECT, so an edit of the live file cannot leave a stale or uncompilable snapshot"
    - "a rule whose anchor matches other than once is a harness error (exit 2), and a selftest breaks and duplicates each anchor in a scratch root to prove it"
    - "toy results are judged by awk field match on the RESULT line (id and outcome), by the harness, never by the runner"

key-files:
  created:
    - scripts/control-mutate
    - tests/controls/wrong-mirror/RULE
    - tests/controls/wrong-mirror/EXPECT
    - tests/conformance/cases/ref_size_8192.toml
    - tests/conformance/cases/ref_size_512k.toml
    - ledger/entries/LED-LOAD-0004.toml
  modified:
    - CMakeLists.txt
    - scripts/conformance-run
    - scripts/check
    - tests/conformance/REQUIRED
    - ledger/REQUIRED

key-decisions:
  - "control-mutate prints CONTROL_ANCHOR_COUNT on stdout and exits 2; it leaves the output file alone when the mutated text equals what is already there, so the build does not redo work on an unchanged toy"
  - "controls_mode checks every control's RULE and EXPECT (present, non-empty after comments, ids well formed and carried by a case) before the first toy is built, so a bad control is reported before any build"
  - "a toy that does not compile is a harness error (exit 2), not a red result: a control that cannot build proves nothing"
  - "no control at all is NO_CONTROLS, status 1: a gate with the mechanism but no toy is dead, not green"
  - "CMake refuses (FATAL_ERROR) a rule whose file= is not a source of ds_core, and re-configures when a RULE file is edited (CMAKE_CONFIGURE_DEPENDS), because the glob only sees a rule added or removed"

patterns-established:
  - "a new toy is two files: tests/controls/NAME/RULE (tag, file, anchor, replace) and EXPECT (case ids that must be guest_failure); no CMake, script or runner change"
  - "the stage prints CONTROL lines only through conformance-run; scripts/check shows them on a failing stage, and sh scripts/conformance-run run shows them always (also written to build/PRESET/controls.txt)"

requirements-completed: [CONT-01]

coverage:
  - id: D1
    description: "A toy library whose 384K mirror is b and 0x3F, generated from the live src/core/mapper.c, is shown guest_failure by accept_384k_banksig while the real core passes the same case"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "sh scripts/conformance-run run -> RESULT accept_384k_banksig pass (real core), CONTROL wrong-mirror RED_OK, STATUS conformance ok n=22; with replace equal to anchor: CONTROL wrong-mirror NOT_RED and sh scripts/check --stage conformance FAIL (exit 1)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A rule whose anchor matches zero times or more than once is a harness error, and an empty or absent EXPECT is a harness error"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "sh scripts/check --selftest mutation-control-anchor -> SELFTEST mutation-control-anchor wrong-mirror PASS (broken anchor n=0 and duplicated line n=2, both exit 2); observed once each: anchor edited to match nothing -> CONTROL_ANCHOR_COUNT rule-wrong-mirror-384k n=0 and exit 2; empty EXPECT, comment-only EXPECT -> HARNESS_EMPTY_EXPECT exit 2; absent EXPECT -> HARNESS_NO_EXPECT exit 2"
        status: pass
    human_judgment: false
  - id: D3
    description: "Sizes 8192 and 524288 are refused load.size_unsupported by the exact three-size rule, and the first content byte is touched only after every length, cap and size decision"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "RESULT ref_size_8192 pass assertions=5 and RESULT ref_size_512k pass assertions=5, RUNNER_DONE 22 equals the 22 ids of tests/conformance/REQUIRED; a scratch toy that classifies any length up to a size as that size turned both rows guest_failure (CONTROL RED_OK for both ids), then was deleted"
        status: pass
    human_judgment: false
  - id: D4
    description: "LED-LOAD-0004 is an open entry for the 512K, 768K and 192K disagreements, pinned to ref_size_512k, listed in ledger/REQUIRED, with all eleven schema keys"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage ledger-entries -> CHECK ledger-entries PASS; grep -c 'status = \"open\"' prints 1; grep -cxF 'LED-LOAD-0004 02-CONTEXT D-44' ledger/REQUIRED prints 1; each of the eleven keys matches one line"
        status: pass
    human_judgment: false

duration: 9 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 09: Toy-control mechanism, the wrong-mirror toy and the size refusal rows Summary

**A wrong 384K mirror is now built at control time from the live `src/core/mapper.c` by a one-line rule (`scripts/control-mutate`), compiled as its own target `ds_toy_wrong-mirror`, and seen as `guest_failure` by the same `accept_384k_banksig` case that the real core passes; the 8192 and 512 KiB requests are refused `size_unsupported`, and LED-LOAD-0004 holds the open size disagreements.**

## Performance

- **Duration:** about 9 min (2026-11-10T05:19Z to 05:28Z)
- **Tasks:** 2 of 2 (Task 1 is the tracer, Task 2 is TDD)
- **Files:** 11 changed outside .planning (402 added lines, 1 removed)

## Accomplishments

- **`scripts/control-mutate RULE OUT [--root DIR]`** (sh and awk). RULE is `key=value` lines after `#` comments with exactly the keys `tag`, `file`, `anchor`, `replace` (a repeated, unknown or missing key is a harness error; `file` must be repository-relative with no `..`). The anchor is counted with awk `index()` as a fixed string (the text goes in through the environment, so no backslash processing and no regex); exactly one occurrence is replaced and the whole file goes to OUT. Zero or several occurrences print `CONTROL_ANCHOR_COUNT TAG n=N` and exit 2.
- **CMake.** `file(GLOB ... CONFIGURE_DEPENDS tests/controls/*/RULE)`; for each rule one `ds_toy_NAME` executable (`EXCLUDE_FROM_ALL`) built from the runner sources, the text sources and the ds_core sources with the rule's `file=` source replaced by the GENERATED `${CMAKE_BINARY_DIR}/controls/NAME/<basename>`. `ds_core` and `ds_runner` are untouched; `grep` over the generated `build.ninja` finds `tests/controls` only in the configure-dependency lists (2 hits), in no ds_core object (0 hits).
- **conformance-run controls mode** (after the matrix and the hash sweep, before `STATUS conformance ok`): validates every control first, then per control runs control-mutate on the live file, builds the toy, runs the toy runner under `supervise` on the same cases, oracle and profile inputs, and requires `RESULT ID guest_failure` (awk field match on fields 2 and 3) for every EXPECT id and a `RUNNER_DONE` line. Prints `CONTROL NAME RED_OK` or `CONTROL NAME NOT_RED` (status 1 after all controls ran).
- **wrong-mirror toy.** `tests/controls/wrong-mirror/` holds only RULE and EXPECT. The anchor is the literal `return b < 0x40 ? (b & 0x1F) : (0x20 + (b & 0x0F));` (the line plan 08 recorded, unique in the file); the replacement is `return b & 0x3F;`.
- **Selftest `mutation-control-anchor`**: per rule, a scratch root; the untouched copy must succeed, a copy with the anchor's first character changed must give exit 2 with `n=0`, a copy with the anchor line duplicated must give exit 2 with `n=2`.
- **Size refusal rows** `ref_size_8192` and `ref_size_512k` (kind refuse, request `len=N`), both in REQUIRED; **LED-LOAD-0004** (open, 11 schema keys, pinned to `TEST:ref_size_512k`, `evidence = []`, withheld any claim about a 512K, 768K or 192K card, states that a 2 MiB image is refused by the cap as `too_large`). `ledger/REQUIRED` carries exactly `LED-LOAD-0004 02-CONTEXT D-44`.

## First content byte (plan request)

`src/core/boot.c` line 286, `__builtin_memcpy(inst->rom, request->content, request->len);`, in `ds_boot` after the `boot_order` loop (all checks) returned `DS_OK`. The only earlier mention of the content pointer is line 189 (`check_buffer_null`), which compares the pointer with NULL and reads no byte. The length, cap, short, bank-multiple and supported-size decisions use `req->len` alone. The content hash (line 288) reads the copy, after the checks too.

## Notes for later plans

- A new toy needs only `tests/controls/NAME/RULE` and `EXPECT` (plus `file=` naming a ds_core source). `ds_toy_NAME` appears in the build after the next configure (glob with CONFIGURE_DEPENDS), and the controls mode runs it. Names are limited to A-Z a-z 0-9 _ -; EXPECT ids to A-Z a-z 0-9 _ and must exist in the case inventory.
- A mutated file that is included by a non-core file or is a non-core source is refused at configure (`file=` must be a ds_core source). If a later toy has to mutate `src/text` or `tests/`, the CMake block needs a second list to search.
- `scripts/check` prints details only for a failing stage, so `CONTROL ... RED_OK` appears in `sh scripts/conformance-run run` output and in `build/PRESET/controls.txt`, not in the `CHECK conformance PASS` line.
- The selftest relies on the anchor occurring once in the live file; a later edit of `mapper.c` that duplicates or rewrites the 384K line turns the rule into a harness error (exit 2, `CONTROL_ANCHOR_COUNT`), by design.
- `runner.c` was not touched (61016 bytes, limit 65536). The refusal matrix (plan 11) adds only case files.

## Task Commits

1. **Task 1 (tracer): toy control mechanism and the wrong-mirror toy shown red by accept_384k_banksig** - `634e2d2` (feat)
2. **Task 2: size refusal rows ref_size_8192 and ref_size_512k and the open entry LED-LOAD-0004** - `29dcb2d` (feat)

## Verification Results

Tracer gate (auto mode: verify re-run end to end after `634e2d2`): `sh scripts/check --stage conformance` exit 0 `CHECK conformance PASS` and `sh scripts/check --selftest mutation-control-anchor` exit 0 with `SELFTEST mutation-control-anchor wrong-mirror PASS`. Log line: `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance && sh scripts/check --selftest mutation-control-anchor` (Task 1) | exit 0; `CHECK conformance PASS`; `SELFTEST mutation-control-anchor wrong-mirror PASS`, `SELFTEST RESULT PASS` |
| `sh scripts/conformance-run run` (shows the controls line) | `RUNNER_DONE 20`, `STATUS hash-sweep ok lengths=21`, `CONTROL wrong-mirror RED_OK`, `STATUS conformance ok n=20`, exit 0 |
| `sh scripts/check --stage conformance && sh scripts/check --stage ledger-entries` (Task 2) | both PASS; `RESULT ref_size_8192 pass assertions=5`, `RESULT ref_size_512k pass assertions=5`, `RUNNER_DONE 22`, `CONTROL wrong-mirror RED_OK`, `STATUS conformance ok n=22` |

Acceptance criteria, all run:

- Task 1: `CONTROL wrong-mirror RED_OK` present (PASS). `ls tests/controls/wrong-mirror` lists exactly `EXPECT` and `RULE` (PASS). Replace set equal to the anchor: `CONTROL wrong-mirror NOT_RED` with `expected guest_failure; got: accept_384k_banksig:pass` and `sh scripts/check --stage conformance` exit 1 `CHECK conformance FAIL`, then RULE restored from a backup (observed once). Anchor edited to match nothing (`return q` for `return b`): exit 2 and `CONTROL_ANCHOR_COUNT rule-wrong-mirror-384k n=0` (observed once, restored). EXPECT emptied: exit 2 `HARNESS_EMPTY_EXPECT wrong-mirror`; EXPECT with comments only: the same; EXPECT removed: exit 2 `HARNESS_NO_EXPECT wrong-mirror` (restored). The generated `build.ninja` ds_core objects list no `tests/controls` path (count 0). `head -5` of `scripts/control-mutate` carries `SPDX-License-Identifier: MIT OR Apache-2.0` and of RULE `0BSD`; `sh scripts/check --stage licences` PASS.
- Task 2: both RESULT lines `pass`, RUNNER_DONE 22 equals the 22 ids of `tests/conformance/REQUIRED` (PASS). `grep -c 'status = "open"'` prints 1, `grep -cxF 'LED-LOAD-0004 02-CONTEXT D-44' ledger/REQUIRED` prints 1, each of the eleven keys matches one `^KEY = ` line (PASS). The two case files and the ledger entry start with the 0BSD header (PASS).
- Liveness of the two size rows (extra, not in the plan): a scratch control (not committed, deleted after the run) replaced `len == (size_t)size_row[i].bytes` by `len <= ...` in the generated mapper; the controls mode reported `CONTROL tmp-size-lenient RED_OK` with both `ref_size_8192` and `ref_size_512k` as guest_failure, so the rows are not vacuous. This also ran the mechanism with a second rule.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, ends `CHECK RESULT PASS` (conformance PASS) |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | `SELFTEST RESULT PASS` (includes `SELFTEST mutation-control-anchor wrong-mirror PASS`) |
| `sh scripts/conformance-run run --preset gate-gcc` and `--preset gate-clang` | `RUNNER_DONE 22`, `CONTROL wrong-mirror RED_OK`, `STATUS conformance ok n=22` for both |
| `sh scripts/audit-history --range 90f8f8d..HEAD` | `audit-history: clean` (2 commits, 11 blobs) |

## Files Created/Modified

See `key-files`. `scripts/control-mutate` carries the MIT OR Apache-2.0 header; RULE, EXPECT, the two case files and LED-LOAD-0004 carry 0BSD.

## Decisions Made

See `key-decisions`. The plan fixed the RULE keys, the exit codes, the line formats and the case ids; the choices left open were the output channel of `CONTROL_ANCHOR_COUNT` (stdout), the up-front validation order, the treatment of a toy that does not compile, and the CMake refusal of a non-core `file=`.

## Deviations from Plan

None - plan executed exactly as written, with the interpretation choices below.

### Interpretation choices (inside the plan's intent)

1. **No RED commit for Task 2.** The plan marks Task 2 `tdd="true"` but states that no core defect is expected, because plan 08 made `supported_size` the exact enum rule. The two rows passed on first run, so there is no failing state to commit. The rows were shown non-vacuous instead, with the scratch lenient-size toy above. `boot.c` and `runner.c`, listed in the plan's files, needed no change and were not edited.
2. **The REQUIRED line is inserted between `LED-LOAD-0003` and `LED-LOAD-0005`,** keeping `ledger/REQUIRED` sorted as plan 08 did, instead of at the end; content and equality with the entry's `origin` are as specified.
3. **The CONTROL lines are not shown by `sh scripts/check --stage conformance` on a pass** (the check script prints stage detail only on failure). The acceptance line was observed with `sh scripts/conformance-run run`, which is also what the stage runs; the lines are kept in `build/PRESET/controls.txt`.
4. **`controls_mode` is called from `run_mode`, not exposed as a separate mode,** because it needs the cases TSV, oracle and profile inputs that the run mode writes.
5. **The `ledger/entries` sources line was shortened to stay under the 400-character line limit of the TOML subset parser** (first draft gave `LINE_TOO_LONG` and `MISSING_FIELD`, fixed before the commit).

6. **CONT-01 was not checked off in REQUIREMENTS.md.** `requirements mark-complete CONT-01` was run and its edit then reverted: the requirement also covers refusing special mappers and unknown layouts with a typed reason, which the refusal matrix of plan 11 has not delivered yet, and this plan adds size refusal only. The frontmatter `requirements-completed` field names it because the plan lists it; the orchestrator or plan 11 decides when the box is ticked.

**Total deviations:** 0 auto-fixed, 6 interpretation choices. **Impact:** none on later plans' contracts.

## Known Stubs

None. No placeholder, hardcoded empty value or TODO was added.

## Threat Flags

None. No new network endpoint or file access beyond build-directory writes. T-02-49 is mitigated as planned: toys are separate EXCLUDE_FROM_ALL targets built only by the controls mode; NOT_RED fails the stage (observed); an empty EXPECT is exit 2 (observed); the generated build graph lists no `tests/controls` path under ds_core (checked). T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist: `scripts/control-mutate`, `tests/controls/wrong-mirror/RULE`, `tests/controls/wrong-mirror/EXPECT`, `tests/conformance/cases/ref_size_8192.toml`, `tests/conformance/cases/ref_size_512k.toml`, `ledger/entries/LED-LOAD-0004.toml` (checked with `[ -f ]`).
- Commits `634e2d2` and `29dcb2d` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 2.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
