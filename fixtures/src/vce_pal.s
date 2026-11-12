; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vce_pal.s - the VCE palette and control register fixture (plans 04-04 and 04-19; D-137, D-141, D-152, D-153, D-169, D-170, ER-10, F-25, F-39, H6-3, H6-5). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF, as st_bypass does), so the VCE registers are at logical $0400 (CR), $0402 (CTA low), $0403 (CTA high) and $0404 and $0405 (CTW and CTR low and high); the register of an access is the offset & 7 (H6-2). The helpers are the macros of fixtures/lib/vce.inc.
; Sequence: write CR $00 (the division ratio 4 of H6-3), run a counted delay of at least 5,460 master clocks, write CR $01 (the division ratio 3 of H6-3), read the unlisted addresses, then write the colour table: entry i = P(i) = (91 * i + 17) & 511 for all 512 entries by the procedure of H6-5 (CTA low, CTA high, data low, data high; the address is incremented
; after the high byte, so the following entries need no further address write), the product formed by shift and add. 91 is odd, so P is a bijection of 0..511. Entry $100 is the border of the line record: P(256) = (91 * 256 + 17) & 511 = 23313 & 511 = 23313 - 45 * 512 = 273, and no later test touches it (the border row of the video case needs 273 to stay).
; The 512 entries are read back against a second computation, the additive running sum E(0) = 17, E(i + 1) = (E(i) + 91) & 511, which is a different arithmetic from the multiplicative writer and catches an 8-bit address alias (entries 256 to 511 would overwrite 0 to 255).
; After that the register tests use entries 0, $20, $21, $30, $31, $40, $50 to $53, $60, $160 and $1FF, each written by the guest before it is read (policy independent, D-167).
; The delay: LDX #128 (2 cycles), 127 passes of DEX (2) and BNE taken (2 + 2 for the taken branch = 4), and a last pass of DEX (2) and BNE not taken (2): 2 + 127 * 6 + 4 = 768 CPU cycles (tests/oracle/huc6280-opcodes.tsv: LDX imm 2, DEX 2, BNE 2 plus the taken-branch adder of 2). The fixture never executes CSH, so the CPU runs at the power-up low speed, 12 master clocks
; per cycle (constants.def cycle_mclk_low, community-reported, ASSUMED LED-CPU-0003): 768 * 12 = 9,216 master clocks, above the 5,460 (four lines of 1,365) that the runner needs so that at least one whole line record lies between the two CR writes with a line of margin on each side. At the high speed (3 master clocks) it would be 2,304, so the fixture must never switch speed.
; The tail delay at the end of the fixture keeps the guest alive for about 13 more lines after the last palette write, so the runner can judge the border one line away from it. Nothing here is read from a DuoSpool run. No HuC, PCEAS, emulator or cc65 library code.
; The classes: MAN (H6-3, H6-5: the increment after the high byte and the procedure, the entry values the guest wrote), GUEST (pal_bijection: the 512 entries
; against the additive sum), ASSUMED LED-VID-0016 (unlisted reads, the unused data bits, the wrap, independent halves, the mirrors and the block transfer: modelling choices that the manual does not give).
        .setcpu "huc6280"

DSV_ID = 15
DSV_CHECKS = 32

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vce.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $FF                      ; ASSUMED:LED-VID-0016 a read of CR gives the unused value
        LIT 2, $FF                      ; ASSUMED:LED-VID-0016 a read of CTA low gives the unused value
        LIT 3, $FF                      ; ASSUMED:LED-VID-0016 a read of CTA high gives the unused value
        LIT 4, $FF                      ; ASSUMED:LED-VID-0016 a read of $0401 gives the unused value
        LIT 5, $FF                      ; ASSUMED:LED-VID-0016 a read of $0406 gives the unused value
        LIT 6, $FF                      ; ASSUMED:LED-VID-0016 a read of $0407 gives the unused value
        LIT 7, $FF                      ; ASSUMED:LED-VID-0016 a read of $0408 (the CR mirror) gives the unused value
        LIT 8, $11                      ; MAN:H6-5 entry 0 low byte = P(0) = 17
        LIT 9, $00                      ; MAN:H6-5 entry 0 bit 8 = 0 (P(0) = 17 < 256)
        LIT 10, $11                     ; MAN:H6-5 entry $100 low byte = P(256) & $FF = 273 & $FF = $11
        LIT 11, $01                     ; MAN:H6-5 entry $100 bit 8 = 1 (273 >= 256)
        LIT 12, $00                     ; GUEST:pal-bijection the XOR mask of the expected byte of the low byte rows
        LIT 13, $00                     ; GUEST:pal-bijection the XOR mask of the expected byte of the bit 8 rows
        LIT 14, $A5                     ; MAN:H6-3 a low byte write does not step the address: the second low write replaced the first
        LIT 15, $01                     ; MAN:H6-3 the high byte write of entry $20 set bit 8
        LIT 16, $33                     ; MAN:H6-3 the address stepped once, after the high byte: the next entry is $21
        LIT 17, $00                     ; MAN:H6-3 entry $21 bit 8 is 0
        LIT 18, $FF                     ; ASSUMED:LED-VID-0016 a CTR high byte reads bits 7 to 1 as 1 over bit 8 = 1
        LIT 19, $FE                     ; ASSUMED:LED-VID-0016 a CTR high byte reads bits 7 to 1 as 1 over bit 8 = 0
        LIT 20, $00                     ; ASSUMED:LED-VID-0016 the halves are independent: a low rewrite changed the low byte
        LIT 21, $01                     ; ASSUMED:LED-VID-0016 the halves are independent: a low rewrite kept bit 8
        LIT 22, $7E                     ; ASSUMED:LED-VID-0016 the halves are independent: a high rewrite kept the low byte
        LIT 23, $01                     ; ASSUMED:LED-VID-0016 the halves are independent: a high rewrite set bit 8
        LIT 24, $A9                     ; ASSUMED:LED-VID-0016 a CTA low write keeps bit 8 of the address
        LIT 25, $56                     ; ASSUMED:LED-VID-0016 a CTA high write keeps the low byte and takes bit 0 only
        LIT 26, $2B                     ; ASSUMED:LED-VID-0016 the address wraps from $1FF to 0: the next entry written is entry 0
        LIT 27, $C4                     ; ASSUMED:LED-VID-0016 entry $1FF holds the value written before the wrap
        LIT 28, $6D                     ; ASSUMED:LED-VID-0016 a write through the mirror $0444 and a read through the mirror $040C reach the same entry
        LIT 29, $01                     ; ASSUMED:LED-VID-0016 the mirrors $0445 and $040D reach the same bit 8
        LIT 30, $9D                     ; ASSUMED:LED-VID-0016 a TIA block write reaches the table alternating CTW low and high: entry $50 low
        LIT 31, $01                     ; ASSUMED:LED-VID-0016 entry $50 bit 8 after the block write
        LIT 32, $11                     ; ASSUMED:LED-VID-0016 the address stepped after the high byte inside the block: entry $51 low

; the source of the block write, entries $50 to $53 (low byte, high byte; entry $50 = $19D, $51 = $011, $52 = $122, $53 = $033)
        .pushseg
        .segment "RODATA"
tia_tab:
        .byte $9D, $01, $11, $00, $22, $01, $33, $00
        .popseg

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        stz a:VCE_CR                    ; CR $00: division ratio 4 (MAN H6-3)
        ldx #128                        ; the counted delay: 768 CPU cycles = 9,216 master clocks at the low speed (see the header)
delay:  dex
        bne delay
        lda #$01
        sta a:VCE_CR                    ; CR $01: division ratio 3 (MAN H6-3)
; the unlisted addresses read the unused value (nothing is written to them)
        lda a:VCE_CR
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-VID-0016 CR reads $FF
        lda a:VCE_CTA_LO
        CHECK_LIT 2                     ; check 2 ASSUMED:LED-VID-0016 CTA low reads $FF
        lda a:VCE_CTA_HI
        CHECK_LIT 3                     ; check 3 ASSUMED:LED-VID-0016 CTA high reads $FF
        lda a:VCE_UNUSED_1
        CHECK_LIT 4                     ; check 4 ASSUMED:LED-VID-0016 $0401 reads $FF
        lda a:VCE_UNUSED_6
        CHECK_LIT 5                     ; check 5 ASSUMED:LED-VID-0016 $0406 reads $FF
        lda a:VCE_UNUSED_7
        CHECK_LIT 6                     ; check 6 ASSUMED:LED-VID-0016 $0407 reads $FF
        lda a:VCE_MIR_CR
        CHECK_LIT 7                     ; check 7 ASSUMED:LED-VID-0016 $0408 reads $FF
; the 512 entries, product by shift and add
        VCE_WRITE_P91
; read entry 0 back (H6-5: CTA low, CTA high, then the data low byte first)
        VCE_CTA 0
        lda a:VCE_CTW_LO
        CHECK_LIT 8                     ; check 8 MAN:H6-5 entry 0 low byte reads 17
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 9                     ; check 9 MAN:H6-5 entry 0 bit 8 reads 0
; read entry $100 back, the border of the line record
        VCE_CTA $0100
        lda a:VCE_CTW_LO
        CHECK_LIT 10                    ; check 10 MAN:H6-5 entry $100 low byte reads $11
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 11                    ; check 11 MAN:H6-5 entry $100 bit 8 reads 1
; all 512 entries against the additive running sum: the OR of the differences must be zero
        VCE_READ_P91
        stz a:GEXP                      ; the expected difference is zero
        lda a:VCE_DIFF_LO
        CHECK_GUEST 12                  ; check 12 GUEST:pal-bijection every low byte equals the additive sum
        lda a:VCE_DIFF_HI
        CHECK_GUEST 13                  ; check 13 GUEST:pal-bijection every bit 8 equals the additive sum
; the address steps after the high byte only: entry $20 gets $5A then $A5 (no step between), then bit 8 set; entry $21 gets $33 and bit 8 clear
        VCE_CTA $0020
        VCE_LO $5A
        VCE_LO $A5
        VCE_HI $01
        VCE_LO $33
        VCE_HI $00
        VCE_CTA $0020
        lda a:VCE_CTW_LO
        CHECK_LIT 14                    ; check 14 MAN:H6-3 entry $20 low byte is the second low write
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 15                    ; check 15 MAN:H6-3 entry $20 bit 8 is set
        lda a:VCE_CTW_LO
        CHECK_LIT 16                    ; check 16 MAN:H6-3 the address stepped once: entry $21 low byte
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 17                    ; check 17 MAN:H6-3 entry $21 bit 8 is clear
; the unused bits of a CTR high byte read 1
        VCE_CTA $0020
        lda a:VCE_CTW_HI
        CHECK_LIT 18                    ; check 18 ASSUMED:LED-VID-0016 entry $20 high byte reads $FF (bit 8 set)
        lda a:VCE_CTW_HI
        CHECK_LIT 19                    ; check 19 ASSUMED:LED-VID-0016 entry $21 high byte reads $FE (bit 8 clear)
; the halves of an entry are independent: entry $30 = $1FF, then a low byte rewrite alone; entry $31 = $07E, then a high byte rewrite alone
        VCE_CTA $0030
        VCE_LO $FF
        VCE_HI $01
        VCE_CTA $0030
        VCE_LO $00
        VCE_CTA $0030
        lda a:VCE_CTW_LO
        CHECK_LIT 20                    ; check 20 ASSUMED:LED-VID-0016 entry $30 low byte is the rewrite
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 21                    ; check 21 ASSUMED:LED-VID-0016 entry $30 bit 8 survived the low rewrite
        VCE_CTA $0031
        VCE_LO $7E
        VCE_HI $00
        VCE_CTA $0031
        VCE_HI $01
        VCE_CTA $0031
        lda a:VCE_CTW_LO
        CHECK_LIT 22                    ; check 22 ASSUMED:LED-VID-0016 entry $31 low byte survived the high rewrite
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 23                    ; check 23 ASSUMED:LED-VID-0016 entry $31 bit 8 is the rewrite
; the halves of the address are independent: entry $60 = $056 and entry $160 = $1A9, then a CTA low write alone and a CTA high write alone
        VCE_CTA $0060
        VCE_LO $56
        VCE_HI $00
        VCE_CTA $0160
        VCE_LO $A9
        VCE_HI $01
        lda #$60
        sta a:VCE_CTA_LO                ; the address was $161: the low write keeps bit 8, so it is $160
        lda a:VCE_CTW_LO
        CHECK_LIT 24                    ; check 24 ASSUMED:LED-VID-0016 entry $160 low byte after a CTA low write alone
        lda #$FE
        sta a:VCE_CTA_HI                ; only bit 0 (here 0) counts: the address is $060
        lda a:VCE_CTW_LO
        CHECK_LIT 25                    ; check 25 ASSUMED:LED-VID-0016 entry $60 low byte after a CTA high write alone
; the wrap: entry $1FF is written, the address wraps to 0 and the next entry written is entry 0
        VCE_CTA $01FF
        VCE_LO $C4
        VCE_HI $01
        VCE_LO $2B
        VCE_HI $00
        VCE_CTA $0000
        lda a:VCE_CTW_LO
        CHECK_LIT 26                    ; check 26 ASSUMED:LED-VID-0016 entry 0 holds the write after the wrap
        VCE_CTA $01FF
        lda a:VCE_CTW_LO
        CHECK_LIT 27                    ; check 27 ASSUMED:LED-VID-0016 entry $1FF holds the write before the wrap
; the window mirrors: entry $40 = $16D written through $0444 and $0445, the address set through $040A and $040B, read through $040C and $040D
        VCE_CTA $0040
        lda #$6D
        sta a:VCE_MIR2_CTW_LO
        lda #$01
        sta a:VCE_MIR2_CTW_HI
        lda #$40
        sta a:VCE_MIR_CTA_LO
        lda #$00
        sta a:VCE_MIR_CTA_HI
        lda a:VCE_MIR_CTR_LO
        CHECK_LIT 28                    ; check 28 ASSUMED:LED-VID-0016 entry $40 low byte through the mirrors
        lda a:VCE_MIR_CTR_HI
        and #$01
        CHECK_LIT 29                    ; check 29 ASSUMED:LED-VID-0016 entry $40 bit 8 through the mirrors
; a block transfer: TIA alternates the destination between CTW low and CTW high, so four entries ($50 to $53) arrive from eight bytes
        VCE_CTA $0050
        tia tia_tab, VCE_CTW_LO, 8
        VCE_CTA $0050
        lda a:VCE_CTW_LO
        CHECK_LIT 30                    ; check 30 ASSUMED:LED-VID-0016 entry $50 low byte after the block write
        lda a:VCE_CTW_HI
        and #$01
        CHECK_LIT 31                    ; check 31 ASSUMED:LED-VID-0016 entry $50 bit 8 after the block write
        lda a:VCE_CTW_LO
        CHECK_LIT 32                    ; check 32 ASSUMED:LED-VID-0016 entry $51 low byte: the address stepped inside the block
; the tail delay: 2 + 255 * 6 + 4 = 1,536 CPU cycles = 18,432 master clocks at the low speed (about 13 lines), so that the run lasts for whole line records after the last palette write and the runner can judge the border one line away from it
        ldx #0
tail:   dex
        bne tail
        rts
