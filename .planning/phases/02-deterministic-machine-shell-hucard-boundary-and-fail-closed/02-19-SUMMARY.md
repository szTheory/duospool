---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 19
subsystem: ci
tags: [github-actions, workflow-check, restricted-yaml, sha-pin, least-privilege, dependabot, pr-template, gitattributes, awk-dialect, ci-01]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: the gate with 42 stages and run_controls (02-01 to 02-18), conformance-run run --preset (02-05), audit-history --range (02-04), decision records 0006 and 0007 (Phase 1)
provides:
  - .github/workflows/gate.yml, one job named gate on ubuntu-24.04 that runs sh scripts/check, a range audit, a gcc run, a clang run and a second-awk run
  - scripts/workflow-check with scripts/lib/workflow-rules.awk, a restricted block-style YAML line checker (exit 2 outside the subset) and eleven structural WF_ codes plus WF_UNPARSABLE
  - tests/workflow (ok-gate, pull-request-target) and the stages workflow and workflow-negatives (before build); the gate now has 44 stages
  - .github/pull_request_template.md (record 0007 line verbatim, asserted with grep -qxF), .github/dependabot.yml, .github/CODEOWNERS, .gitattributes
affects: [02-20 (adds WF_FORBIDDEN_KEY, WF_EXPR_IN_RUN, WF_UNTRUSTED_EXPR, WF_CONCURRENCY, the other nine controls, mutation-workflow-pin and ci-timings), 02-21 (record 0007 sentence), 02-22 (the push checkpoint, criterion 5), any later workflow edit (the ok-gate copy must be refreshed with it)]

actuals:
  tokens: 9856     # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 39424 chars, 15 files, 776 added lines)
  tasks: 2
  commits: 2       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 78e9bbcfd9df3300dfd0e163142e6415b8f8b8dd
plan_head_after: b750c5addb9a343876ac3a72152d2dedad2c97f6
commits: 2

tech-stack:
  added: []
  patterns:
    - "a static checker for a format it does not parse: a restricted line checker accepts a small named subset, and every other shape is WF_UNPARSABLE with exit 2 that an --expect cannot name, so a file it cannot read is never a pass"
    - "one control mini-root is a byte copy of the real file (cmp in the stage), so the controls are mutations of what CI actually runs and cannot drift from it"
    - "a rule that must be mutation-proven is one self-contained awk statement ending in a rule-NAME tag"

key-files:
  created:
    - .github/workflows/gate.yml
    - .github/pull_request_template.md
    - .github/dependabot.yml
    - .github/CODEOWNERS
    - .gitattributes
    - scripts/workflow-check
    - scripts/lib/workflow-rules.awk
    - tests/workflow/ok-gate/ (gate.yml, template, dependabot under .github)
    - tests/workflow/pull-request-target/ (the same three files with the trigger replaced, EXPECT WF_FORBIDDEN_TRIGGER)
    - .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/deferred-items.md
  modified:
    - scripts/check

key-decisions:
  - "the checkout action pin was re-verified before writing: git ls-remote on refs/tags/v7.0.1 still returns 3d3c42e5aac5ba805825da76410c181273ba90b1 and the peeled form returns nothing (a lightweight tag), so the research value stands unchanged"
  - "WF_UNPARSABLE is exit 2 whatever --expect says: a control cannot assert it, so the unparsable shape is asserted in the stage function by plan 20 as the plan fixes"
  - "the ok-gate mini-root must equal the real workflow byte for byte (CONTROL_DRIFT otherwise), which makes every later mutation control a mutation of the file CI runs"
  - "the second awk is the other of gawk and mawk (gawk if the default is anything but gawk), put first on PATH by a symlink in a temporary directory; apt installs it only if it is absent"
  - "the Linux container run found one awk-dialect failure in code owned by an earlier plan (record-lint under mawk); it is logged in deferred-items.md and not fixed here (scope boundary)"

patterns-established:
  - "a workflow edit has three coupled files: the real gate.yml, the ok-gate copy and the pull-request-target mutation of it; the stage workflow-negatives names the drift"
  - "comments in gate.yml avoid the literal strings that acceptance greps count (the action key, the build-grid word, the forbidden trigger names)"

requirements-completed: [CI-01]

coverage:
  - id: D1
    description: "A least-privilege, pinned workflow named gate runs the one local command (sh scripts/check) on ubuntu-24.04 for pull requests, pushes to main and manual dispatch"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage workflow PASS (STATUS workflow ok, WORKFLOWS 1); actionlint .github/workflows/gate.yml exit 0 with shellcheck installed; grep -c on the pinned SHA line is 1, on the forbidden trigger name 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "workflow-check asserts the structural and pinning rules, fails closed outside the YAML subset, and shows a forbidden trigger red"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "sh scripts/check --stage workflow-negatives PASS (ok-gate accepted, pull-request-target gives exactly WF_FORBIDDEN_TRIGGER); scratch mutations: 24 one-line variants each gave its single expected code or exit 2 (listed in this summary); a wrong EXPECT gave CONTROL BROKEN and a one-line edit of the ok-gate copy gave CONTROL_DRIFT (observed once each, reverted)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The CI-only steps (range audit, gcc, clang, second awk) and the routine files exist; the pull-request template carries the record 0007 line"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "workflow stages PASS after the extension; grep -c uses: is 1 and grep -c matrix is 0; the four step names match once each; git ls-files --eol captured to a scratch file has 0 lines with i/crlf; the second-awk step text was run in an Ubuntu 24.04 container (mawk default, gawk installed, shim first on PATH, STATUS workflow ok)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The job runs green on the real runner, and the second-awk method and the 10-minute timeout hold there"
    requirement: CI-01
    verification: []
    human_judgment: true
    rationale: "no CI run exists: pushing is an owner decision (D-64), so success criterion 5 stays unverified-pending-push; a local Ubuntu 24.04 container run is evidence about dialects, not a CI result"
  - id: D5
    description: "The CODEOWNERS default owner is the intended handle"
    requirement: CI-01
    verification: []
    human_judgment: true
    rationale: "the handle comes from git config user.name and nothing independent says it is the owner's GitHub login; owner confirmation is needed"

duration: about 25 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 19: The CI definition and its static workflow check Summary

**A pinned, least-privilege `gate` workflow (one job on ubuntu-24.04: the gate, a range audit, a gcc run, a clang run and a second-awk run) judged by `scripts/workflow-check`, a fail-closed restricted-YAML line checker with eleven structural codes and two controls, plus the template, dependabot, CODEOWNERS and gitattributes files.**

This is a change detector for the shape of the CI definition, not a CI result. Evidence tier E0 (static checks that can be shown red); nothing here says anything about hardware, and no CI run exists, so success criterion 5 stays **unverified-pending-push** (D-64).

## Performance

- **Duration:** about 25 min (2026-11-10)
- **Tasks:** 2 of 2 (Task 1 tracer, Task 2 auto)
- **Files:** 15 changed outside .planning (776 added lines, none removed)
- **Gate cost (this machine, warm):** `sh scripts/check --timing` 44 s wall over 44 stages, all PASS, no `TIMING_WARN`; the two new stages take 0 s each (one awk run over one file, and a handful of mini-roots).

## Accomplishments

- **Tracer (Task 1).** `gate.yml` in block-style YAML: name gate, triggers `pull_request`, `push` to main (block list, not a flow sequence) and `workflow_dispatch`, `permissions: contents: read`, the concurrency group with `cancel-in-progress` only for pull requests, `defaults.run.shell: sh -eu {0}`, one job `gate` with `timeout-minutes: 10`, and the first three steps (checkout pinned to the full SHA with the version comment, `fetch-depth: 0`, `persist-credentials: false`; Tool versions to the step summary; Gate running exactly `sh scripts/check`). `scripts/lib/workflow-rules.awk` is the restricted line checker (it tracks indentation, open blocks, list items, block scalars and duplicate keys, and refuses every other shape) and carries the rules; `scripts/workflow-check` runs it per workflow file, asserts the template line and the dependabot file, and prints violations or `STATUS workflow ok`. Controls `ok-gate` and `pull-request-target`; stages `workflow` and `workflow-negatives` registered before `build`.
- **Expansion (Task 2).** The Range audit step (pull requests only; `BASE_SHA` reaches the command through `env`, the run line is `sh scripts/audit-history --range "${BASE_SHA}..HEAD"`), the gcc and clang steps (`sh scripts/conformance-run run --preset gate-gcc` and `gate-clang`), and the Second awk gate run (the other of gawk and mawk goes first on `PATH` through a symlink in `RUNNER_TEMP`; apt installs it only if it is absent). The job comments state the four failure classes and the two first-run review items. `.github/CODEOWNERS` (default owner line, no enforced review, no branch protection) and `.gitattributes` (`* text=auto eol=lf`, binary patterns marked `binary`).
- **Rules and codes.** WF_NO_PERMISSIONS, WF_EXCESS_PERMISSION (any permission other than `contents: read`, at any level, or a scalar `permissions` value), WF_UNPINNED_ACTION (the ref is not exactly 40 lowercase hex characters, checked by length and a character class, no regex interval), WF_UNEXPECTED_ACTION, WF_FORBIDDEN_TRIGGER, WF_MISSING_FETCH_DEPTH, WF_PERSIST_CREDENTIALS, WF_SHELL, WF_RUNNER, WF_NO_GATE_STEP, WF_TEMPLATE, WF_DEPENDABOT and WF_UNPARSABLE (exit 2). Rule lines carry tags (`rule-pin`, `rule-trigger`, `rule-perm-scope`, `rule-fetch-depth` and so on) for plan 20's selftest; `grep -c rule-pin scripts/lib/workflow-rules.awk` is 1.

## Verification Results

Per-task `<automated>` commands (exit codes and key lines):

| Command | Result |
|---------|--------|
| Task 1: `sh scripts/check --stage workflow && sh scripts/check --stage workflow-negatives` | exit 0: `CHECK workflow PASS`, `CHECK workflow-negatives PASS`. The tracer gate (auto mode) re-ran it after the commit with the same result, so expansion went ahead (`Tracer verified end-to-end - expanding`) |
| Task 2: `sh scripts/check --stage workflow && sh scripts/check --stage workflow-negatives && actionlint .github/workflows/gate.yml` | exit 0: two `CHECK ... PASS` lines; actionlint 1.7.12 printed nothing (zero errors; shellcheck is installed, so the run scripts were linted too) |

Acceptance criteria, all run:

- Task 1: `sh scripts/workflow-check` prints `STATUS workflow ok` and exits 0 on the repository root (also under dash). `grep -c '@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1' .github/workflows/gate.yml` is 1. `grep -c 'pull_request_target' .github/workflows/gate.yml` is 0 (the control copy holds the one deliberate occurrence). `grep -qxF` of the record 0007 line on the template exits 0. `head -5` of `scripts/workflow-check` and `scripts/lib/workflow-rules.awk` each holds `SPDX-License-Identifier: MIT OR Apache-2.0`; `sh scripts/check --stage licences` PASS. actionlint exit 0.
- Task 2: `grep -c 'uses:'` is 1 and `grep -c 'matrix'` is 0; the four step names (Range audit, gcc build and conformance, clang build and conformance, Second awk gate run) each match once, so the count is 4. `git ls-files --eol` captured to a scratch file: `grep -c 'i/crlf'` prints 0 (also 0 for `w/crlf`); 905 files are `i/lf w/lf`, 4 are `i/none`. actionlint exit 0, output empty.

Controls observed red and reverted: the `pull-request-target` EXPECT changed to `WF_SHELL` gave `CONTROL BROKEN pull-request-target (wanted WF_SHELL got WF_FORBIDDEN_TRIGGER)` and `CHECK workflow-negatives FAIL`; one appended comment line in the ok-gate copy gave `CONTROL_DRIFT` and FAIL; both reverted and PASS again.

Scratch mutations of `gate.yml` (24 one-line variants, each judged by the real checker; none committed): tag instead of SHA, short SHA and upper-case SHA each gave only WF_UNPINNED_ACTION; another action name gave WF_UNEXPECTED_ACTION (plus WF_MISSING_FETCH_DEPTH, because no checkout step remains); no permissions block gave only WF_NO_PERMISSIONS; contents write, an extra permission, `read-all` and a job-level write each gave only WF_EXCESS_PERMISSION; no or wrong fetch-depth gave only WF_MISSING_FETCH_DEPTH; no or true persist-credentials gave only WF_PERSIST_CREDENTIALS; shell bash and no default shell gave only WF_SHELL; `ubuntu-latest` and `self-hosted` gave only WF_RUNNER; a wrong gate command gave only WF_NO_GATE_STEP; `workflow_run` gave only WF_FORBIDDEN_TRIGGER; a flow map, an anchor, a tab, a duplicate key and a second document each gave WF_UNPARSABLE with exit 2.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check --timing` | exit 0, 44 `CHECK <stage> PASS` lines (42 before this plan) and `CHECK RESULT PASS`, 44 s; `build/gate/results.tsv` has 44 lines |
| `dash scripts/check` | exit 0, ends `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | exit 0, 27 `SELFTEST ... PASS` lines (unchanged: no mutation was added here) and `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range 78e9bbc..HEAD` | `audit-history: clean` (2 commits, 12 blobs) |
| `actionlint .github/workflows/gate.yml` | exit 0, zero errors (one-off, not part of the gate) |
| remote writes | none: no push, no tag, no pull request, no branch ruleset; `git status` shows only the pre-existing untracked `.planning/state.json` |

One-off Linux container evidence (Ubuntu 24.04, `linux/amd64` under emulation on an arm64 host, gcc 13.3.0, clang 18.1.3, cmake 3.28.3 installed with apt; a local clone of HEAD; this is not a CI run):

- `sh scripts/workflow-check` and both controls passed under mawk 1.3.4 and under gawk 5.2.1 (so the new awk file is readable by both Linux dialects as well as BWK awk 20200816).
- The Second awk step text (extracted from `gate.yml`, with `sudo` provided by a pass-through script) ran under `sh -eu` (dash): default awk mawk, gawk absent, apt installed it, the shim made `awk` resolve to gawk (`second run awk: /usr/bin/gawk`). Installing gawk also made it the system default, as the research recorded, so the shim is belt and braces.
- The whole gate (`sh scripts/check`, all 44 stages including the unchanged build, sanitizers, determinism and fixtures) on that image gave **one failure: `CHECK record-lint FAIL` under mawk** (every other stage PASS). It is an awk lvalue evaluation-order trap in `scripts/lib/record-rules.awk` line 45, owned by an earlier plan; see Deferred Issues. With a scratch-only one-line change in the container copy, the whole gate passed under mawk and again under gawk. Nothing was changed in the repository for it.

## Deviations from Plan

None - plan executed exactly as written. Two things are additions inside the plan's intent, listed as interpretation choices.

### Interpretation choices (inside the plan's intent)

1. **Block list for the push branches.** The research skeleton has `branches: [main]`; a flow sequence is outside the restricted subset the plan itself fixes ("no flow maps"), so the file has a two-line block list.
2. **`CONTROL_DRIFT`.** `workflow-negatives` fails if `tests/workflow/ok-gate/.github/workflows/gate.yml` differs from the real workflow. The plan says ok-gate is a copy; the stage makes that a checked property, so a later workflow edit cannot leave the controls testing an older file. `pull-request-target` is the one-line mutation of the same file.
3. **WF_UNPARSABLE and `--expect`.** Unparsable always exits 2 and cannot be matched by an expectation, so a control cannot pass by naming it. Duplicate keys inside one mapping and a plain value holding `: ` are also unparsable (real YAML rejects both), as are CR characters, tabs in indentation, odd indents, anchors, tags, flow collections and a second document.
4. **Code boundaries.** WF_NO_PERMISSIONS means no top-level `permissions` block; a scalar value (`read-all`, `write-all`) is WF_EXCESS_PERMISSION, so each bad variant gives one code. A workflow with no checkout step gives WF_MISSING_FETCH_DEPTH (no full history). The Gate step is the step named `Gate` whose `run` is the single line `sh scripts/check`; a `run: |` block is not accepted as the gate command. A workflow directory without `gate.yml` gives WF_NO_GATE_STEP; a missing `.github/workflows` directory is exit 2.
5. **Second-awk selection.** The other implementation is mawk when the default resolves to gawk and gawk otherwise (including an unknown default), so the step cannot fail on an unexpected image; the awk identity of both runs is printed.
6. **Dependabot assertion.** `WF_DEPENDABOT` checks four whole lines with `grep -qxF` (version 2, the github-actions ecosystem, monthly, a `groups:` key); the file itself groups all actions in one group.
7. **Words avoided in `gate.yml` comments.** The acceptance greps count the action key, the grid word and the forbidden trigger names, so comments do not use those literal strings.

**Total deviations:** 0 auto-fixed, 7 interpretation choices. **Impact:** none on the plan's intent.

## Deferred Issues

- **record-lint fails under mawk (earlier plan's code).** `CHECK record-lint FAIL` on Ubuntu's default awk, cause and verified one-line fix in `deferred-items.md` (D-19-1). It was found by the one-off container run, which is the awk-dialect failure class that the job's second awk run is for. If left, the first real CI run is red at the gate step. Also recorded in `.planning/WINDOWS.md` as a `fixme`.

## Hand-offs and first-run review items

- **CODEOWNERS handle.** `.github/CODEOWNERS` names the default owner from `git config user.name` (`szTheory`). The owner should confirm this is the GitHub login; review is not enforced and no branch protection is set (D-64).
- **timeout-minutes 10 (D-61).** The second gate run and the two compiler runs add to the job. The local container timings (about 4 min 20 s for install plus one gate run under emulation) suggest the limit is not obviously tight, but only a real run's critical path can settle it; plan 20's `docs/ci-timings.md` records the review item.
- **Second awk (D-74).** The first real run's Tool versions step prints which awk is the default and whether gawk is preinstalled; the apt fallback stays until then.
- **Remote settings (D-80).** Actions default permissions read-only, Actions approving pull requests off and a branch ruleset requiring `gate` are not applied (no remote write here); plan 20's `docs/ci-timings.md` lists them as pending hand-offs.

## Notes for later plans

- **Plan 20** adds the remaining codes to `scripts/lib/workflow-rules.awk` (the tag convention is one self-contained statement per rule) and nine mutation controls under `tests/workflow`; each is a one-line mutation of `tests/workflow/ok-gate`, which stays equal to the real `gate.yml`. `mutation-workflow-pin` deletes the line tagged `rule-pin`. The unparsable case is exit 2 and cannot be an `EXPECT`; assert it in the stage function.
- **Any workflow edit** must be copied to `tests/workflow/ok-gate/.github/workflows/gate.yml` (and the `pull-request-target` mutation regenerated with `sed 's/^  pull_request:$/  pull_request_target:/'`), and followed by a one-off `actionlint .github/workflows/gate.yml`.
- **Plan 21** updates record 0007's sentence about the template; the template now exists and `workflow-check` asserts its line.

## Task Commits

1. **Task 1 (tracer): the pinned least-privilege workflow judged by workflow-check, with a forbidden trigger shown red** - `aab0a4c` (feat)
2. **Task 2: CI-only steps (range audit, gcc, clang, second awk), CODEOWNERS and gitattributes** - `b750c5a` (feat)

## Known Stubs

None. No placeholder text, empty data flow or unwired component was added.

## Threat Flags

None. The new surface is the workflow definition itself, which the plan's threat model covers: T-02-39 is mitigated by the single action pinned to a full SHA (re-verified before writing: tag v7.0.1 still resolves to 3d3c42e5aac5ba805825da76410c181273ba90b1, a lightweight tag), WF_UNPINNED_ACTION and WF_UNEXPECTED_ACTION (observed red on scratch variants), `permissions: contents: read` with WF_EXCESS_PERMISSION, `persist-credentials: false` with WF_PERSIST_CREDENTIALS and the monthly grouped dependabot update. T-02-40 is mitigated for triggers by WF_FORBIDDEN_TRIGGER with the committed `pull-request-target` control; the secrets, self-hosted, continue-on-error and pipe-to-true rules and their controls remain plan 20's (a self-hosted runner already gives WF_RUNNER here). No secret, self-hosted runner or write permission exists. T-02-SC: no package-manager install ran in the repository (the container run installed Ubuntu packages in a throwaway container only).

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `.github/workflows/gate.yml`, `.github/pull_request_template.md`, `.github/dependabot.yml`, `.github/CODEOWNERS`, `.gitattributes`, `scripts/workflow-check`, `scripts/lib/workflow-rules.awk`, `tests/workflow/ok-gate/.github/workflows/gate.yml`, `tests/workflow/pull-request-target/EXPECT`, and the phase `deferred-items.md`.
- Commits `aab0a4c` and `b750c5a` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 2.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
