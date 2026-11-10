---
schema_version: 1
open_count: 1
waived_count: 0
fixed_count: 0
total_count: 1
last_updated: 2026-11-10T04:21:23.027Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02 | unmet-truth | .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-SUMMARY.md | 181 | Full-history audit-history reports home-path-mac in the 02-01 and 02-02 summaries (history, pre-dates plan 02-04); owner decides before publication: rewrite unpublished history or allowlist with reason | open |  | 2026-11-10T04:21:23.027Z |  |

````json
[
  {
    "id": 1,
    "kind": "unmet-truth",
    "phase": "02",
    "file": ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-SUMMARY.md",
    "line": 181,
    "description": "Full-history audit-history reports home-path-mac in the 02-01 and 02-02 summaries (history, pre-dates plan 02-04); owner decides before publication: rewrite unpublished history or allowlist with reason",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-10T04:21:23.027Z",
    "resolved_at": null,
    "milestone": null
  }
]
````
