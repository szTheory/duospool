# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# mirror-oracle.awk - the independent bank-signature oracle of the plain mapper (D-44, D-59). POSIX awk.
# usage: awk -v size=KIB -f tests/conformance/mirror-oracle.awk        KIB is 256, 384 or 1024
# stdout: one row per bank b of the window 0 to 127, tab separated:  SIZE BANK FILEBANK FIRST LAST
#   SIZE the size in KiB, BANK the bank index, FILEBANK the 8 KiB file bank the rule maps it to, FIRST and LAST the tag bytes
#   that the synthetic image carries in the first and the last byte of that file bank (decimal).
# This file shares no code and no table with the core or with the synthetic generator: the mirror arithmetic below is its own
# (modulo and comparison, no bit operations) and so is its copy of the tag formula, FIRST = (k*37+11) mod 256 and
# LAST = (k*91+5) mod 256 for file bank k. If this and the core disagree, the case that compares them fails.
# exit : 0 rows printed | 2 a bad size argument

function mirror(sz, b) {
  if (sz == 256) return b % 32
  if (sz == 384) {
    if (b < 64) return b % 32
    return 32 + (b % 16)
  }
  return b # 1024
}

BEGIN {
  if (size != 256 && size != 384 && size != 1024) {
    print "ORACLE_BAD_SIZE " size > "/dev/stderr"
    exit 2
  }
  for (bank = 0; bank < 128; bank++) {
    fb = mirror(size + 0, bank)
    printf "%d\t%d\t%d\t%d\t%d\n", size, bank, fb, (fb * 37 + 11) % 256, (fb * 91 + 5) % 256
  }
}
