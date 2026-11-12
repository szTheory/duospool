; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; bg.inc - the background helpers of the authored fixtures (plan 04-11; D-156, D-157, D-169, D-170 bg_basic, plan-check ruling 7). Macros and equates only: including this file emits no byte, so no image that does not expand a macro changes. It is
; created by the plan that owns the feature (04-11) and is not edited by a later plan, so every pinned image keeps its bytes; later features get their own includes
; (spr.inc). Included after startup.inc, verdict.inc, check.inc, vce.inc and vdc.inc, which it uses (VDC_REG, VDC_SEL, VDC_DATA, the VDC port equates and the scratch cells VDC_DIFF_LO and VDC_DIFF_HI). No HuC, PCEAS, emulator or cc65 library code.
; The registers are written through the ST triple like every VDC register (VDC_REG n, v): the numbers below are the register numbers of the manual (hexadecimal). The BAT starts at VRAM word 0 (H7-14) and a character is 16 words at code * 16 (H7-15), so a tile code below 64 would overlap a 32 x 32 BAT: the fixtures use the codes from $040.
; The frame delays are counted loops (BG_FRAME): a guest that polled the status register would fill the device log with reads, and the delay count is not judged, only the frame numbers of the records are.
        .setcpu "huc6280"

BG_R_MAWR = $00                 ; the register numbers (R00 to R0E)
BG_R_MARR = $01
BG_R_VWR = $02
BG_R_CR = $05
BG_R_BXR = $07
BG_R_BYR = $08
BG_R_MWR = $09
BG_R_HSR = $0A
BG_R_HDR = $0B
BG_R_VPR = $0C
BG_R_VDW = $0D
BG_R_VCR = $0E

BG_CR_BURST = $0000             ; CR: BB and SB clear: the frames are burst frames (D-157)
BG_CR_BG = $0080                ; CR: BB (bit 7) set: the background is drawn (H7-6)

; BG_REGS_256: the vertical setup of D-139 (VPR $0F02, VDW 239, VCR 4: the sum 264 lines is more than the frame length, so the cycle restarts at each frame) and the 256-wide horizontal setup HSR $0202, HDR $031F (HDS 2, HDW 31: active_x 24, active_w 256 dots, H7-10, H7-13), MWR 0 (a 32 x 32 BAT, VM 0, SM 0), BXR and BYR 0 and CR BG_CR_BURST. Leaves AR = BYR
        .macro BG_REGS_256
        VDC_REG BG_R_VPR, $0F02
        VDC_REG BG_R_VDW, 239
        VDC_REG BG_R_VCR, 4
        VDC_REG BG_R_HSR, $0202
        VDC_REG BG_R_HDR, $031F
        VDC_REG BG_R_MWR, $0000
        VDC_REG BG_R_BXR, $0000
        VDC_REG BG_R_CR, BG_CR_BURST
        VDC_REG BG_R_BYR, $0000
        .endmacro

; BG_FILL_BAT w: the 1,024 words of a 32 x 32 BAT, all the word w, by VWR from word 0 (MAWR steps after each high byte write). Clobbers X and Y. Leaves AR = VWR and MAWR = $0400
        .macro BG_FILL_BAT w
        .local fl
        VDC_REG BG_R_MAWR, 0
        VDC_SEL BG_R_VWR
        ldx #0
        ldy #4                  ; 4 passes of 256 words
fl:     st1 #<(w)
        st2 #>(w)
        dex
        bne fl
        dey
        bne fl
        .endmacro

; BG_HOLD n, m: a counted delay of 1,542 * n + 6 * m + 2 CPU cycles (LDY and LDX immediate 2, DEX and DEY 2, BNE 2 plus the taken-branch adder of 2, from tests/oracle/huc6280-opcodes.tsv): n passes of 256 inner passes of DEX and BNE (1,542 cycles each) and m passes of 6 cycles. Clobbers X and Y. No hardware register is read, so the delay adds nothing to the device log.
        .macro BG_HOLD n, m
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

; BG_FRAME: one frame of delay at the high speed, BG_HOLD 77, 79 = 119,210 CPU cycles = 357,630 master clocks at 3 per cycle, which is 262 lines of 1,365 clocks (the frame length and the line length are ASSUMED LED-VID-0001; nothing is judged from the count, every row is placed by the frame numbers of the records and a frame that is not the intended one fails its rows)
        .macro BG_FRAME
        BG_HOLD 77, 79
        .endmacro

; BG_CMP_WORD bank, code: the next word read through VRR (the low byte, then the high byte, which triggers the next read) against the BAT word of bank and code (H7-14: bank in bits 15-12, code in bits 11-0). The high byte of the expectation is formed at run time, the bank shifted left four times and
; ORed with the high nibble of the code: a different computation from the assembler word the writer
; stored. The OR of the differences accumulates in VDC_DIFF_LO and VDC_DIFF_HI (the caller cleared them). Clobbers A
        .macro BG_CMP_WORD bank, code
        lda a:VDC_LO
        eor #<(code)
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda #(bank)
        asl a
        asl a
        asl a
        asl a
        ora #>(code)
        eor a:VDC_HI
        ora a:VDC_DIFF_HI
        sta a:VDC_DIFF_HI
        .endmacro
