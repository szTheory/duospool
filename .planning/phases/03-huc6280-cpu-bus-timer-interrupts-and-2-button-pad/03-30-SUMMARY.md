---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 30
subsystem: w7-snapshot-and-restore-tracer
tags: [snapshot, restore, digest-stream, validate-then-commit, tiling-list, poisoned-target, lockstep, per-region-perturbation, host-hash, refusal-rows, snapshot-refusals, w7, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the region table and the canonical digest stream with its 14 regions at serial version 7 (03-03, 03-16, 03-22, 03-25, 03-29), the observer sinks and the single-step run (03-10, 03-19), the input schedule and the pad fixture (03-27, 03-29), the model parameters (03-17), the fixtures and the case/kind machinery (03-02 to 03-29), the toy controls and their EXPECT-only runs (quick task 261111-e8l)
provides:
  - "ds_snapshot_size (25021 bytes at serial version 7), ds_snapshot (the digest stream into a caller buffer through ds_digest_stream, so sha256(snapshot) equals the digest) and ds_restore (two passes over the caller's buffer: pass 1 validates and writes nothing, pass 2 stores and cannot fail) in src/core/snapshot.c, ds_snapshot.h and snapshot_refusals.def (14 codes, prefix snapshot.)"
  - "ds_serial.h DS_REGION_COUNT, ds_region_frame and ds_stream_prefix: the restore reads the prefix, tag order and lengths from the one region table of serial.c, so there is no second layout"
  - "DS_INSTANCE_TILING in ds_core.h: a hand-written list of every member of struct ds_instance with a sum static assert against sizeof, and the explicit member tail_pad[6]"
  - "tests/conformance/snapkinds.c: kinds snapshot_hash, snapshot_restore, snapshot_refuse and the deferred snapshot_refusal_coverage"
  - "REQUIRED cases snapshot_equals_digest, snapshot_restore_sampled, snapshot_restore_boundaries, snapshot_restore_block, snapshot_restore_set, snapshot_restore_irq, snapshot_restore_speed, snapshot_restore_pad, snapshot_restore_timer, snapshot_refusal_rows, snapshot_refusal_fields, snapshot_refusal_schedule, snapshot_refusal_api, snapshot_refusal_coverage"
  - "conformance-run snapshot-host-hash EVIDENCE_DIR (SNAPSHOT_HOST_HASH_MISMATCH), called once by the run and sanitize modes; selftests mutation-snapshot-host-hash and mutation-tiling-entry"
affects: [03-31, 03-32, 03-34]

requirements-advanced: [CORE-05, CORE-04]
requirements-completed: []

actuals:
  tokens: 35300
  tasks: 3
  commits: 5
plan_head_before: c2198f810769d9e45129093da52be447417791a3
plan_head_after: b1984de80e0e4caaa03db8f15d86421d208c12e8

tech-stack:
  added: []
  patterns:
    - "restore evidence that does not use the serializer's table: a hand-kept copy of the stream layout in the test (prefix, version, 14 tags and lengths), a raw member-by-member compare over a hand-written tiling list with ROM excluded, a target booted with another seed whose restorable members are poisoned (0xA5 and again 0x5A, so a forgotten field cannot equal both), and lockstep against an uninterrupted reference run that records machine time, pc, opcode and a fold of the instruction and device records of every step"
    - "one decoder per region that both validates (commit 0) and stores (commit 1) from the same reads, so the two passes cannot disagree about where a field is; the decoders are the mirror of emit_payload and are guarded by the round trip (restore, snapshot, equal bytes) and the poisoned compare"
    - "stops are chosen from the reference trace by opcode and flag (after SET with the T flag set, after CLI, PLP, RTI and SEI, a boundary at each speed, after a pad write followed by a pad read, inside a block transfer including the boundary with no byte left) and a class that selects no boundary fails the case, so the evidence cannot be vacuous"
    - "a host-tool step outside the runner: the case writes the snapshot bytes and the digest it printed into the evidence directory, conformance-run hashes the file with sha256sum or shasum -a 256 and compares, and a selftest flips one byte in a scratch copy of the evidence and requires the mismatch code"

key-files:
  created:
    - src/core/snapshot.c
    - src/core/ds_snapshot.h
    - src/core/snapshot_refusals.def
    - tests/conformance/snapkinds.c
    - tests/conformance/cases (the 14 cases named above)
  modified:
    - src/core/ds_core.h
    - src/core/ds_serial.h
    - src/core/serial.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - CMakeLists.txt
    - scripts/conformance-run
    - scripts/check

key-decisions:
  - "Refusal set (own prefix snapshot., F4): argument, overlap, capacity, truncated, trailing, prefix, version, tag, region_length, profile, content, field, time, reserved (14 rows, 16 slots). ds_restore checks the arguments, the overlap of the buffer with the instance (a restore would read bytes it is about to overwrite), the length, the prefix, the version, then region by region the tag, the frame length and the fields"
  - "Restore is accepted onto an instance whose profile hash (PROF), content id, size class and size (CONT) are the stream's, and whose profile binds the stream's power-on policy; POWR (policy, seed, seed flag), TIME, RAM8, MPRS, CPU0, BUS0, TIMR, IRQC, IRQS, BLKT, IOPT and SCHD are stored. mapper, byte_order and header are boot-derived, in no region, and are not restored; the ROM is excluded"
  - "Field domains: TIME below 2^63 (time); P with the B bit clear (reserved) and speed 0 or 1; tma_zero and irq_i_sample below the number of values of their allowed lists (counted from the list, not hard-coded); timer counter and reload at most 127 and the start bit 0 or 1; irq_disable at most 7 and the TIQ latch 0 or 1; BLKT all zero when inactive, otherwise opcode one of $73 $C3 $D3 $E3 $F3, alternation state 0 or 1 and 0 for TII, TDD and TIN, remaining 0 to 65536 (0 is legal while active: the epilogue is next); IRQS a declared list (lines 1 and 2, levels 0 and 1, times that do not decrease) followed by all-zero slots; IOPT port 1 the device that the profile names and ports 2 to 5 empty; SCHD at most 1024 events, each on a port with a pad behind it judged against the IOPT of the same stream, masks of the two-button pad without opposite directions, the order of ds_input_push, every unused slot zero, the reserved word 0 (reserved)"
  - "The timer origin is not bounded: the closed form only subtracts it from a later stamp when that is above it, so no value can wrap. The restored time of 2^63 - 1 is the largest accepted; a run to a deadline of 2^64 - 1 from there is refused as run.time_wrap with the instance unchanged and a short run takes steps (T-03-84), checked in snapshot_refusal_api"
  - "The target of a restore is booted with the twin request: the seed replaced (7, or 8 when the source's is 7), the model.NAME items dropped (so the model parameters are at their defaults) and no schedule or stimulus; every case uses the seeded-prng-v2 policy so that the seed can differ. The poison covers every restorable member of the tiling list; identity members (profile, content id, mapper, size class, byte order, header, ROM, tail) are not poisoned because pass 1 checks or boot derives them"
  - "Per-region perturbation runs once per case at the first lead stop past the boot state, on the restored instance (A, X, Y and S differ there and the block, timer and port state are live, which the boot state cannot show): spans up to 16 bytes are flipped byte by byte, longer ones at the first and last byte (PROF at the first only), through the accessor of the table the digest walks. At least one restored stop per case must have A, X, Y and S pairwise different"
  - "The host hash step is called by both the gate and the sanitize modes of conformance-run on their own evidence directory (the snapshot case runs in both), after the hash sweep; it is also the mode snapshot-host-hash for the selftest. No runner option was added, so no runner invocation needed a new argument (conformance-run run and controls, determinism-run unchanged)"
  - "Sampled cases use their own fixture and deadline (the first deadline past the guest's done state that a probe found: ds_proto 300000, cpu_blk_len0 5900000, cpu_flow 400000, irq_ctl 5900000, spd_speed 600000, pad_2btn 800000, tim_timer 600000); the boundaries case uses ds_proto to 150000 (about 3000 boundaries, one poison byte per boundary alternating, window 4)"

patterns-established:
  - "a new region or instance member needs three edits and each is guarded: the member in the struct, an entry in DS_INSTANCE_TILING (a missing entry breaks the build, selftest mutation-tiling-entry), a region and a decoder (the poisoned compare of the snapshot_restore cases fails when a restorable field is not stored)"

duration: 110min
completed: 2026-11-11
---

# Phase 3 Plan 30: Snapshot equals the digest stream, validate-then-commit restore, restore evidence at every class of stop Summary

**A snapshot of the machine is the 25021-byte canonical digest stream (its SHA-256 is the digest, the ROM is not in it), ds_restore puts it back with a validate pass that writes nothing and a commit pass that cannot fail, a refusal leaves the instance byte-identical, and a fresh instance booted with another seed and poisoned twice (0xA5, 0x5A) ends equal to the source in every member except the ROM and continues in lockstep with the uninterrupted run at the first, last and every k-th boundary, inside a 65536-byte block transfer, after SET, CLI, PLP and RTI, at both speeds, with a running timer, between a pad write and the next pad read (empty target schedule), and at every instruction boundary of ds_proto; the host `shasum -a 256` of the snapshot file equals the digest the core printed.**

## What was built

- **Task 1 (tracer), commit 817ca49.** `snapshot.c`, `ds_snapshot.h`, `snapshot_refusals.def`; `DS_REGION_COUNT`, `ds_region_frame` and `ds_stream_prefix` in `ds_serial.h` and `serial.c` (the restore reads the frame from the one region table, the `w_u8(w, inst->p);` line and the table rows are untouched); the tiling list and the `tail_pad[6]` member in `ds_core.h`; `tests/conformance/snapkinds.c` (all four kinds; Task 2 and 3 only added cases and sampling) registered in `runner.c`, `runner.h` and `CMakeLists.txt`; cases `snapshot_equals_digest` and `snapshot_restore_sampled`; `conformance-run snapshot-host-hash` and the selftest `mutation-snapshot-host-hash`. Tracer gate: `--stage build`, `core-boundary`, `conformance` and `--selftest mutation-snapshot-host-hash` all green before Task 2 started ("Tracer verified end-to-end - expanding"). snapshot_equals_digest checks the size against a hand-kept table (not the core's), that the call writes exactly that many bytes (a canary after the buffer), the prefix, version, every tag and length, that TIME, CPU0 and RAM8 are the instance's members read raw, sha256(snapshot) == digest, and that flipping a ROM byte leaves the snapshot unchanged; it writes `snapshot_equals_digest.snapshot` and the lines `digest=`, `snapshot_bytes=` and `snapshot_file=` beside the evidence text.
- **Task 2, commit f6f5571.** Cases `snapshot_refusal_rows` (the plan's thirteen faults), `snapshot_refusal_fields`, `snapshot_refusal_schedule`, `snapshot_refusal_api` (frame length, NULL buffer, overlap, capacity, and eight legal edges that must be ACCEPTED and re-serialise to exactly the edited bytes, so a core that refused everything fails) and the deferred `snapshot_refusal_coverage` (every code of the .def observed); 53 rows in all. For every refused row the target (a poisoned twin) is compared byte for byte over the whole instance ROM included, and member by member. Selftest `mutation-tiling-entry` (a one-byte member added to a scratch copy of ds_core.h breaks the compile with the tiling message; the unmutated copy compiles).
- **Task 3, commit 5d0b52b.** `snapshot_restore_sampled` widened (first, last, every 97th, window 32, finish, perturb), `snapshot_restore_boundaries` (every boundary of ds_proto), `snapshot_restore_block` (cpu_blk_len0: after the prologue with 65536 bytes to move, the middle, the last active boundary, the boundary with no byte left), `snapshot_restore_set` (cpu_flow: after SET with the T flag set, after CLI, PLP, RTI), `snapshot_restore_irq` (irq_ctl under model.irq_i_sample=post with the ten-event stimulus: after CLI, RTI, SEI; the target has the default model and no stimulus), `snapshot_restore_speed` (spd_speed: the median boundary at each speed), `snapshot_restore_pad` (pad_2btn correct script: right after a $1000 write, the window running through the next read of the pad window, the target schedule empty), `snapshot_restore_timer` (tim_timer, every 61st boundary with the timer running). Commit 2dcc7b2 adds the T-03-84 check to snapshot_refusal_api.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `--stage core-boundary`, `--stage conformance`, `--selftest mutation-snapshot-host-hash`; Task 2 `--stage build`, `--stage conformance`, `--stage sanitizers`; Task 3 `--stage conformance`, `--stage sanitizers`.
- Plan end: `sh scripts/check --timing` PASS (3 min 38 s wall under a load average above 14; sanitizers 76 s, determinism 28 s, named-controls 12 s), `sh scripts/check --selftest` PASS (100 PASS lines, no FAIL, 36 s: the two new selftests added to the 98 of 03-29), `sh scripts/audit-history --range c2198f8..HEAD` clean (4 commits, 33 blobs at that run; the fifth commit is one case line), `dash scripts/check --stage conformance` PASS. All eleven toy controls stay RED_OK (the conformance stage runs them: run.c and the instance members they anchor on are unchanged apart from tail_pad).
- Bite, shown with scratch edits of src/core/snapshot.c restored from a saved copy and `cmp`-checked (nothing committed): a restore that does not store `y`, `inst->p` without the T bit, `blk_remaining`, `input_count`, `irq_stim_at`, `irq_i_sample`, `timer_origin`, `speed`, `powerup_seed` or `mpr` each turn the case that targets that field red with the member named (`snapshot_restore_sampled`, `_set`, `_block`, `_pad`, `_irq`, `_timer`, `_speed`, `_boundaries`); a pass 1 that writes (`machine_time = t` in the validate branch) turns `snapshot_refusal_rows` red through the byte-identity check; a pass 1 without the speed check turns the row `speed-2` red.
- Cost: the new cases add about 3 s of user time to the gate runner and about 12 s to the sanitize runner under load (the per-step runs with both sinks dominate); the reference, source and target runs are single steps of ds_run, so a boundary costs one snapshot, two restores (one in the alternating mode), a compare and the window. `.github/workflows/gate.yml` unchanged.
- gcc is not installed on this machine (`gcc` is Apple clang), so the tiling expressions were compiled with clang only; they are the standard `sizeof(((struct S *)0)->m)` and `offsetof` constructs of C11. The gate-gcc preset on CI will compile them.

## Deviations from Plan

**1. [Rule 1 - Bug in the plan text] The row "remaining 0 while active" is not a refusal.** The plan, D-121 and the Task 2 behaviour list it among the faults, but blk_remaining is 0 while a transfer is active between the last byte step and the epilogue (cpu.c: the epilogue is taken when it is 0), which a run can stop at; refusing it would make that stop unrestorable and break "legal at every run stop including block bytes". The orchestrator brief said the same (0 legal while active). The thirteenth-row set uses "remaining nonzero while inactive" instead (together with 65537), and `snapshot_refusal_api` accepts "0 while active" and "65536 while active" and proves they re-serialise.
**2. [Rule 2 - needed for the tiling] An explicit padding member was added to struct ds_instance.** The struct is 8-byte aligned and the ROM array ends 6 bytes short of the next multiple of 8, so `tail_pad[6]` (zero from ds_boot's memset, in no region) is the member that lets the tiling sum equal sizeof; no other implicit padding exists (a runtime offset check in snapkinds.c shows the members contiguous).
**3. [Plan wording] Fourteen refusal codes and four refusal cases, not one.** The plan lists thirteen faults for one case; the full set of field and API refusals (53 rows) needs more than the 400-character line cap of a case file allows for one expect line, so the 13 are `snapshot_refusal_rows` and the rest are `snapshot_refusal_fields`, `_schedule` and `_api`, with the deferred coverage over all four. Codes: argument, overlap, capacity, truncated, trailing, prefix, version, tag, region_length, profile, content, field, time, reserved.
**4. [Plan wording] More cases than the plan names.** Task 3 names snapshot_restore_sampled and snapshot_restore_boundaries; the classes need different fixtures, so each class has its own REQUIRED case (block, set, irq, speed, pad, timer). `snapshot_restore_set` uses cpu_flow, not irq_ctl, for SET, CLI, PLP and RTI: irq_ctl has no PLP and tim_timer's only SET is taken with the timer interrupt pending, which clears T in the same step, so the T-set boundary exists only in cpu_flow and cpu_alu.
**5. [Plan wording] The after-i class is named per instruction** (after-cli, after-plp, after-rti, after-sei) so a case states exactly which ones it needs and fails when one never runs.
**6. [Commit layout] snapkinds.c carries all four kinds from the Task 1 commit**; the Task 2 commit holds the refusal cases, REQUIRED rows and the tiling selftest, the Task 3 commit the sampling and its cases. Two small changes were committed separately: 2dcc7b2 (a test of the largest accepted time) and b1984de (the time-max row joins the field rows).
**7. [Rule 3 - files outside the plan list]** `ds_serial.h` (the frame accessors the restore needs) and `scripts/check` (two selftests) beyond the 17 named files; 14 case files in all.
**8. [Plan wording] The perturbation is run once per case** at the first lead stop past the boot state (the plan says per region of a restored instance; every region is flipped, once per case rather than at every stop, to keep the sanitizer stage short).

No Rule 4 deviation.

## Evidence discipline

E0 throughout: the core compared with itself (a snapshot restored equals its source; a restored machine continues as the uninterrupted one). Nothing here claims correctness, accuracy, compatibility, cycle accuracy or hardware behaviour, and snapshot evidence gets no ledger entry (D-121). The expected values come from the hand-kept layout of the test (prefix, version 7, 14 tags and lengths, 25021 bytes), from instance members read raw, from the uninterrupted run of the same core, and from the host sha tool; none comes from the digest literal of a DuoSpool output. A matching host hash proves that the file is the stream that the digest was taken over, not that either is right.

## Known Stubs

None. The hostile corpus of about 500 generated inputs and the toys `snapshot-drop-t` and `snapshot-drop-blkt` (D-122) are plan 03-31. `.planning/WINDOWS.md` is absent, so nothing was appended.

## Threat Flags

None beyond the plan's register: T-03-83 (malformed snapshot): pass 1 checks every tag, length and field domain and writes nothing, rows for each class of fault with the instance byte-identical; T-03-84 (TIME near 2^64): refused at 2^63 and above, and a run from 2^63 - 1 to a deadline of 2^64 - 1 is refused as time_wrap; T-03-85 (partial restore): pass 2 copies only after pass 1 accepted the same bytes and has no failing branch, no allocation, no copy of the instance, and a bite shows a pass 1 that writes turns the identity check red. New surface: the evidence directory receives `snapshot_equals_digest.snapshot`, the state of an authored fixture only (the kind requires fixture authored:NAME).

## Notes for next plans

- **03-31 (hostile corpus, toys):** the stream is exactly the digest stream; offsets inside a region are in snapkinds.c (sk_frames, sk_payload_off) and the row table `sk_rows` is the model for generated edits. The restore commit lines to anchor a toy on are single lines in `snapshot.c`: `inst->p = p;` (dec_cpu0, the T bit is in p), the six `inst->blk_*` stores of dec_blkt, and the `input_count`/`input_reserved` stores of dec_schd. The case `snapshot_restore_set` (T flag) and `snapshot_restore_block` (BLKT) are the cases a `snapshot-drop-t` and `snapshot-drop-blkt` toy must turn red; both fail on a skipped store as shown above, and the toy runner runs only the cases of its EXPECT. A hostile corpus must keep "accepted inputs re-serialise identically" (snapshot_refuse already checks it for accepted rows).
- **New region or member:** add it to the struct, to DS_INSTANCE_TILING (else the build breaks), to the region table in serial.c with its emit, to a decoder in snapshot.c (and `DS_REGION_COUNT`, the hand-kept `sk_frames` and `SK_EXPECTED_SIZE`/`SK_VERSION` in snapkinds.c), and bump DS_SERIAL_VERSION; the digest literals and goldens then need the recomputation procedure of the 03-29 SUMMARY.
- **Boot-derived members:** mapper, byte_order and header are not in the stream; a restore onto an instance booted with another declaration of those (the profile hash must still match) leaves them as booted. Nothing observable depends on them after boot today.
- **03-32 (ledger):** nothing for the ledger from this plan; CORE-05 stays under requirements-advanced until 03-31 adds the liveness toys and the hostile corpus of the same check (D-122); CORE-04 is already complete.
- **Cost:** snapshot_restore_irq and _block are the longest cases (128k and 88k single steps, about 1.5 s each under the sanitizers when the machine is loaded); `until_mclk` of each case is the first deadline past the guest's done state that a probe found, so a fixture that grows needs its deadline checked (the case fails with "not in its done state" otherwise).
- **Owner-gate items:** none pending from this plan.

## Self-Check: PASSED

Created files present: src/core/snapshot.c, src/core/ds_snapshot.h, src/core/snapshot_refusals.def, tests/conformance/snapkinds.c and the 14 snapshot cases under tests/conformance/cases. Commits 817ca49, f6f5571, 5d0b52b, 2dcc7b2 and b1984de are ancestors of HEAD.
