#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# equiv.sh - transcript equivalence of two versions of scripts/check (D-58 prerequisite refactor, T-02-01).
# usage: sh tests/check/equiv.sh BASE_DIR
#   EQUIV_ONLY=PREFIX in the environment keeps only the clean scenario and the scenarios whose name starts with PREFIX, for a
#   quick check while editing one control family. It skips the label coverage test and ends in PARTIAL PASS|FAIL, never PASS.
#   BASE_DIR  the pre-refactor tree, for example:  git archive ff126b5 | tar -x -C "$base"
#   The working tree (tracked plus untracked-not-ignored files) is the refactored side. Each scenario applies the same
#   path-based mutation to a scratch copy of both sides, runs  sh scripts/check  in each, and compares stdout, stderr and
#   exit status. The scratch copies carry a shared-object clone of this repository's .git so that the protocol-order stage
#   sees real history on both sides; the real repository is only read.
#   A scenario is vacuous when its mutation did not make the BASE gate print the label it was built to exercise. That is
#   reported as VACUOUS and fails the run, so no scenario can pass by exercising nothing.
# stdout: EQUIV <scenario> SAME|DIFF|VACUOUS , EQUIV LABELS covered|MISSING <label> , EQUIV RESULT PASS|FAIL
# exit  : 0 every scenario SAME and every label covered | 1 a DIFF, a VACUOUS scenario or a missing label | 2 harness error
set -eu
LC_ALL=C; export LC_ALL

usage() {
  echo "usage: sh tests/check/equiv.sh BASE_DIR" >&2
  exit 2
}
[ $# -eq 1 ] || usage
base_dir=$1
[ -f "$base_dir/scripts/check" ] || { echo "HARNESS_NO_FILE $base_dir/scripts/check" >&2; exit 2; }
base_dir=$(cd "$base_dir" && pwd)
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

die() {
  echo "HARNESS_ERROR $*" >&2
  exit 2
}

# ---- pristine copies of both sides ----

# the history clone: objects are shared with the real repository, nothing is written there
git clone -q -n --shared "$root" "$tmp/gitonly" || die "cannot clone $root"

pb="$tmp/pristine-base"
pn="$tmp/pristine-new"
mkdir -p "$pb" "$pn" || die "cannot create scratch directories"
cp -R "$base_dir/." "$pb/" || die "cannot copy the base tree"
rm -rf "$pb/.git"
cp -R "$tmp/gitonly/.git" "$pb/.git" || die "cannot add history to the base copy"
cp -R "$tmp/gitonly/.git" "$pn/.git" || die "cannot add history to the new copy"

git -C "$root" ls-files --cached --others --exclude-standard > "$tmp/files.lst" || die "cannot list the working tree"
while IFS= read -r f; do
  [ -f "$root/$f" ] || continue
  mkdir -p "$pn/$(dirname "$f")" || die "mkdir for $f"
  cp -p "$root/$f" "$pn/$f" || die "cannot copy $f"
done < "$tmp/files.lst"
[ -f "$pn/scripts/check" ] || die "working tree has no scripts/check"

# ---- mutations: one && chain each (a failing step must fail the chain, so set -e cannot be relied on) ----
# Every mutation runs inside a scratch copy root. A target that does not exist fails the chain: no silent no-op.

mut_clean() { :; }

mut_ledger_wrong_expect() { test -f ledger/negative/dup-key/EXPECT && printf 'WRONG_CODE\n' > ledger/negative/dup-key/EXPECT; }
mut_ledger_no_expect() { rm ledger/negative/e0-only/EXPECT; }
mut_ledger_empty_expect() { test -f ledger/negative/e0-only/EXPECT && : > ledger/negative/e0-only/EXPECT; }
mut_ledger_no_required() { rm ledger/negative/required-origin/REQUIRED; }
mut_ledger_no_cases() { rm -rf ledger/negative/*/ && test -d ledger/negative; }

mut_decision_no_expect() { rm tests/decision/bad-date/EXPECT; }
mut_decision_empty_expect() { test -f tests/decision/bad-date/EXPECT && : > tests/decision/bad-date/EXPECT; }
mut_decision_wrong_expect() { test -f tests/decision/bad-date/EXPECT && printf 'WRONG_CODE\n' > tests/decision/bad-date/EXPECT; }
mut_decision_ok_fails() { rm tests/decision/ok-protocol-excluded/0009-control.md; }

mut_profile_no_record() { rm tests/profile/bad-status/pce-hucard-2btn/r1.toml; }
mut_profile_no_expect() { rm tests/profile/bad-status/EXPECT; }
mut_profile_empty_expect() { test -f tests/profile/bad-status/EXPECT && : > tests/profile/bad-status/EXPECT; }
mut_profile_wrong_expect() { test -f tests/profile/bad-status/EXPECT && printf 'WRONG_CODE\n' > tests/profile/bad-status/EXPECT; }
mut_profile_ok_fails() { cp tests/profile/bad-status/pce-hucard-2btn/r1.toml tests/profile/ok-boundary-16384/pce-hucard-2btn/r1.toml; }
mut_profile_no_cases() { rm -rf tests/profile/*/ && test -d tests/profile; }

mut_provenance_no_expect() { rm tests/provenance/bad-id/EXPECT; }
mut_provenance_empty_expect() { test -f tests/provenance/bad-id/EXPECT && : > tests/provenance/bad-id/EXPECT; }
mut_provenance_wrong_expect() { test -f tests/provenance/bad-id/EXPECT && printf 'WRONG_CODE\n' > tests/provenance/bad-id/EXPECT; }
mut_provenance_ok_fails() { rm tests/provenance/ok-one-entry/src/copied.c; }

mut_licence_no_cases() { rm -rf tests/licence/*/ && test -d tests/licence; }
mut_licence_no_expect() { rm tests/licence/mit-no-holder/EXPECT; }
mut_licence_empty_expect() { test -f tests/licence/mit-no-holder/EXPECT && : > tests/licence/mit-no-holder/EXPECT; }
mut_licence_wrong_expect() { test -f tests/licence/mit-no-holder/EXPECT && printf 'WRONG_CODE\n' > tests/licence/mit-no-holder/EXPECT; }
mut_licence_ok_fails() { rm tests/licence/ok-minimal/LICENSE-MIT; }

mut_results_no_protocol() { rm tests/results/empty-value/protocol.md; }
mut_results_no_record() { rm tests/results/empty-value/record.md; }
mut_results_no_expect() { rm tests/results/empty-value/EXPECT; }
mut_results_empty_expect() { test -f tests/results/empty-value/EXPECT && : > tests/results/empty-value/EXPECT; }
mut_results_wrong_expect() { test -f tests/results/empty-value/EXPECT && printf 'WRONG_CODE\n' > tests/results/empty-value/EXPECT; }
mut_results_ok_fails() { cp tests/results/empty-value/record.md tests/results/ok-complete/record.md; }
mut_results_unreadable() { chmod 000 tests/results/empty-value/protocol.md; }
mut_results_only_ok() { rm -rf tests/results/empty-value tests/results/no-register tests/results/no-scores tests/results/row-deleted tests/results/unknown-row tests/results/no-record; }

# scenario table: NAME LABEL (the label the BASE transcript must contain; "-" means none required)
cat > "$tmp/scenarios.lst" <<'EOF'
clean -
ledger-wrong-expect CONTROL BROKEN
ledger-no-expect HARNESS_NO_EXPECT
ledger-empty-expect HARNESS_EMPTY_EXPECT
ledger-no-required CONTROL BROKEN
ledger-no-cases NO_CONTROLS
decision-no-expect HARNESS_NO_EXPECT
decision-empty-expect HARNESS_EMPTY_EXPECT
decision-wrong-expect CONTROL BROKEN
decision-ok-fails SAMPLE REJECTED
profile-no-record HARNESS_NO_RECORD
profile-no-expect HARNESS_NO_EXPECT
profile-empty-expect HARNESS_EMPTY_EXPECT
profile-wrong-expect CONTROL BROKEN
profile-ok-fails SAMPLE REJECTED
profile-no-cases NO_CONTROLS
provenance-no-expect HARNESS_NO_EXPECT
provenance-empty-expect HARNESS_EMPTY_EXPECT
provenance-wrong-expect CONTROL BROKEN
provenance-ok-fails SAMPLE REJECTED
licence-no-cases NO_CONTROLS
licence-no-expect HARNESS_NO_EXPECT
licence-empty-expect HARNESS_EMPTY_EXPECT
licence-wrong-expect CONTROL BROKEN
licence-ok-fails SAMPLE REJECTED
results-no-protocol HARNESS_NO_FILE
results-no-record HARNESS_NO_FILE
results-no-expect HARNESS_NO_EXPECT
results-empty-expect HARNESS_EMPTY_EXPECT
results-wrong-expect CONTROL BROKEN
results-ok-fails SAMPLE REJECTED
results-only-ok NO_CONTROLS
results-unreadable HARNESS_ERROR
EOF


# ---- build every scenario, then run both sides of each (batched in parallel: the gate is fork-bound) ----

out="$tmp/out"
mkdir -p "$out" "$tmp/s"
batch=8
count=0
: > "$tmp/names.lst"
while IFS= read -r line; do
  name=${line%% *}
  want=${line#* }
  if [ -n "${EQUIV_ONLY-}" ]; then
    case $name in clean | "$EQUIV_ONLY"*) ;; *) continue ;; esac
  fi
  fn="mut_$(printf '%s' "$name" | tr '-' '_')"
  s="$tmp/s/$name"
  mkdir -p "$s"
  cp -R "$pb" "$s/base" || die "copy base for $name"
  cp -R "$pn" "$s/new" || die "copy new for $name"
  for side in base new; do
    if ( cd "$s/$side" && "$fn" ) > /dev/null 2>&1; then :; else die "mutation $fn failed on the $side copy"; fi
  done
  printf '%s\t%s\n' "$name" "$want" >> "$tmp/names.lst"
  for side in base new; do
    (
      cd "$s/$side"
      if sh scripts/check > "$out/$name.$side.raw" 2> "$out/$name.$side.err.raw"; then echo 0 > "$out/$name.$side.st"; else echo $? > "$out/$name.$side.st"; fi
    ) &
  done
  count=$((count + 1))
  if [ $((count % batch)) -eq 0 ]; then wait; fi
done < "$tmp/scenarios.lst"
wait

# ---- compare ----

failed=0
tab=$(printf '\t')
while IFS="$tab" read -r name want; do
  s="$tmp/s/$name"
  for side in base new; do
    for kind in "" ".err"; do
      sed "s|$s/$side|<ROOT>|g" "$out/$name.$side$kind.raw" > "$out/$name.$side$kind.txt"
    done
    printf 'exit %s\n' "$(cat "$out/$name.$side.st")" >> "$out/$name.$side.txt"
  done
  verdict=SAME
  if ! cmp -s "$out/$name.base.txt" "$out/$name.new.txt" || ! cmp -s "$out/$name.base.err.txt" "$out/$name.new.err.txt"; then
    verdict=DIFF
  elif [ "$want" != "-" ] && ! grep -Fq "$want" "$out/$name.base.txt"; then
    verdict=VACUOUS
  elif [ "$name" != clean ] && cmp -s "$out/$name.base.txt" "$out/clean.base.txt"; then
    verdict=VACUOUS
  fi
  echo "EQUIV $name $verdict"
  if [ "$verdict" != SAME ]; then
    failed=1
    if [ "$verdict" = DIFF ]; then
      diff "$out/$name.base.txt" "$out/$name.new.txt" | sed -n '1,20p' | sed 's/^/    /'
      diff "$out/$name.base.err.txt" "$out/$name.new.err.txt" | sed -n '1,10p' | sed 's/^/    err /'
    else
      echo "    the base transcript does not contain '$want' (or equals the clean one): the mutation exercised nothing"
    fi
  fi
done < "$tmp/names.lst"

# the clean tree must itself be green on the base, or the comparison proves little
if [ "$(sed -n '$p' "$out/clean.base.txt")" != "exit 0" ] || ! grep -qx 'CHECK RESULT PASS' "$out/clean.base.txt"; then
  echo "EQUIV clean base gate is not green (exit $(cat "$out/clean.base.st"))"
  failed=1
fi

# label coverage over the BASE transcripts
labels_ok=1
if [ -n "${EQUIV_ONLY-}" ]; then
  echo "EQUIV LABELS skipped (EQUIV_ONLY=$EQUIV_ONLY)"
  suffix=" PARTIAL"
else
  suffix=""
  for lab in 'CONTROL BROKEN' 'SAMPLE REJECTED' HARNESS_NO_EXPECT HARNESS_EMPTY_EXPECT NO_CONTROLS HARNESS_NO_RECORD HARNESS_NO_FILE HARNESS_ERROR; do
    if ! cat "$out"/*.base.txt | grep -Fq "$lab"; then
      echo "EQUIV LABELS MISSING $lab"
      labels_ok=0
      failed=1
    fi
  done
  if [ "$labels_ok" -eq 1 ]; then echo "EQUIV LABELS covered"; fi
fi

if [ "$failed" -eq 0 ]; then
  echo "EQUIV RESULT${suffix} PASS"
  exit 0
fi
echo "EQUIV RESULT${suffix} FAIL"
exit 1
