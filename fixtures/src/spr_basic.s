; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spr_basic.s - the sprite fixture (plan 04.1-01; D-155, D-159, D-160 priority part, D-168, D-169, D-170 spr_basic, ER-10, ER-12, H6-5, H7-6, H7-12, H7-13, H7-14, H7-20 to H7-24). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed, writes the vertical setup (VPR $0F02, VDW 239, VCR 4), the 256-wide horizontal setup (HSR $0202, HDR $031F: active_x 24, active_w 256 dots), MWR 0 (a 32 x 32 BAT), BXR = BYR = 0 and CR = $0000 (BB and SB clear: burst frames), then the palette P(i) = (91 * i + 17) & 511 for all 512 entries
; by the procedure of H6-5 (VCE_WRITE_P91 of vce.inc), read back against an additive running sum (VCE_READ_P91), so a value row of the video cases is arithmetic over values the guest wrote (GUEST pal_bijection, ER-10).
; The background is a 32 x 32 BAT of the word $0040 (bank 0, code $040: a tile of pixel 0 everywhere, transparent, index 0 whatever the bank, H7-18) with the word $0041 (code $041: a solid tile of pixel 15, index 15) at row 2 column 7 and in the whole of column 22 (dots 176 to 183 of every display line, a vertical stripe of opaque background).
; The patterns (16 x 16 cells of four planes, plane k at +16k words, H7-24) are uploaded by block transfers to the VRAM areas below and read back (check 1 and 2, GUEST). The first group (solid cells: every dot of a cell has one pixel value, so no bit order, byte order or flip can change it, ER-12): word $2000 pixel 15 (PC $100); $2080 a 32-wide pair, the left cell pixel 12 and
; the right cell pixel 3 (PC $104, CGX); $2100 and $2180 two cells of pixel 10 (a 32-high stack, PC $108, CGY 1);
; $2200, $2280, $2300 and $2380 four cells of pixel 5 (a 64-high stack, PC $110, CGY 3); $2400 and $2440 a pair, the left cell pixel 15 and the right cell pixel 0 (PC $120). The second group (asymmetric, ASSUMED LED-VID-0011): $2480 one cell, the asymmetric pattern spr_asym (PC $124); $2600, $2680, $2700 and $2780 four cells of pixel 1, 2, 4 and 8 (a stack drawn with the CGY
; field 2, PC $130).
; The SATB source block of 256 words is at VRAM word $1000: 19 sprite entries (spr_satb below, listed in the vexp comments with the lines and dots each covers) and, for the other 45 entries, the words ($03C0 + $0400 * w) & $FFFF (w is the word number: Y = X = 960 in the low 10 bits, which no display line reaches). A write of the high byte of DVSSR arms the copy at the next
; vertical blanking (H7-12), one frame of delay lets it run, then CR = $00C0 (BB and SB set)
; draws the sprites from the next frame and six frame delays (BG_FRAME of bg.inc, counted loops that read no register) hold the scene. The frame numbers the vexp judges are measured from this sequence, not computed: a frame that is not the intended one fails its rows, it never passes them. At most 4 sprites cover any display line, so no slot limit binds (plan 04.1-02 owns the limit).
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
        LIT 1, $00                      ; GUEST:pattern-readback the low bytes of every pattern cell read back through VRR equal the ROM table the block upload took them from: the OR of the differences is zero
        LIT 2, $00                      ; GUEST:pattern-readback the high bytes equal the tables
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
; the solid pattern cells (SPR_CELL px: plane k is $FFFF in every row when bit k of px is set, else $0000)
spr_c15:
        SPR_CELL 15
spr_c12:
        SPR_CELL 12
spr_c3:
        SPR_CELL 3
spr_c10:
        SPR_CELL 10
spr_c5:
        SPR_CELL 5
spr_c0:
        SPR_CELL 0
spr_c1:
        SPR_CELL 1
spr_c2:
        SPR_CELL 2
spr_c4:
        SPR_CELL 4
spr_c8:
        SPR_CELL 8
; the asymmetric cell (ASSUMED LED-VID-0011: the leftmost pixel of a row is bit 15). Plane SG0 holds a diagonal, bit 15 - r set in row r; plane SG1 holds the left half of every row, $FF00; SG2 and SG3 are clear. So row r has the pixel value 3 at column r when r < 8 (the diagonal on the bar), 2 at the other columns 0 to 7, and 1 at column r when r >= 8; every other column is pixel 0.
;   row 0   3 2 2 2 2 2 2 2 . . . . . . . .       row 9   2 2 2 2 2 2 2 2 . 1 . . . . . .
;   row 3   2 2 2 3 2 2 2 2 . . . . . . . .       row 15  2 2 2 2 2 2 2 2 . . . . . . . 1
spr_asym:
        .repeat 16, r
        .word $8000 >> r
        .endrepeat
        .repeat 16
        .word $FF00
        .endrepeat
        .repeat 32
        .word $0000
        .endrepeat
; the SATB source block: 19 entries (Y, X, PC, attributes), then the filler words. Y = display line + 64 and X = dot + 32 (H7-13); PC = pattern word address >> 5 (H7-21); the attributes are the sums of the SPR_ bits of spr.inc and the palette bank
spr_satb:
        SPR_ENTRY 84, 72, $100, $0002                   ; 0  S0 16 x 16 pixel 15, palette 2: display lines 20 to 35, dots 40 to 55
        SPR_ENTRY 84, 132, $104, SPR_CGX | 5            ; 1  S1 32 x 16 pair pixel 12 then 3, palette 5: lines 20 to 35, dots 100 to 131
        SPR_ENTRY 124, 172, $108, SPR_CGY1 | 7          ; 2  S2 16 x 32 pixel 10, palette 7: lines 60 to 91, dots 140 to 155
        SPR_ENTRY 100, 232, $110, SPR_CGY3 | 9          ; 3  S3 16 x 64 pixel 5, palette 9: lines 36 to 99, dots 200 to 215
        SPR_ENTRY 164, 204, $100, $0004                 ; 4  S4 16 x 16 pixel 15, palette 4, SPBG 0: lines 100 to 115, dots 172 to 187
        SPR_ENTRY 204, 196, $100, SPR_SPBG | 6          ; 5  S5 16 x 16 pixel 15, palette 6, SPBG 1: lines 140 to 155, dots 164 to 179
        SPR_ENTRY 194, 72, $100, 11                     ; 6  S6 16 x 16 pixel 15, palette 11: lines 130 to 145, dots 40 to 55
        SPR_ENTRY 194, 80, $100, 12                     ; 7  S7 16 x 16 pixel 15, palette 12: lines 130 to 145, dots 48 to 63 (index 7 is behind index 6)
        SPR_ENTRY 244, 192, $120, SPR_CGX | SPR_SPBG | 8 ; 8 S8 32 x 16 pair pixel 15 then 0, palette 8, SPBG 1: lines 180 to 195, dots 160 to 191
        SPR_ENTRY 264, 40, $124, 1                      ; 9  A0 asymmetric cell, no flip, palette 1: lines 200 to 215, dots 8 to 23
        SPR_ENTRY 264, 72, $124, SPR_XFLIP | 1          ; 10 A1 the same, X flip: dots 40 to 55
        SPR_ENTRY 264, 104, $124, SPR_YFLIP | 1         ; 11 A2 the same, Y flip: dots 72 to 87
        SPR_ENTRY 264, 136, $124, SPR_XFLIP | SPR_YFLIP | 1 ; 12 A3 the same, X and Y flip: dots 104 to 119
        SPR_ENTRY 164, 220, $130, SPR_CGY2 | 3          ; 13 A4 16 x 16 cells stacked, the CGY field 2, palette 3: lines 100 to 163, dots 188 to 203
        SPR_ENTRY 124, 24, $124, SPR_XFLIP | 1          ; 14 A5 asymmetric cell, X flip, at X = 24: dots -8 to 7 (left edge clip), lines 60 to 75
        SPR_ENTRY 124, 280, $124, 1                     ; 15 A6 asymmetric cell, no flip, at X = 280: dots 248 to 263 (right edge clip), lines 60 to 75
        SPR_ENTRY 56, 72, $124, 1                       ; 16 A7 asymmetric cell at Y = 56: lines -8 to 7 (top clip), dots 40 to 55
        SPR_ENTRY 214, 132, $104, SPR_CGX | SPR_XFLIP | 5 ; 17 A8 the pair of S1 with X flip: lines 150 to 165, dots 100 to 131
        SPR_ENTRY 230, 232, $130, SPR_YFLIP | SPR_CGY1 | 10 ; 18 A9 16 x 32 (CGY 1), the top cell pixel 1 and the bottom cell pixel 2 of the A4 stack, Y flip, palette 10: lines 166 to 197, dots 200 to 215 (review fix M-02)
        .repeat 180, i
        .word ($03C0 + $0400 * (i + 76)) & $FFFF
        .endrepeat
        .popseg

BAT_DEFAULT = $0040                     ; bank 0, code $040
TILES_AT = $0400                        ; the word address of code $040 (code * 16)
TILE_BYTES = 64                         ; 2 tiles of 16 words

; SPR_PAT addr, table: one 16 x 16 cell (128 bytes) to the VRAM word addr and read back at once against the table; the OR of the differences accumulates in VDC_DIFF_LO and VDC_DIFF_HI
        .macro SPR_PAT addr, table
        SPR_UP addr, table, 128
        SPR_CMP addr, table, 128
        .endmacro

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        BG_REGS_256
        stz a:VCE_CR                    ; VCE CR $00: dot clock divider 4 (MAN H6-3)
        VCE_WRITE_P91                   ; the palette
        BG_FILL_BAT BAT_DEFAULT         ; the BAT: the transparent tile everywhere
        VDMA_POKE 71, $0041             ; row 2 column 7 (word 2 * 32 + 7): bank 0, code $041 (the opaque tile)
        .repeat 30, r
        VDMA_POKE r * 32 + 22, $0041    ; column 22 of every row (the stripe of opaque background, dots 176 to 183)
        .endrepeat
        SPR_UP TILES_AT, spr_tiles, TILE_BYTES
        SPR_UP SPR_SATB_SRC, spr_satb, 512
; the readbacks (the frames so far are burst frames)
        VCE_READ_P91
        lda a:VCE_DIFF_LO
        CHECK_LIT 5                     ; check 5 GUEST:pal_bijection
        lda a:VCE_DIFF_HI
        CHECK_LIT 6                     ; check 6 GUEST:pal_bijection
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
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
; the pattern cells, each uploaded and read back
        SPR_PAT SPR_PAT_AT, spr_c15     ; $2000 S0, S4, S5, S6, S7 (PC $100)
        SPR_PAT $2080, spr_c12          ; the left cell of the pair of S1 and A8 (PC $104)
        SPR_PAT $20C0, spr_c3           ; the right cell
        SPR_PAT $2100, spr_c10          ; the 32-high stack of S2 (PC $108): top cell
        SPR_PAT $2180, spr_c10          ; bottom cell
        SPR_PAT $2200, spr_c5           ; the 64-high stack of S3 (PC $110): four cells
        SPR_PAT $2280, spr_c5
        SPR_PAT $2300, spr_c5
        SPR_PAT $2380, spr_c5
        SPR_PAT $2400, spr_c15          ; the pair of S8 (PC $120): the left cell pixel 15
        SPR_PAT $2440, spr_c0           ; the right cell, pixel 0
        SPR_PAT $2480, spr_asym         ; the asymmetric cell (PC $124)
        SPR_PAT $2600, spr_c1           ; the stack of A4 (PC $130): pixel 1, 2, 4 and 8 from the top; A9 draws the first two as a 32-high stack
        SPR_PAT $2680, spr_c2
        SPR_PAT $2700, spr_c4
        SPR_PAT $2780, spr_c8
        lda a:VDC_DIFF_LO
        CHECK_LIT 1                     ; check 1 GUEST:pattern-readback
        lda a:VDC_DIFF_HI
        CHECK_LIT 2                     ; check 2 GUEST:pattern-readback
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
