---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 15
subsystem: gate
tags: [core-boundary, freestanding, nm, include-allowlist, writable-symbol, negative-controls, selftest-mutation]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: run_controls and the checker skeleton (02-02), the real core sources src/core (02-05 to 02-14), record-lint stages (02-14)
provides:
  - scripts/core-boundary-check with the include allowlist, the freestanding -O0 compile, the nm undefined-symbol rule, the nm writable-symbol rule and the float rule
  - scripts/core-boundary.allow (memcpy memmove memset memcmp; bzero and __chkstk_darwin on Darwin only)
  - twelve mini-root controls under tests/gate/core-boundary (ten negative, two positive)
  - stages core-boundary and core-boundary-negatives (35 stages), selftest lines mutation-core-boundary-allow and mutation-core-boundary-writable
affects: [02-16, 02-21 (container run supplies the first gcc, mawk and gawk evidence for this checker), later plans that add a core source]

actuals:
  tokens: 7028     # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 28112 chars, 25 files, 473 added lines)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: e8b9ee35b3671585039a72e39c0c2a4cc8a42de4
plan_head_after: e70edfee5a60fea6731cde753d0e20b5b359e8c5
commits: 3

tech-stack:
  added: []
  patterns:
    - "one awk program (written to a temp file by the checker) strips comments, and in float mode string and character literals, keeping line numbers; the same program serves the include rule and the float rule"
    - "the writable-symbol rule is three lines tagged rule-writable, so deleting every tagged line from a scratch copy leaves a valid script and turns the control red"
    - "the defined-symbol set of the core objects is added to the allowed set at run time; the allowlist file itself is never widened"

key-files:
  created:
    - scripts/core-boundary-check
    - scripts/core-boundary.allow
    - tests/gate/core-boundary/ (12 mini-roots, each src/core/*.c, ten with EXPECT)
  modified:
    - scripts/check

key-decisions:
  - "undefined symbols are compared with the allowlist plus the external symbols that other core objects define: each core source is compiled standalone, so an internal cross-module call is undefined in its own object; a name no core object defines (clock, getenv, malloc) is still reported"
  - "the include rule also scans the .h and .def files of src/core, not only the .c files; only .c files are compiled, and a .c file that fails the include rule is skipped"
  - "CB_NO_SYMBOLS is raised when nm lists nothing for a compiled object; on Apple clang even an empty translation unit keeps a local ltmp0 symbol, so the rule is proven by a stub nm on PATH rather than by a source control"

patterns-established:
  - "a boundary control is a mini-root (src/core/*.c plus EXPECT) that declares its own prototypes, so exactly one rule can catch it"

requirements-completed: [CORE-02]

coverage:
  - id: D1
    description: "A build-time check rejects core use of host clock, file, thread, environment, device, UI and any allocator: include allowlist, freestanding compile, undefined symbols a subset of an allowlist, no writable symbol in any core object, no float token"
    requirement: CORE-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage core-boundary prints CHECK core-boundary PASS (SCANNED sources=7 objects=7, STATUS core-boundary ok) on the real core with the original allowlist"
        status: pass
    human_judgment: false
  - id: D2
    description: "Each rule is red on a named negative control (time-include, limits-include, clock-call, env-call, alloc-call, global-counter, local-static, init-data, time-macro, float-use) and green on two positive controls (ok-memset-stack, ok-const-tables), each negative emitting exactly the one code in its EXPECT"
    requirement: CORE-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage core-boundary-negatives prints CHECK core-boundary-negatives PASS; every negative control ran with --expect and printed EXPECT_OK (exact code set)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A clean run prints STATUS core-boundary ok; exit 0 without the marker is a harness error and an object with no symbols is a harness error"
    requirement: CORE-02
    verification:
      - kind: integration
        ref: "scripts/check checker() maps exit 0 without a marker to CHECKER_DEAD (exit 2); a stub nm on PATH gave CB_NO_SYMBOLS exit 2; a root with no .c file gave CB_NO_SOURCES exit 2; --expect with no code gave HARNESS_EMPTY_EXPECT exit 2"
        status: pass
    human_judgment: false
  - id: D4
    description: "Deleting one allowlist line or the writable rule turns a control red"
    requirement: CORE-02
    verification:
      - kind: integration
        ref: "SELFTEST mutation-core-boundary-allow PASS and SELFTEST mutation-core-boundary-writable PASS; full sh scripts/check --selftest ends SELFTEST RESULT PASS"
        status: pass
    human_judgment: false

duration: 22 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 15: Core boundary check Summary

**CORE-02 is now a build-time fact: scripts/core-boundary-check compiles every core source freestanding at -O0 and rejects any non-allowlisted include, any undefined host symbol, any writable data symbol and any float token, with ten named negative controls, two positive controls and two selftest mutations; the real core passes with the original allowlist.**

## Performance

- **Duration:** about 22 min (2026-11-10)
- **Tasks:** 3 of 3 (Task 1 tracer, Tasks 2 and 3 auto)
- **Files:** 25 changed outside .planning (473 added lines, no removals)
- **Gate cost (this machine, warm):** `sh scripts/check` about 52 s wall (35 stages, all PASS); the two new stages add about 1 s.

## Accomplishments

- **Tracer (Task 1).** The checker with `--expect`, `--root`, `--cc`, the WR-05 guard and exit 0/1/2; the include rule (only `<stddef.h>`, `<stdint.h>`, `<stdbool.h>`, quoted `ds_*.h` and quoted `*.def`; `include_next`, `import` and a macro operand are violations) over a comment-stripped line stream; the standalone compile with the exact D-54 flag set (`-std=c11 -O0 -ffreestanding -nostdinc -isystem <compiler include dir> -I src/core -fno-common -fno-stack-protector -fno-pie -Werror=date-time -Wall -Wextra`, plus `-D_FORTIFY_SOURCE=0` on Darwin). The compiler include directory comes from `cc -print-resource-dir` plus `/include`, else `cc -print-file-name=include`. Controls `ok-memset-stack` and `time-include` and the stage `core-boundary-negatives`.
- **Rules and controls (Task 2).** L2 undefined symbols (`nm -P -u`, leading underscore stripped on Darwin, `sort -u` in the C locale, against the allowlist); L3 writable symbols (Darwin `nm -m` sections `__DATA,__data|__bss|__common|__thread*` and the common marker; Linux `nm -P` types `B b C D d G g V v u`) with an empty exception list; CB_NO_SYMBOLS as a harness error; the float rule over comment- and literal-stripped source. Ten more controls, each declaring its own prototypes, and the two selftest mutations.
- **Real core (Task 3).** The first real run was clean for the include, compile, writable and float layers (7 sources, 7 objects). The one finding was a gap in the checker, not in the core: see Deviations. After the fix the real core passes with the original allowlist and `scripts/core-boundary.allow` is byte-identical to its Task 1 commit (`git diff --quiet 4a281a3 -- scripts/core-boundary.allow` exit 0).

## Verification Results

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| Task 1: `sh scripts/check --stage core-boundary-negatives` | exit 0, `CHECK core-boundary-negatives PASS`; re-run in auto mode at the tracer gate, same result (tracer verified end to end, then expanded) |
| Task 2: `sh scripts/check --stage core-boundary-negatives && ... --selftest mutation-core-boundary-allow && ... --selftest mutation-core-boundary-writable` | exit 0, `CHECK core-boundary-negatives PASS`, `SELFTEST mutation-core-boundary-allow PASS`, `SELFTEST mutation-core-boundary-writable PASS` |
| Task 3: `sh scripts/check --stage core-boundary && git diff --quiet -- scripts/core-boundary.allow` | exit 0, `CHECK core-boundary PASS`; allowlist unchanged against the Task 1 commit |

Acceptance criteria, all run:

- Task 1: `core-boundary-check --root tests/gate/core-boundary/ok-memset-stack` printed `SCANNED sources=1 objects=1` and `STATUS core-boundary ok` (exit 0); the `time-include` root with `--expect CB_INCLUDE` printed `EXPECT_OK` and without `--expect` printed `src/core/bad.c  CB_INCLUDE  4  <time.h>` (exit 1); `head -5 scripts/core-boundary.allow | grep -c SPDX` printed 2; the stage `licences` printed PASS.
- Task 2: `ls tests/gate/core-boundary | wc -l` prints 12 (baseline after Task 1: 2); every negative EXPECT holds one word (`wc -w` = 1 for all ten); `STATUS core-boundary ok` on `ok-const-tables`; `grep -c rule-writable scripts/core-boundary-check` prints 3 (baseline 0); `CHECK licences PASS` with all twelve control files present.
- Task 3 (no-writable-static criterion, asserted and not reviewed): the CB_WRITABLE rule runs over the real core in stage `core-boundary` and passes; the rule is proven able to fire by `local-static` (symbol `bad_bump.n`), `global-counter` (`bad_counter`) and `init-data` (`bad_table`), each EXPECT `CB_WRITABLE` and each green in `core-boundary-negatives`. As one more probe, a scratch copy of the real `src/core` with `unsigned ds_probe_counter;` appended to `mapper.c` was reported as `CB_WRITABLE ds_probe_counter` (scratch only, nothing committed). The informational grep for non-const `static` in `src/core` printed nothing.

Plan-end commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 35 `CHECK <stage> PASS` lines including `CHECK core-boundary PASS` then `CHECK core-boundary-negatives PASS`, ends `CHECK RESULT PASS` (about 52 s) |
| `dash scripts/check` | same stage lines PASS, ends `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | 21 mutation lines PASS including `SELFTEST mutation-core-boundary-allow PASS` and `SELFTEST mutation-core-boundary-writable PASS`, ends `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range e8b9ee3..HEAD` | `audit-history: clean` (3 commits, 23 blobs) |

Extra probes (run once, scratch roots outside the repository):

- A root with no `.c` file: `CB_NO_SOURCES`, exit 2. A stub `nm` that prints nothing, put first on PATH: `CB_NO_SYMBOLS the object of src/core/ok.c has no symbols`, exit 2. `--expect ""`: `HARNESS_EMPTY_EXPECT`, exit 2.
- A file whose only mentions of `double` and `float` are in a block comment, a line comment and a string literal: clean, so prose cannot trigger the float rule.
- `ok-memset-stack` leaves `_memset` undefined on this machine (`nm -P -u`), which is what the allowlist mutation relies on; the fill value is a parameter, because a zero fill would become `bzero` on Darwin and the mutation would not bite.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Cross-module references in the core read as host calls**
- **Found during:** Task 2, first run of the undefined-symbol rule on the real core (Task 3 would have met it as the first real run).
- **Issue:** each core source is compiled standalone, so a call from `boot.c` to `ds_resolve` or from `serial.c` to `ds_sha256_update` is an undefined symbol in that object. The rule as written (undefined set a subset of memcpy, memmove, memset, memcmp, plus Darwin entries) reported 12 such names (`ds_policy_of`, `ds_poweron_apply`, `ds_resolve`, `ds_sha256`, `ds_size_classify`, `ds_field_name`, and others). No core source can be edited to remove an internal call, and the plan forbids widening the allowlist.
- **Fix:** the checker first collects the external symbols that every compiled core object defines (`nm -P -g`, type other than U, underscore stripped on Darwin) and allows the allowlist plus that defined set. The allowlist file is unchanged. A host name that no core object defines (`clock`, `getenv`, `malloc` in the controls) is still reported; the three call controls pass with exactly `CB_UNDEF_SYMBOL`. A core that defined its own function named `clock` would satisfy the rule, which is its own code and no host call.
- **Files modified:** scripts/core-boundary-check
- **Commit:** b49a0b9 (Task 2; the fix was made before that commit, so no commit holds the defect)

### Interpretation choices (inside the plan's intent)

1. **Tasks 2 and 3 were verified before Task 3's commit for the checker.** The cross-object fix is in the Task 2 commit because it was found there; Task 3's commit holds only the stage registration, as the real run was clean afterwards.
2. **The include rule also scans `.h` and `.def` files** under `src/core` (the plan says it scans `src/core/*.c`); a header that includes `<time.h>` is thus reported even though no control needs it. Compiling stays limited to `.c` files.
3. **A `SCANNED sources=N objects=N` line precedes `STATUS core-boundary ok`** on a clean run, so the log shows how many sources and objects were read. The marker line is exactly as in the plan.
4. **No source control for CB_NO_SYMBOLS or CB_NO_SOURCES.** Both are exit-2 harness errors, which `run_controls` would report as HARNESS_ERROR and fail the stage. They were proven by the ad hoc probes above instead; on Apple clang a comment-only source still yields an `ltmp0` symbol, so a symbol-free object cannot be produced from source here.
5. **Second compiler path:** `--cc gcc` was run, but on this Mac `/usr/bin/gcc` is Apple clang as well, so it is not gcc evidence (see Platform labelling).

**Total deviations:** 1 auto-fixed (Rule 1), 5 interpretation choices. **Impact:** none on later plans except the notes below.

## Platform labelling

- **Evidence tier:** E0 gate evidence only. This is a static boundary check of the sources it can see; it says nothing about machine behaviour.
- **Compilers and tools run here:** Apple clang 21.0.0 on Darwin arm64, BWK awk, the Darwin `nm`, `/bin/sh` and `dash`. The Linux branch of the writable rule (`nm -P` letters `B b C D d G g V v u`), the gcc include-directory fallback (`-print-file-name=include`) and the mawk and gawk behaviour of the awk lines are written from the research notes and have **not** been executed on this machine. The first real gcc, mawk and gawk evidence for this checker comes from the plan 21 container run and from CI; until then they are unverified.
- **x86_64 macOS** was not run (the `__chkstk_darwin` entry is allowed on Darwin and harmless here).

## Notes for later plans

- **A new core source** is picked up by the glob automatically. A new core file must keep all data `const` (or on the stack), include only the allowed headers, and use no float; a violation names the file, line or symbol.
- **A symbol outside the four builtins** (plus `bzero` and `__chkstk_darwin` on Darwin) that a core object genuinely needs is a stop-and-report event, not an allowlist edit. Cross-module `ds_*` calls need nothing.
- **Plan 16 (determinism-run) and plan 21 (container run):** the core-boundary stage runs inside `scripts/check` with `build/` untouched; the container run must show `CHECK core-boundary PASS` and `CHECK core-boundary-negatives PASS` under gcc, and run the awk lines under mawk and gawk. Expect the Linux symbol-type rule to be the part most likely to need adjustment (for example weak or TLS symbol types); report it rather than widening the rule.
- **`scripts/check` stage count is 35**; the selftest holds 21 lines.

## Task Commits

1. **Task 1 (tracer): include rule and freestanding compile with two controls, wired as a stage** - `4a281a3` (feat)
2. **Task 2: undefined-symbol, writable-symbol and float rules, twelve controls, two selftest mutations** - `b49a0b9` (feat)
3. **Task 3: register the core-boundary stage on the real core** - `e70edfe` (feat)

## Known Stubs

None. No hardcoded empty value or placeholder flows to a consumer in the files of this plan.

## Threat Flags

None. No network endpoint, auth path or schema change at a trust boundary was added. T-02-28 and T-02-29 are mitigated (include allowlist, freestanding compile, undefined-symbol allowlist, empty-exception writable rule, ten named negative controls, two mutation proofs); T-02-30 is mitigated by the marker rule, the exit-2 codes and the rule-deletion selftests. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `scripts/core-boundary-check`, `scripts/core-boundary.allow`, and the twelve `tests/gate/core-boundary/*/src/core/*.c` files.
- Commits `4a281a3`, `b49a0b9` and `e70edfe` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 3.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
