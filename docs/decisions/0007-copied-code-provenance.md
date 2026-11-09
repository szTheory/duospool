---
status: owner-recorded
owner-statement: Owner said, in the /gsd-discuss-phase 1 session, 2026-11-09 (CONTEXT.md D-08): "another copy and paste is better than another dep if we truly need a dep that's fine but it's better to have smaller, flatter dependency trees"
owner-date: 2026-11-09
research-ref: AGENTS.md dependency preference; SUMMARY rights gates; PITFALLS M6
reopen-if: the owner changes the dependency principle, or the allowlist needs a licence that is not on it
---

# 0007: Copied-code provenance

## Context and gate

The project prefers a small, flat dependency tree. A small copied helper with recorded provenance can beat a new dependency, but a copy carries licence and maintenance obligations that a dependency hides. This record states the principle and the mechanism that keeps a copy accountable.

Only the principle is owner-recorded. The mechanism is Claude's design under it and stays open to owner change.

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. Copy over dependency, with provenance | A copy is acceptable when it is marked and registered. A dependency is acceptable when truly needed. | Owner principle, recorded in CONTEXT.md D-08 |
| B. Dependencies only | Never copy code. | Not selected |
| C. Copy without records | Copy freely. | Not selected. It loses the licence trail |

## Decision

The owner's principle, in the owner's words: "another copy and paste is better than another dep if we truly need a dep that's fine but it's better to have smaller, flatter dependency trees".

This principle is owner-recorded (CONTEXT.md D-08). It is the only owner-recorded part of this record.

### Mechanism (Claude's design under the principle; not owner-recorded)

- In-file markers on every copied file or block: `SPDX-License-Identifier`, `SPDX-FileCopyrightText` and `Provenance: PROV-NNNN`.
- A central `PROVENANCE.md` register with one `key: value` block per copy. Fields: `id`, `source-url`, `upstream-ref` (full SHA or tag), `upstream-path` and lines, `license-spdx`, `license-url` (pinned), `upstream-notice` (verbatim), `local-paths`, `modifications`, `copied-on`, `copied-by`, `maintenance-owner`, `why-copy-not-dependency`, `sync-policy` (`frozen`, `diverged` or `tracked`) and `licence-decision`.
- SPDX allowlist for copies: MIT, Apache-2.0, BSD-2-Clause, BSD-3-Clause, Zlib, ISC, 0BSD, CC0-1.0. GPL, unclear licences and any `WITH` exception are excluded.
- Gate check: `scripts/provenance-check` (plan 01-04) verifies that every marker resolves, every entry's paths exist and carry the marker, required fields are present and the SPDX value is on the allowlist.
- Staleness is a warning only.
- Emulator code from other projects is read-only reference. It is never copied.
- Unmarked copies are covered by a pull-request checklist line, stated verbatim here:

PR checklist: No code copied from another project without a Provenance marker and a PROVENANCE.md entry.

That line lives in this record now, and in `PROVENANCE.md` when that register ships (plan 01-04). It moves into the pull-request template that arrives with the Phase 2 CI workflow (CI-01, record 0006). Phase 1 has no remote and no pull-request template yet.

## Consequences and what is not decided here

- Every new dependency still needs a written justification and a licence and maintenance record (AGENTS.md).
- Not decided here: any specific copy. Each one needs its own `PROVENANCE.md` entry and a licence decision.
- Not decided here: the exact enforcement of the checklist line, which waits for a remote and a pull-request template.
- The mechanism may be changed by a later record without reopening the owner's principle.
