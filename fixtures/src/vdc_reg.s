; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_reg.s - the VDC register and VRAM fixture (plan 04-06; D-146, D-148, D-149, D-169, D-170, D-175, ER-10, F-26, F-27, H7-5, H7-25, H8-8). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF, as st_bypass does), so the VDC registers are at logical $0000 (status and AR), $0001, $0002 (data LSB) and $0003 (data MSB). ST0, ST1 and ST2 write the physical window without the mapping registers (H8-8: A1,A0 = 00, 10, 11).
; First slice: MAWR = $0100 (AR = 0), then AR = 2 (VWR): the word $1234 and the word $5678 are written by ST1 and ST2 (the high byte write stores the word and steps MAWR by 1, H7-5), MARR = $0100 (AR = 1; the high byte write starts the read), then AR = 2 (VRR) and four LDA reads: low and high of the first word, low and high of the second.
; The high byte read of VRR triggers the next read (H7-5, H7-25), so the second pair reads the second word. The VRAM bytes are drawn under a power-on policy and no check reads a word the guest did not write first (policy independent, D-167). The runner kind vdc_vram reads the same two words from the instance VRAM bytes against literals restated from this file.
; Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 16
DSV_CHECKS = 4

VDC_LO = $0002                  ; data LSB (a read returns the low byte of the read buffer)
VDC_HI = $0003                  ; data MSB (a read of VRR returns the high byte and triggers the next read)

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $34                      ; MAN:H7-5 the first VRR low read is the low byte of the word $1234 written at $0100
        LIT 2, $12                      ; MAN:H7-25 the first VRR high read is the high byte of the same word
        LIT 3, $78                      ; MAN:H7-5 the second VRR low read is the low byte of the word $5678 written at $0101
        LIT 4, $56                      ; MAN:H7-25 the second VRR high read is its high byte

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        st0 #$00                        ; AR = MAWR
        st1 #$00
        st2 #$01                        ; MAWR = $0100
        st0 #$02                        ; AR = VWR
        st1 #$34                        ; the write latch takes the low byte
        st2 #$12                        ; the high byte stores the word $1234 at $0100 and steps MAWR to $0101 (H7-5)
        st1 #$78
        st2 #$56                        ; the word $5678 at $0101, MAWR to $0102
        st0 #$01                        ; AR = MARR
        st1 #$00
        st2 #$01                        ; MARR = $0100: the high byte write starts the read (H7-5)
        st0 #$02                        ; AR = VRR
        lda a:VDC_LO
        CHECK_LIT 1                     ; check 1 MAN:H7-5 the low byte of the first word
        lda a:VDC_HI
        CHECK_LIT 2                     ; check 2 MAN:H7-25 the high byte of the first word; this read triggers the next one
        lda a:VDC_LO
        CHECK_LIT 3                     ; check 3 MAN:H7-5 the low byte of the second word
        lda a:VDC_HI
        CHECK_LIT 4                     ; check 4 MAN:H7-25 the high byte of the second word
        rts
