---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 12
subsystem: publication-audit
tags: [tool-01, history-audit, gitleaks, owner-checkpoint, history-rewrite, public-repo, secret-scanning]
status: complete

requires:
  - phase: 01-04
    provides: "licence-check and the SPDX header convention that scripts/ files must follow"
  - phase: 01-11
    provides: "record 0001 (now owner-recorded: C11 + CMake + Ninja) and the local annotated tag spike-tool-01-evidence"
provides:
  - "scripts/audit-history [REPO]: full-history audit over every ref (exit 0 clean, 1 hits, 2 git or harness failure)"
  - "scripts/audit-history.allow: committed (path, CHECK, reason) allowlist, two NUL-free 16384-byte ASCII boundary controls"
  - "tests/audit/synthetic.sh: 10 synthetic-repository negative controls proving the audit is live"
  - "Public repository https://github.com/szTheory/duospool with main (8ba0bc6) and the tag spike-tool-01-evidence, secret scanning and push protection on"
  - "Pre-publication history rewrite: owner home paths replaced by [repo-root] and [home] across all 64 commits, with record 0001 re-cited"
affects: [phase-2, record-0004-visibility, CI-01]

tech-stack:
  added: []
  patterns:
    - "Every history read in the audit writes to a temp file under a trap-cleaned mktemp directory and checks its exit status before any clean/hit decision, so a failed read exits 2 and never prints clean"
    - "TEXT-PATTERN literals are built so their own source text cannot match them (self-scan control)"

key-files:
  created:
    - scripts/audit-history
    - scripts/audit-history.allow
    - tests/audit/synthetic.sh
    - .gitleaksignore
  modified:
    - docs/decisions/0001-language-build.md

key-decisions:
  - "Owner chose rewrite-history at the Task 2 checkpoint (verbatim selection: \"Rewrite history first\"), then approved publication (\"Yes, publish now\")"
  - "One gitleaks false positive (generic-api-key on a test-case line in 01-05-PLAN.md) allowlisted by fingerprint in .gitleaksignore, owner-approved"
  - "Pre-rewrite history kept only as a bundle backup outside the repository"

patterns-established:
  - "A one-way publication step is preceded by an audit whose own liveness is proven by synthetic positive and negative controls"

requirements-completed: []  # TOOL-01 was already marked complete after the owner's language decision (record 0001, owner-recorded); not marked again

duration: n/a (Task 1 by the plan executor; Tasks 2 and 3 interactive with the owner and the orchestrator)
completed: 2026-11-09

actuals:
  tokens: 4000    # chars/4 over the realized diff (audit-history, allow, synthetic.sh about 15165 chars, plus the record 0001 note and .gitleaksignore)
  tasks: 3
  commits: 3      # measured from the rewritten base; see the note under Task Commits
plan_head_before: 294bd6828a4919fc62c62975f52630529e7c4752
plan_head_after: 8ba0bc6d2b5ee89116e09a13b12590f19cc14daf

coverage:
  - id: D1
    description: "The audit is proven live by synthetic controls, including a failed history read that exits 2 and never prints clean"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh tests/audit/synthetic.sh prints AUDITTEST PASS for rom-blob, missing-object, home-path, allowlisted-ok, allowlisted-binary, clean, self-scan, git-read-failure, dup-blob-path, tag-only (10 of 10)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The owner saw every flagged item before the one-way action and chose; the choice was not auto-selected"
    requirement: TOOL-01
    verification:
      - kind: manual
        ref: "Task 2 checkpoint (gate blocking-human) answered by the owner on 2026-11-09: rewrite-history, then publication approved"
        status: pass
    human_judgment: true
  - id: D3
    description: "After the rewrite the audit is clean and gitleaks reports no leaks"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh scripts/audit-history . printed audit-history: clean (66 commits, 506 blobs); gitleaks reported no leaks with the owner-approved .gitleaksignore fingerprint"
        status: pass
    human_judgment: false
  - id: D4
    description: "Repository is public, remote main equals local main, the evidence tag is on origin, secret scanning enabled"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "gh repo view szTheory/duospool --json visibility prints PUBLIC; remote main == local main == 8ba0bc6; tag spike-tool-01-evidence on origin; gh repo edit --enable-secret-scanning --enable-secret-scanning-push-protection exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "Record 0001 citation of the protocol commit and the evidence tag survive the rewrite through a dated note"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "Protocol commit now 62ad050 (was aea02af); tag spike-tool-01-evidence now commit 1db3100, tag object b605e93; commit 4d109cd; sh scripts/check passes"
        status: pass
    human_judgment: false

self_check: PASSED
---

# Phase 1 Plan 12: Full-history audit, owner checkpoint and public repository Summary

**A history audit proven live against ten synthetic repositories found five home-path text hits and one gitleaks false positive; the owner chose to rewrite history first, and after a clean re-audit szTheory/duospool was published public with main, the evidence tag and secret scanning.**

## Performance

- **Tasks:** 3 of 3 (Task 1 tracer by the plan executor; Task 2 owner checkpoint; Task 3 carried out by the orchestrator with the owner's approval)
- **Date:** 2026-11-09
- **Files:** 3 scripts created, 1 gitleaks allowlist created, record 0001 amended with a dated note

## Task 1: Audit script, allowlist and synthetic controls (tracer)

- `scripts/audit-history` scans every blob reachable from every ref, tags included, using `git rev-list --objects --all` and `git cat-file --batch-check` with `%(rest)`. It flags NUL-BYTES, ROM-SIZE-8K, ROM-SIZE-HDR, EXT, TEXT-PATTERN and UNREADABLE. All history reads go through temp files with their exit status checked; a read failure exits 2 before any clean decision.
- `scripts/audit-history.allow` holds only the two NUL-free 16384-byte ASCII boundary controls, with SPDX lines in the first five lines so `licence-check` stays green.
- `tests/audit/synthetic.sh` runs 10 controls (the plan asked for 8): rom-blob, missing-object, home-path, allowlisted-ok, allowlisted-binary, clean, self-scan, git-read-failure, plus dup-blob-path and tag-only. All print `AUDITTEST <case> PASS`.
- Commit `b383521` (rewritten hash of the original `f3d92a3`).

## Task 2: Owner checkpoint (2026-11-09)

The initial audit of the real repository found:

- 5 TEXT-PATTERN hits for the owner's absolute home path, all prose: `.planning/research/ARCHITECTURE.md`, `01-CONTEXT.md`, `01-RESEARCH.md` (two lines) and `01-VALIDATION.md`.
- 1 gitleaks false positive: `generic-api-key` on a test-case line in `01-05-PLAN.md` (a prior result recorded as owner-recorded-preauth).
- No secrets, credentials or ROM-like blobs.

The owner chose **rewrite-history** (verbatim selection: "Rewrite history first"). Per the plan, the rewrite option was not improvised inside Task 3's automated flow: the procedure was run by the orchestrator with the owner present.

## Task 3: Rewrite, re-audit and publication (orchestrator, owner approval "Yes, publish now")

| Step | Outcome |
|---|---|
| (a) Backup | Bundle of the pre-rewrite history kept outside the repository |
| (b) Rewrite | `git filter-repo --force --replace-text` replaced the owner's absolute repository path with `[repo-root]` and the home directory with `[home]` across all 64 commits. Every hash changed. Tag `spike-tool-01-evidence` was rewritten (commit 1db3100, tag object b605e93). The protocol commit is now 62ad050 (was aea02af). Record 0001's citation was updated with a dated note (commit 4d109cd) |
| (c) Re-audit | `sh scripts/audit-history .` printed `audit-history: clean` (66 commits, 506 blobs). gitleaks found 1 false positive, allowlisted via `.gitleaksignore` with fingerprint `04fff180...:01-05-PLAN.md:generic-api-key:304` (owner-approved, commit 8ba0bc6); gitleaks then reported no leaks. `sh scripts/check` PASS |
| (d) Branch | `main` fast-forwarded to 8ba0bc6 |
| (e) Create | `gh repo create szTheory/duospool --public --source=. --remote=origin --description "PC Engine / TurboGrafx-16 emulator core (working name; not trademark-cleared)"` succeeded (no `--license`, no `--push`) |
| (f) Push | First `git push -u origin main` failed (`send-pack: unexpected disconnect while reading sideband packet`). The tag push then succeeded. A second `git push -u origin main` succeeded. See Deviations |
| (g) Settings | `gh repo edit --enable-secret-scanning --enable-secret-scanning-push-protection` exit 0. Visibility verified PUBLIC. Remote main equals local main (8ba0bc6) |

## Task Commits

1. **Task 1 (tracer): audit script, allowlist, synthetic controls** - `b383521` (feat)
2. **Task 3: record 0001 re-cited after the rewrite** - `4d109cd` (docs)
3. **Task 3: gitleaks false-positive allowlist** - `8ba0bc6` (chore)

Note on measurement: the ledger file `gsd-plan-head-before-01-12` holds `708ef4a` (the pre-rewrite base), which no longer exists after `git filter-repo`. The measured base is its rewritten equivalent, `294bd68` (the parent of `b383521`, the record of the owner's language decision), and `git rev-list --count 294bd68..HEAD` is 3. The frontmatter uses that base. Task 2 produced no commit.

## Deviations from Plan

### Owner-directed

- **Task 3 rewrite-history branch.** The plan says to stop and report for this option rather than improvise it. It was stopped and reviewed, then performed by the orchestrator with the owner's approval and the pre-rewrite bundle kept. Consequence: all hashes changed, handled by the record 0001 dated note and tag re-creation.
- **Extra allowlist file.** `.gitleaksignore` (not in `files_modified`) was added for the single owner-approved gitleaks false positive.

### Plan details adjusted in Task 1 (documented in the checkpoint report)

- `rev-list --missing=print` used so deleted objects surface as UNREADABLE.
- The audit tracks (blob, path) pairs per commit via `ls-tree`, so an allowlisted path that later changes content is re-examined.
- An `--allow FILE` option was added to select the allowlist (used by the controls).
- Two controls beyond the plan: dup-blob-path and tag-only.

### D-33 push order inverted once

D-33 requires main first, then the tag. The first main push failed on a transport error, the tag push succeeded, then the second main push succeeded, so the tag reached the remote before main. Harmless: both were audited, the end state is identical, and nothing was ever reachable on the remote that was not audited clean.

No Rule 1-3 auto-fixes beyond the above. Authentication gates: none.

## Issues Encountered

- Transport failure on the first main push (above); resolved by retry.

## Known Stubs

None.

## Threat Flags

None new. T-01-25 mitigated (audit over all refs, gitleaks second opinion, blocking-human checkpoint before the one-way step). T-01-26 mitigated (10 synthetic controls including git-read-failure exit 2 and self-scan). T-01-27 mitigated (no TEXT-PATTERN, EXT or NUL-BYTES allowlist entry; the history was scrubbed instead; the one gitleaks exception is a fingerprint the owner approved). T-01-28 mitigated (no `--license`, explicit pushes, secret scanning and push protection enabled, visibility PUBLIC verified).

## User Setup Required

None. The repository is public at https://github.com/szTheory/duospool. The pre-rewrite bundle backup sits outside the repository and should be kept private and not published.

## Next Phase Readiness

- Phase 1 plans are 12 of 12. Record 0001 is owner-recorded (C11 + CMake + Ninja), so Phase 2 is unblocked.
- The repository exists, so Phase 2's CI-01 (GitHub Actions) can run. FIX-05 builds the tree-level repository check; `scripts/audit-history` is intentionally not in `scripts/check`.

## Self-Check: PASSED

- `scripts/audit-history`, `scripts/audit-history.allow`, `tests/audit/synthetic.sh`, `.gitleaksignore` exist.
- Commits `b383521`, `4d109cd`, `8ba0bc6` are ancestors of HEAD; tag `spike-tool-01-evidence` is annotated and resolves to commit `1db3100`.
- `sh tests/audit/synthetic.sh` prints 10 PASS lines.
