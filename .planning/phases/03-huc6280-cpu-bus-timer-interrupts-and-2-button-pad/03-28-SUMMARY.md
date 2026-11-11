---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 28
subsystem: w6-input-check-and-input-record-table
tags: [input-check, input-rules, inp-codes, negative-controls, selftest, record-format-5, input-table, schedule-identity, sha256, w6, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: scripts/lib/input-rules.awk and conformance-flatten --inputs (03-27), the instance schedule input_ev/input_count/input_reserved (03-27), the record table pattern and the four pinned places of the record layout (03-18, 03-22, 03-23), the CR-01 rule (03-37 fixture-lint)
provides:
  - "scripts/input-check - the input script checker: two stages per script (toml-subset.awk then input-rules.awk, the same rules file conformance-flatten --inputs runs), each to a file with its status tested, END sentinel from stage 1 required by stage 2, INP_STAGE_DEAD (exit 2) for a dead stage, --root and --expect, STATUS input-check ok n=N"
  - "scripts/lib/input-rules.awk completed: every reason code with a # rule-NAME tagged statement; new codes INP_PORT_EMPTY, INP_HELD_OPPOSITE, INP_HELD_RESERVED; INP_TOO_MANY also for more than 1024 event tables; stage 2 prints no RULES row and exits 3 without stage 1's END row"
  - "stages input-check (static, before the build) and input-check-negatives (run_controls --ok-prefix --require-negative over tests/gate/input-check, 22 mini-roots: ok-scripts and 21 negatives, one per code plus stage 1 pass-through, the file name and the 64 KiB cap)"
  - "selftests mutation-input-order, mutation-input-rules (one line per control, 21), mutation-input-check-dead-stage (stage 1, stage 2 and the sentinel required by input-rules.awk), mutation-input-flatten-refuse (deleting rule-at-order in the flatten copy makes flatten accept the at-decreasing script); awk-exit-status cases input-check-stage1, -stage2 and -rows"
  - "the [input] record table between [stimulus] and [constants]: events and schedule_sha256 (sha256 of the canonical schedule encoding, one line AT,PORT,MASK in decimal and one LF per event; a no-input run is the explicit empty script with the sha256 of zero bytes), DS_TEXT_FORMAT_VERSION 5, no ledger id printed ([withheld] stays 24 of 24)"
affects: [03-29, 03-30, 03-31, 03-32, 03-34]

actuals:
  tokens: 48600
  tasks: 3
  commits: 3
plan_head_before: 29e8b83252f623901fe374afd452de270d7b12bf
plan_head_after: 2dc7ffdca8458ed2d0044c756235aab72dbd5f72

tech-stack:
  added: []
  patterns:
    - "one rules file judges a script twice: in the gate stage (input-check) and on its way to the runner (conformance-flatten --inputs); a selftest deletes the tagged statement in a copy used by flatten and shows the refusal disappear, so a flatten with its own rules or none would not be flipped"
    - "a stage-2 awk program refuses a stage-1 file without its END row itself (no closing row, exit 3), not only the shell around it, so a dead stage 1 cannot read as a clean script"
    - "a record table that names the input of a run holds a count and a sha256 over a documented text encoding that a host tool can recompute (printf and shasum -a 256), so the pinned literal of the REQUIRED case does not come from the printer"
    - "a mini-root with a size cap violation is one real 64 KiB file of comment lines (compresses to a few hundred bytes in git) rather than a test-only cap parameter, so the shipped cap is the one under test"

key-files:
  created:
    - scripts/input-check
    - tests/gate/input-check (ok-scripts with ok.toml and empty.toml, 21 negative mini-roots)
    - tests/conformance/cases/record_input_shown.toml
    - tests/gate/record-lint/missing-input-table
    - tests/gate/record-lint/input-bad-schedule-hash
    - tests/gate/record-lint/input-events-over-cap
  modified:
    - scripts/lib/input-rules.awk
    - scripts/check
    - src/text/text.c
    - src/text/ds_text.h
    - scripts/lib/record-rules.awk
    - tests/conformance/reckinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - tests/gate/record-lint (ten existing records)
    - tests/gate/determinism (four records)
    - tests/gate/flatten-input/ok-script/a.toml

key-decisions:
  - "Codes added to input-rules.awk: INP_PORT_EMPTY (port 2 to 5 while the profile populates port 1 only, D-116; one variable pad_ports holds the populated ports, so the day a profile populates more it is one line and the core keeps its own slot_type), INP_HELD_OPPOSITE (up with down, left with right), INP_HELD_RESERVED (iii, iv, v, vi: the reserved bits of a two-button pad, judged before the unknown-name rule so the diagnosis is specific). A port outside 1 to 5 stays INP_PORT_RANGE."
  - "Because the same rules judge flatten, an event on port 2 to 5 is refused on the way to the runner as well; the ok-script control of 03-27 used port 2 for an equal-time pair and was changed to port 1 at a later time. An equal-time pair with an increasing port can therefore only occur once a profile populates a second port; the rule is covered by the negative (equal at, same port)."
  - "INP_TOO_MANY: a declared events above 1024 reports the cap and suppresses the count-versus-tables comparison (one fact, one code); 1025 or more event tables also report it (checked ad hoc with a 1025-table file, not committed). BAD_FILENAME (an input script name is [a-z][a-z0-9_]*) is judged by input-check as well, with a mini-root, so the gate stage cannot accept a name flatten refuses."
  - "[input] keys: events (decimal, at most 1024) and schedule_sha256 (64 lowercase hex). Canonical encoding for the identity: for each event in slot order the text AT,PORT,MASK in plain decimal and one LF, nothing else; the empty schedule hashes zero bytes (e3b0c442...). The encoding is textual, not a struct image, so it does not depend on the layout of struct ds_input_event and a host can recompute it. The printer validates the schedule as ds_input_push would (count, reserved word, port 1 to 5, mask up to bit 7 and no opposite directions, order, zero unused slots) and a violation is a state error."
  - "The [input] table prints no ledger id, so the [withheld] ids line keeps 24 of 24 and no one-id-per-row layout change was needed here (the standing note is not triggered by this plan); the first plan that adds an id still must do it."
  - "record_input_shown uses kind record_lines with the case key input (the existing apply_input hook is called after the irq stimulus in kind_record_lines): a synthetic image with the four-event pad_2btn_correct schedule; the kind also rebuilds the encoding with snprintf from the parsed events (another formatting path, same hash code) and compares."

requirements-advanced: [INP-02, INP-04]
requirements-completed: []  # none: INP-02 still needs the deterministic-application evidence (input_schedule_chunking, plan 03-29) and INP-04 the IOPT and SCHD digest regions and the none/wrong controls (03-29); this plan delivers the strict script format with a control per reason code and the identity of the input in every record

duration: 70min
completed: 2026-11-11
---

# Phase 3 Plan 28: input-check and the [input] record table Summary

**scripts/input-check judges every input script in two controlled stages with the same input-rules.awk that conformance-flatten runs, 21 negative mini-roots turn one code each red and 24 selftest lines prove each tagged rule, each stage and the flatten coupling removable and detected, and every run record now names its input: [input] events and the sha256 of the canonical AT,PORT,MASK schedule encoding (record format version 5).**

## What was built

- **Task 1 (tracer), commit f7c0bd0.** `scripts/input-check [--expect CODES] [--root DIR]` (POSIX sh): per script a size check (INP_TOO_LARGE above 64 KiB, before stage 1), stage 1 `toml-subset.awk` to a file (status tested, closing END row required), an ERR row of stage 1 is a violation of the file, stage 2 `input-rules.awk` to a file (status tested, closing RULES row required). A dead stage is INP_STAGE_DEAD with exit 2, never a pass; no
  script at all is exit 2. No pipeline has awk on its left. `input-rules.awk` was completed in the same commit (all tags, the three new codes, the END-row requirement), because the tracer must be production quality and the rules are one file; the ok-scripts mini-root (ok.toml with the 0 and 15-digit boundaries and eight buttons, empty.toml, an explicit empty script) and at-decreasing were added, the stages `input-check` (static, after `fixture-lint-negatives`)
  and `input-check-negatives` registered, and the 03-27 flatten ok-script moved off the now-empty port 2. The tracer gate ran twice before expanding: `Tracer verified end-to-end - expanding`.
- **Task 2, commit 507385c.** 19 further mini-roots (21 negatives in all, one per code): bad-format, bad-time-unit, events-mismatch, bad-table-name, at-not-decimal, at-too-long, at-leading-zero, at-decreasing, equal-at-port-order, port-range, port-empty, held-unknown, held-order, held-opposite (both pairs in one script), held-reserved, too-many-events, too-large (65539 bytes of comment lines around a valid script), bad-filename,
  stage1-syntax, key-unknown, key-missing. Produced by one throwaway generator kept outside the repo. Selftests: `mutation-input-order` (the plan's), `mutation-input-rules` (CASE:TAG:FILE table, one line per control, 21 lines; each deletes the tagged lines in a scratch copy and requires the control to flip), `mutation-input-check-dead-stage` (emptied toml-subset.awk, emptied input-rules.awk, and
  input-rules.awk fed a stage 1 file with no END row must print no RULES row and fail), `mutation-input-flatten-refuse` (the flatten copy refuses the at-decreasing script with INP_ORDER and not one byte on stdout, and accepts it with an END row once rule-at-order is deleted from its rules copy). Three awk sites of input-check joined `awk-exit-status` (a shim that makes one awk call exit 2 must give exit 2).
- **Task 3, commit 2dc7ffd.** `emit_input` in text.c after `emit_stimulus`; `DS_TEXT_FORMAT_VERSION` 5; the four pinned places (F10): the printer, record-rules.awk (sequence, keys, value rules REC_TABLES), reckinds.c (expected keys, the row count 68 plus per-event, the refusal-prefix literal, `check_input_rows`, eleven printer state-error controls) and the goldens and mini-root records (both goldens, ten record-lint records, four determinism records: all
  gain `[input] events = 0` and the sha256 of zero bytes, written from the layout, the empty identity checked with `shasum -a 256`). New record-lint mini-roots missing-input-table (the plan's), input-bad-schedule-hash and input-events-over-cap (so the value rules have a control). `record_skeleton_256k` now expects format version 5 and the empty identity (89 rows); the new REQUIRED case `record_input_shown` boots a synthetic image with the four-event pad_2btn_correct script and pins `input.events=4` and
  the identity literal `8d209fc6...`, computed with `printf` and `shasum -a 256` over the four lines `0,1,8 / 140000,1,33 / 320000,1,146 / 510000,1,100` (the held names turned into the CD BIOS mask by hand: run 8, i and right 33, ii and up and left 146, select and right and down 100).

## Verification

- Task verifies as written, all PASS: Task 1 `--stage input-check` and `input-check-negatives`; Task 2 `input-check-negatives`, `flatten-controls`, `--selftest mutation-input-order`, `mutation-input-check-dead-stage`, `mutation-input-flatten-refuse` (and `mutation-input-rules`, `awk-exit-status`); Task 3 `conformance`, `record-lint`, `record-lint-negatives`, `determinism-negatives` (and `build`, `determinism`, `core-boundary`, `sanitizers`, so -Wconversion -Wshadow -Werror, ASan and UBSan stay green).
- Plan end: `sh scripts/check --timing` PASS (2 min 34 s wall: conformance 23 s, sanitizers 50 s, determinism 21 s, named-controls 9 s; the new stages 0 and 1 s), `sh scripts/check --selftest` PASS (94 PASS lines, no FAIL), `dash` runs of `--stage input-check`, `flatten-controls` and `record-lint-negatives` PASS, `sh scripts/audit-history --range 29e8b83..HEAD` clean (3 commits, 66 blobs). All ten toy controls stay RED_OK (named-controls PASS), `shared-state` included: run.c is unchanged.
- Bite: every rule has a deletion selftest (above). The printer was shown to matter with a scratch edit of text.c (a space instead of LF between the hashed lines turns `record_input_shown` into a guest failure; restored from a saved copy). A 1025-table script gives INP_TOO_MANY twice (declared and present) in a scratch run, not committed. record-lint value rules each have a mini-root.

## Deviations from Plan

**1. [Rule 3 - scope] 22 mini-roots instead of 17 negatives.** The plan lists 17 mini-roots; the rules file reports four more codes or paths that would otherwise have no control (INP_KEY_UNKNOWN, INP_KEY_MISSING, BAD_FILENAME, a stage 1 pass-through code), so one mini-root each was added (D-117: one negative per reason code), plus empty.toml in ok-scripts (the explicit empty script) and three record-lint mini-roots instead of one (the value rules of the new table).
**2. [Rule 3 - blocking] Existing mini-roots had to change.** `tests/gate/flatten-input/ok-script/a.toml` (03-27) used port 2 for an equal-time pair, which the shared rules now refuse as INP_PORT_EMPTY; it now uses port 1 at a later time (the control still checks 3 events and 3 INPUT rows). Five record-lint records that the plan does not list (missing-machine-table, missing-model-table, missing-stimulus-table, ok-stimulus-events, stimulus-count-mismatch) needed the [input] block too: without it the ok sample would be rejected and the three REC_TABLES negatives would
go red for the wrong reason (the EXPECT is a code set), so all ten accept-shaped records were updated.
**3. [Plan wording] The rules file was completed in Task 1, not Task 2.** Splitting the code set across two commits would have left a tracer that judges with half a rule set; Task 2 holds the controls and selftests.
**4. [Plan wording] Task 3's golden digest line is unchanged.** The schedule is not in the digest until plan 03-29 (IOPT and SCHD), so the digest, the profile hash and the content hash did not change; only format_version and the new table did.
**5. [Design] record_input_shown is a record_lines case, not record_run.** record_run does not apply input yet (03-29 hooks apply_input into the other kinds); record_lines applies it in the same place as the irq stimulus and keeps the whole skeleton under the same row check.

No Rule 4 deviation. The standing notes were not triggered: the shared-state toy stayed RED_OK; the [withheld] ids line is untouched at 24 of 24.

## Evidence discipline

E0 for everything here: the checker judges the shape of a test input and the record repeats which declared events the core was given. The identity literal comes from a host tool over a documented encoding of the script's own events, with the mask bits from the CD BIOS manual page 28 (CDBIOS-p28, as in 03-27), not from DuoSpool output. No ledger status changed, none promoted (only plan 03-32 may), none of the eight ER-2 candidates touched. No accuracy,
pad-accurate, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64). Remote evidence stays unverified, pending owner push authorisation.

## Known Stubs

None added. The schedule is still not in the digest or any snapshot region (IOPT and SCHD are plan 03-29), so two instances that differ only in their schedule have the same digest until then, although their records now differ (the [input] identity). Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-77 (numeric overflow in at): 15 digits and no leading zero in the rules, each with a negative and a deletion selftest; T-03-78 (oversized scripts): the 64 KiB cap (INP_TOO_LARGE, a real 65539-byte mini-root) and the 1024 cap (INP_TOO_MANY) have their own codes; T-03-79 (which input a run used): [input] identity in every accept record, checked by record-lint, reckinds.c and the REQUIRED case.

## Notes for next plans

- **03-29 (controls, regions, ledger):** the hook for the runner kinds is still one call, `apply_input(inst, k, &refusal)`, in chunkkinds.c, sweepkinds.c, obskinds.c, digkinds.c and reckinds.c; reckinds.c's `kind_record_lines` now has it (record_run does not). The none, wrong and late scripts are new files in tests/conformance/inputs: they are judged by `input-check` (stage `input-check`) and flatten with the same rules, so a script with an event on port 2 to 5 or a reserved name is refused before the runner sees it
  (the refusal rows of `input_refusal_rows` bypass scripts and stay the only path to those cases); a script that must differ from `correct` needs a different held set or time on port 1. The record's [input] identity changes with any event, so a wrong-input case will print a different identity from the correct one; `record_input_shown` pins the correct one for pad_2btn_correct (`8d209fc6...`), so editing that script changes the literal (recompute with printf and shasum over the AT,PORT,MASK lines).
- **Record layout:** the [input] table is the only change; adding the schedule to the digest (IOPT and SCHD) does not change the record text but changes the digest lines of both goldens. The row count of `kind_record_lines` is 68 plus one per stimulus event plus 4 per model parameter plus one per constant. The [withheld] ids line is still 24 of 24: the first plan that prints a new ledger id (for example a constants row or a model row) must change the layout to one id per row in the same task (standing note).
- **Ledger (03-29, 03-34):** LED-IO-0005 (no turbo, opposite directions refused) is now also enforced by the script checker (INP_HELD_OPPOSITE, INP_HELD_RESERVED) and by input_refusal_rows; LED-IO-0004 and the empty-port rule (D-116) by INP_PORT_EMPTY and input_port_refusal. No status change was made here.
- **CI:** the new stages cost 0 and 1 s; `.github/workflows/gate.yml` unchanged (the gate runs `scripts/check`); the committed too-large mini-root is 65539 bytes of repeated comment lines (a few hundred bytes compressed, not a ROM-size match for audit-history). `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: scripts/input-check, tests/gate/input-check (22 directories), tests/conformance/cases/record_input_shown.toml, tests/gate/record-lint/missing-input-table, input-bad-schedule-hash and input-events-over-cap. Commits f7c0bd0, 507385c and 2dc7ffd are ancestors of HEAD.
