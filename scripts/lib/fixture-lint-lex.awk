# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# fixture-lint-lex.awk - stage 1 of scripts/fixture-lint (plan 03-37; D-125): a line reader over fixtures/src/NAME.s and NAME.checks (POSIX awk only).
# in : the files named on the command line (paths relative to the root, as fixture-lint passes them), .checks and .s files in any order.
# out (stdout, TAB separated, the file name in column 2, the 1-based line in column 3):
#   F    file                                  the file has at least one line
#   CHK  file line k word                      a CHECK_LIT, CHECK_GUEST or CHECK_OBSERVE use of check number k (the text before the first comma of the operand, not evaluated; word is the macro name)
#   INC  file line path                        a .include of path
#   BIN  file line operand                    a .incbin (any case) with its operand: stage 2 refuses every one (FXL_INCBIN)
#   INS  file line mnemonic word operand       a source line whose first word is letters and digits (any case; mnemonic is that word in lower case, word as written; stage 2 decides whether it is an instruction or a macro)
#   MAC  file line name                        a .macro definition of name in a source or in a fixtures/lib include (an .inc file is read for these only; ca65 macro names are case sensitive)
#   RAW  file line comment                     a source line whose comment holds the word RAW (a raw-byte line; the cite is judged by stage 2)
#   HDR  file line key value                   a .checks line of the form key=value (id=N, checks=N)
#   ROW  file line index class nfields col5    a .checks data row: TAB separated index, slug, class tag, meaning and an optional fifth column (col5, empty when absent; only the word observe is valid, plan 03-10)
#   END  n                                     the very last line: the sentinel. A stage 1 that died or was emptied does not print it, and stage 2 then reports FXL_STAGE_DEAD.
# A comment is everything after the first semicolon of a source line (a semicolon inside a quoted string is not supported: no fixture needs one). A label at the start of a line is dropped, also when the next word follows the colon without a blank (lda$20 style glue is not a label, a  name := value  assignment is not either).
# "END" in upper case cannot collide with another tag.
function trim(s) { sub(/^[ \t]+/, "", s); sub(/[ \t]+$/, "", s); return s }
FNR == 1 && FILENAME !~ /\.inc$/ { printf "F\t%s\t1\n", FILENAME }
{ nlines++ }
FILENAME ~ /\.checks$/ {
  line = $0
  if (line ~ /^[ \t]*#/ || line ~ /^[ \t]*$/) next
  if (line ~ /^[a-z_]+=/) {
    i = index(line, "=")
    printf "HDR\t%s\t%d\t%s\t%s\n", FILENAME, FNR, substr(line, 1, i - 1), trim(substr(line, i + 1))
    next
  }
  nf = split(line, f, "\t")
  printf "ROW\t%s\t%d\t%s\t%s\t%d\t%s\n", FILENAME, FNR, trim(f[1]), (nf >= 3) ? trim(f[3]) : "", nf, (nf >= 5) ? trim(f[5]) : ""
  next
}
FILENAME ~ /\.(s|inc)$/ {
  code = $0
  i = index(code, ";")
  if (i > 0) code = substr(code, 1, i - 1)
  if (tolower(code) ~ /^[ \t]*([A-Za-z_@][A-Za-z0-9_@]*:[ \t]*)?\.macro[ \t]+[A-Za-z_@][A-Za-z0-9_@]*/) {
    sub(/^[ \t]*([A-Za-z_@][A-Za-z0-9_@]*:[ \t]*)?[.][Mm][Aa][Cc][Rr][Oo][ \t]+/, "", code)
    sub(/[^A-Za-z0-9_@].*$/, "", code)
    printf "MAC\t%s\t%d\t%s\n", FILENAME, FNR, code
  }
}
FILENAME ~ /\.s$/ {
  code = $0
  com = ""
  i = index(code, ";")
  if (i > 0) { com = substr(code, i + 1); code = substr(code, 1, i - 1) }
  if (com ~ /(^|[^A-Za-z0-9_])RAW([^A-Za-z0-9_]|$)/) printf "RAW\t%s\t%d\t%s\n", FILENAME, FNR, trim(com)
  if (code ~ /^[A-Za-z_@][A-Za-z0-9_@]*:/ && code !~ /^[A-Za-z_@][A-Za-z0-9_@]*::?=/) { i = index(code, ":"); code = substr(code, i + 1) }  # a label, also one glued to the next word (lda below is still seen); a  name := value  assignment is not one
  else if (code ~ /^:([ \t]|$)/) code = substr(code, 2)
  code = trim(code)
  if (code == "") next
  i = index(code, " "); j = index(code, "\t")
  if (i == 0 || (j > 0 && j < i)) i = j
  if (i == 0) { word = code; rest = "" } else { word = substr(code, 1, i - 1); rest = trim(substr(code, i + 1)) }
  if (word ~ /^CHECK_(LIT|GUEST|OBSERVE)$/) { i = index(rest, ","); if (i > 0) rest = substr(rest, 1, i - 1); printf "CHK\t%s\t%d\t%s\t%s\n", FILENAME, FNR, trim(rest), word; next }
  if (tolower(word) == ".include") { printf "INC\t%s\t%d\t%s\n", FILENAME, FNR, rest; next }
  if (tolower(word) == ".incbin") { printf "BIN\t%s\t%d\t%s\n", FILENAME, FNR, rest; next }
  lw = tolower(word)  # ca65 takes an instruction in any case: LDA and lda are the same word
  if (lw ~ /^[a-z][a-z0-9]*$/) printf "INS\t%s\t%d\t%s\t%s\t%s\n", FILENAME, FNR, lw, word, rest
  next
}
END { printf "END\t%d\n", nlines + 0 }
