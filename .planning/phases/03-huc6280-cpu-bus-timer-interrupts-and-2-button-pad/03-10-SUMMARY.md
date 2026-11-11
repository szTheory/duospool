---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 10
subsystem: fixtures-and-observers
tags: [mutation-sweep, observe-only-rows, insn-sink, memory-invalid, opcode-exercise, w2]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the DCK1 literal table and DSV1 verdict (03-05), the 256-row opcode table and ds_cpu_op table accessors (03-09), the guest kind and ds_run (03-01, 03-04)
provides:
  - kind mutate_sweep (tests/conformance/sweepkinds.c): each DCK1 entry of an in-memory copy of a built image is XORed with $FF, rerun in the same process and must flip exactly its own check to $46 (D-126); REQUIRED cases mutate_sweep_ds_proto and mutate_sweep_ds_proto_bad_exclude, selftest mutation-sweep-vacuous
  - observe-only rows (defined once here for plans 03-17, 03-20, 03-22, 03-24): optional fifth .checks column observe, macro CHECK_OBSERVE in check.inc, build/gate/fixtures/NAME.observe from fixture-build, items observe=IDX:VALUE (kind_guest) and exclude=IDX,... (kind_mutate_sweep, must equal NAME.observe), lint code FXL_BAD_OBSERVE
  - src/core/ds_obs.h and obs.c: the caller-owned instruction sink (24-byte little-endian records, 16-byte header count/lost/seq, drop-newest), the hook ds_obs_insn called once per instruction from ds_cpu_step_obs, validation in ds_run with the new refusal run.memory_invalid (number 7)
  - kinds insn_sink_shapes (obskinds.c) and the deferred kind opcode_exercise with REQUIRED cases insn_sink_shapes, run_refuse_memory_invalid and opcode_exercise (mode=report)
affects: [03-11, 03-12, 03-13, 03-14, 03-17, 03-19, 03-20, 03-22, 03-24, 03-26]

actuals:
  tokens: 20500
  tasks: 3
  commits: 4
plan_head_before: 7e9c57ad0f17e01427b6f9ba0489f2ef8bf31172
plan_head_after: 2164d9d1b354b9a87a4fe6d3a2cb671e6273282b

tech-stack:
  added: []
  patterns:
    - "the sweep boots a patched heap copy through boot_content, the second half of boot_built_rom (load_image + boot_content); the DCK1 table is found by magic at file offset $1000 and every entry must carry its own check number"
    - "an observe row is class-tagged as usual and marked only by the fifth .checks column; the guest stores the observed byte and result $50 through CHECK_OBSERVE, and the case supplies the expected byte, so a row legitimately differing by case never needs a second fixture"
    - "sink writes are byte by byte little-endian from widened unsigned values; the running count lives in a local struct ds_obs_log of ds_run, the header is overwritten at the start and the end of an accepted call and never read; every pointer test is a uintptr_t range test"
    - "a deferred kind enumerates the cases through case_count/case_get, so the inventory follows the case set without a second list"

key-files:
  created:
    - tests/conformance/sweepkinds.c
    - tests/conformance/obskinds.c
    - src/core/ds_obs.h
    - src/core/obs.c
    - tests/conformance/cases/mutate_sweep_ds_proto.toml
    - tests/conformance/cases/mutate_sweep_ds_proto_bad_exclude.toml
    - tests/conformance/cases/insn_sink_shapes.toml
    - tests/conformance/cases/run_refuse_memory_invalid.toml
    - tests/conformance/cases/opcode_exercise.toml
    - tests/gate/fixture-lint/bad-observe/ (EXPECT, t.s, t.checks)
  modified:
    - src/core/cpu.c
    - src/core/run.c
    - src/core/ds_core.h
    - src/core/run_refusals.def
    - tests/conformance/guestkinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.h
    - tests/conformance/runner.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/run_refuse_sink_unsupported.toml
    - tests/controls/shared-state/RULE
    - CMakeLists.txt
    - scripts/check
    - scripts/fixture-build
    - scripts/fixture-lint
    - scripts/lib/fixture-lint-lex.awk
    - scripts/lib/fixture-lint-rules.awk
    - fixtures/lib/check.inc
    - tests/gate/fixture-lint/ok-fixture/fixtures/src/t.s
    - tests/gate/fixture-lint/ok-fixture/fixtures/src/t.checks

key-decisions:
  - "The expect grammar of guest and mutate_sweep cases is id=N followed by any of verdict=OUTCOME:REASON, poison-t0, observe=IDX:VALUE (repeatable) and exclude=IDX,... (strictly ascending, sweep only); the parser, struct guest_expect and the DSV1 constants moved to runner.h, so no case key was added (F1 schema unchanged)"
  - "NAME.observe is one decimal index per line, ascending, empty for every current fixture; fixture-build fails closed (exit 2) when the .checks file of a source is missing or has a fifth column other than observe, a sixth column or a bad index"
  - "CHECK_OBSERVE k, actual loads the operand into A (or uses A when the operand is left out), stores it at +80+(k-1), then calls ck_pass; the LIT entry stays the author's one line LIT k, $00 and the macro asserts at build time that it exists in check order. The macro emits nothing until used, so no fixture image hash moved (verified: the five authored hashes equal those before the plan)"
  - "Insn sink layout: header inside the sink struct (16 bytes: u32 count, u32 lost saturating, u64 seq = instructions offered), records pointer and capacity as the only inputs, 8-byte aligned record memory, DS_INSN_RECORD_BYTES 24 (u64 stamp, u16 pc, opcode, A X Y S P, speed, seven zero bytes); the hook sees the state before the instruction, before the T flag is reset"
  - "ds_run order: dev sink -> run.sink_unsupported, then insn sink validity -> run.memory_invalid, then time_wrap; the header is written only after every refusal check passed, so a refused call writes nothing"
  - "opcode_exercise takes the defined set from ds_cpu_op_defined (the core table, whose 234/22 split scripts/opcode-check compares with the independent manual transcription), not from a second list in the runner"

requirements-advanced: [FIX-03, CORE-06, CPU-01]
requirements-completed: []  # FIX-03 needs every fixture swept and the lint rules of the later waves, CORE-06 needs the dev log and the observer matrix (03-19), CPU-01 needs the W2 fixtures (03-11 to 03-14)

duration: 95min
completed: 2026-11-11
---

# Phase 3 Plan 10: Mutation sweep, observe-only rows, instruction sink and opcode inventory Summary

**Every fixture check can now be shown non-vacuous by an in-process literal sweep (ds_proto: 8 of 8 flip exactly their own check), the core has a caller-owned instruction trace sink that changes nothing and refuses hostile placements, and the deferred opcode inventory measures that the guest fixtures execute 30 of the 234 defined opcodes.**

## What was built

- **Task 1 (tracer), commit 24e7e48.**
  - `kind mutate_sweep`: loads the built image once, checks the DCK1 magic at file offset $1000, the count against the case's `checks` and that entry k carries the number k, runs the unmutated image (it must reach done and pass), then for each non-excluded k XORs the entry's value byte with $FF in a fresh heap copy, boots it through `boot_content` and runs it with a done-polling stop callback (speed aid only; `until_mclk` is the hard bound). One assertion per swept literal; a result byte other than $46 for the mutated check reads "did not flip check N: the check is vacuous", any other check not $50 reads "also changed check M".
  - `boot_built_rom` split into `load_image` and `boot_content`; the DSV1 constants, `struct guest_expect`, `parse_guest_expect`, `judge_dsv1` and `expect_verdict` moved to runner.h so the sweep reuses them.
  - Observe-only rows exactly as the plan states them: lint lexer and rules accept the optional fifth column (code FXL_BAD_OBSERVE for any other text, a sixth column, or a row marked observe whose source line is not CHECK_OBSERVE and the reverse; negative control tests/gate/fixture-lint/bad-observe, the positive control ok-fixture gained one valid observe row), `fixture-build` writes NAME.observe, `kind_guest` demands an `observe=IDX:VALUE` item for each index (`observe-missing`) and rejects an item for a non-observe row (`observe-unlisted`), `kind_mutate_sweep` requires `exclude=` to equal NAME.observe (`exclude-mismatch`).
  - REQUIRED cases `mutate_sweep_ds_proto` (assertions 8, machine_time 137880 at done) and `mutate_sweep_ds_proto_bad_exclude` (exclude=3 against an empty observe list, expect verdict=harness_failure:exclude-mismatch).
  - Selftest `mutation-sweep-vacuous`: scratch mini-root, baseline sweep must pass, the single line invoking a CHECK macro with argument 3 (regex on name and literal, exit 2 unless exactly one) is replaced by the CHECK_LIT expansion with `cmp #$16` instead of the table read, rebuilt in scratch, and the sweep must give guest_failure with "did not flip check 3" and "the check is vacuous". The older `mutation-guest-wrong-literal` selftest now also copies ds_proto.checks (fixture-build needs it).
- **Task 2, commit 2ddb49e.** `ds_obs.h`/`obs.c`; `ds_run` validates the sink (`ds_obs_insn_sink_valid`: sink aligned and clear of instance, request and result; capacity 0 uses no pointer; otherwise record pointer non-NULL, 8-aligned, byte size without overflow or wrap, clear of instance, request, result and the sink itself) and refuses with `run.memory_invalid` (number 7); `ds_cpu_step_obs(inst, log)` calls the hook once per instruction before the T reset, `ds_cpu_step` is the no-log wrapper. `run_refuse_sink_unsupported` now covers the dev sink alone and the dev sink beside a valid insn sink (the insn sink alone is accepted). `run_refuse_memory_invalid` has nine variants (below). `insn_sink_shapes`: capacities 0, 1, 4, 64, 4096 and 131072, each clean and each with header and record memory poisoned with $A5 (and a non-NULL record pointer at capacity 0), compared with a no-sink reference run (machine time, steps, overshoot, whole-instance digest) and with a separate stepwise run that kept the state before each instruction; header and records decoded by hand, the unused record bytes and 64 guard bytes either side checked untouched. The case ends with 50 assertions on ds-proto (about 90 thousand instructions at the deadline).
- **Task 3, commit 0ab0c4d.** Deferred kind `opcode_exercise` (RESULT before RUNNER_DONE): one run per distinct (ROM, request) of the guest cases through `case_count`/`case_get`, insn sink of 262144 records, stop at done or the deadline, union of the executed opcodes, unexercised defined opcodes into the evidence file (16 per line, `ev_detail` grew from 1024 to 4096 bytes), `lost != 0` is harness_failure. Mode report passes (measured: 5 ROM runs, 30 of 234 defined opcodes exercised, 204 not); mode require with `exclude=0xNN,...` fails each unexercised, not excluded opcode (shown by a scratch probe). `ds_cpu_op_defined` added to cpu.c/ds_core.h.
- **Fix commit 2164d9d.** My first FXL_BAD_OBSERVE statement was chained with `else if` to the line the selftest `mutation-fixture-lint-orphan` deletes; it is now a separate statement (selftest back to PASS).

## Verification

- Task verifies: Task 1 `--stage conformance`, `sanitizers`, `--selftest mutation-sweep-vacuous`; Task 2 `--stage build`, `core-boundary`, `conformance`; Task 3 `--stage conformance`: PASS (the sanitizers and core-boundary stages also run after Tasks 2 and 3).
- Plan end: `sh scripts/check` PASS (55 PASS lines, about 92 s wall), `sh scripts/check --selftest` PASS (56 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 7e9c57a..HEAD` clean (3 commits, 44 blobs at the time of the run, 1220 blob-path pairs).
- Probes outside the repository (scratch directories, nothing committed): the observe machinery with a scratch ds_proto whose check 8 is CHECK_OBSERVE and observe in its .checks: guest case without the item gives harness_failure (observe-missing), with a wrong value guest_failure, with the right value pass (observed byte $44), an item for a non-observe row harness_failure (observe-unlisted), sweep with exclude=8 passes with 7 assertions, without it harness_failure (exclude-mismatch). The sink writer was mutated in three scratch edits (wrong register in the record, an off-by-one capacity test, lost counted twice): the first and third gave guest_failure from insn_sink_shapes, the second ran past the record memory and crashed the runner (so it cannot pass); the file was restored byte for byte. Sink size 1000 in opcode_exercise gave harness_failure for lost != 0.

## The nine variants of run_refuse_memory_invalid

(1) record memory inside the instance's work RAM, (2) inside the instance's ROM copy, (3) over the request, (4) the record memory is the sink struct itself, (5) the sink struct lies inside the instance, (6) misaligned record pointer, (7) NULL record pointer with nonzero capacity, (8) a record range that would wrap the address space, (9) record memory over the result struct. Each: refused, 0 steps, overshoot 0, instance bytes and digest unchanged.

## Deviations from Plan

**1. [Rule 1 - plan text cannot be implemented literally] "sink overlapping the content buffer".** `ds_run` keeps no pointer to the host content buffer (the instance holds a copy, D-42), so it cannot refuse an overlap with it, and the overlap would be harmless to the core because the content was copied at boot. The third item of `run_refuse_memory_invalid` therefore uses the machine's own content, the ROM copy inside the instance, and two further variants (header overlap, result overlap) were added; the sink-overlaps-instance family is otherwise as written. No behaviour of the plan is dropped.
**2. [Rule 3 - blocking] Files outside the plan's list.** `tests/controls/shared-state/RULE` (its anchor is the one line of run.c that calls the step; the call became `ds_cpu_step_obs(inst, obs)`, so anchor and replacement changed with it; the toy and its EXPECT are otherwise as before), `tests/conformance/stepkinds.c` (the sink_unsupported row used an untyped insn buffer; the rows and the memory_invalid variants live there), `tests/gate/fixture-lint/` (the control for the new lint code and the observe row of the positive control), `scripts/fixture-lint` (the code list in its header) and `src/core/ds_core.h` (the sink type in the request, `ds_cpu_op_defined`).
**3. [Rule 2 - fail closed] fixture-build refuses a missing .checks.** The plan does not say what NAME.observe is when a source has no .checks file; an empty list would silently drop observe rows, so a missing file is exit 2. The only mini-root affected (selftest mutation-guest-wrong-literal) now copies ds_proto.checks.
**4. [Rule 2] The observe cross-check of the lint.** Beyond the plan: a row marked observe whose check is not CHECK_OBSERVE, and a CHECK_OBSERVE whose row is not marked, are FXL_BAD_OBSERVE, so the lint and the sweep exclusion cannot disagree about which checks compare.
**5. Reading of "assertions = 8 plus the unmutated baseline run".** The baseline is a precondition (it adds an assertion only when it fails), so the passing case reports assertions=8, one per mutation.
**6. observe=IDX:differs.** The syntax is parsed (the plan names it for the policy-diff mode of plan 03-24) but a case that uses it in a single-run guest case is harness_failure (observe-differs) until that mode exists.

No Rule 4 deviation. The shared-state toy control stayed RED_OK on the first full run after the anchor change, so the standing note (make the toy phase-independent if it recurs) was not triggered and nothing was retuned.

## Evidence discipline

E0 only. The sweep shows that each literal of ds_proto can flip its check; it says nothing about what a check proves, and the ASSUMED and unverified MAN cites of ds_proto are unchanged. The sink cases compare the core with itself (a second observation of the same core), so they establish non-interference and layout, not what an instruction does. The opcode inventory is a measurement (30 of 234 defined opcodes executed by the five ds-proto images), not a coverage claim. No ledger status changed and none of the eight ER-2 candidates was touched. No expected literal comes from DuoSpool output: the record bytes are compared with state the runner itself captured by stepping, and the "defined" set is the table that scripts/opcode-check compares with the manual transcription. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim, no third-party material, nothing pushed (D-64).

## Known Stubs

None added. The unexercised opcodes are the report of the inventory, not a stub; the mnemonic groups of plans 03-11 to 03-14 are still the gap cases named in the 03-09 summary. `observe=IDX:differs` is parsed and refused until plan 03-24 (item 6 above).

## Threat Flags

None beyond the plan's register. T-03-29 (sink overlap) is mitigated by `ds_obs_insn_sink_valid` and nine run_refuse variants with the instance byte-identical; T-03-30 (poisoned header) by the output-only header, the capacity taken from the request and the poisoned shapes of insn_sink_shapes; T-03-31 (vacuous checks) by mutate_sweep with a selftest that shows it catching a vacuous check.

## Notes for next plans

- **Library API for observe rows (03-17, 03-20, 03-22, 03-24):** in `NAME.checks` add the fifth TAB-separated column `observe` to the row (class tag stays MAN, GUEST or ASSUMED); in `NAME.s` write the usual `LIT k, $00` and `CHECK_OBSERVE k, a:operand` (or load A and write `CHECK_OBSERVE k`); in every case that runs the ROM add `;observe=k:VALUE` to its expect, and in the sweep case `;exclude=k` (indexes ascending, the list must equal NAME.observe). The control for "observe row without an expectation" is `id=1;verdict=harness_failure:observe-missing` on the guest kind (probed here). A ROM shared by several cases (mmu_mpr_384k and mmu_mpr_1m, NW1) needs the item in each.
- **Sweep cases for new fixtures:** `kind = "mutate_sweep"`, `fixture = "authored:NAME"`, `expect = "id=N"`, `until_mclk` as the guest case, `checks` as the guest case. The sweep assumes the default request (no byte-order or header change, because the DCK1 table is read at file offset $1000 of the image as built). A fixture longer than the done-poll needs is fine; a mutation that hangs the guest is reported as "did not reach the done state".
- **Insn sink users (03-19, 03-26):** the sink struct is `struct ds_insn_sink { uint8_t header[16]; uint8_t *records; uint32_t capacity; }` in ds_obs.h, set `request.insn`; records are 24 bytes, 8-byte aligned memory; `lost` saturates. `DS_SINK_MAX_RECORDS` does not exist yet (03-19 introduces it with the oversized-capacity row of run_refuse_sink_unsupported); today a capacity above what the address space allows is memory_invalid. The hook is `ds_obs_insn(log, inst, opcode)` placed before the T reset in `ds_cpu_step_obs`; plans that add interrupt entries or block steps decide whether those emit a record (the hook currently fires once per ds_cpu_step_obs call, i.e. per step). `ds_run` keeps the running counts in a local; a stop callback sees the header as it was at the start of the call.
- **03-11 to 03-14:** replace the gap cases in `ds_cpu_step_obs` as before; the cpu.c signature of the step is now `ds_cpu_step_obs(inst, log)` and `ds_cpu_step(inst)` calls it with NULL. `opcode_exercise` is in mode=report; plan 03-14 switches it to `mode=require;exclude=...` and must list the opcodes it allows to stay unexercised. The evidence file at build/gate/evidence/opcode_exercise.txt lists the 204 unexercised opcodes of this plan.
- **opcode_exercise needs `until_mclk` on the guest case** (cases without it are skipped), and runs each distinct (ROM, request) once; a fixture with a very long run (above about 260 thousand instructions before done) needs OX_SINK_RECORDS raised, or lost != 0 makes the case harness_failure.
- **Shared-state toy control:** the anchor is now `inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step_obs(inst, obs);` in run.c; a later edit of that line must change tests/controls/shared-state/RULE in the same commit. The control was RED_OK for all three interleave cases at the end of this plan; per the standing note, if it turns NOT_RED, make the toy phase-independent rather than retuning call sizes.
- **fixture-build:** the number of runner images is still five (ds-proto and its four protocol variants), each now with a NAME.observe beside NAME.pce; plan 03-06 hash pins are unchanged by this plan (the five hashes equal those before it).
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. Measured cost of the new cases in the gate build: insn_sink_shapes 0.08 s, mutate_sweep_ds_proto 0.03 s, opcode_exercise 0.02 s. The first full `sh scripts/check` of this plan took 92 s wall (the 03-09 run took about 56 s; a later `--timing` run shows conformance and sanitizers at 9 s each, so the difference is not from the new cases and is not attributed to anything measured here). `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: tests/conformance/sweepkinds.c, tests/conformance/obskinds.c, src/core/ds_obs.h, src/core/obs.c, the five new case files, tests/gate/fixture-lint/bad-observe. Commits 24e7e48, 2ddb49e, 0ab0c4d and 2164d9d are ancestors of HEAD.
