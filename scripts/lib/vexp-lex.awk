# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# vexp-lex.awk - stage 1 of scripts/video-check: the line reader of one fixtures/src/NAME.vexp file (plan 04-07; D-171, Pitfall 11 of the research; POSIX awk only: no gensub, no asort, no regex intervals).
# in : ONE file on the command line.
# out: stdout, one record per line, TAB separated:
#        R<TAB>LINE<TAB>text          a row: every line that is not a comment (first character #) and not blank, with its 1-based line number and its text as it is (the TABs inside the text are the field separators of stage 2)
#        E<TAB>LINE<TAB>CODE<TAB>detail   a violation of the limits: VEXP_NO_SPDX (the 0BSD pair of lines in the first five), VEXP_LINE_TOO_LONG (over 400 characters), VEXP_NON_ASCII (a byte outside space to tilde and TAB), VEXP_TOO_MANY_ROWS (over 512 rows), VEXP_FILE_TOO_LARGE (over 16 KiB)
#        END<TAB>N                    the closing sentinel, N the number of rows: a dead or emptied stage 1 prints none and scripts/video-check says VIDEO_STAGE_DEAD (exit 2), never a pass
# The limits are the ones of D-171: 512 rows, 400 characters per line, 16 KiB per file, ASCII. Stage 2 (scripts/lib/vexp-rules.awk) judges the grammar.
# Each limit statement carries a tag  # rule-lex-NAME : the video-check stage and the selftest mutation-video-lex of scripts/check generate one control per limit in a scratch directory (the controls hold a long line, a non-ASCII byte, a file without the SPDX pair, 513 rows and 16 KiB, which no committed file may hold) and the selftest deletes each tagged line from a scratch copy (review fix H-04).
BEGIN {
  nrows = 0
  bytes = 0
  have_l = 0
  have_c = 0
}
{
  line = $0
  ln = NR
  bytes += length(line) + 1
  if (length(line) > 400) {
    printf "E\t%d\tVEXP_LINE_TOO_LONG\ta line of %d characters: the limit is 400\n", ln, length(line)  # rule-lex-line-too-long
  }
  if (line ~ /[^ -~\t]/) {
    printf "E\t%d\tVEXP_NON_ASCII\ta byte outside space to tilde and TAB\n", ln  # rule-lex-non-ascii
  }
  if (ln <= 5 && substr(line, 1, 1) == "#") {
    if (index(line, "SPDX-License-Identifier: 0BSD") > 0) have_l = 1
    if (index(line, "SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors") > 0) have_c = 1
  }
  if (substr(line, 1, 1) == "#" || line ~ /^[ \t]*$/) next
  nrows++
  if (nrows == 513) {
    printf "E\t%d\tVEXP_TOO_MANY_ROWS\tmore than 512 rows\n", ln  # rule-lex-too-many-rows
  }
  if (nrows <= 513) printf "R\t%d\t%s\n", ln, line
}
END {
  if (!have_l || !have_c) {
    printf "E\t1\tVEXP_NO_SPDX\tthe 0BSD SPDX pair is not in the first five lines\n"  # rule-lex-no-spdx
  }
  if (bytes > 16384) {
    printf "E\t1\tVEXP_FILE_TOO_LARGE\t%d bytes: the limit is 16384\n", bytes  # rule-lex-file-too-large
  }
  printf "END\t%d\n", nrows
}
