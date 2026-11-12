#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# synthetic.sh - run-time controls for scripts/rights-check (FIX-05, D-56, D-76; T-02-18: a rights gate that cannot fail proves
# nothing). Each case builds a throwaway git repository under mktemp -d (removed by a trap), plants the file under test there
# with git add -f, and runs the checker on it with --root. No ROM-like file is ever committed to the real repository: every
# planted file exists only inside a temporary repository, created at run time. The global git configuration is never read.
# RIGHTS_CHECK selects the checker under test (default scripts/rights-check); the gate's mutation selftest points it at a copy.
# stdout: one line per case  RIGHTSTEST <case> PASS|FAIL
# exit  : 0 every case passed | 1 a case failed | 2 harness error
set -eu
LC_ALL=C; export LC_ALL
here=$(cd "$(dirname "$0")" && pwd)
rights=${RIGHTS_CHECK:-$here/../../scripts/rights-check}
[ -f "$rights" ] || { echo "HARNESS_NO_FILE $rights" >&2; exit 2; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
failed=0
ctl=RIGHTSTEST
out=
rc=0
# the throwaway-repository helpers (g, new, commit, verdict, has) and the isolated git configuration
. "$here/../lib/gitfix.sh"

# run_rights REPO [ARGS...]: run the checker on REPO; sets $out and $rc
run_rights() {
  rr_repo=$1
  shift
  if out=$(sh "$rights" "$@" --root "$rr_repo" 2>&1); then rc=0; else rc=$?; fi
}

# negative REPO CODES : the checker must meet --expect CODES (exact code set) and, run without --expect, must exit 1.
# Sets neg_ok to 1 or 0; $out holds the output of the bare run so that has() can inspect the reason keywords.
negative() {
  run_rights "$1" --expect "$2"
  neg_ok=0
  if [ "$rc" -eq 0 ] && has EXPECT_OK; then neg_ok=1; fi
  run_rights "$1"
  if [ "$rc" -ne 1 ]; then neg_ok=0; fi
}

tab=$(printf '\t')

# sha OF-FILE : the sha256 of a file, chosen the same way as the checker
sha() {
  if command -v sha256sum > /dev/null 2>&1; then sha256sum "$1"; else shasum -a 256 "$1"; fi | awk '{ print $1 }'
}

# manifest REPO ID SOURCE SHA KIND LICENCE : write fixtures/manifest/ID.toml, redistribution committed
manifest() {
  mkdir -p "$1/fixtures/manifest"
  {
    printf 'id = "%s"\n' "$2"
    printf 'kind = "%s"\n' "$5"
    printf 'source = "%s"\n' "$3"
    printf 'recipe = "written by hand for this control"\n'
    printf 'toolchain = "none"\n'
    printf 'size = 1\n'
    printf 'sha256 = "%s"\n' "$4"
    printf 'licence = "%s"\n' "$6"
    printf 'notices = "none"\n'
    printf 'redistribution = "committed"\n'
    printf 'meaning = "rights-check control"\n'
    printf 'verdict_source = "independent-script"\n'
  } > "$1/fixtures/manifest/$2.toml"
}

# ---- negatives: each plants one file (or one manifest entry) in a throwaway repository, and must meet its exact code set ----

# planted-pce: a printable text file whose only fault is its name, so only the extension rule can catch it
d=$(new planted-pce)
printf 'not a rom, only a name\n' > "$d/x.pce"
commit "$d" rom
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has "0${tab}EXT" || ok=0
verdict planted-pce "$ok"

# random-8192-no-extension: 8192 random bytes under a plain name, caught by content
d=$(new random-8192-no-extension)
dd if=/dev/urandom of="$d/blob" bs=8192 count=1 2> /dev/null
commit "$d" random
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has "0${tab}NUL" || ok=0
has 'SHAPE size 8192' || ok=0
verdict random-8192-no-extension "$ok"

# txt-with-rom-bytes: a .txt of 8704 (8192 plus 512) non-ASCII bytes
d=$(new txt-with-rom-bytes)
head -c 8704 /dev/zero | tr '\000' '\251' > "$d/notes.txt"
commit "$d" romtxt
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has 'SHAPE size 8704' || ok=0
has NONASCII || ok=0
verdict txt-with-rom-bytes "$ok"

# fill-ea-no-nul: 8192 bytes of 0xEA, no NUL byte anywhere (a NUL-only rule would miss it)
d=$(new fill-ea-no-nul)
head -c 8192 /dev/zero | tr '\000' '\352' > "$d/fill"
commit "$d" fill
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has "0${tab}UTF8" || ok=0
has 'SHAPE size 8192' || ok=0
if has "0${tab}NUL"; then ok=0; fi
verdict fill-ea-no-nul "$ok"

# shaped-ascii-high-byte: 8192 bytes, printable ASCII but for one 0xEA byte: not valid text, so the shape rule applies (D-76)
d=$(new shaped-ascii-high-byte)
{ head -c 8191 /dev/zero | tr '\000' 'A'; printf '\352'; } > "$d/shaped"
commit "$d" shaped
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has "0${tab}UTF8" || ok=0
has 'SHAPE size 8192' || ok=0
verdict shaped-ascii-high-byte "$ok"

# zip-magic: a zip header followed by text
d=$(new zip-magic)
{ printf 'PK\003\004'; printf 'some text after the header\n'; } > "$d/pack"
commit "$d" zip
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has 'MAGIC zip' || ok=0
verdict zip-magic "$ok"

# hex-blob: a .txt holding one 1100-character hex line (a text file is not exempt from the blob rule)
d=$(new hex-blob)
head -c 1100 /dev/zero | tr '\000' 'a' > "$d/dump.txt"
printf '\n' >> "$d/dump.txt"
commit "$d" hex
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has "1${tab}BLOB" || ok=0
verdict hex-blob "$ok"

# long-line: one 5000-byte line of characters that are neither hex nor base64
d=$(new long-line)
head -c 5000 /dev/zero | tr '\000' '-' > "$d/wide.txt"
printf '\n' >> "$d/wide.txt"
commit "$d" wide
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has 'LINELEN 5000' || ok=0
if has BLOB; then ok=0; fi
verdict long-line "$ok"

# listed-wrong-hash: a clean text fixture listed in the manifest with the wrong sha256
d=$(new listed-wrong-hash)
mkdir -p "$d/fixtures"
printf 'authored fixture text\n' > "$d/fixtures/a.txt"
manifest "$d" a fixtures/a.txt 0000000000000000000000000000000000000000000000000000000000000000 authored 0BSD
commit "$d" listed
negative "$d" RIGHTS_HASH_MISMATCH
verdict listed-wrong-hash "$neg_ok"

# manifest-no-file: an entry whose path does not exist
d=$(new manifest-no-file)
printf 'plain\n' > "$d/a.txt"
manifest "$d" gone fixtures/gone.bin 0000000000000000000000000000000000000000000000000000000000000000 authored 0BSD
commit "$d" stale
negative "$d" RIGHTS_MISSING_ARTIFACT
verdict manifest-no-file "$neg_ok"

# listed-kind-synthetic: right hash, but the kind is not authored
d=$(new listed-kind-synthetic)
mkdir -p "$d/fixtures"
printf 'generated fixture text\n' > "$d/fixtures/s.txt"
manifest "$d" s fixtures/s.txt "$(sha "$d/fixtures/s.txt")" synthetic 0BSD
commit "$d" listed
negative "$d" RIGHTS_BAD_ALLOW
verdict listed-kind-synthetic "$neg_ok"

# WR-01: the excuse is bound to the committed bytes, so an entry whose sha256 is missing or malformed excuses nothing. The unlisted hash
# below is the case that used to pass: with no sha256 key both digests were empty strings and compared equal.

# listed-no-sha256: a clean text fixture listed with no sha256 key at all
d=$(new listed-no-sha256)
mkdir -p "$d/fixtures"
printf 'authored fixture text\n' > "$d/fixtures/n.txt"
manifest "$d" n fixtures/n.txt 0000000000000000000000000000000000000000000000000000000000000000 authored 0BSD
grep -v '^sha256 = ' "$d/fixtures/manifest/n.toml" > "$d/fixtures/manifest/n.toml.new"
mv "$d/fixtures/manifest/n.toml.new" "$d/fixtures/manifest/n.toml"
commit "$d" listed
negative "$d" RIGHTS_BAD_ALLOW
ok=$neg_ok
has 'sha256 missing' || ok=0
verdict listed-no-sha256 "$ok"

# listed-malformed-sha256: the digest is present but is not 64 lower-case hex digits
d=$(new listed-malformed-sha256)
mkdir -p "$d/fixtures"
printf 'authored fixture text\n' > "$d/fixtures/m.txt"
manifest "$d" m fixtures/m.txt abc123 authored 0BSD
commit "$d" listed
negative "$d" RIGHTS_BAD_ALLOW
ok=$neg_ok
has 'sha256 missing' || ok=0
verdict listed-malformed-sha256 "$ok"

# dash-source-no-sha256: a ROM-like name that starts with a dash, listed with no sha256: the hash tool cannot read it as an operand, which used
# to make both digests empty and excuse the file. The source path is refused, so the extension hit stays.
d=$(new dash-source-no-sha256)
printf 'not a rom, only a name\n' > "$d/-x.pce"
manifest "$d" dash -x.pce 0000000000000000000000000000000000000000000000000000000000000000 authored 0BSD
grep -v '^sha256 = ' "$d/fixtures/manifest/dash.toml" > "$d/fixtures/manifest/dash.toml.new"
mv "$d/fixtures/manifest/dash.toml.new" "$d/fixtures/manifest/dash.toml"
commit "$d" dash
negative "$d" ROMLIKE_BINARY,RIGHTS_BAD_ALLOW
ok=$neg_ok
has 'unsafe source path' || ok=0
has "-x.pce${tab}ROMLIKE_BINARY" || ok=0
verdict dash-source-no-sha256 "$ok"

# dash-name-magic: a file whose name starts with a dash and whose content starts with a zip header: head -c 8 -name used to fail, so the MAGIC rule
# was skipped for it
d=$(new dash-name-magic)
{ printf 'PK\003\004'; printf 'some text after the header\n'; } > "$d/-pack"
commit "$d" dashzip
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has 'MAGIC zip' || ok=0
verdict dash-name-magic "$ok"

# ---- positives: each must exit 0 ----

# ok-listed-authored: an authored fixture with a 5000-byte line, listed with the right hash. Without its manifest entry the same
# tree is rejected, so the pass below is the entry's doing and not an accident of the content.
d=$(new ok-listed-authored)
mkdir -p "$d/fixtures"
{ head -c 5000 /dev/zero | tr '\000' '-'; printf '\n'; } > "$d/fixtures/wide.txt"
commit "$d" fixture
run_rights "$d"
unlisted_rc=$rc
manifest "$d" wide fixtures/wide.txt "$(sha "$d/fixtures/wide.txt")" authored 0BSD
commit "$d" listed
run_rights "$d"
ok=0
if [ "$unlisted_rc" -eq 1 ] && [ "$rc" -eq 0 ] && has 'RECORDS 2'; then ok=1; fi
verdict ok-listed-authored "$ok"

# ok-utf8-prose: valid UTF-8 prose with a superscript two after CD-ROM
d=$(new ok-utf8-prose)
printf 'The CD-ROM\302\262 drive is mentioned here.\n' > "$d/prose.txt"
commit "$d" prose
run_rights "$d"
ok=0
if [ "$rc" -eq 0 ] && has 'RECORDS 1'; then ok=1; fi
verdict ok-utf8-prose "$ok"

# ok-ascii-16384: 16384 bytes of printable ASCII in 64-byte lines: an 8 KiB multiple that is valid text, so the shape rule is
# not applied (D-76). The two Phase 1 boundary-size control files are exactly this.
d=$(new ok-ascii-16384)
i=0
: > "$d/boundary.txt"
while [ "$i" -lt 256 ]; do
  { head -c 63 /dev/zero | tr '\000' 'A'; printf '\n'; } >> "$d/boundary.txt"
  i=$((i + 1))
done
commit "$d" boundary
size=$(wc -c < "$d/boundary.txt" | tr -d ' ')
run_rights "$d"
ok=0
if [ "$size" -eq 16384 ] && [ "$rc" -eq 0 ] && has 'RECORDS 1'; then ok=1; fi
verdict ok-ascii-16384 "$ok"

[ "$failed" -eq 0 ] || exit 1
exit 0
