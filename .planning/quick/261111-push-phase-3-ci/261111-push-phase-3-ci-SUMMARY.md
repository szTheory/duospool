---
phase: quick
plan: 261111-push-phase-3-ci
subsystem: ci
tags: [push, pull-request, ci, gate, first-hosted-run]
status: complete
commits: 3
plan_head_before: 2b8a7e67c3d0f76b5cf11207f58c4835fdd3fe1d
plan_head_after: 6627e5071bf549131eccebe9795d63aae13111b8
---

# Quick 261111-push-phase-3-ci: owner-authorised push of Phase 3 and the first CI run

Pushed one new branch and opened one pull request under the owner's recorded answer "Push as a new branch and PR (Recommended)" (2026-11-11). The first hosted run of the Phase 3 code failed twice on gcc-only diagnostics in one test file, was fixed forward, and then went green. The evidence tier is E0, repeatability only.

## Refs

Before (`git ls-remote origin`): `main` 8ba0bc6, `gsd/phase-02-...` 3958316, `refs/pull/1/head` 3958316, `refs/pull/1/merge` f6cb1b6, tag `spike-tool-01-evidence`.
After the first push: exactly one new branch ref, `refs/heads/gsd/phase-03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad` at 2b8a7e6, and GitHub then added `refs/pull/2/head` and `refs/pull/2/merge`.
Final: the branch is at `6627e50` (three fix-forward and docs commits above the first push); `main` is still 8ba0bc6, the Phase 2 branch and `refs/pull/1/*` are unchanged, no tag was pushed, nothing was forced. The local working branch was left checked out; the new branch was cut with `git branch`, fast-forwarded with `git branch -f` (local only) after each fix, and pushed with the literal refspec `NAME:refs/heads/NAME`. No upstream is configured.

Pull request: https://github.com/szTheory/duospool/pull/2 (base `main`, template body, no settings change, not merged).

## Preconditions (all run before the push, all passed)

`sh scripts/check` PASS (201 s on the loaded machine), `sh scripts/check --selftest` PASS (51 s), `sh scripts/rights-check` exit 0 (1620 records), full-history `sh scripts/audit-history` clean (398 commits, 2902 blobs; 221 s), `git ls-files '*.pce'` prints nothing, `git diff b063518..HEAD` has no credential shape and no `/Users/` path.
One note for the owner: three committed planning files (the two Phase 3 review reports and the quick-task plan 261111-e8l) contain a scratch directory path under the assistant's `/private/tmp/claude-501/` scratch tree. It is not a home-directory path and `audit-history` does not flag it, but the scratch directory name embeds the local account name. Phase 2 planning files already pushed with PR 1 contain similar `/private/tmp` paths. Left as is: removing it would need a history rewrite, which was not authorised.

## CI runs

| Run | Head | Conclusion | Job s | What it showed |
|-----|------|-----------|-------|----------------|
| 43122131107 | 2b8a7e6 | failure | 42 | gcc 13, `-std=c11 -pedantic -Werror`: `strtok_r` undeclared in `tests/conformance/reviewkinds.c` (POSIX, not ISO C11). Build, conformance, sanitizers, record-lint, determinism, fixtures and named-controls stages failed (all downstream of the one build error). |
| 43122733150 | 6b313f7 | failure | 36 | the same file: `-Werror=format-truncation` at `reviewkinds.c:116` |
| 43122980214 | cb2eb55 | success | 342 | the first green run, recorded in `docs/ci-timings.md` (row and section) |
| 43124400989 | 6627e50 | success | 541 | last head (the docs commit); a separate run, NOT recorded in `docs/ci-timings.md`, to avoid another triggering commit |

Fixes (commits `6b313f7`, `cb2eb55`, only `tests/conformance/reviewkinds.c`): a by-hand comma split with `strchr` (an empty field is now refused where `strtok_r` skipped it; stricter, fail-closed) and `%.2s` in the `snprintf` so the output is bounded by construction. Both were checked by the macOS `build` and `conformance` stages and a real (not `-fsyntax-only`) gcc 13 compile of every `src` and `tests` C file in a container at `-O0` and `-O2`. The full local gate passed before the docs commit.

## What the first hosted run showed

- **apt `cc65`:** Ubuntu `cc65 2.19-1`; `ca65`/`ld65` report `V2.18 - Ubuntu 2.19-1` (checked in an Ubuntu 24.04 container; the runner log prints only the package line). That is the same `V2.18` identity as the Homebrew 2.19 bottle locally. Install step 8 s.
- **Fixture hash pins reproduce:** `fixture-build` (21 images) and `fixtures` passed in every run, including the failing ones, and in both awk runs. No pin was touched, nothing was weakened. This is repeatability across two builds of the same assembler release, not an independent oracle.
- **gcc 13 and clang 18 with `-Werror`:** gcc found the two defects above; the clang step passed on its first run. Compiler versions are written only to the job summary, which the API does not return, so they are not recorded.
- **Sanitizers (gcc):** passed (46 s gawk run, 43 s mawk run). **mawk and gawk full gates:** both `CHECK RESULT PASS`, 56 stages each. Default awk `/usr/bin/gawk`, second run `/usr/bin/mawk` (already installed, no apt fallback).
- **`timeout-minutes: 20`:** no problem.

## Per-step seconds (green run 43122980214)

Set up, checkout and tool versions 4; Install cc65 8; Gate 119; Range audit 37 (334 commits over `main`); gcc build and conformance 31; clang build and conformance 37; Second awk gate run 104. Queue 0 s, job 342 s, run 344 s. Slowest stages in the gate (gawk, mawk): sanitizers 46/43, conformance 33/24, determinism 19/19; all others 2 s or less. Critical path = the single job. The pre-run estimate was 338 / 458 / 568 s (low, central, high), so the real value sat at the low end.
The last head's run took 541 s (200 s more for the same work, with no code change between), so run-to-run variance on hosted runners is large: two samples, 342 and 541 s.

## Efficiency review (recommendations, no workflow change made)

- **Timeout 20 min:** realistic. 3.5 times the 344 s run and 2.2 times the 541 s run. Do not tighten on two samples; after three or more recorded Phase 3 rows, 15 minutes is the most to consider, and the 541 s run argues for leaving 20.
- **Second awk run:** keep as a whole-gate run (104 s, 30 percent of the job; 52 of 56 stages use awk; no mawk-only difference was seen). The only candidate is skipping `sanitizers` there (about 43 s), which needs a skip mechanism that fail-closed discourages.
- **Sanitizers under clang:** not yet. Adding it costs about 45 s (13 percent). The CI-only defects so far were gcc warnings. The zero-cost option (pair the mawk run with clang) blurs failure attribution. Owner choice; a clang-only finding would be the trigger.
- **Caching:** none. cc65 is an 8 s apt install; the compile is 1 s; every step over 30 s is test execution, not a downloadable artefact, so a cache would only cache results.
- **Overlap:** the `gcc build and conformance` step (31 s) repeats the `Gate` step's `conformance` because `cc` is gcc on the image. It stays (it pins the compiler and the strict flags); first candidate if the job must shrink.
- **Local gap:** macOS cannot see gcc/glibc strict-mode defects. `sh scripts/linux-repro` would have caught both and is a one-off run. Consider running it (or a gcc container compile) before the next push. At `-O2` (not a gate preset), gcc also reports `-Wformat-truncation` in `tests/conformance/buskinds.c:110` and `digkinds.c:299`; recorded, not changed.

## Deviations

- **[Rule 1 - Bug]** two CI-only compile defects in `tests/conformance/reviewkinds.c`, fixed as above, no control added: the gate's own gcc steps are the control, and the CI run is the evidence. The `-O2` warnings are out of scope and deferred.
- The SUMMARY stays on the local working branch only; the pushed branch carries the two fixes and the `docs/ci-timings.md` commit.

## Needs the owner

1. The pull request is open and green; merging it, and the three pending repository settings (read-only default workflow permissions, no Actions approval of pull requests, a ruleset on `main` requiring the `gate` check), remain owner hand-offs and were not touched.
2. Decide whether to keep or scrub the scratch paths (the `/private/tmp/claude-501/` tree) in three committed planning files (history rewrite needed to remove them).
3. Sanitizers under clang and the second-awk subset questions above are owner choices, not made here.
4. Run-time variance (342 s and 541 s) is worth a few more rows before tightening the timeout.

## Self-Check: PASSED

Branch ref, PR 2, runs 43122131107, 43122733150, 43122980214, 43124400989 and commits `6b313f7`, `cb2eb55`, `6627e50` exist; `main` unchanged; `.planning/state.json` untouched and uncommitted.
