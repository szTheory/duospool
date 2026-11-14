---
phase: "05"
slug: "psg-raw-audio"
status: SECURED
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_total: 38
threats_closed: 38
threats_open: 0
asvs_level: 1
block_on: high
created: "2026-11-14"
audited: "2026-11-14"
---

# Phase 05 - Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

Scope: the threat registers of plans 05-01 to 05-08, 05-10 and 05-12 (T-05-01 to T-05-30, T-05-34 to T-05-37, T-05-12a, T-05-12b, T-05-12c and the shared T-05-SC; T-05-24 is in both 05-06 and 05-12 and is counted once). That makes 38 threats. T-05-31 to T-05-33 belong to plans 05-09 and 05-11, which moved to Phase 5.1 at the D-217 cut, so this audit does not cover them. It also re-checks the earlier residuals that this phase touches (Phase 3 IN-03, AR-03, AR-07, U-01).

Evidence basis (HEAD ffe30bd, working tree clean except the untracked state.json):
- The code was read directly: src/core (psg.c, obs.c, run.c, bus.c, cpu.c, snapshot.c, serial.c, ds_core.h, ds_obs.h), the runner (tests/conformance/audiokinds.c, stepkinds.c, obskinds.c, runner.c), scripts/audio-check, scripts/lib/aexp-lex.awk and aexp-rules.awk, scripts/fixture-build (write_aexp), scripts/conformance-run (audio_out_check) and scripts/check (the audio-check stage). The ledger, README, tests/audio data, both .aexp files and the case files were read too. Line numbers refer to the working tree at audit time.
- The auditor did not run the full gate, because the machine was loaded. The auditor ran:
  - `sh scripts/check --stage audio-check`: CHECK audio-check PASS. This covers the repository, the tests/audio mini-roots, the generated lex controls and the 49,152 / 49,153-byte cap controls.
  - These checkers directly, each with STATUS ok and exit 0: core-boundary-check, docs-check, fixture-lint, inventory-check (388 required, 388 cases, 72 pins), ledger-check (supported=15), workflow-check, rights-check.
  - `sh scripts/audit-history --range 913dd4b..HEAD`, covering the whole phase: clean (67 commits, 496 blobs, 2,532 blob-path pairs).
- Gate artefacts from the last full gate: build/gate/results.txt at 2026-11-13 23:54, which is after 632cd58, the last commit that touches src, tests, scripts or fixtures (later commits change only .planning). It holds 388 RESULT lines, all pass. build/sanitize/results.txt holds 386 RESULT lines, all pass; the 2 missing ids are the EXCLUDED_IN_MODE alloc-shim cases. build/gate/controls.txt shows the six PSG toys as RED_OK: psg-atten-max, psg-chsel-mask, psg-dda-no-reset, psg-emit-mutates, psg-fold-skipped-at-step-end, psg-noise-taps.
- No PDF, image, ROM or disc file is tracked (`git ls-files` filter empty). No dependency was added in the phase: the only CMakeLists.txt change adds psgkinds.c and audiokinds.c to the runner sources.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Caller memory to the core | The request, the result and the five sink buffers (insn, dev, vid, psg, pcm) of ds_run | Untrusted size, alignment and overlap |
| Guest bytes to the PSG window | Any value at any of the 1,024 window addresses, under any select | Register writes, timer-paced DDA writes, block floods |
| Core records to caller sinks | DSPE records and DSPA frames written into caller memory | Bytes the core writes |
| Snapshot bytes to the instance | The PSG0, PSGW and PSGT regions of a caller snapshot | Untrusted bytes |
| .aexp text to awk and the runner | Hand-edited literal rows (fixtures/src/*.aexp, then build/gate/fixtures/*.aexp.tsv) | Untrusted text |
| Runner artefacts to disk | `--audio-out` writes build/gate/audio/ID.dspe, .dspa and .tsv | File paths, stream bytes |
| Project claims to readers | Ledger tiers, the README audio paragraph, the validation file | Support claims |
| Workflow file to CI runners | Lever (a) prints nproc and the CPU model | Run text |

---

## Threat Register

38 threats: 37 with disposition mitigate and 1 with disposition accept (T-05-SC). All 38 are closed. Residuals that the audit found are listed under Unregistered Flags, and none of them blocks.

| Threat ID | Category | Component | Severity | Disposition | Mitigation (evidence) | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-05-01 | Information disclosure | cites.txt, TOOLS.md | medium | mitigate | P8 rows of fixtures/lib/cites.txt hold labels and short numeric facts (lines 164-171, at most 364 characters). No PDF or image is tracked. audit-history over 913dd4b..HEAD is clean, and rights-check exits 0 | closed |
| T-05-02 | Repudiation | ledger statements and README | high | mitigate | LED-AUD-0001..0014 are assumed except 0005 and 0008, which are supported with ER-2/ER-22 records. LED-AUD-0006 and 0011 (the ER-14 holds) stay assumed. A MAN class needs an img cite (aexp-rules.awk:239), and FXL_OCR_BACKS_SUPPORTED is enforced (fixture-lint-rules.awk:153). docs-check DOCS_CLAIM_WORD (docs-check:138) and DOCS_SUPPORTED_LIST (:164) pass | closed |
| T-05-03 | Tampering | gate.yml lever (a) | low | mitigate | Commit 9619efe adds only `echo "nproc: ..."` and `sed -n '/^model name/...' /proc/cpuinfo`. No `\|\| true`, and no `${{ }}` in any run text (the 5 expressions are in concurrency and env). The ok-gate copy is byte-identical (cmp), and workflow-check passes | closed |
| T-05-04 | Tampering | psg and pcm sinks overlapping the instance, the request, the result or each other | high | mitigate | Five-channel table (obs.c:158, 175-179). Struct alignment and touches_call (:181-185), struct-to-struct overlap (:186-192), record range alignment and wrap (:145-156, 220-224), records against call memory, every struct and every other record (:225-240). memory_invalid is returned before ds_obs_begin writes anything (run.c:96-110). Audio overlap rows in stepkinds.c:1092ff; run_refuse_memory_invalid passes in gate and sanitize | closed |
| T-05-05 | Tampering | request or result overlapping the instance (IN-03) | high | mitigate | Step 0 of ds_obs_sinks_check (obs.c:171-174). finish() never writes a result that lies inside the instance (run.c:61). rr_call_overlap with a malloc'd instance block (stepkinds.c:1200, 1286-1314). run_refuse_call_overlap passes in gate and sanitize | closed |
| T-05-06 | Denial of service | oversized pcm capacity | medium | mitigate | DS_PCM_SINK_MAX_FRAMES 8,388,608 (ds_obs.h:74, static assert :77), limit of the pcm channel (obs.c:121), refused as sink_unsupported (:214-218). Drop-newest with sticky full by construction, because frames have one size (chan_slot obs.c:296-313; ds_obs_pcm :462-475). Rows at limit+1, UINT32_MAX and the exact 64 MiB limit (stepkinds.c:834-902) | closed |
| T-05-07 | Tampering | ABI skew of struct ds_run_request | medium | mitigate | psg and pcm are the last two members (ds_core.h:420-421). Exact struct_size check (run.c:90). Old sizes minus one and minus two pointers are refused (stepkinds.c:703-713). run_refuse_struct_size passes | closed |
| T-05-08 | Tampering | PSG restore decoders | high | mitigate | Validate then commit: pass 1 writes nothing (snapshot.c:1046), and pass 2 stores the same bytes (:1050). Each decoder stores only under commit (:863-877, 904-905, 935-943). The PSG0 masks use the single ds_psg_reg_mask table (:879-890). Wave bytes up to 31 (:907). snapshot_refusal_psg and snapshot_refusal_psgt pass in gate and sanitize | closed |
| T-05-09 | Denial of service | hostile distances or LFSR 0 in a snapshot | high | mitigate | A frozen counter must have distance 0 (snapshot.c:947-948). Running distances are 1..24,576, or 1..6,291,456 for select 1 (:949-950). Noise distances are 1..11,904 (:956). LFSR is 1..$3FFFF (:961). The grid test runs only in the else branch after r >= 1 (:951-952, 958-959). TIME is below 2^63 (:163, 286). Fold: the noise loop advances by a period of at least 3 x 64 or 3 x nf0 (psg.c:76-81, 109-117), and channel steps are closed form (:120-130) | closed |
| T-05-10 | Tampering | guest writes to unmapped slots or select 6/7 | medium | mitigate | One mask table (psg.c:30-35) shared with the restore decoder. NO_TARGET stores nothing for reg >= 10, R2..R7 at select 6/7, or R7 at select 0..3 (:251-257). Explicit masks: select & 7 (:263), F & $FFF (:86), NFRQ & $1F (:78), level & $1F (:204, 208), widx & 31 (:127, 303) | closed |
| T-05-11 | Repudiation | regenerated digest literals | medium | mitigate | 05-03-SUMMARY records the predicted and observed red lists (section at line 81) and the frames hashed by a script outside the repository (:99). Snapshot digests are recomputed by the host tool in the gate. Process evidence, accepted at ASVS L1 | closed |
| T-05-12 | Tampering | cross-binding restore (d = 3 into d = 6) | low | mitigate | dec_prof compares the profile hash (snapshot.c:206-223). d is read only from the profile (psg.c:24-27). psg_restore_divisor passes (refused as snapshot.profile by d = 6, accepted by d = 3) | closed |
| T-05-13 | Tampering | DSPE and DSPA writers | high | mitigate | DSPE is built byte by byte (bus.c:165-188) and copied whole through chan_slot (obs.c:444-459). Frames use put_u32 (obs.c:462-475). VOL has exactly psg_silence_att rows (psg.c:97) and is indexed only when att < 30 (:211-216). The mix bound 6 x 31 x 32768 < 2^31 is static-asserted (:98) | closed |
| T-05-14 | Denial of service | sample loop and fold under F = 1, d = 3 | medium | mitigate | The sample loop runs at most (stamp - B) / 192 + 1 times (psg.c:152-155). The fold is closed form per channel, and noise is bounded by span / (3 x 64) (:102-131). psg_fold passes in sanitize. See U-05-01 | closed |
| T-05-15 | Repudiation | audio stream digests as evidence | medium | mitigate | Digests are printed labelled `E0 repeatability ... never a verdict` (audiokinds.c:2243, 2433). audio_tone_sel0_d3 uses expect_red=period_L,period_R, and the exact red set is required (:2404) | closed |
| T-05-16 | Information disclosure | `--audio-out` of a commercial image | medium | mitigate | The id must match [a-z0-9_]{1,64} (audiokinds.c:4159-4164). Only an audio case whose fixture is authored and listed or synthetic is accepted (:4183-4208; obskinds.c:1960-1963). Paths are built only under build/gate/audio/ (:4124, 4167-4180), and every check runs before any file is opened (:4317-4324). audio_out_rights (:4389-4476) and conformance-run audio_out_check (:451-516: no unrequested dir, host SHA-256 equals the case evidence, 5 refusals exit 2 with no file) | closed |
| T-05-17 | Tampering | .aexp parsing in the runner | medium | mitigate | write_aexp checks field-count ranges, rejects empty fields and spaces, and treats an unknown kind as exit 2 (fixture-build:188-215). The runner allows 512 rows and 400 characters per line (audiokinds.c:263-264, 586-589) and at most 7 fields (:590-593). Unknown rows, keys, forms, kinds and roles are refused (:456, 485, 513, 517, 553). Every copy into a fixed buffer is preceded by a length check (au_name_ok :105-110, au_gid_ok :113-118, AX_TEXT :455, 534, ax_hex64 :429-432) | closed |
| T-05-18 | Tampering | .aexp parsing in awk | medium | mitigate | AEXP_MAX_BYTES 49152 is checked before parsing, exit 2 (audio-check:25, 199). Stage 1 allows 400 characters, ASCII only, 512 rows and needs SPDX (aexp-lex.awk:19-39). Sentinels END and RULES are required (audio-check:200-205, 216-222). Fields are never passed to the shell. Unknown keys are refused (aexp-rules.awk:149, 166, 187, 197, 310, 317). The audio-check stage passed when the auditor ran it | closed |
| T-05-19 | Repudiation | class inflation and converse rows | high | mitigate | KINDCLASS ceiling (aexp-rules.awk:256) and the img-cite label rule (:239). DISPUTED rules (:444, 447). S-based live and silent class rules (:422, 430). Arithmetic lint (:362, 369, 376, 380, 389, 399, 414, 417). Removal selftests exist (scripts/check:2894-2998). The selftests were not re-run by this audit | closed |
| T-05-20 | Repudiation | hash-only verdicts | medium | mitigate | INV_AUDIO_HASH_ONLY (aexp-rules.awk:507). Runtime audio-hash-only refusal (audiokinds.c:2156-2192). audio_hash_not_verdict passes (an AF7-faulted copy passes its own golden while the literal groups fail). The single golden (psg_tone.aexp:319-320) sits beside the literal scenes of the same ROM | closed |
| T-05-21 | Repudiation | vacuous groups | medium | mitigate | The sweep fails a declared kill that does not flip (audiokinds.c:2742-2743) and a group that no declared fault flips (:2744-2745). Null-synth controls (:2980-3040). AUDIO_GROUP_VACUOUS and AUDIO_SCENE_NO_SWEEP (aexp-rules.awk:282, 529) | closed |
| T-05-22 | Repudiation | attenuation rows | high | mitigate | A MAN live row needs S = 0 or 4 (aexp-rules.awk:422). A MAN silent row needs chON 0, S >= 16 or a $00 byte (:430). Sum 15 is disputed (:447; DISPUTED `sum=15`). LED-AUD-0008 withholds the sum-15 boundary | closed |
| T-05-23 | Repudiation | twin attribution | medium | mitigate | The failing set must equal expect_red exactly, and any other set is misprog-misattributed (audiokinds.c:2404). audio_tone_pan01_twin passes | closed |
| T-05-24 | Repudiation | golden digest drift | low | mitigate | Regenerated with the host tool, old and new recorded (05-06-SUMMARY:207, 216; 05-12-SUMMARY:179). The golden sits beside the literal scenes (INV_AUDIO_HASH_ONLY). audio_out_check recomputes the stream SHA-256 on the host (conformance-run:497-503) | closed |
| T-05-25 | Denial of service | fold under F = 1, NFc = 0, 65,536-byte TIN | medium | mitigate | Each TIN byte is its own step (run.c:7), so each fold is bounded by one step. psg_random_regs and psg_block_flood pass in sanitize. Drop-newest with lost counted (obs.c:303-309) | closed |
| T-05-26 | Repudiation | noise sequence claims | medium | mitigate | KINDCLASS sets noise_seq and run_min to ASSUMED. n4_seq and n4_min are ASSUMED:LED-AUD-0010 (psg_mod.aexp). MAN is used only for two_level, run_grid and noise_ratio (plus ev_decode). The toy psg-noise-taps is RED_OK | closed |
| T-05-27 | Repudiation | DDA evidence | low | mitigate | d_hold, d_ord and d_ram are MAN:P8-4+P8-5. d_hold1 (the latch with chON 0) is ASSUMED:LED-AUD-0004. d_lev is ASSUMED:LED-AUD-0009. d_ev is GUEST:program (psg_mod.aexp:70-74; KINDCLASS dda_level_exact ASSUMED) | closed |
| T-05-28 | Tampering | sink-dependent state | medium | mitigate | With a pcm sink the fold is split at sample instants, and the closed-form step count is additive, so the state is the same without a sink (psg.c:151-166). ds_psg_sample is const (:185). aud_noninterference, observer_psg_mod, observer_psg_tone and psg_block_flood pass. The toy psg-emit-mutates is RED_OK | closed |
| T-05-29 | Repudiation | E1 promotions | high | mitigate | Only LED-AUD-0005 and 0008 are supported, each with ER-2/ER-22 clause records in the entry. 0006 and 0011 are held. DOCS_SUPPORTED_LIST, FXL_OCR_BACKS_SUPPORTED and INV_PIN_BACKLINK (inventory-check:168) pass when run by the auditor | closed |
| T-05-30 | Tampering | weakening checks at the checkpoint | medium | mitigate | REQUIRED gains 54 ids and loses none over 913dd4b..HEAD. EXCLUDED_IN_MODE and VIDEO_TOY_EXEMPT are unchanged. The last full gate has 388/388 pass and sanitize 386/386 pass | closed |
| T-05-34 | Repudiation | README and ledger tiers at close | high | mitigate | README:33-44 names the LFO as Phase 5.1 and gives a withheld list. Claim words are quoted only in the policy line (README:65). docs-check passes, and so do DOCS_SUPPORTED_LIST, inventory and fixture-lint | closed |
| T-05-35 | Repudiation | checkbox honesty | medium | mitigate | AUD-01 stays unticked and Pending for Phase 5.1 (REQUIREMENTS.md:49, 175). AUD-02 and AUD-03 are ticked | closed |
| T-05-36 | Tampering | weakening checks to close the phase | medium | mitigate | tests/audio/RULES has no `later:` entry (the only match is the header comment, line 6). The checks are not weakened (see T-05-30). The six PSG toys are RED_OK | closed |
| T-05-37 | Tampering | evidence classes at close | medium | mitigate | a3a2238: the ev_decode_sel0 R2 row moves unchanged to ev_decode_sel0_pw (ASSUMED:LED-PROF-0003). d_hold is re-anchored and split, and its ordinal 1 moves to d_hold1 (ASSUMED, mark=latch_off). Two unused kinds were dropped from KINDCLASS. The LED-AUD-0005 clause (c) was narrowed to chON 1 and DDA 1 with a P8-4 cite. The commit message records the sweep at 34/34 and 42/42 | closed |
| T-05-12a | Denial of service | the raised .aexp cap | low | mitigate | One constant (audio-check:25). The 49,153-byte control expects exit 2 (scripts/check:1049-1061). An exactly 49,152-byte file passes (ok-size-cap, :1043-1044 region of ac_gen_limits). Selftest mutation-audio-file-too-large (:2983-2998). The C reader keeps 512 rows and 400 characters (audiokinds.c:263-264) | closed |
| T-05-12b | Repudiation | decode rows on disputed points | medium | mitigate | The edge group is ASSUMED:LED-AUD-0003+LED-AUD-0004 with rows marked reg=RA, sel=6 and sel=7 (psg_tone.aexp:278ff). r7_drop is ASSUMED (psg_mod.aexp:55). A MAN group with a disputed marker is refused (aexp-rules.awk:444) | closed |
| T-05-12c | Repudiation | R2 rows on the power-on F | low | mitigate | The only reg=R2 decode row that rests on the power-on F is ev_decode_sel0_pw, which is ASSUMED:LED-PROF-0003 (psg_tone.aexp:65-67). 05-12-SUMMARY:144-151 records the follow-an-R3 rule | closed |
| T-05-SC | Tampering | package installs | low | accept | No package, tool or dependency was added. The CMakeLists.txt diff only adds two runner sources. audit-history is clean. Logged as AR-10 | closed |

*Status: open, closed, or open below block_on threshold (non-blocking).*
*Severity: critical > high > medium > low. Only open threats at or above `block_on: high` count toward `threats_open`.*
*Disposition: mitigate (implementation required), accept (documented risk), transfer (third-party).*

---

## Audited Surfaces Re-checked in Code

| Surface | Check in code or run | Result |
|---------|----------------------|--------|
| Sink overlap and aliasing | Every pair over {instance, request, result, 5 structs, 5 record ranges} is tested before the first header write. Zero-capacity sinks use no pointer (obs.c:147-149). A stop callback that edits a sink mid-call has no effect, because ds_obs_begin copies the pointers and capacities (obs.c:329-345) | closed |
| PSG array indexing | psg_wave[sel x 32 + widx] is reached only with sel <= 5 (NO_TARGET, psg.c:251) and widx <= 31. Every write path keeps widx at most 31 (psg.c:127, 282, 303, 323), and restore refuses widx > 31 (snapshot.c:884). psg_noise[sel - 4] only with sel 4 or 5 (psg.c:251, 312) | closed |
| Fold arithmetic | Offsets from B are computed in u64. Stored distances are at most t + q, so far below 2^32. The step-end rebase subtracts t only from distances greater than t (psg.c:166-176). There is no division by zero, because q >= 3 and the noise period >= 3 x 64 | closed |
| Restore ordering | PROF, then TIME, then PSG0, then PSGT, in the fixed decoder order (snapshot.c:971-975). PSGT reads the PSG0 values of the same pass from vctx, and v->time is set by dec_time in pass 1 | closed |
| Core determinism | core-boundary-check passes: no float, no writable symbol, no allocator, no clock in src/core. psg.c and obs.c state "No allocation, no global, no clock" and the check enforces it | closed |
| Runner capture | Any lost record or frame, or a header count above capacity, is a harness error (audiokinds.c:755). The pcm header fields are checked before use | closed |
| `--audio-out` overwrite | fopen "wb" truncates only the three derived paths under build/gate/audio/ (audiokinds.c:4230, 4285). conformance-run removes and recreates that directory around the explicit request (:211, 455-459). See U-05-03 | closed |

---

## Earlier Residuals Re-checked

| Item | State at this audit | Note |
|------|---------------------|------|
| Phase 3 IN-03 (request or result overlapping the instance) | resolved | Closed by T-05-05 (obs.c:171-174, run.c:61, run_refuse_call_overlap) |
| U-01 of 04.1 (sprite_pend outside a display line) | resolved before this phase | snapshot.c:715-720 (commit e41f9e7) |
| AR-03 (`lost` saturates at 2^32) | unchanged in kind, extended to psg and pcm | obs.c:305. Saturation never writes past a buffer, and the runner treats any lost frame or record as a harness error (audiokinds.c:755) |
| AR-07 (400-character rule for .vexp only) | narrowed | The rule now also applies to .aexp (aexp-lex.awk:19-21; runner audiokinds.c:586) |
| AR-01, AR-02, AR-04, AR-05, AR-06, AR-09 | unchanged | Not touched by Phase 5 code |

---

## Accepted Risks Log

No mitigate threat is accepted in place of its mitigation. AR-01 to AR-07 and AR-09 carry forward as listed above.

| Risk ID | Source | Severity | Residual | Rationale |
|---------|--------|----------|----------|-----------|
| AR-10 | T-05-SC | low | Package installs | No package, tool or dependency was installed or added in Phase 5. The only build change adds two in-repo runner sources. audit-history over the phase range is clean |

---

## Unregistered Flags

Every SUMMARY Threat Flags section says "None" beyond its plan's register. The audit found the items below in source. None of them blocks.

| Item | Severity | Note |
|------|----------|------|
| U-05-01: the D-221 span of 600 clocks is a literal | info | The assert at psg.c:20 uses 600, and 05-CONTEXT D-221 says "ds_max_step_mclk(), 600 clocks today". No assert ties the literal to ds_max_step_mclk() (run.c:39-56), so a later opcode-table change that lengthens the longest step would not trip it. There is no effect today: the fold loops are bounded by the real step span, the arithmetic is u64, and stored distances stay far below 2^32. |
| U-05-02: the runner reads the built .aexp.tsv without a byte cap | info | read_file sizes the buffer from ftell (runner.c:706-723). ax_load then refuses at row 513 or at a line over 400 characters (audiokinds.c:586-589). The 49,152-byte cap lives only in audio-check (audio-check:25, 199), and write_aexp copies any size (fixture-build:188-215). In the gate, audio-check (scripts/check:3858) runs before fixture-build (:3870), so an oversize source fails first. The input is repository-authored, and this matches the T-05-12a statement ("the C reader keeps 512 rows and 400-character lines"). |
| U-05-03: `--audio-out` follows a pre-placed symlink | info | fopen "wb" (audiokinds.c:4230, 4285) follows a symlink placed at build/gate/audio/ID.dspe, .dspa or .tsv. Placing one needs local write access to build/, which is the same trust as the working tree. The gate path recreates the directory first (conformance-run:211, 455-459). This is the same shape as `--video-out` and `--trace-out`. |

No missed high-severity item was found.

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-11-14 | 38 | 38 | 0 | gsd-security-auditor (ASVS L1, block_on high). The auditor ran the audio-check stage, seven static checkers and audit-history. Gate and sanitize results come from the last full gate at 632cd58. The full gate and the selftests were not re-run |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log (AR-10; AR-01 to AR-07 and AR-09 carried)
- [x] `threats_open: 0` confirmed
- [x] `status: SECURED` set in frontmatter

**Approval:** secured 2026-11-14
