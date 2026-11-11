---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 27
subsystem: w6-two-button-pad-input-schedule-input-script-path
tags: [pad, sel-clr, io-port, input-schedule, ds_input_push, input-refusals, conformance-flatten, input-rules, pad_2btn, w6, tracer]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the bus choke point, access stamps and the dev sink (03-16, 03-19, 03-21), the typed refusal pattern and irq_stim (03-22), the guest kind, DSV1 and the check macros (03-04, 03-05), the one TOML subset parser and conformance-flatten (D-51), chunk_equiv and the observer kind (03-26)
provides:
  - "src/core/io.c - the $1000 window: a write stores the latch of port O (bit 0 SEL, bit 1 CLR), a read is $30 | d6<<6 | d7<<7 | nibble (CLR 1 gives 0, SEL 1 the inverted directions, SEL 0 the inverted buttons, pressed reads 0) of the pad on port 1 at the access stamp; power-on latch $FF; ds_input_push copies a declared schedule (at most
    1024 events {at, port, mask}) into the instance before the first step and re-validates every event"
  - "src/core/input_refusals.def - the closed refusal set input.port, input.state, input.bits, input.order, input.queue_full, input.late (own prefix, F4)"
  - "the instance holds the schedule (input_ev[1024], input_count, input_reserved) and the port bytes (o_latch, slot_type[5]), not yet in the digest (plan 03-29 adds the IOPT and SCHD regions)"
  - "scripts/conformance-flatten --inputs (stage 1 toml-subset.awk, stage 2 the new scripts/lib/input-rules.awk) writing build/gate/inputs.tsv (SCRIPT, INPUT and END rows), passed as --inputs to the runner in run mode (gate and sanitize), the toy runs and the determinism stage"
  - "tests/conformance/inputkinds.c - the case key input (a script name), apply_input before the first step, the guest expect item refused=input.NAME[,NAME...] and the deferred kind input_refusal_coverage"
  - "fixtures/src/pad_2btn.s (DSV id 14, 12 checks) with the correct script tests/conformance/inputs/pad_2btn_correct.toml; cases pad_2btn_correct, input_port_refusal, input_refusal_rows, input_refusal_coverage (all REQUIRED)"
affects: [03-28, 03-29, 03-30, 03-31, 03-32, 03-34]

actuals:
  tokens: 21400
  tasks: 3
  commits: 4
plan_head_before: 2cd52a08ae7c7fc2ab49fcdd7f2ee87549cb1b7c
plan_head_after: e0a476329661554f114b02777e3df924c37acc1e

tech-stack:
  added: []
  patterns:
    - "an input is a declared, immutable schedule copied into the instance before the first step and evaluated as a pure function of the read stamp (the mask of the last event of the port at or before the stamp): no cursor, so no chunking, observer or restore can make two reads at one stamp differ"
    - "the pad is a device read through the hardware-page bus path like the timer: the access stamp, the dev log and the bus latch are the bus's, the byte is the device's"
    - "a guest fixture that depends on a scripted input places the events in the middle of counted waits (GAP n) so the placement does not depend on the exact cost of the code between them; the stamps of the reads were read off the device log of the run (E0) and each event sits about 90000 master clocks from the nearest read"
    - "the raw byte of every read is the observed byte of its check, so the DSV1 block is the guest-side pad-read log (R-7) with no masking"
    - "a refusal row of a core entry point boots a fresh instance per code, proves a valid list accepted and visible first (a core that refused everything must fail), then compares the whole instance before and after every refused call"

key-files:
  created:
    - src/core/io.c
    - src/core/input_refusals.def
    - scripts/lib/input-rules.awk
    - tests/conformance/inputkinds.c
    - tests/conformance/inputs/pad_2btn_correct.toml
    - fixtures/src/pad_2btn.s
    - fixtures/src/pad_2btn.checks
    - fixtures/manifest/pad_2btn.toml
    - tests/conformance/cases/pad_2btn_correct.toml
    - tests/conformance/cases/input_port_refusal.toml
    - tests/conformance/cases/input_refusal_rows.toml
    - tests/conformance/cases/input_refusal_coverage.toml
    - tests/gate/flatten-input (ok-script and five negatives)
  modified:
    - src/core/ds_core.h
    - src/core/bus.c
    - src/core/boot.c
    - scripts/conformance-flatten
    - scripts/conformance-run
    - scripts/determinism-run
    - scripts/check
    - tests/conformance/guestkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/REQUIRED
    - CMakeLists.txt
    - fixtures/src/mmu_mpr.s
    - fixtures/src/mmu_mpr.checks
    - fixtures/manifest/mmu_mpr.toml
    - fixtures/manifest/mmu_mpr_384k.toml
    - fixtures/manifest/mmu_mpr_1m.toml

key-decisions:
  - "Wire protocol as D-115 (SEL bit 0, CLR bit 1, CLR 1 reads nibble 0, SEL 1 directions D0 UP D1 RIGHT D2 DOWN D3 LEFT, SEL 0 buttons D0 I D1 II D2 SELECT D3 RUN, pressed 0, bits 4 and 5 read 1, d6 and d7 from the run-request bindings, no settling model); the logical mask is the CD BIOS layout (CDBIOS-p28, re-read as an image). The pcetech disagreement
    (RIGHT and DOWN swapped in the direction nibble) is recorded in io.c, the checks file, the fixture header and the case, and carried into the ledger entry of the protocol (ER-8); the core follows the three-source order of D-115 and does not decide it"
  - "A read of the $1000 window makes the byte read the new io_buffer, like every read in $0800-$17FF (D-104, ASSUMED LED-CPU-0013, mmu_mpr check 34); 'reads change nothing' of D-115 is read as: no pad, latch or schedule state changes"
  - "Instance layout: the schedule (input_ev as an array of 16-byte {at, port, mask}, input_count, input_reserved = 0) sits between irq_stim_level and timer_origin on the 8-byte boundary, and o_latch and slot_type[5] sit between blk_dst and rom; each is one live span with no padding (static asserts), so plan 03-29 can register IOPT (6 bytes) and SCHD (16392
    bytes) with adjacency asserts only. The unused slots are zero, so the schedule is a function of the declared list alone"
  - "ds_input_push check order: queue_full (count above 1024, or NULL with a count), then each event's port (1 to 5, and on a real instance a pad behind it) and bits (a bit above bit 7, UP with DOWN, LEFT with RIGHT), then the order (at never decreases, an equal at needs a greater port), then a NULL instance (state), a machine that has taken a step (late) and
    a schedule that already holds events (state). An empty list is accepted and may be pushed again; once the schedule holds an event it is frozen. The numbers of the .def follow the decision text (port 1, state 2, bits 3, order 4, queue_full 5, late 6), not the check order"
  - "Script format: [script] format = 1, time_unit = \"master_clock\", events = N, then [ev_0001]..[ev_NNNN] with at (decimal, at most 15 digits, no leading zero), port (1 to 5) and held (an array of lowercase names in the canonical bit order i ii select run up right down left, [] for none). The flatten rows are SCRIPT NAME N and INPUT NAME IDX AT PORT MASK
    (the SCRIPT row is added to the plan's INPUT row so that an explicit empty script differs from a missing one); AT is carried as the exact decimal text"
  - "The case key input (an optional string key since plan 03-04) is the script's file stem; a missing rows file, an unknown script, a malformed row, a count mismatch or a script longer than the parse buffer (1100 events) is a harness fault of the case"
  - "pad_2btn timing: events at 0, 140000, 320000 and 510000 master clocks; the guest reads at about 37000 (checks 1 to 6), 228000, 415000 and 601000 (read stamps from the device log: 228156/228336/229224/229404, 414612/414792/415680/415860, 601068/601248/602136/602316). Under d6 = d7 = 1 a raw byte is $F0 | nibble, so every literal assumes the default binding
    and the cases carry request = \"\""

requirements-advanced: [INP-02, INP-04]
requirements-completed: [INP-01]  # INP-01: a 2-button pad through the SEL/CLR guest scan protocol on the I/O port; the guest observes input only through its own reads (pad_2btn_correct; E2 self-consistency, ER-5). INP-02 still needs the input-check stage and the [input] record table (03-28), INP-04 the IOPT and SCHD regions (03-29), INP-03 the none and wrong controls (03-29)

duration: 30min
completed: 2026-11-11
---

# Phase 3 Plan 27: Two-button pad, the input schedule and the input script path Summary

**The $1000 window is the two-button pad on the SEL and CLR protocol (a write is the latch of port O, a read is the byte of the pad on port 1 at the access stamp), a per-port input schedule of at most 1024 master-clock events is copied into the instance before the first step and re-validated by the core with six typed refusals, an input script goes through
the one parser (conformance-flatten --inputs and the new input-rules.awk) to a TSV the runner reads as --inputs, and pad_2btn passes against the correct script with 12 checks while storing every raw read as its guest-side pad-read log.**

## What was built

- **Task 1 (tracer), commit af4f964.** One script event (RUN held from time 0) reaches the guest only through a SEL=0 read.
  - `io.c`: `ds_io_poweron` (latch $FF, pad on port 1 from the profile field input_port1), `ds_io_write`, `ds_io_read` (the whole protocol, with d6 and d7 read from the profile fields the run request bound), `level_at` (the last event of the port at or before the stamp), `ds_input_push` (validate everything, then copy all 1024 slots), `ds_input_refusal_name`.
    `bus.c` routes the I/O window to them (the PSG window keeps io_buffer as before); `boot.c` calls `ds_io_poweron` after the power-on policy.
  - `ds_core.h`: `struct ds_input_event` (16 bytes), `DS_INPUT_MAX`, `DS_PORT_COUNT`, the schedule block and the port bytes of the instance with static asserts, `enum ds_slot_type`, `enum ds_input_refusal` from `input_refusals.def`.
  - `scripts/lib/input-rules.awk` (format, time_unit, events count and cap, table names in order, keys, at digits, port range, held names and canonical order, event order) and `conformance-flatten --inputs` (stage 1 the one parser, stage 2 the rules, a size cap of 64 KiB, INP_STAGE_DEAD when stage 2 prints no closing RULES row, nothing printed unless every
    script is clean). `conformance-run` flattens `tests/conformance/inputs` into `inputs.tsv` in run mode (so the sanitize mode too) and passes `--inputs` to the runner there, to every toy runner and, with the file in its input list, in `determinism-run`.
  - `inputkinds.c`: `--inputs` parsing, `input_events` (reads the SCRIPT and INPUT rows of the named script), `apply_input` (called by `guest_once` right after `apply_irq_stim`, before the first step; a refusal is a fault of the case). `pad_2btn.s` with two checks, its checks file, manifest and the case `pad_2btn_correct`.
  - `mmu_mpr` check 43 (the I/O window "has no pad behind it yet", open bus $5C) now reads $FF (latch $5C is SEL 0, CLR 0, nothing pressed, d6 = d7 = 1); the source, the checks row and the three manifests (256 KiB, 384 KiB, 1 MiB) were re-pinned. Check 42 (a write at the last byte of the window stores in io_buffer) still holds.
- **Task 2, commit 89019c1.** `pad_2btn` (DSV id 14) has 12 checks: 1 RUN held from time 0 reads $F7; 2 the power-on read before any write ($F0: latch $FF has CLR set); 3 a second read with no write is the first (GUEST); 4 SEL 1 with only RUN held reads $FF; 5 and 6 CLR 1 reads the nibble 0 with SEL 0 and SEL 1; 7 to 12 the buttons and directions of three
  further held sets (I+RIGHT, II+UP+LEFT, SELECT+RIGHT+DOWN), so each of the eight mask bits is read through its wire bit once. Every read goes through the write, the nine-cycle wait (five NOPs) and the read, and the raw byte is the observed byte of its check. The script has four events (0, 140000, 320000, 510000); the case runs 800000 master clocks.
- **Task 3, commit f3acb71.** The guest expect item `refused=input.NAME[,NAME...]` (kind guest, mode MODE_REFUSED_INPUT) and the cases `input_port_refusal` (ports 2 to 5, 0, 6, 257, 65537, the full word, first and last event of a list), `input_refusal_rows` (bits, order, queue_full, state, late: 27 refused calls, 92 assertions) and the deferred
  `input_refusal_coverage` (every member observed, names print as input.NAME, 1024 events and five ports). A valid list is first proven accepted, copied whole with every unused slot zero, frozen and read at the stamps of its events; each refused call compares the whole instance before and after.
- **Controls, commit e0a4763.** `tests/gate/flatten-input` (an ok script and five negatives: order, unknown button, event count mismatch, leading zero, format) and three run-time probes in the `flatten-controls` stage (a good and a bad script print not one byte, the ok script prints its SCRIPT and INPUT rows and END, an empty input-rules.awk in a scratch tree is INP_STAGE_DEAD).

## Verification

- Task verifies as written, all PASS: Task 1 `--stage flatten-controls`, `fixtures`, `conformance` (the tracer gate, auto mode, re-ran them before the expansion: `Tracer verified end-to-end - expanding`); Task 2 `--stage fixture-lint`, `fixtures`, `conformance`; Task 3 `--stage conformance`, `sanitizers` (ASan and UBSan over the new cases).
- Plan end (after the last task commit): `sh scripts/check --timing` PASS (2 min 19 s wall: conformance 23 s, sanitizers 42 s, determinism 16 s, named-controls 6 s, fixtures 3 s), `sh scripts/check --selftest` PASS (21 s), `sh scripts/audit-history --range 2cd52a0..HEAD` clean (4 commits, 46 blobs, 1427 blob-path pairs, run before the docs commit), `dash`
  runs of `--stage flatten-controls` and `--stage inventory` PASS. All ten toy controls stay RED_OK (named-controls PASS), `shared-state` included: run.c is unchanged.
- Bite shown with scratch edits of `src/core/io.c` (each restored from a saved copy and `cmp`-checked, nothing committed): CLR ignored turns pad_2btn_correct red at check 2, SEL inverted at check 1, a power-on latch of $00 at check 2, a pressed button reading 1 at check 1, RIGHT and DOWN swapped at check 8. For the refusal rows: no opposite-direction rule,
  an equal-time rule that ignores the port, no late check, no frozen-schedule check, a port rule that ignores the slot, a queue limit of 1025 and an unchecked reserved bit each turn input_refusal_rows or input_port_refusal red. With the script emptied or one held set changed the correct case turns guest_failure at check 1, and with no rows file it is
  harness_failure. A scratch edit of the order rule in input-rules.awk turns the flatten control `bad-order` red.
- Manual facts: the Hu7 CD BIOS manual (sha256 8e92ead8... equals 03-RESEARCH) and the hardware manual (40a32e5d...) were fetched or copied into a scratch directory outside the repo and read as page images: printed page 28 (PDF page 27, EX_JOYSNS: ONE bit 0, TWO 1, SELECT 2, RUN 3, UP 4, RIGHT 5, DOWN 6, LEFT 7, 1 = on) and H8-19 (port K is read, port O is
  written, by a cycle at physical $1FF000 to $1FF3FF; port O gives a high level after reset). H8-18 is cited from the earlier reads of the registry. Nothing was copied or committed.

## Deviations from Plan

**1. [Rule 3 - files outside the plan list] The plan's 17-file waiver is exceeded (40 files over four commits).** Added because the work needs them: `tests/conformance/inputkinds.c` (the input key reader and the refusal rows; guestkinds.c is already large and runner.c must not grow, F7), `CMakeLists.txt` (the new source), `src/core/boot.c` (the $FF latch and
the pad on port 1 are set at boot), `tests/conformance/runner.c` and `runner.h` (one option, one kind row, the declarations), `scripts/determinism-run` (the plan names the invocation), `scripts/check` and `tests/gate/flatten-input` (controls for the new flatten mode), and the three mmu_mpr manifests with the mmu_mpr source and checks (check 43). The plan
names "three images, three manifests" for the last item.
**2. [Plan wording] The flatten rows carry a script name and a SCRIPT row.** The plan says "INPUT idx at port mask rows"; there are several scripts, so the row is `INPUT NAME IDX AT PORT MASK`, and `SCRIPT NAME N` makes an explicit empty script (a no-input run, D-117) differ from a missing one.
**3. [Plan wording] Task 1 implements the whole read protocol, not only SEL=0.** A tracer is production quality and the full formula is shorter than a partial one; Task 2's work is the evidence (the checks, the script, the bite), not more core code. The literal "RUN held from time 0, SEL 0 CLR 0 gives $30 | d6/d7 | $07" is check 1.
**4. [Plan wording] The refusal items are one comma list.** The plan says "as separate items"; the guest expect allows one mode item per case, so `refused=input.bits,order,queue_full,state,late` names the codes and each code has its own variants and its own fresh boot. `input_port_refusal` is a second case, as planned.
**5. [Design] A refused list may still be pushed again after an empty one, but a non-empty schedule is frozen.** "State: not booted or already holds a frozen schedule" (the plan) has no stored frozen flag in the instance: a schedule is frozen when it holds an event; an empty list is a no-op and may be pushed again. A frozen schedule refuses even an empty list.
**6. [Scope] The checks of the checker are not complete.** `input-rules.awk` has the rules the plan lists plus the table, key and order rules it needs to be safe; opposite directions, a reserved bit, a port with no pad and the INP_ code for the 64 KiB cap are plan 03-28's (the core refuses the first three anyway, and the cap is INP_TOO_LARGE in flatten).

No Rule 4 deviation. The standing notes were not triggered: the shared-state toy stayed RED_OK (run.c unchanged), and no id was added to the run record (the [withheld] ids line holds 24 of 24; the plan names LED-IO ids only in comments and checks, which no def prints).

## Evidence discipline

E0 for the mechanism (schedule copy, refusals, row plumbing), E2 for everything the guest reads through the pad: the pad core and the pad fixture are written by one party (ER-5). Every literal of pad_2btn is classed ASSUMED (LED-IO-0002 for the nibble layout and pressed reads 0, LED-IO-0003 for CLR high and the power-on latch, LED-IO-0001 for d6 and d7) or
GUEST (check 3); what a manual page states is the logical mask bit of each button (CDBIOS-p28, re-read) and that port O is written and port K read at physical $1FF000 and drives a high level after reset (H8-18, H8-19). The wire nibble order, the SEL and CLR bits, CLR high reading 0, the $FF latch as CLR set and bits 4 and 5 reading 1 are community or code
evidence. **Disagreement carried into the ledger entry of the protocol (ER-8, plan 03-29): one report (pcetech.txt) orders the direction nibble differently, with RIGHT and DOWN swapped against the order used here; the core follows the order of D-115 and this plan does not decide it.** The nine-cycle wait is in every read of the fixture; the core has no
settling model, so a fixture without it is not labelled hardware-valid. LED-IO-0001 to LED-IO-0005 are written by plans 03-29 and 03-34; none was promoted (only plan 03-32 may) and none of the eight ER-2 candidates was touched. The script is a test input declared by the case, not a claim about any pad. No accuracy, pad-accurate, compatibility,
cycle-accurate, hardware-verified or boot claim; nothing pushed (D-64).

## Known Stubs

None added. The schedule and the port bytes are in the instance but not yet in the digest or any snapshot region (IOPT and SCHD are plan 03-29), so two instances that differ only in their schedule have the same digest until then; the record has no `[input]` table (plan 03-28). Ports 2 to 5 have no device (r1 populates port 1 only, D-116); multitap,
six-button, mouse and turbo are deferred (LED-IO-0006). Not added to `.planning/WINDOWS.md` (the ledger file is absent).

## Threat Flags

None beyond the plan's register. T-03-74 (input script parsing): one parser, schema rules, nothing printed unless every script is clean, a dead stage 2 is INP_STAGE_DEAD (control in flatten-controls), at at most 15 digits and kept as exact text; T-03-75 (oversized schedules): 1024 events and 64 KiB in flatten, the same 1024 in the core as input.queue_full, a
fixed-size instance array; T-03-76 (the core trusting the checker): ds_input_push judges port, bits, order and state itself, and the refusal rows bypass the script path entirely and compare the whole instance.

## Notes for next plans

- **03-28 (input-check, [input] table):** reuse `scripts/lib/input-rules.awk` as stage 2; it reads stage 1 records and prints `V code line detail`, `S n`, `E idx at port mask` and a closing `RULES n`. Its codes today: INP_FORMAT, INP_TIME_UNIT, INP_EVENTS_COUNT, INP_TOO_MANY, INP_TABLE_NAME, INP_KEY_UNKNOWN, INP_KEY_MISSING, INP_AT_NOT_DECIMAL,
  INP_AT_TOO_LONG, INP_AT_LEADING_ZERO, INP_ORDER, INP_PORT_RANGE, INP_HELD_UNKNOWN, INP_HELD_ORDER; flatten adds INP_TOO_LARGE (64 KiB), INP_STAGE_DEAD and BAD_FILENAME. Left for 03-28: opposite directions, a port with no pad, the reserved III to VI bits (a name such as iii is INP_HELD_UNKNOWN today); `held` is an array of lowercase names, so the mini-roots
  write `held = ["run"]`. The `[input]` table goes between `[stimulus]` and `[constants]` (03-23 note): add it to `seq_accept` in record-rules.awk and `build_expected_keys` in reckinds.c and bump DS_TEXT_FORMAT_VERSION. The plan's tag `# rule-NAME` is not on the statements yet. The `flatten-controls` stage already carries negatives for the flatten mode;
  `input-check-negatives` will carry the per-code set.
- **03-29 (controls, regions, ledger):** the hook for the runner kinds is one call: `apply_input(inst, k, &refusal)` next to `apply_irq_stim` in chunkkinds.c (line 144), sweepkinds.c (59), obskinds.c (761), digkinds.c (246) and reckinds.c (657); a refusal is a fault of the case. chunk_equiv applies only irq_stim today, so `chunk_equiv_pad_2btn_correct` needs
  that one call and `input = "pad_2btn_correct"` in the case; the observer kind does not apply input yet, and a sweep needs the same input as its plain case. Layout for the regions: `o_latch` then `slot_type[5]` are 6 contiguous bytes (IOPT); `input_ev[1024]` (16 bytes each, unused slots zero), `input_count` and `input_reserved` are 16392 contiguous bytes
  (SCHD), sitting between `irq_stim_level` and `timer_origin`; the explicit little-endian serialiser writes them field by field. The instance grew by 16400 bytes and 6 bytes. The none, wrong and late scripts are new files in tests/conformance/inputs; the flatten mode takes a directory, so no registration is needed. pad_2btn has no observe rows, so
  `fail-set=LIST` can name check ids directly; with no input at all check 1 fails ($FF where $F7 is expected), checks 7 to 12 fail, and checks 2 to 6 still pass. The device log of the run (`ds_runner --cases ... --trace-out pad_2btn_correct`) does not apply the input, so its values read $FF but its stamps are exact. The pcetech disagreement above belongs in
  the LED-IO-0002 disagreement column.
- **Ledger (03-29, 03-34):** LED-IO-0002 (protocol, nibble order, pressed 0; checks 1, 4 and 7 to 12), LED-IO-0003 (CLR high 0, bits 4 and 5, power-on latch $FF; checks 2, 5, 6; H8-18 and H8-19 are the page basis of "port O is high at reset", status stays assumed until a pinned test exists, R-5), LED-IO-0004 (no settling time, level at the read stamp,
  master-clock unit; the nine-cycle wait in the fixture), LED-IO-0005 (no turbo, opposite directions refused; input_refusal_rows). The pins are pad_2btn_correct and input_refusal_rows. mmu_mpr check 43 now carries ASSUMED:LED-IO-0002.
- **Instance layout (03-30):** the new members are `input_ev`, `input_count`, `input_reserved` (before `timer_origin`) and `o_latch`, `slot_type` (before `rom`); a restore must refuse a schedule with an unused slot that is not zero, a non-ascending time, a port outside 1 to 5 or without a pad, a mask with a bit above bit 7 or with opposite directions,
  `input_count` above 1024, `input_reserved` other than 0, a `slot_type` above 1 and an `o_latch` that is not a byte.
- **Cost:** the new cases are cheap (pad_2btn_correct runs 800000 master clocks, three policies; the refusal cases boot about a dozen images between them; the sanitizers stage is 42 s as before). CI: `.github/workflows/gate.yml` unchanged; nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation. `.planning/state.json` left untouched and uncommitted.

## Self-Check: PASSED

Created files present: src/core/io.c, src/core/input_refusals.def, scripts/lib/input-rules.awk, tests/conformance/inputkinds.c, tests/conformance/inputs/pad_2btn_correct.toml, fixtures/src/pad_2btn.s and pad_2btn.checks, fixtures/manifest/pad_2btn.toml, the cases pad_2btn_correct, input_port_refusal, input_refusal_rows and input_refusal_coverage, and
tests/gate/flatten-input. Commits af4f964, 89019c1, f3acb71 and e0a4763 are ancestors of HEAD.
