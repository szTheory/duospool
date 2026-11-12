---
schema_version: 1
open_count: 2
waived_count: 1
fixed_count: 2
total_count: 5
last_updated: 2026-11-12T07:56:33.239Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02 | unmet-truth | .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-SUMMARY.md | 181 | Full-history audit-history reports home-path-mac in the 02-01 and 02-02 summaries (history, pre-dates plan 02-04); owner decides before publication: rewrite unpublished history or allowlist with reason | waived | resolved-by-allowlist: owner approved (2026-11-10, plan 02-22) a narrow TEXT-PATTERN:home-path-mac allowance for the 02-01 and 02-02 summaries in scripts/audit-history.allow; local home-directory path, no credential; rewriting unpublished history rejected because the summaries record commit hashes | 2026-11-10T04:21:23.027Z | 2026-11-11T01:24:53.101Z |
| 2 | 02 | stub | src/core/ds_core.h | 100 | ds_instance.content_id is a zeroed placeholder slot until plan 02-07 adds SHA-256 content identity (the plan 02-05 boot path does not compute it) | fixed |  | 2026-11-10T04:39:37.029Z | 2026-11-10T05:07:24.411Z |
| 3 | 2 | fixme | scripts/lib/record-rules.awk | 45 | record-lint fails under mawk: kseq[t] = (t in kseq) ? ... evaluates the lvalue first (found by the 02-19 Linux container run; fix in deferred-items.md D-19-1) | fixed |  | 2026-11-10T08:13:01.941Z | 2026-11-10T08:29:33.180Z |
| 4 | 04 | stub | src/core/beam.c | 16 | ds_vdc_advance has an inert body (no VDC or VCE state exists yet); plan 04-03 gives it the grid and the frame wrap, 04-06 the line ladder, and ds_vdc.h declares entry points defined by 04-03 and 04-04 | open |  | 2026-11-12T04:20:14.323Z |  |
| 5 | 4 | stub | src/core/vdc.c |  | LENR and DVSSR MSB writes are stored only: no VRAM-VRAM DMA and no SATB transfer start until plan 04-10 (plan 04-06 prohibition); ds_vdc_flag has no caller until plan 04-08, so no status flag is ever raised by an event | open |  | 2026-11-12T07:56:33.239Z |  |

````json
[
  {
    "id": 1,
    "kind": "unmet-truth",
    "phase": "02",
    "file": ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-SUMMARY.md",
    "line": 181,
    "description": "Full-history audit-history reports home-path-mac in the 02-01 and 02-02 summaries (history, pre-dates plan 02-04); owner decides before publication: rewrite unpublished history or allowlist with reason",
    "status": "waived",
    "reason": "resolved-by-allowlist: owner approved (2026-11-10, plan 02-22) a narrow TEXT-PATTERN:home-path-mac allowance for the 02-01 and 02-02 summaries in scripts/audit-history.allow; local home-directory path, no credential; rewriting unpublished history rejected because the summaries record commit hashes",
    "recorded_at": "2026-11-10T04:21:23.027Z",
    "resolved_at": "2026-11-11T01:24:53.101Z",
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
  },
  {
    "id": 3,
    "kind": "fixme",
    "phase": "2",
    "file": "scripts/lib/record-rules.awk",
    "line": 45,
    "description": "record-lint fails under mawk: kseq[t] = (t in kseq) ? ... evaluates the lvalue first (found by the 02-19 Linux container run; fix in deferred-items.md D-19-1)",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-10T08:13:01.941Z",
    "resolved_at": "2026-11-10T08:29:33.180Z",
    "milestone": null
  },
  {
    "id": 4,
    "kind": "stub",
    "phase": "04",
    "file": "src/core/beam.c",
    "line": 16,
    "description": "ds_vdc_advance has an inert body (no VDC or VCE state exists yet); plan 04-03 gives it the grid and the frame wrap, 04-06 the line ladder, and ds_vdc.h declares entry points defined by 04-03 and 04-04",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-12T04:20:14.323Z",
    "resolved_at": null,
    "milestone": null
  },
  {
    "id": 5,
    "kind": "stub",
    "phase": "4",
    "file": "src/core/vdc.c",
    "line": null,
    "description": "LENR and DVSSR MSB writes are stored only: no VRAM-VRAM DMA and no SATB transfer start until plan 04-10 (plan 04-06 prohibition); ds_vdc_flag has no caller until plan 04-08, so no status flag is ever raised by an event",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-12T07:56:33.239Z",
    "resolved_at": null,
    "milestone": null
  }
]
````
