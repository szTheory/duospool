; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; mmu_mpr.s - the memory map and the mapping registers seen from the guest (plan 03-16; D-102, D-103, D-104, D-124, D-125, D-127; H8-7, H8-8, H8-9, H8-24, S8-90, S8-101). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps a bank into the logical slots $6000 (MPR3) and $8000 (MPR4) and reads or writes through them, so every check is one translate and one class of the bus: unmapped banks, the 8 KiB work RAM mirrored four times,
; HuCard banks whose bytes this file writes itself (segment BANKS: the first byte of file bank k is k and the last byte is k xor $A5, so a bank, an offset and a mirror can each be told apart from the $FF of an unmapped read), and, from
; check 22 on, the windows of the hardware page and the bus latch (io_buffer). ROMBANKS is the image size in banks: 32 gives 256 KiB, 48 gives 384 KiB and 128 gives 1 MiB (plan 03-20; D-127), three images of this one source (scripts/fixture-build). The two checks that name a
; mirror of the HuCard window (18 and 19) differ by image size and take their bank and literal from conditional assembly on ROMBANKS below; every other check is the same in the three images. Every check reads cells the fixture has written (D-127): RAM cells are written first, HuCard bytes are the BANKS data, and io_buffer is set by a write to the I/O window before every read that depends on it.
; The classes: MAN rows follow a page (H8-7: the MPR byte supplies the physical address bits A13 to A20; S8-101: TMA reads what TAM stored), ASSUMED rows name the project's modelling assumption (the ledger entries LED-CPU-0011 to LED-CPU-0015 and LED-LOAD-0003).
; No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 8
DSV_CHECKS = 60

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

SL3 = $08                       ; the TAM and TMA operand of MPR3 (logical $6000-$7FFF)
SL4 = $10                       ; MPR4 (logical $8000-$9FFF)

; Checks 18 and 19 read the first byte of one bank through MPR3: the bank and the expected byte depend on the mirror rule of the image size (ASSUMED:LED-LOAD-0003, the rule text of the ledger; file bank k carries the tag k in its first byte for k = 1 to ROMBANKS - 1, and bank 0 starts with the SEI $78 of the reset code):
;   256 KiB (32 banks):  file bank = b mod 32.   Bank $20 mirrors bank 0 ($78), bank $3F mirrors bank $1F ($1F).
;   384 KiB (48 banks):  file bank = b and $1F below $40, $20 + (b and $0F) from $40.   Bank $40 is file bank 32 ($20), bank $7F is file bank $2F = 47, the last bank of the image ($2F).
;   1 MiB (128 banks):   file bank = b.   Bank $20 is file bank $20 ($20), bank $7F is file bank $7F ($7F).
        .if ROMBANKS = 32
BANK18 = $20
BYTE18 = $78
BANK19 = $3F
BYTE19 = $1F
        .elseif ROMBANKS = 48
BANK18 = $40
BYTE18 = $20
BANK19 = $7F
BYTE19 = $2F
        .elseif ROMBANKS = 128
BANK18 = $20
BYTE18 = $20
BANK19 = $7F
BYTE19 = $7F
        .else
        .error "mmu_mpr.s: no literals for this ROMBANKS (32, 48 and 128 are the three images)"
        .endif

        CHECK_TABLE DSV_CHECKS
        LIT 1, $FF              ; SELFTEST-TARGET
        LIT 2, $FF
        LIT 3, $FF
        LIT 4, $FF
        LIT 5, $FF
        LIT 6, $5A
        LIT 7, $5A
        LIT 8, $5A
        LIT 9, $3C
        LIT 10, $00
        LIT 11, $77
        LIT 12, $01
        LIT 13, $A4
        LIT 14, $1F
        LIT 15, $02
        LIT 16, $03
        LIT 17, $00
        LIT 18, BYTE18
        LIT 19, BYTE19
        LIT 20, $01
        LIT 21, $80
        LIT 22, $00
        LIT 23, $00
        LIT 24, $00
        LIT 25, $FF
        LIT 26, $FF
        LIT 27, $A5
        LIT 28, $A5
        LIT 29, $C3
        LIT 30, $C3
        LIT 31, $3A
        LIT 32, $80
        LIT 33, $00
        LIT 34, $80
        LIT 35, $80
        LIT 36, $7F
        LIT 37, $A0
        LIT 38, $A0
        LIT 39, $F8
        LIT 40, $A5
        LIT 41, $0F
        LIT 42, $96
        LIT 43, $5C
        LIT 44, $FF
        LIT 45, $A5
        LIT 46, $A5
        LIT 47, $A5
        LIT 48, $A5
        LIT 49, $00             ; an observe row (the case supplies the observed byte)
        LIT 50, $FA
        LIT 51, $FA
        LIT 52, $FF
        LIT 53, $F8
        LIT 54, $F8
        LIT 55, $00
        LIT 56, $F9
        LIT 57, $FF
        LIT 58, $00
        LIT 59, $00             ; an observe row
        LIT 60, $00             ; an observe row

; MAP mask, bank: write one mapping register (A is clobbered)
        .macro MAP mask, bank
        lda #(bank)
        tam #(mask)
        .endmacro

; SETIO value: set io_buffer by a write to the I/O window (physical $1FF000, logical $7000 while MPR3 = $FF): the byte of the last access in $0800-$17FF is the byte just written (A is clobbered)
        .macro SETIO value
        lda #(value)
        sta a:$7000
        .endmacro

; the tagged banks: file bank k = 1 to ROMBANKS - 1 holds k in its first byte and k xor $A5 in its last byte, $FF between (bank 0 is the code and the literal table)
        .segment "BANKS"
        .repeat ROMBANKS - 1, bk
        .byte (bk + 1)
        .res $1FFE, $FF
        .byte ((bk + 1) ^ $A5)
        .endrepeat
        .segment "CODE"

fixture_main:
; ---- unmapped banks (D-103, D-104, ASSUMED:LED-CPU-0012) ----
        MAP SL3, $80
        lda a:$6000                     ; check 1 ASSUMED:LED-CPU-0012: bank $80 is not mapped: a read returns the constant $FF
        CHECK_LIT 1
        MAP SL3, $F7
        lda a:$6000                     ; check 2 ASSUMED:LED-CPU-0012: bank $F7 is the last of the unmapped run $80-$F7
        CHECK_LIT 2
        MAP SL3, $FC
        lda a:$6000                     ; check 3 ASSUMED:LED-CPU-0012: bank $FC follows the four RAM banks and is not mapped
        CHECK_LIT 3
        MAP SL3, $FE
        lda a:$6000                     ; check 4 ASSUMED:LED-CPU-0012: bank $FE is the last unmapped bank before the hardware page
        CHECK_LIT 4
        MAP SL3, $80
        lda #$A5
        sta a:$6000                     ; a write to an unmapped bank is ignored
        lda a:$6000                     ; check 5 ASSUMED:LED-CPU-0012: the byte stored to unmapped bank $80 is not kept: the read is still $FF
        CHECK_LIT 5

; ---- the work RAM mirrored four times in banks $F8-$FB (D-103, ASSUMED:LED-CPU-0011) ----
        MAP SL3, $F8
        MAP SL4, $F9
        lda #$00
        sta a:$6601                     ; RAM offset $0601 holds $00
        lda #$5A
        sta a:$6600                     ; RAM offset $0600 written through bank $F8
        lda a:$8600                     ; check 6 ASSUMED:LED-CPU-0011: the same cell read through bank $F9
        CHECK_LIT 6
        MAP SL4, $FA
        lda a:$8600                     ; check 7 ASSUMED:LED-CPU-0011: the same cell read through bank $FA
        CHECK_LIT 7
        MAP SL4, $FB
        lda a:$8600                     ; check 8 ASSUMED:LED-CPU-0011: the same cell read through bank $FB
        CHECK_LIT 8
        lda #$3C
        sta a:$8600                     ; written through bank $FB
        lda a:$6600                     ; check 9 ASSUMED:LED-CPU-0011: the cell read through bank $F8 shows the byte written through bank $FB
        CHECK_LIT 9
        lda a:$8601                     ; check 10 ASSUMED:LED-CPU-0011: the next cell is a different byte, not an alias: it still holds $00
        CHECK_LIT 10
        lda #$77
        sta a:$9234                     ; RAM offset $1234 written through bank $FB (offset bit 12 set)
        lda a:$7234                     ; check 11 ASSUMED:LED-CPU-0011: read through bank $F8 at the same offset
        CHECK_LIT 11

; ---- HuCard banks: the MPR byte is the bank, the offset is the low 13 address bits (H8-7, D-102) ----
        MAP SL3, $01
        lda a:$6000                     ; check 12 MAN:H8-7: the MPR byte supplies the physical bank: bank $01 holds its tag byte $01 at offset 0
        CHECK_LIT 12
        lda a:$7FFF                     ; check 13 MAN:H8-7: the last offset of bank $01 holds $01 xor $A5 = $A4
        CHECK_LIT 13
        MAP SL3, $1F
        lda a:$6000                     ; check 14 MAN:H8-7: bank $1F (the last bank of the 256 KiB image) holds $1F at offset 0
        CHECK_LIT 14
        MAP SL3, $01
        lda a:$6000
        MAP SL3, $02
        lda a:$6000                     ; check 15 MAN:H8-7: a TAM takes effect for the next access: the same logical address now shows bank $02
        CHECK_LIT 15
        MAP SL4, $03
        lda a:$8000                     ; check 16 MAN:H8-7: another slot maps another bank at the same time: MPR4 = $03 shows its tag
        CHECK_LIT 16
        lda a:$E000                     ; the first byte of bank 0 seen through MPR7, which the reset maps to bank 0
        sta a:GEXP
        MAP SL3, $00
        lda a:$6000                     ; check 17 GUEST:bank0-two-views: bank 0 through MPR3 shows the byte that MPR7 shows at $E000
        CHECK_GUEST 17
        MAP SL3, BANK18
        lda a:$6000                     ; check 18 ASSUMED:LED-LOAD-0003: the first byte of bank BANK18 under the mirror rule of this image size: 256 KiB bank $20 mirrors bank 0 (the SEI opcode $78 of the reset code), 384 KiB bank $40 is file bank 32 ($20), 1 MiB bank $20 is file bank $20 ($20)
        CHECK_LIT 18
        MAP SL3, BANK19
        lda a:$6000                     ; check 19 ASSUMED:LED-LOAD-0003: the first byte of bank BANK19: 256 KiB bank $3F mirrors bank $1F ($1F), 384 KiB bank $7F is file bank 47 ($2F), 1 MiB bank $7F is file bank $7F ($7F)
        CHECK_LIT 19
        MAP SL3, $01
        lda #$5C
        sta a:$6000                     ; a write to a HuCard bank is ignored
        lda a:$6000                     ; check 20 ASSUMED:LED-CPU-0012: the byte stored to bank $01 is not kept: the tag byte $01 is still there
        CHECK_LIT 20
        lda #$80
        tam #SL3                        ; MPR3 = $80
        lda #$00
        tma #SL3                        ; A = MPR3
        CHECK_LIT 21                    ; check 21 MAN:S8-101: TMA returns the value TAM stored, also for a bank number that maps nothing: $80

; ---- the hardware page, bank $FF at logical $6000 (H8-9, H8-24, D-103, D-104): window k of $400 bytes at $6000 + k * $400: 0 VDC, 1 VCE, 2 PSG, 3 timer, 4 I/O, 5 interrupt registers, 6 and 7 empty ----
; nothing has written the I/O window yet, so the first read of the PSG window shows the power-on value of io_buffer
        MAP SL3, $FF
        lda a:$6800                     ; check 22 ASSUMED:LED-CPU-0013: io_buffer is constant 0 at power-on: the PSG window reads $00 before any access in $0800-$17FF
        CHECK_LIT 22
        SETIO $A5
        lda a:$6000                     ; check 23 ASSUMED:LED-CPU-0012: the VDC window is a stub that reads $00 (and not io_buffer, which holds $A5)
        CHECK_LIT 23
        SETIO $A5
        lda a:$63FF                     ; check 24 ASSUMED:LED-CPU-0012: the last byte of the 1 KiB VDC window reads the stub too (H8-24: the VDC register is A1-A0, the window repeats)
        CHECK_LIT 24
        SETIO $00
        lda a:$6400                     ; check 25 ASSUMED:LED-CPU-0012: the VCE window is a stub that reads $FF (and not io_buffer, which holds $00)
        CHECK_LIT 25
        SETIO $00
        lda a:$67FF                     ; check 26 ASSUMED:LED-CPU-0012: the last byte of the VCE window reads the stub too (H8-24: the VCE register is A2-A0)
        CHECK_LIT 26
        SETIO $A5
        lda a:$6000                     ; a read in the VDC window
        lda a:$6800                     ; check 27 ASSUMED:LED-CPU-0013: it did not change io_buffer ($0000-$07FF is outside $0800-$17FF)
        CHECK_LIT 27
        SETIO $A5
        lda a:$6400                     ; a read in the VCE window
        lda a:$6800                     ; check 28 ASSUMED:LED-CPU-0013: it did not change io_buffer
        CHECK_LIT 28
        SETIO $C3
        lda a:$6800                     ; check 29 ASSUMED:LED-CPU-0013: the PSG window reads io_buffer, here $C3
        CHECK_LIT 29
        SETIO $C3
        lda a:$6BFF                     ; check 30 ASSUMED:LED-CPU-0013: the last byte of the PSG window does too (H8-24: the PSG register is A3-A0)
        CHECK_LIT 30
        lda #$3A
        sta a:$6805                     ; a write in the PSG window
        lda a:$6807                     ; check 31 ASSUMED:LED-CPU-0013: a write in $0800-$17FF stores its byte in io_buffer, so the next PSG read returns it
        CHECK_LIT 31
        SETIO $A5
        lda a:$6C00                     ; check 32 ASSUMED:LED-CPU-0013: the timer counter read has 7 bits, so bit 7 comes from io_buffer ($A5: bit 7 set) over a counter stub of 0 (H8-24)
        CHECK_LIT 32
        SETIO $25
        lda a:$6C00                     ; check 33 ASSUMED:LED-CPU-0013: with bit 7 of io_buffer clear ($25) the same read is $00
        CHECK_LIT 33
        SETIO $A5
        lda a:$6C00                     ; a timer read ($80)
        lda a:$6800                     ; check 34 ASSUMED:LED-CPU-0013: a read in $0800-$17FF makes the byte read the new io_buffer
        CHECK_LIT 34
        SETIO $FF
        lda a:$6C01                     ; check 35 ASSUMED:LED-CPU-0013: the timer window is one 7-bit read for both of its addresses until plan 03-21: bit 7 from io_buffer, the rest 0
        CHECK_LIT 35
        lda #$7F
        sta a:$6C00                     ; a write to the timer reload register (a stub: the byte is not kept)
        lda a:$6800                     ; check 36 ASSUMED:LED-CPU-0013: the write stored its byte in io_buffer
        CHECK_LIT 36
        SETIO $A5
        lda a:$7402                     ; check 37 ASSUMED:LED-CPU-0013: $1402, the interrupt disable register (H8-24): three register bits (stub 0), the upper five from io_buffer: $A5 and $F8 = $A0
        CHECK_LIT 37
        SETIO $A5
        lda a:$7403                     ; check 38 ASSUMED:LED-CPU-0013: $1403, the interrupt request register, reads the same way
        CHECK_LIT 38
        SETIO $FF
        lda a:$77FE                     ; check 39 ASSUMED:LED-CPU-0013: the window repeats (H8-24: A1-A0 select the register): offset $3FE has A1 = 1 and A0 = 0, so it is $1402: $FF and $F8 = $F8
        CHECK_LIT 39
        SETIO $A5
        lda a:$7400                     ; check 40 ASSUMED:LED-CPU-0013: the interrupt window addresses with A1 = 0 name no register: the read is io_buffer ($A5)
        CHECK_LIT 40
        lda #$0F
        sta a:$7402                     ; a write to the interrupt disable register (a stub: the byte is not kept)
        lda a:$6800                     ; check 41 ASSUMED:LED-CPU-0013: the write stored its byte in io_buffer
        CHECK_LIT 41
        lda #$96
        sta a:$73FF                     ; a write at the last byte of the I/O window
        lda a:$6800                     ; check 42 ASSUMED:LED-CPU-0013: the I/O window is part of $0800-$17FF: the write stored its byte in io_buffer
        CHECK_LIT 42
        SETIO $5C
        lda a:$7000                     ; check 43 ASSUMED:LED-CPU-0012: the I/O window has no pad behind it yet (plan 03-27): the read is open bus, io_buffer ($5C)
        CHECK_LIT 43
        SETIO $A5
        lda a:$7800                     ; check 44 ASSUMED:LED-CPU-0012: $1800-$1FFF have no device (H8-9: reserved): the read is $FF
        CHECK_LIT 44
        SETIO $A5
        lda a:$7800                     ; a read in the empty window
        lda a:$6800                     ; check 45 ASSUMED:LED-CPU-0013: it did not change io_buffer
        CHECK_LIT 45
        SETIO $A5
        lda #$11
        sta a:$7FFF                     ; a write in the empty window
        lda a:$6800                     ; check 46 ASSUMED:LED-CPU-0013: it did not change io_buffer
        CHECK_LIT 46
        SETIO $A5
        lda #$11
        sta a:$6001                     ; a write in the VDC window
        lda a:$6800                     ; check 47 ASSUMED:LED-CPU-0013: it did not change io_buffer
        CHECK_LIT 47
        SETIO $A5
        st0 #$22                        ; ST0 writes physical $1FE000, the VDC window, without the MPRs (H8-8 Table 2-2-3, D-106)
        lda a:$6800                     ; check 48 ASSUMED:LED-CPU-0013: it did not change io_buffer
        CHECK_LIT 48

; ---- the TAM and TMA edges that the manual does not define (D-105, R-4, ER-8; ASSUMED:LED-CPU-0015): the manual documents the one-hot operand 2^i only (S8-90, S8-101) ----
; The operand $00 of TMA is the model parameter tma_zero (model_params.def): bus_latch returns the one-byte latch (the last value TAM or TMA moved), a_unchanged leaves A alone. Check 49 is an observe-only row, so each case
; supplies the byte it expects under its own value of the parameter (observe=49:VALUE); TAM and TMA with a nonzero operand put their byte in the latch, here TAM #$02 with A = $F8 (MPR1 already holds $F8, so the map does not move).
        lda #$F8
        tam #$02                        ; MPR1 = $F8 again; the latch takes $F8
        lda #$5A
        tma #0                          ; TMA with the operand $00 (R-4: it assembles normally): A = the latch ($F8) under bus_latch, $5A (unchanged) under a_unchanged
        CHECK_OBSERVE 49

; TAM with a multi-bit operand: the operand is written as .byte because the manual defines only 2^i (S8-90). MPR1 and MPR2 are the zero-page slot and the verdict slot, so the value written is $FA, another work RAM bank: the RAM is mirrored four times (checks 6 to 11),
; the map stays valid, and a register that was written shows as $FA where it held $F8 or $F9. The single-constant rule under test is "every selected MPR is written" (D-105).
        lda #$FA
        .byte $53, $06                  ; RAW:S8-90 ASSUMED:LED-CPU-0015: TAM with the operand $06 (MPR1 and MPR2, two bits set; the manual defines one bit only)
        tma #$02
        CHECK_LIT 50                    ; check 50 ASSUMED:LED-CPU-0015: MPR1 was written ($FA, it held $F8)
        tma #$04
        CHECK_LIT 51                    ; check 51 ASSUMED:LED-CPU-0015: MPR2 was written too ($FA, it held $F9): a TAM that wrote one register only fails one of the two
        tma #$08
        CHECK_LIT 52                    ; check 52 ASSUMED:LED-CPU-0015: MPR3, which the operand does not select, kept $FF
        lda #$F8
        tam #$02                        ; MPR1 = $F8 again
        lda #$F9
        tam #$04                        ; MPR2 = $F9 again

; TAM with the operand $00 is a no-op: no register changes. MPR0 is set first so that a write to it is seen (its power-on value is a policy question and is never read here).
        lda #$F8
        tam #$01                        ; MPR0 = $F8
        lda #$FB
        tam #0                          ; TAM with the operand $00 (R-4: it assembles normally)
        tma #$01
        CHECK_LIT 53                    ; check 53 ASSUMED:LED-CPU-0015: MPR0 still holds $F8, not the $FB of the TAM #0
        tma #$02
        CHECK_LIT 54                    ; check 54 ASSUMED:LED-CPU-0015: MPR1 still holds $F8
        tma #$80
        CHECK_LIT 55                    ; check 55 ASSUMED:LED-CPU-0015: MPR7 still holds the reset value $00 (H8-8)

; TMA with a multi-bit operand returns the highest selected index (D-105): A is loaded with $77 first so that an instruction that leaves A alone shows.
        lda #$77
        .byte $43, $06                  ; RAW:S8-101 ASSUMED:LED-CPU-0015: TMA with the operand $06 (MPR1 $F8 and MPR2 $F9)
        CHECK_LIT 56                    ; check 56 ASSUMED:LED-CPU-0015: the highest selected index is MPR2, so A = $F9 (the lowest would give $F8)
        lda #$77
        .byte $43, $0A                  ; RAW:S8-101 ASSUMED:LED-CPU-0015: TMA with the operand $0A (MPR1 $F8 and MPR3 $FF)
        CHECK_LIT 57                    ; check 57 ASSUMED:LED-CPU-0015: the highest selected index is MPR3, so A = $FF
        lda #$77
        .byte $43, $FF                  ; RAW:S8-101 ASSUMED:LED-CPU-0015: TMA with every bit set
        CHECK_LIT 58                    ; check 58 ASSUMED:LED-CPU-0015: the highest selected index is MPR7, so A = $00 (the reset value; the lowest would give MPR0 = $F8)

; The one-byte latch takes the value of the last TAM or TMA with a nonzero operand and TAM #0 leaves it alone; the guest sees it only through TMA #0 (the parameter tma_zero), so these two rows are observe-only like check 49: each case supplies the byte.
        lda #$77
        .byte $43, $06                  ; RAW:S8-101 ASSUMED:LED-CPU-0015: TMA with the operand $06 returns $F9, so the latch takes $F9
        lda #$11
        tma #0                          ; bus_latch: $F9; a_unchanged: $11
        CHECK_OBSERVE 59
        lda #$C3
        tam #0                          ; the no-op TAM #0 does not put $C3 in the latch
        lda #$22
        tma #0                          ; bus_latch: $F9 (not $C3); a_unchanged: $22
        CHECK_OBSERVE 60
        rts
