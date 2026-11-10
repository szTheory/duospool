---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 20
subsystem: ci
tags: [workflow-check, expression-injection, concurrency, mutation-proof, ci-timings, mawk, awk-dialect, ci-01]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: gate.yml, scripts/workflow-check, scripts/lib/workflow-rules.awk and the ok-gate and pull-request-target mini-roots (02-19); run_controls, the selftest frame and the gate with 44 stages (02-01 to 02-19)
provides:
  - the four remaining workflow-check codes WF_EXPR_IN_RUN, WF_UNTRUSTED_EXPR, WF_FORBIDDEN_KEY and WF_CONCURRENCY, each with a red control, plus the unparsable shape asserted as exit 2 in the stage function
  - tests/workflow with 12 mini-roots (ok-gate, pull-request-target, untrusted-expr, tag-pinned-action, no-permissions, continue-on-error, or-true, secrets-ref, no-fetch-depth, wrong-gate-command, missing-template-line, wrong-concurrency)
  - selftest mutation-workflow-pin (deleting the one rule-pin line turns the tag-pinned-action control broken)
  - scripts/ci-timings (RUN_ID through a read-only gh run view, or --tsv FILE) with tests/gate/ci-timings and the stage ci-timings-controls; the gate now has 45 stages
  - docs/ci-timings.md (no CI run exists yet, failure classes, standing protocol, pending hand-offs)
  - the record-lint fix for mawk (D-19-1), proven in an Ubuntu 24.04 container under mawk and gawk
affects: [02-21 (record 0007 sentence), 02-22 (the push checkpoint and criterion 5: the first real run fills the timing table), any later gate.yml edit (ok-gate and the controls must follow it)]

actuals:
  tokens: 23455    # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 93819 chars, 49 files, 1518 added lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit); one is the D-19-1 fix, three are the plan tasks
plan_head_before: 39604b14d41503d24a351bfb84fc57bba23b7bd4
plan_head_after: 1f6b0d2eaeedbcca3b5f8c0c3c13d947eb889c3e
commits: 4

tech-stack:
  added: []
  patterns:
    - "an expression opener is scanned on every non-comment line including the content of a run block (a shell comment inside a run block is still expanded by the runner); the allowlist is a set of context names plus one comparison shape"
    - "a secrets expression reports one code (WF_FORBIDDEN_KEY) and is not also reported as untrusted, so the control for it is single-code"
    - "an offline control for a network tool: a stub gh first on PATH records its argv, so the control proves the --tsv mode never calls it and the RUN_ID mode calls only run view with --json and --jq"
    - "awk portability: never assign to an element and test membership of the same element in one statement (mawk evaluates the target first)"

key-files:
  created:
    - scripts/ci-timings
    - tests/gate/ci-timings/ (canned.tsv, malformed.tsv, EXPECT)
    - tests/workflow/ (ten new mini-roots, each with .github/ and an EXPECT)
    - docs/ci-timings.md
  modified:
    - scripts/lib/record-rules.awk
    - scripts/lib/workflow-rules.awk
    - scripts/workflow-check
    - scripts/check
    - .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/deferred-items.md
    - .planning/WINDOWS.md

key-decisions:
  - "the secrets control uses an expression, so a secrets expression is reported as WF_FORBIDDEN_KEY only (not also WF_UNTRUSTED_EXPR); this keeps every control single-code"
  - "WF_EXPR_IN_RUN counts a run value on one line the same as a run block, and scans shell comments in a run block"
  - "the allowlist accepts an allowlisted context compared with == to a quoted lowercase word, because the real workflow needs github.event_name == 'pull_request' in the cancel-in-progress expression"
  - "wrong-concurrency was added as a twelfth mini-root: the plan truth demands a red control for the concurrency shape but its list of eleven names none"
  - "scripts/ci-timings reports queue as run startedAt minus createdAt and wall as the latest job completedAt minus run startedAt, the critical path; a job that never ran is shown as skipped"

patterns-established:
  - "every static rule has a named bad-workflow mini-root that is a one-line mutation of ok-gate with a single-code EXPECT, fixed before the first run (D-80)"
  - "a docs status page states what has not happened (no CI run) before it shows a table, and the table stays empty until a real run earns a row"

requirements-completed: [CI-01]

coverage:
  - id: D1
    description: "Every remaining workflow assertion has a named bad-workflow control shown red, and the unparsable shape is exit 2, never a pass"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage workflow-negatives PASS (12 mini-roots: ok-gate accepted, eleven negatives each with its exact code set); the stage also asserts that a flow-map workflow exits 2 without and with --expect WF_UNPARSABLE; eleven further scratch mutations (block-scalar expression, || true, || :, self-hosted, secrets key, wrong group, unclosed expression, spaced comparison) each gave the expected codes"
        status: pass
    human_judgment: false
  - id: D2
    description: "The pin rule is mutation-proven"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "sh scripts/check --selftest mutation-workflow-pin prints SELFTEST mutation-workflow-pin PASS (before=1, after=0 lines tagged rule-pin; unmutated EXPECT_OK, mutated exit 1 with EXPECT_MISMATCH got <none>)"
        status: pass
    human_judgment: false
  - id: D3
    description: "CI timing is recordable as a markdown row from a run id or a canned TSV, offline in the control, and the docs state that no CI run exists and list the pending hand-offs"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage ci-timings-controls PASS (canned row equals EXPECT and gh is not called; malformed exits 2; RUN_ID mode against a stub gh calls only run view ID --json --jq; non-numeric id exits 2; tree unchanged); a deliberately wrong EXPECT turned the stage red (observed once, reverted); grep -c 'No CI run' docs/ci-timings.md is 1 and grep -c 'Pending hand-offs' is 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "No remote write occurs in this plan"
    requirement: CI-01
    verification:
      - kind: manual
        ref: "git status shows no push, remote or tag created; gh is only ever the stub in the control; the one container run used docker with a read-only mount of a local clone"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 20: Workflow rule completion, timing record tool and the mawk fix Summary

**The static workflow checker now has every rule behind a red control (injection guard, forbidden keys, concurrency shape, pin mutation-proven), `scripts/ci-timings` turns a run into a markdown row and is tested offline, `docs/ci-timings.md` says honestly that no CI run exists, and the mawk bug in `record-lint` is fixed and proven in an Ubuntu 24.04 container.**

## Performance

- **Duration:** about 1 h 10 min (2026-11-10), most of it waiting on two emulated linux/amd64 container runs of the whole gate.
- **Tasks:** 3 of 3 (Task 1 tracer, Tasks 2 and 3 auto) plus the D-19-1 fix, done first.
- **Files:** 49 changed outside .planning (1518 added lines, 4 removed).
- **Gate cost (this machine, warm):** `sh scripts/check` 54 s wall over 45 stages, all PASS (D-60 proposed 60 s; time never fails the gate); `dash scripts/check` 50 s; `sh scripts/check --selftest` 10 s. The new stage `ci-timings-controls` takes 0 s.

## Accomplishments

- **D-19-1 fix first (commit da17509).** `scripts/lib/record-rules.awk` line 45 built the per-table key sequence with `kseq[t] = (t in kseq) ? kseq[t] " " k : k`; mawk evaluates the assignment target first, which creates the element, so every first key got a leading space and `record-lint` failed on Ubuntu's default awk. It is now `if (t in kseq) kseq[t] = kseq[t] " " k; else kseq[t] = k` with a comment saying why. Audit: every other `(x in a)` ternary under `scripts/` and `scripts/lib/` assigns to a different variable, and a search for an assignment whose right side or condition tests membership of the same element found only this line.
- **Tracer (Task 1).** `WF_EXPR_IN_RUN` (an expression opener in a run value or run block) and `WF_UNTRUSTED_EXPR` (any expression outside the allowlist github.event.pull_request.base.sha, github.event_name, github.workflow, github.ref, github.sha, or one of them compared with `==` to a quoted word). They scan every non-comment line, block-scalar content included. Control `untrusted-expr` puts a pull request title expression into a run line; its EXPECT is the exact set `WF_EXPR_IN_RUN,WF_UNTRUSTED_EXPR`. The real workflow stays clean.
- **Remaining rules and controls (Task 2).** `WF_FORBIDDEN_KEY` (a `secrets` key or a `secrets.` or `secrets[` reference, `self-hosted`, a `continue-on-error` key, `|| true` and `|| :` in a run block) and `WF_CONCURRENCY` (the top-level group and cancel-in-progress expression exactly). Controls, each a one-line mutation of ok-gate with a single-code EXPECT: tag-pinned-action (WF_UNPINNED_ACTION), no-permissions (WF_NO_PERMISSIONS, the `permissions:` key renamed), continue-on-error, or-true, secrets-ref (all WF_FORBIDDEN_KEY), no-fetch-depth (WF_MISSING_FETCH_DEPTH), wrong-gate-command (WF_NO_GATE_STEP), missing-template-line (WF_TEMPLATE, the line removed from the template), wrong-concurrency (WF_CONCURRENCY). The stage function builds a scratch workflow whose permissions block is a flow map and requires exit 2 both without `--expect` and with `--expect WF_UNPARSABLE`. Selftest `mutation-workflow-pin` deletes the one line tagged `rule-pin` from a scratch copy of the rules and requires the tag-pinned-action control to flip from EXPECT_OK to exit 1.
- **Timing tool (Task 3).** `scripts/ci-timings RUN_ID` runs `gh run view ID --json databaseId,createdAt,startedAt,conclusion,jobs --jq ...` (read-only) and `--tsv FILE` reads the same record shape; both print one row `| date | run id | queue s | job=s, ... | wall s | conclusion |`. Timestamps become epoch seconds by day arithmetic in awk (no GNU date). Queue is run startedAt minus createdAt; wall is the latest job completedAt minus run startedAt (the critical path). A malformed record, a non-numeric run id, a missing gh or a failed gh call exits 2; nothing is written to the repository. `tests/gate/ci-timings` holds a synthetic run that crosses midnight and a leap day (expected row `| 2028-02-29 | 1234567890 | 12 | gate=145, extra=54 | 148 | success |`) and a malformed one (month 13).
- **docs/ci-timings.md.** States that no CI run exists yet because pushing is an owner decision (D-64) and criterion 5 is unverified-pending-push; an empty timing table with its columns; the four failure classes the job owns; the standing protocol (review every run for efficiency wins, revisit caching when a step exceeds 30 s, review the 10-minute timeout against the first critical path, time never fails the gate); and the Pending hand-offs list (Actions default workflow permissions read-only, Actions approving pull requests off, a branch ruleset on main requiring gate), each applied only with the push checkpoint's authorisation (D-80).

## Verification Results

Per-task `<automated>` commands and acceptance checks (all run, results on this machine, macOS, BWK awk 20200816):

| Command | Result |
|---------|--------|
| `sh scripts/check --stage workflow && sh scripts/check --stage workflow-negatives` (Task 1) | `CHECK workflow PASS`, `CHECK workflow-negatives PASS` |
| `sh scripts/workflow-check --root tests/workflow/untrusted-expr --expect WF_EXPR_IN_RUN,WF_UNTRUSTED_EXPR` | `EXPECT_OK`, exit 0; without `--expect` exit 1 listing both codes |
| `sh scripts/workflow-check` | `WORKFLOWS 1`, `STATUS workflow ok`, exit 0 |
| `sh scripts/check --stage workflow-negatives && sh scripts/check --selftest mutation-workflow-pin` (Task 2) | `CHECK workflow-negatives PASS`, `SELFTEST mutation-workflow-pin PASS` |
| `ls tests/workflow \| wc -l` | 12 (plan says 11, see Deviations) |
| `sh scripts/check --stage ci-timings-controls` (Task 3) | `CHECK ci-timings-controls PASS` |
| `sh scripts/ci-timings --tsv tests/gate/ci-timings/canned.tsv` | the row stored in EXPECT, exit 0 |
| `sh scripts/ci-timings --tsv tests/gate/ci-timings/malformed.tsv` | `CI_TIMINGS_MALFORMED line 5: createdAt is not an ISO UTC timestamp`, exit 2 |
| `grep -c 'No CI run' docs/ci-timings.md` / `grep -c 'Pending hand-offs' docs/ci-timings.md` | 1 / 1 |
| `sh scripts/check --stage licences` (SPDX on scripts/ci-timings) | `CHECK licences PASS` |
| plan-end `sh scripts/check` | `CHECK RESULT PASS`, 45 stages (baseline 44 at plan start; the plan text said 18), 54 s warm wall (D-60 proposal 60 s) |
| plan-end `sh scripts/check --selftest` | `SELFTEST RESULT PASS` (includes `mutation-workflow-pin PASS`) |
| plan-end `dash scripts/check` | `CHECK RESULT PASS` |
| `git status` | no push, no remote, no tag; only local commits |

Observed red controls (the failing behaviour, then reverted): a wrong `EXPECT` row turned `ci-timings-controls` red (`canned-row` and `run-id-call` BROKEN); with the rule-pin line deleted the tag-pinned-action control gave `EXPECT_MISMATCH wanted 'WF_UNPINNED_ACTION' got '<none>'`.

Scratch mutations of the real workflow (not committed), each giving the expected codes: an expression in the Tool versions block gives WF_EXPR_IN_RUN and WF_UNTRUSTED_EXPR; `|| true` and `|| :` in that block give WF_FORBIDDEN_KEY; `runs-on: self-hosted` gives WF_FORBIDDEN_KEY and WF_RUNNER; `runs-on: [self-hosted]` is WF_UNPARSABLE (exit 2); a `secrets: inherit` key and a `${{ secrets.X }}` give WF_FORBIDDEN_KEY (the latter also WF_EXPR_IN_RUN in a run block); a github.sha expression in a run block gives WF_EXPR_IN_RUN only; `group: gate` gives WF_CONCURRENCY; an unclosed expression gives WF_UNTRUSTED_EXPR; `github.event_name == 'x y'` gives WF_UNTRUSTED_EXPR.

### D-19-1 proof: one-off local container runs (NOT a CI claim)

These are local runs on this machine, not GitHub Actions runs, and they change nothing about the unverified-pending-push status of criterion 5. Method: `git clone` of the committed repository into a scratch directory, mounted read-only into an `ubuntu:24.04` container (docker, linux/amd64 under emulation), copied to a writable directory with `build/` removed, `apt-get install` of git, gawk, mawk, cmake, ninja-build, clang, gcc, make and pkg-config (gcc 13.3.0, clang 18.1.3, cmake 3.28.3), then `sh scripts/check` once with `update-alternatives --set awk` pointing at mawk 1.3.4 (20240123) and once at gawk 5.2.1.

| Tree | awk | Result |
|------|-----|--------|
| commit da17509 (the fix alone, parent plus one commit) | mawk 1.3.4 | exit 0, `CHECK RESULT PASS`, all 44 stages PASS (including record-lint and record-lint-negatives) |
| commit da17509 | gawk 5.2.1 | exit 0, `CHECK RESULT PASS`, all 44 stages PASS |
| commit 1f6b0d2 (plan-20 end: new awk rules and ci-timings) | mawk 1.3.4 | exit 0, `CHECK RESULT PASS`, all 45 stages PASS |
| commit 1f6b0d2 | gawk 5.2.1 | exit 0, `CHECK RESULT PASS`, all 45 stages PASS |

A first attempt copied the host `build/` directory into the container and failed with CMake cache and exec-format errors; that was a harness mistake on my side (macOS build outputs inside a Linux run), not a repository finding, and the runs above exclude `build/`. Plan 21's container work was not done here.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] D-19-1: record-lint failed under mawk**
- **Found during:** plan 02-19 (a one-off container run), fixed here first as the dispatch directed.
- **Issue:** `kseq[t] = (t in kseq) ? ... : k` creates the element before the test under mawk.
- **Fix:** `if (t in kseq) kseq[t] = kseq[t] " " k; else kseq[t] = k`. No other occurrence found.
- **Files modified:** `scripts/lib/record-rules.awk`; `deferred-items.md` and `.planning/WINDOWS.md` (entry 3) marked fixed.
- **Commit:** da17509

**2. [Rule 2 - Missing critical functionality] A control for the concurrency rule**
- **Found during:** Task 2.
- **Issue:** the plan truth says the concurrency shape has a named bad-workflow control, but its list of eleven directories names none and WF_CONCURRENCY would be an untested rule.
- **Fix:** added `tests/workflow/wrong-concurrency` (cancel-in-progress changed to `true`, EXPECT `WF_CONCURRENCY`). The directory count is therefore 12, not the 11 of the acceptance line.
- **Commit:** ca7c46a

**3. [Rule 2 - Missing critical functionality] A stub-gh control for the RUN_ID mode**
- **Issue:** the plan asks for the timing control to be offline; testing only `--tsv` would leave the gh call line unproven.
- **Fix:** `ci-timings-controls` also runs the RUN_ID mode against a stub gh on PATH that records its arguments, asserts `run view ID --json ... --jq` and that `--tsv` never calls it, and that a non-numeric id exits 2 before gh is looked up. No network is used.
- **Commit:** 1f6b0d2

### Other notes

- The plan text gives a baseline of 18 stages; the real baseline at plan start was 44 (after plan 19), so the gate now has 45 stages (the plan's own "at least 45" is met exactly by this one new stage).
- Letter label of the new selftest comment in `scripts/check` is (v); the plan's selftest name `mutation-workflow-pin` is exact.

## Deferred Issues

None new. D-19-1 is closed. The earlier open item (the 02-01 and 02-02 home-path history finding, WINDOWS.md entry 1) is unchanged and is an owner decision before publication.

## Hand-offs and first-run review items

- Nothing was pushed and no remote setting was changed. The three remote settings (default workflow permissions read-only, Actions approving pull requests off, a branch ruleset requiring gate) stay pending hand-offs in `docs/ci-timings.md` until the push checkpoint (D-64, D-80).
- After the first real run: `sh scripts/ci-timings RUN_ID`, paste the row into `docs/ci-timings.md` in a normal commit, record the efficiency review, and compare the critical path with `timeout-minutes: 10`.

## Notes for later plans

- Any `gate.yml` edit must also update `tests/workflow/ok-gate/.github/workflows/gate.yml` (a byte copy, the stage reports CONTROL_DRIFT otherwise) and every negative mini-root that mutates the changed region; each is a one-line `sed` mutation of the ok-gate copy (the pull-request-target control is regenerated from it the same way). A stale negative shows up as a wrong code set, not as a pass.
- Rule lines in `scripts/lib/workflow-rules.awk` end in tags (`rule-pin`, `rule-trigger`, `rule-expr-in-run`, `rule-untrusted`, `rule-secrets`, `rule-or-true`, `rule-concurrency-group` and so on); only `rule-pin` is mutation-proven so far.
- awk portability rule: never assign to `a[x]` and test `(x in a)` in the same statement.

## Task Commits

0. D-19-1 fix: da17509 `fix(02-20): record-lint under mawk, build the key sequence without lvalue-first evaluation`
1. Task 1 (tracer): 5e9ec91 `feat(02-20): tracer - untrusted-expression rules and the untrusted-expr control`
2. Task 2: ca7c46a `feat(02-20): remaining workflow rules, a control per rule, the unparsable check and the pin mutation`
3. Task 3: 1f6b0d2 `feat(02-20): CI timing record tool, its offline control and docs/ci-timings.md`

## Known Stubs

None. The timing table in `docs/ci-timings.md` has no rows by design (no CI run exists, D-64); this is stated on the page and is not a stub.

## Threat Flags

None. The plan adds no network endpoint, auth path or trust-boundary file access: `scripts/ci-timings` reads a file or runs a read-only `gh run view`, and the controls use a stub.

## Self-Check: PASSED

Created files (scripts/ci-timings, tests/gate/ci-timings, docs/ci-timings.md, the ten new tests/workflow mini-roots) exist; commits da17509, 5e9ec91, ca7c46a and 1f6b0d2 are ancestors of HEAD; the summary and the new files carry no home-directory path.
