---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
reviewed: 2026-11-10T00:00:00Z
depth: standard
files_reviewed: 28
files_reviewed_list:
  - src/core/ds_core.h
  - src/core/ds_mapper.h
  - src/core/ds_poweron.h
  - src/core/ds_serial.h
  - src/core/ds_sha256.h
  - src/core/boot.c
  - src/core/mapper.c
  - src/core/profile.c
  - src/core/poweron.c
  - src/core/advance.c
  - src/core/serial.c
  - src/core/sha256.c
  - src/core/refusals.def
  - src/core/boot_order.def
  - src/core/profile_fields.def
  - src/text/ds_text.h
  - src/text/text.c
  - CMakeLists.txt
  - CMakePresets.json
  - .github/workflows/gate.yml
  - scripts/conformance-ingest
  - scripts/outcome-sync
  - scripts/inventory-check
  - scripts/rights-check
  - scripts/core-boundary-check
  - scripts/audit-history
  - scripts/workflow-check
  - scripts/fixture-check
findings:
  critical: 1
  warning: 4
  info: 3
  total: 8
status: issues_found
---

# Phase 2: Code Review Report

**Reviewed:** 2026-11-10
**Depth:** standard
**Files Reviewed:** 28
**Status:** issues_found

## Summary

The C core and text printer were traced by hand for out-of-bounds access, size arithmetic, alignment, refusal precedence, SHA-256 padding and text escaping. No defect was found in them. Specifically checked and sound:

- `ds_instance` layout: every field is naturally aligned and `powerup_seed` sits at offset 8736.
- `check_memory` tests alignment as an integer before any cast.
- `put_ch` keeps one byte free for the terminator, and every error path in `text.c` wipes the buffer.
- `ds_sha256_final` handles fill values 55, 56 and 63 correctly, and the bit length is a `uint64_t`.
- Mirror tables stay inside 1 MiB: the highest index read for 384K is 0x2F * 8192 + 8191, below 393216.
- `copy_value` and `in_allowed` have no off-by-one.
- All shifts are on unsigned operands.

The gate workflow keeps `contents: read`, a full-SHA pinned checkout, `persist-credentials: false`, and routes the only untrusted-looking value (`base.sha`) through `env`. No finding there.

The defects are in the shell/awk gate scripts, where the project's own rule ("no pipeline: a failing stage must not read as no violations") is not applied everywhere. One of them was reproduced and is a vacuous pass. One core hardening gap concerns overlapping caller memory.

The necessity pass (#4100) was not performed (necessity-review.md was not loaded), so no `necessity:` count is emitted.

## Critical Issues

### CR-01: Gate scripts pipe awk into `sort`, so a failing awk reads as "no violations" (reproduced in outcome-sync)

**File:** `scripts/outcome-sync:68-79` and `:83-91` (primary); same pattern at `scripts/core-boundary-check:215` and `:233`, `scripts/rights-check:154`, `scripts/audit-history:145`

**Issue:** The check program in `outcome-sync` is run as `req=... awk '...' "$tmp/names" | sort -u > "$viol"`. POSIX `sh` has no `pipefail`, so the pipeline status is `sort`'s. If the awk program dies (syntax or dialect error, runtime error), `$viol` stays empty and the script prints `STATUS outcome-sync ok` and exits 0.

Reproduced: with the real awk and `DS_OUTCOME(skip)` renamed to `DS_OUTCOME(bogus)`, the script reports `OUTSYNC_NAME`, rc=1. With an `awk` shim on PATH that exits 2 on the check program, the same tree prints `STATUS outcome-sync ok`, rc=0. The CI job deliberately runs the gate under both mawk and gawk because dialect drift is expected. A dialect error in this one program would pass silently on exactly that axis.

The same shape exists in:
- `core-boundary-check:215`, where the `awk | sort -u > undef` result feeds `CB_UNDEF_SYMBOL`.
- `core-boundary-check:233`, where `wr=$(awk ... | sort -u)` feeds `CB_WRITABLE`.
- `rights-check:154`, where `sha256_of` feeds the hash comparison.
- `audit-history:145`, which feeds the blob hash list.

The other scripts already follow the safe form (`awk ... > file || { ...; exit 2; }`), so this is an inconsistency, not a design choice.

**Fix:** Redirect to a file and test the status, then sort the file. Optionally also require that the program printed a sentinel (as inventory-check does with `STAGE2_DEAD`).
```sh
req="$required" awk '...' "$tmp/names" > "$tmp/viol.raw" || { echo "HARNESS_AWK_FAILED outcome-sync" >&2; exit 2; }
sort -u "$tmp/viol.raw" > "$viol"
```
Apply the same change to the `--names` block (lines 83-91) and to the four other sites.

## Warnings

### WR-01: rights-check hash comparison passes when the hash tool fails and the manifest has no `sha256`

**File:** `scripts/rights-check:151-155, 224-238`

**Issue:** `sha256_of` is `$sha_cmd "$1" | awk '{ print $1 }'`. The tool's failure is hidden by the pipeline, and `$1` is passed without a `./` prefix. `want=$(mval sha256)` is empty when the manifest entry lacks the key, and nothing validates its format. `[ "$got" != "$want" ]` then compares two empty strings and the entry is accepted.

Reproduced: for a tracked file named `-x`, `sha256sum "-x" 2>/dev/null | awk '{print $1}'` yields an empty string. `[ "$got" != "" ]` is false, so the entry is not reported as `RIGHTS_HASH_MISMATCH`.

The `case $src` guard on line 227 rejects `..` and absolute paths but not a leading `-`. The same unprefixed use appears in `inspect` at line 184 (`head -c 8 "$f"`), so the MAGIC rule is skipped for a dash-named file. This lets a ROM-like file named `-something.bin`, or any file the hash tool cannot read, be excused with no content binding. The excuse is meant to be bound to the committed bytes.

**Fix:**
```sh
case $want in
  [0-9a-f][0-9a-f]*) [ ${#want} -eq 64 ] || { bad "$m" RIGHTS_BAD_ALLOW "sha256 is not 64 hex digits"; return 0; } ;;
  *) bad "$m" RIGHTS_BAD_ALLOW "sha256 missing or malformed"; return 0 ;;
esac
# and capture the tool status:
sha256_of() { out=$($sha_cmd "./$1" 2>/dev/null) || return 1; printf '%s\n' "${out%% *}"; }
```
Also reject a `-*` source in the `case $src` list and use `./$f` in `inspect`.

### WR-02: `ds_boot` does not reject caller memory that overlaps the request or the content

**File:** `src/core/boot.c:306-321`

**Issue:** After all checks pass, `ds_boot` zeroes `sizeof(struct ds_instance)` bytes at `mem`. It then reads `request->byte_order`, `request->header`, `request->len`, `request->has_seed`, `request->powerup_seed` and `request->content` (lines 312-319), and does `memcpy(inst->rom, request->content, request->len)`. If the request struct or the content buffer lies inside `[mem, mem + sizeof(struct ds_instance))`, the memset has already destroyed it.

The memcpy then copies zeros, or has overlapping operands, which is undefined behaviour. The boot returns `DS_OK` with a `content_id` that is the SHA-256 of a zeroed or garbled image. That is a silent wrong-evidence result at the one boundary the project treats as untrusted. The header promises "a refusal writes nothing but result: never mem", but no refusal exists for this case.

A host that reuses one arena for both the loaded file and the instance is the realistic trigger.

**Fix:** Add a `check_memory`-style row (or extend `check_memory`) that refuses overlap using integer address comparison, with a `request_invalid` or `memory_invalid` refusal.
```c
static int overlaps(uintptr_t a, size_t an, uintptr_t b, size_t bn)
{
    return a < b + bn && b < a + an; /* guard b + bn / a + an wrap with an explicit check first */
}
```
Test it against `request`, `request->content` and `result`. Alternatively, copy the scalars out of `request` before the memset and copy the content into a temporary-free path that cannot overlap.

### WR-03: core-boundary-check CB_FLOAT only matches the tokens `float` and `double`

**File:** `scripts/core-boundary-check:136`

**Issue:** The rule is `(^|[^A-Za-z0-9_])(float|double)([^A-Za-z0-9_]|$)` on comment- and literal-stripped source. It does not see:
- floating constants used in integer-context arithmetic (`x = n * 1.5;`, `1e3`, `0x1p3`);
- the `_Float16/32/64/128`, `_Decimal*`, `__fp16` and `__float128` type names (`_Float32` has a `_` prefix and uppercase `F`);
- `_Complex` used without a `double`/`float` token.

On x86-64 and arm64 hard-float targets none of these produces an undefined libcall, so the layer-3 symbol check does not catch them either. The "no floating point in the core" rule is therefore weaker than documented (D-54, CORE-02).

**Fix:** Extend the token regex to `_Float[0-9]+x?|_Decimal[0-9]+|__fp16|__float128|_Complex`, and add a numeric-literal rule such as `[0-9]\.[0-9]|[0-9]\.([^0-9A-Za-z_]|$)|[0-9][eE][-+]?[0-9]|0[xX][0-9a-fA-F.]*[pP]` applied after literal stripping.

### WR-04: audit-history can report "clean" after reading nothing, and its full-history text scan assumes SHA-1

**File:** `scripts/audit-history:145-157, 213-217`

**Issue (a):** There is no guard on the counts. If the `cat-file --batch-check` classification yields zero blobs, `nblobs` and `npaths` are 0 and step 4 finds no `h in sz` matches. The script then prints `audit-history: scanned N commits, 0 blobs, 0 blob-path pairs` and `audit-history: clean`, exit 0. The header itself describes the missing-`%(rest)` bug as the way this audit "passes vacuously", yet only that one cause is defended.

**Issue (b):** Full mode parses `git grep` output with `substr($0, 42)`, which hard-codes a 40-hex commit plus a colon. In a SHA-256 object-format repository, or if git changes the output prefix, every text pattern match is silently dropped and the credential and home-path scans report nothing.

**Fix:** After step 3, `[ "$nblobs" -gt 0 ] && [ "$npaths" -gt 0 ] || { echo "audit-history: no blobs read from $ncommits commits" >&2; exit 2; }`. For (b), strip the commit prefix with `sub(/^[^:]*:/, "")` (or use `git grep --full-name` with `-z`) instead of a fixed offset, and check `git rev-parse --show-object-format` is `sha1` or handle both.

## Info

### IN-01: fixture-check reads tab-separated records with `IFS=$tab`, which collapses empty fields

**File:** `scripts/fixture-check:104-110`

**Issue:** `while IFS="$tab" read -r tag a b c` is the pattern that `inventory-check.sh:103` documents as wrong ("a TAB is IFS white space for read, which would collapse an empty field"). A stage-2 `M` record with an empty size or sha field is shifted left, so `manifest.tsv` gets the wrong columns. The case is still caught because stage 2 also emits a `V` missing-field record for the same entry, so no pass results. The reported code can be a misleading `FIX_HASH_MISMATCH` instead of the real field fault.

**Fix:** Use the awk transform that `inventory-check` uses (lines 104-107) for these records.

### IN-02: The state digest omits `mapper`, `byte_order` and `header` of the instance

**File:** `src/core/serial.c:120-186` (region table), `src/core/ds_core.h:117-120`

**Issue:** The serializer's header says a field "could be added to the struct and silently missed" is what the framed table prevents. Three instance fields are not in any region: `mapper`, `byte_order` and `header`. Today they are effectively constants, since only `plain`, `pce` and `none` boot and the profile hash carries the bound values. But a future mapper or header handling would change behaviour without changing the digest.

**Fix:** Either add a small `LOAD` region (u32 byte_order, u32 header, the mapper name via `w_str`) and bump `DS_SERIAL_VERSION`, or add a comment and a test recording that the omission is deliberate.

### IN-03: REQUIRED parsing differs between conformance-ingest and inventory-check

**File:** `scripts/conformance-ingest:139`, `scripts/inventory-check:74-77`

**Issue:** `conformance-ingest` takes `$1` of every non-comment line of REQUIRED, so `id trailing words` or `id # note` is read as `id`. `inventory-check` accepts only `^[a-z][a-z0-9_]*[ \t]*$` and reports anything else as `INV_BAD_LINE`. The gate fails either way because `inventory-check` runs too, but the two judges disagree about the same file, and `conformance-ingest` run alone (for example by a control) accepts lines the inventory would reject.

**Fix:** Make `conformance-ingest` apply the same grammar and print a bad-line violation for a non-conforming line instead of silently truncating it to `$1`.

---

_Reviewed: 2026-11-10_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
