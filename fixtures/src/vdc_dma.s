; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_dma.s - the VDC VRAM-to-VRAM transfer fixture (plan 04-10; D-151, D-157, D-169, D-170, D-175, ER-10, F-26, F-27, H7-4, H7-11, H7-12). It is assembled by ca65 in the gate and never committed as a binary. The helpers are the macros of fixtures/lib/vdc.inc and fixtures/lib/vdma.inc.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), selects the high speed and writes the VDC through ST0, ST1 and ST2. CR stays 0 (burst: SB = BB = 0, so every frame lets a transfer run, D-151) in sections A to J. Section K is the one overrun case: it sets SB and a vertical setup and covers the cancel at the start of a display period.
; Every destination word the guest reads is a word the transfer moved or a sentinel the guest wrote, so the check bytes are equal under every power-on policy (D-167). The source is a table of 64 words formed by the assembler, word i = ($1234 + $F7 * i) & $FFFF (a multiplication), uploaded to VRAM word $0800 by a TIA block transfer: W0 = $1234, W1 = $132B, W2 = $1422, W3 =
; $1519, W10 = $1BDA, W63 = $4EFD.
; A: LENR = 63 in burst: 64 words from $0800 to $0A00. At once the last destination word is read back (check 1: it still holds its sentinel, so an instant transfer, which copies it, fails); after a counted delay the 64 words equal an additive running sum (GUEST, checks 2 and 3, a different computation from the multiplication) and the word after the block still holds its
; sentinel (check 4, LENR + 1 words, H7-12).
; B: LENR = 0 moves exactly one word (M - 1, H7-12, checks 5 and 6). C and D: DCR bit 2 (SI/D) steps the source down and bit 3 (DI/D) the destination (H7-11, checks 7 to 10). E: DV is set at the end only when DCR bit 1 is set, and a status read clears it (H7-4, checks 11 to 13). F: IW does not apply (CR IW = 11, check 14). G: a forward overlapping copy propagates (check 15).
; H: MAWR, MARR and the read buffer are untouched (checks 16 to 18). I: a LENR low byte write alone starts nothing and a lone high byte write retriggers with the stored low byte (checks 19 and 20). J: the poll count until the last word arrives is recorded as an observe row (check 21, ASSUMED LED-VID-0014, VL1: no speed is claimed).
; K: the overrun. A vertical setup (VPR = $0F02, VDW = 239, VCR = 4, HSR = $0200, HDR = $001F) and CR = $0048 (SB set: not burst; VD enabled) are written, two VBlank flags pass so that the display line 0 of a whole frame has seen CR, and a transfer of 4096 words is started right after the third; the display period that follows starts long before the 4096 words are done, so
; the transfer is cancelled there (H7-4, H7-12: the rest is not transferred in the next period): its first word has moved (check 22), its last word has not (check 23) and DV, enabled in DCR, was never set (check 24). No count of moved words is asserted and the test points keep far from any word boundary (D-169).
; The runner kind vdc_vram (case vdc_dma_vram) reads the final words from the instance VRAM bytes against literals restated from this file (D-175 (c)). Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
; The classes: GUEST (the additive sum), MAN (H7-4 the DV rules, H7-11 the DCR bits, H7-12 the trigger byte and the count M - 1), ASSUMED LED-VID-0014 (the copy takes time; IW never applies; an overlapping copy propagates; the registers the transfer leaves alone; a lone high byte write retriggers; the overrun is cancelled with no DV; any row that needs the word time).
        .setcpu "huc6280"

DSV_ID = 20
DSV_CHECKS = 24

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"
        .include "vdma.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $C3                ; ASSUMED:LED-VID-0014 right after the LENR high byte write the last destination word still holds its sentinel $A5C3 (low byte $C3): the copy takes time, so an instant transfer, which would read the source word $4EFD (low byte $FD), fails
        LIT 2, $00                ; GUEST:vram-sum the 64 destination low bytes equal the additive running sum: the OR of the differences is zero
        LIT 3, $00                ; GUEST:vram-sum the 64 destination high bytes equal the same running sum
        LIT 4, $E1                ; MAN:H7-12 LENR + 1 words move and not one more: the word after the block still holds its sentinel $C2E1 (low byte $E1)
        LIT 5, $DA                ; MAN:H7-12 LENR = 0 moves one word: the destination holds W10 = $1BDA (low byte $DA)
        LIT 6, $22                ; MAN:H7-12 LENR = 0 moves exactly one word: the next destination word still holds its sentinel $2222 (low byte $22)
        LIT 7, $19                ; MAN:H7-11 SI/D set steps the source down: from W3 the first destination word is W3 = $1519 (low byte $19)
        LIT 8, $34                ; MAN:H7-11 SI/D set: the fourth destination word is W0 = $1234 (low byte $34)
        LIT 9, $34                ; MAN:H7-11 DI/D set steps the destination down: W0 lands at the highest address, $0D03, low byte $34
        LIT 10, $19               ; MAN:H7-11 DI/D set: W3 lands at the lowest address $0D00, low byte $19
        LIT 11, $00               ; ASSUMED:LED-VID-0008 (the converse of H7-4, the reading of three emulators) with DCR bit 1 clear the end of the transfer sets no flag: the status read is $00
        LIT 12, $10               ; MAN:H7-4 with DCR bit 1 set the end of the transfer sets DV, bit 4 of the status: $10
        LIT 13, $00               ; MAN:H7-4 the status read cleared DV: the next read is $00
        LIT 14, $2B               ; ASSUMED:LED-VID-0014 IW does not apply to the transfer: with CR IW = 11 the destination is still consecutive and the second word is W1 = $132B (low byte $2B)
        LIT 15, $5B               ; ASSUMED:LED-VID-0014 a forward overlapping copy propagates: a copy from $0E20 to $0E21 of 4 words leaves the fifth word equal to the first, $7A5B (low byte $5B)
        LIT 16, $5A               ; ASSUMED:LED-VID-0014 the transfer leaves the read buffer alone: it still holds the word $5A5A loaded before it (low byte $5A)
        LIT 17, $6B               ; ASSUMED:LED-VID-0014 the transfer leaves MARR alone: the next read operation loads the word $6B6B at $0F11 (low byte $6B)
        LIT 18, $4D               ; ASSUMED:LED-VID-0014 the transfer leaves MAWR alone: a word written afterwards lands at $0F40, $9C4D (low byte $4D)
        LIT 19, $57               ; MAN:H7-12 the transfer is triggered by the LENR high byte: a low byte write alone starts nothing, the destination still holds its sentinel $1357 (low byte $57)
        LIT 20, $34               ; ASSUMED:LED-VID-0014 a lone LENR high byte write retriggers with the stored low byte (LENR = $0003, 4 words): the destination holds W0 = $1234 (low byte $34)
        LIT 21, $00               ; ASSUMED:LED-VID-0014 the polls until the last destination word arrives (an observe row: recorded, never judged by a literal; VL1 has no speed claim)
        LIT 22, $E3               ; ASSUMED:LED-VID-0014 a transfer of 4096 words started at the VBlank entry with SB set is cut at the display start: its first word has moved, the destination holds $5CE3 (low byte $E3)
        LIT 23, $A9               ; ASSUMED:LED-VID-0014 the cut transfer never reached its last word: the destination still holds its sentinel $B7A9 (low byte $A9)
        LIT 24, $00               ; ASSUMED:LED-VID-0014 (H7-4 states it; the transfer is still running at the display start only under the modelled speed) a transfer cancelled by the start of a display period sets no end status: bit 4 of the status read after the next VBlank is 0 although DV is enabled (the class of the row is that of the cut, ASSUMED LED-VID-0014)

SRC = $0800                     ; the source block: 64 words of the table
DST = $0A00                     ; the destination block of section A: 64 words
SENT_LAST = $A5C3               ; the sentinel of the last destination word, DST + 63
SENT_AFTER = $C2E1              ; the sentinel of the word after the block, DST + 64
OVR_SRC = $4000                 ; the overrun source and destination: 4096 words each, far from every other block
OVR_DST = $2000

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
        VDC_REG VR_MAWR, SRC
        VDC_SEL VR_VWR
        tia vdma_tab, VDC_LO, 128       ; the destination alternates $0002 and $0003: low byte, high byte, 64 words at SRC to SRC + 63
; ---- A: 64 words, LENR = 63, in burst ----
        VDMA_POKE DST + 63, SENT_LAST   ; the sentinel of the last destination word
        VDMA_POKE DST + 64, SENT_AFTER  ; the sentinel after the block
        VDMA_GO SRC, DST, 63            ; the LENR high byte write starts the transfer
        VDMA_PEEK DST + 63              ; the MARR high byte write loads the buffer at once: the last destination word, before the transfer can have reached it
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-VID-0014 the last destination word is not copied yet
        VDMA_DELAY 100                  ; 1800 master clocks, more than the 64 words take at the dot clock divider 4 (a counted delay, never a position)
        VDMA_READ_SUM DST, 64
        stz a:GEXP                      ; the expected difference is zero
        lda a:VDC_DIFF_LO
        CHECK_GUEST 2                   ; check 2 GUEST:vram-sum every low byte equals the additive sum
        lda a:VDC_DIFF_HI
        CHECK_GUEST 3                   ; check 3 GUEST:vram-sum every high byte equals the additive sum
        VDMA_PEEK DST + 64
        CHECK_LIT 4                     ; check 4 MAN:H7-12 the word after the block is untouched
; ---- B: LENR = 0 moves one word ----
        VDMA_POKE $0B00, $1111
        VDC_DATA $2222                  ; the second word, at $0B01
        VDMA_GO SRC + 10, $0B00, 0
        VDMA_DELAY 20
        VDMA_PEEK $0B00
        CHECK_LIT 5                     ; check 5 MAN:H7-12 W10 moved
        VDMA_PEEK $0B01
        CHECK_LIT 6                     ; check 6 MAN:H7-12 nothing else moved
; ---- C: SI/D, the source steps down ----
        VDC_REG VR_DCR, DCR_SID
        VDMA_GO SRC + 3, $0C00, 3
        VDMA_DELAY 20
        VDMA_PEEK $0C00
        CHECK_LIT 7                     ; check 7 MAN:H7-11 W3 first
        VDMA_PEEK $0C03
        CHECK_LIT 8                     ; check 8 MAN:H7-11 W0 last
; ---- D: DI/D, the destination steps down ----
        VDC_REG VR_DCR, DCR_DID
        VDMA_GO SRC, $0D03, 3
        VDMA_DELAY 20
        VDMA_PEEK $0D03
        CHECK_LIT 9                     ; check 9 MAN:H7-11 W0 at the highest address
        VDMA_PEEK $0D00
        CHECK_LIT 10                    ; check 10 MAN:H7-11 W3 at the lowest address
; ---- E: DV gated by DCR bit 1, cleared by the status read ----
        VDC_REG VR_DCR, $0000
        VDMA_GO SRC, $0E00, 3
        VDMA_DELAY 20
        lda a:VDC_AR
        CHECK_LIT 11                    ; check 11 ASSUMED:LED-VID-0008 no flag with the enable clear
        VDC_REG VR_DCR, DCR_DV
        VDMA_GO SRC, $0E00, 3
        VDMA_DELAY 20
        lda a:VDC_AR
        CHECK_LIT 12                    ; check 12 MAN:H7-4 DV
        lda a:VDC_AR
        CHECK_LIT 13                    ; check 13 MAN:H7-4 cleared by the read
; ---- F: IW does not apply ----
        VDC_REG VR_DCR, $0000
        VDC_REG VR_CR, $1800            ; IW = 11: VWR and VRR would step by $80
        VDMA_GO SRC, $0E10, 3
        VDMA_DELAY 20
        VDC_REG VR_CR, $0000
        VDMA_PEEK $0E11
        CHECK_LIT 14                    ; check 14 ASSUMED:LED-VID-0014 consecutive destination
; ---- G: a forward overlapping copy propagates ----
        VDC_REG VR_MAWR, $0E20
        VDC_SEL VR_VWR
        VDC_DATA $7A5B                  ; $0E20
        VDC_DATA $1111
        VDC_DATA $2222
        VDC_DATA $3333
        VDC_DATA $4444                  ; $0E24
        VDMA_GO $0E20, $0E21, 3
        VDMA_DELAY 20
        VDMA_PEEK $0E24
        CHECK_LIT 15                    ; check 15 ASSUMED:LED-VID-0014 the first word reached the fifth
; ---- H: MAWR, MARR and the read buffer are untouched ----
        VDC_REG VR_MAWR, $0F10
        VDC_SEL VR_VWR
        VDC_DATA $5A5A                  ; $0F10
        VDC_DATA $6B6B                  ; $0F11
        VDC_REG VR_MAWR, $0F40
        VDC_REG VR_MARR, $0F10          ; the buffer holds $5A5A, MARR = $0F11
        VDMA_GO SRC, $0F20, 3
        VDMA_DELAY 20
        VDC_SEL VR_VWR
        lda a:VDC_LO
        CHECK_LIT 16                    ; check 16 ASSUMED:LED-VID-0014 the buffer was not touched
        lda a:VDC_HI                    ; the high byte read triggers the next read at MARR
        lda a:VDC_LO
        CHECK_LIT 17                    ; check 17 ASSUMED:LED-VID-0014 MARR was not touched
        VDC_SEL VR_VWR
        VDC_DATA $9C4D                  ; the word lands at MAWR
        VDMA_PEEK $0F40
        CHECK_LIT 18                    ; check 18 ASSUMED:LED-VID-0014 MAWR was not touched
; ---- I: the LENR high byte is the trigger ----
        VDMA_POKE $0F80, $1357
        VDC_REG VR_SOUR, SRC
        VDC_REG VR_DESR, $0F80
        VDC_SEL VR_LENR
        st1 #$03                        ; the low byte alone: stored, nothing starts
        VDMA_DELAY 20
        VDMA_PEEK $0F80
        CHECK_LIT 19                    ; check 19 MAN:H7-12 nothing started
        VDC_SEL VR_LENR
        st2 #$00                        ; the high byte alone: LENR = $0003, 4 words
        VDMA_DELAY 20
        VDMA_PEEK $0F80
        CHECK_LIT 20                    ; check 20 ASSUMED:LED-VID-0014 a lone high byte write retriggers
; ---- J: the polls until the last word arrives (an observe row) ----
        VDMA_POKE $103F, SENT_LAST
        VDMA_GO SRC, $1000, 63
        ldx #0
pollj:  VDC_REG VR_MARR, $103F          ; reload the buffer from the last destination word
        VDC_SEL VR_VWR
        lda a:VDC_LO
        cmp #$FD                        ; the low byte of W63, which the transfer brings there last
        beq pollje
        inx
        cpx #200                        ; bounded: a machine that never copies it ends the loop at 200
        bne pollj
pollje: txa
        CHECK_OBSERVE 21                ; check 21 ASSUMED:LED-VID-0014 the number of polls, recorded
; ---- K: the overrun: SB set, a transfer started at the VBlank entry is cut at the display start ----
        VDMA_POKE OVR_SRC, $5CE3        ; the first source word
        VDMA_POKE OVR_DST, $1D2F        ; the first destination word, before the transfer
        VDMA_POKE OVR_DST + 4095, $B7A9 ; the last destination word, before the transfer
        VDC_REG 12, $0F02               ; VPR: VSW = 2 (bits 4-0), VDS = 15 (bits 15-8)
        VDC_REG 13, 239                 ; VDW = 239
        VDC_REG 14, 4                   ; VCR = 4: 17 + 240 + 4 + 3 = 264 lines, more than the frame length
        VDC_REG 10, $0200               ; HSR: HSW = 0, HDS = 2
        VDC_REG 11, $001F               ; HDR: HDW = 31
        VDC_REG VR_DCR, DCR_DV          ; DV enabled: a completion would show
        VDC_REG VR_CR, $0048            ; SB set (bit 6): not burst; VD enabled (bit 3)
        VDMA_WAIT_VD                    ; the first VBlank flag
        lda a:VDC_AR                    ; acknowledge
        VDMA_WAIT_VD                    ; the next one, a whole frame later: the display line 0 of the frame in between saw CR with SB set
        lda a:VDC_AR                    ; acknowledge
        VDMA_GO OVR_SRC, OVR_DST, 4095  ; 4096 words, started within a few dozen master clocks of the VBlank entry
        VDMA_WAIT_VD                    ; the next VBlank, a whole frame later: the display start in between cut the transfer
        lda a:VDC_AR
        and #$10                        ; bit 4: DV
        CHECK_LIT 24                    ; check 24 ASSUMED:LED-VID-0014 no end status for a cancelled transfer
        VDMA_PEEK OVR_DST
        CHECK_LIT 22                    ; check 22 ASSUMED:LED-VID-0014 the first word moved
        VDMA_PEEK OVR_DST + 4095
        CHECK_LIT 23                    ; check 23 ASSUMED:LED-VID-0014 the last word did not
        rts
