# Phase 5 plan check 2 (gsd-plan-checker, round 2 of 3)

Phase: 05-psg-raw-audio. Plans checked: 05-01 .. 05-10 at 2f7dfbe (revision 80e7c66 plus the ROADMAP line 2f7dfbe), the draft 05-VALIDATION.md and the revised 05-RESEARCH "Open Questions" heading. Inputs: 05-PLAN-CHECK-1.md, 05-CONTEXT (D-182..D-229, errata win), 05-RESEARCH section 1 (register
table), sections 2, 3, 6, RF-11, 05-PATTERNS, ROADMAP Phase 5, REQUIREMENTS AUD-01..03, AGENTS.md, .claude/CLAUDE.md, and the repository at HEAD (read-only measurements).

Result: ISSUES FOUND, verdict REVISE. 4 blockers, 3 warnings, 7 info. All nine round-1 blockers are fixed in the plan text; all eight warnings are addressed, warning 1 only in part. Two of the fixes introduce new defects: the 05-06 mask probe and the re-derived twin set. Two blockers are new
findings. One was missed in round 1: the psg_tone.aexp byte budget against D-207. The other is a vacuous gate in 05-10.

## Mechanical checks run

- verify.plan-structure on 10 plans: exit 0, valid true, errors [], warnings []; 29 tasks (3 in nine plans, 2 in 05-02), each with files, action, verify and done.
- files_modified equals the union of task <files> in 10 of 10 plans (script, 0 differences; 05-04 now 41 files with pcm_chan_append.toml and aud_poweron_policy_diff.toml in both lists).
- Waves unchanged: 1:[01,02] 2:[03] 3:[04] 4:[05] 5:[06] 6:[07] 7:[08] 8:[09] 9:[10]; every wave = max(depends_on) + 1; no cycle. The only same-wave pair, 05-01/05-02, has no files_modified overlap, so (d) passes.
- (a) D-228/D-229 everywhere: no "policy 6", "policy-7", "= 6" for v4, "D-182..D-227" or "seq 182 227" remains; the handbook loop is `seq 182 229`; ten of ten stop_rule blocks and standing rules say about 80 percent with "D-229 supersedes D-216"; no bare 60 percent remains.
- 05-03 Task 3 gates against HEAD: `DS_POLICY_SEEDED_V4 = 5` absent today, `policy-6=field` absent, and `policy-5=field` present in tests/conformance/cases/snapshot_refusal_fields.toml, so the negated grep fails at HEAD and measures the move. snapkinds.c:1347 ed_policy5 and :1820
  are where the plan says. dec_powr refuses 0 because DS_POLICY_NONE = 0 (ds_poweron.h:21), so "accepts 1..5, refuses 0 and > 5" is right. mmu_mpr_256k, _384k and _1m exist.
- (e) every plan keeps the plan-complete-only tick rule with the git diff REQUIREMENTS revert (10 of 10), the state.json rule (10 of 10), the no-home-or-temp-path rule (10 of 10) and the Sonnet trailer line. 0 non-ASCII bytes and 0 lines of 400 or more characters in the ten plans
  and in 05-VALIDATION.md.
- estimate-check --calibrated (budget 100,000, confidence low, no calibration): same figures as round 1 (05-04 1.6x, 05-03 1.5x, 05-05 and 05-07 1.3x, 05-06 1.2x, 05-09 1.15x). Neither 05-04 nor 05-06 updated its estimate after it grew (warning 3).
- 05-10 Task 3 gate run against the draft: `grep -q 'nyquist_compliant: true'` already matches line 133 of the draft ("- [ ] `nyquist_compliant: true` set only by plan 05-10"), so that gate cannot fail (blocker 4).
- Phase 4 .vexp sizes measured as calibration for the .aexp cap: 6,971 to 16,223 bytes for 19 to 57 non-comment rows; spr_basic.vexp is 16,223 of 16,384 bytes at 57 rows (blocker 3).

## Round-1 findings: fixed or not (read in the plan text)

- B1 D-228: fixed (05-01 PR-9, LED-PROF-0002 note, handbook table "policies 4 -> 5"; 05-03 truth, Task 3 action and verify; ROADMAP line in 2f7dfbe).
- B2 aud_poweron_policy_diff: fixed in 05-04 Task 2 (select 3 keyed on with no upload; v4 s1 and s2 must differ, zero and v4 s1 must differ, zero and v3 s1 identical; it drives the identity check to report a difference). Files, REQUIRED, verify and VALIDATION row AUD-03 c are present.
- B3 silent_pre: fixed. With R4 $1F, channel 0 sits at S = 0 with chON 0, so the chON-gate mutant reads wave[0] = $10 at a frozen counter (widx 0 after 32 TIN words) and turns the row red. Row R4 $1F -> (0, $00, 0, $001F) checks: DDA 1 -> 0 with chON 0 is no restart, post $1F & $DF.
- B4 pan phase C: the scene is fixed (C1 left S = 0+1+0 = 1, right S = 0+1+15 = 16; C2 mirrored; the silent rows sit in single-channel sub-phases). The twin re-derivation is wrong (blocker 2).
- B5 psg_mod: fixed (upload, F = 256, R1, R5, then R7 $98 or $91, then key-on; the window is anchored on the key-on, after the old-period reload).
- B6 lfo_dynamic: fixed. ER-10 holds (ruling below). The window bounds are still unstated (warning 1).
- B7 mmu_mpr_256k: fixed. B8 D-229: fixed in all ten plans and the handbook section (12). B9 05-VALIDATION.md: the draft exists and covers every criterion and requirement. One of its closing gates is vacuous (blocker 4).
- W1 is partly fixed: D-229 reserves 05-11/05-12 and sends a sixth split to the orchestrator, but the estimates are stale. W2 pcm_chan_append is fixed (T0 0, 1, 191, 192, 193 -> 0, 1, 1, 1, 2; ds_obs_begin takes machine_time as a parameter, so direct calls can set T0). W3 is fixed (the golden is
  regenerated in every 05-06 task). W4 is fixed for P8-10 and P8-2, but a gap remains for P8-6 (blocker 1). W5 is fixed (phases 9a/9b: R1 $EF gives left att 2 against 0). W6 has rows, but they have defects (blocker 1). W7 and W8 are fixed. Info 4, 6 and 8 are applied.

## ER-10 ruling on 05-09 lfo_dynamic (asked): it holds

Every input of the verdict is MAN. F0' = 192 + 4096 + 24 - 16 = 4,296 & 4,095 = 200 and 192 + 4096 + 8 - 16 = 184 come from P8-6, P8-7 and P8-8 (CTL 1, shift 0, no wrap). A select-0 cycle is F0' samples, because 32 x 6 x F0' / 192 = F0' (P8-9, H8-23). The select-1 step is 6 x 128 x 150 = 115,200 (P8-9, FLF != 0), so a
half table lasts 16 x 115,200 / 192 = 9,600 samples: 48 cycles of 200 or 52.2 of 184. The 40-cycle floor is a bound derived from these numbers, not a modelled value. The D-220 restart and the read instant only move the switch instants by at most one select-1 step. They never change the 16-step spacing of the
switches or the run values, and the rule computes no switch instant. Only the cycle that contains a switch is mixed. Its measured length is 184 + k/2 samples (k of its 32 steps at 200), rounded either way, so it lies between the two run values or merges with a run. The cycles on either side are
multiples of 192 clocks and measure exactly. So "at most one other cycle between two runs" holds under any read-instant or restart convention. The TRG-hold value (wave[1][0] = $18, hold-while-1 ASSUMED) equals words 0..15, so the hold-to-run edge is seamless. This argument needs the window to start at or after the R9 $01
event and select 1 to be keyed on before it (warning 1).

## Twin re-derivation (asked): ten groups only if select 1 really never keys on

Scene tone_pan01 has 14 PCM groups: A (sel 0) period L/R and cycle_order L/R; B (sel 1) the same; C1 silent_C1_R and period_0_C1_L; C2 silent_C2_L and period_1_C2_R; C3 period_0_C3_L and period_1_C3_R. Under F + 1 = 129, four cycles take 516 samples against 512, so every select-0 period group is red.
cycle_order_0_A stays green: at 774 clocks per step (4.03 samples), the run-collapse keeps the order. If select 1 never sounds: period_1_B L/R, cycle_order_1_B L/R, period_1_C2_R and period_1_C3_R are red; silent_C1_R and silent_C2_L are green. That gives 10 red and 4 green, which is the plan's set.
As written, though, the twin replaces only bytes $9F. C2 and C3 key select 1 on with AL $1D, which is R4 $9D, so select 1 sounds in C2 and C3 of the twin. period_1_C2_R and period_1_C3_R then go green, the stated rule gives 8 red groups, and the twin case fails on a correct core (blocker 2).

## Blockers (these properties must hold before execution)

1. [verification_derivation / context_compliance ER-10, ER-15] Every row of the 05-06 mask probe is admitted by its kind's ceiling, and every literal in it can be derived by hand without the key-on counter (05-06 Task 1).
   Evidence: (a) the probe rows are ev_decode rows classed MAN:P8-2+P8-3+P8-4+P8-6. The 05-05 KINDCLASS ceiling for ev_decode is MAN:P8-2+P8-3+P8-4, and 05-05 now defines a page-set rule ("MAN labels a subset of the kind's ceiling labels"), so audio-check reports AEXP_CLASS_INFLATED and the Task 1 verify is red as
   written. The 05-05 sentence "the sets above already admit the rows 05-06, 05-07 and 05-09 plan" is false for these rows. The R7 field layout, R8, R9 and the R9 $80 LFO_RESET effect (resets the select-1 counter) are page facts of P8-6 (RESEARCH section 1), so the plain ev_decode rows R7/R8/R9 also need P8-6. (b) The probe runs on select 1
   (R0 $F9) after tone_pan01, where select 1 played and was keyed off. Each row's wave_idx (R3 $F1, R4 $3F, R6 $E5 and their follow-ups) is the select-1 counter after the write. That counter depends on where select 1 stopped, which is the DISPUTED key-on counter ("keyon", LED-AUD-0004). A MAN row cannot rest on it (ER-15, R2),
   and the literal cannot be read from a run (R7). (c) The prohibition "R9 rows use $00 and $80 only" contradicts the probe write R9 $7C. The stored-value checks themselves are all right: $F9 & 7 = 1; $F1 & $0F = 1 -> post $01C0 (select 1's R2 is $C0); $3F & $DF = $1F; $E5 & $1F = 5 with WAVE_STORE at chON 0, DDA 0; $FC & 7 = 4; $78 & $9F = $18; $7C & $83 = $00.
   Exact fix: in 05-05 Task 1, set the ev_decode ceiling in KINDCLASS to MAN:P8-2+P8-3+P8-4+P8-6 and remove the false "already admit" claim. In 05-06 Task 1, place the select-1 probe directly after the R9 $80, R9 $00 pair (TRG 1 sets widx[1] = 0, MAN P8-6) or after an R4 $40 at select 1. Write the
   rows with wave_idx 0, 0 and 1: R3 $F1 (1, $00, 0, $01C0), R4 $3F (1, $00, 0, $001F), R6 $E5 (1, $01, 1, $0005). Place the select-4 R7 $78 row before tone_pan45, so that select 4 is still at widx 0. Reword the prohibition to "R9 stored values $00 and $80 only (CTL = 0)".

2. [verification_derivation / context_compliance D-211] The twin keeps one channel never keyed on, and its expect_red set equals what the stated byte rule produces (05-06 Task 3 and truth line 54).
   Evidence: the twin rule is "every select-1 key-on byte $9F written as $1F", but the select-1 key-on bytes are $9F in B and $9D in C2 and C3 (AL $1D). Select 1 therefore sounds in C2 and C3 of the twin, and period_1_C2_R and period_1_C3_R are green. D-211's "one channel never keyed on" is not met, and the ten-group expect_red list
   produces misprog-misattributed on a correct core.
   Exact fix: change the rule to "every select-1 R4 write with chON 1 is written with bit 7 clear ($9F -> $1F in B, $9D -> $1D in C2 and C3)" in Task 3 and in truth line 54, and keep the ten-group set. The other option (keep $9F only and drop period_1_C2_R and period_1_C3_R, 8 groups) is weaker and breaks D-211's "never keyed on".

3. [scope_sanity / context_compliance D-207, D-212] psg_tone.aexp, the one literal file of psg_tone (fixture-build writes one NAME.aexp.tsv per ROM), stays within D-207's 16 KiB and 512 rows with every row that 05-04, 05-05 and 05-06 add.
   Evidence: the planned rows are tone_sel0 (about 40 ev_value and 40 ev_decode rows, 8 PCM groups, 2 STRUCT groups), three pan scenes (14 PCM groups each), the ev rows for every register and channel (at least 37), the probe and edge rows (about 17), tone_atten (about 19 groups), the twin scene and the golden scene. Each row
   carries a class and derivation comment, and the old and new golden digests are recorded three times. That is about 70 PCM groups at about 200 bytes each, which is over 14 KB before any event row or comment. Phase 4 density is 6,971 to 16,223 bytes for 19 to 57 rows (spr_basic.vexp 16,223 at 57). AEXP_FILE_TOO_LARGE
   (05-05, D-207 "Limits as .vexp") would turn audio-check red partway through 05-06. No plan measures the file or names a route for when it overflows. Round 1 missed this; the revision made it larger (C1..C3, probe, R1 pair, STRUCT groups).
   Exact fix: in 05-04 Task 1, record a byte budget for psg_tone.aexp (the measured tone_sel0 bytes against the per-scene projection). Make every 05-06 task verify check `test "$(wc -c < fixtures/src/psg_tone.aexp)" -lt 16384`. State the route the plans take when the projection exceeds the cap. One route is compact EV rows (one row per write run) and one-line comments. The
   other is moving whole scenes to psg_mod: D-212 names that move for CODE overflow, and the orchestrator would have to record it as a plan-time ruling for .aexp overflow. Raising the cap contradicts D-207 and would need an erratum first.

4. [nyquist 8e / verify format] Each gate in the 05-10 sign-off verify can fail on the draft (05-10 Task 3).
   Evidence: `grep -q 'nyquist_compliant: true'` matches line 133 of the draft (the sign-off checkbox text), so the frontmatter flag can stay false while this conjunct passes. The other gates (`^status: final`, `wave_0_complete: true`, `! grep 'status: draft'`) are meaningful.
   Exact fix: anchor it as `grep -q '^nyquist_compliant: true'` in 05-10 Task 3 (or reword line 133 of the draft so it does not contain the literal string).

## Warnings (these properties should hold)

1. [verification_derivation] 05-09 Task 2 lfo_dynamic: the window and the scene order are part of the row. The scene list does not key select 1 on, and the window bounds are not given. A window that starts before R9 $01, or a select-1 key-on after it, puts a 192-to-200 or 512-to-200 transition cycle at the head of the window.
   That cycle is a run of one with a value outside {200, 184}, red on a correct core. A related claim is unsupported: "the lint recomputes ... F0' = 200 and 184". The 05-05 AEXP_ARITH has no rule for w1/w2/set (it recomputes F' only from w, with d = k x F'), and 05-09 cannot edit the lint. Example fix: key select 1 on while R9 = $81, anchor
   the window on the R9 $01 event with from >= 0 and to inside the scene, and either add set = {F'(w1), F'(w2)} to the 05-05 lint or drop the claim.
2. [verification_derivation] 05-07 Task 1 noise_seq: the ordinal origin is not stated. RF-11 has an event at time 0 (step 1, LFSR $20000 at M = 1), so the event at j x 11,904 is ordinal j + 1. The first post-R7 event has ordinal floor(R7/11,904) + 2. The window now runs to the key-on + 312,096, so about 116 events follow the reload, against the
   stated "+ 112". An off-by-one in the judge turns the ASSUMED row red on a correct core, and fixing it against the run would break R7. Example fix: write the mapping and the bound floor(R7/11,904) + 2 + 116 in the row comment. The startup code is short (startup.inc has no RAM clear), so the bound is far below 256.
3. [scope_sanity] Context risk after the additions (c): 05-04 (160k, 1.6x) and 05-06 (120k, 1.2x) kept round-1 estimates although both grew (aud_poweron_policy_diff and the obskinds.c update; C1..C3 in three scenes, the probe, phases 9a/9b, three golden regenerations). D-229 provides no stop inside a task, so 05-04 Task 1 and 05-06 Task 1 are the
   tasks most likely to exhaust context before their first commit. Expected splits are 05-03 and 05-04, and four more plans are over budget, so a sixth split, which D-229 sends to the orchestrator, is plausible. Example fix: refresh the estimates, and move the 05-06 event rows and probe into Task 3 so that Task 1 carries the scenes only.

## Info

1. Split numbering: 05-01's stop rule names "plan 05-11" and 05-02 takes "the next spare number", while D-229 reserves 05-11 for 05-03 and 05-12 for 05-04 and the standing rule assigns splits in order. Say "the next spare number" everywhere.
2. silent_pre starts at the R1 $FF event, but R5 $FF follows it, so for a few samples channel 0 sits at S = 15 with chON 0. The silence still rests on chON alone, but starting the window at the R5 $FF event makes the lint inputs r5=$FF exact.
3. snapshot_refusal_fields.toml keeps the comment "outside 1 to 4 (5 is the first unknown policy ...)" and meaning "a policy of 5". Update them with the row; the negated grep passes either way.
4. The 05-05 STRUCT count group needs a literal m. Use a frame boundary (k x 357,630 at 262 lines), never the run's stop time (R7).
5. The 05-01 objective line cites D-224 but not D-228 or D-229; the body has both.
6. Frame budget: psg_tone comes to about 30 frames by the sub-phase rule (pan45 alone is about 7.4). tone_atten has about 14 phases at F = 256 and may approach the 16-frame scene limit, so state the phase lengths.
7. Carried: no {FAILING_DIRECTIONS} or {VERIFY_PATHS} probe was supplied, so check 8f is not scored.

## Structured issues

```yaml
issues:
  - plan: "05-06,05-05"
    task: 1
    dimension: verification_derivation
    severity: blocker
    required_property: "every mask-probe row is admitted by its kind ceiling and every literal is hand-derivable without the key-on counter (ER-10, ER-15)"
    description: "MAN:...+P8-6 exceeds ev_decode ceiling MAN:P8-2+P8-3+P8-4 under the page-set rule; select-1 wave_idx after pan01 depends on the DISPUTED keyon counter; prohibition 'R9 $00 and $80 only' vs R9 $7C"
    fix_hint: "ceiling + P8-6; probe after R9 $80 (widx[1] = 0); rows (1,$00,0,$01C0), (1,$00,0,$001F), (1,$01,1,$0005); reword prohibition"
  - plan: "05-06"
    task: 3
    dimension: verification_derivation
    severity: blocker
    required_property: "the twin's select 1 is never keyed on (D-211) and expect_red equals the rule's outcome"
    description: "rule replaces $9F only; C2/C3 key-on bytes are $9D, so period_1_C2_R and period_1_C3_R stay green (8 red, not 10)"
    fix_hint: "clear bit 7 of every select-1 chON-1 R4 write ($9F -> $1F, $9D -> $1D)"
  - plan: "05-04,05-05,05-06"
    dimension: scope_sanity
    severity: blocker
    required_property: "psg_tone.aexp fits D-207's 16 KiB and 512 rows"
    description: "about 70 PCM groups (over 14 KB) plus about 100 event rows and comments; Phase 4 vexp 16,223 bytes at 57 rows; no budget or overflow route planned"
    fix_hint: "byte budget in 05-04, wc -c gate per 05-06 task, compact rows or a recorded D-212-style scene move"
  - {plan: "05-10", task: 3, dimension: nyquist_compliance, severity: blocker, required_property: "every sign-off gate can fail on the draft", description: "'nyquist_compliant: true' matches draft line 133", fix_hint: "grep -q '^nyquist_compliant: true'"}
  - {plan: "05-09", task: 2, dimension: verification_derivation, severity: warning, required_property: "lfo_dynamic measures only cycles after R9 $01 with select 1 already on; its lint claim is backed by a lint rule", description: "window bounds and select-1 key-on unstated; AEXP_ARITH has no w1/w2/set rule", fix_hint: "key select 1 on under R9 $81, anchor at R9 $01, add or drop the set lint"}
  - {plan: "05-07", task: 1, dimension: verification_derivation, severity: warning, required_property: "noise_seq ordinals are defined from RF-11 without reference to a run", description: "ordinal origin unstated (event at T = 0 is step 1); bound says +112, window needs about +116 after the reload", fix_hint: "ordinal = j + 1 for j x 11,904; bound floor(R7/11904) + 118"}
  - {plan: "05-04,05-06", dimension: scope_sanity, severity: warning, required_property: "each plan's estimate reflects its tasks and each first task fits one context", description: "estimates unchanged after growth; no intra-task stop under D-229", fix_hint: "refresh estimates; move 05-06 ev rows and probe to Task 3"}
  - {plan: "05-01,05-02", dimension: dependency_correctness, severity: info, required_property: "split numbers agree with D-229", description: "05-01 names 05-11, reserved for 05-03", fix_hint: "next spare number"}
  - {plan: "05-04", task: 1, dimension: verification_derivation, severity: info, required_property: "silent_pre lint inputs match the whole window", description: "R5 $FF follows R1 $FF", fix_hint: "start at the R5 $FF event"}
  - {plan: "05-03", task: 3, dimension: task_completeness, severity: info, required_property: "case text matches the moved row", description: "toml comment and meaning say 1 to 4 / policy 5", fix_hint: "update with the row"}
  - {plan: "05-05", task: 2, dimension: verification_derivation, severity: info, required_property: "count group literal is not a run value", description: "m unspecified", fix_hint: "frame boundary k x 357,630"}
  - {plan: "05-01", dimension: context_compliance, severity: info, required_property: "objective cites the errata it applies", description: "D-228, D-229 missing from the objective line", fix_hint: "add them"}
  - {plan: "05-06", dimension: scope_sanity, severity: info, required_property: "scene lengths are stated against D-214", description: "tone_atten about 14 phases", fix_hint: "state phase lengths"}
  - {plan: "phase", dimension: nyquist_compliance, severity: info, required_property: "check 8f scored", description: "no probes supplied", fix_hint: "supply FAILING_DIRECTIONS and VERIFY_PATHS"}
```

## Dimension summary

| Dimension | Result |
|---|---|
| 1 Requirement coverage | AUD-01..03 each claimed; AUD-03 c negative now planned (05-04); 05-VALIDATION maps AUD-01 a-f, AUD-02 a-c, AUD-03 a-c, SC1..SC4 |
| 2 Task completeness | 29 of 29 structurally complete; blocker 4 |
| 3 / 3b Dependencies, coupling | pass; no same-wave overlap (d) |
| 4 Key links | pass |
| 5 Scope sanity | blocker 3; warning 3 |
| 6 Verification derivation | blockers 1, 2; warnings 1, 2; ER-10 holds for lfo_dynamic |
| 7 / 7b Context compliance, scope reduction | D-228, D-229 consistent; D-211 (blocker 2), D-207 (blocker 3); no reduction wording; no deferred idea |
| 7c Tier compliance | pass |
| 8 Nyquist | draft present; blocker 4 |
| 9 Data contracts | round-1 warning 2 fixed; lint contract gap (warning 1) |
| 10 CLAUDE.md / AGENTS.md | pass (tick revert, no path leak, no push, attribution line) |
| 11 Research resolution | pass (Open Questions RESOLVED) |
| 12 Pattern compliance | pass |
| Verify format / paths | blocker 4 |

## Recommendation

REVISE. Each blocker is a local edit. Blocker 1 is one KINDCLASS token in 05-05 plus the probe placement and literals in 05-06 Task 1. Blocker 2 is one phrase in 05-06 Task 3 and truth line 54. Blocker 3 adds a byte budget and a wc -c gate (an orchestrator ruling only if the scene-move route is chosen). Blocker 4 is
one anchor in 05-10. Apply warnings 1 and 2 in the same pass: as written, each makes an executor choose a convention that decides whether a row is green.
