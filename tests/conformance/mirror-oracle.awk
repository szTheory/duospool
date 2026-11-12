# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# mirror-oracle.awk - the independent bank-signature oracle of the plain mapper (D-44, D-59). POSIX awk.
# usage: awk -v size=KIB [-v ledger=DIR] -f tests/conformance/mirror-oracle.awk        KIB is 256, 384 or 1024
#   DIR is the ledger entry directory, default ledger/entries (run from the repository root)
# stdout: one row per bank b of the window 0 to 127, tab separated:  SIZE BANK FILEBANK FIRST LAST
#   SIZE the size in KiB, BANK the bank index, FILEBANK the 8 KiB file bank the rule maps it to, FIRST and LAST the tag bytes
#   that the synthetic image carries in the first and the last byte of that file bank (decimal).
# This file shares no code and no table with the core or with the synthetic generator: the mirror arithmetic below is its own
# (modulo and comparison, no bit operations) and so is its copy of the tag formula, FIRST = (k*37+11) mod 256 and
# LAST = (k*91+5) mod 256 for file bank k. If this and the core disagree, the case that compares them fails.
# Rule-text guard (T-02-14): before printing, the statements of LED-LOAD-0003 and LED-LOAD-0005 are read and must contain each
# rule clause below literally. If one is missing or reworded, or a file cannot be read, nothing is printed except the line
# ORACLE_RULE_TEXT_DRIFT file=... clause=... on stderr, and the exit status is 1: an oracle that no longer mirrors the ledger
# rule must not judge anything, and a rule changed in the ledger must be changed here on purpose, in its own arithmetic.
# exit : 0 rows printed | 1 the rule text drifted | 2 a bad size argument

function mirror(sz, b) {
  if (sz == 256) return b % 32
  if (sz == 384) {
    if (b < 64) return b % 32
    return 32 + (b % 16)
  }
  return b # 1024
}

# the statement line of an entry file, "" when the file cannot be read or has none
function statement_of(path,   line, stmt) {
  stmt = ""
  while ((getline line < path) > 0) {
    if (line ~ /^statement = /) stmt = line
  }
  close(path)
  return stmt
}

# one clause must be a literal substring of the statement of the entry and must end there: the next character is one of ; . : or
# the closing quote, so that "1M file bank = b mod 5" does not pass for "1M file bank = b"
function guard(id, clause,   path, stmt, at, ok) {
  path = ledger_dir "/" id ".toml"
  stmt = statement_of(path)
  at = index(stmt, clause)
  ok = 0
  if (at > 0) ok = (substr(stmt, at + length(clause), 1) ~ /^[;.:"]$/)
  if (!ok) {
    print "ORACLE_RULE_TEXT_DRIFT file=" path " clause=" clause > "/dev/stderr"
    drifted = 1
  }
}

BEGIN {
  ledger_dir = (ledger == "") ? "ledger/entries" : ledger
  guard("LED-LOAD-0003", "256K file bank = b mod 32")
  guard("LED-LOAD-0003", "384K file bank = b and 0x1F when b below 0x40, 0x20 + (b and 0x0F) when b is 0x40 or above")
  guard("LED-LOAD-0003", "1M file bank = b")
  guard("LED-LOAD-0005", "banks 0x80 and above are not ROM")
  if (drifted) exit 1
  if (size != 256 && size != 384 && size != 1024) {
    print "ORACLE_BAD_SIZE " size > "/dev/stderr"
    exit 2
  }
  for (bank = 0; bank < 128; bank++) {
    fb = mirror(size + 0, bank)
    printf "%d\t%d\t%d\t%d\t%d\n", size, bank, fb, (fb * 37 + 11) % 256, (fb * 91 + 5) % 256
  }
}
