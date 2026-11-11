/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_mapper.h - the plain-HuCard mapper boundary (D-44). The image size is classified once, at boot, into a size enum, and every
 * mirror table is indexed by that enum: no function here takes a raw size except the single classifier ds_size_classify.
 * The mapper window is the 128 banks $00-$7F of 8 KiB; banks $80 and above are not ROM and are reported as unmapped (no value:
 * Phase 3 owns what such a read returns). Every rule is a modelling assumption (LED-LOAD-0003, LED-LOAD-0005), not a hardware fact.
 * Only stddef.h and stdint.h are reached from here, through ds_core.h (D-54).
 */
#ifndef DS_MAPPER_H
#define DS_MAPPER_H

#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"

#define DS_BANK_BYTES 8192u /* the size of one bank, also the offset limit of a read */
#define DS_BANK_COUNT 128u  /* banks $00-$7F are the HuCard window */
#define DS_BANK_LIMIT 0x80u /* banks at and above this index are not ROM */

/* the supported plain sizes; the value stored in the instance, never the raw length */
enum ds_size { DS_SIZE_NONE = 0, DS_SIZE_256K = 1, DS_SIZE_384K = 2, DS_SIZE_1M = 3, DS_SIZE_COUNT };

/* the result of ds_mapper_read */
enum ds_map_status {
    DS_MAP_OK = 0,
    DS_MAP_UNMAPPED = 1,   /* bank $80 and above (or no mapped image): nothing is read and *out is not written */
    DS_MAP_BAD_OFFSET = 2  /* offset 8192 and above: nothing is read and *out is not written */
};

/* the size enum of a raw length, DS_SIZE_NONE when the length is not one of the supported sizes */
uint32_t ds_size_classify(size_t len);

/* the byte size of an enum value, 0 for DS_SIZE_NONE and for any value that is not a member */
uint32_t ds_size_bytes(uint32_t size_class);

/*
 * Read the byte at (bank, offset) of the 8 KiB banks of a booted instance through the mirror of its size class. The ROM array
 * is touched only on DS_MAP_OK, and only at an index below the size of the image.
 */
uint32_t ds_mapper_read(const struct ds_instance *inst, uint32_t bank, uint32_t offset, uint8_t *out);

#endif
