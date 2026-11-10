---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 01
subsystem: testing
tags: [sh, gate, scripts-check, run_controls, negative-controls, transcript-equivalence, selftest]

requires:
  - phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
    provides: scripts/check gate with six copy-pasted control loops, ff126b5 as the pre-refactor baseline
provides:
  - run_controls helper in scripts/check (one control loop for six control families)
  - sh scripts/check --stage NAME (one stage, that stage's verdict as exit status)
  - sh scripts/check --selftest NAME (one mutation) and mutation-run-controls
  - tests/check/equiv.sh transcript-equivalence harness (base tree versus working tree)
affects: [02-02, later Phase 2 plans that add control families or verify one stage or one mutation]

actuals:
  tokens: 9600     # chars/4 over the realized diff (git diff 25c3999..HEAD, 38373 chars)
  tasks: 3
  commits: 3       # measured: git rev-list --count plan_head_before..HEAD
plan_head_before: 25c3999dc43d82371d857939b3d5501bcabee086
plan_head_after: bb722648714182099961c3c5d05e95cb71182969
commits: 3

tech-stack:
  added: []
  patterns:
    - "one run_controls call per control family; the only line that sets the failing status carries the rc-broken-status marker"
    - "refactor of a gate proven by transcript equivalence (stdout, stderr, exit status) over mutated trees, plus a selftest mutation, never by reading the diff"

key-files:
  created:
    - tests/check/equiv.sh
  modified:
    - scripts/check

key-decisions:
  - "run_controls takes named options before the directory (--ok-prefix, --expect-selects, --need, --opt-file, --require-negative) and checker words with @DIR@, @EXPECT@ and @OPT@ placeholders; @EXPECT@ expands to the two words --expect TEXT for must-fail cases and to nothing for must-pass ones"
  - "the profile stage keeps its hard-coded record path as a template word plus --need HARNESS_NO_RECORD (RESEARCH Pitfall 1)"
  - "--stage prints only that stage's CHECK line (no CHECK RESULT line) and exits with its verdict; an unknown NAME exits 2 after the stage list"
  - "equiv.sh runs both sides inside shared-object clones of the repository's .git so the protocol-order stage sees real history instead of HARNESS_NO_REPO on both sides"

patterns-established:
  - "Mutation selftests assemble their marker token at run time so the script carries the token exactly once, on the line being deleted"
  - "A harness scenario is vacuous unless the base transcript contains the label it was built to exercise; vacuous scenarios fail the run"

requirements-completed: [TOOL-02]

coverage:
  - id: D1
    description: "scripts/check keeps its exit contract and byte-identical stdout for a clean tree and 32 mutated-tree scenarios, before and after the refactor"
    requirement: TOOL-02
    verification:
      - kind: integration
        ref: "sh tests/check/equiv.sh <git archive of ff126b5> (33 scenarios SAME, EQUIV LABELS covered, EQUIV RESULT PASS, run three times)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Six near-identical control loops run through one run_controls helper"
    requirement: TOOL-02
    verification:
      - kind: other
        ref: "grep -c run_controls scripts/check = 9 at the end (8 after Task 2: one definition, one usage comment, six calls; Task 3 adds one selftest comment line); CONTROL BROKEN 12 -> 7; SAMPLE REJECTED 6 -> 2"
        status: pass
    human_judgment: false
  - id: D3
    description: "A broken control still turns the gate red, mutation-proven by deleting the single rc-broken-status line"
    requirement: TOOL-02
    verification:
      - kind: unit
        ref: "sh scripts/check --selftest mutation-run-controls -> SELFTEST mutation-run-controls PASS"
        status: pass
    human_judgment: false
  - id: D4
    description: "--stage NAME and --selftest NAME select exactly one stage or mutation; unknown names exit 2"
    requirement: TOOL-02
    verification:
      - kind: unit
        ref: "sh scripts/check --stage licence-negatives; sh scripts/check --stage bogus-stage (exit 2); sh scripts/check --selftest no-such-mutation (exit 2)"
        status: pass
    human_judgment: false

duration: 22 min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 01: run_controls refactor of the gate Summary

**Six copy-pasted control loops in scripts/check now run through one `run_controls` helper, proven behaviour-preserving by a 33-scenario transcript-equivalence harness against the ff126b5 gate and by a selftest mutation, with `--stage NAME` and `--selftest NAME` added for later single-stage and single-mutation verification.**

## Performance

- **Duration:** 22 min (2026-11-09T22:14 to 22:37 local)
- **Tasks:** 3 of 3
- **Files:** 2 (1 created, 1 modified)
- **Gate wall time (`sh scripts/check`):** 9.99 s after the refactor. Baselines: 9.7 s from the plan, 10.97 s measured on this machine at ff126b5 just before the first edit. The difference is under 2 s in either direction, so no investigation was needed.

## Accomplishments

- `run_controls` replaces the loops of ledger-negatives, decision-negatives, profile-negatives, provenance-negatives, licence-negatives and results-negatives. The score, filename, empty-expect, samples and protocol stages were not touched.
- `sh scripts/check` stdout is byte-identical to the ff126b5 transcript (18 `CHECK name PASS` lines then `CHECK RESULT PASS`); the whole gate also passes under dash.
- `tests/check/equiv.sh BASE_DIR` runs base and working-tree gates over a clean tree and 32 mutated trees (every converted stage, every label) and compares stdout, stderr and exit status. It was shown to catch a weakened helper (a deliberate `rc=0` in place of `rc=1` produced `EQUIV RESULT PARTIAL FAIL` with a readable diff) and then restored.
- The selftest mutation removes the one line carrying the failing-status marker from a scratch copy; the same broken control goes from exit 1 to exit 0, so the gate's redness depends on exactly that line.

## Task Commits

1. **Task 1 (tracer): run_controls helper, --stage flag, equivalence harness, licence-negatives converted** - `892c408` (refactor)
2. **Task 2: convert the five remaining control loops** - `4fe4145` (refactor)
3. **Task 3: selftest mutation-run-controls and --selftest NAME** - `bb72264` (test)

## Verification Results

Per-task `<automated>` commands (stage granularity, D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check --stage licence-negatives && { sh scripts/check --stage bogus-stage ...; test $? -eq 2; }` | exit 0; `CHECK licence-negatives PASS`; bogus stage exit 2 |
| loop over the six `*-negatives` stages with `grep -qx "CHECK $s PASS"` | exit 0, no FAIL line |
| `sh scripts/check --selftest mutation-run-controls \| grep -qx 'SELFTEST mutation-run-controls PASS' && { --selftest no-such-mutation; test $? -eq 2; }` | exit 0 |

Plan-end (wave-end) commands:

| Command | Result |
|---------|--------|
| `sh tests/check/equiv.sh <git archive ff126b5>` (run after Task 1, after Task 2, after Task 3) | 33 `EQUIV ... SAME` lines each time, `EQUIV LABELS covered`, `EQUIV RESULT PASS`, exit 0 (about 175 s each) |
| `EQUIV_ONLY=<family> sh tests/check/equiv.sh ...` after each of the five conversions | `EQUIV RESULT PARTIAL PASS` each |
| `sh scripts/check` | exit 0, 18 stage PASS lines plus `CHECK RESULT PASS`, diff against the ff126b5 transcript empty |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | six `SELFTEST ... PASS` lines (five `mutation-` plus `missing-sentinel`) and `SELFTEST RESULT PASS`; diff against the ff126b5 selftest output is exactly the one added `mutation-run-controls` line |
| `dash scripts/check --selftest mutation-run-controls` | PASS |

Acceptance criteria: `head -5 tests/check/equiv.sh | grep -c SPDX-License-Identifier` = 1; `grep -c run_controls scripts/check` = 9 at plan end, 8 when checked after Task 2 (at least 7); `grep -c 'CONTROL BROKEN'` = 7 (at most 8); `grep -c 'SAMPLE REJECTED'` = 2 (at most 2); `grep -c rc-broken-status` = 1 (exactly 1); `sh scripts/check --stage bogus-stage` exits 2.

## Files Created/Modified

- `scripts/check` - `rc_expand` and `run_controls` helpers, six converted stages, `--stage NAME`, `--selftest [NAME]`, `st_want`, mutation (f), updated usage and header.
- `tests/check/equiv.sh` - the equivalence harness (new, SPDX MIT OR Apache-2.0).

## Decisions Made

See `key-decisions` above. The helper's option names (`--need`, `--opt-file`) and the `@OPT@` placeholder were chosen to cover the ledger REQUIRED pass-through and the two-file requirement of the results stage without per-stage code; the plan left the exact option names open.

## Deviations from Plan

### Auto-fixed Issues

None. No bug, missing critical functionality or blocker needed a Rule 1-3 fix in the plan's own work.

### Additions and interpretation choices (inside the plan's intent)

1. **Scenario set is 33, not 8.** The plan names eight scenario kinds; the harness has a scenario for every branch of every converted stage (wrong expect, no EXPECT, empty EXPECT, failed ok- sample, no cases, missing record or file, REQUIRED removed, only-ok results, unreadable protocol). The seven named labels are covered, plus `HARNESS_ERROR` as an eighth required label (the `results-unreadable` scenario uses `chmod 000` on a protocol file; on a host that ignores file modes the scenario is reported VACUOUS rather than passing silently).
2. **Per-scenario vacuity check.** Besides the set-level label coverage the plan asks for, each scenario must make the BASE transcript contain its own label and differ from the clean transcript, otherwise it is reported `VACUOUS` and fails.
3. **`EQUIV_ONLY=PREFIX` environment filter.** A full run takes about 175 s (66 gate runs, fork-bound), so a family filter was added for the per-conversion re-runs. It skips the label test and ends in `PARTIAL PASS|FAIL`, never `PASS`; every acceptance run quoted above is a full unfiltered run.
4. **History clone in the scratch copies.** A bare `git archive` tree has no repository, so the protocol-order stage would have errored on both sides and made every exit status 2 (masking the 0-versus-1 distinction). Both sides carry a shared-object `git clone` of this repository's `.git` instead; the real repository is only read.
5. **`--selftest NAME` arrived in Task 3** (the plan lists it under Task 3), so the Task 1 commit has the narrower usage line `[--selftest | --stage NAME]`.
6. **`@EXPECT@` placeholder semantics:** it expands to the two words `--expect TEXT` (must-fail) or to nothing (must-pass), instead of a single text word, which avoids tracking the preceding `--expect` word in POSIX sh. Observable behaviour is unchanged (proved by the harness).
7. **Selftest (f) is stricter than the plan's two steps:** it also requires the marker to occur exactly once before the mutation and zero times after (the mutation removed exactly the rule under test) and the unmutated run to name `CONTROL BROKEN broken`.

**Total deviations:** 0 auto-fixed, 7 additive or interpretive notes. **Impact:** none on scope; the harness is stricter than specified.

## Issues Encountered

- The first full harness run was slower than expected (about 175 s) and was moved to the background by the tool timeout; it passed. Recorded as a CI-cost observation: the harness is a one-shot refactor proof against ff126b5, not part of the fast gate and not a CI job. Once a later plan adds a stage or changes stdout on purpose, it will report DIFF against ff126b5 by design and must not be reused as a permanent gate.
- Pre-existing working-tree noise (`.planning/STATE.md` modified, `.planning/state.json` untracked) existed before this plan and is left to the state updates.

## Known Stubs

None.

## Threat Flags

None. No new network endpoint, auth path or file-access pattern at a trust boundary; the harness only reads the real repository (shared clone) and writes under `mktemp -d`. T-02-01 (tampering by the refactor) is mitigated by the equivalence harness plus mutation-run-controls; T-02-02 (vacuous `--stage`) by exit 2 on an unknown name; T-02-SC accepted (no installs).

## Next Phase Readiness

Plan 02-02 can add a control family with one `run_controls` line, verify a single stage with `sh scripts/check --stage NAME`, and a single mutation with `sh scripts/check --selftest NAME`.

## Self-Check: PASSED

- FOUND: `/Users/jon/projects/duospool/tests/check/equiv.sh`, `/Users/jon/projects/duospool/scripts/check`
- FOUND commits (ancestors of HEAD): 892c408, 4fe4145, bb72264
- `git rev-list --count 25c3999dc43d82371d857939b3d5501bcabee086..bb722648714182099961c3c5d05e95cb71182969` = 3
