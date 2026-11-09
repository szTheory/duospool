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
