; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
; fixture-lint control mini-root: ldq is not a mnemonic of the opcode table
        .setcpu "huc6280"
DSV_ID = 1
DSV_CHECKS = 1

fixture_main:
        ldq #$01
        CHECK_LIT 1
        rts
