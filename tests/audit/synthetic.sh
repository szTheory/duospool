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

GIT_CONFIG_NOSYSTEM=1; export GIT_CONFIG_NOSYSTEM
GIT_CONFIG_GLOBAL=/dev/null; export GIT_CONFIG_GLOBAL

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
failed=0

# g REPO GIT-ARGS...
g() {
  r=$1
  shift
  git -c user.name=duospool-test -c user.email=test@example.invalid -c commit.gpgsign=false -C "$r" "$@"
}

# new CASE: create the repository directory and print its path
new() {
  d="$work/$1"
  mkdir -p "$d"
  g "$d" init -q
  printf '%s\n' "$d"
}

# commit REPO MESSAGE: stage everything and commit
commit() {
  g "$1" add -A
  g "$1" commit -q -m "$2"
}

# audit REPO ALLOWFILE: run the audit; sets $out and $rc
audit() {
  if out=$(sh "$audit" --allow "$2" "$1" 2>&1); then rc=0; else rc=$?; fi
}

# verdict CASE OK-FLAG: print the result line, count failures
verdict() {
  if [ "$2" = 1 ]; then
    echo "AUDITTEST $1 PASS"
  else
    echo "AUDITTEST $1 FAIL"
    printf '%s\n' "$out" | sed 's/^/  | /'
    echo "  | exit status $rc"
    failed=1
  fi
}

# has TEXT: is TEXT a fixed substring of $out
has() {
  printf '%s\n' "$out" | grep -qF -- "$1"
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

[ "$failed" -eq 0 ] || exit 1
exit 0
