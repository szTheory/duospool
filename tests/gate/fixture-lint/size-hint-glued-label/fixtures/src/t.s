; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: a label glued to lda $20 hides the instruction from a lexer that wants a blank after the colon
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 1

fixture_main:
glued:lda $20
        CHECK_LIT 1
        rts
