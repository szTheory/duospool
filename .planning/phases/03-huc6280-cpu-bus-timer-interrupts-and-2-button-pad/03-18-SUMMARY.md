---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 18
subsystem: run-record-model-constants-run
tags: [run-record, model-tables, constants-table, run-table, record-format-3, constants-def, record-lint, ledger, w3]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: model_params.def with tma_zero and the ds_model_* accessors (03-17), the master-clocks record of format 2 and both goldens (03-36), ds_run and struct ds_run_result (03-02, 03-35), the DSV1 guest kind and the ds-proto fixture (03-01, 03-05), the bus constants and the VDC penalty (03-16)
provides:
  - run record format 3: after [power_on] one [model_NAME] table per model_params.def row (value, ledger, status, bound_by), the [constants] table (NAME = "VALUE LEDGER" per constants.def row) and the [run] table (reason, overshoot, stop_point, steps); [withheld] is the sorted union of profile, bound field, model and constants ledger ids
  - src/core/constants.def: nine rows (cycle_mclk_low 12 and cycle_mclk_high 3 LED-CPU-0003, reset_cost_mclk 0 LED-CPU-0004, undefined_opcode_bytes 1 and undefined_opcode_cycles 2 LED-CPU-0010, unmapped_read 255 LED-CPU-0012, vdc_vce_penalty_cycles 1 LED-CPU-0017, tam_zero_noop 1 and tma_multibit_highest 1 LED-CPU-0015) read by the core as enumerators DS_CONST_name
  - ds_text_record(inst, run, buf, cap, outlen) with struct ds_text_run {result, model_bound}
  - record_lines checks every model table; new kinds record_run and record_constants; REQUIRED cases record_model_bound, record_run_shown, record_constants_shown; record-lint control missing-model-table
  - ledger entries LED-PROF-0004 and LED-CPU-0015 (assumed, E0, one pin each) and their REQUIRED lines
affects: [03-19, 03-20, 03-22, 03-23, 03-25, 03-27, 03-28, 03-29, 03-32]

actuals:
  tokens: 14140
  tasks: 3
  commits: 3
plan_head_before: 88c8bc0591165ac23178af4abde86eafd4728959
plan_head_after: 8a6e6263ca0d32f599773db3972f656b85f93863

tech-stack:
  added: []
  patterns:
    - "a def with named values is one X-macro file read three ways: the core derives enumerators (DS_CONST_name in ds_core.h) and uses them, the printer builds its row table from the same include, and the conformance kind reads the file as TEXT and compares; a header that stops using the enumerator and hard-codes a number again is caught by the kind that compares the macros with the text"
    - "a variable-length part of the record (a table per model row, a line per constant) is checked without a count a later plan must edit: the lint takes MODELS as one token and constants values of the shape NUMBER LEDGER-ID, record_lines builds its key skeleton from the same defs, the pinned count is 65 fixed rows plus 4 per model row plus 1 per constant"
    - "the record carries what the request said, not only what the instance holds: bound_by is a printer input (struct ds_text_run.model_bound) that the harness computes from the request, so the core gained no state and no digest region"

key-files:
  created:
    - src/core/constants.def
    - tests/conformance/cases/record_model_bound.toml
    - tests/conformance/cases/record_run_shown.toml
    - tests/conformance/cases/record_constants_shown.toml
    - tests/gate/record-lint/missing-model-table/record.toml
    - tests/gate/record-lint/missing-model-table/EXPECT
    - ledger/entries/LED-PROF-0004.toml
    - ledger/entries/LED-CPU-0015.toml
  modified:
    - src/text/text.c
    - src/text/ds_text.h
    - src/core/ds_core.h
    - src/core/boot.c
    - src/core/cpu.c
    - src/core/model.c
    - scripts/lib/record-rules.awk
    - scripts/record-lint
    - tests/conformance/reckinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/hashkinds.c
    - tests/conformance/ilvkinds.c
    - tests/conformance/redkinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/cases/mmu_mpr_tma_zero_latch.toml
    - tests/conformance/cases/model_param_refusal.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - tests/gate/record-lint/ok-accept/record.toml
    - tests/gate/record-lint/missing-field-table/record.toml
    - tests/gate/record-lint/missing-machine-table/record.toml
    - tests/gate/record-lint/path-in-value/record.toml
    - tests/gate/record-lint/pointer-in-value/record.toml
    - tests/gate/record-lint/timestamp-in-value/record.toml
    - tests/gate/determinism/ok-same/a.toml
    - tests/gate/determinism/ok-same/b.toml
    - tests/gate/determinism/differs/a.toml
    - tests/gate/determinism/differs/b.toml
    - ledger/REQUIRED

key-decisions:
  - "bound_by is not instance state: the printer takes an optional struct ds_text_run {result, model_bound} (bit i = row i of model_params.def) that the harness builds from the model.NAME keys of the request; NULL means no run and nothing bound. A byte in the instance would have needed a digest decision and a serial bump for a fact that is not machine state"
  - "the constants table is one table with one line per row, NAME = \"VALUE LEDGER\" (decimal value, one blank, the ledger id), so the plan's wording (a [constants] table, each row equal to constants.def) holds, the lint needs no name list, and a later plan adds a row with one def line, one golden line and no awk or key-skeleton edit"
  - "the lint does not enumerate model names or constants names (MODELS is one repeated token, constants keys are free but their values must be NUMBER LEDGER-ID): plan 03-22 adds a model row and 03-23 and 03-25 add constants rows, and their file lists do not include the mini-roots"
  - "the core reads constants.def: ds_core.h derives the cycle, unmapped-read and VDC penalty macros from DS_CONST_name enumerators, boot.c charges reset_cost_mclk, cpu.c reads tam_zero_noop and tma_multibit_highest as branches; the undefined opcode rows of cpu_ops.def stay literals (opcode-check parses them) and record_constants_shown compares all 22 with the two constants"
  - "model.c gained a _Static_assert that the reset cost is 0, because ds_model_set tests machine_time == 0 as the state before the first step"
  - "the field order of [run] follows the plan text: reason, overshoot, stop_point, steps; values none, until, callback, refused for the reason and none, instruction, block_byte for the stop point; a reason or stop point outside the sets is DS_TEXT_ERR_STATE"
  - "the plan-end ledger entries do not name emulators: the readings behind ER-8 are not in the repository and were not re-read, so the disagreement column says so (see Deviations 5)"

requirements-advanced: [CPU-06, CORE-04]
requirements-completed: []  # CPU-06 needs the second parameter, the speed and timer rows and the plan 03-32 closeout; CORE-04 needs the later run and guest plans

duration: 95min
completed: 2026-11-11
---

# Phase 3 Plan 18: Model, constants and run tables in the run record Summary

**The run record (format version 3) prints every model parameter with its value, ledger id, status and who bound it, every named constant with its ledger id, and how the run ended; the ledger ids of all of them join [withheld]; the core reads the constants from the file the record prints; LED-PROF-0004 and LED-CPU-0015 exist as assumed entries with one pin each.**

## What was built

- **Task 1 (tracer), commit fda4eb9.** `src/core/constants.def` (nine rows, X-macro, SPDX header, a comment per row) and `DS_CONST_name` enumerators in `ds_core.h`; `text.c` gained `emit_model`, `emit_constants`, `emit_run` after `[power_on]`, and `emit_withheld` adds the model and constants ledger ids; `DS_TEXT_FORMAT_VERSION` is 3. `record-rules.awk` takes the MODELS token,
  a [constants] table with value shape NUMBER LEDGER-ID and the [run] keys; `reckinds.c` builds its expected keys from the same defs and pins the row count (78 today: 65 fixed, 4 per model parameter, 1 per constant), and its refusal-prefix literal says format_version 3. Both goldens, `record_skeleton_256k`, the six record-lint mini-roots and four determinism mini-roots changed
  in this commit, so no stage was ever red at a commit point (the plan lists the mini-roots under Task 2; see Deviations 3). The golden digests, profile hashes and content hashes did not change (no state was added); the new blocks and the 17 withheld ids were written by hand from the def files and the ledger, not copied from printer output.
- **Task 2, commit 5518d49.** `record_lines` now checks each model table against the request (value, binder) and the def (ledger, status) and that the row's ledger id is in the withheld list; kind `record_run` runs the built ds-proto image to a deadline with a real `ds_run` and prints the record with the result (case `record_run_shown`); kind `record_constants` reads
  `src/core/constants.def` as text and compares it with the table (case `record_constants_shown`); case `record_model_bound` (`model.tma_zero=a_unchanged` prints value a_unchanged, bound_by run-request); control `tests/gate/record-lint/missing-model-table` (exactly REC_TABLES).
- **Task 3, commit 8a6e626.** `LED-PROF-0004` (pin `TEST:model_param_refusal`, origin 03-CONTEXT D-113) and `LED-CPU-0015` (pin `TEST:mmu_mpr_tma_zero_latch`, origin 03-CONTEXT D-105), each with every schema key and five withheld items (the four standing ones and one specific); the two cases carry the ids in `pins`; `ledger/REQUIRED` lists both.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `conformance`, `record-lint`; Task 2 `record-lint-negatives`, `determinism-negatives`, `conformance`; Task 3 `ledger-entries`, `inventory`.
- Plan end: `sh scripts/check` PASS (RESULT PASS), `sh scripts/check --selftest` PASS (60 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 88c8bc0..HEAD` clean (3 commits, 38 blobs, 1279 blob-path pairs, run before this docs commit).
- Bite shown with scratch edits (each restored byte for byte, nothing committed): bound_by printed as default for every row (record_model_bound red on the run-request line); every constants line carrying the ledger id of the first row (record_constants_shown, and both goldens red); overshoot hard-coded 0 (record_run_shown red: the overshoot is not the machine time past the
  deadline); a changed ledger id in constants.def (both goldens red, as the plan says). The in-kind controls of record_constants fail the comparison for a changed ledger id, a changed value, a dropped row and an added row. The withheld check for the model row cannot go red today because the single model row shares its ledger id (LED-CPU-0015) with two constants rows; it bites
  when a row with its own id arrives (plan 03-22).
- Expected literals: the value, ledger id and status of record_model_bound come from the case's request and the def (ASSUMED:LED-CPU-0015); the goldens' new blocks from the def files; record_run_shown names only the reason and stop point words and asserts structure; nothing is a DuoSpool-derived number.

## Deviations from Plan

**1. [Rule 3 - plan wording] struct ds_text_run instead of a bare run-result pointer.** The plan says `ds_text_record` gains an optional const run-result pointer. The record must print `bound_by = run-request` (case record_model_bound) and the instance does not record how a model value arrived (03-17 note). A byte in the instance would need a digest decision and a serial bump,
so the optional argument is a small struct: the run result pointer (NULL prints reason none) and a bitmask of the model rows the request bound (NULL prints bound_by default). Callers that do not care (hashkinds, ilvkinds, redkinds) pass NULL.
**2. [Rule 3 - consequence of the plan] Files outside the plan's list.** `ds_core.h`, `boot.c`, `cpu.c`, `model.c` (the core reads constants.def, as the plan requires: "the core reads its constants from the same def"), `runner.c` and `runner.h` (one table row and one declaration per new kind), `hashkinds.c`, `ilvkinds.c`, `redkinds.c` (the new argument), `scripts/record-lint`
(a comment) and `record_skeleton_256k.toml` (the pinned format_version literal and the row count in the meaning text).
**3. [Plan wording] The mini-roots change in Task 1's commit, not Task 2's.** The record-lint and determinism stages were red after the layout change until the mini-roots carried the new tables, and F10 asks for one change; only `missing-model-table` and the three cases and kinds are in Task 2.
**4. [Rule 3] The new kinds are not "record kind extended".** The plan says the record kind is extended to run ds-proto; a new kind `record_run` (and `record_constants`) is cleaner than adding run keys to `record_lines`, and `record_lines` stays the skeleton kind. The `--records DIR` contract (D-86) is unchanged: neither new kind writes a file.
**5. [Gap, not fixed here] The ER-8 emulator readings are not named in LED-CPU-0015.** The plan says the disagreement column lists the emulator readings by name as reports only. The repository holds only the statement "four emulators, three ways" (03-CONTEXT ER-8) and the pcetech report (PITFALLS C5); the emulator names and readings were not recorded by an earlier plan and
this plan read no emulator source, so naming them would be invention. The column says so and states that the choices are not a vote. A later plan that reads those sources with provenance should fill it (pending; no owner-gate item).
**6. [Rule 2] Extra checks beyond the plan.** `record_constants` also checks that the values in use equal the def (cycle macros, unmapped read, VDC penalty, reset cost of a fresh boot, the 22 undefined opcode rows), `record_run` also prints none, callback and refused records, and a model selection outside its list is a printer error (checked in `record_lines`).

No Rule 1 or Rule 4 deviation. The shared-state toy control stayed RED_OK, so the standing note was not triggered. No ledger status changed beyond the two new assumed entries; none of the eight ER-2 candidates was touched.

## Evidence discipline

E0 only. The record repeats what the request, the defs and the run returned; the constants and the parameter are modelling assumptions (ledger status assumed) and passing says that the printed value is the value in use, not that the value is right. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; the printer writes ledger ids and the word assumed,
never the word supported. Nothing pushed (D-64). `.planning/state.json` left untouched and uncommitted.

## Known Stubs

None added. The stubs of earlier plans are unchanged. Not added to `.planning/WINDOWS.md`.

## Threat Flags

None beyond the plan's register. T-03-49: the printer still refuses quotes, backslashes and non-ASCII (model values are copied into a bounded buffer and printed through the checked string path), the lint keeps its path, pointer and timestamp rules and the constants values are digits and a ledger id. T-03-50: every def row is printed with ledger id, status and binder, and the
ids join [withheld].

## Notes for next plans

- **The [withheld] line has a hard ceiling of 24 ids.** The ids line prints each id as 16 characters and a line may hold 400, so `WH_MAX` is 24 (392 characters). It holds 17 ids now. Plans 03-22, 03-23, 03-25 and 03-29 add model and constants ids (LED-IRQ-0003, -0005, -0006, -0007, LED-CPU-0006, -0008, and the pad ids); count them, and when the union can pass 24 the layout
  must change (several keys such as ids, ids_more, or one table per area) in the same four-place change with a format version bump. `emit_withheld` fails with a state error rather than truncating.
- **Adding a model row (03-22):** one line in `model_params.def`, the member in `ds_instance`, and the golden block `[model_irq_i_sample]` after `[model_tma_zero]` plus its ledger id in the withheld line; no awk, reckinds expected-key or count edit is needed (the key skeleton and the count follow the def). `record_lines` also has one literal reference to `tmp->tma_zero` (the
  out-of-range selection control) that should become a loop or gain a second line.
- **Adding constants rows (03-23, 03-25):** one `DS_CONST` line each (decimal value, one ledger id), the golden line and the ledger id in the withheld line; the core must read the value (enumerator `DS_CONST_name`) or `record_constants_shown` needs a cross-check like the undefined-opcode one. The lint and the skeleton follow the def. `REC_MAX_ROWS` is 128 and the record holds
  78 rows now.
- **Adding tables (03-23 `[stimulus]`, 03-28 `[input]`):** the four places of F10 are now `text.c`, `record-rules.awk` (`keys[]` and `seq_accept`), `reckinds.c` (`build_expected_keys` and the fixed part of the count, 65 today) and `DS_TEXT_FORMAT_VERSION` (3), plus both goldens, the refusal-prefix literal (`format_version = 3`) and `record_skeleton_256k`
  (`record.format_version=3`), and the 10 mini-root records; the new tables belong after `[run]` or before it, and the mini-root records and goldens carry the current order (power_on, model tables, constants, run, limits, machine).
- **The bound_by mask:** `run_of` in `reckinds.c` builds it from `spec.model_name`; a kind that prints a record for a case with model keys must pass it (kinds that pass NULL print default). A future host that prints the record must pass the same information.
- **03-20:** the plan writes LED-CPU-0012 (unmapped, ROM writes, stubs) and the constants table already names it, and LED-CPU-0017 for the VDC penalty; the record prints them in `[withheld]` before the entries exist (the ledger and the record are independent).
- **03-32 (closeout):** LED-CPU-0015 pins `mmu_mpr_tma_zero_latch` only; the keep case and the refusal cases are named in its evidence column but are not pins (one pin per entry, F9). The ER-8 column awaits the emulator readings (Deviations 5).
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation.

## Self-Check: PASSED

Created files present: src/core/constants.def, ledger/entries/LED-PROF-0004.toml, ledger/entries/LED-CPU-0015.toml, the three record cases and tests/gate/record-lint/missing-model-table. Commits fda4eb9, 5518d49 and 8a6e626 are ancestors of HEAD.
