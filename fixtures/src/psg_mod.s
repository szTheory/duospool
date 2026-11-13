; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; psg_mod.s - the PSG noise and DDA fixture, DSV id 28 (plan 05-07; D-185, D-190, D-191, D-193, D-204, D-208, D-212, RF-6, RF-11, RF-12, PR-5, PR-12). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF): the PSG registers are at $0800 to $0809 (fixtures/lib/psg.inc, not edited here). The PSG is write only (H8-24), so every judgement of the sound is the runner's, from the DSPE and DSPA streams against the hand
; literals of fixtures/src/psg_mod.aexp (ER-21); the DSV1 checks judge only the guest's own data (GUEST). No HuC, PCEAS, emulator or cc65 library code. The fixture never switches to the high speed (RF-12): every delay below is counted at 12 master clocks per cycle.
; Sequence: (a) every channel R0 = s, R4 = $00 (chON 0, DDA 0, AL 0), s = 0..5; then the register scripts of RODATA, played by the script player of psg_tone.s (copied here): a byte $00..$3F writes the next byte to $0800 + that offset; $80 + n is a delay of n units; $C0 + c the
;   TIN upload of table c to R6; $FF the end. A unit is one pass of [LDX #0, 256 DEX and BNE, DEY, BNE]: at least 1,536 cycles = 18,432 master clocks.
; Tables (D-212): table c is high[i] = 16 + ((a x i + b) mod 16) for words 0..15 and low[i] = (e x i + g) mod 16 for words 16..31 with (a, b, e, g) = (7, 3, 11, 9), (9, 12, 13, 4), (11, 5, 7, 14), (13, 10, 9, 6) for c = 2, 3, 4, 5: the channel tables 2..5 of
;   psg_tone.s, whose rotation, reversal, complement, adjacency and crossing rules a throwaway script outside the repository checked in plan 05-06. Each is a permutation of 0..31 (sum 496).
; Scene mod_noise4 (sc_noise4): select 4, R4 $40, R4 $00, table 4, F = 256 (R2 $00, R3 $01), R1 $FF, R5 $FF, then R7 $98 (NE 1, NFRQ 24: NFc 7), and only then the key-on R4 $9F (AL $1F); 20 units (at least 368,640 master clocks: more than one frame of 357,630 and the
;   window to the key-on + 312,096); the key-off R4 $1F. Scene mod_noise5 (sc_noise5): the same on select 5 with table 5 and R7 $91 (NFRQ 17: NFc 14).
; Scene mod_r7 (sc_r7): select 2, table 2, F = 256, R1 $FF, R5 $FF, key-on R4 $9F; 15 units (the first window, key-on + 50,000 to + 260,000); R0 $02 and R7 $98 (no target at select 2); 12 units (the second window, R0 + 2,000 to + 212,000); the key-off.
        .setcpu "huc6280"

DSV_ID = 28
DSV_CHECKS = 3

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "psg.inc"

TS_SUM_LO = $2430               ; the sum of the 32 table words by a table walk (low byte, high byte)
TS_SUM_HI = $2431
TS_CNT_LO = $2432               ; the sum of the counter 0..31 (low byte, high byte)
TS_CNT_HI = $2433
TS_W = $2435                    ; scratch of tsum
PTR = $20                       ; zero page pointer of the script player (logical $2020)
SC_DLY = $80                    ; script command: a delay of (byte - $80) units
SC_UP = $C0                     ; script command: the TIN upload of table (byte - $C0) to R6
SC_END = $FF                    ; script command: the end of the script

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:table-sum the XOR mask of the expected difference of the two sums of table 2
        LIT 2, $00                      ; GUEST:table-sum the same for table 4
        LIT 3, $00                      ; GUEST:table-sum the same for table 5

        .pushseg
        .segment "RODATA"
tab_2:                                  ; table 2 (select 2 in mod_r7): (a, b, e, g) = (7, 3, 11, 9)
        .byte $13, $1A, $11, $18, $1F, $16, $1D, $14, $1B, $12, $19, $10, $17, $1E, $15, $1C
        .byte $09, $04, $0F, $0A, $05, $00, $0B, $06, $01, $0C, $07, $02, $0D, $08, $03, $0E
tab_4:                                  ; table 4 (select 4 in mod_noise4): (a, b, e, g) = (11, 5, 7, 14)
        .byte $15, $10, $1B, $16, $11, $1C, $17, $12, $1D, $18, $13, $1E, $19, $14, $1F, $1A
        .byte $0E, $05, $0C, $03, $0A, $01, $08, $0F, $06, $0D, $04, $0B, $02, $09, $00, $07
tab_5:                                  ; table 5 (select 5 in mod_noise5): (a, b, e, g) = (13, 10, 9, 6)
        .byte $1A, $17, $14, $11, $1E, $1B, $18, $15, $12, $1F, $1C, $19, $16, $13, $10, $1D
        .byte $06, $0F, $08, $01, $0A, $03, $0C, $05, $0E, $07, $00, $09, $02, $0B, $04, $0D
up_tab:                                 ; the upload routine of table c (c = 2..5; 0 and 1 unused)
        .word ul_none, ul_none, ul2, ul_none, ul4, ul5
sc_noise4:                              ; mod_noise4 (see the header)
        .byte 0, 4, 4, $40, 4, $00, SC_UP + 4, 2, $00, 3, $01, 1, $FF, 5, $FF, 7, $98, 4, $9F, SC_DLY + 20, 4, $1F, SC_END
sc_noise5:                              ; mod_noise5
        .byte 0, 5, 4, $40, 4, $00, SC_UP + 5, 2, $00, 3, $01, 1, $FF, 5, $FF, 7, $91, 4, $9F, SC_DLY + 20, 4, $1F, SC_END
sc_r7:                                  ; mod_r7
        .byte 0, 2, 4, $40, 4, $00, SC_UP + 2, 2, $00, 3, $01, 1, $FF, 5, $FF, 4, $9F, SC_DLY + 15
        .byte 0, 2, 7, $98, SC_DLY + 12, 4, $1F, SC_END
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
; (a) every channel: R0 = s, R4 = $00 (chON 0, DDA 0, AL 0), s = 0..5
        ldx #0
chans:  stx a:PSG_R0
        stz a:PSG_R4
        inx
        cpx #6
        bne chans
; the scenes (see the header and fixtures/src/psg_mod.aexp)
        PLAY sc_noise4
        PLAY sc_noise5
        PLAY sc_r7
; checks 1..3: tables 2, 4 and 5 summed by a table walk against the sum of the counter 0..31, two computations of 496; A = (low XOR low) OR (high XOR high), expected 0
        ldx #0
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 1                   ; check 1 GUEST:table-sum table 2 sums to the counter sum 496
        ldx #32
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 2                   ; check 2 GUEST:table-sum table 4 sums to the counter sum 496
        ldx #64
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 3                   ; check 3 GUEST:table-sum table 5 sums to the counter sum 496
        rts

; tsum: X = the offset of the table from tab_2 on entry (tab_4 is 32, tab_5 64); the 32 words summed by a table walk and the counter 0..31 summed; A = (low XOR low) OR (high XOR high)
tsum:   stz a:TS_SUM_LO
        stz a:TS_SUM_HI
        stz a:TS_CNT_LO
        stz a:TS_CNT_HI
        ldy #0
ts_l:   lda a:tab_2,x
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
pl_up:  and #$07                        ; the upload of table c
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

; ul2, ul4, ul5: the 32 words of table c by TIN to R6 (procedure (2) of P8-5, after R4 $40 and R4 $00 of the script); ul_none uploads nothing
ul2:    tin tab_2, PSG_R6, 32
        rts
ul4:    tin tab_4, PSG_R6, 32
        rts
ul5:    tin tab_5, PSG_R6, 32
ul_none:
        rts
