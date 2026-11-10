# DuoSpool

DuoSpool is an emulator project for the NEC / Hudson Soft PC Engine, sold in North America as the TurboGrafx-16. Its
support claims are tied to named hardware and content profiles and to recorded evidence. It makes no accuracy or
compatibility claim today.

DuoSpool is a working name and is not trademark-cleared.

## Status

Phase 2: a deterministic machine shell that accepts or refuses plain HuCard bytes under a named profile. It has no CPU,
video or audio, and nothing is claimed as supported. Every accept or refuse outcome comes from a fresh, headless,
repeatable run, and a matching hash proves repeatability only.

## Local gate

```
sh scripts/check
```

This is the one documented local command. It runs every gate stage and ends with `CHECK RESULT PASS` or
`CHECK RESULT FAIL`; run it before every commit.

CI runs the same command: sh scripts/check.

The gate prints one line per stage (`CHECK <stage> PASS|FAIL|ERROR`) and writes the same verdicts to `build/gate/results.tsv`.

Flags (usage: `sh scripts/check [--selftest [NAME] | --stage NAME] [--timing]`):

- `--selftest [NAME]` mutates a temporary copy of `scripts/` and proves that a removed rule turns the gate red; with `NAME`
  it runs only the mutation printed as `SELFTEST NAME`.
- `--stage NAME` runs only the stage `NAME` and exits with that stage's verdict.
- `--timing` prints the seconds each stage took, and a warning for a stage over 20 s. Time is a record, never a failure.

Exit codes: 0 every stage passed, 1 a stage failed or a control was broken, 2 a harness error (an unknown stage or
mutation name, a dead checker, a missing tool).

Tool prerequisites: a POSIX `sh`, cmake, ninja, a C11 compiler reachable as `cc` (clang or gcc), `nm`, perl, iconv, awk,
git, and a sha256 tool (`sha256sum` or `shasum`). The stage `preflight` reports a missing one by name.

## Where things are

- `docs/decisions/` holds the decision records and their index.
- `ledger/` holds the claim ledger and its schema.
- `PROVENANCE.md` is the register of any code copied from another project.

## Licensing

Licensed under either of Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE)) or MIT license
([LICENSE-MIT](LICENSE-MIT)) at your option.

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in DuoSpool by you, as
defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions.

Authored fixture directories are 0BSD ([LICENSE-0BSD](LICENSE-0BSD)), declared per directory.
No 0BSD fixture directory exists yet: Phase 1 ships no fixtures, and each later fixture directory declares 0BSD in its own directory.

## Rights

No commercial ROMs, firmware, system cards, disc images or private saves are accepted (decision record 0005).
