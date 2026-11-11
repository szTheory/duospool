; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: bad-observe. Row 2 is marked observe but its check compares (CHECK_LIT), and row 3 has a fifth column that is not the word observe.
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 3

fixture_main:
        lda a:kv
        CHECK_LIT 1
        CHECK_LIT 2
        CHECK_OBSERVE 3, a:kv
        rts
kv:     .byte $A5
