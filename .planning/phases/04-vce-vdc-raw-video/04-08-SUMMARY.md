---
phase: 04-vce-vdc-raw-video
plan: 08
subsystem: vdc-raster-timeline-status-irq1
tags: [vdc, beam, vertical-cycle, rcr-counter, vblank, status-flags, irq1, model-params, vdc-stat, model-param-vdc, dev-log-stamps, w4]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the folded 1365-clock grid, the line ladder skeleton and the record sink (04-04), the VDC register port and ds_vdc_flag (04-06), the video judge and the golden role (04-07), the model rows vd_irq_phase and rcr_origin and the BEAM state (04-03)
provides:
  - src/core/beam.c - the vertical machine (VDS+2, VDW+1, VCR, VSW+1 lines, restarted in VDS at frame line 0, an unfinished state cut, a VCR of 0 skipped, v_togo = lines still to come after the current line, display_k from 0 inside VDW), the RCR counter (add 1 saturating at $7FFF, then set to 64 on the origin line of rcr_origin), the EV_VD and EV_RCR events of the ladder
    at their offsets (EV_RCR at (HDS+HDW+2)*8*div, none from 1365; EV_VD at min((HDS+1)*8*div, 1364) for hdw_start, at the EV_LINE itself for line_start) and the VBlank state entry latch of MWR
  - src/core/irq.c - the IRQ1 bit of the request register is the declared stimulus level OR (vdc_status & $3F) != 0; delivery still gated by $1402 and the I flag; src/core/ds_core.h documents ds_irq_pending_query as valid for a folded instance only (F-21)
  - src/core/vid.c - the record fields v_state, active_k (0xFFFF off display), the ACTIVE flag on display lines and the stepping rcr_counter
  - fixtures/src/vdc_stat.s (id 17, 15 checks: MAN 6, ASSUMED 9), its manifest and checks file, the cases vdc_stat and mutate_sweep_vdc_stat, and the kind and case model_param_vdc (three runs of the guest with the dev sink, stamps of the status reads)
  - the vce_pal golden digest repinned (old and new in the .vexp comment)
affects: [04-09, 04-10, 04-11, 04-12, 04-13, 04-15, 04-16, 04-17]

requirements-advanced: [VID-04]
requirements-completed: []  # VID-04 completes in 04-17

actuals:
  tokens: 11300
  tasks: 2
  commits: 2
plan_head_before: 648c5b2fbe9cbe17a6ec88e45ff8a4f1ef9f7fed
plan_head_after: f510be4186952654fd97bc844f1d7e6ea667e12c

tech-stack:
  added: []
  patterns:
    - "A gating row is made able to fail by keeping the delivery mask set: with the I flag set a flag that the gate let through stays in the status for the foreground read, with the I flag clear the handler would acknowledge it first and the row would read 0 either way (found by a core mutant, deviation 3)"
    - "Foreground waits are counted loops and polls of $1403 (a poll changes nothing); the interrupt-driven reads are the ones whose stamps are judged, so the longest instruction of the wait loop bounds the jitter of a stamp shift"
    - "A model-parameter kind judges SHIFTS between runs of one ROM from the dev log (the status reads of $1FE000 with their access stamps), with literals that are arithmetic on the fixture setup and the parameter statement, never a stamp copied from the core"

key-files:
  created:
    - fixtures/src/vdc_stat.s
    - fixtures/src/vdc_stat.checks
    - fixtures/manifest/vdc_stat.toml
    - tests/conformance/cases/vdc_stat.toml
    - tests/conformance/cases/mutate_sweep_vdc_stat.toml
    - tests/conformance/cases/model_param_vdc.toml
  modified:
    - src/core/beam.c
    - src/core/irq.c
    - src/core/vdc.c
    - src/core/vid.c
    - src/core/ds_core.h
    - fixtures/src/vce_pal.vexp
    - tests/conformance/vdckinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - .planning/WINDOWS.md

key-decisions:
  - "v_togo counts the lines of the current state still to come AFTER the current line (so a state of N lines enters with N - 1 and the next state is entered when it is 0); display_k is 0 on the first VDW line and 0xFFFF in every other state; the largest value is 511, inside the F-1 bound of 512"
  - "The VBlank state entry is the transition out of VDW (the first line after the last display line); a frame wrap that cuts VDW, or that falls exactly on the line after it, enters VDS and raises no VD (LED-IRQ-0001)"
  - "rcr_origin first_active: the origin line is v_state VDW with display_k 0; line_before: v_state VDS with v_togo 0 (the last VDS line). The set-to-64 is the one statement inst->rcr_counter = (uint16_t)64u; in rcr_step of beam.c (the anchor of the toy vid-rcr-base of 04-17)"
  - "When EV_RCR exists (offset below 1365) the window start (HDS+1)*8*div is strictly before it, so the stage order EV_VD then EV_RCR is also the time order; the fold needs no sorting"
  - "The golden repin moved into Task 1 together with vid.c: the vertical machine changes v_state and rcr_counter in every record at once, so the Task 1 commit needed the new digest to be green; the vid.c fields (active_k, ACTIVE) came with it so that there is one repin, not two"
  - "model_param_vdc tolerance is 15 master clocks (lda abs, 5 cycles at 3 clocks, the longest instruction of the fixture's wait loops), inside the plan's bound of 21; the kind refuses a tol above 21"

completed: 2026-11-12
---

# Phase 4 Plan 08: Raster timeline, status and IRQ1 Summary

**The VDC now runs a vertical cycle and an RCR counter, raises RR and VD at their modelled positions through the one gated status setter, and drives the IRQ1 level of the interrupt controller; a guest fixture takes both interrupts, reads and acknowledges the status in its handler and records the bytes in guest RAM, a sweep flips every literal of it, and a case shows from the
  dev log that the two model parameters move the VD stamp by +96 and the RR stamp by -1365 master clocks.**

## What was built

- **Task 1, tracer (commit 85d0dfa).** `beam.c`: at EV_LINE, in the order of D-138, the vertical machine advances, the RCR counter steps, the divider is latched and the two event offsets are computed, and the VBlank entry latches MWR and raises VD (phase line_start) or leaves it for EV_VD (hdw_start); the events EV_VD and EV_RCR of the fold call `ds_vdc_flag`, whose enable
  gate is read at that
  instant (CR bit 3 for VD, bit 2 for RR). `irq.c` ORs the VDC level into IRQ1 through a local, and its header comment says what drives the lines now. `ds_core.h` documents the folded-instance rule of `ds_irq_pending_query`. `vid.c` fills v_state, active_k and the ACTIVE flag. `vdc_stat` (id 17) in its first slice: the RR row (status 0 with the enable clear, $04 in the
  handler, $00 on the second
  read). The vce_pal golden was repinned (6c510913... to c667ac7a...; the .vexp comment carries both and the procedure).
- **Task 2 (commit f510be4).** `vdc_stat` complete for the RR and VD rows (15 checks, sections A to E, see the header comment of the source), `mutate_sweep_vdc_stat` (15 literals), and `model_param_vdc` with its kind `kind_model_param_vdc` in `vdckinds.c` (registered in `runner.c` and `runner.h`).

## Verification

- Task verifies, run as written: Task 1 (`--stage fixture-lint`, `--stage conformance`, the three RESULT greps vdc_stat, irq_ctl_pre, cpu_huc) and Task 2 (`--stage conformance` inside the full gate, the RESULT greps vdc_stat, mutate_sweep_vdc_stat, model_param_vdc, video_vce_pal): all PASS. Full `sh scripts/check` PASS before each commit (58 stages, about 3.7 min). `sh
  scripts/check --selftest` PASS (140 SELFTEST lines, no FAIL).
  `sh scripts/audit-history --range 648c5b2..HEAD` clean (2 commits). Stage times of the final tree: conformance 45 s, sanitizers 87 s, determinism 34 s (the conformance trigger of 60 s is not reached).
- **Linux:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13.3, strict C11, -Wconversion -Wshadow -Werror, linux/amd64, 53 s); a one-off local run, not a CI claim. `dash scripts/check` and the second-awk whole-gate runs are owed at the Phase 4 checkpoint and close-out.
- **Phase 3 interrupt fixtures with the VDC level ORed in:** irq_ctl_pre, irq_ctl_post, brk_irq_sample, cpu_blk_irq_pending and cpu_huc pass (the whole conformance stage is green). cpu_huc writes CR = $07A6 (IE for OR and RR) with every vertical register 0: the cycle is 4 lines and the RCR counter runs 64 to 67 against RCR = 0, so no RR flag and no IRQ1 arises (the re-check
  that 04-06 asked for, done with the vertical machine live).
- **Literals and controls shown able to fail (ER-11), each on the built runner with the source restored byte for byte afterwards (cmp):**
  - vdc_stat: RR ungated (the flag set whatever CR bit 2 says) turns it guest_failure; VD ungated turns it guest_failure; a status read that does not clear turns it guest_failure; no VD raised in phase line_start turns it guest_failure; the IRQ1 level not driven from the status turns it guest_failure; the RCR origin never reached turns it guest_failure. A counter base of 63
  leaves vdc_stat green by design (it judges no
    position; 04-17 owns the toy vid-rcr-base).
  - model_param_vdc: vd_irq_phase ignored (hdw_start behaves as line_start), rcr_origin ignored, and the VD offset +40 each turn it guest_failure; an offset +6 (inside the tolerance) stays pass, as the tolerance says.
  - The first version of vdc_stat check 1 stayed green against an RR flag that ignored its enable (deviation 3); the fixture was fixed and the mutant is now red.
- **Measured stamp shifts (evidence text, E0, not a literal):** VD +105 and RR -1365 master clocks against the default run; the case literals are +96 and -1365 with a tolerance of 15.
- Evidence class E0 throughout (repeatability, composability, the model against literals written from arithmetic). The vdc_stat rows are MAN H7-4 (flag layout, set only with the enable, cleared by the read) and ASSUMED LED-IRQ-0009 (the level follows the status and is delivered once), LED-VID-0001 (rows that need the frame to be longer than the distance from the VBlank the
  fixture synchronises on to the next RCR match:
  checks 8 and 14) and LED-VID-0015 (BSY reads 0). model_param_vdc is ASSUMED LED-IRQ-0001 and LED-IRQ-0010. No accuracy, compatibility, raster-accuracy or hardware claim; a pass is not a support claim; positions are labelled modelling assumptions. No ledger status changed.

## Deviations from Plan

1. **[Rule 3 - Blocking] A new runner kind, `model_param_vdc`.** The plan lists only the case file, but nothing in the runner judges the stamps of two runs from the dev log. The kind (vdckinds.c, registered in runner.c and runner.h) runs the ROM with the default request and with each of the two bound requests, takes the VD stamp (the last status read with bit 5 set, the one in
  the handler) and the RR stamp (the first read with bit 2 set), and judges the two shifts and two cross rows (each parameter leaves the other event where it was) against the literals of the case. The case needs `request` with two requests separated by `||`.
2. **[Note] Task 1 carries the record fields and the golden repin.** vid.c (v_state, active_k, ACTIVE) and fixtures/src/vce_pal.vexp are listed under the plan, not under Task 1; the Task 1 core change already changes v_state and rcr_counter in every record, so the Task 1 commit needed the repin to be green, and moving the vid.c fields with it avoided a second repin. The
  literal case video_vce_pal passed on the changed records before the digest was changed; the digest was recomputed with `shasum -a 256` over the --video-out file.
3. **[Rule 1 - Bug] vdc_stat check 1 was vacuous against an ungated flag.** The first version enabled interrupts before the disabled wait; a flag that the gate let through would have been taken and acknowledged by the handler before the foreground read, so the row read 0 either way (a temporary mutant that ignored the enable stayed green). Interrupts now stay masked (I set)
  through every wait whose enable is clear, and CLI follows the write that enables. The mutant is red.
4. **[Note] The vdc_stat design is five sections (A to E) with a VBlank interrupt used as the synchronisation point for the RR section**, so that no row needs an absolute line: the RCR match is tens of lines after the VBlank handler in every bound variant. Two rows (8, 14) take ASSUMED LED-VID-0001 because their literal 0 and $04 depend on that distance and on the frame being
  longer than it (ER-10).
5. **[Note] Tolerance 15, not 21.** The wait loops are lda abs, cmp #, bcs, dex, bne at the high speed; the longest instruction is 5 cycles x 3 = 15 master clocks. The kind refuses a tol above 21.
6. **[Note] Files beyond the plan list:** tests/conformance/vdckinds.c, runner.c, runner.h (deviation 1); `.planning/WINDOWS.md` (entry 4, the inert ladder, marked fixed through the windows tool; entry 5 stays open for the DMA stubs of 04-10, its remark that ds_vdc_flag has no caller is now stale).
7. **[Note] An early requirement tick was reverted.** The standard plan-complete step ran `requirements.mark-complete VID-04`, which ticked VID-04 in REQUIREMENTS.md although this plan only advances it (completion is 04-17, ticking is gated by requirements.ready-ids). The change to REQUIREMENTS.md was discarded with a file-specific checkout before any commit (the file equals HEAD), so nothing was ticked and 04-17 ticks it through the normal step.
8. **[Note] Plan estimate was 75,000 tokens; the realized diff is about 11,300** by chars/4 over the added lines (16 files, 649 insertions, 14 deletions). The stop rule did not fire.

## Known Stubs

None added. The SATB copy hook at the VBlank entry and the DMA step of the ladder (item 1 of D-138) stay empty until 04-10, as the plan says; DS, DV, OR and CR flags have no raiser yet (`.planning/WINDOWS.md` entry 5 covers the DMA half). Records still have no active window (04-09).

## Threat Flags

None. T-04-28 (event offsets from guest registers) is mitigated: the offsets are computed in u32 (at most (127+127+2)*8*4 = 8192), EV_RCR is dropped from 1365 on, EV_VD is capped at 1364, the fold loop keeps its per-fold bound, v_togo is at most 511. T-04-29 (IRQ1 level in Phase 3 fixtures) by the gate in `ds_vdc_flag`, the zero registers under every policy, and the Phase 3
  interrupt cases staying green with their stimulus; no video case declares a stimulus.

## Notes for next plans

- **04-17 anchors.** The RCR set-to-64 is the one statement `inst->rcr_counter = (uint16_t)64u;` in `rcr_step` of `src/core/beam.c`; the status clear is the unchanged statement `inst->vdc_status = (uint8_t)((uint32_t)inst->vdc_status & 0xC0u);` in `ds_vdc_read`. vdc_stat has no position row, so the toy vid-rcr-base leaves it green (the plan says it goes to the vdc_irq cases);
  status-read-keeps-flags turns it red (checked with a mutant).
- **Positions** (all ASSUMED, 04-17 measures them in guest RAM): with HDS = 2, HDW = 31 and divider 4 the EV_RCR offset is 1120 and the hdw_start VD offset 96. model_param_vdc already shows +96 and -1365 from the dev log.
- **Editing vdc_stat later (04-10 DS and DV rows, 04-15 OR and CR rows).** A later row changes the image, so the manifest sha, the checks file, the case `checks` and the sweep `checks` move together. model_param_vdc takes the VD stamp from the LAST status read with bit 5 set and the RR stamp from the FIRST read with bit 2 set: a new row must not read a status with those bits
  set after the VD handler or before
  the RR handler, and must keep the wait loops (lda abs, 5 cycles) as the only foreground code between an event and its handler or the tolerance needs a new derivation. The dev sink of the kind holds 512 records (the fixture makes under 100).
- **Vertical machine semantics for the renderer (04-09 on).** `v_state` and `display_k` are final for the line when the record is rendered (item 2 runs before item 7); `active_k` and the ACTIVE flag come from them; no active window exists yet (active_x, active_w are 0).
- **Golden.** Any plan that changes records repins `video_vce_pal_golden` in the same commit (independent `shasum -a 256` of the --video-out file; make `build/gate/video` first, remove it after: the gate refuses a stray directory). Current digest c667ac7ada36e13b97178cf7a1c011e4494b6f4b3ec79359b894eafddaf35253.
- **Owed:** `dash scripts/check` and the second-awk whole-gate runs at the Phase 4 checkpoint and close-out. Windows entry 5 text could be refreshed when 04-10 closes it.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: fixtures/src/vdc_stat.s, fixtures/src/vdc_stat.checks, fixtures/manifest/vdc_stat.toml, tests/conformance/cases/vdc_stat.toml, mutate_sweep_vdc_stat.toml, model_param_vdc.toml, REQUIRED lines for vdc_stat, mutate_sweep_vdc_stat and model_param_vdc, the kind in vdckinds.c.
- FOUND commits 85d0dfa and f510be4 (ancestors of HEAD).
