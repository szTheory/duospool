; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: lda $20 has no size hint
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 1

fixture_main:
        lda $20
        CHECK_LIT 1
        rts
