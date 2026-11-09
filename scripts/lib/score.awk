# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# score.awk - the executable form of the D-07 decision rule, as pre-registered in
# docs/decisions/0001-language-build.protocol.md section (6) and confirmed by the owner under D-37.
# usage: awk -f scripts/lib/score.awk SCORES
# input (one fact per line, '#' starts a comment):
#   gate  <c|rust> <G1..G5> <PASS|FAIL|ND>      ten lines
#   score <c|rust> <C1..C6> <0.0-5.0 | ND>      twelve lines, at most one decimal digit
#   amendment <yes|no>                          one line
# stdout, in this order:
#   TOTAL c N / TOTAL rust N / WORST c N / WORST rust N / MARGIN nominal N worst N / LEADER c|rust|none /
#   RESULT=owner-recorded-preauth|proposed-default / REASON=...
# exit  : 0 scored | 2 malformed input (SCORE_INPUT_BAD) or missing input (SCORE_INPUT_INCOMPLETE)
# Arithmetic is integer only. A score is held as score10 (tenths of a point, from splitting the text on "."), and
# points = weight x score10 x 2, so 100.00 points is 10000 and nothing is rounded before the margin comparison.
# POSIX awk only: no interval expressions, no gawk extensions.
BEGIN {
  W["C1"] = 25; W["C2"] = 20; W["C3"] = 15; W["C4"] = 15; W["C5"] = 15; W["C6"] = 10
  nc = split("C1 C2 C3 C4 C5 C6", CL, " ")
  ng = split("G1 G2 G3 G4 G5", GL, " ")
  OPT["c"] = 1; OPT["rust"] = 1
  bad = 0; amend = ""
}

function flag_bad(msg) {
  print "SCORE_INPUT_BAD " msg
  bad = 1
}

# line-level comments and blank lines
{ sub(/#.*/, "") }
NF == 0 { next }

$1 == "gate" && NF == 4 {
  o = $2; g = $3; v = $4
  if (!(o in OPT)) { flag_bad("option " o); next }
  if (g !~ /^G[1-5]$/) { flag_bad("gate " g); next }
  if (v != "PASS" && v != "FAIL" && v != "ND") { flag_bad("gate value " v); next }
  if ((o, g) in gate) { flag_bad("duplicate gate " o " " g); next }
  gate[o, g] = v
  next
}

$1 == "score" && NF == 4 {
  o = $2; c = $3; v = $4
  if (!(o in OPT)) { flag_bad("option " o); next }
  if (!(c in W)) { flag_bad("criterion " c); next }
  if ((o, c) in sc) { flag_bad("duplicate score " o " " c); next }
  if (v == "ND") { sc[o, c] = "ND"; next }
  if (v !~ /^[0-5](\.[0-9])?$/) { flag_bad("score " o " " c " " v); next }
  ip = substr(v, 1, 1) + 0
  dp = 0
  if (index(v, ".") > 0) dp = substr(v, 3, 1) + 0
  s10 = ip * 10 + dp
  if (s10 > 50) { flag_bad("score above 5.0 " o " " c " " v); next }
  sc[o, c] = s10
  next
}

$1 == "amendment" && NF == 2 {
  if ($2 != "yes" && $2 != "no") { flag_bad("amendment " $2); next }
  if (amend != "") { flag_bad("duplicate amendment line"); next }
  amend = $2
  next
}

{ flag_bad("line " NR ": " $0) }

# total OPT LEADER-OPT ND-VALUE-FOR-THIS-OPTION: points for OPT with its ND criteria scored at ndv
function total(o, ndv,   i, t, s) {
  t = 0
  for (i = 1; i <= nc; i++) {
    s = sc[o, CL[i]]
    if (s == "ND") s = ndv
    t += W[CL[i]] * s * 2
  }
  return t
}

# fails(OPT): 1 when any of the option's gates is not PASS (FAIL, or ND which is not a demonstrated pass)
function fails(o,   i) {
  for (i = 1; i <= ng; i++) if (gate[o, GL[i]] != "PASS") return 1
  return 0
}

END {
  missing = 0
  for (o in OPT) {
    for (i = 1; i <= ng; i++) if (!((o, GL[i]) in gate)) { print "SCORE_INPUT_INCOMPLETE gate " o " " GL[i]; missing = 1 }
    for (i = 1; i <= nc; i++) if (!((o, CL[i]) in sc)) { print "SCORE_INPUT_INCOMPLETE score " o " " CL[i]; missing = 1 }
  }
  if (amend == "") { print "SCORE_INPUT_INCOMPLETE amendment"; missing = 1 }
  if (bad || missing) exit 2

  nom["c"] = total("c", 25)
  nom["rust"] = total("rust", 25)
  if (nom["c"] > nom["rust"]) { leader = "c"; other = "rust" }
  else if (nom["rust"] > nom["c"]) { leader = "rust"; other = "c" }
  else { leader = "none"; other = "" }

  # worst case: the leader's ND items score 0 and the other option's score 5.0 (score10 50)
  if (leader == "none") {
    wst["c"] = nom["c"]; wst["rust"] = nom["rust"]
    nmargin = 0; wmargin = 0
  } else {
    wst[leader] = total(leader, 0)
    wst[other] = total(other, 50)
    nmargin = nom[leader] - nom[other]
    wmargin = wst[leader] - wst[other]
  }

  print "TOTAL c " nom["c"]
  print "TOTAL rust " nom["rust"]
  print "WORST c " wst["c"]
  print "WORST rust " wst["rust"]
  print "MARGIN nominal " nmargin " worst " wmargin
  print "LEADER " leader

  # precedence, first match wins
  if (amend == "yes") reason = "amended-after-spike"
  else if (leader == "none") reason = "tie"
  else if (fails("c") && fails("rust")) reason = "both-fail-gates"
  else if (fails(leader)) reason = "leader-fails-gate"
  else if (nmargin < 1000) reason = "margin-below-10"
  else if (wmargin < 1000) reason = "nd-sensitivity"
  else reason = "clear-winner"

  if (reason == "clear-winner") print "RESULT=owner-recorded-preauth"
  else print "RESULT=proposed-default"
  print "REASON=" reason
  exit 0
}
