; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_huc.s - the HuC6280-specific instructions outside the block transfers (plan 03-14; D-101, D-105, D-106, D-124, D-125, D-127; S8-34, S8-39, S8-40, S8-67, S8-72, S8-73, S8-79 to S8-82, S8-87, S8-90, S8-101, H8-8, H8-25). It is assembled by ca65 in the gate and never committed as a binary.
; Checks run in ladder order: the register swaps SXY, SAX and SAY and the register clears CLA, CLX and CLY (the tracer), RMB0-7 and SMB0-7, TAM and TMA with one-hot operands, CSH and CSL, ST0 to ST2, then the addressing modes of the arithmetic, logic, shift and test group that no other fixture reaches.
; P is set from a byte by PLP (the I bit is kept set) and read as PHP, PLA and a mask (B reads 1 and T reads 0 through PHP, H8-5). The pages the checks read are filled with $FF first and every cell a check reads is written by the fixture before (D-127).
; Multi-bit and zero TAM and TMA operands are not exercised here (plan 03-17), and neither are the speed-switch timing and the VDC penalty (plans 03-16 and 03-23). This file is generated from a table of checks by a scratch script; the committed text is the source of record.
        .setcpu "huc6280"

DSV_ID = 6
DSV_CHECKS = 58

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

; the cell of the RMB and SMB checks and the zero-page address of its neighbours ($2043 and $2045)
ZPR     = $44

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
        LIT 15, $FE
        LIT 16, $FD
        LIT 17, $FB
        LIT 18, $F7
        LIT 19, $EF
        LIT 20, $DF
        LIT 21, $BF
        LIT 22, $7F
        LIT 23, $01
        LIT 24, $02
        LIT 25, $04
        LIT 26, $08
        LIT 27, $10
        LIT 28, $20
        LIT 29, $40
        LIT 30, $80
        LIT 31, $C1
        LIT 32, $41
        LIT 33, $66
        LIT 34, $66
        LIT 35, $00
        LIT 36, $F8
        LIT 37, $FB
        LIT 38, $FA
        LIT 39, $33
        LIT 40, $44
        LIT 41, $55
        LIT 42, $66
        LIT 43, $A7
        LIT 44, $A7
        LIT 45, $A7
        LIT 46, $A7
        LIT 47, $A7
        LIT 48, $41
        LIT 49, $41
        LIT 50, $C1
        LIT 51, $01
        LIT 52, $01
        LIT 53, $02
        LIT 54, $00
        LIT 55, $00
        LIT 56, $00
        LIT 57, $03
        LIT 58, $C1

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
        lda #$FF ; check 15 rmb0 MAN:S8-67: RMB0 clears bit 0 of the zero-page cell: the cell $FF gives $FE (S8-67)
        sta z:ZPR
        rmb0 z:ZPR
        lda z:ZPR
        CHECK_LIT 15
        lda #$FF ; check 16 rmb1 MAN:S8-67: RMB1 clears bit 1 of the zero-page cell: the cell $FF gives $FD (S8-67)
        sta z:ZPR
        rmb1 z:ZPR
        lda z:ZPR
        CHECK_LIT 16
        lda #$FF ; check 17 rmb2 MAN:S8-67: RMB2 clears bit 2 of the zero-page cell: the cell $FF gives $FB (S8-67)
        sta z:ZPR
        rmb2 z:ZPR
        lda z:ZPR
        CHECK_LIT 17
        lda #$FF ; check 18 rmb3 MAN:S8-67: RMB3 clears bit 3 of the zero-page cell: the cell $FF gives $F7 (S8-67)
        sta z:ZPR
        rmb3 z:ZPR
        lda z:ZPR
        CHECK_LIT 18
        lda #$FF ; check 19 rmb4 MAN:S8-67: RMB4 clears bit 4 of the zero-page cell: the cell $FF gives $EF (S8-67)
        sta z:ZPR
        rmb4 z:ZPR
        lda z:ZPR
        CHECK_LIT 19
        lda #$FF ; check 20 rmb5 MAN:S8-67: RMB5 clears bit 5 of the zero-page cell: the cell $FF gives $DF (S8-67)
        sta z:ZPR
        rmb5 z:ZPR
        lda z:ZPR
        CHECK_LIT 20
        lda #$FF ; check 21 rmb6 MAN:S8-67: RMB6 clears bit 6 of the zero-page cell: the cell $FF gives $BF (S8-67)
        sta z:ZPR
        rmb6 z:ZPR
        lda z:ZPR
        CHECK_LIT 21
        lda #$FF ; check 22 rmb7 MAN:S8-67: RMB7 clears bit 7 of the zero-page cell: the cell $FF gives $7F (S8-67)
        sta z:ZPR
        rmb7 z:ZPR
        lda z:ZPR
        CHECK_LIT 22
        lda #$00 ; check 23 smb0 MAN:S8-79: SMB0 sets bit 0 of the zero-page cell: the cell $00 gives $01 (S8-79)
        sta z:ZPR
        smb0 z:ZPR
        lda z:ZPR
        CHECK_LIT 23
        lda #$00 ; check 24 smb1 MAN:S8-79: SMB1 sets bit 1 of the zero-page cell: the cell $00 gives $02 (S8-79)
        sta z:ZPR
        smb1 z:ZPR
        lda z:ZPR
        CHECK_LIT 24
        lda #$00 ; check 25 smb2 MAN:S8-79: SMB2 sets bit 2 of the zero-page cell: the cell $00 gives $04 (S8-79)
        sta z:ZPR
        smb2 z:ZPR
        lda z:ZPR
        CHECK_LIT 25
        lda #$00 ; check 26 smb3 MAN:S8-79: SMB3 sets bit 3 of the zero-page cell: the cell $00 gives $08 (S8-79)
        sta z:ZPR
        smb3 z:ZPR
        lda z:ZPR
        CHECK_LIT 26
        lda #$00 ; check 27 smb4 MAN:S8-79: SMB4 sets bit 4 of the zero-page cell: the cell $00 gives $10 (S8-79)
        sta z:ZPR
        smb4 z:ZPR
        lda z:ZPR
        CHECK_LIT 27
        lda #$00 ; check 28 smb5 MAN:S8-79: SMB5 sets bit 5 of the zero-page cell: the cell $00 gives $20 (S8-79)
        sta z:ZPR
        smb5 z:ZPR
        lda z:ZPR
        CHECK_LIT 28
        lda #$00 ; check 29 smb6 MAN:S8-79: SMB6 sets bit 6 of the zero-page cell: the cell $00 gives $40 (S8-79)
        sta z:ZPR
        smb6 z:ZPR
        lda z:ZPR
        CHECK_LIT 29
        lda #$00 ; check 30 smb7 MAN:S8-79: SMB7 sets bit 7 of the zero-page cell: the cell $00 gives $80 (S8-79)
        sta z:ZPR
        smb7 z:ZPR
        lda z:ZPR
        CHECK_LIT 30
        lda #$01 ; check 31 rmb-keep-flags MAN:S8-67: RMB changes no flag (S8-67): the cell $01 gives a zero result with P = N V C set and Z clear, the masked P byte stays $C1 (a core that set Z reads $C3)
        sta z:ZPR
        SETP $C1
        rmb0 z:ZPR
        PMASK $C3
        CHECK_LIT 31
        lda #$00 ; check 32 smb-keep-flags MAN:S8-79: SMB changes no flag (S8-79): the cell $00 gives a negative result with P = V and C set and N and Z clear, the masked P byte stays $41 (a core that set N reads $C1)
        sta z:ZPR
        SETP $41
        smb7 z:ZPR
        PMASK $C3
        CHECK_LIT 32
        lda #$5A ; check 33 rmb-neighbours MAN:S8-67: RMB changes only its own zero-page cell (S8-67): the cells before and after it hold $5A and $3C and read $5A EOR $3C = $66 after RMB2 on the cell between them
        sta z:ZPR-1
        lda #$3C
        sta z:ZPR+1
        lda #$FF
        sta z:ZPR
        rmb2 z:ZPR
        lda z:ZPR-1
        eor z:ZPR+1
        CHECK_LIT 33
        lda #$5A ; check 34 smb-neighbours MAN:S8-79: SMB changes only its own zero-page cell (S8-79): the cells before and after it hold $5A and $3C and read $66 after SMB5 on the cell between them
        sta z:ZPR-1
        lda #$3C
        sta z:ZPR+1
        lda #$00
        sta z:ZPR
        smb5 z:ZPR
        lda z:ZPR-1
        eor z:ZPR+1
        CHECK_LIT 34
        lda #$FF ; check 35 tma-mpr7-reset MAN:H8-8: A reset sets MPR7 to $00 (H8-8, H8-18): TMA with the operand $80 (S8-101) reads $00 into A, which held $FF
        tma #$80
        CHECK_LIT 35
        lda #$F8 ; check 36 tma-mpr0 MAN:S8-101: TMA with the one-hot operand $01 reads MPR0 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $F8
        tam #$01
        lda #$FB
        tam #$02
        lda #$FA
        tam #$04
        lda #$33
        tam #$08
        lda #$44
        tam #$10
        lda #$55
        tam #$20
        lda #$66
        tam #$40
        lda #$00
        tma #$01
        CHECK_LIT 36
        lda #$00 ; check 37 tma-mpr1 MAN:S8-101: TMA with the one-hot operand $02 reads MPR1 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $FB
        tma #$02
        CHECK_LIT 37
        lda #$00 ; check 38 tma-mpr2 MAN:S8-101: TMA with the one-hot operand $04 reads MPR2 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $FA
        tma #$04
        CHECK_LIT 38
        lda #$00 ; check 39 tma-mpr3 MAN:S8-101: TMA with the one-hot operand $08 reads MPR3 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $33
        tma #$08
        CHECK_LIT 39
        lda #$00 ; check 40 tma-mpr4 MAN:S8-101: TMA with the one-hot operand $10 reads MPR4 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $44
        tma #$10
        CHECK_LIT 40
        lda #$00 ; check 41 tma-mpr5 MAN:S8-101: TMA with the one-hot operand $20 reads MPR5 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $55
        tma #$20
        CHECK_LIT 41
        lda #$00 ; check 42 tma-mpr6 MAN:S8-101: TMA with the one-hot operand $40 reads MPR6 into A (S8-101): after TAM wrote the seven distinct values $F8 $FB $FA $33 $44 $55 $66 to MPR0 to MPR6 (S8-90) it reads $66
        tma #$40
        CHECK_LIT 42
        lda #$A7 ; check 43 tam-xlate-mpr0 MAN:S8-90: TAM with the operand $01 loads MPR0 (S8-90): MPR0 = $F8 maps logical $0000 to work RAM, so a read there returns the marker $A7 written at $2000
        sta a:$2000
        lda #$F8
        tam #$01
        lda a:$0000
        CHECK_LIT 43
        lda #$F8 ; check 44 tam-xlate-mpr3 MAN:S8-90: TAM with the operand $08 loads MPR3 (S8-90): MPR3 = $F8 maps logical $6000 to work RAM, so a read there returns the marker $A7
        tam #$08
        lda a:$6000
        CHECK_LIT 44
        lda #$F8 ; check 45 tam-xlate-mpr4 MAN:S8-90: TAM with the operand $10 loads MPR4 (S8-90): MPR4 = $F8 maps logical $8000 to work RAM, so a read there returns the marker $A7
        tam #$10
        lda a:$8000
        CHECK_LIT 45
        lda #$F8 ; check 46 tam-xlate-mpr5 MAN:S8-90: TAM with the operand $20 loads MPR5 (S8-90): MPR5 = $F8 maps logical $A000 to work RAM, so a read there returns the marker $A7
        tam #$20
        lda a:$A000
        CHECK_LIT 46
        lda #$F8 ; check 47 tam-xlate-mpr6 MAN:S8-90: TAM with the operand $40 loads MPR6 (S8-90): MPR6 = $F8 maps logical $C000 to work RAM, so a read there returns the marker $A7
        tam #$40
        lda a:$C000
        CHECK_LIT 47
        lda #$45 ; check 48 tam-keep-flags MAN:S8-90: TAM changes no flag (S8-90): P = V and C set with N and Z clear, A = $80, the masked P byte stays $41 (a core that set N reads $C1)
        pha
        lda #$80
        plp
        tam #$08
        PMASK $C3
        CHECK_LIT 48
        lda #$45 ; check 49 tma-keep-flags-n MAN:S8-101: TMA changes no flag (S8-101): MPR3 holds $80, P = V and C set with N and Z clear, the masked P byte stays $41 after the read of a byte with bit 7 set
        pha
        lda #$00
        plp
        tma #$08
        PMASK $C3
        CHECK_LIT 49
        lda #$C5 ; check 50 tma-keep-flags-z MAN:S8-101: TMA changes no flag (S8-101): MPR7 holds $00, P = N V and C set with Z clear, the masked P byte stays $C1 after the read of a zero byte
        pha
        lda #$FF
        plp
        tma #$80
        PMASK $C3
        CHECK_LIT 50
        lda #$F8 ; check 51 csh-length MAN:H8-25: CSH is one byte (H8-25 map, no software-manual page): the instruction after it runs, so INX after CSH leaves X = $01
        tam #$02
        lda #$F9
        tam #$04
        ldx #$00
        csh
        inx
        txa
        CHECK_LIT 51
        ldx #$00 ; check 52 csl-length MAN:H8-25: CSL is one byte (H8-25 map, no software-manual page): the instruction after it runs, so INX after CSL leaves X = $01
        csl
        inx
        txa
        CHECK_LIT 52
        ldx #$00 ; check 53 csh-csl-pair MAN:H8-25: CSH then CSL execute and execution goes on at the next byte each time: two INX, one after each, leave X = $02
        csh
        inx
        csl
        inx
        txa
        CHECK_LIT 53
        lda #$F8 ; check 54 st0-bypass GUEST:ram-before-equals-ram-after: ST0 writes physical $1FE000 and does not consult the MPRs (H8-8 Table 2-2-3, S8-80, D-106): with MPR0 mapped to work RAM, RAM[0] still holds the $5A the guest wrote, read at logical $0000
        tam #$01
        lda #$5A
        sta a:$2000
        sta a:GEXP
        st0 #$A5
        lda a:$0000
        CHECK_GUEST 54
        lda #$5B ; check 55 st1-bypass GUEST:ram-before-equals-ram-after: ST1 writes physical $1FE002 and does not consult the MPRs (H8-8 Table 2-2-3, S8-81, D-106): with MPR0 mapped to work RAM, RAM[2] still holds the $5B the guest wrote
        sta a:$2002
        sta a:GEXP
        st1 #$A6
        lda a:$0002
        CHECK_GUEST 55
        lda #$5C ; check 56 st2-bypass GUEST:ram-before-equals-ram-after: ST2 writes physical $1FE003 and does not consult the MPRs (H8-8 Table 2-2-3, S8-82, D-106): with MPR0 mapped to work RAM, RAM[3] still holds the $5C the guest wrote
        sta a:$2003
        sta a:GEXP
        st2 #$A7
        lda a:$0003
        CHECK_GUEST 56
        ldx #$00 ; check 57 st-lengths MAN:S8-80: ST0, ST1 and ST2 are two bytes each (S8-80 to S8-82): an INX after each runs three times, and the operand $E8 (the INX opcode) is never executed, so X reads $03
        st0 #$E8
        inx
        st1 #$E8
        inx
        st2 #$E8
        inx
        txa
        CHECK_LIT 57
        SETP $C1 ; check 58 st-keep-flags MAN:S8-80: ST0 changes no flag (flag row of S8-80, S8-81, S8-82 is T = 0 and dashes): P = N V C set and Z clear, the operand $00 does not set Z, the masked P byte stays $C1
        st0 #$00
        PMASK $C3
        CHECK_LIT 58
        rts
