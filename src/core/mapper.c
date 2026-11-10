/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * mapper.c - the size-enum indexed mirror tables of the plain mapper (rule plain-v1, D-44) and ds_mapper_read.
 * One row per size enum value: the byte size and the function that maps a bank index b of the window to a file bank. The rows
 * are the assumed rules of LED-LOAD-0003; the independent oracle of the conformance suite carries its own copy of the
 * arithmetic and refuses to run when the ledger text of the rule changes. The core never inspects content to choose a row.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"

typedef uint32_t (*mirror_fn)(uint32_t b);

/* 256K: file bank = b mod 32 */
static uint32_t mirror_256k(uint32_t b)
{
    return b % 32u;
}

/* the table, indexed by the size enum; a row that is not filled stays zero and is never mapped */
static const struct {
    uint32_t bytes;
    mirror_fn mirror;
} size_row[DS_SIZE_COUNT] = {
    [DS_SIZE_256K] = {262144u, mirror_256k},
};

uint32_t ds_size_classify(size_t len)
{
    uint32_t i;

    for (i = 1u; i < (uint32_t)DS_SIZE_COUNT; i++) {
        if (size_row[i].mirror != NULL && len == (size_t)size_row[i].bytes) {
            return i;
        }
    }
    return (uint32_t)DS_SIZE_NONE;
}

uint32_t ds_size_bytes(uint32_t size_class)
{
    return size_class < (uint32_t)DS_SIZE_COUNT && size_row[size_class].mirror != NULL ? size_row[size_class].bytes : 0u;
}

uint32_t ds_mapper_read(const struct ds_instance *inst, uint32_t bank, uint32_t offset, uint8_t *out)
{
    uint32_t cls;
    uint32_t index;

    if (inst == NULL || out == NULL) {
        return (uint32_t)DS_MAP_UNMAPPED;
    }
    cls = inst->size_class;
    if (cls >= (uint32_t)DS_SIZE_COUNT || size_row[cls].mirror == NULL) {
        return (uint32_t)DS_MAP_UNMAPPED;
    }
    if (bank >= DS_BANK_LIMIT) {
        return (uint32_t)DS_MAP_UNMAPPED;
    }
    if (offset >= DS_BANK_BYTES) {
        return (uint32_t)DS_MAP_BAD_OFFSET;
    }
    index = size_row[cls].mirror(bank) * DS_BANK_BYTES + offset;
    /* a table that ever named a bank outside the image must not read outside the array: refuse instead */
    if (index >= size_row[cls].bytes || index >= DS_ROM_CAP) {
        return (uint32_t)DS_MAP_UNMAPPED;
    }
    *out = inst->rom[index];
    return (uint32_t)DS_MAP_OK;
}
