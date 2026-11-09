#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# gates.sh - shared hard-gate harness for both TOOL-01 options (protocol section (3), G1 to G5).
# usage: gates.sh <c|rust> [--no-fuzz] [--profile debug|release]
#   Sources <opt>/hooks.sh, which defines opt_build, opt_runner, opt_install and opt_fuzz60 (the hook contract).
#   --profile selects the profile passed to opt_build (build stage) and opt_runner (test stages); the install and
#   consumer-proof procedure is the same in both profiles. It defaults to debug.
#   --no-fuzz skips the 60-second fuzz stage, and a fully green run then ends PASS-NO-FUZZ, never PASS.
#   Every stage runs even when an earlier stage failed, so one run reports every stage.
# stdout: FIXTURE <name> OK|MISMATCH lines, GATE <stage> PASS|FAIL|SKIPPED (a FAIL is followed by an indented reason),
#         LEAK <n> and SEEDED-LEAK <n> (M15), then  GATES <opt> RESULT PASS|PASS-NO-FUZZ|FAIL
# exit  : 0 PASS or PASS-NO-FUZZ | 1 FAIL | 2 usage error
# Stage functions return their status explicitly: errexit does not apply inside the "if" that runs them.
set -eu
LC_ALL=C; export LC_ALL
SPIKE_DIR=$(cd "$(dirname "$0")" && pwd)
OUT=$SPIKE_DIR/out
manifest=$SPIKE_DIR/fixtures.manifest

usage() {
  echo "usage: gates.sh <c|rust> [--no-fuzz] [--profile debug|release]" >&2
  exit 2
}

[ $# -ge 1 ] || usage
opt=$1
shift
fuzz=yes
profile=debug
while [ $# -gt 0 ]; do
  case $1 in
    --no-fuzz) fuzz=no; shift ;;
    --profile) [ $# -ge 2 ] || usage; profile=$2; shift 2 ;;
    *) usage ;;
  esac
done
case $opt in
  c | rust) ;;
  *) usage ;;
esac
case $profile in
  debug | release) ;;
  *) usage ;;
esac
OPT_DIR=$SPIKE_DIR/$opt
if [ ! -f "$OPT_DIR/hooks.sh" ]; then
  echo "gates: no $OPT_DIR/hooks.sh" >&2
  exit 2
fi
# shellcheck disable=SC1091
. "$OPT_DIR/hooks.sh"

overall=PASS
reason=""
work=""
build_ok=no
runner_ok=no
runner_rc=1
runner_why="runner not run"
transcript=$OUT/runner.$opt.txt

cleanup() {
  if [ -n "$work" ] && [ -d "$work" ]; then
    chmod -R u+rwx "$work" 2> /dev/null || true
    rm -rf "$work"
  fi
  work=""
}
trap cleanup EXIT
trap 'exit 130' INT TERM

# run_stage <name> <function>: print GATE <name> PASS|FAIL and remember a failure.
run_stage() {
  reason=""
  if "$2"; then
    echo "GATE $1 PASS"
  else
    echo "GATE $1 FAIL"
    echo "  reason: ${reason:-see output above}"
    overall=FAIL
  fi
}

stage_fixtures() {
  sh "$SPIKE_DIR/gen-fixtures.sh" || { reason="gen-fixtures.sh reported a mismatch or error"; return 1; }
}

stage_build() {
  if opt_build "$profile"; then
    build_ok=yes
  else
    reason="opt_build $profile failed"
    return 1
  fi
}

# Run the manifest runner once; the test-* and transcript stages read its output.
run_runner() {
  [ "$build_ok" = yes ] || { runner_why="the build stage failed, so there is no runner to run"; return 0; }
  if ! type opt_runner > /dev/null 2>&1; then
    runner_why="opt_runner is not defined by $opt/hooks.sh"
    return 0
  fi
  _rbin=$(opt_runner "$profile") || { runner_why="opt_runner $profile failed"; return 0; }
  if [ ! -x "$_rbin" ]; then
    runner_why="runner binary missing: $_rbin"
    return 0
  fi
  if perl -e 'alarm 120; exec @ARGV' "$_rbin" "$manifest" "$OUT" > "$transcript" 2> "$OUT/runner.$opt.err"; then
    runner_rc=0
  else
    runner_rc=$?
  fi
  runner_ok=yes
  runner_why=""
}

# case_check <norm|ctl>: every manifest case of that kind has a transcript line ending PASS (norm) or
# CONTROL-REJECTED (ctl). Prints the number of cases checked on success.
case_check() {
  awk -v mode="$1" '
    FNR == NR {
      if ($1 == "case") { n++; ids[n] = $2; ctl[$2] = ($6 == "control") }
      next
    }
    { seen[$1] = $NF }
    END {
      bad = 0; cnt = 0
      for (i = 1; i <= n; i++) {
        id = ids[i]
        if (mode == "ctl" && !ctl[id]) continue
        if (mode == "norm" && ctl[id]) continue
        cnt++
        want = ctl[id] ? "CONTROL-REJECTED" : "PASS"
        if (!(id in seen) || seen[id] != want) { print "  case " id ": want " want ", transcript has " ((id in seen) ? seen[id] : "nothing"); bad = 1 }
      }
      if (cnt == 0) { print "  no manifest cases of kind " mode; bad = 1 }
      exit bad
    }' "$manifest" "$transcript"
}

stage_test_refusals() {
  [ "$runner_ok" = yes ] || { reason=$runner_why; return 1; }
  [ "$runner_rc" -eq 0 ] || { reason="runner exited $runner_rc"; return 1; }
  case_check norm || { reason="a non-control case did not PASS"; return 1; }
}

stage_test_controls() {
  [ "$runner_ok" = yes ] || { reason=$runner_why; return 1; }
  case_check ctl || { reason="a control was not reported CONTROL-REJECTED"; return 1; }
  _nc=$(grep -c ' control$' "$manifest" || true)
  grep -qx "RUNNER pass=[0-9]* fail=0 controls_rejected=$_nc" "$transcript" || { reason="the RUNNER line does not report fail=0 controls_rejected=$_nc"; return 1; }
}

stage_test_interleave() {
  [ "$runner_ok" = yes ] || { reason=$runner_why; return 1; }
  grep -qx 'interleave EQUAL PASS' "$transcript" || { reason="the transcript has no 'interleave EQUAL PASS' line"; return 1; }
}

stage_transcript() {
  [ "$runner_ok" = yes ] || { reason="no transcript could be written: $runner_why"; return 1; }
  cp "$transcript" "$OUT/transcript.$opt.txt" || { reason="cannot write transcript.$opt.txt"; return 1; }
}

stage_transcript_identical() {
  cmp "$OUT/transcript.c.txt" "$OUT/transcript.rust.txt" || { reason="transcript.c.txt and transcript.rust.txt differ"; return 1; }
}

# leak_count <file> <needle>...: total of grep -c -a over the needles (fixed strings, empty needles skipped).
leak_count() {
  _lf=$1
  shift
  _lt=0
  for _ln in "$@"; do
    [ -n "$_ln" ] || continue
    _lc=$(grep -c -a -F -- "$_ln" "$_lf" 2> /dev/null || true)
    _lt=$((_lt + _lc))
  done
  echo "$_lt"
}

# build_consumer <prefix> <out-binary>: the G4 consumer build, against the installed prefix only.
build_consumer() {
  (
    cd "$cons" || exit 1
    unset PKG_CONFIG_PATH
    PKG_CONFIG_LIBDIR=$1/lib/pkgconfig
    export PKG_CONFIG_LIBDIR
    cc $(pkgconf --static --cflags --libs duospool) main.c -o "$2"
  )
}

stage_consumer_proof() {
  work=$(mktemp -d) || { reason="mktemp failed"; return 1; }
  workp=$(cd "$work" && pwd -P)
  src=$work/src
  bd=$work/build
  bdl=$work/build-leak
  pfx=$work/prefix
  pfxl=$work/prefix-leak
  cons=$work/consumer
  cp -R "$OPT_DIR" "$src" || { reason="cannot copy the source tree"; return 1; }
  opt_install "$src" "$bd" "$pfx" yes || { reason="opt_install (remap yes) failed"; cleanup; return 1; }
  opt_install "$src" "$bdl" "$pfxl" no || { reason="opt_install (remap no) failed"; cleanup; return 1; }
  for _f in lib/libduospool.a include/duospool.h lib/pkgconfig/duospool.pc; do
    [ -f "$pfx/$_f" ] || { reason="the installed prefix has no $_f"; cleanup; return 1; }
  done
  srcp=$(cd "$src" && pwd -P)
  bdp=$(cd "$bd" && pwd -P)
  bdlp=$(cd "$bdl" && pwd -P)
  # Make the source copy and both build directories unreadable (chmod a-rwx on every entry, children before their
  # parent so the walk does not fail on the directory it just closed). The git worktree is never touched.
  find "$src" "$bd" "$bdl" -depth -exec chmod a-rwx {} + || { reason="chmod of the source copy and build directories failed"; cleanup; return 1; }
  if ls "$src" > /dev/null 2>&1 || cat "$src/src/core.c" > /dev/null 2>&1; then
    reason="the source copy is still readable after chmod -R a-rwx (running as root?)"
    cleanup
    return 1
  fi
  mkdir "$cons" && cp "$SPIKE_DIR/consumer/main.c" "$cons/main.c" || { reason="cannot set up the consumer directory"; cleanup; return 1; }
  build_consumer "$pfx" consumer-bin > "$cons/build.log" 2>&1 || { reason="the consumer did not build against the installed prefix: $(tail -n 3 "$cons/build.log" | tr '\n' ' ')"; cleanup; return 1; }
  "$cons/consumer-bin" "$OUT/lcg-64k.bin" > "$cons/got.txt" || { reason="the consumer exited non-zero"; cleanup; return 1; }
  cmp -s "$cons/got.txt" "$SPIKE_DIR/consumer/golden.txt" || { reason="consumer output '$(cat "$cons/got.txt")' differs from golden.txt"; cleanup; return 1; }
  # (a) header-removed control: without the installed header the consumer build must fail, and because of the header.
  mv "$pfx/include/duospool.h" "$pfx/include/duospool.h.aside" || { reason="cannot move the header aside"; cleanup; return 1; }
  if build_consumer "$pfx" consumer-nohdr > "$cons/nohdr.log" 2>&1; then
    mv "$pfx/include/duospool.h.aside" "$pfx/include/duospool.h"
    reason="control broken: the consumer built without the installed header"
    cleanup
    return 1
  fi
  mv "$pfx/include/duospool.h.aside" "$pfx/include/duospool.h" || { reason="cannot restore the header"; cleanup; return 1; }
  grep -q 'duospool.h' "$cons/nohdr.log" || { reason="the header-removed build failed for another reason: $(tail -n 3 "$cons/nohdr.log" | tr '\n' ' ')"; cleanup; return 1; }
  # (b) path leaks in the installed artifacts (M15): the work dir, both spellings, the build dirs and $HOME.
  _leak=0
  for _f in "$pfx/lib/libduospool.a" "$pfx/lib/pkgconfig/duospool.pc"; do
    _c=$(leak_count "$_f" "$src" "$srcp" "$bd" "$bdp" "$work" "$workp" "$HOME")
    _leak=$((_leak + _c))
  done
  echo "LEAK $_leak"
  # (c) seeded-leak control: the same grep on a build without path remapping must find the leak, or the check is vacuous.
  _seed=0
  for _f in "$pfxl/lib/libduospool.a" "$pfxl/lib/pkgconfig/duospool.pc"; do
    _c=$(leak_count "$_f" "$src" "$srcp" "$bdl" "$bdlp" "$work" "$workp" "$HOME")
    _seed=$((_seed + _c))
  done
  echo "SEEDED-LEAK $_seed"
  cleanup
  [ "$_leak" -eq 0 ] || { reason="the installed archive or .pc embeds $_leak path occurrence(s)"; return 1; }
  [ "$_seed" -ge 1 ] || { reason="the seeded-leak control found nothing: the leak check is vacuous"; return 1; }
}

stage_fuzz60() {
  if ! type opt_fuzz60 > /dev/null 2>&1; then
    reason="opt_fuzz60 is not defined by $opt/hooks.sh"
    return 1
  fi
  # Liveness first: the planted-bug build must be found within 60 s, or a clean run proves nothing (T-01-30).
  # opt_fuzz60 returns 0 when the outcome is the wanted one (planted: crash found; clean: no crash).
  opt_fuzz60 60 yes > "$OUT/fuzz60-planted.$opt.txt" 2>&1 || { reason="liveness control broken: the planted-bug build was not found: $(tail -n 2 "$OUT/fuzz60-planted.$opt.txt" | tr '\n' ' ')"; return 1; }
  _lv=$(sed -n 's/^FUZZ elapsed=\([0-9.]*\) crashed=yes .*/\1/p' "$OUT/fuzz60-planted.$opt.txt" | head -n 1)
  [ -n "$_lv" ] || { reason="the planted-bug run printed no crashed=yes line"; return 1; }
  echo "LIVENESS found_after=$_lv"
  awk -v t="$_lv" 'BEGIN { exit !(t + 0 <= 60) }' || { reason="the planted bug was found after $_lv s, more than 60 s"; return 1; }
  opt_fuzz60 60 no > "$OUT/fuzz60.$opt.txt" 2>&1 || { reason="the 60 s clean fuzz run crashed or failed: $(tail -n 2 "$OUT/fuzz60.$opt.txt" | tr '\n' ' ')"; return 1; }
  grep -q '^FUZZ elapsed=.* crashed=no ' "$OUT/fuzz60.$opt.txt" || { reason="no FUZZ crashed=no line from the clean run"; return 1; }
  grep '^FUZZ ' "$OUT/fuzz60.$opt.txt"
}

mkdir -p "$OUT"
run_stage fixtures stage_fixtures
run_stage build stage_build
run_runner
run_stage test-refusals stage_test_refusals
run_stage test-controls stage_test_controls
run_stage test-interleave stage_test_interleave
run_stage transcript stage_transcript
if [ -f "$OUT/transcript.c.txt" ] && [ -f "$OUT/transcript.rust.txt" ]; then
  run_stage transcript-identical stage_transcript_identical
fi
run_stage consumer-proof stage_consumer_proof
if [ "$fuzz" = no ]; then
  echo "GATE fuzz60 SKIPPED"
else
  run_stage fuzz60 stage_fuzz60
fi

if [ "$overall" = PASS ] && [ "$fuzz" = no ]; then
  result=PASS-NO-FUZZ
else
  result=$overall
fi
echo "GATES $opt RESULT $result"
[ "$result" != FAIL ]
