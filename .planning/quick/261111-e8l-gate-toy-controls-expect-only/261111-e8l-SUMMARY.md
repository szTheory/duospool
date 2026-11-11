---
phase: quick
plan: 261111-e8l
subsystem: gate / conformance harness
tags: [gate-time, toy-controls, conformance-run, selftest]
requires: []
provides:
  - "controls_mode runs each toy against a cases file cut to the ids of its EXPECT (filter_cases, mode filter-cases)"
  - "selftest mutation-toy-filter"
  - "local before/after gate timings in docs/ci-timings.md"
affects: [scripts/conformance-run, scripts/check, docs/ci-timings.md]
key-files:
  modified:
    - scripts/conformance-run
    - scripts/check
    - docs/ci-timings.md
decisions:
  - "The filter is its own mode (filter-cases) that controls_mode calls, so the selftest can drive the real code on a canned TSV without a build"
metrics:
  tasks: 2
  commits: 2
  plan_head_before: d5d2b54bcd6537d0d5d7031f05934f50b35ed6ab
  plan_head_after: 17fe5d0 (the two code commits; the SUMMARY commit follows it)
status: complete
requirements-completed: []
requirements-advanced: []
---

# Quick 261111-e8l: Toy controls run only their EXPECT cases

Each of the nine toy controls now runs against a cases TSV holding only the cases named in its EXPECT (1 to 3 ids) instead of all 181 cases; the verdicts are unchanged and the conformance stage dropped from 122 s to 31 s.

## What changed

- `scripts/conformance-run`: new `filter_cases CASES_TSV IDS OUT` (also reachable as mode `filter-cases`). It keeps the K rows of the wanted ids whole and in original order and rewrites the closing `END` row with the new count (the runner refuses an END count that differs from its case count). Fail
  closed: an id with no row, an empty id list, a source with no END row, or a row that is neither K nor END is exit 2 with a `HARNESS_ERROR` line and no output file. `controls_mode` calls it per toy in its input loop (after the existing exit-2 checks, which are unchanged) and the toy runner and
  supervisor now read `build/gate/controls/NAME/cases.tsv` and `build/gate/controls/NAME.ids`. Judging rules are untouched; the old full-list mode is not kept.
- `scripts/check`: selftest `mutation-toy-filter`. Unmutated copy: id beta gives exactly beta's rows plus `END 1`, an id no case carries gives exit 2 and no file. Mutated copy (the row-selecting condition now selects nothing): exit 2 with `HARNESS_ERROR`, no file, never a pass. It also requires the
  live toy runner line to name the per-toy cases file and id list and to call the filter. Checked red against a reverted wiring and against a removed missing-id check.
- `docs/ci-timings.md`: section for this task with the A/B and before/after numbers.

## Equivalence table (done once by hand before the first commit)

Full-list outcomes come from the gate run just before the change (toy results of build/gate/controls, 181 cases each); subset outcomes from the first run after it. The whole RESULT line up to the evidence path (so including the assertion count) was also identical in every row.

| Toy | EXPECT id | Full list | Subset | Assertions (both) |
|-----|-----------|-----------|--------|-------------------|
| adc-carry | cpu_alu | guest_failure | guest_failure | 1 |
| drop-region | digest_region | guest_failure | guest_failure | 11 |
| heuristic-reverse | noheur_reverse_vector | guest_failure | guest_failure | 68 |
| irq-priority-swap | irq_ctl_pre | guest_failure | guest_failure | 33 |
| shared-state | interleave_ab | guest_failure | guest_failure | 90 |
| shared-state | interleave_twins | guest_failure | guest_failure | 36 |
| shared-state | interleave_aba_order | guest_failure | guest_failure | 9 |
| timer-reload-off | tim_timer | guest_failure | guest_failure | 33 |
| timer-reload-off | timer_pure_function | guest_failure | guest_failure | 9 |
| trace-hook-mutates | observer_ds_proto | guest_failure | guest_failure | 259 |
| unmapped-value | mmu_mpr_256k | guest_failure | guest_failure | 60 |
| wrong-mirror | accept_384k_banksig | guest_failure | guest_failure | 256 |

Why a subset cannot change a verdict by structure: the kinds that read other cases (`alloc_zero`, `nonmutation`, `nonmutation_digest`, `opcode_exercise`, the deferred coverage kinds, trace-out) are in no EXPECT. The shared-state toy advances a process-wide clock, so its result depends on the step
phase; the subset changes that phase relative to the full run, and the three interleave cases stayed red with identical assertion counts. The phase now depends only on the three interleave cases themselves rather than on every case that precedes them in the list, so adding unrelated cases no longer
shifts it (the fragility recorded in the standing brief note). The note's advice is unchanged if the instruction set ever shifts it again: make the toy phase-independent, do not retune by eye.

## Timings (local, load-dependent, not a CI claim)

The machine was loaded by unrelated jobs (load average 13 to 62 during these runs), so per-stage seconds and a same-moment A/B are the reliable comparison.

Conformance stage alone, `sh scripts/conformance-run run`, warm tree, old script (parent commit) vs new, back to back:

| Run | Script | Wall s | Load avg at start |
|-----|--------|--------|-------------------|
| 1 | old | 122 | 13.33 17.68 18.64 |
| 1 | new | 29 | 27.52 20.16 19.41 |
| 2 | old | 140 | 30.46 21.65 19.98 |
| 2 | new | 25 | 61.64 37.94 26.81 |

Full `sh scripts/check --timing` (54 stages, all PASS in both):

| Measure | Before | After |
|---------|--------|-------|
| Load average at start | 24.76 22.07 19.84 | 56.23 39.04 27.65 |
| Load average at end | 17.79 19.39 19.26 | 62.51 50.14 35.70 |
| conformance s | 122 | 31 |
| sanitizers s (unchanged code; the toys do not run in sanitize mode) | 47 | 66 |
| other 52 stages, sum s (unchanged code) | 131 | 215 |
| sum of TIMING s | 300 | 312 |
| `time` wall | 4:59.94 | 5:13.50 |

The total did not fall in this pair of runs because the unchanged stages slowed down under 2 to 3 times the load (sanitizers 47 to 66 s, others 131 to 215 s). The conformance stage itself fell by about 90 s in both the A/B pair and the full runs. A quiet-machine run was not available.

## Verification

- `sh scripts/check --stage build && sh scripts/check --stage conformance`: pass, all nine `CONTROL NAME RED_OK`.
- `sh scripts/check --selftest mutation-toy-filter`: PASS.
- `sh scripts/check`: CHECK RESULT PASS (also run once with `--timing`).
- `sh scripts/check --selftest`: SELFTEST RESULT PASS (includes mutation-toy-filter).
- `sh scripts/audit-history --range d5d2b54..HEAD` (run after the code commits): clean.

## Deviations from Plan

**1. [Rule 3 - structure] The selftest drives the filter through a `filter-cases` mode, not through the whole `controls_mode`.** The plan asked for HARNESS_ERROR/exit 2 from controls_mode on a broken filter. controls_mode needs a built preset and nine generated toys, which is too heavy for a
selftest that mutates a scratch copy of scripts/. The filter is therefore a function that controls_mode calls directly and that is also exposed as a mode; the selftest mutates and runs that same code, and additionally checks that the live toy runner line reads the per-toy files and calls the filter.
The per-toy ids file is the existing `build/gate/controls/NAME.ids` (EXPECT ids, EXPECT order), not a new `controls/NAME/ids.txt`.

**2. [Environment] A first timing attempt of the "after" figure ran with warm toy builds.** The first post-change conformance stage (24 s) was compared with the baseline stage (122 s, which included cold toy builds). To avoid crediting the change with the build, the A/B pair above reruns the old
script warm; the stage saving is about 90 s, from the toy runs, not from the build.

Otherwise none; the plan was followed as written. No ROADMAP, Phase 3 plan file or STATE phase counter was touched. STATE.md has no "Quick Tasks Completed" table, so it was left alone. `.planning/state.json` is untouched and uncommitted.

## Notes for next plans

- A toy control needs its EXPECT ids to exist as cases; `filter_cases` makes that a hard exit 2 at gate time. New toys cost only their EXPECT cases.
- If a new toy's EXPECT names a kind that reads other cases (alloc_zero, nonmutation, nonmutation_digest, opcode_exercise, coverage kinds), the subset would no longer reproduce the full-list verdict; keep such kinds out of EXPECT or have the filter keep the cases they read.

## Self-Check: PASSED

- scripts/conformance-run, scripts/check, docs/ci-timings.md modified: FOUND
- commits 34e7650 and 17fe5d0 are ancestors of HEAD: FOUND
