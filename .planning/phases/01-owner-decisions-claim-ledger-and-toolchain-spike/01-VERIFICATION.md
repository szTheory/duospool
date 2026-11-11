---
phase: 01-owner-decisions-claim-ledger-and-toolchain-spike
verified: 2026-11-10T00:30:00Z
status: passed
score: 6/6 truths verified (5 roadmap success criteria, 1 backstop plan truth)
covered_files:
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-01-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-01-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-02-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-02-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-03-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-03-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-04-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-04-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-05-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-05-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-06-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-06-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-07-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-07-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-08-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-08-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-09-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-09-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-10-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-10-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-11-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-11-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-12-PLAN.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-12-SUMMARY.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-LINUX-AWK-EVIDENCE.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-REVIEW-DISPOSITION.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-REVIEW-FIX.md"
  - ".planning/phases/01-owner-decisions-claim-ledger-and-toolchain-spike/01-REVIEW.md"
  - "docs/decisions/0001-language-build.md"
  - "docs/decisions/0002-first-profile.md"
  - "docs/decisions/0003-api-abi.md"
  - "docs/decisions/0004-licence-visibility.md"
  - "docs/decisions/0005-fixture-rights.md"
  - "docs/decisions/0006-ci-provider.md"
  - "docs/decisions/0007-copied-code-provenance.md"
  - "docs/decisions/0008-evidence-tier-policy.md"
  - "ledger/entries/LED-AUD-0001.toml"
  - "profiles/pce-hucard-2btn/r1.toml"
  - "scripts/audit-history"
  - "scripts/check"
  - "scripts/decision-check"
  - "scripts/ledger-check"
  - "scripts/lib/ledger-rules.awk"
  - "scripts/profile-check"
  - "scripts/protocol-order-check"
covered_digest: "v4:sha256:d22e9d83daf26241078d39c76a33173219a6db71c94e76d06881b7b1572fcad6"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: human_needed
  previous_score: "5/5 roadmap success criteria; 1 backstop truth abstained"
  gaps_closed:
    - "Stale present-tense text in record 0001 (Phase 2 stays blocked / no owner statement)"
    - "awk portability backstop truth (gawk and mawk on Linux), now directly observed"
  gaps_remaining: []
  regressions: []
---

# Phase 1: Owner Decisions, Claim Ledger and Toolchain Spike Verification Report

**Phase Goal:** The owner can confirm the decisions that gate the slice from recorded evidence. Before any code depends on a hardware fact the slice uses, that fact is in a claim ledger with its evidence tier and open/closed status.
**Verified:** 2026-11-10
**Status:** passed
**Re-verification:** Yes. The earlier report ended `human_needed` for two low-severity items; both are re-checked below. The five roadmap truths were regression-checked by re-running the gate, and the changed code (three critical review fixes) is covered by the same gate and negative controls.

## Goal Achievement

### Observable Truths (ROADMAP success criteria are the contract)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| SC1 | Language/build decision record with measured results from both skeletons on one manifest; owner's choice recorded with a date | VERIFIED (regression) | `docs/decisions/0001-language-build.md` front matter is `status: owner-recorded`, `owner-date: 2026-11-09`, owner chose C11 + CMake + Ninja. The M01-M15 results block, the scores file and the scorer output are intact, and the `results` and `results-negatives` gate stages pass. The earlier independent re-derivation of the scorer output from the evidence tag (C 79.00, Rust 80.20, `proposed-default`, `margin-below-10`) is unchanged because the results block and tag are unchanged. |
| SC2 | Versioned first-profile record naming every field, each owner-selected or open | VERIFIED (regression) | `profiles/pce-hucard-2btn/r1.toml` and record 0002 unchanged in substance; `profile-records` and `profile-negatives` stages PASS. The earlier note stands: "console model/region" is mapped deliberately to `media_slot`, `expansion_port`, `vdc_count`, `vpc`, `io_1000_d6_value` per D-28/D-32. |
| SC3 | Claim ledger with the required fields; a ledger check rejects a mislabelled "supported" entry on E0/E4 evidence only | VERIFIED (regression) | `ledger-entries`, `ledger-negatives`, `ledger-samples` stages PASS. The earlier independent out-of-repo variants (E0+E4-only `supported` rejected with `SUPPORTED_NO_QUALIFYING_EVIDENCE`; E1 plus a FACT source accepted; `SUPPORTED` case variant rejected as `UNKNOWN_STATUS`) were not re-run, and the rules file `scripts/lib/ledger-rules.awk` is in the covered set. CR-01 and WR-05 touched the checker input handling; the `filename-controls` and `empty-expect-controls` stages that cover those changes PASS. |
| SC4 | Each dependent conflict is an open ledger entry with a chosen modelling assumption | VERIFIED (regression) | Ten seed entries, all `assumed`, covering PSG clock, 262/263, VBlank/RCR IRQ, dot-clock encoding, 7 MHz width, `$1000` bit 6, byte order, copier header, PSG numbering, plus LED-PROF-0001. `ledger/REQUIRED` check (`required-missing`, `required-origin`, and the new WR-06 `REQUIRED_EMPTY` control) PASS inside `ledger-negatives`. |
| SC5 | Decision records for licence/visibility, fixture rights, API/ABI, CI provider, copied-code provenance, each owner-recorded or explicitly open; fixture-rights states no GPL or unclear-licence material admitted | VERIFIED (regression) | Records 0003-0007 `owner-recorded`; 0005 keeps "No GPL or unclear-licence material is admitted without a further owner decision." (guarded by `fixture-rights-sentence-missing` control); 0008 honestly `proposed-default`. `decision-records` and `decision-negatives` PASS. |
| P1 | (Plan 01-01, `verification: backstop`) Checkers give the same results under gawk and mawk on Linux as under macOS BWK awk | VERIFIED (directly observed, one-off local run, not a CI claim) | See "Behavioral Spot-Checks". I did not rely on `01-LINUX-AWK-EVIDENCE.md`: I reproduced the run myself (clean clone of HEAD d7fd3a7 mounted into `debian:stable-slim`, `/bin/sh` = dash). Under mawk 1.3.4 20250131 and GNU Awk 5.2.1 alike: `sh scripts/check` exit 0, 19 of 19 stage lines PASS, `CHECK RESULT PASS`; `sh scripts/audit-history .` clean; `tests/audit/synthetic.sh` and `tests/protocol/synthetic.sh` all PASS. Together with the macOS BWK awk 20200816 run this is three awk implementations with identical verdicts, including every negative control still rejected for its expected codes (the gate itself fails otherwise). The backstop rule asks for directly observed behavior; this is it. Limit: a local container run at one commit. The scheduled Linux CI job remains a Phase 2 deliverable, and the evidence file already says so. |

**Score:** 6/6 truths verified (0 behavior-unverified, 0 failed, 0 abstained).

### Re-check of the two prior human items

1. **Record 0001 stale text: resolved.** Lines 82-89 (Decision) state the owner choice and "Phase 2 is unblocked". The old paragraph (lines 91-103) is now headed "Scorer outcome at the time of the spike (historical, kept for the audit trail; ... supersedes them)", its opening sentence is past tense ("was open ... Phase 2 was blocked until the owner chose, and the owner has now chosen"), its status statements are past tense ("At that point the status was therefore `proposed-default`"), and it closes "Phase 2 is unblocked". It no longer contradicts the `owner-recorded` front matter. Two residual nits, both Info and not contradictions of the decision: line 19 still reads generically "Where it does not pre-authorize, this record stays proposed-default and Phase 2 is blocked" (a description of the pre-registered rule, immediately superseded by the Decision section), and line 115 says the tag is "local until plan 01-12 publishes it" although it has since been published. Worth a wording pass at the next edit of the record; not worth a gate.
2. **awk backstop: resolved**, see P1. The evidence file's own run reports "all 18 stages" while the gate now has 19 (`empty-expect-controls` was added by WR-05); my reproduction covered all 19 at the current HEAD, so the discrepancy does not weaken the conclusion.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `scripts/check` | Gate entry with stage lines and `CHECK RESULT` | VERIFIED | Run on macOS: 19 stages PASS, `CHECK RESULT PASS`, about 20 s. Also 19/19 PASS in Linux under mawk and gawk. |
| `scripts/audit-history` | History/secret audit (CR-02 shallow-repo refusal) | VERIFIED | `audit-history: scanned 77 commits, 533 blobs, 594 blob-path pairs`, `audit-history: clean`. Also clean in the Linux clone under both awks; shallow-clone control PASS. |
| `scripts/protocol-order-check` | Pre-registration order check (CR-03 exits 2 on git failure) | VERIFIED | `protocol-order` and `protocol-negatives` stages PASS; `SYNTH git-failure-*` PASS on Linux. |
| `scripts/ledger-check`, `decision-check`, `profile-check`, `scripts/lib/*.awk` | Checkers | VERIFIED | Wired from `scripts/check`, all stages PASS. |
| `ledger/`, `profiles/`, `docs/decisions/0001`-`0008` | Records | VERIFIED | As in SC1-SC5. |
| Tag `spike-tool-01-evidence` | Spike evidence | VERIFIED | Present in the repository (also fetched into the Linux clone); `spikes/` absent from the tree by design (D-16). |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `scripts/check` | all checkers | named stages | WIRED | 19 stage lines in the run output. |
| profile open fields | ledger entries | `ledger = [...]` | WIRED | `profile-records` PASS, dangling-ref control exists. |
| record 0001 results block | evidence tag | `check-results --record` | WIRED | `results` PASS. |
| README index | record front matter | `INDEX_DRIFT` | WIRED | Covered by `decision-records`. |

### Data-Flow Trace (Level 4)

Not applicable: no rendered dynamic data; records and shell/awk checkers only.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Gate passes (macOS BWK awk) | `sh scripts/check` | 19 `CHECK ... PASS`, `CHECK RESULT PASS` | PASS |
| History clean (macOS) | `sh scripts/audit-history .` | `scanned 77 commits ... audit-history: clean` | PASS |
| Gate under mawk 1.3.4 20250131, dash as sh, Linux (debian:stable-slim, clean clone of d7fd3a7) | `sh scripts/check` | exit 0, 19/19 PASS, `CHECK RESULT PASS` | PASS |
| Gate under GNU Awk 5.2.1, same container | `sh scripts/check` | exit 0, 19/19 PASS, `CHECK RESULT PASS` | PASS |
| audit-history and synthetic suites under mawk and gawk | `sh scripts/audit-history .`; `sh tests/audit/synthetic.sh`; `sh tests/protocol/synthetic.sh` | clean; AUDITTEST dup-blob-path, tag-only, shallow-clone PASS; SYNTH scorer-missing, git-failure-spikes, git-failure-protocol PASS (both awks) | PASS |

`sh scripts/check --selftest` was not run (per instruction). Probe execution: SKIPPED, the phase declares no `scripts/*/tests/probe-*.sh` probes.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| TOOL-01 | 01-03..01-12 | Language/build record from measured spike; owner-confirmed choice | SATISFIED | SC1 |
| PROF-01 | 01-02 | Named, versioned first-profile record | SATISFIED | SC2 |
| PROF-04 | 01-01 | Claim ledger with profile, tier, source, test link; E0/E4-only not "supported" | SATISFIED | SC3 |
| PROF-05 | 01-02 | Conflicts recorded open with a chosen modelling assumption | SATISFIED | SC4 |

No orphaned requirements (REQUIREMENTS.md maps only these four IDs to Phase 1).

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| scripts, docs, ledger, profile | - | TBD/FIXME/XXX | None found | No unreferenced debt markers. |
| `docs/decisions/0001-language-build.md` | 19, 115 | Present-tense phrasing that predates the owner decision; tag described as "local" | Info | Does not contradict the Decision section or front matter; wording pass suggested. |
| Phase review (`01-REVIEW.md`, `01-REVIEW-FIX.md`, `01-REVIEW-DISPOSITION.md`) | - | CR-01..03, WR-05, WR-06 fixed; WR-01..04, 07..12 and IN-01..04 open | Info (advisory for Phase 2) | Dispositioned in writing; none contradicts a success criterion. WR-07 (a deleted negative control is invisible to the gate) is the weakest point of the "gate is not vacuous" claim until Phase 2 adds a control inventory. |

### Human Verification Required

None. Both previous items are closed by evidence in the repository and by my own reproduction.

### Gaps Summary

No gaps. All five roadmap success criteria and the plan-level awk portability truth hold. The gate is green on macOS and on Linux under both mawk and gawk, the history audit is clean, record 0001 is internally consistent with its owner-recorded front matter, and the open code-review advisories are recorded for Phase 2 rather than hidden. Caveat stated plainly: the Linux result is a one-off local container run, not CI; the Phase 2 Linux job is still owed.

---

_Verified: 2026-11-10_
_Verifier: Claude (gsd-verifier)_
