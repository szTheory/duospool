---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 36
subsystem: core
tags: [placeholder-removal, master-clocks, behaviour-epoch-2, record-format-2, goldens]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: digest serial version 2 without PSGN and VDCP (plan 03-03), ds_run (plans 03-02, 03-35)
provides:
  - struct ds_instance without psg_noise and vdc_phase; one power-on domain (ram8k) with RAM-only seeded-prng-v1
  - run record with time_unit = "master-clocks", behaviour_epoch 2 and format_version 2
  - both goldens and the pinned record literals regenerated in the same commit; golden digests unchanged
affects: [03-24, 03-37, 03-38]

actuals:
  tokens: 8500
  tasks: 1
  commits: 1
plan_head_before: 417e62d3a478f4fe97f935def87c7d65b2e67aba
plan_head_after: f84ceda77f7d87f144043e92c93fedb93bef778d

tech-stack:
  added: []
  patterns:
    - "a dropped power-on domain leaves no trace: each domain has its own stream, so the RAM draws and the poweron_draw_order ram@ literals did not move"

key-files:
  created: []
  modified:
    - src/core/ds_core.h
    - src/core/poweron.c
    - src/core/ds_poweron.h
    - src/text/text.c
    - src/text/ds_text.h
    - tests/conformance/powkinds.c
    - tests/conformance/reckinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/cases/poweron_draw_order.toml
    - tests/conformance/cases/poweron_zero.toml
    - tests/conformance/cases/poweron_seed_zero_supplied.toml
    - tests/conformance/cases/poweron_domain_independence.toml
    - tests/conformance/cases/record_golden_384k_seeded.toml
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml

key-decisions:
  - "poweron_domain keeps its kind and case id: with a single domain left, independence is shown against the stream of a value that names no domain (ds_domain_const gives 0), so the property (another stream cannot move a RAM byte) survives until the v2 domains arrive in plan 03-24"

requirements-advanced: [CORE-04]
requirements-completed: []  # CORE-04 needs the later run and guest plans

duration: 20min
completed: 2026-11-11
---

# Phase 3 Plan 36: Placeholder state out, master-clocks record with epoch 2 Summary

**No Phase 2 placeholder survives: psg_noise, vdc_phase and their power-on domains are gone, the record says master-clocks with behaviour_epoch 2 and format_version 2, and both goldens are current in the same commit.**

## What was built

- **Task 1 (tracer), commit f84ceda.** `ds_core.h` loses the two fields and `DS_BEHAVIOUR_EPOCH` is 2. `ds_poweron.h` and `poweron.c` keep one domain (`DS_DOMAIN_RAM8K`, constant `RAM8K___`), the seeded policy draws only RAM, the zero policy needs no write (the instance is already zeroed). `text.c` prints `time_unit = "master-clocks"`; `DS_TEXT_FORMAT_VERSION` is 2. `emit_power_on` had no placeholder field, so it is unchanged. `powkinds.c` drops the `psg_noise=` and `vdc_phase=` expect items (an old item is now an unknown-item harness error, so a stale case cannot pass silently) and the poweron_domain kind is rewritten as above. The four poweron cases lose their dropped-domain items; the `ram@` literals of `poweron_draw_order` and `poweron_seed_zero_supplied` are untouched.
- **Goldens (T-03-95).** Only three values changed in each golden: `format_version` 1 to 2, `behaviour_epoch` 1 to 2, `time_unit` to `master-clocks`; the comment block says so. The digest lines are unchanged (`31de49dd...` for 256k zero, `e0ec7c31...` for 384k seeded). The host tool was run over the two serial-2 stream files that the plan 03-03 perl script wrote from the documented rules (`shasum -a 256`), and both equal the golden digest lines. The epoch is not in the digest stream, so no digest moved.
- **Record-lint and determinism mini-roots are not edited.** `scripts/lib/record-rules.awk` checks table and key names only, not the `time_unit` value or the version numbers, so the mini-roots that still say `placeholder-cycles`, `format_version = 1` and `behaviour_epoch = 1` stay valid; both stages pass unchanged.

## Verification

- Task verify as written: `--stage build`, `conformance`, `record-lint`, `determinism`, `core-boundary` all PASS.
- `grep -rn 'psg_noise\|vdc_phase\|placeholder-cycles' src tests/conformance scripts` prints nothing.
- Plan end: `sh scripts/check` RESULT PASS, `sh scripts/check --selftest` RESULT PASS, `dash scripts/check` RESULT PASS, `sh scripts/audit-history --range 417e62d..HEAD` clean (1 commit, 16 blobs).

## Deviations from Plan

**1. [Rule 3 - Blocking] hard-coded record literals outside the plan file list**
- **Issue:** `reckinds.c` (refusal record prefix check and the behaviour_epoch line check) and `cases/record_skeleton_256k.toml` (`record.format_version=1;core.behaviour_epoch=1`) pin the old version numbers independently of the C macros, so they turn red when the epoch and format version are bumped.
- **Fix:** the literals are now `format_version = 2` and `behaviour_epoch = 2`. Not weakened: the checks still compare against literal text.
- **Commit:** f84ceda.

**2. [Rule 3 - Blocking] comment-only edits outside the plan file list**
- `stepkinds.c` (a comment naming the dropped fields) and `cases/record_golden_384k_seeded.toml` (the `meaning` text said placeholder-cycles) were edited so the plan-end grep prints nothing and the text is true. No behaviour change.

**3. Plan wording, no behaviour change:** the plan says to remove "any placeholder field from emit_power_on"; there was none (the table prints policy, policy_version, seed_present, seed).

## Evidence discipline

E0 only. Nothing here shows the machine is right; the goldens show repeatability. No ledger status changed, no ROM or third-party material added, no accuracy, compatibility, cycle-accuracy, hardware or boot claim.

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-10 (RAM draws unchanged, `ram@` literals passing) and T-03-95 (digests unchanged and re-checked with the host tool, the three value changes listed above) are mitigated.

## Notes for next plans

- `ledger/entries/LED-PROF-0002.toml` still says the zero policy sets `psg_noise 1, vdc_phase 0` and that stepping is placeholder evolution ("Phase 3 replaces it"). It was outside this plan's file list and ledger statuses stay as they are; the plan that adds `seeded-prng-v2` (03-24) or the ledger-touching plan should reword that statement and assumption text to match the code.
- `enum ds_domain` now has one member; plan 03-24 adds the v2 domains (`cpu`, `mpr`, `tmr`) as fixed ASCII-packed constants and can then retarget `poweron_domain_independence` at real second domains (its `PD_OTHER` stand-in goes away).
- Plans that change the text layout again must bump `DS_TEXT_FORMAT_VERSION`, update both goldens, the refusal-prefix literal in `reckinds.c` and the skeleton case in one task.
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation.

## Self-Check: PASSED

Modified files present; commit f84ceda is an ancestor of HEAD.
