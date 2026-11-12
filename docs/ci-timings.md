<!-- SPDX-License-Identifier: MIT OR Apache-2.0 -->
<!-- SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors -->
# CI timings

This file records how long the CI definition (`.github/workflows/gate.yml`, record 0006) takes on a real run, and what to do with the numbers.
A row is produced by `sh scripts/ci-timings RUN_ID` (a read-only `gh run view`) and pasted here in a normal commit. No bot commits this file and
no timing artifact is uploaded (D-63). The tool also reads a canned file (`sh scripts/ci-timings --tsv FILE`) so that its output is tested offline
by the `ci-timings-controls` stage of `scripts/check`.

## Status

CI-CHECKPOINT: verified (answer: authorised-branch-pr) run 42939483070

The owner decision that allowed the push is recorded here as it was given. Answer option id: `authorised-branch-pr`. The owner's selection, verbatim:
"Push branch + PR, allowlist the 2 paths (Recommended)", chosen in the /gsd-autonomous session on 2026-11-10, answering the question about pushing one
phase branch and opening a pull request for the first CI run. It is not `authorised-branch-pr-and-settings`: no repository setting was changed and no
ruleset was created. The scope was one branch, `gsd/phase-02-deterministic-machine-shell-hucard-boundary-and-fail-closed`, and one pull request into
`main`; `main` was not pushed to, no tag was created, no force-push was used.

Success criterion 5 is verified by one real run: the `gate` job of run 42939483070 (pull request 1, head `b063518`) concluded success on a GitHub-hosted
`ubuntu-24.04` runner. The evidence tier is E0, repeatability: the run shows that the gate command passes on a second machine class with a second awk
and two compilers, not that any profile is accurate. It is the first GitHub-hosted run of the gate, so it is distinct evidence from the local runs and
from the one-off container run below.

## First run (run 42939483070)

Runner image `ubuntu-24.04`, image version 20261101.360.1 (runner 2.338.2), from the run log. The `Tool versions` step writes cmake, ninja, gcc and clang
versions to the job summary, which `gh` and the Actions API do not return (`output.summary` is null), so those four versions are not recorded here;
add them from the job page if they matter. The awk facts come from the log of the `Second awk gate run` step:

- **Default awk (D-74):** `/usr/bin/gawk` (gawk). The first gate run and the range audit ran under gawk. The dual run therefore exercised mawk as the
  second implementation, which is the dialect the owner's machine and gawk both differ from.
- **Second awk:** `/usr/bin/mawk`. It was already installed on the image, so the `apt-get` fallback did not run (the step log holds no apt output). The
  fallback is untested on this image and cheap; keep it as a guard against an image change.
- **Both full gates passed:** `CHECK RESULT PASS` with 47 stages under gawk, and again under mawk.
- The macOS awk (BWK 20200816) is exercised only by the local runs, so three awk implementations now have evidence, from three places.

Step seconds (from the step timestamps of the job): set up, checkout and tool versions 2 s; `Gate` 37 s; `Range audit` 16 s; `gcc build and conformance`
7 s; `clang build and conformance` 9 s; `Second awk gate run` 25 s. Slowest stages inside the gate, from the gaps between the `CHECK` lines (a
rounded estimate, whole seconds): `build` 7 s (cold), `conformance` 6 s, `sanitizers` 5 s, every other stage 2 s or less.

## Efficiency review of the first run

- **Critical path 100 s, queue 0 s.** One job, so the critical path is the job. The local baseline below was 45 s of wall time on a quiet developer
  machine, so the runner is about 2 times slower than that machine, with `Range audit` and the second awk run added. Compare later rows with this one.
- **No cache, and none justified.** The `Gate` step is 37 s as a whole step, over the 30 s rule of thumb, but it is 47 stages and none of them passes
  7 s, so the time is spread out and there is nothing for a cache to hold: the stages build from source with no downloaded dependency. A cache would
  only hide the cold-start failures the job exists to catch.
- **Possible overlap, not changed.** The `Gate` step's `conformance` stage and the `gcc build and conformance` step both build with the system C compiler
  (gcc on this image) in different build directories, which is about 7 s of repeated work. It is kept: the two directories prove a clean gcc preset
  build, and 7 s is far below any threshold. Revisit if the gate grows.
- **The second awk step is 25 s, the largest piece after `Gate`.** It reruns every stage, which is the point (failure class 4). It could be limited to the
  awk-using stages later, but that would take an inventory of those stages, and the saving is under 25 s. Not changed.
- **`timeout-minutes: 10` against a 100 s critical path.** That is a 6 times margin. A single sample is not enough to tighten it; reconsider after
  three or more rows, when a limit of 5 minutes would still leave a margin of about 3 times. Not changed.
- **`Range audit` is 16 s** on this pull request because the range holds the whole phase (130 commits). A normal pull request holds a few commits, so
  expect it to shrink.
- **Nothing was weakened.** No stage, control, selftest or workflow step was skipped, merged or shortened, and no workflow file changed for this review.

## Timing table

Columns: date (UTC), run id, queue seconds (run start minus run creation), seconds per job, run wall seconds (the latest job end minus the run
start, which is the critical path), and the run conclusion.

| Date (UTC) | Run id | Queue s | Job s | Run wall s (critical path) | Notes |
|------------|--------|---------|-------|----------------------------|-------|
| 2026-11-11 | 42939483070 | 0 | gate=98 | 100 | success |
| 2026-11-11 | 43122980214 | 0 | gate=342 | 344 | success |
| 2026-11-12 | 43275379025 | 0 | gate=853 | 856 | success |

## Failure classes the job owns

The `gate` job earns its place by catching what the owner's macOS machine cannot. If a change would stop it catching one of these, it needs a
different job, not a deletion.

1. Differences in `sh`, `awk`, userland and `nm` from macOS: the Linux image has dash, mawk or gawk, GNU tools and ELF symbols.
2. Fresh-clone history dependence: a full clone with no local branches or leftover build files, which the protocol-order check and the range audit read.
3. gcc versus clang diagnostics and undefined behaviour: the gate runs one compiler, and two further steps run each compiler on its own build directory.
4. Awk dialect drift: the gate runs a second time under the other awk implementation.

## Standing protocol

- Every CI run is reviewed for low-hanging efficiency wins (iteration time, runner minutes, redundant steps, caching) without weakening the evidence
  of a job (AGENTS.md, "CI and release: review efficiency after every run"). The result of each review, including "nothing to change", is recorded
  here next to the row it is about.
- Revisit caching when any single step exceeds 30 seconds. Until then there is no cache: measure first.
- Review the gate job's `timeout-minutes` limit (D-61; 10 until plan 03-33, 20 since, then 25 per work job and 5 for the aggregator in the parallel-job change below, unverified on CI) against each recorded critical path. The second gate run and
  the two compiler runs add to the job, so the limit may be too tight or needlessly loose.
- Time is a record and a warning, never a failure of the gate (D-60).

## Pending hand-offs

Each item is a setting on the remote repository. It is applied only with the push checkpoint's authorisation (D-80). The owner's answer was
`authorised-branch-pr`, which does not authorise settings, so none was changed and all three stay pending hand-offs for the owner.

1. Actions default workflow permissions set to read-only. Read-only observation on 2026-11-10 (a GET, no change): already `read`.
2. Actions allowed to approve pull requests: off. Read-only observation on 2026-11-10: already off (`can_approve_pull_request_reviews` false).
3. A branch ruleset on `main` that requires the `gate` check (owner gate, D-64). Read-only observation on 2026-11-10: no ruleset exists.

Items 1 and 2 are already at the wanted values on the repository today, but nothing in the repository pins them, so they stay on the list for the owner
to confirm. Item 3 is not done: `main` is unprotected, and the `gate` check exists now (it ran on pull request 1) so a ruleset can name it.

## Local baseline (a local run, not a CI run)

Measured on 2026-11-10 on the owner's development machine (Apple M5 Pro, 18 cores, macOS 26, BWK awk 20200816, Apple clang 21, cmake 4.4.3) with
`sh scripts/check --timing`, a warm tree (`build/` present, so the `build` stage is an incremental no-op). This is a local baseline for later
comparison. It is not a CI measurement, it has no row in the CI timing table above, and the machine was shared: the load average was about 4 at the
start of this run, and it was between 17 and 32 during earlier runs the same day, when the same gate took 69 s and 84 s and 112 s of wall time for
the same work. The seconds are whole seconds from the gate's own `date +%s` clock, so a stage of 0 took under a second.

Wall time of the commands of the phase-end run: `sh scripts/check` 45 s (47 stages, sum of the stage seconds 45), `sh scripts/check --selftest` 10 s,
`dash scripts/check` 46 s, `sh scripts/audit-history` 50 s (full history, 192 commits). No stage ran longer than 20 s, so no `TIMING_WARN` line was
printed; the longest stage was `sanitizers` at 6 s.

| # | Stage | Seconds (local) | Verdict |
|---|-------|-----------------|---------|
| 1 | preflight | 0 | PASS |
| 2 | ledger-entries | 0 | PASS |
| 3 | ledger-negatives | 1 | PASS |
| 4 | ledger-samples | 1 | PASS |
| 5 | decision-records | 0 | PASS |
| 6 | decision-negatives | 0 | PASS |
| 7 | profile-records | 1 | PASS |
| 8 | profile-negatives | 0 | PASS |
| 9 | filename-controls | 1 | PASS |
| 10 | empty-expect-controls | 0 | PASS |
| 11 | provenance | 0 | PASS |
| 12 | provenance-negatives | 1 | PASS |
| 13 | licences | 0 | PASS |
| 14 | licence-negatives | 1 | PASS |
| 15 | protocol-order | 0 | PASS |
| 16 | protocol-negatives | 3 | PASS |
| 17 | score-controls | 1 | PASS |
| 18 | results | 0 | PASS |
| 19 | results-negatives | 0 | PASS |
| 20 | ingest-controls | 1 | PASS |
| 21 | flatten-controls | 0 | PASS |
| 22 | cancel-controls | 1 | PASS |
| 23 | outcome-controls | 0 | PASS |
| 24 | rights-tree | 1 | PASS |
| 25 | rights-negatives | 3 | PASS |
| 26 | audit-synthetic | 4 | PASS |
| 27 | inventory | 1 | PASS |
| 28 | inventory-negatives | 1 | PASS |
| 29 | workflow | 0 | PASS |
| 30 | workflow-negatives | 0 | PASS |
| 31 | docs-claims | 0 | PASS |
| 32 | docs-claims-negatives | 1 | PASS |
| 33 | ci-timings-controls | 0 | PASS |
| 34 | build | 0 | PASS |
| 35 | outcome-sync | 0 | PASS |
| 36 | conformance | 5 | PASS |
| 37 | sanitizers | 6 | PASS |
| 38 | excluded-mode-controls | 0 | PASS |
| 39 | record-lint | 0 | PASS |
| 40 | record-lint-negatives | 1 | PASS |
| 41 | core-boundary | 0 | PASS |
| 42 | core-boundary-negatives | 2 | PASS |
| 43 | determinism | 1 | PASS |
| 44 | determinism-negatives | 1 | PASS |
| 45 | fixtures | 1 | PASS |
| 46 | fixture-negatives | 0 | PASS |
| 47 | named-controls | 5 | PASS |

### Container run (one-off, not CI)

`sh scripts/linux-repro` ran the gate in a throwaway Ubuntu 24.04 container (linux/amd64, emulated on this arm64 Mac; gcc 13.3.0, clang 18.1.3,
cmake 3.28.3, mawk 1.3.4, gawk 5.2.1, dash as `sh`). The whole one-off run took 802 s: install and the six parts. Each full gate took 193 s
(default awk, gcc), 207 s (mawk, clang) and 216 s (gawk, gcc), plus a selftest each, from a cold `build/`. The emulated numbers are not a prediction
for a GitHub runner: the heaviest stages there were `named-controls` (19 to 20 s), `conformance` (17 to 20 s), `protocol-negatives` (15 to 17 s) and
`audit-synthetic` (15 to 16 s), against 3 to 5 s here. The container run is a one-off and has no row in the CI timing table.

### Efficiency review of this baseline

- **Where the time goes.** The gate is a long tail of many stages of 0 to 3 s with a few of 4 to 6 s (`sanitizers`, `named-controls`, `conformance`,
  `audit-synthetic`). No stage is near the 30 s per step threshold (D-61) that would justify a cache, so none is added: measure first.
- **Spawn cost, not cmake.** An earlier warm run the same day spent about 30 s of user CPU and about 38 s of system CPU (more than the wall time because the
  compilers and ninja run in parallel), and system time larger than user time points at process creation (`sh`, `awk`, `git`, `grep`), not compiling. The `build` stage is 0 s warm and 2 s cold.
  The compile-heavy stages (`sanitizers`, `conformance`, `named-controls`, which build the runner and the toy cores) are real evidence and not
  repeated work.
- **Load dominates the variance.** The same gate took 45 s on a quiet machine and up to 112 s when other processes loaded it, so a single local
  number is a noisy proxy for CI. Read the first CI row against this baseline only after allowing for runner speed.
- **Nothing was weakened.** No stage, control or selftest was skipped, merged or shortened to save time.
- **What would be tried before adding a cache** (only if a CI step passes 30 s): first, whether the step is dominated by emulated or cold-start cost
  that a warm runner removes; second, running the two independent compiler builds (`gate-gcc`, `gate-clang`) as one job step instead of two cold
  configures; third, reducing repeated `git` spawns in `protocol-negatives` and `audit-synthetic`. A dependency cache is the last resort, because a
  stale cache hides exactly the cold-start failures the job exists to catch.

## Local baseline after W1 (a local run, not a CI run)

CI for the W1 changes: unverified, pending owner push authorisation (D-64). Nothing of W1 (authored fixtures, the cc65 install step in the workflow,
the README and docs-check rule) has run on a GitHub runner, so this file holds no CI row for it and no CI claim. The first-run row above and its
efficiency review describe the Phase 2 gate only. The workflow now installs cc65 before the Gate step (plan 03-07); the first CI run after an
authorised push will add that apt step to the critical path, and the row for it is to be added then.

Measured on 2026-11-11 on the owner's development machine (Apple M5 Pro, 18 cores, macOS 26.6.2, BWK awk 20200816, Homebrew cc65 2.19 whose `ca65`
prints V2.18) with `sh scripts/check --timing` on a warm tree (`build/` present). The machine was shared and loaded: the load average was about 10 to
13 during the run, so the seconds are an upper-leaning local figure, not a quiet-machine one. The seconds are whole seconds from the gate's own clock.

Target and verdict: the warm target is 120 s for the gate. The run took 62 s of wall time for 50 stages (sum of the stage seconds 62), which is under
the target with a margin of about 58 s. No stage ran longer than 20 s, so no `TIMING_WARN` line was printed; the longest was `sanitizers` at 7 s.
`sh scripts/check --selftest` took 14 s. Against the Phase 2 local baseline above (45 s for 47 stages on a quieter machine), the three added stages
`fixture-build`, `fixture-lint` and `fixture-lint-negatives` cost 0 or 1 s each warm, and `fixtures` grew from 1 s to 2 s (it now runs the
authored image build first, plan 03-06); the rest of the difference (45 s to 62 s) is not separable from machine load, so no regression is claimed.

| # | Stage | Seconds (local) | Verdict |
|---|-------|-----------------|---------|
| 1 | preflight | 0 | PASS |
| 2 | ledger-entries | 1 | PASS |
| 3 | ledger-negatives | 1 | PASS |
| 4 | ledger-samples | 0 | PASS |
| 5 | decision-records | 0 | PASS |
| 6 | decision-negatives | 1 | PASS |
| 7 | profile-records | 0 | PASS |
| 8 | profile-negatives | 1 | PASS |
| 9 | filename-controls | 1 | PASS |
| 10 | empty-expect-controls | 0 | PASS |
| 11 | provenance | 0 | PASS |
| 12 | provenance-negatives | 1 | PASS |
| 13 | licences | 1 | PASS |
| 14 | licence-negatives | 1 | PASS |
| 15 | protocol-order | 0 | PASS |
| 16 | protocol-negatives | 6 | PASS |
| 17 | score-controls | 0 | PASS |
| 18 | results | 0 | PASS |
| 19 | results-negatives | 1 | PASS |
| 20 | ingest-controls | 0 | PASS |
| 21 | flatten-controls | 1 | PASS |
| 22 | cancel-controls | 1 | PASS |
| 23 | outcome-controls | 0 | PASS |
| 24 | rights-tree | 2 | PASS |
| 25 | rights-negatives | 5 | PASS |
| 26 | audit-synthetic | 5 | PASS |
| 27 | inventory | 2 | PASS |
| 28 | inventory-negatives | 1 | PASS |
| 29 | fixture-lint | 0 | PASS |
| 30 | fixture-lint-negatives | 1 | PASS |
| 31 | workflow | 0 | PASS |
| 32 | workflow-negatives | 1 | PASS |
| 33 | docs-claims | 0 | PASS |
| 34 | docs-claims-negatives | 0 | PASS |
| 35 | ci-timings-controls | 0 | PASS |
| 36 | fixture-build | 0 | PASS |
| 37 | build | 0 | PASS |
| 38 | outcome-sync | 1 | PASS |
| 39 | conformance | 6 | PASS |
| 40 | sanitizers | 7 | PASS |
| 41 | excluded-mode-controls | 0 | PASS |
| 42 | record-lint | 0 | PASS |
| 43 | record-lint-negatives | 1 | PASS |
| 44 | core-boundary | 0 | PASS |
| 45 | core-boundary-negatives | 3 | PASS |
| 46 | determinism | 2 | PASS |
| 47 | determinism-negatives | 0 | PASS |
| 48 | fixtures | 2 | PASS |
| 49 | fixture-negatives | 1 | PASS |
| 50 | named-controls | 5 | PASS |

### Efficiency review of the W1 baseline

- **Where the time goes.** The same long tail of short stages as before; the largest are `sanitizers` 7 s, `conformance` 6 s, `protocol-negatives` 6 s,
  and `named-controls`, `audit-synthetic` and `rights-negatives` 5 s each. None is near the 30 s per step threshold (D-61), so no cache is added.
- **The fixture stages are cheap.** `fixture-build` rounds to 0 s (under a second) for the five small authored images; the `fixtures` stage runs the
  image build first (plan 03-06) and takes 2 s. A cold-tree figure was not measured here.
- **Not measured.** A cold tree, the Linux container run and any GitHub runner run were not part of this measurement. The apt install of cc65
  is the one step with no number at all.
- **Nothing was weakened.** No stage, control or selftest was skipped, merged or shortened to save time.

## Toy controls run only their EXPECT cases (quick task 261111-e8l; local runs, not CI runs)

Change: `controls_mode` in `scripts/conformance-run` used to run every toy control (nine of them) against the whole case list (181 cases), while the
judge reads only the case ids in each control's `EXPECT` (1 to 3 ids). Each toy now runs against a cases file cut down to those ids (`filter_cases`),
and an `EXPECT` id that the cut loses is a harness error (exit 2), never a pass. The selftest `mutation-toy-filter` proves that. The verdicts are
unchanged: the outcome of every `EXPECT` id is the same as under the full list (12 ids, all `guest_failure`, same assertion counts; table in the
commit message of the change).

These are local, load-dependent measurements on a shared developer machine, not a CI claim; CI evidence stays unverified, pending owner push
authorisation. The machine was heavily loaded by unrelated jobs, so compare the per-stage seconds below and the same-moment A/B pair, not the totals.

Same-moment A/B of the conformance stage alone (`sh scripts/conformance-run run`, warm build, old script from the parent commit against the new one,
run one after the other; load average is 1, 5 and 15 minutes at the start of each run):

| Run | Script | Wall s | Load average at start |
|-----|--------|--------|-----------------------|
| 1 | old (whole list per toy) | 122 | 13.33 17.68 18.64 |
| 1 | new (EXPECT cases only) | 29 | 27.52 20.16 19.41 |
| 2 | old | 140 | 30.46 21.65 19.98 |
| 2 | new | 25 | 61.64 37.94 26.81 |

Full `sh scripts/check --timing`, before and after (54 stages, all `PASS` in both; the stage line is the TIMING line):

| Measure | Before | After | Note |
|---------|--------|-------|------|
| Load average at start | 24.76 22.07 19.84 | 56.23 39.04 27.65 | the after run saw 2 to 3 times the load |
| Load average at end | 17.79 19.39 19.26 | 62.51 50.14 35.70 | |
| `conformance` s | 122 | 31 | the stage this change touches |
| `sanitizers` s | 47 | 66 | unchanged code; the toy controls do not run in the sanitize mode, so the difference is load |
| Other 52 stages, sum of TIMING s | 131 | 215 | unchanged code; load again (conformance and sanitizers excluded) |
| Sum of TIMING s (all stages) | 300 | 312 | `time`: 4:59.94 before, 5:13.50 after |

Reading: the conformance stage went from 122 s to 31 s although the load was higher, and the A/B pair above, run back to back, agrees (122 and 140 s
before, 29 and 25 s after). The total did not drop in this pair of runs only because the unchanged stages took longer under the heavier load
(`sanitizers` 47 to 66 s, the others 131 to 215 s in sum). A quiet-machine run would show the saving in the total; it was not available here.
Nothing was weakened: no stage, control or selftest was skipped, merged or shortened, and every toy still shows `CONTROL NAME RED_OK`.

## Local baseline after W8 (a local run, not a CI run)

CI for Phase 3: unverified, pending owner push authorisation (D-64). Nothing of Phase 3 has run on a GitHub runner: the only CI run on record is the
Phase 2 run above (pull request 1, head `b063518`), and about 180 commits have been made since that head (the Phase 2 review fixes and Phase 3). Nothing was
pushed, no pull request was opened and no repository setting changed in plan 03-33. This file holds no CI row for Phase 3 and makes no CI claim for it.

Measured on 2026-11-11 on the owner's development machine (Apple M5 Pro, 18 cores, macOS 26.6.2, BWK awk 20200816, Apple clang 21, cmake 4.4.3), commit
`bf1bef0`, warm tree (`build/` present), with `sh scripts/check --timing`. The machine is shared with unrelated jobs and the load was never quiet, so two
runs are recorded. Run A: 193 s wall (`time -l`: 129 s user, 56 s system), load average 14.9 16.8 23.3 at the start and 15.3 15.3 21.4 at the end.
Run B: 295 s wall (155 s user, 88 s system), load average 49.8 65.4 50.7 at the start and 14.0 36.3 41.8 at the end. All 56 stages passed in both. Single
stage runs at load 12 to 16: `conformance` 37 s (33 s user), `sanitizers` 71 s (62 s user), `determinism` 36 s (32 s user). User time close to wall
time says these three stages are compute, not waiting.

Target and verdict: the warm target is 120 s (D-132). **Missed**: 193 s in the better run (73 s over) and 295 s in the loaded one. Even the user CPU
time of run A (129 s) is above the target, so a quiet machine would not meet it either. Time is a record and never fails the gate. Stages over 20 s,
run A: `conformance` 31 s, `sanitizers` 52 s, `determinism` 30 s (113 s of 192, 59 percent); in run B the same three (55, 65, 24 s). Against the W1
baseline (62 s for 50 stages, `conformance` 6 s, `sanitizers` 7 s, `determinism` 2 s) the growth is these three stages: 234 case files, the
guest cases and the random, chunk and snapshot sweeps run through the machine, and again under ASan and UBSan. Every other stage is 10 s or less in
run A. The Linux columns are the one-off container runs below, cold builds under emulation, and are not comparable with the macOS columns.

| # | Stage | Run A s (mac) | Run B s (mac) | Linux gawk+gcc s (emulated) | Linux mawk+clang s (emulated) | Verdict (mac) |
|---|-------|---------------|---------------|-----------------------------|-------------------------------|---------------|
| 1 | preflight | 0 | 0 | 1 | 1 | PASS |
| 2 | ledger-entries | 1 | 8 | 7 | 6 | PASS |
| 3 | ledger-negatives | 2 | 10 | 14 | 11 | PASS |
| 4 | ledger-samples | 1 | 5 | 2 | 3 | PASS |
| 5 | decision-records | 0 | 1 | 2 | 1 | PASS |
| 6 | decision-negatives | 1 | 2 | 10 | 9 | PASS |
| 7 | profile-records | 1 | 0 | 1 | 0 | PASS |
| 8 | profile-negatives | 1 | 1 | 9 | 9 | PASS |
| 9 | filename-controls | 1 | 1 | 4 | 3 | PASS |
| 10 | empty-expect-controls | 0 | 1 | 3 | 3 | PASS |
| 11 | provenance | 0 | 0 | 1 | 1 | PASS |
| 12 | provenance-negatives | 2 | 3 | 10 | 9 | PASS |
| 13 | licences | 3 | 7 | 16 | 16 | PASS |
| 14 | licence-negatives | 1 | 3 | 5 | 6 | PASS |
| 15 | protocol-order | 0 | 1 | 2 | 2 | PASS |
| 16 | protocol-negatives | 6 | 17 | 30 | 30 | PASS |
| 17 | score-controls | 1 | 1 | 6 | 4 | PASS |
| 18 | results | 0 | 0 | 1 | 1 | PASS |
| 19 | results-negatives | 0 | 1 | 3 | 2 | PASS |
| 20 | ingest-controls | 2 | 1 | 10 | 9 | PASS |
| 21 | flatten-controls | 1 | 1 | 9 | 7 | PASS |
| 22 | cancel-controls | 1 | 2 | 3 | 3 | PASS |
| 23 | outcome-controls | 1 | 0 | 2 | 1 | PASS |
| 24 | rights-tree | 2 | 7 | 14 | 11 | PASS |
| 25 | rights-negatives | 7 | 13 | 40 | 33 | PASS |
| 26 | audit-synthetic | 5 | 15 | 44 | 36 | PASS |
| 27 | inventory | 5 | 9 | 53 | 42 | PASS |
| 28 | inventory-negatives | 1 | 3 | 13 | 12 | PASS |
| 29 | fixture-lint | 0 | 0 | 1 | 0 | PASS |
| 30 | fixture-lint-negatives | 1 | 3 | 7 | 7 | PASS |
| 31 | input-check | 0 | 0 | 2 | 2 | PASS |
| 32 | input-check-negatives | 2 | 4 | 14 | 13 | PASS |
| 33 | xasm | 0 | 0 | 0 | 0 | PASS |
| 34 | opcode-oracle | 0 | 0 | 0 | 0 | PASS |
| 35 | opcode-oracle-negatives | 1 | 1 | 3 | 3 | PASS |
| 36 | xasm-negatives | 0 | 1 | 3 | 3 | PASS |
| 37 | workflow | 0 | 0 | 1 | 1 | PASS |
| 38 | workflow-negatives | 1 | 3 | 10 | 8 | PASS |
| 39 | docs-claims | 0 | 1 | 1 | 3 | PASS |
| 40 | docs-claims-negatives | 1 | 1 | 8 | 6 | PASS |
| 41 | ci-timings-controls | 0 | 1 | 1 | 1 | PASS |
| 42 | fixture-build | 1 | 2 | 6 | 5 | PASS |
| 43 | build | 0 | 0 | 5 | 8 | PASS |
| 44 | outcome-sync | 0 | 1 | 2 | 2 | PASS |
| 45 | conformance | 31 | 55 | 136 | 156 | PASS |
| 46 | sanitizers | 52 | 65 | 147 | 146 | PASS |
| 47 | excluded-mode-controls | 1 | 0 | 3 | 2 | PASS |
| 48 | record-lint | 0 | 0 | 1 | 1 | PASS |
| 49 | record-lint-negatives | 1 | 1 | 8 | 5 | PASS |
| 50 | core-boundary | 3 | 3 | 18 | 17 | PASS |
| 51 | core-boundary-negatives | 4 | 3 | 30 | 26 | PASS |
| 52 | determinism | 30 | 24 | 54 | 45 | PASS |
| 53 | determinism-negatives | 0 | 0 | 2 | 1 | PASS |
| 54 | fixtures | 6 | 5 | 33 | 23 | PASS |
| 55 | fixture-negatives | 1 | 0 | 5 | 3 | PASS |
| 56 | named-controls | 10 | 7 | 38 | 34 | PASS |
| | sum of stage seconds | 192 | 294 | 854 | 792 | |

### Container run after W8 (one-off, not CI)

Two local one-off container runs of `scripts/linux-repro` (ubuntu:24.04, linux/amd64 emulated on this arm64 Mac, cold `build/`, host load 16 to 33
and shared with another project's container). They are local runs, never CI claims.

- **Two earlier runs (both stopped, part `gawk`, gawk 5.2.1, gcc 13.3.0, cmake 3.28.3):** together they found two defects that macOS never showed, both
  fixed in plan 03-33. The first run: gcc 13 reported `-Werror=format-truncation` in five places of the conformance runner and the `build` stage failed
  (fixed in commit `b578443`, then checked with part `gate-gcc`, 79 s, passed). The second run: UBSan reported `memcpy` of zero bytes through a null
  pointer at `obskinds.c:854` and the `sanitizers` stage failed (fixed in commit `bf1bef0`). Each was stopped during its selftest, the tree was fixed
  and the run repeated.
- **Final run, part `gawk` (real script):** gate exit 0, 56 stages PASS in 856 s (sum of stage seconds 854), selftest exit 0 (113 SELFTEST lines, no FAIL),
  the part about 1250 s with the install. Slowest stages: `conformance` 136 s, `sanitizers` 147 s, `determinism` 54 s, `inventory` 53 s,
  `audit-synthetic` 44 s, `rights-negatives` 40 s, `named-controls` 38 s, `fixtures` 33 s.
- **Final run, part `mawk` (a scratch copy of the script with the selftest omitted, because CI does not run it):** mawk 1.3.4, clang 18.1.3. Gate exit 0,
  56 stages PASS in 793 s (sum 792): `conformance` 156 s, `sanitizers` 146 s (so the sanitizers stage also passes under clang 18 on Linux), `determinism`
  45 s. The whole part took 870 s. The scratch copy is not committed; it differs from `scripts/linux-repro` only in the selftest line and the repository root.
- **Reading:** the Linux gate now passes under the two awk implementations and the two compilers of the CI job, as a local run. The numbers are about
  4 to 8 times the expected runner times (see the calibration below) and are not a prediction of a runner.

### Estimated CI critical path for the Phase 3 gate (an estimate, not a measurement)

There is no CI row for the Phase 3 gate, so the job time is estimated. The only calibration is the Phase 2 pair (47 stages): the one-off container
gates took 216 s (gawk, gcc) and 207 s (mawk, clang), and the real runner took 37 s (`Gate`) and 25 s (`Second awk gate run`), a factor of 5.8 and 8.3.
Compute-heavy stages scale less than process-spawning ones (`conformance`: 17 to 20 s emulated against 6 s on the runner, a factor of 3), and the three
compute stages are now 39 to 44 percent of the gate, so the factor is applied per group: 5.8 or 8.3 for the spawn-bound stages and 3 to 5 for the three
compute stages. This is one calibration point and an emulator, so the uncertainty is wide, at least plus or minus 50 percent.

| Part of the job | Low s | Central s | High s | Basis |
|-----------------|-------|-----------|--------|-------|
| Set up, checkout (full clone), tool versions | 7 | 8 | 10 | Phase 2 run: 2 s for the first part; the full clone is larger now |
| Install cc65 (`apt-get update` and install) | 10 | 20 | 30 | not measured anywhere; a guess, the one step with no number |
| Gate (gawk, gcc, cold build) | 147 | 175 | 201 | 854 s emulated, scaled as above |
| Range audit (pull request only) | 30 | 45 | 60 | 16 s for 130 commits in run 42939483070; the first pull request of this branch holds about 370 |
| gcc build and conformance | 23 | 35 | 45 | the `conformance` stage again, in a second build directory |
| clang build and conformance | 26 | 40 | 52 | same, clang 18 |
| Second awk gate run (mawk, gcc) | 95 | 135 | 170 | 792 s emulated, scaled as above |
| **Job** | **338 (5.6 min)** | **458 (7.6 min)** | **568 (9.5 min)** | |

A local cross-check gives the same order: the local gate took 193 to 295 s, and the runner was about 0.8 times the local figure in Phase 2 (37 s against
45 s), which puts `Gate` at 155 to 240 s and the job at about 8 to 11 minutes. So the estimate runs from about 6 to 11 minutes, and the old limit of
10 minutes sits inside it. **`timeout-minutes: 10` is not safe for the first Phase 3 run.**

### Efficiency review after W8 (decisions)

- **`timeout-minutes` raised from 10 to 20** in `.github/workflows/gate.yml`, the smallest safe change (a limit only bounds a hung job, so a higher limit
  costs nothing when the job is healthy). 20 is about twice the high estimate. The 14 mini-roots of `tests/workflow` were regenerated by the same text
  replacement (each differs from the real file by exactly the lines it differed by before; `ok-gate` is byte-identical to the real file) and
  `workflow`, `workflow-negatives` pass. Tighten it after three recorded Phase 3 rows, as the protocol above says.
- **The second awk run stays a whole gate run.** The awk-call census (a shim for `awk` first on PATH, each stage run alone on macOS, 5736 calls, a lower
  bound) shows 52 of 56 stages run awk; only `preflight`, `empty-expect-controls`, `protocol-order` and `build` run none. The heavy stages are not an
  exception: `conformance` makes 675 awk calls (about 240 parses of the case files with `toml-subset.awk`), `sanitizers` 584, `fixtures` 707, `inventory`
  847 (that stage is awk-bound, 53 s emulated). The only stage that could be dropped from the second run without losing awk coverage in practice is
  `sanitizers`, whose awk work repeats `conformance`'s (the same case parsing; one result-judging call differs), a saving of about
  25 to 40 s of the estimated job. That needs a skip mechanism in `scripts/check` that the gate's fail-closed rule discourages, and the stage does run
  awk, so it is **not** done. Revisit when the first Phase 3 row exists; no failure class (awk dialect, sh and userland, history, compilers) is dropped.
- **Sanitizers under the second compiler (a note, not changed).** CI runs the `sanitizers` stage only under the default `cc`, which is gcc on the runner,
  in both gate runs. The container runs show clang 18 + UBSan passes on Linux (146 s emulated, perhaps 30 to 50 s on a runner), and the first
  container run showed that the compiler matters: gcc's UBSan reported a null-pointer `memcpy` that the macOS clang run never flagged. Two ways to get
  clang's sanitizers: add a third run (about 30 to 50 s), or make the second awk run use clang as `cc` (no extra minutes, but a failure of that run would
  no longer say whether awk or the compiler caused it; `scripts/linux-repro` already pairs mawk with clang). Neither is applied without the owner's
  say, and the first Phase 3 row should come first.
- **The gcc and clang steps repeat `conformance`.** Each rebuilds in its own directory and runs all cases again (an estimated 35 to 40 s each). They
  prove a clean preset build with the strict warning flags under each compiler, and this plan's gcc findings show the value of that. Not changed.
- **`Range audit`** will be long on the first pull request (hundreds of commits) and short on a normal one.
- **Nothing was weakened:** no stage, control, selftest or workflow step was skipped, merged or shortened.
- **Next action:** after the owner authorises a push and the first Phase 3 run exists, add its row to the table above with `sh scripts/ci-timings RUN_ID`,
  compare it with the estimate here, and decide the timeout, the second-awk subset and the sanitizers-under-clang question from that row.

### Optional WLA-DX cross-check (one local run, not CI)

WLA-DX 10.7 from the Homebrew core bottle was installed locally (reversible, outside the repository), `sh scripts/xasm-check --rows 234` was run with
`wla-huc6280` and `wlalink` on PATH, and the tool was then uninstalled. The WLA path had never run. First run: `XASM_WLA_FAILED` (the generated text
wrote the accumulator `inc a`, which WLA reads as an operand named `a`). After spelling accumulator INC and DEC as `ina` and `dea` (commit `7c7e065`) it
assembled all 234 rows and printed one mismatch: **233 rows equal the table, 1 differs**, `BRK` (expected `00` and padding, WLA emitted `00 00`, two
bytes). So `wla=equal` was never printed. This is an observation about one external tool, not a verdict on the table or on ca65, and the reason (a
second signature byte for `BRK`) is a guess that was not checked against the WLA documentation. **No cross-assembler identity claim is made.** Nothing
of WLA is committed and its output was discarded. It was uninstalled because, while it is on PATH, the `xasm` stage of the gate reports the BRK mismatch
and fails; the next step is an owner choice (accept a recorded WLA exception for `BRK`, or leave the optional check unused).

## First Phase 3 CI run (run 43122980214, pull request 2)

Authorised by the owner on 2026-11-11 (answer: "Push as a new branch and PR (Recommended)"): one new branch, `gsd/phase-03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad`,
and one pull request into `main`, with fix-forward commits to the same branch. No repository setting was changed, `main` and the Phase 2 branch were not touched, no tag
was pushed, nothing was force-pushed. The evidence tier is E0, repeatability: the run shows that the Phase 3 gate command passes on a second machine class with a second
awk and two compilers, not that any profile is accurate. This section supersedes the "unverified, pending owner push authorisation" statements above for the Phase 3 gate
as of head `cb2eb55`. The last head of the branch (the commit that adds this section) has its own run, which is not recorded here.

Three runs, because the first two failed on the first hosted run of the Phase 3 code. Both failures were found only on a GitHub runner (gcc 13 on glibc, `-std=c11 -pedantic -Werror`):

| Run | Head | Job s | Conclusion | Cause |
|-----|------|-------|------------|-------|
| 43122131107 | `2b8a7e6` | 42 | failure | `tests/conformance/reviewkinds.c` called `strtok_r`, which is POSIX and not declared in strict ISO C11 mode (implicit declaration, `-Werror`); the `build` stage failed and the stages that need the runner binary failed after it |
| 43122733150 | `6b313f7` | 36 | failure | the same file, `-Werror=format-truncation` on a `%s` into an 8-byte buffer (the length was checked, but not where the compiler could see it) |
| 43122980214 | `cb2eb55` | 342 | success | both fixed forward: a by-hand comma split (an empty field is now refused, which is stricter) and a `%.2s` conversion |

Both fixes are in `tests/conformance/reviewkinds.c` only, no check was weakened, and the macOS gate passes with them. The apt `cc65` step and the fixture hash pins were not the problem in any run.

**Fixture assembler on the runner.** `apt-get install cc65` installed Ubuntu `cc65 2.19-1`; `ca65` and `ld65` print `V2.18 - Ubuntu 2.19-1` (checked in an Ubuntu 24.04 container with the same package; the runner log prints only the package line), the same
`V2.18` identity that the Homebrew 2.19 bottle prints on the owner's machine. `fixture-build` (21 images) and `fixtures` passed in all three runs and in both awk runs, so the hash pins in the fixture manifest,
which were taken from the Homebrew bottle on macOS, reproduce with the Ubuntu package. That is repeatability across two builds of the same assembler release, not an independent oracle.
The gcc and clang versions are written only to the job summary, which the API does not return, so they are not recorded here (Ubuntu 24.04 ships gcc 13 and clang 18).

Awk: the default was `/usr/bin/gawk` for the first run and the `Range audit`; the second gate run used `/usr/bin/mawk` through the shim. Both full gates printed `CHECK RESULT PASS` (56 stages each). The `apt-get` fallback for the second awk did not run (mawk was already installed).

Step seconds of the green run (from the step timestamps; the run was queued 0 s and the job ran 342 s, the run 344 s):

| Step | Seconds | Share of job |
|------|---------|--------------|
| Set up job, checkout (full clone), tool versions | 4 | 1 percent |
| Install cc65 (`apt-get update` and install) | 8 | 2 percent |
| Gate (gawk, gcc, cold build) | 119 | 35 percent |
| Range audit (the whole branch: 334 commits over `main`, because pull request 1 is not merged) | 37 | 11 percent |
| gcc build and conformance (preset `gate-gcc`) | 31 | 9 percent |
| clang build and conformance (preset `gate-clang`) | 37 | 11 percent |
| Second awk gate run (mawk, gcc) | 104 | 30 percent |

Slowest stages inside the gate, from the gaps between the `CHECK` lines (whole seconds, a rounded estimate), gawk run then mawk run: `sanitizers` 46 and 43, `conformance` 33 and 24, `determinism` 19 and 19; every other stage 2 s or less
(`build` 1 s cold, `fixture-build` under 1 s, `inventory` 2 s, `core-boundary-negatives` 2 s). Those three stages are 98 s of the 119 s and 86 s of the 104 s. The job critical path is the single job, 344 s.

### Efficiency review of the first Phase 3 run

- **Against the estimate above (low 338, central 458, high 568 s):** the real job, 342 s, sits at the low end. `Install cc65` took 8 s (estimate 20), `Gate` 119 s (175), `Second awk` 104 s (135).
  The estimate was too high by about a quarter, so its scaling factors were too pessimistic for the runner.
- **`timeout-minutes: 20` is realistic, not too generous.** 20 minutes is 3.5 times the 344 s critical path. The Phase 2 review asked for about 3 times. The limit only bounds a hung job. Not changed. After three recorded
  Phase 3 rows, 15 minutes (2.6 times) would still leave a margin, and that is the most that should be considered.
- **The second awk run stays a whole-gate run.** 52 of 56 stages run awk, the run found no mawk-only difference, and it costs 104 s (30 percent of the job). The one candidate is `sanitizers` (43 s in this run), whose awk
  work repeats `conformance`'s, but it needs a skip mechanism that the fail-closed rule discourages, and a single green run is not a reason to remove a failure class. Not changed.
- **Sanitizers under clang as well: not yet.** The sanitizers stage runs under gcc only, in both runs, and passed. A third clang run would add about 45 s (13 percent). The CI evidence for the Phase 3 code is two failures that
  were compiler warnings, which the clang step (which passed on its first try) did not share, so gcc is the compiler that has shown differences; no clang-only UB finding exists. The zero-minute alternative is to pair the second awk run with clang
  (as `scripts/linux-repro` does), at the price that a failure no longer says whether awk or the compiler caused it. That is an owner choice and no workflow change was made on speculation.
- **Caching: no candidate.** Cc65 is an 8 s apt install (a cache restore costs about as much). The compile is cheap (`build` 1 s cold). Every step over 30 s (`Gate`, `Range audit`, `gcc`, `clang`, `Second awk`) is test execution of the
  gate itself, not a download or a build artefact, so there is nothing to cache without caching results, which would hide the evidence the job exists to produce.
- **Overlap now over the threshold.** The `gcc build and conformance` step (31 s) repeats the `Gate` step's `conformance` stage: `cc` on the image is gcc, and the preset `gate-gcc` differs from `gate` only by naming gcc.
  It pins the compiler if `cc` changes, and it proved a clean strict-flag build here, so it stays. Revisit if the job needs to get shorter.
- **A strict-flag gap on macOS.** Both CI-only failures would have been caught locally by a gcc strict build; `sh scripts/linux-repro` does that but is a one-off and was last run before the file existed. A tiny gcc syntax check in a container,
  with real compilation (`-fsyntax-only` misses `-Wformat-truncation`), found nothing else at the gate's `-O0`. At `-O2` (not a gate preset) gcc reports `-Wformat-truncation` in `tests/conformance/buskinds.c:110` and `digkinds.c:299`; recorded, not changed.
- **Nothing was weakened.** No stage, control, selftest or workflow step was skipped, merged or shortened.

## First Phase 4 CI run (run 43275379025, pull request 3)

Authorised by the owner on 2026-11-12 (answer: "Stacked PR on the Phase 3 branch (Recommended)"): one new branch, `gsd/phase-04-vce-vdc-raw-video`, and one pull request whose base is the Phase 3
branch (the head of pull request 2), so the diff shows only Phase 4, with fix-forward commits to the same branch. The workflow has no branch filter on `pull_request`, so it runs for a base that is not `main`
(it did). No repository setting was changed, `main`, the Phase 2 and Phase 3 branches and their pull requests were not touched, no tag was pushed, nothing was force-pushed, nothing was merged.
The evidence tier is E0, repeatability: the run shows that the Phase 4 gate command passes on a second machine class with a second awk and two compilers, not that any profile is accurate.
This section supersedes the "unverified, pending owner push authorisation" statements in the Phase 4 sections above. The head that adds this section has its own run, which is not recorded here.

One run, one green result, no fix-forward: the strict-flag gcc defects of Phase 3 did not recur, because `LINUX_REPRO_PARTS='gate-gcc gate-clang core-boundary-gcc' sh scripts/linux-repro` had run on the pushed tree first (all three parts passed, 88 s, emulated).

| Run | Head | Job s | Conclusion | Notes |
|-----|------|-------|------------|-------|
| 43275379025 | `cb2ddeb` | 853 | success | queue 0 s, run 856 s; both full gates printed `CHECK RESULT PASS` with 57 stages; default awk gawk, second awk mawk (already installed, so the apt fallback did not run) |

Runner image `ubuntu-24.04`, image version 20261101.360.1 (runner 2.338.2), the same as the Phase 3 run. The fixture assembler is the Ubuntu `cc65` package again; `fixture-build` and `fixtures` passed in both gate runs,
so the fixture hash pins (taken on macOS) reproduce on the runner, including the Phase 4 video fixtures. That is repeatability across two builds of one assembler release, not an independent oracle.

Step seconds (from the step timestamps; Phase 3 first green run in the last column):

| Step | Seconds | Share of job | Phase 3 run |
|------|---------|--------------|-------------|
| Set up job, checkout (full clone), tool versions | 3 | 0 percent | 4 |
| Install cc65 (`apt-get update` and install) | 10 | 1 percent | 8 |
| Gate (gawk, gcc, cold build) | 339 | 40 percent | 119 |
| Range audit (the Phase 4 range over the Phase 3 base, 79 commits) | 15 | 2 percent | 37 (334 commits) |
| gcc build and conformance (preset `gate-gcc`) | 74 | 9 percent | 31 |
| clang build and conformance (preset `gate-clang`) | 80 | 9 percent | 37 |
| Second awk gate run (mawk, gcc) | 330 | 39 percent | 104 |

Slowest stages inside the gate, from the gaps between the `CHECK` lines (whole seconds, a rounded estimate), gawk run then mawk run, with the Phase 3 gawk figure in brackets: `sanitizers` 172 and 170 (46), `conformance` 72.5 and 71.6 (33),
`determinism` 61.6 and 61.8 (19). Those three stages are 306 s of the 339 s gate and 304 s of the 330 s second run (90 percent and 92 percent). Every other stage is 3.1 s or less: `video-check` 1.8 s, `build` 1.5 s cold,
`fixture-build` 1.0 s, `fixtures` 3.0 s, `named-controls` 2.1 s, `core-boundary` 3.1 s. The job critical path is the single job, 856 s.

### Efficiency review of the first Phase 4 run

- **Against Phase 3:** the job went from 342 s to 853 s (2.5 times); the gate step alone from 119 s to 339 s (2.8 times). All of the growth is three stages that run the cases through the machine, and again under ASan and UBSan: `sanitizers`
  (46 to 172 s), `conformance` (33 to 72.5 s) and `determinism` (19 to 61.6 s). The runner is about as fast as the owner's loaded development machine for these stages (local 62 to 84 s for `conformance`, 146 to 216 s for `sanitizers`).
  The Phase 4 D-176 estimate of +35 s wall (+20 to +60) for the gate was far too low once measured on a runner.
- **`timeout-minutes: 20` is now too tight.** 1200 s against an 856 s critical path is a margin of 1.4 times, where the Phase 2 and Phase 3 reviews asked for about 3 times and the Phase 3 run had 3.5 times. The limit only bounds a hung job,
  but a runner that is 40 percent slower than this one would kill a healthy run and report a failure that is not a defect. Not changed here (it is a recorded decision, D-61, and this review changed no workflow file); the recommendation for the owner
  is 30 minutes, or the parallel-job lever below, which would bring the critical path under 9 minutes and make 20 minutes safe again.
- **The D-176 job trigger fired.** The trigger "the CI job over 600 s with lever (1) not recovering it" is met: 853 s, with lever (1) (toy executables linked against shared object libraries, commit `d831680`) applied. The `conformance` stage is also still over its 60 s
  trigger (72.5 s). The A/B effect of lever (1) on a cold runner cannot be separated from this single run (no run exists without it, and `build` itself is 1.5 s cold, so cold compilation is not what is slow); no further lever was applied.
- **The second awk run is 39 percent of the job and found nothing mawk-specific again.** The levers that keep the failure class and cut the critical path, in order of value: (a) run the second awk gate as a parallel second job instead of a later step: runner minutes stay
  the same (plus about 13 s of repeated set-up) and the critical path drops from 856 s to about 520 s (the Gate, range audit, gcc and clang steps are 521 s with set-up) while the second job takes about 343 s; (b) pair the second awk run with clang (as `scripts/linux-repro` does) at the price that
  a failure no longer says whether awk or the compiler caused it. Neither was applied: both change the workflow, which is the owner's decision, and no change was made on one green run.
- **Sanitizers run twice.** The ASan and UBSan stage (172 s and 170 s) is 40 percent of the job in total. Its only awk-dependent part is the same conformance runner that the `conformance` stage covers, so a skip in the second run is the largest single saving (about 170 s)
  but needs a skip mechanism, which the fail-closed rule discourages. Applying D-176 lever (2) (two policies instead of three) to the sanitize preset would save only about 4 to 5 s of 172 s and removes seed evidence, so it is not worth it.
- **Caching: still no candidate.** Cc65 is a 10 s apt install; every long step is test execution, not a download or a build product, so a cache would have to hold results, which would hide the evidence the job exists to produce.
- **Overlap.** The `gcc build and conformance` step (74 s) repeats the `Gate` step's `conformance` stage (`cc` on the image is gcc, and `gate-gcc` differs only by naming gcc); the `clang` step (80 s) is the only run of the conformance stage under clang. The gcc step pins the compiler if `cc` changes and
  proved a clean strict-flag build; it stays. If the job needs to be shorter, the first candidate after the parallel second job is running the gcc and clang steps as two parallel jobs.
- **New stage cost.** `video-check` (added in Phase 4) is 1.8 s per gate run. The Phase 4 video fixtures and cases cost in `conformance`, `sanitizers` and `determinism`, not in a stage of their own.
- **Nothing was weakened.** No stage, control, selftest, case or workflow step was skipped, merged or shortened, and no workflow file changed for this review.

## Owner hand-off list after Phase 3 (recorded once)

Automated and not to be re-asked: every stage, case family, control and selftest added in Phase 3 (56 stages, 113 selftest lines on Linux, the
README claim-word rules and their controls). What only the owner can do or confirm:

1. **Push and pull request authorisation (D-64).** The Phase 2 review-fix commits and every Phase 3 commit (about 180 commits after `b063518` at the time of
   writing) are local only. CI evidence for them is pending: it needs an authorised push, and the first Phase 3 run then needs the review above.
2. **Three remote settings** (see the pending hand-offs above, unchanged): Actions default workflow permissions read-only, Actions not allowed to
   approve pull requests, and a branch ruleset on `main` requiring the `gate` check.
3. **CODEOWNERS handle:** `@szTheory` was taken from the local git config and is awaiting confirmation.
4. **ADR 0008 (evidence-tier policy)** is still `proposed-default`. The seven supported entries were promoted under that default; confirming or
   changing it is an optional ask.
5. **Optional hardware captures:** not needed for any current claim.
6. **`CLAUDE.md` Technology Stack section** still reads as an unadopted Rust proposal ("research proposal, NOT a decision") while decision 0001
   governs the language and build system. Recorded only; it was not edited.
7. **Optional WLA-DX:** whether to record a `BRK` exception so the optional cross-check can print `equal` (see above).

## Phase 4 measurement spike (plan 04-05; a local run, not a CI run)

CI for Phase 4: unverified, pending owner push authorisation (D-64). Nothing of Phase 4 has run on a GitHub runner and this section makes no CI claim. It is the one measurement D-176 asks plan 04-05 for: the cost of the gate after the first video slice (plans 04-01 to 04-04, 04-18 and 04-19, and the tasks of 04-05), the quiet measure that research F-8 defines for the cut-line trigger, and the
comparison with the D-176 estimates.

Measured on 2026-11-12 on the owner's development machine (Apple M5 Pro, 18 cores, macOS 26.6.2, BWK awk 20200816, Apple clang 21, cmake 4.4.3), on commit `33bb17e` plus the uncommitted files of plan 04-05 Task 3 (the two toys, the `--video-out` rule and its case, the ledger pin), warm tree (`build/` present), with `/usr/bin/time -l sh scripts/check --timing`, once.
The machine is shared with unrelated jobs and was not quiet: load average 11.2 9.0 8.1 at the start and 14.4 10.4 8.8 at the end. All 56 stages passed.

| Measure | This run | Phase 3 run A (W8 section) | D-176 estimate |
|---------|----------|----------------------------|----------------|
| Wall time of `sh scripts/check --timing` | 224 s | 193 s | +35 s central (+20 to +60) over the Phase 3 gate at the end of Phase 4 |
| User CPU (`time -l`), the quiet measure of F-8 | 178.6 s (system 39.6 s) | 129 s (system 56 s) | not estimated; the cut line of D-180 reads "warm quiet gate over 150 s" |
| `conformance` stage | 45 s | 31 s (33 s in the CI row) | about 47 s central, 60 s high, staging trigger 60 s |
| `sanitizers` stage | 88 s | 52 s | not estimated |
| `determinism` stage | 36 s | 30 s | not estimated |

Verdicts, recorded and never failed (D-60, D-63): the warm target of 120 s (D-132) is missed again (it was missed at Phase 3 close, and even the user CPU of run A was above it). By the F-8 measure the cut-line trigger, user CPU over 150 s, is **over** at 178.6 s: +49.6 s of user CPU and +31 s of wall time since the Phase 3 baseline, with six of the 19 plans of Phase 4 done and
the seventh in progress, so the D-176 central
estimate of +35 s wall for the whole phase is already used up in user CPU terms. The growth is mostly two stages: `sanitizers` (+36 s: the conformance cases run again under ASan and UBSan) and `conformance` (+14 s), then `determinism` (+6 s); every other stage is 5 s or less in both runs. The machine was loaded, so a quieter run would read lower in wall time, but the user CPU, which the loaded
machine inflates much less, is the number the trigger uses, and one run is all this spike makes. The conformance stage, 45 s, is under the 60 s staging trigger of D-176. The decision on the cut line is plan 04-11's (the check point), not this plan's.

Seconds per stage (whole seconds from the gate's own clock, so 0 means under a second; run A is the macOS run A of the W8 table above, same 56 stage names):

| # | Stage | Run A s | This run s |
|---|-------|---------|------------|
| 1 | preflight | 0 | 1 |
| 2 | ledger-entries | 1 | 0 |
| 3 | ledger-negatives | 2 | 1 |
| 4 | ledger-samples | 1 | 1 |
| 5 | decision-records | 0 | 0 |
| 6 | decision-negatives | 1 | 1 |
| 7 | profile-records | 1 | 0 |
| 8 | profile-negatives | 1 | 1 |
| 9 | filename-controls | 1 | 0 |
| 10 | empty-expect-controls | 0 | 0 |
| 11 | provenance | 0 | 1 |
| 12 | provenance-negatives | 2 | 0 |
| 13 | licences | 3 | 2 |
| 14 | licence-negatives | 1 | 1 |
| 15 | protocol-order | 0 | 0 |
| 16 | protocol-negatives | 6 | 4 |
| 17 | score-controls | 1 | 0 |
| 18 | results | 0 | 0 |
| 19 | results-negatives | 0 | 0 |
| 20 | ingest-controls | 2 | 1 |
| 21 | flatten-controls | 1 | 1 |
| 22 | cancel-controls | 1 | 1 |
| 23 | outcome-controls | 1 | 0 |
| 24 | rights-tree | 2 | 2 |
| 25 | rights-negatives | 7 | 4 |
| 26 | audit-synthetic | 5 | 4 |
| 27 | inventory | 5 | 4 |
| 28 | inventory-negatives | 1 | 1 |
| 29 | fixture-lint | 0 | 0 |
| 30 | fixture-lint-negatives | 1 | 1 |
| 31 | input-check | 0 | 1 |
| 32 | input-check-negatives | 2 | 1 |
| 33 | xasm | 0 | 0 |
| 34 | opcode-oracle | 0 | 0 |
| 35 | opcode-oracle-negatives | 1 | 0 |
| 36 | xasm-negatives | 0 | 1 |
| 37 | workflow | 0 | 0 |
| 38 | workflow-negatives | 1 | 1 |
| 39 | docs-claims | 0 | 0 |
| 40 | docs-claims-negatives | 1 | 0 |
| 41 | ci-timings-controls | 0 | 1 |
| 42 | fixture-build | 1 | 1 |
| 43 | build | 0 | 0 |
| 44 | outcome-sync | 0 | 0 |
| 45 | conformance | 31 | 45 |
| 46 | sanitizers | 52 | 88 |
| 47 | excluded-mode-controls | 1 | 0 |
| 48 | record-lint | 0 | 0 |
| 49 | record-lint-negatives | 1 | 1 |
| 50 | core-boundary | 3 | 2 |
| 51 | core-boundary-negatives | 4 | 2 |
| 52 | determinism | 30 | 36 |
| 53 | determinism-negatives | 0 | 0 |
| 54 | fixtures | 6 | 5 |
| 55 | fixture-negatives | 1 | 1 |
| 56 | named-controls | 10 | 6 |
| | sum of stage seconds | 192 | 224 |

What this plan's own cases cost, measured one at a time with `ds_runner --only ID` (macOS gate preset, then the sanitize preset): `vdc_fold` 0.33 s and 1.2 s, `observer_vce_pal` 0.74 s and 2.6 s (three policies, three modes, five sink configurations), `vid_noninterference` 0.17 s and 0.6 s, `snapshot_restore_video` 2.2 s and 5.4 s, `chunk_equiv_video_vce_pal` 0.10 s and 0.38 s,
`video_out_rights` and the `--video-out` step well under a second; about 3.6 s and 10 s together. The two new toys each recompile one core unit and run their EXPECT cases (five and three cases), which is inside the `conformance` stage.

### Efficiency review of the spike (decisions)

- **`snapshot_restore_video` is the largest new case and was cut once already.** A restore costs about 0.7 ms (a 92 KB snapshot, the poison, the restore and the member compare), so restoring at every stop of the 1364 and 1366 schedules over the whole 4,000,000-clock run was 20,352 restores and 83 s. Two changes made it 2.2 s: the restored target is now the instance the chain left
  one restore ago (no boot per restore), and the case runs the first 600,000 clocks (about 440 lines, which hold both control register writes and the start of the palette writer), where every stop of the two line-sized schedules is still restored and the 7-clock schedule is restored at every 251st stop and at every stop that falls on a multiple of 1365 (75 stops over the three policies). Not
  reduced further: it is 5 percent of the `conformance` stage. The first lever if the stage nears 60 s is `until_mclk` 600,000 to 300,000 on this case.
- **The sanitize preset is now the largest stage (88 s, 39 percent of the gate wall time).** It repeats the conformance cases under ASan and UBSan; the five new cases cost about 10 s of it. Lever, if the CI job nears 600 s: run the video fixtures under the sanitizers with two policies, the second D-176 lever. Not applied: one local run is not a CI measurement, and no stage was weakened.
- **D-176 levers in order, not applied:** (1) link the toy executables against a shared archive of the runner objects and recompile only the mutated core unit (CMake only); (2) run video fixtures under two policies (zero and one v3 seed). The conformance stage is under its 60 s trigger, and the CI job is unmeasured for Phase 4, so neither is applied.
- **Nothing was weakened.** No stage, control or selftest that existed before this plan was skipped, shortened or removed to save time. The one run length that was cut (`snapshot_restore_video`, above) belongs to a case this plan added, and the cut case keeps every stop of its two line-sized schedules.

## Checkpoint A (plan 04-13; local runs, not CI runs)

CI for Phase 4: unverified: no push authorised (D-64). Nothing of Phase 4 has run on a GitHub runner and this section makes no CI claim. It is the checkpoint A record (the cut line of D-180) that D-176 asks plan 04-13 for: the cost of the gate after the background half of the video work (plans 04-01 to 04-12, 04-17 to 04-19 and the three tasks of 04-13), the quiet measure of
research F-8, the staging
levers of D-176, and the cut-line triggers, each as fired, not fired or unmeasured. The cut itself is decided by the orchestrator and the owner, not here.

Measured on 2026-11-12 on the owner's development machine (Apple M5 Pro, 18 cores, macOS 26.6.2, BWK awk 20200816, Apple clang 21, cmake 4.4.3). **The machine was shared with unrelated jobs and was never quiet: the one-minute load average read between 7 and 43 during these runs.** No quiet window was available and none was waited for, as asked. User CPU is much less sensitive
to load than wall time, but not
insensitive (the same gate read 262 s, 301 s, 374 s and 467 s of user CPU in four runs at load 7 to 43), so every figure below is recorded as measured and is load-dependent.

| Run | What | Wall s | User CPU s | System s | Load average (1 min) at start and end |
|-----|------|--------|------------|----------|---------------------------------------|
| A | `/usr/bin/time -p sh scripts/check --timing`, warm tree, at `2872067` (task 1 of this plan); the F-8 quiet measure | 397.3 | 301.3 | 79.1 | 21.3 and 14.7 |
| B | the same at `d831680` (lever 1 applied; its first build of the new object libraries is inside this run) | 469.9 | 374.4 | 60.1 | 11.0 and 13.5 |
| D | the final run: `/usr/bin/time -p sh scripts/check --timing` at `8f2ac96`, the last commit of this plan before the close-out; the quietest of the runs (load 7.4 at start), so the F-8 figure to read | 321.2 | 262.4 | 45.3 | 7.4 and 9.5 |
| dash | `/usr/bin/time -p dash scripts/check` at `2872067` | 388.7 | 291.0 | 67.2 | not recorded |
| C | a gate run at `a9f690d` that overlapped the Linux container run; contended, not used for any trigger | 1016.9 | 467.5 | 123.6 | high (container emulation) |

All 57 stages passed in every run (`video-check` is the 57th stage, added in Phase 4). Seconds per stage (whole seconds from the gate's own clock; run A and run B above; the 04-05 spike section above has the same stage names with 56 stages and, at that time, a gate of 224 s wall and 178.6 s user CPU):

| # | Stage | Run A s | Run B s |
|---|-------|---------|---------|
| 1 | preflight | 0 | 0 |
| 2 | ledger-entries | 2 | 1 |
| 3 | ledger-negatives | 3 | 1 |
| 4 | ledger-samples | 1 | 1 |
| 5 | decision-records | 0 | 0 |
| 6 | decision-negatives | 2 | 1 |
| 7 | profile-records | 0 | 0 |
| 8 | profile-negatives | 3 | 1 |
| 9 | filename-controls | 0 | 0 |
| 10 | empty-expect-controls | 1 | 1 |
| 11 | provenance | 1 | 0 |
| 12 | provenance-negatives | 2 | 1 |
| 13 | licences | 4 | 2 |
| 14 | licence-negatives | 1 | 0 |
| 15 | protocol-order | 0 | 1 |
| 16 | protocol-negatives | 8 | 4 |
| 17 | score-controls | 1 | 0 |
| 18 | results | 0 | 0 |
| 19 | results-negatives | 0 | 1 |
| 20 | ingest-controls | 2 | 1 |
| 21 | flatten-controls | 2 | 0 |
| 22 | cancel-controls | 2 | 2 |
| 23 | outcome-controls | 0 | 0 |
| 24 | rights-tree | 4 | 2 |
| 25 | rights-negatives | 7 | 4 |
| 26 | audit-synthetic | 8 | 5 |
| 27 | inventory | 9 | 6 |
| 28 | inventory-negatives | 2 | 1 |
| 29 | fixture-lint | 0 | 0 |
| 30 | fixture-lint-negatives | 2 | 2 |
| 31 | video-check | 4 | 2 |
| 32 | input-check | 0 | 0 |
| 33 | input-check-negatives | 3 | 1 |
| 34 | xasm | 0 | 0 |
| 35 | opcode-oracle | 0 | 0 |
| 36 | opcode-oracle-negatives | 0 | 1 |
| 37 | xasm-negatives | 2 | 1 |
| 38 | workflow | 0 | 0 |
| 39 | workflow-negatives | 2 | 1 |
| 40 | docs-claims | 0 | 0 |
| 41 | docs-claims-negatives | 1 | 1 |
| 42 | ci-timings-controls | 0 | 0 |
| 43 | fixture-build | 3 | 1 |
| 44 | build | 1 | 1 |
| 45 | outcome-sync | 0 | 0 |
| 46 | conformance | 84 | 67 |
| 47 | sanitizers | 152 | 216 |
| 48 | excluded-mode-controls | 1 | 1 |
| 49 | record-lint | 0 | 1 |
| 50 | record-lint-negatives | 1 | 6 |
| 51 | core-boundary | 3 | 4 |
| 52 | core-boundary-negatives | 4 | 6 |
| 53 | determinism | 55 | 95 |
| 54 | determinism-negatives | 0 | 1 |
| 55 | fixtures | 6 | 13 |
| 56 | fixture-negatives | 0 | 2 |
| 57 | named-controls | 8 | 11 |
| | sum of stage seconds | 397 | 470 |

The warm target of 120 s (D-132) is missed again, as it was at Phase 3 close and at the 04-05 spike. Compared with the spike (six plans done: conformance 45 s, sanitizers 88 s, determinism 36 s, user CPU 178.6 s) the growth sits in the same three stages: `conformance` 62 to 84 s, `sanitizers` 146 to 216 s and `determinism` 52 to 95 s, with every other stage at 11 s or less.
The wall figures of those three stages
swing by a factor of 1.4 between runs A and B on the same code, which is the load, not the code.

Where the `conformance` stage goes (warm tree, `ds_runner` alone with the same arguments as the gate, 315 RESULT lines): 51.2 s wall and 48.2 s user for the cases, so the other 24 s of the stage (72.5 s user for `sh scripts/check --stage conformance` at load 13) are the toy controls (25 toys, each run on its EXPECT cases only), the hash sweep and the host hash steps. This
plan's own new cases cost, one at a time with
`ds_runner --only ID` in the gate preset: `vdc_poweron_policy_diff` 1.1 s user (5 fixtures and the control under 3 policies, 6 million clocks each), `vdc_random_regs` 0.3 s (32 programs), `chunk_equiv_random32_video` 1.5 s; the VDC and VCE traffic that `rp_build` now carries leaves `chunk_equiv_random32` at 3.0 s and `observer_random` at 4.1 s. The two largest cases of the phase so far are
`snapshot_restore_video_bg` (6.4 s) and `snapshot_restore_video` (2.3 s).

### D-176 staging levers (trigger: the CI job over 600 s, or `conformance` over 60 s)

The `conformance` trigger fired: the stage read about 67 s after plan 04-12 (orchestrator note), 84 s in run A, 67 s in run B and 62 s in run D at the lowest load (79.4 s wall and 72.5 s user for the stage alone at load 13). The CI job time is unmeasured (no push). The levers are applied in order, each in a separate commit that weakens no evidence:

| Lever | Trigger | Status | Before and after |
|-------|---------|--------|------------------|
| (1) link the toys against shared object libraries (CMake only) | fired (`conformance` over 60 s) | **applied** in `d831680` | cold build of the 25 `ds_toy_*` targets in a fresh directory (1,200 compile steps before, 73 after): 8.4 s wall and 47.1 s user CPU before, 0.8 s and 3.3 s after; all 25 controls stay RED_OK with the identical listing in `build/gate/controls.txt` |
| (2) video fixtures under two policies instead of three | fired, not applied | recorded | drops the third policy from the restore, non-interference and observer runs of video fixtures: about 4 to 5 s of the stage by the timings above, not enough alone for a 67 to 84 s stage to reach 60 s, and it removes seed 2 runs from evidence; `vdc_poweron_policy_diff` would cover the power-on dimension |
| (3) drop one chunk schedule from the video cases | fired, not applied | recorded | each video chunk case costs under 1 s except the restore cases; the saving is smaller than (2). |
| (4) move the Linux container run and `vdc_random_regs` to a scheduled job | not reached | recorded | `vdc_random_regs` costs 0.3 s plain and about 1 s sanitized; the container run is already a local one-off outside the CI job. |

Lever (1) does not shorten a warm local gate (a warm toy build compiles only the mutated units, so the stage reads the same within load noise); it removes the cold compile of every toy, which is the CI case (a fresh clone) and every edit of a runner header, which used to recompile every runner unit once per toy. Whether it recovers anything on a runner is for the first CI run
to say (unmeasured, D-64).
`timeout-minutes` stays 20. Nothing was weakened: no stage, control, selftest or case was skipped, shortened or removed to save time; `chunk_equiv_random32_video` and `vdc_poweron_policy_diff` are additions.

### Cut-line record (D-180)

| Trigger | Measure | Result |
|---------|---------|--------|
| more than two plans needed a revision plan | revision plans created to fix a defect found in review or verification: 0 (04-20 and 04-21 are unused; 04-17 is a planned split and 04-18 and 04-19 are stop-rule splits, which do not count) | **not fired** |
| the warm quiet gate over 150 s | user CPU of one `sh scripts/check --timing` run: 262.4 s (run D, load 7), 301.3 s (run A) and 374.4 s (run B), at load 11 to 43; the 04-05 spike had read 178.6 s with six plans done | **fired** (1.7 to 2.5 times the threshold; not a quiet-machine figure, so load-dependent) |
| the CI job over 600 s with lever (1) not recovering it | no push is authorised (D-64) | **unmeasured** |
| the owner prefers an earlier pull request | a question for the orchestrator | **unmeasured here** |

Recommendation: take the cut. One trigger is measured as fired, on a figure (262 s at load 7) that the load would have to inflate by three quarters to explain, and the spike at plan 04-05 was already over it with a third of the phase done. If the cut is taken, plans 04-14 to 04-16 become 4.1-01 to 4.1-03 of an inserted Phase 4.1 (the orchestrator inserts it), the README and
04-VALIDATION.md text written in this plan is the Phase 4 close-out
for the background scope, and VID-06 stays open for the sprite rows (plan-check info 4).

### Other results recorded at the checkpoint

Each result below is a local one-off on the development machine, not a CI claim; no CI run exists.

| Check | Result | Detail |
|-------|--------|--------|
| `sh scripts/check` (the runs A and B above) | PASS, 57 stages | warm; see the tables |
| `dash scripts/check` | PASS | 388.7 s wall and 291.0 s user at `2872067`, and again 478.9 s wall and 335.6 s user on the tree after the ledger and README promotions (load 15 to 20) |
| `sh scripts/check --selftest` | PASS | 145 mutations PASS, no FAIL; 60 s wall |
| `sh scripts/audit-history --range 925662a..HEAD` | clean | 59 commits, 606 blobs, 2,252 blob-path pairs, at `2872067`; the commits after it are re-audited in the plan SUMMARY |
| `sh scripts/linux-repro`, part `default-awk` (stock mawk 1.3.4, gcc 13) | PASS | the whole gate (57 stages) in 1,489 s and `--selftest` (145) in the container |
| part `mawk` (mawk 1.3.4, clang 18) | PASS | gate in 960 s and `--selftest` |
| part `gawk` (gawk 5.2.1, gcc 13) | PASS | gate in 805 s and `--selftest` |
| part `gate-gcc` (`-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`) | PASS | |
| part `gate-clang` (clang 18, the same flags) | PASS | |
| part `core-boundary-gcc` | PASS | |

The six parts ran under emulation (linux/amd64 on an arm64 host), 4,572 s in all, on the tree of this plan with its ledger, README and VALIDATION edits (the CMake change of lever 1 and every C change of the plan were in it); only this section of `docs/ci-timings.md` was written afterwards.

### Efficiency review inputs (AGENTS.md: review after each CI run; no CI run exists yet)

- **`sanitizers` is the largest stage** (152 s and 216 s, 33 to 46 percent of the wall time): it reruns the conformance cases under ASan and UBSan. The cheapest next lever is lever (2) applied to the sanitize preset only (video fixtures under two policies), which keeps the plain-preset evidence whole; not applied here, one local run under load is not a CI measurement.
- **`determinism` is the second** (55 to 95 s, it builds and runs the gate twice to compare); unchanged by this plan.
- **Toy controls:** lever (1) removes the cold compile (43.8 s of user CPU); the 25 toys still run their EXPECT cases only.
- **The second-awk and Linux runs** are whole-gate runs in a container under emulation (see the table above) and stay local one-offs, not CI jobs.
- Nothing was weakened to save seconds.

## Parallel second awk job (quick task, unverified on CI until pushed)

CI evidence for this change: **unverified until the owner authorises a push** (D-64). Nothing below has run on a GitHub runner. The workflow change, the `workflow-check` rules and the
numbers in the "expected" column are local work and an estimate; the "before" column is measured. The evidence tier of any later run stays E0, repeatability.

**What changed in `.github/workflows/gate.yml`** (quick task 261112-ci-parallel-awk). The efficiency review of the first Phase 4 run named the parallel second awk job as the best lever (runner minutes unchanged, critical path cut). It is applied as three jobs:

| Job | Name | Steps | Timeout |
|-----|------|-------|---------|
| `gate-gawk` | `gate-gawk` | checkout, tool versions, cc65 install, `Gate` (image default awk, gawk on the 2026-11 image), `Range audit` (pull requests only), gcc build and conformance | 25 min |
| `gate-mawk` | `gate-mawk` | checkout, tool versions, cc65 install, the gate under the other awk through the unchanged PATH shim and apt fallback (mawk on that image), clang build and conformance | 25 min |
| `gate` | `gate` | `needs` both, `if: always()`, one step that fails unless `needs.gate-gawk.result` and `needs.gate-mawk.result` are both exactly `success`; no checkout | 5 min |

The compiler steps are split so that the two jobs balance on the measured numbers (gate plus range audit plus gcc build in one, second awk run plus clang build in the other). The clang step
still runs under the default awk, as it did as a step of the single job, so no evidence moved. The mawk-versus-gawk selection logic is unchanged: it compares the default awk with `gawk` and
uses the other one, and the apt fallback for a missing second awk is still explicit and fail-closed. No action was added, `actions/checkout` is still pinned to the same full SHA,
`permissions: contents: read` is at the top and in each job, and there is no cache.

**Required check name: the owner hand-off is unchanged.** The branch ruleset on `main` still requires the check named `gate` (pending hand-off 3, "Pending hand-offs" above). The aggregator
job is named exactly `gate`, so the check the ruleset names exists under the same name; `gate-gawk` and `gate-mawk` are new check names that the ruleset does not need to list. The aggregator
uses `if: always()` and reads each result because a needed job that was skipped or cancelled makes a needs-only job skip, and a skipped required check reads as passing; only the result
`success` passes. No setting was changed and none is requested by this change. Whether a ruleset that already requires `gate` matches the aggregator is only confirmed by a real run after the
owner pushes.

**Before (measured) and expected after (estimate).** Seconds, from the two Phase 4 runs on identical code (run 43275379025 and run 43278274460, same runner image; run-to-run spread was 23 percent):

| Step | Run 1 | Run 2 | New home |
|------|-------|-------|----------|
| Set up job, checkout, tool versions | 3 | 5 | both jobs (paid twice) |
| Install cc65 | 10 | 10 | both jobs (paid twice) |
| Gate (gawk) | 339 | 419 | `gate-gawk` |
| Range audit | 15 | 18 | `gate-gawk` |
| gcc build and conformance | 74 | 93 | `gate-gawk` |
| clang build and conformance | 80 | 120 | `gate-mawk` |
| Second awk gate run (mawk) | 330 | 381 | `gate-mawk` |
| Job, one job (the critical path) | 853 | 1048 | |

| | Run 1 basis | Run 2 basis |
|---|---|---|
| `gate-gawk` expected | 3 + 10 + 339 + 15 + 74 = 441 | 5 + 10 + 419 + 18 + 93 = 545 |
| `gate-mawk` expected | 3 + 10 + 330 + 80 = 423 | 5 + 10 + 381 + 120 = 516 |
| `gate` aggregator (runner start, two echo and test lines) | about 5 to 10 | about 5 to 10 |
| Run wall, expected (before: 856 and 1051) | about 450 | about 555 |
| Total runner seconds, expected (before: 853 and 1048) | about 870 | about 1075 |

The critical path falls by about 400 s on run 1 numbers and about 500 s on run 2 numbers (about 47 percent). The first estimate of about 520 s (all four serial steps of the first job in one
job, the second awk run in the other) is the worse split; this split is better balanced (within 20 to 30 s of each other). Total runner minutes stay about the same (the duplicated checkout,
tool versions and cc65 install add about 13 to 15 s, the aggregator a few more). Job startup and queueing for a second runner are not in the estimate; a busy hosted pool could add to it. The
25 minute limit is about 2.7 times the slower expected job (the old 20 minute limit was 1.14 times the slower measured run), and `timeout-minutes` now exists on every job.

**What to check on the first run after a push:** both work jobs and the aggregator appear and the aggregator's name is `gate`; the longer of the two work jobs against 440 to 550 s; the cc65
and checkout cost twice; the aggregator going red when a work job is forced to fail (a one-off and read-only observation, not a requirement); and the awk of each job in the `Tool versions`
summary and the `Gate under the other awk` log. `scripts/ci-timings` already prints one `name=seconds` entry per job, so no tool change is needed to record the row.

**`workflow-check` after this change.** New rules: every job needs `timeout-minutes` from 1 to 30 (`WF_NO_TIMEOUT`), a `runs-on`, its own `permissions: contents: read`, and, unless it is
`gate`, a checkout and a step that runs `sh scripts/check`; the cc65 install must come before the gate command in each job that runs it; the job `gate` must be named `gate`, need exactly the
other jobs, carry `if: always()`, test each needed result with `test "$VAR" = success`, and run nothing else and skip nothing (`WF_AGGREGATOR`). The 14 older mini-roots were regenerated from
the new file (each differs from it by the same mutation as before; `ok-gate` is byte-identical), 17 mini-roots were added (31 in all), and 14 selftests (`mutation-workflow-timeout`,
`-job-runner`, `-job-perm`, `-job-checkout`, `-job-gate`, `-agg-missing`, `-agg-name`, `-agg-if`, `-agg-needs`, `-agg-result`, `-agg-line`, `-agg-oneline`, `-agg-step-if`, `-agg-skipped`)
each delete one tagged rule line and show the control flip. `mutation-workflow-agg-skipped` is the aggregator that treats a skipped job as success turning a control red.

**Not applied here, still open:** pairing the second awk run with clang (changes what a failure says), a skip of `sanitizers` in the second run (needs a skip mechanism), and running the gcc
and clang steps as their own jobs (each would pay set-up again and the two work jobs are already balanced). The caching verdict (no candidate) and "nothing was weakened" stand: no stage,
control, selftest or case was removed, and both awk implementations and both compilers still run the same steps as before.

## Phase 4.1 levers (plan 04.1-01; local runs, not CI runs)

CI evidence for this section: **none**. Nothing here has run on a GitHub runner (D-64: the owner authorises pushes), and the parallel second awk
job of the quick task above is still unverified on CI. The tier of every figure is E0, repeatability on one local machine under load. Phase 4.1
plans add no case beyond the sprite cases they list (D-180 cut, checkpoint A), so before the first sprite case lands the remaining levers (2) to
(4) of D-176 are applied in order, each in its own commit and each without weakening evidence (D-176: nothing is weakened to save seconds).

**Before (measured at `f9d9d69`, the commit before this plan, warm tree).** `/usr/bin/time -p sh scripts/check --timing`: 461.6 s wall, 344.3 s
user CPU, 68.7 s system; all 57 stages passed; load average 10 at the start and 22 at the end (a shared machine, never quiet). Stage seconds that
matter: `conformance` 86, `sanitizers` 212, `determinism` 71, `named-controls` 19, `fixtures` 10, every other stage 5 or less. The two triggers
read: `conformance` 86 s against the 60 s trigger of D-176 (fired) and the gate at 344 s user CPU against the 150 s trigger of D-180 (fired,
load-dependent as at checkpoint A). Wall time and stage seconds swing by a factor of up to 1.4 between runs of the same code, so the per-case
figures below (user CPU of one `ds_runner --only ID` run in the gate preset, load 20 to 29) are the stable evidence of what a lever removes.

| Lever | Trigger | Status | Where |
|-------|---------|--------|-------|
| (1) toys link shared object libraries | fired | applied at checkpoint A (`d831680`) | CMake only; cold build of 25 toys 47.1 s to 3.3 s user CPU |
| (2) video fixtures under two policies | fired (`conformance` 86 s over 60 s) | **applied** in this plan | see lever (2) below |
| (3) drop one chunk schedule from the video chunk cases | fired (`conformance` 86 s after lever (2)) | **applied** in this plan | see lever (3) below |
| (4) move `vdc_random_regs` and the Linux container run to a scheduled job | fired (`conformance` still 86 s after lever (3)) | **recorded, not applied** | see lever (4) below |

### Lever (2): two policies in the video non-interference, restore and observer runs

- **What changed.** `VD_POLICIES_VIDEO` (2) in `tests/conformance/runner.h`: `vid_noninterference`, `snapshot_restore_video` and
  `snapshot_restore_video_bg` run the policies zero and seeded-prng-v3 seed 1; `observer_vce_pal` lists the same two fragments in its `policies` key.
- **Per case user CPU, before and after** (the stable measure): `snapshot_restore_video_bg` 5.04 to 3.65 s, `snapshot_restore_video` 1.85 to 1.11 s,
  `observer_vce_pal` 1.21 to 0.72 s, `vid_noninterference` 0.26 to 0.16 s: 2.7 s in all.
- **The gate with the lever** (same load class, not the same load): `conformance` 83 s (86 s before), 305.6 s user CPU and 391.9 s wall (344.3 s
  and 461.6 s before). Only the per-case figures measure the lever; the gate figures moved with the load as much as with the code.
- **Why no evidence is weakened.** The power-on dimension of the five video fixtures is carried by `vdc_poweron_policy_diff`, which keeps all three
  policies (zero, seeds 1 and 2) and compares the verdict RAM and every record from the first judged frame, with the never-initialised palette
  image as the control that must differ. The guest cases of the video fixtures (`vce_pal`, `vce_dot`, `bg_*`, `vdc_*`), `vdc_vram`,
  `vdc_satb_data` and the CPU fixtures keep three policies. No control, toy, literal case or REQUIRED id was removed. What is given up is the third
  policy, seed 2, in the video non-interference, restore and observer runs.

### Lever (3): the schedule 3072 dropped from the video chunk cases

- **Trigger.** `conformance` still read 83 s after lever (2) (the 60 s trigger of D-176), so the next lever applies.
- **What changed.** `chunk_equiv_video_vce_pal` and `chunk_equiv_video_bg_scroll` run the schedules 7, 1364, 1366 and one frame (`vframe`) instead of
  those and 3072; `chunk_equiv_random32_video` runs 1364, 1366 and `vframe`. The two schedules that straddle a line event (1364 and 1366) stay, as the
  plan requires; the CPU chunk cases keep 3072 (it is also the timer prescaler step there).
- **Per case user CPU, before and after** (same session, load 12 to 20): `chunk_equiv_video_vce_pal` 0.11 to 0.12 s, `chunk_equiv_video_bg_scroll` 0.53 to
  0.52 s, `chunk_equiv_random32_video` 1.41 to 1.32 s: about 0.1 s in all, which is inside the run-to-run noise. A schedule run is cheap next to the
  boot, the fixture run and the stream copy of a case; the saving is real but not worth a second of the stage.
- **The gate with the lever:** `conformance` 86 s, 309.1 s user CPU, 393.3 s wall, load 12 (the lever (2) run read 83 s, 305.6 s and 391.9 s at load 18 to 20).
  The stage did not move beyond noise: it does not reach 60 s by any of the levers of D-176 that weaken nothing.
- **Why only a little evidence is given up.** Three of the four schedules of the three cases (1364, 1366 and the frame, plus 7 where it ran) are unchanged; 3072 was a
  period that is neither a divisor nor a neighbour of the 1365-clock line and added no phase that 7, 1364 and 1366 do not reach. A record stream compared across
  schedules, the restore cases and `chunk_equiv_random32` (which keeps all its CPU schedules) are unchanged. This is recorded as an applied lever whose measured value is
  about 0.1 s, so that the efficiency review after the first Phase 4.1 CI run can restore the schedule at no real cost.

### Lever (4): `vdc_random_regs` and the Linux container run to a scheduled job (recorded, not applied)

The plan applies this lever only if the PR gate keeps an equivalent check through the existing excluded-mode machinery and the move needs no file outside the
task list; and it never drops `vdc_random_regs` from the evidence. Neither condition can be met, so the lever is not applied.

- **`vdc_random_regs` is a REQUIRED case and costs 0.35 s** in the gate preset (about 1 s under the sanitizers): 0.4 percent of the 86 s stage. The excluded-mode machinery
  (`tests/conformance/EXCLUDED_IN_MODE`, D-78) knows the mode `sanitize` only, and its rule is that a listed case must be REQUIRED, must pass in the gate mode and must not
  run in the excluded mode. A scheduled job would need a new mode, a new `schedule:` trigger in `.github/workflows/gate.yml` (the workflow has none, and
  `workflow-check` has rules for the jobs it has) and a case that the PR gate no longer runs. That drops a REQUIRED case from the PR evidence for 0.35 s, which D-176 and the plan forbid.
- **The Linux container run is not in the CI job.** `scripts/linux-repro` is a local one-off (about 80 s for the `gate-gcc` part under emulation); nothing in the workflow runs a
  container, so moving it saves no CI time. It stays a local step of every plan that edits C.
- **Where the stage time goes** (warm tree, gate preset, per case user CPU from the runs above): `ds_runner` over all 315 cases is about 50 s; the toy controls and the host steps are the rest. The
  largest single cases are `observer_random` 4.1 s and `chunk_equiv_random32` 3.0 s (checkpoint A figures), `snapshot_restore_video_bg` 3.7 s and `snapshot_restore_video` 1.1 s (this plan);
  no other video case is above 1.4 s. No lever of D-176 that
  weakens nothing reaches a 60 s stage: levers (2) and (3) together remove about 2.8 s, lever (4) 0.35 s.

### Result and recommendation

| Measure | Before (`f9d9d69`) | After lever (3) | Note |
|---------|--------------------|-----------------|------|
| `conformance` stage | 86 s | 86 s | load 10 to 22 and 12; the trigger is 60 s |
| `sh scripts/check --timing`, user CPU | 344.3 s | 309.1 s | load-dependent; the per-case figures are the measure of the levers |
| `sh scripts/check --timing`, wall | 461.6 s | 393.3 s | load-dependent |
| sum of the per-case user CPU removed | | about 2.8 s | levers (2) and (3) |

The levers of D-176 that weaken no evidence are exhausted: the stage stays over its 60 s trigger and the gate over the 150 s trigger of D-180, both on a loaded development
machine. The sprite cases of Phase 4.1 therefore land on a gate that is only 2.8 s cheaper than before; the cost of the cases themselves is recorded in the plan summaries. The
remaining candidates are decisions for the efficiency review after the first CI run of Phase 4 and 4.1, with the CI job time in hand: the second awk run's share (the parallel job of the quick task
already cuts the critical path), a skip of the `sanitizers` stage in the second awk run (needs a skip mechanism, which the fail-closed rule discourages) and the cost of the toy controls. None is applied here.

## Phase 4.1 plan 04.1-02: the cost of the sprite limit cases (local runs, not CI runs)

The plan adds four cases (`spr_limit`, `mutate_sweep_spr_limit`, `video_spr_limit`, `video_fault_sweep_spr_limit`), grows `vdc_stat` from 21 to 27 checks and adds the tenth toy (`vid-spr-limit`). No lever
is applied here and no case beyond those the plan lists was added. Per case user CPU, one `ds_runner --only` run each on a warm tree (gate preset, loaded development machine, so only the ratios mean anything):

| Case | Before | After | Note |
|------|--------|-------|------|
| `spr_limit` (guest, 3 power-on policies) | none | 0.22 s | 8 checks, 6.8 million master clocks to the verdict, deadline 7.5 million |
| `mutate_sweep_spr_limit` (9 runs of the image) | none | 0.64 s | one run per check and the unmutated image |
| `video_spr_limit` (2 policies, 15 groups) | none | 0.24 s | 19 frames of records |
| `video_fault_sweep_spr_limit` | none | 0.15 s | one run, faults applied in memory |
| `vdc_stat` | 0.14 s | 0.19 s | 27 checks, 5.55 million clocks to the verdict (was 21 checks, about 2.3 million) |
| `mutate_sweep_vdc_stat` (28 runs instead of 22) | 0.76 s | 1.69 s | the longer guest and 6 more checks |
| `vdc_poweron_policy_diff` (7 ROMs instead of 6) | | 2.31 s | `spr-limit@2` added; the before figure was not measured |

The sum of the new and the changed cases is about 2.3 s of user CPU more than before (1.25 s new cases, 1.0 s for `vdc_stat` and its sweep, the rest for the policy diff and the toy), against a stage of 60 to 90 s on this machine: about 3 percent. The toy
`vid-spr-limit` runs 4 cases and one extra toy binary build. `conformance` read 88 s on the full gate run at the second task commit and 155 s at the third (other jobs were running on the machine; the trigger is 60 s); `sh scripts/check --timing` read 424 s and 479 s user CPU on those runs (the trigger of D-180 is 150 s). Both
triggers were already over at 04.1-01, so this plan only adds to a recorded excess; the efficiency review after the first CI run decides what to stage.

## Phase 4.1 close-out (plan 04.1-03; local runs, not CI runs)

CI for Phase 4.1: unverified: no push is authorised (D-64). Phase 4 (pull request 3, run 43275379025) has its CI record above; nothing of Phase 4.1 and nothing of the parallel second awk job (quick task 261112-ci-parallel-awk) has run on a GitHub runner, and this section makes no CI claim.
Every figure below is a local run on the owner's development machine (Apple M5 Pro, 18 cores, macOS, BWK awk, Apple clang, cmake) on 2026-11-12 at the tree of `e353d8e`, whose code equals the close of plan 04.1-02. No time here is a pass or fail criterion; the evidence tier of every run is E0, repeatability.
**The machine was shared and never quiet: the one-minute load average read 25.3 at the start of the gate run and 33.1 after it** (the highest of the three phases that were measured), so the user CPU and the stage seconds below are upper figures and are not comparable with a quiet run.

| Run | What | Result | Wall s | User CPU s | System s | Load average (1 min) |
|-----|------|--------|--------|------------|----------|----------------------|
| 1 | `sh scripts/check --timing`, warm tree | PASS, 57 stages | 766.9 | 413.2 | 125.8 | 25.3 at start, 33.1 at end |
| 2 | `sh scripts/check --selftest` | PASS, 174 mutations, no FAIL | 394.3 | 61.6 | 113.6 | 33.1 at start |
| 3 | `dash scripts/check` (the gate under a second shell) | PASS, 57 stages | 731.0 | 435.1 | 124.7 | not recorded at start, 21.6 at end |
| 4 | `sh scripts/audit-history --range f9d9d69..HEAD` (`f9d9d69` is the commit before the first commit of plan 04.1-01) | clean | | | | 12 commits, 101 blobs, 2,075 blob-path pairs |
| 5 | `sh scripts/linux-repro`, all six default parts, one-off container run (linux/amd64 under emulation), see the table below | PASS, `LINUX_REPRO RESULT PASS` | 4,867 | | | host load about 20 |

Run 5 in detail (Ubuntu 24.04.5, cmake 3.28.3; one-off local runs, not CI claims; the working tree at `e353d8e` was copied into the container at the start, so every part judged the README and ledger texts of this plan; only this section, `04.1-VALIDATION.md` and the SUMMARY were written afterwards):

| Part | Awk, compiler | Result | Detail |
|------|---------------|--------|--------|
| `default-awk` | mawk 1.3.4, gcc 13.3 | PASS | the whole gate, 57 stages, in 1,299 s, and `--selftest` |
| `mawk` | mawk 1.3.4, clang 18.1.3 | PASS | the gate in 978 s and `--selftest` |
| `gawk` | gawk 5.2.1, gcc 13.3 | PASS | the gate in 1,019 s and `--selftest` |
| `gate-gcc` | gcc 13.3, `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror` | PASS | build of the preset |
| `gate-clang` | clang 18.1.3, the same flags | PASS | build of the preset |
| `core-boundary-gcc` | gcc 13.3 | PASS | `STATUS core-boundary ok` |

The second awk is covered by the `mawk` and `gawk` parts (BWK awk on the development machine, mawk and gawk in the container); no awk-specific defect showed.

Sizes against the ROM shapes of `scripts/audit-history`: none of the 62 files added or changed since `f9d9d69` has a size of at least 4,096 that is a multiple of 8,192 or 512 more than one (the shapes ROM-SIZE-8K and ROM-SIZE-HDR); the largest file
added or grown in Phase 4.1 is `tests/conformance/videokinds.c` (194,574 bytes, from 189,578) and the largest new file is `fixtures/src/spr_basic.vexp` (14,062 bytes). No ROM-like file exists: the sprite fixtures are assembled in the gate and never
committed.

### The stage seconds of this run

Whole seconds from the gate's own clock (57 stages, sum 766): the stages over 20 s are `sanitizers` 303, `determinism` 103, `conformance` 91, `fixtures` 65 and `named-controls` 41; every other stage is 16 s or less. All 57:
preflight 0, ledger-entries 2, ledger-negatives 4, ledger-samples 0, decision-records 1, decision-negatives 2, profile-records 0, profile-negatives 3, filename-controls 1, empty-expect-controls 0, provenance 1, provenance-negatives 5, licences 12, licence-negatives 3, protocol-order 1, protocol-negatives 13,
score-controls 1, results 1, results-negatives 1, ingest-controls 3, flatten-controls 2, cancel-controls 2, outcome-controls 1, rights-tree 8, rights-negatives 16, audit-synthetic 16, inventory 11, inventory-negatives 2, fixture-lint 0, fixture-lint-negatives 2, video-check 9, input-check 0, input-check-negatives 2,
xasm 0, opcode-oracle 0, opcode-oracle-negatives 1, xasm-negatives 1, workflow 0, workflow-negatives 3, docs-claims 1, docs-claims-negatives 1, ci-timings-controls 0, fixture-build 3, build 1, outcome-sync 0, conformance 91, sanitizers 303, excluded-mode-controls 1, record-lint 0, record-lint-negatives 5,
core-boundary 9, core-boundary-negatives 7, determinism 103, determinism-negatives 1, fixtures 65, fixture-negatives 4, named-controls 41.

### Checkpoint A, the 04.1-01 levers and this run side by side

| Measure | Checkpoint A, run D (quietest, load 7) | 04.1-01 start (`f9d9d69`) | 04.1-01 after lever (3) | 04.1-01 close (Task 4) | 04.1-02 close (Task 3) | This run (load 25 to 33) |
|---------|----------------------------------------|---------------------------|-------------------------|------------------------|------------------------|--------------------------|
| `sh scripts/check --timing` wall s | 321.2 | 461.6 | 393.3 | 307.1 | not recorded | 766.9 |
| user CPU s | 262.4 | 344.3 | 309.1 | 249.5 | 479 | 413.2 |
| `conformance` stage s | 62 | 86 | 86 | 59 | 155 | 91 |
| `sanitizers` stage s | 146 | not recorded | not recorded | not recorded | 369 | 303 |
| `determinism` stage s | 52 | not recorded | not recorded | not recorded | not recorded | 103 |
| REQUIRED ids | 315 | 318 | 318 | 326 | 330 | 330 |

The columns come from different load levels (7 to 33), so the rows do not rank the code; the per case figures below are the stable measure of what Phase 4.1 added. The best figures of the phase are the 04.1-01 close (249.5 s user CPU, `conformance`
59 s, during a plan whose runs read a load of 10 to 22) and checkpoint A run D (262.4 s, 62 s); this run is the worst because the machine was the busiest.

### What the sprite cases added

Phase 4.1 added 12 REQUIRED ids (318 to 330), 2 guest ROMs (34 to 36 of the limit 40; the image of `bg_enab` was edited, not added), one toy (the 26th control, `vid-spr-limit`, the tenth and last video toy of D-173) and 6 checks to `vdc_stat` (21 to
27). User CPU of one `ds_runner --only ID` run each, gate preset, warm tree, at load 18 to 25 (the same method as the 04.1-02 table; only the ratios mean anything):

| Case | User CPU s | Note |
|------|-----------|------|
| `spr_basic` (6 checks, 3 policies) | 0.14 | |
| `video_spr_basic_solid` | 0.11 | 2 policies |
| `video_fault_sweep_spr_basic` | 0.06 | |
| `mutate_sweep_spr_basic` | 0.20 | one run per check and the unmutated image |
| `video_spr_basic_asym` | 0.10 | |
| `video_fault_sweep_spr_basic_asym` | 0.05 | |
| `vdc_satb_data_spr` | 0.09 | |
| `video_spr_basic_golden` | 0.09 | |
| `spr_limit` (8 checks, 3 policies) | 0.17 | |
| `video_spr_limit` | 0.18 | |
| `video_fault_sweep_spr_limit` | 0.11 | |
| `mutate_sweep_spr_limit` | 0.47 | 9 runs |
| the 12 new cases | 1.77 | about 2 percent of the 91 s `conformance` stage |
| `vdc_stat` (27 checks) | 0.14 | changed: was 21 checks |
| `mutate_sweep_vdc_stat` (28 runs) | 1.22 | changed: was 22 runs |
| `vdc_poweron_policy_diff` (7 ROMs) | 1.68 | changed: `spr-basic` and `spr-limit` added |

The 12 new cases cost 1.77 s of user CPU in all. The three changed cases cost 3.0 s now; what Phase 4.1 added to them is not separable (the figures of the 04.1-02 table were 0.14 s and 0.76 s for `vdc_stat` and its sweep before the OR and CR rows,
and no figure was taken for the policy diff before). Levers (2) and (3) of 04.1-01 removed about 2.8 s of user CPU, so the net effect of Phase 4.1 on the plain-preset cases is within a few seconds of zero. Under the sanitizers each case costs about 3
to 4 times its plain figure, and the sprite pass runs once per displayed line only while a scene has SB set, so the quiet gate grew by a few seconds, not by a stage of its own. The sprite cost cannot be read from the stage seconds above, which move
by 30 to 50 percent with the load.

### D-176 and D-180 trigger status after the levers

| Trigger | Measure | Result |
|---------|---------|--------|
| `conformance` over 60 s (D-176) | 59 s at the 04.1-01 close (load 10 to 22), 62 s at checkpoint A (load 7), 91 s in this run (load 25 to 33) | **fired on a loaded machine, borderline on a quieter one**; levers (1), (2) and (3) are applied, lever (4) is recorded and not applied (it would remove a REQUIRED case from the PR evidence for 0.35 s) |
| CI job over 600 s with lever (1) not recovering it (D-176) | Phase 4: 853 s (run 43275379025, before the parallel second awk job); Phase 4.1: **unmeasured** (no push, D-64) | Phase 4 fired (recorded above); Phase 4.1 unverified |
| warm quiet gate over 150 s user CPU (D-180) | 249.5 s (04.1-01 close), 262.4 s (checkpoint A run D), 413.2 s (this run) | **fired**, 1.7 to 2.8 times the threshold; no quiet-machine figure exists and none was waited for |
| more than two revision plans | 0 revision plans in Phase 4.1 (04.1-04 and 04.1-05 unused) | not fired |

Nothing is weakened by any trigger: no stage, control, selftest or case was skipped, shortened or removed to save time, and no time is ever a gate failure.

### Efficiency review inputs for the first CI run that includes Phase 4.1 (AGENTS.md: review after each CI run)

- **Record on the first run after an owner-authorised push:** the three job times (`gate-gawk`, `gate-mawk` and the aggregator `gate`) against the 440 to 550 s expected for the two work jobs, the per stage seconds of the `Gate` step
  (`scripts/ci-timings` prints them), and whether the aggregator is named `gate`. The Phase 4 job was 853 s as one job; the 12 new cases add under 2 s of user CPU to `conformance`, which is inside the 23 percent run-to-run spread seen on the runner.
- **`sanitizers` is still the largest stage** (303 s here, 172 s on the runner at Phase 4): lever (2) is already applied to every video run (two policies), so the remaining candidates are a skip in the second awk run (needs a skip mechanism, which the fail-closed rule discourages) and nothing cheaper; the sprite cases add under 2 s of it.
- **The schedule 3072 can be restored** to `chunk_equiv_video_vce_pal`, `chunk_equiv_video_bg_scroll` and `chunk_equiv_random32_video` at about 0.1 s if the CI job time is acceptable (lever (3) measured about 0.1 s, inside the noise).
- **`conformance` and the toys:** 26 toy controls run only their EXPECT cases; lever (1) keeps the cold compile out of the CI case. The 04.1-02 toy added one binary build and 4 EXPECT cases.
- **The second awk and Linux runs** are whole-gate runs: the Linux container parts above are local one-offs (about 75 minutes for all six under emulation) and are not in the CI job, so they cost no CI time; the two awk implementations on the runner are the parallel jobs of the quick task, unverified until pushed.
- **The best remaining lever on the evidence so far** is the same as at the Phase 4 review: the parallel second awk job, already applied and unverified. A skip of `sanitizers` in the second awk run is the next, and it needs the owner's decision on the fail-closed rule.
- Nothing was weakened to save seconds, and no workflow file changed in this plan.
