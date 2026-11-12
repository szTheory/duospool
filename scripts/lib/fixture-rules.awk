# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# fixture-rules.awk - stage 2: schema rules for ONE fixture manifest entry fixtures/manifest/ID.toml (FIX-02, D-59, D-60).
# input : toml-subset.awk records (TAB separated), including its END sentinel.
# env   : fname = the file name without .toml (the entry's id must equal it)
# output: one TAB-separated record per line, tag in column 1:
#           V<TAB>CODE<TAB>line<TAB>detail     a violation
#           M<TAB>fname<TAB>size<TAB>sha256    the entry's declared size and hash (empty when absent), always printed once, so that the
#                                              wrapper can compare the entry with the used list even when the entry also has a schema fault
# If the stage-1 sentinel is missing, V NO_SENTINEL is emitted (with the violations that follow from the empty input): a dead stage 1 must not read as a pass.
# Codes (a code is one rule, so a control can show it red and a selftest mutation can remove it):
#   FIX_MISSING_FIELD            a required key is absent or empty
#   FIX_UNKNOWN_KEY              a key or table that the schema does not name
#   FIX_BAD_ENUM                 kind or redistribution outside its set, a value of the wrong type, a sha256 that is not 64 lowercase hex
#                                digits, a negative size, or a source path that is absolute or has a parent step
#   FIX_ID_FILENAME_MISMATCH     id differs from the file stem
#   FIX_BAD_LICENCE              licence is not one of 0BSD MIT Apache-2.0 BSD-2-Clause BSD-3-Clause ISC Zlib CC0-1.0 (the SPDX ids that
#                                docs/decisions/0007-copied-code-provenance.md allows)
#   FIX_BAD_VERDICT_SOURCE       verdict_source is a list of one or more forms joined by + (no blanks), each one of spec-literal:LED-AREA-NNNN,
#                                guest-computed, independent-script or manual-cite:PAGE (PAGE is H8-n, S8-n, BIOS-n, H7-n or H6-n, n decimal); the literal
#                                duospool-output (the emulator judging itself, FIX-03) is rejected as a form, in any list, as is any other value
#   FIX_AUTHORED_REDIST          kind = authored with a redistribution other than generated-not-committed (D-123): an authored ROM is built in the
#                                gate and never committed, so the only admitted value is the one that says so
# This check enforces structure, not truth: the bytes are judged by the hash comparison in scripts/fixture-check.
BEGIN {
  FS = "\t"
  fname = ENVIRON["fname"]
  nv = 0; sentinel = 0
  n = split("id kind source recipe toolchain size sha256 licence notices redistribution meaning verdict_source", fl, " ")
  for (i = 1; i <= n; i++) ftype[fl[i]] = "s"
  ftype["size"] = "i"
  na = split("0BSD MIT Apache-2.0 BSD-2-Clause BSD-3-Clause ISC Zlib CC0-1.0", al, " ")
  for (i = 1; i <= na; i++) okl[al[i]] = 1
}
function viol(code, line, detail) { nv++; vbuf[nv] = sprintf("V\t%s\t%s\t%s", code, line, detail) }
$1 == "END" { sentinel = 1; next }
$1 == "ERR" { viol($2, $3, $4); next }                 # syntax-stage errors pass straight through
{
  t = $1; k = $2
  if (t != "") { viol("FIX_UNKNOWN_KEY", $5, "table " t); next }
  if (!(k in ftype)) { viol("FIX_UNKNOWN_KEY", $5, k); next }
  if ($3 != ftype[k]) { viol("FIX_BAD_ENUM", $5, "type of " k); bad[k] = 1; next }
  val[k] = $4; ln[k] = $5
}
END {
  if (!sentinel) viol("NO_SENTINEL", 0, "")
  for (i = 1; i <= n; i++) if ((!(fl[i] in val) && !(fl[i] in bad)) || ((fl[i] in val) && ftype[fl[i]] == "s" && val[fl[i]] == "")) viol("FIX_MISSING_FIELD", 0, fl[i])  # rule-missing-field
  if ("id" in val && val["id"] != fname) viol("FIX_ID_FILENAME_MISMATCH", ln["id"], val["id"] " is not the file name " fname)
  v = val["kind"]
  if (v != "" && v != "synthetic" && v != "authored" && v != "exception") viol("FIX_BAD_ENUM", ln["kind"], "kind " v)
  v = val["redistribution"]
  if (v != "" && v != "generated-not-committed" && v != "committed") viol("FIX_BAD_ENUM", ln["redistribution"], "redistribution " v)
  v = val["licence"]
  if (v != "" && !(v in okl)) viol("FIX_BAD_LICENCE", ln["licence"], "licence " v)
  v = val["verdict_source"]
  if (v != "") {  # rule-verdict-source: every +-joined part must be an allowed form; an empty part (a leading, trailing or doubled +) is not one
    np = split(v, vp, "+")
    vbad = 0
    for (i = 1; i <= np; i++) {
      q = vp[i]
      if (q != "guest-computed" && q != "independent-script" && q !~ /^spec-literal:LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-[0-9][0-9][0-9][0-9]$/ && q !~ /^manual-cite:(H8|S8|BIOS|H7|H6)-[0-9][0-9]*$/) vbad = 1
    }
    if (vbad) viol("FIX_BAD_VERDICT_SOURCE", ln["verdict_source"], "verdict_source " v)
  }
  if (val["kind"] == "authored" && ("redistribution" in val) && val["redistribution"] != "generated-not-committed") viol("FIX_AUTHORED_REDIST", ln["redistribution"], "authored entry with redistribution " val["redistribution"])  # rule-authored-redist
  v = val["sha256"]
  if (v != "" && (length(v) != 64 || v !~ /^[0-9a-f]+$/)) viol("FIX_BAD_ENUM", ln["sha256"], "sha256 is not 64 lowercase hex digits")
  if (("size" in val) && val["size"] + 0 < 0) viol("FIX_BAD_ENUM", ln["size"], "size is negative")
  v = val["source"]
  if (v != "" && (v ~ /^\// || v ~ /(^|\/)\.\.(\/|$)/)) viol("FIX_BAD_ENUM", ln["source"], "source path is not relative")
  for (i = 1; i <= nv; i++) print vbuf[i]
  printf "M\t%s\t%s\t%s\n", fname, ("size" in val) ? val["size"] : "", val["sha256"]
}
