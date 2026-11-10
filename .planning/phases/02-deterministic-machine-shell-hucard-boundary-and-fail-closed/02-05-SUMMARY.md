---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 05
subsystem: core
tags: [c11, cmake, presets, x-macro, profile, ds_boot, resolve, conformance-runner, tsv, gate-stage]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: run_controls and --stage NAME in scripts/check (02-01); outcomes.def, conformance-flatten and conformance-ingest (02-02); the supervise mode of conformance-run (02-03); licence-check scope for src and tests (02-04)
provides:
  - CMakeLists.txt and CMakePresets.json (preset gate, a same-named build preset, hidden base) with targets ds_core and ds_runner; no install rule, no public header
  - src/core: profile_fields.def (13 fields), refusals.def (20 refusals), boot_order.def (14 ordered checks), ds_core.h, profile.c (r1 constant and ds_resolve), boot.c (ds_boot, ds_instance_size, ds_instance_align)
  - tests/conformance/runner.c (one-process runner, TSV reader, kinds refuse, accept, resolve, two-pass dispatch), synth.c and synth.h (plain-256k recipe), six cases, REQUIRED
  - scripts/conformance-run run mode and the gate stages build and conformance (28 stages in all)
affects: [02-06 (record printer, r1 equality kinds, gate-gcc and gate-clang presets, outcome-sync stage), 02-07 (content id into the content_id slot), 02-08 (supported sizes, bank tags in synth.c), 02-09, 02-10 (sanitize preset), 02-11 (refusal rows and the content-copy RULE anchor), 02-12 (power-on checks into boot_order.def)]

actuals:
  tokens: 18250    # chars/4 over the realized diff (git diff 0433300..2d4b586 excluding .planning, 73003 chars, 20 files, 1870 added lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 04333007c8ebef1493b505a1c6cd159ad6767b30
plan_head_after: 2d4b586bca974477cd650495312274630d32181f
commits: 4

tech-stack:
  added: []
  patterns:
    - "one X-macro file per table (profile_fields.def, refusals.def, boot_order.def, outcomes.def); struct, enum, name tables, the builtin constant and the check list are all generated from it"
    - "compiler flags reach the build through two preset cache variables (DS_WARNING_FLAGS in the hidden base, DS_EXTRA_FLAGS per preset) so a preset adds to the warning set instead of replacing CMAKE_C_FLAGS"
    - "the runner prints RESULT lines only and decides nothing; the sh run mode flattens, builds, supervises and hands the file to the ingest"
    - "a refusal is proved non-mutating by a canary: the case buffer is filled with 0xA5 and compared byte for byte after every refusal"
    - "every boot_order row guards its own use of the request, so a row stays safe wherever a later plan moves it"

key-files:
  created:
    - CMakeLists.txt
    - CMakePresets.json
    - src/core/ds_core.h
    - src/core/profile.c
    - src/core/boot.c
    - src/core/profile_fields.def
    - src/core/refusals.def
    - src/core/boot_order.def
    - tests/conformance/runner.c
    - tests/conformance/synth.c
    - tests/conformance/synth.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/ (ref_profile_unknown, accept_tracer_256k, resolve_open_unbound, resolve_bind_closed, resolve_bind_not_allowed, resolve_rev_mismatch)
  modified:
    - scripts/check
    - scripts/conformance-run

key-decisions:
  - "ds_boot returns the refusal number and also writes it to the result struct when the result pointer is not NULL; a NULL result suppresses reporting only and is not a check of its own, so the precedence stays exactly the boot_order list"
  - "resolve runs once per boot, on first use, and the profile, rev and bindings rows surface their own codes from it; byte_order and header return a resolve refusal if they run before the bindings row, so moving rows never reads an unresolved profile"
  - "an expect literal load.NAME that no refusal carries can never match and is a guest_failure (a wrong literal turns the case red); only a token that is neither accept nor load.* is a harness fault"
  - "the field= token of an expectation is optional: when absent the field index is not asserted, which keeps the plan 11 refusal rows (plain expect load.NAME) valid even where the core names a field"
  - "a NULL profile id is profile_unknown, not request_invalid; request_invalid is NULL request, struct_size and reserved only"
  - "resolve walks the fields in table order and the first fault wins (closed binding, unbound open field, value outside allowed)"

patterns-established:
  - "case schema: ten keys, root-level, fixture is a synth recipe (plain-256k) or none for resolve cases; request is a semicolon-separated override string; expect is accept[;size=N] or load.NAME[;field=NAME]"
  - "TSV reader refuses a file without its END row, with an END count different from the number of cases, with no case, or with a row after END (exit 2, no RESULT line)"
  - "run mode removes every file it produces first, so a failing step cannot leave an earlier good file to be judged"

requirements-completed: [PROF-02, CORE-01]

coverage:
  - id: D1
    description: "A host passes profile id, rev, bindings, declared mapper, byte order and header and bytes to ds_boot and gets a typed refusal or an instance in caller memory that records the resolved profile; ds_boot never allocates and a refusal writes only the result struct"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance -> CHECK conformance PASS (accept_tracer_256k 10 assertions incl. instance is the caller memory, profile recorded, bound_by, canary past the instance; ref_profile_unknown 5 assertions incl. canary over the whole buffer intact after the refusal)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Open profile fields are never defaulted: an unbound open field, a binding of an owner-selected field, a value outside allowed and a rev other than 1 each give a typed refusal that names the field"
    requirement: PROF-02
    verification:
      - kind: integration
        ref: "resolve_open_unbound, resolve_bind_closed, resolve_bind_not_allowed, resolve_rev_mismatch in tests/conformance/cases; red against the tracer core in commit b0e7179 (four guest_failure), green in 2d4b586"
        status: pass
    human_judgment: false
  - id: D3
    description: "The thin end-to-end path is real: the preset builds the freestanding core, the single-process runner emits RESULT lines from TOML cases, and the sh gate judges them through conformance-ingest; a wrong literal is guest_failure"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "sh scripts/check, dash scripts/check -> CHECK RESULT PASS with 28 stages; one-character change of an expected literal made the conformance stage FAIL with RESULT ref_profile_unknown guest_failure (and again for a changed reason in resolve_bind_closed), reverted"
        status: pass
    human_judgment: false
  - id: D4
    description: "Check order is data (boot_order.def) so a later plan can mutate it and require a red row"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "47 scratch vector rows (D-46 vectors, precedence pairs, field names) all pass against the boot_order core; the row order of boot_order.def is memory, request, profile, rev, bindings, mapper, byte_order, header, buffer_null, empty, cap, short, bank_multiple, supported_size"
        status: pass
    human_judgment: false

duration: 14 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 05: Freestanding core skeleton, typed profile resolve and the runner path Summary

**A CMake `gate` preset builds the freestanding `ds_core` (fixed 1 MiB ROM array in caller memory, no allocator, 13-field r1 profile from an X-macro, 20 typed refusals, 14 ordered checks in `boot_order.def`) and a one-process TOML-driven runner whose RESULT lines `conformance-run run` judges through `conformance-ingest`; open fields never default and every resolve refusal names its field.**

## Performance

- **Duration:** 14 min (2026-11-10T04:25:43Z to about 04:40Z)
- **Tasks:** 3 of 3 (Task 1 is the tracer, Task 3 is TDD)
- **Files:** 20 changed outside .planning (1870 added lines, 4 removed)
- **Gate cost (this machine):** whole `sh scripts/check` about 35 s wall, the build stage reuses an incremental build tree, so a cold configure and compile is not in this figure. Time is a record, never a failure (D-60).

## Accomplishments

- **Build (D-40, D-41).** `CMakeLists.txt` defines `ds_core` (glob of `src/core/*.c` with CONFIGURE_DEPENDS, PUBLIC include directory `src/core`) and `ds_runner` (links `ds_core`); no `find_package`, no `FetchContent`, no `install(`, no public header. `CMakePresets.json` is version 6: hidden `gate-base` (Ninja, `DS_WARNING_FLAGS = -std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`, Debug), `gate` (adds `DS_EXTRA_FLAGS = -fwrapv`, binaryDir `build/gate`) and a build preset also named `gate`. The base carries no `-fwrapv`, so the sanitize preset of plan 10 can inherit it (D-83).
- **Profile (D-48, D-49).** `profile_fields.def` lists the 13 fields in r1.toml order with kind (OWNER or OPEN), the constant and the allowed set; the X-macro generates `struct ds_profile`, `enum ds_field_index`, the name and allowed tables and the r1 constant (a `_Static_assert` states that the profile has no padding). The seven open fields start OPEN with an empty value. `powerup_policy` gets `zero|seeded-prng-v1` as a core constant (the frozen r1 has no allowed list for it).
- **Resolve.** `ds_resolve(id, rev, bindings, out, field_index)` returns `profile_unknown`, `rev_mismatch`, `bind_closed_field`, `open_unbound`, `bind_not_allowed` or OK. A refusal writes only `field_index`; a bound open field records `bound_by = run-request`.
- **Boot (D-42, D-45, D-46, D-66, D-67).** `ds_boot(mem, cap, request, result)` iterates `boot_order.def`. The memory check comes first and tests the address as an integer (`uintptr_t` modulo) before any cast. After the checks the whole instance is zeroed, the profile recorded and the ROM copied into the embedded array. The content copy is one source line (see Notes). `byte_order` and `header` compare the declaration with the resolved binding through the D-67 vocabulary map (`pce`/`tg16`, `refuse` to `none`, `declared-512` to `copier_512`): a difference is `profile_mismatch` naming `content_byte_order` or `content_copier_header`; `tg16` and `copier_512` are then refused as unsupported. `supported_size` is exactly 256 KiB until plan 08.
- **Runner.** `tests/conformance/runner.c` is ISO C, one process. It reads the flattened TSV, requires the END row (a missing row, a count that differs from the number of cases, an empty suite or a row after END is exit 2 with no RESULT line), runs the cases in two passes (ordinary kinds first in TSV order; deferred kinds second, none exist yet) and prints only `RESULT` lines and `RUNNER_DONE n`, flushing after each line. Kinds: `refuse`, `accept`, `resolve`. The request grammar of the plan is implemented in full (`profile, rev, mapper, order, header, len, ptr, mem, cap, req, struct_size, reserved, bind.NAME`). A refusal is checked for non-mutation with a canary buffer, an accept for the copy, the recorded profile, `bound_by` of every field, machine time 0 and no write past the instance. Evidence files hold the id, kind, expectation, observed reason name and assertion count, never content bytes.
- **Gate.** `scripts/conformance-run run [--preset NAME]` flattens the case inventory, derives `ids.txt`, configures and builds the preset (always, incremental, so a stale binary is never judged), runs the runner under the supervise mode, prints the RESULT lines and either the ingest violations or `STATUS conformance ok n=N`. Stages `build` and `conformance` follow the static control stages (28 stages).

## Task Commits

1. **Task 1 (tracer): preset builds the freestanding core and a built-in runner path boots one refusal and one accept** - `6417d5f` (feat)
2. **Task 2: TOML-driven runner, conformance-run run mode, first two cases judged by the gate** - `151f5e2` (feat)
3. **Task 3 RED: resolve kind and four resolve cases, red against the tracer core** - `b0e7179` (test)
4. **Task 3 GREEN: complete typed resolve and the full boot precedence** - `2d4b586` (feat)

## Verification Results

Tracer gate (auto mode): after the Task 1 commit the tracer verify was re-run (`sh scripts/check --stage build && build/gate/ds_runner --tracer | awk ...`, exit 0, two pass lines with 5 and 9 assertions); log line `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage build && build/gate/ds_runner --tracer \| awk '/^RESULT .* pass assertions=[1-9]/ { n++ } END { exit (n == 2) ? 0 : 1 }'` (Task 1) | exit 0; `CHECK build PASS`; `RESULT ref_profile_unknown pass assertions=5`, `RESULT accept_tracer_256k pass assertions=9`, `RUNNER_DONE 2` |
| `sh scripts/check --stage build && sh scripts/check --stage conformance` (Task 2) | exit 0; `CHECK build PASS`, `CHECK conformance PASS`; results.txt is two pass lines and `RUNNER_DONE 2` and nothing else |
| `sh scripts/check --stage conformance` (Task 3, RED) | exit 1; four `guest_failure` (resolve_open_unbound, resolve_bind_closed, resolve_bind_not_allowed, resolve_rev_mismatch), two pass, `RUNNER_DONE 6` |
| `sh scripts/check --stage conformance` (Task 3, GREEN) | exit 0; six pass lines, `RUNNER_DONE 6` |

Acceptance criteria, all run:

- Task 1: `ds_runner --tracer` printed `RUNNER_DONE 2` and both pass lines with non-zero counts. `ls src/core/*.c src/core/*.h` lists boot.c, ds_core.h, profile.c; the stdlib/stdio/string/time/errno/unistd include grep over `src/core` exits 1 (nothing found). No file named `duospool.h` exists; `grep -c 'install(' CMakeLists.txt` prints 0. `grep -n '__builtin_memcpy' src/core/boot.c | grep -c content` prints 1. `cmake --list-presets=build` lists `gate`; `cmake --build --preset gate` exits 0. `sh scripts/check --stage licences` prints PASS and the six new src and runner files carry `MIT OR Apache-2.0` in their first five lines.
- Task 2: results.txt holds `RESULT ref_profile_unknown pass assertions=5`, `RESULT accept_tracer_256k pass assertions=9` (10 after Task 3 added the bound_by assertion), ends with `RUNNER_DONE 2`, and no line that is not RESULT or RUNNER_DONE (counted: 0). Wrong-literal observation, below. Both case files hold all ten keys at the start of a line (checked key by key). SPDX: synth.c, synth.h and conformance-run carry `MIT OR Apache-2.0`; each case file starts with the 0BSD header.
- Task 3: results.txt shows six pass lines and `RUNNER_DONE 6`. `src/core/boot_order.def` exists with the SPDX header; `grep -c 'boot_order.def' src/core/boot.c` prints 3 (baseline: the file was absent at the end of Task 2). The DS_CHECK rows are at lines 13 to 26 in the order memory, request, profile, rev, bindings, mapper, byte_order, header, buffer_null, empty, cap, short, bank_multiple, supported_size. The awk over `refusals.def` prints 20 rows numbered 1 to 20. Changing the expected reason of `resolve_bind_closed` to `load.bind_not_allowed` (run once, then reverted) made that id `guest_failure` and the stage FAIL.

Wrong-literal observation (plan 05 task 2, recorded as asked). With `expect = "load.profile_unknowm"` (one character changed) in `ref_profile_unknown.toml`:

```
CHECK conformance FAIL
    case ref_profile_unknown: refusal differs from the expected one
    RESULT accept_tracer_256k pass assertions=9 evidence=build/gate/evidence/accept_tracer_256k.txt
    RESULT ref_profile_unknown guest_failure assertions=5 evidence=build/gate/evidence/ref_profile_unknown.txt
    RUNNER_DONE 2
    OUTCOME_GUEST_FAILURE	class=guest_failure	id ref_profile_unknown
```

exit 1; the file was restored byte for byte (diffed against a copy) and the stage is green again.

Extra checks outside the plan: a scratch suite of 47 rows (never committed) covering the D-46 vectors (empty, buffer_null, 1/513/8191, 8192, 256K-1, 256K+512 with and without copier_512, 128K, 512K, 2M, 1M+8K, size_max with a dummy pointer, 2.5 MiB plain and sf2, undeclared mapper, order and header, tg16 and copier_512 bound and unbound, rev, unknown profile, the three resolve faults, the field names, seven pairs of competing faults for the precedence, accept with both powerup policies) all passed, and the runner exits 2 without output for a TSV with no END row, a TSV with only END, a truncated TSV and no arguments.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 28 `CHECK ... PASS` stage lines and `CHECK RESULT PASS` (26 before this plan, plus build and conformance); about 35 s wall |
| `dash scripts/check` | `CHECK RESULT PASS`, exit 0 |
| `sh scripts/check --selftest` | eleven `SELFTEST ... PASS` lines, `SELFTEST RESULT PASS`, exit 0 |
| `sh scripts/audit-history --range 0433300..HEAD` | `audit-history: clean` (4 commits, 27 blobs) |

## Files Created/Modified

- `CMakeLists.txt`, `CMakePresets.json` - targets and presets.
- `src/core/ds_core.h`, `profile.c`, `boot.c`, `profile_fields.def`, `refusals.def`, `boot_order.def` - the core.
- `tests/conformance/runner.c`, `synth.c`, `synth.h`, `REQUIRED`, `cases/*.toml` (six) - runner, recipe, inventory.
- `scripts/conformance-run` (run mode), `scripts/check` (stages build and conformance).

## Decisions Made

See `key-decisions`. The plan fixed the enum numbers, the check order, the case schema, the CLI and the run mode; the choices left open were the shape of the result and return value, how the three resolve-level rows share one resolve, whether a mistyped literal is a harness fault or a red case, the field token of an expectation, and how the preset passes flags.

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 fix was needed beyond what is listed below as interpretation.

### Additions and interpretation choices (inside the plan's intent)

1. **Flags through two cache variables.** The research draft passed `CMAKE_C_FLAGS` from the preset; a child preset that sets the same variable replaces the parent's value, so `gate` could not "add `-fwrapv`" to the base. The presets set `DS_WARNING_FLAGS` (base) and `DS_EXTRA_FLAGS` (per preset) and `CMakeLists.txt` applies both with `add_compile_options`. The effective flags are the ones the plan names. Plan 06 and plan 10 add their presets the same way (`DS_EXTRA_FLAGS` for `-fwrapv`, the sanitizer flags).
2. **`tests/conformance/synth.h`** was added (the plan lists only synth.c): the runner needs a prototype for `synth_size` and `synth_fill`, and a header is checked for the SPDX line by `licence-check`. The recipe API is `synth_size(recipe)` (natural size) and `synth_fill(recipe, buf, n)` (the first n bytes of the image, which continues past its natural size so an over-long request has real bytes to point at, capped at 4 MiB in the runner).
3. **Wrong literal is `guest_failure`.** The plan wants a one-character change of an expected literal to show `guest_failure`. A literal such as `load.profile_unknowm` names no refusal, so the runner treats it as an expectation that can never match (a red case) rather than as a harness fault. A token that is neither `accept` nor `load.*` is still a harness fault.
4. **`accept` also asserts `bound_by`** (every open field run-request, every owner field constant), which covers the plan's behaviour line "each resolved open field records bound_by run-request" without a fifth REQUIRED case (the plan fixes six).
5. **`--profile-tsv FILE`** is accepted by the runner and not read (plan 06 uses it); stated in the usage comment.
6. **The tracer's resolve was deliberately lax** (id only, bound values copied without checks) so that the four resolve cases written first in Task 3 are red for the right reason, as the plan's task order implies. The tracer commit therefore accepts an unbound open field; Task 3 closes it.
7. **Each row of `boot_order.def` guards its own use of the request** (`req_missing`), and `byte_order` and `header` return the resolve refusal when they run with a failed resolve, so a later plan that swaps rows gets a wrong refusal and never a NULL read.

**Total deviations:** 0 auto-fixed, 7 interpretation choices. **Impact:** none on later plans' contracts, except that later preset and runner-kind plans should reuse `DS_EXTRA_FLAGS` and the `synth_*` API.

## Known Stubs

- `struct ds_instance.content_id` (`src/core/ds_core.h`, line 100) is a zeroed slot: plan 02-07 adds SHA-256 content identity and fills it. Nothing reads it before then. Recorded in `.planning/WINDOWS.md` (entry 2, kind stub).
- `machine_time` stays 0: `ds_advance` arrives with the placeholder stepping plan (D-68).

## Notes for later plans

- **Plan 11 content-copy RULE anchor (D-85, D-86).** The content copy is the single line 283 of `src/core/boot.c`; the literal anchor substring is `__builtin_memcpy(inst->rom, request->content, request->len);`. `grep -n '__builtin_memcpy' src/core/boot.c | grep -c content` prints 1.
- **Request and expectation grammar as built.** Request keys: `profile, rev, mapper (- undeclared), order (- or pce or tg16), header (- or none or copier_512), len (N or size_max), ptr (ok or null or dummy), mem (ok or null or misaligned), cap (ok or short), req (ok or null), struct_size (ok or bad), reserved (0 or 1), bind.NAME (value, or - to unbind)`. A `ptr=ok` request with `len` above 4 MiB is a harness fault (use `ptr=dummy`). Expect: `accept[;size=N]` or `load.NAME[;field=NAME]`; `field=` is asserted only when present. Resolve cases use `fixture = "none"`.
- **Default bindings** are in the runner: `io_1000_d6_value 1, io_1000_d7_value 1, lines_per_frame 262, psg_tick_divisor 6, content_byte_order pce, content_copier_header refuse, powerup_policy zero`. Plan 12 adds the `powerup` and `seed` keys.
- **Evidence refs** are `DIR/ID.txt` with DIR as given on the command line; an absolute `--evidence-dir` produces a RESULT line the ingest rejects (`ING_BAD_LINE`), so `conformance-run run` always passes a relative `build/PRESET/evidence`.
- **`unresolved_behaviours`** of r1.toml (LED-IRQ-0001, LED-VID-0002, LED-VID-0003) is not carried by the core profile yet: plan 06 (profile equality, printer) decides whether it enters the identity or is explicitly excluded.
- **Only macOS Apple clang 21 was exercised** (`gate` builds with `cc`); gcc and Linux are plan 06 and CI. Strict `-std=c11 -pedantic -Werror` already rejects POSIX-only calls in the runner on this compiler; gcc's `-Wconversion` is the other direction and is plan 06's second-compiler check.
- The 1 MiB instance is allocated per case in the runner (malloc in test code); the core never allocates. A refusal case compares the whole instance buffer plus a 64-byte tail, about 1 MiB per refusal row; the 47-row scratch suite ran in well under a second.

## Threat Flags

None. The new surface is a local build and a local test runner reading a TSV that a local script produced; the core takes caller memory, a struct and bytes it does not read before validation. The threat register mitigations for this plan are implemented: T-02-07 (memory_invalid first, integer alignment test and cap compare before any cast or write), T-02-08 (bind_closed_field, bind_not_allowed, open_unbound, with cases), T-02-09 (a refusal writes only the result struct, checked by the canary), T-02-10 (runner requires the END row and exits 2 otherwise).

## Self-Check: PASSED

- Files exist: CMakeLists.txt, CMakePresets.json, src/core/ds_core.h, src/core/profile.c, src/core/boot.c, src/core/profile_fields.def, src/core/refusals.def, src/core/boot_order.def, tests/conformance/runner.c, tests/conformance/synth.c, tests/conformance/REQUIRED, scripts/conformance-run (checked with `[ -f ]`).
- Commits `6417d5f`, `151f5e2`, `b0e7179`, `2d4b586` are ancestors of HEAD.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
