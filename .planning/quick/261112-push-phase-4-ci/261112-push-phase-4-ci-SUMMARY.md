---
phase: quick
plan: 261112-push-phase-4-ci
subsystem: ci
tags: [push, pull-request, stacked-pr, ci, gate, first-hosted-run]
status: complete
commits: 2
plan_head_before: cb2ddeb6e6af1267c3d07fe109496ab1f1b9da50
plan_head_after: 6c386c8 (the docs commit; the SUMMARY commit follows it)
---

# Quick 261112-push-phase-4-ci: owner-authorised stacked push of Phase 4 and the first CI run

Pushed one new branch and opened one pull request whose base is the Phase 3 branch (the head of pull request 2), under the owner's recorded answer "Stacked PR on the Phase 3 branch (Recommended)" (2026-11-12).
The first hosted run of the Phase 4 code was green on the first try, with no fix-forward.
The evidence tier is E0, repeatability only: the gate passes on a second machine class with a second awk and two compilers;
no profile is claimed accurate.

## Refs

Before (`git ls-remote origin`): `main` 8ba0bc6, `gsd/phase-02-...` 3958316 (`refs/pull/1/head` 3958316, `refs/pull/1/merge` f6cb1b6), `gsd/phase-03-...` 6627e50 (`refs/pull/2/head` 6627e50, `refs/pull/2/merge` 339a27a), tag `spike-tool-01-evidence` b605e93.
After: exactly one new branch ref, `refs/heads/gsd/phase-04-vce-vdc-raw-video`, first at cb2ddeb, then fast-forwarded to 0ce50ba by one docs commit;
GitHub added `refs/pull/3/head` (0ce50ba) and `refs/pull/3/merge` (610fad9).
`main`, the Phase 2 and Phase 3 branches, `refs/pull/1/*`, `refs/pull/2/*` and the tag are byte-for-byte as before.
No tag, no force, no upstream tracking, no settings change, nothing merged.

Pull request: https://github.com/szTheory/duospool/pull/3 (base `gsd/phase-03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad`, head `gsd/phase-04-vce-vdc-raw-video`, state open).
The body is the repository template skeleton plus the scope, evidence-tier, sprites-deferred, stacked and never-run-before statements;
it ends with the required attribution line.
It is not merged and not retargeted.

## CI trigger check (before any push)

`.github/workflows/gate.yml` has `on: pull_request:` with no `branches` filter (plus `push` on `main` and `workflow_dispatch`), so a pull request into a non-`main` base runs the gate. It did. No retarget and no workflow edit was needed.

## Preconditions (all run on cb2ddeb before the push, all passed)

| Command | Result |
|---------|--------|
| `git fetch origin` | read-only; the remote matched the expectation |
| `sh scripts/check` | PASS, 57 stages, 478 s wall (loaded machine) |
| `sh scripts/check --selftest` | exit 0 |
| `sh scripts/rights-check` | exit 0 |
| `sh scripts/audit-history` (full history, no `--range`) | clean; 480 commits, 3704 blobs, 4012 blob-path pairs |
| `git ls-files '*.pce' '*.pdf'` | empty |
| scan of the diff of the Phase 3 remote branch against HEAD for home paths, scratch paths and credential shapes | no credential shape; no real home or scratch path; the only hits are three prose lines in already-committed planning notes that mention the scratch-tree prefix (see the owner note below) |
| `LINUX_REPRO_PARTS='gate-gcc gate-clang core-boundary-gcc' sh scripts/linux-repro` | PASS (gcc 13.3.0 strict C11 `-Werror`, clang 18.1.3, core-boundary under gcc), 88 s, emulated amd64 |

## CI runs

| Run | Head | Event | Job s | Run s | Conclusion |
|-----|------|-------|-------|-------|------------|
| 43275379025 | cb2ddeb | pull_request | 853 | 856 | success (the first Phase 4 run; recorded in `docs/ci-timings.md`) |
| 43278274460 | 0ce50ba | pull_request (the docs commit) | 1048 | 1051 | success (not recorded in `docs/ci-timings.md`: a second docs commit would start a third run, and the brief says record once) |

Both runs: queue 0 s, runner image `ubuntu-24.04` 20261101.360.1, default awk gawk and second awk mawk (apt fallback not needed), 57 stages under each awk, `CHECK RESULT PASS` twice per run. `fixture-build` and `fixtures` passed, so the fixture hash pins (taken on macOS) reproduce with the Ubuntu cc65 package. No pin was touched.

Step seconds (run 1 / run 2): set up, checkout, tool versions 3 / 5; Install cc65 10 / 10; Gate 339 / 419; Range audit 15 / 18; gcc build and conformance 74 / 93; clang build and conformance 80 / 120; Second awk gate run 330 / 381.
Slowest gate stages, run 1 (gawk, then mawk): `sanitizers` 172 and 170 s, `conformance` 72.5 and 71.6 s, `determinism` 61.6 and 61.8 s; every other stage 3.1 s or less (`video-check` 1.8 s, `build` 1.5 s cold, `named-controls` 2.1 s). Per-stage figures of run 2 were not extracted.

## Efficiency review

Recorded in full in `docs/ci-timings.md` ("First Phase 4 CI run", pushed in 0ce50ba). Summary, with the second run added:

- **Job time vs Phase 3 first run (342 s):** 853 s, then 1048 s on the same code and a docs-only change: 2.5 and 3.1 times. The same 3 stages (`sanitizers`, `conformance`, `determinism`) are 90 percent of the gate. No Phase 3 row of 541 s exists in `docs/ci-timings.md`; the only Phase 3 row is 342 s.
- **`timeout-minutes: 20` (1200 s) is now too tight.** Margin 1.4 times on the first run and 1.14 times (1048 s) on the second; run-to-run variance on the same code was 23 percent. A slower runner would kill a healthy run. Not changed (a recorded decision, D-61; no workflow edit is in scope). Recommendation to the owner: 30 minutes, or the parallel second-job lever.
- **Second awk run:** 330 to 381 s, 39 percent of the job, no mawk-specific finding in either run. Best lever: run it as a parallel job (same runner minutes, critical path from 856 s to about 520 s). Alternative: pair it with clang. Neither applied; both change the workflow.
- **Sanitizers:** 172 s, run twice per job (40 percent of the job). D-176 lever (2) would save only 4 to 5 s of 172 s and removes seed evidence; not worth it. A skip in the second awk run is the largest saving but needs a skip mechanism the fail-closed rule discourages.
- **Caching:** no candidate (cc65 is a 10 s apt install; every long step is test execution).
- **D-176 triggers:** the job trigger (CI job over 600 s with lever (1) not recovering it) is met, and `conformance` is still over its 60 s trigger (72.5 s). The shared-library toy lever (1) cannot be isolated on a cold runner from one run (no run exists without it; `build` is 1.5 s cold, so compilation is not what is slow). No further lever applied.
- **`video-check` stage cost:** 1.8 s per gate run.
- Nothing was weakened; no stage, control, selftest, case or workflow step was skipped or shortened.

## Commands run

`git fetch origin`;
`git ls-remote origin`;
`sh scripts/check`;
`sh scripts/check --selftest`;
`sh scripts/rights-check`;
`sh scripts/audit-history`;
`git ls-files '*.pce' '*.pdf'`;
`git diff` of the Phase 3 remote branch against HEAD, piped to credential and path scans;
`LINUX_REPRO_PARTS='gate-gcc gate-clang core-boundary-gcc' sh scripts/linux-repro`;
`git branch gsd/phase-04-vce-vdc-raw-video HEAD`;
`git push origin gsd/phase-04-vce-vdc-raw-video:refs/heads/gsd/phase-04-vce-vdc-raw-video` (twice, the second a fast-forward from a temporary `git worktree add` checkout of the new branch, removed afterwards);
`gh pr create` with the Phase 3 branch as base and the new branch as head, `--body-file` a scratch file (the only `gh` write call);
read-only `gh run list`, `gh run view`, `gh pr view`;
`sh scripts/ci-timings RUN_ID` (read-only);
in the temporary checkout `sh scripts/check --stage` for `docs-claims`, `ci-timings-controls`, `licences`, `workflow` (all PASS;
`record-lint` needs a build tree and was not run there) and `sh scripts/audit-history --range HEAD~1..HEAD` (clean);
`git cherry-pick 0ce50ba` on the local working branch (as 6c386c8).

## Deviations from the brief

None. No fix-forward was needed. One judgement call: the docs commit (the timing row and review) was committed on the new branch through a temporary checkout and pushed (it is a plain fast-forward push of the new branch, and it re-triggered CI as expected, run 2); it was then cherry-picked onto the local working branch so both carry the same text. The SUMMARY is local only.

## Owner hand-offs and notes

1. **Decide the `timeout-minutes` limit** (recommend 30, or split the second awk run into a parallel job). The second run used 87 percent of the current 20 minute limit.
2. **Pull request order:** pull request 3 is stacked on pull request 2 and shows only Phase 4. Do not merge it before pull request 2; do not retarget it without a decision. If pull request 2 is squash-merged or its branch deleted, the stacked base needs the owner's attention.
3. **Remote settings, unchanged and still pending:** Actions default permissions read-only, Actions not allowed to approve pull requests, and a ruleset on `main` requiring the `gate` check.
4. **Scratch-prefix mention in committed planning notes:** three prose lines in already-pushed Phase 3 planning notes name the assistant scratch-tree prefix (a uid-based directory, not a home path); `audit-history` does not flag them. Removing it needs a history rewrite, which was not authorised.
5. **Cut-line record (D-180):** the CI-job trigger is now measured as fired (853 s and 1048 s over 600 s); the cut was already taken (sprites deferred to Phase 4.1), so this is a record, not a new decision.

## Self-Check: PASSED

`refs/heads/gsd/phase-04-vce-vdc-raw-video` and `refs/pull/3/head` are 0ce50ba on the remote; 0ce50ba is an ancestor of the new local branch ref; the local working branch (checked out throughout) holds 6c386c8 with the same content; no worktree is left (`git worktree list` shows only the main checkout); `.planning/state.json` is untouched and uncommitted.
