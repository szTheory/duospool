# Phase 2: Deterministic Machine Shell, HuCard Boundary and Fail-Closed Gate - Pattern Map

**Mapped:** 2026-11-09
**Files analyzed:** 29 file groups (about 60 concrete files) from CONTEXT D-40..D-74 and RESEARCH
**Analogs found:** 23 / 29 file groups (6 groups have no repo analog, listed at the end; 3 of the 23 are only partial matches)

**Analog scope rules used.** (1) The repo is sh + awk today and has no C. Every C analog comes from the Phase 1 spike tag, read with `git show spike-tool-01-evidence:<path>`. Those paths (`spikes/tool-01/...`) are NOT in the working tree at HEAD, so name them only as `spike-tool-01-evidence:<path>`, never as a bare repo path. (2) Every working-tree analog below was verified git-tracked (`git ls-files --error-unmatch`). No gitignored mirror is cited. (3) The spike C is project-authored (same SPDX header as the rest of the repo), so adapting it needs no `PROVENANCE.md` entry; planner should still confirm against record 0007, and must NOT write a line-start `Provenance: PROV-nnnn` comment (that would trip `provenance-check` as `PROV_UNRESOLVED_MARKER`). Use a plain comment like `adapted from tag spike-tool-01-evidence` instead.

## File Classification

| New/Modified File(s) | Role | Data Flow | Closest Analog | Match |
|---|---|---|---|---|
| `scripts/check` (modify: `run_controls`, new stages, `--timing`, `results.tsv`, preflight, selftest mutations) | gate entry | batch | itself (`scripts/check` 27-43, 53-90, 601-708) | exact |
| `scripts/rights-check` (+ `scripts/lib/rights-*.awk` if needed) | checker | batch / file-I/O | `scripts/audit-history` 141-150 + `scripts/licence-check` skeleton | role-match |
| `scripts/core-boundary-check` + `scripts/core-boundary.allow` | checker | batch / file-I/O | `scripts/licence-check` skeleton; `scripts/audit-history.allow` (allow format) | role-match |
| `scripts/inventory-check` | checker | batch | `scripts/ledger-check` 143-182 (REQUIRED cross-file pass) | role-match |
| `scripts/fixture-check` (name discretionary) | checker | batch | `scripts/provenance-check` + `scripts/licence-check` 68-73 (sha256 compare) | role-match |
| `scripts/workflow-check` | checker | batch (line-oriented) | `scripts/decision-check` 133-139 (`grep -qxF` assertions) | partial |
| `scripts/conformance-flatten` | transform | transform | `scripts/profile-check` 80-88 + `scripts/lib/toml-subset.awk` | exact |
| `scripts/conformance-ingest` | checker | batch | `scripts/ledger-check` 143-182 + `scripts/check-results` | role-match |
| `scripts/audit-history` (modify: `--range BASE..HEAD`) | checker | batch | itself 36-44, 62-79 | exact |
| `scripts/licence-check` (modify: `src/`, `tests/**/*.c\|*.h`) | checker | batch | itself 88-100 | exact |
| `scripts/ci-timings`, optional `scripts/linux-repro` | utility | request-response | none (see No Analog) | none |
| `tests/lib/gitfix.sh`, `tests/audit/synthetic.sh` (modify), `tests/rights/synthetic.sh` | test | file-I/O (temp git repos) | `tests/audit/synthetic.sh` 25-61 | exact |
| `tests/gate/<checker>/<case>/` mini-roots + `EXPECT` (5 named controls) | test control | batch | `tests/provenance/<case>/` + `tests/licence/<case>/` (`--root`) | exact |
| `tests/workflow/<case>/` | test control | batch | `tests/provenance/<case>/` | exact |
| `ledger/entries/LED-LOAD-0003/0004/0005`, `LED-PROF-0002`; edit LOAD-0001/0002; `ledger/REQUIRED` | record | CRUD | `ledger/entries/LED-LOAD-0002.toml` | exact |
| `fixtures/manifest/<id>.toml` | record | CRUD | `ledger/entries/*.toml` (strict subset, id = filename) | role-match |
| `CMakeLists.txt`, `CMakePresets.json` | config | batch | `spike-tool-01-evidence:spikes/tool-01/c/CMakeLists.txt` | role-match |
| `src/core/{profile,resolve,instance,mapper,poweron,serial,content_id}.c` + `ds_*.h` | core lib | transform | `spike-tool-01-evidence:.../c/src/core.c` | role-match |
| `src/core/profile_fields.def`, `tests/conformance/outcomes.def` | X-macro table | transform | none; list duplicated in `scripts/lib/profile-rules.awk` 21 | none |
| `src/core/sha256.c`, splitmix64 in `poweron.c` | core util | transform | none in repo (RESEARCH P10, P11) | none |
| `src/text/*.c` | printer | transform | none (spike has no printer); spike `status_name()` for names | partial |
| `tests/conformance/runner.c` | runner | batch, request-response | `spike-tool-01-evidence:.../c/src/runner.c` | role-match (verdict logic must NOT carry over) |
| `tests/conformance/synth.c` | generator | transform | `spike-tool-01-evidence:spikes/tool-01/gen-fixtures.sh` 25 (LCG) | role-match |
| `tests/conformance/cases/<id>.toml`, `tests/conformance/REQUIRED` | record, list | CRUD | ledger entry files + `ledger/REQUIRED` | role-match |
| `tests/conformance/` awk mirror oracle | test oracle | transform | `spike-tool-01-evidence:.../consumer/expect.pl` (independent second implementation) | role-match |
| `tests/controls/{shared-state,wrong-mirror,heuristic-reverse,drop-region}/` | toy lib | transform | none (pattern in RESEARCH Pattern 2) | none |
| `.github/workflows/gate.yml` | config | event-driven | RESEARCH skeleton 311-348 (actionlint-clean) | no repo analog |
| `.github/{dependabot.yml,pull_request_template.md,CODEOWNERS}`, `.gitattributes` | config | n/a | none | none |
| `README.md`, `AGENTS.md`, `docs/decisions/0007-...md` (modify), `docs/ci-timings.md` | docs | n/a | `README.md` 14-18 | partial |

## Pattern Assignments

### 1. `scripts/check` (modify) - gate entry, batch

**Analog:** itself. Keep the exit contract (0 green, 1 any fail, 2 harness error), marker rule, `CHECK <stage>` lines, `--selftest`.

**Marker-aware wrapper and EXPECT loader** (`scripts/check` 27-43, verbatim):
```sh
checker() {
  if "$@" > "$tmp/chk" 2>&1; then r=0; else r=$?; fi
  cat "$tmp/chk"
  if [ "$r" -eq 0 ] && ! grep -Eq '^(EXPECT_OK$|STATUS |RECORDS )' "$tmp/chk"; then
    echo "CHECKER_DEAD $*"
    return 2
  fi
  return "$r"
}
read_expect() {
  exp=$(cat "$1")
  [ -n "$(printf '%s' "$exp" | tr -d ', \t\r\n')" ]
}
```
Planner note: every new checker MUST print a line matching `^(EXPECT_OK$|STATUS |RECORDS )` on clean exit (D-54 `STATUS core-boundary ok` fits; give `conformance-ingest` a `STATUS conformance ...` line). If a different marker is wanted, extend this regex in the same plan.

**Canonical control loop to factor** (`scripts/check` 53-90; the EXPECT branch, lines 56-84, verbatim shape):
```sh
for d in ledger/negative/*/; do
  [ -d "$d" ] || continue
  n=$((n + 1)); case_name=$(basename "$d")
  [ -f "$d/EXPECT" ] || { echo "HARNESS_NO_EXPECT $case_name"; rc=2; continue; }
  read_expect "$d/EXPECT" || { echo "HARNESS_EMPTY_EXPECT $case_name"; rc=2; continue; }
  if checker sh "$here/ledger-check" --expect "$exp" "$d" > "$tmp/case.out"; then r=0; else r=$?; fi
  if [ "$r" -eq 1 ]; then
    echo "CONTROL BROKEN $case_name"; sed 's/^/  /' "$tmp/case.out"
    [ "$rc" -eq 2 ] || rc=1
  elif [ "$r" -ne 0 ]; then
    echo "HARNESS_ERROR $case_name"; sed 's/^/  /' "$tmp/case.out"; rc=2
  fi
done
if [ "$n" -eq 0 ]; then echo "NO_CONTROLS ledger/negative"; [ "$rc" -eq 2 ] || rc=1; fi
return "$rc"
```
(The real file spells the `if` bodies on separate lines; stdout text is what must stay identical.) `rc` is a global set by the stage function, and `name_control` (252-275) and `empty_expect_control` (309-321) rely on that; keep it global.

**Variation table: what `run_controls DIR CHECKER ARGS...` must parametrize** (the six loops; keep score/filename/empty-expect stages as they are, RESEARCH Pitfall 1):

| Stage (lines) | Case glob | Per-case checker args | ok- handling | Special |
|---|---|---|---|---|
| ledger-negatives (53-90) | `ledger/negative/*/` | `--expect E [--required d/REQUIRED] d` | none; positives are a separate stage `ledger/samples/ok-*/` (93-116, `SAMPLE REJECTED`) | optional REQUIRED file |
| decision (123-172) | `tests/decision/*/` | `--expect E d`; ok-: `d` | `ok-*` runs without EXPECT | |
| profile (190-245) | `tests/profile/*/` | `--expect E d/pce-hucard-2btn/r1.toml` | ok- only when no EXPECT; else `HARNESS_NO_EXPECT` | `HARNESS_NO_RECORD` when record absent |
| provenance (359-408), licence (416-465) | `tests/<x>/*/` | `--root d --expect E`; ok-: `--root d` | `ok-*` | identical bar checker name |
| results (541-597) | `tests/results/*/` | `--expect E d/protocol.md --record d/record.md` | `ok-*` | `HARNESS_NO_FILE`; extra `neg` count, `NO_CONTROLS` if no negative |

Suggested shape: `run_controls GLOBDIR LABEL CHECKER_ARGS_WITH_PLACEHOLDERS`, where `@DIR@` becomes the case dir and `--expect @EXPECT@` is dropped for `ok-*` cases. New control families then cost one line each: `run_controls tests/gate/inventory inventory-check --root @DIR@ --expect @EXPECT@`. Proof of equivalence (RESEARCH Pitfall 1): capture `sh scripts/check` stdout before and after; plus three mutated-tree transcripts (broken EXPECT, empty control dir, removed control) equal before and after.

**Selftest mutation pattern to copy for each new rule** (`scripts/check` 606-612, verbatim):
```sh
rm -rf "$cp_dir"
cp -R "$here" "$cp_dir"
grep -v 'SUPPORTED_NO_QUALIFYING_EVIDENCE' "$cp_dir/lib/ledger-rules.awk" > "$cp_dir/lib/ledger-rules.mutated"
mv "$cp_dir/lib/ledger-rules.mutated" "$cp_dir/lib/ledger-rules.awk"
if sh "$cp_dir/ledger-check" --expect SUPPORTED_NO_QUALIFYING_EVIDENCE ledger/negative/e0-e4/ > "$tmp/st.out" 2>&1; then r=0; else r=$?; fi
if [ "$r" -eq 1 ]; then echo "SELFTEST mutation-supported-rule PASS"; else echo "SELFTEST mutation-supported-rule FAIL"; failed=1; fi
```
Mutate the COPY (`$here` is copied whole, so `scripts/core-boundary.allow` and every new `scripts/lib/*.awk` are mutable the same way). Add mutations for `INV_REQUIRED_MISSING`, `FIX_MISSING_FIELD`, `FIX_UNLISTED`, `ROMLIKE_BINARY`, one `core-boundary.allow` line, and a `WF_*` code (D-58, D-54, D-62).

**Stage runner and tail** (`scripts/check` 657-670, 672-678, 699-708): `run_stage NAME FUNCTION` runs `"$fn" > "$tmp/out"`, maps rc 0/1/other to PASS/FAIL/ERROR, prints `CHECK name verdict`, indents details with `sed 's/^/    /'`. This is the single insertion point for `--timing` (`date +%s` around `"$fn"`) and for appending to `build/gate/results.tsv` (D-60, D-63). Argument parsing at 672-678 accepts only a lone `--selftest` and prints `usage: sh scripts/check [--selftest]`; extend both for `--timing`. Stage list is 680-697; new stages go in D-60 order (preflight first; rights-tree after the existing meta stages; CMake build, runner, ingest, boundary, controls, outcome-sync after).

**Preflight** (D-60): a missing tool is `harness_failure` (exit 2 / `CHECK preflight ERROR`), never skip. Model on `name_control` (252-275): print a fixed code, set `rc=2`.

---

### 2. New checker skeleton (every `scripts/*-check`, `conformance-*`)

**Analog:** `scripts/licence-check` (cleanest, 121 lines). Copy this structure verbatim and change only the rules.

**Header + preamble** (lines 1-22, condensed: comment text shortened, statements unchanged):
```sh
#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# licence-check - ... usage: licence-check [--expect CODE[,CODE...]] [--root DIR]
# stdout: sorted violations  FILE<TAB>CODE<TAB>LINE<TAB>DETAIL , then one line  RECORDS n
# stdout on a met --expect: the single line EXPECT_OK (scripts/check treats a silent exit 0 as a dead checker)
# exit  : 0 pass (or --expect met) | 1 violations (or --expect not met) | 2 usage or harness error
set -eu
LC_ALL=C; export LC_ALL
here=$(cd "$(dirname "$0")" && pwd)
usage() { echo "usage: ..." >&2; exit 2; }
```
**Arg parse and the WR-05 empty-expect guard** (31-51): `--expect` / `--root` loop with `[ $# -ge 2 ] || usage`; then
```sh
if [ "$have_expect" -eq 1 ] && [ -z "$(printf '%s' "$expect" | tr -d ', \t\r\n')" ]; then
  echo "HARNESS_EMPTY_EXPECT --expect names no code" >&2
  exit 2
fi
if [ ! -d "$root" ]; then echo "HARNESS_NO_FILE $root" >&2; exit 2; fi
root=$(cd "$root" && pwd)
```
**Temp + violations file** (53-58): `tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT; viol0="$tmp/viol0"; : > "$viol0"`; rules append `printf '%s\tCODE\t0\t%s\n' ... >> "$viol0"`.
**Tail** (102-121, same statements, some one-line `if` bodies joined):
```sh
sort -u "$viol0" > "$viol"
if [ "$have_expect" -eq 1 ]; then
  got=$(awk -F'\t' '{ print $2 }' "$viol" | sort -u | tr '\n' ',' | sed 's/,$//')
  want=$(printf '%s\n' "$expect" | tr ',' '\n' | sort -u | tr '\n' ',' | sed 's/,$//')
  if [ "$got" = "$want" ]; then echo "EXPECT_OK"; exit 0; fi
  echo "EXPECT_MISMATCH wanted '$want' got '${got:-<none>}'" >&2
  cat "$viol" >&2
  exit 1
fi
cat "$viol"
echo "RECORDS $nrec"
if [ -s "$viol" ]; then exit 1; fi
exit 0
```
`--root DIR` mini-root mode is what makes controls possible (precedent: `provenance-check`, `licence-check`). Build every new checker with `--root` from day one. Violation codes are UPPER_SNAKE with a family prefix (`INV_`, `FIX_`, `WF_`, `PROV_`); `core-boundary-check` prints `STATUS core-boundary ok` only on a clean run and exit 0 without the marker is `CHECKER_DEAD` (D-54).

**Stage-1/stage-2 awk invocation** (`scripts/ledger-check` 90-97; same in `profile-check` 81-88):
```sh
# no pipeline: a failing stage 1 must not read as "no violations"
awk -f "$here/lib/toml-subset.awk" "$f" > "$t1" 2>/dev/null || : > "$t1"
# stage 2 always prints at least one record; an empty or failed stage 2 is a dead checker, not a pass
if fname="$base" awk -f "$here/lib/ledger-rules.awk" "$t1" > "$t2" 2>/dev/null && [ -s "$t2" ]; then :; else
  printf '%s\tV\tSTAGE2_DEAD\t0\t\n' "$f" >> "$raw"; return 0
fi
fpath="$f" awk 'BEGIN { FS = OFS = "\t" } { print ENVIRON["fpath"], $0 }' "$t2" >> "$raw"
```
Pass strings to awk through `ENVIRON[...]` or `-v`, never by interpolating into the program. Stage-2 files (`scripts/lib/*-rules.awk`) emit tagged TSV records `V<TAB>CODE<TAB>line<TAB>detail` and emit only `V NO_SENTINEL` if the stage-1 `END` line is missing (`profile-rules.awk` 47, 69).

**Awk portability rules (run under BWK, mawk, gawk):** POSIX only; no regex interval expressions `{n}` (stated in `provenance-register.awk` header); no gawk extensions; line cap 400; ASCII only; no tab inside values.

**sh portability rule** (`scripts/audit-history` 27): macOS `/bin/sh` is bash 3.2 and mis-parses a `case` statement placed inside `$( )`; never do that. Gate must pass under `dash` too (`dash scripts/check`, RESEARCH).

**SPDX headers (enforced by `licence-check` 96-97):** every file under `scripts/`, including `.allow` and `.awk`, needs both strings in its first five lines. `.allow` style: `scripts/audit-history.allow` lines 1-3 (`# SPDX...` x2, then `# Format: path<TAB>CHECK<TAB>reason`). C files use the block form `/* SPDX-License-Identifier: MIT OR Apache-2.0 */` (the check is a fixed-string match, so it passes). Data records and fixtures use `0BSD` (`ledger/entries/*.toml` line 1; `profiles/.../r1.toml` line 1; D-71).

---

### 3. `scripts/conformance-flatten` (transform, exact analog)

**Analog:** `scripts/lib/toml-subset.awk` output contract (lines 6-11) invoked as in `profile-check` 81. Wrap it: run the awk, then fail (exit 1, no TSV on stdout) if any `ERR` line is present or the `END` sentinel is absent, so no partial TSV ever reaches the C runner.
```
# toml-subset.awk 6-9: in: one file. out: TABLE<TAB>KEY<TAB>TYPE<TAB>VALUE<TAB>LINE | ERR<TAB>CODE<TAB>LINE<TAB>detail | last line END<TAB>NR
# TYPE: s string, i integer, b bool, a array (VALUE = elements joined by US=\037)
```
Subset limits the planner must design around (D-69): no `"` or `\` inside strings (qstr line 20 rejects backslash), ASCII only, keys and table names `[a-z][a-z0-9_]*` (so `[field_io_1000_d6_value]` is fine), TAB forbidden in values, no floats or dates. Emit the TSV only after all inputs parse; write a trailing `END` row of its own. Own negative controls (RESEARCH Wave 0): bad TOML gives non-zero exit and no partial TSV; missing `END` is rejected.

---

### 4. `scripts/inventory-check` and `scripts/conformance-ingest` (batch, role-match)

**Analog:** `scripts/ledger-check` 143-182, the REQUIRED cross-file pass. It already does "listed id with no file is `REQUIRED_MISSING`", file-versus-list in both directions via associative arrays, and `REQUIRED_EMPTY` when the list has zero lines (so an emptied REQUIRED cannot switch the requirement off, WR-06):
```sh
$2 == "R" { nreq++; rid[nreq] = $3; rorg[nreq] = $4; rfile = $1; next }
...
if (ENVIRON["reqgiven"] == "1" && nreq == 0) print ENVIRON["reqfile"], "REQUIRED_EMPTY", 0, "no requirement lines"
for (i = 1; i <= nreq; i++) {
  if (!(rid[i] in have)) print rfile, "REQUIRED_MISSING", 0, rid[i]
  ...
}
```
REQUIRED-file parse (`ledger-check` 151-159): skip `^[ \t]*(#.*)?$`, shape-check each line, emit a `B` bad-line record otherwise. Copy for `tests/conformance/REQUIRED` (ids `[a-z][a-z0-9_]*`). Map: `INV_REQUIRED_MISSING` = REQUIRED id with no `cases/<id>.toml`; `INV_UNLISTED_CASE` = case with `required = true` not in REQUIRED; `INV_PIN_UNRESOLVED` / `INV_NOT_REQUIRED` = ledger `TEST:<id>` pin resolved both ways (read pins with the `ledger-rules.awk` shape `pinning_test` `TEST:.+`, line 84). `ledger-check` itself stays structure-only (D-58).

**`conformance-ingest` additions** (no analog in repo for these, RESEARCH Pitfall 2 and Pattern 1): input is a RESULT file path so controls can feed fabricated results without a build. Reject, as non-green: empty input; missing `RUNNER_DONE n` marker; unknown outcome token; duplicate id; id not in inventory; REQUIRED id with no line (`missing_artifact`); `assertions=0` or empty `evidence=`. Only `pass` is green; `skip` is red. Print `STATUS conformance ...` on clean exit. Line shape: `RESULT <id> <outcome> assertions=<n> evidence=<relpath>`.

---

### 5. `scripts/rights-check` (FIX-05) and `scripts/audit-history --range`

**Analog:** `scripts/audit-history` heuristics and `licence-check` skeleton.

**ROM-shape and extension rules to reuse** (`audit-history` 141-144, verbatim):
```awk
if (nul) emit("NUL-BYTES", size, p, 0)
if (size >= 4096 && size % 8192 == 0) emit("ROM-SIZE-8K", size, p, nul)
if (size >= 4096 && size % 8192 == 512) emit("ROM-SIZE-HDR", size, p, nul)
lp = tolower(p)
if (lp ~ /\.(pce|sgx|iso|cue|bin|rom|chd|sav|srm|zip|7z|pem|key)"?$/) emit("EXT", size, p, 0)
```
**NUL detection without extensions** (`audit-history` 120-123): `n1=$(wc -c < f)`, `n2=$(tr -d '\000' < f | wc -c)`, differ means NUL. Use `iconv -f UTF-8 -t UTF-8 FILE >/dev/null` for UTF-8 validity (RESEARCH Don't Hand-Roll; same verdicts on macOS and Ubuntu).
**Enumerate with** `git ls-files -co --exclude-standard` (D-56). Apply the 8192-shape rule only to files that are not pure printable ASCII plus LF/TAB; otherwise `ledger/samples/ok-boundary-16384/LED-CPU-0001.toml` and `tests/profile/ok-boundary-16384/pce-hucard-2btn/r1.toml` (both 16384 B ASCII) false-positive (RESEARCH Pitfall 6).
**Allowlist semantics to copy** (`audit-history` 146-150): suppress only when path AND check match, and a shape allowance never applies once the blob has a NUL byte. For rights-check the allowlist is the fixture manifest, not an `.allow` file (D-56): path exists, sha256 matches, `kind: authored`, SPDX present; a stale entry fails `missing_artifact`.

**`--range` edit sites in `audit-history`:** arg loop (36-44: add `--range) [ $# -ge 2 ] || usage; range=$2; shift 2 ;;`); shallow check (62-70: keep only for full mode, because a range inside a depth-limited fetch is valid when BASE is present); the two enumerations (73 `git rev-list --all` and 79 `git rev-list --objects --missing=print --all`) take `$range` in place of `--all`. The per-commit `git ls-tree -r` pairs and `if (!(h in sz)) next` (line 137) already confine results to blobs read. Preserve the self-match safety comment (24-26): no literal key, token or home path in the source.

**Run-time controls:** copy the helper block from `tests/audit/synthetic.sh` 25-61 (`g`, `new`, `commit`, `verdict`, `has`; `GIT_CONFIG_NOSYSTEM=1`, `GIT_CONFIG_GLOBAL=/dev/null`, explicit `-c user.name -c user.email -c commit.gpgsign=false`). Factor them to `tests/lib/gitfix.sh` and `. ` it from both scripts. Planted ROM control (lines 72-79):
```sh
d=$(new rom-blob)
dd if=/dev/zero of="$d/x.pce" bs=8192 count=1 2>/dev/null
commit "$d" rom
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -ne 0 ] && has NUL-BYTES && has ROM-SIZE-8K && has EXT; then ok=1; fi
verdict rom-blob "$ok"
```
For rights-check controls use `git add -f` in the temp repo (`.gitignore` has `*.bin *.pce`, so these can never be committed in the tree). Shallow-clone control at 183-199 is the model for a `--range` test. `tests/audit/synthetic.sh` is not a gate stage today; D-71 adds it via the existing marker convention: it prints one `AUDITTEST <case> PASS|FAIL` line per case and its exit status is the verdict, so wrap it like `stage_protocol_negatives` (`scripts/check` 473-481, which requires a marker line `^SYNTH .* PASS$` and otherwise reports `NO_CONTROLS`); the audit wrapper needs `^AUDITTEST .* PASS$`.

---

### 6. `scripts/core-boundary-check` + `scripts/core-boundary.allow` (CORE-02)

**Analog:** `licence-check` skeleton (section 2) and `audit-history.allow` for the allow-file location convention (`allow="$here/audit-history.allow"`, line 36; file starts with the two SPDX comment lines then `#` format comment). Tool commands are not in the repo; take them verbatim from RESEARCH lines 351-357 and pitfalls P1-P3 (tested on macOS arm64 and Ubuntu 24.04):
```sh
# Darwin L3 writable-data rule
nm -m obj.o | awk '$2 ~ /^\(__DATA,__(data|bss|common)\)$/ || $2 ~ /^\(__DATA,__thread/ || $0 ~ /\(common\)/ { print "WRITABLE " $NF; bad = 1 } { n++ } END { if (!n) exit 2; exit bad }'
# L2 undefined symbols, Darwin leading underscore stripped
nm -P -u obj.o | awk -v uname="$(uname -s)" '{ s = $1; if (s ~ /^_/ && uname == "Darwin") s = substr(s, 2); print s }' | LC_ALL=C sort -u
```
Linux writable letters: `BbCDdGgVvu` (drop `S s`, RESEARCH P3). Include allowlist grep: `^[[:space:]]*#[[:space:]]*include` allowing only `<stddef.h> <stdint.h> <stdbool.h>` and `"ds_*.h"` (D-65). An object with zero symbols is exit 2, never a pass. Scan `-O0` plain objects only, never sanitizer builds. Controls (all prototyped, D-54): `<time.h>`, `clock_gettime`, `getenv`, `malloc`, global counter, function-local non-const static, initialised data, `__TIME__`, `<limits.h>`; positives `ok.c` (memset, 20 KB stack) and `ok2.c` (const tables). The `--selftest` mutation deletes one `core-boundary.allow` line (use the 606-612 pattern against the copied `scripts/` dir).

---

### 7. `scripts/fixture-check` (FIX-02) and fixture hash compare

**Analog (required-field register):** `scripts/provenance-check` + `scripts/lib/provenance-register.awk` (known-key table, `PROV_MISSING_FIELD`, `PROV_PATH_MISSING` file-exists check, `PROV_BAD_SPDX` allowlist). Map to `FIX_MISSING_FIELD`, `FIX_UNLISTED` (file on disk or generator output with no manifest row), `FIX_HASH_MISMATCH`, `FIX_BAD_VERDICT_SOURCE` (reject the literal `duospool-output`, FIX-03). Parse each `fixtures/manifest/<id>.toml` with the stage-1/stage-2 pair from section 2; fields per D-59.
**Hash compare** (`licence-check` 68-73): `got=$(shasum -a 256 "$f" | awk '{ print $1 }')`. D-60 says use `sha256sum` if present else `shasum -a 256`; pick once in preflight. Spike loop to model the manifest-versus-bytes pass (`spike-tool-01-evidence:spikes/tool-01/gen-fixtures.sh` 35-53): `got_size=$(wc -c < f | tr -d ' ')`, `got_sum=$(shasum -a 256 f | cut -d ' ' -f 1)`, print `FIXTURE name OK|MISMATCH|MISSING`, `n` counter, and `HARNESS_NO_FIXTURE_ROWS` exit 2 when zero rows were read. The gate hashes the runner's `--emit` stdout, not files in the tree (nothing ROM-like is committed).

---

### 8. `scripts/workflow-check` and `tests/workflow/<case>/`

**Analog:** `scripts/decision-check` 133-139 for exact-line assertions, and `licence-check` `grep -Fq -e` for fixed-string tests:
```sh
if grep -qxF 'No GPL or unclear-licence material is admitted without a further owner decision.' "$f"; then :; else
  addv "$f" FIXTURE_RIGHTS_SENTENCE_MISSING 0 ""
fi
```
Use the same `addv`-style one-liner per rule (`WF_NO_PERMISSIONS`, `WF_UNPINNED_ACTION`, `WF_FORBIDDEN_TRIGGER`, `WF_MISSING_FETCH_DEPTH`, ...; codes are planner discretion). The PR-template assertion is `grep -qxF` on the record-0007 line (copy it verbatim from `docs/decisions/0007-copied-code-provenance.md` line 41 or `PROVENANCE.md` line 8): `PR checklist: No code copied from another project without a Provenance marker and a PROVENANCE.md entry.` Fail closed (exit 2) on YAML it cannot parse (anything outside the restricted block-style subset the RESEARCH skeleton uses). Controls: `tests/workflow/<case>/` each with `.github/workflows/gate.yml` variant and `EXPECT`, run with `--root` exactly like `tests/provenance/<case>/`.

---

### 9. Ledger entries (`LED-LOAD-0003/0004/0005`, `LED-PROF-0002`) and pin updates

**Analog:** `ledger/entries/LED-LOAD-0002.toml` (field order and quoting, verbatim shape):
```toml
# SPDX-License-Identifier: 0BSD
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
# Seed entry ... A modelling assumption, not an established hardware fact.
id = "LED-LOAD-0002"
statement = "..."
profile = "pce-hucard-2btn"
status = "assumed"
sources = ["CODE|Beetle|strips 512 bytes when the size has bit 9 set (SUMMARY C8)", "CODE|MAME|..."]
evidence = []
disagreement = "..."
pinning_test = "PENDING:2"
updated = "2026-11-09"
assumption = "..."
revisit_phase = 2
withheld = ["any claim that headered dumps load"]
origin = "SUMMARY C8"
```
Rules the new entries must satisfy (`ledger/SCHEMA.md` 19-31, 45-49; `ledger-rules.awk` 19-24, 84): ASCII, at most 16384 B and 400 chars per line, one-line arrays, `sources` items `LABEL|ref|detail` with LABEL in `FACT CODE REPORT UNKNOWN`, `assumed` needs `assumption` and `revisit_phase`, area `PROF` for the power-on entry (no `PWR` area). `pinning_test` accepts any `TEST:.+`; change `PENDING:2` to `TEST:<case id>` in LOAD-0001/0002 in the same plan that creates that case (`inventory-check` then resolves it). Label the PCEdev wiki findings `REPORT` (secondary), not `FACT` (RESEARCH H1-H3 call it a community wiki), since entries stay `assumed`.
`ledger/REQUIRED` lines are `ID<space>origin text` and the regex only needs `[^ ].*` after the first space (`ledger-check` 155), and the entry's `origin` must equal that text exactly, so listing `LED-LOAD-0003 02-CONTEXT D-44` is structurally fine (answers D-72's question: yes, non-SUMMARY origins are accepted).

---

### 10. `CMakeLists.txt`, `CMakePresets.json`

**Analog:** `spike-tool-01-evidence:spikes/tool-01/c/CMakeLists.txt` lines 3-17:
```cmake
cmake_minimum_required(VERSION 3.20)
project(duospool C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_library(duospool STATIC src/core.c)
target_compile_options(duospool PRIVATE -Wall -Wextra -Wconversion -Werror)
add_executable(duospool-runner src/runner.c)
target_link_libraries(duospool-runner PRIVATE duospool)
```
Carry: C11, extensions OFF, static lib, runner linking it, flags per target. Change: raise `cmake_minimum_required` to 3.25 (presets `"version": 6`, RESEARCH P12); move warning flags into the `gate` preset (RESEARCH 299-308, copy that JSON; `-Werror` only in the preset, D-41); targets `ds_core`, `ds_text`, runner per D-40 with one-way dependency runner -> text -> core; NO `install()` and no `configure_file(duospool.pc.in)` (spike lines 15-17 are the install rules D-05 defers); no public `include/duospool.h`, use a private include dir. Separate `binaryDir` per preset; the sanitizer preset adds `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`. `build/` is already gitignored.

---

### 11. `src/core/*.c` (core library, transform, no I/O)

**Analog:** `spike-tool-01-evidence:spikes/tool-01/c/src/core.c`. Style to keep: 4-space indent, K&R braces, one declaration per line, `DS_`/`ds_` prefix, `/* SPDX */` block header, constants as `#define` with `u` suffixes, no mutable globals or statics, status enum with explicit numbers.

**Instance and constants** (core.c 13-25):
```c
#define DS_CONTENT_SIZE 65536u
#define DS_MAX_SIZE 1048576u
struct ds_instance {
    uint8_t content[DS_CONTENT_SIZE];
    uint64_t time;
    uint64_t digest;
    int loaded;
};
```
Phase 2 version: ROM array is a fixed `uint8_t rom[1048576]` inside the struct placed in caller memory (D-42); no `loaded` flag (no "created but unloaded" state).
**Length-first validation, no read before the decision** (core.c 62-76; the pattern to extend into the D-46 fixed precedence):
```c
if (len == 0u) { return DS_REFUSE_EMPTY; }
if (len > (size_t)DS_MAX_SIZE) {
    return DS_REFUSE_OVERSIZED; /* decided from the length alone: no byte of buf is read */
}
if (len < (size_t)DS_CONTENT_SIZE) { return DS_REFUSE_TRUNCATED; }
```
**Copy-in** (core.c 80): `memcpy(inst->content, buf, DS_CONTENT_SIZE);` Phase 2 uses `__builtin_memcpy` / `__builtin_memset` (D-54; `string.h` is not in the include allowlist).
**Explicit-width digest bytes** (core.c 97-99): `(uint8_t)((inst->time >> shift) & 0xffu)`; keep every narrowing cast explicit (RESEARCH P4: gcc and clang `-Wconversion` disagree in opposite directions) and widen before shifting (P5: `(uint32_t)p[0] << 24`).
**Do NOT carry over:** `calloc`/`free` (core.c 42, 128; D-42 forbids malloc), FNV-1a digest (27-30, D-52 requires SHA-256 over a framed canonical stream), `#ifdef DS_PLANTED_BUG` plant (56-61; controls are separate toy libraries), the public `duospool.h` and `duospool_*` names, `<stdlib.h>`/`<string.h>` includes (8-11), and the single-use `loaded` state.
**Enum style to copy** (`spike-tool-01-evidence:.../include/duospool.h` 17-27): `typedef enum ds_status { DS_OK = 0, DS_REFUSE_EMPTY = 1, ... }` with explicit stable numbers. Phase 2's refusal enum (D-46 plus `memory_invalid` D-66) generates its `load.<name>` strings from a `.def`, not the hand-written `switch` in the spike runner (runner.c 30-43).

---

### 12. `tests/conformance/runner.c` (runner, batch)

**Analog:** `spike-tool-01-evidence:spikes/tool-01/c/src/runner.c`. Carry the shape, change the contract.

Carry: one process runs every case (main loop at 290-326), `run_*` helper per case kind that fills an `observed` string, `read_fixture` using ISO C only (`fopen`/`fread`, 46-77; P6 forbids POSIX-only calls under strict `-std=c11`), the four-instance interleave shape (`run_interleave`, 177-221: A, B and solo stepped in lockstep, final digests compared), keep-state shape (`run_keeps_state`, 119-150: record time and digest, attempt the bad load, require unchanged).
**Change (D-57, D-60):** the spike runner JUDGES (`verdict = "PASS"/"FAIL"/"CONTROL-REJECTED"`, line 322, and `RUNNER pass=%d fail=%d` at 324, exit status from `fail`). Phase 2's runner never aggregates or decides required versus optional: print one line per case
```
RESULT <id> <outcome> assertions=<n> evidence=<relpath>
```
then `RUNNER_DONE <n>`; outcome names come from `outcomes.def` (X-macro), never string literals. Case table comes from the flattened TSV, not the spike's blank-split manifest parser (`split_fields`/`load_cases` 223-269). Fixed-size arrays and `MAX_CASES` guard are fine for the runner (tests may allocate; the core may not).
`--list` prints ids; `--emit <fixture>` writes generator bytes to stdout for the gate's `shasum` compare.

---

### 13. `tests/conformance/synth.c` and the mirror oracle

**Analog:** `spike-tool-01-evidence:spikes/tool-01/gen-fixtures.sh` line 25 (the LCG) and `consumer/expect.pl` (independent second implementation).
```sh
awk 'BEGIN { x = 12345; for (i = 0; i < 98304; i++) { x = (x * 1664525 + 1013904223) % 4294967296; printf "\\%03o", int(x / 65536) % 256 } }'
```
`synth.c` uses the same integer LCG (`x = x*1664525 + 1013904223 mod 2^32`, byte `(x >> 16) & 0xff`) so a fixed seed yields fixed bytes, plus a unique tag byte at the start and a second at the end of every 8 KiB bank (RESEARCH Pitfall 3). The expected bank signatures must come from an awk script that derives the mirror rule from the `LED-LOAD` text, never from the core's enum (D-59); model it on `expect.pl`'s header ("a second implementation of the spec ... not DuoSpool output"). The awk LCG product stays below 2^53 so it is exact on every awk (RESEARCH reuse table).

---

### 14. `tests/gate/<checker>/<case>/` mini-roots and `EXPECT`

**Analog:** `tests/provenance/<case>/` (contents: `EXPECT`, `PROVENANCE.md`, `src/...`) and `tests/licence/<case>/` (`EXPECT`, three LICENSE files, `scripts/stub.sh`), run by `provenance-check --root d --expect E`. `EXPECT` is one line of comma-separated codes fixed before the first run (`tests/licence/script-no-spdx/EXPECT` is the single line `SPDX_HEADER_MISSING`); `ok-<case>` must pass with no EXPECT. Grouping by checker (`tests/gate/inventory/<case>`, `tests/gate/fixture/<case>`, `tests/gate/rights/...`) lets each group be one `run_controls` call.
The five named controls (D-58): removed required test (`INV_REQUIRED_MISSING`), manifest missing a field (`FIX_MISSING_FIELD`), unlisted fixture (`FIX_UNLISTED`), wrong literal through the live runner (needs the C runner, expects `guest_failure`; judge with `conformance-ingest`, EXPECT may use the existing `CODE=` / `EXIT=` line syntax of `tests/score/*/EXPECT`, `scripts/check` 505-518), unlisted ROM-like binary (`ROMLIKE_BINARY`, created at run time, never in the tree).
Gotchas: a control file carrying a line-start `Provenance:` marker must be listed in `tests/provenance/SELF-FIXTURES` (exact root-relative paths, one per line); `tests/gate/**` files are scanned by `rights-check` and `licence-check` at repo root, so keep mini-root contents text-only with SPDX headers where the extended `licence-check` requires them.

---

### 15. `tests/conformance/cases/<id>.toml`, `fixtures/manifest/<id>.toml`

**Analog:** ledger entry files: strict subset, `id` equals the filename (rule `ID_FILENAME_MISMATCH`, `ledger-rules.awk`), single-line arrays, booleans allowed (`required = true`), 0BSD header, at most 16384 B. Keys: `id kind fixture expect meaning required control expect_outcome pins` (cases) and the D-59 list (manifest). Reject unknown keys and missing keys with a checker (same two-stage design, section 2); `tests/conformance/REQUIRED` mirrors `ledger/REQUIRED` (comments and blanks ignored, one id per line).

---

### 16. `.github/workflows/gate.yml` and repo config files

**Analog:** none in repo. Use RESEARCH lines 311-348 verbatim (actionlint 1.7.12 reported 0 errors on that file); add the gcc-then-clang build steps, the second-awk run and `env:`-passed base SHA for `audit-history --range`. Pin `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1` and re-verify with `git ls-remote https://github.com/actions/checkout refs/tags/v7.0.1` immediately before commit. Block-style YAML only, so `workflow-check` can parse it. `.github/pull_request_template.md` carries the record-0007 line verbatim (see section 8). `.gitattributes`: `* text=auto eol=lf`, binary fixtures `-text` (D-61). Record-0006 constraints (`docs/decisions/0006-ci-provider.md` Decision section): SHA pins, `permissions: contents: read`, no `pull_request_target`, `persist-credentials: false`, no secrets.

---

### 17. Docs edits

`README.md` line 11 ("Phase 1 gate ... There is no emulator code yet") is stale after this phase; `README.md` 14-18 already shows `sh scripts/check`. `AGENTS.md` has NO `sh scripts/check` string today (grep verified), so TOOL-02's `grep -F 'sh scripts/check'` on AGENTS.md needs an added line. `docs/decisions/0007-copied-code-provenance.md` line 43 says the line "moves into the pull-request template that arrives with the Phase 2 CI workflow"; update that sentence (run `sh scripts/decision-check docs/decisions` after, which checks front matter, index drift and the 0005 no-GPL sentence).

## Shared Patterns

### Exit codes and markers (all checkers and the gate)
**Source:** `scripts/licence-check` 13-14, `scripts/check` 1-9, 27-35. 0 = pass or `--expect` met, 1 = violations or control broken, 2 = harness error (usage, missing file, empty `--expect`, dead stage 2). Clean exit prints `EXPECT_OK`, `STATUS ...` or `RECORDS n`; exit 0 without one is `CHECKER_DEAD`, mapped to 2. No per-outcome exit codes (D-57).

### Fail-closed against vacuity
**Source:** WR-05 guard (`licence-check` 43-46), `HARNESS_NO_EXPECT` / `HARNESS_EMPTY_EXPECT` / `NO_CONTROLS` (`scripts/check` 56-90), `REQUIRED_EMPTY` (`ledger-check` 173), `NO_SENTINEL` (`profile-rules.awk` 69), `STAGE2_DEAD` (`ledger-check` 93-96). Every new rule ships with a control that proves it can go red, and a selftest mutation that deletes the rule.

### Mini-root and run-time controls
**Source:** `tests/provenance/`, `tests/licence/` (`--root`), `tests/audit/synthetic.sh` 25-61 (temp repos under `mktemp -d`, `trap 'rm -rf "$work"' EXIT`, no global git config). Anything ROM-shaped is created at run time, never committed.

### Harness hygiene from the spike
**Source:** `spike-tool-01-evidence:spikes/tool-01/gates.sh`. Every stage runs even if an earlier one failed and each prints its own line (74-84, 276-293); `trap 'exit 130' INT TERM` and a cleanup trap (64-72); wall-clock watchdog `perl -e 'alarm 120; exec @ARGV' "$bin" args` (111; expiry gives `harness_failure` reason `wall`, scale about 3x under sanitizers, never auto-retry, D-57); `leak_count` using `grep -c -a -F` on fixed needles (176-187) as the model for the D-56 redaction test; seeded-leak control (240-257) that must find a planted marker or the check is declared vacuous.

### Run-record and profile text
**Source:** `toml-subset.awk` limits (section 3) and D-69. Printed strings must not contain `"` or `\`; the printer returns an error status instead of emitting them, so `conformance-flatten` and any record lint can read records with the unchanged parser. Table names `[field_<name>]` are legal. The 13 field names are already duplicated in `scripts/lib/profile-rules.awk` line 21; the D-48 both-directions name-set check should cover that copy too.

### SPDX and licence scope
**Source:** `scripts/licence-check` 88-100. Both SPDX lines in the first five lines; today only `scripts/` is checked, so extending to `src/` and `tests/**/*.c|*.h` (D-71) means adding a `find` over those trees at the same site and a `tests/licence/<case>` control (`SPDX_HEADER_MISSING`). `fixtures/` and generated-data headers are `0BSD`, which the current `spdx_id` fixed string (line 28) would reject, so the extension must carry a per-tree expected id.

## No Analog Found

Planner should use RESEARCH.md patterns and the cited decisions instead:

| File | Role | Data Flow | Reason / where to look |
|---|---|---|---|
| `src/core/profile_fields.def`, `tests/conformance/outcomes.def` | X-macro table | transform | No C X-macros exist. Use D-48/D-57 and extract the same names in sh/awk (`outcome-sync`); closest data analog is the field list in `profile-rules.awk` 21-24 |
| `src/core/sha256.c` | util | transform | RESEARCH P10 (verified vectors, bit-length mutant, 21-length sweep). Write in-house from FIPS 180-4; no spike analog (spike used FNV) |
| splitmix64 in `src/core/poweron.c` | util | transform | RESEARCH P11 (seed 0 starts `0xE220A8397B1DCDAF`; seed 1477776061723855037 first value 1985237415132408290) |
| `tests/controls/*` toy libraries | test control | transform | RESEARCH Pattern 2; D-55 forbids `#ifdef` plants in the core |
| Allocation-counting shim | test util | event-driven | RESEARCH P9 (works on macOS; Linux interposition untested, add a self-check that calls `malloc` and requires count 1) |
| `scripts/ci-timings`, `scripts/linux-repro` | utility | request-response | D-63 (`gh run view --json`), RESEARCH P13 (docker `ubuntu:24.04`). Follow the section-2 header and exit-code conventions only |
| `.github/dependabot.yml`, `CODEOWNERS`, `.gitattributes`, `docs/ci-timings.md` | config / docs | n/a | D-61/D-62/D-63 text is the spec |

Researcher scratch with verified draft code exists under `/private/tmp/claude-501/-Users-jon-projects-duospool/06f2647a-e8db-469c-a561-38fda7909897/scratchpad/r2/` (`ds_sha256.c`, `sm.c`, `shim/`, `wf/`). It is untracked and ephemeral: do not cite it as an analog and do not depend on it existing. If present it may speed the executor, which must still re-prove it against the NIST and splitmix vectors.

## Gotchas the planner should schedule around

1. **Marker regex is closed.** `checker()` (check line 30) only accepts `EXPECT_OK`, `STATUS `, `RECORDS `. New checkers must print one of them.
2. **`provenance-check` scans the whole tree** for line-start `Provenance: PROV-nnnn` (prunes only `.git target build out prefix`). Any control fixture carrying one must be in `tests/provenance/SELF-FIXTURES`. `build/gate` is pruned.
3. **`audit-history.allow` is path-and-CHECK exact.** New files that trip ROM-size or text heuristics need a line there or must be generated at run time. `.gitignore` already blocks `*.bin *.pce *.iso *.cue *.sav *.srm`.
4. **`scripts/check` currently rejects any argument but a lone `--selftest`** (lines 672-678). Add `--timing` and keep the `usage:` text in sync with README.
5. **`profile-check` hard-codes `pce-hucard-2btn/r1.toml`** for controls (check line 197); do not refactor that stage into the generic loop (RESEARCH Pitfall 1).
6. **`ledger-check` stays structure-only**; `TEST:` pin resolution lives in `inventory-check` only (D-58). Create the case before flipping the pin.
7. **Repo `sh` is bash 3.2 on macOS and dash on Ubuntu.** No `[[ ]]`, arrays, `local`, `echo -e`, `case` inside `$( )`, `grep -P`, GNU-only flags. Use `printf`, `LC_ALL=C`, `mktemp -d`.
8. **Spike runner code must be reviewed line by line before reuse**: it decides verdicts (`PASS/FAIL/CONTROL-REJECTED`), uses `calloc`-allocated instances through a public API, and parses a non-TOML manifest. Only helper shapes carry over.

## Metadata

**Analog search scope:** `scripts/`, `scripts/lib/`, `tests/**`, `ledger/`, `profiles/`, `docs/decisions/`, `README.md`, `AGENTS.md`, `.gitignore`, and tag `spike-tool-01-evidence` (`spikes/tool-01/c/`, `gates.sh`, `gen-fixtures.sh`, `fixtures.manifest`, `consumer/expect.pl`).
**Files read in full or in targeted ranges:** about 22 repo files plus 8 spike files. No working-tree C source exists to read.
**Pattern extraction date:** 2026-11-09
