---
phase: 04-vce-vdc-raw-video
plan: 03
subsystem: core-state-layout
tags: [layout, serial-bump, regions, snapshot, regeneration, w2]
status: complete

requires:
  - phase: 04-vce-vdc-raw-video
    provides: the vid sink and the inert fold skeleton (04-02), the H7/H6 cites and ledger entries (04-01)
provides:
  - seven regions VCE0, PALT, VDC0, BEAM, VDMA, VRAM, SATB appended after SCHD (DS_REGION_COUNT 21), DS_SERIAL_VERSION 8, DS_BEHAVIOUR_EPOCH 5, DS_TEXT_FORMAT_VERSION 7, each bumped once
  - the complete VCE and VDC state in struct ds_instance (after tail_pad, explicit vid_pad), in DS_INSTANCE_TILING, with live-span asserts in serial.c
  - seven pass-1/pass-2 snapshot decoders judging the D-166 domain rules except the timeline windows (04-04)
  - ds_vdc_poweron (rcr_counter 16384, frame_lines from the bound lines_per_frame, display_k 0xFFFF), model rows vd_irq_phase and rcr_origin, constants rows line_mclk and rcr_counter_poweron
  - regenerated digest literals, record goldens and hand tables (Tasks 1 and 2, one commit)
affects: [04-04, 04-05, 04-06, 04-08, 04-09, 04-18]

actuals:
  tokens: 60000
  tasks: 2
  commits: 1
plan_head_before: 24c8b99025b3d839cbf45cc2e803e02a55a0d978
plan_head_after: c4e50282ec7c46b45e0a3e86b1d89fd381ceed74

tech-stack:
  added: []
  patterns:
    - "A region of the Phase 4 block is one contiguous declaration group whose one-byte members stand where they restore the alignment of the next member, so the block has no implicit padding and the stream order of serial.c (written field by field) may differ from the struct order"
    - "Old digest literals are checked against shasum -a 256 of a stream dumped by a scratch copy of the previous tree; the new stream is that stream with the version byte changed and the new frames appended by a script that never calls the new core"

key-files:
  created: []
  modified:
    - src/core/ds_core.h
    - src/core/ds_serial.h
    - src/core/ds_snapshot.h
    - src/core/ds_obs.h
    - src/core/serial.c
    - src/core/snapshot.c
    - src/core/boot.c
    - src/core/beam.c
    - src/core/model_params.def
    - src/core/constants.def
    - src/text/ds_text.h
    - tests/conformance/snapkinds.c
    - tests/conformance/digkinds.c
    - tests/conformance/reckinds.c
    - tests/conformance/cases/digest_stream_prefix.toml
    - tests/conformance/cases/digest_stream_blkt.toml
    - tests/conformance/cases/digest_stream_input.toml
    - tests/conformance/cases/digest_stream_irq.toml
    - tests/conformance/cases/digest_stream_model.toml
    - tests/conformance/cases/digest_region.toml
    - tests/conformance/cases/record_skeleton_256k.toml
    - tests/conformance/cases/snapshot_equals_digest.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml

key-decisions:
  - "The struct order of the Phase 4 block is VDC0, BEAM, VDMA, VCE0, then PALT, VRAM, SATB, then vid_pad[6]; with the one-byte members placed as alignment fillers inside BEAM and VDMA the block needs no padding except the trailing six bytes (67,178 + 6 = 67,184)"
  - "Boolean members (burst, vd_pending, satb_pending, satb_done_valid) are judged 0 or 1 on restore, a small extension of the D-166 list"

requirements-advanced: [VID-05, VID-06]
requirements-completed: []  # data-only layout half; VID-05 and VID-06 complete in 04-16

duration: 150min
completed: 2026-11-11
---

# Phase 4 Plan 03: Complete state layout and the one bump (Tasks 1 and 2; the rest is plan 04-18) Summary

**The complete Phase 4 state layout (seven regions, 67,178 payload bytes), the single serial 7 to 8 / epoch 4 to 5 / text 6 to 7 bump, the restore decoders for the new regions and the whole regeneration cascade landed in one green commit; Tasks 3 (seeded-prng-v3) and 4 (restore refusal labels and hostile flips) were NOT started because the stop rule of the plan fired.**

## Status and the split proposal (stop rule)

The plan's stop_rule says: after the Task 1+2 commit, if context is at or above about 60 percent, stop and report a split. The executor judged its context to be at or past that point (the reading list plus the layout, decoder and regeneration work is large), committed Tasks 1 and 2, and stopped. Tasks 3 and 4 are untouched.

**Proposed split:** a new plan with the next spare number (04-18), `depends_on: ["04-03"]`, wave 2, holding Task 3 and Task 4 of this plan verbatim (their text, read_first, behavior and verify are unchanged and still correct). Its files are the plan's Task 3 and Task 4 file lists:
src/core/ds_poweron.h, src/core/poweron.c, src/core/profile_fields.def, src/core/snapshot.c (dec_powr only), tests/conformance/snapkinds.c, profkinds.c, guestkinds.c, powkinds.c, cases/snapshot_refusal_fields.toml, cases/poweron_v3_draw_order.toml, cases/poweron_v3_domain_independence.toml,
cases/poweron_v3_smoke.toml, cases/snapshot_refusal_vdc.toml, cases/snapshot_hostile.toml, tests/conformance/REQUIRED. It must keep the prohibitions: no second serial, epoch or text bump; v3 literals from an independent splitmix64 script, never from a run; policy 4 accepted by dec_powr only together
with the allowed set and DS_POLICY_SEEDED_V3. Plans 04-04 onward need seeded-prng-v3 only for the default guest policy list (Task 3), so 04-18 must run before 04-04 (or 04-04 must declare it as a dependency).

## What was built (Tasks 1 and 2, commit c4e5028)

- **Layout.** `struct ds_instance` gains, after `tail_pad` (that line is byte for byte unchanged): VDC0 (`vdc_reg[20]`, `vdc_rdbuf`, `vdc_ar`, `vdc_status`, `vdc_wlatch`), BEAM (`stage`, `v_state`, `burst`, `line_idx`, eight u16 members, then `line_dirty`, `sprite_pend`, `lat_div`, `vd_pending`,
  `vd_irq_phase`, `rcr_origin`), VDMA (`satb_pending`, `satb_done_valid`, `satb_done_at`, `dma_start`, `dma_done`, `dma_word_mclk`, `dma_state`), VCE0 (`vce_cr`, `vce_cta`), `vce_pal[1024]`, `vdc_vram[65536]`, `vdc_satb[512]` and the explicit `vid_pad[6]`. Member names are the D-165 names;
  `vd_irq_phase` and `rcr_origin` are flat members named as the model rows (F-10). All 47 members are in `DS_INSTANCE_TILING`; static asserts pin the block start (8-aligned, right after `tail_pad`), the end (`vid_pad` is last) and the live spans.
- **Sizes.** `sizeof(struct ds_instance)` 1,141,096 (was 1,073,912); the Phase 4 block is 67,184 bytes (67,178 of payload plus the 6-byte pad). Region payloads: VCE0 3, PALT 1,024, VDC0 45, BEAM 33, VDMA 25, VRAM 65,536, SATB 512. The snapshot is 92,255 bytes: 25,021 + 67,178 payload + 7 frame
  headers of 8 bytes, derived by hand and equal to `ds_snapshot_size()` (the `snapshot_hash` kind compares the hand figure with the build).
- **serial.c.** Seven region rows and writers in the D-165 stream order, explicit little-endian, one statement per field and `w_bytes` statements for the byte arrays (the drop-region anchor line is untouched); `R_COUNT == DS_REGION_COUNT` (21).
- **snapshot.c.** Seven decoders in `regions[]` order, each reading all fields into locals before committing or judging. Domain rules written: AR <= 31, status <= $3F, register masks of D-146 with R03 and R04 zero, cta and every palette entry <= $1FF, stage <= 2, v_state <= 3, v_togo <= 512 (F-1, no
  clamp), display_k < 512 inside VDW and exactly 0xFFFF elsewhere, rcr_counter <= $7FFF, frame_lines in {262, 263}, bg_y <= 511, mwr_active <= $FF, line_dirty <= $3F, sprite_pend <= 3, both model selections below their counts through `model_values`, dma_state <= 2, dma_done <= 65,536, dma_word_mclk
  in {8, 12, 16} whenever dma_state != 0, lat_div in {0, 2, 3, 4} with 0 only at line_idx 0, vd_off and rcr_off below 1365 or 0xFFFF, and line_idx <= time / 1365 + 2 judged in pass 1 only, before any multiplication. `dec_powr` is NOT changed (policy 4 stays refused until Task 3).
- **Boot and constants.** `ds_vdc_poweron` (beam.c) sets `rcr_counter = DS_CONST_rcr_counter_poweron`, `frame_lines` from the bound `lines_per_frame` (263 or else 262, F-48) and `display_k = 0xFFFF`; boot.c calls it after `ds_io_poweron`. `model_params.def` gains `vd_irq_phase` and `rcr_origin` (the
  cap of 4 is now exhausted); `constants.def` gains `line_mclk` 1365 (LED-VID-0001) and `rcr_counter_poweron` 16384 (LED-IRQ-0010); `DS_VID_MAX_DOTS == line_mclk / 2` is a static assert in ds_obs.h. Epoch 5, serial 8, text format 7.
- **Regeneration (D-165, Pitfall 1).** The scratch copy of the previous tree was patched only to dump each stream (outside the repository). For each of the five `digest_stream_*` cases and both goldens, `shasum -a 256` of the dumped version 7 stream equalled the committed old literal (all seven OLD
  OK). A script that never called the new core then set the version to 8 and appended the seven frames (zero except BEAM: rcr_counter 16384 = bytes 00 40 at payload offset 14, frame_lines 262 = 06 01 at 16, display_k 0xFFFF at 12); the 92,255-byte streams hash to the new literals under `shasum -a
  256`. Every case comment gained the method sentences; epoch, format, `[constants]`, `[model_*]` and `[withheld]` lines were written by hand from the def files.

## Layout review (D-165, done before the layout was written)

Walking D-138 to D-160 for non-register state that a next event or a latch reads, every item is a member above: next-event inputs `line_idx`, `stage`, `vd_pending`, `vd_off`, `rcr_off`, `lat_div`; vertical machine `v_state`, `v_togo`, `display_k`; RCR `rcr_counter` and the `rcr_origin` selection;
`vd_irq_phase`; per-frame latches `bg_y` (BYR counter), `mwr_active`, `burst`; record flags `line_dirty`, `sprite_pend`; SATB DMA `satb_pending`, `satb_done_valid`, `satb_done_at`; VRAM DMA `dma_state`, `dma_start`, `dma_word_mclk`, `dma_done` (SOUR, DESR and LENR are live registers in `vdc_reg`).
The per-line latched registers (HSR, HDR, BXR, CR bits, divider) are consumed in the same EV_LINE that latches them (R1), the frame number and line-in-frame are derived from `line_idx / frame_lines`, and the observer sequence is not machine state. No field was found missing.

## Red list as observed (before the regeneration)

After the core change and before any test edit the conformance stage failed 26 cases: the five `digest_stream_*`, `digest_region`, `snapshot_equals_digest`, `record_skeleton_256k`, `record_refusal_text` (the format and epoch literals in reckinds.c), the two record goldens, `chunk_equiv_random32` and
the whole snapshot family (`snapshot_hostile`, `snapshot_refusal_api`, `snapshot_refusal_fields`, `snapshot_refusal_rows`, `snapshot_refusal_schedule`, `snapshot_restore_block`, `_boundaries`, `_irq`, `_pad`, `_sampled`, `_set`, `_speed`, `_timer`): the last group, including `chunk_equiv_random32`
whose restore uses the same helpers, is red only because the hand-kept tables of snapkinds.c (version, frame count, frame list, size, tiling identity list) had not been updated; with them updated `snapshot_hostile` and `snapshot_refusal_fields` are green with no case-file edit, as the plan
predicted. `dev_log_records` stayed green (windows 0 and 1 are not logged yet). No case outside this set turned red, so no layout field was missed and no correction bump is needed.

## Verification

- Task 1 verify (`--stage build`, `--stage core-boundary`, `--selftest mutation-tiling-entry`, `--selftest mutation-restore-skip-field`): PASS after the Task 2 edits (the restore selftest needs the updated hand tables, which is why the plan commits both tasks together). Task 2 verify (`--stage
  conformance`, `record-lint`, `determinism`, RESULT greps for `digest_stream_prefix` and `record_golden_256k_zero`): PASS.
- Full `sh scripts/check`: PASS before the commit (zero FAIL lines). `sh scripts/audit-history --range 24c8b99..HEAD`: clean.
- `sh scripts/check --selftest`: PASS (full run after the commit). **Linux gate-gcc:** `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` PASS (gcc 13, cmake 3.28.3, `-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror`, linux/amd64, one-off local run, not a CI claim). The `dash` and second-awk runs are owed at the checkpoint and close-out.
- Evidence class E0 throughout: layout and repeatability; every digest literal comes from the independent stream script, no hardware, accuracy or compatibility claim.

## Deviations from Plan

1. **[Rule 3 - Blocking] digkinds.c payload checks for the seven new tags.** The `digest_stream` kind stops with a harness error on a tag it does not know, so beyond the `DK_CAP` and `DS_REGIONS_MAX` edits the kind gained one payload check per new tag, written with hand offsets (BEAM and VDMA
   offsets listed in the comments) against the instance members. Files unchanged against the plan's list.
2. **[Rule 3] src/core/ds_obs.h edited** for the `DS_VID_MAX_DOTS == line_mclk / 2` static assert (the plan names the assert but not its file); it is the file that defines `DS_VID_MAX_DOTS`.
3. **[Rule 2 - Missing validation] Four boolean rules added to the restore judgements** (`burst`, `vd_pending`, `satb_pending`, `satb_done_valid` each 0 or 1). They are flags by construction; D-166 does not list them. A later plan that gives one of them another value range must relax its decoder in the same commit.
4. **[Note] Struct order differs from the plan's "u64 first" wording.** Ordering by region with the one-byte members as alignment fillers is the only order found with no implicit padding inside the regions (the sizes are 1 or 5 modulo 8); the block is still followed by exactly one explicit pad, and `vid_pad` is listed in `sk_is_identity` like `tail_pad`.
5. **[Note] Stale prose corrected.** The `record_skeleton_256k` meaning said 89 rows; the record had 120 rows before this plan and has 131 now (8 model, 2 constants rows, 1 withheld id), which `reckinds.c` computes from the defs.
6. **[Note] Plan-listed files not touched yet:** profkinds.c, guestkinds.c, powkinds.c, snapshot_hostile.toml, snapshot_refusal_fields.toml, REQUIRED and the Task 3 and 4 sources belong to the unstarted tasks.

## Known Stubs

None added. `ds_vdc_advance` is still the inert body of 04-02 (recorded in the ledger of open windows by that plan); `ds_vdc_flag`, `ds_vdc_read`, `ds_vdc_write`, `ds_vce_read` and `ds_vce_write` are still declared and undefined.

## Threat Flags

None. T-04-08 (restore domain rules), T-04-09 (line_idx bound before multiplication), T-04-11 (independent literals) and T-04-12 (tiling, selftests, explicit pad) are mitigated in code; the refusal labels that prove T-04-08 and T-04-09 are Task 4 and not yet written.

## Notes for next plans

- The model-parameter cap of 4 is exhausted (`tma_zero`, `irq_i_sample`, `vd_irq_phase`, `rcr_origin`): Phase 5 cannot add a row without reopening D-113 (F-35).
- The three unwritten D-166 timeline rules (pending event time and `satb_done_at` windows, `dma_start <= machine_time` while running) land in 04-04 with the fold; until then `satb_done_at` and `dma_start` accept any value.
- Task 3 (04-18): add the three domains and `DS_POLICY_SEEDED_V3`; remember `streams=distinct` in powkinds.c iterates `DS_DOMAIN_COUNT`, which grows from 4 to 7, and `PD_OTHER` stays `DS_DOMAIN_COUNT`.
- Task 4 (04-18): `snapshot_hostile.toml` line 5 is exactly 400 characters; keep it under the cap if its meaning text is edited.
- Plans that read a literal from a stream (digest cases) can reuse the method: dump, shasum the old, patch version and frames by script. The scratch tooling lived outside the repository and is not committed.
- `.planning/state.json` was left untouched and uncommitted.

## Self-Check: PASSED (Tasks 1 and 2)

- FOUND: src/core/ds_core.h (vid_pad, vdc_reg), src/core/beam.c (ds_vdc_poweron), src/core/snapshot.c (dec_beam), tests/conformance/golden/accept_256k_zero.record.toml (format_version = 7).
- FOUND commit c4e5028 (an ancestor of HEAD).
