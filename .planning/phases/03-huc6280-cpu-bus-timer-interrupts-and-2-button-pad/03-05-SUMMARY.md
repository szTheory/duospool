---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 05
subsystem: fixtures
tags: [fixture-library, dck1-literal-table, dsv1-controls, ca65, startup, verdict, check-macros, t0-precondition]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the guest kind and DSV1 judge (03-01), the authored-ROM loader and optional case keys (03-04), fixture-build and hucard.cfg (03-01)
provides:
  - fixtures/lib/startup.inc, verdict.inc and check.inc, the shared library of every later authored fixture
  - the DCK1 literal table at logical $F000 (file offset $1000) with CHECK_LIT and CHECK_GUEST over it (GUEST checks store an XOR mask entry)
  - ds_proto rebuilt on the library with 8 checks and fixtures/src/ds_proto.checks
  - four DSV1 protocol-control images from one source (ds-proto-predone, -nodone, -badsum, -wrongid) through a variants list in scripts/fixture-build
  - kind_guest expect modes verdict=OUTCOME:REASON and poison-t0, and five REQUIRED control cases
  - 13 more opcodes in cpu.c
affects: [03-06, 03-08, 03-09, 03-10, 03-11, 03-12, 03-37]

actuals:
  tokens: 9000
  tasks: 2
  commits: 2
plan_head_before: dce82543b244236fbda187d3011ed6380adaf521
plan_head_after: 2b09d68d8aea65f77128294ad044508dbbd63682

tech-stack:
  added: []
  patterns:
    - "a fixture defines DSV_ID and DSV_CHECKS, includes startup.inc, verdict.inc, check.inc, calls CHECK_TABLE n and LIT k, v, and provides fixture_main ending in RTS; startup.inc owns reset, MPR1 = $F8, MPR2 = $F9, the RAM vector slots at $2200 and the vector table"
    - "protocol faults are ca65 -D switches read only in verdict.inc, built as extra images by a variants list in fixture-build"
    - "a control case passes when the judge gives the declared outcome and its reason contains the declared word (expect = id=N;verdict=OUTCOME:REASON)"

key-files:
  created:
    - fixtures/lib/startup.inc
    - fixtures/lib/verdict.inc
    - fixtures/lib/check.inc
    - fixtures/src/ds_proto.checks
    - tests/conformance/cases/ds_proto_predone.toml
    - tests/conformance/cases/ds_proto_nodone.toml
    - tests/conformance/cases/ds_proto_badsum.toml
    - tests/conformance/cases/ds_proto_wrongid.toml
    - tests/conformance/cases/ds_proto_t0_poisoned.toml
  modified:
    - src/core/cpu.c
    - fixtures/src/ds_proto.s
    - fixtures/lib/hucard.cfg
    - scripts/fixture-build
    - scripts/check
    - tests/conformance/guestkinds.c
    - tests/conformance/cases/ds_proto.toml
    - tests/conformance/cases/interleave_aba_order.toml
    - tests/conformance/REQUIRED

key-decisions:
  - "Table layout: DCK1, count, then entries {check number k (1-based), value}; entry k is at $F000+5+2*(k-1). LIT defines the symbol cklit_K and asserts its table offset, so a LIT out of order fails the build. CHECK_* read cklit_K+1, never a formula"
  - "GUEST-class checks (1, 2, 8) keep the guest-computed expected byte in GEXP ($2402) and a table mask of $00; MAN and ASSUMED checks (3 to 7) keep the expected byte itself. Every check is therefore patchable, and an in-memory patch of the value byte of each of the 8 entries flipped exactly its own check to $46 in a scratch probe (the real sweep is a later plan)"
  - "The first 13 instructions of the reset code are the ones plan 03-02's run_until_abs takes its literal instruction ends from, so the verdict zeroing loop sits directly after the two TAMs in startup.inc and dsv_init no longer zeroes"
  - "The four protocol faults are switches inside verdict.inc, not in ds_proto.s: predone seals the block at the end of dsv_init, nodone skips the done state, badsum increments the checksum low byte, wrongid writes id 2. predone and badsum both give guest_failure for the checksum (the judge cannot tell the order of writes, only the stale sum)"

requirements-advanced: [FIX-01, FIX-03]
requirements-completed: []  # FIX-01 needs the assembler oracle (03-08 and later), FIX-03 needs the mutation sweep (later plan); neither is complete

duration: 60min
completed: 2026-11-11
---

# Phase 3 Plan 05: Fixture library, DCK1 literal table and DSV1 protocol controls Summary

**ds_proto now runs on project-owned startup, verdict and check libraries with every expected value in a fixed literal table at $F000, and the judge is shown to catch each DSV1 protocol fault (pre-set done, never done, bad checksum, wrong id) and a block planted before the first step.**

## What was built

- **Task 1 (tracer), commit a0d103a.**
  - `startup.inc`: SEI, CLD, stack, MPR1 = $F8, MPR2 = $F9, verdict zeroing, then the eight bytes of four RAM vector slots ($2200 to $2207) filled with the error-flag handler. The ROM vectors (IRQ2/BRK, IRQ1, TIMER, NMI) are `jmp (slot)` stubs, so later fixtures store their own handler address and never edit this file. The reset code calls `dsv_init`, `fixture_main` and `dsv_finish`.
  - `verdict.inc`: `dsv_init` (magic, version, id, counts, trailer, RUNNING with the complement first), `dsv_seal` (checksum of +06..+DF, then done, last), `dsv_finish` (seal, SEI, `bra *`).
  - `check.inc`: `CHECK_TABLE n`, `LIT k, v`, `CHECK_LIT k` (A = observed byte, the entry is the expected byte), `CHECK_GUEST k` (the expected byte is in GEXP, the entry is an XOR mask), and the `ck_pass` and `ck_fail` routines (result $50 or $46 at +10+(k-1), observed byte at +80+(k-1), pass, fail, notrun and progress tallies). Parameters are parenthesised (Pitfall 5); every operand has an explicit `a:`.
  - `ds_proto.s`, 8 checks, ladder order. Logic: 1 GUEST xor-involution, 2 GUEST xor-self-zero. Flags: 3 MAN:H8-5 P byte round trip through PHP, PLP, PHP ($16), 4 and 5 ASSUMED:LED-CPU-0010 CMP equal ($17) and less ($94). Arithmetic: 6 and 7 MAN:S8-17, $7F + $01 = $80 and the P byte $D4. Addressing: 8 GUEST abs,X and abs,Y reach the same byte across PHA and PLA. `ds_proto.checks` has the id and checks headers and one TAB-separated row per check (index, slug, class, meaning).
  - `cpu.c`: CMP abs ($CD), EOR abs ($4D), INC abs ($EE, one read, one write), LDY imm ($A0), STA abs,X ($9D), LDA abs,Y ($B9), JSR ($20), RTS ($60), PHA ($48), PLA ($68), PHP ($08, B = 1, T = 0 on the stack), PLP ($28, loads every flag but B), JMP (ind) ($6C, high byte from the next address). The stack is logical $2100 + S through the normal bus.
  - `selftest mutation-guest-wrong-literal` copies the three library files into the scratch root and mutates the one table entry on the `; SELFTEST-TARGET` line (`LIT 1, $00` to `$01`); it still requires the unmutated copy to pass and the mutated one to give guest_failure with check 1 at result byte $46.
- **Task 2 (TDD), commit 2b09d68.** `scripts/fixture-build` gets a variants list (`STEM NAME FLAGS` lines; the four ds-proto variants) and a `build_one` function; every image gets its own used-list line. `kind_guest` parses `id=N`, `id=N;verdict=OUTCOME:REASON` or `id=N;poison-t0`. In verdict mode the judge runs on a scratch context and the case passes only when the outcome name equals the declared one and the judge's reason text contains the declared word. In poison mode the runner writes the magic and the done state at $1F00 after boot and before the first step, the existing precondition must then give harness_failure ("before the first step"), and the guest is not run. REQUIRED cases: ds_proto_predone (guest_failure, checksum), ds_proto_nodone (timeout, deadline), ds_proto_badsum (guest_failure, checksum), ds_proto_wrongid (harness_failure, differs), ds_proto_t0_poisoned.

## Verification

- Task 1 verify (`--stage fixture-build`, `conformance`, `licences`, `--selftest mutation-guest-wrong-literal`): PASS. Task 2 verify (`fixture-build`, `conformance`, `named-controls`): PASS.
- Plan end: `sh scripts/check` PASS, `sh scripts/check --selftest` PASS, `dash scripts/check` PASS, `sh scripts/audit-history --range dce8254..HEAD` clean (2 commits, 18 blobs).
- Probes outside the repo: with each of the 8 table value bytes flipped in a copy of the image, the runner gave guest_failure with that check at result byte $46 (8 of 8); four wrong declarations (wrong ROM for predone, wrong reason for badsum, wrong outcome for nodone, a verdict declaration on the normal ROM for t0) each gave guest_failure, so a judge that missed a fault would turn its case red.

## Deviations from Plan

**1. [Rule 3 - Blocking] interleave_aba_order call sizes changed (file outside the plan list)**
- **Found during:** Task 1
- **Issue:** after the startup rewrite the conformance stage failed with `CONTROL shared-state NOT_RED` for `interleave_aba_order`. The toy adds one clock every fifth step of the whole process, so a single A-B-A case is red only for some counter phases, and every case that runs the machine before it (alphabetically) moves the phase. The original sizes (9000, 7000, 11000) were red in 3 of 5 phases I tried by shifting the ds_proto deadline in scratch copies. (9000, 6000, 11000) was red in all 8 phases tried, in both the Task 1 and the final state.
- **Fix:** t2 changed from 7000 to 6000 in the case expect and comment. The real runner passes it; nothing else changed.
- **Files modified:** tests/conformance/cases/interleave_aba_order.toml. **Commit:** a0d103a.

**2. [Rule 3 - Blocking] reset code order fixed by plan 03-02's run_until_abs**
- `run_until_abs` expects the instruction ends of the first 13 instructions of ds_proto (24 to 480). The library keeps those instructions (SEI, CLD, LDX, TXS, LDA, TAM, LDA, TAM, LDX #$00, STZ abs,X, INX, BNE, STZ), so the case is unchanged and green. A later change to the head of startup.inc must re-derive that case.

**3. Table entry of check 1 is an XOR mask, not a literal in code.** The plan keeps the `; SELFTEST-TARGET` on the LIT line of check 1. Check 1 is the first logic rung and the only manual-page literals I can cite (S8-17, H8-5) belong to later rungs, so check 1 is GUEST class with a mask entry. The selftest mutates that entry (`LIT 1, $00` to `$01`) and requires check 1 to turn $46; this proves the patched-table mechanism, which is what D-126 needs.

**4. Protocol switches live in verdict.inc, not ds_proto.s.** The plan text puts the `-D` switches in ds_proto.s; the code that reads them (dsv_init, dsv_seal) is library code, so the `.ifdef` blocks are there and ds_proto.s needs none. Behaviour is as specified.

**5. Cycle counts of the 13 new opcodes come from the usual HuC6280 table, not from a manual page re-read.** The manual PDFs are not in the repository and I had no copy here. The values (CMP abs 5, EOR abs 5, INC abs 7, LDY imm 2, STA abs,X 5, LDA abs,Y 5, JSR 7, RTS 7, PHA 3, PLA 4, PHP 3, PLP 4, JMP (ind) 7) are what the plan's W8 sizing waiver asks for, but they are not image-verified; the comment in cpu.c says so, and the opcode oracle of plan 03-09 is the check.

## Evidence discipline

E0 only. Checks 1, 2 and 8 are in-guest self-consistency; 3, 6 and 7 are literals tagged with manual pages (H8-5, S8-17) that are cited but NOT image-verified here (ER-1), so no ledger entry may rest on them yet; 4 and 5 are ASSUMED:LED-CPU-0010 (a ledger entry that later plans create; its flag-formula page is not verified, and H8-4 prose reads the CMP borrow the other way). No ledger status changed. No expected value comes from DuoSpool output. A `pass` here is not a support claim (ER-4). No accuracy, compatibility, cycle-accuracy, hardware or boot claim.

## Known Stubs

None. PLP, PHA and JMP (ind) are implemented but PHA and JMP (ind) are only exercised by check 8 and by the vector stubs that no interrupt reaches yet; the interrupt model arrives in later plans.

## Threat Flags

None beyond the plan's register. T-03-14 (judge recomputes summary and checksum, requires id and count, rejects a block present before the first step) is covered by five proven controls. T-03-16 (includes only from fixtures/lib, no third-party startup code) holds: the three library files are project-owned 0BSD; the lint rule is plan 03-37.

## Notes for next plans

- Hash pins (plan 03-06): the images are 262144 bytes; local Homebrew hashes of this commit: ds-proto e2f89ea4..., predone 2c3a6f79..., nodone 416f3aff..., badsum cb6399f3..., wrongid df59dfac... (read `build/gate/fixtures-authored.used`; the hashes change whenever a .inc or the source changes, as plan 03-37 and later fixtures edit the library).
- Library API for new fixtures: define `DSV_ID` and `DSV_CHECKS`, include the three files in order, `CHECK_TABLE DSV_CHECKS`, one `LIT k, v` per check in order 1..n, `fixture_main` ending in `rts`; use `CHECK_LIT k` (A observed, entry is the expected byte) or `CHECK_GUEST k` (store the model value in `GEXP` first, entry is a mask). The reset code does not zero other RAM: a fixture initialises what it reads. A fixture needing its own interrupt handler stores its address in `VEC_IRQ2`, `VEC_IRQ1`, `VEC_TIMER` or `VEC_NMI`.
- Only `a:` operands are used in the library; JMP absolute and BRA beyond +-127 bytes are not available (not implemented in cpu.c), so long code is reached by JSR.
- The mutation sweep (later plan) patches the value byte of entry k at file offset $1000 + 5 + 2*(k-1) + 1.
- `.checks` format used here: two header lines `id=N` and `checks=N`, then TAB-separated rows `index slug class meaning`; `#` lines are comments. Plan 03-10's optional fifth column `observe` extends it.
- The shared-state toy control depends on the counter phase (see deviation 1). If a later plan makes `CONTROL shared-state NOT_RED` appear for `interleave_aba_order` again, change the call sizes in that case (9000, 6000, 11000 is one set that held across the phases tried), and consider a phase-independent toy.
- `Current Plan:` in STATE.md read 7 before this plan; the state command advanced it. CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation.

## Self-Check: PASSED

Created files present: fixtures/lib/startup.inc, verdict.inc, check.inc, fixtures/src/ds_proto.checks, the five ds_proto_* case files. Commits a0d103a and 2b09d68 are ancestors of HEAD.
