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
c-core-path-end: 2026-11-09T21:58:44Z (gates.sh c --no-fuzz green: build, test-refusals, test-controls, test-interleave, transcript, consumer-proof)
LIVENESS found_after=1.2
FUZZ elapsed=61.1 crashed=no execs_per_sec=29301
G5 recipe c: D-35 explicit-path link, no fallback needed. /usr/bin/clang -g -O1 -fsanitize=fuzzer-no-link,address, linked with /opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/libclang_rt.fuzzer_osx.a -lc++ (llvm@21 never on PATH, D-17). The same two-stage run (planted liveness, then clean 60 s) is the saved spikes/tool-01/out/gates-c-full.txt, which ends GATES c RESULT PASS.
G5 recipe note c: the run sets ASAN_OPTIONS=handle_sigtrap=1. __builtin_trap raises SIGTRAP on arm64 and the ASan runtime ignores it by default, so without the variable the planted run died with no libFuzzer report and no crash artifact (the process was killed, still crashed=yes). With it libFuzzer prints the ASan TRAP report at core.c and writes a crash artifact whose input contains DSPB. This is a runtime environment setting, not a toolchain change; the protocol is unchanged.
G5 portability cost c: the fuzz runtime is a Homebrew llvm@21 archive on an explicit absolute path, because Apple clang 21 ships no libFuzzer runtime; a machine without that Homebrew prefix needs the Linux-container fallback (not used here).
