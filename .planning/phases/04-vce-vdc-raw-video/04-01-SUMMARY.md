---
phase: 04-vce-vdc-raw-video
plan: 01
subsystem: evidence
tags: [cites, ledger, fixture-lint, docs-check, er-1, er-2, manuals, w1]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: cites.txt registry and fixture-lint (03-37), ledger and REQUIRED machinery, docs-check claim-word rule (03-33)
provides:
  - cites.txt image-verified rows for H7-2..H7-16, H7-18..H7-25 and H6-1..H6-5 (28 rows) with the page errata, plus H7 and H6 legend lines
  - TOOLS.md records of the HuC6270 and HuC6260 manual copies (URL, size, sha256, offsets, read date)
  - H7-n and H6-n accepted by both fixture linters (islabel and the manual-cite verdict_source pattern) with three mini-roots
  - 17 new ledger entries LED-VID-0004..0018, LED-IRQ-0009, LED-IRQ-0010 (assumed, 0015 deferred) and 10 amendments, REQUIRED rose 46 to 63
  - docs-check withheld words renders correctly, displays correctly, plays games, a second README policy line and the claim-word-video negative mini-root
affects: [04-02, 04-03, 04-04, 04-06, 04-08, 04-11, 04-16]

actuals:
  tokens: 12750
  tasks: 3
  commits: 3
plan_head_before: 925662ad0da7e6f402f324719e7692313a0418ca
plan_head_after: 543bbd13d8461467b4435388bfcfae4ad1d66b6f

tech-stack:
  added: []
  patterns:
    - "A cites.txt row is one TAB-separated line (label, img, verifier, date, fact in the registrar's words); the manual copies are read from a scratch directory and never enter the repository (ER-9)"
    - "A new ledger entry states its level tag first and carries the page-true part of an E1 candidate apart from its modelling clauses"

key-files:
  created:
    - ledger/entries/LED-VID-0004.toml through LED-VID-0018.toml (15 files)
    - ledger/entries/LED-IRQ-0009.toml
    - ledger/entries/LED-IRQ-0010.toml
    - tests/gate/fixture-lint/ok-h7-cite/
    - tests/gate/fixture-lint/h7-unregistered/
    - tests/gate/fixture/ok-h7-verdict-source/
    - tests/docs/claim-word-video/
  modified:
    - fixtures/lib/cites.txt
    - fixtures/lib/TOOLS.md
    - scripts/lib/fixture-lint-rules.awk
    - scripts/lib/fixture-rules.awk
    - ledger/REQUIRED
    - ledger/entries/LED-VID-0001.toml
    - ledger/entries/LED-VID-0002.toml
    - ledger/entries/LED-VID-0003.toml
    - ledger/entries/LED-IRQ-0001.toml
    - ledger/entries/LED-IRQ-0008.toml
    - ledger/entries/LED-CPU-0012.toml
    - ledger/entries/LED-CPU-0016.toml
    - ledger/entries/LED-CPU-0017.toml
    - ledger/entries/LED-PROF-0002.toml
    - ledger/entries/LED-PROF-0003.toml
    - scripts/docs-check
    - README.md

key-decisions:
  - "Every literal of the E1 candidates was found on its cited page image, so no MAN: use is blocked; no candidate was promoted (promotion stays with ER-2 in 04-13 or 04-16)"
  - "LED-CPU-0016 was touched only in its assumption line and a comment line; statement, evidence, status and pin are byte-identical (F-47)"
  - "scripts/check was not edited: the selftest pair list of mutation-docs-claim-word stays at four controls and the new claim-word-video control is proven red by the docs-claims-negatives stage and by a one-off deletion of the tagged statement in a scratch copy"

requirements-advanced: [VID-01, VID-02, VID-03, VID-04, VID-05, VID-06]
requirements-completed: []  # evidence plumbing only; the six ids complete in 04-16

duration: 90min
completed: 2026-11-11
---

# Phase 4 Plan 01: Evidence plumbing Summary

**Image-verified H7 and H6 manual cites, both fixture linters taught the H7-n and H6-n labels, 17 new assumed ledger entries with 10 amendments, and the D-181 claim words refused by docs-check with a red control; data only, no C, no promotion.**

## What was built

- **Task 1, commit 1f53bf9.** The two manual copies were downloaded into a scratch directory and matched the D-179 byte sizes and SHA-256 values (HuC6270 1,497,291 bytes, HuC6260 315,186 bytes). Each page was rendered at 100 dpi and read as an image; 28 cites.txt rows were appended, verifier gsd-executor, read date 2026-11-11 for every row. `islabel` in fixture-lint-rules.awk and the manual-cite pattern in fixture-rules.awk accept H7-n and H6-n. Mini-roots: ok-h7-cite (MAN:H7-4 and MAN:H6-3 pass), h7-unregistered (MAN:H7-17, exactly FXL_UNKNOWN_CITE), ok-h7-verdict-source (manual-cite:H7-4+manual-cite:H6-3 passes). TOOLS.md records both manuals; no PDF, page image or page text is tracked.
- **Task 2, commit a787e76.** 17 new entries and 10 amendments (below); ledger/REQUIRED rose from 46 to 63 lines, each origin equal to its entry. The README supported list is unchanged (docs-claims green).
- **Task 3, commit 543bbd1.** The control tests/docs/claim-word-video was written first and seen red (CONTROL BROKEN, wanted DOCS_CLAIM_WORD, got none), then the regex gained renders correctly, displays correctly and plays games; README gained a second line starting `Withheld claim words:` with the 14 D-181 additions, each in double quotes. One statement still carries the `rule-claim-word` tag.

## Pages read (ER-1), all verifier gsd-executor, all 2026-11-11

- HuC6270 (label n is PDF page n+3): H7-2, H7-3, H7-4, H7-5, H7-6, H7-7, H7-8, H7-9, H7-10, H7-11, H7-12, H7-13, H7-14, H7-15, H7-16, H7-18, H7-19, H7-20, H7-21, H7-22, H7-23, H7-24, H7-25 (23 rows; H7-1 and H7-17 are used by no decision).
- HuC6260 (label n is PDF page n+2): H6-1, H6-2, H6-3, H6-4, H6-5 (5 rows). The contents page lists the sync generator at H6-6; that page is absent from the copy (recorded in the legend and TOOLS.md).
- H8-8, H8-24 and H8-25 keep their existing img rows (D-179, ER-1).
- Page errata recorded in the fact column: H7-2 prints VDW in the data cell of the R0E (VCR) row; H7-4 says more than 17 while H7-19 says 17 or more; H7-25 prints A0=0 for both data steps; H7-8 prints the SM (0,1) row as 1 0.

## The 17 new ledger entries (origin = REQUIRED text)

| Id | Origin | Level and kind |
|----|--------|----------------|
| LED-VID-0004 | 04-CONTEXT D-136 | VL0/R1 grid, assumed |
| LED-VID-0005 | 04-CONTEXT D-139 | R1 vertical cycle and window, assumed |
| LED-VID-0006 | 04-CONTEXT D-146 | VL0 port and registers, E1 candidate (page-true part only) |
| LED-VID-0007 | 04-CONTEXT D-147 | VL0 VRAM, assumed |
| LED-VID-0008 | 04-CONTEXT D-144 | VL0 status, E1 candidate |
| LED-VID-0009 | 04-CONTEXT D-156 | R1 scroll latches, E1 candidate (H7-9 added, F-12) |
| LED-VID-0010 | 04-CONTEXT D-157 | R1/R2 background chain, assumed |
| LED-VID-0011 | 04-CONTEXT D-159 | R2 sprite decode, E1 candidate (SM = 0 table) |
| LED-VID-0012 | 04-CONTEXT D-160 | R1 overflow and collision, assumed |
| LED-VID-0013 | 04-CONTEXT D-150 | VL1 SATB DMA, E1 candidate |
| LED-VID-0014 | 04-CONTEXT D-151 | VL1 VRAM-VRAM DMA, E1 candidate |
| LED-VID-0015 | 04-CONTEXT D-135 | deferred (VL2, BSY, width modes) |
| LED-VID-0016 | 04-CONTEXT D-152 | VL0 VCE window, E1 candidate |
| LED-VID-0017 | 04-CONTEXT D-141 | CR content 00h and 01h only (F-23), E1 candidate |
| LED-VID-0018 | 04-CONTEXT D-161 | DSVR v1 record definition, assumed |
| LED-IRQ-0009 | 04-CONTEXT D-144 | VL0 IRQ1 level, assumed |
| LED-IRQ-0010 | 04-CONTEXT D-142 | R1 RCR counter and RR, assumed |

All carry pinning_test PENDING:4 and revisit_phase 4. Amended: LED-VID-0001, -0002, -0003, LED-IRQ-0001, LED-IRQ-0008, LED-CPU-0012, LED-CPU-0016 (assumption line and comment only), LED-CPU-0017, LED-PROF-0002, LED-PROF-0003; each keeps its id, origin and REQUIRED line, and no pin changed.

## Verification

- Task 1 verify (fixture-lint, fixture-lint-negatives, fixtures, fixture-negatives, rights-tree, licences): PASS. Task 2 verify (ledger-entries, ledger-negatives, inventory, fixture-lint, docs-claims): PASS. Task 3 verify (docs-claims, docs-claims-negatives, `--selftest mutation-docs-claim-word`): PASS.
- Full `sh scripts/check` after each task commit: PASS (about 4.4 min local wall time on a shared machine). `sh scripts/check --selftest`: PASS. `sh scripts/audit-history --range 925662a..HEAD`: clean (3 commits). `git ls-files` shows no .pdf, .png or .ppm.
- No C file changed, so no Linux gate-gcc part is owed by this plan. `dash scripts/check` and the second awk are owed at the Phase 4 checkpoint and close-out, not here.
- Evidence class: E0 mechanism plus documentary page reads. No hardware claim, no promotion, no withheld word used outside the quoted policy lines.

## Deviations from Plan

None to the plan's scope. Findings and small notes the orchestrator or later plans should know:

1. **[Finding] pcetech note on LED-VID-0002.** Re-reading pcetech.txt (as the plan asked) shows its register section (D1 = 10 MHz, D0 = 7 or 5 MHz) agrees with its clock section (10.8, 7.16, 5.36 MHz from master/2, /3, /4); the old "internally inconsistent" note did not survive. LED-VID-0002 now says so and carries first-hand H6-3 as its FACT source.
2. **[Finding] No literal missing from a page.** Every numeric fact cited by the 17 entries and the candidate refinements was seen on the page images; no MAN: use is blocked. The research errata (H7-2 VDW, H7-4 versus H7-19, H7-25 A0 = 0, H7-8 SM row) were confirmed as printed.
3. **[Note] Reach of the linter edits.** `MAN:H7-n` and `MAN:H6-n` class tags were already accepted for any registered label by the generic class regex; `islabel` governs only the FACT refs of supported entries (FXL_OCR_BACKS_SUPPORTED), so the edit matters when a candidate is promoted. The verdict_source pattern edit was required as written.
4. **[Note] Commit message count.** The Task 1 commit message says 25 HuC6270 rows; the correct count is 23 (plus 5 HuC6260 = 28). The files are right; history was not rewritten.
5. **[Note] Line lengths.** Several amended ledger lines were shortened to stay under 400 characters (sources and statements of LED-CPU-0012, -0017, LED-IRQ-0008, LED-PROF-0002, -0003); no fact was dropped except two descriptive words ("reports" and "by earlier plans") in LED-CPU-0017 sources; LED-CPU-0016 disagreement text is unchanged.
6. **[Note] Manual URLs.** The mirror file names were not recorded in the planning files; the HuC6270 and HuC6260 names follow the pattern of the existing HuC6280 records and downloaded with the recorded sizes and hashes, which is what TOOLS.md now records.

## Known Stubs

None (data and documentation only).

## Threat Flags

None. No new endpoint, auth path or file access at a trust boundary.

## Notes for next plans

- Constants rows may now cite LED-VID-0001, -0002, -0004, -0007, -0010, -0011, -0014, -0015, -0016, -0017 and LED-IRQ-0010 (INV_CONST_LEDGER, F-47). LED-IRQ-0001 pin and LED-VID-0001 and -0003 pins are set by the plans that land vid_frame_lines, vid_active_window and vdc_irq (still PENDING:4).
- Promotion of an E1 candidate (ER-2) must rewrite its statement to the form "the DuoSpool table or behaviour equals the manual's stated value", add a real TEST pin that names the entry, and list the page in a FACT source; the cites rows it needs exist.
- `scripts/check` selftest `mutation-docs-claim-word` still lists four cases; adding claim-word-video to its pair list is optional and was left out of this plan's file set.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED

- FOUND: fixtures/lib/cites.txt (28 H7 and H6 rows), fixtures/lib/TOOLS.md, tests/gate/fixture-lint/ok-h7-cite, tests/gate/fixture-lint/h7-unregistered, tests/gate/fixture/ok-h7-verdict-source, tests/docs/claim-word-video, ledger/entries/LED-VID-0004..0018, LED-IRQ-0009, LED-IRQ-0010, ledger/REQUIRED (63 LED lines).
- FOUND commits: 1f53bf9, a787e76, 543bbd1 (each an ancestor of HEAD).
