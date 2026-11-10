---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 18
subsystem: gate
tags: [required-test-inventory, inventory-check, ledger-pins, named-controls, live-runner-controls, timeout, cancelled, fail-closed, negative-controls]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: two-stage TOML checker pattern and run_controls (02-01 to 02-04), the supervise mode and the cancelled producer (02-03), the rights-check and its planted-pce control (02-04), ds_run and the placeholder stepping (02-12), the case inventory and REQUIRED (02-05 to 02-16), the fixture manifest and its controls (02-17)
provides:
  - scripts/inventory-check with scripts/lib/inventory-rules.awk, eight INV_ codes, both directions of the ledger TEST pin check (ledger-check stays structure-only, D-58)
  - tests/gate/inventory (8 mini-roots) and the stages inventory (before build) and inventory-negatives (with two run-time probes)
  - runner kinds budget_spin and spin_forever (tests/conformance/spinkinds.c) and a timeout outcome in the runner (the first live producer of timeout)
  - conformance-run modes control and control-signal with field-wise ID OUTCOME matching; the mini-roots wrong-literal, timeout-budget, cancel-spin
  - stage named-controls: seven NAMED_CONTROL lines (the five D-58 controls plus timeout-budget and cancelled-spin)
  - selftest mutation-inventory-required-missing and mutation-wrong-literal-compare
affects: [02-19 and later (the gate has 42 stages; runner.c is at 65534 of 65536 bytes, 2 bytes of headroom), any later plan that adds a case with pins (the ledger entry must name TEST:that-case and the case must name the entry), any later plan that adds a required case (REQUIRED and the case file must agree)]

actuals:
  tokens: 17762    # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 71049 chars, 53 files, 978 added and 7 removed lines)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: e40992c2a9a57b600399fb1bf761f6ae6c6bdf65
plan_head_after: 273edaca321c1e5dc7962d89b72730b8998fad8a
commits: 3

tech-stack:
  added: []
  patterns:
    - "a cross-file inventory check reads every file through the shared two-stage reader (toml-subset.awk then a stage-2 awk that prints one record per file), then compares the records in one awk; a rule that must be mutation-proven is one self-contained line carrying a rule-NAME tag"
    - "a mode that verifies a live runner passes the id and outcome through ENVIRON and compares RESULT fields one by one; the line that counts a mismatch is the tagged line, so deleting it makes the control unable to fail (the mutation proves it)"
    - "a control that needs a process to be signalled starts it in the background, waits for its first RESULT line, signals, waits for the supervisor, and then reaps by the control's own cases path so no spinning process survives"

key-files:
  created:
    - scripts/inventory-check
    - scripts/lib/inventory-rules.awk
    - tests/conformance/spinkinds.c
    - tests/gate/inventory/ (8 mini-roots: ok-consistent, removed-required, unlisted-case, pin-unresolved, pin-not-required, pin-backlink, case-missing-field, required-empty)
    - tests/gate/wrong-literal/ (1 case)
    - tests/gate/timeout-budget/ (1 case)
    - tests/gate/cancel-spin/ (2 cases)
  modified:
    - scripts/check
    - scripts/conformance-run
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - CMakeLists.txt

key-decisions:
  - "the verdict of a live-runner control is made on the id and outcome fields of RESULT lines (awk, values through ENVIRON), never on a whole line, because the line also carries the assertion count and the evidence path (D-80)"
  - "the timeout outcome is a ctx flag set by the kind (a machine-time budget consumed without a stop); a failed assertion wins over it, so a timeout never hides a wrong value"
  - "an INV_ code is one rule per code, so each can be shown red by one mini-root; the second direction of the pin check and a REQUIRED line that is not an id are shown red by run-time probes made from ok-consistent, which keeps the eight-directory layout the plan fixes"
  - "inventory-check treats a REQUIRED id whose case says required = false as INV_CASE_FIELD, so REQUIRED and the case files must agree in both directions"

patterns-established:
  - "runner.c is at 65534 of 65536 bytes: any further runner feature goes in its own file with a table row, and a comment is trimmed in runner.c only if the row cannot be avoided"
  - "CONFORMANCE_RUN_ROOT points a mutated copy of conformance-run at the real tree, as RIGHTS_CHECK does for synthetic.sh"

requirements-completed: [FIX-04, RUN-02, FIX-05]

coverage:
  - id: D1
    description: "The required-test inventory fails closed: a REQUIRED id with no case file, a required case not listed, an unresolved ledger TEST pin, a pin at a non-required case, a missing pin back-link (both directions), a case missing any of the ten keys and an emptied REQUIRED each give a distinct code"
    requirement: FIX-04
    verification:
      - kind: integration
        ref: "sh scripts/check --stage inventory PASS (INVENTORY required=100 cases=100 pins=6, STATUS inventory ok) and sh scripts/check --stage inventory-negatives PASS (8 mini-roots, each with exactly its one code, plus the reverse-direction and INV_BAD_LINE probes)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The removed-required rule is mutation-proven"
    requirement: FIX-04
    verification:
      - kind: integration
        ref: "SELFTEST mutation-inventory-required-missing PASS: the removed-required control is met on the copy (EXPECT_OK), and with the line tagged rule-required-missing deleted the same run exits 1"
        status: pass
    human_judgment: false
  - id: D3
    description: "The five named controls and the timeout and cancelled producers each go red through the real checker or the live runner"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage named-controls PASS: build/gate/named-controls.txt holds seven NAMED_CONTROL lines, all RED_OK (removed-required, missing-field, unlisted-fixture, wrong-literal, romlike-binary, timeout-budget, cancelled-spin); editing the wrong-literal EXPECT pair to wrong_literal pass made that line NOT_RED and the stage FAIL (observed once, reverted)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The field-wise comparison that lets the wrong-literal control fail is mutation-proven"
    requirement: FIX-05
    verification:
      - kind: integration
        ref: "SELFTEST mutation-wrong-literal-compare PASS: unmutated conformance-run control exits 1 on the pair wrong_literal pass (CONTROL_PAIR_MISSING wrong_literal pass got:guest_failure), and with the line tagged rule-field-match deleted it exits 0; grep -c rule-field-match scripts/conformance-run prints 1 (baseline 0)"
        status: pass
    human_judgment: false

duration: about 15 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 18: Fail-closed required-test inventory and the named negative controls Summary

**`inventory-check` fails closed on a removed, unlisted or unpinned required test (eight INV_ codes, ledger TEST pins resolved in both directions), and the new `named-controls` stage shows the five D-58 controls plus the timeout and cancelled producers red through the real checkers and the live `ds_runner`, with field-wise `ID OUTCOME` matching and two mutations.**

This is a change detector and a calibration of the gate, not an oracle: the inventory proves that the set of required tests is the set that exists and that ledger pins are mutual, and the controls prove that the gate can turn red for each named failure mode. Evidence tier E0 (repeatability and controls that must fail); nothing here says anything about hardware.

## Performance

- **Duration:** about 15 min (2026-11-10)
- **Tasks:** 3 of 3 (Task 1 tracer, Tasks 2 and 3 auto)
- **Files:** 53 changed outside .planning (978 added lines, 7 removed)
- **Gate cost (this machine, warm):** `sh scripts/check --timing` 46 s wall over 42 stages (dash: 44 s), all PASS, no `TIMING_WARN`, so under D-60's 60 s proposal. Per stage seconds: conformance 6, sanitizers 5, protocol-negatives 5, named-controls 4, rights-negatives 3, audit-synthetic 3, inventory 2, determinism 2, every other stage 0 or 1. The new stages: `inventory` 2 s (one awk stage pair per case file), `inventory-negatives` under 1 s, `named-controls` 4 s (it runs three live runners and the rights script).

## Accomplishments

- **Tracer (Task 1).** `scripts/inventory-check` reads `tests/conformance/REQUIRED`, every case file and every ledger entry through `toml-subset.awk` and `inventory-rules.awk` (stage 2 prints one C or L record per file, never nothing), then one awk compares them. The first two rules: `INV_REQUIRED_MISSING` (tagged `rule-required-missing`) and `INV_UNLISTED_CASE`. Mini-roots `ok-consistent` and `removed-required`; stages `inventory` (before `build`, per D-60) and `inventory-negatives`. Removing `field_flip` from a scratch copy of REQUIRED gave `INV_UNLISTED_CASE field_flip` and exit 1.
- **Pin rules and schema (Task 2).** `INV_PIN_UNRESOLVED`, `INV_NOT_REQUIRED`, `INV_PIN_BACKLINK` (forward: a ledger `TEST:ID` entry the case does not name in `pins`; reverse: a case names a ledger id whose `pinning_test` is not `TEST:that-case`, or that does not exist), `INV_CASE_FIELD` (all ten keys including `request`, types, unknown keys, id equals the file stem), `INV_REQUIRED_EMPTY`, `INV_BAD_LINE`. Six more mini-roots (eight in total). No case file needed a fix: the real repository was already consistent (100 ids in REQUIRED, 100 case files with `required = true`, no pin fault). The six pinned entries resolve as follows.

  | Ledger entry | Case (required = true) | pins array |
  |--------------|------------------------|------------|
  | LED-LOAD-0001 | ref_order_tg16 | ["LED-LOAD-0001"] |
  | LED-LOAD-0002 | ref_header_copier512 | ["LED-LOAD-0002"] |
  | LED-LOAD-0003 | accept_384k_banksig | ["LED-LOAD-0003"] |
  | LED-LOAD-0004 | ref_size_512k | ["LED-LOAD-0004"] |
  | LED-LOAD-0005 | accept_unmapped_banks | ["LED-LOAD-0005"] |
  | LED-PROF-0002 | poweron_draw_order | ["LED-PROF-0002"] |

- **Live-runner controls (Task 3).** `spinkinds.c` gives the runner two kinds: `budget_spin` (ds_run with a stop function that never fires; budget consumed gives the outcome `timeout`, deterministic because the budget is machine time) and `spin_forever` (steps `ds_advance` without end, used only by `tests/gate/cancel-spin`). `conformance-run control DIR` flattens the mini-root, builds the gate preset incrementally, runs the live runner under the supervise mode, prints the RESULT lines and compares each EXPECT pair on fields 1 to 3 with awk; `control-signal DIR SIG` starts the supervised runner in the background, waits for its first RESULT line, sends the signal and reaps any survivor. The stage `named-controls` prints seven `NAMED_CONTROL NAME RED_OK` lines (also written to `build/gate/named-controls.txt`) and checks afterwards that no control runner is left.

## Verification Results

Per-task `<automated>` commands (exit codes and key lines):

| Command | Result |
|---------|--------|
| Task 1: `sh scripts/check --stage inventory && sh scripts/check --stage inventory-negatives` | exit 0: `CHECK inventory PASS`, `CHECK inventory-negatives PASS`. The tracer gate (auto mode) re-ran it after the commit: same result, so expansion went ahead (`Tracer verified end-to-end - expanding`) |
| Task 2: `sh scripts/check --stage inventory && sh scripts/check --stage inventory-negatives && sh scripts/check --selftest mutation-inventory-required-missing` | exit 0: two `CHECK ... PASS` lines and `SELFTEST mutation-inventory-required-missing PASS` |
| Task 3: `sh scripts/check --stage build && sh scripts/check --stage conformance && sh scripts/check --stage named-controls && sh scripts/check --selftest mutation-wrong-literal-compare` | exit 0: three `CHECK ... PASS` lines, `SELFTEST mutation-wrong-literal-compare PASS`; `pgrep -f ds_runner` printed nothing afterwards |

Acceptance criteria, all run:

- Task 1: `sh scripts/inventory-check` printed `STATUS inventory ok` on the real repository (also under dash); `removed-required` with `--expect INV_REQUIRED_MISSING` printed `EXPECT_OK` and without `--expect` exited 1 (PASS). Scratch REQUIRED without `field_flip`: exit 1 with `INV_UNLISTED_CASE` (PASS, observed once). `head -5` of `scripts/inventory-check` holds `SPDX-License-Identifier: MIT OR Apache-2.0` (PASS).
- Task 2: `INVENTORY required=100 cases=100 pins=6`; REQUIRED holds 100 ids and 100 case files say `required = true` (both counts recorded, equal). `ls tests/gate/inventory | wc -l` is 8. The six pinned entries resolve (table above). `head -5` of `scripts/lib/inventory-rules.awk` holds the MIT OR Apache-2.0 id (PASS).
- Task 3: seven `NAMED_CONTROL NAME RED_OK` lines (removed-required, missing-field, unlisted-fixture, wrong-literal, romlike-binary, timeout-budget, cancelled-spin). The wrong-literal EXPECT edited to `wrong_literal pass` gave `NAMED_CONTROL wrong-literal NOT_RED` and `CHECK named-controls FAIL`, then reverted and PASS again. `SELFTEST mutation-wrong-literal-compare PASS`, `grep -c rule-field-match scripts/conformance-run` is 1. `grep -n` shows `run_stage build` at line 1648, `run_stage conformance` at 1650 and `run_stage named-controls` at 1661 of `scripts/check`. `--stage licences` PASS; each control case file starts with the 0BSD header comment; the new scripts and `spinkinds.c` carry the MIT OR Apache-2.0 header.
- The live runner outputs (from the stage run): `RESULT wrong_literal guest_failure assertions=5 ...`, `RESULT budget_spin timeout assertions=2 ...`, `RESULT first_ok pass assertions=5 ...` then `RESULT spin cancelled assertions=0 evidence=signal:TERM` then `RUNNER_DONE 2`.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check --timing` | exit 0, 42 `CHECK <stage> PASS` lines (baseline 18 at plan 02, 39 before this plan) and `CHECK RESULT PASS`, 46 s; `build/gate/results.tsv` has 42 lines, all PASS |
| `dash scripts/check` | exit 0, ends `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | exit 0, 27 `SELFTEST ... PASS` lines (25 before) including `mutation-inventory-required-missing` and `mutation-wrong-literal-compare`, then `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range e40992c..HEAD` | `audit-history: clean` (3 commits, 44 blobs) |

The sanitizers stage (ASan and UBSan over the whole runner, which now compiles `spinkinds.c`) is among the 42 PASS lines.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Files beyond the plan's `files_modified` list, and room made in runner.c**
- **Found during:** Task 3
- **Issue:** `runner.c` was at 65503 of 65536 bytes (the plan 07 limit of 65536 bytes per file under tests), so the two kinds cannot live there, and the runner needs a place to report `timeout`.
- **Fix:** the kinds are in the new `tests/conformance/spinkinds.c` (one function, `kind_spin`, registered by two table rows), declared in `tests/conformance/runner.h`, which also gains the `timeout` field of `struct ctx`; `CMakeLists.txt` lists the source. `runner.c` got the two table rows and three lines in `outcome_of` (a flagged timeout is the outcome `timeout` unless an assertion failed), and a four-line doc comment of `--exclude-ids` was shortened to two lines to pay for them: `runner.c` ends at 65534 of 65536 bytes.
- **Files modified:** `tests/conformance/spinkinds.c`, `tests/conformance/runner.h`, `tests/conformance/runner.c`, `CMakeLists.txt`
- **Verification:** build, conformance and sanitizers stages PASS; `find src tests -size +65536c` prints nothing
- **Commit:** `273edac`

**2. [Rule 1 - Bug in my own first draft] The required-missing rule line was not self-contained**
- **Found during:** Task 2, `SELFTEST mutation-inventory-required-missing FAIL` on the first run
- **Issue:** the tagged line was followed by an `else if`, so deleting it left a dangling `else` and an awk syntax error (exit 2) instead of a flipped control (exit 1).
- **Fix:** the next rule became its own `if`, so every tagged line is a complete statement.
- **Files modified:** `scripts/inventory-check`
- **Verification:** the selftest prints PASS; the same shape is used for the tagged line of `conformance-run`
- **Commit:** `07e6f19`

### Interpretation choices (inside the plan's intent)

1. **`INV_CASE_FIELD` also covers** an unknown key, a wrong type, a table, a syntax error in a case file, a file name that is not an id, and a REQUIRED id whose case says `required = false` (so REQUIRED and the case files must agree in both directions). A ledger file with a syntax error is ignored here because `ledger-check` owns the ledger's format (TEST pin resolution lives only in `inventory-check`, D-58).
2. **Two probes in `inventory-negatives`.** The plan fixes eight mini-roots with one code each, so the reverse direction of the pin check and `INV_BAD_LINE` are shown red by two roots made at run time from `ok-consistent` (a sed edit of one ledger entry and one appended REQUIRED line); each must give exactly its own code.
3. **`CONFORMANCE_RUN_ROOT`.** `control_mode` normally works in the repository root of the script; the environment variable points a mutated copy of the script (which lives in a temporary `scripts/` directory) at the real tree, as `RIGHTS_CHECK` does for `synthetic.sh`. It is documented in the header of the script.
4. **`control` builds the gate preset itself** (an incremental build, a no-op right after the build stage), so a stale `build/gate/ds_runner` is never judged; `flatten_cases` gained an optional source directory for this.
5. **The romlike-binary control has two parts:** `tests/rights/synthetic.sh` must print `RIGHTSTEST planted-pce PASS` (its script prints only PASS or FAIL lines), and the same fault is then made in a temporary repository (a text file named `unlisted.pce`, created at run time, `git init` with the global git configuration disabled) where `rights-check` must give exactly `ROMLIKE_BINARY` and exit 1 without `--expect`, so the code itself is observed. No ROM-like byte is committed.
6. **`control-signal` waits for the first RESULT line** of the runner (at most 10 s) as its "has started" test; the cancel-spin mini-root therefore has one finishing case (`first_ok`) before the spinning one. Only TERM is used (INT is accepted by the mode but not used).
7. **`named-controls` is registered last**, after `fixture-negatives`, which is after both `build` and `conformance` as D-85 requires; its lines also go to `build/gate/named-controls.txt` because a passing stage prints only its CHECK line.
8. **Stage count.** 39 stages before this plan plus `inventory`, `inventory-negatives` and `named-controls` gives 42, as the plan states.

**Total deviations:** 2 auto-fixed (one Rule 3, one Rule 1 in my own draft), 8 interpretation choices. **Impact:** none on the plan's intent.

## Notes for later plans

- **A new required case needs both sides.** Add its id to `tests/conformance/REQUIRED` and a case file with `required = true`; `sh scripts/inventory-check` names the missing half. A case that pins a ledger entry needs the entry's `pinning_test = "TEST:that-case"` and the entry's id in the case's `pins`; a ledger entry that moves from `PENDING:N` to `TEST:ID` needs the case to name it.
- **`runner.c` is at 65534 of 65536 bytes.** Anything new goes in its own file with a table row; there is no more comment text worth trimming.
- **A new live-runner control** is a mini-root under `tests/gate/NAME` (`tests/conformance/REQUIRED`, `tests/conformance/cases`, an EXPECT of `ID OUTCOME` pairs); `sh scripts/conformance-run control tests/gate/NAME` runs it and leaves `build/gate/control/NAME/results.txt` for the ingest.
- **`skip` is still never produced** by any producer and appears only in the ingest controls, where it is red; `harness_failure` and `missing_artifact` come from the judge and the supervisor watchdog.
- **Reaper.** `control_mode` kills a runner whose command line holds `ds_runner --cases build/gate/control/NAME/cases.tsv` after a signalled run (the supervisor has normally killed it already); the stage then checks `pgrep -f 'ds_runner --cases build/gate/control'`. Both are skipped, not failed, where `pgrep` does not exist.

## Task Commits

1. **Task 1 (tracer): inventory-check flags a removed required test through the real checker** - `6481a7c` (feat)
2. **Task 2: both-direction ledger pin rules, case schema rule, six more controls and the required-missing mutation** - `07e6f19` (feat)
3. **Task 3: live-runner controls (wrong literal, budget timeout, cancelled spin) and the named-controls stage** - `273edac` (feat)

## Known Stubs

None. No placeholder text, empty data flow or unwired component was added. The `budget_spin` and `spin_forever` kinds exercise the plan 12 placeholder stepping, which is that plan's documented placeholder and not a stub of this plan.

## Threat Flags

None. No network endpoint, auth path or file-access pattern at a trust boundary was added: the inventory checker reads repository files, the live controls run the repository's own runner on repository mini-roots under `build/gate/control`, and the one process the stage signals is a child it started. T-02-35 is mitigated by INV_REQUIRED_MISSING, INV_UNLISTED_CASE, INV_REQUIRED_EMPTY, the `removed-required` named control and `mutation-inventory-required-missing`. T-02-36 is mitigated by the wrong-literal control through the live runner (id and outcome fields `wrong_literal guest_failure`, judged red by the ingest), the field-wise awk match with a NOT_RED probe (observed once) and `mutation-wrong-literal-compare`. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `scripts/inventory-check`, `scripts/lib/inventory-rules.awk`, `tests/conformance/spinkinds.c`, `tests/gate/inventory/removed-required/EXPECT`, `tests/gate/wrong-literal/EXPECT`, `tests/gate/timeout-budget/EXPECT`, `tests/gate/cancel-spin/EXPECT`.
- Commits `6481a7c`, `07e6f19` and `273edac` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 3.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
