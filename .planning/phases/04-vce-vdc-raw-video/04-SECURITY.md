---
phase: "04"
slug: "vce-vdc-raw-video"
status: SECURED
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_total: 47
threats_closed: 47
threats_open: 0
asvs_level: 1
block_on: high
created: "2026-11-12"
audited: "2026-11-12"
---

# Phase 04 - Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

Scope: the threat registers of plans 04-01 to 04-13, 04-17, 04-18 and 04-19 (T-04-01 to T-04-46 and T-04-56, 47 threats). T-04-47 to T-04-55 belonged to the sprite plans 04-14 to 04-16, were renumbered T-04.1-NN and are audited with Phase 4.1, not here.

Evidence basis (HEAD 7b4a81f, working tree clean except the untracked state.json):
- The code, case files, gate scripts, toy EXPECT files, ledger and README were read directly. Line numbers refer to the working tree at audit time.
- The auditor ran the full gate: `sh scripts/check` gave CHECK RESULT PASS (every stage, among them sanitizers, video-check, docs-claims, named-controls).
- The auditor ran ten selftests, all PASS: mutation-video-rules (14 controls), mutation-video-lex (6), mutation-toy-filter, mutation-docs-claim-word (4), mutation-tiling-entry, mutation-restore-skip-field, mutation-supported-rule, mutation-fixture-lint-ocr, mutation-policy-diff, mutation-sweep-vacuous.
- Scratch experiment 1 (outside the repo): a restore fuzzer built from src/core with clang -fsanitize=address,undefined. It booted vdc-dma, bg-scroll and vdc-irq under seeded-prng-v3, took a snapshot at several stop times and mutated 1 to 6 bytes of TIME, VCE0, PALT, VDC0, BEAM and VDMA per input.
- Result of experiment 1: 13,900 inputs, about 3,300 accepted and 10,600 refused. Every refusal left the target byte-identical, every accepted input re-serialised to the same bytes and then ran 400,000 clocks with a vid sink (some sinks only 3,000 bytes). No sanitizer report, no slow run.
- Scratch experiment 2: a copy of the mini-root tests/video/ok-video with rows that hold `$(touch ...)`, backquotes and `;rm -rf` fed to scripts/video-check: VEXP_FORMAT, exit 1, no file created.
- Toy evidence after the gate: under vid-palette-mask the video cases print `video_group border GUEST fail` and `solidval GUEST fail`; under vid-emit-mutates `border GUEST fail`.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| HuCard bytes to the VDC and VCE windows | Guest code writes any AR, register value, MAWR, MARR, DMA source, length, DVSSR, CTA, CR and palette value | Guest-chosen addresses, lengths, geometry |
| Restore stream to the instance | ds_restore reads a snapshot buffer of the caller (seven new regions: VCE0, PALT, VDC0, BEAM, VDMA, VRAM, SATB) | Untrusted lengths, counters, times, divisors |
| Core to the caller vid sink | The fold writes whole line records into caller memory | Record bytes; capacity from the request only |
| Run request to ds_run | struct_size and the appended vid member are caller-controlled (ABI skew) | Struct size, pointers, capacity |
| Power-on seed to machine state | Caller seed drives seeded-prng-v3 | VRAM, SATB, palette bytes only |
| Runner to the file system | --video-out and --trace-out write host files under build/gate | Case id, fixture key, stream bytes |
| Hand-edited .vexp text to the gate and runner | Parsed by awk (video-check, fixture-build) and by the runner TSV reader | Untrusted rows, keys, literals |
| Ledger, cites and README to readers | Claim words, tiers and evidence classes | Support claims |
| Manual PDFs to the planning record | Downloaded copies read as images in scratch, never committed | Page labels and short facts only |

---

## Threat Register

All 47 threats carry disposition mitigate. Status is closed for every one. Residuals that the reviews left are in the Accepted Risks Log.

| Threat ID | Category | Component | Severity | Disposition | Mitigation (evidence) | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-04-01 | Information disclosure | manual PDFs and page renders | high | mitigate | No PDF, image or render tracked (`git ls-files`); cites.txt header states ER-9 (labels and short facts only); TOOLS.md records URL, size, sha256; stages rights-tree and licences pass | closed |
| T-04-02 | Tampering | cites.txt rows (claim inflation) | high | mitigate | All 28 H7 and H6 rows carry img with verifier and date (cites.txt); `FXL_OCR_BACKS_SUPPORTED` (fixture-lint-rules.awk:24); selftest mutation-fixture-lint-ocr | closed |
| T-04-03 | Repudiation | ledger statements and README | medium | mitigate | `DOCS_CLAIM_WORD` (docs-check:137) with mini-root tests/docs/claim-word-video; selftest mutation-docs-claim-word (4 controls); ledger-check E1 rules | closed |
| T-04-04 | Tampering | vid sink overlap | high | mitigate | Table-driven overlap and alignment over three channels before any step (obs.c:128-191, DS_VID_ALIGN 8); 15 vid variants in run_refuse_memory_invalid (stepkinds.c:810-870) | closed |
| T-04-05 | Denial of service | vid sink overflow and capacity | high | mitigate | Capacity above 64 MiB is sink_unsupported (obs.c:164); room checked before every copy, sticky full (obs.c:347); case run_refuse_sink_unsupported | closed |
| T-04-06 | Tampering | old-size request struct | medium | mitigate | `struct_size` equality (run.c:89); old-size variant in run_refuse_struct_size (stepkinds.c:580) | closed |
| T-04-07 | Information disclosure | poisoned sink header | low | mitigate | Header written at begin and end, never read (obs.c:203-212); vid_chan_append and the 64 MiB boundary variant check the header | closed |
| T-04-08 | Tampering | restore decoders of VCE0, PALT, VDC0, BEAM, VDMA | high | mitigate | Pass 1 judges every field before pass 2 commits (snapshot.c:882-886, 594, 607, 639-646, 693-735, 760-773); cases snapshot_refusal_vdc, _vdma, _beam, _timeline, _window, _reach compare the whole instance (snapkinds.c:2106) | closed |
| T-04-09 | Denial of service | line_idx in a hostile snapshot | high | mitigate | `line_idx > time / 1365 + 2` refused before any product (snapshot.c:693); overflow guard in ds_vdc_pending_time (beam.c:45); rows line-far, line-2-63, line-max | closed |
| T-04-10 | Elevation of privilege | drawn power-on state raising IRQ1 | medium | mitigate | apply_v3 draws only VRAM, SATB, palette (poweron.c:159-172,187-189); registers zero by boot; poweron_v3_smoke `vid=constants` | closed |
| T-04-11 | Repudiation | regenerated digest literals | medium | mitigate | Version 8 method sentence (old literal checked under shasum first) in digest_stream_* comments; gate review I-04 recomputed both literals independently | closed |
| T-04-12 | Tampering | padding or untracked members | medium | mitigate | Tiling list summed against sizeof (snapkinds.c:94-113); selftests mutation-tiling-entry and mutation-restore-skip-field PASS | closed |
| T-04-13 | Tampering | palette and CTA arithmetic | high | mitigate | CTA masked 0x1FF (vce.c:34,39,74); 9-bit store (vce.c:81); restore odd byte at most 1 (snapshot.c:607); case vce_pal in sanitizers; toy vid-palette-mask | closed |
| T-04-14 | Denial of service | fold loop | high | mitigate | Bound 3*(span/1365)+6 (beam.c:249); restore window [time, time+2730) judged by comparison first (snapshot.c:729); case snapshot_refusal_window | closed |
| T-04-15 | Tampering | record writer into the sink | high | mitigate | Whole records only through ds_obs_vid (obs.c:328-358); header offsets asserted (vid.c:56-60); NULL or empty record counted lost, never copied | closed |
| T-04-16 | Repudiation | stream digest as evidence | medium | mitigate | Runner parses records with its own offsets (videokinds.c:31-42); digest printed beside literal groups only; toy vid-palette-mask turns literal groups red (toy evidence above) | closed |
| T-04-17 | Information disclosure | --video-out of a commercial image | high | mitigate | `vo_id_valid` [a-z0-9_] (videokinds.c:3587); fixture permit before any open (videokinds.c:3609-3631, obskinds.c:1824); case video_out_rights deny list; video_out_check (conformance-run:377-435) | closed |
| T-04-18 | Tampering | sink state leaking into machine state | high | mitigate | Render takes a const instance (vid.c:150); gate on `obs->vid.on` (beam.c:234); vid_noninterference over four sink shapes, three policies; toy vid-emit-mutates | closed |
| T-04-19 | Repudiation | digest recomputed only by the core | medium | mitigate | Host sha tool over the written .dsvr equals the evidence video_sha256, else VIDEO_OUT_DIGEST (conformance-run:429-433) | closed |
| T-04-20 | Tampering | VRAM accessor with guest addresses | high | mitigate | One accessor pair, words at or above 32768 read the constant and drop writes (vdc.c:54-75); case vdc_vram_sweep under sanitizers | closed |
| T-04-21 | Tampering | register stores and increments | medium | mitigate | One mask table applied after each byte (vdc.c:33-38,240); mod 65,536 steps (vdc.c:89,96); restore reads the same table (snapshot.c:643); case vdc_regmask | closed |
| T-04-22 | Elevation of privilege | Phase 3 fixtures and CR IE bits | medium | mitigate | Flags only through `ds_vdc_flag` with enable gate (vdc.c:263-295); registers zero under every policy; guest default policies zero plus two v3 seeds (guestkinds.c:867) | closed |
| T-04-23 | Repudiation | symmetric write and read bug | medium | mitigate | vdc_vram compares instance bytes, not port reads; toy vrr-no-readahead (EXPECT vdc_reg) | closed |
| T-04-24 | Tampering | .vexp parsing | medium | mitigate | Lexer limits 400 chars, ASCII, 512 rows, 16 KiB, SPDX (vexp-lex.awk:21-43), LC_ALL=C (video-check:18); runner re-checks and refuses unknown rows (videokinds.c:380-417); fixture-build exit 2; experiment 2 | closed |
| T-04-25 | Repudiation | golden digest used as a verdict | high | mitigate | `INV_VIDEO_HASH_ONLY` (vexp-rules.awk:424); runtime video-hash-only (videokinds.c:2454-2464); case video_hash_not_verdict | closed |
| T-04-26 | Repudiation | vacuous literal groups | high | mitigate | `VIDEO_GROUP_VACUOUS`, `INV_VIDEO_GROUP_UNKILLED`, `VIDEO_ROWS_VACUOUS` (vexp-rules.awk:290,302,378); video_fault_sweep cases; `VIDEO_SCENE_NO_SWEEP` (449); selftest mutation-video-rules | closed |
| T-04-27 | Tampering | class inflation in .vexp rows | medium | mitigate | `VEXP_CLASS` class lint and grid, frame, guest-bind rules (vexp-rules.awk:89-107,327-343); `VEXP_ASYM_CLASS` (287,395); mini-roots class-bad, grid-class, frame-class, guest-bind, rel-asym | closed |
| T-04-28 | Denial of service | event offsets from guest registers | medium | mitigate | u32 offsets at most 8192, RCR none at 1365 or more, VD capped 1364 (beam.c:175-187); v_togo at most 512 judged on restore (snapshot.c:696) | closed |
| T-04-29 | Elevation of privilege | IRQ1 level in Phase 3 fixtures | medium | mitigate | Enable gate at the event instant (vdc.c:263-295); irq_ctl, brk_irq_sample, cpu_blk_irq pass in the gate | closed |
| T-04-30 | Repudiation | interrupt-position claims | high | mitigate | LED-IRQ-0001, LED-IRQ-0010, LED-VID-0001 assumed; ratio rows GUEST, intervals ASSUMED (vdc_irq.checks); agree cases labelled E0/E2, hardware claim none; toy vid-rcr-base with vdc_irq_ratio pass | closed |
| T-04-31 | Tampering | payload length from guest geometry | high | mitigate | Window clipped to 1365 clocks and clamped to 682 dots (vid.c:129-148); DS_VID_MAX_REC_BYTES 2792 asserted (ds_obs.h:54); append refuses a record that does not fit | closed |
| T-04-32 | Tampering | stack record buffer | medium | mitigate | `rec[DS_VID_MAX_REC_BYTES]` (beam.c:235); render writes at most rec_bytes; vce_dot clip rows in sanitizers | closed |
| T-04-33 | Repudiation | width claims | medium | mitigate | LED-VID-0002 and LED-VID-0005 assumed; LED-VID-0017 scoped to contents 00h and 01h (README:28) | closed |
| T-04-34 | Tampering | DMA addresses and lengths | high | mitigate | Every word through the accessor, mod 65,536 steps (vdc.c:103-112); at most LENR+1 words per apply (vdc.c:134-138); vdc_dma, vdc_advance_pure, vdc_snapshot_mid_dma, vdc_dma_lenr_live in sanitizers | closed |
| T-04-35 | Denial of service | DMA progress loop | medium | mitigate | Closed form, re-based every apply, zero-divisor guard (vdc.c:118-150); restore needs dma_done 0, legal word time, dma_start at most time, SATB window (snapshot.c:760-773) | closed |
| T-04-36 | Tampering | SATB copy from hostile DVSSR | medium | mitigate | 256 fixed words via accessor with `(src + i) & 0xFFFF` (vdc.c:170-184); SATB written only there and by restore | closed |
| T-04-37 | Repudiation | DMA timing claims | medium | mitigate | vdc_dma row 24 ASSUMED:LED-VID-0014 (gate H-02 fix); LED-VID-0014 statement keeps trigger, length, DCR bits; no speed claim in README | closed |
| T-04-38 | Tampering | BAT and CG addressing | high | mitigate | Every word through the accessor, masks on code, bank, SCREEN, bg_y (vid.c:115-125,163-166); case bg_basic and video_bg_basic_* in sanitizers | closed |
| T-04-39 | Tampering | render writing machine state | medium | mitigate | const instance parameter (vid.c:150); pixel part only under the sink gate (beam.c:234); vid_noninterference, toy vid-emit-mutates | closed |
| T-04-40 | Repudiation | pixel-order claims | medium | mitigate | LED-VID-0010 assumed; `VEXP_ASYM_CLASS`; toy vid-bit-order turns asym red and keeps solid green (EXPECT pass column) | closed |
| T-04-41 | Tampering | scroll offsets and wrap | medium | mitigate | BXR 0x3FF, BYR 0x1FF masks, power-of-two wrap masks (vid.c:165-167, vdc.c:243, beam.c:167-169); bg_scroll wrap phase in sanitizers | closed |
| T-04-42 | Repudiation | latch-point claims | medium | mitigate | Next-line rows MAN H7-7, first line and wrap ASSUMED LED-VID-0009 (bg_scroll.vexp); toy vid-scroll-latch | closed |
| T-04-43 | Tampering | chunk or restore boundary at a write | medium | mitigate | chunk_equiv_video_bg_scroll chunks 7, 1364, 1366, 3072, one frame; snapshot_restore_video_bg; stream bytes compared | closed |
| T-04-44 | Denial of service | random register programs | medium | mitigate | vdc_random_regs, 32 programs, termination at until_mclk, lost 0, values below 512, in the sanitizers stage (28,320 assertions) | closed |
| T-04-45 | Elevation of privilege | power-on seeds in judged output | medium | mitigate | vdc_poweron_policy_diff over five fixtures plus control vce-pal-nopal that must differ; `VIDEO_FIXTURE_NO_POLICY` (vexp-rules.awk:459); selftest mutation-policy-diff | closed |
| T-04-46 | Repudiation | E1 promotion and README claims | high | mitigate | `DOCS_SUPPORTED_LIST` (docs-check:163): README list equals the 13 supported ids; LED-VID-0013 demoted to assumed (7b4a81f); `FXL_OCR_BACKS_SUPPORTED`, `INV_PIN_BACKLINK`; selftest mutation-supported-rule | closed |
| T-04-56 | Tampering | EXPECT pass column weakening a toy | medium | mitigate | toy_expect fails closed on a second column other than pass, a pass id not REQUIRED, no red id (conformance-run:501-535); selftest mutation-toy-filter | closed |

*Status: open, closed, or open below block_on threshold (non-blocking).*
*Severity: critical > high > medium > low. Only open threats at or above `block_on: high` count toward `threats_open`.*
*Disposition: mitigate (implementation required), accept (documented risk), transfer (third-party).*

---

## Review Findings Re-checked in Code

Every item marked fixed in 04-REVIEW-FIX.md was checked in the tree, not in the report.

| Finding | Check in code | Result |
|---------|---------------|--------|
| Core H-01 dma_done unbounded | Re-base at every apply (vdc.c:144-145); restore requires dma_done 0 (snapshot.c:760); case vdc_dma_lenr_live REQUIRED, 2406 assertions, sanitizers clean | closed |
| Core M-01 frame_lines vs profile | `ds_vdc_frame_lines` (beam.c:285-288) used by power-on and dec_beam, refusal profile (snapshot.c:706); row frame-263=profile | closed |
| Core M-02 no VBlank at exact fill | `vdw_done` on the wrap line (beam.c:117-122); case vdc_vblank_wrap REQUIRED | closed |
| Core L-01 unreachable beam states | VD order rule (snapshot.c:721); rows vd-pend-line-start, vd-pend-stage-rcr, vd-after-rcr, vd-rcr-equal refused | closed, residual accepted (AR-01) |
| Core L-02 restated constants | `ds_vdc_reg_mask` (vdc.c:40), `ds_vce_divider` (vce.c:26), `snap_divider_ok` and `snap_divider_max` (snapshot.c:557-581); second mask table gone | closed |
| Core I-04 stale plan refs | vid.c and vdc.c name plans 04.1-01 and 04.1-02 | closed |
| Gate H-01 toys red for the wrong reason | `vk_capture(..., past_guest_failure)` (videokinds.c:2311,2424); toy evidence shows literal groups fail | closed, residual accepted (AR-04) |
| Gate H-02 vdc_dma row 24 | Row 24 ASSUMED:LED-VID-0014 (vdc_dma.checks:32) | closed |
| Gate H-03 LED-VID-0013 | Status assumed (commit 7b4a81f); not in README supported list | closed |
| Gate H-04 rules without controls | One tag per code, mini-roots under tests/video, generated limit controls; selftests mutation-video-rules 14 PASS, mutation-video-lex 6 PASS | closed |
| Gate M-01 converse rows | vdc_stat rows 1, 3, 16 and vdc_dma row 11 ASSUMED:LED-VID-0008 | closed |
| Gate M-02 row-counter start | Groups solid7 and bb_on7 ASSUMED:LED-VID-0009 (bg_basic.vexp, bg_enab.vexp:41) | closed |
| Gate M-03 class lint binds labels | frame-class, grid-class, guest-bind, rel-asym rules and mini-roots | closed, limits accepted (AR-05) |
| Gate M-04 scene inventory links | `VIDEO_SCENE_NO_CASE`, `VIDEO_SCENE_NO_SWEEP`, `VIDEO_FIXTURE_NO_POLICY` (vexp-rules.awk:441,449,459) | closed |
| Gate M-05 policy_diff comment | Comment states frames are E0 calibration values | closed |
| Gate L-01 RATIO_TOL | `RATIO_TOL = 12` with the derivation (vdc_irq.s:44) | closed |
| Gate L-02 static grammar | Mini-roots rows-count-zero and scene-other-file | closed |
| Gate L-04 long lines | No line over 400 characters among lines changed since 925662a outside .planning (three older lines in run.c and snapkinds.c predate the phase) | closed, enforcement residual accepted (AR-07) |
| Gate L-05 gd[0] | Scene's own golden rows rewritten; video_hash_not_verdict passes | closed |
| Gate I-01, I-02 | spin.inc 9 cycles; "slot-level" and "is booted" wording | closed |

---

## Accepted Risks Log

No threat is accepted in place of its mitigation. These residuals were left by the review rulings (04-REVIEW-FIX.md) and are recorded here as accepted, each with its reason. None is at or above block_on high.

| Risk ID | Source | Severity | Residual | Rationale |
|---------|--------|----------|----------|-----------|
| AR-01 | Core L-01 | low | Restore does not recompute vd_off and rcr_off from HSR, HDR and lat_div, and does not require vd_off none when no VD is outstanding | A register write after EV_LINE legally changes the registers and not that line's offsets; power-on holds 0 there. The fold drains any such event within the window rule, so no hang or memory effect (fuzz experiment 1). |
| AR-02 | Core I-01, I-02 | info | Unreachable zero-divider fallback in dma_begin; line_sample_offset_mclk shifts EV_LINE but not EV_VD and EV_RCR | Dead branch only; the constant is 0 (constants.def:61) so the shift is latent. A future change of that row should add the offset to VD and RCR or a static assert. |
| AR-03 | Core I-03 | info | `lost` saturates at 2^32 while seq is u64; the record frame field is u32 | Needs more than 2^32 lost records or frames in one call (thousands of hours of emulated time); saturation never writes past the buffer. |
| AR-04 | Gate H-01 residual | low | vdc_irq_agree, _263 and _hdw still stop at the failed guest under the toy vid-rcr-base | The toy's direct kill is the guest case vdc_irq in the same EXPECT; the literal comparisons run on the real core in the gate. |
| AR-05 | Gate M-03 limits | low | `sym=` and `held=` are self-declared; a wrap selector is not recognised statically | Claim-class residual guarded by review and the ER-12 rules; no supported entry rests on such a row today. |
| AR-06 | Gate M-06 | low | Flag groups of video_bg_enab and video_unmodelled_mode declare only F0 (out-of-domain literal) | Recorded limit in 04-VALIDATION.md and VIDEO_TOY_EXEMPT; F0 proves the comparison live and nothing more; no claim rests on it. |
| AR-07 | Gate L-04, L-06 | info | The 400-character rule is enforced for .vexp only; record-lint controls stay at format 1 and epoch 1 | Style and test-shape items, not content boundaries; record-lint checks key presence only, so its controls still prove what they test. |
| AR-08 | Gate L-03 | low | README says Phase 4 is complete locally while 04-VALIDATION.md is status draft | Left to the orchestrator's close-out step by ruling; a phase status sentence, not a support claim (DOCS_SUPPORTED_LIST and DOCS_CLAIM_WORD pass). Track until the validation flip. |

---

## Unregistered Flags

SUMMARY Threat Flags sections state "None" beyond each plan's register. The audit found these items, none blocking.

| Item | Severity | Note |
|------|----------|------|
| --video-out rights rule reads the case fixture key; the booted image is the expect `rom=` name | low | Every committed video case has fixture `authored:<rom>` (all video_*.toml checked). The image name is `[a-z0-9-]` only and read from build/gate/fixtures, which fixture-build wipes and refills (fixture-build:104). An inventory rule tying the two would close it. |
| Toy controls linked from shared object libraries (CMakeLists.txt:77-121, commit d831680) | info | Object libraries and toys are EXCLUDE_FROM_ALL, no install rule exists, ds_core is built from its own sources (CMakeLists.txt:38-39); named-controls stage shows each toy red with the expected groups. No register entry, no new attack surface. |
| Phase 3 IN-03 (ds_run request or result overlapping the instance) still unrefused | low | Carried from 03-SECURITY.md; unchanged by this phase. |

No obvious missed high-severity item was found.

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-11-12 | 47 | 47 | 0 | gsd-security-auditor (ASVS L1, block_on high; full gate, ten selftests and two scratch experiments run by the auditor) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log (review residuals AR-01 to AR-08)
- [x] `threats_open: 0` confirmed
- [x] `status: SECURED` set in frontmatter

**Approval:** secured 2026-11-12
