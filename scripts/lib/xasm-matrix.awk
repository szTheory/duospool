# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# xasm-matrix.awk - both awk stages of scripts/xasm-check (plan 03-08; D-129, CR-01). POSIX awk only. The stage is chosen with -v stage=1 or -v stage=2.
#
# stage 1 (-v stage=1 [-v asm=ca65|wla]): input is tests/oracle/huc6280-opcodes.tsv. It checks the shape of every data row and writes, to stdout:
#   V<TAB>XASM_TSV_SHAPE<TAB>line<TAB>detail          a malformed row (rule-shape)
#   S<TAB>text                                          one line of assembler source, in the order of the rows: a slot marker (.align 8, $FF for ca65, .ORG for WLA), the instruction, a label line for a relative branch
#   R<TAB>idx<TAB>opcode<TAB>mnemonic<TAB>mode<TAB>hex  one per defined row: the 0-based slot, the opcode, the mode and the expected bytes: the opcode followed by fixed placeholder operand bytes
#   END<TAB>defined<TAB>undefined                       the very last line, the sentinel (CR-01): a stage 1 that died or was emptied does not print it
# One row is one assembler line in neutral syntax with the size hint written out (z: for zero page, a: for absolute, < inside parentheses and for BBR and BBS, a label for a relative target); only the hint
# differs between assemblers (the WLA form is chosen by the value, not by a prefix). UNDEF rows are skipped (I11). The placeholders are fixed: zero page $20, absolute $1234, immediate $11, TAM and TMA mask $01,
# TST immediate $80 or $82, block transfer words $1234 $2345 $0456; a relative branch targets the byte after itself, so its offset byte is 00.
#
# stage 2 (-v stage=2 [-v asm=ca65|wla]): the inputs are the stage-1 file and then a file holding the assembled bytes as one line of hex digits. Each row owns an 8-byte slot padded with FF, so one wrong length cannot shift the others and extra bytes in a slot show.
#   X<TAB>XASM_STAGE_DEAD<TAB>detail                    when the stage-1 sentinel is missing; exit status 2
#   V<TAB>XASM_MISMATCH<TAB>opcode<TAB>detail           the bytes in the slot differ from the expected bytes (rule-compare), or (ca65) the image has the wrong size (rule-size)
#   N<TAB>rows<TAB>mismatches                           always printed once when the stage ran
# The statement that emits a code ends in a  # rule-NAME  tag, so that a selftest can delete it and show its control go red (mutation-xasm-compare).
function modeinit(   i, n, a) {
  n = split("IMPLIED:1: ACC:1: IMM:2:11 MASK:2:01 ZP:2:20 ZPX:2:20 ZPY:2:20 ABS:3:3412 ABSX:3:3412 ABSY:3:3412 IND:2:20 INDX:2:20 INDY:2:20 ABSIND:3:3412 ABSINDX:3:3412 REL:2:00 ZPREL:3:2000 BLK:7:341245235604 IMMZP:3:8020 IMMZPX:3:8020 IMMABS:4:820020 IMMABSX:4:820020", ml, " ")
  for (i = 1; i <= n; i++) {
    split(ml[i], a, ":")
    mlen[a[1]] = a[2] + 0
    mops[a[1]] = a[3]
  }
}
# the instruction text of one row; m is the lower-case mnemonic, lab the label of a relative target
function ca65src(m, mode, lab) {
  if (mode == "IMPLIED") return m
  if (mode == "ACC") return m " a"
  if (mode == "IMM") return m " #$11"
  if (mode == "MASK") return m " #$01"
  if (mode == "ZP") return m " z:$20"
  if (mode == "ZPX") return m " z:$20,x"
  if (mode == "ZPY") return m " z:$20,y"
  if (mode == "ABS") return m " a:$1234"
  if (mode == "ABSX") return m " a:$1234,x"
  if (mode == "ABSY") return m " a:$1234,y"
  if (mode == "IND") return m " (<$20)"
  if (mode == "INDX") return m " (<$20,x)"
  if (mode == "INDY") return m " (<$20),y"
  if (mode == "ABSIND") return m " ($1234)"
  if (mode == "ABSINDX") return m " ($1234,x)"
  if (mode == "REL") return m " " lab
  if (mode == "ZPREL") return m " <$20," lab
  if (mode == "BLK") return m " $1234,$2345,$0456"
  if (mode == "IMMZP") return m " #$80,z:$20"
  if (mode == "IMMZPX") return m " #$80,z:$20,x"
  if (mode == "IMMABS") return m " #$82,a:$2000"
  if (mode == "IMMABSX") return m " #$82,a:$2000,x"
  return ""
}
function wlasrc(m, mode, lab) {
  if (mode == "IMPLIED") return m
  if (mode == "ACC") return (m == "inc") ? "ina" : (m == "dec") ? "dea" : m " a"  # WLA-DX HuC6280 spells the accumulator INC and DEC as ina and dea ("inc a" is an operand named a)
  if (mode == "IMM") return m " #$11"
  if (mode == "MASK") return m " #$01"
  if (mode == "ZP") return m " $20"
  if (mode == "ZPX") return m " $20,x"
  if (mode == "ZPY") return m " $20,y"
  if (mode == "ABS") return m " $1234"
  if (mode == "ABSX") return m " $1234,x"
  if (mode == "ABSY") return m " $1234,y"
  if (mode == "IND") return m " ($20)"
  if (mode == "INDX") return m " ($20,x)"
  if (mode == "INDY") return m " ($20),y"
  if (mode == "ABSIND") return m " ($1234)"
  if (mode == "ABSINDX") return m " ($1234,x)"
  if (mode == "REL") return m " " lab
  if (mode == "ZPREL") return m " $20," lab
  if (mode == "BLK") return m " $1234,$2345,$0456"
  if (mode == "IMMZP") return m " #$80,$20"
  if (mode == "IMMZPX") return m " #$80,$20,x"
  if (mode == "IMMABS") return m " #$82,$2000"
  if (mode == "IMMABSX") return m " #$82,$2000,x"
  return ""
}
function viol1(ln, detail) { printf "V\tXASM_TSV_SHAPE\t%s\t%s\n", ln, detail }
function viol2(op, detail) { printf "V\tXASM_MISMATCH\t%s\t%s\n", op, detail }
BEGIN {
  FS = "\t"
  if (stage != 1 && stage != 2) { print "X\tXASM_STAGE_DEAD\tno stage chosen"; exit 2 }
  if (asm == "") asm = "ca65"
  modeinit()
  nrow = 0; ndef = 0; nund = 0; sentinel = 0; nmis = 0; hex = ""
}
# ---- stage 1 ----
stage == 1 && /^#/ { next }
stage == 1 && /^[ \t]*$/ { next }
stage == 1 {
  bad = ""
  if (NF != 7) bad = "row has " NF " fields, wanted 7"
  else if ($1 !~ /^[0-9A-F][0-9A-F]$/) bad = "opcode " $1 " is not two upper-case hex digits"
  else if ($1 in seen) bad = "opcode " $1 " appears twice"
  else if ($2 !~ /^[A-Z][A-Z0-9]*$/) bad = "mnemonic " $2 " is not upper case letters and digits"
  else if ($7 !~ /^[HS]8-[0-9]+(\+[HS]8-[0-9]+)*$/) bad = "cite " $7 " is not a +-joined list of H8-n or S8-n pages"
  else if ($2 == "UNDEF") {
    if ($3 != "-" || $4 != "-" || $5 != "-" || $6 != "-") bad = "an UNDEF row must carry - in the mode, bytes, cycles and flags columns"
  } else if (!($3 in mlen)) bad = "mode " $3 " is not a known mode"
  else if ($4 !~ /^[1-7]$/ || ($4 + 0) != mlen[$3]) bad = "bytes " $4 " do not match mode " $3 " (" mlen[$3] ")"
  else if (!(($5 ~ /^[0-9]+$/ && $3 != "BLK") || ($5 == "17+6x" && $3 == "BLK"))) bad = "cycles " $5 " is not a count (BLK rows say 17+6x)"
  else if (!($6 == "RESTORED" || $6 == "unlisted" || $6 ~ /^[N70-][V60-][01][1-][01-][01-][Z-][C01-]$/)) bad = "flags " $6 " is not an N V T B D I Z C string, RESTORED or unlisted"
  if (bad != "") { viol1(NR, bad); seen[$1] = 1; next }  # rule-shape
  seen[$1] = 1
  if ($2 == "UNDEF") { nund++; next }
  m = tolower($2)
  lab = "x" ndef
  line = (asm == "wla") ? wlasrc(m, $3, lab) : ca65src(m, $3, lab)
  if (asm == "wla") printf "S\t.ORG $%04X\n", 8 * ndef
  else printf "S\t        .align 8, $FF\n"
  printf "S\t        %s\n", line
  if ($3 == "REL" || $3 == "ZPREL") printf "S\t%s:\n", lab
  printf "R\t%d\t%s\t%s\t%s\t%s%s\n", ndef, $1, $2, $3, $1, mops[$3]
  ndef++
  next
}
stage == 1 { next }
# ---- stage 2 ----
stage == 2 && FNR == NR {
  if ($1 == "END") { sentinel = 1; next }
  if ($1 == "R") { nrow++; ridx[nrow] = $2 + 0; rop[nrow] = $3; rmn[nrow] = $4; rmode[nrow] = $5; rexp[nrow] = $6 }
  next
}
stage == 2 { hex = hex $0; next }
END {
  if (stage == 1) {
    if (ndef + nund == 0) viol1(0, "the table has no data row")  # rule-shape
    printf "END\t%d\t%d\n", ndef, nund
    exit 0
  }
  if (stage != 2) exit 2
  if (!sentinel) {
    printf "X\tXASM_STAGE_DEAD\tstage 1 printed no END sentinel\n"
    exit 2
  }
  for (i = 1; i <= nrow; i++) {
    want = rexp[i]
    if (i < nrow) while (length(want) < 16) want = want "FF"
    got = toupper(substr(hex, 16 * ridx[i] + 1, length(want)))
    if (got != want) { nmis++; viol2(rop[i], rmn[i] " " rmode[i] " expected " want " got " (got == "" ? "<none>" : got) " (" asm ")") }  # rule-compare
  }
  if (nrow > 0 && asm == "ca65" && length(hex) != 16 * (ridx[nrow]) + length(rexp[nrow])) { nmis++; viol2("--", "the image holds " length(hex) / 2 " bytes, wanted " (8 * ridx[nrow] + length(rexp[nrow]) / 2)) }  # rule-size
  printf "N\t%d\t%d\n", nrow, nmis
}
