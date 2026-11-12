# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# vexp-rules.awk - stage 2 of scripts/video-check: the grammar of the .vexp files, the class lint, the role and kill rules and the toy cross-rules (plan 04-07; D-171, D-172, D-173, ER-10, ER-11, ER-12, F-4, F-6, T-04-24 to T-04-27; POSIX awk only).
# in : one file on the command line, made by scripts/video-check: the facts of the root, then for each .vexp file an F row and the rows of stage 1. Rows are TAB separated, the first field names the kind:
#        A<TAB>0|1               the toy cross-rules are active (the root carries tests/conformance/VIDEO_TOY_EXEMPT or tests/controls: a mini-root without either has no toy registry and only the .vexp rules run)
#        C<TAB>label             a manual page label of fixtures/lib/cites.txt that carries the flag img (a MAN: class names one of these)
#        L<TAB>id                a ledger entry id (an ASSUMED: class names one of these)
#        Y<TAB>case              a case of kind guest (a toy that mutates a video source may list one instead of a literal video case)
#        K<TAB>toy<TAB>case      a case named in column 1 of tests/controls/TOY/EXPECT with no second column: the toy must turn it guest_failure (a kill)
#        P<TAB>toy<TAB>case      a case named with a second column (pass, the F-4 column of plan 04-17): it stays pass under the toy, so it is not a kill and counts neither as toy coverage nor as a literal case of INV_TOY_NO_LITERAL
#        Z<TAB>toy<TAB>file      the file= line of tests/controls/TOY/RULE
#        X<TAB>case<TAB>reason   a line of VIDEO_TOY_EXEMPT: a scene case that no toy lists, with the reason
#        XB<TAB>line<TAB>detail  a malformed line of VIDEO_TOY_EXEMPT (VIDEO_EXEMPT_FORMAT)
#        F<TAB>file              the following rows belong to this .vexp file
#        R<TAB>line<TAB>kind<TAB>fields...   a row of stage 1 (the text split on TABs); E<TAB>line<TAB>code<TAB>detail  a violation of stage 1; END<TAB>n  the sentinel of one file
# out: stdout, V<TAB>FILE<TAB>CODE<TAB>LINE<TAB>detail  for each violation, then  S<TAB>files<TAB>scenes<TAB>groups<TAB>rows<TAB>exempt  and the closing  RULES<TAB>n  (the number of input rows): a dead or skipped stage 2 prints no RULES row and the caller says VIDEO_STAGE_DEAD (exit 2).
# Grammar (four row kinds, TAB separated, keys closed, the order of the keys fixed):
#   scene   NAME  case=CASE_ID  role=literal|struct|golden          a scene NAME may have one line per case, for example its literal case and its golden case
#   group   SCENE GID  form=ROWS|REL|STRUCT  class=MAN:<label>|GUEST:<model>|ASSUMED:<LED-id>  sym=order|asym|none  kills=F0,F5,...
#   row     SCENE GID  SELECTOR  LITERAL                             ROWS selector line=active+K;x=X0..X1 with literal index=LIST or value=LIST (LIST items V or COUNT*V: COUNT dots of the value V); STRUCT selector check=NAME with key=value items; REL selector rel=NAME;runs=A,B with key=value items
#   golden  SCENE sha256=HEX                                         one pinned stream digest of a golden scene
# Codes: VEXP_FORMAT (a shape error), VEXP_UNKNOWN_KEY (an unknown row kind, key, check, relation or selector), VEXP_CLASS (a class that is not MAN with an img cite, GUEST with an allowlisted model or ASSUMED with a ledger id, or a grid-dependent check under a class other than ASSUMED), VEXP_ASYM_CLASS (ER-12: a sym=asym group
#   with a class other than ASSUMED), INV_VIDEO_GROUP_UNKILLED (an empty kills list), VIDEO_GROUP_VACUOUS (a kills list none of whose faults can apply to the form of the group, by the closed form-by-fault table below), VIDEO_ROWS_VACUOUS (a ROWS literal with fewer than 3 distinct values or no transition, D-171), INV_VIDEO_HASH_ONLY (a golden scene
#   with no literal scene of the same NAME, ER-11), VIDEO_SCENE_UNTOYED (a scene case that no toy EXPECT lists and no VIDEO_TOY_EXEMPT line names), INV_TOY_NO_LITERAL (a toy that mutates src/core/vce.c, vdc.c, beam.c or vid.c and lists no literal-role video case and no guest-kind case), VIDEO_EXEMPT_FORMAT, and the codes of stage 1 (VEXP_NO_SPDX, VEXP_LINE_TOO_LONG,
#   VEXP_NON_ASCII, VEXP_TOO_MANY_ROWS, VEXP_FILE_TOO_LARGE). The INV_ codes are printed here verbatim (F-6); scripts/inventory-check is not edited.
# F0 note (plan-check info 7, amendment A7): F0 perturbs the first number of the first row of a group, so it proves only that the comparison is live. A group whose only kill is F0 (the border group of vce_pal) gets its liveness from a toy (vid-palette-mask) or from a null-renderer control, and says so in its .vexp comment.
# Each statement that reports a code carries a tag  # rule-NAME  on its own line; the selftests of scripts/check delete the tagged lines of a scratch copy to prove that the matching mini-root under tests/video turns red without them (D-172: the rules rule-video-hash-only, rule-video-group-vacuous and rule-vexp-class-lint, and one tag for each other rule).
function violf(file, code, ln, detail) {
  printf "V\t%s\t%s\t%s\t%s\n", file, code, ln, detail
  nviol++
}
function isname(s, cap) {
  return (length(s) >= 1 && length(s) < cap && s ~ /^[a-z][a-z0-9_]*$/)
}
# litcheck(file, ln, text, spec, what): the literal text is key=value items separated by semicolons, every value a decimal or a comma list of decimals, and the keys are exactly the comma-joined list spec, each once. Returns 1 when sound.
function litcheck(file, ln, text, spec, what,    n, a, i, p, k, v, nw, w, seen, ok) {
  ok = 1
  split("", seen)
  n = split(text, a, ";")
  for (i = 1; i <= n; i++) {
    p = index(a[i], "=")
    if (p < 2) {
      violf(file, "VEXP_FORMAT", ln, what " literal: an item is KEY=VALUE")
      return 0
    }
    k = substr(a[i], 1, p - 1)
    v = substr(a[i], p + 1)
    if (v !~ /^[0-9]+(,[0-9]+)*$/) {
      violf(file, "VEXP_FORMAT", ln, what " literal: the value of " k " is a decimal or a comma list of decimals")
      return 0
    }
    if (k in seen) {
      violf(file, "VEXP_FORMAT", ln, what " literal: the key " k " twice")
      return 0
    }
    seen[k] = 1
    if (index("," spec ",", "," k ",") == 0) {
      violf(file, "VEXP_UNKNOWN_KEY", ln, what " literal: the key " k " is not one of " spec)  # rule-unknown-key
      ok = 0
    }
  }
  nw = split(spec, w, ",")
  for (i = 1; i <= nw; i++) {
    if (!(w[i] in seen)) {
      violf(file, "VEXP_FORMAT", ln, what " literal: the key " w[i] " is missing")
      ok = 0
    }
  }
  return ok
}
# the class of group i against the registry (ER-10, T-04-27): MAN needs a cite label with the flag img, GUEST a model of the closed allowlist, ASSUMED a ledger id
function class_lint(i,    c, p, kind, lab) {
  c = g_class[i]
  p = index(c, ":")
  if (p == 0) {
    violf(g_file[i], "VEXP_CLASS", g_ln[i], "the class " c " is not MAN:<label>, GUEST:<model> or ASSUMED:<LED-id>")  # rule-vexp-class-lint
    return
  }
  kind = substr(c, 1, p - 1)
  lab = substr(c, p + 1)
  if (kind == "MAN") {
    if (!(lab in cite_img)) {
      violf(g_file[i], "VEXP_CLASS", g_ln[i], "MAN:" lab " names no manual page with an img row in fixtures/lib/cites.txt")  # rule-vexp-class-lint
    }
  } else if (kind == "GUEST") {
    if (!(lab in guest_ok)) {
      violf(g_file[i], "VEXP_CLASS", g_ln[i], "GUEST:" lab " is not in the closed allowlist of in-guest models of this file")  # rule-vexp-class-lint
    }
  } else if (kind == "ASSUMED") {
    if (!(lab in ledger_id)) {
      violf(g_file[i], "VEXP_CLASS", g_ln[i], "ASSUMED:" lab " names no ledger entry")  # rule-vexp-class-lint
    }
  } else {
    violf(g_file[i], "VEXP_CLASS", g_ln[i], "the class " c " is not MAN:<label>, GUEST:<model> or ASSUMED:<LED-id>")  # rule-vexp-class-lint
  }
}
BEGIN {
  FS = "\t"
  # the closed allowlist of GUEST models (ER-10: GUEST is for pure in-guest arithmetic over values the guest itself wrote). A later plan that needs another in-guest model adds it here with a one-line justification and lists this file.
  guest_ok["pal_bijection"] = 1   # the palette bijection P(i) = (91 * i + 17) & 511: the border the guest left is arithmetic over the 512 values it wrote (04-04, F-25)
  # the closed list of faults and the closed form-by-fault table: the faults that can change what a group of that form reads (F0 changes the literal of the group itself; F1 F2 F7 act on pixels and indices; F3 F4 F6 F8 on lines; F5 on the divider; F9 on an origin)
  fa["STRUCT"] = " F0 F3 F4 F5 F6 F8 F9 "
  fa["ROWS"] = " F0 F1 F2 F3 F4 F6 F7 F8 "
  fa["REL"] = " F0 F1 F2 F3 F4 F6 F7 F8 "
  # the closed list of STRUCT checks and the keys of the literal of each (more only by editing this file; the runner knows the same names)
  ck["count"] = "grid"
  ck["origins"] = "step"
  ck["frame_lines"] = "value,step"
  ck["records_per_frame"] = "value"
  ck["rec_bytes"] = "header,version,max_w"
  ck["values_below_512"] = "limit"
  ck["div_table"] = "div,crw,gap"
  ck["div_window"] = "div,at,crw,gap"
  ck["window"] = "crw,div,active_x,active_w,rec_bytes,clipped,index"
  ck["div_first"] = "div,prev,crw"
  ck["border_after_pal"] = "value"
  ck["line_dirty"] = "cr_bit,pal_bit"
  ck["display_lines"] = "vdw,vds,frame_lines"
  ck["width_set"] = "min_distinct"
  ck["display_flags"] = "vdw,vds,frame_lines,skip"
  ck["line_dirty_hv"] = "hv_bit"
  ck["cr_change"] = "div_a,crw,hvw,div_b,w_a,w_b,x_a,x_b,margin"
  # the checks that use the 1365-clock grid, the frame length, the window origin or the join of a write stamp to the grid: their class is ASSUMED (ER-10, the weakest class of the inputs wins)
  split("count origins frame_lines records_per_frame display_lines div_first line_dirty window display_flags line_dirty_hv cr_change", gl, " ")
  for (i = 1; i in gl; i++) {
    gridck[gl[i]] = 1
  }
  # the closed list of REL relations and the keys of the literal of each
  rk["scroll_x_shift"] = "dx"
  rk["scroll_y_shift"] = "dy"
  rk["palette_subst"] = "from,to"
  rk["blank_is_border"] = "index"
  rk["dot_width_scale"] = "lines"
  # the files whose toys must list a literal-role video case or a guest-kind case (INV_TOY_NO_LITERAL, amendment A5)
  watched["src/core/vce.c"] = 1
  watched["src/core/vdc.c"] = 1
  watched["src/core/beam.c"] = 1
  watched["src/core/vid.c"] = 1
  nrec = 0
  nsent = 0
  nfiles = 0
  ns = 0
  ng = 0
  nr = 0
  nviol = 0
  toy_active = 0
}
$1 == "A" { toy_active = ($2 == "1"); next }
$1 == "C" { cite_img[$2] = 1; next }
$1 == "L" { ledger_id[$2] = 1; next }
$1 == "Y" { guest_case[$2] = 1; next }
$1 == "K" { kill_case[$3] = 1; toy_kill[$2 SUBSEP $3] = 1; if (!($2 in toy_seen)) { toy_seen[$2] = 1; toy_list[++ntoy] = $2 }; next }
$1 == "P" { if (!($2 in toy_seen)) { toy_seen[$2] = 1; toy_list[++ntoy] = $2 }; next }
$1 == "Z" { toy_file[$2] = $3; if (!($2 in toy_seen)) { toy_seen[$2] = 1; toy_list[++ntoy] = $2 }; next }
$1 == "X" { exempt[$2] = $3; nexempt++; next }
$1 == "XB" {
  violf("tests/conformance/VIDEO_TOY_EXEMPT", "VIDEO_EXEMPT_FORMAT", $2, $3)  # rule-video-exempt-format
  next
}
$1 == "F" { curfile = $2; nfiles++; next }
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
    violf(curfile, "VEXP_FORMAT", ln, "an empty field or a space inside a field")  # rule-format
    next
  }
  if (kind == "scene") {
    if (na != 3 || !isname($4, 32)) {
      violf(curfile, "VEXP_FORMAT", ln, "a scene row is: scene NAME case=CASE_ID role=literal|struct|golden")  # rule-format
    } else if (substr($5, 1, 5) != "case=" || substr($6, 1, 5) != "role=") {
      violf(curfile, "VEXP_UNKNOWN_KEY", ln, "a scene row has the keys case= and role=, in that order")  # rule-unknown-key
    } else if (!isname(substr($5, 6), 72) || ($6 != "role=literal" && $6 != "role=struct" && $6 != "role=golden")) {
      violf(curfile, "VEXP_FORMAT", ln, "a scene case is [a-z][a-z0-9_]* and a role is literal, struct or golden")  # rule-format
    } else if (substr($5, 6) in case_seen) {
      violf(curfile, "VEXP_FORMAT", ln, "two scene rows name the case " substr($5, 6))  # rule-format
    } else {
      ns++
      sc_name[ns] = $4
      sc_case[ns] = substr($5, 6)
      sc_role[ns] = substr($6, 6)
      sc_file[ns] = curfile
      sc_ln[ns] = ln
      case_seen[substr($5, 6)] = 1
      scene_known[$4] = 1
      if (sc_role[ns] == "literal") has_lit[$4] = 1
      if (sc_role[ns] == "golden") has_gold[$4] = 1
      if (sc_role[ns] == "literal") lit_case[substr($5, 6)] = 1
    }
  } else if (kind == "group") {
    if (na != 6 || !isname($4, 32) || !isname($5, 32)) {
      violf(curfile, "VEXP_FORMAT", ln, "a group row is: group SCENE GID form= class= sym= kills=")  # rule-format
    } else if (substr($6, 1, 5) != "form=" || substr($7, 1, 6) != "class=" || substr($8, 1, 4) != "sym=" || substr($9, 1, 6) != "kills=") {
      violf(curfile, "VEXP_UNKNOWN_KEY", ln, "a group row has the keys form= class= sym= kills=, in that order")  # rule-unknown-key
    } else if (($6 != "form=ROWS" && $6 != "form=REL" && $6 != "form=STRUCT") || ($8 != "sym=order" && $8 != "sym=asym" && $8 != "sym=none")) {
      violf(curfile, "VEXP_FORMAT", ln, "a group form is ROWS, REL or STRUCT and a sym is order, asym or none")  # rule-format
    } else if (($4 SUBSEP $5) in gidx) {
      violf(curfile, "VEXP_FORMAT", ln, "two group rows have the same scene and id")  # rule-format
    } else {
      ng++
      g_scene[ng] = $4
      g_gid[ng] = $5
      g_form[ng] = substr($6, 6)
      g_class[ng] = substr($7, 7)
      g_sym[ng] = substr($8, 5)
      g_kills[ng] = substr($9, 7)
      g_file[ng] = curfile
      g_ln[ng] = ln
      gidx[$4 SUBSEP $5] = ng
      grp_n[$4]++
    }
  } else if (kind == "row") {
    if (na != 4 || !isname($4, 32) || !isname($5, 32)) {
      violf(curfile, "VEXP_FORMAT", ln, "a row row is: row SCENE GID SELECTOR LITERAL")  # rule-format
    } else {
      nr++
      r_scene[nr] = $4
      r_gid[nr] = $5
      r_sel[nr] = $6
      r_lit[nr] = $7
      r_file[nr] = curfile
      r_ln[nr] = ln
    }
  } else if (kind == "golden") {
    if (na != 2 || !isname($4, 32)) {
      violf(curfile, "VEXP_FORMAT", ln, "a golden row is: golden SCENE sha256=HEX")  # rule-format
    } else if (substr($5, 1, 7) != "sha256=") {
      violf(curfile, "VEXP_UNKNOWN_KEY", ln, "a golden row has the key sha256=")  # rule-unknown-key
    } else if (length($5) != 71 || substr($5, 8) !~ /^[0-9a-f]+$/) {
      violf(curfile, "VEXP_FORMAT", ln, "a golden digest is 64 lowercase hexadecimal digits")  # rule-format
    } else {
      gold_n[$4]++
      gold_file[$4] = curfile
      gold_ln[$4] = ln
    }
  } else {
    violf(curfile, "VEXP_UNKNOWN_KEY", ln, "unknown row kind " kind ": a row is scene, group, row or golden")  # rule-unknown-key
  }
  next
}
{ violf("-", "VEXP_FORMAT", NR, "an unexpected input row of stage 1") }  # rule-stage1-row
END {
  if (nsent != nfiles) {
    print "vexp-rules: a file of stage 1 printed no closing END row (or more than one)" > "/dev/stderr"
    exit 3
  }
  # the groups: scene, class, symmetry (ER-12), kills (D-172)
  for (i = 1; i <= ng; i++) {
    if (!(g_scene[i] in scene_known)) {
      violf(g_file[i], "VEXP_FORMAT", g_ln[i], "the group " g_gid[i] " names the scene " g_scene[i] ", which no scene row defines")  # rule-format
    }
    class_lint(i)
    if (g_sym[i] == "asym" && substr(g_class[i], 1, 8) != "ASSUMED:") {
      violf(g_file[i], "VEXP_ASYM_CLASS", g_ln[i], "the group " g_gid[i] " is sym=asym with the class " g_class[i] ": an asymmetric group is ASSUMED only (ER-12: the manual does not print pixel order, plane order, scroll direction, wrap or flips)")  # rule-vexp-asym-class
    }
    if (g_kills[i] == "") {
      violf(g_file[i], "INV_VIDEO_GROUP_UNKILLED", g_ln[i], "the group " g_gid[i] " declares no fault in kills=: no record fault would show it live (D-172)")  # rule-video-group-unkilled
    } else {
      nk = split(g_kills[i], kl, ",")
      okk = 1
      anyapp = 0
      for (j = 1; j <= nk; j++) {
        if (kl[j] !~ /^F[0-9]$/) okk = 0
        if (index(fa[g_form[i]], " " kl[j] " ") > 0) anyapp = 1
      }
      if (!okk) {
        violf(g_file[i], "VEXP_FORMAT", g_ln[i], "a kills list is faults F0 to F9 separated by commas")  # rule-format
      } else if (!anyapp) {
        violf(g_file[i], "VIDEO_GROUP_VACUOUS", g_ln[i], "the group " g_gid[i] " (" g_form[i] ") kills only faults that cannot apply to its form (" g_kills[i] "): no fault could flip it")  # rule-video-group-vacuous
      }
    }
  }
  # the rows: group, selector and literal of the form of the group
  for (i = 1; i <= nr; i++) {
    key = r_scene[i] SUBSEP r_gid[i]
    if (!(key in gidx)) {
      violf(r_file[i], "VEXP_FORMAT", r_ln[i], "the row names the group " r_gid[i] " of the scene " r_scene[i] ", which no group row defines")  # rule-format
      continue
    }
    gi = gidx[key]
    row_n[gi]++
    form = g_form[gi]
    sel = r_sel[i]
    lit = r_lit[i]
    if (form == "STRUCT") {
      if (sel !~ /^check=[a-z0-9_]+$/) {
        violf(r_file[i], "VEXP_UNKNOWN_KEY", r_ln[i], "a STRUCT row has the selector check=NAME")  # rule-unknown-key
      } else if (!(substr(sel, 7) in ck)) {
        violf(r_file[i], "VEXP_UNKNOWN_KEY", r_ln[i], "the STRUCT check " substr(sel, 7) " is not in the closed list of this file")  # rule-unknown-key
      } else {
        chk = substr(sel, 7)
        litcheck(r_file[i], r_ln[i], lit, ck[chk], "STRUCT " chk)
        if ((chk in gridck) && substr(g_class[gi], 1, 8) != "ASSUMED:") {
          violf(r_file[i], "VEXP_CLASS", r_ln[i], "the STRUCT check " chk " uses the 1365-clock grid, the frame length, the window origin or the write-to-line join: its group " g_gid[gi] " must be ASSUMED, not " g_class[gi] " (ER-10)")  # rule-vexp-class-lint
        }
      }
    } else if (form == "ROWS") {
      if (sel !~ /^line=active\+[0-9]+;x=[0-9]+\.\.[0-9]+$/) {
        violf(r_file[i], "VEXP_UNKNOWN_KEY", r_ln[i], "a ROWS row has the selector line=active+K;x=X0..X1")  # rule-unknown-key
      } else if (lit !~ /^(index|value)=[0-9]+(\*[0-9]+)?(,[0-9]+(\*[0-9]+)?)*$/) {
        violf(r_file[i], "VEXP_UNKNOWN_KEY", r_ln[i], "a ROWS row has the literal index=LIST or value=LIST, the items of the list V or COUNT*V")  # rule-unknown-key
      } else {
        xs = sel
        sub(/^.*;x=/, "", xs)
        split(xs, xr, "[.][.]")
        want = xr[2] - xr[1] + 1
        lv = lit
        sub(/^[a-z]+=/, "", lv)
        nit = split(lv, it, ",")
        total = 0
        ntr = 0
        split("", dseen)
        ndist = 0
        prev = ""
        for (j = 1; j <= nit; j++) {
          if (index(it[j], "*") > 0) {
            split(it[j], rv, "*")
            rep = rv[1] + 0
            val = rv[2] + 0
          } else {
            rep = 1
            val = it[j] + 0
          }
          total += rep
          if (!(val in dseen)) {
            dseen[val] = 1
            ndist++
          }
          if (j > 1 && val != prev) ntr++
          prev = val
        }
        if (xr[2] + 0 < xr[1] + 0 || total != want) {
          violf(r_file[i], "VEXP_FORMAT", r_ln[i], "a ROWS literal lists " total " values for the " want " dots of its x range")  # rule-format
        } else if (ndist < 3 || ntr < 1) {
          violf(r_file[i], "VIDEO_ROWS_VACUOUS", r_ln[i], "a ROWS literal has " ndist " distinct values and " ntr " transitions: at least 3 distinct values and one transition are needed, or an all-border or one-colour expectation would pass a null renderer (D-171)")  # rule-video-rows-vacuous
        }
      }
    } else {
      if (sel !~ /^rel=[a-z_]+;runs=[a-z][a-z0-9_]*,[a-z][a-z0-9_]*$/) {
        violf(r_file[i], "VEXP_UNKNOWN_KEY", r_ln[i], "a REL row has the selector rel=NAME;runs=A,B")  # rule-unknown-key
      } else {
        rel = sel
        sub(/;.*$/, "", rel)
        rel = substr(rel, 5)
        if (!(rel in rk)) {
          violf(r_file[i], "VEXP_UNKNOWN_KEY", r_ln[i], "the REL relation " rel " is not in the closed list (scroll_x_shift scroll_y_shift palette_subst blank_is_border dot_width_scale)")  # rule-unknown-key
        } else {
          litcheck(r_file[i], r_ln[i], lit, rk[rel], "REL " rel)
        }
      }
    }
  }
  for (i = 1; i <= ng; i++) {
    if (!(i in row_n)) {
      violf(g_file[i], "VEXP_FORMAT", g_ln[i], "the group " g_gid[i] " has no row")  # rule-format
    }
  }
  # the scene roles (D-172, ER-11)
  for (i = 1; i <= ns; i++) {
    nm = sc_name[i]
    if (sc_role[i] == "literal" && !(nm in grp_n)) {
      violf(sc_file[i], "VEXP_FORMAT", sc_ln[i], "the literal scene " nm " has no group")  # rule-format
    }
    if (sc_role[i] == "struct") {
      for (j = 1; j <= ng; j++) {
        if (g_scene[j] == nm && g_form[j] != "STRUCT") {
          violf(g_file[j], "VEXP_FORMAT", g_ln[j], "the group " g_gid[j] " of the struct-role scene " nm " is not a STRUCT group")  # rule-format
        }
      }
    }
    if (sc_role[i] == "golden") {
      if (!(nm in has_lit)) {
        violf(sc_file[i], "INV_VIDEO_HASH_ONLY", sc_ln[i], "the golden scene " nm " has no literal scene of the same name: a stream digest is a change detector and never a verdict (ER-11, D-172)")  # rule-video-hash-only
      }
      if (gold_n[nm] != 1) {
        violf(sc_file[i], "VEXP_FORMAT", sc_ln[i], "the golden scene " nm " has " gold_n[nm] + 0 " golden rows: it needs exactly one")  # rule-format
      }
    }
  }
  for (nm in gold_n) {
    if (!(nm in has_gold)) {
      violf(gold_file[nm], "VEXP_FORMAT", gold_ln[nm], "a golden row names the scene " nm ", which has no scene row of the role golden")  # rule-format
    }
  }
  # the toy cross-rules (D-173; amendment A5): only where the root carries a toy registry
  if (toy_active) {
    for (i = 1; i <= ns; i++) {
      c = sc_case[i]
      if (!(c in kill_case) && !(c in exempt)) {
        violf(sc_file[i], "VIDEO_SCENE_UNTOYED", sc_ln[i], "the case " c " of the scene " sc_name[i] " is named by no toy EXPECT line (without a second column) and by no VIDEO_TOY_EXEMPT line")  # rule-video-scene-untoyed
      }
    }
    for (t = 1; t <= ntoy; t++) {
      tn = toy_list[t]
      if (!(tn in toy_file) || !(toy_file[tn] in watched)) continue
      has = 0
      for (i = 1; i <= ns; i++) {
        if (((tn SUBSEP sc_case[i]) in toy_kill) && sc_role[i] == "literal") has = 1
      }
      for (c in guest_case) {
        if ((tn SUBSEP c) in toy_kill) has = 1
      }
      if (!has) {
        violf("tests/controls/" tn "/EXPECT", "INV_TOY_NO_LITERAL", 0, "the toy " tn " mutates " toy_file[tn] " and lists no literal-role video case and no guest-kind case")  # rule-toy-no-literal
      }
    }
  }
  printf "S\t%d\t%d\t%d\t%d\t%d\n", nfiles, ns, ng, nr, nexempt + 0
  printf "RULES\t%d\n", nrec
}
