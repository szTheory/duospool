---
phase: 04-vce-vdc-raw-video
plan: 06
subsystem: vdc-register-port-vram-interface
tags: [vdc, vram, vwr, vrr, write-latch, iw-step, accessor, status-read, mask-table, toy-control, block-transfer-restore, w7]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the folded 1365-clock grid and the vid sink (04-04), vce_pal complete, dev read logging of windows 0 and 1 and the fold-window restore rules (04-19), chunk, restore and observer matrices for video (04-05), the VDC0 state region and the poison/restore machinery (04-03, 04-18)
provides:
  - src/core/vdc.c - window 0 as the HuC6270 register port, decoded on offset & 3: AR keeps 5 bits, ONE static const mask table (D-146) applied after every byte update, R03/R04 and AR $14-$1F ignore writes and read the buffer byte, VWR LSB stores the never-cleared write latch, VWR MSB stores (msb << 8) | latch at MAWR then steps
    MAWR by the IW increment (+1, $20, $40, $80 from CR bits 12-11, even when the store is dropped), the read operation (MARR MSB write and VRR MSB read only) loads the buffer in ONE statement then steps MARR, the CPU receives the old buffer
  - the ONE bounds-checked VRAM accessor pair ds_vdc_vram_read / ds_vdc_vram_write (words at and above vdc_vram_words read vdc_oob_read, writes dropped), the status read (flags & $3F, BSY = vdc_bsy_level, bit 7 = 0, bits 0-5 cleared in ONE statement) and ds_vdc_flag, the single status setter with the D-144 enable gate (no caller until 04-08)
  - bus.c window 0 folds, then reads/writes through vdc.c (the READ log of 04-19 stays); DS_VDC_STUB_READ removed; constants vdc_vram_words, vdc_oob_read (LED-VID-0007) and vdc_bsy_level (LED-VID-0015)
  - fixture vdc_reg (id 16, 37 checks: MAN 17, GUEST 2, ASSUMED 18; identical under zero and two v3 seeds) with fixtures/lib/vdc.inc (macros and equates only, never edited later) and the REQUIRED cases vdc_reg, vdc_vram, mutate_sweep_vdc_reg, vdc_vram_sweep, vdc_regmask
  - kinds vdc_vram (VRAM words from instance bytes against literals restated from the fixture, three policies), vdc_vram_sweep (all 65,536 word addresses through the accessor and the port) and vdc_regmask (masks, line_dirty bits, AR width, reserved ARs, status gate); toy vrr-no-readahead (RED_OK); snapshot_hostile token
    vdc-tia=NAME (restore at four stops inside the TIA and TAI block transfers of the VDC data port)
affects: [04-07, 04-08, 04-09, 04-10, 04-11, 04-13, 04-16, 04-17]

requirements-advanced: [VID-03]
requirements-completed: []  # VID-03 completes in 04-16 (the DMA half lands in 04-10)

actuals:
  tokens: 24800
  tasks: 3
  commits: 3
plan_head_before: 29c93e6f2389307d186f9fe4d8dc89bf0d625b6c
plan_head_after: c9d4dd708199b52f3a21529acf763b324d5cb263

tech-stack:
  added: []
  patterns:
    - "One mask table plus a side-effect hook per register: a byte-wise update of the 16-bit slot, then the AND with the table, so R03 and R04 (mask 0) ignore writes by construction and the AR values above $13 return before any slot is touched"
    - "A VRAM word is checked three ways: through the guest (VRR by LDA), from the instance bytes assembled in the test (kind vdc_vram, no port read path), and through the accessor pair directly (kind vdc_vram_sweep), with a copy of every other instance byte compared around the out-of-range writes"
    - "A pattern item in a literal list (tia=ADDR:W0:STEP:COUNT) restates a table formed by an assembler multiplication, so a 55-word expect stays under the 400-character line and the 512-character key cap"
    - "A restore stop chosen by machine state, not by time: the stop callback tests the block transfer opcode, the remaining byte count and the VDC port in the destination or source, so the stop sits inside the transfer whatever the fixture's timing"

key-files:
  created:
    - src/core/vdc.c
    - fixtures/src/vdc_reg.s
    - fixtures/src/vdc_reg.checks
    - fixtures/lib/vdc.inc
    - fixtures/manifest/vdc_reg.toml
    - tests/conformance/cases/vdc_reg.toml
    - tests/conformance/cases/vdc_vram.toml
    - tests/conformance/cases/mutate_sweep_vdc_reg.toml
    - tests/conformance/cases/vdc_vram_sweep.toml
    - tests/conformance/cases/vdc_regmask.toml
    - tests/controls/vrr-no-readahead/RULE
    - tests/controls/vrr-no-readahead/EXPECT
  modified:
    - src/core/bus.c
    - src/core/ds_core.h
    - src/core/ds_vdc.h
    - src/core/constants.def
    - tests/conformance/vdckinds.c
    - tests/conformance/snapkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/cases/snapshot_hostile.toml
    - tests/conformance/cases/mmu_mpr_256k.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - fixtures/manifest/mmu_mpr.toml
    - ledger/entries/LED-VID-0006.toml
    - ledger/entries/LED-VID-0007.toml
    - .planning/WINDOWS.md

key-decisions:
  - "Reads of the data pair return the read buffer byte (LSB for reg 2, MSB for reg 3) for EVERY AR, not only for the reserved ones; the read operation is started only by an MSB read with AR = 2 (VRR). This makes mmu_mpr check 24 (an MSB data read at reset, AR 0) a buffer read of 0 with no side effect, and a read of $0001 is $00 for every state"
  - "VWR MSB stores the word with the latch itself (msb << 8 | vdc_wlatch), never with the low byte of reg[2]; reg[2] is also updated byte-wise like every register, so a restored state whose latch and slot differ still stores what the latch says"
  - "MARR is judged by vdc_regmask after its read operation: a write of $FFFF reads back $0000 (the MSB write starts the read, which steps MARR by the IW step, MAN H7-5); the mask of MARR is the full 16 bits"
  - "The status-clear statement and the buffer-load statement are each ONE statement on its own line, for the toys: the buffer load is the anchor of vrr-no-readahead (04-06) and the status clear the anchor of status-read-keeps-flags (04-17)"
  - "ds_vdc_flag has no caller yet; its enable gate is judged by the status part of vdc_regmask (flags with enables clear leave the status 0, with them set the read returns the bit pattern and clears it, a raised flag survives a later disable, a flag number above 5 changes nothing)"
  - "mmu_mpr checks 23 and 24 take the class ASSUMED:LED-PROF-0003 (all VDC state is 0 at power-on under every policy): the literal $00 depends on that assumed constant, so by ER-10 the row cannot be a MAN row even though the status read is H7-4"

completed: 2026-11-12
---

# Phase 4 Plan 06: VDC interface Summary

**Window 0 is now the HuC6270 register port: VRAM words move through ST0/ST1/ST2, the window mirrors and block transfers, with the write latch, the IW steps, the read buffer that is one word ahead of the CPU and the one bounds-checked accessor pair, and each of those rules is checked in the guest, from the instance VRAM bytes,
  over all 65,536 word addresses under the sanitizers, and by a toy that makes the guest case red.**

## What was built

- **Task 1, tracer (commit 2aa4dbe).** `vdc.c` (register file, mask table, accessor pair, the three operations, status read, `ds_vdc_flag`), `bus.c` window 0 (fold, then `ds_vdc_read` or `ds_vdc_write`; the READ log of 04-19 untouched; the stub define is gone), three constants rows with both accept goldens and the record
  skeleton (142 to 147 rows) changed by hand from the def (`[constants]` 31 rows, `[withheld]` 32 ids), kind `vdc_vram` (words assembled byte by byte from the instance, run under the three default policies, to the end of the DSV1 verdict), the first slice of `vdc_reg` (4 checks) and the cases `vdc_reg` and `vdc_vram`. The tracer
  verify and the full gate passed on the committed tree before anything was expanded.
- **Task 2 (commit a9c4a70).**
  - `vdc_reg` complete, 37 checks in sections A to M: the buffer is the old word (an overwrite after the MARR write shows one read later), the mirrors $0004-$0007 and $03FC-$03FF, the AR width (`$E2` selects VWR; `$15` names no register and its data write does not reach CR), `$0001`, the idle status, the four IW steps of VWR
    (each landing address read back with CR 0) and the read steps for IW 01 and 11, a lone MARR low byte write and a lone VRR low byte read start nothing, the write latch (a lone LSB stores nothing, the MSB uses the latch, a second MSB re-stores it), the out-of-range words and the wrap of MAWR after a dropped word at $FFFF, the
    reserved R03, the ST triple with MPR0 mapped to work RAM against the sta triple, a 32-word TIA upload checked with an additive running sum against a table the assembler forms by multiplication (GUEST), and a TAI readback into work RAM (ASSUMED LED-CPU-0008).
  - `fixtures/lib/vdc.inc` (macros and equates only), `mutate_sweep_vdc_reg` (37 literals flipped, each turns exactly its own check red), the `vdc_vram` case with 55 words (23 explicit, 32 from a `tia=` pattern).
  - `mmu_mpr` checks 23 and 24: number, count and image bytes unchanged; the class text now names the live VDC (status read, MSB data read of the buffer, both `$00` at reset under every policy).
  - `snapshot_hostile` token `vdc-tia=vdc-reg`: restore at four stops chosen from the block transfer state (TIA with 63, 32 and 1 bytes left, TAI with 33 left), the restored instance equal to the source in every member, and the target run to the deadline equal to one uninterrupted run (time, steps, verdict RAM, digest, every restorable member).
- **Task 3 (commit c9d4dd7).** Kinds `vdc_vram_sweep` (accessor part and port part over all 65,536 word addresses; instance bytes outside VRAM compared with a copy around the out-of-range writes) and `vdc_regmask` (masks, line_dirty, AR, reserved ARs, status), the toy `vrr-no-readahead`, the REQUIRED lines, and the pins
  LED-VID-0006 (`TEST:vdc_reg`) and LED-VID-0007 (`TEST:vdc_vram_sweep`), statuses unchanged (assumed).

## Verification

- Task verifies, run as written: Task 1 (`fixture-lint`, `conformance`, `RESULT vdc_vram pass`, `RESULT vdc_reg pass`, `RESULT cpu_huc pass`), Task 2 (`fixture-lint`, `fixtures`, `conformance`, `RESULT mutate_sweep_vdc_reg pass`, `RESULT mmu_mpr_256k pass`, `RESULT snapshot_hostile pass`) and Task 3 (`conformance`, the two
  RESULT greps, `CONTROL vrr-no-readahead RED_OK`, `--stage sanitizers`): all PASS. Full `sh scripts/check` PASS before each of the three commits (about 3.7 min each). `sh scripts/check --selftest`: PASS. `sh scripts/audit-history --range 29c93e6..HEAD`: clean (3 commits).
- **Linux gate-gcc:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, strict C11, linux/amd64, 52 s); a one-off local run, not a CI claim. `dash scripts/check` and the second awk are owed at the Phase 4 checkpoint and close-out.
- Sanitizers: `vdc_vram_sweep` and `vdc_regmask` pass under the sanitize preset (the stage ran 85 s on this tree).
- **Literals and controls shown able to fail (ER-11):** the toy `vrr-no-readahead` is RED_OK and turns `vdc_reg` guest_failure (`vdc_vram` stays green by design, the toy changes no stored word); the sweep `mutate_sweep_vdc_reg` flips all 37 literals and each flip turns exactly its own check red. Two temporary mutants of `vdc.c`,
  each reverted byte for byte (the file equals its committed content): an HSR mask of `$7F3F` turned `vdc_regmask` guest_failure; the write bound of the accessor moved from `$8000` to `$10000` turned `vdc_vram_sweep` guest_failure (it was NOT red before the instance-bytes compare was added, see deviation 4).
- **F-50 re-verification (Phase 3 fixtures, three policies):** `st_bypass`, `cpu_huc`, `cyc_probe`, `spd_speed`, `mmu_mpr` pass under zero and two v3 seeds (the guest kind's default list). Their VDC reads are status reads (`cyc_probe`, `spd_speed`, `mmu_mpr` check 23, all `$00`) and one MSB data read at reset (`mmu_mpr` check
  24, the buffer, `$00`); `st_bypass` and `cpu_huc` only write the window (the `lda` of `$0002` and `$0003` in `cpu_huc` read work RAM through MPR0). `cpu_huc` writes CR = `$07A6` (ST0 `#$A5`, ST1 `#$A6`, ST2 `#$A7`; bits 1 and 2 are IE for OR and RR, BB set): no event exists yet, so no flag and no IRQ1 arises. **04-08 re-checks
  this with the vertical machine live.**
- Evidence class E0 throughout (repeatability, composability). The literal rows are MAN (H7-3, H7-5, H7-7, H7-25, H8-8: triggers, steps, AR width, the high byte of the status word, the ST triple), GUEST (the 32-word table against an additive sum) and ASSUMED LED-VID-0006, LED-VID-0007, LED-VID-0015, LED-CPU-0008 and
  LED-PROF-0003. No accuracy, compatibility or hardware claim; a pass is not a support claim, and the masks are field widths, not a read-back claim.

## Deviations from Plan

1. **[Note] Case pins stay empty in Task 1 and Task 2.** The inventory stage refuses a case pin whose ledger entry does not name the case (the same rule that moved the LED-VID-0004 pin in 04-05), so `vdc_reg` and `vdc_vram` carried `pins = []` until Task 3 edited the ledger; Task 3 sets `vdc_reg` to `["LED-VID-0006"]` and `vdc_vram_sweep` carries `["LED-VID-0007"]`.
2. **[Rule 3 - Blocking] `vdc_vram` has a `tia=ADDR:W0:STEP:COUNT` item.** 55 explicit `ADDR:VALUE` words are 769 characters, over the 400-character line rule and the 512-character key cap; the 32 words of the TIA table are restated as the formula the assembler uses (`($1234 + $F7 * i) & $FFFF`), a multiplication, still independent of the guest's additive sum and of the core.
3. **[Rule 1 - Bug] `vdc_regmask` status part started with DCR = `$001F`.** The masks part leaves DCR at its mask, so DS and DV were enabled and the first "enables clear" step saw flags (a test fault, not a core fault); the part now clears CR and DCR itself first.
4. **[Rule 2 - Missing critical check] `vdc_vram_sweep` compares every instance byte outside the VRAM array around the out-of-range writes.** A mutant that moved the write bound from `$8000` to `$10000` stored past the VRAM array (into SATB and beyond) and the first version of the kind, which looked only at VRAM bytes and at
  reads, stayed green. The kind now takes a copy of the instance before the out-of-range writes and requires every byte outside the 65,536 VRAM bytes equal after them (T-04-20).
5. **[Note] The block-transfer restore stop is a token of `snapshot_hostile`, not a new restore class.** `vdc-tia=NAME` (parsed after `mid-block`) runs a second image under its own seeded-prng-v3 requests (seed 5 for the source, 6 for the target); the stops are chosen by the block transfer state (opcode, bytes left, VDC port in
  the destination or source). `snapshot_restore` was not touched.
6. **[Note] mmu_mpr: only text changed.** The three images are byte for byte the same (the source changed in two comments), so the pins of `mmu_mpr.toml`, `mmu_mpr_1m.toml` and `mmu_mpr_384k.toml` are unchanged and `fixtures` stayed green; only the meaning text of `mmu_mpr.toml` and of the case `mmu_mpr_256k` and the two check
  rows (class `ASSUMED:LED-PROF-0003`) changed. The manifest meaning line was kept under 400 characters by a shorter phrase.
7. **[Note] Files beyond the plan list:** `tests/conformance/cases/mmu_mpr_256k.toml` (meaning text), `src/core/ds_vdc.h` (declarations; it was in the list), `.planning/WINDOWS.md` (one stub entry, below). `CMakeLists.txt` was not touched: the core sources are globbed, so `vdc.c` joined the build by itself.
8. **[Note] `vdc_vram` and the guest cases rely on the run end.** `vdc_reg` runs to `until_mclk` 2,000,000 (a guest case does not stop at DSV1 done) and `vdc_vram` stops at the end of the verdict (machine time 250,116); both cases report their machine time.
9. **[Note] Plan estimate was 105,000 tokens; the realized diff is about 24,800** by chars/4 over the added lines (31 files, 1,535 insertions, 30 deletions).

## Known Stubs

- **LENR and DVSSR MSB writes are stored only** (the plan's prohibition): no VRAM-to-VRAM DMA and no SATB transfer starts until 04-10. Recorded in `.planning/WINDOWS.md` as a stub entry of this plan.
- **`ds_vdc_flag` has no caller**: no status flag is raised by an event until 04-08 (the plan's prohibition). Its gate and the status read are judged by `vdc_regmask`. `.planning/WINDOWS.md` id 4 (the vertical machine) stays open.

## Threat Flags

None. T-04-20 (accessor with guest addresses) is mitigated and proven live by the 65,536-address sweep with the instance-bytes compare and the write-bound mutant; T-04-21 (register stores and increments) by the one mask table, the unsigned modulo-65,536 arithmetic and `vdc_regmask`; T-04-22 (Phase 3 fixtures writing CR with IE
bits) by the three-policy rerun and the absence of any event that raises a flag; T-04-23 (a symmetric write and read bug) by `vdc_vram` (instance bytes) and the toy `vrr-no-readahead`.

## Notes for next plans

- **`vdc.c` is where the later side effects go.** The MSB branch of `ds_vdc_write` has two empty arms for LENR (VRAM DMA start) and DVSSR (SATB pending), both for 04-10; the register slots, the masks and the line_dirty table are final. The BYR write path that 04-11 and 04-12 anchor on (a `bg_y` assignment) does not exist yet: they add it to `ds_vdc_write`.
- **04-17 anchors:** the status-clear statement is `inst->vdc_status = (uint8_t)((uint32_t)inst->vdc_status & 0xC0u);` (one statement) in `ds_vdc_read`; the toy `status-read-keeps-flags` replaces it by an empty statement.
- **Dev log:** a read of window 0 is logged with its returned value (04-19). The six `dev_log_records` literals read status `$00` and stay so only while no flag is raised; the VDC reads of `vdc_reg` (over a hundred, the readback loops and the TAI transfer included) would add records, so a guest case with a dev sink should watch `lost`. `vdc_reg` itself has no sink.
- **`vdc_vram` is the shape for `vdc_satb` (04-10):** its kind parses `rom=` and a word list; a sibling for the SATB words can reuse `vv_parse_words`. The `tia=` pattern item is for any table the assembler forms by a formula.
- **`vdc.inc` is frozen** (like `vce.inc`); later features create `vdma.inc`, `bg.inc`, `spr.inc`, `spin.inc`. Scratch cells `$2420`-`$2423` (and the TAI buffer `$2800`-`$283F`) belong to `vdc.inc`; `vce.inc` uses `$2410`-`$241A`.
- **CR in guests:** `vdc_reg` writes only IW values to CR (`$0800`, `$1000`, `$1800`) and returns it to `$0000`, so IE bits are never set. 04-08's event machine must still run this fixture clean, and the `vdc-tia` restore stops of `snapshot_hostile` run it too.
- **Cost:** the new cases bring `conformance` to about 49 s wall on this machine (trigger 60 s) and the sweep adds little to the sanitizers stage; D-176 levers apply before `conformance` crosses 60 s.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: src/core/vdc.c, fixtures/src/vdc_reg.s, fixtures/src/vdc_reg.checks, fixtures/lib/vdc.inc, fixtures/manifest/vdc_reg.toml, tests/conformance/cases/vdc_reg.toml, vdc_vram.toml, mutate_sweep_vdc_reg.toml, vdc_vram_sweep.toml, vdc_regmask.toml, tests/controls/vrr-no-readahead/RULE and EXPECT, REQUIRED lines for all five cases.
- FOUND commits 2aa4dbe, a9c4a70 and c9d4dd7 (ancestors of HEAD).
