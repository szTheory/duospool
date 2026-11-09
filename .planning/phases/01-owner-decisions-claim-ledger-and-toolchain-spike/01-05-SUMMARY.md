---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 05
subsystem: testing
tags: [pre-registration, protocol, posix-sh, awk, decision-rule, negative-controls, git-ancestry]
status: complete

requires:
  - phase: 01-03
    provides: "decision-check, which excludes *.protocol.md from record rules (D-09)"
  - phase: 01-04
    provides: "licence-check (SPDX headers under scripts/), .gitignore for spikes/tool-01/out/, scripts/check negative-control pattern"
provides:
  - "docs/decisions/0001-language-build.protocol.md: the pre-registered TOOL-01 spike protocol (W-01..W-12, G1..G5, M01..M15, D-13 weights, rubric anchors, arithmetic, D-37 rule)"
  - "scripts/protocol-order-check: protocol-before-spike and scorer-before-spike ancestry, append-only edits, dated amendments, record 0001 citation"
  - "scripts/lib/score.awk: the D-07 rule as integer-hundredths arithmetic with D-37 worst-case ND sensitivity"
  - "tests/protocol/synthetic.sh (13 throwaway-repository controls) and tests/score/ (17 controls)"
  - "scripts/check stages protocol-order, protocol-negatives, score-controls, plus a fifth selftest mutation"
affects: [01-06, 01-07, 01-08, 01-09, 01-10, 01-11, 01-12, phase-02]

tech-stack:
  added: []
  patterns:
    - "A pre-registered file is held by git history: its add commit must be a strict ancestor of the first artifact it governs, and only appends are allowed afterwards"
    - "Decision rule as a scorer with boundary controls on both sides of the threshold (10.00 pre-authorizes, 9.90 does not)"
    - "Score control EXPECT files list output lines that must appear verbatim, with EXIT=n and CODE=X forms for input errors"

key-files:
  created:
    - docs/decisions/0001-language-build.protocol.md
    - scripts/protocol-order-check
    - scripts/lib/score.awk
    - tests/protocol/synthetic.sh
    - tests/score/ (17 case directories, each with scores.txt and EXPECT)
  modified:
    - scripts/check

key-decisions:
  - "PROTOCOL_EDITED is an append-only rule: the add commit's bytes must be an exact prefix of the current file. This replaces 'pre-amendment lines equal the add commit' because appending an amendment normally adds a separator blank line, which would have turned every legitimate amendment into an edit"
  - "Spike ordering is checked against HEAD's history and against every spike-adding commit, not against main and not only the first one"
  - "A gate recorded as ND is not a PASS, so a leader with an ND gate is leader-fails-gate"
  - "The scorer-ordering check applies only once a spikes/tool-01 file exists; a scorer that is missing at that point counts as SCORER_AFTER_SPIKE"

patterns-established:
  - "Every ordering or edit rule has a synthetic-repository control that fails without --expect and passes with it"

requirements-completed: []  # TOOL-01 is advanced here (pre-registration, scorer); it completes with the language decision record

actuals:
  tokens: 15700   # chars/4 over the realized git diff of the three code commits (62741 chars); the SUMMARY and state commit are not included
  tasks: 2
  commits: 3
plan_head_before: a36e70e99b3d8651dccba50dbbdba2891d9e7384
plan_head_after: a1c7bf1c46a9c124197c7a0ba0f1131ba9c5eecc

coverage:
  - id: D1
    description: "The protocol is committed before any file under spikes/ exists in history, and holds W-01..W-12, G1..G5, M01..M15 (26 register rows, debug and release for M02-M07), the D-13 weights table, anchors, arithmetic, D-36 toolchain rule, D-37 rule, the no-extrapolation sentence and the resume rule"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/check stage protocol-order; git log --all --format=%H -- spikes prints nothing; the plan's grep acceptance checks"
        status: pass
    human_judgment: false
  - id: D2
    description: "protocol-order-check fails PROTOCOL_AFTER_SPIKE, PROTOCOL_EDITED, PROTOCOL_AMENDMENT_UNDATED, PROTOCOL_MISSING, PROTOCOL_CITE_MISMATCH and SCORER_AFTER_SPIKE on synthetic repositories, and passes ok-order, ok-amended and ok-cite"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh tests/protocol/synthetic.sh (13 SYNTH lines, all PASS); sh scripts/check stage protocol-negatives"
        status: pass
    human_judgment: false
  - id: D3
    description: "score.awk returns owner-recorded-preauth at exactly 10.00 points (nominal and worst case) and proposed-default/margin-below-10 at 9.90, plus tie, leader-fails-gate, both-fail-gates, nd-sensitivity and amended-after-spike, in exact integer arithmetic"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/check stage score-controls (17 cases); sh scripts/check --selftest mutation-margin-rule"
        status: pass
    human_judgment: false
  - id: D4
    description: "The D-13 weights and rubric anchors match the owner's priorities"
    requirement: TOOL-01
    human_judgment: true
    rationale: "Claude-proposed under D-13; only the owner can confirm the priorities. The owner may amend them before the spike runs, and no blocking checkpoint was added"
---

# Phase 1 Plan 05: Pre-registered spike protocol, order check and executable D-07 scorer Summary

**The TOOL-01 spike protocol (workload, gates, 26-row metric register, weights, rubric anchors, D-37 worst-case >=10 rule) is committed before any spikes/ file exists, with a protocol-order-check that enforces protocol-before-spike, scorer-before-spike and append-only edits forever, and an integer-hundredths scorer pinned at 10.00 versus 9.90 by 17 controls.**

## Performance

- **Duration:** about 30 min (start time was not captured; estimate)
- **Completed:** 2026-11-09
- **Tasks:** 2 (1 tracer, 1 tdd), three code commits
- **Files:** 39 changed (1325 insertions)

## Accomplishments

- The protocol is a single commit (`aea02af`) containing nothing else, so the D-09 ancestry has a clean anchor. It states the D-36 toolchain rule verbatim, cites D-37 as the owner-confirmed D-07 reading, and records the M01 cold-first-build and toolchain-install rows separately from the M02 warm clean build.
- Tracer gate: after Task 1 the whole gate was re-run end to end (`CHECK protocol-order PASS`, `CHECK protocol-negatives PASS`, `CHECK RESULT PASS`) before the scorer was built.
- protocol-order-check prints `PROTOCOL add=aea02af6e6d17bc1933fbb3467e38eb063f9a031 first_spike=none amendments=0 amended_after_spike=no` on the real repository. Thirteen synthetic cases exercise it; every must-fail case is also run without `--expect` and must exit 1.
- score.awk: scores are split on "." into tenths, points are weight x score10 x 2, the maximum is 10000, nothing is rounded. Controls include a leader whose worst case lands exactly on 10.00 (pre-authorized) and one that lands at 9.80 (nd-sensitivity), and an ND on the other option, which is scored 5.0 in the worst case.
- The gate has 14 stages and ends `CHECK RESULT PASS`; `--selftest` ends `SELFTEST RESULT PASS` with a new mutation that removes the margin-below-10 rule.

## Task Commits

1. **Task 1a: the protocol alone** - `aea02af` (docs)
2. **Task 1b: protocol-order-check, synthetic controls, gate stages** - `f30a806` (feat)
3. **Task 2: scorer, 17 controls, SCORER_AFTER_SPIKE** - `a1c7bf1` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] PROTOCOL_EDITED rule would have rejected every real amendment**
- **Found during:** Task 1 (synthetic controls `ok-amended` and `undated-amendment` failed on first run)
- **Issue:** The plan compares the current file's lines before the first amendment heading with the add commit's text. Appending an amendment normally adds a blank separator line before the heading, which lands inside the compared region, so a legitimate amendment read as an in-place edit.
- **Fix:** The add commit's bytes must be an exact byte prefix of the current file. That is the same append-only rule without the separator problem, and it also catches an edit hidden behind a planted or appended amendment heading (new control `edited-under-amendment`).
- **Files modified:** scripts/protocol-order-check, tests/protocol/synthetic.sh
- **Commit:** f30a806

**2. [Rule 2 - Missing critical] Ancestry checked against every spike-adding commit, with rename detection off**
- **Found during:** Task 1
- **Issue:** The plan checks the first spike commit only. With a merge, a later spike-adding commit on a branch that does not descend from the protocol would pass. A file renamed into spikes/ would also not show as an add.
- **Fix:** Every commit that adds a file under spikes/tool-01 must descend strictly from the protocol (and scorer) add commit. `--no-renames` is used on all history queries. The ancestry base is HEAD, as the orchestrator instructed, not `main`.
- **Commit:** f30a806, a1c7bf1

**3. [Structure] Task 1 split into two commits**
- The protocol went in on its own (`aea02af`) so the scripts that enforce the order could be run against a real add commit before the second commit. The plan's verification (`sh scripts/check` with `protocol-order` passing) needs the protocol committed first.

**4. [Rule 2 - Missing critical] Larger control set than the plan's minimum**
- Added protocol controls `edited-under-amendment`, `ok-cite`, `scorer-same-commit`, `scorer-missing` (13 total), and 8 score controls beyond the nine required: `nd-worst-exactly-10`, `nd-worst-below-10`, `other-nd-sensitivity`, `gate-nd-leader`, `max-spread`, `rust-leads`, `bad-value`, `bad-decimal` (17 total).
- The score-controls stage requires every line of a case's EXPECT to appear verbatim in the scorer output (so TOTAL, WORST and MARGIN are pinned, not only RESULT and REASON), with `EXIT=n` and `CODE=X` forms for rejected input. This is stricter than the plan's RESULT and REASON comparison.
- A fifth selftest mutation (`mutation-margin-rule`) proves the 9.90 control is alive.

### Judgment calls inside the plan's latitude

- A gate recorded as `ND` is treated as not PASS (the plan lists ND as a gate value without saying how it scores). A leader with an ND gate gets leader-fails-gate; both options with a non-PASS gate gets both-fail-gates.
- On a tie, WORST equals TOTAL and both margins print 0, with `LEADER none`.
- The scorer ordering check applies once a spikes/ file exists. Before that, a missing scorer is not an error, which keeps Task 1's gate green before Task 2 exists.
- The protocol says it is committed "before any spikes/ file exists in history" rather than "on main", because this repository's history lives on the phase branch and main is fast-forwarded later (01-12).

## Known Stubs

None.

## Threat Flags

None. The plan introduces no network endpoints, auth paths or file access beyond mktemp directories used by the synthetic controls (T-01-15: explicit `-c` identity, signing off, system and global git config disabled, real repository never touched).

## Issues Encountered

- awk on this machine is BWK awk 20200816 only; the scorer is written to POSIX (no interval expressions, no gawk extensions) but was not run under mawk or gawk, neither being installed.

## Next Phase Readiness

- Plan 01-06 may now create `spikes/tool-01/`. The gate will turn red with PROTOCOL_AFTER_SPIKE or SCORER_AFTER_SPIKE if either file is ever re-added later or rewritten out of order.
- Any change to the weights, anchors, metrics or rule from here on must be a dated `## Amendment YYYY-MM-DD: <reason>` section appended to the protocol; one dated after the first spike commit makes record 0001 proposed-default.
- The D-13 weights and anchors are still Claude's proposal. The owner can amend them before the spike runs.

## Self-Check: PASSED

- FOUND: docs/decisions/0001-language-build.protocol.md, scripts/protocol-order-check, scripts/lib/score.awk, tests/protocol/synthetic.sh, tests/score/ (17 cases)
- FOUND commits: aea02af, f30a806, a1c7bf1 (ancestors of HEAD)
- `git log --all --format=%H -- spikes` prints nothing; `sh scripts/check` ends `CHECK RESULT PASS`
