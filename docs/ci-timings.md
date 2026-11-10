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
