---
phase: "03"
slug: "huc6280-cpu-bus-timer-interrupts-and-2-button-pad"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-11-11"
---

# Phase 03 - Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

Evidence basis: the code, case files, gate scripts, workflow and ledger were read directly. The auditor did not run the gate; "the gate passes" is taken from the verifier's concurrent `sh scripts/check` result. Process-only items (tool installs, PDF fetches) rest on the recorded hashes in `fixtures/lib/TOOLS.md`, the SUMMARY records, and repository-side facts (nothing tracked, tools absent).

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Caller to core API | Host passes HuCard bytes, run request, result, sinks, model names, stimulus and input lists to `ds_boot`, `ds_run`, `ds_model_set`, `ds_irq_stim_set`, `ds_input_push` | Untrusted bytes, pointers, counts, 64-bit times |
| Snapshot stream to `ds_restore` | A saved-state buffer is untrusted input to the restore path | Tagged regions, lengths, field values |
| Host trace sinks | Caller-owned memory the core writes records into during a run | Output-only records; header must never steer writes |
| Input script file to harness | Authored TOML-subset script parsed by awk then flattened into events | Text, decimal times, port and button masks |
| Case files and runner | Conformance cases and the runner read repository files; trace-out writes files | Case ids, fixture names, paths |
| Fixture build chain | ca65 and ld65 are external tools; optional WLA-DX is local only; ROM images are built into ignored build/ | Authored sources, assembled images, hashes |
| Manual PDFs | Downloaded to a scratch directory, hash-checked, read as untrusted data, never committed | Page images; only page labels and numbers enter the repo |
| CI workflow | GitHub Actions job with read-only token, no secrets | Workflow text, apt package install |
| Public repository rights boundary | No ROMs, firmware, home paths, manual text or credentials may be tracked | Tracked files and history |

---

## Threat Register

All 106 threats carry disposition mitigate. Status is closed for every one. Line numbers refer to the working tree at audit time.

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-03-01 | Tampering | cpu.c bus translate | high | mitigate | Per-access translate in `src/core/bus.c`; `bank_class` 256 entries asserted (bus.c:41); ROM only via `ds_mapper_read` bounds (mapper.c:82); case mmu_mpr_* in sanitizers stage | closed |
| T-03-02 | Denial of service | ds_run loop | medium | mitigate | Absolute `until_mclk` loop and `time_wrap` refusal (run.c:100-104); undefined opcodes are 2-cycle steps (cpu.c MN_UNDEF) | closed |
| T-03-03 | Tampering | ds_run request | medium | mitigate | NULL, `struct_size`, `reserved` checks before any step (run.c:85-94); sinks validated by `ds_obs_sinks_check` (run.c:95) | closed |
| T-03-04 | Repudiation | DSV1 judge | medium | mitigate | Judge recomputes summary and checksum from raw RAM (guestkinds.c:159,167); pre-step block refused (guestkinds.c:1067-1069); cases ds_proto_badsum, ds_proto_t0_poisoned | closed |
| T-03-05 | Tampering | zero-step boundary | low | mitigate | Early return for `until <= machine_time` (run.c:109); case run_until_abs | closed |
| T-03-06 | Denial of service | spin and interleave kinds | low | mitigate | Absolute deadlines in all kinds; `spin_forever` only in the supervised cancel mini-root (stage cancel-controls) | closed |
| T-03-07 | Tampering | hidden shared state | medium | mitigate | Cases interleave_ab, interleave_aba_order, interleave_twins; toy control shared-state (tests/controls) | closed |
| T-03-08 | Tampering | serial.c region table | medium | mitigate | Byte-wise little-endian emit (serial.c:60); adjacency `_Static_assert` (serial.c:144-183); case field_flip; toy drop-region | closed |
| T-03-09 | Repudiation | regenerated goldens | medium | mitigate | Golden headers state host sha tool recompute (tests/conformance/golden/accept_256k_zero.record.toml header) | closed |
| T-03-10 | Tampering | v1 power-on freeze | low | mitigate | Case poweron_draw_order and poweron_zero present and REQUIRED | closed |
| T-03-11 | Tampering | integer case keys | medium | mitigate | `is_uint_text` 1-19 digits (runner.c:676); `INV_CASE_TYPE` rule (inventory-rules.awk:48) | closed |
| T-03-12 | Tampering | authored fixture name | medium | mitigate | `authored_name` allows [a-z0-9-] only (guestkinds.c:203-215); loader path built from NAME under build/gate/fixtures | closed |
| T-03-13 | Repudiation | audit-history ROM-SIZE rule | low | mitigate | ROM-SIZE-8K and -HDR checks live (audit-history:194-195); allowlist is path and CHECK scoped and NUL-free guarded (see Unregistered Flags) | closed |
| T-03-14 | Spoofing | DSV1 verdict | high | mitigate | Recompute, id and count required, pre-step block refused (guestkinds.c:115-167); five controls ds_proto_badsum, _nodone, _predone, _wrongid, _t0_poisoned | closed |
| T-03-15 | Information disclosure | cites.txt | medium | mitigate | Header states ER-9 (cites.txt:9); rows are labels, flags, dates and short numeric facts; no PDF or image tracked | closed |
| T-03-16 | Tampering | fixture includes | medium | mitigate | Sources include only startup.inc, verdict.inc, check.inc (0BSD, fixtures/lib); `FXL_INCLUDE_PATH` (fixture-lint-rules.awk:98) | closed |
| T-03-17 | Repudiation | expected-value classes | medium | mitigate | `FXL_BAD_CLASS` and `FXL_UNKNOWN_CITE` (fixture-lint-rules.awk:135,150-151) | closed |
| T-03-18 | Tampering | authored ROM bytes | high | mitigate | sha256 and size pins in fixtures/manifest/*.toml; `FIX_HASH_MISMATCH` and `FIX_SIZE_MISMATCH` (fixture-check:126-127); build wipes output first (fixture-build:98) | closed |
| T-03-19 | Information disclosure | ROM-like files committed | high | mitigate | .gitignore ignores *.pce, *.bin and build/; `git ls-files` shows no ROM-like or PDF file; rights-tree, rights-negatives and audit-synthetic stages in scripts/check | closed |
| T-03-20 | Spoofing | assembler identity | medium | mitigate | Identity by encoding matrix (scripts/xasm-check) and hash pins, not `--version` (TOOLS.md); preflight exits 2 without ca65 and ld65 (preflight:21-60) | closed |
| T-03-21 | Elevation of privilege | workflow run step | medium | mitigate | gate.yml: `permissions: contents: read`, no secrets, pinned checkout, `persist-credentials: false`; rules `WF_FORBIDDEN_KEY` and `WF_EXPR_IN_RUN` (workflow-rules.awk:65-73,269) | closed |
| T-03-22 | Repudiation | README status | medium | mitigate | README states assumed or documentary wording and withheld claim words (README:4,15-23); `DOCS_CLAIM_WORD` rule and red control (stage docs-claims-negatives) | closed |
| T-03-23 | Spoofing | PDFs | medium | mitigate | Three sha256 values recorded in TOOLS.md and checked equal per SUMMARY 03-08, 03-09, 03-32; no PDF tracked (process evidence plus repo absence) | closed |
| T-03-24 | Information disclosure | page text in the repo | medium | mitigate | `huc6280-opcodes.tsv` header and cites.txt header state numbers and labels only (ER-9); no page text or image tracked | closed |
| T-03-25 | Tampering | ca65 identity | medium | mitigate | scripts/xasm-check with lib/xasm-matrix.awk byte comparison; stage xasm-negatives (shim, changed-byte controls) | closed |
| T-03-26 | Tampering | opcode decode | high | mitigate | `DS_OP_ROWS == 256` asserted (cpu.c:77); main mnemonic switch has no default; stage opcode-oracle; case opcode_table_manual | closed |
| T-03-27 | Repudiation | table independence | medium | mitigate | Independence recorded in SUMMARY 03-09 and the TSV header; opcode-check compares def and TSV (stage opcode-oracle) | closed |
| T-03-28 | Denial of service | overshoot | low | mitigate | `ds_max_step_mclk` table scan (run.c:47-62) feeds the time_wrap guard; case run_overshoot_bound | closed |
| T-03-29 | Tampering | sink overlap | high | mitigate | `ds_obs_sinks_check` with uintptr_t `ranges_overlap`, alignment, wrap check (obs.c:33-165) returns `memory_invalid`; cases run_refuse_memory_invalid, insn_sink_shapes | closed |
| T-03-30 | Elevation of privilege | poisoned sink header | medium | mitigate | Header only written, never read (obs.c write_header, chan_open); capacity from the request (obs.c:136-140); poisoned rows in insn_sink_shapes | closed |
| T-03-31 | Repudiation | vacuous fixture checks | medium | mitigate | mutate_sweep cases for every authored fixture (tests/conformance/cases/mutate_sweep_*.toml; sweepkinds.c) | closed |
| T-03-32 | Tampering | indexed and indirect arithmetic | high | mitigate | `zp_addr` and indexed modes masked with 0xFF (cpu.c:232,253); 16-bit wraps; all accesses through bus; case cpu_ldst in sanitizers | closed |
| T-03-33 | Repudiation | power-on dependence | medium | mitigate | Three-policy comparison on guest cases (guestkinds.c policies logic); case guest_policies_guard, poweron_diff_seed_diff_digest | closed |
| T-03-34 | Tampering | 8-bit arithmetic and flags | high | mitigate | Explicit masks and casts (cpu.c:347,436-456); sanitize preset has no -fwrapv (CMakePresets.json:27-31); case cpu_alu | closed |
| T-03-35 | Repudiation | self-referential expected values | medium | mitigate | Checks classed MAN, GUEST or ASSUMED (`FXL_BAD_CLASS`); mutate_sweep_cpu_alu | closed |
| T-03-36 | Tampering | toy anchor drift | low | mitigate | control-mutate requires anchor exactly once (control-mutate:19); selftest mutation-control-anchor | closed |
| T-03-37 | Tampering | stack and PC arithmetic | high | mitigate | `push`/`pull` mask S and use 0x2100 base through the bus (cpu.c:200-209); PC masks (cpu.c:711,1013); case cpu_flow | closed |
| T-03-38 | Spoofing | stray vector | medium | mitigate | Default RAM vector sets error flag; judge treats nonzero error flags as guest_failure (guestkinds.c:163) | closed |
| T-03-39 | Tampering | ds_mpr_write | medium | mitigate | Single writer with index bound (bus.c:231-235); translation per access, no cache (bus.c:bank_of) | closed |
| T-03-40 | Tampering | ds_bus_write_phys | medium | mitigate | Address restricted to $1FE000, 2, 3 (bus.c:226); case st_bypass | closed |
| T-03-41 | Tampering | TSV reader in runner | low | mitigate | `ot_load` requires 256 rows, harness_error on any fault (cpukinds.c:133-165); path only from conformance-run | closed |
| T-03-42 | Repudiation | ledger overclaim | high | mitigate | `SUPPORTED_NO_QUALIFYING_EVIDENCE` and `E1_NEEDS_FACT_SOURCE` (ledger-rules.awk:89-91); seven supported entries each carry an E1 line | closed |
| T-03-43 | Repudiation | circular cycle evidence | medium | mitigate | Literals in case cycle_adders_manual from pages; insn-sink lost counter checked; control block-progress-lost | closed |
| T-03-44 | Tampering | RAM index | high | mitigate | `& (DS_RAM8K_BYTES - 1u)` with `_Static_assert` (bus.c:30,188,206) | closed |
| T-03-45 | Tampering | ROM index | high | mitigate | Only HuCard class reaches `ds_mapper_read` (bus.c:184); index bound check (mapper.c:82-84) | closed |
| T-03-46 | Information disclosure | uninitialised device state | low | mitigate | Stubs return named constants or `io_buffer` (bus.c hw_read); instance zeroed at boot | closed |
| T-03-47 | Tampering | ds_model_set | medium | mitigate | Exact name and value match, state check before any write (model.c:181-206); cases model_refuse_unknown_name, _value, _state | closed |
| T-03-48 | Repudiation | silent modelling choice | medium | mitigate | model_params.def rows carry ledger ids; record_model_bound case; digest_model_* cases | closed |
| T-03-49 | Information disclosure | record values | low | mitigate | `put_quoted` refuses quote, backslash, non-ASCII (src/text/text.c:79-92); record-lint REC_PATH, REC_POINTER, REC_TIMESTAMP; 16 KiB cap (record-lint:82) | closed |
| T-03-50 | Repudiation | parameter visibility | medium | mitigate | Cases record_model_bound, record_constants_shown; withheld union printed (text.c) | closed |
| T-03-51 | Information disclosure | trace of non-authored content | high | mitigate | `trace_fixture_permit` allows authored-and-listed or synthetic only (obskinds.c:1607-1615); case trace_out_rights | closed |
| T-03-52 | Tampering | path traversal via --trace-out | medium | mitigate | `trace_id_valid` [a-z][a-z0-9_]* (obskinds.c:1582-1587); paths built by `trace_paths` under build/gate/traces | closed |
| T-03-53 | Tampering | observer changing behaviour | high | mitigate | observer_* cases; sink shape matrix; toy trace-hook-mutates; sinks check refuses overlap | closed |
| T-03-54 | Repudiation | correlated oracle | medium | mitigate | bankclass-oracle.awk labelled E0/E2; rule-text drift guard (conformance-run:259); LED-CPU-0011 | closed |
| T-03-55 | Tampering | stale oracle file | low | mitigate | conformance-run removes and rewrites bankclass.tsv, requires 256 rows (conformance-run:205,260-265) | closed |
| T-03-56 | Repudiation | ST cite error | low | mitigate | LED-CPU-0016 cites H8-8 and S8-80; cites.txt notes the ROADMAP H8-3 error | closed |
| T-03-57 | Tampering | 64-bit time arithmetic | medium | mitigate | Unsigned division and modulo only; `period = reload + 1` (timer.c:22-40); time_wrap (run.c:100-104); case timer_pure_function | closed |
| T-03-58 | Denial of service | interrupt storms | low | mitigate | One TIQ latch bit (timer.c:76-85); runs bounded by `until_mclk` | closed |
| T-03-59 | Repudiation | circular timer evidence | medium | mitigate | tim_timer.checks and case labelled E2 (ER-3); only timer_pure_function backs LED-IRQ-0002 E1 | closed |
| T-03-60 | Tampering | irq_stim copy-in | medium | mitigate | `ds_irq_stim_set` validates count, line, level, order, state before any write (irq.c:55-81); cases irq_stim_refuse_* | closed |
| T-03-61 | Repudiation | stimulus misread as hardware | medium | mitigate | ER-5 label in irq cases (chunk_equiv_irq_ctl_pre, cpu_blk_irq_pending, brk_irq_sample); LED-IRQ-0008 status assumed | closed |
| T-03-62 | Tampering | toy anchors | low | mitigate | Single-statement anchors in tests/controls (irq-priority-swap, timer-reload-off); mutation-control-anchor selftest | closed |
| T-03-63 | Tampering | cycle length selection | medium | mitigate | Speed read once at step start (cpu.c:674,689); `ds_max_step_mclk` takes the low-speed worst case (run.c:61) | closed |
| T-03-64 | Repudiation | speed and penalty overclaim | medium | mitigate | LED-CPU-0006 status open and LED-CPU-0017 status assumed; README wording | closed |
| T-03-65 | Information disclosure | record values | low | mitigate | Same printer and record-lint rules as T-03-49; case record_stimulus_shown | closed |
| T-03-66 | Tampering | v1 freeze | medium | mitigate | Cases poweron_draw_order and poweron_v2_domain_independence present | closed |
| T-03-67 | Repudiation | fixtures dependent on power-on garbage | medium | mitigate | Three-policy equality plus policy-diff control (mmu_mpr_policy_diff, guest_policies_guard) | closed |
| T-03-68 | Repudiation | self-derived v2 literals | low | mitigate | Case poweron_v2_draw_order comment records the independent perl recompute | closed |
| T-03-69 | Denial of service | 65,536-byte transfers | medium | mitigate | One byte per step (cpu.c:648); no fast path; runs bounded by `until_mclk` | closed |
| T-03-70 | Tampering | address wrap and counter | high | mitigate | 16-bit masks (cpu.c:605,612); u32 `remaining` with 0 meaning 65,536 at prologue only (cpu.c:603); case cpu_blk_len0 in sanitizers | closed |
| T-03-71 | Denial of service | random programs running long | low | mitigate | Fixed absolute deadlines; 32 programs (chunk_equiv_random32, observer_random cases) | closed |
| T-03-72 | Tampering | memory safety on random paths | high | mitigate | Random cases not in EXCLUDED_IN_MODE (only two alloc cases excluded); stage sanitizers runs ASan and UBSan (CMakePresets.json:27-31; stage_sanitizers) | closed |
| T-03-73 | Repudiation | repeatability mistaken for correctness | medium | mitigate | ER-7 and E0 labels in chunk_equiv_random32 and observer_random | closed |
| T-03-74 | Tampering | input script parsing | high | mitigate | toml-subset.awk: no escapes, ASCII, 400-char lines (lines 20,29,30); input-check INP_STAGE_DEAD and 64 KiB cap (input-check:56,78); `INP_AT_TOO_LONG` | closed |
| T-03-75 | Denial of service | oversized schedules | medium | mitigate | 1024 events: `INP_TOO_MANY` (input-rules.awk:106,121); core `queue_full` (io.c:99-101); fixed `input_ev[DS_INPUT_MAX]` | closed |
| T-03-76 | Tampering | core trusting the checker | medium | mitigate | `ds_input_push` re-validates port, bits, order, state (io.c:95-130); cases input_refusal_rows, input_port_refusal | closed |
| T-03-77 | Tampering | numeric overflow in at | medium | mitigate | `INP_AT_TOO_LONG` over 15 digits and `INP_AT_LEADING_ZERO` (input-rules.awk:135-139) | closed |
| T-03-78 | Denial of service | oversized scripts | low | mitigate | `INP_TOO_LARGE` 64 KiB (input-check:78) and `INP_TOO_MANY` | closed |
| T-03-79 | Repudiation | which input a run used | low | mitigate | [input] identity in records; cases record_input_shown, record-lint | closed |
| T-03-80 | Spoofing | vacuous negative controls | medium | mitigate | pad_2btn_correct, _glitch, _late, _none, _wrong cases with declared outcomes | closed |
| T-03-81 | Tampering | shared input across instances | medium | mitigate | Case interleave_pad; toy control shared-input | closed |
| T-03-82 | Information disclosure | pad logs | low | mitigate | Logs written to evidence dir for authored fixtures only (SUMMARY 03-29; padlog under evidence); stdout carries ids and hashes | closed |
| T-03-83 | Tampering | malformed snapshot | high | mitigate | `walk(inst, buf, 0)` validates length, tags, region lengths, fields before any write (snapshot.c:610-623); cases snapshot_refusal_rows, snapshot_hostile | closed |
| T-03-84 | Denial of service | TIME near 2^64 | medium | mitigate | `t >= SNAP_TIME_LIMIT` (2^63) refused (snapshot.c:161,276); ds_run time_wrap (run.c:100-104) | closed |
| T-03-85 | Tampering | partial restore | high | mitigate | Commit pass runs only after pass 1 success (snapshot.c:623-627); no allocation; toy snapshot-drop-t | closed |
| T-03-86 | Tampering | memory safety in restore | high | mitigate | Case snapshot_hostile runs in sanitizers stage; ds_restore overlap, length checks (snapshot.c:600-620) | closed |
| T-03-87 | Repudiation | incomplete state hidden by restores | medium | mitigate | Toys snapshot-drop-blkt, snapshot-drop-t, snapshot-origin-any; cases snapshot_restore_* | closed |
| T-03-88 | Denial of service | gate time growth | low | mitigate | Corpus generated in-process and bounded; timings in docs/ci-timings.md | closed |
| T-03-89 | Repudiation | overclaimed support | high | mitigate | Seven supported entries only (ledger/entries LED-CPU-0001, -0002, -0005, -0014, -0016, LED-IRQ-0002, -0004), each with E1 line; `FXL_OCR_BACKS_SUPPORTED` rule (fixture-lint-rules.awk) | closed |
| T-03-90 | Spoofing | PDF identity | medium | mitigate | sha256 recorded (TOOLS.md) and checked per SUMMARY 03-32; process evidence | closed |
| T-03-91 | Information disclosure | manual content | medium | mitigate | cites.txt and TSV hold labels and numbers only; no PDF or render tracked | closed |
| T-03-92 | Repudiation | overclaiming in docs | high | mitigate | `DOCS_CLAIM_WORD`, `DOCS_SUPPORTED_LIST`, `DOCS_SUPPORTED_MEANING` (scripts/docs-check); README lists ids only; stage docs-claims-negatives | closed |
| T-03-93 | Elevation of privilege | unattended remote write | high | mitigate | No Phase 03 branch or tag on origin (`git branch -a`, `git tag`); CI recorded unverified (TOOLS.md, docs/ci-timings.md) | closed |
| T-03-94 | Tampering | ds_run refusal path | medium | mitigate | run_refuse_* cases and run_refusal_coverage check state before and after | closed |
| T-03-95 | Repudiation | regenerated goldens | medium | mitigate | Golden headers record host-tool recompute and the three changed values; reckinds.c epoch literal | closed |
| T-03-96 | Repudiation | silently dead lint stage | medium | mitigate | `FXL_STAGE_DEAD` on missing sentinel (fixture-lint-rules.awk:158,162); CR-01 fix in record-lint (no awk left of a pipe) | closed |
| T-03-97 | Repudiation | ledger overclaim | medium | mitigate | `ledger-check` E1 FACT rule (ledger-rules.awk:89-91); new entries assumed | closed |
| T-03-98 | Repudiation | ledger overclaim | high | mitigate | Same rule; LED-IRQ-0008 and LED-CPU-0017 assumed, LED-CPU-0006 open | closed |
| T-03-99 | Repudiation | circular timer evidence as fact | medium | mitigate | In-guest timer rows E2; only timer_pure_function backs E1 (LED-IRQ-0002) | closed |
| T-03-100 | Repudiation | interrupt model around transfers untested | medium | mitigate | Cases cpu_blk_irq_pending, chunk_equiv_cpu_blk_irq, mutate_sweep_cpu_blk_irq; ER-5 label | closed |
| T-03-101 | Tampering | brew install cc65 | high | mitigate | No network script in repo; identity by encoding matrix and hash pins; preflight fails closed (preflight:21-60; fixture-build:26) | closed |
| T-03-102 | Tampering | apt-get install cc65 in CI | high | mitigate | gate.yml "Install cc65" uses plain `apt-get install` (apt signature checking); `WF_NO_CC65_STEP` rule; identity by matrix and pins; CI not yet run (owner gate) | closed |
| T-03-103 | Tampering | PDF fetch and optional WLA | medium | mitigate | TOOLS.md records HTTPS mirror URL and hashes; `wla-huc6280` and `wlalink` absent on this machine; WLA never tracked | closed |
| T-03-104 | Tampering | PDF fetch for def transcription | low | mitigate | SUMMARY 03-09 states scratch copies reused, hashes equal; nothing committed | closed |
| T-03-105 | Tampering | PDF fetch | low | mitigate | SUMMARY 03-32: hash checked, scratch only; no PDF tracked | closed |
| T-03-106 | Tampering | brew install wla-dx (optional) | medium | mitigate | SUMMARY 03-33: bottle run once, uninstalled; `which wla-huc6280` finds nothing; `wla=optional-absent` path in xasm-check | closed |

*Status: open, closed, or open below block_on threshold (non-blocking).*
*Severity: critical > high > medium > low. Only open threats at or above `block_on: high` count toward `threats_open`.*
*Disposition: mitigate (implementation required), accept (documented risk), transfer (third-party).*

---

## Accepted Risks Log

No accepted risks. Every threat in this phase has disposition mitigate. Review-deferred items are recorded under Unregistered Flags, not as accepted risks.

---

## Unregistered Flags

SUMMARY Threat Flags sections state "None beyond the plan's register" for every plan. The audit found these items, none blocking.

| Item | Severity | Note |
|------|----------|------|
| Core review IN-03: `ds_run` does not refuse a request or result that overlaps the instance | low | Deferred in 03-REVIEW-FIX.md. Sinks are checked (obs.c:33-165). The overlap is a caller memory-contract fault, not a content boundary. Needs an owner choice: add a `memory_invalid` refusal or document the contract in `ds_core.h`. |
| `scripts/audit-history.allow` gained a ROM-SIZE-HDR allowance for `scripts/docs-check` (8704 bytes) | low | Scoped by path and CHECK, and only while the blob has no NUL byte (audit-history:26,201). It is a size coincidence, not a ROM. Review if the script size changes. |
| gate.yml second-awk step runs `sudo apt-get install` of gawk or mawk as a fallback | low | Same apt signature checking as the cc65 step; no secret, read-only token. Noted for the first CI run review. |
| Core review WR-01 (BRK sample) fix needs a human modelling check | info | Logic and modelling choice, ledger LED-IRQ-0005 stays open. Not a security finding. |

No obvious missed high-severity item was found.

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-11-11 | 106 | 106 | 0 | gsd-security-auditor (ASVS L1, block_on high; gate result taken from the verifier) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log (none)
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-11-11
