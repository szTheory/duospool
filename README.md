# DuoSpool

DuoSpool is an emulator project for the NEC / Hudson Soft PC Engine, sold in North America as the TurboGrafx-16. Its
support claims are tied to named hardware and content profiles and to recorded evidence. It makes no accuracy or
compatibility claim today.

DuoSpool is a working name and is not trademark-cleared.

## Status

Phase 1 gate: owner decisions, a claim ledger, a first-profile record and a toolchain spike. There is no emulator code yet.

## Local gate

```
sh scripts/check
```

The gate prints one line per stage and ends with `CHECK RESULT PASS` or `CHECK RESULT FAIL`.

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
