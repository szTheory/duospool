---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 29
subsystem: w6-pad-negative-controls-interleave-input-regions-ledger
tags: [pad, negative-controls, fail-set, padlog, pad-order, exact-edges, interleave-pad, shared-input-toy, observer, chunk-equiv, iopt, schd, serial-7, withheld-rows, record-format-6, ledger, w6, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the two-button pad, the input schedule and the apply_input hook (03-27), input-check and the [input] record table (03-28), chunk_equiv and the observer kind (03-26), the dev sink (03-19), the shared-state toy and the toy mechanism (03-02, 03-12), the record layout pinned in four places (03-18, 03-22, 03-23, 03-28)
provides:
  - "guest expect items fail-set=LIST (exactly those checks read $46), padlog (dev sink on the first run, ID.padlog.tsv beside the evidence text, lost != 0 is harness_failure) and pad-order=A,B (the triple is run first; pairwise different verdict bytes and a SEL high and a SEL low read in each dev log or harness_failure), exact-edges (ds_io_read at at-1, at, at+1 against the protocol byte computed in the test)"
  - "scripts pad_2btn_none (explicit empty), pad_2btn_wrong, pad_2btn_late, pad_2btn_glitch; REQUIRED cases pad_2btn_none (fail-set 1,7,8,9,10,11,12), pad_2btn_wrong (1,4,7..12), pad_2btn_late (9,10), pad_2btn_glitch (all pass), pad_2btn_correct (now with padlog and pad-order), input_exact_edges, mutate_sweep_pad_2btn, interleave_pad, observer_pad, chunk_equiv_pad_2btn_correct"
  - "kind interleave_pad and the toy tests/controls/shared-input (the 11th toy, RED_OK); selftest mutation-pad-order-assert; apply_input in the sweep, chunk, observer, digest and record_run kinds with the input_applied post-condition"
  - "digest regions IOPT (6 bytes) and SCHD (16392 bytes), DS_SERIAL_VERSION 7, digest_stream_input; recomputed digest literals and goldens"
  - "constants rows pad_clr_high_nibble, io_poweron_latch (LED-IO-0003) and pad_settle_mclk (LED-IO-0004) read by io.c; [withheld] as count and one row id_N per ledger id, DS_TEXT_FORMAT_VERSION 6"
  - "ledger entries LED-IO-0002 to LED-IO-0006 (0006 deferred) and the LED-IO-0001 edit, all assumed with E0 evidence and one pin each, none promoted"
affects: [03-30, 03-31, 03-32, 03-34]

actuals:
  tokens: 29900
  tasks: 3
  commits: 5
plan_head_before: cfb6d1bb88bad5d798b839b533c3de215bdfc400
plan_head_after: b49104555ead00f7b80c5bb61e33512e1935ce8b

tech-stack:
  added: []
  patterns:
    - "a negative control is judged by the exact set of failing checks in a consistent done block (a reason that does not start with check means the block itself is wrong), and the case that trusts the correct run first runs the two negative scripts and requires pairwise different verdict bytes and a SEL high and a SEL low read in each dev log; the one statement that compares is tagged and a scratch runner without it must accept an identical triple that the real runner refuses"
    - "a kind that only compares runs with each other asserts the post-condition of the input call (the instance holds the declared events), or a forgotten apply_input would compare runs that all lack the input and pass vacuously"
    - "a toy whose red does not depend on any counter phase: the static returns the last nonzero level when the instance's own level is empty, the correct script is never empty, the wrong script ends with an empty set, and the case steps A before B in every round"
    - "a throwaway C program outside the repo dumps the previous stream with the core's writer, its hash is first shown to equal the committed literal, and a throwaway script appends the new frames, so a literal is recomputed from written rules and not from the new code"
    - "a record table with a variable number of keys (stimulus events, withheld ids) is one count row and N numbered rows, validated by the lint, rebuilt by the test from the defs and the request and written into goldens from the defs and the ledger"

key-files:
  created:
    - tests/conformance/inputs/pad_2btn_none.toml
    - tests/conformance/inputs/pad_2btn_wrong.toml
    - tests/conformance/inputs/pad_2btn_late.toml
    - tests/conformance/inputs/pad_2btn_glitch.toml
    - tests/conformance/cases (pad_2btn_none, pad_2btn_wrong, pad_2btn_late, pad_2btn_glitch, mutate_sweep_pad_2btn, input_exact_edges, interleave_pad, observer_pad, chunk_equiv_pad_2btn_correct, digest_stream_input)
    - tests/controls/shared-input (RULE, EXPECT)
    - ledger/entries/LED-IO-0002.toml to LED-IO-0006.toml
    - tests/gate/record-lint/withheld-count-mismatch and withheld-unsorted
  modified:
    - tests/conformance/guestkinds.c
    - tests/conformance/inputkinds.c
    - tests/conformance/ilvkinds.c
    - tests/conformance/sweepkinds.c
    - tests/conformance/chunkkinds.c
    - tests/conformance/obskinds.c
    - tests/conformance/digkinds.c
    - tests/conformance/reckinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - src/core/serial.c
    - src/core/ds_serial.h
    - src/core/io.c
    - src/core/constants.def
    - src/text/text.c
    - src/text/ds_text.h
    - scripts/check
    - scripts/lib/record-rules.awk
    - ledger/entries/LED-IO-0001.toml
    - ledger/REQUIRED
    - tests/conformance/golden (both records)
    - tests/gate/record-lint and tests/gate/determinism (17 records)

key-decisions:
  - "Ledger ids: the plan, 03-CONTEXT section K and the REQUIRED origins name LED-IO-0002 to LED-IO-0006 (0006 deferred); the orchestrator's message said 0002..0008 with 0008 deferred. Written as the plan and CONTEXT say (five entries plus the LED-IO-0001 edit, six ledger files); an LED-IO-0007 or -0008 would have needed content that no source in the repository gives"
  - "fail-set is a guest expect item that takes the mode slot, judged in a scratch context like verdict; the case's own outcome is pass when the set equals the declaration. The glitch case has no fail-set: its pulses (12 master clocks at 90000, 280000, 460000) lie in the gaps between the guest reads (about 37000, 228000, 415000, 601000) so all 12 checks pass as with the correct script, which is the declared outcome; a scratch script with a pulse exactly on a read stamp (228360) failed check 7, so the placement of the pulses is what makes them invisible"
  - "The wrong script ends with an explicit empty held set (held = []), because the shared-input toy leaks only into a read whose own level is empty; the none and wrong fail sets differ by check 4 (a direction is held in phase 1)"
  - "The late script moves only the third event from 320000 to 450000 (after the phase 3 reads near 415000), so exactly checks 9 and 10 fail: the late control has its own declared set, not the one of the none control"
  - "pad-order lives in kind_guest as an expect item (the plan says the assertion runs inside pad_2btn_correct): the two other scripts run under the first policy only, with the dev sink, before the case's own run is trusted; the statement that compares verdict bytes is one tagged line (rule-pad-order) so the selftest can delete exactly it"
  - "The pad log is the pad-window part of the dev log ($1FF000 to $1FF3FF): a header row stamp, phys, value, kind, latch and one row per access; the latch of a read is the value of the last write there, $FF before any. A read counts for SEL high or low only with CLR low"
  - "[withheld] layout: count = N and rows id_1 to id_N (the stimulus pattern); the lint checks the keys, ledger-id values and strictly ascending order; the test rebuilds the expected sorted list from the defs and the request (fields the request bound, unresolved behaviours, the three load ids, model and constants rows) and compares row by row. WH_MAX 24 to 64, REC_MAX_ROWS 128 to 192"
  - "The goldens' withheld list was computed from constants.def, model_params.def and profile_fields.def with a script and compared with the committed 24 ids plus LED-IO-0003 and LED-IO-0004 before it was written (26 ids, equal), not read from printer output"
  - "Constants are real: io.c reads io_poweron_latch at power-on, pad_clr_high_nibble for CLR high and pad_settle_mclk as a look-up offset (0, so no behaviour change); a value change would change the printed record and the digest-free behaviour, and bumps DS_BEHAVIOUR_EPOCH per the constants.def rule (not changed here: values are the existing behaviour)"

requirements-advanced: [CORE-06]
requirements-completed: [INP-02, INP-03, INP-04]  # INP-03: none and wrong differ from correct (pairwise different verdict bytes, declared fail sets) and a pad-read log per run; INP-04: per-port state (o_latch, slot_type[5], the per-port schedule) is in the digest as IOPT and SCHD and cannot be a single global mask; INP-02: the declared schedule is applied deterministically (exact edges at at-1, at, at+1, chunk_equiv under seven schedules, observer_pad, interleave_pad). INP-01 was completed in 03-27. CORE-06 stays advanced (the snapshot half is 03-30)

duration: 55min
completed: 2026-11-11
---

# Phase 3 Plan 29: Pad negative controls, input exactness, interleave, IOPT and SCHD and the W6 ledger entries Summary

**The same pad_2btn ROM gives three different, pairwise-distinct verdicts for the correct, no-input and wrong-input scripts (declared fail sets, a pad log per run, a runner assertion that bites in a scratch build), a late-event and a glitch script have their own declared outcomes, the level of a change is exact at at-1, at and at+1, two instances with different scripts interleave without sharing a level (the shared-input toy turns that case red), the digest gains IOPT and SCHD (serial version 7), and LED-IO-0002 to LED-IO-0006 and the LED-IO-0001 edit are written, all assumed or deferred, none promoted.**

## What was built

- **Task 1 (tracer), commit 1be0e23.** `fail-set=LIST`, `padlog` and `pad-order=A,B` in `kind_guest`. `pad_2btn_none` (events = 0) fails exactly checks 1 and 7 to 12; `pad_2btn_wrong` (select and up, run and left, i and down, nothing) fails 1, 4 and 7 to 12. `pad_2btn_correct` now carries `padlog;pad-order=pad_2btn_none,pad_2btn_wrong`: it runs the two scripts first, requires the three result-and-observed-byte blocks to be pairwise different (`same=000` in its evidence) and a SEL high and a SEL low read with CLR low in each dev log, or the case is harness_failure. `kind_mutate_sweep` pushes the case's input before every run and `mutate_sweep_pad_2btn` sweeps the 12 checks. `selftest mutation-pad-order-assert`: a scratch tree with the one statement tagged `rule-pad-order` deleted accepts a scratch triple of three identical scripts that the unmutated scratch runner refuses (`harness_failure`, `pad-order`). Tracer gate: `Tracer verified end-to-end - expanding` (the whole gate ran green before Task 2).
- **Task 2, commit 438c4a7.** `pad_2btn_late` (third event at 450000: fail-set 9,10) and `pad_2btn_glitch` (three 12-master-clock pulses in the gaps between reads: all pass); `exact-edges` (`input_exact_edges`: 3 events, 18 reads, plus a check that each edge changes a byte); `kind interleave_pad` (A correct, B wrong, solo copies first, rounds of 20000 master clocks, A before B; digest, machine time and verdict of each equal its solo copy, A passes, B fails, the two verdicts differ); the toy `tests/controls/shared-input` (anchor `return held;` in io.c, a function-local static that remembers the last nonzero level and returns it for an empty one): RED_OK with the right reason (the verdict block of B differs from that of its solo B). `apply_input` joined the chunk, observer, digest and record_run kinds (the sweep got it in Task 1) with the `input_applied` post-condition; `observer_pad` and `chunk_equiv_pad_2btn_correct` are REQUIRED.
- **Task 3 split into three complete commits (see Deviations): eaa8ab4, afbee90, b491045.**
  - *Ledger.* LED-IO-0002 (protocol, nibble order; the pcetech RIGHT and DOWN swap carried in the disagreement as a community report against the three-source order), LED-IO-0003 (CLR high, bits 4 and 5, power-on latch; H8-18 and H8-19 as the page basis, status assumed per R-5), LED-IO-0004 (no settling time, the level at the read stamp, master-clock unit), LED-IO-0005 (no turbo, refused inputs), LED-IO-0006 (multitap, six-button, mouse; deferred; `NONE:` pin). The Mednafen readings (CLR-high line commented out, power-on SEL = CLR = 0, bit 6 left clear) are `REPORT` sources and disagreement text labelled as advisor observations of reading material, not re-read and not copied. LED-IO-0001 gains the bit 6 observation and the pin `TEST:pad_2btn_correct` (was `PENDING:2`). Pins: 0001 and 0002 `pad_2btn_correct`, 0003 `pad_2btn_none`, 0004 `input_exact_edges`, 0005 `input_refusal_rows`; the four cases name them in `pins`. `ledger/REQUIRED` lists 0002 to 0006.
  - *Regions.* IOPT (o_latch, slot_type[0..4], 6 bytes) and SCHD (1024 slots of time u64, port u32, mask u32, then count and a reserved word, 16392 bytes) after BLKT; `DS_SERIAL_VERSION` 7; layout static asserts. The four `digest_stream_*` literals and both golden digests were recomputed outside the C code (below). New REQUIRED case `digest_stream_input` pins SCHD with the four events of pad_2btn_correct (nonzero bytes, literal `3dbe2cac...`). `digest_stream` checks IOPT and SCHD field by field; `digest_region` flips every IOPT byte and the first and last byte of SCHD.
  - *Constants and layout.* Three constants rows (above), read by `io.c`, whose ids LED-IO-0003 and LED-IO-0004 would have been the 25th and 26th [withheld] ids: so the layout change of the standing note was done in the same commit (format 5 to 6, `count` and `id_1`..`id_N`), in all four pinned places (printer, `record-rules.awk` with two new tagged rules and two negative mini-roots plus `selftest mutation-record-lint-withheld`, `reckinds.c`, the 17 mini-root records and the two goldens), with the golden blocks written from the defs and the ledger.

## Evidence files and logs (the SUMMARY lists, as the plan asks)

Per run in `build/gate/evidence/` (E0 runner artifacts; stdout still prints ids, outcomes, sizes and hashes): `pad_2btn_correct.padlog.tsv`, `pad_2btn_none.padlog.tsv`, `pad_2btn_wrong.padlog.tsv`, `pad_2btn_late.padlog.tsv`, `pad_2btn_glitch.padlog.tsv`, each beside its `.txt`. The correct log has 22 pad-window rows (reads at about 37392, 37716, 39552, 40764, 41856, 42948, 228360, 229452, 414864, 415956, 601368, 602460 and the writes before them); the raw reads are also the observed bytes of the DSV1 block (R-7).

## Verification

- Task verifies as written, all PASS: Task 1 `--stage input-check`, `--stage conformance`, `--selftest mutation-pad-order-assert`; Task 2 `--stage conformance`, `--selftest mutation-control-anchor` (11 lines, `shared-input` included); the ledger part `--stage ledger-entries`, `--stage conformance`, `--stage inventory`.
- Plan end: `sh scripts/check --timing` PASS after every commit (last: 3 min 31 s wall under a load average above 50, user 149 s; per stage under that load: sanitizers 69 s, conformance 37 s, determinism 25 s, named-controls 10 s; quiet-machine figures before the load were conformance 19 to 22 s, sanitizers 52 s, determinism 21 s, whole gate 2 min 33 s), `sh scripts/check --selftest` PASS (98 PASS lines, no FAIL, 35 s), `sh scripts/audit-history --range cfb6d1b..HEAD` clean (5 commits, 86 blobs), `dash` runs of `--stage conformance`, `record-lint-negatives` and `input-check` PASS. All eleven toy controls RED_OK (`shared-state` included: run.c is unchanged).
- Bite, shown with scratch edits restored from saved copies and `cmp`-checked (nothing committed): `<=` to `<` in `level_at` turns `input_exact_edges` red at the event stamp (the fixture cases stay green, their reads are far from the events); removing `apply_input` from chunkkinds and obskinds turns `chunk_equiv_pad_2btn_correct` and `observer_pad` red through `input_applied`; swapping the port and mask writes in `SCHD` turns only `digest_stream_input` red (the zero-schedule literals cannot see it); dropping the constants loop of `emit_withheld` turns `record_constants_shown`, both goldens and five skeleton cases red; a scratch pulse placed exactly on the read stamp 228360 fails check 7, so the glitch script's invisibility comes from its placement; the two withheld lint rules each flip their mini-root under `mutation-record-lint-withheld`.
- Digest literals: a throwaway C program outside the repo linked the runner objects, booted each request and wrote the version 6 stream; the 8607-byte streams hashed (`shasum -a 256`) to the six committed values (four digest_stream literals and two golden digests) before anything changed; a throwaway python script set the version to 7 and appended IOPT (FF 01 00 00 00 00) and SCHD (16392 zero bytes), and the 25021-byte streams hash to the new values. For `digest_stream_input` the SCHD frame holds the little-endian encoding of the four events. Both programs live in the session scratch directory, not in the repo.
- Manual pages: the hardware manual (sha256 40a32e5d..., matches 03-RESEARCH) pages H8-18 and H8-19 (PDF pages 21 and 22) and the CD BIOS manual (sha256 8e92ead8...) printed page 28 (PDF page 27) were read as images from a scratch directory outside the repo. Seen: H8-18 lists "H level is output to the output port (port O)" among the reset states; H8-19 gives port K as eight inputs with a pull-up and port O as eight latched outputs, high at reset, read and written at physical $1FF000 to $1FF3FF; page 28 gives the EX_JOYSNS bits (ONE 0, TWO 1, SELECT 2, RUN 3, UP 4, RIGHT 5, DOWN 6, LEFT 7, 1 = on) and the JOYENA enable mask for pads #1 to #5 with a multitap. Nothing was copied or committed.

## Deviations from Plan

**1. [Rule 3 - scope, planned cut not needed] Task 3 was committed as three complete commits instead of one.** The ledger entries (eaa8ab4), the IOPT and SCHD regions with serial 7 and the recomputed digests (afbee90), and the constants rows with the withheld layout change (b491045) each pass the whole gate; no task was left half written and nothing was cut. The orchestrator's fallback (cut at Task 3) was not used.
**2. [Plan wording] LED-IO-0002 to LED-IO-0006, not 0008.** See key decisions; the plan and 03-CONTEXT section K list five new entries (0006 deferred) plus the LED-IO-0001 edit, which the orchestrator message counted as six.
**3. [Rule 3 - files outside the plan list] More than the 38 files named.** Added because the work needs them: `runner.c` and `runner.h` (the evidence directory pointer for the pad log, the kind row, REC_MAX_ROWS), `inputkinds.c` (exact-edges, the `input_applied` post-condition), `obskinds.c`, `digkinds.c`, `reckinds.c` (apply_input hooks, the IOPT and SCHD frame checks, the withheld rebuild), `text.c`, `ds_text.h`, `record-rules.awk` and 17 mini-root records (the layout change that the new constants ids forced), the two withheld negatives, and `digest_stream_input` (a new REQUIRED case so that SCHD is pinned with nonzero bytes).
**4. [Plan wording] The glitch case has no fail-set.** The plan says "declared fail-sets" for both controls; the glitch pulses are chosen to be invisible, so its declared outcome is a plain pass of all 12 checks (the case that distinguishes it from correct is the exact-edges test and the scratch pulse on a read stamp shown above).
**5. [Design] The "level-query return" anchor of the toy is `return held;`** with a replacement that leaks only when the instance's own level is empty (the wrong script's last phase), so the interleave case is red by construction when A is stepped before B; this is argued in the RULE comment, not retuned by eye.
**6. [Plan wording] The padlog of each run is written by its own case** (none, wrong, late, glitch, correct), not by the correct case on behalf of the others; the correct case still runs the two negative scripts first and checks their dev logs and verdicts.

No Rule 4 deviation. The standing notes: the shared-state toy stayed RED_OK (run.c unchanged); the [withheld] layout change was done in the same commit as the first new ids (as the note requires); the ledger provenance gap was handled by labelling advisor-recorded readings as such and naming no emulator reading that is not in the discuss notes.

## Evidence discipline

E0 for the mechanisms (fail sets, order assertion, exact edges, interleave, chunking, observer, regions). E2 for everything the guest reads through the pad: the pad core and the pad fixture are written by one party (ER-5). The declared fail sets follow from the scripts and the fixture literals (ASSUMED LED-IO-0002 to LED-IO-0004), not from DuoSpool output; the digest literals come from a host tool over a stream rebuilt outside the code; the exact-edges bytes are computed in the test from the protocol text. All five new entries are `assumed` (0006 `deferred`) with one pin each; none was promoted (only plan 03-32 may) and none of the eight ER-2 candidates was touched. No accuracy, pad-accurate, compatibility, cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64). Remote evidence stays unverified, pending owner push authorisation.

## Known Stubs

None added. The input schedule is in the digest now; a snapshot and restore of it do not exist until plan 03-30. Ports 2 to 5 have no device (r1 populates port 1 only); multitap, six-button, mouse and turbo are deferred (LED-IO-0006). The ledger file `.planning/WINDOWS.md` is absent, so nothing was appended.

## Threat Flags

None beyond the plan's register. T-03-80 (vacuous controls): declared fail sets, pairwise different verdict bytes and the SEL-high and SEL-low log requirement, each with a deletion selftest or a scratch bite; T-03-81 (shared input): `interleave_pad` plus the shared-input toy RED_OK; T-03-82 (pad logs): the logs go to the evidence directory of authored fixtures only and stdout stays ids, outcomes, sizes and hashes. New surface: the evidence directory now receives `.padlog.tsv` files (authored fixtures, raw reads of a fixture, no content of any commercial image).

## Notes for next plans

- **03-30 (snapshot):** the restore domain gains IOPT and SCHD: `o_latch` any byte, `slot_type[p]` 0 or 1 (port 1 must match the profile's input_port1), `input_count` at most 1024, `input_reserved` 0, every slot below the count a valid event (port 1 to 5 with a pad behind it, mask with no bit above bit 7 and no opposite directions, times non-decreasing, an equal time with a strictly increasing port), every unused slot all zero. The stream is the same bytes as the digest (25021 bytes for the default boot: 8607 for version 6 plus the 6-byte and 16392-byte frames and their 8-byte headers), so `sha256(snapshot)` equals the digest.
- **Record layout (03-30, 03-31):** [withheld] is `count` and `id_1`..`id_N` (at most 64); the record row count is 68 plus the number of ids, plus one per stimulus event, 4 per model parameter and one per constant. The [constants] table has 18 rows now. A new ledger id printed by any later row needs no layout change; a hard limit now is REC_MAX_ROWS (192), at 26 ids and 18 constants a boot record has 120 rows.
- **03-32 (ledger):** LED-IO-0002 to LED-IO-0005 stay `assumed`; the disagreement columns name only readings recorded by the discuss advisor (labelled as such). The LED-CPU-0015 disagreement gap of the standing note is untouched here.
- **03-34 and later:** a case that declares `input` is now applied by every runner kind (guest, sweep, chunk_equiv, observer, digest_stream, record_run, record_lines); a kind that only compares runs must keep asserting `input_applied`. The shared-input toy argument depends on A stepping before B in `interleave_pad`; changing the round order needs the RULE comment revisited. Editing `pad_2btn_correct.toml` changes the `record_input_shown` literal and the `digest_stream_input` literal (recompute with printf, shasum and the python script described above).
- **Cost:** the new cases are cheap (every pad run is 800000 master clocks); the conformance stage grew by about 3 to 4 s of user time and sanitizers by a few seconds; `.github/workflows/gate.yml` unchanged. `.planning/state.json` left untouched and uncommitted.
- **Owner-gate items:** none pending from this plan.

## Self-Check: PASSED

Created files present: the four scripts under tests/conformance/inputs, the ten new cases (pad_2btn_none, pad_2btn_wrong, pad_2btn_late, pad_2btn_glitch, mutate_sweep_pad_2btn, input_exact_edges, interleave_pad, observer_pad, chunk_equiv_pad_2btn_correct, digest_stream_input), tests/controls/shared-input, ledger/entries/LED-IO-0002.toml to LED-IO-0006.toml and the two withheld mini-roots. Commits 1be0e23, 438c4a7, eaa8ab4, afbee90 and b491045 are ancestors of HEAD.
