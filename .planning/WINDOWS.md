---
schema_version: 1
open_count: 1
waived_count: 0
fixed_count: 1
total_count: 2
last_updated: 2026-11-10T05:07:24.411Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02 | unmet-truth | .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-SUMMARY.md | 181 | Full-history audit-history reports home-path-mac in the 02-01 and 02-02 summaries (history, pre-dates plan 02-04); owner decides before publication: rewrite unpublished history or allowlist with reason | open |  | 2026-11-10T04:21:23.027Z |  |
| 2 | 02 | stub | src/core/ds_core.h | 100 | ds_instance.content_id is a zeroed placeholder slot until plan 02-07 adds SHA-256 content identity (the plan 02-05 boot path does not compute it) | fixed |  | 2026-11-10T04:39:37.029Z | 2026-11-10T05:07:24.411Z |

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
  },
  {
    "id": 2,
    "kind": "stub",
    "phase": "02",
    "file": "src/core/ds_core.h",
    "line": 100,
    "description": "ds_instance.content_id is a zeroed placeholder slot until plan 02-07 adds SHA-256 content identity (the plan 02-05 boot path does not compute it)",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-10T04:39:37.029Z",
    "resolved_at": "2026-11-10T05:07:24.411Z",
    "milestone": null
  }
]
````
