; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc.inc - the VDC helpers of the authored fixtures (plan 04-06; D-146, D-148, D-169, D-170 vdc_reg, plan-check ruling 7, F-40). Macros and equates only: including this file emits no byte, so no image that does not expand a macro changes. It is created by the plan that owns the VDC register feature and is never edited by a later plan,
; so every pinned image keeps its bytes; later features get their own includes (vdma.inc, spin.inc, bg.inc, spr.inc). vce.inc is not edited either. Included after startup.inc, verdict.inc and check.inc. No HuC, PCEAS, emulator or cc65 library code.
; The VDC registers are at logical $0000 (write: AR; read: status), $0001, $0002 (data LSB) and $0003 (data MSB) while MPR0 = $FF maps the hardware page at logical $0000 (the fixture sets that); the register of an access is the offset & 3 (A1 and A0, H8-24), so $0004 and $03FC are mirrors of AR.
; ST0, ST1 and ST2 write the physical window without the mapping registers (H8-8: A1,A0 = 00, 10, 11). The scratch cells live in work RAM at logical $2420 to $2426 (MPR1 = $F8), clear of SCRATCH, GEXP, the vce.inc cells, the stack and the verdict block, and are initialised by the macros that use them.
        .setcpu "huc6280"

VDC_AR = $0000                  ; write: AR (5 bits); read: the status
VDC_R1 = $0001                  ; write ignored, read $00
VDC_LO = $0002                  ; data LSB: write the low byte of the register AR names; read the low byte of the read buffer
VDC_HI = $0003                  ; data MSB: write the high byte (the side effects of VWR and MARR); read the high byte of the read buffer (VRR: triggers the next read)
VDC_MIR_AR = $0004              ; window mirrors of the four registers ($0004 & 3 = 0 and so on)
VDC_MIR_R1 = $0005
VDC_MIR_LO = $0006
VDC_MIR_HI = $0007
VDC_FAR_AR = $03FC              ; the last four bytes of the 1 KiB window
VDC_FAR_R1 = $03FD
VDC_FAR_LO = $03FE
VDC_FAR_HI = $03FF

VDC_W0 = $1234                  ; the first word of the TIA upload table
VDC_WSTEP = $00F7               ; word i of the table is (VDC_W0 + VDC_WSTEP * i) & $FFFF, formed by the assembler; the guest checks it with the additive running sum W(i + 1) = (W(i) + VDC_WSTEP) & $FFFF
VDC_TAI_BUF = $2800             ; work RAM at logical $2800 to $283F: the destination of the TAI readback (64 bytes)

VDC_PV_LO = $2420               ; the running sum of the checks: low and high byte
VDC_PV_HI = $2421
VDC_DIFF_LO = $2422             ; the OR of every difference between a read low byte and the expected one
VDC_DIFF_HI = $2423             ; the OR of every difference between a read high byte and the expected one

; VDC_SEL n: AR = n (ST0)
        .macro VDC_SEL n
        st0 #(n)
        .endmacro

; VDC_DATA v: the data word v to the register AR names, low byte then high byte (ST1, ST2)
        .macro VDC_DATA v
        st1 #<(v)
        st2 #>(v)
        .endmacro

; VDC_REG n, v: AR = n and the data word v. A MARR write (n = 1) starts a read operation with its high byte
        .macro VDC_REG n, v
        st0 #(n)
        st1 #<(v)
        st2 #>(v)
        .endmacro

; VDC_STA_AR n: AR = n through the logical address $0000 (the sta form of ST0; the data bytes are written by the caller with sta a:VDC_LO and sta a:VDC_HI)
        .macro VDC_STA_AR n
        lda #(n)
        sta a:VDC_AR
        .endmacro

; VDC_READ_SUM: 32 words read through VRR (the low byte, then the high byte, which triggers the next word) and compared with the additive running sum from VDC_W0 by VDC_WSTEP, which is a different computation from the multiplicative table of the uploader (GUEST: pure arithmetic over the values the guest wrote). The OR of
; the low byte differences ends in VDC_DIFF_LO, that of the high bytes in VDC_DIFF_HI: both are zero when every word equals its sum. The caller set AR = 2 and MARR to the first word. Clobbers A and X.
        .macro VDC_READ_SUM
        .local rloop
        lda #<VDC_W0
        sta a:VDC_PV_LO
        lda #>VDC_W0
        sta a:VDC_PV_HI
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        ldx #32
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

; VDC_CMP_RAM: the 64 bytes at VDC_TAI_BUF (32 words, low byte first) compared with the same additive running sum; the differences end in VDC_DIFF_LO and VDC_DIFF_HI. Clobbers A and X.
        .macro VDC_CMP_RAM
        .local cloop
        lda #<VDC_W0
        sta a:VDC_PV_LO
        lda #>VDC_W0
        sta a:VDC_PV_HI
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        ldx #0
cloop:  lda a:VDC_TAI_BUF,x
        eor a:VDC_PV_LO
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda a:VDC_TAI_BUF+1,x
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
        inx
        inx
        cpx #64
        bne cloop
        .endmacro
