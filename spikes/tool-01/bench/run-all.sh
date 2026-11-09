#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# run-all.sh - runs every row of the protocol's metric register for both options (TOOL-01 spike, D-12, D-15, D-36).
# usage: sh spikes/tool-01/bench/run-all.sh [--dry-run]
#   Reads the register rows between the protocol's metric-register markers. A row whose command starts with `hyperfine` is
#   run with that exact command line (<opt> and <variant> substituted), its raw JSON kept under spikes/tool-01/results/.
#   Other rows are copied from LOG.md (M01, M08, M14), counted from LINES.map (M09), assembled from LOG.md (M10) or read
#   from the end-of-plan gate output (M11 to M13, M15), which is copied verbatim to results/gates-<opt>.txt.
#   Normal mode appends one line per row to results/results.tsv (metric, variant, option, value, evidence) with one printf as
#   soon as that row completes, and never truncates or rewrites the file. On start it skips every (metric, variant,
#   option) already present (SKIP-PRESENT): that is the protocol's resume rule, not a re-run. A present row is never measured
#   again. A row that cannot be measured gets the value `not demonstrated: <reason>`.
#   --dry-run runs nothing: it writes every (row, option) pair with the value dry-run to spikes/tool-01/out/results.dry.tsv
#   and never touches spikes/tool-01/results/.
#   Rust timing rows rely on rust-toolchain.toml (the pinned stable 1.99.1). No command here selects another toolchain (D-36).
# exit  : 0 every row has a line | 1 a row could not be written | 2 usage or harness error
set -eu
LC_ALL=C; export LC_ALL

here=$(cd "$(dirname "$0")" && pwd)
SPIKE_DIR=$(cd "$here/.." && pwd)
root=$(cd "$SPIKE_DIR/../.." && pwd)
cd "$root"

protocol=docs/decisions/0001-language-build.protocol.md
log=spikes/tool-01/LOG.md
resdir=spikes/tool-01/results
outdir=spikes/tool-01/out

dry=no
case ${1:-} in
  "") ;;
  --dry-run) [ $# -eq 1 ] || { echo "usage: run-all.sh [--dry-run]" >&2; exit 2; }; dry=yes ;;
  *) echo "usage: run-all.sh [--dry-run]" >&2; exit 2 ;;
esac

[ -f "$protocol" ] || { echo "HARNESS_NO_FILE $protocol" >&2; exit 2; }
[ -f "$log" ] || { echo "HARNESS_NO_FILE $log" >&2; exit 2; }
mkdir -p "$outdir"

tmp=$(mktemp -d)
restore_all() {
  # Undo any bench edit a killed row left behind (opt_edit only touches these two tracked files).
  if [ "$dry" = no ]; then
    git checkout -- spikes/tool-01/c/src/core.c spikes/tool-01/rust/core/src/lib.rs 2> /dev/null || true
  fi
  rm -rf "$tmp"
}
trap restore_all EXIT
trap 'exit 130' INT TERM

if [ "$dry" = yes ]; then
  tsv=$outdir/results.dry.tsv
  : > "$tsv"
else
  tsv=$resdir/results.tsv
  mkdir -p "$resdir"
  s=$(git status --porcelain -- spikes/tool-01/c spikes/tool-01/rust)
  if [ -n "$s" ]; then
    echo "HARNESS_DIRTY_SKELETON: a skeleton source file is modified; restore it before measuring" >&2
    exit 2
  fi
  command -v hyperfine > /dev/null 2>&1 || { echo "HARNESS_NO_HYPERFINE: hyperfine is not installed" >&2; exit 2; }
fi

# the register: metric<TAB>variant<TAB>text between the first pair of backticks in the Command column
awk -F'|' '
  index($0, "<!-- metric-register:begin -->") { on = 1; next }
  index($0, "<!-- metric-register:end -->") { on = 0; next }
  on && NF >= 6 {
    m = $2; v = $3
    gsub(/^[ \t]+|[ \t]+$/, "", m)
    gsub(/^[ \t]+|[ \t]+$/, "", v)
    if (m !~ /^M[0-9][0-9]$/ || v == "") next
    n = split($5, p, "`")
    print m "\t" v "\t" (n >= 2 ? p[2] : "")
  }' "$protocol" > "$tmp/register"
[ -s "$tmp/register" ] || { echo "HARNESS_EMPTY_REGISTER $protocol" >&2; exit 2; }

tab=$(printf '\t')

# present METRIC VARIANT OPT: true when results.tsv already has that row
present() {
  [ "$dry" = no ] && [ -f "$tsv" ] || return 1
  K_M=$1 K_V=$2 K_O=$3 awk -F'\t' '$1 == ENVIRON["K_M"] && $2 == ENVIRON["K_V"] && $3 == ENVIRON["K_O"] { f = 1 } END { exit !f }' "$tsv"
}

# emit METRIC VARIANT OPT VALUE EVIDENCE: one printf per row, appended as soon as the row is known
emit() {
  _m=$1 _v=$2 _o=$3
  _val=$(printf '%s' "$4" | tr '\t\n' '  ')
  _ev=$(printf '%s' "$5" | tr '\t\n' '  ')
  if [ "$dry" = yes ]; then _val="dry-run"; fi
  printf '%s\t%s\t%s\t%s\t%s\n' "$_m" "$_v" "$_o" "$_val" "$_ev" >> "$tsv"
  echo "ROW $_m $_v $_o done"
}

# log_value PATTERN: the text after PATTERN on the first LOG.md line that starts with it (empty when absent)
log_value() {
  _lp=$1
  awk -v p="$_lp" 'index($0, p) == 1 { print substr($0, length(p) + 1); exit }' "$log"
}

# gates file for an option: the verbatim end-of-plan SLOW gate output (plans 01-07 and 01-09), copied once to results/
gates_file() {
  _go=$1
  if [ "$dry" = yes ]; then
    echo "$outdir/gates-$_go-full.txt"
    return 0
  fi
  _gd=$resdir/gates-$_go.txt
  if [ ! -f "$_gd" ]; then
    if [ -f "$outdir/gates-$_go-full.txt" ]; then
      cp "$outdir/gates-$_go-full.txt" "$_gd"
    else
      sh "$SPIKE_DIR/gates.sh" "$_go" < /dev/null > "$_gd" 2>&1 || true
    fi
  fi
  echo "$_gd"
}

# count_lines OPT CATEGORY: non-blank, non-comment lines over the LINES.map files of that category.
# .c .h .rs files drop // and /* */ comments; every other file type drops lines whose first non-blank character is #.
count_lines() {
  BASE=$SPIKE_DIR/$1 CAT=$2 awk '
    BEGIN { FS = "\t"; base = ENVIRON["BASE"]; cat = ENVIRON["CAT"]; total = 0; err = 0 }
    $2 == cat {
      f = base "/" $1
      cstyle = (f ~ /\.(c|h|rs)$/)
      inblock = 0
      while ((r = (getline line < f)) > 0) {
        s = line
        if (cstyle) {
          out = ""
          while (1) {
            if (inblock) {
              i = index(s, "*/")
              if (i == 0) { s = ""; break }
              s = substr(s, i + 2); inblock = 0
            } else {
              i = index(s, "/*"); j = index(s, "//")
              if (j > 0 && (i == 0 || j < i)) { out = out substr(s, 1, j - 1); s = ""; break }
              if (i == 0) { out = out s; s = ""; break }
              out = out substr(s, 1, i - 1); s = substr(s, i + 2); inblock = 1
            }
          }
          s = out
        } else {
          sub(/^[ \t]*#.*/, "", s)
        }
        if (s ~ /[^ \t\r]/) total++
      }
      if (r < 0) err = 1
      close(f)
    }
    END { if (err) print "ERR"; else print total }' "$SPIKE_DIR/$1/LINES.map"
}

# tools_list OPT: the M10 table as one line, from the LOG.md install lines and the preinstalled tool versions
tools_list() {
  if [ "$1" = c ]; then
    _cm=$(sed -n 's/^ *cmake version //p' "$log" | sed -n 1p)
    _ni=$(awk '/^ninja --version:/ { getline; gsub(/^ +| +$/, ""); print; exit }' "$log")
    _ac=$(sed -n 's/^ *Apple clang version \([0-9.]*\).*/\1/p' "$log" | sed -n 1p)
    _hc=$(sed -n 's/^ *Homebrew clang version \([0-9.]*\).*/\1/p' "$log" | sed -n 1p)
    printf 'cmake %s (preinstalled; build, install); ninja %s (preinstalled; build); Apple clang %s (preinstalled; build, sanitizer, fuzz compile); Homebrew llvm@21 %s (preinstalled; libFuzzer runtime archive by explicit path only, D-17)' "$_cm" "$_ni" "$_ac" "$_hc"
  else
    awk '$1 == "install" && $2 != "hyperfine" {
      src = $0
      sub(/^install [^ ]+ [^ ]+ [^ ]+ [^ ]+ /, "", src)
      printf "%s%s %s (install %s s; licence %s; source %s)", (n++ ? "; " : ""), $2, $3, $4, $5, src
    }' "$log"
  fi
  _hf=$(hyperfine --version 2> /dev/null | sed -n 1p || true)
  printf '; baseline excluded from T by the protocol: sh, awk, git, perl, pkgconf, system cc; measurement only: %s' "${_hf:-hyperfine (not installed)}"
}

# run_timing METRIC VARIANT OPT COMMAND: the register command with <opt> and <variant> substituted, run once
run_timing() {
  _m=$1 _v=$2 _o=$3
  _cmd=$(printf '%s' "$4" | sed "s/<opt>/$_o/g; s/<variant>/$_v/g")
  _json=$resdir/$_m-$_o-$_v.json
  _hl=$outdir/hf-$_m-$_o-$_v.log
  if [ "$dry" = yes ]; then
    emit "$_m" "$_v" "$_o" dry-run "dry run: $_json"
    return 0
  fi
  echo "ROW $_m $_v $_o start $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  rm -f "$_json"
  if (eval "$_cmd") < /dev/null > "$_hl" 2>&1; then _rc=0; else _rc=$?; fi
  if [ "$_rc" -eq 0 ] && [ -f "$_json" ]; then
    _val=$(awk '
      /^ *"(mean|stddev|median|min|max)":/ {
        k = $1; gsub(/[": ]/, "", k); v = $2; gsub(/,/, "", v); a[k] = v
      }
      END {
        if (a["median"] == "" || a["mean"] == "" || a["stddev"] == "" || a["min"] == "" || a["max"] == "") exit 1
        printf "median=%.5f mean=%.5f stddev=%.5f min=%.5f max=%.5f", a["median"], a["mean"], a["stddev"], a["min"], a["max"]
      }' "$_json") || _val=""
    if [ -n "$_val" ]; then
      emit "$_m" "$_v" "$_o" "$_val" "hyperfine --warmup 1 --runs 10; raw JSON $_json"
      return 0
    fi
    emit "$_m" "$_v" "$_o" "not demonstrated: failed: hyperfine JSON has no timing fields" "$_json"
    return 0
  fi
  _why=$(tail -n 3 "$_hl" | tr '\n\t' '  ')
  emit "$_m" "$_v" "$_o" "not demonstrated: failed: hyperfine exit $_rc ($_why)" "log $_hl"
}

# do_row METRIC VARIANT OPT COMMAND
do_row() {
  _m=$1 _v=$2 _o=$3 _c=$4
  if present "$_m" "$_v" "$_o"; then
    echo "SKIP-PRESENT $_m $_v $_o"
    return 0
  fi
  case $_c in
    hyperfine\ *) run_timing "$_m" "$_v" "$_o" "$_c"; return 0 ;;
  esac
  case $_m-$_v in
    M01-cold-first-build)
      _x=$(log_value "M01 cold-first-build $_o ")
      if [ -n "$_x" ]; then
        emit "$_m" "$_v" "$_o" "seconds=$_x" "copied from LOG.md line M01 cold-first-build $_o (single first-ever opt_build debug, never re-measured)"
      else
        emit "$_m" "$_v" "$_o" "not demonstrated: no M01 cold-first-build $_o line in LOG.md" "LOG.md"
      fi
      ;;
    M01-toolchain-install)
      if [ "$_o" = c ]; then
        emit "$_m" "$_v" "$_o" "preinstalled" "protocol M01 row: preinstalled for C"
      else
        _x=$(log_value "M01 toolchain-install $_o ")
        if [ -n "$_x" ]; then
          emit "$_m" "$_v" "$_o" "seconds=$_x" "copied from LOG.md line M01 toolchain-install $_o and the install lines for rustup and 1.99.1"
        else
          emit "$_m" "$_v" "$_o" "not demonstrated: no M01 toolchain-install $_o line in LOG.md" "LOG.md"
        fi
      fi
      ;;
    M08-sanitizer)
      _x=$(log_value "SANITIZER $_o ")
      if [ -n "$_x" ]; then
        emit "$_m" "$_v" "$_o" "$_x" "LOG.md line SANITIZER $_o (opt_sanitizer, plans 01-07 and 01-09)"
      else
        emit "$_m" "$_v" "$_o" "not demonstrated: no SANITIZER $_o line in LOG.md" "LOG.md"
      fi
      ;;
    M09-*)
      _x=$(count_lines "$_o" "$_v")
      if [ -n "$_x" ] && [ "$_x" != ERR ]; then
        emit "$_m" "$_v" "$_o" "$_x" "awk count of non-blank non-comment lines over $_o/LINES.map category $_v"
      else
        emit "$_m" "$_v" "$_o" "not demonstrated: a LINES.map file for $_o could not be read" "$_o/LINES.map"
      fi
      ;;
    M10-list)
      emit "$_m" "$_v" "$_o" "$(tools_list "$_o")" "LOG.md install lines and toolchain versions"
      ;;
    M11-G4 | M12-G5 | M13-liveness | M15-path-leak)
      _gf=$(gates_file "$_o")
      if [ ! -f "$_gf" ]; then
        emit "$_m" "$_v" "$_o" "not demonstrated: no gate output for $_o" "$_gf"
        return 0
      fi
      case $_m in
        M11) _x=$(sed -n 's/^GATE consumer-proof //p' "$_gf" | sed -n 1p) ;;
        M12)
          _gp=$(sed -n 's/^GATE fuzz60 //p' "$_gf" | sed -n 1p)
          _fz=$(sed -n 's/^FUZZ //p' "$_gf" | sed -n 1p)
          _x=
          if [ -n "$_gp" ] && [ -n "$_fz" ]; then _x="$_gp $_fz"; fi
          ;;
        M13) _x=$(sed -n 's/^LIVENESS //p' "$_gf" | sed -n 1p) ;;
        M15)
          _lk=$(sed -n 's/^LEAK //p' "$_gf" | sed -n 1p)
          _sl=$(sed -n 's/^SEEDED-LEAK //p' "$_gf" | sed -n 1p)
          _x=
          if [ -n "$_lk" ] && [ -n "$_sl" ]; then _x="leak=$_lk seeded-leak=$_sl"; fi
          ;;
      esac
      if [ -n "$_x" ]; then
        emit "$_m" "$_v" "$_o" "$_x" "$_gf (end-of-plan SLOW gate output, copied verbatim)"
      else
        emit "$_m" "$_v" "$_o" "not demonstrated: the gate output for $_o has no $_m line" "$_gf"
      fi
      ;;
    M14-wasm)
      if [ "$_o" = c ]; then
        _x=$(log_value "WASM c ")
        if [ -n "$_x" ]; then
          emit "$_m" "$_v" "$_o" "$_x" "LOG.md line WASM c"
        else
          emit "$_m" "$_v" "$_o" "not demonstrated: no WASM c line in LOG.md" "LOG.md"
        fi
      else
        _x=$(log_value "WASM transcript-identical=")
        if [ -n "$_x" ]; then
          emit "$_m" "$_v" "$_o" "transcript-identical=$_x" "LOG.md line WASM transcript-identical (opt_wasm, wasm32-wasip1 under wasmtime)"
        else
          emit "$_m" "$_v" "$_o" "not demonstrated: no WASM transcript-identical line in LOG.md" "LOG.md"
        fi
      fi
      ;;
    *)
      emit "$_m" "$_v" "$_o" "not demonstrated: no rule in run-all.sh for $_m $_v" "$protocol"
      ;;
  esac
}

for opt in c rust; do
  while IFS=$tab read -r metric variant cmd; do
    [ -n "$metric" ] || continue
    do_row "$metric" "$variant" "$opt" "$cmd"
  done < "$tmp/register"
done
echo "RUN-ALL done tsv=$tsv"
