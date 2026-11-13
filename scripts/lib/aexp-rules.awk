# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# aexp-rules.awk - stage 2 of scripts/audio-check: the grammar of the .aexp files, the class ceilings, the disputed points, the arithmetic lint, the rule map, the kill, vacuity and hash rules, the silence pairing, the toy registry and the inventory link (plan 05-05; D-206..D-210, D-225, ER-15..ER-20,
# F-72, F-73, PR-13, PR-15; POSIX awk only, integer arithmetic below 2^31, no printf %d of a larger value).
# in : one file made by scripts/audio-check: the facts, then for each .aexp file an F row and the rows of stage 1. Rows are TAB separated, the first field names the kind:
#        C label | L id | KC kind ceiling form | DM marker ledger | RA 0|1 | RL clause source status need line | DB file line detail (a malformed data line) | A 0|1 | X case reason | XB line detail | K toy case | P toy case | Z toy file | I 0|1 | AC case kind scene req
#        F file | R line kind fields... | E line code detail | END n
# out: V<TAB>FILE<TAB>CODE<TAB>LINE<TAB>detail per violation, then S<TAB>files<TAB>scenes<TAB>groups<TAB>rows<TAB>exempt<TAB>clauses and the closing RULES<TAB>n (a dead stage 2 prints no RULES row: AUDIO_STAGE_DEAD).
# Grammar (five row kinds, TAB separated, keys closed and in a fixed order; scene names [a-z][a-z0-9_]*, group ids [a-z][a-zA-Z0-9_]* for the side suffixes _L and _R of 05-04):
#   aexp    sample_mclk=N                                              the header, exactly once per file (D-225)
#   scene   NAME case=ID role=literal|golden|expect_red [twin=T] [expect_red=GID,...]   an expect_red scene is named T_twin, names the literal scene T and lists groups of its own (D-211, D-225)
#   group   SCENE GID form=F kind=K class=C kills=AF..                 F from tests/audio/KINDCLASS; C is MAN:<P8-n+...>, GUEST:program, ASSUMED:<LED-id+...> or DEF:<LED-id+...> (ER-19 definitions)
#   row     SCENE GID SELECTOR LITERAL                                 selector items anchor, reg, val, nth, ev, from, to, side, stream; literal items from the closed key list below
#   golden  SCENE pcm_sha256=HEX psg_sha256=HEX                        the two stream digests of a golden scene (ER-20: E0 change detectors, never a verdict)
# Lint (ER-18, rule-aexp-arith; E0 consistency of the expectation file, never an oracle, it writes no value): period rows d = k x f; run_grid rows g = 2 x nf; rows with step, f2 and flf: step = 6 x f2 x flf; rows with f, w, shift (0, 2 or 4), k and d: F' = (f + 4096 + w x 2^s - 16 x 2^s) mod 4096
#   (0 read as 4096) and d = k x F'; lfo_dynamic rows with f, w1, w2 and set: set = {F'(w1), F'(w2)} at shift 0; count rows n = ceil(m / 192) and the window to=m; and the P8-10 sum S = (15 - MAL) + (15 - (al >> 1)) + (15 - LAL) from r1, al, r5 and the side: a silent row whose inputs leave its side audible
#   (chon not 0, no R1 or R5 byte $00, S at most 14) and a live, period or cycle_order row whose inputs silence it (chon 0, a byte $00, S at least 15) contradict themselves.
# Classes: a group's class must be a subset of its kind's ceiling (page-set rule: a MAN label set inside the ceiling set; GUEST where the ceiling is GUEST or MAN; ASSUMED always; DEF only where the ceiling is DEF), each MAN label with an img cite, each ASSUMED or DEF label a ledger id
#   (rule-aexp-class-lint, rule-aexp-class-label); a MAN live, period or cycle_order row needs S = 0 or S = 4 (ER-16, rule-aexp-live-class); a MAN silent row needs chon=0 under P8-4, or S >= 16 or an R1/R5 byte $00 under P8-10 (ER-15, D-208, rule-aexp-silent-class).
# Disputed points (ER-15): a MAN group whose row carries a DISPUTED marker (an explicit mark=MARKER item, or found in the row: f=0, flf=0, nf=0 as nfc=0, an item reg=RA..RF, an R0 value with select 6 or 7) or whose S is 15 without a byte $00 is AUDIO_DISPUTED_POINT (rule-audio-disputed-mark, rule-audio-disputed-sum).
# Each statement that reports a code carries a tag  # rule-NAME ; scripts/check deletes the tagged lines of a scratch copy and requires the matching mini-root of tests/audio (or a generated control) to turn red: rule-audio-hash-only, rule-audio-group-vacuous, rule-aexp-class-lint (the three
# D-207 mutations), rule-aexp-class-label, rule-aexp-live-class, rule-aexp-silent-class, rule-aexp-arith, rule-audio-disputed-mark, rule-audio-disputed-sum, rule-audio-rule-no-row, rule-audio-silence-unpaired, rule-audio-group-unkilled, rule-audio-group-af0, rule-audio-scene-case,
# rule-audio-scene-sweep, rule-audio-toy-no-literal, rule-audio-exempt-format, rule-format, rule-unknown-key, rule-data-format and rule-stage1-limits; rule-stage1-row reports an input row stage 1 never prints and has no file control (the RULES sentinel and AUDIO_STAGE_DEAD guard the stage boundary).
function violf(file, code, ln, detail) {
  printf "V\t%s\t%s\t%s\t%s\n", file, code, ln, detail
  nviol++
}
function isname(s, cap) {
  return (length(s) >= 1 && length(s) < cap && s ~ /^[a-z][a-z0-9_]*$/)
}
function isgid(s) {
  return (length(s) >= 1 && length(s) < 32 && s ~ /^[a-z][a-zA-Z0-9_]*$/)
}
# num(s): a decimal (at most 9 digits) or $ and hex digits (at most 7), as a number; -1 when s is neither (every value below 2^31)
function num(s,    v, i, c) {
  if (s ~ /^[0-9]+$/ && length(s) <= 9) return s + 0
  if (s ~ /^\$[0-9A-Fa-f]+$/ && length(s) <= 8) {
    v = 0
    for (i = 2; i <= length(s); i++) {
      c = index("0123456789abcdef", tolower(substr(s, i, 1))) - 1
      v = v * 16 + c
    }
    return v
  }
  return -1
}
# fprime(f, w, s): the modulated select-0 value (f + 4096 + w x 2^s - 16 x 2^s) mod 4096 with 0 read as 4096 (D-184, LED-AUD-0011)
function fprime(f, w, s,    p, v) {
  p = (s == 0) ? 1 : ((s == 2) ? 4 : 16)
  v = (f + 4096 + w * p - 16 * p) % 4096
  return (v == 0) ? 4096 : v
}
# splitkv(text, K, V, file, ln, what): the items of a selector or a literal (separated by semicolons) into K[1..n] and V[1..n]; the count, or -1 after a format violation
function splitkv(text, K, V, file, ln, what,    n, a, i, p) {
  n = split(text, a, ";")
  for (i = 1; i <= n; i++) {
    p = index(a[i], "=")
    if (p < 2 || p == length(a[i])) {
      violf(file, "AEXP_FORMAT", ln, what ": an item is KEY=VALUE")  # rule-format
      return -1
    }
    K[i] = substr(a[i], 1, p - 1)
    V[i] = substr(a[i], p + 1)
  }
  return n
}
function haslabel(set, lab) {
  return index("+" set "+", "+" lab "+") > 0
}
BEGIN {
  FS = "\t"
  # the closed allowlist of GUEST models (ER-10: GUEST is for the guest's own data and write schedule, ER-21)
  guest_ok["program"] = 1
  # the closed form-by-fault table (D-209): the faults that can change what a group of that form reads. AF0 perturbs a literal of the group itself; AF1..AF7 act on the PCM frames; AF8 drops and AF9 relabels an event record, which also moves the anchors of a window.
  fa["EV"] = " AF0 AF8 AF9 "
  fa["ROWS"] = " AF0 AF1 AF2 AF3 AF4 AF5 AF6 AF7 AF8 AF9 "
  fa["REL"] = " AF0 AF1 AF2 AF3 AF4 AF5 AF6 AF7 AF8 AF9 "
  fa["STRUCT"] = " AF0 AF2 AF4 AF8 "
  # per-kind exclusions: AF5 zeroes the window, which cannot flip a row that requires every sample to be 0
  fx["silent"] = " AF5 "
  # the kinds whose rows show a live side (a null synthesiser fails each): the partners of AUDIO_SILENCE_UNPAIRED (D-208)
  split("live period cycle_order two_level lfo_static lfo_dynamic dda_order", lk, " ")
  for (i = 1; i in lk; i++) livek[lk[i]] = 1
  # the kinds of the ER-16 live-row rule (a MAN row needs S = 0 or S = 4) and of the lint's audible check
  aud3["live"] = 1; aud3["period"] = 1; aud3["cycle_order"] = 1
  # the case kinds of the audio domain: a toy of src/core/psg.c must kill one of these (AUDIO_TOY_NO_LITERAL, F-72, PR-15)
  split("audio audio_fault_sweep aud_noninterference psg_dev_join snapshot_restore_audio aud_poweron_policy_diff psg_fold pcm_cadence", ak, " ")
  for (i = 1; i in ak; i++) audiokind[ak[i]] = 1
  watched["src/core/psg.c"] = 1
  # the closed keys: selector items and literal items (the judged keys of the runner kinds and the lint inputs; plan check 3 info 5)
  split("anchor reg val nth ev from to side stream", sk, " ")
  for (i = 1; i in sk; i++) selkey[sk[i]] = 1
  split("f k d nf g f2 flf step w w1 w2 shift set m n r1 al r5 side chon p0 p1 words minrun reg val vals chan effect wave_idx post sample_mclk frame_bytes version format mark peak", lkk, " ")
  for (i = 1; i in lkk; i++) litkey[lkk[i]] = 1
  nrec = 0; nsent = 0; nfiles = 0; ns = 0; ng = 0; nr = 0; nviol = 0; nrl = 0; nexempt = 0
  toy_active = 0; inv_active = 0; rules_active = 0
}
$1 == "A" { toy_active = ($2 == "1"); next }
$1 == "I" { inv_active = ($2 == "1"); next }
$1 == "RA" { rules_active = ($2 == "1"); next }
$1 == "C" { cite_img[$2] = 1; next }
$1 == "L" { ledger_id[$2] = 1; next }
$1 == "KC" { kc_class[$2] = $3; kc_form[$2] = $4; next }
$1 == "DM" { marker[$2] = $3; next }
$1 == "RL" { nrl++; rl_id[nrl] = $2; rl_src[nrl] = $3; rl_st[nrl] = $4; rl_need[nrl] = $5; rl_ln[nrl] = $6; next }
$1 == "DB" {
  violf($2, "AEXP_FORMAT", $3, $4)  # rule-data-format
  next
}
$1 == "AC" { ac_kind[$2] = $3; ac_scene[$2] = $4; ac_req[$2] = $5; ac_list[++nac] = $2; next }
$1 == "K" { kill_case[$3] = 1; toy_kill[$2 SUBSEP $3] = 1; if (!($2 in toy_seen)) { toy_seen[$2] = 1; toy_list[++ntoy] = $2 }; next }
$1 == "P" { if (!($2 in toy_seen)) { toy_seen[$2] = 1; toy_list[++ntoy] = $2 }; next }
$1 == "Z" { toy_file[$2] = $3; if (!($2 in toy_seen)) { toy_seen[$2] = 1; toy_list[++ntoy] = $2 }; next }
$1 == "X" { exempt[$2] = $3; nexempt++; next }
$1 == "XB" {
  violf("tests/conformance/AUDIO_TOY_EXEMPT", "AUDIO_EXEMPT_FORMAT", $2, $3)  # rule-audio-exempt-format
  next
}
$1 == "F" { curfile = $2; nfiles++; file_list[nfiles] = $2; hdr_n[curfile] = 0; next }
$1 == "END" { nsent++; next }
$1 == "E" {
  violf(curfile, $3, $2, $4)  # rule-stage1-limits
  next
}
$1 == "R" {
  nrec++
  ln = $2
  kind = $3
  na = NF - 3
  badf = 0
  for (i = 4; i <= NF; i++) {
    if ($i == "" || index($i, " ") > 0) badf = 1
  }
  if (badf) {
    violf(curfile, "AEXP_FORMAT", ln, "an empty field or a space inside a field")  # rule-format
    next
  }
  if (kind == "aexp") {
    if (na != 1 || $4 !~ /^sample_mclk=[1-9][0-9]*$/) {
      violf(curfile, "AEXP_FORMAT", ln, "the header row is: aexp sample_mclk=N with N >= 1")  # rule-format
    } else {
      hdr_n[curfile]++
    }
  } else if (kind == "scene") {
    if (na < 3 || na > 5 || !isname($4, 32)) {
      violf(curfile, "AEXP_FORMAT", ln, "a scene row is: scene NAME case=ID role=literal|golden|expect_red [twin=NAME] [expect_red=GID,...]")  # rule-format
    } else if (substr($5, 1, 5) != "case=" || substr($6, 1, 5) != "role=" || (na >= 4 && substr($7, 1, 5) != "twin=") || (na == 5 && substr($8, 1, 11) != "expect_red=")) {
      violf(curfile, "AEXP_UNKNOWN_KEY", ln, "a scene row has the keys case= role= and the optional twin= expect_red=, in that order")  # rule-unknown-key
    } else if (!isname(substr($5, 6), 72) || ($6 != "role=literal" && $6 != "role=golden" && $6 != "role=expect_red")) {
      violf(curfile, "AEXP_FORMAT", ln, "a scene case is [a-z][a-z0-9_]* and a role is literal, golden or expect_red")  # rule-format
    } else if (substr($5, 6) in case_seen || (curfile SUBSEP $4) in scene_known) {
      violf(curfile, "AEXP_FORMAT", ln, "two scene rows name the case " substr($5, 6) " or the scene " $4)  # rule-format
    } else {
      ns++
      sc_name[ns] = $4; sc_case[ns] = substr($5, 6); sc_role[ns] = substr($6, 6); sc_file[ns] = curfile; sc_ln[ns] = ln
      sc_twin[ns] = (na >= 4) ? substr($7, 6) : ""
      sc_red[ns] = (na == 5) ? substr($8, 12) : ""
      case_seen[sc_case[ns]] = 1
      scene_known[curfile SUBSEP $4] = ns
    }
  } else if (kind == "group") {
    if (na != 6 || !isname($4, 32) || !isgid($5)) {
      violf(curfile, "AEXP_FORMAT", ln, "a group row is: group SCENE GID form= kind= class= kills=")  # rule-format
    } else if (substr($6, 1, 5) != "form=" || substr($7, 1, 5) != "kind=" || substr($8, 1, 6) != "class=" || substr($9, 1, 6) != "kills=") {
      violf(curfile, "AEXP_UNKNOWN_KEY", ln, "a group row has the keys form= kind= class= kills=, in that order")  # rule-unknown-key
    } else if ((curfile SUBSEP $4 SUBSEP $5) in gidx) {
      violf(curfile, "AEXP_FORMAT", ln, "two group rows have the same scene and id")  # rule-format
    } else {
      ng++
      g_scene[ng] = $4; g_gid[ng] = $5; g_form[ng] = substr($6, 6); g_kind[ng] = substr($7, 6); g_class[ng] = substr($8, 7); g_kills[ng] = substr($9, 7)
      g_file[ng] = curfile; g_ln[ng] = ln
      gidx[curfile SUBSEP $4 SUBSEP $5] = ng
      grp_n[curfile SUBSEP $4]++
    }
  } else if (kind == "row") {
    if (na != 4 || !isname($4, 32) || !isgid($5)) {
      violf(curfile, "AEXP_FORMAT", ln, "a row row is: row SCENE GID SELECTOR LITERAL")  # rule-format
    } else {
      nr++
      r_scene[nr] = $4; r_gid[nr] = $5; r_sel[nr] = $6; r_lit[nr] = $7; r_file[nr] = curfile; r_ln[nr] = ln
    }
  } else if (kind == "golden") {
    if (na != 3 || !isname($4, 32)) {
      violf(curfile, "AEXP_FORMAT", ln, "a golden row is: golden SCENE pcm_sha256=HEX psg_sha256=HEX")  # rule-format
    } else if (substr($5, 1, 11) != "pcm_sha256=" || substr($6, 1, 11) != "psg_sha256=") {
      violf(curfile, "AEXP_UNKNOWN_KEY", ln, "a golden row has the keys pcm_sha256= and psg_sha256=, in that order")  # rule-unknown-key
    } else if (length($5) != 75 || substr($5, 12) !~ /^[0-9a-f]+$/ || length($6) != 75 || substr($6, 12) !~ /^[0-9a-f]+$/) {
      violf(curfile, "AEXP_FORMAT", ln, "a golden digest is 64 lowercase hexadecimal digits")  # rule-format
    } else {
      gold_n[curfile SUBSEP $4]++
      gold_nm[curfile SUBSEP $4] = $4
      gold_file[curfile SUBSEP $4] = curfile
      gold_ln[curfile SUBSEP $4] = ln
    }
  } else {
    violf(curfile, "AEXP_UNKNOWN_KEY", ln, "unknown row kind " kind ": a row is aexp, scene, group, row or golden")  # rule-unknown-key
  }
  next
}
{ violf("-", "AEXP_FORMAT", NR, "an unexpected input row of stage 1") }  # rule-stage1-row
END {
  if (nsent != nfiles) {
    print "aexp-rules: a file of stage 1 printed no closing END row (or more than one)" > "/dev/stderr"
    exit 3
  }
  for (i = 1; i <= nfiles; i++) {
    if (hdr_n[file_list[i]] != 1) {
      violf(file_list[i], "AEXP_FORMAT", 0, "the file has " hdr_n[file_list[i]] + 0 " header rows aexp sample_mclk=N: it needs exactly one (D-225)")  # rule-format
    }
  }
  # the groups: scene, kind, form, class, kills (D-208, D-209, ER-17)
  for (i = 1; i <= ng; i++) {
    gf = g_file[i]; gl = g_ln[i]; gk = g_kind[i]; gc = g_class[i]
    if (!((gf SUBSEP g_scene[i]) in scene_known)) {
      violf(gf, "AEXP_FORMAT", gl, "the group " g_gid[i] " names the scene " g_scene[i] ", which no scene row defines")  # rule-format
    }
    if (!(gk in kc_class)) {
      violf(gf, "AEXP_UNKNOWN_KEY", gl, "the kind " gk " is not in tests/audio/KINDCLASS")  # rule-unknown-key
      continue
    }
    if (g_form[i] != kc_form[gk]) {
      violf(gf, "AEXP_FORMAT", gl, "the kind " gk " takes form " kc_form[gk] ", not " g_form[i])  # rule-format
    }
    # the class labels: MAN pages with an img cite, the GUEST allowlist, ledger ids for ASSUMED and DEF (F-73d)
    p = index(gc, ":")
    ck = (p > 0) ? substr(gc, 1, p - 1) : gc
    cl = (p > 0) ? substr(gc, p + 1) : ""
    g_ck[i] = ck; g_cl[i] = cl
    labok = (cl != "" && cl !~ /^\+|\+$|\+\+/)
    nl = split(cl, labs, "+")
    for (j = 1; j <= nl && labok; j++) {
      if (ck == "MAN" && !(labs[j] in cite_img)) labok = 0
      if ((ck == "ASSUMED" || ck == "DEF") && !(labs[j] in ledger_id)) labok = 0
      if (ck == "GUEST" && (nl != 1 || !(labs[j] in guest_ok))) labok = 0
    }
    if (ck != "MAN" && ck != "GUEST" && ck != "ASSUMED" && ck != "DEF") labok = 0
    if (!labok) {
      violf(gf, "AEXP_CLASS_INFLATED", gl, "the class " gc " of the group " g_gid[i] " is not MAN:<pages with an img cite>, GUEST:program, ASSUMED:<ledger ids> or DEF:<ledger ids> (F-73d)")  # rule-aexp-class-label
    } else {
      # the ceiling of the kind (ER-17, D-208): a page-set rule
      ceil = kc_class[gk]
      cp = index(ceil, ":")
      cek = (cp > 0) ? substr(ceil, 1, cp - 1) : ceil
      cel = (cp > 0) ? substr(ceil, cp + 1) : ""
      inf = 0
      if (ck == "MAN") {
        if (cek != "MAN") inf = 1
        for (j = 1; j <= nl && !inf; j++) if (!haslabel(cel, labs[j])) inf = 1
      } else if (ck == "GUEST") {
        if (cek != "MAN" && !(cek == "GUEST" && cel == cl)) inf = 1
      } else if (ck == "DEF") {
        if (cek != "DEF") inf = 1
      }
      if (inf) {
        violf(gf, "AEXP_CLASS_INFLATED", gl, "the class " gc " of the group " g_gid[i] " exceeds the ceiling " ceil " of the kind " gk " in tests/audio/KINDCLASS (ER-17)")  # rule-aexp-class-lint
      }
    }
    # the kills (D-209)
    kl = g_kills[i]
    if (kl == "") {
      violf(gf, "INV_AUDIO_GROUP_UNKILLED", gl, "the group " g_gid[i] " declares no fault in kills=: no stream fault would show it live (D-209)")  # rule-audio-group-unkilled
      continue
    }
    if (kl !~ /^AF[0-9](,AF[0-9])*$/) {
      violf(gf, "AEXP_FORMAT", gl, "a kills list is faults AF0 to AF9 separated by commas")  # rule-format
      continue
    }
    nk = split(kl, kk, ",")
    split("", kseen)
    anyapp = 0
    dup = 0
    for (j = 1; j <= nk; j++) {
      if (kk[j] in kseen) dup = 1
      kseen[kk[j]] = 1
      if (index(fa[g_form[i]], " " kk[j] " ") > 0 && index(fx[gk], " " kk[j] " ") == 0) anyapp = 1
    }
    if (dup) {
      violf(gf, "AEXP_FORMAT", gl, "a kills list names a fault twice")  # rule-format
    }
    if (!anyapp) {
      violf(gf, "AUDIO_GROUP_VACUOUS", gl, "the group " g_gid[i] " (" g_form[i] ", " gk ") kills only faults that cannot apply to its form and kind (" kl "): no fault could flip it (D-209)")  # rule-audio-group-vacuous
    }
    if (kl == "AF0") {
      af0case = ""
      if ((gf SUBSEP g_scene[i]) in scene_known) af0case = sc_case[scene_known[gf SUBSEP g_scene[i]]]
      if (!toy_active || af0case == "" || (!(af0case in kill_case) && !(af0case in exempt))) {
        violf(gf, "INV_AUDIO_GROUP_UNKILLED", gl, "the group " g_gid[i] " declares only AF0, which shows the comparison live and no more: its case must be killed by a toy EXPECT or listed in tests/conformance/AUDIO_TOY_EXEMPT with a reason (D-209)")  # rule-audio-group-af0
      }
    }
  }
  # the rows: group, closed keys, the lint, the class rules of the row, the markers
  for (i = 1; i <= nr; i++) {
    rf = r_file[i]; rln = r_ln[i]
    key = rf SUBSEP r_scene[i] SUBSEP r_gid[i]
    if (!(key in gidx)) {
      violf(rf, "AEXP_FORMAT", rln, "the row names the group " r_gid[i] " of the scene " r_scene[i] ", which no group row defines")  # rule-format
      continue
    }
    gi = gidx[key]
    row_n[gi]++
    gk = g_kind[gi]
    split("", SK); split("", SV); split("", LK); split("", LV); split("", sv); split("", lv)
    nsk = splitkv(r_sel[i], SK, SV, rf, rln, "a selector")
    nlk = splitkv(r_lit[i], LK, LV, rf, rln, "a literal")
    if (nsk < 0 || nlk < 0) continue
    bad = 0
    for (j = 1; j <= nsk; j++) {
      if (!(SK[j] in selkey)) {
        violf(rf, "AEXP_UNKNOWN_KEY", rln, "the selector item " SK[j] " is not one of anchor reg val nth ev from to side stream")  # rule-unknown-key
        bad = 1
      }
      if (!(SK[j] in sv)) sv[SK[j]] = SV[j]
    }
    for (j = 1; j <= nlk; j++) {
      if (!(LK[j] in litkey)) {
        violf(rf, "AEXP_UNKNOWN_KEY", rln, "the literal key " LK[j] " is not in the closed list of the runner and the lint")  # rule-unknown-key
        bad = 1
      } else if (LK[j] in lv) {
        violf(rf, "AEXP_FORMAT", rln, "the literal key " LK[j] " appears twice")  # rule-format
        bad = 1
      }
      lv[LK[j]] = LV[j]
    }
    if (bad) continue
    # numeric inputs (every value below 2^31)
    split("", nv)
    for (kx in lv) {
      if (kx == "set" || kx == "words" || kx == "vals" || kx == "mark" || kx == "reg" || kx == "side") continue
      nv[kx] = num(lv[kx])
      if (nv[kx] < 0) {
        violf(rf, "AEXP_FORMAT", rln, "the literal value " kx "=" lv[kx] " is not a decimal of at most 9 digits or $ and at most 7 hex digits")  # rule-format
        bad = 1
      }
    }
    if (bad) continue
    side = ""
    if ("side" in sv) side = sv["side"]
    if ("side" in lv) side = lv["side"]
    if (side != "" && side != "L" && side != "R") {
      violf(rf, "AEXP_FORMAT", rln, "a side is L or R")  # rule-format
      continue
    }
    if (side != "") r_side[i] = side
    # markers given by the author (mark=M,...): every one a DISPUTED marker
    split("", rmk)
    if ("mark" in lv) {
      nm = split(lv["mark"], mk, ",")
      for (j = 1; j <= nm; j++) {
        if (!(mk[j] in marker)) {
          violf(rf, "AEXP_UNKNOWN_KEY", rln, "mark=" mk[j] " is not a marker of tests/audio/DISPUTED")  # rule-unknown-key
          bad = 1
        } else rmk[mk[j]] = 1
      }
    }
    if (bad) continue
    # the arithmetic lint (ER-18, D-206)
    if (gk == "period") {
      if (!("f" in nv) || !("k" in nv) || !("d" in nv)) {
        violf(rf, "AEXP_FORMAT", rln, "a period row carries f, k and d (the lint inputs, d = k x f)")  # rule-format
      } else if (!("w" in nv) && nv["d"] != nv["k"] * nv["f"]) {
        violf(rf, "AEXP_ARITH", rln, "d = " nv["d"] " is not k x f = " nv["k"] " x " nv["f"] " (ER-18: E0 consistency of the expectation file)")  # rule-aexp-arith
      }
    }
    if ("w" in nv && "shift" in nv && "f" in nv && "k" in nv && "d" in nv) {
      if (nv["shift"] != 0 && nv["shift"] != 2 && nv["shift"] != 4) {
        violf(rf, "AEXP_FORMAT", rln, "shift is 0, 2 or 4 (LF CTL 1, 2, 3)")  # rule-format
      } else if (nv["d"] != nv["k"] * fprime(nv["f"], nv["w"], nv["shift"])) {
        violf(rf, "AEXP_ARITH", rln, "d = " nv["d"] " is not k x F' = " nv["k"] " x " fprime(nv["f"], nv["w"], nv["shift"]) " (F' = (f + 4096 + (w << s) - (16 << s)) & 4095, 0 read as 4096)")  # rule-aexp-arith
      }
    }
    if (gk == "run_grid") {
      if (!("g" in nv) || !("nf" in nv)) {
        violf(rf, "AEXP_FORMAT", rln, "a run_grid row carries g and nf (g = 2 x nf)")  # rule-format
      } else if (nv["g"] != 2 * nv["nf"]) {
        violf(rf, "AEXP_ARITH", rln, "g = " nv["g"] " is not 2 x nf = 2 x " nv["nf"])  # rule-aexp-arith
      }
    }
    if ("step" in nv && "f2" in nv && "flf" in nv && nv["step"] != 6 * nv["f2"] * nv["flf"]) {
      violf(rf, "AEXP_ARITH", rln, "step = " nv["step"] " is not 6 x f2 x flf = 6 x " nv["f2"] " x " nv["flf"])  # rule-aexp-arith
    }
    if (gk == "lfo_dynamic") {
      if (!("set" in lv) || !("f" in nv) || !("w1" in nv) || !("w2" in nv) || lv["set"] !~ /^[0-9]+,[0-9]+$/) {
        violf(rf, "AEXP_FORMAT", rln, "an lfo_dynamic row carries f, w1, w2 and set=A,B")  # rule-format
      } else {
        split(lv["set"], st, ",")
        e1 = fprime(nv["f"], nv["w1"], 0); e2 = fprime(nv["f"], nv["w2"], 0)
        if (!((st[1] + 0 == e1 && st[2] + 0 == e2) || (st[1] + 0 == e2 && st[2] + 0 == e1))) {
          violf(rf, "AEXP_ARITH", rln, "set = " lv["set"] " is not {F'(w1), F'(w2)} = {" e1 ", " e2 "} at shift 0")  # rule-aexp-arith
        }
      }
    }
    if (gk == "count") {
      if (!("m" in nv) || !("n" in nv)) {
        violf(rf, "AEXP_FORMAT", rln, "a count row carries m and n (n = ceil(m / 192))")  # rule-format
      } else {
        cm = int(nv["m"] / 192) + ((nv["m"] % 192) != 0 ? 1 : 0)
        if (nv["n"] != cm || (sv["from"] != "0" || sv["to"] != lv["m"])) {
          violf(rf, "AEXP_ARITH", rln, "n = " nv["n"] " is not ceil(m / 192) = " cm ", or the window is not from=0;to=m")  # rule-aexp-arith
        }
      }
    }
    # the P8-10 sum of the row's side (LED-AUD-0008)
    hasS = 0; S = -1; byte0 = 0; off = 0
    if ("chon" in nv && nv["chon"] == 0) off = 1
    if (("r1" in nv && nv["r1"] == 0) || ("r5" in nv && nv["r5"] == 0)) byte0 = 1
    if ("r1" in nv && "al" in nv && "r5" in nv && side != "" && nv["r1"] <= 255 && nv["r5"] <= 255 && nv["al"] <= 31) {
      hasS = 1
      mal = (side == "L") ? int(nv["r1"] / 16) : nv["r1"] % 16
      lal = (side == "L") ? int(nv["r5"] / 16) : nv["r5"] % 16
      S = (15 - mal) + (15 - int(nv["al"] / 2)) + (15 - lal)
    }
    if (gk == "silent" && !off && !byte0 && hasS && S <= 14) {
      violf(rf, "AEXP_ARITH", rln, "a silent row whose inputs leave the side audible (chon not 0, no byte $00, S = " S ")")  # rule-aexp-arith
    }
    if ((gk in aud3) && (off || byte0 || (hasS && S >= 15))) {
      violf(rf, "AEXP_ARITH", rln, "a " gk " row whose inputs silence the side (chon 0, a byte $00 or S = " S " >= 15)")  # rule-aexp-arith
    }
    if (g_ck[gi] == "MAN") {
      if (gk in aud3) {
        if (!hasS || off || byte0 || (S != 0 && S != 4)) {
          violf(rf, "AEXP_CLASS_INFLATED", rln, "a MAN " gk " row needs r1, al, r5 and a side giving S = 0 or S = 4 (ER-16: audible rows at sums 5 to 14 are ASSUMED); S = " S)  # rule-aexp-live-class
        }
      }
      if (gk == "silent") {
        okb = 0
        if (off && haslabel(g_cl[gi], "P8-4")) okb = 1
        if ((byte0 || (hasS && S >= 16)) && haslabel(g_cl[gi], "P8-10")) okb = 1
        if (!okb) {
          violf(rf, "AEXP_CLASS_INFLATED", rln, "a MAN silent row needs chon=0 under P8-4, or S >= 16 or an R1/R5 byte $00 under P8-10 (ER-15, D-208); S = " S)  # rule-aexp-silent-class
        }
      }
      # the disputed points (ER-15)
      if (("f" in nv && nv["f"] == 0) && ("f=0" in marker)) rmk["f=0"] = 1
      if (("flf" in nv && nv["flf"] == 0) && ("flf=0" in marker)) rmk["flf=0"] = 1
      if (("nf" in nv && nv["nf"] == 0) && ("nfc=0" in marker)) rmk["nfc=0"] = 1
      for (j = 1; j <= nsk; j++) if (SK[j] == "reg" && ("reg=" SV[j]) in marker) rmk["reg=" SV[j]] = 1
      if ("reg" in lv && ("reg=" lv["reg"]) in marker) rmk["reg=" lv["reg"]] = 1
      for (j = 1; j < nsk; j++) if (SK[j] == "reg" && SV[j] == "R0" && SK[j + 1] == "val" && num(SV[j + 1]) >= 0 && (num(SV[j + 1]) % 8) >= 6 && ("sel=" (num(SV[j + 1]) % 8)) in marker) rmk["sel=" (num(SV[j + 1]) % 8)] = 1
      if ("reg" in lv && lv["reg"] == "R0" && "val" in nv && (nv["val"] % 8) >= 6 && ("sel=" (nv["val"] % 8)) in marker) rmk["sel=" (nv["val"] % 8)] = 1
      nmk = 0; mlist = ""
      for (m in rmk) { nmk++; mlist = mlist " " m }
      if (nmk > 0) {
        violf(rf, "AUDIO_DISPUTED_POINT", rln, "the MAN group " g_gid[gi] " has a row on a disputed point (" substr(mlist, 2) "): such rows are ASSUMED only (ER-15, tests/audio/DISPUTED)")  # rule-audio-disputed-mark
      }
      if (hasS && S == 15 && !byte0) {
        violf(rf, "AUDIO_DISPUTED_POINT", rln, "the MAN group " g_gid[gi] " has a row at the P8-10 sum 15, a disputed point (ER-15, LED-AUD-0008)")  # rule-audio-disputed-sum
      }
    }
    if (gk == "silent") {
      if (side == "") {
        violf(rf, "AEXP_FORMAT", rln, "a silent row names its side")  # rule-format
      } else {
        sil[++nsil] = i
      }
    }
    if ((gk in livek) && side != "") live_side[rf SUBSEP r_scene[i] SUBSEP side] = 1
  }
  # the silence pairing (D-208): a null synthesiser must fail the scene
  for (j = 1; j <= nsil; j++) {
    i = sil[j]
    if (!((r_file[i] SUBSEP r_scene[i] SUBSEP r_side[i]) in live_side)) {
      violf(r_file[i], "AUDIO_SILENCE_UNPAIRED", r_ln[i], "the silent row of " r_gid[i] " has no live row (live, period, cycle_order, two_level, lfo_static, lfo_dynamic or dda_order) on side " r_side[i] " in the scene " r_scene[i] ": a null synthesiser would pass it (D-208)")  # rule-audio-silence-unpaired
    }
  }
  for (i = 1; i <= ng; i++) {
    if (!(i in row_n)) {
      violf(g_file[i], "AEXP_FORMAT", g_ln[i], "the group " g_gid[i] " has no row")  # rule-format
    }
  }
  # the scene roles (D-209, D-211, D-225, ER-20)
  split("", nlit); split("", ngold)
  for (i = 1; i <= ns; i++) {
    nm = sc_name[i]; f = sc_file[i]
    if (sc_role[i] == "literal") nlit[f]++
    if (sc_role[i] == "golden") ngold[f]++
  }
  for (i = 1; i <= ns; i++) {
    nm = sc_name[i]; f = sc_file[i]
    if (sc_role[i] == "expect_red") {
      tw = sc_twin[i]
      if (tw == "" || sc_red[i] == "" || nm != tw "_twin" || !((f SUBSEP tw) in scene_known) || sc_role[scene_known[f SUBSEP tw]] != "literal") {
        violf(f, "AEXP_FORMAT", sc_ln[i], "an expect_red scene is named T_twin, names twin=T of a literal scene T of its file and lists expect_red=GID,... (D-211, D-225)")  # rule-format
      } else {
        nx = split(sc_red[i], rx, ",")
        for (j = 1; j <= nx; j++) {
          if (!isgid(rx[j]) || !((f SUBSEP nm SUBSEP rx[j]) in gidx)) {
            violf(f, "AEXP_FORMAT", sc_ln[i], "the expect_red group " rx[j] " is not a group of the scene " nm)  # rule-format
          }
        }
      }
    } else if (sc_twin[i] != "" || sc_red[i] != "") {
      violf(f, "AEXP_FORMAT", sc_ln[i], "only an expect_red scene carries twin= and expect_red=")  # rule-format
    }
    if (sc_role[i] != "golden" && !((f SUBSEP nm) in grp_n)) {
      violf(f, "AEXP_FORMAT", sc_ln[i], "the " sc_role[i] " scene " nm " has no group")  # rule-format
    }
    if (sc_role[i] == "golden") {
      if ((f SUBSEP nm) in grp_n) {
        violf(f, "AEXP_FORMAT", sc_ln[i], "the golden scene " nm " has groups: a golden scene pins two digests and judges nothing")  # rule-format
      }
      gcount = ((f SUBSEP nm) in gold_n) ? gold_n[f SUBSEP nm] : 0
      if (gcount != 1) {
        violf(f, "AEXP_FORMAT", sc_ln[i], "the golden scene " nm " has " gcount " golden rows: it needs exactly one")  # rule-format
      }
      if (nlit[f] + 0 == 0 || ngold[f] > 1) {
        violf(f, "INV_AUDIO_HASH_ONLY", sc_ln[i], "the golden scene " nm " has no literal scene of the same ROM, or the ROM has " ngold[f] " golden scenes: a stream digest is a change detector and never a verdict (ER-20, D-209: at most one golden per ROM)")  # rule-audio-hash-only
      }
    }
  }
  for (gk2 in gold_n) {
    if (!(gk2 in scene_known) || sc_role[scene_known[gk2]] != "golden") {
      violf(gold_file[gk2], "AEXP_FORMAT", gold_ln[gk2], "a golden row names the scene " gold_nm[gk2] ", which is not a golden scene of its file")  # rule-format
    }
  }
  # the inventory link (the gate M-04 lesson): only where the root carries the case inventory
  if (inv_active) {
    for (i = 1; i <= ns; i++) {
      c = sc_case[i]
      if (!(c in ac_kind) || ac_kind[c] != "audio" || ac_scene[c] != sc_name[i] || ac_req[c] != "1") {
        violf(sc_file[i], "AUDIO_SCENE_NO_CASE", sc_ln[i], "the scene " sc_name[i] " names the case " c ", which is not a REQUIRED case of kind audio whose expect names the scene " sc_name[i])  # rule-audio-scene-case
      }
      if (sc_role[i] == "literal") {
        nsw = 0
        for (j = 1; j <= nac; j++) {
          if (ac_kind[ac_list[j]] == "audio_fault_sweep" && ac_scene[ac_list[j]] == sc_name[i] && ac_req[ac_list[j]] == "1") nsw++
        }
        if (nsw != 1) {
          violf(sc_file[i], "AUDIO_SCENE_NO_SWEEP", sc_ln[i], "the literal scene " sc_name[i] " has " nsw " REQUIRED audio_fault_sweep cases: it needs exactly one, so every declared kill is shown on the captured streams (D-209)")  # rule-audio-scene-sweep
        }
      }
    }
  }
  # the toy rule (F-72, PR-15): a toy of src/core/psg.c kills a case of the audio domain
  if (toy_active) {
    for (t = 1; t <= ntoy; t++) {
      tn = toy_list[t]
      if (!(tn in toy_file) || !(toy_file[tn] in watched)) continue
      has = 0
      for (j = 1; j <= nac; j++) {
        if (((tn SUBSEP ac_list[j]) in toy_kill) && (ac_kind[ac_list[j]] in audiokind)) has = 1
      }
      if (!has) {
        violf("tests/controls/" tn "/EXPECT", "AUDIO_TOY_NO_LITERAL", 0, "the toy " tn " mutates " toy_file[tn] " and lists no case of an audio kind (F-72)")  # rule-audio-toy-no-literal
      }
    }
  }
  # the rule map (ER-17): every clause with status row is covered by a group of a listed kind with a kill other than AF0, or by a REQUIRED case
  if (rules_active) {
    for (i = 1; i <= nrl; i++) {
      if (rl_id[i] in rl_seen) {
        violf("tests/audio/RULES", "AEXP_FORMAT", rl_ln[i], "the clause " rl_id[i] " appears twice")  # rule-data-format
      }
      rl_seen[rl_id[i]] = 1
      if (rl_st[i] != "row") continue
      need = rl_need[i]
      if (substr(need, 1, 5) == "case:") {
        c = substr(need, 6)
        if (!inv_active || !(c in ac_req) || ac_req[c] != "1") {
          violf("tests/audio/RULES", "AUDIO_RULE_NO_ROW", rl_ln[i], "the clause " rl_id[i] " (" rl_src[i] ") names the case " c ", which is not a REQUIRED case (ER-17)")  # rule-audio-rule-no-row
        }
        continue
      }
      nn = split(need, nk2, ",")
      cov = 0
      for (j = 1; j <= nn; j++) {
        if (!(nk2[j] in kc_class)) {
          violf("tests/audio/RULES", "AEXP_FORMAT", rl_ln[i], "the clause " rl_id[i] " names the kind " nk2[j] ", which is not in tests/audio/KINDCLASS")  # rule-data-format
          continue
        }
        for (g = 1; g <= ng; g++) {
          if (g_kind[g] == nk2[j] && g_kills[g] != "" && g_kills[g] != "AF0") cov = 1
        }
      }
      if (!cov) {
        violf("tests/audio/RULES", "AUDIO_RULE_NO_ROW", rl_ln[i], "the clause " rl_id[i] " (" rl_src[i] ") has status row, but no group of the kinds " need " declares a kill other than AF0 (ER-17)")  # rule-audio-rule-no-row
      }
    }
  }
  printf "S\t%d\t%d\t%d\t%d\t%d\t%d\n", nfiles, ns, ng, nr, nexempt + 0, nrl + 0
  printf "RULES\t%d\n", nrec
}
