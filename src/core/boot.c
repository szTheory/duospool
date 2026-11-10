/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * boot.c - ds_boot, ds_instance_size and ds_instance_align (D-42, D-45, D-46, D-66).
 * Tracer version (plan 05 task 1): the first three checks of the fixed precedence are inline, then exactly one input is
 * accepted (256 KiB declared plain, pce, none); every other input is size_unsupported until task 3 completes the precedence.
 * Nothing is read from the request content before every check has passed, and a refusal writes only the result struct.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_TRACER_SIZE 262144u
#define DS_BYTE_ORDER_PCE 1u
#define DS_HEADER_NONE 1u

size_t ds_instance_size(void)
{
    return sizeof(struct ds_instance);
}

size_t ds_instance_align(void)
{
    return _Alignof(struct ds_instance);
}

static uint32_t finish(struct ds_boot_result *result, uint32_t reason, uint32_t field, struct ds_instance *instance)
{
    if (result != NULL) {
        result->reason = reason;
        result->field_index = field;
        result->instance = instance;
    }
    return reason;
}

uint32_t ds_boot(void *mem, size_t cap, const struct ds_boot_request *request, struct ds_boot_result *result)
{
    struct ds_profile profile;
    struct ds_instance *inst;
    uint32_t field = DS_NO_FIELD;
    uint32_t reason;

    /* memory first (D-66): a misaligned cast is undefined, so the address is tested as an integer and never cast before */
    if (mem == NULL || cap < ds_instance_size() || ((uintptr_t)mem % (uintptr_t)ds_instance_align()) != 0u) {
        return finish(result, DS_REFUSE_memory_invalid, DS_NO_FIELD, NULL);
    }
    if (request == NULL || request->struct_size != (uint32_t)sizeof(struct ds_boot_request) || request->reserved != 0u) {
        return finish(result, DS_REFUSE_request_invalid, DS_NO_FIELD, NULL);
    }
    reason = ds_resolve(request->profile_id, request->profile_rev, &request->bindings, &profile, &field);
    if (reason != DS_OK) {
        return finish(result, reason, field, NULL);
    }
    if (request->mapper == NULL || !ds_streq(request->mapper, "plain") || request->byte_order != DS_BYTE_ORDER_PCE ||
        request->header != DS_HEADER_NONE || request->content == NULL || request->len != (size_t)DS_TRACER_SIZE) {
        return finish(result, DS_REFUSE_size_unsupported, DS_NO_FIELD, NULL);
    }

    __builtin_memset(mem, 0, sizeof(struct ds_instance));
    inst = (struct ds_instance *)mem;
    inst->profile = profile;
    inst->mapper[0] = 'p';
    inst->mapper[1] = 'l';
    inst->mapper[2] = 'a';
    inst->mapper[3] = 'i';
    inst->mapper[4] = 'n';
    inst->byte_order = request->byte_order;
    inst->header = request->header;
    inst->content_size = (uint32_t)request->len;
    __builtin_memcpy(inst->rom, request->content, request->len);
    return finish(result, DS_OK, DS_NO_FIELD, inst);
}
