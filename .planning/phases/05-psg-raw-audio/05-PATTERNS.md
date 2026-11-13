# Phase 5: PSG Raw Audio - Pattern Map

**Mapped:** 2026-11-12
**Files analyzed:** 34 file groups (about 130 concrete files: 3 new core sources or tables, 1 new runner kind file, 3 fixture ROMs with .checks, .aexp and manifests, 1 new gate script with 2 awk stages and its data files, 7 toys, about 45 to 60 REQUIRED ids, 12 new and 6 amended ledger entries) from 05-CONTEXT D-182..D-218 and the 10 plans of D-217
**Analogs found:** 34 / 34 groups (5 groups have a partial analog only; 6 sub-features have no repo analog, listed under "No Analog Found")

**Scope rules used.**
1. Every analog is a working-tree file at HEAD `5f50b98`. The code is identical to `913dd4b`; `5f50b98` adds only `05-RESEARCH.md`. Line numbers are of that tree. Plans 05-02 and 05-03 move most of them: re-read the range before editing.
2. Every cited path was checked with `git ls-files` and is git-tracked source. No gitignored mirror path exists in this repo, and none appears here.
3. Findings are numbered **F-51 onward**. They continue the project-wide series (04-RESEARCH F-1..F-25, 04-PATTERNS F-26..F-50), because 05-CONTEXT cites Phase 4 F-2, F-3, F-4, F-5 and F-11 by number. `05-RESEARCH.md` (committed as `5f50b98` while this map was written) numbers its findings RF-1..RF-17, so the series do not collide. Where a finding here overlaps an RF item, it names it.
4. Paths are repository-relative only.
5. Phase 4 analogs are preferred: `beam.c`/`vdc.c` for the device and its fold, the `vid` channel of `obs.c` for the `pcm` sink, the `dev` channel for the `psg` sink, `videokinds.c`/`vdckinds.c` for `audiokinds.c`, `scripts/video-check` with `vexp-lex.awk`/`vexp-rules.awk` for `scripts/audio-check`, plan 04-03 for the bump.
6. Where this map and 05-RESEARCH disagree, both readings are given for the planner to rule. The one that matters is F-64 against RF-2.

## Repo facts that contradict or sharpen 05-CONTEXT (read first)

| # | CONTEXT says | Repo says | Effect |
|---|---|---|---|
| C-1 | D-207, D-214: `audio-check` makes 59 stages | `scripts/check:3647-3703` has 57 `run_stage` lines; `docs/ci-timings.md:709` "video-check is the 57th stage" | 57 -> 58 stages. Fix the number in the handbook and in `docs/ci-timings.md` (F-51); the 05-RESEARCH integration map repeats 59 |
| C-2 | D-214 budget "Constants 34 of 64" | the binding cap is `CROWS_MAX 40` (`tests/conformance/reckinds.c:1217`) in `record_constants_shown`; 64 is `WH_MAX` (`src/text/text.c:278`) for withheld ids | 47 rows break `record_constants_shown` at row 41: raise `CROWS_MAX` (F-53) |
| C-3 | D-205 "the union stays under 64, now 34" | both record goldens print `[withheld] count = 35` | 35 -> 42 withheld ids (F-54) |
| C-4 | D-202 "regeneration follows D-165" (no size note) | `scripts/record-lint:82` refuses a golden over 16,384 bytes; `accept_256k_zero.record.toml` is 16,065 bytes, `accept_384k_seeded` 16,099 | the bump overflows both goldens unless their history comments are compacted (F-56) |
| C-5 | D-204 adds `seeded-prng-v4` as policy 5 (implied by the enum) | `snapkinds.c:1347-1350` writes policy 5 as "the first unknown policy"; `snapshot_refusal_fields.toml` expects `policy-5=field` | that row turns `profile` and goes red at the bump; move it to policy 6 (F-57) |
| C-6 | D-205 "each row lands with the plan that first uses it" | D-202 and D-217 (05-03) say every PSG constants row lands in the bump | internal tension; landing all 13 in 05-03 churns the goldens once (F-84) |
| C-7 | D-207 grammar `scene <S> ...` plus "the .aexp header's S must equal the stream's sample_mclk" | the grammar lists no header row; `<S>` is the scene name everywhere else; D-211's `T'` is not a legal name (`isname`, `vexp-rules.awk:45-47`) | add a header row kind or drop the sentence; rename the twin (F-73) |
| C-8 | D-200 "the sample function is const ... a core-boundary anchor" | `scripts/core-boundary-check` has no const-signature rule; `tests/controls/vid-emit-mutates/RULE` says const is compiler-enforced "which is why no core-boundary rule exists" | the const is enforced by the compiler plus toy `psg-emit-mutates`; a new core-boundary rule would need its own control (F-77) |
| C-9 | D-215 lever (b): "a third parallel job" for the clang build and conformance | `workflow-rules.awk:175` (WF_NO_GATE_STEP) requires every non-`gate` job to run `sh scripts/check`; the clang conformance run needs `build/gate/fixtures` (`conformance-run:758-765`) | lever (b) needs a workflow-check rule change with a control, plus cc65 and `scripts/fixture-build` in the new job (F-76) |
| C-10 | D-207 "the selftest gains three mutations" | the video precedent produces 24 SELFTEST lines from `video-check` ((z7b) 4, (z7c) 14, (z7d) 6; `scripts/check:2735-2820`), and D-216 R4 asks for a removal proof per enforcement rule | 175 -> at least 178; expect more if R4 is applied per tagged rule (F-52) |
| C-11 | D-218 "timing-accurate already exists" | not a regex entry in `docs-check:125`; caught because `accurate` treats a hyphen as a boundary (`docs-check:121-122`) | behaviour as intended; only four new alternations are needed (F-75) |

## Findings for the planner (F-51 onward)

- **F-51 Stage count and placement.**
  - Register `run_stage audio-check stage_audio_check` directly after `run_stage video-check stage_video_check` (`scripts/check:3677`): static, before the build.
  - Use ONE stage function in the shape of `stage_video_check` (`scripts/check:998-1009`): the checker on the repository, then `run_controls --ok-prefix --require-negative tests/audio tests/audio ...`, then the generated lex-limit controls (a twin of `vc_gen_limits`, `:977-996`); return the worst code.
  - That is the F-42 ruling: the gate goes from 57 to 58 stages.
- **F-52 Selftest count.**
  - 175 PASS lines at the 4.1 close (`04.1-REVIEW-FIX.md:182`).
  - The three D-207 mutations copy loop (z7b) (`scripts/check:2735-2762`): copy `scripts/`, check the control is met, delete the `rule-...` tagged lines in the copy of the rules awk, the same run must exit 1. That gives 178.
  - If R4 is applied per tagged rule as in Phase 4, add one line per other `audio-check` rule ((z7c), `:2764-2789`) and one per `.aexp` lex limit ((z7d), `:2791-2820`). Record the final count from the run.
- **F-53 Constants cap.** `parse_constants_def` (`reckinds.c:1236`) returns -1 at row `CROWS_MAX` + 1, and the "added row noticed" control needs `n + 1 <= CROWS_MAX` (`:1383`). 34 + 13 = 47 rows. Raise `CROWS_MAX` (to 64, the `WH_MAX` value) in the plan that adds the 7th PSG row, which is 05-03 if all rows land at the bump.
- **F-54 Withheld ids.**
  - Today 35. Phase 5 adds 7: LED-AUD-0004, -0007, -0008, -0010, -0012 and -0014 through constants rows, and LED-AUD-0009 through `psg_vol.def`. LED-AUD-0001 is already in, through the bound `psg_tick_divisor` field.
  - `psg_vol.def` has no constants row, so LED-AUD-0009 reaches `[withheld]` only if it is added to the fixed list at `text.c:372` AND to its twin in `withheld_expected` (`reckinds.c:448`) in the same change, or if both files get a `psg_vol.def` loop.
  - Result: 42 ids, under `WH_MAX` 64 and `WH_EXP_MAX` 64 (`reckinds.c:385`).
- **F-55 Record row budget.**
  - `REC_MAX_ROWS` is 192 (`runner.h:195`). The formula at `reckinds.c:877` is `60 + 1 + nids + 4 + 1 + 2 + events + 4 * models + consts`.
  - The skeleton has 153 rows today and 173 after Phase 5. With the maximum of 16 stimulus events (`DS_IRQ_STIM_MAX`) it reaches 189 of 192.
  - Three rows of headroom are left; any extra row needs `REC_MAX_ROWS` raised (and with it `exp_key[]` at `reckinds.c:358`).
- **F-56 Golden file size (blocker of the bump plan).** record-lint (`scripts/check:830`, `scripts/record-lint:82`) fails a golden over 16,384 bytes. The two goldens are 16,065 and 16,099 bytes: 49 comment lines of per-plan history, and a record body of about 4,420 bytes. The bump adds:
  - 13 `[constants]` rows (about 40 bytes each);
  - 7 `[withheld]` rows (about 23 bytes each);
  - one method paragraph.

  That totals about 17,300 bytes. Compact both comment blocks in the same commit: keep the D-53 statement, the current regeneration method and the derivation of the current literals. Also avoid exactly 16,384 bytes: `audit-history` flags a tracked blob of 4,096 bytes or more whose size is a multiple of 8,192 (ROM-SIZE-8K).
- **F-57 Hostile policy row.** `ed_policy5` (`snapkinds.c:1347-1350`, table row `:1820`) writes policy 5, and `snapshot_refusal_fields.toml` expects `policy-5=field`. Once `DS_POLICY_SEEDED_V4 = 5`, `dec_powr` (`snapshot.c:262-266`) returns `profile` for a valid policy that is not the bound one. Rename the row to `policy-6`, write 6, and update its comment, all in the bump commit.
- **F-58 seeded-prng-v4 touches these sites** (Phase 4 F-33, extended):
  - Profile and allowed set: `profile_fields.def:32` (the allowed set); `profkinds.c:188` (`core_allowed_without_r1`).
  - `ds_poweron.h`: `:21` (add `DS_POLICY_SEEDED_V4 = 5`), `:24` (add `DS_DOMAIN_PSGW` before `DS_DOMAIN_COUNT`), and the comments at `:7-11`, `:23`, `:49`, `:55`.
  - `poweron.c`:
    - `:32-40` `domain_const`: add `"PSGW____"`. ASCII-packed this is 0x505347575F5F5F5F; recompute it with a host tool when writing.
    - `:85-127`: the four helpers, with version 4 for v4.
    - `:175-190`: `apply` (`>= V4` -> `apply_v4`, which writes 192 bytes into `psg_wave`, each `& $1F`).
    - The header comment at `:4-24`.
  - Snapshot: `snapshot.c:262-263` (`> DS_POLICY_SEEDED_V3` becomes V4; "outside 1 to 4" becomes 5).
  - `ds_core.h:23` and `:144` comments.
  - Policy lists in the runner:
    - `guestkinds.c:867` (`default_policies`, with its header at `:31-32` and `:864`).
    - `vdckinds.c:60` (`vd_policies`, header `:14`, `:27`; `runner.h:408`). This second list is "shared with the observer kind" (`vdckinds.c:55`). D-204 names only the guest list, so switch or keep it explicitly.
  - `snapkinds.c:1349` (F-57).
  - `powkinds.c`: the grammar (header `:9-15`; items `:183-211`) gains `wave@N=` and `drawn=wave`, and the `vid=constants` check near `:233` gains a `psg=constants` twin (registers 0, `nrem` 1, `lfsr` 1, pad 0).
  - No edit needed: `text.c` (`emit_power_on :396-406` calls the helpers) and `boot.c`.
- **F-59 Layout without implicit padding.**
  - `vid_pad[6]` is the last member today, and `ds_core.h:300` asserts that. Move the assert to the new last member.
  - The regions are PSG0 = 42 bytes (12 + 30), PSGW = 192 and PSGT = 40 (u32, which needs 4-alignment).
  - In the D-202 order (PSG0, pad, PSGW, PSGT), only a **6-byte** pad avoids implicit padding: 42 + 6 + 192 + 40 = 280, a multiple of 8, with PSGT at block offset 240. A 2-byte pad leaves 4 bytes of hidden tail and fails the tiling assert (`ds_core.h:297`).
  - 05-RESEARCH RF-10 picks the other valid order: PSGT first, `psg_pad[6]` last, the same 280 bytes. Either order keeps each region one live span.
  - The pad belongs to no region (like `tail_pad` and `vid_pad`). Add it to `sk_is_identity` (`snapkinds.c:82`) and to the zero check in `powkinds.c` (near `:233`).
  - Keep `    uint8_t tail_pad[6];` byte for byte (selftest `mutation-tiling-entry`, `scripts/check:3059-3064`).
  - Add adjacency asserts in the style of `serial.c:190-197`.
- **F-60 Snapshot size pins.** In `snapkinds.c`: `SK_VERSION` 8 -> 9 (`:32`), `SK_FRAMES` 21 -> 24 (`:33`), `sk_frames[]` (`:39-43`) and the `RG_*` enum (`:44`) gain PSG0, PSGW and PSGT. `SK_EXPECTED_SIZE` (`:34`) goes from 92,255 to 92,553 (3 frames x 8 bytes + 274 payload bytes, with the pad in no region).
  - Prose to update: `ds_snapshot.h:30`, `snapshot_equals_digest.toml:15`, goldens line 39, `digest_region.toml:16` ("21 regions") and `digest_stream_prefix.toml:32` ("twenty-one").
- **F-61 Digest kind limits.** `DS_REGIONS_MAX` is 24 (`digkinds.c:147`): exactly full after Phase 5. The per-tag payload checks (`digkinds.c:372-403`) need PSG0, PSGW and PSGT branches; an unknown tag is a harness error (deviation 1 of 04-03). `DK_CAP` 131,072 (`:40`) still holds the 92,553-byte stream. The `digest_region.toml` tags list gains `PSG0,PSGW,PSGT`.
- **F-62 What the regeneration must compute.**
  - All five `digest_stream_*` cases are boot-only (no `until_mclk`) under `seeded-prng-v1 seed 42`, and both record goldens are boots too (zero, and v1 seed 42). So PSG0 is zero, PSGW is zero (v1 draws no wave), and PSGT is six zero u32 followed by `01 00 00 00` x 2 (`nrem`) and `01 00 00 00` x 2 (`lfsr`).
  - No pinned state digest exists after a run (`grep` finds digest literals only in the `digest_stream_*` cases and the goldens). The video `golden` rows in `.vexp` pin DSVR streams, not state, and do not churn.
  - The independent LFSR script of D-202 is therefore needed only for literals of cases that run: `psg_poweron` and `noise_seq` rows.
  - RF-11 gives the closed form for a run with no PSG write, for a future pinned run literal; no committed case needs it today.
- **F-63 Predicted bump red list.** Start from Phase 4's observed 26 cases (`04-03-SUMMARY.md:107-112`): the five `digest_stream_*`, `digest_region`, `snapshot_equals_digest`, `record_skeleton_256k`, `record_refusal_text`, the two goldens, `chunk_equiv_random32` and the snapshot family (red only until the `snapkinds.c` hand tables move). Add for Phase 5:
  - `snapshot_refusal_fields` (F-57);
  - the profile allowed-set case(s) behind `profkinds.c:188`;
  - `record_constants_shown` once `CROWS_MAX` is passed (F-53).

  A case red outside this list means a missed layout field, which calls for the one allowed correction bump (D-202).
- **F-64 Fold base and the PCM cursor: an alternative to RF-2 (needs a ruling).**
  - Machine time during a step: `ds_cpu_step_obs` reads `start = inst->machine_time` (`cpu.c:1101`), and `run.c:117` (also `obskinds.c:171`) adds `total` only after the call returns. Inside a step `machine_time` is therefore the step start.
  - RF-2 recommends carrying the step-local fold point as `uint64_t *psg_at`, the last member of `struct ds_bus`, owned by `ds_cpu_step_obs`. That touches the three positional initialisers at `cpu.c:514`, `:679` and `:694`.
  - Hazard if RF-2 is adopted:
    - `dl_windows` zeroes its bus with `memset` (`obskinds.c:1634`), so `psg_at` is NULL.
    - It then writes `$0805` (window 2, R5) through that bus (`obskinds.c:1642-1645`).
    - `hw_write` must therefore treat `bus->psg_at == NULL` like a NULL bus, or every test bus must set it; otherwise this is a NULL dereference under the sanitizers.
    - The other zeroed test buses (`obskinds.c:1726`, `vdckinds.c:500`) do not write window 2 today.
  - Alternative with no bus field:
    - Keep the stored distances relative to `machine_time - 1` for the whole step. Only the step-end fold rebases them to `start + total - 1`.
    - The state fold is then idempotent: it processes every event with time <= target.
    - The one position PCM emission needs is the next sample instant. Keep it in the pcm channel of `struct ds_obs_log` (`ds_obs.h:103-107`), initialised by the new `ds_obs_begin(log, request, machine_time)` to ceil(T0/192). That value is the header's `first` (D-197).
    - Then `first(i+1) = first(i) + seq(i)` holds by construction, and the instance holds no cursor (D-186).
    - Without a pcm sink there is no cursor at all.
  - Either way, out-of-step bus users fold at stamp = `machine_time` with no rebase: `dl_windows` (`cycle_mclk` 0) and `vf_tie_one` (`vdckinds.c:483`). The D-203 distance ranges hold only between steps, so `psg_fold` must not assert them after such calls. RF-2 folds to `stamp - 1` on a NULL bus for the same reason.
- **F-65 Boot order (= RF-15).** `ds_psg_poweron` goes after `ds_vdc_poweron` (`boot.c:360`). `machine_time` is set to `DS_CONST_reset_cost_mclk` only later, at `boot.c:364`. Noise distances of 1 (the event at time 0) assume `reset_cost_mclk` = 0: add a `_Static_assert` on it, or initialise PSGT after `:364`.
- **F-66 The write path in `bus.c`.**
  - Extend `case HW_PSG:` (`bus.c:186-188`) in the D-188 order: `io_buffer = value`, `ds_psg_advance(stamp)`, `ds_psg_write(...)`, `ds_obs_psg(...)`; the existing `log_dev` at `:196-198` stays last. Reads (`:134-136`, `:153-156`) are unchanged.
  - DSPE must be emitted exactly when `log_dev` emits (`bus != NULL`, `:110`), or `psg_dev_join` breaks. Pass `bus != NULL ? bus->obs : NULL` and `bus->pc`.
  - Never repeat the substring `stamp, bus->pc, HW_PAGE_PHYS | offset,` (anchor of toy `dev-pc-from-instance`, `:111`).
  - Stale prose to rewrite: `bus.c:64` ("the PSG is a stub until Phase 5") and `:66-67`. `ds_obs.h:10` ("PSG reads are not logged") stays true.
- **F-67 `ds_obs_begin` has 7 callers (= RF-17).** They are `run.c:108`, `obskinds.c:1633`, `:1725`, `:2237`, `:2272`, `:2281` and `vdckinds.c:359`.
  - With the new signature each test caller builds a zeroed `struct ds_run_request` carrying only its own sink, and passes `machine_time`.
  - The `run.c:106` gate adds `|| request->psg != NULL || request->pcm != NULL`.
  - The comment `obs.c:126` "(Phase 5 adds aud)" is superseded by the two sinks (D-198).
  - The request grows from 56 to 72 bytes on LP64.
- **F-68 `run_refuse` variants.**
  - Old size: the struct_size OLD-size variant (`stepkinds.c:581`) is `sizeof rq - sizeof(void *)`. With two members appended, add `sizeof rq - 2u * sizeof(void *)` (56 bytes on LP64) and keep the old one as well.
  - psg sink: it is dev-shaped, so the `_Static_assert` at `:760` and the `saved[]` buffer at `:716` cover it.
  - pcm sink: it has a 32-byte header like vid, so it needs its own saved buffer (like `vsaved`, `:818`) and an 8-aligned union (like `rr_vrec`, `:496-499`).
  - The overlap case: `rr_variants` dispatches on the refusal code name (`:579`), so `run_refuse_call_overlap` (also `memory_invalid`) needs a distinguishing `expect` item or its own branch. No new case key is needed (`runner.h:24-25`, `inventory-rules.awk:32`).
- **F-69 IN-03 placement (RF-16 says the same).**
  - Three static `ranges_overlap` copies exist (`boot.c:274`, `obs.c:33`, `snapshot.c:66`), and `touches_call` (`obs.c:45-49`) already tests instance, request and result.
  - Two placements satisfy "after struct_size and reserved, before the sink checks": step 0 of `ds_obs_sinks_check` (`obs.c:128`, called at `run.c:95`), or a test in `run.c`.
  - Do not add a fourth copy without a reason.
- **F-70 Reading the divisor.**
  - No code reads `psg_tick_divisor` yet; it appears only in `profile_fields.def:29`, the default "6" at `runner.c:92`, `r1.toml:72` and `profile-rules.awk:21`. `ds_resolve` refuses an unbound open field (`profile.c:149-150`) and enforces `3|6` (`:151`).
  - The reader is therefore `ds_streq(value, "3") ? 3 : 6` in the shape of `ds_vdc_frame_lines` (`beam.c:297-300`). The D-203 tick-grid restore rule reads the same helper, as `dec_beam` does (`snapshot.c:705-708`).
  - RF-3: a snapshot does not record d. Its open question asks whether a cached byte, refused as `profile` like `frame_lines`, should take the pad position.
  - The d = 3 case uses `request = "bind.psg_tick_divisor=3"`, like `vdc_irq_263.toml`.
- **F-71 Chunk schedules (= RF-9).**
  - `cs_table` (`chunkkinds.c:47-52`) lacks `191` and `193`; `7`, `1366` and `vframe` exist. `CS_SCHED_MAX` 8 covers 5 schedules.
  - The compare words (`:477-497`) gain `audio`, and the "no `restore=N` with video" rule (`:497`) extends to audio.
  - Size the sink like `:197-202`, using the D-201 closed form.
- **F-72 Toy registries.**
  - `video-check` reads every `tests/controls/*/` (`video-check:96-109`), but its `INV_TOY_NO_LITERAL` watches only `vce.c`, `vdc.c`, `beam.c` and `vid.c` (`vexp-rules.awk:154-157`). So psg toys on `psg.c` and `cpu.c` are invisible to it. Keep audio toy EXPECTs to audio cases so that they cannot satisfy `VIDEO_SCENE_UNTOYED`.
  - `audio-check` needs its own watched list (`src/core/psg.c`, plus `obs.c` if `psg-emit-mutates` anchors there).
  - It also needs `tests/conformance/AUDIO_TOY_EXEMPT`, in the format of `VIDEO_TOY_EXEMPT` (`CASE_ID` TAB reason; reader `video-check:91-94`).
- **F-73 `.aexp` grammar gaps.**
  - (a) Header row: add a header row kind (for example `aexp sample_mclk=192`) in lex, rules, `fixture-build` and the runner, or drop the D-207 sentence (C-7).
  - (b) Variable width: optional `twin=` and D-211's `expect_red=<groups>` make scene rows variable-width, while `write_vexp` checks a fixed NF per kind (`fixture-build:158-173`). `write_aexp` needs NF ranges.
  - (c) Twin name: `T'` is not a name; use for example `tone_twin`.
  - (d) MAN classes: these are `+`-joined page lists, but `class_lint` takes one label (`vexp-rules.awk:85-109`). Split on `+` and require an img row per label.
  - (e) Faults: AF0..AF9 need their own form-by-fault table (pattern `vexp-rules.awk:117-119`), and KINDCLASS replaces the grid-check list (`:143-146`).
  - (f) End anchor (RF-13): an end anchor (`to=ev:<selector>`) for a window that ends at a later event, so that no cycle count is read from a run (R7).
- **F-74 P8 labels are accepted nowhere yet.**
  - `fixture-lint-rules.awk:57` `islabel` (`H8|S8|H7|H6|CDBIOS`) and its comment `:29`;
  - `fixture-rules.awk:62` (`manual-cite:` regex) and `:20-21`;
  - the `cites.txt` legend `:6-7`.

  `islabel` also gates the FACT-label checks of supported entries (`fixture-lint-rules.awk:168-172`). Until P8 is a label, a supported entry citing P8-n escapes `FXL_OCR_BACKS_SUPPORTED` and `FXL_UNKNOWN_CITE` silently. So the 05-01 edit is a precondition of every 05-08 and 05-10 promotion. `spec-literal:` already accepts AUD.
- **F-75 docs-check.** `accurate` (hyphen as boundary) already catches pitch-, sound-, audio-, sample-, noise-, LFO- and volume-accurate, and `compatible` catches audio-compatible.
  - Only `band-limited`, `sounds correct`, `plays music` and `authentic sound` need entries in the one regex (`docs-check:125`). Keep the single `# rule-claim-word` statement (`:137`).
  - README gets a third `Withheld claim words:` line; lines 48 and 50 are the precedent, and quotes are stripped only on such lines (`:129-131`).
  - Negative mini-root `tests/docs/claim-word-audio`, a copy of `tests/docs/claim-word-video` (README.md, AGENTS.md, scripts/check, EXPECT `DOCS_CLAIM_WORD`).
- **F-76 CI levers.**
  - Lever (a): add `nproc` and the CPU model inside the existing `{ ... } >> "$GITHUB_STEP_SUMMARY"` blocks (`gate.yml:55-64`, `:95-104`). No `|| true` (WF_FORBIDDEN_KEY) and no `${{` (WF_EXPR_IN_RUN).
  - Lever (b), rule change: today a third job fails `rule-job-gate` (`workflow-rules.awk:175`) and needs a checkout (`:174`). Add a rule change for a conformance-only job class (checked out, timed, needed by `gate`).
    - Add its control under `tests/workflow/<case>/` and a row in the `mutation-workflow-*` table (`scripts/check:2447-2462`).
    - The row `mutation-workflow-job-gate|rule-job-gate|no-gate-run-mawk` must keep detecting a job that runs neither shape.
  - Lever (b), job steps: Checkout, Tool versions, Install cc65, `sh scripts/fixture-build`, then `sh scripts/conformance-run run --preset gate-clang` (fixtures mode needs `fixtures-authored.used`, `conformance-run:758-765`).
  - Lever (b), aggregator: `gate` (`gate.yml:133-152`) gains a third `needs` entry, env and `test` line (`agg_needs_ok`/`agg_results_ok`, `workflow-rules.awk:141-171`). Update the header job list (`gate.yml:13-25`).
  - Byte copy: `tests/workflow/ok-gate/.github/workflows/gate.yml` must stay a byte copy (CONTROL_DRIFT, `scripts/check:1152-1155`), so each workflow commit edits both files. The other 30 control copies stay.
- **F-77 Const sample function.** See C-8. `ds_psg_sample(const struct ds_instance *, ...)` plus toy `psg-emit-mutates` (a cast away from const, present only in the generated toy) is the Phase 4 pattern (`tests/controls/vid-emit-mutates/RULE`).
- **F-78 Existing guests now write live PSG state.** These guards must stay green unchanged (D-188):
  - `mmu_mpr` check 31 writes `$6805` (R5 of select 0), plus `mmu_mpr` checks 22 and 29-30;
  - `spd_speed` check 12;
  - `cyc_probe` `m_psg_rd`;
  - `dl_windows` writes window 2 (F-64).

  Their digests change mid-run, but no pinned literal exists for them (F-62).
- **F-79 Cost lands in 05-03, not 05-04.**
  - The noise counters and LFSRs run from boot in every run (D-193), so the step-end fold has work in every step of every existing case from the bump on.
  - Measure the local gate after 05-03 as well as in 05-04.
  - The `sanitizers` stage re-runs every case (`stage_sanitizers` -> `conformance-run sanitize`; 303 s of 766 s in the 4.1 baseline, `docs/ci-timings.md:1036`).
- **F-80 Toy anchors in files Phase 5 edits (each must stay unique, `control-mutate` fails on n != 1).**
  - `bus.c`: `stamp, bus->pc, HW_PAGE_PHYS | offset,` and `return (uint8_t)DS_UNMAPPED_READ;`.
  - `run.c`: `inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step_obs(inst, obs);` and `if (inst->machine_time >= request->until_mclk) {`.
  - `obs.c`: `r = chan_slot(&log->insn, (size_t)DS_INSN_RECORD_BYTES);`.
  - `serial.c`: `w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);`.
  - `snapshot.c`:
    - `inst->blk_active = active;`;
    - `inst->p = p;` (a substring: no PSG decoder line may contain it);
    - `enabled > 1u || origin > v->time ?`;
    - the selftest line `        inst->y = y;` (`scripts/check:3022`).
  - `cpu.c`: `ds_vdc_advance(inst, start + (uint64_t)total - UINT64_C(1), log);`, `adc-carry` and `brk-sample-pre`.
  - `boot.c`: `__builtin_memcpy(inst->rom, request->content, request->len);`.
- **F-81 Dev and DSPE capacities.** Window-2 writes are already dev-logged today (`bus.c:196`), so dev counts do not change. DSPE is the window-2 subset. `OB_DEV_LARGE` 16,384 (`obskinds.c:503`) is the model for a "large" psg capacity. `psg_block_flood` (a 65,536-byte TIN) overflows on purpose and must never run under an observer "large" configuration (a lost record there is harness_failure).
- **F-82 ROM budget.** `OX_ROMS_MAX 40` (`obskinds.c:305`) and its text "more than 40 distinct guest ROMs" (`:414`) both change to 48. The count is distinct (ROM, request) pairs of kind `guest` (`ox_collect`, `:315-343`). Three audio ROMs, each with a guest case, give 39 of 48.
- **F-83 Ledger origins and pins.**
  - Amended seed entries keep their origins, because `ledger/REQUIRED` must equal `origin` (F-47): LED-AUD-0001 `SUMMARY C1` and -0002 `SUMMARY open-gap 9` (REQUIRED `:1-2`), PROF-0002 and -0003 (`:42-43`), CPU-0013 (`:15`), CPU-0017 (`:19`).
  - Their existing `TEST:` pins stay mutual (`INV_PIN_BACKLINK`).
  - New entries use origin `05-CONTEXT D-nnn` (equal to their REQUIRED line), `pinning_test = "PENDING:5"` and `revisit_phase = 5`.
  - Totals: 63 -> 75 entries and 63 -> 75 REQUIRED lines.
- **F-84 Constants landing.** Adding a row changes both record goldens and `[withheld]`, but not the epoch: the `constants.def` header says a changed VALUE bumps the epoch, and 04-04 added eight rows without a bump. Landing all 13 rows in 05-03 means one golden churn and one F-56 compaction. Landing them per plan (D-205) repeats both edits in 05-04, 05-06, 05-07 and 05-09.
- **F-85 Registering `audiokinds.c`.**
  - Build: add it to the explicit `DS_RUNNER_SOURCES` list (`CMakeLists.txt:63-69`; the toys reuse it). Core sources need no CMake edit (`file(GLOB ... src/core/*.c)`, `:38`).
  - Kinds: `kinds[]` rows (`runner.c:1007-1094`) and declarations (`runner.h:232-242` style).
  - Command line: `audiokinds_arg` joins the chain at `runner.c:1274`; update the usage text (`:1285`), add `audio_out_id` to the "nothing to do" test (`:1281`) and dispatch beside `:1310-1312`.
  - `conformance-run`: `rm -rf build/gate/audio` at `:209`, `audio_out_check` after `:318`, and an entry beside `:974`.
  - Create the file in 05-04 only; 05-05 and later extend it (the F-18 rule: one plan creates each file).
- **F-86 The tracer `.aexp` lands before `audio-check`.**
  - 05-04 lands the `.aexp`, while the static stage comes in 05-05.
  - Until then the runner must refuse an unknown row, key or malformed row itself (the `videokinds.c` header, line 9, does the same).
  - Every `fixtures/src/*.aexp` needs the 0BSD pair (licence-check: every file under `fixtures/`), and the lex SPDX rule mirrors `vexp-lex.awk:28-31,40-42`.

## File Classification

Plans and waves of D-217: W1 05-01 and 05-02, W2 05-03, W3 05-04, W4 05-05, W5 05-06, W6 05-07, W7 05-08, W8 05-09, W9 05-10.

| New/Modified file group | Role | Data flow | Closest analog | Match | Plan |
|---|---|---|---|---|---|
| `src/core/ds_psg.h` (new) | private header | n/a | `src/core/ds_vdc.h` | exact | 05-02 |
| `src/core/psg.c` (new) | device: decode, registers, wave RAM, counters, fold, synthesis | request-response at a stamp, event-driven closed form | `vdc.c` (mask table, write handler), `beam.c` (`ds_vdc_advance`, sink gate), `timer.c` (closed form) | role-match | 05-02 stub, 05-03..05-09 |
| `src/core/psg_vol.def`, `src/core/psg_effects.def` (new) | X-macro tables | transform | `constants.def`, `model_params.def` | role-match | 05-06, 05-04 |
| `src/core/obs.c`, `ds_obs.h`, `run.c` (mod) | 4th and 5th sink channel, IN-03 | streaming | themselves (`dev` channel for psg, `vid` channel for pcm) | exact | 05-02, 05-04 |
| `src/core/ds_core.h` (mod) | request, instance, tiling, epoch | transform | itself (Phase 4 block `:197-248`) | exact | 05-02, 05-03 |
| `src/core/bus.c`, `cpu.c` (mod) | fold points | event-driven | themselves (`HW_VDC` cases, `ds_cpu_step_obs`) | exact | 05-02, 05-03, 05-04 |
| `src/core/serial.c`, `ds_serial.h` (mod) | 3 regions | transform | itself (`VCE0`, `VRAM`, `SATB`) | exact | 05-03 |
| `src/core/snapshot.c` (mod) | 3 decoders, D-203 rules | validate then commit | itself (`dec_vdc0`, `dec_beam`, `dec_vram`) | exact | 05-03 |
| `src/core/poweron.c`, `ds_poweron.h`, `profile_fields.def`, `boot.c` (mod) | v4 and PSG power-on | transform | themselves (`apply_v3`, `ds_vdc_poweron`) | exact | 05-03 |
| `src/core/constants.def`, `src/text/text.c`, `ds_text.h` (mod) | defs and printer | transform | themselves | exact | 05-03 (and per F-84) |
| `tests/conformance/audiokinds.c` (new) | kinds `audio`, `audio_fault_sweep`, hash role, out rights, C-level cases | batch | `videokinds.c` (judge), `vdckinds.c` (C-level) | role-match | 05-04 create, 05-05..05-09 |
| amended runner kinds (obs, step, chunk, snap, dig, rec, prof, pow, guest, vdc) | kinds | batch | themselves | exact | 05-02, 05-03 |
| `runner.c`, `runner.h`, `CMakeLists.txt` | registration | config | themselves | exact | each kind plan |
| `tests/conformance/cases/*.toml`, `REQUIRED` | records | CRUD | `video_bg_basic_asym`, `chunk_equiv_video_vce_pal`, `observer_vce_pal`, `snapshot_restore_video`, `vid_noninterference`, `vdc_poweron_policy_diff` | exact | each |
| `tests/conformance/golden/*.record.toml` | goldens | CRUD | themselves (04-03 method) | exact | 05-03 |
| `fixtures/lib/psg.inc` (new; name discretionary, 05-RESEARCH says `audio.inc`) | guest include | file-I/O | `fixtures/lib/vce.inc` | role-match | 05-04 |
| `fixtures/src/psg_tone.s`, `psg_mod.s`, `psg_lfo.s` and `.checks` | authored ROM | event-driven | `vce_pal.s` (window writes, TIA), `tim_timer.s` (timer IRQ) | role-match | 05-04, 05-07, 05-09 |
| `fixtures/src/*.aexp` (new) | hand-derived literals | CRUD | `fixtures/src/vce_pal.vexp` | role-match | 05-04 onward |
| `fixtures/manifest/psg_*.toml` (3 new) | records | CRUD | `fixtures/manifest/vce_pal.toml` | exact | each fixture plan |
| `fixtures/lib/cites.txt`, `TOOLS.md` (mod) | registries | CRUD | themselves (H7/H6 rows of 04-01) | exact | 05-01 |
| `scripts/audio-check`, `scripts/lib/aexp-lex.awk`, `aexp-rules.awk` (new) | checker | batch | `scripts/video-check`, `vexp-lex.awk`, `vexp-rules.awk` | exact | 05-05 |
| `tests/audio/<case>/` mini-roots, `tests/audio/KINDCLASS`, `RULES`, `DISPUTED`, `tests/conformance/AUDIO_TOY_EXEMPT` (new) | controls and data | batch | `tests/video/<case>/`, `VIDEO_TOY_EXEMPT` | role-match | 05-05 |
| `scripts/check` (mod) | stage, selftests | batch | `stage_video_check`, loop (z7b) | exact | 05-05 |
| `scripts/fixture-build` (mod) | `NAME.aexp.tsv` writer | file-I/O | `write_vexp` | exact | 05-04 |
| `scripts/conformance-run` (mod) | `audio_out_check` | batch | `video_out_check` | exact | 05-04 |
| `scripts/docs-check`, `tests/docs/claim-word-audio/` | checker and control | transform | `docs-check:121-139`, `tests/docs/claim-word-video/` | exact | 05-01 |
| `scripts/lib/fixture-lint-rules.awk`, `fixture-rules.awk` | label rules | transform | themselves (`H7`/`H6` of 04-01) | exact | 05-01 |
| `.github/workflows/gate.yml`, `tests/workflow/ok-gate/...`, `scripts/workflow-check`, `scripts/lib/workflow-rules.awk` | CI | config | themselves | exact | 05-01 |
| `tests/controls/<7 toys>/{RULE,EXPECT}` | toys | transform | `vid-emit-mutates`, `fold-skipped-at-step-end`, `vid-rcr-base` | exact | 05-04..05-09 |
| `ledger/entries/LED-AUD-0003..0014.toml` (new), 6 amended, `ledger/REQUIRED` | records | CRUD | `LED-VID-0018` (definition), `LED-VID-0001` (assumed), `LED-CPU-0016` (supported) | exact | 05-01 |
| `README.md`, `docs/ci-timings.md`, `05-HANDBOOK.md` | docs | n/a | Phase 4 sections | exact | 05-01, 05-04, 05-08, 05-10 |

## Pattern Assignments

### P1. `src/core/ds_psg.h` (new private header; 05-02)

**Analog:** `src/core/ds_vdc.h` (76 lines). Copy its parts:
- the SPDX pair;
- a header comment that names the ONE fold and its call sites and states "valid for a folded instance";
- the include guard and `#include <stddef.h>`, `<stdint.h>`, `"ds_core.h"`, `"ds_obs.h"`;
- one comment and one prototype per entry point.

```c
/* ds_vdc.h:22-23 - the fold prototype */
void ds_vdc_advance(struct ds_instance *inst, uint64_t stamp, struct ds_obs_log *obs);
/* ds_vdc.h:46-50 - the profile-derived value and the shared mask table, read by the write handler AND the restore rule */
uint32_t ds_vdc_frame_lines(const struct ds_instance *inst);
uint16_t ds_vdc_reg_mask(uint32_t slot);
```
**Differs:**
- Prototypes of D-186..D-200: `ds_psg_advance(inst, stamp, obs)`, `ds_psg_write(inst, reg, value, stamp, ...)` returning `sel_before` and effect, `ds_psg_sample(const struct ds_instance *, ...)`, `ds_psg_poweron(inst)`, `ds_psg_divisor(const struct ds_instance *)` (F-70) and `ds_psg_reg_mask(reg)` (D-189).
- The comment states the two call sites only: the window-2 write and the step end. Never a read, never the interrupt sample (D-186).
- The core-boundary include rule allows `ds_*.h` and `*.def` (`scripts/core-boundary-check:128`).

### P2. `src/core/psg.c` (new; 05-02 inert fold, 05-03 decode and noise counters, 05-04 channel stepping and synthesis, 05-06/07/09 behaviour)

**Analog 1 (mask table shared with the restore decoder):** `src/core/vdc.c:33-43`.

```c
/* vdc.c:33-43 */
static const uint16_t vdc_mask[VDC_SLOTS] = {
    0xFFFFu, 0xFFFFu, 0xFFFFu, 0x0000u, 0x0000u, /* MAWR MARR VWR (R03) (R04) */
    ...
};
uint16_t ds_vdc_reg_mask(uint32_t slot)
{
    return slot < (uint32_t)VDC_SLOTS ? vdc_mask[slot] : (uint16_t)0u;
}
```
**Analog 2 (one fold over events, bounded loop, sink gate):** `src/core/beam.c:250-293` (`ds_vdc_advance`) and `:239-244`.

```c
/* beam.c:252-262 - the loop bound comes from the span; a stamp below the next event does nothing */
    uint64_t span = stamp > inst->machine_time ? stamp - inst->machine_time : UINT64_C(0);
    uint64_t bound = (span / (uint64_t)DS_CONST_line_mclk) * UINT64_C(3) + UINT64_C(6);
    ...
        if (!ds_vdc_pending_time(&t, ...) || t > stamp) {
            break;
        }
/* beam.c:239 - the want_pixels gate; nothing the line does to the instance depends on it */
    if (obs != NULL && obs->vid.on) {
```
**Analog 3 (profile-bound value, the one derivation):** `beam.c:297-300`.

```c
uint32_t ds_vdc_frame_lines(const struct ds_instance *inst)
{
    return ds_streq(inst->profile.field[DS_FIELD_lines_per_frame].value, "263") ? 263u : 262u;
}
```
**Analog 4 (closed form on unsigned 64-bit time, no loop over clocks):** `timer.c:22-43` (quoted in 04-PATTERNS P3).

**Differs:**
- Distances from the fold point, u32, with 0 meaning frozen (D-186), and the fold base of F-64 (or RF-2's `psg_at`).
- Closed-form catch-up for a free channel; the LFO pair and the LFSRs iterate event by event, bounded by span / d (span <= 600 between steps, RF-1).
- The sample loop is gated by `obs != NULL && obs->pcm.on` and bounded by span / 192 + 1 (D-195).
- Every PSG period comes from the D-184 table and nothing else.
- Header sentence to reuse (`timer.c:10`): "state changes ONLY inside ds_psg_write and ds_psg_advance, which a guest access or the step end calls with its own stamp".
- Each toy target is ONE statement: the select mask (`psg-chsel-mask`), the attenuation sum (`psg-atten-max`), the DDA reset (`psg-dda-no-reset`), the tap expression (`psg-noise-taps`), the LFO shift (`psg-lfo-shift`) and the emit (`psg-emit-mutates`).
- Every constant is `_Static_assert`ed where it is used (`timer.c:19` style); the D-195 obligations become asserts or unit cases.
- No float, no signed shifts, explicit `& 0x1Fu` and `& 0xFFFu` masks.

### P3. `src/core/psg_vol.def`, `psg_effects.def` (new X-tables)

**Analog:** `src/core/constants.def:1-41` (SPDX pair, a header that defines each column and the contract, one row per line, no continuation lines) and `model_params.def:4-21`.

```c
/* constants.def:41 - one row */
DS_CONST(cycle_mclk_low, 12, "LED-CPU-0003")
```
**Differs:**
- `psg_vol.def` has 30 rows with the formula round(32768 * 10^(-3a/40)) in the header. Its ledger id reaches `[withheld]` only by F-54.
- `psg_effects.def` rows carry their ledger ids (D-196).
- `licence-check` requires MIT OR Apache-2.0 on `.def` files.
- Consumers use `#define X(...) / #include / #undef X`. `reckinds.c:1236-1264` is the precedent of a test reading a `.def` as text.

### P4. `src/core/obs.c`, `ds_obs.h`, `run.c` (mod; 05-02): channels 4 and 5, `ds_obs_begin` once, IN-03

**Analog for `psg`:** the `dev` channel (`ds_obs.h:38-43`, `:67-71`; `obs.c:81-94`, `:302-324`, `chan_slot :227-244`).
**Analog for `pcm`:** the `vid` channel (`ds_obs.h:45-55`, `:73-77`, `:91-101`; `obs.c:97-110`, `:203-212`, `:246-258`, `:328-359`).

```c
/* obs.c:126-136 - the table; the ORDER (structs, capacities, ranges) is behaviour */
#define DS_OBS_SINKS 3u /* the channels of the table: insn, dev, vid (Phase 5 adds aud) */
    sink_ref_insn(&s[0], request->insn);
    sink_ref_dev(&s[1], request->dev);
    sink_ref_vid(&s[2], request->vid);
/* obs.c:260-265 - the signature that changes once (F-67) */
void ds_obs_begin(struct ds_obs_log *log, struct ds_insn_sink *insn, struct ds_dev_sink *dev, struct ds_vid_sink *vid)
/* run.c:106-109 - the gate */
    if (request->insn != NULL || request->dev != NULL || request->vid != NULL) {
        obs = &log;
        ds_obs_begin(obs, request->insn, request->dev, request->vid);
```
**Differs:**
- Header and records: the `psg` header is the dev shape (16 bytes, 24-byte records, `DS_SINK_MAX_RECORDS`). The `pcm` header is 32 bytes with `first`, `sample_mclk`, `frame_bytes`, `version` and `format` (D-197); its unit is the 8-byte frame (`rec_unit = 8`), its limit `DS_PCM_SINK_MAX_FRAMES` 8,388,608, with a `_Static_assert` like `ds_obs.h:42`.
- Overflow: fixed-size frames make the F-2 sticky-full rule equal to `chan_slot`'s, so `pcm` can reuse `struct ds_obs_chan` plus `first` and, under F-64, the cursor.
- IN-03 goes in by F-69. `struct ds_run_request` gains `psg`, `pcm` as its LAST members (`ds_core.h:383-392`) with forward declarations beside `:377-380`.
- `ds_obs_end` (`obs.c:267-274`) writes both new headers.
- Keep `obs.c:284` (`trace-hook-mutates`) and `run.c:117`, `:123` byte for byte (F-80).
- Prose to update: `run.c:4-10` and `ds_obs.h:3-26`.

### P5. `src/core/ds_core.h` (mod; 05-02 request, 05-03 instance, tiling, epoch)

**Analog:** itself. The Phase 4 block (`:197-248`), the tiling list (`:284-297`) and the tail asserts (`:298-300`).

```c
/* ds_core.h:247-248, 295, 300 - the last member, its tiling entry, the last-member assert */
    uint8_t vid_pad[6];
    X(vce_cr) X(vce_cta) X(vce_pal) X(vdc_vram) X(vdc_satb) X(vid_pad)
_Static_assert(offsetof(struct ds_instance, vid_pad) + sizeof(((struct ds_instance *)0)->vid_pad) == sizeof(struct ds_instance), "vid_pad is not the last member of the instance");
```
**Differs:**
- Append after `vid_pad`: PSG0 (42), a 6-byte pad, PSGW (192), PSGT (40) (F-59), or the RF-10 order.
- `DS_BEHAVIOUR_EPOCH` 5 -> 6 (`:22`) in 05-03 only.
- The model cap at `:521` stays 4 (D-205).
- `struct ds_bus` (`:428-437`) changes only if RF-2 is adopted (F-64).

### P6. `src/core/bus.c`, `cpu.c` (mod; 05-02 inert calls, 05-03 decode)

**Analog:** the VCE write case and the step-end fold.

```c
/* bus.c:172-175 - fold to the access stamp, then the device */
    case HW_VCE:
        ds_vdc_advance(inst, stamp, bus != NULL ? bus->obs : NULL);
        ds_vce_write(inst, offset & 0x3FFu, value, stamp);
        break;
/* bus.c:186-188 - today */
    case HW_PSG:
        inst->io_buffer = value;
        break;
/* cpu.c:1099-1106 - the PSG fold goes after the VDC one, same stamp (D-186 (2)) */
    uint64_t start = inst->machine_time;
    uint32_t total = cpu_step_body(inst, log);

    ds_vdc_advance(inst, start + (uint64_t)total - UINT64_C(1), log);
    return total;
```
**Differs:**
- F-66 for the write order and the join rule.
- No call in `sample_and_enter` (`cpu.c:562-576`): the PSG raises no interrupt.
- The new step-end line is the anchor of `psg-fold-skipped-at-step-end` (replacement `(void)start;`, as in `tests/controls/fold-skipped-at-step-end/RULE`).
- ST0-ST2 never reach window 2 (`bus.c:243-248`).

### P7. `src/core/serial.c`, `ds_serial.h` (mod; 05-03): three regions

**Analog:** `VCE0` for a small multi-field region and `VRAM`/`SATB` for byte arrays.

```c
/* serial.c:134 - the enum gains R_PSG0, R_PSGW, R_PSGT before R_COUNT */
enum region_id { R_PROF, ..., R_VRAM, R_SATB, R_COUNT };
/* serial.c:215, 220-221 - rows: tag, payload length, live offset, live length */
    [R_VCE0] = {"VCE0", 3u, offsetof(struct ds_instance, vce_cr), offsetof(struct ds_instance, vce_cta) + sizeof(uint16_t) - offsetof(struct ds_instance, vce_cr)},
    [R_SATB] = {"SATB", DS_SATB_BYTES, offsetof(struct ds_instance, vdc_satb), DS_SATB_BYTES},
/* serial.c:318-321 - payload, one explicit field per line, little-endian */
    case R_VCE0:
        w_u8(w, inst->vce_cr);
        w_u16(w, inst->vce_cta);
        break;
```
**Differs:**
- `DS_SERIAL_VERSION` 8 -> 9 (`ds_serial.h:22`), `DS_REGION_COUNT` 21 -> 24 (`:41`); the region list comment `:7-8` and the count assert `serial.c:411` follow.
- Add the live-span asserts in the style of `:190-197`.
- PSGW uses `w_bytes(w, inst->psg_wave, ...)`, never the `drop-region` anchor text.

### P8. `src/core/snapshot.c` (mod; 05-03): decoders and the D-203 domain

**Analog:** `dec_vdc0` (`:614-648`: read all fields, commit, then judge against the shared mask), `dec_beam` (`:650-743`: profile-bound value `:705-708`, judgement only in pass 1 because `v->time` is set only there `:692-693`), `dec_vram` (`:783-796`), `decoders[]` (`:814-821`) and `walk` (`:824-868`).

```c
/* snapshot.c:195-198 - the pass-1 context; PSGT judges against PSG0 fields decoded before it, so the context must carry them */
struct vctx {
    uint8_t slot_type[DS_PORT_COUNT];
    uint64_t time;
};
/* snapshot.c:643 - the one mask table */
        if (((uint32_t)reg[i] & ~(uint32_t)ds_vdc_reg_mask(i)) != 0u) {
```
**Differs:**
- The PSGT rule "`rem[i]` = 0 exactly when channel i is frozen" reads `ctl`, `dda` and `lfo_ctl` from PSG0. Extend `struct vctx` with the PSG0 values pass 1 has read; that is the `slot_type` precedent.
- The grid rule computes `(v->time + r - 1) % d` only after `r >= 1` is known, so no wrap occurs; `d` comes from `ds_psg_divisor(inst)` (F-70).
- All ranges are checked before any arithmetic. Refusal code 12 (`field`) is reused, so `snapshot_refusals.def` is unchanged.
- Add three rows to `decoders[]` in table order; a missing row fails every restore as `snapshot.tag` (F-34c).
- `dec_powr` accepts policy 5 (F-57, F-58).

### P9. `src/core/poweron.c`, `ds_poweron.h`, `profile_fields.def`, `boot.c` (mod; 05-03)

**Analog:** `apply_v3` (`poweron.c:159-173`) and `ds_vdc_poweron` (`beam.c:302-307`, called at `boot.c:360`).

```c
/* poweron.c:164-165 - one domain stream straight into an array */
    ds_stream_init(&st, inst->powerup_seed, (uint32_t)DS_DOMAIN_VRAM);
    ds_stream_fill(&st, inst->vdc_vram, sizeof inst->vdc_vram);
/* poweron.c:187-189 - additive: v4 runs every earlier apply, then its own */
    if (inst->powerup_policy >= (uint32_t)DS_POLICY_SEEDED_V3) {
        apply_v3(inst);
    }
```
**Differs:** All sites are listed in F-58. `ds_psg_poweron` sets `nrem` = 1 and `lfsr` = `psg_lfsr_reset` under every policy (F-65). The D-204 fallback (drop v4) touches only these sites.

### P10. `constants.def`, `text.c`, `ds_text.h`, `reckinds.c` literals

- `constants.def`: rows of D-205, each with its ledger id FIRST (`inventory-check:149`, rule `INV_CONST_LEDGER`), so 05-01 lands before any row.
- `ds_text.h:36` 7 -> 8, with the version history comment at `:33-34`.
- `reckinds.c:246-247`: `format_version = 8`, `behaviour_epoch = 6`.
- `emit_constants` (`text.c:571-584`) and `emit_model` are generic. `emit_withheld` (`:355-393`) needs F-54 for `psg_vol.def`.
- Caps: F-53 and F-55.

### P11. `tests/conformance/audiokinds.c` (new; 05-04 created, 05-05 judge, later C-level cases)

**Analog 1 (the judge kind):** `videokinds.c`:
- header `:3-17`: the kind, `expect` items, the `.tsv` channel, row kinds, refusal of unknown rows;
- hand-written layout offsets `:26-58`;
- `vx_load` `:384-400`, which reads `build/gate/fixtures/NAME.vexp.tsv`;
- `kind_video` `:2453`, golden role `:2844-2858`, `kind_video_fault_sweep` `:2913`, `kind_video_hash_not_verdict` `:2997`;
- in-process judge self-tests `:3105-3430` (`jf_want(...)` rows, pass, fail and harness per form), `kind_video_judge_forms` `:3571`;
- `--video-out` `:3610-3880` (`videokinds_arg :3620`, `vo_do :3778`, `video_out_main :3858`, `kind_video_out_rights :3888`).

```c
/* videokinds.c:393-396 - the literal channel; a missing file is a harness fault, never an empty scene list */
    snprintf(path, sizeof path, "build/gate/fixtures/%s.vexp.tsv", rom);
    text = read_file(path);
    if (text == NULL) {
        return "vexp-missing: the built vexp rows are missing: run scripts/fixture-build first ...";
```
**Analog 2 (C-level cases):** `vdckinds.c`:
- `kind_vdc_fold` `:627` (random partitions against one jump, the tie through the bus write path, the invariant);
- `kind_vid_noninterference` `:768`, `kind_snapshot_restore_video` `:1012`, `kind_vdc_regmask` `:1549`, `kind_vdc_advance_pure` `:2172`;
- `kind_vdc_poweron_policy_diff` `:2925` (with a control image that must differ), `kind_vdc_random_regs` `:3042`;
- the policy helpers `vd_policy_name`/`vd_compose_request` (`:62-70`).

**Analog 3 (sink-shape kinds):** `kind_vid_chan_append` (`obskinds.c:2301`) for the pcm channel, `kind_insn_sink_shapes` for psg.

**Differs:**
- The runner parses DSPE and DSPA with its own offsets and never includes the core's record writer (ER-19). Streams are built in test code (D-199).
- Selectors convert machine time to sample index with the stream's `sample_mclk` and `first`. The nearest precedent is the dev-log-anchored selector `line=NAME N+D` (`vexp-rules.awk` header, lines 24-26).
- `lost != 0` is `harness_failure` (F-37). The evidence line reads `pcm_sha256=... psg_sha256=... E0 repeatability` (ER-20).
- The policies are zero and v4 seed 1 for literal cases (D-204); use a v4 twin of `vd_policies` if the shared list keeps v3 (F-58).

### P12. Amended runner kinds (05-02, 05-03): exact sites

| File and lines | Edit |
|---|---|
| `obskinds.c:305`, `:414` | `OX_ROMS_MAX` 48 and its error text (F-82) |
| `obskinds.c:503-529`, `:770`, `:829`, `:849`, `:1104`, `:1173`, `:1285` | observer: add psg and pcm capacity columns to `ob_cfgs`, a pcm twin of `ob_vheader_consistent`, an append helper; keep the sink names `large,drop4,poison,cap0,cap1` (F-31) |
| `obskinds.c:1633`, `:1725`, `:2237`, `:2272`, `:2281`; `vdckinds.c:359` | `ds_obs_begin` callers (F-67) |
| `obskinds.c:1634`, `:1642-1645` | the zeroed test bus that writes window 2 (F-64, only if RF-2 is adopted) |
| `stepkinds.c:477-499`, `:574-588`, `:716`, `:760`, `:818`, `:925` | F-68 |
| `chunkkinds.c:34`, `:47-52`, `:168-202`, `:451-497` | F-71 |
| `snapkinds.c:32-44`, `:82`, `:849`, `:1347-1350`, `:1820`, `:2498-2519`, `:2613` | F-60, F-59, F-57; a PSG twin of the `vdc-tia` restore inside a block transfer (a TIN to `$0806`) for `snapshot_restore_audio` |
| `digkinds.c:147`, `:372-403`, `:566-567` | F-61 |
| `reckinds.c:246-247`, `:448`, `:877`, `:1217` | F-53..F-55 |
| `profkinds.c:188`; `powkinds.c:9-15`, `:183-211`, near `:233` | F-58 |
| `guestkinds.c:31-32`, `:864-867`; `vdckinds.c:14`, `:27`, `:60`; `runner.h:405-411` | default policy lists (F-58) |

### P13. Kind registration (every plan that adds a kind)

**Analog:** `runner.c:1044-1048` (video rows), `runner.h:232-242`, `CMakeLists.txt:63-69`. Each new kind needs, in the same change, a `REQUIRED` id and a case file (`INV_UNLISTED_CASE`, `INV_REQUIRED_MISSING`); see F-85.

```c
/* runner.c:1044-1046 */
    {"video", kind_video, 0},
    {"video_out_rights", kind_video_out_rights, 0},
    {"video_fault_sweep", kind_video_fault_sweep, 0},
```

### P14. Cases, REQUIRED and the case TOML format

**Analog:** `video_bg_basic_asym.toml`, `chunk_equiv_video_vce_pal.toml`, `observer_vce_pal.toml`, `snapshot_restore_video.toml`, `vid_noninterference.toml`, `vdc_irq_263.toml` (a bound field).

```toml
# video_bg_basic_asym.toml - the keys of a judged case (no new case key is needed for audio)
id = "video_bg_basic_asym"
kind = "video"
fixture = "authored:bg-basic"
request = ""
expect = "rom=bg-basic;id=22;scene=video_bg_basic_asym"
meaning = "... E0 repeatability"
required = true
control = false
expect_outcome = "pass"
pins = ["LED-VID-0010"]
until_mclk = 6000000
checks = 6
```
**Differs:**
- Keys come from the closed set of 17 (`runner.h:24-25`, `inventory-rules.awk:31-32`).
- The d = 3 case uses `request = "bind.psg_tick_divisor=3"`; the 263-line cadence case uses `bind.lines_per_frame=263`.
- Observer and video-style cases with an explicit `policies` key use `||`-separated fragments (`observer_vce_pal.toml`).
- REQUIRED is one id per line (334 today); every case has `required = true` and one REQUIRED line.

### P15. Goldens and digest literals (05-03): how Phase 4 repinned them

**Method (04-03-SUMMARY.md:97-99; the comment text of `digest_stream_prefix.toml:21-26`):**
1. A scratch copy of the OLD tree dumps each stream outside the repository.
2. Check that `shasum -a 256` of each OLD stream equals the committed OLD literal (all seven OK before going on).
3. A throwaway script that never calls the new core sets the version and appends the frames written by hand from the layout. For Phase 5 those are PSG0 zero, PSGW zero, and PSGT `nrem`/`lfsr` = 1 (F-62).
4. `shasum -a 256` of the new stream is the new literal. Add one method sentence to each case comment.

The epoch, format, `[constants]` and `[withheld]` lines are written by hand from the defs, never copied from printer output.

**Files that churn at the bump:**
- the five `digest_stream_*.toml` (`version=9`, `frames=` gains `PSG0:42,PSGW:192,PSGT:40`, `digest=`);
- `digest_region.toml` (tags, "24 regions");
- both `golden/*.record.toml` (F-56);
- `snapshot_equals_digest.toml` prose;
- `snapshot_refusal_fields.toml` (F-57);
- the `reckinds.c` literals.

### P16. `fixtures/lib/psg.inc` (new) and the three fixture sources

**Analog:** `fixtures/lib/vce.inc` (equates and macros only, "including this file emits no byte", `:4-5`, registers as logical addresses with MPR0 = $FF), `fixtures/src/vce_pal.s` (TIA to a register pair, `:204-206`), `fixtures/src/tim_timer.s` (timer handler installed into `VEC_TIMER`, `:125-157`; RAM vector slots `startup.inc:17-20`).

```asm
; vce.inc:10-14 - registers as equates of the hardware page at logical $0000 (MPR0 = $FF)
VCE_CR = $0400                  ; write only (a read gives the unused value)
VCE_CTA_LO = $0402              ; write only
; vce_pal.s:206 - a block transfer that alternates two registers
        tia tia_tab, VCE_CTW_LO, 8
```
**Differs:**
- PSG registers are `$0800-$0809` with MPR0 = `$FF`; the mirror is `$0810 + reg` (D-188).
- Wave upload: TIN to `$0806` (R6), or an STA loop; never TIA, which alternates R6 and R7.
- Fixture shape: DSV ids 27, 28, 29; 0BSD SPDX in the first five lines.
- Code and data budget: CODE has 4,096 bytes (`hucard.cfg` ROMA). RODATA has `$0FF6` minus the CHECKLIT bytes (F-40). Wave tables go in RODATA or BANKS.
- RF-12: `opcode_exercise` (`OX_SINK_RECORDS`, `obskinds.c:304`) limits a guest to about 35 frames of tight high-speed waiting before insn records are lost.
- `.checks` rows use one label per class (`MAN:P8-n` after F-74).
- `mutate_sweep` needs `exclude=` equal to `.observe` (F-41).
- New routines go in `psg.inc`, never in `startup.inc`, `verdict.inc` or `check.inc` (any emitted byte changes every image hash, F-40).

### P17. `fixtures/manifest/psg_*.toml` and `cites.txt`, `TOOLS.md`

**Analog:** `fixtures/manifest/vce_pal.toml` (keys `id`, `kind`, `source`, `recipe`, `toolchain`, `size`, `sha256`, `licence`, `notices`, `redistribution`, `meaning`, `verdict_source`); `cites.txt` rows `LABEL TAB img|ocr TAB verifier TAB date TAB fact`; `TOOLS.md:58-61` (the H7 and H6 manual rows).

**Differs:**
- `verdict_source` may use `manual-cite:P8-n` only after F-74.
- The `sha256` pin is measured on the first build (repeatability only).
- 25 sources -> 28; 35 authored manifests -> 38; 78 manifests -> 81.

### P18. `scripts/audio-check`, `scripts/lib/aexp-lex.awk`, `aexp-rules.awk` (new; 05-05)

**Analog:** `scripts/video-check` (209 lines):
- usage and `--expect`/`--root`/`--cites`/`--ledger` (`:7-50`);
- facts (cites with img `:79`, ledger ids `:80-83`, toy registry `:85-123`, inventory facts `:124-153`);
- stage 1 per file with a sentinel (`:155-176`), stage 2 with `RULES` (`:179-188`);
- the expect contract (`:190-201`) and `STATUS video-check ok n=N` (`:208`).

`vexp-lex.awk` (47 lines) is copied with the `AEXP_` codes. `vexp-rules.awk` supplies `class_lint` (`:85-109`), the closed tables (`:110-157`), the fact handlers (`:167-187`), `VIDEO_SCENE_UNTOYED` (`:469`) and `INV_TOY_NO_LITERAL` (`:483`), each with a `# rule-NAME` tag.

```sh
# video-check:164-169 - stage 1: a file, its status tested, its sentinel required (no awk on the left of a pipe)
  if awk -f "$here/lib/vexp-lex.awk" "$f" > "$t1" 2> "$tmp/stage1.err"; then :; else
    dead "stage 1 (vexp-lex.awk) failed on $rel"
  fi
  if ! tail -n 1 "$t1" | grep -Eq "^END${tab}[0-9]+\$" || [ "$(grep -c "^END${tab}" "$t1" || true)" -ne 1 ]; then
```
**Differs:**
- New data inputs: `tests/audio/KINDCLASS` (`AEXP_CLASS_INFLATED`), `RULES` (`AUDIO_RULE_NO_ROW`), `DISPUTED` (`AUDIO_DISPUTED_POINT`), and `tests/conformance/AUDIO_TOY_EXEMPT`.
- The arithmetic lint (`AEXP_ARITH`) works in integer awk on numbers below 2^31. mawk's `printf "%d"` saturates above 2^31-1, so compare numerically and never print such values.
- Grammar fixes per F-73. Codes print verbatim; `inventory-check` is not edited (F-6).
- `AUDIO_SILENCE_UNPAIRED`, `INV_AUDIO_HASH_ONLY`, `INV_AUDIO_GROUP_UNKILLED`, `AUDIO_GROUP_VACUOUS`.
- Limits: 512 rows, 400 characters, 16 KiB (the byte cap binds first, F-56).
- POSIX awk only (BWK, gawk and mawk).

### P19. `scripts/check` (05-05): stage, mini-roots, selftests

**Analog:** `stage_video_check` (`:998-1009`), `vc_gen_limits` (`:977-996`), loop (z7b) (`:2735-2762`), loop (z7c) (`:2764-2789`), loop (z7d) (`:2791-2820`).

```sh
# scripts/check:1002-1005
  checker sh "$here/video-check" || vc_a=$?
  run_controls --ok-prefix --require-negative tests/video tests/video sh "$here/video-check" --root @DIR@ @EXPECT@ || vc_b=$?
  vc_gen_limits "$tmp/vc-gen" || return 2
  run_controls --require-negative "$tmp/vc-gen" "$tmp/vc-gen" sh "$here/video-check" --root @DIR@ @EXPECT@ || vc_c=$?
```
**Differs:**
- `run_controls` iterates directories only (`for d in "$rc_glob"/*/`), so the plain data files `tests/audio/KINDCLASS`, `RULES` and `DISPUTED` can sit beside the mini-roots.
- A mini-root that needs its own data carries `tests/audio/<case>/tests/audio/KINDCLASS`, as video mini-roots carry `tests/conformance/VIDEO_TOY_EXEMPT`.
- The three D-207 mutations are tags `rule-audio-hash-only`, `rule-audio-group-vacuous` and `rule-aexp-class-lint`, in a copy of (z7b) (F-52).

### P20. `scripts/fixture-build` (05-04) and `scripts/conformance-run` (05-04)

**Analog:** `write_vexp` (`fixture-build:152-180`, called at `:222` for base images only) and `video_out_check` (`conformance-run:377-440`).

```sh
# fixture-build:161-166 - the fixed field count per row kind (write_aexp needs ranges, F-73)
      if ($1 == "scene") want = 4
      else if ($1 == "group") want = 7
      else if ($1 == "row") want = 5
      else if ($1 == "golden") want = 3
      if (want == 0 || NF != want) { bad = 1; next }
```
**Differs:** `write_aexp` writes `NAME.aexp.tsv`. `audio_out_check` requires exactly three stdout lines (D-199) and recomputes both digests with the host tool. No new variants line is needed: the d = 3 control is a request binding, not an image (D-211).

### P21. Toys: seven new `tests/controls/<name>/{RULE,EXPECT}`

**Analog:**
- `tests/controls/vid-emit-mutates/` for `psg-emit-mutates`: the emit statement plus a write through a cast away from const.
- `tests/controls/fold-skipped-at-step-end/` for `psg-fold-skipped-at-step-end`: replacement `(void)start;`; it lists no literal case, which is amendment A5 of Phase 4 and the precedent for D-210's two E0-only toys.
- `tests/controls/vid-rcr-base/` for the F-4 second column `pass`.

```text
# tests/controls/fold-skipped-at-step-end/RULE (keys: tag, file, anchor, replace; the anchor occurs exactly once, written to the end of its line)
tag=rule-fold-skipped-at-step-end
file=src/core/cpu.c
anchor=ds_vdc_advance(inst, start + (uint64_t)total - UINT64_C(1), log);
replace=(void)start;
```
**Differs:**
- `file=` must be a `src/core/*.c` (`CMakeLists.txt:101-106`); `psg.c` gets its toy object through the glob.
- Toys 26 -> 33 (32 after the cut).
- F-72 governs which checker sees each toy. Anchors per F-80.

### P22. Ledger entries (05-01)

**Analog:**
- `LED-VID-0018.toml` for the definition entries LED-AUD-0013 and -0014: `sources = ["UNKNOWN|record format|..."]`, `disagreement = "none: ..."`.
- `LED-VID-0001.toml` and `LED-AUD-0001.toml` for the assumed entries.
- `LED-CPU-0016.toml` for the shape of an E1 promotion: FACT sources with page labels, an `evidence` row starting `E1|`, `pinning_test = "TEST:..."`.

**Differs:**
- Ledger-check rules: `BAD_SOURCE` (`FACT|CODE|REPORT|UNKNOWN`), `E1_NEEDS_FACT_SOURCE`, `SUPPORTED_PENDING_TEST` and `SUPPORTED_NEEDS_TEST` (`ledger-rules.awk:68-94`); 16 KiB per file (`ledger-check:85`).
- Promotions in 05-08 and 05-10 change the README `Supported entries` paragraph in the same commit (`docs-check:141-170`, `DOCS_SUPPORTED_LIST`).
- F-83 governs origins and pins.

## Exact counts that change (HEAD `5f50b98` to the end of Phase 5)

| Item | Now | After | Site |
|---|---|---|---|
| Gate stages | 57 | 58 | `scripts/check:3647-3703` (C-1) |
| Selftest PASS lines | 175 | >= 178 | F-52 |
| Conformance REQUIRED ids | 334 | about 379 to 394 | `tests/conformance/REQUIRED` |
| Ledger entries / ledger REQUIRED lines | 63 / 63 | 75 / 75 | `ledger/entries`, `ledger/REQUIRED` |
| Toys | 26 | 33 (32 after a cut) | `tests/controls/` |
| Constants rows / `CROWS_MAX` | 34 / 40 | 47 / >= 47 | `constants.def`, `reckinds.c:1217` |
| Withheld ids | 35 | 42 | goldens, `text.c:372` (F-54) |
| Record rows: skeleton / with 16 events / cap | 153 / 169 / 192 | 173 / 189 / 192 | F-55 |
| Golden bytes (cap 16,384) | 16,065 and 16,099 | must stay below 16,384, never exactly 16,384 | F-56 |
| Regions / `DS_REGIONS_MAX` | 21 / 24 | 24 / 24 | `ds_serial.h:41`, `digkinds.c:147` |
| Serial / epoch / text format | 8 / 5 / 7 | 9 / 6 / 8 | `ds_serial.h:22`, `ds_core.h:22`, `ds_text.h:36`, `reckinds.c:246-247` |
| Snapshot frames / size | 21 / 92,255 | 24 / 92,553 | `snapkinds.c:32-34` |
| Instance bytes | 1,141,096 (RF-10) | 1,141,376 | F-59 |
| Run request bytes (LP64) | 56 | 72 | F-67 |
| Policies / domains | 4 / 7 | 5 / 8 | `ds_poweron.h:21`, `:24` |
| Sink channels / request members after `stop_ctx` | 3 / 3 | 5 / 5 | `obs.c:126`, `ds_core.h:389-391` |
| `ds_obs_begin` callers | 7 | 7 (new signature) | F-67 |
| `OX_ROMS_MAX` / distinct guest ROM pairs | 40 / 36 | 48 / 39 | `obskinds.c:305` |
| Authored sources / authored manifests / all manifests | 25 / 35 / 78 | 28 / 38 / 81 | `fixtures/src`, `fixtures/manifest` |
| DSV ids | 1..26 | +27, 28, 29 | D-214 |
| `cs_table` rows | 10 | 12 | `chunkkinds.c:47-52` |
| docs-check alternations / README policy lines | 13 / 2 | 17 / 3 | `docs-check:125`, `README.md:48`, `:50` |
| Workflow jobs / ok-gate copies | 3 / 1 | 4 / 1 | `gate.yml`, F-76 |
| Linux repro parts / model rows / outcomes | 6 / 4 / 7 | unchanged | `linux-repro:41`, `ds_core.h:521`, `outcomes.def` |

## Order dependencies

1. **05-01 before 05-03.** Every `constants.def` row needs its ledger entry first (`INV_CONST_LEDGER`). Every `MAN:P8-n` class (in `.aexp`, `.checks` and `verdict_source`) needs the img row in `cites.txt` and the P8 label rules (F-74). 05-01 has no C, so it runs parallel to 05-02. The RF-2..RF-5 errata and the F-64 ruling belong in 05-01 text, before any C.
2. **05-02 before 05-03.** The request members, `ds_obs_begin` (once) and the inert `ds_psg_advance` at both fold points land first. 05-03 then fills behaviour without touching the ABI.
   - The 05-02 files (core sinks, `obskinds.c`, `stepkinds.c`, `vdckinds.c`) are disjoint from 05-01's (ledger, cites, docs-check, workflow).
   - If RF-2 is adopted, the `ds_bus` member lands with its first user, together with the NULL-`psg_at` rule (F-64).
3. **Within 05-03 (one green commit, as in 04-03):**
   - the layout, tiling and asserts;
   - the serial, epoch and text bump;
   - `decoders[]` rows;
   - `snapkinds.c` hand tables;
   - `digkinds.c` tags;
   - `CROWS_MAX` (before the 41st row);
   - golden compaction (before the rows push past 16,384);
   - the policy-6 row (in the same commit as `DS_POLICY_SEEDED_V4`);
   - the regenerated literals.

   The red list must match F-63 before the regeneration.
4. **05-04 creates `audiokinds.c`**, `--audio-out`, `write_aexp`, `psg.inc`, `psg_tone` (DSV 27) and the two E0 toys. The anchor of `psg-fold-skipped-at-step-end` already exists from 05-02 and must stay one statement.
5. **05-05 lands `audio-check`, after the first `.aexp` exists** (a root with no file is a harness error, `video-check:173-176`). Until then the runner refusals cover the file (F-86).
6. **05-06, 05-07, 05-09 run in order** (they share `psg.c`). The cut moves 05-09 with `psg_lfo` and `psg-lfo-shift` and leaves the layout alone.
7. **05-08 and 05-10 promotions** need F-74 merged and a README list edit in the same commit.
8. **CI levers (05-01):** two separate commits. Each edits `gate.yml` and `tests/workflow/ok-gate/.github/workflows/gate.yml` together; lever (b) also edits `workflow-rules.awk` and adds a control. The owner's yes or no comes at publish time (D-215).

## Shared Patterns

### SPDX and rights
**Source:** `scripts/licence-check:11-15`. Code, scripts, tests and `.def` files carry `SPDX-License-Identifier: MIT OR Apache-2.0` plus the holder line. Everything under `fixtures/` (including `.aexp`) and the data records (ledger, cases, goldens, controls, `tests/audio` data) carry 0BSD. No manual text, page image, ROM or capture is committed (ER-9). Every line stays under 400 characters.

### Stamps, folds and purity
**Source:** `ds_vdc.h:4-10` and `timer.c:10`. State changes only in the handlers a guest access or the step end calls with its own stamp. Everything else is a pure query, valid for a folded instance. Every fold target is a function of the instruction stream alone (D-186). Sinks are read only at the gate and at the emit (D-200, `beam.c:239`).

### Unsigned arithmetic and casts
**Source:** `obs.c:14-30`, `serial.c` writers, `poweron.c`. Mask and cast every 5-, 8-, 12- and 16-bit value. Shifts take widened unsigned operands. Write `UINT64_C(n)` for 64-bit literals. The sanitize preset has no `-fwrapv`. No POSIX calls, no float, `snprintf` bounded with `%.Ns` (D-216 R9). RF-8: compute in-fold times as offsets, never as absolute times near 2^64.

### Validate then commit
**Source:** `snapshot.c:824-894`: pass 1 judges and writes nothing, pass 2 commits and cannot fail. A refusal leaves the instance byte-identical. Check ranges before arithmetic, and bound loops by architectural constants (span / d, span / 192 + 1, 32 words, 6 channels).

### Runner error style
**Source:** `runner.h` `check()` and `harness_error()`. stdout carries only `RESULT` and `RUNNER_DONE`. Sinks are heap blocks of the harness. A lost record in an evidence run is `harness_failure`. Outcomes come from the closed seven of `outcomes.def` (`scripts/outcome-sync` stays unchanged).

### Checker contract
**Source:** `scripts/video-check`:
- two awk stages with sentinels;
- `--root`, and `--expect` (an empty expect is exit 2);
- sorted `FILE TAB CODE TAB LINE TAB DETAIL` output;
- `EXPECT_OK`, and `STATUS NAME ok n=N`;
- exit 0, 1 or 2;
- no awk on the left of a pipe.

### Per-plan gate (D-216 cadence)
**Source:** D-177 and D-216. `sh scripts/check`, `--selftest`, `audit-history --range`, and for C plans `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro`. 05-08 and 05-10 add dash, the second awk and all six parts. Check `wc -c` of every new or grown file against the ROM-SIZE shapes (multiples of 8,192 and 8,192 + 512, 4,096 bytes or more).

## No Analog Found

| Sub-feature | Role | Data flow | Reason and nearest convention |
|---|---|---|---|
| Point-sampled stereo PCM synthesis with a Q15 volume table | core | transform, streaming | no audio code exists; `vid.c` (const render into a stack record, emitted only with a sink) is the nearest shape |
| Multi-counter fold with per-event LFSR iteration and a combined LFO counter | core | event-driven | `beam.c` is one timeline with three event kinds; `timer.c` is one closed form; `psg_fold` and `psg_pure_function` take the place of a unit analog |
| Arithmetic lint that recomputes literals from formula inputs | checker | batch | no checker recomputes a literal; nearest are the table comparisons of `opcode-check` and the field rules of `input-rules.awk` |
| Data-driven rule coverage (`KINDCLASS`, `RULES`, `DISPUTED`) | checker data | batch | no precedent; nearest are the closed `ck`, `gridck` and `guest_ok` tables inside `vexp-rules.awk:110-157` and `VIDEO_TOY_EXEMPT` |
| In-ROM `expect_red` twin scene with an exact red-group set | judge | batch | no precedent; nearest are the golden role refusal (`videokinds.c:2844-2858`) and the F-4 `pass` column of toys |
| Sample-index selectors anchored on decoded event records | judge | batch | nearest is the dev-log-anchored `line=NAME N+D` selector of `vexp-rules.awk` (header lines 24-26) |

## Metadata

**Analog search scope:** `src/core`, `src/text`, `tests/conformance` (runner, kinds, cases, goldens, REQUIRED, exempt list), `tests/controls`, `tests/video`, `tests/docs`, `tests/workflow`, `fixtures/src`, `fixtures/lib`, `fixtures/manifest`, `scripts`, `scripts/lib`, `ledger`, `.github/workflows`, `CMakeLists.txt`, `docs/ci-timings.md`, the Phase 4 summaries and `05-RESEARCH.md`.
**Files read:** about 60 (large files by non-overlapping ranges or grep: `scripts/check`, `videokinds.c`, `vdckinds.c`, `obskinds.c`, `snapkinds.c`, `reckinds.c`, `snapshot.c`, `cpu.c`).
**Tracked-source gate:** every cited path is tracked (`git ls-files`); no mirror path appears.
**Pattern extraction date:** 2026-11-12
