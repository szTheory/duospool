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
/* field_index of a result that names no profile field */
#define DS_NO_FIELD 0xffffffffu

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
#define DS_FIELD(name, kind, value, allowed) DS_FIELD_##name,
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
    uint64_t machine_time;
    struct ds_profile profile; /* the resolved profile of this run */
    uint8_t content_id[DS_CONTENT_ID_BYTES];
    char mapper[DS_MAPPER_NAME_MAX];
    uint32_t size_class; /* enum ds_size of ds_mapper.h: the image size is kept as an enum, never as a raw length */
    uint32_t byte_order; /* 1 pce, 2 tg16 */
    uint32_t header;     /* 1 none, 2 copier_512 */
    uint8_t rom[DS_ROM_CAP];
};

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
 * and no allocator, so there is no "created but unloaded" state.
 */
uint32_t ds_boot(void *mem, size_t cap, const struct ds_boot_request *request, struct ds_boot_result *result);

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
