---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 32
subsystem: w8-ledger-finalisation-and-evidence-lint
tags: [ledger, er-2, supported, e1-documentary, page-image-reread, cites, fixture-lint, inventory-check, assertion-classes, w8, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the eight ER-2 candidate entries written as assumed (03-15, 03-20, 03-26, 03-29, 03-34), the cite registry and fixture-lint (03-37), the per-fixture .checks files, DSV1 and the guest kind (03-01 to 03-31)
provides:
  - "seven ledger entries supported (E1, documentary, ER-2 record in each entry file): LED-CPU-0001, LED-CPU-0002, LED-CPU-0005, LED-CPU-0014, LED-CPU-0016, LED-IRQ-0002 and LED-IRQ-0004 (the page-backed part (a) only); every other entry stays assumed, open or deferred"
  - "LED-CPU-0007 reviewed and kept assumed (the cost clause has only E2 evidence, ER-3); the candidate list is therefore 7 supported of 8"
  - "fixtures/lib/cites.txt: every page behind a candidate re-read as a page image by gsd-executor on 2026-11-11, the three ocr rows (S8-7, S8-8, S8-9) now img, four rows added (S8-94, S8-96, S8-98, S8-100); no ocr row remains"
  - "fixture-lint rules FXL_OCR_BACKS_SUPPORTED (a page named in a FACT source of a supported entry, or cited by a MAN: check backing one, must be img) and FXL_UNKNOWN_LEDGER_ID (an ASSUMED: id must name a ledger entry), a ledger reader stage with an END sentinel, mini-roots ocr-backs-supported, ok-img-backs-supported, unknown-ledger-id and selftests mutation-fixture-lint-ocr, -ledger-id and -dead-ledger"
  - "assertion classes in guest evidence: fixture-build writes build/gate/fixtures/NAME.classes, the guest judge refuses a missing or wrong classes list and prints classes: man=N guest=N assumed=N in the evidence file of every guest case; selftest mutation-guest-classes"
  - "INV_CONST_LEDGER in inventory-check (every ledger id named in constants.def and model_params.def exists), stage 1 scripts/lib/inventory-defs.awk, mini-roots const-unknown-ledger and ok-const-known, selftests mutation-inventory-const-ledger and mutation-const-ledger-dead-stage"
affects: [03-33, 03-34, 03-35, 03-36, 03-38]

requirements-advanced: [CPU-06, FIX-01, FIX-03]
requirements-completed: []

actuals:
  tokens: 60000
  tasks: 3
  commits: 3
plan_head_before: e45957250803396c5d0815d493bd2df61ac4f875
plan_head_after: 35f7ea523ce264900e327f9773f434202e47d4a3

tech-stack:
  added: []
  patterns:
    - "a promotion is a recorded procedure: the entry file carries the ER-2 record (condition by condition, verifier, date, pages, where each literal comes from) and the statement is narrowed to what the pinned case measures; a clause with only E2 evidence keeps the whole entry assumed"
    - "an evidence lint that reads the ledger: a stage-0 reader prints one record per entry and per FACT source with an END sentinel and a tested status, and the rules stage requires the sentinel (CR-01); a control mini-root may carry its own cite registry and ledger so that a red case does not depend on the live flags"
    - "assertion classes without a total: the evidence file prints the three classes (and control=N for a case whose assertions are the runner's own comparisons), they sum to the assertions of the case, and the phase-wide figures below are never added into one number"

key-files:
  created:
    - scripts/lib/inventory-defs.awk
    - tests/gate/fixture-lint/ocr-backs-supported/
    - tests/gate/fixture-lint/ok-img-backs-supported/
    - tests/gate/fixture-lint/unknown-ledger-id/
    - tests/gate/inventory/const-unknown-ledger/
    - tests/gate/inventory/ok-const-known/
  modified:
    - fixtures/lib/cites.txt
    - ledger/entries/LED-CPU-0001.toml
    - ledger/entries/LED-CPU-0002.toml
    - ledger/entries/LED-CPU-0005.toml
    - ledger/entries/LED-CPU-0007.toml
    - ledger/entries/LED-CPU-0014.toml
    - ledger/entries/LED-CPU-0015.toml
    - ledger/entries/LED-CPU-0016.toml
    - ledger/entries/LED-IRQ-0002.toml
    - ledger/entries/LED-IRQ-0004.toml
    - scripts/check
    - scripts/fixture-lint
    - scripts/lib/fixture-lint-rules.awk
    - scripts/fixture-build
    - scripts/inventory-check
    - tests/conformance/guestkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h

key-decisions:
  - "Seven of eight candidates are supported and the wording of each is an equality with the cited page, never a hardware or accuracy claim; LED-CPU-0007 stays assumed because its headline (17 + 6x) can only be shown through the timer counter, which uses the community-reported cycle length (ER-3), and no E1 trace-based count exists"
  - "Statements were narrowed to what the pinned case measures and what was re-read in this plan: LED-CPU-0002 drops T mode on AND, EOR and ORA, BBS and the other relative branches (modelled, not asserted); LED-CPU-0014 claims i = 0 to 6 for TAM and the reset value for MPR7; LED-IRQ-0002 drops the claim that a reload write leaves the counter alone (a reading, A1); LED-IRQ-0004 keeps only the page-backed clauses and moves the acknowledge and masked-borrow rules to the assumption field"
  - "The cite check is by page label: a page named in a FACT source of a supported entry must be img (entry level) and a MAN: check naming such a page must be img (check level); a range of pages in a FACT ref (S8-17 to S8-20) expands over the registered labels"
  - "A control mini-root can carry its own fixtures/lib/cites.txt and ledger/entries (fixture-lint reads the root's own when present, else the repository's), because after the re-read no repository row is ocr and the red control needs one"
  - "The classes line carries control=N instead of the three classes for a protocol control, a fail-set control and the refusal controls, so man + guest + assumed + control equals the assertions in all 39 guest cases"

patterns-established:
  - "re-read by comparison: H8-25 was transcribed again cell by cell from a 200 dpi image without opening the TSV, and the two readings were compared by a script; the differences were exactly the printing errors recorded earlier (opcodes 34 to 37 and B5) and the S8 pages decided them"

duration: 190min
completed: 2026-11-11
---

# Phase 3 Plan 32: Ledger finalisation under ER-2, evidence lint rules and assertion classes Summary

**Seven of the eight ER-2 candidates are now supported (E1, documentary: the model equals the cited manual page, nothing more) after every page behind them was re-read as a page image in this plan; LED-CPU-0007 stays assumed because its cost clause has only E2 evidence; fixture-lint now rejects an ocr page behind a supported entry and an ASSUMED id with no entry, guest evidence is split by class, and every constant or parameter row must name a ledger entry.**

## What was built

- **Task 1 (tracer), commit 9138083.** The page re-read and the whole promotion procedure for LED-CPU-0001, plus the two lint rules that guard it. Pages (PDFs in a scratch directory outside the repository, sha256 equal to the 03-RESEARCH values 40a32e5d..., 51eeda39... and 8e92ead8...; renders at 100 and 200 dpi, never committed): H8-4, H8-5, H8-6, H8-8, H8-9, H8-12 to H8-16, H8-18, H8-20 to H8-22, H8-24, H8-25; S8-7 to S8-9, S8-17, S8-18, S8-20, S8-24, S8-25, S8-30, S8-49, S8-53, S8-55, S8-67, S8-68, S8-70, S8-74, S8-76, S8-78, S8-80 to S8-82, S8-88, S8-89, S8-90, S8-94, S8-96, S8-98, S8-100, S8-101; CDBIOS-p28. The H8-25 map was transcribed again by hand (bytes and cycles of 234 cells, 22 empty cells) without opening the opcode TSV and compared with it by script: 230 of the 234 defined cells agree, and the differences are the printing errors the TSV header already records (opcodes 35, 36, 37 and B5 differ in the printed numbers, 34 in the mnemonic); the six instruction pages that decide them (S8-18, S8-25, S8-53, S8-55, S8-67, S8-68) were read as images and agree with the TSV. S8-7 to S8-9 were read as images: their reference-page columns print several numbers that differ from the page footers (INC S8-58, ORA S8-68, SET S8-88, TAI S8-90, TAMi S8-88) and the footers were used. All three ocr rows are now img; CDBIOS-p28 was already img in cites.txt (the brief called it ocr) and was re-read and re-stamped.
- **Task 2, commit 896de83.** The ER-2 checklist for the other seven candidates and the PENDING pins (below).
- **Task 3, commit 35f7ea5.** Guest evidence by class and INV_CONST_LEDGER (below).

## ER-2 outcome per candidate (verifier gsd-executor, 2026-11-11; the ER-2 record is also in each entry file)

| Entry | Status | Conditions met | Note |
|-------|--------|----------------|------|
| LED-CPU-0001 | supported | all five: H8-25 and the six deciding S8 pages as images; literals are the TSV rows (page transcription made before cpu_ops.def); wording equals the manual; withheld complete; pin TEST:opcode_table_manual | statement says the map H8-25 for 234 defined and 22 empty cells and the instruction pages for the six misprinted cells; the other S8 pages were read by 03-08 and not again here, the statement does not rest on them |
| LED-CPU-0002 | supported | all five: S8-17, S8-74, S8-20, S8-24, S8-30, S8-70, S8-90, S8-101, S8-76, S8-78, S8-49, H8-4, H8-25; the 18 CYCLES literals of cycle_adders_manual are page numbers; pin TEST:cycle_adders_manual | narrowed to what the case measures (taken BEQ and BBRi +2, T mode +3 on ADC, decimal +1 on ADC and SBC, BRK RTI TAMi TMAi CSL CSH) |
| LED-CPU-0005 | supported | all five: H8-25 cells and H8-18; the literals of spd_speed_trace are the 3 cycles and the low first speed; pin TEST:spd_speed_trace | the pinned case also asserts the switch rule of LED-CPU-0006 (not claimed here, stays open) |
| LED-CPU-0007 | assumed | condition not shown: E1 evidence for the cost clause | pages H8-6, S8-88, S8-89, S8-94, S8-96, S8-98, S8-100 re-read and agree; the cost rows (cpu_blk checks 36 to 38, cpu_blk_len0 check 41) count timer steps with the community cycle length, E2 by ER-3; the E1-capable clauses (length 0 = 65536, A X Y S returned: MAN:H8-6 checks 30, 31, 39, 40, 42) were not split off because the ids are final; what would promote it is recorded in the entry |
| LED-CPU-0014 | supported | all five: S8-90, S8-101, H8-8, H8-18, H8-9, H8-24; cpu_huc checks 35 to 50 are MAN class; cycles 5 and 4 are items of cycle_adders_manual; pin TEST:cpu_huc | claims i = 0 to 6 for the TAM write and TMA read and the reset value of MPR7 (no pinned check writes MPR7 through TAM) |
| LED-CPU-0016 | supported | all five: H8-8 Table 2-2-3, S8-80 to S8-82, H8-9, H8-24; st_bypass literals are the three physical addresses and the immediates written in the fixture; pin TEST:st_bypass | the physical addresses are a combination of three pages (recorded as a derivation in the assumption field) |
| LED-IRQ-0002 | supported | all five: H8-20 to H8-22; timer_pure_function asserts 3072 and reload + 1 as literals; pin TEST:timer_pure_function (E1 only through this case, ER-3) | period reload + 1 is a derivation from the printed 143 usec to 18.3 msec range (1 to 128 steps), named as such; "a reload write leaves the counter alone" left out of the statement; the in-guest rows are E2 and not cited as support |
| LED-IRQ-0004 | supported, part (a) only | all five for the restricted statement: H8-5, H8-12 to H8-16, H8-18, H8-24; irq_ctl MAN-class checks 2 to 14 and 16 to 29; pin TEST:irq_ctl_pre | level inputs for IRQ1 and IRQ2 turned out to be page-backed (H8-13 says level sense); the acknowledge-by-write-only rule and the masked-borrow rule are in the assumption field and withheld; H8-14 against H8-15 and H8-21 stays in the disagreement column |

No entry other than these seven is above assumed, open or deferred; LED-TOOL-0001 and LED-IO-0006 keep their NONE reasons; the remaining PENDING pins (LED-AUD, LED-IRQ-0001, LED-VID, LED-PROF-0001) belong to phases 4 to 6.

## Verification

- Task verifies as written, all PASS: Task 1 `--stage fixture-lint`, `fixture-lint-negatives`, `ledger-entries`, `ledger-negatives`; Task 2 `--stage ledger-entries`, `inventory`, `fixture-lint`, `fixtures`; Task 3 `--stage fixture-build`, `conformance`, `inventory`, `inventory-negatives` and `--selftest mutation-const-ledger-dead-stage`. New selftests all PASS: mutation-fixture-lint-ocr, mutation-fixture-lint-ledger-id, mutation-fixture-lint-dead-ledger, mutation-guest-classes, mutation-inventory-const-ledger, mutation-const-ledger-dead-stage; the three older fixture-lint selftests still PASS after the new `--ledger` argument.
- Plan end: `sh scripts/check --timing` PASS (8 min 2 s wall under a loaded machine; conformance 87 s and determinism 41 s warn at 20 s as before, every other stage under 25 s; fixture-lint-negatives 4 s with the three new mini-roots, inventory-negatives 3 s), `sh scripts/check --selftest` PASS (109 PASS lines, no FAIL: the 103 of 03-31 plus the six new selftests), `sh scripts/audit-history --range e459572..HEAD` clean (3 commits, 35 blobs). `dash scripts/check` was not run (optional).
- Tracer gate: the interactive mode re-ran the automated verify of Task 1 end to end (four stages green) before the expansion.

## Assertions by class (ER-4): totals, never one number

Guest evidence of the 39 guest-kind cases of the gate (build/gate/evidence/*.txt, line `classes:`), summed over cases (a ROM run by several cases is counted once per case): MAN 497, GUEST 82, ASSUMED 477, and 267 control assertions (the runner's own comparisons of planted, failing, refused and edge cases, which assert no fixture row); man + guest + assumed + control = 1323 = the assertions of those cases, equal for each case. Distinct check rows of the 13 authored fixtures (fixture-lint counted 561): MAN 378, GUEST 58, ASSUMED 125. MAN means a manual page literal is asserted, GUEST an in-guest computation, ASSUMED the project's own model; a pass is not a support claim, and in-guest timer rows are classed GUEST:timer-e2 or ASSUMED, never MAN as E1.

## Deviations from Plan

**1. [Plan wording] Statements are narrowed, not only reworded.** The plan says to reword to "equals the manual's stated value". For LED-CPU-0002, -0014, -0016, LED-IRQ-0002 and -0004 the statement now names only what the pinned case measures and what was re-read, and the rest is in the assumption field (reasons in the table above). This is the conservative direction and the only way to keep a supported statement and its evidence the same size.
**2. [Plan wording] LED-CPU-0007 stays assumed** (condition not shown, above); the plan allows this ("promote only when every condition holds").
**3. [Rule 3 - blocking, files outside the plan list] Mini-roots carry their own registry and ledger.** After the re-read no repository cite is ocr, so ocr-backs-supported needs a mini-root with its own fixtures/lib/cites.txt and ledger/entries; fixture-lint gained `--ledger DIR` and reads the root's own registry and ledger when present. Added ok-img-backs-supported (same shape, img flag, clean) and ok-const-known as over-firing controls. The older fixture-lint selftests pass `--ledger ledger/entries` (their scratch copy of scripts/ is not under the repository).
**4. [Rule 3 - new file] scripts/lib/inventory-defs.awk** holds stage 1 of the constants rule so that the dead-stage selftest can empty it; the plan lists scripts/inventory-check only. The mini-root const-unknown-ledger is a clean root plus constants.def (with one valid and one unknown id) rather than a copy of case-missing-field, which is red for another reason.
**5. [Plan wording] The classes line has a control=N form.** The plan says man, guest and assumed sum to the assertions. Observe-row judgments and the policy-diff comparison are counted in the class of their row, and a case whose assertions are the runner's own (verdict, fail-set, refusal, exact-edges, policies guard) prints `man=0 guest=0 assumed=0 control=N`; otherwise the sum would not equal the assertions in several cases (the ds_proto protocol controls, the pad fail-set cases and the refusal controls).
**6. [Process] No fixtures/src/*.checks file was changed.** The plan lists six .checks files for class or cite fixes. I reviewed the MAN-class rows behind the candidates (cpu_huc 35 to 50, irq_ctl 2 to 29, st_bypass, cyc_probe, spd_speed 1 and 13, tim_timer MAN rows): all cites are img and the literals are page values; spd_speed check 1 and tim_timer checks 10 and 13 are MAN class but observed through timer steps (E2), and no entry cites them as E1, so their class stays. cpu_huc checks 43 to 47 cite S8-90 for the TAM write while the RAM map F8 is H8-9 and H8-24 (both re-read and listed in LED-CPU-0014). ledger/REQUIRED and poweron_v2_draw_order.toml needed no change (LED-PROF-0003 was already TEST:poweron_v2_draw_order with its back-link).
**7. [Process] The per-plan commit ledger file was not created;** the start commit e459572 (full hash in the frontmatter) is recorded and `commits: 3` is measured with `git rev-list --count e459572..HEAD` before this SUMMARY and the docs commit.

## Evidence discipline

E1 documentary only for the seven supported entries (the model equals a cited page, image-read in this plan); every other entry keeps assumed, open or deferred. No hardware measurement, no emulator comparison, no accuracy, compatibility, cycle-accuracy, interrupt-accuracy, pad-accuracy or boot claim anywhere in the new text. No manual page, image or text was committed: PDFs and renders stayed in a scratch directory outside the repository, and the repository holds page labels and short numeric facts only (ER-9). The LED-CPU-0015 disagreement column was filled only from the records that exist (03-CONTEXT ER-8 and D-105, the 03-17 SUMMARY and the plan brief relaying the discuss advisors' notes), labelled as advisor notes and community or code observations; no emulator is named because no repository record names one, and no source was read.

## Known Stubs

None. `.planning/WINDOWS.md` is absent, so nothing was appended.

## Threat Flags

None beyond the plan's register. T-03-89 (overclaimed support): ER-2 record per entry, ledger-check E1 FACT rule, the new ocr rule with a red mini-root and a mutation selftest, eight candidates only. T-03-90 (PDF identity): sha256 of the three PDFs checked before reading. T-03-91 and T-03-105: renders in scratch, nothing committed.

## Notes for next plans

- **03-33 (README):** README.md line 12 still says "Nothing is claimed as supported". Seven entries are now supported (E1 documentary): LED-CPU-0001, -0002, -0005, -0014, -0016, LED-IRQ-0002, LED-IRQ-0004 (part a). The allowed wording is in 03-CONTEXT specifics; docs-claims passes today only because it checks claim words, not this sentence.
- **LED-CPU-0007 promotion path:** a trace-based block-transfer item in cycle_adders_manual style (17 + 6x for a stated length, measured from the instruction sink) would give it E1 evidence; it needs a fixture change (cyc_probe) and a new hash pin, so it was not done here.
- **Owner-gate items:** none pending from this plan. ADR 0008 is still proposed-default, as ER-2 notes; the promotions follow it.
- **Verification report:** list the seven supported entries with their pins and the class totals above; the first evidence of the classes line is in build/gate/evidence/ID.txt.

## Self-Check: PASSED

Created files present: scripts/lib/inventory-defs.awk, tests/gate/fixture-lint/{ocr-backs-supported,ok-img-backs-supported,unknown-ledger-id}, tests/gate/inventory/{const-unknown-ledger,ok-const-known}. Commits 9138083, 896de83 and 35f7ea5 are ancestors of HEAD.
