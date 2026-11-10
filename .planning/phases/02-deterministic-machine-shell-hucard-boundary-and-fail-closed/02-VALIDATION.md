---
phase: "02"
slug: "deterministic-machine-shell-hucard-boundary-and-fail-closed"
status: draft
nyquist_compliant: true
wave_0_complete: true
created: "2026-11-09"
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution. The full per-success-criterion and per-requirement map (check, placement, negative control, failure signal) is in `02-RESEARCH.md` § "Validation Architecture"; this file is the execution-time contract derived from it.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None added. C runner prints `RESULT <id> <outcome> assertions=<n> evidence=<relpath>`; the sh+awk gate (`scripts/check`) is the only judge (D-57, D-60). CTest optional, not in the verdict |
| **Config file** | `CMakePresets.json`, `tests/conformance/cases/<id>.toml`, `tests/conformance/REQUIRED`, `tests/conformance/outcomes.def` (all Wave 0) |
| **Quick run command** | `sh scripts/check` |
| **Full suite command** | `sh scripts/check && sh scripts/check --selftest && dash scripts/check` plus the sanitizer preset (inside the fast gate, D-41); `sh scripts/audit-history` and one Docker `ubuntu:24.04` run (gcc 13, clang 18, mawk and gawk) at phase gate, labelled one-off, not a CI claim |
| **Estimated runtime** | baseline 9.7 s for the 18 Phase 1 stages; budget warm <= 60 s, cold CI <= 3 min |

---

## Sampling Rate

- **After every task commit (D-81):** the task's own stage-granular command, `sh scripts/check --stage NAME` (or `sh scripts/check --selftest NAME` for one mutation); each is a few seconds
- **After every plan (wave-end, labelled):** `sh scripts/check`, `sh scripts/check --selftest`, `dash scripts/check`, plus the sanitizer stage and the gcc and clang presets where a second compiler exists
- **Phase-end (plan 21):** full suite above plus `sh scripts/audit-history` and the one-off Docker `ubuntu:24.04` run (`sh scripts/linux-repro`), labelled local, not a CI claim
- **Before `/gsd-verify-work`:** phase-end suite green
- **Max feedback latency:** 60 s for the whole gate; per-task stage commands are much shorter

---

## Per-Task Verification Map

Per-task commands run at stage granularity (D-81); full-gate, `--selftest`, equivalence-harness and container runs are labelled wave-end or phase-end. Every `<automated>` command is `sh scripts/check --stage NAME`, `sh scripts/check --selftest NAME` or a named checker invoked by it, each paired with a `<fails_when>` and a named negative control. Waves are sequential because every plan edits `scripts/check`. Status starts pending.

| Task | Plan | Wave | Requirement | Check (automated) | Negative control shown red | Status |
|------|------|------|-------------|-------------------|----------------------------|--------|
| 01-T1 | 01 | 1 | TOOL-02 | `--stage licence-negatives`, bogus `--stage` exits 2; plan-end: `tests/check/equiv.sh BASE` | label-coverage guard in equiv.sh | pending |
| 01-T2 | 01 | 1 | TOOL-02 | six control stages via `--stage`; plan-end: equiv.sh, `dash scripts/check` | seven scenario labels | pending |
| 01-T3 | 01 | 1 | TOOL-02 | `--selftest mutation-run-controls` | mutation-run-controls | pending |
| 02-T1 | 02 | 2 | RUN-02, FIX-04 | `--stage ingest-controls` | outcome-skip, required-no-line, unknown-token | pending |
| 02-T2 | 02 | 2 | RUN-02, FIX-04 | `--stage ingest-controls`, `--selftest mutation-ingest-skip-red` | 18 fabricated ingest controls, mutation-ingest-skip-red | pending |
| 02-T3 | 02 | 2 | RUN-02 | `--stage flatten-controls`, `--selftest mutation-flatten-sentinel` | bad-syntax, partial-output probe, mutation-flatten-sentinel | pending |
| 03-T1 | 03 | 3 | RUN-02 | `--stage cancel-controls`, `--selftest mutation-cancel-trap` | SIGTERM to spin stub, mutation-cancel-trap | pending |
| 03-T2 | 03 | 3 | TOOL-02 | `--stage preflight`, `--timing` | impossible tool name exits 2 | pending |
| 03-T3 | 03 | 3 | RUN-02 | `--stage outcome-controls`, `--selftest mutation-outcome-sync` (the real-repository `outcome-sync` stage is registered after `build` in plan 06, D-85) | extra-row, reordered, mutation-outcome-sync | pending |
| 04-T1 | 04 | 4 | FIX-05 | `--stage rights-tree`, `--stage rights-negatives` | planted-pce | pending |
| 04-T2 | 04 | 4 | FIX-05 | `sh tests/rights/synthetic.sh`, `--selftest mutation-rights-rule` | fourteen run-time controls, mutation-rights-rule | pending |
| 04-T3 | 04 | 4 | FIX-05 | `--stage audit-synthetic`, `--stage licences`, `--stage licence-negatives` | range-dirty, src-no-spdx | pending |
| 05-T1 | 05 | 5 | PROF-02, CORE-01 | `--stage build` plus `ds_runner --tracer`; `cmake --list-presets=build` lists `gate`; the content copy in `src/core/boot.c` is one `__builtin_memcpy` line (D-86) | warnings-as-errors build | pending |
| 05-T2 | 05 | 5 | PROF-02 | `--stage build`, `--stage conformance` (runner dispatch is two-pass: deferred kinds run after the case loop, D-86) | wrong literal seen as guest_failure | pending |
| 05-T3 | 05 | 5 | PROF-02 | `--stage conformance` (resolve_*) | resolve case edit turns guest_failure | pending |
| 06-T1 | 06 | 6 | PROF-03 | `--stage conformance` (record_skeleton_256k) | line-by-line record check | pending |
| 06-T2 | 06 | 6 | PROF-02, PROF-03 | `--stage conformance` (profile_r1_equal, names, field_flip) | r1 TSV edit turns profile_r1_equal red | pending |
| 06-T3 | 06 | 6 | RUN-02 | `cmake --preset gate-clang && cmake --build --preset gate-clang` (configure and build presets for `gate-gcc` and `gate-clang`, `cmake --list-presets=build` lists three, D-86), `--stage build`, then `--stage outcome-sync` (registered after build; runs `build/gate/ds_runner --outcomes` itself, D-85), `--stage conformance` | compiled-list-differs | pending |
| 07-T1 | 07 | 7 | CONT-04 | `--stage conformance` (content_id_vs_shasum, hash sweep) | HASH_SWEEP_MISMATCH, bit-length mutant | pending |
| 07-T2 | 07 | 7 | CONT-04 | `--stage conformance` (NIST, record_content_fields) | NIST literal digit edit | pending |
| 08-T1 | 08 | 8 | CONT-01 | `--stage conformance` (accept_256k_banksig) | oracle arithmetic edit | pending |
| 08-T2 | 08 | 8 | CONT-01 | `--stage conformance` (384K, 1M; the 384K mirror expression is one source line in `src/core/mapper.c`, D-86) | banksig cases vs oracle | pending |
| 08-T3 | 08 | 8 | CONT-01 | `--stage conformance`, `--stage ledger-entries` (REQUIRED lines `LED-LOAD-0003 02-CONTEXT D-44`, `LED-LOAD-0005 02-CONTEXT D-44`, all schema keys) | ORACLE_RULE_TEXT_DRIFT | pending |
| 09-T1 | 09 | 9 | CONT-01 | `--stage conformance` (controls mode; toy generated from live `src/core/mapper.c` by `scripts/control-mutate`, D-85), `--selftest mutation-control-anchor` | tests/controls/wrong-mirror (CONTROL RED_OK), anchor-not-matching mutation (exit 2) | pending |
| 09-T2 | 09 | 9 | CONT-01 | `--stage conformance`, `--stage ledger-entries` (REQUIRED line `LED-LOAD-0004 02-CONTEXT D-44`, all schema keys) | ref_size_8192, ref_size_512k rows | pending |
| 10-T1 | 10 | 10 | CONT-03 | `--stage conformance`, `--stage sanitizers`; sanitize preset flags lack `-fwrapv`, gate presets have it (D-83) | heap, signed-overflow and signed-shift canaries (SANITIZER_DEAD) | pending |
| 10-T2 | 10 | 10 | CONT-03 | `--stage conformance` (alloc_*) | alloc_shim_selfcheck | pending |
| 10-T3 | 10 | 10 | CONT-03 | `--stage conformance` (produces `build/gate/results.txt`, D-86), `--stage excluded-mode-controls`, `--stage sanitizers`, `--selftest mutation-excluded-gate-pass` | five exclusion controls, mutation-excluded-gate-pass | pending |
| 11-T1 | 11 | 11 | CONT-01, CONT-02, PROF-02 | `--stage conformance` (group A, 9 rows; 34 case files) | per-row expectation edit | pending |
| 11-T2 | 11 | 11 | CONT-01, CONT-02 | `--stage conformance` (group B, 9 rows; 43 case files) | per-row expectation edit | pending |
| 11-T3 | 11 | 11 | CONT-01, CONT-02, PROF-02 | `--stage conformance`, `--stage ledger-entries` (group C, 7 rows plus two pin moves; 50 case files) | per-row expectation edit | pending |
| 11-T4 | 11 | 11 | CONT-02 | `--stage conformance` (group D, 8 rows; 58 case files) | per-row expectation edit | pending |
| 11-T5 | 11 | 11 | CONT-02, CONT-03 | `--stage conformance` (6 size and cap rows incl. ref_size_2m_too_large expecting load.too_large, D-82; ref_precedence_table incl. cap-before-size rows; ref_reason_coverage is a deferred kind evaluated after the case loop and passes with its row moved first, D-86; 67 case files), `--selftest mutation-drop-precedence` | mutation-drop-precedence, nonmutation_matrix | pending |
| 11-T6 | 11 | 11 | CONT-02, CONT-03, FIX-05 | `--stage conformance`, `--stage sanitizers`, `--selftest mutation-control-anchor` (73 case files; heuristic-reverse generated from live `src/core/boot.c`, D-85) | tests/controls/heuristic-reverse, redaction_seeded_leak_flagged | pending |
| 12-T1 | 12 | 12 | CORE-03 | `--stage conformance`, `--stage sanitizers` | bare-XOR derivation mutant | pending |
| 12-T2 | 12 | 12 | CORE-03 | `--stage conformance`, `--stage ledger-entries` (REQUIRED line `LED-PROF-0002 02-CONTEXT D-50`, all schema keys) | advance_rules literals | pending |
| 13-T1 | 13 | 13 | CORE-01, PROF-03 | `--stage conformance`, `--stage sanitizers` | digest framing check, profile_hash_fields | pending |
| 13-T2 | 13 | 13 | CORE-01 | `--stage conformance`, `--stage sanitizers` (toy generated from live `src/core/serial.c`, D-85) | tests/controls/drop-region | pending |
| 14-T1 | 14 | 14 | PROF-03, CORE-03, CONT-04 | `--stage conformance` (record_golden_384k_seeded) | golden digit edit | pending |
| 14-T2 | 14 | 14 | PROF-03 | `--stage conformance` (second golden, refusal record, quote) | record_refuses_quote | pending |
| 14-T3 | 14 | 14 | PROF-03 | `--stage conformance` first (writes `build/gate/records/ID.record.toml` for every record-producing case, kinds accept, record_lines, record_golden and record_refusal, via `--records`; file count equals the number of those rows in the TSV, D-86), then `--stage record-lint-negatives`, `--stage record-lint`, `--selftest mutation-record-lint-pointer` | path/pointer/timestamp controls | pending |
| 15-T1 | 15 | 15 | CORE-02 | `--stage core-boundary-negatives` | time-include | pending |
| 15-T2 | 15 | 15 | CORE-02 | `--stage core-boundary-negatives`, two `--selftest` mutations | ten negative controls, allowlist and writable mutations | pending |
| 15-T3 | 15 | 15 | CORE-02 | `--stage core-boundary` | rule mutations | pending |
| 16-T1 | 16 | 16 | CORE-01 | `--stage conformance` (interleave_ab; toy generated from live `src/core/advance.c`, D-85) | tests/controls/shared-state | pending |
| 16-T2 | 16 | 16 | CORE-01, CONT-03 | `--stage conformance`, `--stage sanitizers` | aliasing self-check | pending |
| 16-T3 | 16 | 16 | CORE-01 | `--stage build`, `--stage conformance` (produce the runner and TSVs, D-86), then `--stage determinism` (compares all records of the two fresh processes, one per record-producing case), `--stage determinism-negatives`, `--selftest mutation-determinism-compare` | differs control, mutation-determinism-compare | pending |
| 17-T1 | 17 | 17 | FIX-02 | `--stage build`, `--stage fixture-negatives`, `conformance-run fixtures --only plain-256k` (the mode flattens and builds itself, D-86) | fix-missing-field, fix-unlisted | pending |
| 17-T2 | 17 | 17 | FIX-02 | `--stage build`, then loop of `conformance-run fixtures --only len:N` over the 21 sweep lengths plus plain-384k and plain-1m (23 new manifest files, 24 total) | hash edit in a scratch manifest | pending |
| 17-T3 | 17 | 17 | FIX-02 | `--stage build`, `sh scripts/conformance-run fixtures` (no --only; N_case case-derived manifest files, count equals `ds_runner --fixtures` lines) | deleted manifest file gives FIX_UNLISTED | pending |
| 17-T4 | 17 | 17 | FIX-02 | `--stage build`, `--stage conformance`, `--stage fixtures` (runs the full fixtures mode itself, then fixture-check without `--partial`, D-85), `--stage fixture-negatives`, two `--selftest` mutations | hash-mismatch, stale-entry, two mutations | pending |
| 18-T1 | 18 | 18 | FIX-04 | `--stage inventory`, `--stage inventory-negatives` | removed-required | pending |
| 18-T2 | 18 | 18 | FIX-04 | `--stage inventory`, `--selftest mutation-inventory-required-missing` | pin-backlink and five more | pending |
| 18-T3 | 18 | 18 | FIX-04, RUN-02, FIX-05 | `--stage build`, `--stage conformance`, then `--stage named-controls` (registered after both, D-85), `--selftest mutation-wrong-literal-compare` | seven NAMED_CONTROL lines, field-wise EXPECT probe, comparison-deleted mutation flips wrong-literal to broken | pending |
| 19-T1 | 19 | 19 | CI-01 | `--stage workflow`, `--stage workflow-negatives`, actionlint (one-off) | pull-request-target | pending |
| 19-T2 | 19 | 19 | CI-01 | `--stage workflow`, actionlint (one-off) | static rules still accept the extended workflow | pending |
| 20-T1 | 20 | 20 | CI-01 | `--stage workflow-negatives` | untrusted-expr | pending |
| 20-T2 | 20 | 20 | CI-01 | `--stage workflow-negatives`, `--selftest mutation-workflow-pin` | ten bad-workflow variants, mutation-workflow-pin | pending |
| 20-T3 | 20 | 20 | CI-01 | `--stage ci-timings-controls` | malformed TSV exit 2 | pending |
| 21-T1 | 21 | 21 | TOOL-02 | `--stage docs-claims`, `--stage docs-claims-negatives`, `--selftest mutation-docs-stale` | five docs controls, mutation-docs-stale | pending |
| 21-T2 | 21 | 21 | TOOL-02, CI-01 | phase-end: full gate, `--selftest`, dash, `sh scripts/audit-history`, `sh scripts/linux-repro` | container run labelled one-off (first real gcc, mawk, gawk evidence on this Mac) | pending |
| 21-T3 | 21 | 21 | TOOL-02 | `head -12` of this file anchored `^nyquist_compliant: true$` (not the Sign-Off sentence, D-86) and grep of docs/ci-timings.md | not applicable | pending |
| 22-T1 | 22 | 22 | CI-01 | owner decision (blocking-human checkpoint, one-way) | not applicable | pending |
| 22-T2 | 22 | 22 | CI-01 | exact `CI-CHECKPOINT:` line in docs/ci-timings.md | no remote write unless authorised | pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `scripts/check` `run_controls` refactor plus golden transcripts (behaviour-preserving, proved by `--selftest` and transcript equality) — first plan
- [x] `tests/conformance/outcomes.def` plus shared extractor, the `outcome-sync` checker with `outcome-controls` (plan 03) and the `outcome-sync` stage after `build` (plan 06, D-85)
- [x] `scripts/conformance-flatten` with its own negative controls
- [x] `scripts/conformance-ingest` with fabricated-results controls
- [x] `CMakeLists.txt`, `CMakePresets.json` (gate, gate-gcc, gate-clang, sanitizer), runner skeleton (`--list`, `--emit`, RESULT lines, `RUNNER_DONE`)
- [x] `src/core/profile_fields.def`, `ds_profile`, initial `ds_boot` returning typed refusals
- [x] Preflight stage (cmake, ninja, cc, shasum or sha256sum, perl, iconv, nm: missing is `harness_failure`)
- [x] `tests/lib/gitfix.sh` (from `tests/audit/synthetic.sh`)
- [x] `tests/controls/{shared-state,wrong-mirror,heuristic-reverse,drop-region}/{RULE,EXPECT}` rule files and `scripts/control-mutate` (toys are generated into `build/` from the live source at control time; no committed full-copy snapshots, D-85)
- [x] Independent awk oracle for mirror signatures and the `--emit` hash compare
- [x] Ledger entries LED-LOAD-0003/0004/0005 and a PROF power-on entry; pins move from `PENDING:2` to `TEST:<id>` only after the cases exist
- [x] `.gitattributes`; SPDX headers on every new file (extend `licence-check` to `src/` and `tests/`, D-71)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| First green GitHub Actions run, critical-path timing row | CI-01 | Needs a push to the public remote, which is an owner decision (D-64); otherwise reported unverified-pending-push | After owner authorises: push a `gsd/phase-2-*` branch, open a PR, `scripts/ci-timings <run-id>`, append the row to `docs/ci-timings.md` |

Everything else has automated verification. Authority gates (push, branch ruleset) are recorded hand-offs, not UAT.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies (the one exception is the owner authority gate 22-T1, listed under Manual-Only)
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 60s (a warm `sh scripts/check` took 45 s on a quiet machine at plan 02-21; 69 to 112 s when the shared machine was loaded, see docs/ci-timings.md; `--stage NAME` gives sub-10-second feedback)
- [x] `nyquist_compliant: true` set in frontmatter (plan 02-21, after the full suite, `--selftest`, `dash`, and the container run were green; the full-history audit keeps its two known, owner-decision hits)

**Approval:** pending
