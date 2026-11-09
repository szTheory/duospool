---
status: owner-recorded
owner-statement: Owner selected "Tiered, authored default" in the /gsd-discuss-phase 1 session, 2026-11-09 (CONTEXT.md D-04)
owner-date: 2026-11-09
research-ref: SUMMARY #9
reopen-if: the owner admits a specific third-party fixture, or changes the authored-default tier
---

# 0005: Fixture rights

## Context and gate

Fixtures are test content that enters the repository and public CI. Game images, firmware and disc images are a rights and security boundary, and a matching hash or a public mirror does not resolve rights. The project constraints require recorded provenance and redistribution terms for every admitted fixture.

Several existing PC Engine test programs have unclear or copyleft licences or contain generated startup code with unclear status (STACK.md section 7).

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. Tiered, authored default | Authored fixtures are the default. A third-party fixture needs a recorded exception. | Owner decision, recorded in CONTEXT.md D-04 |
| B. Admit existing community test programs | Use programs such as the sound test or the 240p suite directly. | Not selected. Their licences or generated parts are copyleft or unclear (STACK.md) |

## Decision

The owner selected option A (see `owner-statement`).

- Authored fixtures are the default.
- A third-party fixture is admitted only through a recorded exception. The exception lists SPDX, source URL and commit, every generated part, hash, redistribution terms and owner sign-off.
- GPL or unclear-licence material is never committed. At most it is fetched by pinned hash into an uncommitted local cache.
- Explicitly not admitted: PCE_Soundtest (it contains HuC-generated startup code), any HuC/PCEAS runtime, startup code or headers, and the 240p Test Suite in the repository.

No GPL or unclear-licence material is admitted without a further owner decision.

## Consequences and what is not decided here

- The fast suite relies on fixtures the project authors itself, under the 0BSD licence recorded in 0004.
- Not decided here: the first homebrew or game fixture beyond the authored diagnostics, and any specific exception. Each needs its own recorded exception and owner sign-off.
- Not decided here: how a locally fetched, uncommitted cache is organised, if one is ever used.
- `decision-check` rejects this record if the no-GPL sentence is removed or reworded.
