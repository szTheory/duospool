; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; ds_proto.s - the first authored fixture, now on the shared fixture library (plans 03-01 and 03-05). It is assembled by ca65 in the gate and never
; committed as a binary. Eight checks in ladder order (logic, flags, arithmetic, then one addressing check) write a DSV1 verdict block into work
; RAM, which the runner judges from raw RAM. Project-owned reset code only (fixtures/lib): no HuC, PCEAS, emulator or cc65 library code.
; The expected values are in the literal table of check.inc; ds_proto.checks lists each check with the class of its expected value.
; The same source builds the four DSV1 protocol-control variants through the -D switches DSV_PREDONE, DSV_NODONE, DSV_BADSUM and DSV_WRONGID
; (scripts/fixture-build, fixtures/lib/verdict.inc); none is defined in the normal build.
        .setcpu "huc6280"

DSV_ID = 1
DSV_CHECKS = 8

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; SELFTEST-TARGET
        LIT 2, $00
        LIT 3, $16
        LIT 4, $17
        LIT 5, $94
        LIT 6, $80
        LIT 7, $D4
        LIT 8, $00

fixture_main:
; ---- logic rung: EOR ----
        lda a:kv                        ; check 1 GUEST:xor-involution: (v EOR k) EOR k gives v back
        sta a:GEXP
        eor a:kk
        eor a:kk
        CHECK_GUEST 1
        lda #$00                        ; check 2 GUEST:xor-self: v EOR v gives zero
        sta a:GEXP
        lda a:kv
        eor a:kv
        CHECK_GUEST 2
; ---- flags rung: PHP puts P on the stack with B = 1 and T = 0, I is set by the reset code, ADC of 0 and 0 gives N V C clear and Z set ----
        lda #$00                        ; check 3 MAN:H8-5: P after A = 0 + 0 is Z and I, B reads 1 through PHP; PLP then PHP gives the same byte
        clc
        adc #$00
        php
        plp
        php
        pla
        CHECK_LIT 3
        lda #$00                        ; check 4 ASSUMED:LED-CPU-0010: CMP of equal values sets Z and C
        clc
        adc #$00
        lda #$40
        cmp #$40
        php
        pla
        CHECK_LIT 4
        lda #$00                        ; check 5 ASSUMED:LED-CPU-0010: CMP of a smaller value clears Z and C and sets N
        clc
        adc #$00
        lda #$10
        cmp #$40
        php
        pla
        CHECK_LIT 5
; ---- arithmetic rung: MAN:S8-17, ADC $7F + $01 gives $80 with V set ----
        lda #$7F                        ; check 6 MAN:S8-17: the sum
        clc
        adc #$01
        CHECK_LIT 6
        lda #$7F                        ; check 7 MAN:S8-17: N and V set, Z and C clear (with B = 1 and I = 1 from PHP and the reset code)
        clc
        adc #$01
        php
        pla
        CHECK_LIT 7
; ---- addressing: abs,X and abs,Y reach the same table byte, and PHA then PLA returns it ----
        ldx #$FF                        ; check 8 GUEST:index-and-stack
        lda a:tbl-$FC,x                 ; the model: the byte at tbl + 3 through abs,X
        sta a:GEXP
        ldy #$03
        lda a:tbl,y                     ; the observed byte: the same byte through abs,Y, kept across a push and pull
        pha
        lda #$00
        pla
        CHECK_GUEST 8
        rts

kv:     .byte $A5
kk:     .byte $3C
tbl:    .byte $11, $22, $33, $44
