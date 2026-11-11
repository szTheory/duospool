; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_ldst.s - the loads, stores, register transfers and stack operations with their addressing modes (plan 03-11; D-99, D-125, S8-11 and the instruction pages S8-n). It is assembled by ca65 in the gate
; and never committed as a binary. Checks run in ladder order: the loads first (they are what every later check reads with), then the transfers and clears, the other loads, the stores, the stack and last the wrap probes.
; Memory model of the library: zero page is logical $2000-$20FF, the stack is $2100-$21FF, the cells below are plain work RAM that this fixture writes before it reads (a fixture initialises everything it reads, D-127).
; Every operand below $100 carries z:, a: or <. P as seen through PHP then PLA is N and Z plus $14: the reset code sets I (H8-18, $04), PHP pushes B as 1 ($10, H8-5), T reads 0, D is clear and V stays clear because nothing here
; overflows; C is cleared by the CLC of PFLAGS. The expected values are in the literal table of check.inc and are classed in cpu_ldst.checks.
        .setcpu "huc6280"

DSV_ID = 3
DSV_CHECKS = 14

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

; the cells the fixture writes before it reads them
ZP_A    = $10                   ; zero page: the base of the plain and the indexed zero-page reads
ZP_PTR  = $30                   ; a zero-page pointer to RAM_P0
ZP_PTR2 = $34                   ; a zero-page pointer to RAM_P1 (reached through (zp,X) with X = 4)
ZP_PTR3 = $38                   ; a zero-page pointer to RAM_FZ
RAM_P0  = $2600                 ; the target of (zp) and the base of (zp),Y
RAM_P1  = $2601                 ; the target of (zp,X)
RAM_ABS = $2500                 ; the base of the absolute and the absolute indexed reads
RAM_FN  = $2510                 ; $80: a load of it sets N
RAM_FZ  = $2511                 ; $00: a load of it sets Z

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

fixture_main:
; ---- the cells: written with absolute stores, so no later check reads one that was not set (the checks read them back through the other modes) ----
        lda #$11
        sta a:$2000+ZP_A
        lda #$12
        sta a:$2000+ZP_A+3
        lda #$00                ; ZP_PTR points to RAM_P0
        sta a:$2000+ZP_PTR
        lda #$26
        sta a:$2000+ZP_PTR+1
        lda #$01                ; ZP_PTR2 points to RAM_P1
        sta a:$2000+ZP_PTR2
        lda #$26
        sta a:$2000+ZP_PTR2+1
        lda #<RAM_FZ            ; ZP_PTR3 points to RAM_FZ
        sta a:$2000+ZP_PTR3
        lda #>RAM_FZ
        sta a:$2000+ZP_PTR3+1
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
; ---- LDA, every mode (S8-53): the loaded byte, then N and Z ----
        lda #$5A                ; check 1 MAN:S8-53: A <- the immediate byte
        CHECK_LIT 1
        lda #$80                ; check 2 MAN:S8-53: a negative byte sets N and clears Z
        PFLAGS
        CHECK_LIT 2
        lda #$00                ; check 3 MAN:S8-53: zero sets Z and clears N
        PFLAGS
        CHECK_LIT 3
        lda #$01                ; check 4 MAN:S8-53: a positive byte clears N and Z
        PFLAGS
        CHECK_LIT 4
        lda z:ZP_A              ; check 5 MAN:S8-11: zero page is logical $2000 + zz
        CHECK_LIT 5
        ldx #$03                ; check 6 MAN:S8-11: zero page indexed by X
        lda z:ZP_A,x
        CHECK_LIT 6
        lda (<ZP_PTR)           ; check 7 MAN:S8-53: (zz) reads the byte the pointer names
        CHECK_LIT 7
        ldx #$04                ; check 8 MAN:S8-53: (zz,X) takes the pointer from zz + X
        lda (<ZP_PTR,x)
        CHECK_LIT 8
        ldy #$03                ; check 9 MAN:S8-53: (zz),Y adds Y to the pointer
        lda (<ZP_PTR),y
        CHECK_LIT 9
        lda a:RAM_ABS           ; check 10 MAN:S8-53: absolute
        CHECK_LIT 10
        ldx #$02                ; check 11 MAN:S8-53: absolute indexed by X
        lda a:RAM_ABS,x
        CHECK_LIT 11
        ldy #$05                ; check 12 MAN:S8-53: absolute indexed by Y
        lda a:RAM_ABS,y
        CHECK_LIT 12
        lda a:RAM_FN            ; check 13 MAN:S8-53: a load from memory sets N from bit 7 (absolute)
        PFLAGS
        CHECK_LIT 13
        lda (<ZP_PTR3)          ; check 14 MAN:S8-53: a load from memory sets Z ((zz) form)
        PFLAGS
        CHECK_LIT 14
        rts
