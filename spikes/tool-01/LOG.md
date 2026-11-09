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
SANITIZER c PASS
M08 note c: opt_sanitizer builds core.c and runner.c directly with /usr/bin/clang -fsanitize=address,undefined -fno-sanitize-recover=undefined and runs the whole manifest; a sanitizer report on stderr or any failing case is SANITIZER FAIL.
real-edit toggle c: opt_edit real flips `inst->loaded = 1;` and `inst->loaded = 2;` in spikes/tool-01/c/src/core.c. The field is only compared with 0, so the code changes and the observable output must not; the hook compares the runner transcript (standalone compile, outside every build tree) before and after and fails if they differ.
end-of-plan slow gate c: second full run, LIVENESS found_after=0.4, FUZZ elapsed=61.1 crashed=no execs_per_sec=39814, GATES c RESULT PASS (the first run in Task 1 found the planted bug after 1.2 s; the M13 time-to-find is run-to-run variable and is reported with both samples).
c-work-end: 2026-11-09T22:10:48Z (C option complete: G1 to G5, M08 sanitizer, bench hooks, LINES.map)
rust-work-start: 2026-11-09T22:26:45Z (written second, D-14)
install rustup 1.29.3 16.1 Apache-2.0-OR-MIT homebrew-core formula rustup (brew install rustup; keg-only, bottled; never curl|sh)
install rust-toolchain 1.99.1 20.5 Apache-2.0-OR-MIT rustup toolchain install 1.99.1 --profile minimal -c rustfmt -c clippy -t wasm32-wasip1 (static.rust-lang.org via rustup, D-17/D-36)
M01 toolchain-install rust 36.6 (rustup 16.1 s plus stable 1.99.1 20.5 s; rustup also fetched its default stable channel during the first sync, which is 1.99.1 as well)
rustup -V: rustup 1.29.3 (2026-11-02)
TOOLCHAIN cargo /opt/homebrew/opt/rustup/bin/cargo cargo 1.99.1 (5f94df478 2026-08-27)
TOOLCHAIN rustc /opt/homebrew/opt/rustup/bin/rustc rustc 1.99.1 (58cf8f9c3 2026-10-13)
cargo -V: cargo 1.99.1 (5f94df478 2026-08-27)
rustc -V: rustc 1.99.1 (58cf8f9c3 2026-10-13)
M01 cold-first-build rust 0.686
M01 note rust: single run, perl -MTime::HiRes=time around opt_build debug (cargo build --workspace, core and capi only) with the build directory absent and rust/target absent, immediately after the stable 1.99.1 install; never repeated.
native-static-libs rust: -lSystem -lc -lm (printed by: cargo rustc -p duospool --release --crate-type staticlib -- --print native-static-libs; consumed by the G4 consumer build through pkgconf --static, as Libs.private in rust/duospool.pc.in; taken after the M01 build so as not to disturb it)
INSTALL-BUILD-TYPE rust release+debuginfo
rust-tracer: gates.sh rust --no-fuzz: build, header-identical, consumer-proof PASS (LEAK 0, SEEDED-LEAK 8); test-* FAIL as expected until the runner exists (Task 2)
rust-core-path-end: 2026-11-09T22:32:37Z (gates.sh rust --no-fuzz green: build, header-identical, test-refusals, test-controls, test-interleave, transcript, transcript-identical, consumer-proof; LEAK 0, SEEDED-LEAK 8; cargo clippy --workspace -- -D warnings and cargo fmt --check clean on 1.99.1; transcript.c.txt equals transcript.rust.txt byte for byte)
mutation check rust: changing 'len > MAX_SIZE' to 'len >= MAX_SIZE' in core turned test-refusals, test-controls and transcript-identical FAIL; restored with git checkout and gates went green again.
install nightly-2026-11-09 rustc-1.102.0-nightly-5b7b37aef 17.3 Apache-2.0-OR-MIT rustup toolchain install nightly-2026-11-09 --profile minimal -c miri -c rust-src (static.rust-lang.org via rustup, D-36; used only for cargo-fuzz and Miri; tools required (M10), not part of M01)
install cargo-fuzz 0.13.2 15.4 MIT-OR-Apache-2.0 cargo +nightly-2026-11-09 install cargo-fuzz --version 0.13.2 --locked (crates.io; package-legitimacy check verdict OK, 170393 weekly downloads, repo rust-fuzz/cargo-fuzz)
fuzz-crate dependencies rust: libfuzzer-sys 0.4.13 (licence (MIT OR Apache-2.0) AND NCSA) plus arbitrary 1.5.0, cc 1.11.1, jobserver 0.1.35, libc 0.2.191, shlex 2.0.1, find-msvc-tools 0.1.15, cfg-if 1.0.5, getrandom 0.4.3, r-efi 6.0.0, all in spikes/tool-01/rust/fuzz/Cargo.lock only; the product Cargo.lock still lists 3 names, all workspace members. libfuzzer-sys is not product (separate [workspace], T-01-21).
CANARY asan=ok
G5 recipe rust: cargo +nightly-2026-11-09 fuzz build/run with the default AddressSanitizer, the build tree, corpus and artifacts under spikes/tool-01/out/fuzz-rust-<planted yes|no>/ (not fuzz/corpus and fuzz/artifacts as the plan sketches, so the G4 source copy of rust/ never carries them; all gitignored). The 10 s ASan canary on an empty input finished normally, so no `-s none` fallback was needed. Portability and ergonomics cost: fuzzing needs a second, dated nightly toolchain (C5 (a)); no manual runtime-archive linking was needed.
LIVENESS found_after=0.4
FUZZ elapsed=61.2 crashed=no execs_per_sec=58597
G5 liveness note rust: the first planted-bug form, `bytes.starts_with(b"DSPB")`, was NOT found by cargo-fuzz in 10 s or in a full 60 s run (3,694,953 executions, cov stuck at 79). The compiled code calls bcmp, and under the ASan runtime on macOS libFuzzer never learned the constant. The final form compares the 4-byte prefix as one little-endian integer (0x4250_5344), which libFuzzer's compare tracing sees; it is found in about half a second. The trigger is still exactly "input of at least 4 bytes starting with DSPB", placed first in load. This is a finding about memcmp-style prefix checks under cargo-fuzz on macOS ASan, recorded as an ergonomics observation; it is not a spec deviation.
SANITIZER rust PASS
M08 note rust: opt_sanitizer runs `cargo +nightly-2026-11-09 miri test -p duospool` (D-36, facade crate only; the nightly's miri and rust-src components came from the install line above). The capi test module has 5 tests over the extern "C" entry points: create/load(65536-byte in-memory buffer)/step/time/digest_hex/destroy, null arguments and the null buffer, short and null digest buffers, refused lengths (including usize::MAX) with an 8-byte buffer that must never be read, and the two safe probes. All 5 passed under Miri with default isolation. First run including Miri's std setup 26.8 s wall; the test run itself 11.7 s. No seeded-UB control was run for Miri (not required by the plan).
install wasmtime 50.0.1 7.7 Apache-2.0-WITH-LLVM-exception homebrew-core formula wasmtime (brew install wasmtime, bottled; never curl|sh)
M14 command rust: cargo build --release -p runner --target wasm32-wasip1 (stable 1.99.1; the target came from rust-toolchain.toml in plan 01-08), then wasmtime run --dir spikes/tool-01/out/wasm-manifest::/m --dir spikes/tool-01/out::/o runner.wasm /m/fixtures.manifest /o (wasmtime 50.0.1; the manifest is copied alone into out/wasm-manifest so only it and out/ are preopened). Implemented as opt_wasm in rust/hooks.sh.
WASM transcript-identical=yes
WASM transcript note: the wasm transcript equals the native Rust transcript and also the C transcript byte for byte (E0 repeatability class: one implementation of one spec on two targets, not hardware correctness).
WASM c not attempted, install cost 180,322,002-byte wasi-sdk 34.0 download
real-edit toggle rust: opt_edit real flips `self.content.len() == CONTENT_SIZE` and `self.content.len() >= CONTENT_SIZE` in is_loaded() in spikes/tool-01/rust/core/src/lib.rs. The content vector is only ever empty or exactly CONTENT_SIZE long, so the code changes and the observable output must not; the hook builds the workspace into a dedicated tree (out/edit-check-rust, outside every measured build tree) before and after and fails if the runner transcripts differ. All Rust hooks use the pinned stable 1.99.1; `+nightly` appears only in opt_fuzz60 and opt_sanitizer.
end-of-plan slow gate rust: sh spikes/tool-01/gates.sh rust, run from cold build trees: LIVENESS found_after=0.4, FUZZ elapsed=61.2 crashed=no execs_per_sec=46895, GATES rust RESULT PASS (the earlier Task 1 run: found_after=0.4, execs_per_sec=58597). Saved as out/gates-rust-full.txt.
rust-work-end: 2026-11-09T22:45:30Z (Rust option complete: G1 to G5, M08 Miri, M14 wasm, bench hooks, LINES.map)
