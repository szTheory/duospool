# Protocol for the TOOL-01 language and build-system spike

## (1) Status

This file pre-registers the TOOL-01 spike under D-09. It is committed before any file under `spikes/` exists in history, and `scripts/protocol-order-check` enforces that order from now on (PROTOCOL_AFTER_SPIKE). The scorer named in section (6), `scripts/lib/score.awk`, is held to the same order (SCORER_AFTER_SPIKE).

Changes after this commit are dated `## Amendment YYYY-MM-DD: <reason>` sections appended at the end of the file. In-place edits of text above the first amendment are forbidden, and `scripts/protocol-order-check` fails them with PROTOCOL_EDITED. An amendment dated after the first commit under `spikes/` makes record 0001 proposed-default (REASON=amended-after-spike). This last rule is Claude's design under D-09; the owner may amend it.

The weights, rubric anchors and metrics below are Claude-proposed (D-13). The owner may amend them before the spike runs, as a dated amendment. No blocking checkpoint is added for this.

This file is not a decision record. It carries no front matter and has no row in the decision index.

## (2) Workload spec

Both options (C and Rust) build exactly the same things from this one spec (D-10). The numbered requirements are W-01 to W-12.

- W-01: an opaque instance with create, step and destroy.
- W-02: per-instance state only, with no mutable globals or statics.
- W-03: every entry point returns a status. The enum values are fixed: DS_OK=0, DS_REFUSE_EMPTY=1, DS_REFUSE_TRUNCATED=2, DS_REFUSE_BAD_SIZE=3, DS_REFUSE_OVERSIZED=4, DS_REFUSE_HEADER=5, DS_ERR_NULL_ARG=6, DS_ERR_SHORT_BUFFER=7, DS_ERR_STATE=8.
- W-04: the C API is exactly:
  - `uint32_t duospool_abi_version(void)`, which returns 1
  - `ds_status duospool_create(ds_instance **out)`
  - `ds_status duospool_load(ds_instance *inst, const uint8_t *buf, size_t len)`
  - `ds_status duospool_step(ds_instance *inst, uint32_t cycles)`
  - `ds_status duospool_time(const ds_instance *inst, uint64_t *out)`
  - `ds_status duospool_digest_hex(const ds_instance *inst, char *out, size_t out_len)`, which needs out_len of at least 17 and writes 16 lowercase hex digits plus NUL
  - `void duospool_destroy(ds_instance *inst)`, which accepts NULL
- W-05: loader decision order:
  - buf NULL gives DS_ERR_NULL_ARG (inst NULL also gives DS_ERR_NULL_ARG)
  - len 0 gives DS_REFUSE_EMPTY
  - len above 1048576 gives DS_REFUSE_OVERSIZED, without reading any byte
  - len exactly 66048 gives DS_REFUSE_HEADER (never stripped)
  - len below 65536 gives DS_REFUSE_TRUNCATED
  - any len other than 65536 gives DS_REFUSE_BAD_SIZE
  - a second load on an already-loaded instance gives DS_ERR_STATE
  - otherwise the 65536 bytes are copied and the result is DS_OK

  Every refusal leaves time and digest unchanged.
- W-06: step on an unloaded instance gives DS_ERR_STATE. Otherwise step adds cycles to a u64 machine time and folds the 8 little-endian bytes of the new time into the digest.
- W-07: the digest is FNV-1a 64 (offset basis 0xcbf29ce484222325, prime 0x100000001b3), initialised at create and folded over all 65536 content bytes on a successful load.
- W-08: the manifest and runner, described below.
- W-09: interleave. Instances A and B both load lcg-64k.bin, then step the cycle list 1, 7, 64, 1000 repeated 100 times, alternating A and B per step. Both final digests must equal a solo instance's digest for the same list.
- W-10: one hand-written `duospool.h`, byte-identical in both skeletons.
- W-11: the C consumer `spikes/tool-01/consumer/main.c` takes a fixture path as argv[1]. It calls create, load, step 100, step 200, step 300, time, digest_hex and destroy, and prints exactly `abi=1 load=OK time=600 digest=<16 hex>`. The expected line comes from an independent core-perl implementation of W-07 (`consumer/expect.pl`), not from either skeleton.
- W-12: `spikes/tool-01/gen-fixtures.sh` generates every fixture into `spikes/tool-01/out/` (gitignored, D-35) using an integer LCG with seed 12345: x = (x*1664525 + 1013904223) mod 4294967296, byte = int(x/65536) mod 256, emitted as octal escapes. It never uses awk rand() and never reads real ROM data. It verifies each file's sha256 against the manifest.

### Fixture list (W-12)

- zero-64k.bin: 65536 zero bytes
- lcg-64k.bin: the first 65536 LCG bytes (sha256 prefix ab0b5936cf56fca6)
- empty.bin: 0 bytes
- trunc-65535.bin: the first 65535 bytes of lcg-64k
- hdr-66048.bin: 512 zero bytes followed by lcg-64k
- bad-98304.bin: the first 98304 LCG bytes
- cap-1048576.bin: zeros
- over-1048577.bin: zeros

### Manifest format (W-08)

`spikes/tool-01/fixtures.manifest` is shared byte-identically by both options. Lines starting with `#` are comments. There are two row kinds:

- `fixture <name> <size> <sha256>`
- `case <id> <kind> <arg> <expect> [control]`

Case kinds: `load` (arg = fixture), `api-null`, `api-short`, `refuse-keeps-state` (load lcg-64k, attempt hdr-66048, digest unchanged gives UNCHANGED), `step-digest` (load the arg, step the W-11 list, expect `digest:<hex>`), and `interleave` (expect EQUAL).

Cases:

- accept-zero: OK
- accept-lcg: OK
- refuse-empty: REFUSE_EMPTY
- refuse-trunc: REFUSE_TRUNCATED
- refuse-header: REFUSE_HEADER
- refuse-badsize: REFUSE_BAD_SIZE
- refuse-cap: cap-1048576 gives REFUSE_BAD_SIZE (boundary)
- refuse-over: over-1048577 gives REFUSE_OVERSIZED (boundary)
- refuse-null: ERR_NULL_ARG
- refuse-short: ERR_SHORT_BUFFER with out_len 16
- refuse-keeps-state: UNCHANGED
- step-digest: lcg-64k with the expect.pl digest
- interleave: EQUAL
- ctl-wrong-expect: load lcg-64k expecting REFUSE_EMPTY, marked control

The runner prints one transcript line per case, `<id> <observed> PASS|FAIL|CONTROL-REJECTED|CONTROL-BROKEN`, then `RUNNER pass=<n> fail=<m> controls_rejected=<k>`. A control whose expectation matches is CONTROL-BROKEN, and the runner exits non-zero. Both options must emit byte-identical transcripts. That is a cross-language repeatability check (evidence class E0), not a correctness oracle.

## (3) Hard gates

Hard gates are pass or fail (D-11). A leader that fails any gate cannot be pre-authorized.

- G1: refusal matrix. Every non-control case passes.
- G2: the control is reported CONTROL-REJECTED.
- G3: interleave is EQUAL.
- G4: installed-consumer proof. Build and install from a throwaway copy to a prefix outside it. Run `chmod -R a-rwx` on the copy and build directory, restoring them in a trap. Build the consumer from a separate directory with `PKG_CONFIG_LIBDIR=<prefix>/lib/pkgconfig cc $(pkgconf --static --cflags --libs duospool) main.c`. The `--static` flag makes the build consume `Libs.private`, where the Rust `.pc` carries its native-static-libs flags. Its output must equal expect.pl's line. Removing `include/duospool.h` from the prefix must break the consumer build. A `grep -c -a` of the `.a` and `.pc` for the work dir, its `pwd -P` spelling and `$HOME` must be 0. A deliberately leaky build, without path remapping, must be caught by that grep.
- G5: a 60-second libFuzzer run on the loader without a crash, plus a liveness control: a planted-bug build that traps on the input prefix `DSPB` must be found within 60 s.

The machine-specific fuzz recipes are fixed here from D-35:

- C: Apple clang `-fsanitize=fuzzer-no-link,address`, linked with `/opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/libclang_rt.fuzzer_osx.a -lc++`. Homebrew llvm@21 is never first on PATH (D-17).
- Rust: a 10-second ASan canary first, wrapped in `perl -e 'alarm 10; exec @ARGV'`. If it hangs, use `cargo +nightly-2026-11-09 fuzz run loader -s none -- -max_total_time=60`.

Either fallback is recorded as a portability and ergonomics cost. A Linux container is the last-resort fallback (D-17; research confirmed the Docker daemon runs on this Mac).

Toolchain rule (D-36, amending D-17): the dated nightly-2026-11-09 is used only for cargo-fuzz (G5, M12, M13) and for Miri on the facade crate (M08). It is used for nothing else. Every build, test and gate timing (M01-M07) uses the pinned stable 1.99.1 from rust-toolchain.toml. The nightly install time is recorded under tools required (M10).

In the C option, Homebrew llvm@21 plays the matching role: it is used only for the fuzz runtime archive, by explicit path, never first on PATH (D-17).

## (4) Metric register

Every row is one (metric, variant) pair. `<opt>` stands for `c` or `rust`. Same warning strictness throughout: C `-std=c11 -Wall -Wextra -Wconversion -Werror`, Rust `cargo clippy -- -D warnings`. No sccache or ccache. Debug and release are both measured for every timed build, test and gate row (D-12): M02 to M07 each have a debug and a release variant, with the same warning strictness in both.

<!-- metric-register:begin -->
| Metric | Variant | Name | Command | Feeds |
|---|---|---|---|---|
| M01 | cold-first-build | true-cold first build | `perl -MTime::HiRes=time` timed run of the option's first-ever `opt_build debug` on this machine, taken once in the skeleton tracer task and logged as `M01 cold-first-build <opt> <seconds>` in LOG.md | reported only |
| M01 | toolchain-install | toolchain install time | recorded install time of rustup plus stable 1.99.1 for Rust; `preinstalled` for C | C3 context |
| M02 | debug | clean build, warm toolchain | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M02-<opt>-debug.json --prepare 'sh spikes/tool-01/bench/clean.sh <opt> debug' 'sh spikes/tool-01/bench/build.sh <opt> debug'` | reported only |
| M02 | release | clean build, warm toolchain | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M02-<opt>-release.json --prepare 'sh spikes/tool-01/bench/clean.sh <opt> release' 'sh spikes/tool-01/bench/build.sh <opt> release'` | reported only |
| M03 | debug | incremental build after a comment-only edit | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M03-<opt>-debug.json --prepare 'sh spikes/tool-01/bench/edit.sh <opt> comment' --cleanup 'sh spikes/tool-01/bench/restore.sh <opt>' 'sh spikes/tool-01/bench/build.sh <opt> debug'` | reported only |
| M03 | release | incremental build after a comment-only edit | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M03-<opt>-release.json --prepare 'sh spikes/tool-01/bench/edit.sh <opt> comment' --cleanup 'sh spikes/tool-01/bench/restore.sh <opt>' 'sh spikes/tool-01/bench/build.sh <opt> release'` | reported only |
| M04 | debug | incremental build after a one-line real edit to a core file | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M04-<opt>-debug.json --prepare 'sh spikes/tool-01/bench/edit.sh <opt> real' --cleanup 'sh spikes/tool-01/bench/restore.sh <opt>' 'sh spikes/tool-01/bench/build.sh <opt> debug'` | C2 |
| M04 | release | incremental build after a one-line real edit to a core file | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M04-<opt>-release.json --prepare 'sh spikes/tool-01/bench/edit.sh <opt> real' --cleanup 'sh spikes/tool-01/bench/restore.sh <opt>' 'sh spikes/tool-01/bench/build.sh <opt> release'` | reported only |
| M05 | debug | test time | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M05-<opt>-debug.json 'sh spikes/tool-01/bench/test.sh <opt> debug'` (builds the runner in that profile and runs it over the manifest) | reported only |
| M05 | release | test time | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M05-<opt>-release.json 'sh spikes/tool-01/bench/test.sh <opt> release'` (builds the runner in that profile and runs it over the manifest) | reported only |
| M06 | debug | warm gate time | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M06-<opt>-debug.json 'sh spikes/tool-01/gates.sh <opt> --no-fuzz --profile debug'` (`--profile` selects the build stage's profile and the runner the test stages use; the install and consumer-proof procedure is the same in both variants) | C2 |
| M06 | release | warm gate time | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M06-<opt>-release.json 'sh spikes/tool-01/gates.sh <opt> --no-fuzz --profile release'` | reported only |
| M07 | debug | gate time, clean to green | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M07-<opt>-debug.json --prepare 'sh spikes/tool-01/bench/clean.sh <opt> all' 'sh spikes/tool-01/gates.sh <opt> --no-fuzz --profile debug'` | reported only |
| M07 | release | gate time, clean to green | `hyperfine --warmup 1 --runs 10 --shell=default --export-json spikes/tool-01/results/M07-<opt>-release.json --prepare 'sh spikes/tool-01/bench/clean.sh <opt> all' 'sh spikes/tool-01/gates.sh <opt> --no-fuzz --profile release'` | reported only |
| M08 | sanitizer | sanitizer run over the manifest runner | C: Apple clang `-fsanitize=address,undefined` over the manifest runner. Rust: `cargo +nightly-2026-11-09 miri test` on the facade crate only (nightly allowed here by D-36; build and test rows never use it) | C1, C5 |
| M09 | core | lines of core logic | awk count over `LINES.map`, category core, by the line-count rule in section (6) | reported only |
| M09 | ffi-glue | lines of FFI and ABI glue | awk count over `LINES.map`, category ffi-glue | C4 |
| M09 | build | lines of build files | awk count over `LINES.map`, category build | C4 |
| M09 | scripts | lines of scripts | awk count over `LINES.map`, category scripts | C4 |
| M09 | runner | lines of test runner | awk count over `LINES.map`, category runner | reported only |
| M10 | list | tools required | table of every install used for build, test, install, fuzz and sanitizer, with version, licence and source; the nightly-2026-11-09 install time is listed here (D-36) | C3 |
| M11 | G4 | installed-consumer proof result | G4 procedure in section (3) | G4, C6 |
| M12 | G5 | 60-second libFuzzer run result | G5 procedure in section (3); exec/s recorded as an indicator only | G5, C5 |
| M13 | liveness | planted-bug time-to-find | G5 liveness control in section (3); seconds until the `DSPB` trap is found | G5, C5 |
| M14 | wasm | wasm build and run | Rust: build `--target wasm32-wasip1`, run the runner under wasmtime, compare the transcript to native. C: record `not attempted, install cost 180,322,002-byte wasi-sdk 34.0 download` unless it is attempted | C6 |
| M15 | path-leak | embedded source-path counts | the G4 `grep -c -a` counts over the `.a` and `.pc` | G4 |
<!-- metric-register:end -->

Row notes:

- M01 cold-first-build is the true-cold first build (D-12), reported separately from the M02 warm clean build. It is a single `perl -MTime::HiRes=time` timed run of the option's first-ever `opt_build debug` on this machine, with an empty build directory and, for Rust, immediately after the stable 1.99.1 install and before any other cargo build. It is taken once, in the skeleton's tracer task, when the skeleton holds the library and install targets only (the same scope for both options). That state cannot be recreated later, so the measurement plan copies the logged value and never re-runs it. Any one-time work (downloads, index updates, CMake compiler detection with empty caches) is included.
- M01 toolchain-install is a separate row. The nightly-2026-11-09 install time is not part of it; it is listed under M10.
- M03 and M04 toggle the edit on each run and restore the file afterwards.
- Fuzz executions per second are an indicator only. Sixty seconds is a harness check, not a bug-finding comparison.
- Timing tie band: medians within 20% (slower divided by faster at most 1.20), or both under 2 s, count as a tie.

## (5) Criteria, weights and rubric anchors

Weights (D-13). The six weights sum to 100.

| Criterion | Name | Weight |
|---|---|---|
| C1 | Memory safety at the content boundary | 25 |
| C2 | Fast feedback (incremental and gate time) | 20 |
| C3 | Tools and dependency count | 15 |
| C4 | ABI glue and facade simplicity, review and maintenance burden | 15 |
| C5 | Fuzz and sanitizer ergonomics | 15 |
| C6 | Portability and wasm | 10 |

Scores run 0.0 to 5.0 with at most one decimal digit. A score between anchors needs a written one-line reason in record 0001.

### C1 Memory safety at the content boundary (weight 25)

- 5.0: the loader and core compile with memory-unsafe operations forbidden by the compiler, unsafe code is confined to the FFI facade with a SAFETY comment on every block, and M08 passes.
- 3.0: the loader and core may perform unchecked memory access, but every buffer access is guarded by explicit bounds code, and both M08 and G5 pass.
- 1.0: as 3.0, but M08 or G5 is not demonstrated.
- 0.0: a sanitizer, Miri or fuzz finding in the loader remains unresolved.

### C2 Fast feedback (weight 20)

Computed from M04-debug and M06-debug. For each, both options score 5.0 inside the tie band. Otherwise the faster scores 5.0 and the slower scores floor(50 x faster/slower)/10, where faster and slower are the two median times. C2 is the mean of the two scores, truncated to one decimal.

### C3 Tools and dependency count (weight 15)

T is the count of M10 tools needed for build, test, install, fuzz and sanitizer. The shared baseline is excluded: sh, awk, git, perl, pkgconf, the system cc used by the consumer, and hyperfine. The option with the lower T scores 5.0, and the other scores max(0, 5.0 - (T - T_min)). A third-party library linked into the product artifact caps the score at 2.0. libfuzzer-sys in the separate fuzz crate is not product.

### C4 ABI glue and facade simplicity (weight 15)

G is the M09 lines in ffi-glue + build + scripts. Inside the 1.20 ratio band (larger divided by smaller at most 1.20) both score 5.0. Otherwise the smaller scores 5.0 and the other scores floor(50 x G_small/G_large)/10.

### C5 Fuzz and sanitizer ergonomics (weight 15)

Start at 5.0 and subtract 1.0 for each of the following. The floor is 0.0.

- (a) fuzzing needs a toolchain other than the normal build's
- (b) the G5 run lacks AddressSanitizer
- (c) manual runtime-archive linking or a fallback recipe is needed
- (d) the liveness bug was found in more than 30 s
- (e) M08 needs a fallback or does not run

The score is ND if G5 is not demonstrated.

### C6 Portability and wasm (weight 10)

- 5.0: M14 runs under wasmtime and its transcript equals native.
- 3.0: wasm builds but does not run, or the transcripts differ for a recorded cause.
- 1.0: wasm not attempted, with the install cost recorded.

Subtract 1.0 if a hard gate needed the Linux-container fallback (floor 0).

## (6) Arithmetic and decision rule

The executable form of this section is `scripts/lib/score.awk`. Where prose and scorer disagree, the disagreement is a defect to be reported, not resolved silently.

- Points are integers in hundredths: points = sum over criteria of weight x score10 x 2, where score10 is the score times ten. The maximum is 10000 (100.00 points). Scores carry at most one decimal digit, and no rounding is applied before the margin comparison.
- Not demonstrated (ND, D-15): the nominal score10 is 25 (the midpoint). The worst-case total sets the leader's ND items to 0 and the other option's ND items to 50.
- The leader is the option with the higher nominal total. Equal nominal totals mean no leader.
- RESULT=owner-recorded-preauth only when all of these hold: the leader has G1 to G5 all PASS; the nominal margin is at least 1000; the worst-case margin is at least 1000; and the scorer input says `amendment no`.
- Otherwise RESULT=proposed-default, with REASON one of tie, leader-fails-gate, both-fail-gates, margin-below-10, nd-sensitivity, amended-after-spike.

D-07 as confirmed by the owner under D-37 (owner, 2026-11-09, selected "Stricter: worst-case >=10"): auto-adoption requires that the leader passes every hard gate (a runner-up that fails a gate does not block pre-authorization), that the leader's nominal lead is at least 10.00 points with not-demonstrated items scored at the midpoint, and that the lead is still at least 10.00 when the leader's not-demonstrated items score 0 and the other option's score 5.0 (REASON=nd-sensitivity otherwise). Anything less gives proposed-default and the owner is asked. On a tie or no leader there is no default; the owner's choice is required (`default-in-force: no default; owner choice required`).

Line-count rule: count non-blank, non-comment lines per category from each skeleton's committed `LINES.map` (`path<TAB>category`) with awk. Lock files and generated files are excluded. cloc is not used.

## (7) Bias controls, timebox and honesty

- D-14: an AI executor writes both skeletons, so authoring effort is not a signal. Lines of glue count as review surface. The C skeleton is written first, and record 0001 says so.
- D-15: at most 8 hours of wall-clock time from the first commit under `spikes/` to the commit of record 0001, tracked in `spikes/tool-01/LOG.md`. Unfinished items score ND and are listed.
- Toy-workload build and test times are not extrapolated to emulator scale: each skeleton has one translation unit or crate and no dependencies.
- Resume rule: the measurement run appends each (metric, variant, option) row to results.tsv as soon as that row completes and never rewrites the file. If the run is interrupted, a restart skips every row already present and measures only absent rows. Skipping a present row is not a re-run; a present row is never measured again. A row interrupted before it was appended has no recorded value and is measured on restart; the interruption is logged.

## (8) Record 0001 requirements

- The line `Protocol commit: <40-hex sha>`, equal to the commit that added this file.
- A results table between `<!-- results:begin -->` and `<!-- results:end -->`, with columns `| Metric | Variant | Option | Value | Evidence |`. One row per register row and option. Value is non-empty, or begins with `not demonstrated:`.
- The scores file content, the scorer output, the tools-required table (tool, version, licence, source, used for), and which skeleton was written first.
- When the scorer returns proposed-default, the Decision section contains the sentence `The language decision is open and Phase 2 is blocked.` and the front matter's `default-in-force` names the leader, or, when LEADER=none, reads `no default; owner choice required`.
