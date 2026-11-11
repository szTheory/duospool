---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 21
subsystem: ci
tags: [docs-check, tool-02, linux-repro, container, gcc, clang, mawk, gawk, leaksanitizer, ci-timings]
requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: scripts/check with 45 stages, workflow-check and the ok-gate controls, scripts/ci-timings and docs/ci-timings.md (02-19, 02-20), the mawk fix in record-rules.awk (02-20)
provides:
  - README.md and AGENTS.md state the one local gate command, its flags, exit codes and tool prerequisites; the stale Phase 1 status sentence is gone
  - scripts/docs-check (DOC_NO_GATE_README, DOC_NO_GATE_AGENTS, DOC_STALE_STATUS, DOC_FLAG_UNDOCUMENTED), five controls under tests/docs, the stages docs-claims and docs-claims-negatives, and the selftest mutation-docs-stale; the gate now has 47 stages
  - scripts/linux-repro, the one-off Ubuntu 24.04 container run (gcc 13.3.0, clang 18.1.3, mawk 1.3.4, gawk 5.2.1), six parts, labelled a local container run and not a CI claim
  - the first clang plus LeakSanitizer evidence on Linux, which found and fixed a real leak in ds_runner
  - the local per-stage timing baseline and efficiency review in docs/ci-timings.md; 02-VALIDATION.md wave_0_complete and nyquist_compliant true
affects: [02-22 (push checkpoint and criterion 5: CI-01 stays pending until a real CI run exists), any later edit of scripts/check usage line or README gate section (docs-check must stay green)]

actuals:
  tokens: 8670     # chars/4 over the realized diff (git diff plan_head_before..plan_head_after excluding .planning, 34680 chars, 27 files, 553 added lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit); one is the ds_runner leak fix, three are the plan tasks
plan_head_before: b6ba2a2363b01fcd80665301c9bc8d36f0e13387
plan_head_after: 13a35bce2d2a07cd1abb26674dc69c23a0aeeef4
commits: 4

tech-stack:
  added: []
  patterns:
    - "a docs claim is a static assertion with a red control: the usage line of scripts/check is parsed for its --flags and each must appear in README.md"
    - "a one-off environment reproduction is a script, not a transcript: scripts/linux-repro prints the awk and compiler versions per part and exits 2 (never a silent pass) when docker is absent"
    - "a leak that one compiler hides (gcc left the pointer in a stale stack slot) is found by running the second compiler: D-41 earns its place"

key-files:
  created:
    - scripts/docs-check
    - scripts/linux-repro
    - tests/docs/ (five mini-roots: ok-docs, no-gate-readme, no-gate-agents, stale-status, flag-undocumented)
  modified:
    - README.md
    - AGENTS.md
    - docs/decisions/0007-copied-code-provenance.md
    - docs/ci-timings.md
    - scripts/check
    - tests/conformance/runner.c
    - .planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-VALIDATION.md

key-decisions:
  - "the container parts differ on purpose so that no full gate repeats another: default-awk is mawk with gcc, mawk is mawk with clang, gawk is gawk with gcc"
  - "the stock awk part runs before gawk is installed, because gawk's alternatives priority would otherwise take over the name awk"
  - "the container leaves its logs in a scratch directory and prints its location; nothing from the run is committed"
  - "the full-history audit hits (02-01 and 02-02 summaries, home-path-mac) are not allowlisted: that is an owner decision (WINDOWS.md entry 1)"
  - "CI-01 is un-ticked: criterion 5 (a green CI run) is unverified-pending-push"

requirements-completed: [TOOL-02]

coverage:
  - id: D1
    description: "One documented local command equals the CI gate command, and a static check keeps the documented claims true"
    requirement: TOOL-02
    verification:
      - kind: integration
        ref: "sh scripts/check --stage docs-claims PASS; --stage docs-claims-negatives PASS (ok-docs accepted, four negatives each with its single code); --selftest mutation-docs-stale PASS (before=1 after=0 lines tagged rule-stale, the stale-status control flips from EXPECT_OK to exit 1)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The gate runs green under dash, the full selftest, and a Linux container with gcc 13, clang 18, mawk and gawk"
    requirement: TOOL-02
    verification:
      - kind: integration
        ref: "dash scripts/check exit 0 CHECK RESULT PASS; sh scripts/check --selftest exit 0; sh scripts/linux-repro LINUX_REPRO RESULT PASS with all six parts PASS (local one-off, not CI)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Local per-stage timings are recorded as a labelled baseline with efficiency-review notes"
    requirement: TOOL-02
    verification:
      - kind: integration
        ref: "docs/ci-timings.md holds a local baseline table of 47 rows, the container run labelled one-off, and the review; grep -c 'Local baseline' docs/ci-timings.md is 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "02-VALIDATION.md frontmatter reflects the green suite"
    requirement: TOOL-02
    verification:
      - kind: integration
        ref: "head -12 of 02-VALIDATION.md holds the lines nyquist_compliant: true and wave_0_complete: true (grep -c prints 1 each)"
        status: pass
    human_judgment: false
  - id: D5
    description: "No remote write occurs and CI-01 is not claimed"
    requirement: CI-01
    verification:
      - kind: manual
        ref: "no git push, tag, gh call or remote setting in this plan; CI-01 un-ticked in REQUIREMENTS.md; the only network use was the container image pull and apt inside the container"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 21: Documented gate command, docs-check, and the Linux container run Summary

**README and AGENTS.md now name `sh scripts/check` as the one command CI also runs, `scripts/docs-check` with five controls keeps those claims true, and a new `scripts/linux-repro` ran the whole gate in an Ubuntu 24.04 container (gcc 13 and clang 18, mawk and gawk) and found one real defect, a 40-byte leak in `ds_runner` that only clang's LeakSanitizer reported, now fixed.**

## Performance

- **Duration:** about 1 h 20 min (2026-11-10), most of it waiting for emulated linux/amd64 container runs (three full runs of 800 s each, one with a real finding).
- **Tasks:** 3 of 3 (Task 1 tracer, Tasks 2 and 3 auto) plus one fix commit for a defect the container found.
- **Files:** 27 changed outside .planning (553 added lines, 4 removed).
- **Gate cost (this machine, warm):** `sh scripts/check` 45 s wall on a quiet machine, 47 stages, all PASS; no stage over 20 s (longest `sanitizers` 6 s). The same gate took 69 s, 84 s and 112 s wall earlier the same day while the shared machine was loaded (load average 17 to 32). `--selftest` 10 s, `dash scripts/check` 46 s, `audit-history` 50 s.

## Accomplishments

- **Tracer (Task 1, commit 4024293).** README Status rewritten to Phase 2 (a deterministic machine shell that accepts or refuses plain HuCard bytes; no CPU, video or audio; nothing claimed supported). The Local gate section gives the command, the flags `--selftest [NAME]`, `--stage NAME` and `--timing`, the exit codes 0, 1 and 2, the tool prerequisites (including git and the `cc` name) and the exact line `CI runs the same command: sh scripts/check.` AGENTS.md has a Local gate section before the standing protocols. Record 0007's template sentence now says the pull-request template ships with the Phase 2 CI workflow and `workflow-check` asserts the line; the not-decided bullet was brought in line. `scripts/docs-check` implements the four DOC codes (the flag rule parses the first `# usage:` line of `scripts/check` for `--flags`); five mini-root controls (`ok-docs` plus one code each); stages `docs-claims` and `docs-claims-negatives`; selftest `mutation-docs-stale`. The tracer gate re-ran the plan `<verify>` end to end before expansion: all PASS.
- **Phase-end suite (Task 2).** `sh scripts/check`, `--selftest`, `dash scripts/check` all green; the range audit of this plan's commits is clean; `sh scripts/rights-check` exits 0 and no ROM-like file is tracked.
- **scripts/linux-repro (commit 7c720e2).** Runs the working tree (without `build/`) in `ubuntu:24.04` linux/amd64 with git safe.directory set, installs the toolchain by apt, and runs six parts: `default-awk` (stock mawk, gcc), `mawk` (mawk, clang), `gawk` (gawk, gcc) each as the full gate plus the selftest; `gate-gcc` and `gate-clang` (the presets with `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`); `core-boundary-gcc` (`core-boundary-check --cc gcc` and the stage). It prints the awk and compiler versions per part, the banner `local container run, not a CI claim`, and exits 2 with `LINUX_REPRO_UNAVAILABLE` when docker is absent. Header and SPDX per D-80.
- **The defect the container found (commit 3cfff5c).** With clang 18 as `cc` and LeakSanitizer on, the `sanitizers` stage failed: a direct leak of 40 bytes, symbolized (after adding llvm-symbolizer) to `read_file` called from `main` at the exclusion-file read in `tests/conformance/runner.c`. The buffer `xt` was never freed. gcc did not report it (the pointer was still in a stale stack slot), so the earlier gcc container runs of plan 02-20 passed. Fix: `free(xt)` after the ids are consumed and before the error return. After the fix the stage is PASS under clang on Linux and on this Mac.
- **Timing baseline (Task 3, commit 13a35bc).** `docs/ci-timings.md` gained a local baseline section: 47 per-stage rows, the container run labelled one-off, and the efficiency review.

## Verification Results

All `<automated>` commands and acceptance criteria were run. Results on this machine (macOS, BWK awk 20200816) unless the row says container.

| Command | Result |
|---------|--------|
| `sh scripts/check --stage docs-claims && ... --stage docs-claims-negatives && ... --stage decision-records && sh scripts/check --selftest mutation-docs-stale` (Task 1) | all PASS, exit 0 |
| `grep -c 'sh scripts/check' AGENTS.md` | 1 (baseline 0) |
| `grep -cxF 'CI runs the same command: sh scripts/check.' README.md` / `grep -c 'no emulator code yet' README.md` | 1 / 0 (baselines 0 / 1) |
| `grep -c -e --selftest / --stage / --timing README.md` | 2 / 2 / 2 (baseline 0 each) |
| `sh scripts/docs-check` | `STATUS docs ok`, exit 0; `ls tests/docs` lists 5 directories; the no-gate-readme control passes with `--expect DOC_NO_GATE_README` |
| `head -5 scripts/docs-check` and `scripts/linux-repro` contain the SPDX MIT OR Apache-2.0 line | 1 each |
| `sh scripts/check --timing` (Task 2) | exit 0, `CHECK RESULT PASS`, 47 stages (baseline 45 at plan start; plan text said 18), 45 s wall, no `TIMING_WARN` |
| `sh scripts/check --selftest` | exit 0, `SELFTEST RESULT PASS`, includes `mutation-docs-stale PASS` |
| `dash scripts/check` | exit 0, `CHECK RESULT PASS`, 46 s |
| `sh scripts/audit-history` (full history, 192 commits) | exit 1, 2 hits, both the known `home-path-mac` finding in the 02-01 and 02-02 summaries (WINDOWS.md entry 1, owner decision); not new, see Deviations |
| `sh scripts/audit-history --range <plan start>..HEAD` | exit 0, `clean` (2 commits at the time, 17 blobs) |
| `sh scripts/rights-check`; ROM-like files tracked | exit 0 (`RECORDS 980`); none |
| `sh scripts/linux-repro` (final run) | exit 0, `LINUX_REPRO RESULT PASS`, all six parts PASS, 802 s one-off |
| head -12 of 02-VALIDATION.md anchored greps (Task 3) | `nyquist_compliant: true` 1, `wave_0_complete: true` 1; `grep -c 'Local baseline' docs/ci-timings.md` 1; table rows 47 |
| final `sh scripts/check` on the committed tree | exit 0, `CHECK RESULT PASS`, 47 stages |

### Container run (local one-off, NOT a CI claim)

Method: `scripts/linux-repro` in `ubuntu:24.04` (linux/amd64, emulated on this arm64 Mac), `uname` Linux x86_64, Ubuntu 24.04.5 LTS, cmake 3.28.3, dash as `sh`. Final run, from a cold `build/` each part:

| Part | awk | cc | Result |
|------|-----|----|--------|
| default-awk | mawk 1.3.4 20240123 | gcc 13.3.0 | gate 47 stages PASS (193 s), selftest PASS |
| mawk | mawk 1.3.4 20240123 | clang 18.1.3 | gate 47 stages PASS (207 s), selftest PASS |
| gawk | gawk 5.2.1 | gcc 13.3.0 | gate 47 stages PASS (216 s), selftest PASS |
| gate-gcc | gawk | gcc 13.3.0 | build of the preset with `-Werror` PASS |
| gate-clang | gawk | clang 18.1.3 | build of the preset with `-Werror` PASS |
| core-boundary-gcc | gawk | gcc 13.3.0 | `STATUS core-boundary ok`, stage PASS |

Honest scope: these runs are the only gcc, mawk and gawk evidence of this repository so far and none of them is a CI run. Plan 02-20 ran a smaller container check (the gate at two commits under mawk and gawk, gcc 13 and clang 18 installed, `cc` left at gcc), so this is not literally the first container run; what is new here is clang as `cc` for the full gate and its sanitizer stage, the two presets, and a repeatable script. Two earlier runs of the same script failed and are described under Deviations.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] ds_runner leaked the exclusion-file buffer (clang plus LeakSanitizer on Linux)**
- **Found during:** Task 2, the first full `mawk` part of the container run (clang as `cc`).
- **Issue:** `ds_runner --exclude-ids` read the file with `read_file`, walked it with a moving pointer and never freed it; LeakSanitizer reported `Direct leak of 40 byte(s)` and the `sanitizers` stage failed. gcc did not report it, so the earlier runs were green by accident of codegen. Apple's ASan has no leak detector, so the Mac never saw it.
- **Fix:** `free(xt)` once the ids are consumed (`excluded[]` holds indexes, not pointers into the text) and before the early error return. A case first was not added: the existing `sanitizers` stage with a leak-detecting compiler on Linux is the control, and it went red on this exact defect before the fix and green after. (A Mac-only canary for leaks is impossible; a Linux-only one was not added.)
- **Files modified:** `tests/conformance/runner.c`
- **Verification:** container `build`, `outcome-sync`, `conformance` and `sanitizers` stages under clang PASS; the same stages PASS on this Mac; the final container run PASS.
- **Commit:** 3cfff5c

**2. [Rule 3 - Blocking issue] Ubuntu's clang does not carry the sanitizer runtime**
- **Found during:** Task 2, the first `mawk` part (clang): `cannot find libclang_rt.asan-x86_64.a` while linking the `sanitize` preset.
- **Issue:** on Ubuntu the ASan and UBSan runtimes ship in `libclang-rt-18-dev`, which `clang` with `--no-install-recommends` does not pull. This is a prerequisite of the `sanitizers` stage with clang on Linux, not a defect in the repository.
- **Fix:** `scripts/linux-repro` installs `libclang-rt-18-dev` (and `llvm-18` so a report names functions and lines, with `ASAN_SYMBOLIZER_PATH` set); README documents the prerequisite and the symptom (a link error naming `libclang_rt.asan`). The repository's `preflight` does not check the runtime; the stage fails loudly with a link error, not silently.
- **Files modified:** `scripts/linux-repro`, `README.md`
- **Commit:** 7c720e2

### Other deviations

- **`sh scripts/audit-history` (full history) exits 1, so the plan's chained `<verify>` command is not literally green.** The two hits are the known `home-path-mac` finding in the 02-01 and 02-02 summaries in history that pre-dates plan 02-04 (WINDOWS.md entry 1, `deferred-items.md`): no new hit; the range audit of this plan's own commits is clean. Fixing it means rewriting unpublished history or allowlisting with a reason, which is an owner decision before publication. I did not allowlist or rewrite anything.
- **The plan says the warm gate over 60 s needs its dominant stage reduced.** The first warm runs took 69, 84 and 112 s while the shared machine was loaded (system load 17 to 32); a quiet run took 45 s. No stage is above 6 s and none is a bottleneck by itself, and every stage is real evidence (compiles) or a control; I changed nothing and recorded why in `docs/ci-timings.md` rather than trade a control for time.
- **The plan's acceptance wording** ("states that this is the first real gcc, mawk and gawk evidence on this Mac") was not copied literally because plan 02-20's smaller container check came first. `scripts/linux-repro` instead states that these are the only gcc, mawk and gawk evidence so far, local one-offs, no CI run.
- **Plan text baseline** of 18 stages is stale; the real baseline at plan start was 45, and the gate now has 47 (the plan's "at least 47" is met exactly).

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking environment prerequisite), 3 recorded notes. **Impact:** one real clang-only defect fixed with its control intact; no control weakened.

## Requirements bookkeeping

- TOOL-02 is met (one documented local command equals the CI gate command, kept true by `docs-check`) and is marked complete.
- **CI-01 is deliberately NOT complete.** It was ticked by plan 02-20; this plan un-ticks it in REQUIREMENTS.md and sets its traceability back to Pending. Reason: criterion 5 (a green GitHub Actions run with its critical-path timing) is unverified-pending-push. No CI run exists, pushing is an owner decision (D-64), and the workflow has only been judged statically and by local container runs. Plan 02-22 owns the push checkpoint.

## Deferred Issues

None new. The open items are unchanged: the history home-path finding (owner decision), the pending remote hand-offs (default workflow permissions read-only, Actions approving pull requests off, a branch ruleset on `main`), and the first real CI timing row.

## Hand-offs and first-run review items

- No push, tag, remote setting or `gh` call was made. Nothing in this plan needs the owner except what plan 02-22 already carries.
- `LINUX_REPRO` logs stayed in a scratch directory; nothing from the container run is committed.
- After the first real CI run: `sh scripts/ci-timings RUN_ID`, paste the row, compare with the local baseline in `docs/ci-timings.md` (allow for runner speed and the load note there), and record the efficiency review.

## Notes for later plans

- Any edit to the usage line of `scripts/check` or to README's Local gate section must keep `sh scripts/docs-check` green: a new `--flag` in the usage line must be named in README.
- `sh scripts/linux-repro` takes about 14 minutes emulated; `LINUX_REPRO_PARTS="gate-gcc gate-clang core-boundary-gcc"` runs the three quick parts in about a minute.
- clang is the compiler that finds leaks LeakSanitizer sees; the CI gate uses gcc as `cc`, so a leak clang finds would not fail CI. The `clang build and conformance` step in CI builds and runs the conformance cases under clang; it does not run the sanitizers stage under clang. Consider that when judging the first CI run (not changed here: a new CI step is a plan-22-or-later decision).

## Task Commits

1. Task 1 (tracer): 4024293 `feat(02-21): tracer - gate command documented, docs-check keeps the claims true`
2. Defect fix found by the container: 3cfff5c `fix(02-21): free the exclusion file buffer in ds_runner (LeakSanitizer under clang on Linux)`
3. Task 2: 7c720e2 `feat(02-21): scripts/linux-repro, the one-off Ubuntu 24.04 container run of the gate (gcc 13, clang 18, mawk, gawk)`
4. Task 3: 13a35bc `docs(02-21): local timing baseline, efficiency review and validation status`

## Known Stubs

None.

## Threat Flags

None. `scripts/linux-repro` adds no endpoint or trust-boundary file access beyond a read-only bind mount of the working tree and a scratch output directory; its network use is the container image pull and apt inside the throwaway container. T-02-45 (claiming Linux or criterion 5 without evidence) is mitigated: the container run is labelled a local one-off and CI-01 is un-ticked. T-02-46 (weakening a control for a Linux-only failure) is mitigated: the one Linux-only failure was fixed at its cause and the stage that caught it is unchanged.

## Self-Check: PASSED

Created files (scripts/docs-check, scripts/linux-repro, the five tests/docs mini-roots) exist; commits 4024293, 3cfff5c, 7c720e2 and 13a35bc are ancestors of HEAD; the summary and the new files carry no home-directory path.
