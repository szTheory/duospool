# TOOL-01 spike log (append-only)

SPDX-License-Identifier: MIT OR Apache-2.0
SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors

Protocol: docs/decisions/0001-language-build.protocol.md (pre-registered; weights confirmed unchanged by the owner 2026-11-09).

start: 2026-11-09T21:53:09Z
timebox-end: 2026-11-10T05:53:09Z (start + 8 h, D-15)
written-first: c

## Toolchain versions (D-17)

cmake --version:
    cmake version 4.4.3

    CMake suite maintained and supported by Kitware (kitware.com/cmake).
ninja --version:
    1.13.2
/usr/bin/clang --version:
    Apple clang version 21.0.0 (clang-2100.1.1.101)
    Target: arm64-apple-darwin25.6.0
    Thread model: posix
    InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
/opt/homebrew/opt/llvm@21/bin/clang --version:
    Homebrew clang version 21.1.8
    Target: arm64-apple-darwin25.6.0
    Thread model: posix
    InstalledDir: /opt/homebrew/Cellar/llvm@21/21.1.8/bin
    Configuration file: /opt/homebrew/etc/clang/arm64-apple-darwin25.cfg

## Measurements and events

M01 cold-first-build c 0.768
INSTALL-BUILD-TYPE c RelWithDebInfo
M01 note: single run, perl -MTime::HiRes=time around opt_build debug with the build directory absent; library and install targets only; never repeated.
