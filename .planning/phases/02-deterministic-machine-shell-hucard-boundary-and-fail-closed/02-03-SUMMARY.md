---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 03
subsystem: testing
tags: [sh, gate, conformance, cancelled-outcome, preflight, timing, outcome-sync, negative-controls, selftest-mutation]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: run_controls helper and --stage/--selftest NAME in scripts/check (02-01); outcomes.def and the RESULT-file judge conformance-ingest (02-02)
provides:
  - scripts/conformance-run, supervise mode only: wall watchdog, INT/TERM trap, cancelled RESULT lines with a named signal source (D-77)
  - scripts/preflight, a fail-closed tool presence check (missing tool exits 2)
  - scripts/outcome-sync, outcomes.def versus the seven required names, with --root for mini-roots
  - scripts/check --timing and build/gate/results.tsv written on every run
  - stages preflight (first), cancel-controls, outcome-controls; selftests mutation-cancel-trap and mutation-outcome-sync
affects: [02-04, 02-05, 02-06 (registers the real-repository outcome-sync stage after build), 02-18 (repeats the cancel control against the live runner)]

actuals:
  tokens: 7500     # chars/4 over the realized diff (git diff f2aa870..b615316, 29914 chars, 19 files, 523 added lines)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: f2aa870674dd5d2629e632bcade3cffd028153d2
plan_head_after: b61531613578c8062bb164143403fd04bfae0986
commits: 3

tech-stack:
  added: []
  patterns:
    - "a rule whose removal must flip a control carries a single-line trailing marker token (rule-cancel-trap on the two trap lines), and the selftest assembles the token at run time so scripts/check never contains it"
    - "a control that spawns a process records the child's pid in a file and reaps it by checking the command line first, so a mutated or failing run leaves nothing spinning"
    - "the control sequence is a function that takes the script under test (cancel_sequence RUN WORKDIR), so the stage and the mutation selftest run the same assertions against the real and the mutated copy"
    - "a checker whose only failure mode is a harness error (preflight) has no exit 1: absence of a tool is not a rule violation"

key-files:
  created:
    - scripts/conformance-run
    - scripts/preflight
    - scripts/outcome-sync
    - tests/gate/cancel/ (spin.sh, ids.txt, tests/conformance/REQUIRED, cases a b c)
    - tests/gate/outcome-sync/ (extra-row, missing-row, renamed-upper-case, reordered, ok-seven)
  modified:
    - scripts/check

key-decisions:
  - "conformance-run maps a CMD exit status above 2 to HARNESS_CMD_STATUS and exit 2, keeping the 0/1/2 contract; statuses 0, 1, 2 pass through unchanged"
  - "a wall expiry is detected as status 142 (SIGALRM); a CMD that itself dies of SIGALRM is indistinguishable and is treated as expiry, which is the safe direction (harness error, never green, never cancelled)"
  - "outcome-sync codes are kept minimal per fault: ROWS when the row count is not seven, NAME when the set of names differs, ORDER only when both are clear; so an extra duplicate row is ROWS alone and a missing row is ROWS plus NAME"
  - "--timing is stripped from the argument list before the old parsing, and is a usage error with --selftest, so it is never silently ignored"

patterns-established:
  - "cancel controls drive a real SIGTERM at a real supervisor and a spinning stub (exec sleep) under $tmp; the judge is then run on the produced file both with --expect OUTCOME_CANCELLED and bare (must exit 1)"
  - "results.tsv is truncated and written on every non-selftest run, one stage, verdict, seconds line per stage that ran"

requirements-completed: [RUN-02, TOOL-02]

coverage:
  - id: D1
    description: "The cancelled outcome has a producer: SIGTERM to a supervisor of a spinning stub yields cancelled RESULT lines naming the signal, RUNNER_DONE and exit 1, and the judge calls the file non-green"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage cancel-controls -> CHECK cancel-controls PASS (results file is exactly a pass, b and c cancelled, RUNNER_DONE 3; ingest --expect OUTCOME_CANCELLED gives EXPECT_OK; bare ingest exits 1)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The trap, not something else, produces the cancelled lines"
    requirement: RUN-02
    verification:
      - kind: unit
        ref: "sh scripts/check --selftest mutation-cancel-trap -> PASS (mutated supervisor dies with 143 and writes only the a line)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The wall watchdog is a different outcome from cancelled: exit 2, HARNESS_WALL on stderr, no cancelled line"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "cancel-controls stage, control wall-not-cancelled (--wall 1 against the stub)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A missing build tool is an error, never a skip"
    requirement: TOOL-02
    verification:
      - kind: unit
        ref: "sh scripts/preflight --tools no-such-tool-xyz exits 2 and prints PREFLIGHT_MISSING no-such-tool-xyz; sh scripts/check --stage preflight PASS (controls for the impossible name and for an a|b alternative)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Every gate run leaves a timing record; --timing prints TIMING lines"
    requirement: TOOL-02
    verification:
      - kind: unit
        ref: "sh scripts/check --timing --stage preflight prints TIMING preflight N; build/gate/results.tsv has 23 lines after a full run"
        status: pass
    human_judgment: false
  - id: D6
    description: "outcomes.def holds exactly the seven required names in order, and each rule has a red control"
    requirement: RUN-02
    verification:
      - kind: unit
        ref: "sh scripts/check --stage outcome-controls PASS (extra-row, missing-row, renamed-upper-case, reordered red with exact code sets; ok-seven green); sh scripts/check --selftest mutation-outcome-sync PASS"
        status: pass
    human_judgment: false

duration: 9 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 03: Cancelled producer, preflight, timing record and outcome-sync Summary

**`scripts/conformance-run supervise` now turns a SIGTERM into cancelled RESULT lines that name the signal (judged non-green by the ingest, mutation-proven), the wall watchdog is a separate exit-2 path, `scripts/preflight` fails closed on a missing tool, `scripts/check --timing` and `build/gate/results.tsv` record stage times, and `scripts/outcome-sync` proves outcomes.def is exactly the seven required names, each with controls.**

## Performance

- **Duration:** 9 min (about 2026-11-10T03:45Z to 03:55Z)
- **Tasks:** 3 of 3 (Task 1 is the tracer)
- **Files:** 19 changed (3 scripts created, `scripts/check` modified, 9 + 7 control files under tests/gate)
- **Gate cost (warm, one machine, noisy):** `sh scripts/check --timing` stage seconds sum to 18 s, whole run about 17 s wall; no stage exceeded 20 s, so no `TIMING_WARN`. The slowest is protocol-negatives at 5-6 s; the three stages added here cost cancel-controls 1 s, outcome-controls 1 s, preflight under 1 s (whole-second resolution). The total is under D-60's 60 s warm proposal. `dash scripts/check` took 27 s in one run (machine load); time never fails the gate.

## Accomplishments

- **Cancelled has a producer (D-77, T-02-47).** `conformance-run supervise --ids FILE --out FILE [--wall N] -- CMD...` starts CMD in the background under `perl -e 'alarm shift; exec @ARGV'` with stdout to OUT. The two trap lines (token `rule-cancel-trap`) call a handler that kills CMD, closes a partial last line, appends `RESULT ID cancelled assertions=0 evidence=signal:NAME` for every id in the ids file without a RESULT line, appends `RUNNER_DONE n`, prints `CANCELLED signal:NAME n=COUNT` and exits 1. Unknown modes exit 2 with the usage line.
- **The watchdog is not cancelled.** Alarm expiry is status 142; the supervisor prints `HARNESS_WALL seconds=N` on stderr, exits 2 and appends nothing to OUT. The control asserts exit 2, the HARNESS_WALL line and no `cancelled` text anywhere in OUT or stdout.
- **cancel-controls.** The stub `tests/gate/cancel/spin.sh` records its pid, prints `RESULT a pass assertions=1 evidence=evidence/a.txt` and `exec sleep 600`. The stage SIGTERMs the supervisor once the a line appears (polling up to 5 s) and requires: supervisor exit 1; `CANCELLED signal:TERM n=3`; the file is exactly the a line, `RESULT b cancelled assertions=0 evidence=signal:TERM`, the same for c, `RUNNER_DONE 3`; `conformance-ingest --expect OUTCOME_CANCELLED` prints EXPECT_OK; bare ingest exits 1; then the wall control.
- **mutation-cancel-trap.** The same `cancel_sequence` function runs against a scratch copy (must pass), then against a copy with the two marked lines deleted (marker count 2 before, 0 after): SIGTERM now kills the supervisor with 143 before it writes anything and the sequence is broken (exit 1). No sleep process survives either run.
- **preflight.** `scripts/preflight [--tools LIST]` prints `PREFLIGHT_OK tool` per entry (the first found alternative for `a|b`), `PREFLIGHT_MISSING ENTRY` and exit 2 on absence, `STATUS preflight ok` otherwise; default list `cmake ninja cc nm perl iconv awk git sha256sum|shasum`. It is the first stage; the stage also runs an impossible-name control (exit 2 plus the named line) and an alternative control (`no-such-tool-xyz|sh` is satisfied by `sh`).
- **--timing and results.tsv.** `run_stage` times each stage with `date +%s`, appends `stage<TAB>verdict<TAB>seconds` to `build/gate/results.tsv` (truncated at the start of every non-selftest run, gitignored under `build/`), prints `TIMING stage secs` after the CHECK line with `--timing`, and `TIMING_WARN stage` above 20 s. Usage line updated.
- **outcome-sync.** `scripts/outcome-sync [--expect CODES] [--root DIR]` keeps the literal seven names (the statement of ROADMAP criterion 4) and reports OUTSYNC_ROWS, OUTSYNC_NAME, OUTSYNC_ORDER. Five mini-root controls under `tests/gate/outcome-sync/` (extra-row -> ROWS; missing-row -> ROWS,NAME; renamed-upper-case -> NAME; reordered -> ORDER; ok-seven green) run through `run_controls` in stage `outcome-controls`. `mutation-outcome-sync` deletes every line naming OUTSYNC_ROWS and requires the extra-row expectation to fail with `got '<none>'`; separately checked that the real stage reports `CONTROL BROKEN extra-row` and `CONTROL BROKEN missing-row` under that mutation.

## Task Commits

1. **Task 1 (tracer): supervise mode, cancel controls and mutation-cancel-trap** - `aea108d` (feat)
2. **Task 2: preflight, --timing and results.tsv** - `67db33a` (feat)
3. **Task 3: outcome-sync, outcome-controls and mutation-outcome-sync** - `b615316` (feat)

## Verification Results

Tracer gate (auto mode): after the Task 1 commit the tracer verify was re-run (`sh scripts/check --stage cancel-controls && sh scripts/check --selftest mutation-cancel-trap`, both pass, `pgrep -f 'sleep 600'` empty); log line `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage cancel-controls && sh scripts/check --selftest mutation-cancel-trap` (Task 1; also under dash) | exit 0, `CHECK cancel-controls PASS`, `SELFTEST mutation-cancel-trap PASS` |
| `sh scripts/check --stage preflight && { sh scripts/preflight --tools no-such-tool-xyz >/dev/null 2>&1; test $? -eq 2; } && sh scripts/check --timing --stage preflight \| grep -q '^TIMING preflight ' && test -s build/gate/results.tsv` (Task 2) | exit 0 (printed `verify-ok` and `exit2ok` in the separate runs) |
| `sh scripts/check --stage outcome-controls && sh scripts/check --selftest mutation-outcome-sync` (Task 3; stage also under dash) | exit 0, `CHECK outcome-controls PASS`, `SELFTEST mutation-outcome-sync PASS` |

Acceptance criteria, all run: raw cancel results file is exactly `RESULT a pass ...`, the two cancelled lines, `RUNNER_DONE 3`; `--wall 1` run exits 2 with `HARNESS_WALL seconds=1`; `grep -c rule-cancel-trap scripts/conformance-run` = 2; SPDX `MIT OR Apache-2.0` in the first five lines of conformance-run, preflight, outcome-sync and tests/gate/cancel/spin.sh; `sh scripts/preflight --tools no-such-tool-xyz` exit 2 with `PREFLIGHT_MISSING no-such-tool-xyz`; `CHECK preflight PASS` is the first stage line; `sh scripts/check --timing` prints one TIMING line per stage and `results.tsv` has 23 lines; `sh scripts/outcome-sync` prints `STATUS outcome-sync ok`, exit 0; `ls tests/gate/outcome-sync | wc -l` = 5; `sh scripts/check --stage outcome-sync` prints the usage error, exit 2.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 23 `CHECK ... PASS` stage lines (18 baseline, plus ingest-controls, flatten-controls, preflight, cancel-controls, outcome-controls), `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | ten `SELFTEST ... PASS` lines (the eight before plus mutation-cancel-trap and mutation-outcome-sync), `SELFTEST RESULT PASS`, exit 0 |
| `dash scripts/check` | `CHECK RESULT PASS` |

No leftover `sleep 600` process after any run.

## Files Created/Modified

- `scripts/conformance-run` - supervise mode.
- `scripts/preflight` - tool check.
- `scripts/outcome-sync` - outcome-name check.
- `scripts/check` - stages preflight, cancel-controls, outcome-controls, helpers `cancel_reap` and `cancel_sequence`, `--timing`, results.tsv, selftests (i) and (j).
- `tests/gate/cancel/` - spin.sh (MIT OR Apache-2.0 header), ids.txt, REQUIRED and cases a, b, c (0BSD headers).
- `tests/gate/outcome-sync/` - five mini-roots (0BSD headers on the def files).

## Decisions Made

See `key-decisions`. The plan fixed the CLI, codes and markers; the choices left open were the exit mapping for a CMD status above 2, how wall expiry is recognised, the exact ROWS/NAME/ORDER partition, and where `--timing` is parsed.

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1-3 fix was needed.

### Additions and interpretation choices (inside the plan's intent)

1. **CMD status above 2 maps to harness error.** The artifacts section says normal exit returns CMD's status; the plan's own prohibition keeps exit codes to 0/1/2. `conformance-run` passes 0, 1, 2 through and turns anything else (a killed or missing CMD) into `HARNESS_CMD_STATUS n` and exit 2.
2. **Alarm arming.** `perl -e 'alarm shift; exec @ARGV' N CMD...` instead of interpolating N into the program text: same behaviour, no code built from a variable.
3. **Extra controls beyond the plan:** the `a|b` alternative control in the preflight stage, the bare-ingest-exits-1 check in cancel-controls, and a partial-last-line guard in the cancel handler (a CMD killed mid-line cannot glue a cancelled line onto a fragment).
4. **`--timing` with `--selftest` is a usage error** rather than a silent no-op.

**Total deviations:** 0 auto-fixed, 4 interpretation choices. **Impact:** none on scope or on later plans' contracts.

## Known Stubs

None. `conformance-run` has only supervise mode by design (later plans add modes; an unknown mode exits 2).

## Notes for later plans

- SIGINT cannot be driven by a control here: a background non-interactive shell ignores SIGINT on entry, so the control uses SIGTERM while one handler serves both signals (Ctrl-C in a foreground run reaches the same code). Plan 18 repeats the cancel control against the live runner.
- The supervisor kills only its direct child. A runner that forks workers would need a process-group kill; the planned runner is one process (D-60).
- `--timing` and `results.tsv` have whole-second resolution; stages under a second record 0.
- The `outcome-sync` stage on the real repository (reading build/gate/ds_runner) is plan 06's to register after build; `scripts/outcome-sync` already runs on the real repository by direct call.
- `scripts/check` writes `build/gate/results.tsv` into the directory plan 06 uses as the CMake binary dir for the gate preset; harmless, but do not `rm -rf build/gate` expecting to keep it.
- Only macOS BWK awk, dash and `sh` (bash 3.2 mode) were exercised; a Linux run (gawk/mawk, procps `ps -o command=`) is for CI.

## Threat Flags

None. The new surface is local process control inside the gate; no network, no content parsing beyond the RESULT grammar already covered.

## Self-Check: PASSED

- Files exist: scripts/conformance-run, scripts/preflight, scripts/outcome-sync, tests/gate/cancel/spin.sh, tests/gate/outcome-sync/ok-seven/tests/conformance/outcomes.def (checked with `[ -f ]`).
- Commits `aea108d`, `67db33a`, `b615316` are ancestors of HEAD.
- All task acceptance criteria and plan-end commands re-run and passing, as tabulated above.
