# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# record-rules.awk - stage 2: structure and hygiene rules for ONE run record (D-53, D-55, D-69). POSIX awk only, no regex intervals.
# input : toml-subset.awk records (TAB separated), including its END sentinel.
# env   : rec_fields = the 13 profile field names, in order, separated by blanks (the wrapper cuts them out of profile-rules.awk, so the
#                      list has one source)
# output: one TAB-separated record per line, tag in column 1:
#           V<TAB>CODE<TAB>line<TAB>detail     a violation
#           P<TAB>1                            the record was read (tallied by the wrapper; never a violation)
# codes : REC_TABLES     the set, the order or the keys of the tables differ from the accept record (record, core, profile, the field
#                        tables, withheld, content, power_on, limits, machine) or from the refusal record (record, core, refusal)
#         REC_FIELDS     the field tables are not exactly the 13 profile fields in profile order, each with status, value, bound_by
#         REC_PATH       a value holds a slash or a backslash (a path or an escape)
#         REC_POINTER    a value holds 0x followed by a hex digit (a pointer or an address)
#         REC_TIMESTAMP  a value holds a date (dddd-dd-dd) or a time (dd:dd)
#         plus the codes of toml-subset.awk, and NO_SENTINEL when stage 1 printed no END line.
# A record that carries a path, a pointer or a timestamp is host-dependent; the lint exists so that none can reach a published record.
BEGIN {
  FS = "\t"
  nfield = split(ENVIRON["rec_fields"], fl, " ")
  # the keys of every table that is not a field table, in order; a field table has status value bound_by
  keys["record"] = "format_version"
  keys["core"] = "build_id behaviour_epoch"
  keys["profile"] = "id rev sha256"
  keys["withheld"] = "ids"
  keys["content"] = "sha256 size mapper mapper_rule byte_order header_handling"
  keys["power_on"] = "policy policy_version seed_present seed"
  keys["limits"] = "size_cap"
  keys["machine"] = "time time_unit digest_algorithm digest"
  keys["refusal"] = "reason requested_id requested_rev"
  # the table sequences; the field tables count as one token
  seq_accept = "record core profile FIELDS withheld content power_on limits machine"
  seq_refusal = "record core refusal"
  fieldkeys = "status value bound_by"
  nt = 0; sentinel = 0; nv = 0
}
function viol(code, line, detail) { nv++; vbuf[nv] = sprintf("V\t%s\t%s\t%s", code, line, detail) }
$1 == "END" { sentinel = 1; next }
$1 == "ERR" { viol($2, $3, $4); next }     # syntax-stage errors pass straight through
{
  t = $1; k = $2; v = $4; ln = $5
  if (t == "") { viol("REC_TABLES", ln, "root key " k " outside any table"); next }
  if (!(t in seen)) { seen[t] = ln; nt++; order[nt] = t }
  kseq[t] = (t in kseq) ? kseq[t] " " k : k
  # hygiene: every value, all types (an array is its elements joined by a unit separator)
  if (index(v, "/") > 0 || index(v, "\\") > 0) viol("REC_PATH", ln, t "." k " holds a slash or a backslash")
  if (v ~ /0[xX][0-9a-fA-F]/) viol("REC_POINTER", ln, t "." k " holds 0x and a hex digit")   # rule-pointer
  if (v ~ /[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]/ || v ~ /[0-9][0-9]:[0-9][0-9]/) viol("REC_TIMESTAMP", ln, t "." k " holds a date or a time")
}
END {
  if (!sentinel) viol("NO_SENTINEL", 0, "stage 1 printed no END line")
  # the variant: a record with a refusal table is a refusal record, every other record is an accept record
  want = ("refusal" in seen) ? seq_refusal : seq_accept
  got = ""; last = ""; gotnames = ""; nfound = 0
  for (i = 1; i <= nt; i++) {
    t = order[i]
    if (substr(t, 1, 6) == "field_") {
      tok = "FIELDS"
      gotnames = (nfound++ ? gotnames " " : "") substr(t, 7)
      if (kseq[t] != fieldkeys) viol("REC_FIELDS", seen[t], t " keys are '" kseq[t] "', want '" fieldkeys "'")
    } else {
      tok = t
      if (t in keys && kseq[t] != keys[t]) viol("REC_TABLES", seen[t], t " keys are '" kseq[t] "', want '" keys[t] "'")
    }
    if (tok == "FIELDS" && last == "FIELDS") continue
    got = (got == "" ? "" : got " ") tok
    last = tok
  }
  if (got != want) viol("REC_TABLES", 0, "tables are '" got "', want '" want "'")
  if (want == seq_accept) {
    wantnames = ""
    for (i = 1; i <= nfield; i++) wantnames = (i > 1 ? wantnames " " : "") fl[i]
    if (nfield != 13) viol("REC_FIELDS", 0, "the profile definition lists " nfield " fields, not 13")
    if (gotnames != wantnames) viol("REC_FIELDS", 0, "field tables are '" gotnames "', want the " nfield " profile fields in order")
  }
  for (i = 1; i <= nv; i++) print vbuf[i]
  print "P\t1"
}
