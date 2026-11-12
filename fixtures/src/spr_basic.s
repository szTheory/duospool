; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spr_basic.s - the sprite fixture (plan 04.1-01; D-155, D-159, D-160 priority part, D-169, D-170 spr_basic, ER-10, ER-12, H6-5, H7-6, H7-12, H7-13, H7-14, H7-20, H7-21, H7-24). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed, writes the vertical setup (VPR $0F02, VDW 239, VCR 4), the 256-wide horizontal setup (HSR $0202, HDR $031F: active_x 24, active_w 256 dots), MWR 0 (a 32 x 32 BAT), BXR = BYR = 0 and CR = $0000 (BB and SB clear: burst frames), then the palette P(i) = (91 * i + 17) & 511 for all 512 entries
; by the procedure of H6-5 (VCE_WRITE_P91 of vce.inc), read back against an additive running sum (VCE_READ_P91), so a value row of the video case is arithmetic over values the guest wrote (GUEST pal_bijection, ER-10).
; The background is a 32 x 32 BAT of the word $0040 (bank 0, code $040, a tile of pixel 0 everywhere: transparent, index 0 whatever the bank, H7-18) with one entry of code $041 (a solid tile of pixel 15, index 15) at row 2, column 7. The sprite pattern is one solid 16 x 16 cell of pixel 15 at VRAM word $2000 (PC $100). The SATB source block of 256 words is at VRAM word $1000
; and holds sprite 0 at Y = 84, X = 72, palette 2 (so it is drawn from display line 84 - 64 = 20 on and from dot 72 - 32 = 40 on, H7-13), and for the other 63 entries the words ($03C0 + $0400 * w) & $FFFF (Y = X = 960 in the low 10 bits, which no display line reaches).
; Sequence: the setup above, the palette, the BAT, the tiles, the pattern and the SATB source uploaded by block transfers and read back (checks 1 to 6, GUEST: every VRAM word the renderer reads was written first, D-167), a write of the high byte of DVSSR (the copy runs at the next vertical blanking, H7-12), one frame of delay (the copy has run), then CR = $00C0 (BB and SB set:
; the sprites are drawn from the next frame) and six frame delays (BG_FRAME of bg.inc, counted loops that read no register). The frame numbers the vexp judges are measured from this sequence, not computed: a frame that is not the intended one fails its rows, it never passes them.
; Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 25
DSV_CHECKS = 6

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"
        .include "vdc.inc"
        .include "vdma.inc"
        .include "bg.inc"
        .include "spr.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:pattern-readback the low bytes of the pattern words read back through VRR equal the ROM table the block upload took them from: the OR of the differences is zero
        LIT 2, $00                      ; GUEST:pattern-readback the high bytes equal the table
        LIT 3, $00                      ; GUEST:satb-readback the low bytes of the 256 SATB source words read back through VRR equal the ROM table
        LIT 4, $00                      ; GUEST:satb-readback the high bytes equal the table
        LIT 5, $00                      ; GUEST:pal_bijection the 512 palette low bytes equal the additive running sum E(i + 1) = (E(i) + 91) & 511 from E(0) = 17
        LIT 6, $00                      ; GUEST:pal_bijection the 512 bit 8 values equal the same running sum

        .pushseg
        .segment "RODATA"
; the two background tiles, 16 words each (code $040 and $041, H7-15: eight words of CG0 then eight of CG1)
spr_tiles:
        .repeat 16                      ; code $040: pixel 0 everywhere (no plane set): index 0 whatever the bank
        .word $0000
        .endrepeat
        .repeat 16                      ; code $041: solid, pixel 15 (all four planes set)
        .word $FFFF
        .endrepeat
; the sprite pattern cell at VRAM word $2000: solid, pixel 15 (SG0 to SG3 all set in every row)
spr_c15:
        SPR_CELL 15
; the SATB source block: sprite 0, then 63 entries of the filler word ($03C0 + $0400 * w) & $FFFF (w is the word number; Y = X = 960 in the low 10 bits)
spr_satb:
        SPR_ENTRY 84, 72, $100, $0002
        .repeat 252, i
        .word ($03C0 + $0400 * (i + 4)) & $FFFF
        .endrepeat
        .popseg

BAT_DEFAULT = $0040                     ; bank 0, code $040
TILES_AT = $0400                        ; the word address of code $040 (code * 16)
TILE_BYTES = 64                         ; 2 tiles of 16 words
PAT_C15 = SPR_PAT_AT                    ; the solid pixel 15 cell, PC = $2000 >> 5 = $100

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        BG_REGS_256
        stz a:VCE_CR                    ; VCE CR $00: dot clock divider 4 (MAN H6-3)
        VCE_WRITE_P91                   ; the palette
        BG_FILL_BAT BAT_DEFAULT         ; the BAT: the transparent tile everywhere
        VDMA_POKE 71, $0041             ; row 2 column 7 (word 2 * 32 + 7): bank 0, code $041 (the opaque tile)
        SPR_UP TILES_AT, spr_tiles, TILE_BYTES
        SPR_UP PAT_C15, spr_c15, 128
        SPR_UP SPR_SATB_SRC, spr_satb, 512
; the readbacks (the frames so far are burst frames)
        VCE_READ_P91
        lda a:VCE_DIFF_LO
        CHECK_LIT 5                     ; check 5 GUEST:pal_bijection
        lda a:VCE_DIFF_HI
        CHECK_LIT 6                     ; check 6 GUEST:pal_bijection
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        SPR_CMP PAT_C15, spr_c15, 128
        lda a:VDC_DIFF_LO
        CHECK_LIT 1                     ; check 1 GUEST:pattern-readback
        lda a:VDC_DIFF_HI
        CHECK_LIT 2                     ; check 2 GUEST:pattern-readback
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        SPR_CMP SPR_SATB_SRC, spr_satb, 128
        SPR_CMP SPR_SATB_SRC + 64, spr_satb + 128, 128
        SPR_CMP SPR_SATB_SRC + 128, spr_satb + 256, 128
        SPR_CMP SPR_SATB_SRC + 192, spr_satb + 384, 128
        lda a:VDC_DIFF_LO
        CHECK_LIT 3                     ; check 3 GUEST:satb-readback
        lda a:VDC_DIFF_HI
        CHECK_LIT 4                     ; check 4 GUEST:satb-readback
; the SATB copy at the next vertical blanking, one frame, then the sprites on
        SPR_SATB_GO
        BG_FRAME
        VDC_REG BG_R_CR, SPR_CR_BBSB
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        rts
