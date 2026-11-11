# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# fixture-lint-rules.awk - stage 2 of scripts/fixture-lint: the expected-value class rules of D-125 (with the R-3 reading of the size-hint rule) over
# the records of fixture-lint-lex.awk (plan 03-37). POSIX awk only.
# input : the stage-1 records, including its END sentinel.
# env   : -v ledger=FILE the ledger summary of the stage-0 reader of scripts/fixture-lint: I<TAB>path<TAB>id<TAB>status per entry, F<TAB>path<TAB>id<TAB>status<TAB>ref per FACT source, END (plan 03-32)
#         -v cites=FILE  the cite registry (fixtures/lib/cites.txt: label TAB flag TAB verifier TAB date TAB fact); fixture-lint has already checked its shape
#         -v tsv=FILE    the opcode table (tests/oracle/huc6280-opcodes.tsv: opcode TAB mnemonic TAB mode TAB ...): the mnemonic column, upper case, UNDEF rows skipped, is the set of instruction words a fixture may use
# output: V<TAB>file<TAB>CODE<TAB>line<TAB>detail   one per violation
#         N<TAB>fixtures<TAB>checks<TAB>raw          always printed once when the stage ran
#         X<TAB>FXL_STAGE_DEAD<TAB>detail            when the stage-1 sentinel is missing; exit status 2 (a dead stage 1 must never read as a pass)
# Codes (one rule each; the statement that emits a code ends in a  # rule-NAME  tag, so that a selftest can delete it and show its control go green):
#   FXL_ORPHAN_CHECK   a .checks row that no CHECK_LIT or CHECK_GUEST of the source uses (or a second row for an index already listed)   rule-orphan
#   FXL_UNLISTED_CHECK a CHECK_LIT or CHECK_GUEST that no .checks row lists (or a check number used twice)                                rule-unlisted
#   FXL_BAD_CLASS      the class tag is not MAN:<page>, GUEST:<model> or ASSUMED:LED-AREA-NNNN (DuoSpool output and other emulators are no class)  rule-bad-class
#   FXL_COUNT          the id= or checks= header is missing or malformed, or checks= is not the number of rows                              rule-count
#   FXL_INCLUDE_PATH   an .include that is not a bare file name (resolved through fixtures/lib), fixtures/lib/NAME or build/PATH            rule-include-path
#   FXL_SIZE_HINT      an instruction operand that is a literal below $100 without z:, a: or < in front (Pitfall 5; (zp) and BBR/BBS take <)  rule-size-hint
#   FXL_RAW_UNCITED    a source line with a RAW marker in its comment that does not read RAW:<page>                                         rule-raw-uncited
#   FXL_UNKNOWN_CITE   a MAN:<page> or RAW:<page> whose page is not in cites.txt, or a page named in a FACT source of a supported entry that cites.txt does not list                                                           rule-unknown-cite
#   FXL_BAD_OBSERVE    a .checks fifth column other than the word observe (or a sixth column), or a row marked observe whose check is not CHECK_OBSERVE, or a CHECK_OBSERVE whose row is not marked observe     rule-bad-observe
#   FXL_OCR_BACKS_SUPPORTED a MAN:<page> check whose page is flagged ocr in cites.txt while a supported ledger entry names that page in a FACT source, or a supported entry whose FACT source names an ocr page (ER-1)   rule-ocr-backs-supported
#   FXL_UNKNOWN_LEDGER_ID an ASSUMED:<LED-id> whose id has no entry in the ledger (D-125)                                                   rule-unknown-ledger-id
#   FXL_UNKNOWN_MNEMONIC a lower-case first word of a source line that is not a mnemonic of the opcode table (an assignment  name = value  is not an instruction)  rule-unknown-mnemonic
# A RAW line is any source line whose comment holds the word RAW: raw instruction bytes written in place of an instruction. Data tables are .byte lines without the marker.
# The mnemonic set is the table of tests/oracle/huc6280-opcodes.tsv (plan 03-08), not a list kept here, so the size-hint rule and the unknown-mnemonic rule cannot drift from the manual transcription.
# The two ledger rules (plan 03-32) read the stage-0 summary: a page named in a FACT source of a supported entry is a page that backs a support claim, and must be img in cites.txt (ER-1); a FACT ref lists page labels
# separated by spaces and may name a range  S8-17 to S8-20  (every registered label inside the range counts). An ASSUMED:<id> tag must name an entry of the ledger that is read.
BEGIN {
  FS = "\t"
  sentinel = 0; nfix = 0; nchk = 0; nraw = 0; nst = 0
  while ((getline cl < cites) > 0) {
    if (cl ~ /^#/ || cl ~ /^[ \t]*$/) continue
    split(cl, cf, "\t")
    known[cf[1]] = cf[2]
  }
  close(cites)
  while ((getline tl < tsv) > 0) {
    if (tl ~ /^#/ || tl ~ /^[ \t]*$/) continue
    split(tl, tf, "\t")
    if (tf[2] != "UNDEF") mnem[tolower(tf[2])] = 1
  }
  close(tsv)
  while ((getline ll < ledger) > 0) {
    n = split(ll, lf, "\t")
    if (lf[1] == "END") ledend = 1
    else if (lf[1] == "I") { idknown[lf[3]] = 1 }
    else if (lf[1] == "F" && lf[4] == "supported") factrefs(lf[2], lf[5])
  }
  close(ledger)
  nobl = split("tai tdd tia tii tin", bl, " ")                  # block transfers take absolute operands only: their small numbers are lengths, not zero page
  for (i = 1; i <= nobl; i++) isblock[bl[i]] = 1
}
function islabel(t) { return (t ~ /^(H8-[0-9]+|S8-[0-9]+|CDBIOS-p[0-9]+)$/) }
function labnum(t) { sub(/^[A-Za-z0-9]+-p?/, "", t); return t + 0 }
function labpfx(t) { sub(/[0-9]+$/, "", t); return t }
# one FACT ref of a supported entry: every page label (and every registered label inside a  A to B  range) is recorded as backing a support claim; it must be registered and img (ER-1)
function factrefs(path, ref,   nt, tk, i, a, b, k, lab, pf) {
  nt = split(ref, tk, /[ \t]+/)
  for (i = 1; i <= nt; i++) {
    if (!islabel(tk[i])) continue
    if (tk[i + 1] == "to" && islabel(tk[i + 2]) && labpfx(tk[i]) == labpfx(tk[i + 2])) {
      pf = labpfx(tk[i]); a = labnum(tk[i]); b = labnum(tk[i + 2])
      for (k = a; k <= b; k++) { lab = pf k; supp[lab] = 1; suppfile[lab] = path }
      i += 2
    } else { supp[tk[i]] = 1; suppfile[tk[i]] = path; direct[tk[i]] = 1 }
  }
}
function viol(f, code, ln, detail) { printf "V\t%s\t%s\t%s\t%s\n", f, code, ln, detail }
function stemof(p) { sub(/^.*\//, "", p); sub(/\.[^.]*$/, "", p); return p }
function keyof(v) { return (v ~ /^[0-9]+$/) ? (v + 0) : ("!" v) }
function islit(p,   d) {
  if (p ~ /^\$[0-9A-Fa-f]+$/) { d = substr(p, 2); sub(/^0+/, "", d); return length(d) <= 2 }
  if (p ~ /^%[01]+$/) { d = substr(p, 2); sub(/^0+/, "", d); return length(d) <= 8 }
  if (p ~ /^[0-9]+$/) return (p + 0) < 256
  return 0
}
function addstem(s) { if (!(s in seenstem)) { seenstem[s] = 1; nst++; stemlist[nst] = s } }
$1 == "END" { sentinel = 1; next }
$1 == "F" {
  s = stemof($2); addstem(s)
  if ($2 ~ /\.checks$/) hasc[s] = $2
  else hass[s] = $2
  next
}
$1 == "CHK" {
  s = stemof($2); k = keyof($4); nchk++
  chkn[s, k]++
  if (chkn[s, k] == 1) { chkline[s, k] = $3; chkword[s, k] = $5; nk[s]++; klist[s, nk[s]] = k }
  else dupline[s, k] = $3
  next
}
$1 == "INC" {
  p = $4; gsub(/^"|"$/, "", p)
  okinc = ($4 ~ /^"[^"]+"$/) && (p !~ /\.\./) && (p !~ /^\//) && ((p !~ /\//) || (p ~ /^fixtures\/lib\/[A-Za-z0-9_.-]+$/) || (p ~ /^build\/[A-Za-z0-9_.\/-]+$/))
  if (!okinc) viol($2, "FXL_INCLUDE_PATH", $3, ".include " $4 " is not a bare name, fixtures/lib/NAME or build/PATH")  # rule-include-path
  next
}
$1 == "INS" {
  mn = $4
  if (!(mn in mnem)) {
    if ($5 !~ /^:?=/) viol($2, "FXL_UNKNOWN_MNEMONIC", $3, mn " is not a mnemonic of the opcode table")  # rule-unknown-mnemonic
    next
  }
  if (mn in isblock) next
  np = split($5, pc, ",")
  for (i = 1; i <= np; i++) {
    p = pc[i]
    gsub(/^[ \t]+|[ \t]+$/, "", p)
    while (p ~ /^\(/) p = substr(p, 2)
    while (p ~ /\)$/) p = substr(p, 1, length(p) - 1)
    if (p ~ /^#/) continue
    if (islit(p)) viol($2, "FXL_SIZE_HINT", $3, mn " operand " p " is below $100 and has no z:, a: or < size hint")  # rule-size-hint
  }
  next
}
$1 == "RAW" {
  nraw++
  c = $4
  if (match(c, /(^|[^A-Za-z0-9_])RAW/) == 0) next
  rest = substr(c, RSTART + RLENGTH)
  cite = ""
  if (rest ~ /^:[A-Za-z0-9][A-Za-z0-9-]*/) { cite = substr(rest, 2); sub(/[^A-Za-z0-9-].*$/, "", cite) }
  if (cite == "") viol($2, "FXL_RAW_UNCITED", $3, "RAW line without a RAW:<page> cite")  # rule-raw-uncited
  else if (!(cite in known)) viol($2, "FXL_UNKNOWN_CITE", $3, "RAW:" cite " is not in cites.txt")  # rule-unknown-cite
  next
}
$1 == "HDR" {
  s = stemof($2)
  hdr[s, $4] = $5; hdrline[s, $4] = $3
  next
}
$1 == "ROW" {
  s = stemof($2)
  nr[s]++
  rowidx[s, nr[s]] = keyof($4); rowline[s, nr[s]] = $3; rowfile[s] = $2; rowobs[s, nr[s]] = ($6 + 0 == 5 && $7 == "observe")
  if ($6 + 0 > 5 || ($6 + 0 == 5 && $7 != "observe")) viol($2, "FXL_BAD_OBSERVE", $3, "the fifth column of a .checks row is only the word observe, and there is no sixth")  # rule-bad-observe
  cls = $5
  okcls = (cls ~ /^MAN:[A-Za-z0-9][A-Za-z0-9-]*$/) || (cls ~ /^GUEST:[a-z][a-z0-9-]*$/) || (cls ~ /^ASSUMED:LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-[0-9][0-9][0-9][0-9]$/)
  if (!okcls) viol($2, "FXL_BAD_CLASS", $3, "class tag '" cls "' is not MAN:<page>, GUEST:<model> or ASSUMED:<LED-id>")  # rule-bad-class
  else if (cls ~ /^MAN:/ && !(substr(cls, 5) in known)) viol($2, "FXL_UNKNOWN_CITE", $3, cls " is not in cites.txt")  # rule-unknown-cite
  else if (cls ~ /^MAN:/ && (substr(cls, 5) in supp) && known[substr(cls, 5)] == "ocr") viol($2, "FXL_OCR_BACKS_SUPPORTED", $3, cls " backs a supported entry but its page is flagged ocr in cites.txt")  # rule-ocr-backs-supported
  if (okcls && cls ~ /^ASSUMED:/ && !(substr(cls, 9) in idknown)) viol($2, "FXL_UNKNOWN_LEDGER_ID", $3, cls " names no entry of the ledger")  # rule-unknown-ledger-id
  next
}
END {
  if (!sentinel) {
    printf "X\tFXL_STAGE_DEAD\tstage 1 printed no END sentinel\n"
    exit 2
  }
  if (!ledend) {
    printf "X\tFXL_STAGE_DEAD\tthe ledger summary has no END sentinel\n"
    exit 2
  }
  for (lab in supp) {
    if (!(lab in known) && (lab in direct)) viol(suppfile[lab], "FXL_UNKNOWN_CITE", 0, "a supported entry cites " lab " in a FACT source and cites.txt has no such page")  # rule-unknown-cite
    if ((lab in known) && known[lab] == "ocr") viol(suppfile[lab], "FXL_OCR_BACKS_SUPPORTED", 0, "a supported entry cites " lab " in a FACT source and its page is flagged ocr in cites.txt")  # rule-ocr-backs-supported
  }
  for (si = 1; si <= nst; si++) {
    s = stemlist[si]
    if (s in hass) nfix++
    for (j = 1; j <= nk[s]; j++) {
      k = klist[s, j]
      listed = 0
      for (r = 1; r <= nr[s]; r++) if (rowidx[s, r] == k) listed = 1
      if (!listed) viol(hass[s], "FXL_UNLISTED_CHECK", chkline[s, k], "check " k " has no row in " s ".checks")  # rule-unlisted
      if (chkn[s, k] > 1) viol(hass[s], "FXL_UNLISTED_CHECK", dupline[s, k], "check " k " is used more than once")  # rule-unlisted
    }
    for (r = 1; r <= nr[s]; r++) {
      k = rowidx[s, r]
      seen[s, k]++
      if (!((s, k) in chkn) || seen[s, k] > 1) viol(rowfile[s], "FXL_ORPHAN_CHECK", rowline[s, r], "row " k " matches no CHECK of " s ".s, or repeats an earlier row")  # rule-orphan
      if (((s, k) in chkn) && seen[s, k] == 1 && rowobs[s, r] != (chkword[s, k] == "CHECK_OBSERVE")) viol(rowfile[s], "FXL_BAD_OBSERVE", rowline[s, r], "row " k " and its CHECK disagree: observe in the fifth column goes with CHECK_OBSERVE and with nothing else")  # rule-bad-observe
    }
    if (s in hasc) {
      if (!((s, "id") in hdr) || hdr[s, "id"] !~ /^[1-9][0-9]*$/) viol(hasc[s], "FXL_COUNT", 0, "id= header missing or not a positive number")  # rule-count
      if (!((s, "checks") in hdr) || hdr[s, "checks"] !~ /^[0-9]+$/) viol(hasc[s], "FXL_COUNT", 0, "checks= header missing or not a number")  # rule-count
      else if ((hdr[s, "checks"] + 0) != nr[s] + 0) viol(hasc[s], "FXL_COUNT", hdrline[s, "checks"], "checks=" hdr[s, "checks"] " but the file has " nr[s] + 0 " rows")  # rule-count
    }
  }
  printf "N\t%d\t%d\t%d\n", nfix, nchk, nraw
}
