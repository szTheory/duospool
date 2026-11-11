; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; tim_timer.s - the timer and the timer interrupt seen from the guest (plan 03-21; D-109, D-110, D-111, D-124, D-125, ER-3, ER-5; H8-12, H8-13, H8-14, H8-15, H8-16, H8-18, H8-20, H8-21, H8-22, H8-24). It is assembled by ca65 in the gate and never committed as a binary.
; The timer window is logical $6C00 (reload, write) and $6C01 (control, write) with MPR3 = $FF, the counter is read at either address, and the interrupt registers are $7402 (disable, bit 2 TIQD) and $7403 (request, bit 2 TIQ, a write clears it).
; This first part is the tracer: a timer started at reload 0 delivers a TIQ interrupt through the TIMER vector ($FFFA, through the RAM slot of startup.inc), with I clear and D set in the main program; the handler sees I set and D clear, reads the request register
; with TIQ set, stops the timer, acknowledges with a write to $1403, reads the register again with TIQ clear, and returns with RTI. The entry pushes the program counter high byte, the low byte and the status byte with B = 0 (H8-12).
; What the checks are (ER-3, ER-4): the MAN rows follow a manual page for the register and interrupt facts; the GUEST rows compare cells the guest itself wrote (the interrupted loop address, a marker). Nothing here proves a CPU speed or a timer rate: the later rows that read the
; counter after a counted number of instructions depend on the community-reported CPU cycle length (12 master clocks at the low speed) and are E2 self-consistency, labelled GUEST:timer-e2 or ASSUMED; the one claim of a rate is the C-level case timer_pure_function.
; Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 10
DSV_CHECKS = 10

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $01                      ; SELFTEST-TARGET
        LIT 2, $08
        LIT 3, $04
        LIT 4, $00
        LIT 5, $04
        LIT 6, $00
        LIT 7, $00                      ; GUEST: the table entry is an XOR mask of $00
        LIT 8, $00                      ; GUEST
        LIT 9, $00                      ; GUEST
        LIT 10, $01

SL3 = $08                       ; the TAM operand of MPR3 (logical $6000-$7FFF)
TIM_RELOAD = $6C00              ; the timer window with MPR3 = $FF: the reload register (write) and the counter (read)
TIM_CTRL = $6C01                ; the control register (write, bit 0 starts and stops) and the counter (read)
IRQ_DIS = $7402                 ; $1402: the disable register
IRQ_REQ = $7403                 ; $1403: the request register
IOWIN = $7000                   ; the I/O window: a write sets io_buffer

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
HCOUNT = $2410                  ; how often the handler ran
H_P = $2411                     ; the status byte the entry pushed
H_PCL = $2412                   ; the return address low byte the entry pushed
H_PCH = $2413                   ; and the high byte
H_NOW = $2414                   ; the status byte as the handler sees it (read through PHP)
H_REQ1 = $2415                  ; $1403 at the start of the handler
H_REQ2 = $2416                  ; $1403 after the acknowledge
H_DONE = $2417                  ; a marker stored by the main program after the RTI returned

; DELAY k: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2): k from 1 to 255
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

fixture_main:
        lda #$FF
        tam #SL3                        ; MPR3 = $FF: the hardware page at logical $6000
        stz a:HCOUNT
        stz a:H_P
        stz a:H_PCL
        stz a:H_PCH
        stz a:H_NOW
        stz a:H_REQ1
        stz a:H_REQ2
        stz a:H_DONE
        lda #<timer_h
        sta a:VEC_TIMER                 ; the TIMER vector jumps through this slot (startup.inc)
        lda #>timer_h
        sta a:VEC_TIMER+1

; ---- the tracer: reload 0, start, wait for the interrupt with I clear and D set ----
        stz a:IRQ_DIS                   ; TIQD = 0: the timer interrupt is not disabled (H8-16)
        lda #$00
        sta a:TIM_RELOAD                ; reload 0: a borrow at every prescaler step
        sed                             ; D = 1 in the main program: the entry pushes it as it is and clears it for the handler (H8-12)
        cli                             ; I = 0
        lda #$01
        sta a:TIM_CTRL                  ; start: the counter takes the reload value (H8-22)
        ldx #$00
        ldy #$20
        .align 16, $EA                  ; the loop below is 11 bytes: starting on a 16-byte boundary keeps it inside one page (the padding is NOP, $EA, because it is executed: the default fill of this target is $FF, a branch opcode)
wait:   lda a:HCOUNT                    ; the loop ends as soon as the handler has run, or after a bounded number of rounds (a core with no timer interrupt must fail a check, not hang)
        bne wait_end
        dex
        bne wait
        dey
        bne wait
wait_end:
        .assert >wait = >wait_end, error, "the wait loop must not cross a page: check 7 compares the pushed high byte with its page"
        sei
        cld
        lda #$5A
        sta a:H_DONE                    ; the main program ran on after the RTI
        lda a:HCOUNT                    ; check 1 MAN:H8-21: a borrow with I and TIQD clear requests the CPU: the handler ran once (the handler stops the timer and acknowledges, so there is no second entry)
        CHECK_LIT 1
        lda a:H_P                       ; check 2 MAN:H8-12: the entry pushed the status byte with B = 0, and the byte is the interrupted one: D = 1 and I = 0 as the main program had them (mask $1C: B D I)
        and #$1C
        CHECK_LIT 2
        lda a:H_NOW
        and #$04                        ; check 3 MAN:H8-12: I is set inside the handler
        CHECK_LIT 3
        lda a:H_NOW
        and #$08                        ; check 4 MAN:H8-12: D is reset inside the handler
        CHECK_LIT 4
        lda a:H_REQ1
        and #$04                        ; check 5 MAN:H8-15: bit 2 of the request register reads 1 while the timer interrupt is requested
        CHECK_LIT 5
        lda a:H_REQ2
        and #$04                        ; check 6 MAN:H8-14: a write to the request register resets TIQ
        CHECK_LIT 6
        lda #>wait
        sta a:GEXP
        lda a:H_PCH                     ; check 7 GUEST:interrupted-loop: the pushed return address high byte is the page of the loop the interrupt came from (the entry pushed PCH first, H8-12)
        CHECK_GUEST 7
        sec
        lda a:H_PCL
        sbc #<wait
        cmp #(wait_end - wait + 1)      ; the pushed low byte minus the loop start is inside the loop (an instruction boundary from the loop top to the byte after its last instruction)
        lda #$00
        bcs @out
        lda #$01
@out:   sta a:SCRATCH                   ; 1 when the pushed low byte lies inside the loop
        lda #$01
        sta a:GEXP                      ; the model: the flag is 1
        lda a:SCRATCH
        CHECK_GUEST 8                   ; check 8 GUEST:interrupted-loop: the pushed return address low byte lies inside the wait loop
        lda #$5A
        sta a:GEXP
        lda a:H_DONE
        CHECK_GUEST 9                   ; check 9 GUEST:marker: the RTI returned to the main program, which then stored its marker
        DELAY 255
        lda a:HCOUNT                    ; check 10 MAN:H8-20: the handler stopped the timer (control bit 0 = 0) and acknowledged: six prescaler steps later there was no further interrupt
        CHECK_LIT 10
        rts

; the TIMER vector handler: save A and X, copy the three bytes the entry pushed, read the status as the handler has it, read the request register, stop the timer, acknowledge, read again, count, restore, RTI
timer_h:
        pha
        phx
        tsx                             ; X = S after the entry's three pushes and these two: the pushed status byte is at $2100 + X + 3, the low byte at + 4 and the high byte at + 5
        lda a:$2103,x
        sta a:H_P
        lda a:$2104,x
        sta a:H_PCL
        lda a:$2105,x
        sta a:H_PCH
        php
        pla
        sta a:H_NOW                     ; PHP pushes the flags as they are (B reads 1 and T reads 0 on the stack, H8-5): I and D show here
        lda a:IRQ_REQ
        sta a:H_REQ1
        lda #$00
        sta a:TIM_CTRL                  ; stop the timer (control bit 0 = 0, H8-20)
        sta a:IRQ_REQ                   ; any write to $1403 clears TIQ (H8-14)
        lda a:IRQ_REQ
        sta a:H_REQ2
        inc a:HCOUNT
        plx
        pla
        rti
