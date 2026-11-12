# Phase 4 plan check 1 (gsd-plan-checker)

Phase: 04-vce-vdc-raw-video. Plans checked: 04-01 .. 04-16 (16 files). Inputs: 04-CONTEXT (D-135..D-181, ER-1..ER-13, errata), 04-RESEARCH (F-1..F-25), 04-PATTERNS, 04-VALIDATION, ROADMAP Phase 4, REQUIREMENTS VID-01..06, AGENTS.md, Phase 3 plans as house format.
Result: ISSUES FOUND. 0 blockers, 10 warnings, 11 info. The plan set will deliver the phase goal if the warnings are addressed; none of them, alone, stops a faithful executor from reaching the goal, but items 1, 3, 5 and 6 each let a faithful executor produce evidence that is weaker than the plans claim.

## Mechanical checks run

- verify.plan-structure on all 16 plans: valid true, no errors or warnings, every task has files, action, verify, done. Task counts 3 (13 plans) or 4 (04-03, 04-08); none above 4.
- Frontmatter files_modified equals the union of the task <files> lists in every plan (script compare, 0 differences). Wave numbers 1,1,2..15 equal max(depends_on)+1; graph is a chain after 04-01/04-02 (file-disjoint); no cycles, no forward reference, no same-wave coupling.
- estimate-check (budget 100,000, confidence low, no calibration): over budget 04-03 (110k), 04-04 (130k), 04-06 (105k), 04-07 (120k), 04-08 (130k), 04-10 (120k), 04-11 (110k), 04-14 (110k); at budget 04-09, 04-15 (100k). Files per plan 17..45 (Phase 3 precedent: 28..57, executed).
- Every stage and selftest name used in a <verify> exists in scripts/check (conformance runs the toy controls, so build/gate/controls.txt is produced by --stage conformance). Result and evidence file names (build/gate/results.txt, evidence/ID.txt, fixtures/NAME.*) match the existing gate.
- Literals recomputed by hand: 91*256+17 = 23313 = 45*512+273 (border 273); P(63) = 118; sprite index 256+32+15 = 303; (2+31+2)*8*4 = 1120, 1365-1120 = 245, +96 = 341, +1365 = 1610; HDS 0/HDW 127/div 4 clip = 341-8 = 333; region payloads 3+1024+45+33+25+65536+512 = 67178, with 7 frame headers 67234, snapshot 25021+67234 = 92255 (equals F-3). All correct.
- Ledger: all 10 amended ids exist; LED-VID-0004..0018, LED-IRQ-0009, -0010 are created in 04-01 before any constants row (04-03 line_mclk, rcr_counter_poweron; 04-04 eight rows; 04-06 three; 04-10 one; 04-11 one; 04-14 one = 16 rows, equals D-168) and any model row uses them. Every TEST: pin names a case that is in that plan's files and is mutual.
- All 47 decisions D-135..D-181 and ER-1..ER-13 are referenced by at least one plan; 12 fixtures, 10 toys, the video-check stage (56 -> 57 stages), the sink, the 7 regions, v3 policy, the cut line, the two E1 passes are all present.
- No DERIVED class, no /private/tmp, /Users or home path in any committed-file instruction; withheld words appear in plan text only inside quotes, as negations, or as the negative-control sentence.

## Requirement and success-criterion coverage

| Requirement | Declared in | Covering tasks | Closes |
|---|---|---|---|
| VID-01 | 01,11,12,13,16 | 04-11 T1-T2, 04-12 T1-T2 | 04-12 |
| VID-02 | 01,14,15,16 | 04-14 T1-T3, 04-15 T1-T3 | 04-15 (after the cut line) |
| VID-03 | 01,06,10,13,16 | 04-06 T1-T3 (registers, VRAM), 04-10 T1-T3 (both DMAs) | 04-10 |
| VID-04 | 01,08,10,13,15,16 | 04-08 T1-T4 (RCR, VBlank, status, IRQ1, guest record), 04-10 (DS, DV), 04-15 (OR, CR) | 04-08 |
| VID-05 | 01,03,04,09,13,16 | 04-04 (palette, divider table), 04-09 (modes, geometry, mid-frame) | 04-09 |
| VID-06 | 01,02,03,04,05,07,09,12,13,14,15,16 | sink 04-02, records 04-04, judge 04-07, chunk/restore 04-05, 04-12 | 04-16 |

| ROADMAP criterion | Plan and check | Status |
|---|---|---|
| 1 BG tiles through BAT, record fields, scroll latch | 04-11 video_bg_basic_solid/asym; 04-12 video_bg_scroll with dev-log join and REL; toy vid-bit-order | covered (see issue 7 for the BXR half) |
| 2 sprite flags in guest RAM; VRAM, SATB DMA, VRAM DMA data moved; VL level and ledger labels | 04-15 spr_limit (status read into guest RAM); 04-06 vdc_reg/vdc_vram; 04-10 vdc_dma_vram, vdc_satb_data; ledger 04-01 | covered |
| 3 interrupt scanline and phase in guest RAM, agree with model for 262 and 263, labelled assumption | 04-08 vdc_irq, vdc_irq_263, vdc_irq_agree(_263), model_param_vdc | covered |
| 4 512 palette entries round trip; mid-frame dot-clock change visible, not normalised | 04-04 vce_pal (512-entry GUEST check); 04-09 video_vce_dot, STRUCT width_set | covered |
| 5 hash is a verdict only beside a literal shown to fail | 04-07 INV_VIDEO_HASH_ONLY, video_hash_not_verdict, fault sweep; toys 04-04/05/08/09/11/12/15 | covered (see info 9) |

## The five orchestration concerns

1. Split and plan size. ISSUE (warnings 8 and 9, info 1). The split follows F-17 and is consistent with D-165 (one bump, no behaviour in 04-03) and D-180. No plan is infeasible by task or file count against Phase 3 precedent, but 04-08 is too large (four distinct technical domains in 44 files, two of them gate-machinery and 6502 timing-loop engineering) and 04-04, 04-07, 04-10 are borderline. 04-01 (45 files) is acceptable: 27 of the files are small ledger TOMLs and mini-root files; the real cost is reading about 28 page images; keep the three tasks as separate commits and append cites rows per batch of pages.
2. Serial 7->8 cascade. Mostly OK, with issues. The cascade list is complete against F-3 and Pitfall 1 (five digest_stream cases, digest_region, snapshot_equals_digest, snapshot_refusal_fields, record_skeleton_256k, reckinds.c, snapkinds.c incl. sk_is_identity pad, digkinds.c DK_CAP/DS_REGIONS_MAX, both goldens, profkinds.c, guestkinds.c, boot.c, ds_snapshot.h in 04-03; dev_log_records, dev_log_windows, mmu_mpr pins in 04-04; ds_obs_begin callers in 04-02). record-lint needs no edit (record-rules.awk pins no version). The accept goldens and digest_stream cases are boots with nothing run, so the BEAM region holds boot values when 04-04 makes the fold live; no second regeneration is needed. The method (old literal checked against shasum, new stream built outside the repo without the new core, hand-written epoch) is in the plan and is process-only. F-1 is covered: 04-03 truth and T4 accept v_togo 512 and refuse 513, accept display_k 511 inside VDW. Issues: warnings 1 and 2, info 2 and 3.
3. Deviations. Mixed. vid-scroll-latch redefinition is recorded (04-12 objective and SUMMARY line) but incomplete (warning 4); INV_TOY_NO_LITERAL narrowing contradicts D-172/D-173/PATTERNS and is recorded only in 04-07 action text (warning 5); VIDEO_TOY_EXEMPT, placeholder index 256, judge=A-B with vdc_irq_ratio, vce-pal-nopal and the 57-stage single function are consistent with the locked decisions and recorded in the plans; VIDEO_TOY_EXEMPT count is required in the 04-13 and 04-16 reports. OK for those five.
4. Requirement and checkbox honesty. OK with info. VID-02 sits only in 04-14, 04-15, 04-16 (and 04-01 as evidence); 04-13 omits it; 04-13 and 04-16 prohibit push, ticking and withheld words. The execute-plan update_requirements step copies frontmatter requirements and is gated by the shared-ID rule (#2388), so no ID is marked before its last declaring plan; see info 4 for two residual points.
5. Literal classes, ledger order, Linux step, paths, record layout. ISSUE on classes only (warning 3). Ledger ids first: OK. gate-gcc step: every C-touching plan has it in its verification line (04-12 conditional by design, 04-13 by its Task 2 six-part run after its C edits). Paths: OK. Record layout: 04-02 (header 32 bytes, count/lost/seq/bytes, records 64..2792, DS_VID_MAX_DOTS 682, sticky full), 04-04 (64-byte DSVR v1 header fields, stream prefix, closed-form count) and 04-07 (STRUCT checks list) agree with D-161..D-163 and F-2.

## Issues

### Warnings (these properties should hold before execution)

1. [restore validation] Every restored field that the fold, DMA closed form or renderer uses as a divisor, multiplier, index or loop bound is judged in pass 1. Plans 04-03 (T1 judge list, T4) and 04-04 (T3 window rules).
   Evidence: dma_word_mclk (u16) is the divisor of the closed form (t - dma_start) / dma_word_mclk in 04-10 T1, and lat_div (u8, 0 at boot until the first fold) is multiplied into dma_word_mclk at transfer start and into rcr_off and vd_off; neither field is in the D-166 list, the 04-03 judge list, or the 04-04 window rules (only dma_state <= 2, dma_done <= 65,536, dma_start <= machine_time while running). A hostile snapshot with dma_state 2 and dma_word_mclk 0, or lat_div 0 with line_idx > 0 followed by a LENR MSB write, reaches a zero divisor. The corpus plan (low-bit flips of an idle-DMA base snapshot, sampled valid mid-DMA stops) cannot produce that state, so nothing would catch it.
   Example fix (non-binding): judge dma_word_mclk in {8,12,16} when dma_state != 0 and lat_div in {0,2,3,4}; add one hostile running-DMA refusal label to snapshot_refusal_vdc; guard the division in the closed form.
2. [task ordering] Plan 04-03 gives one change to two tasks in conflicting order. T1 says dec_powr accepts policy 4 and T2 moves the snapshot_refusal_fields policy-4 refusal to policy 5, both committed before T3, which is where DS_POLICY_SEEDED_V3, ds_policy_of and the profile allowed set are added; T3 also says "snapshot.c line 255 compares against V3". Before T3, policy 4 can neither be named nor bound (the check at snapshot.c:254 is policy > DS_POLICY_SEEDED_V2 then policy != bound), so the T1+T2 green state is ill-defined.
   Example fix (non-binding): put the dec_powr change and the refusal relabel in T3, or define the enum value and the allowed set in T1.
3. [ER-10 class discipline] Each expected-literal row carries the weakest class among its inputs, and the class lint cannot see it (GUEST:<model> is accepted for any model name). Plans 04-07 T1, 04-09 T1/T2/truths, 04-04.
   Evidence: 04-07 T1 labels the struct group "GUEST:closed_form for count and origins" while count = floor((M-1)/1365)+1 and origin = k*1365 use the ASSUMED line_mclk (LED-VID-0004); its dirty group is "GUEST:write_join", a join of the core's own dev-log stamp to the core's own record origins; 04-09 labels active_x and active_w rows "GUEST for arithmetic over programmed values" although D-140 makes the window origin ASSUMED (LED-VID-0005) and "GUEST" in ER-10 means pure in-guest arithmetic over values the guest wrote; 04-04/04-07's div group is MAN:H6-3 but which records precede the CR write depends on the grid and the ASSUMED tie rule, and vce_pal (unlike vce_dot, bg_scroll) has no 250-clock margin guard. Only the palette border row (GUEST:pal_bijection) is a legitimate GUEST row.
   Example fix (non-binding): classify those rows ASSUMED:LED-VID-0004/-0001/-0005, or add the margin guard so a MAN div row is independent of the tie rule; state the rule once in the 04-07 grammar.
4. [toy coverage of the latch rule] The next-line scroll latch for both BXR and BYR is shown live by a renderer mutant, or the gap is stated. Plans 04-11 T1 and 04-12 objective, T3.
   Evidence: 04-12 correctly finds the D-173 text (scroll applied to the current line) unobservable under R1 and redefines the toy as "the BYR write no longer reloads bg_y" (vdc.c). That kills only the BYR rows. 04-11 T1 still says the bxr_line = BXR statement is "the vid-scroll-latch anchor of 04-12" (stale), so the BXR latch, half of ROADMAP criterion 1's "BXR/BYR on the next line", has no mutant; D-173 is not amended anywhere a verifier reads (CONTEXT, VALIDATION).
   Example fix (non-binding): amend D-173 text in the 04-12 SUMMARY and VALIDATION, correct the 04-11 reference, and state whether the BXR row is live only by record faults F1/F8 or add a second anchor.
5. [locked rule weakened] A toy mutating a core file is proven by at least one literal-role case, as D-172/D-173 and PATTERNS line 742 say, or the exception is recorded where a verifier looks. Plan 04-07 T2 (and 04-05 T3).
   Evidence: 04-07 limits INV_TOY_NO_LITERAL to toys whose RULE file is vce.c, vdc.c, beam.c or vid.c, and fold-skipped-at-step-end (cpu.c) lists only chunk_equiv_video_vce_pal and vdc_fold. The plan says such toys "say so in their EXPECT comment" but no plan edits that EXPECT after the rule exists (04-05 creates it, 04-07 does not list it), and the 04-07 output line records only the 57-stage ruling. A skipped step-end fold would drop the trailing lines of a run, so video_vce_pal (closed-form count) is likely red under it and could simply be listed.
   Example fix (non-binding): list video_vce_pal in that EXPECT if observed red, otherwise record the exception in the 04-07 SUMMARY and 04-VALIDATION.
6. [verify cannot fail] 04-02 Task 3 verifies the fold placement with commands that pass at HEAD with no change at all. Evidence: build, core-boundary, conformance, grep of insn_sink_shapes and digest_stream_prefix pass, and mutation-control-anchor are all green before the task; the done criterion "the fold is called at all three points" has no check (the fold-skipped toy and vdc_fold only arrive in 04-05).
   Example fix (non-binding): add a static count of ds_vdc_advance call sites in cpu.c and bus.c (three placements) and fail if absent.
7. [files_modified accuracy] fixtures/lib/video.inc is edited by 04-06, 04-08, 04-10, 04-11, 04-12, 04-14 and 04-15, but each lists only the manifest of the fixture it adds. 04-04 itself notes that editing verdict.inc, check.inc or startup.inc "would change every image hash" and creates video.inc with routines (palette loops, shift-add multiply), as verdict.inc has. Every earlier video fixture that includes it (vce_pal, vdc_reg, vce_dot, ...) then changes bytes, and its manifest sha256 pin (measured on first build) goes red in the fixtures stage; those manifests are in no later plan's files_modified. Confirmed: fixtures/manifest/irq_ctl.toml pins sha256 and size, and scripts/fixture-check reports FIX_HASH_MISMATCH and FIX_SIZE_MISMATCH against the built image.
   Example fix (non-binding): per-feature includes or .ifdef guards so existing images keep their bytes, or list the earlier manifests in each plan that grows video.inc.
8. [scope and sizing] Plan 04-08 is not executable as one plan with margin; four more plans are borderline. 04-08: 44 files, 130k estimate, four tasks spanning (a) core ladder, vertical machine, RCR, flags, IRQ OR (b) two guest fixtures with a spin-counter design, 262/263 variants, about 46 checks (c) two runner kinds (vdc_irq_agree, judge=A-B) (d) gate machinery (conformance-run second EXPECT column, selftest) plus two toys and four ledger pins. 04-04 T1 (20 files: VCE, fold, emitter, runner kind with hand-parsed records and SHA-256, fixture, goldens) and 04-10, 04-07 are the next largest. Eight of 16 plans exceed the 100k smart-zone budget, all with confidence low.
   Example fix (non-binding): split 04-08 after Task 2 (ladder, status, IRQ1, vdc_stat RR/VD, model_param_vdc) and move Tasks 3-4 (vdc_irq, agree, pass column, toys, pins) into a new plan numbered 04-17 placed right after it (waves of 04-09..04-16 shift by one); keep 04-17/04-18 for review fixes otherwise; for 04-04, 04-07, 04-10 state a stop-and-split rule at 60 percent context after Task 1.
9. [locked sizing rule] The 15-file / 10-files-per-task cap of Phase 2 D-81 is exceeded by every Phase 4 plan (tasks naming 14 to 27 files: 04-03 T2, 04-04 T1, 04-07 T2, 04-10 T1/T3), and the supersession exists only in the orchestration brief; no Phase 4 artifact records it (grep D-81 over the phase directory: no match; Phase 3 context said "within D-81" and its plans exceeded the cap unrecorded). An audit or verifier reading D-81 will report sixteen violations.
   Example fix (non-binding): one recorded line in 04-CONTEXT (or each plan's standing rules) stating that D-81's file caps are superseded for Phase 4 by D-177(d) and why.
10. [research resolution] 04-RESEARCH.md "## Open Questions" has four questions with recommendations and no (RESOLVED) heading suffix or inline markers. Substance is resolved: Q1 F-1 ruling (no clamp, 512) in CONTEXT errata and 04-03; Q2 video-check (F-6) in 04-07; Q3 split in 04-03/04/05; Q4 palette 91/17 in 04-04 and every later plan. Because the marker is absent, Dimension 11 reads the section as open; severity kept at warning only because each answer is verifiable in the plans.
   Example fix (non-binding): mark the heading (RESOLVED) with the four rulings inline.

### Info (advisory)

1. 04-01 (45 files, three tasks, 90k) is acceptable as data-only. Cost driver: 28 page-image reads plus 27 ledger files. Commit cites rows per page batch so a context reset loses no read.
2. 04-03 T4 and files_modified list tests/conformance/cases/field_flip.toml, which is the resolved-profile identity case (13 profile fields, kind_field_flip in profkinds.c); it has nothing to do with VDC control fields and needs no change (the snapshot control-field flips are the flip class of snapshot_hostile). An executor who edits its expect will turn it red. The same applies to D-178's mention.
3. Pitfall 1 lists snapshot_hostile in the measured red list, but 04-03 T2's red list omits it (its case file has no counts, so it should stay green when snapkinds.c hand tables are updated in the same task). Note it in the red-list record to avoid a false "missed layout field" stop. The digest literals' independence is process-only (method sentence per case); the verifier should re-derive one literal (digest_stream_prefix) independently once.
4. Frontmatter requirements overstate against "completes none" text in 04-01 (all six), 04-02..04-07, 04-13 (VID-06), and execute-plan copies them into SUMMARY requirements-completed. The shared-ID gate prevents early ticking in the normal flow; in the cut flow 04-13 would tick VID-06 while D-180 also says Phase 4.1 closes the sprite rows of VID-06 (CONTEXT is itself ambiguous). 04-13 and 04-16 forbid REQUIREMENTS.md edits, which conflicts with the automatic update_requirements step; say which wins.
5. 04-08 model_param_vdc says the VD and RR assert stamps move "by +96 master clocks and one line"; handler status-read stamps are quantised by instruction boundaries (tens of clocks), so a tolerance and its class (ASSUMED) must be stated, as vdc_irq does (plus or minus 1.5 iterations).
6. 04-09 placeholder payload (index 256 on every active dot) is consistent with D-157 for burst frames but the BURST flag is only set by 04-10; the intermediate records are internally inconsistent by design. Recorded in the 04-09 objective only; mention in the 04-09 SUMMARY.
7. Fault F0 (perturb the group's own literal) flips any group by construction, so a group killed only by F0 (for example border, dirty) is not shown live by it; the null-renderer controls and the toys carry the liveness, not F0. Consider requiring one non-F0 kill per group, or document it in the video-check rules.
8. Check 8b latency: per-task commands run --stage conformance (33 s now, 47 s central after Phase 4, 60 s high) and often --stage sanitizers; VALIDATION states under 60 s per task. House practice, recorded.
9. ROADMAP criterion 5 "renderer deliberately broken": spr_basic, bg_enab, vce_dot (partly) and the golden scenes are shown live by record-level faults F1/F2/F7 and exemptions, not by a renderer mutant; the toy cap of 10 is spent by 04-15. This follows D-172/D-173 as locked; if the owner reads criterion 5 literally, one sprite renderer mutant would be needed.
10. Check 8f: no {FAILING_DIRECTIONS} probe was supplied, so the check is silent; no plan carries any <fails_when> (Phase 3 plans do not either); a probe run would flag every <automated>. 04-VALIDATION.md per-task map is still the single 04-TBD placeholder and is filled only in 04-13/04-16.
11. If the cut is taken at 04-13, README text must not present LED-VID-0008 (status layout incl. OR and CR) as supported beyond RR, VD, DS, DV flags actually raised; the plans leave the scope to ER-2 judgement in T3.

## Dimension summary

| Dimension | Result |
|---|---|
| 1 Requirement coverage | pass (all six IDs in frontmatter and tasks; all five criteria mapped) |
| 2 Task completeness | pass (structure valid); warning 6 on one verify |
| 3 / 3b Dependencies, coupling | pass (chain, correct waves; no undeclared same-wave coupling) |
| 4 Key links | pass |
| 5 Scope sanity | warnings 8, 9 |
| 6 Verification derivation | pass (plumbing plans have implementation-level truths, acceptable) |
| 7 Context compliance, 7b scope reduction | pass on D-135..D-181 coverage and no v1/stub language; warnings 3, 4, 5 on rule fidelity |
| 7c Tier compliance | pass (core / runner / fixtures / gate as in the responsibility map) |
| 8 Nyquist | automated verify on every task; warning 6; info 8, 10 |
| 9 Cross-plan data contracts | pass |
| 10 CLAUDE.md / AGENTS.md | pass (no dependency, no push, evidence honesty, rights); Co-Authored-By trailer present |
| 11 Research resolution | warning 10 |
| 12 Pattern compliance | pass (read_first cites P and F numbers per task) |
| Verify command format | pass (no swallowed-error patterns; greps are regression guards beside new-case greps) |

## Recommendation

Return to the planner with warnings 1-10. Items 1, 2, 6, 7 and 10 are small plan edits; 3, 4, 5 are wording and rule-fidelity edits in 04-07, 04-09, 04-11 and 04-12; 8 and 9 need a decision (split 04-08, record the D-81 supersession). No blocker: the goal-critical paths (palette round trip, per-line records with mid-frame change, guest-recorded interrupt positions, data-moved DMA checks, hash-beside-literal with toys) are all planned with literal sources that are MAN, GUEST or ASSUMED.
