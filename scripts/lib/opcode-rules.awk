# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# opcode-rules.awk - both awk stages of scripts/opcode-check (plan 03-09; D-96, D-97, CR-01). POSIX awk only. The stage is chosen with -v stage=1 or -v stage=2.
#
# stage 1 (-v stage=1 -v kind=def|tsv): input is src/core/cpu_ops.def (kind def) or tests/oracle/huc6280-opcodes.tsv (kind tsv). It checks the shape of every row and writes, to stdout:
#   V<TAB>CODE<TAB>opcode-or-line<TAB>detail            a malformed row (OPC_SHAPE, rule-shape) or an opcode listed twice in one file (OPC_EXTRA_ROW, rule-dup)
#   R<TAB>kind<TAB>OP<TAB>mnemonic<TAB>mode<TAB>bytes<TAB>cycles   one per first row of an opcode, OP two upper-case hex digits; an UNDEF row has mode, bytes and cycles of - in the tsv and the values of the def
#   END<TAB>kind<TAB>rows                                the very last line, the sentinel (CR-01): a stage 1 that died or was emptied does not print it
# The def row is DS_OP(0xNN, MNEMONIC, MODE, bytes, cycles, ACCESS, "cite") on ONE line; the tsv row is opcode, mnemonic, mode, bytes, cycles, flags, cite separated by TAB, # lines are comments.
#
# stage 2 (-v stage=2 [-v partial=1]): the inputs are the stage-1 file of the def and then that of the tsv. It compares them both ways and writes:
#   X<TAB>OPC_STAGE_DEAD<TAB>detail                    when a stage-1 sentinel is missing; exit status 2
#   V<TAB>OPC_MISSING_ROW<TAB>opcode<TAB>detail         the tsv has an opcode the def has no row for (rule-missing)
#   V<TAB>OPC_EXTRA_ROW<TAB>opcode<TAB>detail            the def has an opcode the tsv has no row for (rule-extra)
#   V<TAB>OPC_FIELD<TAB>opcode<TAB>detail                mnemonic, mode, bytes or cycles differ (rule-field); an UNDEF row is compared by membership only (both sides mark the opcode undefined, I11)
#   V<TAB>OPC_UNDEF_COUNT<TAB>kind<TAB>detail            a file does not hold 234 defined and 22 undefined opcodes (rule-count)
#   N<TAB>defined<TAB>undefined<TAB>compared             always printed once when the stage ran
# --partial (task 1 of plan 03-09 only): only the opcodes present in the def are compared and the missing, extra and count rules are off.
# The statement that emits a code ends in a  # rule-NAME  tag, so that a selftest can delete it and show its control go red (mutation-opcode-oracle-field).
function trim(s) {
  sub(/^[ \t\r]+/, "", s)
  sub(/[ \t\r]+$/, "", s)
  return s
}
function shape(where, detail) {
  print "V\tOPC_SHAPE\t" where "\t" detail # rule-shape
}
function emit(op, mnem, mode, nbytes, cyc) {
  if (op in seen) {
    print "V\tOPC_EXTRA_ROW\t" op "\topcode " op " is listed more than once in the " kind " file" # rule-dup
    return
  }
  seen[op] = 1
  rows++
  print "R\t" kind "\t" op "\t" mnem "\t" mode "\t" nbytes "\t" cyc
}
# a block transfer's count is the 17 plus 6 per byte moved: the def holds the 17, the manual prints the formula
function norm_cycles(mode, c) {
  if (mode == "BLK" && c ~ /^[0-9]+$/) return c "+6x"
  return c
}
function stage1def(   line, n, f, i, op) {
  if ($0 !~ /^DS_OP\(/) return
  line = $0
  sub(/^DS_OP\(/, "", line)
  sub(/\)[ \t\r]*$/, "", line)
  n = split(line, f, ",")
  for (i = 1; i <= n; i++) f[i] = trim(f[i])
  if (n != 7 || f[1] !~ /^0x[0-9A-Fa-f][0-9A-Fa-f]$/ || f[2] !~ /^[A-Z][A-Z0-9]*$/ || f[3] !~ /^[A-Z]+$/ || f[4] !~ /^[0-9]$/ || f[5] !~ /^[0-9]+$/ || f[6] !~ /^[A-Z]+$/ || f[7] !~ /^"[^"]*"$/) {
    shape(NR, "a DS_OP row is not DS_OP(0xNN, MNEMONIC, MODE, bytes, cycles, ACCESS, \"cite\") on one line")
    return
  }
  op = toupper(substr(f[1], 3, 2))
  emit(op, f[2], f[3], f[4], f[5])
}
function stage1tsv(   n, f, op) {
  if ($0 ~ /^#/ || $0 ~ /^[ \t\r]*$/) return
  n = split($0, f, "\t")
  if (n != 7 || f[1] !~ /^[0-9A-Fa-f][0-9A-Fa-f]$/ || f[2] !~ /^[A-Z][A-Z0-9]*$/) {
    shape(NR, "a tsv row needs seven TAB separated columns and a two digit hex opcode")
    return
  }
  if (f[2] == "UNDEF") {
    if (f[3] != "-" || f[4] != "-" || f[5] != "-") {
      shape(NR, "an UNDEF row carries - in mode, bytes and cycles")
      return
    }
  } else if (f[3] !~ /^[A-Z]+$/ || f[4] !~ /^[0-9]$/ || (f[5] !~ /^[0-9]+$/ && f[5] != "17+6x")) {
    shape(NR, "a defined row needs a mode, one digit of bytes and a cycle count or 17+6x")
    return
  }
  op = toupper(f[1])
  emit(op, f[2], f[3], f[4], f[5])
}
BEGIN {
  rows = 0
  if (stage != 1 && stage != 2) {
    print "usage: awk -v stage=1|2 -f opcode-rules.awk" > "/dev/stderr"
    bad = 1
    exit 2
  }
}
stage == 1 && kind == "def" { stage1def(); next }
stage == 1 && kind == "tsv" { stage1tsv(); next }
stage == 2 && $1 == "V" { print; next }
stage == 2 && $1 == "R" {
  k = $2
  op = $3
  mn[k, op] = $4
  md[k, op] = $5
  nb[k, op] = $6
  cy[k, op] = $7
  has[k, op] = 1
  list[k] = list[k] " " op
  next
}
stage == 2 && $1 == "END" { sawend[$2] = 1; next }
END {
  if (bad) exit 2
  if (stage == 1) {
    print "END\t" kind "\t" rows
    exit 0
  }
  if (!sawend["def"] || !sawend["tsv"]) {
    print "X\tOPC_STAGE_DEAD\tstage 1 of the " (sawend["def"] ? "tsv" : "def") " file printed no END sentinel"
    exit 2
  }
  ndef = 0
  nund = 0
  ncmp = 0
  tn = split(list["tsv"], tl, " ")
  for (i = 1; i <= tn; i++) {
    op = tl[i]
    if (mn["tsv", op] == "UNDEF") nund++
    else ndef++
  }
  dn = split(list["def"], dl, " ")
  dund = 0
  ddef = 0
  for (i = 1; i <= dn; i++) {
    if (mn["def", dl[i]] == "UNDEF") dund++
    else ddef++
  }
  if (!partial) {
    for (i = 1; i <= tn; i++) {
      op = tl[i]
      if (!(("def", op) in has)) print "V\tOPC_MISSING_ROW\t" op "\tthe manual table has " mn["tsv", op] " here and cpu_ops.def has no row" # rule-missing
    }
    for (i = 1; i <= dn; i++) {
      op = dl[i]
      if (!(("tsv", op) in has)) print "V\tOPC_EXTRA_ROW\t" op "\tcpu_ops.def has " mn["def", op] " here and the manual table has no row" # rule-extra
    }
    if (ddef != 234 || dund != 22) print "V\tOPC_UNDEF_COUNT\tdef\tcpu_ops.def holds " ddef " defined and " dund " undefined opcodes, wanted 234 and 22" # rule-count
    if (ndef != 234 || nund != 22) print "V\tOPC_UNDEF_COUNT\ttsv\tthe manual table holds " ndef " defined and " nund " undefined opcodes, wanted 234 and 22" # rule-count
  }
  for (i = 1; i <= dn; i++) {
    op = dl[i]
    if (!(("tsv", op) in has)) continue
    ncmp++
    dm = mn["def", op]
    tm = mn["tsv", op]
    if (dm == "UNDEF" && tm == "UNDEF") continue
    dc = norm_cycles(md["def", op], cy["def", op])
    if (dm != tm || md["def", op] != md["tsv", op] || nb["def", op] != nb["tsv", op] || dc != cy["tsv", op]) print "V\tOPC_FIELD\t" op "\tcpu_ops.def has " dm " " md["def", op] " " nb["def", op] " " dc ", the manual table has " tm " " md["tsv", op] " " nb["tsv", op] " " cy["tsv", op] # rule-field
  }
  print "N\t" ddef "\t" dund "\t" ncmp
  exit 0
}
