/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */

/*
 * core.c - the TOOL-01 spike workload in C11 (protocol W-01 to W-07).
 * One instance owns all of its state. There are no mutable globals or statics, and the core never opens files.
 */
#include "duospool.h"

#include <stdlib.h>
#include <string.h>

#define DS_CONTENT_SIZE 65536u
#define DS_HEADER_SIZE 66048u
#define DS_MAX_SIZE 1048576u

#define DS_FNV_OFFSET UINT64_C(0xcbf29ce484222325)
#define DS_FNV_PRIME UINT64_C(0x100000001b3)

struct ds_instance {
    uint8_t content[DS_CONTENT_SIZE];
    uint64_t time;
    uint64_t digest;
    int loaded;
};

static uint64_t fold_byte(uint64_t digest, uint8_t byte) {
    digest ^= (uint64_t)byte;
    return digest * DS_FNV_PRIME; /* unsigned: wraps mod 2^64 by definition */
}

uint32_t duospool_abi_version(void) {
    return 1u;
}

ds_status duospool_create(ds_instance **out) {
    ds_instance *inst;
    if (out == NULL) {
        return DS_ERR_NULL_ARG;
    }
    *out = NULL;
    inst = (ds_instance *)calloc(1u, sizeof *inst);
    if (inst == NULL) {
        return DS_ERR_STATE; /* no allocation status exists in W-03 */
    }
    inst->digest = DS_FNV_OFFSET;
    *out = inst;
    return DS_OK;
}

ds_status duospool_load(ds_instance *inst, const uint8_t *buf, size_t len) {
    size_t i;
    if (inst == NULL || buf == NULL) {
        return DS_ERR_NULL_ARG;
    }
    if (len == 0u) {
        return DS_REFUSE_EMPTY;
    }
    if (len > (size_t)DS_MAX_SIZE) {
        return DS_REFUSE_OVERSIZED; /* decided from the length alone: no byte of buf is read */
    }
    if (len == (size_t)DS_HEADER_SIZE) {
        return DS_REFUSE_HEADER;
    }
    if (len < (size_t)DS_CONTENT_SIZE) {
        return DS_REFUSE_TRUNCATED;
    }
    if (len != (size_t)DS_CONTENT_SIZE) {
        return DS_REFUSE_BAD_SIZE;
    }
    if (inst->loaded != 0) {
        return DS_ERR_STATE;
    }
    memcpy(inst->content, buf, DS_CONTENT_SIZE);
    for (i = 0u; i < (size_t)DS_CONTENT_SIZE; i++) {
        inst->digest = fold_byte(inst->digest, inst->content[i]);
    }
    inst->loaded = 1;
    return DS_OK;
}

ds_status duospool_step(ds_instance *inst, uint32_t cycles) {
    unsigned shift;
    if (inst == NULL) {
        return DS_ERR_NULL_ARG;
    }
    if (inst->loaded == 0) {
        return DS_ERR_STATE;
    }
    inst->time += (uint64_t)cycles;
    for (shift = 0u; shift < 64u; shift += 8u) {
        inst->digest = fold_byte(inst->digest, (uint8_t)((inst->time >> shift) & 0xffu));
    }
    return DS_OK;
}

ds_status duospool_time(const ds_instance *inst, uint64_t *out) {
    if (inst == NULL || out == NULL) {
        return DS_ERR_NULL_ARG;
    }
    *out = inst->time;
    return DS_OK;
}

ds_status duospool_digest_hex(const ds_instance *inst, char *out, size_t out_len) {
    static const char hex[] = "0123456789abcdef";
    unsigned i;
    if (inst == NULL || out == NULL) {
        return DS_ERR_NULL_ARG;
    }
    if (out_len < 17u) {
        return DS_ERR_SHORT_BUFFER;
    }
    for (i = 0u; i < 16u; i++) {
        out[i] = hex[(inst->digest >> (60u - 4u * i)) & 0xfu];
    }
    out[16] = '\0';
    return DS_OK;
}

void duospool_destroy(ds_instance *inst) {
    free(inst);
}
