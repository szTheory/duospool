---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 06
subsystem: fixtures
tags: [fixture-manifest, authored-fixtures, preflight, ca65, tool-record, ledger, verdict-source]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the fixture library, ds_proto and its four protocol variants (03-05), the fixture linter (03-37), fixture-build and the authored used list (03-01)
provides:
  - five authored manifest entries (ds_proto and the four DSV1 protocol variants) with pinned size and sha256, merged into the fixtures-stage used list
  - verdict_source as a +-joined list with the new form manual-cite:PAGE (H8-n, S8-n, BIOS-n); duospool-output still rejected
  - FIX_AUTHORED_REDIST (rule-authored-redist) with a red control, and mini-roots ok-authored and authored-duospool-verdict
  - ca65 and ld65 on the default preflight list, an empty-PATH control in the preflight stage and selftest mutation-preflight-ca65
  - fixtures/lib/TOOLS.md (cc65 tool record), the rewritten .gitignore comment, LED-TOOL-0001 (assumed) and its REQUIRED line
affects: [03-08, 03-32, 03-07]

actuals:
  tokens: 9500
  tasks: 3
  commits: 3
plan_head_before: 7811445c5b4edc85a75d53a79f5b2d3e4e9a2649
plan_head_after: 1cd6c5db123c61d374c4241960206aec9aff1c61

tech-stack:
  added: []
  patterns:
    - "the fixtures stage builds the authored images first (fixture_build_run) and conformance-run fixtures appends build/gate/fixtures-authored.used to the synthetic list; a run without --only that finds no authored list exits 2"
    - "one tagged statement per new rule (rule-verdict-source, rule-authored-redist); the preflight control is a function (pf_empty_path_control SCRIPT) so the selftest can run the same control against a mutated scratch copy"

key-files:
  created:
    - fixtures/manifest/ds_proto.toml
    - fixtures/manifest/ds_proto_predone.toml
    - fixtures/manifest/ds_proto_nodone.toml
    - fixtures/manifest/ds_proto_badsum.toml
    - fixtures/manifest/ds_proto_wrongid.toml
    - tests/gate/fixture/ok-authored/
    - tests/gate/fixture/authored-redistribution-committed/
    - tests/gate/fixture/authored-duospool-verdict/
    - fixtures/lib/TOOLS.md
    - ledger/entries/LED-TOOL-0001.toml
  modified:
    - scripts/conformance-run
    - scripts/check
    - scripts/preflight
    - scripts/lib/fixture-rules.awk
    - .gitignore
    - ledger/REQUIRED

key-decisions:
  - "The five manifest entries use verdict_source manual-cite:H8-5+manual-cite:S8-17+guest-computed for ds-proto and guest-computed for the four protocol variants. ds-proto checks 4 and 5 are tagged ASSUMED:LED-CPU-0010, an entry that does not exist yet, so no spec-literal:LED-CPU-0010 is listed; add it when the plan that creates the entry lands"
  - "The hash pins are the Homebrew bottle builds, shown repeatable and independent of the build directory; the Ubuntu apt package has not been run on the committed sources (see Evidence discipline)"

requirements-advanced: [FIX-01]
requirements-completed: []  # FIX-01 also needs the encoding-matrix oracle (plan 03-08) and the CI install step; not complete

duration: 30min
completed: 2026-11-11
---

# Phase 3 Plan 06: Authored fixtures in the manifest, assembler preflight and tool record Summary

**An authored ROM now reaches the gate only through a manifest entry with a pinned size and hash (an unlisted or hash-changed one is red), a missing ca65 or ld65 fails the first gate stage with PREFLIGHT_MISSING and exit 2, and the tool identity is recorded in fixtures/lib/TOOLS.md and an assumed ledger entry.**

## What was built

- **Task 1 (tracer), commit 305631f.** `conformance-run fixtures` appends `build/gate/fixtures-authored.used` after the synthetic lines (HARNESS_ERROR and exit 2 if it is absent or empty in a run without `--only`; an `--only` run is partial and skips it). `stage_fixtures` in `scripts/check` calls `fixture_build_run` first, so the list is built in the same run. `fixture-rules.awk` splits `verdict_source` on `+` and accepts `spec-literal:LED-id`, `guest-computed`, `independent-script` and `manual-cite:(H8|S8|BIOS)-n` per part; `duospool-output` is rejected as a part in any list, as are an empty part and a malformed cite. Five manifest entries (12 keys each, kind authored, recipe with the exact ca65 and ld65 commands and -D flags, licence 0BSD, redistribution generated-not-committed). Before the entries existed the fixtures stage went red with FIX_UNLISTED for all five names; after, it is green. Probes outside the repo: one changed hash digit gave FIX_HASH_MISMATCH, one deleted entry gave FIX_UNLISTED, `manual-cite:S8-17+guest-computed` and `manual-cite:BIOS-28` pass, `duospool-output`, `guest-computed+duospool-output`, `manual-cite:X8-17`, `guest-computed+` and `manual-cite:H8-` give FIX_BAD_VERDICT_SOURCE.
- **Task 2 (TDD), commit 22ac4a5.** Mini-roots `ok-authored` (passes), `authored-redistribution-committed` (EXPECT FIX_AUTHORED_REDIST) and `authored-duospool-verdict` (EXPECT FIX_BAD_VERDICT_SOURCE), each a copy of the ds_proto entry with a used.txt line. The redistribution control was first seen BROKEN (`EXPECT_MISMATCH wanted FIX_AUTHORED_REDIST got <none>`), then green once the tagged statement `rule-authored-redist` went in. `scripts/preflight` default list is now `... sha256sum|shasum ca65 ld65`. `pf_empty_path_control` (scripts/check) runs a preflight script with PATH set to an empty scratch directory and requires exit 2, `PREFLIGHT_MISSING ca65`, `PREFLIGHT_MISSING ld65` and no STATUS line; the preflight stage runs it on the real script. `selftest mutation-preflight-ca65` shows the control alive on the real script, removes ca65 from the default list in a scratch copy and requires the control to go red.
- **Task 3, commit 1cd6c5d.** `fixtures/lib/TOOLS.md` (0BSD): cc65 V2.19 commit, Zlib, local and CI package names, the V2.18 version-string caveat, identity by behaviour, what was and was not measured, WLA-DX optional verify-only (GPL-2.0-or-later, never committed or installed in CI), HuC and SingleStepTests not used, and the three manual PDFs by URL, byte count and sha256 (cited, never committed). The `.gitignore` comment now says authored ROMs live in ignored `build/` and no allow rule exists (patterns unchanged). `LED-TOOL-0001` (assumed, every schema key, REPORT and CODE sources, evidence E0, withheld list from the plan, pinning_test PENDING:3, revisit_phase 3, origin 03-CONTEXT D-128) and its REQUIRED line.

## Verification

- Task verifies as written: `--stage fixtures` + `--stage fixture-negatives`; `--stage preflight` + `--stage fixture-negatives` + `--selftest mutation-preflight-ca65`; `--stage ledger-entries` + `licences` + `rights-tree`: PASS.
- Plan end: `sh scripts/check` PASS (all stages, 53 s), `sh scripts/check --selftest` PASS, `dash scripts/check` PASS, `sh scripts/audit-history --range 7811445..HEAD` clean (3 commits, 19 blobs), `git ls-files '*.pce'` prints nothing.

## Hash pin portability (writer note 5)

The research identity hash (`70801a2f...`) is for a prototype source that is not in the repository, so it cannot be reproduced here, and nothing was installed to try the apt package or a source build. What was measured instead, on the Homebrew cc65 2.19 bottle: two consecutive builds gave the same five hashes; a build of the same files extracted to a different directory gave the same ds-proto hash (no path leaks into the image); the sources as of commit 2b09d68 (plan 03-05) and as of this plan give the same ds-proto hash. The ds-proto hash `edcd060c...` is therefore the pin. The hashes quoted in the 03-05 summary (`e2f89ea4...` and following) do not reproduce at 2b09d68 and were wrong or from an earlier intermediate state; the manifest is the record. Open: the apt package on these sources (see Notes).

## Deviations from Plan

**1. [Rule 3 - structure] `stage_fixtures` calls `fixture_build_run`, not `stage_fixture_build`.** The plan says "call stage_fixture_build first". `stage_fixture_build` is the registered stage (build plus its empty-PATH control); `fixture_build_run` is the build alone, the same call that `stage_conformance` and `stage_sanitizers` already make. The control still runs once, in its own stage; calling it again from the fixtures stage would repeat it for no extra coverage.

**2. The preflight empty-PATH control is a function, and the Task 2 file list is exceeded by nothing.** `pf_empty_path_control` lives in `scripts/check` (in the list). No file outside the plan's list changed apart from the five tracked manifest entries and the three mini-roots that the list names.

**3. The `rule-authored-redist` statement was written in Task 2, not Task 1.** The plan lists the rule under Task 2; I kept the Task 1 commit to the verdict_source change and the control-first order of Task 2.

No Rule 1, 2 or 4 deviations. `README.md` still lists the gate prerequisites without cc65 (not in this plan's file list); see Notes.

## Evidence discipline

E0 only. A hash pin proves that a build is repeatable on one package family; it says nothing about whether the encoded bytes are right (that is the encoding matrix of plan 03-08). LED-TOOL-0001 stays assumed, with E0 evidence and PENDING:3; no ledger status changed elsewhere. ds-proto's manual literals (H8-5, S8-17) and its ASSUMED checks are as described in the 03-05 summary and are not image-verified here. TOOLS.md records the licence of cc65 as it was recorded in the research and does not claim a fresh read of the licence file in this plan. No accuracy, compatibility, cycle-accuracy, hardware or boot claim. No ROM or manual text is committed; the manual PDFs are cited by URL and hash only (ER-9).

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-18 (hash drift gives FIX_HASH_MISMATCH, the used list is built in the same run), T-03-19 (`*.pce`, `build/` ignored, rights-tree and audit-history clean, no tracked `.pce`) and T-03-20 (preflight fails closed and has a control and a selftest; matrix identity is plan 03-08) are mitigated or advanced as planned.

## Notes for next plans

- Plan 03-08 (encoding matrix): the identity claim of LED-TOOL-0001 and TOOLS.md says the matrix check is the behaviour test; update `pinning_test` from PENDING:3 to the real TEST id when that case exists, and re-check the manifest hashes after any change to a `.inc` or to ds_proto.s (every library edit changes all five pins; the fixtures stage names the entry).
- Any new authored fixture needs a manifest entry whose key is NAME with `-` turned into `_`, a variants list line in `scripts/fixture-build` if it has variants, and one entry per image; the fixtures stage fails with FIX_UNLISTED until it exists, and with FIX_STALE if an entry has no image.
- When LED-CPU-0010 is created, add `spec-literal:LED-CPU-0010` to the ds_proto `verdict_source` (checks 4 and 5).
- The CI step (`sudo apt-get install -y cc65`) and the README prerequisite line are not in this plan; `.github/workflows/gate.yml` and `README.md` are unchanged, so a CI run would fail at preflight until a later plan adds the apt step and its workflow-check controls. Nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. The hash pins are unverified on the apt package until such a run.
- The `ds-proto` entry's `verdict_source` is a statement about where the literals come from; a plan that promotes anything must still re-read the manual pages behind the cites (R-8, plan 03-32).
- `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: five fixtures/manifest/ds_proto*.toml, three tests/gate/fixture mini-roots, fixtures/lib/TOOLS.md, ledger/entries/LED-TOOL-0001.toml. Commits 305631f, 22ac4a5 and 1cd6c5d are ancestors of HEAD.
