#!/bin/sh
# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# hooks.sh - the C option's implementation of the shared gates.sh hook contract (TOOL-01 spike). Sourced, never executed.
# The caller (gates.sh or a bench script) sets SPIKE_DIR (spikes/tool-01) and OPT_DIR (spikes/tool-01/c) first.
# Build trees live under SPIKE_DIR/out/, which is gitignored.

c_build_type() {
  case $1 in
    debug) echo Debug ;;
    release) echo Release ;;
    *) echo "hooks: unknown profile $1" >&2; return 1 ;;
  esac
}

# opt_build <debug|release>: configure when needed, then build everything (library and runner).
opt_build() {
  _p=${1:?profile}
  _t=$(c_build_type "$_p") || return 1
  _bd=$SPIKE_DIR/out/build-c-$_p
  _log=$SPIKE_DIR/out/build-c-$_p.log
  mkdir -p "$SPIKE_DIR/out" || return 1
  : > "$_log"
  if [ ! -f "$_bd/CMakeCache.txt" ]; then
    cmake -S "$OPT_DIR" -B "$_bd" -G Ninja "-DCMAKE_BUILD_TYPE=$_t" >> "$_log" 2>&1 || { cat "$_log" >&2; return 1; }
  fi
  cmake --build "$_bd" >> "$_log" 2>&1 || { cat "$_log" >&2; return 1; }
}

# opt_install <src_copy> <build_dir> <prefix> <remap yes|no>
# Builds the library from the given source copy and installs lib/libduospool.a, include/duospool.h and
# lib/pkgconfig/duospool.pc under prefix. With remap yes the compile gets -ffile-prefix-map / -fdebug-prefix-map for both
# spellings of the copy and build paths (macOS /var is a symlink to /private/var, Pitfall 12); with remap no it does not
# (the seeded-leak control).
opt_install() {
  _src=${1:?src_copy}
  _bd=${2:?build_dir}
  _prefix=${3:?prefix}
  _remap=${4:?remap}
  mkdir -p "$_bd" "$_prefix" || return 1
  _srcp=$(cd "$_src" && pwd -P) || return 1
  _bdp=$(cd "$_bd" && pwd -P) || return 1
  # RelWithDebInfo is a debug-info build type on purpose: with it the installed archive carries DWARF paths, so the
  # path-leak grep inspects an archive that can leak. A plain Release archive would pass the grep vacuously.
  set -- "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
  if [ "$_remap" = yes ]; then
    set -- "$@" "-DCMAKE_C_FLAGS=-ffile-prefix-map=$_src=/src -ffile-prefix-map=$_srcp=/src -ffile-prefix-map=$_bd=/build -ffile-prefix-map=$_bdp=/build -fdebug-prefix-map=$_src=/src -fdebug-prefix-map=$_srcp=/src -fdebug-prefix-map=$_bd=/build -fdebug-prefix-map=$_bdp=/build"
  fi
  _ilog=$_bd.install.log
  {
    cmake -S "$_src" -B "$_bd" -G Ninja "$@" &&
      cmake --build "$_bd" --target duospool &&
      cmake --install "$_bd" --prefix "$_prefix"
  } > "$_ilog" 2>&1 || { cat "$_ilog" >&2; return 1; }
  if ! grep -qx 'INSTALL-BUILD-TYPE c RelWithDebInfo' "$SPIKE_DIR/LOG.md" 2> /dev/null; then
    echo 'INSTALL-BUILD-TYPE c RelWithDebInfo' >> "$SPIKE_DIR/LOG.md"
  fi
}
