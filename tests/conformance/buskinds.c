/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * buskinds.c - the case kinds of the bus and the memory map (plan 03-20; D-103, D-106, ER-7). Test code only, never installed. Evidence class E0 (the core against one reading of the project's own assumptions).
 *   bankclass_oracle  (D-103) every bank 0 to 255: the class the core's table gives (the private accessor ds_bank_class) against the class that tests/conformance/bankclass-oracle.awk derives from the statement of LED-CPU-0011.
 *                     The rows come from --bank-oracle FILE, which scripts/conformance-run writes in the same run from the tracked ledger text before the runner starts, so a stale file is never read. The oracle shares no
 *                     code or table with the core (the class names below are this file's own, the awk script has its own), but it has the same author as the core: it is labelled E0/E2, never an independent oracle (ER-7).
 *                     One assertion per bank, 256 in all.
 *   st_bypass         (D-106, H8-8 Table 2-2-3, S8-80 to S8-82, ASSUMED LED-CPU-0016 and LED-CPU-0017) expect = id=N;rom=NAME;st=PHYS:VALUE;st=PHYS:VALUE;... (at most 16 items; until_mclk and checks required). The built ROM (fixtures/src/st_bypass.s,
 *                     which runs ST0, ST1 and ST2 with distinct immediates under four settings of MPR0) runs with a dev sink and nothing else, to its DSV1 done state or the deadline. The guest's verdict must pass (its checks show that no
 *                     store reached the work RAM), and the device-register log must hold exactly the listed records in order: kind ST, the physical address, the immediate, the flags (the VDC and VCE penalty bit for an address below $1FE800, D-107, and the
 *                     physical bit), spare bytes zero, the stamps strictly increasing (each store is its own instruction) and no record dropped or offered besides. The PHYS and VALUE literals are written in the case from the immediates of the fixture source
 *                     and the manual's Table 2-2-3 ($1FE000 | 0, 2, 3 for ST0, ST1, ST2), never from a DuoSpool run. E0: a pass says the core logs the stores as the fixture wrote them, nothing about hardware.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "runner.h"

#define BANKS 256u
#define CLASS_CAP 12u

const char *bank_oracle_path;

/* the oracle's word for a class value of the core: this file's own table, not the awk script's and not the core's */
static const char *class_word(uint32_t cls)
{
    switch ((enum ds_bank_class)cls) {
    case DS_BANK_CLASS_UNMAPPED:
        return "UNMAPPED";
    case DS_BANK_CLASS_HUCARD:
        return "HUCARD";
    case DS_BANK_CLASS_RAM:
        return "RAM";
    case DS_BANK_CLASS_HWPAGE:
        return "HWPAGE";
    case DS_BANK_CLASS_NONE:
    default:
        break;
    }
    return "NONE";
}

/* the rows of the oracle file into word[bank]: exactly 256 rows, each bank once, each class one of the four words. NULL on success, otherwise why not */
static const char *load_bank_oracle(char word[BANKS][CLASS_CAP])
{
    char *text = bank_oracle_path != NULL ? read_file(bank_oracle_path) : NULL;
    char *line;
    unsigned seen[BANKS];
    size_t rows = 0u;
    size_t i;
    const char *err = NULL;

    if (text == NULL) {
        return "no bank-class oracle file (--bank-oracle FILE) or it cannot be read";
    }
    memset(seen, 0, sizeof seen);
    memset(word, 0, (size_t)BANKS * CLASS_CAP);
    for (line = text; *line != '\0' && err == NULL;) {
        char *nl = strchr(line, '\n');
        char *f[2];
        size_t bank;

        if (nl != NULL) {
            *nl = '\0';
        }
        if (split_tab(line, f, 2) != 2 || parse_size(f[0], &bank) != 0 || bank >= BANKS || strlen(f[1]) >= CLASS_CAP) {
            err = "a malformed bank-class oracle row";
        } else if (seen[bank] != 0u) {
            err = "a bank of the bank-class oracle is repeated";
        } else if (strcmp(f[1], "UNMAPPED") != 0 && strcmp(f[1], "HUCARD") != 0 && strcmp(f[1], "RAM") != 0 && strcmp(f[1], "HWPAGE") != 0) {
            err = "a bank-class oracle row names a class that is not one of the four";
        } else {
            seen[bank] = 1u;
            memcpy(word[bank], f[1], strlen(f[1]) + 1u);
            rows++;
        }
        line = nl != NULL ? nl + 1 : line + strlen(line);
    }
    for (i = 0u; err == NULL && i < BANKS; i++) {
        if (seen[i] == 0u) {
            err = "the bank-class oracle has no row for a bank";
        }
    }
    if (err == NULL && rows != BANKS) {
        err = "the bank-class oracle does not hold exactly 256 rows";
    }
    free(text);
    return err;
}

void kind_bankclass_oracle(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char word[BANKS][CLASS_CAP];
    const char *err;
    uint32_t bank;
    char msg[96];

    (void)k;
    err = load_bank_oracle(word);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    for (bank = 0u; bank < BANKS; bank++) {
        snprintf(msg, sizeof msg, "bank 0x%02X: the core's class differs from the oracle's %s", (unsigned)bank, word[bank]);
        check(c, strcmp(class_word(ds_bank_class(bank)), word[bank]) == 0, msg);
    }
    snprintf(got, gotcap, "banks=%u", (unsigned)BANKS);
}

/* ---- kind st_bypass ---- */

#define SB_ITEMS_MAX 16u
#define SB_DEV_CAP 64u

struct sb_item {
    uint32_t phys;
    uint32_t value;
};

static uint32_t sb_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static uint64_t sb_u64(const uint8_t *p)
{
    return (uint64_t)sb_u32(p) | ((uint64_t)sb_u32(p + 4) << 32u);
}

/* id=N;rom=NAME;st=PHYS:VALUE;... (0 on success): PHYS is 0x1FE000, 0x1FE002 or 0x1FE003 (what Table 2-2-3 allows) and VALUE a byte */
static int sb_parse(const char *text, char *rom, size_t romcap, uint64_t *id, struct sb_item *items, unsigned *n)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have_rom = 0;
    int have_id = 0;

    *n = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "id=", 3u) == 0 && !have_id && parse_u64(item + 3, id) == 0 && *id <= 255u) {
            have_id = 1;
        } else if (strncmp(item, "rom=", 4u) == 0 && !have_rom && strlen(item + 4) > 0u && strlen(item + 4) < romcap && strspn(item + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 4)) {
            memcpy(rom, item + 4, strlen(item + 4) + 1u);
            have_rom = 1;
        } else if (strncmp(item, "st=", 3u) == 0 && *n < SB_ITEMS_MAX) {
            char *colon = strchr(item + 3, ':');
            uint64_t phys = 0u;
            uint64_t value = 0u;

            if (colon == NULL) {
                return -1;
            }
            *colon = '\0';
            if (parse_u64(item + 3, &phys) != 0 || (phys != 0x1FE000u && phys != 0x1FE002u && phys != 0x1FE003u) || parse_u64(colon + 1, &value) != 0 || value > 0xFFu) {
                return -1;
            }
            items[*n].phys = (uint32_t)phys;
            items[*n].value = (uint32_t)value;
            (*n)++;
        } else {
            return -1;
        }
    }
    return have_id && have_rom && *n >= 1u ? 0 : -1;
}

void kind_st_bypass(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    char rom[48];
    uint64_t id = 0u;
    uint64_t until = 0u;
    uint64_t checks = 0u;
    struct sb_item items[SB_ITEMS_MAX];
    unsigned n = 0u;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_run_request rq;
    struct ds_run_result res;
    struct ds_dev_sink *dsk;
    unsigned char *drec;
    const char *err;
    char verdict[64] = "";
    uint32_t count;
    unsigned i;
    char msg[160];

    if (sb_parse(k->val[K_EXPECT], rom, sizeof rom, &id, items, &n) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0 || until == 0u || !k->has[K_CHECKS] ||
        parse_u64(k->val[K_CHECKS], &checks) != 0 || checks == 0u || checks > 255u) {
        harness_error(c, "an st_bypass case needs until_mclk, checks and expect = id=N;rom=NAME;st=PHYS:VALUE;... (PHYS 0x1FE000, 0x1FE002 or 0x1FE003, at most 16 items)");
        return;
    }
    dsk = (struct ds_dev_sink *)calloc(1u, sizeof *dsk);
    drec = (unsigned char *)calloc((size_t)SB_DEV_CAP, DS_DEV_RECORD_BYTES);
    if (dsk == NULL || drec == NULL) {
        free(dsk);
        free(drec);
        harness_error(c, "out of memory for the dev sink of st_bypass");
        return;
    }
    err = boot_built_rom(rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        free(dsk);
        free(drec);
        harness_error(c, err);
        return;
    }
    dsk->records = drec;
    dsk->capacity = SB_DEV_CAP;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.stop = dsv1_done; /* a speed aid only (D-95): until_mclk is the hard bound */
    rq.dev = dsk;
    if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of st_bypass");
    } else {
        note_run(c, res.machine_time, res.reason);
        judge_dsv1(c, ci->ram8k + V_OFF, (unsigned)id, (unsigned)checks, verdict, sizeof verdict);
    }
    release_boot(&b);
    count = sb_u32(dsk->header);
    if (!c->harness && !c->timeout) {
        check(c, sb_u32(dsk->header + 4) == 0u, "the dev sink of st_bypass dropped a record");
        snprintf(msg, sizeof msg, "the dev log holds %u records and offered %llu, the case lists %u", (unsigned)count, (unsigned long long)sb_u64(dsk->header + 8), n);
        check(c, count == n && sb_u64(dsk->header + 8) == (uint64_t)n, msg);
        for (i = 0u; i < n && i < count; i++) {
            const unsigned char *r = drec + (size_t)i * DS_DEV_RECORD_BYTES;
            unsigned j;
            int zeros = 1;

            snprintf(msg, sizeof msg, "store %u: the kind byte is not ST", i + 1u);
            check(c, (uint32_t)r[13] == (uint32_t)DS_DEV_ST, msg);
            snprintf(msg, sizeof msg, "store %u: the physical address is not %06X", i + 1u, (unsigned)items[i].phys);
            check(c, sb_u32(r + 8) == items[i].phys, msg);
            snprintf(msg, sizeof msg, "store %u: the value is not %02X", i + 1u, (unsigned)items[i].value);
            check(c, (uint32_t)r[12] == items[i].value, msg);
            snprintf(msg, sizeof msg, "store %u: the flags are not the penalty bit and the physical bit", i + 1u);
            check(c, (unsigned)r[14] == ((unsigned)DS_DEV_FLAG_PENALTY | (unsigned)DS_DEV_FLAG_PHYS), msg);
            for (j = 18u; j < DS_DEV_RECORD_BYTES; j++) {
                zeros = zeros && r[j] == 0u;
            }
            snprintf(msg, sizeof msg, "store %u: the spare bytes are not zero", i + 1u);
            check(c, zeros, msg);
            if (i > 0u) {
                snprintf(msg, sizeof msg, "store %u: the stamp does not follow the stamp of the store before it", i + 1u);
                check(c, sb_u64(r) > sb_u64(drec + (size_t)(i - 1u) * DS_DEV_RECORD_BYTES), msg);
            }
        }
    }
    free(dsk);
    free(drec);
    snprintf(got, gotcap, "stores=%u %s", (unsigned)count, verdict);
}
