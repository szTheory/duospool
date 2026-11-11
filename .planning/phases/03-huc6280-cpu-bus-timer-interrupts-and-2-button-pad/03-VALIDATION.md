---
phase: "03"
slug: "huc6280-cpu-bus-timer-interrupts-and-2-button-pad"
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-11-10"
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution. The requirement to check to stage map, negative controls, oracles and failure signals are in `03-RESEARCH.md` § "Validation Architecture". This file is the execution-time contract derived from it.

> `nyquist_compliant` stays false until execution proves the map: every row below green, every negative control seen red, the full gate, `--selftest`, dash run and audit-history clean. It is flipped in the last plan, never earlier.

> **Authority.** The 33 plans `03-NN-PLAN.md` are authoritative for stage names, case ids and verify commands. The planner is still revising some plans, so rows here are derived from the plan set as of 2026-11-10; on any mismatch the plan wins and this file is updated. Run ids in `03-RESEARCH.md` are proposals.

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
- **After every wave (W1..W8, labelled):** full `sh scripts/check` green with goldens regenerated and that wave's ledger entries written; `sh scripts/check --timing` recorded
- **After plans that add a checker:** also `sh scripts/check --selftest` for its mutation
- **Before `/gsd-verify-work`:** full gate green on gate and sanitize presets, `sh scripts/check --selftest`, `dash scripts/check`, `sh scripts/audit-history`, one `sh scripts/linux-repro` run labelled local, not a CI claim
- **Max feedback latency:** 120 s for the whole gate; per-task stage commands are much shorter

---

## Per-Task Verification Map

Keyed by plan id and task. The Check column lists each task's own `<automated>` stages in order, each run as `sh scripts/check <arg>`; a bare `sh scripts/check` is the full gate.

Wave is the plan's sequential wave with the context wave (W1..W8) in brackets. Requirements are the plan's frontmatter set. The control column names the red case that must be seen. If a plan changes, the plan wins.

| Task | Plan | Wave | Requirement | Check (automated) | Negative control shown red | Status |
|------|------|------|-------------|-------------------|----------------------------|--------|
| 01-T1 | 01 | 1 (W1) | CORE-04, FIX-01, FIX-03 | `--stage build`, `--stage core-boundary`, `--stage conformance` | judge: DSV1 magic or done present before first step is harness_failure; non-done UNTIL is timeout | pending |
| 01-T2 | 01 | 1 (W1) | CORE-04, FIX-01, FIX-03 | `--stage fixture-build`, `--stage conformance`, `--stage licences`, `--stage inventory` | fixture-build with empty PATH exits 2 with PREFLIGHT_MISSING ca65 | pending |
| 01-T3 | 01 | 1 (W1) | CORE-04, FIX-01, FIX-03 | `--stage conformance`, `--stage sanitizers` | scratch wrong literal gives guest_failure (check 1 at $46); until=1200 gives timeout; ds_proto_until_now zero-step case | pending |
| 02-T1 | 02 | 2 (W1) | CORE-04 | `--stage conformance`, `--stage inventory` | run_until_abs edit (deadline or overshoot literal) turns the case red; chunk-sum identity | pending |
| 02-T2 | 02 | 2 (W1) | CORE-04 | `--stage build`, `--stage conformance`, `--stage named-controls`, `--selftest mutation-control-anchor` | shared-state toy re-pointed at a run.c anchor is RED_OK; mutation-control-anchor | pending |
| 02-T3 | 02 | 2 (W1) | CORE-04 | `--stage conformance`, `--stage core-boundary`, `--stage sanitizers` | six run refusal rows each leave the instance byte-identical; coverage row fails if a refusal has no case | pending |
| 03-T1 | 03 | 3 (W1) | CORE-04, CORE-05 | `--stage build`, `--stage conformance` | field_flip row for CPU0 changes the digest; drop-region toy still RED_OK | pending |
| 03-T2 | 03 | 3 (W1) | CORE-04, CORE-05 | `--stage conformance`, `--stage core-boundary` | placeholder domains must be absent; power-on case literal edit turns red | pending |
| 03-T3 | 03 | 3 (W1) | CORE-04, CORE-05 | `--stage conformance`, `--stage record-lint`, `--stage record-lint-negatives`, `--stage determinism`, `--stage determinism-negatives` | record-lint-negatives and determinism-negatives (differs control); golden byte edit | pending |
| 04-T1 | 04 | 4 (W1) | CORE-04, FIX-03 | `--stage build`, `--stage conformance`, `--stage inventory`, `--stage fixtures` | case with missing optional-key partner is INV_CASE_FIELD; ds_proto passes with machine_time and stop=until | pending |
| 04-T2 | 04 | 4 (W1) | CORE-04, FIX-03 | `--stage build`, `--stage conformance`, `--stage outcome-sync` | every Phase 2 case still passes after kind move; outcome-sync row drift | pending |
| 04-T3 | 04 | 4 (W1) | CORE-04, FIX-03 | `--stage inventory`, `--stage inventory-negatives` | case-unknown-key gives INV_CASE_FIELD; case-optional-wrong-type (until_mclk = "x"); case-missing-field | pending |
| 05-T1 | 05 | 5 (W1) | FIX-01, FIX-03, CPU-06 | `--stage fixture-build`, `--stage conformance`, `--stage licences` | DCK1 literal table edit turns ds_proto guest_failure; build is byte-stable | pending |
| 05-T2 | 05 | 5 (W1) | FIX-01, FIX-03, CPU-06 | `--stage conformance`, `--stage named-controls` | ds_proto_predone, _nodone, _badsum, _wrongid each give their declared outcome (named-controls) | pending |
| 05-T3 | 05 | 5 (W1) | FIX-01, FIX-03, CPU-06 | `--stage fixture-lint`, `--stage fixture-lint-negatives`, `--selftest mutation-fixture-lint-orphan` | mini-roots FXL_ORPHAN_CHECK, FXL_UNLISTED_CHECK, FXL_BAD_CLASS, FXL_COUNT, FXL_INCLUDE; mutation-fixture-lint-orphan | pending |
| 06-T1 | 06 | 6 (W1) | FIX-01 | `--stage fixtures`, `--stage fixture-negatives` | manifest sha256 changed by one digit gives FIX_HASH_MISMATCH; deleted authored entry fails the stage | pending |
| 06-T2 | 06 | 6 (W1) | FIX-01 | `--stage preflight`, `--stage fixture-negatives` | authored-redistribution-committed and authored-duospool-version mini-roots; empty-PATH preflight exits 2 | pending |
| 06-T3 | 06 | 6 (W1) | FIX-01 | `--stage ledger-entries`, `--stage licences`, `--stage rights-tree` | ledger-entries schema keys on LED-TOOL-0001; rights-tree rejects planted files | pending |
| 07-T1 | 07 | 7 (W1) | FIX-01, CORE-04, CPU-06 | `--stage workflow`, `--stage workflow-negatives`, `--selftest mutation-workflow-pin` | workflow-negatives (cc65 step missing or unpinned); mutation-workflow-pin | pending |
| 07-T2 | 07 | 7 (W1) | FIX-01, CORE-04, CPU-06 | `--stage docs-claims`, `--stage docs-claims-negatives` | docs-claims-negatives (tool rule removed turns red) | pending |
| 07-T3 | 07 | 7 (W1) | FIX-01, CORE-04, CPU-06 | `--stage ledger-entries`, `--stage inventory`, `--timing` | ledger REQUIRED line removed turns red; timing is recorded, never fails (D-60) | pending |
| 08-T1 | 08 | 8 (W2) | FIX-01, CPU-01, CPU-06 | `--stage xasm` | five TSV rows through matrix and ca65; one changed byte is XASM_MISMATCH | pending |
| 08-T2 | 08 | 8 (W2) | FIX-01, CPU-01, CPU-06 | `--stage xasm`, `--stage ledger-entries` | every TSV row assembled and compared; ledger-entries on LED-CPU-0001/0002/0009/0010 | pending |
| 08-T3 | 08 | 8 (W2) | FIX-01, CPU-01, CPU-06 | `--stage xasm-negatives`, `--stage fixture-lint-negatives`, `--stage licence-negatives`, `--selftest mutation-xasm-compare` | xasm-negatives with shim ca65 flipping a byte; mutation-xasm-compare; mnemonic lint rule; tests/oracle licence cover | pending |
| 09-T1 | 09 | 9 (W2) | CPU-01, CPU-06, CORE-04 | `--stage build`, `--stage conformance`, `--stage core-boundary` | opcode-check --partial: a def row differing from its TSV row is OPC_FIELD | pending |
| 09-T2 | 09 | 9 (W2) | CPU-01, CPU-06, CORE-04 | `--stage opcode-oracle`, `--stage build`, `--stage conformance` | all 256 rows two-way; row missing on either side is red | pending |
| 09-T3 | 09 | 9 (W2) | CPU-01, CPU-06, CORE-04 | `--stage opcode-oracle-negatives`, `--stage conformance`, `--selftest mutation-opcode-oracle-field` | mini-roots OPC_FIELD, OPC_MISSING_ROW, OPC_EXTRA_ROW, OPC_UNDEF_COUNT; mutation-opcode-oracle-field | pending |
| 10-T1 | 10 | 10 (W2) | FIX-03, CORE-06, CPU-01 | `--stage conformance`, `--stage sanitizers` | mutate_sweep_ds_proto flips each of 8 checks once; an unflipped check fails its sweep | pending |
| 10-T2 | 10 | 10 (W2) | FIX-03, CORE-06, CPU-01 | `--stage build`, `--stage core-boundary`, `--stage conformance` | sink overlapping instance or request is memory_invalid and leaves instance byte-identical | pending |
| 10-T3 | 10 | 10 (W2) | FIX-03, CORE-06, CPU-01 | `--stage conformance` | opcode_exercise lists unexercised defined opcodes into its evidence | pending |
| 11-T1 | 11 | 11 (W2) | CPU-01, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | LDA mode literal changed in a scratch ROM is guest_failure | pending |
| 11-T2 | 11 | 11 (W2) | CPU-01, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | per-check literal edit via sweep; fixture-lint class and cite rules | pending |
| 11-T3 | 11 | 11 (W2) | CPU-01, FIX-03 | `--stage conformance`, `--stage inventory` | uninitialised RAM read differs across policies and is guest_failure; cpu_ldst sweep | pending |
| 12-T1 | 12 | 12 (W2) | CPU-01, CPU-06, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | ADC GUEST reference model: literal flip is guest_failure; fixture-lint | pending |
| 12-T2 | 12 | 12 (W2) | CPU-01, CPU-06, FIX-03 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | SED before SET order; T cleared by next instruction; decimal flag sources (Pitfall 8) | pending |
| 12-T3 | 12 | 12 (W2) | CPU-01, CPU-06, FIX-03 | `--stage conformance`, `--selftest mutation-control-anchor` | adc-carry toy RED_OK; replacement equal to anchor gives NOT_RED; mutation-control-anchor | pending |
| 13-T1 | 13 | 13 (W2) | CPU-01, CPU-06 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | branch and JSR/RTS literal flip is guest_failure | pending |
| 13-T2 | 13 | 13 (W2) | CPU-01, CPU-06 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | BRK/RTI vector literal edit; unexpected vector handler sets DSV1 error flag | pending |
| 13-T3 | 13 | 13 (W2) | CPU-01, CPU-06 | `--stage fixtures`, `--stage conformance` | cpu_flow sweep; startup.inc re-pin: stale hash is FIX_HASH_MISMATCH | pending |
| 14-T1 | 14 | 14 (W2) | CPU-01, CPU-02, CPU-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | SXY/SAX/SAY and CLA/CLX/CLY literal flip is guest_failure | pending |
| 14-T2 | 14 | 14 (W2) | CPU-01, CPU-02, CPU-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | TAM/TMA one-hot and ST0-ST2 literal flip; RMB/SMB bit mask edit | pending |
| 14-T3 | 14 | 14 (W2) | CPU-01, CPU-02, CPU-04 | `--stage conformance` | cpu_huc sweep; opcode_exercise require mode fails on an unexercised defined opcode | pending |
| 15-T1 | 15 | 15 (W2) | CPU-01, CPU-06, FIX-01 | `--stage build`, `--stage conformance` | scratch TSV with one cycle changed via --opcode-tsv makes opcode_table_manual guest_failure | pending |
| 15-T2 | 15 | 15 (W2) | CPU-01, CPU-06, FIX-01 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | cycle_adders_manual literal edit; sink lost != 0 is harness_failure | pending |
| 15-T3 | 15 | 15 (W2) | CPU-01, CPU-06, FIX-01 | `--stage ledger-entries`, `--stage inventory`, `--stage ledger-negatives` | ledger-negatives (assumed-no-assumption); LED-CPU entries schema | pending |
| 16-T1 | 16 | 16 (W3) | CPU-02, CPU-04, CORE-06 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | unmapped-bank toy value edit; mmu_mpr bank $80 read literal | pending |
| 16-T2 | 16 | 16 (W3) | CPU-02, CPU-04, CORE-06 | `--stage conformance`, `--stage record-lint`, `--selftest mutation-control-anchor` | unmapped-value toy RED_OK on mmu_mpr_256k; mutation-control-anchor; BUS0 record line lint | pending |
| 16-T3 | 16 | 16 (W3) | CPU-02, CPU-04, CORE-06 | `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | VDC/VCE penalty literal flip in cyc_probe/mmu_mpr; penalty class ASSUMED labelled | pending |
| 17-T1 | 17 | 17 (W3) | CPU-06, CPU-02 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | tma_zero latch versus keep checks must differ between model values | pending |
| 17-T2 | 17 | 17 (W3) | CPU-06, CPU-02 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | TAM/TMA multi-bit literal flip; RAW probe class label | pending |
| 17-T3 | 17 | 17 (W3) | CPU-06, CPU-02 | `--stage conformance`, `--stage sanitizers` | model_refuse_unknown_name, _value and _state each refused with code, instance byte-identical | pending |
| 18-T1 | 18 | 18 (W3) | CPU-06, CORE-04 | `--stage build`, `--stage conformance`, `--stage record-lint` | golden regenerated only with [model_tma_zero] and [run] present; record-lint rejects missing table | pending |
| 18-T2 | 18 | 18 (W3) | CPU-06, CORE-04 | `--stage record-lint-negatives`, `--stage determinism-negatives`, `--stage conformance` | record-lint mini-root missing-model-table; determinism-negatives | pending |
| 18-T3 | 18 | 18 (W3) | CPU-06, CORE-04 | `--stage ledger-entries`, `--stage inventory` | ledger-entries schema on LED-PROF-0004 and LED-CPU-0015; inventory pins | pending |
| 19-T1 | 19 | 19 (W3) | CORE-06 | `--stage build`, `--stage core-boundary`, `--stage conformance` | observer_ds_proto: every sink config, whole and chunked, is byte-identical to no-sink run | pending |
| 19-T2 | 19 | 19 (W3) | CORE-06 | `--stage conformance`, `--selftest mutation-control-anchor` | trace-hook-mutates toy RED_OK; replacement equal to anchor gives NOT_RED | pending |
| 19-T3 | 19 | 19 (W3) | CORE-06 | `--stage conformance`, `--stage rights-tree` | trace_out_rights: non-authored or synthetic recipe request refused; rights-tree | pending |
| 20-T1 | 20 | 20 (W3) | CPU-02, CPU-06, CORE-06 | `--stage build`, `--stage conformance`, `--stage ledger-entries`, `--stage inventory` | ORACLE_RULE_TEXT_DRIFT when a LED-CPU-0011 clause is edited (scratch); oracle vs runner mismatch | pending |
| 20-T2 | 20 | 20 (W3) | CPU-02, CPU-06, CORE-06 | `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | 384K and 1M bank-tag checks differ from 256K exactly where mirror rules differ; st_bypass literal | pending |
| 20-T3 | 20 | 20 (W3) | CPU-02, CPU-06, CORE-06 | `--stage ledger-entries`, `--stage inventory`, `--stage ledger-negatives` | ledger-negatives; LED-LOAD-0005 edit keeps schema and REQUIRED | pending |
| 21-T1 | 21 | 21 (W4) | CPU-03, CORE-04 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | first timer check literal edit (reload 0 TIQ) is guest_failure | pending |
| 21-T2 | 21 | 21 (W4) | CPU-03, CORE-04 | `--stage conformance`, `--stage sanitizers` | naive per-clock reference model disagreement is red; timer-reload-off toy comes in plan 22 | pending |
| 21-T3 | 21 | 21 (W4) | CPU-03, CORE-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | tim_timer literal flip via sweep; same-value control write is a no-op check | pending |
| 22-T1 | 22 | 22 (W4) | CPU-03, CPU-06, CORE-05 | `--stage build`, `--stage fixtures`, `--stage conformance` | irq_stim handler literal flip; $1403 bit 1 read inside the handler | pending |
| 22-T2 | 22 | 22 (W4) | CPU-03, CPU-06, CORE-05 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | irq_i_sample pre versus post distinguishing check must differ between the two | pending |
| 22-T3 | 22 | 22 (W4) | CPU-03, CPU-06, CORE-05 | `--stage conformance`, `--stage record-lint`, `--selftest mutation-control-anchor` | timer-reload-off RED_OK on tim_timer and timer_pure_function; irq-priority-swap RED_OK on irq_ctl_pre | pending |
| 23-T1 | 23 | 23 (W4) | CPU-04, CPU-03, CPU-06, CORE-06 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | CSH loop-rate literal flip (E2 self-consistency, circular on CPU speed) | pending |
| 23-T2 | 23 | 23 (W4) | CPU-04, CPU-03, CPU-06, CORE-06 | `--stage conformance`, `--stage record-lint`, `--stage record-lint-negatives`, `--stage determinism-negatives` | record-lint mini-root missing-stimulus-table; determinism-negatives | pending |
| 23-T3 | 23 | 23 (W4) | CPU-04, CPU-03, CPU-06, CORE-06 | `--stage ledger-entries`, `--stage inventory`, `--stage conformance` | observer_tim_timer sink matrix; ledger LED-IRQ entries schema | pending |
| 24-T1 | 24 | 24 (W4) | CPU-06, CPU-02, CPU-03, CPU-04 | `--stage build`, `--stage conformance`, `--stage profile-records` | v2 boot register draw changes record and digest; profile-records mismatch | pending |
| 24-T2 | 24 | 24 (W4) | CPU-06, CPU-02, CPU-03, CPU-04 | `--stage conformance` | independent draw literals; seed edit changes draw | pending |
| 24-T3 | 24 | 24 (W4) | CPU-06, CPU-02, CPU-03, CPU-04 | `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | policy-diff negative control (mmu_mpr_policy_diff must differ); per-check bytes identical across three policies | pending |
| 25-T1 | 25 | 25 (W5) | CPU-05, CPU-01, CORE-04 | `--stage build`, `--stage core-boundary`, `--stage fixtures`, `--stage conformance` | block cycle count 17+6x measured by timer; literal flip is guest_failure | pending |
| 25-T2 | 25 | 25 (W5) | CPU-05, CPU-01, CORE-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance`, `--stage sanitizers` | TDD/TIN/TIA/TAI pattern literals; length 0 as 65,536 bytes | pending |
| 25-T3 | 25 | 25 (W5) | CPU-05, CPU-01, CORE-04 | `--stage conformance`, `--selftest mutation-control-anchor` | block-progress-lost toy RED_OK on cpu_blk resume sub-check; mutation-control-anchor | pending |
| 26-T1 | 26 | 26 (W5) | CORE-04, CPU-05, CORE-06, CPU-06 | `--stage build`, `--stage conformance` | chunk_equiv_cpu_blk_len0 under six schedules; each needs a BLOCK_BYTE stop | pending |
| 26-T2 | 26 | 26 (W5) | CORE-04, CPU-05, CORE-06, CPU-06 | `--stage conformance`, `--stage sanitizers` | chunk_equiv_random32 and observer extensions; schedule digest mismatch reported per schedule | pending |
| 26-T3 | 26 | 26 (W5) | CORE-04, CPU-05, CORE-06, CPU-06 | `--stage ledger-entries`, `--stage inventory` | ledger-negatives; LED-CPU-0004/0007/0008 REQUIRED lines | pending |
| 27-T1 | 27 | 27 (W6) | INP-01, INP-02, INP-04 | `--stage flatten-controls`, `--stage fixtures`, `--stage conformance` | flatten-controls: script event press RUN on port 1 reaches guest via SEL=0 read | pending |
| 27-T2 | 27 | 27 (W6) | INP-01, INP-02, INP-04 | `--stage fixture-lint`, `--stage fixtures`, `--stage conformance` | pad_2btn_correct literal flip; raw reads stored in observed bytes | pending |
| 27-T3 | 27 | 27 (W6) | INP-01, INP-02, INP-04 | `--stage conformance`, `--stage sanitizers` | input_port_refusal: ports 0, 2-5 and 6 refused with code | pending |
| 28-T1 | 28 | 28 (W6) | INP-02, INP-04 | `--stage input-check`, `--stage input-check-negatives` | input-check-negatives: decreasing at goes red | pending |
| 28-T2 | 28 | 28 (W6) | INP-02, INP-04 | `--stage input-check-negatives`, `--stage flatten-controls`, `--selftest mutation-input-order` | one mini-root per INP_* code; mutation-input-order | pending |
| 28-T3 | 28 | 28 (W6) | INP-02, INP-04 | `--stage conformance`, `--stage record-lint`, `--stage record-lint-negatives`, `--stage determinism-negatives` | [input] table lint; record-lint-negatives; determinism-negatives | pending |
| 29-T1 | 29 | 29 (W6) | INP-03, INP-01, INP-04, CORE-06 | `--stage input-check`, `--stage conformance` | none and wrong scripts must fail exactly the declared check set (fail-set=LIST); identical verdict bytes none vs correct are red | pending |
| 29-T2 | 29 | 29 (W6) | INP-03, INP-01, INP-04, CORE-06 | `--stage conformance`, `--selftest mutation-control-anchor` | pad_2btn_late and pad_2btn_glitch declared fail-sets; shared-input toy RED_OK; mutation-control-anchor | pending |
| 29-T3 | 29 | 29 (W6) | INP-03, INP-01, INP-04, CORE-06 | `--stage conformance`, `--stage ledger-entries`, `--stage inventory` | IOPT/SCHD region presence; ledger LED-IO schema; inventory pins | pending |
| 30-T1 | 30 | 30 (W7) | CORE-05, CORE-04 | `--stage build`, `--stage core-boundary`, `--stage conformance` | restore into poisoned fresh instance continues identically; digest mismatch red | pending |
| 30-T2 | 30 | 30 (W7) | CORE-05, CORE-04 | `--stage build`, `--stage conformance`, `--stage sanitizers` | snapshot_refusal_rows: each bad field refused with code, target byte-identical | pending |
| 30-T3 | 30 | 30 (W7) | CORE-05, CORE-04 | `--stage conformance`, `--stage sanitizers` | per-region perturbation changes the digest; restore at every boundary | pending |
| 31-T1 | 31 | 31 (W7) | CORE-05 | `--stage conformance`, `--stage sanitizers` | hostile first 50 generated cases under sanitizers: refused is byte-identical, accepted re-serialises equal | pending |
| 31-T2 | 31 | 31 (W7) | CORE-05 | `--stage conformance`, `--stage sanitizers`, `--timing` | corpus about 500 under sanitizers; sanitizer abort is red | pending |
| 31-T3 | 31 | 31 (W7) | CORE-05 | `--stage conformance`, `--selftest mutation-restore-skip-field`, `--selftest mutation-control-anchor` | snapshot-drop-t and snapshot-drop-blkt RED_OK; mutation-restore-skip-field; mutation-control-anchor | pending |
| 32-T1 | 32 | 32 (W8) | CPU-06, FIX-01, FIX-03 | `--stage fixture-lint`, `--stage fixture-lint-negatives`, `--stage ledger-entries`, `--stage ledger-negatives` | fixture-lint cite rule: uncited literal is FIX_UNCITED; ledger-negatives | pending |
| 32-T2 | 32 | 32 (W8) | CPU-06, FIX-01, FIX-03 | `--stage ledger-entries`, `--stage inventory`, `--stage fixture-lint`, `--stage fixtures` | assumed-no-assumption negative; PENDING pins move to TEST ids only after cases exist | pending |
| 32-T3 | 32 | 32 (W8) | CPU-06, FIX-01, FIX-03 | `--stage fixture-build`, `--stage conformance` | class counts must sum to assertions; guest judge refuses (harness_failure) when class missing | pending |
| 33-T1 | 33 | 33 (W8) | FIX-01, FIX-03, CPU-06 | `--stage docs-claims`, `--stage docs-claims-negatives` | docs-claims-negatives: withheld-claim wording removed or `supported` claim added turns red | pending |
| 33-T2 | 33 | 33 (W8) | FIX-01, FIX-03, CPU-06 | `--timing`, `--stage xasm` | timing re-baseline recorded, never fails; xasm runs WLA only when present, no second-assembler claim otherwise | pending |
| 33-T3 | 33 | 33 (W8) | FIX-01, FIX-03, CPU-06 | `--stage ci-timings-controls`, `sh scripts/check` | ci-timings-controls malformed TSV exits 2; full gate green; hand-off list complete | pending |

*Status: pending, green, red, flaky. All rows start pending.*

### Requirement to evidence map (from RESEARCH)

| Requirement | Main checks | Evidence tier and oracle | Failure signal |
|-------------|-------------|--------------------------|----------------|
| CPU-01 | `opcode_table_manual`, `cycle_adders_manual`, guest `cpu_ldst`, `cpu_alu`, `cpu_flow`, `cpu_huc`, `cpu_blk`; stages `opcode-oracle`, `xasm` | E1 documentary (page-image TSV), E2 guest reference models | `guest_failure` with failing check ids; `OPC_*`; `XASM_MISMATCH` |
| CPU-02 | `mmu_mpr_256k/_384k/_1m`, `bankclass_oracle`, `tam_tma_manual`, `st_bypass` | E1 manual literals; awk bank-class oracle labelled E0/E2 (ER-7); $80-$F7 and RAM x4 stay historical report | `guest_failure`; evidence names bank and offset |
| CPU-03 | `timer_pure_function`, `tim_timer`, `irq_ctl_pre`, `irq_ctl_post`, `irq_regs_manual` | E1 C-level step and period; in-guest timer checks E2 (ER-3) | `guest_failure`; DSV1 error flag on unexpected vector |
| CPU-04 | `spd_speed`, `speed_manual` | E1 reset speed and CSL/CSH cycles; switch behaviour and penalty ASSUMED | `guest_failure` |
| CPU-05 | `cpu_blk`, `block_cycles_manual`, `chunk_equiv_cpu_blk`, `chunk_equiv_cpu_blk_len0`, `chunk_equiv_random32` | E1 17+6x and length 0 = 65,536; chunk identity E0 | digest mismatch per schedule |
| CPU-06 | `ledger-check`, `inventory-check`, `fixture-lint` cite rules, `model_param_refusal`, `[model_*]` golden | Ledger discipline, no oracle claim | `LED`, `INV_*`, `FIX_UNCITED`; refusal outcome |
| INP-01..04 | `pad_2btn_correct/_none/_wrong/_late/_glitch`, `input_exact_edges`, `input_port_refusal`, `input_schedule_chunking`, stage `input-check` | E2 self-consistency; wire order stays historical report (ER-5) | `INP_*` set differs; fail-set differs; refusal code |
| CORE-04 | `run_until_abs`, `run_overshoot_bound`, `chunk_equiv_*` | E0 internal consistency | digest or machine-time mismatch |
| CORE-05 | `snapshot_restore_sampled`, `_boundaries`, `snapshot_tiling`, `snapshot_hostile` | E0, host `shasum -a 256` of digest stream | memcmp or lockstep difference; sanitizer abort |
| CORE-06 | `observer_*` matrix, `trace_out_rights`, liveness toy | E0 structural | verdict, time, stop or digest differs between sink configs |
| FIX-01 | `fixture-build`, `xasm`, preflight, authored manifest entries | Two encoders: ca65 and the manual TSV; WLA optional | `PREFLIGHT_MISSING`, `FIX_HASH_MISMATCH`, `XASM_MISMATCH` |
| FIX-03 | `ds_proto` and four controls, `mutate_sweep_*`, `fixture-lint` class rules | Expected values never from DuoSpool output | unflipped check fails its sweep; `FIX_*` |

### ROADMAP Phase 3 success criteria

| SC | Where checked | Stated scope |
|----|---------------|--------------|
| 1 Authored fixtures with cited literals, wrong literal fails | Plans 05, 08, 10-15, 20-25, `xasm` | State what ran: ca65 encodings equal the manual transcription; no second-assembler claim when WLA is absent |
| 2 Pad fixture, three scripts, per-port script in mclk | Plans 27-29 | Three scripts from one ROM, differing verdicts, pad log |
| 3 Chunked equals uninterrupted; snapshot restore | Plans 02, 26, 30, 31 | Fixed mclk chunk sizes incl. prime sizes and timer-event ticks; no frame exists yet |
| 4 Trace and device-write log change nothing | Plans 10, 19, 23, 26, 29 | Observer matrix and liveness toy |
| 5 Every cycle count and penalty has a ledger entry or assumption label | Plans 15, 18, 20, 23, 26, 29, 32 | VDC/VCE penalty LED-CPU-0017, speed switch LED-CPU-0006, both ASSUMED |

---

## Wave 0 Requirements

Infrastructure that must exist before the code that needs it. Owning plan in brackets; plan text is authoritative.

- [ ] Assembler preflight: `ca65` and `ld65` on PATH (`brew install cc65`), preflight entries that fail closed with `PREFLIGHT_MISSING`, CI apt step, tool record `fixtures/lib/TOOLS.md` [plans 01, 06, 07]
- [ ] Runner headroom: record and hash kinds moved out of `tests/conformance/runner.c`, new kind files registered in `CMakeLists.txt` (Pitfall 1) [plans 01, 04]
- [ ] Optional case keys (`until_mclk`, `checks`, `policies`, and the rest) with inventory-rules controls; the ten-key schema stays enforced (Pitfall 2) [plan 04]
- [ ] Fixture build: `scripts/fixture-build` and its stage before `build`, `fixtures/lib/hucard.cfg`, authored manifest entries [plans 01, 06]
- [ ] DSV1 judge `kind_guest` plus `startup.inc`, `verdict.inc`, `check.inc` with the DCK1 literal table, before any CPU fixture [plans 01, 05]
- [ ] `ds_proto` and its four control variants (`_predone`, `_nodone`, `_badsum`, `_wrongid`) [plans 01, 05]
- [ ] `fixtures/lib/cites.txt` seeded from the page-image table, rows flagged `img` or `ocr`, with `scripts/fixture-lint` and its mini-roots [plan 05]
- [ ] Opcode oracle: `tests/oracle/huc6280-opcodes.tsv` produced from page images by a pass that does not read `cpu_ops.def`, with `scripts/xasm-check` and `scripts/opcode-check` [plans 08, 09]
- [ ] `tests/gate` mini-roots for `fixture-lint`, `xasm`, `input-check`, authored `fixture-check`, and the empty-PATH preflight control [plans 05, 06, 08, 28]
- [ ] Anchored one-line statements for the toys (`adc-carry`, `unmapped-value`, `trace-hook-mutates`, `timer-reload-off`, `irq-priority-swap`, `block-progress-lost`, `shared-input`, `snapshot-drop-t`, `snapshot-drop-blkt`) (Pitfall 4) [plans 12, 16, 19, 22, 25, 29, 31]
- [ ] Run API migration: `ds_run` absolute-deadline model and `run_refusals.def` replacing the placeholder advance [plans 01, 02]

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| First green GitHub Actions run with the Phase 3 gate, critical-path timing row | CI-01 follow-through | A push to the remote is an owner decision (D-64); otherwise reported unverified-pending-push | After the owner authorises: push a `gsd/phase-3-*` branch, open a PR, run `sh scripts/ci-timings <run-id>`, append the row to `docs/ci-timings.md` |
| Page-image verification of every manual or SW-page literal | CPU-06, FIX-03 | Reading a page image needs a person or an executor with the verifier; OCR alone stays flagged `ocr` | An executor reads the page image with the verifier; each ledger entry records the verifier, the date and the fact seen; `cites.txt` flag becomes `img` |
| Hardware captures | none | Optional, not needed: no claim in this phase depends on physical hardware | Not scheduled. Any hardware observation later is recorded with provenance and is a separate owner decision |

Everything else has automated verification. Authority gates are recorded hand-offs, not UAT.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or a Wave 0 dependency
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Every negative control in the map seen red once, and every new checker has a `--selftest` mutation
- [ ] Feedback latency under 120 s for the full gate; time is recorded, never a failure (D-60)
- [ ] No row claims more than E0, E1 documentary or E2 self-consistency; nothing marked `supported`
- [ ] `nyquist_compliant: true` set in frontmatter only after the full suite, `--selftest`, dash run and audit-history are green

**Approval:** pending
