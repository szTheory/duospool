; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spr_limit.s - the sprite slot, overflow and collision fixture (plan 04.1-02; D-155, D-159, D-160, D-168, D-169, D-170 spr_limit, ER-10, ER-12, H6-5, H7-4, H7-10, H7-13, H7-19 to H7-24). It is assembled by ca65 in the gate and never committed as a binary. spr.inc, vdma.inc and bg.inc are used unchanged; the plan-specific code is in this file.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed, writes the vertical setup (VPR $0F02, VDW 239, VCR 4), the 256-wide horizontal setup (HSR $0202, HDR $031F: HDS 2, HDW 31, HSW 2, HDE 3, so d = 32 and e = 10 and the active window is 256 dots from dot 24), MWR 0, BXR = BYR = 0 and CR = $0000, then the palette P(i) = (91 * i + 17) & 511 by
; the procedure of H6-5
; (read back against an additive running sum: checks 1 and 2, GUEST pal_bijection), a 32 x 32 BAT of the transparent tile (code $040, index 0 whatever the bank, H7-18) and eleven solid pattern cells: $2000 pixel 15 (PC $100); $2080 and $20C0 a 32-wide pair, the left cell pixel 12 and the right cell pixel 3 (PC $104); and the eight cells of a 32-wide, 64-high sprite (PC $180,
; cell (cx, cy) at
; $3000 + $40 * cx + $80 * cy, H7-22, H7-23): (0, 0) pixel 12, (1, 0) pixel 3, (0, 1) pixel 5, (1, 1) pixel 0, (0, 2) pixel 6, (1, 2) pixel 0, (0, 3) and (1, 3) pixel 9. Every cell is solid, so no bit order, byte order or flip can change what a row reads (ER-12).
; The sprite attribute table (SATB) is not CPU-writable (H7-20): the guest forms the 256 words in VRAM at SPR_SATB_SRC with the writers BAND (n entries of one Y, X stepping by dx, one pattern code, the palette counting up from a base) and ZFILL (zero words: Y = 0 does not cover a display line, which starts at 64) and a write of the high byte of DVSSR copies them at the next
; vertical blanking (H7-12, D-150). Sprites are
; switched on (CR bits 7 and 6) only after the first copy: a seeded power-on SATB would draw random sprites. The scenes (each has its own SATB or its own HSR or HDR; the frames the vexp judges are measured from the sequence, not computed):
; S16 16 sprites of 16 dots at one line (display line 20), dots 0 to 255 side by side, palette 0 to 15. S16 again with HDR $011F (HDE 1: e = 8, 12 slots). S18 18 sprites, X step 14 (dots 14 * i, overlapping by two). S18 with HDR $051F (HDE 5: e = 12, so 2 * (e - 2) = 20 and the cap of 16 binds). S17 17 sprites, X step 15. SPART seven 32-wide sprites, a 16-wide and a 32-wide at
; display line 140: 15 slots used,
;   one left. SNAR entries 55 to 63 only (zeros before), with HDR $031B (HDW 27: d = 28, 2d + 1 = 57 entries searched). SCOL a 32-wide, 64-high sprite 0 with a collider that overlaps its transparent half (lines 36 to 51), one that overlaps its opaque half (lines 52 to 67) and two other sprites that overlap each other (lines 100 to 115).
; The guest reads the status register after one clean frame (a read clears bits 0 to 5, H7-4) and stores it as a check: CR bit 1 (OR) enables the overflow flag and CR bit 0 the collision flag (H7-4); the status polled, not taken as IRQ1 (vdc_stat judges the level). Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 26
DSV_CHECKS = 8

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
        LIT 4, $02                      ; MAN:H7-19 the same 16 sprites with e = 8 (2 * (8 - 2) = 12 slots): four are dropped and OR is set, bit 1 alone
        LIT 5, $02                      ; MAN:H7-19 18 sprites on a line with 16 slots: OR is set, bit 1 alone
        LIT 6, $02                      ; ASSUMED:LED-VID-0012 17 sprites (H7-19 says 17 or more, H7-4 says more than 17): the model sets OR
        LIT 7, $01                      ; MAN:H7-4 sprite 0 opaque over another sprite with CR bit 0 set: the CR flag, bit 0 alone
        LIT 8, $00                      ; MAN:H7-4 the same scene with CR bit 0 clear: the flag is not set, the status read is $00

LIM_N = $2450                           ; the writer parameters in work RAM (clear of SCRATCH, the verdict block and the stack): the entry count
LIM_I = $2451                           ; the entry being written
LIM_Y = $2452                           ; word 0 (Y), low then high byte
LIM_X = $2454                           ; word 1 (X), low then high byte, stepped by LIM_DX after each entry
LIM_PC = $2456                          ; word 2 (the pattern code)
LIM_ATTR = $2458                        ; word 3 (the attributes; the palette of entry i is the low nibble of the base plus i, modulo 16)
LIM_DX = $245A                          ; the X step in dots
LIM_PX = $245B                          ; the pixel value of the cell being written

HDR_A = $031F                           ; HDW 31, HDE 3: d = 32, e = 10
HDR_E8 = $011F                          ; HDE 1: e = 8
HDR_E12 = $051F                         ; HDE 5: e = 12
HDR_D28 = $031B                         ; HDW 27: d = 28
CR_OR = $00C2                           ; BB, SB and the OR enable (bit 1)
CR_COL = $00C1                          ; BB, SB and the CR enable (bit 0)
CR_BBSB = $00C0                         ; BB and SB, both flag enables clear

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
        CELL $2080, 12                  ; PC $104: the 32-wide pair, left cell
        CELL $20C0, 3                   ; the right cell
        CELL $3000, 12                  ; PC $180: the 32 x 64 sprite 0, cell (0, 0)
        CELL $3040, 3                   ; (1, 0) is at +$40: pixel 3
        CELL $3080, 5                   ; (0, 1) is at +$80
        CELL $30C0, 0                   ; (1, 1): pixel 0, transparent
        CELL $3100, 6                   ; (0, 2)
        CELL $3140, 0                   ; (1, 2): transparent
        CELL $3180, 9                   ; (0, 3)
        CELL $31C0, 9                   ; (1, 3)
        VCE_READ_P91
        lda a:VCE_DIFF_LO
        CHECK_LIT 1                     ; check 1 GUEST:pal_bijection
        lda a:VCE_DIFF_HI
        CHECK_LIT 2                     ; check 2 GUEST:pal_bijection
; ---- S16: 16 sprites of one line, then the same with e = 8 ----
        SATB_START
        BAND 16, 84, 32, 16, $100, 0
        ZFILL 192
        SPR_SATB_GO
        BG_FRAME                        ; the copy runs at the vertical blanking of this frame
        VDC_REG BG_R_CR, CR_OR          ; the sprites on
        PHASE_STATUS
        CHECK_LIT 3                  ; check 3 MAN:H7-19: 16 sprites, 16 slots, no overflow
        VDC_REG BG_R_HDR, HDR_E8
        PHASE_STATUS
        CHECK_LIT 4                  ; check 4 MAN:H7-19: 12 slots, four dropped, OR set
; ---- S18 with e = 10, then with e = 12 ----
        VDC_REG BG_R_HDR, HDR_A
        SATB_START
        BAND 18, 84, 32, 14, $100, 0
        ZFILL 184
        SPR_SATB_GO
        BG_FRAME
        PHASE_STATUS
        CHECK_LIT 5                  ; check 5 MAN:H7-19: 18 sprites, OR set
        VDC_REG BG_R_HDR, HDR_E12
        BG_FRAME                        ; a clean frame with the cap of 16 binding (no status check: the record rows judge it)
; ---- S17 ----
        VDC_REG BG_R_HDR, HDR_A
        SATB_START
        BAND 17, 84, 32, 15, $100, 0
        ZFILL 188
        SPR_SATB_GO
        BG_FRAME
        PHASE_STATUS
        CHECK_LIT 6                  ; check 6 ASSUMED:LED-VID-0012: 17 sprites, OR set
; ---- SPART: seven 32-wide sprites, a 16-wide one and a 32-wide one (one slot left) ----
        SATB_START
        BAND 7, 204, 32, 32, $104, SPR_CGX
        BAND 1, 204, 256, 0, $100, 7
        BAND 1, 204, 272, 0, $104, SPR_CGX | 8
        ZFILL 220
        SPR_SATB_GO
        BG_FRAME
        BG_FRAME
; ---- SNAR: d = 28, entries 55 to 63 only ----
        VDC_REG BG_R_HDR, HDR_D28
        SATB_START
        ZFILL 220
        BAND 9, 84, 32, 16, $100, 3
        SPR_SATB_GO
        BG_FRAME
        BG_FRAME
; ---- SCOL: sprite 0 and its colliders; the CR flag enabled, then disabled ----
        VDC_REG BG_R_HDR, HDR_A
        SATB_START
        BAND 1, 84, 72, 0, $180, SPR_CGX | SPR_CGY3 | 1
        BAND 1, 100, 88, 0, $100, 2
        BAND 1, 116, 80, 0, $100, 3
        BAND 1, 164, 72, 0, $100, 4
        BAND 1, 164, 80, 0, $100, 5
        ZFILL 236
        SPR_SATB_GO
        BG_FRAME
        VDC_REG BG_R_CR, CR_COL
        PHASE_STATUS
        CHECK_LIT 7                  ; check 7 MAN:H7-4: CR set
        VDC_REG BG_R_CR, CR_BBSB
        PHASE_STATUS
        CHECK_LIT 8                  ; check 8 MAN:H7-4: no flag with its enable clear
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
