# Phase 3: HuC6280 CPU, Bus, Timer, Interrupts and 2-Button Pad - Pattern Map

**Mapped:** 2026-11-10
**Files analyzed:** 41 file groups (about 150 concrete files) from 03-CONTEXT D-90..D-134, waves W1..W8, and the Phase 2 amendment list
**Analogs found:** 36 / 41 groups (5 groups have no repo analog, listed under "No Analog Found"; 9 of the 36 are role-match or partial only)

**Scope rules used.**
1. Phase 2 is implemented and committed, so every analog below is a working-tree file. No spike-tag path is needed this time.
2. Every cited path was checked with `git ls-files -- PATH` and is git-tracked. No gitignored mirror is cited, and none exists in this repo.
3. The repo has NO CPU, bus, timer, pad or assembler code. "Analog" for the new C files means the closest Phase 2 file with the same
   role and data flow (X-macro table, const table indexed by an enum, canonical stream, runner kind file).
4. Line numbers are at HEAD (`277a13a`). Re-read the range before editing: Phase 3 waves move them.

## Findings the CONTEXT does not state (code forces these; schedule around them)

- **F1 Case schema has three sync points.** Optional keys (`until_mclk policies checks input irq_stim sinks chunks`) need edits in all three:
  - `tests/conformance/runner.h:19` (the `K_` enum, `NKEYS`) and `tests/conformance/runner.c:624-630` (`key_def`: only type letters s, b, a exist, so add `i` for integers).
  - `tests/conformance/runner.c:1556-1560`: `run_case` calls `harness_error("a key is missing")` for EVERY index below `NKEYS`. Optional keys need a mask there.
  - `scripts/lib/inventory-rules.awk:22-25`: the `want` table. An unknown key is `INV_CASE_FIELD`. Its header comment names "the ten keys".
- **F2 Fixture names cannot contain an underscore.** `scripts/fixture-check:76` requires the used-list NAME to match `^[a-z0-9:-]+$`, and `scripts/conformance-run:436` has `*[!a-z0-9:-]*`.
  - The manifest id is the NAME with `-` and `:` turned into `_` (`scripts/fixture-check:123`; `scripts/check:784-801` does the same with `tr ':-' '__'`).
  - So `fixture-build` must print `cpu-ldst` for manifest id `cpu_ldst`, or widen both regexes in the same plan.
- **F3 The used list has one owner today.** `stage_fixtures` (`scripts/check:784`) does `rm -f build/gate/fixtures.used`, then `fixtures_mode` (`scripts/conformance-run:427-489`) truncates it and fills it only from `ds_runner --fixtures`.
  - D-123 says `fixture-build` also writes `build/gate/fixtures.used`. One of the two must append, or `fixtures_mode` must call `fixture-build`.
  - `tests/conformance/fixlist.c` resolves every case fixture through `synth_*` and exits 2 on an unknown recipe, so `guest` cases need a skip rule by kind there.
- **F4 Refusals are closed and coverage-checked.** `src/core/refusals.def` has 22 of 32 slots (`DS_REFUSAL_SLOTS`, `src/core/ds_core.h:39`), every name prints as `load.NAME` (`src/core/profile.c:170-178`).
  - `kind_coverage` (`tests/conformance/refkinds.c:52-74`) fails when any member was not observed by a `refuse` row.
  - Run, input-push, model-param and restore refusals therefore belong in their own `.def` and enum (own name prefix), not in `refusals.def`.
- **F5 Toy controls and the runner inputs.** `controls_mode` (`scripts/conformance-run:309-399`, toy invocation line 375) passes the toy runner only `--cases --evidence-dir --profile-tsv --rules-fields --oracle --marker-seed --golden-dir`.
  - Any new runner input (`--fixture-dir`, `--bank-oracle`, `--opcode-tsv`, `--input`) must be added to BOTH invocations (line 209 and line 375).
  - A toy whose listed case cannot find its fixture gives `harness_failure`, but the check at line 388 demands `guest_failure`.
  - Toys are built one at a time in the loop at lines 352-380. "Built in parallel" (D-131) needs one `cmake --build --target ds_toy_a ds_toy_b ...` call.
- **F6 Workflow mini-roots are copies of the real workflow.** All 12 `tests/workflow/*/.github/workflows/gate.yml` are the real file with one line changed (the diff is 0 or 2 lines).
  - `stage_workflow_negatives` (`scripts/check:856-860`) `cmp`s `ok-gate` with the real file.
  - Adding the `apt cc65` step means regenerating all 12 copies (ok-gate and missing-template-line are byte copies, the other ten differ by one line) (derive them by script from the real file: one `cp` plus one `sed` each).
- **F7 The "65536-byte" runner.c limit is really a size-shape hazard.** Phase 2 notes call it a "rights-tree 65536-byte limit", but no current check caps file size.
  - `tests/conformance/runner.c` is 65876 bytes at HEAD, already past 65536, and the gate is green.
  - The real hazard is `scripts/audit-history:194-195`: it flags EVERY blob of any commit whose size is >= 4096 and `size % 8192 == 0` (`ROM-SIZE-8K`) or `== 512` (`ROM-SIZE-HDR`), text included.
    Only `scripts/audit-history.allow` (path plus check, NUL-free) excuses it. `rights-check` exempts valid text from the shape rule (D-76); `audit-history` does not.
  - runner.c is `8*8192 + 340`, so net growth near 7.8 KB passes through 73728 (`9*8192`). New runner logic goes in new `*kinds.c` files; runner.c gets one declaration and one table row per kind and must not grow.
- **F8 ca65 comments collide with `provenance-check`.** The marker head is `^[[:space:]]*(//|#|;|\*|/\*)[[:space:]]*Provenance: ` (`scripts/provenance-check:58`), and `;` is the ca65 comment character.
  Never write a line-start `; Provenance: PROV-NNNN` in a fixture source unless a real register entry exists.
- **F9 One ledger entry pins exactly one case.** `scripts/inventory-check:140-158`: an entry's `pinning_test = "TEST:id"` must be named back in that case's `pins`, and a case may list many ledger ids.
  Plan the 32 new entries against about 50 new cases accordingly (many entries can share one pin case).
- **F10 The record layout is pinned in four places.**
  - `tests/conformance/runner.c:968-1008` (`build_expected_keys`) and `:1171` (a pinned row count of 61).
  - `scripts/lib/record-rules.awk:23-33` (`keys[]`, `seq_accept`), `src/text/text.c:468-485` (`ds_text_record`), `DS_TEXT_FORMAT_VERSION` (`src/text/ds_text.h:34`).
  - `[model_<name>]`, `[input]`, `[stimulus]`, `[run]` touch all four, plus the two goldens and 9 record-lint and determinism mini-root files.
- **F11 RESULT grammar already accepts two optional tokens.** `scripts/conformance-ingest:211-213` allows `machine_time=N` and `stop=REASON` after `evidence=`, but `run_case` (`tests/conformance/runner.c:1590`) prints neither.
  D-95 stop reason and overshoot can be printed there without touching the judge. The control matchers compare only fields 1-3.
- **F12 The power-on policy names live in 7 places.** `src/core/profile_fields.def:32`, `tests/conformance/profkinds.c:188`, `src/core/ds_poweron.h:18`, `src/core/poweron.c:72-78`,
  `src/core/boot.c:135-141` and `:354`, `src/text/text.c:377-381`. `seeded-prng-v2` (W4) edits all of them.
- **F13 Only some scripts on the CONTEXT migration list contain placeholder code.** At HEAD `scripts/check`, `scripts/conformance-run`, `scripts/determinism-run` and `scripts/outcome-sync`
  hold no `ds_advance`, `psg_noise`, `vdc_phase` or `placeholder` text. Only `scripts/conformance-ingest` mentions `machine_time` (the grammar comment).
  Their Phase 3 edits are for new stages and arguments, not for the unit migration.

## File Classification

Waves: W1 tracer, W2 instruction set, W3 bus/MMU, W4 timer/IRQ/speed, W5 block/chunk, W6 pad/input, W7 snapshot, W8 close-out.

| New/Modified file group | Role | Data flow | Closest analog | Match | Wave |
|---|---|---|---|---|---|
| `src/core/ds_core.h`, epoch and serial bump | core header | transform | itself (15-30, 113-130, 165-185) | exact | W1+ |
| run API: `src/core/run.c` (+ header), replaces `advance.c` | core service | request-response | `src/core/boot.c` 273-362 + `src/core/advance.c` 43-64 | role-match | W1, W5 |
| `src/core/cpu_ops.def` + `cpu*.c` | table + dispatch | transform | `src/core/refusals.def`, `boot_order.def`, `boot.c` 300-309, `mapper.c` 15-43 | role-match | W1, W2, W5 |
| `src/core/bus.c`, `bank_class[256]`, MMU, windows | core service | request-response | `src/core/mapper.c` 35-87 | role-match | W1, W3 |
| `src/core/model_params.def` + accessors | X-macro table | transform | `src/core/profile_fields.def` + `profile.c` 13-52 | exact | W3 |
| `src/core/timer.c`, `irq.c` | core service | event-driven (pure query on mclk) | `src/core/poweron.c` 22-98, `serial.c` 35-55 | partial | W4 |
| `src/core/pad.c` (+ input schedule copy-in) | core service | request-response | `src/core/boot.c` 273-362 (copy-in), `mapper.c` 62-87 | partial | W6 |
| `src/core/serial.c` + `ds_serial.h` (regions, snapshot) | serializer | transform | itself (120-231) | exact | W1, W3-W7 |
| `src/core/restore.c` (`ds_restore`) | core service | transform | `src/core/boot.c` 300-362 (validate then commit) | role-match | W7 |
| `src/core/poweron.c` + `ds_poweron.h` (`seeded-prng-v2`) | core util | transform | itself (22-98) | exact | W4 |
| `src/text/text.c`, `ds_text.h` (unit, `[model_*]`, `[input]`, `[stimulus]`, `[run]`) | printer | transform | itself (339-410, 459-485) | exact | W1, W3, W4, W6 |
| new runner kind files (`guest`, `mutate_sweep`, `chunk_equiv`, `observer`, `snapshot`, pad, bus, irq) | runner kinds | batch | `tests/conformance/spinkinds.c`, `ilvkinds.c`, `mapkinds.c` | role-match | W1-W7 |
| runner kind table, `runner.h`, `CMakeLists.txt` source list | registration | config | `runner.c` 1478-1524, `runner.h` 19-36/176, `CMakeLists.txt` 62-67 | exact | each |
| migrated kinds: `stepkinds.c`, `spinkinds.c`, `powkinds.c`, `ilvkinds.c`, `digkinds.c` | runner kinds | batch | themselves | exact | W1 |
| `tests/conformance/cases/*.toml`, `REQUIRED`, goldens | records | CRUD | `cases/poweron_draw_order.toml`, `golden/accept_256k_zero.record.toml` | exact | each |
| `scripts/fixture-build` (new) | gate tool | batch, file-I/O | `scripts/conformance-run` 408-489 + `scripts/preflight` | role-match | W1 |
| `scripts/fixture-lint` + `scripts/lib/fixture-lint-rules.awk` (new) | checker | batch | `scripts/fixture-check` + `scripts/lib/fixture-rules.awk` | exact | W1, W2 |
| `scripts/xasm-check` (new) | checker over an external tool | batch | `scripts/core-boundary-check` | role-match | W2 |
| `scripts/input-check` + `scripts/lib/input-rules.awk` (new) | checker | batch | `scripts/conformance-flatten` + `scripts/lib/inventory-rules.awk` | role-match | W6 |
| opcode oracle checker (new, awk) | checker | transform | `scripts/outcome-sync` 55-100, `tests/conformance/mirror-oracle.awk` | role-match | W2 |
| `tests/oracle/huc6280-opcodes.tsv`, bank-class oracle | oracle data / awk | transform | `tests/conformance/mirror-oracle.awk` | role-match | W2, W3 |
| `scripts/conformance-flatten` (INPUT rows), `scripts/conformance-run` (args, modes) | harness | batch | themselves | exact | W1, W6 |
| `scripts/preflight`, `stage_preflight` | tool presence | batch | itself (21, 185-196) | exact | W1 |
| `scripts/check` (stages, timing rows, selftest mutations) | gate entry | batch | itself | exact | each |
| `scripts/lib/inventory-rules.awk`, `fixture-rules.awk`, `record-rules.awk` | stage-2 rules | transform | themselves | exact | W1, W3 |
| `scripts/licence-check` (tests/oracle) | checker | batch | itself 106-126 | exact | W2 |
| `scripts/workflow-check`, `workflow-rules.awk`, `.github/workflows/gate.yml`, 12 mini-roots | CI config + checker | event-driven | themselves | exact | W1 |
| `scripts/linux-repro`, `README.md`, `scripts/docs-check`, `docs/ci-timings.md`, `.gitignore` | docs / tooling | n/a | themselves | exact | W1, W8 |
| `fixtures/src/*.s`, `fixtures/lib/{hucard.cfg,startup.s,verdict.inc,check.inc,cites.txt,TOOLS.md}`, `*.checks` | authored guest source | file-I/O | none (closest `tests/conformance/synth.c`) | none | W1-W6 |
| `fixtures/manifest/<id>.toml` (authored) | record | CRUD | `fixtures/manifest/plain_256k.toml` | exact | W1+ |
| `ledger/entries/LED-*.toml` (32 new, 3 edits), `ledger/REQUIRED` | record | CRUD | `ledger/entries/LED-LOAD-0003.toml`, `LED-PROF-0002.toml` | exact | each |
| `tests/controls/<name>/{RULE,EXPECT}` toys | control | transform | `tests/controls/wrong-mirror/`, `shared-state/` | exact | W1-W7 |
| `tests/gate/<family>/<case>/` mini-roots (inventory, fixture, input, xasm, protocol) | control | batch | `tests/gate/fixture/*`, `tests/gate/flatten/*`, `tests/gate/core-boundary/*` | exact | each |
| `tests/gate/{cancel-spin,timeout-budget,wrong-literal}` | live-runner controls | batch | themselves | exact | W1 |

## Pattern Assignments

### P1. `src/core/ds_core.h` (modify): unit migration and instance growth (W1, then each wave)

**Analog:** itself. Keep the structure: constants block (15-30), enums from `.def` files (34-52), instance (113-130), boot request/result (132-163).

**Constants to change** (`src/core/ds_core.h:21-30`):
```c
/* behaviour epoch (D-53): bumped by any change that can alter what a run observes; goldens compare it, never a build id */
#define DS_BEHAVIOUR_EPOCH 1u
#define DS_POWERON_POLICY_VERSION 1u
#define DS_RAM8K_BYTES 8192u
#define DS_MPR_COUNT 8u
#define DS_ADVANCE_MAX 1048576u
```
**Differs:**
- Epoch 1 to 2 (W1). Delete `DS_ADVANCE_MAX` and the `ds_advance`/`ds_run`/`ds_stop_fn` block (165-185).
- `DS_POWERON_POLICY_VERSION` becomes a per-policy lookup (v1 = 1, v2 = 2).
- `machine_time` (line 114) keeps its name; its comment becomes "master clocks". Remove `psg_noise` and `vdc_phase` (124-125).
- Add CPU, bus, timer, interrupt, schedule and stimulus state (D-42 growth: still no malloc, no pointers).

Also:
- Every new member needs a `_Static_assert` in the tiling list (W7) and a region in `serial.c` (P8).
- Keep `static inline ds_streq` (187-195) and the rule that only `stddef.h`, `stdint.h`, `stdbool.h` are included.
- New headers must be named `ds_*.h` (core-boundary include rule, P23).

### P2. Run API: `src/core/run.c` plus `ds_run.h` replacing `advance.c` (W1; stop points W5)

**Analog 1 (request and result shape):** `src/core/ds_core.h:132-163` (`struct ds_boot_request` with `struct_size` and `reserved`, `struct ds_boot_result`) and the check order in `src/core/boot.c:321-362`.
**Analog 2 (overlap refusal for sinks):** `src/core/boot.c:273-298`, the integer-difference range test:
```c
static int ranges_overlap(uintptr_t a, size_t an, uintptr_t b, size_t bn)
{
    if (an == 0u || bn == 0u) {
        return 0;
    }
    if (a <= b) {
        return (b - a) < (uintptr_t)an;
    }
    return (a - b) < (uintptr_t)bn;
}
```
**Analog 3 (loop shape):** `src/core/advance.c:43-64` (`ds_run`: stop checked before the first step and after each step; a `NULL` stop never fires).
**Differs:**
- Absolute `until_mclk`, no budget arithmetic (D-93). `until <= machine_time` returns UNTIL with 0 steps.
- The result carries reason UNTIL=1, CALLBACK=2, REFUSED=3, machine time, overshoot, stop point, steps.
- All state lives in the instance between steps. A sink overlapping the instance, request or content is refused `memory_invalid` (extends WR-02).
- Refusal codes for the run request go in their own `.def` (F4).
- Use `__builtin_memcpy/memset` only (D-54). Unsigned masks for every 8/16-bit value (D-83: no `-fwrapv` on the sanitize preset).

### P3. `src/core/cpu_ops.def` and `cpu*.c` (W1 about 12 rows, W2 all 256, W5 block states)

**Analog (def style):** `src/core/refusals.def:1-12` and `profile_fields.def:20-26`: SPDX block comment, a header comment that defines each column, then one X-macro row per line.
```c
/* refusals.def */
DS_REFUSAL(memory_invalid, 1)
DS_REFUSAL(request_invalid, 2)
/* profile_fields.def */
DS_FIELD(media_slot, OWNER, "hucard", "hucard", "")
DS_FIELD(io_1000_d6_value, OPEN, "", "0|1", "LED-IO-0001")
```
**Analog (expansion into `static const` tables):** `src/core/boot.c:300-309` and `src/core/profile.c:21-25`:
```c
static const struct {
    const char *name;
    boot_check_fn fn;
} boot_order[] = {
#define DS_CHECK(name) {#name, check_##name},
#include "boot_order.def"
#undef DS_CHECK
};
```
**Analog (dispatch through function pointers in a const table indexed by an enum):** `src/core/mapper.c:15-43` (`mirror_fn`, `size_row[DS_SIZE_COUNT]` with designated initializers).
**Differs:**
- Row = `DS_OP(opcode, mnemonic, mode, bytes, cycles, access, cite)`, exactly 256 rows in opcode order. The 22 undefined rows are 1-byte 2-cycle NOPs that clear T (D-96).
- No computed goto, no GNU extensions, no `float`. `DS_MAX_STEP_MCLK` is computed from the table at compile time (D-94), never a guessed constant.
- Both consumers of the def are read by awk too (oracle checker, P18): one row per line, no continuation lines, the row name token first, like `outcomes.def` rows read by `scripts/outcome-sync:67-68`.
- Flag helpers use explicit unsigned masks and explicit casts: gcc and clang disagree about `-Wconversion` in opposite directions (02-PATTERNS gotcha).
- All tables `static const`: `core-boundary-check` reports any writable symbol as `CB_WRITABLE` (empty exception list).

### P4. `src/core/bus.c`, `bank_class[256]`, hardware-page windows (W1 minimal, W3 full)

**Analog:** `src/core/mapper.c`. Same shape: one const table, one choke-point function returning a status, no read unless the lookup says OK.
```c
/* mapper.c:35-43 */
static const struct {
    uint32_t bytes;
    mirror_fn mirror;
} size_row[DS_SIZE_COUNT] = {
    [DS_SIZE_256K] = {262144u, mirror_256k},
    [DS_SIZE_384K] = {393216u, mirror_384k},
    [DS_SIZE_1M] = {1048576u, mirror_1m},
};
```
```c
/* mapper.c:74-86: a bank at or above $80 never reaches the table; an index outside the image is refused */
    if (bank >= DS_BANK_LIMIT) { return (uint32_t)DS_MAP_UNMAPPED; }
    index = size_row[cls].mirror(bank) * DS_BANK_BYTES + offset;
    if (index >= size_row[cls].bytes || index >= DS_ROM_CAP) { return (uint32_t)DS_MAP_UNMAPPED; }
```
**Differs:**
- Per-access translate `phys = (mpr[a >> 13] << 13) | (a & 0x1FFF)` through a `bank_class[256]` const table (HuCard, unmapped, RAM mirror x4, hardware page).
- Call `ds_mapper_read` only for banks below `$80` (this fulfils LED-LOAD-0005). One `switch` on `offset >> 10` for the hardware page (no function-pointer framework, D-103).
- One `ds_mpr_write` choke point. Every access carries its mclk stamp. `DS_UNMAPPED_READ` is one named constant `0xFF`.
- The 8 KiB RAM mirror uses `& $1FFF` guarded by a `_Static_assert` (D-103), same style as `serial.c:130-133`.
- Do NOT edit `ds_mapper_read`'s contract: `accept_unmapped_banks` and `tests/conformance/mapkinds.c:110-151` keep testing it. Add bus-level cases instead.
- An independent bank-class oracle follows `tests/conformance/mirror-oracle.awk` (rule-text guard, own arithmetic, P18).

### P5. `src/core/model_params.def` and parameter accessors (W3)

**Analog:** `src/core/profile_fields.def` with its generated accessors in `src/core/profile.c:13-52`. Four consumers from one def (names, allowed sets, ledger ids, defaults), each its own `#define ... #include ... #undef` block:
```c
#define DS_FIELD(name, kind, value, allowed, ledger) allowed,
static const char *const ds_field_alloweds[DS_FIELD_COUNT] = {
#include "profile_fields.def"
};
#undef DS_FIELD
```
**Differs:**
- At most 4 rows. A row exists only if two values are implemented AND a fixture distinguishes them (`irq_i_sample` pre|post, `tma_zero` bus_latch|a_unchanged).
- Columns: name, allowed set, default, ledger id, status.
- Run-request keys `model.<name>` follow the `bind.<field>` branch of `parse_spec` (`tests/conformance/runner.c:262-267`) and a request-side struct like `struct ds_bindings` (`ds_core.h:86-88`: `NULL` = unbound, text checked against the allowed set).
- A value outside the set is a typed refusal (own `.def`, F4) and nothing is written. Changing a default bumps `DS_BEHAVIOUR_EPOCH` (D-113).
- A both-ways name check against the record printer follows `names_both_ways` (`tests/conformance/profkinds.c`, `scripts/lib/profile-rules.awk:21`).

### P6. `src/core/timer.c`, `irq.c` (W4)

**Analog:** `src/core/poweron.c:22-98` for constants plus pure functions over instance state, and `src/core/serial.c:35-55` for explicit little-endian integer handling. No better analog exists (see No Analog Found).
**Differs:**
- Closed form on mclk: a step every 3072 mclk from `origin`, period (reload+1) steps.
- Timer state mutates only inside guest accesses at their stamps. Everything else is a pure query of `(origin, counter, reload, enabled, latch, now)` (D-109).
- `irq_i_sample` reads the model parameter (P5). Interrupt entry runs in the SAME step as its instruction: nothing to persist, nothing to snapshot.
- 8/16-bit arithmetic is unsigned and masked. 64-bit mclk division is plain unsigned `/` and `%` (no floating point anywhere: `CB_FLOAT`).

### P7. `src/core/pad.c`, input schedule copy-in, `ds_input_push` (W6)

**Analog:** `src/core/boot.c:321-362` for the copy-in discipline (validate everything first, then write into the instance; a refusal writes nothing), and `ds_mapper_read`'s status-return style.
**Differs:**
- Immutable schedule copy of up to 1024 events x 16 B inside the instance, read by timestamp, no cursor (D-117).
- `ds_input_push` is accepted only before the first step. Refusals: port, state, bits, order, queue_full, late (own `.def`, F4).
- `o_latch` and `slot_type[5]` in one region `IOPT` (D-116). A read of the `$1000` window is a pure function of `o_latch`, the schedule and the stamp.

### P8. `src/core/serial.c`, `ds_serial.h`: regions and snapshot (W1, W3, W4, W6, W7)

**Analog:** itself. One region table serves digest, `ds_digest_stream` and the `ds_region_info` accessor (`serial.c:118-145`); a frame is a 4-byte tag, a u32 length, the payload.
```c
/* serial.c:120-145 */
enum region_id { R_PROF, R_CONT, R_POWR, R_TIME, R_RAM8, R_MPRS, R_PSGN, R_VDCP, R_COUNT };
_Static_assert(offsetof(struct ds_instance, has_seed) == offsetof(struct ds_instance, powerup_policy) + sizeof(uint32_t),
               "powerup_policy and has_seed are not adjacent");
static const struct region regions[R_COUNT] = {
    [R_TIME] = {"TIME", 8u, offsetof(struct ds_instance, machine_time), sizeof(uint64_t)},
    [R_RAM8] = {"RAM8", DS_RAM8K_BYTES, offsetof(struct ds_instance, ram8k), DS_RAM8K_BYTES},
    [R_MPRS] = {"MPRS", DS_MPR_COUNT, offsetof(struct ds_instance, mpr), DS_MPR_COUNT},
};
```
**Differs:**
- Remove `R_PSGN` and `R_VDCP` (rows at 143-144, `emit_payload` cases at 177-182). Add `CPU0 BLKT BUS0 TIMR IRQC IOPT SCHD IRQS` (D-121).
- `DS_SERIAL_VERSION` (`ds_serial.h:22`) goes 1 to 2 in W1 and again per wave that changes the layout. The comment at `ds_serial.h:7` ("eight regions") is stale after W1.
- The snapshot is the digest stream byte for byte (so `sha256(snapshot) == digest`); `ds_snapshot_size()` is a pure function of the table.
- Keep this exact line in `serial.c` (the drop-region toy anchors on it, `tests/controls/drop-region/RULE`): `w_bytes(w, inst->ram8k, DS_RAM8K_BYTES);`.
- Every new multi-field region that is also a contiguous live range needs the same adjacency `_Static_assert` as `POWR`.

### P9. `src/core/restore.c`: `ds_restore` two-pass (W7)

**Analog:** `src/core/boot.c:300-362`: an ordered list of checks that cannot write, then the commit (`__builtin_memset`, field-by-field copy). `boot_order.def` is the model for "order is behaviour, a swap of two rows must go red" (selftest `mutation-drop-precedence`, `scripts/check:1423-1475`).
**Differs:**
- Pass 1 validates prefix, version, tag order, lengths, trailing bytes and every field domain (speed 0|1, timer counter <= 127, block phase enum, remaining 1..65536 only when active, reserved bits zero).
- Pass 2 commits and cannot fail. A refusal leaves the instance byte-identical (assert it with the digest and a raw `memcmp`, as `nonmutation_digest` does in `tests/conformance/digkinds.c`).
- Profile hash and content id must match (else refused).

### P10. `src/core/poweron.c`, `ds_poweron.h`: `seeded-prng-v2` (W4)

**Analog:** itself, additive only (v1 frozen byte for byte, D-114).
```c
/* poweron.c:22-27 */
static const uint64_t domain_const[DS_DOMAIN_COUNT] = {
    UINT64_C(0x52414D384B5F5F5F), /* ram8k */
    UINT64_C(0x5053474E4F495345), /* psg_noise */
    UINT64_C(0x5644435048415345), /* vdc_phase */
};
```
**Differs:**
- v2 draws v1's domains unchanged, then fixed-constant domains `cpu`, `mpr`, `tmr` (ASCII-packed like the existing three, never enum ordinals). No domain for `io_buffer` or `mpr_latch`.
- Each domain has its own stream (`ds_stream_init`, `poweron.c:45-50`), so dropping the `psg_noise` and `vdc_phase` domains does not shift the RAM draws. The `ram@` literals of `poweron_draw_order` stay valid; its `psg_noise` and `vdc_phase` literals go.
- Update the seven name sites in F12 and `DS_POWERON_POLICY_VERSION` per policy (`ds_core.h:24`, `text.c:388`).

### P11. `src/text/text.c`, `ds_text.h` (W1 unit; W3 `[model_*]`; W4 `[stimulus]`; W6 `[input]`; `[run]`)

**Analog:** itself. New tables copy `emit_power_on` (`src/text/text.c:373-391`) and the sorted-id union in `emit_withheld` (339-370):
```c
static void emit_machine(struct out *o, const struct ds_instance *inst)
{
    ...
    table(o, "", "machine");
    kv_u64(o, "time", inst->machine_time);
    kv_lit(o, "time_unit", "placeholder-cycles");   /* line 407 */
```
**Differs:**
- Line 407 becomes `master-clocks`.
- The record gains `[model_<name>]` (value, ledger id, status, bound_by), `[input]` (script identity sha256), `[stimulus]`, `[run]` (stop reason, overshoot, stop point), in a fixed order after `[power_on]`.
- `[withheld]` becomes the union of profile-unresolved ids, bound open fields and model rows (`emit_withheld` adds its `idset_add_list` calls).
- D-69 holds: the printer refuses a value with a quote or backslash (`put_quoted`, 78-103); printable ASCII only; the 16 KiB cap (`REC_CAP`, `runner.h:16`) still applies.
- All four pinned places in F10 change in the same plan, plus `tests/conformance/golden/accept_256k_zero.record.toml` and `accept_384k_seeded.record.toml` (`behaviour_epoch`, `policy_version`, `time_unit`, `digest`; recompute hashes with the host tool, as the golden comments say).

### P12. New runner kind files (W1 guest; W2 mutate_sweep; W3 observer, bus; W5 chunk_equiv; W6 pad; W7 snapshot)

**Analog (small kind file):** `tests/conformance/spinkinds.c` (97 lines): a header comment listing each kind and its `expect` grammar, then `void kind_x(const struct kase *k, struct ctx *c, char *got, size_t gotcap)`.
```c
/* spinkinds.c:76-81 and 87-95 */
    err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    if (err != NULL) { harness_error(c, err); return; }
    w = (struct ds_instance *)(void *)b.mem;
    ...
    check(c, status == (uint32_t)DS_RUN_BUDGET, "ds_run did not end on the exhausted budget");
    if (status == (uint32_t)DS_RUN_BUDGET && !c->failed) {
        c->timeout = 1;
        snprintf(c->why, sizeof c->why, "the machine-time budget of %llu cycles was exhausted without a stop", (unsigned long long)budget);
    }
    release_boot(&b);
```
**Analog (several instances, LCG schedule, digest compare, canaries):** `tests/conformance/ilvkinds.c:1-26` (kind list), `:38-117` (arena with guard bytes), `:380-454` (A, B and solo copies stepped on a schedule, `same_digest`, `same_record`). Use it for `chunk_equiv`, `observer` and the shared-input toy (the D-55 rewrite).
**Analog (reads an independent oracle file and the instance):** `tests/conformance/mapkinds.c:20-109` (`oracle_path`, `load_oracle`, per-bank asserts). Use it for the opcode TSV, the bank-class oracle and the DSV1 recompute from raw `inst->ram8k`.
**Analog (ISO C file read):** `read_file` (`tests/conformance/runner.c:649-676`), for loading the built ROM from `--fixture-dir`.
**Differs:**
- The `guest` kind judges DSV1 per D-124: recompute the summary from raw RAM offset `$1F00`, require equality with the guest's summary, `checks=` equal to the case count, a matching id, and no magic or done at t=0, else `harness_failure`.
- All `$50` and consistent gives `pass` (assertions = n_checks). Any `$46`, `$00` or mismatch gives `guest_failure`. `assertions=0` gives `harness_failure`.
- `UNTIL` without done gives `timeout` through `c->timeout`, as above.
- Nothing reads through the bus except the core itself: kinds read `inst->ram8k` (D-108), never a `ds_peek`.
- `check()` increments the assertion count (`runner.h:80`); a pass needs at least one assertion (`outcome_of`, `runner.c:1526-1538`).

### P13. Kind registration (every wave)

Three one-line edits per kind, all in files that already exist:
1. Declare the kind in `tests/conformance/runner.h` next to its family (`:176` is `kind_spin`).
2. Add a row to `kinds[]` in `tests/conformance/runner.c:1478-1524`: `{"name", kind_fn, 0},`. The last field `1` defers to pass two (only `coverage` uses it).
3. Append the new `.c` to `DS_RUNNER_SOURCES` in `CMakeLists.txt:62-65`. The toy targets (`ds_toy_NAME`) reuse that variable, so there is no second edit.
```cmake
set(DS_RUNNER_SOURCES
  tests/conformance/runner.c tests/conformance/profkinds.c ... tests/conformance/spinkinds.c tests/conformance/synth.c)
add_executable(ds_runner ${DS_RUNNER_SOURCES} ${DS_SHIM_SOURCES})
```
- New CLI inputs are parsed in `main` (`runner.c:1657-1690`) beside `--oracle`; keep the usage text (1692-1694) in step. A second kind-side global like `oracle_path` (`mapkinds.c:23`) carries each path.
- `main` is already long: put argument parsing for new inputs in a helper file, not in runner.c (F7).

### P14. Migrated kinds and cases (W1)

**Analog:** themselves. See the migration table below for exact sites.
- The rewrite pattern: replace "cycles" with absolute `until_mclk`; replace per-call chunk literals with chunk lists compared at a fixed deadline past done (D-95: `run(N)+run(M) == run(N+M)`).
- `kind_run_budget` (`tests/conformance/stepkinds.c:254-274`) becomes an absolute-deadline test with the stop callback; its stop function shape (`stop_ctx`, `stop_at`, lines 175-185) is reusable as the done-polling aid.
- `ilvkinds.c` keeps its arena and schedule helpers; only the step call (`ds_advance(...)`) and its limits (`IL_CHUNK_MOD`, `DS_ADVANCE_MAX`) change.

### P15. Cases, REQUIRED, goldens (every wave)

**Analog:** `tests/conformance/cases/poweron_draw_order.toml` (a header comment explaining how the literals were derived independently, then the ten keys), `tests/conformance/REQUIRED` (one id per line, comments allowed), `golden/accept_256k_zero.record.toml`.
```toml
id = "poweron_draw_order"
kind = "poweron"
fixture = "plain-256k"
request = "bind.powerup_policy=seeded-prng-v1;seed=42"
expect = "policy=seeded-prng-v1;seed=42;ram@0=..."
meaning = "A seed-42 boot fills RAM ... "
required = true
control = false
expect_outcome = "pass"
pins = ["LED-PROF-0002"]
```
**Differs:**
- Optional keys `until_mclk` (integer), `policies`, `checks`, `input`, `irq_stim`, `sinks`, `chunks` (F1).
- Every case needs its id in `REQUIRED` in the same change (`INV_UNLISTED_CASE`). The file name equals the id, and the id is `[a-z][a-z0-9_]*`.
- A `#` comment header on the literals states they were derived independently, never from core output (D-125, Phase 2 precedent). Each line stays under 400 chars (stage 1 rejects longer lines).
- Plan for about 50 new REQUIRED ids (CONTEXT: about 16 guest cases, `st_bypass`, `chunk_equiv_*`, `observer_*`, `snapshot_*`).

### P16. `scripts/fixture-build` (new, W1)

**Analog 1 (the used-list producer):** `scripts/conformance-run:408-416` (`pick_sha_tool`) and the loop `:463-483` that writes `NAME SIZE SHA256` lines with host tools only:
```sh
    fm_sha=$("$fm_bd/ds_runner" --emit "$fm_name" | $sha_tool | awk '{ print $1 }')
    fm_bytes=$("$fm_bd/ds_runner" --emit "$fm_name" | wc -c | tr -d ' ')
    ...
    printf '%s %s %s\n' "$fm_name" "$fm_bytes" "$fm_sha" >> "$fm_bd/fixtures.used"
```
**Analog 2 (tool presence, a missing tool is exit 2):** `scripts/preflight:21-63` (`PREFLIGHT_MISSING`, exit 2, no exit 1).
**Analog 3 (path hygiene for a rule file):** `scripts/control-mutate:101-104` (reject an absolute path, `..` and an empty component).
**Differs:**
- Assembles `fixtures/src/*.s` with `ca65 --cpu huc6280` and links with `ld65 -C fixtures/lib/hucard.cfg -D ROMBANKS=N` into `build/gate/fixtures/` (ignored by `.gitignore`).
- Every output is hashed by the host tool, never by the runner. A missing `ca65` or `ld65` is `PREFLIGHT_MISSING` (exit 2), never a skip.
- Do not use `ca65 --version` as identity (it prints V2.18 even for V2.19): identity is the encoding-matrix bytes (P19).
- The NAME constraint (F2) and the used-list ownership (F3) are decided here.
- Same exit contract as every checker: 0 clean, 1 violations, 2 harness error. Print `STATUS fixture-build ok n=N` (the marker regex is closed: Shared Conventions).

### P17. `scripts/fixture-lint` (new, W1; rules grow W2)

**Analog:** `scripts/fixture-check` (153 lines) with its stage 2 `scripts/lib/fixture-rules.awk`: two awk stages, `--root`, `--expect`, `V<TAB>CODE<TAB>line<TAB>detail` records, `rule-` tag comments, an `M` record always printed.
```sh
# scripts/fixture-check:97-100 (stage 1 with no pipeline; stage 2 must print at least one record)
awk -f "$here/lib/toml-subset.awk" "$m" > "$tmp/stage1" 2> /dev/null || : > "$tmp/stage1"
if fname="$stem" awk -f "$here/lib/fixture-rules.awk" "$tmp/stage1" > "$tmp/stage2" 2> /dev/null && [ -s "$tmp/stage2" ]; then :; else
  printf '%s\tSTAGE2_DEAD\t0\t\n' "$rel" >> "$viol0"
```
**Differs:**
- The inputs are `fixtures/src/NAME.s` and `NAME.checks`, not TOML, so stage 1 is a line reader (not `toml-subset.awk`).
- Rules per D-125: one `.checks` row per `CHECK`, no orphans, tag regex, counts equal `checks=`, `.include` only from `fixtures/lib` or `build`, mnemonics a subset of the opcode TSV, `z:`/`a:` always explicit, `; RAW:` lines counted and cited.
- `MAN:` cites must exist in `cites.txt` with the `img` flag when they back a `supported` ledger entry (ER-1).
- Codes use an `FXL_` prefix (planner discretion). Each is one self-contained statement ending in a `# rule-NAME` tag so a selftest can delete it (P22).
- Controls: mini-roots under `tests/gate/fixture-lint/<case>/` with `EXPECT`, run by `run_controls` (P21).

### P18. Opcode oracle TSV, two-way checker, bank-class oracle (W2, W3)

**Analog (independent second implementation with a rule-text guard):** `tests/conformance/mirror-oracle.awk:1-30`: the header states "shares no code and no table with the core", reads the ledger statement and refuses to print when its clauses drift (`ORACLE_RULE_TEXT_DRIFT`, exit 1).
**Analog (awk reads the `.def` rows, both-ways comparison, `--root`, `--names`):** `scripts/outcome-sync:55-100`:
```sh
awk '/^DS_OUTCOME\(/ { s = substr($0, 12); i = index(s, ")"); if (i > 0) s = substr(s, 1, i - 1); print s }' "$def" > "$tmp/names"
```
**Analog (oracle rows reach the runner as a file):** `scripts/conformance-run:181-189` writes `oracle.tsv` before the run and passes `--oracle`.
**Differs:**
- `tests/oracle/huc6280-opcodes.tsv` columns `opcode mnemonic mode bytes cycles flags cite`: a second transcription that never reads `cpu_ops.def`. Built-in cross-check: 234 defined, the complement equals the 22 opcodes.
- Header lines start with `#` (comment-tolerant reader) and carry SPDX plus the numeric-data statement (ER-9).
- Keep each file's size away from `8192*k` and `8192*k+512` (F7).
- Checker codes follow the `OUTSYNC_*` pattern (one code per rule, a `STATUS ... ok` marker).
- The bank-class oracle copies the mirror-oracle style: independent arithmetic, a rule-text guard on the LED-CPU-0011 statement.

### P19. `scripts/xasm-check` (new, W2)

**Analog:** `scripts/core-boundary-check` (297 lines): an external tool run per input with a `--cc CMD` override, one standalone compile per file, a code per failure (`CB_COMPILE`), exit 2 for a missing tool, mini-roots as controls (`tests/gate/core-boundary/<case>/`).
```sh
# scripts/core-boundary-check:196-197 (the tool invocation is one function, the command is an option)
"$cc" -std=c11 -O0 -ffreestanding -nostdinc ... -c "$1" -o "$2"
```
**Differs:**
- Assembles the TSV-derived matrix (every mnemonic x mode, neutral syntax, only headers and zero-page hints differ) with `--ca65 CMD` (default `ca65`) and compares output bytes with the TSV encodings.
- Codes: `XASM_MISMATCH`, `PREFLIGHT_MISSING`, a TSV-changed code.
- Controls from D-129: PATH without ca65 (copy the `no-such-tool-xyz` control, `scripts/check:185-190`), a shim `ca65` script that emits one mutated byte, a TSV with one byte changed.
- Odd encodings (TAM/TMA multi-bit, `#0`) go through `.byte` plus a `; RAW:` marker.
- WLA is optional verify-only: print `optional-absent` when `wla-huc6280` or `wlalink` is not on PATH and make no identity claim (D-129).

### P20. `scripts/input-check` (new, W6) and `conformance-flatten` INPUT rows

**Analog:** `scripts/conformance-flatten` (150 lines): stage 1 `toml-subset.awk`, no stdout until every input is clean, an `END` sentinel, `--expect`. Stage 2 analog: `scripts/lib/inventory-rules.awk:1-52` (per-file schema rules, `V` records, `NO_SENTINEL`).
**Differs:**
- `[script] format=1, time_unit="master_clock", events=N` and `[ev_NNNN]` tables (legal: table names are `[a-z][a-z0-9_]*`).
- `at` is unsigned decimal, at most 15 digits (awk numbers are doubles, exact below 2^53), no leading zero. `held` names in canonical bit order.
- Non-decreasing `at`; equal `at` needs a strictly increasing port. Caps: 1024 events and 64 KiB. Codes `INP_*`, about 15 negative cases, one per code (D-117).
- Flatten gains an `INPUT idx at port mask` row type. The case TSV reader refuses rows with a table column (`runner.c:761-763`), so inputs go in a separate TSV, or under a row tag the reader accepts.
- There is no per-checker `--selftest` flag in this repo. D-117's `--selftest` means a `mutation-input-*` block in `scripts/check` `selftest()` (P22).

### P21. `scripts/check`: adding a stage (every wave)

**Analog:** itself. A stage is a shell function that prints details and returns 0, 1 or 2, registered with one line. `run_stage` records seconds in `build/gate/results.tsv` and prints `TIMING` lines with `--timing` (already built).
```sh
# scripts/check:806-808 (a controls stage = one run_controls call over EXPECT-file mini-roots)
stage_fixture_negatives() {
  run_controls --ok-prefix --require-negative --need HARNESS_NO_FILE @DIR@used.txt tests/gate/fixture tests/gate/fixture sh "$here/fixture-check" --root @DIR@ --used @DIR@used.txt @EXPECT@
}
```
```sh
# scripts/check:1819-1832 (the single timing and results hook) and :1921-1923 (registration order)
  t0=$(date +%s)
  if "$fn" > "$tmp/out" 2>&1; then rc=0; else rc=$?; fi
  secs=$(( $(date +%s) - t0 ))
  ...
run_stage fixtures stage_fixtures
run_stage fixture-negatives stage_fixture_negatives
```
**Differs:**
- New stages `fixture-build` (before `fixtures`), `fixture-lint`, `xasm`, `input-check`, plus a controls stage for each (D-131).
- Static checkers go before `build` (like `inventory`, `workflow`). Stages that read build outputs go after `conformance` (like `fixtures`, `named-controls`).
- A stage may read only files that an earlier stage of the SAME run wrote (D-85). `fixtures` already `rm -f`s its input; copy that.
- `stage_preflight` (182-198) needs `ca65 ld65` in the default list (`scripts/preflight:21`) and one control: PATH without `ca65` gives exit 2 and `PREFLIGHT_MISSING ca65`.
- Time: no code change is needed to record it. Re-baseline `docs/ci-timings.md` after W1 and W8 (P28). Target 120 s warm; a stage over 20 s warns (`TIMING_WARN`, line 1831); time never fails.
- Usage text: `scripts/check:6` and the `README.md` flags list must stay in step (`scripts/docs-check:68-82`). Adding a flag edits both; adding a stage does not.

### P22. Controls: how a control registers and goes red

**Analog 1 (checker controls):** a mini-root `tests/gate/<family>/<case>/` with an `EXPECT` file of exact codes and the case input, run through `run_controls` (`scripts/check:58-175`).
- `ok-<case>` must pass without `EXPECT`. `--require-negative` requires at least one must-fail case. An empty `EXPECT` is `HARNESS_EMPTY_EXPECT` (exit 2).

**Analog 2 (rule-tag selftest, one per enforcement rule):** the tag comment `# rule-NAME` on the ONE line that emits a code, and a `selftest()` block in `scripts/check` that copies `scripts/`, deletes the tagged line and requires the control to flip (`scripts/check:1579-1597`):
```sh
    before=$(grep -c 'rule-missing-field' "$cp_dir/lib/fixture-rules.awk" || true)
    grep -v 'rule-missing-field' "$cp_dir/lib/fixture-rules.awk" > "$cp_dir/lib/fixture-rules.mutated" || true
    mv "$cp_dir/lib/fixture-rules.mutated" "$cp_dir/lib/fixture-rules.awk"
    after=$(grep -c 'rule-missing-field' "$cp_dir/lib/fixture-rules.awk" || true)
    if sh "$cp_dir/fixture-check" --root "$fxr" --used "$fxr/used.txt" --expect FIX_MISSING_FIELD > "$tmp/st.out" 2>&1; then r=0; else r=$?; fi
    if [ "$st_alive" -eq 1 ] && [ "$before" -ge 1 ] && [ "$after" -eq 0 ] && [ "$r" -eq 1 ]; then
```
**Analog 3 (toy cores):** `tests/controls/NAME/RULE` and `EXPECT`, generated at control time by `scripts/control-mutate`, built as `ds_toy_NAME` (`CMakeLists.txt:77-103`), run on the same cases TSV (`scripts/conformance-run:309-399`):
```
# tests/controls/wrong-mirror/RULE (keys: tag, file, anchor, replace; the anchor must occur exactly once or the run is exit 2)
tag=rule-wrong-mirror-384k
file=src/core/mapper.c
anchor=return b < 0x40 ? (b & 0x1F) : (0x20 + (b & 0x0F));
replace=return b & 0x3F;
# tests/controls/wrong-mirror/EXPECT: case ids that must come out guest_failure on the toy
accept_384k_banksig
```
**Differs:**
- Toys are capped at 8 in the fast gate (D-131): block progress lost on stop, trace hook mutates state, snapshot drops T flag, snapshot drops BLKT, ADC carry, unmapped value, timer reload off by one, IRQ priority swap. The shared-input toy comes on top.
- `file=` must be a source of `ds_core` (CMake aborts otherwise, `CMakeLists.txt:85-90`) and the anchor must fit on one line of live source, so write the live line with a toy in mind.
- `tests/controls/shared-state/RULE` anchors on `advance.c`, which W1 deletes: re-point it (migration table). The selftest `mutation-control-anchor` (`scripts/check:1355-1394`) breaks every anchor and requires exit 2 with `n=0` and `n=2`, so every new RULE is covered automatically.
- The shared-input toy needs a hidden static pad level. A hidden global fails `core-boundary-check`, so it exists only in the generated toy source.
- The protocol controls (pre-set done, never done, bad checksum, wrong id) are guest fixtures run under the live runner: copy the `nc_live` pair (`scripts/check:1016-1042`) with `tests/gate/wrong-literal` as the mini-root shape.

### P23. Core boundary and licence rules (every wave that adds `src/core` files)

**Analog:** `scripts/core-boundary-check:8-22` (the rules) and `:124-129` (the include rule):
```awk
  if (c == "<") {
    if (name == "stddef.h" || name == "stdint.h" || name == "stdbool.h") return ""
    return "<" name ">"
  }
  if (name ~ /^ds_[A-Za-z0-9_]+\.h$/ || name ~ /^[A-Za-z0-9_]+\.def$/) return ""
```
**Differs:**
- New core headers MUST be named `ds_*.h`. New tables MUST be `static const` (no writable symbol, no function-local static).
- Undefined symbols may only be `memcpy memmove memset memcmp` (plus `bzero` and `__chkstk_darwin` on Darwin): `scripts/core-boundary.allow`.
- A very large stack frame can pull `__chkstk_darwin`: keep big buffers in the instance. Symbols that one core object defines and another calls are allowed (internal references).
- `scripts/licence-check:106-126` already scans every `.c`, `.h`, `.def` under `src/` and `tests/` (MIT OR Apache-2.0) and EVERY file under `fixtures/` (0BSD, first five lines).
- It does NOT scan `tests/oracle/*`, `*.awk`, `*.tsv` or case TOML: extend the `find` at lines 112-120 (P27).

### P24. Authored fixture sources (`fixtures/src`, `fixtures/lib`) (W1-W6)

**No repo analog for assembler code.** Closest for the conventions:
- `tests/conformance/synth.c` and its manifest entries (`fixtures/manifest/plain_256k.toml:7-17`): a unique tag byte at the start and end of every 8 KiB bank, an exact recipe string, 0BSD notices.
- `fixtures/manifest/plain_256k.toml:1-6` for the 0BSD header and the "bytes are never committed" statement.

Rules every source and include file must follow:
- First five lines: `; SPDX-License-Identifier: 0BSD` and `; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors` (`scripts/licence-check:27-33`, enforced on all of `fixtures/`; the check is a fixed-string match, so use the file's own comment character).
- Begin with `.setcpu "huc6280"`. No HuC/PCEAS/PCE_Soundtest/emulator code, no cc65 library. `.include` only from `fixtures/lib` or `build` (D-123, D-125).
- Never a line-start `Provenance:` marker (F8).
- Lines under 400 chars. Avoid 1024-character base64 or hex runs (`rights-check` BLOB rule). Built ROMs live only under ignored `build/`.
- `.checks` and `cites.txt` rows use `MAN:<page>`, `GUEST:<model>`, `ASSUMED:<LED-id>`. DuoSpool or other-emulator output is forbidden as an expected value (D-125).

### P25. `fixtures/manifest/<id>.toml` (authored entries, W1 on)

**Analog:** `fixtures/manifest/plain_256k.toml` (all keys, one per line, 0BSD header).
```toml
id = "plain_256k"
kind = "synthetic"
source = "tests/conformance/synth.c"
recipe = "plain-256k: 262144 bytes (32 file banks); lcg32 seed=12345 ..."
toolchain = "ds_runner --emit plain-256k (tests/conformance/synth.c, ISO C11 ...), hashed by sha256sum or shasum -a 256"
size = 262144
sha256 = "cb038cda079ab0edec440521c4dc250a8a8c2af6e068470dbda7f05ff07b0287"
licence = "0BSD"
notices = "none: generated at test time ..."
redistribution = "generated-not-committed"
meaning = "..."
verdict_source = "spec-literal:LED-LOAD-0003"
```
**Differs:**
- `kind = "authored"` (already in the enum, `fixture-rules.awk:51`). `source = "fixtures/src/cpu_ldst.s"`.
- `recipe` is the exact ca65 and ld65 command line with `-D ROMBANKS=N`. `toolchain` says "cc65 V2.19 (ca65 prints V2.18; identity by encoding-matrix bytes)".
- `redistribution = "generated-not-committed"`. `size` and `sha256` are pinned from the used list.
- `verdict_source` must accept a `+`-joined list (D-59 amendment): edit the regexp at `scripts/lib/fixture-rules.awk:52` and keep `duospool-output` rejected. The control `tests/gate/fixture/bad-verdict-source` must stay red for that literal.
- The id equals the file stem and uses underscores; the used-list NAME uses hyphens (F2). The stage compares counts and names (`scripts/check:784-801`), so a manifest entry that nothing emits is a harness error.
- A whole entry is at most 16384 bytes (`scripts/fixture-check:92-93`).

### P26. Ledger entries (32 new, 3 edits) and `ledger/REQUIRED` (every wave)

**Analog:** `ledger/entries/LED-LOAD-0003.toml` and `LED-PROF-0002.toml` (all keys, quoting, one-line arrays). Required keys: `id statement profile status sources evidence disagreement pinning_test updated`. `assumed` also needs `assumption` and `revisit_phase` (integer).
```toml
id = "LED-PROF-0002"
statement = "DuoSpool models power-on as one of two policies ..."
profile = "pce-hucard-2btn"
status = "assumed"
sources = ["UNKNOWN|hardware power-on RAM contents|no source read; ..."]
evidence = []
disagreement = "none: ..."
pinning_test = "TEST:poweron_draw_order"
updated = "2026-11-10"
assumption = "..."
revisit_phase = 3
withheld = ["any claim that guests do not depend on uninitialised memory"]
origin = "02-CONTEXT D-50"
```
**Rules (`ledger/SCHEMA.md`, `scripts/lib/ledger-rules.awk:19-24, 76-90`):**
- Id `LED-(PROF|CPU|VID|AUD|IRQ|LOAD|IO|TOOL)-NNNN`, file name equals id, ASCII, at most 16384 bytes per file and 400 chars per line.
- Sources `LABEL|ref|detail` with LABEL in `FACT CODE REPORT UNKNOWN`. Evidence `E0..E4|ref`. `pinning_test` is `TEST:id`, `PENDING:<phase>` or `NONE:reason`.
- `supported` needs a qualifying E1/E2/E3 item; E1 needs a `FACT` source; `PENDING:` is forbidden; E0 and E4 never count.
- ER-2 allows exactly eight candidates (LED-CPU-0001, -0002, -0005, -0007, -0014, -0016, LED-IRQ-0002, -0004), only with an image-verified cite. Everything else is `assumed` or `open`.
- `ledger/REQUIRED` line shape is `ID 03-CONTEXT D-nnn`. The entry's `origin` must equal the text after the first space (`LED-LOAD-0003 02-CONTEXT D-44`). One line per new id.
- Pin lifecycle: `PENDING:3` until the case exists, then `TEST:<case>` with the case listing the id in `pins` (F9). Create the case before flipping the pin. `LED-IO-0001` still reads `PENDING:2`; update it when the pad case lands.
- Edits to existing entries: LED-IO-0001 (Mednafen bit 6 observation), LED-PROF-0002 (unit and MPR clauses superseded, REQUIRED line unchanged, `revisit_phase`), LED-LOAD-0005 (pointer to LED-CPU-0011/0012). `inventory-check` re-resolves pins on every gate run.

### P27. Gate scripts that gain a rule (W1-W3)

- `scripts/lib/inventory-rules.awk` (F1): extend `want` for the optional keys. Add a control `tests/gate/inventory/case-optional-key` and keep `case-missing-field` red.
- `scripts/lib/fixture-rules.awk:52` (P25) and `:36-37`, `:45` if keys are added.
- `scripts/lib/record-rules.awk:23-33, 55-75`: `keys[]` and `seq_accept` gain the new tables. `model_` tables are variable-named like `field_` tables (the `substr(t, 1, 6) == "field_"` branch at line 59): add a `model_` branch the same way.
- `scripts/licence-check:112-120`: add a `find` clause for `tests/oracle` (and `*.awk`, `*.tsv` if wanted) with an expected id per tree. `fixtures/` already passes 0BSD. Add `tests/licence/<case>` mini-roots (`ok-src` and `script-no-spdx` are the models).
- `scripts/conformance-run`: pass new runner inputs at lines 209 and 375 (F5); extend `fixtures_mode` (F3). Keep its header comment (lines 5-75) true.

### P28. CI, workflow-check, docs (W1; re-baseline W8)

**Analog (apt in a run step, already accepted by the checker):** `.github/workflows/gate.yml:67-76`:
```yaml
      - name: Second awk gate run
        run: |
          ...
          if ! command -v "$other" > /dev/null; then
            sudo apt-get update -qq
            sudo apt-get install -y -qq "$other"
          fi
```
**Differs:**
- One new step before `Gate` (line 53), for example `name: Install cc65` with `run: sudo apt-get install -y cc65` (after `apt-get update -qq`, as above).
- No new action, no secret, no `|| true`, no `${{ }}` inside `run:` (`workflow-rules.awk` rules `WF_EXPR_IN_RUN`, `WF_FORBIDDEN_KEY`). `uses:` stays exactly `actions/checkout@<40-hex>` (`check_uses`, `workflow-rules.awk:97-104`).
- Teach `scripts/workflow-check` and `scripts/lib/workflow-rules.awk` the step: a rule (new code, one tagged statement) that a step installs cc65 before `Gate`, plus a control. Model it on `rule-gate-found` (`workflow-rules.awk:112`) and `WF_NO_GATE_STEP` (line 299). Regenerate the 12 mini-root copies (F6).
- `scripts/linux-repro:60`: add `cc65` to the `apt-get install` list.
- README "Tool prerequisites" (the paragraph after the flags list): add cc65 (ca65, ld65) and the Homebrew line. `scripts/docs-check` only checks flags today: add a tool-name rule modelled on its flag loop (lines 68-82) if README claims about tools are to be enforced.
- `.gitignore:17-18`: rewrite the comment that says Phase 3 will need a `!` allow rule (D-133). The patterns `*.pce *.bin ...` stay: built ROMs live under ignored `build/`.
- `docs/ci-timings.md`: append a local baseline table in the style of the existing 47-row table (stage, seconds, verdict) after W1 and W8. A CI row from `sh scripts/ci-timings RUN_ID` comes only after the owner authorises a push (D-64, still pending).

## Placeholder-cycles to master-clocks migration (W1: delete, rewrite and regenerate in one plan)

| File | Sites at HEAD | Action |
|---|---|---|
| `src/core/advance.c` | whole file (64 lines) | delete; the run loop moves to the new run file |
| `src/core/ds_core.h` | 22, 24, 28-30, 114, 123-125, 143, 165-185 | epoch 2; remove `DS_ADVANCE_MAX`, `psg_noise`, `vdc_phase` and the placeholder `ds_advance`/`ds_run` API; comments |
| `src/core/serial.c`, `ds_serial.h` | `serial.c` 120, 143-144, 177-182; `ds_serial.h` 7, 22 | drop `PSGN`, `VDCP`; serial version 2 |
| `src/core/poweron.c`, `ds_poweron.h` | `poweron.c` 5-12, 22-27, 83-96; `ds_poweron.h` 21 | drop the `psg_noise` and `vdc_phase` domains (each domain has its own stream, so RAM draws do not move) |
| `src/text/text.c`, `ds_text.h` | `text.c` 393-396, 407; `ds_text.h` 17, 34 | unit `master-clocks`; `DS_TEXT_FORMAT_VERSION` bump |
| `tests/conformance/runner.c` | 539 (stays valid), 968-1008, 1171, 1504-1505 | pinned row count; remove the `advance` and `run_budget` rows |
| `tests/conformance/stepkinds.c` | whole file (274 lines) | rewrite for `ds_run` absolute deadlines, or delete with its cases |
| `tests/conformance/spinkinds.c` | 22-97 | `kind_spin` uses `ds_advance` (line 84) and `ds_run` (88): re-point to `ds_run` with `until_mclk` |
| `tests/conformance/powkinds.c` | 177-194, 241-243, 284, 295-296 | `psg_noise` and `vdc_phase` literals; seed-policy strings |
| `tests/conformance/ilvkinds.c` | 7, 41, 355, 435-447, 516-517, 556-562, 658-664, 741-743, 835 | every `ds_advance` and `DS_ADVANCE_MAX` site |
| `tests/conformance/digkinds.c` | 261-273 | `R_TIME`, `R_PSGN`, `R_VDCP` region reads |
| cases | `advance_rules`, `run_budget`, `run_budget_stop`, `interleave_ab`, `interleave_twins`, `interleave_aba_order`, `interleave_destroy_recreate`, `interleave_canaries`, `poweron_draw_order`, `poweron_zero`, `poweron_seed_zero_supplied`, `poweron_domain_independence`, `record_golden_*` | delete or rewrite literals; `REQUIRED` in the same change |
| goldens | `tests/conformance/golden/accept_256k_zero.record.toml`, `accept_384k_seeded.record.toml` | epoch, `time_unit`, digest; recompute hashes with the host tool |
| gate mini-roots | `tests/gate/cancel-spin` (kind `spin_forever`), `tests/gate/timeout-budget` (kind `budget_spin`, `expect = "budget=100000,step=1000"`) | re-point kinds and `expect` to `until_mclk`; `tests/gate/wrong-literal` is a `refuse` case and needs no API change |
| record-lint and determinism mini-roots | `tests/gate/record-lint/{ok-accept,missing-field-table,pointer-in-value,path-in-value,timestamp-in-value}/record.toml` (line 99-104); `tests/gate/determinism/{ok-same,differs}/{a,b}.toml` (line 104) | `time_unit` string, and the new tables once `record-rules.awk` requires them |
| toy `RULE` files | `tests/controls/shared-state/RULE` (file `src/core/advance.c`), `tests/controls/drop-region/RULE` | re-point `shared-state` to a live line of the new run file or CPU step; `drop-region` only needs its anchor line kept |
| scripts | `scripts/conformance-ingest` lines 21 and 213 | none needed (the tokens are already allowed); comment only |
| docs | `README.md` Status paragraph; `ledger/entries/LED-PROF-0002.toml` | rewrite the status; supersede the placeholder clauses |

## Shared Conventions (every Phase 3 plan must follow)

### SPDX headers and licence rules
**Source:** `scripts/licence-check:27-33, 94-126`.
- Code, scripts and tests: both lines in the first five lines: `SPDX-License-Identifier: MIT OR Apache-2.0` and `SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors`. C files use the block form `/* ... */` on lines 1-2.
- Every regular file under `scripts/` (including `.awk` and `.allow`) and every `.c .h .def` under `src/` and `tests/` is checked. `tests/licence/` is the one headerless exception.
- Everything under `fixtures/` (sources, includes, configs, `.checks`, `cites.txt`, `TOOLS.md`, manifests) carries `SPDX-License-Identifier: 0BSD` plus the holder line.
- Data records (`ledger/entries`, cases, goldens, `tests/controls`) also use 0BSD by convention but are not scanned.
- A copied helper needs a `PROVENANCE.md` entry and a marker (record 0007). Cite emulator behaviour in ledger sources only; never copy emulator code (ER-9, D-130). cc65 is Zlib (on the allowlist) and is an external tool.
- The tool record `fixtures/lib/TOOLS.md` is a routine agent check with a 0BSD header (D-128).

### X-macro `.def` style
**Source:** `src/core/refusals.def`, `profile_fields.def`, `boot_order.def`, `tests/conformance/outcomes.def`.
- SPDX block comment, then a header comment defining every column and the contract ("order is behaviour", "numbers never reused"), then one row per line with no continuation.
- The consumer pattern is `#define DS_X(...) ... / #include "x.def" / #undef DS_X`, once per generated table, all `static const`.
- Enum slot counts have a compile-time ceiling (`DS_REFUSAL_SLOTS`, `ds_core.h:39`): add the `_Static_assert` when a new def has a ceiling.
- sh and awk readers take row names straight from the def (`scripts/outcome-sync:67-68`). Never duplicate a list in a script.

### Error and refusal style
- Typed numeric refusals in a closed set with stable numbers (`0` is OK). The first failing check wins in a fixed order (`boot_order.def`).
- A refusal writes nothing but the result struct and leaves the instance byte-identical (the D-43 digest-before-and-after test).
- Every check function guards its own use of the request so a swapped row never dereferences NULL (`boot.c:51-58` comment and `req_missing`).
- Runner messages come from `check(c, cond, "what failed")` and `harness_error(c, why)`, never from `printf`. Output goes through `emit_out`/`emit_err` (`runner.h:127-128`); stdout carries only `RESULT` and `RUNNER_DONE`.

### Runner kind registration
See P13. Three edits: a `runner.h` declaration, one `kinds[]` row, `DS_RUNNER_SOURCES`. A kind prints through `ctx`, sets `got` for the evidence file, releases what it booted, and never aggregates (`runner.c:1540-1592`).

### The runner.c size hazard
See F7. runner.c is 65876 bytes: do not grow it. Put new kind code, CLI parsing and request keys in new files.
- Avoid any tracked blob whose size is a multiple of 8192 or 8192+512 (>= 4096) in any commit, text included. Check with `wc -c`.
- The shape rule of `rights-check` (D-76) exempts valid text; `audit-history` (`ROM-SIZE-8K`/`ROM-SIZE-HDR`) does not.

### REQUIRED and EXCLUDED_IN_MODE inventory handling
- `tests/conformance/REQUIRED`: one id per line (comments allowed). An id with no RESULT line is `missing_artifact`. An id with no case file is `INV_REQUIRED_MISSING`. A case with `required = true` that is not listed is `INV_UNLISTED_CASE`. An emptied list is `INV_REQUIRED_EMPTY`. Add the id and the case file in the same change.
- `tests/conformance/EXCLUDED_IN_MODE` rows are `MODE ID` (today only `sanitize alloc_*`). Phase 3 adds no excluded case (D-78): the sanitizer stage runs every fixture under ASan+UBSan.
- A case that cannot run in a mode needs a listed row with its reason comment, must be REQUIRED, and must pass in the gate mode. An exclusion is never a `skip`.
- RESULT grammar: `RESULT id outcome assertions=N evidence=REL [machine_time=N] [stop=REASON]`, then `RUNNER_DONE n` (`scripts/conformance-ingest:20-23`). Only `pass` is green. A pass with `assertions=0` is `ING_ASSERTIONS_ZERO`.

### How controls register
See P22.
- Checker controls: a mini-root plus `EXPECT` plus a `run_controls` call.
- Enforcement rules: a `# rule-NAME` tag on one line and a `mutation-NAME` block in `selftest()`.
- Toy cores: `tests/controls/NAME/RULE` and `EXPECT`.
- Live-runner controls: a mini-root with `tests/conformance/{REQUIRED,cases}` and an `EXPECT` of `ID OUTCOME` pairs, run by `conformance-run control` and matched on RESULT fields 1-3 only (D-80).

### How a new gate stage is added and timed
See P21.
- One function, one `run_stage NAME FUNCTION` line in dependency order. `--stage NAME` already works; `--timing` and `build/gate/results.tsv` are automatic.
- Every checker prints a marker (`EXPECT_OK`, `STATUS ...` or `RECORDS ...`). The wrapper `checker()` (`scripts/check:32-40`) turns a silent exit 0 into `CHECKER_DEAD` (exit 2).
- Exit contract everywhere: 0 pass, 1 violation or broken control, 2 harness error (missing tool, dead stage, empty `--expect`).

### Ledger entry schema and pin rules
See P26: keys, status rules, the ER-2 candidate limit, the `REQUIRED` line shape, one TEST pin per entry, the pin back-link, `PENDING:3` until the case exists.

### Manifest entry schema
See P25 and `scripts/lib/fixture-rules.awk:15-40`.
- 12 required keys: `id kind source recipe toolchain size sha256 licence notices redistribution meaning verdict_source`.
- Kind `synthetic|authored|exception`. Redistribution `generated-not-committed|committed`. Licence in the 0007 allowlist (`0BSD MIT Apache-2.0 BSD-2-Clause BSD-3-Clause ISC Zlib CC0-1.0`).
- sha256 is 64 lowercase hex. `source` is relative. `verdict_source` is never `duospool-output`.

### rights-check shape rule
**Source:** `scripts/rights-check:10-18`, D-56, D-76.
- A file that is not valid text and has size >= 4096 with `size % 8192 == 0` or `== 512` is `ROMLIKE_BINARY`.
- Valid text is exempt from that one rule but is still subject to `BLOB` (a run of 1024+ base64 or hex characters), `LINELEN` (4096), the extension list and the manifest allowlist.
- Only an entry with `redistribution = committed` can excuse a hit, and only while its path and hash check out. `generated-not-committed` excuses nothing, so built ROMs must never be tracked or untracked-not-ignored.
- Run-time controls plant files in a throwaway repo (`tests/lib/gitfix.sh`, `tests/rights/synthetic.sh`), never in the tree.

### core-boundary-check allow rules
See P23. `scripts/core-boundary.allow` is the whole list of undefined symbols; adding a name needs a one-line justification comment. `STATUS core-boundary ok` is printed only on a clean run.

### Other conventions that bite
- **POSIX sh and awk only.** No `[[ ]]`, arrays, `local`, `echo -e`, `grep -P`, `case` inside `$( )` (bash 3.2 on macOS, dash on Ubuntu).
  awk: no regex intervals `{n}`, no gawk extensions, no pipeline whose left side is awk (CR-01: write a file, test the awk status, then sort). Pass data to awk through `ENVIRON` or `-v`.
- **Strict TOML subset only** (`scripts/lib/toml-subset.awk`): ASCII, 400 chars per line, no escapes, no floats, tables `[a-z][a-z0-9_]*`, no TAB inside a value, arrays of strings on one line. No second parser (D-51, D-117).
- **Every checker** takes `--expect CODE[,CODE]` and `--root DIR`, rejects an empty `--expect` with exit 2 (`HARNESS_EMPTY_EXPECT`), prints `EXPECT_OK` on a met expectation, and sorts and dedupes violations as `FILE<TAB>CODE<TAB>LINE<TAB>DETAIL` (`scripts/licence-check:13-14, 128-147`).
- **Plan sizing (D-81):** a plan modifies fewer than 15 files and a task names fewer than 10 items. Verify commands run at stage granularity (`sh scripts/check --stage NAME`); the wave end runs the whole gate.
- **Evidence wording (ER-6):** never "accurate", "cycle-accurate", "compatible", "boots" or "passes the CPU tests" without a class.
  Use: "executes the authored fixture suite; E1 documentary for the entries listed as supported; every other claim is a labelled modelling assumption".

## No Analog Found

Files with no close match in the repo. The planner should use CONTEXT/RESEARCH decisions and the nearest convention named above.

| File | Role | Data flow | Reason / where to look |
|---|---|---|---|
| `fixtures/src/*.s`, `fixtures/lib/startup.s`, `verdict.inc`, `check.inc` | guest assembler | file-I/O | no assembler source exists; conventions only (P24); D-123, D-124 (DSV1 layout), D-126 (literal table at a fixed ROM offset) |
| `fixtures/lib/hucard.cfg` (ld65 config) | build config | file-I/O | no linker config; ROMBANKS via `-D`, tag banks, `$FF` fill (D-123); take memory-map facts from the ledger, not from cc65's stock `pce.cfg` |
| `src/core/cpu*.c` instruction semantics, block-transfer state | core | transform | no CPU code; follow the manual-derived `cpu_ops.def` and D-98..D-101; style from P1-P3 |
| `src/core/timer.c`, `irq.c` closed-form model | core | event-driven | no scheduler exists; D-109..D-111; unit test with a naive per-clock reference model (not shipped) |
| in-guest random-program generator and the 32-program differential | test | batch | none; build on the `ilvkinds.c` LCG schedule helpers (`schedule_make`) and the chunk list of D-95 |
| hostile snapshot corpus (about 500 generated cases) | test | transform | none; generate in-process (D-133), bounded, accepted inputs re-serialised identically |

## Metadata

**Analog search scope:** `src/core`, `src/text`, `tests/conformance`, `tests/controls`, `tests/gate`, `tests/workflow`, `tests/licence`, `tests/rights`, `tests/lib`, `scripts`, `scripts/lib`, `fixtures/manifest`, `ledger`, `.github`, `CMakeLists.txt`, `CMakePresets.json`, `docs/ci-timings.md`, `README.md`, `.gitignore`.
**Files read:** about 60 (scripts/check, scripts/conformance-run and tests/conformance/runner.c in non-overlapping ranges).
**Tracked-source gate:** every cited path verified with `git ls-files`; no mirror path appears.
**Pattern extraction date:** 2026-11-10
