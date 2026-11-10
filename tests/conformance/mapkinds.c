/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * mapkinds.c - the case kinds that read an accepted instance through the plain mapper: banksig (every bank of the window against
 * the independent oracle) and unmapped (banks and offsets outside the window). They were part of runner.c and moved here when
 * plan 11 needed room for the refusal-matrix kinds; the boot helpers they use (boot_accepted, release_boot) stay in runner.c and
 * are declared in runner.h. Test code only, never installed.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "runner.h"
#include "synth.h"

/*
 * kind banksig (D-44, D-59): boot the request over the fixture and read byte 0 and byte 8191 of every bank 0 to 127 of the window
 * through ds_mapper_read; each must be an OK read equal to the FIRST and LAST column of the oracle row of that bank. The rows
 * come from --oracle FILE, written by scripts/conformance-run from tests/conformance/mirror-oracle.awk, which shares no code or
 * table with the core or with the generator; the size selecting the rows is the natural size of the fixture recipe, not anything
 * the core reports. Assertions: two per bank, 256 for the 128 banks.
 */
#define ORACLE_BANKS 128u

const char *oracle_path;

/* the rows of one size (KiB) of the oracle TSV into first/last by bank; NULL on success, otherwise why not */
static const char *load_oracle(size_t kib, unsigned *first, unsigned *last)
{
    char *text = oracle_path != NULL ? read_file(oracle_path) : NULL;
    char *line;
    unsigned seen[ORACLE_BANKS];
    size_t i;
    const char *err = NULL;

    if (text == NULL) {
        return "no oracle file (--oracle FILE) or it cannot be read";
    }
    memset(seen, 0, sizeof seen);
    for (line = text; *line != '\0' && err == NULL;) {
        char *nl = strchr(line, '\n');
        char *f[5];
        size_t sz;
        size_t bank;
        size_t fb;
        size_t fi;
        size_t la;

        if (nl != NULL) {
            *nl = '\0';
        }
        if (split_tab(line, f, 5) != 5 || parse_size(f[0], &sz) != 0 || parse_size(f[1], &bank) != 0 ||
            parse_size(f[2], &fb) != 0 || parse_size(f[3], &fi) != 0 || parse_size(f[4], &la) != 0 || fi > 255u || la > 255u) {
            err = "a malformed oracle row";
        } else if (sz == kib) {
            if (bank >= ORACLE_BANKS || seen[bank] != 0u) {
                err = "an oracle bank is out of range or repeated";
            } else {
                seen[bank] = 1u;
                first[bank] = (unsigned)fi;
                last[bank] = (unsigned)la;
            }
        }
        line = nl != NULL ? nl + 1 : line + strlen(line);
    }
    for (i = 0u; err == NULL && i < ORACLE_BANKS; i++) {
        if (seen[i] == 0u) {
            err = "the oracle has no row for a bank of this size";
        }
    }
    free(text);
    return err;
}

void kind_banksig(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    unsigned first[ORACLE_BANKS];
    unsigned last[ORACLE_BANKS];
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    size_t natural = synth_size(k->val[K_FIXTURE]);
    const char *err;
    uint32_t bank;

    err = natural % 1024u == 0u && natural != 0u ? load_oracle(natural / 1024u, first, last) : "the fixture has no size in whole KiB";
    if (err == NULL) {
        err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    }
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    for (bank = 0u; bank < ORACLE_BANKS; bank++) {
        uint8_t lo = 0u;
        uint8_t hi = 0u;
        uint32_t st_lo = ds_mapper_read(inst, bank, 0u, &lo);
        uint32_t st_hi = ds_mapper_read(inst, bank, DS_BANK_BYTES - 1u, &hi);

        check(c, st_lo == (uint32_t)DS_MAP_OK && (unsigned)lo == first[bank], "the first byte of a bank differs from the oracle");
        check(c, st_hi == (uint32_t)DS_MAP_OK && (unsigned)hi == last[bank], "the last byte of a bank differs from the oracle");
    }
    snprintf(got, gotcap, "banks=%u", (unsigned)ORACLE_BANKS);
    release_boot(&b);
}

/*
 * kind unmapped (D-44, T-02-11): for each of the three sizes (the recipes plain-256k, plain-384k, plain-1m, each booted with the
 * default request) every bank 0x80 to 0xFF must give DS_MAP_UNMAPPED, bank 0xFFFFFFFF must too, an offset of 8192 and the largest
 * offset in a mapped bank must give DS_MAP_BAD_OFFSET, and none of these statuses may write the output byte (it keeps its
 * sentinel), which also shows that the ROM array was not read. The case names no value for an unmapped read: Phase 3 owns it.
 */
void kind_unmapped(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const char *const recipe[3] = {"plain-256k", "plain-384k", "plain-1m"};
    size_t r;

    (void)k;
    for (r = 0u; r < 3u; r++) {
        struct booted b;
        struct spec s;
        const struct ds_instance *inst = NULL;
        const char *err = boot_accepted(recipe[r], "", &b, &s, &inst);
        uint32_t bank;
        uint8_t v;

        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        for (bank = DS_BANK_LIMIT; bank <= 0xFFu; bank++) {
            v = 0x5au;
            check(c, ds_mapper_read(inst, bank, 0u, &v) == (uint32_t)DS_MAP_UNMAPPED && v == 0x5au,
                  "a bank of 0x80 or above is not unmapped");
        }
        v = 0x5au;
        check(c, ds_mapper_read(inst, 0xFFFFFFFFu, 0u, &v) == (uint32_t)DS_MAP_UNMAPPED && v == 0x5au,
              "bank 0xFFFFFFFF is not unmapped");
        v = 0x5au;
        check(c, ds_mapper_read(inst, 0u, DS_BANK_BYTES, &v) == (uint32_t)DS_MAP_BAD_OFFSET && v == 0x5au,
              "offset 8192 is not a bad offset");
        v = 0x5au;
        check(c, ds_mapper_read(inst, DS_BANK_LIMIT - 1u, 0xFFFFFFFFu, &v) == (uint32_t)DS_MAP_BAD_OFFSET && v == 0x5au,
              "the largest offset is not a bad offset");
        release_boot(&b);
    }
    snprintf(got, gotcap, "sizes=3");
}
