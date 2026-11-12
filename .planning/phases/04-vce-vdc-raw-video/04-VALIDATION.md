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

## Amendments and exceptions (recorded from plan check 1; the plans carry the detail)

| # | Item | Ruling | Owner plan |
|---|------|--------|------------|
| A1 | Plan set | 19 plans in 18 waves: CONTEXT 04-03 = 04-03 (layout, bump), 04-18 (v3, refusals), 04-04 (tracer Task 1), 04-19 (vce_pal completion, toy, read logging, window rules), 04-05 (hardening); 04-18 and 04-19 are stop-rule splits. CONTEXT 04-06 = 04-08 and 04-17. Checkpoint 04-13, close-out 04-16, spares 04-20, 04-21; order follows depends_on and waves | all |
| A2 | Plan sizing | Phase 2 D-81 file caps superseded for Phase 4: 04-CONTEXT.md reconciliations, writer note 6 | all |
| A3 | Toy vid-scroll-latch (D-173) | D-173's "scroll applied to the current line" is unobservable under R1 (the write's own line is emitted at its EV_LINE; from line n+1 on it equals the next-line rule). The toy instead removes the row-counter reload on a BYR write (vdc.c), killed by the BYR next-line rows [MAN H7-7] | 04-11 (anchor), 04-12 (toy) |
| A4 | BXR next-line liveness | No renderer mutant (toy cap of 10 spent; under R1 the only mutant class, a late or missing latch, is what record faults emulate). The BXR rows are live by fault sweep F1 (shift) and F8 (swap lines K and K+1) and the REL scroll_x_shift row: a recorded limit | 04-12 |
| A5 | INV_TOY_NO_LITERAL scope | Applied to toys mutating src/core/vce.c, vdc.c, beam.c or vid.c. fold-skipped-at-step-end (cpu.c) lists video_vce_pal if 04-05 observes it red (exception lapses); otherwise its EXPECT comment (04-05) records that no literal group fails reliably and its liveness rests on chunk_equiv_video_vce_pal and vdc_fold | 04-05, 04-07 |
| A6 | ER-10 classes | GUEST only for in-guest arithmetic over guest-written values (allowlist in vexp-rules.awk, initially pal_bijection); rows using 1365, frame_lines, the window origin or the write join are ASSUMED (LED-VID-0004, -0001, -0005); MAN value rows judged one whole line from their write; latch rows need a 250-clock margin | 04-04, 04-07, 04-09, 04-12 |
| A7 | F0 liveness | F0 proves only that a group's comparison is live; a group whose only kill is F0 (for example the vce_pal border) takes its liveness from a toy (vid-palette-mask) or a null-renderer control | 04-07 |
| A8 | Fixture includes | Per-feature includes holding macros and equates only (vce.inc, vdc.inc, vdma.inc, spin.inc, bg.inc, spr.inc), never edited by a later plan, so every pinned image keeps its bytes | 04-19 onward |
| A9 | Requirement ticks | Frontmatter requirements_advanced marks ids a plan advances but does not complete; REQUIREMENTS.md ticks happen only through the standard plan-complete step; 04-13 ticks none (VID-06 stays open even if the cut is taken) | all |
| A10 | D-174 loop cost and check (4) | 24-bit add and P <= 12 cycles cannot both hold branch-free; the loop is inc a / bra (6 cycles, 18 clocks), the 24-bit clc/adc sum runs in the handlers. The +-1.5-iteration windows for 245, 341, 1,610 clocks are disjoint only if the last-display-line handler returns before VD (under 81 cycles); 04-17 records whether it held | 04-17 |
| A11 | vce_pal div before the CR $01 write | The fixture writes CR $00, waits at least 5,460 master clocks (speed stated in its comment), then writes CR $01; div 4 rows are judged only between that write and CR $01, never on records resting on the power-on CR (LED-PROF-0003) | 04-04, 04-07 |

## Wave 0 Requirements

- [ ] `H7-` and `H6-` cite labels accepted by the fixture lint rules, rows in `cites.txt`, both manuals named in `TOOLS.md` (04-01)
- [ ] docs-check claim words and policy line with a negative control (04-01)
- [ ] ledger entries and amendments; each constants row id exists before `constants.def` lands (04-01)
- [ ] sink channel table with a `vid` channel and no producer; fold placement in `ds_cpu_step_obs` (04-02)
- [ ] snapshot capacities, hand tables and the two goldens (plan file 04-03)
- [ ] seeded-prng-v3, the new default guest policy list and the restore-refusal cases snapshot_refusal_vdc (plan file 04-18, the stop-rule split of 04-03)
- [ ] dev read logging of windows 0 and 1 and the D-166 fold-window restore rules (plan file 04-19)
- [ ] optional second EXPECT column for stays-green toys (04-17, before the first stays-green toy)
- [ ] `.vexp.tsv` channel in `fixture-build` and the `video-check` stage with its mini-roots (plan file 04-07)
- [ ] `RODATA` segment in `hucard.cfg` and the first per-feature include `fixtures/lib/vce.inc` (plan file 04-19, the stop-rule split of 04-04); later includes vdc.inc (04-06), vdma.inc (04-10), bg.inc (04-11), spr.inc (04-14), spin.inc (04-17); there is no video.inc

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
