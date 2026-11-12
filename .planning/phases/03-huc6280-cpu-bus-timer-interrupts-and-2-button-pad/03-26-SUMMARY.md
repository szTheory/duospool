---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 26
subsystem: w5-chunk-invariance-random-programs-observer-ledger
tags: [chunk-equiv, random-programs, splitmix64, observer-matrix, block-transfer, pending-stop, ledger, w5]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: resumable block transfers, the cpu_blk fixture in three images and the block-progress-lost toy (03-25), irq_stim, the pure ds_irq_pending_query and the irq_ctl fixture (03-22), the observer matrix and both sinks (03-19), the three-policy guest runs (03-24), the closed-form timer (03-21)
provides:
  - kind chunk_equiv (chunkkinds.c; keys until_mclk, chunks, irq_stim; expect compare=verdict,machine_time,stops,digest[;block-stop][;pending-stop][;random=N]): one uninterrupted run past done and one run per schedule (1, 7, 168, 3072, frame = a fixed 357368, tick-1, tick+1) compared on time, steps, verdict RAM, digest and the step boundary sequence
  - randprog.c: 256 KiB in-memory programs from a splitmix64 stream (rp_seed, rp_build, boot_random): defined opcodes, random operands, forward branches, short block lengths, vectors at a spin loop
  - observer kind: applies irq_stim, runs random programs (fixture none, random=N), counts block byte stops (pending-stop), offers the instruction sink once per instruction, folds the block state, fits the large sink to the deadline
  - cases chunk_equiv_cpu_blk_len0, _cpu_blk, _cpu_blk_irq, _ds_proto, _tim_timer, _irq_ctl_pre, _random32, observer_cpu_blk, observer_random (all REQUIRED); the toys block-progress-lost (cpu_blk_stop_resume plus four of the new cases) and trace-hook-mutates (observer_cpu_blk, observer_random) list them
  - ledger entries LED-CPU-0007, LED-CPU-0008 and LED-IRQ-0007 (assumed, origin D-100, REQUIRED lines) pinned by cpu_blk_len0, cpu_blk and cpu_blk_irq_pending, whose pins keys name them back
affects: [03-27, 03-29, 03-30, 03-31, 03-32, 03-33]

actuals:
  tokens: 18200
  tasks: 3
  commits: 4
plan_head_before: 80a41f309c55a82eec7e4bb0206d0ead715a9970
plan_head_after: 181873defaa46d3ae480242b2d893558be2547c5

tech-stack:
  added: []
  patterns:
    - "the stop sequence of a chunk comparison is the sequence of step boundaries, not a callback count: a callback that never holds is called before the first step of every call and after every step, so it folds the state only at a machine time it has not seen, and the fold is the same for any chunking when the machine is chunk invariant"
    - "a schedule is deadlines period * j + delta clipped to the end; deadlines the previous call overshot are skipped (ds_run takes no step for them, checked once after the loop with a deadline behind machine time), so a chunk of one clock costs one call per step"
    - "the pending-at-stop assertion is made from the result and the instance, not from the guest: a call that took a step and ended with stop point block_byte, and ds_irq_pending_query at the machine time of the stop equal to the request bits of the timer and IRQ2"
    - "a random program set is made from a seed that is a hashed function of the index (one splitmix64 output of a state spread by an odd constant), because indexes spaced by the splitmix gamma give streams that are shifted copies of one stream"

key-files:
  created:
    - tests/conformance/chunkkinds.c
    - tests/conformance/randprog.c
    - tests/conformance/cases/chunk_equiv_cpu_blk_len0.toml
    - tests/conformance/cases/chunk_equiv_cpu_blk.toml
    - tests/conformance/cases/chunk_equiv_cpu_blk_irq.toml
    - tests/conformance/cases/chunk_equiv_ds_proto.toml
    - tests/conformance/cases/chunk_equiv_tim_timer.toml
    - tests/conformance/cases/chunk_equiv_irq_ctl_pre.toml
    - tests/conformance/cases/chunk_equiv_random32.toml
    - tests/conformance/cases/observer_cpu_blk.toml
    - tests/conformance/cases/observer_random.toml
    - ledger/entries/LED-CPU-0007.toml
    - ledger/entries/LED-CPU-0008.toml
    - ledger/entries/LED-IRQ-0007.toml
  modified:
    - tests/conformance/obskinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - CMakeLists.txt
    - tests/conformance/REQUIRED
    - tests/controls/block-progress-lost/EXPECT
    - tests/controls/block-progress-lost/RULE
    - tests/controls/trace-hook-mutates/EXPECT
    - tests/conformance/cases/cpu_blk.toml
    - tests/conformance/cases/cpu_blk_len0.toml
    - tests/conformance/cases/cpu_blk_irq_pending.toml
    - tests/conformance/cases/cpu_blk_stop_resume.toml
    - ledger/REQUIRED

key-decisions:
  - "Seven schedule names, not six: the plan lists the sizes 1, 7, 168, 3072, a frame-sized 357368 and the ticks 3072k-1, 3072k and 3072k+1; the size 3072 is the deadlines 3072k, so the names are 1, 7, 168, 3072, frame, tick-1 and tick+1. frame is labelled a fixed size in the case text because the machine has no frame yet"
  - "The step boundary sequence is the stop sequence of chunk_equiv (the plan's wording is 'stop sequence'): every call must end on its deadline (reason UNTIL for all, so the ordered stop reasons are equal), and the fold of the states at every step boundary must equal the uninterrupted run's, which catches a divergence that recovers before the end"
  - "pending-stop is asserted per schedule in chunk_equiv and on the no-sink reference of every non-whole mode in observer (chunk:13 puts the 11th boundary at 2.2 million clocks; stop:53000 lands inside the 4096-byte transfer, from a scratch count: about 50800 callbacks before it, 4096 inside); the sinks must not change which calls those are"
  - "Boot cost decides the gate cost of the random set: the gate preset is unoptimised and every run boots a 256 KiB image (hash, copy), about 7 ms natively. Per program chunk_equiv_random32 makes 8 boots (the image is built once per program and copied), observer_random 12 (three sink configurations; cap0 and cap1 are covered by the authored observer cases)"
  - "The random programs are shaped: without it every program fell into a spin loop within about 20 instructions (shifted seeds, then BRK, JSR, RTI, JMP, RTS and backward branches) or into one 65536-byte transfer for the whole deadline; now 6000 to 50000 steps, 242000 block byte stops over the set"
  - "Ledger pins: one entry names one pinning case, so cpu_blk is the pin of LED-CPU-0008 and cpu_blk_stop_resume is listed in its evidence and has no pins back-link; the three entries carry the four standard withheld items of section K (the plan's 'four withheld items')"
  - "No new id in the run record: LED-CPU-0008 and LED-IRQ-0007 were printed through the constants rows since plan 03-25 and LED-CPU-0007 is not printed by any def, so the [withheld] ids line stays 24 of 24 and no layout change was needed"

requirements-advanced: [CORE-06, CPU-06]
requirements-completed: [CPU-05, CORE-04]

duration: 150min
completed: 2026-11-11
---

# Phase 3 Plan 26: chunk_equiv, the random program set, the observer extensions and the W5 ledger entries Summary

**A new kind runs a fixture or one of 32 in-process random programs to one absolute deadline past done and again under seven chunk schedules (1, 7, 168, 3072, a fixed 357368, 3072k-1 and 3072k+1 master clocks) and requires the same machine time, steps, verdict RAM, digest and step boundary sequence,
with stops inside the 65,536-byte transfer and at block byte boundaries while the timer interrupt and IRQ2 are pending; the observer matrix now covers the irq image and the random set; LED-CPU-0007, LED-CPU-0008 and LED-IRQ-0007 are written as assumed entries pinned to the cases that test them.**

## What was built

- **Task 1 (tracer), commit 1930be3.** `chunkkinds.c` with kind `chunk_equiv` (registered by one line in runner.h, one row of the kind table and `DS_RUNNER_SOURCES`), the case `chunk_equiv_cpu_blk_len0` (until 5,900,000; the guest is done at 5,804,508; 181,106 block byte stops over the seven
  schedules, 51 assertions) and its lines in REQUIRED and in the EXPECT of `block-progress-lost`. `randprog.c` is registered in the same commit because the kind boots random programs (deviation 1). Tracer verify as written: `--stage build` and `--stage conformance` PASS; the toy `block-progress-
  lost` is RED_OK with the case in its EXPECT, which shows that the case can fail.
- **Task 2, commits 038a78f and 181873d (the second trims observer_random).**
  - `randprog.c`: splitmix64 programs, shaped as in the key decisions; `rp_seed` is a hashed function of the index.
  - Chunk cases: `chunk_equiv_ds_proto` (until 400,000, done at 137,880), `_tim_timer` (800,000, done at 415,128), `_cpu_blk` (1,150,000, done at 1,086,132, block-stop), `_irq_ctl_pre` (4,700,000, done at 4,607,868, the 10-event irq_stim), `_cpu_blk_irq` (2,600,000, done at 2,407,476, irq_stim
    `2000000,1,1`, `2100000,2,1`, `2400000,1,0`, pending-stop: 6,792 block byte stops with TIQ and IRQ2 pending over the seven schedules, every schedule at least one) and `_random32` (32 programs x 7 schedules, until 600,000, 256 runs, 242,000 block byte stops).
  - Observer: `ob_exec` takes a source (an authored image or a random program) and the case, applies the case's `irq_stim` (the kind did not before), counts block byte stops and the pending ones, and the stop callback folds the block state and counts the steps that began inside a transfer.
    `observer_cpu_blk` runs the irq image with `whole,chunk:13,stop:53000`, the five sink configurations and pending-stop; `observer_random` runs the 32 programs with `whole,chunk:7,stop:2000` and three sink configurations (large, drop4, poison).
  - Toys: `block-progress-lost` EXPECT gained `chunk_equiv_cpu_blk`, `_cpu_blk_irq`, `_random32` and `observer_cpu_blk`; `trace-hook-mutates` EXPECT gained `observer_cpu_blk` and `observer_random`; all ten toys are RED_OK.
- **Task 3, commit d88e057.** The three entries (all `assumed`, four withheld items, origin D-100, REQUIRED lines) and the `pins` of `cpu_blk` (LED-CPU-0008), `cpu_blk_len0` (LED-CPU-0007) and `cpu_blk_irq_pending` (LED-IRQ-0007). LED-CPU-0007 cites H8-6 and S8-88/S8-89 and carries the H8-25 17+16z
  footnote in `disagreement` (errata R-1); LED-CPU-0008's `disagreement` records that S8-88 draws S decremented twice before the X push while the core uses three levels (the cells and the net S are the same); LED-IRQ-0007 says no page states a sample rule for transfers and the alternative (a sample
  between bytes, the deferred `block_xfer_irq`) is not implemented.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build` and `--stage conformance`; Task 2 `--stage conformance` and `--stage sanitizers` (the sanitizers run ASan and UBSan over every new case, the random sets included, T-03-72); Task 3 `--stage ledger-entries` and `--stage inventory`.
- Plan end (after the last task commit): `sh scripts/check --timing` PASS (2 min 29 s wall at a load average of 10 to 18; an earlier run before the last small commit took 3 min 50 s at a load average of 20 to 25), `sh scripts/check --selftest` PASS, `sh scripts/audit-history --range 80a41f3..HEAD`
  clean (4 commits, 33 blobs, 1410 blob-path pairs, run before the docs commit), `sh scripts/check --stage inventory` also passes under `dash`.
- Timing (D-132, warn only, wall seconds, so comparisons with earlier plans are rough because the machine was loaded): `TIMING_WARN conformance` 21, `TIMING_WARN sanitizers` 43 (about 27 after plan 03-25), determinism 17 (about 8 after plan 03-25: the stage runs the runner twice and the new cases
  are in it), named-controls 6, fixtures 3. CPU seconds (user plus sys) of the new cases alone, measured one case at a time: gate preset 5.8 with observer_random at five sink configurations (random32 2.0, observer_random 3.0, now about 2), sanitizers about 19 (random32 6.0, observer_random 10.4, now
  about 7); every other new case is below 1 second in both. The cost is boot bound, see key decisions.
- Bite shown with scratch edits of src/core/run.c (each restored from a saved copy and checked with `git status`, nothing committed): clearing `blk_toggle` at the start of every call turns red chunk_equiv_cpu_blk, _cpu_blk_irq, _cpu_blk_len0, _random32, observer_random and cpu_blk_stop_resume;
  setting `timer_origin` to the machine time at the start of every call turns red every chunk_equiv case, every observer case and the interleave cases; clearing `tiq_latch` at the start of each call turned nothing red (the controller computes TIQ from the timer, so that is not a bug and not a probe
  of the cases). A scratch change of chunk_equiv_cpu_blk to `pending-stop` fails with the message that no stop had both interrupts pending (the plain image has no IRQ2). The toy `block-progress-lost` (a run that gives back the byte just moved when it ends inside a transfer) turns red all of its
  listed cases.
- Manual facts: nothing new was read; the page values of the three entries are those read as images in plans 03-22 and 03-25 (H8-6, S8-88, S8-89, H8-12) and recorded by the phase researcher.

## Deviations from Plan

**1. [Rule 3 - blocking] randprog.c is in the Task 1 commit, not Task 2's.** chunk_equiv boots random programs through `boot_random`, so the file has to exist for Task 1 to build; its cases come with Task 2.
**2. [Plan wording] Seven schedules.** The plan says six and lists eight items; 3072 and 3072k are the same deadlines (key decisions).
**3. [Plan wording] The "stop sequence" is the step boundary sequence.** The plan says identical "stop sequence"; a chunked run has more calls than the uninterrupted one, so the comparable thing is the state at every step boundary (a callback that never holds, folded at each machine time it has not seen), together with every call ending on its deadline.
**4. [Rule 1 - bug in an existing check] The observer's "one offer per executed instruction" check was wrong for block transfers.** `iseq == res.steps` counted the byte steps and the epilogue as instructions; the instruction sink is offered once per instruction (at the prologue). It now subtracts
the steps that began inside a transfer (the stop callback counts them), in the per-call check and in the whole-run check. No existing observer case executes a block transfer, which is why plan 03-25 did not meet it.
**5. [Rule 2 - cost] Two changes keep the random sets cheap.** chunk_equiv skips deadlines already behind the machine time (a chunk of one clock otherwise makes a call per clock, 600,000 calls per run; the contract that such a call takes no step is checked once after the loop), and the observer's
large sink capacity is fitted to the deadline (`until / 6 + 16` records, an instruction taking at least two cycles of at least three master clocks; a large sink that still loses a record stays harness_failure) instead of 262,144 records per run. observer_random was first written with five sink
configurations and was then cut to three (commit 181873d) to save about a third of its boots.
**6. [Rule 1 - generator bug found by probing] The seeds of the random programs were shifted copies of one stream.** The first generator used seeds BASE + i * gamma with gamma the splitmix increment, so program i was program 0's stream advanced by i draws, and every program fell into the spin loop
at the same step (16 to 24). The seed is now one splitmix64 output of a state spread by an unrelated odd constant. Then the shapes (forward-only branches, short block lengths, rare BRK, JSR, RTI, JMP, RTS) were added because the programs still fell into the spin or into a 65536-byte transfer within
a few dozen instructions.
**7. [Rule 3 - files outside the list] Added because the work needs them:** `tests/controls/block-progress-lost/RULE` (a comment line), `tests/controls/trace-hook-mutates/EXPECT` (the two new observer cases), the comment of `tests/conformance/cases/cpu_blk_stop_resume.toml` (it said the entry was
not written yet). The plan's 24-file waiver is exceeded by three (27 files over four commits): the RULE comment, the trace-hook-mutates EXPECT and the cpu_blk_stop_resume comment.
**8. [Plan wording] The toy EXPECT of block-progress-lost lists five more cases than the plan's one** (cpu_blk, cpu_blk_irq, random32 and observer_cpu_blk besides len0), because each ends calls inside transfers and each turns red on the toy; they show that those cases can fail.
**9. [Design] cpu_blk_stop_resume has no pins back-link.** The 03-25 notes said it would pin LED-CPU-0008 with cpu_blk; one entry names one pinning case (inventory-check, both directions), so cpu_blk is the pin and the stop-resume case is in the evidence list.
**10. [Rule 3] The runner's `got` text is limited to 63 characters;** the first line of chunk_equiv's detail was cut, so it prints `runs=N bs=N ps=N done=N`.

No Rule 4 deviation. The shared-state toy was not touched and stayed RED_OK (run.c is unchanged by this plan).

## Evidence discipline

E0 throughout for the new cases: the core is compared with itself under chunkings, sinks and random programs (repeatability and consistency, ER-7); no literal comes from a DuoSpool run, and the pending-at-stop assertions use the pure query of plan 03-22. The random programs have no meaning and no
claim is made about what any hardware does with them. The ledger entries are `assumed`: LED-CPU-0007's count, length rule and restore are page values (H8-6, S8-88, S8-89; the H8-25 footnote is the misprint of R-1 and is not used), and its in-guest cycle rows are E2 self-consistency because they use
the community-reported cycle length; LED-CPU-0008 and LED-IRQ-0007 are modelling assumptions whose pins assert the modelled behaviour. None of the eight ER-2 candidates was promoted (only plan 03-32 may), the `[withheld]` lists name hardware cycle accuracy, interrupt latency in cycles, game
compatibility and TG-16 or region claims, and the IRQ2 level of the irq cases is a declared test stimulus (ER-5, D-112). A pass is not a support claim; no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. Not covered by a committed check: block accesses to the VDC and VCE windows (random programs reach them by chance and the penalty is charged in the bus, but no case asserts it for a block), a block transfer with the I/O window as its source (not modelled), and the "mid-frame" half of
CORE-04 (no frame exists before the VDC; the frame-sized chunk is a fixed size). A stop callback that ends a run inside a transfer is now covered (observer_cpu_blk, stop:53000, and the random observer's stop:2000). Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-71 (long random runs): every run has a fixed absolute deadline (600,000 and 300,000 master clocks) and the set is 32 programs; T-03-72 (memory safety on random paths): both random cases run in the sanitizers stage under ASan and UBSan and passed; T-03-73
(repeatability mistaken for correctness): the cases, their meanings and the ledger evidence say E0 and ER-7.

## Notes for next plans

- **03-27 and later kinds:** to add a chunk case write a case, not code: `kind = "chunk_equiv"`, `fixture = "authored:NAME"`, `until_mclk` past the guest's done time (the evidence file prints `done=`), `chunks = "1,7,168,3072,frame,tick-1,tick+1"`, `expect =
  "compare=verdict,machine_time,stops,digest"` (plus `block-stop`, `pending-stop` or `random=N`), the case's `irq_stim` if the guest needs one, and add it to REQUIRED. The frame schedule needs `until_mclk` above 357,368 or the case fails (a schedule must split the run). The pad fixture of 03-27
  needs a scripted-input key on the kind if its run depends on one (chunk_equiv boots through `boot_built_rom` and applies only irq_stim; the guest kind's input key is not applied here).
- **03-29 and 03-30 (snapshot):** chunk equality is of outputs, machine time, stop sequence and digests; once a snapshot exists a schedule can also restore at each stop. The BLKT restore domain is as recorded in the 03-25 notes: `blk_active` 0 or 1; when 0 every other field 0; when 1 `blk_opcode`
  one of $73, $C3, $D3, $E3, $F3, `blk_toggle` 0 or 1 (always 0 for TII, TDD and TIN), `blk_remaining` at most 65536 with 0 legal while active (the epilogue is next), `blk_src` and `blk_dst` any 16-bit value. randprog.c gives a restore case a set of states of every kind (242,000 stops inside
  transfers over the 32 programs) that no fixture provides.
- **Observer kind users:** the case keys are unchanged; `fixture = "none"` with `random=N` (1 to 64) in the expect selects the random set, `pending-stop` asserts the pending block stop of every non-whole mode, and a case that declares an `irq_stim` now gets it applied to every run of the matrix. The large sink capacity follows the deadline (`until / 6 + 16`, at most 262,144).
- **Record layout:** the `[withheld]` ids line still holds 24 of 24 ids; this plan added no id (LED-CPU-0008 and LED-IRQ-0007 were already printed through the constants rows, LED-CPU-0007 is in no def). The next plan that adds a ledger id to a def or a model row must change the layout in the same task (standing note).
- **Ledger (03-32):** the three entries are `assumed` with pins cpu_blk_len0, cpu_blk and cpu_blk_irq_pending. LED-CPU-0007 is one of the eight ER-2 candidates: the page re-read, the literal-from-page rule and the statement wording ("the DuoSpool table or behaviour equals the manual's stated value")
  are for plan 03-32 to check before any promotion; the cycle rows are E2 and cannot back a promotion alone. The standing LED-CPU-0015 provenance gap (the `disagreement` column does not name the emulator readings) is untouched; 03-32 must fill it from the recorded sources and label them community or
  code observations.
- **Cost:** the random cases are boot bound (a 256 KiB image per run in an unoptimised preset). If the sanitizers stage needs trimming later, the cheapest cuts are the `7` schedule of chunk_equiv_random32 (the same stops as `1` except at the high speed), or a smaller set for observer_random; neither was done here because the plan asks for the 32-program set.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: tests/conformance/chunkkinds.c and randprog.c, the nine case files (seven chunk_equiv, observer_cpu_blk, observer_random) and the three ledger entries. Commits 1930be3, 038a78f, d88e057 and 181873d are ancestors of HEAD.
