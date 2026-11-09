---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 11
subsystem: decision-record
tags: [tool-01, language-decision, scoring, proposed-default, evidence-tag, results-gate]
status: complete

requires:
  - phase: 01-05
    provides: "pre-registered protocol (aea02af6) and the executable scorer scripts/lib/score.awk"
  - phase: 01-10
    provides: "results.tsv (26 rows per option), raw hyperfine JSON, gate outputs, check-results"
provides:
  - "docs/decisions/0001-language-build.md: TOOL-01 record, status proposed-default, every register row for both options, tools table, scores and scorer output, leader-sensitivity table"
  - "scripts/check stage results: check-results --record on the real record"
  - "annotated tag spike-tool-01-evidence (local) pointing at 4d175ff, preserving the whole spike tree including derive-scores.awk, scores.txt and score-output.txt"
  - "spikes/ removed from main (b956d9e); working tree in its final layout"
affects: [01-12, phase-2]

tech-stack:
  added: []
  patterns:
    - "Scores computed by an awk script from the protocol formulas (C2, C3, C4) and anchors for C1, C5, C6, with a reason per score and the amendment flag taken from protocol-order-check"

key-files:
  created:
    - docs/decisions/0001-language-build.md
    - spikes/tool-01/bench/derive-scores.awk (tag only)
    - spikes/tool-01/results/scores.txt (tag only)
    - spikes/tool-01/results/score-output.txt (tag only)
  modified:
    - scripts/check
    - docs/decisions/README.md
    - .planning/PROJECT.md
    - spikes/tool-01/LOG.md (tag only)

key-decisions:
  - "Record 0001 is proposed-default, not owner-recorded: score.awk printed RESULT=proposed-default, REASON=margin-below-10 (Rust 80.20, C 79.00, nominal margin 1.20 points; the D-07/D-37 rule needs 10.00). No owner statement was written. default-in-force names rust per the pre-registered score, provisional only. Phase 2 stays blocked and the owner is asked."
  - "C3 counted Rust T=4 and C T=3: wasmtime (M14 only, feeds C6) and Apple clang (system cc, protocol baseline) are outside T by the protocol's literal text."
  - "C5 rubric item (a) was not applied to C (fuzz compiles with the normal clang; the archive link is item (c)), and item (d) was not applied to Rust (M13 is measured on the final planted form, 0.4 s). The Rust first-planted-form observation is recorded as a cost, not a subtraction."

patterns-established:
  - "A 1.20-point lead is inside the effect of single rubric readings, so the record carries a sensitivity table instead of choosing silently"

requirements-completed: []  # TOOL-01 is not complete: the language decision is open (proposed-default) and needs an owner choice

duration: ~15 min
completed: 2026-11-09

actuals:
  tokens: 10734   # chars/4 over the realized diff (record 30372, derive-scores.awk 5392, scores.txt 3539, score-output 146, check/README/PROJECT diff 3486)
  tasks: 2
  commits: 2
plan_head_before: f8b6e8a5c03452a998c7dee7acfef33729342142
plan_head_after: b956d9e8784c6ab4a68a90c3cdeaff0e5f797aaa

coverage:
  - id: D1
    description: "Record 0001 has a result for every (metric, variant) and both options, and the gate enforces it"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/check-results docs/decisions/0001-language-build.protocol.md --record docs/decisions/0001-language-build.md exits 0, RECORDS 52; sh scripts/check ends CHECK RESULT PASS with CHECK results PASS"
        status: pass
    human_judgment: false
  - id: D2
    description: "Record status follows the scorer: proposed-default with blocks Phase 2, the SC1 sentence and the named provisional leader"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "RESULT=proposed-default in score-output.txt equals status: proposed-default; grep -qxF 'The language decision is open and Phase 2 is blocked.' passes; LEADER rust with default-in-force naming rust"
        status: pass
    human_judgment: true
  - id: D3
    description: "Record cites the protocol add commit, C written first, and the no-extrapolation sentence"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "Protocol commit aea02af6e6d17bc1933fbb3467e38eb063f9a031 equals protocol-order-check add=; protocol-order PASS; grep counts one each for the citation, 'written first: C' and the sentence"
        status: pass
    human_judgment: false
  - id: D4
    description: "Evidence tag exists, contains the spike tree, and main is clean after removal with the gate green"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "git rev-parse spike-tool-01-evidence^{tag} succeeds; the tag lists 67 spikes/tool-01 files including both skeleton sources, gates.sh, manifest, LOG.md, results and the score files; git ls-files spikes prints nothing; test ! -d spikes; sh scripts/check CHECK RESULT PASS; protocol-order-check still prints first_spike=1e3d304d..."
        status: pass
    human_judgment: false
  - id: D5
    description: "PROJECT.md pointer row and README index row carry the status word"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "grep -n '→ docs/decisions/0001-language-build.md' .planning/PROJECT.md; decision-records PASS (no INDEX_DRIFT)"
        status: pass
    human_judgment: false

self_check: PASSED
---

# Phase 1 Plan 11: Language and build record, scored and tagged Summary

**Both skeletons passed every hard gate, but the pre-registered rule did not pre-authorize a winner: Rust leads 80.20 to 79.00, so record 0001 is proposed-default, Phase 2 is blocked, and the owner is asked to choose Rust or C11.**

## Performance

- **Duration:** about 15 minutes (2026-11-09T22:53Z to 23:08Z, inside the D-15 timebox that ends 2026-11-10T05:53:09Z)
- **Tasks:** 2 of 2
- **Files:** 1 record created, 3 modified on main; 3 score files created and preserved only in the tag; 67 spike files removed from main

## Accomplishments

- `derive-scores.awk` computes C2, C3 and C4 from `results.tsv` by the protocol formulas in integer arithmetic. C1, C5 and C6 come from the anchors, each with a written reason. The amendment flag came from `protocol-order-check` (`amended_after_spike=no`).
- The scorer printed TOTAL c 7900, TOTAL rust 8020, MARGIN nominal 120 worst 120, LEADER rust, `RESULT=proposed-default`, `REASON=margin-below-10`. No item was not demonstrated.
- Record 0001 holds all 52 results rows, headline timings, the tools-required table (the D-36 nightly install included, 17.3 s), the scores with evidence, the scorer output, fallbacks, limits and a sensitivity table.
- Annotated tag `spike-tool-01-evidence` (local only; 01-12 publishes it) was created on the record commit `4d175ff` and verified before `git rm -r spikes/tool-01`; the gate stays green after removal.

## Scores (pre-registered rubric, nothing rescored)

| Criterion (weight) | C11 | Rust | Evidence |
|---|---|---|---|
| C1 memory safety (25) | 3.0 | 5.0 | C: explicit bounds guards, M08 and G5 pass (3.0 anchor). Rust: `forbid(unsafe_code)` on core and runner, unsafe only in capi with a SAFETY comment on every block, Miri passes (5.0 anchor) |
| C2 fast feedback (20) | 5.0 | 3.8 | M04-debug tie (both under 2 s); M06-debug 1.79 s vs 3.38 s gives Rust 2.6; mean 3.8 |
| C3 tools (15) | 5.0 | 4.0 | T 3 vs 4 |
| C4 glue (15) | 5.0 | 2.0 | G 198 vs 476, ratio 2.40, floor(50 x 198/476) = 20 tenths |
| C5 fuzz and sanitizer (15) | 4.0 | 4.0 | C: runtime archive linked by explicit path (c). Rust: dated nightly needed (a) |
| C6 portability and wasm (10) | 1.0 | 5.0 | C wasm not attempted (install cost recorded). Rust wasm32-wasip1 under wasmtime, transcript equals native |

Rust planted-bug ergonomics observation, recorded as a cost: `starts_with(b"DSPB")` was not found in 60 s (3,694,953 executions); a single little-endian integer compare was found in about 0.4 s.

## Not-demonstrated and not-done items

None scores ND. Recorded in the record's limits: C wasm not attempted (scored at the 1.0 anchor), no seeded-UB control for Miri, hyperfine install not timed, one machine and one session with a shared-host load average of 22.8 at the start, run-to-run variation in M13 for C (1.2 s then 0.4 s).

## Sensitivity (reported, not used to change the result)

Every variant stays `margin-below-10`, but the leader flips under two plausible single readings: counting wasmtime in Rust's tool count (C leads by 1.80) or applying rubric item (d) to Rust for the first planted-bug form (C leads by 1.80). The as-scored reading follows the protocol literally. This is why the record treats the lead as weak evidence and asks the owner.

## Task Commits

1. **Task 1 (tracer): scores, scorer output, record 0001, index, PROJECT.md pointer, results stage, evidence tag** - `4d175ff` (feat); tag `spike-tool-01-evidence` points here
2. **Task 2: remove spikes/ from main** - `b956d9e` (chore)

## Deviations from Plan

### Plan details applied with a stated interpretation

- **C3 tool count.** The plan says to count M10 tools "needed for build, test, install, fuzz and sanitizer" excluding baseline tools. I excluded wasmtime (M14 only) and Apple clang (system cc) by that text and disclosed the effect in the sensitivity table. This is an interpretation, not an amendment; no protocol text was edited.
- **No Rule 1-3 auto-fixes were needed.** Authentication gates: none.

### Notes

- In the M10 rust results row, one `|` in "curl|sh" was written "curl-sh" in the record's markdown table so the table parses. The source `results.tsv` is untouched in the tag.
- The `results` stage in `scripts/check` was added before the existing `results-negatives` stage.

## Issues Encountered

- `sed` on macOS does not support `\|` alternation; my first attempt to read `amended_after_spike` produced an empty value and the scorer reported SCORE_INPUT_BAD. I re-read the value with awk, replaced only the last line of the scores file and re-ran the scorer. No measurement or score changed.

## Known Stubs

None.

## Threat Flags

None. No new network endpoint, auth path or trust-boundary file access was added. T-01-23 mitigated (status equals the scorer's RESULT, no owner statement, no research-ranked default). T-01-24 mitigated (tag verified to contain the tree before `git rm`; protocol-order-check still finds the first spike commit in history). T-01-33 mitigated (derived scores from the script, reasons per score, amendment flag from the checker).

## User Setup Required

Owner decision needed. The owner reviews `docs/decisions/0001-language-build.md` and states a language and build choice verbatim (Rust with Cargo, or C11 with CMake and Ninja). A later edit sets `status: owner-recorded` quoting that statement and date. Until then Phase 2 is blocked.

## Next Phase Readiness

- 01-12 can publish `main` and the `spike-tool-01-evidence` tag after the owner review. The evidence tag is local only.
- Phase 2 is blocked on the owner-recorded language decision.

## Self-Check: PASSED

- Files present: docs/decisions/0001-language-build.md, scripts/check (results stage), docs/decisions/README.md (0001 row), .planning/PROJECT.md (pointer row); `spikes/` absent.
- Commits `4d175ff` and `b956d9e` are ancestors of HEAD; tag `spike-tool-01-evidence` is an annotated tag whose commit is `4d175ff`.
- `sh scripts/check` ends `CHECK RESULT PASS`.
