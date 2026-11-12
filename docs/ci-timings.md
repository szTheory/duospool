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
- Review the gate job's `timeout-minutes` limit (D-61; 10 until plan 03-33, 20 since) against each recorded critical path. The second gate run and
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
