---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 29
files_reviewed_list:
  - scripts/fixture-build
  - scripts/fixture-lint
  - scripts/lib/fixture-lint-lex.awk
  - scripts/lib/fixture-lint-rules.awk
  - scripts/input-check
  - scripts/lib/input-rules.awk
  - scripts/xasm-check
  - scripts/lib/xasm-matrix.awk
  - scripts/opcode-check
  - scripts/lib/opcode-rules.awk
  - scripts/conformance-run
  - scripts/conformance-flatten
  - scripts/determinism-run
  - scripts/inventory-check
  - scripts/lib/inventory-rules.awk
  - scripts/lib/inventory-defs.awk
  - scripts/lib/record-rules.awk
  - scripts/record-lint
  - scripts/docs-check
  - scripts/workflow-check
  - scripts/lib/workflow-rules.awk
  - scripts/preflight
  - scripts/audit-history
  - scripts/audit-history.allow
  - scripts/rights-check
  - scripts/core-boundary-check
  - scripts/linux-repro
  - .github/workflows/gate.yml
  - CMakeLists.txt
findings:
  critical: 1
  warning: 5
  info: 4
  total: 10
  necessity: 0
status: issues_found
---

# Phase 3: Code Review Report (gate scripts, CI workflow, CMake)

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 29
**Status:** issues_found

## Summary

I read every listed file in full, plus the Phase 3 stage and selftest sections of `scripts/check` (run_controls, the fixture/input/xasm/opcode stages, mutation-toy-filter, mutation-workflow-cc65, mutation-snapshot-host-hash, the fixture-lint mutations) so that I could judge the controls against the code they control. Every claim below was reproduced in a scratch directory outside the repo (`/private/tmp/claude-501/-Users-jon-projects-duospool/06f2647a-e8db-469c-a561-38fda7909897/scratchpad`) or checked against the live tree. No repo file other than this review was modified. The local awk is the macOS one-true-awk (20200816); gawk and mawk are not installed here, so the dialect review of the new awk files (intervals, gensub, `length(array)`, `%c`, uninitialised comparisons, `getline`, `ENVIRON`) was by reading: I found no construct outside the common subset (the only `%c` is `sprintf("%c", 13)` in workflow-rules.awk; no regex intervals in any awk program).

What holds up well:
- The new stage-1 sentinel pattern is correct in fixture-lint (ledger reader, lex, rules), input-check, xasm-check, opcode-check and inventory-check: every stage writes a file, its status is tested, its END/DEND/RULES/N record is required, and an `--expect` that names no code is exit 2.
- The toy-control filter (`filter_cases`) is fail-closed: an id with no K row, an END-less source, a non-K/END row, or an empty list is exit 2 with the output removed, and a toy that cannot see its case yields `no-result`, which is `NOT_RED`, never a pass. The mutation-toy-filter selftest deletes exactly the one statement it names (verified: one match, zero after) and also pins the wiring.
- gate.yml keeps `permissions: contents: read`, one action pinned to a full SHA, no secrets, no `continue-on-error`, the base SHA reaches the shell through `env`, and the new apt step carries no expression. The new audit-history.allow entry is exact path plus check (`scripts/docs-check` / `ROM-SIZE-HDR`), is NUL-gated by `emit()`, and does not widen anything; the HEAD blob of `scripts/docs-check` is indeed 8704 bytes.
- rights-check's `./`-prefixing, digest-shape check and `sha256_of` status handling are correct.

The defects are concentrated in (a) one remaining CR-01 pipeline that fails open, (b) lint rules that can be bypassed or silently inert, and (c) two end-of-stage checks that can pass on empty evidence.

## Critical Issues

### CR-01: record-lint reports a bad record as clean when its violation-filter awk fails (CR-01 lesson not applied here)

**File:** `scripts/record-lint:124`
**Issue:** `awk 'BEGIN { FS = "\t" } $2 != "P"' "$raw" | sort -u > "$viol"` puts awk on the left of a pipe. POSIX sh has no pipefail, so the status is that of `sort`; a dead or failing awk leaves `$viol` empty and the script prints `STATUS record-lint ok`. This is the exact pattern the same commit range fixed in audit-history, core-boundary-check and rights-check ("CR-01: no awk in this file is the left side of a pipe"), and record-lint is the checker that judges every run record (accept, refusal and golden) and the per-case determinism inputs. Reproduction: a shim `awk` that exits 2 with no output for the one program containing `$2 != "P"` and otherwise execs the real awk:

```
$ sh scripts/record-lint tests/gate/record-lint/path-in-value/record.toml
tests/gate/record-lint/path-in-value/record.toml	REC_PATH	9	core.build_id holds a slash or a backslash      (rc=1)
$ PATH=<shim>:$PATH sh scripts/record-lint tests/gate/record-lint/path-in-value/record.toml
STATUS record-lint ok n=1                                                                                    (rc=0)
```

Mitigation noted for calibration: a wholly dead awk is still caught, because the stage awks and the `nrec=$(awk ...)` on line 125 fail under `set -e`; only a selective failure of this one invocation (a resource failure, a dialect regression in this one program) reads as clean. That is still a fail-open path in the evidence engine.
**Fix:**
```sh
awk 'BEGIN { FS = "\t" } $2 != "P"' "$raw" > "$tmp/viol.raw" || { echo "HARNESS_AWK failed (violation filter)" >&2; exit 2; }
sort -u "$tmp/viol.raw" > "$viol"
```

## Warnings

### WR-01: fixture-lint's mnemonic rules see only lower-case, space-separated first words, so the size-hint and unknown-mnemonic rules are bypassable

**File:** `scripts/lib/fixture-lint-lex.awk:38-47` (INS emission), consumed by `scripts/lib/fixture-lint-rules.awk:100-117`
**Issue:** The lexer emits an `INS` record only when the first word matches `^[a-z][a-z0-9]*$`, and it strips a label only when the colon is followed by a blank or end of line. ca65 accepts instructions in any case and accepts a label glued to the mnemonic. Reproduced against a copy of `tests/gate/fixture-lint/ok-fixture` plus a failing-control line:
- `LDA $20` (upper case): no FXL_SIZE_HINT. The lower-case `lda $20` gives `FXL_SIZE_HINT`. ca65 `--cpu huc6280` assembles `LDA $20` with exit 0.
- `FOOBAR $44` (upper case): no FXL_UNKNOWN_MNEMONIC; `xyzzy $12` (lower case) is reported.
- `glued:lda $20`: no violation (`FIXTURE_LINT ... STATUS fixture-lint ok`, rc 0); ca65 accepts it.
The size-hint rule exists because a missing `z:`/`a:` hint silently changes the encoded instruction length (Pitfall 5); the xasm matrix proves the assembler's encodings, not the fixtures' hints. Today's fixtures are all lower case, so nothing is wrong yet, but the rule claims to cover the instruction set and does not.
**Fix:** In the lexer, lower-case the word for the INS test and strip a label with a colon not followed by a blank:
```awk
if (code ~ /^[A-Za-z_@][A-Za-z0-9_@]*:/ && code !~ /^[A-Za-z_@][A-Za-z0-9_@]*::?=/) { i = index(code, ":"); code = substr(code, i + 1) }
...
lw = tolower(word)
if (lw ~ /^[a-z][a-z0-9]*$/) printf "INS\t%s\t%d\t%s\t%s\n", FILENAME, FNR, lw, rest
```
(the rule-side `mn in mnem` lookup is already lower-case). Add a `size-hint-upper` and a `size-hint-glued-label` control mini-root.

### WR-02: fixture-lint does not restrict `.incbin`, so a fixture can embed any file, including one outside the repository

**File:** `scripts/lib/fixture-lint-lex.awk:46`, `scripts/lib/fixture-lint-rules.awk:94-99`
**Issue:** Only `.include` is path-checked (FXL_INCLUDE_PATH). `.incbin "/etc/hosts"` appended to the ok-fixture mini-root passes (`FIXTURE_LINT fixtures=1 checks=4 raw=1`, `STATUS fixture-lint ok`, rc 0). A fixture built on the owner's machine can therefore pull any local file (a private ROM, a system card) into an image whose hash then goes into the used list, with the rights boundary (`D-123`, `D-133`, "no commercial ROMs ... used") never seeing the source. rights-check scans only committed/untracked-unignored files, and `build/` is ignored by design.
**Fix:** Treat `.incbin` like `.include` in the lexer (`tolower(word) == ".incbin"` emits an `INC`-style record with a distinct tag) and apply the same bare-name / `fixtures/lib/` / `build/` rule, or refuse `.incbin` outright with a new code (`FXL_INCBIN`) and a control.

### WR-03: inventory-check's INV_CONST_LEDGER rule is silently inert when a DS_CONST/DS_MODEL row is not at the start of a line

**File:** `scripts/lib/inventory-defs.awk:9`, `scripts/inventory-check:125-132`
**Issue:** Only a line matching `^DS_(CONST|MODEL)\(` is a row, and the wrapper requires nothing beyond the `DEND` sentinel. An X-macro row may legally be indented (`  DS_CONST(...)`) or written `DS_CONST (...)`. Reproduced on a scratch copy of the tree's REQUIRED, cases, ledger and the two `.def` files: after indenting every row and appending `  DS_CONST(bogus, 1, "LED-CPU-9999")` (a ledger id that does not exist), `inventory-check` still printed `INVENTORY required=234 cases=234 pins=37` / `STATUS inventory ok`, rc 0. The comment says "an emptied copy of this file is a dead stage", but an emptied or reformatted file prints exactly one sentinel and passes. The W7 guarantee (every named constant resolves to a ledger entry) depends on the rows being seen.
**Fix:** Match the row start with leading blanks (`/^[ \t]*DS_(CONST|MODEL)[ \t]*\(/`), and have the wrapper require at least one `D` record per def file that exists (NO_SENTINEL-class violation otherwise), for example by printing `DEND <count>` and refusing a count of 0.

### WR-04: trace_out_check passes a trace with zero instruction records

**File:** `scripts/conformance-run:322-356` (condition at line 352)
**Issue:** The per-file check is `bytes == records * 24` with host size and SHA agreeing. For `records=0 lost=0 bytes=0 sha256=e3b0c442...` and an empty file, every condition is satisfied, so a tracer that records nothing (hook unwired, deadline of 0 steps, sink never attached) reads as `TRACE_OUT ds_proto insn dev ok`. The live output shows `dev records=0` is normal for ds_proto (`build/gate/trace-out.txt`), so the loop cannot distinguish "legitimately empty" from "dead" for either file; the insn file is the one that must never be empty (the live run has 83447 records).
**Fix:** Inside the loop, require `[ "$to_k" != insn ] || [ "$to_recs" -ge 1 ]` and emit `TRACE_OUT_EMPTY` (exit 1) otherwise.

### WR-05: xasm-check's optional WLA path turns a documented, permanent difference into a gate failure and can print `wla=equal` without any comparison

**File:** `scripts/xasm-check:161-197`, header comment lines 15-16
**Issue:** The header states that BRK differs in WLA-DX (it emits two bytes) and that "the wla=equal marker has never been printed". Stage 2 of the WLA form compares every row and the `V` records are appended to `viol0`, so on any machine that has `wla-huc6280` and `wlalink` on PATH the xasm stage (and the `xasm-negatives` ok-tsv control, which runs the same code with the real PATH) fails with `XASM_MISMATCH` for opcode 00. D-129/D-130 call the cross-check optional and verify-only; here its mere presence on the owner's machine turns the local gate red, which pushes the owner to uninstall the tool or to ignore a red gate. Separately, unlike the ca65 path, the WLA stage-2 output loop (lines 187-191) does not require its `N` summary record, so a hollow stage 2 that exits 0 with no output would still set `wla=equal` (the string that would later be read as a cross-assembler claim).
**Fix:** Either carry the BRK exception as an explicit, tagged table row (`BRK` expected `0000` for the WLA form only, with a note) or report WLA differences as an informational `XASM_WLA_DIFF` line that does not fail the stage; and require exactly one `N` record in the WLA loop before setting `wla=equal`, mirroring lines 141-152.

## Info

### IN-01: Remaining awk-on-the-left-of-a-pipe instances (guarded, but the CR-01 comment claims otherwise)

**File:** `scripts/determinism-run:178`, `scripts/conformance-run:210` and `:690`, `scripts/docs-check:150-160`, `scripts/core-boundary-check:230`
**Issue:** Same class as CR-01, but each is guarded by a later emptiness or equality check, so none is fail-open today: determinism-run:178 (`want.names` empty gives exit 2; a partial list mismatches the runner's records), conformance-run:210/690 (`[ ! -s ids.txt ]` guard), docs-check `listed=` (compared with the ledger-derived list), core-boundary-check:230 (`nm | awk`: a failing `nm` shrinks the allowed set, which can only add violations). The commit message for the pipeline sweep says no awk is left of a pipe "in this file" for three files; these were not touched.
**Fix:** Same two-step shape as CR-01 for consistency (write to a file, test the status, then sort), so that the guard is not load-bearing.

### IN-02: opcode-check attributes every violation to cpu_ops.def, including those of the TSV

**File:** `scripts/opcode-check:116`
**Issue:** `V` rows from both stage-1 runs are printed as `"$defrel" CODE ...`. An `OPC_SHAPE` or `OPC_EXTRA_ROW` (duplicate) found in `tests/oracle/huc6280-opcodes.tsv` is reported against `src/core/cpu_ops.def`, which sends the reader to the wrong file (the gate message "re-read the page image, then correct the side that is wrong" depends on knowing the side). The `kind` is not carried in the V row.
**Fix:** Have `opcode-rules.awk` stage 1 add the kind as a fourth field of its `V` rows (`V<TAB>CODE<TAB>where<TAB>kind<TAB>detail`) or prefix the detail with `[def]`/`[tsv]`, and map it to the file in the wrapper.

### IN-03: WF_NO_CC65_STEP is satisfied by a step that only echoes the install command

**File:** `scripts/lib/workflow-rules.awk:108-111`
**Issue:** `cc65_line` matches any non-comment line containing `apt-get install` and a standalone `cc65` word. Replacing the install step's body with `echo apt-get install cc65` still gives `WORKFLOWS 1` / `STATUS workflow ok` (reproduced on a scratch copy of the ok-gate mini-root). The gate's own preflight still fails closed without ca65 and ld65, so this is a weak static check, not a hole.
**Fix:** Require the line to begin with `apt-get install` or `sudo apt-get install` after trimming (anchor with `^(sudo )?apt-get install`), which still lets the real step through.

### IN-04: Small input-validation gaps in fixture-build and fixture-lint

**File:** `scripts/fixture-build:63` and `:80`, `scripts/fixture-build:158-180`, `scripts/fixture-lint:108`
**Issue:** (a) `--out ''` passes the `$# -ge 2` test and the cleanup `rm -rf "$out/fx" "$out/fixtures" "$out/fixtures-authored.used"` becomes `rm -rf /fx /fixtures /fixtures-authored.used`; validate `[ -n "$out" ]` (and the same for `--src`). (b) A variants-list line whose stem has no `fixtures/src/STEM.s` is skipped without a message, so renaming or deleting a source makes its variant images vanish silently; only the later manifest/used-list comparison would notice. Check after the loop that every variant stem was seen. (c) fixture-lint's ledger reader globs `LED-*.toml` while inventory-check reads `*.toml`; an entry under another name is visible to one and not the other (fail-closed for `ASSUMED:` ids, so harmless today, but the two readers should agree).
**Fix:** As described in each item.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
