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
 *   cycle_probe    boots an authored ROM (rom=NAME in the expect), runs it with an instruction sink to the case's until_mclk or the DSV1 done state (whichever comes first) and judges the cycle count of single
 *                  instructions from the trace. The expect is rom=NAME followed by items PC:OPCODE:CYCLES:SPEED separated by semicolons, in the order the trace executes them (PC and OPCODE as 0xNN hex, CYCLES a
 *                  decimal count, SPEED L for the power-up low speed or H for high). For each item the runner finds the first trace record at or after the previous match whose pc and opcode are the item's (the listing
 *                  gives the address and the opcode: order and address are independent of the core, ER-7), and asserts two things: the speed byte of that record is the speed in force that the item states (the record holds
 *                  the state BEFORE the instruction, so CSH itself is at L and the next instruction at H, D-101), and the start stamp of the next record minus its own start stamp equals CYCLES times the master clocks per cycle of
 *                  the stated speed (12 for L, 3 for H, D-90: written here as literals, not read from the core). The CYCLES literals are page values taken by the author of the case from the manual pages, never from cpu_ops.def
 *                  or a DuoSpool run (D-98, ER-2). A sink that lost a record (lost != 0) is harness_failure: the log is evidence here (D-119, T-03-43). What is measured is the core's own charged time, so the comparison is a check of the
 *                  core against page literals; it is not an in-guest clock and no timer is involved (the timer arrives in plan 03-21).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_obs.h"
#include "ds_timer.h"
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

/* ---- cycle_probe ---- */

#define CP_ITEMS_MAX 32u
#define CP_SINK_RECORDS 8192u
#define CP_MCLK_LOW 12u  /* master clocks per CPU cycle at the power-up low speed (D-90), a literal of the judge */
#define CP_MCLK_HIGH 3u  /* and at high speed */

struct cp_item {
    uint32_t pc;
    uint32_t opcode;
    uint32_t cycles;
    int high; /* 0 = L, 1 = H */
};

static uint32_t cp_get_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u);
}

static uint64_t cp_get_u64(const uint8_t *p)
{
    uint64_t v = 0u;
    unsigned i;

    for (i = 0u; i < 8u; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

/* one item PC:OPCODE:CYCLES:SPEED (0 on success) */
static int cp_item_parse(char *tok, struct cp_item *it)
{
    char *f[4];
    unsigned n = 0u;
    uint64_t pc;
    uint64_t op;
    uint64_t cyc;

    for (;;) {
        char *colon;

        f[n++] = tok;
        colon = strchr(tok, ':');
        if (colon == NULL) {
            break;
        }
        if (n == 4u) {
            return -1;
        }
        *colon = '\0';
        tok = colon + 1;
    }
    if (n != 4u || parse_u64(f[0], &pc) != 0 || pc > 0xFFFFu || parse_u64(f[1], &op) != 0 || op > 0xFFu || parse_u64(f[2], &cyc) != 0 || cyc < 1u || cyc > 64u ||
        (strcmp(f[3], "L") != 0 && strcmp(f[3], "H") != 0)) {
        return -1;
    }
    it->pc = (uint32_t)pc;
    it->opcode = (uint32_t)op;
    it->cycles = (uint32_t)cyc;
    it->high = strcmp(f[3], "H") == 0;
    return 0;
}

/* rom=NAME;PC:OP:CYC:S;... (0 on success) */
static int cp_parse(const char *text, char *rom, size_t romcap, struct cp_item *items, unsigned *nitems)
{
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    int have_rom = 0;

    *nitems = 0u;
    if (pk_copy_text(text, list, sizeof list) != 0) {
        return -1;
    }
    while ((item = pk_next_item(&rest, ';')) != NULL) {
        if (strncmp(item, "rom=", 4u) == 0 && !have_rom && strlen(item + 4) > 0u && strlen(item + 4) < romcap && strspn(item + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") == strlen(item + 4)) {
            memcpy(rom, item + 4, strlen(item + 4) + 1u);
            have_rom = 1;
        } else if (*nitems < CP_ITEMS_MAX && cp_item_parse(item, &items[*nitems]) == 0) {
            (*nitems)++;
        } else {
            return -1;
        }
    }
    return have_rom && *nitems >= 1u ? 0 : -1;
}

void kind_cycle_probe(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct cp_item items[CP_ITEMS_MAX];
    unsigned nitems = 0u;
    char rom[48];
    uint64_t until = 0u;
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_insn_sink *sink;
    uint8_t *records;
    struct ds_run_request rq;
    struct ds_run_result res;
    const char *err;
    uint32_t count;
    uint32_t cursor = 0u;
    unsigned i;
    unsigned matched = 0u;

    if (cp_parse(k->val[K_EXPECT], rom, sizeof rom, items, &nitems) != 0 || !k->has[K_UNTIL_MCLK] || parse_u64(k->val[K_UNTIL_MCLK], &until) != 0) {
        harness_error(c, "a cycle_probe case needs until_mclk and an expect rom=NAME;PC:OPCODE:CYCLES:SPEED;... (PC and OPCODE hex, SPEED L or H)");
        return;
    }
    sink = (struct ds_insn_sink *)calloc(1u, sizeof *sink);
    records = (uint8_t *)malloc((size_t)CP_SINK_RECORDS * DS_INSN_RECORD_BYTES);
    if (sink == NULL || records == NULL) {
        free(sink);
        free(records);
        harness_error(c, "out of memory for the cycle probe sink");
        return;
    }
    err = boot_built_rom(rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        free(sink);
        free(records);
        harness_error(c, err);
        return;
    }
    sink->records = records;
    sink->capacity = CP_SINK_RECORDS;
    memset(&rq, 0, sizeof rq);
    memset(&res, 0, sizeof res);
    rq.struct_size = (uint32_t)sizeof rq;
    rq.until_mclk = until;
    rq.stop = dsv1_done;
    rq.insn = sink;
    if (ds_run((struct ds_instance *)(void *)b.mem, &rq, &res) == (uint32_t)DS_RUN_REASON_REFUSED) {
        harness_error(c, "ds_run refused the request of the cycle probe");
    }
    release_boot(&b);
    count = (uint32_t)sink->header[0] | ((uint32_t)sink->header[1] << 8u) | ((uint32_t)sink->header[2] << 16u) | ((uint32_t)sink->header[3] << 24u);
    if (!c->harness && (sink->header[4] != 0u || sink->header[5] != 0u || sink->header[6] != 0u || sink->header[7] != 0u)) {
        harness_error(c, "the instruction sink of the cycle probe lost a record (lost != 0): the trace would be incomplete");
    }
    for (i = 0u; i < nitems && !c->harness; i++) {
        char what[120];
        uint32_t idx = cursor;
        int found = 0;
        uint64_t delta = 0u;
        uint32_t per = items[i].high ? CP_MCLK_HIGH : CP_MCLK_LOW;
        uint32_t rec_speed = 0u;

        while (idx + 1u < count) {
            const uint8_t *r = records + (size_t)idx * DS_INSN_RECORD_BYTES;

            if (cp_get_u16(r + 8) == items[i].pc && r[10] == (uint8_t)items[i].opcode) {
                const uint8_t *nx = r + DS_INSN_RECORD_BYTES;

                found = 1;
                delta = cp_get_u64(nx) - cp_get_u64(r);
                rec_speed = r[16];
                break;
            }
            idx++;
        }
        snprintf(what, sizeof what, "item %u (pc 0x%04X opcode 0x%02X) is not in the trace after the previous item, or has no record after it", i + 1u, items[i].pc, items[i].opcode);
        check(c, found, what);
        if (!found) {
            continue;
        }
        cursor = idx + 1u;
        matched++;
        snprintf(what, sizeof what, "item %u (pc 0x%04X opcode 0x%02X): the speed byte is %u, the item states %s", i + 1u, items[i].pc, items[i].opcode, (unsigned)rec_speed, items[i].high ? "H" : "L");
        check(c, rec_speed == (items[i].high ? 1u : 0u), what);
        snprintf(what, sizeof what, "item %u (pc 0x%04X opcode 0x%02X): %llu master clocks to the next instruction, the page literal is %u cycles of %u", i + 1u, items[i].pc, items[i].opcode,
                 (unsigned long long)delta, items[i].cycles, per);
        check(c, delta == (uint64_t)items[i].cycles * per, what);
    }
    free(sink);
    free(records);
    note_run(c, res.machine_time, res.reason);
    snprintf(got, gotcap, "items=%u matched=%u", nitems, matched);
}

/* ---- timer_pure (plan 03-21; D-109, ER-3, A1) ---- */

#define TP_STEP (3u * 1024u)       /* master clocks per prescaler step: OSC1 / 3 / 1024 (H8-20, H8-21), written here as the manual's factors, not read from the core */
#define TP_PERIODS 3u              /* periods sampled after a start */
#define TP_FREQ_HZ 21480000u       /* the 21.48 MHz of H8-21 */

/* the naive reference model: it counts master clocks one at a time and shares no formula with timer.c */
struct tp_model {
    unsigned enabled;
    unsigned counter;
    unsigned reload;
    unsigned prescale; /* clocks since the last step */
    unsigned tiq;      /* the sticky request bit: set by any borrow, cleared by an acknowledge */
    uint64_t borrows;  /* borrows since the start */
    uint64_t now;
};

/* the model moves to stamp t one master clock at a time; the working copies are locals so that the loop is cheap in a debug build (the clock loop is what the case spends its time in) */
static void tp_to(struct tp_model *m, uint64_t t)
{
    uint64_t now = m->now;
    unsigned prescale = m->prescale;
    unsigned counter = m->counter;
    unsigned tiq = m->tiq;
    uint64_t borrows = m->borrows;

    if (m->enabled == 0u) { /* a stopped timer does nothing per clock: only the time moves */
        if (now < t) {
            m->now = t;
        }
        return;
    }
    while (now < t) {
        now++;
        prescale++;
        if (prescale == TP_STEP) {
            prescale = 0u;
            if (counter == 0u) { /* a step with the counter at 0 borrows and reloads */
                borrows++;
                tiq = 1u;
                counter = m->reload;
            } else {
                counter--;
            }
        }
    }
    m->now = now;
    m->prescale = prescale;
    m->counter = counter;
    m->tiq = tiq;
    m->borrows = borrows;
}

struct tp_stat {
    unsigned long compared;
    unsigned long bad;
    char first[160];
};

static void tp_bad(struct tp_stat *st, const char *what, unsigned reload, uint64_t t)
{
    if (st->bad == 0u) {
        snprintf(st->first, sizeof st->first, "%s (reload %u, stamp %llu)", what, reload, (unsigned long long)t);
    }
    st->bad++;
}

/* compare the core with the model at stamp t: the counter by both read addresses, the request bit through the controller, and with with_borrows the borrows since the stored origin */
static void tp_cmp(struct ds_instance *inst, struct tp_model *m, uint64_t t, int with_borrows, int settle, struct tp_stat *st, const char *what)
{
    uint8_t cv = 0u;
    uint64_t bv = 0u;

    tp_to(m, t);
    if (settle != 0) {
        ds_timer_settle(inst, t);
    }
    ds_timer_query(inst, t, &cv, &bv);
    st->compared++;
    if ((unsigned)cv != m->counter || (unsigned)ds_timer_read(inst, 0u, t) != m->counter || (unsigned)ds_timer_read(inst, 1u, t) != m->counter) {
        tp_bad(st, what, m->reload, t);
    }
    if (ds_timer_tiq(inst, t) != m->tiq || ds_irq_read(inst, 3u, t) != (m->tiq != 0u ? 4u : 0u)) {
        tp_bad(st, "the request bit differs", m->reload, t);
    }
    if (with_borrows != 0 && bv != m->borrows) {
        tp_bad(st, "the borrow count differs", m->reload, t);
    }
}

static void tp_start(struct ds_instance *inst, struct tp_model *m, uint64_t t, unsigned reload)
{
    ds_timer_write(inst, 1u, 0u, t); /* stop (a no-op when stopped), set the reload, clear a request left by the previous sequence, start */
    tp_to(m, t);
    m->enabled = 0u;
    ds_timer_write(inst, 0u, (uint8_t)reload, t);
    m->reload = reload;
    ds_irq_write(inst, 3u, 0u, t);
    m->tiq = 0u;
    ds_timer_write(inst, 1u, 1u, t);
    m->enabled = 1u;
    m->counter = reload;
    m->prescale = 0u;
    m->borrows = 0u;
}

/* one start and three periods of samples (before, at and after every step boundary, and mid-step); returns the stamp of the last sample */
static uint64_t tp_pure(struct ds_instance *inst, struct tp_model *m, uint64_t t0, unsigned reload, int settle, struct tp_stat *st)
{
    uint64_t steps = (uint64_t)TP_PERIODS * (uint64_t)(reload + 1u) + 2u;
    uint64_t b;
    uint64_t last = t0;

    int wb = settle == 0; /* after a settle the borrows are folded into the request bit and the origin moves, so the count since the origin is compared only for the pure queries */

    tp_start(inst, m, t0, reload);
    tp_cmp(inst, m, t0, wb, settle, st, "the counter differs right after the start");
    for (b = 1u; b <= steps; b++) {
        uint64_t edge = t0 + b * (uint64_t)TP_STEP;

        tp_cmp(inst, m, edge - (uint64_t)TP_STEP / 2u, wb, settle, st, "the counter differs in mid-step");
        tp_cmp(inst, m, edge - 1u, wb, settle, st, "the counter differs one clock before a step");
        tp_cmp(inst, m, edge, wb, settle, st, "the counter differs at a step");
        tp_cmp(inst, m, edge + 1u, wb, settle, st, "the counter differs one clock after a step");
        last = edge + 1u;
    }
    return last;
}

/* the first and second borrow fall (reload + 1) and 2 * (reload + 1) steps after the start: literals written here from the period of H8-21 (A1), compared with the core's borrow count directly */
static void tp_period(struct ds_instance *inst, uint64_t t0, unsigned reload, struct tp_stat *st)
{
    uint64_t period = ((uint64_t)reload + 1u) * (uint64_t)TP_STEP;
    uint8_t cv = 0u;
    uint64_t bv = 0u;
    uint64_t t[4];
    uint64_t want[4];
    unsigned i;

    ds_timer_write(inst, 1u, 0u, t0);
    ds_timer_write(inst, 0u, (uint8_t)reload, t0);
    ds_timer_write(inst, 1u, 1u, t0);
    t[0] = t0 + period - 1u;
    want[0] = 0u;
    t[1] = t0 + period;
    want[1] = 1u;
    t[2] = t0 + 2u * period - 1u;
    want[2] = 1u;
    t[3] = t0 + 2u * period;
    want[3] = 2u;
    for (i = 0u; i < 4u; i++) {
        ds_timer_query(inst, t[i], &cv, &bv);
        st->compared++;
        if (bv != want[i]) {
            tp_bad(st, "a borrow does not fall (reload + 1) steps after the previous one", reload, t[i]);
        }
    }
    ds_timer_query(inst, t0 + period, &cv, NULL);
    st->compared++;
    if ((unsigned)cv != reload) { /* the counter holds the reload value right after a borrow */
        tp_bad(st, "the counter is not the reload value right after a borrow", reload, t0 + period);
    }
    ds_timer_write(inst, 1u, 0u, t0 + 2u * period + 1u); /* leave the timer stopped for the next sequence (the borrows folded into the latch are cleared by tp_start) */
}

/* the edges: a reload write while running, a stop that holds, no-op rewrites, a restart that resets the prescaler phase and an acknowledge; times are chosen off the step grid */
static void tp_edges(struct ds_instance *inst, struct tp_model *m, uint64_t t0, unsigned reload, struct tp_stat *st)
{
    uint64_t t = tp_pure(inst, m, t0, reload, 0, st);
    unsigned r2 = (reload * 7u + 3u) & 0x7Fu;
    uint64_t tm = t + 7u;
    uint64_t ts;
    uint64_t tr;
    uint64_t tn;
    unsigned k;

    tp_to(m, tm);
    ds_timer_write(inst, 0u, (uint8_t)r2, tm); /* reload write while running: only the next reload changes */
    m->reload = r2;
    tp_cmp(inst, m, tm, 0, 0, st, "a reload write while running changed the counter");
    for (k = 1u; k <= 2u * (r2 + 1u) + 2u; k++) {
        tp_cmp(inst, m, tm + (uint64_t)k * (uint64_t)TP_STEP - 1u, 0, 0, st, "after a reload write the counter differs one clock before a step");
        tp_cmp(inst, m, tm + (uint64_t)k * (uint64_t)TP_STEP, 0, 0, st, "after a reload write the counter differs at a step");
    }
    tm = tm + (uint64_t)(2u * (r2 + 1u) + 2u) * (uint64_t)TP_STEP;
    ts = tm + 3u * (uint64_t)TP_STEP / 2u + 3u; /* a stop in mid-step */
    tp_to(m, ts);
    ds_timer_write(inst, 1u, 0u, ts);
    m->enabled = 0u;
    tp_cmp(inst, m, ts, 0, 0, st, "the counter differs at the stop");
    tp_cmp(inst, m, ts + 1u, 0, 0, st, "a stopped timer does not hold its counter");
    tp_cmp(inst, m, ts + 123457u, 0, 0, st, "a stopped timer does not hold its counter after a long time");
    ds_timer_write(inst, 1u, 0u, ts + 200000u); /* a write that repeats the stopped state changes nothing */
    tp_cmp(inst, m, ts + 200001u, 0, 0, st, "a write of 0 to a stopped timer changed the counter");
    ds_timer_write(inst, 0u, (uint8_t)reload, ts + 250000u); /* a reload write while stopped leaves the counter */
    m->reload = reload;
    tp_cmp(inst, m, ts + 250001u, 0, 0, st, "a reload write while stopped changed the counter");
    tr = ts + 300000u + 1111u; /* restart off the grid: the counter is loaded and the prescaler phase restarts at tr */
    tp_to(m, tr);
    ds_timer_write(inst, 1u, 1u, tr);
    m->enabled = 1u;
    m->counter = m->reload;
    m->prescale = 0u;
    tp_cmp(inst, m, tr, 0, 0, st, "a start did not load the reload value");
    for (k = 1u; k <= (reload + 1u) + 2u; k++) {
        tp_cmp(inst, m, tr + (uint64_t)k * (uint64_t)TP_STEP - 1u, 0, 0, st, "after a restart the counter differs one clock before a step");
        tp_cmp(inst, m, tr + (uint64_t)k * (uint64_t)TP_STEP, 0, 0, st, "after a restart the counter differs at a step");
    }
    tn = tr + (uint64_t)((reload + 1u) + 2u) * (uint64_t)TP_STEP + 10u;
    tp_to(m, tn);
    ds_timer_write(inst, 1u, 1u, tn); /* a write that repeats the running state changes nothing */
    tp_cmp(inst, m, tn + 1u, 0, 0, st, "a write of 1 to a running timer changed the counter");
    t = tn + 5u;
    tp_to(m, t);
    ds_irq_write(inst, 3u, 0u, t); /* an acknowledge clears the request ... */
    m->tiq = 0u;
    tp_cmp(inst, m, t, 0, 0, st, "an acknowledge did not clear the request");
    tp_cmp(inst, m, t + 1u, 0, 0, st, "the request came back by itself after an acknowledge");
    for (k = 1u; k <= 2u * (reload + 1u) + 1u; k++) { /* ... and a later borrow sets it again */
        tp_cmp(inst, m, t + (uint64_t)k * (uint64_t)TP_STEP, 0, 0, st, "after an acknowledge the counter or the request differs at a step");
    }
    ds_timer_write(inst, 1u, 0u, t + (uint64_t)(2u * (reload + 1u) + 2u) * (uint64_t)TP_STEP);
    m->enabled = 0u;
}

void kind_timer_pure(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    static const uint64_t phase[3] = {0u, 1u, 3071u};
    char rom[48];
    struct booted b;
    const struct ds_instance *ci = NULL;
    struct ds_instance *inst;
    struct tp_model m;
    struct tp_stat st_pure;
    struct tp_stat st_period;
    struct tp_stat st_edges;
    struct tp_stat st_settle;
    uint64_t cursor = 8u * (uint64_t)TP_STEP;
    unsigned reload;
    unsigned ph;
    const char *err;

    if (strncmp(k->val[K_EXPECT], "rom=", 4u) != 0 || strlen(k->val[K_EXPECT] + 4) < 1u || strlen(k->val[K_EXPECT] + 4) >= sizeof rom ||
        strspn(k->val[K_EXPECT] + 4, "abcdefghijklmnopqrstuvwxyz0123456789-") != strlen(k->val[K_EXPECT] + 4)) {
        harness_error(c, "a timer_pure case needs an expect rom=NAME");
        return;
    }
    memcpy(rom, k->val[K_EXPECT] + 4, strlen(k->val[K_EXPECT] + 4) + 1u);
    err = boot_built_rom(rom, k->has[K_REQUEST] ? k->val[K_REQUEST] : "", &b, &ci);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    inst = (struct ds_instance *)(void *)b.mem;
    memset(&m, 0, sizeof m);
    memset(&st_pure, 0, sizeof st_pure);
    memset(&st_period, 0, sizeof st_period);
    memset(&st_edges, 0, sizeof st_edges);
    memset(&st_settle, 0, sizeof st_settle);
    /* the numbers of H8-21 in integers: 21.48 MHz / 3 / 1024 is 6.992 kHz, one step of the shortest interval (reload 0) is 143 usec and the longest (reload 127, 128 steps) is 18.3 msec */
    check(c, (uint64_t)TP_STEP == (uint64_t)DS_TIMER_STEP_MCLK, "the prescaler step of the core is not 3 * 1024 master clocks (OSC1 / 3 / 1024, H8-20, H8-21)");
    check(c, (uint64_t)TP_FREQ_HZ / TP_STEP == 6992u, "21.48 MHz over the step is not the 6.992 kHz of H8-21");
    check(c, (uint64_t)TP_STEP * 1000000u / TP_FREQ_HZ == 143u, "the shortest interval (reload 0, one step) is not the 143 usec of H8-21");
    check(c, 128u * (uint64_t)TP_STEP * 10000u / TP_FREQ_HZ == 183u, "the longest interval (reload 127, 128 steps) is not the 18.3 msec of H8-21");
    for (reload = 0u; reload <= 127u && !c->harness; reload++) {
        for (ph = 0u; ph < 3u; ph++) {
            uint64_t t0 = ((cursor + (uint64_t)TP_STEP - 1u) / (uint64_t)TP_STEP) * (uint64_t)TP_STEP + phase[ph];

            tp_period(inst, t0, reload, &st_period);
            t0 = t0 + 2u * ((uint64_t)reload + 1u) * (uint64_t)TP_STEP + (uint64_t)TP_STEP;
            cursor = tp_pure(inst, &m, t0, reload, 0, &st_pure);
            tp_to(&m, cursor);
            ds_timer_write(inst, 1u, 0u, cursor);
            m.enabled = 0u;
            cursor = cursor + (uint64_t)TP_STEP;
        }
        if (reload == 0u || reload == 1u || reload == 127u || reload % 8u == 3u) {
            uint64_t t0 = ((cursor + (uint64_t)TP_STEP - 1u) / (uint64_t)TP_STEP) * (uint64_t)TP_STEP + 17u;

            tp_edges(inst, &m, t0, reload, &st_edges);
            cursor = m.now + 2u * (uint64_t)TP_STEP + 2000000u;
            tp_to(&m, cursor);
            t0 = ((cursor + (uint64_t)TP_STEP - 1u) / (uint64_t)TP_STEP) * (uint64_t)TP_STEP + 3071u;
            cursor = tp_pure(inst, &m, t0, reload, 1, &st_settle); /* the same samples after a settle at each of them: the folded state answers every later query alike */
            ds_timer_write(inst, 1u, 0u, cursor);
            tp_to(&m, cursor);
            m.enabled = 0u;
            cursor = cursor + (uint64_t)TP_STEP;
        }
    }
    release_boot(&b);
    check(c, st_pure.compared > 100000u && st_period.compared > 1000u && st_edges.compared > 1000u && st_settle.compared > 1000u, "a timer_pure sequence compared too few samples to count as a test");
    check(c, st_pure.bad == 0u, st_pure.first);
    check(c, st_period.bad == 0u, st_period.first);
    check(c, st_edges.bad == 0u, st_edges.first);
    check(c, st_settle.bad == 0u, st_settle.first);
    snprintf(got, gotcap, "reloads=128 phases=3 compared=%lu", st_pure.compared + st_period.compared + st_edges.compared + st_settle.compared);
}
