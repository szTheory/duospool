/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_core.h - the private header of the freestanding core (D-40; ds_profile.h is folded in). It is reached only through the
 * internal include directory src/core: there is no public duospool.h and no install rule (D-05).
 * The core never allocates and never opens a file (D-42): the instance lives in caller memory, and the ROM bytes are copied into
 * a fixed 1 MiB array inside it. Only stddef.h, stdint.h and stdbool.h may ever be included here (D-54).
 */
#ifndef DS_CORE_H
#define DS_CORE_H

#include <stddef.h>
#include <stdint.h>

/* the allocation cap is structural: the ROM array of the instance is exactly this big (D-42, D-46) */
#define DS_ROM_CAP 1048576u
#define DS_PROFILE_ID_MAX 32u
#define DS_FIELD_VALUE_MAX 30u
#define DS_MAPPER_NAME_MAX 16u
#define DS_CONTENT_ID_BYTES 32u
/* behaviour epoch (D-53): bumped by any change that can alter what a run observes; goldens compare it, never a build id */
#define DS_BEHAVIOUR_EPOCH 2u
/* version of the power-on policy table (D-50): the record prints it next to the policy name; both policies are version 1 */
#define DS_POWERON_POLICY_VERSION 1u
/* field_index of a result that names no profile field */
#define DS_NO_FIELD 0xffffffffu
/* power-on state (D-50): the 8 KiB work RAM and the 8 mapping registers */
#define DS_RAM8K_BYTES 8192u
#define DS_MPR_COUNT 8u

/* ---- refusals (refusals.def) ---- */

enum ds_refusal {
    DS_OK = 0,
#define DS_REFUSAL(name, number) DS_REFUSE_##name = number,
#include "refusals.def"
#undef DS_REFUSAL
    DS_REFUSAL_SLOTS = 32 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "load.NAME" for a refusal number, "ok" for 0, NULL for a number that is not a member */
const char *ds_refusal_name(uint32_t number);

/* ---- profile (profile_fields.def) ---- */

enum ds_field_index {
#define DS_FIELD(name, kind, value, allowed, ledger) DS_FIELD_##name,
#include "profile_fields.def"
#undef DS_FIELD
    DS_FIELD_COUNT
};

enum ds_field_status {
    DS_STATUS_OWNER = 1, /* owner-selected constant */
    DS_STATUS_OPEN = 2   /* open: no value until a run request binds it */
};

enum ds_bound_by {
    DS_BOUND_NONE = 0,       /* an open field that nothing has bound */
    DS_BOUND_CONSTANT = 1,   /* an owner-selected constant of the profile */
    DS_BOUND_RUN_REQUEST = 2 /* an open field bound by the run request: no support claim (D-49) */
};

/* initial bound_by of a field row, selected by its kind column */
enum { DS_BOUNDINIT_OWNER = DS_BOUND_CONSTANT, DS_BOUNDINIT_OPEN = DS_BOUND_NONE };

struct ds_field {
    uint8_t status;
    uint8_t bound_by;
    char value[DS_FIELD_VALUE_MAX];
};

struct ds_profile {
    char id[DS_PROFILE_ID_MAX];
    uint32_t rev;
    struct ds_field field[DS_FIELD_COUNT];
};

/* the profile has no padding, so its size is the sum of its parts */
_Static_assert(sizeof(struct ds_field) == 2u + DS_FIELD_VALUE_MAX, "ds_field has padding");
_Static_assert(sizeof(struct ds_profile) == DS_PROFILE_ID_MAX + sizeof(uint32_t) + DS_FIELD_COUNT * sizeof(struct ds_field),
               "ds_profile has padding");

/* one value per field, NULL = unbound; a binding is text and is checked against the field's allowed set */
struct ds_bindings {
    const char *value[DS_FIELD_COUNT];
};

const char *ds_field_name(uint32_t field);    /* NULL for an index at or above DS_FIELD_COUNT */
const char *ds_field_allowed(uint32_t field); /* values separated by '|', empty when the profile lists none */
const char *ds_field_ledger(uint32_t field);  /* ledger ids separated by '|', empty when none; NULL for an index at or above DS_FIELD_COUNT */

/* the registered r1 constant for id and rev, NULL when either does not match */
const struct ds_profile *ds_profile_builtin(const char *id, uint32_t rev);

/*
 * The unresolved behaviours of the built-in profile: ledger ids, in sorted order, the root key unresolved_behaviours of r1.toml.
 * They are part of the profile identity text of a later plan; label text never is.
 */
uint32_t ds_profile_unresolved_count(void);
const char *ds_profile_unresolved(uint32_t index); /* NULL for an index at or above the count */

/*
 * Resolve profile id and rev against the bindings. Returns DS_OK and fills out (when not NULL), or a refusal; field_index (when
 * not NULL) is the offending field or DS_NO_FIELD. Nothing but field_index is written on a refusal.
 */
uint32_t ds_resolve(const char *id, uint32_t rev, const struct ds_bindings *bindings, struct ds_profile *out,
                    uint32_t *field_index);

/* ---- instance and boot ---- */

struct ds_instance {
    uint64_t machine_time; /* master clocks of OSC1, 0 at boot, advanced only by ds_run (D-90) */
    struct ds_profile profile; /* the resolved profile of this run */
    uint8_t content_id[DS_CONTENT_ID_BYTES];
    char mapper[DS_MAPPER_NAME_MAX];
    uint32_t size_class; /* enum ds_size of ds_mapper.h: the image size is kept as an enum, never as a raw length */
    uint32_t byte_order; /* 1 pce, 2 tg16 */
    uint32_t header;     /* 1 none, 2 copier_512 */
    /* power-on state (D-50, D-75, D-91): the work RAM and the mapping registers; none of it is a hardware claim (LED-PROF-0002) */
    uint8_t ram8k[DS_RAM8K_BYTES];
    uint8_t mpr[DS_MPR_COUNT]; /* a documented all-zero constant, an open ledger item for Phase 3 */
    uint32_t powerup_policy;   /* enum ds_policy of ds_poweron.h: 1 zero, 2 seeded-prng-v1 */
    uint32_t has_seed;         /* 1 when the request carried a seed */
    uint64_t powerup_seed;     /* the seed of the run, 0 when has_seed is 0 */
    /* HuC6280 state (plan 03-01, D-102): eight bytes, no padding, no pointers, no caches. p holds N V T D I Z C in their own bit positions; B is never stored */
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t p;
    uint8_t speed; /* 0 low (the power-up speed), 1 high (D-90, D-114) */
    /* bus state (plan 03-16, region BUS0 of the digest; D-104, D-105): io_buffer is the last byte that crossed the data bus in a read or write of the windows $0800-$17FF of the hardware page, mpr_latch the byte of TAM and TMA (plan 03-17); both are constant 0 at power-on under every policy */
    uint8_t io_buffer;
    uint8_t mpr_latch;
    uint8_t rom[DS_ROM_CAP];
};

/* the CPU block and the bus block have no padding: pc follows the power-up seed, io_buffer the speed bit, mpr_latch io_buffer, and the ROM array follows mpr_latch */
_Static_assert(offsetof(struct ds_instance, pc) == offsetof(struct ds_instance, powerup_seed) + sizeof(uint64_t), "padding before the CPU state");
_Static_assert(offsetof(struct ds_instance, io_buffer) == offsetof(struct ds_instance, speed) + 1u, "padding between the CPU state and io_buffer");
_Static_assert(offsetof(struct ds_instance, mpr_latch) == offsetof(struct ds_instance, io_buffer) + 1u, "padding between io_buffer and mpr_latch");
_Static_assert(offsetof(struct ds_instance, rom) == offsetof(struct ds_instance, mpr_latch) + 1u, "padding after the bus state");

struct ds_boot_request {
    uint32_t struct_size; /* sizeof(struct ds_boot_request) */
    uint32_t reserved;    /* must be 0 */
    const char *profile_id;
    uint32_t profile_rev;
    struct ds_bindings bindings;
    const char *mapper;  /* NULL = undeclared */
    uint32_t byte_order; /* 0 undeclared, 1 pce, 2 tg16 */
    uint32_t header;     /* 0 undeclared, 1 none, 2 copier_512 */
    const uint8_t *content;
    size_t len;
    uint64_t powerup_seed; /* read only when has_seed is nonzero: required by seeded-prng-v1, refused under zero (D-50, D-79(b)) */
    uint32_t has_seed;
};

struct ds_boot_result {
    uint32_t reason;      /* a refusal number, 0 = OK */
    uint32_t field_index; /* the offending profile field or DS_NO_FIELD */
    struct ds_instance *instance;
};

size_t ds_instance_size(void);
size_t ds_instance_align(void);

/*
 * Boot a machine in caller memory (mem, cap) from a request, or refuse. The return value is the refusal number and is also
 * written to result (when result is not NULL). A refusal writes nothing but result: never mem. There is no instance to mutate
 * and no allocator, so there is no "created but unloaded" state. The block (mem, ds_instance_size() bytes) must not overlap the
 * request struct, the content bytes or the result struct: when every other check has passed and one does, the boot is refused as
 * load.memory_invalid (WR-02), because the instance is zeroed before the request and the content are read.
 */
uint32_t ds_boot(void *mem, size_t cap, const struct ds_boot_request *request, struct ds_boot_result *result);

/* ---- machine time, CPU and bounded run (cpu.c, run.c; plan 03-01, D-90..D-95) ---- */

/* one CPU cycle is 12 master clocks at the power-up low speed and 3 at high speed; the interrupt entry is 8 cycles (assumed, D-90, D-98, D-99) */
#define DS_MCLK_PER_CYCLE_LOW 12u
#define DS_MCLK_PER_CYCLE_HIGH 3u
#define DS_INT_ENTRY_CYCLES 8u
/* the named cycle adders of D-98 and D-107, applied on top of the opcode table's base counts (ds_max_step_mclk adds them to the table maximum) */
#define DS_ADDER_BRANCH_TAKEN 2u /* a taken conditional branch, BBR and BBS included (S8-20); BRA and BSR are fixed counts in the table */
#define DS_ADDER_T_MODE 3u       /* ADC, AND, EOR, ORA with the T flag set (S8-17) */
#define DS_ADDER_DECIMAL 1u      /* ADC and SBC with the D flag set (S8-17) */
#define DS_ADDER_VDC_ACCESS 1u   /* one CPU cycle, at the speed the instruction started at, for every bus access to physical $1FE000-$1FE7FF (the VDC and VCE windows), zero-page-mapped, block and ST accesses included; charged in the bus before the device sees the access (D-107, ASSUMED LED-CPU-0017: the scope and the order are modelling assumptions) */
/* the value an unmapped read returns (D-104) */
#define DS_UNMAPPED_READ 0xFFu
/* the stub reads of the hardware-page windows whose device does not exist yet (D-104, ASSUMED LED-CPU-0012): the VDC window until Phase 4 and the VCE window until Phase 4 */
#define DS_VDC_STUB_READ 0x00u
#define DS_VCE_STUB_READ 0xFFu

typedef int (*ds_stop_fn)(const struct ds_instance *inst, void *ctx);

enum ds_run_reason { DS_RUN_REASON_UNTIL = 1, DS_RUN_REASON_CALLBACK = 2, DS_RUN_REASON_REFUSED = 3 };
enum ds_stop_point { DS_STOP_INSTRUCTION = 1, DS_STOP_BLOCK_BYTE = 2 /* reserved for plan W5: nothing returns it yet */ };

/* the refusal codes of a run request (run_refusals.def, plan 03-35; F4): their own set and prefix, never part of refusals.def */
enum ds_run_refusal {
    DS_RUNREF_NONE = 0,
#define DS_RUNREF(name, number) DS_RUNREF_##name = number,
#include "run_refusals.def"
#undef DS_RUNREF
    DS_RUNREF_SLOTS = 16 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "run.NAME" for a run refusal number, "ok" for DS_RUNREF_NONE, NULL for a number that is not a member */
const char *ds_run_refusal_name(uint32_t number);

/* the instruction sink of ds_obs.h (plan 03-10); the device-register sink is not built yet (plan 03-19) and its pointer stays untyped */
struct ds_insn_sink;

/* the run request: an absolute deadline in master clocks, never a budget (D-93); insn is the caller-owned instruction sink (NULL = none, ds_obs.h), dev the device-register sink, which must be NULL until plan 03-19 */
struct ds_run_request {
    uint32_t struct_size; /* sizeof(struct ds_run_request) */
    uint32_t reserved;    /* must be 0 */
    uint64_t until_mclk;
    ds_stop_fn stop; /* NULL never stops; checked before the first step and after every step */
    void *stop_ctx;
    struct ds_insn_sink *insn;
    void *dev;
};

struct ds_run_result {
    uint32_t reason;     /* enum ds_run_reason */
    uint32_t refusal;    /* enum ds_run_refusal, DS_RUNREF_NONE unless the reason is REFUSED */
    uint32_t stop_point; /* enum ds_stop_point */
    uint32_t pad;        /* always 0 */
    uint64_t machine_time;
    uint64_t overshoot; /* machine_time - until_mclk when this call took a step that ended past the deadline, else 0 (a call with 0 steps has none) */
    uint64_t steps;
};

/*
 * Run the machine in whole instructions until machine time reaches until_mclk (UNTIL) or the stop callback holds (CALLBACK). A deadline at or
 * below the current machine time returns UNTIL with zero steps. Stops only fall between steps, so the end time can pass the deadline by less
 * than ds_max_step_mclk(). No state lives outside the instance between steps: run(N) then run(M) equals run(M) from the start (D-92).
 * A NULL instance or request, a wrong struct_size, a nonzero reserved, a non-NULL dev sink, an insn sink that overlaps the call's own memory or is misaligned (memory_invalid) and a deadline that would wrap machine time are
 * refused with REFUSED and a refusal code; the instance is then unchanged. Returns the reason, also written to result when it is not NULL.
 */
uint32_t ds_run(struct ds_instance *inst, const struct ds_run_request *request, struct ds_run_result *result);

/* the longest single step in master clocks: the longest opcode of the table with the adders that can apply to it, plus the interrupt entry, at the low-speed cycle length, scanned from the table, never a guess (D-94) */
uint64_t ds_max_step_mclk(void);

/* the CPU core (cpu.c): reset through the vector at logical $FFFE/$FFFF with MPR7 = $00 at machine_time 0, and one instruction per step */
void ds_cpu_reset(struct ds_instance *inst);
uint32_t ds_cpu_step(struct ds_instance *inst); /* executes one instruction and returns the master clocks it took (ds_cpu_step_obs of ds_obs.h adds the observer hook) */
int ds_cpu_op_defined(uint32_t opcode); /* 1 when the opcode row of the table is a defined instruction, 0 for the 22 undefined rows (opcode is masked to 8 bits); the table is checked against the manual transcription by scripts/opcode-check */
uint32_t ds_cpu_op_max_cycles(uint32_t opcode); /* the longest one opcode can take in CPU cycles: its table count plus every adder that can apply, the VDC and VCE access penalty included (a bus access occupies at least one cycle, so an instruction makes at most as many accesses as its table count); opcode is masked to 8 bits */
void ds_op_meta(uint32_t opcode, uint32_t *bytes, uint32_t *cycles); /* the table's instruction length and base cycle count of one opcode (a block transfer holds its 17; an undefined row holds its assumed 1 and 2, LED-CPU-0010), for the manual comparison of the opcode_table kind (plan 03-15); opcode is masked to 8 bits */

/*
 * The bus choke point (bus.c, plan 03-16; D-102 to D-108): every CPU access translates through the MPRs per access, phys = (mpr[a >> 13] << 13) | (a & $1FFF), and dispatches through a const bank_class[256]
 * table. One struct ds_bus lives in a local of the step and carries what one instruction's accesses add to it (the VDC and VCE penalty cycles); a NULL bus is accepted (the reset reads, a test).
 * Reads change state (io_buffer, D-104), so the instance is not const here, and no observer ever reads through the bus (D-108).
 */
struct ds_bus {
    uint32_t penalty; /* CPU cycles added by accesses to physical $1FE000-$1FE7FF during this step (D-107): DS_ADDER_VDC_ACCESS per access */
};

uint8_t ds_bus_read(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr);
void ds_bus_write(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr, uint8_t value);
void ds_mpr_write(struct ds_instance *inst, uint32_t index, uint8_t value); /* the only writer of mpr[] */
void ds_bus_write_phys(struct ds_instance *inst, struct ds_bus *bus, uint32_t phys, uint8_t value); /* the physical write of ST0 to ST2 (D-106): $1FE000 | n for n in 0, 2, 3, the MPRs are never consulted; any other address is dropped */

/* NUL-terminated string equality without string.h; both arguments must be non-NULL */
static inline int ds_streq(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

#endif
