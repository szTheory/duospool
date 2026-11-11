---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 04
subsystem: conformance
tags: [case-schema, optional-keys, guestkinds, authored-fixture, result-tail, inventory-controls]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the guest kind and boot_built_rom (plan 03-01), runner headroom (plan 03-03), the run kinds (plan 03-02), epoch 2 record (plan 03-36)
provides:
  - seven optional case keys (until_mclk policies checks input irq_stim sinks chunks) registered in the runner.h enum, the runner.c key table (type i for the two integers) and the inventory want table
  - guestkinds.c with kind_guest, the DSV1 judge, the authored-ROM loader and the authored_name and note_run helpers
  - fixture = authored:NAME as the way a guest case names its ROM; fixlist.c skips it
  - RESULT lines of guest and run kinds end machine_time=N stop=REASON
  - inventory controls ok-case-optional-key and case-optional-wrong-type with the rule INV_CASE_TYPE
affects: [03-05, 03-06, 03-09, 03-24, 03-31]

actuals:
  tokens: 9000
  tasks: 2
  commits: 2
plan_head_before: 1b3b9724aafaa5dec695aa00c9df2c691666e238
plan_head_after: 87a9c85815f0dc648507460548907dd0b1798dd4

tech-stack:
  added: []
  patterns:
    - "optional case keys are a bit mask (K_OPTIONAL_MASK) in runner.h, not a second key list; the inventory awk keeps the ten mandatory keys in the missing-key loop and puts the seven optional keys in the want table only"
    - "a kind that runs the machine calls note_run(c, machine_time, reason) and the RESULT line carries the tail; kinds that do not run print the old line"

key-files:
  created:
    - tests/conformance/guestkinds.c
    - tests/gate/inventory/ok-case-optional-key/tests/conformance/REQUIRED
    - tests/gate/inventory/ok-case-optional-key/tests/conformance/cases/opt_keys.toml
    - tests/gate/inventory/ok-case-optional-key/ledger/entries/LED-PROF-0001.toml
    - tests/gate/inventory/case-optional-wrong-type/tests/conformance/REQUIRED
    - tests/gate/inventory/case-optional-wrong-type/tests/conformance/cases/opt_bad.toml
    - tests/gate/inventory/case-optional-wrong-type/ledger/entries/LED-PROF-0001.toml
    - tests/gate/inventory/case-optional-wrong-type/EXPECT
  modified:
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/spinkinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/fixlist.c
    - CMakeLists.txt
    - scripts/lib/inventory-rules.awk
    - scripts/inventory-check
    - tests/conformance/cases/ds_proto.toml

key-decisions:
  - "Guest case grammar after this plan: fixture = authored:NAME, until_mclk and checks are the integer keys (both required by the guest kind, harness_failure when absent), expect = id=N. The run kinds of stepkinds.c keep their rom=NAME expect grammar and fixture = none"
  - "Integer keys are unsigned decimal of 1 to 19 digits (T-03-11). conformance-flatten already refuses hex (BAD_VALUE) and the runner refuses any other text of type i; inventory gives INV_CASE_TYPE for a non-integer until_mclk or checks"
  - "Stop reason names are until, callback and refused (the ingest accepts [a-z][a-z0-9_:-]*). The last ds_run of a case wins: run_until reports its last item, run_chunk_sum its reference run, run_refuse its first valid run"

requirements-advanced: [CORE-04, FIX-03]
requirements-completed: []  # both need later plans (run kinds in W2 and later, mutation sweep)

duration: 20min
completed: 2026-11-11
---

# Phase 3 Plan 04: Optional case keys, guestkinds.c and authored fixture key Summary

**Cases may now carry until_mclk, policies, checks, input, irq_stim, sinks and chunks in all three sync points; the guest kind lives in guestkinds.c, names its ROM as authored:NAME, and guest and run RESULT lines end machine_time=N stop=REASON.**

## What was built

- **Task 1 (tracer), commit 3b3e1a7.** `runner.h` gets the seven new `K_` entries and `K_OPTIONAL_MASK`; `run_case` refuses a missing key only when its mask bit is clear (a missing mandatory key is still a harness error, probed with a scratch case). `key_def` gains the seven rows with type letter `i` for until_mclk and checks, and a type-`i` value must be one to nineteen decimal digits. `inventory-rules.awk` has the seven keys in `want` (the missing-key loop still runs over the ten). `kind_guest`, the DSV1 judge, `read_rom` and `boot_built_rom` moved from `spinkinds.c` into the new `guestkinds.c` (in `DS_RUNNER_SOURCES`; the toy runners reuse that list); `stepkinds.c` still calls `boot_built_rom` through `runner.h`. The guest kind takes the ROM from `authored_name()` (only `[a-z0-9-]`, file read only from `build/gate/fixtures/NAME.pce`, T-03-12), the deadline and check count from the keys, the fixture id from `expect = "id=N"`. `fixlist.c` skips `authored:` fixtures. `ds_proto` is migrated: `fixture = "authored:ds-proto"`, `until_mclk = 4000000`, `checks = 2`, `expect = "id=1"`. Its RESULT line reads `... machine_time=4000008 stop=until`.
- **Task 2 (TDD), commit 87a9c85.** RED first: with the two mini-roots in place `inventory-negatives` failed with `wanted INV_CASE_TYPE got INV_CASE_FIELD`. GREEN: one tagged statement `rule-case-int-key` in `inventory-rules.awk` emits `INV_CASE_TYPE` for a non-integer until_mclk or checks. `ok-case-optional-key` (all seventeen keys, valid) passes, `case-optional-wrong-type` (`until_mclk = "x"`) gives exactly `INV_CASE_TYPE`, and `case-missing-field` stays red.
- **Selftest statement (plan text).** No selftest block was added. Deleting the `rule-case-int-key` line leaves `case-optional-wrong-type` without its code, so the `run_controls` call in the `inventory-negatives` stage goes red. Checked by hand on a scratch copy of `scripts/` with the line removed: `EXPECT_MISMATCH wanted 'INV_CASE_TYPE' got 'INV_CASE_FIELD'`.

## Size record (F7)

runner.c 45024 bytes (was 44366), guestkinds.c 13613, spinkinds.c 4945 (was 16579), runner.h 12330, stepkinds.c 25406. None is a multiple of 8192 or 8192k+512; `audit-history --range 1b3b972..HEAD` is clean (2 commits, 16 blobs). runner.c grew by 658 bytes: the key rows, `is_uint_text`, the mask test and the RESULT tail (the plan's "key table, mask, one row per kind" scope; there is no new kind row because the guest kind already had one).

## Verification

- Task 1 verify as written (`--stage build`, `conformance`, `inventory`, `fixtures`): all PASS. Task 2 verify (`--stage inventory`, `--stage inventory-negatives`): PASS, after a recorded RED.
- Extra probes (scratch, outside the repo): a guest case with no `meaning` key gives harness_failure "a key is missing"; a guest case with no optional keys is accepted by the runner and reaches the kind (harness_failure "a guest case needs the integer keys"); a hex until_mclk is refused by `conformance-flatten` (BAD_VALUE).
- Plan end: `sh scripts/check` PASS, `sh scripts/check --selftest` PASS (including mutation-guest-wrong-literal, still guest_failure with check 1 at result byte $46), `dash scripts/check` PASS, `sh scripts/audit-history --range 1b3b972..HEAD` clean.

## Deviations from Plan

**1. [Rule 3 - Blocking] comment-only edit outside the plan file list**
- `scripts/inventory-check` lists every INV_ code in its header; INV_CASE_TYPE was added there so the header stays true. No behaviour change. Commit 87a9c85.

**2. Plan wording, no behaviour change:** the plan says the guest kind reads `until_mclk` and `checks` from the case keys "while still accepting the plan 03-01 expect grammar for the run kinds". The guest kind itself no longer accepts `rom=NAME;id=N;checks=N;until=N` (its ROM, deadline and count come from keys); only the run kinds keep `rom=NAME` in their expect. No other case used the old guest grammar.

## Evidence discipline

E0 only. The guest verdict is what the authored fixture wrote; the RESULT tail shows repeatable machine time, not correct timing. No ledger status changed, no ROM or third-party material added, no accuracy, compatibility, cycle-accuracy, hardware or boot claim.

## Known Stubs

None. The keys policies, input, irq_stim, sinks and chunks are accepted by the schema as strings but no kind reads them yet; later waves (03-24, 03-31 and the pad and observer plans) add the readers.

## Threat Flags

None beyond the plan's register. T-03-11 (type i of at most 19 digits in the runner, INV_CASE_TYPE with a red control in inventory) and T-03-12 (authored name restricted to `[a-z0-9-]`, loader reads only `build/gate/fixtures/NAME.pce`) are mitigated.

## Notes for next plans

- New kinds that run the machine call `note_run(c, inst->machine_time, reason)` after `ds_run` to get the RESULT tail; kinds that do not run need nothing.
- A kind reads an optional key with `k->has[K_X]` first (the value pointer is NULL when absent); integers go through `parse_u64`, which is safe because the runner already refused anything that is not 1 to 19 decimal digits.
- Wrong type on the seven optional string keys is `INV_CASE_FIELD` (generic); only until_mclk and checks have `INV_CASE_TYPE`. If a later plan gives another optional key an integer type, change its entry in the `want` table, the `key_def` row and the `rule-case-int-key` condition together.
- Guest cases from plan 03-05 on use `fixture = "authored:NAME"`, `until_mclk`, `checks` and `expect = "id=N"`. The used list for authored fixtures stays `build/gate/fixtures-authored.used`.
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation.

## Self-Check: PASSED

Created files present (guestkinds.c and the six mini-root files plus EXPECT). Commits 3b3e1a7 and 87a9c85 are ancestors of HEAD.
