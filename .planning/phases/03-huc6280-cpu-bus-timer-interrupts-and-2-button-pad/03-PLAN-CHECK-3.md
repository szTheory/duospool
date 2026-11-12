# Phase 3 plan check, round 3 (confirmation)

Verdict: PASS. 0 blockers, 5 warnings, 8 info. Saved by the orchestrator. All 13 round 2 warnings resolved (W9 partial, see NW1, NW2, NW4).

## Warnings

- NW1 Plans 17, 20, 24: the mmu_mpr ROM is shared by several REQUIRED cases and an observe index without an expectation is a harness_failure. 17: add mmu_mpr_256k to the cases that carry the TMA #0 item. 20: same for the 384k and 1m cases. 24: state one form (for example observe=IDX:policy, exempt from cross-policy equality) for every non-diff mmu_mpr case, delete the stale "per-check exclusion list in the case" sentence, list the mmu_mpr case files in files_modified.
- NW2 Plan 17: must_haves line 49 calls the multi-bit edge checks observe-only; delete that phrase so they stay ordinary LIT rows (marking them would drop them from the sweep, weakening D-126).
- NW3 Plans 22 and 25: mutate_sweep_irq_ctl is created in 22 T2 but kind_mutate_sweep gains irq_stim support only in 25 T2. Move the irq_stim support sentence to 22 T2 and add sweepkinds.c to its files; 25 then only reuses it.
- NW4 Plan 10: no guest-side mechanism for observe rows is specified. Add one sentence to T1: observe rows carry a LIT entry the sweep skips, name the check.inc macro, and say observe and exclude are expect items with no schema change (not case keys).
- NW5 (accepted since round 1): 26 plans exceed the 14-file figure under recorded D-81 waivers, up to 57 in plan 28; estimates uncalibrated.

## Info

03-11 T3 stray `, scripts/check` after the files tag. 03-36 duplicate output tag. 03-32 omits ledger/REQUIRED from frontmatter; rule-const-ledger must tolerate mini-roots with no def files and name an existing stage-dead code (NO_SENTINEL or STAGE2_DEAD). 03-10 mutation-sweep-vacuous needs an edit-line marker like SELFTEST-TARGET. 03-09 DS_MAX_STEP_MCLK vs ds_max_step_mclk(). 03-30 region list order.
