; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: a registered P8 page, check 1 MAN:P8-9, backing a supported entry (plan 05-01, F-74)
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 1

fixture_main:
        lda a:kv
        CHECK_LIT 1
        rts
kv:     .byte $A5
