---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 02
subsystem: testing
tags: [posix-sh, awk, claim-ledger, profile-record, strict-toml-subset, decision-records, negative-controls]

requires:
  - phase: 01-01
    provides: "scripts/check gate, ledger-check, toml-subset.awk stage 1, decision-check, ledger REQUIRED mechanism, LED-AUD-0001"
provides:
  - "ledger/REQUIRED with ten IDs and ten assumed ledger entries (nine new plus LED-AUD-0001), supported=0"
  - "profiles/pce-hucard-2btn/r1.toml: versioned first-profile record naming all 13 D-28 fields, 6 owner-selected and 7 open"
  - "scripts/profile-check and scripts/lib/profile-rules.awk: two-stage profile checker (14 reason codes plus size gates)"
  - "docs/decisions/0002-first-profile.md: owner-recorded (D-06) with the criterion-2 mapping table (D-32)"
  - "tests/profile/: 19 control cases (17 must-fail, 2 must-pass boundary)"
  - "ledger-check --profiles DIR: PROFILE_UNKNOWN cross-check (D-23) with ledger/negative/profile-unknown"
affects: [01-03, phase-02-contract, phase-04-video, phase-05-audio, ledger, profile]

tech-stack:
  added: []
  patterns:
    - "Profile record = one [table] per field with status owner-selected|open; an open field carries a proposal and never a value"
    - "Wrapper resolves REF and DECISION records from stage 2 with file tests, after awk validated the ID and path shape"
    - "Per-field declared value type (vdc_count integer, vpc boolean, rest string) checked as PROFILE_WRONG_TYPE"

key-files:
  created:
    - profiles/pce-hucard-2btn/r1.toml
    - scripts/profile-check
    - scripts/lib/profile-rules.awk
    - docs/decisions/0002-first-profile.md
    - ledger/entries/LED-VID-0001.toml
    - ledger/entries/LED-IRQ-0001.toml
    - ledger/entries/LED-VID-0002.toml
    - ledger/entries/LED-VID-0003.toml
    - ledger/entries/LED-IO-0001.toml
    - ledger/entries/LED-LOAD-0001.toml
    - ledger/entries/LED-LOAD-0002.toml
    - ledger/entries/LED-AUD-0002.toml
    - ledger/entries/LED-PROF-0001.toml
    - ledger/negative/profile-unknown/
    - tests/profile/
  modified:
    - ledger/REQUIRED
    - ledger/SCHEMA.md
    - scripts/check
    - scripts/ledger-check
    - scripts/lib/ledger-rules.awk

key-decisions:
  - "Open fields carry no value: OPEN_HAS_VALUE is a must-fail control, so a research proposal can never become a silent default"
  - "A proposal must also be a member of allowed (VALUE_NOT_ALLOWED), not only a value"
  - "Ledger IDs and decision paths are shape-validated in awk before the wrapper uses them as file names"

patterns-established:
  - "scripts/check --selftest also mutates profile-rules.awk (removes OPEN_HAS_VALUE) and requires the matching control to break"

requirements-completed: [PROF-01, PROF-05]

actuals:
  tokens: 36200   # chars/4 over the realized git diff; includes two padded 16 KiB boundary control files
  tasks: 3
  commits: 3
plan_head_before: 7553a1491c9e3f7b1aaad9072b1df75e2a959dcc
plan_head_after: e3c639eaafab223fbffa10ac3cdc71179dbddb5c

coverage:
  - id: D1
    description: "Ten mandated conflicts are assumed ledger entries with a recorded assumption, revisit phase, withheld claims and origin; none is supported (ROADMAP SC4, PROF-05)"
    requirement: PROF-05
    verification:
      - kind: integration
        ref: "sh scripts/ledger-check --required ledger/REQUIRED ledger/entries/ (STATUS open=0 assumed=10 supported=0 ...)"
        status: pass
      - kind: integration
        ref: "sh scripts/check (stage ledger-entries)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The first-profile record names all 13 D-28 fields, each owner-selected (with a decision pointer) or open (proposal, no value) (ROADMAP SC2, PROF-01)"
    requirement: PROF-01
    verification:
      - kind: integration
        ref: "sh scripts/check (stages profile-records, profile-negatives)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Record 0002 maps every ROADMAP criterion-2 term to r1 fields (D-32)"
    requirement: PROF-01
    verification:
      - kind: integration
        ref: "grep of the seven terms inside the '## Criterion-2 mapping' section, exactly seven data rows"
        status: pass
    human_judgment: false
  - id: D4
    description: "Boundary and precision edges: 16384 bytes accepted and 16385 rejected, 400-char line accepted and 401 rejected, float rejected as BAD_VALUE, wrong value type rejected as PROFILE_WRONG_TYPE"
    requirement: PROF-01
    verification:
      - kind: integration
        ref: "tests/profile/{ok-boundary-16384,too-big,ok-line-400,long-line,float-value,wrong-type}"
        status: pass
    human_judgment: false
  - id: D5
    description: "A ledger entry whose profile has no profiles/<id>/rN.toml fails with PROFILE_UNKNOWN (D-23)"
    requirement: PROF-01
    verification:
      - kind: integration
        ref: "sh scripts/ledger-check --expect PROFILE_UNKNOWN ledger/negative/profile-unknown/"
        status: pass
    human_judgment: false
  - id: D6
    description: "The seed assumptions are accurate transcriptions of the MEDIUM-confidence research and are the right modelling placeholders"
    requirement: PROF-05
    verification: []
    human_judgment: true
    rationale: "The checker enforces structure, not truth (SCHEMA.md). A reviewer should compare each entry with SUMMARY.md C1-C8, C14 and open-gap 9; each is revisited at its revisit_phase."

duration: ~45min
completed: 2026-11-09
status: complete
---

# Phase 1 Plan 02: Seed ledger and first-profile record Summary

**Ten assumed ledger entries (supported=0), a versioned `pce-hucard-2btn` r1 profile record with 6 owner-selected and 7 open fields, a fail-closed `profile-check` with 19 controls, and decision record 0002 mapping all seven ROADMAP criterion-2 terms.**

## Performance

- **Duration:** about 45 min (start time not recorded; estimate)
- **Completed:** 2026-11-09
- **Tasks:** 3 (1 tracer, 1 auto, 1 TDD)
- **Commits:** 3 task commits, measured from the plan ledger
- **Gate runtime:** `sh scripts/check` under 5 s on macOS `/bin/sh`

## Accomplishments

- **Tracer (Task 1):** nine seed entries (C2-C8, open-gap 9, C14) transcribed from the RESEARCH table, all `assumed`, none `supported`, each worded "DuoSpool models..." or "The first profile assumes...", each with its assumption, revisit phase, withheld claims and an origin equal to its REQUIRED line. LED-VID-0001 states that `lines_per_frame` means the power-on value (D-34). The REQUIRED gate was green end to end before expansion.
- **Profile record and checker (Task 2):** r1.toml names all 13 D-28 fields. Owner-selected: media_slot, expansion_port, vdc_count (integer 1), vpc (boolean false), chip_revision, input_port1, each pointing at record 0002. Open with a labelled proposal and no value: io_1000_d6_value, io_1000_d7_value, lines_per_frame, psg_tick_divisor, content_byte_order, content_copier_header, powerup_policy. `profile-check` resolves ledger IDs and decision paths against the file system and fails closed (STAGE2_DEAD, NO_SENTINEL, size gates). Record 0002 is `owner-recorded` from the verbatim D-06 selection and carries the criterion-2 mapping; the ok-supported-e3 sample from plan 01-01 now resolves to it.
- **Controls (Task 3):** 19 `tests/profile/` cases, each a copy of r1 with one defect and an EXPECT file fixed before the first run. The 16384/16385 byte and 400/401 character edges, the float edge and the wrong-type edge are pinned. `ledger-check --profiles DIR` adds PROFILE_UNKNOWN. `--selftest` gained a profile-rule mutation.
- Mutation probes beyond the plan: each profile rule (OPEN_HAS_VALUE, OPEN_NO_PROPOSAL, SELECTED_NO_VALUE, SELECTED_NO_DECISION, PROFILE_UNKNOWN_FIELD, PROFILE_MISSING_FIELD, PROFILE_BAD_STATUS, VALUE_NOT_ALLOWED, REV_MISMATCH, PROFILE_WRONG_TYPE) was removed in turn from a copy; the matching control broke each time.

## Task Commits

1. **Task 1: tracer, seed ledger and REQUIRED** - `c2e7e2e` (feat)
2. **Task 2: r1 record, profile-check, 0002** - `8ab8c1a` (feat)
3. **Task 3: profile negatives and PROFILE_UNKNOWN** - `e3c639e` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Shape validation of IDs and paths before file lookups**
- **Found during:** Task 2
- **Issue:** the wrapper builds `DIR/<ID>.toml`, a decision path, and (in ledger-check) `profiles/<value>/` from record values. An unchecked value such as `../x` would probe arbitrary paths.
- **Fix:** profile-rules.awk accepts a ledger ID only in the exact LED-AREA-NNNN shape and a decision path only as a plain relative path with no leading slash and no `..` (otherwise LEDGER_REF_MISSING or SELECTED_NO_DECISION); ledger-check treats any profile value outside `[a-z0-9-]` as PROFILE_UNKNOWN without touching the file system.
- **Files modified:** scripts/lib/profile-rules.awk, scripts/ledger-check
- **Commits:** 8ab8c1a, e3c639e

**2. [Rule 2 - Hardening] Two extra controls and two extra checks beyond the plan list**
- **Issue:** SELECTED_HAS_PROPOSAL and PROFILE_UNKNOWN_KEY had no control, and a proposal outside `allowed` was not checked.
- **Fix:** added `tests/profile/selected-has-proposal` and `tests/profile/unknown-key`; `allowed` is checked against the proposal as well as the value; root keys and field keys are type-checked (PROFILE_WRONG_TYPE). PROFILE_ID_MISMATCH has no control because the control layout fixes the directory name `pce-hucard-2btn`; it is a known gap.
- **Commit:** e3c639e

### Plan sequencing

- **Task 3 RED/GREEN:** the profile cases passed as soon as written because profile-check already existed from Task 2; only the ledger PROFILE_UNKNOWN control was genuinely red first (`EXPECT_MISMATCH wanted PROFILE_UNKNOWN got <none>`) and then green after the cross-check was added. Everything went into one commit. The mutation probes above are the compensation for the profile cases.
- **Task 2 edit method:** `scripts/check` edits were made with a short python script rather than the Edit tool; the result is the same text.

## Authentication Gates

None.

## Known Stubs

None. `powerup_policy` has no ledger entry yet by design (Phase 2 defines it). The word "placeholder" appears in the `psg_tick_divisor` label as a deliberate statement that the proposal is not established.

## Threat Flags

None. T-01-04 (assumed status, supported=0, model wording), T-01-05 (OPEN_HAS_VALUE, SELECTED_NO_DECISION, VALUE_NOT_ALLOWED, LEDGER_REF_MISSING controls) and T-01-06 (16 KiB and 400-character caps, boundary controls) are mitigated as planned. Prohibitions held: no open or assumed hardware fact is phrased as established, and no open field has a value or an owner-selected status.

## Decisions Made

- Open fields carry no value; proposals are labelled "Proposal only".
- Proposals are checked against `allowed`.
- Region and console model stay out of the field list (D-32); the mapping lives in record 0002.

## Issues / Backstops

- gawk and mawk were still not run (see plan 01-01); the new awk uses only the same POSIX subset (no intervals, no gawk extensions).
- The seed assumptions are MEDIUM-confidence transcriptions. See coverage D6.
- `ledger/REQUIRED` gate and `scripts/check` read `profiles/` and `docs/decisions/` relative to the current directory; `sh scripts/check` always runs from the repository root.

## Next Phase Readiness

Plan 01-03 and later can add records and controls without editing the gate structure. Phases 2-5 bind the r1 field names and the open-field proposals through the run request.

## Self-Check: PASSED

- Files found: profiles/pce-hucard-2btn/r1.toml, scripts/profile-check, scripts/lib/profile-rules.awk, docs/decisions/0002-first-profile.md, ledger/REQUIRED (10 lines), ten ledger entries, tests/profile (19 cases), ledger/negative/profile-unknown
- Commits found: c2e7e2e, 8ab8c1a, e3c639e
- `sh scripts/check` ends `CHECK RESULT PASS`; `sh scripts/check --selftest` ends `SELFTEST RESULT PASS`
