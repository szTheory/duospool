; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vce.inc - the VCE helpers of the authored fixtures (plan 04-19; D-169, D-170 vce_pal, plan-check ruling 7, F-40). Macros and equates only: including this file emits no byte, so no image that does not expand a macro changes. It is created by the plan that owns the VCE feature and is never edited by a later plan,
; so every pinned image keeps its bytes; later features get their own includes (vdc.inc, vdma.inc, spin.inc, bg.inc, spr.inc). Included after startup.inc, verdict.inc and check.inc. No HuC, PCEAS, emulator or cc65 library code.
; The VCE registers are at logical $0400 to $0407 while MPR0 = $FF maps the hardware page at logical $0000 (the fixture sets that); the register of an access is the offset & 7 (H6-2), so $0408 is a mirror of CR and $0444 a mirror of the CTW low byte.
; The procedure of H6-5: CTA low, CTA high, then the data low byte first, then the data high byte; the address is incremented after the high byte only. The scratch cells live in work RAM at logical $2410 to $241B (MPR1 = $F8), clear of SCRATCH, GEXP, the stack and the verdict block, and are initialised by the macros that use them.
        .setcpu "huc6280"

VCE_CR = $0400                  ; write only (a read gives the unused value)
VCE_CTA_LO = $0402              ; write only
VCE_CTA_HI = $0403              ; write only, bit 0 is the 9th address bit
VCE_CTW_LO = $0404              ; CTW write, CTR read: bits 7:0 of the entry
VCE_CTW_HI = $0405              ; CTW write, CTR read: bit 8 of the entry, then the address steps
VCE_UNUSED_1 = $0401            ; no register: reads give the unused value
VCE_UNUSED_6 = $0406
VCE_UNUSED_7 = $0407
VCE_MIR_CR = $0408              ; window mirror of CR ($0408 & 7 = 0)
VCE_MIR_CTA_LO = $040A          ; window mirror of CTA low ($040A & 7 = 2)
VCE_MIR_CTA_HI = $040B          ; window mirror of CTA high ($040B & 7 = 3)
VCE_MIR_CTR_LO = $040C          ; window mirror of CTW low and CTR low ($040C & 7 = 4)
VCE_MIR_CTR_HI = $040D          ; window mirror of CTW high and CTR high ($040D & 7 = 5)
VCE_MIR2_CTW_LO = $0444         ; a second mirror of CTW low ($0444 & 7 = 4)
VCE_MIR2_CTW_HI = $0445         ; a second mirror of CTW high ($0445 & 7 = 5)

PSUM_LO = $2410                 ; the running value of the additive reader: low byte and bit 8
PSUM_HI = $2411
VCE_I_LO = $2412                ; the entry index of the multiplicative writer, 0 to 511
VCE_I_HI = $2413
VCE_M_LO = $2414                ; the multiplicand, shifted left once per multiplier bit
VCE_M_HI = $2415
VCE_ACC_LO = $2416              ; the product being summed
VCE_ACC_HI = $2417
VCE_K = $2418                   ; the multiplier, shifted right once per step
VCE_DIFF_LO = $2419             ; the OR of every difference between a read low byte and the expected one
VCE_DIFF_HI = $241A             ; the OR of every difference between a read bit 8 and the expected one

; VCE_CTA v: the colour table address v (0 to 511), low byte then high byte
        .macro VCE_CTA v
        lda #<(v)
        sta a:VCE_CTA_LO
        lda #>(v)
        sta a:VCE_CTA_HI
        .endmacro

; VCE_LO v and VCE_HI v: one data byte at the current address (the address steps after the high byte only)
        .macro VCE_LO v
        lda #(v)
        sta a:VCE_CTW_LO
        .endmacro

        .macro VCE_HI v
        lda #(v)
        sta a:VCE_CTW_HI
        .endmacro

; VCE_WRITE_P91: all 512 entries by the procedure of H6-5, entry i = P(i) = (91 * i + 17) & 511, the product formed by shift and add (91 = 64 + 16 + 8 + 2 + 1, one add per set bit of the multiplier, taken lowest bit first) from the index; the address is written once and steps itself. Clobbers A and the cells above.
        .macro VCE_WRITE_P91
        .local wloop, mloop, mskip
        VCE_CTA 0
        stz a:VCE_I_LO
        stz a:VCE_I_HI
wloop:  lda #17                         ; the sum starts at the offset 17
        sta a:VCE_ACC_LO
        stz a:VCE_ACC_HI
        lda a:VCE_I_LO                  ; the multiplicand is the index
        sta a:VCE_M_LO
        lda a:VCE_I_HI
        sta a:VCE_M_HI
        lda #91
        sta a:VCE_K
mloop:  lsr a:VCE_K                     ; the lowest multiplier bit goes to the carry
        bcc mskip
        clc
        lda a:VCE_ACC_LO
        adc a:VCE_M_LO
        sta a:VCE_ACC_LO
        lda a:VCE_ACC_HI
        adc a:VCE_M_HI
        sta a:VCE_ACC_HI
mskip:  asl a:VCE_M_LO                  ; the multiplicand doubles for the next bit
        rol a:VCE_M_HI
        lda a:VCE_K
        bne mloop
        lda a:VCE_ACC_LO                ; the entry is the sum kept to 9 bits
        sta a:VCE_CTW_LO
        lda a:VCE_ACC_HI
        and #$01
        sta a:VCE_CTW_HI
        inc a:VCE_I_LO
        bne wloop
        inc a:VCE_I_HI
        lda a:VCE_I_HI
        cmp #$02
        bne wloop
        .endmacro

; VCE_READ_P91: all 512 entries read back by the procedure of H6-5 and compared with the additive running sum E(0) = 17, E(i + 1) = (E(i) + 91) & 511, which is a different computation from the multiplicative writer (GUEST: pure arithmetic over values the guest wrote). The low byte difference ends in VCE_DIFF_LO, the
; bit 8 difference in VCE_DIFF_HI: both are zero when every entry equals its sum. The reads of the high byte keep only bit 0. Clobbers A, X and Y.
        .macro VCE_READ_P91
        .local rloop
        VCE_CTA 0
        lda #17
        sta a:PSUM_LO
        stz a:PSUM_HI
        stz a:VCE_DIFF_LO
        stz a:VCE_DIFF_HI
        ldy #2                          ; two passes of 256 entries
        ldx #0
rloop:  lda a:VCE_CTW_LO                ; CTR low
        eor a:PSUM_LO
        ora a:VCE_DIFF_LO
        sta a:VCE_DIFF_LO
        lda a:VCE_CTW_HI                ; CTR high: bit 0 is bit 8 of the entry, the address then steps
        and #$01
        eor a:PSUM_HI
        ora a:VCE_DIFF_HI
        sta a:VCE_DIFF_HI
        clc
        lda a:PSUM_LO
        adc #91
        sta a:PSUM_LO
        lda a:PSUM_HI
        adc #0
        and #$01
        sta a:PSUM_HI
        dex
        bne rloop
        dey
        bne rloop
        .endmacro
