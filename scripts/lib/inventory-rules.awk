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
# Rule of this stage:
#   INV_CASE_FIELD   a case file that is not a well-formed case: one of the ten keys of D-58 and D-79(a) (id kind fixture request expect
#                    meaning required control expect_outcome pins) is absent (request included, an empty value is allowed), a key is of the
#                    wrong type or unknown, a table or a syntax error is present, the file name is not an id, or id differs from the file stem.
#                    The seven optional keys of plan 03-04 (until_mclk policies checks input irq_stim sinks chunks) may be absent; present, each has
#                    its type (until_mclk and checks integers, the rest strings), and any other key is still unknown.
#   INV_CASE_TYPE    until_mclk or checks of a case is not an integer (plan 03-04, T-03-11); the wrong type of any other key stays INV_CASE_FIELD.
#                    The rule line carries the tag rule-case-int-key; no selftest covers it: deleting the line leaves the control case-optional-wrong-type
#                    without its code, so the inventory-negatives stage (run_controls) goes red.
BEGIN {
  FS = "\t"
  fname = ENVIRON["fname"]
  what = ENVIRON["what"]
  nv = 0; sentinel = 0
  n = split("id kind fixture request expect meaning required control expect_outcome pins", fl, " ")
  split("s s s s s s b b s a", ft, " ")
  for (i = 1; i <= n; i++) want[fl[i]] = ft[i]
  # the optional keys (plan 03-04, F1): in want so that they are not unknown, but never in the missing-key loop (that loop runs over fl, the ten keys)
  nopt = split("until_mclk policies checks input irq_stim sinks chunks", ol, " ")
  split("i s i s s s s", ot, " ")
  for (i = 1; i <= nopt; i++) want[ol[i]] = ot[i]
}
function viol(code, line, detail) { nv++; vbuf[nv] = sprintf("V\t%s\t%s\t%s", code, line, detail) }
$1 == "END" { sentinel = 1; next }
$1 == "ERR" { if (what == "case") viol("INV_CASE_FIELD", $3, "syntax " $2); next }   # a ledger file's syntax is the business of ledger-check
{
  if (what == "ledger") {
    if ($1 != "") next
    if ($2 == "id" && $3 == "s") lid = $4
    if ($2 == "pinning_test" && $3 == "s") lpin = $4
    next
  }
  if ($1 != "") { viol("INV_CASE_FIELD", $5, "table " $1); next }
  if (!($2 in want)) { viol("INV_CASE_FIELD", $5, "unknown key " $2); next }
  if (($2 == "until_mclk" || $2 == "checks") && $3 != "i") { viol("INV_CASE_TYPE", $5, "type of " $2 " is not an integer"); next }  # rule-case-int-key
  if ($3 != want[$2]) { viol("INV_CASE_FIELD", $5, "type of " $2); bad[$2] = 1; next }
  have[$2] = 1; val[$2] = $4; ln[$2] = $5
}
END {
  if (!sentinel) viol("NO_SENTINEL", 0, "")
  if (what == "case") {
    for (i = 1; i <= n; i++) if (!(fl[i] in have) && !(fl[i] in bad)) viol("INV_CASE_FIELD", 0, "missing key " fl[i])  # rule-case-field
    if (fname !~ /^[a-z][a-z0-9_]*$/) viol("INV_CASE_FIELD", 0, "the file name is not an id")
    if (("id" in have) && val["id"] != fname) viol("INV_CASE_FIELD", ln["id"], "id " val["id"] " is not the file name " fname)
  }
  for (i = 1; i <= nv; i++) print vbuf[i]
  if (what == "ledger") printf "L\t%s\t%s\n", (lid != "" ? lid : fname), lpin
  else { cpins = val["pins"]; gsub(/\037/, ",", cpins); printf "C\t%s\t%s\t%s\n", fname, val["required"], cpins }
}
