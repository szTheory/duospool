/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */

/*
 * fuzz_loader.c - libFuzzer target over the C loader (protocol G5).
 * Uses only the public duospool_* API: create, load the input bytes, step once when the load was accepted, read the
 * digest, destroy. It never aborts on a refusal; refusals are the expected outcome for almost every input.
 */
#include "duospool.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    ds_instance *inst = NULL;
    char digest[17];
    static const uint8_t empty[1] = {0};
    if (duospool_create(&inst) != DS_OK) {
        return 0;
    }
    /* libFuzzer may pass a NULL data pointer for size 0; the loader's W-05 rule refuses NULL, so pass a valid pointer. */
    if (duospool_load(inst, data == NULL ? empty : data, size) == DS_OK) {
        (void)duospool_step(inst, 100u);
    }
    (void)duospool_digest_hex(inst, digest, sizeof digest);
    duospool_destroy(inst);
    return 0;
}
