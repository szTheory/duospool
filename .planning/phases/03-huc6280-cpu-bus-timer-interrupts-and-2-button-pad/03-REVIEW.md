---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 27
files_reviewed_list:
  - src/core/cpu.c
  - src/core/cpu_ops.def
  - src/core/bus.c
  - src/core/timer.c
  - src/core/irq.c
  - src/core/io.c
  - src/core/run.c
  - src/core/snapshot.c
  - src/core/serial.c
  - src/core/obs.c
  - src/core/model.c
  - src/core/poweron.c
  - src/core/boot.c
  - src/core/ds_core.h
  - src/core/ds_obs.h
  - src/core/ds_snapshot.h
  - src/core/ds_timer.h
  - src/core/ds_serial.h
  - src/core/constants.def
  - src/core/model_params.def
  - src/core/input_refusals.def
  - src/core/run_refusals.def
  - src/core/snapshot_refusals.def
  - src/core/stim_refusals.def
  - src/core/model_refusals.def
  - src/text/text.c
  - src/text/ds_text.h
findings:
  critical: 0
  warning: 3
  info: 3
  total: 6
  necessity: 0
status: issues_found
---

# Phase 3: Code Review Report

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 27
**Status:** issues_found

## Summary

I read all 27 files in full, plus `src/core/mapper.c` (the ROM read path the bus calls), and checked the findings below against `03-CONTEXT.md` (D-110, D-111, D-121) and the headers' own contracts.

The memory-safety surface looks sound. The bank-class table is 256 entries by static assert and `mpr[]` is `uint8_t`. The ROM index is bounded twice, and the 384K mirror stays inside 48 banks. Restore pass 1 writes nothing, and the SCHD tail read stays inside the checked region. The sink checks cover every pair of struct and record range with difference-based overlap tests that cannot wrap. The timer closed form has no zero divisor and no wrapping product, since `n * 3072` is at most `stamp - origin`. The guard in `ds_run` really does bound a step: I counted the accesses of the worst rows (ADC (ind),X in T mode: 7 accesses against a table count of 7; block prologue: 9 accesses against 17).

I did not stop at reading. I compiled the core with `-fsanitize=address,undefined -fno-sanitize-recover` in a scratch directory (`/private/tmp/claude-501/-Users-jon-projects-duospool/06f2647a-e8db-469c-a561-38fda7909897/scratchpad/h/`) and ran three checks. The first was 80,000 random bit-flip mutants of live snapshots through `ds_restore`. Every refusal left the digest unchanged, every accepted mutant round-tripped byte for byte, and there were no sanitizer reports. The second ran 300 random ROMs, biased toward block transfers, TAM/TMA, ST0-2, CLI/SEI and BRK, with an input schedule and an IRQ stimulus. Each ran to a deadline in one call and again with a stop callback firing after every step, and the final digests and machine times were identical (7.1 million stops inside block transfers). The third was targeted repros for the first two findings below. `sh scripts/check --stage build` passes.

There are no Critical findings. The three warnings are a modelling-rule gap that the default parameter value turns into wrong behaviour (BRK), an observer-record field that contradicts its documented meaning (dev sink `pc`), and a restore validation gap against the stated "reachable states only" invariant.

## Warnings

### WR-01: BRK under the default `irq_i_sample = pre` lets a pending IRQ pre-empt the BRK handler before its first instruction

**File:** `src/core/cpu.c:1050-1054` (BRK sets I), `src/core/cpu.c:1083` (sample uses `i_before`)
**Issue:** D-111 scopes the `pre` rule to "the sample after CLI, SEI, PLP and RTI". The code applies `i_before` to every instruction, and BRK is the one other instruction that changes I. With the default `pre`, BRK sets I at line 1054 but the sample at 1083 still sees the I value from before the BRK. If I was clear and an IRQ line or TIQ is pending (a level held by the stimulus, or a latched timer borrow), the entry follows the BRK in the same step. The handler never runs one instruction.

I reproduced this in a scratch harness. The ROM is `CLI; BRK`, the BRK vector points at `LDA #$42` and the IRQ1 vector at `LDX #$77`, with an IRQ1 level high from time 0:
```
step 0 (CLI): pc=E001 p=00 s=00
step 1 (BRK): pc=E100 p=04 s=FA t=216   <- 192 clocks = 8 (BRK) + 8 (entry); S dropped by 6 (two frames)
step 2:       pc=E102 x=77              <- LDX of the IRQ1 handler; the BRK handler's LDA never ran
```
BRK's own frame is left on the stack and the IRQ frame is pushed over it. The hardware interrupt entry sets I the same way, so no instruction that sets I as part of its own entry belongs under the CLI/SEI/PLP/RTI latency rule. This is a modelling choice stated nowhere. The ledger entry for `irq_i_sample` (LED-IRQ-0005) lists four instructions, and BRK is not among them.

**Fix:** Treat BRK like the interrupt entry for the sample (use the post value), and say so in D-111 and the ledger entry:
```c
i_sample = (inst->irq_i_sample == (uint8_t)DS_IRQ_I_SAMPLE_POST || mn == (uint32_t)MN_BRK)
               ? ((uint32_t)inst->p & P_I)
               : i_before;
```
Then add a fixture or case for BRK with a pending IRQ under both parameter values. This changes behaviour, so bump `DS_BEHAVIOUR_EPOCH`. If the project instead wants the current behaviour, record it as an explicit assumption with its own ledger id, because D-111 as written does not say it.

### WR-02: Dev-sink record `pc` is the next instruction's address for block-transfer byte steps, contradicting the documented "the instruction's address"

**File:** `src/core/obs.c:262`, `src/core/ds_obs.h:14`, `src/core/cpu.c:605` (prologue sets `inst->pc = pc + 7`), `src/core/cpu.c:1081` (`inst->pc = next` before the entry runs)
**Issue:** `ds_obs_dev` writes `inst->pc` into bytes +16..+17 of every record, and `ds_obs.h` documents that field as "the instruction's address". That only holds while `inst->pc` is still the old value, that is, for ordinary instructions. The block prologue advances `inst->pc` to `pc + 7`, so every later byte step logs the address after the transfer. Interrupt-entry accesses log `next`, because `inst->pc` is updated at line 1081 before `sample_and_enter`.

I confirmed this for the block case. The ROM is `LDA #$FF; TAM #2; TII $E000,$2002,3`, with the TII at `$E004`, run with a dev sink:
```
rec 0..2 (prologue pushes)  pc=E004
rec 3..5 (TII byte writes)  pc=E00B     <- next instruction, not the TII
```
Within one transfer the prologue's records say `E004` and the byte records say `E00B`, while the insn sink's single record for the transfer says `E004`. A consumer that joins dev records to insn records by `pc`, which is the point of carrying the field, loses every VRAM-DMA write (TIA/TII to the VDC is the usual use). The pc is also what makes the evidence legible, so this is an evidence-integrity defect rather than only cosmetic.

**Fix:** Do not read `inst->pc` in the observer. Put the address in `struct ds_bus` (for example `uint16_t pc`, set by `ds_cpu_step_obs`), and for block byte steps and epilogues set it to `(uint16_t)(inst->pc - 7u)`, which is the transfer's opcode address under the same 16-bit wrap. State in `ds_obs.h` what the entry-time records carry, which is the interrupted `next`. Then `ds_obs_dev` takes the pc as a parameter. A conformance case that dev-logs a TII/TIA and checks that all of its records carry the opcode address would pin this.

### WR-03: `ds_restore` accepts an unreachable timer state (`timer_origin` after the machine time), contradicting the stated "reachable states only" domain rule

**File:** `src/core/snapshot.c:366-367` (`dec_timr`), contrast with the claim at `src/core/snapshot.c:10`
**Issue:** The header comment of `snapshot.c` says the field domains are "the tests of the decoders below" and that "the instance can reach all of them and only them". `dec_timr` checks `counter`, `reload` and `enabled` against their ranges and takes `origin` as any `u64`, justified by "the closed form only subtracts it from a later stamp when that is above it, so no value can wrap". That is true for wrap safety, but the timer invariant is stronger. Every write that sets `timer_origin` uses a stamp that is at most the end of the instruction doing the write, and `ds_timer_settle` only moves it to a step boundary at or below the access stamp. A state between steps therefore always has `timer_origin <= machine_time`.

A stream with `enabled = 1` and `origin` far above `TIME` (for example 2^62 against a machine time of 100) is accepted. `fold` returns no steps for every stamp below the origin, so the timer never counts and never borrows, and the machine behaves like one the emulator cannot produce. That is state a guest cannot reach, silently accepted and then carried by the digest as if it were a reachable machine. It also means the round-trip evidence (E0: repeatability) cannot distinguish it from a genuine snapshot. The same reasoning applies to a few other cross-field relations that restore does not check, for example `blk_active = 1` at `TIME = 0`, but the timer origin is the one that changes observable timing.

**Fix:** Make `dec_timr` judge `origin` against the machine time of the same stream. `walk` already carries a `vctx` between regions, and `TIME` is decoded before `TIMR`:
```c
/* in dec_time (validate branch): v->time = t; */
/* in dec_timr (validate branch): */
if (origin > v->time) { return (uint32_t)DS_SNAPREF_field; }
```
The disabled case is reachable too, since a stopped timer keeps a stale origin, but that origin is still at most the machine time, so the one check covers both. Add a restore-refusal case for `origin > time`.

## Info

### IN-01: Opcode and address constants are duplicated across files instead of being derived from one place

**File:** `src/core/snapshot.c:450`, `src/core/run.c:47`, `src/core/cpu.c:848-854`, `src/core/bus.c:103`
**Issue:** Four facts are copied as literals into a second file:
- `dec_blkt` hard-codes the five block-transfer opcodes (`0x73`, `0xC3`, `0xD3`, `0xE3`, `0xF3`) and the "toggle only for `0xE3` and `0xF3`" rule. `cpu.c` owns the same set as `BLK_OPCODE_*`.
- `ds_max_step_mclk` hard-codes the opcodes `0x54` and `0xD4` for CSL and CSH.
- `cpu.c` spells the ST0 to ST2 targets as `0x1FE000u`, `0x1FE002u` and `0x1FE003u`, while `bus.c` owns `HW_PAGE_PHYS`.
- The B-bit mask `0x10u` appears five times in `cpu.c` ("B has no mask here") and again as `SNAP_P_B_BIT`.

`scripts/opcode-check` guards the table's bytes and cycles but not these copies. A future opcode renumbering or a sixth block instruction would make restore refuse genuine snapshots, or make the step guard miss an extra cost, with nothing failing in the build.
**Fix:** Put `DS_BLK_OPCODE_*`, `DS_HW_PAGE_PHYS` and `DS_P_B_BIT` in `ds_core.h` and use them in all files. Better still, have `ds_cpu_op_max_cycles` fold the CSL/CSH extra in, so `run.c` does not need the opcode numbers.

### IN-02: Documentation that no longer matches the code

**File:** `src/text/ds_text.h:15-18`, `src/text/text.c:516`, `src/core/snapshot_refusals.def:10`
**Issue:**
- The `ds_text.h` header's list of record tables omits `stimulus` and `input`. `ds_text_record` prints `... model_*, stimulus, input, constants, run, limits, machine`, and the format-version note a few lines further down already counts those tables.
- The comment at `text.c:516` says "the [withheld] list is full at 24 of 24". `WH_MAX` is 64 and the table prints one row per id since format version 6.
- `snapshot_refusals.def` row `argument` says it is returned when "(ds_snapshot) the written pointer's target is NULL". `ds_snapshot` accepts `written == NULL` (code at `snapshot.c:111` and `ds_snapshot.h:33` say "when not NULL").

**Fix:** Update the three texts. The `.def` row should drop the `written` clause.

### IN-03: `ds_run` does not refuse a request or result that overlaps the instance, unlike the sinks and `ds_boot`

**File:** `src/core/run.c:83-98`, `src/core/obs.c:45-49`
**Issue:** `touches_call` refuses a sink whose memory overlaps the instance, the request or the result. `ds_boot` likewise refuses an instance block that overlaps the request, the content or the result (WR-02 of that file). `ds_run` never checks the request or the result against the instance. A caller that places a `struct ds_run_result` inside the instance, or a request there, gets silent corruption. `finish` writes `result->reason` and `result->refusal` and then reads `inst->blk_active`. The loop re-reads `request->until_mclk` and `request->stop` every iteration while the instance changes. This is a caller bug, not a core memory-safety hole, but it is the same class of fault the other entry points turn into `memory_invalid`, so the contract is uneven.
**Fix:** In `ds_run`, after the struct-size and reserved checks, add `ranges_overlap` checks of the request and the result against the instance and return `DS_RUNREF_memory_invalid`. Alternatively state in `ds_core.h` that this overlap is a caller contract violation. Either is acceptable once it is explicit.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
