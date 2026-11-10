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

- **After every task commit:** `sh scripts/check`
- **After every plan wave:** `sh scripts/check --selftest`, `dash scripts/check`, sanitizer preset, gcc and clang presets where a second compiler exists
- **Before `/gsd-verify-work`:** full suite above green, plus `sh scripts/audit-history`
- **Max feedback latency:** 60 s

---

## Per-Task Verification Map

Filled by the planner. Every `<automated>` command is `sh scripts/check` (all or `--stage NAME`, `--selftest`) or a named checker invoked by it, each paired with a `<fails_when>` and a named negative control. Status starts pending.

| Task | Plan | Wave | Requirement | Check (automated) | Negative control shown red | Status |
|------|------|------|-------------|-------------------|----------------------------|--------|
| 01-T1 | 01 | 1 | TOOL-02 | `sh tests/check/equiv.sh BASE` + `--stage licence-negatives` | label-coverage guard in equiv.sh | pending |
| 01-T2 | 01 | 1 | TOOL-02 | equiv.sh + `dash scripts/check` | seven scenario labels | pending |
| 01-T3 | 01 | 1 | TOOL-02 | `sh scripts/check --selftest` | mutation-run-controls | pending |
| 02-T1 | 02 | 2 | RUN-02, FIX-04 | `--stage ingest-controls` | outcome-skip, required-no-line, unknown-token | pending |
| 02-T2 | 02 | 2 | RUN-02 | `--stage flatten-controls` | bad-syntax, partial-output probe | pending |
| 02-T3 | 02 | 2 | RUN-02, TOOL-02 | `--timing`, `--selftest`, preflight | 17+ fabricated ingest controls, mutation-ingest-skip-red, mutation-outcome-sync | pending |
| 03-T1 | 03 | 3 | PROF-02, PROF-03 | `--stage build`, `--stage conformance` | wrong literal seen as guest_failure | pending |
| 03-T2 | 03 | 3 | PROF-02, PROF-03, CORE-01 | `--stage conformance` (resolve_*, profile_r1_equal, field_flip) | r1 TSV edit turns profile_r1_equal red | pending |
| 03-T3 | 03 | 3 | RUN-02 | `--stage outcome-sync` (compiled names) | compiled-list-differs | pending |
| 04-T1 | 04 | 4 | CONT-01, CONT-04 | `--stage conformance` (accept_256k_banksig, content_id_vs_shasum) | ORACLE_RULE_TEXT_DRIFT | pending |
| 04-T2 | 04 | 4 | CONT-01, CONT-04 | `--stage conformance` (384K, 1M, NIST, unmapped) | tests/controls/wrong-mirror | pending |
| 04-T3 | 04 | 4 | CONT-01 | `--stage conformance` (ref_size_8192, ref_size_512k) | per-row expectation edit | pending |
| 05-T1 | 05 | 4 | FIX-05 | `--stage rights-tree`, `--stage rights-negatives` | planted-pce | pending |
| 05-T2 | 05 | 4 | FIX-05 | `sh tests/rights/synthetic.sh`, `--selftest` | ten run-time controls, mutation-rights-rule | pending |
| 05-T3 | 05 | 4 | FIX-05 | `--stage audit-synthetic`, `--stage licences` | range-dirty, src-no-spdx | pending |
| 06-T1 | 06 | 5 | CONT-03 | `--stage sanitizers` | heap and shift canaries (SANITIZER_DEAD) | pending |
| 06-T2 | 06 | 5 | CONT-02, CONT-03, PROF-02 | `--stage conformance`, `--stage sanitizers`, `--selftest` | mutation-drop-precedence, alloc_shim_selfcheck | pending |
| 06-T3 | 06 | 5 | CONT-02, FIX-05 | `--stage conformance`, `--stage ledger-entries` | tests/controls/heuristic-reverse, redaction_seeded_leak_flagged | pending |
| 07-T1 | 07 | 6 | CORE-03, PROF-03 | `--stage conformance` (record_golden_384k_seeded, splitmix_seed0) | golden digit edit | pending |
| 07-T2 | 07 | 6 | CORE-01, CORE-03 | `--stage conformance`, `--stage sanitizers` | tests/controls/drop-region | pending |
| 07-T3 | 07 | 6 | PROF-03 | `--stage record-lint`, `--stage record-lint-negatives`, `--selftest` | path/pointer/timestamp controls, mutation-record-lint-pointer | pending |
| 08-T1 | 08 | 7 | CORE-02 | `--stage core-boundary-negatives` | time-include | pending |
| 08-T2 | 08 | 7 | CORE-02 | `--stage core-boundary-negatives`, `--selftest` | ten negative controls, allowlist and writable mutations | pending |
| 08-T3 | 08 | 7 | CORE-02 | `--stage core-boundary`, `dash scripts/check` | rule mutations | pending |
| 09-T1 | 09 | 8 | CORE-01 | `--stage conformance` (interleave_ab) | tests/controls/shared-state | pending |
| 09-T2 | 09 | 8 | CORE-01, CONT-03 | `--stage conformance`, `--stage sanitizers` | aliasing self-check, shared-state toy | pending |
| 09-T3 | 09 | 8 | CORE-01 | `--stage determinism`, `--stage determinism-negatives`, `--selftest` | differs control, mutation-determinism-compare | pending |
| 10-T1 | 10 | 9 | FIX-02 | `--stage fixture-negatives` | fix-missing-field, fix-unlisted | pending |
| 10-T2 | 10 | 9 | FIX-02, FIX-04 | `--stage inventory`, `--stage inventory-negatives`, `--stage fixtures`, `--selftest` | removed-required and five more, three mutations | pending |
| 10-T3 | 10 | 9 | FIX-04, RUN-02 | `--stage named-controls`, full gate, `--selftest`, dash | five NAMED_CONTROL lines, timeout control | pending |
| 11-T1 | 11 | 10 | CI-01 | `--stage workflow`, `--stage workflow-negatives`, actionlint (one-off) | pull-request-target | pending |
| 11-T2 | 11 | 10 | CI-01 | `--stage workflow-negatives`, `--selftest`, actionlint (one-off) | ten bad-workflow variants, mutation-workflow-pin | pending |
| 11-T3 | 11 | 10 | CI-01 | `--stage ci-timings-controls`, full gate | malformed TSV exit 2 | pending |
| 12-T1 | 12 | 11 | TOOL-02 | `--stage docs-claims`, `--stage docs-claims-negatives`, `--selftest` | five docs controls, mutation-docs-stale | pending |
| 12-T2 | 12 | 11 | TOOL-02, CI-01 | full gate, `--selftest`, dash, `sh scripts/audit-history`, `sh scripts/linux-repro` | container run labelled one-off | pending |
| 12-T3 | 12 | 11 | CI-01 | owner decision (blocking-human checkpoint) | not applicable | pending |
| 12-T4 | 12 | 11 | CI-01 | recorded outcome in docs/ci-timings.md | no remote write unless authorised | pending |

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
