---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 13
subsystem: core
tags: [state-digest, canonical-stream, profile-identity-hash, region-table, per-region-perturbation, toy-control, non-mutation, sha256]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: in-house SHA-256 (02-04), the boot order and instance (02-05), the toy-control and anchor mechanism (02-09), the refusal matrix (02-11), seeded power-on state and placeholder stepping (02-12)
provides:
  - serial.c and ds_serial.h: ds_digest, ds_digest_stream, ds_region_info and ds_profile_hash over one eight-region table, a framed little-endian stream, no struct bytes
  - profile_fields.def ledger column and ds_field_ledger; profile_equal now compares the ledger ids with r1
  - runner kinds digest_stream, digest_pair, profile_hash, digest_region, nonmutation_digest (digkinds.c); 7 new cases (95 in total)
  - tests/controls/drop-region (RULE and EXPECT, no committed copy of a core file): CONTROL drop-region RED_OK
affects: [02-14 (record may print the digest and the profile hash), 02-15, 02-16 (shared-state rule), later phases that add machine state (each needs a region and a serializer version bump)]

actuals:
  tokens: 16359    # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 65437 chars, 21 files, 1187 added and 23 removed lines)
  tasks: 2
  commits: 2       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: a433090ee4b463e4422471208113c2a199fb40ab
plan_head_after: 3292bde620a2e8ce238cb0662ad695da9ab56073
commits: 2

tech-stack:
  added: []
  patterns:
    - "one region table (tag, payload length, live offset and length through offsetof) serves the digest, the stream capture and the test accessor; the payload of each region is one explicit-field call per statement in a switch"
    - "every integer is written byte by byte from unsigned operands, so the sanitize preset (no -fwrapv) ran clean over the new code"
    - "independent literals from a throwaway perl script outside the repo (Digest::SHA, Math::BigInt) that writes the stream of the written rules, cross-checked against the splitmix literals of plan 12"
    - "a kind family is its own file (digkinds.c), as powkinds.c and stepkinds.c"

key-files:
  created:
    - src/core/ds_serial.h
    - src/core/serial.c
    - tests/conformance/digkinds.c
    - tests/controls/drop-region/RULE
    - tests/controls/drop-region/EXPECT
    - tests/conformance/cases/ (7 files: digest_stream_prefix, poweron_same_seed_same_digest, poweron_diff_seed_diff_digest, profile_hash_fields, profile_hash_identity, digest_region, nonmutation_digest)
  modified:
    - src/core/ds_core.h
    - src/core/profile.c
    - src/core/profile_fields.def
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/profkinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/profile_r1_equal.toml
    - CMakeLists.txt

key-decisions:
  - "the digest stream is the prefix duospool-state plus NUL, version u32 1, then eight regions PROF CONT POWR TIME RAM8 MPRS PSGN VDCP, each framed as four ASCII tag bytes (reading order, not a packed integer), a u32 length and the payload; the ROM bytes are excluded and stand as the content id"
  - "the profile hash string rule: a string is a u32 length and its bytes, bounded by the array size, so bytes after a NUL are never hashed; the stream also writes the field count and the unresolved count"
  - "ds_region_info returns the tag as a static C string and a span of live storage; one table row carries tag, payload length, live offset and live length"
  - "digest_region flips the first and the last byte of each live span (PROF the first only, because the tail of that span is the padding of the last value)"
  - "nonmutation_digest aims every refuse row at the memory of the live instance, not at a fresh block"

patterns-established:
  - "a new piece of machine state is a new region of the table plus a serializer version bump; digest_region and digest_stream_prefix widen by the table"

requirements-completed: [CORE-01, PROF-03]

coverage:
  - id: D1
    description: "The state digest is SHA-256 over a framed little-endian stream (prefix, NUL, version 1, eight tagged regions); the framing, every payload and the stream length are checked by code that does not call ds_digest, and the digest equals the SHA-256 of the captured stream and a perl-derived literal"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance PASS; RESULT digest_stream_prefix pass assertions=22"
        status: pass
    human_judgment: false
  - id: D2
    description: "The same inputs and seed give the same digest in differently filled blocks, and a different seed gives a different digest"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "RESULT poweron_same_seed_same_digest pass assertions=3, poweron_diff_seed_diff_digest pass assertions=3"
        status: pass
    human_judgment: false
  - id: D3
    description: "Every region matters: flipping the first and last live byte of each of the 8 regions changes the digest and restoring it restores it; the drop-region toy (RAM8 listed, payload not written) is shown red"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "RESULT digest_region pass assertions=8; CONTROL drop-region RED_OK; replacement equal to anchor printed CONTROL drop-region NOT_RED and the stage FAIL; SELFTEST mutation-control-anchor drop-region PASS; scratch serial.c mutations M1, M4, M5, M6 each turned digest_region red"
        status: pass
    human_judgment: false
  - id: D4
    description: "A refused boot never changes the digest of a live instance (upgrade of the plan 11 byte comparison): 45 refuse rows aimed at the live instance memory plus the whole-run comparison"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "RESULT nonmutation_digest pass assertions=46; a scratch write into the instance on refusal turned nonmutation_digest and nonmutation_matrix guest_failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "The profile identity hash covers id, rev, every resolved field (name, status, value, bound_by) and the sorted unresolved ids, never label text; all 13 fields are shown to matter and the canonical stream equals perl-derived literals"
    requirement: PROF-03
    verification:
      - kind: integration
        ref: "RESULT profile_hash_fields pass assertions=13, profile_hash_identity pass assertions=9, profile_r1_equal pass assertions=94 (ledger ids now compared)"
        status: pass
    human_judgment: false

duration: 14 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 13: Canonical state digest and profile identity hash Summary

**The machine state is now hashed as a framed little-endian stream of eight tagged regions behind a prefix and version (never struct bytes), the profile identity is hashed over id, rev and every resolved field, and each region is shown to matter by a per-region flip and by a toy serializer that drops the RAM8 payload and is observed red.**

## Performance

- **Duration:** about 14 min (2026-11-10, 06:24Z to 06:38Z)
- **Tasks:** 2 of 2 (Task 1 tracer, Task 2 auto with tdd)
- **Files:** 21 changed outside .planning (1187 added lines, 23 removed)
- **Gate cost (this machine, warm):** `sh scripts/check` 31.9 s wall, no stage over 20 s; the conformance stage alone about 4.6 s; `sh scripts/check --selftest` 8.4 s.

## Accomplishments

- **Tracer (Task 1).** `serial.c` has a small writer (bytes, u32 and u64 little-endian from unsigned operands, bounded length-prefixed strings) feeding `ds_sha256_update`, one eight-row region table (tag, payload length, live offset and length by `offsetof`), `ds_digest_stream` (the bytes the digest hashes, for tests), `ds_digest`, `ds_region_info` and `ds_profile_hash`. There is no `__builtin_memcpy`, no struct hashed, no libc. The tracer was proven end to end before expansion: the digest stream framing is parsed in `digest_stream_prefix` by code that never calls `ds_digest` (prefix, NUL, version, each frame in order, each payload against the instance, the stream consumed exactly), and the digest equals both the SHA-256 of the captured stream and a literal derived by a separate perl script.
- **Profile identity.** `ds_profile_hash` writes `duospool-profile` and a NUL, version, id, rev, the field count, then name, status, value and bound_by for each of the 13 fields in table order, then the sorted unresolved ids; strings are a u32 length and their bytes. `profile_fields.def` gained the ledger id column (`ds_field_ledger`; `powerup_policy` carries `LED-PROF-0002` as a core constant because r1 omits it), and `profile_equal` now compares the ledger ids of every field with the ledger key of r1 (94 assertions, up from 81).
- **Per-region perturbation and non-mutation (Task 2).** `digest_region` reaches every region through `ds_region_info`, flips the first and the last byte of its live span and requires the digest to change and to come back (8 assertions, one per region, order PROF CONT POWR TIME RAM8 MPRS PSGN VDCP). `nonmutation_digest` boots a seeded live instance and then boots every one of the 45 refuse rows into the memory of that live instance, comparing the digest before and after each (46 assertions).
- **Toy control.** `tests/controls/drop-region` holds `RULE` (tag `rule-drop-ram8-payload`, file `src/core/serial.c`, the anchor `w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);` which matches one line of the live file, replacement `;`) and `EXPECT` (`digest_region`). The toy is generated into `build/` and compiled with the live core; `ls tests/controls/drop-region` lists exactly those two files. The controls mode prints `CONTROL drop-region RED_OK` besides the two earlier toys.
- **Independent derivation.** The literals come from a throwaway perl script kept outside the repo (Digest::SHA, Math::BigInt) that writes the stream of the written rules: the profile hashes of the unbound r1 constant, of the default-bound profile (policy zero) and of the seed-42 profile, and the full digest of a seed-42 boot of `plain-256k` (content sha256 from the host `shasum` of the emitted recipe, RAM and registers from the splitmix literals of plan 12, which the script reproduced: `37db8e2e...`, `86e36fa2...`, `0x97809872`, `0xA228BF93`). The stream is 8387 bytes (15 + 4 + eight frames of 8 bytes of overhead + 8304 of payload). The method is recorded in the comments of the cases.

## Verification Results

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance && sh scripts/check --stage sanitizers` (Task 1, run on the tree as committed) | exit 0, `CHECK conformance PASS`, `CHECK sanitizers PASS`; `RESULT digest_stream_prefix pass assertions=22`, `poweron_same_seed_same_digest pass 3`, `poweron_diff_seed_diff_digest pass 3`, `profile_hash_fields pass 13`, `profile_hash_identity pass 9`, `profile_r1_equal pass 94`; sanitize results identical (UBSan clean, no `-fwrapv`) |
| same pair (Task 2) | exit 0, both PASS; `digest_region pass assertions=8`, `nonmutation_digest pass assertions=46` in both presets; `CONTROL drop-region RED_OK`, `CONTROL heuristic-reverse RED_OK`, `CONTROL wrong-mirror RED_OK` in `build/gate/controls.txt` |

Acceptance criteria, all run:

- Task 1: `profile_hash_fields` assertions=13. `grep -n '__builtin_memcpy' src/core/serial.c` prints nothing (exit 1): there is no memcpy at all, so there is no whole-struct copy into the stream (reviewed). `sh scripts/check --stage licences` prints `CHECK licences PASS` with `ds_serial.h` and `serial.c` present.
- Task 2: `digest_region` assertions=8, equal to the region count; `nonmutation_digest` assertions=46, which is 45 refuse rows (counted with `grep -c '"refuse"'` over the case files) plus one. `ls tests/controls/drop-region` lists exactly `EXPECT` and `RULE`. `head -5` of `RULE` contains `SPDX-License-Identifier: 0BSD`. Replacement set equal to the anchor (run once, then restored from a copy) printed `CONTROL drop-region NOT_RED` with `expected guest_failure; got: digest_region:pass` and `CHECK conformance FAIL`. `sh scripts/check --selftest` printed `SELFTEST mutation-control-anchor drop-region PASS`.
- Task 1 and 2, TDD: the red state was observed against a zero-returning stub of `serial.c` before the real one was written: `digest_region`, `digest_stream_prefix`, `poweron_diff_seed_diff_digest`, `profile_hash_fields` and `profile_hash_identity` were `guest_failure`; `poweron_same_seed_same_digest` and `nonmutation_digest` passed on the stub, as they must for a digest that is constantly zero (they guard the other direction; `poweron_diff_seed_diff_digest`, `digest_region` and the literal catch the dead digest). The stub was never committed.
- Liveness (each observed once on a scratch copy, then restored with `cmp` against the saved good copy): M1 drop the `vdc_phase` write while the frame still declares 4 bytes turned `digest_region` and `digest_stream_prefix` red; M2 drop `bound_by` from the profile hash turned `digest_stream_prefix`, `profile_hash_fields` and `profile_hash_identity` red; M3 hash the whole 30-byte array instead of the text turned `digest_stream_prefix` and `profile_hash_identity` red; M4 write seed 0 into POWR turned `digest_stream_prefix` and `digest_region` red (with first-byte-only flipping it was only `digest_stream_prefix`); M5 truncate machine time to 32 bits turned only `digest_region` red (the last-byte flip, which is why the case flips both ends); M6 drop the RAM8 write by hand turned `digest_region` and `digest_stream_prefix` red; a scratch write into the instance on every refusal in `boot.c` turned `nonmutation_digest` and `nonmutation_matrix` red.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, ends `CHECK RESULT PASS` (31.9 s wall); three toy controls print RED_OK |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | `SELFTEST mutation-control-anchor drop-region PASS`, `heuristic-reverse PASS`, `wrong-mirror PASS`, `mutation-drop-precedence PASS`, then `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range a433090..HEAD` | `audit-history: clean` (2 commits, 25 blobs) |

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 fix was needed in committed code.

### Interpretation choices (inside the plan's intent)

1. **Files beyond `files_modified`.** `tests/conformance/digkinds.c` (the new kind family, as `powkinds.c` and `stepkinds.c` before it), `tests/conformance/runner.h`, `CMakeLists.txt` (one source joined the runner list), `tests/conformance/profkinds.c` and `tests/conformance/cases/profile_r1_equal.toml` (the ledger comparison). `runner.c` only gained five table rows.
2. **One extra case, `profile_hash_identity` (seven new cases, not six).** It pins the canonical profile stream with perl-derived literals (the unbound constant and the default-bound profile), and checks that the id and the revision matter, that bytes after a NUL are not hashed, and that the unresolved ids are sorted. `profile_hash_fields` stays at exactly 13 assertions as the plan requires. `REQUIRED` now has 95 ids.
3. **The literal digest rides in `digest_stream_prefix`.** The plan asked only for the framing and the SHA-256 equality; the case additionally pins the full digest of a seed-42 boot to a perl-derived literal, so the encoding (not only the self-consistency) is checked.
4. **`digest_region` flips the first and the last byte of each live span**, with `PROF:first` for the one span whose tail is the padding of the last value. The plan said one byte; the M5 mutation (time truncated to 32 bits) was invisible to a first-byte flip and the M4 mutation was caught by one case only. The assertion count is still one per region (8).
5. **`nonmutation_digest` aims each refuse row at the memory of the live instance** (with the row's own `mem=` and `cap=` overrides), which is the stronger reading of "a refused boot never changes the digest of a live instance" than a fresh block; plan 11's byte comparison keeps the fresh-block form.
6. **`ds_region_info` shape.** `uint32_t ds_region_info(struct ds_instance *inst, uint32_t index, const char **tag, uint8_t **ptr, size_t *len)`: tag as a static string, a writable pointer so the case can flip a byte, return 1 for a valid index and 0 otherwise (the count is found by walking, no separate count function).
7. **String framing of the profile hash and the field count.** The plan fixed the field list but not the string encoding; a string is a u32 length and its bytes, and the stream also carries the field count and the unresolved count, so no two different profiles can give the same stream.
8. **Task 1 was committed with the two Task 2 kinds and cases withheld.** The kinds `digest_region` and `nonmutation_digest` were written together with Task 1 during the red phase and then removed from the Task 1 commit and restored for Task 2, so each commit passes the gate on its own.
9. **CORE-01 and PROF-03 are not yet checked off in REQUIREMENTS.md.** `requirements.ready-ids` reported both blocked (a later plan of this phase also declares each and has no SUMMARY yet, the shared-ID gate); they are marked when the last declaring plan finishes.

**Total deviations:** 0 auto-fixed, 9 interpretation choices. **Impact:** none on later plans except the notes below.

## Notes for later plans

- **The digest does not cover `mapper`, `byte_order` or `header`.** The plan fixed eight regions. These three instance fields are constant under every accepting boot today (mapper "plain", pce, no header; tg16 and copier headers are refused), so they cannot make two accepted instances differ. A later plan that accepts a second value for any of them must add a region and bump `DS_SERIAL_VERSION`.
- **New machine state needs a region.** Any later field of `struct ds_instance` that can differ between two accepted runs (CPU registers, VDC state, a mapper bank register) must be a row of the table in `serial.c` and its `expect` in `digest_stream_prefix` and `digest_region`; the version bump changes the two perl-derived digest literals.
- **Plan 14 (record).** The digest and the profile hash are available as `ds_digest` and `ds_profile_hash` if the record should print them. Printing a digest is repeatability evidence only; label it so.
- **Plan 16 (shared-state rule).** Unchanged: the anchor is the `psg_noise` update line of `advance.c`. The digest now covers `psg_noise` (region PSGN).
- **Request grammar and runner.** Unchanged; every runner invocation still needs `--marker-seed N`. The new kinds: `digest_stream`, `digest_pair` (request `A||B`, expect `same` or `differ`), `digest_region` (expect `tags=...`), `profile_hash` (expect `fields` or `identity;unbound=HEX;resolved=HEX`), `nonmutation_digest`.

## Task Commits

1. **Task 1 (tracer): framed state digest, profile identity hash and same-seed versus different-seed digests** - `d551ff0` (feat)
2. **Task 2: per-region perturbation, digest-based non-mutation and the drop-region toy** - `3292bde` (feat)

## Known Stubs

None. No hardcoded empty value flows to a consumer; `fixture = "none"` in `profile_hash_fields`, `profile_hash_identity` and `nonmutation_digest` is real (the kinds boot their own fixture where they need one).

## Threat Flags

None. No network endpoint, file access, auth path or schema change at a trust boundary was added. T-02-24 is mitigated: `digest_region` perturbs every region of a live instance, the `drop-region` toy is observed red, the framing is checked by an independent stream capture, and `profile_hash_fields` flips all 13 fields. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `src/core/ds_serial.h`, `src/core/serial.c`, `tests/conformance/digkinds.c`, `tests/controls/drop-region/RULE`, `tests/controls/drop-region/EXPECT` and the 7 new case files (95 ids in `REQUIRED`).
- Commits `d551ff0` and `3292bde` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 2.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
