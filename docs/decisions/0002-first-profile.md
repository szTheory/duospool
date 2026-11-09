---
status: owner-recorded
owner-statement: Owner selected "Select proposed scope" in the /gsd-discuss-phase 1 session, 2026-11-09 (CONTEXT.md D-06)
owner-date: 2026-11-09
research-ref: SUMMARY #2
reopen-if: a profile field value, status or allowed set changes (new rN.toml per D-27)
---

# 0002: First profile scope (pce-hucard-2btn)

## Context and gate

The first emulated profile needs a named scope so that later phases can bind against fixed field names. ROADMAP success criterion 2 asks for a versioned profile record that names every field and marks each one owner-selected or open. The record is `profiles/pce-hucard-2btn/r1.toml`, checked by `sh scripts/profile-check`. A revision is frozen once a record or phase references it; a change is a new `rN.toml` (D-27).

The check enforces structure, not truth. An owner-selected field is a scope choice, not a hardware fact. An open field carries a labelled research proposal and no value, and stays open until the owner selects it.

## Options considered

| Option | Reading | Evidence label |
|--------|---------|----------------|
| A. Select the proposed scope | Base PC Engine HuCard machine, one HuC6280, one VDC, no VPC, two-button pad, no expansion unit. Disputed hardware facts stay open. | Owner decision, recorded in CONTEXT.md D-06 |
| B. Any other scope | For example adding CD-ROM media, SuperGrafx or per-model regions to the first profile. | Not selected; the owner chose option A (CONTEXT.md D-06) |

## Decision

The owner selected option A: "Select proposed scope" (CONTEXT.md D-06, `/gsd-discuss-phase 1` session, 2026-11-09).

Owner-selected fields (each carries `decision = "docs/decisions/0002-first-profile.md"`):

| Field | Value |
|-------|-------|
| `media_slot` | `hucard` |
| `expansion_port` | `absent` |
| `vdc_count` | `1` |
| `vpc` | `false` |
| `chip_revision` | `huc6280` |
| `input_port1` | `pad2` |

Selecting `huc6280` makes no claim about which retail models shipped which revision. That question stays open as LED-PROF-0001 (C14).

Open fields (a labelled proposal, no value). Phases 2-5 bind each open field explicitly in the run request (`bound_by = "run-request"`, the D-28 Phase 2 contract):

| Field | Ledger entry | Gates |
|-------|--------------|-------|
| `io_1000_d6_value` | LED-IO-0001 | Phase 2 |
| `io_1000_d7_value` | LED-IO-0001 | Phase 2 |
| `lines_per_frame` | LED-VID-0001 | Phase 4 |
| `psg_tick_divisor` | LED-AUD-0001 | Phase 5 |
| `content_byte_order` | LED-LOAD-0001 | Phase 2 |
| `content_copier_header` | LED-LOAD-0002 | Phase 2 |
| `powerup_policy` | none yet | Phase 2 defines it |

`lines_per_frame` means the power-on line count if the count turns out to be guest-selectable (D-34). The record also lists LED-IRQ-0001, LED-VID-0002 and LED-VID-0003 as `unresolved_behaviours`: behaviours the first profile depends on and does not settle.

## Criterion-2 mapping

ROADMAP success criterion 2 lists seven terms. The closed D-28 field list has no region or console-model field on purpose (C6, C14), so each term maps to the fields that realise it (D-32).

| ROADMAP criterion-2 term | r1 field(s) |
|--------------------------|-------------|
| console model/region | `media_slot`, `expansion_port`, `vdc_count`, `vpc`, `io_1000_d6_value` |
| `$1000` bit-6 value | `io_1000_d6_value` |
| CPU/PSG revision | `chip_revision`, `psg_tick_divisor` |
| line count/frame timing | `lines_per_frame` (frame clocks are derived, not stored) |
| controller type | `input_port1` |
| expansion-port presence | `expansion_port` |
| content format | `media_slot`, `content_byte_order`, `content_copier_header` |

Region and console model are deliberately not fields. The `$1000` bit-6 meaning is disputed (C6, LED-IO-0001) and the per-model revision question is open (C14, LED-PROF-0001), so the profile names observed behaviour instead of a region.

## Consequences and what is not decided here

- Phases 2-5 bind the r1 field names. Renaming a field later touches every consumer, so a change is a new `rN.toml` and a new decision record.
- This record does not decide any open field. Each stays open until the owner selects a value, and research proposals are not defaults.
- This record does not decide console model, region, SuperGrafx, CD-ROM, or which retail units contain which CPU revision.
- Hardware facts that the open fields depend on are tracked as ledger entries with status `assumed` and no supporting evidence.
