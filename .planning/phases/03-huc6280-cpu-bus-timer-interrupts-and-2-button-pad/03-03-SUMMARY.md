---
phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
plan: 03
subsystem: core
tags: [digest, serial-version-2, cpu0-region, goldens, runner-headroom]
status: complete

requires:
  - phase: 03-huc6280-cpu-bus-timer-interrupts-and-2-button-pad
    provides: the CPU register block in struct ds_instance (plan 03-01) and ds_run (plans 03-02, 03-35)
provides:
  - digest stream serial version 2 with regions PROF CONT POWR TIME RAM8 MPRS CPU0 (PSGN and VDCP no longer in the stream)
  - CPU0 payload a x y s p pc(u16 LE) speed, eight bytes, with an adjacency _Static_assert chain for the accessor span
  - both goldens and the digest_stream_prefix literal recomputed outside the C code
  - runner.c at 44366 bytes; record_lines and the record parser in reckinds.c, content_id and hash_literal in the new hashkinds.c
affects: [03-04, 03-16, 03-22, 03-25, 03-29, 03-30, 03-31, 03-36]

actuals:
  tokens: 7500
  tasks: 2
  commits: 2
plan_head_before: d0f0d8c60f87d2736573179afc6affe238a72a67
plan_head_after: 5d30988929b9f021ea55e025f3e1e7db9a3c628a

tech-stack:
  added: []
  patterns:
    - "one explicit w_u8 or w_u16 line per CPU field in the stream, so a later plan can anchor on one line (plan 03-31 anchors the line that emits p)"
    - "a golden or digest literal is recomputed by a throwaway perl script that writes the stream of the documented rules, cross-checked with shasum -a 256 over the stream file"

key-files:
  created:
    - tests/conformance/hashkinds.c
  modified:
    - src/core/serial.c
    - src/core/ds_serial.h
    - src/core/ds_core.h
    - tests/conformance/digkinds.c
    - tests/conformance/cases/digest_stream_prefix.toml
    - tests/conformance/cases/digest_region.toml
    - tests/conformance/golden/accept_256k_zero.record.toml
    - tests/conformance/golden/accept_384k_seeded.record.toml
    - tests/conformance/reckinds.c
    - tests/conformance/runner.c
    - tests/conformance/runner.h
    - CMakeLists.txt

key-decisions:
  - "CPU0 stream order is a x y s p pc speed (the order the plan names), which differs from the memory order (pc first); the accessor span is the eight live bytes pc..speed, so its first live byte is the low byte of pc and its last is speed"
  - "sweep_len stays in runner.c: main prints it for --sweep-lengths and passes it to list_fixtures, so only the kinds moved"

requirements-advanced: [CORE-04, CORE-05]
requirements-completed: []  # none ticked: CORE-05 needs the snapshot and restore plans (03-30 and later); CORE-04 needs the later run plans

duration: 25min
completed: 2026-11-11
---

# Phase 3 Plan 03: Digest serial version 2 with CPU0 and runner headroom Summary

**The digest stream is on serial version 2 (PROF CONT POWR TIME RAM8 MPRS CPU0), its goldens and literals are recomputed outside the C code, and the record and hash kinds have left runner.c (66016 to 44366 bytes).**

## What was built

- **Task 1 (tracer), commit b06ae0e.** `serial.c` loses R_PSGN and R_VDCP and their emit cases and gains R_CPU0 (tag CPU0, 8 payload bytes: a, x, y, s, p, pc as a u16 little-endian, speed, each its own `w_u8`/`w_u16` line; B is never stored). Six adjacency `_Static_assert`s tie pc, a, x, y, s, p and speed into one live range for `ds_region_info`. `DS_SERIAL_VERSION` is 2 and the "eight regions" comment is replaced. The drop-region anchor line `w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);` is untouched. `digkinds.c` checks the CPU0 payload byte by byte against the instance (PSGN and VDCP branches removed); `digest_stream_prefix` and `digest_region` list the seven regions with the new literal. The psg_noise and vdc_phase fields stay in the instance until plan 03-36; only comments in `ds_core.h` changed.
- **Digest recomputation (T-03-09).** A throwaway perl script in the scratchpad (outside the repo; Digest::SHA and Math::BigInt) rebuilt the 256 KiB and 384 KiB recipe images from the lcg32 rule (content sha256 matched the fixture manifest), the seed-42 RAM bytes from the splitmix64 rule of poweron.c, and the CPU0 bytes from the reset rules (a x y s 0, p = the I bit, speed 0, pc = the reset vector at offsets 8190 and 8191 of file bank 0). It wrote each stream to a file and printed the SHA-256, which `shasum -a 256` over the same file confirmed. Values: 256k zero 31de49dd..., 384k seeded e0ec7c31..., 256k seed 42 eddf37b1.... The C output matched all three on the first run, so there was no copy from a run. The method is in the case and golden comments. The profile sha256 values in the goldens were reused unchanged (their derivation is unchanged by this plan).
- **Task 2, commit 5d30988.** `record_lines`, `parse_record`, `find_row` and their helpers moved to `reckinds.c`; `content_id`, `hash_literal`, `hash_recipe`, `to_hex` and `is_hex64` moved to the new `hashkinds.c`; `struct recrow`, the two row limits and the declarations moved to `runner.h`; `hashkinds.c` is in `DS_RUNNER_SOURCES` (the toy runners use the same list). A diff of removed against added lines shows only the `static` removals and two section comments differ. The pinned `n_exp == 61u` check moved with `kind_record_lines`. runner.c needs no new row (no new kind).

## Size record (F7, T-03-13)

runner.c 44366 bytes (was 66016); runner.h 11029; reckinds.c 27664; hashkinds.c 8184. None is a multiple of 8192 or 8192k+512 (nearest 8192 multiples 40960, 49152, 24576, 32768, 8192). `audit-history --range d0f0d8c..HEAD` is clean (2 commits, 13 blobs).

## Verification

- Task 1 verify as written: `--stage build`, `--stage conformance`, `--stage record-lint` PASS. Task 2 verify as written: `--stage build`, `--stage conformance`, `--stage outcome-sync` PASS.
- Mechanism check done by hand and reverted (not committed): swapping the x and p emission lines in serial.c turned `digest_stream_prefix`, `record_golden_256k_zero` and `record_golden_384k_seeded` guest_failure, so the tracer can fail. The restored file was byte-identical to the committed one.
- Plan end: `sh scripts/check` PASS (RESULT PASS), `sh scripts/check --selftest` PASS (includes the drop-region toy, still red as it must be), `dash scripts/check` PASS, `sh scripts/audit-history --range d0f0d8c..HEAD` clean, `grep -n 'PSGN\|VDCP' src/core/serial.c` prints nothing.

## Deviations from Plan

**1. [Rule 3 - Blocking] a comment line over 400 characters**
- **Issue:** `scripts/conformance-flatten` refused the lengthened comment in `digest_stream_prefix.toml` (LINE_TOO_LONG); the golden comment lines were long for the same reason.
- **Fix:** the comments are wrapped at about 140 characters. No content change. Commit: b06ae0e.

**2. Plan wording, no behaviour change:** the behaviour line says "field_flip on a CPU register changes the digest", but the existing `field_flip` kind flips profile fields only (its text and the 13-field count are unchanged, so `field_flip.toml` is not edited). The CPU register flip is covered by `digest_region` (first byte of the CPU0 span is the low byte of pc, last is speed) and by the byte-by-byte CPU0 payload check in `digest_stream`. A stronger all-eight-bytes flip with a non-boot register state belongs to the snapshot plans (03-30 and later); at boot a, x, y and s are all 0, so `digest_stream` cannot tell those four apart from each other (the swap mutation above did change p and was caught).

**3. Comment edit outside the plan file list:** `src/core/ds_core.h` comments on psg_noise and vdc_phase said psg_noise leaves in plan 03-03; they now say both leave the instance in plan 03-36. Comment only.

## Evidence discipline

E0 only. The digests pin the encoding and show repeatability; they do not show that CPU state is right. No ledger status changed, no ROM or third-party material was added, and no accuracy, compatibility, cycle-accuracy, hardware or boot claim is made.

## Known Stubs

None.

## Threat Flags

None beyond the plan's register. T-03-08 (explicit little-endian emit, adjacency asserts, CPU0 coverage, drop-region toy kept red), T-03-09 (digests recomputed outside the code) and T-03-13 (size recorded) are mitigated.

## Notes for the next plans

- Later region plans (16, 22, 25, 29) append rows to the `regions[]` table and `emit_payload` in `serial.c`, bump `DS_SERIAL_VERSION`, and must update `digest_stream_prefix` (frames and literal), `digest_region` (tags) and both goldens in the same task. The perl script is not in the repo; rebuild it from the rules in `serial.c` and the case comments (the CPU0 bytes at boot are a x y s = 0, p = 0x04, pc = the reset vector, speed 0).
- Plan 03-31 anchors the CPU0 line that emits P: it is the single line `w_u8(w, inst->p);`.
- `hash_recipe`, `parse_record` and `find_row` are now extern and declared in `runner.h`; new record or hash kinds go in `reckinds.c` and `hashkinds.c`, and runner.c needs only one declaration (in `runner.h`) and one `kinds[]` row per kind.
- CI: `.github/workflows/gate.yml` is unchanged and nothing was pushed (D-64); remote evidence stays unverified, pending owner push authorisation.

## Self-Check: PASSED

Created file tests/conformance/hashkinds.c present. Commits b06ae0e and 5d30988 are ancestors of HEAD.
