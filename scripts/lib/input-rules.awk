# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# input-rules.awk - stage 2 of the input script checker: the schema rules of one input script (plan 03-27; D-51, D-117, ER-5; POSIX awk only).
# Stage 1 is scripts/lib/toml-subset.awk, the only parser of hand-edited data; this file reads its records (TABLE KEY TYPE VALUE LINE, one per line, ERR and END rows are the caller's to judge) and judges the schema.
# An input script is a strict TOML subset: a table [script] with format = 1, time_unit = "master_clock" and events = N, then the tables [ev_0001] to [ev_NNNN] in that order, each with at (the machine time in master clocks, an unsigned decimal of at most 15 digits
# and no leading zero), port (1 to 5) and held (an array of button names in canonical bit order: i ii select run up right down left, the bit order of the CD BIOS mask, CDBIOS-p28). An event sets the absolute level of its port from time at on (D-116).
# in : the stage 1 records of ONE file on stdin or as the file operand
# out: stdout, one record per line:
#        V<TAB>CODE<TAB>LINE<TAB>detail           a violation (the caller prefixes the file name); when any V is printed no S or E row is to be used
#        S<TAB>N                                  the number of events of the script (once, last but the sentinel)
#        E<TAB>IDX<TAB>AT<TAB>PORT<TAB>MASK       one event, IDX from 1; AT is the decimal text of the script (already checked to be exact in an awk double), MASK the decimal button mask
#        RULES<TAB>N                              the closing sentinel, N the number of records read: a dead or skipped stage 2 prints none and the caller treats that as INP_STAGE_DEAD
# codes: INP_FORMAT (no [script] table, format not 1), INP_TIME_UNIT, INP_EVENTS_COUNT (events missing, not a plain decimal, or different from the number of [ev_NNNN] tables), INP_TOO_MANY (more than 1024 events), INP_TABLE_NAME (a table that is neither script
#        nor the next ev_NNNN, or a root key), INP_KEY_UNKNOWN, INP_KEY_MISSING, INP_AT_NOT_DECIMAL, INP_AT_TOO_LONG, INP_AT_LEADING_ZERO, INP_ORDER (at decreases, or an equal at whose port does not increase), INP_PORT_RANGE, INP_HELD_UNKNOWN, INP_HELD_ORDER (a name out of the canonical order or repeated)
# Opposite directions, a port with no pad and the size cap are judged by the caller and by the core (ds_input_push), which re-validates every event whatever this file says (D-117, T-03-76).
function viol(code, ln, detail) {
  printf "V\t%s\t%s\t%s\n", code, ln, detail
}
BEGIN {
  FS = "\t"
  nrec = 0
  ntab = 0
  nhelds = split("i ii select run up right down left", hname, " ")
  for (h = 1; h <= nhelds; h++) {
    hbit[hname[h]] = h
  }
}
$1 == "ERR" || $1 == "END" { next }
NF == 5 {
  nrec++
  t = $1
  k = $2
  id = t SUBSEP k
  if (!(t in seen_tab)) {
    seen_tab[t] = 1
    tabs[++ntab] = t
  }
  val[id] = $4
  typ[id] = $3
  lin[id] = $5
  keys[t] = keys[t] " " k
  nk[t]++
  next
}
{ viol("INP_FORMAT", NR, "an unexpected stage 1 row") }
function need_keys(t, want, nwant,    w, i, j, found, ks, nks) {
  # every key of the table is one of want and every key of want is present once
  nks = split(keys[t], ks, " ")
  for (i = 1; i <= nks; i++) {
    found = 0
    for (j = 1; j <= nwant; j++) {
      if (ks[i] == want[j]) found = 1
    }
    if (!found) viol("INP_KEY_UNKNOWN", lin[t SUBSEP ks[i]], "table [" t "] has the key " ks[i] ", which an input script does not use")
  }
  for (j = 1; j <= nwant; j++) {
    if (!((t SUBSEP want[j]) in val)) {
      viol("INP_KEY_MISSING", 0, "table [" t "] has no key " want[j])
      return 0
    }
  }
  return 1
}
END {
  events = 0
  n_script = 0
  nrec_total = nrec
  # root keys belong to no table
  if (nk[""] > 0) {
    viol("INP_TABLE_NAME", 0, "a key outside every table: an input script has the tables script and ev_NNNN only")
  }
  # the script table
  if (!("script" in seen_tab)) {
    viol("INP_FORMAT", 0, "no [script] table")
  } else {
    sw[1] = "format"; sw[2] = "time_unit"; sw[3] = "events"
    if (need_keys("script", sw, 3)) {
      if (typ["script" SUBSEP "format"] != "i" || val["script" SUBSEP "format"] != "1") {
        viol("INP_FORMAT", lin["script" SUBSEP "format"], "format must be the integer 1")
      }
      if (typ["script" SUBSEP "time_unit"] != "s" || val["script" SUBSEP "time_unit"] != "master_clock") {
        viol("INP_TIME_UNIT", lin["script" SUBSEP "time_unit"], "time_unit must be the string master_clock")
      }
      ev = val["script" SUBSEP "events"]
      if (typ["script" SUBSEP "events"] != "i" || ev !~ /^(0|[1-9][0-9]*)$/ || length(ev) > 6) {
        viol("INP_EVENTS_COUNT", lin["script" SUBSEP "events"], "events must be a plain decimal integer")
        events = -1
      } else {
        events = ev + 0
        if (events > 1024) viol("INP_TOO_MANY", lin["script" SUBSEP "events"], "more than 1024 events")
      }
    } else {
      events = -1
    }
  }
  # the event tables in order
  nev = 0
  for (ti = 1; ti <= ntab; ti++) {
    t = tabs[ti]
    if (t == "" || t == "script") continue
    nev++
    want_name = sprintf("ev_%04d", nev)
    if (t != want_name) {
      viol("INP_TABLE_NAME", 0, "table [" t "] where [" want_name "] was expected: the event tables are ev_0001, ev_0002, ... in order")
      continue
    }
    ew[1] = "at"; ew[2] = "port"; ew[3] = "held"
    if (!need_keys(t, ew, 3)) continue
    # at
    a = val[t SUBSEP "at"]
    ok_at = 1
    if (typ[t SUBSEP "at"] != "i" || a !~ /^[0-9]+$/) {
      viol("INP_AT_NOT_DECIMAL", lin[t SUBSEP "at"], "at must be an unsigned decimal integer")
      ok_at = 0
    } else if (length(a) > 15) {
      viol("INP_AT_TOO_LONG", lin[t SUBSEP "at"], "at has more than 15 digits")
      ok_at = 0
    } else if (length(a) > 1 && substr(a, 1, 1) == "0") {
      viol("INP_AT_LEADING_ZERO", lin[t SUBSEP "at"], "at has a leading zero")
      ok_at = 0
    }
    # port
    p = val[t SUBSEP "port"]
    ok_port = 1
    if (typ[t SUBSEP "port"] != "i" || p !~ /^[1-5]$/) {
      viol("INP_PORT_RANGE", lin[t SUBSEP "port"], "port must be an integer from 1 to 5")
      ok_port = 0
    }
    # held
    mask = 0
    ok_held = 1
    if (typ[t SUBSEP "held"] != "a") {
      viol("INP_HELD_UNKNOWN", lin[t SUBSEP "held"], "held must be an array of button names")
      ok_held = 0
    } else {
      nh = split(val[t SUBSEP "held"], hv, "\037")
      last = 0
      for (j = 1; j <= nh; j++) {
        if (!(hv[j] in hbit)) {
          viol("INP_HELD_UNKNOWN", lin[t SUBSEP "held"], "unknown button name " hv[j])
          ok_held = 0
          break
        }
        if (hbit[hv[j]] <= last) {
          viol("INP_HELD_ORDER", lin[t SUBSEP "held"], "button names are listed once each in the canonical bit order i ii select run up right down left")
          ok_held = 0
          break
        }
        last = hbit[hv[j]]
        mask += 2 ^ (last - 1)
      }
    }
    if (ok_at && ok_port) {
      # the order of events: at never decreases, and an equal at needs a strictly greater port than the event before (D-117)
      if (have_prev) {
        if (a + 0 < prev_at + 0 || (a + 0 == prev_at + 0 && p + 0 <= prev_port + 0)) {
          viol("INP_ORDER", lin[t SUBSEP "at"], "events are not in order: at never decreases and an equal at needs a greater port than the event before")
        }
      }
      have_prev = 1
      prev_at = a
      prev_port = p
    }
    if (ok_at && ok_port && ok_held) {
      out_n++
      out[out_n] = sprintf("E\t%d\t%s\t%s\t%d", nev, a, p, mask)
    }
  }
  if (events >= 0 && events != nev) {
    viol("INP_EVENTS_COUNT", lin["script" SUBSEP "events"], "events = " events " but the file holds " nev " event tables")
  }
  printf "S\t%d\n", nev
  for (i = 1; i <= out_n; i++) print out[i]
  printf "RULES\t%d\n", nrec_total
}
