# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# inventory-defs.awk - stage 1 of the constants rule of scripts/inventory-check (plan 03-32; D-113, W7, CR-01). POSIX awk only.
# input : src/core/constants.def or src/core/model_params.def (one X-macro row per constant or parameter: DS_CONST(name, value, "LED-x") and DS_MODEL(name, allowed, default, "LED-x", status)); -v rel=PATH the name to print
# output: D<TAB>ledger id<TAB>line<TAB>rel   one per ledger id a row names; a row that names no quoted LED-AREA-NNNN id prints the id  -  so the rule can say so
#         DEND                                a last sentinel line: a stage that printed none (an emptied copy of this file) is a dead stage, NO_SENTINEL in scripts/inventory-check, never a pass
# Only a line that starts with DS_CONST( or DS_MODEL( is a row; the header comments of the two files mention those names after a comment star and are skipped.
/^DS_(CONST|MODEL)\(/ {
  line = $0; found = 0
  while (match(line, /"LED-[A-Z]+-[0-9]+"/)) {
    printf "D\t%s\t%d\t%s\n", substr(line, RSTART + 1, RLENGTH - 2), NR, rel
    found = 1
    line = substr(line, RSTART + RLENGTH)
  }
  if (!found) printf "D\t-\t%d\t%s\n", NR, rel
}
END { print "DEND" }
