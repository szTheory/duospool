; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; tim_timer.s - the timer and the timer interrupt seen from the guest (plan 03-21; D-109, D-110, D-111, D-124, D-125, ER-3, ER-5; H8-12, H8-13, H8-14, H8-15, H8-16, H8-18, H8-20, H8-21, H8-22, H8-24). It is assembled by ca65 in the gate and never committed as a binary.
; The timer window is logical $6C00 (reload, write) and $6C01 (control, write) with MPR3 = $FF, the counter is read at either address, and the interrupt registers are $7402 (disable, bit 2 TIQD) and $7403 (request, bit 2 TIQ, a write clears it).
; Part 1 is the tracer: a timer started at reload 0 delivers a TIQ interrupt through the TIMER vector ($FFFA, through the RAM slot of startup.inc), with I clear and D set in the main program; the handler sees I set and D clear, reads the request register
; with TIQ set, stops the timer, acknowledges with a write to $1403, reads the register again with TIQ clear, and returns with RTI. The entry pushes the program counter high byte, the low byte and the status byte with B = 0 (H8-12).
; The reset state (checks 11 to 13) is read first, before anything touches the timer. Part 2 (checks 14 to 27) reads the counter after counted delays: the expected value is the reload minus the elapsed CPU cycles over CYC_STEP, written by the assembler from the instruction counts of the file
; (the macro DELAY k costs 6 * k cycles, LDY # 2, STA abs 5, LDA abs 5; a data access is in the last cycle of its instruction, an assumption of this project, LED-CPU-0004). CYC_STEP = 256 is the prescaler step 3072 master clocks over the community-reported CPU cycle of 12 master clocks at the low speed:
; every check that depends on it is E2 self-consistency (ER-3). Part 3 (checks 28 to 33) takes the interrupt with the request masked by TIQD and by I, and sees what the entry pushed after CLI and SET.
; What the checks are (ER-3, ER-4): the MAN rows follow a manual page for the register and interrupt facts; the GUEST rows compare cells the guest itself wrote (the interrupted loop address, a marker). Nothing here proves a CPU speed or a timer rate: the later rows that read the
; counter after a counted number of instructions depend on the community-reported CPU cycle length (12 master clocks at the low speed) and are E2 self-consistency, labelled GUEST:timer-e2 or ASSUMED; the one claim of a rate is the C-level case timer_pure_function.
; Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 10
DSV_CHECKS = 33

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

CYC_STEP = 256                  ; CPU cycles per prescaler step at the low speed: 3072 master clocks over the community-reported 12 master clocks per cycle (ER-3): every row that uses it is E2 self-consistency

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
        LIT 11, $00
        LIT 12, $00
        LIT 13, $00
        LIT 14, $00                     ; GUEST (14 to 17): the table entry is an XOR mask of $00
        LIT 15, $00
        LIT 16, $00
        LIT 17, $00
        LIT 18, 1                       ; reload 2, four steps: 2, 1, 0, then the borrow reloads 2, then 1
        LIT 19, 5                       ; reload 5, six steps: 5, 4, 3, 2, 1, 0, then the sixth step borrows and the counter is the reload value 5
        LIT 20, $4D                     ; the reload 77 loaded by the start
        LIT 21, 127 - (1 + 6 * 100 + 2 + 4) / CYC_STEP
        LIT 22, 127 - (1 + 6 * 100 + 2 + 4) / CYC_STEP
        LIT 23, 127 - (1 + 6 * 100 + 2 + 4) / CYC_STEP
        LIT 24, 100 - (1 + 6 * 100 + 2 + 5 + 6 * 60 + 4) / CYC_STEP
        LIT 25, 127 - (1 + 6 * 100 + 2 + 5 + 6 * 60 + 4) / CYC_STEP
        LIT 26, $AA
        LIT 27, $2A
        LIT 28, $04
        LIT 29, $00
        LIT 30, $01
        LIT 31, $00
        LIT 32, $04
        LIT 33, $20

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

; RESET_SCEN: a known state before a scenario: I set, D clear, the timer stopped, TIQD clear, no request, no handler count
        .macro RESET_SCEN
        sei
        cld
        stz a:TIM_CTRL
        stz a:IRQ_DIS
        stz a:IRQ_REQ
        stz a:HCOUNT
        stz a:H_P
        .endmacro

; TIM_START reload: set the reload register and start (the counter takes the reload value): 2 + 5 + 2 + 5 cycles, the control access is in the last cycle of the second STA
        .macro TIM_START reload
        lda #(reload)
        sta a:TIM_RELOAD
        lda #$01
        sta a:TIM_CTRL
        .endmacro

; MEASURE_RAW reload, k1, k2, addr: start, two counted delays (k2 may be 0), then read the counter at addr: A = the observed byte. The read access is 1 + 6 * (k1 + k2) + 4 cycles after the start access
        .macro MEASURE_RAW reload, k1, k2, addr
        TIM_START reload
        DELAY k1
        .if (k2) > 0
        DELAY k2
        .endif
        lda a:addr
        .endmacro

; MEASURE reload, k1, k2, addr: as MEASURE_RAW, then GEXP = reload - the elapsed cycles over CYC_STEP (E2: instruction counts and the community cycle length) and A = the observed byte again
        .macro MEASURE reload, k1, k2, addr
        MEASURE_RAW reload, k1, k2, addr
        sta a:SCRATCH
        lda #((reload) - (1 + 6 * ((k1) + (k2)) + 4) / CYC_STEP)
        sta a:GEXP
        lda a:SCRATCH
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

; ---- the reset state, read before anything touches the timer (H8-16, H8-18) ----
        lda a:IRQ_REQ
        and #$04                        ; check 11 MAN:H8-18: TIQ is reset by the reset
        CHECK_LIT 11
        lda a:IRQ_DIS
        and #$07                        ; check 12 MAN:H8-16: the disable register is all 0 after the reset
        CHECK_LIT 12
        DELAY 255
        DELAY 255
        lda a:IRQ_REQ
        and #$04                        ; check 13 MAN:H8-18: the timer is stopped by the reset: nearly twelve prescaler steps pass and no borrow requests the interrupt
        CHECK_LIT 13

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

; ---- part 2: the counter read after counted delays (E2, ER-3), edges of the control register ----
        RESET_SCEN
        MEASURE 127, 41, 0, TIM_RELOAD
        CHECK_GUEST 14                  ; check 14 GUEST:timer-e2: reload 127, 251 cycles after the start (under 256): no prescaler step yet, the counter reads 127
        RESET_SCEN
        MEASURE 127, 42, 0, TIM_RELOAD
        CHECK_GUEST 15                  ; check 15 GUEST:timer-e2: 257 cycles after the start: one step, 126 (the first boundary seen from both sides, with check 14)
        RESET_SCEN
        MEASURE 127, 200, 11, TIM_RELOAD
        CHECK_GUEST 16                  ; check 16 GUEST:timer-e2: 1271 cycles, four steps and 96 percent of a fifth: the counter reads 123 (the steps that have happened, never rounded up)
        RESET_SCEN
        MEASURE 127, 100, 0, TIM_CTRL
        CHECK_GUEST 17                  ; check 17 GUEST:timer-e2: the control address reads the same counter (D-109): 605 cycles, two steps, 125
        RESET_SCEN
        MEASURE_RAW 2, 186, 0, TIM_RELOAD
        CHECK_LIT 18                    ; check 18 ASSUMED:LED-IRQ-0002: reload 2 and 1121 cycles (four steps): 2, 1, 0, the borrow reloads 2, then 1: the period is reload + 1 = 3 steps (assumption A1)
        RESET_SCEN
        MEASURE_RAW 5, 130, 130, TIM_RELOAD
        CHECK_LIT 19                    ; check 19 ASSUMED:LED-IRQ-0003: reload 5 and 1565 cycles (six steps): the sixth step borrows and the counter reads the reload value 5 (no $7F window after the borrow)
        RESET_SCEN
        TIM_START 127
        DELAY 100
        lda #$00
        sta a:TIM_CTRL                  ; stop with the counter at 127 - 2 = 125 held
        lda #$4D
        sta a:TIM_RELOAD                ; the reload register while stopped: 77
        lda #$01
        sta a:TIM_CTRL                  ; start: the counter takes the reload value (H8-22), not the held 125 and not the old reload 127
        lda a:TIM_RELOAD
        CHECK_LIT 20                    ; check 20 MAN:H8-22: the counter is loaded with the reload value when control bit 0 goes from 0 to 1: 77
        RESET_SCEN
        TIM_START 127
        DELAY 100
        lda #$00
        sta a:TIM_CTRL                  ; stop 1 + 600 + 2 + 4 cycles after the start access: two steps, 125
        lda a:TIM_RELOAD
        CHECK_LIT 21                    ; check 21 ASSUMED:LED-IRQ-0003: a stopped timer holds its counter (E2: the value is 127 minus two steps from the instruction counts)
        DELAY 255
        DELAY 255
        lda a:TIM_RELOAD
        CHECK_LIT 22                    ; check 22 ASSUMED:LED-IRQ-0003: the held counter is the same eleven steps later
        lda #$00
        sta a:TIM_CTRL                  ; writing 0 to a stopped timer
        lda a:TIM_RELOAD
        CHECK_LIT 23                    ; check 23 ASSUMED:LED-IRQ-0003: a write that repeats the stopped state changes nothing
        RESET_SCEN
        TIM_START 100
        DELAY 100
        lda #$14
        sta a:TIM_RELOAD                ; a reload write while running: 20 for the next reload only
        DELAY 60
        lda a:TIM_RELOAD
        CHECK_LIT 24                    ; check 24 ASSUMED:LED-IRQ-0002: the write did not touch the counter: 972 cycles after the start, three steps, 100 - 3 = 97 (a counter reloaded by the write would read about 19)
        RESET_SCEN
        TIM_START 127
        DELAY 100
        lda #$01
        sta a:TIM_CTRL                  ; writing 1 to a running timer
        DELAY 60
        lda a:TIM_RELOAD
        CHECK_LIT 25                    ; check 25 ASSUMED:LED-IRQ-0003: the write did not reload the counter or restart the prescaler: 972 cycles after the start, three steps, 124 (a restart would read 126)
        RESET_SCEN
        TIM_START $2A
        lda #$00
        sta a:TIM_CTRL                  ; stop at once: the counter holds the reload value $2A
        lda #$FF
        sta a:IOWIN                     ; io_buffer = $FF
        lda a:TIM_RELOAD
        CHECK_LIT 26                    ; check 26 ASSUMED:LED-IRQ-0003: bit 7 of a counter read comes from io_buffer: $80 over the 7-bit counter $2A
        lda #$00
        sta a:IOWIN                     ; io_buffer = $00
        lda a:TIM_RELOAD
        CHECK_LIT 27                    ; check 27 ASSUMED:LED-IRQ-0003: with bit 7 of io_buffer clear the same read is the 7-bit counter, $2A

; ---- part 3: the request latches while masked, and what the entry pushes after CLI and SET ----
        RESET_SCEN
        lda #$04
        sta a:IRQ_DIS                   ; TIQD = 1: the timer interrupt is disabled (H8-16)
        TIM_START $00
        cli                             ; I = 0: only TIQD masks
        DELAY 130
        DELAY 130
        lda a:IRQ_REQ
        and #$04                        ; check 28 ASSUMED:LED-IRQ-0003: the borrow set TIQ although TIQD masked it (six borrows at reload 0)
        CHECK_LIT 28
        lda a:HCOUNT                    ; check 29 MAN:H8-16: TIQD = 1 disables the timer interrupt: the handler did not run
        CHECK_LIT 29
        lda #$00
        sta a:IRQ_DIS                   ; TIQD = 0: the latched request is delivered now, I being clear
        nop
        nop
        DELAY 20
        lda a:HCOUNT
        sei
        CHECK_LIT 30                    ; check 30 MAN:H8-15: TIQ is one bit: six borrows give one request, so one entry (the handler stops the timer and acknowledges)
        RESET_SCEN
        TIM_START $00
        DELAY 130
        DELAY 130
        lda a:HCOUNT                    ; check 31 MAN:H8-21: the CPU is requested only if I is 0: with I set the handler did not run
        CHECK_LIT 31
        lda a:IRQ_REQ
        and #$04                        ; check 32 ASSUMED:LED-IRQ-0003: the request was latched while I masked it
        CHECK_LIT 32
        cli                             ; with irq_i_sample = pre the interrupt is not taken after CLI
        set                             ; T = 1: the interrupt is taken after this instruction, and the entry pushes T as it is
        nop
        DELAY 10
        lda a:H_P
        and #$20
        sei
        CHECK_LIT 33                    ; check 33 ASSUMED:LED-IRQ-0005: the entry pushed T = 1 (bit 5): the interrupt was taken after SET, one instruction after CLI
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
