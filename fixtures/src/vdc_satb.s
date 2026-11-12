; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_satb.s - the VDC VRAM-to-SATB transfer fixture (plan 04-10; D-150, D-169, D-170, D-175, ER-10, F-26, F-27, H7-4, H7-11, H7-12, H7-20). It is assembled by ca65 in the gate and never committed as a binary. The helpers are the macros of fixtures/lib/vdc.inc and fixtures/lib/vdma.inc.
; The SATB is not CPU-readable (H7-20: it is filled only by the transfer), so this guest judges only what it can see, the DS flag of the status register, and the runner kind vdc_satb_data (case vdc_satb_data) reads the 512 SATB bytes of the instance at the end of the run against the source pattern (D-175 (c)). The guest uploads a 256-word pattern P1 to VRAM word $1000
; with a TIA block transfer, sets a vertical setup
; (VPR = $0F02, VDW = 239, VCR = 4, HSR = $0200, HDR = $001F: 264 lines, more than the frame length) with CR = $0008 (the VBlank flag enabled), and synchronises on a VBlank flag by polling the interrupt request register (the I flag stays set; the level follows the status flags, ASSUMED LED-IRQ-0009). The waits after a VBlank are counted delays of 9180 master clocks.
; The steps: 1 a DVSSR low byte write alone arms nothing: after the next VBlank and the wait the status is $20 (VD only, no DS) (check 1, H7-12: the high byte is the trigger). 2 A DVSSR high byte write (DVSSR = $1000) waits for the following VBlank: mid-frame the status is $00 (check 2, H7-12). 3 After that VBlank and the wait the status is $28, VD and DS (check 3,
; ASSUMED LED-VID-0013: DS follows the copy,
; DCR bit 0 set), and a second read gives $00 (check 4, H7-4). 4 With nothing re-armed the next VBlank copies nothing: the status is $20 (check 5, H7-12). 5 With DCR bit 0 clear a re-armed copy raises no DS: $20 (check 6, H7-4 gating, ASSUMED LED-VID-0013 for the copy). 6 With DCR bit 4 (DSR) and bit 0 set the copy repeats at the next VBlank without a new write: $28 (check 7).
; The polls from a VBlank until DS appears are counted (check 8, an observe row, ASSUMED LED-VID-0013: VL1 claims no duration). 7 The source block is rewritten with a second pattern P2 and the repeat copies it at the next VBlank: $28 (check 9); DCR is then cleared, so no later VBlank copies, and the SATB holds P2 at the end (the runner kind judges it).
; The patterns are formed by the assembler: P1 word i = ($1234 + $F7 * i) & $FFFF and P2 word i = ($A5A5 + $F1 * i) & $FFFF (multiplications, restated by the runner kind). Every status byte the guest judges is a flag byte, never a word of VRAM or SATB, so the check bytes are equal under every power-on policy (D-167). Nothing here is read from a DuoSpool run. No HuC,
; PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 21
DSV_CHECKS = 9

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"
        .include "vdma.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $20                ; ASSUMED:LED-VID-0013 (H7-12 states it; DS shows a copy within the wait only under the modelled DS delay) a DVSSR low byte write alone starts nothing: after a VBlank and the wait the status is $20 (VD alone, bit 5), no DS
        LIT 2, $00                ; ASSUMED:LED-VID-0013 (H7-12 states it; same DS delay) a DVSSR high byte write is carried out at the beginning of the FOLLOWING vertical blanking: mid-frame the status is $00
        LIT 3, $28                ; ASSUMED:LED-VID-0013 after that VBlank and the wait the status is $28: VD (bit 5) and DS (bit 3), DCR bit 0 set
        LIT 4, $00                ; MAN:H7-4 a status read clears bits 0 to 5: the second read is $00
        LIT 5, $20                ; ASSUMED:LED-VID-0013 (H7-12 states it; same DS delay) nothing re-armed: the next VBlank copies nothing, the status is $20 (VD alone)
        LIT 6, $20                ; ASSUMED:LED-VID-0013 with DCR bit 0 clear a re-armed copy raises no DS: the status is $20 (VD alone)
        LIT 7, $28                ; ASSUMED:LED-VID-0013 with DCR bit 4 (DSR) and bit 0 set the copy repeats at the next VBlank without a new write: VD and DS, $28
        LIT 8, $00                ; ASSUMED:LED-VID-0013 the polls from a VBlank until DS appears (an observe row: recorded, never judged by a literal; VL1 claims no duration)
        LIT 9, $28                ; ASSUMED:LED-VID-0013 after the source is rewritten the repeat copy at the next VBlank raises DS again: $28

SRCA = $1000                    ; the source block: 256 words
P2_W0 = $A5A5                   ; the second pattern: word i = (P2_W0 + P2_STEP * i) & $FFFF
P2_STEP = $00F1

; WAITDS: the wait after a VBlank flag, long enough for the DS time of the copy (a counted delay of 9180 master clocks, never a position)
        .macro WAITDS
        VDMA_DELAY 255
        VDMA_DELAY 255
        .endmacro

; the two patterns, formed by the assembler (multiplications)
        .pushseg
        .segment "RODATA"
satb_p1:
        .repeat 256, i
        .word (VDC_W0 + VDC_WSTEP * i) & $FFFF
        .endrepeat
satb_p2:
        .repeat 256, i
        .word (P2_W0 + P2_STEP * i) & $FFFF
        .endrepeat
        .popseg

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        VDC_REG VR_CR, $0000
        VDC_REG VR_DCR, $0000
        VDC_REG VR_MAWR, SRCA
        VDC_SEL VR_VWR
        tia satb_p1, VDC_LO, 512        ; P1: 256 words at SRCA to SRCA + 255
        VDC_REG 12, $0F02               ; VPR: VSW = 2 (bits 4-0), VDS = 15 (bits 15-8)
        VDC_REG 13, 239                 ; VDW = 239
        VDC_REG 14, 4                   ; VCR = 4: 17 + 240 + 4 + 3 = 264 lines, more than the frame length
        VDC_REG 10, $0200               ; HSR: HSW = 0, HDS = 2
        VDC_REG 11, $001F               ; HDR: HDW = 31
        VDC_REG VR_CR, $0008            ; CR bit 3: the VBlank flag is enabled
        VDMA_WAIT_VD
        lda a:VDC_AR                    ; acknowledge: from here every wait is for a new VBlank
; ---- 1: a DVSSR low byte write alone arms nothing ----
        VDC_REG VR_DCR, DCR_DS
        VDC_SEL VR_DVSSR
        st1 #$34                        ; the low byte alone (DVSSR = $0034): nothing is armed
        VDMA_WAIT_VD
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-VID-0013 no DS, VD alone
; ---- 2: the high byte write waits for the following VBlank ----
        VDC_REG VR_DVSSR, SRCA          ; the high byte write arms the copy of the next VBlank
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 2                     ; check 2 ASSUMED:LED-VID-0013 mid-frame the status is clear
; ---- 3: the copy at that VBlank, DS after it, cleared by the read ----
        VDMA_WAIT_VD
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 3                     ; check 3 ASSUMED:LED-VID-0013 VD and DS
        lda a:VDC_AR
        CHECK_LIT 4                     ; check 4 MAN:H7-4 cleared by the read
; ---- 4: nothing re-armed, nothing copied ----
        VDMA_WAIT_VD
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 5                     ; check 5 ASSUMED:LED-VID-0013 VD alone
; ---- 5: DCR bit 0 clear: the copy raises no DS ----
        VDC_REG VR_DCR, $0000
        VDC_REG VR_DVSSR, SRCA          ; re-armed
        VDMA_WAIT_VD
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 6                     ; check 6 ASSUMED:LED-VID-0013 VD alone
; ---- 6: DSR repeats the copy at every VBlank ----
        VDC_REG VR_DCR, DCR_DSR | DCR_DS
        VDMA_WAIT_VD
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 7                     ; check 7 ASSUMED:LED-VID-0013 VD and DS without a new write
        VDMA_WAIT_VD
        lda a:VDC_AR                    ; acknowledge VD (the status read also clears everything else)
        ldx #0
pollds: lda a:VDC_AR
        and #$08                        ; bit 3: DS
        bne pollde
        inx
        cpx #250                        ; bounded: a machine that raises no DS ends the loop at 250
        bne pollds
pollde: txa
        CHECK_OBSERVE 8                 ; check 8 ASSUMED:LED-VID-0013 the number of polls, recorded
; ---- 7: the source is rewritten, the next repeat copies P2 ----
        VDC_REG VR_MAWR, SRCA
        VDC_SEL VR_VWR
        tia satb_p2, VDC_LO, 512        ; P2: 256 words at SRCA to SRCA + 255
        VDMA_WAIT_VD
        WAITDS
        lda a:VDC_AR
        CHECK_LIT 9                     ; check 9 ASSUMED:LED-VID-0013 VD and DS again
        VDC_REG VR_DCR, $0000           ; no further copy: the SATB keeps P2
        rts
