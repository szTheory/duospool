<!-- SPDX-License-Identifier: 0BSD -->
<!-- SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors -->

# Fixture toolchain record

This file records the external tools that build the authored HuCard fixtures under `fixtures/src` (FIX-01; decisions D-123, D-128, D-129, D-130, D-133). It is a tool record, not an owner decision. The claim that ties it to the ledger is `LED-TOOL-0001`, status assumed.

## The assembler and linker: cc65 V2.19 (ca65 and ld65)

| Item | Record |
|------|--------|
| Source | https://github.com/cc65/cc65, tag V2.19, commit `555282497c3ecf8b313d87d5973093af19c35bd5` |
| Licence | Zlib. It is on the SPDX allowlist of `docs/decisions/0007-copied-code-provenance.md`, so the tool does not constrain DuoSpool's own licence. Nothing of cc65 is copied into this repository: no library, no startup code, no header, no linker config. `fixtures/lib/hucard.cfg` is project-owned. |
| Licence check | Zlib is the licence recorded for cc65 in 03-RESEARCH and the Phase 1 stack research (the repository licence file). It is a routine agent check, not an owner decision; repeat it from the repository licence file whenever the pinned commit changes. This plan did not re-read the file. |
| Local package | Homebrew core bottle `cc65` 2.19 (`brew install cc65`) |
| CI package | Ubuntu 24.04 `cc65` 2.19-1 (`sudo apt-get install -y cc65`); the CI step is added by a later plan, and no install script is kept in this repository |
| Version string | Every build above prints `ca65 V2.18` (the V2.19 tag builds a binary that reports V2.18, and the Ubuntu package prints `V2.18 - Ubuntu 2.19-1`). Never read `--version` to identify the tool. |
| Identity | By behaviour: the bytes of the encoding matrix that `scripts/xasm-check` (plan 03-08) assembles and compares with the manual transcription (`tests/oracle/huc6280-opcodes.tsv`), plus the image hash pins in `fixtures/manifest/ds_proto*.toml`. A tool that prints the right version and encodes a byte differently fails there. The gate stage `xasm` runs it on every gate run. |
| Presence | `scripts/preflight` requires `ca65` and `ld65` by default and `scripts/fixture-build` refuses to run without them (`PREFLIGHT_MISSING`, exit 2). A missing assembler is never a skip. |

### What was measured, and what was not

- 03-RESEARCH (2026-11-10) assembled one prototype source to one 256 KiB image on the Homebrew bottle, on Ubuntu `cc65` 2.19-1 in a container and on a source build of the V2.19 tag, and got the same SHA-256 from all three. That prototype source is not in this repository, so the measurement is not reproduced here.
- Plan 03-06 (2026-11-11) measured the committed sources on the Homebrew bottle only: `ds_proto.s` and its four protocol variants gave the same five hashes on two consecutive builds, from a build in the working tree and from a build of the same files
  extracted to a different directory, and for the sources as they were at commit `2b09d68` (plan 03-05) and at plan 03-06. The pins in the manifest are those hashes. The hashes quoted in the plan 03-05 summary do not reproduce and should be ignored;
  the manifest is the record.
- Plan 03-08 (2026-11-11) ran the encoding matrix on the Homebrew bottle (`ca65 V2.18`): all 234 defined rows of the manual transcription assemble to the transcribed bytes (opcode plus fixed placeholder operands), `STATUS xasm ok rows=234 wla=optional-absent`. The matrix covers every defined opcode in every mode the manual lists, so it is a byte-level identity check of the assembler against the transcription, not a check of cycles or behaviour. WLA-DX was absent, so no second-assembler claim is made.
- Not measured here: the Ubuntu package on these sources. The pins are repeatability evidence on one package family (E0). Until a run on the apt package shows the same hashes (needs a push, which is an owner decision), CI and local identity is an assumption recorded in `LED-TOOL-0001`.

## The opcode table: numeric data from the manual pages

`tests/oracle/huc6280-opcodes.tsv` is numeric data transcribed from the cited manual pages (the H8-25 opcode map and the S8 instruction pages, with the flag tables S8-7 to S8-9), read as page images by an agent on 2026-11-11 (method, verifier and the printing errors found are in the file header). It holds opcodes, mnemonics, mode names, byte and cycle counts, flag strings and page labels. It never holds page text or an image (ER-9), and it is licensed 0BSD like the other numeric data of the repository. It is the independent second table for the core opcode table (plan 03-09) and the matrix input of `scripts/xasm-check`.

## WLA-DX: optional, verify-only

WLA-DX (`wla-huc6280`, `wlalink`) is GPL-2.0-or-later. It is never committed, vendored, installed in CI or distributed, and its `include/pcengine` files are never included in a fixture (D-130). It is not on the preflight list. A later plan may run it
as a one-off cross-check when it happens to be on PATH and print `optional-absent` otherwise; its output is compared and discarded. No claim of a second-assembler check is made while it is absent.

## Not used

- HuC, PCEAS and their library or startup code: the redistribution status is unclear, so nothing from them is included in fixtures.
- SingleStepTests data (65x02 and the Mesen-generated HuC6280 set): not used in Phase 3 (D-130). Such data is an emulator-derived oracle (E4) and is deferred.

## Manuals cited, never committed

The three PDFs are read as untrusted data from a scratch directory outside the repository. Only the URL, the byte count, the SHA-256 and the page label are recorded here (ER-9); no text and no image of a page is committed. The page labels in `fixtures/lib/cites.txt` follow the page rules below.


- HuC6280 Hardware Manual (label H8-n is PDF page n+3): https://raw.githubusercontent.com/MiSTer-devel/TurboGrafx16_MiSTer/master/docs/HuC6280%20-%20CMOS%208-bit%20Microprocessor%20Hardware%20Manual.pdf ; 1196754 bytes ; sha256 `40a32e5dec7fefd8ea1893be91a53e6f6d4eae5f9c86154019ae792b40374fcb`
- HuC6280 Software Manual (label S8-n is PDF page n+2): https://raw.githubusercontent.com/MiSTer-devel/TurboGrafx16_MiSTer/master/docs/HuC6280%20-%20CMOS%208-bit%20Microprocessor%20Software%20Manual.pdf ; 2416355 bytes ; sha256 `51eeda397e413527fe33f5446d83bd6ff7a49da2a0fd56cf3840c65c0920fd0c`
- Hu7 CD BIOS Manual (printed page 28 is PDF page 27): https://raw.githubusercontent.com/MiSTer-devel/TurboGrafx16_MiSTer/master/docs/Hu7%20CD%20System%20-%20BIOS%20Manual.pdf ; 1053799 bytes ; sha256 `8e92ead829d9c93316bc66b2eb271e43c7f7e8a91d86db00e1d798ab7c908a6e`

Whether the hosting mirror may redistribute these documents is not settled here; this repository does not redistribute them.
