; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_reg.s - the VDC register and VRAM fixture (plan 04-06; D-146, D-148, D-149, D-169, D-170, D-175, ER-10, F-26, F-27, H7-2, H7-3, H7-5, H7-7, H7-25, H8-8). It is assembled by ca65 in the gate and never committed as a binary. The helpers are the macros of fixtures/lib/vdc.inc.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF, as st_bypass does), so the VDC registers are at logical $0000 (status and AR), $0001, $0002 (data LSB) and $0003 (data MSB), repeated every four bytes up to $03FF. ST0, ST1 and ST2 write the physical window without the mapping registers (H8-8: A1,A0 = 00, 10, 11).
; Sections: A two words written by VWR and read back by VRR (the high byte write of MARR starts the read, the high byte read of VRR triggers the next, H7-5, H7-25); B the buffer is the OLD word (a word overwritten after the MARR write is seen one read later); C the window mirrors $0004 to $0007 and $03FC to $03FF; D the AR width (H7-3) and a reserved AR;
; E $0001 (reads $00, H7-25; a write is ignored); F the status read; G the IW steps +1, $20, $40, $80 of VWR and VRR (H7-7), each word read back from the address it must land at; H MARR low byte and VRR low read start no read; I the write latch; J the out-of-range word addresses and the wrap of MAWR; K the reserved register R03; L the ST triple with MPR0 mapped to work RAM
; and the sta triple (H8-8); M a TIA upload of 32 words to the data port, read back through VRR by an LDA loop against an additive running sum (GUEST: a different arithmetic from the multiplicative table the assembler forms), and a TAI readback of the same 32 words into work RAM (ASSUMED LED-CPU-0008).
; Every word the guest reads it wrote first, so the check bytes are equal under every power-on policy (D-167); the VRAM bytes are never read at a word the guest did not write. CR stays $0000 or an IW value ($0800, $1000, $1800) and returns to $0000: the IE bits are never set, so no status flag can arise whatever the vertical machine does (plan 04-08).
; The runner kind vdc_vram reads the final words from the instance VRAM bytes against literals restated from this file (D-175 (c)). Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
; The classes: MAN (H7-3, H7-5, H7-7, H7-25, H8-8: the documented triggers, steps, widths and the ST triple), GUEST (vram-sum: 32 words against an additive running sum), ASSUMED (LED-VID-0006 mirrors, reserved registers, the write latch, the idle write rules; LED-VID-0007 the out-of-range rule; LED-VID-0015 the status BSY level; LED-CPU-0008 the TAI readback).
        .setcpu "huc6280"

DSV_ID = 16
DSV_CHECKS = 37

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $34                ; MAN:H7-5 the first VRR low read is the low byte of the word $1234 written at $0100
        LIT 2, $12                ; MAN:H7-25 the first VRR high read is the high byte of the same word; this read triggers the next one
        LIT 3, $78                ; MAN:H7-5 the second VRR low read is the low byte of the word $5678 written at $0101
        LIT 4, $56                ; MAN:H7-25 the second VRR high read is its high byte
        LIT 5, $2B                ; MAN:H7-5 the CPU receives the buffer as it was: the word at $0121 was overwritten after the MARR write, and the first V
        LIT 6, $BC                ; MAN:H7-25 the high byte read of VRR triggered the next word, which now holds the overwrite $9ABC: the next VRR low read 
        LIT 7, $A1                ; ASSUMED:LED-VID-0006 a word written through the mirrors $0004, $0006 and $0007 comes back through the mirror $0006: the low byte $A
        LIT 8, $B2                ; ASSUMED:LED-VID-0006 the high byte $B2 through the mirror $0007
        LIT 9, $C3                ; ASSUMED:LED-VID-0006 a word written through the last mirror $03FC, $03FE and $03FF comes back through $03FE: the low byte $C3 of th
        LIT 10, $D4               ; ASSUMED:LED-VID-0006 the high byte $D4 through $03FF
        LIT 11, $0A               ; MAN:H7-3 AR keeps bits 4-0: a write of $E2 selects VWR and the word $0B0A is stored at MAWR $0220; the low byte reads $
        LIT 12, $22               ; ASSUMED:LED-VID-0006 an AR of $15 names no register (bit 4 is kept) and its data write does not reach CR: the second word of two la
        LIT 13, $00               ; MAN:H7-25 a read of $0001 returns $00 while the read buffer is not zero
        LIT 14, $5C               ; ASSUMED:LED-VID-0006 a write to $0001 is ignored and does not change AR: the next data write still reaches VWR, the word $6D5C at $
        LIT 15, $00               ; ASSUMED:LED-VID-0015 the status read with nothing pending is $00, bit 6 (BSY) included
        LIT 16, $00               ; ASSUMED:LED-VID-0006 the status read through the mirror $03FC is $00
        LIT 17, $02               ; MAN:H7-7 IW 00 steps MAWR by 1: the second word $0A02 of two lands at $0401 and its low byte reads $02
        LIT 18, $12               ; MAN:H7-7 IW 01 steps MAWR by $20: the second word $0B12 lands at $0520 (from $0500) and its low byte reads $12
        LIT 19, $22               ; MAN:H7-7 IW 10 steps MAWR by $40: the second word $0C22 lands at $0640 (from $0600) and its low byte reads $22
        LIT 20, $32               ; MAN:H7-7 IW 11 steps MAWR by $80: the second word $0D32 lands at $0780 (from $0700) and its low byte reads $32
        LIT 21, $12               ; MAN:H7-7 with IW 01 the read operation steps MARR by $20: after MARR = $0500 the second VRR pair reads the word at $052
        LIT 22, $32               ; MAN:H7-7 with IW 11 the read operation steps MARR by $80: after MARR = $0700 the second VRR pair reads the word at $078
        LIT 23, $34               ; ASSUMED:LED-VID-0006 a write of the low byte of MARR alone starts no read: the buffer still holds the word $1234 loaded from $0100,
        LIT 24, $34               ; ASSUMED:LED-VID-0006 a read of the low byte of VRR starts no read: a second low read gives $34 again
        LIT 25, $11               ; MAN:H7-5 a VWR low byte write alone stores nothing: the word $1111 at $0B00 still reads $11 after a write of $22 to the
        LIT 26, $22               ; ASSUMED:LED-VID-0006 the high byte write after the lone low byte write stores the word $3322 at $0B00 with the latched low byte: re
        LIT 27, $22               ; ASSUMED:LED-VID-0006 the write latch is not cleared by the write: a second high byte write alone stores $4422 at $0B01, low byte $2
        LIT 28, $21               ; ASSUMED:LED-VID-0007 a write at word address $8000 is dropped and does not alias word $0000: the sentinel $4321 still reads $21
        LIT 29, $00               ; ASSUMED:LED-VID-0007 a read at word address $8000 returns 0 over a buffer that held $4321 (low byte $21)
        LIT 30, $88               ; ASSUMED:LED-VID-0007 MAWR steps and wraps modulo 65,536 after a write at $FFFF that is dropped: the next word $7788 lands at $0000,
        LIT 31, $34               ; ASSUMED:LED-VID-0006 with AR = 3 (reserved) a data low read returns the low byte of the read buffer ($34 of the word $1234 loaded f
        LIT 32, $34               ; ASSUMED:LED-VID-0006 the high byte read with AR = 3 started no read operation: with AR = 2 the low byte still reads $34
        LIT 33, $DE               ; MAN:H8-8 ST0, ST1 and ST2 reach the VDC while MPR0 maps work RAM: the word $C0DE written at $0A00 reads $DE
        LIT 34, $DE               ; MAN:H8-8 the same word written with sta to $0000, $0002 and $0003 at $0A10 reads $DE: the sta triple equals the ST trip
        LIT 35, $00               ; GUEST:vram-sum a TIA upload of 64 bytes to the data port stores 32 words; the low bytes read back through VRR equal the addit
        LIT 36, $00               ; GUEST:vram-sum the high bytes of the same 32 words equal the same running sum: the OR of the differences is zero
        LIT 37, $00               ; ASSUMED:LED-CPU-0008 a TAI readback of 64 bytes from the data port into work RAM gives the same 32 words: the OR of the differences

; the source of the TIA upload: 32 words, word i = ($1234 + $F7 * i) & $FFFF formed by the assembler (a multiplication); the guest checks them with an additive running sum
        .pushseg
        .segment "RODATA"
vdc_tab:
        .repeat 32, i
        .word (VDC_W0 + VDC_WSTEP * i) & $FFFF
        .endrepeat
        .popseg

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
; ---- A: two words through VWR, read back through VRR ----
        VDC_REG 0, $0100                ; MAWR = $0100
        VDC_SEL 2                       ; AR = VWR
        VDC_DATA $1234                  ; the word $1234 at $0100 (the high byte write stores it), MAWR = $0101 (H7-5)
        VDC_DATA $5678                  ; the word $5678 at $0101, MAWR = $0102
        VDC_REG 1, $0100                ; MARR = $0100: the high byte write starts the read (H7-5)
        VDC_SEL 2                       ; AR = VRR
        lda a:VDC_LO
        CHECK_LIT 1                     ; check 1 MAN:H7-5 the low byte of the first word
        lda a:VDC_HI
        CHECK_LIT 2                     ; check 2 MAN:H7-25 the high byte of the first word; this read triggers the next one
        lda a:VDC_LO
        CHECK_LIT 3                     ; check 3 MAN:H7-5 the low byte of the second word
        lda a:VDC_HI
        CHECK_LIT 4                     ; check 4 MAN:H7-25 the high byte of the second word
; ---- B: the CPU receives the buffer as it was ----
        VDC_REG 0, $0120
        VDC_SEL 2
        VDC_DATA $1A2B                  ; the word $1A2B at $0120
        VDC_DATA $3C4D                  ; the word $3C4D at $0121
        VDC_REG 1, $0120                ; the buffer holds $1A2B, MARR = $0121
        VDC_REG 0, $0121
        VDC_SEL 2
        VDC_DATA $9ABC                  ; the word at $0121 is now $9ABC, after the buffer was loaded
        lda a:VDC_LO
        CHECK_LIT 5                     ; check 5 MAN:H7-5 the buffer is the old word: the low byte $2B, not $BC
        lda a:VDC_HI                    ; the high byte read triggers the next word, the overwrite $9ABC
        lda a:VDC_LO
        CHECK_LIT 6                     ; check 6 MAN:H7-25 the next word is the new one: the low byte $BC
; ---- C: the window mirrors (A2 to A9 are not decoded) ----
        VDC_REG 0, $0200                ; MAWR = $0200 by the ST triple
        lda #$02
        sta a:VDC_MIR_AR                ; AR = VWR through the mirror $0004
        lda #$A1
        sta a:VDC_MIR_LO                ; the low byte through $0006
        lda #$B2
        sta a:VDC_MIR_HI                ; the high byte through $0007: the word $B2A1 at $0200
        lda #$00
        sta a:VDC_FAR_AR                ; AR = MAWR through the last mirror $03FC
        lda #$00
        sta a:VDC_FAR_LO
        lda #$03
        sta a:VDC_FAR_HI                ; MAWR = $0300 through $03FE and $03FF
        lda #$02
        sta a:VDC_FAR_AR                ; AR = VWR through $03FC
        lda #$C3
        sta a:VDC_FAR_LO
        lda #$D4
        sta a:VDC_FAR_HI                ; the word $D4C3 at $0300
        VDC_REG 1, $0200                ; MARR = $0200 by the ST triple
        VDC_SEL 2
        lda a:VDC_MIR_LO
        CHECK_LIT 7                     ; check 7 ASSUMED:LED-VID-0006 the low byte through the mirror $0006
        lda a:VDC_MIR_HI
        CHECK_LIT 8                     ; check 8 ASSUMED:LED-VID-0006 the high byte through the mirror $0007
        lda #$01
        sta a:VDC_FAR_AR                ; AR = MARR through $03FC
        lda #$00
        sta a:VDC_FAR_LO
        lda #$03
        sta a:VDC_FAR_HI                ; MARR = $0300: the read starts
        lda #$02
        sta a:VDC_FAR_AR                ; AR = VRR
        lda a:VDC_FAR_LO
        CHECK_LIT 9                     ; check 9 ASSUMED:LED-VID-0006 the low byte through $03FE
        lda a:VDC_FAR_HI
        CHECK_LIT 10                    ; check 10 ASSUMED:LED-VID-0006 the high byte through $03FF
; ---- D: the AR width ----
        VDC_REG 0, $0220                ; MAWR = $0220
        VDC_REG $E2, $0B0A              ; AR = $E2 keeps bits 4-0 = 2 (VWR): the word $0B0A at $0220 (H7-3)
        VDC_REG 1, $0220
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 11                    ; check 11 MAN:H7-3 the word was stored by an AR of $E2
        VDC_REG 5, $0000                ; CR = 0: IW +1
        VDC_REG 0, $0230                ; MAWR = $0230
        VDC_REG $15, $1800              ; AR = $15 (bit 4 kept: no register): the data write must not reach CR
        VDC_SEL 2
        VDC_DATA $2111                  ; the word $2111 at $0230
        VDC_DATA $3222                  ; the word $3222 at $0231 (IW +1), at $02B0 if the write had set IW 11 in CR
        VDC_REG 1, $0231
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 12                    ; check 12 ASSUMED:LED-VID-0006 an AR of $15 reaches no register
; ---- E: $0001 ----
        lda a:VDC_R1
        CHECK_LIT 13                    ; check 13 MAN:H7-25 a read of $0001 is $00 (the read buffer holds $3222)
        VDC_REG 0, $0240                ; MAWR = $0240
        VDC_SEL 2
        lda #$07
        sta a:VDC_R1                    ; a write to $0001 is ignored: AR stays 2
        VDC_DATA $6D5C                  ; the word $6D5C at $0240
        VDC_REG 1, $0240
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 14                    ; check 14 ASSUMED:LED-VID-0006 the write to $0001 changed nothing
; ---- F: the status read ----
        lda a:VDC_AR
        CHECK_LIT 15                    ; check 15 ASSUMED:LED-VID-0015 the status is $00
        lda a:VDC_FAR_AR
        CHECK_LIT 16                    ; check 16 ASSUMED:LED-VID-0006 the status through the mirror $03FC is $00
; ---- G: the IW steps ----
        VDC_REG 5, $0000                ; IW 00: +1
        VDC_REG 0, $0400
        VDC_SEL 2
        VDC_DATA $0A01
        VDC_DATA $0A02                  ; the second word at $0401
        VDC_REG 1, $0401
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 17                    ; check 17 MAN:H7-7 IW 00: the second word is one word on
        VDC_REG 5, $0800                ; IW 01: +$20
        VDC_REG 0, $0500
        VDC_SEL 2
        VDC_DATA $0B11
        VDC_DATA $0B12                  ; the second word at $0520
        VDC_REG 5, $0000                ; IW +1 again for the read of the landing address
        VDC_REG 1, $0520
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 18                    ; check 18 MAN:H7-7 IW 01: the second word is $20 words on
        VDC_REG 5, $1000                ; IW 10: +$40
        VDC_REG 0, $0600
        VDC_SEL 2
        VDC_DATA $0C21
        VDC_DATA $0C22                  ; the second word at $0640
        VDC_REG 5, $0000
        VDC_REG 1, $0640
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 19                    ; check 19 MAN:H7-7 IW 10: the second word is $40 words on
        VDC_REG 5, $1800                ; IW 11: +$80
        VDC_REG 0, $0700
        VDC_SEL 2
        VDC_DATA $0D31
        VDC_DATA $0D32                  ; the second word at $0780
        VDC_REG 5, $0000
        VDC_REG 1, $0780
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 20                    ; check 20 MAN:H7-7 IW 11: the second word is $80 words on
        VDC_REG 5, $0800                ; IW 01 for the read operation: MARR = $0500 loads $0B11 and steps to $0520
        VDC_REG 1, $0500
        VDC_SEL 2
        lda a:VDC_LO                    ; the low byte of the first word
        lda a:VDC_HI                    ; the high byte read loads the word at MARR = $0520
        lda a:VDC_LO
        CHECK_LIT 21                    ; check 21 MAN:H7-7 the read pointer stepped by $20
        VDC_REG 5, $1800                ; IW 11: MARR = $0700 loads $0D31 and steps to $0780
        VDC_REG 1, $0700
        VDC_SEL 2
        lda a:VDC_LO
        lda a:VDC_HI
        lda a:VDC_LO
        CHECK_LIT 22                    ; check 22 MAN:H7-7 the read pointer stepped by $80
        VDC_REG 5, $0000                ; CR = 0 again
; ---- H: a MARR low byte write and a VRR low byte read start no read ----
        VDC_REG 1, $0100                ; the buffer holds $1234, MARR = $0101
        VDC_SEL 1
        st1 #$20                        ; the low byte of MARR alone: MARR = $0120, no read (the word there is $1A2B)
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 23                    ; check 23 ASSUMED:LED-VID-0006 the buffer is still $1234
        lda a:VDC_LO
        CHECK_LIT 24                    ; check 24 ASSUMED:LED-VID-0006 a low byte read starts no read
; ---- I: the write latch ----
        VDC_REG 0, $0B00
        VDC_SEL 2
        VDC_DATA $1111                  ; the word $1111 at $0B00
        VDC_REG 0, $0B00
        VDC_SEL 2
        st1 #$22                        ; the low byte alone: nothing is stored, the latch takes $22
        VDC_REG 1, $0B00
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 25                    ; check 25 MAN:H7-5 the word at $0B00 is still $1111
        VDC_REG 0, $0B00
        VDC_SEL 2
        st2 #$33                        ; the high byte alone: the word $3322 at $0B00 (the latched low byte)
        st2 #$44                        ; the high byte again: the word $4422 at $0B01 (the latch is not cleared)
        VDC_REG 1, $0B00
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 26                    ; check 26 ASSUMED:LED-VID-0006 the high byte write used the latched low byte
        lda a:VDC_HI
        lda a:VDC_LO
        CHECK_LIT 27                    ; check 27 ASSUMED:LED-VID-0006 the latch survived the first high byte write
; ---- J: the out-of-range word addresses ----
        VDC_REG 0, $0000
        VDC_SEL 2
        VDC_DATA $4321                  ; the sentinel $4321 at word $0000
        VDC_REG 0, $8000
        VDC_SEL 2
        VDC_DATA $FFFF                  ; a word at $8000 is dropped (and must not alias word $0000)
        VDC_REG 1, $0000
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 28                    ; check 28 ASSUMED:LED-VID-0007 the write at $8000 was dropped
        VDC_REG 1, $8000                ; the buffer held $4321: a read at $8000 loads 0
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 29                    ; check 29 ASSUMED:LED-VID-0007 a read at $8000 is 0
        VDC_REG 0, $FFFF
        VDC_SEL 2
        VDC_DATA $0F0F                  ; a word at $FFFF is dropped and MAWR wraps to $0000
        VDC_DATA $7788                  ; the word $7788 at $0000
        VDC_REG 1, $0000
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 30                    ; check 30 ASSUMED:LED-VID-0007 MAWR wrapped after the dropped word
; ---- K: the reserved register R03 ----
        VDC_REG 1, $0100                ; the buffer holds $1234, MARR = $0101
        VDC_SEL 3                       ; AR = 3: reserved, a data read gives the buffer byte
        lda a:VDC_LO
        CHECK_LIT 31                    ; check 31 ASSUMED:LED-VID-0006 the low byte of the buffer
        lda a:VDC_HI                    ; the high byte read: no read operation (AR is not VRR)
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 32                    ; check 32 ASSUMED:LED-VID-0006 the buffer was not reloaded
; ---- L: the ST triple with MPR0 mapped to work RAM, and the sta triple ----
        lda #$F8
        tam #$01                        ; MPR0 = $F8: logical $0000-$1FFF is work RAM, not the VDC
        VDC_REG 0, $0A00                ; the ST triple reaches the VDC whatever MPR0 holds (H8-8)
        VDC_SEL 2
        VDC_DATA $C0DE                  ; the word $C0DE at $0A00
        lda #$FF
        tam #$01                        ; MPR0 = $FF again
        VDC_STA_AR 0
        lda #$10
        sta a:VDC_LO
        lda #$0A
        sta a:VDC_HI                    ; MAWR = $0A10 by the sta triple
        VDC_STA_AR 2
        lda #$DE
        sta a:VDC_LO
        lda #$C0
        sta a:VDC_HI                    ; the word $C0DE at $0A10
        VDC_REG 1, $0A00
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 33                    ; check 33 MAN:H8-8 the word written by ST with MPR0 = work RAM
        VDC_REG 1, $0A10
        VDC_SEL 2
        lda a:VDC_LO
        CHECK_LIT 34                    ; check 34 MAN:H8-8 the word written by sta equals it
; ---- M: a TIA upload of 32 words and an LDA readback, a TAI readback ----
        VDC_REG 0, $0800
        VDC_SEL 2
        tia vdc_tab, VDC_LO, 64         ; the destination alternates $0002 and $0003: low byte, high byte, 32 words at $0800 to $081F
        VDC_REG 1, $0800
        VDC_SEL 2
        VDC_READ_SUM
        stz a:GEXP                      ; the expected difference is zero
        lda a:VDC_DIFF_LO
        CHECK_GUEST 35                  ; check 35 GUEST:vram-sum every low byte equals the additive sum
        lda a:VDC_DIFF_HI
        CHECK_GUEST 36                  ; check 36 GUEST:vram-sum every high byte equals the additive sum
        VDC_REG 1, $0800
        VDC_SEL 2
        tai VDC_LO, VDC_TAI_BUF, 64     ; the source alternates $0002 and $0003: the buffer low byte, then the high byte, 32 words into work RAM
        VDC_CMP_RAM
        lda a:VDC_DIFF_LO
        ora a:VDC_DIFF_HI
        CHECK_LIT 37                    ; check 37 ASSUMED:LED-CPU-0008 the TAI readback equals the additive sum
        rts
