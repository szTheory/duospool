; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spr_limit.s - the sprite slot, overflow and collision fixture, first slice (plan 04.1-02, task 1; D-155, D-159, D-160, D-168, D-169, D-170 spr_limit, ER-10, H6-5, H7-4, H7-10, H7-13, H7-19 to H7-24). It is assembled by ca65 in the gate and never committed as a binary. spr.inc, vdma.inc and bg.inc are used unchanged.
; The guest sets the 256-wide window (HSR $0202, HDR $031F: HDS 2, HSW 2, HDE 3, HDW 31, so d = 32 and e = 10: 16 slots), the palette P(i) = (91 * i + 17) & 511 (checks 1 and 2, GUEST pal_bijection), a BAT of the transparent tile and one solid pattern cell (pixel 15, PC $100). The SATB is formed in VRAM by the writers BAND and ZFILL and copied by DMA before the sprites are
; switched on (CR bits 7, 6 and 1: the OR enable).
; Scene S16: 16 sprites of 16 dots side by side on display line 20, palette 0 to 15: no overflow, the status read after a clean frame is $00 (check 3). Scene S18: 18 sprites at an X step of 14 dots on the same line: the 17th and the 18th are dropped for lack of slots, OVERFLOW is set and the status read is $02 (check 4). The raster and the record fields are judged by the video
; case from fixtures/src/spr_limit.vexp.
; Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 26
DSV_CHECKS = 4

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"
        .include "vdc.inc"
        .include "vdma.inc"
        .include "bg.inc"
        .include "spr.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST:pal_bijection the 512 palette low bytes equal the additive running sum E(i + 1) = (E(i) + 91) & 511 from E(0) = 17
        LIT 2, $00                      ; GUEST:pal_bijection the 512 bit 8 values equal the same running sum
        LIT 3, $00                      ; MAN:H7-19 16 sprites on a line with 16 slots (e = 10): no OR flag, the status read after a clean frame is $00
        LIT 4, $02                      ; MAN:H7-19 18 sprites on a line with 16 slots: OR is set, bit 1 alone

LIM_N = $2450                           ; the writer parameters in work RAM (clear of SCRATCH, the verdict block and the stack): the entry count
LIM_I = $2451                           ; the entry being written
LIM_Y = $2452                           ; word 0 (Y), low then high byte
LIM_X = $2454                           ; word 1 (X), low then high byte, stepped by LIM_DX after each entry
LIM_PC = $2456                          ; word 2 (the pattern code)
LIM_ATTR = $2458                        ; word 3 (the attributes; the palette of entry i is the low nibble of the base plus i, modulo 16)
LIM_DX = $245A                          ; the X step in dots
LIM_PX = $245B                          ; the pixel value of the cell being written

HDR_A = $031F                           ; HDW 31, HDE 3: d = 32, e = 10
CR_OR = $00C2                           ; BB, SB and the OR enable (bit 1)

BAT_DEFAULT = $0040                     ; bank 0, code $040 (the transparent tile at the word address $0400)
TILES_AT = $0400

; ZFILL nwords: nwords (1 to 256) zero words by VWR (AR = VWR). Clobbers X
        .macro ZFILL nwords
        .local zl
        ldx #((nwords) & 255)
zl:     st1 #0
        st2 #0
        dex
        bne zl
        .endmacro

; BAND n, ey, ex0, edx, epc, eattr: n entries (at most 255) at the VRAM write address: word 0 = ey, word 1 = ex0 + edx * i, word 2 = epc, word 3 = eattr + (i & 15) (AR = VWR is set here). Clobbers A, X and Y
        .macro BAND n, ey, ex0, edx, epc, eattr
        lda #(n)
        sta a:LIM_N
        lda #<(ey)
        sta a:LIM_Y
        lda #>(ey)
        sta a:LIM_Y+1
        lda #<(ex0)
        sta a:LIM_X
        lda #>(ex0)
        sta a:LIM_X+1
        lda #<(epc)
        sta a:LIM_PC
        lda #>(epc)
        sta a:LIM_PC+1
        lda #<(eattr)
        sta a:LIM_ATTR
        lda #>(eattr)
        sta a:LIM_ATTR+1
        lda #(edx)
        sta a:LIM_DX
        jsr lim_band
        .endmacro

; CELL addr, px: the 64 words of one 16 x 16 cell of the solid pixel px at the VRAM word addr: plane k (16 words at +16 k) is $FFFF when bit k of px is set and $0000 otherwise (H7-24)
        .macro CELL addr, px
        VDC_REG VR_MAWR, (addr)
        VDC_SEL VR_VWR
        lda #(px)
        jsr lim_cell
        .endmacro

; PHASE_STATUS: the status read that clears the flags of the frames so far, one frame of delay (every line of the scene passes), then the status read, left in A for the CHECK_LIT that follows. Clobbers A, X and Y
        .macro PHASE_STATUS
        lda a:VDC_AR
        BG_FRAME
        lda a:VDC_AR
        .endmacro

; SATB_START: the VRAM write address at the start of the 256-word SATB source block
        .macro SATB_START
        VDC_REG VR_MAWR, SPR_SATB_SRC
        VDC_SEL VR_VWR
        .endmacro

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        BG_REGS_256                     ; the vertical setup, HSR $0202, HDR $031F, MWR 0, BXR = BYR = 0, CR = $0000 (burst frames)
        stz a:VCE_CR                    ; VCE CR $00: dot clock divider 4 (MAN H6-3)
        VCE_WRITE_P91                   ; the palette
        BG_FILL_BAT BAT_DEFAULT         ; the BAT: the transparent tile everywhere
        VDC_REG VR_MAWR, TILES_AT
        VDC_SEL VR_VWR
        ZFILL 16                        ; the tile of code $040: 16 zero words (pixel 0 everywhere)
        CELL $2000, 15                  ; PC $100: the 16-wide sprites
        VCE_READ_P91
        lda a:VCE_DIFF_LO
        CHECK_LIT 1                     ; check 1 GUEST:pal_bijection
        lda a:VCE_DIFF_HI
        CHECK_LIT 2                     ; check 2 GUEST:pal_bijection
; ---- S16: 16 sprites of one line ----
        SATB_START
        BAND 16, 84, 32, 16, $100, 0
        ZFILL 192
        SPR_SATB_GO
        BG_FRAME                        ; the copy runs at the vertical blanking of this frame
        VDC_REG BG_R_CR, CR_OR          ; the sprites on
        PHASE_STATUS
        CHECK_LIT 3                  ; check 3 MAN:H7-19: 16 sprites, 16 slots, no overflow
; ---- S18: 18 sprites ----
        SATB_START
        BAND 18, 84, 32, 14, $100, 0
        ZFILL 184
        SPR_SATB_GO
        BG_FRAME
        PHASE_STATUS
        CHECK_LIT 4                  ; check 4 MAN:H7-19: 18 sprites, OR set
        BG_FRAME
        rts

; lim_band: LIM_N entries written by VWR from the parameters (the macro BAND fills them). Clobbers A
lim_band:
        stz a:LIM_I
lb_next:
        lda a:LIM_Y
        sta a:VDC_LO
        lda a:LIM_Y+1
        sta a:VDC_HI
        lda a:LIM_X
        sta a:VDC_LO
        lda a:LIM_X+1
        sta a:VDC_HI
        lda a:LIM_PC
        sta a:VDC_LO
        lda a:LIM_PC+1
        sta a:VDC_HI
        lda a:LIM_I
        and #$0F
        clc
        adc a:LIM_ATTR
        sta a:VDC_LO
        lda a:LIM_ATTR+1
        sta a:VDC_HI
        clc
        lda a:LIM_X
        adc a:LIM_DX
        sta a:LIM_X
        lda a:LIM_X+1
        adc #0
        sta a:LIM_X+1
        inc a:LIM_I
        lda a:LIM_I
        cmp a:LIM_N
        bne lb_next
        rts

; lim_cell: A = the pixel value; the VRAM write address is set (AR = VWR): 4 planes of 16 words, each $FFFF or $0000. Clobbers A, X and Y
lim_cell:
        ldy #4
lc_plane:
        lsr a
        pha
        bcs lc_one
        lda #$00
        bra lc_put
lc_one: lda #$FF
lc_put: ldx #16
lc_word:
        sta a:VDC_LO
        sta a:VDC_HI
        dex
        bne lc_word
        pla
        dey
        bne lc_plane
        rts
