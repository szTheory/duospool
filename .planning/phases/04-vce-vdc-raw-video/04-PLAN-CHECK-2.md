# Phase 4 plan check 2 (gsd-plan-checker, round 2 of 3)

Phase: 04-vce-vdc-raw-video. Plans checked: 04-01 .. 04-17 (17 files, uncommitted). Inputs: 04-CONTEXT (D-135..D-181, ER-1..ER-13, errata, reconciliation notes 5 and 6), 04-RESEARCH, 04-PATTERNS, 04-VALIDATION, ROADMAP Phase 4, REQUIREMENTS VID-01..06, AGENTS.md, 04-PLAN-CHECK-1.
Result: ISSUES FOUND. 0 blockers, 9 warnings, 9 info. All 10 round-1 warnings and all 11 round-1 info items are resolved or carried as advisory (table below). The revision introduced or exposed nine new warnings, none of which stops a faithful executor from reaching the phase goal; items 1 to 4 are the ones that would make the executor deviate or make a stated truth unproven.

## Mechanical checks run

- verify.plan-structure on all 17 plans: valid true, errors [], warnings [] each; task counts 3 (13 plans), 4 (04-03), 2 (04-08, 04-17); the key requirements_advanced is accepted and listed in frontmatter_fields (see info 1).
- phase-plan-index: waves 1:[01,02] 2:[03] 3:[04] 4:[05] 5:[06] 6:[07] 7:[08] 8:[17] 9:[09] 10:[10] 11:[11] 12:[12] 13:[13] 14:[14] 15:[15] 16:[16]; every wave equals max(depends_on)+1; no cycle, no forward reference, no missing reference; no same-wave pair except 04-01/04-02, which share no file (script compare over all 17 file lists).
- files_modified equals the union of the task <files> lists in every plan (awk compare, 0 differences, 17 of 17).
- Cross-plan file sharing (all sequential by depends_on): the six per-feature includes vce.inc (04-04), vdc.inc (04-06), vdma.inc (04-10), bg.inc (04-11), spr.inc (04-14), spin.inc (04-17) are each listed by exactly one plan; hucard.cfg only by 04-04; check.inc, startup.inc, verdict.inc by none. mmu_mpr.s/.checks/three manifests are edited by 04-04 and 04-06 and re-pinned in both; vdc_stat.s/.checks/manifest by 04-08, 04-10, 04-15 and re-pinned in each; vce_pal.s by 04-04 and 04-13 (.ifndef NOPAL, byte-neutral, plan says verify the pin). No later plan edits an earlier include, so no earlier fixture manifest pin goes red by an include edit.
- Numbering map paragraph is byte-identical in all 17 plans (md5); standing rules (Sizing, Requirements, ER-10) identical apart from the golden rule in the plans that change records. Spare numbers 04-18, 04-19. CONTEXT-to-file map checked against each plan's "CONTEXT plan 04-NN" text: consistent.
- Every <automated> stage and selftest name used exists in scripts/check, except video-check, mutation-video-hash-only, mutation-video-group-vacuous, mutation-vexp-class-lint, which are created by 04-07 Task 2 and used only from 04-07 Task 2 on. Only one <automated> block is multi-line (04-02 Task 3, see warning 7).
- estimate-check (budget 100,000, confidence low, no calibration): over budget 04-03 (110k), 04-04 (130k), 04-06 (105k), 04-07 (120k), 04-10 (120k), 04-11 (110k), 04-14 (110k); at budget 04-09, 04-15; under 04-01 90k, 04-02 85k, 04-05 95k, 04-08 75k, 04-12 95k, 04-13 90k, 04-16 80k, 04-17 85k. Round 1 had eight over budget; the 04-08 split removed one.
- All 47 decisions D-135..D-181 and ER-1..ER-13 are referenced by a plan (ER-5 only by substance: E0/E2 labels, no irq_stim in video cases). No deferred idea is implemented (no reference renderer, no video.dsvr run-record artifact: --video-out is D-162, locked). No scope-reduction wording (v1/static/stub/placeholder) in task actions.
- The ds_run_request locals in tests/conformance (about 30 declarations in 13 files) are all memset to zero (checked by awk), so appending the vid member in 04-02 does not need edits outside the plan's file list.
- Opcode table check for warning 3: tests/oracle/huc6280-opcodes.tsv gives CLC 2 cycles, ADC zp 4, ADC imm 2, ADC abs 5.

## Round 1 item status

| # | Round-1 item | Status | Evidence |
|---|---|---|---|
| W1 | restore divisors dma_word_mclk and lat_div judged | RESOLVED | 04-03 T1 lines 145-148 (dma_word_mclk in {8,12,16} while dma_state != 0; lat_div in {0,2,3,4}, 0 only when line_idx = 0); T4 line 219 labels dma-word-0, lat-div-5, lat-div-0-late; T-04-08 line 248; 04-10 T1 lines 139-142 (static assert plus runtime guard leaving the transfer pending while lat_div is 0) |
| W2 | 04-03 dec_powr/refusal ordering | RESOLVED | T1 line 148 (dec_powr not changed in T1+T2), T2 line 175 (snapshot_refusal_fields stays green because policy 4 is still refused), T3 line 201 (V3, ds_policy_of, allowed set, dec_powr and the policy-5 relabel in one commit); T3 files list line 186 carries snapshot.c, snapkinds.c, snapshot_refusal_fields.toml |
| W3 | ER-10 classes | RESOLVED, one residual (new warning 5) | rule stated once 04-07 lines 136-137; closed GUEST allowlist (pal_bijection) with lint tagged rule-vexp-class-lint and class-bad mini-root, 04-07 lines 175, 182; groups reclassified 04-07 lines 151-153, 04-09 lines 124-125, 142-145; margin guard in videokinds.c 04-12 line 123; VALIDATION A6. Only pal_bijection is used as a GUEST model in any .vexp plan text |
| W4 | next-line latch toy coverage | RESOLVED | 04-11 line 125 (bxr_line is not a toy anchor), 04-12 truths lines 53-54 and objective 78-79, T3 line 164 updates VALIDATION; VALIDATION A3 (line 61) and A4 (line 62). D-173 text in CONTEXT itself is not edited; VALIDATION states the plans are authoritative, which is where a verifier looks |
| W5 | INV_TOY_NO_LITERAL scope | RESOLVED | VALIDATION A5 line 63; 04-05 T3 line 170 (list video_vce_pal only if observed red, else EXPECT comment plus VALIDATION row); 04-07 T2 lines 185-186 |
| W6 | 04-02 T3 verify cannot fail | RESOLVED, one new defect (warning 7) | 04-02 lines 172-176: static count of ds_vdc_advance(inst call lines in cpu.c and bus.c (at HEAD both 0), the variable name inst exists in both files |
| W7 | video.inc edited by seven plans | RESOLVED, one stale mention (warning 8) | per-feature macro-only includes, each owned by one plan (04-04:193-194, 04-06:164, 04-10:145, 04-11:133, 04-14:122, 04-17:133), rule in every plan's standing rules (e.g. 04-01:129-130), VALIDATION A8 line 66 |
| W8 | 04-08 too large; borderline plans | RESOLVED for 04-08, advisory for the rest | 04-08 now 2 tasks, 13 files, 75k; 04-17 2 tasks, 28 files, 85k; stop_rule blocks in 04-04:107-109, 04-07:95-97, 04-10:92-94; CONTEXT item 6 line 459. Seven plans remain over the smart-zone budget (warning 9) |
| W9 | D-81 supersession unrecorded | RESOLVED | 04-CONTEXT.md reconciliations item 6 (lines 458-459) and the Sizing line of every plan |
| W10 | Open Questions not marked | RESOLVED | 04-RESEARCH.md lines 662-668: heading (RESOLVED) and RESOLVED marker on each of the four questions |
| I1 | 04-01 cost, commit per page batch | RESOLVED | 04-01 T1 line 148 |
| I2 | field_flip.toml listed in 04-03 | RESOLVED | not in 04-03 files_modified; T4 line 221 says it is not touched |
| I3 | snapshot_hostile in red list; verifier re-derives one digest | RESOLVED for the list; the verifier re-derivation stays a process item | 04-03 T2 line 175 |
| I4 | requirements frontmatter vs "completes none" | PARTLY, see info 1 | requirements_advanced key and the Requirements standing rule in every plan; 04-13:60, 04-16:59; VALIDATION A9 |
| I5 | model_param_vdc tolerance and class | RESOLVED for the tolerance (04-08:37, 128, 133-134); the +1365 literal has a sign error (warning 1) | |
| I6 | 04-09 BURST inconsistency | RESOLVED | 04-09 line 203 output note |
| I7 | F0 liveness | RESOLVED | 04-07 line 138; VALIDATION A7 |
| I8 | check 8b latency | CARRIED (house practice) | per-task commands still run --stage conformance |
| I9 | criterion 5 literal reading | CARRIED to the owner note | 04-13:159-160, 04-16:153 |
| I10 | no fails_when, VALIDATION map placeholder | CARRIED | no {FAILING_DIRECTIONS} or {VERIFY_PATHS} probe was supplied, checks 8f and the path probe are silent; the per-task map is filled in 04-13 (plans 04-01..04-12 and 04-17) and 04-16 |
| I11 | README scope if the cut is taken | RESOLVED | 04-13 lines 156-157, 04-16 line 135 |

## Requirement and criterion coverage (unchanged by the revision)

| Requirement | Declared in (requirements) | Completed (not in requirements_advanced) | Status |
|---|---|---|---|
| VID-01 | 01, 11, 12, 13, 16 | 04-12 | covered |
| VID-02 | 01, 14, 15, 16 | 04-15 | covered (sprites move to 4.1 if the cut is taken) |
| VID-03 | 01, 06, 10, 13, 16 | 04-10 | covered |
| VID-04 | 01, 08, 10, 13, 15, 16, 17 | 04-17 | covered |
| VID-05 | 01, 03, 04, 09, 13, 16 | 04-09 | covered |
| VID-06 | 01-07, 09, 12, 13, 14, 15, 16 | 04-16 | covered |

Criteria: 1 -> 04-11, 04-12; 2 -> 04-06, 04-10, 04-14, 04-15; 3 -> 04-08, 04-17 (262 and 263); 4 -> 04-04, 04-09; 5 -> 04-07 mechanism plus the ten toys (palette-mask 04-04, emit-mutates and fold-skipped 04-05, vrr-no-readahead 04-06, rcr-base and status-read-keeps-flags 04-17, dot-div 04-09, bit-order 04-11, scroll-latch 04-12, spr-limit 04-15 = 10, the D-173 cap).

## Literals recomputed (throwaway awk in the scratch directory, outside the repo)

- P(i) = (91*i + 17) & 511: bijection over 0..511 (0 duplicates); P(63) = 118; P(256) = 273; 91*256 + 17 = 23313 = 45*512 + 273. Plans correct.
- Gap arithmetic: (HDS+HDW+2)*8*div = 35*32 = 1120; 1365 - 1120 = 245; hdw_start adds (HDS+1)*8*4 = 96 giving 341; line_before adds one line giving 1365 + 245 = 1610. 04-17 correct. The vertical setup VPR $0F02, VDW 239, VCR 4 sums 17 + 240 + 4 + 3 = 264, at least 262 and 263 (D-139). Plans correct.
- 512 and 513: the largest vertical state length is VDW + 1 = 512 (VDW 9 bits); VDS + 2 = 257, VCR 255, VSW + 1 = 32; v_togo <= 512 with no clamp, 513 refused, display_k <= 511. 04-03 correct.
- Sizes: region payloads 3 + 1024 + 45 + 33 + 25 + 65536 + 512 = 67178; with seven 8-byte frame headers 67234; snapshot 25021 + 67234 = 92255; BEAM 33, VDC0 45, VDMA 25 member sums verified; DS_VID_MAX_DOTS 682, rec_bytes 64 + 2728 = 2792; 262 * 1365 = 357,630, 263 * 1365 = 358,995; clip 0/127/div 4 = 341 - 8 = 333; 04-02 sink case 64 + 2792 = 2856 stored, next 64 gives 2920 > 2900 (sticky full, lost 2). All correct.
- Found wrong or unsupported: the sign of the RR stamp shift (warning 1) and the spin-loop cost (warning 3).

## Issues

### Warnings (these properties should hold before execution)

1. [literal_independence / sign] Plan 04-08 (truth line 37, T2 behavior line 128, T2 action lines 133-134).
   Required property: every expected delta in model_param_vdc has the direction that D-142 and the plan set's own arithmetic give.
   Evidence: D-142 sets rcr_counter to 64 on the origin line; with rcr_origin = line_before the origin is the last VDS line, so the RR for an unchanged RCR value fires one line EARLIER, a delta of -1365 master clocks on the RR status-read stamp. 04-17 truth line 50 already relies on this direction (the RCR-to-VD gap grows to 245 + 1365 = 1610). 04-08 says the RR stamp "moves by +1365". The VD delta +96 is correct (hdw_start is later). An executor that implements the literal as written gets a red case and, to turn it green, may read the sign off a run.
   Example fix (non-binding): state -1365 (or "earlier by 1365") in the truth, the behavior list and the action; keep the tolerance of 21 clocks.

2. [files_modified / pin mutuality] Plan 04-17 Task 2 (files line 147-148, action line 164).
   Required property: every ledger pin set by a task has its case file in the plan's file set so INV_PIN_BACKLINK holds in both directions at the plan-end gate.
   Evidence: LED-IRQ-0009 TEST:vdc_stat and LED-VID-0008 TEST:vdc_stat need tests/conformance/cases/vdc_stat.toml to name both ids in pins; that file is in 04-08, 04-10, 04-15 but not in 04-17's files_modified, and 04-08 cannot pre-add the pins (the reverse backlink check, scripts/inventory-check line 175, would be red while the ledger still says PENDING:4). The pins of LED-IRQ-0001 (vdc_irq.toml) and LED-IRQ-0010 (vdc_irq_agree.toml) name Task 1 files from a Task 2 edit; both sides must land in one commit. Round 1 confirmed this property for the 16-plan set; 04-17 breaks it.
   Example fix (non-binding): add vdc_stat.toml to 04-17 files_modified and say the pins of both sides go in the same Task 2 commit.

3. [feasibility of the fixed-cost loop] Plan 04-17 Task 1 (action lines 133-135, truths lines 49-50, key_link "iterations * 18").
   Required property: the per-iteration cost used as divisor is derived from the fixture's instruction sequence and the ASSUMED tolerance windows separate the hypotheses they claim to separate.
   Evidence: the plan states "24-bit add through clc/adc, 6 cycles = 18 master clocks". With the repo's own table (CLC 2, ADC zp 4, ADC imm 2) clc plus one adc is 6 cycles but adds 8 bits; a 24-bit counter without a data-dependent branch (prohibited) needs three load/add/store sequences, about 30 cycles or more (90 clocks). The ±1.5-iteration windows around 245 and 341 (96 clocks apart) separate only if one iteration is below 32 clocks; at 36 clocks (D-174's own bound of 12 cycles) they already overlap. So the number 18 in the key_link, in check (4) and in vdc_irq_agree is not achievable as described, and the check loses its stated discriminating power. The literal is derived by a computation, not a run, so independence is not violated, but the fixture cannot be written to the plan.
   Example fix (non-binding): state the loop's exact instruction sequence and cycle sum from the H8-25 table, derive the divisor and the tolerance from it, and either keep the counter width within what that cost allows or weaken the stated separation claim to what the real cost supports; the check (1) ratio tolerance is 8 iterations, not 1 (see info 6).

4. [coverage of a locked check] Plan 04-17 Task 1 (truth line 50, behavior line 126-127) against D-174 check (4).
   Required property: every truth a plan states is exercised by at least one case.
   Evidence: truth 3 asserts the RCR-to-VD gap of 245, 341 and 1610 clocks; the plan's cases (vdc_irq, vdc_irq_263, vdc_irq_agree, vdc_irq_agree_263, vdc_irq_ratio) all run the default binding (vd_irq_phase line_start, rcr_origin first_active), so only 245 can be measured in guest RAM. 341 and 1610 are exercised only by 04-08 model_param_vdc, which judges dev-log stamps, not guest-recorded values. D-174 (4) says the check separates the vd_irq_phase and rcr_origin values. Read strictly this is a partial delivery of a locked decision.
   Example fix (non-binding): add bound variants of the vdc_irq judgement (bind.vd_irq_phase and bind.rcr_origin in the request, same ROM, other expected window), or restate truth 3 and record in 04-VALIDATION that the alternatives are verified through model_param_vdc only.

5. [ER-10 class discipline] Plans 04-04 (truth line 63, T1 lines 166-167, T2 vce_pal.s) and 04-07 (T1 line 152 div group).
   Required property: a row classed MAN depends on no assumed constant (ER-10, weakest class wins).
   Evidence: the div group claims MAN:H6-3 for "div 4 on every record ending at least one whole line before the CR $01 write". vce_pal.s writes only CR $01 (04-04 line 169), so div 4 before the write rests on the power-on CR of 0, which is DuoSpool's own assumption (LED-PROF-0003, D-167), not the page. Also no step makes a "before" interval exist: records ending a whole line before the write need a write stamp of at least 2,730 clocks, and the plan does not give the fixture a counted delay, so the "div 4 then 3" literal and kill F5 may be vacuous on the before half.
   Example fix (non-binding): have vce_pal.s write CR $00 explicitly after a counted delay of more than two lines and judge div 4 only after that write, or class the pre-write half ASSUMED:LED-PROF-0003.

6. [stale reference that changes a locked trigger] Plan 04-13 Task 2 (line 138).
   Required property: the D-180 cut trigger "more than two plans needed a revision plan" counts only revision plans.
   Evidence: the record to be written says "count of revision plans used (04-17, 04-18)". 04-17 is now a planned split plan (04-08's second half), and the spare numbers are 04-18 and 04-19 (every plan's numbering rule). An executor that counts existing 04-17 as a revision plan starts the count at one and the trigger fires after two more instead of three.
   Example fix (non-binding): "(04-18, 04-19, and any further spare number)".

7. [verify not runnable as written] Plan 04-02 Task 3 (<automated>, lines 172-173).
   Required property: every <automated> command runs verbatim.
   Evidence: the command is wrapped across two lines inside the element: line 172 ends with grep -q '^RESULT insn_sink_shapes pass' and line 173 starts with build/gate/results.txt. As a shell script the first grep reads stdin and the second line executes build/gate/results.txt. It fails closed (never passes by accident); the executor must repair it by judgement. It is the only multi-line automated block of the 17 plans.
   Example fix (non-binding): put the command on one line.

8. [contract document contradicts the plans] 04-VALIDATION.md lines 76-78 (Wave 0).
   Required property: the validation contract's Wave 0 list names artifacts the plans create.
   Evidence: line 78 still lists "RODATA segment in hucard.cfg and fixtures/lib/video.inc (04-03)"; video.inc no longer exists (ruling 7, row A8 of the same file) and RODATA is created in file 04-04; line 77 says "video-check skeleton (04-05)" while video-check is created in file 04-07 (it is the CONTEXT numbering). Plans 04-13 and 04-16 tick these items and 04-16 sets wave_0_complete true; an item that can never be literally satisfied forces a judgement or a stray video.inc.
   Example fix (non-binding): rewrite the two items against the file numbers and the per-feature includes.

9. [scope sanity, carried from round 1 item 8] Plans 04-03 (110k), 04-04 (130k), 04-06 (105k), 04-07 (120k), 04-10 (120k), 04-11 (110k), 04-14 (110k) exceed the smart-zone budget (confidence low, uncalibrated; advisory by ADR-2629, never a blocker). Stop-and-split rules exist only for 04-04, 04-07, 04-10.
   Required property: a plan that nears its context budget has a stated stop point.
   Example fix (non-binding): the same stop_rule (after Task 1, at about 60 percent) on 04-03, 04-06, 04-11, 04-14, or accept the exposure and record it.

### Info (advisory)

1. [requirements_advanced] Frontmatter key requirements_advanced is accepted by verify.plan-structure (valid true, listed in frontmatter_fields) but no gsd-core tooling reads it (grep over bin, workflows, references, templates: no reader). Tick behaviour is therefore decided by the requirements key alone: execute-plan update_requirements runs requirements.ready-ids, which blocks an id while any sibling plan declaring it lacks a SUMMARY. With this set every VID id is declared by 04-16, so all six tick at 04-16 (the intended outcome; no early ticking at 04-17 or 04-10). The plans' sentence that 04-13 "ticks none because every requirement is listed in requirements_advanced" is right for the wrong reason (it ticks none because 04-14..04-16 are still siblings). Two residuals: (a) execute-plan.md line 420 and the summary template say requirements-completed MUST copy the requirements array verbatim; the plan rule (advanced ids go elsewhere) contradicts that line, and an executor following the template would claim all six ids completed in 04-01's SUMMARY, which audit-milestone reads; say in the standing rule that it overrides the template. (b) In the cut flow the standard step at 04-13 blocks all five ids (04-14..04-16 still exist when it runs); after the orchestrator moves them to Phase 4.1 nothing ticks VID-01, 03, 04, 05; the SUMMARY honesty table goes to the orchestrator, who should run requirements mark-complete for those four.
2. [video-check mini-roots] 04-07 Task 2 does not say where video-check finds cites.txt and ledger/entries when run with --root on a mini-root that carries only t.vexp (fixture-lint falls back to the repository when the root has none, scripts/fixture-lint lines 56-60); say the same fallback (and, for class-bad, a mini-root registry if needed).
3. [video-check and the pass column] 04-07 builds the toy cross-rules before 04-17 adds the second EXPECT column; a pass-column id (video_bg_basic_solid in 04-11) is not a kill and should not count as toy coverage or as the literal-role case of INV_TOY_NO_LITERAL; 04-17 does not edit video-check. Also 04-07 T3 adds the golden scene video_vce_pal_golden, which no toy names, and its Task 3 file list omits VIDEO_TOY_EXEMPT (it is in the plan's list): the exemption line belongs in T3, as 04-11 T3 and 04-14 T3 do for their golden scenes.
4. [phantom golden] 04-10 Task 2 regenerates "the vce_pal and vce_dot golden rows" (files include fixtures/src/vce_dot.vexp), but 04-09 creates no vce_dot golden scene or case (only vce_pal has video_vce_pal_golden).
5. [literal by measurement] 04-03 T2 says SK_EXPECTED_SIZE is "the measured size". The hand sum is 25,021 + 67,178 + 7*8 = 92,255 (verified above); say to derive it and compare, so the size literal is not read off the build.
6. [check (1) tolerance] 04-17 check (1) (RCR a + 8 follows RCR a by 8 times the a-to-a+1 interval "within one iteration" in D-174): 1365 over the iteration cost is not an integer, so the single interval is quantised and its 8-fold multiple carries an error of up to 8 iterations; the tolerance must be stated as 8 iterations (or the baselines made long). Left to the executor, but the plan should not inherit "within one".
7. [spare numbers] three stop_rule plans but two spare numbers (04-18, 04-19); the rule already says "next spare number".
8. [carried] Checks 8b, 8f and the path probe as in round 1 item list (I8, I10); verifier should re-derive digest_stream_prefix independently once (I3).
9. [ER-5] not named in any plan; its substance (E2 label, irq_stim absent in video cases) is present in 04-17 truth 4, 04-13 T3 and the video kind.

### Structured issues

```yaml
issues:
  - {plan: "04-08", dimension: literal_independence, severity: warning, required_property: "Every expected delta in model_param_vdc has the direction D-142 and 04-17's 1610 arithmetic give", description: "RR stamp for rcr_origin=line_before is 1365 EARLIER; plan says +1365 (lines 37, 128, 133-134)", fix_hint: "state -1365"}
  - {plan: "04-17", dimension: key_links_planned, severity: warning, required_property: "Every ledger pin set by a task has its case file in the plan file set; both pin sides land together", description: "LED-IRQ-0009 and LED-VID-0008 pin vdc_stat, whose case file is not in 04-17 files_modified; INV_PIN_BACKLINK reverse check fails if added early in 04-08", fix_hint: "add vdc_stat.toml; same-commit rule"}
  - {plan: "04-17", task: 1, dimension: verification_derivation, severity: warning, required_property: "Divisor and tolerance derive from the fixture's real instruction sequence and separate the hypotheses claimed", description: "24-bit add in 6 cycles is not achievable (CLC 2, ADC zp 4, ADC imm 2); windows for 245 and 341 overlap above 31 clocks per iteration", fix_hint: "derive P from the H8-25 table, restate separation claim"}
  - {plan: "04-17", task: 1, dimension: requirement_coverage, severity: warning, required_property: "Every stated truth is exercised by a case", description: "341 and 1610 of truth 3 and D-174 check 4 have no guest-RAM case; only default binding is run", fix_hint: "bound variants or record the limit"}
  - {plan: "04-04", dimension: context_compliance, severity: warning, required_property: "A MAN row depends on no assumed constant (ER-10)", description: "div 4 before the CR $01 write rests on power-on CR 0 (LED-PROF-0003) and no delay guarantees a before interval", fix_hint: "write CR $00 after a delay or class the half ASSUMED"}
  - {plan: "04-13", task: 2, dimension: context_compliance, severity: warning, required_property: "D-180 trigger counts only revision plans", description: "line 138 names 04-17 as a revision plan; 04-17 is a planned split", fix_hint: "04-18, 04-19"}
  - {plan: "04-02", task: 3, dimension: task_completeness, severity: warning, required_property: "Every automated command runs verbatim", description: "command split over lines 172-173", fix_hint: "one line"}
  - {plan: null, dimension: key_links_planned, severity: warning, required_property: "Wave 0 list names artifacts the plans create", description: "04-VALIDATION.md line 78 video.inc and CONTEXT-numbered plan ids", fix_hint: "rewrite against file numbers and per-feature includes"}
  - {plan: "04-03/04/06/07/10/11/14", dimension: scope_sanity, severity: warning, required_property: "A plan near its context budget has a stated stop point", description: "seven plans over 100k; stop_rule on three", fix_hint: "extend or accept"}
```

## Dimension summary

| Dimension | Result |
|---|---|
| 1 Requirement coverage | pass |
| 2 Task completeness | pass (structure valid); warning 7 |
| 3 / 3b Dependencies, coupling | pass (graph and waves verified by tool and script, 04-17 correctly after 04-08 and before 04-09; no stale 04-08/04-09 reference except warning 6) |
| 4 Key links | pass; warning 2 on pins |
| 5 Scope sanity | warning 9 (carried) |
| 6 Verification derivation | pass; warning 3 |
| 7 / 7b Context, scope reduction | pass on coverage and no reduction wording; warnings 4 (check 4 of D-174), 5 |
| 7c Tier compliance | pass |
| 8 Nyquist | automated verify on every task; warning 7; info carried |
| 9 Data contracts | pass |
| 10 AGENTS.md / CLAUDE.md | pass (no push, no dependency, rights, Co-Authored-By) |
| 11 Research resolution | pass |
| 12 Pattern compliance | pass |
| Verify format / path | pass (no swallowed-error patterns; stage names resolve) |

## Recommendation

Return to the planner for warnings 1 to 8 (all are one-line or one-paragraph edits in 04-04, 04-08, 04-13, 04-17, 04-02 and 04-VALIDATION; warning 3 and 4 need a short design statement for the spin loop and the variants), decide on 9. No blocker: the goal-critical paths remain planned with literals from MAN, GUEST or ASSUMED sources. Round 3 would check these eight edits only.
