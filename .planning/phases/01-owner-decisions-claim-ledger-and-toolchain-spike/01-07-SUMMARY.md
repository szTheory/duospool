---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 07
subsystem: testing
tags: [tool-01, spike, c11, libfuzzer, asan, ubsan, gates, liveness-control]
status: complete

requires:
  - phase: 01-06
    provides: "C skeleton, shared gates.sh harness with the fuzz60 slot, hook contract, installed-consumer proof"
provides:
  - "spikes/tool-01/c/fuzz/fuzz_loader.c: libFuzzer target over the public duospool_* API"
  - "DS_PLANTED_BUG liveness trap in core.c (only under the macro; CMake never defines it)"
  - "opt_fuzz60, opt_sanitizer, opt_clean, opt_edit comment|real, opt_restore, opt_test in c/hooks.sh (C hook contract complete)"
  - "gates.sh fuzz60 stage (G5): planted liveness first, then clean 60 s"
  - "spikes/tool-01/c/LINES.map for the M09 line-count metric"
affects: [01-08, 01-09, 01-10, 01-11]

tech-stack:
  added: []
  patterns:
    - "Fuzz liveness first: the planted-bug build must crash within 60 s before the clean run counts"
    - "libFuzzer runtime archive from Homebrew llvm@21 linked by explicit path; llvm@21 never on PATH"
    - "Edit hooks assert observable-output equality with a standalone compile outside every build tree, so the bench build trees are not disturbed"

key-files:
  created:
    - spikes/tool-01/c/fuzz/fuzz_loader.c
    - spikes/tool-01/c/LINES.map
  modified:
    - spikes/tool-01/c/src/core.c
    - spikes/tool-01/c/hooks.sh
    - spikes/tool-01/gates.sh
    - spikes/tool-01/LOG.md

key-decisions:
  - "ASAN_OPTIONS=handle_sigtrap=1 is set for the fuzz run: __builtin_trap raises SIGTRAP on arm64 and ASan ignores it by default, so without it the planted run is killed with no libFuzzer report and no crash artifact. A runtime environment setting, not a toolchain change; the protocol is not amended."
  - "opt_fuzz60 returns 0 when the outcome is the wanted one (planted: crash found; clean: no crash), replacing the provisional inverted-status reading in gates.sh"
  - "opt_edit real toggles `inst->loaded = 1;` and `inst->loaded = 2;` (field only compared with 0); the hook checks the runner transcript is identical before and after"

patterns-established:
  - "hooks.sh defaults SPIKE_DIR and OPT_DIR from $PWD when sourced from the repo root without them (one-off verify commands)"

requirements-completed: []  # TOOL-01 advances here (C option complete); it completes with the language decision record

actuals:
  tokens: 3640    # chars/4 over the realized git diff of the two code commits (14559 chars); this SUMMARY and the state commit are not included
  tasks: 2
  commits: 2
plan_head_before: 1d4038f75687af14494c79aecae98c306b84b914
plan_head_after: 83770426d1809ea605fec73de68e39ebf66e6a11

coverage:
  - id: D1
    description: "sh spikes/tool-01/gates.sh c passes build, G1, G2, G3, G4 and fuzz60 (G5) and ends GATES c RESULT PASS"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "saved spikes/tool-01/out/gates-c-full.txt from the end-of-plan SLOW run: GATE fuzz60 PASS, GATES c RESULT PASS, exit 0 (two full runs, both green)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Planted DSPB bug is found within 60 s through the real libFuzzer path and the clean 60 s run does not crash"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "LIVENESS found_after=1.2 (Task 1 run) and 0.4 (end-of-plan run); clean FUZZ elapsed=61.1 crashed=no execs_per_sec=29301 and 39814; crash artifact input contains DSPB, ASan TRAP report at core.c"
        status: pass
    human_judgment: false
  - id: D3
    description: "DS_PLANTED_BUG never reaches the product build: CMakeLists.txt does not mention it, G4 consumer-proof still matches the golden line"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "grep -c DS_PLANTED_BUG c/CMakeLists.txt prints 0; GATE consumer-proof PASS in both full runs"
        status: pass
    human_judgment: false
  - id: D4
    description: "M08 for C: manifest runner under Apple clang ASan+UBSan prints SANITIZER PASS"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "opt_sanitizer prints SANITIZER PASS; the built binary carries ASan symbols (119 matches); LOG.md has SANITIZER c PASS"
        status: pass
    human_judgment: false
  - id: D5
    description: "Bench hooks work; the real edit leaves the transcript unchanged; restore leaves c/ clean"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "opt_edit real && opt_test debug && opt_test release && opt_restore, then opt_edit comment && opt_restore && opt_clean release, all exit 0; git status --porcelain -- spikes/tool-01/c empty after the commit"
        status: pass
    human_judgment: false
  - id: D6
    description: "LINES.map maps all seven committed C-option files to core, ffi-glue, build, scripts or runner"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "7 tab rows; every mapped path exists; the map equals git ls-files under c/ minus LINES.map itself"
        status: pass
    human_judgment: false

duration: ~17 min
completed: 2026-11-09
---

# Phase 1 Plan 07: C fuzz gate G5, sanitizer row, bench hooks and line map Summary

**The C11/CMake option now passes all five hard gates on this Mac: a libFuzzer run linked from the Homebrew llvm@21 archive by explicit path finds the planted `DSPB` trap in about a second and the clean 60-second run does not crash, and the M08 ASan+UBSan manifest run, the bench hooks and the line map are in place.**

## Performance

- **Duration:** about 17 minutes (including two roughly three-minute SLOW gate runs and a five-minute `scripts/check`)
- **Completed:** 2026-11-09
- **Tasks:** 2 (1 tracer, 1 auto), two code commits

## Accomplishments

- **Tracer through the whole G5 path first.** A 4-byte `DSPB` input reaches `core.c` ahead of the length-refusal ladder, traps under the instrumented build, and libFuzzer reports it and writes a crash artifact. Then the full gate was run once to confirm planted liveness and the clean run together (tracer verified end to end before Task 2).
- **Explicit-path link worked first time**, so no fallback (llvm@21 `-fsanitize=fuzzer,undefined`, Linux container) was needed. Recipe: `/usr/bin/clang -g -O1 -fsanitize=fuzzer-no-link,address`, linked with `/opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/libclang_rt.fuzzer_osx.a -lc++`. The portability cost is recorded in LOG.md: the runtime comes from a Homebrew prefix because Apple clang 21 ships no libFuzzer runtime.
- **Liveness before cleanliness (T-01-30).** `gates.sh` runs the planted build first, prints `LIVENESS found_after=<s>`, requires at most 60 s, and only then runs the clean 60 s build. `--no-fuzz` still prints `GATE fuzz60 SKIPPED` and ends `PASS-NO-FUZZ`.
- **M13 samples:** time-to-find 1.2 s and 0.4 s (two runs). Clean run throughput 29301 and 39814 exec/s (an indicator only, M12).
- **M08 for C:** `opt_sanitizer` compiles `core.c` and `runner.c` directly with `-fsanitize=address,undefined -fno-sanitize-recover=undefined`, runs the whole manifest, and fails on any stderr report or failing case. Result `SANITIZER PASS`.
- **Bench hooks:** `opt_clean`, `opt_edit comment|real`, `opt_restore`, `opt_test [debug|release]`. The `real` edit changes code (`inst->loaded = 1;` to `= 2;`, the field is only compared with 0) and the hook proves the runner transcript is identical before and after, using a standalone compile outside every build tree so a later bench build is not pre-warmed or disturbed.
- **LINES.map:** seven rows, equal to the tracked files under `c/` apart from itself.
- `sh scripts/check` ends `CHECK RESULT PASS`; `git ls-files spikes/tool-01/out` prints nothing.

## Task Commits

1. **Task 1 (tracer): libFuzzer gate G5 with planted DSPB liveness control** - `dddd803` (feat)
2. **Task 2: sanitizer row, bench hooks and LINES.map** - `8377042` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The planted trap produced no libFuzzer report on arm64**
- **Found during:** Task 1 (first 10 s planted run)
- **Issue:** `__builtin_trap()` raises SIGTRAP on arm64. ASan does not handle SIGTRAP by default, so the process was killed ("Trace/BPT trap") with no libFuzzer message and no crash artifact. The run was still non-zero, but the "found through the real libFuzzer path" evidence (report plus artifact) was missing.
- **Fix:** the fuzz run sets `ASAN_OPTIONS=handle_sigtrap=1`; libFuzzer then prints the ASan TRAP report at `core.c:59` and writes a crash artifact. The planted-run success check requires an `ERROR: libFuzzer` or `ERROR: AddressSanitizer` line in the log, not only a non-zero status.
- **Files modified:** spikes/tool-01/c/hooks.sh
- **Commit:** dddd803

### Plan details not followed literally

- **`opt_fuzz60` status convention.** Plan 01-06 left a provisional `fuzz60` stage that read a zero exit of the planted run as "liveness broken". This plan's task text defines planted success as a crash. `opt_fuzz60` now returns 0 when the outcome is the wanted one for its mode, and the stage was rewritten to match (planted first, then clean), as plan 01-06 allowed.
- **Zero-filled 64 KiB seed in the clean run.** One generated all-zero seed (under the gitignored `out/`) is added to each run's corpus so the clean run also exercises the accepted-load and step path rather than only refusals. It is generated at run time and never committed.
- **Known noise:** the planted run prints a shell job-status line ("Abort trap: 6") into `out/fuzz60-planted.c.txt`. It does not affect any gate.

The protocol was not edited and no amendment was needed. No Known Stubs. No new security-relevant surface beyond the plan's threat model (T-01-29 and T-01-30 mitigations are implemented and exercised above).

## Issues Encountered

- `sh scripts/check` takes about five minutes on this machine (the synthetic protocol tests dominate). It passed; the cost is noted for later plans that run it per commit.

## Next Phase Readiness

- The C option has every hook the bench plan needs: `opt_clean`, `opt_edit`, `opt_restore`, `opt_test`, `opt_sanitizer`, `opt_fuzz60`. `LINES.map` is ready for M09.
- LOG.md records `c-work-end: 2026-11-09T22:10:48Z`, so plan 01-08 can begin the Rust option; the 8 h timebox started 2026-11-09T21:53:09Z.

## Self-Check: PASSED

- `spikes/tool-01/c/fuzz/fuzz_loader.c` and `spikes/tool-01/c/LINES.map` exist; commits `dddd803` and `8377042` are ancestors of HEAD.
- `git ls-files spikes/tool-01/out` prints nothing; `git status --porcelain -- spikes/tool-01/c` is empty.
