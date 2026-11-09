---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 04
subsystem: testing
tags: [posix-sh, awk, provenance, licensing, spdx, gitignore, negative-controls]
status: complete

requires:
  - phase: 01-02
    provides: "scripts/check stage and negative-control runner pattern"
  - phase: 01-03
    provides: "record 0007 (copied-code provenance principle and PR checklist line)"
provides:
  - "PROVENANCE.md: zero-entry copied-code register carrying the D-26 PR checklist line"
  - "scripts/provenance-check and scripts/lib/provenance-register.awk: D-26 gate (SPDX allowlist, markers resolve both ways, whole-tree scan, exact-path exclusions)"
  - "scripts/licence-check: Apache-2.0 sha256 pin, MIT/0BSD holder and permission-sentence pins, SPDX header on every file under scripts/"
  - "LICENSE-MIT, LICENSE-APACHE, LICENSE-0BSD with holder 'The DuoSpool Authors' 2026"
  - "README.md with the not-trademark-cleared note, dual-licence and contribution wording, 0BSD fixture note and the local gate"
  - ".gitignore with build outputs, spikes/tool-01/out/ and nine active ROM-like extension patterns"
  - "scripts/check stages provenance, provenance-negatives, licences, licence-negatives, plus a fourth selftest mutation"
affects: [01-05, 01-11, 01-12, phase-02-ci, fixtures, spike]

tech-stack:
  added: []
  patterns:
    - "Checker prints EXPECT_OK or a RECORDS line on clean exit so scripts/check can tell a dead checker from a pass"
    - "Control fixtures that carry a real marker are listed by exact path in tests/provenance/SELF-FIXTURES; the list is validated in both directions"
    - "Awk register parser avoids interval expressions so it runs under mawk, BWK awk and gawk"
    - "Licence texts fetched verbatim and pinned by hash or fixed-sentence grep, never retyped"

key-files:
  created:
    - PROVENANCE.md
    - scripts/provenance-check
    - scripts/lib/provenance-register.awk
    - scripts/licence-check
    - LICENSE-MIT
    - LICENSE-APACHE
    - LICENSE-0BSD
    - README.md
    - .gitignore
    - tests/provenance/ (18 case directories plus SELF-FIXTURES)
    - tests/licence/ (5 case directories)
  modified:
    - scripts/check

key-decisions:
  - "Marker scan covers the whole tree (scripts/, docs/, tests/, .planning/); only exact paths are skipped, and the SELF-FIXTURES list fails with PROV_BAD_EXCLUSION when stale and PROV_UNRESOLVED_MARKER when incomplete"
  - "A register block holding a syntax error is reported once (PROV_SYNTAX) and not field-checked, so one defect gives one reason code"
  - "An entry with a malformed id skips the per-path marker check, because no valid marker can exist for it"

patterns-established:
  - "One-defect case directory per rule with an EXPECT file, plus must-pass ok-* cases, for every checker"

requirements-completed: []  # TOOL-01 is only advanced here (the .gitignore the spike relies on); it completes with the language decision record

actuals:
  tokens: 34000   # chars/4 over the realized git diff since the plan base (includes ~11k tokens of licence text)
  tasks: 3
  commits: 3
plan_head_before: fc21f53629635baa2b51b3ee269dac30b19265d4
plan_head_after: fbb8b0845955052e259d6f3ca58645be291a3f0c

coverage:
  - id: D1
    description: "PROVENANCE.md ships with zero entries and provenance-check passes on it, scanning scripts/, docs/, tests/ and .planning/ (D-26)"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/provenance-check (RECORDS 0, exit 0); sh scripts/check stage provenance"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every D-26 rule has a must-fail control (GPL SPDX, WITH exception, missing/unknown field, bad id/ref/sync-policy, duplicate id, unresolved marker in src/scripts/docs, bad exclusion, missing path, missing marker, syntax) and three must-pass samples"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/check stage provenance-negatives; sh scripts/check --selftest mutation-provenance-rule"
        status: pass
    human_judgment: false
  - id: D3
    description: "LICENSE-APACHE is byte-identical to apache.org (sha256 cfc7749b...), MIT and 0BSD carry the holder line, and scripts/ files all carry both SPDX lines"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/licence-check (RECORDS 13); tests/licence controls altered-apache, mit-no-holder, zero-bsd-reworded, script-no-spdx; ok-minimal passes"
        status: pass
    human_judgment: false
  - id: D4
    description: "Generated spike fixtures and ROM-like extensions cannot be staged by accident"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "git check-ignore on spikes/tool-01/out/lcg-64k.bin and the nine ROM-like sample paths, all exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "README wording (trademark note, dual-licence and contribution clause, 0BSD fixture note) is accurate for the owner's intent"
    requirement: TOOL-01
    human_judgment: true
    rationale: "The checks do not read README.md; only the owner can confirm the wording says what was meant"
---

# Phase 1 Plan 04: Provenance register, licence files and ignore rules Summary

**A zero-entry copied-code register with a whole-tree provenance-check (SPDX allowlist, two-way marker resolution, exact-path exclusions), verbatim hash-pinned Apache-2.0 plus MIT and 0BSD texts with a licence-check, a README carrying the trademark and contribution wording, and a .gitignore that blocks generated spike fixtures and ROM-like files, each checker with a must-fail control per rule.**

## Performance

- **Duration:** about 25 min (start time was not captured; estimate)
- **Completed:** 2026-11-09
- **Tasks:** 3 (1 tracer, 1 tdd, 1 auto)
- **Files:** 87 changed (2356 insertions)

## Accomplishments

- Tracer: PROVENANCE.md (zero entries) through provenance-check into scripts/check, with the GPL control `gpl-spdx`. Re-ran the gate end to end before expanding, and confirmed that removing SELF-FIXTURES turns the real-root scan red with PROV_UNRESOLVED_MARKER.
- 18 provenance control cases (15 must-fail including the tracer's gpl-spdx, 3 must-pass); `dup-id` isolates PROV_DUP_ID with EXPECT written first and no checker change needed. SELF-FIXTURES lists the 12 marker-bearing fixture paths exactly, and a deliberately stale line was confirmed to give PROV_BAD_EXCLUSION.
- Licence texts: LICENSE-APACHE fetched from apache.org and matches the pinned sha256 `cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30`; MIT and 0BSD fetched from SPDX list v3.29.0 with only the template copyright line replaced.
- licence-check and five controls; the gate now has 11 stages and ends `CHECK RESULT PASS`; `--selftest` ends `SELFTEST RESULT PASS`.
- Plan 01-03 asked this plan to add the PR checklist line to PROVENANCE.md; it is there verbatim (and still in record 0007).

## Task Commits

1. **Task 1: tracer, PROVENANCE.md, provenance-check, gpl-spdx control** - `ea15464` (feat)
2. **Task 2: provenance negative corpus** - `f57f7f9` (test)
3. **Task 3: licence files, README, .gitignore, licence-check and controls** - `fbb8b08` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Structure] Register parser split into scripts/lib/provenance-register.awk**
- **Found during:** Task 1
- **Issue:** The plan lists only scripts/provenance-check. The register rules are too long for an inline awk script, and plans 01-01..01-03 keep awk rules in scripts/lib/.
- **Fix:** Added `scripts/lib/provenance-register.awk` (SPDX-headed, no interval expressions for mawk portability). licence-check covers it.
- **Files modified:** scripts/lib/provenance-register.awk
- **Commit:** ea15464

**2. [Rule 2 - Missing critical] Fourth selftest mutation**
- **Found during:** Task 1
- **Issue:** The plan did not extend `--selftest`, so a deleted PROV_BAD_SPDX rule would only show up through the control run.
- **Fix:** Added mutation (d) to scripts/check: delete the PROV_BAD_SPDX rule from a copy and require the gpl-spdx control to turn broken.
- **Files modified:** scripts/check
- **Commit:** ea15464

**3. [Rule 1 - Design consequence] bad-id control carries no marker**
- **Found during:** Task 2
- **Issue:** PROV-1 cannot have a valid four-digit marker. A marker for another id would add PROV_UNRESOLVED_MARKER, and a missing one would add PROV_PATH_NO_MARKER, so the plan's single-code EXPECT for `bad-id` could not hold.
- **Fix:** The checker skips the per-path marker check for an entry whose id is malformed (existence is still checked), and the `bad-id` stub carries no marker. EXPECT stays exactly `PROV_BAD_ID`.
- **Files modified:** scripts/lib/provenance-register.awk, tests/provenance/bad-id/
- **Commit:** ea15464, f57f7f9

**4. [Rule 2 - Missing critical] Extra must-pass control ok-stale-warning**
- **Found during:** Task 2
- **Issue:** The "PROV_STALE never affects the exit code" rule had no control.
- **Fix:** Added `tests/provenance/ok-stale-warning` (tracked entry copied 2024-01-15): it prints `WARN PROV_STALE PROV-0001` and exits 0. Its stub is listed in SELF-FIXTURES.
- **Files modified:** tests/provenance/ok-stale-warning/, tests/provenance/SELF-FIXTURES
- **Commit:** f57f7f9

**5. [Rule 1 - Wording] README 0BSD sentence on one line**
- **Found during:** Task 3
- **Issue:** The first draft wrapped `No 0BSD fixture directory exists yet` across two lines, so the plan's one-line grep acceptance would not match.
- **Fix:** Kept the sentence on one line.
- **Files modified:** README.md
- **Commit:** fbb8b08

**6. [Scope] TOOL-01 left unchecked in REQUIREMENTS.md**
- **Found during:** state updates
- **Issue:** `requirements.mark-complete TOOL-01` (from the plan frontmatter) ticked TOOL-01, but TOOL-01 needs the owner-confirmed language decision record from the later spike and decision plans (01-11). This plan only supplies the `.gitignore` the spike relies on.
- **Fix:** Reverted the REQUIREMENTS.md change before the metadata commit. The frontmatter `requirements-completed` is kept for traceability, but the requirement itself stays Pending.
- **Files modified:** none (reverted)

**Earlier scripts needing SPDX lines:** none. Every script from plans 01-01..01-03 already carried both lines, so no earlier script was edited.

**Total deviations:** 6 (two additions, four adjustments). **Impact:** none on acceptance criteria.

## Authentication Gates

None.

## Known Stubs

None. The `src/` files under tests/provenance/ and the `scripts/` stubs under tests/licence/ are intentional authored control fixtures, not wired UI or data.

## Threat Flags

None. No new network endpoints, auth paths or file access beyond the planned curl fetch of licence texts, which was verified by hash (Apache) and fixed-sentence grep (MIT, 0BSD).

## Self-Check: PASSED

- Created files found: PROVENANCE.md, scripts/provenance-check, scripts/lib/provenance-register.awk, scripts/licence-check, LICENSE-MIT, LICENSE-APACHE, LICENSE-0BSD, README.md, .gitignore, tests/provenance/SELF-FIXTURES, tests/licence/ok-minimal
- Commits found on HEAD: ea15464, f57f7f9, fbb8b08
- `sh scripts/check` ends `CHECK RESULT PASS`; `sh scripts/check --selftest` ends `SELFTEST RESULT PASS`
