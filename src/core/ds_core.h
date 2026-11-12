/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * ds_core.h - the private header of the freestanding core (D-40; ds_profile.h is folded in). It is reached only through the
 * internal include directory src/core: there is no public duospool.h and no install rule (D-05).
 * The core never allocates and never opens a file (D-42): the instance lives in caller memory, and the ROM bytes are copied into
 * a fixed 1 MiB array inside it. Only stddef.h, stdint.h and stdbool.h may ever be included here (D-54).
 */
#ifndef DS_CORE_H
#define DS_CORE_H

#include <stddef.h>
#include <stdint.h>

/* the allocation cap is structural: the ROM array of the instance is exactly this big (D-42, D-46) */
#define DS_ROM_CAP 1048576u
#define DS_PROFILE_ID_MAX 32u
#define DS_FIELD_VALUE_MAX 30u
#define DS_MAPPER_NAME_MAX 16u
#define DS_CONTENT_ID_BYTES 32u
/* behaviour epoch (D-53): bumped by any change that can alter what a run observes; goldens compare it, never a build id */
#define DS_BEHAVIOUR_EPOCH 5u
/* the version of the power-on policy table that the record prints next to the policy name is ds_policy_version() of ds_poweron.h (D-50, D-134): 1 for zero and seeded-prng-v1, 2 for seeded-prng-v2, 3 for seeded-prng-v3 */
/* field_index of a result that names no profile field */
#define DS_NO_FIELD 0xffffffffu
/* power-on state (D-50): the 8 KiB work RAM and the 8 mapping registers */
#define DS_RAM8K_BYTES 8192u
#define DS_MPR_COUNT 8u
/* the byte arrays of the VCE and VDC state (plan 04-03; D-147, D-153): the 512 palette entries of 9 bits stored as 1,024 bytes, the 32,768 words of VRAM and the 256 words of the SATB */
#define DS_PAL_BYTES 1024u
#define DS_VRAM_BYTES 65536u
#define DS_SATB_BYTES 512u

/* ---- refusals (refusals.def) ---- */

enum ds_refusal {
    DS_OK = 0,
#define DS_REFUSAL(name, number) DS_REFUSE_##name = number,
#include "refusals.def"
#undef DS_REFUSAL
    DS_REFUSAL_SLOTS = 32 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "load.NAME" for a refusal number, "ok" for 0, NULL for a number that is not a member */
const char *ds_refusal_name(uint32_t number);

/* ---- profile (profile_fields.def) ---- */

enum ds_field_index {
#define DS_FIELD(name, kind, value, allowed, ledger) DS_FIELD_##name,
#include "profile_fields.def"
#undef DS_FIELD
    DS_FIELD_COUNT
};

enum ds_field_status {
    DS_STATUS_OWNER = 1, /* owner-selected constant */
    DS_STATUS_OPEN = 2   /* open: no value until a run request binds it */
};

enum ds_bound_by {
    DS_BOUND_NONE = 0,       /* an open field that nothing has bound */
    DS_BOUND_CONSTANT = 1,   /* an owner-selected constant of the profile */
    DS_BOUND_RUN_REQUEST = 2 /* an open field bound by the run request: no support claim (D-49) */
};

/* initial bound_by of a field row, selected by its kind column */
enum { DS_BOUNDINIT_OWNER = DS_BOUND_CONSTANT, DS_BOUNDINIT_OPEN = DS_BOUND_NONE };

struct ds_field {
    uint8_t status;
    uint8_t bound_by;
    char value[DS_FIELD_VALUE_MAX];
};

struct ds_profile {
    char id[DS_PROFILE_ID_MAX];
    uint32_t rev;
    struct ds_field field[DS_FIELD_COUNT];
};

/* the profile has no padding, so its size is the sum of its parts */
_Static_assert(sizeof(struct ds_field) == 2u + DS_FIELD_VALUE_MAX, "ds_field has padding");
_Static_assert(sizeof(struct ds_profile) == DS_PROFILE_ID_MAX + sizeof(uint32_t) + DS_FIELD_COUNT * sizeof(struct ds_field),
               "ds_profile has padding");

/* one value per field, NULL = unbound; a binding is text and is checked against the field's allowed set */
struct ds_bindings {
    const char *value[DS_FIELD_COUNT];
};

const char *ds_field_name(uint32_t field);    /* NULL for an index at or above DS_FIELD_COUNT */
const char *ds_field_allowed(uint32_t field); /* values separated by '|', empty when the profile lists none */
const char *ds_field_ledger(uint32_t field);  /* ledger ids separated by '|', empty when none; NULL for an index at or above DS_FIELD_COUNT */

/* the registered r1 constant for id and rev, NULL when either does not match */
const struct ds_profile *ds_profile_builtin(const char *id, uint32_t rev);

/*
 * The unresolved behaviours of the built-in profile: ledger ids, in sorted order, the root key unresolved_behaviours of r1.toml.
 * They are part of the profile identity text of a later plan; label text never is.
 */
uint32_t ds_profile_unresolved_count(void);
const char *ds_profile_unresolved(uint32_t index); /* NULL for an index at or above the count */

/*
 * Resolve profile id and rev against the bindings. Returns DS_OK and fills out (when not NULL), or a refusal; field_index (when
 * not NULL) is the offending field or DS_NO_FIELD. Nothing but field_index is written on a refusal.
 */
uint32_t ds_resolve(const char *id, uint32_t rev, const struct ds_bindings *bindings, struct ds_profile *out,
                    uint32_t *field_index);

/* ---- instance and boot ---- */

/* the most events of the declared IRQ1 and IRQ2 stimulus (D-112) */
#define DS_IRQ_STIM_MAX 16u

/* the input schedule (plan 03-27, io.c; D-116, D-117): at most DS_INPUT_MAX events, one pad per port, ports numbered 1 to DS_PORT_COUNT (the five ports of the multitap-capable $1000 interface, of which r1 populates port 1 only) */
#define DS_INPUT_MAX 1024u
#define DS_PORT_COUNT 5u

/*
 * One event of the input schedule: from machine time at (master clocks) the pad on port (1-based) holds exactly the buttons of mask (an absolute level, not an edge). The mask is in the CD BIOS layout (the manual's EX_JOYSNS byte, CDBIOS-p28):
 * bit 0 I (ONE), 1 II (TWO), 2 SELECT, 3 RUN, 4 UP, 5 RIGHT, 6 DOWN, 7 LEFT, 1 = pressed. Bits 8 and above are reserved (III to VI of later pad kinds) and refused for the two-button pad.
 */
struct ds_input_event {
    uint64_t at;
    uint32_t port;
    uint32_t mask;
};
_Static_assert(sizeof(struct ds_input_event) == 16u, "struct ds_input_event has padding");

struct ds_instance {
    uint64_t machine_time; /* master clocks of OSC1, 0 at boot, advanced only by ds_run (D-90) */
    struct ds_profile profile; /* the resolved profile of this run */
    uint8_t content_id[DS_CONTENT_ID_BYTES];
    char mapper[DS_MAPPER_NAME_MAX];
    uint32_t size_class; /* enum ds_size of ds_mapper.h: the image size is kept as an enum, never as a raw length */
    uint32_t byte_order; /* 1 pce, 2 tg16 */
    uint32_t header;     /* 1 none, 2 copier_512 */
    /* power-on state (D-50, D-75, D-91): the work RAM and the mapping registers; none of it is a hardware claim (LED-PROF-0002) */
    uint8_t ram8k[DS_RAM8K_BYTES];
    uint8_t mpr[DS_MPR_COUNT]; /* MPR0 to MPR6 are 0 under zero and seeded-prng-v1 and drawn under seeded-prng-v2, MPR7 is $00 at reset under every policy (D-114) */
    uint32_t powerup_policy;   /* enum ds_policy of ds_poweron.h: 1 zero, 2 seeded-prng-v1, 3 seeded-prng-v2, 4 seeded-prng-v3 */
    uint32_t has_seed;         /* 1 when the request carried a seed */
    uint64_t powerup_seed;     /* the seed of the run, 0 when has_seed is 0 */
    /* the declared IRQ1 and IRQ2 test stimulus (plan 03-22, irq.c; D-112, ER-5): up to DS_IRQ_STIM_MAX events, slot i holding the machine time at which line irq_stim_line[i] (1 or 2, 0 = an unused slot) takes the level irq_stim_level[i] (0 or 1). Written only by ds_irq_stim_set before the first step; the level of a line at a stamp is a pure function of these arrays. A test hook, never a claim about the VDC. The three arrays are one live span of 160 bytes (region IRQS) and the 8-byte times come first so that the span sits on the 8-byte boundary after the power-up seed */
    uint64_t irq_stim_at[DS_IRQ_STIM_MAX];
    uint8_t irq_stim_line[DS_IRQ_STIM_MAX];
    uint8_t irq_stim_level[DS_IRQ_STIM_MAX];
    /* the input schedule (plan 03-27, io.c; D-116, D-117): an immutable copy of the declared events, in non-decreasing time order (equal times in strictly increasing port order), copied in by ds_input_push before the first step.
     * Slot i below input_count is a declared event and every other slot is zero, so the schedule is a function of the declared list alone. The level of a port at a stamp is the mask of its last event at or before the stamp,
     * a pure function of these members: there is no cursor, so no chunking of a run can change what a read sees. input_reserved is always 0 and keeps the timer block on its 8-byte boundary */
    struct ds_input_event input_ev[DS_INPUT_MAX];
    uint32_t input_count;
    uint32_t input_reserved;
    /* the timer (plan 03-21, timer.c; D-109): timer_origin is the master-clock stamp of the last prescaler step boundary the state was folded to, or of the start write; timer_counter is the 7-bit counter value at timer_origin, timer_reload the 7-bit reload register and timer_enabled the start bit. All four are region TIMR of the digest (plan 03-22); the counter and the reload register are what the power-on policy made them, the start bit is 0 at reset (H8-18) */
    uint64_t timer_origin;
    uint8_t timer_counter;
    uint8_t timer_reload;
    uint8_t timer_enabled;
    /* the interrupt controller (plans 03-21 and 03-22, irq.c; D-110, D-111): irq_disable holds the three bits of the disable register $1402 (1 = disabled, bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD; 0 at reset, H8-16), tiq_latch the TIQ request bit (0 at reset, H8-15) and irq_i_sample the selected value of the model parameter irq_i_sample (an index into its allowed list, 0 = pre = the default; set only by ds_model_set before the first step: the row of model_params.def). The three are region IRQC */
    uint8_t irq_disable;
    uint8_t tiq_latch;
    uint8_t irq_i_sample;
    /* HuC6280 state (plan 03-01, D-102): eight bytes, no padding, no pointers, no caches. p holds N V T D I Z C in their own bit positions; B is never stored */
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t p;
    uint8_t speed; /* 0 low (the power-up speed), 1 high (D-90, D-114) */
    /* bus state (plan 03-16, region BUS0 of the digest; D-104, D-105): io_buffer is the last byte that crossed the data bus in a read or write of the windows $0800-$17FF of the hardware page, mpr_latch the byte of TAM and TMA (plan 03-17); both are constant 0 at power-on under every policy. tma_zero is the selected value of the model parameter tma_zero (plan 03-17, model_params.def): an index into its allowed list, 0 = bus_latch = the default, so a zeroed instance holds the default; set only by ds_model_set before the first step */
    uint8_t io_buffer;
    uint8_t mpr_latch;
    uint8_t tma_zero;
    /* the block transfer in progress (plan 03-25, cpu.c; D-92, D-100): blk_active is 1 between the prologue step and the epilogue step of TII, TDD, TIN, TIA or TAI and 0 otherwise; blk_opcode is that instruction's opcode ($73, $C3, $D3, $E3, $F3, else 0), blk_toggle the alternation state of TIA and TAI (0: the next byte adds one, 1: it subtracts one), blk_remaining the bytes still to move (0 to 65536: the length operand 0 is 65536 and is turned into it by the prologue; 0 while active means the epilogue is next) and blk_src and blk_dst the 16-bit source and destination addresses (the hidden SH:X and DH:Y registers of H8-6). Every field is 0 when no transfer is active. The eleven bytes are region BLKT */
    uint8_t blk_active;
    uint8_t blk_opcode;
    uint8_t blk_toggle;
    uint32_t blk_remaining;
    uint16_t blk_src;
    uint16_t blk_dst;
    /* the I/O port (plan 03-27, io.c; D-115, D-116): o_latch is the byte last written to the $1000 window (the latch of port O, $FF at power-on under every policy: the port drives a high level at reset, MAN:H8-18 and H8-19),
     * whose bit 0 is SEL and bit 1 CLR; slot_type[p] is the kind of device on port p + 1 (enum ds_slot_type, 0 none, 1 two-button pad), set at boot from the profile and never changed by a run */
    uint8_t o_latch;
    uint8_t slot_type[DS_PORT_COUNT];
    uint8_t rom[DS_ROM_CAP];
    /* explicit tail padding (plan 03-30; D-122): the struct is 8-byte aligned and the ROM array ends 6 bytes short of the next multiple of 8, so the padding is a member of its own and the tiling list below can name every byte of the instance. Zero from the zeroing of ds_boot, in no region of the stream */
    uint8_t tail_pad[6];
    /*
     * The VCE and VDC state of Phase 4 (plan 04-03; D-165, D-166, D-167, F-10), appended after tail_pad so that the line above stays byte for byte. The block starts at an 8-aligned offset and is declared region by region (VDC0, BEAM, VDMA, VCE0, then the three byte arrays), each region one contiguous live span carried whole by its payload.
     * The members of a region are ordered so that none needs padding: the one-byte members stand where they restore the alignment of the next member (the stream order of serial.c is another order, written field by field). Every byte is 0 at boot under every policy except rcr_counter (a constant), frame_lines (the bound lines_per_frame) and display_k (0xFFFF), and, under seeded-prng-v3 only, the palette, VRAM and SATB bytes (poweron.c).
     * This plan stores the state and carries it through the digest and the snapshot; the behaviour that reads and writes it lands in the plans that follow.
     */
    /* VDC0, 45 bytes: the 20 register slots R00 to R13 (R03 and R04 stay 0; the masks of D-146 are judged on restore), the read buffer, the address register, the status flags and the write latch */
    uint16_t vdc_reg[20];
    uint16_t vdc_rdbuf;
    uint8_t vdc_ar;
    uint8_t vdc_status;
    uint8_t vdc_wlatch;
    /*
     * BEAM, 33 bytes (D-138 to D-143): stage 0 EV_LINE next, 1 EV_VD next, 2 EV_RCR next; v_state 0 VDS, 1 VDW, 2 VCR, 3 VSW with v_togo lines left in it (at most 512, F-1); burst the burst latch; line_idx the next line whose EV_LINE is pending; display_k the display line (0xFFFF outside VDW); rcr_counter the RCR counter (16384 at power-on); frame_lines the bound lines_per_frame; bg_y the BYR counter; mwr_active the MWR value
     * latched at VBlank entry; vd_off and rcr_off the event offsets in master clocks (0xFFFF none); line_dirty and sprite_pend the record flags; lat_div the divider latched at EV_LINE (0 only before line 0); vd_pending the VD event is outstanding; vd_irq_phase and rcr_origin the selections of the two model rows of model_params.def (flat members named as the rows: model.c writes inst->name, F-10)
     */
    uint8_t stage;
    uint8_t v_state;
    uint8_t burst;
    uint64_t line_idx;
    uint16_t v_togo;
    uint16_t display_k;
    uint16_t rcr_counter;
    uint16_t frame_lines;
    uint16_t bg_y;
    uint16_t mwr_active;
    uint16_t vd_off;
    uint16_t rcr_off;
    uint8_t line_dirty;
    uint8_t sprite_pend;
    uint8_t lat_div;
    uint8_t vd_pending;
    uint8_t vd_irq_phase;
    uint8_t rcr_origin;
    /* VDMA, 25 bytes: the SATB copy (pending flag; the DS completion time and its validity) and the VRAM-to-VRAM transfer (dma_state 0 idle, 1 pending, 2 running; the start time, the master clocks per word and the words done) */
    uint8_t satb_pending;
    uint8_t satb_done_valid;
    uint64_t satb_done_at;
    uint64_t dma_start;
    uint32_t dma_done;
    uint16_t dma_word_mclk;
    uint8_t dma_state;
    /* VCE0, 3 bytes: the control register and the colour table address (9 bits) */
    uint8_t vce_cr;
    uint16_t vce_cta;
    /* PALT 1,024 bytes (entry i is byte 2i | byte 2i+1 << 8), VRAM 65,536 bytes (word w is byte 2w | byte 2w+1 << 8) and SATB 512 bytes: byte arrays so that the live span equals the stream payload on every host (D-147) */
    uint8_t vce_pal[DS_PAL_BYTES];
    uint8_t vdc_vram[DS_VRAM_BYTES];
    uint8_t vdc_satb[DS_SATB_BYTES];
    /* explicit trailing padding of the Phase 4 block (the block is 67,178 bytes, which is 2 modulo 8): in no region of the stream, zero from the zeroing of ds_boot, an identity member of the restore evidence like tail_pad */
    uint8_t vid_pad[6];
};

/* the stimulus arrays, the timer block, the controller bytes, the CPU block, the bus block and the block transfer state have no padding and sit in the order of the declarations (the digest regions IRQS, TIMR, IRQC, CPU0, BUS0 and BLKT are each one live span, serial.c): the stimulus times follow the power-up seed, and the ROM array follows the block transfer state */
_Static_assert(offsetof(struct ds_instance, irq_stim_at) == offsetof(struct ds_instance, powerup_seed) + sizeof(uint64_t), "padding before the stimulus times");
_Static_assert(offsetof(struct ds_instance, irq_stim_line) == offsetof(struct ds_instance, irq_stim_at) + DS_IRQ_STIM_MAX * sizeof(uint64_t), "padding between the stimulus times and lines");
_Static_assert(offsetof(struct ds_instance, irq_stim_level) == offsetof(struct ds_instance, irq_stim_line) + DS_IRQ_STIM_MAX, "padding between the stimulus lines and levels");
_Static_assert(offsetof(struct ds_instance, input_ev) == offsetof(struct ds_instance, irq_stim_level) + DS_IRQ_STIM_MAX, "padding before the input schedule");
_Static_assert(offsetof(struct ds_instance, input_count) == offsetof(struct ds_instance, input_ev) + DS_INPUT_MAX * sizeof(struct ds_input_event), "padding between the input events and their count");
_Static_assert(offsetof(struct ds_instance, input_reserved) == offsetof(struct ds_instance, input_count) + sizeof(uint32_t), "padding between the input count and the reserved word");
_Static_assert(offsetof(struct ds_instance, timer_origin) == offsetof(struct ds_instance, input_reserved) + sizeof(uint32_t), "padding before the timer origin");
_Static_assert(offsetof(struct ds_instance, timer_counter) == offsetof(struct ds_instance, timer_origin) + sizeof(uint64_t), "padding after the timer origin");
_Static_assert(offsetof(struct ds_instance, timer_reload) == offsetof(struct ds_instance, timer_counter) + 1u, "padding in the timer bytes");
_Static_assert(offsetof(struct ds_instance, timer_enabled) == offsetof(struct ds_instance, timer_reload) + 1u, "padding in the timer bytes");
_Static_assert(offsetof(struct ds_instance, irq_disable) == offsetof(struct ds_instance, timer_enabled) + 1u, "padding between the timer and the controller bytes");
_Static_assert(offsetof(struct ds_instance, tiq_latch) == offsetof(struct ds_instance, irq_disable) + 1u, "padding in the controller bytes");
_Static_assert(offsetof(struct ds_instance, irq_i_sample) == offsetof(struct ds_instance, tiq_latch) + 1u, "padding in the controller bytes");
_Static_assert(offsetof(struct ds_instance, pc) == offsetof(struct ds_instance, irq_i_sample) + 1u, "padding between the controller bytes and the CPU state");
_Static_assert(offsetof(struct ds_instance, io_buffer) == offsetof(struct ds_instance, speed) + 1u, "padding between the CPU state and io_buffer");
_Static_assert(offsetof(struct ds_instance, mpr_latch) == offsetof(struct ds_instance, io_buffer) + 1u, "padding between io_buffer and mpr_latch");
_Static_assert(offsetof(struct ds_instance, tma_zero) == offsetof(struct ds_instance, mpr_latch) + 1u, "padding between mpr_latch and tma_zero");
_Static_assert(offsetof(struct ds_instance, blk_active) == offsetof(struct ds_instance, tma_zero) + 1u, "padding between the bus bytes and the block transfer state");
_Static_assert(offsetof(struct ds_instance, blk_opcode) == offsetof(struct ds_instance, blk_active) + 1u, "padding in the block transfer bytes");
_Static_assert(offsetof(struct ds_instance, blk_toggle) == offsetof(struct ds_instance, blk_opcode) + 1u, "padding in the block transfer bytes");
_Static_assert(offsetof(struct ds_instance, blk_remaining) == offsetof(struct ds_instance, blk_toggle) + 1u, "padding before the block transfer length");
_Static_assert(offsetof(struct ds_instance, blk_src) == offsetof(struct ds_instance, blk_remaining) + sizeof(uint32_t), "padding after the block transfer length");
_Static_assert(offsetof(struct ds_instance, blk_dst) == offsetof(struct ds_instance, blk_src) + sizeof(uint16_t), "padding between the block transfer addresses");
_Static_assert(offsetof(struct ds_instance, o_latch) == offsetof(struct ds_instance, blk_dst) + sizeof(uint16_t), "padding between the block transfer state and the I/O port bytes");
_Static_assert(offsetof(struct ds_instance, slot_type) == offsetof(struct ds_instance, o_latch) + 1u, "padding between o_latch and slot_type");
_Static_assert(offsetof(struct ds_instance, rom) == offsetof(struct ds_instance, slot_type) + DS_PORT_COUNT, "padding between the I/O port bytes and the ROM");

/*
 * The tiling list (plan 03-30; D-122, Pattern 8): EVERY member of struct ds_instance, in declaration order, written by hand and independent of the serializer's region table (serial.c). The sum of the member sizes must equal sizeof(struct ds_instance), so a member
 * added to the struct without an entry here (or an entry that outlives its member) breaks the build, and implicit padding cannot hide a byte (the one gap, the tail, is the explicit member tail_pad). The restore evidence of the runner (snapkinds.c) walks this list to
 * poison every restorable member, to compare the whole instance except the ROM and to check that the members follow one another without a gap. Adding a member means adding it here AND giving it a region in serial.c (or deciding that it is derived at boot, like mapper).
 */
#define DS_INSTANCE_TILING(X) \
    X(machine_time) X(profile) X(content_id) X(mapper) X(size_class) X(byte_order) X(header) X(ram8k) X(mpr) \
    X(powerup_policy) X(has_seed) X(powerup_seed) X(irq_stim_at) X(irq_stim_line) X(irq_stim_level) \
    X(input_ev) X(input_count) X(input_reserved) X(timer_origin) X(timer_counter) X(timer_reload) X(timer_enabled) \
    X(irq_disable) X(tiq_latch) X(irq_i_sample) X(pc) X(a) X(x) X(y) X(s) X(p) X(speed) \
    X(io_buffer) X(mpr_latch) X(tma_zero) X(blk_active) X(blk_opcode) X(blk_toggle) X(blk_remaining) X(blk_src) X(blk_dst) \
    X(o_latch) X(slot_type) X(rom) X(tail_pad) \
    X(vdc_reg) X(vdc_rdbuf) X(vdc_ar) X(vdc_status) X(vdc_wlatch) \
    X(stage) X(v_state) X(burst) X(line_idx) X(v_togo) X(display_k) X(rcr_counter) X(frame_lines) X(bg_y) X(mwr_active) X(vd_off) X(rcr_off) \
    X(line_dirty) X(sprite_pend) X(lat_div) X(vd_pending) X(vd_irq_phase) X(rcr_origin) \
    X(satb_pending) X(satb_done_valid) X(satb_done_at) X(dma_start) X(dma_done) X(dma_word_mclk) X(dma_state) \
    X(vce_cr) X(vce_cta) X(vce_pal) X(vdc_vram) X(vdc_satb) X(vid_pad)
#define DS_TILING_SIZE(m) +sizeof(((struct ds_instance *)0)->m)
_Static_assert(0u DS_INSTANCE_TILING(DS_TILING_SIZE) == sizeof(struct ds_instance), "the tiling list does not account for every byte of struct ds_instance: a member without an entry, an entry without a member, or implicit padding");
_Static_assert(offsetof(struct ds_instance, tail_pad) == offsetof(struct ds_instance, rom) + DS_ROM_CAP, "padding between the ROM and the tail padding");
_Static_assert(offsetof(struct ds_instance, vdc_reg) == offsetof(struct ds_instance, tail_pad) + 6u && offsetof(struct ds_instance, vdc_reg) % 8u == 0u, "the Phase 4 block does not follow tail_pad at an 8-aligned offset");
_Static_assert(offsetof(struct ds_instance, vid_pad) + sizeof(((struct ds_instance *)0)->vid_pad) == sizeof(struct ds_instance), "vid_pad is not the last member of the instance");

struct ds_boot_request {
    uint32_t struct_size; /* sizeof(struct ds_boot_request) */
    uint32_t reserved;    /* must be 0 */
    const char *profile_id;
    uint32_t profile_rev;
    struct ds_bindings bindings;
    const char *mapper;  /* NULL = undeclared */
    uint32_t byte_order; /* 0 undeclared, 1 pce, 2 tg16 */
    uint32_t header;     /* 0 undeclared, 1 none, 2 copier_512 */
    const uint8_t *content;
    size_t len;
    uint64_t powerup_seed; /* read only when has_seed is nonzero: required by both seeded policies, refused under zero (D-50, D-79(b)) */
    uint32_t has_seed;
};

struct ds_boot_result {
    uint32_t reason;      /* a refusal number, 0 = OK */
    uint32_t field_index; /* the offending profile field or DS_NO_FIELD */
    struct ds_instance *instance;
};

size_t ds_instance_size(void);
size_t ds_instance_align(void);

/*
 * Boot a machine in caller memory (mem, cap) from a request, or refuse. The return value is the refusal number and is also
 * written to result (when result is not NULL). A refusal writes nothing but result: never mem. There is no instance to mutate
 * and no allocator, so there is no "created but unloaded" state. The block (mem, ds_instance_size() bytes) must not overlap the
 * request struct, the content bytes or the result struct: when every other check has passed and one does, the boot is refused as
 * load.memory_invalid (WR-02), because the instance is zeroed before the request and the content are read.
 */
uint32_t ds_boot(void *mem, size_t cap, const struct ds_boot_request *request, struct ds_boot_result *result);

/* ---- machine time, CPU and bounded run (cpu.c, run.c; plan 03-01, D-90..D-95) ---- */

/*
 * The named modelling constants (constants.def, plan 03-18): one enumerator DS_CONST_name per row, so the value the run record prints is the value the core reads. A constant is one value with a ledger id, never a model parameter.
 */
enum ds_constant {
#define DS_CONST(name, value, ledger) DS_CONST_##name = value,
#include "constants.def"
#undef DS_CONST
    DS_CONST_END_OF_LIST
};

/* one CPU cycle is 12 master clocks at the power-up low speed and 3 at high speed (constants.def, D-90); the interrupt entry is DS_INT_ENTRY_CYCLES cycles (8, assumed, D-98, D-99) */
#define DS_MCLK_PER_CYCLE_LOW ((uint32_t)DS_CONST_cycle_mclk_low)
#define DS_MCLK_PER_CYCLE_HIGH ((uint32_t)DS_CONST_cycle_mclk_high)
#define DS_INT_ENTRY_CYCLES ((uint32_t)DS_CONST_interrupt_entry_cycles) /* constants.def (ASSUMED LED-CPU-0009) */
/* the named cycle adders of D-98 and D-107, applied on top of the opcode table's base counts (ds_max_step_mclk adds them to the table maximum) */
#define DS_ADDER_BRANCH_TAKEN 2u /* a taken conditional branch, BBR and BBS included (S8-20); BRA and BSR are fixed counts in the table */
#define DS_ADDER_T_MODE 3u       /* ADC, AND, EOR, ORA with the T flag set (S8-17) */
#define DS_ADDER_DECIMAL 1u      /* ADC and SBC with the D flag set (S8-17) */
#define DS_ADDER_VDC_ACCESS ((uint32_t)DS_CONST_vdc_vce_penalty_cycles) /* constants.def: one CPU cycle, at the speed the instruction started at, per bus access to physical $1FE000-$1FE7FF (the VDC and VCE windows), zero-page-mapped, block and ST accesses included; charged in the bus before the device sees it (D-107, ASSUMED LED-CPU-0017) */
/* the value an unmapped read returns (D-104) */
#define DS_UNMAPPED_READ ((uint32_t)DS_CONST_unmapped_read) /* constants.def */

typedef int (*ds_stop_fn)(const struct ds_instance *inst, void *ctx);

enum ds_run_reason { DS_RUN_REASON_UNTIL = 1, DS_RUN_REASON_CALLBACK = 2, DS_RUN_REASON_REFUSED = 3 };
enum ds_stop_point { DS_STOP_INSTRUCTION = 1, DS_STOP_BLOCK_BYTE = 2 /* the run ended inside a block transfer: the prologue or a byte step was the last step and the epilogue is still to come (plan 03-25, D-92) */ };

/* the refusal codes of a run request (run_refusals.def, plan 03-35; F4): their own set and prefix, never part of refusals.def */
enum ds_run_refusal {
    DS_RUNREF_NONE = 0,
#define DS_RUNREF(name, number) DS_RUNREF_##name = number,
#include "run_refusals.def"
#undef DS_RUNREF
    DS_RUNREF_SLOTS = 16 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "run.NAME" for a run refusal number, "ok" for DS_RUNREF_NONE, NULL for a number that is not a member */
const char *ds_run_refusal_name(uint32_t number);

/* the instruction sink, the device-register sink and the video sink of ds_obs.h (plans 03-10, 03-19 and 04-02) and the running log of one ds_run call, whose layouts live there */
struct ds_insn_sink;
struct ds_dev_sink;
struct ds_vid_sink;
struct ds_obs_log;

/* the run request: an absolute deadline in master clocks, never a budget (D-93); insn is the caller-owned instruction sink, dev the caller-owned device-register sink and vid the caller-owned video sink, appended last in plan 04-02 (NULL = none, ds_obs.h) */
struct ds_run_request {
    uint32_t struct_size; /* sizeof(struct ds_run_request) */
    uint32_t reserved;    /* must be 0 */
    uint64_t until_mclk;
    ds_stop_fn stop; /* NULL never stops; checked before the first step and after every step */
    void *stop_ctx;
    struct ds_insn_sink *insn;
    struct ds_dev_sink *dev;
    struct ds_vid_sink *vid;
};

struct ds_run_result {
    uint32_t reason;     /* enum ds_run_reason */
    uint32_t refusal;    /* enum ds_run_refusal, DS_RUNREF_NONE unless the reason is REFUSED */
    uint32_t stop_point; /* enum ds_stop_point */
    uint32_t pad;        /* always 0 */
    uint64_t machine_time;
    uint64_t overshoot; /* machine_time - until_mclk when this call took a step that ended past the deadline, else 0 (a call with 0 steps has none) */
    uint64_t steps;
};

/*
 * Run the machine in whole instructions until machine time reaches until_mclk (UNTIL) or the stop callback holds (CALLBACK). A deadline at or
 * below the current machine time returns UNTIL with zero steps. Stops only fall between steps, so the end time can pass the deadline by less
 * than ds_max_step_mclk(). No state lives outside the instance between steps: run(N) then run(M) equals run(M) from the start (D-92).
 * A NULL instance or request, a wrong struct_size, a nonzero reserved, a sink with a capacity above DS_SINK_MAX_RECORDS or DS_VID_SINK_MAX_BYTES (sink_unsupported), a sink that overlaps the call's own memory, another sink or itself or is misaligned (memory_invalid) and a deadline that would wrap machine time are
 * refused with REFUSED and a refusal code; the instance is then unchanged. Returns the reason, also written to result when it is not NULL.
 */
uint32_t ds_run(struct ds_instance *inst, const struct ds_run_request *request, struct ds_run_result *result);

/* the longest single step in master clocks: the longest opcode of the table with the adders that can apply to it, plus the interrupt entry, at the low-speed cycle length, scanned from the table, never a guess (D-94) */
uint64_t ds_max_step_mclk(void);

/* the CPU core (cpu.c): reset through the vector at logical $FFFE/$FFFF with MPR7 = $00 at machine_time 0, and one instruction per step */
void ds_cpu_reset(struct ds_instance *inst);
uint32_t ds_cpu_step(struct ds_instance *inst); /* executes one instruction and returns the master clocks it took (ds_cpu_step_obs of ds_obs.h adds the observer hook) */
int ds_cpu_op_defined(uint32_t opcode); /* 1 when the opcode row of the table is a defined instruction, 0 for the 22 undefined rows (opcode is masked to 8 bits); the table is checked against the manual transcription by scripts/opcode-check */
uint32_t ds_cpu_op_max_cycles(uint32_t opcode); /* the longest one opcode can take in CPU cycles: its table count plus every adder that can apply, the VDC and VCE access penalty included (a bus access occupies at least one cycle, so an instruction makes at most as many accesses as its table count); opcode is masked to 8 bits */
void ds_op_meta(uint32_t opcode, uint32_t *bytes, uint32_t *cycles); /* the table's instruction length and base cycle count of one opcode (a block transfer holds its 17; an undefined row holds its assumed 1 and 2, LED-CPU-0010), for the manual comparison of the opcode_table kind (plan 03-15); opcode is masked to 8 bits */

/*
 * The bus choke point (bus.c, plan 03-16; D-102 to D-108): every CPU access translates through the MPRs per access, phys = (mpr[a >> 13] << 13) | (a & $1FFF), and dispatches through a const bank_class[256]
 * table. One struct ds_bus lives in a local of the step and carries what one instruction's accesses add to it (the VDC and VCE penalty cycles); a NULL bus is accepted (the reset reads, a test).
 * Reads change state (io_buffer, D-104), so the instance is not const here, and no observer ever reads through the bus (D-108).
 */
struct ds_bus {
    uint32_t penalty;        /* CPU cycles added by accesses to physical $1FE000-$1FE7FF during this step (D-107): DS_ADDER_VDC_ACCESS per access */
    struct ds_obs_log *obs;  /* the observer log of the running ds_run call or NULL (plan 03-19): the hardware-page accesses call the dev hook through it, a hook over a const instance that cannot change the machine (D-119) */
    /* the access stamp (plan 03-21; D-92, D-108): the master-clock stamp of an access is start + (last + penalty) * cycle_mclk, the last cycle of the instruction (a single data access) with the penalty cycles charged so far, and one cycle earlier for the read of a read-modify-write instruction (rmw). The rule is one assumption of this project (LED-CPU-0004, no page states it) */
    uint64_t start;          /* machine time at the start of the step (or of the interrupt entry) */
    uint32_t cycle_mclk;     /* master clocks per CPU cycle at the speed the step started at (D-101) */
    uint32_t last;           /* zero-based index of the last cycle of the instruction before the penalty: its table cycles plus the flag adders, minus one */
    uint32_t rmw;            /* 1 for a read-modify-write instruction, whose read is one cycle before the last */
    uint32_t pc;             /* the 16-bit address the dev records carry (WR-02): the opcode address for an instruction and for every step of a block transfer; for an interrupt entry the next address, which the entry pushes */
};

/* the class of a bank (bus.c, plan 03-20; D-103, ASSUMED LED-CPU-0011): the value of the const bank_class[256] table the bus dispatches on, for the independent bank-class oracle of the runner; a bank above 255 gives DS_BANK_CLASS_NONE */
enum ds_bank_class { DS_BANK_CLASS_UNMAPPED = 0, DS_BANK_CLASS_HUCARD = 1, DS_BANK_CLASS_RAM = 2, DS_BANK_CLASS_HWPAGE = 3, DS_BANK_CLASS_NONE = 4 };
uint32_t ds_bank_class(uint32_t bank);

uint8_t ds_bus_read(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr);
void ds_bus_write(struct ds_instance *inst, struct ds_bus *bus, uint16_t addr, uint8_t value);
void ds_mpr_write(struct ds_instance *inst, uint32_t index, uint8_t value); /* the only writer of mpr[] */

/*
 * The interrupt controller (irq.c, plans 03-21 and 03-22; D-110, D-111, D-112): the disable register at $1402 and the request register at $1403 of the hardware page, the three sources and the sample.
 * ds_irq_read returns the three register bits of the register (reg 2 the disable bits, bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD; reg 3 the request bits, bit 0 the IRQ2 level, bit 1 the IRQ1 level, bit 2 TIQ) at the stamp and changes nothing;
 * ds_irq_write stores the disable bits (reg 2) or acknowledges TIQ (reg 3: any write clears the TIQ request only, the IRQ levels are inputs) after folding the timer to the stamp.
 * ds_irq_pending_query is the pure query of what is pending at the stamp, after the disable mask: bit 0 IRQ2, bit 1 IRQ1, bit 2 TIQ (the layout of the request register), 0 when nothing is deliverable. ds_irq_pending is the pure sample:
 * 0 when nothing is pending, else the logical address of the vector's low byte of the winner (TIQ $FFFA, then IRQ1 $FFF8, then IRQ2 $FFF6, H8-13); the caller applies the I flag.
 */
uint32_t ds_irq_read(const struct ds_instance *inst, uint32_t reg, uint64_t stamp);
void ds_irq_write(struct ds_instance *inst, uint32_t reg, uint8_t value, uint64_t stamp);
uint32_t ds_irq_pending_query(const struct ds_instance *inst, uint64_t stamp);
uint32_t ds_irq_pending(const struct ds_instance *inst, uint64_t stamp);

/*
 * The declared IRQ1 and IRQ2 test stimulus (irq.c, plan 03-22; D-112, T-03-60, ER-5): a list of events (at, line, level), copied into the instance by ds_irq_stim_set before the first step. A line is 0 (low, not requested) before its first event and
 * after an event holds that event's level (the last event of the line whose time is at or before the stamp, ties inclusive); the levels are a pure function of the stamp. It is a stimulus declared by a case, never a claim about the VDC or any hardware
 * source. The call refuses with a code of stim_refusals.def and then writes nothing (the instance is byte-identical): count above DS_IRQ_STIM_MAX (or a NULL list with a nonzero count), a line other than 1 or 2, a level other than 0 or 1,
 * a time that decreases, and, last, a machine that has taken a step (machine time not 0) or a NULL instance. A count of 0 declares no stimulus.
 */
struct ds_irq_event {
    uint64_t at;    /* master clocks of machine time */
    uint32_t line;  /* 1 = IRQ1, 2 = IRQ2 */
    uint32_t level; /* 1 = requested (the pin is low), 0 = not requested */
};
_Static_assert(sizeof(struct ds_irq_event) == 16u, "struct ds_irq_event has padding");

enum ds_stim_refusal {
    DS_STIMREF_NONE = 0,
#define DS_STIMREF(name, number) DS_STIMREF_##name = number,
#include "stim_refusals.def"
#undef DS_STIMREF
    DS_STIMREF_SLOTS = 8 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "stim.NAME" for a stimulus refusal number, "ok" for DS_STIMREF_NONE, NULL for a number that is not a member */
const char *ds_stim_refusal_name(uint32_t number);
uint32_t ds_irq_stim_set(struct ds_instance *inst, const struct ds_irq_event *events, uint32_t count);

/*
 * The I/O port and the input schedule (io.c, plan 03-27; D-115, D-116, D-117; ASSUMED LED-IO-0002 to LED-IO-0005, E2 self-consistency only, ER-5): the $1000 window of the hardware page. A write stores its byte in o_latch (bit 0 SEL, bit 1 CLR). A read is a pure function of o_latch,
 * the schedule and the stamp: $30 | d6 << 6 | d7 << 7 | nibble, d6 and d7 the values the run request bound to io_1000_d6_value and io_1000_d7_value (LED-IO-0001), the nibble 0 while CLR is 1, else the inverted directions while SEL is 1 (D0 UP, D1 RIGHT, D2 DOWN, D3 LEFT) or the
 * inverted buttons while SEL is 0 (D0 I, D1 II, D2 SELECT, D3 RUN), a pressed button reading 0, of the pad on port 1 at the stamp. A read changes nothing here (the bus latch is the bus's, bus.c). There is no settling time: the level a read sees is the level at its own stamp.
 * ds_input_push copies a declared schedule into the instance before the first step. It refuses with a code of input_refusals.def and then writes nothing (the instance is byte-identical): count above DS_INPUT_MAX (or a NULL list with a nonzero count), an event whose port is 0, above
 * DS_PORT_COUNT or an empty port (port), an event whose mask has a bit above bit 7 or presses opposite directions (bits), times that decrease or an equal time whose port does not increase (order), and, last, a NULL instance or an instance that already holds a schedule (state)
 * and a machine that has taken a step (late). A count of 0 is accepted and declares no input. The schedule is immutable once it holds an event: it is never edited in place.
 */
enum ds_slot_type { DS_SLOT_NONE = 0, DS_SLOT_PAD2 = 1 };

enum ds_input_refusal {
    DS_INPUTREF_NONE = 0,
#define DS_INPUTREF(name, number) DS_INPUTREF_##name = number,
#include "input_refusals.def"
#undef DS_INPUTREF
    DS_INPUTREF_SLOTS = 8 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "input.NAME" for an input refusal number, "ok" for DS_INPUTREF_NONE, NULL for a number that is not a member */
const char *ds_input_refusal_name(uint32_t number);
uint32_t ds_input_push(struct ds_instance *inst, const struct ds_input_event *events, uint32_t count);
void ds_io_poweron(struct ds_instance *inst);
uint8_t ds_io_read(const struct ds_instance *inst, uint64_t stamp);
void ds_io_write(struct ds_instance *inst, uint8_t value);

void ds_bus_write_phys(struct ds_instance *inst, struct ds_bus *bus, uint32_t phys, uint8_t value); /* the physical write of ST0 to ST2 (D-106): $1FE000 | n for n in 0, 2, 3, the MPRs are never consulted; any other address is dropped */

/* ---- model parameters (model_params.def, model_refusals.def, model.c; plan 03-17; D-105, D-113) ---- */

enum ds_model_index {
#define DS_MODEL(name, allowed, def, ledger, status) DS_MODEL_##name,
#include "model_params.def"
#undef DS_MODEL
    DS_MODEL_COUNT
};
_Static_assert((unsigned)DS_MODEL_COUNT <= 4u, "model_params.def has more than the 4 rows of D-113");

/* the values of tma_zero in the order of its allowed list: the stored selection is the index */
enum { DS_TMA_ZERO_BUS_LATCH = 0, DS_TMA_ZERO_A_UNCHANGED = 1 };

/* the values of irq_i_sample in the order of its allowed list: the stored selection is the index (cpu.c, ds_cpu_step_obs) */
enum { DS_IRQ_I_SAMPLE_PRE = 0, DS_IRQ_I_SAMPLE_POST = 1 };

/* the refusal codes of ds_model_set (model_refusals.def; F4): their own set and prefix, never part of refusals.def or run_refusals.def */
enum ds_model_refusal {
    DS_MODELREF_NONE = 0,
#define DS_MODELREF(name, number) DS_MODELREF_##name = number,
#include "model_refusals.def"
#undef DS_MODELREF
    DS_MODELREF_SLOTS = 8 /* the name table has this many slots: a number at or above it fails to compile */
};

/* "model.NAME" for a model refusal number, "ok" for DS_MODELREF_NONE, NULL for a number that is not a member */
const char *ds_model_refusal_name(uint32_t number);

/* the rows of model_params.def; NULL for an index at or above DS_MODEL_COUNT */
const char *ds_model_name(uint32_t index);
const char *ds_model_allowed(uint32_t index); /* values separated by '|', the first is the default */
const char *ds_model_default(uint32_t index);
const char *ds_model_ledger(uint32_t index);
const char *ds_model_status(uint32_t index);

/* the value of row index that the instance holds: a pointer to its token inside the allowed list (the token is NOT NUL-terminated: *len is its length); NULL for a NULL instance or len, an index at or above DS_MODEL_COUNT or a stored selection outside the list */
const char *ds_model_value(const struct ds_instance *inst, uint32_t index, size_t *len);

/*
 * Select a value of a model parameter by name. Returns DS_MODELREF_NONE and stores the selection, or a refusal and writes nothing (the instance is byte-identical): unknown_name, value_not_allowed, state. Both texts are untrusted:
 * the name and the value must equal a row and one of its allowed values exactly. The state check is the last: a parameter is set before the first step (machine time 0) and is refused afterwards.
 */
uint32_t ds_model_set(struct ds_instance *inst, const char *name, const char *value);

/* NUL-terminated string equality without string.h; both arguments must be non-NULL */
static inline int ds_streq(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

#endif
