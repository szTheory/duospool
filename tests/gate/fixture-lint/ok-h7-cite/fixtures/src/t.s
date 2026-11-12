; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: two registered H7 and H6 pages, check 1 MAN:H7-4 and check 2 MAN:H6-3
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 2

fixture_main:
        lda a:kv
        CHECK_LIT 1
        lda a:kv
        CHECK_LIT 2
        rts
kv:     .byte $A5
