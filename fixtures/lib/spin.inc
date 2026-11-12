; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spin.inc - the spin-counter helpers of the authored fixtures (plan 04-17; D-174, ER-3, ER-10, F-19, F-40, plan-check ruling 7). Macros and equates only: including this file emits no byte, so no image that does not expand a macro changes. It is created by the plan that owns the guest interrupt measurements and is never edited by a later plan.
; Included after startup.inc, verdict.inc, check.inc and vdc.inc. No HuC, PCEAS, emulator or cc65 library code.
;
; The loop and its cost (plan-check round 2, W3; every cycle count is the column of tests/oracle/huc6280-opcodes.tsv, the H8-25 map, none is taken from DuoSpool output):
;   spin:   inc a      opcode 1A, 2 cycles
;           bra spin   opcode 80, 4 cycles
; One iteration is P = 6 CPU cycles, 18 master clocks at the high speed (3 master clocks per cycle after CSH; the cycle length is the community figure of ER-3, ASSUMED LED-CPU-0009). The loop has no data-dependent branch. A branch-free 24-bit increment in the loop itself would cost 24 cycles (clc 2, adc #1 2, sax 3, adc #0 2, sax 3, say 3, adc #0 2, say 3, bra 4)
; and leave inconsistent register states between its instructions, so it is not used: A is the only counter in the loop, it is an 8-bit value that is consistent at every instruction boundary, and the handlers widen it (D-174's 24-bit clc/adc add, moved from the loop into the handlers, A10 of 04-VALIDATION.md).
; A foreground that touches only A leaves X and Y free: the handlers keep their log index in Y across interrupts.
;
; The handler cost rule. Every handler adds K to A before its RTI, where 6 * K is the exact number of CPU cycles from the end of the interrupted instruction to the end of the RTI:
;   interrupt entry   SPIN_ENTRY  8 cycles (interrupt_entry_cycles of constants.def, ASSUMED LED-CPU-0009)
;   vector trampoline SPIN_JMPIND 7 cycles (the jmp (VEC_IRQ1) of startup.inc: JMP ABSIND, opcode 6C)
;   body              the sum of the table cycles of its instructions, plus one penalty cycle for every access to the VDC window ($0000 to $0003, including ST0, ST1 and ST2) and the VCE window (vdc_vce_penalty_cycles = 1, ASSUMED LED-CPU-0017), padded with 2-cycle NOPs and one 3-cycle exchange (SXY) to a multiple of 6
;   RTI               SPIN_RTI    7 cycles (opcode 40)
; A total that is a multiple of 6 leaves the loop at the phase where it was interrupted, so the next sample is again an exact number of iterations away. There is no 1-cycle instruction, so an odd total is made even with one SXY. Every handler is fixed-cost on its normal path (no data-dependent branch).
; With such handlers, the 8-bit samples s(n) taken at consecutive handler entries give the elapsed foreground-equivalent iterations as (s(n) - s(n-1)) mod 256, provided two samples are less than 256 iterations (4,608 master clocks, 3.37 lines) apart; a 24-bit sum of those deltas is the elapsed time, and a sum of 6 * g cycles is within 4 cycles of the true boundary-to-boundary time (a boundary
; after inc a is at 2 cycles into an iteration and one after bra at 0, so the difference of two boundaries is 6 * g + {0, +4, -4}). The interrupt is taken at the first instruction end whose last cycle is at or after the event (D-111), so the event-to-boundary wait is at least 1 and under 5 cycles.
;
; The status read sits 9 cycles after the first instruction of every handler that the guest measures (sta (zp),y 7 + tax 2, or sta zp 4 + tax 2 + sxy 3; the count of fixtures/src/vdc_irq.s, corrected from 7 by the info item I-01 of the gate review), so the read stamps of the dev log of two handlers differ by exactly the difference of their boundaries (the agreement kind of 04-17).

SPIN_ENTRY = 8                  ; cycles of the interrupt entry (ASSUMED LED-CPU-0009)
SPIN_JMPIND = 7                 ; cycles of jmp (VEC_IRQ1)
SPIN_RTI = 7                    ; cycles of RTI
SPIN_FIXED = SPIN_ENTRY + SPIN_JMPIND + SPIN_RTI
SPIN_P = 6                      ; cycles of one iteration of inc a, bra
SPIN_CLK = 18                   ; master clocks of one iteration at the high speed

; SPIN_LOOP: the foreground loop; it never ends by itself (the last handler redirects its return address)
        .macro SPIN_LOOP
        .local sp
sp:     inc a
        bra sp
        .endmacro

; ADD24 lo: A is added to the 24-bit value at the zero-page cells lo, lo+1, lo+2 (clc/adc, no data-dependent time is needed here: this runs after the measurement)
        .macro ADD24 lo
        .local nc
        clc
        adc z:lo
        sta z:lo
        bcc nc
        inc z:lo+1
        bne nc
        inc z:lo+2
nc:
        .endmacro
