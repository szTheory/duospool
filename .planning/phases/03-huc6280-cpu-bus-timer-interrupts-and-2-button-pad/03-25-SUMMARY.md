---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 25
subsystem: w5-block-transfers-resumable-steps-blkt-region
tags: [block-transfer, tii, tdd, tin, tia, tai, blkt, serial-v6, cpu-blk, stop-resume, block-progress-lost, irq-after-epilogue, w5]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the access stamp rule and the closed-form timer (03-21), the interrupt sample, irq_stim and the digest regions TIMR, IRQC, IRQS (03-22), the constants table and goldens (03-18, 03-23), the three-policy fixture runs (03-24), the guest kind, the mutation sweep and run_chunk_sum (03-02, 03-04, 03-10)
provides:
  - cpu.c: TII, TDD, TIN, TIA and TAI as three kinds of step on instance state - a 14-cycle prologue (latch the six operand bytes, length 0 = 65536, push Y, A, X, PC past the seven bytes), one 6-cycle step per byte (source read in cycle 4, destination write in cycle 6, 16-bit wrap, TIA and TAI add-then-subtract alternation) and a 3-cycle epilogue (pop X, A, Y from memory, clear the state), 17 + 6x cycles in all; the interrupt is sampled once, at the end of the epilogue
  - ds_core.h: instance fields blk_active, blk_opcode, blk_toggle, blk_remaining (u32), blk_src, blk_dst (all 0 when no transfer is active); run.c reports stop_point block_byte for a run that ends inside a transfer
  - serial.c: region BLKT (11 bytes) after IRQS, DS_SERIAL_VERSION 6; constants.def rows block_prologue_cycles 14 (LED-CPU-0008) and block_irq_after_epilogue 1 (LED-IRQ-0007), read by cpu.c
  - fixtures/src/cpu_blk.s (DSV id 13, 54 checks, three images from one source: cpu-blk, cpu-blk-len0 with -D LEN0, cpu-blk-irq with -D BLK_IRQ), cases cpu_blk, cpu_blk_len0, cpu_blk_irq_pending, cpu_blk_stop_resume, mutate_sweep_cpu_blk, _len0 and _irq, digest_stream_blkt; opcode_exercise in mode=require with no exclude list (234 of 234 defined opcodes executed)
  - toy control block-progress-lost (tests/controls, red on cpu_blk_stop_resume)
affects: [03-26, 03-27, 03-29, 03-30, 03-32]

actuals:
  tokens: 23200
  tasks: 3
  commits: 5
plan_head_before: a77c3685cc34c96e9df447098c90bed0ea11657a
plan_head_after: 98f636e1db8d1f0741220409f23903ef6e0146a3

tech-stack:
  added: []
  patterns:
    - "a multi-step instruction keeps its progress in the instance and every step is a whole step of the run loop: the instruction hook sees the transfer once (at the prologue), a stop between any two steps needs no extra mechanism, and run.c tells the caller that the stop was inside a transfer from the instance state alone"
    - "the two bus accesses of a byte step are stamped by setting bus->last before each access (read idx 3, write idx 5, penalty added by the bus), so the existing access stamp rule serves block bytes and the penalty of the VDC window still moves the second access"
    - "an access stamp is pinned to one cycle from inside a guest with two timer measurements, one that ends exactly on a prescaler step (a borrow at a tie counts) and one a cycle short of it: a stamp one cycle off in either direction turns exactly one of them"
    - "one fixture source builds several images that differ only in the literal table: every image runs all the checks, a check whose meaning depends on the image has one table entry per image (conditional LIT lines) and the check macro stays outside the conditional, so the lint's one-use-per-check-number rule holds"
    - "an interrupt scenario inside a long instruction is synchronised on the observable level of a masked source (IRQ1D set, the IRQ1 level of the declared stimulus polled) instead of on the code timing, so the interrupt source declared by the case lands inside the transfer wherever the code before it ended"

key-files:
  created:
    - fixtures/src/cpu_blk.s
    - fixtures/src/cpu_blk.checks
    - fixtures/manifest/cpu_blk.toml
    - fixtures/manifest/cpu_blk_len0.toml
    - fixtures/manifest/cpu_blk_irq.toml
    - tests/conformance/cases/cpu_blk.toml
    - tests/conformance/cases/cpu_blk_len0.toml
    - tests/conformance/cases/cpu_blk_irq_pending.toml
    - tests/conformance/cases/cpu_blk_stop_resume.toml
    - tests/conformance/cases/mutate_sweep_cpu_blk.toml
    - tests/conformance/cases/mutate_sweep_cpu_blk_len0.toml
    - tests/conformance/cases/mutate_sweep_cpu_blk_irq.toml
    - tests/conformance/cases/digest_stream_blkt.toml
    - tests/controls/block-progress-lost/RULE
    - tests/controls/block-progress-lost/EXPECT
  modified:
    - src/core/cpu.c
    - src/core/ds_core.h
    - src/core/run.c
    - src/core/serial.c
    - src/core/ds_serial.h
    - src/core/constants.def
    - scripts/fixture-build
    - tests/conformance/digkinds.c
    - tests/conformance/REQUIRED
    - tests/conformance/cases/opcode_exercise.toml
    - tests/conformance/cases/digest_stream_prefix.toml
    - tests/conformance/cases/digest_stream_model.toml
    - tests/conformance/cases/digest_stream_irq.toml
    - tests/conformance/cases/digest_region.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml

key-decisions:
  - "The transfer state is decoded, not the raw bytes: blk_src, blk_dst and blk_remaining hold what the plan's latched[7] would, because the six operand bytes are read once in the prologue and nothing reads them again; PC is advanced past the instruction in the prologue, so the state needs no instruction address. remaining is a u32 (0 to 65536, the length operand 0 becomes 65536 only in the prologue); remaining 0 while active means the epilogue is next, so there is no separate phase field. Every field is 0 when no transfer is active"
  - "Y, A and X are pushed one level each in the prologue (three levels, S goes down three) and pulled in the epilogue (S goes up three), all through the bus. S8-88 draws S decremented only twice before the X push; the cells used (Y at S, A at S-1, X at S-2) and the net S are the same, and only a stop inside a transfer could see the difference in S (it is a modelling choice of LED-CPU-0008, not shown by any guest check)"
  - "stop_point is block_byte whenever the instance is inside a transfer at the end of a (not refused) call, i.e. after the prologue and after every byte step but not after the epilogue; a stop callback and a deadline both end calls there"
  - "The interrupt sample after the epilogue is the same helper as after any instruction (sample_and_enter, the stamp one cycle before the end of the epilogue, the I flag of the start of the step); constants.def block_irq_after_epilogue 0 would skip it and leave it to the next instruction (cpu_blk_irq_pending check 48 turns red)"
  - "The toy block-progress-lost anchors in src/core/run.c, not cpu.c as the plan says: a step cannot see that a run stops (the stop is the caller's, outside the step, D-92), so only the run loop can lose progress at a stop. The anchor is the line that ends a run at its deadline; the replacement gives the byte just moved back to the remaining count while a transfer is active"
  - "cpu_blk is one source with three images whose check numbers are shared: all images run the 38 common checks (1 to 38), the 4 long-transfer checks (39 to 42), the 8 interrupt checks (43 to 50) and the 4 stamp checks (51 to 54); the long transfer is a TIA of 3 bytes in the plain and irq images and of 65536 bytes (length operand 0) in the len0 image, the interrupt scenario runs with I set in the plain and len0 images (the latched TIQ is read, no handler runs) and with I clear and a declared IRQ2 level in the irq image. The expected literals of 39 to 50 are assembled per image"
  - "run.c takes the opcode-table guard (ds_max_step_mclk, a scan of 256 rows) only when a time is above half the 64-bit range; below it the wrap test cannot refuse. A chunk of one master clock makes about a million calls, and the scan cost 10 s of the case's run; behaviour is identical for every request (the time_wrap controls near 2^64 still scan)"
  - "the cases pin no ledger entry yet: pins is empty because inventory-check requires the ledger entry to name the case back and LED-CPU-0007, LED-CPU-0008 and LED-IRQ-0007 are written by plans 03-26 and 03-32; each case says which entry it is meant to pin"

requirements-advanced: [CPU-05, CPU-01, CORE-04]
requirements-completed: []

duration: 95min
completed: 2026-11-11
---

# Phase 3 Plan 25: Resumable block transfers, the cpu_blk fixture, the BLKT region and the block-progress-lost toy Summary

**TII, TDD, TIN, TIA and TAI now run as a 14-cycle prologue, one 6-cycle step per byte and a 3-cycle epilogue on state in the instance (length 0 = 65536, 16-bit wrap, add-then-subtract alternation, X A Y popped from memory, 17 + 6x cycles, one interrupt sample at the end of the epilogue), a 54-check fixture in three images verifies every mode, the long transfer, the interrupt around a transfer and the stamps of the two accesses of a byte step to one cycle, every chunk list that stops inside a transfer reproduces the one-call run, and the digest has a BLKT region (serial version 6).**

## What was built

- **Task 1 (tracer), commit 3dc7bbe.**
  - `cpu.c`: `block_prologue` (reads the six operand bytes through the bus with every access stamped in cycle 14, calls the instruction hook once, pushes Y, A, X, sets the state and PC), `block_byte` (read in cycle 4, write in cycle 6, the address moves of Table 2-1-1 with 16-bit masks, the TIA and TAI toggle, one line `inst->blk_remaining = inst->blk_remaining - 1u`), `block_epilogue` (pops X, A, Y, clears the state, then `sample_and_enter`) and `block_step`; `ds_cpu_step_obs` goes to `block_step` first when a transfer is active and the five opcodes go to `block_prologue`. `sample_and_enter` is the interrupt sample and entry that every other step uses.
  - `ds_core.h`: the six `blk_*` fields in one padding-free span between `tma_zero` and `rom` with a `_Static_assert` chain. `run.c`: stop_point block_byte from `blk_active`; the guard change above.
  - `fixtures/src/cpu_blk.s`, `.checks`, manifest `cpu_blk`; case `cpu_blk`; case `cpu_blk_stop_resume` (kind run_chunk_sum on `cpu-blk`, chunks 1, 7, 168, 3072 and the rotation, one-call run to 1150000 master clocks past the done state); toy `block-progress-lost`.
  - Tracer gate (auto mode): the Task 1 verify (`build`, `core-boundary`, `fixtures`, `conformance`, `--selftest mutation-control-anchor`) was run on a tree holding only the Task 1 files and passed before the expansion ("Tracer verified end-to-end - expanding"); a scratch core with TIA's destination alternation removed turned cpu_blk red at check 17 first.
- **Task 2, commits de25a81 and b29a74a.** `scripts/fixture-build` gained the `cpu-blk-len0` and `cpu-blk-irq` image lines; manifests for both; cases `cpu_blk_len0`, `cpu_blk_irq_pending` (irq_stim `2000000,1,1`, `2100000,2,1`, `2400000,1,0`: the IRQ1 level is the synchronisation line the guest polls with IRQ1D set, IRQ2 rises 100000 clocks later inside the 294912-clock TII), the three sweeps (the irq sweep with the same irq_stim as the plain case); `opcode_exercise` expect is `mode=require` with no exclude list. The follow-up commit adds checks 51 to 54: a one-byte TIN that writes the timer control register and one that reads the counter, each measured exactly on a prescaler step and one cycle short of it.
- **Task 3, commits 0a1c4b1 and 98f636e.** `serial.c`/`ds_serial.h` region BLKT and version 6; `constants.def` rows and their reader in `cpu.c`; `digkinds.c` checks the BLKT frame, the item `poke=block-mid` writes a transfer in progress and `BLKT:all` flips every byte of the region; the digest literals of `digest_stream_prefix`, `_model` and `_irq`, the new `digest_stream_blkt` and both record goldens were derived outside the C code (see Evidence); the second commit empties the `pins` of the four block cases (deviation 7).

## Verification

- Task verifies as written, all PASS: Task 1 as above; Task 2 `fixture-lint`, `fixtures`, `conformance`, `sanitizers`; Task 3 `conformance`, `record-lint`.
- Plan end: `sh scripts/check` PASS (1 min 52 s wall), `sh scripts/check --selftest` PASS (68 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range a77c368..HEAD` clean (5 commits, 52 blobs, 1414 blob-path pairs, run before this docs commit). The ten toy controls are RED_OK, `shared-state` and `block-progress-lost` included (the standing shared-state note was not triggered: run.c gained a stop_point expression and a guard branch but the machine-time store the toy anchors on is untouched).
- Timing (D-132, warn only): `TIMING_WARN sanitizers` at 26 to 27 s (was 29 s; the block cases add little); conformance 15 s, determinism 8 s, named-controls 5 to 6 s. The images finish at 1.07 million (cpu-blk), 2.41 million (cpu-blk-irq) and 5.79 million (cpu-blk-len0) master clocks; the three guest cases run each 3 times, the sweeps stop at the done state.
- Bite shown with scratch edits of cpu.c (each restored byte for byte and `cmp`-checked, nothing committed): TIA destination without alternation turns check 17 red, TAI source without alternation check 22, TIN destination incrementing check 14, TDD running upwards check 10 (the first version of check 10 read the byte at R+8 and did NOT see this mutation: it was moved to R+4), pushes in the other order check 6, no pops check 32, length 0 left as 0 check 39 on the len0 image, a byte step of 7 cycles check 36, an interrupt sampled after every byte turns cpu_blk_irq_pending red (timeout), no sample after the epilogue (the constant at 0) check 48, a prologue of 13 or 15 cycles checks 52 and 51, a read in cycle 3 or 5 checks 53 and 54, a write one cycle early or late checks 52 and 51. With the constant block_irq_after_epilogue at 0 and block_prologue_cycles at 13 in the def the goldens print other values and the cases above turn red.
- Manual facts: H8-6 (Table 2-1-1, length 0 = 65536, 6 bus cycles per byte, 17 + 6x, A X Y saved in the stack and returned), S8-88 (TAI flowchart: Y, A, X saved, first byte post-increment, second post-decrement, S decremented twice before the X push) and S8-89 (`F3, SL, SH, DL, DH, LL, LH`, 7 bytes, 17+6x, T=0) were read as page images from the hardware and software manual PDFs in a scratch directory outside the repo (sha256 40a32e5d... and 51eeda39... equal 03-RESEARCH); nothing was copied or committed.

## Deviations from Plan

**1. [Plan wording] The toy block-progress-lost anchors in `src/core/run.c`, not in cpu.c.** The plan names a remaining-count store line in cpu.c; a step cannot know that a run stops, so a cpu.c mutant cannot be red only under chunking. The RULE and the commit say so. The cpu.c store line is still one distinct line.
**2. [Plan wording] Tasks 1 and 2 were built together and committed in five commits.** The fixture source carries the LEN0 and BLK_IRQ sections from the first commit (one source); Task 1's verify was run on a tree with the Task 2 files moved aside. The follow-up commit b29a74a (checks 51 to 54) and the pin commit 98f636e are not in the plan.
**3. [Plan wording] 54 checks, not "about 40".** The five instructions, wrap, length 257, the stack clobber, the three timed rows, the long transfer (4), the interrupt rows (8) and the four stamp rows came to 54 (the DSV1 limit is 96). Every image runs all 54; the literals of checks 39 to 50 differ by image (conditional `LIT` lines), the plain and len0 images run the interrupt scenario with I set.
**4. [Plan wording] The state has no `latched[7]`** (key decision 1). The prologue latches the operand bytes into the decoded fields and PC moves in the prologue.
**5. [Rule 2 - missing] Checks 51 to 54 were added** because the 14/3 split and the read-at-4 and write-at-6 of the byte step were pinned by no committed check (the totals are E2 rows; a split of 13 and 4 or a write in another cycle left every guest check green). The four rows turn red on each one-cycle move.
**6. [Rule 3 - blocking] `ds_run` pays for the opcode-table guard only above half the range.** The chunk of one clock in cpu_blk_stop_resume makes 1.1 million calls; each scanned 256 table rows, so the case took 13 s (and about ten times that under the sanitizers). Identical behaviour for every request (see key decisions).
**7. [Rule 3 - blocking] The four block cases have `pins = []`.** inventory-check refuses a pin whose ledger entry does not name the case back, and the entries LED-CPU-0007, LED-CPU-0008 and LED-IRQ-0007 do not exist yet; plan 03-26 writes them and sets the pins (the plan's own text: LED-IRQ-0007 pins cpu_blk_irq_pending).
**8. [Rule 3 - files outside the list] The plan's 28-file waiver is exceeded (31 files over five commits).** Added because the work needs them: `src/core/ds_serial.h` and `tests/conformance/digkinds.c` (named by the Task 3 text but not in the files list), `tests/conformance/cases/digest_stream_blkt.toml` (the nonzero-BLKT pin the Task 3 text asks for, "digest literals ... computed independently"), `digest_stream_model.toml` and `digest_stream_irq.toml` (they carry the stream version and the digest), `src/core/run.c` (stop point and the guard), the second and third image manifests, `block-progress-lost/EXPECT`. `field_flip.toml` is unchanged: it flips the 13 profile fields, not region bytes; the remaining count is flipped by `digest_region` (`BLKT:all`) and pinned by `digest_stream_blkt`.
**9. [Design] The destination in the interrupt scenario is checked by pages.** `count_pages` reads the last byte of each of the 16 pages of the 4096-byte destination (cleared first), so "bytes written when the first handler runs" is 16 pages = $10; the first version counted every byte and cost 1.4 million clocks per run.

No Rule 4 deviation. The shared-state toy needed no change.

## Evidence discipline

E0 for the mechanism, E2 self-consistency for every in-guest timer row (36, 37, 38, 41, 51 to 54: the 12 master clocks per low-speed cycle, the 3 per high-speed cycle and the 3072-clock prescaler step are inputs, and the access stamp rule LED-CPU-0004). The MAN rows follow H8-6, S8-88 and S8-89 and compare bytes the guest moved with the page rules (post-increment, post-decrement, a fixed destination, the alternating destination or source, the length 0 rule, A X Y returned); GUEST rows compare with bytes the guest read or sums the assembler wrote from instruction counts. Rows classed ASSUMED:LED-CPU-0008 are the project's byte-at-a-time aliasing, 16-bit address wrap and stack-clobber reading (the epilogue pops from memory); rows classed ASSUMED:LED-IRQ-0007 are the rule that no interrupt is sampled inside a transfer and that it is sampled once at the end of the epilogue, which no manual page states. None of the three ledger ids is a ledger file yet; no status changed and none of the eight ER-2 candidates was touched. The 17+16z footnote of H8-25 was not used (R-1). The IRQ2 level of cpu_blk_irq_pending is a declared test stimulus (ER-5, D-112), not evidence about the VDC. The digest literals: a throwaway script outside the repo took the 8588-byte version 5 stream of each boot from the core's stream writer (a scratch patch of `ds_digest`, reverted), checked that its sha256 equals the committed version 5 literal, set the version to 6 and appended the BLKT frame (11 zero bytes, or the bytes `01 E3 01 00 00 01 00 34 12 CD AB` for the poked case) before hashing with Python's hashlib and, for the golden, `shasum -a 256`; the core's output then equalled all of them on the first run, so the encoding is pinned by a derivation that shares no code with the writer. A pass is not a support claim; no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. Block reads of the I/O page are not modelled (the manual states nothing; deferred, ledger open, D-100) and there is no per-byte trace record. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-69 (65536-byte transfers): each byte is a bounded step, runs are bounded by until_mclk, there is no fast path; cpu-blk-len0 runs under the sanitizers. T-03-70 (wrap and counter): 16-bit masks in `block_move` and the prologue, `remaining` is a u32 and the value 65536 is made only in the prologue; the len0 image shows the 65536th byte lands at the right cell. T-03-100 (interrupt model around transfers untested): cpu_blk_irq_pending has the timer and a declared IRQ2 pending inside a 4096-byte transfer, with eight rows; the chunk and observer coverage of the same stops is plan 03-26.

## Notes for next plans

- **03-26 (chunk_equiv, observer, ledger):** `cpu_blk_stop_resume` exists (run_chunk_sum on cpu-blk, one-call reference to 1150000, the plain image is done at 1.07 million); the case `cpu_blk_irq_pending` and the irq sweep carry `irq_stim ["2000000,1,1", "2100000,2,1", "2400000,1,0"]` and the irq image finishes at 2.41 million, so a chunk case on cpu-blk-irq needs until above 2.5 million and the same irq_stim; the guest polls the IRQ1 level (IRQ1D set), so the stimulus must have IRQ1 rise after the guest has arrived at the poll (the arrival was not measured; the whole plain image is done at 1.07 million clocks and the rise is at 2 million) and IRQ2 inside the next 296000. `stop_point` is block_byte after a prologue and after each byte; a chunk of 1, 7, 168 or 3072 clocks stops inside a transfer many times. Write LED-CPU-0007 (17 + 6x, length 0 = 65536; disagreement: the H8-25 footnote prints 17+16z, R-1; pin cpu_blk_len0), LED-CPU-0008 (aliasing, wrap, stack-clobber, 14/3 split, read@4 write@6, the three S levels versus the two of the S8-88 drawing; pins cpu_blk and cpu_blk_stop_resume, the stamp rows 51 to 54 are E2) and LED-IRQ-0007 (pin cpu_blk_irq_pending; the alternative parameter block_xfer_irq is deferred) and set `pins` in the four cases at the same time. The toy `block-progress-lost` has EXPECT `cpu_blk_stop_resume`; a chunk_equiv case that stops inside a transfer can join it (the replacement in run.c turns any run that ends at a deadline inside a transfer red).
- **03-29 and 03-30 (snapshot):** the BLKT fields are `blk_active`, `blk_opcode`, `blk_toggle`, `blk_remaining`, `blk_src`, `blk_dst` between `tma_zero` and `rom`. A restore must validate: `blk_active` 0 or 1; when 0 every other field 0; when 1 `blk_opcode` one of $73, $C3, $D3, $E3, $F3, `blk_toggle` 0 or 1 (always 0 for TII, TDD and TIN), `blk_remaining` at most 65536 (0 is legal while active: the epilogue is next); S and the stack bytes are ordinary state. The CONTEXT line "remaining 1..65536 only when active" does not hold: remaining 0 is the epilogue-pending state. The stream order of BLKT is the declaration order (active, opcode, toggle, remaining as u32, source as u16, destination as u16).
- **Record layout:** the `[withheld]` ids line now holds 24 of 24 ids (392 characters, the cap of the printer): the next plan that adds a ledger id (a constants or model row with a new id) must change the layout in the same task as a record format bump (one id per row), as the standing note says. DS_SERIAL_VERSION is 6.
- **Sweeps and policies:** cpu_blk initialises MPR0, MPR3 to MPR6, the timer reload before every start, the interrupt registers and every register a transfer must give back, so it passes under zero and both seeds of seeded-prng-v2; the sweeps run the default policy only. The len0 image runs 4.7 million clocks of transfer in each guest run, which costs a few tenths of a second.
- **Not covered by a committed check:** block accesses to the VDC and VCE windows (the penalty is charged in the bus for every access, D-107, but no block case aims at those windows), a block transfer with the I/O window as a source (not modelled), and a stop callback (rather than a deadline) inside a transfer outside the observer kinds of plan 03-26.
- **LED-CPU-0015 provenance gap (standing note):** untouched; plan 03-32 must still fill the disagreement column from the recorded sources.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/src/cpu_blk.s and cpu_blk.checks, the three manifests cpu_blk, cpu_blk_len0 and cpu_blk_irq, the cases cpu_blk, cpu_blk_len0, cpu_blk_irq_pending, cpu_blk_stop_resume, mutate_sweep_cpu_blk, _len0, _irq and digest_stream_blkt, and tests/controls/block-progress-lost (RULE and EXPECT). Commits 3dc7bbe, de25a81, b29a74a, 0a1c4b1 and 98f636e are ancestors of HEAD.
