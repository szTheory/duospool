; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_huc.s - the HuC6280-specific instructions outside the block transfers (plan 03-14; D-101, D-105, D-106, D-124, D-125, D-127; S8-34, S8-39, S8-40, S8-67, S8-72, S8-73, S8-79 to S8-82, S8-87, S8-90, S8-101, H8-8, H8-25). It is assembled by ca65 in the gate and never committed as a binary.
; Checks run in ladder order: the register swaps SXY, SAX and SAY and the register clears CLA, CLX and CLY (the tracer), RMB0-7 and SMB0-7, TAM and TMA with one-hot operands, CSH and CSL, ST0 to ST2, then the addressing modes of the arithmetic, logic, shift and test group that no other fixture reaches.
; P is set from a byte by PLP (the I bit is kept set) and read as PHP, PLA and a mask (B reads 1 and T reads 0 through PHP, H8-5). The pages the checks read are filled with $FF first and every cell a check reads is written by the fixture before (D-127).
; The operand cells of the mode-coverage checks hold one bit each: zero page $2010 = $01, zero page,X (X = 3) $2013 = $02, (zz,X) (X = 4) $2601 = $04, (zz) $2600 = $08, (zz),Y (Y = 3) $2603 = $10, absolute,X (X = 2) $2502 = $20, absolute,Y (Y = 5) $2505 = $40, absolute $2500 = $80.
; Multi-bit and zero TAM and TMA operands are not exercised here (plan 03-17), and neither are the speed-switch timing and the VDC penalty (plans 03-16 and 03-23). This file is generated from a table of checks by a scratch script; the committed text is the source of record.
        .setcpu "huc6280"

DSV_ID = 6
DSV_CHECKS = 76

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

; the cell of the RMB and SMB checks and the zero-page address of its neighbours ($2043 and $2045)
ZPR     = $44

; the cells of the mode-coverage checks (work RAM): zero-page operands $2010 and $2013, the pointer $2030 to $2600 and $2034 to $2601, the absolute operands $2500, $2502 and $2505,
; the read-modify-write work cells $2050 (zero page), $2700 (absolute) and the accumulator cell $2060
ZPA     = $10
ZP_PTR  = $30
ZP_PTR2 = $34
ZPW     = $50
ACC     = $60
RAM_ABS = $2500
RAM_P0  = $2600
RAM_W   = $2700

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
; CMPHIT: after a compare, add the bits of mask to ACC when the compare left Z and C both set (equal operands)
        .macro CMPHIT mask
        .local skip
        php
        pla
        and #$83
        cmp #$03
        bne skip
        lda z:ACC
        ora #(mask)
        sta z:ACC
skip:
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
        LIT 59, $6F
        LIT 60, $3E
        LIT 61, $A1
        LIT 62, $20
        LIT 63, $80
        LIT 64, $7D
        LIT 65, $03
        LIT 66, $03
        LIT 67, $2A
        LIT 68, $2B
        LIT 69, $15
        LIT 70, $95
        LIT 71, $2A
        LIT 72, $14
        LIT 73, $C2
        LIT 74, $41
        LIT 75, $30
        LIT 76, $C2

fixture_main:
        ldx #$00                ; the pages the checks read are filled with $FF, then every cell that a check reads is written
        lda #$FF
fill:   sta a:$2000,x
        sta a:$2500,x
        sta a:$2600,x
        sta a:$2700,x
        inx
        bne fill
        lda #$01                ; the operand cells of the mode-coverage checks
        sta a:$2000+ZPA
        lda #$02
        sta a:$2000+ZPA+3
        lda #<RAM_P0
        sta a:$2000+ZP_PTR
        lda #>RAM_P0
        sta a:$2000+ZP_PTR+1
        lda #<(RAM_P0+1)
        sta a:$2000+ZP_PTR2
        lda #>(RAM_P0+1)
        sta a:$2000+ZP_PTR2+1
        lda #$08
        sta a:RAM_P0
        lda #$04
        sta a:RAM_P0+1
        lda #$10
        sta a:RAM_P0+3
        lda #$80
        sta a:RAM_ABS
        lda #$20
        sta a:RAM_ABS+2
        lda #$40
        sta a:RAM_ABS+5
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
        lda #$00 ; check 59 ora-modes MAN:S8-58: ORA (zz,X), zz, (zz), zz,X, abs,Y and abs,X of the operand cells (see the file header): A = $00 ORed with the six cells and with zz again gives $6F (S8-58): the cells $04, $01, $08, $02, $40 and $20 hold no bit $10
        ldx #$04
        ora (<ZP_PTR,x)
        ora z:ZPA
        ora (<ZP_PTR)
        ldx #$03
        ora z:ZPA,x
        ldy #$05
        ora a:RAM_ABS,y
        ldx #$02
        ora a:RAM_ABS,x
        ora z:ZPA
        CHECK_LIT 59
        stz z:ACC ; check 60 and-modes MAN:S8-18: AND (zz,X), (zz),Y, (zz), zz,X, abs,Y and abs,X with A = $3F each time (S8-18): the ORed results are $04, $10, $08, $02, $00 and $20, so $3E
        lda #$3F
        ldx #$04
        and (<ZP_PTR,x)
        ora z:ACC
        sta z:ACC
        lda #$3F
        ldy #$03
        and (<ZP_PTR),y
        ora z:ACC
        sta z:ACC
        lda #$3F
        and (<ZP_PTR)
        ora z:ACC
        sta z:ACC
        lda #$3F
        ldx #$03
        and z:ZPA,x
        ora z:ACC
        sta z:ACC
        lda #$3F
        ldy #$05
        and a:RAM_ABS,y
        ora z:ACC
        sta z:ACC
        lda #$3F
        ldx #$02
        and a:RAM_ABS,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 60
        lda #$FF ; check 61 eor-modes MAN:S8-47: EOR (zz,X), zz, (zz),Y, (zz), zz,X and abs,Y then zz again from A = $FF (S8-47): $FF EOR $5F EOR $01 gives $A1
        ldx #$04
        eor (<ZP_PTR,x)
        eor z:ZPA
        ldy #$03
        eor (<ZP_PTR),y
        eor (<ZP_PTR)
        ldx #$03
        eor z:ZPA,x
        ldy #$05
        eor a:RAM_ABS,y
        eor z:ZPA
        CHECK_LIT 61
        clc ; check 62 adc-modes MAN:S8-17: ADC (zz),Y, (zz) and (zz) again with C clear from A = $00 (S8-17): $10 + $08 + $08 gives $20
        lda #$00
        ldy #$03
        adc (<ZP_PTR),y
        adc (<ZP_PTR)
        adc (<ZP_PTR)
        CHECK_LIT 62
        sec ; check 63 sbc-modes MAN:S8-74: SBC (zz,X), zz, (zz),Y, (zz), zz,X, abs,Y and abs,X with C set from A = $FF (S8-74): $FF minus $04, $01, $10, $08, $02, $40 and $20 gives $80
        lda #$FF
        ldx #$04
        sbc (<ZP_PTR,x)
        sbc z:ZPA
        ldy #$03
        sbc (<ZP_PTR),y
        sbc (<ZP_PTR)
        ldx #$03
        sbc z:ZPA,x
        ldy #$05
        sbc a:RAM_ABS,y
        ldx #$02
        sbc a:RAM_ABS,x
        CHECK_LIT 63
        stz z:ACC ; check 64 cmp-modes MAN:S8-41: CMP (zz,X), zz, (zz),Y, (zz), abs,Y and abs,X with A equal to the cell (S8-41): Z and C set each time, so the ORed cell values give $7D
        lda #$04
        ldx #$04
        cmp (<ZP_PTR,x)
        CMPHIT $04
        lda #$01
        cmp z:ZPA
        CMPHIT $01
        lda #$10
        ldy #$03
        cmp (<ZP_PTR),y
        CMPHIT $10
        lda #$08
        cmp (<ZP_PTR)
        CMPHIT $08
        lda #$40
        ldy #$05
        cmp a:RAM_ABS,y
        CMPHIT $40
        lda #$20
        ldx #$02
        cmp a:RAM_ABS,x
        CMPHIT $20
        lda z:ACC
        CHECK_LIT 64
        ldx #$01 ; check 65 cpx-zp MAN:S8-42: CPX zero page with X equal to the cell $01 (S8-42): Z and C set, N clear, the masked P byte is $03
        cpx z:ZPA
        PMASK $83
        CHECK_LIT 65
        ldy #$80 ; check 66 cpy-abs MAN:S8-43: CPY absolute with Y equal to the cell $80 (S8-43): Z and C set, N clear, the masked P byte is $03
        cpy a:RAM_ABS
        PMASK $83
        CHECK_LIT 66
        stz z:ACC ; check 67 asl-modes MAN:S8-19: ASL absolute ($01 gives $02), zero page,X ($04 gives $08) and absolute,X ($10 gives $20) shift left (S8-19): the ORed results are $2A
        lda #$01
        sta a:RAM_W
        asl a:RAM_W
        lda a:RAM_W
        ora z:ACC
        sta z:ACC
        ldx #$03
        lda #$04
        sta z:ZPW,x
        asl z:ZPW,x
        lda z:ZPW,x
        ora z:ACC
        sta z:ACC
        ldx #$02
        lda #$10
        sta a:RAM_W,x
        asl a:RAM_W,x
        lda a:RAM_W,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 67
        stz z:ACC ; check 68 rol-modes MAN:S8-68: ROL zero page ($01 gives $03), absolute ($04 gives $09) and absolute,X ($10 gives $21) with C set rotate left through C (S8-68): the ORed results are $2B
        lda #$01
        sta z:ZPW
        sec
        rol z:ZPW
        lda z:ZPW
        ora z:ACC
        sta z:ACC
        lda #$04
        sta a:RAM_W
        sec
        rol a:RAM_W
        lda a:RAM_W
        ora z:ACC
        sta z:ACC
        ldx #$02
        lda #$10
        sta a:RAM_W,x
        sec
        rol a:RAM_W,x
        lda a:RAM_W,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 68
        stz z:ACC ; check 69 lsr-modes MAN:S8-56: LSR zero page ($02 gives $01), zero page,X ($08 gives $04) and absolute,X ($20 gives $10) shift right (S8-56): the ORed results are $15
        lda #$02
        sta z:ZPW
        lsr z:ZPW
        lda z:ZPW
        ora z:ACC
        sta z:ACC
        ldx #$03
        lda #$08
        sta z:ZPW,x
        lsr z:ZPW,x
        lda z:ZPW,x
        ora z:ACC
        sta z:ACC
        ldx #$02
        lda #$20
        sta a:RAM_W,x
        lsr a:RAM_W,x
        lda a:RAM_W,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 69
        stz z:ACC ; check 70 ror-modes MAN:S8-69: ROR zero page ($02 gives $81), absolute ($08 gives $84) and zero page,X ($20 gives $90) with C set rotate right through C (S8-69): the ORed results are $95
        lda #$02
        sta z:ZPW
        sec
        ror z:ZPW
        lda z:ZPW
        ora z:ACC
        sta z:ACC
        lda #$08
        sta a:RAM_W
        sec
        ror a:RAM_W
        lda a:RAM_W
        ora z:ACC
        sta z:ACC
        ldx #$03
        lda #$20
        sta z:ZPW,x
        sec
        ror z:ZPW,x
        lda z:ZPW,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 70
        stz z:ACC ; check 71 dec-modes MAN:S8-44: DEC zero page ($03 gives $02), absolute ($09 gives $08) and zero page,X ($21 gives $20) subtract one (S8-44): the ORed results are $2A
        lda #$03
        sta z:ZPW
        dec z:ZPW
        lda z:ZPW
        ora z:ACC
        sta z:ACC
        lda #$09
        sta a:RAM_W
        dec a:RAM_W
        lda a:RAM_W
        ora z:ACC
        sta z:ACC
        ldx #$03
        lda #$21
        sta z:ZPW,x
        dec z:ZPW,x
        lda z:ZPW,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 71
        stz z:ACC ; check 72 inc-modes MAN:S8-48: INC zero page,X ($03 gives $04) and absolute,X ($0F gives $10) add one (S8-48): the ORed results are $14
        ldx #$03
        lda #$03
        sta z:ZPW,x
        inc z:ZPW,x
        lda z:ZPW,x
        ora z:ACC
        sta z:ACC
        ldx #$02
        lda #$0F
        sta a:RAM_W,x
        inc a:RAM_W,x
        lda a:RAM_W,x
        ora z:ACC
        sta z:ACC
        lda z:ACC
        CHECK_LIT 72
        lda #$C0 ; check 73 bit-abs MAN:S8-25: BIT absolute on the cell $C0 with A = $00 and C clear (S8-25): N = M7 = 1, V = M6 = 1, Z = 1 and C unchanged, the masked P byte is $C2
        sta a:RAM_W
        SETP $00
        lda #$00
        bit a:RAM_W
        PMASK $C3
        CHECK_LIT 73
        ldx #$03 ; check 74 bit-zpx MAN:S8-25: BIT zero page,X (X = 3) on the cell $40 with A = $FF and C set (S8-25): N = 0, V = 1, Z = 0 and C unchanged, the masked P byte is $41
        lda #$40
        sta z:ZPW,x
        SETP $01
        lda #$FF
        bit z:ZPW,x
        PMASK $C3
        CHECK_LIT 74
        lda #$3C ; check 75 trb-abs ASSUMED:LED-CPU-0010: TRB absolute on the cell $3C with A = $0F stores not A AND M (the formula of S8-102; its prose says the result is not stored, the stored reading is the project's): the cell reads $30
        sta a:RAM_W
        lda #$0F
        trb a:RAM_W
        lda a:RAM_W
        CHECK_LIT 75
        lda #$F0 ; check 76 tst-abs MAN:S8-104: TST #imm,absolute with the immediate $0F on the cell $F0 (S8-104): Z = 1 (the AND is zero), N = M7 = 1, V = M6 = 1 and C unchanged with C clear, the masked P byte is $C2
        sta a:RAM_W
        SETP $00
        tst #$0F, a:RAM_W
        PMASK $C3
        CHECK_LIT 76
        rts
