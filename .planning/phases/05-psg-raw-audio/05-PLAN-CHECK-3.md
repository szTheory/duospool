# Phase 5 plan check 3 (gsd-plan-checker, round 3 of 3, final)

Phase: 05-psg-raw-audio. Plans checked: 05-01 .. 05-10 and 05-VALIDATION.md at af2a732 (revision after plan check 2), against 05-CONTEXT (D-182..D-230; errata D-219..D-230 win), 05-PLAN-CHECK-1.md, 05-PLAN-CHECK-2.md, the af2a732 diff and the repository at HEAD (read-only).

Result: ISSUES FOUND, verdict REVISE. 2 blockers, 1 warning, 8 info. Every round-2 blocker, warning and info item is fixed in the plan text. One blocker is a defect the revision added: a verify command that is a shell syntax error. The other was there before the revision and rounds 1 and 2 missed it. It
is a conflict between two locked decisions, so the orchestrator has to rule on it. The revision only made it easier to hit.

## Mechanical checks run

- verify.plan-structure: 10 of 10 exit 0, valid true, errors [], warnings []; 29 tasks, each with files, action, verify and done. frontmatter.validate --schema plan: 10 of 10 valid, no missing or invalid fields.
- files_modified equals the union of task <files> in 10 of 10 plans (script). Waves and depends_on unchanged (1:[01,02] 2:[03] .. 9:[10]); no cycle; no same-wave overlap.
- 0 non-ASCII bytes and 0 lines of 400 or more characters in the ten plans and 05-VALIDATION.md. Home and temp paths appear only in the rule sentence that forbids them and in grep guards.
- Policy numbers: v4 is enum value 5 everywhere; policy-6=field only for the moved hostile row (05-01 PR-9, 05-03). D-ranges: 05-01 uses D-182..D-230, errata D-219..D-230 and `seq 182 230`; no stale D-..229 range is left. Stop rule: "about 80 percent" in every plan; "60 percent" only as "D-229 supersedes D-216's 60 percent".
- No 16 KiB or 16,384 is left for .aexp: the remaining hits are the .vexp cap, record goldens (F-56), ledger files and the 16,384-record psg sink.
- Every task that grows an .aexp has a `wc -c` gate under 32768: 05-04 T1, 05-05 T2 and T3, 05-06 T1..T3, 05-07 T1 and T2, 05-09 T2. bash -n and dash -n on all 29 <automated> blocks: 28 parse; 05-09 T2 does not (blocker 1).
- 05-10 T3 sign-off gates run against the draft: `^nyquist_compliant: true`, `^status: final` and `^wave_0_complete: true` each fail, and `! grep -q '^status: draft'` fails, so every conjunct can fail. The draft line 133 is reworded.
- The new gates fail at HEAD: `AEXP_MAX_BYTES *= *32768` (neither file exists yet) and the wc -c gates (no .aexp yet). estimate-check --calibrated (budget 100,000, confidence low): 05-04 1.75x, 05-03 1.5x, 05-05/05-06/05-07 1.3x, 05-09 1.15x; others under budget. All are advisory and D-229 handles splits.

## Round-2 findings, read in the plan text

- B1, mask probe: fixed. The 05-05 KINDCLASS ev_decode set is MAN:P8-2+P8-3+P8-4+P8-6 and the false "already admit" sentence is gone. In 05-06 T3 the probe follows R9 $80 ($FE,$10,$FF,$0080) and R9 $00 after tone_pan01, with select 1 off. Rows recomputed: R0 $F9 ($FE,$00,$FF,$0001); R3 $F1 (1,$00,0,$01C0)
  over R2 $C0; R3 $00 (1,$00,0,$00C0); R4 $3F (1,$00,0,$001F); R6 $E5 (1,$01,1,$0005: WAVE_STORE at chON 0, DDA 0, TRG 0); R9 $7C ($FE,$00,$FF,$0000). R7 $78 (4,$00,0,$0018) sits after select 4's R4 $40. Each wave_idx is a page fact, and none depends on the keyon counter. The prohibition now reads "stored
  values $00 and $80 only (CTL = 0)".
- B2, twin: fixed in truth line 54 and T3. Bit 7 is cleared on every select-1 chON-1 R4 write ($9F -> $1F in B, $9D -> $1D in C2 and C3). Red set recomputed from the scene: select 0 at F+1 = 129 (516 vs 512, or fewer than five crossings) turns period_0_A_L/R, period_0_C1_L and period_0_C3_L red. Select 1 never on turns
  period_1_B_L/R, cycle_order_1_B_L/R, period_1_C2_R and period_1_C3_R red. That is 10 red. Green: cycle_order_0_A_L/R, silent_C1_R and silent_C2_L, 4 groups. This equals the expect_red list.
- B3, D-230: fixed. One constant AEXP_MAX_BYTES = 32768 is checked before parsing; there is a 32,768-byte pass control, a 32,769-byte refusal and a lex selftest. 05-04 T1 records a byte budget and projection, uses one-line comments when the projection is over, never moves a scene for .aexp size, and stops and reports if still over.
  The handbook (9) carries the cap; 05-05 truth and T-05-18 say 32,768.
  The runner's .aexp.tsv reader has no byte cap (read_file mallocs the file size; only 400 characters and 512 rows), so a file between 16 and 32 KiB is safe there.
- B4: fixed (anchored greps). W1, lfo_dynamic: fixed. R9 $81 is written first and both selects are keyed on under TRG 1; the window runs from R9 $01 +0 to +4,032,000 (21,000 x 192). The 05-05 set rule recomputes 6 x 128 x 150 = 115,200, F'(w1) = 200 and F'(w2) = 184. The round-2 ER-10 ruling still holds.
- W2, noise_seq: fixed. Ordinal j + 1 at j x 11,904; the reload has ordinal floor(R7/11,904) + 2; the bound is + 116, which holds while the key-on follows R7 by under 2,400 clocks (312,096 + delta < 117 x 2,688). mod_noise4 is the first scene, so the bound is far below 256.
- W3: fixed. 05-04 is now 175k with a 1a/1b stop that has its own verify; 05-06 is 130k with a 1a/1b stop, and its event rows and probe moved to T3.
- Info 1-6: applied (split numbering, silent_pre from R5 $FF, toml comment and meaning, frame-boundary m, objective cites, tone_atten phase lengths of about 6.9 frames).

## Blockers (these properties must hold before execution)

1. [verify format] Every <automated> command is valid sh as written (05-09 Task 2). This defect was added by the revision.
   Evidence: lines 175-176 break inside a command substitution, `test "$(wc -c <` NEWLINE `fixtures/src/psg_lfo.aexp)" -lt 32768`. bash -n: "syntax error near unexpected token `newline'"; dash: "Syntax error: newline unexpected"; zsh: parse error. The plans require each narrow verify "exactly as written", so this task can never go green on a correct
   implementation. All 28 other blocks break only after `&&`.
   Example fix (non-binding): move the line break to before `test`. This is a whitespace-only edit the orchestrator can make in place.

2. [context_compliance / verification_derivation D-190, D-192, D-203] The restore validation accepts every state that the write path can reach (05-03 Tasks 2 and 3, exposed by 05-09 Tasks 2 and 3). This was there before the revision.
   Evidence: 05-03 T3 (D-190) defines R6 at chON 0, DDA 0 as wave[widx] = v, then widx + 1, with no TRG term. R9 bit 7 only sets widx[1] = 0 at the write (D-192, 05-03 T3 line 241). 05-03 T2 (D-203 "lfo_ctl bit 7 implies widx[1] = 0") refuses that state in restore pass 1 and adds a hostile row "TRG with widx[1] != 0".
   The guest sequence R9 $80, R0 1, R4 $00, R6 v therefore reaches lfo_ctl $80 with widx[1] = 1, and the core then refuses to restore its own snapshot. That sequence is the natural LFO setup: reset, upload the modulator table, release.
   The revised 05-09 lfo_dynamic writes R9 $81 first and then programs select 1's table. TRG is 1 from the psg_lfo setup through lfo_needs_sel1 anyway. 05-09 T3 requires a snapshot_restore_audio stop "between LF TRG 1 and 0", and D-200 also lists a stop "inside a TIN to $0806". A stop inside that upload makes the restore fail on a core built as planned.
   Fixing it in the core contradicts one of the two locked decisions, so the executor would have to stop. If no stop lands there, every case stays green while the restore claim of AUD-02/D-203 is false for a reachable state.
   Example fix (non-binding): an orchestrator erratum. Either drop the widx[1] invariant from D-203 (keep "rem[1] = 0 while TRG 1"; 05-03 drops that refusal and its hostile row), or rule R6 at select 1 under TRG 1 as stored without increment (ASSUMED, LED-AUD-0004). Optionally, also have 05-09 upload the lfo_dynamic table in setup, before the first R9 $80.

## Warning

1. [task_completeness, D-230] 05-05 T1 states how the exit-2 size control and its selftest are asserted. Evidence: D-230 makes a 32,769-byte .aexp a harness error (exit 2). The plan routes it through "a twin of vc_gen_limits" and "the z7d copy". In scripts/check, run_controls turns any exit 2 into HARNESS_ERROR, so the stage returns 2 (lines 162-165).
   The z7d loop also counts a case as alive only on exit 0 with EXPECT_OK, and wants exit 1 after the mutation (lines 2806-2812). A literal copy therefore turns stage audio-check (the T1 verify) and the plan-end selftest red. An executor who "fixes" this by switching to exit 1 contradicts D-230. Example fix: assert exit 2 plus the AEXP_FILE_TOO_LARGE line outside
   run_controls, as pf_empty_path_control does (scripts/check:184), and set the selftest's alive condition for this case to exit 2.

## Info

1. 05-06 T1 <done> still says "every register's decode is pinned by a literal"; that work moved to T3. T3's RULES line does not move the D-196 decode clauses to row.
2. 05-06 commit 1a has no narrow verify of its own (05-04's 1a has one).
3. 05-VALIDATION 05-06-T1 still lists "event rows, mask probe", and 05-05-T1 omits the D-230 control. The plan wins and 05-10 corrects the file.
4. KINDCLASS widens D-208's listed ceilings (ev_decode +P8-6; period and live +P8-10). This is correct by the evidence, but it is recorded only as plan-check fixes; the orchestrator may want to record it as a ruling.
5. The 05-05 lexer's closed key list must admit w1, w2 and set; the set rule implies these keys but does not list them.
6. lfo_dynamic's window ends at the minimum hold (21,000 samples). Hold the scene longer than the window.
7. The 05-04 budget projects bytes only; project the 512-row limit too (tone_sel0 alone has 32 R6 rows).
8. Carried: check 8f was not scored (no FAILING_DIRECTIONS or VERIFY_PATHS probe).

## Structured issues

```yaml
issues:
  - {plan: "05-09", task: 2, dimension: verify_format, severity: blocker, required_property: "every automated command is valid sh as written", description: "line break inside $(wc -c < ...): bash/dash/zsh syntax error; added by af2a732", fix_hint: "break the line before test"}
  - plan: "05-03,05-09"
    dimension: context_compliance
    severity: blocker
    required_property: "restore accepts every state the write path reaches"
    description: "R6 at select 1 (chON 0, DDA 0) under TRG 1 increments widx[1] (D-190); restore refuses lfo_ctl bit 7 with widx[1] != 0 (D-203); 05-09 lfo_dynamic uploads under TRG 1 and T3 needs a stop between TRG 1 and 0"
    fix_hint: "orchestrator erratum: drop the widx[1] invariant, or rule R6 under TRG as no-increment ASSUMED"
  - {plan: "05-05", task: 1, dimension: task_completeness, severity: warning, required_property: "the D-230 exit-2 control and its selftest are expressible in the gate", description: "run_controls maps exit 2 to HARNESS_ERROR; z7d alive needs exit 0 + EXPECT_OK", fix_hint: "dedicated exit-2 assertion as pf_empty_path_control"}
  - {plan: "05-06", task: 1, dimension: task_completeness, severity: info, required_property: "done matches the task", description: "done still claims the decode rows moved to T3", fix_hint: "drop the clause"}
  - {plan: "05-06", task: 1, dimension: scope_sanity, severity: info, required_property: "each named stop point has a verify", description: "1a has none", fix_hint: "name the 1a subset"}
  - {plan: "phase", dimension: nyquist_compliance, severity: info, required_property: "VALIDATION rows match the plans", description: "05-06-T1 and 05-05-T1 rows stale", fix_hint: "05-10 corrects"}
  - {plan: "05-05", task: 1, dimension: context_compliance, severity: info, required_property: "ceiling widenings are recorded rulings", description: "ev_decode +P8-6, period/live +P8-10 beyond D-208", fix_hint: "record as plan-time ruling"}
  - {plan: "05-05", task: 1, dimension: verification_derivation, severity: info, required_property: "closed keys cover the set rule", description: "w1, w2, set not listed", fix_hint: "list them"}
  - {plan: "05-09", task: 2, dimension: verification_derivation, severity: info, required_property: "window inside the scene", description: "to = minimum hold", fix_hint: "hold longer"}
  - {plan: "05-04", task: 1, dimension: scope_sanity, severity: info, required_property: "row budget projected", description: "bytes only", fix_hint: "project rows against 512"}
  - {plan: "phase", dimension: nyquist_compliance, severity: info, required_property: "check 8f scored", description: "no probes supplied", fix_hint: "supply probes"}
```

## Recommendation

REVISE. This is the final round, so the orchestrator escalates. Blocker 1 is a whitespace-only edit the orchestrator can make in place. Blocker 2 needs an erratum ruling between D-190/D-192 and D-203, plus the matching edits in 05-03 T2 (refusal and hostile row) or in the R6 rule, and optionally a 05-09 setup-order note. Apply the warning in the same pass. Everything else is ready for execution.
