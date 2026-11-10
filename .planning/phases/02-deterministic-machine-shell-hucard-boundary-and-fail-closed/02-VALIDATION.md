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

Filled by the planner per task from `02-RESEARCH.md` § "Phase Requirements to Test Map" (15 requirements, 5 success criteria). Every `<automated>` command is `sh scripts/check` or a named checker invoked by it, each paired with a `<fails_when>` and a named negative control. Task ids are assigned in the PLAN files.

| Requirement | Representative case ids / checks | Negative control |
|-------------|----------------------------------|------------------|
| PROF-02, PROF-03 | `ref_profile_*`, `resolve_*`, `field_flip_<name>` x13, `record_golden`, `record_lint` | `tests/gate/wrong-literal`, planted path/pointer in a record |
| CONT-01..03 | `ref_*` refusal rows, `accept_{256k,384k,1m}_banksig`, `nonmutation_*`, `alloc_zero_*`, `noheur_*`, sanitizer rerun | `tests/controls/wrong-mirror`, `heuristic-reverse` |
| CONT-04, CORE-02 | `content_id_nist_*`, `content_id_vs_shasum`, `core-boundary-check` | `<time.h>`, `clock_gettime`, `getenv`, `malloc`, global, local static, `__TIME__` controls |
| CORE-01, CORE-03 | `interleave_*`, `fresh_process`, `poweron_*`, `splitmix_*`, `digest_region_*` | `tests/controls/shared-state`, `drop-region` |
| RUN-02, FIX-04 | `conformance-ingest` fabricated-results controls, `inventory-check`, `outcome-sync` | removed-required, unknown token, duplicate id, zero tests, `assertions=0` |
| FIX-02, FIX-05 | `fixture-check`, `rights-check`, `redaction_synthetic_rom`, `audit-history --range` | missing field, unlisted fixture, run-time ROM-like binary, seeded leak |
| TOOL-02, CI-01 | `workflow-check`, `--timing`, README/AGENTS `grep -F 'sh scripts/check'` | bad-workflow variants under `tests/workflow/`; CI run evidence only if the owner authorises a push (D-64) |

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
