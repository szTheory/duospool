; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; bg_enab.s - the enable fixture (plan 04-12; D-157, D-169, D-170 bg_enab, ER-10, H6-5, H7-6, H7-14, H7-15, H7-18). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed, writes the vertical setup (VPR $0F02, VDW 239, VCR 4), the 256-wide horizontal setup (HSR $0202, HDR $031F), MWR 0 (a 32 x 32 BAT), BXR = BYR = 0 and CR = $0000, then the palette P(i) = (91 * i + 17) & 511 (VCE_WRITE_P91 of vce.inc), a 32 x 32 BAT (the word $0040 everywhere except
; row 0 columns 0 to 3: $3040, $0041, $F042, $7043, the layout of H7-14) and four solid tiles from word $0400 (code $040 to $043: pixel 15, 12, 3 and 0, H7-15), reads the tiles and the palette back (checks 1 to 4, GUEST) and then writes the VDC control register CR (R05) one phase at a time, two frames each:
; BB (bit 7) alone, BB and SB (bit 6), SB alone, neither (a burst frame: BB = SB = 0 at the first display line, H7-6), and BB again. The CR writes are numbered from 0 in the order the guest makes them: 0 is the setup (BB = SB = 0), 1 BB, 2 BB and SB, 3 SB, 4 neither, 5 BB, 6 SB (the mid-frame toggle). Then a counted delay places the write 6 inside display line
; about 3 of a frame (the BAT row 0, whose columns hold four different tiles), where BB is cleared and SB set, so the frame is not a burst frame and the dots that follow take the backdrop index (bg_off_index, ASSUMED LED-VID-0010) from the next line (H7-6: valid from the next horizontal display period), and one more frame.
; The tiles are solid, so no bit order, byte order or flip changes them (ER-12). The frames the video scene judges are measured from the guest's own counted sequence (BG_FRAME) and calibrated from one run, never a literal source: a frame that is not the intended one fails its rows. The guest polls no status, so the device log stays small. Nothing here is read
; from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 24
DSV_CHECKS = 4

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"
        .include "vdc.inc"
        .include "bg.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:tile-readback the 128 tile low bytes read back through VRR equal the table the upload took them from: the OR of the differences is zero
        LIT 2, $00                      ; GUEST:tile-readback the high bytes equal the table
        LIT 3, $00                      ; GUEST:pal_bijection the 512 palette low bytes equal the additive running sum E(i + 1) = (E(i) + 91) & 511 from E(0) = 17
        LIT 4, $00                      ; GUEST:pal_bijection the 512 bit 8 values equal the same running sum

; the tiles, 4 of 16 words (little endian words: low byte plane 0 or 2, high byte plane 1 or 3): eight words of CG0 (planes 0 and 1) then eight words of CG1 (planes 2 and 3)
        .pushseg
        .segment "RODATA"
be_tiles:
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
        .popseg

BAT_DEFAULT = $0040                     ; bank 0, code $040
TILES_AT = $0400                        ; the word address of code $040 (code * 16)
TILE_BYTES = 128                        ; 4 tiles of 16 words

; BE_PHASE cr: CR = cr and two frames of delay
        .macro BE_PHASE cr
        VDC_REG BG_R_CR, cr
        BG_FRAME
        BG_FRAME
        .endmacro

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        BG_REGS_256
        stz a:VCE_CR                    ; VCE CR $00: dot clock divider 4 (MAN H6-3)
        VCE_WRITE_P91                   ; the palette
; the BAT: the default word everywhere, then the 4 entries of row 0 (the words of H7-14: bank << 12 | code)
        BG_FILL_BAT BAT_DEFAULT
        VDC_REG BG_R_MAWR, 0
        VDC_SEL BG_R_VWR
        VDC_DATA $3040                  ; row 0 column 0: bank 3, code $040
        VDC_DATA $0041                  ; column 1: bank 0, code $041
        VDC_DATA $F042                  ; column 2: bank 15, code $042
        VDC_DATA $7043                  ; column 3: bank 7, code $043
; the tiles: one block upload, the destination alternates $0002 and $0003 (low byte, high byte)
        VDC_REG BG_R_MAWR, TILES_AT
        VDC_SEL BG_R_VWR
        tia be_tiles, VDC_LO, TILE_BYTES
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
        eor a:be_tiles,x
        ora a:VDC_DIFF_LO
        sta a:VDC_DIFF_LO
        lda a:VDC_HI
        eor a:be_tiles+1,x
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
; the SATB (plan 04.1-01): no sprite is drawn here, but SB is set in two of the phases, so a seeded power-on SATB and VRAM would draw random sprites (D-167: a guest must initialise what the renderer reads). The 256 words at VRAM word $1000 are zeroed and the high byte write of DVSSR (R13) copies them to the SATB at the next vertical blanking (H7-12), before the first phase
; that sets SB; a zero SATB covers no display line (Y = 0 is above the display).
        VDC_REG BG_R_MAWR, $1000
        VDC_SEL BG_R_VWR
        ldx #0
satz:   st1 #0
        st2 #0
        dex
        bne satz                        ; 256 words of zero
        VDC_REG $13, $1000              ; DVSSR: the copy runs at the next vertical blanking
; the enable matrix, two frames a phase: BB (write 1), BB and SB (2), SB (3), neither (4, a burst frame), BB (5)
        BE_PHASE $0080
        BE_PHASE $00C0
        BE_PHASE $0040
        BE_PHASE $0000
; BB again (write 5), one frame, then 34 * 1542 + 107 * 6 + 2 = 53,072 cycles (about 116.6 lines of 455 cycles; plan 04.1-01 took the 4,128 cycles of the SATB block above from the 57,200 cycles this hold had, so that the write below sits where it sat before the block) so that the next write falls about 3 display lines into a frame (a calibration of the counted sequence, never
; a literal of any row: a write placed wrongly is refused by the judge), then BB clear and SB set (write 6), two frames
        VDC_REG BG_R_CR, $0080
        BG_FRAME
        BG_HOLD 34, 107
        VDC_REG BG_R_CR, $0040
        BG_FRAME
        BG_FRAME
        rts
