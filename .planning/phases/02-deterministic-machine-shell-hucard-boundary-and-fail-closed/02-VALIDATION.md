---
phase: "02"
slug: "deterministic-machine-shell-hucard-boundary-and-fail-closed"
status: draft
nyquist_compliant: false
wave_0_complete: false
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
| 03-T3 | 03 | 3 | RUN-02 | `--stage outcome-sync`, `--selftest mutation-outcome-sync` | extra-row, reordered, mutation-outcome-sync | pending |
| 04-T1 | 04 | 4 | FIX-05 | `--stage rights-tree`, `--stage rights-negatives` | planted-pce | pending |
| 04-T2 | 04 | 4 | FIX-05 | `sh tests/rights/synthetic.sh`, `--selftest mutation-rights-rule` | fourteen run-time controls, mutation-rights-rule | pending |
| 04-T3 | 04 | 4 | FIX-05 | `--stage audit-synthetic`, `--stage licences`, `--stage licence-negatives` | range-dirty, src-no-spdx | pending |
| 05-T1 | 05 | 5 | PROF-02, CORE-01 | `--stage build` plus `ds_runner --tracer` | warnings-as-errors build | pending |
| 05-T2 | 05 | 5 | PROF-02 | `--stage build`, `--stage conformance` | wrong literal seen as guest_failure | pending |
| 05-T3 | 05 | 5 | PROF-02 | `--stage conformance` (resolve_*) | resolve case edit turns guest_failure | pending |
| 06-T1 | 06 | 6 | PROF-03 | `--stage conformance` (record_skeleton_256k) | line-by-line record check | pending |
| 06-T2 | 06 | 6 | PROF-02, PROF-03 | `--stage conformance` (profile_r1_equal, names, field_flip) | r1 TSV edit turns profile_r1_equal red | pending |
| 06-T3 | 06 | 6 | RUN-02 | `--stage outcome-sync`, `--stage conformance` | compiled-list-differs | pending |
| 07-T1 | 07 | 7 | CONT-04 | `--stage conformance` (content_id_vs_shasum, hash sweep) | HASH_SWEEP_MISMATCH, bit-length mutant | pending |
| 07-T2 | 07 | 7 | CONT-04 | `--stage conformance` (NIST, record_content_fields) | NIST literal digit edit | pending |
| 08-T1 | 08 | 8 | CONT-01 | `--stage conformance` (accept_256k_banksig) | oracle arithmetic edit | pending |
| 08-T2 | 08 | 8 | CONT-01 | `--stage conformance` (384K, 1M) | banksig cases vs oracle | pending |
| 08-T3 | 08 | 8 | CONT-01 | `--stage conformance`, `--stage ledger-entries` | ORACLE_RULE_TEXT_DRIFT | pending |
| 09-T1 | 09 | 9 | CONT-01 | `--stage conformance` (controls mode) | tests/controls/wrong-mirror (CONTROL RED_OK) | pending |
| 09-T2 | 09 | 9 | CONT-01 | `--stage conformance`, `--stage ledger-entries` | ref_size_8192, ref_size_512k rows | pending |
| 10-T1 | 10 | 10 | CONT-03 | `--stage conformance`, `--stage sanitizers` | heap and shift canaries (SANITIZER_DEAD) | pending |
| 10-T2 | 10 | 10 | CONT-03 | `--stage conformance` (alloc_*) | alloc_shim_selfcheck | pending |
| 10-T3 | 10 | 10 | CONT-03 | `--stage excluded-mode-controls`, `--stage sanitizers`, `--selftest mutation-excluded-gate-pass` | five exclusion controls, mutation-excluded-gate-pass | pending |
| 11-T1 | 11 | 11 | CONT-01, CONT-02, PROF-02 | `--stage conformance`, `--stage ledger-entries` | per-row expectation edit | pending |
| 11-T2 | 11 | 11 | CONT-02, CONT-03 | `--stage conformance`, `--selftest mutation-drop-precedence` | mutation-drop-precedence, nonmutation_matrix | pending |
| 11-T3 | 11 | 11 | CONT-02, CONT-03, FIX-05 | `--stage conformance`, `--stage sanitizers` | tests/controls/heuristic-reverse, redaction_seeded_leak_flagged | pending |
| 12-T1 | 12 | 12 | CORE-03 | `--stage conformance`, `--stage sanitizers` | bare-XOR derivation mutant | pending |
| 12-T2 | 12 | 12 | CORE-03 | `--stage conformance`, `--stage ledger-entries` | advance_rules literals | pending |
| 13-T1 | 13 | 13 | CORE-01, PROF-03 | `--stage conformance`, `--stage sanitizers` | digest framing check, profile_hash_fields | pending |
| 13-T2 | 13 | 13 | CORE-01 | `--stage conformance`, `--stage sanitizers` | tests/controls/drop-region | pending |
| 14-T1 | 14 | 14 | PROF-03, CORE-03, CONT-04 | `--stage conformance` (record_golden_384k_seeded) | golden digit edit | pending |
| 14-T2 | 14 | 14 | PROF-03 | `--stage conformance` (second golden, refusal record, quote) | record_refuses_quote | pending |
| 14-T3 | 14 | 14 | PROF-03 | `--stage record-lint`, `--stage record-lint-negatives`, `--selftest mutation-record-lint-pointer` | path/pointer/timestamp controls | pending |
| 15-T1 | 15 | 15 | CORE-02 | `--stage core-boundary-negatives` | time-include | pending |
| 15-T2 | 15 | 15 | CORE-02 | `--stage core-boundary-negatives`, two `--selftest` mutations | ten negative controls, allowlist and writable mutations | pending |
| 15-T3 | 15 | 15 | CORE-02 | `--stage core-boundary` | rule mutations | pending |
| 16-T1 | 16 | 16 | CORE-01 | `--stage conformance` (interleave_ab) | tests/controls/shared-state | pending |
| 16-T2 | 16 | 16 | CORE-01, CONT-03 | `--stage conformance`, `--stage sanitizers` | aliasing self-check | pending |
| 16-T3 | 16 | 16 | CORE-01 | `--stage determinism`, `--stage determinism-negatives`, `--selftest mutation-determinism-compare` | differs control, mutation-determinism-compare | pending |
| 17-T1 | 17 | 17 | FIX-02 | `--stage fixture-negatives`, `conformance-run fixtures --only plain-256k` | fix-missing-field, fix-unlisted | pending |
| 17-T2 | 17 | 17 | FIX-02 | `--stage fixtures`, `--stage fixture-negatives`, two `--selftest` mutations | hash-mismatch, stale-entry, two mutations | pending |
| 18-T1 | 18 | 18 | FIX-04 | `--stage inventory`, `--stage inventory-negatives` | removed-required | pending |
| 18-T2 | 18 | 18 | FIX-04 | `--stage inventory`, `--selftest mutation-inventory-required-missing` | pin-backlink and five more | pending |
| 18-T3 | 18 | 18 | FIX-04, RUN-02, FIX-05 | `--stage named-controls` | seven NAMED_CONTROL lines, field-wise EXPECT probe | pending |
| 19-T1 | 19 | 19 | CI-01 | `--stage workflow`, `--stage workflow-negatives`, actionlint (one-off) | pull-request-target | pending |
| 19-T2 | 19 | 19 | CI-01 | `--stage workflow`, actionlint (one-off) | static rules still accept the extended workflow | pending |
| 20-T1 | 20 | 20 | CI-01 | `--stage workflow-negatives` | untrusted-expr | pending |
| 20-T2 | 20 | 20 | CI-01 | `--stage workflow-negatives`, `--selftest mutation-workflow-pin` | ten bad-workflow variants, mutation-workflow-pin | pending |
| 20-T3 | 20 | 20 | CI-01 | `--stage ci-timings-controls` | malformed TSV exit 2 | pending |
| 21-T1 | 21 | 21 | TOOL-02 | `--stage docs-claims`, `--stage docs-claims-negatives`, `--selftest mutation-docs-stale` | five docs controls, mutation-docs-stale | pending |
| 21-T2 | 21 | 21 | TOOL-02, CI-01 | phase-end: full gate, `--selftest`, dash, `sh scripts/audit-history`, `sh scripts/linux-repro` | container run labelled one-off (first real gcc, mawk, gawk evidence on this Mac) | pending |
| 21-T3 | 21 | 21 | TOOL-02 | grep of docs/ci-timings.md and this file | not applicable | pending |
| 22-T1 | 22 | 22 | CI-01 | owner decision (blocking-human checkpoint, one-way) | not applicable | pending |
| 22-T2 | 22 | 22 | CI-01 | recorded outcome in docs/ci-timings.md | no remote write unless authorised | pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `scripts/check` `run_controls` refactor plus golden transcripts (behaviour-preserving, proved by `--selftest` and transcript equality) — first plan
- [ ] `tests/conformance/outcomes.def` plus shared extractor and `outcome-sync`
- [ ] `scripts/conformance-flatten` with its own negative controls
- [ ] `scripts/conformance-ingest` with fabricated-results controls
- [ ] `CMakeLists.txt`, `CMakePresets.json` (gate, gate-gcc, gate-clang, sanitizer), runner skeleton (`--list`, `--emit`, RESULT lines, `RUNNER_DONE`)
- [ ] `src/core/profile_fields.def`, `ds_profile`, initial `ds_boot` returning typed refusals
- [ ] Preflight stage (cmake, ninja, cc, shasum or sha256sum, perl, iconv, nm: missing is `harness_failure`)
- [ ] `tests/lib/gitfix.sh` (from `tests/audit/synthetic.sh`)
- [ ] `tests/controls/{shared-state,wrong-mirror,heuristic-reverse,drop-region}` toy libraries
- [ ] Independent awk oracle for mirror signatures and the `--emit` hash compare
- [ ] Ledger entries LED-LOAD-0003/0004/0005 and a PROF power-on entry; pins move from `PENDING:2` to `TEST:<id>` only after the cases exist
- [ ] `.gitattributes`; SPDX headers on every new file (extend `licence-check` to `src/` and `tests/`, D-71)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| First green GitHub Actions run, critical-path timing row | CI-01 | Needs a push to the public remote, which is an owner decision (D-64); otherwise reported unverified-pending-push | After owner authorises: push a `gsd/phase-2-*` branch, open a PR, `scripts/ci-timings <run-id>`, append the row to `docs/ci-timings.md` |

Everything else has automated verification. Authority gates (push, branch ruleset) are recorded hand-offs, not UAT.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
