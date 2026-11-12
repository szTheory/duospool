; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; irq_ctl.s - the interrupt controller seen from the guest (plan 03-22; D-110, D-111, D-112, D-124, D-125, ER-5; H8-12, H8-13, H8-14, H8-15, H8-16, H8-17, H8-18, H8-21, H8-24). It is assembled by ca65 in the gate and never committed as a binary.
; The interrupt registers are $7402 (disable, bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD) and $7403 (request, bit 0 the IRQ2 level, bit 1 the IRQ1 level, bit 2 TIQ, a write clears TIQ only) with MPR3 = $FF; the timer window is $6C00 and $6C01.
; IRQ1 and IRQ2 are driven by nothing in the machine yet: the case declares a TEST stimulus (the case key irq_stim, a list of events AT,LINE,LEVEL in master clocks) and this file is written against it. The stimulus is a pure function of machine time, it is
; not evidence about the VDC or any hardware source (ER-5, D-112, ASSUMED LED-IRQ-0008), and the fixture is only meaningful under the case that declares it: every scenario first polls the request register until the line it needs reads high, so the
; scenarios do not depend on how long the code before them took, and every wait is bounded (a machine with no stimulus fails a check, it does not hang).
; The stimulus the cases declare (windows in master clocks, the line is high from the first time to the second): window A IRQ1 from 1200000 to 1500000; B IRQ2 from 2200000 to 2500000; C IRQ1 and IRQ2 from 3200000 to 3500000; D IRQ1 from 4200000 to 4500000.
; Windows B, C and D (checks 16 to 30) come after A: B IRQ2 and BRK (IRQ2D masks, BRK and IRQ2 reach the same handler through $FFF6 and are told apart by the pushed B bit), C all three sources (the request register reads 7, a write clears TIQ only, the three are taken in the order timer, IRQ1,
; IRQ2 whatever the latency parameter), D the latency probe (an IRQ1 that is pending when CLI executes: the A register the handler sees at its entry tells whether the entry came right after CLI or after the next instruction, the observe row 30 of the parameter irq_i_sample, ASSUMED LED-IRQ-0005).
; Window E (checks 31 to 33) needs no stimulus: it starts the timer at reload 0 with I clear and puts a write to $1403, and then a sample, at exactly the stamp of the first borrow (counted instructions, CYC_STEP, E2 self-consistency, ER-3): ties are inclusive and a write in the sample cycle takes effect before the sample (D-111, ASSUMED LED-IRQ-0006).
; Window A (checks 5 to 15): the IRQ1 level reads in bit 1, I and IRQ1D each mask the delivery, the unmasking write is taken at once, the entry goes through $FFF8, pushes B = 0 and the interrupted D and I, the handler sees I set and D clear and bit 1 of $1403 set,
; a write to $1403 does not clear a level, the level ends with its window. Checks 1 to 4 read the register state before any event. Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 11
DSV_CHECKS = 33

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
        LIT 16, $01
        LIT 17, $00
        LIT 18, $02
        LIT 19, $10
        LIT 20, $00
        LIT 21, $00
        LIT 22, $07
        LIT 23, $03
        LIT 24, $07
        LIT 25, $03
        LIT 26, $04
        LIT 27, $01
        LIT 28, $02
        LIT 29, $01
        LIT 30, $00                     ; an observe row (the table entry is $00 as for every check): the case judges the byte, which is the A register at the entry of the handler
        LIT 31, $00
        LIT 32, $00
        LIT 33, $00                     ; GUEST: the table entry is an XOR mask of $00

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
T_PCL = $241E                   ; the return address low byte the timer entry pushed

CYC_STEP = 256                  ; CPU cycles per prescaler step at the low speed: 3072 master clocks over the community-reported 12 master clocks per cycle (ER-3): the rows of window E that use it are E2 self-consistency

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
        lda #<irq2_h
        sta a:VEC_IRQ2                  ; the IRQ2 and BRK vector (both use $FFF6)
        lda #>irq2_h
        sta a:VEC_IRQ2+1
        lda #<timer_h
        sta a:VEC_TIMER
        lda #>timer_h
        sta a:VEC_TIMER+1
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

; ---- window B: IRQ2 and BRK ----
        jsr reset_scen
        WAIT_SET $01                    ; the IRQ2 level of the declared stimulus
        lda a:IRQ_REQ
        and #$07
        CHECK_LIT 16                    ; check 16 MAN:H8-15: bit 0 of the request register is the IRQ2 level; IRQ1 (bit 1) and TIQ (bit 2) read 0
        lda #$01
        sta a:IRQ_DIS                   ; IRQ2D = 1
        cli                             ; I = 0: only IRQ2D masks
        DELAY 30
        sei
        lda a:H2_COUNT
        CHECK_LIT 17                    ; check 17 MAN:H8-16: IRQ2D = 1 disables the interrupt request at the IRQ2 pin: with I clear the handler did not run
        brk                             ; BRK is not masked by I (I is set here): it pushes B = 1 and goes through $FFF6 like IRQ2
        .byte $EA                       ; the byte after BRK: the return address is BRK + 2 (H8-17)
        lda #$00
        sta a:IRQ_DIS                   ; IRQ2D = 0 (the BRK handler masked it again); I is still set, the RTI restored it
        cli                             ; the pending IRQ2 is taken after the next instruction (pre) or at once (post)
        nop
        DELAY 4
        sei
        lda a:H2_COUNT
        CHECK_LIT 18                    ; check 18 MAN:H8-13: BRK and IRQ2 reach the same handler through $FFF6: it ran twice
        lda a:PBLOG
        and #$10
        CHECK_LIT 19                    ; check 19 MAN:H8-5: the BRK entry pushed the status byte with B = 1
        lda a:PBLOG+1
        and #$10
        CHECK_LIT 20                    ; check 20 MAN:H8-5: the IRQ2 entry pushed the status byte with B = 0, so the shared handler tells them apart
        lda a:H1_COUNT
        CHECK_LIT 21                    ; check 21 MAN:H8-13: IRQ2 does not use the IRQ1 vector $FFF8
        WAIT_CLEAR $03

; ---- window C: IRQ1, IRQ2 and the timer pending together ----
        jsr reset_scen
        WAIT_SET $03                    ; both levels
        lda #$00
        sta a:TIM_RELOAD                ; reload 0: a borrow at every prescaler step
        lda #$01
        sta a:TIM_CTRL                  ; start
        DELAY 60                        ; 361 cycles: one step (256 cycles) has passed, so TIQ is set
        lda a:IRQ_REQ
        and #$07
        CHECK_LIT 22                    ; check 22 MAN:H8-15: IRQ2, IRQ1 and TIQ read 1 together (bits 0, 1 and 2)
        sta a:IRQ_REQ                   ; a write to the request register resets TIQ only
        lda a:IRQ_REQ
        and #$07
        CHECK_LIT 23                    ; check 23 MAN:H8-15: after the write TIQ reads 0 and the two levels, which are inputs, still read 1
        DELAY 60                        ; the timer still runs at reload 0
        lda a:IRQ_REQ
        and #$07
        CHECK_LIT 24                    ; check 24 MAN:H8-15: the next borrow sets TIQ again
        cli                             ; all three are pending with I clear: the handlers mask or acknowledge their source, so each is taken once
        nop
        DELAY 30
        sei
        lda a:LOGN
        CHECK_LIT 25                    ; check 25 MAN:H8-13: three entries, one per source
        lda a:LOG
        CHECK_LIT 26                    ; check 26 MAN:H8-13: the timer (code $04) is taken first: the priority is timer, IRQ1, IRQ2
        lda a:LOG+1
        CHECK_LIT 27                    ; check 27 MAN:H8-13: IRQ1 (code $01) is taken before IRQ2
        lda a:LOG+2
        CHECK_LIT 28                    ; check 28 MAN:H8-13: IRQ2 (code $02) is taken last
        WAIT_CLEAR $03

; ---- window D: an IRQ1 pending when CLI executes ----
        jsr reset_scen
        WAIT_SET $02                    ; the IRQ1 level of the declared stimulus, I is set
        lda #$00                        ; A = 0 before CLI
        cli                             ; I = 0 with IRQ1 pending
        lda #$77                        ; the instruction after CLI: with irq_i_sample = pre the entry comes after it and the handler sees A = $77; with post it comes at once and the handler sees A = 0
        nop
        DELAY 4
        sei
        lda a:H1_COUNT
        CHECK_LIT 29                    ; check 29 MAN:H8-13: the pending IRQ1 is taken after CLI under both values of the parameter, once
        CHECK_OBSERVE 30, a:H_A         ; check 30 ASSUMED:LED-IRQ-0005: the A register at the entry, observe-only: $77 under pre, $00 under post (the case supplies it)
        WAIT_CLEAR $03

; ---- window E: the sample cycle and the cycle of a write to $1403 (ties are inclusive, D-111; E2, ER-3, LED-CPU-0004 and LED-IRQ-0006 assumed) ----
; The timer is started at reload 0 with I clear: its first borrow is due 3072 master clocks after the start access, which is CYC_STEP = 256 CPU cycles later. A single data access is in the last cycle of its instruction, and so is the sample, so STA abs (5 cycles)
; has its access and its sample at the same stamp. The instructions between the start access and the next instruction are counted: DELAY k is 6 * k cycles, LDA abs 5, NOP 2; the instruction that follows the start STA begins one cycle after the start access.
; Scenario 1: the acknowledge write is at exactly the stamp of the borrow (1 + 251 cycles after the start access, then 4 cycles into STA abs: 256). The borrow counts at the tie and the write clears it before the sample, so no entry is taken.
        jsr reset_scen
        lda #$00
        sta a:TIM_RELOAD                ; reload 0
        cli                             ; I = 0
        lda #$01
        sta a:TIM_CTRL                  ; start: the access stamp T0
        DELAY 41                        ; 246 cycles
        lda a:SCRATCH                   ; 5 cycles: 251 after the end of the start STA, so the STA below starts 252 cycles after T0 and its access is 256 cycles after T0
        sta a:IRQ_REQ                   ; the acknowledge at the stamp of the borrow
        lda #$00
        sta a:TIM_CTRL                  ; stop (7 cycles later, well inside the next step)
        DELAY 10
        sei
        lda a:HT_COUNT
        CHECK_LIT 31                    ; check 31 ASSUMED:LED-IRQ-0006: a write to $1403 in the cycle of the borrow takes effect before the sample: the borrow is absorbed and the handler did not run
        lda a:IRQ_REQ
        and #$04
        CHECK_LIT 32                    ; check 32 ASSUMED:LED-IRQ-0006: TIQ reads 0 after that write (the borrow was cleared, not delayed)
; Scenario 2: the borrow is due at exactly the sample stamp of a NOP (the second of two: it starts 255 cycles after T0, its sample is at 256). Ties are inclusive, so the entry is taken right after that NOP and pushes the address of the label after it.
        jsr reset_scen
        lda #$00
        sta a:TIM_RELOAD
        cli
        lda #$01
        sta a:TIM_CTRL                  ; start: the access stamp T0
        DELAY 42                        ; 252 cycles
        nop                             ; 2 cycles: the next NOP starts 255 cycles after T0
        nop                             ; its sample (the start of its last cycle) is 256 cycles after T0, the borrow stamp
tie_after:
        nop
        nop
        DELAY 10
        sei
        lda #<tie_after
        sta a:GEXP
        lda a:T_PCL
        CHECK_GUEST 33                  ; check 33 GUEST:tie-address (ASSUMED:LED-IRQ-0006): the entry came right after the NOP whose sample stamp equals the borrow stamp: the pushed return address is the label after it (a sample that excluded the tie pushes the next one)
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
        stz a:T_PCL
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

; the IRQ2 and BRK vector handler: save A and X, log the status byte the entry pushed (the B bit tells BRK from IRQ2), mask IRQ2, log and count, RTI
irq2_h:
        pha
        phx
        tsx
        lda a:$2103,x
        ldx a:H2_COUNT
        sta a:PBLOG,x
        lda a:IRQ_DIS
        ora #$01
        sta a:IRQ_DIS
        ldx a:LOGN
        lda #$02
        sta a:LOG,x
        inc a:LOGN
        inc a:H2_COUNT
        plx
        pla
        rti

; the TIMER vector handler: save A and X, copy the return address low byte the entry pushed, stop the timer, acknowledge with a write to $1403, log and count, RTI
timer_h:
        pha
        phx
        tsx                             ; the entry pushed PCH, PCL and P, then A and X: PCL is at $2100 + S + 4
        lda a:$2104,x
        sta a:T_PCL
        lda #$00
        sta a:TIM_CTRL
        sta a:IRQ_REQ
        ldx a:LOGN
        lda #$04
        sta a:LOG,x
        inc a:LOGN
        inc a:HT_COUNT
        plx
        pla
        rti
