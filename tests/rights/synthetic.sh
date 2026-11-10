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

# ---- planted-pce: an 8192-byte zero-filled x.pce, staged with git add -f ----
d=$(new planted-pce)
dd if=/dev/zero of="$d/x.pce" bs=8192 count=1 2>/dev/null
commit "$d" rom
negative "$d" ROMLIKE_BINARY
ok=$neg_ok
has ROMLIKE_BINARY || ok=0
verdict planted-pce "$ok"

# ---- clean-prose: a repository holding prose only ----
d=$(new clean-prose)
printf 'plain text\nsecond line\n' > "$d/a.txt"
commit "$d" prose
run_rights "$d"
ok=0
if [ "$rc" -eq 0 ] && has 'RECORDS 1'; then ok=1; fi
verdict clean-prose "$ok"

[ "$failed" -eq 0 ] || exit 1
exit 0
