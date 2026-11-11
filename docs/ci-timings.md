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
- Review the gate job's `timeout-minutes: 10` limit (D-61) against the first recorded critical path. The second gate run and the two compiler runs
  add to the job, so the limit may be too tight or needlessly loose.
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
