; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; psg_lfo.s - the PSG LFO fixture, DSV id 29 (plan 05.1-01, grown by 05.1-02; D-184, D-185, D-192, D-208, D-212, D-219, D-231, C-05, C-11, RF-12, PR-5, PR-12). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF): the PSG registers are at $0800 to $0809 (fixtures/lib/psg.inc, not edited here). The PSG is write only (H8-24), so every judgement of the sound is the runner's, from the DSPE and DSPA streams against the hand
; literals of fixtures/src/psg_lfo.aexp (ER-21); the DSV1 checks judge only the guest's own data (GUEST). No HuC, PCEAS, emulator or cc65 library code. The fixture never switches to the high speed (RF-12): every delay below is counted at 12 master clocks per cycle.
; The script player of psg_mod.s is copied here (PR-5, no new include): a byte $00..$3F writes the next byte to $0800 + that offset; $80 + n is a delay of n units; $C0 + c the TIN upload of table c to R6; $FF the end. A unit is one pass of
;   [LDX #0, 256 DEX and BNE, DEY, BNE]: at least 1,536 cycles = 18,432 master clocks.
; Tables (D-212, the 05-06 rules): tables 0 and 1 are high[i] = 16 + ((a x i + b) mod 16) for words 0..15 and low[i] = (e x i + g) mod 16 for words 16..31, a permutation of 0..31 (sum 496); table 0 (select 0) has (a, b, e, g) = (7, 3, 11, 9), the table 2 of psg_mod.s,
;   table 1 (select 1) has (5, 8, 3, 1), so its word 0 is 16 + 8 = 24 = $18 (the held modulation word of the held scenes) and its word 1 is 16 + 13 = 29 = $1D; table 2 (lfo_dynamic only) is $18 sixteen times, then $08 sixteen times.
; Setup: (a) every channel R0 = s, R4 = $00 (chON 0, DDA 0, AL 0), s = 0..5; (b) sc_setup: select 1: R4 $40, R4 $00, table 1 (P8-5 procedure (2): the index wraps back to 0), R5 $00 (LAL = RAL = 0: silent by the byte-zero clause of P8-10), R3 $08 then R2 $83
;   (F1 = $883, the decode pair of C-05), R3 $01 and R2 $00 (F1 = 256), R4 $80 (chON 1, AL 0), R1 $FF; R9 $80 (TRG 1: widx[1] = 0, the select-1 counter frozen, CTL 0); select 0: R4 $40, R4 $00, table 0, R2 $00, R3 $02 (F0 = 512), R5 $FF, the key-on R4 $9F (AL $1F).
; Scene lfo_static (sc_static): R9 $80 (CTL 0), 28 units; R9 $82 (CTL 2, TRG 1), 30 units; R9 $83 (CTL 3, TRG 1), 35 units. Scene lfo_ctl1 (sc_ctl1): R9 $81 (CTL 1, TRG 1), 29 units. Scene lfo_needs_sel1 (sc_needs): select 1 R4 $00 (chON 0), R9 $80, 28 units;
;   R9 $81, 28 units. Each phase lasts at least 5.2 select-0 cycles of 192 x F0' master clocks plus 2,000 (28 units >= 516,096 > 5.2 x 98,304 + 2,000; 30 >= 552,960 > 5.2 x 104,448 + 2,000; 35 >= 645,120 > 5.2 x 122,880 + 2,000; 29 >= 534,528 > 5.2 x 99,840 + 2,000).
; Plan 05.1-02 appends its scenes after sc_needs and before the checks, and writes nothing before them, so every anchor of psg_lfo.aexp keeps its count. Checks 1 and 2 judge the guest's own table data (GUEST, ER-21).
; Scene lfo_release (sc_release, plan 05.1-02; C-04, D-220): select 0 R4 $00 (off); R9 $80 (CTL 0, TRG 1: widx[1] = 0, the select-1 counter frozen); select 1 R3 $01, R2 $00 (F1 = 256 again), R5 $FF, the key-on R4 $9F (AL $1F) under TRG 1: frozen at word 0; 1 unit
;   (at least 18,432 master clocks, above 2,000); R9 $00 (TRG 0): the uncoupled select 1 restarts from word 0 and steps every 6 x 256 = 1,536 master clocks; 4 units (at least 73,728 master clocks, past the window end of + 60,000).
; Scene lfo_dynamic (sc_dynamic, plan 05.1-02; D-208, D-219, D-231): select 1 R4 $00 (chON 0, so its upload stores, procedure (2)); R9 $81 (CTL 1, TRG 1: widx[1] = 0, frozen); table 2 (words 0..15 = $18, 16..31 = $08; the 32 stores wrap the index back to 0); R5 $00
;   (silent by the byte-zero clause of P8-10); R3 $00, R2 $80 (F1 = 128); R8 $96 (FLF 150); R4 $80 (chON 1, frozen under TRG 1); select 0: R3 $00, R2 $C0 (F0 = 192), the key-on R4 $9F under R9 $81 (F0' = 192 + $18 - 16 = 200 from the held word wave[1][0]); 1 unit;
;   R9 $01 (TRG 0): select 1 steps every 6 x 128 x 150 = 115,200 master clocks (600 samples), so select 0 reloads at F0' = 200 for 16 steps (9,600 samples) and at 192 + $08 - 16 = 184 for the next 16; 4 x 58 units (at least 4,276,224 master clocks, above the
;   22,000 samples = 4,224,000 master clocks of two half-table switches and the window of 21,000 samples). The scene lasts about 12 frames (D-214).
        .setcpu "huc6280"

DSV_ID = 29
DSV_CHECKS = 2

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
        LIT 1, $00                      ; GUEST:table-sum the XOR mask of the expected difference of the two sums of table 0
        LIT 2, $00                      ; GUEST:table-sum the same for table 1

        .pushseg
        .segment "RODATA"
tab_0:                                  ; table 0 (select 0): (a, b, e, g) = (7, 3, 11, 9)
        .byte $13, $1A, $11, $18, $1F, $16, $1D, $14, $1B, $12, $19, $10, $17, $1E, $15, $1C
        .byte $09, $04, $0F, $0A, $05, $00, $0B, $06, $01, $0C, $07, $02, $0D, $08, $03, $0E
tab_1:                                  ; table 1 (select 1, silent, the modulating words): (a, b, e, g) = (5, 8, 3, 1); word 0 = $18
        .byte $18, $1D, $12, $17, $1C, $11, $16, $1B, $10, $15, $1A, $1F, $14, $19, $1E, $13
        .byte $01, $04, $07, $0A, $0D, $00, $03, $06, $09, $0C, $0F, $02, $05, $08, $0B, $0E
tab_2:                                  ; table 2 (select 1 in lfo_dynamic, silent, the running modulation): words 0..15 = $18, 16..31 = $08
        .byte $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18
        .byte $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08, $08
up_tab:                                 ; the upload routine of table c (c = 0, 1, 2)
        .word ul0, ul1, ul2
sc_setup:                               ; (b) select 1, R9 $80, select 0 (see the header)
        .byte 0, 1, 4, $40, 4, $00, SC_UP + 1, 5, $00, 3, $08, 2, $83, 3, $01, 2, $00, 4, $80, 1, $FF, 9, $80
        .byte 0, 0, 4, $40, 4, $00, SC_UP + 0, 2, $00, 3, $02, 5, $FF, 4, $9F, SC_END
sc_static:                              ; lfo_static: CTL 0, 2, 3 under TRG 1
        .byte 9, $80, SC_DLY + 28, 9, $82, SC_DLY + 30, 9, $83, SC_DLY + 35, SC_END
sc_ctl1:                                ; lfo_ctl1: CTL 1 under TRG 1
        .byte 9, $81, SC_DLY + 29, SC_END
sc_needs:                               ; lfo_needs_sel1: select 1 off, then CTL 0 and CTL 1
        .byte 0, 1, 4, $00, 9, $80, SC_DLY + 28, 9, $81, SC_DLY + 28, SC_END
sc_release:                             ; lfo_release: select 0 off, select 1 keyed on under TRG 1, then the TRG 1 -> 0 release
        .byte 0, 0, 4, $00, 9, $80, 0, 1, 3, $01, 2, $00, 5, $FF, 4, $9F, SC_DLY + 1, 9, $00, SC_DLY + 4, SC_END
sc_dynamic:                             ; lfo_dynamic: select 1 off, R9 $81, table 2, select 1 and select 0 keyed on under TRG 1, then R9 $01
        .byte 0, 1, 4, $00, 9, $81, SC_UP + 2, 5, $00, 3, $00, 2, $80, 8, $96, 4, $80
        .byte 0, 0, 3, $00, 2, $C0, 4, $9F, SC_DLY + 1, 9, $01, SC_DLY + 58, SC_DLY + 58, SC_DLY + 58, SC_DLY + 58, SC_END
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
; the setup and the scenes (see the header and fixtures/src/psg_lfo.aexp)
        PLAY sc_setup
        PLAY sc_static
        PLAY sc_ctl1
        PLAY sc_needs
        PLAY sc_release
        PLAY sc_dynamic
; checks 1 and 2: tables 0 and 1 summed by a table walk against the sum of the counter 0..31, two computations of 496; A = (low XOR low) OR (high XOR high), expected 0
        ldx #0
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 1                   ; check 1 GUEST:table-sum table 0 sums to the counter sum 496
        ldx #32
        jsr tsum
        stz a:GEXP
        CHECK_GUEST 2                   ; check 2 GUEST:table-sum table 1 sums to the counter sum 496
        rts

; tsum: X = the offset of the table from tab_0 on entry (tab_1 is 32); the 32 words summed by a table walk and the counter 0..31 summed; A = (low XOR low) OR (high XOR high)
tsum:   stz a:TS_SUM_LO
        stz a:TS_SUM_HI
        stz a:TS_CNT_LO
        stz a:TS_CNT_HI
        ldy #0
ts_l:   lda a:tab_0,x
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

; ul0, ul1, ul2: the 32 words of table c by TIN to R6 (procedure (2) of P8-5, with chON 0: after R4 $40 and R4 $00 of the script, or R4 $00 alone in lfo_dynamic)
ul0:    tin tab_0, PSG_R6, 32
        rts
ul1:    tin tab_1, PSG_R6, 32
        rts
ul2:    tin tab_2, PSG_R6, 32
        rts
