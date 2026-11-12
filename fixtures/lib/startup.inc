; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; startup.inc - project-owned reset code and vectors of every authored fixture (plan 03-05; D-123, D-124). No HuC, PCEAS, emulator or cc65
; library code. A fixture defines DSV_ID and DSV_CHECKS, includes startup.inc, verdict.inc and check.inc in that order, and provides a
; subroutine fixture_main that ends with RTS. The reset code maps work RAM, zeroes the verdict block, installs the RAM vector slots, starts the DSV1 block, calls
; fixture_main and seals the block; it never returns. Every operand carries an explicit size hint (a:), so the lint rule of D-125 holds.
;
; Memory model (Pitfall 19):
;   MPR1 = $F8  work RAM at logical $2000: zero page $2000-$20FF, stack $2100-$21FF, the RAM vector slots and the scratch cells below.
;   MPR2 = $F9  work RAM again at logical $4000, so the verdict block (work-RAM offset $1F00) is at logical $5F00 and no later remap of MPR1 moves it.
; Vectors: IRQ2 and BRK, IRQ1, TIMER and NMI each jump through a word in RAM (the vector slots), which the reset code fills with the
; error-flag handler. A later fixture stores its own handler address into a slot and never edits this file.
        .setcpu "huc6280"

VERDICT = $5F00                 ; logical address of the DSV1 block while MPR2 = $F9
VEC_IRQ2 = $2200                ; the RAM vector slots (two bytes each, little endian)
VEC_IRQ1 = $2202
VEC_TIMER = $2204
VEC_NMI = $2206
SCRATCH = $2400                 ; scratch cells of work RAM, outside the verdict block and the stack
SCRATCH2 = $2401
GEXP = $2402                    ; the guest-computed expected value of a GUEST-class check (check.inc)

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
        ldx #$00                ; zero the 256-byte verdict block (X wraps to $00 after 256 stores); the first thirteen instructions of this reset code are
rz:     stz a:VERDICT,x         ; the ones that cases of plan 03-02 (run_until_abs) take their instruction ends from, so this loop stays directly after the two TAMs
        inx
        bne rz
        ldx #$F8                ; the eight bytes of the four vector slots: X runs $F8 to $FF
sl_loop: lda a:slot_init-$F8,x
        sta a:VEC_IRQ2-$F8,x
        inx
        bne sl_loop
        jsr dsv_init
        jsr fixture_main
        jsr dsv_finish          ; never returns
err:    lda #$01                ; the error-flag handler: set the flag in the block and spin
        sta a:VERDICT+$0D
errsp:  bra errsp

slot_init:
        .word err, err, err, err

v_irq2: jmp (VEC_IRQ2)
v_irq1: jmp (VEC_IRQ1)
v_timer: jmp (VEC_TIMER)
v_nmi:  jmp (VEC_NMI)

        .segment "VECTORS"
        .word v_irq2            ; IRQ2 and BRK
        .word v_irq1            ; IRQ1
        .word v_timer           ; TIMER
        .word v_nmi             ; NMI
        .word reset             ; RESET

        .segment "CODE"
