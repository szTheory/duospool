---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 17
subsystem: model-parameters-and-tam-tma-edges
tags: [model-params, tma-zero, ds-model-set, tam-tma-edges, mpr-latch, observe-rows, bus0-region, serial-version-4, model-refusals, w3]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: bus.c, mpr_latch in the instance and in BUS0 with no writer (03-16), the mmu_mpr fixture of 48 checks and its manifest pin (03-16), TAM and TMA with the D-105 multi-bit constants (03-11, 03-14), observe-only rows and the mutation sweep (03-10), the digest stream of serial version 3 and its goldens (03-16)
provides:
  - src/core/model_params.def (one row, tma_zero {bus_latch default, a_unchanged}, ledger LED-CPU-0015, status assumed), src/core/model_refusals.def (unknown_name 1, value_not_allowed 2, state 3; prefix model.), src/core/model.c with ds_model_set, ds_model_value and the name, allowed, default, ledger and status accessors
  - model.NAME request key in parse_spec and apply_model (guestkinds.c), applied after the boot and before the first step by boot_content, boot_accepted and the digest kinds' boot; a boot row (refuse, accept) with a model key is a harness fault
  - TMA #0 under both values, TAM and TMA latch writes (mpr_latch), mmu_mpr checks 49 to 60 (3 observe rows, 9 ordinary ASSUMED rows) and the REQUIRED cases mmu_mpr_tma_zero_latch, mmu_mpr_tma_zero_keep and the control mmu_mpr_observe_missing
  - BUS0 is three bytes (io_buffer, mpr_latch, tma_zero selection), DS_SERIAL_VERSION 4; digest_stream_prefix, digest_region and both goldens updated; new cases digest_stream_model, digest_model_tma_zero_differs, digest_model_default_same
  - kinds model_refuse and the deferred model_refusal_coverage; REQUIRED cases model_refuse_unknown_name, model_refuse_value, model_refuse_state, model_param_refusal
affects: [03-18, 03-19, 03-20, 03-22, 03-24, 03-30, 03-32]

actuals:
  tokens: 20072
  tasks: 3
  commits: 3
plan_head_before: 49a4ca7f898d914c2ab8e7c04f1c97bac82065cf
plan_head_after: 7509dff0c9ef43e77bd82b3747b0d81c04718294

tech-stack:
  added: []
  patterns:
    - "a model parameter is one X-macro row whose name is also the name of a one-byte instance member holding the selection (an index into the allowed list, the first value being the default so a zeroed instance holds the default); the setter validates name, then value, then state, and only then writes"
    - "a row of model_params.def exists only with a fixture that distinguishes both values: the same ROM runs in two cases that differ in the request key model.NAME and in the observe=IDX:VALUE items"
    - "a refusal kind keeps a copy of the whole instance and its digest around each refused call (instance byte-identical), and a deferred coverage kind requires every member of the refusal def observed and the parameter rows well formed"
    - "the digest literals of a serial bump are recomputed by a perl script that first rebuilds the previous version and must reproduce the committed digests, then appends the new byte; one extra case with a non-default selection pins the new byte independently of the C code"

key-files:
  created:
    - src/core/model_params.def
    - src/core/model_refusals.def
    - src/core/model.c
    - tests/conformance/cases/mmu_mpr_tma_zero_latch.toml
    - tests/conformance/cases/mmu_mpr_tma_zero_keep.toml
    - tests/conformance/cases/mmu_mpr_observe_missing.toml
    - tests/conformance/cases/model_refuse_unknown_name.toml
    - tests/conformance/cases/model_refuse_value.toml
    - tests/conformance/cases/model_refuse_state.toml
    - tests/conformance/cases/model_param_refusal.toml
    - tests/conformance/cases/digest_stream_model.toml
    - tests/conformance/cases/digest_model_tma_zero_differs.toml
    - tests/conformance/cases/digest_model_default_same.toml
  modified:
    - src/core/cpu.c
    - src/core/ds_core.h
    - src/core/serial.c
    - src/core/ds_serial.h
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/guestkinds.c
    - tests/conformance/digkinds.c
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - fixtures/manifest/mmu_mpr.toml
    - tests/conformance/REQUIRED
    - tests/conformance/cases/mmu_mpr_256k.toml
    - tests/conformance/cases/digest_stream_prefix.toml
    - tests/conformance/cases/digest_region.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml

key-decisions:
  - "TMA #0 under bus_latch returns mpr_latch, under a_unchanged leaves A; the latch is the byte of the last TAM or TMA with a nonzero operand (TAM stores A, TMA the byte it returned); TAM #0 is a no-op for the registers and for the latch; TMA #0 does not change the latch. All of it is ASSUMED:LED-CPU-0015: the manual defines only the operand 2^i (S8-90 and S8-101, read as images by this plan: operand 2^i, i = 0 to 7, nothing for zero or several bits)"
  - "the selection of a parameter is the instance member with the row's name (tma_zero, one byte, after mpr_latch) and the third byte of BUS0, so a restored instance behaves as the original and D-121's region set is unchanged; DS_SERIAL_VERSION 4"
  - "ds_model_set checks the name, then the value, then the state (machine time not 0, or a NULL instance, is model.state), so a bad argument after the first step keeps its own code; a refused call writes nothing"
  - "a value outside the first-value-is-default rule would break the zeroed-instance default, so model_param_refusal checks that the default text equals the first allowed value"
  - "the TMA #0 edge is three observe-only rows (49 after a TAM, 59 after a TMA, 60 after a no-op TAM #0), each judged per case; the multi-bit TAM and TMA and the TAM #0 register checks are nine ordinary LIT rows (50 to 58) and are swept"
  - "the model keys of a request are applied by the shared boot helpers (boot_content, boot_accepted, the digest kinds' block_boot); a boot row (refuse, accept) refuses a model key as a harness fault rather than ignoring it silently"

requirements-advanced: [CPU-06, CPU-02]
requirements-completed: []  # CPU-06 needs the ledger entries for the model parameters and the printed [model_*] record (03-18, 03-20, 03-32); CPU-02 needs the 384K and 1M variants and the bank-class oracle (03-20)

duration: 70min
completed: 2026-11-11
---

# Phase 3 Plan 17: Model parameters, tma_zero and the TAM/TMA edges Summary

**The core has a named, validated, snapshot-carried modelling parameter, tma_zero {bus_latch, a_unchanged}, selected only through ds_model_set before the first step; TMA #0, multi-bit TAM and TMA and TAM #0 are single documented constants probed from the guest as ASSUMED:LED-CPU-0015, and the same mmu_mpr ROM gives different observed bytes under the two values.**

## What was built

- **Task 1 (tracer), commit d7c8a4b.** `model_params.def` (one row), `model_refusals.def` (three codes, prefix `model.`) and `model.c` (`ds_model_set`, `ds_model_value`, row accessors) with the instance member `tma_zero`; `cpu.c` implements TMA #0 under both values and the latch writes (the multi-bit constants of D-105 were already in the table code and are now commented as single constants); the runner takes `model.NAME=VALUE` in `parse_spec` and applies it after boot (`apply_model`). mmu_mpr check 49 is an observe-only row: TAM $F8, A = $5A, TMA #0; the cases `mmu_mpr_tma_zero_latch` (observe=49:248) and `mmu_mpr_tma_zero_keep` (observe=49:90) run the same ROM; `mmu_mpr_256k` carries the bus_latch item; `mmu_mpr_observe_missing` is the control (harness_failure:observe-missing). Tracer verify (`build`, `core-boundary`, `fixtures`, `conformance`) passed, and a scratch edit that made TMA #0 ignore the parameter turned `mmu_mpr_tma_zero_keep` guest_failure before the commit (the tracer can fail). The task carries no `blocking-human` gate; auto-mode re-run of the verify end to end passed, so the expansion went ahead.
- **Task 2, commit 52a664c.** mmu_mpr checks 50 to 60: TAM $06 (written as `.byte $53, $06` with `; RAW:S8-90`) with A = $FA writes MPR1 and MPR2 ($FA, a RAM mirror, so the map stays valid) and spares MPR3 (3 rows); TAM #0 with A = $FB leaves MPR0 (set first), MPR1 and MPR7 (3 rows); TMA with operands $06, $0A and $FF (`.byte $43, ...` with `; RAW:S8-101`) returns MPR2 $F9, MPR3 $FF and MPR7 $00, the highest selected index, with A preloaded to $77 (3 rows); two more observe rows (59: the latch after a multi-bit TMA, 60: the latch after a no-op TAM #0). Observe values: latch case 248, 249, 249; keep case 90, 17, 34. The core semantics were already in the Task 1 commit (cpu.c), so the TDD order was red-by-mutation: five scratch mutations each turned the mmu_mpr cases guest_failure (TAM writing only the first selected register, TMA returning the lowest index, TAM #0 writing MPR0, TMA not updating the latch, TAM #0 updating the latch). Manifest hash re-pinned (host `shasum -a 256` equals the emitted hash: a77af5cf...).
- **Task 3, commit 7509dff.** BUS0 grew a third byte (the tma_zero selection index) and `DS_SERIAL_VERSION` is 4; `digest_stream_prefix` (version, frames, literal), `digest_region`, `digkinds.c` and both goldens changed in the same commit. Kind `model_refuse` (variants per code: 11 bad names incl. NULL, 13 bad values, and the state sequence: valid selection accepted before the first step and after a zero-step run, refused after the first step whether or not it equals the held value, bad arguments after the first step keep their own code, NULL instance is model.state) with the whole instance and digest unchanged around every refused call; deferred kind `model_refusal_coverage` (case `model_param_refusal`) requires every member of `model_refusals.def` observed and every row of `model_params.def` well formed (at most 4, two allowed values or more, default first, ledger id shape, status assumed or open).

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `core-boundary`, `fixtures`, `conformance`; Task 2 `fixture-lint`, `fixtures`, `conformance`; Task 3 `conformance`, `sanitizers`.
- Plan end: `sh scripts/check` PASS (55 PASS lines, toy controls including shared-state and unmapped-value RED_OK), `sh scripts/check --selftest` PASS (60 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 49a4ca7..HEAD` clean (3 commits, 41 blobs, 1281 blob-path pairs, run before this docs commit).
- Digest literals computed outside the C code: a perl script (scratch directory, not committed) rebuilt the three version 3 streams and reproduced the committed digests af54a963..., cc8d3b89... and 617a12f3... first, then appended `00 00 00` to BUS0 and set the version to 4: 5065a6c1... (digest_stream_prefix), 3d70806f... (256K zero golden), 1843d592... (384K seeded golden), and for the non-default selection (`00 00 01`) 7fc0dc11... (digest_stream_model). The stream files hash to the same values under `shasum -a 256`. The C output matched on the first run.
- Bite shown with scratch edits (each restored byte for byte, nothing committed): TMA #0 ignoring the parameter (keep case red); five TAM/TMA edge mutations (see Task 2); BUS0 without the selection byte (digest_stream_prefix, digest_stream_model, digest_region, digest_model_tma_zero_differs and both goldens red); no state check (model_refuse_state red); name matched by first letter (model_refuse_unknown_name and model_refuse_state red); a prefix-accepting value match (model_refuse_value red); a setter that writes before validating the value crashes the runner on the NULL-instance variant (harness ERROR, so it cannot pass).
- A scratch mutation sweep case over mmu_mpr (not committed; plan 03-20 adds the real ones) gave `mutations=57 excluded=3` with `exclude=49,59,60`: each of the 57 ordinary literals flips exactly its own check.
- Manual facts: S8-90 and S8-101 were read as page images from a scratch copy of the software manual (sha256 51eeda39..., equal to 03-RESEARCH): the function line is `MPRi <- A` and `A <- MPRi`, the machine code `53, 2^i` and `43, 2^i`, i = 0 to 7, T flag 0; there is no statement for a zero or multi-bit operand. Nothing was committed or copied from them.

## Deviations from Plan

**1. [Rule 3 - structure] A new core file `src/core/model.c`.** The plan names cpu.c, ds_core.h and the defs; the setter and tables are a self-contained unit (about 200 lines) and cpu.c is already the largest core file, so they live in model.c (the CMake glob picks it up; core-boundary and sanitizers pass).
**2. [Rule 3 - consequence of the files list] `fixtures/lib` RAW cite form.** The plan says the multi-bit lines carry "RAW: ASSUMED:LED-CPU-0015"; the lint requires `RAW:<page>` with a page label from cites.txt, so the lines read `; RAW:S8-90 ASSUMED:LED-CPU-0015: ...` and `; RAW:S8-101 ASSUMED:LED-CPU-0015: ...` (the page names the instruction, the ledger id names the assumption).
**3. [Plan wording] Task 2's cpu.c edit is in Task 1's commit.** The TAM and TMA edge constants and the latch writes are in the TMA #0 tracer because TMA #0 and the latch cannot be separated from the rest of the TAM/TMA semantics; Task 2 is the fixture section and the cases.
**4. [Plan wording] `field_flip.toml` is not edited.** As in plans 03-03 and 03-16, the field_flip kind flips profile fields only. The instruments that show the BUS0 selection byte are `digest_region` (flipping the last byte of the BUS0 span changes the digest), the new `digest_stream_model`, `digest_model_tma_zero_differs` and the three-byte frame check of `digest_stream_prefix`.
**5. [Rule 2 - beyond the plan list] Extra cases and checks.** Three observe rows and nine ordinary rows instead of "the TMA #0 check" (the latch is observable only through TMA #0, so rows 59 and 60 are observe rows too and every case carries three items); `digest_stream_model`, `digest_model_tma_zero_differs`, `digest_model_default_same` (a selection that is not in the stream would otherwise pass every digest case); and `block_boot` in digkinds.c applies model keys. `model_refusal_coverage` also checks the shape of every row of `model_params.def`.
**6. [Note] No sweep case for mmu_mpr exists yet.** The plan's must-have names "every mmu_mpr sweep case"; plan 03-20 creates `mutate_sweep_mmu_mpr` and the variant sweeps. They must carry `exclude=49,59,60` (the observe rows). A scratch sweep with that list passed (above).
**7. [Count] 12 mmu_mpr rows were added (49 to 60), 3 observe, 9 ordinary;** the DSV1 limit is 96.

No Rule 1 or Rule 4 deviation. The shared-state toy control stayed RED_OK throughout, so the standing note was not triggered.

## Evidence discipline

E0 only. tma_zero, the multi-bit and zero edges of TAM and TMA, and the one-byte latch are modelling assumptions (ASSUMED:LED-CPU-0015, status assumed): the manual defines the operand 2^i only (S8-90, S8-101), four emulators disagree on the rest (ER-8), and the plan chooses one constant per edge and one parameter for TMA #0. The expected literals of mmu_mpr checks 49 to 60 and of the observe items come from the stated rules (the latch holds the byte of the last TAM or TMA, the highest index wins, A is unchanged), a hand derivation of the registers the fixture itself wrote, or the reset value MPR7 = $00 (H8-18); none comes from DuoSpool output or another emulator. Passing says that the core does what the parameter and the constants say, and that both values are exercised and distinguished; it says nothing about the hardware. The digest literals show that the stream encoding is as documented and repeatable, not that any state is right. No ledger entry is written here (LED-CPU-0015 is written by plan 03-18, which names the three edges and both values); no ledger status changed and none of the eight ER-2 candidates was touched. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. `mpr_latch` now has its writers (TAM and TMA); the VDC, VCE, PSG, timer, interrupt-register and I/O stubs of plan 03-16 are unchanged. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-47 (ds_model_set): the name and the value are untrusted text compared exactly against the def tables (no prefix, no case folding) before any write; the refusal after the first step; the instance and its digest are byte-identical after every refusal (model_refuse cases). T-03-48 (silent modelling choice): the parameter row carries its ledger id and status; both values are exercised by fixtures; the `[model_*]` printing and the ledger entry are plan 03-18's.

## Notes for next plans

- **03-18 (ledger and record):** write LED-CPU-0015 (TAM/TMA edges: multi-bit TAM writes every selected MPR, TAM #0 a no-op including the latch, multi-bit TMA returns the highest index, TMA #0 is the parameter tma_zero {bus_latch default, a_unchanged}, the latch is the byte of the last TAM or TMA with a nonzero operand; disagreement column per ER-8: four emulators, three ways; pins `TEST:mmu_mpr_tma_zero_latch`, `TEST:mmu_mpr_tma_zero_keep`, `TEST:model_refuse_state` and so on). Print `[model_tma_zero]` (value, ledger id, status, bound_by = default|run-request) and add LED-CPU-0015 to `[withheld]`; the accessors are `ds_model_name/allowed/default/ledger/status` and `ds_model_value(inst, index, &len)` (the value is a pointer into the allowed text with a length, not NUL-terminated). The record printer needs to know whether a value came from the default or from a model key: `ds_model_set` does not record that, so a `bound_by` flag must be added to the instance (and then to a digest region or kept out of the digest deliberately) or derived by the runner from the request.
- **03-20 (variants, sweeps):** `mutate_sweep_mmu_mpr` and the 384K and 1M sweeps need `exclude=49,59,60`. mmu_mpr has 60 checks now (the new edge section sits after check 48 and uses MPR1, MPR2 and MPR0 and leaves MPR0 = $F8; MPR3 is still $FF at the end). Every case that runs the mmu_mpr ROM (mmu_mpr_384k, mmu_mpr_1m, NW1) needs `observe=49:248;observe=59:249;observe=60:249` (default bus_latch) or the keep values 90, 17, 34 and `checks = 60`. The edge section assumes MPR1 = $F8 and MPR2 = $F9 from the startup code.
- **03-22 (second parameter irq_i_sample):** a new row is one line in `model_params.def` plus its member in `ds_instance` (the name of the row), an enum value if the code needs one, a region payload byte and a serial bump with the same perl derivation; the first allowed value is the default; `DS_MODEL_COUNT` may not pass 4. `SPEC_MODEL_MAX` is 4. A row without a fixture that distinguishes both values must not be added (D-113).
- **03-24 (policies):** `apply_model` runs after `ds_boot`, so a power-on policy draw (v2, which draws MPR0 to MPR6) is not affected by a model key. The guest kind's policy fragments are appended to the case request, so `model.NAME` keys in the request and `bind.powerup_policy` fragments coexist.
- **03-30 and later (snapshot):** BUS0 is three bytes (io_buffer, mpr_latch, tma_zero); `ds_instance` has the new member `tma_zero` between `mpr_latch` and `rom`, so the tiling list must include it. A restore must validate the selection byte against the number of allowed values (domain check, D-121 pass 1).
- **Runner inputs:** no new runner option or case key was added, so the three runner invocations are unchanged; the new request key `model.NAME` is parsed by the shared `parse_spec`.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/model_params.def, src/core/model_refusals.def, src/core/model.c, and the ten new case files. Commits d7c8a4b, 52a664c and 7509dff are ancestors of HEAD.
