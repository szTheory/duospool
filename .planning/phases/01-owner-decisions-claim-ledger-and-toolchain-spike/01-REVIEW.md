---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
reviewed: 2026-11-09T00:00:00Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - scripts/check
  - scripts/ledger-check
  - scripts/decision-check
  - scripts/profile-check
  - scripts/provenance-check
  - scripts/licence-check
  - scripts/check-results
  - scripts/protocol-order-check
  - scripts/audit-history
  - scripts/audit-history.allow
  - scripts/lib/toml-subset.awk
  - scripts/lib/ledger-rules.awk
  - scripts/lib/profile-rules.awk
  - scripts/lib/provenance-register.awk
  - scripts/lib/score.awk
  - tests/audit/synthetic.sh
  - tests/protocol/synthetic.sh
findings:
  critical: 3
  warning: 12
  info: 4
  total: 19
  necessity: 0
status: issues_found
---

# Phase 1: Code Review Report

**Reviewed:** 2026-11-09
**Depth:** standard
**Files Reviewed:** 17 (including the two-line data file `scripts/audit-history.allow`)
**Status:** issues_found

## Summary

Baseline: `sh scripts/check`, `sh scripts/check --selftest`, `sh scripts/audit-history` and `sh tests/audit/synthetic.sh` all pass on this Mac. I also ran the whole gate, the selftest and the audit controls with `/bin/dash` shimmed in as `sh` (dash is present at `/bin/dash`): everything passes, so the dash question is answered by test, not by reading. gawk and mawk are not installed here and were NOT run; the gawk/mawk reading below is by source inspection only. I found no gawk/mawk/dash incompatibility in the awk or sh sources (no interval expressions in awk, `split(..., "|")` is a literal single-character separator in all three, `delete array` is accepted by all three, no `sed -i`, no `local`, no bashisms in `#!/bin/sh` scripts).

`score.awk` arithmetic and the D-37 margin rule are correct: integer hundredths (`weight x score10 x 2`, 100.00 points = 10000), nominal lead >= 1000, worst-case lead >= 1000 (leader's ND at 0, other option's ND at 5.0), leader must pass all five gates (ND gate counts as not-pass), precedence amended > tie > both-fail > leader-fails > margin > nd-sensitivity. I traced the 17 `tests/score` expectations by hand against it and found no arithmetic bug. The `supported` rules in `ledger-rules.awk` (E1/E2/E3 qualify, E0/E4 never, E1 needs a FACT source, PENDING test rejected, non-TEST pin rejected) match the spec.

The defects are in the fail-closed machinery around those rules. Three demonstrated fail-open paths (file-name field injection in two checkers, a non-zero git exit reading as PASS in the protocol-order check, and a vacuous "clean" on a shallow clone in the history audit), plus several places where a gate accepts a weaker value than its own documentation (D-26) or its negative controls can be emptied without the gate noticing.

No necessity-category findings (the Necessity rubric was read; nothing in scope guards a provably impossible state).

## Critical Issues

### CR-01: A file name containing TAB forges the checkers' internal record format and turns garbage into PASS (ledger-check, profile-check)

**File:** `scripts/ledger-check:72`, `scripts/ledger-check:76-93`, `scripts/profile-check:68-69`, `scripts/profile-check:96`
**Issue:** Both wrappers build a TAB-separated stream whose first column is the file name (`print ENVIRON["fpath"], $0` in ledger-check; `printf '%s\t%s...' "$f"` in profile-check) and then dispatch on column 2 as a tag (`V`/`F`/`I`/`S`/`R`/`B` in ledger-check, `P` in profile-check). A file whose name contains TABs shifts the columns, so the tag is attacker-chosen and every violation of that file is reclassified. Verified:

- `ledger-check` on a directory holding one file named `x<TAB>F<TAB>y<TAB>z.toml` containing the single line `garbage line` prints only `STATUS open=0 ...` and exits 0. The same content under a normal name gives MISSING_FIELD x9 and exit 1.
- `profile-check` on `profiles/pce-x/r1<TAB>P<TAB>x.toml` containing `garbage` prints `RECORDS profiles=19` and exits 0 (the violations are filtered out as `P` records and the record count is inflated).

The ledger, profile and decision files are hand-edited and treated as untrusted input; a tab in a name is invisible in a diff review. This defeats the project's central property (a gate that cannot read garbage as PASS). `decision-check` is safe only by accident (it decides on `-s "$viol"`, not on a tag column); `provenance-check` fails closed because the shifted id never resolves.

**Fix:** Reject names before they reach the stream, in the file loops, with a violation (not a skip):
```sh
check_name() {   # BAD_FILENAME for anything outside a plain name
  case $1 in
    *[!A-Za-z0-9._/-]*) printf '%s\tV\tBAD_FILENAME\t0\t\n' "bad-name" >> "$raw"; return 1 ;;
  esac
  return 0
}
```
(use a fixed placeholder as the first column, never the raw name, when the name itself is the problem), and apply the same check to the directory component in profile-check. Add a negative control that creates the tab-named file at run time under `mktemp -d` (git cannot be relied on to carry such a name on every host).

### CR-02: audit-history reports "clean" (exit 0) on a shallow clone, so the "full-history" audit is vacuous exactly where CI would run it

**File:** `scripts/audit-history:63-68`
**Issue:** The script's contract is a pre-publication audit of the full history, and it is explicit about the vacuity traps it closes (`%(rest)`, `--missing=print`, exit 2 on read failure, empty history). It does not close the shallow-clone trap. Verified: `git clone --depth 1 file:///...duospool`, then `sh scripts/audit-history <clone>` prints `scanned 1 commits, 312 blobs, 368 blob-path pairs`, `audit-history: clean`, exit 0. A default CI checkout is shallow (depth 1), and the only guard against an empty result is `ncommits -eq 0`. History that is cut off is silently treated as history that was audited.
**Fix:** Fail closed before enumerating:
```sh
if [ "$(git rev-parse --is-shallow-repository 2>/dev/null)" != false ]; then
  echo "audit-history: shallow or unreadable repository: the audit needs full history" >&2
  exit 2
fi
```
(`--is-shallow-repository` needs git 2.15 or newer; treat any output other than `false` as exit 2.) Add a synthetic case to `tests/audit/synthetic.sh` that audits a `--depth 1` clone of a two-commit repo and expects exit 2.

### CR-03: protocol-order-check turns a failing `git log` into "no spike files yet", so an ordering violation reads as PASS

**File:** `scripts/protocol-order-check:66`, `scripts/protocol-order-check:73`, `scripts/protocol-order-check:115`
**Issue:** `add_commit` ends in `| tail -n 1 || true`, the spike enumeration is `$(g log ... 2> /dev/null || true)`, and the amendment-introduction lookup does the same. Every git failure is converted into an empty value, and an empty `spike_adds` disables the ordering checks (`for s in $spike_adds`, `[ -n "$spike_adds" ]` around the scorer check) and `first_spike`. Verified with a synthetic repo in which a spike commit precedes the protocol commit: intact, the checker reports `PROTOCOL_AFTER_SPIKE` (exit 1). After deleting the loose `spikes` tree object, `git log -- spikes/tool-01` fails, and the checker prints `first_spike=none`, `RECORDS 1` and exits 0. A non-zero git exit read as PASS is the specific failure mode this gate exists to prevent. (Separately, `g show "$add:$proto"` at line 96 aborts under `set -e` with git's status 128 instead of the documented 2.)
**Fix:** Capture status explicitly and fail with 2:
```sh
spike_adds=$(g log --format=%H --diff-filter=A --no-renames --reverse -- "$spikes" 2> "$tmp/err") || {
  echo "HARNESS_GIT_FAILED log -- $spikes" >&2; cat "$tmp/err" >&2; exit 2; }
```
For `add_commit`, write `git log` to a file, test its status, then `tail -n 1` the file. Same for the `-S` lookup at line 115 and for `g show` (`|| exit 2`). Add a synthetic control that removes the `spikes` tree object and expects exit 2.

## Warnings

### WR-01: provenance-check accepts any whitespace-free `upstream-ref`, including a mutable branch name; the 40-hex branch is dead

**File:** `scripts/lib/provenance-register.awk:95-101`
**Issue:** D-26 requires `upstream-ref` to be a full SHA or a tag. The code sets `ok = 1` for a 40-hex string and, in the `else if`, for any string with no space or tab, so the first branch changes nothing. Verified: `upstream-ref: main` in an otherwise valid one-entry register gives `RECORDS 1`, exit 0. A mutable ref makes the recorded provenance unreproducible, which is the purpose of the field. The `bad-ref` control only covers a value with whitespace.
**Fix:** Require a 40-hex SHA, or an explicit tag form that cannot be a branch, e.g. `v?[0-9]+(\.[0-9]+)*` or a `refs/tags/` prefix; reject `main|master|HEAD|trunk|develop`. Add a `bad-ref-branch` control.

### WR-02: `license-spdx` validation checks each token against the allowlist but not the expression structure

**File:** `scripts/lib/provenance-register.awk:87-93`
**Issue:** The value is split on spaces and each token must be in the allowlist, and `OR` is itself in the allowlist. Verified: `license-spdx: OR` passes. `MIT Apache-2.0` (no operator), `OR MIT`, `MIT OR` and `OR OR` also pass. The licence gate for copied code (GPL excluded, D-26) is therefore weaker than its description.
**Fix:** Validate as `ID ( OR ID )*`: alternate tokens, odd positions must be licence ids (never `OR`), even positions must be exactly `OR`, and the count must be odd.

### WR-03: provenance marker scan prunes `build`, `target`, `out` and `prefix` at any depth, not only at the root

**File:** `scripts/provenance-check:124`
**Issue:** `find . \( -type d \( -name .git -o -name target -o -name build -o -name out -o -name prefix \) -prune \)` matches by name at every depth. The comment says "untracked output directories", which are root-level. Verified: a `// Provenance: PROV-0042` marker with no register entry, placed in `build/gen/hidden.c` or `crates/a/target/x.c`, produces `RECORDS 1` and exit 0. Real source directories named `build` or `out` (for example `crates/core/build/`) escape the gate.
**Fix:** Prune only the root-level output directories, and `.git` anywhere: `-path ./target -o -path ./build -o -path ./out -o -path ./prefix -o -name .git`.

### WR-04: marker grammar misses common comment leaders, so an unregistered marker in them is invisible

**File:** `scripts/provenance-check:52`
**Issue:** `marker_head` allows only `//`, `#`, `;`, `*`, `/*` followed by optional whitespace then `Provenance:`. `///`, `//!` (Rust doc comments, the likely core language), `##`, `--`, `%` and `<!--` do not match. Verified: `/// Provenance: PROV-0099` with no register entry gives `RECORDS 0`, exit 0. A registered entry whose only marker uses such a leader fails closed (`PROV_PATH_NO_MARKER`), but an unregistered one passes.
**Fix:** Allow repeated leader characters and the additional leaders, for example `^[[:space:]]*(//[/!]?|#+|;+|--|%|<!--|\*|/\*+)[[:space:]]*Provenance: `, and add a control for each style.

### WR-05: an empty `--expect` (or an empty EXPECT file) makes a negative control pass vacuously

**File:** `scripts/check:57`, `scripts/check:137`, `scripts/check:188`, `scripts/check:259`, `scripts/check:312`, `scripts/check:435`; every checker's `--expect` tail (e.g. `scripts/ledger-check:150-160`)
**Issue:** The harness reads `exp=$(cat EXPECT)` with no non-empty check, and the checkers normalise `--expect ""` to the empty set; when the directory under test has no violations, `got = want = ""` and the checker prints `EXPECT_OK`. Verified: `sh scripts/ledger-check --expect "" ledger/entries/` prints `EXPECT_OK`, exit 0. A negative control whose EXPECT file is empty (or truncated to nothing) therefore passes while asserting nothing, and the stage reports PASS.
**Fix:** In `scripts/check`, treat an empty `$exp` as `HARNESS_NO_EXPECT` (rc 2) in every stage; in the checkers, make `--expect` with an empty value a usage error (exit 2). Positive samples never need `--expect`.

### WR-06: an empty `ledger/REQUIRED` removes the coverage requirement and the stage still passes

**File:** `scripts/ledger-check:116-126`, `scripts/ledger-check:139-142`
**Issue:** The REQUIRED check iterates over the R records it read; zero R records means zero checks. Verified: `sh scripts/ledger-check --required <empty file> ledger/entries/` prints the STATUS line and exits 0. Deleting the lines of `ledger/REQUIRED` (or leaving only comments) disables the "these ten claims must exist with these origins" gate without any signal.
**Fix:** When `--required` is given, count R records and emit a violation (e.g. `REQUIRED_EMPTY`) if there are none. Optionally pin the expected count in `scripts/check`.

### WR-07: negative and sample controls are discovered by glob, so deleting a control or a rule is invisible to the default gate; `--selftest` is not part of it

**File:** `scripts/check:45-104`, `scripts/check:529-552`
**Issue:** Each `*_negatives` / `*_samples` stage requires only `n > 0` controls. Removing `ledger/negative/e0-e4/` (the control for the supported-evidence rule) or `tests/score/margin-9.90/` leaves the remaining controls passing and the stage green. The mutation proof that would notice a dead rule (`--selftest`) runs only when invoked by hand; the default path never calls it. The `provenance-negatives` stage has the same shape. The protocol stage (`scripts/check:339-346`) likewise accepts any run that prints one `SYNTH ... PASS` line and exits 0, not the full case count.
**Fix:** Pin the control inventory: a checked-in list of required control names per stage (one line each), compared against the directory listing, with `CONTROL_MISSING` as a failure. Have the default run fail if the count printed by `tests/protocol/synthetic.sh` differs from the pinned count. Wire `--selftest` into the Phase 2 gate (and state that in `scripts/check` until then).

### WR-08: selftest mutation (e) does not exercise `stage_score_controls`, and (a) accepts any exit 1

**File:** `scripts/check:494-504`, `scripts/check:468-469`
**Issue:** Mutations (a) to (d) run the real checker's `--expect` path against a mutated copy and require the broken-control exit (1). Mutation (e) instead runs the mutated `score.awk` directly and greps its output, so it proves only that the scorer's output changed, not that the gate stage that consumes `tests/score/margin-9.90/` would turn red; the stage could be dead and (e) would still PASS. Separately, (a) and (c) treat any exit status 1 as success: a mutated copy that fails for an unrelated reason that also exits 1 (for example a missing file reported as a violation) passes.
**Fix:** For (e), build the mutated `scripts/` copy as for the others and run the score-control logic against it (factor the loop in `stage_score_controls` into a function that takes the scorer path); require the specific output (`CONTROL BROKEN margin-9.90`). For (a) and (c), also require the expected code to appear in the mismatch output, not just exit 1.

### WR-09: audit-history's TEXT-PATTERN allowlist entry suppresses every secret kind in every commit for that path

**File:** `scripts/audit-history:177`, `scripts/audit-history:19`
**Issue:** Text hits are suppressed when `(path SUBSEP "TEXT-PATTERN") in al`; the kind (`private-key`, `aws-key`, `github-token`, `home-path-*`) and the commit are not part of the key. The header says "a hit is suppressed only when its path AND its CHECK match", but for text hits CHECK is the generic `TEXT-PATTERN`, so allowlisting a path for one home-path false positive also hides a private key committed to that path later or earlier. (No TEXT-PATTERN entries exist today; the exposure is the mechanism.)
**Fix:** Key text allowances on `path`, `TEXT-PATTERN`, and the kind (a fourth column), and keep the credential kinds (`private-key`, `aws-key`, `github-*`, `slack-token`) non-allowlistable.

### WR-10: the "full-history" audit never reads commit messages, tag messages or author metadata

**File:** `scripts/audit-history:63-97`, `scripts/audit-history:147-167`
**Issue:** The text scan is `git grep` over each commit's tree. Commit and tag message text (and author/committer identity strings) is never scanned. For a pre-publication audit whose own motivation was a home-directory path leak (the history was rewritten for exactly that), message text is the obvious residual channel; `--replace-text` rewrites blobs, not messages. A scan of messages is currently clean (`git log --all --format=%B | grep -c '/Users/\|/home/'` returns 0), so this is a missing capability, not a present leak.
**Fix:** Add `git log --all --format='%H%n%B'` and `git for-each-ref --format='%(contents)' refs/tags` passes through the same patterns, or state in the header that messages are out of scope so the audit does not claim more than it checks.

### WR-11: the scorer is "pre-registered" only by add-order; later edits to `score.awk` are not detected

**File:** `scripts/protocol-order-check:134-148`
**Issue:** The protocol gets an append-only guarantee (the add commit's bytes must remain an exact prefix, line 96-101). The scorer, which is what turns the pre-registered weights into the D-07 decision, gets only an ancestry check on its add commit. Editing `scripts/lib/score.awk` after the spike (a threshold, a weight, the margin comparison) leaves `SCORER_AFTER_SPIKE` silent, and the `tests/score` controls live in the same tree and can be edited with it. `git log -- scripts/lib/score.awk` currently shows a single commit (461eac5), so nothing has been altered, but nothing would flag it.
**Fix:** Pin the scorer: after the first spike commit exists, require `git diff <first_spike> HEAD -- scripts/lib/score.awk` (or the blob hash at the first spike commit) to be empty, with `SCORER_EDITED_AFTER_SPIKE`, and add the synthetic control (scorer modified after a spike commit).

### WR-12: the results gate does not tie the record's score block or `amendment` flag to the scorer or to protocol-order-check

**File:** `scripts/check:394-397`, `scripts/check-results:84-86`
**Issue:** `stage_results` verifies the metric table is complete and that a line starting `RESULT=` exists; it does not run `score.awk` over the record's scores block, compare the result with the "Scorer output" block, or compare `amendment` with the `amended_after_spike` value `protocol-order-check` prints. In `docs/decisions/0001-language-build.md:275-276` the comment says the flag is "not typed by hand" while the next line is hand-typed `amendment no`. The record is the evidence for a decision; the claim "the pre-registered scorer returned X" is currently carried by prose, not checked.
**Fix:** In `stage_results`, extract the fenced scores block and the scorer-output block from the record, run `score.awk` on the former and require the latter to equal its output byte for byte; obtain `amendment` from `protocol-order-check`'s info line (or fail if they differ). `RESULT=` should also be required to be one of the two legal values.

## Info

### IN-01: `audit-history` hard-codes a 40-character object name when splitting `git grep` output

**File:** `scripts/audit-history:156`
**Issue:** `s = substr($0, 42)` assumes `<40-hex>:`. In a SHA-256 repository the path would start inside the hash. The effect is a wrong path in the hit and a failed allowlist match (fail-closed), not a missed secret.
**Fix:** Split at the first `:` (`i = index($0, ":"); s = substr($0, i + 1)`).

### IN-02: `licence-check` depends on `shasum` with no fallback and reports a missing tool as a hash mismatch with an empty hash

**File:** `scripts/licence-check:63`
**Issue:** `shasum` is Perl-provided; minimal Linux images ship `sha256sum` only. With `shasum` absent the pipeline yields an empty string and the user sees `LICENSE_HASH_MISMATCH` with a blank detail, which looks like a tampered licence. Fail-closed, but misleading.
**Fix:** Use `sha256sum` when present, `shasum -a 256` otherwise, and exit 2 (`HARNESS_NO_TOOL`) when neither exists or the hash is empty.

### IN-03: `tests/audit/synthetic.sh` is not run by `scripts/check`, and `has EXT` matches `TEXT-PATTERN`

**File:** `tests/audit/synthetic.sh:78`, `tests/audit/synthetic.sh:164`
**Issue:** The audit's non-vacuity controls (the T-01-26 rationale) run only by hand; no stage calls them, unlike `tests/protocol/synthetic.sh`. Inside the controls, `has EXT` is a fixed-substring grep, so any output line containing `TEXT-PATTERN` also satisfies it; the `rom-blob` and `dup-blob-path` cases would pass on the wrong flag if a text hit appeared. Both cases currently contain no text-pattern content, so they pass for the right reason.
**Fix:** Add an `audit-negatives` stage that runs the script (with the marker-line guard used for protocol), and match the flag at the start of a line (`grep -q '^EXT '`).

### IN-04: `check-results` does not check the Evidence column or duplicate result rows

**File:** `scripts/check-results:79-83`, `scripts/check-results:93-102`
**Issue:** Only the Value column is tested for emptiness; Evidence (column 6 in record mode, column 5 in TSV mode) is parsed away. Two rows for the same (metric, variant, option) with different values are both accepted, so a stale value can sit beside the real one. Documented as "structure only", but a duplicate is a structural defect.
**Fix:** Emit `RESULT_DUPLICATE` when `seen[m SUBSEP v SUBSEP o]` is already set, and `RESULT_NO_EVIDENCE` for an empty evidence cell.

---

_Reviewed: 2026-11-09_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
