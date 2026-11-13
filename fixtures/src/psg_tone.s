; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; psg_tone.s - the PSG tone fixture, DSV id 27 (plan 05-04, grown by 05-06; D-183, D-187, D-191, D-194, D-196, D-206..D-208, D-211, D-212, D-227, RF-12, RF-13, PR-5). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), so the PSG registers are at logical $0800 to $0809 (fixtures/lib/psg.inc). The PSG is write only (H8-24): the guest cannot read what it plays, so every judgement of the sound is the runner's,
; from the DSPE and DSPA streams against the hand literals of fixtures/src/psg_tone.aexp (ER-21); the DSV1 checks here judge only the guest's own table data (GUEST).
; Sequence: (a) no PSG write for at least 2,000 master clocks after reset (the power-on silence scene, ASSUMED LED-PROF-0003); (b) for s = 0..5: R0 = s, R4 = $00 (every channel chON 0, DDA 0, AL 0); (c) the select-0 tracer scene: R0 $00, R4 $40 (DDA 1: the wave index resets,
; P8-5 procedure (2)), R4 $1F (chON 0, DDA 0, AL $1F: channel 0 then sits at attenuation sum 0 with only chON off, so the silence before key-on rests on chON alone), TIN of the 32-word table to R6 ($0806), R2 $00, R3 $01 (F = $100 = 256), R1 $FF, R5 $FF;
; (d) a counted delay of at least 20,000 master clocks; (e) R4 $9F (key-on: chON 1, DDA 0, AL $1F); (f) a counted delay of at least 400,000 master clocks; (g) R4 $1F (key-off); (h) the GUEST checks of the table and the DSV1 verdict.
; The table (RESEARCH section 6): 32 distinct words, a permutation of 0..31, a high half >= 16 and a low half <= 15, a unique maximum 31 at word 5. Closed form of the high half: words 0..5 = 16 + 3i, 6..10 = 29 - 3(i - 6), 11..15 = 18 + 3(i - 11);
; the low half as listed (15 11 7 3 0 4 8 12 14 10 6 2 1 5 9 13). No rotation, reversal, complement or reversed complement of the table is a rotation of it, so a reversed or inverted playback fails the cycle_order rows.
; Delays (cycles from tests/oracle/huc6280-opcodes.tsv: LDX imm 2, LDY imm 2, DEX 2, DEY 2, BNE 2 plus the taken-branch adder of 2; 12 master clocks per cycle at the power-up low speed, constants.def cycle_mclk_low, ASSUMED LED-CPU-0003; the fixture never executes CSH, RF-12, D-227):
;   (a) LDX #64, 63 passes of DEX and BNE taken (6), a last DEX and BNE not taken (4): 2 + 63 x 6 + 4 = 384 cycles = 4,608 master clocks;
;   (d) two passes of LDX #0 and the same loop: 2 x (2 + 255 x 6 + 4) = 2 x 1,536 = 3,072 cycles = 36,864 master clocks;
;   (f) LDY #24 and 24 outer passes of [LDX #0, the inner loop (1,536), DEY, BNE]: 2 + 24 x (1,536 + 2) + 23 x 4 + 2 = 37,008 cycles = 444,096 master clocks, past the end (key-on + 300,000) of the period and cycle_order windows.
; At the high speed (3 master clocks per cycle) the delays would be a quarter as long, so the fixture must never switch speed. Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 27
DSV_CHECKS = 4

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "psg.inc"

TS_SUM_LO = $2430               ; the sum of the 32 table words by a table walk (low byte, high byte)
TS_SUM_HI = $2431
TS_CNT_LO = $2432               ; the sum of the counter 0..31 (low byte, high byte)
TS_CNT_HI = $2433
TS_DIFF = $2434                 ; the OR of the differences between a high-half word and its closed form
TS_W = $2435                    ; the closed-form word, or the bit of a word in its map byte
TS_MAP = $2436                  ; four bytes: bit (w & 7) of byte (w >> 3) is set for every word w

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:table-sum the XOR mask of the expected low byte of the sum
        LIT 2, $00                      ; GUEST:table-sum the XOR mask of the expected high byte of the sum
        LIT 3, $00                      ; GUEST:table-order the XOR mask of the expected OR of the differences
        LIT 4, $00                      ; GUEST:table-perm the XOR mask of the expected AND of the map bytes

        .pushseg
        .segment "RODATA"
tone_tab:
        .byte $10, $13, $16, $19, $1C, $1F, $1D, $1A, $17, $14, $11, $12, $15, $18, $1B, $1E
        .byte $0F, $0B, $07, $03, $00, $04, $08, $0C, $0E, $0A, $06, $02, $01, $05, $09, $0D
bit_tab:
        .byte $01, $02, $04, $08, $10, $20, $40, $80
        .popseg

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
; (a) the power-on silence: 4,608 master clocks with no PSG write (see the header)
        ldx #64
pre:    dex
        bne pre
; (b) every channel: R0 = s, R4 = $00 (chON 0, DDA 0, AL 0), s = 0..5
        ldx #0
chans:  stx a:PSG_R0
        stz a:PSG_R4
        inx
        cpx #6
        bne chans
; (c) the select-0 tracer scene: the upload by procedure (2) of P8-5, then F = 256 and full volume, chON still 0
        PSG_SELECT 0
        PSG_WRITE PSG_R4, $40           ; chON 0, DDA 1: the wave index of channel 0 resets to 0
        PSG_WRITE PSG_R4, $1F           ; chON 0, DDA 0, AL $1F
        tin tone_tab, PSG_R6, 32        ; the 32 words, each stored at the wave index, which then steps; after 32 it is 0 again
        PSG_WRITE PSG_R2, $00           ; F low 8 bits
        PSG_WRITE PSG_R3, $01           ; F high 4 bits: F = $100 = 256
        PSG_WRITE PSG_R1, $FF           ; LMAL F, RMAL F
        PSG_WRITE PSG_R5, $FF           ; LAL F, RAL F
; (d) 36,864 master clocks of silence before the key-on (see the header)
        ldx #0
gap1:   dex
        bne gap1
        ldx #0
gap2:   dex
        bne gap2
; (e) key-on: chON 1, DDA 0, AL $1F
        PSG_WRITE PSG_R4, $9F
; (f) 444,096 master clocks of the tone (see the header)
        ldy #24
tone_o: ldx #0
tone_i: dex
        bne tone_i
        dey
        bne tone_o
; (g) key-off
        PSG_WRITE PSG_R4, $1F
; (h) check 1 and 2: the sum of the 32 words by a table walk against the sum of the counter 0..31, two computations of 496 = $01F0 (the table is a permutation of 0..31)
        stz a:TS_SUM_LO
        stz a:TS_SUM_HI
        stz a:TS_CNT_LO
        stz a:TS_CNT_HI
        ldx #0
sum:    lda a:tone_tab,x
        clc
        adc a:TS_SUM_LO
        sta a:TS_SUM_LO
        lda a:TS_SUM_HI
        adc #0
        sta a:TS_SUM_HI
        txa
        clc
        adc a:TS_CNT_LO
        sta a:TS_CNT_LO
        lda a:TS_CNT_HI
        adc #0
        sta a:TS_CNT_HI
        inx
        cpx #32
        bne sum
        lda a:TS_CNT_LO
        sta a:GEXP
        lda a:TS_SUM_LO
        CHECK_GUEST 1                   ; check 1 GUEST:table-sum the low byte of the table sum equals the low byte of the counter sum
        lda a:TS_CNT_HI
        sta a:GEXP
        lda a:TS_SUM_HI
        CHECK_GUEST 2                   ; check 2 GUEST:table-sum the high byte of the table sum equals the high byte of the counter sum
; check 3: the table walked in order: words 0..15 against the closed form 16 + 3i, 29 - 3(i - 6), 18 + 3(i - 11); the OR of the differences must be zero
        stz a:TS_DIFF
        lda #16
        sta a:TS_W
        ldx #0
up1:    lda a:tone_tab,x
        eor a:TS_W
        ora a:TS_DIFF
        sta a:TS_DIFF
        lda a:TS_W
        clc
        adc #3
        sta a:TS_W
        inx
        cpx #6
        bne up1
        lda #29
        sta a:TS_W
down:   lda a:tone_tab,x
        eor a:TS_W
        ora a:TS_DIFF
        sta a:TS_DIFF
        lda a:TS_W
        sec
        sbc #3
        sta a:TS_W
        inx
        cpx #11
        bne down
        lda #18
        sta a:TS_W
up2:    lda a:tone_tab,x
        eor a:TS_W
        ora a:TS_DIFF
        sta a:TS_DIFF
        lda a:TS_W
        clc
        adc #3
        sta a:TS_W
        inx
        cpx #16
        bne up2
        stz a:GEXP                      ; the expected difference is zero
        lda a:TS_DIFF
        CHECK_GUEST 3                   ; check 3 GUEST:table-order the high half equals its closed form
; check 4: the 32 words are a permutation of 0..31: every bit of the four map bytes is set
        stz a:TS_MAP
        stz a:TS_MAP+1
        stz a:TS_MAP+2
        stz a:TS_MAP+3
        ldx #0
map:    lda a:tone_tab,x
        and #$07
        tay
        lda a:bit_tab,y
        sta a:TS_W
        lda a:tone_tab,x
        lsr a
        lsr a
        lsr a
        and #$03
        tay
        lda a:TS_MAP,y
        ora a:TS_W
        sta a:TS_MAP,y
        inx
        cpx #32
        bne map
        lda #$FF
        sta a:GEXP                      ; every bit set
        lda a:TS_MAP
        and a:TS_MAP+1
        and a:TS_MAP+2
        and a:TS_MAP+3
        CHECK_GUEST 4                   ; check 4 GUEST:table-perm the 32 words cover 0..31
        rts
