; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_dma.s - the VDC VRAM-to-VRAM transfer fixture (plan 04-10; D-151, D-157, D-169, D-170, D-175, ER-10, F-26, F-27, H7-4, H7-11, H7-12). It is assembled by ca65 in the gate and never committed as a binary. The helpers are the macros of fixtures/lib/vdc.inc and fixtures/lib/vdma.inc.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed and writes the VDC through ST0, ST1 and ST2. CR stays 0 (burst: SB = BB = 0, so every frame lets a transfer run, D-151) except in the one overrun section, which is the single case that covers the cancel at the start of a display period.
; Section A (this slice): 64 words of the assembler table (word i = ($1234 + $F7 * i) & $FFFF, a multiplication) are uploaded to the source block at VRAM word $0800 by a TIA block transfer; a sentinel word is written to the last destination word $0A3F and one beyond the block, $0A40; SOUR = $0800, DESR = $0A00 and LENR = 63 are written with the LENR
; high byte last, which starts the transfer of LENR + 1 = 64 words (H7-12(a)). At once, before the transfer can have moved its last word, the last destination word is read back (check 3): it must still hold the sentinel, so a transfer that completes instantly reads the copied word and fails. After a counted delay the 64 destination words are read back through
; VRR and compared with an additive running sum, a different computation from the assembler's multiplication (GUEST, checks 1 and 2), and the word after the block is read back (check 4): LENR + 1 words moved and not one more (H7-12, M - 1). Nothing here is read from a DuoSpool run.
; The runner kind vdc_vram (case vdc_dma_vram) reads the 64 destination words and the sentinels from the instance VRAM bytes against literals restated from this file (D-175 (c)), so a symmetric bug of the port cannot cancel.
; The classes: GUEST (the additive sum), MAN H7-12 (the trigger, the count), ASSUMED LED-VID-0014 (the copy takes time: check 3 is the expected-red control of an instant transfer, VL1, no speed is claimed). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 20
DSV_CHECKS = 4

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"
        .include "vdma.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                ; GUEST:vram-sum the 64 destination low bytes equal the additive running sum: the OR of the differences is zero
        LIT 2, $00                ; GUEST:vram-sum the 64 destination high bytes equal the same running sum
        LIT 3, $C3                ; ASSUMED:LED-VID-0014 right after the LENR high byte write the last destination word still holds its sentinel $A5C3 (low byte $C3): the copy takes time, so an instant transfer, which would read the source word $4EFD (low byte $FD), fails
        LIT 4, $E1                ; MAN:H7-12 LENR + 1 words move and not one more: the word after the block still holds its sentinel $C2E1 (low byte $E1)

SRC = $0800                     ; the source block: 64 words of the table
DST = $0A00                     ; the destination block: 64 words
SENT_LAST = $A5C3               ; the sentinel of the last destination word, DST + 63
SENT_AFTER = $C2E1              ; the sentinel of the word after the block, DST + 64

; the source of the upload: 64 words, word i = ($1234 + $F7 * i) & $FFFF formed by the assembler (a multiplication); the guest checks the copy with an additive running sum
        .pushseg
        .segment "RODATA"
vdma_tab:
        .repeat 64, i
        .word (VDC_W0 + VDC_WSTEP * i) & $FFFF
        .endrepeat
        .popseg

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        csh                             ; the high speed: 3 master clocks per CPU cycle
        VDC_REG VR_CR, $0000            ; CR = 0: no enable, burst
        VDC_REG VR_DCR, $0000           ; DCR = 0: source and destination increment, no end interrupt
; ---- A: 64 words, LENR = 63, in burst ----
        VDC_REG VR_MAWR, SRC
        VDC_SEL VR_VWR
        tia vdma_tab, VDC_LO, 128       ; the destination alternates $0002 and $0003: low byte, high byte, 64 words at SRC to SRC + 63
        VDMA_POKE DST + 63, SENT_LAST   ; the sentinel of the last destination word
        VDMA_POKE DST + 64, SENT_AFTER  ; the sentinel after the block
        VDMA_GO SRC, DST, 63            ; the LENR high byte write starts the transfer
        VDMA_PEEK DST + 63              ; the buffer is loaded at once by the MARR high byte write: the last destination word, before the transfer can have reached it
        CHECK_LIT 3                     ; check 3 ASSUMED:LED-VID-0014 the last destination word is not copied yet
        VDMA_DELAY 100                  ; 1800 master clocks, more than the 64 words take at the dot clock divider 4 (a counted delay, never a position)
        VDMA_READ_SUM DST, 64
        stz a:GEXP                      ; the expected difference is zero
        lda a:VDC_DIFF_LO
        CHECK_GUEST 1                   ; check 1 GUEST:vram-sum every low byte equals the additive sum
        lda a:VDC_DIFF_HI
        CHECK_GUEST 2                   ; check 2 GUEST:vram-sum every high byte equals the additive sum
        VDMA_PEEK DST + 64
        CHECK_LIT 4                     ; check 4 MAN:H7-12 the word after the block is untouched
        rts
