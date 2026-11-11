---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
fixed_at: 2026-11-11T00:00:00Z
review_path: .planning/phases/03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad/03-REVIEW.md and 03-REVIEW-gate.md
iteration: 1
findings_in_scope: 10
fixed: 10
skipped: 0
status: all_fixed
---

# Phase 3: Code Review Fix Report

**Fixed at:** 2026-11-11
**Source reviews:** `03-REVIEW.md` (core) and `03-REVIEW-gate.md` (gate scripts)
**Scope:** every Critical and Warning finding (core WR-01..WR-03, gate CR-01 and WR-01..WR-05), plus the cheap Info items (core IN-02, gate IN-01..IN-04). Core IN-01 and IN-03 are deferred (reasons below).

**Where the gates ran:** the main checkout on the current branch, no worktree. `sh scripts/check` (every stage PASS), `sh scripts/check --selftest` (124 SELFTEST lines) and `sh scripts/audit-history --range a66fb81..HEAD` all passed after the last fix commit. Gate controls were also shown against the previous script, in scratch copies outside the repository, as stated per row.

## Result per finding

| Finding | Status | Commit | Control that proves it |
|---------|--------|--------|------------------------|
| gate CR-01 record-lint awk piped into sort | fixed | 566db7f | awk-exit-status shim cases `record-lint-violations` and `record-lint-codes` (FAIL on the old script, PASS on the new) |
| core WR-01 BRK vs pending IRQ under `pre` | fixed, requires human verification (logic) | 89cf1c8 | case `brk_irq_sample` (both values of `irq_i_sample`) and toy control `brk-sample-pre` (CONTROL RED_OK on the old rule) |
| core WR-02 dev-record pc for block steps and entry | fixed | 1ff65bc | case `dev_record_pc` (TII, six records, all pc 0xE004) and toy control `dev-pc-from-instance` (RED_OK on the old rule) |
| core WR-03 restore must require timer_origin <= TIME | fixed | 7ff6176 | rows `timer-origin-ahead` and `timer-origin-far` (refused as `snapshot.field`) and `timer-origin-at-time` (accepted) in `snapshot_refusal_fields` and `snapshot_refusal_api`; toy control `snapshot-origin-any` (RED_OK) |
| gate WR-01 fixture-lint case and glue blindness | fixed | 77dc6a9 | mini-roots `size-hint-upper`, `size-hint-glued-label`, `unknown-mnemonic-upper` (EXPECT_MISMATCH on the old script) |
| gate WR-02 `.incbin` unchecked | fixed | da6edfb | mini-root `incbin` (new code `FXL_INCBIN`; EXPECT_MISMATCH on the old script) |
| gate WR-03 inventory-defs indented rows and sentinel | fixed | 005c916 | mini-roots `const-indented-unknown-ledger` and `const-no-rows` (EXPECT_MISMATCH on the old scripts) |
| gate WR-04 trace_out_check accepts zero instruction records | fixed | bdc2a32 | selftest `trace-out-empty` through the new mode `conformance-run trace-out-check` and a shim runner (FAIL with the rule removed) |
| gate WR-05 xasm-check optional WLA path | fixed | ffdf1ba | shim `wla-huc6280` and `wlalink` controls `xasm-wla-failed`, `-equal`, `-brk-only`, `-other-diff`, `-hollow-stage2` over the mini-root `ok-tsv-brk` (the old script is CONTROL BROKEN on brk-only and hollow-stage2) |
| core IN-02 stale comments | fixed | ea7d1d6 | none needed (comment text only) |
| gate IN-01 awk-left-of-pipe sites | fixed | 22156fb | awk-exit-status cases `determinism-run-cases`, `conformance-run-ids`, `conformance-run-control-ids`, `docs-check-supported-list`, `core-boundary-defined` (all FAIL on the previous scripts) |
| gate IN-02 opcode-check attribution | fixed | a96f6f0 | `opcode-oracle-negatives` now requires the repeated tsv opcode to be reported against the table file (CONTROL BROKEN with the old script) |
| gate IN-03 WF_NO_CC65_STEP echo | fixed | 793d9bf | mini-root `tests/workflow/cc65-echo-only` (EXPECT_MISMATCH on the old rule) |
| gate IN-04 `--out` guard, variants-list source, ledger glob | fixed | a896ff8 | `fixture-build` stage controls `fixture-build-bad-out` and `fixture-build-missing-variant-source`; mini-root `ok-ledger-other-name` (HARNESS_NO_FILE on the old glob) |
| core IN-01 duplicated opcode and address constants | deferred | none | see below |
| core IN-03 `ds_run` request and result overlap with the instance | deferred | none | see below |

## Behaviour changes, goldens and ledger

- **WR-01 changes behaviour.** The sample after BRK now uses the I the BRK left (set) under both values of `irq_i_sample`. `DS_BEHAVIOUR_EPOCH` goes 2 to 3. Erratum **R-9** is recorded in `03-CONTEXT.md` (Research errata) and in `LED-IRQ-0005` (statement, a comment block, one evidence line, one withheld line) and in the `model_params.def` comment. The ledger status stays `open`. No existing case literal depended on the old behaviour: `irq_ctl` window B runs BRK with I already set, `cpu_flow` and `cyc_probe` run BRK with no interrupt pending.
- **WR-02 changes the content of the dev sink.** `struct ds_bus` gains `pc`; `ds_obs_dev` takes it as a parameter; `ds_obs.h` states what the records carry (opcode address for an instruction and every step of a block transfer, the interrupted next address for an interrupt entry). `DS_BEHAVIOUR_EPOCH` goes 3 to 4.
- **What moved because of the epoch bumps:** the `behaviour_epoch` line of both goldens (`accept_256k_zero.record.toml`, `accept_384k_seeded.record.toml`), the refusal-record literal in `tests/conformance/reckinds.c` and the `core.behaviour_epoch=` pin of `record_skeleton_256k`, each written by hand as the literal 4 (the epoch is not in the digest stream, so no digest, profile hash or content hash moved). The record-lint and determinism mini-roots pin no epoch value and were not edited.
- **WR-03 reuses `snapshot.field`.** No new refusal code, so `snapshot_refusals.def` has no new row and the refusal coverage kind needs none; the `field` row text names the new case. It changes no reachable machine, so the epoch stays 4. The check covers a disabled timer too (a stopped timer keeps a stale origin that is still at most the machine time), which is wider than "when enabled".
- **New REQUIRED cases:** `brk_irq_sample`, `dev_record_pc` (the inventory has 236 required cases, from 234 at the start of this work; the two new cases use the new kinds in `tests/conformance/reviewkinds.c`, added to the runner sources in `CMakeLists.txt`). New toy controls: `brk-sample-pre`, `dev-pc-from-instance`, `snapshot-origin-any` (16 toys in all, every one RED_OK).
- **Literal sources.** The BRK image and the TII image are hand-assembled in `reviewkinds.c` from opcodes of `tests/oracle/huc6280-opcodes.tsv`; the expected log bytes, the pc 0xE004 and the six record count come from the instruction definitions, and the IRQ rise at 150 master clocks from the manual cycle counts (LDA# 2, TAM 5, CLI 2, BRK 8 at 12 per cycle). The snapshot rows read the TIME payload of the stream they edit. No expected literal comes from DuoSpool output.

## Deferred (not applied)

### core IN-01: opcode and address constants duplicated across files

**Reason:** the fix touches four files (`snapshot.c`, `run.c`, `cpu.c`, `bus.c`) and moves opcode numbers and the B-bit mask into `ds_core.h`, or folds the CSL/CSH cost into `ds_cpu_op_max_cycles`. That is a refactor of core plumbing across the step guard and the restore decoders, not a small and safe change; it needs its own plan and a check that ties the copies to `cpu_ops.def`.

### core IN-03: `ds_run` does not refuse a request or result overlapping the instance

**Reason:** it adds new `memory_invalid` refusals to `ds_run`, which means an order-of-checks decision, new refusal rows and cases in the run refusal inventory, and a behaviour change for callers. The review accepts documenting the contract instead; that choice belongs to the owner.

## Notes for the orchestrator

- **WR-01 needs a human look.** It is a logic change with a modelling decision in it: BRK is sampled like an interrupt entry, with the I it left. The case pins the order of the two handlers under both parameter values; it says nothing about hardware.
- **`fixture-build` has a new `--partial` flag.** The ten scratch-root selftest invocations in `scripts/check` pass it, because their roots hold one source and the strict check now refuses a variants-list stem with no source. The real gate stage runs strict.
- **WLA-DX was not installed** and no real WLA tool was run; the controls use shim executables in a scratch directory. A machine that has WLA-DX now gets `STATUS xasm ok ... wla=known-diff-brk` and one `XASM_WLA_DIFF` line instead of a red gate; `wla=equal` is still printed only for an identical image.
- **Not done:** `dash scripts/check` (optional in the brief) was not run.

---

_Fixed: 2026-11-11_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
