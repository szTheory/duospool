#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# synthetic.sh - negative controls for scripts/audit-history (T-01-26: an audit that cannot fail proves nothing).
# Each case builds a throwaway git repository under mktemp -d (removed by a trap) and runs the audit on it with an explicit
# --allow file. The real repository and the global git configuration are never read or written.
# No literal home path, key header or token appears in this file: such strings are assembled at run time from parts, so
# this file does not match the audit's own patterns.
# stdout: one line per case  AUDITTEST <case> PASS|FAIL
# exit  : 0 every case passed | 1 a case failed | 2 harness error
set -eu
LC_ALL=C; export LC_ALL
here=$(cd "$(dirname "$0")" && pwd)
audit="$here/../../scripts/audit-history"
[ -f "$audit" ] || { echo "HARNESS_NO_FILE $audit" >&2; exit 2; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
failed=0
ctl=AUDITTEST
# the throwaway-repository helpers (g, new, commit, verdict, has) and the isolated git configuration
. "$here/../lib/gitfix.sh"

# audit REPO ALLOWFILE: run the audit; sets $out and $rc
audit() {
  if out=$(sh "$audit" --allow "$2" "$1" 2>&1); then rc=0; else rc=$?; fi
}

# audit_range REPO ALLOWFILE RANGE: run the audit in --range mode; sets $out and $rc
audit_range() {
  if out=$(sh "$audit" --allow "$2" --range "$3" "$1" 2>&1); then rc=0; else rc=$?; fi
}

empty_allow="$work/empty.allow"
printf '# empty\n' > "$empty_allow"
tab=$(printf '\t')

# ---- rom-blob: an 8192-byte zero-filled x.pce ----
d=$(new rom-blob)
dd if=/dev/zero of="$d/x.pce" bs=8192 count=1 2>/dev/null
commit "$d" rom
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -ne 0 ] && has NUL-BYTES && has ROM-SIZE-8K && has EXT; then ok=1; fi
verdict rom-blob "$ok"

# ---- missing-object: delete the loose object of a committed file ----
d=$(new missing-object)
printf 'some content for a loose object\n' > "$d/f.txt"
commit "$d" file
h=$(git -C "$d" hash-object "$d/f.txt")
rest=${h#??}
pre=${h%"$rest"}
rm -f "$d/.git/objects/$pre/$rest"
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -ne 0 ] && has UNREADABLE; then ok=1; fi
verdict missing-object "$ok"

# ---- home-path: a text file holding a macOS home-directory path of a made-up user ----
d=$(new home-path)
printf 'see %s%s%s for notes\n' "/Us" "ers/" "someone/projects/x" > "$d/notes.txt"
commit "$d" notes
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -eq 1 ] && has TEXT-PATTERN && has notes.txt; then ok=1; fi
verdict home-path "$ok"

# ---- allowlisted-ok: an allowlisted NUL-free 16384-byte text file ----
allow_rom="$work/rom.allow"
printf 'ok/x16384.txt%sROM-SIZE-8K%sNUL-free size-boundary control\n' "$tab" "$tab" > "$allow_rom"
d=$(new allowlisted-ok)
mkdir -p "$d/ok"
head -c 16384 /dev/zero | tr '\000' 'A' > "$d/ok/x16384.txt"
commit "$d" boundary
audit "$d" "$allow_rom"
ok=0
if [ "$rc" -eq 0 ] && has 'audit-history: clean'; then ok=1; fi
verdict allowlisted-ok "$ok"

# ---- allowlisted-binary: the allowlisted path later gains NUL bytes ----
d=$(new allowlisted-binary)
mkdir -p "$d/ok"
head -c 16384 /dev/zero | tr '\000' 'A' > "$d/ok/x16384.txt"
commit "$d" boundary
head -c 16384 /dev/zero > "$d/ok/x16384.txt"
commit "$d" "now binary"
audit "$d" "$allow_rom"
ok=0
if [ "$rc" -eq 1 ] && { has NUL-BYTES || has ROM-SIZE-8K; }; then ok=1; fi
verdict allowlisted-binary "$ok"

# ---- clean ----
d=$(new clean)
printf 'plain text\n' > "$d/a.txt"
commit "$d" plain
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -eq 0 ] && has 'audit-history: clean'; then ok=1; fi
verdict clean "$ok"

# ---- self-scan: the only content is a copy of scripts/audit-history ----
d=$(new self-scan)
mkdir -p "$d/scripts"
cp "$audit" "$d/scripts/audit-history"
commit "$d" "copy of the audit"
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -eq 0 ] && has 'audit-history: clean'; then ok=1; fi
verdict self-scan "$ok"

# ---- git-read-failure: a ref points at an object that does not exist; the read must fail with exit 2, never clean ----
d=$(new git-read-failure)
printf 'content\n' > "$d/a.txt"
commit "$d" plain
br=$(g "$d" symbolic-ref --short HEAD)
printf '%s\n' 0123456789abcdef0123456789abcdef01234567 > "$d/.git/refs/heads/$br"
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -eq 2 ] && ! has 'audit-history: clean'; then ok=1; fi
verdict git-read-failure "$ok"

# ---- dup-blob-path: one blob under two paths; rev-list --objects names it once, the .pce path must still be flagged ----
d=$(new dup-blob-path)
printf 'identical content\n' > "$d/a.txt"
printf 'identical content\n' > "$d/b.pce"
commit "$d" dup
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -eq 1 ] && has 'EXT' && has 'b.pce'; then ok=1; fi
verdict dup-blob-path "$ok"

# ---- tag-only: the offending blob is reachable only from a tag ----
d=$(new tag-only)
printf 'plain\n' > "$d/a.txt"
commit "$d" base
main_br=$(g "$d" symbolic-ref --short HEAD)
g "$d" checkout -q -b side
dd if=/dev/zero of="$d/y.pce" bs=8192 count=1 2>/dev/null
commit "$d" "rom on side"
g "$d" tag evidence
g "$d" checkout -q "$main_br"
g "$d" branch -q -D side
audit "$d" "$empty_allow"
ok=0
if [ "$rc" -eq 1 ] && has y.pce; then ok=1; fi
verdict tag-only "$ok"

# ---- shallow-clone: a --depth 1 clone of a two-commit repository holds a cut-off history; the audit must refuse with exit 2,
# never print clean (CR-02). The control is only meaningful if the clone really is shallow and the full source audits clean.
d=$(new shallow-source)
printf 'first\n' > "$d/a.txt"
commit "$d" first
printf 'second\n' > "$d/b.txt"
commit "$d" second
audit "$d" "$empty_allow"
full_ok=0
if [ "$rc" -eq 0 ] && has 'audit-history: clean'; then full_ok=1; fi
shallow_dir="$work/shallow-clone"
git clone -q --depth 1 "file://$d" "$shallow_dir"
is_shallow=$(git -C "$shallow_dir" rev-parse --is-shallow-repository)
audit "$shallow_dir" "$empty_allow"
ok=0
if [ "$full_ok" -eq 1 ] && [ "$is_shallow" = true ] && [ "$rc" -eq 2 ] && ! has 'audit-history: clean' && has 'shallow'; then ok=1; fi
verdict shallow-clone "$ok"

# ---- range-clean: a bad blob committed at BASE is outside BASE..HEAD, so the range audit is clean; the full audit of the same
# repository must be dirty, otherwise the control would pass for a repository that holds nothing bad ----
d=$(new range-clean)
dd if=/dev/zero of="$d/x.pce" bs=8192 count=1 2>/dev/null
commit "$d" "rom before the range"
base=$(g "$d" rev-parse HEAD)
printf 'later change\n' > "$d/b.txt"
commit "$d" "clean change"
audit "$d" "$empty_allow"
full_rc=$rc
audit_range "$d" "$empty_allow" "$base..HEAD"
ok=0
if [ "$full_rc" -eq 1 ] && [ "$rc" -eq 0 ] && has 'audit-history: clean' && has 'range'; then ok=1; fi
verdict range-clean "$ok"

# ---- range-dirty: a bad blob added inside the range is reported, by path ----
d=$(new range-dirty)
printf 'first\n' > "$d/a.txt"
commit "$d" first
base=$(g "$d" rev-parse HEAD)
dd if=/dev/zero of="$d/y.pce" bs=8192 count=1 2>/dev/null
commit "$d" "rom inside the range"
audit_range "$d" "$empty_allow" "$base..HEAD"
ok=0
if [ "$rc" -eq 1 ] && has y.pce && has NUL-BYTES; then ok=1; fi
verdict range-dirty "$ok"

# ---- range-base-missing: BASE is not in the repository (a depth-limited fetch): exit 2, never clean ----
d=$(new range-base-missing)
printf 'content\n' > "$d/a.txt"
commit "$d" plain
audit_range "$d" "$empty_allow" "0123456789abcdef0123456789abcdef01234567..HEAD"
ok=0
if [ "$rc" -eq 2 ] && ! has 'audit-history: clean'; then ok=1; fi
verdict range-base-missing "$ok"

# ---- range-text: a home path that was already there at BASE is not reported again, one added in the range is ----
d=$(new range-text)
printf 'see %s%s%s for notes\n' "/Us" "ers/" "someone/projects/x" > "$d/old.txt"
commit "$d" "old note"
base=$(g "$d" rev-parse HEAD)
printf 'see %s%s%s for more\n' "/Us" "ers/" "someone/projects/y" > "$d/new.txt"
commit "$d" "new note"
audit_range "$d" "$empty_allow" "$base..HEAD"
ok=0
if [ "$rc" -eq 1 ] && has TEXT-PATTERN && has new.txt && ! has old.txt; then ok=1; fi
verdict range-text "$ok"

# ---- text-allow-kind: a TEXT-PATTERN:KIND allowance hides that kind in that path only (the owner-approved shape for the two
# executor summaries); a second path, and a second kind in the same path, are still reported ----
kind_allow="$work/kind.allow"
printf 'notes.txt%sTEXT-PATTERN:home-path-mac%sexecutor summary, no credential\n' "$tab" "$tab" > "$kind_allow"
d=$(new text-allow-kind-ok)
printf 'see %s%s%s for notes\n' "/Us" "ers/" "someone/projects/x" > "$d/notes.txt"
commit "$d" notes
audit "$d" "$kind_allow"
ok=0
if [ "$rc" -eq 0 ] && has 'audit-history: clean'; then ok=1; fi
verdict text-allow-kind-ok "$ok"

d=$(new text-allow-kind-other-path)
printf 'see %s%s%s for notes\n' "/Us" "ers/" "someone/projects/x" > "$d/notes.txt"
printf 'see %s%s%s for more\n' "/Us" "ers/" "someone/projects/y" > "$d/more.txt"
commit "$d" notes
audit "$d" "$kind_allow"
ok=0
if [ "$rc" -eq 1 ] && has TEXT-PATTERN && has more.txt && ! has notes.txt; then ok=1; fi
verdict text-allow-kind-other-path "$ok"

d=$(new text-allow-kind-other-kind)
printf 'see %s%s%s for notes\n' "/Us" "ers/" "someone/projects/x" > "$d/notes.txt"
printf 'and %s%s%s for linux\n' "/ho" "me/" "someone/projects/z" >> "$d/notes.txt"
commit "$d" notes
audit "$d" "$kind_allow"
ok=0
if [ "$rc" -eq 1 ] && has 'home-path-linux' && ! has 'home-path-mac'; then ok=1; fi
verdict text-allow-kind-other-kind "$ok"

[ "$failed" -eq 0 ] || exit 1
exit 0
