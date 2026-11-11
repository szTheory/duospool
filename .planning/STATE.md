---
gsd_state_version: "1.0"
current_phase: 3
current_phase_name: HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad
current_plan: 6
status: planning
stopped_at: Completed 03-36-PLAN.md
last_updated: "2026-11-11T06:44:59.772Z"
last_activity: 2026-11-11
last_activity_desc: Phase 2 complete, transitioned to Phase 3
state_head: f84ceda77f7d87f144043e92c93fedb93bef778d
progress:
  total_phases: 6
  completed_phases: 2
  total_plans: 72
  completed_plans: 39
  percent: 33
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-11-09)

**Core value:** For every hardware/content profile DuoSpool claims to support, a fresh, deterministic, headless run can reproduce the machine's behavior and show the evidence (guest progress, machine time, raw video, raw audio) that backs the claim.
**Current focus:** Phase 3 — HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad

## Current Position

Phase: 3 — HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad
Current Plan: 6
Total Plans in Phase: 38
Status: Ready to execute
Last activity: 2026-11-11 — Completed 03-36 (W1: placeholder state out, master-clocks record, epoch 2, goldens regenerated)

Progress: [███░░░░░░░] 33%

## Performance Metrics

**Velocity:**
- Total plans completed: 34
- Average duration: -
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 1 | 12 | - | - |
| 2 | 22 | - | - |

**Recent Trend:**
- Last 5 plans: -
- Trend: -

*Updated after each plan completion*
**Per-Plan Metrics:**

| Plan | Duration | Tasks | Files |
|------|----------|-------|-------|
| Phase 1 P01 | 60min | 3 tasks | 67 files |
| Phase 1 P02 | 45min | 3 tasks | 56 files |
| Phase 1 P03 | 15 min | 3 tasks | 60 files |
| Phase 1 P04 | 25min | 3 tasks | 87 files |
| Phase 1 P05 | 30min | 2 tasks | 39 files |
| Phase 1 P06 | 10 min | 2 tasks | 13 files |
| Phase 1 P07 | 17 min | 2 tasks | 6 files |
| Phase 1 P08 | 25 min | 2 tasks | 14 files |
| Phase 1 P09 | 20 min | 2 tasks | 9 files |
| Phase 1 P10 | 13 min | 3 tasks | 40 files |
| Phase 1 P11 | 15 min | 2 tasks | 5 files |
| Phase 1 P12 | n/a | 3 tasks | 5 files |
| Phase 02 P01 | 22 min | 3 tasks | 2 files |
| Phase 02 P02 | 5 min | 3 tasks | 88 files |
| Phase 02 P03 | 9 min | 3 tasks | 19 files |
| Phase 02 P04 | 23 min | 3 tasks | 25 files |
| Phase 02 P05 | 14 min | 3 tasks | 20 files |
| Phase 02 P06 | 11 min | 3 tasks | 20 files |
| Phase 02 P07 | 10 min | 2 tasks | 20 files |
| Phase 02 P08 | 8 min | 3 tasks | 18 files |
| Phase 02 P09 | 9 min | 2 tasks | 11 files |
| Phase 02 P10 | 13 min | 3 tasks | 50 files |
| Phase 02 P11 | 18 min | 6 tasks | 62 files |
| Phase 02 P12 | 25 min | 2 tasks | 26 files |
| Phase 02 P13 | 14 min | 2 tasks | 21 files |
| Phase 02 P14 | 75 min | 3 tasks | 30 files |
| Phase 02 P15 | 22 min | 3 tasks | 25 files |
| Phase 02 P16 | 60 min | 3 tasks | 20 files |
| Phase 02 P17 | 25 min | 4 tasks | 74 files |
| Phase 02 P18 | 15 min | 3 tasks | 53 files |
| Phase 02 P19 | 25 min | 2 tasks | 15 files |
| Phase 02 P20 | 70min | 3 tasks | 49 files |
| Phase 02 P21 | 80 min | 3 tasks | 27 files |
| Phase 02 P22 | 35 min | 2 tasks | 7 files |
| Phase 03 P01 | 25min | 3 tasks | 15 files |
| Phase 03 P02 | 35min | 2 tasks | 17 files |
| Phase 3 P35 | 25min | 2 tasks | 14 files |
| Phase 3 P03 | 25min | 2 tasks | 13 files |
| Phase 3 P36 | 20min | 1 tasks | 16 files |

## Accumulated Context

### Decisions

Decisions are logged in the PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Init]: First vertical slice (base profile + HuCard + authored fixtures + headless runner + installed-consumer proof) is a candidate. The owner has not selected it.
- [Roadmap]: 6 phases (granularity standard). Loader folded into the deterministic-shell phase; both VDC slices merged into one video phase; Phases 4 and 5 can run in parallel.
- [Roadmap]: E4 (emulator differential) is triage only and never backs a correctness claim. This is a proposed default; owner confirmation is pending (decision 17).
- [Phase 1]: Gate checkers print a marker line on a clean exit and scripts/check fails closed on a silent exit 0 (bash 3.2 /bin/sh exits 0 on script syntax errors)
- [Phase 1]: 01-02: open profile fields carry no value; proposals labelled and checked against allowed (OPEN_HAS_VALUE control)
- [Phase 1]: 01-02: ledger IDs, decision paths and profile values are shape-validated before use as file names
- [Phase 1]: Records 0003-0007 are owner-recorded using only verbatim CONTEXT.md quotes; 0007 marks the D-26 mechanism as Claude's design
- [Phase 1]: decision-check scans every DIR/*.md except README.md and *.protocol.md and enforces index drift, record naming, duplicate numbers and the 0005 no-GPL sentence
- [Phase 1]: 01-04: provenance-check scans the whole tree with exact-path exclusions (SELF-FIXTURES); a malformed-id entry skips the per-path marker check
- [Phase 1]: 01-05: PROTOCOL_EDITED is an append-only byte-prefix rule (a blank separator before an amendment heading must not read as an edit)
- [Phase 1]: 01-05: a gate recorded as ND is not PASS; a leader with an ND gate is leader-fails-gate (D-37)
- [Phase 1]: 01-06: opt_install builds RelWithDebInfo for both the remapped and seeded-leak runs so the path-leak grep inspects an archive that carries DWARF paths; prefix remap covers both /var and /private/var spellings
- [Phase 1]: 01-07: C fuzz run sets ASAN_OPTIONS=handle_sigtrap=1 so the arm64 SIGTRAP from the planted __builtin_trap is reported and a crash artifact is written; runtime setting only, protocol unchanged
- [Phase 1]: 01-08: Rust build trees live under out/build-rust-<profile> (not rust/target) and the installed release build carries debug info so the path-leak grep is non-vacuous; Rust core allocates the content buffer with try_reserve_exact (ERR_STATE) while Box<Handle> in create can abort
- [Phase 1]: 01-09: planted-bug prefix compared as one little-endian integer because bytes.starts_with was not found by cargo-fuzz in 60 s under macOS ASan (found in 0.4 s after); fuzz and Miri trees under out/
- [Phase 1]: 01-10: run-all.sh timing parser takes the last occurrence of each key; with hyperfine 2.0.0 that is the time_wall_clock block, verified for all 24 JSON files by an independent cross-check (0 mismatches); script left as run
- [Phase 1]: 01-10: M09 line counts follow LINES.map as pre-registered, so the Rust ffi-glue count (231) includes the capi Miri test module; C4 inputs are 198 (C) and 476 (Rust); for the 01-11 scorer to weigh
- [Phase 1]: 01-11: Record 0001 (language/build) is proposed-default: score.awk RESULT=proposed-default REASON=margin-below-10 (Rust 80.20 vs C 79.00); Phase 2 blocked pending an owner choice; leader Rust is provisional and flips under two plausible rubric readings
- [Phase 1]: 01-12: Owner chose rewrite-history before publication; repo szTheory/duospool published public (main 8ba0bc6 plus tag spike-tool-01-evidence), secret scanning on
- [Phase 02]: run_controls helper is the single control loop for ledger/decision/profile/provenance/licence/results negatives; stage and mutation selectable via --stage NAME and --selftest NAME
- [Phase 02]: Gate refactors are proven by tests/check/equiv.sh transcript equivalence against a git archive of the base commit plus a selftest mutation; the harness is a one-shot proof and will DIFF once later plans change gate stdout on purpose
- [Phase 02]: 02-02: conformance-ingest judges against the repository's outcomes.def (not the mini-root's); a malformed REQUIRED id is reported as OUTCOME_MISSING_ARTIFACT — Mini-root controls supply REQUIRED and cases but must use the one real outcome set; no extra code needed
- [Phase 02]: 02-02: conformance-flatten prints violations and its STATUS marker on stderr; stdout is TSV only (empty on any error) — Keeps scripts/check checker() marker rule intact without a validate-only mode and guarantees no partial TSV reaches the runner
- [Phase 02]: 02-03: conformance-run maps a CMD status above 2 to HARNESS_CMD_STATUS and exit 2; wall expiry is status 142 and is never a cancelled line — keeps the 0/1/2 exit contract and the watchdog distinct from cancelled (D-57, D-77)
- [Phase 02]: 02-03: outcome-sync codes partition faults minimally: ROWS (count), NAME (set), ORDER only when both are clear — each mini-root control then has an exact, small expected code set and a mutation flips exactly one control
- [Phase 02]: 02-04: rights-check applies every rule to every file except SHAPE (non-text only, D-76); BLOB is an in-line run; archive magics are real headers; a NUL file is never excused by the manifest allowlist — Bulk text classification uses perl because BSD grep ends a line at a NUL; an excuse must not cover a file with a NUL byte (T-02-16)
- [Phase 02]: 02-04: full-history audit-history left red on two pre-existing home-path hits in the 02-01 and 02-02 summary history; not allowlisted, owner decides before publication — An allowlist entry would silently hide a finding that is an owner decision; PR range mode is clean; recorded in WINDOWS.md
- [Phase 02]: 02-05: flags reach the build through two preset cache variables (DS_WARNING_FLAGS in the hidden base, DS_EXTRA_FLAGS per preset) — a child preset that sets CMAKE_C_FLAGS replaces the parent's value, so the gate preset could not add -fwrapv to the base; later presets add DS_EXTRA_FLAGS
- [Phase 02]: 02-05: a load.NAME expectation that no refusal carries is a guest_failure (wrong literal), only a token that is neither accept nor load.* is a harness fault; field= in an expectation is asserted only when present
- [Phase 02]: 02-05: resolve runs once per boot and the profile, rev and bindings rows surface their own codes; every boot_order row guards its own request use so a later plan can swap rows and get a wrong refusal, never a NULL read
- [Phase 02]: 02-06: the r1 constant carries unresolved_behaviours through ds_profile_unresolved(_count) in profile.c, not through struct ds_profile; plan 13 hashes the ids through the same accessor
- [Phase 02]: 02-06: ds_text prints with no library call; a quote, backslash, non-ASCII byte or unterminated string is DS_TEXT_ERR_STRING and an empty text (D-69); ds_text_profile prints the identity tables and is an exact part of the record
- [Phase 02]: 02-06: record skeleton row count is pinned at 50 in runner.c build_expected_keys; plans 07, 12 and 14 that add keys or tables must edit that list in the same commit
- [Phase 02]: 02-06: gate-gcc resolves to Apple clang on this Mac and is not a real gcc check; real gcc runs in plan 21 and CI. Stage outcome-sync builds the gate preset itself before reading build/gate/ds_runner
- [Phase 02]: 02-07: content id is the in-house SHA-256 of the copied ROM bytes, taken as the last step of ds_boot; independent check is a gate sweep of 21 lengths against the host sha tool (HASH_SWEEP_MISMATCH)
- [Phase 02]: 02-07: pinned record sha256 literal uses the len:N recipe, not plain-256k, because plan 08 adds bank tag bytes to plain-256k; profile case kinds moved to tests/conformance/profkinds.c behind runner.h to keep runner.c under 64 KiB
- [Phase 02]: 02-08: ds_instance keeps size_class (the size enum), never the raw length; content.size in the record is derived from it, so the record is unchanged
- [Phase 02]: 02-08: ds_mapper_read checks the bank before the offset: bank 0x80 and above is DS_MAP_UNMAPPED whatever the offset; the 384K mirror expression is one line, src/core/mapper.c line 26 (anchor for the plan 09 wrong-mirror RULE)
- [Phase 02]: 02-08: the oracle rule-text guard requires each ledger clause literally and terminated by one of ; . : or the closing quote, exit 1 with ORACLE_RULE_TEXT_DRIFT
- [Phase 02]: [02-09] Toy controls are generated from the live source by a one-line rule-tagged substitution (scripts/control-mutate); anchor count other than one, an empty or absent EXPECT, or a toy that does not compile is a harness error (exit 2), a toy that passes is NOT_RED (status 1)
- [Phase 02]: [02-09] LED-LOAD-0004 stays open: 512K, 768K and 192K are refused size_unsupported, a 2 MiB image is refused too_large by the cap; ledger/REQUIRED keeps sorted order
- [Phase 02]: Sanitizer exclusion has a single source: EXCLUDED_IN_MODE feeds the runner --exclude-ids, the supervisor ids file and the ingest forgiveness, and the ingest asserts the file both ways (10) — an exclusion that cannot drift from what ran; removing a row makes the case run and fail, an unlisted exclusion is OUTCOME_MISSING_ARTIFACT
- [Phase 02]: alloc_zero kind lives in allockinds.c, not runner.c; runner.c is 64027 of 65536 bytes (plan 11 must split a kind file out before adding a deferred kind) (10) — rights-tree 65536-byte limit; precedent profkinds.c
- [Phase 02]: Plan 11: precedence table is read from the request key then the expect key because a case line is limited to 400 characters; it needs at least one item per adjacent pair of boot_order.def rows
- [Phase 02]: Plan 11: reason coverage is a deferred kind judged against the refusal enum itself (no hard-coded count), so adding a refusal needs only a refuse row
- [Phase 02]: Plan 11: every runner line goes through emit_out/emit_err and an in-process log read by the redaction scan; redaction markers come only from --marker-seed and a missing seed is a harness failure
- [Phase 02]: Plan 11: runner.c split, banksig and unmapped moved to mapkinds.c; new kinds in refkinds.c and redkinds.c (runner.c 59967 bytes, limit 65536)
- [Phase 02]: 02-12: seed is a u64 plus a has_seed flag; seed 0 is a valid seed, distinct from no seed
- [Phase 02]: 02-12: ds_advance and ds_run use their own status enums so the refusal enum stays the 22 boot-time reasons
- [Phase 02]: 02-12: ds_run checks stop before the first step and after every step; machine time never passes start plus budget
- [Phase 02]: 02-13: digest stream = prefix duospool-state+NUL, version 1, eight tagged regions (PROF CONT POWR TIME RAM8 MPRS PSGN VDCP), little-endian, ROM excluded (content id stands for it); profile hash strings are u32 length plus bytes bounded by the array; digest_region flips first and last live byte per region; nonmutation_digest aims refuse rows at live instance memory
- [Phase 02]: 02-14: the record prints the seed and machine time as unsigned decimal u64; seed_present is a TOML boolean, policy_version an integer (DS_POWERON_POLICY_VERSION); record pinned at 61 rows in runner.c
- [Phase 02]: 02-14: a failed print wipes the buffer it wrote; the refusal record prints the requested id and a hostile id (quote, backslash, non-ASCII, 64+ chars) gives DS_TEXT_ERR_STRING and no record
- [Phase 02]: 02-14: runner --records DIR writes DIR/ID.record.toml for kinds accept, record_lines, record_golden, record_refusal (conformance-run makes build/NAME/records empty each run); --golden-dir DIR; record-lint stage derives the expected files from the kind column of cases.tsv
- [Phase 02]: 02-15: core-boundary undefined-symbol rule allows the allowlist plus external symbols defined by the core objects themselves (cross-module ds_* calls), never a widened allowlist file
- [Phase 02]: 02-16: the shared-state toy folds the full hidden counter value (^ ++shared_n) into the psg_noise update; a parity-only fold would not move the digest on even-length calls
- [Phase 02]: 02-16: DET_MISSING_FILE is exit 2 and asserted directly in the determinism-negatives stage; tests/gate/determinism holds differs and ok-same
- [Phase 02]: 02-17: a len=N override over a plain-* fixture is the recipe plain-*:N (first N bytes of the tagged image), not len:N — The bank tags of the natural size are part of the bytes a case emits, so a hash of the pure stream would not be what the case generates; ds_runner --fixtures names recipes with synth_name through the same parse_spec and synth_size the cases use, and a real run emits exactly the listed set
- [Phase 02]: 02-17: the fixtures stage makes its own used list (conformance-run fixtures, no --only) and asserts it names every manifest file; --partial only for --only runs — A stale or partial list must never be judged; FIX_STALE and the count-and-name comparison cover an entry nothing emits (D-85); runner.c is at 65503 of 65536 bytes, so new runner logic goes in its own file
- [Phase 02]: 02-18: live-runner controls are judged on the id and outcome fields of RESULT lines (awk, values through ENVIRON), never a whole line; a flagged timeout loses to a failed assertion — the RESULT line also carries the assertion count and the evidence path, so a whole-line match is brittle and a wrong literal could hide behind it (D-80, T-02-36)
- [Phase 02]: 02-18: inventory-check enforces REQUIRED and the case files in both directions and the ledger TEST pins in both directions; ledger-check stays structure-only — a green gate that silently ran fewer tests is worse than a red one (D-58, T-02-35)
- [Phase 02]: 02-19: workflow-check is a restricted block-style YAML line checker; WF_UNPARSABLE exits 2 and cannot be named by an --expect, and the ok-gate control must stay byte-equal to the real gate.yml (CONTROL_DRIFT)
- [Phase 02]: 02-19: the Linux container run of the whole gate found record-lint failing under mawk (lvalue-first evaluation, scripts/lib/record-rules.awk line 45); logged in deferred-items.md D-19-1, not fixed in plan 19
- [Phase 02]: 02-20: a secrets expression reports WF_FORBIDDEN_KEY only; wrong-concurrency added as a twelfth workflow control; ci-timings wall = latest job end minus run start
- [Phase 02]: 02-21: linux-repro parts differ on purpose (default-awk mawk+gcc, mawk+clang, gawk+gcc) so no full gate repeats another; the container run is a local one-off, not CI evidence
- [Phase 02]: 02-21: ds_runner leak (exclusion file buffer) found only by clang LeakSanitizer on Linux; fixed at the cause. CI-01 un-ticked: criterion 5 unverified-pending-push
- [Phase 02]: 02-22: checkpoint answer authorised-branch-pr; one branch and one PR pushed, first CI run 42939483070 green (100 s, default awk gawk, second mawk); remote settings stay pending hand-offs
- [Phase 02]: 02-22: audit-history gained a kind-scoped allowance (TEXT-PATTERN:KIND); the two home-path-mac summary hits are allowlisted by kind with the owner's reason
- [Phase 03]: 03-01: DSV1 outcome mapping reading of D-124: harness-side preconditions give harness_failure, guest-written faults give guest_failure, UNTIL without done gives timeout (a visible guest fault wins over timeout)
- [Phase 03]: 03-01: ds_cpu_reset is called from ds_boot so a booted instance is always in reset state; ds_cpu_step returns mclk and ds_run adds them in one statement
- [Phase 03]: 03-02: D-68 and D-75 superseded by D-91; the placeholder stepping API is deleted and every stepping kind runs ds_run to absolute deadlines
- [Phase 03]: 03-02: ds_run overshoot is 0 for a call that took no step; run_until items assert overshoot = time - until instead of a separate literal
- [Phase 3]: 03-35: ds_run refusal codes live in run_refusals.def (run.NAME, 16 slots), never refusals.def; sink and deadline renamed sink_unsupported and time_wrap, numbers 1..6 unchanged
- [Phase 3]: 03-03: CPU0 stream order is a x y s p pc speed (memory order differs); accessor span is pc..speed
- [Phase 3]: 03-36: poweron_domain keeps its kind with a no-domain stand-in stream until the v2 domains arrive in 03-24

### Pending Todos

None yet.

### Blockers/Concerns

- [Phase 1]: Owner decisions recorded 2026-11-09: language/build (1) = C11 + CMake + Ninja (owner chose against the Rust nominal leader), first-profile scope (2), API/ABI posture (6), licence/visibility (8), fixture rights (9), CI provider (14). Phase 2 is unblocked.
- [Phase 3]: No fixture can be admitted until the licence (8) and fixture-rights (9) decisions are recorded.
- [Phases 4-5]: Hardware timing conflicts (C1 PSG clock, C2 line count, C3 IRQ position, C4 dot-clock encoding, C5 active width) stay modelling assumptions. Settling them needs a primary source or an owner-approved capture (10).
- [Phase 6]: LibFuturo evaluation needs owner-supplied or owner-authorized access to the current contract (7). Package format needs decision 12.
- RESOLVED 2026-11-09: Phase 2 language/build gate cleared. Record 0001 is owner-recorded: C11 + CMake + Ninja (owner chose it after the scorer returned proposed-default, Rust 80.20 vs C 79.00)

## Deferred Items

Items acknowledged and deferred at milestone close, most recent first:

| Category | Item | Status | Deferred At | Milestone |
|----------|------|--------|-------------|-----------|
| *(none)* | | | | |

## Session Continuity

Last session: 2026-11-11T06:44:59.618Z
Stopped at: Completed 03-36-PLAN.md
Resume file: None
