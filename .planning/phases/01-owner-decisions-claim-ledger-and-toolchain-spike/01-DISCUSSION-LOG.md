# Phase 1: Owner Decisions, Claim Ledger and Toolchain Spike - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md. This log preserves the alternatives that were considered.

**Date:** 2026-11-09
**Phase:** 01-owner-decisions-claim-ledger-and-toolchain-spike
**Areas discussed:** Spike protocol, Claim ledger, Decision records + provenance rule, First-profile record, Owner-gate values

**Mode:** Advisor mode with `minimal_decisive` calibration. Following the owner's fan-out instruction, all gray areas were auto-selected. Five parallel research agents covered them, one area each, looking through the architect, security, DevOps, legal, test and emulator lenses, each with an adversarial pass. Claude combined their findings and adopted the recommendations for the implementation areas. The owner was asked only about the authority-gated values.

---

## Synthesis corrections (Claude's challenge pass)

- The ledger agent proposed flat TOML and the profile agent proposed restricted TOML with tables. These were unified into one strict subset (D-18).
- The profile proposal cited research IDs C1-C21. These were replaced with stable LED IDs, with the research ID kept in `origin`.
- The decision-records agent proposed checking that the owner authored the commit. This was dropped because the agent commits under the owner's git identity, so the check would prove nothing. The verbatim owner statement plus its source is the attestation instead.
- The spike agent assumed a human author with uneven familiarity in the two languages. In practice an AI executor writes both skeletons, so lines of glue were reweighted as review and maintenance surface.
- The spike agent proposed a separate branch. The project uses `branching_strategy: none`, so the spike is committed on `main`, tagged, and then removed from `main` (D-16).

## Licence (decision 8a)

| Option | Description | Selected |
|--------|-------------|----------|
| MIT OR Apache-2.0 + 0BSD (Recommended) | Code under a dual licence; fixture sources and ROMs under 0BSD | ✓ |
| GPL-3.0-or-later | Copyleft; could absorb GPL emulator code and tests | |
| Leave open | Phase 3 fixture admission stays blocked | |

**User's choice:** MIT OR Apache-2.0 + 0BSD

## Visibility (decision 8b)

| Option | Description | Selected |
|--------|-------------|----------|
| Private until milestone gate (Recommended) | Publish later, after a history audit | |
| Public now | Create a public remote now | ✓ |
| Leave open | No remote | |

**User's choice:** Public now. The plan must audit the history and pause at a checkpoint before the first push.

## CI provider (decision 14)

| Option | Description | Selected |
|--------|-------------|----------|
| GitHub Actions (Recommended) | A thin wrapper around `scripts/check`, with actions pinned by SHA | ✓ |
| Forgejo/Codeberg CI | Self-hostable, but no hosted macOS or Windows runners | |
| Leave open | CI-01 is blocked | |

**User's choice:** GitHub Actions

## Language pre-authorization (decision 1)

| Option | Description | Selected |
|--------|-------------|----------|
| Pre-authorize clear winner (Recommended) | Adopt the winner if it passes all gates and leads by 10 points or more | ✓ |
| I'll confirm after the spike | The record stays proposed-default and Phase 2 is blocked | |

**User's choice:** Pre-authorize clear winner

## Fixture rights (decision 9)

| Option | Description | Selected |
|--------|-------------|----------|
| Tiered, authored default (Recommended) | Third-party fixtures only by recorded exception; never GPL or unclear material in the repo | ✓ |
| Authored-only, strict | No exceptions | |
| Leave open | Phase 3 is blocked | |

**User's choice:** Tiered, authored default

## API/ABI posture (decision 6)

| Option | Description | Selected |
|--------|-------------|----------|
| No freeze, 0.x (Recommended) | Internal API first; header only after a consumer proof | ✓ |
| Leave open | Blocks nothing until Phase 6 | |

**User's choice:** No freeze, 0.x

## First-profile scope (decision 2)

| Option | Description | Selected |
|--------|-------------|----------|
| Select proposed scope (Recommended) | HuC6280, 2-button pad, HuCard only, no expansion; disputed fields stay open | ✓ |
| Leave all open | Every field must be bound per run | |

**User's choice:** Select proposed scope

## Implementation areas adopted from research (no owner question; per the owner's instruction)

- **Spike protocol:** a protocol committed before the spike; an identical workload; hard gates, then weighted scores with a tie band and a margin rule; throwaway skeletons preserved by a tag. Alternatives considered: promoting the spike as the Phase 2 seed (rejected: sunk-cost bias); pure owner judgement (rejected: post-hoc rationalisation); a fully automatic weighted sum (rejected: overrides the owner's authority).
- **Claim ledger:** one strict-TOML file per entry under `ledger/`, LED IDs, a POSIX sh+awk checker, and a negative corpus with expected reason codes. Alternatives considered: a single TOML file (merge conflicts), JSON (no comments), YAML (implicit typing), a Markdown table (fragile), Python `tomllib` (system python3 too old, adds a runtime), deferring the check to Phase 2 (fails success criterion 3).
- **Decision records:** `docs/decisions/` with trimmed MADR, a closed status enum, and a verbatim owner statement. Alternatives considered: full MADR (ceremony), Nygard (too thin), `.planning/decisions/` (tied to the workflow tool).
- **Provenance:** in-file SPDX tags, a central `PROVENANCE.md` register and a gate check. Alternatives considered: full REUSE now (adds a tool; deferred), header-only or register-only (rot).
- **Profile record:** restricted TOML with frozen numbered revisions, observed-behaviour field names, open fields without values, and no defaults in code. Alternatives considered: a full TOML crate (adds a dependency), Rust constants only (the owner cannot edit status), a documentation file mirrored by code (drift), a slug that encodes region and revision (asserts disputed facts).

## Claude's Discretion

- LED numbering, the wording of assumptions, and reason-code names.
- How the checks are split into scripts.
- The hyperfine command lines, the fixture generator, and the C presets.

## Deferred Ideas

- Full REUSE adoption.
- Signed commits as owner attestation.
- An owner decision on decision 17.
- Remaining non-blocking owner gates: 7, 10, 11, 12, 13, 15, 16, 18.
