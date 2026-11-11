---
phase: "02"
slug: deterministic-machine-shell-hucard-boundary-and-fail-closed
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-11-10"
---

# Phase 02 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.
> Evidence basis: source read at the cited locations, plus a fresh run on 2026-11-10 of `sh scripts/check` (every stage PASS, RESULT PASS) and `sh scripts/check --selftest` (all mutation selftests PASS, exit 0).

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Host to core: HuCard bytes and boot request | Caller-supplied memory block, request struct, content bytes and result struct enter `ds_boot`; the core never opens files and never infers meaning from content | Untrusted ROM bytes, lengths, pointers, profile bindings |
| Core to host output (records, logs, CI) | Text and record printers emit state; ROM-internal bytes must never be echoed | Content id (SHA-256), sizes, machine-time counters, profile fields |
| Gate harness to repository files | Shell and awk gate scripts, the runner and ingest read repository files, manifests, TSVs and case files | Repository text, fixture manifests, results TSV |
| CI workflow boundary | GitHub Actions runs `scripts/check` on pull requests and pushes; one third-party action; no secrets | Workflow YAML, PR base SHA, checkout token (not persisted) |
| Public repository (rights boundary) | Nothing ROM-like, firmware-like, disc-like, save-like, credential-like or home-path-like may be committed | Commercial content, local paths, secrets |

---

## Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-02-01 | Tampering | run_controls refactor of six loops | high | mitigate | tests/check/equiv.sh (label-coverage and VACUOUS rules, lines 14-16, 213-223); 02-01-SUMMARY records 33 scenarios SAME and RESULT PASS; selftest mutation-run-controls PASS (scripts/check:1168) | closed |
| T-02-02 | Denial of Service | --stage flag | low | mitigate | `sh scripts/check --stage nonesuch` exits 2 (observed); documented at scripts/check:9 | closed |
| T-02-03 | Tampering | conformance-ingest accepting empty or marker-less run | high | mitigate | scripts/conformance-ingest codes ING_EMPTY, ING_NO_DONE, ING_DONE_COUNT, OUTCOME_MISSING_ARTIFACT with tests/gate/ingest controls; stage ingest-controls PASS; selftest mutation-ingest-skip-red PASS | closed |
| T-02-04 | Spoofing | forged or ambiguous outcome token, duplicate or unlisted id | high | mitigate | conformance-ingest ING_UNKNOWN_TOKEN, ING_DUPLICATE_ID, ING_UNLISTED_ID with EXPECT files under tests/gate/ingest; tokens from tests/conformance/outcomes.def; selftest mutation-outcome-names PASS | closed |
| T-02-05 | Tampering | partial TSV released after a parse error | medium | mitigate | scripts/conformance-flatten:70-148 holds key rows back until clean and requires one last END row (FLAT_NO_SENTINEL); stage flatten-controls PASS; selftest mutation-flatten-sentinel PASS | closed |
| T-02-06 | Repudiation | missing build tool silently skipping a stage | medium | mitigate | scripts/preflight exits 2 on an absent tool (lines 42-60); stage preflight PASS; empty-expect and filename controls PASS | closed |
| T-02-07 | Tampering | misaligned, NULL or short caller memory cast to instance | high | mitigate | src/core/boot.c check_memory is the first row of boot_order.def; NULL, cap and uintptr_t alignment tested before any cast; cases ref_mem_null, ref_mem_short, ref_mem_misaligned in REQUIRED; sanitizers stage PASS | closed |
| T-02-08 | Elevation of Privilege | binding overriding a closed field or value outside allowed | high | mitigate | boot.c check_bindings returns bind_closed_field, bind_not_allowed, open_unbound with field index; cases resolve_bind_closed, resolve_bind_not_allowed, resolve_open_unbound, ref_bind_closed, ref_bind_not_allowed, ref_open_unbound PASS | closed |
| T-02-09 | Tampering | core instance mutated by a refused boot | high | mitigate | boot.c ds_boot runs every check before memset; refusal returns through finish() writing only the result; case nonmutation_matrix and nonmutation_digest PASS | closed |
| T-02-10 | Tampering | runner trusting a truncated cases TSV | medium | mitigate | tests/conformance/runner.c:708-788 refuses a TSV without END, with a bad END count or rows after END; flatten prints nothing on error (T-02-05) | closed |
| T-02-11 | Tampering | ds_mapper_read out-of-range bank or offset | high | mitigate | src/core/mapper.c:62-87: bank >= DS_BANK_LIMIT returns UNMAPPED, offset >= 8192 returns BAD_OFFSET, index checked against image size and DS_ROM_CAP before rom[]; case accept_unmapped_banks; sanitizers PASS | closed |
| T-02-12 | Tampering | wrong SHA-256 giving false content identity | medium | mitigate | src/core/sha256.c; NIST literals, one-million-a feed and 21-length host-tool sweep run in the conformance stage (PASS); content id taken after all checks in boot.c | closed |
| T-02-13 | Information Disclosure | generator writing ROM-like bytes into tree | high | mitigate | images emitted on stdout/in memory only (fixtures/manifest holds TOML only; no *.bin tracked); rights-tree and rights-negatives stages PASS; `git ls-files` shows no ROM, disc, save or archive extension | closed |
| T-02-14 | Tampering | expectation sharing code with implementation | high | mitigate | tests/conformance/mirror-oracle.awk is an independent awk oracle with ORACLE_RULE_TEXT_DRIFT guard; wrong-mirror toy in tests/controls shown red by the named-controls stage (PASS) | closed |
| T-02-15 | Information Disclosure | commercial ROM, firmware or disc image committed | high | mitigate | scripts/rights-check default-deny content scan (NUL, CTRL, UTF8, NONASCII, EXT, MAGIC, BLOB, LINELEN, SHAPE; header lines 10-20); reads content not names and does not use .gitignore; stages rights-tree and rights-negatives PASS | closed |
| T-02-16 | Tampering | manifest allowlist abused to excuse a file | high | mitigate | rights-check:225-270: RIGHTS_BAD_ALLOW (unsafe path, non-lowercase-hex sha256, non-authored kind, non-SPDX licence), RIGHTS_MISSING_ARTIFACT, RIGHTS_HASH_MISMATCH; excuse never applies to a NUL file; WR-01 fix closed by rights-negatives PASS | closed |
| T-02-17 | Information Disclosure | add-then-delete blob via refs/pull/N/head; ROM re-encoded as short text | medium | accept | Residual risk written in scripts/rights-check:25-27 and plan summary; audit-history --range runs in CI and the full-history audit is an owner checkpoint (D-56, D-76) | closed |
| T-02-18 | Repudiation | selftest and controls vacuous so rights gate cannot fail | high | mitigate | selftest mutation-rights-rule PASS; tests/rights/synthetic.sh asserts exact code sets; awk-exit-status case rights-check-sha256 PASS (CR-01 fix) | closed |
| T-02-19 | Denial of Service | oversized or truncated image causing OOB read or huge allocation | high | mitigate | boot.c check_cap compares len to size_t DS_ROM_CAP (1 MiB) before any read; no allocator in src/core; cases ref_too_large_size_max, ref_too_large_1m_plus_8k; sanitizers and alloc shim PASS | closed |
| T-02-20 | Tampering | host mutating its buffer after load (TOCTOU) | high | mitigate | boot.c copies content into inst->rom with memcpy and hashes the copy; case boot_copy_in_isolated PASS; WR-02 overlap refusal (memory_overlaps) adds request, content and result overlap cases | closed |
| T-02-21 | Information Disclosure | ROM-internal strings echoed to logs, records or CI | high | mitigate | src/text/text.c emits only profile fields, sha256, size and counters (emit_content:214-240); core has no I/O; cases redaction_synthetic_rom and redaction_seeded_leak_flagged PASS | closed |
| T-02-22 | Tampering | content heuristics changing behaviour on adversarial bytes | high | mitigate | boot.c never reads bytes before checks; mapper and order come only from the request; cases noheur_mcgenjin_string, noheur_populous_string, noheur_reverse_vector; heuristic-reverse toy shown red | closed |
| T-02-23 | Tampering | refusal mutating caller memory or a live instance | high | mitigate | ds_boot refusal path is finish(result, reason, field, NULL) only; cases nonmutation_matrix and nonmutation_digest (byte and digest compare with canaries) PASS | closed |
| T-02-24 | Tampering | state field omitted from canonical digest | high | mitigate | cases digest_region, digest_stream_prefix, profile_hash_fields; drop-region toy red; src/core/serial.c framed region table. Informational: REVIEW IN-02 notes mapper, byte_order, header are not in a region (constants today) | closed |
| T-02-25 | Tampering | host-dependent bytes entering a record | high | mitigate | scripts/record-lint (REC_PATH, REC_POINTER, REC_TIMESTAMP) with planted controls; goldens (record_golden_384k_seeded); text.c put_quoted refuses quote, backslash, non-ASCII; selftest mutation-record-lint-pointer PASS | closed |
| T-02-26 | Spoofing | seed supplied under zero policy or missing under seeded policy | medium | mitigate | boot.c check_powerup returns powerup_seed_required and powerup_seed_unexpected; cases ref_seed_required, ref_seed_forbidden, poweron_seed_zero_supplied PASS | closed |
| T-02-27 | Repudiation | placeholder state evolution read as hardware claim | medium | mitigate | LED-PROF-0002 withheld list emitted via text.c emit_withheld (line 339); ledger-entries, profile-records stages PASS | closed |
| T-02-28 | Tampering | core using host clock, file, thread, env, device, UI or allocator | high | mitigate | scripts/core-boundary-check with scripts/core-boundary.allow; stages core-boundary and core-boundary-negatives PASS; selftest mutation-core-boundary-allow PASS; WR-03 float rule fixed with six negatives | closed |
| T-02-29 | Tampering | hidden mutable global or function-local static | high | mitigate | core-boundary-check writable-symbol rule with empty exception list; selftest mutation-core-boundary-writable PASS; core-boundary-negatives PASS | closed |
| T-02-30 | Repudiation | checker exiting 0 without checking anything | high | mitigate | scripts/check CHECKER_DEAD marker rule; CB_NO_SOURCES and CB_NO_SYMBOLS exit 2 in core-boundary-check; awk-exit-status selftest (9 sites) PASS closes CR-01 | closed |
| T-02-31 | Tampering | hidden shared state between instances | high | mitigate | cases interleave_ab, interleave_aba_order, interleave_canaries, interleave_destroy_recreate, interleave_twins; shared-state toy red; writable-symbol rule (T-02-29) | closed |
| T-02-32 | Information Disclosure | host environment leaking into records | high | mitigate | scripts/determinism-run runs a second process under env -i with differing TZ, cwd, TMPDIR and allocator scribbling, byte-compares, record-lints both; determinism and determinism-negatives PASS | closed |
| T-02-33 | Repudiation | determinism check that can never fail | high | mitigate | tests/gate/determinism differs control; selftest mutation-determinism-compare PASS; output labelled repeatability only | closed |
| T-02-34 | Tampering | fixture used undocumented or with changed generator | high | mitigate | scripts/fixture-check FIX_UNLISTED, FIX_HASH_MISMATCH, FIX_STALE (also FIX_SIZE_MISMATCH); tests/gate/fixture controls; selftests mutation-fixture-unlisted and mutation-fixture-missing-field PASS | closed |
| T-02-35 | Tampering | REQUIRED list shrinking or case files deleted | high | mitigate | scripts/inventory-check INV_REQUIRED_MISSING, INV_UNLISTED_CASE, INV_REQUIRED_EMPTY; removed-required control; selftest mutation-inventory-required-missing PASS; 104 REQUIRED ids and 104 case files | closed |
| T-02-36 | Spoofing | wrong expectation accepted as a pass | high | mitigate | tests/gate/wrong-literal control run through the live runner, non-green in ingest; selftest mutation-wrong-literal-compare PASS; NOT_RED check in conformance-run | closed |
| T-02-37 | Repudiation | verdict source that is the emulator's own output | medium | mitigate | scripts/fixture-check with scripts/lib/fixture-rules.awk FIX_BAD_VERDICT_SOURCE; tests/gate/fixture/bad-verdict-source control; fixture-negatives PASS | closed |
| T-02-38 | Tampering | workflow injection through untrusted expression in run block | high | mitigate | scripts/lib/workflow-rules.awk WF_EXPR_IN_RUN, WF_UNTRUSTED_EXPR; gate.yml passes BASE_SHA through env (Range audit step); tests/workflow/untrusted-expr control; workflow-negatives PASS | closed |
| T-02-39 | Tampering | compromised or moved GitHub Action | high | mitigate | .github/workflows/gate.yml pins actions/checkout to 3d3c42e5aac5ba805825da76410c181273ba90b1; git ls-remote v7.0.1 matches today; permissions contents read; persist-credentials false; .github/dependabot.yml monthly grouped; WF_UNPINNED_ACTION control; mutation-workflow-pin PASS | closed |
| T-02-40 | Elevation of Privilege | pull_request_target, workflow_run, secrets, self-hosted runners | high | mitigate | gate.yml triggers are pull_request, push main, workflow_dispatch only; no secrets, self-hosted or continue-on-error (grep empty); WF_FORBIDDEN_TRIGGER, WF_FORBIDDEN_KEY, WF_RUNNER with pull-request-target, secrets-ref, continue-on-error, or-true controls | closed |
| T-02-41 | Repudiation | workflow checker accepting YAML it cannot parse | high | mitigate | workflow-rules.awk WF_UNPARSABLE exit 2; unparsable control; selftest mutation-workflow-pin PASS | closed |
| T-02-42 | Information Disclosure | CI logs echoing ROM-like content | medium | mitigate | gate prints ids, outcomes, sizes and hashes only (text.c, redaction cases); rights-check and audit-history --range run in gate.yml | closed |
| T-02-43 | Information Disclosure | unauthorised push publishing history or content | high | mitigate | 02-22-SUMMARY records the owner answer authorised-branch-pr verbatim (2026-11-10): one branch, one PR, no push to main, no tag, no force; rights check and full-history audit were green first | closed |
| T-02-44 | Tampering | prompt injection in fetched pages, tag lookups, gh output | high | mitigate | 02-22-SUMMARY records gh output and logs read as data only; checkout SHA re-verified against the tag by plain ls-remote comparison (repeated in this audit: match) | closed |
| T-02-45 | Tampering | claiming criterion 5 or Linux coverage without evidence | high | mitigate | scripts/linux-repro:180-182 labels runs local one-offs, not a CI claim; docs-claims and docs-claims-negatives stages PASS; CI-01 ticked only on the green pushed head b063518 (02-22-SUMMARY) | closed |
| T-02-46 | Tampering | weakening a stage or control to pass a Linux-only failure | high | mitigate | 02-21-SUMMARY: the one Linux-only failure fixed at its cause; every stage and selftest still present and green in the current full gate and selftest run | closed |
| T-02-47 | Tampering | interrupted run read as green or watchdog conflated with cancelled | high | mitigate | scripts/conformance-run supervise mode: trap emits cancelled RESULT lines with signal source and RUNNER_DONE, wall expiry exit 2; stage cancel-controls PASS; selftest mutation-cancel-trap PASS | closed |
| T-02-48 | Tampering | C profile constant drifting from r1.toml | high | mitigate | cases profile_r1_equal, profile_names_both_ways, profile_hash_fields (13 fields) PASS in conformance stage; src/core/profile.c with profile_fields.def | closed |
| T-02-49 | Tampering | toy control library leaking into ds_core or silent-pass control | medium | mitigate | toys are separate targets built by conformance-run controls mode; NOT_RED fails the stage; empty EXPECT exits 2; named-controls stage PASS | closed |
| T-02-50 | Repudiation | mode-scoped exclusion growing silently | high | mitigate | tests/conformance/EXCLUDED_IN_MODE (2 entries); conformance-ingest EXCL_NOT_PASS_IN_GATE, EXCL_NOT_REQUIRED, EXCL_RAN_IN_MODE with controls; excluded-mode-controls PASS; mutation-excluded-gate-pass PASS | closed |
| T-02-SC | Tampering | npm, pip, cargo installs (supply chain) | low | accept | No package-manager installs in any plan; the one GitHub Action is covered by T-02-39; no dependency manifests added | closed |

*Status: open · closed · open — below {block_on} threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-02-01 | T-02-17 | Rights gate is detection, not prevention: an add-then-delete commit can still publish a blob via refs/pull/N/head, and a ROM re-encoded as short printable text can evade the shape rule. Pre-release audit-history and full-history audit are the compensating checks. | plan-time disposition (accept) | 2026-11-10 |
| AR-02-02 | T-02-SC | No package-manager installs in this phase; the only external dependency is one SHA-pinned GitHub Action, covered by T-02-39. | plan-time disposition (accept) | 2026-11-10 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-11-10 | 51 | 51 | 0 | gsd-security-auditor (Claude) |

Notes (informational, no threat reopened):
- Unregistered flags: none. Every 02-*-SUMMARY.md `## Threat Flags` section reads None.
- Prior review fixes CR-01, WR-01, WR-02, WR-03, WR-04 confirmed present; their controls pass in the current gate (stages awk-exit-status selftest, rights-negatives, conformance, core-boundary-negatives, audit-synthetic).
- REVIEW IN-01 (fixture-check IFS tab), IN-02 (digest omits mapper, byte_order, header) and IN-03 (REQUIRED grammar) remain open as info; none removes a declared mitigation.
- Remote repository settings (branch protection, rulesets) are owner hand-offs outside this phase's threat register.

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-11-10
