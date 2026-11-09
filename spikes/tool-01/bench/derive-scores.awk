# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# derive-scores.awk - computes the three criteria that the protocol defines as formulas (C2, C3, C4) from results.tsv,
# exactly as docs/decisions/0001-language-build.protocol.md section (5) states them. C1, C5 and C6 are anchor
# judgements and are assigned by hand in scores.txt, each with a written reason.
# usage: awk -f spikes/tool-01/bench/derive-scores.awk spikes/tool-01/results/results.tsv
# stdout: '#' lines with the working, then the `score <opt> <C2|C3|C4> <x.y>` lines in the scorer input format.
# Arithmetic is integer only. Medians are read as text and held in units of 10 microseconds (0.19373 s = 19373), so no
# float comparison decides a band or a floor.
BEGIN {
  FS = "\t"
  # C3: tools that the protocol's baseline excludes from T, by the name the M10 list entry begins with.
  EXCL["Apple"] = "Apple clang is the system cc, in the protocol's shared baseline (the system cc used by the consumer)"
  EXCL["wasmtime"] = "wasmtime is used only to run the M14 wasm build (feeds C6); C3 counts tools needed for build, test, install, fuzz and sanitizer"
}

# value like "median=0.19373 mean=..." -> median in units of 1e-5 s
function median5(v,   m, a, parts, n) {
  if (!match(v, /median=[0-9]+\.[0-9][0-9][0-9][0-9][0-9]/)) return -1
  m = substr(v, RSTART + 7, RLENGTH - 7)
  n = split(m, parts, ".")
  return parts[1] * 100000 + parts[2]
}

function tenths(s10) { return int(s10 / 10) "." (s10 % 10) }

# slower score in tenths: floor(50 x faster / slower), integer division
function ratio_score(f, s,   x) {
  x = 50 * f
  return (x - (x % s)) / s
}

# split an M10 list value into top-level entries (paren depth 0, separator "; ") and count the T tools
function count_tools(o, v,   i, ch, depth, cur, n, ent, e, k, first, kept, nm) {
  depth = 0; cur = ""; n = 0
  for (i = 1; i <= length(v); i++) {
    ch = substr(v, i, 1)
    if (ch == "(") depth++
    if (ch == ")") depth--
    if (depth == 0 && substr(v, i, 2) == "; ") { ent[++n] = cur; cur = ""; i++; continue }
    cur = cur ch
  }
  ent[++n] = cur
  kept = 0
  for (e = 1; e <= n; e++) {
    if (ent[e] ~ /^baseline excluded/ || ent[e] ~ /^measurement only/) continue
    split(ent[e], w, " ")
    first = w[1]
    if (first == "Homebrew") first = "Homebrew"
    nm = ent[e]
    sub(/ \(.*/, "", nm)
    if (first in EXCL) { print "# C3 " o ": excluded " nm " (" EXCL[first] ")"; continue }
    kept++
    print "# C3 " o ": counted " nm
  }
  return kept
}

$1 == "M04" && $2 == "debug" { m04[$3] = median5($4) }
$1 == "M06" && $2 == "debug" { m06[$3] = median5($4) }
$1 == "M09" { g9[$3, $2] = $4 + 0 }
$1 == "M10" && $2 == "list" { m10[$3] = $4 }

# for one timing pair: sets sc["c"], sc["rust"] in tenths
function timing_pair(name, a, b,   f, s, fo, so, tied) {
  if (a <= b) { f = a; s = b; fo = "c"; so = "rust" } else { f = b; s = a; fo = "rust"; so = "c" }
  tied = (5 * s <= 6 * f) || (s < 200000)
  if (tied) { ts["c"] = 50; ts["rust"] = 50; print "# " name ": c " a " rust " b " (units 1e-5 s): tie (within 20% or both under 2 s), both 5.0"; return }
  ts[fo] = 50
  ts[so] = ratio_score(f, s)
  print "# " name ": c " a " rust " b " (units 1e-5 s): faster " fo " 5.0, slower " so " floor(50 x " f "/" s ") = " ts[so] " tenths"
}

END {
  if (m04["c"] < 0 || m04["rust"] < 0 || m06["c"] < 0 || m06["rust"] < 0) { print "DERIVE_INPUT_BAD missing M04 or M06 debug median"; exit 2 }
  timing_pair("C2 M04-debug", m04["c"], m04["rust"])
  a["c"] = ts["c"]; a["rust"] = ts["rust"]
  timing_pair("C2 M06-debug", m06["c"], m06["rust"])
  b["c"] = ts["c"]; b["rust"] = ts["rust"]
  c2["c"] = int((a["c"] + b["c"]) / 2)
  c2["rust"] = int((a["rust"] + b["rust"]) / 2)
  print "# C2 mean truncated to one decimal: c (" a["c"] "+" b["c"] ")/2 = " c2["c"] " ; rust (" a["rust"] "+" b["rust"] ")/2 = " c2["rust"]

  tc = count_tools("c", m10["c"])
  tr = count_tools("rust", m10["rust"])
  tmin = (tc < tr) ? tc : tr
  print "# C3: T c " tc ", T rust " tr ", T_min " tmin
  s3["c"] = 50 - (tc - tmin) * 10; if (s3["c"] < 0) s3["c"] = 0
  s3["rust"] = 50 - (tr - tmin) * 10; if (s3["rust"] < 0) s3["rust"] = 0
  print "# C3: no third-party library is linked into either product artifact (product Cargo.lock lists workspace members only), so no 2.0 cap"

  gc = g9["c", "ffi-glue"] + g9["c", "build"] + g9["c", "scripts"]
  gr = g9["rust", "ffi-glue"] + g9["rust", "build"] + g9["rust", "scripts"]
  print "# C4: G c " gc " (ffi-glue " g9["c", "ffi-glue"] " + build " g9["c", "build"] " + scripts " g9["c", "scripts"] "), G rust " gr " (ffi-glue " g9["rust", "ffi-glue"] " + build " g9["rust", "build"] " + scripts " g9["rust", "scripts"] ")"
  if (gc <= gr) { sm = gc; lg = gr; so = "rust"; fo = "c" } else { sm = gr; lg = gc; so = "c"; fo = "rust" }
  if (5 * lg <= 6 * sm) { s4["c"] = 50; s4["rust"] = 50; print "# C4: inside the 1.20 ratio band, both 5.0" }
  else {
    s4[fo] = 50
    s4[so] = ratio_score(sm, lg)
    print "# C4: outside the 1.20 band: smaller (" fo ") 5.0, larger (" so ") floor(50 x " sm "/" lg ") = " s4[so] " tenths"
  }

  print "score c C2 " tenths(c2["c"])
  print "score rust C2 " tenths(c2["rust"])
  print "score c C3 " tenths(s3["c"])
  print "score rust C3 " tenths(s3["rust"])
  print "score c C4 " tenths(s4["c"])
  print "score rust C4 " tenths(s4["rust"])
}
