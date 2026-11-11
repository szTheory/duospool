---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 34
subsystem: w4-ledger-timer-interrupt-speed-entries
tags: [ledger, timer, interrupt-controller, speed-switch, power-on, assumed, open, pins, w4]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the closed-form timer and timer_pure_function (03-21), the interrupt controller, irq_stim and irq_ctl (03-22), spd_speed, spd_speed_trace, record_stimulus_shown and the four W4 constants rows (03-23), cpu_flow, vdc_penalty_assumed and the entries LED-CPU-0009 and LED-CPU-0017 (03-15, 03-20)
provides:
  - ledger entries LED-IRQ-0002, -0003, -0004, -0005 (open), -0006, -0008, LED-CPU-0005, LED-CPU-0006 (open) and LED-PROF-0003 (PENDING:3), each with every schema key, the withheld items, an origin equal to its ledger/REQUIRED line, and verifier, date and fact on every FACT source (R-8)
  - eight REQUIRED lines (nine with LED-IRQ-0002) and the pins back-links on timer_pure_function, tim_timer, irq_ctl_pre, irq_ctl_post, record_stimulus_shown, spd_speed and spd_speed_trace (F9)
  - LED-CPU-0009 and LED-CPU-0017 edited (not duplicated) to name the W4 constants row and the spd_speed E2 rows
affects: [03-24, 03-25, 03-26, 03-29, 03-30, 03-32]

actuals:
  tokens: 21000
  tasks: 2
  commits: 2
plan_head_before: 74533fd13b69047cafb8e73205e09052fff4dc4c
plan_head_after: 47e1a9e8c8b568fb48f726b1967619328119a72a

tech-stack:
  added: []
  patterns:
    - "a ledger entry whose claim mixes page facts and modelling choices is split in its statement as (a) page facts and (b) assumptions, so that only (a) could ever be promoted (writer note 3)"
    - "one pin per entry: when two cases test one claim, the case that tests the claim is the pin and the other is named in the entry comment (LED-CPU-0005 pinned by spd_speed_trace, LED-CPU-0006 by spd_speed)"
    - "a ledger key is one line of at most 399 characters, so detail beyond the key goes in comment lines above the id"

key-files:
  created:
    - ledger/entries/LED-IRQ-0002.toml
    - ledger/entries/LED-IRQ-0003.toml
    - ledger/entries/LED-IRQ-0004.toml
    - ledger/entries/LED-IRQ-0005.toml
    - ledger/entries/LED-IRQ-0006.toml
    - ledger/entries/LED-IRQ-0008.toml
    - ledger/entries/LED-CPU-0005.toml
    - ledger/entries/LED-CPU-0006.toml
    - ledger/entries/LED-PROF-0003.toml
  modified:
    - ledger/REQUIRED
    - ledger/entries/LED-CPU-0009.toml
    - ledger/entries/LED-CPU-0017.toml
    - tests/conformance/cases/timer_pure_function.toml
    - tests/conformance/cases/tim_timer.toml
    - tests/conformance/cases/irq_ctl_pre.toml
    - tests/conformance/cases/irq_ctl_post.toml
    - tests/conformance/cases/record_stimulus_shown.toml
    - tests/conformance/cases/spd_speed.toml
    - tests/conformance/cases/spd_speed_trace.toml

key-decisions:
  - "Pins (F9, one per entry): LED-IRQ-0002 timer_pure_function, -0003 tim_timer, -0004 irq_ctl_pre, -0005 irq_ctl_post, -0006 irq_ctl_pre (irq_ctl_pre pins two ids), -0008 record_stimulus_shown (the plan's choice; irq_stim_refusal_coverage is named in the evidence), LED-CPU-0005 spd_speed_trace, LED-CPU-0006
    spd_speed, LED-PROF-0003 PENDING:3 (no case exists before plan 03-24), LED-CPU-0017 stays vdc_penalty_assumed and LED-CPU-0009 stays cpu_flow"
  - "LED-CPU-0005 is pinned by spd_speed_trace and not by spd_speed as the plan text says: the entry claims only the H8-25 map cells (1 byte, 3 cycles) and the H8-18 power-up speed, which the trace case compares with page literals independently of the timer ratio; cycle_adders_manual items 15 to 18 and spd_speed checks
    1 and 13 are named in the entry comment. LED-CPU-0006 (the switch behaviour, E2) is then pinned by spd_speed"
  - "LED-IRQ-0004 statement is split (a) page facts: register layout, disable bits, priority and vectors; (b) assumptions: a write is the only acknowledge and clears TIQ only, a borrow sets TIQ even when masked, IRQ1 and IRQ2 are levels. The H8-14 conditional TIQ wording against H8-15 and H8-21 is its disagreement and the model follows the unconditional reading; LED-IRQ-0003 points to it"
  - "Status: every entry assumed except LED-IRQ-0005 and LED-CPU-0006 (open, as the model row and the plan say); LED-IRQ-0002, -0004 and LED-CPU-0005 list an E1 item as the existing candidate entries LED-CPU-0001 and -0002 do, counted only after plan 03-32; nothing is promoted"
  - "Emulator readings are labelled reports or code observations and come only from research/PITFALLS.md C5, 03-CONTEXT and the plan SUMMARYs (MAME timer history, the MAME delayed-interrupt changelog, a Mednafen comment on the speed change, the Mesen $7F window named as deferred); none was re-read and no emulator code was read or copied"

requirements-advanced: [CPU-03, CPU-04, CPU-06]
requirements-completed: []

duration: 25min
completed: 2026-11-11
---

# Phase 3 Plan 34: W4 ledger entries for the timer, the interrupt controller and the speed switch Summary

**Nine ledger entries (seven assumed, two open, one with a PENDING pin) now carry the timer, interrupt controller, speed switch and power-on claims of context wave W4 with page facts split from modelling choices, labelled emulator reports, real pins in both directions, and nothing promoted.**

## What was built

- **Task 1, commit 0846804.** LED-IRQ-0002 end to end: the timer step (3072 master clocks, OSC1 / 3 / 1024), the load on a 0 to 1 write and on a borrow and the prescaler reset on a stop as page facts (H8-20, H8-21, H8-22), the period of reload + 1 steps and the untouched counter on a reload write as named derivations
  (A1), pin `timer_pure_function`, its REQUIRED line and the pins back-link. The E1 item is listed, not counted; the in-guest rows (tim_timer 14 to 18, 20, 24) are E2 self-consistency (ER-3).
- **Task 2, commit 47e1a9e.** LED-IRQ-0003 (timer edges: hold on stop is the constant `timer_hold_on_stop`, no-op rewrites, bit 7 from io_buffer, no $7F window, the TIQ latch when masked; pin tim_timer), LED-IRQ-0004 (split as above; pin irq_ctl_pre), LED-IRQ-0005 (open; the two values of `irq_i_sample`; pin
  irq_ctl_post), LED-IRQ-0006 (sample stamp constant `irq_sample_cycles_before_end`, ties inclusive, same-cycle acknowledge, same-step entry; pin irq_ctl_pre), LED-IRQ-0008 (the test-only stimulus, NMI undriven; pin record_stimulus_shown), LED-CPU-0005 (CSL and CSH one byte, three cycles, low speed at reset; pin
  spd_speed_trace), LED-CPU-0006 (open; the switch behaviour, constant `speed_switch_extra_cycles`, alternative last_cycle; pin spd_speed) and LED-PROF-0003 (power-on state under zero, v1 and the not yet existing v2; PENDING:3).
  - LED-CPU-0017 (written by plan 03-20) gained an E2 evidence item for spd_speed checks 9 to 12 and a comment; LED-CPU-0009 (plan 03-15) gained the constants-row comment, the corrected statement that a hardware entry exists since plans 03-21 and 03-22, and an evidence item for the hardware entry checks of tim_timer and irq_ctl_pre. Neither pin nor statement changed.
  - Eight REQUIRED lines with origins D-101 (twice), D-109, D-110, D-111 (twice), D-112 and D-114; six `pins` arrays set.

## Page facts (R-8)

The hardware manual copy in a scratch directory outside the repo (sha256 40a32e5d... equals 03-RESEARCH) was read as page images on 2026-11-11 by this executor: H8-12, H8-13, H8-14, H8-15, H8-16, H8-18, H8-20, H8-21, H8-22 and H8-25. The facts recorded in the FACT sources are the ones seen: the sample in the last bus
cycle (H8-12); priority timer, IRQ1, IRQ2 and the vectors FFF6, FFF8, FFFA (H8-13); the conditional TIQ wording (H8-14: TIQ is set if a borrow occurs with I reset and TIQD 0); the request register bits and TIQ set by a borrow (H8-15); the disable register bits, 1 disables, 0 at reset (H8-16); the reset list including
low speed (H8-18); the prescaler, the load conditions and the invalid reload and counter after reset (H8-20); the stop that resets the prescaler and the 143 usec to 18.3 msec range (H8-21); Fig 2-7-2 (H8-22); the CSL and CSH map cells of 1 byte and 3 cycles (H8-25). Nothing was copied or committed.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage ledger-entries`, `inventory`, `ledger-negatives`; Task 2 `--stage ledger-entries`, `inventory`, `conformance` (about 123 s here).
- Plan end: `sh scripts/check` PASS (about 190 s user time, the full run took six minutes wall here), `sh scripts/check --selftest` PASS (32 s), `sh scripts/audit-history --range 74533fd..HEAD` clean (2 commits, 20 blobs, 1365 blob-path pairs, run before this docs commit). `dash scripts/check` was not run (optional).

## Deviations from Plan

**1. [Plan wording] LED-CPU-0005 is pinned by spd_speed_trace, not spd_speed.** The plan's action pins both CPU-0005 and CPU-0006 to spd_speed; the orchestrator's note and 03-23 summary ask for the case that tests the claim. The 0005 claim is the H8-25 cells and the reset speed, which spd_speed_trace compares with page
literals; spd_speed then pins LED-CPU-0006. This adds `spd_speed_trace.toml` to the touched files (not in the plan's file list).
**2. [Plan wording] The files list gains LED-CPU-0009.toml and LED-CPU-0017.toml.** Both entries already existed (plans 03-15 and 03-20), so they were edited, not rewritten, as the brief asks.
**3. [Rule 1 - stale text] LED-CPU-0009's comment said no hardware interrupt entry exists in the core.** That was true when plan 03-15 wrote it and false since 03-21; the comment now says so and an evidence item names the checks that see a hardware entry.
**4. [Rule 3 - format] Every key is one line of at most 399 characters.** The first draft of LED-IRQ-0002 failed ledger-entries with LINE_TOO_LONG on several lines; the entries put detail in comment lines and keep the arrays short.
**5. [Note] LED-CPU-0017's old evidence text was shortened to make room for one E2 item.** Its meaning is unchanged (the same trace facts and the same "the literals are the assumption" caveat).

No Rule 4 deviation. The `[withheld]` ids line of the run record was not touched (22 ids, cap 24): none of these entries adds an id to the record, because their ids are already printed by the constants and model rows (LED-IRQ-0003, -0005, -0006, LED-CPU-0006, LED-CPU-0009). LED-IRQ-0008 is not in the record and was not added.

## Evidence discipline

Every entry is `assumed` or `open` with E0 evidence; LED-IRQ-0002, -0004 and LED-CPU-0005 also list an E1 item that is counted only after the plan 03-32 re-read, as LED-CPU-0001 and -0002 do. The timer and speed rows of in-guest fixtures are labelled E2 self-consistency (ER-3): they depend on the community-reported 12
master clocks per low-speed cycle and the access stamp rule (LED-CPU-0003, -0004). `timer_pure_function` is the only E1-capable timer claim (LED-IRQ-0002); nothing is promoted (only plan 03-32 may, and only the eight named candidates). The IRQ1 and IRQ2 stimulus is a test hook (ER-5). Every entry lists the four
withheld items (hardware cycle accuracy, interrupt latency in cycles, game compatibility, TG-16 or region claims). No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-98 (ledger overclaim): every entry is assumed or open, ledger-check passes with the new entries, and the E1 items are labelled uncounted. T-03-99 (circular timer evidence): the in-guest rows are recorded as E2 self-consistency and only the C-level test backs the E1-capable statement.

## Notes for next plans

- **03-32 (promotion and provenance):** candidates in this plan are LED-IRQ-0002, LED-IRQ-0004 (part (a) only) and LED-CPU-0005; each lists an E1 item and image reads dated 2026-11-11 (H8-12 to H8-16, H8-18, H8-20 to H8-22, H8-25 re-read here), so the W8 re-read should repeat them. The standing LED-CPU-0015 provenance
  gap is untouched; the emulator readings in the new entries are the ones recorded in research/PITFALLS.md C5 and 03-CONTEXT, with the readings of timer stop (hold or reload), the $7F window and NMI wiring named only as open items (ER-8) because their per-emulator readings are not recorded in the repository.
- **03-24 (power-up v2):** LED-PROF-0003 has pin PENDING:3 and describes the v2 clause as not yet existing; when the poweron_v2 cases exist, set its `pinning_test` to the case that tests it and add the `pins` back-link in the same plan (or leave it to 03-32).
- **03-25 (block transfers):** LED-IRQ-0007 (no interrupt sampled inside a block transfer) is still unwritten; the `[withheld]` ids line holds 22 of 24 ids.
- **Ledger edits:** a ledger key is one line of at most 399 characters; use comment lines for detail.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created entries present: LED-IRQ-0002, -0003, -0004, -0005, -0006, -0008, LED-CPU-0005, -0006 and LED-PROF-0003. Commits 0846804 and 47e1a9e are ancestors of HEAD.
