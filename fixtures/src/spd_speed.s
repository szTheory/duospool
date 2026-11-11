; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; spd_speed.s - the CPU speed switch seen from the guest, with the timer as the clock (plan 03-23; D-101, D-107, D-124, D-125, ER-3; H8-18, H8-20, H8-21, H8-25). It is assembled by ca65 in the gate and never committed as a binary.
; The timer window is logical $6C00 (reload, counter read) and $6C01 (control) with MPR3 = $FF; the hardware page windows at logical $0000 (VDC), $0400 (VCE) and $0800 (PSG) are visible through MPR0 = $FF. The timer steps every 3072 master clocks (H8-20, H8-21), so
; a fixed instruction loop is measured by the counter delta: the elapsed master clocks are the loop's CPU cycles times the clocks per cycle of the speed in force, over 3072, rounded down. Every row that depends on the clocks per cycle (12 at the low speed is a community report,
; 3 at the high speed follows the manual's prescaler, LED-CPU-0003) is E2 self-consistency (ER-3): the expected counter is written by the assembler from the instruction counts of this file, never read from the core. What is a manual fact is the power-up speed (H8-18: low speed mode is set by
; the reset) and that CSL and CSH are one byte (H8-25); the ratio of the two speeds, the charge of a switch at the OLD speed and the VDC and VCE penalty of one cycle are modelling assumptions of this project (ASSUMED LED-CPU-0006 and LED-CPU-0017), shown here and not proven.
; Check 1 measures the loop before anything switches the speed (power-up low speed). Then the subroutine probe at the fixed logical address $E100 runs CSH, INX, CSH, INX, CSL, INX, CSL, INX; its start stamps and speed bytes are judged from the instruction trace by the case
; spd_speed_trace (kind cycle_probe, the addresses are those of the ca65 listing). Checks 2 and 3 repeat the loop after CSH and after CSL, check 4 relates the two measurements (low = 4 * high + the remainder, computed in the guest), checks 5 and 6 repeat a switch to the speed already in force.
; Checks 7 and 8 are tuned so that the interval between the timer start and the counter read ends 12 and 6 master clocks past (CSH) or before (CSL) a step boundary: they say which speed the switching instruction itself was charged at (3 cycles at the old speed, D-101). Checks 9 to 12 compare a loop of
; 256 reads of a hardware-page window with the same loop over a work-RAM cell: the difference is exactly the number of reads times the penalty cycles times the clocks per cycle, so one step of 3072 master clocks for 256 reads at the low speed and 1024 reads at the high speed, whatever the phase of the timer; the PSG window
; sits outside the penalty range (D-107) and shows no difference. Check 13 shows the probe completed. Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 12
DSV_CHECKS = 13

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

MCLK_L = 12                     ; master clocks per CPU cycle at the low speed (community report, ER-3, LED-CPU-0003): every row that uses it is E2 self-consistency
MCLK_H = 3                      ; at the high speed (the manual's prescaler, LED-CPU-0003)
STEP = 3072                     ; master clocks per timer step (3 * 1024, H8-20, H8-21)
PENALTY = 1                     ; CPU cycles per VDC or VCE access (ASSUMED LED-CPU-0017, D-107)
LOOP_E = 6 * 405 + 5            ; the elapsed CPU cycles of the fixed loop: the control access is in the last cycle of the STA, so 1 cycle of it remains, 6 * (255 + 150) for the two delays, 4 cycles of the LDA before its access
RATIO_REM = (LOOP_E .mod 1024) / 256   ; steps at the low speed = 4 * steps at the high speed + this remainder
BA1 = 20                        ; check 7: counted delay (units of 6 cycles) before CSH, at the low speed
BB1 = 88                        ; and after it, at the high speed
BA2 = 87                        ; check 8: before CSL, at the high speed
BB2 = 20                        ; after it, at the low speed

        CHECK_TABLE DSV_CHECKS
        LIT 1, 127 - (MCLK_L * LOOP_E) / STEP                                          ; MAN:H8-18 (low speed after reset), the step count is E2
        LIT 2, 127 - (MCLK_H * LOOP_E) / STEP                                          ; ASSUMED:LED-CPU-0006
        LIT 3, 127 - (MCLK_L * LOOP_E) / STEP
        LIT 4, $00                                                                     ; GUEST: the table entry is an XOR mask of $00
        LIT 5, 127 - (MCLK_H * LOOP_E) / STEP
        LIT 6, 127 - (MCLK_L * LOOP_E) / STEP
        LIT 7, 127 - (MCLK_L * (1 + 6 * BA1 + 3) + MCLK_H * (6 * BB1 + 4)) / STEP       ; CSH charged at the old (low) speed
        LIT 8, 127 - (MCLK_H * (1 + 6 * BA2 + 3) + MCLK_L * (6 * BB2 + 4)) / STEP       ; CSL charged at the old (high) speed
        LIT 9, (256 * PENALTY * MCLK_L) / STEP                                         ; the VDC window, low speed
        LIT 10, (1024 * PENALTY * MCLK_H) / STEP                                       ; the VDC window, high speed
        LIT 11, (256 * PENALTY * MCLK_L) / STEP                                        ; the VCE window, low speed
        LIT 12, $00                                                                    ; the PSG window is outside the penalty range
        LIT 13, $04                                                                    ; X after four INX

SL0 = $01                       ; the TAM operand of MPR0 (logical $0000-$1FFF; bit n selects MPR n)
SL3 = $08                       ; the TAM operand of MPR3 (logical $6000-$7FFF)
TIM_RELOAD = $6C00              ; the timer window with MPR3 = $FF: the reload register (write) and the counter (read)
TIM_CTRL = $6C01                ; the control register (write, bit 0 starts and stops) and the counter (read)
W_VDC = $0000                   ; the VDC window while MPR0 = $FF (physical $1FE000)
W_VCE = $0400                   ; the VCE window (physical $1FE400)
W_PSG = $0800                   ; the PSG window, the first byte outside the penalty range (physical $1FE800)

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
C_LOW = $2410                   ; the counter of the fixed loop at the low speed (check 1)
C_HIGH = $2411                  ; and at the high speed (check 2)
C_RAM = $2412                   ; the counter after the loop over a work-RAM cell
C_WIN = $2413                   ; the counter after the same loop over a window
MK_X = $2414                    ; X after the four INX of the probe
RAMCELL = $2420                 ; the work-RAM cell the base loop reads

; DELAY k: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2): k from 1 to 255
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

; TIM_START reload: set the reload register and start (the counter takes the reload value): 2 + 5 + 2 + 5 cycles, the control access is in the last cycle of the second STA
        .macro TIM_START reload
        lda #(reload)
        sta a:TIM_RELOAD
        lda #$01
        sta a:TIM_CTRL
        .endmacro

; SPD_LOOP: stop, start at reload 127, two counted delays of 255 and 150 (LOOP_E cycles up to the read), read the counter: A = the counter over the seven bits
        .macro SPD_LOOP
        stz a:TIM_CTRL
        TIM_START 127
        DELAY 255
        DELAY 150
        lda a:TIM_RELOAD
        and #$7F
        .endmacro

; RD_LOOP addr, outer: outer * 256 reads of addr (LDA abs, DEX, BNE per read)
        .macro RD_LOOP addr, outer
        .local o, i
        ldy #(outer)
o:      ldx #$00
i:      lda a:addr
        dex
        bne i
        dey
        bne o
        .endmacro

; PEN_MEASURE addr, outer: stop, start at reload 127, the read loop, read the counter: A = the counter over the seven bits
        .macro PEN_MEASURE addr, outer
        stz a:TIM_CTRL
        TIM_START 127
        RD_LOOP addr, outer
        lda a:TIM_RELOAD
        and #$7F
        .endmacro

; PEN_DIFF addr, outer: the base loop over RAMCELL, then the same loop over addr; A = counter(base) - counter(addr), the steps the second loop took more (it runs longer by outer * 256 penalty cycles when addr is in the penalty range)
        .macro PEN_DIFF addr, outer
        PEN_MEASURE RAMCELL, outer
        sta a:C_RAM
        PEN_MEASURE addr, outer
        sta a:C_WIN
        sec
        lda a:C_RAM
        sbc a:C_WIN
        .endmacro

        .res $100 - (* - reset), $FF      ; pad the code window so that probe starts at offset $100 of it (logical $E100, hucard.cfg ROMA); the assert below fails the build if the library code ever outgrows that
        .assert * - reset = $100, error, "probe must start at the fixed address $E100"
probe:
m_csh_l: csh                            ; $E100 CSH at the low speed: 3 cycles at the old speed (H8-25, D-101)
m_inx_h: inx                            ; $E101 INX at the high speed: 2 cycles
m_csh_h: csh                            ; $E102 CSH at the high speed: the speed stays high, 3 cycles
m_inx_h2: inx                           ; $E103 INX at the high speed
m_csl_h: csl                            ; $E104 CSL at the high speed: 3 cycles at the old speed
m_inx_l: inx                            ; $E105 INX at the low speed
m_csl_l: csl                            ; $E106 CSL at the low speed: the speed stays low, 3 cycles
m_inx_l2: inx                           ; $E107 INX at the low speed
        stx a:MK_X
        rts

fixture_main:
        lda #$FF
        tam #SL3                        ; MPR3 = $FF: the timer window at logical $6000-$7FFF
        stz a:C_LOW
        stz a:C_HIGH
        stz a:C_RAM
        stz a:C_WIN
        stz a:MK_X
        stz a:RAMCELL
; ---- power-up speed, before anything switches it ----
        SPD_LOOP
        sta a:C_LOW
        CHECK_LIT 1                     ; check 1 MAN:H8-18: low speed mode is set by the reset, so the fixed loop takes 12 clocks per cycle: nine steps of 3072 (E2: the 12 is community-reported)
        ldx #$00
        jsr probe
; ---- the same loop after CSH and after CSL ----
        csh
        SPD_LOOP
        sta a:C_HIGH
        CHECK_LIT 2                     ; check 2 ASSUMED:LED-CPU-0006: after CSH the same loop takes 3 clocks per cycle: two steps (E2)
        csl
        SPD_LOOP
        CHECK_LIT 3                     ; check 3 ASSUMED:LED-CPU-0006: after CSL the loop is back to nine steps (E2)
        lda #$7F
        sec
        sbc a:C_HIGH                    ; the steps at the high speed
        asl a
        asl a
        clc
        adc #RATIO_REM
        sta a:GEXP                      ; the model: steps at the low speed = 4 * steps at the high speed + the remainder of the loop length
        lda #$7F
        sec
        sbc a:C_LOW                     ; the steps at the low speed
        CHECK_GUEST 4                   ; check 4 GUEST:speed-ratio-e2: the loop takes four times as long at the low speed (E2, ASSUMED:LED-CPU-0006: the two measurements of one loop at the two speeds)
        csh
        csh
        SPD_LOOP
        CHECK_LIT 5                     ; check 5 ASSUMED:LED-CPU-0006: CSH at the high speed leaves the speed high
        csl
        csl
        SPD_LOOP
        CHECK_LIT 6                     ; check 6 ASSUMED:LED-CPU-0006: CSL at the low speed leaves the speed low
; ---- which speed the switching instruction itself is charged at (D-101) ----
        stz a:TIM_CTRL
        TIM_START 127
        DELAY BA1
        csh
        DELAY BB1
        lda a:TIM_RELOAD
        and #$7F
        csl
        CHECK_LIT 7                     ; check 7 ASSUMED:LED-CPU-0006: CSH is 3 cycles at the old (low) speed: the interval is 3084 master clocks, one step; charged at the new speed it would be 3057, no step; with no switch at all 7872, two steps
        csh
        stz a:TIM_CTRL
        TIM_START 127
        DELAY BA2
        csl
        DELAY BB2
        lda a:TIM_RELOAD
        and #$7F
        CHECK_LIT 8                     ; check 8 ASSUMED:LED-CPU-0006: CSL is 3 cycles at the old (high) speed: the interval is 3066 master clocks, no step; charged at the new speed it would be 3093, one step
; ---- the VDC and VCE penalty, one cycle at the speed in force (D-107, ASSUMED LED-CPU-0017) ----
        lda #$FF
        tam #SL0                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        PEN_DIFF W_VDC, 1
        CHECK_LIT 9                     ; check 9 ASSUMED:LED-CPU-0017: 256 reads of the VDC window cost 256 more cycles of 12 clocks: exactly one step more than the same loop over RAM, whatever the phase
        csh
        PEN_DIFF W_VDC, 4
        csl
        CHECK_LIT 10                    ; check 10 ASSUMED:LED-CPU-0017: at the high speed 1024 reads cost 1024 more cycles of 3 clocks: one step; a penalty charged at the low speed would give four
        PEN_DIFF W_VCE, 1
        CHECK_LIT 11                    ; check 11 ASSUMED:LED-CPU-0017: the VCE window carries the penalty too
        PEN_DIFF W_PSG, 1
        CHECK_LIT 12                    ; check 12 ASSUMED:LED-CPU-0017: the PSG window, the first address outside the range, carries none
        lda #$F8
        tam #SL0                        ; MPR0 = $F8 again
        lda a:MK_X
        CHECK_LIT 13                    ; check 13 MAN:H8-25: CSL and CSH are one byte each: four INX in the probe leave X = 4, so the probe ran through
        stz a:TIM_CTRL
        rts
