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
#   WF_MISSING_FETCH_DEPTH WF_PERSIST_CREDENTIALS WF_SHELL WF_RUNNER WF_NO_GATE_STEP WF_EXPR_IN_RUN WF_UNTRUSTED_EXPR
#   WF_FORBIDDEN_KEY WF_CONCURRENCY WF_NO_CC65_STEP WF_UNPARSABLE.
# The expression rules read every ${{ }} on every line that is not a YAML comment (the content of a run block included, a shell comment
# in it too: the runner expands it before the shell sees the text). An expression inside a run value or run block is WF_EXPR_IN_RUN;
# an expression anywhere that is not on the allowlist is WF_UNTRUSTED_EXPR (D-62, T-02-38). Allowed: github.event.pull_request.base.sha,
# github.event_name, github.workflow, github.ref, github.sha, and one of those compared with == to a quoted lowercase word.
# WF_FORBIDDEN_KEY: a secrets reference (the key secrets, or the text secrets. or secrets[ on any non-comment line), a self-hosted runner, a
# continue-on-error key, or a failure swallowed in a run block with || true or || : (the gate must be able to fail, D-62).
# WF_CONCURRENCY: the top-level concurrency block must carry group gate-${{ github.workflow }}-${{ github.ref }} and cancel-in-progress
# ${{ github.event_name == 'pull_request' }} exactly (a push to main is never cancelled by a later one).
# WF_NO_CC65_STEP: a step before the Gate step must run an apt-get install command that names cc65 (the fixture assembler, D-60, D-62
# amended); the Gate step's preflight fails closed without it. A comment line in the run block does not count.
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

# expr_ok: an expression body this checker allows: an allowlisted context, or one compared with == to a quoted lowercase word
function expr_ok(e,   n) {
  if (e in allow) return 1
  n = index(e, " == ")
  if (n == 0) return 0
  return ((substr(e, 1, n - 1) in allow) && substr(e, n + 4) ~ /^'[a-z_]+'$/)
}

# scan_text: the forbidden-key rules that read the text of a line. inrun is 1 for a run value or run block content.
function scan_text(s, ln, inrun) {
  if (index(s, "secrets.") > 0 || index(s, "secrets[") > 0) viol("WF_FORBIDDEN_KEY", ln, "a secrets reference")  # rule-secrets
  if (index(s, "self-hosted") > 0) viol("WF_FORBIDDEN_KEY", ln, "a self-hosted runner")  # rule-self-hosted
  if (inrun && s ~ /\|\|[ ]*(true|:)([ ;]|$)/) viol("WF_FORBIDDEN_KEY", ln, "a failure swallowed with || true")  # rule-or-true
}

# scan_exprs: every ${{ }} on one line. inrun is 1 when the line is a run value or the content of a run block.
function scan_exprs(s, ln, inrun,   p, q, e) {
  while ((p = index(s, "${{")) > 0) {
    if (inrun) viol("WF_EXPR_IN_RUN", ln, "an expression inside a run block (pass it through env instead)")  # rule-expr-in-run
    s = substr(s, p + 3)
    q = index(s, "}}")
    if (q == 0) {
      viol("WF_UNTRUSTED_EXPR", ln, "expression not closed on its line")  # rule-untrusted-open
      return
    }
    e = trim(substr(s, 1, q - 1))
    s = substr(s, q + 2)
    if (index(e, "secrets") == 0 && !expr_ok(e)) viol("WF_UNTRUSTED_EXPR", ln, "expression is not on the allowlist: " e)  # rule-untrusted
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

# cc65_line: 1 when one run text line is an apt-get install command that names the package cc65: the trimmed line begins with apt-get install or sudo apt-get install (a shell comment, or echo apt-get install cc65, is not one: IN-03)
function cc65_line(s,   t) {
  t = trim(s)
  if (substr(t, 1, 1) == "#") return 0
  return (t ~ /^(sudo )?apt-get install[ ]/ && s ~ /(^|[ ])cc65([ ]|$)/)
}

# step_end: the per-step rules, evaluated when the step closes
function step_end(   ischeckout) {
  if (!instep) return
  ischeckout = (index(suses, "actions/checkout@") == 1)
  if (ischeckout) havecheckout = 1
  if (ischeckout && sfd != "0") viol("WF_MISSING_FETCH_DEPTH", stepline, "the checkout step needs fetch-depth: 0 (the gate reads history)")  # rule-fetch-depth
  if (ischeckout && spc != "false") viol("WF_PERSIST_CREDENTIALS", stepline, "the checkout step needs persist-credentials: false")  # rule-persist
  if (sname == "Gate" && srun == "sh scripts/check") gatefound = 1  # rule-gate-found
  if (sname == "Gate" && cc65seen) cc65ok = 1
  if (scc65) cc65seen = 1
}

BEGIN {
  FS = "\n"
  cr = sprintf("%c", 13)
  stepsk = -1
  stepind = -1
  maxind = 0
  nsc = 0
  cid[0] = 0
  allow["github.event.pull_request.base.sha"] = 1
  allow["github.event_name"] = 1
  allow["github.workflow"] = 1
  allow["github.ref"] = 1
  allow["github.sha"] = 1
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
    if (ind > bsk) {
      if (instep && bskey == "run" && cc65_line(text)) scc65 = 1
      scan_text(text, ln, bskey == "run")
      scan_exprs(text, ln, bskey == "run")
      next
    }
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
    bskey = key
  } else check_scalar(val, ln)

  # expressions on this line (a run value on one line is a run block for the expression rules)
  scan_text(text, ln, haskey && key == "run" && !kopen && !bs)
  scan_exprs(text, ln, haskey && key == "run" && !kopen && !bs)

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
    scc65 = 0
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

  # forbidden keys
  if (key == "secrets" || key == "continue-on-error") viol("WF_FORBIDDEN_KEY", ln, key " is a forbidden key")  # rule-forbidden-key

  # concurrency: the group and the cancel expression, exactly
  if (top == "concurrency" && ki == 2 && key == "group" && val == "gate-${{ github.workflow }}-${{ github.ref }}") cgroup = 1  # rule-concurrency-group
  if (top == "concurrency" && ki == 2 && key == "cancel-in-progress" && val == "${{ github.event_name == 'pull_request' }}") ccancel = 1  # rule-concurrency-cancel

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
  if (instep && ki == stepind + 2 && key == "run" && cc65_line(val)) scc65 = 1
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
  if (!cgroup || !ccancel) viol("WF_CONCURRENCY", 0, "concurrency needs group gate-${{ github.workflow }}-${{ github.ref }} and cancel-in-progress ${{ github.event_name == 'pull_request' }}")  # rule-concurrency
  if (!runson) viol("WF_RUNNER", 0, "no runs-on")  # rule-runner-missing
  if (needgate && !havecheckout) viol("WF_MISSING_FETCH_DEPTH", 0, "no actions/checkout step: the gate needs the full history")  # rule-checkout-missing
  if (needgate && !gatefound) viol("WF_NO_GATE_STEP", 0, "no step named Gate that runs exactly: sh scripts/check")  # rule-gate-step
  if (needgate && gatefound && !cc65ok) viol("WF_NO_CC65_STEP", 0, "no step before the Gate step runs apt-get install with cc65")  # rule-cc65-step
  printf "E\t%d\n", NR
}
