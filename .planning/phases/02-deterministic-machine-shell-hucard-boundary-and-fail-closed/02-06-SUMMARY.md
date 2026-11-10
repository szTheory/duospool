---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 06
subsystem: text
tags: [c11, ds_text, record-printer, canonical-text, r1-equality, field-flip, outcome-sync, cmake-presets, gate-stage]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: ds_core, the 13-field X-table, ds_resolve and ds_boot (02-05); the runner, conformance-run run mode and DS_EXTRA_FLAGS presets (02-05); outcomes.def and outcome-sync (02-02, 02-03); run_controls and --stage (02-01)
provides:
  - src/text (ds_text static library, links ds_core): ds_text_record and ds_text_profile, canonical flat-table printer with no parser in C and no library call
  - runner kinds record_lines, profile_equal, names_both_ways, field_flip; flags --profile-tsv, --rules-fields, --outcomes
  - cases record_skeleton_256k, profile_r1_equal, profile_names_both_ways, field_flip (all in REQUIRED; 10 cases in all)
  - ds_profile_unresolved_count and ds_profile_unresolved: the unresolved_behaviours ids of r1 carried by the built-in constant
  - presets gate-gcc and gate-clang (configure plus same-named build preset, own binaryDir)
  - scripts/outcome-sync --names FILE, tests/gate/outcome-names (2 controls), gate stage outcome-sync (29 stages), selftest mutation-outcome-names
affects: [02-07 (content sha256 becomes the first key of the content table), 02-12 (power_on table), 02-13 (profile hash reuses ds_profile_unresolved), 02-14 (completes text.c: profile sha256, withheld, power_on, machine, refusal record; record_golden), 02-10 (sanitize preset reuses DS_EXTRA_FLAGS), 02-21 (real gcc run of gate-gcc)]

actuals:
  tokens: 18570    # chars/4 over the realized diff (git diff 1dd4212..fdcc748 excluding .planning, 74278 chars, 20 files, 1420 added lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 1dd4212951679e9f8b4954027fa14dc1203775f9
plan_head_after: fdcc748af06c4c307f19a77312cf10f349d11a0b
commits: 4

tech-stack:
  added: []
  patterns:
    - "the printer owns no library call: a bounded appender with a sticky error, a decimal printer and one validator for every string; on any error the buffer is an empty string and the length is 0, so a failed print never reads as a short record"
    - "the identity text is one function used twice: ds_text_profile prints the [profile] and field tables, ds_text_record embeds the same text, and a case proves it is an exact part of the record"
    - "the parser of hand-edited data stays in sh and awk: conformance-run flattens r1.toml and cuts the 13 names out of profile-rules.awk, the runner reads two line formats"
    - "a stage that needs a binary builds it itself (stage_outcome_sync calls stage_build first), so it can never read an old binary"
    - "an equality case documents the gaps of the frozen record and asserts them instead of skipping them"

key-files:
  created:
    - src/text/ds_text.h
    - src/text/text.c
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/cases/profile_r1_equal.toml
    - tests/conformance/cases/profile_names_both_ways.toml
    - tests/conformance/cases/field_flip.toml
    - tests/gate/outcome-names/ok-same/ (names.txt, tests/conformance/outcomes.def)
    - tests/gate/outcome-names/compiled-list-differs/ (names.txt, EXPECT, tests/conformance/outcomes.def)
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - src/core/ds_core.h
    - src/core/profile.c
    - tests/conformance/runner.c
    - tests/conformance/REQUIRED
    - scripts/conformance-run
    - scripts/outcome-sync
    - scripts/check

key-decisions:
  - "the record is printed from instance fields and strings only, with a closed status enum (ok, argument, capacity, string, state); a double quote, a backslash, a byte outside printable ASCII or an unterminated field is DS_TEXT_ERR_STRING and nothing is escaped (D-69)"
  - "ds_text_profile is a second entry point (profile and field tables, no content or limits) so field_flip can print a modified copy of a resolved profile without copying a 1 MiB instance"
  - "the r1 constant carries unresolved_behaviours through an accessor pair in profile.c (three sorted ledger ids), not through struct ds_profile, so the struct stays free of pointers and padding; plan 13 hashes the ids through the same accessor"
  - "where the frozen r1 has no allowed list the case asserts the core list instead of skipping it: empty for vdc_count, vpc and input_port1, and the D-67 set zero|seeded-prng-v1 for powerup_policy"
  - "field_flip makes one assertion per field and each assertion requires that a change of value, of status and of binder all change the printed text"
  - "the field names of profile-rules.awk reach the runner as a line file cut out by conformance-run, because C must not parse awk (D-40, D-51)"
  - "gate-gcc and gate-clang inherit the hidden base and add only -fwrapv, the compiler and the binaryDir, so the sanitize preset of plan 10 can inherit the base without -fwrapv (D-83)"

patterns-established:
  - "case kinds that need extra inputs fail as harness_failure when a flag is missing, never as a pass"
  - "scalar expectations of a record case are TABLE.KEY=VALUE items separated by semicolons; the table and key order is checked separately from an independently built list of 50 rows"
  - "negative controls of the printer live inside the case that checks it (quote, backslash, non-ASCII, unterminated field, unknown enumeration, short buffer)"

requirements-completed: [PROF-02, PROF-03, RUN-02]

coverage:
  - id: D1
    description: "A canonical record skeleton is printed outside the core by ds_text: 50 rows in fixed table and key order, LF and printable ASCII only, decimal integers, and a quote, a backslash or a short buffer makes the printer return an error status and emit nothing"
    requirement: PROF-03
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance -> CHECK conformance PASS; RESULT record_skeleton_256k pass assertions=73; swapping two content keys in text.c made the case guest_failure (row out of order), restored"
        status: pass
    human_judgment: false
  - id: D2
    description: "The built-in r1 constant equals profiles/pce-hucard-2btn/r1.toml field by field (13 fields, 6 owner-selected, 7 open), including the unresolved behaviour ids, and the three documented gaps of the frozen record are asserted"
    requirement: PROF-02
    verification:
      - kind: integration
        ref: "RESULT profile_r1_equal pass assertions=81; red in commit 83a8a3d (one guest_failure: the unresolved ids were missing from the constant), green in c52ef41; editing the lines_per_frame allowed list in a scratch r1 TSV made it guest_failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "Flipping the value, the status or the binder of any one of the 13 fields changes the printed profile identity text, and the field-name set is the same in the X-list, the r1 table names and scripts/lib/profile-rules.awk, in both directions"
    requirement: PROF-03
    verification:
      - kind: integration
        ref: "RESULT field_flip pass assertions=13; RESULT profile_names_both_ways pass assertions=54; dropping the bound_by line from the printer made field_flip guest_failure naming media_slot; renaming psg_tick_divisor in a scratch r1 TSV and lines_per_frame in a scratch rules list each made the names case guest_failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "The outcome names compiled into the runner equal outcomes.def and the seven names of the success criterion, checked by a gate stage that builds its own input"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage outcome-sync -> CHECK outcome-sync PASS; tests/gate/outcome-names has ok-same and compiled-list-differs (EXPECT OUTSYNC_NAME); sh scripts/check --selftest mutation-outcome-names -> PASS; after rm -f build/gate/ds_runner build/gate/outcome-names.txt the stage rebuilds and passes"
        status: pass
    human_judgment: false
  - id: D5
    description: "Second-compiler presets gate-gcc and gate-clang exist with their own binary directories and build the runner, which then passes the whole conformance run"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "cmake --preset NAME && cmake --build --preset NAME exit 0 for both; sh scripts/conformance-run run --preset NAME -> STATUS conformance ok n=10 for both; cmake --list-presets=build lists gate, gate-gcc, gate-clang. gate-gcc resolves to Apple clang on this machine: not a real gcc check"
        status: pass
    human_judgment: false

duration: 11 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 06: Record printer, r1 equality, field flip and outcome-name sync Summary

**`ds_text` prints the canonical record skeleton (50 rows, no library call, a quote or backslash is an error and an empty text), the r1 constant is proved equal to the frozen `r1.toml` field by field and shown to change the printed identity when any one of 13 fields changes, and the outcome names compiled into the runner are proved equal to `outcomes.def` by a new gate stage; `gate-gcc` and `gate-clang` presets exist.**

## Performance

- **Duration:** 11 min (2026-11-10T04:44:54Z to about 04:56Z)
- **Tasks:** 3 of 3 (Task 1 is the tracer, Task 2 is TDD)
- **Files:** 20 changed outside .planning (1420 added lines, 30 removed)
- **Timing baseline (D-60, record only, never a failure):** `sh scripts/check --timing`, 29 stages, 21 s wall on this machine. Every stage is 0 or 1 s except `protocol-negatives` 4 s, `audit-synthetic` 4 s and `rights-negatives` 3 s. `build`, `outcome-sync` and `conformance` are 0 s because the build tree is incremental; a cold `cmake --preset gate-clang` plus build measured 1 s. No `TIMING_WARN` line (the warn threshold is 20 s per stage).

## Accomplishments

- **Printer (D-40, D-53, D-69).** `src/text/ds_text.h` and `text.c` print `[record]`, `[core]` (build_id, behaviour_epoch), `[profile]`, 13 `[field_NAME]` tables (status, value, bound_by), `[content]` (size, mapper, mapper_rule, byte_order, header_handling) and `[limits]`. Tables are flat, one blank line apart, LF only, integers decimal, strings printable ASCII in double quotes. There is no stdio and no string.h: a bounded appender with a sticky error, a decimal printer and one validator. Any error leaves `buf[0] = 0` and `*outlen = 0`. `build_id` comes from the CMake cache variable `DS_BUILD_ID` (default the literal `unset`) through a compile definition of `ds_text` only. The printed text is valid for the TOML subset: a scratch dump flattened cleanly through `conformance-flatten` (`END 1`, `STATUS flatten ok 1`).
- **Targets.** `ds_text` is a static library that links `ds_core` PUBLIC; `ds_runner` links `ds_text` and `ds_core`. `src/core` mentions `ds_text` nowhere (working-tree grep, with the `test -f src/core/ds_core.h` anchor first).
- **record_skeleton_256k (73 assertions).** Boots the default accepted request and checks: the 50-row skeleton from an independently built key list (so the printer cannot agree with itself), layout (LF, blank line only before a table header, nothing at the end), only LF and 0x20..0x7e bytes, the ten scalar lines of the case, status, value and binder of each of the 13 field tables from the built-in profile and the request, the profile text as an exact part of the record after a blank line and before `[content]`, a second print equal to the first, the capacity edge (len + 1 fits, len gives an error and an empty text, a NULL buffer is an argument error), and six refusals (double quote in a field value, backslash in a field value, double quote in the mapper, backslash in the profile id, byte 0x80, an unterminated mapper name) plus two state errors (unknown field status, unknown byte order).
- **profile_r1_equal (81 assertions), profile_names_both_ways (54), field_flip (13).** `conformance-run` now flattens `profiles/pce-hucard-2btn/r1.toml` into `build/PRESET/r1.tsv` (and requires its END row) and cuts the 13 names out of `scripts/lib/profile-rules.awk` into `rules-fields.txt` (exactly 13 lines, else harness error). The equality case compares status, owner values, allowed lists, absence of any open value, that each open proposal is an allowed value, 6 owner and 7 open fields, root `profile_id`, `rev` and the `unresolved_behaviours` ids both ways. The names case checks X-list, r1 tables and the rules list against each other in both directions. The flip case resolves the default request, prints the profile identity text and, per field, changes the value, the status and the binder in a copy: one assertion per field that all three change the text.
- **Presets (D-86).** `gate-gcc` and `gate-clang` inherit `gate-base` and add `DS_EXTRA_FLAGS = -fwrapv`, `CMAKE_C_COMPILER` and their own `binaryDir`, each with a same-named build preset. `cmake --list-presets=build` lists `gate`, `gate-gcc`, `gate-clang`.
- **Outcome names (D-57, D-85).** `ds_runner --outcomes` prints the compiled names from the same `outcomes.def` include. `scripts/outcome-sync --names FILE` compares the list row for row and in order against the def rows (any difference is `OUTSYNC_NAME`; a missing file is exit 2). Stage `outcome-sync` is registered immediately after `build`; it calls the build itself, writes `build/gate/outcome-names.txt` from the fresh binary, judges the real repository and runs `tests/gate/outcome-names` (`ok-same`, `compiled-list-differs`) through `run_controls`.

## Task Commits

1. **Task 1 (tracer): ds_text prints the record skeleton and a case checks it line by line** - `cdbaaf6` (feat)
2. **Task 2 RED: r1 equality, names both ways and field flip, red against the plan 05 constant** - `83a8a3d` (test)
3. **Task 2 GREEN: the r1 constant carries the unresolved behaviour ids** - `c52ef41` (feat)
4. **Task 3: second-compiler presets and runner-compiled outcome names in outcome-sync** - `fdcc748` (feat)

## Verification Results

Tracer gate (auto mode): after `cdbaaf6` the tracer verify was re-run (`sh scripts/check --stage conformance`, exit 0, `RESULT record_skeleton_256k pass assertions=73`); log line `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance` (Task 1) | exit 0; `CHECK conformance PASS`; results.txt `RESULT record_skeleton_256k pass assertions=73`, `RUNNER_DONE 7` |
| `sh scripts/check --stage conformance` (Task 2, RED, commit 83a8a3d) | exit 1; ten RESULT lines, one `guest_failure` (profile_r1_equal: "an unresolved behaviour id of r1 is missing from the constant"), nine pass (field_flip 13, profile_names_both_ways 54), `RUNNER_DONE 10` |
| `sh scripts/check --stage conformance` (Task 2, GREEN) | exit 0; ten pass lines, `RUNNER_DONE 10`, `field_flip pass assertions=13` |
| `cmake --preset gate-clang && cmake --build --preset gate-clang && sh scripts/check --stage build && sh scripts/check --stage outcome-sync && sh scripts/check --stage conformance` (Task 3) | exit 0; `CHECK build PASS`, `CHECK outcome-sync PASS`, `CHECK conformance PASS` |

Acceptance criteria, all run:

- Task 1: results.txt has `RESULT record_skeleton_256k pass` and `RUNNER_DONE 7`. `test -f src/core/ds_core.h && ! grep -rn 'ds_text' src/core` holds (no match). `head -5` of `ds_text.h` and `text.c` shows `SPDX-License-Identifier: MIT OR Apache-2.0`; `sh scripts/check --stage licences` PASS.
- Task 2: ten pass lines and `RUNNER_DONE 10`; `field_flip` reports `assertions=13`. Observed once and reverted (scratch copy of the r1 TSV, runner run directly): changing the `lines_per_frame` allowed list from `262|263` to `262|264` gave `RESULT profile_r1_equal guest_failure` ("field lines_per_frame: the allowed list differs from r1"). The head of each of the three new case files holds the 0BSD header comment.
- Task 3: `build/gate-clang/ds_runner` exists. `cmake --list-presets=build` lists the three presets (baseline: gate only); `cmake --build --preset gate-gcc` exits 0 after `cmake --preset gate-gcc`, and `build/gate-gcc/ds_runner` exists. `build/gate/ds_runner --outcomes | wc -l` is 7. `CHECK outcome-sync PASS`; `ls tests/gate/outcome-names` shows 2 cases. After `rm -f build/gate/ds_runner build/gate/outcome-names.txt` the stage run alone rebuilds through the build function and passes (observed once; it can never read an old binary because it always builds first). In `scripts/check` the `run_stage outcome-sync` line (1039) follows `run_stage build` (1038). `sh scripts/check` prints 29 `CHECK ... PASS` stage lines (30 `^CHECK .* PASS` lines with the RESULT line).

Mutation observations (each reverted, tree clean afterwards):

| Mutation | Case that went red |
|----------|--------------------|
| expect `core.behaviour_epoch=2` in a scratch TSV | record_skeleton_256k, "a scalar line of the record differs" |
| swap `byte_order` and `header_handling` in `text.c` | record_skeleton_256k, "a row is missing, extra or out of order" |
| drop the `bound_by` line from the printer | field_flip, "field media_slot: a change of its value, status or binder leaves the printed identity unchanged" |
| rename the `psg_tick_divisor` table in a scratch r1 TSV | profile_names_both_ways and profile_r1_equal |
| rename `lines_per_frame` in a scratch rules list | profile_names_both_ways, "not exactly once in the list of scripts/lib/profile-rules.awk" |
| delete the two `rule-compiled-names` lines in a scratch `outcome-sync` | `--selftest mutation-outcome-names` shows the control going broken (`EXPECT_MISMATCH ... got '<none>'`) |

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 29 `CHECK ... PASS` stage lines and `CHECK RESULT PASS` (28 before this plan, plus outcome-sync) |
| `dash scripts/check` | `CHECK RESULT PASS`, exit 0, 29 stage PASS lines |
| `sh scripts/check --selftest` | twelve `SELFTEST ... PASS` lines (eleven before, plus mutation-outcome-names), `SELFTEST RESULT PASS`, exit 0 |
| `sh scripts/audit-history --range 1dd4212..HEAD` | `audit-history: clean` (4 commits, 22 blobs) |
| `sh scripts/conformance-run run --preset gate-clang` and `--preset gate-gcc` | `STATUS conformance ok n=10` for both |

Second-compiler result, stated honestly: on this Mac `clang` is Apple clang 21 and `gcc` is also Apple clang 21 (`CMAKE_C_COMPILER_ID` is `AppleClang` in `build/gate-gcc`). The `gate-gcc` run is therefore NOT a real gcc check; a real gcc run belongs to plan 21's container run and CI. As an informal extra (not a gate, not committed), a Homebrew `arm-none-eabi-gcc` 16.2 compiled `src/text/text.c`, `src/core/profile.c`, `src/core/boot.c` with `-ffreestanding` and the runner and `synth.c` against the Apple SDK headers with `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror -fwrapv -fsyntax-only`, all exit 0. That shows no `-Wconversion` finding under a real gcc front end, which is not a substitute for the CI run.

## Files Created/Modified

- `src/text/ds_text.h`, `src/text/text.c` - the printer.
- `src/core/ds_core.h`, `src/core/profile.c` - `DS_BEHAVIOUR_EPOCH`, the unresolved-behaviour accessors.
- `CMakeLists.txt`, `CMakePresets.json` - `ds_text` target, `DS_BUILD_ID`, two presets.
- `tests/conformance/runner.c`, `REQUIRED`, `cases/` (four new) - four kinds, three flags, four cases.
- `scripts/conformance-run`, `scripts/outcome-sync`, `scripts/check`, `tests/gate/outcome-names/` - inputs for the profile kinds, `--names`, the stage, the controls, the selftest.

## Decisions Made

See `key-decisions`. The plan fixed the table list and key order, the error rule of the printer, the case ids and counts, the flag names (two of three) and the stage position; the choices left open were how an unresolved behaviour enters the constant, how the rules field list reaches the runner, how the three r1 gaps are handled, what one field_flip assertion means, and whether the outcome-sync stage trusts a previous build.

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 defect was found in earlier plans' code. The one defect the red cases found in the plan 05 constant (the unresolved behaviour ids were absent) is the defect the plan expected.

### Additions and interpretation choices (inside the plan's intent)

1. **`unresolved_behaviours` enters the constant through an accessor pair** (`ds_profile_unresolved_count`, `ds_profile_unresolved`), because the plan says the ids must be "present in the constant" and plan 05 left the decision open. `struct ds_profile` is unchanged (no pointer, no padding). Plan 13 ("make profile.c expose the sorted unresolved_behaviours ids") can use the same functions. The ids are not printed in the skeleton: the `[withheld]` table is plan 14's.
2. **`ds_text_profile` was added** (profile and field tables only). The plan names only `ds_text_record`, but `field_flip` needs "the profile and field tables of the record" for a modified copy of a profile, and printing a copy of a full 1 MiB instance 39 times would be wasteful. The record embeds the same text; a case assertion proves the text is an exact part of the record.
3. **`DS_BEHAVIOUR_EPOCH` (value 1) was added to `src/core/ds_core.h`**, which the plan does not list under files_modified: behaviour_epoch is a fact of the core, not of the printer.
4. **A third runner flag, `--rules-fields FILE`.** The plan lists two flags and wants the names checked "against the field list in scripts/lib/profile-rules.awk". Reading an awk script from C would be a hand-written parser of a script (against D-40 and D-51), so `conformance-run` cuts the list out with sed and hands the runner a line file; it exits 2 unless exactly 13 names result. The sed pattern is anchored on the `split("...", fl, " ")` line, so a change of shape fails closed.
5. **The three r1 gaps are asserted, not skipped.** The plan says the equality case skips allowed keys that r1 omits. The case instead asserts the core list there (empty for `vdc_count`, `vpc`, `input_port1`; `zero|seeded-prng-v1` for `powerup_policy`), which is stricter and is documented in the case comments. It also checks that each open proposal of r1 is one of the core's allowed values.
6. **`field_flip` flips three attributes per field** (value, status, binder) inside one assertion, so the count stays 13 while the binder is covered too. The `fields=13` token of its `expect` is checked against the profile's field count and a mismatch is a harness failure of the case, not a counted assertion.
7. **`record_skeleton_256k` also carries the D-69 controls** (quote, backslash, non-ASCII, unterminated field, unknown enumeration, short buffer), because the plan's must-have truth names the refusal and the plan lists no separate case for it.
8. **Stage `outcome-sync` calls `stage_build` itself** before asking the binary for its names. In a full run the build stage has just run, so this is an incremental no-op; run alone it can never read an old binary. The plan allowed "fails or rebuilds"; this takes the rebuild branch.
9. **Selftest `mutation-outcome-names`** was added (not in the plan): it deletes the two `rule-compiled-names` lines of a scratch `outcome-sync` and requires the `compiled-list-differs` control to break, matching the pattern of the other rule-bearing checks. This is why the selftest count is twelve.
10. **`CMakePresets.json` was rewritten through a JSON dump**, so `cmakeMinimumRequired` and the build presets are now multi-line; content is otherwise the plan 05 file plus the two presets.
11. **`scripts/conformance-run` was changed in Task 2, not Task 3.** `--preset` already existed from plan 05; the plan's Task 3 file list names it, but nothing in Task 3 needed to change.

**Total deviations:** 0 auto-fixed, 11 interpretation choices. **Impact:** none on later plans' contracts. Plan 14 builds on `ds_text_profile` and `ds_text_record`; plan 13 reuses the unresolved-behaviour accessors; plan 07 adds the content sha256 as the first key of `[content]`, which changes the skeleton from 50 to 51 rows, so `n_exp` and the key list in `build_expected_keys` (runner.c) must change in the same commit.

## Known Stubs

None added by this plan. The earlier stub (`ds_instance.content_id`, plan 05) is unchanged and still tracked in `.planning/WINDOWS.md`; the record skeleton does not print it yet (plan 07).

## Notes for later plans

- **Skeleton row count is a pinned number.** `record_skeleton_256k` asserts exactly 50 rows from `build_expected_keys` in `tests/conformance/runner.c`. A plan that adds a key or a table (07: `content.sha256`; 12: `power_on`; 14: `profile.sha256`, `withheld`, `machine`) must edit that list and the `n_exp == 50u` assertion, and the case `expect` if a scalar line changes. Plan 14 replaces the skeleton check with its golden.
- **Printer API as built.** `enum ds_text_status ds_text_record(const struct ds_instance *, char *buf, size_t cap, size_t *outlen)` and `ds_text_profile(const struct ds_profile *, ...)`; statuses `DS_TEXT_OK, _ERR_ARGUMENT, _ERR_CAPACITY, _ERR_STRING, _ERR_STATE`. Text is NUL-terminated inside `cap` (needs len + 1). `DS_TEXT_FORMAT_VERSION` is 1.
- **Request grammar of the new kinds.** `record_lines`: fixture and request as for `accept` (the boot must be accepted, else harness failure), `expect` is `TABLE.KEY=VALUE` items separated by `;`. `profile_equal`, `names_both_ways`: fixture `none`, need `--profile-tsv` (and `--rules-fields` for the names). `field_flip`: fixture `none`, request must resolve, `expect = "changed;fields=13"`.
- **Presets.** Add further presets the same way: inherit `gate-base`, set `DS_EXTRA_FLAGS`, own `binaryDir`, same-named build preset. The sanitize preset must not carry `-fwrapv`.
- **Case file line limit.** `toml-subset.awk` rejects a line over 400 characters (`LINE_TOO_LONG`); long explanations go in comment lines.
- **Real gcc is still unrun.** The compiler-disagreement risk (`-Wconversion` opposite directions) is covered here only by the informal arm-none-eabi-gcc syntax check above.

## Threat Flags

None. The new surface is a local printer of values the core already holds, a local build, and sh and awk gate scripts reading repository files. The threat register mitigation for this plan is implemented: T-02-48 (profile_r1_equal both ways, profile_names_both_ways, field_flip with 13 assertions, each reproduced red by a scratch mutation). T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist: `src/text/ds_text.h`, `src/text/text.c`, `tests/conformance/cases/record_skeleton_256k.toml`, `tests/conformance/cases/profile_r1_equal.toml`, `tests/conformance/cases/profile_names_both_ways.toml`, `tests/conformance/cases/field_flip.toml`, `tests/gate/outcome-names/ok-same/names.txt`, `tests/gate/outcome-names/compiled-list-differs/EXPECT` (checked with `[ -f ]`).
- Commits `cdbaaf6`, `83a8a3d`, `c52ef41`, `fdcc748` are ancestors of HEAD.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
