# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# gitfix.sh - throwaway git repository helpers shared by tests/audit/synthetic.sh and tests/rights/synthetic.sh.
# This file is sourced (. file), never run. The caller sets, before sourcing:
#   work     a scratch directory (mktemp -d, removed by the caller's trap)
#   failed   0, the failure flag that verdict sets to 1
#   ctl      the result-line prefix, for example AUDITTEST or RIGHTSTEST
# and must set `out` and `rc` itself (the captured output and status of the run under test) before calling verdict or has.
# The global git configuration is never read or written: the real repository is never touched by a case.
GIT_CONFIG_NOSYSTEM=1; export GIT_CONFIG_NOSYSTEM
GIT_CONFIG_GLOBAL=/dev/null; export GIT_CONFIG_GLOBAL

# g REPO GIT-ARGS...
g() {
  r=$1
  shift
  git -c user.name=duospool-test -c user.email=test@example.invalid -c commit.gpgsign=false -C "$r" "$@"
}

# new CASE: create the repository directory under $work and print its path
new() {
  d="$work/$1"
  mkdir -p "$d"
  g "$d" init -q
  printf '%s\n' "$d"
}

# commit REPO MESSAGE: stage everything (forced, so an ignored name is staged too) and commit
commit() {
  g "$1" add -A -f
  g "$1" commit -q -m "$2"
}

# verdict CASE OK-FLAG: print the result line, count failures
verdict() {
  if [ "$2" = 1 ]; then
    echo "$ctl $1 PASS"
  else
    echo "$ctl $1 FAIL"
    printf '%s\n' "$out" | sed 's/^/  | /'
    echo "  | exit status $rc"
    failed=1
  fi
}

# has TEXT: is TEXT a fixed substring of $out
has() {
  printf '%s\n' "$out" | grep -qF -- "$1"
}
