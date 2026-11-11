; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: a clean source: every rule met, one cited raw-byte line
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 3

        .include "startup.inc"
        .include "verdict.inc"
        .include "fixtures/lib/check.inc"

fixture_main:
        lda a:kv                        ; check 1 GUEST:copy
        sta a:GEXP
        CHECK_GUEST 1
        lda z:$20                       ; check 2 MAN:H8-5, explicit zero page
        CHECK_LIT 2
        lda (<$20)                      ; indirect through zero page
        bbr0 <$20, skip
        tst #$80, z:$20
        tii $1000, $2000, $0010         ; block transfer operands are absolute
        .byte $53, $01                  ; RAW:H8-8 TAM with a one-bit operand
skip:   lda #$10                        ; immediates need no hint
        CHECK_LIT 3
        rts
kv:     .byte $A5
