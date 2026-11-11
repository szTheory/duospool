# Phase 2: Deterministic Machine Shell, HuCard Boundary and Fail-Closed Gate - Research

**Researched:** 2026-11-09
**Domain:** C11 deterministic core library (CMake/Ninja), POSIX sh/awk fail-closed gate, GitHub Actions workflow
**Confidence:** HIGH for toolchain behaviour and gate design (run on this Mac and in an Ubuntu 24.04 container); MEDIUM for HuCard mirroring (three emulator sources agree; no primary Hudson document reachable)

**Label legend (as requested).** CODE = read or run in this session (repo file with path:line, third-party source, command output). FACT = authoritative document fetched this session. REPORT = secondary or community source, or recalled. PROPOSAL = my recommendation. UNKNOWN = not verified. Mapping to the GSD tags: CODE/FACT are the verified tier, REPORT is cited-secondary, PROPOSAL/UNKNOWN are assumed.

## Summary

Phase 2 is fully designed by D-40..D-64. This research does not reopen any of them. It adds (a) a Validation Architecture that maps every requirement and success criterion to an automated check, a placement, a negative control and a failure signal; (b) corrections to implementation details that the discuss phase got slightly wrong or could not check, each verified on macOS arm64 (Apple clang 21) and Ubuntu 24.04 (gcc 13.3, clang 18.1.3); (c) the HuCard evidence report; (d) a precise list of what existing code is reusable.

Eight things the planner must plan around (details in Common Pitfalls and Open Questions): the D-54 header claim ("only stddef/stdint/stdbool exist") is false and needs an include allowlist; Darwin `nm -P` types const data as `S`/`s` so only `nm -m` works there; gcc and clang `-Wconversion` disagree in opposite directions so every narrowing needs an explicit cast; strict `-std=c11` on glibc hides POSIX functions from the runner; `r1.toml` has 7 open fields (not 8) and `powerup_policy` has no `allowed` list in the frozen record; the refusal enum has no code for a bad caller memory argument; a stepping API that only advances a counter makes the interleave and perturbation tests vacuous; and the D-56 "shape" rule would flag two existing 16384-byte ASCII control files.

**Primary recommendation:** Build in this order: (1) behaviour-preserving `run_controls` refactor of `scripts/check`, proved by transcript equality and `--selftest`; (2) Wave 0 infrastructure (outcomes.def, flatten script, separable result-ingest, CMake presets, runner skeleton emitting RESULT lines); (3) core and refusal matrix; (4) enforcement checks, each shipped together with its mutation-proven controls; (5) CI workflow last. Never add an enforcement rule without the control that proves it can go red.

<user_constraints>
## User Constraints (from CONTEXT.md)

The decisions below are locked. The full verbatim text is in `02-CONTEXT.md` (D-40..D-64, about 45 dense lines); it is not re-copied here, to avoid two copies drifting. The planner MUST read that file. One-line index of what is locked:

- D-40 layout `src/core` -> `ds_core`, `src/text` -> `ds_text`, `tests/conformance` runner; no public header, no install rules.
- D-41 preset `gate` (Ninja, `build/gate`, `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror -fwrapv`, extensions OFF); separate ASan+UBSan preset inside the fast gate.
- D-42 caller-supplied fixed-size instance with embedded 1 MiB ROM array, copy-in, no malloc, `ds_boot(mem, cap, &request, &out)`. D-43 refusal cannot mutate by API shape.
- D-44 sizes 256K/384K/1M plain only, mirror by 8 KiB bank index in `$00-$7F`, all `assumed`; no profile r2. D-45 declared mapper/byte_order/header, never guessed. D-46 closed refusal enum, fixed precedence, 1 MiB cap. D-47 in-house SHA-256.
- D-48 `profile_fields.def` X-macro. D-49 resolve without defaults. D-50 power-on `zero` / `seeded-prng-v1`, splitmix64 in-house. D-51 one awk parser plus `scripts/conformance-flatten` TSV. D-52 canonical state digest. D-53 run record.
- D-54 `scripts/core-boundary-check`. D-55 CORE-01 tests. D-56 `scripts/rights-check` plus `audit-history --range`.
- D-57 seven outcomes, `outcomes.def`, RESULT lines, gate recomputes verdict. D-58 inventory, five named controls, `run_controls` refactor first. D-59 manifest and synthetic fixtures. D-60 gate structure and stage order.
- D-61 one `gate` job on ubuntu-24.04, `fetch-depth: 0`. D-62 pinning and security, `scripts/workflow-check`. D-63 timing record. D-64 push and merge policy NOT decided: CI-01 stays "unverified-pending-push" until the owner authorises a push.

### Claude's Discretion (verbatim)
- Exact CMake target and file names inside the layout; exact `RESULT` field order beyond the fields named; header names; the wording of new ledger statements and reason codes beyond those listed; how many plans and in which wave order, with the constraint that the `scripts/check` control-loop refactor comes first and the CI workflow comes after the gate stages it calls exist.
- Whether `src/text` stays a separate library or folds into the runner if it stays under ~150 lines.

### Deferred Ideas (OUT OF SCOPE, verbatim)
- Public `duospool.h`, install rules, `.pc`, installed-consumer proof (Phase 6, D-05).
- libFuzzer/Miri/wasm jobs (CI-02, staged); scheduled macOS/Windows jobs (only if a trial shows a distinct failure class); zizmor.
- TG16 byte order, copier headers, 512K/768K/192K sizes, special mappers (need new evidence).
- Branch ruleset on main (owner gate, D-64); REUSE adoption; a CI cache (measure first).
</user_constraints>

## Project Constraints (from CLAUDE.md / AGENTS.md)

- No commercial ROMs, firmware, system cards, disc images, private saves, or credentials in the tree, in public CI, or in logs. A hash grants no redistribution right.
- Core boundary: deterministic behaviour in the core; hosts own files, pacing, devices, UI, persistence.
- Small flat dependency tree; a copied helper needs recorded provenance (record 0007). SHA-256 and splitmix64 are written in-house, so no PROVENANCE entry.
- Evidence honesty: a matching hash is repeatability only (E0). Support claims name profile and tier. Phase 2 cannot reach E2 without a CPU, so LED-LOAD entries stay `assumed`.
- Fast PR path stays fast; CI jobs must own a distinct failure class; review CI efficiency after every run and record it.
- Authority gates (visibility, push to public remote, release, rights) need the owner. Routine reversible work proceeds.
- Standing protocols: verification shifts left with zero human UAT; reproduce missing environments (for example a Linux container) and label such runs one-off, not CI claims.
- GSD workflow: file edits go through a GSD command. This research only writes this RESEARCH.md.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PROF-02 | Unsupported or mismatched profile refused with typed reason | Refusal rows `profile_unknown`, `profile_mismatch`, resolve codes; Validation map rows V-PROF-02 |
| PROF-03 | Every guest-visible setting in profile identity and in every run record | X-macro 13 fields, per-field flip test, record golden; note 7 (not 8) open fields in r1 |
| CONT-01 | Plain HuCard of supported sizes with documented mirroring; others refused | Mirror derivation checked against MAME, Beetle, Geargrafx source (CODE); bank-signature tests with independent awk oracle |
| CONT-02 | Byte order and copier header are declared inputs; never guessed | Heuristic-negative tests derived from MAME's `ROM[0x1fff] < 0xe0` and `len % 0x4000 == 512`; PCEdev pinout evidence |
| CONT-03 | Malformed content refused without panic, unbounded allocation, or mutation | Refusal matrix, SIZE_MAX dummy pointer under sanitizers, alloc shim (works on macOS, CODE), digest before/after |
| CONT-04 | Core never reads files; content identity recorded | `nm` L1/L2 check, in-house SHA-256 verified vs NIST and `shasum` (CODE), run-record `[content]` |
| CORE-01 | Per-instance state; two instances do not share state | Interleave harness plus liveness control library; non-vacuous stepping requirement |
| CORE-02 | No host clock/fs/thread/env/device/UI in core, enforced | `core-boundary-check`; verified compile and `nm` behaviour on both platforms; corrected L1 |
| CORE-03 | Seeded documented power-on policy, recorded per run | splitmix64 vectors verified (CODE); one stream per domain; record `[power_on]` |
| RUN-02 | Seven distinct outcomes; only pass is green | `outcomes.def`, ingest recomputation, fabricated-results controls |
| FIX-02 | Fixture manifest, fail closed on missing field or unlisted fixture | `fixtures/manifest/<id>.toml`, hash of `--emit` output vs manifest, mini-root controls |
| FIX-04 | Required-test inventory fails closed, calibrated by named control | `inventory-check` codes, REQUIRED list, removed-required-test control |
| FIX-05 | No commercial ROMs or firmware in tree, CI, logs | `rights-check` (false-positive survey of current tree done), redaction test, `audit-history --range` |
| TOOL-02 | One documented local command equals the fast PR gate | `sh scripts/check` unchanged; workflow-check asserts the CI step is that command; README/AGENTS grep |
| CI-01 | CI runs the fast gate with distinct failure classes, timings recorded | Workflow skeleton validated by actionlint 1.7.12; runner image facts; D-64 push dependency |
</phase_requirements>

## Architectural Responsibility Map

This is a native C library plus shell gate, not a web app. Tiers are adapted.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Profile resolve/validate, mapper boundary, power-on, digest, run facts | `ds_core` (freestanding C11) | none | Deterministic machine behaviour; no libc beyond mem*; no I/O |
| Canonical text printing of profile and run record | `ds_text` | runner | Printing only (D-47/D-40); keeps core text-free |
| Reading TOML (profile, cases, manifest) | sh+awk `toml-subset.awk` and `conformance-flatten` | C runner reads TSV | One parser, gate must work with a broken C build (D-51) |
| Running cases, emitting RESULT lines | C runner (`tests/conformance`) | none | Runs all cases in one process; never judges |
| Verdict (required vs optional, outcome colour) | sh gate (`scripts/check`, ingest) | none | Gate recomputes from RESULT lines; runner summary never trusted |
| Rights tree scan, manifest, inventory, workflow checks | sh+awk checkers under `scripts/` | tests/gate mini-roots | Independent of the C build; fail closed |
| Wall-clock watchdog | sh harness (`perl -e 'alarm'`) | CI `timeout-minutes` | Core has no clock; wall expiry is `harness_failure`, never guest evidence |
| CI | GitHub Actions `gate` job | none | Thin wrapper over `sh scripts/check` (record 0006) |

## Standard Stack

### Core (all CODE: probed this session)
| Tool | Version here / on runner | Purpose | Notes |
|------|--------------------------|---------|-------|
| C compiler | Apple clang 21.0.0 (arm64-apple-darwin25.6.0); runner gcc 13.3.0, clang 18.1.3 (also 16, 17) | `-std=c11` core, runner | runner list FACT from runner-images `Ubuntu2404-Readme.md`, image 20261101.360.1 |
| CMake | 4.4.3 here; 3.31.6 on runner image; 3.28.3 from Ubuntu apt (tested) | presets, Ninja generator | Presets `"version": 6` works on 3.28.3 and 4.4.3 (CODE). Do not use a newer preset schema |
| Ninja | 1.13.2 here and on runner; 1.11.1 Ubuntu apt | generator | |
| POSIX sh | macOS `/bin/sh` = bash 3.2; Ubuntu `/bin/sh` = dash; `/bin/dash` also on this Mac | gate | Existing gate passes under both (`dash scripts/check` PASS, 9.96 s here, CODE) |
| awk | macOS BWK 20200816; Ubuntu mawk 1.3.4 20240123 or gawk 5.2.1 | parsers and checks | Phase 1 evidence passes under mawk and gawk. Installing gawk makes it the default `/usr/bin/awk` (CODE) |
| actions/checkout | v7.0.1 = `3d3c42e5aac5ba805825da76410c181273ba90b1` | CI checkout | `git ls-remote` this session: lightweight tag, no `^{}` line (CODE) |
| actionlint | 1.7.12 (local) | one-off workflow lint | Draft workflow below lints clean (CODE) |
| Docker | 29.5.2, aarch64 host, runs `ubuntu:24.04` as linux/amd64 under emulation | one-off Linux repro | apt install of gcc, clang, cmake, ninja took about 80-100 s per run |

### Supporting / Alternatives
No libraries. No test framework (D-60). CTest is optional convenience. Alternatives already rejected in CONTEXT (mbedTLS, Brad Conte SHA, Catch2/Unity, Rust, python).

**Installation:** none (no package manager step). CI uses preinstalled tools on `ubuntu-24.04`.

## Package Legitimacy Audit

No npm, PyPI or crates packages are installed in this phase, so `package-legitimacy check` does not apply. The only external dependency is the GitHub Action `actions/checkout`, pinned by full SHA. SHA verified against the tag by `git ls-remote https://github.com/actions/checkout refs/tags/v7.0.1` (CODE): `3d3c42e5aac5ba805825da76410c181273ba90b1 refs/tags/v7.0.1`, and `v7` points at the same object. Re-verify immediately before committing the workflow (D-62), as the tag could move. Packages removed: none. Packages flagged: none.

## Architecture Patterns

### System Architecture Diagram

```
 host bytes + profile id + declared {mapper, byte_order, header} + bindings
        |
        v
   ds_boot(mem, cap, &request, &out)                         [ds_core, no malloc]
        |-- validate request/struct_size/reserved ---------> refusal result (writes only `out`)
        |-- resolve profile (D-49) ------------------------> refusal naming the field
        |-- mapper/byte_order/header declared? supported? -> refusal
        |-- null/empty/cap/short/bank-multiple/size enum --> refusal   (fixed precedence D-46)
        v
   zero instance memory, copy ROM, power-on policy (splitmix64 domains)
        |
        v
   instance (digest, machine time, mapper read of 128 banks)  ---> ds_text prints profile + run record
        |
        v
   runner (tests/conformance): all cases in ONE process --> stdout: RESULT <id> <outcome> assertions=<n> evidence=<relpath>
                                                          \-> --emit <fixture> bytes, --list ids
        |
        v   (sh only from here; C never judges)
   scripts/check: preflight -> existing stages -> rights-tree -> manifest/inventory/ledger-pin
        -> workflow-check -> cmake --preset gate + build -> runner (matrix) -> RESULT ingestion vs REQUIRED
        -> determinism + core-boundary -> controls (tests/gate mini-roots) -> outcome-sync
        |
        v
   CHECK <stage> PASS|FAIL|ERROR  + CHECK RESULT; exit 0 | 1 | 2     <-- same command in CI (job `gate`)
```

### Recommended Project Structure
```
src/core/            ds_core: profile, resolve, instance, mapper, poweron, serial, sha256, content id
src/text/            ds_text: printers only
tests/conformance/   runner, synth.c, outcomes.def, cases/<id>.toml, REQUIRED
tests/controls/      toy libraries that MUST fail the real tests (shared-state, wrong-mirror, heuristic)
tests/gate/<case>/   mini-roots + EXPECT, judged by the real checkers
tests/workflow/<case>/  workflow-check controls
fixtures/manifest/<id>.toml
scripts/             check (refactored), rights-check, core-boundary-check (+ .allow), inventory-check,
                     fixture-check (name is discretion), workflow-check, conformance-flatten, conformance-ingest
.github/workflows/gate.yml, .github/dependabot.yml, pull_request_template.md, CODEOWNERS
CMakeLists.txt, CMakePresets.json, .gitattributes
```

### Pattern 1: Judge RESULT lines in a separable ingester (Wave 0)
**What:** `conformance-ingest RESULTS_FILE --required REQUIRED --inventory DIR` reads a file, so controls feed fabricated results and prove each outcome's colour without a build. **Why:** otherwise "skip is red", "unknown token is harness_failure", "zero tests is red" can only be shown by crafting C behaviour. Emit one verdict line plus a marker (`STATUS ...`) so `checker()` in `scripts/check` can treat marker-less exit 0 as `CHECKER_DEAD` (existing convention, CODE: `scripts/check:27-35`).

### Pattern 2: Toy control libraries instead of `#ifdef` plants
A control is a second tiny library compiled against the same tests (D-55). `tests/controls/shared-state` (hidden static counter), `wrong-mirror` (384K as `b & 0x3F`), `heuristic-reverse` (bit-reverses when byte at `$1FFF` is below `$E0`), `drop-region` (serializer omitting a region). The real test binary links the toy and the test must report failure. This keeps the production core free of test hooks.

### Pattern 3: Mini-root controls
`tests/gate/<case>/` is a full mini repository root with an `EXPECT` file; the real checker runs with `--root`. Existing precedent: `tests/provenance/<case>` with `--root` (CODE: `scripts/check:359-408`). Run-time-created controls (planted ROM-like file in a temp git repo) follow `tests/audit/synthetic.sh` (`new`, `commit`, `verdict` helpers; `GIT_CONFIG_GLOBAL=/dev/null`; CODE).

### Anti-Patterns to Avoid
- Runner decides pass/fail or required-vs-optional (D-60: the sh gate is sole judge).
- Per-case process spawn (gate is spawn-dominated: 9.7 s for 18 stages locally, CODE).
- Scanning sanitizer builds with `nm` (instrumentation adds symbols); scan `-O0` plain objects.
- Putting `-Werror` in a user-facing default; keep it in the `gate` preset only (D-41), because newer compilers add warnings.
- Reusing the spike's FNV-1a digest or `calloc`ed instance: D-47/D-42 reject both.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parsing TOML in C | second parser | `toml-subset.awk` via `conformance-flatten` TSV with `END` sentinel | Two parsers need a differential corpus (D-51) |
| UTF-8 validity in `rights-check` | awk byte machine | `iconv -f UTF-8 -t UTF-8 FILE >/dev/null` | Same verdicts on macOS and Ubuntu for random bytes, 0xEA fill, overlong, surrogate, truncated, 0xFF; accepts `CD-ROM²` (CODE, both platforms) |
| Symbol inspection | ELF/Mach-O reader | `nm -P -u` (undefined) and `nm -m` on Darwin / `nm -P` on Linux (writable types) | Verified formats below |
| Wall-clock timeouts | sh job control | `perl -e 'alarm N; exec @ARGV'` | macOS has no stock `timeout(1)` (this Mac has Homebrew's; do not rely on it) |
| Hashing in the gate | own sha in sh/awk | `sha256sum` if present else `shasum -a 256` (macOS has both here: `/sbin/sha256sum`, `/usr/bin/shasum`) | Independent oracle for the C implementation |
| Fixture ROM bytes | any real or ROM-derived data | `synth.c` integer LCG | FIX-05 |

**Key insight:** the value of Phase 2 is that each rule is mutation-proven. Hand-rolled helpers are limited to SHA-256 (about 60 lines, verified), splitmix64 (6 lines) and the awk/sh checkers.

## Existing Code: Reuse and Refactor (focus 5)

| Asset | State (CODE) | Reuse | Must change |
|-------|--------------|-------|-------------|
| `scripts/check` (708 lines) | 18 stages, helpers `checker()`, `read_expect()`, `run_stage()`; selftest has 5 mutations; `set -eu`, `trap rm -rf $tmp EXIT` | Keep exit contract 0/1/2, `CHECKER_DEAD` marker rule, `CHECK <stage>` lines, `--selftest` | About 400 lines of copy-pasted loops in 6 stages (`stage_ledger_negatives` 53-90, `decision_negatives` 123-172, `profile_negatives` 190-245, `provenance_negatives` 359-408, `licence_negatives` 416-465, `results_negatives` 541-597). Factor to `run_controls DIR CHECKER ARGS...` with the same codes `HARNESS_NO_EXPECT`, `HARNESS_EMPTY_EXPECT`, `CONTROL BROKEN`, `SAMPLE REJECTED`, `NO_CONTROLS`. Proof of equivalence: capture `sh scripts/check` stdout before; after refactor stdout equal; plus three mutated-tree transcripts (broken EXPECT, empty dir, removed control) equal. Add selftest mutations for every new rule |
| `scripts/lib/toml-subset.awk` (68 lines) | POSIX awk only; emits `TABLE KEY TYPE VALUE LINE` plus `END NR`; line cap 400, ASCII only, `\037`-joined arrays, no escapes | The only parser. `conformance-flatten` wraps it | Run-record rules in D-53 say strings escape `"` and `\`, but this parser rejects any backslash (`qstr`: "subset: no escapes at all"). Records are output (printed by C), but the golden-compare path never parses them with this awk unless a lint does. If a record lint parses records with it, record strings must not contain `\` or `"` in Phase 2, or the parser needs a deliberate extension. Planner to decide |
| `scripts/ledger-check`, `profile-check`, `decision-check`, `provenance-check`, `licence-check` | all follow `--expect`, `--root`, `EXPECT_OK` marker, exit 0/1/2, `HARNESS_EMPTY_EXPECT` | Template for every new checker (`rights-check`, `inventory-check`, `fixture-check`, `workflow-check`, `core-boundary-check`) | New files under `scripts/` need the two SPDX lines in the first five lines (`licence-check` enforces, CODE: `scripts/licence-check:11`, "Every regular file under ROOT/scripts/ must carry both SPDX lines in its first five lines (SPDX_HEADER_MISSING)"); `.allow` files carry them as comments like `audit-history.allow` |
| `scripts/audit-history` (208 lines) | `git rev-list --all`, `--objects --missing=print --all`, per-commit `git ls-tree -r`, readfail discipline, shallow-clone exit 2, allowlist `path<TAB>CHECK<TAB>reason` | Add `--range BASE..HEAD`: replace both `--all` with the range, keep the shallow check only for full mode (a range inside a depth-limited fetch is valid when BASE is present) | Self-match safety (patterns written so the file does not match itself) must be preserved. `tests/audit/synthetic.sh` is NOT a stage of `scripts/check` today (CODE: no reference in `scripts/check`); decide whether to add it (it takes a few seconds) |
| `tests/audit/synthetic.sh` | temp repos, `AUDITTEST <case> PASS/FAIL`, no literal key/home path | Factor helpers `g`, `new`, `commit`, `verdict` to `tests/lib/gitfix.sh` for `rights-check` run-time controls | none |
| Spike tag `spike-tool-01-evidence`, path `spikes/tool-01/c/` | `CMakeLists.txt` (static lib + runner, `-Wall -Wextra -Wconversion -Werror`, C11 extensions OFF); `src/core.c` (status enum; refusal decided from length before any read; `memcpy` into instance); `src/runner.c` (`run_keeps_state`, `run_interleave` shape); `gates.sh` (every stage runs, `GATE <stage> PASS/FAIL`, `case_check` awk, `perl alarm 120`, `trap 'exit 130' INT TERM`, seeded-leak control); `gen-fixtures.sh` (LCG, hash vs manifest) | Layout and stage ideas; the `perl alarm` watchdog; the seeded-leak control idea (maps to the D-56 redaction test); the manifest-hash compare | Spike core uses `calloc`, FNV-1a digest, a public `duospool.h`, `DS_PLANTED_BUG` `#ifdef` plant: none of these carry over. Spike gen-fixtures writes bytes via awk and `printf "$(cat esc)"` (octal escapes). D-59 replaces this with a C generator; note the awk LCG product stays below 2^53 (1664525 * 4294967295 is about 7.15e15), so it is exact in double precision on every awk |
| `ledger/entries/LED-LOAD-0001.toml`, `0002.toml` | `pinning_test = "PENDING:2"` (lines 11); fields per `ledger/SCHEMA.md` | Change pins to `TEST:<id>` once cases exist; add LED-LOAD-0003/0004/0005 and a PROF entry | Area list is fixed `PROF CPU VID AUD IRQ LOAD IO TOOL`; ASCII only, at most 16384 bytes, at most 400 chars per line, one-line arrays; `assumed` needs `assumption` and `revisit_phase`; `sources` items are `LABEL|ref|detail` with LABEL in `FACT CODE REPORT UNKNOWN` |
| `profiles/pce-hucard-2btn/r1.toml` | 13 field tables; frozen by record 0002 | Authority for the X-macro equality test | See Common Pitfall 5 |
| `.gitignore` | `build/`, `*.bin`, `*.pce`, etc. already ignored | `build/gate` is already ignored | `.gitignore` is not the FIX-05 gate (D-56) |

Measured baseline (CODE): `sh scripts/check` 9.7 s, `dash scripts/check` 9.96 s, `--selftest` 0.29 s, 18 stages, all PASS on this machine.

## HuCard Evidence Report (focus 2)

**Verdict: no primary Hudson document could be fetched. Nothing here promotes a ledger entry. The new evidence is consistent with D-44/D-45 and contradicts none of it.**

What I tried and could not reach: `pcetech.txt` (no copy found by search; the RHDN mirror `otakon273.duckdns.org/.../documents/` timed out); Archaic Pixels (`archaicpixels.com` timed out; `www.` has a certificate mismatch); necretro.org (Anubis bot wall, "Access Denied"); consolemods.org TG16 connector page (HTTP 403); git.redump.net MAME mirror (HTTP 403, but raw GitHub works). `dumping.guide`, Wikipedia HuCard and the nicole.express article carry nothing on bit order, sizes or headers. UNKNOWN: whether Hudson's "Develo Book"/pcetech states a 384 KiB rule.

Prompt-injection note: the nicole.express page (`/2022/the-voice-of-the-heavens-is-only-2kb.html`) contained text addressed to AI agents asking for a game copy and to change instructions. It was ignored and is flagged. Also: the existing LED-LOAD-0001 source `REPORT|Wikipedia, nicole.express|...reverses data lines` is NOT reproduced by the pages I fetched (the Wikipedia HuCard page and that nicole.express article say nothing about it). The better citation is the PCEdev wiki below.

### Evidence that supports or constrains the decisions

| # | Finding | Label and source | Bearing |
|---|---------|------------------|---------|
| H1 | Physical window: banks `$00-$7F` are "HuCard" on PC Engine, SuperGrafx and PC Engine CD; `$80-$87` "(unmapped)" on PC Engine and SuperGrafx (RAM 64 KiB on CD); `$88-$F7` "(unmapped)"; `$F8` RAM 8 KiB; `$FF` hardware registers. Quote: "most HuCards lack any sort of memory mapper and simply use the highest address bit as a chip enable." | FACT (community wiki, secondary to Hudson): `https://pce.nesdev.org/w/index.php?title=CPU_memory_map&action=raw`, fetched | Supports D-44 window and "banks `>= $80` not ROM" (LED-LOAD-0005). Wiki, not Hudson, so stays `assumed` |
| H2 | "PA20 is used as /CE" and PA0..PA20 is the "21-bit post-MMU address" | FACT: `https://pce.nesdev.org/wiki/HuCard_pinout` | Consistent with a 1 MiB window with no mapper |
| H3 | "The order of these pins depends on region. For PC Engine, they are ascending, and for TurboGrafx-16, they are descending." Table: pin 15 "CPU D0 (PCE) or D7 (TG16)", pin 23 "CPU D7 (PCE) or D0 (TG16)" | FACT: same page | A physical wiring difference. It does not say which orientation any dump file has. This is the strongest argument for "declared, never detected": file orientation depends on the dumper adapter. Better source for LED-LOAD-0001 than the cited secondary articles |
| H4 | MAME bit-reverses the whole image when `ROM[0x1fff] < 0xe0` ("check for bit-reversal (US carts)", `pce_slot.cpp` lines 240-251) | CODE: `raw.githubusercontent.com/mamedev/mame/master/src/devices/bus/pce/pce_slot.cpp`, read | PROPOSAL (my inference, not in source): `$1FFF` is the reset-vector high byte of bank 0 (MPR7 at reset maps bank 0 at `$E000-$FFFF`), so a PCE-order card should hold `>= $E0` there; the "unexplained threshold" in LED-LOAD-0001 is probably this. It is exactly the content heuristic D-45 forbids, so it becomes a negative test (below) |
| H5 | Header detection differs: MAME strips 512 bytes only when `(len % 0x4000) == 512` and not loaded via softlist (line 224); Geargrafx `if(size & 512)` (media.cpp:393-398); Beetle `if(len & 512)` (huc.cpp:196-201) | CODE: three files read | Confirms LED-LOAD-0002. Disagreement set: sizes `8K*(2k+1)+512`, for example 24576+512, are stripped by Beetle/Geargrafx but not MAME. Add such rows to the refusal matrix as heuristic-disagreement rows |
| H6 | Street Fighter II' recognised by `len == 0x280000` (2.5 MiB) in MAME (`pce_slot.cpp:294`); Populous by the string `POPULOUS` at `0x1f26`; Beetle detects `MCGENJIN` at `0x1FD0` | CODE | D-46's 2.5 MiB `sf2` vector is right. Content-string detection exists in emulators: add "image containing `POPULOUS` / `MCGENJIN` at those offsets, declared `plain`, loads as plain" as a negative test |

### 384 KiB rule and conflicts (bank index `b` = 0..127)

| Size | MAME (`rom_map_setup`, 128 KiB blocks) | Beetle `huc.cpp` and Geargrafx `media.cpp` (8 KiB banks) | D-44 |
|------|----------------------------------------|------------------------------------------------------------|------|
| 384K | block map `0,1,0,1,2,2,2,2` (`pce_slot.cpp:78-89`) | `b<64`: file bank `b & 0x1F`; `b>=64`: `(b & 0x0F) + 0x20` (huc.cpp:281-298, media.cpp:1201-1220, quoted from source) | Equal to all three. I derived MAME's table into bank terms: blocks 0-3 give file blocks 0,1,0,1, which is `b & 0x1F`; blocks 4-7 give file block 2, which is `$20 + (b & $0F)`. CODE (derivation) |
| 256K | generic fill: hand-simulated to `0,1,0,1,0,1,0,1` = `b mod 32` | default `b % bank_count` = `b mod 32` | Equal |
| 1M | identity | `b % 128` = identity | Equal |
| 512K | `0,1,2,3` repeated: `b mod 64` | `b<64`: `b`; `b>=64`: `(b & 0x1F) + 0x20` (file banks 32..63 repeated) | **Conflict** (LED-LOAD-0004; sizes refused: correct) |
| 192K | special `0,1,2,2,0,1,2,2` | default modulo 24 banks | **Conflict** |
| 768K | not special-cased | Geargrafx special case 0x60 banks | Refused |

Inference [PROPOSAL, UNKNOWN as hardware fact]: the three-way agreement on 384K fits a 256K region selected when PA19 is 0 plus a 128K region selected when PA19 is 1; no source explains the wiring. Search found only a forum guess about two mask-ROM blobs. Keep LED-LOAD-0003 `assumed`.

Geargrafx pads non-8K-aligned images with `0xFF` and Beetle does the same (`memset(HuCROM, 0xFF, m_len)`); neither matters because DuoSpool refuses non-bank-multiples. The "open-bus value for unmapped banks" belongs to Phase 3.

## Implementation Pitfalls: C11 on macOS arm64 and Ubuntu 24.04 (focus 3)

All rows were run this session on Apple clang 21.0.0 and in `ubuntu:24.04` with gcc 13.3.0 and clang 18.1.3 (CODE) unless labelled otherwise. Scratch sources are in `/private/tmp/claude-501/-Users-jon-projects-duospool/06f2647a-e8db-469c-a561-38fda7909897/scratchpad/r2/`.

| # | Topic | Observation | Action |
|---|-------|-------------|--------|
| P1 | Freestanding headers (D-54 L1) | Under `-std=c11 -ffreestanding -nostdinc -isystem <compiler include dir>` these resolve: `stddef stdint stdbool float stdarg stdalign stdnoreturn iso646`, and `limits.h` on clang (Apple 21, clang 18) but NOT on gcc 13 (`<limits.h>` FAIL). `stdio string stdlib time` fail everywhere. Compiler include dirs: Apple `.../XcodeDefault.xctoolchain/usr/lib/clang/21/include`; gcc `/usr/lib/gcc/x86_64-linux-gnu/13/include`; clang 18 `/usr/lib/llvm-18/lib/clang/18/include` | The claim "only stddef/stdint/stdbool exist" is wrong. Add an include allowlist: `grep -nE '^[[:space:]]*#[[:space:]]*include'` on each core source, allowed `<stddef.h> <stdint.h> <stdbool.h>` (and `<stdalign.h>` if wanted) plus `"ds_*.h"`; everything else is a violation. Keep D-54's ban on `limits.h` and the `float|double` grep (`float.h` resolves). Negative control: include `<limits.h>` (compiles on clang, so only the grep catches it) |
| P2 | `nm -P -u` (L2) | Darwin arm64 at `-O0`: `memcpy`, `bzero` (a zero `__builtin_memset` becomes `_bzero`), leading underscore. Linux: `memcpy U`, `memset U` followed by padding spaces. A 20 KB stack array added NO `___chkstk_darwin` on arm64 (the allow entry from D-54 is harmless but unneeded here; UNKNOWN for x86_64 macOS). SHA-256 written with plain loops: empty undefined set on all three compilers | Normalise: strip a leading `_` on Darwin, take field 1, `LC_ALL=C sort -u`, compare to allow file. Keep `bzero` allowed on Darwin only |
| P3 | `nm` writable-data detection (L3) | Linux gcc and clang `nm -P`: bss `b`, data `d`, const `R`, function-local static `local.0 b` (gcc) or `bump.local b` (clang). **Darwin `nm -P` types const data `S`/`s`** (external `const` table shows `S`, static const `s`), so a letter rule false-positives on const tables there. Darwin `nm -m` shows `(__DATA,__bss)`, `(__DATA,__data)`, `(__TEXT,__const)`. Tested rule: `nm -m obj \| awk '$2 ~ /^\(__DATA,__(data\|bss\|common)\)$/ \|\| $2 ~ /^\(__DATA,__thread/ \|\| $0 ~ /\(common\)/ {print "WRITABLE " $NF}'` flagged `_bump.local _counter _init` (and their `ltmpN` aliases) and nothing on a const-only object. Function-local `static const` is read-only on all three | Use `nm -m` on Darwin and `nm -P` letters `BbCDdGgVvu` on Linux (drop `S s`, which on Linux are not the const class). The negative control for a function-local static must be non-const. Control: an object with zero symbols is exit 2 (harness), not a pass |
| P4 | `-Wconversion` | Apple clang 21 and clang 18 warn on `return ~a;` (u8) and `return (hi << 8) \| lo;` (u16) but NOT on `uint8_t r = a + b`, `a += 1`, `x >> 8`, `x & 0xff`. gcc 13 warns on `return x >> 8;`, `return x & 0xff;`, `return (x >> 4) & 0xf0;` (u16 to u8) but NOT on `~a` or `(hi << 8) \| lo`. Opposite directions. A variant with an explicit cast on every narrowing was clean on all three with `-Wall -Wextra -Wconversion -Wshadow -Werror` | Rule: every narrowing from promoted `int` to u8/u16 gets an explicit cast (`(uint8_t)(x >> 8)`, `(uint8_t)~a`, `(uint16_t)(((uint16_t)hi << 8) \| lo)`); build 24-bit-plus values from `(uint32_t)p[0] << 24` |
| P5 | Shifts and `-fwrapv` | `(uint32_t)(p[0] << 24)` with `p[0] = 0x80` is UB (left shift of 128 by 24 into `int` sign bit); UBSan reports "left shift of 128 by 24 places cannot be represented in type 'int'" on all three. `-fwrapv` does not cover this | Widen before shifting: `(uint32_t)p[0] << 24`. The sanitizer preset catches regressions; add the shift to the SHA-256 and serializer tests |
| P6 | Strict `-std=c11` hides POSIX | gcc 13 and clang 18 with `-std=c11 -pedantic -Wall -Wextra -Werror`: `strdup` and `fdopen` are errors (implicit declaration). macOS accepts them | The runner uses ISO C only (`fopen`, `fread`, `snprintf`) or defines `_POSIX_C_SOURCE 200809L` before any include in the runner only. Catching this is the job of the Linux CI job (distinct failure class) |
| P7 | Alignment of caller memory | `_Alignof` of a struct holding `uint64_t` plus a 1 MiB array: 8 on both; `sizeof` of the toy struct 1048584. `max_align_t` 8 (Darwin arm64) vs 16 (Linux x86_64). Casting `raw + 1` and reading a member: UBSan `-fsanitize=alignment` reports "member access within misaligned address ... requires 8 byte alignment" on all three | `ds_boot` must check `(uintptr_t)mem % ds_instance_align() == 0` and `cap >= ds_instance_size()` before any cast or write (`uintptr_t` is in `<stdint.h>`). Add a misaligned-memory refusal row (`mem + 1`) and a short-cap row. `calloc` memory (16-byte aligned) satisfies the test harness |
| P8 | 1 MiB instance on stack vs heap | Six 1 MiB locals under ASan with the default 8 MiB main-thread stack ran fine on both OSes. macOS secondary threads default to 512 KiB (REPORT, not tested) | The interleave harness holds 3-4 instances: allocate them with `calloc` in the test code (tests may allocate; only the core may not). Add `-Wframe-larger-than=` or simply forbid large locals by review. Keep canary-adjacent layout by allocating one block of N instances plus guard bytes |
| P9 | Link-time allocation shim | On macOS arm64 a `malloc`/`calloc`/`realloc` definition in the test executable counted a call made by a statically linked object (count 1) and did not count libsystem-internal `printf` allocations (count 0); forwarding used `malloc_zone_malloc(malloc_default_zone(), n)` | The D-46 allocation-counting shim is feasible on macOS. Linux: ELF interposition normally redirects all callers including libc (REPORT, untested here), so count only inside the measured window and forward with `__libc_malloc` or reset the counter around the call. UNKNOWN until built; `nm` L2 (no allocator symbols in core) stays the primary proof, and the shim is a second witness with its own self-check (call `malloc` from the test, require count 1) |
| P10 | SHA-256 | In-house implementation (about 60 lines: big-endian word load, `K` table, `ds_sha256_update` buffering, final pad `0x80`, zeros to 56 mod 64, 64-bit length in bits big-endian) compiled clean with `-Wconversion -Werror` and freestanding flags on all three compilers with an empty undefined-symbol set. Digests matched `shasum -a 256` for: empty `e3b0c442...b855`; `abc` `ba7816bf...15ad`; 448-bit message `248d6a61...06c1`; one million `a` `cdc76e5c...2cd0`; and 21 lengths 0,1,54,55,56,57,63,64,65,119,120,121,127,128,129,8191,8192,8193,262144,393216,1048576 (LCG data fed in 1..7-byte chunks, so buffering is exercised). Mutant using byte length instead of bit length: 55-byte digest wrong (`8668abb5...` vs `1b852bbf...`) | Use exactly these vectors and lengths. Pitfalls proven by the mutant: length must be bits; padding boundary lengths 55/56/63/64 and the 119/120 pair catch most bugs. Keep NIST vectors as spec literals; cross-check bulk lengths against `shasum`/`sha256sum` in the gate |
| P11 | splitmix64 | Constants `0x9E3779B97F4A7C15`, `0xBF58476D1CE4E5B9`, `0x94D049BB133111EB`, shifts 30, 27, 31 (FACT: Vigna `prng.di.unimi.it/splitmix64.c` first line seen; rand_xoshiro `splitmix64.rs` constants quoted). **Verified: seed 0 first output is `0xE220A8397B1DCDAF`** (16294208416658607535), then `0x6E789E6AA1B965F4`, `0x06C45D188009454F`. Seed 1234567: `0x599ED017FB08FC85 0x2C73F08458540FA5 0x883EBCE5A3F27C77 0x3FBEF740E9177B3F 0xE3B8346708CB5ECD` (decimal 6457827717110365317, 3203168211198807973, 9817491932198370423, 4593380528125082431, 16408922859458223821, which match the values I recall from published tests: REPORT). Third-party check: rand_xoshiro test `reference` seed 1477776061723855037 has first value 1985237415132408290 (FACT: rand_xoshiro source); computed here: `1985237415132408290`, then `2979275885539914483` | Use seed 0 and seed 1477776061723855037 as spec literals (second is third-party). `-Wconversion` clean; unsigned wraparound is defined, `-fwrapv` unnecessary for it |
| P12 | CMake presets | `CMakePresets.json` with `"version": 6`, hidden base preset, `gate`, `gate-gcc`, `gate-clang` with separate `binaryDir`s configured, built and ran on CMake 3.28.3 and 4.4.3. Effective flags `-pedantic -Wall -Wextra -Wconversion -Wshadow -Werror -fwrapv -g -std=c11` (plus `-arch arm64` on macOS). Whole configure+build+run of a one-file library took 0.36 s locally | Use separate binary dirs per compiler (changing `CMAKE_C_COMPILER` in one dir forces a cache wipe; my probe was inconclusive, so treat as UNKNOWN and avoid). `CMAKE_C_EXTENSIONS OFF` yields `-std=c11` |
| P13 | Local Linux repro | `docker run --platform linux/amd64 ubuntu:24.04` plus apt install (gcc, clang, cmake, ninja-build, gawk, mawk, binutils) works here; about 80-100 s per fresh run | One-off at phase verification, labelled "local container run, not a CI claim". Not part of the gate |
| P14 | `iconv`, `/bin/sh`, sha tools | `iconv -f UTF-8 -t UTF-8` identical verdicts on both OSes (see Don't Hand-Roll). Ubuntu `/bin/sh` is dash and `perl` is present in the base image | Gate may use `iconv`, `perl`, `sha256sum`/`shasum` |

## Common Pitfalls

### Pitfall 1: The refactor changes behaviour silently
**What goes wrong:** `run_controls` unifies loops that differ subtly (profile controls use a fixed record path `<d>pce-hucard-2btn/r1.toml` and `ok-*` without EXPECT; score controls use `scores.txt` with `EXIT=`/`CODE=` lines; filename controls build names at run time).
**Avoid:** keep score, filename and empty-expect stages as they are; only unify the six loops with one shape (dir, checker, mode flags). Capture golden transcripts before and after.
**Warning signs:** a stage count or `CONTROL BROKEN` line differs; selftest mutation stops flipping.

### Pitfall 2: The gate trusts a runner that ran nothing
**What goes wrong:** zero RESULT lines, a crashed runner, or `assertions=0` read as green.
**Avoid:** ingest treats empty input, a non-zero runner exit with no marker, duplicate ids, unknown tokens, ids outside the inventory, and REQUIRED ids with no line as non-green; runner prints a final `RUNNER_DONE n` marker and the ingester requires it.

### Pitfall 3: Mirror-table tests that reuse the core's table
**What goes wrong:** expected bank signatures derived from the same enum the core uses prove nothing (D-59 forbids). **Avoid:** the independent awk script derives expectations from the rule text in LED-LOAD, and the `wrong-mirror` control must fail. Tag each bank in synthetic images with a unique byte at the start of every 8 KiB bank and a second byte at the end, so a one-bank offset error is visible.

### Pitfall 4: Perturbation test passes because the digest is stateless
If `ds_step` only adds to a counter, the interleave test cannot distinguish shared from isolated state. See Open Question 3.

### Pitfall 5: r1.toml is frozen and has gaps (in-repo values, quoted from `profiles/pce-hucard-2btn/r1.toml`)
- 13 field tables: 6 `owner-selected` (`media_slot expansion_port vdc_count vpc chip_revision input_port1`) and **7 `open`** (`io_1000_d6_value io_1000_d7_value lines_per_frame psg_tick_divisor content_byte_order content_copier_header powerup_policy`, from `awk` over `status =` lines). D-49 says "All 8 open fields": the number is 7.
- `[content_byte_order]` line 82: `allowed = ["pce", "tg16"]`; `[content_copier_header]` line 89: `allowed = ["refuse", "declared-512"]`. D-45 request vocabulary is `byte_order` `pce|tg16` and `header` `none|copier_512`. The mapping profile `refuse` to request `none`, and profile `declared-512` to request `copier_512`, is nowhere defined.
- `[powerup_policy]` (lines 93-96) has `proposal = "seeded-prng-v1"` but no `allowed` and no `ledger` key. D-50 offers `zero` and `seeded-prng-v1`. D-48's "exhaustive-equality test vs r1.toml" and D-49 `BIND_NOT_ALLOWED` need a defined allowed set; r1 is frozen ("Frozen once a decision record or phase references it; a change is a new rN.toml", r1.toml line 4), so r1 cannot gain the key.
- `unresolved_behaviours = ["LED-IRQ-0001", "LED-VID-0002", "LED-VID-0003"]` is a root key that the resolver must also carry into identity or explicitly exclude.

### Pitfall 6: Rights check false positives on the current tree
Surveyed `git ls-files -co --exclude-standard` (381 files, CODE): no NUL bytes anywhere; non-ASCII UTF-8 prose in 26 files (maximum 150 non-ASCII bytes in 7318, about 2%, far under the 20% rule); exactly two files hit the size-shape rule: `ledger/samples/ok-boundary-16384/LED-CPU-0001.toml` and `tests/profile/ok-boundary-16384/pce-hucard-2btn/r1.toml` (16384 bytes, pure ASCII, already in `scripts/audit-history.allow`). **Avoid:** apply the shape rule only to files that are not pure printable ASCII plus LF/TAB (the same "NUL-free text" precedent), or give those two files manifest entries. `.planning/state.json` is untracked and not ignored today; it is text, so it passes.

### Pitfall 7: CI checkout and PR range
Depth-1 checkout fails `protocol-order` (measured in discuss). For `audit-history --range`, pass the base SHA through `env:` and not by `${{ }}` inside `run:` (D-62). `fetch-depth: 0` is already decided.

### Pitfall 8: awk dialect and default
Ubuntu default awk is mawk, but installing gawk makes `/usr/bin/awk` resolve to gawk (CODE: after installing both, `readlink -f /usr/bin/awk` is `/usr/bin/gawk`). To run the gate under the other implementation without root, put a symlink named `awk` to `$(command -v mawk)` or `gawk` first on `PATH`. Whether gawk is preinstalled on the runner image is UNKNOWN (the image README does not list awk), so the first CI run must print `command -v gawk mawk` and `readlink -f "$(command -v awk)"` (the draft workflow does).

### Pitfall 9: ASan builds and Homebrew LLVM
Use `/usr/bin/clang` for ASan locally (D-55). `detect_leaks=1` applies on Linux only; Apple's ASan does not support LeakSanitizer (REPORT).

## Code Examples

### CMake presets (tested on CMake 3.28.3 and 4.4.3)
```json
{ "version": 6, "cmakeMinimumRequired": { "major": 3, "minor": 25, "patch": 0 },
  "configurePresets": [
    { "name": "gate-base", "hidden": true, "generator": "Ninja",
      "cacheVariables": { "CMAKE_C_FLAGS": "-pedantic -Wall -Wextra -Wconversion -Wshadow -Werror -fwrapv", "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "gate",       "inherits": "gate-base", "binaryDir": "${sourceDir}/build/gate" },
    { "name": "gate-gcc",   "inherits": "gate-base", "binaryDir": "${sourceDir}/build/gate-gcc",   "cacheVariables": { "CMAKE_C_COMPILER": "gcc" } },
    { "name": "gate-clang", "inherits": "gate-base", "binaryDir": "${sourceDir}/build/gate-clang", "cacheVariables": { "CMAKE_C_COMPILER": "clang" } } ],
  "buildPresets": [ { "name": "gate", "configurePreset": "gate" }, { "name": "gate-gcc", "configurePreset": "gate-gcc" }, { "name": "gate-clang", "configurePreset": "gate-clang" } ] }
```
`CMAKE_C_FLAGS` is passed as a cache variable. A sanitizer preset adds `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`.

### Workflow skeleton (actionlint 1.7.12 reports 0 errors; a bogus `defaults` key is rejected)
```yaml
name: gate
on:
  pull_request:
  push:
    branches: [main]
  workflow_dispatch:
permissions:
  contents: read
concurrency:
  group: gate-${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: ${{ github.event_name == 'pull_request' }}
defaults:
  run:
    shell: sh -eu {0}
jobs:
  gate:
    name: gate
    runs-on: ubuntu-24.04
    timeout-minutes: 10
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          fetch-depth: 0
          persist-credentials: false
      - name: Tool versions
        run: |
          {
            echo "image: ${ImageVersion:-unknown}"
            cmake --version | sed -n 1p
            ninja --version
            gcc --version | sed -n 1p
            clang --version | sed -n 1p
            readlink -f "$(command -v awk)"
          } >> "$GITHUB_STEP_SUMMARY"
      - name: Gate
        run: sh scripts/check
```
This is the exact file that actionlint accepted. Block style is also what a restricted line checker (`workflow-check`) can parse. The gcc-then-clang build steps, the second awk run and the base-SHA `env:` for `audit-history --range` are added on top of this skeleton.

### Darwin writable-symbol rule and Linux undefined-symbol normalisation (tested)
```sh
# Darwin L3
nm -m obj.o | awk '$2 ~ /^\(__DATA,__(data|bss|common)\)$/ || $2 ~ /^\(__DATA,__thread/ || $0 ~ /\(common\)/ { print "WRITABLE " $NF; bad = 1 } { n++ } END { if (!n) exit 2; exit bad }'
# L2 (both): undefined symbols, Darwin leading underscore stripped
nm -P -u obj.o | awk -v uname="$(uname -s)" '{ s = $1; if (s ~ /^_/ && uname == "Darwin") s = substr(s, 2); print s }' | LC_ALL=C sort -u
```
The Darwin L3 line and the `-v` normalisation ran on this Mac's BWK awk (CODE); mawk and gawk runs of these two exact lines were not done (UNKNOWN), although the Phase 1 gate awk passes under both.

## State of the Art

| Old approach | Current approach | Impact |
|--------------|------------------|--------|
| `actions/checkout` by moving tag | full-SHA pin with `# v7.0.1` comment, `persist-credentials: false` | Required by record 0006 |
| One shared build dir for several compilers | one `binaryDir` per preset | Avoids cache churn |
| Test frameworks (Catch2/Unity) | closed-outcome RESULT lines plus sh judge | Matches D-57/D-60 |

**Deprecated/outdated in this repo's own notes:** the `.gitignore` comment says the Phase 2 FIX-05 tree check "is the real gate": it is, and `.gitignore` stays a convenience.

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | `$1FFF` high byte `>= $E0` is the rationale for MAME's bit-reversal threshold | HuCard H4 | Low: used only to design a negative test; D-45 forbids the heuristic anyway |
| A2 | 384K cards are a 256K plus 128K split selected by PA19 | HuCard | None for code: D-44 stays `assumed` |
| A3 | Linux ELF interposition lets a test-executable `malloc` also catch libc-internal callers | P9 | Low: counter window logic; `nm` L2 remains primary |
| A4 | macOS secondary threads default to 512 KiB stack; Apple ASan lacks LeakSanitizer | P8, Pitfall 9 | Low |
| A5 | gawk is or is not preinstalled on the runner | Pitfall 8 | Medium: affects whether CI needs an apt install step |
| A6 | Switching `CMAKE_C_COMPILER` inside one binaryDir misbehaves (probe inconclusive) | P12 | Low: separate dirs avoid the question |
| A7 | Seed 1234567 splitmix64 values match published vectors (matched my recollection; source not fetched) | P11 | Low: seed 0 and the rand_xoshiro vector are the cited literals |
| A8 | Darwin x86_64 may emit `___chkstk_darwin` for big stack frames | P2 | Low: allow entry kept |

## Open Questions (RESOLVED) (for the planner; none changes a locked decision)

1. **(RESOLVED: CONTEXT D-66) Bad caller-memory refusal.** D-42/D-46 define `ds_boot(mem, cap, ...)` but the closed refusal enum has no code for `mem == NULL`, `cap < ds_instance_size()`, or misaligned `mem`. Recommendation: add one stable code `memory_invalid` (D-46 enum numbers are not yet frozen), tested by rows `mem NULL`, `cap = size - 1`, `mem + 1`; order it first (before request validity) or right after it. Needed because P7 shows the cast is UB otherwise.
2. **(RESOLVED: CONTEXT D-67) Profile-vs-request vocabulary and unsupported bindings.** Define the map `refuse -> none`, `declared-512 -> copier_512`, `pce -> pce`, `tg16 -> tg16`, and what happens when the run request binds `content_byte_order=tg16` or `content_copier_header=declared-512` (both are inside r1's `allowed`). Recommendation: resolve succeeds (value allowed), `ds_boot` refuses `byte_order_unsupported` / `header_unsupported`, and a request whose declared value differs from the resolved binding is `profile_mismatch`. Also decide `powerup_policy` allowed set when r1 has none (recommendation: constant-only, and the equality test skips `allowed` for fields r1 omits).
3. **(RESOLVED: CONTEXT D-68, D-75) Non-vacuous stepping.** D-55/D-57 imply a stepping API, but with no CPU, state that only advances a time counter cannot detect sharing, and the "perturb every region" test needs regions that evolve. Recommendation: a placeholder `ds_advance(inst, cycles)` that advances machine time and the power-on-seeded `psg_noise` LFSR and `vdc_phase` counter by fixed documented rules, labelled in the ledger "placeholder state evolution, no hardware claim" (extend the D-50 PROF entry's withheld list). Without it the liveness controls cannot fail.
4. **(RESOLVED: CONTEXT D-69) Record string escapes vs the awk parser.** D-53 allows `\` and `"` escapes in printed strings; `toml-subset.awk` forbids backslashes. Either keep Phase 2 record strings free of both characters (then drop the escape rule or make the printer refuse them) or extend the parser deliberately with its own control.
5. **(RESOLVED: CONTEXT D-70) Heuristic-disagreement rows.** Add `8192+512`, `16384+512`, `24576+512` and an image with `POPULOUS` at `0x1F26` and bit-0 reversed content to the refusal/accept matrix (H5/H6). Cheap, derived from three emulator sources.
6. **(RESOLVED: CONTEXT D-71) `tests/audit/synthetic.sh` in the gate?** It is not a stage today. Adding it protects the new `--range` mode; cost is a few seconds.
7. **(RESOLVED: CONTEXT D-72) Where `LED-PROF-0002` (power-on) and LED-LOAD-0003..0005 ids go in `ledger/REQUIRED`.** REQUIRED lines are `ID SUMMARY C#`-shaped (`LED-LOAD-0001 SUMMARY C7`); new entries have no SUMMARY conflict id. Decide whether to list them.
8. **(RESOLVED: CONTEXT D-71, D-80) Repo SPDX for C sources.** `licence-check` only checks `scripts/`. Recommend extending it to `src/` and `tests/*.c` with a control, since licensing is dual MIT OR Apache-2.0.
9. **(RESOLVED: CONTEXT D-74) awk dual-run on CI** needs a documented method (Pitfall 8); first CI run decides whether apt is needed.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cc (Apple clang) | build, boundary check | yes | 21.0.0 | none needed |
| gcc | both-compiler CI run | macOS `gcc` is Apple clang; real gcc only on Linux | 13.3.0 (runner, container) | Docker repro |
| clang 18 | CI | container/runner only | 18.1.3 | Apple clang locally |
| cmake / ninja | build | yes | 4.4.3 / 1.13.2 | none (missing is `harness_failure`, D-60) |
| docker | Linux repro | yes (daemon up) | 29.5.2 | skip, label as unverified |
| actionlint | one-off workflow lint | yes | 1.7.12 | none |
| gh | `scripts/ci-timings`, SHA check | yes | present | none |
| perl | watchdog | yes | 5.34 | none |
| shasum / sha256sum | hash oracle | both present | | pick one at runtime |
| iconv | UTF-8 check | yes | system | none |
| dash | local run under CI's sh | yes (`/bin/dash`) | | |
| gawk / mawk | awk dual-run | not installed on this Mac | | container; CI step |
| python3 | not used by the gate | present | | not required, do not depend on it |

Missing with no fallback: none. Missing with fallback: gawk/mawk locally (container).

## Validation Architecture (focus 1)

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None added. C runner prints `RESULT <id> <outcome> assertions=<n> evidence=<relpath>`; sh/awk gate judges (D-57/D-60). CTest is optional and not in the verdict |
| Config file | `CMakePresets.json`; `tests/conformance/cases/<id>.toml`; `tests/conformance/REQUIRED`; `tests/conformance/outcomes.def` (all Wave 0) |
| Quick run command | `sh scripts/check` (baseline 9.7 s for 18 stages; budget warm <= 60 s, cold CI <= 3 min, D-60) |
| Full suite command | `sh scripts/check && sh scripts/check --selftest && dash scripts/check` plus sanitizer preset (inside the fast gate per D-41), `sh scripts/audit-history`, one-off Linux container run |

### Phase Success Criteria to checks

| SC | Check (automated) | Where | Negative control (shown red) | Expected failure signal |
|----|-------------------|-------|------------------------------|-------------------------|
| SC1 refusal matrix | One runner process executes `ref_*` rows: empty (NULL and non-NULL), buffer_null, too_short (1, 513, 8191), size_unsupported (8192), not-bank-multiple (256K-1), `256K+512` as none and as copier_512, 128K/192K/512K/768K/2M, too_large (1M+8K and `SIZE_MAX` dummy pointer), 2.5 MiB `sf2`, 1M `populous/tennokoe/arcade_card`, undeclared mapper/order/header, TG16, profile mismatch/unknown, bad `struct_size`, nonzero reserved, NULL request, plus new memory rows. Each asserts its own enum value; a separate case asserts the set of observed reasons covers every enum member except OK. `nonmutation_<row>` hashes a live instance's state digest before and after each row; allocation shim count 0 per row; sanitizer preset reruns the matrix | fast gate: runner stage; sanitizer stage | `tests/gate/wrong-literal`: a case expecting the wrong reason through the live runner must be seen as `guest_failure`; `drop_precedence` selftest mutation swaps two checks in a copy of the table and requires a matrix row red; shim self-check counts a deliberate `malloc` | `RESULT ref_<row> guest_failure ...`; gate line `CHECK conformance FAIL` with the id; exit 1 |
| SC2 supported sizes, mirrors, record | `accept_{256k,384k,1m}_banksig`: for all 128 banks read the first and last tag byte via the mapper boundary; expected values from independent awk reading the rule from the LED-LOAD text; record contains content sha256 (also compared to `shasum` of `--emit`), size, mapper, mapper_rule, byte_order, header_handling; `nm` L1/L2 shows no file API | fast gate | `tests/controls/wrong-mirror` (384K as `b & 0x3F`) must fail the same case; manifest hash of `--emit` changed by one byte must give `FIX_HASH_MISMATCH` | `guest_failure` on `accept_384k_banksig`; `FIX_HASH_MISMATCH` |
| SC3 profile identity, determinism, boundary | `record_golden` (cmp), `record_lint` (13 `[field_*]`, `bound_by`, no `/` paths, no `0x` pointers, no timestamps); `field_flip_<name>` x13; `digest_region_<tag>`; `interleave_*` (A,B,A order; twins; destroy and recreate; canaries); `fresh_process` (second run under `env -i`, different `TZ`, cwd, `TMPDIR`, malloc scribble; `cmp` records); `core-boundary-check` | fast gate; boundary check in determinism stage | `tests/controls/shared-state` must fail interleave; `drop-region` must fail the region test; planted record with `/tmp/x` or `0x7ff...` fails lint; boundary check controls: `<time.h>`, `clock_gettime`, `getenv`, `malloc`, global counter, function-local non-const static, initialised data, `__TIME__`; positive `ok.c`, `ok2.c`; `--selftest` deletes one `core-boundary.allow` line and requires red | `guest_failure interleave_*`; `STATUS`-less or `VIOL` output from boundary check, exit 1; marker missing is exit 2 |
| SC4 outcomes and named controls | `conformance-ingest` over fabricated RESULT files: one per outcome (only `pass` green), unknown token, duplicate id, unlisted id, zero tests, `assertions=0`, empty evidence, REQUIRED id missing; the five named controls as mini-roots under `tests/gate/` | fast gate | the controls are themselves the negatives; selftest mutations delete the rule emitting `INV_REQUIRED_MISSING`, `FIX_MISSING_FIELD`, `FIX_UNLISTED`, `ROMLIKE_BINARY` and require each control to flip to broken | codes `INV_REQUIRED_MISSING`, `FIX_MISSING_FIELD`, `FIX_UNLISTED`, `ROMLIKE_BINARY`; `guest_failure`; ingester exit 1, harness error exit 2 |
| SC5 CI | `workflow-check` static assertions (permissions, pinned SHA, `fetch-depth: 0`, shell, no forbidden triggers/keys, the `run:` equals `sh scripts/check`, template line by `grep -F`); actionlint one-off; first green run and timing row | static in fast gate; run evidence only after owner authorises push (D-64) | `tests/workflow/<case>` bad variants: `pull_request_target`, tag-pinned action, missing `permissions`, `continue-on-error`, `\|\| true`, `secrets.`, untrusted `${{ github.event.pull_request.title }}` in `run:`, missing `fetch-depth`, unparsable YAML (fail closed) | `WF_*` codes (planner names), exit 1; unparsable input exit 2 |

### Phase Requirements to Test Map

| Req ID | Behavior | Test type | Automated command | File exists? |
|--------|----------|-----------|-------------------|--------------|
| PROF-02 | typed refusal for unknown/mismatched profile and rev; no fallback | unit via runner | `sh scripts/check` (case ids `ref_profile_unknown`, `ref_profile_mismatch`, `resolve_rev_mismatch`, `resolve_open_unbound`, `resolve_bind_closed`, `resolve_bind_not_allowed`) | Wave 0 |
| PROF-03 | 13 fields with value and origin in identity and record; flipping any field changes identity | unit + golden | same (`field_flip_<name>`, `record_golden`, `record_lint`) | Wave 0 |
| CONT-01 | three plain sizes mirror per rule; all others refused | unit + independent oracle | same (`accept_*_banksig`, `ref_size_*`) + awk oracle script | Wave 0 |
| CONT-02 | declared order/header; no guessing | unit | same (`ref_byte_order_*`, `ref_header_*`, `noheur_reverse_vector`, `noheur_populous_string`, `noheur_mcgenjin_string`, `noheur_header_len_disagreement`) | Wave 0 |
| CONT-03 | no panic, no alloc, no mutation on refusal; sanitizers clean | unit + sanitizer | sanitizer preset run of the matrix; `nonmutation_*`; `alloc_zero_*` | Wave 0 |
| CONT-04 | core never opens a file; identity recorded | static + unit | `core-boundary-check`; `content_id_nist_*` (empty, abc, 448-bit, 1M `a`); `content_id_vs_shasum` | Wave 0 |
| CORE-01 | no shared mutable state | integration | `interleave_*`, `fresh_process`, shared-state control | Wave 0 |
| CORE-02 | no clock/fs/thread/env/device/UI use | static | `sh scripts/core-boundary-check` and `--selftest` | Wave 0 |
| CORE-03 | documented seeded power-on; seed required for seeded policy, refused for zero; recorded | unit | `poweron_zero`, `poweron_seed_required`, `poweron_seed_refused_zero`, `splitmix_seed0`, `splitmix_ref_1477776061723855037`, `poweron_domain_independence`, `poweron_same_seed_same_digest`, `poweron_diff_seed_diff_digest` | Wave 0 |
| RUN-02 | seven outcomes; only pass green | gate | `conformance-ingest` fabricated-results controls; `outcome-sync` | Wave 0 |
| FIX-02 | manifest fields complete; unlisted fixture fails | gate | `fixture-check` + `tests/gate/{fix-missing-field,fix-unlisted}` | Wave 0 |
| FIX-04 | required inventory fails closed; ledger pins resolve both ways | gate | `inventory-check` + `tests/gate/removed-required` | Wave 0 |
| FIX-05 | no commercial ROM/firmware/disc/save; no leak to logs | gate + unit | `rights-check` + run-time controls; `redaction_synthetic_rom`; `audit-history --range` control | Wave 0 |
| TOOL-02 | one command equals the CI gate | static | `workflow-check` asserts `run: sh scripts/check`; README/AGENTS `grep -F 'sh scripts/check'` | Wave 0 |
| CI-01 | workflow safe, pinned, deterministic; timings recorded | static now, run evidence after push | `workflow-check`; `scripts/check --timing`; `docs/ci-timings.md` row after first run | Wave 0 |

### Sampling Rate
- **Per task commit:** `sh scripts/check` (target well under 60 s; stages whose inputs did not change are still cheap because spawn-dominated).
- **Per wave merge:** `sh scripts/check --selftest`, `dash scripts/check`, the sanitizer preset, `cmake --preset gate-gcc`/`gate-clang` builds where a second compiler exists.
- **Phase gate:** all of the above plus `sh scripts/audit-history` (full history), one Docker `ubuntu:24.04` run (gcc 13 and clang 18, mawk and gawk) labelled one-off, and CI evidence only if the owner authorises a push; otherwise SC5 is "unverified-pending-push".

### Wave 0 Gaps (must exist before other tasks can be verified)
- [ ] `scripts/check` `run_controls` refactor plus golden transcripts of current output and three mutated-tree outputs (first plan, behaviour-preserving, proved by `--selftest` and transcript equality)
- [ ] `tests/conformance/outcomes.def` (7 rows) plus extractor shared by C and sh/awk; `outcome-sync` check
- [ ] `scripts/conformance-flatten` plus its own negative controls (bad TOML yields non-zero exit and no partial TSV; `END` sentinel required)
- [ ] `scripts/conformance-ingest` (separable, file input) with fabricated-results controls
- [ ] `CMakeLists.txt`, `CMakePresets.json` (gate, gate-gcc, gate-clang, sanitizer), runner skeleton with `--list`, `--emit`, RESULT lines and `RUNNER_DONE` marker
- [ ] `src/core/profile_fields.def` plus `ds_profile` and an initial `ds_boot` returning typed refusals, so every refusal row can be asserted
- [ ] Preflight stage (cmake, ninja, cc, shasum or sha256sum, perl, iconv, nm: missing is `harness_failure`, never skip)
- [ ] `tests/lib/gitfix.sh` (factored from `tests/audit/synthetic.sh`) for run-time git controls
- [ ] `tests/controls/{shared-state,wrong-mirror,heuristic-reverse,drop-region}` toy libraries
- [ ] Independent awk oracle script for mirror signatures and the `--emit` hash compare
- [ ] Ledger entries LED-LOAD-0003/0004/0005, a PROF power-on entry; pins moved from `PENDING:2` to `TEST:<id>` only after the cases exist (order matters: `ledger-check` rejects unknown shapes, and `inventory-check` resolves `TEST:` ids both ways)
- [ ] `.gitattributes`, SPDX headers on every new file under `scripts/` including `.allow` files
- [ ] Optional: `scripts/linux-repro` helper for the one-off container run (label "not CI")

## Security Domain

`security_enforcement` is enabled (absent means enabled; ASVS level 1 per config). Content bytes, fixtures and the CI workflow are the attack surface.

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication / V3 Session | no | no accounts or sessions in this phase |
| V4 Access Control | partly | workflow `permissions: contents: read`; branch ruleset is an owner gate (D-64) |
| V5 Input Validation | yes | length-first validation before any read, cap check as `len > cap` on `size_t`, fixed precedence, SIZE_MAX dummy-pointer test under UBSan (pointer-overflow), mem alignment and capacity checks |
| V6 Cryptography | partly | SHA-256 is an identity function, not a security primitive; in-house, verified against NIST vectors and `shasum`; never used for rights decisions |
| V10 Malicious code / supply chain | yes | one pinned Action; no `pull_request_target`, no secrets, no self-hosted runner; Dependabot for the Action; rights check; provenance check |
| V14 Configuration | yes | `workflow-check` fails closed on YAML it cannot parse; `actions/checkout` `persist-credentials: false`; no untrusted `${{ }}` in `run:` |

### Known Threat Patterns
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Oversized or truncated image causes OOB read or huge allocation | DoS / Tampering | structural cap (fixed 1 MiB array), no allocator, length checks before pointer arithmetic, ASan/UBSan matrix run |
| Host mutates buffer after load (TOCTOU) | Tampering | copy-in (D-42) |
| Misaligned or short caller memory | Tampering | alignment and capacity checks (P7) |
| Commercial ROM committed or echoed to logs | Information disclosure / legal | `rights-check`, redaction test with unique markers and a seeded-leak control, `audit-history --range`; residual: add-then-delete still publishes via `refs/pull/N/head` (detection, not prevention) |
| Workflow injection via PR title/branch in `run:` | Tampering / EoP | pass values through `env:`, `workflow-check` bans untrusted expressions in `run:` |
| Compromised Action | Tampering | full-SHA pin, `permissions: contents: read`, monthly Dependabot |
| Prompt injection in fetched web pages | Tampering (of the research process) | observed in nicole.express page; treat fetched text as data |

## Sources

### Primary (verified in this session)
- Repo files read: `scripts/check`, `scripts/lib/toml-subset.awk`, `scripts/audit-history`, `scripts/licence-check`, `scripts/ledger-check`, `profiles/pce-hucard-2btn/r1.toml`, `ledger/SCHEMA.md`, `ledger/entries/LED-LOAD-0001.toml`, `0002.toml`, `docs/decisions/0006-ci-provider.md`, `0007`, `.planning/phases/01-.../01-LINUX-AWK-EVIDENCE.md`, `.planning/ROADMAP.md` Phase 2, `.planning/REQUIREMENTS.md`
- Spike tag: `git show spike-tool-01-evidence:spikes/tool-01/{c/CMakeLists.txt,c/src/core.c,c/src/runner.c,gates.sh,gen-fixtures.sh,fixtures.manifest}`
- MAME `pce_slot.cpp`: `https://raw.githubusercontent.com/mamedev/mame/master/src/devices/bus/pce/pce_slot.cpp` (lines 66-127, 200-310)
- Geargrafx `media.cpp`: `https://raw.githubusercontent.com/drhelius/Geargrafx/main/src/media.cpp` (lines 385-414, 1188-1273)
- Beetle PCE `huc.cpp`: `https://raw.githubusercontent.com/libretro/beetle-pce-libretro/master/mednafen/pce/huc.cpp` (lines 190-330)
- PCEdev wiki: `https://pce.nesdev.org/w/index.php?title=CPU_memory_map&action=raw`, `https://pce.nesdev.org/wiki/HuCard_pinout`
- `https://github.com/actions/runner-images` `images/ubuntu/Ubuntu2404-Readme.md` (image 20261101.360.1)
- `git ls-remote https://github.com/actions/checkout` (tag v7.0.1)
- Vigna `https://prng.di.unimi.it/splitmix64.c`; rand_xoshiro `https://raw.githubusercontent.com/rust-random/rngs/master/rand_xoshiro/src/splitmix64.rs`
- Local runs: splitmix64, SHA-256 (NIST vectors, 21-length sweep vs `shasum`, bit-length mutant), freestanding compile, `nm`, `-Wconversion`, UBSan shift and alignment, ASan stack, CMake presets, actionlint, iconv, allocation shim, `dash scripts/check`; Ubuntu 24.04 container runs (gcc 13.3.0, clang 18.1.3, cmake 3.28.3, ninja 1.11.1, mawk 1.3.4, gawk 5.2.1)

### Secondary / not reproducible
- Searches returned but pages did not state the claims: Wikipedia HuCard, `dumping.guide/carts/nec`, nicole.express (contains an injection attempt). Unreachable: pcetech.txt mirrors, Archaic Pixels, necretro.org, consolemods.org, git.redump.net.

## Metadata

**Confidence breakdown:**
- Standard stack and toolchain behaviour: HIGH, run on both platforms
- Validation architecture: HIGH for structure; the specific case ids are PROPOSAL (names are discretionary)
- HuCard mirroring: MEDIUM, three independent codebases agree for 384K; no primary document
- HuCard byte order and header: MEDIUM-LOW, only wiring evidence and divergent emulator heuristics
- CI: MEDIUM, workflow lints clean but no run exists yet (D-64)

**Research date:** 2026-11-09
**Valid until:** 2026-12-09 for tool versions and the runner image (fast-moving); the HuCard section is stable until a primary source appears.
