---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 06
subsystem: testing
tags: [tool-01, spike, c11, cmake, ninja, pkgconf, gates, installed-consumer, fixtures, fnv-1a]
status: complete

requires:
  - phase: 01-05
    provides: "pre-registered protocol (W-01..W-12, G1..G5, M01..M15), protocol-order-check and the scorer, both committed before any spikes/ file"
provides:
  - "spikes/tool-01/gates.sh: the shared hard-gate harness (G1-G4 live, G5 slot) for both options, with the hook contract"
  - "spikes/tool-01/fixtures.manifest + gen-fixtures.sh: byte-identical shared manifest and a deterministic LCG fixture generator into the gitignored out/"
  - "spikes/tool-01/consumer/{main.c,expect.pl,golden.txt}: the independent installed-package consumer and a Perl re-implementation of the W-07 digest"
  - "spikes/tool-01/c/: hand-written duospool.h (canonical W-04 copy), C11 core, manifest runner, CMake install layout, .pc, hooks.sh"
  - "LOG.md: written-first c, timebox start and end, toolchain versions, M01 cold-first-build c 0.768"
affects: [01-07, 01-08, 01-09, 01-10, 01-11]

tech-stack:
  added: []
  patterns:
    - "Gate stages run in the main shell and return explicit statuses; errexit is not relied on inside the if that runs a stage"
    - "Every gate that can pass vacuously has a live control: header-removed build, seeded path leak (remap no), wrong-expectation case"
    - "Installed-consumer proof from a throwaway copy made unreadable, consuming only the installed .a, header and .pc through pkgconf --static"

key-files:
  created:
    - spikes/tool-01/LOG.md
    - spikes/tool-01/fixtures.manifest
    - spikes/tool-01/gen-fixtures.sh
    - spikes/tool-01/gates.sh
    - spikes/tool-01/consumer/main.c
    - spikes/tool-01/consumer/expect.pl
    - spikes/tool-01/consumer/golden.txt
    - spikes/tool-01/c/CMakeLists.txt
    - spikes/tool-01/c/include/duospool.h
    - spikes/tool-01/c/src/core.c
    - spikes/tool-01/c/src/runner.c
    - spikes/tool-01/c/duospool.pc.in
    - spikes/tool-01/c/hooks.sh
  modified: []

key-decisions:
  - "opt_install builds RelWithDebInfo in both the remapped and the seeded-leak run, so the leak grep inspects an archive that carries DWARF paths; the build type is logged as INSTALL-BUILD-TYPE c RelWithDebInfo"
  - "Prefix remapping covers both spellings (/var/... and /private/var/...) of the source copy and build directory, because the compiler may see either"

patterns-established:
  - "Hook contract sourced by gates.sh with SPIKE_DIR and OPT_DIR set by the caller; build trees live under the gitignored spikes/tool-01/out/"

requirements-completed: []  # TOOL-01 is advanced here (C skeleton, shared harness); it completes with the language decision record

actuals:
  tokens: 11200   # chars/4 over the realized git diff of the two code commits (44783 chars); this SUMMARY and the state commit are not included
  tasks: 2
  commits: 2
plan_head_before: ae18874e0b826dab78785a13dd6f64d47072f80c
plan_head_after: 52bc510183391a3cc46ad8f2d9d388188f2e03c9

coverage:
  - id: D1
    description: "The first spikes/ commit is a strict descendant of the protocol and scorer commits; protocol-order passes with first_spike set"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/protocol-order-check prints first_spike=1e3d304dd44fd959fc028973722caec4502bc3ca amended_after_spike=no; sh scripts/check ends CHECK RESULT PASS after both commits"
        status: pass
    human_judgment: false
  - id: D2
    description: "sh spikes/tool-01/gates.sh c --no-fuzz passes build, test-refusals (G1), test-controls (G2), test-interleave (G3), transcript and consumer-proof (G4), prints GATE fuzz60 SKIPPED and ends GATES c RESULT PASS-NO-FUZZ"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh spikes/tool-01/gates.sh c --no-fuzz (about 2 s warm); mutation len >= cap in core.c turned test-refusals and test-controls red, then restored"
        status: pass
    human_judgment: false
  - id: D3
    description: "Boundary: 1048576 bytes gives REFUSE_BAD_SIZE and 1048577 gives REFUSE_OVERSIZED without the buffer being read"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "transcript.c.txt lines refuse-cap REFUSE_BAD_SIZE PASS and refuse-over REFUSE_OVERSIZED PASS; ad-hoc ASan+UBSan run with an 8-byte buffer and lengths 1048577, 1048576 and SIZE_MAX returned 4 3 4 with no report"
        status: pass
    human_judgment: false
  - id: D4
    description: "Installed-consumer proof: consumer built only against the installed .a, header and .pc with the source and build copies unreadable, output equals expect.pl; removing the installed header breaks the build; LEAK 0 for the remapped prefix and SEEDED-LEAK 10 for the unremapped build"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "GATE consumer-proof PASS with LEAK 0 and SEEDED-LEAK 10; a deliberately wrong golden.txt turned consumer-proof FAIL and was restored"
        status: pass
    human_judgment: false
  - id: D5
    description: "Generated fixtures are only in the gitignored out/ and no fixture byte file is tracked; lcg-64k.bin sha256 starts ab0b5936cf56fca6"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "git ls-files spikes/tool-01/out prints nothing; FIXTURE lines all OK; an independent Perl LCG gave the same lcg-64k sha256"
        status: pass
    human_judgment: false
  - id: D6
    description: "LOG.md records written-first: c, the timebox start and end, the cmake, ninja and both clang versions, and exactly one M01 cold-first-build c line from the first-ever C build"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "grep of LOG.md: written-first: c, M01 cold-first-build c 0.768 (one line), INSTALL-BUILD-TYPE c RelWithDebInfo"
        status: pass
    human_judgment: false

duration: ~10 min
completed: 2026-11-09
---

# Phase 1 Plan 06: C skeleton, shared gate harness and installed-consumer proof Summary

**The C11/CMake skeleton implements the pre-registered workload and passes hard gates G1 to G4 on this Mac: a generated LCG fixture flows through the C core, an install prefix and an independent consumer to a golden line computed by a separate Perl implementation, with live controls for a wrong expectation, a missing header and an unremapped path leak.**

## Performance

- **Duration:** about 10 minutes (LOG start 21:53:09Z, last commit about 21:59Z; no start epoch captured separately)
- **Completed:** 2026-11-09
- **Tasks:** 2 (1 tracer, 1 auto), two code commits
- **Files:** 13 created (1136 insertions)

## Accomplishments

- **Tracer through the whole path first.** Task 1 crossed fixture generator, manifest, core, CMake install, `.pc`, installed prefix and consumer to the golden line before the runner existed. The test-* stages printed FAIL with a stated reason ("opt_runner is not defined") until Task 2, as the plan expects.
- **Independent golden.** `expect.pl` computes FNV-1a 64 (digest `c61f7b5e0b76fc29` for lcg-64k plus steps 100, 200, 300, time 600) using two 32-bit halves, so no multiplication leaves exact integer range. It was first checked against the published FNV-1a vectors for "a" (`af63dc4c8601ec8c`) and "foobar" (`85944171f73967e8`). The C core independently produces the same digest. Matching digests here show two implementations of one spec agree (repeatability class), not hardware correctness.
- **G4 proof is live.** The source copy and both build directories are made unreadable (`ls` and `cat` are checked to fail), the consumer is built with `PKG_CONFIG_LIBDIR=<prefix>/lib/pkgconfig cc $(pkgconf --static --cflags --libs duospool) main.c`, and removing the installed header makes the build fail with an error that names `duospool.h`. The remapped archive and `.pc` contain 0 occurrences of the work dir (both spellings), the build dirs and `$HOME`; the same grep finds 10 in the deliberately unremapped build, so the check is not vacuous.
- **Boundary behaviour.** `refuse-cap` (1048576 bytes) gives REFUSE_BAD_SIZE and `refuse-over` (1048577) gives REFUSE_OVERSIZED. The loader decides oversize from the length alone; an ad-hoc ASan+UBSan run that passed an 8-byte buffer with lengths 1048577, 1048576 and SIZE_MAX reported nothing. A temporary off-by-one mutation at the cap turned the gate red (test-refusals and test-controls FAIL), then was restored with `git checkout`.
- **M01 cold-first-build c = 0.768 s**, logged once from the first cmake invocation of the spike (empty build directory, library and install targets only). The very small value is expected for one translation unit; it is not extrapolated to emulator scale.
- Full run, warm: `sh spikes/tool-01/gates.sh c --no-fuzz` takes about 2 s and ends `GATES c RESULT PASS-NO-FUZZ`. `sh scripts/check` still ends `CHECK RESULT PASS`.

## Task Commits

1. **Task 1 (tracer): C core through install prefix and independent consumer to the golden line** - `1e3d304` (feat). First commit under `spikes/`; `protocol-order` now reports `first_spike=1e3d304dd44fd959fc028973722caec4502bc3ca`.
2. **Task 2: C manifest runner, gates G1-G3, G4 header-removed, path-leak and seeded-leak controls** - `52bc510` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] `chmod -R a-rwx` fails on macOS when applied top-down**
- **Found during:** Task 1 (first gates.sh run)
- **Issue:** BSD `chmod -R a-rwx dir` closes the directory before descending and prints `Permission denied` for each top directory. Inside a stage function the non-zero exit was silently ignored (errexit does not apply there), so the gate passed while printing errors and, in principle, leaving the contents' modes unchanged.
- **Fix:** the same mode change is applied to every entry children-first, with the exit status checked: `find <dirs> -depth -exec chmod a-rwx {} +`. The effect the protocol asks for (copy and build directories unreadable) is the same, and the gate additionally verifies unreadability with `ls` and `cat` (fails honestly if run as root). Restoration in the EXIT trap still uses `chmod -R u+rwx`, which works top-down.
- **Files modified:** spikes/tool-01/gates.sh
- **Commit:** 1e3d304 (the fix was made before the first commit)

### Plan details not followed literally

- **Both temp-path spellings in the prefix remap, and `$work` as a leak needle.** The plan lists the copy and build paths; the hook also maps the `pwd -P` spelling, and the leak grep also searches the whole work dir and its `pwd -P` spelling. These are supersets of the specified checks (a leaked prefix path would be caught too).
- **An extra `GATE fixtures` stage line** (generator plus sha256 verification) precedes `GATE build`. It makes a fixture mismatch visible as its own stage instead of as a downstream failure.
- **`fuzz60` stage body is provisional.** Without `--no-fuzz`, gates.sh calls `opt_fuzz60 60 no` (must not crash, prints `FUZZ ... crashed=no`) and `opt_fuzz60 60 yes` (the planted-bug build must be found, `crashed=yes`). Until plan 01-07 defines `opt_fuzz60` the stage reports FAIL with "opt_fuzz60 is not defined"; plan 01-07 owns G5 and may refine the stage.
- **lcg-64k.bin is cut from the first 98304 LCG bytes** (the same LCG stream, one awk run) instead of running the LCG twice. Its sha256 matches `ab0b5936cf56fca6...` and an independent Perl LCG.

The protocol was not edited and no amendment was needed.

## Issues Encountered

- `hyperfine` is not installed on this machine (`which hyperfine` finds nothing). It is not needed by this plan, but the M02-M07 rows in plan 01-10 use it. Installing it is a package-manager install, so it is flagged here for plan 01-10 rather than done in this plan.
- No Known Stubs. No new security-relevant surface beyond the plan's threat model (T-01-16, T-01-17, T-01-18 mitigations are implemented and exercised above).

## Next Phase Readiness

- Plan 01-07 can add `opt_fuzz60`, `opt_sanitizer`, `opt_clean`, `opt_edit`, `opt_restore`, `opt_test` to `spikes/tool-01/c/hooks.sh` and `LINES.map`. `opt_runner [profile]` and `opt_build <profile>` already follow the contract (build trees: `spikes/tool-01/out/build-c-<profile>`).
- Plans 01-08 and 01-09 reuse gates.sh, fixtures.manifest, expect.pl and the consumer unchanged, and must copy `duospool.h` byte-for-byte from `spikes/tool-01/c/include/duospool.h` (W-10).
- The timebox runs from the first spikes/ commit: LOG.md records start `2026-11-09T21:53:09Z` and end `2026-11-10T05:53:09Z`.

## Self-Check: PASSED

- All 13 files in key-files.created exist on disk.
- Commits `1e3d304` and `52bc510` are ancestors of HEAD.
- `git ls-files spikes/tool-01/out` prints nothing.
