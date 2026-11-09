# Stack Research

**Domain:** Portable, deterministic PC Engine / TurboGrafx-16 emulation core, headless consumer, and original diagnostic HuCard fixtures
**Researched:** 2026-11-09
**Confidence:** MEDIUM overall (tool versions, licenses and maintenance status: HIGH, checked against official repos and docs today; the language ranking is a judgment call between two close options: MEDIUM)

## How to read this file

Nothing here is an owner decision. Language, build system, license, host targets and package format are all open in `.planning/PROJECT.md`. Everything below is a **proposed default** with decision criteria, so the owner can overrule it at the Phase 1 discussion.

Claim labels used throughout:

- **[SF]** source-backed fact, with a URL in Sources
- **[CO]** code or machine observation made during this research
- **[HR]** historical report or community report
- **[P]** proposal or judgment
- **[U]** unknown, not verified

Tool versions were read from the official GitHub release APIs and docs on 2026-11-09, not from training data. Several ecosystems are newer than older references suggest: Rust is 1.99.x, Zig is 0.17.x, CMake is 4.4.x.

---

## 1. Language and build system

### Decision criteria (from AGENTS.md and PROJECT.md)

| # | Criterion | Why it matters here |
|---|-----------|---------------------|
| C1 | Explicit per-instance state, no hidden globals | Core Value: fresh isolated machine per run; multiple instances |
| C2 | Defined behavior and determinism (no UB, no host clock, no hash-order nondeterminism) | Repeatable runs are the product |
| C3 | Memory safety at content boundaries | HuCard now; disc images, cue/track parsers, archives and saves later are named security boundaries |
| C4 | Stable C-compatible boundary available | LibFuturo is a C interface candidate; API/ABI is not frozen |
| C5 | Fast build and test feedback | One local command equals the fast PR gate |
| C6 | Small, flat dependency tree and few tools | Owner preference; supply-chain cost |
| C7 | Fuzzing and sanitizer support | Malformed or oversized content is an explicit test class |
| C8 | Installed-package proof by an independent consumer | Required slice deliverable |
| C9 | WebAssembly and other host portability, later | Possible later target, not promised |
| C10 | Toolchain availability and licensing | License is still open; the toolchain must not constrain it |

### Ranked recommendation [P]

| Rank | Option | Verdict | Confidence |
|------|--------|---------|------------|
| 1 | **Rust (stable, edition 2024), Cargo, `no_std`-capable zero-dependency core crate, thin `extern "C"` facade** | Proposed default | MEDIUM |
| 2 | **C11 with CMake + Ninja, clang/gcc** | Strong runner-up; wins if the weights in "When C wins" apply | MEDIUM |
| 3 | C++20 (no exceptions/RTTI/STL in the core) | Viable but dominated by 1 and 2 | LOW |
| 4 | Zig | Do not use for the core today; revisit after 1.0 | HIGH (that it is pre-1.0) |

The ranking is close between 1 and 2, and I challenged it once. Strongest case against Rust: a second toolchain, nightly-only fuzz tooling, slower builds, and an FFI layer that needs `unsafe`. Strongest case for it: the highest-risk code in this project is parsing and executing untrusted guest content, and the project has stated determinism and security as first-order requirements. Rust makes whole classes of defects (out-of-bounds, use-after-free, uninitialized reads, data races across instances) unrepresentable in the safe core, instead of relying on sanitizer coverage to find them. The challenge does not flip the ranking, but it does produce a cheap flip test (see "Phase 1 toolchain spike" below).

### Core Technologies

| Technology | Version (verified) | Purpose | Why Recommended |
|------------|--------------------|---------|-----------------|
| Rust toolchain (stable) | **1.99.1**, released 2026-10-16 [SF] | Core, loader, headless runner | Tier 1 on aarch64-apple-darwin, x86_64-linux-gnu and x86_64-windows-msvc, so all three host platforms have tested host tools [SF]. Pin with `rust-toolchain.toml`. Rust has no LTS channel, so the pin is the stability mechanism [P]. |
| Rust edition | **2024** (stable since 1.85.0, 2025-02-20) [SF]; no newer edition appears in the edition guide [SF] | Language level | Use the current edition from day one; no migration debt in a greenfield. |
| Cargo | Bundled with the toolchain | Build, test, workspace | One tool for build/test/doc/bench. A core crate with zero third-party dependencies gives a flat tree by construction (`Cargo.lock` lists only workspace members). Enforced by `cargo-deny` (below). |
| C ABI facade | Hand-written `duospool.h` plus a `#[no_mangle] extern "C"` crate (own crate in the workspace) | Stable C-compatible boundary option | Keeps `unsafe` and the ABI in one small crate. The core crate carries `#![forbid(unsafe_code)]` [P]. Since Rust 1.81, a panic escaping a plain `extern "C"` function aborts instead of being undefined behavior [SF]. Set `panic = "abort"` for the facade and make every entry point return an explicit status [P]. |
| C11 compiler (consumer only) | Apple clang 21.0.0 present on this machine [CO]; any C11 compiler | Independent installed-package consumer | The proof consumer should be in a different language from the core so it cannot share types or assumptions with it [P]. |

### Supporting Libraries

Default stance: **no third-party runtime crates in the core.** The dependency list below is deliberately empty for the core and facade.

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none) | - | Core and facade | Hold at zero. Any addition needs a written justification and license/maintenance record (AGENTS.md: a small copied helper with provenance can beat a dependency). |
| `proptest` | Apache-2.0 (and MIT), active (pushed 2026-10-10) [SF]; version not pinned here [U] | Property tests for loader, banking, MMU | Dev-dependency only, and only if hand-written table-driven tests prove insufficient. Prefer a tiny in-repo deterministic PRNG for generated cases. |
| `libfuzzer-sys` | transitively via cargo-fuzz | Fuzz harness glue | Lives only in the separate `fuzz/` crate, outside the main workspace, so it never enters the product dependency tree [P]. |

### Development Tools

| Tool | Version (verified) | License | Purpose | Notes |
|------|--------------------|---------|---------|-------|
| `cargo test` (libtest) | bundled | MIT/Apache-2.0 | Unit and integration tests | Stable, no extra dependency. |
| `cargo-nextest` | **0.9.151**, 2026-11-03 [SF] | Apache-2.0 [SF] (dual MIT/Apache-2.0 is not confirmed) | Per-test process isolation, timeouts, JUnit output | Optional accelerator, not required. Do not let it define the project's outcome taxonomy (see section 3). |
| `cargo-deny` | **0.20.2**, 2026-07-09 [SF] | Apache-2.0 [SF] | License allowlist, advisory check, ban list | Gate: fail if any crate other than workspace members appears in the core/facade tree. Also enforces "does not constrain the future license". |
| `cargo-fuzz` | **0.13.2**, 2026-06-09 [SF] | MIT or Apache-2.0 [SF] | libFuzzer driver | **Needs a nightly compiler** and is Unix-like x86-64/aarch64 only; also needs a C++11 compiler [SF]. Pin a dated nightly used only for fuzz and Miri. |
| Miri | Nightly component; repo active 2026-11-09, Apache-2.0 [SF] | Apache-2.0 | UB detection on the `unsafe` facade | Run on the facade crate only; the core is `forbid(unsafe_code)`. Staged, not in the PR fast path [P]. |
| `cargo-c` | **v0.10.26**, 2026-10-11 [SF] | MIT [SF] | Build and install `.a`/`.so`, header and `.pc` file for C consumers [SF] | Optional. README lists `pkg-config` and OpenSSL headers on Unix as prerequisites [SF], which is heavy for a CI tool. Compare against a ~30-line `scripts/install` that copies the staticlib, header and a hand-written `.pc` file; pick whichever is smaller after the spike [P]. |
| `cbindgen` | **0.29.4**, 2026-06-10 [SF] | MPL-2.0 [SF] (tool only; generated output is not covered) | Header generation | **Not recommended initially.** A hand-written, curated header plus a C compile test avoids freezing an ABI by accident and avoids generated-header drift [P]. Revisit once an API shape is proven by a consumer. |
| `wasmtime` | **v50.0.1**, 2026-10-29 [SF] | Apache-2.0 [SF] | Run wasm builds in a deterministic sandbox | Staged CI only (section 6). |
| `rustfmt`, `clippy` | bundled components | MIT/Apache-2.0 | Format and lint | Part of the fast gate; deny warnings in CI only, not on dev machines. |

### Build system

| Technology | Role | Why |
|------------|------|-----|
| **Cargo workspace** | Primary build for core, facade, runner, tests | Zero extra tool for the main loop. |
| **POSIX shell script + `cc`** (e.g. `scripts/consumer-proof`) | Build the independent C consumer against the installed prefix | Smallest possible consumer proof: `cc $(pkg-config --cflags --libs duospool)`. No CMake needed unless a CMake `find_package` config is promised later [P]. |
| **One local gate command**, e.g. `scripts/check` | fmt, clippy, tests, loader refusals, conformance inventory, C consumer proof | Matches the "one documented local command" requirement. Avoid a task-runner dependency (`just`, `make`) unless the script grows; `just` is not installed here [CO]. |

### Phase 1 toolchain spike [P]

Because ranks 1 and 2 are close, spend at most one day measuring instead of debating. For **each** of Rust and C, build a skeleton with: `create`/`step`/`destroy` on an opaque instance, a status-code return path, a 64 KiB "HuCard" loader with refusal cases, a C consumer against an installed prefix with source paths removed, a 60-second fuzz run on the loader, and a wasm build. Record clean-build time, incremental-build time, test time, lines of glue, and tools required. Record the result as a decision record. Both skeletons share the same fixture manifest, so nothing is thrown away [P].

### When C wins (flip conditions) [P]

Switch the proposed default to C11 + CMake if the owner weights any of these highest:

1. A single toolchain already installed everywhere. Today: clang 21, CMake 4.4.3 and Ninja 1.13.2 are present, while `rustc`, `cargo`, `zig`, `meson`, cc65 and WLA-DX are not [CO].
2. Direct reuse of C code or a C header with no binding layer, including a LibFuturo adapter. Note the project rules still forbid assuming a LibFuturo dependency.
3. Fastest possible compile loop on a large core, or contributors who are C-only.
4. Wanting libFuzzer/sanitizers without a nightly toolchain.

If C is chosen, use these pins and settings:

| Technology | Version | Notes |
|------------|---------|-------|
| C11 (`-std=c11`), clang (primary) and gcc | LLVM **23.1.5** release (2026-11-03) [SF]; Apple clang 21.0.0 locally [CO] | Build with `-Wall -Wextra -Wconversion`, `-fwrapv`, and no floating point in the core [P]. |
| CMake | **4.4.5** (2026-10-27) [SF]; 4.4.3 is installed here [CO]. Presets schema v13 is documented with 4.5 [SF] | BSD-3-Clause [SF]. Use `CMakePresets.json` with configure/build/test/package workflow presets [SF] for the one-command gate. |
| Ninja | **1.13.2** [SF] | Apache-2.0 [SF]. |
| Fuzzing | clang libFuzzer (`-fsanitize=fuzzer,address`) | **Apple clang 21 on this machine does not ship the libFuzzer runtime** (`libclang_rt.fuzzer_osx.a` not found) [CO]. Homebrew `llvm@21` and `llvm@18` are installed and are the likely fix [CO]; the link with them was not tested [U]. |
| Meson | 1.12.1 (2026-09-22) [SF], Apache-2.0 [SF] | Acceptable alternative to CMake. No decisive advantage here, and it adds a Python dependency to the build [P]. |

---

## 2. Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Rust + Cargo | C11 + CMake/Ninja | See "When C wins". Ranked 2; the difference is judgment, not a defect in C. |
| Rust + Cargo | C++20 + CMake | Only if a template-heavy design is wanted for compile-time specialization of CPU/VDC variants. Costs: no memory safety, and exceptions, RTTI and STL all need policing for determinism and ABI. Existing emulators are frequently C++ [HR], which is evidence of what works, not of what fits this project's constraints. |
| Rust + Cargo | Zig 0.17.1 | After Zig 1.0 and a stability policy. Today it is pre-1.0, with no 1.0 date given [SF]. Version 0.17.0 removed `@cImport`, split the build system into separate configure and maker processes, and broke ZLS compatibility [SF]. A core written against it would carry a breaking-change tax every release. |
| Hand-written header | cbindgen / cargo-c generated header | After a real consumer proves the API shape. |
| cargo-fuzz (libFuzzer) | AFL++ **v5.03c** (2026-09-02) | AFL++ tool is **AGPL-3.0-or-later**; the code compiled into harnesses stays Apache-2.0 [SF]. Fine as an external tool, but do not vendor it, and it adds a license question; defer. |
| cargo-fuzz (libFuzzer) | honggfuzz (Apache-2.0, active 2026-09-25) [SF] | If a second engine adds coverage later. libFuzzer's own docs say its original authors have stopped active work and only important bugs get fixed [SF], so keep the fuzz target engine-agnostic: a plain function taking `&[u8]`. |
| Cargo test + project-owned conformance runner | Catch2 v3.16.1 (BSL-1.0), Unity v2.7.0 (MIT) [SF] | Only on the C path. The C path should prefer plain `assert`-style tests with CTest to avoid a dependency [P]. |
| Native-only first | Emscripten **7.0.0** (2026-11-06) / wasi-sdk **34** (2026-08-25, Apache-2.0) [SF] | C path wasm routes. Emscripten's license shows no single SPDX in the GitHub API [SF]; check it before relying on it [U]. |

---

## 3. Test tooling

### Principle [P]

The distinct outcomes the project requires (pass, guest failure, timeout, harness failure, skip, cancellation, missing artifact) and "fail closed on missing required checks" are **not** what generic frameworks model. So: use the language's built-in test runner for unit tests, and **own the conformance runner**.

| Layer | Tool | Notes |
|-------|------|-------|
| Unit tests (CPU opcodes, MMU banking, VDC/VCE register behavior, loader) | `cargo test` | Fast gate. |
| Conformance runner | Project-owned binary reading a manifest (fixture id, ROM path, hash, scripted input, machine-time bound, expected observations, required/optional) | Emits one machine-readable result per inventory entry with a closed outcome enum. Fails if any required inventory entry has no result. |
| Negative controls | Deliberately broken expectation, no-input and wrong-input scripts, truncated and oversized ROMs | The runner must reject each. A control that passes means the gate is broken. |
| Replay of fuzz findings | Corpus directory replayed through `cargo test` on stable | Gives the PR gate deterministic regression coverage from fuzzing without needing nightly there [P]. |
| CPU instruction vectors | **SingleStepTests/65x02** (MIT; last push 2025-05-10) [SF]: directories `6502`, `nes6502`, `rockwell65c02`, `synertek65c02`, `wdc65c02` [SF] | Useful for the shared 65C02 subset (the Rockwell variant covers bit-test and bit-branch instructions, which HuC6280 shares [P]). It has **no HuC6280 set**, and HuC6280-only instructions (block transfers, ST0/1/2, TAM/TMA, SXY/SAX/SAY, CSH/CSL, SET, TST, BSR) are not covered [SF/P]. Whether cycle counts and bus activity transfer to HuC6280 is **[U]** and needs a primary-source check. Treat results as a partial oracle for decode and flags, not for timing. Fetch a pinned commit into an uncommitted cache; do not vendor the large JSON files into the repo [P]. |
| Determinism hygiene checks | Same ROM and script run twice in one process and in two processes; compare raw video and audio digests | Proves repeatability only, not correctness, per AGENTS.md. Label it that way in the runner output. |
| Supply-chain gate | `cargo-deny` in the fast gate | Allowlist for tool and dependency licenses; ban non-workspace crates in the product graph. |

### Determinism rules for the core [P]

- No `std::time`, threads, files, environment or randomness in the core crate. Pass everything in through the API.
- No default-hasher `HashMap`/`HashSet`; the standard library's `RandomState` is seeded randomly, so iteration order differs between runs. Use fixed arrays or ordered structures.
- No floating point in machine behavior. Host-side colour conversion belongs to the consumer.
- Explicit wrapping/checked arithmetic on 8/16-bit machine values. Overflow checks stay enabled in test builds so accidental wraps are caught.
- One owned state struct per instance; tests create two instances and run them interleaved to catch hidden sharing.

### Performance tracking [P]

Use a project-owned headless benchmark: named workload (fixture plus input script), machine cycles emulated per host second, recorded with build identity, and output digest held constant. Source-attributed profiling uses platform profilers (Instruments on macOS, `perf` on Linux); none is installed or pinned here [U]. Keep it out of the PR path until noise is characterized.

---

## 4. Fuzzing [P]

| Target | Input | Oracle | Stage |
|--------|-------|--------|-------|
| HuCard loader | Arbitrary bytes as a ROM image | Never crashes; returns accept or a typed refusal; no allocation beyond a cap | Scheduled/nightly, plus corpus replay in the PR gate |
| Bounded execution | Arbitrary ROM plus arbitrary input script, bounded machine time | No panic or abort, no host-time or hash-order dependence, run twice and compare | Scheduled/nightly |
| Facade API sequence | Random call sequences over the C ABI (null buffers, short buffers, reset, unload) | Status codes only; no UB under Miri/ASan | Scheduled/nightly |
| Later: disc/cue/track parsers | Arbitrary bytes | Same as loader | Only when CD-ROM² is scoped |

The fuzz crate lives outside the main workspace and uses a dated nightly. The fuzz target body is a plain `fn(&[u8])` that can be driven by libFuzzer, honggfuzz or a replay test.

---

## 5. WebAssembly and other host portability (later target, not promised)

- **Rust path** [SF]: `wasm32-unknown-unknown`, `wasm32-wasip1` and `wasm32-wasip2` are Tier 2 ("guaranteed to build", tests not always run, no host tools). A `no_std`, no-I/O core is the easiest possible wasm payload. `wasm-bindgen` (0.2.130) is **not** needed for an exported C-style API; add it only if a browser host needs generated glue [P].
- **C path**: wasi-sdk 34 (Apache-2.0) or Emscripten 7.0.0 [SF].
- Running the core under `wasmtime` v50.0.1 and comparing output digests against the native run is a cheap cross-host repeatability check. It shares one implementation, so it proves repeatability, not correctness [P].
- Add a browser or device CI job only if that target is promised and the job catches a distinct recurring fault (AGENTS.md).

---

## 6. CI staging suggestion [P]

| Job | Failure class | Path |
|-----|---------------|------|
| `scripts/check` (fmt, clippy, unit tests, loader refusals, conformance inventory with negative controls, `cargo-deny`, C consumer proof) | Logic, inventory and packaging regressions | Fast PR gate |
| Fixture reproducibility (rebuild ROMs with pinned assemblers, compare bytes to committed) | Fixture provenance drift | Separate job; assembler setup cost stays out of the fast gate |
| Fuzz (nightly toolchain, time-boxed) and Miri | Memory/UB in loader and FFI | Scheduled |
| wasm digest comparison | Host-portability drift | Scheduled, only once wasm is promised |
| Benchmarks | Throughput regressions | Scheduled or manual until noise is characterized |

The CI provider is not owner-selected [U]. Measure critical-path cost after the first run before adding caches or parallelism, per AGENTS.md.

---

## 7. ROM-authoring toolchain for diagnostic HuCard fixtures

### Why independent tools matter [P]

If the emulator's decoder and the fixture assembler are both written by the same author from the same reading of the HuC6280 manual, an opcode misunderstanding is correlated and cannot be detected. Assemble fixtures with an **independent, third-party assembler** and, where cheap, **cross-assemble the same source with a second assembler and require byte identity**. Dual-assembler agreement is an oracle for instruction *encoding* only. It is not an oracle for hardware behavior. The dual-assembler workflow was **not run** here; building downloaded toolchain code was blocked in this session [U].

### Candidates

| Tool | Version / status (verified 2026-11-09) | License | HuC6280 support | Verdict |
|------|-----------------------------------------|---------|-----------------|---------|
| **ca65 / ld65 (cc65)** | Tagged release **V2.19** (2020-11-20); `master` active, last commit 2026-11-08 [SF]. Homebrew stable is 2.19 [CO] | **Zlib** [SF] | `.setcpu "huc6280"` / `--cpu huc6280`. The V2.19 tag's instruction table includes block transfers (TAI/TDD/TIA/TII/TIN), ST0/1/2, TAM/TMA, SXY/SAX/SAY, CLA/CLX/CLY, CSH/CSL, SET, TST, BSR, Rockwell bit ops [CO]. The test tree has a `huc6280-cpudetect.ref` [SF]. | **Primary.** Most permissive license, mature segmented assembler and a flexible linker config. Its `pce` target config is for 8/16/32 KiB images and needs a bank-rotation post-step [SF], so write a project-owned `ld65` config instead of using the stock one. Do not use the cc65 PCE runtime library for fixtures. |
| **WLA-DX** | **v10.7**, 2026-06-28; repo active 2026-11-06 [SF] | **GPL-2.0-or-later** (README and `legal.rst`; GitHub API reports no SPDX) [SF] | HUC6280 is a supported target; docs mention PC Engine zero-page addressing hints [SF/CO]. | **Secondary / cross-check.** GPL covers the tool. Output binaries are not normally covered, but that is a general understanding, not a statement found in its docs [U]. Its `include/pcengine` register definitions ship under the repo license; write your own register constants rather than including them in fixtures [P]. Does not constrain DuoSpool's license when used only as an external build tool. |
| **PCEAS / HuCC (pce-devel/huc)** | Rolling prerelease tag `current`, assets built 2026-11-04 (Darwin/Linux/Win64); default branch commits through 2026-11-04; CI passing [SF]. Source builds with `make`. Contains pceas (MagicKit derived), HuCC and the last HuC4 [SF] | **Unclear.** The repo's `LICENSE` says original authors "neglected to specify the conditions"; it is "next to impossible to make an exact statement on the legal status"; the MagicKit assembler source says "freeware... distribute, use and modify as you wish"; Hecht's changes are BSD-2; TGEmu and many tests are GPL [SF]. GitHub reports `NOASSERTION` [SF] | Native PC Engine assembler and C compiler; community-standard toolchain [HR]. Supports SuperGrafx [SF]. | **Optional third check, not a dependency.** Use only as an external tool. Do **not** vendor it, and do **not** include its library, startup code or headers in fixtures, because their redistribution rights are unclear [SF]. A rolling `current` tag is not a reproducible pin; pin by commit and build from source if used [P]. |
| **Macroassembler AS (asl)** | `Macroassembler-AS/asl-releases` pushed 2026-05-27 [SF]; exact current build **[U]** | GPL (reported as GPL-2.0 on the release-tracking repo; distro packages say GPL-2.0-only OR GPL-3.0-only, GPLv2+) [SF/HR] | HuC6280 is in its supported-processor list per community listings [HR] | **Not recommended** for now: a third assembler without a demonstrated need, and its upstream and licensing are scattered. Keep as a fallback. |
| **Project-owned mini assembler or byte emitter** | n/a | Owner's | n/a | **Avoid for the encoding oracle** (correlated errors, see above). Acceptable only for emitting raw data tables or padding around assembler output. |

### Recommended fixture workflow [P]

1. **Sources**: original, project-authored `.s` files (own startup, reset vector, MPR bank-map setup and register constants). Fixture source license follows the owner's license decision [open].
2. **Assemble** with ca65/ld65 pinned to the `V2.19` tag or a named `master` commit, built from source with a recorded hash. Cross-assemble with WLA-DX v10.7 where the source is written in the common subset, and require byte identity.
3. **Commit** the source, the built `.pce` bytes, and a manifest recording source path, assembler name/version/command line, SHA-256, license, notices, redistribution terms, and the test's exact meaning. The fast PR gate uses the committed bytes, so no assembler is needed there.
4. **Reproduce** in a separate job that rebuilds with the pinned assembler and compares bytes.
5. Output is a raw HuCard image with no emulator-specific header. Keep any copier-header handling in the loader, and test refusal of a header-prefixed image as a negative control [P, header specifics U].

### Existing PC Engine test programs: rights and toolchain caution

| Program | License | Caution |
|---------|---------|---------|
| PCE_Soundtest (pce-devel) | MIT [SF] | Built with `huc` (`make-beep.sh`) and the committed `beep.s` includes HuC-generated startup code [CO], so the generated parts inherit HuC's unclear status. Its result is an analog-capture observation, not a deterministic oracle [HR]. Do not admit as a fast-suite fixture without a rights review [P]. |
| 240p Test Suite PCE | GPL-2.0-or-later per source file (per project seed) | Strong copyleft; whether it can sit in a repository under another license is an owner and legal decision tied to the open license choice. Not a fast-suite candidate. |
| Etripator (pce-devel disassembler) | GPL-3.0 [SF]; latest release v0.9, 2022-12-03 [SF] | Inspection tool only. Never ship its output or code in the repo. |

---

## Installation

These commands are for the proposed default and have not been run here. None of the Rust or cc65/WLA-DX tools is installed on this machine [CO].

```bash
# Toolchain (pin with rust-toolchain.toml, channel = "1.99.1")
rustup toolchain install 1.99.1 --component rustfmt clippy

# Dev tools (cargo-fuzz and Miri need a pinned nightly; use that only for fuzz/Miri)
cargo install --locked cargo-deny cargo-nextest
rustup toolchain install nightly-<pinned-date> --component miri
cargo +nightly-<pinned-date> install --locked cargo-fuzz

# Fixture assemblers (external tools; build from pinned tag, record hash)
#   cc65 V2.19 : zlib, also available as a Homebrew bottle (2.19)
#   WLA-DX v10.7: GPL-2.0-or-later, also available as a Homebrew bottle (10.7)
```

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Zig as the core language | Pre-1.0 with no date; 0.17.0 removed `@cImport`, reworked the build system and broke tooling [SF]. Its GitHub repository now says "Moved to Codeberg" [SF]. | Rust or C11 |
| Default-hasher `HashMap`, wall-clock time, threads, floats in the core | Break determinism (random per-process hash seed, host timing) | Fixed arrays, explicit machine time, integer arithmetic |
| `unsafe` anywhere in the core crate | Defeats the memory-safety rationale for choosing Rust | Confine to the facade crate; `forbid(unsafe_code)` on the core |
| Panics crossing the C ABI as a control-flow mechanism | Aborts the host process (Rust 1.81+) [SF] | Status codes at every entry point |
| Generated header (cbindgen) as the public contract at the start | Freezes whatever the code happens to look like | Hand-written header, revisited after a consumer proves the shape |
| Vendoring HuC/PCEAS, its library, startup code or headers into fixtures | Redistribution status explicitly unclear [SF] | Own startup code; external tool only |
| A self-written assembler as the only source of fixture bytes | Correlated errors with the emulator decoder | ca65 plus WLA-DX cross-assembly |
| Copying code from MAME, Mednafen or other emulators | Their licenses were not examined here and DuoSpool's license is open [U] | Use them as reading material only; record provenance for any helper that is copied after a license decision |
| AFL++ as a vendored or bundled component | Tool is AGPL-3.0-or-later [SF] | External tool use only, or libFuzzer/honggfuzz |
| Autotools, Bazel, or a task-runner dependency for the fast gate | Extra tools and maintenance for no distinct benefit | Cargo plus one shell script (or CMake presets on the C path) |
| Treating a matching hash, a boot or a title screen as accuracy evidence | Violates the Core Value | Semantic assertions tied to an independent oracle or labeled repeatability-only |

## Stack Patterns by Variant

**If the owner accepts the proposed default (Rust):**
- Workspace of `core` (safe, no I/O), `capi` (the only `unsafe`, exports `extern "C"`), `runner` (headless consumer, uses the C ABI or the core directly), `conformance` (manifest runner), and `fuzz` (outside the workspace).
- The installed-package proof is a ~50-line C program built outside the repo against the installed prefix.

**If the owner chooses C:**
- Static library `libduospool` with opaque `struct` handles, CMake `install(EXPORT)` plus a `.pc` file, and a consumer built from a separate CMake project with `CMAKE_PREFIX_PATH` pointing only at the install tree.
- Compile with sanitizers (ASan, UBSan) in a separate preset, run libFuzzer from Homebrew LLVM on macOS, and add `-fwrapv` plus no-float rules for determinism.

**If WebAssembly becomes a promised target:**
- Rust: build `wasm32-unknown-unknown` from the same core, export the C-style functions, and run the same conformance manifest under a wasm host (output compared against native as a repeatability check).
- C: build with wasi-sdk or Emscripten with the same manifest.

**If CD-ROM², SuperGrafx or Arcade Card enter scope:**
- This does not change the language choice, but it raises the weight of C3 (parser safety) and of multi-input content shapes at the C boundary, which are the exact mismatches the LibFuturo assessment lists as unproven. Revisit the facade shape, not the toolchain.

## Version Compatibility

| Item | Compatible With | Notes |
|------|-----------------|-------|
| Rust 1.99.1 stable | Edition 2024 | Edition 2024 needs 1.85 or newer [SF]. |
| `cargo-fuzz` 0.13.2 | Nightly toolchain, x86-64/aarch64 Unix-like | Not on Windows [SF]. Keep fuzzing off the Windows CI row. |
| Apple clang 21.0.0 | No libFuzzer runtime on this machine [CO] | Use Homebrew `llvm@21` for fuzzing on the C path. |
| wasm32 targets | Rust Tier 2 | Build guaranteed, tests not always run upstream [SF]. Run your own tests under wasmtime. |
| ca65 V2.19 vs `master` | HuC6280 mnemonic table present in both [CO] | Pin one. V2.19 is a 2020 tag, while `master` has many later fixes; choose and record. |
| WLA-DX v10.7 | GPL-2.0-or-later tool | Used as an external tool only. |
| cc65 `pce` target | 8/16/32 KiB images with a bank-rotation post-step [SF] | Too small and awkward for general fixtures; use a custom linker config. |

## Open owner decisions (not resolved by this file)

| Decision | Why research cannot settle it |
|----------|-------------------------------|
| Language and build system | Judgment between Rust and C weighted by owner priorities (toolchain count, LibFuturo directness, safety); the Phase 1 spike supplies data |
| DuoSpool license and repository visibility | Owner authority. This file only records that none of the recommended build tools (Cargo/rustc, cc65, CMake, Ninja) constrains it, and that WLA-DX and AFL++ are GPL/AGPL tools used externally. HuC/PCEAS has unclear provenance; 240p Suite is GPL |
| Fixture source license | Follows the license decision; also decides whether GPL test programs may be redistributed beside the code |
| Host/platform targets and package format | Open until the installed-consumer proof is planned; the proposal above is a `.a` + header + `.pc` prefix |
| Whether a wasm target is promised | Determines when wasm CI is worth its cost |
| CI provider | Not selected |

## Gaps and unknowns

- [U] HuC6280-specific instruction timing and bus-activity oracles; SingleStepTests do not cover them.
- [U] Whether the dual-assembler byte-identity workflow works for a real HuC6280 source file; blocked here, so run it in the Phase 1 spike.
- [U] Rust versus C build, incremental-build and test times for this project; measure in the spike.
- [U] Whether libFuzzer links with Homebrew `llvm@21` on this Mac.
- [U] Emscripten's exact license; `proptest` and `honggfuzz` versions; Macroassembler AS's current build and license text.
- [U] Whether `cargo-c` is lighter than a hand-written install script.

## Sources

- [pce-devel/huc repository](https://github.com/pce-devel/huc), its [LICENSE](https://github.com/pce-devel/huc/blob/master/LICENSE), README and `releases/tags/current` assets via the GitHub API (HuCC/pceas status and license text): HIGH
- [pce-devel/PCE_Soundtest](https://github.com/pce-devel/PCE_Soundtest) (`make-beep.sh`, `beep.s`): HIGH
- [cc65/cc65](https://github.com/cc65/cc65): `doc/ca65.sgml`, `src/ca65/instr.c`, `cfg/pce.cfg`, `doc/pce.sgml`, `test/asm/cpudetect/`, tag V2.19, Zlib license: HIGH
- [vhelin/wla-dx](https://github.com/vhelin/wla-dx): README, `doc/legal.rst`, `doc/files.rst`, `doc/codetoknow.rst`, release v10.7: HIGH
- [Macroassembler-AS/asl-releases](https://github.com/Macroassembler-AS/asl-releases) and distro packaging pages (AS license, maintenance): LOW-MEDIUM
- [SingleStepTests/65x02](https://github.com/SingleStepTests/65x02): HIGH
- [Rust platform support](https://doc.rust-lang.org/rustc/platform-support.html), [Edition guide 2024](https://doc.rust-lang.org/stable/edition-guide/rust-2024/index.html), [Rust 1.81.0 release post](https://blog.rust-lang.org/2024/09/05/Rust-1.81.0/), [Reference: panic](https://doc.rust-lang.org/reference/panic.html): HIGH
- Rust 1.99.1 and other release versions from the GitHub releases API for rust-lang/rust, Kitware/CMake, mesonbuild/meson, ninja-build/ninja, llvm/llvm-project, emscripten-core/emscripten, WebAssembly/wasi-sdk, AFLplusplus/AFLplusplus, rust-fuzz/cargo-fuzz, lu-zero/cargo-c, mozilla/cbindgen, nextest-rs/nextest, EmbarkStudios/cargo-deny, bytecodealliance/wasmtime, rustwasm/wasm-bindgen, catchorg/Catch2, ThrowTheSwitch/Unity: HIGH
- [Zig downloads](https://ziglang.org/download/) and [Zig 0.17.0 release notes](https://ziglang.org/download/0.17.0/release-notes.html): HIGH
- [rust-fuzz/cargo-fuzz](https://github.com/rust-fuzz/cargo-fuzz) (nightly requirement, platforms, license): HIGH
- [LLVM libFuzzer docs](https://llvm.org/docs/LibFuzzer.html) (status, build flags, platforms): HIGH
- [AFL++ README](https://github.com/AFLplusplus/AFLplusplus) (AGPL-3.0-or-later, Apache-2.0 harness code): HIGH
- [lu-zero/cargo-c](https://github.com/lu-zero/cargo-c) README: HIGH
- [CMake presets manual](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html): HIGH
- Local machine observations (toolchains present or absent, Apple clang lacking the libFuzzer runtime, Homebrew bottle versions): CO, 2026-11-09
- Project inputs: `.planning/PROJECT.md`, `AGENTS.md`, `.planning/bootstrap/PROJECT-SEED.md`, `.planning/bootstrap/EMULATOR-DNA.md`

---
*Stack research for: PC Engine / TurboGrafx-16 emulator (DuoSpool)*
*Researched: 2026-11-09*
