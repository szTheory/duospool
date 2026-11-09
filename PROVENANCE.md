# Copied-code provenance register

This file records every piece of code copied from another project into DuoSpool. The project prefers a small, flat
dependency tree, and a small copied helper with recorded provenance can beat a new dependency (CONTEXT.md D-08).
A copy carries licence and maintenance obligations that a dependency hides, so each copy is marked in the file and
registered here. The principle and the mechanism are in docs/decisions/0007-copied-code-provenance.md.

PR checklist: No code copied from another project without a Provenance marker and a PROVENANCE.md entry.

That line moves into the pull-request template when the Phase 2 CI workflow arrives (CI-01, record 0006); until then this file and record 0007 carry it.

Each entry is one block of `key: value` lines, separated from the next by a blank line. `scripts/provenance-check`
reads only the text after the `## Entries` line. Emulator code from other projects is read-only reference and is never copied.

## Entries
