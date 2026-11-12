; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vce_dot.s - the dot clock and horizontal geometry fixture (plan 04-09; D-139, D-140, D-141, D-161, D-169, D-170 vce_dot, ER-10, H6-3, H6-5, H7-10, H7-13). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF, as vce_pal does), so the VDC registers are at logical $0000 (AR), $0002 and $0003 (data) and the VCE registers at logical $0400 (CR) and $0402 to $0405. The helpers are the macros of fixtures/lib/vce.inc and vdc.inc (macros and equates only, never edited by this plan).
; Sequence: the D-139 vertical setup (VPR $0F02, VDW 239, VCR 4: the sum 264 lines is more than the frame length, so the cycle never repeats inside a frame), the horizontal registers HSR $0202 and HDR $031F (HDS 2, HDW 31), palette entry $100 and VCE CR $00; then the scenes below, each held for about one frame by a counted delay.
; The border: palette entry $100 is the colour of every blank dot and of the border (D-157, D-161). The guest forms its value by the additive running sum E(0) = 17, E(i + 1) = (E(i) + 91) & 511 stepped 256 times, which is P(256) = (91 * 256 + 17) & 511 = 23313 - 45 * 512 = 273, and writes it by the
; procedure of H6-5 (CTA low, CTA high, data low, data high), so the border row of the video case is arithmetic over values the guest wrote (GUEST pal_bijection, ER-10).
; The delay: HOLD n, m is LDY #n, then n passes of (LDX #0, 256 passes of DEX and BNE, DEY, BNE) of 1,542 CPU cycles each, then m passes of DEX and BNE of 6 cycles each: 1,542 * n + 6 * m + 2 cycles (tests/oracle/huc6280-opcodes.tsv: LDX and LDY imm 2, DEX and DEY 2, BNE 2 plus the taken-branch
; adder of 2). The fixture never executes CSH, so the CPU runs at the power-up low speed, 12 master clocks per cycle (constants.def cycle_mclk_low,
; community-reported, ASSUMED LED-CPU-0003): HOLD 19, 84 is 29,804 cycles = 357,648 master clocks, a little over one frame of 262 lines (357,630 clocks, ASSUMED LED-VID-0001). The runner judges no instant from these counts: every row is placed by the write stamps of the dev log, and a counted
; instant that lands within 250 clocks of a line boundary is refused by the runner (harness_failure), never judged.
; No CR change is made while a VRAM access is pending: the fixture touches no VRAM (H7-5). Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 19
DSV_CHECKS = 4
MIDDLY = 1                      ; the counted delay before the mid-frame writes: 6 * MIDDLY + 2 CPU cycles

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"
        .include "vdc.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $FF                      ; ASSUMED:LED-VID-0016 a read of CR gives the unused value
        LIT 2, $11                      ; MAN:H6-5 entry $100 low byte = P(256) & $FF = 273 & $FF = $11
        LIT 3, $01                      ; MAN:H6-5 entry $100 bit 8 = 1 (273 >= 256)
        LIT 4, $00                      ; MAN:H7-25 the high byte of the status word reads 00h

; HOLD n, m: the counted delay of the header, 1,542 * n + 6 * m + 2 CPU cycles. Clobbers X and Y.
        .macro HOLD n, m
        .local outer, inner, rem
        ldy #(n)
outer:  ldx #0
inner:  dex
        bne inner
        dey
        bne outer
        ldx #(m)
rem:    dex
        bne rem
        .endmacro

; SETUP hsr, hdr, cr: the window registers of a setup and then its closing CR write. Clobbers A.
        .macro SETUP hsr, hdr, cr
        VDC_REG 10, hsr
        VDC_REG 11, hdr
        lda #(cr)
        sta a:VCE_CR
        .endmacro

; FRAME: one frame of delay, HOLD 19, 84 (29,804 cycles)
        .macro FRAME
        HOLD 19, 84
        .endmacro

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        VDC_REG 12, $0F02               ; VPR: VSW = 2 (bits 4-0), VDS = 15 (bits 15-8): VDS + 2 = 17 lines before the display
        VDC_REG 13, 239                 ; VDW = 239: 240 display lines
        VDC_REG 14, 4                   ; VCR = 4: the sum (2+1) + (15+2) + (239+1) + 4 = 264 lines
        VDC_REG 10, $0202               ; HSR: HSW = 2, HDS = 2
        VDC_REG 11, $031F               ; HDR: HDW = 31, HDE = 3
; the border colour: E(256) by the additive running sum, then entry $100 by the procedure of H6-5
        lda #17
        sta a:PSUM_LO
        stz a:PSUM_HI
        ldx #0                          ; 256 passes (X wraps)
ploop:  clc
        lda a:PSUM_LO
        adc #91
        sta a:PSUM_LO
        lda a:PSUM_HI
        adc #0
        and #1                          ; the sum is kept to 9 bits: bit 8 only
        sta a:PSUM_HI
        dex
        bne ploop
        VCE_CTA $0100
        lda a:PSUM_LO
        sta a:VCE_CTW_LO
        lda a:PSUM_HI
        sta a:VCE_CTW_HI
; the scenes, one setup after another, each held for a frame by FRAME. Every setup is closed by a write of the VCE CR (even when the value is unchanged), so the window of that write is the span the runner judges (fixtures/src/vce_dot.vexp, STRUCT window and div_window): the records at least one line after it and two lines before the next. The CR writes of the log are, in order:
;   c0 $00  S0 HDS 2 HDW 31, division ratio 4 (MAN H6-3)                      c1 $01  S1 the same window, division ratio 3 (MAN H6-3)
;   c2 $02  S2 the same window, ASSUMED LED-VID-0002 division ratio 2         c3 $03  S3 the same window, ASSUMED LED-VID-0002 division ratio 2
;   c4 $00  S4 HDS 0, HDW 127: the window would end at (8 + 1024) * 4 clocks, so it is clipped to 333 dots (D-140)
;   c5 $00  S5 HDS 127: the window starts at (1024) * 4 clocks, beyond the line, so no dot fits and the lines are header-only
;   c6 $00  S6 the setup of S0 again, the base of the mid-frame change            c7 $01  the mid-frame change: CR $01 and HDR $0327 (HDW 39, 320 dots) in one line at a counted instant
; The HSR and HDR writes of the log (the data MSB write completes each): h0 HSR and h1 HDR of S0, h2 h3 of S4, h4 h5 of S5, h6 h7 of S6, h8 HDR of the mid-frame change.
; S0: HSR $0202 and HDR $031F are programmed above, CR $00
        stz a:VCE_CR                    ; c0: CR $00, division ratio 4
        FRAME
; S1: CR $01, division ratio 3 (MAN H6-3): the window ends at (24 + 256) * 3 = 840 clocks
        lda #$01
        sta a:VCE_CR                    ; c1
        FRAME
; S2 and S3: CR $02 and $03, division ratio 2 (ASSUMED LED-VID-0002): the window ends at (24 + 256) * 2 = 560 clocks
        lda #$02
        sta a:VCE_CR                    ; c2
        FRAME
        lda #$03
        sta a:VCE_CR                    ; c3
        FRAME
; S4: HDS 0 (HSR $0002), HDW 127 (HDR $007F), CR $00: clipped to floor(1365 / 4) - 8 = 333 dots
        SETUP $0002, $007F, $00         ; c4
        FRAME
; S5: HDS 127 (HSR $7F02), HDW 31, CR $00: the window starts beyond the line, no dot fits
        SETUP $7F02, $031F, $00         ; c5
        FRAME
; S6: the setup of S0 again
        SETUP $0202, $031F, $00         ; c6
        FRAME
; the mid-frame change: a counted delay places the writes inside one display line, then CR $01 (division ratio 3) and HDR $0327 (HDW = 39: 320 dots) follow within about 20 CPU cycles. The delay count was set so that both writes fall at least 250 master clocks from both boundaries of their line; the runner measures that from the stamps of the dev log and refuses the case otherwise.
        ldx #MIDDLY
mid:    dex
        bne mid
        lda #$01
        sta a:VCE_CR                    ; c7: CR $01
        VDC_REG 11, $0327               ; h8: HDR, HDW = 39
        FRAME
        FRAME
; the readbacks of what the guest programmed
        lda a:VCE_CR
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-VID-0016 CR reads $FF
        VCE_CTA $0100
        lda a:VCE_CTW_LO
        CHECK_LIT 2                     ; check 2 MAN:H6-5 entry $100 low byte reads $11
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 3                     ; check 3 MAN:H6-5 entry $100 bit 8 reads 1
        lda a:VDC_R1
        CHECK_LIT 4                     ; check 4 MAN:H7-25 the high byte of the status word reads $00
        rts
