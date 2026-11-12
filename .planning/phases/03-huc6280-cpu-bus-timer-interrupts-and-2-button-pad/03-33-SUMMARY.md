---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 33
subsystem: w8-close-readme-wording-timing-ci-budget-and-handoff
tags: [readme, docs-check, claim-words, timing, ci-budget, linux-repro, wla-dx, handoff, w8, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: seven supported ledger entries (03-32), the README and docs-check tool rule (03-38), the CI workflow and its 14 mini-roots (03-07), xasm-check with its optional WLA path (03-08)
provides:
  - "README Status names the seven supported entries by id with the plain meaning (the model equals the cited manual page, a documentary comparison, not a hardware measurement), keeps LED-CPU-0007 assumed, and holds the withheld claim words only quoted on one policy line"
  - "docs-check rules DOCS_CLAIM_WORD, DOCS_SUPPORTED_LIST and DOCS_SUPPORTED_MEANING with five mini-roots (claim-word, claim-word-quoted, supported-list, supported-meaning, ok-supported) and the selftest mutation-docs-claim-word (four lines, all PASS)"
  - "the gate job timeout raised from 10 to 20 minutes with the reasoning in docs/ci-timings.md; the 14 tests/workflow mini-roots regenerated, ok-gate byte-identical to the real file"
  - "docs/ci-timings.md: W8 local baseline (two runs, 56 stages), the one-off Linux container runs, the awk-call census, the CI critical-path estimate with its uncertainty, the efficiency decisions, the WLA-DX one-off record and the owner hand-off list"
  - "three real Linux-only defects found by the container run and fixed: gcc 13 format-truncation errors in five places of the conformance runner, and a null memcpy that UBSan reports on Linux"
  - "the optional WLA path ran for the first time: after one fix of the generated text it assembles all 234 rows, 233 equal the table, BRK differs; no cross-assembler claim"
affects: [phase-3-verification]

requirements-advanced: []
requirements-completed: [CPU-06, FIX-01, FIX-03]

actuals:
  tokens: 90000
  tasks: 3
  commits: 7
plan_head_before: e2c834bc850c130ef8d08e4b9b4f68184dbdfc5d
plan_head_after: 8fb4bb3728aebf49328bc1a27079d8bbd371dfc1

tech-stack:
  added: []
  patterns:
    - "a documentation claim is a gated artefact: the README paragraph that lists supported entries must equal the set of ledger entries
      with status supported (DOCS_SUPPORTED_LIST), must say the plain meaning (DOCS_SUPPORTED_MEANING), and the withheld words are legal
      only quoted on the policy line (DOCS_CLAIM_WORD), each with a red mini-root and a mutation selftest that deletes the tagged rule line"
    - "a CI budget decision from three independent estimates: a calibration of the one-off container run against the one real runner run, a per-group scaling, and a local cross-check; the limit is set to twice the high estimate and the next action is a review after the first real row"

key-files:
  created:
    - tests/docs/claim-word/
    - tests/docs/claim-word-quoted/
    - tests/docs/supported-list/
    - tests/docs/supported-meaning/
    - tests/docs/ok-supported/
  modified:
    - README.md
    - scripts/docs-check
    - scripts/check
    - scripts/audit-history.allow
    - scripts/xasm-check
    - scripts/lib/xasm-matrix.awk
    - tests/conformance/chunkkinds.c
    - tests/conformance/guestkinds.c
    - tests/conformance/obskinds.c
    - tests/conformance/reckinds.c
    - .github/workflows/gate.yml
    - tests/workflow/ (14 regenerated gate.yml copies)
    - docs/ci-timings.md

key-decisions:
  - "The README wording is enforced from the ledger, not only from a word list: the paragraph that starts with Supported entries must list exactly the entries whose status is supported, so a promotion or a demotion that forgets the README turns docs-claims red"
  - "timeout-minutes 10 is not safe: the estimated Phase 3 job is 6 to 10 minutes (three methods give 5.6 to 11), so it is raised to 20 (twice the high estimate), the smallest safe change, because a limit only bounds a hung job"
  - "The second awk run stays a whole gate run: 52 of 56 stages run awk (5736 calls counted, a lower bound) and the heavy stages are not exceptions, so nothing can be restricted without dropping awk coverage; the only candidate, sanitizers, mostly repeats conformance's awk work and is recorded for review after the first CI row"
  - "WLA-DX was installed, run once and uninstalled: while it is on PATH the xasm stage turns red on the BRK difference, and the owner has not decided whether BRK is an accepted WLA exception"

patterns-established:
  - "a one-off Linux container run is part of finishing a wave: this run found three defects (two warnings-as-errors classes and a UBSan check) that nine waves of macOS-only gates had not"

duration: 330min
completed: 2026-11-11
---

# Phase 3 Plan 33: W8 close - README wording, timing re-baseline, CI budget review, WLA and Linux runs, owner hand-off Summary

**The README now says exactly what Phase 3 evidences and the gate enforces it from the ledger; the Linux container run found and fixed three
defects that only gcc 13 on Linux shows; the CI timeout is raised from 10 to 20 minutes on the evidence of an estimate that puts the Phase 3
job at 6 to 10 minutes; the first WLA-DX run is recorded (233 of 234 rows equal, BRK differs, no claim); nothing was pushed.**

## What was built

- **Task 1 (tracer), commit f3e7ef2.** README Status rewritten. It lists LED-CPU-0001, -0002, -0005, -0014, -0016, LED-IRQ-0002 and
  LED-IRQ-0004 (part a only) as supported at E1, documentary, with the plain meaning "the model equals the cited manual page: a documentary
  comparison, not a hardware measurement"; LED-CPU-0007 stays assumed; every other claim is a labelled modelling assumption; the gate makes
  no second-assembler claim. The withheld words (accurate, cycle-accurate, interrupt-accurate, pad-accurate, hardware-verified, compatible,
  supports TG-16/PC Engine, boots, passes the CPU tests) appear only quoted, on one policy line. `scripts/docs-check` gained three rules,
  each one tagged line with a mini-root: DOCS_CLAIM_WORD (case-insensitive whole words, hyphen is a boundary, allowed only inside double
  quotes on the line that starts with `Withheld claim words:`), DOCS_SUPPORTED_LIST (the README paragraph that starts with `Supported
  entries` equals the ledger's supported set; skipped for a root with no `ledger/entries`) and DOCS_SUPPORTED_MEANING (the fixed phrases are
  present once anything is supported; the text is joined first because the README wraps). Order of work: the rules were added and shown to
  fire on the old README (red), then the README was rewritten (green). Mini-roots: claim-word (plain word), claim-word-quoted (a quoted word
  on a non-policy line is still a violation), supported-list, supported-meaning, ok-supported (over-fire control: the policy line, the list
  and the meaning together pass). Selftest `mutation-docs-claim-word` deletes each tagged line from a scratch copy and requires the matching
  control to flip; all four PASS. A first draft of two rules left an empty `then` body when the tagged line was deleted (a syntax error that
  the selftest exposed); they were rewritten as single-line `||` forms.
- **Task 2, commits b578443, 7c7e065, bf1bef0 and the timing records.** `sh scripts/check --timing` twice (below); `scripts/linux-repro` several times (below); WLA-DX from the Homebrew bottle, run once, uninstalled. Three real defects found by the Linux runs are fixed here (deviations).
- **Task 3, commits 1c1bd74, ce0d71a, 8fb4bb3.** CI status, the efficiency review and the hand-off list are in `docs/ci-timings.md`. The workflow timeout and its 14 mini-roots changed together.

## Verification

- Task 1 verify: `--stage docs-claims` and `--stage docs-claims-negatives` PASS (the new controls: five mini-roots, 11 docs mini-roots in all). Task 2 verify: `--timing` PASS, `--stage xasm` PASS (with WLA absent: `wla=optional-absent`). Task 3 verify: `--stage ci-timings-controls` PASS and the full gate PASS.
- Plan end, on the tree at commit 8fb4bb3 (docs commits only after): `sh scripts/check` PASS (56 stages, 57 PASS lines with the result
  line), `sh scripts/check --selftest` PASS (113 PASS lines, no FAIL, 4 of them new), `sh scripts/audit-history --range e2c834b..HEAD` clean
  after the allowlist entry (one ROM-SIZE-HDR size coincidence on an 8704-byte shell script, see deviations). `dash scripts/check` was not
  run (optional). Nothing pushed.
- Linux container (local one-off, not CI): final part `gawk` (gawk 5.2.1, gcc 13.3.0): gate exit 0, 56 stages PASS in 856 s emulated, selftest exit 0 (113 lines, no FAIL). Final part `mawk` (mawk 1.3.4, clang 18.1.3, a scratch copy of the script with the selftest omitted because CI does not run it): gate exit 0, 56 stages PASS in 793 s emulated.

## Timing and the CI budget review (full text and tables in docs/ci-timings.md)

- **Local, warm tree, commit bf1bef0, shared machine:** run A 193 s wall (129 s user, 56 s system, load 15), run B 295 s wall (load 50
  falling to 14). The 120 s target is **missed** in both, and the user CPU alone (129 s) exceeds it. Stages over 20 s: `conformance` 31 s
  (55 in run B), `sanitizers` 52 s (65), `determinism` 30 s (24): 113 of 192 s. The W1 baseline was 62 s for 50 stages. Time never fails the
  gate (D-132).
- **Linux (emulated amd64, cold):** gawk+gcc 856 s, mawk+clang 793 s; the compute stages are 39 to 44 percent of it. These are not runner predictions.
- **CI critical path estimate** (no Phase 3 CI row exists): calibration of the Phase 2 pair (container 216 and 207 s against runner 37 and
  25 s), scaled per group (spawn-bound 5.8 to 8.3, compute 3 to 5): low 338 s, central 458 s, high 568 s (5.6, 7.6 and 9.5 minutes); a local
  cross-check gives about 8 to 11 minutes. The cc65 apt step has no measurement at all (guessed 10 to 30 s). One calibration point and an
  emulator: at least plus or minus 50 percent.
- **Decision 1, timeout:** 10 minutes sits inside the estimate, so `timeout-minutes` is now 20 (twice the high estimate). The 14 mini-roots were regenerated by the same text replacement, each differs from the real file by the same lines as before (diff line counts equal before and after), `ok-gate` is byte-identical, `workflow` and `workflow-negatives` PASS.
- **Decision 2, second awk run:** stays a whole gate run. Census (an `awk` shim first on PATH, each stage alone, 5736 calls, a lower bound):
  52 of 56 stages run awk; only `preflight`, `empty-expect-controls`, `protocol-order` and `build` run none; `conformance` makes 675 calls,
  `sanitizers` 584, `fixtures` 707, `inventory` 847. The one stage that duplicates another's awk work is `sanitizers` (about 240 case parses
  with `toml-subset.awk`, the same as `conformance`; one judging call differs), worth about 25 to 40 s of the estimated job, but dropping it
  needs a skip mechanism in the fail-closed gate and the stage does run awk, so it is not done.
- **Decision 3, sanitizers under the second compiler (a note, nothing changed):** CI runs `sanitizers` only under the default `cc` (gcc on
  the runner) in both gate runs. The container shows clang 18 + UBSan passes on Linux (146 s emulated). The compiler matters: gcc's UBSan
  flagged a null memcpy that the macOS clang run never did. Options: a third run (about 30 to 50 s) or make the second awk run use clang (no
  extra minutes, but a failure would no longer say whether awk or the compiler caused it). Left for the owner and for the first CI row.
- **Next action (recorded):** after the owner authorises a push, add the first Phase 3 CI row with `sh scripts/ci-timings RUN_ID` and review the timeout, the second-awk subset and the sanitizers-under-clang question against it.

## WLA-DX (optional cross-assembler, one local run)

WLA-DX 10.7 (Homebrew core bottle, GPL-2.0-or-later, external tool only) was installed, `sh scripts/xasm-check --rows 234` was run, and the
tool was uninstalled. The WLA text had never run. First result: `XASM_WLA_FAILED` (`inc a` is read as an operand named `a`; WLA spells the
accumulator forms `ina` and `dea`). After that fix (commit 7c7e065): 234 rows assembled, **233 equal the table, 1 differs (BRK: WLA emitted
`00 00`)**. So `wla=equal` has never been printed and **no cross-assembler identity claim is made**; the reason for the second byte is a
guess, not checked against the WLA documentation. Nothing of WLA is committed. Uninstalled because, while it is on PATH, the `xasm` stage
fails on the BRK mismatch.

## Requirements closed (evidence)

- **CPU-06** (instruction and cycle claims cite their source; an uncited cycle count is a labelled assumption): seven ledger entries are
  supported with page-image-read cites (03-32, E1 documentary), every other cycle claim (LED-CPU-0007 and the timer, bus and interrupt model
  entries) is assumed or open with the label; `ledger-entries` enforces an E1 FACT source for supported, and INV_CONST_LEDGER (03-32)
  requires every constant and model parameter row to name an existing ledger id. Gate green.
- **FIX-01** (original authored fixtures, project-owned startup code, an external license-checked assembler, a cross-assembler check where
  practical): 13 authored fixtures, rights and licence stages, cc65 zlib recorded in `fixtures/lib/TOOLS.md`; the gate-enforced `xasm` stage
  compares all 234 ca65 encodings with the manual transcription (two independent encoders, identity by bytes); the WLA one-off ran and gave
  233 of 234, so a byte-identity check across two assemblers is **partial and optional** and is not claimed. It was not practical to
  assemble the fixture images with WLA (project-owned ca65 startup and linker configuration). This is the one requirement closed with a
  stated limit; the verifier may re-open it.
- **FIX-03** (fixture verdicts from documented behaviour or an independent in-guest verdict, never DuoSpool's own recorded output): fixture-lint classes every expected literal as MAN, GUEST or ASSUMED (561 check rows; MAN 378, GUEST 58, ASSUMED 125, 03-32), the guest judge refuses a missing or wrong classes list, and no golden or recorded output supplies a literal. Gate green.

## What was automated and what is handed to the owner (recorded once)

**Automated (verifier need not re-ask):** 56 gate stages (9 added in Phase 3: fixture-build, fixture-lint, fixture-lint-negatives,
input-check, input-check-negatives, xasm, opcode-oracle, opcode-oracle-negatives, xasm-negatives) with their mini-root control families, 234
conformance case files including 39 guest-kind cases, the toy and named controls, 113 selftest mutations, the README claim-word and
supported-list rules with five controls, the 14 workflow mini-roots, a Linux container reproduction under both awks and both compilers
(local one-off, not CI).

**Handed to the owner (not done, not authorised in this plan):**
1. Push and pull request authorisation (D-64) for the Phase 2 review-fix commits and all Phase 3 commits (about 180 after the pushed head b063518). CI evidence is `unverified, pending owner push authorisation`.
2. Three remote settings: Actions default workflow permissions read-only, Actions not allowed to approve pull requests, and a branch ruleset on `main` requiring `gate`.
3. CODEOWNERS handle (`@szTheory`, taken from the local git config) awaiting confirmation.
4. ADR 0008 (evidence-tier policy) still `proposed-default` (the seven promotions follow it); an optional ask.
5. Optional hardware captures: not needed for any current claim.
6. The `CLAUDE.md` Technology Stack section still reads as an unadopted Rust proposal ("research proposal, NOT a decision") while decision 0001 governs. Recorded only; not edited.
7. Whether to accept a recorded WLA exception for BRK so the optional cross-check can print `equal`.

## Deviations from Plan

**1. [Rule 1 - Bug] gcc 13 format-truncation errors on Linux (commit b578443).** The first container run failed the `build` stage with
`-Werror=format-truncation` in `chunkkinds.c:512`, `guestkinds.c:1203` and `:1585`, `obskinds.c:1341` and `reckinds.c:534` (buffers too
small for the longest possible text). Only buffer sizes changed. These files are not in the plan's list; the plan could not complete its
Linux step without them, and a first CI run would have been red.
**2. [Rule 1 - Bug] null memcpy under UBSan on Linux (commit bf1bef0).** `obskinds.c:854` copied zero bytes into a null pointer when a trace log was empty (undefined behaviour). Each copy is now guarded by its count.
**3. [Rule 3 - blocking, files outside the plan list] The WLA text for accumulator INC and DEC (commit 7c7e065)** in `scripts/lib/xasm-matrix.awk`, and the header of `scripts/xasm-check` now records the one run. The plan only said to run it; the path failed on its first ever run. It was fixed within three attempts, the BRK difference was not hidden or excused.
**4. [Rule 3 - blocking] Audit allowlist entry (commit 8fb4bb3).** The history audit's copier-header size rule matched an 8704-byte revision of `scripts/docs-check` (8192 + 512, a coincidence). A path-and-check allowlist entry was added; it applies only while the blob has no NUL byte.
**5. [Plan wording] More than the plan's word rule.** The plan names a claim-word rule only; DOCS_SUPPORTED_LIST and DOCS_SUPPORTED_MEANING were added at the orchestrator's request so that the README cannot drift from the ledger. `scripts/check` (the selftest) and the four extra mini-roots are outside the plan's file list.
**6. [Plan wording] The plan wanted per-stage seconds against 120 s; the result is a miss, not a pass.** Recorded as such, with the compute stages named. The WLA step ended with "uninstalled" rather than "left", with the reason recorded.
**7. [Process] The Linux run is not one clean run of `scripts/linux-repro`.** Two earlier runs were stopped after they had found the defects
above (both during their selftest), and the final `mawk` part used a scratch copy of the script (selftest omitted, repository root changed)
because CI does not run the selftest and the wait was about 20 minutes per selftest. The scratch copy is not committed and is described in
`docs/ci-timings.md`.
**8. [Process] The per-plan commit ledger file was not created;** the start commit is recorded in the frontmatter and `commits: 7` is measured with `git rev-list --count e2c834b..HEAD` before this SUMMARY and the docs commit.

## Evidence discipline

E1 documentary only for the seven supported entries; LED-CPU-0007 stays assumed; no ledger status changed in this plan. The README makes no
accuracy, compatibility, cycle-accuracy, hardware-verification or boot claim and the gate rejects those words. The WLA result is a single
observation of an external tool, labelled unproven. Linux numbers are emulated local one-offs, the CI estimate is an estimate with its
uncertainty, and CI for Phase 3 is `unverified, pending owner push authorisation`. No ROM, firmware, third-party text or WLA output is
committed. No push, pull request, tag or setting change was made.

## Known Stubs

None. `.planning/WINDOWS.md` is absent, so nothing was appended.

## Threat Flags

None beyond the plan's register. T-03-92 (overclaiming in docs): the claim-word, supported-list and meaning rules each have a red mini-root and a mutation selftest. T-03-93 (unattended remote write): no push, PR, tag or setting change. T-03-106 (WLA install): Homebrew core bottle, local only, uninstalled afterwards, output discarded.

## Notes for next plans

- **Verification:** read the requirement evidence above; FIX-01 is closed with the stated limit (cross-assembler identity partial and optional). Recheck that README's supported list still equals the ledger if any entry changes (the gate does it).
- **Open numbers:** the cc65 apt step cost and the whole Phase 3 CI critical path are estimates until the first real run; the local 120 s target is missed (193 s best) and the three compute stages are the cause; a faster local loop would need per-case timing of `conformance`, `sanitizers` and `determinism`, which is not recorded yet.
- **Stale text:** `scripts/linux-repro` still prints "no CI run exists" in its closing note; a Phase 2 CI run does exist. Left unchanged (out of scope).
- `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: tests/docs/{claim-word,claim-word-quoted,supported-list,supported-meaning,ok-supported}. Commits f3e7ef2, b578443, 7c7e065, bf1bef0, 1c1bd74, ce0d71a and 8fb4bb3 are ancestors of HEAD.
