# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# workflow-rules.awk - the restricted YAML line checker and the structural rules of scripts/workflow-check (CI-01, D-62).
# usage: awk -v needgate=0|1 -f workflow-rules.awk WORKFLOW.yml
# This is not a YAML parser. It reads the small block-style subset that a workflow here may use and refuses everything else, so a
# construct it cannot see (a flow map, an anchor, a tag, a second document, a tab, a multi-line plain value) is WF_UNPARSABLE and
# the caller exits 2: a file the checker cannot read is never a pass. Accepted line shapes, all at two-space indents:
#   a blank line, a comment line, KEY: VALUE, KEY: (opens a block), - VALUE, - KEY: VALUE, - KEY: (opens a block inside a list item),
#   and the content of a block scalar (a key whose value is | or >, content lines indented deeper than the key).
# A value is a plain scalar (no ": " inside, no leading flow, anchor, tag or reserved indicator) or a quoted scalar closed on its line.
# A key is [A-Za-z_][A-Za-z0-9_.-]* and may not repeat inside one mapping. A list item sits two spaces under its key.
# Output records, one per line:  V TAB CODE TAB LINE TAB DETAIL   and a single last  E TAB LINES  (a run that prints no E line is dead).
# Codes owned here: WF_NO_PERMISSIONS WF_EXCESS_PERMISSION WF_UNPINNED_ACTION WF_UNEXPECTED_ACTION WF_FORBIDDEN_TRIGGER
#   WF_MISSING_FETCH_DEPTH WF_PERSIST_CREDENTIALS WF_SHELL WF_RUNNER WF_NO_GATE_STEP WF_UNPARSABLE.
# Every rule is one self-contained statement ending in a tag comment (rule-NAME), so a selftest can delete the line and show the
# control that depends on it flip. Awk dialect: no regex intervals, no gensub, no length of an array (BWK, mawk and gawk all read it).

function viol(code, ln, detail) {
  printf "V\t%s\t%d\t%s\n", code, ln, detail
}

# bad: record WF_UNPARSABLE and stop reading; END still prints the E line
function bad(ln, why) {
  viol("WF_UNPARSABLE", ln, why)
  dead = 1
  exit 0
}

function trim(s) {
  sub(/^[ ]+/, "", s)
  sub(/[ ]+$/, "", s)
  return s
}

# clear_from: forget the open blocks and list items at indent k and deeper
function clear_from(k,   j) {
  for (j = k; j <= maxind; j += 2) {
    delete par[j]
    delete dsh[j]
  }
}

# check_scalar: a value this checker can read, else WF_UNPARSABLE
function check_scalar(v, ln,   c) {
  c = substr(v, 1, 1)
  if (c == "\"" || c == "'") {
    if (length(v) < 2 || substr(v, length(v), 1) != c) bad(ln, "quoted value not closed on its line")
    return
  }
  if (index("{[&*!%@`|>", c) > 0) bad(ln, "flow, anchor, tag, block or reserved indicator at the start of a value")
  if (substr(v, 1, 2) == "- ") bad(ln, "list entry inside a value")
  if (index(v, ": ") > 0 || substr(v, length(v), 1) == ":") bad(ln, "colon inside a plain value")
}

# check_uses: every uses value is exactly actions/checkout at a full 40-character lowercase hex commit
function check_uses(v, ln,   n, nm, ref) {
  n = index(v, "@")
  nm = (n > 0) ? substr(v, 1, n - 1) : v
  ref = (n > 0) ? substr(v, n + 1) : ""
  if (length(ref) != 40 || ref !~ /^[0-9a-f]+$/) viol("WF_UNPINNED_ACTION", ln, nm " is not pinned to a full commit SHA")  # rule-pin
  if (nm != "actions/checkout") viol("WF_UNEXPECTED_ACTION", ln, nm " is not on the action allowlist")  # rule-allowlist
}

# step_end: the per-step rules, evaluated when the step closes
function step_end(   ischeckout) {
  if (!instep) return
  ischeckout = (index(suses, "actions/checkout@") == 1)
  if (ischeckout) havecheckout = 1
  if (ischeckout && sfd != "0") viol("WF_MISSING_FETCH_DEPTH", stepline, "the checkout step needs fetch-depth: 0 (the gate reads history)")  # rule-fetch-depth
  if (ischeckout && spc != "false") viol("WF_PERSIST_CREDENTIALS", stepline, "the checkout step needs persist-credentials: false")  # rule-persist
  if (sname == "Gate" && srun == "sh scripts/check") gatefound = 1  # rule-gate-found
}

BEGIN {
  FS = "\n"
  cr = sprintf("%c", 13)
  stepsk = -1
  stepind = -1
  maxind = 0
  nsc = 0
  cid[0] = 0
}

{
  ln = NR
  line = $0
  if (index(line, cr) > 0) bad(ln, "carriage return in the file")
  sub(/[ ]+$/, "", line)
  if (line == "") next
  match(line, /^[ ]*/)
  ind = RLENGTH
  text = substr(line, ind + 1)
  # the content of a block scalar is not read as structure
  if (bs) {
    if (ind > bsk) next
    bs = 0
  }
  if (substr(text, 1, 1) == "\t") bad(ln, "tab in indentation")
  if (substr(text, 1, 1) == "#") next
  if (ind % 2 != 0) bad(ln, "odd indentation")
  if (ind > maxind) maxind = ind

  # shape of the line
  isdash = 0
  rest = text
  if (substr(text, 1, 2) == "- ") {
    isdash = 1
    rest = substr(text, 3)
    if (substr(rest, 1, 1) == " ") bad(ln, "more than one space after a dash")
  } else if (text == "-") bad(ln, "empty list item")
  haskey = 0
  if (match(rest, /^[A-Za-z_][A-Za-z0-9_.-]*:/)) {
    nx = substr(rest, RLENGTH + 1, 1)
    if (nx == "" || nx == " ") {
      haskey = 1
      key = substr(rest, 1, RLENGTH - 1)
      val = trim(substr(rest, RLENGTH + 1))
    }
  }
  if (!haskey && !isdash) bad(ln, "not a comment, a key or a list item")
  if (!haskey) val = trim(rest)
  if (isdash && ind == 0) bad(ln, "list at the top level")
  ki = isdash ? ind + 2 : ind

  # place the line under its parent
  if (isdash) {
    if (!((ind - 2) in par)) bad(ln, "list item without a parent key")
  } else if (ind > 0) {
    if (!(((ind - 2) in par) || ((ind - 2) in dsh))) bad(ln, "key without a parent")
  }
  clear_from(ind)
  if (isdash) {
    dsh[ind] = 1
    cid[ind + 2] = ++nsc
  }

  # the value: an open block, a block scalar, or a scalar this checker can read
  vc = substr(val, 1, 1)
  if (vc != "\"" && vc != "'") {
    sub(/[ ]+#.*$/, "", val)
    if (substr(val, 1, 1) == "#") val = ""
  }
  kopen = 0
  if (val == "") {
    if (!haskey) bad(ln, "empty list item")
    kopen = 1
  } else if (val ~ /^[|>][-+0-9]*$/) {
    if (!haskey) bad(ln, "block scalar as a list item")
    bs = 1
    bsk = ki
  } else check_scalar(val, ln)

  # leaving a permissions block or the steps list
  if (inperm && ind <= pind) inperm = 0
  if (instep && ind <= stepsk) {
    step_end()
    instep = 0
  }
  if (isdash && ind == stepind) {
    step_end()
    instep = 1
    sname = ""
    srun = ""
    suses = ""
    sfd = ""
    spc = ""
    stepline = ln
  }

  if (!haskey) {
    # a scalar list item
    if (inperm) viol("WF_EXCESS_PERMISSION", ln, "a list item inside a permissions block")  # rule-perm-scope
    if (top == "on" && (val == "pull_request_target" || val == "workflow_run")) viol("WF_FORBIDDEN_TRIGGER", ln, val " is a forbidden trigger")  # rule-trigger
    next
  }

  # a key line
  dk = cid[ki] SUBSEP key
  if (dk in seen) bad(ln, "duplicate key " key)
  seen[dk] = 1
  if (kopen) {
    par[ki] = key
    cid[ki + 2] = ++nsc
  }
  if (ki == 0) top = key

  # permissions: a top-level block exists and nothing grants more than contents: read
  if (inperm && (isdash || !(key == "contents" && val == "read"))) viol("WF_EXCESS_PERMISSION", ln, key ": " val " is more than contents: read")  # rule-perm-scope
  if (key == "permissions" && ki == 0) haveperm = 1
  if (key == "permissions" && kopen) {
    inperm = 1
    pind = ki
  }
  if (key == "permissions" && !kopen) viol("WF_EXCESS_PERMISSION", ln, "permissions given as a scalar value")  # rule-perm-scalar

  # triggers
  if (top == "on" && ki > 0 && (key == "pull_request_target" || key == "workflow_run")) viol("WF_FORBIDDEN_TRIGGER", ln, key " is a forbidden trigger")  # rule-trigger
  if (ki == 0 && key == "on" && (val == "pull_request_target" || val == "workflow_run")) viol("WF_FORBIDDEN_TRIGGER", ln, val " is a forbidden trigger")  # rule-trigger

  # shell and runner
  if (key == "shell" && val != "sh -eu {0}") viol("WF_SHELL", ln, "shell must be sh -eu {0}")  # rule-shell
  if (key == "shell" && val == "sh -eu {0}" && top == "defaults") shellok = 1
  if (key == "runs-on") runson = 1
  if (key == "runs-on" && val != "ubuntu-24.04") viol("WF_RUNNER", ln, "runs-on must be ubuntu-24.04")  # rule-runner

  # actions
  if (key == "uses") check_uses(val, ln)

  # per-step facts, read when the step closes
  if (key == "steps" && kopen) {
    stepsk = ki
    stepind = ki + 2
  }
  if (instep && ki == stepind + 2 && key == "name") sname = val
  if (instep && ki == stepind + 2 && key == "run") srun = val
  if (instep && ki == stepind + 2 && key == "uses") suses = val
  if (instep && key == "fetch-depth") sfd = val
  if (instep && key == "persist-credentials") spc = val
}

END {
  if (dead) {
    printf "E\t%d\n", NR
    exit 0
  }
  if (NR == 0) {
    viol("WF_UNPARSABLE", 0, "empty file")
    printf "E\t%d\n", NR
    exit 0
  }
  step_end()
  if (!haveperm) viol("WF_NO_PERMISSIONS", 0, "no top-level permissions block")  # rule-perm-missing
  if (!shellok) viol("WF_SHELL", 0, "defaults.run.shell: sh -eu {0} is not set")  # rule-shell-missing
  if (!runson) viol("WF_RUNNER", 0, "no runs-on")  # rule-runner-missing
  if (needgate && !havecheckout) viol("WF_MISSING_FETCH_DEPTH", 0, "no actions/checkout step: the gate needs the full history")  # rule-checkout-missing
  if (needgate && !gatefound) viol("WF_NO_GATE_STEP", 0, "no step named Gate that runs exactly: sh scripts/check")  # rule-gate-step
  printf "E\t%d\n", NR
}
