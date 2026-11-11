/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * inputkinds.c - the input schedule of a case and the refusal rows of ds_input_push (plan 03-27; D-51, D-116, D-117, ER-5, T-03-74 to T-03-76, F4, F5). Test code only. The pad is a test input declared by the case: E2 self-consistency, never evidence about any pad.
 *   The case key input names an input script (the file stem under tests/conformance/inputs). The runner never reads a script: scripts/conformance-flatten --inputs parsed and judged it with the one parser (D-51) and wrote the rows
 *     SCRIPT<TAB>NAME<TAB>N and INPUT<TAB>NAME<TAB>IDX<TAB>AT<TAB>PORT<TAB>MASK  (and a last END row) into the file named by --inputs, which conformance-run and determinism-run pass to every runner invocation, the toy runners included (F5).
 *     input_events reads the rows of NAME into core events; apply_input gives them to a booted instance through ds_input_push before the first step. The core re-validates every event, so a bad row is a refusal there and never undefined behaviour here.
 *   guest expect item refused=input.NAME[,NAME...] (the guest kind of guestkinds.c): the built image is booted and synthetic events, bypassing every script, are pushed directly; each variant of each named code must come back as exactly that code (input_refusals.def) with the
 *     instance byte-identical afterwards, after a valid list has been proven accepted (a core that refused everything must fail) and the codes are kept for the deferred kind input_refusal_coverage, which requires every member of the .def observed. The guest is run only for the late refusal.
 * Evidence tier E0 for the mechanism; the accepted lists are proven by reading the instance and by ds_io_read at the stamps of the events, which are the plan's own protocol literals (ASSUMED LED-IO-0002), not DuoSpool output.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "runner.h"

const char *inputs_tsv_path;

/* --inputs FILE, parsed beside --oracle (F7: runner.c gets one call): 1 when argv[*a] was this option (and *a moves to its value), otherwise 0 */
int inputkinds_arg(int argc, char **argv, int *a)
{
    if (strcmp(argv[*a], "--inputs") == 0 && *a + 1 < argc) {
        inputs_tsv_path = argv[*a + 1];
        *a += 1;
        return 1;
    }
    return 0;
}

/* the rows of the flattened input scripts, read once and kept for the life of the process */
static char *inputs_text;

/* an unsigned decimal field of at most 20 characters, 0 on success */
static int field_u64(const char *s, uint64_t *out)
{
    return strlen(s) > 0u && strlen(s) <= 20u && strspn(s, "0123456789") == strlen(s) ? parse_u64(s, out) : -1;
}

const char *input_events(const struct kase *k, struct ds_input_event *ev, unsigned cap, unsigned *n, int *declared)
{
    const char *name = k->val[K_INPUT];
    const char *p;
    unsigned want = 0u;
    unsigned count = 0u;
    int have_script = 0;
    int have_end = 0;

    *n = 0u;
    *declared = 0;
    if (!k->has[K_INPUT]) {
        return NULL;
    }
    *declared = 1;
    if (strlen(name) == 0u || strlen(name) > 40u || strspn(name, "abcdefghijklmnopqrstuvwxyz0123456789_") != strlen(name) || name[0] < 'a' || name[0] > 'z') {
        return "the input key of a case names an input script: [a-z][a-z0-9_]*";
    }
    if (inputs_text == NULL) {
        inputs_text = inputs_tsv_path != NULL ? read_file(inputs_tsv_path) : NULL;
        if (inputs_text == NULL) {
            return "no input rows file (--inputs FILE) or it cannot be read: scripts/conformance-run writes build/gate/inputs.tsv";
        }
    }
    for (p = inputs_text; *p != '\0';) {
        const char *eol = strchr(p, '\n');
        size_t len = eol != NULL ? (size_t)(eol - p) : strlen(p);
        char line[160];
        char *f[7];
        int nf;

        if (len >= sizeof line) {
            return "an input row is longer than the reader allows";
        }
        memcpy(line, p, len);
        line[len] = '\0';
        p = eol != NULL ? eol + 1 : p + len;
        if (len == 0u) {
            continue;
        }
        nf = split_tab(line, f, 7);
        if (nf == 2 && strcmp(f[0], "END") == 0) {
            have_end = 1;
        } else if (nf == 3 && strcmp(f[0], "SCRIPT") == 0 && strcmp(f[1], name) == 0) {
            uint64_t v = 0u;

            if (have_script || field_u64(f[2], &v) != 0 || v > (uint64_t)DS_INPUT_MAX) {
                return "a SCRIPT row of the input rows is repeated or has a bad event count";
            }
            have_script = 1;
            want = (unsigned)v;
        } else if (nf == 6 && strcmp(f[0], "INPUT") == 0 && strcmp(f[1], name) == 0) {
            uint64_t idx = 0u;
            uint64_t at = 0u;
            uint64_t port = 0u;
            uint64_t mask = 0u;

            if (field_u64(f[2], &idx) != 0 || field_u64(f[3], &at) != 0 || field_u64(f[4], &port) != 0 || field_u64(f[5], &mask) != 0 || port > 0xFFFFFFFFu || mask > 0xFFFFFFFFu
                || idx != (uint64_t)count + 1u) {
                return "an INPUT row is malformed or out of sequence";
            }
            if (count >= cap) {
                return "an input script holds more events than the parse buffer";
            }
            ev[count].at = at;
            ev[count].port = (uint32_t)port;
            ev[count].mask = (uint32_t)mask;
            count++;
        } else if (!((nf == 3 && strcmp(f[0], "SCRIPT") == 0) || (nf == 6 && strcmp(f[0], "INPUT") == 0))) {
            return "an input row has an unexpected shape";
        }
    }
    if (!have_end) {
        return "the input rows have no closing END row (a truncated file must never read as a smaller set)";
    }
    if (!have_script) {
        return "the input key names a script that the input rows do not hold";
    }
    if (count != want) {
        return "the INPUT rows of a script do not match its SCRIPT event count";
    }
    *n = count;
    return NULL;
}

const char *apply_input(struct ds_instance *inst, const struct kase *k, uint32_t *refusal)
{
    struct ds_input_event ev[INPUT_PARSE_MAX];
    unsigned n = 0u;
    int declared = 0;
    const char *err;

    *refusal = 0u;
    err = input_events(k, ev, INPUT_PARSE_MAX, &n, &declared);
    if (err != NULL || !declared) {
        return err;
    }
    *refusal = ds_input_push(inst, ev, (uint32_t)n);
    return NULL;
}

/* ---- the refusal rows ---- */

static unsigned char ir_observed[DS_INPUTREF_SLOTS];

/* a valid list of n events: strictly increasing times 10, 20, ..., port 1, the masks alternate RUN (8) and I with UP (0x11) */
static void ir_valid(struct ds_input_event *ev, uint32_t n)
{
    uint32_t i;

    for (i = 0u; i < n; i++) {
        ev[i].at = (uint64_t)(i + 1u) * UINT64_C(10);
        ev[i].port = 1u;
        ev[i].mask = (i & 1u) != 0u ? 0x11u : 0x08u;
    }
}

/* one call that must be refused with want: the instance (inst, live in memory of ds_instance_size() bytes, or NULL) must be byte-identical afterwards */
static void ir_refused(struct ctx *c, struct ds_instance *inst, const struct ds_input_event *ev, uint32_t count, uint32_t want, unsigned *calls)
{
    size_t size = ds_instance_size();
    unsigned char *copy = (unsigned char *)malloc(size);
    uint32_t got;

    if (copy == NULL) {
        harness_error(c, "out of memory for the instance copy");
        return;
    }
    if (inst != NULL) {
        memcpy(copy, inst, size);
    }
    got = ds_input_push(inst, ev, count);
    check(c, got == want, "ds_input_push returned a code other than the one the case names");
    check(c, ds_input_refusal_name(got) != NULL && (got == 0u || strncmp(ds_input_refusal_name(got), "input.", 6u) == 0), "an input refusal does not print as input.NAME");
    if (inst != NULL) {
        check(c, memcmp(copy, inst, size) == 0, "a refused ds_input_push changed the instance");
    }
    if (got < (uint32_t)DS_INPUTREF_SLOTS) {
        ir_observed[got] = 1u;
    }
    (*calls)++;
    free(copy);
}

/* a refusal row of one event list: n events with the one event at index bad replaced by (port, mask), the other events valid */
static void ir_one_bad(struct ctx *c, struct ds_instance *inst, uint32_t n, uint32_t bad, uint32_t port, uint32_t mask, uint32_t want, unsigned *calls)
{
    struct ds_input_event ev[4];

    ir_valid(ev, n);
    ev[bad].port = port;
    ev[bad].mask = mask;
    ir_refused(c, inst, ev, n, want, calls);
}

static int ir_name_is(const char *list, const char *name)
{
    size_t n = strlen(name);
    const char *p = list;

    while (*p != '\0') {
        const char *comma = strchr(p, ',');
        size_t len = comma != NULL ? (size_t)(comma - p) : strlen(p);

        if (len == n && strncmp(p, name, n) == 0) {
            return 1;
        }
        p = comma != NULL ? comma + 1 : p + len;
    }
    return 0;
}

/* the list names only members of input_refusals.def: 0 when every name is one, -1 otherwise */
static int ir_names_ok(const char *list)
{
    const char *p = list;

    if (strlen(list) == 0u) {
        return -1;
    }
    while (*p != '\0') {
        const char *comma = strchr(p, ',');
        size_t len = comma != NULL ? (size_t)(comma - p) : strlen(p);
        char name[24];
        char full[32];
        uint32_t i;
        int found = 0;

        if (len == 0u || len >= sizeof name) {
            return -1;
        }
        memcpy(name, p, len);
        name[len] = '\0';
        snprintf(full, sizeof full, "input.%s", name);
        for (i = 1u; i < (uint32_t)DS_INPUTREF_SLOTS; i++) {
            if (ds_input_refusal_name(i) != NULL && strcmp(ds_input_refusal_name(i), full) == 0) {
                found = 1;
            }
        }
        if (!found) {
            return -1;
        }
        p = comma != NULL ? comma + 1 : p + len;
    }
    return 0;
}

/* a fresh boot of the case's image: NULL and the instance, or why not (a fault of the case) */
static const char *ir_boot(const struct kase *k, const char *rom, struct booted *b, struct ds_instance **wr)
{
    const struct ds_instance *inst = NULL;
    const char *err = boot_built_rom(rom, k->val[K_REQUEST], b, &inst);

    *wr = err == NULL ? (struct ds_instance *)(void *)b->mem : NULL;
    return err;
}

/* the proofs that a core which refused everything would fail (on fresh boots): an empty list is accepted and may be pushed again, a valid list is accepted and copied whole with every unused slot zero and then frozen (a second push is state),
 * two events at one time on two populated ports are accepted, and the level a read sees follows the events (RUN at its own stamp, with SEL 0 and CLR 0; the stamps are the plan's own literals) */
static void ir_accepts(const struct kase *k, struct ctx *c, const char *rom)
{
    struct booted b;
    struct ds_instance *wr = NULL;
    struct ds_input_event ev[4];
    const char *err;
    uint32_t i;
    int zero_tail = 1;

    err = ir_boot(k, rom, &b, &wr);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    check(c, wr->o_latch == 0xFFu && wr->slot_type[0] == (uint8_t)DS_SLOT_PAD2 && wr->slot_type[1] == 0u && wr->slot_type[4] == 0u && wr->input_count == 0u, "a fresh instance does not hold the $FF latch, one pad on port 1 and an empty schedule");
    check(c, ds_input_push(wr, NULL, 0u) == (uint32_t)DS_INPUTREF_NONE && ds_input_push(wr, ev, 0u) == (uint32_t)DS_INPUTREF_NONE && wr->input_count == 0u, "an empty input list was not accepted twice");
    ir_valid(ev, 2u);
    check(c, ds_input_push(wr, ev, 2u) == (uint32_t)DS_INPUTREF_NONE && wr->input_count == 2u, "a valid input list before the first step was not accepted");
    for (i = 2u; i < DS_INPUT_MAX; i++) {
        zero_tail = zero_tail && wr->input_ev[i].at == 0u && wr->input_ev[i].port == 0u && wr->input_ev[i].mask == 0u;
    }
    check(c, wr->input_ev[0].at == 10u && wr->input_ev[0].port == 1u && wr->input_ev[0].mask == 0x08u && wr->input_ev[1].at == 20u && wr->input_ev[1].mask == 0x11u && zero_tail, "the accepted list was not copied whole with every unused slot zero");
    ds_io_write(wr, 0x00u); /* SEL 0, CLR 0: the buttons */
    check(c, ds_io_read(wr, 9u) == 0xFFu && ds_io_read(wr, 10u) == 0xF7u && ds_io_read(wr, 19u) == 0xF7u && ds_io_read(wr, 20u) == 0xFEu, "the buttons read at the stamps of the events are not nothing, RUN, RUN and I (d6 = d7 = 1)");
    ds_io_write(wr, 0x01u); /* SEL 1, CLR 0: the directions */
    check(c, ds_io_read(wr, 9u) == 0xFFu && ds_io_read(wr, 10u) == 0xFFu && ds_io_read(wr, 20u) == 0xFEu, "the directions read at the stamps of the events are not nothing, nothing and UP");
    ds_io_write(wr, 0x02u); /* CLR 1: the nibble is 0 whatever is held */
    check(c, ds_io_read(wr, 20u) == 0xF0u, "a read with CLR 1 is not the nibble 0");
    check(c, ds_input_push(wr, ev, 2u) == (uint32_t)DS_INPUTREF_state, "a second valid push onto a frozen schedule was not refused as state");
    release_boot(&b);

    err = ir_boot(k, rom, &b, &wr);
    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    wr->slot_type[1] = (uint8_t)DS_SLOT_PAD2; /* r1 populates port 1 only; the core's multi-port rule is shown on an instance whose second port the harness populates */
    ev[0].at = 5u;
    ev[0].port = 1u;
    ev[0].mask = 0x60u; /* RIGHT and DOWN: not opposite */
    ev[1].at = 5u;
    ev[1].port = 2u;
    ev[1].mask = 0x90u; /* UP and LEFT: not opposite */
    check(c, ds_input_push(wr, ev, 2u) == (uint32_t)DS_INPUTREF_NONE && wr->input_count == 2u, "two events at one time on two populated ports in increasing port order were not accepted");
    release_boot(&b);
}

void input_refuse(const struct kase *k, struct ctx *c, const struct guest_expect *ex, const char *rom, uint64_t until, char *got, size_t gotcap)
{
    struct booted b;
    struct ds_instance *wr = NULL;
    struct ds_input_event ev[4];
    struct ds_input_event *big;
    unsigned calls = 0u;
    const char *err;
    uint32_t i;
    static const uint32_t bad_ports[6] = {0u, 2u, 3u, 4u, 5u, 6u};
    static const uint32_t bad_masks[7] = {0x50u, 0xA0u, 0xF0u, 0x100u, 0x800u, 0x80000000u, 0xFFFFFFFFu};

    if (ir_names_ok(ex->reason) != 0) {
        harness_error(c, "a refused=input.NAME expect names a code that input_refusals.def does not have");
        return;
    }
    ir_accepts(k, c, rom);
    if (c->harness) {
        return;
    }
    if (ir_name_is(ex->reason, "port") || ir_name_is(ex->reason, "bits") || ir_name_is(ex->reason, "order") || ir_name_is(ex->reason, "queue_full")) {
        err = ir_boot(k, rom, &b, &wr);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        if (ir_name_is(ex->reason, "port")) {
            for (i = 0u; i < 6u; i++) { /* ports 2 to 5 are empty in r1 (D-116), 0 and 6 are no port, and each position of the list is judged */
                ir_one_bad(c, wr, 1u, 0u, bad_ports[i], 0x08u, (uint32_t)DS_INPUTREF_port, &calls);
                ir_one_bad(c, wr, 3u, 2u, bad_ports[i], 0x08u, (uint32_t)DS_INPUTREF_port, &calls);
            }
            ir_one_bad(c, wr, 2u, 1u, 0xFFFFFFFFu, 0x08u, (uint32_t)DS_INPUTREF_port, &calls);
            ir_one_bad(c, wr, 2u, 1u, 257u, 0x08u, (uint32_t)DS_INPUTREF_port, &calls); /* 257 is 1 in the low byte and 0x10001 is 1 in the low half: the full word is judged */
            ir_one_bad(c, wr, 2u, 1u, 0x10001u, 0x08u, (uint32_t)DS_INPUTREF_port, &calls);
        }
        if (ir_name_is(ex->reason, "bits")) {
            for (i = 0u; i < 7u; i++) {
                ir_one_bad(c, wr, 1u, 0u, 1u, bad_masks[i], (uint32_t)DS_INPUTREF_bits, &calls);
                ir_one_bad(c, wr, 3u, 2u, 1u, bad_masks[i], (uint32_t)DS_INPUTREF_bits, &calls);
            }
        }
        if (ir_name_is(ex->reason, "order")) {
            ev[0].at = 100u;
            ev[0].port = 1u;
            ev[0].mask = 0x08u;
            ev[1] = ev[0];
            ev[1].at = 99u; /* one earlier than the event before it */
            ir_refused(c, wr, ev, 2u, (uint32_t)DS_INPUTREF_order, &calls);
            ev[1].at = 100u; /* equal time, equal port: the port does not increase */
            ir_refused(c, wr, ev, 2u, (uint32_t)DS_INPUTREF_order, &calls);
            wr->slot_type[1] = (uint8_t)DS_SLOT_PAD2; /* the harness populates port 2 so that a decreasing port at one time can be tested; r1 populates port 1 only */
            ev[0].port = 2u;
            ev[1].port = 1u; /* equal time, the port goes down */
            ir_refused(c, wr, ev, 2u, (uint32_t)DS_INPUTREF_order, &calls);
            wr->slot_type[1] = 0u;
        }
        if (ir_name_is(ex->reason, "queue_full")) {
            big = (struct ds_input_event *)malloc(((size_t)DS_INPUT_MAX + 1u) * sizeof *big);
            if (big == NULL) {
                harness_error(c, "out of memory for the long input list");
                release_boot(&b);
                return;
            }
            for (i = 0u; i < (uint32_t)DS_INPUT_MAX + 1u; i++) {
                big[i].at = (uint64_t)i * UINT64_C(10);
                big[i].port = 1u;
                big[i].mask = (i & 1u) != 0u ? 0x11u : 0x08u;
            }
            check(c, ds_input_push(wr, big, (uint32_t)DS_INPUT_MAX) == (uint32_t)DS_INPUTREF_NONE && wr->input_count == (uint32_t)DS_INPUT_MAX, "a list of exactly 1024 events was not accepted");
            release_boot(&b);
            err = ir_boot(k, rom, &b, &wr);
            if (err != NULL) {
                free(big);
                harness_error(c, err);
                return;
            }
            ir_refused(c, wr, big, (uint32_t)DS_INPUT_MAX + 1u, (uint32_t)DS_INPUTREF_queue_full, &calls);
            ir_refused(c, wr, NULL, 1u, (uint32_t)DS_INPUTREF_queue_full, &calls);
            ir_refused(c, wr, big, 0xFFFFFFFFu, (uint32_t)DS_INPUTREF_queue_full, &calls);
            free(big);
        }
        release_boot(&b);
    }
    if (ir_name_is(ex->reason, "state")) {
        err = ir_boot(k, rom, &b, &wr);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        ir_valid(ev, 2u);
        ir_refused(c, NULL, ev, 2u, (uint32_t)DS_INPUTREF_state, &calls); /* an instance that is not booted */
        ir_refused(c, NULL, NULL, 0u, (uint32_t)DS_INPUTREF_state, &calls);
        check(c, ds_input_push(wr, ev, 2u) == (uint32_t)DS_INPUTREF_NONE, "the valid list before the state refusals was not accepted");
        ir_refused(c, wr, ev, 2u, (uint32_t)DS_INPUTREF_state, &calls); /* a schedule that is already frozen */
        ir_valid(ev, 1u);
        ir_refused(c, wr, ev, 1u, (uint32_t)DS_INPUTREF_state, &calls);
        ir_refused(c, wr, NULL, 0u, (uint32_t)DS_INPUTREF_state, &calls);
        release_boot(&b);
    }
    if (ir_name_is(ex->reason, "late")) {
        struct ds_run_request rq;
        struct ds_run_result res;
        uint32_t reason;

        err = ir_boot(k, rom, &b, &wr);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        memset(&rq, 0, sizeof rq);
        memset(&res, 0, sizeof res);
        rq.struct_size = (uint32_t)sizeof rq;
        rq.until_mclk = until;
        reason = ds_run(wr, &rq, &res);
        note_run(c, wr->machine_time, reason);
        check(c, reason == (uint32_t)DS_RUN_REASON_UNTIL && res.steps >= 1u && wr->machine_time != 0u, "the run before the late refusal took no step");
        ir_valid(ev, 2u);
        ir_refused(c, wr, ev, 2u, (uint32_t)DS_INPUTREF_late, &calls);
        ir_refused(c, wr, NULL, 0u, (uint32_t)DS_INPUTREF_late, &calls); /* an empty list is a push too */
        release_boot(&b);
    }
    check(c, calls >= 1u, "the refuse case made no refused call");
    snprintf(got, gotcap, "calls=%u", calls);
}

/*
 * input_refusal_coverage (DEFERRED, D-86 pattern): every member of input_refusals.def was observed by a refused=input.NAME row, the table has no gap and every name prints as input.NAME, and the limits are the 1024 events and five ports of D-116 and D-117.
 */
void kind_input_refusal_coverage(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    uint32_t n;
    unsigned members = 0u;
    unsigned seen = 0u;

    (void)k;
    for (n = 1u; n < (uint32_t)DS_INPUTREF_SLOTS; n++) {
        const char *name = ds_input_refusal_name(n);

        if (name == NULL) {
            continue;
        }
        members++;
        seen += ir_observed[n] != 0u ? 1u : 0u;
        check(c, strncmp(name, "input.", 6u) == 0, "an input refusal name does not start with input.");
        check(c, ir_observed[n] != 0u, "an input refusal code was never observed by a refused=input.NAME row");
        if (ir_observed[n] == 0u) {
            emit_err("input_refusal_coverage: %s was not observed by any refuse row\n", name);
        }
    }
    check(c, members > 0u, "input_refusals.def has no member");
    check(c, (unsigned)DS_INPUT_MAX == 1024u && (unsigned)DS_PORT_COUNT == 5u, "the input limits are not the 1024 events and five ports of D-116 and D-117");
    snprintf(got, gotcap, "members=%u observed=%u", members, seen);
}
