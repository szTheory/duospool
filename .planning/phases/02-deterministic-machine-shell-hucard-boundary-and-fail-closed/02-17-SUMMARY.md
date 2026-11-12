---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 17
subsystem: fixtures
tags: [fixture-manifest, fixture-check, ds_runner-fixtures, host-sha-tool, fail-closed, synthetic-recipes, e0-repeatability]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: synth.c recipes and the --emit, --hash, --spec and --sweep-lengths flags (02-07), the two-stage TOML checker pattern (02-01, 02-04), run_controls and the selftest mechanism (02-02, 02-03), the case inventory and runner (02-08 to 02-16)
provides:
  - fixtures/manifest with 43 entries (one per recipe the gate emits), redistribution generated-not-committed, licence 0BSD, no ROM-like byte committed
  - ds_runner --cases FILE --fixtures (tests/conformance/fixlist.c): NAME SIZE for every recipe a case or the hash sweep emits, 43 lines, derived through parse_spec and synth_size
  - synth.c recipe form plain-*:N and synth_name, so a case with a len=N override is named for the bytes it really generates
  - scripts/fixture-check (--root, --used, --partial, --expect) with scripts/lib/fixture-rules.awk, ten FIX_ codes
  - conformance-run fixtures [--only NAME] with the shared flatten_cases, build_preset and pick_sha_tool functions
  - tests/gate/fixture (7 mini-root controls); stages fixtures and fixture-negatives; selftest mutation-fixture-missing-field and mutation-fixture-unlisted
affects: [02-18 and later (the gate has 39 stages), any later plan that adds a case with a new fixture key or a new len=N override (it needs a manifest entry, FIX_UNLISTED names it), 02-19 (runner.c is at 65503 of 65536 bytes)]

actuals:
  tokens: 34236    # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 136944 chars, 74 files, 1612 added and 45 removed lines; 43 manifest files and 7 control mini-roots make most of the files)
  tasks: 4
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: f2d6e881cb263189ad95aa385da36f758b20f87e
plan_head_after: 753b93d385d030ad245cea37b2162ed0e6a116de
commits: 4

tech-stack:
  added: []
  patterns:
    - "a manifest entry per byte string: the ID is the recipe name with hyphens and colons turned into underscores, the entry's size and sha256 are compared with what ds_runner --emit wrote, as measured by wc -c and the host sha tool, never by files in the tree"
    - "the list of recipes the gate emits comes from the runner (--fixtures) through the same request parser and recipe-size function the cases use, so the manifest coverage claim is the --fixtures output and cannot drift from the cases"
    - "a mode that needs the cases TSV or the binary produces them itself (shared flatten_cases and build_preset functions), so it never reads a stale file or needs an earlier stage"
    - "a stage that judges a used list asserts afterwards, by count and by name, that the list covers every manifest file, so a partial or stale list cannot be judged"

key-files:
  created:
    - fixtures/manifest/ (43 entries: plain_256k, plain_384k, plain_1m, 21 len_N, 19 plain_256k_N and plain_1m_N)
    - scripts/fixture-check
    - scripts/lib/fixture-rules.awk
    - tests/conformance/fixlist.c
    - tests/gate/fixture/ (7 mini-roots: ok-listed, fix-missing-field, fix-unlisted, hash-mismatch, size-mismatch, bad-verdict-source, stale-entry)
  modified:
    - scripts/conformance-run
    - scripts/check
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/synth.c
    - tests/conformance/synth.h
    - CMakeLists.txt

key-decisions:
  - "a case with a len=N override over a plain-* fixture is the recipe plain-*:N (the first N bytes of the tagged image), not len:N: the bank tags of the natural size are part of those bytes, so a hash of the pure stream would not be what the case emits"
  - "the --fixtures list is derived from the declared fixture and request keys with the kinds' own parser; the one kind with a grammar of its own (precedence, a table of REQ>>CODE items) is read the way refkinds.c reads it; a case that cannot be resolved is exit 2"
  - "the stage fixtures asserts coverage twice: fixture-check reports FIX_STALE for an entry no used line names, and the stage compares manifest stems with used names (a gap is a harness error)"
  - "verdict_source is checked by shape only (spec-literal:LED-AREA-NNNN, guest-computed, independent-script); the literal duospool-output and any other value is FIX_BAD_VERDICT_SOURCE; whether the named ledger entry exists is not checked here"

patterns-established:
  - "runner.c is at 65503 of 65536 bytes: any further runner feature goes in its own file, as fixlist.c did, with a call and a declaration only"

requirements-completed: [FIX-02]

coverage:
  - id: D1
    description: "Every recipe the gate can emit for a case or for the hash sweep has a manifest entry with all keys, and the gate compares the hash and size of the runner's emitted bytes, measured by host tools, with the entry"
    requirement: FIX-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage fixtures PASS: FIXTURES manifest=43 used=43, STATUS fixtures ok; ls fixtures/manifest | wc -l is 43 and ds_runner --cases build/gate/cases.tsv --fixtures | wc -l is 43; ds_runner --emit plain-256k | shasum -a 256 equals the sha256 of fixtures/manifest/plain_256k.toml (cb038cda...)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The list the runner prints is exactly the set of recipes a real run generates (nothing undocumented, nothing listed that is never emitted)"
    requirement: FIX-02
    verification:
      - kind: integration
        ref: "scratch copy of the tree with a log line in synth_fill, full runner run: 23 distinct (recipe, length) pairs, mapped to recipe names, equal the 22 case-derived names of --fixtures plus len:262144 (also a sweep length); in-run-not-in-list and in-list-not-in-run were both empty. A second resolver written in python over cases.tsv gave the same 43 names (assert on set equality)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A missing field, an unlisted fixture, a hash mismatch, a size mismatch, the rejected verdict source duospool-output and a stale entry each fail closed with their own code, and an empty used list is a harness error"
    requirement: FIX-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage fixture-negatives PASS (7 mini-roots: each fix case gives exactly its one code, ok-listed passes); an empty --used exits 2 with HARNESS_EMPTY_USED"
        status: pass
    human_judgment: false
  - id: D4
    description: "The missing-field rule and the unlisted rule are mutation-proven"
    requirement: FIX-02
    verification:
      - kind: integration
        ref: "SELFTEST mutation-fixture-missing-field PASS and SELFTEST mutation-fixture-unlisted PASS; sh scripts/check --selftest prints 25 PASS lines and SELFTEST RESULT PASS"
        status: pass
    human_judgment: false

duration: 25 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 17: Fail-closed fixture manifest Summary

**The 43 synthetic recipes the gate emits (3 named, 21 hash-sweep lengths, 19 case-derived len=N images) each have a manifest entry, and a new `fixtures` stage hashes the runner's `--emit` bytes with the host sha tool and compares size and sha256 with the entry, failing closed on every field, hash, size, verdict-source, licence and staleness fault.**

This is a change detector and an inventory, not an oracle: the manifest hashes document what the generator produces today, so a changed generator, an image used without an entry or an entry nobody emits turns the gate red. Matching hashes prove repeatability (E0) and that the documentation is current. They say nothing about whether a recipe resembles hardware or a real HuCard. Nothing ROM-like is committed (every entry is `generated-not-committed`).

## Performance

- **Duration:** about 25 min (2026-11-10)
- **Tasks:** 4 of 4 (Task 1 tracer, Tasks 2 to 4 auto)
- **Files:** 74 changed outside .planning (1612 added lines, 45 removed; 43 manifest files and 7 control mini-roots are most of them)
- **Gate cost (this machine, warm):** `sh scripts/check --timing` 39 s wall over 39 stages, all PASS. The new `fixtures` stage takes 2 s (it flattens, builds incrementally and hashes 43 images), `fixture-negatives` under 1 s; no `TIMING_WARN`.

## Accomplishments

- **Tracer (Task 1).** `ds_runner --cases FILE --fixtures` (new `tests/conformance/fixlist.c`, one call in `main`) prints `NAME SIZE` per line, de-duplicated and sorted. It reads each case with `parse_spec` and `synth_size`, the functions the kinds use, takes every request of a `||` list, and counts a request as materialising bytes only when `ptr` is ok and `len` is within the content limit (`ptr=null`, `ptr=dummy` and `len=size_max` give none). `scripts/fixture-check` is the two-stage checker (stage 1 `toml-subset.awk` with no pipeline, stage 2 `fixture-rules.awk` with the V and M records, a NO_SENTINEL and a STAGE2_DEAD guard), joined with the used list by an awk comparison. `conformance-run fixtures [--only NAME]` writes `build/gate/fixtures.used` from host tools only and runs the checker. First entry: `plain_256k`.
- **Group 1 (Task 2).** `plain_384k`, `plain_1m` and one `len_N` for each of the 21 lines of `ds_runner --sweep-lengths`, written mechanically (the generator is a scratch script, not committed; hashes come from `ds_runner --emit` through `shasum -a 256`, never typed). The hash of `len_262144` (985d45f2...) equals the literal the record case `record_content_fields` already pins, an independent agreement.
- **Group 2 (Task 3).** N_case = 19: `plain_1m_1056768` and eighteen `plain_256k_N` entries (N = 0, 1, 513, 8191, 8192, 8704, 16896, 25088, 131072, 196608, 262143, 262656, 393216, 524288, 786432, 1049088, 2097152, 2621440) from the refusal rows, the precedence table and the interleave cases. Final count 43 equals the line count of `--fixtures`. The plan expected at least 14.
- **Remaining rules, stage and mutations (Task 4).** `FIX_BAD_VERDICT_SOURCE` (rejects the literal `duospool-output` and every value outside the three accepted forms), `FIX_BAD_LICENCE` (eight SPDX ids), `FIX_STALE` (disabled by `--partial`, which only a `--only` run passes), controls `hash-mismatch`, `size-mismatch`, `bad-verdict-source` and `stale-entry`, the stage `fixtures` (removes any old used list, runs the full mode with no `--only`, then compares manifest stems with used names by count and by name), the stage `fixture-negatives`, and two selftest mutations.

## Verification Results

Per-task `<automated>` commands (exit codes and key lines):

| Command | Result |
|---------|--------|
| Task 1: `sh scripts/check --stage build && sh scripts/check --stage fixture-negatives && sh scripts/conformance-run fixtures --only plain-256k` | exit 0: `CHECK build PASS`, `CHECK fixture-negatives PASS`, `FIXTURES manifest=1 used=1`, `STATUS fixtures ok`. The tracer gate (auto mode) re-ran this after the commit: same result, so expansion went ahead |
| Task 2: build, then `conformance-run fixtures --only len:N` for each of the 21 lengths, then `--only plain-384k` and `--only plain-1m` | exit 0, every run printed `STATUS fixtures ok` with `manifest=24 used=1` |
| Task 3: `sh scripts/check --stage build && sh scripts/conformance-run fixtures` | exit 0: `FIXTURES manifest=43 used=43`, `STATUS fixtures ok`, no FIX_ code |
| Task 4: `sh scripts/check --stage build && --stage conformance && --stage fixtures && --stage fixture-negatives && --selftest mutation-fixture-missing-field && --selftest mutation-fixture-unlisted` | exit 0, four `CHECK ... PASS` lines and two `SELFTEST ... PASS` lines |

Acceptance criteria, all run:

- Task 1: `sh scripts/fixture-check --root tests/gate/fixture/ok-listed --used tests/gate/fixture/ok-listed/used.txt` prints `STATUS fixtures ok` (PASS). `ds_runner --emit plain-256k | shasum -a 256` gives `cb038cda079ab0edec440521c4dc250a8a8c2af6e068470dbda7f05ff07b0287`, the sha256 value in `fixtures/manifest/plain_256k.toml` (PASS). An empty used file exits 2 with `HARNESS_EMPTY_USED` (PASS). `head -3` of the manifest entry holds `SPDX-License-Identifier: 0BSD`, `head -5` of `scripts/fixture-check` holds `MIT OR Apache-2.0`, `--stage licences` PASS.
- Task 2: `ls fixtures/manifest | wc -l` is 24 (baseline 1), `ls fixtures/manifest | grep -c '^len_'` is 21, `--sweep-lengths | wc -l` is 21, the verify loop exits 0, `--stage licences` PASS.
- Task 3: `ls fixtures/manifest | wc -l` is 43 and `--fixtures | wc -l` is 43 (both recorded, at least 38 required), N_case = 43 - 24 = 19, `conformance-run fixtures` prints no FIX_ code, `--stage licences` PASS.
- Task 4: after `rm -f build/gate/fixtures.used`, `--stage fixtures` printed `CHECK fixtures PASS` and recreated the file with 43 lines, equal to the 43 manifest files (baseline: absent). In a scratch root with the manifest file of `len_65` removed, `fixture-check` exited 1 with `FIX_UNLISTED  len:65` (observed once). A scratch entry `len_999.toml` made the real stage fail with `FIX_STALE` (observed once, removed, stage PASS again). A one-line used file without `--partial` gave 42 `FIX_STALE` lines, with `--partial` it printed `STATUS fixtures ok` (both observed once). `ls tests/gate/fixture | wc -l` is 7. `--stage licences` PASS.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 39 `CHECK <stage> PASS` lines, ends `CHECK RESULT PASS` (39 s) |
| `dash scripts/check` | exit 0, ends `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | exit 0, 25 `SELFTEST ... PASS` lines including `mutation-fixture-missing-field` and `mutation-fixture-unlisted`, then `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range f2d6e88..HEAD` | `audit-history: clean` (4 commits, 68 blobs) |

The sanitizers stage (ASan and UBSan over the whole runner, which now compiles the changed `synth.c` and `fixlist.c`) is among the 39 PASS lines.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug in the plan's premise] A `len=N` override is not the recipe `len:N`**
- **Found during:** Task 1, while writing the resolver
- **Issue:** the plan names the case-derived recipes `len_N` (hash of `len:N`). The bytes a case with fixture `plain-256k` and `len=513` really generates are the first 513 bytes of the tagged image (bank tag bytes are overwritten at indexes that the pure `len:513` stream does not carry), and the interleave images (`plain-256k` at 393216 bytes) differ from both `len:393216` and `plain-384k`. A manifest that hashed `len:N` would document bytes no case emits.
- **Fix:** `synth.c` accepts the recipe form `plain-*:N` (N written without a leading zero, N equal to the natural size is refused so each byte string has one spelling) and gains `synth_name(fixture, len, ...)`, which names what `synth_fill(fixture, buf, len)` writes (the fixture itself, `plain-*:N`, or `len:N` over a `len:N` fixture). `synth_fill` delegates tagging to the base image, so `plain-256k:513` and `synth_fill("plain-256k", buf, 513)` are the same bytes by construction. The manifest IDs are therefore `plain_256k_513`, not `len_513`.
- **Files modified:** `tests/conformance/synth.c`, `tests/conformance/synth.h`
- **Verification:** the runtime trace described under D2 above (23 emitted pairs equal the list)
- **Commit:** `9e8b151`

**2. [Rule 3 - Blocking] Files beyond the plan's `files_modified` list**
- **Found during:** Task 1
- **Issue:** `runner.c` was at 65147 of 65536 bytes, so the list logic cannot live there, and the new source must be in the CMake source list.
- **Fix:** `tests/conformance/fixlist.c` (the list), `tests/conformance/runner.h` (one declaration), `tests/conformance/synth.c` and `synth.h` (deviation 1), `CMakeLists.txt` (one source). `runner.c` got a flag, a relaxed usage test and one call, and ends at 65503 of 65536 bytes.
- **Commit:** `9e8b151`

### Interpretation choices (inside the plan's intent)

1. **`--partial` is accepted from Task 1.** The plan lists it under Task 4, but the Task 1 verify runs `conformance-run fixtures --only plain-256k`, which passes `--partial`. The option was a no-op until Task 4 added the `FIX_STALE` rule.
2. **`--fixtures` is used with `--cases`.** The flag is an ordinary option of the case run (`ds_runner --cases build/gate/cases.tsv --fixtures`), not a print-and-exit flag, because it needs the loaded cases. It needs no `--evidence-dir`.
3. **The precedence kind is read by its own grammar.** `ref_precedence_table` holds a table of `REQ>>CODE` items in its request and expect keys; the list reads them the way `refkinds.c` does (otherwise the case fails to resolve, which is exit 2). Every other kind takes one request or a `||` list.
4. **Extra harness and structure codes.** `HARNESS_EMPTY_USED` and `HARNESS_BAD_USED` (exit 2, a used line must be `NAME SIZE SHA256`), `NO_SENTINEL` and `STAGE2_DEAD` (as in the profile checker). A value of the wrong type, a sha256 that is not 64 lowercase hex digits, a negative size and an absolute or parent-step source path are all `FIX_BAD_ENUM` (the plan lists no separate code).
5. **Size is measured, not trusted.** `conformance-run fixtures` takes the size from `wc -c` over `--emit` and stops with a harness error when the runner's own size column disagrees, and it runs `--emit` on its own first so a failing emit cannot read as the digest of an empty image.
6. **Shared functions in `conformance-run`.** `flatten_cases`, `build_preset` and `pick_sha_tool` are shared between the run mode, the fixtures mode and the hash sweep (D-86 says one shared function for the two producing steps). Run-mode behaviour is unchanged: the whole gate and the sanitizers stage pass.
7. **Stage count.** The plan states 39 after this plan; the gate has 39 `run_stage` lines (the plan 16 summary said 38 for the count before, which was one too high: it was 37).
8. **Case meaning text.** Case-derived entries say that accept or refuse is the case's own expectation (a spec literal), not a hash of core output; their verdict source is `independent-script` as the plan says.

**Total deviations:** 2 auto-fixed (one Rule 1, one Rule 3), 8 interpretation choices. **Impact:** none on the plan's intent; the manifest count (43) and the IDs of the case-derived entries differ from the plan's `len_N` wording.

## Notes for later plans

- **A new case that adds a fixture key or a `len=N` override that materialises bytes needs a manifest entry.** `sh scripts/conformance-run fixtures` prints `FIX_UNLISTED` with the recipe name. Generate the entry from the runner's bytes (size by `wc -c`, hash by the host tool over `ds_runner --emit NAME`), never by hand. A case that stops using a recipe leaves a `FIX_STALE` entry to delete.
- **`runner.c` is at 65503 of 65536 bytes.** Put anything new in its own file.
- **Case-local mutations are not fixtures.** The `noheur` edits, the redaction ROM and similar images made at run time are applied after generation; their case meaning names the base recipe, and the list correctly does not carry them. A future kind that builds an image some other way than through `synth_fill` with the case's fixture and request would be invisible to `--fixtures`; give it a resolver branch in `fixlist.c`.
- **Rights allowlist.** All 43 entries are `generated-not-committed`, so none excuses a path in `rights-check` (only a `committed` authored entry does). Phase 3 authored fixtures must use `kind = "authored"` with `committed` and an SPDX licence.
- **Limitation.** The `recipe` text of an entry contains the generator spec string (`ds_runner --spec`) but no rule checks that the text equals the live spec; the hash comparison catches a changed generator, and the text is documentation that a person updates with the hash. `verdict_source = spec-literal:LED-...` is shape-checked only.
- **Evidence tier.** E0 (repeatability). The host sha tool shares no code with the generator, but the manifest and the generator come from the same source of truth, so agreement shows that the documentation matches the current generator, not that the generator is right.

## Task Commits

1. **Task 1 (tracer): one manifest entry compared with the runner's emitted bytes, plus the missing-field and unlisted controls** - `9e8b151` (feat)
2. **Task 2: manifest group 1, the named recipes and the 21 sweep recipes (23 files)** - `f370a2c` (feat)
3. **Task 3: manifest group 2, the case-derived recipes (19 files)** - `7452666` (feat)
4. **Task 4: remaining rules and controls, the fixtures stage and two mutations** - `753b93d` (feat)

## Known Stubs

None. No placeholder text, empty data flow or unwired component was added; the placeholder stepping that some cases exercise is plan 12's documented placeholder and is not a stub of this plan.

## Threat Flags

None. No network endpoint, auth path or file-access pattern at a trust boundary was added (the manifest and the used list are read-only inputs to a local checker; the emitted bytes go to a pipe, never to a file). T-02-34 is mitigated by FIX_UNLISTED, FIX_HASH_MISMATCH, FIX_SIZE_MISMATCH and FIX_STALE with controls and two mutations, the independent host hash, and the `--fixtures` enumeration of the exact emitted set including the sweep (checked against a real run). T-02-37 is mitigated by FIX_BAD_VERDICT_SOURCE and its `bad-verdict-source` control. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `scripts/fixture-check`, `scripts/lib/fixture-rules.awk`, `tests/conformance/fixlist.c`, `fixtures/manifest/plain_256k.toml`, `fixtures/manifest/len_65.toml`, `fixtures/manifest/plain_256k_513.toml`, the seven `tests/gate/fixture/*/used.txt`.
- Commits `9e8b151`, `f370a2c`, `7452666` and `753b93d` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 4.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
