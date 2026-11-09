---
status: owner-recorded
owner-statement: Owner selected "No freeze, 0.x" in the /gsd-discuss-phase 1 session, 2026-11-09 (CONTEXT.md D-05)
owner-date: 2026-11-09
research-ref: SUMMARY #6
reopen-if: a release decision is made, or a consumer proves that a frozen API or ABI is needed
---

# 0003: Core API and ABI posture

## Context and gate

The core needs a stated API and ABI posture before any host or consumer binds against it. A C-compatible boundary is a candidate (STACK.md), and the sibling LibFuturo interface is not frozen. Freezing a shape before a real consumer proves it would lock in whatever the first code happened to look like.

This record fixes the posture only. It does not choose the language (record 0001, plan 01-11) and does not design the API.

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. No freeze, 0.x | Any commit may break the API, ABI, state layout or manifest format. A hand-written header comes only after a consumer proves the shape. | Owner decision, recorded in CONTEXT.md D-05 |
| B. Freeze an ABI early | Publish a stable C header at the first slice. | Not selected |
| C. Generate the header (cbindgen) as the contract | The header follows the code. | Not selected. STACK.md warns that a generated header freezes whatever the code looks like |

## Decision

The owner selected option A (see `owner-statement`).

- The internal advance API comes first.
- A hand-written C header comes only after a consumer proves the shape. cbindgen is never the contract.
- 0.x means any commit may break the API, ABI, state layout or manifest format.
- The installed-consumer proof pins an exact version.
- Once a header exists it exports `duospool_abi_version()`, and the value must match exactly.
- Nothing is published to crates.io or any package registry before a release decision.
- A libretro wrapper is a host, not part of the core API.

The TOOL-01 spike header is throwaway evidence for the language decision. It is not this header and does not start the freeze clock.

## Consequences and what is not decided here

- Hosts and consumers must pin an exact version and expect breaking changes between any two commits.
- Not decided here: the language and build system (record 0001), the shape of the first real header, package format, and the release policy.
- Not decided here: whether LibFuturo is adopted as the C interface. That stays a prototype-adapter question.
- Changing this posture needs a new record that supersedes this one.
