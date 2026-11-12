---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 20
subsystem: bank-class-oracle-mmu-variants-st-bypass-w3-ledger
tags: [bankclass-oracle, mmu-mpr-variants, st-bypass, rombanks, ledger, w3-close, rule-text-guard]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the bus with bank_class[256] and the mmu_mpr fixture of 60 checks (03-16, 03-17), the dev sink and the observer matrix (03-19), the mutation sweep and observe-only rows (03-10), the mirror-oracle precedent (Phase 2)
provides:
  - tests/conformance/bankclass-oracle.awk, kind bankclass_oracle (buskinds.c), the accessor ds_bank_class in bus.c with enum ds_bank_class in ds_core.h, the runner option --bank-oracle, and scripts/conformance-run writing build/gate/bankclass.tsv in the same run (determinism-run and the toy controls pass it too)
  - mmu_mpr as three images of one source (256K, 384K, 1M by ROMBANKS; fixture-build variants list gained a ROMBANKS column), REQUIRED cases mmu_mpr_384k, mmu_mpr_1m and three mutate_sweep_mmu_mpr cases (57 literals each, exclude=49,59,60)
  - fixtures/src/st_bypass.s (DSV id 9, five GUEST checks, ST0-ST2 under four settings of MPR0), kind st_bypass (the dev log of twelve ST records and the guest verdict), REQUIRED cases st_bypass and mutate_sweep_st_bypass
  - ledger entries LED-CPU-0011, -0012, -0013, -0014, -0016, -0017 (all assumed, E0 evidence, one real pin each) and the LED-LOAD-0005 edit; selftests mutation-bankclass-drift and mutation-st-bypass-literal
affects: [03-21, 03-22, 03-24, 03-25, 03-27, 03-30, 03-32]

actuals:
  tokens: 20100
  tasks: 3
  commits: 3
plan_head_before: 52a5ac5f6c47aa9eea9d420d738b782bf4a87415
plan_head_after: 2481ae67e0e6cd0b07d174c7eea87f4b6a8bad77

tech-stack:
  added: []
  patterns:
    - "an oracle that re-derives from ledger text parses the statement, requires each clause literally and ends the clause at ; . : or the closing quote, refuses to print on a drift (ORACLE_RULE_TEXT_DRIFT, exit 1), and is written by conformance-run in the same run before the runner starts, so a reworded clause or a stale file can never be judged"
    - "one fixture source can build as several images: the variants list of fixture-build is STEM NAME ROMBANKS FLAGS, ROMBANKS goes to both tools, and literals that differ by image size come from conditional assembly on ROMBANKS (symbols BANK18 BYTE18 BANK19 BYTE19), with a single CHECK_LIT per check so the lint still sees one use"
    - "a device-register log kind compares the whole log with a list written from the fixture source: exactly N records, each kind, physical address, value, flags, zero spare bytes and strictly increasing stamps, with the guest verdict judged by judge_dsv1 in the same run"

key-files:
  created:
    - tests/conformance/bankclass-oracle.awk
    - tests/conformance/buskinds.c
    - fixtures/src/st_bypass.s
    - fixtures/src/st_bypass.checks
    - fixtures/manifest/mmu_mpr_384k.toml
    - fixtures/manifest/mmu_mpr_1m.toml
    - fixtures/manifest/st_bypass.toml
    - tests/conformance/cases/bankclass_oracle.toml
    - tests/conformance/cases/mmu_mpr_384k.toml
    - tests/conformance/cases/mmu_mpr_1m.toml
    - tests/conformance/cases/st_bypass.toml
    - tests/conformance/cases/mutate_sweep_mmu_mpr.toml
    - tests/conformance/cases/mutate_sweep_mmu_mpr_384k.toml
    - tests/conformance/cases/mutate_sweep_mmu_mpr_1m.toml
    - tests/conformance/cases/mutate_sweep_st_bypass.toml
    - ledger/entries/LED-CPU-0011.toml
    - ledger/entries/LED-CPU-0012.toml
    - ledger/entries/LED-CPU-0013.toml
    - ledger/entries/LED-CPU-0014.toml
    - ledger/entries/LED-CPU-0016.toml
    - ledger/entries/LED-CPU-0017.toml
  modified:
    - src/core/bus.c
    - src/core/ds_core.h
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - CMakeLists.txt
    - scripts/conformance-run
    - scripts/determinism-run
    - scripts/fixture-build
    - scripts/check
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - tests/conformance/REQUIRED
    - tests/conformance/cases/mmu_mpr_256k.toml
    - tests/conformance/cases/cpu_huc.toml
    - tests/conformance/cases/vdc_penalty_assumed.toml
    - ledger/entries/LED-LOAD-0005.toml
    - ledger/REQUIRED

key-decisions:
  - "The bank-class oracle hard-codes the ends of the five runs of the LED-CPU-0011 statement in its own arithmetic (comparisons) and guards the five clauses of the statement text, the mirror-oracle precedent; it does not parse numbers out of the text. The accessor is ds_bank_class(uint32_t bank) returning enum ds_bank_class (a bank above 255 is DS_BANK_CLASS_NONE);
    the table in bus.c keeps its BC_ names as aliases of the enum so the runner and the table share no class names beyond the enum"
  - "mmu_mpr checks 18 and 19 differ by image size and nothing else does: 256K bank $20 -> $78 and bank $3F -> $1F (as before); 384K bank $40 -> $20 and bank $7F -> $2F, the two banks where the 384K rule (b and $1F below $40, $20 + (b and $0F) from $40) differs from the 256K one; 1M bank $20 -> $20 and bank $7F -> $7F (no mirror). Every image keeps 60 checks, so the
    checks=60 literal of every mmu_mpr case and the exclude list 49,59,60 are shared; the TMA #0 observe items (bus_latch default: 248, 249, 249) are on the 384K and 1M cases because the ROM carries the same observe rows (NW1)"
  - "st_bypass needs a program that executes ST0 to ST2 under several MPR0 settings, and no existing fixture does (cpu_huc runs them with MPR0 = $F8 only), so a small authored fixture was added (see Deviations 1): twelve stores with distinct immediates under MPR0 = work RAM, a HuCard bank, an unmapped bank and the hardware page. The kind compares the whole dev log
    with the twelve records written in the case, and the guest verdict (five GUEST checks that no store reached the RAM, the first one the D-106 check that ST0 leaves RAM[0] unchanged) must pass"
  - "The six ledger entries are written as assumed with E0 evidence and a real pin each (0011 bankclass_oracle, 0012 mmu_mpr_256k, 0013 mmu_mpr_1m, 0014 cpu_huc, 0016 st_bypass, 0017 vdc_penalty_assumed). -0014 and -0016 are ER-2 candidates and stay assumed for plan 03-32; their statements are worded as the manual's (the 0014 'equals the manual's stated value' form,
    the 0016 form of research mismatch 1). Page facts were read as images by this plan (H8-7, H8-8, H8-9, H8-24, S8-80, S8-90, S8-101) and recorded with verifier and date; emulator readings in -0017 are labelled reports taken from 03-CONTEXT D-107 and the research notes, not re-read"
  - "LED-LOAD-0005 gets a comment line and an assumption that point at LED-CPU-0011 and -0012; its statement (parsed by mirror-oracle.awk), origin and REQUIRED line are unchanged, updated is 2026-11-11"

requirements-advanced: [CPU-06]
requirements-completed: [CPU-02, CORE-06]  # CPU-02: the map of all 256 banks, three image sizes, the stubs and the ST bypass are verified by fixtures and an oracle; CORE-06: both sinks, the observer matrix and the dev-log kinds exist and the W3 ledger entries are written. CPU-06 still needs the timer, speed, block and interrupt entries (03-32).

duration: 25min
completed: 2026-11-11
---

# Phase 3 Plan 20: Bank-class oracle, mmu_mpr size variants, ST bypass and the W3 ledger entries Summary

**The 256 bank classes of the core now equal, bank by bank, an awk re-derivation of the LED-CPU-0011 text that refuses to judge when a clause is reworded; mmu_mpr runs as 256K, 384K and 1M images from one source; the ST0-ST2 stores are logged at $1FE000, $1FE002 and $1FE003 under four MPR0 settings and never reach the RAM; and W3 has its six assumed ledger entries with real pins.**

## What was built

- **Task 1 (tracer), commit 39f162e.**
  - `LED-CPU-0011` (assumed, E0): the statement lists five clauses (banks 0x00-0x7F HuCard; 0x80-0xF7 unmapped; 0xF8-0xFB the work RAM mirrored four times; 0xFC-0xFE unmapped; 0xFF the hardware page of six 1 KiB windows and an empty 2 KiB). Sources: the manual facts H8-7, H8-8, H8-9, H8-24 read as images with verifier and date, the historical reports for the x4
    mirror, the PSG mirror and the unmapped run, and the Develo-book disagreement (it ends the run at F6) from 03-CONTEXT ER-8.
  - `tests/conformance/bankclass-oracle.awk`: guards the five clauses (literal substring ending at ; . : or the closing quote), then prints 256 rows `BANK TAB CLASS` from its own comparisons. On a drift it prints only `ORACLE_RULE_TEXT_DRIFT file=... clause=...` on stderr and exits 1.
  - Core: `uint32_t ds_bank_class(uint32_t bank)` in bus.c over the same const table the bus dispatches on, with `enum ds_bank_class` in ds_core.h. Runner: `buskinds.c` with `kind_bankclass_oracle` (256 assertions, one per bank; a malformed file, a repeated bank or an unknown class word is a harness fault), the option `--bank-oracle FILE`, CMake entry.
    `scripts/conformance-run` runs the awk script after the mirror oracle into `$bd/bankclass.tsv`, requires exactly 256 rows and passes the file to the real runner and to every toy runner; `scripts/determinism-run` requires and passes it too. Case `bankclass_oracle` (REQUIRED, pins LED-CPU-0011).
  - Selftest `mutation-bankclass-drift`: a copy of LED-CPU-0011 with `banks 0x80-0xF7 unmapped` changed to `0xF6` must stop the oracle (exit 1, the drift line, no row) while the unmutated copy prints 256 rows; then the live runner on a scratch oracle file must pass, and with bank 128 changed to HUCARD must be guest_failure.
- **Task 2, commit a5b77c0.**
  - `fixture-build`: the variants list is `STEM NAME ROMBANKS FLAGS`, ROMBANKS goes to ca65 and ld65, the four ds-proto lines carry 32, and `mmu_mpr mmu-mpr-384k 48` and `mmu_mpr mmu-mpr-1m 128` are new. `mmu_mpr.s` takes the bank and the literal of checks 18 and 19 from `.if ROMBANKS` symbols; the 256K image hash is unchanged (a77af5cf...). Manifests
    `mmu_mpr_384k.toml` (393216 bytes, c61fd02d...) and `mmu_mpr_1m.toml` (1048576 bytes, ad4d41b8...) were pinned from the host `shasum -a 256` of the built files.
  - Cases `mmu_mpr_384k` and `mmu_mpr_1m` (id=8, checks=60, observe=49:248;59:249;60:249, until_mclk 4000000, 63 assertions each) and `mutate_sweep_mmu_mpr`, `_384k`, `_1m` (57 swept literals each, exclude=49,59,60).
  - `fixtures/src/st_bypass.s` (id 9, five GUEST checks), `.checks`, manifest `st_bypass.toml` (9b5f8b6d...), `kind_st_bypass` in buskinds.c (78 assertions), cases `st_bypass` and `mutate_sweep_st_bypass` (REQUIRED). The case lists the twelve records `st=0x1FE000:0xA1; 0x1FE002:0xA2; 0x1FE003:0xA3; B1..B3; C1..C3; D1..D3` from the immediates of the fixture source and Table 2-2-3.
  - Selftest `mutation-st-bypass-literal`: a scratch mini-root builds the fixture, `st_bypass` must pass, then `st=0x1FE002:0xB2` changed to `0xB5` must give guest_failure naming store 5.
- **Task 3, commit 2481ae6.** `LED-CPU-0012` (unmapped reads, ignored writes, stubs), `-0013` (io_buffer), `-0014` (TAM and TMA one-hot, the equals-the-manual form, page facts S8-90, S8-101, H8-8), `-0016` (ST0 to ST2, the H8-8 Table 2-2-3 and S8-80 cite with the ROADMAP H8-3 error recorded in the disagreement field only), `-0017` (the VDC and VCE penalty with the
  labelled emulator reports). REQUIRED lines with the origins D-103, D-104, D-104, D-105, D-106, D-107. The `pins` arrays of mmu_mpr_256k (0012), mmu_mpr_1m (0013), cpu_huc (0014), st_bypass (0016) and vdc_penalty_assumed (0017) were filled; `LED-LOAD-0005` was edited as described above.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage build`, `conformance`, `ledger-entries`, `inventory`, `--selftest mutation-bankclass-drift`; Task 2 `--stage fixtures`, `conformance`, `sanitizers`, `--selftest mutation-st-bypass-literal`; Task 3 `--stage ledger-entries`, `inventory`, `ledger-negatives` (and `ledger-samples`, `rights-tree`, `licences`). The tracer
  gate ran the end-to-end verify before the expansion: it passed and the tracer can fail (below).
- Plan end: `sh scripts/check` PASS (55 PASS lines, about 105 s wall; all seven toy controls RED_OK including shared-state), `sh scripts/check --selftest` PASS (63 PASS lines), `dash scripts/check` PASS, `sh scripts/audit-history --range 52a5ac5..HEAD` clean (3 commits, 46 blobs, 1317 blob-path pairs, run before this docs commit).
- Bite shown with scratch edits (each restored byte for byte, nothing committed): the core table with bank $FB changed to UNMAPPED turns `bankclass_oracle` guest_failure naming bank 0xFB; ST1 written to $1FE003, ST0 written through the MPRs at logical $0000, and ST2 with a changed value each turn `st_bypass` guest_failure (the second at guest check 1, the D-106
  check); the selftests show a reworded clause stopping the oracle and one wrong literal turning st_bypass red.
- Manual facts: the hardware manual (sha256 40a32e5d...) and the software manual (sha256 51eeda39..., both equal to 03-RESEARCH) were copied to a scratch directory outside the repo and read as page images: H8-7 (the MPR byte as A13 to A20), H8-8 (reset MPR7 = 0, Table 2-2-2 with the first row misprinted MPR 1, Table 2-2-3 A1,A0 = 00, 10, 11 with A20 high), H8-9 and
  H8-24 (chip-enable windows, RAM CER at $1F0000-$1F7FFF, the timer read has 7 bits, the interrupt registers 3 bits), S8-80 (ST0: 03 nn, 2 bytes, 4 cycles, CE7 L, A1 L, A0 L), S8-90 (TAMi 53, 2^i, 5 cycles) and S8-101 (TMAi 43, 2^i, 4 cycles). Nothing was committed or copied.

## Deviations from Plan

**1. [Rule 3 - blocking] A new authored fixture for st_bypass.** The plan names no fixture source for the case, but "three ST records per MPR setting" needs a program that runs the stores under several MPR0 settings, and cpu_huc runs them only with MPR0 = $F8. Added `fixtures/src/st_bypass.s`, its `.checks`, a manifest and `mutate_sweep_st_bypass` (all REQUIRED); the
  plan's 31-file waiver is exceeded (38 files changed over the three commits).
**2. [Rule 3 - blocking] `fixture-build` variants carry ROMBANKS.** The script passed `-D ROMBANKS=32` to both tools, and a variant flag `-D ROMBANKS=48` would define the symbol twice; the variants list is now `STEM NAME ROMBANKS FLAGS` (the four ds-proto lines carry 32). The header comment of the script says so.
**3. [Rule 3 - files outside the plan list] `fixtures/src/mmu_mpr.s` and `mmu_mpr.checks`** (the conditional assembly and the two renamed check rows 18 and 19, slugs `mirror-rule-a` and `mirror-rule-b`), **`scripts/determinism-run`** (a third runner invocation: it needs the oracle file too) and **`tests/conformance/cases/vdc_penalty_assumed.toml`, `cpu_huc.toml`,
  `mmu_mpr_256k.toml`** (pins). The plan counts "both runner invocations"; there are three (the real run, the toy runners, determinism-run) and all take `--bank-oracle`.
**4. [Plan wording] The 384K variant checks banks $40 and $7F, not $20 and $3F.** The 384K rule equals the 256K rule below bank $40, so banks $20 and $3F would show no difference; the checks of the 384K image are placed where the two rules differ (bank $40 is file bank 32, tag $20; bank $7F is file bank 47, tag $2F). The 1M image reads banks $20 and $7F (tag $20 and $7F, no mirror).
**5. [Rule 2 - extra test] The drift selftest also proves the case turns red.** `mutation-bankclass-drift` additionally runs the live runner on a wrong oracle row (bank 128), so the selftest covers the comparison and not only the guard. No toy control was added for the bank table (the plan does not list one); the scratch probe above stands for it.
**6. [Note] The `[withheld] ids` line cap (standing note) was not approached.** No ledger id was added to the printed record, which holds 17.

No Rule 1 or Rule 4 deviation. The shared-state toy control stayed RED_OK (the standing note was not triggered).

## Evidence discipline

E0 only. The bank-class oracle has the same author as the core and re-derives one reading of the ledger text: it shows the table equals that text bank by bank, not that the text is right (ER-7, T-03-54). The mmu_mpr 384K and 1M literals come from the mirror rule of LED-LOAD-0003 (assumed) and the tag formula of the fixture's own BANKS segment. The st_bypass physical
addresses are Table 2-2-3 combined with H8-9 and H8-24, and the immediates are written in the fixture source and the case; none comes from DuoSpool output or another emulator. The penalty flag in the dev records is the labelled assumption of D-107 (LED-CPU-0017). Ledger: six entries written, all `assumed` with E0 evidence, one pin each; no status was promoted and none
of the eight ER-2 candidates was promoted (LED-CPU-0014 and -0016 are candidates and wait for plan 03-32). The disagreement fields name only readings recorded in 03-CONTEXT, 03-RESEARCH and the research notes, as reports; no emulator source was read or copied. No accuracy, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. The VDC and VCE stubs, the timer, interrupt-register, I/O and PSG windows keep the stubs of plan 03-16 and are now described by LED-CPU-0012 and -0013. Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-54 (correlated oracle): labelled E0/E2 in the case, the kind header and LED-CPU-0011, guarded by the clause check, no promotion rests on it. T-03-55 (stale oracle file): `conformance-run` removes and rewrites `bankclass.tsv` in each run before the runner starts, and requires 256 rows. T-03-56 (ST cite error): H8-8 Table 2-2-3 and
S8-80 cited in the entry, the case and the fixture; the ROADMAP H8-3 error is recorded only in the LED-CPU-0016 disagreement field. The new runner option `--bank-oracle` reads a file given by the harness and is test code only.

## Notes for next plans

- **03-21, 03-22, 03-27 (timer, interrupt registers, pad):** the mmu_mpr checks 32 to 48 (timer, $1402 and $1403, I/O window reads) are shared by the three images, so one edit of `mmu_mpr.s` changes three images: re-pin the three manifests (`mmu_mpr.toml`, `mmu_mpr_384k.toml`, `mmu_mpr_1m.toml`) from `build/gate/fixtures-authored.used` and the host `shasum -a 256`.
  New rows go after check 60 (the cases say `checks = 60` in nine places: mmu_mpr_256k, _384k, _1m, mmu_mpr_tma_zero_latch, _keep, mmu_mpr_observe_missing and the three sweeps) and a new observe row must be added to `exclude=` of the three sweeps and to the `observe=` items of the six guest cases. LED-CPU-0012 and -0013 name those rows; if they change, edit the
  statements and keep the pins.
- **st_bypass:** the dev log of the st_bypass ROM must hold exactly the twelve ST records: a change that makes startup or the checks touch the hardware page, or that changes the dev record stamp (plan 03-21 may refine it with the access stamp), must keep the stamps strictly increasing and the flags (penalty and physical) as they are, or update `st_bypass.toml` on purpose.
- **Bank classes:** a new bank class (none is planned) means a new enum value, a new clause in the LED-CPU-0011 statement, a new clause in the awk guard and a new word in `buskinds.c`; the four of them are checked against each other, so a half-done edit turns the gate red.
- **Runner inputs:** any new invocation of `ds_runner` that runs the whole case list must pass `--bank-oracle` (conformance-run writes `$bd/bankclass.tsv`; a runner without it gives harness_failure on `bankclass_oracle`).
- **03-32 (ledger finalise):** LED-CPU-0014 and -0016 are the ER-2 candidates of this plan, assumed now; their pages were read as images here (H8-8, S8-80, S8-90, S8-101) and must be re-read there. The standing note on LED-CPU-0015 stays open: its disagreement column still names no emulator reading. LED-CPU-0017 revisit_phase is 4 (the video phase), the others 3.
- **Timing:** `sh scripts/check` took about 105 s wall here (conformance stage about 27 s including the new cases: the three sweeps cost under 1 s together, st_bypass 0.1 s); the toy runners now run 8 more cases each.
- CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: bankclass-oracle.awk, buskinds.c, st_bypass.s and .checks, the three manifests, the eight case files, the six ledger entries (LED-CPU-0011, -0012, -0013, -0014, -0016, -0017). Commits 39f162e, a5b77c0 and 2481ae6 are ancestors of HEAD.
