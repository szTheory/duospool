# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# inventory-rules.awk - stage 2 of scripts/inventory-check: what the required-test inventory needs from ONE file (FIX-04, D-58).
# input : toml-subset.awk records (TAB separated), including its END sentinel.
# env   : fname = the file name without .toml;  what = case (a tests/conformance/cases file) or ledger (a ledger/entries file)
# output: one TAB-separated record per line, tag in column 1:
#           V<TAB>CODE<TAB>line<TAB>detail         a violation of this one file
#           C<TAB>stem<TAB>required<TAB>pins       a case: the value of its required key (true, false or empty) and its pins joined by commas
#           L<TAB>id<TAB>pinning_test              a ledger entry: its id (the file name when the key is absent) and its pinning_test value
# A C or L record is always printed once, so that the wrapper can tell a dead stage from a clean file. The cross-file rules (REQUIRED, the
# unlisted case, the pins in both directions) live in scripts/inventory-check, which sees every file; this stage owns the shape of one file.
# If the stage-1 sentinel is missing, V NO_SENTINEL is emitted: a dead stage 1 must not read as a pass.
BEGIN {
  FS = "\t"
  fname = ENVIRON["fname"]
  what = ENVIRON["what"]
  nv = 0; sentinel = 0
}
function viol(code, line, detail) { nv++; vbuf[nv] = sprintf("V\t%s\t%s\t%s", code, line, detail) }
$1 == "END" { sentinel = 1; next }
$1 == "ERR" { next }                    # syntax errors of a file are the business of the checker that owns the file's format
{
  if ($1 != "") next
  if (what == "ledger") {
    if ($2 == "id" && $3 == "s") lid = $4
    if ($2 == "pinning_test" && $3 == "s") lpin = $4
  } else {
    if ($2 == "required" && $3 == "b") creq = $4
    if ($2 == "pins" && $3 == "a") cpins = $4
  }
}
END {
  if (!sentinel) viol("NO_SENTINEL", 0, "")
  for (i = 1; i <= nv; i++) print vbuf[i]
  if (what == "ledger") printf "L\t%s\t%s\n", (lid != "" ? lid : fname), lpin
  else { gsub(/\037/, ",", cpins); printf "C\t%s\t%s\t%s\n", fname, creq, cpins }
}
