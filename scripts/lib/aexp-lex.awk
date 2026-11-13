# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# aexp-lex.awk - stage 1 of scripts/audio-check: the line reader of one fixtures/src/NAME.aexp file (plan 05-05; D-206, D-207, D-230, the vexp-lex.awk pattern; POSIX awk only: no gensub, no asort, no regex intervals).
# in : ONE file on the command line. The byte cap of D-230 (AEXP_MAX_BYTES = 32768 per file) is checked by scripts/audio-check BEFORE this stage runs (a larger file is a harness error, exit 2, fail closed), so this stage never sees a file over the cap.
# out: stdout, one record per line, TAB separated:
#        R<TAB>LINE<TAB>text          a row: every line that is not a comment (first character #) and not blank, with its 1-based line number and its text as it is (the TABs inside the text are the field separators of stage 2)
#        E<TAB>LINE<TAB>CODE<TAB>detail   a violation of the limits: AEXP_NO_SPDX (the 0BSD pair of lines in the first five), AEXP_LINE_TOO_LONG (over 400 characters), AEXP_NON_ASCII (a byte outside space to tilde and TAB), AEXP_TOO_MANY_ROWS (over 512 rows)
#        END<TAB>N                    the closing sentinel, N the number of rows: a dead or emptied stage 1 prints none and scripts/audio-check says AUDIO_STAGE_DEAD (exit 2), never a pass
# Each limit statement carries a tag  # rule-lex-NAME : the audio-check stage of scripts/check generates one control per limit in a scratch directory (no committed file may hold a long line, a non-ASCII byte or 513 rows) and the selftest mutation-audio-lex deletes each tagged line from a scratch copy.
BEGIN {
  nrows = 0
  have_l = 0
  have_c = 0
}
{
  line = $0
  ln = NR
  if (length(line) > 400) {
    printf "E\t%d\tAEXP_LINE_TOO_LONG\ta line of %d characters: the limit is 400\n", ln, length(line)  # rule-lex-line-too-long
  }
  if (line ~ /[^ -~\t]/) {
    printf "E\t%d\tAEXP_NON_ASCII\ta byte outside space to tilde and TAB\n", ln  # rule-lex-non-ascii
  }
  if (ln <= 5 && substr(line, 1, 1) == "#") {
    if (index(line, "SPDX-License-Identifier: 0BSD") > 0) have_l = 1
    if (index(line, "SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors") > 0) have_c = 1
  }
  if (substr(line, 1, 1) == "#" || line ~ /^[ \t]*$/) next
  nrows++
  if (nrows == 513) {
    printf "E\t%d\tAEXP_TOO_MANY_ROWS\tmore than 512 rows\n", ln  # rule-lex-too-many-rows
  }
  if (nrows <= 513) printf "R\t%d\t%s\n", ln, line
}
END {
  if (!have_l || !have_c) {
    printf "E\t1\tAEXP_NO_SPDX\tthe 0BSD SPDX pair is not in the first five lines\n"  # rule-lex-no-spdx
  }
  printf "END\t%d\n", nrows
}
