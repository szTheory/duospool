#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# gen-fixtures - generate every TOOL-01 spike fixture into spikes/tool-01/out/ (W-12, D-35).
# usage: gen-fixtures.sh
#   Bytes come from an integer LCG (seed 12345: x = (x*1664525 + 1013904223) mod 2^32, byte = int(x/65536) mod 256),
#   written as octal escapes by awk and turned into bytes by the shell's printf. awk rand() and real ROM data are
#   never used. The output directory is gitignored: generated fixtures are never committed.
#   Each file's sha256 is checked against the "fixture" rows of fixtures.manifest.
# stdout: one line per fixture  FIXTURE <name> OK|MISMATCH|MISSING
# exit  : 0 all fixtures match | 1 a mismatch | 2 harness error
set -eu
LC_ALL=C; export LC_ALL
here=$(cd "$(dirname "$0")" && pwd)
out=$here/out
manifest=$here/fixtures.manifest

[ -f "$manifest" ] || { echo "HARNESS_NO_FILE $manifest" >&2; exit 2; }
mkdir -p "$out"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# 98304 LCG bytes: bad-98304.bin. lcg-64k.bin is its first 65536 bytes (the first 65536 LCG bytes).
awk 'BEGIN { x = 12345; for (i = 0; i < 98304; i++) { x = (x * 1664525 + 1013904223) % 4294967296; printf "\\%03o", int(x / 65536) % 256 } }' > "$tmp/lcg.esc"
printf "$(cat "$tmp/lcg.esc")" > "$out/bad-98304.bin"
dd if="$out/bad-98304.bin" of="$out/lcg-64k.bin" bs=65536 count=1 2> /dev/null
dd if="$out/lcg-64k.bin" of="$out/trunc-65535.bin" bs=65535 count=1 2> /dev/null
: > "$out/empty.bin"
dd if=/dev/zero of="$out/zero-64k.bin" bs=65536 count=1 2> /dev/null
dd if=/dev/zero of="$out/cap-1048576.bin" bs=1048576 count=1 2> /dev/null
dd if=/dev/zero of="$out/over-1048577.bin" bs=1048577 count=1 2> /dev/null
{ dd if=/dev/zero bs=512 count=1 2> /dev/null; cat "$out/lcg-64k.bin"; } > "$out/hdr-66048.bin"

rc=0
n=0
while read -r kind name size sum rest; do
  [ "$kind" = fixture ] || continue
  n=$((n + 1))
  if [ ! -f "$out/$name" ]; then
    echo "FIXTURE $name MISSING"
    rc=1
    continue
  fi
  got_size=$(wc -c < "$out/$name" | tr -d ' ')
  got_sum=$(shasum -a 256 "$out/$name" | cut -d ' ' -f 1)
  if [ "$got_size" = "$size" ] && [ "$got_sum" = "$sum" ]; then
    echo "FIXTURE $name OK"
  else
    echo "FIXTURE $name MISMATCH size=$got_size sha256=$got_sum"
    rc=1
  fi
done < "$manifest"
if [ "$n" -eq 0 ]; then
  echo "HARNESS_NO_FIXTURE_ROWS $manifest" >&2
  exit 2
fi
exit "$rc"
