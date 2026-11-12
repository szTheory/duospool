---
phase: "04"
slug: "vce-vdc-raw-video"
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-11-11"
---

# Phase 4 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution. The requirement to check map, negative controls, oracles and Wave 0 gaps are in `04-RESEARCH.md` section "Validation Architecture"; the plans `04-NN-PLAN.md` are authoritative for stage names, case ids and verify commands, and on any mismatch the plan wins.

> `nyquist_compliant` stays false until execution proves the map: every requirement row green, every negative control seen red, the full gate, `--selftest`, the dash run, the second awk and audit-history clean. It is flipped in the closing plan, never earlier.

> **Evidence discipline.** Every green here is E0 repeatability. Raw-video hashes count only paired with a literal shown to fail (ER-11). Manual-page literals are E1 documentary only after page-image verification (ER-1, ER-2). No claim word from the withheld list is used.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None added. The sh+awk gate (`scripts/check`) is the only judge; the C conformance runner `build/gate/ds_runner` prints RESULT lines judged by `conformance-ingest` |
| **Config file** | `CMakePresets.json`, `tests/conformance/cases/*.toml`, `tests/conformance/REQUIRED`, `fixtures/manifest/*.toml`, `fixtures/src/*.{s,checks,vexp}`, `ledger/REQUIRED` |
| **Quick run command** | `sh scripts/check --stage conformance` (one case: `build/gate/ds_runner ... --only ID`, see the research file); narrow stages `video-check`, `fixture-build`, `fixture-lint`, `inventory`, `docs-claims` |
| **Full suite command** | `sh scripts/check`, then `sh scripts/check --selftest`, then `dash scripts/check`, then `sh scripts/audit-history`; sanitizers run inside the full gate |
| **Linux strict C11** | `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` per C-touching plan; `gate-gcc gate-clang core-boundary-gcc` per wave end; all six parts at the checkpoint and the close-out |
| **Estimated runtime** | Phase 3 close: 56 stages, CI job 342-541 s (gate 119 s, second awk 104 s); Phase 4 adds about +35 s local, +55 s CI (range +30..+95); staging levers apply if CI exceeds 600 s or conformance exceeds 60 s |

---

## Sampling Rate

- **After every task commit:** the task's own narrow command (`ds_runner --only ID` plus the narrowest stage)
- **After every plan:** `sh scripts/check` green with goldens regenerated and the plan's ledger pins edited
- **After every wave:** the Linux parts above
- **Before verify-work:** full suite green, `--selftest`, dash, second awk
- **Max feedback latency:** under 60 s for the per-task command

---

## Per-Task Verification Map

Filled by the planner and executors. Requirement rows (VID-01..VID-06 with literal source, failure control and wave) are in `04-RESEARCH.md` section "Phase Requirements to Test Map".

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 04-TBD | - | - | VID-01..06 | T-04-TBD | see plans | conformance | per plan | W0 | pending |

*Status: pending, green, red, flaky*

---

## Wave 0 Requirements

- [ ] `H7-` and `H6-` cite labels accepted by the fixture lint rules, rows in `cites.txt`, both manuals named in `TOOLS.md` (04-01)
- [ ] docs-check claim words and policy line with a negative control (04-01)
- [ ] ledger entries and amendments; each constants row id exists before `constants.def` lands (04-01)
- [ ] sink channel table with a `vid` channel and no producer; fold placement in `ds_cpu_step_obs` (04-02)
- [ ] snapshot capacities, hand tables and the two goldens (04-03)
- [ ] optional second EXPECT column for stays-green toys (before 04-06)
- [ ] `.vexp.tsv` channel and the `video-check` skeleton (04-05)
- [ ] `RODATA` segment in `hucard.cfg` and `fixtures/lib/video.inc` (04-03)

---

## Manual-Only Verifications

None expected: every behaviour is checked by the gate. Optional hardware captures are not needed and not requested.

---

## Validation Sign-Off

- [ ] Every task has an automated verify or a Wave 0 dependency
- [ ] Sampling continuity: no three consecutive tasks without an automated verify
- [ ] Every negative control seen red
- [ ] No watch-mode flags
- [ ] `nyquist_compliant: true` set only by the closing plan

**Approval:** pending
