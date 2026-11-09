---
status: owner-recorded
owner-statement: Owner selected "MIT OR Apache-2.0 + 0BSD" (licence), "Public now" (visibility) and "The DuoSpool Authors" (copyright holder) in the /gsd-discuss-phase 1 session, 2026-11-09 (CONTEXT.md D-01, D-02, D-31)
owner-date: 2026-11-09
research-ref: SUMMARY #8
reopen-if: the owner proposes relicensing (needs every contributor's consent once outside contributions exist)
---

# 0004: Licence and repository visibility

## Context and gate

DuoSpool needs a licence for code, a licence for authored fixtures, a named copyright holder and a visibility decision before any file is published. Licensing and repository visibility are owner-authority items in the project constraints. The `DuoSpool` name is a working name chosen by the owner and is not trademark-cleared.

Both choices are one-way. Relicensing later needs every contributor's consent once outside contributions exist, and published history cannot be unpublished.

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. MIT OR Apache-2.0 for code, 0BSD for authored fixtures | Dual permissive licence for code, public-domain-like terms for fixture sources and the ROMs built from them. | Owner decision, recorded in CONTEXT.md D-01 |
| B. Public now | The repository becomes public. No remote exists at the time of this record. | Owner decision, recorded in CONTEXT.md D-02 |
| C. Copyright holder "The DuoSpool Authors" | Used in `LICENSE-MIT`, `LICENSE-0BSD` and the SPDX copyright headers. | Owner decision, recorded in CONTEXT.md D-31 |
| D. Any other licence, or staying private | Not selected. | The owner chose the options above |

## Decision

The owner selected "MIT OR Apache-2.0 + 0BSD" (licence), "Public now" (visibility) and "The DuoSpool Authors" (copyright holder), in the `/gsd-discuss-phase 1` session, 2026-11-09.

- Code is `MIT OR Apache-2.0`. Authored fixture sources and the ROMs built from them are `0BSD`, declared per directory.
- The licence files are `LICENSE-MIT`, `LICENSE-APACHE` and `LICENSE-0BSD`, plus SPDX header lines on new source files. The holder line is `Copyright (c) 2026 The DuoSpool Authors`.
- GPL emulators (Mednafen, Geargrafx) are read-only reference. No GPL code is absorbed.
- Contributions are inbound=outbound under the same dual licence, in the Apache-2.0-defined sense.
- Visibility is public. The first push comes only after the full-history pre-publication audit (`scripts/audit-history` plus gitleaks) and an owner checkpoint immediately before the remote is created (CONTEXT.md D-02, D-33). The main branch is pushed first, then tags.
- The DuoSpool name is a working name and is not trademark-cleared.

## Consequences and what is not decided here

- Both choices are one-way. Changing the code licence needs every contributor's consent once outside contributions exist. Published history cannot be unpublished.
- Not decided here: release policy (roadmap decision 18) and package publication (record 0003).
- Not decided here: trademark clearance of the `DuoSpool` name, which stays unresolved.
- Creating the public remote is an outward-facing action. It happens only after the audit and the owner checkpoint, and this record does not authorise it by itself.
- Third-party fixtures and code copied from other projects are governed by records 0005 and 0007, not by this record.
