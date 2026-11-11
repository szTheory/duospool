---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
reviewed_file: 01-REVIEW.md
recorded: 2026-11-09
counts: {critical: 3, warning: 12, info: 4}
fixed: 5
open: 14
---

# Phase 1 review disposition

One row per finding in `01-REVIEW.md`. `fixed` rows point to the commit in `01-REVIEW-FIX.md`. `open` rows are not forgotten: they are advisory and are carried to Phase 2, where the gate is extended into the full fast gate (CI-01) and the same checkers are ported to the chosen language.

| ID | Severity | Status | Note |
|----|----------|--------|------|
| CR-01 | Critical | fixed | 4c4d6c6: BAD_FILENAME rejects TAB/newline names; `filename-controls` stage |
| CR-02 | Critical | fixed | 20fdc23: audit-history exits 2 on a shallow clone |
| CR-03 | Critical | fixed | eb97a86: protocol-order-check exits 2 on git failures |
| WR-01 | Warning | open | provenance-check accepts `upstream-ref: main`; tighten to full SHA or tag before the first copied file (register has zero entries today) |
| WR-02 | Warning | open | `license-spdx` expression structure not validated; allowlist comparison is by token. Same trigger as WR-01 |
| WR-03 | Warning | open | marker scan prunes `build`/`target`/`out`/`prefix` at any depth. Phase 2 |
| WR-04 | Warning | open | marker grammar misses `///`, `//!`, `--` leaders. Phase 2, when the C core adds `//` and `/* */` leaders |
| WR-05 | Warning | fixed | de78d13: empty --expect / EXPECT file now HARNESS_EMPTY_EXPECT |
| WR-06 | Warning | fixed | ae61f30: empty REQUIRED now REQUIRED_EMPTY |
| WR-07 | Warning | open | controls found by glob; a deleted control is invisible. Phase 2 adds a control inventory to the fail-closed gate (FIX-04 / CI-01) |
| WR-08 | Warning | open | selftest mutation (e) does not exercise `stage_score_controls`. Phase 2 |
| WR-09 | Warning | open | an audit-history TEXT-PATTERN allowlist entry suppresses all secret kinds for that path. Today the allowlist has no TEXT-PATTERN entries |
| WR-10 | Warning | open | audit-history does not scan commit/tag messages or author metadata. A one-off scan before publication was clean; add to the script before the next publication |
| WR-11 | Warning | open | scorer is pre-registered by add-commit order only. Low risk: the spike is complete and the decision recorded |
| WR-12 | Warning | open | `stage_results` does not re-run the scorer over the record block; the `amendment` flag is hand-typed. Low risk now (record 0001 is owner-recorded) |
| IN-01 | Info | open | audit-history assumes 40-char SHA-1 object names |
| IN-02 | Info | open | licence-check requires `shasum` with no fallback |
| IN-03 | Info | open | tests/audit/synthetic.sh is not wired into scripts/check |
| IN-04 | Info | open | check-results does not check the Evidence column or duplicate rows |
