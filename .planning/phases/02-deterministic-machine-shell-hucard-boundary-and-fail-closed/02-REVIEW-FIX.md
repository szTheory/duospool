---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
fixed_at: 2026-11-10T00:00:00Z
review_path: .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-REVIEW.md
iteration: 1
findings_in_scope: 5
fixed: 5
skipped: 0
deferred: 0
status: all_fixed
---

# Phase 2: Code Review Fix Report

**Fixed at:** 2026-11-10
**Source review:** `.planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-REVIEW.md`
**Iteration:** 1
**Scope:** CR-01, WR-01, WR-02, WR-03, WR-04. The Info findings (IN-01 to IN-03) were out of scope and remain open in REVIEW.md.

| Finding | Status | Commit | Control that proves it |
|---------|--------|--------|------------------------|
| CR-01 awk piped into `sort` hides the awk status | fixed | `ace9ed5` | `sh scripts/check --selftest awk-exit-status` (9 cases). Each runs one gate script with an awk shim that exits 2 on exactly one program (a failing hash tool for rights-check) and requires exit 2 with no OK line. All 9 cases are green on the new scripts and FAIL on the old ones. |
| WR-01 rights-check excuse with no or malformed sha256, dash-named files | fixed | `95a4183` | `tests/rights/synthetic.sh` (gate stage `rights-negatives`): `listed-no-sha256`, `listed-malformed-sha256`, `dash-source-no-sha256`, `dash-name-magic`. All four FAIL against the previous script. `dash-source-no-sha256` exited 0 under it (the vacuous pass). |
| WR-02 `ds_boot` block overlapping request, content or result | fixed (existing refusal reused) | `c59c3c1` | New conformance kind `mem_overlap` and cases `ref_mem_overlap_content`, `_request`, `_result`, `_edge`. All four fail against the previous `boot.c`. |
| WR-03 CB_FLOAT blind to float constants and sized types | fixed | `3fb461f` | `tests/gate/core-boundary` (stage `core-boundary-negatives`): `float-literal-decimal`, `float-literal-exponent`, `float-literal-hex`, `float-type-sized`, `float-type-complex`, `float-type-decimal`, plus the positive `ok-int-literals`. The six negatives exit 1 (expectation not met) against the previous script. |
| WR-04 audit-history reads less than it reports, SHA-1 prefix | fixed | `ef50e50` | `tests/audit/synthetic.sh` (stage `audit-synthetic`): `sha256-home-path`, `classification-dropped`. Both FAIL against the previous script. |

## Fixed Issues

### CR-01: awk status hidden by `| sort`

**Files modified:** `scripts/outcome-sync`, `scripts/core-boundary-check`, `scripts/rights-check`, `scripts/audit-history`, `scripts/check`
**Commit:** `ace9ed5`
**Applied fix:** Every site that ended `awk ... | sort -u` (or `tool | awk`) now writes to a file, tests its own status as a harness error (exit 2), and only then sorts the file.
- `outcome-sync`: the row check and the compiled-list comparison.
- `core-boundary-check`: the allowlist, the undefined-symbol list, and both writable-symbol branches.
- `rights-check`: `sha256_of` no longer pipes, and its caller exits 2 with `HARNESS_SHA256_FAILED`.
- `audit-history`: the blob hash list. Two neighbouring awk calls that wrote the blob/path pairs with no status test also got one.

The first awk of `outcome-sync` was already covered by `set -e`. I tested it and it already exited 2, so it was left alone and has no case.

**Control detail:** `scripts/check --selftest awk-exit-status` puts a shim `awk` first on PATH. The shim execs the real awk unless an argument contains a marker for one program, in which case it exits 2. For each site it runs a benign pass (the OK line and exit 0, so the case is alive), then a forced pass (exit 2 and no OK line). The sites are `outcome-sync-row-check`, `outcome-sync-compiled-list`, `core-boundary-allowlist`, `core-boundary-undefined`, `core-boundary-writable`, `audit-history-pairs`, `audit-history-tree-paths`, `audit-history-hashes` and `rights-check-sha256`.

### WR-01: rights-check hash binding

**Files modified:** `scripts/rights-check`, `tests/rights/synthetic.sh`
**Commit:** `95a4183`
**Applied fix:**
- The manifest `sha256` must be 64 lower-case hex digits, else `RIGHTS_BAD_ALLOW`.
- A source starting with `-` is refused as an unsafe path.
- `sha256_of` and the MAGIC read in `inspect` pass `./FILE`.

The review's fix sketch allowed upper-case hex. I required lower-case because the tools print lower-case and every committed manifest uses it.

### WR-02: ds_boot overlap

**Files modified:** `src/core/boot.c`, `src/core/ds_core.h`, `tests/conformance/refkinds.c`, `tests/conformance/runner.c`, `tests/conformance/runner.h`, `tests/conformance/REQUIRED`, four new `tests/conformance/cases/ref_mem_overlap_*.toml`
**Commit:** `c59c3c1`
**Applied fix:**
- Finding reverified: the memset ran before the reads of `request` and `content`, and `finish` wrote `result` last.
- The existing `load.memory_invalid` fits, so there is no new enum member (22 members unchanged).
- The overlap test is one extra check after the last row of `boot_order.def`, with no new table row. No input that any row refuses changes outcome, and the precedence is untouched. Only an input that used to boot with undefined behaviour now refuses.
- Ranges are compared by address difference, so no sum can wrap.
- The core stays free of allocator, globals and floats; the core-boundary and sanitizer stages are green.
- The ds_core.h contract comment now states the rule.

REQUIRED goes from 100 to 104 ids, with 104 case files. Cases `content`, `request` and `result` place that object inside the instance block. Case `edge` shows that a content touching the block at either end boots, and one byte inside refuses. Every refusal leaves the block byte-identical (outside the result struct itself when the result is the overlapped object).

**Status:** fixed, but it is a behaviour change, so a human should confirm that the "overlap check runs last" placement is wanted. The alternative is a row in `boot_order.def` (see Notes).

### WR-03: CB_FLOAT coverage

**Files modified:** `scripts/core-boundary-check`, seven new dirs under `tests/gate/core-boundary/`
**Commit:** `3fb461f`
**Applied fix:**
- The type-name rule now also matches `_FloatN[x]`, `_DecimalN[x]`, `__fp16`, `__bf16`, `__float128`, `_Complex` and `__complex__`.
- The scanner walks pp-numbers on the comment- and literal-stripped line. It reports a decimal constant with a point or an `e` exponent, and a hex constant with a point or a `p` exponent (so `0x1e3` is an integer).
- Only POSIX awk is used (no interval expressions).
- Controls that need a type name the compiler may not accept put it in an `#if 0` block, because the scanner is lexical.

**Status:** fixed: requires human verification (it is a rule-logic change). The real core still passes; the positive `ok-int-literals` shows the main false-positive shapes do not fire. A GNU case range such as `1...5` would be flagged, which fails closed.

### WR-04: audit-history coverage and prefix

**Files modified:** `scripts/audit-history`, `tests/audit/synthetic.sh`
**Commit:** `ef50e50`
**Applied fix:**
- (a) In full mode, every blob hash that `git ls-tree -r` listed must be in the facts or the unreadable list, else exit 2 with the number of unread blob paths. I did not use the review's literal `nblobs > 0 && npaths > 0` guard. A range with only deletions, or a history with only empty trees, legitimately has 0 blobs, and the literal guard would fail a clean PR. The coverage check catches the real hole, a classification that drops blobs. Range mode is exempt by design, since it reads only new blobs.
- (b) The `git grep` prefix is cut by the length of the commit name the loop passed in (40 or 64 digits), and a line that does not start with it is a harness error instead of being dropped.

The full-history audit of this repository still ends `audit-history: clean` with the new coverage check.

## Verification (where it ran)

Everything ran in the main checkout, on the current branch, with no worktree: `sh scripts/check` and `sh scripts/check --selftest`. Neither was run in an isolated worktree, so the numbers below are reproducible from this tree.

- `sh scripts/check`: `CHECK RESULT PASS` (47 stages, run after all five fixes were in the tree, immediately before the WR-02 commit).
- `sh scripts/check --selftest`: `SELFTEST RESULT PASS`, including the 9 `awk-exit-status` cases.
- Awk coverage: only the macOS one-true-awk (`awk version 20200816`) exists on this machine. `mawk` and `gawk` are NOT installed, so there is no mawk or gawk evidence from this run. The new awk code avoids interval expressions and gawk-only functions. The CI gate, which runs both, is where that will be shown.
- Not in scope: the necessity pass (#4100) was not performed by the reviewer, so no necessity finding was fixed or skipped.

## Notes

- `.planning/state.json` was untracked before this run and was left alone.
- `awk-exit-status` is a selftest (run with `sh scripts/check --selftest awk-exit-status`), not a gate stage, so the 47-stage count is unchanged. The WR-01, WR-03 and WR-04 controls are gate stages and run on every `sh scripts/check`.
- Decision records D-40 to D-86, ROADMAP, STATE and requirement checkboxes were not touched.
- No item was deferred, so `deferred-items.md` was not edited. If the owner prefers WR-02 as a row in `boot_order.def` (so the table is the single list of checks), it needs a precedence-table item for the new adjacent pair and a decision on D-46; that is an owner or planner choice.

---

_Fixed: 2026-11-10_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
