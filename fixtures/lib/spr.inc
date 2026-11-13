; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spr.inc - the sprite helpers of the authored fixtures (plan 04.1-01; D-159, D-160, D-169, D-170 spr_basic, plan-check ruling 7). Macros and equates only: including this file emits no byte, so no image that does not expand a macro changes. It is created by the plan that owns the feature (04.1-01) and is not edited by a later plan, so every pinned image keeps its bytes.
; Included after startup.inc, verdict.inc, check.inc, vce.inc, vdc.inc, vdma.inc and bg.inc, which it uses (VDC_REG, VDC_SEL, VDC_LO, VDC_HI, the VR_ register numbers of vdma.inc and the scratch cells VDC_DIFF_LO and VDC_DIFF_HI). No HuC, PCEAS, emulator or cc65 library code.
; The sprite attribute table (SATB, 256 words, 4 per sprite, H7-20) is not CPU-writable: a guest forms the 256 words in VRAM at SPR_SATB_SRC and a write of the high byte of DVSSR (R13) copies them at the next vertical blanking (H7-12, D-150). The words of an entry are (H7-20): word 0 Y bits 9-0, word 1 X bits 9-0, word 2 the pattern code bits 10-0, word 3 the attributes:
; Y-flip bit 15, CGY bits 13-12, X-flip bit 11, CGX bit 8, SPBG bit 7, the palette bank bits 3-0.
; The pattern code PC holds the VRAM word address of the pattern shifted right by 5 (H7-21: PC bits 10-1 are address bits 15-6), so a pattern at the word address A has PC = A >> 5 and a pattern area must start on a 64-word boundary (H7-24); a 32-wide pattern pair, a 32-high stack and a 64-high stack of 16-wide sprites need the 128, 256 and 512-word alignments the fixtures keep
; (H7-22, H7-23). A 16 x 16 pattern is four planes SG0 to SG3 of 16 words each, plane k at +16k, row r at +r (H7-24). The leftmost pixel of a row is bit 15 (ASSUMED LED-VID-0011).
        .setcpu "huc6280"

SPR_CR_SB = $0040               ; CR: SB (bit 6) set: the sprites are drawn (H7-6)
SPR_CR_BBSB = $00C0             ; CR: BB (bit 7) and SB set
SPR_SATB_SRC = $1000            ; the VRAM word address of the 256-word SATB source block
SPR_PAT_AT = $2000              ; the VRAM word address of the first pattern area

SPR_YFLIP = $8000               ; attribute bits (H7-20)
SPR_XFLIP = $0800
SPR_CGY1 = $1000                ; CGY 01: 32 lines high (H7-23)
SPR_CGY2 = $2000                ; CGY 10: "not used" on the page
SPR_CGY3 = $3000                ; CGY 11: 64 lines high
SPR_CGX = $0100                 ; CGX: 32 dots wide (H7-22)
SPR_SPBG = $0080                ; SPBG: the sprite is in front of the background (H7-21)

; SPR_ENTRY ey, ex, epc, eattr: the four words of one SATB entry (Y, X, pattern code, attributes; the parameter names avoid the register names x and y)
        .macro SPR_ENTRY ey, ex, epc, eattr
        .word (ey), (ex), (epc), (eattr)
        .endmacro

; SPR_CELL px: the 64 words of one 16 x 16 pattern cell whose every dot is the pixel value px (0 to 15): plane k is $FFFF in all 16 rows when bit k of px is set and $0000 otherwise. A solid cell reads the same under every bit order, byte order and flip (ER-12)
        .macro SPR_CELL px
        .repeat 4, spr_k
        .repeat 16
        .word (((px) >> spr_k) & 1) * $FFFF
        .endrepeat
        .endrepeat
        .endmacro

; SPR_UP addr, table, nbytes: nbytes (even, at most 65534) of table to the VRAM words from addr by VWR and a block transfer (the destination alternates the low and the high byte). Leaves AR = VWR
        .macro SPR_UP addr, table, nbytes
        VDC_REG VR_MAWR, (addr)
        VDC_SEL VR_VWR
        tia table, VDC_LO, nbytes
        .endmacro

; SPR_CMP addr, table, nbytes: nbytes (even, at most 254) of the VRAM from addr read back through VRR and compared with table. The OR of the low byte differences accumulates in VDC_DIFF_LO and that of the high bytes in VDC_DIFF_HI (the caller cleared them: both stay zero when every word equals its table word). Clobbers A and X. Leaves AR = VWR
        .macro SPR_CMP addr, table, nbytes
        .local cl
        VDC_REG VR_MARR, (addr)
        VDC_SEL VR_VWR
        ldx #0
cl:     lda a:VDC_LO
        eor a:table,x
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda a:VDC_HI
        eor a:table+1,x
        ora a:VDC_DIFF_HI
        sta a:VDC_DIFF_HI
        inx
        inx
        cpx #(nbytes)
        bne cl
        .endmacro

; SPR_SATB_GO: the write of the high byte of DVSSR that arms the copy of the 256 words at SPR_SATB_SRC to the SATB at the next vertical blanking (H7-12)
        .macro SPR_SATB_GO
        VDC_REG VR_DVSSR, SPR_SATB_SRC
        .endmacro
