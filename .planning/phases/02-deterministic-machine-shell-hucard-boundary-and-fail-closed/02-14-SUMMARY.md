---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 14
subsystem: record
tags: [run-record, golden, refusal-record, record-lint, hostile-string, byte-compare, e0-repeatability]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: record skeleton and ds_text (02-06), content id (02-07), seeded power-on and placeholder stepping (02-12), state digest and profile identity hash (02-13), run_controls and the checker skeleton (02-02)
provides:
  - full accept record (ds_text_record): profile sha256, withheld ids, power_on, machine with time_unit placeholder-cycles and the state digest; 61 rows
  - refusal record (ds_text_refusal): record, core, refusal tables and no machine table; hostile ids refused at the printer and the buffer wiped
  - runner kinds record_golden, record_refusal, record_refuses_quote; flags --records DIR and --golden-dir DIR; request value prefix @hex:
  - two goldens (accept_384k_seeded, accept_256k_zero) cross-checked with the host sha tool over independently rebuilt streams
  - scripts/record-lint with REC_TABLES REC_FIELDS REC_PATH REC_POINTER REC_TIMESTAMP, seven controls, stages record-lint and record-lint-negatives (33 stages), selftest mutation-record-lint-pointer
affects: [02-15, 02-16 (determinism-run reads the --records directory and record-lint), later phases that add a key or a table to the record]

actuals:
  tokens: 22578    # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 90313 chars, 30 files, 1905 added and 25 removed lines)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 703a2a03b3db01570bae0d6de03ef151be02566f
plan_head_after: 92876b5d4792bbc543486012616f5d28f1d7d6c2
commits: 3

tech-stack:
  added: []
  patterns:
    - "records render to a buffer and are compared with a golden read through ISO C file I/O after the golden's leading comment block; the golden is generated once from the implementation and then verified by a separate perl script over independently rebuilt streams"
    - "a failed print wipes the part of the buffer it wrote, not only its first byte: an error emits nothing"
    - "record-lint reuses toml-subset.awk unchanged as stage 1; its 13 field names are cut out of profile-rules.awk so the list has one source"
    - "the record-lint stage derives the record-producing case ids from the kind column of build/gate/cases.tsv and never from a literal count"

key-files:
  created:
    - tests/conformance/reckinds.c
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/cases/record_golden_384k_seeded.toml
    - tests/conformance/cases/record_golden_256k_zero.toml
    - tests/conformance/cases/record_refusal_text.toml
    - tests/conformance/cases/record_refuses_quote.toml
    - scripts/record-lint
    - scripts/lib/record-rules.awk
    - tests/gate/record-lint/ (7 case directories)
  modified:
    - src/text/text.c
    - src/text/ds_text.h
    - src/core/ds_core.h
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - scripts/conformance-run
    - scripts/check
    - CMakeLists.txt

key-decisions:
  - "the seed and the machine time are printed as unsigned decimal integers (put_u64), so a seed at or above 2^63 is printed in full; the D-18 subset reads any digit string, while a strict TOML reader would limit integers to 64-bit signed"
  - "power_on.seed_present is a TOML boolean (the subset has type b) and policy_version is the integer DS_POWERON_POLICY_VERSION (1), added to ds_core.h"
  - "a refusal record prints the requested id from the request; ids of 64 characters or more, or with a double quote, a backslash or a non-ASCII byte, are DS_TEXT_ERR_STRING (D-69), so a very long hostile id yields no record at all"
  - "ds_text_profile now carries the profile sha256 (the profile identity text is the [profile] table with its hash plus the 13 field tables), and the profile text is followed by [withheld] instead of [content]"

patterns-established:
  - "a record-producing case kind writes DIR/ID.record.toml through record_emit; kind accept re-boots through record_for_accept; the refusal printer text is also put into the case evidence file"

requirements-completed: [PROF-03, CORE-03, CONT-04]

coverage:
  - id: D1
    description: "Every accept record carries the resolved profile identity (id, rev, resolved sha256, 13 field tables), the withheld ledger ids, content identity, power-on policy, version and seed, the cap, and machine time labelled placeholder-cycles with the state digest; the printed record equals a committed golden byte for byte"
    requirement: PROF-03
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance PASS; RESULT record_golden_384k_seeded pass assertions=5, record_golden_256k_zero pass assertions=5, record_skeleton_256k pass assertions=73 (61-row skeleton); cross-check script printed MATCH for profile.sha256, content.sha256, machine.digest, withheld.ids and content.size on both goldens"
        status: pass
    human_judgment: false
  - id: D2
    description: "A refusal produces a refusal record (record, core, refusal) and no machine table"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "RESULT record_refusal_text pass assertions=16; the case evidence file holds [refusal] once and [machine] zero times"
        status: pass
    human_judgment: false
  - id: D3
    description: "Printed strings contain no double quote or backslash: the printer returns an error status, an empty text and a zero length for a hostile requested id, and writes nothing"
    requirement: CONT-04
    verification:
      - kind: integration
        ref: "RESULT record_refuses_quote pass assertions=27 (quote, backslash and non-ASCII ids built with @hex:, plus a 64-character id and a clean control id); with the quote and backslash checks removed from put_quoted the case was guest_failure (scratch mutation, restored)"
        status: pass
    human_judgment: false
  - id: D4
    description: "record-lint rejects paths, pointers, timestamps and structural gaps, each rule has a named red control, and the pointer rule is mutation-proven"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "CHECK record-lint-negatives PASS (7 controls), CHECK record-lint PASS; SELFTEST mutation-record-lint-pointer PASS; scratch deletion of each of REC_PATH, REC_TIMESTAMP, REC_FIELDS, REC_TABLES turned its control broken (exit 1); a dead stage 1 gives NO_SENTINEL and a dead stage 2 gives STAGE2_DEAD"
        status: pass
    human_judgment: false

duration: 75 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 14: Run record, goldens, refusal record and record-lint Summary

**The run record is now complete and byte-comparable: an accept record of 61 rows (profile sha256, withheld ledger ids, power-on policy and seed, machine time labelled placeholder-cycles, state digest), a refusal record with no machine table, two goldens compared byte for byte and cross-checked with the host sha tool, a printer that refuses a quote or a backslash and wipes its buffer, and record-lint with seven controls and a mutation-proven pointer rule.**

## Performance

- **Duration:** about 75 min (2026-11-10)
- **Tasks:** 3 of 3 (Task 1 tracer, Tasks 2 and 3 auto with tdd)
- **Files:** 30 changed outside .planning (1905 added lines, 25 removed)
- **Gate cost (this machine, warm):** `sh scripts/check` about 63 s wall (33 stages, all PASS); `sh scripts/check --selftest` ends `SELFTEST RESULT PASS` with 13 lines.

## Accomplishments

- **Tracer (Task 1).** `text.c` prints, in order, the tables `record`, `core`, `profile` (id, rev, sha256 from `ds_profile_hash`), the 13 `field_NAME` tables, `withheld`, `content`, `power_on` (policy, policy_version, seed_present, seed), `limits` and `machine` (time, time_unit `placeholder-cycles`, digest_algorithm, digest from `ds_digest`). Every value comes from the instance through the core's accessors; there is no run-facts struct. The runner kind `record_golden` renders the record into a buffer (stdout carries only RESULT and RUNNER_DONE lines) and compares it byte for byte with `tests/conformance/golden/NAME.record.toml` after the leading comment block. The 384K seed-42 golden was generated once from the implementation and then verified by a throwaway perl script outside the repo (see Independent cross-check).
- **Withheld ids.** The set is the ledger ids of every run-request-bound field, the unresolved behaviours LED-IRQ-0001 LED-VID-0002 LED-VID-0003, and LED-LOAD-0003, LED-LOAD-0005 and LED-PROF-0002, sorted and de-duplicated (11 ids for the default request): `LED-AUD-0001 LED-IO-0001 LED-IRQ-0001 LED-LOAD-0001 LED-LOAD-0002 LED-LOAD-0003 LED-LOAD-0005 LED-PROF-0002 LED-VID-0001 LED-VID-0002 LED-VID-0003`.
- **Second golden, refusal record and hostile strings (Task 2).** `accept_256k_zero` (zero policy, `seed_present = false`, `seed = 0`); `record_refusal_text` (a 1 MiB image declared mcgenjin, refused `load.mapper_unsupported`: the record ends in the refusal table with reason, requested id and rev, and holds no profile, content, power_on or machine table); `record_refuses_quote` (three requests built with the new `@hex:` value prefix: a quote, a backslash and a non-ASCII byte each give a core refusal `load.profile_unknown`, and the printer returns `DS_TEXT_ERR_STRING`, an empty text, length 0, a buffer that holds no table header and no file; a clean id printed first and a 64-character id as a further control).
- **record-lint and stages (Task 3).** `scripts/record-lint` (file or directory arguments, `--expect` with the WR-05 guard, exit 0/1/2, marker `STATUS record-lint ok n=N`) over `toml-subset.awk` unchanged; `scripts/lib/record-rules.awk` carries REC_TABLES, REC_FIELDS, REC_PATH, REC_POINTER (the rule line carries the comment token `rule-pointer`), REC_TIMESTAMP, with no regex intervals. Seven controls under `tests/gate/record-lint`: `ok-accept`, `ok-refusal`, `path-in-value`, `pointer-in-value`, `timestamp-in-value`, `missing-field-table`, `missing-machine-table`, each negative with exactly one code in EXPECT. Stages `record-lint` and `record-lint-negatives` (33 stages). The `--records DIR` flag follows the single contract of D-86: every case of kind accept, record_lines, record_golden or record_refusal writes `DIR/ID.record.toml` (record text only), `record_refuses_quote` and every other kind write nothing, and `scripts/conformance-run` makes `build/gate/records` empty on every run and passes it.

## Independent cross-check of the goldens (method and results)

A throwaway perl script kept outside the repo (Digest::SHA and Math::BigInt, in the session scratch area) reads a candidate record, rebuilds from the written stream rules (not from the C code) the profile identity stream (`duospool-profile` plus NUL, version, id, rev, field count, name, status, value and bound_by of the 13 fields, then the sorted unresolved ids), the state stream (prefix, version, eight framed regions with the splitmix64 RAM, psg and vdc values of plan 12 for seeded runs), and the withheld set (union, sorted, de-duplicated). The content hash was taken from the host `shasum -a 256` over the bytes of `ds_runner --emit RECIPE`, which shares no code with the in-house SHA-256. Results, each line compared with the candidate record:

| Golden | profile.sha256 | content.sha256 (host shasum) | machine.digest | withheld.ids | content.size |
|--------|----------------|------------------------------|----------------|--------------|--------------|
| accept_384k_seeded (seed 42, seeded-prng-v1) | MATCH `aa7e8b34...ae41` | MATCH `dea83b86...37e0` | MATCH `8016b1cd...202d` | MATCH | MATCH 393216 |
| accept_256k_zero (policy zero) | MATCH `da2b433d...1bf0` | MATCH `cb038cda...0287` | MATCH `b1a4d2b6...166b` | MATCH | MATCH 262144 |

Both runs ended `CROSSCHECK ok`. The goldens' comment blocks label them as E0 repeatability records. A matching hash shows repeatability, and agreement with an independent re-derivation of the written rules, never that the machine behaviour is right.

## Verification Results

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| Task 1: `sh scripts/check --stage conformance && sh scripts/check --stage sanitizers` | exit 0, `CHECK conformance PASS`, `CHECK sanitizers PASS`; `RESULT record_golden_384k_seeded pass assertions=5` in both presets, no compiler warning under `-Werror` |
| Task 2: `sh scripts/check --stage conformance` | exit 0, `CHECK conformance PASS`; `record_golden_256k_zero pass 5`, `record_refusal_text pass 16`, `record_refuses_quote pass 27` |
| Task 3: `sh scripts/check --stage conformance && ... --stage record-lint-negatives && ... --stage record-lint && ... --selftest mutation-record-lint-pointer` | all four PASS (`SELFTEST mutation-record-lint-pointer PASS`), no `STAGE2_DEAD` or `CHECKER_DEAD` |

Acceptance criteria, all run:

- Task 1: `grep -c '^\[field_' golden` = 13; `grep -c behaviour_epoch golden` = 1; `grep -cE 'tmp|home|Users'` on both goldens = 0; `head -3` holds `SPDX-License-Identifier: 0BSD`. One digit of the golden edited (`seed = 42` to `seed = 43`, run once, restored and confirmed with `cmp`): `RESULT record_golden_384k_seeded guest_failure`.
- Task 2: the three new ids pass; `grep -c '\[refusal\]'` on the evidence file prints 1 and `grep -c '\[machine\]'` prints 0; second golden header holds `SPDX-License-Identifier: 0BSD`.
- Task 3: `ls tests/gate/record-lint | wc -l` = 7, each negative control names one code. After `rm -rf build/gate/records`, `sh scripts/check --stage conformance` recreated it and `sh scripts/check --stage record-lint` printed `CHECK record-lint PASS`. The files in `build/gate/records` and the rows of `build/gate/cases.tsv` whose kind is accept, record_lines, record_golden or record_refusal are both 6 (accept_tracer_256k, record_content_fields, record_golden_256k_zero, record_golden_384k_seeded, record_refusal_text, record_skeleton_256k); every name ends `.record.toml`; the `record_refuses_quote` case has no file. Removing `record_refusal_text.record.toml` before the stage made it print `RECORDS_COUNT_MISMATCH files=5 cases=6` and `RECORD_MISSING record_refusal_text` with `CHECK record-lint FAIL` (observed once, then restored by rerunning conformance). `sh scripts/check` printed 33 `CHECK <stage> PASS` lines and ended `CHECK RESULT PASS`. `head -5` of `scripts/record-lint` and `scripts/lib/record-rules.awk` holds `SPDX-License-Identifier: MIT OR Apache-2.0`.
- TDD red states observed: with the quote and backslash tests removed from `put_quoted`, `record_refuses_quote` was `guest_failure` (restored, `cmp`-verified); with each of the REC_PATH, REC_TIMESTAMP, REC_FIELDS and REC_TABLES rule lines deleted from a scratch copy of scripts, the matching control exited 1 (broken as wanted); a stage 1 that prints nothing gave `NO_SENTINEL`, and a stage 2 that prints nothing gave `STAGE2_DEAD`.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 33 stage lines PASS, ends `CHECK RESULT PASS` (about 63 s) |
| `dash scripts/check` | 33 stage lines PASS, ends `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | `SELFTEST mutation-record-lint-pointer PASS` among the 13 lines, then `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range 703a2a0..HEAD` | `audit-history: clean` (3 commits, 31 blobs) |

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 fix was needed in committed code.

### Interpretation choices (inside the plan's intent)

1. **Plumbing of Tasks 2 and 3 landed with Task 1's commit.** The runner kinds `record_refusal` and `record_refuses_quote`, the `@hex:` prefix, the `--records` flag, the `record_emit` calls and the `conformance-run` changes were written while the tracer was proven and are in `3dc215f`; Task 2's commit adds the cases, the second golden and the REQUIRED lines, and Task 3's commit adds the lint, the controls, the stages and the selftest. Each commit passes the gate on its own tree. `REQUIRED` was committed with only the first new id in Task 1.
2. **Files beyond `files_modified`.** `tests/conformance/reckinds.c` (a new kind family, because `runner.c` is 64781 of 65536 bytes), `tests/conformance/runner.h`, `src/text/ds_text.h` (the refusal printer's declaration and the record description), `CMakeLists.txt` (one source joined the runner list) and the four case files.
3. **The golden directory is a runner flag, `--golden-dir DIR`.** The plan names the golden path but not how the runner finds it; `scripts/conformance-run` passes `tests/conformance/golden` to the real runner and to the toy runners (which do not get `--records`). The case `expect` holds the golden name (for example `accept_384k_seeded`), not a path.
4. **`ds_text` includes `ds_serial.h` and `ds_poweron.h` besides `ds_core.h`.** `ds_digest` and `ds_profile_hash` are declared in `ds_serial.h` and the policy enum in `ds_poweron.h`; `text.c` already included `ds_mapper.h` for `ds_size_bytes`. No struct repackages the run facts (D-86 holds). The accessor-only reading of the artifacts note was kept for values: the only getter-like addition is `DS_POWERON_POLICY_VERSION` in `ds_core.h`.
5. **`ds_text_refusal` is a new public printer function** (`reason`, `requested_id`, `requested_rev`), because a refusal has no instance to print. `finish` now wipes the bytes a failed print wrote, so "emit nothing" holds for the whole buffer; plan 06's tests only required the first byte to be NUL.
6. **The refusal text is also printed into the case evidence file** (a `record:` block after the standard lines) through `set_evidence_detail`, so the acceptance grep on the evidence file can see `[refusal]` and not `[machine]`.
7. **record_refuses_quote holds three requests, not two** (quote, backslash, non-ASCII byte), and checks a clean id and a 64-character id as controls.
8. **record-lint codes beyond the five of the plan.** The codes of `toml-subset.awk`, plus `EMPTY_FILE`, `FILE_TOO_BIG` (16 KiB, the record cap), `BAD_FILENAME`, `NO_SENTINEL` and `STAGE2_DEAD`, as `profile-check` has them. A directory without a `.toml` file is a harness error (exit 2).
9. **The record-lint stage adds its own failure lines** (`RECORDS_DIR_EMPTY`, `RECORDS_COUNT_MISMATCH`, `RECORD_MISSING`, `NO_RECORD_CASES`) and returns harness status 2 when `build/gate/records` or `build/gate/cases.tsv` is absent, so it never passes on a missing input.
10. **`--records` also applies in sanitize mode** (the directory is `build/sanitize/records`); the record-lint stage reads only `build/gate/records`.
11. **PROF-03, CORE-03 and CONT-04** were reported ready by `requirements.ready-ids` (no later plan of this phase declares them) and are marked complete in REQUIREMENTS.md.

**Total deviations:** 0 auto-fixed, 11 interpretation choices. **Impact:** none on later plans except the notes below.

## Notes for later plans

- **The pinned record row count is 61** (`build_expected_keys` and the `n_exp == 61u` assertion in `runner.c` kind `record_lines`): format_version 1, core 2, profile 3, field 39, withheld 1, content 6, power_on 4, limits 1, machine 4. A plan that adds a key or a table edits both, the two goldens, `record-rules.awk` and the record-lint controls in the same commit, and bumps `DS_TEXT_FORMAT_VERSION`.
- **Label for machine time:** the record carries `time_unit = "placeholder-cycles"` (plan 12's note); nothing in it is a hardware claim. The digest is repeatability evidence only.
- **The digest does not cover `mapper`, `byte_order` or `header`** (plan 13's note); the record prints all three under `[content]` but they cannot differ between two accepted runs today.
- **Plan 16 (determinism-run):** run `ds_runner` with `--records DIR`; the directory holds exactly one `ID.record.toml` per row of kind accept, record_lines, record_golden or record_refusal in `build/gate/cases.tsv` (6 today); `scripts/record-lint DIR` lints it and exits with the marker `STATUS record-lint ok n=N`.
- **runner.c is 64781 of 65536 bytes.** Any new kind goes in its own file (as `reckinds.c`).
- **A very long hostile requested id** (64 characters or more) yields no refusal record at all (DS_TEXT_ERR_STRING); a host that must still report the refusal prints the reason name alone.
- **Goldens compare the default build id `unset`.** A build that sets `DS_BUILD_ID` makes the record golden cases fail, by design (D-53); goldens compare `behaviour_epoch`, which is part of the bytes.

## Task Commits

1. **Task 1 (tracer): full seeded record rendered to a buffer and compared with a golden** - `3dc215f` (feat)
2. **Task 2: second golden, refusal record text and hostile-string refusal cases** - `9bc957f` (feat)
3. **Task 3: record-lint with seven controls, two stages and the pointer-rule mutation** - `92876b5` (feat)

## Known Stubs

None. `fixture = "none"` is not used by the new cases, and no hardcoded empty value flows to a consumer. The `seed = 0` of the zero-policy golden is the real value of an unseeded run (`seed_present = false`).

## Threat Flags

None. No network endpoint, auth path or schema change at a trust boundary was added. T-02-25 is mitigated: the record is printed through a bounded appender with no pointer, padding, wall time or path, goldens are compared byte for byte, the printer refuses a quote, a backslash or a non-ASCII byte and wipes the buffer, and record-lint (REC_PATH, REC_POINTER, REC_TIMESTAMP) has planted controls with a mutation-proven pointer rule. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `tests/conformance/reckinds.c`, both goldens, the four case files, `scripts/record-lint`, `scripts/lib/record-rules.awk`, the seven `tests/gate/record-lint/*/record.toml`.
- Commits `3dc215f`, `9bc957f` and `92876b5` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 3.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
