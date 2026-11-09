# Phase 1: Owner Decisions, Claim Ledger and Toolchain Spike - Context

**Gathered:** 2026-11-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 1 is a gate phase. It delivers:

1. A measured Rust/Cargo vs C11/CMake toolchain spike and a language/build decision record (TOOL-01).
2. A versioned first-profile record (PROF-01).
3. A claim ledger with a checker and negative controls, seeded with every open hardware fact the slice depends on (PROF-04, PROF-05).
4. Decision records for licence/visibility, fixture rights, API/ABI posture, CI provider, and the copied-code provenance rule.

No emulator code is written in this phase. The spike skeletons are throwaway evidence and are not the Phase 2 code base.

Owner decisions taken in this discussion are recorded below. They are not proposals.

</domain>

<decisions>
## Implementation Decisions

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

- **D-18 One strict TOML subset for all hand-edited data:** This covers ledger entries, profile records, and later manifests.
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

### Added after research (2026-11-09)

- **D-31 (owner, copyright holder):** The owner selected "The DuoSpool Authors". `LICENSE-MIT`, `LICENSE-0BSD` and the `SPDX-FileCopyrightText` headers read "Copyright (c) 2026 The DuoSpool Authors".
- **D-32 Criterion-to-field mapping:** ROADMAP criterion 2 names "console model/region". D-28 deliberately has no `region` or `console_model` field (C6).
  - Record 0002 carries a mapping table that maps each criterion-2 term to the D-28 field(s) that realise it. "console model/region" maps to `media_slot`, `expansion_port`, `vdc_count`, `vpc` and `io_1000_d6_value`.
  - A grep check verifies that every criterion-2 term appears in that table.
- **D-33 Publish once, at the end of Phase 1:**
  - The history audit (`scripts/audit-history` plus `gitleaks`, already installed, MIT) runs after the spike commits and the evidence tag exist.
  - The D-02 `checkpoint:decision` follows the audit and shows the owner every flagged item. Known review items: absolute `[home]/...` paths in `.planning/research/ARCHITECTURE.md`, and the LibFuturo/sibling mentions in `AGENTS.md` and `.claude/CLAUDE.md`.
  - `main` is pushed first, then the tags.
- **D-34 `lines_per_frame` semantics:** If the line count is a guest-selectable VCE bit, the field means the power-on value, not a hardware constant. Its `label` and the ledger entry say so.
- **D-36 Amendment to D-17 (2026-11-09, made before the protocol commit, Claude's design under the owner's instruction to adopt research recommendations):**
  - D-17 said "a dated nightly is used only for cargo-fuzz". That contradicted D-12's Miri row, because Miri needs nightly.
  - Amended rule: the same dated nightly (`nightly-2026-11-09`) is used only for cargo-fuzz and for Miri on the facade crate. Both are spike and staged checks; neither is the measured build.
  - Build and test timings always use the pinned stable 1.99.1.
  - The nightly install time is recorded under "tools required".
- **D-37 (owner, D-07 reading, 2026-11-09):**
  - The owner selected "Stricter: worst-case ≥10".
  - Auto-adoption under D-07 requires all of the following:
    - The leader passes every hard gate.
    - The leader's nominal lead is ≥10.00 points, with not-demonstrated items scored at the midpoint.
    - The lead is still ≥10.00 when the leader's not-demonstrated items score 0 and the other option's score the maximum.
  - Anything less is `proposed-default`, and the owner is asked.
  - On a tie or no leader, there is no default; the owner's choice is required.
- **D-38 Refinement of D-19 (2026-11-09, Claude's design, from RESEARCH Pitfall 5 and D-23):**
  - Control cases are per-case directories, not single files:
    - `ledger/negative/<case>/` holds one or more `LED-*.toml` files plus an `EXPECT` file listing the expected reason code(s), fixed before the first run.
    - `ledger/samples/ok-<case>/` holds the must-pass cases.
  - Directories are needed because the duplicate-ID case requires two files with the same ID, and a shared directory would add `ID_FILENAME_MISMATCH` to every negative.
- **D-35 Spike fixture bytes are not committed:**
  - The generated 64 KiB fixtures look like ROMs. Only the generator (an integer LCG, not awk `rand()`) and the manifest are committed; the generated bytes are gitignored.
  - The protocol commit (D-09) also carries 0-5 rubric anchors per criterion, the score arithmetic, and the "not demonstrated" rule. A small scorer with tie, 9.9-point-margin and leader-fails-gate controls makes the decision rule executable.
  - Fuzzing on this Mac: Homebrew LLVM ASan hangs at process start on macOS 26.6.2. C uses Apple-clang ASan plus a manual link of the llvm@21 libFuzzer archive. Rust runs a 10-second ASan canary and falls back to `-s none`. Either fallback is recorded as a portability cost.

### Phase 1 gate entry point

- **D-30:** Phase 1 creates a minimal `scripts/check` that runs `ledger-check` (entries, negatives and samples), the decision-record front-matter check, and the provenance check. Phase 2 extends this same entry point into the full fast gate. It does not start a second one.

### Claude's Discretion

- The exact LED numbering, the wording of assumptions for the seed entries, and the reason-code names.
- Whether the decision, provenance and ledger checks are one awk script or three small ones behind `scripts/check`.
- The exact `hyperfine` command lines, the shape of the fixture generator script, and the C build presets for the spike.
- The order of records within the plan, with one constraint: the protocol commit (D-09) must come before any spike code, and the public-remote checkpoint (D-02) must come after the history audit.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project policy and scope
- `AGENTS.md`: standing project policy (fan-out rule, dependency preference, rights, evidence honesty).
- `.planning/PROJECT.md`: constraints and the Key Decisions table that becomes pointers (D-25).
- `.planning/REQUIREMENTS.md`: TOOL-01, PROF-01, PROF-04, PROF-05, and the E0-E4 tier definitions (lines 14-17).
- `.planning/ROADMAP.md` §"Phase 1": goal, five success criteria, owner gates, and the decision-number table.

### Research inputs
- `.planning/research/STACK.md` §1, §"Phase 1 toolchain spike", §"When C wins", §3, §4, §7: spike scope, tool versions, fuzzing caveats (Apple clang has no libFuzzer runtime), fixture toolchain licences.
- `.planning/research/SUMMARY.md` §"How to Read This Summary" (labels and tiers), §"Owner-Gated Decisions" (lines 53-79), §"Hardware Baseline" (114-142), §"Cross-Source Conflicts" C1-C21 (237-265): the seed ledger content and the profile field proposals.
- `.planning/research/ARCHITECTURE.md` §1.10 and the "Profile" glossary entry: the immutable profile struct proposal that D-27/D-28 harden.
- `.planning/research/PITFALLS.md`: C1/C4 (ledger before golden hash), C7 (rights), C11 (implicit profile), M6 (copying).

### Bootstrap
- `.planning/bootstrap/PROJECT-SEED.md`, `.planning/bootstrap/EMULATOR-DNA.md`: North Star and the engineering lessons behind fail-closed inventories and installed-consumer proofs.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- None. Greenfield repository: planning docs, `AGENTS.md` and `.claude/CLAUDE.md` only. There is no remote.

### Established Patterns
- Research documents label every claim (FACT/CODE/REPORT/PROPOSAL/UNKNOWN/OWNER). The ledger `sources` labels reuse this vocabulary.

### Integration Points
- New top-level trees: `docs/decisions/`, `ledger/`, `profiles/`, `scripts/`, `spikes/` (temporary), `PROVENANCE.md`, and the `LICENSE-*` files.
- Phase 2 consumes the following: the language record (hard gate), `profiles/pce-hucard-2btn/r1.toml`, the strict TOML subset, the `ledger/` schema and negative corpus (to be ported to the chosen language), and `scripts/check`.

</code_context>

<specifics>
## Specific Ideas

- The owner's dependency principle, verbatim: "another copy and paste is better than another dep if we truly need a dep that's fine but it's better to have smaller, flatter dependency trees."
- The owner's working style for decisions: fan out across stakeholder lenses, use primary sources where they change the answer, run an adversarial pass, then adopt one coherent recommendation. Ask the owner only at authority gates.
- QEMU-style frozen versioned machine types are the model for profile revisions. Mednafen/libretro hidden defaults in code are the anti-pattern to avoid.

</specifics>

<deferred>
## Deferred Ideas

- Full REUSE (`REUSE.toml` plus `reuse lint`) adoption. It is now possible because the licence is decided, but it adds a tool. Revisit in Phase 2 if the in-house SPDX/provenance check becomes a burden.
- Signed commits as owner attestation for `owner-recorded` decision flips. The owner can opt in later.
- An owner decision on evidence-tier policy (decision 17). It remains `proposed-default` (D-29).
- Remaining open owner gates that do not block Phase 1: LibFuturo access (7), hardware captures (10), timing fidelity and performance budget (11), host/package format (12), wasm promise (13), save-state, replay and debugger scope (15), PAL (16), release policy (18).

</deferred>

---

*Phase: 01-owner-decisions-claim-ledger-and-toolchain-spike*
*Context gathered: 2026-11-09*
