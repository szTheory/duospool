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
; Grown by plan 05-06: after (g) a script player (play) runs the register scripts of RODATA: a byte $00..$3F is a write of the next byte to $0800 + that offset; $80 + n a delay of n units; $C0 + c the TIN upload of channel table c to R6; $FF the end.
;   A delay unit is one pass of [LDX #0, the inner loop (1,534), DEY, BNE]: at least 1,536 cycles = 18,432 master clocks, so n units last at least n x 18,432 master clocks (the outer branch adds 2 or 4 cycles per pass).
;   Channel tables (D-212, Claude's discretion): table c (select c) is high[i] = 16 + ((a x i + b) mod 16) for words 0..15 and low[i] = (e x i + g) mod 16 for words 16..31, with (a, b, e, g) = (3, 0, 5, 15), (5, 7, 3, 2), (7, 3, 11, 9), (9, 12, 13, 4), (11, 5, 7, 14), (13, 10, 9, 6);
;   a and e odd, so each half is a permutation and the table a permutation of 0..31 (sum 496). A throwaway script outside the repository checked that no rotation, reversal, complement or reversed complement of a table is a rotation of itself,
;   of another channel's table or of tone_tab, that cyclically adjacent words differ, and that the only rising crossing of the threshold 15.5 per cycle is word 31 -> word 0 (words 0..15 are 16..31, words 16..31 are 0..15).
;   Scenes tone_pan01, tone_pan23, tone_pan45 (selects a, b = 0, 1; 2, 3; 4, 5 at F = 128, 192; 256, 320; 384, 448): set-up of each channel (R0, R4 $40, R4 $00, the TIN upload, R2, R3; R7 $00 at select 4 and 5), R1 $FF, then five sub-phases A, B, C1, C2, C3, each opened by
;   R4 $40 (the wave index 0, D-190) and R4 $1F or $1D of its channel(s), then R5, then the key-on; a delay of n units (13, 21, 29: at least 6.1 cycles of the slower channel + 2,000 master clocks) and the key-off. A: a solo (R5 $FF, R4 $9F); B: b solo;
;   C1: a alone hard left (R5 $F0, R4 $9D); C2: b alone hard right (R5 $0F, R4 $9D); C3: both, as in C1 and C2. The literals and their derivation are in fixtures/src/psg_tone.aexp. Checks 5..10: each channel table sums to 496 two ways (GUEST).
        .setcpu "huc6280"

DSV_ID = 27
DSV_CHECKS = 10

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
PTR = $20                       ; zero page pointer of the script player (logical $2020)
SC_DLY = $80                    ; script command: a delay of (byte - $80) units
SC_UP = $C0                     ; script command: the TIN upload of channel table (byte - $C0) to R6
SC_END = $FF                    ; script command: the end of the script

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:table-sum the XOR mask of the expected low byte of the sum
        LIT 2, $00                      ; GUEST:table-sum the XOR mask of the expected high byte of the sum
        LIT 3, $00                      ; GUEST:table-order the XOR mask of the expected OR of the differences
        LIT 4, $00                      ; GUEST:table-perm the XOR mask of the expected AND of the map bytes
        LIT 5, $00                      ; GUEST:table-sum the XOR mask of the expected difference of the two sums of channel table 0
        LIT 6, $00                      ; GUEST:table-sum the same for channel table 1
        LIT 7, $00                      ; GUEST:table-sum the same for channel table 2
        LIT 8, $00                      ; GUEST:table-sum the same for channel table 3
        LIT 9, $00                      ; GUEST:table-sum the same for channel table 4
        LIT 10, $00                     ; GUEST:table-sum the same for channel table 5

        .pushseg
        .segment "RODATA"
tone_tab:
        .byte $10, $13, $16, $19, $1C, $1F, $1D, $1A, $17, $14, $11, $12, $15, $18, $1B, $1E
        .byte $0F, $0B, $07, $03, $00, $04, $08, $0C, $0E, $0A, $06, $02, $01, $05, $09, $0D
bit_tab:
        .byte $01, $02, $04, $08, $10, $20, $40, $80
chan_tab:                               ; the six channel tables, 32 bytes each (see the header for the formula)
        .byte $10, $13, $16, $19, $1C, $1F, $12, $15, $18, $1B, $1E, $11, $14, $17, $1A, $1D   ; table 0 (select 0): (a, b, e, g) = (3, 0, 5, 15)
        .byte $0F, $04, $09, $0E, $03, $08, $0D, $02, $07, $0C, $01, $06, $0B, $00, $05, $0A
        .byte $17, $1C, $11, $16, $1B, $10, $15, $1A, $1F, $14, $19, $1E, $13, $18, $1D, $12   ; table 1 (select 1): (a, b, e, g) = (5, 7, 3, 2)
        .byte $02, $05, $08, $0B, $0E, $01, $04, $07, $0A, $0D, $00, $03, $06, $09, $0C, $0F
        .byte $13, $1A, $11, $18, $1F, $16, $1D, $14, $1B, $12, $19, $10, $17, $1E, $15, $1C   ; table 2 (select 2): (a, b, e, g) = (7, 3, 11, 9)
        .byte $09, $04, $0F, $0A, $05, $00, $0B, $06, $01, $0C, $07, $02, $0D, $08, $03, $0E
        .byte $1C, $15, $1E, $17, $10, $19, $12, $1B, $14, $1D, $16, $1F, $18, $11, $1A, $13   ; table 3 (select 3): (a, b, e, g) = (9, 12, 13, 4)
        .byte $04, $01, $0E, $0B, $08, $05, $02, $0F, $0C, $09, $06, $03, $00, $0D, $0A, $07
        .byte $15, $10, $1B, $16, $11, $1C, $17, $12, $1D, $18, $13, $1E, $19, $14, $1F, $1A   ; table 4 (select 4): (a, b, e, g) = (11, 5, 7, 14)
        .byte $0E, $05, $0C, $03, $0A, $01, $08, $0F, $06, $0D, $04, $0B, $02, $09, $00, $07
        .byte $1A, $17, $14, $11, $1E, $1B, $18, $15, $12, $1F, $1C, $19, $16, $13, $10, $1D   ; table 5 (select 5): (a, b, e, g) = (13, 10, 9, 6)
        .byte $06, $0F, $08, $01, $0A, $03, $0C, $05, $0E, $07, $00, $09, $02, $0B, $04, $0D
up_tab:                                 ; the upload routine of each channel table
        .word ul0, ul1, ul2, ul3, ul4, ul5
; the pan-pair scene script of selects a, b at F a, F b, n delay units (the header); a macro so the three scenes share one shape
        .macro PAN_SCENE ca, cb, fa, fb, n, r7
        .byte 0, (ca), 4, $40, 4, $00, SC_UP + (ca), 2, <(fa), 3, >(fa)                 ; set-up of select a: the wave index 0, chON 0, the upload, F a
        .if r7
        .byte 7, $00                                                                    ; R7 $00 (NE 0) at select 4
        .endif
        .byte 0, (cb), 4, $40, 4, $00, SC_UP + (cb), 2, <(fb), 3, >(fb)                 ; set-up of select b
        .if r7
        .byte 7, $00                                                                    ; R7 $00 (NE 0) at select 5
        .endif
        .byte 1, $FF                                                                    ; R1 $FF: LMAL F, RMAL F
        .byte 0, (ca), 4, $40, 4, $1F, 5, $FF, 4, $9F, SC_DLY + (n), 4, $1F             ; A: a solo at S = 0 both sides
        .byte 0, (cb), 4, $40, 4, $1F, 5, $FF, 4, $9F, SC_DLY + (n), 4, $1F             ; B: b solo at S = 0 both sides
        .byte 0, (ca), 4, $40, 4, $1D, 5, $F0, 4, $9D, SC_DLY + (n), 4, $1D             ; C1: a alone hard left (left S = 1, right S = 16)
        .byte 0, (cb), 4, $40, 4, $1D, 5, $0F, 4, $9D, SC_DLY + (n), 4, $1D             ; C2: b alone hard right (right S = 1, left S = 16)
        .byte 0, (ca), 4, $40, 4, $1D, 5, $F0, 4, $9D                                   ; C3: a hard left ...
        .byte 0, (cb), 4, $40, 4, $1D, 5, $0F, 4, $9D, SC_DLY + (n)                     ; ... and b hard right
        .byte 0, (ca), 4, $1D, 0, (cb), 4, $1D, SC_END                                  ; both keyed off
        .endmacro
sc_pan01:
        PAN_SCENE 0, 1, 128, 192, 13, 0
sc_pan23:
        PAN_SCENE 2, 3, 256, 320, 21, 0
sc_pan45:
        PAN_SCENE 4, 5, 384, 448, 29, 1
        .popseg

; PLAY script: the script player run over one script of RODATA
        .macro PLAY script
        lda #<(script)
        sta z:PTR
        lda #>(script)
        sta z:PTR+1
        jsr play
        .endmacro

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
; (g2) the scenes of plan 05-06 (see the header and fixtures/src/psg_tone.aexp)
        PLAY sc_pan01
        PLAY sc_pan23
        PLAY sc_pan45
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
; checks 5..10: channel table c (c = 0..5) summed by a table walk against the sum of the counter 0..31, two computations of 496; A = (low XOR low) OR (high XOR high), expected 0
        ldx #0
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 5                   ; check 5 GUEST:table-sum channel table 0 sums to the counter sum 496
        ldx #32
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 6                   ; check 6 GUEST:table-sum channel table 1 sums to the counter sum 496
        ldx #64
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 7                   ; check 7 GUEST:table-sum channel table 2 sums to the counter sum 496
        ldx #96
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 8                   ; check 8 GUEST:table-sum channel table 3 sums to the counter sum 496
        ldx #128
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 9                   ; check 9 GUEST:table-sum channel table 4 sums to the counter sum 496
        ldx #160
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 10                  ; check 10 GUEST:table-sum channel table 5 sums to the counter sum 496
        rts

; tsum: X = 32c on entry; the 32 words of channel table c summed by a table walk and the counter 0..31 summed; A = (low XOR low) OR (high XOR high)
tsum:   stz a:TS_SUM_LO
        stz a:TS_SUM_HI
        stz a:TS_CNT_LO
        stz a:TS_CNT_HI
        ldy #0
ts_l:   lda a:chan_tab,x
        clc
        adc a:TS_SUM_LO
        sta a:TS_SUM_LO
        lda a:TS_SUM_HI
        adc #0
        sta a:TS_SUM_HI
        tya
        clc
        adc a:TS_CNT_LO
        sta a:TS_CNT_LO
        lda a:TS_CNT_HI
        adc #0
        sta a:TS_CNT_HI
        inx
        iny
        cpy #32
        bne ts_l
        lda a:TS_SUM_LO
        eor a:TS_CNT_LO
        sta a:TS_W
        lda a:TS_SUM_HI
        eor a:TS_CNT_HI
        ora a:TS_W
        rts

; play: run the script at PTR (see the header for the commands) up to its $FF
play:   jsr getb
        cmp #SC_END
        beq pl_end
        cmp #SC_DLY
        bcs pl_cmd
        tax                             ; a register offset: the next byte is written to $0800 + offset
        jsr getb
        sta a:PSG_R0,x
        bra play
pl_cmd: cmp #SC_UP
        bcs pl_up
        and #$3F                        ; a delay of n units (see the header)
        tay
pl_o:   ldx #0
pl_i:   dex
        bne pl_i
        dey
        bne pl_o
        bra play
pl_up:  and #$07                        ; the upload of channel table c
        asl a
        tax
        jsr pl_jmp
        bra play
pl_end: rts
pl_jmp: jmp (up_tab,x)

; getb: A = the script byte at PTR, PTR advanced by one
getb:   lda (PTR)
        inc z:PTR
        bne gb_r
        inc z:PTR+1
gb_r:   rts

; ul0..ul5: the 32 words of channel table c by TIN to R6 (procedure (2) of P8-5, after R4 $40 and R4 $00 of the script)
ul0:    tin chan_tab, PSG_R6, 32
        rts
ul1:    tin chan_tab+32, PSG_R6, 32
        rts
ul2:    tin chan_tab+64, PSG_R6, 32
        rts
ul3:    tin chan_tab+96, PSG_R6, 32
        rts
ul4:    tin chan_tab+128, PSG_R6, 32
        rts
ul5:    tin chan_tab+160, PSG_R6, 32
        rts
