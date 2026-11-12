---
phase: 04-vce-vdc-raw-video
plan: 07
subsystem: video-judge-vexp-fault-sweep-hash-role
tags: [video-judge, vexp, rows-rel-struct, fault-sweep, golden-role, hash-not-verdict, video-check, selftest-mutations, null-renderer, w3]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the video kind and the vid and dev sinks (04-04), the folded grid and the observer, restore and chunk matrices of video (04-05), the live VDC register interface and the vdc_reg guest (04-06), the toys vid-palette-mask and vid-emit-mutates (04-03, 04-05)
provides:
  - fixtures/src/vce_pal.vexp - the hand-derived literal rows of the vce_pal scene (groups struct, frame, div, divline, border, dirty, each with a class and a kills list) plus the golden scene row and the pinned digest; scripts/fixture-build writes it as build/gate/fixtures/vce-pal.vexp.tsv (write_vexp, exit 2 on a malformed row, never
    an empty file); the runner takes no new argument
  - the video judge in tests/conformance/videokinds.c - four row kinds (scene, group, row, golden) read strictly (T-04-24), ROWS (index and value lists, COUNT*V items, the non-vacuity rule), REL (scroll_x_shift, scroll_y_shift, palette_subst, blank_is_border, dot_width_scale on two named runs) and STRUCT (12 checks) forms, a case outcome
    that is the conjunction of its groups, class and form counts in the evidence text
  - the record faults F0..F9 and the sweep (kinds video_fault_sweep, video_hash_not_verdict, video_judge_forms) and the golden role with the runtime refusal video-hash-only; a vid sink that was never written is a harness fault
  - scripts/video-check (two stages: lib/vexp-lex.awk, lib/vexp-rules.awk), the stage video-check (ONE stage, 57 stages, F-42 ruling), 10 mini-roots under tests/video, tests/conformance/VIDEO_TOY_EXEMPT, and the selftests mutation-video-hash-only, mutation-video-group-vacuous, mutation-vexp-class-lint (two cases), mutation-video-rules
    (five cases) and three awk-exit-status cases
affects: [04-08, 04-09, 04-10, 04-11, 04-12, 04-13, 04-16, 04-17]

requirements-advanced: [VID-06]
requirements-completed: []  # VID-06 completes in 04-16

actuals:
  tokens: 44300
  tasks: 3
  commits: 4
plan_head_before: 3d6edc8adcf3c48d85ae115c4577385977040aa2
plan_head_after: 574db2a4dd7628517a82afcf0a2fe1c1febdccd9

tech-stack:
  added: []
  patterns:
    - "A literal channel in two hops: fixtures/src/NAME.vexp (hand-derived rows, comments carry the derivation) -> build/gate/fixtures/NAME.vexp.tsv (fixture-build) -> the runner, which reads it by a relative path like NAME.classes; the case keeps only rom, id and scene"
    - "A fault is applied at the record the group looks at first (its focus), to a COPY of the captured records: a changed record is a scratch image, a dropped, duplicated or swapped line changes only a pointer array, the capture is never touched"
    - "A mini-root needs no toy registry unless it carries tests/conformance/VIDEO_TOY_EXEMPT or tests/controls: the toy cross-rules run only where a registry exists, so each negative root gives exactly one code"
    - "Every enforcement statement of the rules file is one tagged line (# rule-NAME) inside braces, so a selftest can delete the tagged lines of a scratch copy without breaking the awk"

key-files:
  created:
    - fixtures/src/vce_pal.vexp
    - scripts/video-check
    - scripts/lib/vexp-lex.awk
    - scripts/lib/vexp-rules.awk
    - tests/conformance/VIDEO_TOY_EXEMPT
    - tests/conformance/cases/video_fault_sweep_vce_pal.toml
    - tests/conformance/cases/video_vce_pal_golden.toml
    - tests/conformance/cases/video_hash_not_verdict.toml
    - tests/conformance/cases/video_judge_forms.toml
    - tests/video/ (10 mini-roots: ok-video, hash-only, group-unkilled, group-vacuous, class-bad, asym-man, unknown-key, rows-vacuous, scene-untoyed, toy-no-literal)
  modified:
    - scripts/fixture-build
    - scripts/check
    - tests/conformance/videokinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/video_vce_pal.toml

key-decisions:
  - "F-42 ruling: ONE stage. stage_video_check runs the checker on the repository, then run_controls over tests/video, and returns the worse code; the gate has 57 stages (a pair would have given 58)"
  - "A group has ONE class, so the plan's struct group (ASSUMED LED-VID-0004 for count and origins, ASSUMED LED-VID-0001 where frame_lines enters) is two groups: struct (count, origins, rec_bytes, values_below_512; LED-VID-0004) and frame (frame_lines, records_per_frame; LED-VID-0001). The six groups are struct, frame, div, divline, border, dirty"
  - "Faults act at a focus, the record of the first row of the group (n / 2 for a struct row, the first judged record of a window, the first record after a write). F4 inserts a copy of the record BEFORE the focus so that the focus position holds the previous line (a copy of the focus itself would change no row that reads it); F0 raises
    the first number of the first row (for a ROWS row the first expected value)"
  - "The STRUCT list grew by editing the rules file, as the plan allows: div_first (the first record after a CR write carries the new divider), border_after_pal, line_dirty, beyond the nine of the plan; display_lines and width_set are judged by video_judge_forms only (vce_pal has no active window yet)"
  - "The grid-dependent checks (count, origins, frame_lines, records_per_frame, display_lines, div_first, line_dirty) must sit in an ASSUMED group: the ER-10 rule is a lint (VEXP_CLASS), tagged with the class lint, not only a comment"
  - "The toy cross-rules (VIDEO_SCENE_UNTOYED, INV_TOY_NO_LITERAL) are active only where the root carries tests/conformance/VIDEO_TOY_EXEMPT or tests/controls. The repository has both; a negative mini-root without either runs only the .vexp rules, so group-unkilled and the others give one code each"
  - "The golden digest 6c510913928bfa8703e0f5006bba728f894ccf9ce910ede9b7fcef50afd09c55 was measured on the first run and recomputed outside the core with shasum -a 256 over build/gate/video/video_vce_pal.dsvr (the --video-out file); E0 repeatability, valid only beside video_vce_pal"

completed: 2026-11-12
---

# Phase 4 Plan 07: Video judge Summary

**The literals of the vce_pal video scene now live in one hand-written file and reach the runner through fixture-build; the judge evaluates them by group (ROWS, REL and STRUCT, each with a class), every group is shown able to fail under the record faults it declares, a stream hash only counts beside a literal case of the same scene
  (refused statically and at run time), and a static stage with ten mini-roots and nine selftest mutations keeps each of those rules load-bearing.**

## What was built

- **Task 1, tracer (commit 496424e).** `fixtures/src/vce_pal.vexp` holds the six groups (struct, frame, div, divline, border, dirty) with the derivations in comments (91 * 256 + 17 = 23313 = 45 * 512 + 273 for the border; the div window rule and amendment A11). `scripts/fixture-build` gained `write_vexp` (the `write_classes` pattern: awk
  status tested, exit 2 on a malformed row, never an empty file). `videokinds.c` was reorganised around `vk_capture` (boot, run, sinks, walk, dev scan) and a group evaluator; `kind_video` reads `vce-pal.vexp.tsv`, judges every group of the scene, prints the class counts and the form counts and `video_sha256=... E0 repeatability` beside
  the verdict. The expect of `video_vce_pal` shrank to `rom`, `id`, `scene`. The palette toy still turns it red (CONTROL vid-palette-mask RED_OK). The full gate was green on the committed tree before Task 2 started.
- **Task 2 (commit 9f98e2b).** `scripts/video-check` (POSIX sh, two awk stages with sentinels, `--root`, `--expect`, `--cites`, `--ledger`, exit 0/1/2), `scripts/lib/vexp-lex.awk` (512 rows, 400 characters per line, 16 KiB, ASCII, SPDX pair) and `scripts/lib/vexp-rules.awk` (closed grammar, class lint against the img rows of cites.txt
  and the ledger ids, the GUEST allowlist with `pal_bijection` only, ER-12, role rules, kill rules, the form-by-fault table, the ROWS non-vacuity lint, the toy cross-rules). Ten mini-roots under `tests/video`, `tests/conformance/VIDEO_TOY_EXEMPT`, the stage registered after `fixture-lint-negatives`, and the selftests below.
- **Task 3 (commits a5b3f4c and 574db2a).** The faults F0..F9, `vk_sweep_group`, the golden role (`vk_golden_judge`), and the kinds `video_fault_sweep`, `video_hash_not_verdict` and `video_judge_forms`; four cases, four REQUIRED ids, the golden scene row and digest in the `.vexp`, the exempt line for the golden case. The second commit
  sizes two message buffers after the gcc 13 run (deviation 3).

## Verification

- Task verifies, run as written (in pieces and again inside the full gate): Task 1 (`fixture-build`, `test -s vce-pal.vexp.tsv`, `conformance`, `RESULT video_vce_pal pass`, `CONTROL vid-palette-mask RED_OK`), Task 2 (`video-check`, the three `--selftest mutation-*` names, `licences`), Task 3 (`conformance`, the four RESULT greps,
  `video-check`): all PASS. Full `sh scripts/check` PASS (57 stages) before each commit; `sh scripts/check --selftest` PASS (140 SELFTEST lines, no FAIL); `sh scripts/audit-history --range 3d6edc8..HEAD` clean (3 commits scanned at that moment, the fix commit followed).
- **Linux:** `LINUX_REPRO_PARTS=gate-gcc` first FAILED (two `-Wformat-truncation` errors in the new sweep code, macOS never showed them), then PASSED after the fix (gcc 13, strict C11, linux/amd64, 54 s). **Wave W3 end:** `LINUX_REPRO_PARTS='gate-gcc gate-clang core-boundary-gcc'` PASS (all three parts). One-off local runs, not CI
  claims. The mawk and gawk whole-gate parts were not run (the orchestrator named three parts); instead `video-check`, its ten mini-roots and the nine mutation selftests were run in the same Ubuntu 24.04 image under mawk and under gawk, all PASS. `dash scripts/check` and the second-awk whole gate are owed at the Phase 4 checkpoint and close-out.
- **Mini-roots (each gives exactly its code):** hash-only INV_VIDEO_HASH_ONLY, group-unkilled INV_VIDEO_GROUP_UNKILLED, group-vacuous VIDEO_GROUP_VACUOUS, class-bad VEXP_CLASS (three bad classes), asym-man VEXP_ASYM_CLASS, unknown-key VEXP_UNKNOWN_KEY, rows-vacuous VIDEO_ROWS_VACUOUS, scene-untoyed VIDEO_SCENE_UNTOYED, toy-no-literal INV_TOY_NO_LITERAL, ok-video passes.
- **Selftests:** `mutation-video-hash-only`, `mutation-video-group-vacuous`, `mutation-vexp-class-lint` (cases class-bad and asym-man) and, beyond the plan, `mutation-video-rules` (group-unkilled, unknown-key, rows-vacuous, scene-untoyed, toy-no-literal): deleting the tagged lines of a scratch copy turns each control from met to exit 1.
  Three `awk-exit-status` cases prove a failed awk stage of video-check is exit 2.
- **Literals and controls shown able to fail (ER-11), run on scratch copies of the built rows and restored byte for byte:** a border group that declares F5 (a fault that cannot flip it) gives guest_failure `VIDEO_GROUP_VACUOUS: F5 ... does not flip group border` from the sweep; a wrong golden digest gives guest_failure on
  `video_vce_pal_golden` while `video_hash_not_verdict` stays pass; the literal scene row made a struct-role row gives harness_failure `video-hash-only` on the golden case. `video_judge_forms` (77 checks) shows each form and each closed check both ways and the null renderers (no record, all border, all index 0) never passing.
- Evidence class E0 throughout. The classes of the rows are in the `.vexp` file and in the evidence text: div MAN H6-3 (judged only a whole line from the write that sets it, A11), border GUEST pal_bijection, struct, frame, divline and dirty ASSUMED (LED-VID-0004, LED-VID-0001). No accuracy, compatibility or hardware claim; a pass is not
  a support claim, and the pinned digest is a change detector.

## Deviations from Plan

1. **[Note] The plan's struct group became two groups.** One group has one class; the plan wants LED-VID-0004 for count and origins and LED-VID-0001 where frame_lines enters, so `frame` (frame_lines, records_per_frame) is its own group (kills F0, F4). The plan's struct kills F4, F8, F9 are unchanged. Six groups instead of five.
2. **[Note] The closed STRUCT list is longer than the plan's nine** (allowed: "more only by editing the rules file"): `div_first` for divline, `border_after_pal` for border and `line_dirty` for dirty, because those three are not in the plan's list and are not ROWS or REL. `display_lines` and `width_set` are implemented and judged only by `video_judge_forms`.
3. **[Rule 1 - Bug] Two `-Wformat-truncation` errors on gcc 13** (`vk_sweep_group` copied a 240-byte text into a 200-byte field, and the sweep message buffer was 320). Found by `LINUX_REPRO_PARTS=gate-gcc`, fixed in commit 574db2a (400 and 440 bytes), gate-gcc PASS. macOS clang did not warn.
4. **[Note] The toy cross-rules run only where the root has a toy registry** (VIDEO_TOY_EXEMPT or tests/controls). The plan lists no VIDEO_TOY_EXEMPT file for the seven negative mini-roots; without this rule each would also give VIDEO_SCENE_UNTOYED.
5. **[Note] Files beyond the plan list:** `tests/video/scene-untoyed/tests/conformance/VIDEO_TOY_EXEMPT` (an empty list with its format comment; scene-untoyed needs a registry to be untoyed), `scripts/lib` and the `--cites`/`--ledger` options of `scripts/video-check` (the selftests copy scripts/ so the registry fallback of the repository
  is passed explicitly, as fixture-lint's selftests do). `tests/conformance/cases/video_vce_pal.toml` is in the plan list and changed (expect and comments).
6. **[Note] More mutation selftests than the plan names:** the brief asks for a selftest for every enforcement rule, so `mutation-video-rules` covers five more tagged rules and the class lint has a second case (ER-12). All use the one tagged-line mechanism.
7. **[Note] Faults need a focus and F4 differs from the plain wording.** "Duplicate a line" inserts a copy of the record before the focus (see key decisions); F1 F2 F3 F6 F7 act on pixels and do not change header-only records, so no vce_pal group declares them, and `video_judge_forms` shows them flip a ROWS group built by hand (a group
  left green by a declared fault is reported vacuous there too).
8. **[Note] `video_judge_forms` omits F0 for `values_below_512`:** F0 raises the limit 512 to 513, which every value below 512 still satisfies, so that check is live through its wrong literal, not through F0.
9. **[Note] The sweep applies faults to the LAST run of the case** (the variant of a REL pair); the vce_pal scene has one run. A multi-run scene needs the runs of its case built by its own kind.
10. **[Note] Plan estimate was 120,000 tokens; the realized diff is about 44,300** by chars/4 over the added lines (39 files, 3,426 insertions, 282 deletions, the vce_pal records and the golden digest included).

## Known Stubs

None. The ROWS and REL forms are not used by the vce_pal scene (its records are header-only until 04-09); they are exercised by `video_judge_forms` and by the `ok-video` mini-root, and the first scene that uses them (04-09) must give each such group a kills list that the sweep can flip.

## Threat Flags

None. T-04-24 (`.vexp` parsing) is mitigated by one strict lexer with limits, a closed key set that fails closed in awk and again in the runner (an unknown row, key, check or relation is a harness fault), and `write_vexp` exiting 2; T-04-25 (a golden used as a verdict) by INV_VIDEO_HASH_ONLY, the runtime refusal `video-hash-only` and
  `video_hash_not_verdict`; T-04-26 (vacuous literal groups) by the sweep, VIDEO_GROUP_VACUOUS statically and at run time, the ROWS non-vacuity lint, the null-renderer rows and the selftests; T-04-27 (class inflation) by the class lint against cites.txt and the ledger and by ER-12.

## Notes for next plans

- **Scenes.** A new scene adds `fixtures/src/NAME.vexp` (a stem with a `.vexp` gets `build/gate/fixtures/NAME.vexp.tsv`); its cases need `expect = "rom=NAME;id=N;scene=SCENE"`, a scene row per case, a toy that lists the literal case (or a VIDEO_TOY_EXEMPT line), a golden scene row beside a literal case if a golden is wanted, and a fault
  sweep case (kind `video_fault_sweep`, same expect). Every group declares kills that the sweep can flip; a group whose only kill is F0 takes its liveness from a toy, said in its comment (A7).
- **Re-pinning.** `video_vce_pal_golden` pins the stream digest of vce_pal. 04-08 and 04-09 change the records (flags, active lines): repin it independently (`shasum -a 256` of `build/gate/video/video_vce_pal.dsvr`) in the same commit as the change. The F5 fault of `video_hash_not_verdict` needs a group with the id `div` in the scene.
- **Toy EXPECT second column (F-4, 04-17).** `scripts/video-check` already accepts a second column `pass` and counts such a line neither as toy coverage nor as the literal case of INV_TOY_NO_LITERAL; 04-17 need not edit it.
- **Adding to the closed lists.** A STRUCT check is added in two places that must agree: the `ck[]` table of `vexp-rules.awk` and the `vk_checks[]` table of `videokinds.c`; a REL relation in `rk[]` and `vk_rel_row`; a GUEST model in `guest_ok[]` with a one-line justification. The three mini-root classes of the selftests need no change.
- **Counting rule.** The kinds that parse the record (`vk_*` in videokinds.c) share `vk_count_closed_form` with `ob_lines_due`, `cs_lines_due` and `vd_lines_due` (04-05 note); the STRUCT count row takes its grid from the `.vexp` literal and is the independent restatement.
- **Cost.** The four new cases add about 5 s to `conformance` and little to the sanitizers stage (the sweep and the hash kind each rerun the guest once); `conformance` stays near 50 s on this machine (trigger 60 s).
- **Owed:** `dash scripts/check` and the second-awk whole-gate runs (mawk, gawk parts) at the Phase 4 checkpoint and close-out.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: fixtures/src/vce_pal.vexp, scripts/video-check, scripts/lib/vexp-lex.awk, scripts/lib/vexp-rules.awk, tests/conformance/VIDEO_TOY_EXEMPT, tests/conformance/cases/video_fault_sweep_vce_pal.toml, video_vce_pal_golden.toml, video_hash_not_verdict.toml, video_judge_forms.toml, the ten directories of tests/video, REQUIRED lines for the four new cases.
- FOUND commits 496424e, 9f98e2b, a5b3f4c and 574db2a (ancestors of HEAD).
