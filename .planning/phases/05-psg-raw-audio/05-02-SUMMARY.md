---
phase: 05-psg-raw-audio
plan: 02
subsystem: sink-plumbing-and-in-03
tags: [aud, psg, pcm, sinks, run-request, in-03, obs, w1]
status: complete

requires:
  - phase: 05-psg-raw-audio
    provides: 05-01 handbook (PR-1, PR-4), ledger LED-AUD-0013 and -0014
provides:
  - struct ds_psg_sink (dev shape) and struct ds_pcm_sink (32-byte header, 8-byte frames, DS_PCM_SINK_MAX_FRAMES 8388608) in ds_obs.h; request members psg and pcm appended last (56 -> 72 bytes on LP64)
  - ds_obs_begin(log, request, machine_time) with all seven callers; hooks ds_obs_psg and ds_obs_pcm (no producer); pcm header count, lost, seq, frame_bytes 8, version 1, format 1 (first and sample_mclk 0 until 05-04, PR-4)
  - src/core/ds_psg.h and the inert ds_psg_advance at the window-2 write (step_end 0) and directly after the VDC fold (step_end 1)
  - the five-channel sink table (insn, dev, vid, psg, pcm) and the IN-03 call-overlap refusal (step 0 of ds_obs_sinks_check; a result inside the instance is never written)
  - cases psg_chan_append, pcm_chan_append, run_refuse_call_overlap; amended run_refuse_struct_size, run_refuse_memory_invalid, run_refuse_sink_unsupported; OX_ROMS_MAX 48
affects: [05-03, 05-04, 05-05]

requirements-completed: []
requirements-advanced: [AUD-02]

actuals:
  tokens: 14900
  tasks: 2
  commits: 2
plan_head_before: 8cb666e9ba3274e5bc48b1320473b9c7a3327c5a
plan_head_after: 4b3e09dbe0eae610b700e9042fdb4e15a41c5277

tech-stack:
  added: []
  patterns:
    - "A refusal whose reporting struct is itself the bad memory reports by return value only: a result inside the instance is refused and never written, so the refused instance stays byte-identical"
    - "Refusal rows over five sinks are generated from an index table (rr_valid, rr_mem, rr_put, rr_copy) beside a zero-step acceptance of the all-valid base request, so each row's only defect is its own"

key-files:
  created:
    - src/core/ds_psg.h
    - src/core/psg.c
    - tests/conformance/cases/psg_chan_append.toml
    - tests/conformance/cases/pcm_chan_append.toml
    - tests/conformance/cases/run_refuse_call_overlap.toml
  modified:
    - src/core/ds_core.h
    - src/core/ds_obs.h
    - src/core/obs.c
    - src/core/run.c
    - src/core/bus.c
    - src/core/cpu.c
    - tests/conformance/obskinds.c
    - tests/conformance/vdckinds.c
    - tests/conformance/stepkinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - tests/conformance/cases/run_refuse_struct_size.toml
    - tests/conformance/cases/run_refuse_memory_invalid.toml
    - tests/conformance/cases/run_refuse_sink_unsupported.toml
    - tests/conformance/REQUIRED

key-decisions:
  - "05-02: IN-03 is step 0 of ds_obs_sinks_check (run.c calls it after struct_size and reserved), through one exported helper ds_obs_overlaps_instance in obs.c (no fourth ranges_overlap copy)"
  - "05-02: a result that overlaps the instance is never written by finish(): writing it would change the refused instance, so REFUSED as the return value is the only report (ds_core.h states it)"
  - "05-02: pcm reuses struct ds_obs_chan; frames have one size, so chan_slot's drop-newest rule is the F-2 sticky full rule (no separate flag)"

duration: about 50 min
completed: 2026-11-13
---

# Phase 5 Plan 02: Sink plumbing and IN-03 Summary

**Two caller-owned audio sinks (psg dev-shaped, pcm 8-byte frames with a 32-byte DSPA header) appended last to the run request (72 bytes on LP64), ds_obs_begin changed once to the request form, a five-channel refusal table, the IN-03 call-overlap refusal, and an inert ds_psg_advance at both fold points; every digest and golden unchanged.**

## Performance

- Duration: about 50 minutes, plus the end-of-plan gate runs
- Tasks: 2 of 2
- Files changed: 20 (git diff 8cb666e..4b3e09d, 933 insertions, 40 deletions)

## Accomplishments

- Task 1 (6eed657): ds_obs.h gains struct ds_psg_sink (DS_PSG_RECORD_BYTES 24, header 16, align 8, DS_SINK_MAX_RECORDS) and struct ds_pcm_sink (DS_PCM_FRAME_BYTES 8, header 32, DS_PCM_ALIGN 8, DS_PCM_SINK_MAX_FRAMES 8388608 with two _Static_asserts), two struct ds_obs_chan channels in the log, and the hooks ds_obs_psg and ds_obs_pcm.
  ds_core.h appends `struct ds_psg_sink *psg; struct ds_pcm_sink *pcm;` after vid (forward declarations beside the others); no struct ds_bus change (PR-1).
  ds_obs_begin(log, request, machine_time) zeroes every attached header (pcm also writes frame_bytes 8, version 1, format 1) and leaves machine_time unused with a PR-4 comment; ds_obs_end writes both new headers.
  run.c opens the log when insn, dev, vid, psg or pcm is set and passes the request and inst->machine_time. The seven callers (run.c, obskinds.c x5, vdckinds.c) build a zeroed request carrying only their own sink.
  ds_psg.h names the one fold and its two call sites and PR-1; psg.c holds the inert body.
  bus.c HW_PSG: `inst->io_buffer = value;` then `ds_psg_advance(inst, stamp, 0u, bus != NULL ? bus->obs : NULL);`, log_dev stays last, reads untouched, the stale "stub until Phase 5" prose rewritten. cpu.c: `ds_psg_advance(inst, start + (uint64_t)total - UINT64_C(1), 1u, log);` directly after the VDC fold line (grep count 1); nothing in sample_and_enter.
  New kinds psg_chan_append and pcm_chan_append (parts attached and append; capacities 0, 1, 4, 64; drop-newest whole records; lost = seq - count; poisoned header zeroed; pcm frames read back at hand-written byte offsets; first and sample_mclk asserted 0 with a PR-4 comment). The attached part asserts no count, so it holds before and after the 05-04 producer.
- Task 2 (4b3e09d): DS_OBS_SINKS 3 -> 5 with sink_ref_psg and sink_ref_pcm; order unchanged (structs memory_invalid, capacities sink_unsupported, ranges memory_invalid). Step 0 refuses a request or result overlapping the instance as memory_invalid. finish() never writes a result that overlaps the instance.
  stepkinds.c: the two-pointer old size (56 bytes on LP64) beside the one-pointer variant; 11 audio rows in sink_unsupported (9 oversized, 2 exact-limit NULL boundaries that are memory_invalid) and an accepted 64 MiB pcm boundary call;
  an accepted all-five-valid base request and 50 audio memory_invalid rows (per sink: 9 own rows and 4 against each of the 4 other sinks); run_refuse_call_overlap through the optional expect item rows=call_overlap (no new case key, F-68).
  OX_ROMS_MAX 40 -> 48 with "more than 48 distinct guest ROMs".

## Request size

struct ds_run_request: 72 bytes on LP64 (56 before; the one-pointer old size 64 and the two-pointer old size 56 are both refused as run.struct_size).

## R4 deletion proofs (scratch copy of the tree, rebuilt with the gate preset, cases run with the live build data)

| Mutation (one line or block) | Case | Result |
|---|---|---|
| none (baseline of the scratch harness) | run_refuse_call_overlap, run_refuse_sink_unsupported, run_refuse_memory_invalid, psg_chan_append, pcm_chan_append | all pass |
| IN-03 step 0 deleted from ds_obs_sinks_check | run_refuse_call_overlap | guest_failure: "a request inside the instance was not refused as run.memory_invalid with 0 steps (IN-03)" |
| pcm capacity limit removed (sink_ref_pcm limit = UINT32_MAX) | run_refuse_sink_unsupported | guest_failure: "a malformed run request was not refused" |
| psg channel not checked (sink_ref_psg given NULL) | run_refuse_memory_invalid, run_refuse_sink_unsupported | both guest_failure |
| pcm channel not checked (sink_ref_pcm given NULL) | run_refuse_memory_invalid | guest_failure |
| finish() writes a result inside the instance (guard removed) | run_refuse_call_overlap | guest_failure: "a refused call whose result lies inside the instance changed the instance bytes" |

## Gate results (end of plan, one at a time)

| Run | Result | Wall s | User CPU s | Load (1 min) |
|---|---|---|---|---|
| `sh scripts/check` at 4b3e09d | PASS, 57 stages | 313.7 | 264.3 | about 7.6 to 8.4 |
| `sh scripts/check --selftest` | SELFTEST RESULT PASS, 175 PASS lines, 0 FAIL | 59.5 | 25.1 | |
| `sh scripts/audit-history --range 8cb666e..HEAD` | clean (2 commits, 26 blobs, 2,081 blob-path pairs) | | | |
| `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` | LINUX_REPRO gate-gcc PASS (gcc 13.3.0, mawk 1.3.4, Ubuntu 24.04.5, strict C11 -Werror) | 49 | | |

Task verifies: Task 1 (core-boundary, conformance, the five RESULT greps and the cpu.c count) green before 6eed657; Task 2 (conformance, the six greps) green before 4b3e09d. digest_stream_prefix, record_golden_256k_zero and every golden pass unchanged.
Sizes: no new or grown file has a ROM-SIZE shape (obs.c 16,965, ds_obs.h 15,295, stepkinds.c 66,461, obskinds.c 122,807; none a multiple of 8,192 or 8,192 + 512).

## Live budget table (after 05-02)

| Budget | Value after 05-02 | Phase 5 target |
|---|---|---|
| Gate stages | 57 | 58 |
| Selftest PASS lines | 175 | >= 178 |
| Power-on policies | 4 | 5 |
| Constants rows / CROWS_MAX | 34 / 40 | 49 / 64 |
| Withheld ids | 35 | 42 |
| Record rows (skeleton / 16 events / cap) | 153 / 169 / 192 | 175 / 191 / 192 |
| Regions; serial / epoch / text | 21; 8 / 5 / 7 | 24; 9 / 6 / 8 |
| Snapshot / instance / request bytes | 92,255 / 1,141,096 / 72 (done) | 92,553 / 1,141,376 / 72 |
| OX_ROMS_MAX / guest ROMs | 48 (done) / 36 | 48 / 39 |
| Sink channels | 5 (done) | 5 |
| DSV ids | 1..26 | 27, 28, 29 |
| Toys | 26 | 33 |
| Ledger entries / REQUIRED lines | 75 / 75 | 75 / 75 |
| Conformance REQUIRED cases | 337 | about 379 to 394 |
| CI critical path | 562 s (4.1 run) | about 587 to 612 s predicted |
| Local warm gate user CPU | 264.3 s at load about 8 (baseline 270.0 s) | +10 to +20 percent; trigger (3) above 337.5 s |

## Task Commits

1. Task 1: sinks, request members, ds_obs_begin once, inert fold, sink-shape cases - `6eed657` (feat)
2. Task 2: five-channel table, IN-03, old-size rows, OX_ROMS_MAX 48 - `4b3e09d` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1/2 - IN-03 non-mutation] finish() never writes a result that overlaps the instance**
- Found during: Task 2. The plan asks for a result-overlap row with "memory_invalid and an unchanged instance", but finish() writes the refusal into the result, which then changes the instance it lies in.
- Fix: the refusal is step 0 of ds_obs_sinks_check as planned; finish() skips a result that overlaps the instance (this also covers an earlier struct_size, reserved or time_wrap refusal with such a result), and ds_core.h says so.
  Row 2 judges REFUSED by the return value and the byte-identical block; row 1 reads run.memory_invalid from an outside result. A deletion of the guard turns the case red (R4 table).
- Files: src/core/run.c, src/core/obs.c (helper ds_obs_overlaps_instance declared in ds_obs.h, used by both; no fourth ranges_overlap copy), src/core/ds_core.h. Commit 4b3e09d.

**2. [Rule 2 - acceptance witness] Each call-overlap row and the audio memory_invalid set carry an accepted control**
- After each call-overlap row the same call with the request or result outside the instance must be accepted and run; before the 50 audio rows the all-five-valid base request must be accepted (zero-step). So a row cannot pass through an unrelated refusal. Commit 4b3e09d.

**3. [Rule 3 - prose] IN-03 and five-sink prose in ds_core.h and ds_obs.h landed with Task 1**
- The header comments describe the final contract (IN-03 included) from the first commit; the code of IN-03 lands in Task 2. ds_core.h and ds_obs.h are in the plan's files_modified, but not in Task 2's file list, which changed them for the helper and the result rule.

**4. Commit trailer**: both commits end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, the model that wrote them, as the orchestrator allowed when the harness asks for its own trailer; no script checks it.

Requirement ticks: none in the plan commits; any AUD tick by the plan-complete step is reverted with a file-specific checkout of .planning/REQUIREMENTS.md (only 05-10 completes AUD ids).

## Known Stubs

- src/core/psg.c ds_psg_advance is inert by design (plan text: 05-03 fills it); ds_obs_psg and ds_obs_pcm have no producer until 05-04; the pcm header fields first and sample_mclk are 0 until 05-04 (PR-4). None blocks this plan's goal (plumbing with no producer).

## Threat Flags

None beyond the plan's threat model: T-05-04 (five-channel overlap rows), T-05-05 (IN-03), T-05-06 (pcm capacity cap) and T-05-07 (both old sizes) are each covered by a case that turns red when its rule is deleted (R4 table).

## Notes for next plans

- 05-03: ds_psg_advance(inst, stamp, step_end, obs) is called from bus.c (after io_buffer, before log_dev; insert ds_psg_write and ds_obs_psg between the fold and log_dev, D-188) and from cpu.c directly after the VDC fold; the cpu.c line is the future anchor of toy psg-fold-skipped-at-step-end (grep count 1). psg.c includes ds_core.h, ds_obs.h, ds_psg.h only.
- 05-04: ds_obs_begin already receives machine_time ((void) with a PR-4 comment): set the pcm cursor and header first = ceil(T0/192) and sample_mclk 192 in write_pheader (obs.c) and in ds_obs_begin. Then update the two PR-4 assertions of pcm_chan_append (obskinds.c ac_append: first and sample_mclk 0) and nothing else:
  the attached parts assert no count, and run_refuse_sink_unsupported's 64 MiB pcm acceptance checks only count, lost, seq, frame_bytes, version, format.
  The attached pcm sink holds 512 frames (about 209 needed for 40,000 master clocks); ds_obs_pcm is the frame hook (whole frames, chan_slot rule).
- run_refuse: rr_valid, rr_mem, rr_put, rr_copy and rr_five_request index the five sinks (0 insn .. 4 pcm) if a later plan adds rows.
- OX_ROMS_MAX is 48 for 39 guest ROMs after 05-09.

## Owner items

- None new. Carried: lever (a) commit 9619efe awaits the owner's yes or no at publish time (05-01).

## Self-Check: PASSED

Files: src/core/ds_psg.h, src/core/psg.c and the three new case files exist. Commits 6eed657 and 4b3e09d are ancestors of HEAD.
