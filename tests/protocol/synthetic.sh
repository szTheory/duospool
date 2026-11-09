#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# synthetic.sh - negative controls for scripts/protocol-order-check (T-01-13, T-01-15).
# Each case builds a throwaway git repository under mktemp -d (removed by a trap) and runs the checker on it with --repo.
# The real repository and the global git configuration are never read or written: git runs with an explicit identity,
# signing off, and the system and global config files disabled.
# stdout: one line per case  SYNTH <case> PASS|FAIL
# exit  : 0 every case passed | 1 a case failed | 2 harness error
set -eu
LC_ALL=C; export LC_ALL
here=$(cd "$(dirname "$0")" && pwd)
checker="$here/../../scripts/protocol-order-check"
[ -f "$checker" ] || { echo "HARNESS_NO_FILE $checker" >&2; exit 2; }

GIT_CONFIG_NOSYSTEM=1; export GIT_CONFIG_NOSYSTEM
GIT_CONFIG_GLOBAL=/dev/null; export GIT_CONFIG_GLOBAL

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
failed=0

proto=docs/decisions/0001-language-build.protocol.md
record=docs/decisions/0001-language-build.md

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

# put REPO PATH CONTENT: write a file (one trailing newline)
put() {
  mkdir -p "$(dirname "$1/$2")"
  printf '%s\n' "$3" > "$1/$2"
}

# commit REPO MESSAGE: stage everything in the throwaway repository and commit
commit() {
  g "$1" add -A
  g "$1" commit -q -m "$2"
}

body='# Protocol
the fixed rubric
'

# add_scorer REPO / add_protocol REPO / add_spike REPO: the commits whose order is under test
add_scorer() { put "$1" scripts/lib/score.awk 'BEGIN { exit 0 }'; commit "$1" scorer; }
add_protocol() { put "$1" "$proto" "$body"; commit "$1" protocol; }
add_spike() { put "$1" spikes/tool-01/skeleton.txt "skeleton"; commit "$1" spike; }

# expect_codes CASE REPO CODES: the checker must meet --expect CODES, and must exit 1 without it
expect_codes() {
  if sh "$checker" --repo "$2" --expect "$3" > "$work/out" 2>&1; then r=0; else r=$?; fi
  if [ "$r" -ne 0 ] || ! grep -qx 'EXPECT_OK' "$work/out"; then
    echo "SYNTH $1 FAIL"
    sed 's/^/  /' "$work/out"
    failed=1
    return
  fi
  if sh "$checker" --repo "$2" > "$work/out" 2>&1; then r=0; else r=$?; fi
  if [ "$r" -ne 1 ]; then
    echo "SYNTH $1 FAIL"
    echo "  without --expect the checker exited $r, wanted 1"
    failed=1
    return
  fi
  echo "SYNTH $1 PASS"
}

# expect_pass CASE REPO [PATTERN]: the checker must exit 0 with no violations, and the info line must match PATTERN
expect_pass() {
  if sh "$checker" --repo "$2" > "$work/out" 2>&1; then r=0; else r=$?; fi
  if [ "$r" -ne 0 ] || ! grep -q '^RECORDS ' "$work/out" || { [ $# -ge 3 ] && ! grep -Eq "$3" "$work/out"; }; then
    echo "SYNTH $1 FAIL"
    sed 's/^/  /' "$work/out"
    failed=1
    return
  fi
  echo "SYNTH $1 PASS"
}

# ok-order: protocol commit, then a spike commit
d=$(new ok-order)
add_scorer "$d"
add_protocol "$d"
add_spike "$d"
expect_pass ok-order "$d" '^PROTOCOL add=[0-9a-f]{40} first_spike=[0-9a-f]{40} amendments=0 amended_after_spike=no$'

# spike-first: a spike commit, then the protocol
d=$(new spike-first)
add_scorer "$d"
add_spike "$d"
add_protocol "$d"
expect_codes spike-first "$d" PROTOCOL_AFTER_SPIKE

# same-commit: protocol and spike file in one commit is not strictly before
d=$(new same-commit)
add_scorer "$d"
put "$d" "$proto" "$body"
put "$d" spikes/tool-01/skeleton.txt skeleton
commit "$d" both
expect_codes same-commit "$d" PROTOCOL_AFTER_SPIKE

# edited-in-place: a committed edit of pre-amendment text
d=$(new edited-in-place)
add_scorer "$d"
add_protocol "$d"
add_spike "$d"
put "$d" "$proto" '# Protocol
the changed rubric
'
commit "$d" edit
expect_codes edited-in-place "$d" PROTOCOL_EDITED

# edited-under-amendment: an in-place edit hidden behind a valid appended amendment is still an edit
d=$(new edited-under-amendment)
add_scorer "$d"
add_protocol "$d"
add_spike "$d"
printf '# Protocol\nthe quietly changed rubric\n\n## Amendment 2026-11-10: tidy wording\ntext\n' > "$d/$proto"
commit "$d" edit-and-amend
expect_codes edited-under-amendment "$d" PROTOCOL_EDITED

# ok-amended: an appended dated amendment passes and is counted; it came after the spike
d=$(new ok-amended)
add_scorer "$d"
add_protocol "$d"
add_spike "$d"
{ cat "$d/$proto"; printf '\n## Amendment 2026-11-10: clarify a metric\nthe clarification\n'; } > "$d/amended.tmp"
mv "$d/amended.tmp" "$d/$proto"
commit "$d" amend
expect_pass ok-amended "$d" ' amendments=1 amended_after_spike=yes$'

# undated-amendment: an amendment heading without a date
d=$(new undated-amendment)
add_scorer "$d"
add_protocol "$d"
add_spike "$d"
{ cat "$d/$proto"; printf '\n## Amendment: no date given\ntext\n'; } > "$d/amended.tmp"
mv "$d/amended.tmp" "$d/$proto"
commit "$d" amend
expect_codes undated-amendment "$d" PROTOCOL_AMENDMENT_UNDATED

# no-protocol: spike files and no protocol at all
d=$(new no-protocol)
add_scorer "$d"
add_spike "$d"
expect_codes no-protocol "$d" PROTOCOL_MISSING

# cite-mismatch: record 0001 cites a commit that is not the protocol's add commit
d=$(new cite-mismatch)
add_scorer "$d"
add_protocol "$d"
add_spike "$d"
put "$d" "$record" 'Protocol commit: 0000000000000000000000000000000000000000'
commit "$d" record
expect_codes cite-mismatch "$d" PROTOCOL_CITE_MISMATCH

# ok-cite: record 0001 cites the protocol's add commit
d=$(new ok-cite)
add_scorer "$d"
add_protocol "$d"
sha=$(g "$d" rev-parse HEAD)
add_spike "$d"
put "$d" "$record" "Protocol commit: $sha"
commit "$d" record
expect_pass ok-cite "$d" "^PROTOCOL add=$sha "

# scorer-after-spike: the scorer is committed after a spike file
d=$(new scorer-after-spike)
add_protocol "$d"
add_spike "$d"
add_scorer "$d"
expect_codes scorer-after-spike "$d" SCORER_AFTER_SPIKE

# scorer-same-commit: scorer and spike file in one commit is not strictly before
d=$(new scorer-same-commit)
add_protocol "$d"
put "$d" scripts/lib/score.awk 'BEGIN { exit 0 }'
put "$d" spikes/tool-01/skeleton.txt skeleton
commit "$d" both
expect_codes scorer-same-commit "$d" SCORER_AFTER_SPIKE

# scorer-missing: a spike file exists and the scorer was never committed
d=$(new scorer-missing)
add_protocol "$d"
add_spike "$d"
expect_codes scorer-missing "$d" SCORER_AFTER_SPIKE

if [ "$failed" -ne 0 ]; then
  exit 1
fi
exit 0
