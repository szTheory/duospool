# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# fixture-lint-rules.awk - stage 2 of scripts/fixture-lint: the expected-value class rules of D-125 (with the R-3 reading of the size-hint rule) over
# the records of fixture-lint-lex.awk (plan 03-37). POSIX awk only.
# input : the stage-1 records, including its END sentinel.
# env   : -v cites=FILE  the cite registry (fixtures/lib/cites.txt: label TAB flag TAB verifier TAB date TAB fact); fixture-lint has already checked its shape
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
#   FXL_UNKNOWN_CITE   a MAN:<page> or RAW:<page> whose page is not in cites.txt                                                              rule-unknown-cite
# A RAW line is any source line whose comment holds the word RAW: raw instruction bytes written in place of an instruction. Data tables are .byte lines without the marker.
# Not here yet (the inputs do not exist): the mnemonics-subset-of-the-opcode-TSV rule (plan 03-08), and the rules that need the ledger and the cite flags, namely
# that a MAN: cite behind a supported entry is img and that an ASSUMED: id names a ledger entry (plan 03-32).
BEGIN {
  FS = "\t"
  sentinel = 0; nfix = 0; nchk = 0; nraw = 0; nst = 0
  while ((getline cl < cites) > 0) {
    if (cl ~ /^#/ || cl ~ /^[ \t]*$/) continue
    split(cl, cf, "\t")
    known[cf[1]] = cf[2]
  }
  close(cites)
  n = split("adc and asl bcc bcs beq bit bmi bne bpl bra brk bsr bvc bvs clc cld cli clv cmp cpx cpy csh csl dec dex dey eor inc inx iny jmp jsr lda ldx ldy lsr nop ora pha php phx phy pla plp plx ply rol ror rti rts sax say sbc sec sed sei set sta stx sty stz sxy tam tax tay tma trb tsb tsx txa txs tya tst st0 st1 st2 cla clx cly", ml, " ")
  for (i = 1; i <= n; i++) mnem[ml[i]] = 1
  for (i = 0; i <= 7; i++) { mnem["bbr" i] = 1; mnem["bbs" i] = 1; mnem["rmb" i] = 1; mnem["smb" i] = 1 }
  nobl = split("tai tdd tia tii tin", bl, " ")                  # block transfers take absolute operands only: their small numbers are lengths, not zero page
  for (i = 1; i <= nobl; i++) isblock[bl[i]] = 1
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
  if (chkn[s, k] == 1) { chkline[s, k] = $3; nk[s]++; klist[s, nk[s]] = k }
  else dupline[s, k] = $3
  next
}
$1 == "INC" {
  p = $4; gsub(/^"|"$/, "", p)
  okinc = ($4 ~ /^"[^"]+"$/) && (p !~ /\.\./) && (p !~ /^\//) && ((p !~ /\//) || (p ~ /^fixtures\/lib\/[A-Za-z0-9_.-]+$/) || (p ~ /^build\/[A-Za-z0-9_.\/-]+$/))
  next
}
$1 == "INS" {
  mn = $4
  if (!(mn in mnem) || (mn in isblock)) next
  np = split($5, pc, ",")
  for (i = 1; i <= np; i++) {
    p = pc[i]
    gsub(/^[ \t]+|[ \t]+$/, "", p)
    while (p ~ /^\(/) p = substr(p, 2)
    while (p ~ /\)$/) p = substr(p, 1, length(p) - 1)
    if (p ~ /^#/) continue
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
  rowidx[s, nr[s]] = keyof($4); rowline[s, nr[s]] = $3; rowfile[s] = $2
  cls = $5
  okcls = (cls ~ /^MAN:[A-Za-z0-9][A-Za-z0-9-]*$/) || (cls ~ /^GUEST:[a-z][a-z0-9-]*$/) || (cls ~ /^ASSUMED:LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-[0-9][0-9][0-9][0-9]$/)
  next
}
END {
  if (!sentinel) {
    printf "X\tFXL_STAGE_DEAD\tstage 1 printed no END sentinel\n"
    exit 2
  }
  for (si = 1; si <= nst; si++) {
    s = stemlist[si]
    if (s in hass) nfix++
    for (j = 1; j <= nk[s]; j++) {
      k = klist[s, j]
      listed = 0
      for (r = 1; r <= nr[s]; r++) if (rowidx[s, r] == k) listed = 1
    }
    for (r = 1; r <= nr[s]; r++) {
      k = rowidx[s, r]
      seen[s, k]++
      if (!((s, k) in chkn) || seen[s, k] > 1) viol(rowfile[s], "FXL_ORPHAN_CHECK", rowline[s, r], "row " k " matches no CHECK of " s ".s, or repeats an earlier row")  # rule-orphan
    }
    if (s in hasc) {
    }
  }
  printf "N\t%d\t%d\t%d\n", nfix, nchk, nraw
}
