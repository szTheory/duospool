---
phase: 02-deterministic-machine-shell-hucard-boundary-and-fail-closed
verified: 2026-11-10T00:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files: [".github/workflows/gate.yml", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-01-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-02-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-02-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-03-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-03-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-04-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-04-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-05-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-05-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-06-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-06-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-07-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-07-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-08-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-08-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-09-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-09-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-10-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-10-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-11-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-11-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-12-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-12-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-13-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-13-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-14-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-14-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-15-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-15-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-16-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-16-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-17-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-17-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-18-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-18-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-19-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-19-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-20-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-20-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-21-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-21-SUMMARY.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-22-PLAN.md", ".planning/phases/02-deterministic-machine-shell-hucard-boundary-and-fail-closed/02-22-SUMMARY.md", "CMakeLists.txt", "README.md", "docs/ci-timings.md", "scripts/audit-history", "scripts/check", "scripts/core-boundary-check", "scripts/outcome-sync", "scripts/rights-check", "src/core/boot.c", "src/core/ds_core.h", "src/core/mapper.c", "src/core/poweron.c", "src/core/profile.c", "src/core/serial.c", "tests/conformance/REQUIRED"]
covered_digest: "v4:sha256:28094302393af74ea516ff2c3eb2ae75c14a787bf8600b3d745cca322b43bbb0"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 2: Deterministic Machine Shell, HuCard Boundary and Fail-Closed Gate Verification Report

**Phase Goal:** A host passes DuoSpool content bytes and a named profile. It gets back either a typed refusal or a fresh, isolated, reproducibly initialised machine instance whose identity is recorded. One local command, mirrored in CI, fails closed when any required check is missing or skipped, has no evidence, or violates the rights boundary.
**Verified:** 2026-11-10
**Status:** passed
**Re-verification:** No, initial verification
**Mode:** mvp phase (goal is a capability statement, not a user story; verified against the roadmap goal and success criteria directly)
**Evidence tier of everything below: E0 (repeatability and literal checks). No claim of hardware accuracy or compatibility is made or allowed.**

## Verification Environment, Stated Plainly

- Run on one macOS machine (Darwin 25.6.0) with Apple clang and the one-true-awk (`awk version 20200816`). `gawk` and `mawk` are NOT installed here.
- Real gcc 13, clang 18, mawk and gawk evidence exists only from (a) the one-off container run `scripts/linux-repro` (recorded in plan 02-21) and (b) the CI run on the earlier head. This verification did not reproduce either; it relies on the recorded records plus `gh run view`, which I did read (see Truth 5).
- CI is on head `3958316` (PR 1, run 42940308647, `gate` job `success`, read with `gh run view`, read only). The review-fix commits `ace9ed5`..`0ae9c89` (CR-01, WR-01..WR-04 and the fix report) are local only, not pushed (decision D-64, no push without owner authorisation). **The final head `0ae9c89` has had no CI run and awaits an owner-authorised push.** Nothing was pushed by this verification.
- Working tree at verification: clean except the pre-existing untracked `.planning/state.json`.

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Refusal matrix in the fast gate: empty, truncated, header-prefixed, 513-byte, oversized, non-multiple-of-8 KiB, undeclared byte order or header, unsupported special mappers, mismatched profile; each its own typed reason; no panic or over-cap allocation; digest of an existing instance unchanged by a refusal | VERIFIED | `src/core/refusals.def` has 22 closed `DS_REFUSAL` rows (`empty`, `too_short`, `too_large`, `size_not_bank_multiple`, `size_unsupported`, `byte_order_undeclared`, `header_undeclared`, `header_unsupported`, `mapper_unsupported`, `profile_mismatch` and others). `boot_order.def` fixes precedence (15 checks, cap before supported size, D-82). Case files exist for each named input: `ref_empty_null`, `ref_short_513`, `ref_header_copier512`, `ref_header_undeclared`, `ref_order_undeclared`, `ref_order_tg16`, `ref_not_bank_*`, `ref_size_128k..768k`, `ref_too_large_*`, `ref_mapper_sf2/populous/tennokoe/arcade_card/mcgenjin`, `ref_profile_mismatch`. `nonmutation_matrix` / `nonmutation_digest` prove the instance digest is unchanged by refusals; `alloc_zero_matrix` and `alloc_shim_selfcheck` prove no allocation past the cap. I ran `sh scripts/check`: all 104 `RESULT` lines in `build/gate/results.txt` are `pass`; stages `conformance`, `sanitizers`, `excluded-mode-controls` PASS. |
| 2 | Each supported HuCard size reads its expected bank signature through every documented mirror via the mapper boundary; the core never opens a file; the run record has content hash, size, declared format, byte order, header handling | VERIFIED | `accept_256k_banksig`, `accept_384k_banksig`, `accept_1m_banksig` (256 assertions each) judged against an independent awk oracle (`tests/conformance/mirror-oracle.awk`), plus `accept_unmapped_banks` (393). `ds_boot` takes `(mem, cap, request, result)` with host-supplied content; core-boundary stage forbids file APIs. `record_content_fields` and `build/gate/records/record_golden_384k_seeded.record.toml` show the record fields. The mirror rule is ledger entry LED-LOAD-0003 with `status = "assumed"` and `evidence = []`, labelled a modelling assumption, not a hardware fact (honest E0). |
| 3 | Run record contains full profile identity and power-on policy and seed; two interleaved instances equal each instance alone; same inputs in a fresh process give identical records; build-time check rejects core use of clocks, filesystem, threads, environment, devices, UI, with a negative control shown to fail | VERIFIED | Golden record (read): profile id, rev, sha256 and one `[field_*]` table per profile field with `status`, `value`, `bound_by`. `interleave_ab`, `interleave_aba_order`, `interleave_twins`, `interleave_canaries`, `interleave_destroy_recreate` pass; `poweron_*` cases cover seeded policy. Stages `determinism` (two fresh processes, all records compared) and `determinism-negatives` PASS. `scripts/core-boundary-check` runs four layers (includes, compile with `-nostdinc`, undefined and writable symbol lists, float scan); `core-boundary-negatives` PASS with 20 controls (`time-include`, `clock-call`, `env-call`, `alloc-call`, `global-counter`, `float-*` and others) each required red. |
| 4 | One documented local command; seven distinct outcomes, only pass green; each named negative control fails the gate (removed required test, missing manifest field, unlisted fixture, wrong literal, unlisted ROM-like binary) | VERIFIED | README documents `sh scripts/check`. `tests/conformance/outcomes.def` has 7 rows (pass, guest_failure, timeout, harness_failure, skip, cancelled, missing_artifact), kept in step with the compiled list by `outcome-sync`. `build/gate/named-controls.txt` (written by my gate run): `removed-required`, `missing-field`, `unlisted-fixture`, `wrong-literal`, `romlike-binary`, `timeout-budget`, `cancelled-spin`, all `RED_OK`. I also reproduced two independently in a scratch clone of `0ae9c89`: a planted 256 KiB `planted.pce` made `--stage rights-tree` FAIL with `ROMLIKE_BINARY`; deleting `ref_empty_null` from REQUIRED made `--stage inventory` FAIL with `INV_UNLISTED_CASE`. |
| 5 | CI on the owner-selected provider runs the same fast gate on every change; each job tied to a named failure class; critical-path time recorded; blocked if provider undecided | VERIFIED (with a labelled limit) | Provider decided (GitHub Actions, record 0006). `.github/workflows/gate.yml` runs `sh scripts/check` on `pull_request`, `push` to main and `workflow_dispatch`; header names four failure classes; `contents: read`, SHA-pinned checkout, `persist-credentials: false`, base SHA through `env`. `gh run view 42940308647`: head `3958316`, conclusion success, job `gate` success. `docs/ci-timings.md` records first run 42939483070 (head `b063518`), critical path 100 s, queue 0 s, per-step seconds, an efficiency review and the four failure classes. **Limit:** the run is on the earlier head; the six local review-fix commits have not been through CI. Run 42940308647 is not yet a row in the timing table (info only). |

**Score:** 5/5 truths verified, 0 present-but-behavior-unverified.

### Review-fix verification (read from the tree, not the report)

| Finding | In tree? | Evidence |
|---------|----------|----------|
| CR-01 awk status hidden by `\| sort` | Yes | `scripts/outcome-sync:81,94` write `viol.raw` / `viol.names` and exit 2 `HARNESS_AWK_FAILED` on awk failure. `core-boundary-check` allowlist, undefined and writable symbol awks each write a file with `|| harness`. `rights-check` `sha256_of` no longer pipes and the caller exits 2 `HARNESS_SHA256_FAILED` (line 244). `audit-history` hash pairs, tree paths and hashes carry status tests. `sh scripts/check --selftest` printed all 9 `SELFTEST awk-exit-status` cases PASS and `SELFTEST RESULT PASS` (38 SELFTEST PASS lines, 39 s). Residual (not a gap): `core-boundary-check:230` is `nm \| awk >> file \|\| harness`, so an `nm` failure there is not seen, but it only shrinks the allowed set, which fails closed (more undefined symbols reported). `rights-check:199` has `tr \| awk` on the NUL-file path (low risk). |
| WR-01 rights-check hash binding | Yes | `rights-check:231` refuses `-*` sources; `:241` requires 64 lower-case hex else `RIGHTS_BAD_ALLOW`; `./$1` used in `sha256_of` and line 197. Stage `rights-negatives` PASS (includes the four new cases). |
| WR-02 ds_boot overlap | Yes | `src/core/boot.c` `ranges_overlap` and `memory_overlaps` run after the last `boot_order.def` row and refuse `load.memory_invalid` before the memset. Cases `ref_mem_overlap_content/_request/_result/_edge` exist; all 104 REQUIRED ids pass (REQUIRED has 104 case files). Core-boundary and sanitizer stages PASS. Placement as a post-table check (not a `boot_order.def` row) is a recorded planner/owner choice, not a defect. |
| WR-03 CB_FLOAT | Yes | `scripts/core-boundary-check:153` matches `_FloatN`, `_DecimalN`, `__fp16`, `__bf16`, `__float128`, `_Complex`, `__complex__`; a numeric-literal scanner was added. Six `float-literal-*` / `float-type-*` negatives plus positive `ok-int-literals` exist under `tests/gate/core-boundary/`; stage PASS. |
| WR-04 audit-history | Yes | `scripts/audit-history` has a blob-coverage check (lines ~163-170) and a prefix-by-length parse. `sh scripts/audit-history` over full history: `scanned 207 commits, 1374 blobs, 1579 blob-path pairs`, `audit-history: clean`. |

Info findings IN-01..IN-03 were out of scope for the fixer and remain open in 02-REVIEW.md. None contradicts a success criterion (each is a hardening or consistency item whose failure mode the gate already catches by a second route).

### Gate runs by me

| Command | Result |
|---------|--------|
| `sh scripts/check` | `CHECK RESULT PASS`, exit 0, 47 stages, no FAIL, ERROR or SKIP lines |
| `sh scripts/check --selftest` | `SELFTEST RESULT PASS`, exit 0, 38 SELFTEST PASS lines (39 s), including 9 `awk-exit-status` cases |
| `dash scripts/check` | `CHECK RESULT PASS` (extra evidence for POSIX sh portability on this Mac) |
| `sh scripts/audit-history` | `clean` over 207 commits (178 s) |
| Scratch-clone negatives (planted ROM-like file, removed REQUIRED id) | Both turned the relevant stage FAIL |

All local runs used Apple clang and BWK awk only. They are E0 repeatability evidence.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/core/boot.c`, `mapper.c`, `profile.c`, `poweron.c`, `serial.c`, `sha256.c`, `advance.c` | The deterministic machine shell | VERIFIED | Substantive (2129 lines across core and text), wired into `ds_runner` and conformance; boot.c read, ordered check table plus overlap check plus zero-then-fill boot. |
| `src/core/refusals.def`, `boot_order.def`, `profile_fields.def` | Closed refusal set, precedence, profile fields as data | VERIFIED | Read; each has a gate check that the compiled lists match. |
| `profiles/pce-hucard-2btn/r1.toml`, `ledger/entries/LED-LOAD-0003..0005`, `LED-PROF-0002` | Profile identity and recorded policy | VERIFIED | Present; LED-LOAD-0003 is `assumed`, no evidence claimed. |
| `tests/conformance/` (104 cases, REQUIRED, outcomes.def, oracle awk) | Required-test inventory | VERIFIED | 104 REQUIRED ids equal 104 case files; all `pass` in the live run. |
| `fixtures/manifest/*.toml` (43) | Manifest per fixture, no committed binaries | VERIFIED | Entries carry source, recipe, toolchain, size, sha256, licence, notices, redistribution (`generated-not-committed`), meaning, verdict source. `git ls-files` shows no `.pce/.sgx/.bin/.iso/.cue/.chd/.sav`. |
| `scripts/check` and 20+ checker scripts | One gate command, fail closed | VERIFIED | 47 stages; negative controls per stage. |
| `.github/workflows/gate.yml`, `docs/ci-timings.md` | CI mirror and timing record | VERIFIED | See Truth 5. |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `scripts/check` | every checker and control | `run_stage` / `run_controls` | WIRED (stage list prints all 47) |
| `tests/conformance/outcomes.def` | runner and ingest | `outcome-sync` stage vs `ds_runner --outcomes` | WIRED |
| `boot_order.def` | `boot.c` `boot_order[]` | X-macro include | WIRED |
| REQUIRED | cases, ingest and `inventory-check` | inventory stage and ingest | WIRED (removing an id reds the gate) |
| `gate.yml` | `scripts/check` | `run: sh scripts/check` | WIRED, verified by the green CI run on `3958316` |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
|----------|------|--------|-----------|--------|
| Run record | profile fields, content id, seed, digest | `ds_boot` result, `ds_sha256`, `ds_state_serialize` of the live instance | Yes: compared against host `shasum` / `sha256sum` and NIST vectors; two fresh processes compared | FLOWING |

### Requirements Coverage

All 15 phase requirement IDs appear in the `requirements:` frontmatter of at least one plan, and all are marked Complete in REQUIREMENTS.md. No ORPHANED requirement (REQUIREMENTS.md maps no additional ID to Phase 2).

| Requirement | Source plan(s) | Status | Evidence |
|-------------|----------------|--------|----------|
| PROF-02 | 05, 06, 11 | SATISFIED | `ref_profile_unknown`, `ref_profile_mismatch`, `ref_profile_rev_mismatch`, `resolve_*`, `ref_open_unbound`: typed refusal, no fallback |
| PROF-03 | 06, 13, 14 | SATISFIED | `profile_hash_fields`, `field_flip`, golden record lists every field with its binding |
| CONT-01 | 08, 09, 11 | SATISFIED | accept banksig cases for 256K, 384K, 1M; size and mapper refusal rows |
| CONT-02 | 11 | SATISFIED | byte order and header are declared inputs; `ref_order_*`, `ref_header_*`, `noheur_*` controls (no heuristic) |
| CONT-03 | 10, 11, 16 | SATISFIED | `alloc_*`, `nonmutation_*`, sanitizers stage, `ref_size_*` |
| CONT-04 | 07, 14 | SATISFIED | no file API in core (core-boundary); content_id vs host `shasum`, NIST digests, `record_content_fields` |
| CORE-01 | 05, 13, 16 | SATISFIED | `interleave_*`, `tests/controls/shared-state` red control |
| CORE-02 | 15 | SATISFIED | `core-boundary` plus 20 negative controls; WR-03 float gap closed |
| CORE-03 | 12, 14 | SATISFIED | `poweron_*`, `advance_rules`, LED-PROF-0002, seed in record |
| RUN-02 | 02, 03, 06, 18 | SATISFIED | 7 outcomes, `outcome-sync`, `timeout-budget` and `cancelled-spin` live controls |
| FIX-02 | 17 | SATISFIED | 43 manifests, `fixture-check` with `fix-missing-field`, `fix-unlisted` controls |
| FIX-04 | 02, 18 | SATISFIED | `inventory`, `removed-required` control, `mutation-inventory-required-missing` |
| FIX-05 | 04, 18 | SATISFIED | `rights-tree`, `rights-negatives`, `audit-history` clean, `romlike-binary` control |
| TOOL-02 | 01, 03, 21 | SATISFIED | README documents `sh scripts/check`; `docs-claims` stages keep docs true |
| CI-01 | 19, 20, 21, 22 | SATISFIED | Workflow, static `workflow-check`, timings, one real green run (limit stated in Truth 5) |

### Evidence Discipline (E0)

- No document or test claims more than E0. `README.md` says "It makes no accuracy or compatibility claim today" and "a matching hash proves repeatability only". LED-LOAD-0003 is `assumed` with `withheld = ["any claim that the rule matches hardware"]`. `docs/ci-timings.md` states E0 explicitly. A grep for accuracy and compatibility claims across `src`, `docs`, `README.md`, `ledger`, `profiles` and the case files found only negations or withheld-claim lists.
- Rights boundary: no ROM, firmware, disc image or save is tracked; fixtures are generated at test time and never committed.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| none | | `TBD`, `FIXME`, `XXX` in non-control source, scripts, docs, tests | none | None found outside the rights and scanner control fixtures that name the markers on purpose |

### Warnings and Notes (none block the phase)

1. **CI is one commit-range behind.** Green CI exists for `3958316`; the six review-fix commits are local and unpushed (D-64). A push needs owner authorisation. The fixes are covered locally by `sh scripts/check`, `--selftest`, `dash` and `audit-history`; only the gcc, clang, mawk and gawk legs of the new awk code remain unobserved. The WR-03 scanner was written to POSIX awk only (no interval expressions), but that is untested under mawk and gawk until the push.
2. **Stale README sentence.** `README.md:58` still says "No 0BSD fixture directory exists yet: Phase 1 ships no fixtures". `fixtures/manifest/` now exists with 43 0BSD-declared files (no binaries). It is a docs inaccuracy, not a claim above E0, and is not covered by `docs-claims`. Suggest a one-line fix.
3. **Timing table lacks the second run.** `docs/ci-timings.md` has one row (run 42939483070, head `b063518`); run 42940308647 on `3958316` is green but unrecorded.
4. **Open info findings** IN-01 (fixture-check `IFS` tab collapsing), IN-02 (state digest omits `mapper`, `byte_order`, `header`), IN-03 (REQUIRED grammar differs between ingest and inventory-check). The gate catches each by a second route today; IN-02 should be settled before a second mapper exists.
5. **Fixer's own flags:** WR-02 placement (post-table check versus a `boot_order.def` row) and WR-03 rule logic were flagged "human should confirm". Both are covered by shipped controls that fail against the old code; the choice is a planner decision recorded in 02-REVIEW-FIX.md, not a verification gap.
6. **Recorded hand-offs (not phase failures):** the three remote settings (Actions default token permissions read-only, Actions cannot approve PRs, branch ruleset on `main` requiring `gate`), CODEOWNERS handle confirmation, the CLAUDE.md Technology Stack section reading as an unadopted Rust proposal (decision 0001 governs), and CI sanitizers running only under gcc.
7. **Deliberate scope:** sanitizer and compiler diversity here is Apple clang only; gcc 13, clang 18, mawk and gawk evidence is the container run and CI, as stated above.

### Human Verification Required

None. Authority gates (push, remote settings) are recorded owner hand-offs, not UAT items.

### Gaps Summary

No gaps. Every success criterion and all 15 requirements are met with evidence from a fresh local gate run, a selftest run, an independent negative-control reproduction, a full-history audit and a green CI run on the earlier head. The one labelled limit is that the final local head `0ae9c89` has not yet been through CI.

---

_Verified: 2026-11-10_
_Verifier: Claude (gsd-verifier)_
