---
phase: "03"
slug: "huc6280-cpu-bus-timer-interrupts-and-2-button-pad"
status: complete
nyquist_compliant: true
wave_0_complete: true
created: "2026-11-10"
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution. The requirement to check to stage map, negative controls, oracles and failure signals are in `03-RESEARCH.md` § "Validation Architecture". This file is the execution-time contract derived from it.

> `nyquist_compliant` stays false until execution proves the map: every row below green, every negative control seen red, the full gate, `--selftest`, dash run and audit-history clean. It is flipped in the last plan, never earlier.

> **Authority.** The 38 plans `03-NN-PLAN.md` are authoritative for stage names, case ids and verify commands. The planner is still revising some plans, so rows here are derived from the plan set as of 2026-11-11.

> Plans 03-34..03-38 are splits: 34 W4 ledger, 35 ds_run refusals, 36 placeholder state removal, 37 cites.txt and fixture-lint, 38 README, docs-check, W1 ledger and timing; on any mismatch the plan wins and this file is updated. Run ids in `03-RESEARCH.md` are proposals.

> **Evidence discipline.** Every green here is E0 repeatability. In-guest timer and speed checks are E2 self-consistency, circular on CPU speed (ER-3). Manual-page literals are E1 documentary only after page-image verification. VDC/VCE penalty and speed-switch behaviour are ASSUMED, no oracle. Nothing is `supported` without an independent oracle.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None added. The sh+awk gate (`scripts/check`) is the only judge; the C conformance runner `build/gate/ds_runner` prints `RESULT <id> <outcome> assertions=<n> evidence=<relpath>` and `conformance-ingest` judges them (D-57, D-60). Guest fixtures are judged from the DSV1 block in guest RAM |
| **Config file** | `CMakePresets.json`, `tests/conformance/cases/<id>.toml`, `tests/conformance/REQUIRED`, `tests/conformance/EXCLUDED_IN_MODE`, `fixtures/manifest/*.toml`, `ledger/REQUIRED` |
| **Quick run command** | `sh scripts/check --stage NAME` (new stages: `fixture-build`, `fixture-lint`, `xasm`, `opcode-oracle`, `input-check`, `profile-records`; each with a `-negatives` partner) |
| **Full suite command** | `sh scripts/check`, then `sh scripts/check --selftest`, then `dash scripts/check`, then `sh scripts/audit-history`; the sanitizer stage runs inside the full gate |
| **Selftest** | `sh scripts/check --selftest [NAME]`: every new checker has a removal mutation that must turn its control red |
| **Estimated runtime** | Baseline 75.7 s wall and 48 CHECK lines before Phase 3 (slowest `sanitizers` 10 s, `conformance` 10 s, `named-controls` 7 s). Target 120 s warm and quiet; stages over 20 s warn |
| **Time rule (D-60, D-132)** | Per-stage time is recorded with `--timing` and never fails the gate. Measure after W1 and W8; if CI critical path nears half the 10-minute budget, run toys and sweeps under one compiler |

---

## Sampling Rate

- **After every task commit (D-81):** the task's own stage-granular command from the map below, `sh scripts/check --stage NAME` or `--selftest NAME`; a few seconds each
- **After every wave (wave 1..38 in execution order, grouped W1..W8, labelled):** full `sh scripts/check` green with goldens regenerated and that wave's ledger entries written; `sh scripts/check --timing` recorded
- **After plans that add a checker:** also `sh scripts/check --selftest` for its mutation
- **Before `/gsd-verify-work`:** full gate green on gate and sanitize presets, `sh scripts/check --selftest`, `dash scripts/check`, `sh scripts/audit-history`, one `sh scripts/linux-repro` run labelled local, not a CI claim
- **Max feedback latency:** 120 s for the whole gate; per-task stage commands are much shorter

---

## Per-Task Verification Map

Keyed by plan id and task, ordered by execution wave. Execution order is 01 02 35 03 36 04 05 37 06 07 38 08 to 22 23 34 24 to 33; the wave number is the plan's position in that order and the context wave (W1..W8) is in brackets.

The Check column lists each task's own `<automated>` stages in order, each run as `sh scripts/check <arg>`; a bare `sh scripts/check` is the full gate. Requirements are the plan's frontmatter set. The control column names the red case that must be seen. If a plan changes, the plan wins.

| Task | Plan | Wave | Requirement | Check (automated) | Negative control shown red | Status |
|------|------|------|-------------|-------------------|----------------------------|--------|
| 01-T1 | 01 | 1 (W1) | CORE-04, FIX-01, FIX-03 | `--stage fixture-build`, `--stage core-boundary`, `--stage conformance` | judge refuses DSV1 magic or done before the first step (harness_failure); UNTIL without done is timeout | pending |
| 01-T2 | 01 | 1 (W1) | CORE-04, FIX-01, FIX-03 | `--stage licences`, `--stage inventory`, `--stage sanitizers`, `--stage fixture-build` | empty-PATH run of fixture-build exits 2 with PREFLIGHT_MISSING ca65 and ld65; stage red if control fails | pending |
| 01-T3 | 01 | 1 (W1) | CORE-04, FIX-01, FIX-03 | `--selftest mutation-guest-wrong-literal` | selftest mutation-guest-wrong-literal: wrong literal on the SELFTEST-TARGET line is guest_failure | pending |
| 02-T1 | 02 | 2 (W1) | CORE-04 | `--stage conformance`, `--stage inventory` | run_chunk_sum chunk lists {T},{1..},{7..},{168..},{3072..} must agree; deadline or overshoot literal edit turns red | pending |
| 02-T2 | 02 | 2 (W1) | CORE-04 | `--stage build`, `--stage conformance`, `--stage named-controls`, `--selftest mutation-control-anchor` | shared-state toy re-pointed at a run.c anchor is RED_OK; mutation-control-anchor; budget_spin still ends timeout | pending |
| 35-T1 | 35 | 3 (W1) | CORE-04 | `--stage build`, `--stage core-boundary`, `--stage conformance` | run_refuse_struct_size leaves instance digest and raw bytes unchanged; refusal code must be run.struct_size | pending |
| 35-T2 | 35 | 3 (W1) | CORE-04 | `--stage conformance`, `--stage sanitizers` | run_refusal_coverage: scratch TSV without one row is guest_failure; five rows leave instance byte-identical | pending |
| 03-T1 | 03 | 4 (W1) | CORE-04, CORE-05 | `--stage build`, `--stage conformance`, `--stage record-lint` | field_flip on a CPU register changes the digest; drop-region toy still RED_OK; goldens regenerated in task | pending |
| 03-T2 | 03 | 4 (W1) | CORE-04, CORE-05 | `--stage build`, `--stage conformance`, `--stage outcome-sync` | record_* and hash cases pass unchanged after the kind move; runner.c must be under 50000 bytes | pending |
| 36-T1 | 36 | 5 (W1) | CORE-04 | `--stage build`, `--stage conformance`, `--stage record-lint`, `--stage determinism`, `--stage core-boundary` | dropped psg_noise and vdc_phase items gone; [machine] time_unit master-clocks and epoch 2 in regenerated goldens | pending |
| 04-T1 | 04 | 6 (W1) | CORE-04, FIX-03 | `--stage build`, `--stage conformance`, `--stage inventory`, `--stage fixtures` | case missing a mandatory key is still a harness error; missing optional key accepted | pending |
| 04-T2 | 04 | 6 (W1) | CORE-04, FIX-03 | `--stage inventory`, `--stage inventory-negatives` | case-optional-wrong-type gives exactly INV_CASE_TYPE; case-missing-field stays red; ok-case-optional-key passes | pending |
| 05-T1 | 05 | 7 (W1) | FIX-01, FIX-03 | `--stage fixture-build`, `--stage conformance`, `--stage licences`, `--selftest mutation-guest-wrong-literal` | literal edit in the DCK1 CHECKLIT table turns ds_proto guest_failure; mutation-guest-wrong-literal still red | pending |
| 05-T2 | 05 | 7 (W1) | FIX-01, FIX-03 | `--stage fixture-build`, `--stage conformance`, `--stage named-controls` | predone and badsum guest_failure, nodone timeout, wrongid harness_failure; t0_poisoned planted magic gives harness_failure | pending |
| 37-T1 | 37 | 8 (W1) | FIX-03, CPU-06 | `--stage fixture-lint`, `--stage fixture-lint-negatives`, `--selftest mutation-fixture-lint-orphan`, `--selftest mutation-fixture-lint-dead-stage` | orphan-check gives exactly FXL_ORPHAN_CHECK; mutation-fixture-lint-orphan; mutation-fixture-lint-dead-stage | pending |
| 37-T2 | 37 | 8 (W1) | FIX-03, CPU-06 | `--stage fixture-lint`, `--stage fixture-lint-negatives` | seven mini-roots each give exactly their FXL_* code (UNLISTED_CHECK, BAD_CLASS, COUNT, INCLUDE_PATH, SIZE_HINT, RAW_UNCITED, UNKNOWN_CITE) | pending |
| 06-T1 | 06 | 9 (W1) | FIX-01 | `--stage fixtures`, `--stage fixture-negatives` | manifest sha256 changed by one digit is FIX_HASH_MISMATCH; deleted authored entry is FIX_UNLISTED; duospool-output verdict rejected | pending |
| 06-T2 | 06 | 9 (W1) | FIX-01 | `--stage preflight`, `--stage fixture-negatives`, `--selftest mutation-preflight-ca65` | FIX_AUTHORED_REDIST and FIX_BAD_VERDICT_SOURCE mini-roots; mutation-preflight-ca65; empty-PATH preflight exits 2 | pending |
| 06-T3 | 06 | 9 (W1) | FIX-01 | `--stage ledger-entries`, `--stage licences`, `--stage rights-tree` | ledger-entries schema keys on LED-TOOL-0001; rights-tree rejects planted files | pending |
| 07-T1 | 07 | 10 (W1) | FIX-01 | `--stage workflow`, `--stage workflow-negatives`, `--selftest mutation-workflow-pin` | no-cc65-step and cc65-after-gate each give exactly WF_NO_CC65_STEP; existing 12 mini-roots keep their codes | pending |
| 07-T2 | 07 | 10 (W1) | FIX-01 | `--selftest mutation-workflow-cc65`, `--stage workflow` | selftest mutation-workflow-cc65 turns the cc65 rule red; linux-repro installs the assembler | pending |
| 38-T1 | 38 | 11 (W1) | FIX-01, CORE-04, CPU-06 | `--stage docs-claims`, `--stage docs-claims-negatives` | tests/docs/missing-tool (README without ca65) gives exactly DOCS_TOOL_MISSING | pending |
| 38-T2 | 38 | 11 (W1) | FIX-01, CORE-04, CPU-06 | `--stage ledger-entries`, `--stage inventory`, `--timing` | ledger REQUIRED or pin edit turns ledger-entries red; W1 timing recorded, never fails (D-60) | pending |
| 08-T1 | 08 | 12 (W2) | FIX-01, CPU-01, CPU-06 | `--stage xasm` | five TSV rows through matrix and ca65; empty PATH gives PREFLIGHT_MISSING and exit 2 | pending |
| 08-T2 | 08 | 12 (W2) | FIX-01, CPU-01, CPU-06 | `--stage xasm`, `--stage ledger-entries` | every TSV row's bytes equal ca65 bytes; mismatch is XASM_MISMATCH; ledger-entries on the CPU-01 pair | pending |
| 08-T3 | 08 | 12 (W2) | FIX-01, CPU-01, CPU-06 | `--stage xasm-negatives`, `--stage fixture-lint-negatives`, `--stage licence-negatives`, `--selftest mutation-xasm-compare`, `--selftest mutation-xasm-dead-stage` | xasm-negatives: tsv-byte-changed, shim ca65 flipping a byte (XASM_MISMATCH); mutation-xasm-compare, mutation-xasm-dead-stage; FXL_UNKNOWN_MNEMONIC | pending |
| 09-T1 | 09 | 13 (W2) | CPU-01, CPU-06, CORE-04 | `--stage build`, `--stage conformance`, `--stage core-boundary` | opcode-check --partial reports OPC_FIELD for any differing field; ds_proto unchanged under table-driven dispatch | pending |
| 09-T2 | 09 | 13 (W2) | CPU-01, CPU-06, CORE-04 | `--stage opcode-oracle`, `--stage build`, `--stage conformance` | all 256 rows two-way def versus TSV; missing or extra row is red | pending |
| 09-T3 | 09 | 13 (W2) | CPU-01, CPU-06, CORE-04 | `--stage opcode-oracle-negatives`, `--stage conformance`, `--selftest mutation-opcode-oracle-field`, `--selftest mutation-opcode-oracle-dead-stage` | mini-roots OPC_FIELD, OPC_MISSING_ROW, OPC_EXTRA_ROW, OPC_UNDEF_COUNT; mutation-opcode-oracle-field and -dead-stage | pending |
| 10-T1 | 10 | 14 (W2) | FIX-03, CORE-06, CPU-01 | `--stage conformance`, `--stage sanitizers`, `--selftest mutation-sweep-vacuous` | mutation-sweep-vacuous: a check comparing an immediate instead of its LIT entry is named vacuous by mutate_sweep | pending |
| 10-T2 | 10 | 14 (W2) | FIX-03, CORE-06, CPU-01 | `--stage build`, `--stage core-boundary`, `--stage conformance` | sink overlapping instance or request is memory_invalid and byte-identical; misaligned or NULL pointer refused; sink shapes equal no-sink | pending |
| 10-T3 | 10 | 14 (W2) | FIX-03, CORE-06, CPU-01 | `--stage conformance` | observe-only row design (fifth .checks column, observe=IDX:VALUE, case exclude key) with a red control case; opcode_exercise report mode | pending |
| 11-T1 | 11 | 15 (W2) | CPU-01, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | LDA mode literal edit is guest_failure; fixture-lint cite and class rules apply | pending |
| 11-T2 | 11 | 15 (W2) | CPU-01, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | per-check literal flips via sweep; zp,X and S wrap checks; PHP pushes B=1 and T=0 | pending |
| 11-T3 | 11 | 15 (W2) | CPU-01, FIX-03 | `--stage conformance`, `--stage inventory`, `--selftest mutation-policies-uninit` | mutation-policies-uninit: uninitialised RAM read differs across policies and is guest_failure; mutate_sweep_cpu_ldst | pending |
| 12-T1 | 12 | 16 (W2) | CPU-01, CPU-06, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | ADC checked against an in-guest GUEST model over a 16-value edge set; literal flip is guest_failure | pending |
| 12-T2 | 12 | 16 (W2) | CPU-01, CPU-06, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | SED before SET order (Pitfall 7); T cleared by next instruction; decimal flag sources (Pitfall 8) | pending |
| 12-T3 | 12 | 16 (W2) | CPU-01, CPU-06, FIX-03 | `--stage conformance`, `--selftest mutation-control-anchor` | adc-carry toy RED_OK; replacement equal to anchor gives NOT_RED; mutation-control-anchor; mutate_sweep_cpu_alu | pending |
| 13-T1 | 13 | 17 (W2) | CPU-01, CPU-06 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | branch marker byte written only on target path; JSR pushed-PC literal flip is guest_failure | pending |
| 13-T2 | 13 | 17 (W2) | CPU-01, CPU-06 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | BRK pushed PC and B bit literals; RTI restores P and T; unexpected vector handler sets DSV1 error flag | pending |
| 13-T3 | 13 | 17 (W2) | CPU-01, CPU-06 | `--stage fixtures`, `--stage conformance` | mutate_sweep_cpu_flow; startup.inc re-pin: stale manifest hash is FIX_HASH_MISMATCH | pending |
| 14-T1 | 14 | 18 (W2) | CPU-01, CPU-02, CPU-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | swap and clear literal flips are guest_failure | pending |
| 14-T2 | 14 | 18 (W2) | CPU-01, CPU-02, CPU-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | TAM/TMA one-hot round trip; ST0 leaves RAM[0] unchanged; RMB/SMB bit literals | pending |
| 14-T3 | 14 | 18 (W2) | CPU-01, CPU-02, CPU-04 | `--stage conformance` | mutate_sweep_cpu_huc; opcode_exercise in require mode fails on any unexercised defined non-block opcode | pending |
| 15-T1 | 15 | 19 (W2) | CPU-01, CPU-06, FIX-01 | `--stage build`, `--stage conformance`, `--selftest mutation-opcode-table-tsv` | mutation-opcode-table-tsv: TSV with one cycle changed via --opcode-tsv makes opcode_table_manual guest_failure | pending |
| 15-T2 | 15 | 19 (W2) | CPU-01, CPU-06, FIX-01 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | cycle_adders_manual page-literal edit is guest_failure; sink lost != 0 is harness_failure | pending |
| 15-T3 | 15 | 19 (W2) | CPU-01, CPU-06, FIX-01 | `--stage ledger-entries`, `--stage inventory`, `--stage ledger-negatives` | ledger-negatives (assumed-no-assumption); LED-CPU entries need real pins | pending |
| 16-T1 | 16 | 20 (W3) | CPU-02, CPU-04, CORE-06 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | bank $80 read must be $FF; RAM mirror readback through $F9-$FB; literal edit is guest_failure | pending |
| 16-T2 | 16 | 20 (W3) | CPU-02, CPU-04, CORE-06 | `--stage conformance`, `--stage record-lint`, `--selftest mutation-control-anchor` | unmapped-value toy RED_OK on mmu_mpr_256k; mutation-control-anchor; BUS0 region and window stubs lint | pending |
| 16-T3 | 16 | 20 (W3) | CPU-02, CPU-04, CORE-06 | `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | vdc_penalty_assumed literal flip; run_overshoot_bound still holds with the penalty adder; penalty class ASSUMED | pending |
| 17-T1 | 17 | 21 (W3) | CPU-06, CPU-02 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | tma_zero latch and keep cases must expect different observed bytes; ds_model_set after first step is refused | pending |
| 17-T2 | 17 | 21 (W3) | CPU-06, CPU-02 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | TAM #0 and multi-bit TAM/TMA RAW probes counted by fixture-lint; literal flip | pending |
| 17-T3 | 17 | 21 (W3) | CPU-06, CPU-02 | `--stage conformance`, `--stage sanitizers` | model_refuse_unknown_name, _value, _state each refused and byte-identical; field_flip on BUS0 changes digest | pending |
| 18-T1 | 18 | 22 (W3) | CPU-06, CORE-04 | `--stage build`, `--stage conformance`, `--stage record-lint` | record lint accepts [model_tma_zero] and [run]; golden edit turns record_golden red | pending |
| 18-T2 | 18 | 22 (W3) | CPU-06, CORE-04 | `--stage record-lint-negatives`, `--stage determinism-negatives`, `--stage conformance` | mini-root missing-model-table; record_constants_shown with changed ledger id is guest_failure; determinism-negatives | pending |
| 18-T3 | 18 | 22 (W3) | CPU-06, CORE-04 | `--stage ledger-entries`, `--stage inventory` | ledger-entries schema on LED-PROF-0004 and LED-CPU-0015; inventory pins | pending |
| 19-T1 | 19 | 23 (W3) | CORE-06 | `--stage build`, `--stage core-boundary`, `--stage conformance` | dev_log_records order check; observer_ds_proto: every sink config, whole and in 7 chunks, equals no-sink; lost != 0 harness_failure | pending |
| 19-T2 | 19 | 23 (W3) | CORE-06 | `--stage conformance`, `--selftest mutation-control-anchor` | trace-hook-mutates toy RED_OK on observer_ds_proto; replacement equal to anchor gives NOT_RED | pending |
| 19-T3 | 19 | 23 (W3) | CORE-06 | `--stage conformance`, `--stage rights-tree` | trace_out_rights refuses non-authored or synthetic recipe and values with slash or ..; rights-tree | pending |
| 20-T1 | 20 | 24 (W3) | CPU-02, CPU-06, CORE-06 | `--stage build`, `--stage conformance`, `--stage ledger-entries`, `--stage inventory`, `--selftest mutation-bankclass-drift` | mutation-bankclass-drift: edited LED-CPU-0011 clause gives ORACLE_RULE_TEXT_DRIFT | pending |
| 20-T2 | 20 | 24 (W3) | CPU-02, CPU-06, CORE-06 | `--stage fixtures`, `--stage conformance`, `--stage sanitizers`, `--selftest mutation-st-bypass-literal` | mutation-st-bypass-literal: one wrong ST value is guest_failure; 384K and 1M bank tags differ from 256K only where mirror rules differ | pending |
| 20-T3 | 20 | 24 (W3) | CPU-02, CPU-06, CORE-06 | `--stage ledger-entries`, `--stage inventory`, `--stage ledger-negatives` | ledger-negatives; LED-LOAD-0005 edit keeps schema, REQUIRED and pins | pending |
| 21-T1 | 21 | 25 (W4) | CPU-03, CORE-04 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | TIQ handler literals (I set, D clear, B=0) flip is guest_failure; $1403 write clears TIQ | pending |
| 21-T2 | 21 | 25 (W4) | CPU-03, CORE-04 | `--stage conformance`, `--stage sanitizers` | per-clock reference model disagreement over reload 0..127 and three start phases is red (C level, E1 formula) | pending |
| 21-T3 | 21 | 25 (W4) | CPU-03, CORE-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | mutate_sweep_tim_timer; same-value control write no-op check; in-guest checks E2 self-consistency | pending |
| 22-T1 | 22 | 26 (W4) | CPU-03, CPU-06, CORE-05 | `--stage build`, `--stage fixtures`, `--stage conformance` | irq_stim unsorted, over 16 events or line 3 refused before first step; handler $1403 bit 1 literal | pending |
| 22-T2 | 22 | 26 (W4) | CPU-03, CPU-06, CORE-05 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | irq_i_sample pre versus post distinguishing check must differ between the two cases; priority and BRK sharing literals | pending |
| 22-T3 | 22 | 26 (W4) | CPU-03, CPU-06, CORE-05 | `--stage conformance`, `--stage record-lint`, `--selftest mutation-control-anchor` | timer-reload-off RED_OK on tim_timer and timer_pure_function; irq-priority-swap RED_OK on irq_ctl_pre; mutation-control-anchor | pending |
| 23-T1 | 23 | 27 (W4) | CPU-04, CPU-03, CPU-06, CORE-06 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | CSH loop-rate literal flip (E2, circular on CPU speed); observer_tim_timer under every sink config | pending |
| 23-T2 | 23 | 27 (W4) | CPU-04, CPU-03, CPU-06, CORE-06 | `--stage conformance`, `--stage record-lint`, `--stage record-lint-negatives`, `--stage determinism-negatives` | mini-root missing-stimulus-table; [constants] W4 rows must keep record_constants_shown green | pending |
| 34-T1 | 34 | 28 (W4) | CPU-03, CPU-04, CPU-06 | `--stage ledger-entries`, `--stage inventory`, `--stage ledger-negatives` | ledger-negatives; LED-IRQ-0002 pin back-link must resolve | pending |
| 34-T2 | 34 | 28 (W4) | CPU-03, CPU-04, CPU-06 | `--stage ledger-entries`, `--stage inventory`, `--stage conformance` | assumed-no-assumption and missing-REQUIRED negatives on the eight W4 entries | pending |
| 24-T1 | 24 | 29 (W4) | CPU-06, CPU-02, CPU-03, CPU-04 | `--stage build`, `--stage conformance`, `--stage profile-records` | v1 poweron_draw_order literals unchanged; v2 boot record prints policy_version 2; profile-records mismatch | pending |
| 24-T2 | 24 | 29 (W4) | CPU-06, CPU-02, CPU-03, CPU-04 | `--stage conformance` | v2 draw literals from an independent perl splitmix64 recomputation; domain-independence ram@ equal to v1 | pending |
| 24-T3 | 24 | 29 (W4) | CPU-06, CPU-02, CPU-03, CPU-04 | `--stage fixtures`, `--stage conformance`, `--stage sanitizers`, `--selftest mutation-policy-diff` | mutation-policy-diff (policy-diff control); guest_policies_guard: explicit policies key without policies-override gives harness_failure | pending |
| 25-T1 | 25 | 30 (W5) | CPU-05, CPU-01, CORE-04 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance`, `--selftest mutation-control-anchor` | block-progress-lost toy RED_OK on cpu_blk_stop_resume; mutation-control-anchor; 17+6x cycles literal flip | pending |
| 25-T2 | 25 | 30 (W5) | CPU-05, CPU-01, CORE-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | length 0 copies 65,536 bytes; cpu_blk_irq_pending interrupt order after epilogue; TDD/TIN/TIA/TAI pattern literals | pending |
| 25-T3 | 25 | 30 (W5) | CPU-05, CPU-01, CORE-04 | `--stage conformance`, `--stage record-lint` | field_flip on BLKT remaining changes digest; golden edit turns record_golden red | pending |
| 26-T1 | 26 | 31 (W5) | CORE-04, CPU-05, CORE-06, CPU-06 | `--stage build`, `--stage conformance` | block-progress-lost RED_OK also on chunk_equiv_cpu_blk_len0; every schedule stops inside the transfer | pending |
| 26-T2 | 26 | 31 (W5) | CORE-04, CPU-05, CORE-06, CPU-06 | `--stage conformance`, `--stage sanitizers` | chunk_equiv_random32 (32 programs x 6 schedules) digest mismatch reported per schedule; observer_cpu_blk and observer_random | pending |
| 26-T3 | 26 | 31 (W5) | CORE-04, CPU-05, CORE-06, CPU-06 | `--stage ledger-entries`, `--stage inventory` | ledger-negatives; block ledger entries need real pins | pending |
| 27-T1 | 27 | 32 (W6) | INP-01, INP-02, INP-04 | `--stage flatten-controls`, `--stage fixtures`, `--stage conformance` | raw read literal ($30 | d6/d7 | $07) flip is guest_failure; flatten-controls | pending |
| 27-T2 | 27 | 32 (W6) | INP-01, INP-02, INP-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | pad_2btn_correct literal flips; reads must change nothing; power-on latch $FF reads as SEL=1 CLR=1 | pending |
| 27-T3 | 27 | 32 (W6) | INP-01, INP-02, INP-04 | `--stage conformance`, `--stage sanitizers` | input_port_refusal ports 0, 2-5, 6; input_refusal_rows (bits, order, queue_full, late) each refused | pending |
| 28-T1 | 28 | 33 (W6) | INP-02, INP-04 | `--stage input-check`, `--stage input-check-negatives` | input-check-negatives: at-decreasing gives exactly INP_ORDER | pending |
| 28-T2 | 28 | 33 (W6) | INP-02, INP-04 | `--stage input-check-negatives`, `--stage flatten-controls`, `--selftest mutation-input-order`, `--selftest mutation-input-check-dead-stage`, `--selftest mutation-input-flatten-refuse` | one mini-root per INP_* code; mutation-input-order, mutation-input-check-dead-stage, mutation-input-flatten-refuse | pending |
| 28-T3 | 28 | 33 (W6) | INP-02, INP-04 | `--stage conformance`, `--stage record-lint`, `--stage record-lint-negatives`, `--stage determinism-negatives` | mini-root missing-input-table; empty-schedule identity differs from a real schedule | pending |
| 29-T1 | 29 | 34 (W6) | INP-03, INP-01, INP-04, CORE-06 | `--stage input-check`, `--stage conformance`, `--selftest mutation-pad-order-assert` | none and wrong scripts pass only when exactly the declared fail-set is $46; mutation-pad-order-assert | pending |
| 29-T2 | 29 | 34 (W6) | INP-03, INP-01, INP-04, CORE-06 | `--stage conformance`, `--selftest mutation-control-anchor` | pad_2btn_late and pad_2btn_glitch declared fail-sets; input_exact_edges at-1, at, at+1; shared-input toy RED_OK on interleave_pad | pending |
| 29-T3 | 29 | 34 (W6) | INP-03, INP-01, INP-04, CORE-06 | `--stage conformance`, `--stage ledger-entries`, `--stage inventory` | field_flip on IOPT and SCHD changes digest; ledger-entries schema on W6 entries; inventory pins | pending |
| 30-T1 | 30 | 35 (W7) | CORE-05, CORE-04 | `--stage build`, `--stage core-boundary`, `--stage conformance`, `--selftest mutation-snapshot-host-hash` | mutation-snapshot-host-hash: snapshot sha256 must equal host shasum of the digest stream; poisoned restore must continue identically | pending |
| 30-T2 | 30 | 35 (W7) | CORE-05, CORE-04 | `--stage build`, `--stage conformance`, `--stage sanitizers` | snapshot_refusal_rows: each hostile field refused before any write; added member without tiling entry breaks build | pending |
| 30-T3 | 30 | 35 (W7) | CORE-05, CORE-04 | `--stage conformance`, `--stage sanitizers` | per-region perturbation changes the digest; lockstep (time, pc, opcode) at every sampled stop and boundary | pending |
| 31-T1 | 31 | 36 (W7) | CORE-05 | `--stage conformance`, `--stage sanitizers` | refused target byte-identical; accepted re-serialises equal; sanitizer abort is red | pending |
| 31-T2 | 31 | 36 (W7) | CORE-05 | `--stage conformance`, `--stage sanitizers`, `--timing` | about 500 hostile cases clean under sanitizers; random-stop restore digests identical | pending |
| 31-T3 | 31 | 36 (W7) | CORE-05 | `--stage conformance`, `--selftest mutation-restore-skip-field`, `--selftest mutation-control-anchor` | snapshot-drop-t and snapshot-drop-blkt RED_OK; mutation-restore-skip-field; mutation-control-anchor | pending |
| 32-T1 | 32 | 37 (W8) | CPU-06, FIX-01, FIX-03 | `--stage fixture-lint`, `--stage fixture-lint-negatives`, `--stage ledger-entries`, `--stage ledger-negatives` | FXL_OCR_BACKS_SUPPORTED and FXL_UNKNOWN_LEDGER_ID mini-roots; ledger-negatives | pending |
| 32-T2 | 32 | 37 (W8) | CPU-06, FIX-01, FIX-03 | `--stage ledger-entries`, `--stage inventory`, `--stage fixture-lint`, `--stage fixtures` | assumed-no-assumption negative; PENDING pins move to TEST ids only after cases exist; each supported entry meets ER-2 | pending |
| 32-T3 | 32 | 37 (W8) | CPU-06, FIX-01, FIX-03 | `--stage fixture-build`, `--stage conformance`, `--stage inventory`, `--stage inventory-negatives`, `--selftest mutation-const-ledger-dead-stage` | class counts man+guest+assumed must sum to assertions; guest judge refuses (harness_failure) when classes file is missing; mutation-const-ledger-dead-stage | pending |
| 33-T1 | 33 | 38 (W8) | FIX-01, FIX-03, CPU-06 | `--stage docs-claims`, `--stage docs-claims-negatives` | tests/docs/claim-word (README with cycle-accurate) gives DOCS_CLAIM_WORD | pending |
| 33-T2 | 33 | 38 (W8) | FIX-01, FIX-03, CPU-06 | `--timing`, `--stage xasm` | timing re-baseline recorded, never fails; WLA cross-check only when installed, no second-assembler claim otherwise | pending |
| 33-T3 | 33 | 38 (W8) | FIX-01, FIX-03, CPU-06 | `--stage ci-timings-controls`, `sh scripts/check` | ci-timings-controls malformed TSV exits 2; full gate green; owner hand-off list complete | pending |

*Status: pending, green, red, flaky. All rows start pending.*

### Requirement to evidence map (from RESEARCH, updated for the 38-plan set)

| Requirement | Main checks (plans) | Evidence tier and oracle | Failure signal |
|-------------|---------------------|--------------------------|----------------|
| CPU-01 | `opcode_table_manual` (15), `cycle_adders_manual` (15), guest `cpu_ldst` (11), `cpu_alu` (12), `cpu_flow` (13), `cpu_huc` (14), `cpu_blk` (25); stages `opcode-oracle` (09), `xasm` (08) | E1 documentary (page-image TSV), E2 guest reference models | `guest_failure` with failing check ids; `OPC_*`; `XASM_MISMATCH` |
| CPU-02 | `mmu_mpr_256k/_384k/_1m` (16, 20), `bankclass_oracle` (20), TAM/TMA (14, 17), `st_bypass` (20), policy diff (24) | E1 manual literals; awk bank-class oracle labelled E0/E2 (ER-7); $80-$F7 and RAM x4 stay historical report | `guest_failure`; evidence names bank and offset |
| CPU-03 | `timer_pure_function` (21), `tim_timer` (21), `irq_ctl_pre/_post` (22), `irq_regs_manual` | E1 C-level step and period; in-guest timer checks E2 (ER-3) | `guest_failure`; DSV1 error flag on unexpected vector |
| CPU-04 | `spd_speed` (23), `speed_manual`, VDC/VCE penalty (16) | E1 reset speed and CSL/CSH cycles; switch behaviour and penalty ASSUMED | `guest_failure` |
| CPU-05 | `cpu_blk` and `cpu_blk_stop_resume` (25), `chunk_equiv_cpu_blk(_len0)` and `chunk_equiv_random32` (26) | E1 17+6x and length 0 = 65,536; chunk identity E0 | digest mismatch per schedule |
| CPU-06 | `ledger-check`, `inventory-check`, `fixture-lint` cite rules (05, 37, 32), `model_param_refusal` (17), `[model_*]` golden (18) | Ledger discipline, no oracle claim | `LED`, `INV_*`, `FXL_*`, `FIX_UNCITED`; refusal outcome |
| INP-01..04 | `pad_2btn_correct/_none/_wrong/_late/_glitch` (27, 29), `input_exact_edges`, `input_port_refusal`, stage `input-check` (28) | E2 self-consistency; wire order stays historical report (ER-5) | `INP_*` set differs; fail-set differs; refusal code |
| CORE-04 | `run_until_abs`, `run_chunk_sum` (02), `run_refuse_*` (35), `run_overshoot_bound` (09), `chunk_equiv_*` (26) | E0 internal consistency | digest or machine-time mismatch |
| CORE-05 | `snapshot_restore_sampled`, `_boundaries`, `snapshot_tiling`, `snapshot_hostile` (30, 31), CPU0 digest region (03) | E0, host `shasum -a 256` of digest stream | memcmp or lockstep difference; sanitizer abort |
| CORE-06 | `observer_*` matrix (19, 23, 26, 29), `trace_out_rights` (19), insn sink (10), liveness toy | E0 structural | verdict, time, stop or digest differs between sink configs |
| FIX-01 | `fixture-build` (01), `xasm` (08), preflight (06), authored manifest entries (06), CI cc65 step (07) | Two encoders: ca65 and the manual TSV; WLA optional | `PREFLIGHT_MISSING`, `FIX_HASH_MISMATCH`, `XASM_MISMATCH` |
| FIX-03 | `ds_proto` and four controls (05), `mutate_sweep_*` (10 to 14, 21, 24), `fixture-lint` class rules (37), observe-only rows (10) | Expected values never from DuoSpool output | unflipped or vacuous check fails its sweep; `FXL_*`, `FIX_*` |

### ROADMAP Phase 3 success criteria

| SC | Where checked | Stated scope |
|----|---------------|--------------|
| 1 Authored fixtures with cited literals, wrong literal fails, cross-assembler identity where practical | Plans 01, 05, 37, 08 to 15, 16, 20 to 24, 25; `xasm` (08); `mutate_sweep_*` | State what ran: ca65 encodings equal the manual transcription; no second-assembler claim unless WLA ran (33) |
| 2 Pad fixture with three scripts, per-port script in mclk, differing verdicts, guest log | Plans 27, 28, 29; `input-check`, `padlog.tsv` | None, wrong and correct scripts from one ROM; late and glitch controls; E2 self-consistency |
| 3 Chunked equals uninterrupted; stops mid-block incl. length 0; snapshot restore continues identically | Plans 02, 25, 26, 30, 31 | Fixed mclk chunk sizes incl. prime sizes and timer-event ticks; no frame exists yet; E0 |
| 4 Trace and device-write log change no verdict, time or digest | Plans 10, 19, 23, 26, 29 | Observer matrix and `trace-hook-mutates` liveness toy |
| 5 Every cycle count and penalty has a ledger entry or an assumption label; parameters named | Plans 15, 17, 18, 20, 23, 34, 26, 29, 32, 38 | VDC/VCE penalty LED-CPU-0017 and speed switch LED-CPU-0006 are ASSUMED; model parameters named in `[model_*]` |

---

## Wave 0 Requirements

Infrastructure that must exist before the code that needs it. Owning plan in brackets; plan text is authoritative.

- [ ] Assembler preflight: `ca65` and `ld65` on PATH (`brew install cc65`), preflight entries that fail closed with `PREFLIGHT_MISSING`, CI apt step, tool record `fixtures/lib/TOOLS.md` [plans 01, 06, 07, 38]
- [ ] Runner headroom: record and hash kinds moved out of `tests/conformance/runner.c`, new kind files registered in `CMakeLists.txt` (Pitfall 1) [plan 03]
- [ ] Optional case keys (`until_mclk`, `checks`, `policies`, and the rest) with inventory-rules controls; the ten-key schema stays enforced (Pitfall 2) [plan 04]
- [ ] Fixture build: `scripts/fixture-build` and its stage before `build`, `fixtures/lib/hucard.cfg`, authored manifest entries [plans 01, 06]
- [ ] DSV1 judge `kind_guest` plus `startup.inc`, `verdict.inc`, `check.inc` with the DCK1 literal table, before any CPU fixture [plans 01, 05]
- [ ] `ds_proto` and its four control variants (`_predone`, `_nodone`, `_badsum`, `_wrongid`) [plans 01, 05]
- [ ] `fixtures/lib/cites.txt` seeded from the page-image table, rows flagged `img` or `ocr`, [plan 05]; `scripts/fixture-lint`, its mini-roots, stage-1 sentinel and dead-stage guard [plan 37]
- [ ] Opcode oracle: `tests/oracle/huc6280-opcodes.tsv` produced from page images by a pass that does not read `cpu_ops.def`, with `scripts/xasm-check` and `scripts/opcode-check` [plans 08, 09]
- [ ] `tests/gate` mini-roots for `fixture-lint`, `xasm`, `input-check`, authored `fixture-check`, and the empty-PATH preflight control [plans 05, 06, 08, 28, 37]
- [ ] Anchored one-line statements for the toys (`adc-carry`, `unmapped-value`, `trace-hook-mutates`, `timer-reload-off`, `irq-priority-swap`, `block-progress-lost`, `shared-input`, `snapshot-drop-t`, `snapshot-drop-blkt`) (Pitfall 4) [plans 12, 16, 19, 22, 25, 29, 31]
- [ ] Run API migration: `ds_run` absolute-deadline model (plans 01, 02), `run_refusals.def` with the coverage row (35), placeholder state and domains removed, master-clock record epoch 2 (36)
- [ ] Observe-only row design: fifth `.checks` column, `NAME.observe`, `observe=IDX:VALUE` and sweep `exclude` key, defined once before plans 17, 20, 22 and 24 use it [plan 10]
- [ ] Policies guard: `guest_policies_guard` control and the default policy list in `kind_guest` [plans 11, 24]
- [ ] Ledger split: W1 entries (38), W2 (15), W3 (18, 20), W4 (34), W5 (26), W6 (29), finalisation (32); each needs real pins before `PENDING` is cleared

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| CI evidence for the Phase 3 gate: first green GitHub Actions run, critical-path timing row | CI-01 follow-through | Pushing needs explicit owner authorisation (D-64); until then the claim is unverified-pending-push and plan 33 pushes nothing | After the owner authorises: push a `gsd/phase-3-*` branch, open a PR, run `sh scripts/ci-timings <run-id>`, append the row to `docs/ci-timings.md` |
| Page-image verification of manual and SW-page literals behind every `img` cite | CPU-06, FIX-03 | An executor reads the page image with the verifier; OCR alone stays flagged `ocr` and cannot back a supported entry | Each ledger entry records the verifier, the date and the fact seen; `cites.txt` rows carry the same; plan 32 promotes entries only on those records |
| Hardware captures | none | Optional and not needed: no claim in this phase depends on physical hardware | Not scheduled. Any later capture gets provenance and is a separate owner decision |

Everything else has automated verification. Authority gates are recorded hand-offs, not UAT.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or a Wave 0 dependency
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Every negative control in the map seen red once, and every new checker has a `--selftest` mutation
- [ ] Feedback latency under 120 s for the full gate on a quiet machine; time is recorded, never a failure (D-60)
- [ ] No row claims more than E0, E1 documentary or E2 self-consistency; nothing marked `supported`
- [ ] `nyquist_compliant: true` set in frontmatter only after the full suite, `--selftest`, dash run and audit-history are green

**Approval:** pending
