---
status: owner-recorded
owner-statement: Owner selected "GitHub Actions" in the /gsd-discuss-phase 1 session, 2026-11-09 (CONTEXT.md D-03)
owner-date: 2026-11-09
research-ref: SUMMARY #14
reopen-if: the repository leaves GitHub, or a CI job needs a secret or an elevated permission
---

# 0006: CI provider

## Context and gate

The fast PR path must stay fast, and the gate must be runnable locally with one command. CI has to be a thin layer over that command so the provider can change without changing what the gate checks. The repository will be public (record 0004), which makes workflow security a first-order concern.

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. GitHub Actions | A thin workflow wrapper around the provider-neutral `scripts/check`. | Owner decision, recorded in CONTEXT.md D-03 |
| B. Another provider | Not selected. | The owner chose option A |

## Decision

The owner selected option A (see `owner-statement`).

- The workflow YAML is a thin wrapper around `scripts/check`. The gate logic stays in the script.
- Actions are pinned by full commit SHA, with `permissions: contents: read` by default.
- No `pull_request_target`, `persist-credentials: false`, and no secrets in PR jobs.
- Proposed matrix, confirmed by measured value in Phase 2: the PR path runs on Linux x86_64, which differs from local macOS; scheduled jobs run on macOS aarch64 and Windows.
- Public-repository minutes are free, so cost is not the constraint. A job earns its place by catching a distinct failure class.

The workflow file itself is Phase 2 work (CI-01). This record only fixes the provider and the security posture.

## Consequences and what is not decided here

- Not decided here: the workflow file, job names and caching. The matrix above is a proposal that Phase 2 confirms or changes by measured value.
- Not decided here: fuzzing, Miri and wasm jobs, which are scheduled and staged by measured value.
- Any job that needs a secret or a permission above `contents: read` reopens this record.
- The pull-request template with the copied-code checklist line (record 0007) arrives with the Phase 2 workflow.
