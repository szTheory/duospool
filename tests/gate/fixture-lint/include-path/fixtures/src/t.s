; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: an include from outside fixtures/lib
        .setcpu "huc6280"
        .include "../elsewhere/other.inc"
DSV_ID = 1
DSV_CHECKS = 1

fixture_main:
        lda a:kv
        CHECK_LIT 1
        rts
kv:     .byte $A5
