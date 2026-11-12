; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdma.inc - the VDC DMA helpers of the authored fixtures (plan 04-10; D-150, D-151, D-169, D-170 vdc_dma and vdc_satb, plan-check ruling 7). Macros and equates only: including this file emits no byte, so no image that does not expand a macro changes. It is created by the plan that owns the feature
; (04-10) and is not edited by a later plan, so every pinned image keeps its bytes. Included after startup.inc, verdict.inc, check.inc and vdc.inc, which it uses (VDC_REG, VDC_SEL, VDC_DATA, the VDC_* port equates and the scratch cells VDC_PV_LO, VDC_PV_HI, VDC_DIFF_LO, VDC_DIFF_HI). No HuC, PCEAS, emulator or cc65 library code.
; The DMA registers are written through the ST triple like every VDC register: VDC_REG n, v with n the register number (hexadecimal): R0F DCR, R10 SOUR, R11 DESR, R12 LENR (the high byte write, the last of the three, starts the VRAM-to-VRAM transfer, H7-12(a)), R13 DVSSR (the high byte write arms the VRAM-to-SATB copy of the next vertical blanking, H7-12(b)).
; The table of the guest data is formed by the assembler as word i = (VDC_W0 + VDC_WSTEP * i) & $FFFF (a multiplication; vdc.inc holds the two constants) and checked by the guest with an additive running sum (GUEST). The delays are counted loops: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2), k from 1 to 255, 18
; master clocks per step at the high speed, which the fixture selects with CSH.
        .setcpu "huc6280"

VR_DCR = $0F                    ; the register numbers (R0F to R13)
VR_SOUR = $10
VR_DESR = $11
VR_LENR = $12
VR_DVSSR = $13
VR_CR = $05
VR_MAWR = $00
VR_MARR = $01
VR_VWR = $02

DCR_SID = $04                   ; DCR bit 2: the source decrements
DCR_DID = $08                   ; DCR bit 3: the destination decrements
DCR_DV = $02                    ; DCR bit 1: the VRAM-VRAM end interrupt (DV) is enabled
DCR_DS = $01                    ; DCR bit 0: the VRAM-SATB end interrupt (DS) is enabled
DCR_DSR = $10                   ; DCR bit 4: the VRAM-SATB copy repeats at every vertical blanking

; VDMA_DELAY k: a counted delay of 6 * k CPU cycles. Clobbers Y
        .macro VDMA_DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

; VDMA_GO sour, desr, m1: SOUR and DESR, then LENR = m1 (the number of words less one); the LENR high byte write is the last access, so the transfer starts there. AR ends at LENR
        .macro VDMA_GO sour, desr, m1
        VDC_REG VR_SOUR, (sour)
        VDC_REG VR_DESR, (desr)
        VDC_REG VR_LENR, (m1)
        .endmacro

; VDMA_PEEK addr: A = the low byte of the VRAM word at addr, read through MARR and VRR (the MARR high byte write loads the buffer at once, the low byte read returns it). Leaves AR = VRR
        .macro VDMA_PEEK addr
        VDC_REG VR_MARR, (addr)
        VDC_SEL VR_VWR
        lda a:VDC_LO
        .endmacro

; VDMA_POKE addr, v: the VRAM word v at addr by VWR (MAWR = addr, the high byte write stores it and steps MAWR). Leaves AR = VWR
        .macro VDMA_POKE addr, v
        VDC_REG VR_MAWR, (addr)
        VDC_SEL VR_VWR
        VDC_DATA (v)
        .endmacro

; VDMA_READ_SUM addr, n: n words (n from 1 to 255) from addr read through VRR and compared with the additive running sum from VDC_W0 by VDC_WSTEP, which is a different computation from the multiplicative table of the assembler (GUEST). The OR of the low byte differences ends in VDC_DIFF_LO and that of the high bytes in VDC_DIFF_HI:
; both are zero when every word equals its sum. Clobbers A and X
        .macro VDMA_READ_SUM addr, n
        .local rloop
        VDC_REG VR_MARR, (addr)
        VDC_SEL VR_VWR
        lda #<VDC_W0
        sta a:VDC_PV_LO
        lda #>VDC_W0
        sta a:VDC_PV_HI
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        ldx #(n)
rloop:  lda a:VDC_LO
        eor a:VDC_PV_LO
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda a:VDC_HI
        eor a:VDC_PV_HI
        ora a:VDC_DIFF_HI
        sta a:VDC_DIFF_HI
        clc
        lda a:VDC_PV_LO
        adc #<VDC_WSTEP
        sta a:VDC_PV_LO
        lda a:VDC_PV_HI
        adc #>VDC_WSTEP
        sta a:VDC_PV_HI
        dex
        bne rloop
        .endmacro
