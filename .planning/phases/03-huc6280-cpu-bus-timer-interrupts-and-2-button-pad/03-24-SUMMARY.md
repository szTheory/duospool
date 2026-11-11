---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 24
subsystem: w4-poweron-v2-three-policy-fixture-runs
tags: [power-on, seeded-prng-v2, policy-diff, observe-policy, policies-guard, mmu-mpr, w4]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the timer reload and counter and the TIMR/IRQC/IRQS regions (03-21, 03-22), the instance layout and digest regions, the guest kind with observe rows and the default policy list (03-04, 03-10, 03-11), the three mmu_mpr images and their case sets (03-16, 03-20), the ledger entry LED-PROF-0003 with pin PENDING:3 (03-34)
provides:
  - seeded-prng-v2 in src/core/poweron.c, defined once - the v1 ram8k domain unchanged, then the domains cpu (A, X, Y, S and the N V Z C bits), mpr (MPR0 to MPR6) and tmr (timer reload and counter, 7 bits each), ASCII-packed constants CPU_____, MPR_____ and TMR_____, little-endian draws; reset facts and io_buffer and mpr_latch constant under every policy
  - the policy name at all seven places (F12), policy_version per policy through ds_policy_version() (1 for zero and v1, 2 for v2), helpers ds_policy_is_seeded and ds_policy_name; v1, zero and profile r1 byte for byte unchanged, no r2
  - poweron kind items a, x, y, s, p, timer_reload, timer_counter, mpr@IDX, cpu=zero, tmr=zero, drawn=DOMAIN, reset=constants and streams=distinct; cases poweron_v2_smoke, poweron_v2_draw_order (perl-derived literals), poweron_v2_domain_independence and record_power_on_v2
  - the guest kind's default policy list is zero, seeded-prng-v2 seed 1 and seed 2 (GP_DEFAULT_COUNT 3); observe=IDX:policy (exempt from the cross-policy equality), the mode policy-diff=IDX with observe=IDX:differs, and the policies guard with its control guest_policies_guard
  - mmu_mpr: power-up observe rows 61 to 65 (MPR0 and MPR3 to MPR6, 65 checks), three manifests re-pinned, nine cases updated, case mmu_mpr_policy_diff, selftest mutation-policy-diff
  - LED-PROF-0003 pin moved from PENDING:3 to TEST:poweron_v2_draw_order with the pins back-link
affects: [03-25, 03-26, 03-29, 03-30, 03-32]

actuals:
  tokens: 25600
  tasks: 3
  commits: 3
plan_head_before: 17fa0b42b291319a8a072a2dbb193681692476f1
plan_head_after: 391b36b39bc816e82f9b47f1b7427609552decae

tech-stack:
  added: []
  patterns:
    - "a power-on policy that adds registers is additive: v2 draws the v1 domain first and unchanged, each new domain has its own stream, and the reset (cpu.c) keeps the registers the manual leaves open as the policy made them instead of zeroing them, so zero and v1 are byte for byte what they were"
    - "a row that legitimately reads power-on state is an observe row marked observe=IDX:policy and exempt from the cross-policy equality; the negative control is a case with policy-diff=IDX and observe=IDX:differs that fails when the byte is the same under the policies, and a selftest builds the fixture copy that makes it the same"
    - "a REQUIRED guest case may not narrow the default policy list silently: an explicit policies key needs a meaning that starts with policies-override: and a reason, and a control case proves the guard is red-capable"

key-files:
  created:
    - tests/conformance/cases/poweron_v2_smoke.toml
    - tests/conformance/cases/poweron_v2_draw_order.toml
    - tests/conformance/cases/poweron_v2_domain_independence.toml
    - tests/conformance/cases/record_power_on_v2.toml
    - tests/conformance/cases/mmu_mpr_policy_diff.toml
    - tests/conformance/cases/guest_policies_guard.toml
  modified:
    - src/core/poweron.c
    - src/core/ds_poweron.h
    - src/core/cpu.c
    - src/core/ds_core.h
    - src/core/boot.c
    - src/core/profile_fields.def
    - src/text/text.c
    - tests/conformance/powkinds.c
    - tests/conformance/profkinds.c
    - tests/conformance/guestkinds.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/poweron_draw_order.toml
    - tests/conformance/cases/poweron_zero.toml
    - tests/conformance/cases/mmu_mpr_256k.toml
    - tests/conformance/cases/mmu_mpr_384k.toml
    - tests/conformance/cases/mmu_mpr_1m.toml
    - tests/conformance/cases/mmu_mpr_tma_zero_latch.toml
    - tests/conformance/cases/mmu_mpr_tma_zero_keep.toml
    - tests/conformance/cases/mmu_mpr_observe_missing.toml
    - tests/conformance/cases/mutate_sweep_mmu_mpr.toml
    - tests/conformance/cases/mutate_sweep_mmu_mpr_384k.toml
    - tests/conformance/cases/mutate_sweep_mmu_mpr_1m.toml
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - fixtures/manifest/mmu_mpr.toml
    - fixtures/manifest/mmu_mpr_384k.toml
    - fixtures/manifest/mmu_mpr_1m.toml
    - ledger/entries/LED-PROF-0002.toml
    - ledger/entries/LED-PROF-0003.toml
    - scripts/check

key-decisions:
  - "v2 layout (poweron.c header, the independent perl script checks it): cpu draws 5 bytes, A, X, Y, S and a byte whose bits 7, 6, 1 and 0 are N, V, Z and C (the byte and $C3; I stays set, D and T clear); mpr draws 7 bytes for MPR0 to MPR6; tmr draws 2 bytes, reload then counter, each and $7F. Seed 42 gives A $8A, X $EB, Y $57, S $BE, P $87 after the reset, MPR0 to MPR6 da cb c6 28 69 a0 61, reload $73 and counter $2D"
  - "ds_cpu_reset no longer zeroes A, X, Y, S and the N V Z C bits: it keeps what ds_poweron_apply made (boot is the only caller, and the instance is zeroed first), so zero and v1 give the same state as before; I, D, T, speed, the timer start bit, the disable register, TIQ and MPR7 are still set there"
  - "policy_version is ds_policy_version(policy) in poweron.c (zero and v1 print 1, v2 prints 2) and the macro DS_POWERON_POLICY_VERSION is removed from ds_core.h; DS_BEHAVIOUR_EPOCH is not bumped because no existing run (zero, v1) observes a change"
  - "the guest kind compares every policy after the first with the first: an observe=IDX:policy or observe=IDX:differs row is exempt from the byte comparison (its result byte is still compared); policy-diff=IDX requires the byte of row IDX to differ from the first policy's byte in every later policy (zero against both v2 seeds), not only against seed 1"
  - "the mmu_mpr power-up rows are 61 to 65 for MPR0, MPR3, MPR4, MPR5 and MPR6, placed at the start of fixture_main; MPR1 and MPR2 cannot be observed because the shared startup.inc writes them before any fixture code (and its first thirteen instructions are pinned by run_until_abs), so the mpr domain draws seven registers and the fixture sees five. The policy-diff row is 62 (MPR3: seed 1 gives $08, seed 2 gives $42, zero gives $00, from the perl script)"
  - "the plan wording says LED-PROF-0003's pin is flipped in plan 03-32; the orchestrator note and plan 03-34 ask for it here, so the pin is TEST:poweron_v2_draw_order with the case's pins back-link in this plan, and plan 03-32 may re-read it"

requirements-advanced: [CPU-06, CPU-02, CPU-03, CPU-04]
requirements-completed: []

duration: 55min
completed: 2026-11-11
---

# Phase 3 Plan 24: seeded-prng-v2, the three-policy fixture runs and the policy-diff control Summary

**A third power-on policy, seeded-prng-v2, draws the registers the manual leaves open (A, X, Y, S, the N V Z C flags, MPR0 to MPR6, the timer reload and counter) from three new domains with literals recomputed by an independent perl splitmix64, every guest case now runs under zero and two v2 seeds, and a policy-diff control (MPR3 read before any write) shows that the comparison across policies can see a difference.**

## What was built

- **Task 1 (tracer), commit 7d43ec8.** `ds_poweron.h` and `poweron.c`: the policy value 3, the domains cpu, mpr and tmr with ASCII-packed constants, `apply_v2` run after the v1 RAM draw, and the helpers `ds_policy_is_seeded`, `ds_policy_name` and `ds_policy_version`. The seven F12 sites changed together: `profile_fields.def` (allowed set), `profkinds.c` (core list without r1), `ds_poweron.h`, `poweron.c`, both places of `boot.c` (the seed check and the recorded policy) and `text.c` (name and version of `[power_on]`). `cpu.c` keeps the drawn registers at reset. The `poweron` kind gained items for the new registers; cases `poweron_v2_smoke` (reset facts constant, each domain drawn, no value that comes from a run) and `record_power_on_v2` (record prints `seeded-prng-v2` and `policy_version = 2`). The goldens did not change: the record layout and the allowed list are not printed, and the v1 and zero records are identical. The tracer gate (auto mode) re-ran the whole `sh scripts/check` before the expansion: it passed.
- **Task 2, commit 7957d7e.** `poweron_v2_draw_order` pins seed 42: A, X, Y, S, P, MPR0 to MPR6 and MPR7, the timer reload and counter, and the unchanged v1 `ram@` literals, all from a throwaway perl script in a scratch directory (Math::BigInt, no C code; it checked the three new constants against their ASCII names and reproduced the committed v1 `ram@0` and `ram@8184` literals, which shows its splitmix64 is the D-75 one). `poweron_v2_domain_independence` (RAM equals v1, four distinct domain streams), and `cpu=zero;tmr=zero` items on `poweron_zero` and `poweron_draw_order` show v1 and zero draw nothing new. LED-PROF-0003's pin is now `TEST:poweron_v2_draw_order`.
- **Task 3, commit 391b36b.** `guestkinds.c`: the default list is zero, `seeded-prng-v2;seed=1` and `seed=2`; `observe=IDX:policy`, the mode `policy-diff=IDX` with `observe=IDX:differs`, and the policies guard. `mmu_mpr.s` reads MPR0 and MPR3 to MPR6 first (checks 61 to 65, observe rows), the three manifests are re-pinned from the host `shasum -a 256` of the built images, the nine mmu_mpr case files carry `checks = 65`, the `observe=IDX:policy` items (the sweeps list 61 to 65 in `exclude=`), `mmu_mpr_policy_diff` and the control `guest_policies_guard`, and `scripts/check` gains the selftest `mutation-policy-diff`.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `conformance`, `profile-records`; Task 2 `--stage conformance`; Task 3 `--stage fixtures`, `conformance`, `sanitizers`, `--selftest mutation-policy-diff`.
- Plan end: `sh scripts/check --timing` PASS (1 min 42 s wall; 55 stage lines), `sh scripts/check --selftest` PASS (26 s, 67 PASS lines), `sh scripts/audit-history --range 17fa0b4..HEAD` clean (3 commits, 40 blobs, 1389 blob-path pairs, run before this docs commit). `dash scripts/check` was not run (optional).
- Timing (D-132, warn only): `TIMING_WARN sanitizers` at 29 s (it was about 19 s: every guest case now runs three times under the sanitizers); conformance 10 s, determinism 10 s, named-controls 7 s.
- Bite shown with scratch edits, each restored and `cmp`-checked, nothing committed: with `apply_v2` not called `poweron_v2_smoke` fails (the cpu domain drew nothing); a changed MPR constant fails `poweron_v2_draw_order` only (the structural smoke case cannot see it, as designed); an always-true `policies_override_ok` turns `guest_policies_guard` red (outcome pass, not the declared harness_failure); a `row_exempt` that never exempts turns the six mmu_mpr cases red at check 61 (the exemption is needed and used); the selftest `mutation-policy-diff` builds a copy that initialises MPR3 before observing and requires guest_failure naming row 62 (it also passes the unmutated copy first). The existing selftest `mutation-policies-uninit` still passes with the new list.

## Deviations from Plan

**1. [Plan wording] The mmu_mpr power-up section has five rows, not seven.** The plan says it reads MPR0-6; the shared `startup.inc` writes MPR1 and MPR2 before `fixture_main`, so only MPR0 and MPR3 to MPR6 are observable (and the thirteen-instruction anchor of `run_until_abs` forbids moving them). The core draws all seven (`poweron_v2_draw_order` pins them); the fixture rows are 61 to 65.
**2. [Plan wording] The files list is exceeded (37 files over three commits; the plan's waiver names 31).** Added because the work needs them: `src/core/cpu.c` (reset keeps the drawn registers), `tests/conformance/runner.h` (the grammar and the `policy` flag of the observe item), `fixtures/src/mmu_mpr.checks` (rows 61 to 65), `record_power_on_v2.toml` (the plan's own "record prints policy_version 2" behaviour), `poweron_zero.toml` and `poweron_draw_order.toml` (the v1 and zero control items), `LED-PROF-0002.toml` (it said "two policies") and `LED-PROF-0003.toml`.
**3. [Plan wording] LED-PROF-0003's pin is flipped here, not in plan 03-32.** The plan objective says plan 03-32 flips it; the orchestrator note and the 03-34 summary say the pin moves when the v2 cases exist. The comment, evidence and assumption text of the entry were updated and the case has the `pins` back-link; plan 03-32 can still re-read it.
**4. [Design] policy-diff compares every later policy with the first**, so the control also needs seed 2 to differ (it does: MPR3 is $42). The plan says "between zero and a v2 seed".
**5. [Plan wording] `golden` files were not regenerated.** The plan says to regenerate them if the allowed set or the version print changes; neither is printed for the policies the goldens run (zero and v1), so they are byte for byte unchanged and still pass. A v2 record is checked by `record_power_on_v2` instead.
**6. [Rule 3 - format] Ledger comment and key lines kept under 400 characters** (three drafts of the LED-PROF-0003 evidence key failed `ledger-entries` with LINE_TOO_LONG until shortened; the irq_ctl_pre and spd_speed mention moved to a comment line).

No Rule 4 deviation. The shared-state toy control was not touched (run.c unchanged) and stayed RED_OK. The `[withheld] ids` line of the run record was not touched (22 ids, cap 24): no id was added to the record.

## Evidence discipline

E0 throughout. The v2 literals come from a perl script (no DuoSpool output); the structural items (`drawn=`, `reset=constants`, `streams=distinct`) name no value. The power-up rows are ASSUMED:LED-PROF-0003 (status assumed) and are never compared with a value; `observe=IDX:policy` is judged nowhere and `policy-diff` shows only that the bytes differ between policies. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; the draws are a modelling assumption, not a statement about what real hardware holds at power-on (the LED-PROF-0003 withheld list says so). Nothing promoted, nothing pushed (D-64).

## Known Stubs

None added. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-66 (v1 freeze): v1 and zero draw in the old order and the old domains only (`poweron_draw_order`, `poweron_zero` with the new cpu=zero and tmr=zero items, the goldens unchanged). T-03-67 (fixtures that depend on power-on garbage): every REQUIRED guest case runs under three policies with equality of result and observed bytes, and the policy-diff control with its selftest shows the comparison can see a difference; every guest case already passed unchanged under the v2 seeds before the fixture edit. T-03-68 (self-derived literals): `poweron_v2_draw_order` is pinned to the perl recomputation recorded in its comment. The new guard (explicit policies key) closes the route by which a case could have narrowed the three-policy run silently.

## Notes for next plans

- **Default policy list:** every guest case runs three times (zero, v2 seed 1, v2 seed 2), so any new fixture must initialise what it reads, including A, X, Y, the carry and the other N V Z C flags at the first instruction of `fixture_main` (startup.inc sets I, D, S, MPR1, MPR2 only), MPR0 and MPR3 to MPR6, and the timer reload and counter (write the reload before every start). All nine existing fixtures passed unchanged.
- **Sweeps and other kinds** still run zero only; `mutate_sweep_*` of a new mmu_mpr-like fixture needs its power-up rows in `exclude=`. A case with an explicit `policies` key must start its meaning with `policies-override:` and give the reason.
- **03-25 (block transfers), 03-26:** `ds_poweron_apply` runs before `ds_cpu_reset`; a restore (03-29, 03-30) must validate `powerup_policy` (1 to 3), the timer counter and reload (at most 127) and the P byte as before; the drawn registers are plain state afterwards.
- **03-32 (ledger):** LED-PROF-0003 is pinned by `poweron_v2_draw_order` (TEST) and lists three E0 items; the claim that MPR0 to MPR6 are random at power-up is only the pcetech report (not re-read) and the disagreement column says so. LED-IRQ-0007 is still unwritten; the `[withheld] ids` line holds 22 of 24 ids.
- **mmu_mpr:** a new check goes after 65; the case files say `checks = 65` in nine places and the three sweeps' `exclude=` must equal the `.observe` list (49, 59, 60 to 65). The three manifests carry the new hashes (the pins prove repeatability of the build only).
- **Timing:** `sh scripts/check` about 1 min 42 s wall here, the sanitizers stage 29 s (a D-132 warning, not a failure). CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: the six case files (poweron_v2_smoke, poweron_v2_draw_order, poweron_v2_domain_independence, record_power_on_v2, mmu_mpr_policy_diff, guest_policies_guard). Commits 7d43ec8, 7957d7e and 391b36b are ancestors of HEAD.
