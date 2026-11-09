---
status: proposed-default
blocks: Phase 2 (hard gate: language/build must be owner-recorded)
default-in-force: rust per the pre-registered score, provisional only; Phase 2 does not start until the owner records a choice
research-ref: SUMMARY #1
---

# 0001: Language and build system

## Context and gate

TOOL-01 asks for a language and build-system choice for the core, the loader and the headless runner, with measured results rather than a preference. Two skeletons, C11 with CMake and Ninja and Rust with Cargo, were built from one workload spec and run on one manifest (spike tree, `spikes/tool-01/`, preserved by the annotated tag `spike-tool-01-evidence`). The protocol was pre-registered before any spike file existed, and `scripts/protocol-order-check` enforces that order.

Protocol commit: aea02af6e6d17bc1933fbb3467e38eb063f9a031

The protocol is `docs/decisions/0001-language-build.protocol.md` (not a record, no front matter). It fixes the workload, the hard gates G1 to G5, the metric register M01 to M15, the criteria and weights, the rubric anchors and the decision rule. This record applies it. No amendment exists (`amendments=0`, `amended_after_spike=no`).

Under D-07 the owner pre-authorized a clear winner, and under D-37 (owner, 2026-11-09, "Stricter: worst-case >=10") that means: the leader passes every hard gate, leads by at least 10.00 points nominally, and still leads by 10.00 when its not-demonstrated items score 0 and the other option's score the maximum. `scripts/lib/score.awk` is the executable form of that rule. Where it does not pre-authorize, this record stays proposed-default and Phase 2 is blocked.

Bias controls (D-14, D-15):

- written first: C. An AI executor wrote both skeletons, C first and Rust second (LOG.md: `written-first: c`; Rust work started after the C option was complete). Authoring effort is therefore not a signal, and lines of glue count as review surface.
- Timebox: start 2026-11-09T21:53:09Z, timebox end 2026-11-10T05:53:09Z. Measurement finished 2026-11-09T23:00:29Z and this record was scored the same evening, inside the box.
- Not demonstrated (D-15): none. Every register row for both options has a measured or copied value, so no criterion is scored ND. Things that were not done are listed under "Limits and observations" below and none of them feeds a score as ND.

## Options considered

| Option | Skeleton | Total (points of 100.00) | Gates |
|--------|----------|--------------------------|-------|
| Rust (stable 1.99.1, edition 2024, Cargo workspace: `core` safe crate, `capi` facade, `runner`) | written second | 80.20 | G1 to G5 PASS |
| C11 (CMake 4.4.3 plus Ninja 1.13.2, Apple clang 21.0.0, `-Wall -Wextra -Wconversion -Werror`) | written first | 79.00 | G1 to G5 PASS |

Both options passed every hard gate and both runners produced byte-identical transcripts (an E0 repeatability check across languages, not a correctness oracle). The nominal margin is 1.20 points, so the pre-registered rule does not pre-authorize either option.

### Headline measurements (stable toolchain for every timing; medians in seconds, hyperfine 2.0.0, 10 runs after 1 warmup)

| Row | Rust debug | C debug | Rust release | C release |
|-----|-----------|---------|--------------|-----------|
| M02 clean build, warm toolchain | 0.722 | 0.766 | 0.889 | 0.703 |
| M03 incremental, comment-only edit | 0.453 | 0.224 | 0.863 | 0.217 |
| M04 incremental, one-line real edit | 0.802 | 0.194 | 0.779 | 0.199 |
| M05 test time | 0.118 | 0.035 | 0.102 | 0.036 |
| M06 warm gate time | 3.380 | 1.791 | 3.176 | 1.525 |
| M07 gate time, clean to green | 3.947 | 2.464 | 4.255 | 2.468 |

M01 true-cold first build: Rust 0.686 s, C 0.768 s (single first-ever run each, never repeated). Rust toolchain install: 36.6 s (rustup 16.1 s plus stable 1.99.1 20.5 s); C toolchain preinstalled. M09 lines of glue (ffi-glue + build + scripts): C 198, Rust 476; lines of core logic: C 109, Rust 148.

### Scores and the reason for every score

Each score is also in the scores file below. C2, C3 and C4 are computed by `spikes/tool-01/bench/derive-scores.awk` from the register formulas. C1, C5 and C6 are assigned from the anchors. No score sits between anchors, so no between-anchor reason is needed beyond the derivations here.

| Criterion (weight) | C11 | Rust | Reason, with evidence |
|--------------------|-----|------|-----------------------|
| C1 memory safety at the content boundary (25) | 3.0 | 5.0 | C: unchecked access is possible, but every buffer access is guarded by explicit bounds code, and M08 (ASan plus UBSan) and G5 pass: the 3.0 anchor. Rust: `core` and `runner` carry `#![forbid(unsafe_code)]`, `unsafe` is confined to the `capi` crate with a SAFETY comment on every block, and M08 (Miri on the facade) passes: the 5.0 anchor. |
| C2 fast feedback (20) | 5.0 | 3.8 | M04-debug: C 0.19373 s, Rust 0.80201 s, both under 2 s, so a tie, both 5.0. M06-debug: C 1.79103 s, Rust 3.38013 s, outside the tie band, so Rust scores floor(50 x 1.79103/3.38013) = 26 tenths = 2.6. Mean truncated: C 5.0, Rust 3.8. |
| C3 tools and dependency count (15) | 5.0 | 4.0 | T for C is 3 (cmake, ninja, Homebrew llvm@21). T for Rust is 4 (rustup, rust-toolchain 1.99.1, nightly-2026-11-09, cargo-fuzz). Excluded: Apple clang (the system cc, protocol baseline), wasmtime (used only for M14, which feeds C6), hyperfine (baseline). Rust scores 5.0 - (4 - 3) = 4.0. No third-party library is linked into either product artifact. |
| C4 ABI glue and facade simplicity (15) | 5.0 | 2.0 | G for C is 198 (38 + 15 + 145), for Rust 476 (231 + 69 + 176). Ratio 2.40 is outside the 1.20 band, so Rust scores floor(50 x 198/476) = 20 tenths = 2.0. |
| C5 fuzz and sanitizer ergonomics (15) | 4.0 | 4.0 | C: -1.0 for (c), the libFuzzer runtime archive is linked by explicit path from Homebrew llvm@21 because Apple clang ships none. (a) was not applied: fuzz compiles with the normal build's clang. Rust: -1.0 for (a), fuzzing needs the dated nightly, a toolchain other than the build's. For both: ASan present, liveness found in 0.4 s, M08 ran without fallback. |
| C6 portability and wasm (10) | 1.0 | 5.0 | C: wasm not attempted, install cost recorded (180,322,002-byte wasi-sdk 34.0 download), no container fallback: the 1.0 anchor. Rust: wasm32-wasip1 runs under wasmtime 50.0.1 and the transcript equals native: the 5.0 anchor. |

Rust planted-bug ergonomics observation (not a rubric subtraction, recorded as a cost): the first planted-bug form, `bytes.starts_with(b"DSPB")`, was not found by cargo-fuzz in 10 s or in a full 60 s run (3,694,953 executions, coverage stuck). The compiled code calls `bcmp`, and under the ASan runtime on macOS libFuzzer never learned the constant. The final form compares the 4-byte prefix as one little-endian integer and is found in about half a second. The trigger is still exactly "input of at least 4 bytes starting with DSPB". M13 (0.4 s) is measured on the final form and rubric item (d) is a time-to-find test, so no point was subtracted. This is a fact about prefix checks under cargo-fuzz on macOS ASan that a reviewer should weigh.

### Sensitivity of the leader to reasonable readings of the rubric

The 1.20-point margin is smaller than the effect of several judgement calls. Each variant below changes one reading and re-runs the scorer unchanged. Every variant stays proposed-default (`margin-below-10`), but the leader is not stable:

| Variant | C total | Rust total | Leader |
|---------|---------|------------|--------|
| As scored | 79.00 | 80.20 | Rust |
| wasmtime counted in Rust's T (T = 5, Rust C3 3.0) | 79.00 | 77.20 | C |
| Apple clang counted in C's T (T = 4 for both, both C3 5.0) | 79.00 | 83.20 | Rust |
| both of the two above (T c 4, T rust 5) | 79.00 | 80.20 | Rust |
| C5 item (a) applied to C as well (C C5 3.0) | 76.00 | 80.20 | Rust |
| C5 item (d) applied to Rust for the first planted-bug form (Rust C5 3.0) | 79.00 | 77.20 | C |
| wasmtime counted and (d) applied | 79.00 | 74.20 | C |

Choosing among these readings is not done silently here: the protocol text says T counts tools "needed for build, test, install, fuzz and sanitizer", which excludes wasmtime, and (d) tests the recorded M13 time, so the as-scored column follows the protocol literally. The point of the table is that a 1.20-point lead is inside the noise of those readings. It is the reason the owner is asked, not a reason to prefer either option.

## Decision

The language decision is open and Phase 2 is blocked.

The scorer's REASON is `margin-below-10`: both options pass every hard gate, Rust leads nominally by 1.20 points (80.20 against 79.00), and D-07 as confirmed by D-37 needs a lead of at least 10.00 points before a result stands in for an owner choice. The status is therefore `proposed-default`, not `owner-recorded`. The `default-in-force` names Rust only because the pre-registered score put it ahead; it is provisional, the STACK.md research ranking was not used, and the sensitivity table shows the leader can flip.

What the owner is asked to decide: Rust with Cargo, or C11 with CMake and Ninja, for the core, loader and headless runner. The trade the measurements show:

- Rust is ahead on memory safety at the content boundary (C1 5.0 against 3.0, with `forbid(unsafe_code)` on the core and Miri on the facade) and on wasm (it ran under wasmtime with an identical transcript; C did not attempt it).
- C is ahead on feedback time (gate about half of Rust's, debug incremental about 4x faster on a real edit), on glue (198 lines against 476) and on tool count (3 against 4), on a toy workload of one translation unit or crate.
- Fuzz ergonomics are level at 4.0 for different reasons: C needs a Homebrew-supplied libFuzzer archive on this Mac, Rust needs a dated nightly.

Until the owner states a choice, a later edit sets `status: owner-recorded`, quoting that statement verbatim with its date. Phase 2 stays blocked until then. No owner statement is recorded here.

## Consequences and what is not decided here

- Phase 2 inherits a clean start in the final layout. The spike tree is removed from main after the evidence tag exists, and Phase 2 may copy files from the tag with a provenance note (D-16).
- Toy-workload build and test times are not extrapolated to emulator scale: each skeleton has one translation unit or crate and no dependencies. Incremental and gate times at emulator scale are unmeasured, and the build-time ranking may not hold there.
- The 1.20-point margin, the fragile leader and the single-machine, single-session measurements (see below) are reasons to treat the scores as weak evidence. The measured gate results and facts (G1 to G5, M08, M11 to M15) are firmer than the totals.
- Not decided here: host and package format (decision 12) and whether a wasm target is promised (decision 13). The `.pc` plus static library installed-consumer proof passed for both options.
- Reopen if a Phase 2+ measurement contradicts a scored criterion by more than its tie band.

## Evidence and provenance

Evidence tag: `spike-tool-01-evidence` (annotated, local until plan 01-12 publishes it). It preserves the whole spike tree: both skeletons, `gates.sh`, the manifest, LOG.md, `bench/` including `derive-scores.awk`, the 24 raw hyperfine JSON files, `results.tsv`, `gates-c.txt`, `gates-rust.txt`, `scores.txt` and `score-output.txt`. The evidence paths cited in the table below resolve inside that tag (`git show spike-tool-01-evidence:<path>`).

### Results

One row per register row and option. The values are copied from `results.tsv`, written by plan 01-10 once, never re-run or dropped; every run's raw hyperfine JSON is in the tag. In the M10 rust row, one `|` in the source text ("curl|sh") is written "curl-sh" so the markdown table parses; nothing else is altered. `scripts/check-results --record` enforces that every register row has a value for both options.

<!-- results:begin -->
| Metric | Variant | Option | Value | Evidence |
|---|---|---|---|---|
| M01 | cold-first-build | c | seconds=0.768 | copied from LOG.md line M01 cold-first-build c (single first-ever opt_build debug, never re-measured) |
| M01 | toolchain-install | c | preinstalled | protocol M01 row: preinstalled for C |
| M02 | debug | c | median=0.76583 mean=0.78520 stddev=0.11689 min=0.67255 max=1.05690 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M02-c-debug.json |
| M02 | release | c | median=0.70303 mean=0.73408 stddev=0.05894 min=0.68184 max=0.85237 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M02-c-release.json |
| M03 | debug | c | median=0.22438 mean=0.24675 stddev=0.05243 min=0.19982 max=0.34000 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M03-c-debug.json |
| M03 | release | c | median=0.21722 mean=0.21526 stddev=0.00977 min=0.19424 max=0.22625 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M03-c-release.json |
| M04 | debug | c | median=0.19373 mean=0.19519 stddev=0.00936 min=0.18364 max=0.21065 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M04-c-debug.json |
| M04 | release | c | median=0.19877 mean=0.19928 stddev=0.01136 min=0.18111 max=0.21224 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M04-c-release.json |
| M05 | debug | c | median=0.03523 mean=0.03570 stddev=0.00197 min=0.03340 max=0.03949 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M05-c-debug.json |
| M05 | release | c | median=0.03582 mean=0.03479 stddev=0.00242 min=0.03098 max=0.03721 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M05-c-release.json |
| M06 | debug | c | median=1.79103 mean=1.76777 stddev=0.08497 min=1.57522 max=1.85446 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M06-c-debug.json |
| M06 | release | c | median=1.52524 mean=1.49076 stddev=0.15208 min=1.28789 max=1.72765 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M06-c-release.json |
| M07 | debug | c | median=2.46434 mean=2.54371 stddev=0.21320 min=2.33119 max=2.98182 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M07-c-debug.json |
| M07 | release | c | median=2.46817 mean=2.47902 stddev=0.15460 min=2.28758 max=2.72369 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M07-c-release.json |
| M08 | sanitizer | c | PASS | LOG.md line SANITIZER c (opt_sanitizer, plans 01-07 and 01-09) |
| M09 | core | c | 109 | awk count of non-blank non-comment lines over c/LINES.map category core |
| M09 | ffi-glue | c | 38 | awk count of non-blank non-comment lines over c/LINES.map category ffi-glue |
| M09 | build | c | 15 | awk count of non-blank non-comment lines over c/LINES.map category build |
| M09 | scripts | c | 145 | awk count of non-blank non-comment lines over c/LINES.map category scripts |
| M09 | runner | c | 311 | awk count of non-blank non-comment lines over c/LINES.map category runner |
| M10 | list | c | cmake 4.4.3 (preinstalled; build, install); ninja 1.13.2 (preinstalled; build); Apple clang 21.0.0 (preinstalled; build, sanitizer, fuzz compile); Homebrew llvm@21 21.1.8 (preinstalled; libFuzzer runtime archive by explicit path only, D-17); baseline excluded from T by the protocol: sh, awk, git, perl, pkgconf, system cc; measurement only: hyperfine 2.0.0 | LOG.md install lines and toolchain versions |
| M11 | G4 | c | PASS | spikes/tool-01/results/gates-c.txt (end-of-plan SLOW gate output, copied verbatim) |
| M12 | G5 | c | PASS elapsed=61.1 crashed=no execs_per_sec=39814 | spikes/tool-01/results/gates-c.txt (end-of-plan SLOW gate output, copied verbatim) |
| M13 | liveness | c | found_after=0.4 | spikes/tool-01/results/gates-c.txt (end-of-plan SLOW gate output, copied verbatim) |
| M14 | wasm | c | not attempted, install cost 180,322,002-byte wasi-sdk 34.0 download | LOG.md line WASM c |
| M15 | path-leak | c | leak=0 seeded-leak=10 | spikes/tool-01/results/gates-c.txt (end-of-plan SLOW gate output, copied verbatim) |
| M01 | cold-first-build | rust | seconds=0.686 | copied from LOG.md line M01 cold-first-build rust (single first-ever opt_build debug, never re-measured) |
| M01 | toolchain-install | rust | seconds=36.6 (rustup 16.1 s plus stable 1.99.1 20.5 s; rustup also fetched its default stable channel during the first sync, which is 1.99.1 as well) | copied from LOG.md line M01 toolchain-install rust and the install lines for rustup and 1.99.1 |
| M02 | debug | rust | median=0.72240 mean=0.71787 stddev=0.02578 min=0.67268 max=0.75022 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M02-rust-debug.json |
| M02 | release | rust | median=0.88911 mean=0.89928 stddev=0.02626 min=0.87813 max=0.96510 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M02-rust-release.json |
| M03 | debug | rust | median=0.45316 mean=0.47275 stddev=0.05702 min=0.42424 max=0.58407 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M03-rust-debug.json |
| M03 | release | rust | median=0.86287 mean=0.91048 stddev=0.13163 min=0.79256 max=1.15478 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M03-rust-release.json |
| M04 | debug | rust | median=0.80201 mean=0.84329 stddev=0.27557 min=0.58283 max=1.49011 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M04-rust-debug.json |
| M04 | release | rust | median=0.77937 mean=0.78288 stddev=0.03274 min=0.72277 max=0.82074 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M04-rust-release.json |
| M05 | debug | rust | median=0.11780 mean=0.15620 stddev=0.09694 min=0.10941 max=0.42364 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M05-rust-debug.json |
| M05 | release | rust | median=0.10185 mean=0.10153 stddev=0.00370 min=0.09689 max=0.10790 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M05-rust-release.json |
| M06 | debug | rust | median=3.38013 mean=3.43064 stddev=0.14427 min=3.27320 max=3.67873 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M06-rust-debug.json |
| M06 | release | rust | median=3.17626 mean=3.15102 stddev=0.22058 min=2.75808 max=3.42142 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M06-rust-release.json |
| M07 | debug | rust | median=3.94651 mean=3.94070 stddev=0.16282 min=3.68141 max=4.28334 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M07-rust-debug.json |
| M07 | release | rust | median=4.25537 mean=4.11360 stddev=0.44591 min=3.32231 max=4.77078 | hyperfine --warmup 1 --runs 10; raw JSON spikes/tool-01/results/M07-rust-release.json |
| M08 | sanitizer | rust | PASS | LOG.md line SANITIZER rust (opt_sanitizer, plans 01-07 and 01-09) |
| M09 | core | rust | 148 | awk count of non-blank non-comment lines over rust/LINES.map category core |
| M09 | ffi-glue | rust | 231 | awk count of non-blank non-comment lines over rust/LINES.map category ffi-glue |
| M09 | build | rust | 69 | awk count of non-blank non-comment lines over rust/LINES.map category build |
| M09 | scripts | rust | 176 | awk count of non-blank non-comment lines over rust/LINES.map category scripts |
| M09 | runner | rust | 181 | awk count of non-blank non-comment lines over rust/LINES.map category runner |
| M10 | list | rust | rustup 1.29.3 (install 16.1 s; licence Apache-2.0-OR-MIT; source homebrew-core formula rustup (brew install rustup; keg-only, bottled; never curl-sh)); rust-toolchain 1.99.1 (install 20.5 s; licence Apache-2.0-OR-MIT; source rustup toolchain install 1.99.1 --profile minimal -c rustfmt -c clippy -t wasm32-wasip1 (static.rust-lang.org via rustup, D-17/D-36)); nightly-2026-11-09 rustc-1.102.0-nightly-5b7b37aef (install 17.3 s; licence Apache-2.0-OR-MIT; source rustup toolchain install nightly-2026-11-09 --profile minimal -c miri -c rust-src (static.rust-lang.org via rustup, D-36; used only for cargo-fuzz and Miri; tools required (M10), not part of M01)); cargo-fuzz 0.13.2 (install 15.4 s; licence MIT-OR-Apache-2.0; source cargo +nightly-2026-11-09 install cargo-fuzz --version 0.13.2 --locked (crates.io; package-legitimacy check verdict OK, 170393 weekly downloads, repo rust-fuzz/cargo-fuzz)); wasmtime 50.0.1 (install 7.7 s; licence Apache-2.0-WITH-LLVM-exception; source homebrew-core formula wasmtime (brew install wasmtime, bottled; never curl-sh)); baseline excluded from T by the protocol: sh, awk, git, perl, pkgconf, system cc; measurement only: hyperfine 2.0.0 | LOG.md install lines and toolchain versions |
| M11 | G4 | rust | PASS | spikes/tool-01/results/gates-rust.txt (end-of-plan SLOW gate output, copied verbatim) |
| M12 | G5 | rust | PASS elapsed=61.2 crashed=no execs_per_sec=46895 | spikes/tool-01/results/gates-rust.txt (end-of-plan SLOW gate output, copied verbatim) |
| M13 | liveness | rust | found_after=0.4 | spikes/tool-01/results/gates-rust.txt (end-of-plan SLOW gate output, copied verbatim) |
| M14 | wasm | rust | transcript-identical=yes | LOG.md line WASM transcript-identical (opt_wasm, wasm32-wasip1 under wasmtime) |
| M15 | path-leak | rust | leak=0 seeded-leak=8 | spikes/tool-01/results/gates-rust.txt (end-of-plan SLOW gate output, copied verbatim) |
<!-- results:end -->

### Tools required (M10, D-36)

Tool count T for C3 uses the rows not marked baseline. The nightly toolchain is used only for cargo-fuzz and for Miri on the facade crate (D-36); no timing row in the results table used it, and its install time is listed here, not in M01.

| Tool | Version | Licence | Source | Used for | In T |
|------|---------|---------|--------|----------|------|
| CMake | 4.4.3 | BSD-3-Clause (STACK.md research) | preinstalled on this machine | C build and install | yes |
| Ninja | 1.13.2 | Apache-2.0 (STACK.md research) | preinstalled on this machine | C build | yes |
| Apple clang | 21.0.0 (clang-2100.1.1.101) | Xcode toolchain, LLVM-based | system cc, preinstalled | C build, ASan plus UBSan sanitizer, fuzz compile | no, baseline (system cc) |
| Homebrew llvm@21 | 21.1.8 | Apache-2.0 WITH LLVM-exception | Homebrew, preinstalled; never first on PATH (D-17) | libFuzzer runtime archive by explicit path, fuzz only | yes |
| rustup | 1.29.3 | Apache-2.0-OR-MIT | homebrew-core formula `rustup` (keg-only, bottled; never curl-pipe-sh); install 16.1 s | Rust toolchain manager | yes |
| Rust toolchain | 1.99.1 (stable, pinned by rust-toolchain.toml) | Apache-2.0-OR-MIT | `rustup toolchain install 1.99.1 --profile minimal -c rustfmt -c clippy -t wasm32-wasip1`; install 20.5 s | every Rust build, test, gate and timing row | yes |
| Rust nightly | nightly-2026-11-09 (rustc 1.102.0-nightly 5b7b37aef) | Apache-2.0-OR-MIT | `rustup toolchain install nightly-2026-11-09 --profile minimal -c miri -c rust-src`; install 17.3 s | cargo-fuzz (G5, M12, M13) and Miri on the facade (M08) only (D-36) | yes |
| cargo-fuzz | 0.13.2 | MIT-OR-Apache-2.0 | `cargo +nightly-2026-11-09 install cargo-fuzz --version 0.13.2 --locked` (crates.io; package-legitimacy check verdict OK, 170393 weekly downloads, repo rust-fuzz/cargo-fuzz); install 15.4 s | libFuzzer driver | yes |
| wasmtime | 50.0.1 | Apache-2.0-WITH-LLVM-exception | homebrew-core formula `wasmtime` (bottled); install 7.7 s | M14 wasm run only | no, M14 only |
| hyperfine | 2.0.0 | Apache-2.0-OR-MIT | homebrew-core formula (owner-approved D-12); install not timed | measurement only | no, baseline |

The Rust fuzz crate (outside the product workspace) pulls libfuzzer-sys 0.4.13 (licence (MIT OR Apache-2.0) AND NCSA) and eight small transitive crates, all in `rust/fuzz/Cargo.lock` only. The product `Cargo.lock` lists three names, all workspace members. Baseline tools excluded from T by the protocol: sh, awk, git, perl, pkgconf, the system cc, hyperfine.

### Fallbacks used

- C fuzz recipe (D-35): `/usr/bin/clang -g -O1 -fsanitize=fuzzer-no-link,address` linked with the Homebrew llvm@21 `libclang_rt.fuzzer_osx.a` by explicit path and `-lc++`. This is the fixed C recipe, not an unplanned fallback. It is the C5 item (c) cost: a machine without that Homebrew prefix would need the Linux-container fallback, which was not used. The run also needs `ASAN_OPTIONS=handle_sigtrap=1`: `__builtin_trap` raises SIGTRAP on arm64 and the ASan runtime ignores it by default, so without the variable the planted run died with no libFuzzer report. This is a runtime setting, not a toolchain change.
- Rust canary outcome: the 10 s ASan canary passed (`CANARY asan=ok`), so the `-s none` fallback was not needed. Fuzzing needs the second, dated nightly (C5 item (a)); no manual runtime-archive linking was needed. The planted-bug form had to change from `starts_with` to a single integer compare to be found (see the observation above).
- C wasm: not attempted. The install cost, a 180,322,002-byte wasi-sdk 34.0 download, is the recorded portability cost and scores C6 at the 1.0 anchor, not ND.
- Neither option used the Linux-container fallback, so no C6 subtraction applies.

### Limits and observations

- One machine, one session: Apple Silicon, macOS (Darwin 25.6.0), a shared host. The load average was 22.8 when the measurement run started (from the preceding `scripts/check`), so the first rows ran under some contention. No row was repeated to change a value.
- M13 liveness is run-to-run variable: C was found after 1.2 s in the first run and 0.4 s in the recorded second run; Rust was found after 0.4 s in both runs. The recorded rows are the end-of-plan runs.
- Fuzz executions per second are an indicator only. Sixty seconds is a harness check, not a bug-finding comparison.
- The Rust M04 debug run has an outlier (maximum 1.49 s, standard deviation 0.276 s); the median is used, as the protocol says, and M04-debug is a tie under the both-under-2-s rule either way.
- No seeded-UB control was run for Miri (the protocol does not require one). `run-all.sh` reads hyperfine 2.0.0's JSON by key order; it was cross-checked against all 24 raw files with an independent script (0 mismatches) and left as it ran.
- The M09 line counts exclude blank lines and comments by the protocol rule, and `LINES.map` assigns the C fuzz harness and the Rust fuzz target to the runner category, which does not feed any score.
- The matching hashes and byte-identical transcripts prove repeatability across two implementations of one spec. They do not prove correctness against an independent oracle.

### Scores file

The scorer input, `spikes/tool-01/results/scores.txt`, including the reasons as comments:

```text
# TOOL-01 scores, scorer input format (scripts/lib/score.awk). Written from spikes/tool-01/results/results.tsv, the gate
# outputs results/gates-c.txt and results/gates-rust.txt, and the pre-registered rubric (protocol section 5). Nothing is
# rescored after seeing the totals. No item is not demonstrated: every metric has a measured or copied value.
#
# Hard gates. G1 test-refusals, G2 test-controls (CONTROL-REJECTED), G3 test-interleave, G4 consumer-proof (LEAK 0 and
# the seeded leak caught), G5 fuzz60 (60 s, no crash) plus the planted-bug liveness control found.
gate c G1 PASS
gate c G2 PASS
gate c G3 PASS
gate c G4 PASS
gate c G5 PASS
gate rust G1 PASS
gate rust G2 PASS
gate rust G3 PASS
gate rust G4 PASS
gate rust G5 PASS
#
# C1 (anchor judgement). c 3.0: the C loader and core can make unchecked accesses; every buffer access is guarded by explicit
# bounds code (the length decides before memcpy of exactly 65536 bytes; digest_hex checks out_len); M08 (ASan+UBSan) and G5 pass.
# Exactly the 3.0 anchor. rust 5.0: core and runner carry #![forbid(unsafe_code)], unsafe is confined to the capi crate with a
# SAFETY comment on every block, and M08 (Miri on the facade) passes. Exactly the 5.0 anchor.
score c C1 3.0
score rust C1 5.0
#
# C2 (computed by derive-scores.awk). M04-debug c 0.19373 s, rust 0.80201 s: both under 2 s, a tie, both 5.0.
# M06-debug c 1.79103 s, rust 3.38013 s: outside the tie band, rust floor(50 x 1.79103/3.38013) = 26 tenths = 2.6.
# C2 = mean truncated: c 5.0, rust (5.0+2.6)/2 = 3.8.
score c C2 5.0
score rust C2 3.8
#
# C3 (computed). T c 3 (cmake, ninja, Homebrew llvm@21), T rust 4 (rustup, rust-toolchain 1.99.1, nightly-2026-11-09,
# cargo-fuzz). Excluded by the protocol baseline or scope: Apple clang (the system cc), wasmtime (M14 only), hyperfine.
# c 5.0, rust 5.0 - (4-3) = 4.0. No third-party library is linked into the product artifact.
score c C3 5.0
score rust C3 4.0
#
# C4 (computed). G c 198 (38+15+145), G rust 476 (231+69+176); ratio 2.40 is outside 1.20; c 5.0, rust floor(50 x 198/476) = 20 tenths.
score c C4 5.0
score rust C4 2.0
#
# C5 (rubric subtraction from 5.0, one point each). c: (c) manual runtime-archive linking is needed (Homebrew llvm@21 libFuzzer
# archive by explicit path) = -1.0; (a) not applied, because fuzz compiles with the normal build's clang and the archive is the
# (c) item; (b) ASan present; (d) liveness found in 0.4 s and 1.2 s; (e) M08 ran with plain Apple clang. c 4.0.
# rust: (a) fuzzing needs the dated nightly, a toolchain other than the build's = -1.0; (b) ASan present; (c) the 10 s canary
# passed, so no -s none fallback and no manual archive linking; (d) found in 0.4 s; (e) Miri ran, and the nightly for Miri is
# the D-36 design, not a fallback. rust 4.0. The Rust planted-bug ergonomics observation (a starts_with(b"DSPB") form was not
# found in 60 s and the single-integer prefix compare was needed; LOG.md) is recorded in the record as an ergonomics cost and
# is not a rubric subtraction, because M13 is measured on the final form (0.4 s) and (d) is a time-to-find test.
score c C5 4.0
score rust C5 4.0
#
# C6 (anchor judgement). c 1.0: wasm not attempted, install cost recorded (180,322,002-byte wasi-sdk 34.0 download); no Linux
# container fallback used. rust 5.0: wasm32-wasip1 runs under wasmtime 50.0.1 and the transcript equals native; no container fallback.
score c C6 1.0
score rust C6 5.0
#
# amendment flag: the amended_after_spike value printed by scripts/protocol-order-check (not typed by hand)
amendment no
```

### Scorer output

The output of `awk -f scripts/lib/score.awk spikes/tool-01/results/scores.txt`, saved as `spikes/tool-01/results/score-output.txt`. The record status follows the `RESULT=` line.

```text
TOTAL c 7900
TOTAL rust 8020
WORST c 7900
WORST rust 8020
MARGIN nominal 120 worst 120
LEADER rust
RESULT=proposed-default
REASON=margin-below-10
```
