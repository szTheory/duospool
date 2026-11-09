# Walking Skeleton — DuoSpool (Phase 1 gate slice)

**Phase:** 1
**Generated:** 2026-11-09

This phase builds records, shell checkers and a throwaway toolchain spike. It does not build an
application. The skeleton is therefore the thinnest gate path, not an app stack: there is no
routing, database, UI or deployment, and none is invented here.

## Capability Proven End-to-End

`sh scripts/check`, run from the repository root, parses one real claim-ledger entry, rejects one
deliberately mislabelled negative control (a `supported` entry backed only by E0 and E4 evidence)
for exactly the expected reason, validates the front matter of one real decision record, and ends
with `CHECK RESULT PASS` (exit 0). Any dead or skipped stage ends with a non-zero exit instead.

Plan `01-01` Task 1 builds this path. Every later Phase 1 plan adds a stage to the same entry point.

## Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Gate entry point | One POSIX `sh` script, `scripts/check`. Exit codes: 0 pass, 1 violation, 2 harness error. Phase 2 extends this script and does not add a second entry point (D-30) | Provider-neutral. CI (GitHub Actions, D-03) becomes a thin wrapper in Phase 2 |
| Record syntax | One strict TOML subset for all hand-edited data (D-18): ASCII only, 16 KiB cap, no floats, dates or inline tables | Any real TOML parser can read it. No parser dependency |
| Parser shape | Two stages. `scripts/lib/toml-subset.awk` turns syntax into flat TAB records and ends with a sentinel line. Schema rules come next (`ledger-rules.awk`, later `profile-rules.awk`) | One owner for syntax. The sentinel means a crashed stage cannot read as "no violations" |
| Checker contract | Each checker takes `--expect CODE[,CODE]` and prints one sorted reason code per violation. Every negative control is a directory holding an `EXPECT` file | The negative controls show the gate is alive. A control that stops failing means the gate is broken |
| Decision records | `docs/decisions/NNNN-slug.md` with a small closed front matter (D-25). Only a verbatim owner statement permits `owner-recorded` | The record is the single source of truth. `PROJECT.md` only points to it |
| Ledger layout | `ledger/entries/LED-<AREA>-<NNNN>.toml`, one file per entry, plus `ledger/REQUIRED`, `ledger/negative/<case>/` and `ledger/samples/ok-<case>/` (D-19, with the per-case-directory refinement from RESEARCH Pitfall 5) | A product artifact rather than planning text. Phase 2 ports the checker to the chosen language |
| Directory layout | Top-level `docs/decisions/`, `ledger/`, `profiles/`, `scripts/`, `scripts/lib/`, `tests/<checker>/<case>/`, `PROVENANCE.md`, `LICENSE-*`. The temporary `spikes/tool-01/` tree is tagged and then removed (D-16) | Phase 2 starts clean in this layout |
| Licence of new files | `MIT OR Apache-2.0` for code, with SPDX header lines on every new script. `0BSD` for fixture directories (D-01, D-31) | Owner-recorded in this session |
| Portability floor | macOS `/bin/sh` (bash 3.2), BWK awk 20200816, `LC_ALL=C`, linted with `dash -n` | The Linux CI awk run is staged in Phase 2 (backstop) |

## Stack Touched in Phase 1

- [x] Project scaffold: `scripts/check` plus the checker library (no framework)
- [ ] Routing: not applicable (no service)
- [ ] Database: not applicable. The "data layer" is the strict-subset record files, read by the checkers
- [ ] UI: not applicable (headless gate)
- [x] Run command: `sh scripts/check` is the documented local full-gate command

## Out of Scope (Deferred to Later Slices)

- Any emulator code: CPU, VDC, VCE, PSG, loader. The spike stubs are throwaway evidence (D-16)
- The CI workflow YAML (Phase 2, CI-01; provider recorded in decision record 0006)
- The fixture manifest checker, the conformance inventory, and `TEST:` ID resolution (Phase 2, FIX-02/FIX-04, D-22)
- Full REUSE adoption, signed-commit attestation, and an owner decision on evidence policy (CONTEXT Deferred Ideas)
- A repository-content check for ROM-like binaries in the working tree (Phase 2, FIX-05). Phase 1 audits history only, before publication

## Subsequent Slice Plan

- Phase 2: the deterministic machine shell and the HuCard refusal matrix, built in the language chosen by record 0001, behind the same `scripts/check`
- Phase 3: HuC6280 fixtures with guest-RAM verdicts, admitted under records 0004 and 0005
- Phases 4-5: raw video and audio. Each new hardware fact gets a ledger entry first
- Phase 6: the runner and the installed-consumer proof, under the record 0003 posture
