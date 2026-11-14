---
phase: 05-psg-raw-audio
fixed_at: 2026-11-14T06:23:29Z
review_path: .planning/phases/05-psg-raw-audio/05-REVIEW.md
iteration: 1
findings_in_scope: 16
fixed: 10
skipped: 6
status: partial
---

# Phase 5: Code Review Fix Report (PSG raw audio: core and gate)

**Fixed at:** 2026-11-14
**Source review:** 05-REVIEW.md (WR-01 to WR-07, IN-01 to IN-09)
**Iteration:** 1
**Scope (orchestrator brief):** all seven warnings. Among the info items: the pure comment and documentation corrections (the stale 05-09 references, the duplicated README sentence, stale comments), and IN-03 only if the tie is trivial and changes no behaviour. Every other info item is skipped with a reason.

The status is `partial` only because the brief keeps six info items out of scope. Every warning is fixed.

**Summary:**
- Findings considered: 16 (7 warning, 9 info).
- Fixed: 10.
  - WR-01 to WR-07.
  - IN-03: the tie is a single assertion row.
  - IN-05: the comment correction the review offers.
  - IN-08.
  - The 05-09 references named in WR-06 and IN-08 are fixed under WR-02 (ds_psg.h) and WR-06 (psg.c, psg_effects.def).
- Skipped: 6 (IN-01, IN-02, IN-04, IN-06, IN-07, IN-09). Each is a code, lint or semantic change outside the brief; reasons are below.
- Reviewer statements found wrong: one. The review called the WR-01 hole latent. It was not: one committed declared kill (AF8 on d_hold1) was vacuous.
- Deviations: three.
  - The WR-04 pin rows live inside allow_case instead of a new expect item.
  - The WR-04 gcc follow-up is a second commit.
  - The commit trailer.
- No snapshot layout, serial version, epoch, text format or golden changed.

Commits (all on the current branch, no push):
- `70d8cd3` fix(05): WR-01 the audio fault sweep keeps a harness outcome apart from a kill
- `c3a49a9` fix(05): WR-02 restore refuses PSG distances the restoring d cannot reach
- `97fa045` fix(05): WR-03 pin the PSG tick and sample phases to 0 at compile time
- `3b7d98e` fix(05): WR-04 the --audio-out and --video-out rights check covers the image that runs
- `5d5d6f1` fix(05): WR-05 narrow LED-AUD-0008's strict-decrease clause to sums of at most 4
- `215c4cb` fix(05): WR-06 the LFO entries say the LFO is decoded and logged, not modelled, until Phase 5.1
- `d88c0c3` docs(05.1): WR-06 note that 05.1-01 returns the planned LFO clauses to the ledger
- `a3f6825` fix(05): WR-07 one docs control per new D-218 withheld word
- `caedaa8` docs(05): IN-08 drop the repeated README PSG sentence and a stale run.c comment
- `5608c22` fix(05): IN-03 tie the fold bound's 600 to ds_max_step_mclk() in psg_fold
- `beb478e` docs(05): IN-05 psg_fold's one jump is a no-rebase fold sliced every 192 clocks
- `4c89b15` fix(05): WR-04 shorten the swap-row message so gcc 13 sees no truncation
- docs(05): review fix report (this file)

| Finding | Status | Commit | Evidence |
|---------|--------|--------|----------|
| WR-01 sweep counts a harness outcome as a kill | fixed | 70d8cd3 | audio_judge_forms: 4 new rows; mutant (harness counted as a flip) red on 2 rows; all 9 audio_fault_sweep cases pass |
| WR-02 restore accepts unreachable PSGT distances | fixed (tightened; requires human verification) | c3a49a9 | snapshot_refusal_psgt: p-rem1coupled refused; psg_restore_divisor: 6 d = 3 rows; mutant (old bounds) red on both |
| WR-03 phase rows nothing reads or pins | fixed | 97fa045 | each row set to 1 fails the build with the assert text |
| WR-04 rights check judges the fixture key, boots rom= | fixed (audio and video; deviation) | 3b7d98e, 4c89b15 | swap rows in allow_case; mutant (no rom= check) red on audio_out_rights and video_out_rights |
| WR-05 LED-AUD-0008 broader than evidence and model | fixed (narrowed, stays supported; requires human verification) | 5d5d6f1 | ER-2 and ER-22 re-run recorded in the entry; docs-claims, ledger-entries, audio-check PASS |
| WR-06 LFO "models" statements; 05-09 references | fixed | 215c4cb, d88c0c3 (ds_psg.h in c3a49a9) | no 05-09 left in src/core; three entries stay assumed |
| WR-07 D-218 control blind to one lost word | fixed | a3f6825 | scratch mutation: each deleted word turns exactly its own control red |
| IN-03 600 not tied to ds_max_step_mclk() | fixed (test row) | 5608c22 | psg_fold passes at <= 600, red at <= 599 (the value is exactly 600) |
| IN-05 psg_fold "one jump" comment | fixed (comment) | beb478e | comment and case text match the code |
| IN-08 stale run.c comment, duplicated README sentence | fixed | caedaa8 | docs-claims PASS |
| IN-01 ER-16 lint covers three kinds | skipped | - | lint change, outside the brief |
| IN-02 PSG constants rows can break the core | skipped | - | static asserts, not a comment fix; outside the brief |
| IN-04 lagging-stamp guards (necessity) | skipped | - | behaviour change and a necessity finding |
| IN-06 RULES coverage per kind | skipped | - | RULES grammar or recorded limit; outside the brief |
| IN-07 --audio-out file handling | skipped | - | I/O behaviour change; outside the brief |
| IN-09 supported id under [withheld] | skipped | - | needs a ruling on what [withheld] means (D-53); not a comment fix |

## Fixed Issues

### WR-01: The audio fault sweep counts a harness outcome as a kill

**Files modified:** `tests/conformance/audiokinds.c`, `fixtures/src/psg_mod.aexp`
**Commit:** 70d8cd3

**Applied fix:**
- `au_kill_set` sets a flip only for AU_FAIL. AU_HARNESS goes into a separate mask, with the reason of the first harness outcome on a declared kill.
- `au_sweep_verdict` returns AU_HARNESS when a declared kill gave only a harness outcome. `kind_audio_fault_sweep` then reports a harness error naming the group, the fault and the reason. This is the rule of videokinds.c:2794-2808.
- The detail lines print `harness=` beside `observed=`.
- `audio_judge_forms` gains `aj_sweep_harness` on the synthetic capture.
  - The group is silent, from key-on + 393,216 to the key-off + 384,000.
  - AF8 drops the key-on record, so the start anchor names the key-off and the window is empty: a harness outcome, not a kill.
  - AF6 fails the comparison: a kill.
  - Declaring AF6 and AF8 makes the sweep a harness failure; declaring AF6 alone passes.
  - A scratch mutant that counts harness as a flip turns two of these rows red.

**The review was wrong that the hole was latent.** After the fix, `audio_fault_sweep_mod_dda` reported a harness failure on group d_hold1.
- AF8 drops its R4 $DF record, and the start anchor moves to the next R4 $DF, past the fixed end anchor. The window is empty.
- The declared AF8 kill had only ever been observed as a harness outcome.
- AF8 is dropped from d_hold1's kills, with a note on its comment line. AF0, AF5 and AF6 still fail the group's comparison, and AF2, AF4 and AF7 also flip it.
- d_hold (MAN), which the review traced, is unaffected.
- The mod_dda sweep now prints groups=10 declared=33 flipped=33, with d_hold1 harness=AF8,AF9 (both undeclared). No other committed sweep has a declared harness kill.

### WR-02: Restore accepts PSGT distances the current model cannot reach

**Files modified:** `src/core/snapshot.c`, `src/core/ds_psg.h`, `tests/conformance/snapkinds.c`, `tests/conformance/psgkinds.c`, `tests/conformance/cases/snapshot_refusal_psgt.toml`, `tests/conformance/cases/snapshot_refusal_psg.toml`, `tests/conformance/cases/psg_restore_divisor.toml`, `.planning/phases/05.1-psg-lfo/05.1-CONTEXT.md`
**Commit:** c3a49a9
**Status:** fixed, requires human verification (a restore rule, a logic change)

**Decision: tighten (no named limit).** The brief made the choice depend on whether tightening forces a snapshot version bump in Phase 5.1. It does not:
- The decoder's acceptance set is not part of the serial layout. Plan 04.1 L-02 (`e41f9e7`) changed a restore rule with no bump.
- 05.1-01 only widens the set again (Phase 5.1 accepts a superset of what Phase 5 accepts), so every Phase 5 snapshot stays restorable.
- A 5.1 snapshot with a coupled distance is refused, fail-closed, by a Phase 5 build.
- Any epoch question from the LFO behaviour itself is the existing R11/C-08 hazard, which is unchanged.

**The model and the decisions agree with the tightening.**
- D-203 frames its ranges as reachable-state invariants (the 04.1 L-02 class). Its coupled bound 6,291,456 for select 1 is LFO behaviour, which moved to 05.1-01 under D-234 ("every 05-09 reference reads as 05.1-01").
- D-223 makes d the restoring instance's: a snapshot of the other d is refused by the profile identity first. So per-d bounds refuse no legitimate state.
- The step-end rebase leaves every running channel at 1..d x Fe and every noise counter at 1..d x 64 x NFc (psg.c fold_to and the rebase). psg_fold already asserts the d = 6 channel bound for every channel.

**Code:**
- `dec_psgt` bounds a running channel distance, select 1 included, by d x 4096 (24,576 at d = 6, 12,288 at d = 3).
- It bounds both noise distances by d x 64 x 31 (11,904 and 5,952). A static assert keeps the d = 6 constants divisible by 6.
- `DS_PSG_COUPLED_REM_MAX` stays: it still bounds the fold offsets in psg.c, and 05.1 needs it.

**Rows:**
- p-rem1max is now the largest on-grid 24,576 (accept), and p-rem1over the next on-grid value (field).
- The new p-rem1coupled, the former coupled maximum, is refused (field).
- psg_restore_divisor gains six d = 3 rows at time 0: 12,286 and 5,950 accepted; 12,289 and 5,953 refused; covering select 0, select 1 and noise.
- A scratch decoder with the old bounds turns snapshot_refusal_psgt red (p-rem1over, p-rem1coupled) and psg_restore_divisor red.

**Phase 5.1 consequence:** ruling C-09 in `05.1-CONTEXT.md`.
- 05.1-01 restores the coupled select-1 bound d x 4096 x 256 in `dec_psgt` with the coupled fold. It applies whatever the current coupling, since a pending coupled distance outlives a coupling change until its next reload.
- p-rem1coupled turns to accept, and the d = 3 select-1 row 12,289 turns to accept, each with a new refused row beyond it.
- snapshot.c and the two cases lie outside 05.1-01's files_modified; C-09 authorises them.
- 05.1-01 Task 3's restore stop between two coupled select-1 steps depends on this.

### WR-03: The tick and sample phase rows are read and pinned by nothing

**Files modified:** `src/core/psg.c`, `src/core/obs.c`
**Commit:** 97fa045
**Applied fix:**
- A static assert in psg.c pins `psg_tick_phase_mclk == 0 && psg_sample_phase_mclk == 0`.
- A second one in `ds_obs_begin` pins the sample phase where the first frame is computed.
- Scratch check: setting either row to 1 fails the build with the assert's message. No record or behaviour change.

### WR-04: The rights check judges the fixture key but boots rom=

**Files modified:** `tests/conformance/audiokinds.c`, `tests/conformance/videokinds.c`, `tests/conformance/cases/audio_out_rights.toml`, `tests/conformance/cases/video_out_rights.toml`
**Commits:** 3b7d98e, 4c89b15

**Applied fix:**
- The per-case checks move into `ao_case_check` and `vo_case_check`.
- These also require the expect's rom= to be the authored image the fixture key names, and listed. An expect with no parsable rom= is refused.
- The --video-out side was the same shape and got the same fix.
- Every committed audio and video case already has fixture = authored:ROM, so no allowed request changed.
- The refusals that conformance-run drives (a bad id, fixture none, a guest case, no case) return before the new check, so their exit 2 is unchanged.

**Deviation (pin location).** A new expect item (`deny_swap=ID:KEY`) pushed both expect lines past 400 characters. Dropping an existing item to make room would weaken a control, so instead:
- `allow_case` now also swaps each other `allow_fixture` key of the same expect into an in-memory copy of the case and requires a refusal. In practice that key is the always-permitted synthetic plain-256k.
- At least one swap must exist.
- A scratch mutant without the rom= check turns both rights cases red.
- The swap key is synthetic, not a second authored image. Both go through the same comparison.

**Follow-up (4c89b15).** The first Linux gcc 13 build failed with `-Wformat-truncation`: the swap message could reach 161 bytes in a 160-byte buffer. It is now at most 144 bytes. clang on the host did not warn.

### WR-05: LED-AUD-0008 claims a strict decrease for every step

**Files modified:** `ledger/entries/LED-AUD-0008.toml`, `tests/audio/RULES`, `README.md`
**Commit:** 5d5d6f1
**Status:** fixed, requires human verification (a claim change)

**Applied fix:**
- The strict-decrease clause now reads "per one-step decrease of an R1, R4 or R5 field that leaves the sum at most 4", with "sums 5 to 15 imply nothing".
- The limit 4 is the audible range of the P8-10 worked example, where ER-16 allows audible MAN rows.
- This removes the 16-to-17 contradiction (0 to 0) and the unstated audible converse at sums 5 to 14.

**Kept supported.** Checked against ER-2 and ER-22 after narrowing:
- ER-2 conditions 1 to 5 are unchanged (img rows, page-rule literals, "equal to the manual's stated rules", withheld list, TEST pin).
- ER-22: the same MAN guest groups cover the narrowed clause. These are ao_r1 (R1 $FF to $EF, sum 0 to 1), ao_al (AL $1F to $1E, sum 0) and ao_lal (sums 0, 1, 2) of audio_tone_atten.
- There is no in-page conflict there: DX-5 concerns the sum 15 only.
- The re-run is recorded in the entry's comments.

The README supported list is unchanged. The README gains a scope sentence for LED-AUD-0008, like the existing one for LED-AUD-0005. RULES aud08-d uses the new wording.

### WR-06: LFO "models" statements and 05-09 references

**Files modified:** `ledger/entries/LED-AUD-0006.toml`, `LED-AUD-0007.toml`, `LED-AUD-0011.toml`, `src/core/psg.c`, `src/core/psg_effects.def`, `.planning/phases/05.1-psg-lfo/05.1-CONTEXT.md` (ds_psg.h in c3a49a9)
**Commits:** 215c4cb, d88c0c3

**Applied fix:**
- LED-AUD-0006 states the channel and noise periods, with "the LFO-coupled select-1 step is not modelled until Phase 5.1".
- LED-AUD-0007 states F = 0 and NFc = 0. FLF = 0 as 256 and the select-0 wrap are now "held as constants but not modelled": `psg_lfo_f0_as` feeds only a static assert.
- LED-AUD-0011 states the register side that runs:
  - R8 and R9 (masked $83) are stored;
  - R9 with TRG 1 resets the select-1 index and freezes its counter until TRG 0;
  - R9 with CTL != 0 is logged with UNMODELLED, and select 0 stays unmodulated.
- Each planned clause moved to the assumption field or a comment, marked "planned for Phase 5.1 (05.1-01)". The RULES lines for them are already `limit lfo-in-5.1`.
- All three stay assumed (ER-14).
- psg.c and psg_effects.def name 05.1-01. The psg_effects.def rule text reaches no output.
- Ruling C-10 in 05.1-CONTEXT tells 05.1-01 to return the planned clauses when the coupled fold lands, so no statement turns false.

### WR-07: The D-218 docs control misses the loss of one word

**Files modified:** `scripts/docs-check` (a comment), new `tests/docs/claim-word-audio-band-limited`, `-sounds-correct`, `-plays-music`, `-authentic-sound`
**Commit:** a3f6825

**Applied fix:** four mini-roots, each holding exactly one of the four new words, run by docs-claims-negatives like every other docs control. The existing two-word control is kept unchanged.

Scratch mutation proof (not committed). Each word was deleted in turn from a copy of the regex:

| Deleted | claim-word-audio | -band-limited | -sounds-correct | -plays-music | -authentic-sound |
|---------|------------------|---------------|-----------------|--------------|------------------|
| band-limited | ok | RED | ok | ok | ok |
| sounds correct | ok | ok | RED | ok | ok |
| plays music | ok | ok | ok | RED | ok |
| authentic sound | ok | ok | ok | ok | RED |

The D-181 multi-word items were not split. The brief limited this fix to the D-218 words.

### IN-03: The 600 of the fold bound

**Files modified:** `tests/conformance/psgkinds.c`, `tests/conformance/cases/psg_fold.toml`, `src/core/psg.c` (a comment)
**Commit:** 5608c22
**Applied fix:**
- `ds_max_step_mclk()` is computed at run time from the opcode table, so no compile-time tie exists.
- psg_fold asserts `ds_max_step_mclk() <= 600`, and psg.c names the case beside the assert.
- The value is exactly 600: the row passes, and a scratch bound of 599 turns it red.

### IN-05: psg_fold's "one jump"

**Files modified:** `tests/conformance/psgkinds.c`, `tests/conformance/cases/psg_fold.toml`
**Commit:** beb478e
**Applied fix:** the review's comment-correction option.
- The comments now say the reference run is a no-rebase fold. Because it carries a pcm channel, it is folded at every 192-clock sample instant.
- They also say the sink-less whole-step fold is compared by the state digests of aud_noninterference (checked: sinks on against no sink, digest compared).
- Taken as a pure comment correction under the brief.

### IN-08: Stale comment and duplicated README sentence

**Files modified:** `README.md`, `src/core/run.c`
**Commit:** caedaa8
**Applied fix:**
- The audio paragraph keeps "The PSG entries listed as supported below are E1 documentary..."; the repeat at the end of the supported paragraph is gone. No gate rule pins the removed phrase.
- run.c says the psg and pcm sinks have been produced since plan 05-04.

## Skipped Issues

### IN-01: The ER-16 audible-row lint covers only live, period and cycle_order

**File:** `scripts/lib/aexp-rules.awk:86,416-424`
**Reason:** a lint change with a new mini-root, not a comment or documentation correction; outside the brief. Today every affected row sits at S = 0 (review scan), so nothing is wrong now.
**Original issue:** `aud3` limits the S = 0 or 4 rule to three kinds; a future MAN two_level row at S = 10 would pass the lint.

### IN-02: Three PSG constants rows can break the core with no build error

**File:** `src/core/psg.c:19,56,98,206`
**Reason:** static asserts on constants are code, not a comment correction; outside the brief. A cheap follow-up of the WR-03 shape.
**Original issue:** psg_noise_nf0_ticks 0, psg_lfsr_reset 0 or psg_noise_high_level above 31 would hang, lock or overflow the model with no build error.

### IN-04: Impossible-state guards in chan_start and the sample loop

**File:** `src/core/psg.c:235,160`
**Reason:** a necessity finding whose fix changes behaviour (fail closed instead of computing). The brief excludes it, and removing or changing such a fallback is a scope decision for the owner (#4100).
**Original issue:** the fallbacks schedule against a mismatched base instead of refusing.

### IN-06: RULES coverage is per kind, not per clause

**File:** `scripts/lib/aexp-rules.awk:564-574`
**Reason:** either a RULES grammar change (`need=scene:gid`) or a new recorded-limit line; neither corrects a stale text. Outside the brief.
**Original issue:** a clause is covered by any group of a listed kind, so a deleted scene could leave a clause "covered" by an unrelated group.

### IN-07: --audio-out truncates and follows existing files

**File:** `tests/conformance/audiokinds.c:4230,4285,4338`
**Reason:** a change of file I/O behaviour (O_EXCL temporaries and rename); outside the brief. It affects only direct runner use, the shape --video-out already has.
**Original issue:** fopen "wb" follows a symlink, and a partial failure leaves the first file behind.

### IN-09: LED-AUD-0008 is supported in the README and listed under [withheld]

**File:** `README.md:46`, `src/text/text.c:270-276,379-380`
**Reason:** text.c documents [withheld] under D-53 as "nothing in the record may read as support for any of them". So either the table's meaning changes (a ruling on D-53) or supported ids are filtered (a record format and golden change). Neither is a comment correction. The pattern predates Phase 5 (LED-VID-0014). Left for an owner or orchestrator ruling.
**Original issue:** a reader of a run record sees a supported id as withheld.

## Notes for the verifier and the owner

- **Evidence discipline.**
  - Every new row is E0: it shows a judge, decoder or rights rule able to fail. None is a claim.
  - No claim was widened. LED-AUD-0008 was narrowed, and the LFO entries were corrected to what runs.
  - ER-14 holds: LED-AUD-0006 and -0011 stay assumed. The supported set is unchanged (15 ids).
- **D-203 text.** The 05-CONTEXT text of D-203 still reads "(6,291,456 for i = 1)". It is read as 05.1-01's under D-234, and the ruling is recorded in 05.1-CONTEXT C-09; 05-CONTEXT was not edited. The orchestrator may prefer an erratum there.
- **05-SECURITY.md.** Row T-05-09 cites the old select-1 bound and old snapshot.c line numbers. It is an audited record and was not edited.
- **05.1-01 inherits two rulings.**
  - C-09: restore the coupled select-1 bound.
  - C-10: return the planned LFO clauses to the three ledger entries.
  - Both touch files outside its files_modified, and both are authorised in 05.1-CONTEXT.
- **Pre-existing long lines.** run.c:100 (443 characters) and snapkinds.c 622 and 2078 (418) predate this pass; no line this pass added is over 400. Historical quotes of LED-AUD-0008's old statement (05-08 and 05-10 SUMMARY, VALIDATION) were not rewritten.
- **One fixture data change.** fixtures/src/psg_mod.aexp (the d_hold1 kills). The ROM bytes, manifests and goldens are unchanged.

## Verification

All gates ran in the main checkout (`workflow.use_worktrees` is false, so no worktree was created).

- **Per commit:** narrow verifies.
  - WR-01: the judge-forms case and all 9 audio_fault_sweep cases, plus audio_hash_not_verdict and the psg_mod audio cases (by `ds_runner --only`), the stages fixture-build, audio-check and fixture-lint, and the mutant.
  - WR-02: the refusal, divisor, psg_fold and snapshot_restore_audio cases, core-boundary, the full conformance stage (116 s), and the mutant.
  - WR-03: the build and core-boundary, and the two compile-fail checks.
  - WR-04: both rights cases, a real --audio-out and --video-out run, and the mutant.
  - WR-05 and WR-06: ledger-entries, docs-claims, audio-check and build.
  - WR-07: docs-claims-negatives, docs-claims, licences, rights-tree, preflight, and the scratch mutation table.
  - IN-03, IN-05, IN-08: psg_fold, core-boundary, docs-claims and build.
- **No snapshot, format or golden change**, so no full gate was owed before any single commit.
- **At the end (HEAD 4c89b15), one gate at a time:**
  - `sh scripts/check --timing`: CHECK RESULT PASS, 58 stages, 533 s of stage time (conformance 109 s, sanitizers 241 s). An earlier full run at beb478e also passed (522 s).
  - `sh scripts/check --selftest`: SELFTEST RESULT PASS (207 PASS lines).
  - `sh scripts/audit-history --range 59d8eed..HEAD`: clean (13 commits, 40 blobs, 2,275 blob-path pairs).
  - `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro`: gate-gcc PASS (gcc 13.3.0, strict build of the gate preset, 65 s). The first run at beb478e failed on the WR-04 message truncation, fixed in 4c89b15. This is a local one-off container run, not a CI claim.
- **Untouched:** `.planning/state.json` (untracked). No requirement was ticked, and no push, tag, PR or settings change was made.

**Commit trailer (deviation).** The commits end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, the attribution the session gave for the model that made them, instead of the Sonnet 5.5 trailer of the brief (the 04.1 fix pass did the same). Nothing in the gate checks the trailer.

---

_Fixed: 2026-11-14T06:23:29Z_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
