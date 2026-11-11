; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_blk.s - the block transfers TII, TDD, TIN, TIA and TAI seen from the guest (plan 03-25; D-92, D-100, D-124, D-125, D-127; H8-6, S8-88, S8-89, H8-25). It is assembled by ca65 in the gate and never committed as a binary.
; The expected values come from the pages (Table 2-1-1 source and destination modes, the length 0 = 65536 rule, 17 + 6x cycles, A X Y returned from the stack), from bytes the guest computes itself, or from the assumed ledger entries named in cpu_blk.checks.
; Three images are built from this one source (scripts/fixture-build): cpu-blk (the plain image), cpu-blk-len0 (-D LEN0: the long transfer is a TIA of 65536 bytes, the length operand 0) and cpu-blk-irq (-D BLK_IRQ: the long transfer runs with I clear, the timer
; at reload 0 and an IRQ2 level that the case declares inside it). All three run the same 50 checks; the checks 39 to 50 differ in their expected literals by image (the table entries are assembled per image, the meaning is in the checks file).
; Checks 1 to 38 are the common part: one transfer per instruction and mode with the registers and flags set before it (A, X, Y and the flag byte are loaded by PREP, PLP last, so a flag a transfer changed would show), the observed bytes read after it (OBS),
; then the overlap cases, the 16-bit wrap of the source and destination addresses, a length above 256, a destination on the stack (the epilogue pops the clobbered bytes), and the cycle count read from the timer (E2: the instruction counts and the community
; cycle length are inputs; the transfers are tuned to end 6 cycles past a prescaler step and 6 before one, at the low speed and at the high speed). Checks 39 to 42 are the long transfer (a TIA whose last source byte is the one at $FFFF), 43 to 50 the interrupt
; around a transfer: the timer borrows every step during a 4096-byte TII, and in the irq image an IRQ2 level rises inside it; no interrupt is taken before the transfer ends (the handlers see all 4096 bytes written), the handlers then run (the timer interrupt
; first), A, X and Y read back equal to their values before the transfer, and the pushed return address is the instruction after the transfer (ASSUMED LED-IRQ-0007).
; Memory: MPR0 = $F8 (work RAM at logical $0000, so the 16-bit wrap is visible), MPR1 = $F8 and MPR2 = $F9 (startup.inc), MPR3 = $01 (HuCard bank 1, all $FF: the long source at $6000), MPR5 = $FF (the hardware page at $A000: timer $AC00 and $AC01,
; interrupt registers $B402 and $B403), MPR4 = $02 and MPR6 = $04 (more $FF banks). Every cell a check reads is written by the fixture first (D-127), and every register a transfer must give back is loaded first.
        .setcpu "huc6280"

DSV_ID = 13
DSV_CHECKS = 50

        .ifdef LEN0
LONG_OP = 0                     ; the length operand 0 means 65536 bytes (H8-6)
LONG_BYTES = 65536
LONG_SRC = $0000                ; the walk covers the whole logical space once: the last byte read is the one at $FFFF
        .else
LONG_OP = 3
LONG_BYTES = 3
LONG_SRC = $FFFD                ; three bytes: $FFFD, $FFFE and $FFFF
        .endif
        .ifdef BLK_IRQ
IRQ_ON = 1
        .else
IRQ_ON = 0
        .endif

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        .assert reset = $E000, error, "the vector bytes the long transfer reads ($00 and $E0) are the reset address $E000: startup.inc must start the code segment"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $A1
        LIT 2, $90
        LIT 3, $5A
        LIT 4, $5A
        LIT 5, $A5
        LIT 6, $5B
        LIT 7, $3C
        LIT 8, $00                      ; GUEST: the table entry is an XOR mask of $00
        LIT 9, $C3
        LIT 10, $18
        LIT 11, $00
        LIT 12, $00
        LIT 13, $A1
        LIT 14, $90
        LIT 15, $5A
        LIT 16, $00
        LIT 17, $8F
        LIT 18, $90
        LIT 19, $07
        LIT 20, $F6
        LIT 21, $00
        LIT 22, $A1
        LIT 23, $B2
        LIT 24, $A1
        LIT 25, $00
        LIT 26, $00                     ; GUEST
        LIT 27, $C1
        LIT 28, $B2
        LIT 29, $FF
        LIT 30, $00
        LIT 31, $5A
        LIT 32, $C3
        LIT 33, $D4
        LIT 34, $E5
        LIT 35, $00                     ; GUEST
        LIT 36, $00                     ; GUEST (timer, E2)
        LIT 37, $00                     ; GUEST (timer, E2)
        LIT 38, $00                     ; GUEST (timer, E2)
        .ifdef LEN0
        LIT 39, $00                     ; the 65536th byte is at $FFFF and goes to the second cell: the first cell holds the byte at $FFFE
        LIT 40, $E0
        .else
        LIT 39, $E0                     ; three bytes: the last one is at $FFFF and goes to the first cell
        LIT 40, $00
        .endif
        LIT 41, $00                     ; GUEST (timer, E2)
        LIT 42, $00
        .if IRQ_ON
        LIT 43, $01                     ; the timer interrupt is taken once, after the epilogue
        LIT 44, $10                     ; 4096 = $1000 destination bytes were written when it was taken
        LIT 45, $01
        LIT 46, $12                     ; first the timer interrupt, then IRQ2
        LIT 47, $00
        LIT 48, $00
        LIT 49, $00
        LIT 50, $00
        .else
        LIT 43, $00                     ; I is set: nothing is taken
        LIT 44, $10                     ; the transfer still moved 4096 bytes (counted by the main program afterwards)
        LIT 45, $00
        LIT 46, $00
        LIT 47, $FF                     ; the handler cells keep their initial $FF: no handler ran
        LIT 48, $FF
        LIT 49, $00
        LIT 50, $04                     ; the timer borrow is latched (bit 2 of the request register) and masked by I
        .endif

SL0 = $01                       ; the TAM operands of MPR0, MPR3, MPR4, MPR5 and MPR6
SL3 = $08
SL4 = $10
SL5 = $20
SL6 = $40
TIM_RELOAD = $AC00              ; the timer window with MPR5 = $FF: the reload register (write) and the counter (read)
TIM_CTRL = $AC01                ; the control register (write, bit 0 starts and stops) and the counter (read)
IRQ_DIS = $B402                 ; $1402: the disable register (bit 0 IRQ2D, bit 1 IRQ1D, bit 2 TIQD)
IRQ_REQ = $B403                 ; $1403: the request register (bit 0 IRQ2, bit 1 IRQ1, bit 2 TIQ; a write clears TIQ)

A_PRE = $A5                     ; the register values every transfer must give back
X_PRE = $5B
Y_PRE = $3C
SENT = $5A                      ; the sentinel byte the destinations are filled with first

ARENA = $2800                   ; 512 bytes of work RAM for the small transfers
DLONG = $2910                   ; the two cells of the long transfer
DST4K = $2A00                   ; 4096 bytes of work RAM, the destination of the interrupt transfer
PTR = $20                       ; zero page pointer (logical $2020)

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
S0 = $2410                      ; S before a transfer
RA = $2411                      ; the registers and flags after a transfer
RX = $2412
RY = $2413
RP = $2414
RS = $2415
TMP = $2416
SSAVE = $2417
CNT = $2418                     ; the pages of DST4K completed, counted by count_pages
TIQ_RUNS = $241A
IRQ2_RUNS = $241B
LOGN = $241C
LOG = $241D                     ; two bytes: the order of the entries
TIQ_BYTES_HI = $241F            ; the pages of DST4K complete when the first handler ran (16 = $10 for all 4096 bytes)
TIQ_REGDIFF = $2420
TIQ_RETDIFF = $2421
H_A = $2422
H_X = $2423
H_Y = $2424
H_T = $2425
LATCH = $2426                   ; bit 2 of the request register after the transfer

CYC_STEP = 256                  ; CPU cycles per prescaler step at the low speed: 3072 master clocks over the community-reported 12 master clocks per cycle (ER-3): the rows that use it are E2 self-consistency
CYC_STEP_HI = 1024              ; at the high speed: 3072 over 3 master clocks per cycle (LED-CPU-0003)
LR = 100                        ; the reload of the long transfer's timer

; PREP pbyte: S0 = S, then A, X and Y are loaded and the flag byte is set last (PLP): the transfer that follows sees them
        .macro PREP pbyte
        tsx
        stx a:S0
        lda #(pbyte)
        pha
        ldx #X_PRE
        ldy #Y_PRE
        lda #A_PRE
        plp
        .endmacro

; OBS: the registers, the flag byte (B and T read as 1 and 0 through PHP) and S after a transfer: stores only, so nothing changes before PHP
        .macro OBS
        sta a:RA
        stx a:RX
        sty a:RY
        php
        pla
        sta a:RP
        tsx
        stx a:RS
        .endmacro

; REGDIFF: A = (RA xor A_PRE) or (RX xor X_PRE) or (RY xor Y_PRE) or (RS xor S0): 0 when the transfer gave every register and S back
        .macro REGDIFF
        lda a:RA
        eor #A_PRE
        sta a:TMP
        lda a:RX
        eor #X_PRE
        ora a:TMP
        sta a:TMP
        lda a:RY
        eor #Y_PRE
        ora a:TMP
        sta a:TMP
        lda a:RS
        eor a:S0
        ora a:TMP
        .endmacro

; DELAY k: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2): k from 1 to 255
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

; TIMED_TII nbytes, fast: the timer at reload 127 is started, then A, X and Y are loaded and a TII of nbytes bytes from the $FF bank to the arena runs, then the counter is read (A = the counter, stored in TMP) and the timer stopped. With fast = 1 the speed is high from before the start
; to after the read. From the start access (the last cycle of the STA) to the read (the last cycle of the LDA abs) it is 1 + (2 + 2 + 2 + 17 + 6x) + 4 = 28 + 6x CPU cycles
        .macro TIMED_TII nbytes, fast
        lda #127
        sta a:TIM_RELOAD
        .if (fast) <> 0
        csh
        .endif
        lda #$01
        sta a:TIM_CTRL
        lda #A_PRE
        ldx #X_PRE
        ldy #Y_PRE
        tii $6000, ARENA, (nbytes)
        lda a:TIM_CTRL
        .if (fast) <> 0
        csl
        .endif
        sta a:TMP
        stz a:TIM_CTRL
        .endmacro

fixture_main:
        lda #$F8
        tam #SL0                        ; MPR0 = work RAM: logical $0000-$1FFF
        lda #$01
        tam #SL3                        ; MPR3 = HuCard bank 1 (all $FF): logical $6000
        lda #$02
        tam #SL4
        lda #$FF
        tam #SL5                        ; MPR5 = the hardware page: logical $A000
        lda #$04
        tam #SL6
        lda #<timer_h
        sta a:VEC_TIMER                 ; the TIMER vector jumps through this slot (startup.inc)
        lda #>timer_h
        sta a:VEC_TIMER+1
        lda #<irq2_h
        sta a:VEC_IRQ2                  ; IRQ2 and BRK
        lda #>irq2_h
        sta a:VEC_IRQ2+1
        stz a:TIM_CTRL                  ; the timer is stopped and the interrupt registers are clear before anything uses them
        stz a:IRQ_DIS
        sta a:IRQ_REQ

; ---- A: TII of 16 bytes, the registers, S and the flags (N V Z C set before it) ----
        lda #SENT
        jsr fill_arena
        PREP $C7
        tii PAT, ARENA+$10, 16
        OBS
        lda a:ARENA+$10
        CHECK_LIT 1                     ; check 1 MAN:H8-6: the first byte goes to the first destination address (post-increment source and destination)
        lda a:ARENA+$1F
        CHECK_LIT 2                     ; check 2 MAN:H8-6: the sixteenth byte is the last one moved
        lda a:ARENA+$20
        CHECK_LIT 3                     ; check 3 MAN:H8-6: exactly 16 bytes move: the byte after the block is untouched
        lda a:ARENA+$0F
        CHECK_LIT 4                     ; check 4 MAN:H8-6: the byte before the block is untouched
        lda a:RA
        CHECK_LIT 5                     ; check 5 MAN:H8-6: A is returned from the stack
        lda a:RX
        CHECK_LIT 6                     ; check 6 MAN:H8-6: X is returned from the stack
        lda a:RY
        CHECK_LIT 7                     ; check 7 MAN:H8-6: Y is returned from the stack
        lda a:S0
        sta a:GEXP
        lda a:RS
        CHECK_GUEST 8                   ; check 8 GUEST:stack-pointer: S after the transfer is S before it (three bytes of stack are used and given back)
        lda a:RP
        and #$C3
        CHECK_LIT 9                     ; check 9 MAN:S8-89: N, V, Z and C are not changed by the transfer (the flag row has only T)

; ---- B: TDD shifts a block up by one byte (post-decrement, high address first) ----
        lda #SENT
        jsr fill_arena
        jsr init_r
        PREP $04
        tdd ARENA+$47, ARENA+$48, 8
        OBS
        lda a:ARENA+$48
        CHECK_LIT 10                    ; check 10 MAN:H8-6: the byte at R+7 moved to R+8: the transfer runs downwards
        lda a:RP
        and #$C3
        CHECK_LIT 11                    ; check 11 MAN:S8-89: N, V, Z and C stay clear
        REGDIFF
        CHECK_LIT 12                    ; check 12 MAN:H8-6: A, X, Y and S are given back by TDD

; ---- C: TII with overlapping blocks copies one byte at a time ----
        lda #SENT
        jsr fill_arena
        jsr init_r
        PREP $04
        tii ARENA+$40, ARENA+$41, 8
        OBS
        lda a:ARENA+$48
        CHECK_LIT 13                    ; check 13 ASSUMED:LED-CPU-0008: the first byte spreads over the whole block (one byte per six bus cycles, the source is read again after each write)

; ---- D: TIN, the destination stays fixed ----
        lda #SENT
        jsr fill_arena
        PREP $04
        tin PAT, ARENA+$80, 16
        OBS
        lda a:ARENA+$80
        CHECK_LIT 14                    ; check 14 MAN:H8-6: sixteen bytes went to one address: it holds the last one
        lda a:ARENA+$81
        CHECK_LIT 15                    ; check 15 MAN:H8-6: the destination address is fixed: its neighbour is untouched
        REGDIFF
        CHECK_LIT 16                    ; check 16 MAN:H8-6: A, X, Y and S are given back by TIN

; ---- E: TIA, the destination alternates (post-increment first, then post-decrement) ----
        lda #SENT
        jsr fill_arena
        PREP $04
        tia PAT, ARENA+$80, 16
        OBS
        lda a:ARENA+$80
        CHECK_LIT 17                    ; check 17 MAN:H8-6: even length: the first destination cell holds the byte before the last
        lda a:ARENA+$81
        CHECK_LIT 18                    ; check 18 MAN:H8-6: and the second cell holds the last byte
        lda #SENT
        jsr fill_arena
        PREP $04
        tia PAT, ARENA+$80, 7
        OBS
        lda a:ARENA+$80
        CHECK_LIT 19                    ; check 19 MAN:S8-88: odd length: the first cell holds the last byte (the alternation state is kept across bytes)
        lda a:ARENA+$81
        CHECK_LIT 20                    ; check 20 MAN:S8-88: the second cell holds the byte before the last
        REGDIFF
        CHECK_LIT 21                    ; check 21 MAN:H8-6: A, X, Y and S are given back by TIA

; ---- F: TAI, the source alternates ----
        lda #SENT
        jsr fill_arena
        PREP $04
        tai PAT, ARENA+$A0, 8
        OBS
        lda a:ARENA+$A2
        CHECK_LIT 22                    ; check 22 MAN:S8-88: the source address goes up and then down again: the third byte moved is the first source byte
        lda a:ARENA+$A7
        CHECK_LIT 23                    ; check 23 MAN:S8-88: the eighth byte moved is the second source byte
        lda #SENT
        jsr fill_arena
        PREP $04
        tai PAT, ARENA+$A0, 7
        OBS
        lda a:ARENA+$A6
        CHECK_LIT 24                    ; check 24 MAN:S8-88: odd length: the seventh byte moved is the first source byte
        REGDIFF
        CHECK_LIT 25                    ; check 25 MAN:H8-6: A, X, Y and S are given back by TAI

; ---- G: the 16-bit addresses wrap, lengths above 256 ----
        lda #$C1
        sta a:$0000                     ; work RAM at logical $0000 (MPR0): the bytes after $FFFF
        lda #$D2
        sta a:$0001
        lda #SENT
        jsr fill_arena
        PREP $04
        tii $FFFE, ARENA, 4
        OBS
        lda a:$FFFF
        sta a:GEXP
        lda a:ARENA+1
        CHECK_GUEST 26                  ; check 26 GUEST:vector-byte: the source address $FFFE then $FFFF is read from the HuCard vectors: the second byte moved is the byte at $FFFF
        lda a:ARENA+2
        CHECK_LIT 27                    ; check 27 ASSUMED:LED-CPU-0008: the 16-bit source address wraps from $FFFF to $0000: the third byte moved is the byte at $0000
        PREP $04
        tii PAT, $FFFF, 3
        OBS
        lda a:$0000
        CHECK_LIT 28                    ; check 28 ASSUMED:LED-CPU-0008: the 16-bit destination address wraps from $FFFF to $0000: the second byte moved lands at $0000
        lda #$00
        jsr fill_arena2
        PREP $04
        tii $6000, ARENA, $0101
        OBS
        lda a:ARENA+$100
        CHECK_LIT 29                    ; check 29 MAN:H8-6: the length is 16 bits: a length of 257 moves the 257th byte
        lda a:ARENA+$101
        CHECK_LIT 30                    ; check 30 MAN:H8-6: and no byte more
        lda #SENT
        jsr fill_arena
        PREP $04
        tii PAT, ARENA, 1
        OBS
        lda a:ARENA+1
        CHECK_LIT 31                    ; check 31 MAN:H8-6: a length of 1 moves one byte

; ---- H: a destination on the stack: the epilogue pops what the transfer wrote there ----
        tsx
        stx a:SSAVE
        ldx #$90
        txs                             ; S = $90: Y is saved at $2190, A at $218F and X at $218E (S8-88: Y, A, X, one level each)
        PREP $04
        tii PAT, $218C, 6
        OBS
        ldx a:SSAVE
        txs
        lda a:RX
        CHECK_LIT 32                    ; check 32 ASSUMED:LED-CPU-0008: X is popped from memory: the third byte moved overwrote its saved copy
        lda a:RA
        CHECK_LIT 33                    ; check 33 ASSUMED:LED-CPU-0008: A is popped from memory: the fourth byte moved overwrote its saved copy
        lda a:RY
        CHECK_LIT 34                    ; check 34 ASSUMED:LED-CPU-0008: Y is popped from memory: the fifth byte moved overwrote its saved copy
        lda a:S0
        sta a:GEXP
        lda a:RS
        CHECK_GUEST 35                  ; check 35 GUEST:stack-pointer: S is back where it was whatever the transfer wrote

; ---- I: the cycle count, read from the timer (E2) ----
        TIMED_TII 935, 0
        lda #(127 - (28 + 6 * 935) / CYC_STEP)
        sta a:GEXP
        lda a:TMP
        CHECK_GUEST 36                  ; check 36 GUEST:timer-e2: 935 bytes, 5638 cycles from the start access to the read, 6 past the 22nd step: 17 + 6x cycles, a transfer that is 7 cycles short reads one step more
        TIMED_TII 933, 0
        lda #(127 - (28 + 6 * 933) / CYC_STEP)
        sta a:GEXP
        lda a:TMP
        CHECK_GUEST 37                  ; check 37 GUEST:timer-e2: 933 bytes, 5626 cycles, 6 before the 22nd step: a transfer that is 7 cycles long reads one step less
        TIMED_TII 679, 1
        lda #(127 - ((28 + 6 * 679) / CYC_STEP_HI))
        sta a:GEXP
        lda a:TMP
        CHECK_GUEST 38                  ; check 38 GUEST:timer-e2: at the high speed each step of the transfer takes 3 clocks per cycle: 4102 cycles, 4 steps of 1024 and 6 more

; ---- L: the long transfer: a TIA whose last source byte is the one at $FFFF (65536 bytes in the len0 image) ----
        lda #SENT
        sta a:DLONG
        sta a:DLONG+1
        tsx
        stx a:S0
        lda #LR
        sta a:TIM_RELOAD
        lda #$01
        sta a:TIM_CTRL                  ; start
        lda #$03
        tam #SL5                        ; MPR5 = a $FF bank: the 64 KiB source walk must not find the hardware page
        ldx #X_PRE
        ldy #Y_PRE
        lda #A_PRE
        tia LONG_SRC, DLONG, LONG_OP
        sta a:RA
        stx a:RX
        sty a:RY
        tsx
        stx a:RS
        lda #$FF
        tam #SL5                        ; the hardware page again
        lda a:TIM_CTRL                  ; read at 1 + (7 + 6 + (17 + 6x) + 15 + 7 + 7) + 4 = 64 + 6x cycles from the start access
        sta a:TMP
        stz a:TIM_CTRL
        lda a:DLONG
        CHECK_LIT 39                    ; check 39 MAN:H8-6: the cell the destination returns to holds the byte at $FFFF after 3 bytes and the byte at $FFFE after 65536
        lda a:DLONG+1
        CHECK_LIT 40                    ; check 40 MAN:H8-6: and the other cell holds the byte at $FFFE after 3 bytes and the byte at $FFFF after 65536
        lda #(LR - ((64 + 6 * LONG_BYTES) / CYC_STEP) .mod (LR + 1))
        sta a:GEXP
        lda a:TMP
        CHECK_GUEST 41                  ; check 41 GUEST:timer-e2: the counter after the long transfer: 17 + 6x cycles, so every byte costs six (one cycle more per byte would be 256 steps more in the len0 image)
        REGDIFF
        CHECK_LIT 42                    ; check 42 MAN:H8-6: A, X, Y and S are given back by the long transfer

; ---- the interrupt around a 4096-byte transfer ----
        jsr prep_dst
        stz a:TIQ_RUNS
        stz a:IRQ2_RUNS
        stz a:LOGN
        stz a:LOG
        stz a:LOG+1
        stz a:TIQ_BYTES_HI
        lda #$FF
        sta a:TIQ_REGDIFF
        sta a:TIQ_RETDIFF
        stz a:TIM_CTRL
        stz a:IRQ_DIS
        sta a:IRQ_REQ                   ; any write acknowledges TIQ
        .if IRQ_ON
        lda #$02
        sta a:IRQ_DIS                   ; IRQ1D: the IRQ1 level of the case is only the synchronisation line, never delivered
        cli
        jsr wait_irq1                   ; the case declares IRQ1 high, then IRQ2 high a little later: the transfer starts just after the poll sees IRQ1
        .endif
        tsx
        stx a:S0
        lda #$00
        sta a:TIM_RELOAD
        lda #$01
        sta a:TIM_CTRL                  ; reload 0: a borrow every prescaler step, TIQ latched from the first one
        ldx #X_PRE
        ldy #Y_PRE
        lda #A_PRE
        tii $6000, DST4K, $1000
after_tii:
        sta a:RA
        stx a:RX
        sty a:RY
        tsx
        stx a:RS
        .if IRQ_ON
        DELAY 30                        ; the pending IRQ2 is taken after the next instruction, and the handler is short
        .endif
        sei
        lda a:IRQ_REQ
        and #$04
        sta a:LATCH
        .if IRQ_ON = 0
        jsr count_pages                 ; no handler ran: the main program counts what the transfer wrote
        lda a:CNT
        sta a:TIQ_BYTES_HI
        .endif
        stz a:TIM_CTRL
        sta a:IRQ_REQ
        stz a:IRQ_DIS
        lda a:TIQ_RUNS
        CHECK_LIT 43                    ; check 43 ASSUMED:LED-IRQ-0007: the timer interrupt is taken once (the borrows during the transfer coalesce in one latch); with I set it is not taken
        lda a:TIQ_BYTES_HI
        CHECK_LIT 44                    ; check 44 ASSUMED:LED-IRQ-0007: when it is taken, all 4096 destination bytes are written (16 = $10 pages complete): no interrupt is sampled inside the transfer
        lda a:IRQ2_RUNS
        CHECK_LIT 45                    ; check 45 ASSUMED:LED-IRQ-0007: the IRQ2 handler runs once, after the timer interrupt (IRQ2 rose inside the transfer and is still high)
        lda a:LOG
        asl a
        asl a
        asl a
        asl a
        ora a:LOG+1
        CHECK_LIT 46                    ; check 46 ASSUMED:LED-IRQ-0007: the order of the entries is the timer interrupt, then IRQ2 (the priority of H8-13 applies when both are pending at the end of the transfer)
        lda a:TIQ_REGDIFF
        CHECK_LIT 47                    ; check 47 ASSUMED:LED-IRQ-0007: at the entry of the timer handler A, X and Y already hold their values from before the transfer (the epilogue ran first)
        lda a:TIQ_RETDIFF
        CHECK_LIT 48                    ; check 48 ASSUMED:LED-IRQ-0007: the return address the entry pushed is the instruction after the transfer
        REGDIFF
        CHECK_LIT 49                    ; check 49 MAN:H8-6: A, X, Y and S read back equal to their values before the transfer after the handlers returned
        lda a:LATCH
        CHECK_LIT 50                    ; check 50 MAN:H8-15: the request register shows TIQ while it is masked by I and 0 once the handler acknowledged it

        rts

; ---- subroutines ----

; fill_arena: A = value: the 256 bytes at ARENA (the small transfers stay inside this page)
fill_arena:
        ldx #$00
fa_l:   sta a:ARENA,x
        inx
        bne fa_l
        rts

; fill_arena2: A = value: the 512 bytes at ARENA (the length 257 transfer crosses into the second page)
fill_arena2:
        ldx #$00
fb_l:   sta a:ARENA,x
        sta a:ARENA+$100,x
        inx
        bne fb_l
        rts

; prep_dst: the last byte of each of the 16 pages of DST4K is cleared, so that count_pages sees which pages the transfer has written (the transfer moves $FF bytes in increasing address order)
prep_dst:
        stz z:PTR
        lda #>DST4K
        sta z:PTR+1
        ldx #16
        ldy #$FF
pd_p:   lda #$00
        sta (PTR),y
        inc z:PTR+1
        dex
        bne pd_p
        rts

; count_pages: CNT = the number of pages of DST4K (0 to 16) whose last byte holds $FF: with the destination written upwards that is the number of 256-byte pages the transfer has completed, 16 = $10 for all 4096 bytes
count_pages:
        stz a:CNT
        stz z:PTR
        lda #>DST4K
        sta z:PTR+1
        ldx #16
        ldy #$FF
cp_p:   lda (PTR),y
        cmp #$FF
        bne cp_n
        inc a:CNT
cp_n:   inc z:PTR+1
        dex
        bne cp_p
        rts

; init_r: R = ARENA+$40 holds PAT[0] to PAT[8]
init_r:
        ldx #$08
ir_l:   lda a:PAT,x
        sta a:ARENA+$40,x
        dex
        bpl ir_l
        rts

; wait_irq1: poll the request register until the IRQ1 level reads high (bit 1), at most 24576 rounds of about 19 cycles; a machine with no stimulus fails a check, it does not hang
wait_irq1:
        ldy #$60
wi_o:   ldx #$00
wi_i:   lda a:IRQ_REQ
        and #$02
        bne wi_end
        dex
        bne wi_i
        dey
        bne wi_o
wi_end: rts

; the timer handler: records A, X, Y at its entry, the pushed return address and how many destination bytes were already written, then stops and acknowledges the timer
timer_h:
        sta a:H_A
        stx a:H_X
        sty a:H_Y
        tsx                             ; X = S after the entry's three pushes: the pushed return address is at $2100 + X + 2 (low) and + 3 (high)
        lda a:$2102,x
        eor #<after_tii
        sta a:H_T
        lda a:$2103,x
        eor #>after_tii
        ora a:H_T
        sta a:TIQ_RETDIFF
        lda a:H_A
        eor #A_PRE
        sta a:H_T
        lda a:H_X
        eor #X_PRE
        ora a:H_T
        sta a:H_T
        lda a:H_Y
        eor #Y_PRE
        ora a:H_T
        sta a:TIQ_REGDIFF
        jsr count_pages
        lda a:CNT
        sta a:TIQ_BYTES_HI
        stz a:TIM_CTRL
        sta a:IRQ_REQ
        ldx a:LOGN
        lda #$01
        sta a:LOG,x
        inc a:LOGN
        inc a:TIQ_RUNS
        lda a:H_A
        ldx a:H_X
        ldy a:H_Y
        rti

; the IRQ2 handler (the vector $FFF6 is shared with BRK): masks IRQ2 (the level stays high), counts and logs, gives the registers back
irq2_h:
        pha
        phx
        phy
        lda #$03
        sta a:IRQ_DIS                   ; IRQ2D and IRQ1D
        ldx a:LOGN
        lda #$02
        sta a:LOG,x
        inc a:LOGN
        inc a:IRQ2_RUNS
        ply
        plx
        pla
        rti

; the 32-byte source pattern: no byte equals the sentinel $5A or one of A_PRE, X_PRE and Y_PRE
PAT:    .byte $A1, $B2, $C3, $D4, $E5, $F6, $07, $18, $29, $3A, $4B, $5C, $6D, $7E, $8F, $90
        .byte $01, $12, $23, $34, $45, $56, $67, $78, $89, $9A, $AB, $BC, $CD, $DE, $EF, $F0
