# Decision records

Each record is a trimmed MADR with a small closed front matter, checked by `sh scripts/decision-check docs/decisions` (CONTEXT.md D-25). MADR 4.0.0 compatibility is by spirit only. A `.protocol.md` file is a pre-registration, not a record, and is not indexed.

Allowed status words: `open`, `proposed-default`, `owner-recorded`, `superseded`, `withdrawn`. Only a verbatim owner statement allows `owner-recorded`. An agent may draft a record and set `proposed-default`.

Every record has exactly one row in this index, and the row's status matches the record's front matter. `decision-check` reports `INDEX_DRIFT` when they disagree.

| Record | Title | Status |
|--------|-------|--------|
| [0001](0001-language-build.md) | Language and build system | proposed-default |
| [0002](0002-first-profile.md) | First profile scope (pce-hucard-2btn) | owner-recorded |
| [0003](0003-api-abi.md) | Core API and ABI posture | owner-recorded |
| [0004](0004-licence-visibility.md) | Licence and repository visibility | owner-recorded |
| [0005](0005-fixture-rights.md) | Fixture rights | owner-recorded |
| [0006](0006-ci-provider.md) | CI provider | owner-recorded |
| [0007](0007-copied-code-provenance.md) | Copied-code provenance | owner-recorded |
| [0008](0008-evidence-tier-policy.md) | Evidence-tier policy | proposed-default |
