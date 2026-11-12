# Phase 4: VCE/VDC Raw Video - Pattern Map

**Mapped:** 2026-11-11
**Files analyzed:** 32 file groups (about 120 concrete files: 8 new core or runner sources, 12 fixture ROM sources with their .checks and .vexp, 10 toys, about 65 REQUIRED ids, 17 ledger entries) from 04-CONTEXT D-135..D-181, 04-RESEARCH F-1..F-25 and the 14 plans of D-180
**Analogs found:** 32 / 32 groups (3 groups have a partial analog only; 4 sub-features have no repo analog, listed under "No Analog Found")

**Scope rules used.**
1. Phase 3 is implemented and committed. Every analog is a working-tree file at HEAD `df20aa5`; line numbers are of that tree. Re-read the range before editing: plan 04-02 and 04-03 move most of them.
2. Every cited path was checked with `git ls-files` and is git-tracked source. No gitignored mirror path appears (none exists in this repo).
3. Findings below are numbered **F-26 onward**: they continue the F-1..F-25 of `04-RESEARCH.md`, which the planner must read as well (the orchestrator's example "F14" would collide with research F-14, the docs-check finding).
4. Paths are repository-relative only. No machine-local path appears in this file.
5. Phase 3 analogs are preferred, as asked: `timer.c`/`irq.c` for the fold-style device, the `dev` and `insn` channels of `obs.c` for the third `vid` sink, `serial.c`/`snapshot.c` for the seven regions, `irq_ctl.s` for the VDC fixtures, `trace-hook-mutates` and `timer-reload-off` for the toys, `input-check` plus its stage pair for `video-check`.

## Findings the CONTEXT and RESEARCH do not state (F-26 onward)

- **F-26 Eight existing toy anchors must stay unique through the Phase 4 edits (`scripts/control-mutate` fails closed on n != 1; selftest `mutation-control-anchor` breaks every anchor).** The Phase 4 edits that threaten them:
  - `trace-hook-mutates`: `src/core/obs.c:226` `r = chan_slot(&log->insn, (size_t)DS_INSN_RECORD_BYTES);` The table-driven refactor of D-162 keeps this exact statement once; the vid writer uses a different statement name, and the toy `vid-emit-mutates` gets its own anchor in the new writer.
  - `dev-pc-from-instance`: `src/core/bus.c:109` the substring `stamp, bus->pc, HW_PAGE_PHYS | offset,` of the one `ds_obs_dev(` call inside `log_dev`. Logging reads of windows 0 and 1 (D-149) must CALL `log_dev`; a second `ds_obs_dev(` call with the same argument text makes the count 2.
  - `unmapped-value`: `src/core/bus.c:199` `return (uint8_t)DS_UNMAPPED_READ;` appears once. The VCE reads use `DS_CONST_vce_unused_read`, never this statement.
  - `shared-state`: `src/core/run.c:117` `inst->machine_time = inst->machine_time + (uint64_t)ds_cpu_step_obs(inst, obs);` stays as is because the fold lives inside `ds_cpu_step_obs` (F-9).
  - `block-progress-lost`: `src/core/run.c:123` `if (inst->machine_time >= request->until_mclk) {` is unique today; do not copy that line shape for the vid check.
  - `drop-region`: `src/core/serial.c:228` `w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);` The VRAM, SATB and PALT cases must use other statements (for example `w_bytes(w, inst->vram, DS_VRAM_BYTES);`).
  - `snapshot-origin-any` (`enabled > 1u || origin > v->time ?`, snapshot.c:369), `snapshot-drop-blkt`, `snapshot-drop-t`, `brk-sample-pre`, `adc-carry`: keep those lines byte for byte.
  - Rule for the ten new toys (D-173): `file=` must be a `src/core/*.c` file (CMakeLists.txt:93 aborts otherwise) and the anchor one live statement, so the render decisions of D-155 are written as one statement each (divider select in `vce.c`, RCR base in `beam.c`, slot `min` in `vid.c`).
- **F-27 `hw_read` returns early for windows 0 and 1, which skips BOTH the `io_buffer` store and the dev log.** `src/core/bus.c:121-125` has `case HW_VDC: return (uint8_t)DS_VDC_STUB_READ;` and `case HW_VCE: return ...`, while the store `inst->io_buffer = value;` (line 142) and the window-4-only
  log (143-145) sit after the switch. D-104 says VDC and VCE reads never touch `io_buffer`, D-149 says they are logged. Keep the early-return shape, call the VDC or VCE read, then `log_dev(inst, bus, stamp, offset, value, (uint32_t)DS_DEV_READ)` BEFORE the return. Reuse the `stamp` computed once at
  line 120 for the fold, the device read and the log (D-149: "stamp = the stamp the fold used"). `charge_penalty` stays first (line 119).
- **F-28 `ds_cpu_step_obs` has three exits, and `start` is `inst->machine_time`, which does not move during the step.** The exits are `block_step` (cpu.c:687), `block_prologue` (705) and the final `return sample_and_enter(...)` (1090); `total` at the end includes any interrupt entry. F-9 says
  "rename the body to a static function"; the wrapper reads `start = inst->machine_time` BEFORE calling it, calls it, then folds at `start + total - 1` on every path (all three exits come back through the one wrapper). `run.c:117` is the only place that adds `total` to `machine_time`.
- **F-29 `ds_vdc_advance` must be a no-op for a stamp at or below what is already folded, and for a NULL bus.** `bus.c:88-100` `access_stamp` returns `inst->machine_time` for a NULL bus, and `tests/conformance/obskinds.c:1420` (`dl_windows`) drives the bus with `cycle_mclk = 0`, so every access has
  the instance's stamp. Both hit the fold with a stamp that is not after the last fold target. The composability rule of D-137 ("advance(a) then advance(b) equals advance(b)") needs "b <= folded point does nothing"; never rewind. Pitfall 10 of the research (a helper that folds at time 0 with no vid
  sink consumes line 0) is the same code path.
- **F-30 The D-149 read logging leaves stale prose and one hand-written rule loop in five places.**
  - `src/core/ds_obs.h:8-11` ("reads of the other windows are not logged", "every read and write of window 4"), `src/core/bus.c:61-68` (the stub comment block), `tests/conformance/obskinds.c:26-29` and `:1377-1378` (header and block comments).
  - `tests/conformance/obskinds.c:1438-1449`: the read loop keeps only `if (k == 4u)`; after D-149 windows 0 and 1 also append an expected `DS_DEV_READ` record (aux = the latch UNCHANGED, `want[].aux = w->io_buffer` already says that when read after the access). The `want[16]` array and `DL_WIN_CAP * 4u` (16) hold: 6 writes + 3 reads + 3 ST = 12.
  - `tests/conformance/cases/dev_log_records.toml` header comment ("the reads of the VDC window ... are not logged") and its `meaning` ("nothing for the reads of the VDC and timer windows"), plus the 4 new `rec=` items for `cyc-probe` (research Pitfall 4).
- **F-31 The observer matrix is a `static const` table of record counts (`obskinds.c:509-525`), and its header check assumes 16 bytes and `count = min(seq, cap)` (`obskinds.c:725`).** The vid channel has a 32-byte header with `bytes` at +16 and byte capacities. The "4-record" configuration (F-2)
  cannot be a constant: compute its byte capacity from the first four record sizes of the large run, after that run. Write a vid twin of `ob_header_consistent`. Reuse the sink NAMES (`large,drop4,poison,cap0,cap1`) so `ob_parse_sinks` and the case key `sinks` do not change (research: no new case
  key).
- **F-32 `stepkinds.c` sink variants are typed to the insn and dev structs.** `stepkinds.c:484-493` holds `rr_insn`, `rr_dev` and 24-byte-record unions; `:694` is `_Static_assert(sizeof(struct ds_dev_sink) == sizeof(struct ds_insn_sink), ...)` (true only for those two; `struct ds_vid_sink` is 48
  bytes); the `saved[sizeof sink]` buffer at `:650` is insn-sized. The vid overlap controls need their own saved buffer and an 8-aligned byte union (`rr_vrec`), plus the OLD-size request (`sizeof rq - sizeof(void *)`, research F-16). `:574` builds the struct_size variants from `sizeof rq`, so they
  adapt by themselves.
- **F-33 `seeded-prng-v3` touches nine sites, not the seven of Phase 3 F12.** `src/core/profile_fields.def:32` (allowed set), `tests/conformance/profkinds.c:188` (`core_allowed_without_r1`), `src/core/ds_poweron.h:20` (enum) and `:23` (domains), `src/core/poweron.c:29-34` (`domain_const[]`) and
  `:79-112` (`ds_policy_of`, `ds_policy_is_seeded`, `ds_policy_name`, `ds_policy_version`), `src/core/snapshot.c:255` (`policy > (uint32_t)DS_POLICY_SEEDED_V2` becomes V3), `tests/conformance/guestkinds.c:840` (`default_policies`) and the `poweron` kind grammar of `powkinds.c` (header line 11 lists
  `a=N | x=N | ... | timer_counter=N`: add `vram@N=`, `satb@N=`, `pal@N=` in the style of `ram@N=`). `boot.c:134-141` and `text.c:398-406` need NO edit (they call the helpers). `mmu_mpr_policy_diff` stays valid because v3 repeats every v2 draw. The v3 literals are derived by the throwaway-perl
  method of `poweron_v2_draw_order` (independent of the core); never from a run.
- **F-34 Snapshot decoder traps.** (a) `v->time` is set only in pass 1 (`dec_time`, snapshot.c:267-277 returns before the store when `commit` is set) and is 0 in pass 2: `dec_beam` must judge only when `!commit`, as `dec_timr` does. (b) A decoder reads ALL its fields into locals first, then `if
  (commit) { store; return NONE; }`, then judges (`dec_timr`, `dec_blkt`). (c) `decoders[DS_REGION_COUNT]` (snapshot.c:550-556) has no count guard: a missing row is zero-initialised and fails closed through the tag check at line 590 as `snapshot.tag` on EVERY restore, which reads like a harness
  fault. Add the seven rows in `regions[]` order of serial.c. (d) `snapkinds.c:79-90` `sk_is_identity` and `snap_poison` (poisons every non-identity tile with 0xA5/0x5A): every new member, explicit pad included, is restored by a decoder or listed there. (e) Model selections are judged with
  `model_values((uint32_t)DS_MODEL_vd_irq_phase)` like `dec_irqc` (snapshot.c:385).
- **F-35 The model-parameter cap is exhausted by Phase 4.** `ds_core.h:452` `_Static_assert((unsigned)DS_MODEL_COUNT <= 4u, ...)`: `tma_zero`, `irq_i_sample`, `vd_irq_phase`, `rcr_origin` make four. Phase 5 cannot add a row without reopening D-113; record it in the 04-03 summary. `model.c:89-115`
  switches on `inst->name`, so both new members are flat `uint8_t` members with exactly those names (research F-10).
- **F-36 Record text arithmetic.** `REC_MAX_ROWS` is 192 (`runner.h:195`). Rows today are `60 + 1 + nids(26) + 4 + 1 + 2 + 4*2 + 18 = 120`; after Phase 4 about `60 + 1 + 35 + 4 + 1 + 2 + 4*4 + 34 = 153`, plus up to 16 stimulus events = 169: it fits. `reckinds.c:877` computes the expectation from
  the defs (no edit); the hand-kept literals are `reckinds.c:247-248` (`format_version = 6`, `behaviour_epoch = 4`), the `expect` and the "89 rows" prose of `record_skeleton_256k.toml` (already stale), and `src/text/ds_text.h:36` (`DS_TEXT_FORMAT_VERSION 6u`).
- **F-37 The `video` kind inherits two hazards from `kind_st_bypass`.** (a) `rq.stop = dsv1_done` (buskinds.c:223) ends the run at the first step boundary after done, so the closed-form record count of D-137 must use `res.machine_time`, never `until_mclk`. (b) The sink block is a heap block of the
  harness (`padrun_new`, guestkinds.c:906-921) and a nonzero `lost` is `harness_error` (guestkinds.c:1096-1100 precedent), because the stream is evidence. Size the capacity as `lines_due * 2792` from the closed form of the case's `until_mclk` plus one line.
- **F-38 `scripts/fixture-build` always builds the base image of every stem (line 185 `build_one "$stem" "$name" 32`), with no flags.** `vdc_irq` therefore needs `.ifndef LINES` / `LINES = 262` in the source, ONE variants line (`vdc_irq vdc-irq-263 32 -D LINES=263`) and ONE extra manifest
  (`vdc_irq_263.toml`), which gives 13 images. Two variants lines would give 14 images and a third unnamed manifest. `write_classes` (lines 125-144) and `write_observe` (106-120) are the templates for the new `NAME.vexp.tsv` writer: each exits 2 on a malformed row, never writes an empty file. The
  runner reads `build/gate/fixtures/NAME.*` by relative path (`guestkinds.c:527` and `:585`): copy that shape.
- **F-39 `[DERIVED]` (D-170 `vce_pal`: "dots from div [DERIVED]") is not a legal class.** `fixture-build` `write_classes` accepts only `MAN`, `GUEST`, `ASSUMED` (line 134) and `fixture-lint` rule FXL_BAD_CLASS the same set; ER-4 and ER-10 name the same three. Use `GUEST:` for pure arithmetic over
  values the guest wrote, or `ASSUMED:LED-VID-0001`; keep the `.vexp` class set equal to the `.checks` set.
- **F-40 `hucard.cfg` ordering and the one-byte `GEXP`.** `fixtures/lib/hucard.cfg:18-23`: segments are placed in the order they are listed, so `RODATA: load = ROMB, type = ro, optional = yes;` goes AFTER the `CHECKLIT` line, which keeps CHECKLIT at file offset $1000 (`sweepkinds.c:24`
  `SW_TABLE_OFF 0x1000`). `ROMB` is `$0FF6` bytes (`fill = yes`, `$FF`). `CHECK_GUEST` (check.inc:58-65) compares against the single byte `GEXP` ($2402), so a 16-bit guest model is two rows (D-169). New routines live in `video.inc`, not in `check.inc`, `startup.inc` or `verdict.inc` (any emitted
  byte changes all 21 image hashes). `startup.inc:52-58` points every vector slot at `err`: a VDC fixture MUST install its own `VEC_IRQ1` handler before the first CLI.
- **F-41 The guest kind compares ALL observed bytes (+80..+DF) across the three power-on policies** (guestkinds.c header, "Power-on policies"). A spin-counter measurement is machine-time deterministic, so it passes the equality; a row that reads VRAM or palette that the fixture did not write fails
  it under v3. A raw measurement row with no honest literal must be an observe row (`.checks` fifth column `observe`, `LIT k` still required, `check.inc:68` assert) judged by `vdc_irq_agree` or the `video` kind rather than by an `observe=IDX:VALUE` item copied from a run (D-169 forbids that).
  `mutate_sweep` needs `exclude=` equal to the `.observe` file exactly (`sweepkinds.c` header).
- **F-42 Stage count.** Every Phase 3 checker is TWO `run_stage` lines (`input-check` + `input-check-negatives`, `docs-claims` + `docs-claims-negatives`, `scripts/check:3454-3465`). D-176 says "ONE new stage ... 57 stages"; following the precedent gives 58. Decide in 04-05: either one stage
  function that runs the checker then `run_controls` (and combines the two return codes), or accept 58 and fix the number in D-176 prose and `docs/ci-timings.md`.
- **F-43 The F-4 second EXPECT column has four exact edit sites in `scripts/conformance-run`.** `:463` (`awk '... { print $1 }'` already ignores a second column, so ids stay valid), `:468-476` (id charset and "no case carries" checks run over the ids file), `:517` (the supervised toy run takes
  `--ids` = the ids to run), `:530-535` (the loop that demands `guest_failure`). Write two id files (red and `pass`), pass their union to `filter_cases` (line 122, which also rewrites the `END` count) and to `--ids`, and check `pass` for the second list. Selftest `mutation-toy-filter` is at
  `scripts/check:1498`. The toys are already built in ONE cmake call (`:502`), so no change there.
- **F-44 `chunk_equiv` video cases need a stream accumulator, and the stop fold is CPU-only.** `chunkkinds.c:68-88` `cs_stop` folds time, registers, `io_buffer` and block state, not beam state (the digest covers it). The vid stream must be concatenated across calls and compared with `memcmp`
  (D-163): copy `ob_log_append` (`obskinds.c:694`) rather than hashing. `cs_table[]` (`chunkkinds.c:41-46`) has 7 rows and `CS_SCHED_MAX` is 8: add `1364`, `1366`, `vframe` (research F-20) and the case lists 5.
- **F-45 Dev-sink capacities after reads are logged.** `OB_DEV_LARGE` 4096 (`obskinds.c:501`), `DL_DEV_CAP` 64, `TRACE_DEV_CAP` 65536. `spd_speed` makes 1,280 reads of the VDC window; the 32 random programs of `randprog.c` map arbitrary banks and may read windows 0 and 1 many times; a lost record
  in a "large" configuration is `harness_failure` by rule. At 04-03 run the 32 programs once and compare `lost` before fixing `OB_DEV_LARGE` (raise it, do not weaken the rule).
- **F-46 Core boundary for the renderer.** `scripts/core-boundary.allow` lists `memcpy memmove memset memcmp` plus `bzero` and `__chkstk_darwin` on Darwin only. A stack record of at most 2,792 bytes plus a 64-entry sprite list stays under 4 KiB; a zero-initialised stack array compiles to `memset`
  or `bzero` (allowed). Tables are `static const` only (any writable symbol is CB_WRITABLE): the SCREEN size table, the CGY height table and the divider table are `static const` arrays.
- **F-47 Ledger edits.** `LED-CPU-0016` is `supported` (E1, pin `TEST:st_bypass`; its `withheld` already lists "any claim about what the VDC does with the stored byte"). The 04-01 amendment "ST reaches a live VDC" may touch `assumption`/`disagreement`/comments only; statement, evidence and status
  stay, or `SUPPORTED_*` rules and the README supported list (`docs-check:141-163`) go red. Amended seed entries keep their `origin` (`SUMMARY C2`, `C4`, `C5`, `C3`) because `ledger/REQUIRED` must equal it. A `constants.def` row needs its ledger entry FIRST (`inventory-check:149`, rule
  `INV_CONST_LEDGER`), so 04-01 lands before any row.
- **F-48 `lines_per_frame` is always bound at boot.** `ds_resolve` (profile.c, comment at 112-118) refuses an unbound open field (`open_unbound`) and checks the allowed set `262|263`; `runner.c:90` binds "262" by default. `ds_vdc_poweron` may therefore test `ds_streq(value, "263")` with 262 as the
  only other case and needs no third state, exactly like `ds_io_poweron` (io.c:52-57) tests `"pad2"`. `profiles/pce-hucard-2btn/r1.toml` is not edited (D-134).
- **F-49 `ds_obs_begin` and `ds_obs_end` have more callers than `ds_run`.** `run.c:108`, `obskinds.c:1416` and `:1508` call `ds_obs_begin(&log, NULL, &dsk)`; the third parameter edits all three. `ds_obs_end` (obs.c:210-216) writes each channel's header; for the vid channel `write_header` writes 32
  bytes (`count`, `lost`, `seq`, `bytes`, zero word) and keeps the `if (ch->on)` guard of obs.c:165, so a channel the request did not carry writes nothing.
- **F-50 ST immediates in Phase 3 fixtures now select real registers (verified list).** `st_bypass` settings 1-3 write AR = $01 (MARR) with ST2 as an MSB write (a read operation, only the read buffer changes) and setting 2 and 4 AR = $11 (DESR); none is LENR ($12) or DVSSR ($13), so no DMA starts.
  `cpu_huc` checks 54-57: `st0 #$A5` is AR 5 (CR), `st1 #$A6`/`st2 #$A7` make CR = $07A6, `st0 #$E8` is AR 8 (BYR); `cyc_probe` and `mmu_mpr` `st0 #$22` is AR 2 (VWR, no data write). Their reads go through MPR0 = $F8 (RAM) or MPR1, never the VDC window. Re-verify with the three-policy run at 04-04
  (D-178).

## File Classification

Waves of D-180: W1 04-01/04-02, W2 04-03, W3 04-04/04-05, W4 04-06..04-08, W5 04-09/04-10, W6 04-11, W7 04-12/04-13, W8 04-14.

| New/Modified file group | Role | Data flow | Closest analog | Match | Plan |
|---|---|---|---|---|---|
| `src/core/ds_vdc.h` (new) | private header | n/a | `src/core/ds_timer.h` | exact | 04-02 |
| `src/core/vce.c` (new) | device: window, CR, palette, divider table | request-response at a stamp | `src/core/timer.c` (write/read), `io.c` (poweron) | role-match | 04-03, 04-07 |
| `src/core/vdc.c` (new) | device: registers, VRAM accessor pair, port paths, DMA closed form, status | CRUD plus event-driven | `src/core/timer.c`, `mapper.c` (bounds-checked accessor), `irq.c` (validate then write) | role-match | 04-04, 04-08 |
| `src/core/beam.c` (new) | fold: line ladder, vertical machine, RCR, latches | event-driven closed form | `src/core/timer.c` `fold`, `cpu.c:560-572` | partial | 04-02, 04-03, 04-06, 04-07 |
| `src/core/vid.c` (new) | renderer (const) and record writer | transform, streaming | `src/core/obs.c` writers, `serial.c` `w_*` | partial | 04-03, 04-09, 04-12 |
| `src/core/obs.c`, `ds_obs.h`, `run.c` (mod) | third sink channel | streaming | themselves (`dev` channel) | exact | 04-02 |
| `src/core/ds_core.h` (mod) | instance, request, tiling | transform | itself | exact | 04-02, 04-03 |
| `src/core/serial.c`, `ds_serial.h` (mod) | serializer, 7 regions | transform | itself (`TIMR`, `IOPT`, `SCHD`) | exact | 04-03 |
| `src/core/snapshot.c`, `ds_snapshot.h` (mod) | restore, 7 decoders | transform, validate then commit | itself (`dec_timr`, `dec_blkt`, `dec_irqs`) | exact | 04-03 |
| `src/core/poweron.c`, `ds_poweron.h`, `profile_fields.def`, `boot.c` (mod) | power-on v3, VDC power-on | transform | itself (`apply_v2`, `ds_io_poweron`) | exact | 04-03 |
| `src/core/bus.c` (mod) | hardware-page windows 0 and 1 | request-response | itself (`hw_read`, `hw_write`) | exact | 04-02, 04-03, 04-04 |
| `src/core/irq.c`, `cpu.c` (mod) | IRQ1 level OR, fold placement | event-driven | itself | exact | 04-02, 04-06 |
| `model_params.def`, `constants.def`, `src/text/text.c`, `ds_text.h` | defs and printer | transform | themselves | exact | 04-03 and each |
| `tests/conformance/videokinds.c` (new) | kinds: video judge, fault sweep, agree, sink shapes, hash role | batch | `buskinds.c` `kind_st_bypass`, `guestkinds.c` `guest_once`, `obskinds.c` | role-match | 04-03, 04-05, 04-06 |
| `tests/conformance/vdckinds.c` (new) | kinds: vram, satb, sweep, regmask, pure, fold, snapshot, random, policy diff | batch | `cpukinds.c` `kind_timer_pure`, `snapkinds.c` | role-match | 04-04, 04-08, 04-11 |
| amended runner kinds (guest, chunk, obs, step, snap, dig, rec, prof, pow, randprog) | kinds | batch | themselves | exact | 04-02, 04-03 |
| `runner.c` `kinds[]`, `runner.h`, `CMakeLists.txt` | registration | config | themselves | exact | each |
| `tests/conformance/cases/*.toml`, `REQUIRED`, `golden/` | records | CRUD | `irq_ctl_pre`, `mutate_sweep_irq_ctl`, `chunk_equiv_irq_ctl_pre`, `observer_tim_timer` | exact | each |
| `fixtures/lib/video.inc` (new), `hucard.cfg` (mod) | guest include, linker cfg | file-I/O | `check.inc`, `startup.inc`, `hucard.cfg` | role-match | 04-03 |
| `fixtures/src/{12 ROMs}.s` and `.checks` | authored guest ROM | event-driven, streaming | `fixtures/src/irq_ctl.s` and `.checks` | role-match | each fixture plan |
| `fixtures/src/*.vexp` (new) | hand-derived literal data | CRUD | `tests/conformance/inputs/*.toml` (via `toml-subset.awk`) | partial | 04-05 and each |
| `fixtures/manifest/*.toml` (13 new) | record | CRUD | `irq_ctl.toml`, `mmu_mpr_384k.toml` | exact | each |
| `fixtures/lib/cites.txt`, `TOOLS.md` (mod) | registry | CRUD | themselves | exact | 04-01 |
| `scripts/video-check`, `scripts/lib/vexp-rules.awk` (new) | checker | batch | `scripts/input-check`, `scripts/lib/input-rules.awk` | exact | 04-05 |
| `tests/video/<case>/` (new mini-roots) | controls | batch | `tests/gate/input-check/<case>/` | exact | 04-05 |
| `scripts/check` (stage, selftest mutations) | gate | batch | itself (`stage_input_check*`, `mutation-docs-claim-word`) | exact | 04-05 |
| `scripts/fixture-build` (mod) | gate tool | file-I/O | itself (`write_observe`, `write_classes`, variants) | exact | 04-03, 04-05 |
| `scripts/conformance-run` (mod) | harness | batch | itself (`controls_mode`, `filter_cases`) | exact | 04-06 |
| `scripts/docs-check` (mod) | checker | transform | itself (line 125) | exact | 04-01 |
| `scripts/lib/fixture-lint-rules.awk`, `fixture-rules.awk` (mod) | rules | transform | themselves | exact | 04-01 |
| `tests/controls/<10 toys>/{RULE,EXPECT}` (new) | controls | transform | `trace-hook-mutates`, `timer-reload-off`, `irq-priority-swap` | exact | 04-03..04-13 |
| `ledger/entries/LED-*.toml` (17 new, 10 amended), `ledger/REQUIRED` | record | CRUD | `LED-VID-0001.toml`, `LED-IRQ-0001.toml` | exact | 04-01 |

## Pattern Assignments

### P1. `src/core/ds_vdc.h` (new private header; 04-02)

**Analog:** `src/core/ds_timer.h` (39 lines): SPDX block, a header comment that states what is pure and what mutates, include guard, `#include <stdint.h>` and `"ds_core.h"`, then one comment plus one prototype per entry point.

```c
/* src/core/ds_timer.h:11-16, 27-37 */
#ifndef DS_TIMER_H
#define DS_TIMER_H
#include <stdint.h>
#include "ds_core.h"
/* the pure query: the counter value and the number of borrows since the stored origin at the stamp ... */
void ds_timer_query(const struct ds_instance *inst, uint64_t stamp, uint8_t *counter, uint64_t *borrows);
void ds_timer_settle(struct ds_instance *inst, uint64_t stamp);
```
**Differs:** prototypes named in D-137: `ds_vdc_advance(inst, stamp, obs)`, `ds_vdc_flag(inst, bit)`, `ds_vdc_read/write`, `ds_vce_read/write`, `ds_vdc_poweron`. Includes only stddef, stdint, `ds_core.h` and `ds_obs.h` (core-boundary include rule: `scripts/core-boundary-check:124-129` accepts only
`stddef.h`, `stdint.h`, `stdbool.h` and `ds_*.h`/`*.def`). The header comment states "valid for a folded instance" for every pure query (F-21).

### P2. `src/core/vce.c` (new; 04-03 window, CR, palette; 04-07 divider)

**Analog:** `src/core/timer.c` for a register window with a stamp-carrying read and write, and a `_Static_assert` on every constant it reads; `src/core/io.c:52-57` for the poweron function.

```c
/* timer.c:19 - constants are asserted where they are used */
_Static_assert((uint32_t)DS_CONST_timer_hold_on_stop <= 1u, "timer_hold_on_stop is 0 or 1");
/* timer.c:60-67 - a read has no side effect, the register is selected by the low bits */
uint8_t ds_timer_read(const struct ds_instance *inst, uint32_t reg, uint64_t stamp)
{
    uint8_t v;
    (void)reg; /* both addresses of the window read the counter (D-109) */
    ds_timer_query(inst, stamp, &v, NULL);
    return v;
}
/* timer.c:91-95 - a write: fold first, then select the register by (reg & 1u) */
void ds_timer_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp)
{
    ds_timer_settle(inst, stamp);
    if ((reg & 1u) == 0u) {
        inst->timer_reload = (uint8_t)((uint32_t)value & 0x7Fu);
```
**Differs:**
- Decode is `offset & 7` (D-152): `$0400` CR (write), `$0402/$0403` CTA, `$0404/$0405` CTW/CTR; the unused offsets read `DS_CONST_vce_unused_read`. None touches `io_buffer`.
- Palette halves are stored as `uint16_t pal[512]` masked to `$1FF`; every narrowing is an explicit mask and cast (`-Wconversion -Werror`, sanitize preset has no `-fwrapv`): write `(uint16_t)(((uint32_t)pal & 0x100u) | (uint32_t)v)`.
- The divider is a pure function of CR bits 1:0 through the four constants `vce_div_cr0..3`; write it as ONE `static const uint8_t` array indexed by `cr & 3u` so the toy `vid-dot-div` has a single anchor statement (F-26, research Pitfall 8). It is latched at EV_LINE in `beam.c`, never read mid-line.
- The write handlers fold FIRST (`ds_vdc_advance(inst, stamp, obs)` is called by `bus.c`, not here), then change state, then set `line_dirty` bit 0 or 4 (D-161).

### P3. `src/core/vdc.c` (new; 04-04 port and VRAM; 04-08 DMA)

**Analog 1 (closed form on machine time, state changes only in the access handlers):** `src/core/timer.c:22-43` (the pure `fold`) and `:77-89` (`ds_timer_settle`), which is the model for the DMA progress closed form of D-151.

```c
/* timer.c:28-39 - elapsed steps from a stored origin; unsigned 64-bit division, no loop over clocks */
    if (inst->timer_enabled != 0u && stamp > inst->timer_origin) {
        n = (stamp - inst->timer_origin) / (uint64_t)DS_TIMER_STEP_MCLK;
        if (n <= v) { v = v - n; } else {
            uint64_t m = n - (v + UINT64_C(1));
            uint64_t period = (uint64_t)inst->timer_reload + UINT64_C(1);
```
**Analog 2 (bounds-checked accessor returning a status):** `src/core/mapper.c:62-87`: arguments checked first, an out-of-range index never reads, one named return code per refusal.

```c
/* mapper.c:74-86 */
    if (bank >= DS_BANK_LIMIT) { return (uint32_t)DS_MAP_UNMAPPED; }
    ...
    if (index >= size_row[cls].bytes || index >= DS_ROM_CAP) { return (uint32_t)DS_MAP_UNMAPPED; }
    *out = inst->rom[index];
    return (uint32_t)DS_MAP_OK;
```
**Analog 3 (validate every input, then write):** `src/core/irq.c:51-80` `ds_irq_stim_set`: all refusals first, the writes last, every slot written so the region is a function of the declared list alone.
**Differs:**
- ONE accessor pair for VRAM (D-147): `vram_read(inst, word)` returns `DS_CONST_vdc_oob_read` at or above `$8000`, `vram_write` drops it. All users (port, both DMAs, renderer) call it. The sweep case `vdc_vram_sweep` runs the 65,536 addresses under the sanitizers.
- VRAM is stored as 65,536 bytes little-endian (D-147) so the live span equals the stream payload; word access is `lo | (hi << 8)` on unsigned widened operands.
- Register stores are `uint16_t` masked by one `static const uint16_t reg_mask[20]` (D-146); a byte write updates one half then ANDs the mask.
- Flags are set only through `ds_vdc_flag(inst, bit)`, which applies the enable gate at the event instant (D-144). Write it as the single place that sets `status`.
- DMA progress is `words_due = (t - start) / word_mclk - done` capped at 65,536 (D-151, research "Code Examples"); the naive per-clock reference lives in the C test `vdc_advance_pure` (P15), not in the core.

### P4. `src/core/beam.c` (new; 04-02 skeleton, 04-03 grid and frame wrap, 04-06 ladder, 04-07 geometry)

**Analog:** no scheduler exists. Use `timer.c` for "closed form on machine time, unsigned 64-bit `/` and `%`, no loop over clocks" and `cpu.c:560-572` for where the fold is called relative to the sample.

```c
/* cpu.c:560-572 - the sample stamp is the start of the last cycle; the VDC fold goes BEFORE ds_irq_pending (F-24) */
static uint32_t sample_and_enter(struct ds_instance *inst, struct ds_obs_log *log, uint64_t start, uint32_t total, uint32_t mclk_per_cycle, uint32_t i_sample)
{
    uint32_t vector;
    if (i_sample == 0u) {
        vector = ds_irq_pending(inst, start + (uint64_t)total - (uint64_t)DS_CONST_irq_sample_cycles_before_end * (uint64_t)mclk_per_cycle);
```
**Differs:**
- The fold is a loop over EVENTS, not clocks: `while (next_event_time(inst) <= stamp) process one event`, ties in the order EV_LINE, EV_VD, EV_RCR (D-138). The next event is derived from `(stage, vd_pending, vd_off, rcr_off, line_idx)` plus `satb_done_at` and the DMA fields (research "Event model check"); nothing like `vdc_now` is stored.
- Every loop bound is architectural: events per call are bounded by `(stamp - machine_time) / 1365 + 3`, and the restore window rule (D-166) keeps a hostile `line_idx` from looping 2^63/1365 times.
- Time arithmetic is `uint64_t` with `UINT64_C(1365)` from `DS_CONST_line_mclk`; `line_idx * 1365` is computed only after the pass-1 bound `line_idx <= time / 1365 + 2` (F-34 a).
- A stamp at or below the folded point does nothing (F-29).
- The 04-03 shape (research Pattern 1): the public `ds_cpu_step_obs` wraps the static core and folds at `start + total - 1` (F-28).

### P5. `src/core/vid.c` (new; 04-03 record writer, 04-09 BG, 04-12 sprites)

**Analog:** `src/core/obs.c:14-30` (the byte-by-byte little-endian writers) and `:244-266` (`ds_obs_dev`, one fixed layout written field by field, spare bytes zeroed in a loop).

```c
/* obs.c:14-30 */
static void put_u16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8u) & 0xFFu);
}
static void put_u64(uint8_t *p, uint64_t v)
{
    put_u32(p, (uint32_t)(v & UINT64_C(0xFFFFFFFF)));
    put_u32(p + 4, (uint32_t)((v >> 32u) & UINT64_C(0xFFFFFFFF)));
}
/* obs.c:256-265 - fields written one by one, nothing through a struct cast */
    put_u64(r, stamp);
    put_u32(r + 8, phys);
    r[12] = (uint8_t)(value & 0xFFu);
    for (i = 18u; i < DS_DEV_RECORD_BYTES; i++) { r[i] = 0u; }
```
**Differs:**
- The record is DSVR v1 (D-161): 64-byte header at the offsets of D-161, then `u16 index[active_w]`, `u16 colour[active_w]`, zero padding to a multiple of 8. A table of `{offset, width}` rows with `_Static_assert` on the sum is the safest shape (D-177 "table-driven record writers").
- The render function takes `const struct ds_instance *` and an out-parameter and writes a stack record; the STATE part (BYR counter, sprite evaluation, OR/CR detection) always runs and the PIXEL part only when the vid sink is attached (D-155). The only reads of the sink pointer are the `want_pixels` gate and the emit call (D-164).
- A `memcpy` of zero bytes through a NULL pointer is a UBSan finding on Linux (Phase 3, `obskinds.c:854` comment): never copy a header-only record through a NULL payload pointer.
- No RGB, no float, no host colour (D-154); `vce_cr` is carried in the record, never applied.

### P6. `src/core/obs.c`, `ds_obs.h`, `run.c` (modify; plan 04-02): the third channel

**Analog:** the `dev` channel itself. The table-driven shape is already half there: `struct sink_ref` (obs.c:52-63), `sink_ref_insn`/`sink_ref_dev` (65-91), `record_range_ok` (94-105), and the three-step order of `ds_obs_sinks_check` (struct checks, capacities, record ranges).

```c
/* obs.c:109-123 - arrays of two and loops to 2u become three; the ORDER (structs, capacities, ranges) is behaviour */
    struct sink_ref s[2];
    sink_ref_insn(&s[0], request->insn);
    sink_ref_dev(&s[1], request->dev);
    for (i = 0u; i < 2u; i++) {
        if (s[i].present && ((s[i].obj % (uintptr_t)s[i].obj_align) != 0u || touches_call(s[i].obj, s[i].obj_bytes, inst, request, result))) {
            return (uint32_t)DS_RUNREF_memory_invalid;
```
```c
/* obs.c:184-202 - the offer, counted in seq either way; drop-newest. The vid twin needs a byte capacity and a sticky full flag (F-2) */
    ch->seq = ch->seq + UINT64_C(1);
    if (ch->count >= ch->capacity) {
        if (ch->lost < UINT32_MAX) { ch->lost = ch->lost + 1u; }
        return NULL;
    }
    r = ch->records + (size_t)ch->count * record_bytes;
```
**Differs:**
- `struct ds_vid_sink { uint8_t header[32]; uint8_t *bytes; uint32_t capacity; }` in `ds_obs.h` (D-162); `DS_VID_SINK_MAX_BYTES` 64 MiB (a capacity above it is `run.sink_unsupported`, the same code as the others); `_Static_assert` on `DS_VID_MAX_DOTS` 682 against `DS_CONST_line_mclk`.
- The channel stores whole records only: `count` records, `bytes` stored, a sticky `full` once one record does not fit, so a later smaller header-only record cannot slip in (F-2). `lost = seq - count`.
- Keep `obs.c:226` untouched (F-26) and give the vid writer its own slot function.
- `ds_run` (run.c:106-109): the test `request->insn != NULL || request->dev != NULL` also tests `request->vid`, and `ds_obs_begin` takes the third parameter (F-49). The anchor `if (inst->machine_time >= request->until_mclk) {` at run.c:123 must stay unique.
- `struct ds_run_request` gets `struct ds_vid_sink *vid` as the LAST member (ds_core.h:317-325); every local in the repo is already zeroed by `memset` or `rr_base_request` (research F-16).

### P7. `src/core/ds_core.h` (modify; 04-02 request, 04-03 instance and tiling)

**Analog:** itself. The end of the instance and the tiling list are the model.

```c
/* ds_core.h:183-188 - the last members; the new block goes AFTER tail_pad (research F-10) */
    uint8_t o_latch;
    uint8_t slot_type[DS_PORT_COUNT];
    uint8_t rom[DS_ROM_CAP];
    uint8_t tail_pad[6];
};
/* ds_core.h:223-232 - every member in the tiling list, the sum asserted against sizeof(struct ds_instance) */
#define DS_INSTANCE_TILING(X) \
    X(machine_time) X(profile) ... X(o_latch) X(slot_type) X(rom) X(tail_pad)
_Static_assert(0u DS_INSTANCE_TILING(DS_TILING_SIZE) == sizeof(struct ds_instance), "the tiling list does not account for every byte ...");
```
**Differs:**
- New block after `tail_pad`: `uint64_t` members first, then `u32`, `u16`, `u8`, then an explicit named trailing pad so the total is a multiple of 8. Keep the line `    uint8_t tail_pad[6];` byte for byte (selftest `mutation-tiling-entry`, scripts/check:2842-2847, greps exactly that line).
- Adjacency `_Static_assert`s in the style of lines 191-216 for every region that is one live span.
- `DS_VDC_STUB_READ` and `DS_VCE_STUB_READ` (ds_core.h:290-292) are removed with their only users (bus.c:123,125); `DS_BEHAVIOUR_EPOCH` 4 to 5 (line 22) in 04-03 only.

### P8. `src/core/serial.c`, `ds_serial.h` (modify; 04-03): seven regions

**Analog:** itself; the last three regions (`TIMR` 11 bytes, `IOPT` 6, `SCHD` 16,392) are the model for a fixed-length region that is one live span with explicit little-endian fields.

```c
/* serial.c:185-201 - one row per region: tag, payload length, live offset, live length */
    [R_TIMR] = {"TIMR", 11u, offsetof(struct ds_instance, timer_origin), offsetof(struct ds_instance, timer_enabled) + 1u - offsetof(struct ds_instance, timer_origin)},
/* serial.c:166 - a live-span size assert next to the rows */
_Static_assert(offsetof(struct ds_instance, timer_enabled) + 1u - offsetof(struct ds_instance, timer_origin) == 11u, "the timer fields are not the 11 bytes of the TIMR payload");
/* serial.c:247-252 - the payload, one explicit field per line */
    case R_TIMR:
        w_u64(w, inst->timer_origin);
        w_u8(w, inst->timer_counter);
```
**Differs:** seven rows appended after `R_SCHD` (`VCE0 PALT VDC0 BEAM VDMA VRAM SATB`, 21 regions, `DS_REGION_COUNT` 14 to 21 at `ds_serial.h:41`, the assert at `serial.c:336` follows). `PALT` (1,024 bytes) and `VRAM` (65,536) and `SATB` (512) are byte arrays; the `u16` palette is written
little-endian from the member. The stream order of BEAM (D-165: `u8` before `u16` fields) may differ from the struct order, but each live span stays contiguous and fully carried. `DS_SERIAL_VERSION` 7 to 8 (`ds_serial.h:22`); update the region list comment at line 7 and the size mention at
`ds_snapshot.h:30`.

### P9. `src/core/snapshot.c` (modify; 04-03): seven decoders and the hostile corpus

**Analog:** `dec_timr` (snapshot.c:352-370) for a decoder that judges a value against another region read earlier; `dec_blkt` (426-456) for cross-field rules; `dec_irqc` (372-389) for model selections.

```c
/* snapshot.c:352-369 - read all fields, commit or judge; v->time came from dec_time in pass 1 */
    uint64_t origin = r_u64(r);
    ...
    if (commit) {
        inst->timer_origin = origin;
        ...
        return (uint32_t)DS_SNAPREF_NONE;
    }
    return counter > 127u || reload > 127u || enabled > 1u || origin > v->time ? (uint32_t)DS_SNAPREF_field : (uint32_t)DS_SNAPREF_NONE;
```
```c
/* snapshot.c:550-556 and :590 - the table the walk indexes; the tag cross-check is the only guard */
} decoders[DS_REGION_COUNT] = {
    {"PROF", dec_prof}, ... {"IOPT", dec_iopt}, {"SCHD", dec_schd},
```
**Differs:** judgements of D-166 plus F-1 (`v_togo <= 512`, `display_k < 512`, no clamp), the window rules checked BEFORE any multiplication, `rcr_counter <= $7FFF`, `frame_lines in {262, 263}`. `dec_powr` accepts policy 4. The hostile corpus (`snapshot_hostile`, `snapkinds.c`) flips every new
control field and samples VRAM bytes; it does not perturb all 65,536 (research F-22). Keep `inst->blk_active = active;` and the other toy anchors unique (F-26).

### P10. `src/core/poweron.c`, `ds_poweron.h`, `boot.c` (modify; 04-03): v3 and the VDC power-on

**Analog:** `apply_v2` and `ds_poweron_apply` for the policy, `ds_io_poweron` for the "constant under every policy" fields.

```c
/* poweron.c:29-34 - ASCII-packed constants, never ordinals */
static const uint64_t domain_const[DS_DOMAIN_COUNT] = {
    UINT64_C(0x52414D384B5F5F5F), /* ram8k */
    UINT64_C(0x4350555F5F5F5F5F), /* cpu: "CPU_____" */
/* poweron.c:143-155 */
void ds_poweron_apply(struct ds_instance *inst)
{
    if (ds_policy_is_seeded(inst->powerup_policy) != 0u) { ... ds_stream_fill(&ram, inst->ram8k, (size_t)DS_RAM8K_BYTES); }
    if (inst->powerup_policy == (uint32_t)DS_POLICY_SEEDED_V2) { apply_v2(inst); }
}
/* io.c:52-57 and boot.c:357-358 - constants set after the zeroing, before the CPU reset */
    ds_poweron_apply(inst);
    ds_io_poweron(inst);
```
**Differs:** `DS_POLICY_SEEDED_V3 = 4` and domains `VRAM____`, `SATB____`, `PAL_____` (`"VRAM____"` is `0x5652414D5F5F5F5F`, `"SATB____"` `0x534154425F5F5F5F`, `"PAL_____"` `0x50414C5F5F5F5F5F`; recompute with the host tool when writing, do not trust this note); v3 draws every v2 draw first, then
the three streams straight into the arrays; the palette draws 1,024 bytes then clears bits 1..7 of every odd byte. A v3 policy has `ds_policy_is_seeded` 1 and version 3, and `apply` tests `>= V2` for the v2 draws (not `==`). `ds_vdc_poweron(inst)` is called after `ds_io_poweron`: `rcr_counter =
16384`, `frame_lines` from `lines_per_frame` (F-48), every other VDC/VCE/beam byte stays 0 (F-11 of the research: BEAM is not all zero at boot). `boot.c:345` `__builtin_memset` already zeroes the larger instance.

### P11. `src/core/bus.c` (modify; 04-02 fold, 04-03 VCE and read log, 04-04 VDC)

**Analog:** itself. The only choke point is `hw_read`/`hw_write`; both already compute the stamp after the penalty.

```c
/* bus.c:113-126 and :142-146 - the shape to keep (F-27) */
static uint8_t hw_read(struct ds_instance *inst, struct ds_bus *bus, uint32_t offset)
{
    ...
    charge_penalty(bus, offset);
    stamp = access_stamp(inst, bus, 1);
    switch ((enum hw_window)(offset >> 10u)) {
    case HW_VDC:
        return (uint8_t)DS_VDC_STUB_READ;
    case HW_VCE:
        return (uint8_t)DS_VCE_STUB_READ;
    ...
    inst->io_buffer = value;
    if ((enum hw_window)(offset >> 10u) == HW_IO) { /* the $1000 window is the one whose reads are logged (D-119) */
        log_dev(inst, bus, stamp, offset, value, (uint32_t)DS_DEV_READ);
    }
```
```c
/* bus.c:150-176 - hw_write splits the default case into VDC and VCE, and the logging at 177 already covers windows 0 to 5 */
    default: /* the VDC and VCE windows (stubs: the write is dropped) and the empty windows */
        break;
```
**Differs:**
- `case HW_VDC:` and `case HW_VCE:` fold (`ds_vdc_advance(inst, stamp, bus != NULL ? bus->obs : NULL)`), read, `log_dev(... DS_DEV_READ)`, then `return` (F-27). `$1403` (`HW_IRQ`, `(reg & 3u) == 3u`) folds before `ds_irq_read`; `$1402` and `$1400/$1401` do not.
- `hw_write` windows 0 and 1 fold, then call `ds_vdc_write` or `ds_vce_write`; the ST path (`ds_bus_write_phys`, line 224-229) already reaches `hw_write`, so ST0/ST1/ST2 need no new code, only the comment at line 227.
- The fold target is the `stamp` of the access, never recomputed (research Pitfall 2).

### P12. `src/core/irq.c`, `cpu.c` (modify; 04-02 fold points, 04-06 level)

**Analog:** itself.

```c
/* irq.c:97-100 - the level is ORed into the IRQ1 bit; the VDC part reads folded state and ignores the stamp (F-21) */
static uint32_t request_bits(const struct ds_instance *inst, uint64_t stamp)
{
    return (stim_level(inst, 2u, stamp) != 0u ? IRQ_BIT_IRQ2 : 0u) | (stim_level(inst, 1u, stamp) != 0u ? IRQ_BIT_IRQ1 : 0u) | (ds_timer_tiq(inst, stamp) != 0u ? IRQ_BIT_TIQ : 0u);
}
```
**Differs:** `stim_level(inst, 1u, stamp) != 0u || (inst->vdc_status & 0x3Fu) != 0u` for the IRQ1 bit (a line over 400 characters when written in full: split it into a local). The irq.c header comment (lines 8-9, "Nothing drives them yet (the VDC arrives in Phase 4)") is rewritten.
`ds_irq_pending_query` stays const; document "valid for a folded instance" in `ds_core.h` and never call it past the fold in a new test (the stop callbacks at `obskinds.c:804` and `chunkkinds.c:184` call it at `machine_time`, which the step-end fold has reached minus 1).

### P13. `model_params.def`, `constants.def`, `profile_fields.def`, `src/text/text.c`, `ds_text.h`

**Analog:** the rows themselves.

```c
/* model_params.def:20-21 */
DS_MODEL(tma_zero, "bus_latch|a_unchanged", "bus_latch", "LED-CPU-0015", "assumed")
DS_MODEL(irq_i_sample, "pre|post", "pre", "LED-IRQ-0005", "open")
/* constants.def:42-45 - decimal literal, no suffix, a single ledger id */
DS_CONST(block_irq_after_epilogue, 1, "LED-IRQ-0007")
DS_CONST(pad_settle_mclk, 0, "LED-IO-0004")
```
**Differs:** two model rows `vd_irq_phase "line_start|hdw_start"` (LED-IRQ-0001, open) and `rcr_origin "first_active|line_before"` (LED-IRQ-0010, open) in 04-03; 16 constants rows each landing with the plan that reads it, each ledger id present first (F-47). `emit_model` (text.c:432-453) and
`emit_constants` (571-584) are generic and need no edit; `emit_withheld` (355-393) gains ids by itself through the union of model and constant ledger ids (cap `WH_MAX 64`, today 26). `ds_text.h:36` 6 to 7 in 04-03 only. F-35 and F-36 apply.

### P14. `tests/conformance/videokinds.c` (new; kinds `video`, `video_fault_sweep`, `video_hash_not_verdict`, `vdc_irq_agree`, `vid_sink_shapes`)

**Analog 1 (boot a built ROM, attach a sink, run to done, judge DSV1, read the sink log):** `tests/conformance/buskinds.c:177-264` `kind_st_bypass`.

```c
/* buskinds.c:202-231 - heap sink, boot a built ROM, stop at done as a speed aid, judge the verdict, release */
    dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    drec = (unsigned char *)calloc((size_t)SB_DEV_CAP, DS_DEV_RECORD_BYTES);
    ...
    err = boot_built_rom(rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    dsk->records = drec;
    dsk->capacity = SB_DEV_CAP;
    rq.stop = dsv1_done; /* a speed aid only (D-95): until_mclk is the hard bound */
    rq.dev = dsk;
    if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of st_bypass");
    } else {
        note_run(c, res.machine_time, res.reason);
        judge_dsv1(c, ci->ram8k + V_OFF, (unsigned)id, (unsigned)checks, verdict, sizeof verdict);
```
**Analog 2 (the `--video-out` rights rule and a CLI hook that lives outside runner.c):** `obskinds.c:1543-1649` (`trace_fixture_permit`, `trace_authored_listed`, `obskinds_arg` at 1554-1562; called from `runner.c:1251`; declared at `runner.h:340-343`). Add `videokinds_arg` beside `obskinds_arg`;
refuse unless the fixture is listed in `build/gate/fixtures-authored.used` or a synthetic recipe.
**Analog 3 (SHA-256 of a captured stream through the core's own hash helper, comparison outside):** `tests/conformance/digkinds.c` `capture` (lines 40-60) and `ds_sha256.h`.
**Differs:**
- Records are parsed with hand-written offsets; the runner never includes a core record header (ER-13). The stream is `"duospool-video"` NUL, a u32 version, then the records (D-163); the digest and the text form exist only here.
- Literals come from `build/gate/fixtures/NAME.vexp.tsv` (F-38, research F-5); no expected value is produced by the core, another emulator or a hash (D-169).
- The judge forms ROWS, REL, STRUCT, the roles `literal`/`struct`/`golden`, and the fault list F0..F9 of D-172 live here. The evidence text prints `video_sha256=... E0 repeatability` through `set_evidence_detail` beside the literal verdict (ER-11).
- Every video case declares `irq_stim` none and asserts events = 0 (D-144). A missing sink or a null renderer is `harness_failure` or red, never a pass.
- F-37: the closed form uses `res.machine_time`; `lost != 0` is `harness_error`.

### P15. `tests/conformance/vdckinds.c` (new; C-level cases of D-175)

**Analog:** `tests/conformance/cpukinds.c:419-570` `timer_pure`: a naive per-clock reference model that shares no formula with the unit, driven at samples before, at and after every boundary and compared to the unit's pure query and borrow count.

```c
/* cpukinds.c:437-464 - the model moves to stamp t one master clock at a time; no formula of timer.c */
static void tp_to(struct tp_model *m, uint64_t t)
{
    ...
    while (now < t) {
        now++;
        prescale++;
        if (prescale == TP_STEP) {
            prescale = 0u;
            if (counter == 0u) { /* a step with the counter at 0 borrows and reloads */
/* cpukinds.c:487-506 - one comparison helper, counts compared and bad, remembers the first fault text */
static void tp_cmp(struct ds_instance *inst, struct tp_model *m, uint64_t t, int with_borrows, int settle, struct tp_stat *st, const char *what)
```
**Differs:**
- `vdc_advance_pure` compares the DMA closed form of D-151 with a per-dot reference over random stamp partitions, and `vdc_fold` runs random chunk sequences against one jump with accesses at event time +-1 and the next-event invariant.
- `vdc_vram`, `vdc_satb` read `ci->vram` and `ci->satb` bytes directly against literals restated from the fixture source; this is the D-108 precedent (kinds read `inst->ram8k`, never a peek API). Compare 16-bit words by assembling bytes in the test, not by casting the array.
- `vdc_snapshot_mid_dma` and the restore stops of D-166 reuse the helpers of `snapkinds.c` (`snap_poison`, `snap_first_diff`, F-34); `vdc_random_regs` and `vdc_regmask` run under the sanitizers.
- Evidence class E0 throughout; no literal from a DuoSpool run (ER-7).

### P16. Amended runner kinds (04-02 and 04-03): exact sites

| File and lines | Edit |
|---|---|
| `guestkinds.c:840`, `:1081-1101` | `default_policies` becomes `""`, `bind.powerup_policy=seeded-prng-v3;seed=1`, `...;seed=2` (three strings, `GP_DEFAULT_COUNT` stays 3); a video case adds a `vid` sink next to `run.dev` and treats `lost != 0` as `harness_failure` |
| `obskinds.c:509-525`, `:725`, `:737-869`, `:1386-1522` | F-31, F-30, F-49; `ob_exec` allocates the vid sink and guard bytes like the dev block; `dl_windows` read loop |
| `stepkinds.c:477-493`, `:574-581`, `:600-736` | F-32: vid overlap controls per channel, `sink_unsupported` above 64 MiB, the OLD-size variant |
| `chunkkinds.c:41-46`, `:68-88` | F-44 |
| `snapkinds.c:31-43`, `:79-90`, `:848` (a loop over `DS_REGION_COUNT`), `:2068` (`kind_snapshot_hostile`) | `SK_VERSION 8`, `SK_FRAMES 21`, `SK_EXPECTED_SIZE` (92,255 per research, exact at the tracer), `sk_frames[]` and the `RG_*` enum gain seven rows; `sk_is_identity` only if a pad member is added |
| `digkinds.c:40`, `:147`, `:461` | `DK_CAP 32768u` to at least 131072u; `DS_REGIONS_MAX 16u` to at least 24u; `digest_region` uses first-and-last flips for `VRAM`, `SATB` and `PALT` (research F-22) |
| `reckinds.c:247-248` | `format_version = 7`, `behaviour_epoch = 5` |
| `profkinds.c:188` | allowed string gains `seeded-prng-v3` |
| `powkinds.c` header line 11 | grammar `vram@N=`, `satb@N=`, `pal@N=` (F-33) |
| `randprog.c` (139 lines) | VDC/VCE traffic only through the hardware page with MPR0 = $FF; never feed `$1403` timing assumptions |

### P17. Kind registration (every plan that adds a kind)

**Analog:** `tests/conformance/runner.c:1009-1081` `kinds[]` rows, `runner.h:340-346` declarations, `CMakeLists.txt:63-69` source list. Three edits per kind:

```c
/* runner.c:1058-1060 */
    {"digest_stream", kind_digest_stream, 0},
    {"digest_pair", kind_digest_pair, 0},
    {"digest_region", kind_digest_region, 0},
```
```cmake
# CMakeLists.txt:63-69 - an explicit list: videokinds.c and vdckinds.c are added here (the toys reuse the variable)
set(DS_RUNNER_SOURCES
  tests/conformance/runner.c ... tests/conformance/reviewkinds.c)
```
**Differs:** core sources are a `file(GLOB ... src/core/*.c)` with `CONFIGURE_DEPENDS` (CMakeLists.txt:38), so `vdc.c vce.c beam.c vid.c` need no CMake edit, but a new toy's `file=` must name one of them. Each kind needs a `REQUIRED` id and a case file in the same change (`INV_UNLISTED_CASE`,
`INV_REQUIRED_MISSING`). F-18: 04-04 and 04-05 both edit `runner.c`, `runner.h`, `REQUIRED`; create `videokinds.c` in 04-03 and `vdckinds.c` in 04-04 so only one plan edits each file.

### P18. Cases, REQUIRED, goldens (every plan)

**Analog:** `tests/conformance/cases/irq_ctl_pre.toml` (a guest case), `mutate_sweep_irq_ctl.toml`, `chunk_equiv_irq_ctl_pre.toml`, `observer_tim_timer.toml`; `tests/conformance/REQUIRED` (one id per line).

```toml
# chunk_equiv_irq_ctl_pre.toml - the keys of a chunk case; a video case adds the new tokens (F-44) and no irq_stim
id = "chunk_equiv_irq_ctl_pre"
kind = "chunk_equiv"
fixture = "authored:irq-ctl"
expect = "compare=verdict,machine_time,stops,digest"
until_mclk = 4700000
chunks = "1,7,168,3072,frame,tick-1,tick+1"
```
```toml
# observer_tim_timer.toml - reuse the sink names for the vid channel (F-31)
sinks = "large,drop4,poison,cap0,cap1"
chunks = "whole,chunk:7,stop:5000"
```
**Differs:** video chunk schedules are `7`, `1364`, `1366`, `3072`, `vframe` (D-176; `frame` = 357,368 is a historical constant, not 262 x 1365). The three policies of a guest or video case come from the default list, so a case with an explicit `policies` key needs a `meaning` starting
`policies-override:` (guest kind guard). Goldens (`golden/accept_256k_zero.record.toml`, `accept_384k_seeded.record.toml`) and the five `digest_stream_*` literals follow the regeneration procedure of D-165 and research Pitfall 1: check each OLD literal equals `shasum -a 256` of the OLD stream,
extend each case comment with one sentence naming the method (the style of `digest_stream_prefix.toml` lines 4-12), hand-write the epoch literal. `scripts/lib/inventory-rules.awk` is NOT touched (no new case key).

### P19. `fixtures/lib/video.inc` (new) and the 12 fixture sources (04-03 onward)

**Analog:** `fixtures/src/irq_ctl.s` for the interrupt-driven structure, `fixtures/lib/check.inc` for the macros, `fixtures/lib/startup.inc` for the reset code and vector slots, `fixtures/lib/verdict.inc` for DSV1.

```asm
; irq_ctl.s:17-24 - the shape of every fixture
DSV_ID = 11
DSV_CHECKS = 33
        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        CHECK_TABLE DSV_CHECKS
        LIT 1, $00
```
```asm
; irq_ctl.s:99-113 - map the hardware page, install the handlers into the RAM vector slots (startup.inc points them at err)
fixture_main:
        lda #$FF
        tam #SL3                        ; MPR3 = $FF: the hardware page at logical $6000
        lda #<irq1_h
        sta a:VEC_IRQ1                  ; the IRQ1 vector jumps through this slot (startup.inc)
        lda #>irq1_h
        sta a:VEC_IRQ1+1
```
```asm
; irq_ctl.s:80-97 - counted delay and bounded polling; the video spin counter is a variant with a 24-bit clc/adc and no data-dependent branch (D-174)
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro
```
```asm
; check.inc:51-56 and :58-65 - the macros a check uses (one byte observed, one byte expected)
        .macro CHECK_LIT k
        ldx #((k) - 1)
        sta a:VERDICT+$80,x             ; the observed byte
        cmp a:.ident(.concat("cklit_", .string(k))) + 1
        CHECK_RESULT
```
**`.checks` format** (`irq_ctl.checks:4-12`): `#` header, then `id=N` and `checks=N`, then one TAB-separated row per check: index, slug, class tag (`MAN:H7-4`, `GUEST:model`, `ASSUMED:LED-VID-0004`), meaning, and optionally the fifth column `observe`.
**Differs:**
- Every source starts with the 0BSD SPDX pair in the first five lines using `;` (`licence-check` is a fixed-string match on all of `fixtures/`), `.setcpu "huc6280"`, and never a line-start `; Provenance:` (Phase 3 F8).
- Operands below $100 carry `z:`, `a:` or `<` (rule `FXL_SIZE_HINT`); only mnemonics of `tests/oracle/huc6280-opcodes.tsv` (ST0/ST1/ST2, TIA/TAI, SAX, etc. are in it) and macros defined in `fixtures/lib` appear.
- VDC handlers write `VEC_IRQ1`, read status ($6000 with MPR3 = $FF) to acknowledge, and store the 24-bit counter and the status byte into cells above `$2410` (outside the verdict block, stack and vector slots; `irq_ctl.s:65-76` lists a safe cell map). Every cell a check reads is written first (D-127).
- Tile and sprite data are `.word` tables whose ASCII-art is the comment above them, or one-line formulas (D-169); `RODATA` holds tables (budget `ROMB` 4,086 bytes minus `5 + 2n` for CHECKLIT, F-40).
- GUEST checks recompute with different arithmetic than the writer (additive versus multiplicative, forward versus reverse), one palette bijection only (research F-25), and use `GUEST:` or `ASSUMED:` for derived rows (F-39).
- `vdc_irq`: `.ifndef LINES` default 262 and one `-D LINES=263` variant (F-38).

### P20. `fixtures/manifest/*.toml` (13 new) and `cites.txt`, `TOOLS.md`, rules

**Analog:** `fixtures/manifest/irq_ctl.toml` and `mmu_mpr_384k.toml` (the variant: its `recipe` carries `-D ROMBANKS=48`); `fixtures/lib/cites.txt:13-35` for rows.

```toml
# irq_ctl.toml - the keys; kind authored, redistribution generated-not-committed, size and sha256 pinned after the first build
id = "irq_ctl"
kind = "authored"
source = "fixtures/src/irq_ctl.s"
recipe = "irq-ctl: ca65 --cpu huc6280 -D ROMBANKS=32 -I fixtures/lib -o build/gate/fx/irq-ctl.o fixtures/src/irq_ctl.s; ld65 -C fixtures/lib/hucard.cfg -D ROMBANKS=32 -o build/gate/fixtures/irq-ctl.pce build/gate/fx/irq-ctl.o"
size = 262144
verdict_source = "manual-cite:H8-5+manual-cite:H8-12+...+spec-literal:LED-IRQ-0005+spec-literal:LED-IRQ-0008"
```
```text
cites.txt row: LABEL TAB img|ocr TAB verifier TAB date TAB short numeric fact (no manual text, ER-9)
```
**Differs:**
- `scripts/lib/fixture-rules.awk:62` accepts `manual-cite:(H8|S8|BIOS)-n` only: add `H7` and `H6` (research table "Scripts, fixtures, ledger"; not in the CONTEXT list). `spec-literal:LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-nnnn` already accepts VID and IRQ.
- `scripts/lib/fixture-lint-rules.awk:56`: `islabel` regex gains `H7-[0-9]+` and `H6-[0-9]+`; the header comment says labels are H8, S8 and CDBIOS, update it.
- The `sha256` pin is measured on the first build and proves repeatability only; `fixture-check` compares size and hash with the used list, so 13 images need 13 entries (F-38).
- `cites.txt`: H7 label n is PDF page n+3 and H6 label n is n+2 (CONTEXT D-179); record the page errata of the research (VDW in the R0E row, "more than 17" versus "17 or more", A0 = 0 on H7-25). Only `img` rows may back a `supported` entry (rule `FXL_OCR_BACKS_SUPPORTED`).

### P21. `scripts/video-check` and `scripts/lib/vexp-rules.awk` (new; 04-05)

**Analog:** `scripts/input-check` (134 lines) with `scripts/lib/input-rules.awk`: stage 1 is `toml-subset.awk` (the only parser of hand-edited data, D-51), stage 2 the rules file, each writing to a file whose status is tested, a closing sentinel required from each, `--root`, `--expect`, `EXPECT_OK`, `STATUS ... ok`.

```sh
# input-check:89-107 - stage 1 then stage 2, dead stages are exit 2, never a pass
  if awk -f "$here/lib/toml-subset.awk" "$f" > "$t1" 2> "$tmp/stage1.err"; then :; else
    dead "stage 1 (toml-subset.awk) failed on $rel"
  fi
  if ! tail -n 1 "$t1" | grep -Eq "^END${tab}[0-9]+\$" || [ "$(grep -c "^END${tab}" "$t1" || true)" -ne 1 ]; then
    dead "stage 1 (toml-subset.awk) printed no closing END row for $rel"
  fi
  if awk -f "$here/lib/input-rules.awk" "$t1" > "$t2" 2> "$tmp/stage2.err"; then :; else
    dead "stage 2 (input-rules.awk) failed on $rel"
  fi
```
```sh
# input-check:112-122 - the --expect contract of every checker
  cut -f2 "$viol" > "$tmp/got.codes"
  ...
  if cmp -s "$tmp/got" "$tmp/want"; then
    echo "EXPECT_OK"
    exit 0
  fi
```
```awk
# input-rules.awk:46-62 - the stage 2 record reader and the one-statement-per-code tagging
$1 == "END" { nsent++; next }
$1 == "ERR" { next }
NF == 5 { ... }
{ viol("INP_FORMAT", NR, "an unexpected stage 1 row") }  # rule-stage1-row
```
**Differs:**
- Codes (F-6 of the research): `INV_VIDEO_HASH_ONLY`, `INV_VIDEO_GROUP_UNKILLED`, `INV_TOY_NO_LITERAL`, `VIDEO_GROUP_VACUOUS` and the `.vexp` grammar codes; the script prints them verbatim; `inventory-check` is NOT edited.
- `.vexp` limits: at most 512 rows, 400 characters per line, 16 KiB per file, unknown keys fail closed (D-171); the toml-subset constraints already give ASCII, tables `[a-z][a-z0-9_]*`, no TAB inside a value, one-line arrays.
- Exit contract 0 pass, 1 violation or unmet `--expect`, 2 usage or harness error; empty `--expect` is `HARNESS_EMPTY_EXPECT` exit 2; `STATUS video-check ok n=N` is the marker `checker()` requires (`scripts/check:35`).
- New files under `scripts/` need the MIT OR Apache pair; `.awk` too. Test under BWK awk, gawk and mawk (POSIX awk only: no `gensub`, `asort`, regex intervals).
- Each emitting statement ends in a `# rule-NAME` tag for the three selftest mutations (P22).

### P22. `scripts/check` (04-05): the stage pair, the negatives, three selftest mutations

**Analog:** `stage_input_check` and `stage_input_check_negatives` (`scripts/check:962-969`), `run_stage` registration (3454-3465), `mutation-docs-claim-word` (2374-2400).

```sh
# scripts/check:962-969
stage_input_check() {
  checker sh "$here/input-check"
}
stage_input_check_negatives() {
  run_controls --ok-prefix --require-negative tests/gate/input-check tests/gate/input-check sh "$here/input-check" --root @DIR@ @EXPECT@
}
```
```sh
# scripts/check:3454-3457 - registration order: static checkers before the build, after fixture-lint-negatives
run_stage fixture-lint stage_fixture_lint
run_stage fixture-lint-negatives stage_fixture_lint_negatives
run_stage input-check stage_input_check
run_stage input-check-negatives stage_input_check_negatives
```
```sh
# scripts/check:2388-2393 - the rule-deletion selftest: copy scripts/, delete the tagged lines, the control must stop being met
      before=$(grep -c "$cw_tag" "$cp_dir/docs-check" || true)
      grep -v "$cw_tag" "$cp_dir/docs-check" > "$cp_dir/docs-check.mutated" || true
      ...
      if [ "$st_alive" -eq 1 ] && [ "$before" -ge 1 ] && [ "$after" -eq 0 ] && [ "$r" -eq 1 ]; then
```
**Differs:** F-42 (one or two `run_stage` lines). Mini-roots under `tests/video/<case>/` follow `tests/gate/input-check/<case>/`: an `EXPECT` file with the exact codes, `ok-<case>` must pass without `EXPECT` (`--ok-prefix`), at least one must-fail (`--require-negative`), and a case tree below it
(`tests/video/<case>/fixtures/src/NAME.vexp`, a `tests/conformance/cases/` and `tests/controls/` slice, `tests/conformance/VIDEO_TOY_EXEMPT`). Three new `mutation-*` blocks (`rule-video-hash-only`, `rule-video-group-vacuous`, `rule-vexp-class-lint`) add three `SELFTEST` lines to the Phase 3 count.
`VIDEO_TOY_EXEMPT` is not scanned by `licence-check` (not .c, .h or .def).

### P23. `scripts/fixture-build`, `scripts/conformance-run`, `scripts/docs-check`

**Analog:** themselves.

```sh
# fixture-build:46-56 - the variants list: STEM NAME ROMBANKS FLAGS
variants='ds_proto ds-proto-predone 32 -D DSV_PREDONE
...
mmu_mpr mmu-mpr-384k 48
cpu_blk cpu-blk-irq 32 -D BLK_IRQ'
# fixture-build:125-144 - write_classes: awk with its status tested, nothing on the left of a pipe, exit 2 on any malformed row
```
```sh
# conformance-run:463 and :530-535 - ids come from column 1; the red check is per id
    awk '/^[ \t]*(#.*)?$/ { next } { print $1 }' "${cm_dir}EXPECT" > "$cm_bd/controls/$cm_name.ids"
    ...
      if ! awk -v id="$cm_id" '$1 == "RESULT" && $2 == id && $3 == "guest_failure" { f = 1 } END { exit !f }' "$cm_out"; then
```
```sh
# docs-check:123-137 - one regex string, one tagged statement that emits the code
    re = "(^|[^a-z0-9_])(accurate|cycle-accurate|...|boots|passes the cpu tests)([^a-z0-9_]|$)"
  addv README.md DOCS_CLAIM_WORD "$cw_n" "a withheld claim word outside the quoted policy line" # rule-claim-word
```
**Differs:** (a) `fixture-build` writes `NAME.vexp.tsv` from the validated `fixtures/src/NAME.vexp` beside `.observe` and `.classes` (F-38, research F-5); the file name stem check `*[!a-z0-9_]*` (line 182) and the hyphen-for-underscore NAME rule stay. (b) `conformance-run` F-43. (c) `docs-check`:
hyphenated compounds (`pixel-accurate`, `video-compatible`) are already caught because a hyphen is a boundary (research F-14); only `renders correctly`, `displays correctly`, `plays games` need alternation entries; keep ONE `# rule-claim-word` tagged statement (line 137), and add one negative
mini-root under `tests/docs/` modelled on `tests/docs/claim-word`.

### P24. Toys: ten new `tests/controls/<name>/{RULE,EXPECT}`

**Analog:** `tests/controls/trace-hook-mutates/` (a state-mutating hook), `timer-reload-off/` (a one-statement constant), `irq-priority-swap/` (a one-statement table).

```text
# tests/controls/timer-reload-off/RULE - keys: tag, file, anchor, replace (anchor occurs exactly once, written to the end of its line)
tag=rule-timer-reload-off
file=src/core/timer.c
anchor=uint64_t period = (uint64_t)inst->timer_reload + UINT64_C(1);
replace=uint64_t period = (uint64_t)inst->timer_reload + UINT64_C(2);
# tests/controls/timer-reload-off/EXPECT - case ids that must come out guest_failure on the toy
tim_timer
timer_pure_function
```
```text
# tests/controls/trace-hook-mutates/RULE - the template for vid-emit-mutates: the same statement plus a write through a cast away from const, present only in the generated toy
tag=rule-trace-hook-mutates
file=src/core/obs.c
anchor=r = chan_slot(&log->insn, (size_t)DS_INSN_RECORD_BYTES);
replace=r = chan_slot(&log->insn, (size_t)DS_INSN_RECORD_BYTES); ((struct ds_instance *)(void *)inst)->a = (uint8_t)(inst->a + 1u);
```
**Differs (the ten of D-173, anchors to be authored as ONE statement in the named file):**

| Toy | Plan | File to mutate | Statement to make unique | EXPECT shape |
|---|---|---|---|---|
| `vid-palette-mask` | 04-03 | `vce.c` | the palette store line (mask $1FF to $FF) | literal-role case of `vce_pal` |
| `vid-emit-mutates` | 04-03 | `vid.c` | the emit call (const cast write, as above) | observer video case |
| `fold-skipped-at-step-end` | 04-03 | `cpu.c` | the step-end fold call line | `chunk_equiv_video_*`, observer video |
| `vrr-no-readahead` | 04-04 | `vdc.c` | the read-buffer load | `vdc_reg` |
| `vid-rcr-base` | 04-06 | `beam.c` | the counter set to 64 | offset group red, ratio case `pass` (F-4) |
| `status-read-keeps-flags` | 04-06 | `vdc.c` | the status clear on read | `vdc_stat` |
| `vid-dot-div` | 04-07 | `vce.c` | the divider array | `vce_dot` |
| `vid-bit-order` | 04-09 | `vid.c` | the pixel bit extraction | asymmetric case red, solid case `pass` (F-4) |
| `vid-scroll-latch` | 04-10 | `beam.c` | the BXR latch statement | `bg_scroll` |
| `vid-spr-limit` | 04-13 | `vid.c` | the slot `min(16, ...)` | `spr_limit` |

Each EXPECT lists at least one `literal`-role case (`INV_TOY_NO_LITERAL`); an EXPECT line with a second column `pass` is the F-4 extension (F-43). The toy build already compiles every core and runner source into each `ds_toy_NAME` in one `cmake --build` (`scripts/conformance-run:502`); the shared-archive lever of D-176 is the cost control.

### P25. `ledger/entries/LED-*.toml` (17 new, 10 amended) and `ledger/REQUIRED` (04-01)

**Analog:** `ledger/entries/LED-VID-0001.toml` (an `assumed` entry with `sources`, `disagreement`, `assumption`, `revisit_phase`, `withheld`, `origin`), `LED-IRQ-0001.toml`, `LED-CPU-0016.toml` (a `supported` entry, F-47); `ledger/REQUIRED` lines `LED-VID-0001 SUMMARY C2` and `LED-IRQ-0008 03-CONTEXT D-112`.

```toml
id = "LED-VID-0001"
status = "assumed"
sources = ["REPORT|pcetech.txt|263 lines fixed (SUMMARY C2)", "UNKNOWN|manual pages read|state no line count (SUMMARY C2)"]
pinning_test = "PENDING:4"
assumption = "..."
revisit_phase = 4
withheld = ["any frame length claim", "any frame rate claim", "any samples-per-frame claim"]
origin = "SUMMARY C2"
```
**Differs:** new ids `LED-VID-0004..0018` and `LED-IRQ-0009`, `LED-IRQ-0010`, all `assumed` (0015 `deferred`), `pinning_test = "PENDING:4"`, `revisit_phase = 4`, a statement that STARTS with its level tag (VL0/VL1/R1/R2), and a REQUIRED line `ID 04-CONTEXT D-nnn` whose text equals `origin` exactly.
At most eight E1 candidates, promoted only under ER-2 in 04-11 or 04-14 (`SUPPORTED_PENDING_TEST` rejects `PENDING:` on a supported entry; pins are mutual, `INV_PIN_BACKLINK`). Lines under 400 characters, ASCII, at most 16,384 bytes per file.

## Shared Patterns

### SPDX and rights
**Source:** `scripts/licence-check:12-15, 32-33`. Code, scripts, tests and `.def` files: `SPDX-License-Identifier: MIT OR Apache-2.0` plus `SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors`, C files as `/* ... */` on lines 1-2. Everything under `fixtures/` and the data records
(ledger, cases, goldens, controls, `.vexp`): `0BSD`. No manual page text or image, no ROM, no capture, no GPL material is committed (ER-9). A line over 400 characters fails the TOML-subset checkers; keep every committed line under it.

### Stamps, folds and purity (the Phase 3 timer rule)
**Source:** `src/core/timer.c:10-11` header: state changes only inside the handlers that a guest access calls with its own stamp; everything else is a pure query. For Phase 4: only `ds_vdc_advance` and the register write handlers mutate beam, flag and DMA state; the render function is `const`; `ds_irq_pending_query` is "valid for a folded instance".
```c
/* timer.c:10 - the sentence to reuse in the headers of vdc.c and beam.c */
 * The mutation rule of this file: the state changes ONLY inside ds_timer_write and ds_timer_settle, which a guest access calls with its own stamp.
```

### Unsigned arithmetic and casts
**Source:** `src/core/obs.c:14-30`, `serial.c:40-69`, `poweron.c`. Every 8-bit and 16-bit value is masked and cast explicitly, shifts take widened unsigned operands, `UINT64_C(n)` for 64-bit literals; the sanitize preset has no `-fwrapv`, so a signed or implicit wrap is a defect. `snprintf` into a
fixed buffer bounds its inputs with a precision (`%.Ns`); no POSIX calls (`strtok_r`, `strdup`, `getline`, `strcasecmp`); no `0b` literals, no statement expressions.

### Validate then commit
**Source:** `src/core/snapshot.c:559-603` (`walk`: pass 1 judges and writes nothing, pass 2 commits and cannot fail) and `irq.c:51-80`. A refusal leaves the instance byte-identical (assert it with a digest and a raw `memcmp`, as `nonmutation_digest` does). Hostile arithmetic: offsets in u32 with
checked bounds, loops bounded by architectural constants (DMA <= 65,536, SATB scan <= 64, dots <= 682, lines by time).

### Refusal and error style in the runner
**Source:** `runner.h:99-100` and every kind: messages come from `check(c, cond, "what failed")` and `harness_error(c, why)`; output goes through `emit_out`/`emit_err`; stdout carries only `RESULT` and `RUNNER_DONE`. A kind sets `got` for the evidence file, calls `release_boot`, and never
aggregates. Sinks are heap blocks of the harness; a lost record in an evidence run is `harness_failure`.

### X-macro `.def` style
**Source:** `src/core/model_params.def:4-19`, `constants.def:4-27`. SPDX block, a header comment defining each column and the contract ("order is behaviour", "numbers never reused"), one row per line, no continuation lines; consumers use `#define DS_X(...) ... / #include "x.def" / #undef DS_X`; sh
and awk readers take names from the def (`scripts/inventory-check:25, 149` reads `constants.def` for ledger ids).

### Checker contract (every new gate script)
**Source:** `scripts/input-check:1-134`, `scripts/check:32-40`. Two awk stages with sentinels, `--root DIR`, `--expect CODE[,CODE]` (empty is exit 2), sorted violations `FILE<TAB>CODE<TAB>LINE<TAB>DETAIL`, `EXPECT_OK` on a met expectation, a `STATUS NAME ok n=N` marker on a clean run (a silent exit
0 is `CHECKER_DEAD`), exit 0 clean, 1 violation, 2 harness error. POSIX sh and awk only; no pipeline whose left side is awk (write a file, test the status, then sort).

### Evidence wording
**Source:** CONTEXT ER-4, ER-10..ER-13, D-181. Checks are classed MAN, GUEST or ASSUMED; the word `pass` is never a support claim; a video hash is E0 and valid only beside a literal case that the fault sweep and the toys show live; withheld words (`accurate`, `cycle-accurate`, `pixel-accurate`, `compatible`, `renders correctly`, ...) never appear without a class.

### Per-plan gate
**Source:** CONTEXT D-177. Every plan ends with `sh scripts/check` green, goldens regenerated and `LINUX_REPRO_PARTS=gate-gcc sh scripts/linux-repro` for a C plan (gcc 13, strict C11, `-Werror`, `-Wshadow`, `-Wconversion`); wave end `gate-gcc gate-clang core-boundary-gcc`. Audit shape rule: no
tracked blob of 4,096 bytes or more whose size is a multiple of 8,192 or 8,192 plus 512 (`scripts/audit-history:194-195`, `ROM-SIZE-8K` and `ROM-SIZE-HDR`, text included): check `wc -c` on every new or grown file (the 8,704-byte `docs-check` already has an allowance line in
`scripts/audit-history.allow`).

## No Analog Found

Sub-features with no close match in the repository. The planner should use CONTEXT and RESEARCH decisions and the nearest convention named.

| Sub-feature | Role | Data flow | Reason and nearest convention |
|---|---|---|---|
| Spin-counter guest loop (24-bit `clc`/`adc`, no data-dependent branch, handler that samples before reading status) | guest ROM | event-driven | no fixture measures interrupt timing; closest are the `DELAY`/`WAIT_SET` macros of `irq_ctl.s:80-97` and the `PEN_DIFF` loops of `spd_speed.s`; D-174 fixes the cost rule |
| Video judge ROWS, REL, STRUCT with the fault sweep F0..F9 on captured bytes | runner kind | batch | no judge compares structured output; `sweepkinds.c` (flip a literal in the image) and `digkinds.c` (perturb a captured stream) are the nearest shapes, not the same job |
| Variable-length, byte-capacity sink with a sticky full flag | core channel | streaming | `obs.c` channels are fixed-size records with a record-count capacity; the header, overlap and drop-newest logic carries over, the size arithmetic does not |
| Multi-event time ladder (EV_LINE, EV_VD, EV_RCR, DS, DV) | core scheduler | event-driven | the timer is one closed form with one event kind; `vdc_advance_pure` and `vdc_fold` take the place of a unit analog |
| `.vexp` literal rows | data format | CRUD | no repo file holds expected pixel rows; `tests/conformance/inputs/*.toml` with `input-rules.awk` is the nearest strict-subset format |
| Per-line background and sprite rendering | core | transform | no pixel code exists; the manual-derived decisions D-155..D-160 and the research map are the only sources |

## Metadata

**Analog search scope:** `src/core`, `src/text`, `tests/conformance` (runner, kinds, cases, goldens, REQUIRED), `tests/controls`, `tests/gate`, `tests/docs`, `fixtures/src`, `fixtures/lib`, `fixtures/manifest`, `scripts`, `scripts/lib`, `ledger`, `CMakeLists.txt`, `CMakePresets.json`.
**Files read:** about 70 (large files by non-overlapping ranges: `obskinds.c`, `guestkinds.c`, `snapkinds.c`, `scripts/check`, `scripts/conformance-run`).
**Tracked-source gate:** every cited path was verified with `git ls-files`; no mirror path appears.
**Pattern extraction date:** 2026-11-11
