#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# hooks.sh - the Rust option's implementation of the shared gates.sh hook contract (TOOL-01 spike). Sourced, never executed.
# The caller (gates.sh or a bench script) sets SPIKE_DIR (spikes/tool-01) and OPT_DIR (spikes/tool-01/rust) first.
# Build trees live under SPIKE_DIR/out/, which is gitignored.
# When sourced from the repository root without those variables (a one-off verify command), they default from $PWD.
# Every cargo command runs from a directory that holds rust-toolchain.toml, which pins the stable 1.99.1 (D-36).
SPIKE_DIR=${SPIKE_DIR:-$PWD/spikes/tool-01}
OPT_DIR=${OPT_DIR:-$SPIKE_DIR/rust}

# rustup is a keg-only Homebrew formula: put its bin directory on PATH here, never in a shell profile.
RUSTUP_BIN=$(brew --prefix rustup 2> /dev/null || echo /opt/homebrew/opt/rustup)/bin
PATH="$RUSTUP_BIN:$HOME/.cargo/bin:$PATH"
export PATH

rust_profile_flag() {
  case $1 in
    debug) echo "" ;;
    release) echo "--release" ;;
    *) echo "hooks: unknown profile $1" >&2; return 1 ;;
  esac
}

# opt_build <debug|release>: build the whole workspace in that profile, into out/build-rust-<profile>.
opt_build() {
  _p=${1:?profile}
  _pf=$(rust_profile_flag "$_p") || return 1
  _bd=$SPIKE_DIR/out/build-rust-$_p
  _log=$_bd.log
  mkdir -p "$SPIKE_DIR/out" || return 1
  : > "$_log"
  # shellcheck disable=SC2086
  ( cd "$OPT_DIR" && CARGO_TARGET_DIR=$_bd CARGO_TERM_COLOR=never cargo build --workspace $_pf ) >> "$_log" 2>&1 || { cat "$_log" >&2; return 1; }
}

# opt_install <src_copy> <build_dir> <prefix> <remap yes|no>
# Builds the facade staticlib from the given source copy (target dir = build_dir) and installs lib/libduospool.a,
# include/duospool.h and lib/pkgconfig/duospool.pc under prefix. With remap yes RUSTFLAGS maps the copy, the build
# directory (both spellings, macOS /var is a symlink to /private/var, Pitfall 12), ~/.cargo and the rustc sysroot;
# with remap no nothing is mapped (the seeded-leak control).
opt_install() {
  _src=${1:?src_copy}
  _bd=${2:?build_dir}
  _prefix=${3:?prefix}
  _remap=${4:?remap}
  mkdir -p "$_bd" "$_prefix/lib/pkgconfig" "$_prefix/include" || return 1
  _srcp=$(cd "$_src" && pwd -P) || return 1
  _bdp=$(cd "$_bd" && pwd -P) || return 1
  # A copy of the source tree can carry a target directory from an ad-hoc cargo run; the build uses build_dir only.
  rm -rf "$_src/target"
  _flags=
  if [ "$_remap" = yes ]; then
    _sysroot=$(cd "$_src" && rustc --print sysroot) || return 1
    _flags="--remap-path-prefix=$_src=/src --remap-path-prefix=$_srcp=/src --remap-path-prefix=$_bd=/build --remap-path-prefix=$_bdp=/build --remap-path-prefix=$HOME/.cargo=/cargo --remap-path-prefix=$_sysroot=/rustc"
  fi
  _ilog=$_bd.install.log
  # Debug info is on in the installed release build on purpose (the C option's RelWithDebInfo precedent): with it the
  # archive carries absolute paths when they are not remapped, so the path-leak grep inspects an archive that can leak.
  ( cd "$_src" && RUSTFLAGS=$_flags CARGO_TARGET_DIR=$_bd CARGO_TERM_COLOR=never CARGO_PROFILE_RELEASE_DEBUG=2 \
    cargo build --locked --release -p duospool ) > "$_ilog" 2>&1 || { cat "$_ilog" >&2; return 1; }
  cp "$_bd/release/libduospool.a" "$_prefix/lib/libduospool.a" &&
    cp "$_src/include/duospool.h" "$_prefix/include/duospool.h" &&
    cp "$_src/duospool.pc.in" "$_prefix/lib/pkgconfig/duospool.pc" || return 1
  if ! grep -qx 'INSTALL-BUILD-TYPE rust release+debuginfo' "$SPIKE_DIR/LOG.md" 2> /dev/null; then
    echo 'INSTALL-BUILD-TYPE rust release+debuginfo' >> "$SPIKE_DIR/LOG.md"
  fi
}

# opt_runner [debug|release]: print the path of the manifest runner binary for that profile (default debug).
opt_runner() {
  _p=${1:-debug}
  rust_profile_flag "$_p" > /dev/null || return 1
  echo "$SPIKE_DIR/out/build-rust-$_p/$_p/runner"
}

# opt_fuzz60 <seconds> <planted yes|no> (protocol G5, D-35 recipe, D-36 toolchain rule)
# cargo-fuzz on the dated nightly (the only place besides opt_sanitizer that names it, D-36). A 10-second ASan canary
# runs first on an empty input under perl alarm; when it hangs, the build and the run use `-s none` and the fallback is
# an ergonomics cost (C5). The build tree, the seed corpus and crash artifacts live under SPIKE_DIR/out/ (gitignored),
# so the G4 copy of this option directory never carries them.
# Prints  CANARY asan=ok|hang  and  FUZZ elapsed=<s> crashed=<yes|no> execs_per_sec=<n>  and returns 0 when the outcome is
# the wanted one (planted yes: a crash was found; planted no: no crash), 1 otherwise.
opt_fuzz60() {
  _secs=${1:?seconds}
  _pl=${2:?planted}
  case $_pl in yes | no) ;; *) echo "hooks: planted must be yes or no" >&2; return 1 ;; esac
  _fd=$SPIKE_DIR/out/fuzz-rust-$_pl
  rm -rf "$_fd"
  mkdir -p "$_fd/corpus" "$_fd/artifacts" "$_fd/target" || return 1
  _feat=
  if [ "$_pl" = yes ]; then _feat=planted-bug; fi
  _fx="$OPT_DIR/fuzz"
  _san=asan
  # Build with ASan (the cargo-fuzz default), then run the canary on an empty input.
  # shellcheck disable=SC2086
  ( cd "$OPT_DIR" && CARGO_TERM_COLOR=never cargo +nightly-2026-11-09 fuzz build --target-dir "$_fd/target" ${_feat:+--features $_feat} loader ) \
    > "$_fd/build.log" 2>&1 || { cat "$_fd/build.log" >&2; return 1; }
  : > "$_fd/canary-empty"
  if ( cd "$OPT_DIR" && CARGO_TERM_COLOR=never perl -e 'alarm 10; exec @ARGV' cargo +nightly-2026-11-09 fuzz run --target-dir "$_fd/target" ${_feat:+--features $_feat} loader "$_fd/canary-empty" ) \
    > "$_fd/canary.log" 2>&1; then
    echo "CANARY asan=ok"
  else
    _crc=$?
    # A hang (SIGALRM, 142) is the failure the canary exists to detect; any other non-zero status on an empty input is also a failed canary.
    echo "CANARY asan=hang"
    echo "hooks: canary exit status $_crc, falling back to -s none" >&2
    _san=none
    ( cd "$OPT_DIR" && CARGO_TERM_COLOR=never cargo +nightly-2026-11-09 fuzz build -s none --target-dir "$_fd/target-none" ${_feat:+--features $_feat} loader ) \
      > "$_fd/build-none.log" 2>&1 || { cat "$_fd/build-none.log" >&2; return 1; }
  fi
  # One generated all-zero 64 KiB seed so the clean run also exercises the accepted-load path (generated bytes stay in out/).
  head -c 65536 /dev/zero > "$_fd/corpus/seed-zero" || return 1
  _t0=$(perl -MTime::HiRes=time -e 'printf "%.3f", time')
  _hard=$((_secs + 90))
  if [ "$_san" = asan ]; then
    # shellcheck disable=SC2086
    ( cd "$OPT_DIR" && CARGO_TERM_COLOR=never perl -e 'alarm shift; exec @ARGV' "$_hard" cargo +nightly-2026-11-09 fuzz run --target-dir "$_fd/target" ${_feat:+--features $_feat} loader "$_fd/corpus" \
      -- -max_total_time="$_secs" -max_len=1048600 "-artifact_prefix=$_fd/artifacts/" > "$_fd/run.log" 2>&1 ) 2> /dev/null && _rc=0 || _rc=$?
  else
    # shellcheck disable=SC2086
    ( cd "$OPT_DIR" && CARGO_TERM_COLOR=never perl -e 'alarm shift; exec @ARGV' "$_hard" cargo +nightly-2026-11-09 fuzz run -s none --target-dir "$_fd/target-none" ${_feat:+--features $_feat} loader "$_fd/corpus" \
      -- -max_total_time="$_secs" -max_len=1048600 "-artifact_prefix=$_fd/artifacts/" > "$_fd/run.log" 2>&1 ) 2> /dev/null && _rc=0 || _rc=$?
  fi
  _t1=$(perl -MTime::HiRes=time -e 'printf "%.3f", time')
  _el=$(awk -v a="$_t0" -v b="$_t1" 'BEGIN { printf "%.1f", b - a }')
  _eps=$(awk '{ for (i = 1; i < NF; i++) if ($i == "exec/s:") v = $(i + 1) } END { print (v == "" ? 0 : v) }' "$_fd/run.log")
  _crashed=no
  if [ "$_rc" -ne 0 ]; then _crashed=yes; fi
  echo "FUZZ elapsed=$_el crashed=$_crashed execs_per_sec=$_eps"
  if [ "$_pl" = yes ]; then
    [ "$_crashed" = yes ] && grep -q 'ERROR: \(libFuzzer\|AddressSanitizer\)' "$_fd/run.log"
  else
    [ "$_crashed" = no ]
  fi
}

# opt_sanitizer (protocol M08, D-36): Miri over the facade crate's own test module, which drives the extern "C" entry points
# (create, load, step, time, digest_hex, destroy, the null and short-buffer paths). Miri is allowed on the dated nightly
# by D-36 and only here; build and test rows never use it. The Miri build tree lives under SPIKE_DIR/out/.
# Prints SANITIZER PASS or SANITIZER FAIL and returns 0 or 1.
opt_sanitizer() {
  _md=$SPIKE_DIR/out/miri-rust
  mkdir -p "$_md" || return 1
  if ( cd "$OPT_DIR" && CARGO_TARGET_DIR=$_md CARGO_TERM_COLOR=never cargo +nightly-2026-11-09 miri test -p duospool ) > "$_md.log" 2>&1 &&
    grep -q '^test result: ok\. [1-9][0-9]* passed; 0 failed' "$_md.log"; then
    echo "SANITIZER PASS"
  else
    tail -n 30 "$_md.log" >&2
    echo "SANITIZER FAIL"
    return 1
  fi
}

# opt_clean <debug|release|all> (bench): remove the matching build trees (and their logs) so the next build is cold.
# `all` also removes the unused in-tree target directory and the install, Miri, fuzz and edit-check trees.
opt_clean() {
  _w=${1:?debug|release|all}
  case $_w in
    debug | release) rm -rf "$SPIKE_DIR/out/build-rust-$_w" "$SPIKE_DIR/out/build-rust-$_w.log" ;;
    all)
      rm -rf "$SPIKE_DIR"/out/build-rust-debug "$SPIKE_DIR"/out/build-rust-release "$SPIKE_DIR"/out/build-rust-debug.log \
        "$SPIKE_DIR"/out/build-rust-release.log "$OPT_DIR/target"
      ;;
    *) echo "hooks: opt_clean wants debug, release or all" >&2; return 1 ;;
  esac
}

# rust_run_standalone <target_dir>: build the workspace in debug into a dedicated target directory (outside every
# measured build tree) and print the manifest runner transcript on stdout; the runner's exit status is returned.
rust_run_standalone() {
  _td=${1:?target_dir}
  mkdir -p "$_td" || return 1
  ( cd "$OPT_DIR" && CARGO_TARGET_DIR=$_td CARGO_TERM_COLOR=never cargo build --workspace ) > "$_td.log" 2>&1 || { cat "$_td.log" >&2; return 1; }
  [ -f "$SPIKE_DIR/out/lcg-64k.bin" ] || sh "$SPIKE_DIR/gen-fixtures.sh" > /dev/null || return 1
  perl -e 'alarm 300; exec @ARGV' "$_td/debug/runner" "$SPIKE_DIR/fixtures.manifest" "$SPIKE_DIR/out" 2> /dev/null
}

# opt_edit <comment|real> (protocol M03, M04): edit core/src/lib.rs in place.
#   comment: append a unique comment line. real: toggle one comparison between two forms that cannot differ (the content
#   vector is only ever empty or exactly CONTENT_SIZE long), so the code changes and the observable output must not. The
#   hook proves that by comparing the runner transcript before and after, built in a dedicated tree outside every
#   measured build tree. Undo either with opt_restore.
opt_edit() {
  _k=${1:?comment|real}
  _core=$OPT_DIR/core/src/lib.rs
  case $_k in
    comment)
      printf '// bench edit %s %s\n' "$(date +%s)" "$$" >> "$_core" || return 1
      ;;
    real)
      rust_run_standalone "$SPIKE_DIR/out/edit-check-rust" > "$SPIKE_DIR/out/edit-check-rust-before.txt" || { echo "hooks: opt_edit: the pre-edit transcript run failed" >&2; return 1; }
      if grep -q 'self.content.len() == CONTENT_SIZE' "$_core"; then
        perl -pi -e 's/self\.content\.len\(\) == CONTENT_SIZE/self.content.len() >= CONTENT_SIZE/' "$_core" || return 1
      else
        perl -pi -e 's/self\.content\.len\(\) >= CONTENT_SIZE/self.content.len() == CONTENT_SIZE/' "$_core" || return 1
      fi
      rust_run_standalone "$SPIKE_DIR/out/edit-check-rust" > "$SPIKE_DIR/out/edit-check-rust-after.txt" || { echo "hooks: opt_edit: the post-edit transcript run failed" >&2; return 1; }
      cmp -s "$SPIKE_DIR/out/edit-check-rust-before.txt" "$SPIKE_DIR/out/edit-check-rust-after.txt" || { echo "hooks: opt_edit real changed the runner transcript" >&2; return 1; }
      ;;
    *) echo "hooks: opt_edit wants comment or real" >&2; return 1 ;;
  esac
}

# opt_restore: undo opt_edit.
opt_restore() {
  git -C "$OPT_DIR" checkout -- core/src/lib.rs
}

# opt_test [debug|release] (protocol M05, default debug): build that profile, then run its manifest runner over the manifest.
opt_test() {
  _p=${1:-debug}
  opt_build "$_p" || return 1
  _rb=$(opt_runner "$_p") || return 1
  [ -f "$SPIKE_DIR/out/lcg-64k.bin" ] || sh "$SPIKE_DIR/gen-fixtures.sh" > /dev/null || return 1
  perl -e 'alarm 300; exec @ARGV' "$_rb" "$SPIKE_DIR/fixtures.manifest" "$SPIKE_DIR/out" > /dev/null
}

# opt_wasm (protocol M14): build the runner for wasm32-wasip1 on the pinned stable toolchain, run it under wasmtime with
# the manifest directory and out/ preopened, and compare its transcript with the native Rust runner's.
# Prints  WASM transcript-identical=yes|no  and returns 0 only for yes. wasmtime is a Homebrew formula (D-12).
opt_wasm() {
  _wd=$SPIKE_DIR/out/build-rust-wasm
  mkdir -p "$_wd" "$SPIKE_DIR/out" || return 1
  ( cd "$OPT_DIR" && CARGO_TARGET_DIR=$_wd CARGO_TERM_COLOR=never cargo build --release -p runner --target wasm32-wasip1 ) > "$_wd.log" 2>&1 || { cat "$_wd.log" >&2; return 1; }
  [ -f "$SPIKE_DIR/out/lcg-64k.bin" ] || sh "$SPIKE_DIR/gen-fixtures.sh" > /dev/null || return 1
  opt_build debug || return 1
  _nr=$(opt_runner debug) || return 1
  perl -e 'alarm 300; exec @ARGV' "$_nr" "$SPIKE_DIR/fixtures.manifest" "$SPIKE_DIR/out" > "$SPIKE_DIR/out/transcript.native-for-wasm.txt" 2> /dev/null || return 1
  # The guest sees the repository-independent guest paths /m (manifest) and /o (out directory).
  mkdir -p "$SPIKE_DIR/out/wasm-manifest" || return 1
  cp "$SPIKE_DIR/fixtures.manifest" "$SPIKE_DIR/out/wasm-manifest/fixtures.manifest" || return 1
  perl -e 'alarm 300; exec @ARGV' wasmtime run --dir "$SPIKE_DIR/out/wasm-manifest::/m" --dir "$SPIKE_DIR/out::/o" \
    "$_wd/wasm32-wasip1/release/runner.wasm" /m/fixtures.manifest /o > "$SPIKE_DIR/out/transcript.wasm.txt" 2> "$SPIKE_DIR/out/transcript.wasm.err" || true
  if cmp -s "$SPIKE_DIR/out/transcript.native-for-wasm.txt" "$SPIKE_DIR/out/transcript.wasm.txt"; then
    echo "WASM transcript-identical=yes"
  else
    echo "WASM transcript-identical=no"
    return 1
  fi
}
