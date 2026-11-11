# Phase 3 plan check, round 1

Verdict: REVISE. 4 blockers, 11 warnings, 10 info. Saved by the orchestrator from the plan checker's hand-back.

## Blockers

- B1 No 03-VALIDATION.md (nyquist). Fix: generate from the RESEARCH Validation Architecture. Orchestrator step.
- B2 RESEARCH Open Questions not marked resolved. Fixed by the orchestrator (RESOLVED markers added).
- B3 Plans 25 and 26: nothing makes an interrupt pending during a block transfer, and LED-IRQ-0007 pins cpu_blk, which does not test it. Fix: a cpu_blk check with a timer or irq_stim event inside a long transfer, a handler recording bytes already written, A/X/Y checked after RTI, class ASSUMED:LED-IRQ-0007, included in chunk_equiv and observer with a stop at a block-byte boundary.
- B4 Plans 15, 27, 29: no mutate_sweep for pad_2btn or cyc_probe (D-126, ER-7). Fix: add the sweeps (the sweep kind must push the case's input events), or record why cyc_probe has no patchable literals.

## Warnings

- W1 Plans 05, 08, 09, 28: CR-01 lesson not carried. New checkers must test awk status and the stage-1 sentinel, with a dead-stage control.
- W2 Plans 01, 05: DSV1 judge maps badsum and wrongid to harness_failure while D-124 says guest_failure. Record one reading. Add a runner-side poisoned-RAM control for the t=0 precondition.
- W3 Plans 03 (tasks 1, 2) and 22 (task 2): version or row changes precede golden regeneration, so the task verify cannot pass. Regenerate in the same task.
- W4 Plan 25: the block-progress-lost toy expects cpu_blk, an uninterrupted run. Move the toy EXPECT to plan 26 or add a stop-and-resume check in 25.
- W5 Plan 30: D-121 host shasum comparison of the snapshot has no implementing task.
- W6 Plan 18: the [run] record table is exercised only with a NULL run. Add record_run_shown.
- W7 Plans 18, 23, 32: named constants' ledger ids are not printed in the record (D-113). Add them to [withheld] or a [constants] table.
- W8 Plans 26, 29, 30: no chunk_equiv for pad_2btn or irq_ctl. Add chunk_equiv_pad_2btn_correct and chunk_equiv_irq_ctl_pre and one pad stop in the sampled restores.
- W9 Plans 10, 11, 15, 20, 28, 29: several new checks have only a scratch observed-once note. Register selftest mutation blocks (outside the toy cap).
- W10 Plans 07, 15, 20, 23, 28, 29 (D-81) and ten plans near the smart-zone budget: enumerate real file counts, split plan 23's ledger task.
- W11 Plan 01 task 1: verify does not exercise the new path. Move the first guest case into task 1.

## Info

I1 one plan per wave is acceptable. I2 tracers present. I3 split truths cosmetic. I4 files_modified omissions in plans 21, 22, 26, 08, 09. I5 pin limits. I6 D-97 count-only cross-check. I7 D-55 interleave on synthetic images. I8 LED-TOOL-0001 NONE pin allowed. I9 claim-word lint covers README only. I10 D-122 toy moved to a selftest mutation, reasonable.
