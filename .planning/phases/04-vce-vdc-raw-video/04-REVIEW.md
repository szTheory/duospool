---
phase: 04-vce-vdc-raw-video
reviewed: 2026-11-12T00:00:00Z
reviewed_range: 925662a..HEAD (src/ and CMakeLists.txt)
depth: deep
files_reviewed: 24
files_reviewed_list:
  - CMakeLists.txt
  - src/core/beam.c
  - src/core/boot.c
  - src/core/bus.c
  - src/core/constants.def
  - src/core/cpu.c
  - src/core/ds_core.h
  - src/core/ds_obs.h
  - src/core/ds_poweron.h
  - src/core/ds_serial.h
  - src/core/ds_snapshot.h
  - src/core/ds_vdc.h
  - src/core/irq.c
  - src/core/model_params.def
  - src/core/obs.c
  - src/core/poweron.c
  - src/core/profile_fields.def
  - src/core/run.c
  - src/core/serial.c
  - src/core/snapshot.c
  - src/core/vce.c
  - src/core/vdc.c
  - src/core/vid.c
  - src/text/ds_text.h
severity:
  blocker: 0
  high: 1
  medium: 2
  low: 2
  info: 4
findings:
  critical: 0
  warning: 5
  info: 4
  total: 9
  necessity: 0
status: issues_found
---

# Phase 4: Code Review Report (core, VCE/VDC raw video)

**Reviewed:** 2026-11-12
**Range:** 925662a..HEAD, restricted to src/ and CMakeLists.txt
**Depth:** deep (cross-file: bus -> fold -> ladder -> DMA -> render -> sink; snapshot writer against decoders; power-on)
**Files Reviewed:** 24
**Status:** issues_found

## Summary

The fold (ds_vdc_advance), the line ladder, the 1365 grid, the sink and the render hold up under adversarial reading.
I traced the loop bound (3*(span/1365)+6), the stamp ordering and tie rule, the step-end fold at start+total-1, the sample fold and the DMA closed form, and found no defect in any of them.
The caller advances machine_time after the step-end fold, so span stays one step.
Register masks, the IW step, read-ahead, status clear-on-read, the enable gates, the IRQ1 OR and the seeded-prng-v3 domains all match D-141..D-167.
v1 and v2 are unchanged. The 64-byte record header matches D-161 byte for byte. The sink's drop-newest accounting cannot write past the caller buffer.

The defects are at the snapshot boundary and at one vertical-cycle edge:

- **High (H-01):** a guest can reach a legal state whose snapshot ds_restore refuses. It happens when the guest writes the LENR low byte while a transfer runs (dma_done goes past 65,536).
- **Medium (M-01):** restore accepts a frame length that disagrees with the bound profile.
- **Medium (M-02):** a vertical setup whose VDS and VDW states fill the frame exactly never gets a VBlank entry. That means no VD, no SATB copy and no MWR latch, ever.

Three findings (H-01, M-01, M-02) and L-01 were reproduced with small C programs. The programs were linked against a strict-C11 clang build of ds_core with ASan and UBSan, made outside the repository. That build compiled with no warnings (-std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Werror). gcc 13 is not installed on this host, so the gcc strict build is **unverified** by this review.

Severity mapping for the workflow frontmatter: high, medium and low count as warning; info counts as info; there is no blocker or critical. No core rule violation was found: no malloc, file I/O, clock, env or float in the new files, and every new table is static const. No mutable static or signed-overflow pattern was found.

## Narrative Findings (AI reviewer)

## High

### H-01: dma_done can go past 65,536 in a legal run, so the snapshot of a reachable state is refused (restore at every stop breaks)

**File:** `src/core/vdc.c:127-137` (closed form), `src/core/vdc.c:231-233` (LENR low byte stored live while running), `src/core/snapshot.c:727` (rule dma_done <= 65,536)
**Issue:**
- LENR is a live counter (D-151), and a LENR low-byte write while dma_state == 2 changes it without restarting. ds_vdc_dma_apply computes `remaining = LENR + 1` afresh and adds `n` to dma_done each time.
- So dma_done counts every word since the start, and nothing bounds it by the original LENR + 1.
- dec_vdma refuses any dma_done above 65,536. A legal guest therefore produces a state whose snapshot the core writes but cannot restore. That breaks the D-137 restore-at-every-stop property and the save-state contract.
- A guest that keeps raising the low byte keeps the transfer alive forever. dma_done then wraps at 2^32 words, the closed form sees `due - dma_done` as huge, and the rest of the transfer completes at once.

**Reproduced:**
- Boot with the zero policy (CR = 0, so the frame is burst). SOUR = DESR = 0, then LENR = $FFFF.
- Run 60,000 words: dma_done is 59,999 and LENR is $15A0.
- Write the LENR low byte = $FF: LENR is now $15FF.
- Run 5,600 words: dma_done is 65,600 and the transfer is still running.
- ds_snapshot returns ok; ds_restore of those bytes into a fresh boot returns snapshot.field.

**Fix:** Keep the progress bounded by construction: re-base the closed form whenever words are applied, so dma_done is always 0 between folds and the restore rule can require dma_done == 0.
```c
/* in ds_vdc_dma_apply, after the loop: */
inst->dma_start = inst->dma_start + n * (uint64_t)inst->dma_word_mclk;
inst->dma_done = 0u; /* progress is the distance from dma_start; never accumulates */
```
Re-basing also preserves chunk invariance, because the due count stays the same function of time. Alternatively, re-base only on a SOUR/DESR/LENR byte write while running. Add a vdc_snapshot_mid_dma row that writes the LENR low byte mid-transfer and restores past 65,536 words.

## Medium

### M-01: restore accepts a BEAM frame_lines that disagrees with the profile's bound lines_per_frame

**File:** `src/core/snapshot.c:679`, `src/core/beam.c:283`
**Issue:**
- frame_lines is the bound profile field, constant for the run (D-136), and ds_vdc_poweron derives it from `profile.field[lines_per_frame]`.
- dec_beam only checks that the value is 262 or 263. It never compares it with the instance's profile, unlike dec_powr, which refuses a policy that differs from the bound one (DS_SNAPREF_profile).
- dec_prof only proves that the profile hash matches, and frame_lines is not part of that hash.
- A crafted or corrupted snapshot can therefore make a 262-line profile run 263-line frames. The run record still says 262, and every video claim is tied to the profile.

**Reproduced:** booted with lines_per_frame = 262, ran 300 lines, snapshotted, set the BEAM frame_lines bytes to 263 and restored. The result was `restore=ok`, `inst->frame_lines=263`, profile value `262`.
**Fix:** In dec_beam pass 1, compute the expected value from `inst->profile.field[DS_FIELD_lines_per_frame]` exactly as ds_vdc_poweron does (one shared helper). Return DS_SNAPREF_profile when it differs, then drop the bare {262, 263} test.

### M-02: when VDS and VDW fill the frame exactly, there is never a VBlank entry (no VD, no SATB copy, no MWR latch)

**File:** `src/core/beam.c:115-118` (the wrap returns 0 before the VDW-completed check at line 134)
**Issue:**
- vertical_step tests the frame wrap first and returns 0, so whatever ended on the last line of the frame is ignored.
- When (VDS+2) + (VDW+1) == frame_lines, VDW finishes on the frame's last line. Its "first line after VDW" (D-143) is frame line 0, which the wrap claims for VDS.
- The result: VD is never raised, ds_vdc_satb_copy never runs (a pending DVSSR waits forever, and DSR never repeats), and mwr_active never latches.
- D-139 withholds VD only when "the frame wrap cuts VDW". Here VDW is complete, not cut. One register value separates "VD every frame" (sum 261) from "VD never" (sum 262), so a guest at that boundary hangs waiting for VBlank.

**Reproduced:** VPR VDS = 20, VDW = 239 (sum 262), CR = $08, DCR = $01, DVSSR MSB written. The status read 00 in frames 2 to 4 and satb_pending stayed armed. With VDS = 19 (sum 261), the status read $20 in every frame.
**Fix:** Decide the boundary explicitly and record it in LED-IRQ-0001 / LED-VID-0005. If VBlank entry is "the line after the last VDW line", let the wrap line report it:
```c
if (inst->frame_lines != 0u && k % (uint64_t)inst->frame_lines == UINT64_C(0)) {
    int vdw_done = prev == (uint32_t)VS_VDW && inst->v_togo == 0u;
    enter_state(inst, (uint32_t)VS_VDS, state_len(inst, (uint32_t)VS_VDS));
    return vdw_done; /* VDW completed on the last line: this line is the VBlank state entry */
}
```
Pin the decision with one fixture row at sum = frame_lines.

## Low

### L-01: restore accepts beam combinations that no run can reach, including events out of time order

**File:** `src/core/snapshot.c:685-701`, `src/core/beam.c:70-80` (next_stage assumes vd_off < rcr_off), `src/core/beam.c:168-169`
**Issue:** dec_beam checks vd_off and rcr_off separately (each < 1365 or none) and checks only the time of the current stage. It accepts the following, none of which a live run can produce:
- stage VD with rcr_off < vd_off. The RCR event then runs after the VD event although its time is earlier.
- vd_pending = 1 and stage VD while vd_irq_phase is line_start. VD is raised at EV_VD under a model that never schedules one.
- offsets that do not equal (HDS+HDW+2)*8*lat_div or (HDS+1)*8*lat_div.

No crash or unbounded loop follows, because the next fold drains the event. But the comment at beam.c:169 ("the stage order VD then RCR is also the time order") is an invariant the restore does not enforce.

**Reproduced:** stage = 1, vd_pending = 1, vd_off = 1300, rcr_off = 10 at line 300 was accepted (`restore=ok`). The fold then evaluated RR at a time 1,290 clocks before the VD it had just raised.
**Fix:** In pass 1, require vd_pending == 0 when vd_irq_phase == 0, and vd_off < rcr_off when both exist. Better: recompute both offsets from the restored HSR, HDR and lat_div and require equality.

### L-02: the restore rules restate core constants by hand, so a constant change silently makes legal snapshots unrestorable

**File:** `src/core/snapshot.c:558` (second copy of the D-146 mask table; the first is `vdc.c:33`), `src/core/snapshot.c:731` (8/12/16), `src/core/snapshot.c:735` (4096)
**Issue:**
- dma_word_mclk is `DS_CONST_vdc_dma_dots_per_word * divider` and satb_done_at - t is `256 * dots_per_word * divider`, but the decoder hard-codes 8, 12, 16 and 4096.
- If constants.def changes vdc_dma_dots_per_word (or a vce_div row), the core still compiles and every static assert holds. Every snapshot taken mid-transfer or between a SATB copy and DS is then refused.
- The two mask tables can drift apart: a mask widened in vdc.c makes restore refuse legal register values.

**Fix:** Expose the mask table and the divider table from one place (a const array in a private header, or accessors such as ds_vdc_reg_mask(i) and ds_vce_divider(cr)). Derive the allowed word times as `DS_CONST_vdc_dma_dots_per_word * ds_vce_divider(c)` for c in 0..3, and the SATB window as `256 * dots_per_word * max divider`. A `_Static_assert` can tie the maximum to 4096 if the literal stays.

## Info

### I-01: unreachable zero-divider branches in the DMA start, and a literal chosen only to satisfy the restore rule

**File:** `src/core/vdc.c:153-154`, `src/core/beam.c:196`
**Issue:** dma_begin runs only after the bus has folded to the access stamp. Any fold to a stamp >= 0 processes the EV_LINE of line 0 at time 0 (pending_time of line_idx 0 is 0), which latches a non-zero divider.
So `div == 0` cannot occur at dma_begin, and `lat_div != 0` in dma_line is always true. The fallback `div != 0u ? div : 2u` exists only so that a pending transfer carries a word time the restore rule accepts (comment at vdc.c:146). This is dead code (Code Quality), not a necessity finding.
**Fix:** Drop the branch and assert `lat_div != 0` in a debug path, or document it as a restore-rule shim.

### I-02: line_sample_offset_mclk shifts EV_LINE but not the EV_VD and EV_RCR it schedules

**File:** `src/core/beam.c:49` against `src/core/beam.c:55-66`
**Issue:** EV_LINE is at origin + line_sample_offset_mclk, but EV_VD and EV_RCR are at origin + off. With the constant at 0 nothing happens. A value above 15 (the smallest vd_off is 16) would put an EV_VD before the EV_LINE that schedules it, and the ladder order of D-138 would break. Latent, because the row is a constants.def value.
**Fix:** Add the offset to the VD and RCR base too, or `_Static_assert(DS_CONST_line_sample_offset_mclk == 0)` until a decision changes it.

### I-03: counters that are narrower than the values they count

**File:** `src/core/obs.c:342-354` (lost saturates at UINT32_MAX while seq is u64), `src/core/vid.c:174` (frame stored as u32)
**Issue:** The vid header documents lost = seq - count. That breaks once more than 2^32 records are lost in one call: about 1.6e4 hours of emulated time with a small sink. The record's frame field wraps after 2^32 frames. Both are latent, but they contradict the documented invariant.
**Fix:** Document the saturation in ds_obs.h, or widen lost in the header's reserved u64.

### I-04: stale plan references after the D-180 cut

**File:** `src/core/vid.c:10`, `src/core/vid.c:205`, `src/core/vdc.c:16`
**Issue:** The comments say sprites arrive in plan 04-14 and OR/CR in plan 04-15. Both moved to Phase 4.1 (commit 29edb6e). A reader looking for the sprite stage will look in the wrong place.
**Fix:** Reword them to "Phase 4.1".

## Checked and found sound (no finding)

- **Fold:**
  - Inclusive tie rule (an event at time <= stamp runs first).
  - Monotone on lower stamps, DMA and DS included.
  - The bound covers the worst legal case (3q + 5 events).
  - The sample fold runs before ds_irq_pending.
  - The $1403 read folds; $1402 does not.
  - A NULL bus folds to machine_time with no log.
- **Ladder:**
  - Order is vertical, RCR, bg_y, divider, dma_line, VBlank, offsets, render, clear.
  - vd_off is below rcr_off whenever rcr_off exists.
  - EV_RCR is none at offsets of 1365 or more.
  - No (ax+aw)*div can equal 1365 (odd), so the clip and the RCR "none" rule agree.
- **Arithmetic:** offsets are at most 8192 in u32, and line_origin is guarded against u64 overflow in ds_vdc_pending_time.
- **DMA:**
  - Closed form, words capped by LENR+1, cancel at display line 0 with no DV.
  - DV and DS are gated at the fold instant; no access can fall in between.
  - dma_word_mclk is guarded against 0.
- **Sink:**
  - room = capacity - stored with stored <= capacity, so there is no write past the buffer.
  - Sticky full flag; seq is counted on every offer.
  - Header offsets +0, +4, +8, +16, +24.
  - The render is const, and the gate is `obs != NULL && obs->vid.on`.
  - The record header offsets are asserted and every byte up to rec_bytes is written.
- **Registers:**
  - Masks, AR $14-$1F ignored, R03 and R04 forced to 0.
  - VWR latch, MAWR step on a dropped store, VRR read-ahead with the old buffer returned.
  - Status clears bits 0-5 and BSY is constant; the IRQ1 level is ORed with the stimulus.
  - VCE CTA steps after the high byte; reads of unused registers return $FF; the high-byte read is ORed with $FE.
- **Restore:**
  - Every index, divisor and loop bound used by the fold is judged in pass 1: AR, status, masks, CTA and palette odd bytes, stage, v_state, v_togo <= 512, display_k, lat_div, the fold window and the DMA windows.
  - Pass 2 commits only after pass 1 accepted every byte.
- **Power-on:**
  - v3 is v2 plus three independent splitmix64 domains in the order VRAM, SATB, PAL, with odd palette bytes masked to bit 0.
  - v1 and v2 paths are unchanged; every register and beam byte is 0 except rcr_counter, frame_lines and display_k.
- **CMakeLists:** the toy object libraries get the same global flags; the extra definitions DS_BUILD_ID and DS_ALLOC_SHIM are not read by any core source.

---

_Reviewed: 2026-11-12_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
