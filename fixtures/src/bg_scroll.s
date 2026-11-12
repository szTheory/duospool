; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; bg_scroll.s - the scroll fixture (plan 04-12; D-156, D-169, D-170 bg_scroll, ER-10, ER-12, H7-6, H7-7, H7-9, H7-14, H7-15, H7-18). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed, writes the vertical setup (VPR $0F02, VDW 239, VCR 4), the 256-wide horizontal setup (HSR $0202, HDR $031F), MWR 0 (a 32 x 32 BAT), BXR = BYR = 0 and CR = $0000 (burst frames), then the palette P(i) = (91 * i + 17) & 511 (VCE_WRITE_P91 of vce.inc), a 32 x 32
; BAT and four solid tiles, reads the tiles and the palette back (checks 1 to 4, GUEST) and sets CR BB (the background is drawn from the next frame, whose display line 0 latches the burst state as !(BB || SB), D-157).
; The tiles are 16 words each from word $1000 (code $100 to $103, H7-15: the BAT of the 64 x 64 phase fills the words below $1000), all solid so that no bit order, byte order or flip changes them (ER-12): code $100 pixel 15, $101 pixel 12, $102 pixel 3, $103 pixel 0 (index 0 whatever the bank, H7-18).
; The 32 x 32 BAT (H7-14: word 0 is the top-left character, +1 to the right, +$20 per row; bank in bits 15-12, code in bits 11-0): the word of BAT row r and column c is the code $100 + (c & 3) in the bank 1 for r < 8 and the bank 2 for r >= 8. A line of the display therefore shows, from the left edge and in tiles of 8 dots, the four indices of its bank in the order
; pixel 15, 12, 3, 0 repeated: bank 1 gives 31, 28, 19, 0 and bank 2 gives 47, 44, 35, 0. The period is 4 tiles = 32 dots, so a horizontal scroll of 16 dots (two tiles) gives the same row whichever way the scroll runs and whether or not the 256-dot virtual line wraps: the row of the latch rows is direction-independent (ER-12).
; Latch phase (the frames the scene video_bg_scroll judges): CR BB, two frames, a counted delay (BS_ALIGN) that puts the next write inside a display line, then four register writes at counted distances: BXR = 16 in line n1, BXR = 0 six lines later, BYR = 63 in line n2 thirty lines later, BYR = 0 eight lines later, then one frame. The writes are placed by the counted
; delays only (the guest polls no status, so the device log stays small); the line each write falls in is found by the judge from the stamp of the write in the dev log against the origins of the records (never from this file), and a write nearer than 250 master clocks to a line origin is refused by the judge (Pitfall 2). Register write numbers of the guest,
; counted from 0: BXR 0 is the setup, 1 is
; 16, 2 is 0, 3 is 500; BYR 0 is the setup, 1 is 63, 2 is 0, 3 is 300; the VDC CR 0 is the setup and 1 is BB.
; Wrap phase (the scene video_bg_scroll_asym): the 4,096 words of a 64 x 64 BAT (the word of row r and column c is the bank c >> 2 with the code $100 + ((c + r) & 3), so every column pair of 4 has its own bank and the type shifts by one per row), then MWR = $0050 (SCREEN 101: 64 x 64, latched at the next VBlank entry, H7-9), BXR = 500 and BYR = 300, then four frames.
; Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 23
DSV_CHECKS = 4

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"
        .include "vdc.inc"
        .include "bg.inc"

BS_HI = $2430                           ; scratch cells of work RAM: the high byte of the BAT word being written, and a copy of the row counter
BS_T = $2431

; BS_ALIGN_N and BS_ALIGN_M: the counted delay (BG_HOLD) that puts the first latch write inside a display line, at least 250 master clocks from both line origins. The value is a calibration of the counted sequence of this file, never a literal of any row: a write placed wrongly is refused by the judge.
BS_ALIGN_N = 2
BS_ALIGN_M = 100

; BS_LINES n: a counted delay of n lines of 455 CPU cycles (1,365 master clocks at the high speed, ASSUMED LED-VID-0001, never judged from this count) between the start of one VDC_REG write (15 cycles) and the start of the next one of the same shape, so the second write falls at about the same place of its line as the first. 1542 * N + 6 * M + 2 + 15 = 455 * n.
        .macro BS_LINES n
        BG_HOLD ((n * 455 - 17) / 1542), (((n * 455 - 17) - 1542 * ((n * 455 - 17) / 1542)) / 6)
        .endmacro

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:tile-readback the 128 tile low bytes read back through VRR equal the table the upload took them from: the OR of the differences is zero
        LIT 2, $00                      ; GUEST:tile-readback the high bytes equal the table
        LIT 3, $00                      ; GUEST:pal_bijection the 512 palette low bytes equal the additive running sum E(i + 1) = (E(i) + 91) & 511 from E(0) = 17
        LIT 4, $00                      ; GUEST:pal_bijection the 512 bit 8 values equal the same running sum

; the tiles, 4 of 16 words (little endian words: low byte plane 0 or 2, high byte plane 1 or 3): eight words of CG0 (planes 0 and 1) then eight words of CG1 (planes 2 and 3)
        .pushseg
        .segment "RODATA"
bs_tiles:
; code $100: solid, pixel 15 (all four planes set)
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
; code $101: solid, pixel 12 (planes 2 and 3 set)
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
; code $102: solid, pixel 3 (planes 0 and 1 set)
        .word $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF, $FFFF
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
; code $103: pixel 0 everywhere (no plane set)
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
        .word $0000, $0000, $0000, $0000, $0000, $0000, $0000, $0000
        .popseg

TILES_AT = $1000                        ; the word address of code $100 (code * 16)
TILE_BYTES = 128                        ; 4 tiles of 16 words

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        BG_REGS_256
        stz a:VCE_CR                    ; VCE CR $00: dot clock divider 4 (MAN H6-3)
        VCE_WRITE_P91                   ; the palette
; the 32 x 32 BAT: row r (Y), column c (X); the high byte is bank 1 (r < 8) or bank 2, in the high nibble, with the code high nibble 1; the low byte is the code low byte c & 3
        VDC_REG BG_R_MAWR, 0
        VDC_SEL BG_R_VWR
        ldy #0
rows32: lda #$11
        cpy #8
        bcc hi32
        lda #$21
hi32:   sta a:BS_HI
        ldx #0
cols32: txa
        and #3
        sta a:VDC_LO
        lda a:BS_HI
        sta a:VDC_HI
        inx
        cpx #32
        bne cols32
        iny
        cpy #32
        bne rows32
; the tiles: one block upload, the destination alternates $0002 and $0003 (low byte, high byte)
        VDC_REG BG_R_MAWR, TILES_AT
        VDC_SEL BG_R_VWR
        tia bs_tiles, VDC_LO, TILE_BYTES
; the readbacks (the frames so far are burst frames)
        VCE_READ_P91
        lda a:VCE_DIFF_LO
        CHECK_LIT 3                     ; check 3 GUEST:pal_bijection
        lda a:VCE_DIFF_HI
        CHECK_LIT 4                     ; check 4 GUEST:pal_bijection
        stz a:VDC_DIFF_LO
        stz a:VDC_DIFF_HI
        VDC_REG BG_R_MARR, TILES_AT
        VDC_SEL BG_R_VWR
        ldx #0
tloop:  lda a:VDC_LO
        eor a:bs_tiles,x
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda a:VDC_HI
        eor a:bs_tiles+1,x
        ora a:VDC_DIFF_HI
        sta a:VDC_DIFF_HI
        inx
        inx
        cpx #TILE_BYTES
        bne tloop
        lda a:VDC_DIFF_LO
        CHECK_LIT 1                     ; check 1 GUEST:tile-readback
        lda a:VDC_DIFF_HI
        CHECK_LIT 2                     ; check 2 GUEST:tile-readback
; the latch phase: the background on (BB), a frame and a part of a frame, the delay that puts the first write inside a display line, then the four counted writes
        VDC_REG BG_R_CR, BG_CR_BG
        BG_FRAME
        BG_HOLD 29, 130                 ; 45,500 cycles = 100 lines: places the first write about 15 display lines into a frame (a calibration of the counted sequence)
        BG_HOLD BS_ALIGN_N, BS_ALIGN_M
        VDC_REG BG_R_BXR, 16            ; BXR write 1: two tiles to the left (or right, the row is the same), from the next line (H7-7)
        BS_LINES 6
        VDC_REG BG_R_BXR, 0             ; BXR write 2: back to 0 from the next line
        BS_LINES 30
        VDC_REG BG_R_BYR, 63            ; BYR write 1: the next line shows virtual row 63 + 1 = 64 (H7-7)
        BS_LINES 8
        VDC_REG BG_R_BYR, 0             ; BYR write 2: the next line shows virtual row 0 + 1
        BG_FRAME
; the wrap phase: the 64 x 64 BAT (the bank is column >> 2, the code low byte is (c + r) & 3, the code high nibble 1), then SCREEN 101, BXR 500 and BYR 300, four frames
        VDC_REG BG_R_MAWR, 0
        VDC_SEL BG_R_VWR
        ldy #0
rows64: ldx #0
cols64: txa
        sty a:BS_T
        clc
        adc a:BS_T
        and #3
        sta a:VDC_LO
        txa
        and #$3C
        asl a
        asl a
        ora #$01
        sta a:VDC_HI
        inx
        cpx #64
        bne cols64
        iny
        cpy #64
        bne rows64
        VDC_REG BG_R_MWR, $0050
        VDC_REG BG_R_BXR, 500
        VDC_REG BG_R_BYR, 300
        BG_FRAME
        BG_FRAME
        BG_FRAME
        BG_FRAME
        rts
