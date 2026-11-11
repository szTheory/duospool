---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
fixed_at: 2026-11-09
review_path: .planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-REVIEW.md
iteration: 1
findings_in_scope: 5
fixed: 5
skipped: 0
status: all_fixed
---

# Phase 1: Code Review Fix Report

**Source review:** `.planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-REVIEW.md`
**Scope:** CR-01, CR-02, CR-03, WR-05, WR-06 only. All other warnings and infos stay open.
**Verification ran in:** the main checkout (no worktree), `/bin/sh` and `/bin/dash` (the gate and the synthetic controls).

Every finding was reproduced first, fixed, and the reproduction re-run to show it now fails closed. Every new control was
also run against the pre-fix script and failed there (the control is live, not vacuous).

| Finding | Status | Commit | Evidence |
|---------|--------|--------|----------|
| CR-01 | fixed | 4c4d6c6 | Before: `ledger-check` on a dir holding `x<TAB>F<TAB>y<TAB>z.toml` (content `garbage line`) printed only `STATUS ...`, exit 0; `profile-check` on `r1<TAB>P<TAB>x.toml` printed `RECORDS profiles=19`, exit 0. After: both print `<bad-filename> BAD_FILENAME` and exit 1. A path with TAB or newline (file, directory component, `--required` file) never reaches the record stream. New stage `filename-controls` in `scripts/check` creates the names at run time (7 cases, EXPECT = `BAD_FILENAME`, and exit 1 without `--expect`); on the old scripts it reports CONTROL BROKEN for all 7. |
| CR-02 | fixed | 20fdc23 | Before: `audit-history` on a `--depth 1` clone of a two-commit repo printed `scanned 1 commits ... audit-history: clean`, exit 0. After: `audit-history: shallow or unreadable repository (true): the audit needs full history`, exit 2 (anything other than `false` from `git rev-parse --is-shallow-repository` is exit 2). New control `shallow-clone` in `tests/audit/synthetic.sh` (also checks the source repo audits clean and the clone really is shallow); FAILs on the old script. |
| CR-03 | fixed | eb97a86 | Before: synthetic repo with a spike commit before the protocol commit reports `PROTOCOL_AFTER_SPIKE` (exit 1); after deleting the loose `spikes` tree object it printed `first_spike=none`, `RECORDS 1`, exit 0. After: `HARNESS_GIT_FAILED git log -- spikes/tool-01`, exit 2. `|| true` and `2>/dev/null` masking removed from `add_commit`, the spikes enumeration and the amendment `-S` lookup; `merge-base --is-ancestor` now separates status 1 (not an ancestor) from any other status (exit 2); `git show` failure exits 2 instead of 128; an unborn HEAD is still the legitimate "no protocol" state. New controls `git-failure-spikes` and `git-failure-protocol` in `tests/protocol/synthetic.sh` (precondition: intact repo exits 1; then expects exit 2, `HARNESS_GIT_FAILED`, no `RECORDS`/`EXPECT_OK`); both FAIL on the old script. |
| WR-05 | fixed | de78d13 | Before: `ledger-check --expect "" ledger/entries/` printed `EXPECT_OK`, exit 0. After: `HARNESS_EMPTY_EXPECT --expect names no code`, exit 2, in all seven checkers (empty, blank and comma-only values). `scripts/check` gains `read_expect`: an empty, blank or comma-only EXPECT file is `HARNESS_EMPTY_EXPECT` (stage ERROR, exit 2) in every negative stage and in the score controls. Demonstrated by emptying `ledger/negative/e0-e4/EXPECT` and `tests/score/tie/EXPECT` (restored): both stages went to ERROR. New stage `empty-expect-controls` pins the checkers and the helper. |
| WR-06 | fixed | ae61f30 | Before: `ledger-check --required <empty file> ledger/entries/` and a comment-only file both exited 0. After: `REQUIRED_EMPTY 0 no requirement lines`, exit 1. Emptying the real `ledger/REQUIRED` now turns `ledger-entries` red (restored). New committed controls `ledger/negative/required-empty` (zero bytes) and `ledger/negative/required-comments-only`, EXPECT = `REQUIRED_EMPTY`; both FAIL on the old script. `ledger/SCHEMA.md` lists the new codes. |

## Gate results at the last commit

- `sh scripts/check`: `CHECK RESULT PASS` (18 stages, including the two new ones)
- `dash scripts/check`: `CHECK RESULT PASS`; `sh scripts/check --selftest`: `SELFTEST RESULT PASS`
- `sh tests/audit/synthetic.sh`: 11 of 11 PASS (also under dash)
- `sh tests/protocol/synthetic.sh`: 15 of 15 PASS (also under dash)
- `sh scripts/audit-history .`: `audit-history: clean`
- Root pin guard run before the first edit and before every commit: exit 0

## Notes

- The new `filename-controls` stage exits 2 (harness error) if the filesystem cannot create a TAB-named file, rather than
  skipping silently.
- `decision-check` and `provenance-check` were not changed for CR-01, per the review (they fail closed on a tab-named file);
  they did receive the shared WR-05 `--expect` guard.
- Not addressed (still open, dispositions are the orchestrator's): WR-01..WR-04, WR-07..WR-12, IN-01..IN-04.

---

_Fixed: 2026-11-09_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
