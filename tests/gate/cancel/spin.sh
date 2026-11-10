#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# spin.sh - stand-in for the conformance runner in the cancel controls (D-77). It records its pid in the file named by $1
# (so the control can clean up whatever the supervisor leaves), prints one finished case on stdout, then spins: exec replaces
# this shell with sleep, so the pid in the file is the pid of the process that the supervisor must kill.
# usage: sh spin.sh PIDFILE
echo "$$" > "$1"
echo 'RESULT a pass assertions=1 evidence=evidence/a.txt'
exec sleep 600
