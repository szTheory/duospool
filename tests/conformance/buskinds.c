/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * buskinds.c - the case kinds of the bus and the memory map (plan 03-20; D-103, D-106, ER-7). Test code only, never installed. Evidence class E0 (the core against one reading of the project's own assumptions).
 *   bankclass_oracle  (D-103) every bank 0 to 255: the class the core's table gives (the private accessor ds_bank_class) against the class that tests/conformance/bankclass-oracle.awk derives from the statement of LED-CPU-0011.
 *                     The rows come from --bank-oracle FILE, which scripts/conformance-run writes in the same run from the tracked ledger text before the runner starts, so a stale file is never read. The oracle shares no
 *                     code or table with the core (the class names below are this file's own, the awk script has its own), but it has the same author as the core: it is labelled E0/E2, never an independent oracle (ER-7).
 *                     One assertion per bank, 256 in all.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
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
