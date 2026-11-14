---
phase: 05-psg-raw-audio
scope: core and gate together (src/core PSG, observer sinks, snapshot regions, power-on v4; runner audio judge, sweep, --audio-out; audio-check lint; toys; ledger; README)
reviewed_range: f6afede..HEAD (HEAD 59d8eed; plans 05-01 to 05-08, 05-10 and 05-12; the last commit touches only 05-SECURITY.md)
reviewed: 2026-11-14T05:23:13Z
depth: deep
status: issues_found
findings:
  critical: 0
  warning: 7
  info: 9
  total: 16
  necessity: 1
files_reviewed: 77
files_reviewed_list:
  - src/core/psg.c
  - src/core/ds_psg.h
  - src/core/psg_effects.def
  - src/core/psg_vol.def
  - src/core/constants.def
  - src/core/obs.c
  - src/core/ds_obs.h
  - src/core/run.c
  - src/core/bus.c
  - src/core/cpu.c
  - src/core/boot.c
  - src/core/poweron.c
  - src/core/ds_poweron.h
  - src/core/snapshot.c
  - src/core/ds_snapshot.h
  - src/core/serial.c
  - src/core/ds_serial.h
  - src/core/ds_core.h
  - src/core/profile_fields.def
  - src/text/text.c
  - src/text/ds_text.h
  - scripts/audio-check
  - scripts/lib/aexp-lex.awk
  - scripts/lib/aexp-rules.awk
  - scripts/conformance-run
  - scripts/check
  - scripts/fixture-build
  - scripts/docs-check
  - scripts/lib/fixture-lint-rules.awk
  - scripts/lib/fixture-rules.awk
  - tests/conformance/audiokinds.c
  - tests/conformance/psgkinds.c
  - tests/conformance/stepkinds.c
  - tests/conformance/chunkkinds.c
  - tests/conformance/runner.c
  - tests/conformance/runner.h
  - tests/conformance/obskinds.c
  - tests/conformance/guestkinds.c
  - tests/conformance/AUDIO_TOY_EXEMPT
  - tests/conformance/cases/run_refuse_call_overlap.toml
  - tests/conformance/cases/snapshot_refusal_psg.toml
  - tests/conformance/cases/snapshot_refusal_psgt.toml
  - tests/conformance/cases/psg_restore_divisor.toml
  - tests/conformance/cases/audio_tone_sel0_d3.toml
  - tests/conformance/cases/audio_tone_pan01_twin.toml
  - tests/conformance/golden/accept_256k_zero.record.toml
  - tests/audio/RULES
  - tests/audio/KINDCLASS
  - tests/controls/psg-atten-max/RULE
  - tests/controls/psg-chsel-mask/RULE
  - tests/controls/psg-dda-no-reset/RULE
  - tests/controls/psg-emit-mutates/RULE
  - tests/controls/psg-fold-skipped-at-step-end/RULE
  - tests/controls/psg-noise-taps/RULE
  - tests/docs/claim-word-audio/README.md
  - tests/docs/claim-word-audio/EXPECT
  - fixtures/src/psg_tone.aexp
  - fixtures/src/psg_mod.aexp
  - fixtures/src/psg_tone.checks
  - fixtures/src/psg_mod.checks
  - ledger/REQUIRED
  - ledger/entries/LED-AUD-0001.toml
  - ledger/entries/LED-AUD-0002.toml
  - ledger/entries/LED-AUD-0003.toml
  - ledger/entries/LED-AUD-0004.toml
  - ledger/entries/LED-AUD-0005.toml
  - ledger/entries/LED-AUD-0006.toml
  - ledger/entries/LED-AUD-0007.toml
  - ledger/entries/LED-AUD-0008.toml
  - ledger/entries/LED-AUD-0009.toml
  - ledger/entries/LED-AUD-0010.toml
  - ledger/entries/LED-AUD-0011.toml
  - ledger/entries/LED-AUD-0012.toml
  - ledger/entries/LED-AUD-0013.toml
  - ledger/entries/LED-AUD-0014.toml
  - README.md
  - .github/workflows/gate.yml
---

# Phase 5: Code Review Report (PSG raw audio, core and gate)

**Reviewed:** 2026-11-14
**Range:** f6afede..HEAD (HEAD 59d8eed)
**Depth:** deep (bus write -> fold -> sample loop -> DSPE/DSPA sinks -> step-end rebase; restore rules; v4 power-on; judge, sweep, lint, toys, --audio-out; claims)
**Files Reviewed:** 77 (the core and runner sources in full; cases, controls and ledger entries where the findings or the focus list pointed)
**Status:** issues_found (0 critical, 7 warning, 9 info)

## Summary

The PSG core is sound as code. I found no out-of-bounds access, no undefined behaviour, no signed arithmetic, no float, no global state and no chunking defect:
- Every wave access is `sel * 32 + widx` with `sel <= 5` (R2..R7 at select 6/7 are dropped first, psg.c:251) and `widx <= 31` (masked on every store; restore refuses more than 31).
- `fold_to` never stores a distance above `t + q`: offsets are bounded by the largest step (600) plus the largest distance. The rebase leaves every running distance at 1..q.
- The sample loop is bounded by `lim = span / 192 + 1`, and `cursor * 192 <= stamp` cannot wrap.
- The mix stays below 6 x 31 x 32768. `side_att` is at most 91, and VOL is read only below 30.
- The restore decoder runs PSG0 before PSGT and judges the tick grid only once `r >= 1` is known.

The defects are in the evidence machinery and the claims around the code:
- **WR-01:** the audio fault sweep counts a harness outcome as a kill. The video sweep separates the two; audio does not.
- **WR-02:** restore accepts PSGT distances that the current model cannot reach: the coupled bound for select 1 (no LFO exists) and the d = 6 bounds at d = 3.
- **WR-03:** two constants rows (`psg_tick_phase_mclk`, `psg_sample_phase_mclk`) are printed in every run record and named in ledger statements, but nothing reads or pins them.
- **WR-04:** the `--audio-out` rights check judges the case's `fixture` key but boots the image named by the expect's `rom=`.
- **WR-05, WR-06:** a supported E1 statement (LED-AUD-0008) is broader than both its evidence and the model, and three assumed entries describe LFO behaviour the core does not have.
- **WR-07:** the D-218 docs control cannot detect the loss of any single one of the four new withheld words.

Experiments (outside the repository; nothing in it was modified):
- In a scratch copy of `scripts/`, `band-limited|sounds correct|authentic sound` was removed from the claim-word regex of docs-check. `docs-check --root tests/docs/claim-word-audio --expect DOCS_CLAIM_WORD` still printed EXPECT_OK. Only removing all four new words turned the control red (WR-07).
- A scratch awk pass computed the P8-10 sum of every windowed MAN row of both `.aexp` files. Every row is at S = 0 or S = 4, apart from the atten_order rows at S = 1 and 2, which LED-AUD-0008's strict-decrease clause covers (IN-01, WR-05).
- No full gate was run, as instructed.

## Structural Findings (fallow)

None provided.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: The audio fault sweep counts a harness outcome as a kill (the video sweep does not)

**Files:**
- `tests/conformance/audiokinds.c:2644`: `if (au_eval_group(f, gi, &x, af == 0u, why, sizeof why) != AU_PASS) *flips |= 1u << af;`
- `tests/conformance/audiokinds.c:2742-2745`: the declared kills are checked against `flips`.
- `tests/conformance/audiokinds.c:1212-1215`: an empty window returns AU_HARNESS.
- The contrast is `tests/conformance/videokinds.c:2794-2808`. There only VG_FAIL counts as flipped, and VG_HARNESS is counted apart and reported.

**Issue:**
- `au_kill_set` treats AU_HARNESS like AU_FAIL. A fault can make a row malformed instead of making its judged property fail, and that still shows the declared kill as observed.
- Example: AF8 or AF9 moves a start anchor (`au_find` then finds the next matching record), so `from` can pass a fixed `to=ev:...` end anchor. The window becomes empty, the row returns AU_HARNESS, and the group "flips".
- In the same way, a noise_seq row whose R7 record a fault removes or moves can return AU_HARNESS ("ordinal past the bits").
- D-209 requires that every declared kill flips its group through the judged comparison. The phase's own lesson, in the video sweep, keeps the harness case out of the flipped count.
- For the committed groups I traced (d_hold and d_hold1 under AF8), the fault gives a genuine AU_FAIL. The hole is latent, but nothing guards it.

**Fix:**
```c
int v = au_eval_group(f, gi, &x, af == 0u, why, sizeof why);
if (v == AU_FAIL) {
    *flips |= 1u << af;
} else if (v == AU_HARNESS) {
    *harness |= 1u << af;   /* report: a declared kill that only breaks the row is vacuous */
}
```
In `kind_audio_fault_sweep`, a declared kill whose bit is set in `harness` (and not in `flips`) should fail with a named reason. `audio_judge_forms` should add a row that pins this.

### WR-02: Restore accepts PSGT distances the current model cannot reach

**Files:**
- `src/core/snapshot.c:949`: `rem[k] > (k == 1u ? DS_PSG_COUPLED_REM_MAX : DS_PSG_CHAN_REM_MAX)`.
- `src/core/snapshot.c:956`: `nrem[k] > DS_PSG_NOISE_REM_MAX`.
- `tests/conformance/cases/snapshot_refusal_psgt.toml` (`p-rem1max=accept`) and `tests/conformance/snapkinds.c:2044`.
- `src/core/ds_psg.h:10-11`.
- The tighter bound is asserted in `tests/conformance/psgkinds.c:594`: `r > 6u * 4096u` for every channel.

**Issue:**
- The decoder accepts a select-1 distance up to 6,291,456, the LFO-coupled bound. The core never couples select 1: there is no LFO model, `chan_period` is "uncoupled for every channel" (psg.c:83), and plan 05-09 was cut to Phase 5.1 (D-217, D-234).
- No run can produce a select-1 distance above 24,576. Even after 5.1, it could exceed that only while LF CTL != 0.
- The decoder also applies the d = 6 maxima (24,576 and 11,904) whatever the restoring instance's d. At d = 3 the reachable maxima are 12,288 and 5,952.
- A crafted snapshot therefore restores a state that no run reaches. For example, select 1 holds its wave index for up to 6.29 M clocks and then resumes, and that state is re-serialised as valid.
- The fold test proves the 24,576 bound for every channel, so the decoder is looser than the invariant the gate itself asserts.
- No crash and no UB: the arithmetic stays inside u32. This is the Phase 4 L-02 class, "restore accepts an unreachable state", but snapshots are a named security boundary.

**Fix:**
```c
uint32_t chan_max = d * 4096u;                 /* DS_PSG_CHAN_REM_MAX at d = 6 */
uint32_t noise_max = d * 64u * 31u;            /* DS_PSG_NOISE_REM_MAX at d = 6 */
... rem[k] > chan_max ...                      /* select 1 too, until the LFO couples it (05.1) */
... nrem[k] > noise_max ...
```
Then change `p-rem1max` to `p-rem1over24576=field`, add d = 3 rows, and correct the ds_psg.h and snapshot.c comments. Alternatively, record the over-acceptance as a deliberate forward-compatibility choice in D-203 and in the case comment.

### WR-03: `psg_tick_phase_mclk` and `psg_sample_phase_mclk` are constants rows that nothing reads or pins

**Files:**
- `src/core/constants.def:81-82`.
- `tests/conformance/golden/accept_256k_zero.record.toml:224-225`: both rows are printed in every run record.
- `ledger/entries/LED-AUD-0012.toml:6` and `ledger/entries/LED-AUD-0014.toml:6`: both statements name the rows.
- The hard-coded phase 0 is in `src/core/psg.c:237` (`stamp % d`), `src/core/psg.c:156` (`cursor * 192`) and `src/core/obs.c:342` (`first = ceil(T0 / 192)`).

**Issue:**
- `grep -rn "tick_phase\|sample_phase" src tests scripts` finds no consumer. The core implements phase 0 by construction.
- For other conventions that psg.c implements only one way, the file pins the value: `_Static_assert(DS_CONST_psg_select_67_drop == 1 && DS_CONST_psg_keyon_widx_step == 0, ...)` at psg.c:240, and `DS_CONST_reset_cost_mclk == 0` at psg.c:22. These two rows have no such pin.
- Suppose someone edits either row to a non-zero value. The build passes, every run record prints the new value beside its ledger id, and the model is unchanged. The record, which is the evidence artefact, would then misstate the model.

**Fix:**
```c
_Static_assert(DS_CONST_psg_tick_phase_mclk == 0 && DS_CONST_psg_sample_phase_mclk == 0,
               "psg.c and obs.c implement only phase 0 of the tick and sample grids (LED-AUD-0012, LED-AUD-0014)");
```
Add it to psg.c, or implement the phases.

### WR-04: The `--audio-out` rights check judges the `fixture` key but boots the expect's `rom=` (inherited from `--video-out`)

**Files:**
- `tests/conformance/audiokinds.c:4194`: `output_fixture_permitted(k->val[K_FIXTURE])`.
- `tests/conformance/audiokinds.c:4325-4328`: `au_parse(k->val[K_EXPECT], &e)` and then `boot_built_rom(e.rom, ...)`.
- The same shape is in `tests/conformance/videokinds.c:3683` and `:3812`.

**Issue:**
- The rule of T-05-16 and F-85 is that streams are written only for an authored image listed in `build/gate/fixtures-authored.used` or a synthetic recipe.
- The check reads the case's `fixture` key, but the image that runs and whose streams are written is `build/gate/fixtures/<e.rom>.pce`.
- Nothing ties the two together. In audio cases the `fixture` key is used only by this check (it is the only `K_FIXTURE` reference in audiokinds.c), and no lint compares it with `rom=`.
- So a case with `fixture = "authored:psg-tone"` and `expect = "rom=<any image placed in build/gate/fixtures>;..."` passes the check and writes that image's streams.
- The id and path rules are sound (`[a-z0-9_]{1,64}`, the `rom=` charset is `[a-z0-9-]`, no traversal). The gap is which image the permission covers.

**Fix:** in `ao_request`, after parsing the expect, require that `rom=` names the same image as the fixture key:
```c
char want[64];
snprintf(want, sizeof want, "authored:%s", e.rom);
if (!output_fixture_permitted(want) || strcmp(want, k->val[K_FIXTURE]) != 0) return AO_NOT_PERMITTED;
```
Apply the same change to `vo_request`. Optionally add an inventory lint: in every audio and video case, `rom=` equals the authored fixture name.

### WR-05: LED-AUD-0008 (supported, E1) claims a strict decrease for every one-step decrease, which contradicts its own silence clause and goes past its evidence

**Files:**
- `ledger/entries/LED-AUD-0008.toml:11` (statement), `:6-9` (ER-2 and ER-22 record), `:19` (assumption).
- `README.md:46` (listed as supported).
- `fixtures/src/psg_tone.aexp`: groups ao_r1, ao_al and ao_lal.

**Issue:**
- The statement says: "its level as strictly decreasing per one-step decrease of the R1, R4 and R5 fields, equal to the manual's stated rules". No range limits that clause.
- The same statement makes a side silent when the P8-10 sum is 16 or more. A one-step decrease from S = 16 to S = 17 (for example R1 $77 to $66 at AL $1F, R5 $77) takes the level from 0 to 0. That is not strictly decreasing, in the statement or in the model (att >= 30 contributes 0, psg.c:211-217).
- In the other direction, a strict decrease between, say, S = 12 and S = 13 asserts that both sides are audible. That is the unstated converse that ER-16 keeps out of MAN rows (the statement's own record says "nothing is said of an audible side").
- The covering evidence judges S = 0, 1 and 2 only (my scratch scan of the MAN rows).
- A supported entry whose literal wording the model does not satisfy is the claim-discipline class the Core Value forbids.

**Fix:** narrow the clause, for example: "and its level as strictly decreasing per one-step decrease of the R1, R4 and R5 fields while the P8-10 sum stays at most 4". That matches the audible range of the P8-10 example and the evidence. Re-run the ER-2 record. Alternatively, keep a wider bound only with MAN rows at those sums and an argument that the pages state audibility there.

### WR-06: Three assumed entries say "DuoSpool models" an LFO that the core does not implement, and the core cites a plan that no longer exists

**Files:**
- `ledger/entries/LED-AUD-0006.toml:6`: "... and an LFO-coupled select-1 step of 6 x F2 x FLF".
- `ledger/entries/LED-AUD-0007.toml:6`: "FLF = 0 as 256 ... and the modulated select-0 value masked to 12 bits with 0 read as 4096".
- `ledger/entries/LED-AUD-0011.toml:6`: "modulation applies while LF CTL != 0 ..." (the whole statement), and `:14`: "Until plan 05-09 ...".
- `src/core/psg.c:83`, `src/core/psg.c:330` and `src/core/psg_effects.def:17` ("until plan 05-09").
- `src/core/ds_psg.h:10` ("once the LFO couples it, 05-09").

**Issue:**
- The core stores R8 and R9 and logs UNMODELLED. It never couples select 1 or modulates select 0 (psg.c:83-89, 317-332). The README says so ("LFO modulation is Phase 5.1", README.md:38).
- The three ledger statements still describe coupling and modulation as present behaviour.
- Their status is assumed, not supported, but a ledger statement is the project's record of what the model does. A reader of LED-AUD-0011 cannot tell that none of it runs.
- Plan 05-09 was cut. The LFO moved to 05.1-01 (D-217, D-234), and the KINDCLASS and RULES entries already say `05.1-01` and `lfo-in-5.1`. These references were not updated.

**Fix:**
- Split each LFO clause out of the statement into the assumption field as "planned for Phase 5.1 (05.1-01); not modelled in this build". For LED-AUD-0011, restate the entry as the register-side behaviour that exists: storage, the TRG freeze and index reset, and the UNMODELLED bit.
- Replace "05-09" with "05.1-01" in psg.c, psg_effects.def and ds_psg.h.

### WR-07: The D-218 docs control cannot detect the loss of any one new withheld word

**Files:**
- `scripts/docs-check:126` (the regex).
- `scripts/docs-check:176-179` (`--expect` compares the set of codes).
- `tests/docs/claim-word-audio/README.md:5` and `EXPECT` (DOCS_CLAIM_WORD).

**Issue:**
- The control's one line holds two of the four new words ("plays music" and "band-limited"). "sounds correct" and "authentic sound" never appear in it.
- Every hit gives the same code, and the expect compares code sets.
- Experiment: with `band-limited|sounds correct|authentic sound` deleted from a scratch copy of the regex, the control still printed EXPECT_OK. Only deleting all four new words turned it red.
- D-218 asks for these words "with its negative control". As built, the control shows that the mechanism works, not that each word is withheld.

**Fix:** use one control per new word (claim-word-audio-band-limited, -sounds-correct, -plays-music, -authentic-sound), each a mini-root with exactly one of them. Alternatively, add a per-word selftest that deletes one alternative from a scratch regex and requires its own control to turn red. Apply the same treatment to the multi-word D-181 items.

## Info

### IN-01: The ER-16 audible-row lint covers only live, period and cycle_order

**File:** `scripts/lib/aexp-rules.awk:86,416-424`
**Issue:**
- `aud3` limits the S = 0 or S = 4 rule (and the silenced-input AEXP_ARITH check) to three kinds.
- Other MAN kinds also need an audible side: two_level, run_grid, noise_ratio, same, dda_hold, dda_order and dda_ram_kept (KINDCLASS gives all of them a MAN ceiling).
- Today every such row sits at S = 0 (scratch scan), so nothing is wrong now. A future MAN two_level row at S = 10 would pass the lint.
**Fix:** extend `aud3` to every MAN kind that reads a non-zero side, or list the exceptions explicitly. Add a tests/audio mini-root for one of the new kinds.

### IN-02: Three PSG constants rows can break the core with no build error

**Files:** `src/core/psg.c:19`, `src/core/psg.c:56`, `src/core/psg.c:98,206`
**Issue:**
- `psg_noise_nf0_ticks` is bounded only above. A row value of 0 makes `noise_period` return 0 for NFRQ $1F, and the noise loop in `fold_to` never terminates. A guest writing R7 $1F would hang the core.
- `psg_lfsr_reset = 0` locks both LFSRs at 0 (`lfsr_step(0) == 0`). The restore decoder refuses LFSR 0, so every snapshot taken after time 0 would be refused.
- `psg_noise_high_level` is not masked or asserted to 31 or less, while the mix bound at psg.c:98 assumes level 31 or less.
**Fix:** add `_Static_assert(DS_CONST_psg_noise_nf0_ticks >= 1 && DS_CONST_psg_lfsr_reset != 0 && (DS_CONST_psg_lfsr_reset & ~0x3FFFF) == 0 && DS_CONST_psg_noise_high_level <= 31, ...)`.

### IN-03: The 600 in the fold-offset assertion is not tied to `ds_max_step_mclk()`

**File:** `src/core/psg.c:20`
**Issue:** the literal 600 stands for the largest step (D-221), but no static assertion or case checks `ds_max_step_mclk() <= 600`. If the opcode table or the penalty grew, the stated proof would drift silently. The 2^23 bound leaves wide headroom, so nothing breaks today.
**Fix:** add one row to an existing C-level case: `check(c, ds_max_step_mclk() <= 600u, ...)`, or name the constant once.

### IN-04: Impossible-state guards: lagging stamps in `chan_start` and in the sample loop

**Files:** `src/core/psg.c:235`, `src/core/psg.c:160`
**Category:** necessity
**Issue:**
- `stamp >= inst->machine_time ? stamp - inst->machine_time + 1u : 0u` and `si >= base_t ? si - base_t + 1u : 0u` guard a stamp or sample instant below the first unfolded instant.
- On the write path the stamp is `bus->start + idx * cycle_mclk`, with `bus->start` the step start, or the machine time for a NULL bus (bus.c:97-104). `ds_psg_advance` has already returned for `stamp < machine_time` (psg.c:146).
- Inside `ds_run`, the cursor starts at `ceil(T0 / 192)` (obs.c:342) and is advanced only past emitted instants.
- If the `chan_start` branch were ever taken, it would schedule the next event against the wrong base, off the tick grid. That is worse than refusing.
- This is judgment-level because ds_psg_write is reachable from test code with arbitrary stamps.
**Fix:** state the invariant in a comment, and make the fallback fail closed (for example, freeze the channel: `rem = 0`) instead of computing a value from a mismatched base.

### IN-05: psg_fold's "one jump" is folded in pieces of at most 192 clocks

**Files:** `tests/conformance/psgkinds.c:513-514,693-698`, `src/core/psg.c:151-165`
**Issue:**
- Both the "one jump" run and the partition runs carry a pcm channel, so `ds_psg_advance` calls `fold_to` at every sample instant.
- The reference is therefore never a long-span fold, as the case comment says it is. It is a no-rebase run sliced every 192 clocks.
- The comparison still catches rebase and partition defects. The sink-less long-span fold (up to 600 clocks per step) is compared only by the state digests of aud_noninterference.
**Fix:** run the one jump with no pcm channel (and compare samples through a second, sampled copy), or correct the case comment.

### IN-06: RULES coverage is per kind, not per clause

**File:** `scripts/lib/aexp-rules.awk:564-574`
**Issue:**
- A clause with status row is covered by any group of a listed kind, in any scene of any file, with a kill other than AF0.
- For example, aud05-d (LED-AUD-0005: chON 0 silences the channel) and aud08-a, -b and -c are all satisfied by any one silent group.
- A deleted scene could leave a clause "covered" by an unrelated group.
**Fix:** let a RULES row name its group (`need=scene:gid`), or record the coarseness as a limit in tests/audio/RULES.

### IN-07: `--audio-out` truncates and follows existing files, and leaves partial output on failure

**File:** `tests/conformance/audiokinds.c:4230,4285,4338`
**Issue:**
- `fopen(path, "wb")` truncates an existing `build/gate/audio/ID.*` and follows a symlink there.
- If the `.dspe` write succeeds and the `.dspa` or `.tsv` write fails, the first file stays while the runner exits 2.
- `audio_out_check` creates the directory fresh and fails earlier, so only direct runner use is affected (the `--video-out` shape).
**Fix:** write to `ID.ext.tmp` with `O_CREAT|O_EXCL`, rename after all three succeed, and remove the temporaries on failure.

### IN-08: Stale comments and a duplicated README sentence

**Files:**
- `src/core/run.c:10`: "(no producer until plan 05-04)". Producers exist since 05-04.
- `README.md:35-36` ("The PSG entries listed as supported below are E1 documentary: they equal the cited PSG manual page") and `README.md:51-52` ("For the PSG, E1 documentary: the entries the README lists as supported equal the cited PSG manual page") state the same sentence twice.
- The 05-09 references are covered in WR-06.
**Fix:** drop the parenthesis in run.c and keep one README sentence.

### IN-09: LED-AUD-0008 is listed as supported in the README and as withheld in every run record

**Files:** `README.md:46`, `tests/conformance/golden/accept_256k_zero.record.toml:98,103` (`[withheld]` `id_4 = "LED-AUD-0008"`), `src/text/text.c:379-380`
**Issue:**
- `emit_withheld` adds the ledger id of every constants row. `psg_silence_att` carries LED-AUD-0008, so the supported entry appears under `[withheld]`.
- This is the pattern already present for LED-VID-0014 (`id_39`, through `vdc_dma_dots_per_word`). It is not new in kind, but a reader of a record sees a supported id as withheld.
**Fix:** document that `[withheld]` means "ids that bound this run" (not "unsupported"), or filter supported ids into a separate row.

## Checked and found sound (no finding)

- **Fold arithmetic (psg.c):**
  - `t = stamp - machine_time + 1` is at most the step span.
  - The closed form `steps = (t - o) / q + 1` with `q >= d >= 3` (F = 0 is read as 4096, which the static assertion pins).
  - The stored `o + steps * q` is at most `t + q`, below 2^32.
  - The noise loop advances by at least `d * 32`, takes select 4 first on a tie and leaves both distances above `t`.
  - The step-end rebase leaves 1..q, so after every step the stored distance is 1..24,576 or 1..11,904 at d = 6.
- **Key-on:** `(w - w mod d) + d * Fe` as an offset from B is at least 2 and strictly after the stamp, and on the grid. The DDA 1 -> 0 and TRG 1 -> 0 restarts are gated by constants that are 1, and their `was`/`dda_was` logic gives no running-but-frozen state.
- **Tie order:** events at an instant, then the sample, then the write (the write path folds to its stamp inclusive before `ds_psg_write`). psg_fold pins the R6 write at E - 1, E and E + 1, and a key-on at a sample instant.
- **LFSR:** `lfsr_step` never reaches 0 from a non-zero state. The RF-6 output mask $30C01 follows from the taps (static assertion), and I re-derived it from `post`.
- **Synthesis:** `side_att <= 91`, VOL is read only below 30, the sum is at most 6,094,848, and `ds_psg_sample` is const (the emit-mutates toy is the cast-away control).
- **DSPE (bus.c):** 24 bytes, little-endian, built byte by byte. One record per window-2 write, exactly where a dev record is logged (both require `bus != NULL`). Undecoded writes carry chan $FF, effect NO_TARGET, wave_idx $FF and post $FFFF.
- **DSPA and the header (obs.c):** `first = ceil(T0 / 192)` without forming `T0 + 191`. The 32-byte header is written at the start and the end. Drop-newest uses the sticky rule through `chan_slot`, and `lost` saturates. `first(i+1) = first(i) + seq(i)` holds by construction across calls, because each call starts its cursor at `ceil(T0 / 192)` and the previous call emitted up to `floor((T0 - 1) / 192)`.
- **IN-03 overlap:**
  - A request or result that overlaps the instance is refused before any sink is read.
  - `finish` never writes a result inside the instance, `ds_obs_overlaps_instance` treats NULL as no overlap, and the sink records and structs are checked against the call memory and each other.
  - run_refuse_call_overlap checks both rows for byte identity and runs the corrected call.
- **Snapshot PSG0, PSGW, PSGT:** the masks come from the one `ds_psg_reg_mask` table. Restore refuses a non-zero widx under DDA, a pending event on a frozen channel, a missing event on a running one, LFSR 0 or above $3FFFF, and events off the grid of the restoring d. Serial order equals decode order, and pass 2 cannot fail.
- **seeded-prng-v4:** an independent PSGW____ stream after the unchanged v3 domains, each byte & $1F. Zero and v1 to v3 leave the wave RAM 0. The policy range in `dec_powr`, the name, the version 4 and the profile field set are all updated.
- **Runner `.aexp` reader:**
  - `ax_split` stores at most 8 fields into `fld[8]`, and every row kind checks its exact field count before indexing.
  - Lines are capped at 400 characters and rows at 512.
  - `au_num` accepts at most 16 hex digits or 19 decimal digits, so it cannot overflow.
  - Every `memcpy` is preceded by a length check below the destination size.
  - Every window index is checked against `fcount` before a sample is read.
  - The `rom=` charset is `[a-z0-9-]`, so no path is built from untrusted text.
- **audio-check:** the 49,152-byte cap is checked before parsing (exit 2), with an exact-cap ok control and an oversize control. Both awk stages have their status tested and require a sentinel. Every tagged rule except rule-stage1-row (which, as its header says, has no file control) has a deletion selftest.
- **`--audio-out` paths:** the id is `[a-z0-9_]{1,64}`, the paths are built by the runner under `build/gate/audio/`, and stdout holds three lines. The wrapper recomputes both digests with a host tool against the case evidence, and checks five refusals for exit 2 with no new file.
- **Toys:** each of the six anchors occurs exactly once in its file (control-mutate requires that), and each EXPECT names only audio cases (F-72); the toys of psg.c each kill at least one case of an audio kind. The F-4 "must stay pass" partners are consistent with the scene literals I read.
- **Claims:** the README audio paragraph matches the D-218 wording and the Phase 5 withheld list. No withheld word is used without quotes. Hashes are labelled E0 everywhere. The supported list equals the ledger's supported set (LED-AUD-0005 and LED-AUD-0008 added).

---

_Reviewed: 2026-11-14T05:23:13Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
