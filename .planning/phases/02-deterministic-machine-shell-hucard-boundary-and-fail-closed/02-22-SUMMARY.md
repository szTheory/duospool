---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 22
subsystem: ci
tags: [ci-01, push-checkpoint, github-actions, first-run, awk-default, audit-history, allowlist, ci-timings]
status: complete
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: scripts/check with 47 stages, .github/workflows/gate.yml, scripts/ci-timings and docs/ci-timings.md (02-19, 02-20), the documented gate command and the Linux container run (02-21)
provides:
  - the owner's answer to the push checkpoint recorded verbatim, and acted on exactly (one branch, one pull request, nothing else remote)
  - the first GitHub-hosted run of the gate (run 42939483070, pull request 1, success, critical path 100 s) with its timing row, the awk default and an efficiency review in docs/ci-timings.md
  - scripts/audit-history accepts a kind-scoped allowance (TEXT-PATTERN:KIND), two owner-approved allowlist entries, three controls that fail against the previous script; full-history audit is clean
affects: [phase 2 verification (success criterion 5), the owner's pending remote-settings hand-offs, later CI runs (compare against the 100 s row)]

actuals:
  tokens: 3086     # chars/4 over the realized diff excluding .planning (12342 chars, 5 non-planning files: audit-history, audit-history.allow, synthetic.sh, ci-timings.md and the planning-only files excluded)
  tasks: 2         # task 1 was the owner decision (answered before this run), task 2 executed
  commits: 2       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 91e139ae4d0df72cbcc7827eff8a9bd79b129a56
plan_head_after: 7c7fe41a1d1665a6f4a1afc99343ce8d55c7960f
commits: 2

tech-stack:
  added: []
  patterns:
    - "an allowance is as narrow as the tool can express: when the format only scoped by path, the format gained a kind scope and a control that fails without it, instead of widening the allowance"
    - "CI evidence is recorded where it was observed: image version, awk path and per-step seconds come from the run log and step timestamps, and what the API cannot return (the job summary) is stated as not recorded"

key-files:
  created: []
  modified:
    - scripts/audit-history
    - scripts/audit-history.allow
    - tests/audit/synthetic.sh
    - docs/ci-timings.md
    - .planning/WINDOWS.md
    - .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-VALIDATION.md
    - .planning/REQUIREMENTS.md

key-decisions:
  - "checkpoint answer authorised-branch-pr (owner, 2026-11-10, verbatim: \"Push branch + PR, allowlist the 2 paths (Recommended)\"): one branch, one pull request, no settings, no ruleset, no tag, no push to main"
  - "the two home-path-mac hits are allowlisted by kind (TEXT-PATTERN:home-path-mac) with the owner's reason, not by a path-wide TEXT-PATTERN line, so a credential added to those paths later is still reported"
  - "no workflow change after the first run: nothing measured justifies a cache, a tighter timeout-minutes or dropping the second awk run"
  - "CI-01 is ticked on the green run of the pushed head b063518 with its row recorded; the remote settings stay pending hand-offs and are not part of CI-01's evidence"

requirements-completed: [CI-01]

coverage:
  - id: D1
    description: "Success criterion 5 has an honest recorded status: a real GitHub-hosted run of the gate, with its critical-path time"
    requirement: CI-01
    verification:
      - kind: ci
        ref: "gh run 42939483070 (pull request 1, head b063518, ubuntu-24.04 image 20261101.360.1): job gate success in 98 s, run wall 100 s, queue 0 s, CHECK RESULT PASS twice (gawk, then mawk); sh scripts/ci-timings 42939483070 row present exactly once in docs/ci-timings.md"
        status: pass
    human_judgment: false
  - id: D2
    description: "The push was authorised and bounded: exactly one new remote branch, one pull request, nothing else"
    requirement: CI-01
    verification:
      - kind: command
        ref: "git ls-remote --heads --tags origin: main unchanged at 8ba0bc6, the phase branch added, the only tag is the pre-existing spike-tool-01-evidence; read-only GETs show no ruleset and the Actions permission settings untouched"
        status: pass
    human_judgment: false
  - id: D3
    description: "The two known audit hits are allowlisted by kind with a written reason and the allowance does not hide a new home path elsewhere"
    requirement: CI-01
    verification:
      - kind: integration
        ref: "sh scripts/audit-history exit 0 (197 commits, clean); sh tests/audit/synthetic.sh: text-allow-kind-ok, text-allow-kind-other-path, text-allow-kind-other-kind PASS (and FAIL against the previous script); scratch clone: a new mac-shape path in another file and a linux-shape path inside an allowed summary are both reported"
        status: pass
    human_judgment: false
  - id: D4
    description: "The remote hardening settings (Actions default permissions, Actions approving pull requests, a ruleset on main) are applied only by the owner"
    requirement: CI-01
    verification: []
    human_judgment: true
    rationale: "Owner authority gate: the answer did not authorise settings; they stay pending hand-offs listed in docs/ci-timings.md"
---

# Phase 2 Plan 22: Push checkpoint and first CI run Summary

**The owner authorised one branch and one pull request, and the first GitHub-hosted run of the gate passed in 100 s on ubuntu-24.04 (default awk gawk, second awk mawk), so success criterion 5 is verified at evidence tier E0 (repeatability, not accuracy).**

## Checkpoint outcome

- **Answer token:** `authorised-branch-pr`. The owner's selection, verbatim: "Push branch + PR, allowlist the 2 paths (Recommended)", chosen in the /gsd-autonomous session on 2026-11-10, answering the question about pushing one phase branch and opening a pull request for the first CI run. It is not `authorised-branch-pr-and-settings`.
- **Applied:** branch `gsd/phase-02-deterministic-machine-shell-hucard-boundary-and-fail-closed` created from the local head and pushed (no upstream tracking); pull request 1 into `main` opened with the PR template filled in (rights checklist, `sh scripts/check` result, evidence tier E0, the unverified and one-off labels). `main` was not pushed to, no tag was created, nothing was force-pushed or deleted. `git ls-remote` shows `main` unchanged at 8ba0bc6.
- **Not applied (pending hand-offs for the owner):** Actions default workflow permissions read-only, Actions approving pull requests off, a branch ruleset on `main` requiring `gate`. A read-only look at the repository shows items 1 and 2 are already at those values and that no ruleset exists; none was changed.
- **Success criterion 5:** verified with run id 42939483070. `docs/ci-timings.md` carries the exact line `CI-CHECKPOINT: verified (answer: authorised-branch-pr) run 42939483070` once, and the `sh scripts/ci-timings 42939483070` row once.

## Performance

- **Duration:** about 35 min of executor time, most of it the full-history audit (about 100 s each run) and the full gate (about 2 min each run) on this machine.
- **Tasks:** 2 of 2 (task 1 answered by the owner before this run).
- **Files modified:** 7.

## First CI run (what it shows)

Run 42939483070, `gate` job, `ubuntu-24.04` image version 20261101.360.1, runner 2.338.2.

| Fact | Value |
|------|-------|
| Conclusion | success; job 98 s, run wall (critical path) 100 s, queue 0 s |
| Default awk (D-74) | `/usr/bin/gawk` |
| Second awk | `/usr/bin/mawk`, already on the image, so the `apt-get` fallback did not run |
| Full gates | `CHECK RESULT PASS`, 47 stages, under gawk and again under mawk |
| Steps | set up and checkout 2 s, Gate 37 s, Range audit 16 s, gcc 7 s, clang 9 s, second awk run 25 s |
| Slowest stages | build 7 s (cold), conformance 6 s, sanitizers 5 s, the rest 2 s or less |

Not recorded: the cmake, ninja, gcc and clang versions. The `Tool versions` step writes them to the job summary, which the Actions API and `gh` return as null. The local baseline of 45 s on a quiet developer machine makes the runner about 2 times slower with the range audit and second awk run included.

## Efficiency review

No cache (nothing is downloaded, the Gate step is 37 s only as the sum of 47 stages none over 7 s); the Gate step's `conformance` stage and the `gcc build and conformance` step overlap by about 7 s, kept on purpose; the second awk run (25 s) is the point of failure class 4; `timeout-minutes: 10` is 6 times the critical path, to be reconsidered after three or more rows; the range audit (16 s) is large only because this first pull request holds the whole phase. Nothing was changed and nothing was weakened (D-63). One carry-over from plan 02-21: the CI gate builds with gcc, so a leak only clang with LeakSanitizer finds would not fail CI, because the clang step runs conformance but not the sanitizers stage. That is a later plan's decision, not made here.

## Task Commits

1. Precondition work, allowlist (owner-approved): b063518 `fix(02-22): allowlist the two known home-path-mac hits by kind, with controls`
2. Task 2: 7c7fe41 `docs(02-22): record the first CI run, the owner answer, the awk default and the efficiency review`

The pushed head that ran in CI is b063518. Commits after it (task 2 docs, this SUMMARY, state updates) were pushed to the same branch afterwards; they change only `docs/` and `.planning/`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] A kind-scoped audit allowance**
- **Found during:** the owner-approved allowlist step before Task 2
- **Issue:** `scripts/audit-history` matched a text hit by path and the fixed check name `TEXT-PATTERN`, so the only expressible allowance for the two summaries would also have hidden a credential or a Linux home path added to those files later. The owner asked for the allowance to be as narrow as the format allows, naming the `home-path-mac` pattern.
- **Fix:** the final decision stage also accepts the check `TEXT-PATTERN:KIND` (one kind in one path); the wide form still works; the header comments and the allowlist's format line say to prefer the narrow form. Three controls were added to `tests/audit/synthetic.sh`: the allowed kind is clean, a second path is still reported, a second kind in the same path is still reported. All three fail against the previous script (run in a scratch copy).
- **Files modified:** scripts/audit-history, scripts/audit-history.allow, tests/audit/synthetic.sh
- **Verification:** `sh scripts/audit-history` exit 0 (197 commits); `sh scripts/check` PASS (47 stages), `--selftest` PASS, `sh scripts/rights-check` PASS; a scratch clone showed a new mac-shape path in another file and a Linux-shape path inside an allowed summary are both reported.
- **Commit:** b063518

**Total deviations:** 1 auto-fixed (1 missing critical functionality). **Impact:** a small change to a project-owned tool that makes the owner's approved allowance narrower than the unchanged tool could express; no control was weakened.

**Residual (not a defect):** the allowance is per path and kind, not per commit, so a new mac-shape home path written into those same two summary files would not be reported. The files are executor summaries that are not edited again; review any diff of them.

## Windows ledger

`.planning/WINDOWS.md` entry 1 (unmet-truth, the two home-path hits) is marked waived with the reason "resolved-by-allowlist", because the ledger has no separate "resolved" status (its states are open, waived and fixed). It now reads open_count 0.

## Issues Encountered

None that blocked. One shell slip: the first push used a zsh variable followed by a colon, which zsh read as a modifier, so the refspec was malformed and git rejected it locally before any network write; the corrected push followed.

## Known Stubs

None.

## Threat Flags

None. T-02-43 (unauthorised push) is mitigated: the push, the pull request and nothing else followed the owner's recorded answer; the local gate, selftest, rights check and full-history audit were green at the commit pushed. T-02-44 (injection through fetched content) is mitigated: gh output and logs were read as data only.

## Next Phase Readiness

Phase 2 plans are all executed. Open for the owner: the three remote-settings hand-offs, and the pull request itself (this plan opened it; it did not merge it). Phase verification should read criterion 5 as verified by run 42939483070 at E0.

## Self-Check: PASSED

Commits b063518 and 7c7fe41 are ancestors of HEAD; docs/ci-timings.md holds exactly one `CI-CHECKPOINT:` line (verified shape, run 42939483070) and the exact ci-timings row once; `git ls-remote --heads origin` showed `main` at 8ba0bc6 and the one phase branch; no home-directory path appears in the files this plan wrote.
