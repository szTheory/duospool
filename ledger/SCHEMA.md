<!-- SPDX-License-Identifier: 0BSD -->
<!-- SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors -->

# Claim ledger schema

The claim ledger records, for each behaviour DuoSpool might claim, what is known, what is assumed, and what evidence backs it. Each entry is one file, `ledger/entries/LED-<AREA>-<NNNN>.toml`, written in the strict TOML subset below and checked by `sh scripts/ledger-check`. The gate entry point is `sh scripts/check`.

This check enforces structure, not truth. A well-formed entry can still be wrong. The checker only guarantees that an entry cannot be labelled `supported` without the kind of evidence the policy requires (decision record 0008, proposed default).

## File format (D-18)

- ASCII only. At most 16384 bytes per file, and at most 400 characters per line.
- Allowed: `# comments`, `key = "string"`, `key = <integer>`, `key = true|false`, single-line `key = ["a", "b"]`, and one-level `[table]` headers.
- Not allowed: floats, dates, inline tables, multiline strings, string escapes, single quotes, trailing commas.
- Unknown, duplicate and missing keys are errors. Any real TOML parser reads an accepted file.

## Fields (D-21)

| Field | Type | Content |
|-------|------|---------|
| `id` | string | Entry ID, equal to the file name without `.toml` |
| `statement` | string | One testable sentence. Worded as a modelling assumption while the entry is unsettled, never as fact |
| `profile` | string | A profile id; `profiles/<id>/rN.toml` must exist (PROFILE_UNKNOWN) |
| `status` | string | `open`, `assumed`, `supported`, `refuted`, `deferred` or `withdrawn` |
| `sources` | array | `"LABEL\|ref\|detail"` items, LABEL one of `FACT CODE REPORT UNKNOWN`. At least one |
| `evidence` | array | `"E0..E4\|ref"` items. May be empty unless `supported` |
| `disagreement` | string | The competing positions, or `none` |
| `pinning_test` | string | `TEST:<id>`, `PENDING:<phase>` or `NONE:<reason>` |
| `updated` | string | ISO date, `YYYY-MM-DD` |
| `assumption` | string | Required when `assumed` |
| `revisit_phase` | integer | Required when `assumed` |
| `withheld` | array | Optional: the claims not made while the entry is unsettled |
| `origin` | string | Optional: the research conflict ID, for example `SUMMARY C1` |

Types are checked: `sources`, `evidence` and `withheld` are arrays, `revisit_phase` is an integer, every other field is a string (WRONG_TYPE).

`open` and `assumed` are both unsettled. `assumed` means a modelling assumption has been chosen. It is printed as such and is never phrased as fact.

## Evidence tiers

E0 repeatability only; E1 documented vector or register behaviour asserted literally from a cited source; E2 authored diagnostic ROM with an independent verdict; E3 hardware measurement with recorded setup (owner gate); E4 differential against another emulator.

## `supported` rules (D-22, D-29)

- At least one E1, E2 or E3 evidence item. E0 and E4 never count, alone or combined (SUPPORTED_NO_QUALIFYING_EVIDENCE).
- An E1 item needs a `FACT` source on the entry (E1_NEEDS_FACT_SOURCE).
- An E3 item must reference an owner-gate decision record, written `docs/decisions/NNNN-...` (E3_NEEDS_OWNER_GATE).
- `pinning_test` must be `TEST:` (SUPPORTED_NEEDS_TEST). `PENDING:` is allowed only on entries that are not `supported` (SUPPORTED_PENDING_TEST).
- From Phase 2, `TEST:` IDs resolve against the conformance inventory in both directions.

E3 path-shape limit: the Phase 1 checker accepts an E3 reference by its `docs/decisions/NNNN-` path shape only. It does not open the cited record or read its status. An E3 reference must nevertheless name an `owner-recorded` decision record (D-22), and a reviewer checks that on any entry moved to `supported`.

## IDs (D-20)

- `LED-<AREA>-<NNNN>` with AREA one of `PROF CPU VID AUD IRQ LOAD IO TOOL` and four digits.
- The file name equals the ID.
- IDs are never reused or renumbered. A retired entry becomes `withdrawn`; it is not deleted.
- The research conflict ID (for example `SUMMARY C2`) goes in `origin`, never in the ID.

## Required entries

`ledger/REQUIRED` lists the entries that must exist, one per line as `LED-ID<space>origin text`. The checker confirms that each listed entry exists and that its `origin` equals the rest of the line. Blank lines and `#` comment lines are ignored.

## Control layout (D-38, a dated refinement of D-19)

- `ledger/negative/<case>/` holds one or two `LED-*.toml` files and an `EXPECT` file listing the exact reason codes, comma-separated, fixed before the first run. An optional `REQUIRED` file in the case directory is passed to the checker. Per-case directories are needed because the duplicate-ID case requires two files with one ID, and a shared directory would add ID_FILENAME_MISMATCH to every negative.
- `ledger/samples/ok-<case>/` holds the must-pass cases.
- `sh scripts/check` runs `ledger-check` on `ledger/entries/` (must pass), on each negative with `--expect` (must fail for exactly that code set) and on each sample (must pass). A control that stops behaving this way is reported as `CONTROL BROKEN` or `SAMPLE REJECTED`. `sh scripts/check --selftest` proves that the gate itself is alive.

## Reason codes

Stage 1 (syntax): LINE_TOO_LONG, NON_ASCII_OR_CTRL, DUP_TABLE, SYNTAX, DUP_KEY, BAD_STRING, TRAILING_GARBAGE, BAD_ARRAY, BAD_VALUE.

Stage 2 (schema and rules): UNKNOWN_TABLE, UNKNOWN_KEY, MISSING_FIELD, WRONG_TYPE, BAD_ID, ID_FILENAME_MISMATCH, UNKNOWN_STATUS, BAD_DATE, NO_SOURCE, BAD_SOURCE, BAD_EVIDENCE, BAD_PINNING, ASSUMED_NO_ASSUMPTION, ASSUMED_NO_REVISIT, SUPPORTED_NO_QUALIFYING_EVIDENCE, E1_NEEDS_FACT_SOURCE, E3_NEEDS_OWNER_GATE, SUPPORTED_PENDING_TEST, SUPPORTED_NEEDS_TEST.

Wrapper (files, sets, harness): EMPTY_FILE, FILE_TOO_BIG, EMPTY_LEDGER, PROFILE_UNKNOWN (`--profiles DIR`, default `profiles`), NO_SENTINEL, STAGE2_DEAD, DUP_ID, REQUIRED_MISSING, REQUIRED_ORIGIN_MISMATCH, REQUIRED_BAD_LINE, REQUIRED_EMPTY (a `--required` file with no requirement line), BAD_FILENAME (a path holding TAB or newline), HARNESS_NO_FILE (exit 2), HARNESS_EMPTY_EXPECT (exit 2, an `--expect` naming no code), NO_CONTROLS (gate).

Exit codes: 0 pass, 1 violation or unmet expectation, 2 usage or harness error.
