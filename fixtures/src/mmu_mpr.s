; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; mmu_mpr.s - the memory map and the mapping registers seen from the guest (plan 03-16; D-102, D-103, D-104, D-124, D-125, D-127; H8-7, H8-8, H8-9, H8-24, S8-90, S8-101). It is assembled by ca65 in the gate and never committed as a binary.
; The guest maps a bank into the logical slots $6000 (MPR3) and $8000 (MPR4) and reads or writes through them, so every check is one translate and one class of the bus: unmapped banks, the 8 KiB work RAM mirrored four times,
; HuCard banks whose bytes this file writes itself (segment BANKS: the first byte of file bank k is k and the last byte is k xor $A5, so a bank, an offset and a mirror can each be told apart from the $FF of an unmapped read), and, from
; check 22 on, the windows of the hardware page and the bus latch (io_buffer). ROMBANKS is the image size in banks (32 gives 256 KiB); the checks that name a mirror hold for the 256 KiB image only and plan 03-20 adds the 384 KiB and
; 1 MiB literals by conditional assembly on ROMBANKS. Every check reads cells the fixture has written (D-127): RAM cells are written first, HuCard bytes are the BANKS data, and io_buffer is set by a write to the I/O window before every read that depends on it.
; The classes: MAN rows follow a page (H8-7: the MPR byte supplies the physical address bits A13 to A20; S8-101: TMA reads what TAM stored), ASSUMED rows name the project's modelling assumption (the ledger entries are written by plan 03-20).
; No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 8
DSV_CHECKS = 48

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

SL3 = $08                       ; the TAM and TMA operand of MPR3 (logical $6000-$7FFF)
SL4 = $10                       ; MPR4 (logical $8000-$9FFF)

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
        LIT 18, $78
        LIT 19, $1F
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
        MAP SL3, $20
        lda a:$6000                     ; check 18 ASSUMED:LED-LOAD-0003: in a 256 KiB image bank $20 is a mirror of bank 0: its first byte is the SEI opcode $78 of the reset code
        CHECK_LIT 18
        MAP SL3, $3F
        lda a:$6000                     ; check 19 ASSUMED:LED-LOAD-0003: bank $3F mirrors bank $1F of the 256 KiB image
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
        rts
