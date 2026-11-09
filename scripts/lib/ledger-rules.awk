# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# ledger-rules.awk - stage 2: schema + supported-rules for ONE ledger entry (D-21, D-22).
# input : toml-subset.awk records (TAB separated), including its END sentinel.
# env   : fname = basename of the entry without .toml
# output: one TAB-separated record per line, tag in column 1:
#           V<TAB>CODE<TAB>line<TAB>detail    a violation
#           S<TAB>status                      a valid status (tallied by the wrapper; never a violation)
#           I<TAB>id                          the id value (the wrapper tallies DUP_ID across files)
#           F<TAB>fname<TAB>origin            this file's name and origin value (REQUIRED cross-check)
#           PROFILE<TAB>value<TAB>line        the profile value (the wrapper checks that profiles/<value>/rN.toml exists)
# If the stage-1 sentinel is missing, only  V NO_SENTINEL  is emitted (a dead stage 1 must not read as a pass).
# This check enforces structure, not truth.
BEGIN {
  FS = "\t"
  fname = ENVIRON["fname"]
  nv = 0; sentinel = 0
  n = split("id statement profile status sources evidence disagreement pinning_test updated", req, " ")
  m = split("assumption revisit_phase withheld origin", opt, " ")
  for (i = 1; i <= n; i++) known[req[i]] = 1
  for (i = 1; i <= m; i++) known[opt[i]] = 1
  # expected value types: s string, a array, i integer
  split("id statement profile status disagreement pinning_test updated assumption origin", ts, " ")
  for (i in ts) want[ts[i]] = "s"
  split("sources evidence withheld", ta, " ")
  for (i in ta) want[ta[i]] = "a"
  want["revisit_phase"] = "i"
}
function viol(code, line, detail) { nv++; vbuf[nv] = sprintf("V\t%s\t%s\t%s", code, line, detail) }
$1 == "END" { sentinel = 1; next }
$1 == "ERR" {                                          # syntax-stage errors pass straight through
  viol($2, $3, $4)
  if ($2 == "BAD_VALUE" || $2 == "BAD_STRING" || $2 == "BAD_ARRAY" || $2 == "TRAILING_GARBAGE") errkey[$4] = 1
  next
}
{
  if ($1 != "") { viol("UNKNOWN_TABLE", $5, $1); next }
  key = $2; typ[key] = $3; val[key] = $4; ln[key] = $5
  if (!(key in known)) viol("UNKNOWN_KEY", $5, key)
}
function nth(s, k,    a) { split(s, a, "\037"); return a[k] }
function cnt(s) { return (s == "") ? 0 : split(s, tmp, "\037") }
# a key is usable by the rules when it is present and has the right type
function ok(k) { return (k in typ) && !(k in wt) }
END {
  if (!sentinel) { print "V\tNO_SENTINEL\t0\tstage 1 produced no END sentinel"; exit }
  for (kk in typ) {
    if ((kk in want) && typ[kk] != want[kk]) { viol("WRONG_TYPE", ln[kk], kk); wt[kk] = 1 }
  }
  for (i = 1; i <= n; i++) if (!(req[i] in typ) && !(req[i] in errkey)) viol("MISSING_FIELD", 0, req[i])
  if (ok("id")) {
    if (val["id"] !~ /^LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-[0-9][0-9][0-9][0-9]$/) viol("BAD_ID", ln["id"], val["id"])
    else if (val["id"] != fname) viol("ID_FILENAME_MISMATCH", ln["id"], val["id"] " vs " fname)
    printf "I\t%s\n", val["id"]
  }
  st = ok("status") ? val["status"] : ""
  if (ok("status")) {
    if (st !~ /^(open|assumed|supported|refuted|deferred|withdrawn)$/) viol("UNKNOWN_STATUS", ln["status"], st)
    else printf "S\t%s\n", st
  }
  if (ok("updated") && val["updated"] !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$/) viol("BAD_DATE", ln["updated"], val["updated"])
  hasfact = 0
  if (ok("sources")) {
    k = cnt(val["sources"]); if (k == 0) viol("NO_SOURCE", ln["sources"], "")
    for (i = 1; i <= k; i++) {
      e = nth(val["sources"], i); split(e, p, "|")
      if (p[1] !~ /^(FACT|CODE|REPORT|UNKNOWN)$/ || p[2] == "") viol("BAD_SOURCE", ln["sources"], e)
      if (p[1] == "FACT") hasfact = 1
    }
  }
  q = 0; e1 = 0; e3bad = 0
  if (ok("evidence")) {
    k = cnt(val["evidence"])
    for (i = 1; i <= k; i++) {
      e = nth(val["evidence"], i); split(e, p, "|")
      if (p[1] !~ /^E[0-4]$/ || p[2] == "") { viol("BAD_EVIDENCE", ln["evidence"], e); continue }
      if (p[1] == "E1" || p[1] == "E2" || p[1] == "E3") q++      # E0 and E4 never count
      if (p[1] == "E1") e1 = 1
      if (p[1] == "E3" && p[2] !~ /^docs\/decisions\/[0-9][0-9][0-9][0-9]-/) e3bad = 1
    }
  }
  pin = ok("pinning_test") ? val["pinning_test"] : ""
  if (ok("pinning_test") && pin !~ /^(TEST:.+|PENDING:[0-9]+|NONE:.+)$/) viol("BAD_PINNING", ln["pinning_test"], pin)
  if (st == "assumed") {
    if (!ok("assumption") || val["assumption"] == "") { if (!("assumption" in errkey) && !("assumption" in wt)) viol("ASSUMED_NO_ASSUMPTION", 0, "") }
    if (!ok("revisit_phase")) { if (!("revisit_phase" in errkey) && !("revisit_phase" in wt)) viol("ASSUMED_NO_REVISIT", 0, "") }
  }
  if (st == "supported") {
    if (q == 0) viol("SUPPORTED_NO_QUALIFYING_EVIDENCE", ln["status"], "needs E1/E2/E3; E0 and E4 never count")
    if (e1 && !hasfact) viol("E1_NEEDS_FACT_SOURCE", ln["evidence"], "")
    if (e3bad) viol("E3_NEEDS_OWNER_GATE", ln["evidence"], "")
    if (pin ~ /^PENDING:/) viol("SUPPORTED_PENDING_TEST", ln["pinning_test"], "")
    else if (pin !~ /^TEST:/ && pin != "") viol("SUPPORTED_NEEDS_TEST", ln["pinning_test"], pin)
  }
  printf "F\t%s\t%s\n", fname, (ok("origin") ? val["origin"] : "")
  if (ok("profile")) printf "PROFILE\t%s\t%s\n", val["profile"], ln["profile"]
  for (i = 1; i <= nv; i++) print vbuf[i]
}
