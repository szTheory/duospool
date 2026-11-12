# Phase 3 plan check, round 2

Verdict: REVISE (0 blockers, 13 warnings, 17 info). All round 1 blockers resolved. Saved by the orchestrator.

## Warnings and fixes

1. Plan 02 T2: name the kind_advance removal (stepkinds.c pstate/apply_items, runner.c row, runner.h line) and list those files.
2. Plans 04 T2 and 32 T3: inventory mini-roots need tests/conformance/REQUIRED, cases/ and ledger/entries or inventory-check exits 2.
3. Plans 33 T1 and 38 T1: docs-check mini-roots need README.md, AGENTS.md and scripts/check; plan 38 also needs the preflight source its rule reads.
4. Plan 06 T2: authored-redistribution-committed control has no rule; add FIX_AUTHORED_REDIST with a control or move it to rights-negatives; fix the spec-literal example.
5. Plan 15 T1: selftest mutation-opcode-table-tsv is not created by the action; add it and scripts/check to the task files.
6. Plans 35 T2 and 10: the run_refuse_sink_unsupported row must pass a non-NULL dev sink.
7. Plans 11, 12, 13, 14, 16, 21 vs 24: say "no policies key, default list"; add a plan 24 guard that rejects an explicit policies key on a REQUIRED guest case except documented overrides.
8. Plan 05: check.inc needs opcodes outside plan 01's subset; list cpu.c and the added opcodes or fit the subset.
9. Plans 17, 20, 22, 24: define observe-only rows (per-case expected observed byte), classed in .checks and exempted from the sweep by an explicit exclude list, or use one ROM variant per parameter value.
10. Plans 25 and 20: sweeps for cpu-blk-irq, cpu-blk-len0, mmu_mpr_384k and mmu_mpr_1m variants.
11. Plan 26 T2: name a pure-query pending accessor (plan 21 or 22) or add it to plan 26 files.
12. Plans 30 and 32: red control for SNAPSHOT_HOST_HASH_MISMATCH (tamper or shim), dead-stage selftest for rule-const-ledger.
13. Renumber the six T-03-SC threats to T-03-101..106.

## Info worth acting on in the same pass

I2 files lists omit files the actions require (plans 09, 10, 11, 15, 20, 22, 24, 27, 29, 30, 01). I3 plan 05 re-literals ds_proto so the plan 01 selftest target may move. I4 D-93 sink overlap rows. I5 refusal rows for late/state/level codes. I6 [constants] rows are generic. I7 prefer spd_speed and cpu_blk as pins for LED-CPU-0006 and 0008. I10 plan 02 budget_spin max step bound. I11 plan 15 UNDEF row compare. I14 avoid "boots". I15 epoch is not in the digest stream. I16 plan 21 T2 private timer test entry.
