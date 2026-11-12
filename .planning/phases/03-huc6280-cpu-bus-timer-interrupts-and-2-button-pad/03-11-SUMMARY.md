---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 11
subsystem: cpu-core-and-fixtures
tags: [loads, stores, transfers, stack, addressing-modes, cpu-ldst, policy-list, mutation-sweep, w2]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the opcode table and mnemonic switch (03-09), the fixture library with CHECK_LIT and CHECK_GUEST (03-05), the mutate_sweep kind and observe rows (03-10), the guest kind and cites registry (03-04, 03-37)
provides:
  - cpu.c effective addresses for the nine memory modes (zp, zp,X, zp,Y, (zp), (zp,X), (zp),Y, abs, abs,X, abs,Y) and the semantics of LDA LDX LDY STA STX STY STZ, TAX TAY TXA TYA TSX TXS, CLA CLX CLY, PHA PHX PHY PLA PLX PLY (PHP and PLP existed)
  - fixtures/src/cpu_ldst.s, cpu_ldst.checks and the manifest entry cpu_ldst: 82 checks in ladder order, REQUIRED case cpu_ldst
  - 22 more image-read instruction-page cites in fixtures/lib/cites.txt (S8-34 39 40 53 54 55 59 61 62 63 64 65 66 83 84 85 86 91 92 105 106 107 108)
  - kind_guest runs each ROM once per power-on policy (built-in default zero and seeded-prng-v1 seed 42, or the optional policies key) and compares the DSV1 result and observed bytes across runs
  - REQUIRED case mutate_sweep_cpu_ldst (82 literals) and selftest mutation-policies-uninit
affects: [03-12, 03-13, 03-14, 03-24, 03-32]

actuals:
  tokens: 17700
  tasks: 3
  commits: 3
plan_head_before: a7eaa63447444612a4401841b05b639cc0b500d6
plan_head_after: a1a194546c7a94daacdd45e667be1076a22ff2e6

tech-stack:
  added: []
  patterns:
    - "one eff_addr(inst, mode, op1, abs_addr) in cpu.c owns every address computation of the memory modes; a zero-page address is $2000 | zz and an indexed one $2000 | ((zz + index) & $FF); pointer bytes are read only by a mode that executes"
    - "a flag probe is CLC, PHP, PLA and a compare with N, Z plus $14 (I from the reset code, B reads 1 through PHP); a register is read back through the opposite transfer, a memory cell through an absolute load"
    - "the fixture fills the pages it reads with $FF, then writes every cell a check reads; the caller's S is kept in SSAVE and any check that moves S puts it back from there"
    - "kind_guest splits one boot-run-judge into guest_once and loops over the policy fragments appended to the case request with a semicolon; later runs judge into scratch contexts and add a failure only on a difference, so a passing case keeps assertions = checks"

key-files:
  created:
    - fixtures/src/cpu_ldst.s
    - fixtures/src/cpu_ldst.checks
    - fixtures/manifest/cpu_ldst.toml
    - tests/conformance/cases/cpu_ldst.toml
    - tests/conformance/cases/mutate_sweep_cpu_ldst.toml
  modified:
    - src/core/cpu.c
    - fixtures/lib/cites.txt
    - tests/conformance/guestkinds.c
    - tests/conformance/REQUIRED
    - scripts/check

key-decisions:
  - "Flag expectations of loads, transfers and pulls are N and Z from the flag row of the instruction page; the $14 base of every P byte is H8-18 (I set) plus H8-5 (B reads 1, T reads 0). Each row is tagged with the instruction page, the H8-5 part is stated in its meaning column"
  - "Zero page and indexed zero page: MAN:S8-11 (high byte always $20). Pointer forms at zz = $FF and (zz,X) pointer wrap: ASSUMED:LED-CPU-0010 (D-99 assumption, the manual does not state them). Stack: MAN:H8-3, wrap of S is the plain 8-bit register"
  - "absolute,X and absolute,Y wrap at $FFFF are GUEST checks: MPR0 is set to bank 0, the model is the byte that bank 0 shows at $E001 and $E002 through MPR7"
  - "The default policy list lives in guestkinds.c (default_policies[]); the case key policies exists already in the schema and is honoured (fragments separated by ||); plan 03-24 changes the array, no case file repeats the list"
  - "The poison-t0 control uses the first policy only (it never runs the guest); the mutate_sweep kind keeps the default request (the policy comparison is the guest kind's job)"

requirements-advanced: [CPU-01, FIX-03]
requirements-completed: []  # CPU-01 needs the W2 fixtures of 03-12 to 03-14 and the oracle cross-check; FIX-03 needs every fixture swept and the later lint rules

duration: 85min
completed: 2026-11-11
---

# Phase 3 Plan 11: Loads, stores, transfers, stack and the power-on policy list Summary

**cpu.c now executes the data-movement group in all nine memory modes with the zero page and stack wrapping inside $20xx and $21xx, the authored fixture cpu_ldst (82 checks, instruction-page and guest-model literals) passes under zero and seeded-prng-v1 seed 42 with byte-identical result and observed bytes, and every literal of it flips exactly its own check in the sweep.**

## What was built

- **Task 1 (tracer), commit 12a46fa.** `eff_addr`, `zp_addr`, `zp_pointer`, `operand_value` and `store_operand` in cpu.c replace the old absolute-only helpers (the precomputed `ea` is gone; the pointer bytes of an indirect mode are read only when the mnemonic executes). The same commit holds the handlers of the whole group, because the mode helper is shared and the fixture's data setup needs STA (see deviation 1). cpu_ldst started with 14 checks (LDA in all nine modes, N, Z and the positive-byte case), a manifest entry with a pinned hash, the cite S8-53 and the REQUIRED case. Tracer verify (`fixture-lint`, `fixtures`, `conformance`) passed before the commit, so expansion went ahead; no `blocking-human` gate.
- **Task 2, commit 5306b1c.** cpu_ldst grows to 82 checks in ladder order: LDA (14), transfers and clears (12), LDX (6), LDY (6), STA (9), STX/STY/STZ (13), stack (10), wrap probes (12). Rows: the loaded or stored byte through a different access path, N and Z rows (a negative, a zero and a positive byte), "no flags" rows for TXS, CLA, STA, STX and STZ (the flag state is set to something that a buggy flag update would change), PHP pushed byte read from the stack page, PLP restoring N V I C and Z. The expected values of the 22 new MAN rows cite instruction pages that this run read as images (the 22 new cites.txt rows, flag `img`, verifier gsd-executor). The source and the `.checks` file are produced from one table by a scratch script; the committed text is the source of record.
- **Task 3, commit a1a1945.** `kind_guest` is split into `guest_once` (one boot, run and judge) and a loop over the policy list. Run 0 is judged exactly as before; each further run is judged into scratch contexts, its outcome must equal run 0's and its result bytes (+10..+6F) and observed bytes (+80..+DF) must equal run 0's, otherwise the case is guest_failure with a message naming the policy and the first differing check. REQUIRED case `mutate_sweep_cpu_ldst`; selftest `mutation-policies-uninit` (scratch ds_proto whose check 2 stores the power-on byte at $2500 as both observed and expected byte: it passes under every policy, so only the comparison catches it).

## Verification

- Task verifies: Task 1 `--stage fixture-lint`, `fixtures`, `conformance`; Task 2 plus `sanitizers`; Task 3 `--stage conformance`, `inventory`, `--selftest mutation-policies-uninit`: all PASS.
- Plan end: `sh scripts/check` PASS (about 71 s wall), `sh scripts/check --selftest` PASS (57 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range a7eaa63..HEAD` clean (3 commits, 16 blobs, 1215 blob-path pairs). The shared-state toy control stayed RED_OK; the standing note was not triggered and no interleave case was touched.
- Bite shown (scratch edits of cpu.c, restored byte for byte, nothing committed): (zp),Y ignoring Y fails check 9; zp,X without the in-page wrap fails check 70; TXS setting N and Z fails check 22; the pointer high byte read without the wrap fails check 75; STZ setting flags fails check 59. The mutation sweep: 82 of 82 literals flip exactly their own check.
- During development the first full run hung in garbage: my S restore after the TXS rows put S at $FF instead of the caller's $FD, so the next JSR overwrote fixture_main's return address. Fixed by SSAVE (kept in the generator and the source); the stack probe also saves and restores $21FF, which it overwrites by design.

## Deviations from Plan

**1. [Rule 3 - structure] Task 1 commit holds the whole group in cpu.c.** The plan lists LDA only for Task 1 and the rest for Task 2. The addressing helper is shared by every mnemonic and the fixture's data setup needs STA from its first line, so the stores, transfers, clears and stack handlers were written together with the helper and landed in the Task 1 commit. Task 2 changed no core file; what it added is fixture, checks, manifest and cites.
**2. [Rule 2 - behaviour widened by the shared helper] ADC, CMP, EOR and INC now also execute in the zero-page and indirect modes.** They used `operand_value`/`has_ea`, which now cover all nine modes. This is the correct semantics (their group is plan 03-12, which still owns T mode, decimal mode and the gap mnemonics) and it moves no existing fixture or hash (ds_proto and its variants are unchanged: the five old image hashes equal those before the plan).
**3. [Plan text "about 60 checks"] 82 checks.** Each row is one mode or one flag effect that an S8 page lists for the 29 mnemonics of the group; the ladder, the "no flags" rows and the wrap probes account for the difference. Code size is 2.4 KiB of the 4 KiB code window, so no check was dropped for space. The sweep covers all 82.
**4. [Rule 2 - CLA, CLX, CLY] Not in the plan's behaviour list but in the gap comment of 03-09 under plan 03-11** ("CLA CLX CLY are register clears"); implemented and checked (4 rows).
**5. [Files outside the plan list] `scripts/check` only for the selftest (in the plan list); `tests/conformance/cases/cpu_ldst.toml` and `fixtures/lib/cites.txt` are in the plan list.** No other outside file was touched.
**6. Reading of the policies key.** The schema already carries `policies` (type string, optional). I defined its value as request fragments separated by `||` (an empty fragment is the zero policy). No case uses it yet. The plan says "array of request strings"; the TSV flattener has no array type for optional keys, so a string is the form that exists.

No Rule 1 bug and no Rule 4 deviation.

## Evidence discipline

E0 only. The 82 checks are in-guest self-consistency and manual-page literals: MAN rows cite instruction pages S8-34 to S8-108 and S8-11, H8-3, H8-5 (the 22 new cites are image-read by this run, a person-or-agent read per ER-1, recorded as `img`), GUEST rows (abs,X and abs,Y wrap) compute the expectation in the guest, and four rows are ASSUMED:LED-CPU-0010 (the (zp,X) pointer wrap, the (zp) and (zp),Y pointer at zz = $FF) because the manual does not state them. LED-CPU-0010 does not exist as a ledger file yet (the ledger plans create it); no ledger status changed and none of the eight ER-2 candidates was touched. The policy comparison shows independence from the power-on state of RAM under two policies (CPU registers are zero under both in this core, and MPR0 to MPR6 are set by the fixture); it says nothing about hardware. A pass is not a support claim. No expected value comes from DuoSpool output, no manual text or image is committed (the PDFs and renders stay in the scratch directory), no accuracy, compatibility, cycle-accurate, hardware-verified or boot claim, nothing pushed (D-64).

## Known Stubs

None added. The mnemonics of plans 03-12, 03-13 and 03-14 and the block transfers are still the documented gap in `ds_cpu_step_obs` (advance by table length, charge table cycles). `observe=IDX:differs` is still refused until plan 03-24.

## Threat Flags

None beyond the plan's register. T-03-32 (address arithmetic): every address goes through `eff_addr` with explicit masks (8-bit zero-page wrap, 16-bit wrap) and the bus translate; the wrap probes and the sanitizers stage pass on cpu_ldst. T-03-33 (power-on dependence): the policy comparison runs on every guest case and is shown to catch a fixture that reads uninitialised RAM (selftest).

## Notes for next plans

- **03-12 to 03-14:** use `operand_value`/`eff_addr`/`store_operand` for the memory modes; AND ORA EOR ADC SBC CMP CPX CPY BIT TSB TRB DEC INC ASL LSR ROL ROR and the accumulator forms are still to do where noted in the gap comments. The cpu_ldst fixture assumes only the instructions implemented by 03-11 plus ADC/CMP/EOR in the check macros (ck_tally uses `adc #$FF`).
- **Fixtures after this one:** the page-fill pattern (fill the pages a fixture reads with $FF, then write each cell) is how cpu_ldst passes the policy list; the first `.checks` row or the fixture header should say so. A fixture that must keep the caller's return address safe keeps S in a RAM cell (SSAVE = $2403) and restores it from there, not with `ldx #$FF`. P through PHP, PLA is N and Z plus $14 only while C is cleared and V is clear (V is cleared by ck_tally's `adc` after every check; there is no CLV yet, plan 03-13).
- **Policy list (03-24):** change `default_policies[]` and `GP_DEFAULT_COUNT` in tests/conformance/guestkinds.c to zero plus two seeded-prng-v2 seeds; the message names the policy by its request fragment ("zero" for the empty one). Observe rows that legitimately differ by policy (`observe=IDX:differs`) are not honoured yet: a later run's observe_judge uses the same items as run 0.
- **Cites:** 03-32's re-read (R-8) should cover the 22 new rows (flag `img` was set by this run from page images; no OCR text used for any value). S8-60 (PHP) was not read; PHP is cited from H8-5.
- **mutate_sweep_cpu_ldst** takes ~0.03 s; cpu_ldst under two policies ~0.01 s each (the first full check run took about 71 s wall, in line with plan 03-10). `opcode_exercise` (mode report) now sees cpu_ldst: the evidence file lists the unexercised defined opcodes.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: fixtures/src/cpu_ldst.s, fixtures/src/cpu_ldst.checks, fixtures/manifest/cpu_ldst.toml, tests/conformance/cases/cpu_ldst.toml, tests/conformance/cases/mutate_sweep_cpu_ldst.toml. Commits 12a46fa, 5306b1c and a1a1945 are ancestors of HEAD.
