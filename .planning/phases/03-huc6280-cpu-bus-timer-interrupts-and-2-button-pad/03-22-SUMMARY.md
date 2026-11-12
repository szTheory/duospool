---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 22
subsystem: interrupt-controller-irq-stim-i-sample-digest-regions
tags: [interrupt-controller, irq1, irq2, irq-stim, irq-i-sample, model-param, digest-regions, serial-v5, toys, irq-ctl, w4]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the closed-form timer, TIQ entry in the same step, access stamps and the $1402/$1403 registers with TIQ only (03-21), the model parameter machinery and BUS0 (03-17, 03-18), the record printer and goldens (03-18), the guest kind, observe rows and the mutation sweep (03-04, 03-10), the toy control mechanism (03-12)
provides:
  - src/core/irq.c complete - IRQ1 and IRQ2 levels from the declared stimulus, the request register bits 0 to 2, the priority chain TIQ > IRQ1 > IRQ2 as one statement, ds_irq_pending_query (pending bits after the disable mask, pure) beside ds_irq_pending (winning vector), ds_irq_stim_set with the
    typed stim.* refusals of the new src/core/stim_refusals.def (count, line, level, order, state)
  - the model parameter irq_i_sample (pre default, post, LED-IRQ-0005, open) in model_params.def and in cpu.c - the I flag of the sample is the one from before or after the instruction
  - digest regions TIMR (11 bytes), IRQC (3) and IRQS (160) and DS_SERIAL_VERSION 5; digest literals and both record goldens recomputed outside the C code
  - fixtures/src/irq_ctl.s (DSV id 11, 33 checks, check 30 observe-only), cases irq_ctl_pre, irq_ctl_post, mutate_sweep_irq_ctl, irq_stim_refuse_order/count/line/level/state, deferred irq_stim_refusal_coverage, digest_stream_irq, digest_model_irq_i_sample_differs
  - the case key irq_stim as an array of strings AT,LINE,LEVEL, read by one runner function and applied by the guest kind, the mutation sweep and digest_stream
  - toys timer-reload-off and irq-priority-swap (tests/controls); the gate now runs nine toy controls, all RED_OK
affects: [03-23, 03-24, 03-25, 03-26, 03-27, 03-29, 03-30, 03-32, 03-34]

actuals:
  tokens: 24800
  tasks: 3
  commits: 4
plan_head_before: 51e02a07bd26fbab01e9b21c1869f9d1bf0bf88f
plan_head_after: 73933fa11267cc6f56f9ef197286292aa52423ce

tech-stack:
  added: []
  patterns:
    - "a device input that no hardware drives yet is a declared test stimulus: a case key whose events are copied into the instance before the first step, validated by the core with typed refusals (so the runner cannot hand it a bad list), evaluated as a pure function of the stamp, carried in its
      own digest region, and always labelled a test hook in the case, the fixture header and the checks file"
    - "a guest fixture that depends on a declared time-based stimulus synchronises on the observable level (polls the request register until the line reads high, bounded) instead of on its own timing, so it is robust against the length of the code before it and the same ROM passes under every power-on policy"
    - "a latency parameter is shown by an observe-only row whose value is a register the handler saw at its entry (A after CLI: $77 under pre, $00 under post), judged per case; one run-request key flips the model value and the case's observe item changes with it"
    - "a region whose live storage is struct-of-arrays is kept one contiguous live span by ordering the instance members, with the stream interleaving per slot and an unused slot all zero by construction, so the digest_region first and last byte flips reach the first and last payload member"
    - "a tie of a timer borrow with a sample is made exact in a guest by counting instruction cycles from the start access (DELAY k = 6k cycles, LDA abs 5, NOP 2): E2 self-consistency that depends on the community-reported cycle length and the access stamp rule"

key-files:
  created:
    - src/core/stim_refusals.def
    - fixtures/src/irq_ctl.s
    - fixtures/src/irq_ctl.checks
    - fixtures/manifest/irq_ctl.toml
    - tests/conformance/cases/irq_ctl_pre.toml
    - tests/conformance/cases/irq_ctl_post.toml
    - tests/conformance/cases/mutate_sweep_irq_ctl.toml
    - tests/conformance/cases/irq_stim_refuse_order.toml
    - tests/conformance/cases/irq_stim_refuse_count.toml
    - tests/conformance/cases/irq_stim_refuse_line.toml
    - tests/conformance/cases/irq_stim_refuse_level.toml
    - tests/conformance/cases/irq_stim_refuse_state.toml
    - tests/conformance/cases/irq_stim_refusal_coverage.toml
    - tests/conformance/cases/digest_stream_irq.toml
    - tests/conformance/cases/digest_model_irq_i_sample_differs.toml
    - tests/controls/timer-reload-off/RULE
    - tests/controls/timer-reload-off/EXPECT
    - tests/controls/irq-priority-swap/RULE
    - tests/controls/irq-priority-swap/EXPECT
  modified:
    - src/core/irq.c
    - src/core/cpu.c
    - src/core/ds_core.h
    - src/core/model_params.def
    - src/core/model.c
    - src/core/serial.c
    - src/core/ds_serial.h
    - tests/conformance/guestkinds.c
    - tests/conformance/sweepkinds.c
    - tests/conformance/digkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/digest_stream_prefix.toml
    - tests/conformance/cases/digest_stream_model.toml
    - tests/conformance/cases/digest_region.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - scripts/lib/inventory-rules.awk
    - tests/gate/inventory/case-optional-wrong-type/tests/conformance/cases/opt_bad.toml
    - tests/gate/inventory/ok-case-optional-key/tests/conformance/cases/opt_keys.toml
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks

key-decisions:
  - "irq_stim is an array of strings AT,LINE,LEVEL (flatten type a, the elements joined by octal 037 in the TSV): the key type in runner.c and inventory-rules.awk changed from s to a and the two inventory mini-root samples were updated. One reader (irq_stim_parse in guestkinds.c, at most 32 elements
    so that a 17-event list can be given to the core and refused there) and one applier (apply_irq_stim) serve the guest kind, the sweep and digest_stream; plans 03-25 and 03-26 call them"
  - "ds_irq_stim_set takes struct ds_irq_event {at u64, line u32, level u32} so that the core judges the full 32-bit line and level (a line of 257 or a level of 256 is not 1 or 0). Refusal order: count (more than 16, or NULL with a count), then each event's line and level, then the order of times
    across events, then state (machine time not 0 or a NULL instance) last; a refusal writes nothing, a count of 0 clears. Equal times are legal"
  - "A line is low before its first event and after an event holds that event's level (the last event of the line at or before the stamp, ties inclusive). Level 1 means requested (the pin is low): it is what bits 0 and 1 of $1403 read. Disable bits: bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD, all masking delivery only, never the request register read (H8-15, H8-16)"
  - "The IRQS payload has no count byte: a slot with line 0 is unused and ds_irq_stim_set zeroes every unused slot, so the region is a function of the declared list alone and the instance stays free of a second source of truth. IRQS is the 16 slots (u64 time, line, level) = 160 bytes; TIMR is origin
    (u64), counter, reload, start bit = 11; IRQC is disable mask, TIQ latch, irq_i_sample selection = 3; appended after BUS0 in that order, serial version 5"
  - "The instance members were reordered so that every region is one contiguous live span with no padding (static asserts in ds_core.h and serial.c): irq_stim_at[16], irq_stim_line[16], irq_stim_level[16], timer_origin, timer_counter, timer_reload, timer_enabled, irq_disable, tiq_latch,
    irq_i_sample, then the CPU block, then io_buffer, mpr_latch, tma_zero, then the ROM. The timer bytes that plan 03-21 put after tma_zero moved up; nothing outside the core named offsets"
  - "irq_i_sample is a model row with status open (LED-IRQ-0005: 'latency, parameter, open'). The sample uses i_before under pre and the I flag after the instruction under post; one statement in cpu.c. The default is pre, tim_timer keeps the default (its check 33 holds under pre only). The row adds
    one withheld id, LED-IRQ-0005: the [withheld] ids line holds 18 ids after this plan (cap 24), so the layout note was not triggered and no record format change was made"
  - "The priority chain is the single statement 'return (bits & IRQ_BIT_TIQ) != 0u ? VEC_TIMER_LOW : ...' in ds_irq_pending: the anchor of irq-priority-swap. The anchor of timer-reload-off is the period statement of timer.c as plan 03-21 prepared it; the replacement is reload + 2, not reload + 0 (division by zero for reload 0 traps on some hosts and not on others), recorded in the RULE"
  - "irq_ctl synchronises on the declared stimulus by polling $1403 (bounded) rather than by timing, so its checks are robust against the code before them and all pass under both power-on policies; windows (master clocks) IRQ1 1200000-1500000, IRQ2 2200000-2500000, IRQ1+IRQ2 3200000-3500000, IRQ1 4200000-4500000. The case declares 10 of the 16 events"
  - "mmu_mpr checks 37 to 41 (the $1402/$1403 reads and writes) still pass unchanged: no stimulus is declared there so IRQ1 and IRQ2 read 0, and the disable write of check 41 cannot lead to an entry because I stays set. Only comments and the checks text were re-judged, so the three images are byte-identical and no manifest was re-pinned"

requirements-advanced: [CPU-06, CORE-05]
requirements-completed: [CPU-03]  # CPU-03: the timer (03-21) and now the interrupt controller with its registers, priority, vectors, sampling point and the named latency parameter, verified by guest fixtures with two red toys. CPU-06 still needs the speed, block and entry ledger rows (03-32); CORE-05 needs the restore and the remaining regions (BLKT, IOPT, SCHD, W5 and later)

duration: 30min
completed: 2026-11-11
---

# Phase 3 Plan 22: Interrupt controller, declared IRQ1/IRQ2 stimulus, irq_i_sample and the TIMR/IRQC/IRQS regions Summary

**The HuC6280 interrupt controller is complete: IRQ1 and IRQ2 levels come from a declared, core-validated test stimulus (irq_stim, at most 16 events, a pure function of time, never a VDC claim), the request register shows all three sources, delivery follows the priority timer, IRQ1, IRQ2 with BRK
sharing $FFF6 by its pushed B bit, the I flag of the sample is the model parameter irq_i_sample (pre or post), the timer and controller state are in the digest as TIMR, IRQC and IRQS (serial version 5), and a 33-check fixture plus two toys that turn red on a period off by one and on a swapped
priority verify it.**

## What was built

- **Task 1 (tracer), commit 2ac7d43.**
  - `irq.c`: `request_bits` (bit 0 the IRQ2 level, bit 1 the IRQ1 level, bit 2 TIQ), `stim_level` (the last event of a line at or before the stamp, ties inclusive), `ds_irq_read` for both registers, `ds_irq_pending_query` (request bits and not the disable mask, pure, declared in ds_core.h) and
    `ds_irq_pending` (one statement chain for the winner). `ds_irq_stim_set` copies a validated list into the instance before the first step; `src/core/stim_refusals.def` has `count`, `line`, `level`, `order` and `state` (prefix `stim.`, F4, a slot table with a compile-time ceiling).
  - Runner: the key `irq_stim` is an array of strings; `irq_stim_parse` and `apply_irq_stim` in guestkinds.c; the guest expect item `refused=stim.NAME` (no guest is run; a valid list is proven accepted and cleared first, then the case's list and the extra variants a key cannot express are refused with the instance byte-identical); the deferred kind `stim_refusal_coverage`.
  - Fixture `irq_ctl` first checks (1 to 15: reset and read-back of the registers, window A: the IRQ1 level reads in bit 1, I and IRQ1D each mask it, the unmasking write is taken at once, the entry goes through $FFF8 with B = 0 and the interrupted D and I, the handler sees I set, D clear and bit 1
    of $1403, a write to $1403 does not clear a level, the level ends with its window); cases `irq_ctl_pre` and the `order`, `count` and `line` refusal rows.
- **Task 2, commit c5a6a81.** The row `irq_i_sample` (`pre|post`, LED-IRQ-0005, open) and its use in `cpu.c`; windows B, C and D (checks 16 to 30): IRQ2D masks, BRK and IRQ2 reach the same handler through $FFF6 and are told apart by the pushed B (check 19 reads 1 for BRK, 20 reads 0 for IRQ2), the
  request register reads 7 with all three sources and a write clears TIQ only, the three entries come in the order timer, IRQ1, IRQ2 under both values, and the CLI probe (check 30, observe-only, `observe=30:119` under pre and `observe=30:0` under post). Cases `irq_ctl_post` (request key
  `model.irq_i_sample=post`), `mutate_sweep_irq_ctl` (exclude 30, applying the case's stimulus: `kind_mutate_sweep` now does), the `level` and `state` refusal rows and the deferred `irq_stim_refusal_coverage`; both goldens gained `[model_irq_i_sample]` and the id LED-IRQ-0005 (written by hand from
  the def); `model_refuse` rows now cover the second row; the mmu_mpr comments for checks 37 to 41 were re-judged.
- **Task 3, commit 8bd8162.** `serial.c`: regions TIMR, IRQC and IRQS as one live span each, `DS_SERIAL_VERSION` 5. Digest literals recomputed outside the C code: a python script rebuilt the version 4 streams from the code, each hashed (shasum -a 256) to the committed version 4 literal (5065a6c1...,
  7fc0dc11..., and the two golden digests 3d70806f..., 1843d592...), then set the version to 5 and appended the three frames written from the rules; `digest_stream_prefix`, `digest_stream_model` and both goldens now hold the new literals. New `digest_stream_irq` pins the encoding with nonzero bytes
  (the kind gained the hook `poke=timer-irq` and applies the case's irq_stim) and `digest_model_irq_i_sample_differs` pins the selection byte; `digest_region` lists the 11 regions. Toys `timer-reload-off` (period reload + 2; red on tim_timer and timer_pure_function) and `irq-priority-swap` (IRQ1
  before the timer; red on irq_ctl_pre).
- **Window E, commit 73933fa (scope note below).** Checks 31 to 33 put a write to $1403 and then a sample at exactly the stamp of the first borrow (counted instructions, E2): the write at the tie absorbs the borrow, TIQ reads 0 after it, and a borrow due at the sample stamp of a NOP is taken right after that NOP (the pushed return address is the label after it).

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `fixtures`, `conformance`; Task 2 `--stage fixture-lint`, `fixtures`, `conformance`; Task 3 `--stage conformance`, `record-lint`, `--selftest mutation-control-anchor` (nine rule lines PASS). The tracer gate (auto mode) re-ran the end-to-
  end verify, a full `sh scripts/check`, before the expansion: it passed (`Tracer verified end-to-end - expanding`).
- Plan end: `sh scripts/check` PASS (about 135 s wall for the last run), `sh scripts/check --selftest` PASS, `dash scripts/check` PASS, `sh scripts/audit-history --range 51e02a0..HEAD` clean (4 commits, 59 blobs, 1363 blob-path pairs, run before this docs commit). The nine toy controls are RED_OK, `shared-state` included (the standing note was not triggered: run.c is untouched).
- Bite shown with scratch edits (each restored byte for byte and `cmp`-checked, nothing committed): a pending query that ignores the disable register and a chain that serves the IRQ2 vector for IRQ1 each turn irq_ctl_pre guest_failure (the tracer can fail); an order check with a tolerance of one
  still fails through the variant list; a line check that looks at the low byte only fails the `line` row (257); a core that always samples with the I from before turns irq_ctl_post red while irq_ctl_pre passes (both values are implemented and the fixture tells them apart); an acknowledge that does
  not fold the due borrow first and a tie counted as exclusive each turn irq_ctl_pre (checks 31 to 33) red. A probe that dropped `i_before` did not compile (an unused variable), so the post-only direction is covered by the observe item instead.
- Manual facts: the hardware manual copy in a scratch directory outside the repo (sha256 40a32e5d... equals 03-RESEARCH) was read as page images for H8-12 (interrupt inputs sampled in the last bus cycle of an instruction; NMI), H8-13 (priority RESET, NMI, BRK, TIMER, IRQ1, IRQ2 and the vector table
  FFF6/FFF8/FFFA/FFFC), H8-14 (IRQ2 and the timer interrupt, TIQ reset by a write), H8-15 (request register: bit 0 IRQ2, bit 1 IRQ1, bit 2 TIQ, the inverted pin level, reset 0), H8-16 (disable register: bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD, 1 disables, 0 at reset). Nothing was copied or committed.

## Deviations from Plan

**1. [Rule 3 - files outside the plan list] The plan's 32-file waiver is exceeded (42 files over four commits).** Added because the work needs them: `tests/conformance/runner.c` and `scripts/lib/inventory-rules.awk` plus the two inventory mini-root samples (the key type of irq_stim is now an array,
the plan's own wording "array of strings at,line,level"), `tests/conformance/digkinds.c` (three payload branches and the `poke=timer-irq` hook), `fixtures/src/mmu_mpr.s` and `mmu_mpr.checks` (comments, checks 37 to 41), `src/core/model.c` (a comment), and the extra case files below.
**2. [Plan wording] `field_flip.toml` is unchanged.** The plan lists it and says "field_flip rows change the digest"; that case is the profile-identity flip of the 13 profile fields, not a region perturbation. The perturbation of the new regions is `digest_region`, which was updated, and `digest_stream_irq` pins the encoding with nonzero bytes.
**3. [Plan wording] One more refusal code and row.** `stim_refusals.def` has `state` beside the four rows the plan names (the plan itself says the stimulus is legal "only before the first step"), so there is a fifth REQUIRED row `irq_stim_refuse_state` and the deferred coverage requires all five.
The deferred coverage needs a kind, so the kind `stim_refusal_coverage` was added (the refusal rows are kind guest with `refused=stim.NAME` as planned).
**4. [Rule 1 - broken by the new row] `model_refuse` bad name.** `model_refuse_unknown_name` used `irq_i_sample` as a name that is NOT a row; it is one now. The bad-name list uses near misses (`irq_i_sampl`, a trailing blank, upper case, a `|` join) and the value and state variants gained calls on the second row.
**5. [Rule 2 - strengthening, not in the plan] Window E (checks 31 to 33) and two extra digest cases.** The plan's behaviour list says a write to $1403 in the sampled cycle takes effect before the sample (ties inclusive); nothing in a guest showed it, so three checks were added in a fourth commit
(E2, ASSUMED:LED-IRQ-0006). `digest_stream_irq` and `digest_model_irq_i_sample_differs` follow the brief's rule that a new model row needs its region byte, a serial bump and a distinguishing case.
**6. [Plan wording] `timer-reload-off` uses reload + 2.** The plan text says the replacement uses reload instead of reload + 1; plan 03-21 recorded that reload + 0 divides by zero for reload 0 (it only turned red on arm64 because integer division there does not trap), so the toy takes the direction
that is defined everywhere. tim_timer check 18 and timer_pure_function both turn red on it.
**7. [Design] The instance members were reordered and the stimulus has no count byte** (key decisions). The tiling list of plans 03-26 and 03-30 must be written against the new order.
**8. [Note] The three mmu_mpr images and manifests were not re-pinned.** Only comments changed, so the hashes are identical; the plan's "three manifests to re-pin" did not apply.

No Rule 4 deviation. The shared-state toy control stayed RED_OK, so the standing note was not triggered; the `[withheld] ids` line holds 18 ids (cap 24).

## Evidence discipline

E0 for the mechanism: the core is compared with literals written in the fixture, the case or a python re-derivation, never with DuoSpool output. The register facts of H8-12 to H8-16 are cited from image reads (the cites registry has all of them as img); the ASSUMED rows name LED-IRQ-0005 (the
latency, open), LED-IRQ-0006 (stamp, tie, ack in the sample cycle, E2) and LED-IRQ-0008 (the stimulus), none of which exists as a ledger entry yet (plan 03-34 writes them) and none was promoted. The checks that read counted cycles (31 to 33) are E2 self-consistency: they depend on the community-
reported 12 master clocks per CPU cycle and the access stamp rule of LED-CPU-0004. **The IRQ1 and IRQ2 stimulus is a test hook, not evidence about the VDC or any hardware source (ER-5, D-112);** every case file, the fixture header and the checks file say so, and no accuracy, compatibility, cycle-
accurate, hardware-verified or boot claim is made. **A manual disagreement to carry into the ledger entry (plan 03-34):** H8-14 states the TIQ set conditionally ("if a timer borrow occurs with I reset and TIQD at 0, TIQ is set and a timer interrupt occurs"), while H8-15 ("set by a timer borrow") and
H8-21 read as unconditional; the core follows the unconditional reading (the latch is set by every borrow, masked or not), shown by tim_timer checks 28 and 32. Nothing pushed (D-64).

## Known Stubs

None added. IRQ1 and IRQ2 have no hardware source until the VDC arrives in Phase 4 (the stimulus is the declared test hook; with none declared both levels read 0); the I/O window pad is plan 03-27, the PSG Phase 5, as before. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-60 (irq_stim copy-in): at most 16 events, a line of 1 or 2, a level of 0 or 1, non-decreasing times, copied before the first step only, judged on the full 32-bit words by the core after the one parser; shown by five refuse rows with variants. T-03-61 (misread as
hardware evidence): the ER-5 label is in every case comment, the fixture header and the checks file (LED-IRQ-0008 is written by 03-34). T-03-62 (toy anchors): both anchors are single statements and the `mutation-control-anchor` selftest passes for both. The new runner reader of `irq_stim` is test
code and bounds its input (32 elements, 64 characters each).

## Notes for next plans

- **03-23 (record `[stimulus]` table):** the stimulus is in the instance as `irq_stim_at/line/level[16]` (line 0 = an unused slot); `emit_stimulus` can count events as the slots with a nonzero line. The `[withheld] ids` line holds 18 ids (LED-IRQ-0005 was added by the model row); the four new W4
  constants rows of 03-23 add at most four more, so check the cap of 24 (the standing note) before adding LED-IRQ ids; LED-IRQ-0008 (the stimulus) is NOT in the list yet and would be the nineteenth.
- **03-24 (power-up v2):** TIMR and IRQC are now in the digest, so the timer counter and reload drawn under `seeded-prng-v2` change the digest, as intended; irq_ctl initialises everything it reads and its checks do not read the counter (only window C starts the timer at reload 0 and window E counts from the start access). The IRQS region is declared by the case, never drawn.
- **03-25 (block transfers):** use `apply_irq_stim` and the `irq_stim` case key; `kind_mutate_sweep` applies the case's stimulus before every run (this plan's NW3). `ds_irq_pending_query` is ready for 03-26 (a stop with an interrupt pending); both entry points are pure.
- **03-26 (chunk and random cases), 03-30 (snapshot):** the instance layout changed: `irq_stim_at[16]`, `irq_stim_line[16]`, `irq_stim_level[16]`, `timer_origin`, the three timer bytes, `irq_disable`, `tiq_latch`, `irq_i_sample`, then `pc..speed`, then `io_buffer`, `mpr_latch`, `tma_zero`, then the
  ROM; every region of the digest is one live span. A restore must validate: stimulus slots in non-decreasing time with line 0 only after the last used slot and unused slots all zero, line 1 or 2 and level 0 or 1 in used slots, `irq_i_sample` below 2, `irq_disable` below 8, `tiq_latch` and
  `timer_enabled` 0 or 1, counter and reload at most 127.
- **Sweeps and observers:** every sweep case of a fixture that needs a stimulus must carry the same `irq_stim` as its plain case; the observer kind (obskinds.c) does not apply irq_stim yet (no observer case needs it); `digest_stream` applies it and has the `poke=timer-irq` hook.
- **Toys:** nine toy controls now run (about 5 s each in the conformance stage, the gate is about 135 s wall here). `trace-hook-mutates` EXPECT was not extended.
- **Ledger (03-34, 03-32):** LED-IRQ-0004 (controller facts: the checks cite the pages and no ASSUMED row names it), -0005 (check 30 and the model row, status open; disagreement: no manual statement of the I value the sample sees), -0006 (checks 31 to 33), -0008 (checks 1, 15 and the stimulus). The
  pins: LED-IRQ-0004 `irq_ctl_pre`, -0005 `irq_ctl_post`, -0006 `irq_ctl_pre`, -0008 `irq_stim_refusal_coverage`. The H8-14 conditional wording above belongs in the LED-IRQ-0004 disagreement; the standing LED-CPU-0015 provenance gap is untouched (this plan writes no ledger entry).
- **Timing:** `sh scripts/check` about 135 s wall here (conformance about 45 s). CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/stim_refusals.def, fixtures/src/irq_ctl.s and irq_ctl.checks, fixtures/manifest/irq_ctl.toml, the cases irq_ctl_pre, irq_ctl_post, mutate_sweep_irq_ctl, irq_stim_refuse_order, _count, _line, _level, _state, irq_stim_refusal_coverage, digest_stream_irq and
digest_model_irq_i_sample_differs, and tests/controls/timer-reload-off and irq-priority-swap (RULE and EXPECT each). Commits 2ac7d43, c5a6a81, 8bd8162 and 73933fa are ancestors of HEAD.

