---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 16
subsystem: determinism
tags: [interleave, shared-state-toy, canaries, fresh-process, env-i, malloc-scribble, byte-compare, e0-repeatability]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: toy-control mechanism and anchor rule (02-09), placeholder stepping with the one psg_noise update line (02-12), state digest (02-13), run record, --records contract and record-lint (02-14), core-boundary check (02-15)
provides:
  - ilvkinds.c with six runner kinds (interleave, interleave_twins, interleave_destroy_recreate, interleave_aba_order, interleave_canaries, liveness_shared_state_selfcheck) and six REQUIRED cases (100 cases in the directory, 94 before)
  - tests/controls/shared-state (RULE, EXPECT of three ids): a toy built at control time from the live advance.c with a hidden static counter folded into the psg_noise update
  - scripts/determinism-run (run, compare) with DET_RECORD_DIFF, DET_RECORD_LINT, DET_MISSING_FILE, DET_RUN_FAILED; controls differs and ok-same; stages determinism and determinism-negatives; selftest mutation-determinism-compare
affects: [02-17 and later (stage count is now 38), 02-21 (container run supplies the first Linux evidence for the MALLOC_PERTURB_ branch)]

actuals:
  tokens: 17701    # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 70805 chars, 20 files, 1693 added and 2 removed lines)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 9b0db242eaeba62f6322b5a2fd043bf85713e24a
plan_head_after: bd0740c2cd2aeed73ccff00f70b86229e4ecb4a5
commits: 3

tech-stack:
  added: []
  patterns:
    - "an interleave case allocates one arena in test code (instances aligned to ds_instance_align, canary bytes on both sides of every block and in the alignment tail) and steps instances on a fixed-seed 64-bit LCG schedule beside solo copies that receive only their own calls"
    - "a comparison helper is proven alive by running it twice, once over separate instances and once with two pointers aliased, and asserting that only the aliased run reports a difference (a scratch ctx collects the helper's assertions)"
    - "a toy rule is one anchored line and one replacement; the liveness toy for shared state is a hidden function-local static in a generated copy of the live advance.c, never committed"
    - "the fresh-process comparison runs the whole runner through the existing supervise watchdog, the second time under env -i with a different TZ, TMPDIR, working directory, marker seed and allocator scribbling"

key-files:
  created:
    - tests/conformance/ilvkinds.c
    - tests/conformance/cases/ (6 files: interleave_ab, interleave_twins, interleave_destroy_recreate, interleave_aba_order, interleave_canaries, liveness_shared_state_selfcheck)
    - tests/controls/shared-state/RULE
    - tests/controls/shared-state/EXPECT
    - scripts/determinism-run
    - tests/gate/determinism/ (differs: a.toml, b.toml, EXPECT; ok-same: a.toml, b.toml)
  modified:
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - CMakeLists.txt
    - scripts/check

key-decisions:
  - "the interleave cases carry two requests A||B in one request key (as digest_pair does) over one fixture: B is the same fixture with len=393216 and seed 2, A is 256K with seed 1"
  - "the shared-state toy folds the whole counter value (XOR of ++shared_n), not its low bit: a parity-only fold would not move the digest when a call has an even number of cycles"
  - "DET_MISSING_FILE is exit 2 and is asserted directly in the stage function, so tests/gate/determinism holds exactly two control directories (differs, ok-same)"
  - "the control record pairs are named a.toml and b.toml (the plan says a.txt and b.txt): compare runs record-lint on both, and a record that is not a clean record is not compared"

patterns-established:
  - "a runner kind family lives in its own file when runner.c is near its size cap (now 65147 of 65536 bytes): one table row per kind in runner.c, declarations in runner.h"

requirements-completed: [CORE-01, CONT-03]

coverage:
  - id: D1
    description: "Two instances run interleaved in one process equal each solo run in digest and printed record, under a fixed-seed schedule, with identical-input twins, destroy-and-recreate over stale bytes mid-run, A-B-A order and canary-guarded adjacent memory"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance PASS and --stage sanitizers PASS; RESULT interleave_ab pass assertions=90, interleave_twins pass 36, interleave_destroy_recreate pass 87, interleave_aba_order pass 9, interleave_canaries pass 66, liveness_shared_state_selfcheck pass 2 (the same counts in the sanitize preset, no ASan or UBSan report)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A toy library with a hidden static counter is shown red by the interleave cases, so the interleave test is alive"
    requirement: CORE-01
    verification:
      - kind: integration
        ref: "CONTROL shared-state RED_OK (EXPECT interleave_ab, interleave_twins, interleave_aba_order, each guest_failure on the toy); with the rule's replacement set equal to its anchor the stage printed CONTROL shared-state NOT_RED and failed (observed once, restored); SELFTEST mutation-control-anchor shared-state PASS"
        status: pass
    human_judgment: false
  - id: D3
    description: "The same inputs in a fresh process produce byte-identical records, the second process run under an empty environment, a different time zone, working directory and temporary directory, with allocator scribbling on"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "sh scripts/determinism-run run: DETERMINISM_RECORDS cases=6 process1=6 process2=6, STATUS determinism ok n=6; CHECK determinism PASS; a scratch edit of the record writer that appended the TZ value gave DET_RECORD_DIFF on all six records (restored)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The record comparison itself can fail (a differing pair is rejected, a missing file is exit 2) and is mutation-proven"
    requirement: CONT-03
    verification:
      - kind: integration
        ref: "CHECK determinism-negatives PASS (differs gives exactly DET_RECORD_DIFF, ok-same passes, missing path gives exit 2 with DET_MISSING_FILE); SELFTEST mutation-determinism-compare PASS"
        status: pass
    human_judgment: false

duration: 60 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 16: Interleave isolation and the fresh-process determinism run Summary

**Two instances stepped alternately in one process now end with the digest and printed record each has when run alone (five scenario shapes, canary-guarded blocks), a hidden-static toy library is shown red by those cases, and a second process run under `env -i` with another time zone, working directory, temporary directory, marker seed and allocator scribbling writes byte-identical records, with both proofs shown able to fail.**

This proves isolation and repeatability of the placeholder stepping at evidence tier E0. It does not prove hardware correctness, and nothing in it is a claim about the PC Engine.

## Performance

- **Duration:** about 60 min (2026-11-10)
- **Tasks:** 3 of 3 (Task 1 tracer, Task 2 auto with tdd, Task 3 auto)
- **Files:** 20 changed outside .planning (1693 added lines, 2 removed)
- **Gate cost (this machine, warm):** `sh scripts/check --timing` 63 s wall over 38 stages, all PASS. D-60's 60 s proposal is exceeded by about 3 s (time never fails the gate); the dominant stages are `conformance` (8 to 12 s) and `sanitizers` (10 to 11 s), the interleave cases adding about 1 s to each; `determinism` takes 2 s and `determinism-negatives` under 1 s. `sh scripts/check --selftest` ends `SELFTEST RESULT PASS` with 22 mutation lines.

## Accomplishments

- **Tracer (Task 1).** `tests/conformance/ilvkinds.c` builds one arena in test code: instance blocks aligned to `ds_instance_align()`, 64 canary bytes (rounded up to the alignment) on each side of every block and in the alignment tail, a pattern that depends on position. Kind `interleave` boots A (256K content, seed 1, seeded-prng-v1) and B (384K content, seed 2) plus solo copies A0 and B0 in four slots, then steps a 64-call schedule from a fixed-seed 64-bit LCG (calls of 1 to 4093 cycles, far below `DS_ADVANCE_MAX`), advancing A with A0 and B with B0 by identical cycles while A and B alternate. Digests are compared at every eighth call and at the end, printed records at the end, machine time against the sum of the given cycles, and non-vacuity is asserted (A and B differ at power-on, stepping changes the digest). 90 assertions.
- **Shared-state toy.** `tests/controls/shared-state/RULE` (tag `rule-shared-counter`) anchors on the single live line `inst->psg_noise = noise_step(inst->psg_noise);` of `src/core/advance.c` and replaces it with a block holding a function-local static counter folded into the register (`^ ++shared_n`). `scripts/control-mutate` generates the toy source into `build/` at control time; no copy of `advance.c` is committed and `ls tests/controls/shared-state` lists exactly RULE and EXPECT. The generated toy is outside `src/core`, so the boundary check never scans it. On the toy, `interleave_ab`, `interleave_twins`, `interleave_aba_order`, `interleave_destroy_recreate` and `liveness_shared_state_selfcheck` come out `guest_failure`; only `interleave_canaries` passes (it checks memory, not stepping). EXPECT lists the three ids the plan requires.
- **Four more shapes and the liveness check (Task 2).** `interleave_twins` steps two identical-input instances interleaved, each in its own call sizes (the call counts differ, asserted) for the same total of 30000 cycles, and requires equality with each other and with a third instance that took the total in one call. `interleave_destroy_recreate` fills the block of B with a stale pattern at call 32 and boots B again over it without clearing the block first: the reboot must equal B's fresh digest, replaying B's earlier calls must equal the solo copy, and the run continues to the end with A unaffected. `interleave_aba_order` runs A (9000 cycles), then B (7000), then A (11000), then boots a reference A that takes 20000 in other call sizes and a reference B afterwards; both must match in digest and record. `interleave_canaries` uses three adjacent blocks and shows the canary check able to fail (a changed guard byte and a changed padding byte are seen, then restored), then checks no canary moves through boot, 48 steps, the discard of the middle block and its reboot, and that the first instance's digest does not move. `liveness_shared_state_selfcheck` runs the interleave helper twice: over separate instances it must report no difference and with B's memory aliased to A's (second pointer equal to the first) it must report one.
- **Fresh-process run (Task 3).** `scripts/determinism-run run` asserts the stage inputs exist (exit 2 naming the missing path, it never builds), derives the record-producing case names from the kind column of `build/gate/cases.tsv` with awk (kinds accept, record_lines, record_golden, record_refusal), runs the built runner through the existing `conformance-run supervise` watchdog twice with the same absolute arguments plus `--records`, the second time as `env -i TZ=Pacific/Kiritimati TMPDIR=<dir> MallocScribble=1 MallocPreScribble=1 /bin/sh -c 'cd <other dir> && exec ...'` (Linux: `MALLOC_PERTURB_=165`), requires that each record directory holds exactly the expected `ID.record.toml` set, and then calls `compare` over the two directories: `record-lint` on both, equal name sets, `cmp` on every pair. `compare` is also the interface of the controls. Marker `STATUS determinism ok n=6`.
- **Controls and mutation.** `tests/gate/determinism/differs` (two lint-clean records that differ in one hex digit of the state digest, EXPECT `DET_RECORD_DIFF`) and `ok-same` (byte copies). The missing-file case is asserted in the stage function because it is exit 2. `scripts/determinism-run` carries the token `rule-cmp` on exactly one line (the `cmp` call); `SELFTEST mutation-determinism-compare` deletes it from a scratch copy and requires the differs control to exit 1.

## Verification Results

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| Task 1: `sh scripts/check --stage conformance` | exit 0, `CHECK conformance PASS`; `RESULT interleave_ab pass assertions=90`; four `CONTROL ... RED_OK` lines (drop-region, heuristic-reverse, shared-state, wrong-mirror) in `scripts/conformance-run run` |
| Task 2: `sh scripts/check --stage conformance && sh scripts/check --stage sanitizers` | exit 0, both PASS; all six new ids pass in both presets |
| Task 3: `sh scripts/check --stage build && ... --stage conformance && ... --stage determinism && ... --stage determinism-negatives && ... --selftest mutation-determinism-compare` | exit 0, four `CHECK ... PASS` lines and `SELFTEST mutation-determinism-compare PASS` |

Acceptance criteria, all run:

- Task 1: `RESULT interleave_ab pass assertions=90` (at least 8); four RED_OK lines; `ls tests/controls/shared-state` lists exactly RULE and EXPECT. With the rule's replacement set equal to its anchor the conformance stage ended `CHECK conformance FAIL` with `CONTROL shared-state NOT_RED` and `expected guest_failure; got: interleave_ab:pass` (observed once; the saved RULE was copied back and the stage passed again). `sh scripts/check --selftest` prints `SELFTEST mutation-control-anchor shared-state PASS`.
- Task 2: the five new ids pass; the toy's EXPECT lists three ids (three non-comment lines; the file also carries the same four comment lines as the other toys' EXPECT files, so a literal `wc -l` is 7, see Deviations); the sanitize results show the same cases passing with no sanitizer report.
- Task 3: `sh scripts/determinism-run run` prints `DETERMINISM_RECORDS cases=6 process1=6 process2=6` and `STATUS determinism ok n=6`. The six names in each directory are `accept_tracer_256k`, `record_content_fields`, `record_golden_256k_zero`, `record_golden_384k_seeded`, `record_refusal_text` and `record_skeleton_256k` (each with `.record.toml`); the two directories hold identical name sets (`cmp` of the sorted lists equal); the two golden cases are two of the six, not a stand-in for them. With `build/gate/cases.tsv` moved aside (observed once, restored) the run exited 2 with `HARNESS_NO_FILE build/gate/cases.tsv is missing: ...` (the script is absent at the baseline). `sh scripts/check` printed 38 `CHECK <stage> PASS` lines including `determinism` and `determinism-negatives`, and ended `CHECK RESULT PASS`. `head -5 scripts/determinism-run` holds `SPDX-License-Identifier: MIT OR Apache-2.0` and `head -5 tests/controls/shared-state/RULE` holds `SPDX-License-Identifier: 0BSD`; `sh scripts/check --stage licences` passes. `grep -c 'rule-cmp' scripts/determinism-run` prints 1. In `scripts/check` the `run_stage` lines for build (1363), conformance (1365) and determinism (1372) appear in that order.
- Hostile-environment liveness (scratch, not committed): the record writer in `reckinds.c` was changed to append the `TZ` value to each record file; the determinism run then printed `DET_RECORD_DIFF` for all six records and exited 1, and after restoring the file the stage passed again. The comparison and the hostile second process therefore do reach the records.
- Per-stage seconds (`sh scripts/check --timing`, warm): conformance 12, sanitizers 10, rights-negatives 4, audit-synthetic 4, ledger-negatives 4, protocol-negatives 6, determinism 2, core-boundary-negatives 2, every other stage 0 to 2. Warm total 63 s wall, over the 60 s proposal of D-60; no stage exceeds the 20 s warning line.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 38 `CHECK <stage> PASS` lines, ends `CHECK RESULT PASS` (63 s) |
| `dash scripts/check` | 38 stage lines PASS, ends `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | 22 mutation lines PASS including `mutation-control-anchor shared-state` and `mutation-determinism-compare`, then `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range 9b0db24..HEAD` | `audit-history: clean` (3 commits, 21 blobs) |
| fresh-process comparison | `STATUS determinism ok n=6` |

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 fix was needed in committed code. No core change was needed (the interleave cases found no state-sharing defect).

### Interpretation choices (inside the plan's intent)

1. **Plumbing of Task 2 landed with Task 1's commit.** All six kinds live in `ilvkinds.c`, which is one file, so the file and the six table rows in `runner.c` are in Task 1's commit; Task 1's tree holds only the `interleave_ab` case, its REQUIRED line and a one-id EXPECT, and Task 2's commit adds the other five cases, their REQUIRED lines and the two extra EXPECT ids. Each commit passes the gate on its own tree.
2. **Files beyond `files_modified`.** `tests/conformance/runner.h` (declarations of the six kinds) and `tests/conformance/ilvkinds.c`. `runner.c` got only six table rows (65147 of 65536 bytes now).
3. **Kind name `interleave`, request A||B.** The plan names the case `interleave_ab` and says "add runner kind interleave". Two requests travel in the one request key, separated by `||`, as in `digest_pair`; the fixture is `plain-256k` and B is that recipe with `len=393216`.
4. **EXPECT of the toy holds three ids plus the standard four comment lines.** The plan says the toy's EXPECT lists three ids (`wc -l` equals 3). The other toys' EXPECT files begin with the SPDX header and a comment, and `conformance-run` ignores comment and blank lines, so the file has three ids and seven lines; the count of ids is three.
5. **The determinism control files are `a.toml` and `b.toml`, not `a.txt` and `b.txt`.** `compare` lints both records first, so the pair must be clean records. `ok-same` holds byte copies of the clean accept record of the record-lint controls; `differs` holds that record and a copy with the last hex digit of the state digest changed.
6. **`missing-file` is not a control directory.** Its code is exit 2, which `run_controls` would report as a harness error; the plan says to assert it directly in the stage function, which does, over the path `tests/gate/determinism/ok-same/absent.toml`. `tests/gate/determinism` therefore lists two directories, `differs` and `ok-same`.
7. **The run subcommand reads two more inputs than the four named.** `build/gate/rules-fields.txt` (the runner's `--rules-fields` argument) and `build/gate/ids.txt` (the supervise watchdog's id list) are needed to run the whole runner under the same watchdog the conformance step uses. Both are made by the same conformance step; a missing one is the same exit 2 harness message.
8. **Extra codes.** `DET_RECORD_LINT` (record-lint reported a violation, nothing is compared) and `DET_RUN_FAILED` (a runner process exited non-zero, so records were not compared). The plan names DET_RECORD_DIFF and DET_MISSING_FILE; both are present and behave as specified.
9. **The two processes use different marker seeds** (1 and 2). The plan does not say; a record must not depend on the marker seed, and this tests it. Both runs run the whole runner (all 100 cases), not only the record-producing ones.
10. **Stage count.** The plan expects at least 37 (35 after plan 15 plus two). The gate has 38 stages now: plan 15's summary said 35 but the count before this plan was 36.
11. **CORE-01 and CONT-03 are marked complete only where `requirements.ready-ids` allows it** (see the state step below).

**Total deviations:** 0 auto-fixed, 11 interpretation choices. **Impact:** none on later plans except the notes below.

## Notes for later plans

- **runner.c is 65147 of 65536 bytes.** Any new kind goes in its own file (as `ilvkinds.c`, `reckinds.c`), with one table row in `runner.c` and a declaration in `runner.h`.
- **A later change to `ds_advance`** must keep the line `inst->psg_noise = noise_step(inst->psg_noise);` unique in `src/core/advance.c`, or the anchor of `tests/controls/shared-state/RULE` must be updated with it; `scripts/control-mutate` exits 2 on a count other than one and `SELFTEST mutation-control-anchor shared-state` catches the loss of the anchor.
- **Phase 3 (real CPU, video, audio).** The interleave and fresh-process cases reach the machine through `ds_advance`, `ds_digest` and `ds_text_record`. When real stepping replaces the placeholder, the case expectations need no literals (they compare an instance with its own solo run), but the shared-state toy needs a new anchor line in the new stepping code.
- **Platform labelling.** The second process ran on Darwin arm64 with `MallocScribble` and `MallocPreScribble`. The Linux branch (`MALLOC_PERTURB_=165`) and `/bin/sh` under a Linux libc are written from the plan and the research notes and have not been executed here; the first Linux evidence for them is the plan 21 container run and CI.
- **Evidence tier.** E0. Equal digests and equal record bytes show isolation of the placeholder state and independence of the records from the host environment variables that were varied (TZ, TMPDIR, working directory, allocator fill, marker seed). They do not show that the host-dependent inputs that were not varied (CPU, OS, compiler) are harmless, and they say nothing about hardware behaviour.

## Task Commits

1. **Task 1 (tracer): interleave kind over two instances, shown red by the shared-state toy** - `b924d70` (feat)
2. **Task 2: twins, destroy and recreate, A-B-A order, canaries and the helper's liveness check** - `c81fdb4` (feat)
3. **Task 3: fresh-process determinism run, byte comparison with controls, stage and mutation** - `bd0740c` (feat)

## Known Stubs

None. `fixture = "plain-256k"` and the seeded requests are real inputs; the placeholder stepping that the interleave cases exercise is documented as a placeholder in `advance.c`, in LED-PROF-0002 and in plan 12's summary, and is not a stub of this plan.

## Threat Flags

None. No network endpoint, auth path or schema change at a trust boundary was added. T-02-31 is mitigated by the five interleave shapes with canaries, the shared-state toy and the aliasing self-check (and by the writable-symbol rule of plan 15); T-02-32 by the second process under `env -i` with different TZ, working directory, TMPDIR, marker seed and allocator scribbling, byte comparison and record-lint on both sets, with a planted environment leak shown caught; T-02-33 by the `differs` control, `mutation-determinism-compare` and the liveness self-check, all labelled repeatability only. T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `tests/conformance/ilvkinds.c`, the six case files, `tests/controls/shared-state/RULE` and `EXPECT`, `scripts/determinism-run`, `tests/gate/determinism/differs/a.toml`, `b.toml`, `EXPECT` and `tests/gate/determinism/ok-same/a.toml`, `b.toml`.
- Commits `b924d70`, `c81fdb4` and `bd0740c` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 3.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
