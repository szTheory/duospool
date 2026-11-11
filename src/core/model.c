/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * model.c - the named modelling parameters and ds_model_set (plan 03-17; D-105, D-113, D-121, D-134).
 * The rows are generated from model_params.def and the refusal names from model_refusals.def; nothing here is writable but the instance. A selection is a one-byte index into the allowed list of its row, kept in the
 * instance member that has the row's name (tma_zero lives in region BUS0), so a snapshot of the instance carries it and a restored machine behaves as the original (D-121). The setter validates the name, then the value, then the
 * state of the machine, and writes only after all three passed, so a refusal leaves the instance byte-identical (T-03-47).
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_MODELREF(name, number) [number] = "model." #name,
static const char *const ds_model_refusal_names[DS_MODELREF_SLOTS] = {
    [0] = "ok",
#include "model_refusals.def"
};
#undef DS_MODELREF

/* the slot ceiling (F4): the rows are numbered 1..N without a gap or a reuse, so N is the row count and must leave a slot below DS_MODELREF_SLOTS */
#define DS_MODELREF(name, number) +1
enum { DS_MODELREF_ROWS = 0
#include "model_refusals.def"
};
#undef DS_MODELREF
_Static_assert(DS_MODELREF_ROWS < (int)DS_MODELREF_SLOTS, "model_refusals.def has more rows than DS_MODELREF_SLOTS slots");

#define DS_MODEL(name, allowed, def, ledger, status) #name,
static const char *const ds_model_names[DS_MODEL_COUNT] = {
#include "model_params.def"
};
#undef DS_MODEL

#define DS_MODEL(name, allowed, def, ledger, status) allowed,
static const char *const ds_model_alloweds[DS_MODEL_COUNT] = {
#include "model_params.def"
};
#undef DS_MODEL

#define DS_MODEL(name, allowed, def, ledger, status) def,
static const char *const ds_model_defaults[DS_MODEL_COUNT] = {
#include "model_params.def"
};
#undef DS_MODEL

#define DS_MODEL(name, allowed, def, ledger, status) ledger,
static const char *const ds_model_ledgers[DS_MODEL_COUNT] = {
#include "model_params.def"
};
#undef DS_MODEL

#define DS_MODEL(name, allowed, def, ledger, status) status,
static const char *const ds_model_statuses[DS_MODEL_COUNT] = {
#include "model_params.def"
};
#undef DS_MODEL

const char *ds_model_refusal_name(uint32_t number)
{
    return number < (uint32_t)DS_MODELREF_SLOTS ? ds_model_refusal_names[number] : NULL;
}

const char *ds_model_name(uint32_t index)
{
    return index < (uint32_t)DS_MODEL_COUNT ? ds_model_names[index] : NULL;
}

const char *ds_model_allowed(uint32_t index)
{
    return index < (uint32_t)DS_MODEL_COUNT ? ds_model_alloweds[index] : NULL;
}

const char *ds_model_default(uint32_t index)
{
    return index < (uint32_t)DS_MODEL_COUNT ? ds_model_defaults[index] : NULL;
}

const char *ds_model_ledger(uint32_t index)
{
    return index < (uint32_t)DS_MODEL_COUNT ? ds_model_ledgers[index] : NULL;
}

const char *ds_model_status(uint32_t index)
{
    return index < (uint32_t)DS_MODEL_COUNT ? ds_model_statuses[index] : NULL;
}

/* the selection byte of row index in the instance: the member that has the row's name */
static uint8_t selection_get(const struct ds_instance *inst, uint32_t index)
{
    switch (index) {
#define DS_MODEL(name, allowed, def, ledger, status) \
    case DS_MODEL_##name:                            \
        return inst->name;
#include "model_params.def"
#undef DS_MODEL
    default:
        return 0u;
    }
}

static void selection_put(struct ds_instance *inst, uint32_t index, uint8_t value)
{
    switch (index) {
#define DS_MODEL(name, allowed, def, ledger, status) \
    case DS_MODEL_##name:                            \
        inst->name = value;                          \
        break;
#include "model_params.def"
#undef DS_MODEL
    default:
        break;
    }
}

/* the end of the token at list: a '|' or the terminating NUL */
static int token_end(char ch)
{
    return ch == '\0' || ch == '|';
}

/* the index of the token of the '|' separated list that equals value exactly, or -1 */
static int token_index(const char *list, const char *value)
{
    int idx = 0;

    for (;;) {
        const char *v = value;

        while (!token_end(*list) && *list == *v) {
            list++;
            v++;
        }
        if (token_end(*list) && *v == '\0') {
            return idx;
        }
        while (!token_end(*list)) {
            list++;
        }
        if (*list == '\0') {
            return -1;
        }
        list++;
        idx++;
    }
}

const char *ds_model_value(const struct ds_instance *inst, uint32_t index, size_t *len)
{
    const char *list;
    uint32_t want;
    uint32_t at = 0u;

    if (inst == NULL || len == NULL || index >= (uint32_t)DS_MODEL_COUNT) {
        return NULL;
    }
    list = ds_model_alloweds[index];
    want = (uint32_t)selection_get(inst, index);
    for (;;) {
        size_t n = 0u;

        while (!token_end(list[n])) {
            n++;
        }
        if (at == want) {
            *len = n;
            return list;
        }
        if (list[n] == '\0') {
            return NULL;
        }
        list = list + n + 1u;
        at++;
    }
}

uint32_t ds_model_set(struct ds_instance *inst, const char *name, const char *value)
{
    uint32_t row = (uint32_t)DS_MODEL_COUNT;
    uint32_t i;
    int idx;

    if (name != NULL) {
        for (i = 0u; i < (uint32_t)DS_MODEL_COUNT; i++) {
            if (ds_streq(name, ds_model_names[i])) {
                row = i;
            }
        }
    }
    if (row >= (uint32_t)DS_MODEL_COUNT) {
        return (uint32_t)DS_MODELREF_unknown_name;
    }
    idx = value != NULL ? token_index(ds_model_alloweds[row], value) : -1;
    if (idx < 0) {
        return (uint32_t)DS_MODELREF_value_not_allowed;
    }
    if (inst == NULL || inst->machine_time != UINT64_C(0)) {
        return (uint32_t)DS_MODELREF_state;
    }
    selection_put(inst, row, (uint8_t)idx);
    return (uint32_t)DS_MODELREF_NONE;
}
