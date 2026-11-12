; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; bg_basic.s - the background fixture (plan 04-11; D-156, D-157, D-158, D-169, D-170 bg_basic, ER-10, ER-12, H6-5, H7-6, H7-9, H7-14, H7-15, H7-18). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed, writes the vertical setup (VPR $0F02, VDW 239, VCR 4), the 256-wide horizontal setup (HSR $0202, HDR $031F: active_x 24, active_w 256 dots), MWR 0 (a 32 x 32 BAT, VM 0, SM 0), BXR = BYR = 0 and CR = $0000 (BB and SB clear: burst frames), then the palette, the BAT and the tiles.
; The palette is P(i) = (91 * i + 17) & 511 for all 512 entries by the procedure of H6-5 (VCE_WRITE_P91 of vce.inc, a multiplication by shift and add) and is read back against an additive running sum (VCE_READ_P91), so a value row of the video case is arithmetic over values the guest wrote (GUEST pal_bijection, ER-10).
; The BAT (a 32 x 32 map from VRAM word 0, H7-14) holds the word $0040 everywhere (bank 0, code $040: a solid tile of pixel 15, index 15) except 8 entries (row 0 and row 1, columns 0 to 3). The tiles are 16 words each from word $0400 (code $040, H7-15:
; words 0 to 7 hold CH0 and CH1, words 8 to 15 hold CH2 and CH3), hand packed in the table bg_tiles below; the low byte of a word is read as plane 0 or 2 and the high byte as plane 1 or 3, the leftmost dot of a row is bit 7 (ASSUMED LED-VID-0010).
; Two groups of tiles, ER-12. The order-insensitive group (BAT row 0): solid tiles of pixel 15, 12 and 3 and a tile of pixel 0 in banks 3, 0, 15 and 7: the indices (bank << 4) | px are 63, 12 and 243, and 0 for the tile of pixel 0 whatever its bank
; (H7-18: VD3 to VD0 all 0 forces VD7 to VD4 to 0); no bit order, byte order or flip changes a solid tile. The asymmetric group (BAT row 1): a solid reference tile, a one-pixel diagonal (bit order) and
; solid tiles of pixel 1 and pixel 2 (the plane byte order).
; Sequence: the setup above, then the palette, the BAT and tile uploads and their readbacks (checks 1 to 6; every VRAM and palette word the readbacks and the renderer read was written first, D-167), then CR = $0080 (BB set: the background is drawn from the next frame, whose display line 0 latches the
; burst state as !(BB || SB), D-157) and six frame delays (phase A, the frames the scenes of
; fixtures/src/bg_basic.vexp judge), then MWR = $0001 (VM = 1) and four more frame delays (phase B, the unmodelled-mode frames, D-158). The frame numbers the vexp judges are measured from this sequence, not computed: a frame that is not the intended one fails its rows, it never passes them.
; Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 22
DSV_CHECKS = 6

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"
        .include "vdc.inc"
        .include "bg.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:bat-fields the low bytes of the 8 BAT words read back equal the code low bytes: the OR of the differences is zero
        LIT 2, $00                      ; GUEST:bat-fields the high bytes equal (bank << 4) | (code >> 8) formed in the guest by shifts
        LIT 3, $00                      ; GUEST:tile-readback the 224 tile bytes at code $040 read back through VRR (low bytes) equal the table the upload took them from
        LIT 4, $00                      ; GUEST:tile-readback the high bytes equal the table
        LIT 5, $00                      ; GUEST:pal_bijection the 512 palette low bytes equal the additive running sum E(i + 1) = (E(i) + 91) & 511 from E(0) = 17
        LIT 6, $00                      ; GUEST:pal_bijection the 512 bit 8 values equal the same running sum

; the tiles, 7 of 16 words (little endian words: low byte plane 0 or 2, high byte plane 1 or 3), one tile per code from $040. Each tile: eight words of CG0 (planes 0 and 1) then eight words of CG1 (planes 2 and 3), one word per tile row.
        .pushseg
        .segment "RODATA"
bg_tiles:
; code $040: solid, pixel 15 (all four planes set)          ########  every row
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
; code $041: solid, pixel 12 (planes 2 and 3 set, planes 0 and 1 clear)
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
; code $042: solid, pixel 3 (planes 0 and 1 set, planes 2 and 3 clear)
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
; code $043: pixel 0 everywhere (no plane set): index 0 whatever the bank
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
; code $044: a one-pixel diagonal of pixel 1 (plane 0 only, the low byte of CG0 words 0 to 7); the set bit of row r is bit 7 - r:
;   row 0  1.......    $0080
;   row 1  .1......    $0040
;   row 2  ..1.....    $0020
;   row 3  ...1....    $0010
;   row 4  ....1...    $0008
;   row 5  .....1..    $0004
;   row 6  ......1.    $0002
;   row 7  .......1    $0001
        .word $0080, $0040, $0020, $0010, $0008, $0004, $0002, $0001
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
; code $045: solid, pixel 1 as the low byte of CG0 (plane 0 under the assumed byte order)
        .word $00FF, $00FF, $00FF, $00FF, $00FF, $00FF, $00FF, $00FF
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
; code $046: solid, pixel 2 as the high byte of CG0 (plane 1 under the assumed byte order)
        .word $FF00, $FF00, $FF00, $FF00, $FF00, $FF00, $FF00, $FF00
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
        .popseg

BAT_DEFAULT = $0040                     ; bank 0, code $040
TILES_AT = $0400                        ; the word address of code $040 (code * 16)
TILE_BYTES = 224                        ; 7 tiles of 16 words

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        BG_REGS_256
        stz a:VCE_CR                    ; VCE CR $00: dot clock divider 4 (MAN H6-3)
        VCE_WRITE_P91                   ; the palette
; the BAT: the default word everywhere, then the 8 entries of the two rows (the words of H7-14: bank << 12 | code)
        BG_FILL_BAT BAT_DEFAULT
        VDC_REG BG_R_MAWR, 0
        VDC_SEL BG_R_VWR
        VDC_DATA $3040                  ; row 0 column 0: bank 3, code $040
        VDC_DATA $0041                  ; column 1: bank 0, code $041
        VDC_DATA $F042                  ; column 2: bank 15, code $042
        VDC_DATA $7043                  ; column 3: bank 7, code $043
        VDC_REG BG_R_MAWR, 32
        VDC_SEL BG_R_VWR
        VDC_DATA $3040                  ; row 1 column 0: bank 3, code $040
        VDC_DATA $2044                  ; column 1: bank 2, code $044
        VDC_DATA $4045                  ; column 2: bank 4, code $045
        VDC_DATA $4046                  ; column 3: bank 4, code $046
; the tiles: one block upload, the destination alternates $0002 and $0003 (low byte, high byte)
        VDC_REG BG_R_MAWR, TILES_AT
        VDC_SEL BG_R_VWR
        tia bg_tiles, VDC_LO, TILE_BYTES
; the readbacks (the frames so far are burst frames)
        VCE_READ_P91
        lda a:VCE_DIFF_LO
        CHECK_LIT 5                     ; check 5 GUEST:pal_bijection
        lda a:VCE_DIFF_HI
        CHECK_LIT 6                     ; check 6 GUEST:pal_bijection
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        VDC_REG BG_R_MARR, 0
        VDC_SEL BG_R_VWR
        BG_CMP_WORD 3, $0040
        BG_CMP_WORD 0, $0041
        BG_CMP_WORD 15, $0042
        BG_CMP_WORD 7, $0043
        VDC_REG BG_R_MARR, 32
        VDC_SEL BG_R_VWR
        BG_CMP_WORD 3, $0040
        BG_CMP_WORD 2, $0044
        BG_CMP_WORD 4, $0045
        BG_CMP_WORD 4, $0046
        lda a:VDC_DIFF_LO
        CHECK_LIT 1                     ; check 1 GUEST:bat-fields
        lda a:VDC_DIFF_HI
        CHECK_LIT 2                     ; check 2 GUEST:bat-fields
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        VDC_REG BG_R_MARR, TILES_AT
        VDC_SEL BG_R_VWR
        ldx #0
tloop:  lda a:VDC_LO
        eor a:bg_tiles,x
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda a:VDC_HI
        eor a:bg_tiles+1,x
        ora a:VDC_DIFF_HI
        sta a:VDC_DIFF_HI
        inx
        inx
        cpx #TILE_BYTES
        bne tloop
        lda a:VDC_DIFF_LO
        CHECK_LIT 3                     ; check 3 GUEST:tile-readback
        lda a:VDC_DIFF_HI
        CHECK_LIT 4                     ; check 4 GUEST:tile-readback
; phase A: the background on, six frames
        VDC_REG BG_R_CR, BG_CR_BG
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
; phase B: VM = 1 in MWR (a mode this machine does not render, D-158), then four more frames: the latch at the next VBlank entry brings it into the records
        VDC_REG BG_R_MWR, $0001
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        rts
