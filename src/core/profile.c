/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * profile.c - the built-in r1 profile constant and ds_resolve (D-48, D-49).
 * The constant is generated from profile_fields.def: owner-selected fields carry their value, open fields are OPEN with an
 * empty value. Nothing here is writable and nothing is allocated.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_FIELD(name, kind, value, allowed) {DS_STATUS_##kind, DS_BOUNDINIT_##kind, value},
static const struct ds_profile ds_profile_r1 = {"pce-hucard-2btn",
                                                1u,
                                                {
#include "profile_fields.def"
                                                }};
#undef DS_FIELD

#define DS_FIELD(name, kind, value, allowed) #name,
static const char *const ds_field_names[DS_FIELD_COUNT] = {
#include "profile_fields.def"
};
#undef DS_FIELD

#define DS_FIELD(name, kind, value, allowed) allowed,
static const char *const ds_field_alloweds[DS_FIELD_COUNT] = {
#include "profile_fields.def"
};
#undef DS_FIELD

const char *ds_field_name(uint32_t field)
{
    return field < (uint32_t)DS_FIELD_COUNT ? ds_field_names[field] : NULL;
}

const char *ds_field_allowed(uint32_t field)
{
    return field < (uint32_t)DS_FIELD_COUNT ? ds_field_alloweds[field] : NULL;
}

const struct ds_profile *ds_profile_builtin(const char *id, uint32_t rev)
{
    if (id == NULL || !ds_streq(id, ds_profile_r1.id) || rev != ds_profile_r1.rev) {
        return NULL;
    }
    return &ds_profile_r1;
}

/* copy a binding into a field value; 0 when it does not fit with its terminator */
static int copy_value(char *dst, const char *src)
{
    uint32_t i = 0u;
    while (src[i] != '\0') {
        if (i + 1u >= DS_FIELD_VALUE_MAX) {
            return 0;
        }
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return 1;
}

/*
 * Tracer resolve (plan 05 task 1): only the profile id is checked, and a bound value is copied onto its open field as given.
 * The typed refusals of D-49 (rev, closed field, allowed set, unbound field) arrive in task 3.
 */
uint32_t ds_resolve(const char *id, uint32_t rev, const struct ds_bindings *bindings, struct ds_profile *out,
                    uint32_t *field_index)
{
    struct ds_profile work;
    uint32_t i;

    (void)rev;
    if (field_index != NULL) {
        *field_index = DS_NO_FIELD;
    }
    if (id == NULL || !ds_streq(id, ds_profile_r1.id)) {
        return DS_REFUSE_profile_unknown;
    }
    work = ds_profile_r1;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        if (work.field[i].status == DS_STATUS_OPEN && bindings != NULL && bindings->value[i] != NULL) {
            if (copy_value(work.field[i].value, bindings->value[i])) {
                work.field[i].bound_by = DS_BOUND_RUN_REQUEST;
            }
        }
    }
    if (out != NULL) {
        *out = work;
    }
    return DS_OK;
}

#define DS_REFUSAL(name, number) [number] = "load." #name,
static const char *const ds_refusal_names[DS_REFUSAL_SLOTS] = {
    [0] = "ok",
#include "refusals.def"
};
#undef DS_REFUSAL

const char *ds_refusal_name(uint32_t number)
{
    return number < (uint32_t)DS_REFUSAL_SLOTS ? ds_refusal_names[number] : NULL;
}
