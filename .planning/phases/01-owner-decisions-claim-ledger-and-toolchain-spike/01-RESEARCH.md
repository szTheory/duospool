# Phase 1: Owner Decisions, Claim Ledger and Toolchain Spike - Research

**Researched:** 2026-11-09
**Domain:** Gate phase. POSIX sh/awk record checkers, a measured Rust-vs-C toolchain spike on macOS arm64, a pre-publication history audit, licence files, and seed content for a claim ledger. No emulator code.
**Confidence:** HIGH for everything probed on this machine (tagged `VERIFIED: machine`); MEDIUM for the Rust-side spike mechanics (rustc/cargo are not installed here, so they were not run); MEDIUM for hardware-fact seed content (it is a transcription of SUMMARY.md, which is itself MEDIUM).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

### Owner decisions recorded in this session (2026-11-09, `/gsd-discuss-phase 1`)

Each of these becomes an `owner-recorded` decision record. The `owner-statement` field quotes the owner's selected option verbatim and cites this session and date.

- **D-01 (decision 8a, licence):** The owner selected "MIT OR Apache-2.0 + 0BSD".
  - Code is licensed `MIT OR Apache-2.0`.
  - Authored fixture sources and the fixture ROMs built from them are `0BSD`, declared per directory.
  - Phase 1 adds `LICENSE-MIT`, `LICENSE-APACHE` and the 0BSD text for fixture directories, and SPDX headers on new source files.
  - Reading GPL emulators (Mednafen, Geargrafx) stays read-only. No GPL code is absorbed.
  - Outside contributions are inbound=outbound under the same dual licence, recorded in the licence record.
  - **Reversibility:** one-way. Relicensing later needs every contributor's consent once outside contributions exist.
- **D-02 (decision 8b, visibility):** The owner selected "Public now".
  - The repository becomes public. No remote exists today.
  - Before the first push, a pre-publication audit must pass:
    - The full git history is scanned for ROM, firmware or disc-like binaries, saves, credentials and tokens.
    - The `DuoSpool` name is noted as not trademark-cleared.
  - Creating the public remote (`gh repo create --public` or equivalent) is an outward-facing action that cannot be undone. The planner MUST put a `checkpoint:decision` immediately before it, so the owner sees the audit result first.
  - **Reversibility:** one-way. Published history cannot be unpublished.
- **D-03 (decision 14, CI provider):** The owner selected "GitHub Actions".
  - The workflow YAML is a thin wrapper around the provider-neutral `scripts/check`.
  - Actions are pinned by full commit SHA, with `permissions: contents: read` by default.
  - No `pull_request_target`, `persist-credentials: false`, and no secrets in PR jobs.
  - Proposed matrix, which Phase 2 confirms by measured value:
    - PR path: Linux x86_64. It is the platform that differs from local macOS.
    - Scheduled: macOS aarch64 and Windows.
    - Public-repo minutes are free, so cost is not the constraint. Distinct failure classes are (AGENTS.md).
  - Phase 1 only records the decision. The CI workflow itself is Phase 2 (CI-01).
- **D-04 (decision 9, fixture rights):** The owner selected "Tiered, authored default".
  - Authored fixtures are the default.
  - A third-party fixture is admitted only by a recorded exception. The exception records SPDX, source URL and commit, every generated part, hash, redistribution terms, and owner sign-off.
  - GPL or unclear-licence material is never committed. At most it is fetched by pinned hash into an uncommitted local cache.
  - Explicitly not admitted: PCE_Soundtest, which has HuC-generated startup code, and any HuC/PCEAS runtime, startup code or headers. The 240p Test Suite is not admitted in-repo.
  - The record states that no GPL or unclear-licence material is admitted without a further owner decision.
- **D-05 (decision 6, API/ABI posture):** The owner selected "No freeze, 0.x".
  - An internal advance API comes first.
  - A hand-written C header comes only after a consumer proves the shape. No cbindgen as the contract.
  - 0.x means any commit may break the API, ABI, state layout or manifest format.
  - The installed-consumer proof pins an exact version.
  - Once a header exists, it exports a `duospool_abi_version()` that must match exactly.
  - No crates.io or package publication before a release decision. A libretro wrapper would be a host, not part of the core API.
- **D-06 (decision 2, first-profile scope):** The owner selected "Select proposed scope".
  - These scope fields are `owner-selected`:
    - base PC Engine HuCard machine
    - `chip_revision = huc6280` (not 6280A)
    - 2-button pad on port 1, no multitap
    - expansion port absent (no CD)
    - `vdc_count = 1`, no VPC
    - media slot HuCard only
    - profile id `pce-hucard-2btn`
  - The disputed hardware fields stay `open`, with labelled proposals: line count, PSG tick divisor, `$1000` D6 and D7 values, content byte order, copier-header policy, and power-up policy.
  - Selecting HuC6280 as the emulated chip makes no claim about which retail models shipped which revision (C14 stays open in the ledger).
- **D-07 (decision 1, language/build pre-authorization):** The owner selected "Pre-authorize clear winner".
  - If exactly one option passes every hard gate in the protocol and leads by ≥10 weighted points, the language record becomes `owner-recorded`, citing this pre-authorization as the owner statement.
  - Any tie, a margin under 10 points, or a gate failure in both options leaves the record `proposed-default`. Phase 2 then stays blocked and the owner is asked.
- **D-08 (copied-code provenance principle):** The owner's words in this session were: "another copy and paste is better than another dep if we truly need a dep that's fine but it's better to have smaller, flatter dependency trees".
  - The provenance-rule record is `owner-recorded` for this principle.
  - The mechanism (D-24..D-26) is Claude's design under it.

### Toolchain spike protocol (TOOL-01)

- **D-09 Pre-registration:** A protocol file is committed on `main` before any skeleton code exists: `docs/decisions/0001-language-build.protocol.md`.
  - It contains the fixed workload, the metrics and how each is measured, the hard gates, the weights, the tie band and the margin rule.
  - The decision record cites the protocol's commit hash.
  - Any change after commit is a dated amendment that says why. No in-place edits.
- **D-10 Identical workload:** Both skeletons build exactly the same things from one written spec:
  - An opaque `create/step/destroy` instance.
  - A per-instance state struct with no globals. Two instances run interleaved and give identical digests.
  - A status-code enum returned by every entry point.
  - A 64 KiB HuCard loader with a closed refusal set and one accept case. The refusal set is: empty, truncated, oversized, bad size, header-prefixed, null buffer, and short buffer.
  - A stub `step` that advances a machine-time counter and a deterministic state digest.
  - A manifest-driven test runner. It reads one shared, byte-identical `fixtures.manifest` that includes negative controls (a deliberately wrong expectation must be rejected).
  - A hand-written C header.
  - A C consumer that runs create, load, step and destroy.
  - No CPU, VDC or other machine logic.
  - Fixtures are generated by a script (zero-fill, truncations, seeded PRNG). Never real ROMs.
- **D-11 Hard gates (pass/fail; an option that fails any gate cannot win):**
  - Refusal matrix passes.
  - Negative controls are rejected.
  - Two-instance interleave check passes.
  - Installed-consumer proof passes: install to a prefix, copy the consumer to a separate dir, make the source and build trees unreadable, build with `cc` against the installed `.a`, header and `.pc` only, and match the golden output.
    - Its negative control: removing the header from the prefix must break the build.
    - The `.a` and `.pc` are checked for embedded absolute source paths.
  - The 60-second libFuzzer harness runs on the loader without crashing.
- **D-12 Measurements:**
  - Timings use `hyperfine` (Homebrew, a dev-machine measurement tool only; recorded in "tools required"): `--warmup 1 --runs 10 --prepare 'rm -rf <build dir>'`.
  - Clean build is reported twice: a true-cold first build (first downloads) separately from a warm-toolchain clean build.
  - Incremental build is measured after a comment-only edit and after a one-line real edit to a core file, rebuilding what the gate needs.
  - Gate time is measured warm, and clean-to-green.
  - Debug and release are both measured, with the same warning strictness (`-Wall -Wextra -Wconversion` vs `clippy -D warnings`). No sccache or ccache.
  - Sanitizer runs (C) and Miri on the facade (Rust) are reported as separate rows.
  - Lines are counted per category: core logic, FFI/ABI glue, build files, scripts, test runner.
  - "Tools required" lists every install with its licence and source.
  - wasm: Rust builds `wasm32-wasip1` via a `rustup` target and runs under wasmtime, with digests compared to native. If the C wasm toolchain (wasi-sdk) is not installed, the record says "not attempted, install cost X". It does not guess.
  - Fuzz executions per second are an indicator only. Sixty seconds is a harness check, not a bug-finding comparison.
- **D-13 Weights (Claude-proposed; fixed in the protocol commit; the owner may amend before the spike runs):**

  | Criterion | Weight |
  |---|---|
  | Memory safety at the content boundary (scored by what is prevented by construction vs left to sanitizers) | 25 |
  | Fast feedback (incremental and gate time) | 20 |
  | Tools and dependency count | 15 |
  | ABI-glue and facade simplicity, plus review and maintenance burden | 15 |
  | Fuzz and sanitizer ergonomics | 15 |
  | Portability and wasm | 10 |

  - Tie band: timing medians within 20%, or both under 2 s, score as a tie.
  - Extrapolating toy-workload build times to emulator scale is forbidden, and the record says so.
- **D-14 Bias controls:**
  - An AI executor writes both skeletons, so authoring effort is not a signal. Lines of glue count as review and maintenance surface.
  - Record which skeleton was written first. Both use the same spec and the same manifest.
- **D-15 Timebox:** At most one working day in total. Unfinished items are scored "not demonstrated", not zero, and are listed.
- **D-16 Spike code fate:**
  - The skeletons live in `spikes/tool-01/{rust,c}/`, committed on `main` and tagged `spike-tool-01-evidence`.
  - After the decision is recorded, they are removed from `main` in a follow-up commit. The tag preserves them.
  - Phase 2 starts clean in the final layout. It may copy individual files with a provenance note.
- **D-17 Toolchain installs on this Mac:**
  - `rustup` with `rust-toolchain.toml` pinned to 1.99.1, plus the wasm target.
  - A dated nightly is used only for cargo-fuzz.
  - Homebrew `llvm@21` is used only for C fuzzing, by explicit path. It is never first on PATH, so the C build still measures Apple clang.
  - Record the versions of `cmake`, `ninja` and `clang`.
  - Whether libFuzzer links with Homebrew `llvm@21` is itself a recorded result. If it fails, the fallback is a Linux container and is recorded as a portability cost.

### Shared record format

- **D-18 One strict TOML subset for all hand-edited data** (ledger entries, profile records, and later manifests):
  - Allowed: `# comments`, `key = "string"`, `key = <integer>`, `key = true|false`, single-line `key = ["a", "b"]`, and one-level `[table]` headers.
  - Not allowed: inline tables, multiline strings, dates or floats.
  - ASCII only, 16 KiB size cap per file.
  - Unknown, duplicate and missing keys are errors.
  - Any real TOML parser reads it, so it is not a private format. No TOML dependency is added: Phase 1 uses an sh+awk checker, and Phase 2 hand-writes a subset parser in the chosen language outside the core.

### Claim ledger (PROF-04, PROF-05)

- **D-19 Layout:** The ledger is a product artifact, not `.planning/`:
  - `ledger/entries/LED-<AREA>-<NNNN>.toml`: one file per entry.
  - `ledger/SCHEMA.md`
  - `ledger/REQUIRED`: one ID per line, which must exist.
  - `ledger/negative/`: must-fail control files, each paired with an expected reason code.
  - `ledger/samples/ok-*.toml`: must-pass control files.
- **D-20 IDs:**
  - `LED-<AREA>-<NNNN>`, with AREA in `PROF CPU VID AUD IRQ LOAD IO TOOL`.
  - The filename equals the ID.
  - IDs are never reused or renumbered; retired entries become `withdrawn`.
  - The research conflict ID goes in `origin = "SUMMARY C2"`, never in the ID itself.
- **D-21 Schema (required unless noted):**

  | Field | Content |
  |---|---|
  | `id` | Entry ID |
  | `statement` | One testable sentence |
  | `profile` | Must match a profile id under `profiles/` |
  | `status` | `open`, `assumed`, `supported`, `refuted`, `deferred` or `withdrawn` |
  | `sources` | `["LABEL\|ref\|detail", …]`, LABEL in `FACT CODE REPORT UNKNOWN` |
  | `evidence` | `["E0..E4\|ref", …]`; may be empty unless `supported` |
  | `disagreement` | The competing positions, or `"none"` |
  | `pinning_test` | `TEST:<id>`, `PENDING:<phase>` or `NONE:<reason>` |
  | `updated` | ISO date string |
  | `assumption` | Required when `assumed` |
  | `revisit_phase` | Required when `assumed` |
  | `withheld` | Optional: the claims not made while the entry is unsettled |
  | `origin` | Optional: the research conflict ID |

  `open` and `assumed` are both unsettled. `assumed` means a modelling assumption has been chosen. It is printed as such and is never phrased as fact.
- **D-22 `supported` rules, enforced by the checker:**
  - At least one E1, E2 or E3 evidence item. E0 and E4 never count, alone or combined (decision 17's proposed default; see D-29).
  - An E1 item needs a `FACT` source.
  - An E3 item must reference an owner-gate record.
  - `pinning_test` must be `TEST:`.
  - `PENDING:` is allowed only on non-supported entries.
  - From Phase 2, `TEST:` IDs resolve against the conformance inventory in both directions.
- **D-23 Seed entries and checker:**
  - `ledger/REQUIRED` lists the 9 mandated conflicts, each as an `assumed` entry with its modelling assumption (from SUMMARY C1-C8 and the PSG channel-numbering normalisation):
    - PSG clock
    - 262 vs 263 lines
    - VBlank/RCR IRQ position
    - dot-clock encoding
    - 7 MHz active width
    - `$1000` bit 6
    - HuCard byte order
    - copier-header rule
    - PSG channel numbering
  - Also seed C14 (which models ship the HuC6280A), because the profile references it.
  - `scripts/ledger-check` is POSIX sh+awk. It prints one reason code per violation, prints counts by status, and exits nonzero on any violation.
  - The gate runs it in three ways:
    - On `ledger/entries/`, which must pass.
    - On each `ledger/negative/*` with `--expect <REASON>`, each of which must fail with exactly that code.
    - On each `ledger/samples/ok-*`, each of which must pass.
  - The negative set: supported with E0 only; with E4 only; with E0+E4; E1 with only a REPORT source; supported with `PENDING:`; unknown status; missing field; unknown key; filename/ID mismatch; duplicate ID; assumed without an assumption.
  - The checker also cross-checks the profile record (D-27).
  - An empty entry directory or a missing REQUIRED ID is a rejection.
  - `SCHEMA.md` states plainly that the checker enforces structure, not truth.

### Decision records and provenance

- **D-24 Decision records:**
  - Location and naming: `docs/decisions/NNNN-slug.md`, with a `docs/decisions/README.md` index.
  - Planned records:
    - 0001 language-build (+ `.protocol.md`)
    - 0002 first-profile
    - 0003 api-abi
    - 0004 licence-visibility
    - 0005 fixture-rights
    - 0006 ci-provider
    - 0007 copied-code-provenance
    - 0008 evidence-tier-policy
  - Each record carries `research-ref: SUMMARY #N` for traceability. Records get new sequential numbers; roadmap decision numbers are not reused.
- **D-25 Record format:** Trimmed MADR with a small closed front matter.
  - Front matter fields:
    - `status`: `open`, `proposed-default`, `owner-recorded`, `superseded` or `withdrawn`
    - `blocks`: required unless `owner-recorded`
    - `default-in-force`: required when `proposed-default`
    - `owner-statement`: verbatim, plus where and when it was said; required for `owner-recorded`
    - `owner-date`
    - `supersedes` / `superseded-by`
    - `reopen-if`
  - Body sections: Context and gate; Options considered (with evidence labels); Decision; Consequences and what is not decided here.
  - An agent may draft a record and set `proposed-default`, but writes `owner-recorded` only with a verbatim owner statement. There is no git-author check, because the agent commits under the owner's identity and that check would prove nothing.
  - `PROJECT.md` Key Decisions rows for owner-gated items become pointers (`→ docs/decisions/0004-…`) plus the status word. The record is the single source of truth.
- **D-26 Copied-code provenance mechanism:**
  - Every copied file or block carries an in-file `SPDX-License-Identifier`, `SPDX-FileCopyrightText` and `Provenance: PROV-NNNN`.
  - A central `PROVENANCE.md` register uses one `key: value` block per copy. Fields:
    - `id`
    - `source-url`
    - `upstream-ref` (full SHA or tag)
    - `upstream-path` + lines
    - `license-spdx`
    - `license-url` (pinned)
    - `upstream-notice` (verbatim)
    - `local-paths`
    - `modifications`
    - `copied-on`
    - `copied-by`
    - `maintenance-owner`
    - `why-copy-not-dependency`
    - `sync-policy` (`frozen`, `diverged` or `tracked`)
    - `licence-decision`
  - `PROVENANCE.md` ships in Phase 1 with zero entries.
  - The SPDX licence allowlist for copies is derived from D-01 and kept in the record: MIT, Apache-2.0, BSD-2/3-Clause, Zlib, ISC, 0BSD, CC0-1.0. GPL and unclear licences are excluded.
  - A gate check (Phase 1 sh, or folded into Phase 2's gate) enforces:
    - Every marker resolves.
    - Every entry's paths exist and carry the marker.
    - Required fields are present.
    - The SPDX value is on the allowlist.
  - Staleness is a warning. Unmarked copies are covered by a PR checklist line and by the rule that emulator code is read-only reference.

### First-profile record (PROF-01)

- **D-27 Format and identity:**
  - The record is `profiles/pce-hucard-2btn/r1.toml`, written in the strict subset (D-18).
  - Each field is a `[table]` with:
    - `status`: `owner-selected` or `open`, nothing else.
    - `value`: only when `owner-selected`.
    - `allowed`: optional.
    - `proposal`: only when `open`. It is a labelled research default.
    - `ledger`: LED IDs.
    - `decision`: required when `owner-selected`; it points to `docs/decisions/0002-first-profile.md`.
    - `label`: human meaning only. It never feeds logic.
  - Identity is `(id, rev)` plus the canonical resolved text.
  - The id is a scope slug that deliberately omits region and revision, which are disputed in C6 and C14.
  - Any change to a guest-visible value, a status or an allowed set creates a new `rN.toml`. Once a rev is referenced, its file is frozen. Prose and comment edits do not bump the rev.
- **D-28 Field names describe observed behaviour (C6), and open fields carry no value:**
  - The fields are:
    - `media_slot`
    - `expansion_port`
    - `vdc_count`
    - `vpc`
    - `chip_revision` (one field, because CPU and PSG are one die)
    - `input_port1`
    - `io_1000_d6_value`
    - `io_1000_d7_value`
    - `lines_per_frame` (frame clocks are derived, not stored)
    - `psg_tick_divisor`
    - `content_byte_order`
    - `content_copier_header`
    - `powerup_policy`
  - The record also lists `unresolved_behaviours`, the ledger IDs for C3, C4 and C5.
  - Phase 2 contract, recorded here for the planner of Phase 2:
    - The core takes a typed `Profile` struct and never parses text.
    - `validate()` has no defaults. An open field is refused unless the run request binds it explicitly. The binding is recorded `bound_by = "run-request"` and is excluded from support claims.
    - Every run record embeds the full resolved field text, the bindings and the withheld-claims list.
    - A test asserts that the parsed file equals the built-in profile constant, using exhaustive destructuring.

### Evidence-tier policy

- **D-29 E4 is triage only (decision 17):** The checker enforces this already, because success criterion 3 requires it. Record 0008 is written as `proposed-default` and is not asked of the owner in this session. Its `default-in-force` is: E4 never backs correctness, and E1 needs a cited FACT source.

### Phase 1 gate entry point

- **D-30:** Phase 1 creates a minimal `scripts/check` that runs `ledger-check` (entries, negatives and samples), the decision-record front-matter check, and the provenance check. Phase 2 extends this same entry point into the full fast gate. It does not start a second one.

### Claude's Discretion

- The exact LED numbering, the wording of assumptions for the seed entries, and the reason-code names.
- Whether the decision, provenance and ledger checks are one awk script or three small ones behind `scripts/check`.
- The exact `hyperfine` command lines, the shape of the fixture generator script, and the C build presets for the spike.
- The order of records within the plan, with one constraint: the protocol commit (D-09) must come before any spike code, and the public-remote checkpoint (D-02) must come after the history audit.

### Deferred Ideas (OUT OF SCOPE)

- Full REUSE (`REUSE.toml` plus `reuse lint`) adoption. It is now possible because the licence is decided, but it adds a tool. Revisit in Phase 2 if the in-house SPDX/provenance check becomes a burden.
- Signed commits as owner attestation for `owner-recorded` decision flips. The owner can opt in later.
- An owner decision on evidence-tier policy (decision 17). It remains `proposed-default` (D-29).
- Remaining open owner gates that do not block Phase 1: LibFuturo access (7), hardware captures (10), timing fidelity and performance budget (11), host/package format (12), wasm promise (13), save-state, replay and debugger scope (15), PAL (16), release policy (18).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TOOL-01 | Language/build decision record from a measured Rust/Cargo vs C11/CMake spike; owner-confirmed choice recorded | "Toolchain spike" sections: machine inventory, install commands, hyperfine usage, installed-consumer mechanics (verified on the C path), fuzz findings (the Homebrew ASan hang), scoring/margin script, protocol-before-code check |
| PROF-01 | Named, versioned first-profile record; every field owner-selected or open | "First-profile record" section: field table, per-field proposals, a field-name-to-roadmap-criterion mapping, the `profile-check` rules and negatives |
| PROF-04 | Claim ledger with fields and a checker that rejects E0/E4-only "supported" | "Strict TOML subset + ledger checker" sections: a tested stage-1 parser and stage-2 rules (reason codes, `--expect` harness), negative-case layout |
| PROF-05 | Conflicts recorded as open ledger entries with a modelling assumption | "Seed ledger content": ten concrete entries (9 mandated + C14), each with competing positions, source labels, assumption, withheld claims |
</phase_requirements>

## Summary

Phase 1 is mostly a documents-plus-small-scripts phase with one measured experiment. The risky parts are not the hardware facts (they are transcriptions) but (1) shell/awk checkers that quietly fail open, (2) a spike whose pre-registered rubric decides the outcome before any code runs, (3) an irreversible public push, and (4) several machine-specific toolchain traps found by probing this Mac. The checkers can be built with zero new dependencies on POSIX `sh` + `awk`; a two-stage design (generic strict-TOML-subset parser, then schema rules) was prototyped in this session and passes on the macOS one-true-awk (`awk version 20200816`) with a negative-control harness that rejects each defect for exactly the intended reason.

Four machine findings change the plan. (a) Homebrew `llvm@21` and `llvm@18` AddressSanitizer binaries hang at process start on this macOS 26.6.2 arm64 machine, even for `int main(void){return 0;}`; Apple clang's ASan works; libFuzzer from `llvm@21` links and runs when ASan is not requested (`-fsanitize=fuzzer` or `fuzzer,undefined`), and Apple-clang ASan plus the `llvm@21` libFuzzer archive linked by hand also runs. The Rust `cargo-fuzz` default sanitizer (address) probably has the same upstream-runtime problem and needs a canary first. (b) A CMake `.pc` file generated with `configure_file` bakes in `/usr/local` as `prefix` instead of the `--prefix` given at `cmake --install`; use `prefix=${pcfiledir}/../..`. (c) `git cat-file --batch-check` silently reports `missing` for `rev-list --objects` lines unless `%(rest)` is in the format, so a naive history audit passes vacuously. (d) `gitleaks` 8.30.1 (MIT) and `trufflehog` (AGPL) are already installed here; the current 8-commit history is clean under gitleaks and under a blob/path/NUL audit.

**Primary recommendation:** Build `scripts/check` from three tiny awk/sh checkers over one shared strict-TOML-subset stage-1 parser; make every checker exit 2 on harness error and 1 on violation, require a stage sentinel so an awk crash cannot read as "no violations"; pre-register a scoring rubric (not just weights) in the protocol commit; write the history audit as a ~25-line `git`+`tr`+`awk` script (gitleaks as an optional one-time second opinion); and use a per-case directory for negative ledger controls so ID/filename rules do not contaminate every case.

## Architectural Responsibility Map

This phase has no runtime tiers. "Tier" here means the repo artifact that owns a responsibility.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Hand-edited record syntax (strict TOML subset) | `scripts/lib/toml-subset.awk` (stage 1) | Phase 2 language-native subset parser | One syntax owner; every schema checker consumes its flat records |
| Ledger/profile schema and `supported` rules | `scripts/lib/ledger-rules.awk`, `profile-rules.awk` (stage 2) | `ledger/SCHEMA.md` (prose) | Rules are code; SCHEMA.md only states them and says "structure, not truth" (D-23) |
| Negative-control calibration | `scripts/ledger-check --expect` run by `scripts/check` | `ledger/negative/<case>/EXPECT` | A control that stops failing means the gate broke |
| Decision-record state | `docs/decisions/*.md` front matter + `scripts/decision-check` | `.planning/PROJECT.md` pointer rows | Record is single source of truth (D-25) |
| Copied-code provenance | `PROVENANCE.md` + `scripts/provenance-check` | PR checklist line | Zero entries today; gate must still pass and still reject malformed blocks |
| Spike measurement | `spikes/tool-01/bench/*.sh` (one script per measured command) | hyperfine (dev-only) | hyperfine times a script, scripts hold the logic |
| Pre-publication audit | `scripts/audit-history` | gitleaks (one-time, optional) | Fewest tools; fail-closed on unreadable objects |
| Public remote creation | Owner (checkpoint:decision) | `gh repo create` | Irreversible; D-02 |

## Standard Stack

### Core (all already on this machine unless noted)

| Tool | Version | Purpose | Provenance |
|------|---------|---------|-----------|
| POSIX `sh` | `/bin/sh` is bash 3.2.57 (POSIX mode); `/bin/dash` also present | Checker wrappers; lint with `dash -n` | [VERIFIED: machine `sh --version`, `ls -la /bin/sh`, `which dash`] |
| `awk` | macOS one-true-awk `20200816`; no gawk, no mawk installed | Parsers and rules | [VERIFIED: machine `awk --version`; `which gawk mawk` empty] |
| `git` | /opt/homebrew/bin/git | History audit, protocol-order check | [VERIFIED: machine] |
| `shellcheck` | /opt/homebrew/bin/shellcheck | Optional lint (`-s sh`) | [VERIFIED: machine]; dev-only, not a gate dependency |
| `cmake` 4.4.3, `ninja` 1.13.2, `pkgconf` 3.0.9, Apple clang 21.0.0 (`clang-2100.1.1.101`) | as listed | C spike path | [VERIFIED: machine `cmake --version`, `ninja --version`, `clang --version`, `brew list --versions`] |
| Homebrew `llvm@21` 21.1.8, `llvm@18` 18.1.8 | as listed | libFuzzer runtime for C fuzzing (explicit path only, D-17) | [VERIFIED: machine `brew list --versions`; `libclang_rt.fuzzer_osx.a` exists at `/opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/`] |
| `gh` 2.101.0, logged in as `szTheory` | | Public remote creation | [VERIFIED: machine `gh auth status`] |
| `gitleaks` 8.30.1 (MIT) | | Optional one-time history second opinion | [VERIFIED: machine `which gitleaks`, `brew info gitleaks`] |
| `python3` 3.14.4 at `/opt/homebrew/bin/python3` (has `tomllib`); `/usr/bin/python3` is 3.9.6 (no `tomllib`) | | Optional dev-only TOML cross-check (`python3 -I`) | [VERIFIED: machine; `/usr/bin/python3 -I -c 'import tomllib'` -> `ModuleNotFoundError`] |

### To install for the spike (not installed; do not make any of them a gate dependency)

| Tool | Version | Install | Notes |
|------|---------|---------|-------|
| `rustup` (brew, keg-only) | 1.29.3, Apache-2.0 OR MIT | `brew install rustup` then `export PATH="$(brew --prefix rustup)/bin:$HOME/.cargo/bin:$PATH"` | Homebrew caveat: keg-only; "This formula no longer provides `rustup-init`" [VERIFIED: machine `brew info rustup`]. Avoids `curl \| sh`. |
| Rust stable pinned | 1.99.1 (`58cf8f9c3 2026-10-13`, channel date 2026-10-15) | `rustup toolchain install 1.99.1 --profile minimal -c rustfmt -c clippy -t wasm32-wasip1` | Manifest for 1.99.1 and `rust-std` for `wasm32-wasip1` both `available = true` [VERIFIED: machine `curl static.rust-lang.org/dist/channel-rust-1.99.1.toml`] |
| `rust-toolchain.toml` | n/a | `[toolchain]` with `channel = "1.99.1"`, `components = ["rustfmt","clippy"]`, `targets = ["wasm32-wasip1"]`, `profile = "minimal"` | Format [CITED: rust-lang.github.io/rustup/overrides.html] |
| Dated nightly (fuzz + Miri only) | `nightly-2026-11-09` (published today) | `rustup toolchain install nightly-2026-11-09 --profile minimal -c miri -c rust-src` | `miri-preview` for aarch64-apple-darwin and `rust-src` are available in that nightly manifest [VERIFIED: machine `curl .../dist/2026-11-09/channel-rust-nightly.toml`]. If a later date is chosen, re-check Miri availability. |
| `cargo-fuzz` | 0.13.2 (2026-06-09), MIT OR Apache-2.0 | `cargo +nightly-2026-11-09 install cargo-fuzz --version 0.13.2 --locked` | [VERIFIED: crates.io API]; legitimacy seam returned OK |
| `hyperfine` | 2.0.0 (2026-10-07), Apache-2.0 OR MIT | `brew install hyperfine` | Formula exists [VERIFIED: machine `brew info hyperfine`] |
| `wasmtime` | 50.0.1, Apache-2.0 WITH LLVM-exception | `brew install wasmtime` | Formula exists [VERIFIED: machine `brew info wasmtime`] |
| wasi-sdk (C wasm path) | 34.0 (2026-08-25) | No Homebrew formula/cask named `wasi-sdk`. GitHub release asset `wasi-sdk-34.0-arm64-macos.tar.gz`, 180,322,002 bytes. Homebrew has `wasi-libc` 34 and `wasi-runtimes` 23.1.5 but they need a matching clang+lld that is not installed. | [VERIFIED: machine `brew info --cask wasi-sdk` -> "No Cask"; GitHub releases API]. Per D-12 record "not attempted, install cost 180 MB download + untar" unless time permits. |

**Version verification done this session:** Rust 1.99.1 and the 2026-11-09 nightly, cargo-fuzz 0.13.2, hyperfine 2.0.0, wasmtime 50.0.1, rustup 1.29.3, wasi-sdk 34.0 asset size, SPDX list v3.29.0. [VERIFIED: machine, curl/brew/GitHub API on 2026-11-09]

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `scripts/audit-history` (git+tr+awk) | `gitleaks git` | Gitleaks finds credentials by rule set, not ROM-like blobs; it does not replace the blob audit. Use once at the checkpoint (already installed), not as a gate dependency. `trufflehog` is AGPL-3.0-only: do not adopt [VERIFIED: machine `brew info trufflehog`]. |
| In-house SPDX/provenance check | `reuse lint` (REUSE 3.3, `REUSE.toml`) | Deferred by CONTEXT. REUSE expects `LICENSES/<SPDX>.txt`, not root `LICENSE-MIT`; adopting later means adding that directory [CITED: reuse.software/spec-3.3]. |
| awk checkers | python `tomllib` | Python is not a gate dependency (owner preference; `/usr/bin/python3` lacks it). Use only as a dev-time cross-check. |
| Per-test negative **files** with one shared ID | Per-case negative **directories** | Recommended refinement of D-19 (see Pitfall 5). Each case is a mini-ledger with an `EXPECT` file. |

## Package Legitimacy Audit

Only one external package registry is touched, and only for the spike's Rust fuzz harness (outside the product tree).

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| cargo-fuzz | crates.io | since 2017-02 | ~169k/wk | github.com/rust-fuzz/cargo-fuzz | OK | Approved (tool, nightly only) |
| libfuzzer-sys | crates.io | long-lived | ~1.67M/wk | github.com/rust-fuzz/libfuzzer | OK | Approved for `spikes/tool-01/rust/fuzz/` only. License is `(MIT OR Apache-2.0) AND NCSA` (bundles libFuzzer); list it in "tools required" [VERIFIED: crates.io API, `gsd-tools query package-legitimacy check --ecosystem crates`] |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none
Homebrew formulae (`rustup`, `hyperfine`, `wasmtime`) and the GitHub-released wasi-sdk are not registry packages; they were verified by `brew info` / release API. No npm/PyPI package is recommended. The product dependency tree stays empty.

## Architecture Patterns

### System Architecture Diagram (the Phase 1 gate; Phase 2 extends the same entry point, D-30)

```
                         scripts/check  (sh; exit 0 / 1 violations / 2 harness error)
                                   |
   +-------------------------------+--------------------------------------------------+
   |                               |                                                  |
 ledger-check                  decision-check                                   provenance-check
   |                               |                                                  |
   | entries/ must pass            | docs/decisions/NNNN-*.md front matter            | PROVENANCE.md blocks
   | negative/<case>/ must fail    |  closed fields, conditional requireds            |  required fields, SPDX allowlist
   |   with EXPECT code set        |  README index == records                         |  markers <-> register
   | samples/ok-* must pass        | negatives: tests/decision/*  (EXPECT)            | negatives: tests/provenance/* (EXPECT)
   | REQUIRED ids exist (+origin)  |                                                  |
   | profile cross-check           |                                                  |
   v                               v                                                  v
 toml-subset.awk (stage 1: syntax, flat TAB records, ERR lines, sentinel)   [ledger + profile only; MADR front matter and
 ledger-rules.awk / profile-rules.awk (stage 2: schema, supported rules)     PROVENANCE use their own tiny `key: value` awk]
                                   |
   scripts/check also: (a) protocol-before-spike ancestry check, (b) 0001 results-table completeness,
   (c) licence-file hash check, (d) audit-history (on demand, before publication), (e) optional toml cross-check
```

### Recommended Project Structure

```
LICENSE-MIT  LICENSE-APACHE  LICENSE-0BSD  README.md  PROVENANCE.md  .gitignore
docs/decisions/{README.md,0001-language-build.protocol.md,0001-language-build.md,0002..0008-*.md}
ledger/{SCHEMA.md,REQUIRED,entries/LED-*.toml,negative/<case>/{LED-*.toml,EXPECT},samples/ok-*.toml}
profiles/pce-hucard-2btn/r1.toml
scripts/{check,ledger-check,profile-check,decision-check,provenance-check,audit-history,toml-crosscheck}
scripts/lib/{toml-subset.awk,ledger-rules.awk,profile-rules.awk}
tests/{decision,provenance,profile}/<case>/...        # negative controls for the other checkers
spikes/tool-01/{SPEC.md,fixtures.manifest,gen-fixtures.sh,gates.sh,score.awk,bench/,rust/,c/,consumer/}   # throwaway, tagged then removed (D-16)
```

`.gitignore` must exist before the first spike build: `target/`, `build/`, `prefix/`, `*.a`, `spikes/tool-01/out/`, `fuzz/artifacts/`, `fuzz/corpus/`. Generated fixtures (64 KiB zero-fill, truncations, PRNG data) are ROM-sized blobs; commit only the generator and `fixtures.manifest` (names, sizes, sha256), never the bytes, or the pre-publication audit and Phase 2's FIX-05 repo check will flag them [VERIFIED: machine, `blobaudit` flags any 8 KiB-multiple blob; 65536 is one].

### Pattern 1: Two-stage strict-subset checker (prototyped, runs on BWK awk 20200816)

**What:** Stage 1 reads one file and emits flat records `TABLE<TAB>KEY<TAB>TYPE<TAB>VALUE<TAB>LINE` or `ERR<TAB>CODE<TAB>LINE<TAB>detail`. Stage 2 is schema-specific awk that consumes records. A `sh` wrapper handles files, size cap, exit codes and `--expect`.
**When to use:** ledger entries, profile records now; manifests in Phase 2.
**Verified behaviour (this session):** 15 malformed inputs each produced the intended single code (`BAD_VALUE` for floats/dates/single quotes/inline tables/bare words, `BAD_ARRAY` for non-string or trailing-comma arrays, `BAD_STRING` for escapes, `TRAILING_GARBAGE`, `SYNTAX` for uppercase keys, `DUP_KEY`, `DUP_TABLE`, `NON_ASCII_OR_CTRL`); an accepted file also parsed under `python3.14 -I tomllib` with identical content [VERIFIED: machine]. `shellcheck -s sh` and `dash -n` were clean on the wrapper.

Stage 1 (`scripts/lib/toml-subset.awk`; this is the tested file, copy it):

```awk
# toml-subset.awk - strict TOML subset reader (POSIX awk only; no gawk/mawk/BWK extensions).
# in : one file. out (stdout): flat records  TABLE<TAB>KEY<TAB>TYPE<TAB>VALUE<TAB>LINE
#                              or errors     ERR<TAB>CODE<TAB>LINE<TAB>detail
# TABLE is "" for root keys.  TYPE: s (string) i (integer) b (bool) a (array of strings; VALUE = elements joined by US=\037)
function err(code, detail) { printf "ERR\t%s\t%d\t%s\n", code, NR, detail }
# parse one quoted string starting at s[pos]; sets PV (content) and PE (index after closing quote); returns 1 ok, 0 bad
function qstr(s, pos,    i, c, out) {
  if (substr(s, pos, 1) != "\"") return 0
  out = ""
  for (i = pos + 1; i <= length(s); i++) {
    c = substr(s, i, 1)
    if (c == "\"") { PV = out; PE = i + 1; return 1 }
    if (c == "\\") return 0            # subset: no escapes at all
    out = out c
  }
  return 0
}
BEGIN { FS = "\n"; table = ""; seen_tab[""] = 1 }
{
  line = $0
  if (length(line) > 400) { err("LINE_TOO_LONG", ""); next }
  if (line ~ /[^ -~\t]/) { err("NON_ASCII_OR_CTRL", ""); next }   # CR, bytes > 126, other control chars
  if (line ~ /^[ \t]*(#.*)?$/) next
  if (line ~ /^\[[a-z][a-z0-9_]*\][ \t]*(#.*)?$/) {
    t = line; sub(/^\[/, "", t); sub(/\].*$/, "", t)
    if (t in seen_tab) err("DUP_TABLE", t)
    seen_tab[t] = 1; table = t; next
  }
  if (!match(line, /^[a-z][a-z0-9_]*[ \t]*=[ \t]*/)) { err("SYNTAX", "not key = value / [table] / comment"); next }
  key = substr(line, 1, RLENGTH); sub(/[ \t]*=[ \t]*$/, "", key)
  rest = substr(line, RLENGTH + 1)
  id = table SUBSEP key
  if (id in seen_key) { err("DUP_KEY", key); next }
  seen_key[id] = 1
  if (rest ~ /^"/) {
    if (!qstr(rest, 1)) { err("BAD_STRING", key); next }
    tail = substr(rest, PE); if (tail !~ /^[ \t]*(#.*)?$/) { err("TRAILING_GARBAGE", key); next }
    printf "%s\t%s\ts\t%s\t%d\n", table, key, PV, NR; next
  }
  if (rest ~ /^(true|false)[ \t]*(#.*)?$/) { v = rest; sub(/[ \t#].*$/, "", v); printf "%s\t%s\tb\t%s\t%d\n", table, key, v, NR; next }
  if (rest ~ /^-?[0-9]+[ \t]*(#.*)?$/)      { v = rest; sub(/[ \t#].*$/, "", v); printf "%s\t%s\ti\t%s\t%d\n", table, key, v, NR; next }
  if (rest ~ /^\[/) {
    p = 2; joined = ""; n = 0; ok = 1
    while (1) {
      while (substr(rest, p, 1) == " " || substr(rest, p, 1) == "\t") p++
      if (substr(rest, p, 1) == "]") { if (n > 0 && lastcomma) ok = 0; p++; break }   # no trailing comma
      if (!qstr(rest, p)) { ok = 0; break }
      joined = (n++ ? joined "\037" : "") PV; p = PE
      while (substr(rest, p, 1) == " " || substr(rest, p, 1) == "\t") p++
      lastcomma = 0
      if (substr(rest, p, 1) == ",") { p++; lastcomma = 1 }
      else if (substr(rest, p, 1) != "]") { ok = 0; break }
    }
    if (ok && substr(rest, p) !~ /^[ \t]*(#.*)?$/) ok = 0
    if (!ok) { err("BAD_ARRAY", key); next }
    printf "%s\t%s\ta\t%s\t%d\n", table, key, joined, NR; next
  }
  err("BAD_VALUE", key)   # inline tables, floats, dates, multiline strings, single quotes, bare words
}
```

Stage 2 (`scripts/lib/ledger-rules.awk`; tested file, codes are the Claude's-discretion reason-code names):

```awk
# ledger-rules.awk - schema + supported-rules for ONE ledger entry. input: toml-subset.awk records (TAB separated).
# -v fname=<basename without .toml>   out: "CODE<TAB>line<TAB>detail" per violation
BEGIN {
  FS = "\t"
  n = split("id statement profile status sources evidence disagreement pinning_test updated", req, " ")
  m = split("assumption revisit_phase withheld origin", opt, " ")
  for (i = 1; i <= n; i++) known[req[i]] = 1
  for (i = 1; i <= m; i++) known[opt[i]] = 1
}
function viol(code, line, detail) { printf "%s\t%s\t%s\n", code, line, detail }
$1 == "ERR" { viol($2, $3, $4); next }                 # syntax-stage errors pass straight through
{
  if ($1 != "") { viol("UNKNOWN_TABLE", $5, $1); next }
  key = $2; typ[key] = $3; val[key] = $4; ln[key] = $5
  if (!(key in known)) viol("UNKNOWN_KEY", $5, key)
}
function nth(s, k,    a) { split(s, a, "\037"); return a[k] }
function cnt(s) { return (s == "") ? 0 : split(s, tmp, "\037") }
END {
  for (i = 1; i <= n; i++) if (!(req[i] in typ)) viol("MISSING_FIELD", 0, req[i])
  if ("id" in typ) {
    if (val["id"] !~ /^LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-[0-9][0-9][0-9][0-9]$/) viol("BAD_ID", ln["id"], val["id"])
    else if (val["id"] != fname) viol("ID_FILENAME_MISMATCH", ln["id"], val["id"] " vs " fname)
  }
  st = ("status" in typ) ? val["status"] : ""
  if ("status" in typ && st !~ /^(open|assumed|supported|refuted|deferred|withdrawn)$/) viol("UNKNOWN_STATUS", ln["status"], st)
  if ("updated" in typ && val["updated"] !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$/) viol("BAD_DATE", ln["updated"], val["updated"])
  hasfact = 0
  if ("sources" in typ) {
    k = cnt(val["sources"]); if (k == 0) viol("NO_SOURCE", ln["sources"], "")
    for (i = 1; i <= k; i++) {
      e = nth(val["sources"], i); split(e, p, "|")
      if (p[1] !~ /^(FACT|CODE|REPORT|UNKNOWN)$/ || p[2] == "") viol("BAD_SOURCE", ln["sources"], e)
      if (p[1] == "FACT") hasfact = 1
    }
  }
  q = 0; e1 = 0; e3bad = 0
  if ("evidence" in typ) {
    k = cnt(val["evidence"])
    for (i = 1; i <= k; i++) {
      e = nth(val["evidence"], i); split(e, p, "|")
      if (p[1] !~ /^E[0-4]$/ || p[2] == "") { viol("BAD_EVIDENCE", ln["evidence"], e); continue }
      if (p[1] == "E1" || p[1] == "E2" || p[1] == "E3") q++      # E0 and E4 never count
      if (p[1] == "E1") e1 = 1
      if (p[1] == "E3" && p[2] !~ /^docs\/decisions\/[0-9][0-9][0-9][0-9]-/) e3bad = 1
    }
  }
  pin = ("pinning_test" in typ) ? val["pinning_test"] : ""
  if ("pinning_test" in typ && pin !~ /^(TEST:.+|PENDING:[0-9]+|NONE:.+)$/) viol("BAD_PINNING", ln["pinning_test"], pin)
  if (st == "assumed") {
    if (!("assumption" in typ) || val["assumption"] == "") viol("ASSUMED_NO_ASSUMPTION", 0, "")
    if (!("revisit_phase" in typ)) viol("ASSUMED_NO_REVISIT", 0, "")
  }
  if (st == "supported") {
    if (q == 0) viol("SUPPORTED_NO_QUALIFYING_EVIDENCE", ln["status"], "needs E1/E2/E3; E0 and E4 never count")
    if (e1 && !hasfact) viol("E1_NEEDS_FACT_SOURCE", ln["evidence"], "")
    if (e3bad) viol("E3_NEEDS_OWNER_GATE", ln["evidence"], "")
    if (pin ~ /^PENDING:/) viol("SUPPORTED_PENDING_TEST", ln["pinning_test"], "")
    else if (pin !~ /^TEST:/ && pin != "") viol("SUPPORTED_NEEDS_TEST", ln["pinning_test"], pin)
  }
}
```

Wrapper (`scripts/ledger-check`; tested for single-file/`--expect` use):

```sh
#!/bin/sh
# usage: ledger-check [--expect CODE] FILE...      exit 0 ok | 1 violations (or expectation not met) | 2 usage/harness error
set -eu
LC_ALL=C; export LC_ALL
here=$(cd "$(dirname "$0")" && pwd)
expect=""
if [ "${1:-}" = "--expect" ]; then expect=${2:?}; shift 2; fi
[ $# -ge 1 ] || { echo "usage: ledger-check [--expect CODE] FILE..." >&2; exit 2; }
out=$(mktemp); trap 'rm -f "$out"' EXIT
for f in "$@"; do
  [ -f "$f" ] || { echo "HARNESS_NO_FILE $f" >&2; exit 2; }
  size=$(wc -c < "$f" | tr -d ' ')
  [ "$size" -gt 0 ] || { echo "EMPTY_FILE	$f" >> "$out"; continue; }
  [ "$size" -le 16384 ] || { echo "FILE_TOO_BIG	$f" >> "$out"; continue; }
  base=$(basename "$f" .toml)
  awk -f "$here/toml-subset.awk" "$f" | awk -v fname="$base" -f "$here/ledger-rules.awk" | sed "s|^|$f	|" >> "$out"
done
if [ -n "$expect" ]; then
  codes=$(awk -F'\t' '{print $2}' "$out" | sort -u | tr '\n' ' ')
  if [ "$codes" = "$expect " ]; then exit 0; fi
  echo "EXPECT_MISMATCH wanted '$expect' got '${codes:-<none>}'" >&2; exit 1
fi
if [ -s "$out" ]; then cat "$out"; exit 1; fi
```

What the prototype does NOT yet have and the plan must add: directory mode (glob `*.toml`, empty directory is `EMPTY_LEDGER`), `ledger/REQUIRED` existence and `origin` match, cross-file `DUP_ID`, `profile` must name an existing `profiles/<id>/` directory, a stage sentinel (Pitfall 1), `--expect` accepting a comma-separated code set, ENVIRON instead of `-v` for names (Pitfall 7), counts by status on stdout, and the profile checker.

### Pattern 2: Negative case = directory with an `EXPECT` file

`ledger/negative/<case>/LED-CPU-0001.toml` (a copy of a valid sample with exactly one defect) plus `ledger/negative/<case>/EXPECT` holding the exact code set, e.g. `SUPPORTED_NO_QUALIFYING_EVIDENCE`. `scripts/check` runs `ledger-check --expect "$(cat EXPECT)" <case>/`. Exit 0 means "rejected for exactly that reason"; exit 1 means the control is broken (passes or fails for another reason); exit 2 is a harness error and must fail the gate distinctly. Prototype result: ten cases (E0-only, E4-only, E0+E4, E1 with REPORT-only source, supported with `PENDING:`, unknown status, missing field, unknown key, assumed without assumption, ID/filename mismatch) each rejected for the intended single code [VERIFIED: machine]. Git does not track empty directories: the "empty entries directory" negative must hold a `.gitkeep` and the checker must glob `*.toml` only.

Cases that need a set rather than a file: duplicate ID (two files, same `id`; expect `DUP_ID,ID_FILENAME_MISMATCH`), missing REQUIRED ID (`REQUIRED_MISSING`), empty directory (`EMPTY_LEDGER`).

### Pattern 3: One shell script per measured command

hyperfine times a command string; keep logic in `spikes/tool-01/bench/{build-c.sh,build-rust.sh,edit-comment.sh,edit-real.sh,gate-c.sh,gate-rust.sh}` so the pre-registered protocol can name each measured command byte-for-byte. Pass `--shell=default` (or `-S sh`) explicitly rather than rely on defaults: the hyperfine README says the default is no intermediate shell, while `--prepare`/`--cleanup`/`--conclude` run through a shell [CITED: raw.githubusercontent.com/sharkdp/hyperfine README]. Example: `hyperfine --warmup 1 --runs 10 --shell=default --prepare 'rm -rf build' 'sh bench/build-c.sh'`. hyperfine's `--setup` runs once per command set, `--prepare` before each run [CITED: same README]; the README does not state prepare-versus-warmup ordering, so state the observed behaviour in the protocol if it matters.

### Anti-Patterns to Avoid

- **Pipeline as the only status** (`awk … | awk …`): a failing stage 1 yields empty stage-2 input and "no violations". Require the sentinel or capture to a temp file and test each exit status.
- **`awk -v name="$f"`**: `-v` processes backslash escapes; pass arbitrary strings through `ENVIRON`.
- **Scoring after the run**: weights alone do not pre-register a decision; the rubric anchors must be in the protocol commit.
- **`awk rand()` for fixtures**: sequences differ per awk implementation. Use an integer LCG (see Code Examples).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Full TOML parsing | a general TOML parser | The strict subset (D-18) plus optional `python3 -I tomllib` cross-check | The subset is deliberately tiny; anything else is `BAD_VALUE` |
| Secret scanning | regexes for every token format | `gitleaks git` once, manually | Rule set maintained upstream; MIT; already installed |
| Licence texts | retyped or "reconstructed" licence text | Apache text byte-for-byte from apache.org (sha256 below); MIT and 0BSD from the SPDX list with placeholders filled | Verbatim text is the legal artifact |
| Benchmark statistics | `time` loops with manual medians | hyperfine `--export-json` | Handles warmup, outliers, mean/median/min/max |
| Timeout wrapper | custom `timeout` | libFuzzer `-max_total_time=60`; `perl -e 'alarm N; exec @ARGV'` where a hard cap is needed (macOS has no `timeout`; `/opt/homebrew/bin/timeout` exists only via coreutils) | [VERIFIED: machine `which timeout perl`] |
| Weighted-score arithmetic | hand-computed totals | a 15-line `score.awk` reading a scores file | Removes arithmetic mistakes; testable with a tie fixture |

**Key insight:** every check here is small enough that a hand-written awk script is cheaper than a dependency, but each one needs a negative control, because the failure mode is a silent pass.

## Common Pitfalls

### Pitfall 1: Checker passes because awk produced nothing
**What goes wrong:** On an awk that rejects a construct, stage 1 prints a syntax error to stderr and nothing to stdout; the wrapper sees zero violations.
**Why it happens:** Pipelines hide upstream exit status; no `pipefail` in POSIX sh.
**How to avoid:** Stage 1 prints a final `END<TAB>n` line; stage 2 reports `NO_SENTINEL` if absent. Run ok-samples (must pass) and negatives (must fail with the expected code) every time: a dead checker breaks both.
**Warning signs:** All negatives report "CONTROL BROKEN"; checker time near zero.

### Pitfall 2: awk dialect drift (macOS BSD/one-true-awk vs gawk/mawk on Linux CI)
**What goes wrong:** A script written against macOS awk fails or mis-parses on Linux, or the reverse.
**How to avoid (rules used by the prototype; the "awk" column is by the POSIX awk definition and common behaviour, not re-verified on gawk/mawk here [ASSUMED]):**
- No regex interval expressions (`{n,m}`): old mawk and older BWK awk do not support them. Spell out `[0-9][0-9][0-9][0-9]`.
- No `\s \d \w`, no `[[:alpha:]]` classes (use explicit ranges), always `LC_ALL=C` so ranges are bytes and non-ASCII is detectable.
- No `nextfile`, `length(array)`, `gensub`, `strftime`, `systime`, `PROCINFO`, `asort`, `BEGINFILE`, `FPAT`, `IGNORECASE`, `RS` regex, `\y`. Clear arrays with `split("", a)`, count elements yourself.
- `split(s, a, "|")` with a single non-space character separator is used literally by all awks; use `"\037"` as the internal joiner.
- Use regex **literals** (`/.../`), not strings, to avoid double-escaping differences.
- Bracket expressions: write `[^"\\]` (works in all) and avoid `\]` inside brackets.
- `printf "%c", 0` is not portable; never emit binary from awk (see Pitfall 8).
- `match()`/`RSTART`/`RLENGTH`/`substr`/`sub`/`gsub`/`index`/`split` are POSIX and used freely.
- macOS ships one-true-awk 20200816 here; newer macOS may ship a UTF-8-aware version, so `LC_ALL=C` stays mandatory.
**Warning signs:** a CI-only failure; a pattern with `{`; any `\s`.
**Mitigation to consider:** the scheduled Linux job (D-03) runs `scripts/check` on gawk and, if desired, mawk; Phase 2 decides the matrix.

### Pitfall 3: sh and tool portability on macOS vs Linux
`sed -i` differs (avoid: write to a temp file); `wc -c` pads with spaces on macOS (`tr -d ' '`); `date +%N`, `readlink -f`, `grep -P`, `stat -c`, `sha256sum` are not baseline on macOS (use `shasum -a 256`, available on both [VERIFIED: machine `which shasum`; `sha256sum` is at `/sbin/sha256sum` here but not assumed elsewhere]); `timeout` absent; shell glob that matches nothing returns the literal pattern (guard with `[ -e "$f" ] || continue`); `/bin/sh` here is bash 3.2 so no arrays, no `[[ ]]`, no `local` assumptions; lint with `dash -n` and `shellcheck -s sh` [VERIFIED: both clean on the prototype wrapper]. **macOS `/bin/sh` (bash 3.2) mis-parses a `case` statement placed inside `$( … )`** (`syntax error near unexpected token `;;`` while `dash -n` accepted the same file): put such code in a function defined outside the command substitution, or use the leading-paren `(pattern)` form [VERIFIED: machine; reproduced on the first draft of `audit-history`]. So lint with `dash -n` AND run the script under `/bin/sh` itself.

### Pitfall 4: `git cat-file --batch-check` reports `missing` for `rev-list --objects` output
**What goes wrong:** `git rev-list --objects --all | git cat-file --batch-check='%(objecttype) %(objectname) %(objectsize)'` treats the whole input line (`<hash> <path>`) as the object name and prints `missing` for every path-bearing line. A loop that only inspects `blob` lines then reports "clean".
**Reproduced:** with a synthetic repo containing an 8 KiB zero-filled `x.pce`, the format without `%(rest)` printed `commit …`, then `<hash> x.pce missing`; adding `%(rest)` to the format fixed it [VERIFIED: machine]. A first-pass audit of the real repo in this session was vacuous for this reason and was redone.
**How to avoid:** include `%(rest)` in the format, and fail the audit on any `missing` object. A tested script is in Code Examples. Also: `while read` in a pipeline runs in a subshell, so set a flag by capturing output (`[ -n "$out" ]`), not by assigning a variable. A shell cannot pass NUL bytes (`$'\x00'` becomes empty and matches everything); detect NULs by comparing `wc -c` before and after `tr -d '\000'`.

### Pitfall 5: Negative-control files contaminated by filename/ID rule
**What goes wrong:** `neg/e0-only.toml` has `id = "LED-CPU-0001"` so every negative also trips `ID_FILENAME_MISMATCH`, and "fails with exactly that code" can never hold.
**How to avoid:** per-case directories named by ID inside (Pattern 2). Reproduced in this session before the fix [VERIFIED: machine].

### Pitfall 6: CMake `.pc` bakes the configure-time prefix
**What goes wrong:** `configure_file(duospool.pc.in … @ONLY)` with `prefix=@CMAKE_INSTALL_PREFIX@` produced `prefix=/usr/local` even though `cmake --install build --prefix <dir>` installed into `<dir>`; a consumer using the `.pc` would link against the wrong tree or an unrelated installed copy [VERIFIED: machine].
**How to avoid:** `prefix=${pcfiledir}/../..` in the `.pc` (pkgconf resolved it to the installed prefix) [VERIFIED: machine]. The same applies to the hand-written Rust-path `.pc`: do not hard-code an absolute prefix.

### Pitfall 7: `awk -v` escape processing and filenames
`awk -v fname="$base"` interprets backslashes in the value. Filenames here are controlled, but use `fname="$base" awk '… ENVIRON["fname"]'` so the pattern is safe for Phase 2 manifests.

### Pitfall 8: Binary fixtures from awk
`printf "%c", 0` works on the one-true-awk here (`41 00 42`) [VERIFIED: machine] but is implementation-specific elsewhere [ASSUMED]. Portable, verified recipe: awk integer LCG prints octal escapes as text, then `printf "$(cat file)"` writes bytes. awk+dash-`printf` and an independent perl implementation both produced sha256 prefix `ab0b5936cf56fca6` for 65,536 bytes seeded 12345 [VERIFIED: machine]. Do not use `rand()`.

### Pitfall 9: Spike fixtures look like ROMs to the audit and to FIX-05
Zero-fill 65,536 bytes is a multiple of 8 KiB. Keep bytes out of git (generated into an ignored `out/` dir), record names/sizes/sha256 in `fixtures.manifest`. If any generated binary must be committed, add an explicit allowlist entry with a reason to the audit.

### Pitfall 10: Homebrew LLVM AddressSanitizer hangs at startup on this Mac
**What goes wrong:** `clang -fsanitize=address t.c` from `/opt/homebrew/opt/llvm@21/bin/clang` (21.1.8) and from `llvm@18` produce binaries that never reach `main` (even `int main(void){return 0;}`); a `sample` of the process shows it inside dyld initializer execution; `ASAN_OPTIONS=verbosity=1` prints only "libc interceptors initialized". Same result with the sandbox disabled. Apple clang's `-fsanitize=address` binary exits 0. A web search found no matching upstream report [VERIFIED: machine; CITED: websearch returned no match].
**What still works [VERIFIED: machine]:** `llvm@21 clang -fsanitize=fuzzer fz.c` and `-fsanitize=fuzzer,undefined` link, run, and found the planted trap within seconds (exit 133 = SIGTRAP); `/usr/bin/clang -fsanitize=fuzzer-no-link,address fz.c /opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/libclang_rt.fuzzer_osx.a -lc++` linked and ran and also found the trap. Apple clang `-fsanitize=fuzzer` still has no runtime (STACK.md, unchanged).
**Effect on the spike:** D-17's question "does libFuzzer link with llvm@21" is answered yes; C fuzz recipe = `llvm@21` `-fsanitize=fuzzer,undefined` (no ASan) for the 60-second gate; a separate sanitizer row uses Apple clang `-fsanitize=address,undefined`. Rust: `cargo-fuzz` defaults to the address sanitizer using rustc's own LLVM runtime; whether it hangs here is not tested (no Rust installed) [ASSUMED risk]. First action in the Rust fuzz step: a 10-second ASan canary (`perl -e 'alarm 10; exec @ARGV' ./hello`); if it hangs, run `cargo +nightly-2026-11-09 fuzz run <target> -s none -- -max_total_time=60` and record the sanitizer difference as an ergonomics cost (fuzz and sanitizer criterion, weight 15).
**Warning signs:** a fuzz run with no output and 100% CPU for minutes.

### Pitfall 11: A fuzz run that "does not crash" can be vacuous
Add a harness liveness control: a build flag or feature that plants a deliberate bug reachable by a 4-byte prefix; the harness must find it in under 60 s. The C demo found its planted trap in seconds [VERIFIED: machine]. Otherwise a harness that never calls the loader passes the gate.

### Pitfall 12: Absolute paths embedded in installed artifacts
**C [VERIFIED: machine]:** a Debug static library built from `<tmp>/src/core.c` contained the absolute source and build directory in 5 string occurrences (`strings -a lib.a | grep -c <tmp>` = 5; DWARF `DW_AT_name`/`comp_dir` and the `assert` `__FILE__` string). With `CMAKE_C_FLAGS="-ffile-prefix-map=<src>=/src -ffile-prefix-map=<build>=/build -fdebug-prefix-map=<src>=/src"` the count was 0 and the strings read `/src/core.c`, `/build`. Use `grep -c -a "$PATH"` on the `.a` and `.pc` rather than depending on `strings`.
**Rust [CITED: doc.rust-lang.org/rustc/command-line-arguments.html for `--remap-path-prefix FROM=TO`]:** set `RUSTFLAGS="--remap-path-prefix=$WORK=/src --remap-path-prefix=$HOME/.cargo=/cargo --remap-path-prefix=$(rustc --print sysroot)=/rustc"` for the staticlib build; panic `Location` strings and DWARF carry crate paths. Cargo's `profile.*.trim-paths` stability could not be confirmed from the docs fetched and is **not recommended** until checked [ASSUMED]. Distributed `std` paths appear as `/rustc/<hash>/…` by default [ASSUMED]; the gate must grep for the work dir, the real path of the work dir (`pwd -P`, macOS `/private/var/...`), and `$HOME`.
**macOS temp path trap:** `mktemp -d` yields `/var/folders/...` which is a symlink to `/private/var/folders/...`; check both spellings.

### Pitfall 13: Rust staticlib link flags on macOS are unverified here
Capture them from the toolchain instead of remembering them: `cargo rustc --release --crate-type staticlib -- --print native-static-libs` prints the exact `native-static-libs:` line; put those flags into the hand-written `.pc` `Libs.private`/`Libs`. Expected content on macOS is `-lSystem -lc -lm` (possibly `-liconv` and frameworks for std-bearing code) [ASSUMED; rustc docs fetched did not describe the option]. Also set `panic = "abort"` in the profile, and note that `cargo test` ignores it.

### Pitfall 14: Making the source tree "unavailable" safely
Do not chmod the git worktree. Build and install from a throwaway copy (`mktemp -d`, `cp -R` or `git archive`), install to a prefix outside the copy, then `chmod -R a-rwx` the copy (and build dir), run the consumer build from a separate directory with only `PKG_CONFIG_LIBDIR=<prefix>/lib/pkgconfig`, and always restore in a `trap`. Verified on the C path: `chmod 000` on the hidden source and build directories (`ls` -> Permission denied), consumer built with `cc $(pkgconf --cflags --libs duospool) main.c` and printed the expected output; negative control (rename `duospool.h` away) gave `fatal error: 'duospool.h' file not found` [VERIFIED: machine].

### Pitfall 15: Rubric not pre-registered; the 25-point criterion nearly decides the outcome
D-13 weights memory safety "by construction vs left to sanitizers" at 25 of 100. Without explicit anchors that criterion is subjective and, scored honestly, favours Rust by up to the full margin D-07 needs (10 points) before any measurement. The protocol must therefore define, before any code: (a) a 0-5 anchor table per criterion (what evidence earns 5, 3, 0), (b) how points are computed (`weight * score / 5`), (c) which measurements feed which criterion, (d) what "not demonstrated" scores (D-15: not zero; define as the midpoint or as excluded with weights renormalised, pick one now). Otherwise D-07's auto-ratification would rest on post-hoc judgement. Raise this with the owner as part of the allowed "amend before the spike runs" step (D-13).

### Pitfall 16: Cargo workspace absorption and Cargo.lock
`spikes/tool-01/rust/Cargo.toml` needs its own `[workspace]` table (and the `fuzz/` crate another) so Phase 2's root workspace can never swallow or conflict with it. Commit `Cargo.lock`.

### Pitfall 17: Incremental and clean builds are not what the toy suggests
Ninja has no early cutoff: a comment-only edit still recompiles the C file. Rust dev profile uses incremental compilation by default, release does not (`CARGO_INCREMENTAL`) [ASSUMED]. D-13 already forbids extrapolating toy build times; the protocol should also say the toy has one translation unit/crate and no dependencies.

### Pitfall 18: Public push publishes tags and `.planning/`
`git push` of branches does not publish tags; the `spike-tool-01-evidence` tag push is a separate publication, and the audit must cover `--all` refs. `.planning/` is tracked and will become public, and one tracked file (`.planning/research/ARCHITECTURE.md`, a `S16` source row) contains `[repo-root]/...` absolute paths (one line, present in 6 of the 8 commits) [VERIFIED: machine, `git grep` over `git rev-list --all`]; commit author identity is `szTheory@users.noreply.github.com` (no private email) [VERIFIED: machine]. Present these as "review items" at the D-02 checkpoint (scrubbing means rewriting 8 commits, cheap only before the first push). `AGENTS.md` and `.claude/CLAUDE.md` also name LibFuturo and sibling projects; the owner may want to review that wording before publishing [ASSUMED concern].

## Code Examples

### History audit (tested on a synthetic repo and the real repo; real repo clean)

```sh
#!/bin/sh
# scripts/audit-history <repo>  -- exit 1 if anything is flagged or any object is unreadable
set -eu
repo=${1:-.}
cd "$repo"
# Function body kept OUT of $( ): macOS /bin/sh is bash 3.2, which mis-parses `case` patterns inside command substitution.
scan_blobs() {
  git rev-list --objects --all |
  git cat-file --batch-check='%(objecttype) %(objectname) %(objectsize) %(rest)' |
  while read -r type hash size rest; do
    [ "$type" = commit ] && continue
    [ "$type" = tree ] && continue
    [ "$type" = tag ] && continue
    [ "$type" = blob ] || { echo "UNREADABLE $type $hash $rest"; continue; }
    nonul=$(git cat-file blob "$hash" | LC_ALL=C tr -d '\000' | wc -c | tr -d ' ')
    [ "$nonul" -ne "$size" ] && echo "NUL-BYTES    $size $rest"
    [ "$size" -ge 4096 ] && [ $((size % 8192)) -eq 0 ]   && echo "ROM-SIZE-8K  $size $rest"
    [ "$size" -ge 4096 ] && [ $((size % 8192)) -eq 512 ] && echo "ROM-SIZE-HDR $size $rest"
    case "$rest" in
      *.[Pp][Cc][Ee]|*.[Ss][Gg][Xx]|*.[Ii][Ss][Oo]|*.[Cc][Uu][Ee]|*.[Bb][Ii][Nn]|*.[Rr][Oo][Mm]|*.[Cc][Hh][Dd]|*.[Ss][Aa][Vv]|*.[Ss][Rr][Mm]|*.[Zz][Ii][Pp]|*.7[Zz]|*.[Pp][Ee][Mm]|*.[Kk][Ee][Yy]) echo "EXT          $size $rest";;
    esac
  done
}
out=$(scan_blobs)
# text-level patterns (private keys, common token shapes, absolute home paths)
pat='BEGIN [A-Z ]*PRIVATE KEY|AKIA[0-9A-Z]{16}|gh[pousr]_[A-Za-z0-9]{20,}|xox[abp]-[0-9A-Za-z-]{10,}|/Users/[a-z]+/|/home/[a-z]+/'
txt=$(git grep -I -n -E "$pat" $(git rev-list --all) -- . 2>/dev/null | sed 's/^[0-9a-f]*://' | sort -u || true)
if [ -n "$out" ] || [ -n "$txt" ]; then printf '%s\n%s\n' "$out" "$txt"; exit 1; fi
echo "audit-history: clean"
```
Results this session: synthetic repo flagged `NUL-BYTES`, `ROM-SIZE-8K`, `EXT` for the planted `x.pce` and `NUL-BYTES` for a 3-byte `y.dat`; the real repo (8 commits, largest blob 69,040 bytes) flagged nothing under the blob/ext/NUL checks; `gitleaks git --no-banner --redact -v .` reported "8 commits scanned ... no leaks found"; the only text-pattern hit is the `[home]/` path above [VERIFIED: machine; the exact combined pattern string in the script above was exercised only as separate greps for `/Users/`, e-mail, and credential-like words, so run the assembled script once before relying on it]. The `git grep` line with `$(git rev-list --all)` can exceed ARG_MAX on a large history; fine for this repo. Because the home-path pattern will always hit that line until scrubbed, give the script an explicit, committed allowlist file (path+pattern, reason) rather than loosening the pattern.

### Public remote flow (all flags confirmed in `gh repo create --help` / `gh repo edit --help` this session)

```sh
# 0. owner checkpoint:decision first (audit output shown). Then:
gh repo view szTheory/duospool          # verified absent: "Could not resolve to a Repository"
gh repo create szTheory/duospool --public --source=. --remote=origin \
   --description "PC Engine / TurboGrafx-16 emulator core (working name; not trademark-cleared)"
git push -u origin main                 # explicit; do not rely on --push for tags
git push origin spike-tool-01-evidence  # tag is a separate publication, after the audit covered it
gh repo edit szTheory/duospool --enable-secret-scanning --enable-secret-scanning-push-protection
```
Prerequisites that must be committed before the checkpoint: `LICENSE-MIT`, `LICENSE-APACHE`, `LICENSE-0BSD`, a README with the "name is not trademark-cleared" note and the contribution clause, `.gitignore`, record 0004 `owner-recorded`. Do not pass `--license` to `gh` (it would create a commit on the remote and diverge). `szTheory/duospool` is free as of this session [VERIFIED: machine].

### Installed-consumer proof, C path (every step run in the scratchpad; passes)

```cmake
cmake_minimum_required(VERSION 3.20)
project(duospool C)
add_library(duospool STATIC core.c)
target_include_directories(duospool PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
install(TARGETS duospool ARCHIVE DESTINATION lib)
install(FILES include/duospool.h DESTINATION include)
configure_file(duospool.pc.in duospool.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/duospool.pc DESTINATION lib/pkgconfig)
```
`duospool.pc.in`: `prefix=${pcfiledir}/../..` / `libdir=${prefix}/lib` / `includedir=${prefix}/include` / `Cflags: -I${includedir}` / `Libs: -L${libdir} -lduospool`. Configure with `-DCMAKE_C_FLAGS="-ffile-prefix-map=$SRC=/src -ffile-prefix-map=$BUILD=/build"`, install with `cmake --install build --prefix "$PREFIX"`, then:
`PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" cc -Wall $(pkgconf --cflags --libs duospool) main.c -o consumer && ./consumer`.
Gate assertions: output equals golden; `grep -c -a "$SRC" "$PREFIX/lib/libduospool.a"` is 0 (also the `pwd -P` spelling and `$HOME`); same grep on the `.pc`; negative control removes `include/duospool.h` and the consumer build must fail.

### Installed-consumer proof, Rust path (not runnable here; mechanics for the executor)

`[lib] crate-type = ["staticlib"]` in the facade crate; `[profile.release] panic = "abort"`; build with the remap `RUSTFLAGS` above; copy `target/release/libduospool.a`, the hand-written `duospool.h`, and a hand-written `duospool.pc` (`prefix=${pcfiledir}/../..`, `Libs: -L${libdir} -lduospool`, `Libs.private:` = flags printed by `--print native-static-libs`) into the same `include/`, `lib/`, `lib/pkgconfig/` layout as the C path so one consumer script serves both. Same assertions as the C path.

### Fuzz recipes on this machine

```sh
# C, 60 s gate run (explicit path; never first on PATH, D-17). No ASan (Pitfall 10).
/opt/homebrew/opt/llvm@21/bin/clang -g -O1 -fsanitize=fuzzer,undefined fuzz_loader.c libduospool_src.c -o fuzz_loader
./fuzz_loader -max_total_time=60 -max_len=70000 corpus/        # 70000 > 65536 + header to exercise oversize refusal
# C, separate sanitizer row with Apple clang (works):
/usr/bin/clang -g -O1 -fsanitize=address,undefined tests.c core.c -o tests_asan && ./tests_asan
# C, ASan + libFuzzer via Apple clang and the Homebrew archive (linked and ran in the probe):
/usr/bin/clang -g -O1 -fsanitize=fuzzer-no-link,address fuzz_loader.c core.c \
   /opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/libclang_rt.fuzzer_osx.a -lc++ -o fuzz_loader_asan
# Rust (after the ASan canary):
cargo +nightly-2026-11-09 fuzz run loader -- -max_total_time=60       # default ASan
cargo +nightly-2026-11-09 fuzz run loader -s none -- -max_total_time=60   # fallback
```
A Linux container is the documented fallback if neither C variant works (docker client 29.5.2 is installed; whether the daemon is running was not established) [VERIFIED: machine for the client; daemon not checked].

### Fixture bytes, portable and deterministic

```sh
# gen-fixtures.sh: 65536 bytes from an integer LCG, seed 12345 (sha256 prefix ab0b5936cf56fca6 when verified)
awk 'BEGIN{x=12345; for(i=0;i<65536;i++){x=(x*1664525+1013904223)%4294967296; printf "\\%03o", int(x/65536)%256}}' > "$tmp"
printf "$(cat "$tmp")" > "$out"
# zero-fill: dd if=/dev/zero of="$out" bs=1024 count=64 2>/dev/null ; truncations: dd bs=1 count=N
```
The LCG product stays below 2^53 so awk doubles are exact [derived: 4294967296 x 1664525 ~ 7.1e15 < 9.007e15].

### Protocol-before-code check (D-09)

```sh
proto=$(git log --format=%H --diff-filter=A -- docs/decisions/0001-language-build.protocol.md | tail -n 1)
first_spike=$(git log --format=%H --diff-filter=A --reverse -- spikes/tool-01 | head -n 1)
[ -n "$proto" ] || { echo PROTOCOL_MISSING; exit 1; }
[ -z "$first_spike" ] || git merge-base --is-ancestor "$proto" "$first_spike" || { echo PROTOCOL_AFTER_SPIKE; exit 1; }
```
The decision record 0001 cites `$proto`; the check also asserts `git rev-parse --verify "$cited"` succeeds. Amendments after the protocol commit are new dated sections, detected by `git diff --diff-filter=M` on lines before the amendment heading (a simple version: the check fails if the protocol file's first N lines hash changed since the cited commit).

### Licence files (sources and exact handling)

| File | Source | Handling |
|------|--------|----------|
| `LICENSE-APACHE` | https://www.apache.org/licenses/LICENSE-2.0.txt, 11,358 bytes | Byte-for-byte. Do not fill the appendix placeholders. `sha256 cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30` [VERIFIED: machine `shasum -a 256`]. The SPDX-list copy (`text/Apache-2.0.txt`, 10,280 bytes, sha256 `074e6e32…`) is a normalised variant; do not mix them. |
| `LICENSE-MIT` | SPDX list `text/MIT.txt` (list v3.29.0, 1,078 bytes) | Template contains `Copyright (c) <year> <copyright holders>`: fill year and holder. **The holder name is an open owner input** (see Open Questions). |
| `LICENSE-0BSD` | SPDX list `text/0BSD.txt` (643 bytes), OSI-approved: true | Template has `Copyright (C) YEAR by AUTHOR EMAIL`: fill. 0BSD is a permission text; keep the sentence "Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted." intact. |
| README wording | Rust-ecosystem convention (serde README) | "Licensed under either of Apache License, Version 2.0 or MIT license at your option." and "Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in <project> by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions." [CITED: raw.githubusercontent.com/serde-rs/serde/master/README.md]. Add: fixtures under `fixtures/` are 0BSD, per-directory. |
Source file header (D-01): `// SPDX-License-Identifier: MIT OR Apache-2.0` and `// SPDX-FileCopyrightText: <year> <holder>`; fixture sources use `SPDX-License-Identifier: 0BSD`. The SPDX identifiers are valid list IDs (`MIT`, `Apache-2.0`, `0BSD`) [VERIFIED: machine, SPDX list files exist at those names]. A `scripts/check` step compares `LICENSE-APACHE` to the pinned sha256 and checks that the MIT/0BSD files contain the unmodified permission paragraphs (grep a distinctive line each).

## Seed Ledger Content (PROF-05, D-23)

Ten entries, all `profile = "pce-hucard-2btn"`, `status = "assumed"`, `evidence = []`, `updated = "2026-11-09"` (or the execution date), `pinning_test = "PENDING:<n>"`. Competing positions and labels are transcribed from SUMMARY.md lines 237-265 and 118-142 and 449 (read this session); `FACT` second-hand through ARCHITECTURE (manual pages read by one researcher, SUMMARY C19) is stated as such in the `sources` detail. Suggested IDs (Claude's discretion, numbering per area):

| ID | origin | statement (testable sentence) | competing positions (label: source) | assumption (placeholder, not fact) | withheld claims | revisit |
|----|--------|-------------------------------|-------------------------------------|-------------------------------------|-----------------|---------|
| LED-AUD-0001 | `SUMMARY C1` | The PSG advances its counters at master clock / 6 (about 3.58 MHz). | REPORT: pcetech.txt (effective tick master/6); CODE: Geargrafx, Mednafen, Ares (master/6). FACT (second-hand): HuC6280 manual p. H8-23 per ARCHITECTURE shows OSC1/3 = 7.16 MHz on the clock pin; patents cited by pcetech say 7.16 MHz while pcetech's formulas imply 3.58 | `psg_tick_divisor` proposal 6 (master/6) as a labelled placeholder; field stays `open` | any PSG pitch, period or timing claim; PCM rate claims tied to the tick | 5 |
| LED-VID-0001 | `SUMMARY C2` | A frame is 262 lines of 1365 master clocks (357,630 clocks, about 60.05 Hz). | REPORT: pcetech (263 fixed); CODE: MAME screen config 263 but TODO "add 263-line mode"; CODE: Mednafen, Geargrafx, Ares default 262 with 263 selectable by a VCE control-register bit; the manual pages read do not state a count | `lines_per_frame` proposal 262 (default of three codebases that also model 263); alternative 263 = 358,995 clocks, about 59.83 Hz [FACT, derived] | frame length, frame rate, samples-per-frame claims | 4 |
| LED-IRQ-0001 | `SUMMARY C3` | VBlank IRQ is raised on the first line after the active display area; the RCR IRQ is raised at the end of the HDW period. | REPORT: pcetech (VBlank on first line after active area); CODE: MAME (beginning of HDW after VDW ends; RCR near end of HDW); CODE: Mednafen (RCR 4 VDC clocks before HDW end); REPORT: Covell says Mednafen VSync IRQ fires late; hardware position [UNKNOWN] | Fixtures record IRQ scanline and phase into guest RAM; the core uses the stated placeholder; no literal line number asserted | any literal IRQ line/phase; "raster accurate" wording | 4 |
| LED-VID-0002 | `SUMMARY C4` | The VCE selects the dot clock from the low two bits of its control register: 0 = master/4, 1 = master/3, 2 and 3 = master/2. | FACT: manual page read documents only /4 and /3; CODE: several codebases implement /2; REPORT: pcetech register section says 7 and 5 MHz (internally inconsistent); register encoding [UNKNOWN] | Implement three enumerated modes (5.37, 7.16, 10.74 MHz); the master/2 encoding is a placeholder to re-check against the HuC6260 page before the first VCE fixture | any claim about the encoding of mode 2/3; mid-frame mode-change correctness (C11) | 4 |
| LED-VID-0003 | `SUMMARY C5` | In the 7 MHz class the active line is 320 pixels wide. | REPORT: pcetech display table (320); CODE: Geargrafx `k_huc6260_line_width` table and Mednafen overscan comments (341, 341.3); unresolved | Raw video records the full line plus active-window coordinates; no fixed width is asserted | any "frame is W x H" claim | 4 |
| LED-IO-0001 | `SUMMARY C6` | Bit 6 of a `$1000` read is a country bit that reads 1 on a PC Engine. | REPORT: pcetech; CODE: Geargrafx (country: 1 = PC Engine); REPORT: one teardown author reads it as "boot ROM present" (disputed) | Name it by observed behaviour (`io_1000_d6_value`, open); proposal 1; test behaviour, not semantics; D7 (CD sense, 1 = absent) shares the entry | any "region" claim; TG-16 profile | 2 |
| LED-LOAD-0001 | `SUMMARY C7` | The first-profile loader accepts only HuCard images already in PC Engine byte order, declared by the caller. | REPORT: secondary articles (Wikipedia, nicole.express) say North American card wiring reverses the data lines; CODE: MAME bit-reverses when a byte at offset $1FFF is below $E0 (unexplained threshold); CODE: MiSTer notes its US BIOS is already bit-swapped; ARCHITECTURE did not pin it [UNKNOWN] | `content_byte_order` proposal `pce` declared; `tg16`-order images refused; never detected | any TG-16 compatibility claim; any claim about which order a given image is in | 2 |
| LED-LOAD-0002 | `SUMMARY C8` | A copier header is present exactly when the image size modulo 8 KiB is 512 and the caller declares it. | CODE: Beetle strips 512 bytes when the size has bit 9 set; CODE: MAME detects a header only when length mod 16 KB = 512; heuristics disagree | `content_copier_header` proposal `refuse`: a header-prefixed image is refused, never stripped or detected; header handling is a declared input | any "loads headered dumps" claim | 2 |
| LED-AUD-0002 | `SUMMARY` open-gap 9 (line 449) | PSG channels are numbered 0 to 5, noise is on channels 4 and 5, and channel 1 modulates channel 0 (LFO). | FACT (second-hand): wiki per ARCHITECTURE (6 channels, 32 x 5-bit waveform); CODE: noise/LFO placement; ARCHITECTURE numbers channels 0-based, PITFALLS and the Mesen2 notes number them 1-based | The ledger and all fixtures use 0-based indices; any 1-based source is converted at citation time and the base is written in the `sources` detail | any noise or LFO behaviour beyond channel placement | 5 |
| LED-PROF-0001 | `SUMMARY C14` | The first profile emulates a HuC6280 (not 6280A); which retail models contain the 6280A is not claimed. | REPORT: PCE_Soundtest ("some CoreGrafx"); REPORT: Wikipedia (CoreGrafx has the A, CoreGrafx II reverts); analog capture is the only discriminator (E3) | `chip_revision = huc6280` is owner-selected (D-06); no per-model statement | per-model profiles, HuC6280A/PSG DC-offset claims | 6 |

Each entry also carries `disagreement` (the table's "competing positions" as one string, no inner double quotes), `sources` as `["REPORT|pcetech.txt|…", "CODE|Geargrafx|…", …]`, `withheld` (the right-hand column), `revisit_phase` (the number), and `origin`. Optional extra seeds the planner may add (not mandated): C9 block-transfer interrupt latency (`LED-CPU-…`, revisit 3), C10 ST0-ST2 MMU bypass (revisit 3), C13 save-state scope. The Phase 3 and 4 requirements (CPU-06, VID-04) will otherwise create them ad hoc.

`ledger/REQUIRED` format proposal: `ID<space>origin`, e.g. `LED-AUD-0001 SUMMARY C1`. The checker asserts the file exists, the entry exists, and `origin` equals the rest of the line; this ties each mandated topic to an ID without a separate topic field. `ledger/SCHEMA.md` states: "This check enforces structure, not truth."

## First-Profile Record (PROF-01, D-27/D-28)

`profiles/pce-hucard-2btn/r1.toml` (strict subset, one `[table]` per field). Root keys: `profile_id = "pce-hucard-2btn"`, `rev = 1`, `unresolved_behaviours = ["LED-IRQ-0001", "LED-VID-0002", "LED-VID-0003"]` (C3, C4, C5).

| Field | status | value / proposal | allowed | ledger | notes |
|-------|--------|------------------|---------|--------|-------|
| `media_slot` | owner-selected | value `hucard` | `["hucard"]` | `[]` | D-06 |
| `expansion_port` | owner-selected | value `absent` | `["absent"]` | `[]` | no CD |
| `vdc_count` | owner-selected | value `1` (integer) | | `[]` | |
| `vpc` | owner-selected | value `false` | | `[]` | no VPC |
| `chip_revision` | owner-selected | value `huc6280` | `["huc6280"]` | `["LED-PROF-0001"]` | one field: CPU and PSG are one die; does not claim which models shipped which |
| `input_port1` | owner-selected | value `pad2` | | `[]` | 2-button, no multitap |
| `io_1000_d6_value` | open | proposal `1` | `["0","1"]` | `["LED-IO-0001"]` | |
| `io_1000_d7_value` | open | proposal `1` | `["0","1"]` | `["LED-IO-0001"]` | CD sense: 1 = absent |
| `lines_per_frame` | open | proposal `262` | `["262","263"]` | `["LED-VID-0001"]` | frame clocks derived, not stored; see Open Question 2 |
| `psg_tick_divisor` | open | proposal `6` | `["3","6"]` | `["LED-AUD-0001"]` | |
| `content_byte_order` | open | proposal `pce` | `["pce","tg16"]` | `["LED-LOAD-0001"]` | |
| `content_copier_header` | open | proposal `refuse` | `["refuse","declared-512"]` | `["LED-LOAD-0002"]` | |
| `powerup_policy` | open | proposal `seeded-prng-v1` | | `[]` | Phase 2 defines it; no ledger entry yet |

Every owner-selected field carries `decision = "docs/decisions/0002-first-profile.md"`; `label` is human text only. Because the strict subset allows arrays of strings only, `allowed` lists are strings even for numeric fields; `value` keeps its natural type.

`profile-check` rules (stage 2 over the same stage-1 records, each with a negative case): exact field set (no extra, none missing) -> `PROFILE_UNKNOWN_FIELD` / `PROFILE_MISSING_FIELD`; `status` in {owner-selected, open} only -> `PROFILE_BAD_STATUS`; `open` has `proposal` and no `value` -> `OPEN_HAS_VALUE`, `OPEN_NO_PROPOSAL`; `owner-selected` has `value` and a `decision` whose file exists -> `SELECTED_NO_VALUE`, `SELECTED_NO_DECISION`; value member of `allowed` when `allowed` given -> `VALUE_NOT_ALLOWED`; every `ledger` ID and every `unresolved_behaviours` ID exists in `ledger/entries/` -> `LEDGER_REF_MISSING`; filename `rN.toml` equals `rev` -> `REV_MISMATCH`. Revision freeze (D-27) is a process rule; a cheap check is "an `rN.toml` referenced by a decision record must have no commits after the referencing commit" (optional).

**Criterion-2 mapping (planner must surface; see Open Question 1):** ROADMAP success criterion 2 lists "console model/region, `$1000` bit-6 value, CPU/PSG revision, line count/frame timing, controller type, expansion-port presence, content format". D-28's closed field list deliberately has no console-model/region field (id omits region/revision because of C6/C14). Recommended mapping, written into record 0002 and checked by grep: console model/region -> intentionally not a field; expressed through `io_1000_d6_value` (observed bit), `chip_revision`, and the open entries LED-IO-0001 and LED-PROF-0001; bit 6 -> `io_1000_d6_value`; CPU/PSG revision -> `chip_revision` + `psg_tick_divisor`; line count/frame timing -> `lines_per_frame` (+ derived clocks); controller -> `input_port1`; expansion port -> `expansion_port`; content format -> `media_slot`, `content_byte_order`, `content_copier_header`.

## Decision Records and Other Checkers (D-24..D-26, D-30)

**Front matter facts.** MADR current release is 4.0.0 (2024-09-17); its YAML front matter is optional `status`, `date`, `decision-makers`, `consulted`, `informed`, with headings `Context and Problem Statement`, `Considered Options`, `Decision Outcome`, `Consequences` [CITED: adr.github.io/madr]. D-25 uses its own closed field set and trimmed headings, so MADR compatibility is by spirit only; say so in `docs/decisions/README.md`. REUSE specification is 3.3 (2024-11-14); `REUSE.toml` is recommended over deprecated `.reuse/dep5`; comment-header tags `SPDX-FileCopyrightText` and `SPDX-License-Identifier` are the required licensing information [CITED: reuse.software/spec-3.3].

**`scripts/decision-check` (awk over `---` delimited front matter, single-line `key: value`):**
- Front matter must start on line 1 with `---` and end with `---`; only these keys exist: `status`, `blocks`, `default-in-force`, `owner-statement`, `owner-date`, `supersedes`, `superseded-by`, `reopen-if`, `research-ref` (unknown or duplicate key is an error; use single-line values, long text allowed).
- `status` in `open | proposed-default | owner-recorded | superseded | withdrawn`.
- `blocks` required unless `owner-recorded`; `default-in-force` required when `proposed-default`; `owner-statement` and `owner-date` (`YYYY-MM-DD`) required when `owner-recorded`, and the statement must contain a double-quote character (verbatim quote) and the words "session" or a date; `research-ref` always required.
- Filename `NNNN-slug.md` with four-digit number; numbers unique; `docs/decisions/README.md` index lists every record and its status word (drift is an error); `0005` must contain the sentence "No GPL or unclear-licence material is admitted without a further owner decision." (grep -F), as ROADMAP criterion 5 requires.
- The `.protocol.md` file is excluded from the record rule set (no front matter requirement) but is covered by the ancestry check.
- Negatives (each in `tests/decision/<case>/` with `EXPECT`): `owner-recorded` without statement; `proposed-default` without `default-in-force`; non-owner-recorded without `blocks`; unknown key; unknown status; bad date.
- Optional: PROJECT.md pointer rows (`→ docs/decisions/0004-…`) must carry the same status word as the record.
- Do not attempt a git-author check (D-25: the agent commits under the owner identity).

**`scripts/provenance-check` (`PROVENANCE.md` = blank-line separated `key: value` blocks, zero blocks is valid):** required keys exactly the D-26 list (`id`, `source-url`, `upstream-ref`, `upstream-path`, `license-spdx`, `license-url`, `upstream-notice`, `local-paths`, `modifications`, `copied-on`, `copied-by`, `maintenance-owner`, `why-copy-not-dependency`, `sync-policy`, `licence-decision`); `sync-policy` in `frozen | diverged | tracked`; `license-spdx` tokens (split on spaces, `OR` allowed, `WITH` rejected because exceptions add terms) all on the allowlist MIT, Apache-2.0, BSD-2-Clause, BSD-3-Clause, Zlib, ISC, 0BSD, CC0-1.0; `upstream-ref` matches a 40-hex SHA or a tag; marker scan: files containing `Provenance: PROV-` followed by four digits must resolve to an `id`, and every register `local-paths` entry must exist and contain its marker. **Self-match trap:** the checker, `docs/decisions/0007-*.md` and `PROVENANCE.md` itself mention the marker text; scan only the product source directories (not `scripts/`, `docs/`, `.planning/`) or require the marker at line start in a comment form. Negatives: a block missing a field, a bad SPDX value (`GPL-2.0-only`), an unresolved marker, a missing local path. Staleness is a warning only (D-26).

**Optional dev-only TOML cross-check (`scripts/toml-crosscheck`)**: pick `python3` where `python3 -I -c 'import tomllib'` succeeds (candidates: `python3`, `/opt/homebrew/bin/python3`), run `python3 -I -c 'import tomllib,sys; tomllib.load(open(sys.argv[1],"rb"))' file` over `ledger/entries`, `ledger/samples`, `profiles`, and print `SKIP: no python >= 3.11` otherwise, exit 0. Property to assert in review: every file the awk checker accepts is accepted by tomllib; tomllib accepting a file the subset rejects (floats, dates) is expected. Not part of `scripts/check`'s pass/fail.

## Toolchain Spike Mechanics (TOOL-01, D-09..D-17)

**Protocol file contents to pre-register** (`docs/decisions/0001-language-build.protocol.md`): the workload spec (D-10) as numbered requirements; exact measured command lines (one script each); the hard gates (D-11); weights (D-13) plus the rubric anchors and score arithmetic (Pitfall 15); the tie band (medians within 20% or both under 2 s are a tie); the margin rule (>= 10 weighted points and exactly one option passing all gates); the "not demonstrated" scoring rule; the line-count rule (non-blank, non-comment lines, per category from a committed `LINES.map`, counted by awk, no `cloc`); the timebox (D-15); which skeleton is written first (D-14); a metric register with stable IDs (`M01 clean-cold`, `M02 clean-warm`, `M03 incr-comment`, `M04 incr-real`, `M05 test`, `M06 gate-warm`, `M07 gate-clean-to-green`, `M08 sanitizer-row`, `M09 loc-by-category`, `M10 tools-required`, `M11 consumer-proof`, `M12 fuzz-60s`, `M13 fuzz-liveness-control`, `M14 wasm`, `M15 path-leak-check`) so record 0001 can be checked for completeness with grep.

**Shared interface so one harness serves both skeletons:** each skeleton exposes `gate.sh` stages `build`, `test-refusals`, `test-controls`, `test-interleave`, `install`, `consumer-proof`, `fuzz60`, each printing `GATE <stage> PASS|FAIL`; both emit a byte-identical `golden.txt` transcript from the same `fixtures.manifest` (diff must be empty: a cross-language check, E0 class).

**Timing commands (hyperfine 2.0.0, flags per its README):**
- Warm clean build: `hyperfine --warmup 1 --runs 10 --shell=default --prepare 'rm -rf build' 'sh bench/build-c.sh'` (Rust: `--prepare 'rm -rf target'`, `sh bench/build-rust.sh`).
- Incremental (comment-only and one-line real edit): `--prepare 'sh bench/edit-comment.sh'` with a unique comment per call, `--cleanup 'sh bench/restore.sh'`; commands rebuild "what the gate needs" (`cmake --build build` / `cargo build`).
- True-cold: one run each, not hyperfine (it cannot repeat first-download cost). Use `perl -MTime::HiRes=time -e '$t=time; system(@ARGV); printf "%.2f\n", time-$t' -- <cmd>` since BSD `date` has no sub-second output [ASSUMED: BSD date lacks `%N`; verified `date -u +%Y-%m-%d` only]. Rust cold = `rustup toolchain install …` + first build; C cold = first configure+build; record the Rust download sizes and that cmake/ninja/clang were pre-installed.
- Debug and release both, same strictness: C `-Wall -Wextra -Wconversion`, Rust `cargo clippy -- -D warnings`; no sccache/ccache.
- wasm: `cargo build --release --target wasm32-wasip1` then `wasmtime run <file>.wasm` (WASI: pass `--dir` only if the stub reads files; the spike's runner should read nothing) and compare digests with native [CITED: STACK.md section 5; wasmtime run syntax ASSUMED].
- Rust `Miri` row: `cargo +nightly-2026-11-09 miri test` on the facade only; the core is `#![forbid(unsafe_code)]`.

**Scoring script:** a committed `score.awk` reading `scores.txt` (`criterion option score(0-5|ND)`), the weights, and gate results; prints both totals, margin, `RESULT=owner-recorded-preauth|proposed-default` per D-07. Negative controls: a tie file, a 9.9-point margin file, a file where the leader fails a gate; each must give `proposed-default`.

**Owner record rule (D-07):** if the script says pre-authorised, record 0001 is `owner-recorded` and its `owner-statement` quotes the D-07 selection ("Pre-authorize clear winner") with the session and date. Otherwise `status: proposed-default`, `blocks: Phase 2`, `default-in-force:` Rust per STACK.md [CITED: .claude/CLAUDE.md Technology Stack, rank 1] unless the data says otherwise.

**Spike lifecycle (D-16):** commit spikes on `main`, tag `spike-tool-01-evidence`, then a removal commit. Tag with `git tag -a`; do not push the tag until the audit and checkpoint.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `.reuse/dep5` | `REUSE.toml` (spec 3.3) | spec 3.2 era; 3.3 dated 2024-11-14 [CITED: reuse.software/spec-3.3] | Deferred; note for Phase 2 |
| MADR 3 | MADR 4.0.0 (2024-09-17) [CITED: adr.github.io/madr] | 2024-09 | D-25 is a custom closed set, not MADR-conformant |
| Apple clang libFuzzer | Not shipped; Homebrew `llvm` provides `libclang_rt.fuzzer_osx.a` [VERIFIED: machine] | n/a | Use explicit paths |
| `rustup-init` formula | `rustup` keg-only formula, `rustup-init` removed [VERIFIED: machine `brew info rustup`] | recent | Use `brew install rustup` + PATH |

**Deprecated/outdated:** `.reuse/dep5`; AFL++/trufflehog (AGPL) as dependencies.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | gawk/mawk/BWK dialect rules listed in Pitfall 2 (intervals, classes, `nextfile`, `printf %c` 0) match their documented behaviour | Pitfall 2, 8 | A checker fails on Linux CI; caught by the scheduled Linux run. Mitigate by running `scripts/check` once in a Linux container or on the first CI run |
| A2 | `cargo-fuzz` default ASan probably hangs here like Homebrew LLVM's ASan | Pitfall 10 | If wrong, no effect (canary passes); if right, fallback `-s none` costs ergonomics points |
| A3 | Rust staticlib on macOS needs `-lSystem -lc -lm` (maybe `-liconv`) | Pitfall 13 | Consumer link fails; fixed by pasting the `--print native-static-libs` output |
| A4 | Cargo `trim-paths` stability unknown; plain `--remap-path-prefix` is sufficient | Pitfall 12 | Path-leak gate fails on Rust; add more prefixes |
| A5 | Rust dev profile is incremental by default, release is not | Pitfall 17 | Misattributed timing; the protocol should state observed values |
| A6 | `lines_per_frame` proposal 262 and `psg_tick_divisor` proposal 6 are sensible placeholders | Seed ledger, profile | Open fields; owner may overrule; no claim is made |
| A7 | LED-VID-0002 modelling assumption (low two bits: 0=/4, 1=/3, 2-3=/2) | Seed ledger | It is a placeholder to re-check against the HuC6260 page; ledger status stays `assumed` |
| A8 | 0-based PSG channel numbering equals the hardware channel-select value | LED-AUD-0002 | Fixtures written 0-based could mismatch registers; Phase 5 re-reads PSG manual |
| A9 | BSD `date` lacks sub-second format; perl `Time::HiRes` is the portable alternative | Spike timing | Cosmetic |
| A10 | `AGENTS.md`/`.claude/CLAUDE.md` wording about LibFuturo and siblings may need an owner glance before going public | Pitfall 18 | Minor disclosure; owner decides at the checkpoint |
| A11 | The `git grep` pattern set in `audit-history` is adequate for a no-secrets docs-only history | Code Examples | gitleaks second opinion at the checkpoint covers this |

## Open Questions

1. **Console model/region field (criterion 2 vs D-28).**
   - What we know: ROADMAP criterion 2 names "console model/region"; D-27/D-28 give a closed field list without it and say the id omits region/revision.
   - What's unclear: whether a verifier will accept a mapping table or require a field.
   - Recommendation: keep D-28 unchanged; record the mapping in 0002 and check it by grep. If the owner wants a literal field, add an `open` `console_model` table with no value and `label` only.
2. **Meaning of `lines_per_frame` if 262/263 is a guest-visible VCE bit (CODE per SUMMARY C2).**
   - What we know: three codebases make 263 selectable by a VCE control bit.
   - What's unclear: whether the profile field is the power-on default or a hardware constant.
   - Recommendation: say in the `label` and in LED-VID-0001 that it is "line count at power-on / when the selectable bit is clear" and defer the decision to Phase 4.
3. **Copyright holder name and year for LICENSE-MIT, LICENSE-0BSD and SPDX headers.** Not in CONTEXT. Owner input (a name or handle); do not invent. Treat as a plan checkpoint:human-verify item.
4. **Publish at end of Phase 1 vs earlier.** Recommend once, after the audit covering spike commits and the tag; Phase 2 adds Actions. Planner must keep the D-02 checkpoint after the audit task.
5. **Rubric amendment window (D-13).** The owner may amend weights before the spike runs; the rubric anchors (Pitfall 15) are new and should be shown at the same time. Whether to ask in-session or record as a Claude-proposed default is a planning decision.
6. **Docker daemon state** (fallback for Linux fuzz/awk portability) was not established; only the client exists. Check before promising the container fallback.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| sh/awk/git/shasum/perl | all checkers | yes | one-true-awk 20200816; perl 5.34.1 | none needed |
| shellcheck, dash | lint (dev) | yes | | `sh -n` |
| cmake / ninja / pkgconf / Apple clang | C spike | yes | 4.4.3 / 1.13.2 / 3.0.9 / 21.0.0 | |
| Homebrew llvm@21 (libFuzzer) | C fuzz | yes (ASan broken, fuzzer+UBSan OK) | 21.1.8 | Apple ASan + manual libFuzzer archive; Linux container |
| rustup / cargo / rustc | Rust spike | **no** | | `brew install rustup` (+ toolchains above); if the install fails, Rust path is "not demonstrated" |
| hyperfine | timings | **no** | brew 2.0.0 | `perl Time::HiRes` loop (worse) |
| wasmtime | Rust wasm run | **no** | brew 50.0.1 | "not attempted" |
| wasi-sdk | C wasm | **no** (no brew formula) | 34.0 tarball, 180,322,002 bytes | "not attempted, install cost 180 MB" |
| gh (authenticated) | public repo | yes | 2.101.0 | owner runs manually |
| gitleaks | optional audit second opinion | yes | 8.30.1 | skip |
| docker | Linux fallback | client only | 29.5.2 | daemon state unknown |
| python3 >= 3.11 | optional toml cross-check | `/opt/homebrew/bin/python3` 3.14.4 | | skip |

**Missing with no fallback:** none blocking. **Missing with fallback:** Rust toolchain stack, hyperfine, wasmtime, wasi-sdk (install is routine and reversible; no checkpoint required, but record the cost).

## Validation Architecture

> `workflow.nyquist_validation` is true in `.planning/config.json`; section included.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | None external. POSIX `sh` + `awk` checkers; each checker is its own tested subject through negative-control directories |
| Config file | none; layout in "Recommended Project Structure" |
| Quick run command | `sh scripts/check` (target: under 2 seconds; 10 ledger entries, about a dozen negatives, a handful of decision records) |
| Full suite command | `sh scripts/check && sh spikes/tool-01/gates.sh` (the spike gates need the toolchain installs; run only in the spike wave) |
| Lint (dev) | `for f in scripts/* scripts/lib/*; do dash -n "$f"; done`; `shellcheck -s sh scripts/*` where installed |
| Exit codes | `0` pass, `1` violation (or expectation not met), `2` harness/usage error. `scripts/check` treats 2 as a distinct failure and never as an "expected failure" |

### Phase Requirements -> Test Map

| Req ID / criterion | Behavior | Test Type | Automated Command | Negative control (must fail, direction stated) | File Exists? |
|--------|----------|-----------|-------------------|-----------------------------------------------|-------------|
| SC3 / PROF-04 | Ledger entries conform to schema; `supported` requires E1/E2/E3 | unit | `scripts/ledger-check ledger/entries/` (0) | each `ledger/negative/<case>/` fails with its EXPECT set: `e0-only`, `e4-only`, `e0-e4` -> `SUPPORTED_NO_QUALIFYING_EVIDENCE`; `e1-report-only` -> `E1_NEEDS_FACT_SOURCE`; `supported-pending` -> `SUPPORTED_PENDING_TEST`; `unknown-status`; `missing-field`; `unknown-key`; `assumed-no-assumption`; `id-filename-mismatch`; `duplicate-id`; `empty-dir` -> `EMPTY_LEDGER`; `required-missing` | Wave 0 |
| SC3 | Mislabelled sample marked supported on E0/E4 only is rejected | unit | `scripts/ledger-check --expect SUPPORTED_NO_QUALIFYING_EVIDENCE ledger/negative/e0-e4/` | rejected; if the rule were deleted the control reports `EXPECT_MISMATCH` and exits 1 | Wave 0 |
| SC3 | ok-samples pass | unit | `for f in ledger/samples/ok-*.toml; do scripts/ledger-check "$f"; done` | a sample that fails means the checker is over-strict | Wave 0 |
| SC3 | Checker is alive (not vacuous) | meta | `scripts/check --selftest` runs a mutation: remove a rule, the matching negative must flip to CONTROL BROKEN | stage sentinel missing -> `NO_SENTINEL` | Wave 0 |
| SC4 / PROF-05 | Ten mandated entries exist, are `assumed`, carry assumption, origin matches REQUIRED | unit | `scripts/ledger-check --required ledger/REQUIRED ledger/entries/` | `required-missing` case fails `REQUIRED_MISSING`; `supported=0` asserted for Phase 1 from the status counts printed | Wave 1 |
| SC2 / PROF-01 | Profile record names every field; each owner-selected or open; no value on open | unit | `scripts/profile-check profiles/pce-hucard-2btn/r1.toml` | `tests/profile/<case>/`: open-with-value, selected-without-decision, unknown field, missing field, bad status, ledger ref missing, rev mismatch | Wave 1 |
| SC2 | Criterion-2 mapping present in 0002 | grep | `grep -F` each criterion phrase in `docs/decisions/0002-first-profile.md` | removing the mapping row fails | Wave 1 |
| SC5 | Records 0003-0008 exist with valid state; 0005 contains the no-GPL sentence | unit | `scripts/decision-check docs/decisions` | `tests/decision/<case>/`: owner-recorded w/o statement, proposed-default w/o default-in-force, missing blocks, unknown key/status, bad date, README drift | Wave 1 |
| SC5 / D-26 | Provenance register valid (zero entries) and marker rules reject bad input | unit | `scripts/provenance-check` | `tests/provenance/<case>/`: missing field, GPL SPDX, unresolved marker, missing local path | Wave 0 |
| SC1 / TOOL-01 | Protocol committed before any spike file | git | protocol-before-code check (Code Examples) | a synthetic repo where the spike commit precedes the protocol -> `PROTOCOL_AFTER_SPIKE` | Wave 2 |
| SC1 | Both skeletons pass hard gates on the shared manifest | integration | `sh spikes/tool-01/gates.sh rust` and `... c` | refusal matrix (7 refusal inputs + 1 accept), deliberately wrong expectation rejected, two-instance interleave equals solo digests, consumer proof with header removed must break, `.a` and `.pc` grep for work dir and `$HOME` is 0 (and a seeded leak build must be caught), fuzz harness liveness control (planted 4-byte-prefix bug) found within 60 s | Wave 3 |
| SC1 | Golden transcript identical across languages | integration | `diff spikes/tool-01/out/golden.rust.txt spikes/tool-01/out/golden.c.txt` | an edited golden fails | Wave 3 |
| SC1 | Record 0001 has every metric ID for both options, values or `not demonstrated: <reason>` | grep/awk | `sh scripts/check-results docs/decisions/0001-language-build.md docs/decisions/0001-language-build.protocol.md` | delete one row -> `RESULT_MISSING M07 rust` | Wave 3 |
| SC1 | Score arithmetic and D-07 rule | unit | `awk -f spikes/tool-01/score.awk scores.txt` | tie file, 9.9-point file, leader-fails-gate file each print `proposed-default` | Wave 3 |
| SC1 | Owner choice recorded with date or record says open and Phase 2 blocked | unit | `scripts/decision-check` (rules above) | `owner-recorded` without date fails; `proposed-default` without `blocks: Phase 2` fails | Wave 1/3 |
| D-01 | Licence files present and canonical | unit | `shasum -a 256 LICENSE-APACHE` equals the pinned value; grep permission lines in MIT/0BSD | altered Apache file fails | Wave 0 |
| D-02 | History audit clean before publication | script | `sh scripts/audit-history .` | synthetic repo with `x.pce` (8 KiB zeros) fails; an `UNREADABLE`/`missing` object fails | Wave 4 |
| FIX-05 precursor | No ROM-like blob in tree | script | `sh scripts/audit-history .` also over the working tree | generated spike fixtures must not be committed | Wave 4 |

### Sampling Rate
- **Per task commit:** `sh scripts/check` (the Phase 1 fast gate, under 2 s) plus `dash -n` on any touched script.
- **Per wave merge:** `sh scripts/check` + the wave's own gate (`gates.sh` for the spike waves, `audit-history` for the publication wave).
- **Phase gate:** full `scripts/check`, spike gates for both skeletons recorded in 0001, `audit-history` clean, owner checkpoint resolved, before `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `scripts/lib/toml-subset.awk`, `scripts/lib/ledger-rules.awk`, `scripts/ledger-check` (copy the tested prototype; add directory mode, sentinel, ENVIRON, comma-set `--expect`, REQUIRED, DUP_ID, status counts)
- [ ] `scripts/check` skeleton aggregating all checks with exit-code discipline (D-30)
- [ ] `ledger/negative/*`, `ledger/samples/ok-*`, `tests/{decision,provenance,profile}/*` negative corpora
- [ ] `scripts/provenance-check` + `PROVENANCE.md` (zero entries)
- [ ] `.gitignore`, `LICENSE-*`, SPDX header convention
- [ ] Toolchain installs for the spike wave (rustup toolchains, hyperfine, wasmtime)

## Security Domain

`security_enforcement` is enabled (absent = enabled); ASVS level 1.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no (no service) | n/a; `gh` auth is the owner's existing keyring login |
| V3 Session Management | no | n/a |
| V4 Access Control | partial | GitHub repo visibility and branch settings are owner actions; Actions permissions are Phase 2 (D-03) |
| V5 Input Validation | yes | Strict subset parser: ASCII-only, 400-char lines, 16 KiB cap, closed keys; no `eval`, no `system()`, no `getline` from data-named files; quote every shell variable; `--` before filenames; fail closed |
| V6 Cryptography | no | licence-file `shasum -a 256` pinning is integrity, not crypto design; never hand-roll |
| V10 Malicious code / supply chain | yes | No product dependencies; spike tools pinned by version; `cargo install --locked`; legitimacy check on crates; pre-publication audit; GitHub Actions pinned by SHA in Phase 2 |
| V14 Configuration | yes | `.gitignore` for build outputs; no secrets in repo; secret scanning + push protection enabled after repo creation |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Hostile ledger/profile/manifest file crashes or fools the checker (huge file, control chars, injection through awk `-v`) | Tampering / DoS | Size cap before parsing, `LC_ALL=C`, byte-class rejection, `ENVIRON` not `-v`, fail on sentinel absence |
| Vacuous pass (checker dead, awk dialect error) | Repudiation | Negative controls and sentinel; exit 2 distinct |
| Secrets/ROM bytes/home paths published with history | Information disclosure | `audit-history` + gitleaks once; allowlist file for known benign hits; checkpoint before `gh repo create` |
| Rights leak: commercial or GPL fixtures committed | Information disclosure / legal | D-04 tiering; ignored generated fixtures; audit extension/size heuristics |
| Supply-chain via spike tool installs (brew/crates) | Tampering | Version pins, `--locked`, legitimacy seam OK for crates; no `curl | sh` |
| Absolute paths and usernames in published artifacts | Information disclosure | Remap flags; grep gate on installed artifacts |
| Tag publication surprise | Information disclosure | Audit `--all`; push tags explicitly |

## Sources

### Primary (HIGH confidence, verified this session)
- Machine probes on this Mac (macOS 26.6.2 arm64, 2026-11-09): tool versions, Homebrew formula info, `llvm@21`/`@18` ASan hang and libFuzzer results, CMake install/`.pc`/path-embedding/consumer experiments, awk prototype runs, git audit experiments, `gitleaks` run.
- `.planning/phases/01-.../01-CONTEXT.md` (all decisions, read in full), `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/research/SUMMARY.md` lines 1-285 and 449, `.planning/research/ARCHITECTURE.md` and `PITFALLS.md` (targeted grep), `.planning/PROJECT.md` Key Decisions, `.planning/ROADMAP.md` Phase 1-3 text, `AGENTS.md`, `.claude/CLAUDE.md`.
- https://static.rust-lang.org/dist/channel-rust-1.99.1.toml and `.../2026-11-09/channel-rust-nightly.toml` (toolchain, wasm32-wasip1, Miri availability).
- crates.io API for `cargo-fuzz` 0.13.2 and `libfuzzer-sys`; `gsd-tools query package-legitimacy check` (both OK).
- https://api.github.com/repos/WebAssembly/wasi-sdk/releases/latest (v34.0, asset size); spdx/license-list-data v3.29.0 text files; https://www.apache.org/licenses/LICENSE-2.0.txt.
- `gh repo create --help`, `gh repo edit --help`, `brew info` for rustup, hyperfine, wasmtime, gitleaks, trufflehog, wasi-libc, wasi-runtimes.

### Secondary (MEDIUM confidence, official docs fetched)
- https://rust-lang.github.io/rustup/overrides.html (`rust-toolchain.toml` fields)
- https://doc.rust-lang.org/rustc/command-line-arguments.html (`--remap-path-prefix`)
- https://doc.rust-lang.org/cargo/reference/profiles.html (`panic`, `strip`; `trim-paths` not on the fetched page)
- https://raw.githubusercontent.com/sharkdp/hyperfine/master/README.md (`--warmup`, `--runs`, `--prepare`, `--setup`, `--shell`)
- https://adr.github.io/madr/ (MADR 4.0.0 fields and headings), https://reuse.software/spec-3.3/ (REUSE 3.3, `REUSE.toml`, dep5 deprecated)
- https://raw.githubusercontent.com/serde-rs/serde/master/README.md (dual-licence wording)

### Tertiary (LOW confidence, marked [ASSUMED])
- awk dialect behaviour of gawk/mawk (not installed here); Rust `native-static-libs` content on macOS; `cargo-fuzz` ASan behaviour on this OS; Cargo `trim-paths` status; BSD `date` format limits.

## Metadata

**Confidence breakdown:**
- Checker design and shell/awk footguns: HIGH on the one-true-awk (run), MEDIUM for gawk/mawk (not run).
- Machine inventory, installs, libFuzzer/ASan findings, C installed-consumer mechanics, audit script: HIGH (run).
- Rust spike mechanics: MEDIUM (rustc/cargo absent; commands come from manifests and docs).
- Seed ledger and profile content: MEDIUM (transcribed from SUMMARY.md; hardware facts are open by design).
- Licence/MADR/REUSE facts: HIGH (primary pages fetched).

**Research date:** 2026-11-09
**Valid until:** 2026-11-16 for Homebrew/toolchain versions (fast-moving); 30 days for the rest.
