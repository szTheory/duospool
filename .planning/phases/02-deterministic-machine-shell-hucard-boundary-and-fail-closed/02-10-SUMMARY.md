---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 10
subsystem: gate
tags: [sanitizers, asan, ubsan, canary, alloc-shim, excluded-in-mode, conformance-gate, cmake-presets]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: the runner, the refuse kind with the ptr=dummy and len=size_max request keys (02-06, 02-07); the plain mapper and the size rows (02-08, 02-09); conformance-run run mode, the supervise watchdog and the ingest judge (02-05)
provides:
  - preset sanitize (ASan and UBSan, no recover, no -fwrapv, D-83) and conformance-run sanitize, with /usr/bin/clang on Darwin; stage sanitizers inside the fast gate
  - ds_canary heap, overflow and shift, each required to be reported by the sanitizer (non-zero exit and its own message) or SANITIZER_DEAD NAME and status 1
  - case ref_too_large_size_max (ptr=dummy, len=size_max, expect load.too_large), passing under the gate and the sanitize preset
  - allocation-counting shim (tests/conformance/allocshim.c) and the alloc_zero kind (tests/conformance/allockinds.c) with cases alloc_shim_selfcheck and alloc_zero_matrix
  - tests/conformance/EXCLUDED_IN_MODE, runner --exclude-ids, conformance-ingest --mode --excluded --gate-results with EXCL_BAD_LINE, EXCL_NOT_REQUIRED, EXCL_NOT_PASS_IN_GATE, EXCL_RAN_IN_MODE
  - five controls under tests/gate/excluded, stage excluded-mode-controls, selftest mutation-excluded-gate-pass (31 stages in all)
affects: [02-11 (the full refusal matrix runs under the sanitize preset and under the allocation matrix automatically; runner.c has about 1.5 KB left, split a kind file out first), 02-12 to 02-16 (their verify lines run the sanitizers stage), 02-15 (nm check is the primary no-allocator proof; the shim is the second witness)]

actuals:
  tokens: 16833    # chars/4 over the realized diff (git diff plan_head_before..HEAD excluding .planning, 67333 chars, 50 files, 780 added and 18 removed lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: f5289607f06399f4a342108f16417703e77d5d74
plan_head_after: d07f8c0094de38569a70ab0bd123a417f99eb653
commits: 4

tech-stack:
  added: []
  patterns:
    - "a sanitizer stage is only believed after planted defects were reported by it: three canaries, each with a non-zero exit and the runtime's own message, else SANITIZER_DEAD"
    - "a mode-scoped exclusion has one source: the checked file EXCLUDED_IN_MODE gives the ids that the runner leaves out, that the supervisor stops expecting and that the ingest forgives, and the ingest asserts the file both ways"
    - "a kind that needs a build-time witness fails closed when the witness is absent (counter that never moves), never skips"

key-files:
  created:
    - tests/conformance/canary.c
    - tests/conformance/allocshim.c
    - tests/conformance/allockinds.c
    - tests/conformance/EXCLUDED_IN_MODE
    - tests/conformance/cases/ref_too_large_size_max.toml
    - tests/conformance/cases/alloc_shim_selfcheck.toml
    - tests/conformance/cases/alloc_zero_matrix.toml
    - tests/gate/excluded/ok-excl
    - tests/gate/excluded/excl-unlisted
    - tests/gate/excluded/excl-not-pass-in-gate
    - tests/gate/excluded/excl-not-required
    - tests/gate/excluded/excl-ran-in-mode
  modified:
    - CMakePresets.json
    - CMakeLists.txt
    - scripts/conformance-run
    - scripts/conformance-ingest
    - scripts/check
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED

key-decisions:
  - "sanitize compiler is chosen by conformance-run (/usr/bin/clang on Darwin, cc elsewhere) and exported as CC; a build/sanitize configured with another compiler is removed first, because cmake keeps the compiler of an existing cache"
  - "the sanitize link line gets only the -fsanitize flags (add_link_options), so the warning set never reaches the linker"
  - "the sanitize run keeps the hash sweep (SHA-256 shifts under UBSan) and drops the toy controls, which the gate preset already judged"
  - "the exclusion set has a single source: tests/conformance/EXCLUDED_IN_MODE rows for the mode go to build/sanitize/exclude-ids.txt, which feeds the runner (--exclude-ids), the supervisor ids file and the printed EXCLUDED_IN_MODE lines"
  - "a runner --exclude-ids id that names no case is a harness fault (exit 2), so a stale exclusion cannot linger"
  - "the alloc_zero kind lives in allockinds.c, with ds_shim_calls() falling back to a counter that never moves when DS_ALLOC_SHIM is 0, so both cases fail closed there"
  - "the matrix needs a live shim before it reads any row; the liveness probe is not an assertion, so assertions equal the number of refuse rows"
  - "the exclusion file is validated on every gate run too (conformance-run run passes --mode gate --excluded), so a bad row is caught without the sanitize stage"

patterns-established:
  - "a new refuse row is covered by the allocation matrix and the sanitize run with no further change: both read the case inventory"
  - "a new mode-scoped exclusion is one row in EXCLUDED_IN_MODE plus its reason in the file comment; the ingest rejects it unless the id is REQUIRED and passes in the gate run"

requirements-completed: [CONT-03]

coverage:
  - id: D1
    description: "The sanitize preset (ASan and UBSan, no recover, no -fwrapv) is alive: a heap overflow, a signed overflow and a signed left shift are each reported with a non-zero exit, and a canary that is not reported is SANITIZER_DEAD"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "sh scripts/conformance-run sanitize -> CANARY heap reported exit=134, CANARY overflow reported exit=134 (tests/conformance/canary.c:41:15: runtime error: signed integer overflow), CANARY shift reported exit=134 (canary.c:51:15: runtime error: left shift of 1 by 31 places); with -fwrapv added to the preset: SANITIZER_DEAD overflow and SANITIZER_DEAD shift; with -fno-sanitize-recover=all removed: SANITIZER_DEAD overflow and shift (UBSan printed and exited 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The hardest length row, SIZE_MAX over a dummy pointer that is never dereferenced, is refused too_large in the gate and the sanitize preset"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "RESULT ref_too_large_size_max pass assertions=5 in build/gate/results.txt and build/sanitize/results.txt; sh scripts/check --stage conformance and --stage sanitizers PASS"
        status: pass
    human_judgment: false
  - id: D3
    description: "Zero allocations on every refusal row are witnessed by a counting shim that proves itself alive"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "RESULT alloc_shim_selfcheck pass assertions=1 and RESULT alloc_zero_matrix pass assertions=4 (4 refuse rows in build/gate/cases.tsv); a scratch build with DS_ALLOC_SHIM=OFF gave guest_failure for both; a scratch ds_boot that called malloc once made alloc_zero_matrix guest_failure with a line per row (made 1 allocator calls), then was reverted"
        status: pass
    human_judgment: false
  - id: D4
    description: "The one sanitize-mode exclusion is an enumerated, checked file: an unlisted exclusion, a listed id that does not pass in the gate run, a listed id that is not REQUIRED and a listed id that ran in its mode each fail by an exact code set, and the exclusion is logged and never a skip"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "sh scripts/check --stage excluded-mode-controls PASS (ok-excl passes; excl-unlisted OUTCOME_MISSING_ARTIFACT, excl-not-pass-in-gate EXCL_NOT_PASS_IN_GATE, excl-not-required EXCL_NOT_REQUIRED, excl-ran-in-mode EXCL_RAN_IN_MODE); sh scripts/check --selftest mutation-excluded-gate-pass PASS; the sanitize run prints EXCLUDED_IN_MODE sanitize alloc_shim_selfcheck and EXCLUDED_IN_MODE sanitize alloc_zero_matrix, RUNNER_DONE 23, and grep -c '^RESULT [a-z0-9_]* skip ' over build/sanitize/results.txt prints 0"
        status: pass
    human_judgment: false

duration: 13 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 10: Sanitizer stage, allocation shim and the checked mode exclusion Summary

**The conformance matrix now also runs under ASan and UBSan (no recover, no -fwrapv) after three planted canaries prove the sanitizers report; a counting allocation shim with a self-check shows zero allocations on every refusal row in the gate preset; and the only rows the sanitize mode leaves out are an enumerated file that the ingest asserts in both directions.**

## Performance

- **Duration:** about 13 min (2026-11-10T05:32Z to 05:45Z)
- **Tasks:** 3 of 3 (Task 1 is the tracer, Task 2 is TDD with a red commit, Task 3 is auto)
- **Files:** 50 changed outside .planning (780 added lines, 18 removed), 12 of them new control mini-roots

## Accomplishments

- **Sanitize preset.** `sanitize` inherits the hidden base (warning flags) and adds `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`, `DS_SANITIZE=ON`, `DS_ALLOC_SHIM=OFF`, binary directory `build/sanitize`. `-fwrapv` lives only in `DS_EXTRA_FLAGS` of the three gate presets in the tree as found, so no relocation was needed. `CMakeLists.txt` repeats only the `-fsanitize` flags on the link line.
- **Canaries.** `ds_canary heap|overflow|shift` (built only with `DS_SANITIZE`). `conformance-run sanitize` requires a non-zero exit and `AddressSanitizer` (heap) or `runtime error` (overflow, shift), prints `CANARY NAME reported exit=N` and the first UBSan line cut to a repository-relative path, else `SANITIZER_DEAD NAME` and status 1 (all three are run before the status is decided).
- **Sanitize mode.** Same steps as the run mode on `build/sanitize`: flatten, oracle, build, canaries, runner under the supervisor (wall 900 s, 3 times the default), ingest, hash sweep (kept: SHA-256 shifts run under UBSan), final line `STATUS sanitize ok n=23`. The toy controls are not rerun there. `ASAN_OPTIONS=detect_leaks=1` is exported on Linux only. The existing core and runner produced no ASan or UBSan finding, so no core fix was needed.
- **Hardest length row.** `ref_too_large_size_max` (`ptr=dummy;len=size_max`, expect `load.too_large`), `pass assertions=5` in both presets.
- **Allocation shim.** `allocshim.c` defines `malloc`, `calloc`, `realloc` and `free` in the runner executable (macOS default zone, tested; glibc `__libc_*`, untested here and stated in the header; any other platform defines nothing, so the self-check fails closed). `alloc_shim_selfcheck`: one allocator call from the test must move the counter by exactly 1. `alloc_zero_matrix`: the shim must be live first, then every refuse-kind row is booted again with fresh test-allocated memory and the counter read around `ds_boot` must be 0; assertions equal the rows run.
- **Exclusion.** `tests/conformance/EXCLUDED_IN_MODE` (`sanitize alloc_shim_selfcheck`, `sanitize alloc_zero_matrix`). The runner takes `--exclude-ids FILE`. The ingest takes `--mode NAME --excluded FILE [--gate-results FILE]` and reports `EXCL_BAD_LINE`, `EXCL_NOT_REQUIRED`, `EXCL_NOT_PASS_IN_GATE`, `EXCL_RAN_IN_MODE`; an unlisted REQUIRED id without a RESULT line stays `OUTCOME_MISSING_ARTIFACT`. The gate run (every preset) validates the file too (`--mode gate`).
- **Controls.** Five mini-roots under `tests/gate/excluded`, stage `excluded-mode-controls`, selftest `mutation-excluded-gate-pass` (deletes the single line tagged `rule-excl-gate-pass` from a scratch copy of the ingest; the control then flips).

## Recorded counts and facts

- Compiler of the sanitizer run: `SANITIZER_COMPILER /usr/bin/clang` (Apple clang 21.0.0), recorded in `build/sanitize/CMakeCache.txt` as `CMAKE_C_COMPILER:FILEPATH=/usr/bin/clang`.
- D-83 flag discrimination, `grep -c -e '-fwrapv' build/NAME/CMakeCache.txt` (gate-gcc and gate-clang configured first with `cmake --preset`): `sanitize` 0, `gate` 1, `gate-gcc` 1, `gate-clang` 1. Baseline: `build/sanitize` did not exist before the task.
- Refuse-kind rows in `build/gate/cases.tsv`: 4 (`ref_profile_unknown`, `ref_size_8192`, `ref_size_512k`, `ref_too_large_size_max`); `RESULT alloc_zero_matrix pass assertions=4`. Later plans add rows and the count rises by itself.
- `RUNNER_DONE 25` in the gate runs (required cases: 25), `RUNNER_DONE 23` in the sanitize run.
- `sh scripts/check` prints 31 `CHECK` stage lines.
- `tests/conformance/runner.c` is 64027 bytes (limit 65536 of the rights-tree acceptance criterion). The alloc kind went to `allockinds.c` for that reason. Plan 11 has about 1.5 KB of headroom in `runner.c` and should move a kind file out before adding a deferred kind.
- Sanitizers stage time: about 6 s (`TIMING sanitizers 6`).

## Notes for later plans

- Any plan that adds a refuse row is automatically witnessed for zero allocation (gate presets) and run under ASan and UBSan (sanitize). A new row that needs a content buffer above 4 MiB must use `ptr=dummy` or `ptr=null` (runner limit), as `ref_too_large_size_max` does.
- A new UBSan finding in core code is a real defect: fix it with unsigned arithmetic or an explicit cast (widen before shifting). Never add `-fwrapv` to the sanitize preset.
- `sh scripts/check --stage sanitizers` alone needs `build/gate/results.txt` (exit 2, `HARNESS_NO_GATE_RESULTS`, otherwise): run `--stage conformance` first, as every plan's verify line already does.
- The toys (`ds_toy_NAME`) link the shim when `DS_ALLOC_SHIM` is ON, so their runners agree with the real one on the alloc cases.
- `scripts/conformance-run` sanitize prints UBSan paths cut to `tests/...`; do not paste raw sanitizer output with absolute paths into committed files.

## Task Commits

1. **Task 1 (tracer): sanitize preset with planted canaries and the SIZE_MAX refusal row** - `bbd041a` (feat)
2. **Task 2 (red): alloc_zero kind and the two alloc cases, red because no shim is linked** - `41ef4d4` (test)
3. **Task 2 (green): allocation shim, alloc kind moved to allockinds.c** - `a796a7d` (feat)
4. **Task 3: mode-scoped exclusion file, ingest assertions, controls and selftest** - `d07f8c0` (feat)

## Verification Results

Tracer gate (auto mode: verify re-run end to end after `bbd041a`): `sh scripts/check --stage conformance` and `sh scripts/check --stage sanitizers` both PASS. Log line: `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance && sh scripts/check --stage sanitizers` (Task 1) | exit 0; `CHECK conformance PASS`, `CHECK sanitizers PASS` |
| `sh scripts/check --stage conformance` (Task 2, after green) | exit 0; `RESULT alloc_shim_selfcheck pass assertions=1`, `RESULT alloc_zero_matrix pass assertions=4`, `RUNNER_DONE 25` |
| `sh scripts/check --stage conformance && sh scripts/check --stage excluded-mode-controls && sh scripts/check --stage sanitizers && sh scripts/check --selftest mutation-excluded-gate-pass` (Task 3) | exit 0; three `CHECK ... PASS`, `SELFTEST mutation-excluded-gate-pass PASS`, `SELFTEST RESULT PASS` |

Acceptance criteria, all run:

- Task 1: `CHECK sanitizers PASS`; the raw sanitize output lists the three canaries as reported, the overflow and shift ones with their runtime error line. Flag counts as recorded above. `build/sanitize` exists, compiler `/usr/bin/clang`. `RESULT ref_too_large_size_max pass assertions=5` in both runs. Observed once each and reverted: `-fwrapv` added to the sanitize preset gave `SANITIZER_DEAD overflow` (and `shift`); `-fno-sanitize-recover=all` removed gave `SANITIZER_DEAD overflow` and `shift` (UBSan reports and the process exits 0). `head -5` of `canary.c` carries `SPDX-License-Identifier: MIT OR Apache-2.0`; `sh scripts/check --stage licences` PASS.
- Task 2: `build/gate/results.txt` shows both cases `pass`, matrix assertions 4 equal to the 4 refuse rows. Red state observed before the shim was linked (`guest_failure` for both, one assertion each). A scratch build with `DS_ALLOC_SHIM=OFF` gave `guest_failure` for both ids (the scratch directory was removed). Extra, not in the plan: a scratch `ds_boot` calling `malloc` once turned `alloc_zero_matrix` red with one `made 1 allocator calls` line per row, then `src/core/boot.c` was restored (`git status` clean for `src`). `head -5` of `allocshim.c` and `allockinds.c` carry the SPDX line.
- Task 3: the sanitize run prints both `EXCLUDED_IN_MODE sanitize ID` lines; `grep -c '^RESULT [a-z0-9_]* skip '` over `build/sanitize/results.txt` prints 0. `ls tests/gate/excluded` lists 5 directories; `sh scripts/check` prints 31 stage lines. `head -5` of `EXCLUDED_IN_MODE` carries the two SPDX comment lines. The row-removal observation differs from the plan text, see deviations (interpretation 3).

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 31 `CHECK` stage lines, ends `CHECK RESULT PASS` |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | `SELFTEST RESULT PASS` (includes `SELFTEST mutation-control-anchor wrong-mirror PASS`, `SELFTEST mutation-excluded-gate-pass PASS`) |
| `sh scripts/conformance-run run --preset gate-gcc` and `--preset gate-clang` | `RUNNER_DONE 25`, `CONTROL wrong-mirror RED_OK`, `STATUS conformance ok n=25` for both |
| `sh scripts/audit-history --range f528960..HEAD` | `audit-history: clean` (4 commits, 33 blobs) |

## Deviations from Plan

None - plan executed as written, with the interpretation choices below. No auto-fix was needed: the core and the runner were clean under ASan and UBSan on the first run.

### Interpretation choices (inside the plan's intent)

1. **The alloc_zero kind is in `tests/conformance/allockinds.c`, not in `runner.c`.** The plan lists `runner.c` for Task 2, but adding the kind there left `runner.c` at 65169 bytes, 367 bytes under the 65536-byte limit of the rights-tree criterion and with Task 3 still to add `--exclude-ids`. Plan 07 set the precedent (profile kinds in `profkinds.c`). `runner.c` keeps `boot_row`, the case accessors and the counter marks around `ds_boot` (declared in `runner.h`). The file is registered in `CMakeLists.txt` for the runner and the toys.
2. **The five controls are named `ok-excl`, `excl-unlisted`, `excl-not-pass-in-gate`, `excl-not-required`, `excl-ran-in-mode`.** The plan names the passing one `excl-ok`; `run_controls` treats only a directory named `ok-*` as must-pass, so it is `ok-excl`. Five directories, as the acceptance line requires.
3. **Removing a row of `EXCLUDED_IN_MODE` fails the live sanitize run as `OUTCOME_GUEST_FAILURE`, not `OUTCOME_MISSING_ARTIFACT`.** The exclusion id file is derived from the same file (single source), so with a row removed the case is no longer excluded, runs in the sanitize build and cannot pass without the shim. The `OUTCOME_MISSING_ARTIFACT` form is observed as the plan words it, on a scratch copy of the file with one row removed judged against the clean sanitize results (`conformance-ingest --mode sanitize --excluded SCRATCH --gate-results build/gate/results.txt` gave `OUTCOME_MISSING_ARTIFACT ... required alloc_zero_matrix has no RESULT line`, exit 1), and by the `excl-unlisted` control. The live-run observation (removed, run, restored) was made once and the file restored (`git status` clean).
4. **The Task 2 red commit leaves the gate red between `41ef4d4` and `a796a7d`** (both alloc cases `guest_failure`, as TDD requires). There is no remote write, and `a796a7d` restores green.
5. **`EXCL_BAD_LINE` has no committed control** (the plan lists five); it was probed by hand with a one-word row and a repeated row (both reported, exit 1). Adding a sixth mini-root would change the directory count that the acceptance line fixes at 5.
6. **CONT-03 was not checked off in REQUIREMENTS.md.** This plan delivers the oversized-length row, the sanitizer stage and the zero-allocation witness; the requirement also covers the zero-length, truncated and malformed matrix of plan 11. The frontmatter `requirements-completed` names it because the plan lists it; the orchestrator or plan 11 decides when the box is ticked.

**Total deviations:** 0 auto-fixed, 6 interpretation choices. **Impact:** none on later plans' contracts; `runner.c` headroom is the one thing plan 11 must watch.

## Known Stubs

None. No placeholder, hardcoded empty value or TODO was added.

## Threat Flags

None. No new network endpoint or file access beyond build-directory writes. T-02-19 is mitigated as planned: the 1 MiB cap is a `size_t` comparison before any read, `ref_too_large_size_max` passes under UBSan and ASan, and the shim witnesses zero allocation with a self-check that fails when the shim is absent (observed). T-02-50 is mitigated as planned: the exclusion is a checked file, an unlisted exclusion is `OUTCOME_MISSING_ARTIFACT`, the four `EXCL_*` codes are covered by controls (`EXCL_BAD_LINE` by a manual probe), `mutation-excluded-gate-pass` flips the control, and no `skip` outcome is produced. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `tests/conformance/canary.c`, `tests/conformance/allocshim.c`, `tests/conformance/allockinds.c`, `tests/conformance/EXCLUDED_IN_MODE`, `tests/conformance/cases/ref_too_large_size_max.toml`, `tests/conformance/cases/alloc_shim_selfcheck.toml`, `tests/conformance/cases/alloc_zero_matrix.toml`, and the five `tests/gate/excluded/*/EXPECT`-or-`results.txt` mini-roots.
- Commits `bbd041a`, `41ef4d4`, `a796a7d` and `d07f8c0` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 4.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
