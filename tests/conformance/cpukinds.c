/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * cpukinds.c - the CPU table kinds (plan 03-15; D-97, D-98, ER-2, ER-7, T-03-41, T-03-43). Test code only. Evidence class E0 for what the runner measures: the expected values come from
 * the manual transcription (tests/oracle/huc6280-opcodes.tsv) and from page literals written in the case, never from src/core/cpu_ops.def or from a DuoSpool run. Nothing here is an accuracy,
 * compatibility, cycle-accuracy or hardware claim, and the word pass is not a support claim.
 *   opcode_table   compares the byte count and the base cycle count that the compiled core table reports (ds_op_meta, ds_cpu_op_defined: private accessors that read the static const table) for
 *                  all 256 opcodes with the rows of the TSV that scripts/conformance-run passes as --opcode-tsv. One assertion per opcode, 256 in all. The expect is
 *                  defined=N;undefined=M (the manual's 234 and 22, written in the case): the TSV must hold exactly those counts. A defined row compares bytes and cycles (a block transfer prints 17+6x in the TSV
 *                  and the core table holds its base 17; the 6 per byte is the block step of plan 03-25) and requires the core to mark the opcode defined. An UNDEF row is a one-assertion check
 *                  that the core marks the opcode undefined: its 1 byte and 2 cycles are an assumption (LED-CPU-0010), carried in the TSV as - and not compared. The TSV is untrusted input (T-03-41): seven TAB
 *                  separated columns on every data row, an opcode of exactly two hex digits, each opcode once, 256 rows, numbers of at most one digit (bytes) or two digits (cycles); any fault is harness_failure.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

const char *opcode_tsv_path;

/* --opcode-tsv FILE, parsed beside --oracle (F7: runner.c gets one call): 1 when argv[*a] was this option (and *a moves to its value), otherwise 0 */
int cpukinds_arg(int argc, char **argv, int *a)
{
    if (strcmp(argv[*a], "--opcode-tsv") == 0 && *a + 1 < argc) {
        opcode_tsv_path = argv[*a + 1];
        *a += 1;
        return 1;
    }
    return 0;
}

/* ---- opcode_table ---- */

#define OT_BLOCK_BASE 17u /* the base of the block-transfer formula 17+6x (H8-6, S8-89) */

struct ot_row {
    int seen;
    int defined; /* 0 for an UNDEF row */
    unsigned bytes;
    unsigned cycles; /* the base: 17 for a block transfer */
    char mnemonic[12];
};

static int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

/* a TSV number: one or two decimal digits, no sign (0 on success) */
static int small_number(const char *s, unsigned *out)
{
    size_t n = strlen(s);

    if (n < 1u || n > 2u || s[0] < '0' || s[0] > '9' || (n == 2u && (s[1] < '0' || s[1] > '9'))) {
        return -1;
    }
    *out = (unsigned)(s[0] - '0');
    if (n == 2u) {
        *out = *out * 10u + (unsigned)(s[1] - '0');
    }
    return 0;
}

/* one data row into rows[]: NULL on success, otherwise why the row is unusable */
static const char *ot_row_parse(char *line, struct ot_row *rows)
{
    char *f[8];
    int hi;
    int lo;
    unsigned op;
    struct ot_row *r;

    if (split_tab(line, f, 8) != 7) {
        return "a TSV data row does not have exactly seven TAB separated columns";
    }
    hi = hex_digit(f[0][0]);
    lo = hi >= 0 ? hex_digit(f[0][1]) : -1;
    if (hi < 0 || lo < 0 || f[0][2] != '\0') {
        return "a TSV opcode is not two uppercase hex digits";
    }
    op = (unsigned)hi * 16u + (unsigned)lo;
    r = &rows[op];
    if (r->seen != 0) {
        return "a TSV opcode appears twice";
    }
    if (f[1][0] == '\0' || strlen(f[1]) >= sizeof r->mnemonic || f[2][0] == '\0' || f[6][0] == '\0') {
        return "a TSV mnemonic, mode or cite is empty or too long";
    }
    memcpy(r->mnemonic, f[1], strlen(f[1]) + 1u);
    r->seen = 1;
    if (strcmp(f[1], "UNDEF") == 0) {
        if (strcmp(f[3], "-") != 0 || strcmp(f[4], "-") != 0) {
            return "an UNDEF row must carry - in the bytes and cycles columns";
        }
        r->defined = 0;
        return NULL;
    }
    r->defined = 1;
    if (small_number(f[3], &r->bytes) != 0 || r->bytes < 1u || r->bytes > 7u) {
        return "the bytes column of a defined row is not 1 to 7";
    }
    if (strcmp(f[4], "17+6x") == 0) {
        if (strcmp(f[2], "BLK") != 0 || r->bytes != 7u) {
            return "the cycles 17+6x belong to a 7-byte BLK row only";
        }
        r->cycles = OT_BLOCK_BASE;
    } else if (small_number(f[4], &r->cycles) != 0 || r->cycles < 1u || strcmp(f[2], "BLK") == 0) {
        return "the cycles column of a defined row is not a count (or a BLK row lacks 17+6x)";
    }
    return NULL;
}

/* the TSV file into rows[256]: NULL on success, otherwise why not (every fault is a harness fault) */
static const char *ot_load(struct ot_row *rows, unsigned *defined, unsigned *undefined)
{
    char *text = opcode_tsv_path != NULL ? read_file(opcode_tsv_path) : NULL;
    char *line;
    const char *err = NULL;
    unsigned i;

    if (text == NULL) {
        return "no opcode TSV (--opcode-tsv FILE) or it cannot be read";
    }
    memset(rows, 0, 256u * sizeof rows[0]);
    for (line = text; *line != '\0' && err == NULL;) {
        char *nl = strchr(line, '\n');

        if (nl != NULL) {
            *nl = '\0';
        }
        if (line[0] != '\0' && line[0] != '#') {
            err = ot_row_parse(line, rows);
        }
        line = nl != NULL ? nl + 1 : line + strlen(line);
    }
    free(text);
    *defined = 0u;
    *undefined = 0u;
    for (i = 0u; err == NULL && i < 256u; i++) {
        if (rows[i].seen == 0) {
            err = "the opcode TSV has no row for an opcode";
        } else if (rows[i].defined != 0) {
            (*defined)++;
        } else {
            (*undefined)++;
        }
    }
    return err;
}

/* defined=N;undefined=M (0 on success) */
static int ot_expect(const char *text, uint64_t *defined, uint64_t *undefined)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have = 0;

    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "defined=", 8u) == 0 && (have & 1) == 0 && parse_u64(item + 8, defined) == 0) {
            have |= 1;
        } else if (strncmp(item, "undefined=", 10u) == 0 && (have & 2) == 0 && parse_u64(item + 10, undefined) == 0) {
            have |= 2;
        } else {
            return -1;
        }
    }
    return have == 3 ? 0 : -1;
}

void kind_opcode_table(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct ot_row rows[256];
    uint64_t want_defined = 0u;
    uint64_t want_undefined = 0u;
    unsigned defined = 0u;
    unsigned undefined = 0u;
    const char *err;
    unsigned op;

    if (ot_expect(k->val[K_EXPECT], &want_defined, &want_undefined) != 0) {
        harness_error(c, "an opcode_table expect is defined=N;undefined=M");
        return;
    }
    err = ot_load(rows, &defined, &undefined);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if ((uint64_t)defined != want_defined || (uint64_t)undefined != want_undefined) {
        harness_error(c, "the opcode TSV does not hold the defined and undefined counts that the case states");
        return;
    }
    for (op = 0u; op < 256u; op++) {
        char what[96];
        int core_defined = ds_cpu_op_defined(op) != 0;
        uint32_t bytes = 0u;
        uint32_t cycles = 0u;
        int ok;

        if (rows[op].defined == 0) {
            ok = !core_defined;
            snprintf(what, sizeof what, "opcode 0x%02X is undefined in the manual but the core table defines it", op);
        } else {
            ds_op_meta(op, &bytes, &cycles);
            ok = core_defined && bytes == rows[op].bytes && cycles == rows[op].cycles;
            snprintf(what, sizeof what, "opcode 0x%02X (%s): core defined=%d bytes=%u cycles=%u, manual bytes=%u cycles=%u", op, rows[op].mnemonic, core_defined ? 1 : 0,
                     (unsigned)bytes, (unsigned)cycles, rows[op].bytes, rows[op].cycles);
        }
        check(c, ok, what);
    }
    snprintf(got, gotcap, "opcodes=256 defined=%u undefined=%u", defined, undefined);
}
