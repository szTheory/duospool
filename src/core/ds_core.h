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
#define DS_BEHAVIOUR_EPOCH 1u
/* version of the power-on policy table (D-50): the record prints it next to the policy name; both policies are version 1 */
#define DS_POWERON_POLICY_VERSION 1u
/* field_index of a result that names no profile field */
#define DS_NO_FIELD 0xffffffffu
/* power-on state (D-50): the 8 KiB work RAM, the 8 mapping registers, and the per-call limit of the placeholder stepping (D-68) */
#define DS_RAM8K_BYTES 8192u
#define DS_MPR_COUNT 8u
#define DS_ADVANCE_MAX 1048576u

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
    uint64_t machine_time; /* master clocks of OSC1 under ds_run (D-90); the placeholder ds_advance still adds placeholder cycles until plan 03-02 */
    struct ds_profile profile; /* the resolved profile of this run */
    uint8_t content_id[DS_CONTENT_ID_BYTES];
    char mapper[DS_MAPPER_NAME_MAX];
    uint32_t size_class; /* enum ds_size of ds_mapper.h: the image size is kept as an enum, never as a raw length */
    uint32_t byte_order; /* 1 pce, 2 tg16 */
    uint32_t header;     /* 1 none, 2 copier_512 */
    /* power-on state (D-50, D-75) and the placeholder state that ds_advance evolves (D-68); none of it is a hardware claim (LED-PROF-0002) */
    uint8_t ram8k[DS_RAM8K_BYTES];
    uint8_t mpr[DS_MPR_COUNT]; /* a documented all-zero constant, an open ledger item for Phase 3 */
    uint32_t psg_noise;        /* a fixed 32-bit xorshift placeholder register, never zero */
    uint32_t vdc_phase;        /* a wrapping counter placeholder */
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
    uint8_t rom[DS_ROM_CAP];
};

/* the CPU block has no padding on either side: pc follows the power-up seed and the ROM array follows the speed bit */
_Static_assert(offsetof(struct ds_instance, pc) == offsetof(struct ds_instance, powerup_seed) + sizeof(uint64_t), "padding before the CPU state");
_Static_assert(offsetof(struct ds_instance, rom) == offsetof(struct ds_instance, speed) + 1u, "padding after the CPU state");

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
/* the value an unmapped read returns (D-104) */
#define DS_UNMAPPED_READ 0xFFu

typedef int (*ds_stop_fn)(const struct ds_instance *inst, void *ctx);

enum ds_run_reason { DS_RUN_REASON_UNTIL = 1, DS_RUN_REASON_CALLBACK = 2, DS_RUN_REASON_REFUSED = 3 };
enum ds_stop_point { DS_STOP_INSTRUCTION = 1, DS_STOP_BLOCK_BYTE = 2 /* reserved for plan W5: nothing returns it yet */ };

/* the refusal codes of a run request; plan 03-35 moves them into their own .def (they are not load refusals) */
enum ds_run_refusal {
    DS_RUNREF_NONE = 0,
    DS_RUNREF_instance_null = 1,
    DS_RUNREF_request_null = 2,
    DS_RUNREF_struct_size = 3,
    DS_RUNREF_reserved = 4,
    DS_RUNREF_sink = 5,
    DS_RUNREF_deadline = 6
};

/* the run request: an absolute deadline in master clocks, never a budget (D-93); insn and dev are the observer sinks and must be NULL until the sink plans */
struct ds_run_request {
    uint32_t struct_size; /* sizeof(struct ds_run_request) */
    uint32_t reserved;    /* must be 0 */
    uint64_t until_mclk;
    ds_stop_fn stop; /* NULL never stops; checked before the first step and after every step */
    void *stop_ctx;
    void *insn;
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
 * A NULL instance or request, a wrong struct_size, a nonzero reserved, a non-NULL sink and a deadline that would wrap machine time are
 * refused with REFUSED and a refusal code; the instance is then unchanged. Returns the reason, also written to result when it is not NULL.
 */
uint32_t ds_run(struct ds_instance *inst, const struct ds_run_request *request, struct ds_run_result *result);

/* the longest single step in master clocks: the longest instruction of the implemented opcode table plus the interrupt entry, at low speed (D-94) */
uint64_t ds_max_step_mclk(void);

/* the CPU core (cpu.c): reset through the vector at logical $FFFE/$FFFF with MPR7 = $00 at machine_time 0, and one instruction per step */
void ds_cpu_reset(struct ds_instance *inst);
uint32_t ds_cpu_step(struct ds_instance *inst); /* executes one instruction and returns the master clocks it took */
uint32_t ds_cpu_max_cycles(void);               /* the longest instruction of the opcode table in CPU cycles, taken-branch adder included */

/* the bus choke point (cpu.c until plan 03-16 moves it into bus.c): every CPU access translates through the MPRs (D-102) */
uint8_t ds_bus_read(const struct ds_instance *inst, uint16_t addr);
void ds_bus_write(struct ds_instance *inst, uint16_t addr, uint8_t value);
void ds_mpr_write(struct ds_instance *inst, uint32_t index, uint8_t value); /* the only writer of mpr[] */

/* ---- placeholder stepping (advance.c, D-68, D-75): fixed rules, labelled placeholder, no hardware claim (LED-PROF-0002) ---- */

enum ds_advance_status { DS_ADVANCE_OK = 0, DS_ADVANCE_REFUSED = 1 };

/*
 * Add cycles to the machine time, step the psg_noise register once per cycle and add the cycles to vdc_phase (wrapping). A call of
 * more than DS_ADVANCE_MAX cycles, or with a NULL instance, is refused and changes nothing.
 */
uint32_t ds_advance(struct ds_instance *inst, uint64_t cycles);

enum ds_run_status { DS_RUN_STOPPED = 1, DS_RUN_BUDGET = 2, DS_RUN_REFUSED = 3 };

/*
 * The expand half of the run-API migration (D-91): the Phase 2 budgeted run keeps its behaviour under this name until plan 03-02, which
 * deletes it together with ds_advance. Run in steps of at most step cycles until stop returns nonzero (checked before the first step and
 * after every step; NULL never stops) or budget cycles have been consumed. Machine time never exceeds its start plus the budget, and a
 * stop that holds exactly when the budget is consumed is a stop. Returns DS_RUN_STOPPED, DS_RUN_BUDGET (what the runner reports as
 * timeout, D-57), or DS_RUN_REFUSED for a NULL instance or a step of 0 or above DS_ADVANCE_MAX.
 */
uint32_t ds_run_placeholder(struct ds_instance *inst, uint64_t budget, uint64_t step, ds_stop_fn stop, void *ctx);

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
