# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# provenance-register.awk - parse the PROVENANCE.md register (D-26). Reads the register on stdin or as a file.
# env : today = YYYY-MM-DD (used only for the staleness warning)
# out : tab-separated records
#   V<TAB>CODE<TAB>LINE<TAB>DETAIL   violation in the register
#   W<TAB>CODE<TAB>LINE<TAB>DETAIL   warning (never affects the exit code)
#   I<TAB>ID                         a well-formed register id (for marker resolution)
#   P<TAB>ID<TAB>PATH<TAB>LINE       local path to check for existence and the marker of ID
#   Q<TAB>PATH<TAB>LINE              local path to check for existence only (the id is malformed)
#   N<TAB>COUNT                      number of entry blocks parsed
# Only the text after the line "## Entries" is parsed. Entries are blocks separated by blank lines.
# A block that holds a syntax error is reported once and is not field-checked. No interval expressions are
# used, so the script runs under mawk, BWK awk and gawk alike.

BEGIN {
  FS = "\n"
  nk = split("id source-url upstream-ref upstream-path license-spdx license-url upstream-notice local-paths modifications copied-on copied-by maintenance-owner why-copy-not-dependency sync-policy licence-decision", ks, " ")
  for (i = 1; i <= nk; i++) known[ks[i]] = 1
  na = split("MIT Apache-2.0 BSD-2-Clause BSD-3-Clause Zlib ISC 0BSD CC0-1.0 OR", al, " ")
  for (i = 1; i <= na; i++) allowed[al[i]] = 1
  inentries = 0
  nb = 0
  nentries = 0
  tdays = days(ENVIRON["today"])
}

function viol(code, line, detail) { printf "V\t%s\t%d\t%s\n", code, line, detail }

# days since a fixed epoch for a YYYY-MM-DD string (civil-from-days algorithm; years >= 0)
function days(s,    y, m, d, era, yoe, doy, doe) {
  y = substr(s, 1, 4) + 0; m = substr(s, 6, 2) + 0; d = substr(s, 9, 2) + 0
  if (m <= 2) y--
  era = int(y / 400)
  yoe = y - era * 400
  doy = int((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5) + d - 1
  doe = yoe * 365 + int(yoe / 4) - int(yoe / 100) + doy
  return era * 146097 + doe
}

function trim(s) {
  sub(/^[ \t]+/, "", s)
  sub(/[ \t]+$/, "", s)
  return s
}

function finish_block(    i, k, v, c, bad, line1, id, idok, sp, n, t, ok, np, ps, p, kl) {
  if (nb == 0) return
  line1 = bn[1]
  bad = 0
  delete val
  delete kline
  for (i = 1; i <= nb; i++) {
    if (bl[i] ~ /^[a-z][a-z-]*:( .*)?$/) {
      c = index(bl[i], ":")
      k = substr(bl[i], 1, c - 1)
      v = trim(substr(bl[i], c + 1))
      if (k in val) { viol("PROV_SYNTAX", bn[i], "duplicate key " k); bad = 1 }
      else { val[k] = v; kline[k] = bn[i] }
    } else {
      viol("PROV_SYNTAX", bn[i], bl[i])
      bad = 1
    }
  }
  nb = 0
  if (bad) return
  nentries++

  for (k in val) if (!(k in known)) viol("PROV_UNKNOWN_FIELD", kline[k], k)
  for (i = 1; i <= nk; i++) if (!(ks[i] in val) || val[ks[i]] == "") viol("PROV_MISSING_FIELD", line1, ks[i])

  id = val["id"]
  idok = 0
  if (id != "") {
    if (id ~ /^PROV-[0-9][0-9][0-9][0-9]$/) {
      idok = 1
      seen[id]++
      if (seen[id] > 1) viol("PROV_DUP_ID", kline["id"], id)
      printf "I\t%s\n", id
    } else viol("PROV_BAD_ID", kline["id"], id)
  }

  sp = val["sync-policy"]
  if (sp != "" && sp != "frozen" && sp != "diverged" && sp != "tracked") viol("PROV_BAD_SYNC_POLICY", kline["sync-policy"], sp)

  v = val["license-spdx"]
  if (v != "") {
    n = split(v, t, " ")
    ok = 1
    for (i = 1; i <= n; i++) if (!(t[i] in allowed)) ok = 0
    if (!ok) viol("PROV_BAD_SPDX", kline["license-spdx"], v)
  }

  v = val["upstream-ref"]
  if (v != "") {
    ok = 0
    if (length(v) == 40 && v ~ /^[0-9a-f]+$/) ok = 1
    else if (v !~ /[ \t]/) ok = 1
    if (!ok) viol("PROV_BAD_REF", kline["upstream-ref"], v)
  }

  v = val["copied-on"]
  datok = 0
  if (v != "") {
    if (v ~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$/) datok = 1
    else viol("PROV_BAD_DATE", kline["copied-on"], v)
  }
  if (idok && datok && sp == "tracked" && tdays - days(v) > 365) printf "W\tPROV_STALE\t%d\t%s\n", kline["copied-on"], id

  v = val["local-paths"]
  if (v != "") {
    np = split(v, ps, ",")
    for (i = 1; i <= np; i++) {
      p = trim(ps[i])
      if (p == "") p = "<empty>"
      if (idok) printf "P\t%s\t%s\t%d\n", id, p, kline["local-paths"]
      else printf "Q\t%s\t%d\n", p, kline["local-paths"]
    }
  }
}

$0 == "## Entries" && !inentries { inentries = 1; next }
!inentries { next }
$0 ~ /^[ \t\r]*$/ { finish_block(); next }
{ nb++; bl[nb] = $0; bn[nb] = NR }

END {
  finish_block()
  if (!inentries) viol("PROV_SYNTAX", 0, "no ## Entries line")
  printf "N\t%d\n", nentries
}
