/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * boot.c - ds_boot, ds_instance_size and ds_instance_align (D-42, D-45, D-46, D-66, D-67).
 * ds_boot runs the checks of boot_order.def in order and the first refusal wins; nothing is read from the request content
 * before every check has passed (the checks look at the pointer and the length only), and a refusal writes only the result
 * struct, never caller memory. After the checks the whole instance is zeroed (so padding is deterministic), the profile is
 * recorded and the bytes are copied into the fixed ROM array: the instance never borrows the host's buffer. The content id is
 * the SHA-256 of that copy (D-47); the core never opens a file and never looks at what the bytes mean.
 * The core declares and checks; it never guesses (D-45): no byte order, header or mapper is inferred from content.
 * After the last row, and only then, one more check refuses a block that overlaps the request, the content or the result as
 * load.memory_invalid (WR-02): the instance is zeroed before the request and the content are read, so an overlap would boot from
 * zeroed or half-overwritten bytes and report a content id for them.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_core.h"
#include "ds_mapper.h"
#include "ds_poweron.h"
#include "ds_sha256.h"

#define DS_MIN_SIZE 8192u
#define DS_BYTE_ORDER_PCE 1u
#define DS_BYTE_ORDER_TG16 2u
#define DS_HEADER_NONE 1u
#define DS_HEADER_COPIER_512 2u

size_t ds_instance_size(void)
{
    return sizeof(struct ds_instance);
}

size_t ds_instance_align(void)
{
    return _Alignof(struct ds_instance);
}

/* state shared by the checks of one boot */
struct boot_ctx {
    void *mem;
    size_t cap;
    const struct ds_boot_request *req;
    struct ds_profile profile; /* the resolved profile, valid once resolved is set and resolve_code is DS_OK */
    int resolved;
    uint32_t resolve_code;
    uint32_t resolve_field;
    uint32_t field; /* the profile field named by the refusal being returned, DS_NO_FIELD otherwise */
};

/*
 * Every row after the request row guards its own use of the request, so that each row is safe wherever boot_order.def puts it
 * (a later plan swaps rows to prove that order is behaviour; a swapped row must give a wrong refusal, never a NULL read).
 */
static int req_missing(const struct boot_ctx *c)
{
    return c->req == NULL;
}

/* resolve once, on first use, so that the rows that need the resolved profile work in any order */
static void resolve_once(struct boot_ctx *c)
{
    if (!c->resolved) {
        c->resolved = 1;
        c->resolve_code = ds_resolve(c->req->profile_id, c->req->profile_rev, &c->req->bindings, &c->profile, &c->resolve_field);
    }
}

static uint32_t check_memory(struct boot_ctx *c)
{
    /* the address is tested as an integer: a misaligned cast is undefined, so the pointer is never cast before this passes */
    if (c->mem == NULL || c->cap < ds_instance_size() || ((uintptr_t)c->mem % (uintptr_t)ds_instance_align()) != 0u) {
        return DS_REFUSE_memory_invalid;
    }
    return DS_OK;
}

static uint32_t check_request(struct boot_ctx *c)
{
    if (c->req == NULL || c->req->struct_size != (uint32_t)sizeof(struct ds_boot_request) || c->req->reserved != 0u) {
        return DS_REFUSE_request_invalid;
    }
    return DS_OK;
}

static uint32_t check_profile(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    resolve_once(c);
    return c->resolve_code == DS_REFUSE_profile_unknown ? DS_REFUSE_profile_unknown : DS_OK;
}

static uint32_t check_rev(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    resolve_once(c);
    return c->resolve_code == DS_REFUSE_rev_mismatch ? DS_REFUSE_rev_mismatch : DS_OK;
}

static uint32_t check_bindings(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    resolve_once(c);
    if (c->resolve_code == DS_REFUSE_open_unbound || c->resolve_code == DS_REFUSE_bind_closed_field ||
        c->resolve_code == DS_REFUSE_bind_not_allowed) {
        c->field = c->resolve_field;
        return c->resolve_code;
    }
    return DS_OK;
}

/*
 * The power-on policy is bound in the request (D-50): seeded-prng-v1 needs a seed and zero refuses one (D-79(b)). The seed is a
 * 64-bit value, so its presence is a separate flag: seed 0 is a valid seed and is not the same as no seed.
 */
static uint32_t check_powerup(struct boot_ctx *c)
{
    uint32_t policy;

    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    resolve_once(c);
    if (c->resolve_code != DS_OK) {
        c->field = c->resolve_field;
        return c->resolve_code;
    }
    policy = ds_policy_of(c->profile.field[DS_FIELD_powerup_policy].value);
    if (policy == (uint32_t)DS_POLICY_SEEDED && c->req->has_seed == 0u) {
        c->field = DS_FIELD_powerup_policy;
        return DS_REFUSE_powerup_seed_required;
    }
    if (policy == (uint32_t)DS_POLICY_ZERO && c->req->has_seed != 0u) {
        c->field = DS_FIELD_powerup_policy;
        return DS_REFUSE_powerup_seed_unexpected;
    }
    return DS_OK;
}

static uint32_t check_mapper(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    if (c->req->mapper == NULL) {
        return DS_REFUSE_mapper_undeclared;
    }
    /* plain is the only mapper; sf2, populous, tennokoe, arcade_card, mcgenjin and every other name are refused, not guessed */
    return ds_streq(c->req->mapper, "plain") ? DS_OK : DS_REFUSE_mapper_unsupported;
}

/* the request vocabulary of a bound profile value (D-67): pce -> 1, tg16 -> 2, 0 for anything else */
static uint32_t byte_order_of(const char *value)
{
    if (ds_streq(value, "pce")) {
        return DS_BYTE_ORDER_PCE;
    }
    return ds_streq(value, "tg16") ? DS_BYTE_ORDER_TG16 : 0u;
}

static uint32_t header_of(const char *value)
{
    if (ds_streq(value, "refuse")) {
        return DS_HEADER_NONE;
    }
    return ds_streq(value, "declared-512") ? DS_HEADER_COPIER_512 : 0u;
}

static uint32_t check_byte_order(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    resolve_once(c);
    if (c->req->byte_order == 0u) {
        return DS_REFUSE_byte_order_undeclared;
    }
    if (c->resolve_code != DS_OK) {
        c->field = c->resolve_field;
        return c->resolve_code;
    }
    /* a declaration that differs from what the resolved profile binds is a content/profile mismatch (D-67) */
    if (c->req->byte_order != byte_order_of(c->profile.field[DS_FIELD_content_byte_order].value)) {
        c->field = DS_FIELD_content_byte_order;
        return DS_REFUSE_profile_mismatch;
    }
    return c->req->byte_order == DS_BYTE_ORDER_TG16 ? DS_REFUSE_byte_order_unsupported : DS_OK;
}

static uint32_t check_header(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    resolve_once(c);
    if (c->req->header == 0u) {
        return DS_REFUSE_header_undeclared;
    }
    if (c->resolve_code != DS_OK) {
        c->field = c->resolve_field;
        return c->resolve_code;
    }
    if (c->req->header != header_of(c->profile.field[DS_FIELD_content_copier_header].value)) {
        c->field = DS_FIELD_content_copier_header;
        return DS_REFUSE_profile_mismatch;
    }
    return c->req->header == DS_HEADER_COPIER_512 ? DS_REFUSE_header_unsupported : DS_OK;
}

static uint32_t check_buffer_null(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    return c->req->content == NULL && c->req->len > 0u ? DS_REFUSE_buffer_null : DS_OK;
}

static uint32_t check_empty(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    return c->req->len == 0u ? DS_REFUSE_empty : DS_OK;
}

static uint32_t check_cap(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    /* compared as a size_t before any read or arithmetic on the length */
    return c->req->len > (size_t)DS_ROM_CAP ? DS_REFUSE_too_large : DS_OK;
}

static uint32_t check_short(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    return c->req->len < (size_t)DS_MIN_SIZE ? DS_REFUSE_too_short : DS_OK;
}

static uint32_t check_bank_multiple(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    return c->req->len % (size_t)DS_MIN_SIZE != 0u ? DS_REFUSE_size_not_bank_multiple : DS_OK;
}

static uint32_t check_supported_size(struct boot_ctx *c)
{
    if (req_missing(c)) {
        return DS_REFUSE_request_invalid;
    }
    /* supported means a member of the size enum (ds_size_classify); no raw size is compared here */
    return ds_size_classify(c->req->len) == (uint32_t)DS_SIZE_NONE ? DS_REFUSE_size_unsupported : DS_OK;
}

static const char ds_mapper_plain[] = "plain";

/*
 * Do [a, a + an) and [b, b + bn) share a byte? Tested on integer addresses with differences only, so no sum can wrap; an empty range
 * shares nothing. WR-02: ds_boot zeroes the instance before it reads the request and the content, so caller memory that overlaps either
 * (or the result, which is written last) would be destroyed or garbled while it is still needed.
 */
static int ranges_overlap(uintptr_t a, size_t an, uintptr_t b, size_t bn)
{
    if (an == 0u || bn == 0u) {
        return 0;
    }
    if (a <= b) {
        return (b - a) < (uintptr_t)an;
    }
    return (a - b) < (uintptr_t)bn;
}

/*
 * The last check of a boot, run only when every row of boot_order.def has passed (so it changes the outcome of no input that any row
 * refuses, and the precedence of the rows is untouched): the memory the instance will occupy must not overlap the request struct, the
 * content bytes or the result struct. An overlap is the same fault as an unusable block, load.memory_invalid. Nothing is read here but
 * pointers and lengths.
 */
static int memory_overlaps(const struct boot_ctx *c, const struct ds_boot_result *result)
{
    uintptr_t m = (uintptr_t)c->mem;
    size_t isize = sizeof(struct ds_instance);

    return ranges_overlap(m, isize, (uintptr_t)c->req, sizeof *c->req) ||
           ranges_overlap(m, isize, (uintptr_t)c->req->content, c->req->len) ||
           (result != NULL && ranges_overlap(m, isize, (uintptr_t)result, sizeof *result));
}

typedef uint32_t (*boot_check_fn)(struct boot_ctx *c);

static const struct {
    const char *name;
    boot_check_fn fn;
} boot_order[] = {
#define DS_CHECK(name) {#name, check_##name},
#include "boot_order.def"
#undef DS_CHECK
};

static uint32_t finish(struct ds_boot_result *result, uint32_t reason, uint32_t field, struct ds_instance *instance)
{
    if (result != NULL) {
        result->reason = reason;
        result->field_index = field;
        result->instance = instance;
    }
    return reason;
}

uint32_t ds_boot(void *mem, size_t cap, const struct ds_boot_request *request, struct ds_boot_result *result)
{
    struct boot_ctx ctx;
    struct ds_instance *inst;
    size_t i;

    ctx.mem = mem;
    ctx.cap = cap;
    ctx.req = request;
    ctx.resolved = 0;
    ctx.resolve_code = DS_OK;
    ctx.resolve_field = DS_NO_FIELD;
    ctx.field = DS_NO_FIELD;
    for (i = 0u; i < sizeof boot_order / sizeof boot_order[0]; i++) {
        uint32_t reason = boot_order[i].fn(&ctx);
        if (reason != DS_OK) {
            return finish(result, reason, ctx.field, NULL);
        }
    }

    if (memory_overlaps(&ctx, result)) {
        return finish(result, DS_REFUSE_memory_invalid, DS_NO_FIELD, NULL);
    }

    __builtin_memset(mem, 0, sizeof(struct ds_instance));
    inst = (struct ds_instance *)mem;
    inst->profile = ctx.profile;
    for (i = 0u; i < sizeof ds_mapper_plain; i++) {
        inst->mapper[i] = ds_mapper_plain[i];
    }
    inst->byte_order = request->byte_order;
    inst->header = request->header;
    inst->size_class = ds_size_classify(request->len);
    inst->powerup_policy = ds_policy_of(ctx.profile.field[DS_FIELD_powerup_policy].value);
    inst->has_seed = request->has_seed != 0u ? 1u : 0u;
    inst->powerup_seed = inst->has_seed != 0u ? request->powerup_seed : UINT64_C(0);
    ds_poweron_apply(inst);
    __builtin_memcpy(inst->rom, request->content, request->len);
    /* content identity (D-47): SHA-256 of the copied bytes exactly as supplied, taken only after every check has passed */
    ds_sha256(inst->rom, request->len, inst->content_id);
    return finish(result, DS_OK, DS_NO_FIELD, inst);
}
