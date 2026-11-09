---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
plan: 08
subsystem: testing
tags: [tool-01, spike, rust, cargo, rustup, extern-c, gates, installed-consumer, forbid-unsafe]
status: complete

requires:
  - phase: 01-06
    provides: "shared gates.sh harness, hook contract, canonical duospool.h, independent consumer, manifest, C transcript"
  - phase: 01-07
    provides: "complete C option (G5, sanitizer, bench hooks) so the Rust option is written second (D-14)"
provides:
  - "spikes/tool-01/rust/: self-contained Cargo workspace (core, capi, runner), rust-toolchain.toml pin to stable 1.99.1, Cargo.lock listing only workspace members"
  - "duospool_core: safe zero-dependency core with forbid(unsafe_code); duospool (capi): extern C facade, the only hand-written unsafe, with a safe probe module; runner: safe manifest runner with forbid(unsafe_code)"
  - "rust/hooks.sh: opt_build, opt_install, opt_runner (no nightly anywhere); rust/duospool.pc.in with Libs.private from native-static-libs"
  - "gates.sh header-identical stage (W-10) and transcript.rust.txt byte-identical to transcript.c.txt"
  - "LOG.md: rust-work-start, install rustup and rust-toolchain lines, TOOLCHAIN cargo/rustc lines, M01 toolchain-install and cold-first-build, native-static-libs, rust-core-path-end"
affects: [01-09, 01-10, 01-11]

tech-stack:
  added: [rustup 1.29.3 (Homebrew), rust stable 1.99.1]
  patterns:
    - "Unsafe confined to capi; the runner reaches C-boundary-only cases through safe probe functions exported by capi"
    - "capi decides length refusals from the length alone (core::length_status) before it forms any slice, so a refused length never reads the caller's buffer"
    - "Cargo build trees live under the gitignored out/ (out/build-rust-<profile>), mirroring the C option, so the G4 source-copy stays small"

key-files:
  created:
    - spikes/tool-01/rust/rust-toolchain.toml
    - spikes/tool-01/rust/Cargo.toml
    - spikes/tool-01/rust/Cargo.lock
    - spikes/tool-01/rust/core/Cargo.toml
    - spikes/tool-01/rust/core/src/lib.rs
    - spikes/tool-01/rust/capi/Cargo.toml
    - spikes/tool-01/rust/capi/src/lib.rs
    - spikes/tool-01/rust/runner/Cargo.toml
    - spikes/tool-01/rust/runner/src/main.rs
    - spikes/tool-01/rust/include/duospool.h
    - spikes/tool-01/rust/duospool.pc.in
    - spikes/tool-01/rust/hooks.sh
  modified:
    - spikes/tool-01/gates.sh
    - spikes/tool-01/LOG.md

key-decisions:
  - "Build trees use CARGO_TARGET_DIR=spikes/tool-01/out/build-rust-<profile> rather than spikes/tool-01/rust/target, so the gates.sh G4 copy of the option directory never carries a target tree; opt_install additionally removes any target/ in its throwaway copy"
  - "The installed release build sets CARGO_PROFILE_RELEASE_DEBUG=2 (logged INSTALL-BUILD-TYPE rust release+debuginfo), the same rationale as the C option's RelWithDebInfo: the leak grep must inspect an archive that can leak"
  - "Rust Box allocation failure aborts, whereas the C core returns DS_ERR_STATE; the 64 KiB content buffer is allocated with try_reserve_exact at load and maps failure to ERR_STATE, only the small Instance box inside create can abort"

patterns-established:
  - "hooks.sh puts the keg-only rustup bin directory on PATH itself and never edits a shell profile"

requirements-completed: []  # TOOL-01 is advanced here (Rust skeleton core path); it completes with the language decision record

actuals:
  tokens: 8826    # chars/4 over the realized git diff of the two code commits (35303 chars); this SUMMARY and the state commit are not included
  tasks: 2
  commits: 2
plan_head_before: cad553d8583cbf0e516d62337d2927d6909ca08d
plan_head_after: 9ab861564611cc590f4b5f9040e03597347000f4

coverage:
  - id: D1
    description: "sh spikes/tool-01/gates.sh rust --no-fuzz ends GATES rust RESULT PASS-NO-FUZZ with build, header-identical, G1, G2, G3, transcript, transcript-identical and G4 consumer-proof all PASS"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "sh spikes/tool-01/gates.sh rust --no-fuzz (about 3.5 s warm); saved out/gates-rust-t2.txt; LEAK 0 and SEEDED-LEAK 8"
        status: pass
    human_judgment: false
  - id: D2
    description: "rust/include/duospool.h is byte-identical to c/include/duospool.h and the Rust transcript is byte-identical to the C transcript (E0 repeatability class, not a correctness oracle)"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "cmp rust/include/duospool.h c/include/duospool.h; cmp out/transcript.c.txt out/transcript.rust.txt; GATE header-identical PASS, GATE transcript-identical PASS"
        status: pass
    human_judgment: false
  - id: D3
    description: "Unsafe is confined to the capi facade: core and runner carry forbid(unsafe_code), core has zero dependencies, every capi unsafe block has a SAFETY comment, panic=abort in dev and release"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "grep unsafe on core and runner prints only the forbid line; capi has 15 unsafe blocks and 15 SAFETY comments; Cargo.lock has 3 name entries, all workspace members; grep -c nightly rust/hooks.sh prints 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The gates are live: a length-check mutation in the Rust core turns test-refusals, test-controls and transcript-identical red; restoring it turns them green"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "len > MAX_SIZE changed to len >= MAX_SIZE, gates.sh rust --no-fuzz FAIL on those three stages, git checkout, PASS-NO-FUZZ again"
        status: pass
    human_judgment: false
  - id: D5
    description: "Toolchain precondition and install evidence: TOOLCHAIN cargo and rustc lines (both 1.99.1, rustup shim paths) precede the M01 cold-first-build line; install rustup and install rust-toolchain 1.99.1 lines with times"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "LOG.md lines 49-57: install rustup 1.29.3 16.1 s, install rust-toolchain 1.99.1 20.5 s, TOOLCHAIN cargo/rustc 1.99.1, M01 cold-first-build rust 0.686"
        status: pass
    human_judgment: false
  - id: D6
    description: "clippy and rustfmt strictness on stable 1.99.1; the repository fast gate still passes"
    requirement: TOOL-01
    verification:
      - kind: integration
        ref: "cargo clippy --workspace -- -D warnings and cargo fmt --check exit 0 in spikes/tool-01/rust; sh scripts/check ends CHECK RESULT PASS"
        status: pass
    human_judgment: false

duration: ~25 min
completed: 2026-11-09
---

# Phase 1 Plan 08: Rust skeleton core path, G1 to G4 and byte-identical transcript Summary

**The Rust/Cargo skeleton on stable 1.99.1 passes hard gates G1 to G4 against the same manifest, consumer and harness as C: a safe zero-dependency core behind an `extern "C"` facade reproduces the independent golden line through an installed prefix, and its transcript is byte-identical to the C option's.**

## Performance

- **Duration:** about 25 minutes (rust-work-start 22:26:45Z; most of the wall time was `scripts/check`, which took several minutes)
- **Tasks:** 2 (1 tracer, 1 auto), two code commits
- **Files:** 12 created under `spikes/tool-01/rust/`, 2 modified (`gates.sh`, `LOG.md`); 789 insertions

## Accomplishments

- **Toolchain and measurements.** `brew install rustup` took 16.1 s and `rustup toolchain install 1.99.1 --profile minimal -c rustfmt -c clippy -t wasm32-wasip1` took 20.5 s (M01 toolchain-install 36.6 s total). Before any build, `TOOLCHAIN cargo` and `TOOLCHAIN rustc` lines show `/opt/homebrew/opt/rustup/bin/{cargo,rustc}` reporting 1.99.1 from the `rust-toolchain.toml` pin. **M01 cold-first-build rust = 0.686 s** (core and capi only, `rust/target` absent, taken once, never repeated). The value is small because the toy workload is two tiny crates with no dependencies; it is not extrapolated.
- **Tracer first.** Task 1 crossed core, capi, the installed prefix and the independent C consumer to the golden line before the runner existed: build, header-identical and consumer-proof PASS, with the test stages failing for the stated reason (opt_runner not defined) as the plan expects.
- **G4 is live for Rust.** The consumer builds only against the installed `.a`, header and `.pc` through `pkgconf --static`, which consumes `Libs.private: -lSystem -lc -lm` (printed by `cargo rustc -p duospool --release --crate-type staticlib -- --print native-static-libs`). The header-removed control breaks the build, the remapped archive and `.pc` have `LEAK 0` with prefixes for the copy, build dir (both spellings), `~/.cargo` and the rustc sysroot, and the unremapped build gives `SEEDED-LEAK 8`. No extra remap prefix was needed and the grep was not weakened.
- **Unsafe confinement.** `duospool_core` and `runner` carry `#![forbid(unsafe_code)]`; the core has no dependencies. All 15 unsafe blocks live in capi, each with a `// SAFETY:` comment, and the runner reaches the null-buffer and short-buffer cases through the safe `probe_load_null` and `probe_digest_short`. `panic = "abort"` is set for dev and release.
- **W-05 "no byte read for a refused length" preserved across the ABI.** capi calls `duospool_core::length_status` on the raw length first and forms a slice only for the exact 65536-byte length, so a refused length (including 1048577 or `SIZE_MAX`) never builds a slice over a possibly shorter caller buffer.
- **Gates are live.** Mutating `len > MAX_SIZE` to `len >= MAX_SIZE` in the Rust core turned test-refusals, test-controls and transcript-identical red; `git checkout` restored it and the run went green.
- **Identical to C.** `cmp out/transcript.c.txt out/transcript.rust.txt` exits 0 (E0 class: two implementations of one spec agree, not hardware correctness). `gates.sh c --no-fuzz` still ends `PASS-NO-FUZZ` with the new `header-identical` stage. A warm `gates.sh rust --no-fuzz` takes about 3.5 s.
- `cargo clippy --workspace -- -D warnings` and `cargo fmt --check` are clean on stable 1.99.1. `sh scripts/check` ends `CHECK RESULT PASS`.

## Task Commits

1. **Task 1 (tracer): Rust core and capi through installed prefix to the shared consumer golden line** - `c90c8ea` (feat)
2. **Task 2: safe Rust manifest runner, G1-G3, byte-identical C transcript and full G4 controls** - `9ab8615` (feat)

## Deviations from Plan

### Plan details not followed literally

- **Build trees under `out/`.** The plan text sketches `target/debug/runner`; hooks.sh uses `CARGO_TARGET_DIR=spikes/tool-01/out/build-rust-<profile>` (so the runner is `out/build-rust-<profile>/<profile>/runner`), mirroring the C option. This keeps the gates.sh G4 copy of the option directory from carrying a target tree. The M01 precondition (`rust/target` absent) holds either way. Plan 01-09 builds its hooks on this hooks.sh, so `opt_clean` and friends must remove those directories.
- **Release install with debug info.** `opt_install` sets `CARGO_PROFILE_RELEASE_DEBUG=2`, recorded as `INSTALL-BUILD-TYPE rust release+debuginfo`, so the path-leak grep inspects an archive that can carry paths (the C option's RelWithDebInfo precedent). The grep, needles and seeded-leak control are unchanged.
- **`.pc` finished after M01.** The plan lists the `.pc` before the M01 build, but the native-static-libs flags can only be printed by a cargo build, and M01 must be the first cargo build. The `.pc` therefore carried an empty `Libs.private:` during the M01 build, and the flags were added after it (logged in LOG.md). No effect on the M01 scope (core and capi library plus install).
- **Allocation failure.** Rust's default allocation failure aborts, where C returns `DS_ERR_STATE`. The content buffer is allocated with `try_reserve_exact` and a failure maps to `ERR_STATE`; only the small `Box<Handle>` in `duospool_create` can abort. A spec deviation of this kind is reported here and scored as written, not tuned away.
- `cargo fmt --check` produced no diff on the first run; `rustfmt` was not needed to rewrite any file.

### Auto-fixed Issues

**1. [Rule 1 - Bug] Trailing space dropped from `Libs.private: ` by the file writer**
- **Found during:** Task 1 (od of the .pc after the substitution did not apply)
- **Issue:** the placeholder line `Libs.private: ` lost its trailing space, so the planned `perl` substitution did not match and the acceptance grep `^Libs.private: ` would have failed.
- **Fix:** replaced the line with `Libs.private: -lSystem -lc -lm` via an exact edit before the first gates run.
- **Files modified:** spikes/tool-01/rust/duospool.pc.in
- **Commit:** c90c8ea

**Total deviations:** 1 auto-fixed (1 bug) plus the plan-detail notes above. **Impact:** none on the gates or scoring; the protocol was not edited and no amendment was needed.

## Issues Encountered

- `sh scripts/check` took several minutes (the protocol and scorer tests dominate, as in plan 01-07); it passed and did not stall beyond 3 minutes of silence.
- rustup fetched its default `stable` channel as part of the first sync, which is also 1.99.1, so `~/.rustup` is about 1.9 GB (two toolchain directories). Builds select 1.99.1 through `rust-toolchain.toml`.
- No Known Stubs. No new security-relevant surface beyond the plan's threat model (T-01-19 mitigations are implemented and checked by the acceptance counts; T-01-20 by `LEAK 0` and the seeded control; T-01-SC by the Homebrew formula install with no crates fetched).

## Next Phase Readiness

- Plan 01-09 can add the Rust fuzz gate, Miri, wasm, `LINES.map` and the bench hooks (`opt_fuzz60`, `opt_sanitizer`, `opt_clean`, `opt_edit`, `opt_restore`, `opt_test`) to `spikes/tool-01/rust/hooks.sh`; build trees are `spikes/tool-01/out/build-rust-{debug,release}` and the runner is `opt_runner [debug|release]`.
- The nightly 2026-11-09 install, cargo-fuzz and wasmtime remain for 01-09 (D-36); nothing in this plan used nightly.
- The timebox started 2026-11-09T21:53:09Z and ends 2026-11-10T05:53:09Z; the Rust core path ended well inside it (rust-core-path-end is in LOG.md).

## Self-Check: PASSED

- All 12 files in key-files.created exist on disk and are tracked (`git ls-files spikes/tool-01/rust` lists 12 entries); `git ls-files spikes/tool-01/out` prints nothing.
- Commits `c90c8ea` and `9ab8615` are ancestors of HEAD.
- Plan acceptance criteria and `<verification>` commands were re-run: gates rust PASS-NO-FUZZ, transcripts identical, clippy and fmt clean, `scripts/check` PASS.
