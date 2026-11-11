; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: FOOBAR is neither a mnemonic nor a macro, whatever its case
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 1

fixture_main:
        FOOBAR $44
        CHECK_LIT 1
        rts
