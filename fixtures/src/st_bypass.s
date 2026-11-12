; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; st_bypass.s - the ST0, ST1 and ST2 bypass fixture (plan 03-20; D-106, D-124, D-125, ER-7, H8-8 Table 2-2-3, S8-80, S8-81, S8-82, H8-24). It is assembled by ca65 in the gate and never committed as a binary.
; The three immediate stores go to the VDC chip enable with the register address bits A1 and A0 forced by the instruction (ST0 00, ST1 10, ST2 11), whatever the mapping registers hold (H8-8 Table 2-2-3: A20 high in the write cycle, a special code on A0 and A1).
; The fixture executes the three stores four times, each time with MPR0 (logical $0000-$1FFF, the addresses an emulator that treated the store as a write to logical $0000, $0002 or $0003 would use) mapped to something else:
;   setting 1, MPR0 = $F8: work RAM, so such an emulator would change RAM cells 0, 2 and 3.   setting 2, MPR0 = $01: HuCard bank 1.   setting 3, MPR0 = $80: a bank that maps nothing.   setting 4, MPR0 = $FF: the hardware page itself.
; The immediates are distinct in every store ($A1 to $A3, $B1 to $B3, $C1 to $C3, $D1 to $D3): the runner kind st_bypass compares the device-register log of the run with exactly these twelve records, in this order (physical $1FE000, $1FE002 and $1FE003 with the immediate),
; and their literals are the immediates of this file, repeated in the case; none comes from DuoSpool output. What the guest itself checks, the five checks below, is that no store reached the work RAM: the three cells RAM[0], RAM[2] and RAM[3] (also read at logical $2000 through MPR1, which
; never moves) keep the sentinels the guest wrote, in the four settings, and, in setting 1, that logical $0000 through MPR0 still reads the first sentinel (D-106). The class is GUEST (the expected byte is the sum of the sentinels, kept in GEXP as they are written).
; Nothing else touches the hardware page, so the dev log holds the twelve ST records and no other. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 9
DSV_CHECKS = 5

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                      ; GUEST: the table entry is an XOR mask of $00
        LIT 2, $00
        LIT 3, $00
        LIT 4, $00
        LIT 5, $00

SENT0 = $5A                     ; the sentinels the guest writes into RAM[0], RAM[2] and RAM[3] (logical $2000, $2002, $2003 through MPR1, which never moves)
SENT2 = $5B
SENT3 = $5C
Z0 = $2000
Z2 = $2002
Z3 = $2003

; SENTINELS: write the three sentinels into the work RAM (through MPR1, which never moves)
        .macro SENTINELS
        lda #SENT0
        sta a:Z0
        lda #SENT2
        sta a:Z2
        lda #SENT3
        sta a:Z3
        .endmacro

; MODELSUM: GEXP = the sum of the three sentinels, computed here from the same constants (the guest-computed model of the sum checks)
        .macro MODELSUM
        lda #SENT0
        clc
        adc #SENT2
        clc
        adc #SENT3
        sta a:GEXP
        .endmacro

; RAMSUM: A = RAM[0] + RAM[2] + RAM[3] as the three cells read now
        .macro RAMSUM
        lda a:Z0
        clc
        adc a:Z2
        clc
        adc a:Z3
        .endmacro

fixture_main:
; ---- setting 1, MPR0 = $F8 (work RAM): the stores must not change the RAM the bus would reach through logical $0000, $0002 and $0003 ----
        SENTINELS
        lda #$F8
        tam #$01                        ; MPR0 = $F8
        st0 #$A1                        ; physical $1FE000 <- $A1
        lda #SENT0
        sta a:GEXP                      ; the model of check 1: RAM[0] still holds its sentinel
        lda a:$0000                     ; RAM[0] read through MPR0 itself
        CHECK_GUEST 1                   ; check 1 GUEST:ram-before-equals-ram-after: ST0 with MPR0 mapped to work RAM did not change RAM[0] (D-106; H8-8 Table 2-2-3, S8-80)
        st1 #$A2                        ; physical $1FE002 <- $A2
        st2 #$A3                        ; physical $1FE003 <- $A3
        MODELSUM
        RAMSUM
        CHECK_GUEST 2                   ; check 2 GUEST:ram-before-equals-ram-after: the three stores left RAM[0], RAM[2] and RAM[3] as written (setting 1, MPR0 = $F8)

; ---- setting 2, MPR0 = $01 (HuCard bank 1) ----
        SENTINELS
        lda #$01
        tam #$01                        ; MPR0 = $01
        st0 #$B1
        st1 #$B2
        st2 #$B3
        MODELSUM
        RAMSUM
        CHECK_GUEST 3                   ; check 3 GUEST:ram-before-equals-ram-after: the stores did not reach the work RAM with MPR0 mapped to a HuCard bank

; ---- setting 3, MPR0 = $80 (a bank that maps nothing) ----
        SENTINELS
        lda #$80
        tam #$01                        ; MPR0 = $80
        st0 #$C1
        st1 #$C2
        st2 #$C3
        MODELSUM
        RAMSUM
        CHECK_GUEST 4                   ; check 4 GUEST:ram-before-equals-ram-after: the stores did not reach the work RAM with MPR0 mapped to a bank that maps nothing

; ---- setting 4, MPR0 = $FF (the hardware page: logical $0000 to $0003 are the VDC window itself) ----
        SENTINELS
        lda #$FF
        tam #$01                        ; MPR0 = $FF
        st0 #$D1
        st1 #$D2
        st2 #$D3
        MODELSUM
        RAMSUM
        CHECK_GUEST 5                   ; check 5 GUEST:ram-before-equals-ram-after: the stores did not reach the work RAM with MPR0 mapped to the hardware page
        rts
