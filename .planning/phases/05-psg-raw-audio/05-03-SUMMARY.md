---
phase: 05-psg-raw-audio
plan: 03
subsystem: psg-state-layout-bump-and-register-decode
tags: [aud, psg, bump, serial-9, epoch-6, text-8, snapshot, restore-domain, poweron-v4, w2]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-01 handbook and ledger LED-AUD-0003..0014; 05-02 inert ds_psg_advance at both fold points
provides:
  - the final PSG state layout (PSG0 42, psg_pad 6, PSGW 192, PSGT 40; instance 1,141,376 bytes) and the one bump (serial 9, epoch 6, text 8, 24 regions, snapshot 92,553 bytes)
  - restore pass 1 refusing every D-203 violation (D-231 drops the TRG rule), with 34 hostile rows in snapshot_refusal_psg and snapshot_refusal_psgt
  - psg.c ds_psg_divisor, ds_psg_reg_mask, ds_psg_poweron, ds_psg_advance (noise counters and LFSRs from time 0, closed-form channel catch-up, PR-1 rebase), ds_psg_write (decode, storage, counter bookkeeping, effect bits)
  - src/core/psg_effects.def (b0..b6), 15 psg_ constants rows (49 in all), LED-AUD-0009 in the fixed withheld list (42 ids)
  - seeded-prng-v4 = policy 5 (v3 draws, then PSGW____ & $1F); guest and video default lists zero, v4 s1, v4 s2; hostile row policy-6=field
  - psgkinds.c with psg_decode, psg_wave_ptr, psg_lfo_trg, psg_poweron, psg_restore_divisor
affects: [05-04, 05-05, 05-06, 05-07, 05-09, 05-10]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-02]

actuals:
  tokens: 49219
  tasks: 3
  commits: 3
plan_head_before: 2ab40cd885d95356213c1ff3fc8946f074a78c57
plan_head_after: 4515695de1b2d483b959b89b3169f0e4e8e35e06

tech-stack:
  added: []
  patterns:
    - "Stored PSG distances are offsets from B = machine_time - 1 (PR-1): a write-path fold processes events at offsets <= stamp - B without rebasing, the step-end fold rebases by subtracting t; no absolute time near 2^64 is formed"
    - "A snapshot edit that moves TIME also moves every grid-bound counter (sk_psg_follow_time), as the beam follows the time since plan 04-19"
    - "Hostile rows reset the PSG to a valid all-frozen baseline first, and each range row sits on the tick grid, so a row fails only its own rule"

key-files:
  created:
    - src/core/psg_effects.def
    - tests/conformance/psgkinds.c
    - tests/conformance/cases/psg_decode.toml
    - tests/conformance/cases/psg_wave_ptr.toml
    - tests/conformance/cases/psg_lfo_trg.toml
    - tests/conformance/cases/psg_poweron.toml
    - tests/conformance/cases/psg_restore_divisor.toml
    - tests/conformance/cases/snapshot_refusal_psg.toml
    - tests/conformance/cases/snapshot_refusal_psgt.toml
  modified:
    - src/core/ds_core.h, ds_serial.h, serial.c, snapshot.c, ds_snapshot.h, boot.c, bus.c, psg.c, ds_psg.h, constants.def, poweron.c, ds_poweron.h, profile_fields.def
    - src/text/ds_text.h, src/text/text.c
    - tests/conformance/reckinds.c, snapkinds.c, digkinds.c, powkinds.c, profkinds.c, guestkinds.c, vdckinds.c, runner.c, runner.h, REQUIRED; CMakeLists.txt
    - tests/conformance/golden/accept_256k_zero.record.toml, accept_384k_seeded.record.toml
    - cases digest_stream_prefix, _blkt, _input, _irq, _model, digest_region, snapshot_equals_digest, record_skeleton_256k, snapshot_refusal_fields

key-decisions:
  - "05-03: the closed-form channel catch-up landed in Task 2 with the restore domain (planned for Task 3): from the bump on a restored snapshot may hold a running channel, and its distance must be folded and rebased like the noise counters"
  - "05-03: the D-203 hostile rows landed with the rules in Task 2 (R4), in two new snapshot_refuse cases, because the 400-character line rule leaves no room in the expect of snapshot_refusal_fields"
  - "05-03: a counter starts at offset (stamp - B) + d x Fe - stamp mod d, so its next event is (w - w mod d) + d x Fe on the tick grid; the stored distance is next - B"
  - "05-03: epoch 6 and serial 9 rely on an unreleased epoch (D-202, R11); nothing of 05-03 is pushed"

duration: about 3 h 40 min wall (gates at load 14 to 33)
completed: 2026-11-13
---

# Phase 5 Plan 03: PSG state layout, the one bump, register decode Summary

**The PSG state is laid out once (PSG0, PSGW and PSGT behind vid_pad, 280 bytes) and versioned once (serial 9, epoch 6, text 8, 24 regions). Restore accepts only the D-203 domain, the noise counters clock from time 0 under every policy, and writes go through one masked decode with the D-185 counter bookkeeping. seeded-prng-v4 draws the wave RAM after the v3 bytes.**

## Performance

- Tasks: 3 of 3; no stop-rule split was needed (05-11 stays free)
- Files: 46 changed in 2ab40cd..HEAD (9 added, 37 modified), 1,626 insertions, 244 deletions

## Accomplishments

- Task 1 (a9d2c3f): both record goldens' history blocks were compacted to the D-53 statement, the P15 method and the derivation of every literal in the file (16,065 / 16,099 -> 7,149 / 7,183 bytes, bodies unchanged byte for byte). CROWS_MAX went 40 -> 64 (the WH_MAX value). record_constants_shown needed no text change (it names no cap).
  F-55 bound: 60 + 1 + 42 + 4 + 1 + 2 + 16 + 16 + 49 = 191 of REC_MAX_ROWS 192. REC_MAX_ROWS is unchanged (headroom 1 row at 16 stimulus events).
- Task 2 (2575af4): the layout, the four versions, the three regions and their decoders, ds_psg_poweron in boot after ds_vdc_poweron (with a _Static_assert that reset_cost_mclk is 0), the fold, 15 constants rows, LED-AUD-0009, the regenerated literals and 34 hostile rows. The goldens are now 8,557 and 8,601 bytes.
- Task 3 (4515695): bus.c calls ds_psg_write after the fold; psg_effects.def; seeded-prng-v4; the policy row move; the five psgkinds cases.

## Observed red list against the predicted one (Task 2, before any literal moved)

| Case | Predicted | Observed |
|---|---|---|
| digest_stream_prefix, _blkt, _input, _irq, _model | red | red |
| digest_region | red | red |
| record_skeleton_256k | red | red |
| record_golden_256k_zero, record_golden_384k_seeded | red | red |
| snapshot_equals_digest | red | green (it compares the snapshot with the digest stream; it has no literal) |
| record_refusal_text | red if it prints versions | green (its version strings moved in reckinds.c in the same change) |
| snapshot family until the snapkinds tables move | red | snapshot_refusal_api and snapshot_refusal_window red. Their accept rows move TIME, so the snapshot's noise distances fell off the new tick grid. Fixed by sk_psg_follow_time (the edits now move PSGT with the time, as they move BEAM since plan 04-19) |

No red case fell outside the predicted list, so the correction bump was not used.

## Regeneration (D-165 method, P15)

A scratch export of the version 8 tree (HEAD a9d2c3f) had its digkinds.c patched (scratch only) to write each captured stream, and two scratch cases dumped the golden boots. All seven OLD streams (92,255 bytes) hashed to the committed literals first:
digest_stream_prefix 588a0c69..., _blkt f39a41e6..., _input 36eacc43..., _irq 390c34c5..., _model b7a68417..., accept_256k_zero bff30afe..., accept_384k_seeded 5ecef2a4....
A throwaway python script outside the repository, which never called the new core, set the version to 9 and appended PSG0 (42 zero bytes), PSGW (192 zero bytes: zero and seeded-prng-v1 draw no wave) and PSGT (24 zero bytes, then 01 00 00 00 four times). The new literal is shasum -a 256 of each 92,553-byte stream:

| Case | New literal |
|---|---|
| digest_stream_prefix | 1e72766e7786f92e385cc753d9d8d370293836fbe6b1a2932d77df3c3cb6cc20 |
| digest_stream_blkt | bfd2f28481686cc486fde300dec85dc8283087cc66bafe29d187ba789aeb8813 |
| digest_stream_input | 2b4dc9611e262bea08abb922203acfc7ffc32d14474c1db09deffc764111ca69 |
| digest_stream_irq | 8914f1d281168cc4ffcbab4b7759912c3287c981ddef717694333fc45ab0a418 |
| digest_stream_model | 653edd0427a6916dfb10db62ae5f0b5fb3cb6d312703b644dbcd1e12c25605cc |
| accept_256k_zero | 50ad8746bbe272d13ebea964c3d4702f358b29deb44edb55d6e4589b708b0f83 |
| accept_384k_seeded | ba00feeb53d3c434e7e91efc6cd2ab7a9bda0c3cd6f0cab49805c56a00c22b42 |

The goldens' format_version 8, behaviour_epoch 6, the 15 psg_ rows and the 42 withheld ids were written by hand from constants.def and the ledger ids, never from printer output.
The v4 seed 1 wave literal (bytes 0..15 080d150c1b1903020e0a1b190a141103, SHA-256 0a36d3cc...) came from an independent splitmix64 script. That script first reproduced the committed ram@0 literal 37db8e2e... of poweron_v3_domain_independence (R7).

## R3 and R4 results (scratch copies, never committed)

- R4, D-203 rule lines deleted one at a time from snapshot.c (16 mutants):
  - snapshot_refusal_psg went red for each of select, lfo_ctl, freq, ctl, widx, dda, DDA-with-widx, noise[0], noise[1] and wave.
  - snapshot_refusal_psgt went red for each of frozen-with-rem, running range, channel grid, noise range, noise grid and LFSR range.
  - The noise[0] line was at first not killed (p-nz20 edits noise[1] only); the added row p-nz40 kills it.
- R4, dec_powr bound deleted: snapshot_refusal_fields red (policy 6 then reads as profile, not field).
- R4, CROWS_MAX back to 40 with 41 rows: record_constants_shown harness_failure (64 admits the same 41 rows).
- R3 mutants of psg.c:
  - tap expression (s >> 11 -> s >> 10): psg_poweron red
  - select mask (& $0F): psg_decode red
  - DDA reset statement deleted: psg_wave_ptr red
  - key-on alignment (no - stamp mod d): psg_wave_ptr and psg_lfo_trg red
- D-223: the PROF identity refuses a psg_tick_divisor=3 snapshot in a default instance as snapshot.profile. psg_restore_divisor is the witness, so no new refusal row was needed.

## Gate results (end of plan, one at a time, at 4515695)

| Run | Result | Wall | User CPU | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check` | PASS, CHECK RESULT PASS | 665 s | 429.0 s | about 22 to 31 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 175 PASS lines, 0 FAIL | 102 s | 38.1 s | |
| `sh scripts/audit-history --range 2ab40cd..HEAD` | clean (3 commits, 55 blobs, 2,124 blob-path pairs) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | LINUX_REPRO gate-gcc PASS (gcc 13.3.0, mawk 1.3.4, Ubuntu 24.04.5, strict C11 -Werror) | 75 s | | |
| full gate before the Task 1 commit | PASS | 2,359 s | 450.2 s | about 14 |
| full gate before the Task 2 commit | PASS | 863 s | 543.0 s | about 25 to 33 |

Task verifies: Task 1 VERIFY_OK, Task 2 VERIFY2_OK, Task 3 VERIFY3_OK, each run exactly as written in the live checkout before its commit.

## Cost after the bump (F-79, for the 05-04 comparison)

- Conformance stage alone at 4515695: 74.6 s user CPU, 81.4 s wall, load about 15 (05-01 baseline 67 s at load 5.7 to 9.1). The noise fold now runs in every step.
- Full gate user CPU: 429.0 s at load 22 to 31, against the 05-01 baseline of 270.0 s at load 5.7 to 9.1. The load is not comparable (other work on the machine pushed it to 14 to 33 all day; the two earlier full gates of this plan gave 450 s and 543 s). Cut trigger (3) needs a measurement at a comparable load before it can be judged; 05-04 should re-measure on a quiet machine.

## Live budget table (after 05-03)

| Budget | Value after 05-03 | Phase 5 target |
|---|---|---|
| Gate stages | 57 | 58 |
| Selftest PASS lines | 175 | >= 178 |
| Power-on policies | 5 (done) | 5 |
| Constants rows / CROWS_MAX | 49 / 64 (done) | 49 / 64 |
| Withheld ids | 42 (done) | 42 |
| Record rows (skeleton / 16 events / cap) | 175 / 191 / 192 | 175 / 191 / 192 |
| Regions; serial / epoch / text | 24; 9 / 6 / 8 (done) | 24; 9 / 6 / 8 |
| Snapshot / instance / request bytes | 92,553 / 1,141,376 / 72 (done) | same |
| Golden bytes (256k / 384k) | 8,557 / 8,601 of 16,384 | below 16,384 |
| OX_ROMS_MAX / guest ROMs | 48 / 36 | 48 / 39 |
| DSV ids | 1..26 | 27, 28, 29 |
| Toys | 26 | 33 |
| Ledger entries / REQUIRED lines | 75 / 75 | 75 / 75 |
| Conformance REQUIRED cases | 344 (337 + 7) | about 379 to 394 |
| CI critical path | 562 s (4.1 run) | about 587 to 612 s predicted |
| Local warm gate user CPU | 429.0 s at load 22 to 31 (not comparable) | +10 to +20 percent; trigger (3) above 337.5 s at a comparable load |

## Task Commits

1. Task 1: golden compaction, CROWS_MAX 64 - `a9d2c3f` (test)
2. Task 2: the one bump, layout, decoders, fold, constants, literals, hostile rows - `2575af4` (feat)
3. Task 3: decode and storage, effects, v4, policy-6 row, psgkinds - `4515695` (feat)

## Deviations from Plan

**1. [Rule 2 - correctness] The closed-form channel catch-up moved from Task 3 to Task 2.** From the bump on, restore accepts a running channel (rem 1..24,576 on the grid). The fold must advance and rebase such a channel; otherwise a restored running counter drifts off the grid at the next step end, and its next snapshot would be refused. Commit 2575af4.

**2. [Rule 2 - R4] The hostile rows landed with the rules in Task 2, in two new cases.** R4 asks every restore rule to land with a row that fails when the rule line is deleted.
The expect line of snapshot_refusal_fields has no room under the 400-character rule. So 34 rows (p-*) went into two new cases: snapshot_refusal_psg (registers and wave) and snapshot_refusal_psgt (counters and LFSRs), with two REQUIRED lines (files outside files_modified).
snapshot_hostile and snapshot_refusal_coverage needed no expect change: the hostile kind walks the frame table, which gained PSG0, PSGW and PSGT, and no refusal code was added. Commit 2575af4.

**3. [Rule 1 - predicted family] Snapshot edits that move TIME keep the noise distances on the grid.** win_set and ed_time_below now call sk_psg_follow_time, so the accept rows of snapshot_refusal_api and snapshot_refusal_window judge only their own rule. Commit 2575af4.

**4. [Rule 1 - test] psg_decode folds to the stamp before it judges a reg $A..$F write.** The first write at time 0 folds the noise event due at time 0 (the LFSR steps). The row now folds first, so it judges the write alone. Found in the scratch run before the commit.

**5. Golden size.** The compaction left both goldens near 7.2 KB, against the plan's "near 11 KB". Every element the plan names is kept, and the headroom is larger (7.8 KB after the bump).

**6. Commit trailer.** All three commits end with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`, as the standing brief requires.

**7. Out of scope, not changed.** scripts/check line 2261 is a comment that still says the guest default list is seeded-prng-v3 seeds 1 and 2; the list is v4 since 4515695. Logged for 05-05, which owns scripts/check.

Requirement ticks: none in the plan commits; any AUD tick made by the plan-complete step is reverted with a file-specific checkout of .planning/REQUIREMENTS.md (only 05-10 completes AUD ids).

## Reliance on the unreleased epoch

Epoch 6, serial 9 and text 8 are not released (no push). Under R11 a behaviour change before the first push needs no further bump; after a push it needs one (D-202).

## Known Stubs

- ds_psg_advance takes obs but uses none of it, and bus.c drops the decoded fields of ds_psg_write. The plan says DSPE emission and synthesis land in 05-04.
- LFO coupling and F0' land in 05-09; until then an R9 write with CTL != 0 is flagged UNMODELLED.
- None of these blocks this plan's goal.

## Threat Flags

None beyond the plan's threat model:
- T-05-08 and T-05-09: 34 hostile rows, each D-203 line shown red when deleted
- T-05-10: one mask table, NO_TARGET rows
- T-05-11: OLD literals re-verified, frames hashed outside the repository
- T-05-12: psg_restore_divisor

## Notes for next plans

- 05-04: bus.c already holds `struct ds_psg_write_info ev;` and `(void)ds_psg_write(inst, offset & 0x0Fu, value, stamp, &ev);` between the fold and log_dev. Emit ds_obs_psg from ev there.
- 05-04: ds_psg_advance takes obs but uses none of it. The sample loop must interleave with the noise events (the fold processes them in time order, select 4 before select 5 at a tie) and with the closed-form channel steps (today per channel, after the noise loop).
- 05-04: stored distances are offsets from B = machine_time - 1; chan_start writes (stamp - B) + d x Fe - stamp mod d. Tests that call ds_psg_advance directly (psgkinds) keep machine_time 0 and fold without a rebase.
- The default guest list is v4 seeds 1 and 2, so every guest ROM now boots with a drawn wave RAM under the seeded rows.
- psgkinds.c is the home of the later C-level kinds (psg_fold and others); runner.h has a psgkinds block after kind_vdc_regmask.
- Cost: 05-04's fold-cost comparison needs a quiet-machine baseline; this plan's numbers are at load 14 to 33.

## Owner items

- None new. Carried: lever (a) commit 9619efe awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

Files: src/core/psg_effects.def, tests/conformance/psgkinds.c and the seven new case files exist. Commits a9d2c3f, 2575af4 and 4515695 are ancestors of HEAD.
