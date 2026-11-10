---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 08
subsystem: core
tags: [c11, mapper, size-enum, mirror-tables, bank-signatures, awk-oracle, conformance-gate, ledger]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: ds_boot with the fixed ROM copy and the SHA-256 content id (02-05, 02-07); the runner, its case kinds and the len:N recipe (02-06, 02-07); conformance-run run mode (02-05)
provides:
  - src/core/ds_mapper.h and mapper.c, the size enum (DS_SIZE_256K, DS_SIZE_384K, DS_SIZE_1M), one size-indexed mirror table, ds_size_classify, ds_size_bytes and ds_mapper_read with DS_MAP_OK, DS_MAP_UNMAPPED and DS_MAP_BAD_OFFSET
  - ds_boot accepts exactly the three enum sizes and stores the enum (size_class) in the instance, never the raw length; the record derives content.size from the enum
  - synthetic recipes plain-256k, plain-384k, plain-1m carrying the bank tag bytes ((k*37+11) mod 256 first, (k*91+5) mod 256 last)
  - tests/conformance/mirror-oracle.awk, an independent bank-signature oracle with its own arithmetic and tag formula and the ORACLE_RULE_TEXT_DRIFT guard
  - runner flag --oracle and case kinds banksig and unmapped; conformance-run writes the 384-row oracle.tsv before the runner starts
  - cases accept_256k_banksig, accept_384k_banksig, accept_1m_banksig, accept_unmapped_banks (all in REQUIRED; 20 cases in all)
  - ledger entries LED-LOAD-0003 and LED-LOAD-0005 (assumed, live pins), both listed in ledger/REQUIRED
affects: [02-09 (wrong-mirror RULE anchors the 384K line recorded below; size refusal matrix; LED-LOAD-0004), 02-10 (sanitizer stage reruns the cases), 02-12 (power_on table), 02-14 (record golden: content.size is unchanged in the record), 02-17 (fixture manifest: plain-* recipes now carry tag bytes, len:N is still the pure stream), Phase 3 (owns the value of an unmapped read)]

actuals:
  tokens: 9521     # chars/4 over the realized diff (git diff 74098e5..HEAD excluding .planning, 38083 chars, 18 files, 521 added and 18 removed lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 74098e57bc738ae2eafe16a4cbd8200b1a52f57e
plan_head_after: b65fca1869c763b562c1735dfcec1b47625f213f
commits: 4

tech-stack:
  added: []
  patterns:
    - "the size enum is the only key of the mirror tables; the single place a raw length meets the enum is ds_size_classify, called by the supported-size check and by the instance fill"
    - "the expectation comes from a script in another language that shares nothing with the implementation, and the script refuses to run when the ledger text it mirrors changes"
    - "a status that reports no value (unmapped, bad offset) must not write the output byte: the case keeps a sentinel and checks it, which also shows the ROM array was not read"

key-files:
  created:
    - src/core/ds_mapper.h
    - src/core/mapper.c
    - tests/conformance/mirror-oracle.awk
    - tests/conformance/cases/accept_256k_banksig.toml
    - tests/conformance/cases/accept_384k_banksig.toml
    - tests/conformance/cases/accept_1m_banksig.toml
    - tests/conformance/cases/accept_unmapped_banks.toml
    - ledger/entries/LED-LOAD-0003.toml
    - ledger/entries/LED-LOAD-0005.toml
  modified:
    - src/core/boot.c
    - src/core/boot_order.def
    - src/core/ds_core.h
    - src/text/text.c
    - tests/conformance/synth.c
    - tests/conformance/runner.c
    - tests/conformance/REQUIRED
    - scripts/conformance-run
    - ledger/REQUIRED

key-decisions:
  - "ds_instance.content_size became size_class (the enum); the record's content.size is ds_size_bytes(size_class), so the printed record is unchanged and a state with no valid size class makes the printer refuse (DS_TEXT_ERR_STATE) instead of printing 0"
  - "ds_mapper_read checks the bank before the offset: bank 0x80 and above is DS_MAP_UNMAPPED whatever the offset, and DS_MAP_BAD_OFFSET is only reported for a mapped bank; a NULL instance, a NULL output or an instance with no size class is reported DS_MAP_UNMAPPED and writes nothing"
  - "the oracle guard accepts a clause only when the next character in the statement is one of ; . : or the closing quote, so a reworded tail such as '1M file bank = b mod 5' is drift, not a match"
  - "the oracle size selecting the rows is the natural size of the fixture recipe (synth_size), never a value the core reports"

patterns-established:
  - "banksig and unmapped kinds: banksig reads --oracle FILE (rows SIZE BANK FILEBANK FIRST LAST) and makes two assertions per bank; unmapped makes 393 assertions over the three recipes and needs no oracle"
  - "plain-* recipes carry tag bytes, len:N never does; a pinned literal must use len:N"

requirements-completed: [CONT-01]

coverage:
  - id: D1
    description: "Each supported plain size (256 KiB, 384 KiB, 1 MiB) reads the oracle's first and last bank-signature byte through all 128 banks of the window via the mapper boundary"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance -> CHECK conformance PASS; RESULT accept_256k_banksig, accept_384k_banksig, accept_1m_banksig each pass assertions=256; RUNNER_DONE 20; a scratch oracle with b mod 31 made accept_256k_banksig guest_failure, reverted"
        status: pass
    human_judgment: false
  - id: D2
    description: "Banks 0x80 and above are unmapped and offset 8192 is a bad offset for all three sizes, with no byte written and no ROM read"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "RESULT accept_unmapped_banks pass assertions=393"
        status: pass
    human_judgment: false
  - id: D3
    description: "The oracle is independent and guarded: it carries its own arithmetic and tag formula and prints ORACLE_RULE_TEXT_DRIFT (exit 1, conformance stage FAIL) when the ledger rule text changes"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "scratch ledger copy with 'b mod 32' changed to 'b mod 31' and the real LED-LOAD-0003 with '0x40 or above' changed to '0x41 or above': ORACLE_RULE_TEXT_DRIFT, exit 1, stage FAIL; both reverted, stage PASS"
        status: pass
    human_judgment: false
  - id: D4
    description: "ds_boot accepts exactly the three enum sizes; 8192, 512 KiB and 768 KiB are size_unsupported and 1 MiB plus 8 KiB is too_large"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "a scratch refuse/accept run of ds_runner over len=8192, 524288, 786432 (size_unsupported), 262144, 393216, 1048576 (accept) and 1056768 (too_large): all seven pass; the committed size matrix is plan 09's"
        status: pass
    human_judgment: false
  - id: D5
    description: "LED-LOAD-0003 and LED-LOAD-0005 exist as assumed entries pinned to accept_384k_banksig and accept_unmapped_banks, and neither is promoted"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage ledger-entries -> CHECK ledger-entries PASS (13 keys each, REQUIRED lines equal the origin text)"
        status: pass
    human_judgment: false

duration: 8 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 08: Plain-HuCard mapper boundary with an independent oracle Summary

**The plain mapper now has a size enum with one size-indexed mirror table (256K b mod 32, 384K and 1M rules, banks 0x80 and above unmapped), and an awk oracle that shares nothing with the core and refuses to run when the ledger rule text drifts proves the documented first and last byte of all 128 banks for all three sizes.**

## Performance

- **Duration:** about 8 min (2026-11-10T05:10Z to 05:18Z)
- **Tasks:** 3 of 3 (Task 1 is the tracer, Task 2 is TDD)
- **Files:** 18 changed outside .planning (521 added lines, 18 removed)

## Accomplishments

- **Mapper boundary (D-44).** `src/core/mapper.c` holds one table indexed by `enum ds_size`: byte size plus the function that maps a bank index to a file bank. `ds_size_classify` is the only function that takes a raw length; `ds_mapper_read(inst, bank, offset, out)` returns `DS_MAP_UNMAPPED` for bank 0x80 and above, `DS_MAP_BAD_OFFSET` for offset 8192 and above, and otherwise reads at an index that is checked against both the image size and the 1 MiB array before the read (T-02-11).
- **Boot.** `check_supported_size` is `ds_size_classify(len) == DS_SIZE_NONE`, so the supported set is exactly the three enum members; the raw-length constant `DS_SUPPORTED_SIZE` is gone. The instance keeps `size_class`; the record prints `content.size` from the enum.
- **Independent oracle (D-59, T-02-14).** `tests/conformance/mirror-oracle.awk` (POSIX awk, no bit operations) prints rows `SIZE BANK FILEBANK FIRST LAST` with its own arithmetic and its own tag formula. `conformance-run` runs it for 256, 384 and 1024 into `build/PRESET/oracle.tsv` (384 rows, counted) before the runner starts, and the runner reads it with `--oracle`. Before printing it reads the statements of LED-LOAD-0003 and LED-LOAD-0005 and requires the four rule clauses literally (ORACLE_RULE_TEXT_DRIFT otherwise).
- **Cases.** `accept_{256k,384k,1m}_banksig` (kind `banksig`, 256 assertions each: two per bank, status and value together) and `accept_unmapped_banks` (kind `unmapped`, 393 assertions: 128 banks x 3 sizes, bank 0xFFFFFFFF, offset 8192 and 0xFFFFFFFF, each with an unchanged sentinel byte).
- **Ledger (D-85).** LED-LOAD-0003 (the three mirror clauses in one sentence, withholds any claim that the rule matches hardware, revisit phase 6) and LED-LOAD-0005 (window $00-$7F, banks 0x80 and above not ROM, withholds the value of an unmapped read, revisit phase 3), both `assumed`, evidence empty, listed in `ledger/REQUIRED` as `LED-LOAD-0003 02-CONTEXT D-44` and `LED-LOAD-0005 02-CONTEXT D-44`. No CPU exists, so nothing here can reach E2; both stay assumed.

## Notes for plan 09 (wrong-mirror RULE anchor)

- **The 384K mirror expression is one line: `src/core/mapper.c` line 26.** Anchor substring, literal and unique in the tree: `return b < 0x40 ? (b & 0x1F) : (0x20 + (b & 0x0F));`. `grep -n '0x20' src/core/mapper.c | grep -c '0x0F'` prints 1 (baseline before Task 2: 0). The literals 0x1F, 0x40, 0x20 and 0x0F appear on that line only.
- **Supported-size check:** `src/core/boot.c` line 231, `return ds_size_classify(c->req->len) == (uint32_t)DS_SIZE_NONE ? DS_REFUSE_size_unsupported : DS_OK;`; the only reference to `size_unsupported` in boot.c, and it names the enum classification, not a raw length. `ds_size_classify` is called again at line 285 to fill `inst->size_class` (the fill runs only after every check passed).
- The case that must go red under the wrong-mirror toy (384K as `b & 0x3F`) is `accept_384k_banksig`.
- `runner.c` is 61016 bytes (limit 65536); plan 09 has about 4 KB before it must split a kind file out as plan 07 did.
- `plain-256k` bytes changed (tag bytes). Nothing pins its hash: `record_content_fields` uses `len:262144`. `len:N` is unchanged.

## Task Commits

1. **Task 1 (tracer): a 256K image through ds_boot, the size-enum mapper and the independent oracle** - `ceb4531` (feat)
2. **Task 2 RED: 384K and 1M bank signature cases, red on the 256K-only core** - `68a3afa` (test)
3. **Task 2 GREEN: 384K and 1M mirror rows; the supported sizes are exactly the three enum members** - `743022a` (feat)
4. **Task 3: unmapped banks, the oracle rule-text guard and ledger entries LED-LOAD-0003 and LED-LOAD-0005** - `b65fca1` (feat)

## Verification Results

Tracer gate (auto mode): after `ceb4531` the tracer verify `sh scripts/check --stage conformance` was re-run end to end: exit 0, `CHECK conformance PASS`, `RESULT accept_256k_banksig pass assertions=256`, `RUNNER_DONE 17`. Log line `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance` (Task 1) | exit 0, `CHECK conformance PASS`, `RESULT accept_256k_banksig pass assertions=256`, `RUNNER_DONE 17` |
| `sh scripts/check --stage conformance` (Task 2 RED, before `68a3afa`) | `CHECK conformance FAIL`; `accept_384k_banksig` and `accept_1m_banksig` `harness_failure` ("the request was refused: this kind needs an accepted boot"), `accept_256k_banksig pass`, `RUNNER_DONE 19` |
| `sh scripts/check --stage conformance` (Task 2 GREEN) | exit 0, `CHECK conformance PASS`; 256k, 384k and 1m banksig each `pass assertions=256`, `RUNNER_DONE 19` |
| `sh scripts/check --stage conformance && sh scripts/check --stage ledger-entries` (Task 3) | both PASS; `RESULT accept_unmapped_banks pass assertions=393`, `RUNNER_DONE 20`; no ORACLE_RULE_TEXT_DRIFT on the unmodified tree |

Acceptance criteria, all run:

- Task 1: `accept_256k_banksig pass` with `assertions=256` and `RUNNER_DONE 17` (PASS). Scratch oracle with `b mod 31` for 256K: `RESULT accept_256k_banksig guest_failure assertions=256`, file restored from a backup copy and the stage re-run green (PASS, observed once). `git ls-files | grep -E '\.(bin|pce)$'` counts 0 (PASS). `sh scripts/check --stage licences` PASS; `head -3` of `src/core/mapper.c`, `src/core/ds_mapper.h` and `tests/conformance/mirror-oracle.awk` each carries `SPDX-License-Identifier: MIT OR Apache-2.0` (PASS).
- Task 2: `accept_384k_banksig` and `accept_1m_banksig` `pass assertions=256`, `RUNNER_DONE 19` (PASS). `grep -n '0x20' src/core/mapper.c | grep -c '0x0F'` prints 1 and the line also holds 0x1F and 0x40 (PASS). `grep -n size_unsupported src/core/boot.c` shows only line 231, the enum classification (PASS). The head of both new case files holds the 0BSD header (PASS).
- Task 3: `accept_unmapped_banks pass`, `RUNNER_DONE 20` (PASS). Drift control: a copy of the ledger entries with `b mod 32` changed to `b mod 31` made the oracle print `ORACLE_RULE_TEXT_DRIFT file=... clause=256K file bank = b mod 32` and exit 1 (PASS); the real `LED-LOAD-0003.toml` with `0x40 or above` changed to `0x41 or above` made `sh scripts/check --stage conformance` print `CHECK conformance FAIL` with `ORACLE_RULE_TEXT_DRIFT`, then the file was restored from a backup copy and the stage passed again (PASS); a tail change `1M file bank = b mod 5.` also gave drift (the terminator rule). `grep -cxF` for both REQUIRED lines prints 1 each; `grep -c 'status = "assumed"'` prints 1 on each entry; for both entries each of the 13 keys matches exactly one `^KEY = ` line; `sh scripts/check --stage ledger-entries` PASS; both entries start with the 0BSD header (all PASS).

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, ends `CHECK RESULT PASS` (conformance PASS) |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range 74098e5..HEAD` | `audit-history: clean` (4 commits, 24 blobs) |
| `sh scripts/conformance-run run --preset gate-gcc` and `--preset gate-clang` | `STATUS hash-sweep ok lengths=21` and `STATUS conformance ok n=20` for both |

A scratch run (files outside the repo, not committed) of seven refuse and accept requests over the new core: `len=8192`, `524288`, `786432` gave `load.size_unsupported`, `262144`, `393216`, `1048576` were accepted with the right size, `1056768` gave `load.too_large`; all seven passed.

## Files Created/Modified

See `key-files`. New `.c`, `.h` and `.awk` files carry the MIT OR Apache-2.0 header, the four new case files and both ledger entries carry 0BSD.

## Decisions Made

See `key-decisions`. The plan fixed the enum, the rule clauses, the tag formula, the flag and kind names and the case ids; the choices left open were the status order in `ds_mapper_read`, how the instance and record carry the size, and how strict the drift guard is.

## Deviations from Plan

None - plan executed exactly as written, with the interpretation choices below.

### Interpretation choices (inside the plan's intent)

1. **TDD RED for Task 2 is `harness_failure`, not `guest_failure`.** The 256K-only core refuses 384K and 1M at boot, and the banksig kind needs an accepted boot, so the red state is a harness failure ("the request was refused"). It shows the cases are required, judged and not vacuous. The plan's "watch them fail on the 256K-only core" is met; the outcome class differs from the wrong-mirror guest failure that plan 09 will show.
2. **`ds_instance.content_size` was replaced by `size_class`** (plan: "stored in the instance instead of the raw size"). `src/text/text.c` and the `has_size` check in `runner.c` derive the byte size from the enum through `ds_size_bytes`; the printed record is byte-identical (record_skeleton_256k 73 and record_content_fields 70 assertions, unchanged).
3. **The 1M banksig case carries `pins = []`;** only `accept_384k_banksig` (LED-LOAD-0003) and `accept_unmapped_banks` (LED-LOAD-0005) pin a ledger entry, as the plan lists.
4. **The REQUIRED lines are inserted after `LED-LOAD-0002`,** keeping `ledger/REQUIRED` sorted, instead of at the end of the file; content and equality with the entries' `origin` are as specified.
5. **The oracle guard is stricter than a substring match** (the next character after the clause must be `;`, `.`, `:` or the closing quote). The plan says "literally"; a reworded tail would otherwise pass.
6. **`accept_unmapped_banks` asserts more than the plan lists:** bank 0xFFFFFFFF, the largest offset 0xFFFFFFFF and the unchanged sentinel byte on every status (393 assertions), because T-02-11 is about never reading or writing past the array.
7. **`synth_spec()` is unchanged** (the LCG spec string). The tag formula is documented in `synth.c` and in `mirror-oracle.awk` and belongs to the `plain-*` recipe names; `len:N` has no tags. Plan 17 hashes the spec with the bytes and should treat the recipe name as part of the key.

**Total deviations:** 0 auto-fixed, 7 interpretation choices. **Impact:** none on later plans' contracts beyond the notes for plan 09 above.

## Known Stubs

None. No placeholder, hardcoded empty value or TODO was added to the files of this plan.

## Threat Flags

None. No new network endpoint, file access or trust boundary. T-02-11 is mitigated as planned (table indexed by the enum, unmapped and bad-offset statuses before any array access, an index check against the image and the array, `accept_unmapped_banks` over all three sizes; the sanitizer rerun is plan 10's). T-02-14 is mitigated (independent awk oracle, the drift guard observed red twice; the wrong-mirror toy is plan 09's). T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist: `src/core/ds_mapper.h`, `src/core/mapper.c`, `tests/conformance/mirror-oracle.awk`, `tests/conformance/cases/accept_256k_banksig.toml`, `accept_384k_banksig.toml`, `accept_1m_banksig.toml`, `accept_unmapped_banks.toml`, `ledger/entries/LED-LOAD-0003.toml`, `ledger/entries/LED-LOAD-0005.toml` (checked with `[ -f ]`).
- Commits `ceb4531`, `68a3afa`, `743022a`, `b65fca1` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 4.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
