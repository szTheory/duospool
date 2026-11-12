; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vce_pal.s - the VCE palette and control register fixture, first slice (plan 04-04; D-137, D-141, D-152, D-153, D-169, D-170, ER-10, F-25, H6-3, H6-5). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF, as st_bypass does), so the VCE registers are at logical $0400 (CR), $0402 (CTA low), $0403 (CTA high) and $0404 and $0405 (CTW and CTR low and high); the register of an access is the offset & 7 (H6-2).
; Sequence: write CR $00 (the division ratio 4 of H6-3), run a counted delay of at least 5,460 master clocks, write CR $01 (the division ratio 3 of H6-3), then write the colour table: entry i = P(i) = (91 * i + 17) & 511 for all 512 entries by the procedure of H6-5 (CTA low, CTA high, data low, data high; the address is incremented after the high byte, so the
; following entries need no further address write), and read entries 0 and $100 back. 91 is odd, so P is a bijection of 0..511 (a later slice of this fixture checks all 512 entries against a second computation). Entry $100 is the border of the line record: P(256) = (91 * 256 + 17) & 511 = 23313 & 511 = 23313 - 45 * 512 = 273.
; The delay: LDX #128 (2 cycles), 127 passes of DEX (2) and BNE taken (2 + 2 for the taken branch = 4), and a last pass of DEX (2) and BNE not taken (2): 2 + 127 * 6 + 4 = 768 CPU cycles (tests/oracle/huc6280-opcodes.tsv: LDX imm 2, DEX 2, BNE 2 plus the taken-branch adder of 2). The fixture never executes CSH, so the CPU runs at the power-up low speed, 12 master clocks
; per cycle (constants.def cycle_mclk_low, community-reported, ASSUMED LED-CPU-0003): 768 * 12 = 9,216 master clocks, above the 5,460 (four lines of 1,365) that the runner needs so that at least one whole line record lies between the two CR writes with a line of margin on each side. At the high speed (3 master clocks) it would be 2,304, so the fixture must never switch speed.
; The tail delay at the end of the fixture keeps the guest alive for about 13 more lines. The checks of this slice read the table back (MAN H6-5: the table is readable and writable, low byte first; the values are the P(i) the guest wrote, 17 = $11 for entry 0, and 273 = $111 for entry $100). Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 15
DSV_CHECKS = 4

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $11                      ; MAN:H6-5 entry 0 low byte = P(0) = 17
        LIT 2, $00                      ; MAN:H6-5 entry 0 bit 8 = 0 (P(0) = 17 < 256)
        LIT 3, $11                      ; MAN:H6-5 entry $100 low byte = P(256) & $FF = 273 & $FF = $11
        LIT 4, $01                      ; MAN:H6-5 entry $100 bit 8 = 1 (273 >= 256)

VCE_CR = $0400                  ; logical addresses while MPR0 = $FF: the VCE registers of the hardware page
VCE_CTA_LO = $0402
VCE_CTA_HI = $0403
VCE_CTW_LO = $0404
VCE_CTW_HI = $0405
PSUM_LO = $2410                 ; the running value P(i) of the writer, low byte and bit 8 (work RAM through MPR1)
PSUM_HI = $2411

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        stz a:VCE_CR                    ; CR $00: division ratio 4 (MAN H6-3)
        ldx #128                        ; the counted delay: 768 CPU cycles = 9,216 master clocks at the low speed (see the header)
delay:  dex
        bne delay
        lda #$01
        sta a:VCE_CR                    ; CR $01: division ratio 3 (MAN H6-3)
; the 512 entries by the additive running sum P(i + 1) = P(i) + 91 mod 512 starting from P(0) = 17
        stz a:VCE_CTA_LO                ; CTA = 0 (low, then high)
        stz a:VCE_CTA_HI
        lda #17
        sta a:PSUM_LO
        stz a:PSUM_HI
        ldy #2                          ; two passes of 256 entries
        ldx #0
wr:     lda a:PSUM_LO
        sta a:VCE_CTW_LO                ; data low
        lda a:PSUM_HI
        sta a:VCE_CTW_HI                ; data high, then the address steps
        clc
        lda a:PSUM_LO
        adc #91
        sta a:PSUM_LO
        lda a:PSUM_HI
        adc #0
        and #$01                        ; the sum is kept to 9 bits
        sta a:PSUM_HI
        dex
        bne wr
        dey
        bne wr
; read entry 0 back (H6-5: CTA low, CTA high, then the data low byte first)
        stz a:VCE_CTA_LO
        stz a:VCE_CTA_HI
        lda a:VCE_CTW_LO
        CHECK_LIT 1                     ; check 1 MAN:H6-5 entry 0 low byte reads 17
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 2                     ; check 2 MAN:H6-5 entry 0 bit 8 reads 0
; read entry $100 back, the border of the line record
        stz a:VCE_CTA_LO
        lda #$01
        sta a:VCE_CTA_HI
        lda a:VCE_CTW_LO
        CHECK_LIT 3                     ; check 3 MAN:H6-5 entry $100 low byte reads $11
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 4                     ; check 4 MAN:H6-5 entry $100 bit 8 reads 1
; the tail delay: 2 + 255 * 6 + 4 = 1,536 CPU cycles = 18,432 master clocks at the low speed (about 13 lines), so that the run lasts for whole line records after the last palette write and the runner can judge the border one line away from it
        ldx #0
tail:   dex
        bne tail
        rts
