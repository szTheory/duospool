; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; ds_proto.s - the first authored fixture: the tracer of context wave W1 (plan 03-01). It is assembled by ca65 in the gate and never
; committed as a binary. It has two checks and writes a DSV1 verdict block into work RAM, which the runner judges from raw RAM.
; Project-owned reset code only: no HuC, PCEAS, emulator or cc65 library code.
;
;   check 1  MAN:S8-17   LDA #$7F, CLC, ADC #$01 gives $80 with V set (binary add, N V Z C flags, software manual page S8-17)
;   check 2  GUEST:sum   a 4-byte ROM table summed by two different loops: loop A adds the table in the accumulator, loop B adds a table
;                        of the 8-bit complements in a RAM cell; the two sums must cancel to $00 modulo 256 (in-guest model, no literal)
;
; DSV1 layout (the judge in tests/conformance/spinkinds.c reads the same bytes; one block of 256 bytes at work-RAM offset $1F00):
;   +00 "DSV1"   +04 state ($01 running, $A5 done)   +05 complement of the state   +06 version 1   +07 n_checks   +08 fixture id
;   +09 pass   +0A fail   +0B notrun   +0C progress   +0D error flags   +10..+6F result per check ($00 not run, $50 pass, $46 fail)
;   +80..+DF observed byte per check   +E0 +E1 16-bit little-endian sum of +06..+DF   +FE +FF trailer $5A $A5   every other byte zero.
; The sum is written before the state flips to done; the guest then executes SEI and branches to itself.
; MPR1 = $F8 maps work RAM at logical $2000 (zero page and stack); MPR2 = $F9 maps it again at $4000, so the verdict block sits at
; logical $5F00 and no later remap of MPR1 can move it. Every vector points at a handler that sets an error flag and spins.
        .setcpu "huc6280"

VERDICT = $5F00                 ; logical address of the block while MPR2 = $F9
SCRATCH = $2400                 ; one scratch cell in work RAM, outside the verdict block

        .macro TALLY
        lda a:VERDICT+$0B       ; notrun - 1, as an add of $FF
        clc
        adc #$FF
        sta a:VERDICT+$0B
        lda a:VERDICT+$0C       ; progress + 1
        clc
        adc #$01
        sta a:VERDICT+$0C
        .endmacro

        .segment "CODE"

reset:
        sei
        cld
        ldx #$FF
        txs
        lda #$F8                ; MPR1 = $F8 (work RAM at logical $2000)
        tam #$02
        lda #$F9                ; MPR2 = $F9 (the verdict slot at logical $4000)
        tam #$04

        ldx #$00                ; zero the 256-byte block (X wraps to $00 after 256 stores)
zero:   stz a:VERDICT,x
        inx
        bne zero

        lda #$44                ; magic "DSV1"
        sta a:VERDICT+$00
        lda #$53
        sta a:VERDICT+$01
        lda #$56
        sta a:VERDICT+$02
        lda #$31
        sta a:VERDICT+$03
        lda #$01                ; version 1, and the same value for the fixture id and the RUNNING state
        sta a:VERDICT+$06
        sta a:VERDICT+$08
        lda #$02                ; n_checks = 2, and notrun starts at 2
        sta a:VERDICT+$07
        sta a:VERDICT+$0B
        lda #$5A                ; trailer
        sta a:VERDICT+$FE
        lda #$A5
        sta a:VERDICT+$FF
        lda #$FE                ; state RUNNING: $01 and its complement $FE
        sta a:VERDICT+$05
        lda #$01
        sta a:VERDICT+$04

; ---- check 1: MAN:S8-17 ----
        lda #$7F
        clc
        adc #$01
        sta a:VERDICT+$80       ; observed byte of check 1
        cmp #$80                ; SELFTEST-TARGET
        bne c1_fail
        bvs c1_pass             ; V must be set as well
c1_fail:
        lda #$46
        sta a:VERDICT+$10
        lda a:VERDICT+$0A
        clc
        adc #$01
        sta a:VERDICT+$0A
        bra c1_done
c1_pass:
        lda #$50
        sta a:VERDICT+$10
        lda a:VERDICT+$09
        clc
        adc #$01
        sta a:VERDICT+$09
c1_done:
        TALLY

; ---- check 2: GUEST:sum ----
        ldx #$FC                ; loop A: X runs $FC to $FF, the operand base is the table minus $FC
        lda #$00
la:     clc
        adc a:tbl-$FC,x
        inx
        bne la
        sta a:VERDICT+$81       ; observed byte of check 2: the sum of loop A
        lda #$00                ; loop B: the same walk, but the running sum lives in a RAM cell and the table holds complements
        sta a:SCRATCH
        ldx #$FC
lb:     clc
        lda a:SCRATCH
        adc a:tbln-$FC,x
        sta a:SCRATCH
        inx
        bne lb
        ldx #$00                ; the complements and the sums must cancel to $00
        clc
        adc a:VERDICT+$81,x
        cmp #$00
        beq c2_pass
c2_fail:
        lda #$46
        sta a:VERDICT+$11
        lda a:VERDICT+$0A
        clc
        adc #$01
        sta a:VERDICT+$0A
        bra c2_done
c2_pass:
        lda #$50
        sta a:VERDICT+$11
        lda a:VERDICT+$09
        clc
        adc #$01
        sta a:VERDICT+$09
c2_done:
        TALLY

; ---- checksum of +06..+DF (218 bytes, X runs $26 to $FF), then the state flips to done last ----
        ldx #$26
sum:    lda a:VERDICT+$E0
        clc
        adc a:VERDICT+$06-$26,x
        sta a:VERDICT+$E0
        lda a:VERDICT+$E1
        adc #$00
        sta a:VERDICT+$E1
        inx
        bne sum
        lda #$5A                ; complement first, then the state
        sta a:VERDICT+$05
        lda #$A5
        sta a:VERDICT+$04
        sei
spin:   bra spin

err:    lda #$01                ; every vector: set an error flag and spin
        sta a:VERDICT+$0D
errsp:  bra errsp

tbl:    .byte $11, $22, $33, $44
tbln:   .byte $EF, $DE, $CD, $BC

        .segment "VECTORS"
        .word err               ; IRQ2 and BRK
        .word err               ; IRQ1
        .word err               ; TIMER
        .word err               ; NMI
        .word reset             ; RESET
