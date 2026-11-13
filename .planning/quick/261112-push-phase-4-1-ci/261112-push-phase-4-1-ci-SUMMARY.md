---
phase: quick
plan: 261112-push-phase-4-1-ci
subsystem: ci
tags: [push, pull-request, stacked-pr, ci, gate, parallel-jobs, aggregator, first-hosted-run]
status: complete
commits: 2
plan_head_before: 0e507c904878906afb956faa71450c7297372ece
plan_head_after: 050e8c0 (the docs commit; this SUMMARY's commit follows it)
---

# Quick 261112-push-phase-4-1-ci: owner-authorised stacked push of Phase 4.1 and the first run of the parallel awk jobs

One new branch was pushed and one pull request opened, with the Phase 4 branch (the head of pull request 3) as its base. The owner's recorded answers were "Stacked PR on PR 3" and option A for the ancestry gap (2026-11-12).
The first hosted run of the Phase 4.1 code, which is also the first run of the three-job workflow (`gate-gawk`, `gate-mawk` and the aggregator `gate`), was green on the first try. No fix-forward was needed.
The evidence tier is E0, repeatability only. No sprite ledger entry is supported, the supported set stays at 13, and no profile is claimed accurate.

## Refs

Before (`git ls-remote origin`): `main` 8ba0bc6, Phase 2 3958316 (`refs/pull/1/head`), Phase 3 6627e50 (`refs/pull/2/head`), Phase 4 0ce50ba (`refs/pull/3/head`), tag `spike-tool-01-evidence` b605e93.
After: exactly one new branch ref, `refs/heads/gsd/phase-04.1-vdc-sprites`, first at 89a3ed8 and then fast-forwarded by the docs commits. GitHub added `refs/pull/4/*`.
Every other ref is as before. No tag, no force-push, no settings change, nothing merged. Pull requests 1 to 3 were not edited.

Pull request: https://github.com/szTheory/duospool/pull/4 (base `gsd/phase-04-vce-vdc-raw-video`, head `gsd/phase-04.1-vdc-sprites`, open, 35 commits before the docs commits).
The body follows the repository template. It covers the scope, the E0 statement, the review, security and verification results, the CI change, the merge order (pull request 1, then 2, then 3, then 4) and the three pending remote settings,
and it ends with the required attribution line. It was edited once, by me, to replace "re-run in progress" with the local gate result.

## Ancestry gap and the merge commit

The precondition "origin/gsd/phase-04-vce-vdc-raw-video is an ancestor of HEAD" failed. The Phase 4 docs commit 0ce50ba existed locally only as its cherry-pick 6c386c8: same parent, same tree, different committer time.
I stopped and reported, and the owner chose option A.
In a temporary worktree, the branch was created from HEAD 0e507c9 and given one `git merge --no-ff` of 0ce50ba (89a3ed8, "chore: merge the Phase 4 branch head so the stacked PR shows only Phase 4.1").
`git diff 0e507c9 89a3ed8` was empty (tree 085bd8f for both), and 0ce50ba is now an ancestor. The local working branch was not changed by this.

## Preconditions

| Command | Result |
|---------|--------|
| `git status --short` | only the untracked `.planning/state.json` (never added) |
| `sh scripts/audit-history --range origin/gsd/phase-04-vce-vdc-raw-video..HEAD` | clean, 34 commits; again after the merge, clean, 35 commits |
| `sh scripts/rights-check` | exit 0 |
| `git merge-tree --write-tree` of the Phase 4 head and HEAD | no conflict, tree equals HEAD's |
| `sh scripts/check` on the merged tree in the worktree | PASS, 57 stages, 1152 s wall (load average about 128) |

## CI runs

| Run | Head | Event | Jobs (s) | Run wall s | Conclusion |
|-----|------|-------|----------|------------|------------|
| 43352015009 | 89a3ed8 | pull_request | gate-gawk 438, gate-mawk 555, gate 3 | 562 | success (recorded in `docs/ci-timings.md`) |

The docs push that carries this SUMMARY starts one more run, which is not recorded in `docs/ci-timings.md`.

Run 43352015009: queue 0 s, image `ubuntu-24.04` 20261101.360.1, default awk gawk, second awk mawk (the apt fallback was not needed), `CHECK RESULT PASS` with 57 stages in each work job.
Steps, gawk job: set-up 3, cc65 9, Gate 339, Range audit 3, gcc 82. Steps, mawk job: set-up 6, cc65 14, Gate under the other awk 414, clang 120. The aggregator started 2 s after the mawk job ended and ran for 3 s.
Stages, gawk then mawk: `sanitizers` 153.7 and 195.3, `conformance` 82.2 and 97.1, `determinism` 69.4 and 83.5 (90 and 91 percent of each gate). Every other gawk stage took 3.0 s or less.

## Efficiency review (in full in `docs/ci-timings.md`)

- **The split works:** the critical path is 562 s, against 856 and 1051 s as one job (34 and 47 percent shorter). Runner seconds stayed the same (996). It is 7 s above the expected band of about 450 to 555 s.
- **The legs did not balance:** 438 s against 555 s. Every C-heavy stage in the mawk job was 1.18 to 1.27 times slower, and the clang step took 120 s against gcc's 82 s. In Phase 4 run 1, on one machine, mawk was faster than gawk.
  The likely cause is a slower runner draw for the mawk job. This was not confirmed, because the CPU model is not logged.
- **The aggregator reports correctly on success** and is named `gate`. From the workflow text, it fails when a leg fails: `if: always()` plus `test "$X" = success` under `sh -eu`, enforced by `WF_AGGREGATOR`. It was not exercised on CI on purpose.
- **Timeouts:** 25 minutes is 2.7 times the slower job. The D-176 CI-job trigger (600 s) did not fire on this run, but the mawk job's margin is 45 s.
- **Levers, none applied:** (a) log `nproc` and the CPU model, at zero cost; (b) a third parallel job for clang, which would have cut this run to about 440 s for about 20 s of extra runner set-up;
  (c) skip `sanitizers` in the mawk leg (150 to 195 s), which needs the owner's decision on the fail-closed rule. Caching still has no candidate.
- Nothing was weakened.

## Commits

- 89a3ed8 on `gsd/phase-04.1-vdc-sprites` only: the owner-chosen merge commit (no file change).
- 050e8c0 on `gsd/phase-04.1-vdc-sprites`: `docs/ci-timings.md` (the timing row, the protocol line and the run section) and this directory's PLAN.
- The SUMMARY commit follows on the same branch. Both docs commits are pushed to the same branch.
- Local working branch: the three files are copied into one local commit (never pushed). Its hash is in the hand-off report, since a commit cannot name its own hash.

## Deviations from the brief

1. The ancestry precondition failed. I stopped and reported, and the owner chose option A (the merge commit above). No other deviation.
2. The pull request was opened while the local `sh scripts/check` re-run was still running, because the machine load was about 128. It passed afterwards, and the body was updated to say so.

## Owner hand-offs

1. **Merge order:** pull request 1, then 2, then 3, then 4. Do not merge out of order and do not retarget without a decision. If pull request 3 is squash-merged, the stacked base of pull request 4 needs attention.
2. **Remote settings, unchanged and still pending:** Actions default permissions read-only, Actions not allowed to approve pull requests, and a ruleset on `main` requiring the `gate` check. `gate` is now the aggregator, and it exists under that name.
3. **Workflow levers (a) and (b) above:** each is a small workflow change that needs the owner's decision.
4. **A committed absolute scratch path exists in older planning notes** (for example the context block of the 261111-e8l quick PLAN). `audit-history` does not flag it, and removing it needs a history rewrite, which is not authorised.

## Self-Check: PASSED

89a3ed8 and 050e8c0 are on `origin/gsd/phase-04.1-vdc-sprites` after the push; 0ce50ba is an ancestor; run 43352015009 concluded success; `.planning/state.json` is untouched and uncommitted.
