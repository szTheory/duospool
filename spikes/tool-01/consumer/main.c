/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */

/*
 * main.c - the independent installed-package consumer (protocol W-11, G4).
 * usage: consumer <fixture-path>
 * It includes only duospool.h, does its own file I/O (the core never opens files), and prints
 *   abi=1 load=OK time=600 digest=<16 hex>
 * The expected line is computed independently by consumer/expect.pl.
 */
#include <duospool.h>

#include <stdio.h>
#include <stdlib.h>

static int fail(const char *what, int status) {
    fprintf(stderr, "consumer: %s failed (status %d)\n", what, status);
    return 1;
}

int main(int argc, char **argv) {
    FILE *fp;
    uint8_t *buf;
    size_t got;
    ds_instance *inst = NULL;
    ds_status st;
    uint64_t time = 0;
    char digest[17];

    if (argc != 2) {
        fprintf(stderr, "usage: consumer <fixture-path>\n");
        return 2;
    }
    buf = (uint8_t *)malloc(1048577u);
    if (buf == NULL) {
        return 2;
    }
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "consumer: cannot open %s\n", argv[1]);
        free(buf);
        return 2;
    }
    got = fread(buf, 1u, 1048577u, fp);
    fclose(fp);

    if (duospool_abi_version() != 1u) {
        free(buf);
        return fail("abi version", (int)duospool_abi_version());
    }
    st = duospool_create(&inst);
    if (st != DS_OK) {
        free(buf);
        return fail("create", (int)st);
    }
    st = duospool_load(inst, buf, got);
    free(buf);
    if (st != DS_OK) {
        duospool_destroy(inst);
        return fail("load", (int)st);
    }
    if ((st = duospool_step(inst, 100u)) != DS_OK || (st = duospool_step(inst, 200u)) != DS_OK ||
        (st = duospool_step(inst, 300u)) != DS_OK) {
        duospool_destroy(inst);
        return fail("step", (int)st);
    }
    if ((st = duospool_time(inst, &time)) != DS_OK || (st = duospool_digest_hex(inst, digest, sizeof digest)) != DS_OK) {
        duospool_destroy(inst);
        return fail("read-back", (int)st);
    }
    printf("abi=%u load=OK time=%llu digest=%s\n", (unsigned)duospool_abi_version(), (unsigned long long)time, digest);
    duospool_destroy(inst);
    return 0;
}
