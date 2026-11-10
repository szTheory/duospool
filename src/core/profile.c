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

/*
 * The unresolved behaviours of profile pce-hucard-2btn rev 1: the root key unresolved_behaviours of r1.toml, ledger ids in sorted
 * order. The conformance case profile_r1_equal proves this list equals the frozen record in both directions.
 */
static const char *const ds_unresolved_r1[] = {"LED-IRQ-0001", "LED-VID-0002", "LED-VID-0003"};

uint32_t ds_profile_unresolved_count(void)
{
    return (uint32_t)(sizeof ds_unresolved_r1 / sizeof ds_unresolved_r1[0]);
}

const char *ds_profile_unresolved(uint32_t index)
{
    return index < ds_profile_unresolved_count() ? ds_unresolved_r1[index] : NULL;
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

/* is v exactly one of the values of the '|'-separated set; an empty set allows nothing */
static int in_allowed(const char *set, const char *v)
{
    while (*set != '\0') {
        const char *start = set;
        const char *p = v;
        while (*start != '\0' && *start != '|' && *p == *start) {
            start++;
            p++;
        }
        if ((*start == '\0' || *start == '|') && *p == '\0') {
            return 1;
        }
        while (*set != '\0' && *set != '|') {
            set++;
        }
        if (*set == '|') {
            set++;
        }
    }
    return 0;
}

/*
 * Resolve profile id and rev against the bindings (D-49). The profile is registered by id and rev, so a resolved profile is by
 * construction equal to the built-in r1. Then every field is looked at in table order and the first fault wins:
 *   an owner-selected field that the request binds, whatever the value      bind_closed_field
 *   an open field that the request leaves unbound (there is no default)     open_unbound
 *   an open field bound to a value outside its allowed set                  bind_not_allowed
 * A refusal names the field in field_index and writes nothing else; out is written only on success. A bound open field records
 * the run request as its binder, which keeps it out of every support claim.
 */
uint32_t ds_resolve(const char *id, uint32_t rev, const struct ds_bindings *bindings, struct ds_profile *out,
                    uint32_t *field_index)
{
    struct ds_profile work;
    uint32_t i;

    if (field_index != NULL) {
        *field_index = DS_NO_FIELD;
    }
    if (id == NULL || !ds_streq(id, ds_profile_r1.id)) {
        return DS_REFUSE_profile_unknown;
    }
    if (rev != ds_profile_r1.rev) {
        return DS_REFUSE_rev_mismatch;
    }
    work = ds_profile_r1;
    for (i = 0u; i < (uint32_t)DS_FIELD_COUNT; i++) {
        const char *bound = bindings != NULL ? bindings->value[i] : NULL;
        uint32_t refusal = DS_OK;

        if (work.field[i].status == DS_STATUS_OWNER) {
            if (bound != NULL) {
                refusal = DS_REFUSE_bind_closed_field;
            }
        } else if (bound == NULL) {
            refusal = DS_REFUSE_open_unbound;
        } else if (!in_allowed(ds_field_alloweds[i], bound) || !copy_value(work.field[i].value, bound)) {
            refusal = DS_REFUSE_bind_not_allowed;
        } else {
            work.field[i].bound_by = DS_BOUND_RUN_REQUEST;
        }
        if (refusal != DS_OK) {
            if (field_index != NULL) {
                *field_index = i;
            }
            return refusal;
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
