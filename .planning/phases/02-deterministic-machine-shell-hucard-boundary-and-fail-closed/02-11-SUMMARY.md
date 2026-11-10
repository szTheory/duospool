---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 11
subsystem: conformance
tags: [refusal-matrix, precedence, non-mutation, no-heuristic, redaction, toy-control, deferred-kind, ledger-pins]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: the core, the request grammar and the two-pass runner dispatch (02-05); the plain mapper and size enum (02-08); the toy-control mechanism and ref_size rows (02-09); the sanitize preset, allocation matrix and mode exclusion (02-10)
provides:
  - 48 new case files, 73 in tests/conformance/cases and 73 ids in tests/conformance/REQUIRED (39 refusal rows, precedence, coverage, non-mutation, three no-heuristic, copy-in, two redaction)
  - runner kinds precedence, coverage (deferred, D-86), nonmutation, noheur, copy_in, redaction and redaction_leak_control; runner flags --only ID and --marker-seed N
  - one output function (emit_out and emit_err) with an in-process log that the no-leak scanner reads
  - tests/controls/heuristic-reverse (RULE and EXPECT, generated from the live boot.c), CONTROL heuristic-reverse RED_OK
  - selftest mutation-drop-precedence (two rows of boot_order.def swapped in a scratch build)
  - LED-LOAD-0001 pinned to TEST:ref_order_tg16 and LED-LOAD-0002 to TEST:ref_header_copier512, each with the PCEdev HuCard pinout REPORT source
  - mapkinds.c (banksig and unmapped moved out of runner.c), refkinds.c and redkinds.c
affects: [02-12 (power-on checks go into boot_order.def: the precedence table then needs one item more per added row, and the coverage kind needs refuse rows for the two new refusals), 02-13 (digest upgrade of the non-mutation compare), 02-15, 02-16 (every new runner invocation must pass --marker-seed)]

actuals:
  tokens: 30463    # chars/4 over the realized diff (git diff plan_head_before..HEAD excluding .planning, 121851 chars, 62 files, 1944 added and 184 removed lines)
  tasks: 6
  commits: 7       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: c6bffb91fa0891f2d2ce842c8f5b72b5d3d238ff
plan_head_after: c38f047290473f2f41deb032f5762cb49c07b270
commits: 7

tech-stack:
  added: []
  patterns:
    - "a case is a request-override string plus an expected load.NAME; a refusal matrix of 43 refuse rows is data, and every later consumer of the case inventory (allocation matrix, sanitize run, non-mutation matrix, coverage) widens by itself"
    - "an aggregate over other rows is a deferred kind: the refuse handler records what it saw, the second pass judges the set, so the verdict never depends on file order"
    - "a proof that something never appears is shown able to fail in both directions: a seeded leak is flagged and a clean copy is not, and a scratch leak into the real path turns the real case red"
    - "a toy is two files (RULE, EXPECT); heuristic-reverse anchors on the one live copy-in line of boot.c"

key-files:
  created:
    - tests/conformance/mapkinds.c
    - tests/conformance/refkinds.c
    - tests/conformance/redkinds.c
    - tests/controls/heuristic-reverse/RULE
    - tests/controls/heuristic-reverse/EXPECT
    - tests/conformance/cases/ (48 files: ref_mem_*, ref_request_*, ref_profile_rev_mismatch, ref_profile_mismatch, ref_open_unbound, ref_bind_*, ref_mapper_*, ref_order_*, ref_header_*, ref_empty_*, ref_buffer_null, ref_short_*, ref_not_bank_*, ref_size_128k/192k/768k/2m_too_large, ref_too_large_1m_plus_8k, ref_too_large_2_5m_plain, ref_precedence_table, ref_reason_coverage, nonmutation_matrix, noheur_*, boot_copy_in_isolated, redaction_*)
  modified:
    - CMakeLists.txt
    - scripts/check
    - scripts/conformance-run
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/allockinds.c
    - tests/conformance/REQUIRED
    - ledger/entries/LED-LOAD-0001.toml
    - ledger/entries/LED-LOAD-0002.toml

key-decisions:
  - "the precedence table is read from the request key and then the expect key: a case line is limited to 400 characters and 14 REQ>>CODE items do not fit in one"
  - "the precedence kind requires at least one item per adjacent pair of rows of boot_order.def (derived by including the def file), so a check added to the list cannot be added without a pair"
  - "coverage is judged against the refusal enum itself (members derived from ds_refusal_name), not against a hard-coded count, so plan 12 extends it by adding refusals and refuse rows only"
  - "every line the runner prints in run mode goes through emit_out or emit_err; the utility modes (--outcomes, --sweep-lengths, --hash, --emit, --spec) keep raw stdout because they never run cases"
  - "the redaction markers come only from --marker-seed; without it the kinds are a harness failure, never a pass, so a runner invoked without the seed cannot silently skip the scan"
  - "the mirrored-size bank reads of the noheur and copy_in kinds use the plain mapper identity range (all 128 banks for 1 MiB, the first 32 for 256K and 384K)"

patterns-established:
  - "a new refuse row is one TOML file and one REQUIRED line; the precedence mutation, coverage, allocation matrix, non-mutation matrix and sanitize run all pick it up"
  - "a new kind family is its own file and a line in the kinds table of runner.c; runner.c keeps the reader, the boot kinds and main (59967 bytes, limit 65536)"

requirements-completed: [CONT-01, CONT-02, CONT-03, PROF-02]

coverage:
  - id: D1
    description: "Each refusal-matrix input returns its own typed reason: the 39 new refuse rows plus the four earlier ones, all 20 enum members observed"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance PASS; RUNNER_DONE 73; 43 refuse-kind rows pass; RESULT ref_reason_coverage pass assertions=21 (members=20 observed=20)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The fixed precedence is table-tested pair by pair, including the cap before the bank-multiple and supported-size rules, and a swap of two rows of boot_order.def is shown red"
    requirement: CONT-01
    verification:
      - kind: integration
        ref: "RESULT ref_precedence_table pass assertions=71 (14 items, 14 rows); sh scripts/check --selftest mutation-drop-precedence -> SELFTEST mutation-drop-precedence PASS; the mutated scratch build gives guest_failure with failed_item=6 (mapper before byte order)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A refusal writes no byte of caller memory or of the guard canaries around it and disturbs no live instance"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "RESULT nonmutation_matrix pass assertions=44 (43 refuse rows plus the live block); a scratch write of one byte in the rev check turned nonmutation_matrix, ref_profile_rev_mismatch and ref_precedence_table guest_failure, then src/core/boot.c was restored"
        status: pass
    human_judgment: false
  - id: D4
    description: "The core never guesses: POPULOUS at 0x1F26, MCGENJIN at 0x1FD0 and a byte at 0x1FFF below 0xE0 all load as declared, and a toy with a bit-reversal heuristic generated from the live boot.c is shown red"
    requirement: CONT-02
    verification:
      - kind: integration
        ref: "RESULT noheur_populous_string, noheur_mcgenjin_string, noheur_reverse_vector, boot_copy_in_isolated pass; CONTROL heuristic-reverse RED_OK (toy gave guest_failure for noheur_reverse_vector, noheur_populous_string, noheur_mcgenjin_string); replace set equal to anchor gave CONTROL heuristic-reverse NOT_RED and the stage FAIL; SELFTEST mutation-control-anchor heuristic-reverse PASS"
        status: pass
    human_judgment: false
  - id: D5
    description: "ROM-internal strings never leave the process: run-time unique markers are absent from every runner output, the record and the evidence text, and the scanner is shown able to see a leak"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "RESULT redaction_synthetic_rom pass (markers=4 hits=0), RESULT redaction_seeded_leak_flagged pass (flagged=2, clean copy not flagged); a scratch emit_err of one marker turned redaction_synthetic_rom guest_failure, then redkinds.c was restored"
        status: pass
    human_judgment: false
  - id: D6
    description: "LED-LOAD-0001 and LED-LOAD-0002 point at cases that exist and carry the HuCard pinout REPORT source, still assumed"
    requirement: CONT-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage ledger-entries PASS; grep -c 'PENDING:2' prints 0 for both; grep -c 'status = \"assumed\"' prints 1 for both; both case files exist"
        status: pass
    human_judgment: false

duration: 18 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 11: Refusal matrix, fixed precedence, no-heuristic proofs and redaction Summary

**The full refusal matrix (43 refuse rows, every one of the 20 reasons observed) now runs against the core, the check order is table-tested and a row swap is shown red, no refusal touches a byte of caller memory, a bit-reversal toy generated from the live boot.c is shown red by the no-heuristic row, and run-time unique markers in a synthetic ROM never reach any runner output.**

## Performance

- **Duration:** about 18 min (2026-11-10T05:47Z to 06:06Z)
- **Tasks:** 6 of 6 (the five-task rule is waived by D-86); Task 1 is the tracer, the rest are auto; one extra refactor commit
- **Files:** 62 changed outside .planning (1944 added lines, 184 removed)
- **Gate cost (this machine, warm):** `sh scripts/check` 30.7 s wall, `sh scripts/check --timing` 34.1 s, against D-60's 60 s proposal: not exceeded. No stage is over 20 s. Dominant stages: sanitizers 5 s, protocol-negatives 4 to 6 s, audit-synthetic 4 s, rights-negatives 3 to 4 s, conformance 2 s. `sh scripts/check --selftest` 7.7 s (the new mutation builds a scratch tree twice and takes about 1.5 s).

## Accomplishments

- **Tracer and groups A to D (Tasks 1 to 4).** 33 refuse rows written from the request grammar of plan 05. Tests first: every row passed against the plan 05 core on the first run, so no core change was needed and `boot_order.def` is unchanged. Case counts after each task, from the baseline of 25: 34, 43, 50, 58, each equal to the number of ids in `REQUIRED`.
- **Group E and the table cases (Task 5).** Six size and cap rows, then `ref_precedence_table`, `ref_reason_coverage` and `nonmutation_matrix` (67 files).
  - `ref_precedence_table`: 14 REQ>>CODE items, one per adjacent pair of the 14 rows of `boot_order.def` (the buffer and empty rows are one position; an empty request cannot also be over the cap, so empty is tested against short), including the two cap rows from D-82: `len=2097152` and `len=1049088` both give `too_large`. 71 assertions.
  - `ref_reason_coverage`: deferred kind. The refuse handler calls `observe_name` with the reason it saw; the second pass requires every enum member except OK to have been seen (`members=20 observed=20`, 21 assertions). Its RESULT line is the last RESULT line and precedes `RUNNER_DONE`.
  - `nonmutation_matrix`: boots a live instance, then every refuse row into a block of a non-uniform byte pattern between 64-byte guards (plus 64 bytes of slack), and requires the expected refusal and a byte-identical region; at the end the live block must equal its snapshot. 44 assertions = 43 refuse rows + the live-block compare.
  - `scripts/check --selftest mutation-drop-precedence`: builds an unmutated scratch tree and requires `ref_precedence_table` to pass, swaps the adjacent `mapper` and `byte_order` rows in the scratch `boot_order.def`, rebuilds and requires `guest_failure` (item 6, mapper before byte order).
- **No-heuristic, copy-in, redaction (Task 6).** `noheur` boots the recipe with edits (`str@0x1F26=POPULOUS`, `str@0x1FD0=MCGENJIN`, `byte@0x1FFF=0x2D`), declared plain, pce, none, and requires the copy to equal the supplied bytes and the banks to read them back through the mapper. `copy_in` overwrites every host byte after boot and requires every bank to still read the original. `redaction` embeds four 16-character markers (derived from `--marker-seed`) at 0x0100, 0x1F26, 0x1FD0 and 64 bytes before the end, boots, prints the record and scans one buffer (everything printed so far on stdout and stderr, the record, the evidence text, the observed-value text) for each marker as bytes, lowercase hex and uppercase hex. `redaction_leak_control` appends a marker (bytes, then hex) to scratch copies of the log and requires both to be flagged and the clean copy not.
- **Toy.** `tests/controls/heuristic-reverse` holds only RULE and EXPECT. The anchor is `__builtin_memcpy(inst->rom, request->content, request->len);` (one live line in `src/core/boot.c`, as plan 05 recorded); the replacement copies and then bit-reverses each byte when `rom[0x1FFF] < 0xE0`.
- **Ledger.** LED-LOAD-0001 `pinning_test = "TEST:ref_order_tg16"`, LED-LOAD-0002 `"TEST:ref_header_copier512"`, each with `REPORT|PCEdev wiki HuCard_pinout|D0-D7 ascend on PC Engine, descend on TG16: wiring, not dump orientation` (LED-LOAD-0002: `... wiring, not header handling`); `updated` is 2026-11-10; both stay `assumed`; no key removed.
- **Residual risk (D-45, recorded as asked).** A host that declares `plain` for SF2 bytes of a supported size loads them as plain: `ref_mapper_sf2` refuses only the declared `sf2` mapper, and the core never inspects content. Withheld claim: no claim that any image is plain or in PC order.

## Verification Results

Tracer gate (auto mode: the Task 1 verify re-run end to end after the cases were written, then once more with a changed expectation): `sh scripts/check --stage conformance` PASS with 9 of 9 group A rows passing; `ref_mem_short` expecting `load.empty` gave `CHECK conformance FAIL` with `refusal differs from the expected one`, then the file was restored from a copy (`git status` clean). Log line: `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance` (Task 1) | exit 0, `CHECK conformance PASS`, `RUNNER_DONE 34`, `RESULT ref_mem_null pass`, `ref_request_struct_size pass`, `ref_open_unbound pass` (5 assertions each) |
| `sh scripts/check --stage conformance` (Task 2) | exit 0, `RUNNER_DONE 43`, `RESULT ref_mapper_sf2 pass`, `ref_mapper_mcgenjin pass` |
| `sh scripts/check --stage conformance && sh scripts/check --stage ledger-entries` (Task 3) | exit 0, both PASS, `RUNNER_DONE 50` (the first ledger run was `LINE_TOO_LONG`, see deviations) |
| `sh scripts/check --stage conformance` (Task 4) | exit 0, `RUNNER_DONE 58`, `ref_short_513 pass`, `ref_not_bank_256k_plus512_none pass` |
| `sh scripts/check --stage conformance && sh scripts/check --selftest mutation-drop-precedence` (Task 5) | exit 0, `RUNNER_DONE 67`, `SELFTEST mutation-drop-precedence PASS`, `SELFTEST RESULT PASS` |
| `sh scripts/check --stage conformance && sh scripts/check --stage sanitizers && sh scripts/check --selftest mutation-control-anchor` (Task 6) | exit 0, `RUNNER_DONE 73`, `CHECK sanitizers PASS`, `SELFTEST mutation-control-anchor heuristic-reverse PASS` and `wrong-mirror PASS` |

Acceptance criteria, all run:

- Counts: `ls tests/conformance/cases | wc -l` printed 34, 43, 50, 58, 67 and 73 after Tasks 1 to 6, each equal to `grep -c '^[a-z]' tests/conformance/REQUIRED`. Every new case file starts with the 0BSD header; `sh scripts/check --stage licences` PASS after each task.
- Task 2: `grep -n 'load.mapper_unsupported' tests/conformance/cases/ref_mapper_sf2.toml` prints one line (the expect line; the first draft of the meaning text repeated the literal and printed two, so the meaning was reworded).
- Task 3: `grep -c 'PENDING:2'` prints 0 for both ledger entries (baseline 1 each); `grep -c 'status = "assumed"'` prints 1 each; `TEST:ref_order_tg16` and `TEST:ref_header_copier512` are named and both case files exist.
- Task 5: `ref_size_2m_too_large.toml` exists with `expect = "load.too_large"`; `ref_size_2m.toml` is absent; `grep -l 'load.size_unsupported'` over the cases lists exactly ref_size_8192, ref_size_128k, ref_size_192k, ref_size_512k and ref_size_768k.
- Task 5, order independence (scratch TSV, not committed): with the `ref_reason_coverage` rows moved to the first data row, `ds_runner --cases SCRATCH` still gave `RESULT ref_reason_coverage pass assertions=21`, as the 67th and last RESULT line and before `RUNNER_DONE 67`; in the real order it is also the last RESULT line. Baseline: the kind did not exist.
- Task 5, unobserved reason (scratch TSV with the `ref_open_unbound` row removed and END decremented): `RESULT ref_reason_coverage guest_failure` and `coverage: load.open_unbound was not observed by any refuse row`; `nonmutation_matrix` passed with 43 assertions (42 rows plus the live block).
- Task 5, `--only`: `--only ref_precedence_table` printed one RESULT and `RUNNER_DONE 1`; `--only nope` exited 2 with `HARNESS --only names an id that no case carries`.
- Task 6: `CONTROL heuristic-reverse RED_OK` and `CONTROL wrong-mirror RED_OK` in `sh scripts/conformance-run run`; the six cases pass (assertion counts: noheur_populous_string 267, noheur_mcgenjin_string 267, noheur_reverse_vector 68, boot_copy_in_isolated 101, redaction_synthetic_rom 3, redaction_seeded_leak_flagged 3). `ls tests/controls/heuristic-reverse` lists exactly EXPECT and RULE. Replacement set equal to the anchor (run once, then restored from a copy): `CONTROL heuristic-reverse NOT_RED`, `expected guest_failure; got: noheur_reverse_vector:pass`, stage FAIL. On the toy the failing cases were the three noheur rows plus the accept, content and record cases that read a 256K image whose byte at 0x1FFF is 5.
- Sanitizer stage: `STATUS sanitize ok n=71` (73 less the two excluded), the three canaries reported, `EXCLUDED_IN_MODE sanitize alloc_shim_selfcheck` and `alloc_zero_matrix` printed, no ASan or UBSan finding on the full matrix including the 2.5 MiB rows, the non-mutation matrix and the redaction kinds.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, ends `CHECK RESULT PASS` (30.7 s wall) |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | 16 `SELFTEST` lines then `SELFTEST RESULT PASS` (includes `mutation-drop-precedence` and `mutation-control-anchor heuristic-reverse`) |
| `sh scripts/conformance-run run --preset gate-gcc` and `--preset gate-clang` | `RUNNER_DONE 73`, `CONTROL heuristic-reverse RED_OK`, `CONTROL wrong-mirror RED_OK`, `STATUS conformance ok n=73` for both |
| `sh scripts/audit-history --range c6bffb9..HEAD` | `audit-history: clean` (7 commits, 76 blobs) |

Liveness observations made once and reverted (not committed): a one-byte write added to the rev check made `nonmutation_matrix`, `ref_profile_rev_mismatch` and `ref_precedence_table` `guest_failure` (`src/core/boot.c` restored with `git checkout -- src/core/boot.c`, `git status` clean for `src`); one `emit_err` of a marker added to the redaction kind made `redaction_synthetic_rom` `guest_failure` (`redkinds.c` restored from a copy).

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 fix to the core was needed: every refusal row passed against the plan 05 to 10 core on first run, and the core was clean under ASan and UBSan.

### Interpretation choices (inside the plan's intent)

1. **`runner.c` was split before the new kinds went in.** The plan asked for a kind file to be split out first. `kind_banksig` and `kind_unmapped` moved to `tests/conformance/mapkinds.c` (a refactor commit with no behaviour change, 58 of 58 passing), the shared structs and boot helpers (`struct expect`, `struct booted`, `parse_expect`, `fill_request`, `run_boot`, `boot_accepted`, `release_boot`) were declared in `runner.h`, and the runner source list became one CMake variable (`DS_RUNNER_SOURCES`) used by `ds_runner` and the toys. The new kinds live in `refkinds.c` (precedence, coverage, nonmutation, noheur, copy_in) and `redkinds.c` (emit, scanner, redaction kinds). `runner.c` is now 59967 bytes (was 64027; limit 65536).
2. **The precedence table spans the request and expect keys.** The plan words the expectations as one `REQ>>CODE` list in the expect key, but 14 items are about 540 characters and the TOML subset allows 400 per line. The kind reads the request key first and the expect key second (7 items each, about 285 characters per line, leaving room for plan 12's added pair). The kind refuses a table with fewer items than the number of rows of `boot_order.def` minus one.
3. **Group A to E expect lines carry `field=` where the core names a field** (`ref_profile_mismatch`, `ref_open_unbound`, `ref_bind_closed`, `ref_bind_not_allowed`). Plan 05 made the token optional and the plan says `expect load.NAME`; the extra token only makes the row stricter.
4. **TDD: no red commit for Tasks 2 to 4.** No core defect existed, so the rows passed first time and there is no failing state to commit; for Tasks 5 and 6 the kinds did not exist before the cases, so the cases could not be red first. Liveness was shown instead (observations above, the toy, the leak control, the swap mutation).
5. **LED-LOAD-0001 `sources` lost its four `(SUMMARY C7)` suffixes.** Adding the pinout source pushed the line to 403 characters (`LINE_TOO_LONG`, seen once, then fixed); the suffixes repeat the `origin` field, so nothing is lost. LED-LOAD-0002 kept its suffixes.
6. **`ref_size_192k` and `ref_size_768k` have no pins.** They were first written with `LED-LOAD-0004`, whose `pinning_test` names `ref_size_512k` only; the pins were removed in the Task 6 commit so a case never points at an entry that does not point back (`ref_size_512k` keeps its pin from plan 09). Likewise the new noheur rows carry no ledger pins.
7. **The sf2 row's meaning text avoids the literal `load.mapper_unsupported`** so that the plan's one-line grep holds.
8. **`--marker-seed` is passed to the toy runners too**, and without it the redaction kinds are a harness failure, so a new runner invocation in a later plan must pass it.
9. **Other runner output.** `allockinds.c` now prints its per-row diagnostic through `emit_err`. The utility modes of `ds_runner` (`--outcomes`, `--sweep-lengths`, `--spec`, `--hash`, `--emit`) still write to stdout directly: they run no case and their output is data for other scripts.
10. **Requirements.** CONT-01, CONT-02, CONT-03 and PROF-02 are marked complete in REQUIREMENTS.md here, the plan that closes the refusal matrix (plans 09 and 10 had left them open on purpose). The non-mutation compare is byte-wise; plan 13 upgrades it to the state digest (T-02-23).
11. **No "both directions" pin check exists yet** between case `pins` and ledger `pinning_test` (nothing in `ledger-check` or the ingest reads case pins), so the pairs `ref_order_tg16` / LED-LOAD-0001 and `ref_header_copier512` / LED-LOAD-0002 are consistent by hand and by the greps above.

**Total deviations:** 0 auto-fixed, 11 interpretation choices. **Impact:** plan 12 must add refuse rows and precedence items for its two new refusals (see notes).

## Notes for later plans

- **Plan 12 (boot_order.def and refusals.def).** Inserting the two power-on rows raises the row count to 16, so `ref_precedence_table` then needs at least 15 items (it has 14): add the pair the plan names, in the request key or the expect key (each line has about 100 characters of headroom). Appending `powerup_seed_required` and `powerup_seed_unexpected` makes `ref_reason_coverage` require two more observed reasons, so both need a refuse-kind row (`ref_seed_required`, `ref_seed_forbidden`); the coverage case itself needs no edit. Both rows are also picked up automatically by the allocation matrix, the non-mutation matrix and the sanitize run. The heuristic-reverse rule anchors on the content copy line of `boot.c`: keep it on one line.
- **Every runner invocation needs `--marker-seed N`** (`scripts/conformance-run` computes it once per run from the host clock and pid). A direct `ds_runner` call without it gives `harness_failure` for `redaction_synthetic_rom` and `redaction_seeded_leak_flagged` (by design).
- **Output goes through `emit_out` and `emit_err`** (declared in `runner.h`) in every kind; a raw `printf` or `fprintf(stderr, ...)` in a kind would bypass the log that the redaction scan reads.
- **A refuse row above 4 MiB** still needs `ptr=dummy` or `ptr=null`; the 2.5 MiB rows allocate their content.
- **noheur edit syntax** for new rows: `str@0xOFFSET=TEXT` (letters and digits) and `byte@0xOFFSET=0xVV`, items separated by `;`, request must declare plain, pce and none.

## Task Commits

1. **Task 1 (tracer): group A refusal rows** - `f15852a` (feat)
2. **Task 2: group B refusal rows (bindings and mappers)** - `730c179` (feat)
3. **Task 3: group C refusal rows and the two ledger pin moves** - `b398150` (feat)
4. **Task 4: group D refusal rows (short and not a bank multiple)** - `bf2dcd5` (feat)
5. **Task 5a: banksig and unmapped moved into mapkinds.c** - `c11aa1c` (refactor)
6. **Task 5b: size and cap rows, precedence table, deferred coverage, non-mutation matrix, precedence mutation** - `956b170` (feat)
7. **Task 6: no-heuristic rows, heuristic-reverse toy, copy-in and redaction** - `c38f047` (feat)

## Known Stubs

None. No placeholder, hardcoded empty value or TODO was added. (The `request` and `expect` keys of `ref_reason_coverage`, `nonmutation_matrix` and `redaction_seeded_leak_flagged` hold descriptive tokens that their kinds ignore, stated in each case's comment; they are not data stubs.)

## Threat Flags

None. No new network endpoint or file access beyond build-directory writes. The register is mitigated as planned: T-02-20 by `boot_copy_in_isolated` (host buffer overwritten after boot, 101 assertions), T-02-21 by the single emit function, `redaction_synthetic_rom` and the seeded-leak control (both directions shown), T-02-22 by the three noheur rows and the heuristic-reverse toy shown red, T-02-23 by `nonmutation_matrix` (byte compare with guards; the digest upgrade is plan 13). T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `tests/conformance/mapkinds.c`, `refkinds.c`, `redkinds.c`, `tests/controls/heuristic-reverse/RULE` and `EXPECT`, and the 48 case files (73 in the directory, 73 ids in `REQUIRED`).
- Commits `f15852a`, `730c179`, `b398150`, `bf2dcd5`, `c11aa1c`, `956b170` and `c38f047` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 7.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
