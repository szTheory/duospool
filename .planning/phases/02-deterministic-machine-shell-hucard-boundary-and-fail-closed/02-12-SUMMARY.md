---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 12
subsystem: core
tags: [power-on, splitmix64, seeded-prng-v1, placeholder-stepping, xorshift32, machine-time-budget, refusals, ledger]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: the boot-order table and the refusal enum (02-05), the toy-control and anchor mechanism (02-09), the refusal matrix, precedence table and coverage kind (02-11)
provides:
  - poweron.c: splitmix64, domain constants, per-domain sub-seed and streams, the zero and seeded-prng-v1 policies; ds_poweron.h (internal test access)
  - advance.c: ds_advance (placeholder rules, DS_ADVANCE_MAX = 1048576 per call) and ds_run (machine-time budget, DS_RUN_STOPPED / DS_RUN_BUDGET / DS_RUN_REFUSED)
  - refusals 21 powerup_seed_required and 22 powerup_seed_unexpected; the powerup row of boot_order.def directly after bindings
  - request fields powerup_seed and has_seed; instance fields ram8k, mpr, psg_noise, vdc_phase, powerup_policy, has_seed, powerup_seed
  - runner request key seed=N; kinds splitmix, poweron, poweron_domain (powkinds.c), advance, run_budget (stepkinds.c); 10 new cases (83 in total)
  - LED-PROF-0002 (assumed, pinned to TEST:poweron_draw_order) listed in ledger/REQUIRED
affects: [02-13 (the state digest must cover the new instance fields), 02-14 (record labels time placeholder-cycles), 02-15, 02-16 (shared-state rule anchors on the one psg_noise update line of advance.c)]

actuals:
  tokens: 16496    # chars/4 over the realized diff (git diff plan_head_before..HEAD excluding .planning, 65983 chars, 26 files, 1119 added and 11 removed lines)
  tasks: 2
  commits: 2       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 7101b3238fe998b919bd0afb3a1ee8c477de29d4
plan_head_after: b6630127f7e968b90223daa79033d4fb60f28cbf
commits: 2

tech-stack:
  added: []
  patterns:
    - "a stream per domain: sub = one splitmix64 step from (seed XOR fixed ASCII-packed constant); the stream is a splitmix64 generator started at sub, draws written little-endian"
    - "intended wraparound is unsigned and written with explicit casts; the sanitize preset (no -fwrapv) ran clean over the new code"
    - "expected values are literals from a throwaway perl script (Math::BigInt, outside the repo), never from the code under test; a literal that disagreed with the code was re-derived, not copied from the code"
    - "a kind family is its own file: powkinds.c (power-on) and stepkinds.c (stepping), sharing two small list helpers declared in runner.h"

key-files:
  created:
    - src/core/ds_poweron.h
    - src/core/poweron.c
    - src/core/advance.c
    - tests/conformance/powkinds.c
    - tests/conformance/stepkinds.c
    - ledger/entries/LED-PROF-0002.toml
    - tests/conformance/cases/ (10 files: splitmix_seed0, poweron_zero, poweron_draw_order, poweron_seed_zero_supplied, ref_seed_required, ref_seed_forbidden, advance_rules, run_budget, run_budget_stop, poweron_domain_independence)
  modified:
    - src/core/ds_core.h
    - src/core/boot.c
    - src/core/boot_order.def
    - src/core/refusals.def
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - tests/conformance/cases/ref_precedence_table.toml
    - ledger/REQUIRED
    - CMakeLists.txt

key-decisions:
  - "the seed is a u64 plus a has_seed flag: seed 0 is a valid seed and is not the absence of one (poweron_seed_zero_supplied pins it); any nonzero has_seed reads as true"
  - "both seed refusals name the field powerup_policy, like the other bound-field refusals"
  - "ds_advance and ds_run return their own small status enums (not refusal enum members), so the refusal enum stays exactly the 22 boot-time reasons that ref_reason_coverage counts"
  - "ds_run checks the stop function before the first step and after every step, so a stop that holds exactly when the budget is consumed is a stop and machine time never passes start plus budget"
  - "the psg_noise update is the single source line inst->psg_noise = noise_step(inst->psg_noise); in ds_advance, the anchor for plan 16's shared-state rule"

patterns-established:
  - "a new refusal is two lines of refusals.def, one row of boot_order.def, one check function and one refuse row per reason; coverage, allocation, non-mutation and sanitize matrices widen by themselves"

requirements-completed: [CORE-03]

coverage:
  - id: D1
    description: "A seeded boot fills RAM, psg_noise and vdc_phase from independent per-domain streams (sub = splitmix64(seed XOR DOMAIN_CONST)); the zero policy gives zero RAM, psg_noise 1, vdc_phase 0; both match perl-derived literals"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "sh scripts/check --stage conformance PASS; RESULT splitmix_seed0, poweron_zero, poweron_draw_order, poweron_seed_zero_supplied pass; a scratch bare-XOR derivation turned poweron_draw_order and poweron_seed_zero_supplied guest_failure (work RAM differs from the literal)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The seed refusals are stable enum members 21 and 22, checked after bindings and before mapper, with refuse rows, a precedence pair on each side and coverage over 22 reasons"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "RESULT ref_seed_required, ref_seed_forbidden pass; ref_precedence_table pass assertions=86 (16 items, 16 rows); ref_reason_coverage pass assertions=23 (22 members); awk over refusals.def prints 22 rows ending 21 powerup_seed_required, 22 powerup_seed_unexpected"
        status: pass
    human_judgment: false
  - id: D3
    description: "Stepping is non-vacuous and specified by literals: time, xorshift32 psg_noise and wrapping vdc_phase evolve by the documented rules, a call above DS_ADVANCE_MAX is refused with no change, the result does not depend on how cycles are split, and ds_run reports budget exhaustion without overshoot"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "RESULT advance_rules (22 assertions), run_budget (19), run_budget_stop (16) pass; a scratch edit of the xorshift shift turned advance_rules guest_failure; a scratch chunk edit turned run_budget guest_failure; the red stub run is recorded below"
        status: pass
    human_judgment: false
  - id: D4
    description: "Domain streams are independent: extra draws in psg_noise and vdc_phase leave the ram8k bytes unchanged, the three domains differ, the stream does not start at the bare XOR, and a boot lays down the same streams"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "RESULT poweron_domain_independence pass assertions=10"
        status: pass
    human_judgment: false
  - id: D5
    description: "LED-PROF-0002 records the power-on policy as an assumption and withholds every hardware claim for placeholder evolution and the MPR constant"
    requirement: CORE-03
    verification:
      - kind: integration
        ref: "sh scripts/check --stage ledger-entries PASS; grep -cxF 'LED-PROF-0002 02-CONTEXT D-50' ledger/REQUIRED prints 1; 13 schema keys each match one line; grep -c LFSR prints 1"
        status: pass
    human_judgment: false

duration: 25 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 12: Seeded power-on state and placeholder stepping Summary

**Power-on is now one of two documented policies (zero, or seeded-prng-v1 with a splitmix64 stream per domain from sub = splitmix64(seed XOR DOMAIN_CONST)), and `ds_advance` / `ds_run` give the machine evolving seeded state and a machine-time budget, with every expected value a perl-derived literal and every placeholder rule labelled as no hardware claim.**

## Performance

- **Duration:** about 25 min (2026-11-10, ending 06:22Z)
- **Tasks:** 2 of 2 (Task 1 tracer, Task 2 auto with tdd)
- **Files:** 26 changed outside .planning (1119 added lines, 11 removed)
- **Gate cost (this machine, warm):** `sh scripts/check` 45.3 s wall, over the 30.7 s of plan 11 because a build was cold after the header change; no stage is over 20 s. `sh scripts/check --selftest` 10.2 s.

## Accomplishments

- **Tracer (Task 1).** `poweron.c` implements splitmix64 with unsigned arithmetic only, the three fixed ASCII-packed domain constants (`RAM8K___`, `PSGNOISE`, `VDCPHASE`), the derivation `sub = one splitmix64 step from (seed XOR constant)`, and little-endian draws; `ram8k` draws 8192 bytes (1024 draws), `psg_noise` takes the low 32 bits of its first draw (forced to 1 if zero) and `vdc_phase` the low 32 bits of its first draw. Zero policy: RAM zero, psg_noise 1, vdc_phase 0, MPR bytes all zero. `check_powerup` sits directly after `bindings` in `boot_order.def` and returns `powerup_seed_required` (21) or `powerup_seed_unexpected` (22), naming `powerup_policy`. The runner gained `seed=N` (decimal or 0x hex, `parse_u64`) and the kinds `splitmix`, `poweron` and `poweron_domain`.
- **Placeholder stepping (Task 2).** `ds_advance` adds the cycles to machine time, steps `psg_noise` once per cycle (xorshift32: 13, 17, 5) and adds the cycles to `vdc_phase`, wrapping; a call above `DS_ADVANCE_MAX` (1048576) or with a NULL instance is refused and changes nothing. `ds_run` steps in chunks of at most `step`, checks the stop function before the first step and after every step, never passes the budget, and returns `DS_RUN_BUDGET` (the outcome the runner maps to timeout later, D-57). The kinds `advance` and `run_budget` are in `stepkinds.c`.
- **Independent derivation.** All literals come from a throwaway perl script kept outside the repo (Math::BigInt with unsigned 64-bit masks, the constants and rules of the plan): the seed-0 splitmix outputs (matching the plan's `0xE220A8397B1DCDAF`, `0x6E789E6AA1B965F4`, `0x06C45D188009454F`), for seed 42 the first 16 RAM bytes `37db8e2ea18a90d91bed048d161cb5d2`, the last draw (bytes 8184..8191, `86e36fa23afcda5d`), `psg_noise 0x97809872`, `vdc_phase 0xA228BF93`, the values after 1000 and 2000 cycles, the xorshift32 sequence from 1 (`0x42021`, `0x4080601`, `0x9DCCA8C5`, which are the commonly cited 270369, 67634689, 2647435461), the `0xFFFFFFF0 + 32` wrap and one maximum call. The method is recorded in the comment of `poweron_draw_order` and `advance_rules`.
- **Ledger.** `LED-PROF-0002` (area PROF, assumed, pinned to `TEST:poweron_draw_order`, revisit phase 3, origin `02-CONTEXT D-50`) with all 13 schema keys; sources are all `UNKNOWN`, because no hardware source was read and splitmix is the project's own choice. The withheld list names the uninitialised-memory claim, the placeholder generator ("the word LFSR here means a fixed placeholder generator, not a hardware LFSR"), the MPR constant and the machine-time unit. Listed in `ledger/REQUIRED` as exactly `LED-PROF-0002 02-CONTEXT D-50`.

## Verification Results

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage conformance && sh scripts/check --stage sanitizers` (Task 1) | exit 0, `CHECK conformance PASS`, `CHECK sanitizers PASS`; `RUNNER_DONE 79`; `RESULT splitmix_seed0 pass assertions=4`, `poweron_zero pass 8`, `poweron_draw_order pass 9`, `poweron_seed_zero_supplied pass 7`, `ref_seed_required pass 5`, `ref_seed_forbidden pass 5`, `ref_reason_coverage pass assertions=23` (22 members), `ref_precedence_table pass assertions=86` |
| `sh scripts/check --stage conformance && sh scripts/check --stage ledger-entries` (Task 2) | exit 0, both PASS, `RUNNER_DONE 83`; `advance_rules pass 22`, `run_budget pass 19`, `run_budget_stop pass 16`, `poweron_domain_independence pass 10` |

Acceptance criteria, all run:

- Task 1: `awk` over `src/core/refusals.def` counts 22 rows, the last two `21 powerup_seed_required` and `22 powerup_seed_unexpected`. `sh scripts/check --stage licences` prints `CHECK licences PASS`. `sh scripts/check --selftest` prints `SELFTEST mutation-control-anchor heuristic-reverse PASS` and `wrong-mirror PASS` (no anchor lost: boot.c still has the one-line content copy) and the conformance output carries `CONTROL heuristic-reverse RED_OK` and `CONTROL wrong-mirror RED_OK`.
- Task 1, derivation liveness (observed once, reverted by copying the good file back): replacing the one splitmix step in `ds_stream_init` by `s->state = start;` (a bare seed XOR constant) gave `RESULT poweron_draw_order guest_failure` and `poweron_seed_zero_supplied guest_failure` with `the work RAM differs from the independently derived literal`, while `poweron_zero` stayed pass; stage FAIL, then restored and PASS.
- Task 2, TDD (red before green, not committed): the cases and kinds were written first against a counting-only `advance.c` stub; `advance_rules`, `run_budget` and `run_budget_stop` came out `guest_failure`, then the real `advance.c` was written. Stepping liveness (each observed once and reverted): a shift of 16 instead of 17 in the xorshift turned `advance_rules` red; a chunk rule that overshoots the budget turned `run_budget` red (`machine time after ds_run differs from the expected value`).
- Task 2: `grep -cxF 'LED-PROF-0002 02-CONTEXT D-50' ledger/REQUIRED` prints 1 (baseline 0); each of the 13 schema keys has exactly one `KEY = ` line; `grep -c LFSR ledger/entries/LED-PROF-0002.toml` prints 1; `head -5` of `advance.c` shows the SPDX MIT OR Apache-2.0 line and the ledger entry starts with the 0BSD comment header.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, ends `CHECK RESULT PASS` (45.3 s wall, cold build) |
| `dash scripts/check` | `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | 16 `SELFTEST` lines including `mutation-drop-precedence` and both `mutation-control-anchor` rules, then `SELFTEST RESULT PASS` |
| `sh scripts/audit-history --range 7101b32..HEAD` | `audit-history: clean` (2 commits, 30 blobs) |

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1 to 3 fix was needed in committed code.

### Interpretation choices (inside the plan's intent)

1. **Files beyond `files_modified`.** `CMakeLists.txt` (two kind files joined the runner source list), `src/core/ds_poweron.h` (named in the plan's artifacts, absent from its files list), `tests/conformance/runner.h` and the two kind files `powkinds.c` and `stepkinds.c`. `runner.c` was not grown beyond the request key, `parse_u64` and five table rows.
2. **Two extra cases.** `poweron_seed_zero_supplied` (an explicit seed 0 is a seed: the plan's eight cases would not have distinguished `has_seed` from a zero seed) and `run_budget_stop` (the stop-function behaviours did not fit in the 400-character expect line of `run_budget`). 10 new cases in total, 83 in the directory and in REQUIRED.
3. **The precedence table got two pairs, not one**: bindings before power-on (`bind.lines_per_frame=-` with a seeded policy and no seed gives `open_unbound`), and power-on before mapper for both seed refusals (a seeded policy with no seed, and a seed under zero, each with an undeclared mapper). The table now has 16 items for 16 rows.
4. **A literal of my own was wrong, the code was right.** The first `advance_rules` literal for the final `psg_noise` (`0xE6BE574A`) forgot that the register also steps during the preceding 32-cycle call; perl over 32 + 1048576 steps gave `0x9FC86668`, which the code already produced. Corrected in the case before anything else changed, and the comment states the step count.
5. **`ds_advance` and `ds_run` have their own status enums** (`DS_ADVANCE_REFUSED`, `DS_RUN_REFUSED`), not new refusal numbers, so that `ref_reason_coverage` counts exactly the 22 boot-time reasons the plan asked for.
6. **The seed refusals name `powerup_policy`** in the result field index; the new rows expect `field=powerup_policy`.
7. **`ds_poweron_apply` takes only the instance** (policy, has_seed and seed are recorded in it first), so the recorded policy is the one that drove the draws.
8. **Task 2 is not red-committed**: the red state (stub) was observed but never committed, as in plan 11, because a red commit would put a failing gate in history. The `poweron_domain` kind landed in the Task 1 commit because it only needs `poweron.c`; its case belongs to Task 2 as the plan says.
9. **One scratch mutation hung.** A mutation of the run loop from `<` to `<=` made `ds_run` spin forever on a zero-length chunk, so the stage hit its time limit and the runner was killed; it was never committed (the committed loop cannot reach that state because the chunk is bounded by the budget left). The overshoot mutation above was used instead.
10. **CORE-03 is not yet checked off in REQUIREMENTS.md.** `requirements.ready-ids` reported it blocked (a later plan of this phase also declares it and has no SUMMARY yet, the shared-ID gate); it is marked when the last declaring plan finishes.
11. **`ledger/REQUIRED` line placed in sorted position** (after LED-PROF-0001) rather than at the end of the file; the checker accepts either and the line is exactly the required text.

**Total deviations:** 0 auto-fixed, 11 interpretation choices. **Impact:** none on later plans except the notes below.

## Notes for later plans

- **Plan 13 (digest).** The digest must cover the new instance state: `ram8k`, `mpr`, `psg_noise`, `vdc_phase`, `powerup_policy`, `has_seed`, `powerup_seed` (and machine time as before). The instance size changed (the ROM array is still the last member); every `ds_instance_size()` consumer reads it at run time.
- **Plan 14 (record).** Machine time is placeholder cycles: label it `placeholder-cycles`, never a hardware unit (T-02-27).
- **Plan 16 (shared-state rule).** The anchor is the one line `inst->psg_noise = noise_step(inst->psg_noise);` in `ds_advance` (`src/core/advance.c`). The xorshift rule itself is the static `noise_step` above it.
- **Request grammar.** `seed=N` (decimal or 0x hex) sets `has_seed`; the policy comes only from `bind.powerup_policy`. A boot with the seeded policy and no seed, or a seed under zero, is refused (rows `ref_seed_required`, `ref_seed_forbidden`).
- **Every runner invocation still needs `--marker-seed N`** (plan 11).

## Task Commits

1. **Task 1 (tracer): seeded power-on state, seed refusals, splitmix and draw-order literals** - `035a93c` (feat)
2. **Task 2: placeholder stepping, budgeted run, domain independence, LED-PROF-0002** - `b663012` (feat)

## Known Stubs

None. `advance.c` is a deliberate placeholder (documented in the file, the ledger entry and this summary: Phase 3 replaces it) and is not a stub that blocks the plan's goal; no empty value flows to a consumer. The `fixture = "none"` of `splitmix_seed0` and `poweron_domain_independence` is real (the kinds boot their own fixture where they need one).

## Threat Flags

None. No network endpoint, file access, auth path or schema change at a trust boundary was added. T-02-26 is mitigated by `ref_seed_required`, `ref_seed_forbidden` and the two precedence pairs; T-02-27 by the `LED-PROF-0002` withheld list (placeholder generator, MPR constant, machine-time unit). T-02-SC: no package-manager install was run.

## Self-Check: PASSED

- Files exist (checked with `[ -f ]`): `src/core/ds_poweron.h`, `src/core/poweron.c`, `src/core/advance.c`, `tests/conformance/powkinds.c`, `tests/conformance/stepkinds.c`, `ledger/entries/LED-PROF-0002.toml`, and the 10 new case files (83 in the directory, 83 ids in `REQUIRED`).
- Commits `035a93c` and `b663012` are ancestors of HEAD; `git rev-list --count` from `plan_head_before` is 2.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above.
