# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# bankclass-oracle.awk - the bank-class oracle of the memory map (plan 03-20; D-103, ER-7, ASSUMED LED-CPU-0011). POSIX awk.
# usage: awk [-v ledger=DIR] -f tests/conformance/bankclass-oracle.awk        DIR is the ledger entry directory, default ledger/entries (run from the repository root)
# stdout: 256 rows, one per bank 0 to 255, tab separated:  BANK CLASS   with CLASS one of HUCARD, UNMAPPED, RAM, HWPAGE.
# What this is: a re-derivation of the class of every bank from the statement of LED-CPU-0011 by the same author as the core (ER-7). It shares no code and no table with src/core: the arithmetic below is its own (comparisons of the
# bank number against the ends of the runs the statement names), and the class names are its own words. It is therefore E0/E2 labelled: it shows that the core's table equals one reading of the ledger text, not that the text is right.
# Rule-text guard (T-03-54, the precedent of mirror-oracle.awk): before printing, the statement of LED-CPU-0011 is read and must contain each clause below literally, and each clause must end there (the next character is one of ; . : or the closing
# quote). If one is missing or reworded, or the file cannot be read, nothing is printed except the line ORACLE_RULE_TEXT_DRIFT file=... clause=... on stderr, and the exit status is 1: an oracle that no longer mirrors the ledger text must not
# judge anything, and a rule changed in the ledger must be changed here on purpose.
# exit : 0 rows printed | 1 the rule text drifted

# the statement line of an entry file, "" when the file cannot be read or has none
function statement_of(path,   line, stmt) {
  stmt = ""
  while ((getline line < path) > 0) {
    if (line ~ /^statement = /) stmt = line
  }
  close(path)
  return stmt
}

# one clause must be a literal substring of the statement and must end there
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
  guard("LED-CPU-0011", "banks 0x00-0x7F HuCard")
  guard("LED-CPU-0011", "banks 0x80-0xF7 unmapped")
  guard("LED-CPU-0011", "banks 0xF8-0xFB the 8 KiB work RAM mirrored four times")
  guard("LED-CPU-0011", "banks 0xFC-0xFE unmapped")
  guard("LED-CPU-0011", "bank 0xFF the hardware page of six 1 KiB windows (VDC, VCE, PSG, timer, I/O, interrupt registers) and an empty 2 KiB")
  if (drifted) exit 1
  # the ends of the runs of the statement, in decimal: 0x7F 127, 0xF7 247, 0xFB 251, 0xFE 254, 0xFF 255
  for (bank = 0; bank < 256; bank++) {
    if (bank <= 127) class = "HUCARD"
    else if (bank <= 247) class = "UNMAPPED"
    else if (bank <= 251) class = "RAM"
    else if (bank <= 254) class = "UNMAPPED"
    else class = "HWPAGE"
    printf "%d\t%s\n", bank, class
  }
}
