; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_ldst.s - the loads, stores, register transfers, register clears and stack operations with their addressing modes (plan 03-11; D-99, D-125, S8-11 and the instruction pages S8-n). It is assembled by ca65 in the gate
; and never committed as a binary. Checks run in ladder order: the loads first (they are what every later check reads with), then the transfers and clears, the other loads, the stores, the stack and last the wrap probes.
; Memory model of the library: zero page is logical $2000-$20FF, the stack is $2100-$21FF, the other cells are plain work RAM. A fixture initialises everything it reads (D-127), so the pages used here are filled with $FF first
; and every cell a check reads is then written by the fixture itself. Every operand below $100 carries z:, a: or <. P as seen through PHP then PLA is N and Z plus $14: the reset code sets I (H8-18, $04), PHP pushes B as 1
; ($10, H8-5), T reads 0, D is clear and V stays clear because nothing here overflows; C is cleared by the CLC of PFLAGS. The expected values are in the literal table of check.inc and are classed in cpu_ldst.checks.
; This file is generated from a table of checks by a scratch script; the committed text is the source of record.
        .setcpu "huc6280"

DSV_ID = 3
DSV_CHECKS = 82

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

SSAVE   = $2403                 ; the caller's S, kept in work RAM outside the filled pages
; the cells of the loads
ZP_A    = $10                   ; zero page: the base of the plain and the indexed zero-page reads ($11 at +0, $12 at +3)
ZP_PTR  = $30                   ; a zero-page pointer to RAM_P0
ZP_PTR2 = $34                   ; a zero-page pointer to RAM_P1 (reached through (zp,X) with X = 4)
ZP_PTR3 = $38                   ; a zero-page pointer to RAM_FZ
RAM_P0  = $2600                 ; the target of (zp) and the base of (zp),Y
RAM_P1  = $2601                 ; the target of (zp,X)
RAM_ABS = $2500                 ; the base of the absolute and the absolute indexed reads
RAM_FN  = $2510                 ; $80: a load of it sets N
RAM_FZ  = $2511                 ; $00: a load of it sets Z
; the cells of the stores (all filled with $FF before the first store)
ZP_PTRW = $3A                   ; a zero-page pointer to RAM_ST; the pointer after it names RAM_ST+1
ZP_ST   = $40                   ; STA zero-page targets
ZP_STX  = $50                   ; STX zero-page targets
ZP_STY  = $60                   ; STY zero-page targets
ZP_STZ  = $70                   ; STZ zero-page targets
RAM_ST  = $2700                 ; absolute and indirect store targets

; P of a flag probe: CLC first (C is not under test and the check macros leave it undefined), then PHP, PLA
        .macro PFLAGS
        clc
        php
        pla
        .endmacro

        CHECK_TABLE DSV_CHECKS

        LIT 1, $5A
        LIT 2, $94
        LIT 3, $16
        LIT 4, $14
        LIT 5, $11
        LIT 6, $12
        LIT 7, $21
        LIT 8, $22
        LIT 9, $23
        LIT 10, $B1
        LIT 11, $B2
        LIT 12, $B3
        LIT 13, $94
        LIT 14, $16
        LIT 15, $7C
        LIT 16, $3D
        LIT 17, $94
        LIT 18, $16
        LIT 19, $94
        LIT 20, $EC
        LIT 21, $94
        LIT 22, $14
        LIT 23, $00
        LIT 24, $00
        LIT 25, $00
        LIT 26, $94
        LIT 27, $6B
        LIT 28, $11
        LIT 29, $12
        LIT 30, $B1
        LIT 31, $B3
        LIT 32, $94
        LIT 33, $3D
        LIT 34, $11
        LIT 35, $12
        LIT 36, $B1
        LIT 37, $B2
        LIT 38, $16
        LIT 39, $A1
        LIT 40, $A2
        LIT 41, $A3
        LIT 42, $A4
        LIT 43, $A5
        LIT 44, $A6
        LIT 45, $A7
        LIT 46, $A8
        LIT 47, $16
        LIT 48, $C1
        LIT 49, $C2
        LIT 50, $C3
        LIT 51, $16
        LIT 52, $D1
        LIT 53, $D2
        LIT 54, $D3
        LIT 55, $00
        LIT 56, $00
        LIT 57, $00
        LIT 58, $00
        LIT 59, $94
        LIT 60, $B7
        LIT 61, $94
        LIT 62, $16
        LIT 63, $B8
        LIT 64, $94
        LIT 65, $B9
        LIT 66, $16
        LIT 67, $16
        LIT 68, $D5
        LIT 69, $16
        LIT 70, $11
        LIT 71, $E1
        LIT 72, $EE
        LIT 73, $11
        LIT 74, $F1
        LIT 75, $21
        LIT 76, $23
        LIT 77, $00
        LIT 78, $00
        LIT 79, $FE
        LIT 80, $A1
        LIT 81, $A2
        LIT 82, $A1

fixture_main:
        tsx                     ; the S of the caller: the checks that move S put it back from here
        stx a:SSAVE
; ---- the cells: the four pages are filled with $FF, then every cell that a check reads is written (absolute stores, so the check that reads it through another mode proves the mode) ----
        ldx #$00
        lda #$FF
fill:   sta a:$2000,x
        sta a:RAM_ABS,x
        sta a:RAM_P0,x
        sta a:RAM_ST,x
        inx
        bne fill
        lda #$11
        sta a:$2000+ZP_A
        lda #$12
        sta a:$2000+ZP_A+3
        lda #<RAM_P0            ; ZP_PTR points to RAM_P0
        sta a:$2000+ZP_PTR
        lda #>RAM_P0
        sta a:$2000+ZP_PTR+1
        lda #<RAM_P1            ; ZP_PTR2 points to RAM_P1
        sta a:$2000+ZP_PTR2
        lda #>RAM_P1
        sta a:$2000+ZP_PTR2+1
        lda #<RAM_FZ            ; ZP_PTR3 points to RAM_FZ
        sta a:$2000+ZP_PTR3
        lda #>RAM_FZ
        sta a:$2000+ZP_PTR3+1
        lda #<RAM_ST            ; ZP_PTRW points to RAM_ST and the next pointer to RAM_ST+1
        sta a:$2000+ZP_PTRW
        lda #>RAM_ST
        sta a:$2000+ZP_PTRW+1
        lda #<(RAM_ST+1)
        sta a:$2000+ZP_PTRW+2
        lda #>(RAM_ST+1)
        sta a:$2000+ZP_PTRW+3
        lda #$21
        sta a:RAM_P0
        lda #$22
        sta a:RAM_P1
        lda #$23
        sta a:RAM_P0+3
        lda #$B1
        sta a:RAM_ABS
        lda #$B2
        sta a:RAM_ABS+2
        lda #$B3
        sta a:RAM_ABS+5
        lda #$80
        sta a:RAM_FN
        lda #$00
        sta a:RAM_FZ
        lda #<RAM_P0            ; the pointer at $20FF and $2000 (wrapped) names $2600
        sta a:$2000+$FF
        lda #>RAM_P0
        sta a:$2000
        lda #$27                ; the pointer high byte a missing wrap would use, at $2100
        sta a:$2100
        lda #<(RAM_P0+$80)      ; the (zz,X) probe: $98 + $F0 wraps to $88, a pointer to $2680 ($F1); the unwrapped $2188 names $2681 ($F2)
        sta a:$2000+$88
        lda #>(RAM_P0+$80)
        sta a:$2000+$89
        lda #$F1
        sta a:RAM_P0+$80
        lda #<(RAM_P0+$81)
        sta a:$2188
        lda #>(RAM_P0+$81)
        sta a:$2189
        lda #$F2
        sta a:RAM_P0+$81
; ---- LDA, every mode (S8-53, S8-11): the loaded byte, then N and Z ----
        lda #$5A                ; check 1 lda-imm MAN:S8-53: LDA immediate: A takes the immediate byte ($5A)
        CHECK_LIT 1
        lda #$80                ; check 2 lda-imm-flag-n MAN:S8-53: LDA of $80 sets N and clears Z: P byte $94 (N, B, I)
        PFLAGS
        CHECK_LIT 2
        lda #$00                ; check 3 lda-imm-flag-z MAN:S8-53: LDA of $00 sets Z and clears N: P byte $16 (Z, B, I)
        PFLAGS
        CHECK_LIT 3
        lda #$01                ; check 4 lda-imm-flag-pos MAN:S8-53: LDA of $01 clears N and Z: P byte $14
        PFLAGS
        CHECK_LIT 4
        lda z:ZP_A              ; check 5 lda-zp MAN:S8-11: LDA zero page reads logical $2000 plus the operand (the cell $2010 holds $11)
        CHECK_LIT 5
        ldx #$03                ; check 6 lda-zpx MAN:S8-11: LDA zero page,X reads $2000 plus operand plus X (X = 3 reads the cell $2013 that holds $12)
        lda z:ZP_A,x
        CHECK_LIT 6
        lda (<ZP_PTR)           ; check 7 lda-ind MAN:S8-53: LDA (zz) reads the byte that the pointer in zero page names ($21 at $2600)
        CHECK_LIT 7
        ldx #$04                ; check 8 lda-indx MAN:S8-53: LDA (zz,X) takes the pointer from zz plus X (X = 4: the pointer at $34 names $2601, which holds $22)
        lda (<ZP_PTR,x)
        CHECK_LIT 8
        ldy #$03                ; check 9 lda-indy MAN:S8-53: LDA (zz),Y adds Y to the pointer (Y = 3: $2603 holds $23)
        lda (<ZP_PTR),y
        CHECK_LIT 9
        lda a:RAM_ABS           ; check 10 lda-abs MAN:S8-53: LDA absolute ($B1 at $2500)
        CHECK_LIT 10
        ldx #$02                ; check 11 lda-absx MAN:S8-53: LDA absolute,X (X = 2: $B2 at $2502)
        lda a:RAM_ABS,x
        CHECK_LIT 11
        ldy #$05                ; check 12 lda-absy MAN:S8-53: LDA absolute,Y (Y = 5: $B3 at $2505)
        lda a:RAM_ABS,y
        CHECK_LIT 12
        lda a:RAM_FN            ; check 13 lda-abs-flag-n MAN:S8-53: LDA absolute of a byte with bit 7 set sets N: P byte $94
        PFLAGS
        CHECK_LIT 13
        lda (<ZP_PTR3)          ; check 14 lda-ind-flag-z MAN:S8-53: LDA (zz) of a zero byte sets Z: P byte $16
        PFLAGS
        CHECK_LIT 14
; ---- Register transfers and clears (S8-91, S8-92, S8-105 to S8-108, S8-34, S8-39, S8-40): the register is read back through the opposite transfer ----
        lda #$7C                ; check 15 tax-txa MAN:S8-91: TAX copies A to X: A = $7C, TAX, A cleared, TXA gives $7C
        tax
        lda #$00
        txa
        CHECK_LIT 15
        lda #$3D                ; check 16 tay-tya MAN:S8-92: TAY copies A to Y: A = $3D, TAY, A cleared, TYA gives $3D
        tay
        lda #$00
        tya
        CHECK_LIT 16
        lda #$80                ; check 17 tax-flag-n MAN:S8-91: TAX sets N from the moved value: A = $80 with N cleared by LDY first, P byte $94
        ldy #$01
        tax
        PFLAGS
        CHECK_LIT 17
        lda #$00                ; check 18 tay-flag-z MAN:S8-92: TAY sets Z from the moved value: A = $00 with Z cleared by LDX first, P byte $16
        ldx #$01
        tay
        PFLAGS
        CHECK_LIT 18
        ldx #$80                ; check 19 txa-flag-n MAN:S8-106: TXA sets N from the moved value: X = $80 with N cleared by LDA first, P byte $94
        lda #$01
        txa
        PFLAGS
        CHECK_LIT 19
        ldx #$EC                ; check 20 txs-tsx MAN:S8-107: TXS copies X to S and TSX copies it back: X = $EC, TXS, X cleared, TSX, TXA gives $EC
        txs
        ldx #$00
        tsx
        txa
        ldx a:SSAVE
        txs
        CHECK_LIT 20
        ldx #$EC                ; check 21 tsx-flag-n MAN:S8-105: TSX sets N from S: S = $EC, X = $01 clears N first, P byte $94
        txs
        ldx #$01
        tsx
        PFLAGS
        ldx a:SSAVE
        txs
        CHECK_LIT 21
        ldx #$80                ; check 22 txs-no-flags MAN:S8-107: TXS changes no flag: X = $80, N cleared by LDA, TXS, P byte $14 (N stays clear)
        lda #$01
        txs
        PFLAGS
        ldx a:SSAVE
        txs
        CHECK_LIT 22
        lda #$55                ; check 23 cla-value MAN:S8-34: CLA clears A
        cla
        CHECK_LIT 23
        ldx #$55                ; check 24 clx-value MAN:S8-39: CLX clears X (read back with TXA)
        clx
        txa
        CHECK_LIT 24
        ldy #$55                ; check 25 cly-value MAN:S8-40: CLY clears Y (read back with TYA)
        cly
        tya
        CHECK_LIT 25
        ldy #$80                ; check 26 cla-no-flags MAN:S8-34: CLA changes no flag: N set by LDY, CLA, P byte $94 (Z stays clear)
        cla
        PFLAGS
        CHECK_LIT 26
; ---- LDX (S8-54), read back through TXA ----
        ldx #$6B                ; check 27 ldx-imm MAN:S8-54: LDX immediate
        txa
        CHECK_LIT 27
        ldx z:ZP_A              ; check 28 ldx-zp MAN:S8-11: LDX zero page ($2010 holds $11)
        txa
        CHECK_LIT 28
        ldy #$03                ; check 29 ldx-zpy MAN:S8-11: LDX zero page,Y reads $2000 plus operand plus Y (Y = 3: $2013 holds $12)
        ldx z:ZP_A,y
        txa
        CHECK_LIT 29
        ldx a:RAM_ABS           ; check 30 ldx-abs MAN:S8-54: LDX absolute ($B1 at $2500)
        txa
        CHECK_LIT 30
        ldy #$05                ; check 31 ldx-absy MAN:S8-54: LDX absolute,Y (Y = 5: $B3 at $2505)
        ldx a:RAM_ABS,y
        txa
        CHECK_LIT 31
        ldx a:RAM_FN            ; check 32 ldx-flag-n MAN:S8-54: LDX of a byte with bit 7 set sets N: P byte $94
        PFLAGS
        CHECK_LIT 32
; ---- LDY (S8-55), read back through TYA ----
        ldy #$3D                ; check 33 ldy-imm MAN:S8-55: LDY immediate
        tya
        CHECK_LIT 33
        ldy z:ZP_A              ; check 34 ldy-zp MAN:S8-11: LDY zero page ($2010 holds $11)
        tya
        CHECK_LIT 34
        ldx #$03                ; check 35 ldy-zpx MAN:S8-11: LDY zero page,X (X = 3: $2013 holds $12)
        ldy z:ZP_A,x
        tya
        CHECK_LIT 35
        ldy a:RAM_ABS           ; check 36 ldy-abs MAN:S8-55: LDY absolute ($B1 at $2500)
        tya
        CHECK_LIT 36
        ldx #$02                ; check 37 ldy-absx MAN:S8-55: LDY absolute,X (X = 2: $B2 at $2502)
        ldy a:RAM_ABS,x
        tya
        CHECK_LIT 37
        ldy a:RAM_FZ            ; check 38 ldy-flag-z MAN:S8-55: LDY of a zero byte sets Z: P byte $16
        PFLAGS
        CHECK_LIT 38
; ---- STA (S8-83): the cell is filled with $FF first and read back with an absolute load ----
        lda #$A1                ; check 39 sta-zp MAN:S8-83: STA zero page stores A at $2000 plus the operand ($2040)
        sta z:ZP_ST
        lda a:$2000+ZP_ST
        CHECK_LIT 39
        lda #$A2                ; check 40 sta-zpx MAN:S8-83: STA zero page,X (X = 3 stores at $2043)
        ldx #$03
        sta z:ZP_ST,x
        lda a:$2000+ZP_ST+3
        CHECK_LIT 40
        lda #$A3                ; check 41 sta-ind MAN:S8-83: STA (zz) stores through the pointer at $3A ($2700)
        sta (<ZP_PTRW)
        lda a:RAM_ST
        CHECK_LIT 41
        lda #$A4                ; check 42 sta-indx MAN:S8-83: STA (zz,X) takes the pointer from zz plus X (X = 2: the pointer at $3C names $2701)
        ldx #$02
        sta (<ZP_PTRW,x)
        lda a:RAM_ST+1
        CHECK_LIT 42
        lda #$A5                ; check 43 sta-indy MAN:S8-83: STA (zz),Y adds Y to the pointer (Y = 3: $2703)
        ldy #$03
        sta (<ZP_PTRW),y
        lda a:RAM_ST+3
        CHECK_LIT 43
        lda #$A6                ; check 44 sta-abs MAN:S8-83: STA absolute ($2710)
        sta a:RAM_ST+$10
        lda a:RAM_ST+$10
        CHECK_LIT 44
        lda #$A7                ; check 45 sta-absx MAN:S8-83: STA absolute,X (X = 3: $2713)
        ldx #$03
        sta a:RAM_ST+$10,x
        lda a:RAM_ST+$13
        CHECK_LIT 45
        lda #$A8                ; check 46 sta-absy MAN:S8-83: STA absolute,Y (Y = 5: $2715)
        ldy #$05
        sta a:RAM_ST+$10,y
        lda a:RAM_ST+$15
        CHECK_LIT 46
        lda #$5A                ; check 47 sta-no-flags MAN:S8-83: STA changes no flag: A = $5A, Z set by LDX of $00, STA, P byte $16 (a store that set N and Z from A would give $14)
        ldx #$00
        sta a:RAM_ST+$20
        PFLAGS
        CHECK_LIT 47
; ---- STX (S8-84), STY (S8-85), STZ (S8-86) ----
        ldx #$C1                ; check 48 stx-zp MAN:S8-84: STX zero page ($2051)
        stx z:ZP_STX+1
        lda a:$2000+ZP_STX+1
        CHECK_LIT 48
        ldx #$C2                ; check 49 stx-zpy MAN:S8-84: STX zero page,Y (Y = 3 stores at $2053)
        ldy #$03
        stx z:ZP_STX,y
        lda a:$2000+ZP_STX+3
        CHECK_LIT 49
        ldx #$C3                ; check 50 stx-abs MAN:S8-84: STX absolute ($2730)
        stx a:RAM_ST+$30
        lda a:RAM_ST+$30
        CHECK_LIT 50
        ldx #$04                ; check 51 stx-no-flags MAN:S8-84: STX changes no flag: X = $04, Z set by LDA of $00, STX, P byte $16
        lda #$00
        stx a:RAM_ST+$31
        PFLAGS
        CHECK_LIT 51
        ldy #$D1                ; check 52 sty-zp MAN:S8-85: STY zero page ($2061)
        sty z:ZP_STY+1
        lda a:$2000+ZP_STY+1
        CHECK_LIT 52
        ldy #$D2                ; check 53 sty-zpx MAN:S8-85: STY zero page,X (X = 3 stores at $2063)
        ldx #$03
        sty z:ZP_STY,x
        lda a:$2000+ZP_STY+3
        CHECK_LIT 53
        ldy #$D3                ; check 54 sty-abs MAN:S8-85: STY absolute ($2740)
        sty a:RAM_ST+$40
        lda a:RAM_ST+$40
        CHECK_LIT 54
        stz z:ZP_STZ+1          ; check 55 stz-zp MAN:S8-86: STZ zero page stores $00 ($2071 was $FF)
        lda a:$2000+ZP_STZ+1
        CHECK_LIT 55
        ldx #$03                ; check 56 stz-zpx MAN:S8-86: STZ zero page,X (X = 3: $2073 was $FF)
        stz z:ZP_STZ,x
        lda a:$2000+ZP_STZ+3
        CHECK_LIT 56
        stz a:RAM_ST+$50        ; check 57 stz-abs MAN:S8-86: STZ absolute ($2750 was $FF)
        lda a:RAM_ST+$50
        CHECK_LIT 57
        ldx #$02                ; check 58 stz-absx MAN:S8-86: STZ absolute,X (X = 2: $2752 was $FF)
        stz a:RAM_ST+$50,x
        lda a:RAM_ST+$52
        CHECK_LIT 58
        lda #$80                ; check 59 stz-no-flags MAN:S8-86: STZ changes no flag: N set by LDA of $80, STZ, P byte $94 (a store that set Z from the zero it writes would give $16)
        stz a:RAM_ST+$54
        PFLAGS
        CHECK_LIT 59
; ---- Stack operations (S8-59 to S8-66, H8-5): the stack is $2100 plus S ----
        lda #$B7                ; check 60 pha-pla MAN:S8-59: PHA pushes A and PLA pulls it: A = $B7, PHA, A cleared, PLA gives $B7
        pha
        lda #$00
        pla
        CHECK_LIT 60
        lda #$80                ; check 61 pla-flag-n MAN:S8-63: PLA sets N from the pulled byte: $80 pulled, N cleared by LDA first, P byte $94
        pha
        lda #$01
        pla
        PFLAGS
        CHECK_LIT 61
        lda #$00                ; check 62 pla-flag-z MAN:S8-63: PLA sets Z from the pulled byte: $00 pulled, P byte $16
        pha
        lda #$01
        pla
        PFLAGS
        CHECK_LIT 62
        ldx #$B8                ; check 63 phx-plx MAN:S8-61: PHX pushes X and PLX pulls it: X = $B8, PHX, X cleared, PLX, TXA gives $B8
        phx
        ldx #$00
        plx
        txa
        CHECK_LIT 63
        ldx #$80                ; check 64 plx-flag-n MAN:S8-65: PLX sets N from the pulled byte: $80 pulled, P byte $94
        phx
        ldx #$01
        plx
        PFLAGS
        CHECK_LIT 64
        ldy #$B9                ; check 65 phy-ply MAN:S8-62: PHY pushes Y and PLY pulls it: Y = $B9, PHY, Y cleared, PLY, TYA gives $B9
        phy
        ldy #$00
        ply
        tya
        CHECK_LIT 65
        ldy #$00                ; check 66 ply-flag-z MAN:S8-66: PLY sets Z from the pulled byte: $00 pulled, P byte $16
        phy
        ldy #$01
        ply
        PFLAGS
        CHECK_LIT 66
        lda #$00                ; check 67 php-pushed-byte MAN:H8-5: PHP pushes P with B = 1 and T = 0: with Z and I set the byte on the stack is $16 (read from the stack page; PLX then restores S)
        clc
        php
        tsx
        lda a:$2101,x
        plx
        CHECK_LIT 67
        lda #$C5                ; check 68 plp-loads-flags MAN:S8-64: PLP restores N, V, I and C from the stack: $C5 pulled, PHP then PLA gives $D5 (B reads 1)
        pha
        plp
        php
        pla
        CHECK_LIT 68
        lda #$06                ; check 69 plp-loads-z MAN:S8-64: PLP restores Z: $06 pulled, PHP then PLA gives $16
        pha
        plp
        php
        pla
        CHECK_LIT 69
; ---- Wrap probes: the zero page and the stack wrap inside their pages (S8-11, H8-3; LED-CPU-0010 for the pointer forms) ----
        lda #$EE                ; check 70 zpx-wrap-load MAN:S8-11: LDA zero page,X with the sum above $FF wraps inside zero page: $20 + $F0 reads $2010 ($11), not $2110 ($EE)
        sta a:$2110
        sta a:$2180
        ldx #$F0
        lda z:$20,x
        CHECK_LIT 70
        lda #$E1                ; check 71 zpx-wrap-store MAN:S8-11: STA zero page,X wraps inside zero page: $90 + $F0 stores at $2080
        ldx #$F0
        sta z:$90,x
        lda a:$2080
        CHECK_LIT 71
        lda a:$2180             ; check 72 zpx-wrap-no-spill MAN:S8-11: the same store leaves $2180 alone ($EE written before the probe)
        CHECK_LIT 72
        ldy #$F0                ; check 73 zpy-wrap-load MAN:S8-11: LDX zero page,Y wraps inside zero page: $20 + $F0 reads $2010 ($11), not $2110
        ldx z:$20,y
        txa
        CHECK_LIT 73
        ldx #$F0                ; check 74 indx-wrap ASSUMED:LED-CPU-0010: LDA (zz,X) takes its pointer from the zero-page cell (zz + X) mod 256: $98 + $F0 names $2088, whose pointer reads $F1 at $2680, not the pointer at $2188
        lda (<$98,x)
        CHECK_LIT 74
        lda (<$FF)              ; check 75 ind-ff-pointer ASSUMED:LED-CPU-0010: LDA (zz) with zz = $FF takes the pointer high byte from $2000, not $2100: the pointer is $2600 ($21)
        CHECK_LIT 75
        ldy #$03                ; check 76 indy-ff-pointer ASSUMED:LED-CPU-0010: LDA (zz),Y with zz = $FF: pointer $2600 plus Y = 3 reads $23
        lda (<$FF),y
        CHECK_LIT 76
        lda #$00                ; check 77 absx-wrap GUEST:abs-index-wrap: LDA absolute,X wraps at $FFFF: $FFFF + 2 reads logical $0001 (MPR0 = bank 0), the byte that bank 0 shows at $E001; the model reads that byte at $E001
        tam #$01
        lda a:$E001
        sta a:GEXP
        ldx #$02
        lda a:$FFFF,x
        CHECK_GUEST 77
        lda a:$E002             ; check 78 absy-wrap GUEST:abs-index-wrap: LDA absolute,Y wraps at $FFFF: $FFFF + 3 reads logical $0002, which bank 0 shows at $E002
        sta a:GEXP
        ldy #$03
        lda a:$FFFF,y
        CHECK_GUEST 78
        lda a:$21FF             ; check 79 stack-wrap-s MAN:H8-3: a push with S = $00 writes $2100 and wraps S to $FF: two pushes leave S = $FE
        sta a:SCRATCH           ; the high byte of the return address of fixture_main sits at $21FF: kept, and put back before the pull probe
        tsx
        stx a:SSAVE          ; the S of the caller, put back after the probe
        ldx #$00
        txs
        lda #$A1
        pha
        lda #$A2
        pha
        tsx
        txa
        ldx a:SSAVE
        txs
        CHECK_LIT 79
        lda a:$2100             ; check 80 stack-wrap-low MAN:H8-3: the first push of the wrap probe wrote $2100
        CHECK_LIT 80
        lda a:$21FF             ; check 81 stack-wrap-high MAN:H8-3: the second push of the wrap probe wrote $21FF
        CHECK_LIT 81
        lda a:SCRATCH           ; check 82 stack-pull-wrap MAN:H8-3: a pull with S = $FF wraps S to $00 and reads $2100: the byte the first push left there
        sta a:$21FF             ; the return address of fixture_main is whole again
        tsx
        stx a:SSAVE
        ldx #$FF
        txs
        pla
        ldx a:SSAVE
        txs
        CHECK_LIT 82
        rts
