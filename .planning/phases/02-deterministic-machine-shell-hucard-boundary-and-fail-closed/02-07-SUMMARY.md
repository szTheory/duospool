---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 07
subsystem: core
tags: [c11, sha256, content-identity, fips-180-4, nist-vectors, hash-sweep, synthetic-fixtures, conformance-gate]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: ds_boot with the fixed ROM copy and the zeroed content_id slot (02-05); the ds_text record printer, the runner and the case kinds (02-06); conformance-run run mode and the gate presets (02-05, 02-06)
provides:
  - src/core/ds_sha256.h and sha256.c, in-house FIPS 180-4 SHA-256 (init, update, final, one-shot), unsigned arithmetic only
  - ds_boot stores the SHA-256 of the copied ROM bytes as the content id, after every validation check has passed
  - content.sha256 as the first key of the [content] table of the record (skeleton is now 51 rows)
  - synthetic recipe len:N beside plain-256k, the generator spec string (synth_spec), runner flags --emit, --hash, --sweep-lengths, --spec
  - the hash sweep in scripts/conformance-run (core digest against sha256sum or shasum -a 256 for 21 lengths, code HASH_SWEEP_MISMATCH)
  - case kinds content_id and hash_literal; cases content_id_vs_shasum, content_id_nist_empty, _abc, _448bit, _million_a and record_content_fields (all in REQUIRED; 16 cases in all)
  - tests/conformance/runner.h and profkinds.c (the profile kinds moved out of runner.c)
affects: [02-08 (supported sizes and bank tag bytes change plain-256k, not len:N), 02-12 (power_on table), 02-13 (profile hash can reuse ds_sha256), 02-14 (record golden: content.sha256 is already the first content key), 02-17 (fixture manifest takes the 21 lengths from --sweep-lengths and the spec string from --spec), 02-10 (sanitize preset: sha256.c is already unsigned-only)]

actuals:
  tokens: 20423    # chars/4 over the realized diff (git diff 4c1507a..HEAD excluding .planning, 81692 chars, 20 files, 1134 added and 421 removed lines; about 16 KB of the added lines are a move of the profile kinds)
  tasks: 2
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 4c1507a5c805a31c87e727125eb163a6a359e38b
plan_head_after: 360cab9d0b21894d245849ffb9c7f92940ed3056
commits: 3

tech-stack:
  added: []
  patterns:
    - "the content id is computed from the copy inside the instance, not from the caller's buffer, so the hash and the bytes the machine will run can never differ"
    - "the independent check lives in the gate script: the same image bytes go through the runner's --emit to a host tool that shares no code with the core, and the 64-hex field is compared, not the whole host line"
    - "one definition of the sweep list (a static array in runner.c printed by --sweep-lengths); the gate and the later fixture manifest read the output"
    - "a self-consistency case says so in its own header comment and in its meaning; the independent oracle is a different mechanism (the sweep)"
    - "pinned literals use the pure LCG recipe len:N, which no later plan changes, never a recipe that a later plan will extend"

key-files:
  created:
    - src/core/ds_sha256.h
    - src/core/sha256.c
    - tests/conformance/runner.h
    - tests/conformance/profkinds.c
    - tests/conformance/cases/content_id_vs_shasum.toml
    - tests/conformance/cases/content_id_nist_empty.toml
    - tests/conformance/cases/content_id_nist_abc.toml
    - tests/conformance/cases/content_id_nist_448bit.toml
    - tests/conformance/cases/content_id_nist_million_a.toml
    - tests/conformance/cases/record_content_fields.toml
  modified:
    - src/core/boot.c
    - src/text/text.c
    - src/text/ds_text.h
    - tests/conformance/synth.c
    - tests/conformance/synth.h
    - tests/conformance/runner.c
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/REQUIRED
    - scripts/conformance-run
    - CMakeLists.txt

key-decisions:
  - "the content id is the SHA-256 of inst->rom after the copy, taken as the last step of ds_boot, so a refused boot never hashes anything and the core never inspects what the bytes mean"
  - "the sweep lives inside conformance-run run mode (function hash_sweep after the ingest) and therefore also runs for the gate-gcc and gate-clang presets; a missing host tool or an unusable answer is a harness error (exit 2), a difference is exit 1"
  - "record_content_fields pins the sha256 literal of len:262144, not of plain-256k, because plan 08 adds bank tag bytes to plain-256k and the pinned literal must not need an edit then"
  - "profile kinds (profile_equal, names_both_ways, field_flip) moved to profkinds.c behind runner.h so that tests/conformance/runner.c stays under the 65536-byte limit of the acceptance criterion without cutting any check"
  - "kind names: content_id (boot, recompute over a fresh copy, record line) and hash_literal (input built in the runner, request names the input, expect is the NIST literal)"

patterns-established:
  - "hash_literal request grammar: fixture is none, request is one of empty, abc, 448bit, million-a, expect is 64 lowercase hex characters (anything else is a harness failure, never a pass)"
  - "a recipe is the pair (name, SYNTH_SPEC string); len:N accepts decimal without a leading zero up to 16777216, anything else is an unknown recipe"
  - "runner flags that print and exit (--outcomes, --sweep-lengths, --spec, --hash, --emit) are handled before the case arguments; --emit writes only to stdout"

requirements-completed: [CONT-04]

coverage:
  - id: D1
    description: "In-house FIPS 180-4 SHA-256 equals the four NIST example values (empty, abc, the 448-bit message, one million a fed in uneven 1 to 7 byte chunks and in one call)"
    requirement: CONT-04
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance -> CHECK conformance PASS; RESULT content_id_nist_empty, content_id_nist_abc, content_id_nist_448bit, content_id_nist_million_a each pass assertions=4; editing one hex digit of the abc literal made it guest_failure, reverted"
        status: pass
    human_judgment: false
  - id: D2
    description: "The core digest equals the host sha tool for all 21 lengths including the padding boundaries 55, 56, 57, 63, 64, 65, 119, 120, 121 and the three supported image sizes"
    requirement: CONT-04
    verification:
      - kind: integration
        ref: "sh scripts/conformance-run run -> STATUS hash-sweep ok lengths=21; build/gate/ds_runner --sweep-lengths | wc -l is 21; a scratch mutant that counted the length in bytes gave HASH_SWEEP_MISMATCH at lengths 1, 54, 55, 56, 57, 63, 64, 65 and more, reverted; the same sweep is green for gate-gcc and gate-clang"
        status: pass
    human_judgment: false
  - id: D3
    description: "ds_boot stores the content id over the supplied bytes and the record prints it as the first key of the content table with size, mapper plain, mapper_rule plain-v1, byte order pce, header handling none and the cap 1048576, each as an exact line; no ROM-internal string is recorded"
    requirement: CONT-04
    verification:
      - kind: integration
        ref: "RESULT content_id_vs_shasum pass assertions=7; RESULT record_content_fields pass assertions=70; RESULT record_skeleton_256k pass assertions=73 (51 rows, content.sha256 first); RUNNER_DONE 16"
        status: pass
    human_judgment: false
  - id: D4
    description: "The core never opens a file and the generator leaves no ROM-like file in the tree: images exist only in memory and on the stdout of --emit"
    requirement: CONT-04
    verification:
      - kind: integration
        ref: "git ls-files lists no *.bin or *.pce and no file under src or tests above 65536 bytes (largest tests/conformance/runner.c at 55130 bytes); sh scripts/check --stage rights-tree and audit-synthetic PASS in the full gate run"
        status: pass
    human_judgment: false

duration: 10 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 07: Content identity with in-house SHA-256 Summary

**An in-house FIPS 180-4 SHA-256 is the content id of every boot, printed as the first key of the record's content table, and a gate sweep proves it equals the host `sha256sum` or `shasum -a 256` at 21 lengths around the padding boundaries; the four NIST vectors are pinned as spec literals.**

## Performance

- **Duration:** about 10 min (2026-11-10T04:58Z to 05:08Z)
- **Tasks:** 2 of 2 (Task 1 is the tracer, Task 2 is TDD)
- **Files:** 20 changed outside .planning (1134 added lines, 421 removed; most removals are the move of the profile kinds out of runner.c)

## Accomplishments

- **SHA-256 (D-47, D-83).** `src/core/sha256.c` implements init, update (any chunk size, internal 64-byte buffer), final (0x80, zeros to 56 mod 64, 64-bit big-endian length in bits) and a one-shot helper. Every intended wrap is `uint32_t` or `uint64_t` arithmetic: bytes are widened before the word-load shifts, rotates use counts 2 to 25. Only `__builtin_memcpy` and `__builtin_memset`, and includes limited to stddef and stdint. No code was copied, so there is no PROVENANCE entry.
- **Boot and record.** `ds_boot` hashes the copied ROM bytes as its last step, after every check has passed, and stores the digest in `content_id`. `ds_text` prints `sha256 = "<64 lowercase hex>"` as the first key of `[content]` (new `kv_hex`). The skeleton is 51 rows; `build_expected_keys`, the pinned count and the case meaning were edited in the same commit as plan 06 asked.
- **Recipes and flags.** `len:N` gives N bytes of the same LCG as `plain-256k` (seed 12345); the generator spec string is exposed by `synth_spec()` and `--spec`. `--sweep-lengths` prints the 21 lengths (the only definition), `--emit RECIPE` writes the bytes to stdout, `--hash RECIPE` prints the core's SHA-256 of a recipe.
- **Hash sweep (D-59).** After the ingest, `conformance-run run` calls `hash_sweep`: for each sweep length the runner's `--emit len:N` goes through `sha256sum` (else `shasum -a 256`, chosen once) and the first 64-hex field is compared with `--hash len:N`. A difference prints `HASH_SWEEP_MISMATCH length=N core=... host=...` and exits 1; on success `STATUS hash-sweep ok lengths=21`.
- **Cases.** `content_id_vs_shasum` (kind `content_id`: stored id, digest of a fresh copy of the recipe bytes, the record line, 64 lowercase hex, non-zero, and a flipped bit changes the digest). Four `content_id_nist_*` cases (kind `hash_literal`: input built in the runner, fed in uneven chunks of 1 to 7 bytes and in one call). `record_content_fields` (kind `record_lines` over `len:262144`, seven exact lines including the sha256 literal taken from the host tool).
- **Split of the runner.** `runner.c` crossed 65536 bytes (69449) with this plan, which the acceptance criterion forbids, so the three profile kinds and their TSV loaders (16 KB) moved to `tests/conformance/profkinds.c`, with the shared structures in `tests/conformance/runner.h`. No check was removed; `runner.c` is now 55130 bytes.

## Task Commits

1. **Task 1 (tracer): in-house SHA-256 content id, recorded and checked against the host hash tool** - `0019efe` (feat)
2. **Task 2 RED: NIST vectors and record content fields, red until the hash_literal kind exists** - `c59356d` (test)
3. **Task 2 GREEN: hash_literal kind checks the in-house SHA-256 against the NIST vectors** - `360cab9` (feat)

## Verification Results

Tracer gate (auto mode): after `0019efe` the tracer verify was re-run (`sh scripts/check --stage conformance`, exit 0, `RESULT content_id_vs_shasum pass assertions=7`, `RUNNER_DONE 11`); log line `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance` (Task 1) | exit 0, `CHECK conformance PASS`; `sh scripts/conformance-run run` shows `RESULT content_id_vs_shasum pass assertions=7`, `RUNNER_DONE 11`, `STATUS hash-sweep ok lengths=21`, `STATUS conformance ok n=11`; no HASH_SWEEP_MISMATCH |
| `sh scripts/check --stage conformance` (Task 2 RED, commit c59356d) | exit 1; 16 RESULT lines, four `harness_failure` (content_id_nist_*: unknown kind), `record_content_fields pass assertions=70`, `RUNNER_DONE 16` |
| `sh scripts/check --stage conformance` (Task 2 GREEN) | exit 0, `CHECK conformance PASS`; four nist cases `pass assertions=4`, `RUNNER_DONE 16`, `STATUS conformance ok n=16` |

Acceptance criteria, all run:

- Task 1: results show `RESULT content_id_vs_shasum pass` and `RUNNER_DONE 11` (PASS). `build/gate/ds_runner --sweep-lengths | wc -l` is 21 (PASS). Byte-length mutant (`s->bits += (uint64_t)len;` instead of `* 8u`, scratch, observed once, reverted): `HASH_SWEEP_MISMATCH` printed at lengths 1, 54, 55, 56, 57, 63, 64, 65 and more (PASS; the plan expected 55 or 56). `git ls-files` shows no file under src or tests above 65536 bytes (`runner.c` is 55130) and no `*.bin`, `*.pce` (PASS; this needed the split). `head -5` of both new core files has `SPDX-License-Identifier: MIT OR Apache-2.0` and `sh scripts/check --stage licences` PASS.
- Task 2: results show the four `content_id_nist_*` and `record_content_fields` pass with `RUNNER_DONE 16` (PASS). Changing `ba7816bf...` to `ca7816bf...` in the abc literal gave `RESULT content_id_nist_abc guest_failure` ("the digest fed in uneven chunks differs from the NIST literal"), then `git checkout --` on that one file restored it (PASS). The head of each new case file holds the 0BSD header comment (PASS).
- NIST literals re-derived while authoring with `printf ... | shasum -a 256` (and `head -c 1000000 /dev/zero | tr '\0' a | shasum -a 256`): all four equal the plan's values.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 30 lines ending in PASS, `CHECK RESULT PASS` |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range 4c1507a..HEAD` | `audit-history: clean` (3 commits, 22 blobs) |
| `sh scripts/conformance-run run --preset gate-gcc` and `--preset gate-clang` | `STATUS hash-sweep ok lengths=21` and `STATUS conformance ok n=16` for both |

Informal extra, not a gate, not committed: the core, text, runner and synth sources compiled with Apple clang `-std=c11 -fsanitize=undefined -fno-sanitize-recover=all` and no `-fwrapv` (the D-83 sanitize condition), hash and emit agreed with `shasum -a 256` for lengths 0, 1, 55, 56, 64, 65, 119, 120, 8193, 262144 and 1048576, and the whole 16-case run printed `RUNNER_DONE 16` with no runtime error. The real sanitize preset is plan 10's.

## Files Created/Modified

See `key-files`. All new `.c` and `.h` files carry the MIT OR Apache-2.0 header, the six new case files carry the 0BSD header.

## Decisions Made

See `key-decisions`. The plan fixed the algorithm, the record keys, the flag names, the sweep list and the case ids; the choices left open were the two kind names, where in `conformance-run` the sweep runs, which recipe the pinned record literal uses, and how to stay under the file size limit.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `runner.c` exceeded the 65536-byte acceptance limit after the Task 1 additions**
- **Found during:** Task 1 acceptance check (`git ls-files` size scan: 69449 bytes)
- **Issue:** the criterion "no file under src or tests larger than 65536 bytes" would fail, and Task 2 adds more to the same file.
- **Fix:** moved `kind_profile_equal`, `kind_names_both_ways`, `kind_field_flip`, their TSV loaders and the two path globals (16 KB, unchanged text) to `tests/conformance/profkinds.c`; the structures and helpers they share went to `tests/conformance/runner.h` (`struct kase`, `struct ctx`, `struct spec`, the request enums, `check`, `harness_error`, `parse_size`, `read_file`, `split_tab`, `parse_spec` now non-static). `CMakeLists.txt` lists the new source.
- **Files modified:** `tests/conformance/runner.c`, `tests/conformance/runner.h`, `tests/conformance/profkinds.c`, `CMakeLists.txt`
- **Verification:** 11 then 16 cases pass with the same assertion counts as before the move (field_flip 13, profile_names_both_ways 54, profile_r1_equal 81, record_skeleton_256k 73); full `sh scripts/check` PASS.
- **Commit:** `0019efe`

### Additions and interpretation choices (inside the plan's intent)

1. **A fourth print-and-exit flag, `--spec`,** and `synth_spec()`, because the plan says the LCG seed spec string is part of every recipe and plan 17 hashes it into the manifest; the string needs one definition and a way to be read.
2. **`synth_parse` was added** and `synth_size` kept as a wrapper, because `len:0` is a valid recipe whose natural size is 0, which `synth_size`'s "0 means unknown" cannot express. `len:N` rejects a leading zero and values above 16777216.
3. **`record_content_fields` uses fixture `len:262144`,** not `plain-256k`. The sha256 literal is the host `shasum -a 256` of that stream (985d45f2...2ec47), equal to the core's `--hash`. Plan 08 changes `plain-256k`; it cannot change `len:N`.
4. **TDD RED for the NIST cases is `harness_failure` (unknown kind),** not a wrong digest: the core hash already existed from Task 1, so there was no core behaviour to be red against. The red state shows the cases are required and judged. `record_content_fields` passed already at RED because the printer and hash came with Task 1; this is stated, not hidden.
5. **The sweep prints an extra `STATUS hash-sweep ok lengths=21` line** before `STATUS conformance ok n=N`; `checker` in `scripts/check` needs a `STATUS` line and still gets one.
6. **`content_id_vs_shasum` is also checked for non-zero id and a flipped-bit change,** two assertions beyond the plan text, so that a never-filled slot or a hash that ignores the data is caught inside the case and not only by the sweep.

**Total deviations:** 1 auto-fixed (Rule 3), 6 interpretation choices. **Impact:** none on later plans' contracts except that case-kind code for the profile kinds now lives in `profkinds.c`.

## Known Stubs

None. The stub recorded for plan 05 (`ds_instance.content_id` was a zeroed placeholder) is resolved by this plan and marked fixed in `.planning/WINDOWS.md` (id 2). The other open ledger entry (id 1, the home-path history finding) is unrelated and still an owner decision.

## Notes for later plans

- **Record skeleton is 51 rows.** Plans 12 and 14 that add tables must edit `build_expected_keys` and the pinned `n_exp == 51u` in `runner.c` (and the case meaning text), as plan 06 noted for 50.
- **`runner.h` is the place for shared case-kind helpers.** A new kind file includes it; `profile_tsv_path` and `rules_fields_path` are the only shared globals.
- **Sweep list for plan 17:** `build/PRESET/ds_runner --sweep-lengths` (21 lines) and `--spec`; the host-side comparison command is in `hash_sweep` of `scripts/conformance-run`.
- **`DS_CONTENT_ID_BYTES` is 32** and the id is raw bytes in the instance; only the printer turns it into hex. Plan 13 can hash the profile text with `ds_sha256` from the core.
- **Real gcc and the real sanitize preset are still unrun** (plans 10 and 21); the UBSan result above is an informal single-compiler run.

## Threat Flags

None. `--emit` writes synthetic bytes to stdout only, no file, no network. T-02-12 is mitigated as planned (NIST literals, the chunked one-million-a feed, the 21-length sweep against the host tool, the bit-length mutant observed red). T-02-13: no image is written to the tree. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist: `src/core/ds_sha256.h`, `src/core/sha256.c`, `tests/conformance/runner.h`, `tests/conformance/profkinds.c`, `tests/conformance/cases/content_id_vs_shasum.toml`, `content_id_nist_empty.toml`, `content_id_nist_abc.toml`, `content_id_nist_448bit.toml`, `content_id_nist_million_a.toml`, `record_content_fields.toml` (checked with `[ -f ]`).
- Commits `0019efe`, `c59356d`, `360cab9` are ancestors of HEAD.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
