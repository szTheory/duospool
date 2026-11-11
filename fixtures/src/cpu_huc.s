; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_huc.s - the HuC6280-specific instructions outside the block transfers (plan 03-14; D-101, D-105, D-106, D-124, D-125, D-127; S8-34, S8-39, S8-40, S8-67, S8-72, S8-73, S8-79 to S8-82, S8-87, S8-90, S8-101, H8-8, H8-25). It is assembled by ca65 in the gate and never committed as a binary.
; Checks run in ladder order: the register swaps SXY, SAX and SAY and the register clears CLA, CLX and CLY (the tracer), RMB0-7 and SMB0-7, TAM and TMA with one-hot operands, CSH and CSL, ST0 to ST2, then the addressing modes of the arithmetic, logic, shift and test group that no other fixture reaches.
; P is set from a byte by PLP (the I bit is kept set) and read as PHP, PLA and a mask (B reads 1 and T reads 0 through PHP, H8-5). The pages the checks read are filled with $FF first and every cell a check reads is written by the fixture before (D-127).
; Multi-bit and zero TAM and TMA operands are not exercised here (plan 03-17), and neither are the speed-switch timing and the VDC penalty (plans 03-16 and 03-23). This file is generated from a table of checks by a scratch script; the committed text is the source of record.
        .setcpu "huc6280"

DSV_ID = 6
DSV_CHECKS = 14

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

; P is set from a byte by PLP (the I bit is kept set); P is read as a masked PHP byte
        .macro SETP value
        lda #((value) | $04)
        pha
        plp
        .endmacro
        .macro PMASK mask
        php
        pla
        and #(mask)
        .endmacro

        CHECK_TABLE DSV_CHECKS

        LIT 1, $34
        LIT 2, $12
        LIT 3, $78
        LIT 4, $56
        LIT 5, $BC
        LIT 6, $9A
        LIT 7, $C3
        LIT 8, $00
        LIT 9, $00
        LIT 10, $00
        LIT 11, $00
        LIT 12, $C1
        LIT 13, $C1
        LIT 14, $C1

fixture_main:
        ldx #$00                ; the pages the checks read are filled with $FF, then every cell that a check reads is written
        lda #$FF
fill:   sta a:$2000,x
        sta a:$2500,x
        sta a:$2600,x
        sta a:$2700,x
        inx
        bne fill
        ldx #$12 ; check 1 sxy-x MAN:S8-87: SXY exchanges X and Y: X = $12 and Y = $34 before, X reads $34 after (S8-87)
        ldy #$34
        sxy
        txa
        CHECK_LIT 1
        ldx #$12 ; check 2 sxy-y MAN:S8-87: SXY exchanges X and Y: X = $12 and Y = $34 before, Y reads $12 after (S8-87)
        ldy #$34
        sxy
        tya
        CHECK_LIT 2
        lda #$56 ; check 3 sax-a MAN:S8-72: SAX exchanges A and X: A = $56 and X = $78 before, A reads $78 after (S8-72)
        ldx #$78
        sax
        CHECK_LIT 3
        lda #$56 ; check 4 sax-x MAN:S8-72: SAX exchanges A and X: A = $56 and X = $78 before, X reads $56 after (S8-72)
        ldx #$78
        sax
        txa
        CHECK_LIT 4
        lda #$9A ; check 5 say-a MAN:S8-73: SAY exchanges A and Y: A = $9A and Y = $BC before, A reads $BC after (S8-73)
        ldy #$BC
        say
        CHECK_LIT 5
        lda #$9A ; check 6 say-y MAN:S8-73: SAY exchanges A and Y: A = $9A and Y = $BC before, Y reads $9A after (S8-73)
        ldy #$BC
        say
        tya
        CHECK_LIT 6
        lda #$C7 ; check 7 swaps-keep-flags-set MAN:S8-87: SXY, SAX and SAY change no flag (flag rows of S8-72, S8-73, S8-87 are T = 0 and dashes): P = N V Z C set, positive non-zero registers, the masked P byte stays $C3
        pha
        ldx #$01
        ldy #$02
        lda #$03
        plp
        sxy
        sax
        say
        PMASK $C3
        CHECK_LIT 7
        lda #$04 ; check 8 swaps-keep-flags-clear MAN:S8-87: SXY, SAX and SAY change no flag: P = N V Z C clear, registers that would set N and Z ($80, $00, $FF), the masked P byte stays $00
        pha
        ldx #$80
        ldy #$00
        lda #$FF
        plp
        sxy
        sax
        say
        PMASK $C3
        CHECK_LIT 8
        lda #$FF ; check 9 cla MAN:S8-34: CLA sets A to $00: A = $FF before (S8-34)
        cla
        CHECK_LIT 9
        ldx #$FF ; check 10 clx MAN:S8-39: CLX sets X to $00: X = $FF before (S8-39)
        clx
        txa
        CHECK_LIT 10
        ldy #$FF ; check 11 cly MAN:S8-40: CLY sets Y to $00: Y = $FF before (S8-40)
        cly
        tya
        CHECK_LIT 11
        SETP $C1 ; check 12 cla-keep-flags MAN:S8-34: CLA changes no flag (S8-34): P = N V C set and Z clear, the masked P byte stays $C1 (a core that set Z for the zero result reads $C3)
        cla
        PMASK $C3
        CHECK_LIT 12
        SETP $C1 ; check 13 clx-keep-flags MAN:S8-39: CLX changes no flag (S8-39): P = N V C set and Z clear, the masked P byte stays $C1
        clx
        PMASK $C3
        CHECK_LIT 13
        SETP $C1 ; check 14 cly-keep-flags MAN:S8-40: CLY changes no flag (S8-40): P = N V C set and Z clear, the masked P byte stays $C1
        cly
        PMASK $C3
        CHECK_LIT 14
        rts
