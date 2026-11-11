---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 31
subsystem: w7-hostile-snapshot-corpus-and-restore-controls
tags: [snapshot, restore, hostile-corpus, random-programs, toy-controls, selftest, sanitizers, w7, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: ds_snapshot, ds_restore (validate-then-commit), the tiling list, snapkinds.c (stream layout copy, sk_rows, poison and member compare), the 32 random programs of randprog.c and the chunk_equiv kind (03-26, 03-30)
provides:
  - "kind snapshot_hostile (snapkinds.c) and the REQUIRED case snapshot_hostile: 529 hostile snapshot inputs generated in memory from one snapshot of a machine inside a block transfer (boundary 92, length 56, frame 77, sample 64, flip 216, data 24), 437 refused with the target byte-identical and 92 accepted, re-serialised identically and run for a bounded time; counts by class in the evidence text; the sanitizers stage runs it"
  - "chunk_equiv item restore=N and the case chunk_equiv_random32 with restore=3: three stops per random program drawn from a fixed splitmix64 stream, snapshot, restore into a fresh poisoned instance, raw member compare, run on; time, steps, verdict RAM, digest and stop sequence equal the uninterrupted run (96 restores, 20 inside a block transfer, 31 at the second speed)"
  - "toys snapshot-drop-t and snapshot-drop-blkt (tests/controls) and the selftest mutation-restore-skip-field (scripts/check): the dropped-state controls of D-122 within the toy cap of D-131"
  - "snap_poison and snap_first_diff exported from snapkinds.c through runner.h, shared by chunkkinds.c"
affects: [03-32, 03-34]

requirements-advanced: []
requirements-completed: [CORE-05]

actuals:
  tokens: 13400
  tasks: 3
  commits: 3
plan_head_before: 9dba1d9a16e4ec4812b946ac8081be92ae3142e9
plan_head_after: 0a1d91ddd6ee0c703c2bd5189f72d67e0d2f0b46

tech-stack:
  added: []
  patterns:
    - "a hostile corpus is generated from one good snapshot with the hand-kept layout of the test, never the core's table: each input has a known verdict class (must refuse, must accept, either) and every outcome is checked against it, so a core that refuses everything or accepts everything fails; an accepted input is run for a bounded time and the snapshot of the state that run reached must restore again (a state a run can reach must be restorable)"
    - "a restore schedule inside a chunk-invariance kind: the stop callback state (dedupe by machine time, sequence fold) is carried across the instances, so the stop sequence of snapshot, restore and continue equals the uninterrupted sequence"
    - "a skip-a-field control as a scratch-tree selftest: the unmutated scratch runner must pass the case, then one store of pass 2 is changed to (void)y (the warnings are errors, so a removed store of a local would not build) and the same case must give guest_failure"

key-files:
  created:
    - tests/conformance/cases/snapshot_hostile.toml
    - tests/controls/snapshot-drop-t/RULE
    - tests/controls/snapshot-drop-t/EXPECT
    - tests/controls/snapshot-drop-blkt/RULE
    - tests/controls/snapshot-drop-blkt/EXPECT
  modified:
    - tests/conformance/snapkinds.c
    - tests/conformance/chunkkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/cases/chunk_equiv_random32.toml
    - tests/conformance/REQUIRED
    - scripts/check

key-decisions:
  - "Toy anchors are single lines of the restore in snapshot.c (inst->p = p; for T, inst->blk_active = active; for BLKT), not the emit lines of serial.c that the plan names: the emit lines are shared with the digest, so a change there changes every digest and golden and is not a restore defect; the live serial.c stays untouched and the toy's snapshot is still the digest stream"
  - "EXPECT of snapshot-drop-t is snapshot_restore_set only; EXPECT of snapshot-drop-blkt is snapshot_restore_block, snapshot_hostile and chunk_equiv_random32. The plan names snapshot_restore_sampled for both, but that case is ds_proto at every 97th boundary, which has no T-set boundary and no block transfer; each EXPECT names only cases that were measured red on the toy for the reason the toy models (the member compare names p, or blk_active)"
  - "The hostile source is the cpu_blk_len0 fixture stopped inside its 65536-byte transfer (until_mclk 1000000, asserted: active, 0 < remaining < 65536) with the pad_2btn_correct input script and an interrupt stimulus list declared at times past the stop, so BLKT, IRQS, SCHD and IOPT are all live in one snapshot; the case needs no second source"
  - "Corpus size 529 (the plan says about 500): boundary 92 (31 distinct boundaries, three lengths each), length 56 (4 values per frame), frame 77 (prefix 15, version 4 and its two extremes, 56 tag bytes), sample 64 (as the plan), flip 216 (low bit of every control byte: profile hash, content id, policy, time, P, speed, tma_zero, timer, IRQC, three stimulus slots, BLKT, IOPT, schedule count and reserved word and three events), data 24 (RAM, mapping registers, A X Y S pc, bus latches: every value legal, so expected accepted)"
  - "Restore schedule stops are drawn in the middle half of each of N+1 equal parts of the run (increasing, a quarter part apart), per program from rp_seed(index) xor a fixed salt; the target boots from the same image and request as the source (the random32 case uses the default request, whose power-on policy has no seed to vary), and its restorable members are poisoned 0xA5 and 0x5A alternately"

patterns-established:
  - "a new restore-relevant control comes with its own bite: scratch edits of snapshot.c (a pass 1 that writes before validating, a frame length check that lets +1 through, a restore that drops blk_remaining) were each shown to turn the new cases red and were restored byte for byte"

duration: 75min
completed: 2026-11-11
---

# Phase 3 Plan 31: Hostile snapshot corpus, restore at random stops, dropped-state controls Summary

**529 hostile snapshot inputs generated in memory around every region boundary, frame length, tag, sampled truncation and control field of a snapshot taken inside a block transfer are each refused with the target byte-identical or accepted and re-serialised to identical bytes (and then run safely), the 32 random programs survive three snapshot-and-restore stops each into fresh poisoned instances with the uninterrupted run's digest and stop sequence, and dropping the T flag or the block transfer state in restore, or skipping a field copy, turns the gate red.**

## What was built

- **Task 1 (tracer), commit 42a7750.** `kind_snapshot_hostile` in `snapkinds.c`, registered in `runner.c` and `runner.h`, the case `snapshot_hostile` (REQUIRED) with the boundary class only (92 inputs): the length of the buffer at every region boundary (prefix, version, each frame and payload, the end of the stream) and one byte either side. Verdict per input: refused (code in the closed set, target byte-identical to its copy from before the call, ROM included) or accepted (exact stream length, re-serialises identically, SHA-256 of the input equals the digest of the target). The source is cpu_blk_len0 stopped inside its block transfer with the pad_2btn_correct input and an interrupt stimulus declared so that every region is live; the kind asserts the stop is inside a transfer. `snap_poison` and `snap_first_diff` were made non-static for sharing. Tracer gate: `--stage conformance` and `--stage sanitizers` both green ("Tracer verified end-to-end - expanding").
- **Task 2, commit 3f4c891.** The classes length, frame, sample, flip and data (529 inputs in all, counts by class in the evidence text through `set_evidence_detail`). Every accepted input is also run for a bounded 3000 master clocks and the snapshot of the state that run reached must be accepted by restore and give the same state member for member. `chunk_equiv` gained `restore=N` (`cs_exec_restore`): N stops drawn from a fixed stream per program, snapshot, restore into a freshly booted and poisoned instance of the same image, raw compare with the source, run on with the same stop callback state; `chunk_equiv_random32` has `restore=3`. A vacuity guard requires, for 16 or more programs, at least one restore stop inside a block transfer and one at the second speed.
- **Task 3, commit 0a1d91d.** `tests/controls/snapshot-drop-t` and `snapshot-drop-blkt` (RULE and EXPECT) and the selftest `mutation-restore-skip-field`.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage conformance`, `--stage sanitizers`; Task 2 `--stage conformance`, `--stage sanitizers`; Task 3 `--stage conformance`, `--selftest mutation-restore-skip-field`, `--selftest mutation-control-anchor` (14 PASS lines, including both new toys).
- Plan end: `sh scripts/check --timing` PASS (4 min 30 s wall under a load average of 20 to 25; conformance 42 s, sanitizers 83 s, determinism 30 s, named-controls 10 s, audit-synthetic 10 s; the stages that warn at 20 s are unchanged), `sh scripts/check --selftest` PASS (103 PASS lines, no FAIL, 50 s: the 100 of 03-30 plus mutation-restore-skip-field and one mutation-control-anchor line per new toy), `dash scripts/check --stage conformance` PASS, `sh scripts/audit-history --range 9dba1d9..HEAD` clean (3 commits, 14 blobs). All 13 toys are RED_OK (the conformance stage runs them; the 8 D-131 toys are now complete: block-progress-lost, trace-hook-mutates, snapshot-drop-t, snapshot-drop-blkt, adc-carry, unmapped-value, timer-reload-off, irq-priority-swap, plus shared-input and the four Phase 2 toys).
- Cost: the hostile case takes under 1 s on the gate runner and about the same under ASan and UBSan (529 restores and 92 bounded runs); the restore schedule adds about 1 s to chunk_equiv_random32 (3.2 s for the whole case on the gate runner). `.github/workflows/gate.yml` unchanged.
- Bite, shown with scratch edits of src/core/snapshot.c restored from a saved copy and `cmp`-checked (nothing committed): a pass 1 that stores `blk_src` before validating turns snapshot_hostile red ("a refused hostile input changed the target"); a frame length check that accepts lengths above the table's turns it red (length and frame inputs accepted and not re-serialising); a restore that does not store `blk_remaining` turns chunk_equiv_random32 red with the member named (`blk_remaining differs after the restore`) at several drawn stops.
- Toy reds were measured on the built toys, one case at a time: on snapshot-drop-t only snapshot_restore_set goes red (member `p` differs at boundaries after SET); on snapshot-drop-blkt snapshot_restore_block, snapshot_hostile, snapshot_refusal_api and chunk_equiv_random32 go red (the other restore cases pass because their fixtures have no block transfer in progress at a stop).

## Deviations from Plan

**1. [Plan wording] Toy anchors are lines of snapshot.c, not of serial.c.** The plan says the anchors are the emit lines in serial.c, and so did the orchestrator's brief for 03-30 (`w_u8(w, inst->p);` is unchanged). A toy there would change the digest stream and so every digest literal and golden, which is a different defect from a restore that drops state. The anchors are `inst->p = p;` (dec_cpu0) and `inst->blk_active = active;` (dec_blkt); the replacements are `inst->p = (uint8_t)(p & 0xDFu);` and `inst->blk_active = 0u;`. The plan's "replacement emits a zeroed payload" is a restore that stores the active flag as 0 (the other five stores stay, which keeps the toy compiling and the stream valid).
**2. [Plan wording] EXPECT cases differ from the plan's.** The plan names snapshot_restore_sampled for both toys; that case (ds_proto, every 97th boundary) has neither a T-set boundary nor a block transfer, so it passes on both toys. EXPECT names the cases that are red by construction and were measured red (see Verification).
**3. [Plan wording] snapshot_hostile uses one source and one case, 529 inputs.** The plan says about 500; the class sizes above sum to 529. `tests/conformance/cases` gained only snapshot_hostile.toml; chunk_equiv_random32.toml was edited, not added.
**4. [Rule 1 - found while writing the selftest] The skip-field selftest replaces the store with `(void)y;` instead of deleting it.** The gate preset builds with -Werror and an unused local `y` does not compile; deleting the line made the mutated scratch tree fail to build, which is a harness failure and not a red case. The mutation still removes the field copy of pass 2.
**5. [Rule 3 - files outside the plan list]** runner.c and runner.h (the kind registration and the two shared helpers) and chunk_equiv_random32.toml's comment and meaning text beyond the files named. No Rule 4 deviation.
**6. [Process]** The per-plan commit ledger (`gsd-plan-head-before-03-31`) of protocol 0c was not created; the plan start commit 9dba1d9 is recorded here and `commits: 3` is measured with `git rev-list --count 9dba1d9..HEAD` before this SUMMARY and the docs commit.

## Evidence discipline

E0 throughout: the core compared with itself and with the hand-kept layout of the test (the verdict class of each hostile input is derived from the layout, not from DuoSpool output; no literal comes from a DuoSpool run). A refused or accepted verdict says the input was handled consistently and safely, not that the snapshot format is right; a restored random program continuing like the uninterrupted one says the snapshot is complete for those programs, not that the programs mean anything. No ledger entry (D-121) and no accuracy, compatibility, cycle-accuracy, hardware or boot claim. No snapshot, ROM-like binary or corpus file is committed or written: the corpus lives in memory for the duration of the case (D-133, D-76).

## Known Stubs

None. `.planning/WINDOWS.md` is absent, so nothing was appended.

## Threat Flags

None beyond the plan's register. T-03-86 (memory safety in restore): 529 inputs under ASan and UBSan in the sanitizers stage, accepted inputs re-serialise identically and run for a bounded time; T-03-87 (incomplete state hidden by passing restores): two toys and one selftest mutation turn red, plus the tiling assert of 03-30; T-03-88 (gate time): the corpus is bounded and in memory, timings above.

## Notes for next plans

- **03-32 (ledger):** nothing for the ledger from this plan; CORE-05 is complete (requirements-completed). Snapshot and restore evidence stays E0 with no ledger entry.
- **Hostile corpus and layout changes:** the generator uses `sk_frames`, `sk_frame_off` and `sk_payload_off` (hand-kept) and the control-field table `hs_ctl` (payload offsets): a new region or field needs its row there as well as in `sk_frames` and `SK_EXPECTED_SIZE` (the case checks the sum against ds_snapshot_size).
- **Toy anchors:** `inst->p = p;` and `inst->blk_active = active;` must each occur exactly once in snapshot.c (control-mutate and mutation-control-anchor enforce it); the skip-field selftest needs exactly one `        inst->y = y;` line in pass 2 of dec_cpu0.
- **Restore schedule in chunk_equiv:** T-flag stops never occurred among the 96 drawn stops (the random programs rarely stop right after SET); the T flag is covered by snapshot_restore_set. The case requires block and second-speed stops only.
- **Cost:** the whole gate was 4 min 30 s under a load average above 20; stage times are listed above and were not weakened or skipped.
- **Owner-gate items:** none pending from this plan.

## Self-Check: PASSED

Created files present: tests/conformance/cases/snapshot_hostile.toml, tests/controls/snapshot-drop-t/RULE and EXPECT, tests/controls/snapshot-drop-blkt/RULE and EXPECT. Commits 42a7750, 3f4c891 and 0a1d91d are ancestors of HEAD.
