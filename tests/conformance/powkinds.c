/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * powkinds.c - the case kinds of power-on state (plan 12; D-50, D-75; the placeholder registers and their domains left in plan 03-36, D-91). Every expected value in a
 * case is a literal derived independently of the C code (a throwaway perl script applying the D-75 derivation with arbitrary-precision
 * integers; the method is noted in the meaning key of each case). Evidence tier E0 (repeatability and a literal): these cases show the
 * implementation agrees with the project's own written rules, and claim nothing about hardware (LED-PROF-0002).
 *   splitmix        request seed=N; expect a comma-separated list of 0x outputs of successive splitmix64 steps started from state N
 *   poweron         boots the request (fixture and request key) and checks the instance against the expect items, separated by ';':
 *                   ram=zero | ram@OFFSET=HEXBYTES | mpr=zero | time=N | policy=NAME | seed=N|none
 *                   plan 03-24 (seeded-prng-v2; D-114): a=N | x=N | y=N | s=N | p=N (the register after the boot's reset) | timer_reload=N | timer_counter=N
 *                   | mpr@INDEX=HEXBYTES (the mapping registers from INDEX on) | cpu=zero | tmr=zero (every register of the domain is 0, as under zero and v1)
 *                   | drawn=cpu|mpr|tmr (the domain is not all zero: a structural assertion that names no value; mpr means MPR0 to MPR6)
 *                   | reset=constants (the reset facts of H8-18 and H8-8 hold: I set, D and T clear, low speed, MPR7 $00, timer stopped, disable register 0, TIQ clear, and io_buffer and mpr_latch 0 under every policy)
 *                   | streams=distinct (the four domain streams of the request seed start with four different 8-byte words)
 *   poweron_domain  request seed=N; expect extra=A,B: the first 16 bytes of the ram8k stream are unchanged when another stream (the one
 *                   of a value that names no domain, whose constant is 0) draws A or B values in between, that other stream's bytes
 *                   differ from the ram8k bytes, and a boot reproduces the ram8k stream
 * (the run kinds run_until and run_chunk_sum are in stepkinds.c; the stepping that this file once covered is gone, D-91.) Test code only, never installed.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ds_core.h"
#include "ds_poweron.h"
#include "runner.h"

/* the text of a key, copied into a work buffer; 0 on success */
int pk_copy_text(const char *src, char *dst, size_t cap)
{
    if (strlen(src) >= cap) {
        return -1;
    }
    memcpy(dst, src, strlen(src) + 1u);
    return 0;
}

/* the next item of a list in place: the item, with *rest left at the following one (NULL at the end) */
char *pk_next_item(char **rest, char sep)
{
    char *item = *rest;
    char *end;

    if (item == NULL || *item == '\0') {
        return NULL;
    }
    end = strchr(item, sep);
    if (end != NULL) {
        *end = '\0';
        *rest = end + 1;
    } else {
        *rest = NULL;
    }
    return item;
}

static int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
}

/* HEX digits to bytes; the count, or -1 for an odd length, a non-digit or an overflow of cap */
static long hex_bytes(const char *h, unsigned char *out, size_t cap)
{
    size_t n = strlen(h);
    size_t i;

    if (n == 0u || n % 2u != 0u || n / 2u > cap) {
        return -1;
    }
    for (i = 0u; i < n / 2u; i++) {
        int hi = hex_digit(h[2u * i]);
        int lo = hex_digit(h[2u * i + 1u]);

        if (hi < 0 || lo < 0) {
            return -1;
        }
        out[i] = (unsigned char)(((unsigned)hi << 4) | (unsigned)lo);
    }
    return (long)(n / 2u);
}

/* ---- kind splitmix ---- */

void kind_splitmix(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct spec s;
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    const char *err = parse_spec(&s, k->val[K_REQUEST], 0u);
    uint64_t state;
    unsigned draws = 0u;

    if (err == NULL && !s.has_seed) {
        err = "a splitmix case needs seed=N";
    }
    if (err != NULL || pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        harness_error(c, err != NULL ? err : "the expect list is too long");
        return;
    }
    state = s.seed;
    while ((item = pk_next_item(&rest, ',')) != NULL) {
        uint64_t want;

        if (parse_u64(item, &want) != 0) {
            harness_error(c, "a splitmix literal is not a 0x number");
            return;
        }
        draws++;
        check(c, ds_splitmix64_next(&state) == want, "a splitmix64 output differs from the independently derived literal");
    }
    check(c, draws >= 1u, "the splitmix case lists no literal");
    snprintf(got, gotcap, "draws=%u", draws);
}

/* ---- kind poweron ---- */

static int is_all_zero(const unsigned char *p, size_t n)
{
    size_t i;

    for (i = 0u; i < n; i++) {
        if (p[i] != 0u) {
            return 0;
        }
    }
    return 1;
}

void kind_poweron(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct booted b;
    struct spec s;
    const struct ds_instance *inst = NULL;
    const char *err = boot_accepted(k->val[K_FIXTURE], k->val[K_REQUEST], &b, &s, &inst);
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;

    if (err != NULL) {
        harness_error(c, err);
        return;
    }
    if (pk_copy_text(k->val[K_EXPECT], list, sizeof list) != 0) {
        release_boot(&b);
        harness_error(c, "the expect list is too long");
        return;
    }
    snprintf(got, gotcap, "booted");
    while ((item = pk_next_item(&rest, ';')) != NULL && !c->harness) {
        uint64_t num;

        if (strcmp(item, "ram=zero") == 0) {
            check(c, is_all_zero(inst->ram8k, sizeof inst->ram8k), "the work RAM is not all zero");
        } else if (strcmp(item, "mpr=zero") == 0) {
            check(c, is_all_zero(inst->mpr, sizeof inst->mpr), "the mapping registers are not the all-zero constant");
        } else if (strncmp(item, "ram@", 4) == 0) {
            char *eq = strchr(item, '=');
            unsigned char want[64];
            size_t off;
            long n;

            if (eq == NULL) {
                harness_error(c, "a ram@ item has no =");
                break;
            }
            *eq = '\0';
            n = hex_bytes(eq + 1, want, sizeof want);
            if (parse_size(item + 4, &off) != 0 || n < 0 || off > sizeof inst->ram8k || (size_t)n > sizeof inst->ram8k - off) {
                harness_error(c, "a ram@ item is malformed or outside the 8 KiB RAM");
                break;
            }
            check(c, memcmp(inst->ram8k + off, want, (size_t)n) == 0, "the work RAM differs from the independently derived literal");
        } else if (strncmp(item, "vram@", 5) == 0 || strncmp(item, "satb@", 5) == 0 || strncmp(item, "pal@", 4) == 0) {
            const int is_vram = item[0] == 'v';
            const int is_satb = item[0] == 's';
            const unsigned char *arr = is_vram ? inst->vdc_vram : is_satb ? inst->vdc_satb : inst->vce_pal;
            const size_t arr_len = is_vram ? sizeof inst->vdc_vram : is_satb ? sizeof inst->vdc_satb : sizeof inst->vce_pal;
            char *eq = strchr(item, '=');
            unsigned char want[64];
            size_t off;
            long n;

            if (eq == NULL) {
                harness_error(c, "a vram@, satb@ or pal@ item has no =");
                break;
            }
            *eq = '\0';
            n = hex_bytes(eq + 1, want, sizeof want);
            if (parse_size(item + (item[0] == 'p' ? 4 : 5), &off) != 0 || n < 0 || off > arr_len || (size_t)n > arr_len - off) {
                harness_error(c, "a vram@, satb@ or pal@ item is malformed or outside its array");
                break;
            }
            check(c, memcmp(arr + off, want, (size_t)n) == 0, "the video array differs from the independently derived literal");
        } else if (strcmp(item, "video=zero") == 0) {
            check(c, is_all_zero(inst->vdc_vram, sizeof inst->vdc_vram) && is_all_zero(inst->vdc_satb, sizeof inst->vdc_satb) && is_all_zero(inst->vce_pal, sizeof inst->vce_pal),
                  "VRAM, SATB or the palette is not all zero");
        } else if (strcmp(item, "drawn=vram") == 0) {
            check(c, !is_all_zero(inst->vdc_vram, sizeof inst->vdc_vram), "the vram domain drew nothing: VRAM is all 0");
        } else if (strcmp(item, "drawn=satb") == 0) {
            check(c, !is_all_zero(inst->vdc_satb, sizeof inst->vdc_satb), "the satb domain drew nothing: SATB is all 0");
        } else if (strcmp(item, "drawn=pal") == 0) {
            check(c, !is_all_zero(inst->vce_pal, sizeof inst->vce_pal), "the pal domain drew nothing: the palette is all 0");
        } else if (strcmp(item, "palbits=clear") == 0) {
            size_t pi;
            unsigned bad = 0u;

            for (pi = 1u; pi < sizeof inst->vce_pal; pi += 2u) {
                bad += (inst->vce_pal[pi] & 0xFEu) != 0u ? 1u : 0u;
            }
            check(c, bad == 0u, "an odd palette byte has a bit set among bits 1 to 7");
        } else if (strcmp(item, "vid=constants") == 0) {
            unsigned regs = 0u;
            unsigned i;

            for (i = 0u; i < 20u; i++) {
                regs |= inst->vdc_reg[i];
            }
            check(c, regs == 0u && inst->vdc_rdbuf == 0u && inst->vdc_ar == 0u && inst->vdc_status == 0u && inst->vdc_wlatch == 0u, "a VDC register, the read buffer, the address register, the status or the write latch is not 0 at power-on (D-167)");
            check(c, inst->vce_cr == 0u && inst->vce_cta == 0u, "the VCE control register or colour table address is not 0 at power-on (D-167)");
            check(c, inst->stage == 0u && inst->v_state == 0u && inst->burst == 0u && inst->line_idx == 0u && inst->v_togo == 0u && inst->bg_y == 0u && inst->mwr_active == 0u, "a beam member is not 0 at power-on (D-167)");
            check(c, inst->vd_off == 0u && inst->rcr_off == 0u && inst->line_dirty == 0u && inst->sprite_pend == 0u && inst->lat_div == 0u && inst->vd_pending == 0u, "a beam event or record member is not 0 at power-on (D-167)");
            check(c, inst->satb_pending == 0u && inst->satb_done_valid == 0u && inst->satb_done_at == 0u && inst->dma_start == 0u && inst->dma_done == 0u && inst->dma_word_mclk == 0u && inst->dma_state == 0u, "a DMA member is not 0 at power-on (D-167)");
            check(c, is_all_zero(inst->vid_pad, sizeof inst->vid_pad), "the video block padding is not 0");
            check(c, inst->rcr_counter == 16384u && inst->display_k == 0xFFFFu && (inst->frame_lines == 262u || inst->frame_lines == 263u), "a boot field (rcr_counter, display_k, frame_lines) is not its constant");
        } else if (strcmp(item, "cpu=zero") == 0) {
            check(c, inst->a == 0u && inst->x == 0u && inst->y == 0u && inst->s == 0u && inst->p == 0x04u, "the CPU registers are not the zero constants (A, X, Y, S 0 and only I set in P)");
        } else if (strcmp(item, "tmr=zero") == 0) {
            check(c, inst->timer_reload == 0u && inst->timer_counter == 0u, "the timer reload or counter is not 0");
        } else if (strcmp(item, "drawn=cpu") == 0) {
            check(c, (inst->a | inst->x | inst->y | inst->s | (inst->p & 0xC3u)) != 0u, "the cpu domain drew nothing: A, X, Y, S and N V Z C are all 0");
        } else if (strcmp(item, "drawn=mpr") == 0) {
            check(c, !is_all_zero(inst->mpr, 7u), "the mpr domain drew nothing: MPR0 to MPR6 are all 0");
        } else if (strcmp(item, "drawn=tmr") == 0) {
            check(c, (inst->timer_reload | inst->timer_counter) != 0u, "the tmr domain drew nothing: the timer reload and counter are both 0");
        } else if (strcmp(item, "reset=constants") == 0) {
            check(c, (inst->p & 0x04u) != 0u, "the I flag is not set after reset (H8-18)");
            check(c, (inst->p & 0x28u) == 0u, "the D or T flag is set after reset (H8-18)");
            check(c, inst->speed == 0u, "the speed is not low after reset (H8-18)");
            check(c, inst->mpr[7] == 0u, "MPR7 is not $00 after reset (H8-8)");
            check(c, inst->timer_enabled == 0u, "the timer is not stopped after reset (H8-18)");
            check(c, inst->irq_disable == 0u && inst->tiq_latch == 0u, "the disable register or TIQ is not clear after reset (H8-18)");
            check(c, inst->io_buffer == 0u && inst->mpr_latch == 0u, "io_buffer or mpr_latch is not 0 at power-on (D-114: they have no domain)");
        } else if (strcmp(item, "streams=distinct") == 0) {
            uint64_t w[(size_t)DS_DOMAIN_COUNT];
            uint32_t d;
            uint32_t e;

            for (d = 0u; d < (uint32_t)DS_DOMAIN_COUNT; d++) {
                struct ds_stream st;

                ds_stream_init(&st, inst->powerup_seed, d);
                w[d] = ds_stream_next(&st);
            }
            for (d = 0u; d < (uint32_t)DS_DOMAIN_COUNT; d++) {
                for (e = d + 1u; e < (uint32_t)DS_DOMAIN_COUNT; e++) {
                    check(c, w[d] != w[e], "two power-on domains start with the same 64-bit word");
                }
            }
        } else if (strncmp(item, "mpr@", 4) == 0) {
            char *eq = strchr(item, '=');
            unsigned char want[8];
            size_t off;
            long n;

            if (eq == NULL) {
                harness_error(c, "an mpr@ item has no =");
                break;
            }
            *eq = '\0';
            n = hex_bytes(eq + 1, want, sizeof want);
            if (parse_size(item + 4, &off) != 0 || n < 0 || off > sizeof inst->mpr || (size_t)n > sizeof inst->mpr - off) {
                harness_error(c, "an mpr@ item is malformed or outside the eight mapping registers");
                break;
            }
            check(c, memcmp(inst->mpr + off, want, (size_t)n) == 0, "the mapping registers differ from the independently derived literal");
        } else if ((item[0] == 'a' || item[0] == 'x' || item[0] == 'y' || item[0] == 's' || item[0] == 'p') && item[1] == '=') {
            char *eq = item + 1;
            uint8_t got_reg = item[0] == 'a' ? inst->a : item[0] == 'x' ? inst->x : item[0] == 'y' ? inst->y : item[0] == 's' ? inst->s : inst->p;

            if (parse_u64(eq + 1, &num) != 0 || num > 255u) {
                harness_error(c, "a register expect item is not a byte");
                break;
            }
            check(c, got_reg == (uint8_t)num, "a CPU register differs from the independently derived literal");
        } else if (strncmp(item, "timer_reload=", 13) == 0 || strncmp(item, "timer_counter=", 14) == 0) {
            char *eq = strchr(item, '=');
            int is_reload = item[6] == 'r';

            if (parse_u64(eq + 1, &num) != 0 || num > 255u) {
                harness_error(c, "a timer expect item is not a byte");
                break;
            }
            check(c, (is_reload ? inst->timer_reload : inst->timer_counter) == (uint8_t)num, "a timer register differs from the independently derived literal");
        } else if (strncmp(item, "time=", 5) == 0) {
            char *eq = strchr(item, '=');

            if (parse_u64(eq + 1, &num) != 0) {
                harness_error(c, "a numeric expect item is not a number");
                break;
            }
            check(c, inst->machine_time == num, "machine time is not the expected value");
        } else if (strncmp(item, "policy=", 7) == 0) {
            check(c, strcmp(inst->profile.field[DS_FIELD_powerup_policy].value, item + 7) == 0,
                  "the recorded profile value of powerup_policy differs");
            check(c, inst->powerup_policy == ds_policy_of(item + 7) && inst->powerup_policy != 0u, "the recorded policy code differs");
        } else if (strncmp(item, "seed=", 5) == 0) {
            if (strcmp(item + 5, "none") == 0) {
                check(c, inst->has_seed == 0u && inst->powerup_seed == 0u, "a seedless boot recorded a seed");
            } else if (parse_u64(item + 5, &num) != 0) {
                harness_error(c, "the seed item is not a number or none");
            } else {
                check(c, inst->has_seed == 1u && inst->powerup_seed == num, "the recorded seed differs");
            }
        } else {
            harness_error(c, "unknown poweron expect item");
        }
    }
    release_boot(&b);
}

/* ---- kind poweron_domain ---- */

#define PD_BYTES 16u
/* a value that names no domain: ds_domain_const gives it the constant 0, so its stream is a second, independent stream of the same seed */
#define PD_OTHER ((uint32_t)DS_DOMAIN_COUNT)

void kind_poweron_domain(const struct kase *k, struct ctx *c, char *got, size_t gotcap)
{
    struct spec s;
    char list[PK_LIST_MAX];
    char *rest = list;
    char *item;
    const char *err = parse_spec(&s, k->val[K_REQUEST], 0u);
    unsigned char base[PD_BYTES];
    unsigned char other16[PD_BYTES];
    struct ds_stream st;
    uint64_t bare;
    unsigned runs = 0u;

    if (err == NULL && !s.has_seed) {
        err = "a poweron_domain case needs seed=N";
    }
    if (err != NULL || strncmp(k->val[K_EXPECT], "extra=", 6) != 0 || pk_copy_text(k->val[K_EXPECT] + 6, list, sizeof list) != 0) {
        harness_error(c, err != NULL ? err : "the expect key is not extra=A,B");
        return;
    }
    check(c, ds_domain_const(PD_OTHER) == UINT64_C(0), "a value that names no domain has a nonzero constant");
    ds_stream_init(&st, s.seed, (uint32_t)DS_DOMAIN_RAM8K);
    ds_stream_fill(&st, base, sizeof base);
    ds_stream_init(&st, s.seed, PD_OTHER);
    ds_stream_fill(&st, other16, sizeof other16);
    check(c, memcmp(base, other16, sizeof base) != 0, "the ram8k domain and another stream drew the same bytes");
    /* the derivation is a splitmix64 step started from (seed XOR constant): a stream that starts at the bare XOR is another stream */
    ds_stream_init(&st, s.seed, (uint32_t)DS_DOMAIN_RAM8K);
    bare = s.seed ^ ds_domain_const((uint32_t)DS_DOMAIN_RAM8K);
    check(c, st.state != bare, "the stream starts at the bare seed XOR constant: the splitmix step of D-75 is missing");

    while ((item = pk_next_item(&rest, ',')) != NULL) {
        uint64_t extra;
        struct ds_stream ram;
        struct ds_stream other;
        unsigned char mixed[PD_BYTES];
        uint64_t i;

        if (parse_u64(item, &extra) != 0) {
            harness_error(c, "an extra count is not a number");
            return;
        }
        ds_stream_init(&ram, s.seed, (uint32_t)DS_DOMAIN_RAM8K);
        ds_stream_init(&other, s.seed, PD_OTHER);
        ds_stream_fill(&ram, mixed, 8u);
        for (i = 0u; i < extra; i++) {
            (void)ds_stream_next(&other);
        }
        ds_stream_fill(&ram, mixed + 8, 8u);
        check(c, memcmp(mixed, base, sizeof base) == 0, "the ram8k bytes changed when another stream drew extra values");
        runs++;
    }
    check(c, runs >= 1u, "the poweron_domain case lists no extra count");
    {
        /* a boot with the same seed lays down exactly the ram8k stream */
        struct booted b;
        struct spec bs;
        const struct ds_instance *inst = NULL;
        char req[96];

        snprintf(req, sizeof req, "bind.powerup_policy=seeded-prng-v1;seed=%llu", (unsigned long long)s.seed);
        err = boot_accepted("plain-256k", req, &b, &bs, &inst);
        if (err != NULL) {
            harness_error(c, err);
            return;
        }
        check(c, memcmp(inst->ram8k, base, sizeof base) == 0, "a boot with this seed does not lay down the ram8k stream");
        release_boot(&b);
    }
    snprintf(got, gotcap, "runs=%u", runs);
}
