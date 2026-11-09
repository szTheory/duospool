# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# toml-subset.awk - strict TOML subset reader (POSIX awk only; no gawk/mawk/BWK extensions).
# Stage 1 of the two-stage checker (D-18). Schema rules live in the stage-2 files.
# in : one file. out (stdout): flat records  TABLE<TAB>KEY<TAB>TYPE<TAB>VALUE<TAB>LINE
#                              or errors     ERR<TAB>CODE<TAB>LINE<TAB>detail
#                              and, as the very last line, the sentinel  END<TAB>NR
# TABLE is "" for root keys.  TYPE: s (string) i (integer) b (bool) a (array of strings; VALUE = elements joined by US=\037)
# The sentinel makes a crashed or skipped stage 1 detectable: stage 2 reports NO_SENTINEL when it is absent.
# "END" in upper case cannot collide with a table name, because tables match [a-z][a-z0-9_]*.
function err(code, detail) { printf "ERR\t%s\t%d\t%s\n", code, NR, detail }
# parse one quoted string starting at s[pos]; sets PV (content) and PE (index after closing quote); returns 1 ok, 0 bad
function qstr(s, pos,    i, c, out) {
  if (substr(s, pos, 1) != "\"") return 0
  out = ""
  for (i = pos + 1; i <= length(s); i++) {
    c = substr(s, i, 1)
    if (c == "\"") { PV = out; PE = i + 1; return 1 }
    if (c == "\\") return 0            # subset: no escapes at all
    if (c == "\t") return 0            # a TAB inside a value would break the TAB-separated record format
    out = out c
  }
  return 0
}
BEGIN { FS = "\n"; table = ""; seen_tab[""] = 1 }
{
  line = $0
  if (length(line) > 400) { err("LINE_TOO_LONG", ""); next }
  if (line ~ /[^ -~\t]/) { err("NON_ASCII_OR_CTRL", ""); next }   # CR, bytes > 126, other control chars
  if (line ~ /^[ \t]*(#.*)?$/) next
  if (line ~ /^\[[a-z][a-z0-9_]*\][ \t]*(#.*)?$/) {
    t = line; sub(/^\[/, "", t); sub(/\].*$/, "", t)
    if (t in seen_tab) err("DUP_TABLE", t)
    seen_tab[t] = 1; table = t; next
  }
  if (!match(line, /^[a-z][a-z0-9_]*[ \t]*=[ \t]*/)) { err("SYNTAX", "not key = value / [table] / comment"); next }
  key = substr(line, 1, RLENGTH); sub(/[ \t]*=[ \t]*$/, "", key)
  rest = substr(line, RLENGTH + 1)
  id = table SUBSEP key
  if (id in seen_key) { err("DUP_KEY", key); next }
  seen_key[id] = 1
  if (rest ~ /^"/) {
    if (!qstr(rest, 1)) { err("BAD_STRING", key); next }
    tail = substr(rest, PE); if (tail !~ /^[ \t]*(#.*)?$/) { err("TRAILING_GARBAGE", key); next }
    printf "%s\t%s\ts\t%s\t%d\n", table, key, PV, NR; next
  }
  if (rest ~ /^(true|false)[ \t]*(#.*)?$/) { v = rest; sub(/[ \t#].*$/, "", v); printf "%s\t%s\tb\t%s\t%d\n", table, key, v, NR; next }
  if (rest ~ /^-?[0-9]+[ \t]*(#.*)?$/)      { v = rest; sub(/[ \t#].*$/, "", v); printf "%s\t%s\ti\t%s\t%d\n", table, key, v, NR; next }
  if (rest ~ /^\[/) {
    p = 2; joined = ""; n = 0; ok = 1
    while (1) {
      while (substr(rest, p, 1) == " " || substr(rest, p, 1) == "\t") p++
      if (substr(rest, p, 1) == "]") { if (n > 0 && lastcomma) ok = 0; p++; break }   # no trailing comma
      if (!qstr(rest, p)) { ok = 0; break }
      joined = (n++ ? joined "\037" : "") PV; p = PE
      while (substr(rest, p, 1) == " " || substr(rest, p, 1) == "\t") p++
      lastcomma = 0
      if (substr(rest, p, 1) == ",") { p++; lastcomma = 1 }
      else if (substr(rest, p, 1) != "]") { ok = 0; break }
    }
    if (ok && substr(rest, p) !~ /^[ \t]*(#.*)?$/) ok = 0
    if (!ok) { err("BAD_ARRAY", key); next }
    printf "%s\t%s\ta\t%s\t%d\n", table, key, joined, NR; next
  }
  err("BAD_VALUE", key)   # inline tables, floats, dates, multiline strings, single quotes, bare words
}
END { printf "END\t%d\n", NR }
