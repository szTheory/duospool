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

Filled at checkpoint A (plan 04-13) from the plans 04-01 to 04-12, 04-17 to 04-19 and this plan; plans 04-14 to 04-16 add their own rows. Green means the task verify and the full gate passed at its commit (see each SUMMARY). Requirement rows (VID-01..VID-06 with literal source, failure control and wave) are in `04-RESEARCH.md` section "Phase Requirements to Test Map".

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 04-01-T1 | 04-01 | W1 | VID-01..06 | T-04-01,T-04-02,T-04-03 | ER-1 image reads, cites.txt and TOOLS.md rows, and the H7/H6 cite prefixes in both fixt... | conformance and gate | `stage fixture-lint,fixture-lint-negatives,fixtures,fixture-negatives,rights-tree,licences` | yes | green |
| 04-01-T2 | 04-01 | W1 | VID-01..06 | T-04-01,T-04-02,T-04-03 | 17 new ledger entries, 10 amendments and their REQUIRED lines | conformance and gate | `stage ledger-entries,ledger-negatives,inventory,fixture-lint,docs-claims` | yes | green |
| 04-01-T3 | 04-01 | W1 | VID-01..06 | T-04-01,T-04-02,T-04-03 | docs-check withheld words of D-181 with a second README policy line and a negative mini... | conformance and gate | `stage docs-claims,docs-claims-negatives; selftest` | yes | green |
| 04-02-T1 | 04-02 | W1 | VID-06 | T-04-04,T-04-05,T-04-06 | Tracer - a ds_run request with a vid sink reaches the obs layer, which writes its 32-by... | conformance and gate | `stage build,core-boundary,conformance; RESULT vid_chan_append` | yes | green |
| 04-02-T2 | 04-02 | W1 | VID-06 | T-04-04,T-04-05,T-04-06 | Whole-record drop-newest append with sticky full, table-driven sink checks, refusal var... | conformance and gate | `stage conformance,sanitizers; RESULT vid_chan_append,run_refuse_memory_invalid,run_refuse_struct_size` | yes | green |
| 04-02-T3 | 04-02 | W1 | VID-06 | T-04-04,T-04-05,T-04-06 | Inert fold skeleton at the three fold points with the step-end fold inside ds_cpu_step_obs | conformance and gate | `stage conformance; selftest; RESULT insn_sink_shapes` | yes | green |
| 04-03-T1 | 04-03 | W2 | VID-05,VID-06 | T-04-09,T-04-11,T-04-12 | Complete state layout, seven regions and decoders, boot values, model and constants row... | conformance and gate | `stage build,core-boundary; selftest` | yes | green |
| 04-03-T2 | 04-03 | W2 | VID-05,VID-06 | T-04-09,T-04-11,T-04-12 | Regenerate the whole red list by the independent method of D-165 | conformance and gate | `stage conformance,record-lint,determinism; RESULT digest_stream_prefix,record_golden_256k_zero` | yes | green |
| 04-04-T1 | 04-04 | W2 | VID-05,VID-06 | T-04-15,T-04-16 | Tracer - guest VCE palette and CR writes become header-only scanline records in the vid... | conformance and gate | `stage fixture-lint,core-boundary,conformance; RESULT video_vce_pal,vce_pal` | yes | green |
| 04-05-T1 | 04-05 | W2 | VID-06 | T-04-17,T-04-18,T-04-19 | Tracer - a chunked video run equals one run byte for byte, end to end through the new c... | conformance and gate | `stage conformance; RESULT chunk_equiv_video_vce_pal` | yes | green |
| 04-05-T2 | 04-05 | W2 | VID-06 | T-04-17,T-04-18,T-04-19 | vdc_fold, restore at video stops, the observer vid matrix and sink non-interference | conformance and gate | `stage conformance,sanitizers; RESULT vdc_fold,observer_vce_pal,vid_noninterference,snapshot_restore_video` | yes | green |
| 04-05-T3 | 04-05 | W2 | VID-06 | T-04-17,T-04-18,T-04-19 | Toys vid-emit-mutates and fold-skipped-at-step-end, --video-out with external recomputa... | conformance and gate | `stage conformance,ci-timings-controls; selftest; RESULT video_out_rights; CONTROL vid-emit-mutates,fold-skipped-at-step-end` | yes | green |
| 04-06-T1 | 04-06 | W3 | VID-03 | T-04-20,T-04-21,T-04-22 | Tracer - a word written through ST0/ST1/ST2 lands in instance VRAM and comes back throu... | conformance and gate | `stage fixture-lint,conformance; RESULT vdc_vram,vdc_reg,cpu_huc` | yes | green |
| 04-06-T2 | 04-06 | W3 | VID-03 | T-04-20,T-04-21,T-04-22 | vdc_reg complete (about 36 checks), mmu_mpr checks 23 and 24, Phase 3 VDC writers re-ve... | conformance and gate | `stage fixture-lint,fixtures,conformance; RESULT mutate_sweep_vdc_reg,mmu_mpr_256k,snapshot_hostile` | yes | green |
| 04-06-T3 | 04-06 | W3 | VID-03 | T-04-20,T-04-21,T-04-22 | vdc_vram_sweep and vdc_regmask under the sanitizers, toy vrr-no-readahead and the ledge... | conformance and gate | `stage conformance,sanitizers; RESULT vdc_vram_sweep,vdc_regmask; CONTROL vrr-no-readahead` | yes | green |
| 04-07-T1 | 04-07 | W3 | VID-06 | T-04-24,T-04-25,T-04-26 | Tracer - vce_pal literals move into vce_pal.vexp, travel through fixture-build as vce-p... | conformance and gate | `stage fixture-build,conformance; RESULT video_vce_pal; CONTROL vid-palette-mask` | yes | green |
| 04-07-T2 | 04-07 | W3 | VID-06 | T-04-24,T-04-25,T-04-26 | The static stage video-check with its rules, mini-roots and three selftest mutations | conformance and gate | `stage video-check,licences; selftest` | yes | green |
| 04-07-T3 | 04-07 | W3 | VID-06 | T-04-24,T-04-25,T-04-26 | Fault sweep F0..F9, golden role with runtime refusal, video_hash_not_verdict and the ju... | conformance and gate | `stage conformance,video-check; RESULT video_fault_sweep_vce_pal,video_hash_not_verdict,video_judge_forms,video_vce_pal_golden` | yes | green |
| 04-08-T1 | 04-08 | W4 | VID-04 | T-04-28,T-04-29 | Tracer - an RCR match raises RR through ds_vdc_flag, drives IRQ1, and the guest handler... | conformance and gate | `stage fixture-lint,conformance; RESULT vdc_stat,irq_ctl_pre,cpu_huc` | yes | green |
| 04-08-T2 | 04-08 | W4 | VID-04 | T-04-28,T-04-29 | VBlank at vd_irq_phase, rcr_origin behaviour, model_param_vdc, vdc_stat complete and th... | conformance and gate | `stage conformance; RESULT vdc_stat,mutate_sweep_vdc_stat,model_param_vdc,video_vce_pal` | yes | green |
| 04-09-T1 | 04-09 | W4 | VID-05,VID-06 | T-04-31,T-04-32,T-04-33 | Tracer - a programmed HDS/HDW and VCE CR produce display-line records with the active w... | conformance and gate | `stage video-check,conformance; RESULT video_vce_dot,video_vce_pal` | yes | green |
| 04-09-T2 | 04-09 | W4 | VID-05,VID-06 | T-04-31,T-04-32,T-04-33 | All four CR encodings, the mid-frame change, clipping and the frame-length and active-w... | conformance and gate | `stage video-check,conformance; RESULT vid_active_window,vid_frame_lines,video_fault_sweep_vce_dot,mutate_sweep_vce_dot` | yes | green |
| 04-09-T3 | 04-09 | W4 | VID-05,VID-06 | T-04-31,T-04-32,T-04-33 | Toy vid-dot-div, the vce_pal golden row and the ledger pins and amendments | conformance and gate | `stage conformance,video-check,ledger-entries,inventory; CONTROL vid-dot-div` | yes | green |
| 04-10-T1 | 04-10 | W4 | VID-03,VID-04 | T-04-34,T-04-35,T-04-36 | Tracer - a LENR MSB write in burst moves 64 VRAM words, seen by guest VRR readback and ... | conformance and gate | `stage fixture-lint,conformance; RESULT vdc_dma,vdc_dma_vram` | yes | green |
| 04-10-T2 | 04-10 | W4 | VID-03,VID-04 | T-04-34,T-04-35,T-04-36 | vdc_dma complete with the burst latch, cancel, overrun and expected-red controls, and v... | conformance and gate | `stage conformance,video-check; RESULT mutate_sweep_vdc_dma,vdc_advance_pure,video_vce_dot` | yes | green |
| 04-10-T3 | 04-10 | W4 | VID-03,VID-04 | T-04-34,T-04-35,T-04-36 | SATB DMA with DS, vdc_satb and vdc_satb_data, DS and DV rows of vdc_stat, mid-transfer ... | conformance and gate | `stage conformance,sanitizers; RESULT vdc_satb,vdc_satb_data,vdc_snapshot_mid_dma,vdc_stat` | yes | green |
| 04-11-T1 | 04-11 | W5 | VID-01 | T-04-38,T-04-39,T-04-40 | Tracer - one BAT entry pointing at one solid tile appears as its bank-and-pixel index a... | conformance and gate | `stage video-check,conformance; RESULT video_bg_basic_solid,vid_noninterference` | yes | green |
| 04-11-T2 | 04-11 | W5 | VID-01 | T-04-38,T-04-39,T-04-40 | bg_basic complete with the solid and asymmetric scenes, the unmodelled-mode case, fault... | conformance and gate | `stage video-check,conformance; RESULT video_bg_basic_asym,video_fault_sweep_bg_basic,video_unmodelled_mode,mutate_sweep_bg_basic` | yes | green |
| 04-11-T3 | 04-11 | W5 | VID-01 | T-04-38,T-04-39,T-04-40 | Toy vid-bit-order with the stays-green solid case, and the ledger pins | conformance and gate | `stage conformance,video-check,ledger-entries; selftest; CONTROL vid-bit-order` | yes | green |
| 04-12-T1 | 04-12 | W5 | VID-01,VID-06 | T-04-41,T-04-42,T-04-43 | Tracer - a counted mid-frame BXR write shifts the next line and not its own, joined by ... | conformance and gate | `stage video-check,conformance; RESULT video_bg_scroll,bg_scroll` | yes | green |
| 04-12-T2 | 04-12 | W5 | VID-01,VID-06 | T-04-41,T-04-42,T-04-43 | bg_scroll complete (BYR next-line, 64x64 wrap) and bg_enab (BB/SB matrix, burst, backdr... | conformance and gate | `stage video-check,conformance; RESULT video_bg_scroll_asym,video_bg_enab,video_fault_sweep_bg_enab,mutate_sweep_bg_scroll` | yes | green |
| 04-12-T3 | 04-12 | W5 | VID-01,VID-06 | T-04-41,T-04-42,T-04-43 | Toy vid-scroll-latch, chunk and restore invariance on a video scene, and the ledger pins | conformance and gate | `stage conformance,video-check; RESULT chunk_equiv_video_bg_scroll,snapshot_restore_video_bg; CONTROL vid-scroll-latch` | yes | green |
| 04-17-T1 | 04-17 | W4 | VID-04 | T-04-30,T-04-56 | Tracer - vdc_irq spin-counter fixture for 262 and 263 lines, the vdc_irq_agree kind and... | conformance and gate | `stage conformance` | yes | green |
| 04-17-T2 | 04-17 | W4 | VID-04 | T-04-30,T-04-56 | EXPECT pass column for stays-green toys, the guest judge subset, toys vid-rcr-base and ... | conformance and gate | `stage conformance,inventory; selftest; RESULT vdc_irq_ratio; CONTROL vid-rcr-base,status-read-keeps-flags` | yes | green |
| 04-18-T1 | 04-18 | W2 | VID-06 | T-04-08,T-04-10 | Additive power-on policy seeded-prng-v3 at its nine sites and the new default guest pol... | conformance and gate | `stage conformance,inventory; RESULT poweron_v3_draw_order,poweron_v3_smoke,mmu_mpr_policy_diff,snapshot_refusal_fields` | yes | green |
| 04-18-T2 | 04-18 | W2 | VID-06 | T-04-08,T-04-10 | Restore refusals for the new regions and the hostile-corpus flips | conformance and gate | `stage conformance,sanitizers; selftest; RESULT snapshot_refusal_vdc,snapshot_hostile` | yes | green |
| 04-19-T1 | 04-19 | W2 | VID-05,VID-06 | T-04-13,T-04-14 | vce_pal complete (about 28 checks), RODATA and vce.inc, mutate sweep, toy vid-palette-m... | conformance and gate | `stage fixture-lint,fixtures,conformance; selftest; RESULT mutate_sweep_vce_pal; CONTROL vid-palette-mask` | yes | green |
| 04-19-T2 | 04-19 | W2 | VID-05,VID-06 | T-04-13,T-04-14 | Dev read logging of windows 0 and 1, its cascade, mmu_mpr checks 25 and 26, and the D-1... | conformance and gate | `stage conformance,fixtures,sanitizers; RESULT dev_log_records,dev_log_windows,snapshot_refusal_vdc` | yes | green |
| 04-13-T1 | 04-13 | W6 | VID-01..06 | T-04-44,T-04-45 | vdc_random_regs, vdc_poweron_policy_diff with the nopal control, VDC/VCE traffic in the random differential | conformance and gate | `stage fixtures,conformance,sanitizers; RESULT vdc_random_regs,vdc_poweron_policy_diff` | yes | green |
| 04-13-T2 | 04-13 | W6 | VID-01..06 | T-04-44,T-04-45 | The checkpoint verification set, cost record against D-176 and the cut-line record | conformance and gate | `stage ci-timings-controls,docs-claims` | yes | green |
| 04-13-T3 | 04-13 | W6 | VID-01..06 | T-04-46 | ER-2 promotion pass, README wording, VALIDATION rows | conformance and gate | `stage ledger-entries,inventory,fixture-lint,docs-claims,docs-claims-negatives` | yes | green |

*Status: pending, green, red, flaky*

---

## Amendments and exceptions (recorded from plan check 1; the plans carry the detail)

| # | Item | Ruling | Owner plan |
|---|------|--------|------------|
| A1 | Plan set | 19 plans in 18 waves: CONTEXT 04-03 = 04-03 (layout, bump), 04-18 (v3, refusals), 04-04 (tracer Task 1), 04-19 (vce_pal completion, toy, read logging, window rules), 04-05 (hardening); 04-18 and 04-19 are stop-rule splits. CONTEXT 04-06 = 04-08 and 04-17. Checkpoint 04-13, close-out 04-16, spares 04-20, 04-21; order follows depends_on and waves | all |
| A2 | Plan sizing | Phase 2 D-81 file caps superseded for Phase 4: 04-CONTEXT.md reconciliations, writer note 6 | all |
| A3 | Toy vid-scroll-latch (D-173) | D-173's "scroll applied to the current line" is unobservable under R1 (the write's own line is emitted at its EV_LINE; from line n+1 on it equals the next-line rule). The toy instead removes the row-counter reload on a BYR write (vdc.c), killed by the BYR next-line rows [MAN H7-7] | 04-11 (anchor), 04-12 (toy) **Observed by 04-12:** the toy (tests/controls/vid-scroll-latch, anchor the one statement of the BYR write path in vdc.c, replacement an empty statement) turns guest_failure exactly video_bg_scroll (its byr_next rows, MAN H7-7) and video_fault_sweep_bg_scroll (its baseline); a run of every case against the toy binary shows no other guest_failure, and the real core passes both (REQUIRED). EXPECT names those two ids. |
| A4 | BXR next-line liveness | No renderer mutant (toy cap of 10 spent; under R1 the only mutant class, a late or missing latch, is what record faults emulate). The BXR rows are live by fault sweep F1 (shift) and F8 (swap lines K and K+1) and the REL scroll_x_shift row: a recorded limit | 04-12 **Observed by 04-12:** the fault sweep video_fault_sweep_bg_scroll flips the BXR group latch under F0, F1, F7 and F8 (its first row is the line after the first write, so F8 exchanges the unshifted line with the shifted one) and the REL group latch_rel under F0, F1 and F7 (all declared faults flipped, none vacuous); the BXR rows are classed MAN H7-7 on a 32-dot period row that holds for a scroll in either direction (a shift of 16 dots, ER-12), and the judge refuses a write within 250 master clocks of a line origin as a fault of the case (A6). The limit stands: no renderer mutant exists for a late or missing BXR latch. |
| A5 | INV_TOY_NO_LITERAL scope | Applied to toys mutating src/core/vce.c, vdc.c, beam.c or vid.c. fold-skipped-at-step-end (cpu.c) lists video_vce_pal if 04-05 observes it red (exception lapses); otherwise its EXPECT comment (04-05) records that no literal group fails reliably and its liveness rests on chunk_equiv_video_vce_pal and vdc_fold. Observed by 04-05: under the toy video_vce_pal is harness_failure (all its literal rows pass, the border window is empty), so the exception stands; guest_failure on the toy: chunk_equiv_video_vce_pal, vdc_fold, observer_vce_pal, vid_noninterference, snapshot_restore_video (the five ids of its EXPECT) | 04-05, 04-07 |
| A6 | ER-10 classes | GUEST only for in-guest arithmetic over guest-written values (allowlist in vexp-rules.awk, initially pal_bijection); rows using 1365, frame_lines, the window origin or the write join are ASSUMED (LED-VID-0004, -0001, -0005); MAN value rows judged one whole line from their write; latch rows need a 250-clock margin | 04-04, 04-07, 04-09, 04-12 |
| A7 | F0 liveness | F0 proves only that a group's comparison is live; a group whose only kill is F0 (for example the vce_pal border) takes its liveness from a toy (vid-palette-mask) or a null-renderer control | 04-07 |
| A8 | Fixture includes | Per-feature includes holding macros and equates only (vce.inc, vdc.inc, vdma.inc, spin.inc, bg.inc, spr.inc), never edited by a later plan, so every pinned image keeps its bytes | 04-19 onward |
| A9 | Requirement ticks | Frontmatter requirements_advanced marks ids a plan advances but does not complete; REQUIREMENTS.md ticks happen only through the standard plan-complete step; 04-13 ticks none (VID-06 stays open even if the cut is taken) | all |
| A10 | D-174 loop cost and check (4) | 24-bit add and P <= 12 cycles cannot both hold branch-free; the loop is inc a / bra (6 cycles, 18 clocks), the 24-bit clc/adc sum runs in the handlers. The +-1.5-iteration windows for 245, 341, 1,610 clocks are disjoint only if the last-display-line handler returns before VD (under 81 cycles); 04-17 records whether it held | 04-17 |
| A11 | vce_pal div before the CR $01 write | The fixture writes CR $00, waits at least 5,460 master clocks (speed stated in its comment), then writes CR $01; div 4 rows are judged only between that write and CR $01, never on records resting on the power-on CR (LED-PROF-0003) | 04-04, 04-07 |

**A10 outcome (plan 04-17, the separation held).** The handler of the last display line (h_last in fixtures/src/vdc_irq.s) is interrupt entry 8 + vector jump 7 + body 44 + RTI 7 = 66 cycles, summed from tests/oracle/huc6280-opcodes.tsv in the source comment and asserted at link time to be at most 72. With a wait of under 5 cycles before the entry it returns under 71 cycles
after the RCR event, before VD, which comes 81.7 cycles later by default. The gaps measured by the guest in iterations of 18 master clocks are 14, 19 and 90 for the default rows, vd_irq_phase = hdw_start and rcr_origin = line_before: three disjoint windows, each within 1.5 iterations of the hand-derived 245, 341 and 1,610 master clocks. E0/E2 modelling assumption (ASSUMED LED-
IRQ-0001, LED-IRQ-0010); the handler cost itself rests on ASSUMED LED-CPU-0009 (entry cycles) and LED-CPU-0017 (window penalty). The ratio check assumes no constant (GUEST).

## Wave 0 Requirements

- [x] `H7-` and `H6-` cite labels accepted by the fixture lint rules, rows in `cites.txt`, both manuals named in `TOOLS.md` (04-01)
- [x] docs-check claim words and policy line with a negative control (04-01)
- [x] ledger entries and amendments; each constants row id exists before `constants.def` lands (04-01)
- [x] sink channel table with a `vid` channel and no producer; fold placement in `ds_cpu_step_obs` (04-02)
- [x] snapshot capacities, hand tables and the two goldens (plan file 04-03)
- [x] seeded-prng-v3, the new default guest policy list and the restore-refusal cases snapshot_refusal_vdc (plan file 04-18, the stop-rule split of 04-03)
- [x] dev read logging of windows 0 and 1 and the D-166 fold-window restore rules (plan file 04-19)
- [x] optional second EXPECT column for stays-green toys (04-17, before the first stays-green toy)
- [x] `.vexp.tsv` channel in `fixture-build` and the `video-check` stage with its mini-roots (plan file 04-07)
- [ ] (done at checkpoint A except spr.inc, which plan 04-14 creates) `RODATA` segment in `hucard.cfg` and the first per-feature include `fixtures/lib/vce.inc` (plan file 04-19, the stop-rule split of 04-04); later includes vdc.inc (04-06), vdma.inc (04-10), bg.inc (04-11), spr.inc (04-14), spin.inc (04-17); there is no video.inc

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

---

## Checkpoint A inputs (plan 04-13)

Recorded for the close-out, whichever way the cut line of D-180 is decided. The cut itself is the orchestrator's decision from the triggers in `docs/ci-timings.md` (checkpoint A). `nyquist_compliant` stays false here: it is set true by the plan that closes the phase (this plan if the cut is taken, with the background scope; otherwise 04-16).

- **Automated at this checkpoint:** `sh scripts/check`, `dash scripts/check`, `sh scripts/check --selftest`, `sh scripts/audit-history --range 925662a..HEAD`, the six Linux parts (`default-awk`, `mawk`, `gawk`, `gate-gcc`, `gate-clang`, `core-boundary-gcc`), the sanitizers over every case
  including `vdc_random_regs` and the random differentials, and all 25 toy controls RED_OK. Results are in `docs/ci-timings.md` and in the plan SUMMARY.
- **Handed off (cannot be automated here):** a push and a CI run for the job time (D-64, owner authority); the owner's preference for an earlier pull request (cut-line trigger 4).
- **Not automated by design:** hardware captures (none requested), any claim beyond E0 repeatability and the E1 documentary entries.
