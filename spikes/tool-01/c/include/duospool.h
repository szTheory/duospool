/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */

/*
 * duospool.h - hand-written C API of the TOOL-01 spike workload (protocol W-03, W-04, W-10).
 * This file is byte-identical in the C and the Rust skeleton. It is spike evidence, not a frozen ABI.
 */
#ifndef DUOSPOOL_H
#define DUOSPOOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ds_status {
    DS_OK = 0,
    DS_REFUSE_EMPTY = 1,
    DS_REFUSE_TRUNCATED = 2,
    DS_REFUSE_BAD_SIZE = 3,
    DS_REFUSE_OVERSIZED = 4,
    DS_REFUSE_HEADER = 5,
    DS_ERR_NULL_ARG = 6,
    DS_ERR_SHORT_BUFFER = 7,
    DS_ERR_STATE = 8
} ds_status;

/* Opaque instance. All state is per instance; there are no globals. */
typedef struct ds_instance ds_instance;

uint32_t duospool_abi_version(void);
ds_status duospool_create(ds_instance **out);
ds_status duospool_load(ds_instance *inst, const uint8_t *buf, size_t len);
ds_status duospool_step(ds_instance *inst, uint32_t cycles);
ds_status duospool_time(const ds_instance *inst, uint64_t *out);
/* out_len must be at least 17: 16 lowercase hex digits plus NUL. */
ds_status duospool_digest_hex(const ds_instance *inst, char *out, size_t out_len);
/* Accepts NULL. */
void duospool_destroy(ds_instance *inst);

#ifdef __cplusplus
}
#endif

#endif /* DUOSPOOL_H */
