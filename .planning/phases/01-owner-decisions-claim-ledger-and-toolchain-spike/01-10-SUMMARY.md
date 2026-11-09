---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 10
subsystem: testing
tags: [tool-01, spike, hyperfine, measurements, results-table, check-results, negative-controls]
status: complete

requires:
  - phase: 01-05
    provides: "pre-registered protocol with the metric register markers, and the scorer"
  - phase: 01-07
    provides: "C option hooks (opt_clean/opt_edit/opt_restore/opt_test), LINES.map, saved gates-c-full.txt"
  - phase: 01-09
    provides: "Rust option hooks (incl. opt_wasm), LINES.map, saved gates-rust-full.txt"
provides:
  - "spikes/tool-01/bench/{clean,build,edit,restore,test}.sh: wrappers that forward one argument to the option hook, with BENCH_DRYRUN proof and exit 2 on a missing or invalid argument"
  - "spikes/tool-01/bench/run-all.sh [--dry-run]: runs every register row for both options with the protocol's exact hyperfine commands, append-only results.tsv, SKIP-PRESENT resume"
  - "spikes/tool-01/results/: 24 raw hyperfine JSON files, results.tsv (26 rows per option, no not-demonstrated rows), gates-c.txt and gates-rust.txt (verbatim)"
  - "scripts/check-results with five rule codes, tests/results/ (5 must-fail controls plus ok-complete), and the scripts/check stage results-negatives"
affects: [01-11, 01-12]

tech-stack:
  added: [hyperfine 2.0.0 (measurement only, Apache-2.0 OR MIT, Homebrew)]
  patterns:
    - "Append-only results file with a per-row skip-present resume rule instead of a rewrite"
    - "Checker contract reused: FILE TAB CODE TAB LINE TAB DETAIL, EXPECT_OK marker, a must-fail control per code, NO_CONTROLS when none exist"

key-files:
  created:
    - spikes/tool-01/bench/clean.sh
    - spikes/tool-01/bench/build.sh
    - spikes/tool-01/bench/edit.sh
    - spikes/tool-01/bench/restore.sh
    - spikes/tool-01/bench/test.sh
    - spikes/tool-01/bench/run-all.sh
    - scripts/check-results
    - tests/results/ok-complete/
    - tests/results/row-deleted/
    - tests/results/empty-value/
    - tests/results/unknown-row/
    - tests/results/no-register/
    - tests/results/no-scores/
    - spikes/tool-01/results/ (24 M0N-<opt>-<variant>.json, results.tsv, gates-c.txt, gates-rust.txt)
  modified:
    - scripts/check
    - spikes/tool-01/LOG.md

key-decisions:
  - "run-all.sh classifies a register row as a timing row by its command starting with hyperfine; every other row is copied from LOG.md, counted from LINES.map, or read from the verbatim gate output"
  - "M09 comment rule: .c/.h/.rs files drop // and /* */ comments; every other file drops lines whose first non-blank character is #. Every file in LINES.map counts, including the Rust capi Miri test module"

patterns-established:
  - "Dry run writes only to the gitignored out/ and never into results/; the real run refuses to start on a dirty skeleton and restores the two edited sources on exit"

requirements-completed: []  # TOOL-01 is advanced here (measurements complete); it completes with the language decision record in 01-11/01-12

duration: ~13 min
completed: 2026-11-09

actuals:
  tokens: 8937    # chars/4 over the realized code diff (scripts, bench wrappers, tests: 35749 chars); the 204k chars of raw JSON/TSV measurement data are not counted as code
  tasks: 3
  commits: 4
plan_head_before: 6fb46b97038a19e5785e55eb709d8975b54e76ed
plan_head_after: 9a5b0a2f2aadba4842885f324ff3507664ef54bd

coverage:
  - id: D1
    description: "results.tsv has one row per register (metric, variant) for each of c and rust and check-results accepts it; every value is measured or copied, none is not-demonstrated"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/check-results docs/decisions/0001-language-build.protocol.md --tsv spikes/tool-01/results/results.tsv exits 0, RECORDS 52; 26 rows for c and 26 for rust; grep -c 'not demonstrated' prints 0; grep -c dry-run prints 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every timing row is the protocol's exact hyperfine command with raw JSON kept under results/, all Rust timings on the pinned stable 1.99.1 (no nightly anywhere in run-all.sh)"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "24 M0N-<opt>-<variant>.json files; grep -c nightly spikes/tool-01/bench/run-all.sh prints 0; python cross-check of all 24 JSON files against results.tsv wall-clock summary: 0 mismatches"
        status: pass
    human_judgment: false
  - id: D3
    description: "M01 cold-first-build values copied from the LOG.md lines of plans 01-06 and 01-08, never re-measured; M02 is the warm clean build under hyperfine"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "results.tsv M01 cold-first-build c seconds=0.768 and rust seconds=0.686, equal to the LOG.md lines; two M01 cold rows (one per option)"
        status: pass
    human_judgment: false
  - id: D4
    description: "check-results rejects RESULT_MISSING, RESULT_EMPTY, RESULT_UNKNOWN_ROW, REGISTER_EMPTY and SCORES_MISSING, each with a must-fail control, and accepts ok-complete; the gate reports NO_CONTROLS when no control exists"
    requirement: TOOL-01
    verification:
      - kind: unit
        ref: "sh scripts/check ends CHECK RESULT PASS with CHECK results-negatives PASS; RED run showed three controls reporting nothing before the rules existed; a copy of scripts/ with only ok-complete gives NO_CONTROLS"
        status: pass
    human_judgment: false
  - id: D5
    description: "No row was re-run, dropped or cherry-picked, and the skeleton sources are clean after the run"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "run-all.log has 0 SKIP-PRESENT lines and exit=0 (one uninterrupted run); git status --porcelain -- spikes/tool-01/c spikes/tool-01/rust is empty"
        status: pass
    human_judgment: false

self_check: PASSED
---

# Phase 1 Plan 10: Pre-registered measurements and results completeness check Summary

**Both skeletons were measured once with the protocol's exact hyperfine commands (24 raw JSON files, 26 rows per option, nothing not-demonstrated) and scripts/check-results now proves a results table complete against the register, with a must-fail control per rule.**

## Performance

- **Duration:** about 13 minutes (the measurement run itself took 6.5 minutes: 2026-11-09T22:53:59Z to 23:00:29Z)
- **Tasks:** 3 (1 tracer, 1 TDD auto, 1 SLOW auto); 4 commits
- **Files:** wrappers, run-all.sh, check-results, 6 control directories, 24 JSON files, results.tsv, 2 gate outputs

## Accomplishments

- **Tracer.** The protocol register flows through `run-all.sh --dry-run` into `out/results.dry.tsv` (52 rows, 26 per option), and `check-results` accepts it; the gate gained the `results-negatives` stage. The dry run never wrote into `results/`.
- **Bench wrappers.** `clean.sh`, `build.sh`, `edit.sh`, `test.sh` take `<opt>` first and forward the second argument unchanged to `opt_clean`, `opt_build`, `opt_edit`, `opt_test`; `restore.sh` takes only `<opt>`. A missing or invalid argument exits 2, and `BENCH_DRYRUN=1` prints exactly one `HOOK <opt> <hook> <arg>` line. Every acceptance line matched.
- **Task 2 (TDD).** Five defect cases (row-deleted, empty-value, unknown-row, no-register, no-scores) each differ from `ok-complete` by one defect. The RED run showed empty-value, unknown-row and no-scores reporting no violation at all (wrong reason: rule missing), then GREEN added RESULT_EMPTY, RESULT_UNKNOWN_ROW and SCORES_MISSING and made zero negative cases a NO_CONTROLS failure.
- **Task 3 (SLOW, once).** `hyperfine 2.0.0` was installed (Homebrew, Apache-2.0 OR MIT, logged in LOG.md); one uninterrupted `run-all.sh` produced every row. `check-results` on results.tsv exits 0 (RECORDS 52). The skeleton sources are clean (`git status --porcelain -- spikes/tool-01/c spikes/tool-01/rust` empty).

### Measured values (for plan 01-11; scoring is not done here)

Median wall seconds from hyperfine (`--warmup 1 --runs 10`):

| Row | C | Rust |
|-----|---|------|
| M02 clean build, debug / release | 0.766 / 0.703 | 0.722 / 0.889 |
| M03 comment edit, debug / release | 0.224 / 0.217 | 0.453 / 0.863 |
| M04 real edit, debug / release | 0.194 / 0.199 | 0.802 / 0.779 |
| M05 test, debug / release | 0.035 / 0.036 | 0.118 / 0.102 |
| M06 warm gate, debug / release | 1.791 / 1.525 | 3.380 / 3.176 |
| M07 gate clean to green, debug / release | 2.464 / 2.468 | 3.947 / 4.255 |
| M01 cold first build (copied from LOG.md) | 0.768 | 0.686 |
| M01 toolchain install | preinstalled | 36.6 s |
| M09 core / ffi-glue / build / scripts / runner (lines) | 109 / 38 / 15 / 145 / 311 | 148 / 231 / 69 / 176 / 181 |
| M08, M11, M12, M13 (s), M15 | PASS, PASS, PASS (61.1 s, 39,814 exec/s), 0.4, leak 0 (seeded 10) | PASS, PASS, PASS (61.2 s, 46,895 exec/s), 0.4, leak 0 (seeded 8) |
| M14 wasm | not attempted, install cost 180,322,002-byte wasi-sdk 34.0 download | transcript-identical=yes |

Observations for the scorer, not decisions: the M09 Rust ffi-glue count (231) includes the capi `#[cfg(test)]` module written for Miri, because the protocol counts every file in LINES.map; the C4 inputs (ffi-glue plus build plus scripts) are 198 for C and 476 for Rust. The M13 time-to-find is run-to-run variable (the C LOG.md has an earlier sample of 1.2 s); results.tsv carries the end-of-plan gate value.

## Task Commits

1. **Task 1 (tracer): register to run-all.sh to results TSV to check-results, wired into the gate** - `b899246` (feat)
2. **Task 2 RED: one must-fail control per check-results rule** - `c1e0d7c` (test)
3. **Task 2 GREEN: RESULT_EMPTY, RESULT_UNKNOWN_ROW, SCORES_MISSING, NO_CONTROLS** - `b9aff14` (feat)
4. **Task 3: pre-registered measurements, raw JSON, gate outputs, LOG.md** - `9a5b0a2` (feat)

## Deviations from Plan

### Plan details not followed literally

- **Install time of hyperfine not recorded.** The plan's log form is `install hyperfine <version> <s> ...`. I did not time the `brew install`, so the LOG.md line reads `not-timed` in the seconds field. hyperfine is excluded from C3's tool count, so no score uses the time.
- **Task 2 split into a RED and a GREEN commit** (a `test` commit then a `feat` commit) to follow the TDD gate; the plan lists one commit per task.
- **Task 1 check-results is a subset.** As the plan says, the Task 1 version implemented only the parse, REGISTER_EMPTY and RESULT_MISSING; Task 2 added the rest.

### Auto-fixed Issues

None. No bug fix was needed in the hooks or the protocol.

### Known limitation (recorded, not changed)

- **run-all.sh's JSON timing parser depends on key order.** hyperfine 2.0.0 writes JSON with `schema_version: 2`, where per-metric summaries (memory_peak_resident, time_cpu, time_system, time_user, time_wall_clock) each carry mean, stddev, median, min and max; the parser keeps the last occurrence of each key, which is the `time_wall_clock` block in 2.0.0's output. I found this after the run, cross-checked all 24 raw JSON files against the `time_wall_clock` summary with an independent script (0 mismatches, `count` 10 each), and left the committed script as it ran, so the script that produced the results is the one preserved. A later hyperfine version with a different key order could misparse, so any re-use should pin the metric explicitly. Not an amendment of the protocol.

## Issues Encountered

- **Machine not idle at the start.** The load average was 22.8 on the shared host (from my preceding `scripts/check`), so the first timing rows ran under some contention. No row was repeated to change a value; this is noted in LOG.md. The tie-band and truncation rules in the protocol are unchanged.
- **Pitfall: sandbox test of run-all.sh.** To exercise the non-dry-run parsing, resume and restore paths without touching results/, I ran run-all.sh in a scratch copy with a stub hyperfine; its output never entered the repository.
- Known Stubs: none. No new security-relevant surface beyond the plan's threat model (T-01-22 mitigated by exact register commands, raw JSON, one run, dry-run isolation and five must-fail controls; T-01-34 mitigated: `grep -c nightly` on run-all.sh is 0 and no timing used `+nightly`; T-01-SC mitigated: hyperfine installed from a Homebrew formula, version and licence logged).

## Next Phase Readiness

- 01-11 can read `spikes/tool-01/results/results.tsv` (five tab-separated columns), `gates-c.txt` and `gates-rust.txt`, and run `scripts/check-results ... --record` on record 0001's results table once it is filled; the record-mode and `SCORES_MISSING` rule are already live and controlled.
- The timebox end is 2026-11-10T05:53:09Z; this plan finished at 2026-11-09T23:02Z.

## Self-Check: PASSED

- Created files exist: scripts/check-results, spikes/tool-01/bench/{clean,build,edit,restore,test,run-all}.sh, tests/results/{ok-complete,row-deleted,empty-value,unknown-row,no-register,no-scores}/, spikes/tool-01/results/{results.tsv,gates-c.txt,gates-rust.txt} and 24 JSON files.
- Commits `b899246`, `c1e0d7c`, `b9aff14`, `9a5b0a2` are ancestors of HEAD; `git rev-list --count 6fb46b9..HEAD` is 4 before this SUMMARY commit.
- Plan acceptance and verify commands were re-run after the measurement run: check-results on results.tsv exit 0; `sh scripts/check` ends `CHECK RESULT PASS` with `CHECK results-negatives PASS`; equal row counts (26 and 26); M01 cold rows 2; `grep -c nightly` 0; `grep -n '^install hyperfine'` one line; skeleton sources clean.
