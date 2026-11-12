---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 01
subsystem: core
tags: [huc6280, ds_run, master-clocks, ca65, dsv1, fixture-build, tracer]
status: complete

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: ds_boot, ds_mapper_read, the conformance runner and gate, the fail-closed stage pattern
provides:
  - ds_run(inst, request, result) with an absolute until_mclk deadline, a stop callback and typed refusals
  - a minimal HuC6280 core (19 opcodes, reset through the vector, one bus choke point) stepping in master clocks
  - scripts/fixture-build and the fixture-build gate stage (ca65 + ld65, PREFLIGHT_MISSING exit 2, host-hashed used list)
  - the guest case kind (DSV1 judged from raw inst->ram8k) and the first authored fixture ds_proto
  - selftest mutation-guest-wrong-literal
affects: [03-02, 03-03, 03-04, 03-05, 03-06, 03-09, 03-16, 03-35]

actuals:
  tokens: 15346
  tasks: 3
  commits: 2
plan_head_before: 6fbc336f9c770d100ff2ddb4b01d5f869d318458
plan_head_after: 20d575b00fe39aa64258920d804e54ff551b2e76

tech-stack:
  added: [cc65 2.19 (ca65, ld65) as a local Homebrew tool, not a repository dependency]
  patterns:
    - "authored fixture built in the gate into ignored build/, never committed (D-123, D-133)"
    - "guest verdict protocol DSV1 judged from raw work RAM by the runner (D-124)"
    - "absolute-deadline run API: steps are indivisible, no state outside the instance"

key-files:
  created:
    - src/core/run.c
    - src/core/cpu.c
    - fixtures/src/ds_proto.s
    - fixtures/lib/hucard.cfg
    - scripts/fixture-build
    - tests/conformance/cases/ds_proto.toml
  modified:
    - src/core/ds_core.h
    - src/core/advance.c
    - src/core/boot.c
    - tests/conformance/spinkinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.h
    - tests/conformance/runner.c
    - scripts/check
    - tests/conformance/REQUIRED

key-decisions:
  - "DSV1 mapping, one reading of D-124 used by every later plan: harness-side preconditions (block present before the first step, checks count or fixture id differing from the case, zero checks, ds_run refused, unreadable ROM) give harness_failure; guest-written faults give guest_failure; UNTIL without done gives timeout, but a guest fault visible in a not-done block wins over the timeout"
  - "Reset is called from ds_boot (one line in boot.c) so a booted instance always holds a valid reset state, instead of a lazy first-run flag that a later snapshot would have to carry"
  - "ds_cpu_step returns the master clocks it took and ds_run adds them in one statement, which keeps one distinct machine-time store in run.c for the re-pointed shared-state toy of plan 03-02"
  - "The run-refusal and reason enums use new prefixes (DS_RUNREF_*, DS_RUN_REASON_*) because DS_RUN_REFUSED still belongs to the placeholder until plan 03-02"

requirements-advanced: [CORE-04, FIX-01, FIX-03]
requirements-completed: []  # none ticked: this tracer slice advances all three, but each needs later plans of the phase (chunk equivalence, assembler oracle, mutation sweep)

duration: 25min
completed: 2026-11-11
---

# Phase 3 Plan 01: Tracer slice Summary

**An authored ca65 fixture, built in the gate and never committed, boots on a minimal HuC6280 core, runs through the new absolute-deadline ds_run in master clocks and is judged pass from a DSV1 verdict block read out of raw work RAM; a wrong literal is mechanically shown to turn it red.**

## What was built

- **ds_run** (src/core/run.c, private header ds_core.h): request with struct_size, reserved, absolute `until_mclk`, optional stop callback and two sink pointers that must be NULL; result with reason UNTIL/CALLBACK/REFUSED, refusal code, machine_time, overshoot, stop point INSTRUCTION and steps. A
  deadline at or below the machine time returns UNTIL with zero steps. A deadline or machine time within one maximal step of 2^64 is refused (Pitfall 20). `ds_max_step_mclk()` is computed by scanning the static const opcode table (5 cycles longest, plus the 8-cycle interrupt entry, times 12 mclk is
  156) and is never a constant.
- **cpu.c**: 19 opcode rows with the manual's bytes and base cycles (taken branch +2), flags from explicit unsigned masks, TAM, every other opcode the D-96 undefined behaviour (1 byte, 2 cycles, T cleared). Reset reads $FFFE/$FFFF through the normal translate path with MPR7 = $00; I set, D and T
  clear, low speed. One cycle is 12 mclk at low speed and 3 at high speed. The bus is per-access translate `((uint32_t)mpr[a >> 13] << 13) | (a & $1FFF)`: banks below $80 go through ds_mapper_read (writes ignored), $F8-$FB index the 8 KiB RAM with `& $1FFF` (guarded by a `_Static_assert`), everything
  else reads $FF. `ds_mpr_write` is the only MPR writer.
- **Placeholder run API** renamed `ds_run_placeholder` at the prototype, definition and both call sites (expand half of the migration); every Phase 2 case is unchanged and green. Plan 03-02 deletes it.
- **fixtures/lib/hucard.cfg** (ROMBANKS via -D; bank 0 at $E000, vectors at $FFF6, CHECKLIT segment at $F000 reserved for plan 03-05, extension banks $FF) and **fixtures/src/ds_proto.s** (two checks: MAN:S8-17 ADC $7F+$01 gives $80 with V set; GUEST:sum a 4-byte table summed by two different loops that must cancel to $00; DSV1 block at RAM offset $1F00 reached through MPR2 = $F9).
- **scripts/fixture-build**: builtin-only tool check first (PREFLIGHT_MISSING ca65 / ld65, exit 2, works with an empty PATH), then ca65 --cpu huc6280 and ld65 per source into ignored build/gate, host-hashed `NAME SIZE SHA256` lines in build/gate/fixtures-authored.used (second file; the synthetic fixtures.used stays with conformance-run, F3). NAME is the stem with `_` turned into `-` (F2).
- **Gate**: `fixture-build` stage registered before `build` with the empty-PATH control; the conformance and sanitizers stages run the build first, so no stale ROM is read. The sanitizers stage runs ds_proto under ASan and UBSan.
- **guest kind** (spinkinds.c, one declaration in runner.h and one row in kinds[]): expect `rom=NAME;id=N;checks=N;until=N`; loads build/gate/fixtures/NAME.pce, boots, requires no DSV1 block before the first step, runs, judges from raw `inst->ram8k`. REQUIRED case `ds_proto` (fixture = "none", ten keys).
- **selftest mutation-guest-wrong-literal**: scratch mini-root builds ds_proto.s unmutated (must pass), then with the literal on the single `; SELFTEST-TARGET` line changed (must be guest_failure with `check 1 ... result byte $46`); marker count other than one is a harness error (exit 2).

## Task commits

1. Task 1 (tracer): `e32254d` feat(03-01): tracer slice
2. Task 2 (harden): no file change was needed. licences, inventory, sanitizers and fixture-build were already green after Task 1; the empty-PATH control observation is recorded below. No commit.
3. Task 3: `20d575b` test(03-01): selftest mutation-guest-wrong-literal

## Verification

- Task verifies as written: `sh scripts/check --stage fixture-build && --stage core-boundary && --stage conformance` pass; `--stage licences`, `inventory`, `sanitizers`, `fixture-build` pass; `sh scripts/check --selftest mutation-guest-wrong-literal` prints PASS.
- Plan end: `sh scripts/check` PASS (about 56 s warm, ds_proto adds under 1 s; fixture-build 0 s, conformance 6 s, sanitizers 6 s), `sh scripts/check --selftest` PASS (all 39 mutations), `dash scripts/check` PASS, `sh scripts/audit-history --range 6fbc336..HEAD` clean (2 commits, 16 blobs) and the full-history audit clean. No file under build/ and no *.pce is tracked.
- Observed during development (throwaway runs against the built ROM, outside the repo): deadlines of 1, 2000, 40000, 100000 and 130000 mclk give outcome `timeout` (block absent, then state running); 4000000 gives `pass`, so the deadline mechanism and the UNTIL-without-done mapping work.
- Task 2 preflight observation: a scratch copy of fixture-build with the tool-check loop deleted still exits 2 on an empty PATH (from the sha-tool check) but prints no PREFLIGHT_MISSING line, so the stage control (exit 2 AND both PREFLIGHT_MISSING lines AND no STATUS line) would turn the stage red. The permanent selftest for the preflight lands in plan 03-06.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] boot.c edited (a 15th file) to wire the reset**
- **Found during:** Task 1
- **Issue:** the plan says "cpu.c: boot reads the reset vector" but ds_boot (boot.c) is the only place that can do it without a lazy first-run flag, and boot.c is not in the plan's 14-file list.
- **Fix:** one call, `ds_cpu_reset(inst)`, after the ROM copy and before the content hash. A lazy flag would have been state that every later snapshot and restore must carry.
- **Files modified:** src/core/boot.c
- **Commit:** e32254d. The D-81 sizing note ("exactly 14 files") is exceeded by one file; no behaviour of ds_boot changes for any Phase 2 case.

**2. [Rule 2 - Missing critical] BVS ($70) added to the implemented opcode set**
- **Found during:** Task 1
- **Issue:** check 1 is specified as "gives $80 with V set"; with only the listed opcodes V cannot be tested, so the check would have asserted less than its own description.
- **Fix:** BVS (2 bytes, 2 cycles, +2 taken) joins the table and ds_proto branches on V after the CMP. It is replaced by the full cpu_ops.def in plan 03-09.
- **Files modified:** src/core/cpu.c, fixtures/src/ds_proto.s
- **Commit:** e32254d

**3. [Rule 3 - Blocking] guest kind boots the ROM itself instead of calling boot_accepted**
- **Found during:** Task 1
- **Issue:** boot_accepted resolves the fixture through the synthetic recipes and returns "unknown fixture recipe" for fixture = "none".
- **Fix:** kind_guest reads the built image, uses parse_spec with the image length, fill_request and ds_boot directly. No runner.c logic grew (one kinds[] row).
- **Commit:** e32254d

### Notes (not deviations)

- Task 2 had nothing to fix, so it produced no commit.
- Requirements CORE-04, FIX-01 and FIX-03 are listed in the plan frontmatter and are advanced, not completed: no checkbox in REQUIREMENTS.md was ticked, because chunk-equivalence of runs, the assembler encoding oracle and the mutation sweep are later plans.
- CI: `.github/workflows/gate.yml` is unchanged, so a CI run of this branch would fail at the fixture-build stage until the apt cc65 step and its workflow-check mini-roots are added by the plan that owns them. Nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. README prerequisites do not yet name cc65 either.

## Evidence discipline

E0 only: repeatability and a mechanism check. check 1 takes its literal from the software manual page S8-17 (an img-verified cite in 03-RESEARCH, not re-read here, and no ledger entry cites it yet); check 2 is an in-guest computation. Nothing here is an accuracy, compatibility, cycle-accuracy,
hardware or boot claim, and no ledger status changed. Cycle counts in the opcode rows come from the manual per the plan and are not yet checked by the oracle of plan 03-09. The image hash of ds-proto on this Homebrew build is `12601a2376c05ce7a314bb0c6a03e24e9aa582e69e015b8be87b334f11e865de` (262144
bytes); it is recorded here as an observation and is pinned only in the manifest plan (03-06), so the Writer note 5 hash-portability question stays open until then.

## Known Stubs

None. The unimplemented opcodes execute as the defined D-96 behaviour by design until plans 03-09 to 03-14, and `DS_STOP_BLOCK_BYTE` is a reserved enum value that nothing returns.

## Threat Flags

None beyond the plan's register. T-03-01 (bus translate bounds), T-03-02 (absolute deadline, wrap guard, defined undefined opcodes), T-03-03 (request checks before any step) and T-03-04 (runner recomputes the DSV1 sum from raw RAM and refuses a block present before the first step) are implemented; T-03-101 (local cc65 install) was the Homebrew core bottle with no network script.

## Notes for the next plans

- `ds_max_step_mclk()` lives in run.c and calls `ds_cpu_max_cycles()` from cpu.c; plan 03-09 replaces the table scan, plan 03-16 adds the VDC penalty adder.
- The one machine-time store is `inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step(inst);` in run.c, the natural anchor for the shared-state toy of plan 03-02.
- Plan 03-02 deletes `ds_run_placeholder`, `ds_advance`, advance.c and the `ds_run_status` enum; `ds_stop_fn` already lives in the new section of ds_core.h.
- The fixture list for plan 03-04 is `build/gate/fixtures-authored.used`; the guest kind currently hard-codes `build/gate/fixtures/` relative to the working directory.

## Self-Check: PASSED

Created files present: src/core/run.c, src/core/cpu.c, fixtures/src/ds_proto.s, fixtures/lib/hucard.cfg, scripts/fixture-build, tests/conformance/cases/ds_proto.toml. Commits e32254d and 20d575b are ancestors of HEAD.
