; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: checks= is 3 but two rows
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 2

fixture_main:
        lda a:kv
        CHECK_LIT 1
        CHECK_LIT 2
        rts
kv:     .byte $A5
