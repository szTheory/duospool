---
phase: 05-psg-raw-audio
plan: 08
subsystem: checkpoint-a-cut-line
tags: [aud, checkpoint, cut-line, d-217, d-214, er-2, er-22, promotion, mutant-sweep, linux-repro, dash, second-awk, w7]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-01..05-07 and 05-12 (the PSG model, psg_tone and psg_mod with their literal scenes, the audio judge, the fault sweep, six audio toys)
provides:
  - docs/ci-timings.md "Phase 5 checkpoint A": the phase-gate set (sh, dash, selftest, the Phase 5 audit range, all six Linux parts), two bracketed timing pairs against the 05-01 baseline tree, instruction counts, the D-214 rows, the CI estimate and the "Cut-line record (D-217)"
  - LED-AUD-0008 supported (E1 documentary) under ER-2 and ER-22, pinned to TEST:audio_tone_atten, README list equal to the ledger set
  - the handbook sections 9 and 11 with the measured values and the outcome
  - a one-off scratch mutant sweep of src/core/psg.c (108 mutants, table below; not committed, not CI)
affects: [05-09, 05-10, a possible Phase 5.1]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-02, AUD-03]

actuals:
  tokens: 14900
  tasks: 3
  commits: 3
plan_head_before: 29f9016b9007381800f53590a0dedb79da522b2e
plan_head_after: 33cba951343d7b1a14529e8a6aa624b7c44b8888

tech-stack:
  added: []
  patterns:
    - "A load-confounded user CPU comparison is bracketed: the baseline tree runs warm from a scratch clone right after the current tree, and instructions retired by one runner process (/usr/bin/time -l) give a load-independent check"
    - "A scratch mutant sweep classifies each kill by guest literal scenes, other guest cases (sweeps, golden, controls, proof set) and C-level cases, so a clause killed only at C level is visible for ER-22"

key-files:
  created:
    - .planning/phases/05-psg-raw-audio/05-08-SUMMARY.md
  modified:
    - docs/ci-timings.md
    - ledger/entries/LED-AUD-0008.toml
    - README.md
    - tests/conformance/cases/audio_tone_atten.toml
    - .planning/phases/05-psg-raw-audio/05-HANDBOOK.md

key-decisions:
  - "05-08: LED-AUD-0008 is supported (E1 documentary): every clause has a MAN group in a guest-fixture case and img sources with no in-page conflict on it; LED-AUD-0003 and LED-AUD-0005 stay assumed, each with its blocking clause recorded"
  - "05-08: cut trigger (3) is borderline (+20.0 and +32.4 percent user CPU in two bracketed pairs, +22.2 and +25.2 percent instructions); triggers 1, 4, 5, 6 not fired, trigger 2 unverified; the executor recommends the cut (LFO to Phase 5.1) and the orchestrator decides"

duration: about 3 h 40 min wall including a machine restart (about 2 h 50 min after it); gates at load 6 to 44
completed: 2026-11-13
---

# Phase 5 Plan 08: Checkpoint A and the cut decision Summary

**The phase so far passes every gate shape (sh, dash, selftest, audit, six Linux parts with mawk and gawk). LED-AUD-0008 (silence and the attenuation order) is now supported (E1 documentary). The mutant sweep leaves only equivalent survivors. Cut trigger (3) is borderline, so the record recommends moving the LFO to Phase 5.1 and leaves the decision to the orchestrator.**

## Performance

- Tasks: 3 of 3, no split. Commits fec4cd8, 1e3a6e1, 33cba95; 5 files, 173 insertions, 25 deletions (before this SUMMARY).
- The machine restarted during the first attempt (no commit had been made). The scratch directory was lost and rebuilt; the repository was clean at 29f9016 and `.planning/state.json` kept its bytes.

## Gate results (one gate per checkout at a time; the baseline clone is a separate checkout)

| Run | Result | Wall s | User CPU s | Load (1 min) |
|---|---|---|---|---|
| `dash scripts/check` at 29f9016 | PASS, 59 CHECK PASS lines (58 stages) | 495.9 | 403.5 | 10.3 to 9.9 |
| `sh scripts/check --timing` at 29f9016, runs A and C | PASS, 58 stages | 384.3, 376.4 | 316.8, 317.1 | means 11.5, 7.7 |
| `sh scripts/check --selftest` at 29f9016 | SELFTEST RESULT PASS, 207 PASS lines | 109.3 | 38.8 | 10.7 |
| `sh scripts/audit-history --range f6afede..29f9016` (Phase 5) | clean: 37 commits, 383 blobs, 2,431 pairs | 37.4 | | |
| `sh scripts/linux-repro`, all six parts at 29f9016 | RESULT PASS: default-awk (mawk, gcc 13; 1,493 s), mawk (clang 18; 1,120 s), gawk (5.2.1, gcc; 914 s), gate-gcc, gate-clang, core-boundary-gcc; each gate 58 stages and its selftest exit 0 | 5,142 | | 10 to 27 |
| `sh scripts/check --timing` before the Task 2 commit | PASS | 450.6 | 357.6 | 10.2 to 13.0 |
| Task 1, 2 and 3 verifies as written | VERIFY1_OK, VERIFY2_OK, VERIFY3_OK | | | |
| plan end: `sh scripts/check --timing` at 33cba95 | PASS, 59 CHECK PASS lines | 403.2 | 339.5 | 7.0 to 12.9 |
| plan end: `sh scripts/check --selftest` | SELFTEST RESULT PASS, 207 PASS lines | 143.2 | 45.2 | 12.9 to 44.1 |
| plan end: `sh scripts/audit-history --range 29f9016..HEAD` | clean: 3 commits, 6 blobs, 2,222 pairs | | | |

No C file changed in this plan, so the Linux run covers the plan's tree for every C path. ROM-SIZE: none of the 248 files added or modified in Phase 5 has a ROM-SIZE shape; this plan's five files are text.

## Timing against D-214 and cut trigger (3)

The machine was never quiet: unrelated jobs held the load at 6 to 18. A quiet window was waited for, about 9 minutes before the restart and 9 after, and did not come. The 05-01 baseline tree (9619efe) was therefore re-measured warm from a scratch clone, right next to this tree:

| Pair | This tree, user CPU | 9619efe, user CPU | Growth |
|---|---|---|---|
| 1 (runs A, B, C) | 316.8 and 317.1 s (means 11.5, 7.7) | 264.2 s (mean 12.9) | +20.0 percent (+17.4 on the recorded 270.0 s) |
| 2 (runs D, E, two hours later) | 353.4 s (mean 10.6) | 266.9 s (mean 8.8) | +32.4 percent (+30.9 on 270.0 s) |

The two other runs of this tree read 357.6 s (the Task 2 gate) and 339.5 s (plan end), against a threshold of 337.5 s.
Instructions retired by one `ds_runner` process over the full case list, this tree against 9619efe: +22.2 percent in the gate preset (1,011.97 G against 827.87 G) and +25.2 percent in the sanitize preset (4,076.45 G against 3,256.00 G).
The user CPU of the same work moved 18 to 53 percent with the load. A likely cause, not verified: Apple silicon runs a contended job partly on its efficiency cores.
Trigger (3) is borderline: two of four like-for-like measures are over +25 percent. Every measure is over the D-214 band for the whole phase (+10 to +20 percent) before the LFO lands.

Stage growth against run B: sanitizers 135 to 168 s, conformance 62 to 76 s, determinism 58 to 62 s, and audio-check is new at 5 to 8 s (estimated 2 s). The 53 audio cases run in about 7 s, so most of the growth is the PSG fold in every guest step and the six audio toys.
CI critical path estimate, unverified (D-64): gate-gawk about 523 s; gate-mawk about 528 s on a fast runner draw and 665 s on a slow one. The path is about 525 to 670 s against the 600 s job trigger.

## Cut-line record (D-217), for the orchestrator

| # | Trigger | Evidence | Result |
|---|---|---|---|
| 1 | more than two revision plans (05-16+) | none exists; 05-12 is a stop-rule split (PR-16); the sweep leaves no missing row | not fired |
| 2 | a CI work job over 600 s | no Phase 5 CI run; estimate 525 to 670 s by runner draw | unverified, no CI run yet |
| 3 | local warm user CPU over +25 percent | +20.0 / +32.4 percent in two pairs; +22.2 / +25.2 percent instructions | borderline |
| 4 | the LFO needs a model row or an unsettleable discrepancy | 05-09 adds no model row; DX-4 and DX-16 do not act on its MAN rows (TRG 1 held, w = $18) | not fired |
| 5 | the LFO fold cannot be bounded | at most 600 clocks per fold (D-221), about 420 events at d = 3, F = FLF = 1 | not fired (analytical) |
| 6 | owner preference for an earlier pull request | none relayed | not fired |

**Recommendation: take the cut.** Insert Phase 5.1 with 05-09 as plan 5.1-01 (psg_lfo, psg-lfo-shift). 05-10 closes Phase 5 with the LFO under DSPE UNMODELLED. ROADMAP criterion 1 gains "LFO registers logged and decoded; LFO modulation in 5.1", and the toy count stays 32.
This is a judgment on a borderline trigger. Read strictly ("cut only on a trigger that fired beyond doubt"), the rule says stay, with 05-09 next. Both readings rest on the same numbers in docs/ci-timings.md. The insertion, if taken, is the orchestrator's (gsd-tools).

## ER-2 and ER-22 promotion pass (per clause)

**LED-AUD-0008, promoted (commit 1e3a6e1).** Sources: P8-3, P8-4 and P8-10, all img rows. DX-5 is the only in-page conflict, and it concerns the sum 15, which is outside the statement.

| Clause | MAN group(s) in guest-fixture cases | Declared kills | Mutants and toys that turn the case red |
|---|---|---|---|
| chON 0 silent | silent_pre_L, silent_pre_R (audio_tone_sel0, MAN:P8-4); same_Y (audio_tone_atten, MAN:P8-4) | AF6; AF5, AF6 | Y03, Y04 |
| R1 or R5 $00 silent | sil_P5 (R1 $00), sil_P4 (R5 $00), audio_tone_atten, MAN:P8-10 | AF6 | case-level: psg-atten-max, S01, S03 |
| P8-10 sum 16 or more silent | sil_P3_R (S = 16, audio_tone_atten); sil_C1_R, sil_C2_L (audio_tone_pan01, -pan23, -pan45), MAN:P8-10 | AF3, AF6 | S02, S03, Y10 |
| strict decrease per R1, R4, R5 step | ao_r1 (R1 $FF, $EF), ao_al ($1F, $1E), ao_lal (R5 $FF, $EF, $DF), audio_tone_atten, MAN:P8-3+P8-4+P8-10 | AF5, AF6 | psg-atten-max, S01, Y01, Y13 |

ER-2 conditions:
- img cite: yes.
- A literal from the page: yes.
- Statement says "equal to the manual's stated rules": yes, worded in.
- withheld lists hardware accuracy, timing and compatibility: yes, added.
- Mutual pin: TEST:audio_tone_atten, with the case's pins naming the entry. That pin edit is a file outside files_modified; see deviation 1.

The full gate passed before the commit, and README's supported list matches the ledger set (DOCS_SUPPORTED_LIST).

**LED-AUD-0005, stays assumed.** Clause (a), store and increment, is covered by ev_decode_sel0 and cycle_order. Clause (b), the DDA reset, is covered by ev_decode_sel0, d_dec and the toy psg-dda-no-reset. Clause (d), chON 0 off, is covered by silent_pre. Clause (c) blocks, for two reasons:
- "with DDA 1 an R6 write goes to the D/A" includes chON 0. There the latch load is the ER-15 disputed point latch_off (LED-AUD-0004, RULES aud04-h), and P8-4 gives the D/A only for chON 1 and DDA 1.
- Its timing row d_hold rests, at ordinal 1, on the latch level before write 1. That level comes from the latch_off point or from the assumed power-on latch (05-07 deviation 6), so by ER-10 the group is not a clean MAN row.
- d_ord and d_ram are clean, but they judge the order of the plateaus and RAM kept, not "until the next write".
- The fix belongs in 05-10, not a revision plan: narrow (c) to chON 1 and DDA 1 (a correction to P8-4) and split d_hold's ordinal 1 into an ASSUMED row with mark=latch_off.

**LED-AUD-0003, stays assumed.**
- Covered by MAN groups: A3-A0 0..9 select R0..R9; R0 bits 2-0 pick channel 0..5; R1, R8 and R9 ignore R0 (ev_decode groups of psg_tone).
- Blocked as disputed or converse (ER-15, ER-16):
  - "$A to $F none" and "(6, 7 none)" are ER-15 disputed points with ASSUMED rows only (the edge group).
  - "R7 exists for select 4 and 5 only" is a converse held ASSUMED (the edge row at select 3).
- Blocked as partial on the mask clause: no guest literal row shows R2 bits 1-0, R3 bit 3, R8, or R9 bits 1-0 stored.
  - The sweep's K03 (R2 $FC) is killed only by the golden, the twin control and C-level cases.
  - K04 (R3 $07) and C01 (F & $7FF) are killed at C level only; no guest F reaches $800.
  - K09 (R8 $00) is killed by psg_decode only.
  - K10 (R9 $80) is killed at C level only.
  - The entry's own disagreement field records the stored bits of shaded fields as a project choice (D-189).
- Path: 05-09 brings guest R8 and R9 CTL writes. A later promotion would need guest rows for R2 bits 1-0 and an F of $800 or more, and a narrowed statement, whose disputed clauses would then need another ledger home (tests/audio/DISPUTED maps reg $A..$F to LED-AUD-0003).

**Not reviewed here:** LED-AUD-0002 waits for 05-09 (its LFO placement clause has no guest row). LED-AUD-0001, -0006 and -0011 are held by ER-14. The supported count is 14.

## Scratch mutant sweep of src/core/psg.c (one-off, outside the repository, not CI)

Method:
- A scratch clone of 29f9016 was built with its own `--stage conformance`. For each mutant, one statement of psg.c was edited and ds_runner was rebuilt in the gate preset.
- The runner ran the 53 audio, psg_ and aud_ cases. Outcomes were compared with the unmutated run, where all 53 pass.
- Columns: LIT names the guest literal scenes whose case turned (sel0, p01, p23, p45, atten, noise4, noise5, r7, dda). "Other guest" names fault sweeps, the golden, the twin and d3 controls, DSV checks and proof-set cases. "C-level" lists those cases, or counts them when a literal scene also turned.
- Q04 hung after its kills (period 0) and was stopped; K01 and W07 ended with a bus error. Each counts as killed.
- Totals: 108 mutants, 102 killed, 3 survived, 3 stillborn (-Werror; replaced by F03b, Y15b and W34b, all killed). Every function has at least one mutant.

| Id | Line: change | Result | LIT | Other guest | C-level |
|---|---|---|---|---|---|
| D01 | 26: `? 3u : 6u` -> `? 6u : 3u` | KILLED | noise4 noise5 atten p01 p23 p45 sel0 | sweep golden d3 | 5 C-level |
| K01 | 30: `{0x07u,` -> `{0x03u,` | KILLED | noise4 noise5 atten p45 | sweep golden twin psg_mod psg_tone restore | 12 C-level |
| K02 | 30: `0x07u, 0xFFu,` -> `0x07u, 0xFEu,` | KILLED | atten p01 sel0 | sweep golden d3 | 2 C-level |
| K03 | 30: `0xFFu, 0xFFu, 0x0Fu` -> `0xFFu, 0xFCu, 0x0Fu` | KILLED |  | golden twin | psg_decode psg_fold psg_pure_function |
| K04 | 30: `0x0Fu, 0xDFu` -> `0x07u, 0xDFu` | KILLED |  |  | psg_decode psg_pure_function snapshot_refusal_psg |
| K05 | 30: `0xDFu, 0xFFu` -> `0x9Fu, 0xFFu` | KILLED | dda atten p01 p45 sel0 | sweep golden d3 restore | 3 C-level |
| K06 | 30: `0xDFu, 0xFFu, 0x1Fu` -> `0xDFu, 0xFEu, 0x1Fu` | KILLED | atten p01 p45 sel0 | sweep golden d3 | 2 C-level |
| K07 | 30: `0x1Fu, 0x9Fu` -> `0x0Fu, 0x9Fu` | KILLED | dda r7 atten p01 p23 p45 sel0 | sweep golden twin d3 restore | 5 C-level |
| K08 | 30: `0x9Fu, 0xFFu` -> `0x1Fu, 0xFFu` | KILLED | noise4 noise5 | sweep restore | 2 C-level |
| K09 | 30: `0x9Fu, 0xFFu, 0x83u` -> `0x9Fu, 0x00u, 0x83u` | KILLED |  |  | psg_decode |
| K10 | 30: `0x83u, 0u` -> `0x80u, 0u` | KILLED |  |  | psg_decode psg_lfo_trg snapshot_refusal_psg |
| K11 | 30: `0x83u, 0u` -> `0x03u, 0u` | KILLED | p01 | sweep golden | 3 C-level |
| K12 | 34: `reg < 16u` -> `reg < 15u` | SURVIVED |  |  |  |
| P01 | 41: `k < 6u` -> `k < 5u` | KILLED |  |  | psg_fold |
| P02 | 42: `inst->psg_freq[k] = 0u;` -> `(void)0;` | SURVIVED |  |  |  |
| P03 | 43: `inst->psg_ctl[k] = 0u;` -> `(void)0;` | KILLED |  |  | psg_decode psg_fold psg_pure_function psg_wave_ptr |
| P04 | 45: `inst->psg_widx[k] = 0u;` -> `(void)0;` | KILLED |  |  | psg_decode psg_fold |
| P05 | 49: `inst->psg_select = 0u;` -> `(void)0;` | KILLED |  |  | psg_decode |
| P06 | 55: `= 1u;` -> `= 2u;` | KILLED | noise4 | sweep restore | 5 C-level |
| P07 | 56: `(uint32_t)DS_CONST_psg_lfsr_reset` -> `... + 1u` | KILLED | noise4 | sweep | 1 C-level |
| L01 | 63: `(s >> 11)` -> `(s >> 10)` | KILLED | noise4 | sweep | 1 C-level |
| L02 | 65: `(b << 17)` -> `(b << 16)` | KILLED | noise4 | sweep | 1 C-level |
| N01 | 72: `(post >> 10)` -> `(post >> 9)` | KILLED | noise4 | sweep |  |
| N02 | 72: ` ^ (post >> 17)) & 1u` -> `) & 1u` | KILLED | noise4 | sweep |  |
| Q01 | 78: `31u - (` -> `32u - (` | KILLED | noise4 noise5 | sweep | 3 C-level |
| Q02 | 78: `& 0x1Fu` -> `& 0x0Fu` | KILLED | noise4 noise5 | sweep | 1 C-level |
| Q03 | 80: `64u * nfc` -> `63u * nfc` | KILLED | noise4 noise5 | sweep | 2 C-level |
| Q04 | 80: `nfc != 0u` -> `nfc == 0u` | KILLED (hang) | noise4 noise5 | sweep psg_tone restore | 7 C-level |
| C01 | 86: `& 0xFFFu` -> `& 0x7FFu` | KILLED |  |  | psg_pure_function |
| C02 | 88: `(uint64_t)d * (` -> `(uint64_t)(d + 1u) * (` | KILLED | r7 atten p01 p23 p45 sel0 | sweep golden restore | 4 C-level |
| F01 | 110: `no[1] < no[0]` -> `no[1] <= no[0]` | SURVIVED |  |  |  |
| F02 | 112: `no[n] > t` -> `no[n] >= t` | KILLED | noise4 | sweep | 2 C-level |
| F03 | 115: LFSR step statement deleted | STILLBORN (unused lfsr_step) |  |  |  |
| F03b | 115: the step taken only for n > 9 (never) | KILLED | noise4 noise5 | sweep | 1 C-level |
| F04 | 123: `o <= t` -> `o < t` | KILLED |  | golden | psg_fold psg_lfo_trg psg_wave_ptr |
| F05 | 125: `/ q + 1u` -> `/ q + 0u` | KILLED | dda r7 atten p01 p23 p45 sel0 | sweep golden twin d3 restore | 4 C-level |
| F06 | 127: `(steps & 31u)` -> `(steps & 15u)` | KILLED |  |  | psg_pure_function |
| F07 | 128: `o + steps * q` -> `o + (steps + 1u) * q` | KILLED | dda atten p01 p23 p45 sel0 | sweep golden d3 | 3 C-level |
| F08 | 120: `k < 6u` -> `k < 5u` | KILLED | p45 | sweep golden | 1 C-level |
| A01 | 146: `stamp < base_t` -> `stamp <= base_t` | KILLED |  |  | psg_fold psg_poweron |
| A02 | 149: `base_t + 1u` -> `base_t + 0u` | KILLED | noise4 noise5 r7 atten p01 p23 p45 sel0 | sweep golden restore | 6 C-level |
| A03 | 152: `_per_sample + 1u` -> `_per_sample + 0u` | KILLED | all nine | noninterference sweep repeat golden twin d3 chunk observer restore | 8 C-level |
| A04 | 155: `cursor <= stamp` -> `cursor < stamp` | KILLED | dda r7 p01 p23 sel0 | sweep repeat golden d3 chunk observer restore | 5 C-level |
| A05 | 160: `si - base_t + 1u` -> `si - base_t + 0u` | KILLED | noise4 | sweep golden |  |
| A06 | 163: `cursor + 1u` -> `cursor + 2u` | KILLED | all nine | sweep repeat golden twin d3 chunk observer restore | 5 C-level |
| A07 | 166: the step-end `fold_to` deleted | KILLED | dda r7 atten p01 p23 p45 sel0 | noninterference sweep golden twin d3 observer restore | 10 C-level |
| A08 | 167: `step_end != 0u` -> `step_end == 0u` | KILLED | all nine | sweep golden twin d3 restore | 9 C-level |
| A09 | 170: the channel rebase deleted | KILLED | dda r7 atten p01 p23 p45 sel0 | sweep golden twin d3 | 2 C-level |
| A10 | 174: the noise rebase deleted | KILLED | noise4 noise5 | sweep restore | 4 C-level |
| S01 | 182: `2u * (15u - mal)` -> `1u * (15u - mal)` | KILLED | atten | sweep golden |  |
| S02 | 182: `(31u - al)` -> `(30u - al)` | KILLED | all nine | sweep golden twin d3 | 4 C-level |
| S03 | 182: `(15u - lal)` -> `(14u - lal)` | KILLED | all nine | sweep golden twin d3 | 4 C-level |
| Y01 | 187: `>> 4u) & 0xFu` -> `>> 3u) & 0xFu` | KILLED | atten | sweep golden |  |
| Y02 | 188: `& 0xFu` -> `& 0x7u` | KILLED | atten | sweep golden | 1 C-level |
| Y03 | 199: `(ctl & 0x80u) == 0u` -> `!= 0u` | KILLED | all nine | sweep golden twin d3 | 4 C-level |
| Y04 | 200: `continue;` -> `;` | KILLED | dda noise5 r7 atten p01 p23 p45 sel0 | sweep golden twin d3 |  |
| Y05 | 203: `(ctl & 0x40u) != 0u` -> `== 0u` | KILLED | all nine | sweep golden twin d3 | 4 C-level |
| Y06 | 205: `ch >= 4u` -> `ch > 4u` | KILLED | noise4 noise5 | sweep |  |
| Y07 | 205: `& 0x80u) != 0u` -> `& 0x40u) != 0u` | KILLED | noise4 noise5 | sweep |  |
| Y08 | 206: `) != 0u ?` -> `) == 0u ?` | KILLED | noise4 | sweep |  |
| Y09 | 208: `& 31u)` -> `& 15u)` | KILLED | dda r7 atten p01 p23 p45 sel0 | sweep golden twin d3 |  |
| Y10 | 210: `(bal >> 4u) & 0xFu` -> `bal & 0xFu` | KILLED | atten p01 p23 p45 | sweep golden |  |
| Y11 | 211: left silence at att + 1 | KILLED |  |  | psg_vol_table |
| Y12 | 212: `sum_l += level` -> `sum_l = level` | KILLED | dda | sweep |  |
| Y13 | 214: `ctl & 0x1Fu` -> `ctl & 0x0Fu` | KILLED | atten | sweep golden |  |
| Y14 | 215: right silence at att + 1 | KILLED |  |  | psg_vol_table |
| Y15 | 219: `*l = sum_l;` -> `*l = sum_r;` | STILLBORN (unused sum_l) |  |  |  |
| Y15b | 219: `*l = sum_r + 0u * sum_l;` | KILLED | atten p01 p23 p45 | sweep golden |  |
| R01 | 228: `&& (ctl & 0x40u) == 0u` -> `&& 1` | KILLED | dda | sweep restore | 2 C-level |
| R02 | 228: `ch == 1u` -> `ch == 0u` | KILLED |  |  | psg_fold psg_lfo_trg |
| R03 | 228: `psg_lfo_ctl & 0x80u` -> `& 0x01u` | KILLED |  |  | psg_fold psg_lfo_trg |
| T01 | 237: ` - stamp % d` -> ` - 0u` | KILLED |  |  | psg_fold psg_lfo_trg psg_wave_ptr |
| T02 | 237: `w_off + chan_period` -> `w_off + 1u + chan_period` | KILLED |  | golden restore | psg_fold psg_lfo_trg psg_wave_ptr |
| W01 | 244: `value & ds_psg_reg_mask(reg)` -> `value` | KILLED | p01 p45 | sweep golden | 3 C-level |
| W02 | 248: `post = m;` -> `post = value;` | KILLED | p01 p45 | sweep golden | 3 C-level |
| W03 | 251: `reg >= 10u` -> `reg >= 11u` | KILLED | atten | sweep golden | 1 C-level |
| W04 | 251: `sel > 5u` -> `sel > 4u` | KILLED | noise5 p45 | sweep golden | 1 C-level |
| W05 | 251: `sel < 4u` -> `sel < 5u` | KILLED | noise4 noise5 p45 | sweep golden restore | 2 C-level |
| W06 | 258: `reg <= 7u` -> `reg <= 6u` | KILLED | noise4 noise5 p45 | sweep golden |  |
| W07 | 263: `value & ds_psg_reg_mask(0u)` -> `value & 3u` | KILLED | noise4 noise5 atten p45 | sweep golden twin psg_mod psg_tone restore | 12 C-level |
| W08 | 266: the R1 store deleted | KILLED | all nine | sweep golden twin d3 | 5 C-level |
| W09 | 269: R2 keeps the old low byte (`& 0xFFFu` for `& 0xF00u`) | KILLED | p01 | sweep golden |  |
| W10 | 273: `(m << 8)` -> `(m << 7)` | KILLED | atten p01 p23 p45 sel0 | sweep golden d3 | 4 C-level |
| W11 | 280: the R4 store deleted | KILLED | all nine | sweep golden twin d3 restore | 8 C-level |
| W12 | 281: `(m & 0x40u) != 0u` -> `== 0u` | KILLED | dda atten p01 p45 sel0 | sweep golden d3 | 1 C-level |
| W13 | 282: the DDA index reset deleted | KILLED | dda atten p01 p45 | sweep golden restore | 1 C-level |
| W14 | 286: the freeze deleted | KILLED | dda atten p01 p45 | sweep golden twin restore | 2 C-level |
| W15 | 287: `was == 0u &&` -> `was != 0u &&` | KILLED | dda r7 atten p01 p23 p45 sel0 | sweep golden twin d3 restore | 5 C-level |
| W16 | 287: DDA release restart off | KILLED | dda | sweep | 1 C-level |
| W17 | 293: the R5 store deleted | KILLED | all nine | sweep golden twin d3 | 5 C-level |
| W18 | 298: `(ctl & 0x40u) != 0u` -> `== 0u` | KILLED | all nine | sweep golden twin d3 | 6 C-level |
| W19 | 299: the latch store deleted | KILLED | dda | sweep | 2 C-level |
| W20 | 301: `(ctl & 0x80u) == 0u` -> `!= 0u` | KILLED | all nine | sweep golden twin d3 | 3 C-level |
| W21 | 302: the wave store deleted | KILLED | all nine | sweep golden twin d3 | 5 C-level |
| W22 | 303: `+ 1u) & 31u` -> `+ 2u) & 31u` | KILLED | all nine | sweep golden twin d3 | 2 C-level |
| W23 | 305: write-when-on off | KILLED |  |  | psg_fold psg_wave_ptr |
| W24 | 306: the held store deleted | KILLED |  |  | psg_wave_ptr |
| W25 | 312: the R7 store deleted | KILLED | noise4 noise5 | sweep restore | 2 C-level |
| W26 | 315: the R8 store deleted | KILLED |  |  | psg_decode |
| W27 | 321: the R9 store deleted | KILLED |  |  | psg_decode psg_fold psg_lfo_trg |
| W28 | 322: `(m & 0x80u) != 0u` -> `== 0u` | KILLED | p01 | sweep golden | 2 C-level |
| W29 | 323: the TRG index reset deleted | KILLED | p01 | sweep golden | 1 C-level |
| W30 | 324: the TRG freeze deleted | KILLED |  |  | psg_fold psg_lfo_trg |
| W31 | 326: `trg_was != 0u` -> `== 0u` | KILLED |  |  | psg_fold psg_lfo_trg |
| W32 | 329: `(m & 0x03u)` -> `(m & 0x02u)` | KILLED |  |  | psg_lfo_trg |
| W33 | 337: `chan <= 5u` -> `chan < 5u` | KILLED | noise5 p45 | sweep golden |  |
| W34 | 338: `(uint16_t)post` -> `(uint16_t)m` | STILLBORN (post unused) |  |  |  |
| W34b | 338: `(uint16_t)(m + 0u * post)` | KILLED | atten p01 p45 sel0 | sweep golden d3 | 1 C-level |

"all nine" means sel0, p01, p23, p45, atten, noise4, noise5, r7 and dda.

Survivors and their dispositions. None needs a row, so trigger (1) gets no revision plan:
- **K12, equivalent.** reg_mask[15] is 0, so `reg < 15u` returns 0 for reg 15 either way.
- **P02, equivalent in the core.** boot.c clears the whole instance before ds_psg_poweron, which is the only core caller, and no policy draws psg_freq. Only the C-level pk_reset reuses a dirty instance, and no case reads F after it.
- **F01, equivalent.** At a tie the two noise events touch only their own counter and LFSR, so their order changes no state.

Killed only at C level or by E0 guest cases (no guest literal scene turned). Each is already a named RULES row or limit, or 05-09 scope:
- K03, K04, C01 (stored bits of R2 and R3, F of $800 or more; see LED-AUD-0003 above).
- K09, K10, W26, W27, W30, W31, W32, R02, R03 (R8 and R9 storage, TRG and CTL: LFO, 05-09).
- P01, P03, P04, P05 (power-on statements reached only through pk_reset; equivalent in the boot path).
- F04, T01, T02, A01 (the tie order, the key-on phase and the fold edge: RULES d185-c, aud12-f and aud04-e, ASSUMED, no MAN row owed).
- F06 (more than 15 steps in one fold needs F of 6 or less).
- Y11, Y14 (silence at att 29: an audible-below-sum-15 converse held ASSUMED by ER-16; psg_vol_table checks att 29, RULES aud09-c).
- W23, W24 (an R6 write while playing: the disputed point "held", RULES aud04-d psg_wave_ptr).

## Live budget table (after 05-08)

| Budget | Value after 05-08 | Phase 5 target |
|---|---|---|
| Gate stages | 58 | 58 |
| Selftest PASS lines | 207 (206 rows) | >= 178 |
| Constants rows / CROWS_MAX | 49 / 64 | 49 / 64 |
| Withheld ids | 42 | 42 |
| Regions | 24 | 24 |
| OX_ROMS_MAX / guest ROMs | 48 / 38 | 48 / 39 |
| DSV ids | 27, 28 used | 27, 28, 29 |
| Toys | 32 | 33 (32 after a cut) |
| Conformance REQUIRED cases | 388 | about 379 to 394 |
| Supported ledger entries | 14 (13 + LED-AUD-0008) | at most 4 new |
| .aexp bytes (psg_tone / psg_mod) | 40,664 / 13,105 | below 49,152 |
| Handbook bytes | 25,592 | at most 25,600 |
| Local warm gate user CPU | 316.8 to 357.6 s: +20.0 / +32.4 percent in two pairs; instructions +22.2 / +25.2 percent | +10 to +20 percent; trigger (3) over +25 percent |
| CI critical path | unverified; estimate 525 to 670 s | job trigger 600 s |

## Task Commits

1. Task 1: the checkpoint A runs and the cost against D-214, `fec4cd8` (docs)
2. Task 2: the LED-AUD-0008 promotion, its mutual pin and the README list, `1e3a6e1` (feat)
3. Task 3: the cut-line record and the handbook update, `33cba95` (docs)

## Deviations from Plan

**1. [Rule 3, files] tests/conformance/cases/audio_tone_atten.toml is outside files_modified.** The plan's action needs a TEST: pin whose case names the entry in pins (INV_PIN_BACKLINK). Only `pins = ["LED-AUD-0008"]` changed.

**2. [Rule 1, evidence] Trigger (3) changed after Task 1 was committed.** Task 1 recorded pair 1 (+20.0 percent, under the trigger). The Task 2 gate then read 357.6 s, so a second pair and instruction counts were taken. Task 3 records them and points the Task 1 paragraph to them (fix forward, no history rewrite). The trigger reads borderline, not "not fired".

**3. [Process] The machine restarted during the first attempt, before any commit.** All gates were rerun from the start. The first dash run rebuilt the stale tree and overlapped the cold warm-up of the baseline clone for about 4 minutes; its time is recorded as contended.

**4. [Process] Gate overlaps.**
- No two gates ran at once in this checkout.
- The baseline clone, the second pair and the instruction counts ran in separate scratch checkouts.
- The scratch sweep ran beside linux-repro, which runs on a read-only copy taken at its start.
- The narrow --stage verifies ran only after linux-repro finished.

**5. [Plan text] The cut decision is the orchestrator's.** As instructed, this plan records the triggers and a recommendation and does not decide. The handbook says "Recommendation: cut (LFO to 5.1); the orchestrator decides".

State and requirements: the plan-complete step ran through gsd-tools. git diff .planning/REQUIREMENTS.md was checked after it, and any AUD tick was reverted (05-10 completes them). `.planning/state.json` was restored to its original bytes and is not committed.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model:
- T-05-29: the per-clause table above. LED-AUD-0003 and -0005 are held on recorded clauses. DOCS_SUPPORTED_LIST, FXL_OCR_BACKS_SUPPORTED and INV_PIN_BACKLINK passed in the full gate.
- T-05-30: no check, control or golden changed, and every survivor is listed.

## Notes for next plans

- **The orchestrator:** decide the cut on the record. If cutting, insert Phase 5.1 with gsd-tools: 05-09 becomes 5.1-01, 05-10 runs next with the LFO under DSPE UNMODELLED, ROADMAP criterion 1 gains the LFO sentence, and the toy count stays 32. The 05-10 plan text then needs its LFO rows, LED-AUD-0002 and RULES later:05-09 lines re-targeted to 5.1.
- **05-09 (or 5.1-01):**
  - The sweep shows R8, R9 CTL and TRG paths killed only at C level today (K09, K10, W26 to W32, R02, R03). psg_lfo's guest rows should turn them red; re-running those mutants against psg_lfo is a cheap check.
  - LED-AUD-0002's placement clause needs its guest row.
- **05-10:**
  - LED-AUD-0005 becomes promotable after narrowing clause (c) to chON 1 and DDA 1 and splitting d_hold ordinal 1 into an ASSUMED latch_off row.
  - LED-AUD-0003 needs guest rows for R2 bits 1-0 and an F of $800 or more, and a statement decision on its disputed and converse clauses.
  - Re-measure trigger (3)'s quantity with the instruction-count method; it does not depend on the load.
  - The README's Phase 5 paragraph is 05-10's; this plan added only the supported list entry and the D-218 sentence.
- **CI:** the first Phase 5 CI run should be read against the 600 s job trigger with lever (a)'s CPU line.

## Owner items

- None new.
- Carried: lever (a), commit 9619efe, awaits the owner's yes or no at publish time (05-01).
- Push and pull-request authorisation for the first Phase 5 CI run (D-64).

## Self-Check: PASSED

docs/ci-timings.md, LED-AUD-0008.toml, README.md, audio_tone_atten.toml, the handbook and this SUMMARY exist. Commits fec4cd8, 1e3a6e1 and 33cba95 are ancestors of HEAD.
