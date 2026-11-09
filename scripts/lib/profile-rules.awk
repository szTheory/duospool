# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# profile-rules.awk - stage 2: schema rules for ONE profile record rN.toml (PROF-01, D-27, D-28).
# input : toml-subset.awk records (TAB separated), including its END sentinel.
# env   : fname = basename of the record without .toml (for example r1)
#         dname = name of the directory that holds it (must equal profile_id)
# output: one TAB-separated record per line, tag in column 1:
#           V<TAB>CODE<TAB>line<TAB>detail     a violation
#           REF<TAB>ID<TAB>line                a ledger ID to resolve (the wrapper tests the file)
#           DECISION<TAB>path<TAB>line         a decision-record path to resolve (the wrapper tests the file)
#           P<TAB>profile_id                   the record was read (tallied by the wrapper; never a violation)
# If the stage-1 sentinel is missing, only  V NO_SENTINEL  is emitted (a dead stage 1 must not read as a pass).
# This check enforces structure, not truth.
BEGIN {
  FS = "\t"
  fname = ENVIRON["fname"]
  dname = ENVIRON["dname"]
  nv = 0; sentinel = 0
  # the 13 D-28 fields, each with its declared value type: s string, i integer, b boolean
  n = split("media_slot expansion_port vdc_count vpc chip_revision input_port1 io_1000_d6_value io_1000_d7_value lines_per_frame psg_tick_divisor content_byte_order content_copier_header powerup_policy", fl, " ")
  for (i = 1; i <= n; i++) ftype[fl[i]] = "s"
  ftype["vdc_count"] = "i"
  ftype["vpc"] = "b"
  # keys allowed inside a field table, with their types (value is per-field, see ftype)
  want["status"] = "s"; want["allowed"] = "a"; want["proposal"] = "s"
  want["ledger"] = "a"; want["decision"] = "s"; want["label"] = "s"; want["value"] = "*"
  # root keys and their types
  rwant["profile_id"] = "s"; rwant["rev"] = "i"; rwant["unresolved_behaviours"] = "a"
}
function viol(code, line, detail) { nv++; vbuf[nv] = sprintf("V\t%s\t%s\t%s", code, line, detail) }
function nth(s, k,    a) { split(s, a, "\037"); return a[k] }
function cnt(s,    a) { return (s == "") ? 0 : split(s, a, "\037") }
function inlist(s, x,    i, k) { k = cnt(s); for (i = 1; i <= k; i++) if (nth(s, i) == x) return 1; return 0 }
# a ledger ID is looked up as a file name by the wrapper, so only the exact ID shape may pass
function goodid(s) { return s ~ /^LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-[0-9][0-9][0-9][0-9]$/ }
# a decision path is looked up as a file by the wrapper: relative, no parent steps, plain characters
function goodpath(s) { return s ~ /^[A-Za-z0-9._\/-]+$/ && s !~ /^\// && s !~ /\.\./ }
function refs(where, line, s,    i, k, e) {
  k = cnt(s)
  for (i = 1; i <= k; i++) {
    e = nth(s, i)
    if (goodid(e)) printf "REF\t%s\t%s\n", e, line
    else viol("LEDGER_REF_MISSING", line, where ": bad ledger id " e)
  }
}
$1 == "END" { sentinel = 1; next }
$1 == "ERR" {                                          # syntax-stage errors pass straight through
  viol($2, $3, $4)
  if ($2 == "BAD_VALUE" || $2 == "BAD_STRING" || $2 == "BAD_ARRAY" || $2 == "TRAILING_GARBAGE") errkey[$4] = 1
  next
}
{
  t = $1; k = $2
  if (t == "") {
    if (!(k in rwant)) { viol("PROFILE_UNKNOWN_KEY", $5, k); next }
    rtyp[k] = $3; rval[k] = $4; rln[k] = $5
    next
  }
  if (!(t in ftype)) {
    if (!(t in unk)) { unk[t] = 1; viol("PROFILE_UNKNOWN_FIELD", $5, t) }
    next
  }
  if (!(k in want)) { viol("PROFILE_UNKNOWN_KEY", $5, t "." k); next }
  have[t] = 1
  typ[t SUBSEP k] = $3; val[t SUBSEP k] = $4; ln[t SUBSEP k] = $5
}
END {
  if (!sentinel) { print "V\tNO_SENTINEL\t0\tstage 1 produced no END sentinel"; exit }

  # ---- root keys ----
  nroot = split("profile_id rev unresolved_behaviours", rk, " ")
  for (i = 1; i <= nroot; i++) {
    k = rk[i]
    if (!(k in rtyp)) { if (!(k in errkey)) viol("PROFILE_MISSING_FIELD", 0, "root key " k); continue }
    if (rtyp[k] != rwant[k]) { viol("PROFILE_WRONG_TYPE", rln[k], k); rbad[k] = 1 }
  }
  if (("rev" in rtyp) && !("rev" in rbad)) {
    if (fname != "r" rval["rev"]) viol("REV_MISMATCH", rln["rev"], "file " fname ".toml vs rev " rval["rev"])
  } else if (!("rev" in rtyp) && fname !~ /^r[0-9]+$/) viol("REV_MISMATCH", 0, "file name " fname ".toml is not rN.toml")
  if (("profile_id" in rtyp) && !("profile_id" in rbad)) {
    if (rval["profile_id"] != dname) viol("PROFILE_ID_MISMATCH", rln["profile_id"], rval["profile_id"] " vs directory " dname)
  }
  if (("unresolved_behaviours" in rtyp) && !("unresolved_behaviours" in rbad)) refs("unresolved_behaviours", rln["unresolved_behaviours"], rval["unresolved_behaviours"])

  # ---- field tables ----
  for (i = 1; i <= n; i++) {
    f = fl[i]
    if (!(f in have)) { viol("PROFILE_MISSING_FIELD", 0, f); continue }
    # key types; a key of the wrong type is reported once and skipped by the rules below
    for (kk in want) {
      id = f SUBSEP kk
      if (!(id in typ)) continue
      w = (kk == "value") ? ftype[f] : want[kk]
      if (typ[id] != w) { viol("PROFILE_WRONG_TYPE", ln[id], f "." kk); wt[id] = 1 }
    }
    sid = f SUBSEP "status"
    st = ((sid in typ) && !(sid in wt)) ? val[sid] : ""
    vid = f SUBSEP "value"; pid = f SUBSEP "proposal"; did = f SUBSEP "decision"
    aid = f SUBSEP "allowed"; lid = f SUBSEP "ledger"
    hasval = (vid in typ); hasprop = (pid in typ)
    if (st != "owner-selected" && st != "open") {
      if (!(sid in wt)) viol("PROFILE_BAD_STATUS", ((sid in ln) ? ln[sid] : 0), f ": " ((sid in typ) ? st : "status missing"))
    } else if (st == "open") {
      if (hasval) viol("OPEN_HAS_VALUE", ln[vid], f)
      if (!hasprop) viol("OPEN_NO_PROPOSAL", 0, f)
    } else {
      if (!hasval) { if (!("value" in errkey)) viol("SELECTED_NO_VALUE", 0, f) }
      if (!(did in typ)) viol("SELECTED_NO_DECISION", 0, f)
      if (hasprop) viol("SELECTED_HAS_PROPOSAL", ln[pid], f)
    }
    if ((did in typ) && !(did in wt)) {
      if (goodpath(val[did])) printf "DECISION\t%s\t%s\n", val[did], ln[did]
      else viol("SELECTED_NO_DECISION", ln[did], f ": not a plain relative path")
    }
    # allowed set: the value (and a proposal) must be a member, by string form
    if ((aid in typ) && !(aid in wt)) {
      if (hasval && !(vid in wt) && !inlist(val[aid], val[vid])) viol("VALUE_NOT_ALLOWED", ln[vid], f ": " val[vid])
      if (hasprop && !(pid in wt) && !inlist(val[aid], val[pid])) viol("VALUE_NOT_ALLOWED", ln[pid], f ": proposal " val[pid])
    }
    if ((lid in typ) && !(lid in wt)) refs(f ".ledger", ln[lid], val[lid])
  }
  if (("profile_id" in rtyp) && !("profile_id" in rbad)) printf "P\t%s\n", rval["profile_id"]
  else printf "P\t%s\n", "?"
  for (i = 1; i <= nv; i++) print vbuf[i]
}
