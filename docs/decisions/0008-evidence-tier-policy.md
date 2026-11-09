---
status: proposed-default
blocks: no phase while the default is in force; an owner decision is needed to change the evidence policy
default-in-force: E4 never backs a correctness claim; an E1 item needs a cited FACT source; E0 proves repeatability only
research-ref: SUMMARY #17
---

# 0008: Evidence-tier policy

## Context and gate

Every ledger entry that reaches `supported` must say what kind of evidence backs it. The tiers are E0 (repeatability only), E1 (documented vector or register behaviour asserted literally), E2 (authored diagnostic ROM with an independent verdict), E3 (hardware measurement, owner gate) and E4 (differential against another emulator). The research summary (decision 17) proposes that E4 never backs a correctness claim, and records the disagreement between its source files as conflict C12.

Phase 1 success criterion 3 requires `scripts/ledger-check` to reject a sample marked supported on E0 and E4 evidence only. The checker therefore enforces the proposed default structurally, whether or not the owner has confirmed it. The owner is not asked in this session (CONTEXT D-29 and the deferred ideas).

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. E4 is triage only | Emulator-to-emulator agreement finds disagreements and decides nothing. Documented vectors (E1) are valid evidence for the behaviour they cite. | REPORT: FEATURES and ARCHITECTURE research files; project policy in AGENTS.md (emulator agreement is not correctness) |
| B. PITFALLS C1 reading | Only the last three tiers (differential, authored diagnostic, hardware) can back a correctness claim. That would admit emulator differential and exclude documented vectors. | REPORT: PITFALLS research file; conflict C12 in the research summary |

Option A is the stricter reading and is the only one consistent with AGENTS.md. Option B would let two emulators that share a wrong assumption certify each other.

## Decision

Proposed default (not owner-recorded): option A.

- E0 proves repeatability only and never counts toward `supported`.
- E4 never backs a correctness claim and never counts toward `supported`, alone or combined with E0.
- A `supported` entry needs at least one E1, E2 or E3 item.
- An E1 item needs a cited FACT source on the entry.
- An E3 item must cite an owner-gate decision record.

`scripts/ledger-check` enforces these rules structurally and the gate runs negative controls that prove it (`ledger/negative/e0-e4/` and its siblings). The check enforces structure, not truth.

## Consequences and what is not decided here

- The owner has not confirmed this policy. Changing it needs an owner decision and a new record that supersedes this one.
- This record does not decide whether a documented vector may back a claim without a second source, or how E3 hardware measurements are set up and bounded.
- Later phases that add a ledger entry must follow these rules. A claim backed only by E0 or E4 stays `open` or `assumed`.
