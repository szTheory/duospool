---
phase: quick
plan: 261112-push-phase-4-1-ci
type: execute
wave: 1
depends_on: []
files_modified:
  - docs/ci-timings.md
autonomous: false
requirements: []
must_haves:
  truths:
    - "Exactly one new remote branch, gsd/phase-04.1-vdc-sprites, and one pull request whose base is gsd/phase-04-vce-vdc-raw-video (the head of pull request 3)"
    - "The Phase 4 branch head is an ancestor of the pushed branch, through one merge commit whose tree equals the Phase 4.1 head's tree"
    - "No push to main or any other existing branch, no tag, no force-push, no settings change, no merge, no edit of pull requests 1 to 3"
    - "The first hosted run of the three-job workflow (gate-gawk, gate-mawk, aggregator gate) is recorded with job and stage seconds and reviewed for efficiency"
---

<objective>
Push Phase 4.1 (VDC sprites, E0 repeatability only) and the local quick-task commits after the head of pull request 3 as one stacked pull request, watch its CI to completion, fix forward on the same branch if it fails, and record the first run of the parallel second awk workflow in `docs/ci-timings.md`.
</objective>

<authority>
Owner answers (2026-11-12): "Stacked PR on PR 3"; then option A for the ancestry gap: create the branch from HEAD in a temporary worktree, add one `git merge --no-ff` of the Phase 4 branch head on the new branch only, and check that the merged tree equals HEAD's tree before pushing.
Not authorised: pushes to main or any other existing branch, tags, force-push, any repository, Actions or ruleset setting, merging, closing, editing or retargeting pull requests 1 to 3. If CI does not run for a non-main base, stop and report.
</authority>

<tasks>
1. Preconditions: clean tree except the untracked `.planning/state.json`; `sh scripts/audit-history --range origin/gsd/phase-04-vce-vdc-raw-video..HEAD`; `sh scripts/rights-check`. Stop on any failure.
   The ancestry check failed (the Phase 4 docs commit 0ce50ba exists locally only as its cherry-pick 6c386c8, identical tree); reported, and the owner chose option A.
2. Branch from HEAD in a temporary worktree, one merge commit of the Phase 4 head, tree check, `git push -u origin gsd/phase-04.1-vdc-sprites`, `gh pr create --base gsd/phase-04-vce-vdc-raw-video`.
3. Watch CI. On failure: diagnose from logs, fix forward with atomic commits, narrow verify plus `sh scripts/check` before each fix push; stop after 3 failed rounds.
4. Efficiency review: job and step seconds (`gate-gawk`, `gate-mawk`, aggregator, `sanitizers`, `conformance`, `determinism`), critical path against 853 to 1048 s (pull request 3) and the expected 450 to 555 s,
   the aggregator's behaviour from the workflow text, and cost levers. Record them in `docs/ci-timings.md` and this directory's SUMMARY, push the docs commit to the same branch,
   and copy the files back to the local working branch in a local commit.
</tasks>
