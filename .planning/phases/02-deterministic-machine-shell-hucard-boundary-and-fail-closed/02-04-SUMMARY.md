---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
plan: 04
subsystem: testing
tags: [sh, awk, perl, gate, rights-check, default-deny, audit-history, licence-check, negative-controls, selftest-mutation]

requires:
  - phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
    provides: run_controls, --stage and --selftest NAME in scripts/check (02-01); outcomes.def (02-02); preflight, outcome-sync and the five mini-root controls (02-03)
provides:
  - scripts/rights-check, a default-deny content scan of the working tree (FIX-05, D-56, D-76) with a manifest-only allowlist
  - scripts/lib/rights-rules.awk, the line rules (LINELEN, BLOB, bzip2 header) that apply to text files too
  - tests/lib/gitfix.sh, throwaway git repository helpers shared by the audit and rights suites
  - tests/rights/synthetic.sh, fourteen run-time controls (eleven negatives, three positives)
  - scripts/audit-history --range BASE..HEAD for PR checks, with four new run-time controls
  - licence-check scope extended to c, h and def files under src and tests, and to fixtures (0BSD)
  - stages rights-tree, rights-negatives, audit-synthetic (26 stages in all); selftest mutation-rights-rule
affects: [02-05 and later (every new c, h or def file under src or tests must carry the SPDX header from its first gate run), 02-06, the fixture-manifest plan (fixtures/manifest/*.toml is now the rights allowlist and must carry 0BSD headers)]

actuals:
  tokens: 19600    # chars/4 over the realized diff (git diff 28b256f..HEAD excluding .planning, 78588 chars, 25 files, 1280 added lines)
  tasks: 3
  commits: 4       # measured: git rev-list --count plan_head_before..HEAD (before this SUMMARY commit)
plan_head_before: 28b256fa3b4879cfe4bfd4e011035114fa144038
plan_head_after: c9a3fc6da866af2800d33565f1bcacb752448e4d
commits: 4

tech-stack:
  added: []
  patterns:
    - "a bulk text classifier that must see NUL bytes uses perl, not grep: BSD grep ends a line at a NUL and reports a file holding one as clean"
    - "a rule whose removal must flip a control carries a single-line trailing marker (rule-extension on the one awk rule line of rights-check); the selftest assembles the token at run time"
    - "a control that depends on a manifest or an allowlist is shown red first: ok-listed-authored runs the same tree without its manifest entry and requires exit 1 before it requires exit 0"
    - "the per-tree expected SPDX id is a parameter of one scan function (spdx_scan LIST ID), so code and fixtures share the rule"

key-files:
  created:
    - scripts/rights-check
    - scripts/lib/rights-rules.awk
    - tests/lib/gitfix.sh
    - tests/rights/synthetic.sh
    - tests/licence/src-no-spdx (EXPECT, LICENSE-*, scripts/stub.sh, src/core/x.c)
    - tests/licence/ok-src (LICENSE-*, scripts/stub.sh, src/core/x.c)
  modified:
    - scripts/audit-history
    - scripts/licence-check
    - scripts/check
    - tests/audit/synthetic.sh
    - tests/provenance/path-missing/src/other.c
    - tests/gate/outcome-sync/*/tests/conformance/outcomes.def (header id only, five files)
    - .planning/phases/02-.../02-01-SUMMARY.md and 02-02-SUMMARY.md (absolute home path removed)

key-decisions:
  - "the line rules BLOB and LINELEN, and every other rule except SHAPE, apply to every file; SHAPE applies only to a file that is not valid text (D-76); no entropy rule; no use of .gitignore as the gate"
  - "BLOB is a run of 1024 or more base64 or hex characters anywhere in a line, not only a line made wholly of them, so a prefix or suffix cannot hide it"
  - "archive magics are the real file headers (PK 03 04, PK 05 06, PK 07 08, 7z BC AF 27 1C, Rar! 1A 07, gzip 1F 8B, BZh and a digit), not the bare two letters PK, which would flag prose that starts with those letters"
  - "text classification is one perl process per batch; the per-file path (tr, wc, iconv, od, awk) runs only for files that are not plain printable ASCII, so the real tree scans in about 1 s"
  - "a path holding a TAB or a newline is reported as BAD_FILENAME, the code the other checkers use, rather than skipped"
  - "an allowlist excuse never applies to a file holding a NUL byte; an invalid, stale or non-authored entry is itself a violation and excuses nothing"
  - "the full-history audit is left red on two pre-existing hits and is not allowlisted: it is a pre-release owner decision (see Deferred Issues)"

patterns-established:
  - "range mode of audit-history reads only what BASE..HEAD adds, and matches text patterns against new blobs, so old text is not re-reported"
  - "RIGHTS_CHECK selects the checker under test, so the mutation selftest runs the whole suite against a mutated copy"

requirements-completed: [FIX-05]

coverage:
  - id: D1
    description: "Every file that is not valid text is denied by default regardless of .gitignore, and the shape rule applies to exactly those files; a ROM-like file under any name is flagged"
    requirement: FIX-05
    verification:
      - kind: integration
        ref: "sh scripts/check --stage rights-negatives -> CHECK rights-negatives PASS (random-8192-no-extension, txt-with-rom-bytes, fill-ea-no-nul, shaped-ascii-high-byte each exit 1 with ROMLIKE_BINARY and the named reason keywords)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Valid text is exempt from the shape rule only: the blob, line-length, extension, magic and allowlist rules still apply to it, and a 16384-byte printable-ASCII file, UTF-8 prose and a listed authored fixture are accepted"
    requirement: FIX-05
    verification:
      - kind: integration
        ref: "tests/rights/synthetic.sh: hex-blob, long-line, zip-magic red; ok-ascii-16384, ok-utf8-prose, ok-listed-authored green; sh scripts/check --stage rights-tree PASS on the real tree (the two Phase 1 16384-byte ASCII files stay clean)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The only allowlist is a fixture manifest entry whose path exists, whose sha256 matches, whose kind is authored and whose licence is an SPDX id; a wrong hash, a missing file and a synthetic kind are rejected"
    requirement: FIX-05
    verification:
      - kind: integration
        ref: "tests/rights/synthetic.sh: listed-wrong-hash (RIGHTS_HASH_MISMATCH), manifest-no-file (RIGHTS_MISSING_ARTIFACT), listed-kind-synthetic (RIGHTS_BAD_ALLOW) each met with an exact code set"
        status: pass
    human_judgment: false
  - id: D4
    description: "Each rule is proven by a control that goes red when the rule is removed, and the extension rule is mutation-proven in the gate"
    requirement: FIX-05
    verification:
      - kind: unit
        ref: "sh scripts/check --selftest mutation-rights-rule -> PASS (planted-pce flips to FAIL with the one marked line deleted); a manual sweep deleting each other rule line (shape, utf8, nul, linelen, blob, magic, nonascii, hash, missing-artifact, bad-allow) turned the matching control red"
        status: pass
    human_judgment: false
  - id: D5
    description: "audit-history has a --range BASE..HEAD mode with run-time controls in the gate"
    requirement: FIX-05
    verification:
      - kind: integration
        ref: "sh scripts/check --stage audit-synthetic -> PASS; AUDITTEST range-clean, range-dirty, range-base-missing, range-text PASS; usage errors and an empty range exit 2"
        status: pass
    human_judgment: false
  - id: D6
    description: "licence-check also requires SPDX headers on c, h and def files under src and tests, and 0BSD on fixtures, with a red control"
    requirement: FIX-05
    verification:
      - kind: integration
        ref: "sh scripts/check --stage licences and --stage licence-negatives PASS (src-no-spdx red with SPDX_HEADER_MISSING, ok-src green); removing the header from a copy of tests/conformance/outcomes.def made licence-check exit 1 with SPDX_HEADER_MISSING, restored copy exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "Residual risks are recorded, not hidden: detection not prevention; add-then-delete still publishes a blob via refs/pull/N/head; a ROM re-encoded into short-line printable text evades the shape rule"
    requirement: FIX-05
    verification:
      - kind: manual
        ref: "usage comment of scripts/rights-check and this SUMMARY"
        status: pass
    human_judgment: true
    rationale: "A recorded residual risk is a statement of accepted exposure; whether the exposure is acceptable before publication is an owner decision"

duration: 23 min
completed: 2026-11-10
status: complete
---

# Phase 2 Plan 04: Rights check, audit --range and licence-header scope Summary

**`scripts/rights-check` denies any non-text file by content (NUL, control bytes, invalid UTF-8, non-ASCII ratio, archive magic, shape for non-text only, base64 or hex blobs, over-long lines, ROM-like extensions), allows only a verified authored fixture-manifest entry, and has fourteen run-time controls plus a mutation selftest; `audit-history --range BASE..HEAD` audits only what a PR adds; `licence-check` now guards `src/`, `tests/` C files and `fixtures/` from the first gate run.**

## Performance

- **Duration:** 23 min (2026-11-10T03:56:52Z to about 04:20Z)
- **Tasks:** 3 of 3 (Task 1 is the tracer)
- **Files:** 25 changed outside .planning (1280 added lines, 60 removed)
- **Gate cost (this machine, under load, noisy):** `sh scripts/check --timing` 80 s wall. The three new stages: rights-tree 3 s, rights-negatives 9 s, audit-synthetic 10 s. `scripts/rights-check` alone is about 1 to 2 s on the 519-file tree. protocol-negatives alone took 15 s in the same run, so the whole run was slower than the 17 s of plan 03; time is a record, never a failure (D-60).

## Accomplishments

- **rights-check (D-56, D-76).** Enumerates `git ls-files -co --exclude-standard -z` with the host git configuration switched off, so a tracked file that is also ignored is still scanned. A name rule (`rule-extension`) and content rules emit `ROMLIKE_BINARY` with a reason keyword (NUL, CTRL, UTF8, NONASCII, EXT, MAGIC, SHAPE, BLOB, LINELEN). One perl process per batch finds the files that are not plain printable ASCII; only those take the per-file path (`tr`, `wc`, `iconv`, `od`, `awk`). The shape rule runs only for a file that failed the valid-text test. File content is never put in a shell variable.
- **Allowlist.** `fixtures/manifest/*.toml` (read with `toml-subset.awk`, stage 1, no pipeline; a missing directory is an empty allowlist). An entry with `redistribution = committed` excuses its source path only while the path exists (`RIGHTS_MISSING_ARTIFACT`), the sha256 matches (`RIGHTS_HASH_MISMATCH`), the kind is `authored` and the licence is an SPDX-shaped id (`RIGHTS_BAD_ALLOW`); a failing entry excuses nothing. An excuse never applies to a file holding a NUL byte (checked by hand: a listed file with a NUL stays flagged). An unparseable entry, a `..` or absolute source path are `RIGHTS_BAD_ALLOW`.
- **Controls.** `tests/rights/synthetic.sh` prints fourteen `RIGHTSTEST ... PASS` lines: planted-pce, random-8192-no-extension, txt-with-rom-bytes, fill-ea-no-nul, shaped-ascii-high-byte, zip-magic, hex-blob, long-line, listed-wrong-hash, manifest-no-file, listed-kind-synthetic (negatives, each met with an exact code set through `--expect` and required to exit 1 without it, with the reason keyword asserted where one is expected) and ok-listed-authored, ok-utf8-prose, ok-ascii-16384 (positives). Every planted file lives only in a temporary repository staged with `git add -f`; nothing ROM-like is committed.
- **mutation-rights-rule.** Shows the unmutated copy flags a one-file repository holding a printable `x.pce`, deletes the single marked extension-rule line (marker count 1 before, 0 after), and requires the suite against the copy to exit non-zero and print `RIGHTSTEST planted-pce FAIL`.
- **audit-history --range.** `git rev-list BASE..HEAD` for commits and `git rev-list --objects --missing=print BASE..HEAD` for objects, so only new blobs are read; text patterns are matched against new text blobs (`grep` per blob and path) instead of every commit's whole tree, so old text is not reported again. The form must be `BASE..HEAD` (an option, a three-dot form or an open end is a usage error, exit 2); an unresolvable BASE (`readfail`) or an empty range exits 2, never clean. The shallow refusal applies to full-history mode only. The full-history mode is unchanged.
- **licence-check scope (D-71).** One `spdx_scan LIST ID` function now serves `scripts/` (all files), `src/` and `tests/` files named `*.c`, `*.h`, `*.def` (with `tests/licence/` excluded and the exclusion stated in the usage comment) at MIT OR Apache-2.0, and `fixtures/` (all files) at 0BSD. New controls `tests/licence/src-no-spdx` (red, `SPDX_HEADER_MISSING`) and `tests/licence/ok-src` (green).

## Task Commits

1. **Task 1 (tracer): rights-check with the planted-control** - `8aefa5c` (feat)
2. **Task 2: full rule set, manifest allowlist, fourteen controls, mutation selftest** - `e0763f6` (feat)
3. **Task 3: audit-history --range, audit-synthetic stage, licence-check scope** - `cc02f3f` (feat)
4. **Out-of-task hygiene: absolute home paths removed from the 02-01 and 02-02 summaries** - `c9a3fc6` (docs)

## Verification Results

Tracer gate (auto mode): after the Task 1 commit the tracer verify was re-run (`sh scripts/check --stage rights-tree && sh scripts/check --stage rights-negatives && sh tests/audit/synthetic.sh`, exit 0, 11 AUDITTEST PASS lines as before the refactor); log line `Tracer verified end-to-end - expanding`.

Per-task `<automated>` commands:

| Command | Result |
|---------|--------|
| `sh scripts/check --stage rights-tree && sh scripts/check --stage rights-negatives && sh tests/audit/synthetic.sh` (Task 1) | exit 0; `CHECK rights-tree PASS`, `CHECK rights-negatives PASS`, 11 `AUDITTEST ... PASS` (baseline 11, recorded before the refactor) |
| `sh tests/rights/synthetic.sh && sh scripts/check --selftest mutation-rights-rule` (Task 2) | exit 0; 14 `RIGHTSTEST ... PASS`, `SELFTEST mutation-rights-rule PASS` (also `dash tests/rights/synthetic.sh` 14 PASS) |
| `sh tests/audit/synthetic.sh && sh scripts/check --stage licences && sh scripts/check --stage licence-negatives && sh scripts/check --stage provenance-negatives && sh scripts/check --stage audit-synthetic` (Task 3) | exit 0; 15 AUDITTEST PASS including `range-clean`, `range-dirty`, `range-base-missing`; all four stages PASS |

Acceptance criteria, all run: `sh scripts/rights-check` on the real repository prints `RECORDS 519` (above 300, baseline 396 at planning time; 517 when first run, the rest are this plan's files) and exits 0; `git ls-files -- '*.pce' '*.sgx' '*.bin' '*.rom' '*.iso' '*.cue' '*.chd' '*.sav' '*.srm'` prints nothing; `grep -c rule-extension scripts/rights-check` is 1; SPDX `MIT OR Apache-2.0` in the first five lines of rights-check, rights-rules.awk, gitfix.sh and tests/rights/synthetic.sh; the AUDITTEST PASS count before the refactor equals after (11); stage total 26 (baseline 18 at phase start); a scratch copy of `tests/conformance/outcomes.def` with its SPDX lines removed makes `licence-check --root SCRATCH` exit 1 with `SPDX_HEADER_MISSING`, restored exit 0; `--range 28b256f..HEAD` of this repository is clean.

Plan-end (wave-end) commands (D-81):

| Command | Result |
|---------|--------|
| `sh scripts/check` | exit 0, 26 `CHECK ... PASS` stage lines and `CHECK RESULT PASS` |
| `sh scripts/check --selftest` | eleven `SELFTEST ... PASS` lines, `SELFTEST RESULT PASS`, exit 0 |
| `dash scripts/check` | `CHECK RESULT PASS`, exit 0 |
| `sh scripts/audit-history` (full history) | **exit 1, two pre-existing TEXT-PATTERN hits, see Deferred Issues**; identical hits and exit status with the pre-plan script (`git show HEAD:scripts/audit-history`) |

No process from the cancel stub of plan 03 is left after any run (an unrelated `sleep 600` from another session exists on this machine and is not the gate's).

## Files Created/Modified

- `scripts/rights-check`, `scripts/lib/rights-rules.awk` - the checker and its line rules.
- `tests/lib/gitfix.sh`, `tests/rights/synthetic.sh` - shared helpers and the fourteen controls.
- `scripts/audit-history`, `tests/audit/synthetic.sh` - range mode and its four controls.
- `scripts/licence-check`, `tests/licence/src-no-spdx`, `tests/licence/ok-src` - extended scope and controls.
- `scripts/check` - stages rights-tree, rights-negatives, audit-synthetic; selftest (k) mutation-rights-rule.
- `tests/provenance/path-missing/src/other.c`, five `tests/gate/outcome-sync/*/tests/conformance/outcomes.def` - headers.

## Decisions Made

See `key-decisions`. The plan fixed the codes, markers, CLI and the D-76 scope; the choices left open were how to classify text in bulk, the exact magic signatures, whether BLOB is whole-line or in-line, the handling of unusual file names, and how range mode scans text.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] planted-pce could not flip under the extension mutation as the plan wrote it**
- **Found during:** Task 2
- **Issue:** The plan makes planted-pce an 8192-byte zero-filled `x.pce` and the mutation deletes only the extension rule. A zero-filled file is also caught by the NUL and shape rules, so the control would stay green with the rule removed and `mutation-rights-rule` could never pass.
- **Fix:** In Task 2 planted-pce is a short printable text file whose only fault is its name (the tracer commit keeps the zero-filled form). The NUL path is covered by random-8192-no-extension (asserts the NUL reason) and by `audit-history`'s own rom-blob case.
- **Files modified:** tests/rights/synthetic.sh
- **Commit:** e0763f6

**2. [Rule 3 - Blocking] bulk text classification with grep was unsound**
- **Found during:** Task 1 (own sanity check after the first green run)
- **Issue:** BSD grep ends a line at a NUL, so `grep -a -l -v` called a text file containing one NUL clean. The tracer controls had not noticed because planted-pce was also caught by name.
- **Fix:** one perl process per batch classifies files (perl reads bytes). perl is already a declared gate tool in `scripts/preflight`. Verified by hand on a one-NUL file, a trailing-NUL file and a DEL file.
- **Files modified:** scripts/rights-check
- **Commit:** 8aefa5c

**3. [Rule 3 - Blocking] the extended licence rule flagged files created by earlier plans**
- **Found during:** Task 3
- **Issue:** The plan expected only `tests/provenance/path-missing/src/other.c` to be headerless, but the five `tests/gate/outcome-sync/*/tests/conformance/outcomes.def` mini-root files from plan 03 carry `0BSD`, while the new rule requires MIT OR Apache-2.0 for def files under `tests/` (D-71).
- **Fix:** the first line of those five files now names `MIT OR Apache-2.0` (the content rows are untouched, so outcome-sync and its mutation selftest still pass). `tests/provenance/path-missing/src/other.c` gained the two SPDX lines as planned; provenance-negatives is unchanged.
- **Files modified:** the five outcomes.def files, tests/provenance/path-missing/src/other.c
- **Commit:** cc02f3f

**4. [Rule 2 - Missing critical] absolute home paths in two earlier summaries**
- **Found during:** Task 3 verification
- **Issue:** `scripts/audit-history` reported `home-path-mac` in `02-01-SUMMARY.md` and `02-02-SUMMARY.md`, which list absolute paths of the owner's checkout. The hits predate this plan (same result with the pre-plan script).
- **Fix:** the two lines in the working tree now use repository-relative paths. History cannot be changed (no rewrite); see Deferred Issues.
- **Files modified:** 02-01-SUMMARY.md, 02-02-SUMMARY.md
- **Commit:** c9a3fc6

### Additions and interpretation choices (inside the plan's intent)

1. **BAD_FILENAME** for a path with a TAB or a newline (the code the other checkers use); the plan did not name this case. A newline is detected by comparing the NUL-separated entry count with the line count. Checked by hand, not by one of the fourteen controls (the plan fixes the number at fourteen).
2. **MAGIC signatures** are the real headers rather than the bare letters PK (see key-decisions); **BLOB** is an in-line run, a superset of "a line of 1024 or more characters".
3. **ok-listed-authored** also requires the same tree to be rejected without its manifest entry, and **range-text** is a fourth range control beyond the three the plan names.
4. **Selftest alive check:** the unmutated copy is shown to flag a one-file repository before the mutation, so a red result cannot come from a copy that was broken some other way.
5. The host's git configuration is disabled for the scan (`GIT_CONFIG_GLOBAL=/dev/null`), so a developer's global excludes file cannot hide an untracked file.

**Total deviations:** 4 auto-fixed (1 Rule 1, 2 Rule 3, 1 Rule 2), 5 interpretation choices. **Impact:** none on scope or on later plans' contracts, except that later plans' `.def`, `.c` and `.h` files under `tests/` and `src/` must carry the MIT OR Apache-2.0 header from their first gate run.

## Deferred Issues

- **Full-history audit is red on two hits that pre-date this plan.** `sh scripts/audit-history` exits 1: `home-path-mac` in `.planning/phases/02-.../02-01-SUMMARY.md` (line 181) and `02-02-SUMMARY.md` (line 203), present in the history of those summary commits. Not suppressed through `scripts/audit-history.allow` on purpose: an allowlist line would hide a finding the owner decides on before publication. The PR range mode is clean for this plan's commits (`--range 28b256f..HEAD`). Options for the owner at the publication checkpoint: squash or rewrite the unpublished history, or allowlist the two paths with a recorded reason. Recorded in `.planning/WINDOWS.md` as an open unmet-truth entry, so `/gsd-ship` shows it.

## Known Stubs

None.

## Notes for later plans

- Residual risks (T-02-17, accepted): the check is detection, not prevention. A ROM re-encoded into printable text with short lines (for example base64 wrapped at 76 columns) evades the shape, line-length and blob rules (D-76). An add-then-delete commit still publishes the blob through `refs/pull/N/head`; `audit-history --range` and the full-history audit are the pre-release checks.
- The rights allowlist source is `fixtures/manifest/*.toml`. The fixture-manifest plan should write entries with `redistribution = committed` only for files that really are committed; entries for generated fixtures use `generated-not-committed` and excuse nothing. Files under `fixtures/` need the 0BSD header from their first gate run (manifest entries are TOML comments).
- `scripts/check` stages now run `tests/rights/synthetic.sh` and `tests/audit/synthetic.sh` (about 9 and 10 s here, two checker runs per control). If the PR gate budget tightens, the cheapest saving is dropping the bare run of `negative` for cases whose exit status another control already proves.
- The iconv UTF-8 test and the perl and awk steps were run only on macOS (BWK awk 20200816, BSD tr, Apple iconv, dash, bash 3.2 as sh). That glibc iconv and mawk give identical verdicts on Ubuntu is for the first CI run to show (D-76 asks for identical verdicts); it is not proven here.
- `rights-check` needs perl, iconv, xargs and od; `perl` and `iconv` are in the preflight list, `xargs` and `od` are not and could be added.
- `tests/licence/` is excluded from the C and def scan by design; any control directory elsewhere under `tests/` (for example `tests/gate/`) carries real headers.

## Threat Flags

None. The new surface is a local read-only scan and a local git read; no network, no remote write, no new parsing of untrusted content beyond bytes counted by tr, wc, iconv, od and awk (the content boundary this check exists to guard).

## Self-Check: PASSED

- Files exist: scripts/rights-check, scripts/lib/rights-rules.awk, tests/lib/gitfix.sh, tests/rights/synthetic.sh, tests/licence/src-no-spdx/src/core/x.c, tests/licence/ok-src/src/core/x.c (checked with `[ -f ]`).
- Commits `8aefa5c`, `e0763f6`, `cc02f3f`, `c9a3fc6` are ancestors of HEAD.
- All task acceptance criteria and plan-end commands re-run and passing as tabulated above, with the one documented exception (full-history audit, pre-existing hits).
