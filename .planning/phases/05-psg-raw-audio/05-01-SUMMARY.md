---
phase: 05-psg-raw-audio
plan: 01
subsystem: evidence-data-ledger-and-process-scaffolding
tags: [aud, psg, er-1, cites, p8-labels, fixture-lint, ledger, docs-check, handbook, ci-lever-a, baseline, w1]
status: complete

requires:
  - phase: 04.1-vdc-sprites
    provides: the Phase 4 and 4.1 gate, the cites.txt and fixture-lint label machinery (H7 and H6 of 04-01), the ledger schema and REQUIRED, docs-check withheld words, the three-job workflow and its ok-gate control copy
provides:
  - fixtures/lib/cites.txt - the P8 legend line, ten P8 img rows (P8-2 to P8-10, P8-12), an H8-23 img row, H8-18 and H8-24 re-read with the PSG facts, the OCR errata note (ER-1, D-213)
  - fixtures/lib/TOOLS.md - the PSG manual by URL, 934252 bytes, sha256 2f463fd3...6c04b, 14 PDF pages, label P8-n = PDF page n+2 (ER-9)
  - scripts/lib/fixture-lint-rules.awk islabel and scripts/lib/fixture-rules.awk manual-cite accept P8-n (F-74), with controls p8-label-no-img, p8-fact-no-img and ok-p8-label
  - ledger/entries LED-AUD-0003..0014 (new, assumed, PENDING:5) and amended LED-AUD-0001, -0002, LED-PROF-0002, -0003, LED-CPU-0013, -0017; ledger/REQUIRED 63 -> 75 lines
  - scripts/docs-check rejects band-limited, sounds correct, plays music and authentic sound; README third withheld line; mini-root tests/docs/claim-word-audio (DOCS_CLAIM_WORD)
  - .planning/phases/05-psg-raw-audio/05-HANDBOOK.md (25,170 bytes) for every later Phase 5 plan
  - CI lever (a) as its own revertible commit 9619efe; the Phase 5 local gate baseline in docs/ci-timings.md (cut trigger 3)
affects: [05-02, 05-03, 05-04, 05-05, 05-08, 05-10]

requirements-completed: []
requirements-advanced: [AUD-01, AUD-02, AUD-03]

actuals:
  tokens: 16600
  tasks: 3
  commits: 5
plan_head_before: f6afedeba1cd3524fb6b78d732d3df4f7d62fe21
plan_head_after: d6ca356

tech-stack:
  added: []
  patterns:
    - "A page label rule is proven by a control whose only violation comes from that rule: islabel gates only the FACT sources of supported entries, so its control is a root with a supported entry citing an unregistered P8 page and a registered MAN class"
    - "Ledger notes that the schema has no key for (E1 candidate, ER-14 hold, constants carried) live in # comment lines of the entry"

key-files:
  created:
    - .planning/phases/05-psg-raw-audio/05-HANDBOOK.md
    - ledger/entries/LED-AUD-0003.toml (to LED-AUD-0014.toml, twelve files)
    - tests/gate/fixture-lint/p8-label-no-img/
    - tests/gate/fixture-lint/p8-fact-no-img/
    - tests/gate/fixture-lint/ok-p8-label/
    - tests/docs/claim-word-audio/
  modified:
    - fixtures/lib/cites.txt
    - fixtures/lib/TOOLS.md
    - scripts/lib/fixture-lint-rules.awk
    - scripts/lib/fixture-rules.awk
    - ledger/entries/LED-AUD-0001.toml, LED-AUD-0002.toml, LED-PROF-0002.toml, LED-PROF-0003.toml, LED-CPU-0013.toml, LED-CPU-0017.toml
    - ledger/REQUIRED
    - scripts/docs-check
    - README.md
    - .github/workflows/gate.yml
    - tests/workflow/ok-gate/.github/workflows/gate.yml
    - docs/ci-timings.md

key-decisions:
  - "05-01: the P8 islabel rule gets its own negative control p8-fact-no-img (FACT route), because removing the P8 alternative cannot turn a positive root red; the positive root is named ok-p8-label so run_controls treats it as must-pass"
  - "05-01: the ER-1 image read of P8-2..P8-10, P8-12, H8-18, H8-23 and H8-24 found no number that differs from the 05-RESEARCH table; the OCR errata (P8 prefix, P8-7 01000 = -8, P8-12 C1 R2 = 5D) are confirmed"
  - "05-01: local warm gate baseline for cut trigger (3): user CPU 270.0 s, wall 328.2 s at load 5.7 to 9.1 (tree 9619efe); trigger (3) fires above 337.5 s at a comparable load"

duration: about 45 min
completed: 2026-11-13
---

# Phase 5 Plan 01: Evidence data and process scaffolding Summary

**ER-1 image reads of the PSG manual recorded as ten P8 rows plus H8-23, the P8 label rule with a control that fails when the rule is deleted, twelve new and six amended ledger entries carrying DX-1..DX-16, the D-218 withheld words, the Phase 5 handbook, CI lever (a) as its own commit, and the local gate baseline 270.0 s user CPU.**

## Performance

- Duration: about 45 minutes (first task commit 01:10, last 01:25 local time, plus three gate runs)
- Tasks: 3 of 3
- Files changed: 45 (git diff f6afede..d6ca356)

## Accomplishments

- Task 1 (3b24c7d): both manuals downloaded into a scratch directory outside the repository, matched by size and SHA-256 (PSG 934252 bytes, 2f463fd3...6c04b; hardware 1196754 bytes, 40a32e5d...74fcb), rendered at 150 dpi and read as images: P8-2 to P8-10, P8-12, H8-18, H8-23, H8-24.
  cites.txt gained the P8 legend, ten P8 rows and an H8-23 row (verifier gsd-executor, 2026-11-13, short facts in own words); the H8-18 fact gained "no PSG entry", the H8-24 fact the PSG window, A3-A0 decode and no read function, both with this read's verifier and date. No manual text or image is committed.
- Task 2 (3d607b1): LED-AUD-0003..0014 per D-213 (all assumed, PENDING:5, revisit 5, origin 05-CONTEXT D-nnn equal to the REQUIRED line); DX-1 in -0001, DX-2 in -0002, DX-3 in -0010, DX-4 and DX-16 in -0011, DX-5 in -0008, DX-6 and DX-7 in -0009, DX-8, DX-9, DX-10, DX-13 in -0004, DX-10 and DX-11 in -0012, DX-12 in -0005, DX-14 in -0007, DX-15 in LED-CPU-0013;
  -0013 and -0014 say "none: project definition; no source is compared". LED-AUD-0011 follows D-219 and D-231 (statement plus a comment for the widx[1] rule); LED-AUD-0004 names the D-220 restart edges and constants and withholds the cross-binding restore of a d = 3 snapshot (D-223).
  ER-14 hold in comments on -0006 and -0011; E1 candidate comments on -0002, -0003, -0005, -0008 only (no E1 evidence row). Amendments keep origins and pins (F-83).
- Task 3 (2aa458e, 9619efe, d6ca356): the handbook (sections 1 to 12, every D-182..D-232, ER-14..ER-22, PR-1..PR-19); lever (a) in both tool-versions blocks with the ok-gate copy byte-identical; the baseline section in docs/ci-timings.md and the baseline row of the handbook budget table.

## Image-read findings (ER-1)

None of the numbers on the images differs from the 05-RESEARCH "Manual Page Image Verification" table. Confirmed on the images: the page prefix P8, P8-7 row 01000 = -8 and row 00000 = -10 (hex), P8-12 C1 R2 = 5D (F 3,421), A4 F 254 (R3 0, R2 FE) 440.45 Hz, A2 F 1,017 110.00 Hz, C8 F 27 4,143.5 Hz;
P8-9 prints the noise section as 2.3.3 inside 2.2 and P8-6 cross-refers to 2.3.3 and 2.3.2 (writer note 3); P8-10's text says "exceeds F" while its example sums to exactly 15 with no sound (DX-5); H8-24 lists the PSG at $1FE800-$1FEBFF, logical $0800-$080F with MPR0 $FF, (A3-A0), and no function column entry; H8-18 has no PSG item.

## P8 label rule proof (R4, scratch copy, not committed)

| Run (a scratch copy of scripts/ with `P8-[0-9]+` removed from islabel) | Result |
|---|---|
| ok-p8-label | still `FIXTURE_LINT ... STATUS fixture-lint ok` (a positive root cannot turn red by losing a check) |
| p8-label-no-img (MAN:P8-11 in .checks) | still EXPECT_OK: the MAN class route checks the registry directly and does not use islabel |
| p8-fact-no-img (supported entry with FACT P8-11) | `EXPECT_MISMATCH wanted 'FXL_UNKNOWN_CITE' got ''`, exit 1: the control goes NOT_RED, so fixture-lint-negatives turns red |
| ok-p8-label with its P8-9 row deleted (unmutated linter) | red: FXL_UNKNOWN_CITE on the MAN:P8-9 row and on the FACT P8-9 source |

docs-check proof (scratch copy with the four new alternations removed): claim-word-audio gives `EXPECT_MISMATCH wanted 'DOCS_CLAIM_WORD' got '<none>'`, exit 1; the real script gives EXPECT_OK.

## Gate results

| Run | Result | Wall s | User CPU s | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check --timing` at 9619efe's tree (before the lever commit; the recorded baseline) | PASS, 57 stages | 328.2 | 270.0 | 5.7 to 9.1 |
| `sh scripts/check` at d6ca356 (plan end) | PASS, 57 stages | 375.0 | 303.7 | about 9.5 to 9.7 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 175 PASS lines, no FAIL | 76.6 | 31.0 | about 9.1 to 9.4 |
| `sh scripts/audit-history --range f6afede..HEAD` | clean (5 commits, 39 blobs, 2,078 blob-path pairs) | | | |

No C file changed, so no Linux run (plan text and D-216). The two gate runs differ by 12 percent of user CPU on the same code at similar load: the run-to-run spread 05-08 must keep in mind when it judges cut trigger (3).
Sizes: none of the 45 changed files has a ROM-SIZE shape (4,096 bytes or more and a multiple of 8,192, or 8,192 + 512).

## Live budget table (after 05-01)

| Budget | Value after 05-01 | Phase 5 target |
|---|---|---|
| Gate stages | 57 | 58 |
| Selftest PASS lines | 175 | >= 178 |
| Power-on policies | 4 | 5 |
| Constants rows / CROWS_MAX | 34 / 40 | 49 / 64 |
| Withheld ids | 35 | 42 |
| Record rows (skeleton / 16 events / cap) | 153 / 169 / 192 | 175 / 191 / 192 |
| Regions; serial / epoch / text | 21; 8 / 5 / 7 | 24; 9 / 6 / 8 |
| Snapshot / instance / request bytes | 92,255 / 1,141,096 / 56 | 92,553 / 1,141,376 / 72 |
| OX_ROMS_MAX / guest ROMs | 40 / 36 | 48 / 39 |
| DSV ids | 1..26 | 27, 28, 29 |
| Toys | 26 | 33 |
| Ledger entries / REQUIRED lines | 75 / 75 (done) | 75 / 75 |
| Conformance REQUIRED cases | 334 | about 379 to 394 |
| .aexp files | 0 | cap 32,768 bytes each |
| CI critical path | 562 s (4.1 run; lever (a) adds no step time) | about 587 to 612 s predicted |
| Local warm gate user CPU | 270.0 s baseline (303.7 s second run) | +10 to +20 percent; trigger (3) above 337.5 s |
| Handbook bytes | 25,170 of 25,600 | cap 25,600 |

## Task Commits

1. Task 1: ER-1 image reads, P8 label rules and controls - `3b24c7d` (feat)
2. Task 2: ledger entries, REQUIRED, D-218 withheld words - `3d607b1` (feat)
3. Task 3: handbook `2aa458e` (docs), lever (a) `9619efe` (ci, revertible, full gate PASS before it), baseline `d6ca356` (docs)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The positive mini-root is named ok-p8-label, not p8-label-ok**
- Found during: Task 1. `run_controls --ok-prefix` treats only `ok-*` directories as must-pass; a `p8-label-ok` root without EXPECT would be HARNESS_NO_EXPECT.
- Fix: tests/gate/fixture-lint/ok-p8-label (MAN:P8-9 with a copy of the P8-9 row, plus a supported ledger entry with FACT P8-9). Commit 3b24c7d.

**2. [Rule 2 - R4 control] Third mini-root tests/gate/fixture-lint/p8-fact-no-img**
- Found during: Task 1. islabel is used only for the FACT sources of supported entries (factrefs); the MAN class route looks the label up in cites.txt directly. Removing the P8 alternative therefore cannot turn the positive root red (the plan's wording), and leaves p8-label-no-img red as well, so neither planned root proves the rule.
- Fix: a negative root whose only violation is a supported entry citing the unregistered P8-11 in a FACT source (EXPECT FXL_UNKNOWN_CITE); the scratch proof above shows it goes NOT_RED when the P8 alternative is deleted. Outside files_modified. Commit 3b24c7d.

**3. [Rule 3 - 400-character lines] Compressed statements in the amended entries**
- LED-PROF-0003's statement was 392 characters; the PSG clause fits only by shortening policy names (v1, v2, v4) and phrasing; LED-PROF-0002 likewise; LED-CPU-0013's disagreement was rewritten more briefly to add DX-15. Meanings are unchanged. Notes the schema has no key for (E1 candidate, ER-14 hold, constants) are # comments. Commit 3d607b1.

**4. Handbook size 25,170 bytes**, above the 15,000 to 20,000 target and under the 25,600 cap the verify enforces; it carries every id the verify requires. Later amendments (05-08, 05-10) have about 430 bytes of room and must compress elsewhere.

**5. Commit trailer**: the five commits end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, the model that wrote them, instead of the requested Sonnet 5.5 line; no script checks the trailer.

Requirement ticks: none happened in the plan commits; the plan-complete step's ticks of AUD-01..03 (if any) are reverted with a file-specific checkout (only 05-10 completes them).

## Known Stubs

None. The handbook's budget rows are targets, not stubs.

## Threat Flags

None: no new endpoint, parser, file access or trust boundary. Lever (a) adds two print commands (no `|| true`, no `${{ }}`), and workflow and workflow-negatives stay green.

## Notes for next plans

- 05-02: no file of this plan overlaps its files; nothing to merge.
- 05-03: every constants row has its ledger id (LED-AUD-0004 for psg_select_67_drop, psg_wave_write_when_on, psg_keyon_widx_step, psg_dda_release_restart, psg_trg_release_restart; -0007, -0008, -0010, -0012, -0014 for the rest), so INV_CONST_LEDGER passes; LED-AUD-0009 joins the fixed withheld list there (PR-8).
  LED-PROF-0002 and -0003 already state seeded-prng-v4 and the PSG power-on; if 05-03 takes the D-204 fallback (no v4), both statements need a correction amendment in that plan.
- 05-04 onward: MAN:P8-n classes and manual-cite:P8-n are accepted only for the ten registered pages (P8-1 and P8-11 have no row on purpose). The manual-cite P8 alternative has no own control yet; the first manifest that uses it (psg_tone) exercises it.
- 05-08: compare cut trigger (3) against 270.0 s user CPU at a comparable load and record the run-to-run spread (303.7 s on the same code at load 9.5); the CI trigger is recorded against the two-leg 562 s path, with about 587 to 612 s predicted.
- Handbook room is about 430 bytes.

## Owner items (hand-offs, no new gate)

- Lever (a), commit 9619efe: the owner's yes or no at publish time, before any push (D-215, D-226). A no is `git revert 9619efe`.
- The commit trailer names Claude Opus 5.5 instead of the Sonnet 5.5 line the brief asked for (deviation 5).

## Self-Check: PASSED

Files: fixtures/lib/cites.txt, fixtures/lib/TOOLS.md, the three fixture-lint mini-roots, twelve new ledger files, tests/docs/claim-word-audio, 05-HANDBOOK.md and the docs/ci-timings.md section exist. Commits 3b24c7d, 3d607b1, 2aa458e, 9619efe and d6ca356 are ancestors of HEAD.
