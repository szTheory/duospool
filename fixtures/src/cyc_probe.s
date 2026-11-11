; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cyc_probe.s - the cycle probe fixture (plan 03-15; D-98, D-101, D-124, D-125, ER-7, S8-17, S8-20, S8-24, S8-30, S8-70, S8-74, S8-76, S8-78, S8-90, S8-101, H8-25). It is assembled by ca65 in the gate and never committed as a binary.
; The subroutine probe executes ONE fixed instruction sequence at a fixed address: the code is padded to the offset $100 of the code window directly after the library includes (an assert pins it), so the first instruction of probe is at logical $E100 and every later
; address is fixed by the sequence itself. The sequence holds the instructions whose cycle counts the manual states: an ADC with no flag, BEQ not taken and taken, BBR0 not taken and BBR1 taken, SET and an ADC in T mode, SED and an ADC and an SBC in
; decimal mode, TAM and TMA, BRK into a handler (the handler is one INC and the RTI), CSH, an INX at the new speed, CSL and an INX back at the old speed. This file measures nothing: no cycle is counted in the guest and no timer is read.
; The cycle counts are judged by the runner kind cycle_probe from the instruction trace (start stamps of the insn sink), against literals written in the case from the manual pages (cases cyc_probe, cycle_adders_manual). What the guest does check
; is that the sequence completed and took the paths the manual defines (the twelve checks below: branch paths, the T-mode cell, the decimal results, the TAM and TMA round trip, the BRK handler and the RTI return, the speed switch pair and the end marker), so the sweep of the
; literal table applies like for every fixture. Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 7
DSV_CHECKS = 12

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $01                      ; SELFTEST-TARGET
        LIT 2, $00
        LIT 3, $01
        LIT 4, $00
        LIT 5, $21
        LIT 6, $10
        LIT 7, $08
        LIT 8, $F9
        LIT 9, $01
        LIT 10, $01
        LIT 11, $02
        LIT 12, $00

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
MK_BEQN = $2410                 ; incremented by the fall-through path of the BEQ that is not taken
MK_BEQT = $2411                 ; incremented by the skipped path of the BEQ that is taken
MK_BBRN = $2412
MK_BBRT = $2413
MK_TCELL = $2414                ; the zero-page cell $10 after the T-mode ADC, copied
MK_DADC = $2415                 ; A after the decimal ADC
MK_DSBC = $2416                 ; A after the decimal SBC
MK_TMA = $2417                  ; the byte TMA read back
MK_BRK = $2418                  ; how often the BRK handler ran
MK_AFT = $2419                  ; incremented by the instruction after the BRK signature byte
MK_X = $241A                    ; X after the two INX of the speed pair
MK_END = $241B                  ; the last store of the probe
ZPA = $40                       ; the zero-page cell of the bit branches (logical $2040): bit 0 set, bit 1 clear
ZA = $2000+ZPA
ZT = $10                        ; the zero-page cell of the T-mode step: X = ZT selects it (logical $2010)
ZTA = $2000+ZT

        .res $100 - (* - reset), $FF      ; pad the code window so that probe starts at offset $100 of it (logical $E100, hucard.cfg ROMA); the assert below fails the build if the library code ever outgrows that
        .assert * - reset = $100, error, "probe must start at the fixed address $E100"
probe:
        ldx #ZT
        lda #$20
        sta a:ZTA                       ; the T-mode cell holds $20
        lda #$01
        sta a:ZA
        clc
        lda #$01
m_adc:  adc #$01                        ; A = $02, no flag mode: 2 cycles (S8-17)
m_beq_n: beq pn1                        ; Z is clear: not taken, 2 cycles (S8-24)
        inc a:MK_BEQN
pn1:    lda #$00
m_beq_t: beq pt1                        ; Z is set: taken, 2 cycles and 2 more (S8-24)
        inc a:MK_BEQT
pt1:    nop
m_bbr_n: bbr0 <ZPA, pn2                 ; bit 0 of the cell is 1: not taken, 6 cycles (S8-20)
        inc a:MK_BBRN
pn2:    nop
m_bbr_t: bbr1 <ZPA, pt2                 ; bit 1 of the cell is 0: taken, 6 cycles and 2 more (S8-20)
        inc a:MK_BBRT
pt2:    nop
        lda #$02
        clc
m_set:  set                             ; SET: 2 cycles (S8-78)
m_adc_t: adc #$01                       ; T mode: the cell $2000 + X receives $20 + $01, A stays: 2 cycles and 3 more (S8-17)
        lda a:ZTA
        sta a:MK_TCELL
        lda #$09
        clc
m_sed:  sed                             ; SED: 2 cycles (S8-76)
m_adc_d: adc #$01                       ; decimal: $09 + $01 gives $10, 2 cycles and 1 more (S8-17)
        sta a:MK_DADC
m_sbc_d: sbc #$01                       ; decimal: $10 - $01 - not C (C is clear) gives $08, 2 cycles and 1 more (S8-74)
        sta a:MK_DSBC
        cld
        lda #$F9                        ; MPR2 already holds $F9 (startup.inc): the TAM below writes the same value back
m_tam:  tam #$04                        ; TAM: 5 cycles (S8-90)
        lda #$00
m_tma:  tma #$04                        ; TMA: 4 cycles (S8-101)
        sta a:MK_TMA
        lda #<probe_h
        sta a:VEC_IRQ2
        lda #>probe_h
        sta a:VEC_IRQ2+1
m_brk:  brk                             ; BRK: 8 cycles (S8-30); the handler is one INC and the RTI
        nop                             ; the signature byte: BRK pushes its own address plus 2, so this byte is never executed
        inc a:MK_AFT
        lda #<err
        sta a:VEC_IRQ2
        lda #>err
        sta a:VEC_IRQ2+1
        ldx #$00
m_csh:  csh                             ; CSH: 3 cycles at the old (low) speed (H8-25)
m_inx_h: inx                            ; the next instruction runs at the new (high) speed: INX is 2 cycles (S8-49)
m_csl:  csl                             ; CSL: 3 cycles at the old (high) speed (H8-25)
m_inx_l: inx                            ; the next instruction runs at the low speed again: 2 cycles (S8-49)
        stx a:MK_X
        lda #$5A
        sta a:MK_END
        rts

; the BRK handler: one INC, then RTI (7 cycles, S8-70)
probe_h: inc a:MK_BRK
m_rti:  rti

fixture_main:
        stz a:MK_BEQN
        stz a:MK_BEQT
        stz a:MK_BBRN
        stz a:MK_BBRT
        stz a:MK_BRK
        stz a:MK_AFT
        stz a:MK_END
        jsr probe
        lda a:MK_BEQN                   ; check 1 MAN:S8-24: BEQ does not branch when Z is 0, so the fall-through INC ran once
        CHECK_LIT 1
        lda a:MK_BEQT                   ; check 2 MAN:S8-24: BEQ branches when Z is 1, so the INC behind it did not run
        CHECK_LIT 2
        lda a:MK_BBRN                   ; check 3 MAN:S8-20: BBR0 does not branch when bit 0 is 1, so the fall-through INC ran once
        CHECK_LIT 3
        lda a:MK_BBRT                   ; check 4 MAN:S8-20: BBR1 branches when bit 1 is 0, so the INC behind it did not run
        CHECK_LIT 4
        lda a:MK_TCELL                  ; check 5 MAN:S8-17: with T set ADC adds the immediate to the zero-page cell X: $20 + $01 = $21
        CHECK_LIT 5
        lda a:MK_DADC                   ; check 6 MAN:S8-17: decimal mode ADC: $09 + $01 = $10
        CHECK_LIT 6
        lda a:MK_DSBC                   ; check 7 MAN:S8-74: decimal mode SBC: $10 - $01 - not C (C clear) = $08
        CHECK_LIT 7
        lda a:MK_TMA                    ; check 8 MAN:S8-101: TMA reads the MPR that TAM wrote: MPR2 = $F9
        CHECK_LIT 8
        lda a:MK_BRK                    ; check 9 MAN:S8-30: BRK jumps through the IRQ2 and BRK vector: the handler ran once
        CHECK_LIT 9
        lda a:MK_AFT                    ; check 10 MAN:S8-70: RTI returns to the pushed address (BRK plus 2), so the instruction after the signature byte ran once
        CHECK_LIT 10
        lda a:MK_X                      ; check 11 MAN:H8-25: CSH and CSL are one byte each: X = $00 and an INX after each leave X = $02
        CHECK_LIT 11
        lda #$5A                        ; check 12 GUEST:probe-end-marker: the probe stored the byte $5A in its last step; the model is that byte, the cell is cleared before the probe
        sta a:GEXP
        lda a:MK_END
        CHECK_GUEST 12
        rts
