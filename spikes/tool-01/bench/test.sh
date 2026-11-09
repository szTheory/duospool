#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# test.sh - bench wrapper (TOOL-01 spike, Pattern 3): sources <opt>/hooks.sh and calls opt_test with its argument forwarded
# unchanged. No measurement logic lives here. BENCH_DRYRUN=1 validates the arguments and prints the one line it would act on.
# usage: test.sh <c|rust> <debug|release>
set -eu
LC_ALL=C; export LC_ALL
usage() {
  echo "usage: test.sh <c|rust> <debug|release>" >&2
  exit 2
}
[ $# -eq 2 ] || usage
opt=$1
arg=$2
case $opt in c | rust) ;; *) usage ;; esac
case $arg in debug | release) ;; *) usage ;; esac
if [ "${BENCH_DRYRUN:-}" = 1 ]; then
  echo "HOOK $opt opt_test $arg"
  exit 0
fi
SPIKE_DIR=$(cd "$(dirname "$0")/.." && pwd)
OPT_DIR=$SPIKE_DIR/$opt
export SPIKE_DIR OPT_DIR
# shellcheck disable=SC1090,SC1091
. "$OPT_DIR/hooks.sh"
opt_test "$arg"
