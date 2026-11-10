<!-- SPDX-License-Identifier: MIT OR Apache-2.0 -->
<!-- SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors -->
# CI timings

This file records how long the CI definition (`.github/workflows/gate.yml`, record 0006) takes on a real run, and what to do with the numbers.
A row is produced by `sh scripts/ci-timings RUN_ID` (a read-only `gh run view`) and pasted here in a normal commit. No bot commits this file and
no timing artifact is uploaded (D-63). The tool also reads a canned file (`sh scripts/ci-timings --tsv FILE`) so that its output is tested offline
by the `ci-timings-controls` stage of `scripts/check`.

## Status

No CI run exists yet. Pushing a branch to the public remote is an owner decision (D-64) and has not been authorised, so the workflow has only been
judged statically by `scripts/workflow-check` and its controls. The Phase 2 success criterion that needs a green CI run is therefore
unverified-pending-push, not satisfied. The table below stays empty until a real run is recorded: a row built from a canned file or a local run
would be a claim about CI that nothing has earned.

A local one-off run of the gate in an Ubuntu 24.04 container is a useful dry run of failure classes 1, 3 and 4 below. It is not a CI run, it has no
row in this table, and it does not change the status above.

## Timing table

Columns: date (UTC), run id, queue seconds (run start minus run creation), seconds per job, run wall seconds (the latest job end minus the run
start, which is the critical path), and the run conclusion.

| Date (UTC) | Run id | Queue s | Job s | Run wall s (critical path) | Notes |
|------------|--------|---------|-------|----------------------------|-------|

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

Each item is a setting on the remote repository. It is applied only with the push checkpoint's authorisation (D-80); until then it stays here as a
pending hand-off, and nothing in the repository applies it.

1. Actions default workflow permissions set to read-only.
2. Actions allowed to approve pull requests: off.
3. A branch ruleset on `main` that requires the `gate` check (owner gate, D-64).

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
