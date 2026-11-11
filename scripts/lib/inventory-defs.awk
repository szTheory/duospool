# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# inventory-defs.awk - stage 1 of the constants rule of scripts/inventory-check (plan 03-32; D-113, W7, CR-01). POSIX awk only.
# input : src/core/constants.def or src/core/model_params.def (one X-macro row per constant or parameter: DS_CONST(name, value, "LED-x") and DS_MODEL(name, allowed, default, "LED-x", status)); -v rel=PATH the name to print
# output: D<TAB>ledger id<TAB>line<TAB>rel   one per ledger id a row names; a row that names no quoted LED-AREA-NNNN id prints the id  -  so the rule can say so
#         DEND<TAB>N                          a last sentinel line with the number of rows read: a stage that printed none (an emptied copy of this file) or read no row is a dead stage, NO_SENTINEL in scripts/inventory-check, never a pass
# A row is a line that starts with DS_CONST or DS_MODEL and an open parenthesis after optional blanks (an indented row, or DS_CONST (...), is a row too: WR-03). The header comments of the two files mention those names after a comment star and are skipped.
# A line that is not a comment and holds one of the two names elsewhere than at the start of a row (a row folded into another line) prints the id  -  too, so the rule reports it instead of skipping it.
# The last line is  DEND<TAB>N  with N the number of rows read; scripts/inventory-check requires N of at least 1 for a def file that exists, so a file whose rows are no longer seen is a dead stage and not a pass.
/^[ \t]*DS_(CONST|MODEL)[ \t]*\(/ {
  line = $0; found = 0; rows++
  while (match(line, /"LED-[A-Z]+-[0-9]+"/)) {
    printf "D\t%s\t%d\t%s\n", substr(line, RSTART + 1, RLENGTH - 2), NR, rel
    found = 1
    line = substr(line, RSTART + RLENGTH)
  }
  if (!found) printf "D\t-\t%d\t%s\n", NR, rel
  next
}
/DS_(CONST|MODEL)/ && $0 !~ /^[ \t]*(\*|\/\*|\/\/)/ { printf "D\t-\t%d\t%s\n", NR, rel }
END { printf "DEND\t%d\n", rows + 0 }
