---
phase: "1"
slug: "owner-decisions-claim-ledger-and-toolchain-spike"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-11-09"
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Source: `01-RESEARCH.md` §"Validation Architecture".

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None external. POSIX `sh` + `awk` checkers; each checker is a tested subject through its own negative-control directories |
| **Config file** | none — Wave 0 creates `scripts/check` and the checkers |
| **Quick run command** | `sh scripts/check` |
| **Full suite command** | `sh scripts/check && sh spikes/tool-01/gates.sh rust && sh spikes/tool-01/gates.sh c` (spike gates only in the spike wave) |
| **Estimated runtime** | ~2 seconds quick; spike gates several minutes (builds + 60 s fuzz per option) |
| **Exit codes** | `0` pass, `1` violation / expectation not met, `2` harness or usage error (never counted as an expected failure) |

---

## Sampling Rate

- **After every task commit:** Run `sh scripts/check`, plus `dash -n` on any script the task touched
- **After every plan wave:** Run `sh scripts/check` plus that wave's own gate (`gates.sh` in the spike wave, `scripts/audit-history .` in the publication wave)
- **Before `/gsd-verify-work`:** The full suite must be green, record 0001 complete, the audit clean, and the owner checkpoint resolved
- **Max feedback latency:** 5 seconds for the quick gate

---

## Per-Task Verification Map

The planner fills in Task IDs. Rows are keyed by success criterion; each row names its negative control and the observable signal that means failure.

| Criterion | Requirement | Behavior | Test Type | Automated Command | Negative control (must fail) | File Exists | Status |
|-----------|-------------|----------|-----------|-------------------|------------------------------|-------------|--------|
| SC3 | PROF-04 | Ledger schema; `supported` needs E1/E2/E3 | unit | `scripts/ledger-check ledger/entries/` | `ledger/negative/<case>/` each fails with its EXPECT code(s) | ❌ W0 | ⬜ pending |
| SC3 | PROF-04 | Mislabelled E0/E4-only "supported" sample rejected | unit | `scripts/ledger-check --expect SUPPORTED_NO_QUALIFYING_EVIDENCE ledger/negative/e0-e4/` | deleting the rule → `EXPECT_MISMATCH`, exit 1 | ❌ W0 | ⬜ pending |
| SC3 | PROF-04 | Checker not vacuous | meta | `scripts/check --selftest` | missing stage sentinel → `NO_SENTINEL` | ❌ W0 | ⬜ pending |
| SC4 | PROF-05 | 9 mandated conflicts + C14 present, `assumed`, assumption set | unit | `scripts/ledger-check --required ledger/REQUIRED ledger/entries/` | `required-missing` → `REQUIRED_MISSING` | ❌ W1 | ⬜ pending |
| SC2 | PROF-01 | Profile names every field; owner-selected or open; no value on open fields | unit | `scripts/profile-check profiles/pce-hucard-2btn/r1.toml` | `tests/profile/<case>/` (open-with-value, selected-without-decision, unknown/missing field, bad status, dangling ledger ref) | ❌ W1 | ⬜ pending |
| SC2 | PROF-01 | Criterion-2 mapping table in 0002 (D-32) | grep | `grep -F` per criterion term in `docs/decisions/0002-first-profile.md` | removing a mapping row fails | ❌ W1 | ⬜ pending |
| SC5 | — | Decision records 0002–0008 have valid front matter; 0005 has the no-GPL sentence | unit | `scripts/decision-check docs/decisions` | `tests/decision/<case>/` (owner-recorded without statement, proposed-default without default-in-force, missing blocks, unknown status) | ❌ W1 | ⬜ pending |
| SC5 | — | Provenance register valid; bad markers rejected | unit | `scripts/provenance-check` | `tests/provenance/<case>/` (missing field, GPL SPDX, unresolved marker, missing path) | ❌ W0 | ⬜ pending |
| SC1 | TOOL-01 | Protocol committed before any spike code | git | protocol-before-code check | synthetic repo with spike first → `PROTOCOL_AFTER_SPIKE` | ❌ W2 | ⬜ pending |
| SC1 | TOOL-01 | Both skeletons pass hard gates on the shared manifest | integration | `sh spikes/tool-01/gates.sh {rust,c}` | wrong expectation rejected; header removed breaks consumer; seeded path leak caught; planted fuzz bug found within 60 s | ❌ W3 | ⬜ pending |
| SC1 | TOOL-01 | Record 0001 has every metric for both options (value or `not demonstrated: <reason>`) | awk | `sh scripts/check-results docs/decisions/0001-language-build.md docs/decisions/0001-language-build.protocol.md` | a deleted row → `RESULT_MISSING` | ❌ W3 | ⬜ pending |
| SC1 | TOOL-01 | Score arithmetic and D-07 margin rule | unit | `awk -f spikes/tool-01/score.awk <scores>` | a tie, a 9.9-point lead and a leader that fails a gate each print `proposed-default` | ❌ W2 | ⬜ pending |
| D-01 | — | Licence files canonical | unit | `shasum -a 256 LICENSE-APACHE` matches the pinned hash; grep holder line | an altered file fails | ❌ W0 | ⬜ pending |
| D-02/D-33 | — | History clean before publication | script | `sh scripts/audit-history .` + `gitleaks git` | synthetic repo with `x.pce` (8 KiB) fails; a missing object fails | ❌ W4 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `scripts/lib/toml-subset.awk`, `scripts/lib/ledger-rules.awk`, `scripts/ledger-check`: directory mode, stage sentinel, `ENVIRON` instead of `-v`, comma-set `--expect`, REQUIRED, DUP_ID, status counts
- [ ] `scripts/check`, aggregating every check with exit-code discipline (D-30), plus `--selftest`
- [ ] Negative corpora: `ledger/negative/*`, `ledger/samples/ok-*`, `tests/{decision,provenance,profile}/*`
- [ ] `scripts/provenance-check` + `PROVENANCE.md` with zero entries
- [ ] `.gitignore` (build outputs, generated spike fixtures), `LICENSE-MIT`, `LICENSE-APACHE`, `LICENSE-0BSD`, SPDX header convention
- [ ] Spike-wave toolchain installs (rustup 1.99.1 + nightly-2026-11-09, wasm32-wasip1, cargo-fuzz 0.13.2, hyperfine, wasmtime)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Owner reviews the audit findings and approves the public push | D-02 / D-33 | One-way outward action; owner authority | At the `checkpoint:decision`, review the audit output (including `[home]/...` paths in research docs and the LibFuturo/sibling mentions), then approve or decline `gh repo create --public` |
| Owner amends the D-13 weights or rubric anchors (optional) | TOOL-01 | Owner judgement; must happen before the protocol commit | Review the protocol draft before it is committed |
| Language decision when the D-07 rule yields `proposed-default` | TOOL-01 | Owner authority | Review 0001 and record a verbatim choice |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s (quick gate)
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
