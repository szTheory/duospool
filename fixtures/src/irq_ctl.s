; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; irq_ctl.s - the interrupt controller seen from the guest (plan 03-22; D-110, D-111, D-112, D-124, D-125, ER-5; H8-12, H8-13, H8-14, H8-15, H8-16, H8-17, H8-18, H8-21, H8-24). It is assembled by ca65 in the gate and never committed as a binary.
; The interrupt registers are $7402 (disable, bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD) and $7403 (request, bit 0 the IRQ2 level, bit 1 the IRQ1 level, bit 2 TIQ, a write clears TIQ only) with MPR3 = $FF; the timer window is $6C00 and $6C01.
; IRQ1 and IRQ2 are driven by nothing in the machine yet: the case declares a TEST stimulus (the case key irq_stim, a list of events AT,LINE,LEVEL in master clocks) and this file is written against it. The stimulus is a pure function of machine time, it is
; not evidence about the VDC or any hardware source (ER-5, D-112, ASSUMED LED-IRQ-0008), and the fixture is only meaningful under the case that declares it: every scenario first polls the request register until the line it needs reads high, so the
; scenarios do not depend on how long the code before them took, and every wait is bounded (a machine with no stimulus fails a check, it does not hang).
; The stimulus the cases declare (windows in master clocks, the line is high from the first time to the second): window A IRQ1 from 1200000 to 1500000; B IRQ2 from 2200000 to 2500000; C IRQ1 and IRQ2 from 3200000 to 3500000; D IRQ1 from 4200000 to 4500000.
; Window A (checks 5 to 15): the IRQ1 level reads in bit 1, I and IRQ1D each mask the delivery, the unmasking write is taken at once, the entry goes through $FFF8, pushes B = 0 and the interrupted D and I, the handler sees I set and D clear and bit 1 of $1403 set,
; a write to $1403 does not clear a level, the level ends with its window. Checks 1 to 4 read the register state before any event. Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 11
DSV_CHECKS = 15

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00
        LIT 2, $00
        LIT 3, $05
        LIT 4, $07
        LIT 5, $02
        LIT 6, $00
        LIT 7, $00
        LIT 8, $02
        LIT 9, $01
        LIT 10, $00
        LIT 11, $08
        LIT 12, $04
        LIT 13, $02
        LIT 14, $02
        LIT 15, $00

SL3 = $08                       ; the TAM operand of MPR3 (logical $6000-$7FFF)
TIM_RELOAD = $6C00              ; the timer window with MPR3 = $FF: the reload register (write) and the counter (read)
TIM_CTRL = $6C01                ; the control register (write, bit 0 starts and stops)
IRQ_DIS = $7402                 ; $1402: the disable register
IRQ_REQ = $7403                 ; $1403: the request register

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
H1_COUNT = $2410                ; how often the IRQ1 handler ran
H2_COUNT = $2411                ; how often the IRQ2 and BRK handler ran
HT_COUNT = $2412                ; how often the timer handler ran
LOGN = $2413                    ; the number of entries in the order log
H_P1 = $2414                    ; the status byte the IRQ1 entry pushed
H_NOW1 = $2415                  ; the status byte as the IRQ1 handler sees it (read through PHP)
H_REQ1 = $2416                  ; $1403 at the start of the IRQ1 handler
H_A = $2417                     ; A at the entry of the IRQ1 handler
LOG = $2418                     ; the order log: one byte per handler run (4 bytes)
PBLOG = $241C                   ; the status bytes pushed by the IRQ2 and BRK entries, in order (2 bytes)

; DELAY k: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2): k from 1 to 255
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

; WAIT_SET mask: poll the request register until every bit of mask reads 1, bounded; WAIT_CLEAR mask: until every bit of mask reads 0, bounded
        .macro WAIT_SET mask
        lda #(mask)
        jsr wait_set
        .endmacro

        .macro WAIT_CLEAR mask
        lda #(mask)
        jsr wait_clear
        .endmacro

fixture_main:
        lda #$FF
        tam #SL3                        ; MPR3 = $FF: the hardware page at logical $6000
        lda #<irq1_h
        sta a:VEC_IRQ1                  ; the IRQ1 vector jumps through this slot (startup.inc)
        lda #>irq1_h
        sta a:VEC_IRQ1+1
        jsr reset_scen

; ---- the register state before any declared event (H8-15, H8-16, H8-18) ----
        lda a:IRQ_REQ
        and #$03
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-IRQ-0008: no line is asserted before the first event of the declared stimulus: bits 0 and 1 of the request register read 0
        lda a:IRQ_DIS
        and #$07
        CHECK_LIT 2                     ; check 2 MAN:H8-16: the disable register is 0 after the reset
        lda #$05
        sta a:IRQ_DIS
        lda a:IRQ_DIS
        and #$07
        CHECK_LIT 3                     ; check 3 MAN:H8-16: the disable register reads back what was written: IRQ2D and TIQD, not IRQ1D
        lda #$FF
        sta a:IRQ_DIS
        lda a:IRQ_DIS
        and #$07
        CHECK_LIT 4                     ; check 4 MAN:H8-16: three bits are stored: $FF reads back as 7 in the three register bits

; ---- window A: IRQ1 only ----
        jsr reset_scen
        WAIT_SET $02                    ; the IRQ1 level of the declared stimulus
        lda a:IRQ_REQ
        and #$07
        CHECK_LIT 5                     ; check 5 MAN:H8-15: bit 1 of the request register is the IRQ1 level; IRQ2 (bit 0) and TIQ (bit 2) read 0
        DELAY 30
        lda a:H1_COUNT
        CHECK_LIT 6                     ; check 6 MAN:H8-13: I is set (nothing cleared it), so the asserted IRQ1 is not taken
        lda #$02
        sta a:IRQ_DIS                   ; IRQ1D = 1
        cli                             ; I = 0: only IRQ1D masks
        DELAY 30
        lda a:H1_COUNT
        CHECK_LIT 7                     ; check 7 MAN:H8-16: IRQ1D = 1 disables the interrupt request at the IRQ1 pin: the handler did not run with I clear
        lda a:IRQ_REQ
        and #$03
        CHECK_LIT 8                     ; check 8 MAN:H8-15: the request register shows the level of the pin whatever the disable bit says: bit 1 reads 1
        sed                             ; D = 1 in the main program: the entry pushes it as it is and clears it for the handler (H8-12)
        lda #$00
        sta a:IRQ_DIS                   ; IRQ1D = 0: the write is in the last cycle of the instruction, so it takes effect before the sample (D-111): the entry follows this instruction
        nop
        DELAY 4
        cld
        lda a:H1_COUNT
        CHECK_LIT 9                     ; check 9 MAN:H8-13: the IRQ1 interrupt was taken once (the handler masks the line again, so there is no second entry)
        lda a:H2_COUNT
        CHECK_LIT 10                    ; check 10 MAN:H8-13: through the IRQ1 vector $FFF8: the IRQ2 and BRK vector $FFF6 was not used
        lda a:H_P1
        and #$1C
        CHECK_LIT 11                    ; check 11 MAN:H8-12: the entry pushed the status byte with B = 0, D = 1 and I = 0 as the main program had them (mask $1C: B D I)
        lda a:H_NOW1
        and #$0C
        CHECK_LIT 12                    ; check 12 MAN:H8-12: I is set and D is reset inside the handler (mask $0C: D I)
        lda a:H_REQ1
        and #$03
        CHECK_LIT 13                    ; check 13 MAN:H8-15: bit 1 of the request register reads 1 inside the handler, bit 0 reads 0
        sta a:IRQ_REQ                   ; a write to the request register resets TIQ only
        lda a:IRQ_REQ
        and #$02
        CHECK_LIT 14                    ; check 14 MAN:H8-15: the IRQ1 level is an input and is not reset by the write: bit 1 still reads 1 inside the window
        WAIT_CLEAR $03                  ; the level ends with its window
        lda a:IRQ_REQ
        and #$03
        CHECK_LIT 15                    ; check 15 ASSUMED:LED-IRQ-0008: after the second event of the declared stimulus both lines read 0 again (the level is a pure function of time)
        rts

; reset_scen: a known state before a scenario: I set, D clear, the timer stopped, the disable register and TIQ clear, every counter and logged cell zero
reset_scen:
        sei
        cld
        stz a:TIM_CTRL
        stz a:IRQ_DIS
        stz a:IRQ_REQ
        stz a:H1_COUNT
        stz a:H2_COUNT
        stz a:HT_COUNT
        stz a:LOGN
        stz a:H_P1
        stz a:H_NOW1
        stz a:H_REQ1
        stz a:H_A
        ldx #$05
rs_log: stz a:LOG,x
        dex
        bpl rs_log
        rts

; wait_set: A = mask: poll the request register until (value AND mask) = mask, at most 12288 rounds of 23 cycles; wait_clear: until (value AND mask) = 0 (a bounded wait never hangs, so a machine with no stimulus fails a check)
wait_set:
        sta a:SCRATCH2
        ldy #$30
ws_o:   ldx #$00
ws_i:   lda a:IRQ_REQ
        and a:SCRATCH2
        cmp a:SCRATCH2
        beq ws_end
        dex
        bne ws_i
        dey
        bne ws_o
ws_end: rts

wait_clear:
        sta a:SCRATCH2
        ldy #$30
wc_o:   ldx #$00
wc_i:   lda a:IRQ_REQ
        and a:SCRATCH2
        beq wc_end
        dex
        bne wc_i
        dey
        bne wc_o
wc_end: rts

; the IRQ1 vector handler: store A as the entry left it, save A and X, copy the status byte the entry pushed, read the status as the handler has it and the request register, mask IRQ1 (the level stays high, so this ends the storm), log and count, RTI
irq1_h:
        sta a:H_A
        pha
        phx
        tsx                             ; X = S after the entry's three pushes and these two: the pushed status byte is at $2100 + X + 3
        lda a:$2103,x
        sta a:H_P1
        php
        pla
        sta a:H_NOW1                    ; PHP pushes the flags as they are: I and D show here
        lda a:IRQ_REQ
        sta a:H_REQ1
        lda a:IRQ_DIS
        ora #$02
        sta a:IRQ_DIS
        ldx a:LOGN
        lda #$01
        sta a:LOG,x
        inc a:LOGN
        inc a:H1_COUNT
        plx
        pla
        rti
