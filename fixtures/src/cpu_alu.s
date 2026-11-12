; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_alu.s - the arithmetic, logic, shift, compare and bit-test group with decimal mode and the T flag (plan 03-12; D-98, D-99, D-125, D-127, S8-17, S8-18, S8-19, S8-25, S8-41 to S8-50, S8-56, S8-58, S8-68, S8-69, S8-74, S8-78, S8-102 to S8-104). It is assembled by ca65 in the gate
; and never committed as a binary. Checks run in ladder order: the logic rungs first (AND, ORA, EOR with literal bytes), then the flag rows of the logic group, then binary ADC and SBC against an in-guest model, decimal mode on valid BCD, the T-mode forms,
; the shifts against an in-guest model, INC, DEC, the compares and last the bit tests. The in-guest models are built only from the logic instructions verified before them, a table of the eight bit masks and branches (no adder and no shifter is used to model an adder or a shifter):
; model_add is a bit-serial full adder, model_shl and model_shr move one bit at a time through the mask table. An aggregated model check runs 32 sums (16 edge pairs with C clear and set) or 32 shifts (16 edge bytes with C clear and set) and expects a difference byte of zero.
; Decimal mode is exercised on valid BCD only, and D is cleared before the check macro runs (the tally adds in binary). The decimal flags N, Z and C, the decimal borrow value, T mode with decimal mode, TSB and TRB Z and the TRB stored result are ASSUMED class (LED-CPU-0010);
; invalid BCD is not exercised. N and V of TSB and TRB are not asserted (the two sources disagree, research mismatch 9). P is read as PHP, PLA and a mask: PHP pushes B as 1 and T as 0 (H8-5), so the mask keeps N V Z and C only. Memory is filled with $FF first
; and every cell a check reads is written by the fixture before (D-127). This file is generated from a table of checks by a scratch script; the committed text is the source of record.
        .setcpu "huc6280"

DSV_ID = 4
DSV_CHECKS = 96

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

; the cells of the model (work RAM $2410-$242F, outside the verdict block, the stack and the filled pages)
MA      = $2410
MB      = $2411
MC      = $2412
MM      = $2413
MRES    = $2414
MCUR    = $2415
MMASK   = $2416
MAI     = $2417
MBI     = $2418
MCI     = $2419
MXOR    = $241A
MT      = $241B
MCIN7   = $241C
MCO     = $241D
MV      = $241E
MFILL   = $241F
MR      = $2420
RFL     = $2421
GFL     = $2422
MDR     = $2423
MDF     = $2424
KIND    = $2425
; the cells of the checks
ZP_A    = $10                   ; zero page: $2010 holds $01 and $2013 holds $02
ZP_PTR  = $30                   ; a zero-page pointer to RAM_P0
ZP_PTR2 = $34                   ; a zero-page pointer to RAM_P1 (reached through (zp,X) with X = 4)
ZP_W    = $50                   ; a work cell of the read-modify-write checks (written by each check before it is read)
RAM_P0  = $2600                 ; the target of (zp) ($03) and the base of (zp),Y ($2603 holds $05)
RAM_P1  = $2601                 ; the target of (zp,X) ($04)
RAM_ABS = $2500                 ; the base of the absolute and the absolute indexed reads: $06, $07 at +2, $08 at +5
RAM_W   = $2700                 ; the absolute work cells of the read-modify-write checks

; P is set from a byte by PLP (the I bit is kept set); P is read as a masked PHP byte
        .macro SETP value
        lda #((value) | $04)
        pha
        plp
        .endmacro
        .macro PMASK mask
        php
        pla
        and #(mask)
        .endmacro

        CHECK_TABLE DSV_CHECKS

        LIT 1, $30
        LIT 2, $3C
        LIT 3, $F0
        LIT 4, $C1
        LIT 5, $43
        LIT 6, $C1
        LIT 7, $43
        LIT 8, $01
        LIT 9, $25
        LIT 10, $F8
        LIT 11, $30
        LIT 12, $31
        LIT 13, $03
        LIT 14, $C0
        LIT 15, $43
        LIT 16, $00
        LIT 17, $00
        LIT 18, $12
        LIT 19, $13
        LIT 20, $15
        LIT 21, $17
        LIT 22, $18
        LIT 23, $19
        LIT 24, $30
        LIT 25, $2F
        LIT 26, $80
        LIT 27, $41
        LIT 28, $00
        LIT 29, $00
        LIT 30, $46
        LIT 31, $00
        LIT 32, $01
        LIT 33, $02
        LIT 34, $40
        LIT 35, $34
        LIT 36, $39
        LIT 37, $33
        LIT 38, $78
        LIT 39, $00
        LIT 40, $00
        LIT 41, $30
        LIT 42, $77
        LIT 43, $03
        LIT 44, $30
        LIT 45, $3C
        LIT 46, $F0
        LIT 47, $42
        LIT 48, $20
        LIT 49, $00
        LIT 50, $00
        LIT 51, $02
        LIT 52, $00
        LIT 53, $00
        LIT 54, $01
        LIT 55, $01
        LIT 56, $00
        LIT 57, $00
        LIT 58, $81
        LIT 59, $81
        LIT 60, $00
        LIT 61, $00
        LIT 62, $81
        LIT 63, $81
        LIT 64, $42
        LIT 65, $03
        LIT 66, $41
        LIT 67, $81
        LIT 68, $80
        LIT 69, $00
        LIT 70, $7F
        LIT 71, $11
        LIT 72, $03
        LIT 73, $7F
        LIT 74, $81
        LIT 75, $00
        LIT 76, $41
        LIT 77, $43
        LIT 78, $C0
        LIT 79, $03
        LIT 80, $01
        LIT 81, $80
        LIT 82, $03
        LIT 83, $80
        LIT 84, $03
        LIT 85, $C2
        LIT 86, $42
        LIT 87, $80
        LIT 88, $3F
        LIT 89, $83
        LIT 90, $02
        LIT 91, $00
        LIT 92, $F0
        LIT 93, $02
        LIT 94, $C2
        LIT 95, $80
        LIT 96, $40

fixture_main:
        ldx #$00                ; the pages the checks read are filled with $FF, then every cell that a check reads is written
        lda #$FF
fill:   sta a:$2000,x
        sta a:RAM_ABS,x
        sta a:RAM_P0,x
        sta a:RAM_W,x
        inx
        bne fill
        lda #$01
        sta a:$2000+ZP_A
        lda #$02
        sta a:$2000+ZP_A+3
        lda #<RAM_P0
        sta a:$2000+ZP_PTR
        lda #>RAM_P0
        sta a:$2000+ZP_PTR+1
        lda #<RAM_P1
        sta a:$2000+ZP_PTR2
        lda #>RAM_P1
        sta a:$2000+ZP_PTR2+1
        lda #$03
        sta a:RAM_P0
        lda #$04
        sta a:RAM_P1
        lda #$05
        sta a:RAM_P0+3
        lda #$06
        sta a:RAM_ABS
        lda #$07
        sta a:RAM_ABS+2
        lda #$08
        sta a:RAM_ABS+5
        lda #$F0                ; check 1 and-imm MAN:S8-18: AND immediate: A = $F0 AND $3C gives $30
        and #$3C
        CHECK_LIT 1
        lda #$30                ; check 2 ora-imm MAN:S8-58: ORA immediate: A = $30 OR $0C gives $3C
        ora #$0C
        CHECK_LIT 2
        lda #$FF                ; check 3 eor-imm MAN:S8-47: EOR immediate: A = $FF EOR $0F gives $F0
        eor #$0F
        CHECK_LIT 3
        SETP $41                ; check 4 and-flag-n MAN:S8-18: AND sets N from the result and leaves V and C alone: V and C set by PLP, $FF AND $80 gives N, so the P byte masked with $C3 is $C1
        lda #$FF
        and #$80
        PMASK $C3
        CHECK_LIT 4
        SETP $41                ; check 5 and-flag-z MAN:S8-18: AND sets Z from the result and leaves V and C alone: $0F AND $F0 gives zero, so the P byte masked with $C3 is $43 (V, Z, C)
        lda #$0F
        and #$F0
        PMASK $C3
        CHECK_LIT 5
        SETP $41                ; check 6 ora-flag-n MAN:S8-58: ORA sets N from the result and leaves V and C alone: $00 OR $80, V and C set, masked P byte $C1
        lda #$00
        ora #$80
        PMASK $C3
        CHECK_LIT 6
        SETP $41                ; check 7 eor-flag-z MAN:S8-47: EOR sets Z from the result and leaves V and C alone: $80 EOR $80 gives zero, V and C set, masked P byte $43
        lda #$80
        eor #$80
        PMASK $C3
        CHECK_LIT 7
        lda #$0F                ; check 8 and-zp MAN:S8-18: AND zero page: $0F AND the cell $2010 ($01) gives $01
        and z:ZP_A
        CHECK_LIT 8
        ldy #$03                ; check 9 ora-indy MAN:S8-58: ORA (zz),Y: $20 OR the byte at $2603 ($05) gives $25 (Y = 3)
        lda #$20
        ora (<ZP_PTR),y
        CHECK_LIT 9
        ldx #$02                ; check 10 eor-absx MAN:S8-47: EOR absolute,X: $FF EOR the byte at $2502 ($07) gives $F8 (X = 2)
        lda #$FF
        eor a:RAM_ABS,x
        CHECK_LIT 10
        clc                     ; check 11 adc-imm MAN:S8-17: ADC immediate with C clear: $10 + $20 gives $30
        lda #$10
        adc #$20
        CHECK_LIT 11
        sec                     ; check 12 adc-carry-in MAN:S8-17: ADC adds the carry: $10 + $20 + C = $31
        lda #$10
        adc #$20
        CHECK_LIT 12
        clc                     ; check 13 adc-carry-out MAN:S8-17: ADC sets C and Z for $FF + $01 = $100: masked P byte $03 (N and V clear)
        lda #$FF
        adc #$01
        PMASK $C3
        CHECK_LIT 13
        clc                     ; check 14 adc-overflow-pos MAN:S8-17: ADC sets V and N for $7F + $01 = $80 (two positives give a negative): masked P byte $C0
        lda #$7F
        adc #$01
        PMASK $C3
        CHECK_LIT 14
        clc                     ; check 15 adc-overflow-neg MAN:S8-17: ADC of $80 + $80 = $100 sets V, Z and C (two negatives give a positive): masked P byte $43
        lda #$80
        adc #$80
        PMASK $C3
        CHECK_LIT 15
        stz a:KIND              ; check 16 adc-model-result GUEST:adc-binary: ADC against the in-guest bit-serial adder over 16 edge pairs with C clear and set: the OR of every result difference is zero
        jsr agg_addsub
        stz a:GEXP
        lda a:MDR
        CHECK_GUEST 16
        stz a:GEXP              ; check 17 adc-model-flags GUEST:adc-binary: ADC N V Z C against the in-guest adder over the same 32 sums: the OR of every flag difference is zero
        lda a:MDF
        CHECK_GUEST 17
        sec                     ; check 18 adc-zp MAN:S8-17: ADC zero page with C set: $10 + the cell value $01 + 1 gives $12
        lda #$10
        adc z:ZP_A
        CHECK_LIT 18
        sec                     ; check 19 adc-zpx MAN:S8-17: ADC zero page,X (X = 3) with C set: $10 + the cell value $02 + 1 gives $13
        ldx #$03
        lda #$10
        adc z:ZP_A,x
        CHECK_LIT 19
        sec                     ; check 20 adc-indx MAN:S8-17: ADC (zz,X) (X = 4) with C set: $10 + the cell value $04 + 1 gives $15
        ldx #$04
        lda #$10
        adc (<ZP_PTR,x)
        CHECK_LIT 20
        sec                     ; check 21 adc-abs MAN:S8-17: ADC absolute with C set: $10 + the cell value $06 + 1 gives $17
        lda #$10
        adc a:RAM_ABS
        CHECK_LIT 21
        sec                     ; check 22 adc-absx MAN:S8-17: ADC absolute,X (X = 2) with C set: $10 + the cell value $07 + 1 gives $18
        ldx #$02
        lda #$10
        adc a:RAM_ABS,x
        CHECK_LIT 22
        sec                     ; check 23 adc-absy MAN:S8-17: ADC absolute,Y (Y = 5) with C set: $10 + the cell value $08 + 1 gives $19
        ldy #$05
        lda #$10
        adc a:RAM_ABS,y
        CHECK_LIT 23
        sec                     ; check 24 sbc-imm MAN:S8-74: SBC immediate with C set (no borrow): $50 - $20 - 0 = $30
        lda #$50
        sbc #$20
        CHECK_LIT 24
        clc                     ; check 25 sbc-borrow-in MAN:S8-74: SBC subtracts the complement of C (A - M - not C): C clear gives $50 - $20 - 1 = $2F
        lda #$50
        sbc #$20
        CHECK_LIT 25
        sec                     ; check 26 sbc-borrow-out MAN:S8-74: SBC of $20 - $30 with C set borrows: A = $F0, so C is clear and N is set: masked P byte $80
        lda #$20
        sbc #$30
        PMASK $C3
        CHECK_LIT 26
        sec                     ; check 27 sbc-overflow MAN:S8-74: SBC sets V for $80 - $01 = $7F (a negative minus a positive gives a positive): C stays set, masked P byte $41
        lda #$80
        sbc #$01
        PMASK $C3
        CHECK_LIT 27
        lda #$01                ; check 28 sbc-model-result GUEST:sbc-binary: SBC against the in-guest adder fed the complement of M (A + not M + C, S8-74) over 16 edge pairs with C clear and set: the OR of every result difference is zero
        sta a:KIND
        jsr agg_addsub
        stz a:GEXP
        lda a:MDR
        CHECK_GUEST 28
        stz a:GEXP              ; check 29 sbc-model-flags GUEST:sbc-binary: SBC N V Z C against the in-guest adder over the same 32 differences: the OR of every flag difference is zero
        lda a:MDF
        CHECK_GUEST 29
        sed                     ; check 30 dec-adc-simple MAN:S8-17: ADC in decimal mode on valid BCD, C clear: $12 + $34 gives $46
        clc
        lda #$12
        adc #$34
        cld
        CHECK_LIT 30
        sed                     ; check 31 dec-adc-wrap-value MAN:S8-17: ADC in decimal mode on $99 + $01 leaves the decimal sum $00 in A
        clc
        lda #$99
        adc #$01
        cld
        CHECK_LIT 31
        sed                     ; check 32 dec-adc-wrap-c ASSUMED:LED-CPU-0010: ADC in decimal mode on $99 + $01: C set from the decimal carry (the page lists C without defining it in decimal mode, D-99)
        clc
        lda #$99
        adc #$01
        php
        pla
        and #$01
        cld
        CHECK_LIT 32
        sed                     ; check 33 dec-adc-wrap-z ASSUMED:LED-CPU-0010: ADC in decimal mode on $99 + $01: Z set from the adjusted result $00 (D-99)
        clc
        lda #$99
        adc #$01
        php
        pla
        and #$02
        cld
        CHECK_LIT 33
        SETP $40                ; check 34 dec-adc-v-kept-set MAN:S8-17: ADC in decimal mode leaves V alone: V set by PLP before SED, $01 + $01, V still set (binary mode would clear it): masked V bit $40
        sed
        lda #$01
        adc #$01
        php
        pla
        and #$40
        cld
        CHECK_LIT 34
        sed                     ; check 35 dec-sbc-simple MAN:S8-74: SBC in decimal mode on valid BCD, C set: $46 - $12 gives $34
        sec
        lda #$46
        sbc #$12
        cld
        CHECK_LIT 35
        sed                     ; check 36 dec-sbc-digit-borrow MAN:S8-74: SBC in decimal mode borrows from the high digit: $40 - $01 gives $39
        sec
        lda #$40
        sbc #$01
        cld
        CHECK_LIT 36
        sed                     ; check 37 dec-sbc-borrow-in MAN:S8-74: SBC in decimal mode subtracts the complement of C: C clear, $46 - $12 - 1 gives $33
        clc
        lda #$46
        sbc #$12
        cld
        CHECK_LIT 37
        sed                     ; check 38 dec-sbc-borrow-value ASSUMED:LED-CPU-0010: SBC in decimal mode on $12 - $34 (a borrow out): A = $78, the ten's complement of the difference (the project's reading of a decimal borrow, D-99)
        sec
        lda #$12
        sbc #$34
        cld
        CHECK_LIT 38
        sed                     ; check 39 dec-sbc-borrow-c ASSUMED:LED-CPU-0010: SBC in decimal mode on $12 - $34: C clear for the borrow (D-99)
        sec
        lda #$12
        sbc #$34
        php
        pla
        and #$01
        cld
        CHECK_LIT 39
        SETP $00                ; check 40 dec-sbc-v-kept MAN:S8-74: SBC in decimal mode leaves V alone: V clear, $80 - $01 (binary mode would set it): masked V bit $00
        sed
        sec
        lda #$80
        sbc #$01
        php
        pla
        and #$40
        cld
        CHECK_LIT 40
        ldx #$F0                ; check 41 t-adc-cell MAN:S8-17: SET then ADC: the sum goes to the zero-page cell $20F0 (X = $F0): $10 + $20 = $30 in the cell
        lda #$10
        sta a:$2000+$F0
        lda #$77
        clc
        set
        adc #$20
        lda a:$2000+$F0
        CHECK_LIT 41
        ldx #$F0                ; check 42 t-adc-a-kept MAN:S8-17: SET then ADC leaves A alone: A stays $77
        lda #$10
        sta a:$2000+$F0
        lda #$77
        clc
        set
        adc #$20
        CHECK_LIT 42
        ldx #$F0                ; check 43 t-adc-flags MAN:S8-17: SET then ADC takes its flags from the cell sum: cell $FF + $01 = $00 sets Z and C (A = $00 would give neither): masked P byte $03
        lda #$FF
        sta a:$2000+$F0
        lda #$00
        clc
        set
        adc #$01
        PMASK $C3
        CHECK_LIT 43
        ldx #$F0                ; check 44 t-and MAN:S8-18: SET then AND: the cell $F0 AND $3C = $30 is stored in the cell
        lda #$F0
        sta a:$2000+$F0
        lda #$0F
        set
        and #$3C
        lda a:$2000+$F0
        CHECK_LIT 44
        ldx #$F0                ; check 45 t-ora MAN:S8-58: SET then ORA: the cell $30 OR $0C = $3C is stored in the cell
        lda #$30
        sta a:$2000+$F0
        lda #$0F
        set
        ora #$0C
        lda a:$2000+$F0
        CHECK_LIT 45
        ldx #$F0                ; check 46 t-eor MAN:S8-47: SET then EOR: the cell $FF EOR $0F = $F0 is stored in the cell
        lda #$FF
        sta a:$2000+$F0
        lda #$0F
        set
        eor #$0F
        lda a:$2000+$F0
        CHECK_LIT 46
        ldx #$F0                ; check 47 t-cleared-next MAN:S8-17: T mode applies only to the instruction right after SET: SET, CLC, then ADC works on A ($41 + $01 = $42)
        lda #$41
        set
        clc
        adc #$01
        CHECK_LIT 47
        ldx #$F0                ; check 48 t-decimal ASSUMED:LED-CPU-0010: SED before SET, then ADC: the cell $19 + $01 = $20 in decimal (the combination of T mode and decimal mode is stated on no page, D-99)
        lda #$19
        sta a:$2000+$F0
        sed
        clc
        set
        adc #$01
        cld
        lda a:$2000+$F0
        CHECK_LIT 48
        stz a:KIND              ; check 49 asl-model-result GUEST:shift: ASL A against the in-guest bit-serial shifter over 16 edge values with C clear and set: the OR of every result difference is zero
        jsr agg_shift
        stz a:GEXP
        lda a:MDR
        CHECK_GUEST 49
        stz a:GEXP              ; check 50 asl-model-flags GUEST:shift: ASL A N Z C against the in-guest shifter over the same 32 inputs (C is the bit shifted out, not the old C): the OR of every flag difference is zero
        lda a:MDF
        CHECK_GUEST 50
        lda #$81                ; check 51 asl-mem-zp MAN:S8-19: ASL zero page is a read-modify-write: the cell $2050 ($81) becomes $02
        sta z:ZP_W
        asl z:ZP_W
        lda z:ZP_W
        CHECK_LIT 51
        lda #$01                ; check 52 lsr-model-result GUEST:shift: LSR A against the in-guest shifter over 16 edge values with C clear and set: the OR of every result difference is zero
        sta a:KIND
        jsr agg_shift
        stz a:GEXP
        lda a:MDR
        CHECK_GUEST 52
        stz a:GEXP              ; check 53 lsr-model-flags GUEST:shift: LSR A N Z C against the in-guest shifter over the same 32 inputs: the OR of every flag difference is zero
        lda a:MDF
        CHECK_GUEST 53
        SETP $00                ; check 54 lsr-n-clear MAN:S8-56: LSR always clears N (bit 7 is filled with zero): $FF loads with N set, LSR gives $7F, masked P byte $01 (C only)
        lda #$FF
        lsr a
        PMASK $83
        CHECK_LIT 54
        lda #$03                ; check 55 lsr-mem-abs MAN:S8-56: LSR absolute: the cell $2700 ($03) becomes $01
        sta a:RAM_W
        lsr a:RAM_W
        lda a:RAM_W
        CHECK_LIT 55
        lda #$02                ; check 56 rol-model-result GUEST:shift: ROL A against the in-guest shifter (C comes in at bit 0) over 16 edge values with C clear and set: the OR of every result difference is zero
        sta a:KIND
        jsr agg_shift
        stz a:GEXP
        lda a:MDR
        CHECK_GUEST 56
        stz a:GEXP              ; check 57 rol-model-flags GUEST:shift: ROL A N Z C against the in-guest shifter over the same 32 inputs: the OR of every flag difference is zero
        lda a:MDF
        CHECK_GUEST 57
        sec                     ; check 58 rol-carry-in MAN:S8-68: ROL rotates C into bit 0: C set, $40 gives $81
        lda #$40
        rol a
        CHECK_LIT 58
        ldx #$03                ; check 59 rol-mem-zpx MAN:S8-68: ROL zero page,X rotates C in: C set, the cell $2053 ($C0) becomes $81 (X = 3)
        lda #$C0
        sta z:ZP_W,x
        sec
        rol z:ZP_W,x
        lda z:ZP_W,x
        CHECK_LIT 59
        lda #$03                ; check 60 ror-model-result GUEST:shift: ROR A against the in-guest shifter (C comes in at bit 7) over 16 edge values with C clear and set: the OR of every result difference is zero
        sta a:KIND
        jsr agg_shift
        stz a:GEXP
        lda a:MDR
        CHECK_GUEST 60
        stz a:GEXP              ; check 61 ror-model-flags GUEST:shift: ROR A N Z C against the in-guest shifter over the same 32 inputs: the OR of every flag difference is zero
        lda a:MDF
        CHECK_GUEST 61
        sec                     ; check 62 ror-carry-in MAN:S8-69: ROR rotates C into bit 7: C set, $02 gives $81
        lda #$02
        ror a
        CHECK_LIT 62
        ldx #$02                ; check 63 ror-mem-absx MAN:S8-69: ROR absolute,X rotates C in: C set, the cell $2702 ($02) becomes $81 (X = 2)
        lda #$02
        sta a:RAM_W,x
        sec
        ror a:RAM_W,x
        lda a:RAM_W,x
        CHECK_LIT 63
        lda #$41                ; check 64 inc-a MAN:S8-48: INC accumulator: $41 becomes $42
        inc a
        CHECK_LIT 64
        SETP $01                ; check 65 inc-a-wrap MAN:S8-48: INC accumulator wraps $FF to $00: Z set, C untouched (set by PLP), masked P byte $03
        lda #$FF
        inc a
        PMASK $C3
        CHECK_LIT 65
        lda #$42                ; check 66 dec-a MAN:S8-44: DEC accumulator: $42 becomes $41
        dec a
        CHECK_LIT 66
        SETP $01                ; check 67 dec-a-wrap MAN:S8-44: DEC accumulator wraps $00 to $FF: N set, C untouched (set by PLP), masked P byte $81
        lda #$00
        dec a
        PMASK $C3
        CHECK_LIT 67
        lda #$7F                ; check 68 inc-mem-zp MAN:S8-48: INC zero page: the cell $2050 ($7F) becomes $80
        sta z:ZP_W
        inc z:ZP_W
        lda z:ZP_W
        CHECK_LIT 68
        ldx #$02                ; check 69 dec-mem-absx MAN:S8-44: DEC absolute,X: the cell $2702 ($01) becomes $00 (X = 2)
        lda #$01
        sta a:RAM_W,x
        dec a:RAM_W,x
        lda a:RAM_W,x
        CHECK_LIT 69
        ldx #$7E                ; check 70 inx-value MAN:S8-49: INX: $7E becomes $7F (read back with TXA)
        inx
        txa
        CHECK_LIT 70
        ldy #$10                ; check 71 iny-value MAN:S8-50: INY: $10 becomes $11 (read back with TYA)
        iny
        tya
        CHECK_LIT 71
        SETP $01                ; check 72 iny-wrap MAN:S8-50: INY wraps $FF to $00: Z set, C untouched, masked P byte $03
        ldy #$FF
        iny
        PMASK $C3
        CHECK_LIT 72
        ldx #$80                ; check 73 dex-value MAN:S8-45: DEX: $80 becomes $7F (read back with TXA; the page's function line prints Y, its title, opcode CA and mnemonic are X)
        dex
        txa
        CHECK_LIT 73
        SETP $01                ; check 74 dex-wrap MAN:S8-45: DEX wraps $00 to $FF: N set, C untouched, masked P byte $81
        ldx #$00
        dex
        PMASK $C3
        CHECK_LIT 74
        ldy #$01                ; check 75 dey-value MAN:S8-46: DEY: $01 becomes $00 (read back with TYA)
        dey
        tya
        CHECK_LIT 75
        SETP $40                ; check 76 cmp-gt MAN:S8-41: CMP of $30 with $20 (A - M, S8-41): C set, Z and N clear, V untouched (set by PLP): masked P byte $41
        lda #$30
        cmp #$20
        PMASK $C3
        CHECK_LIT 76
        SETP $40                ; check 77 cmp-eq MAN:S8-41: CMP of equal bytes sets Z and C, V untouched: masked P byte $43
        lda #$20
        cmp #$20
        PMASK $C3
        CHECK_LIT 77
        SETP $41                ; check 78 cmp-lt MAN:S8-41: CMP of $10 with $20 clears C (C was set) and sets N from the difference $F0, V untouched: masked P byte $C0
        lda #$10
        cmp #$20
        PMASK $C3
        CHECK_LIT 78
        SETP $00                ; check 79 cmp-zpx MAN:S8-41: CMP zero page,X with equal bytes ($02 and the cell $2013) sets Z and C (X = 3): masked P byte $03
        ldx #$03
        lda #$02
        cmp z:ZP_A,x
        PMASK $C3
        CHECK_LIT 79
        SETP $00                ; check 80 cpx-gt MAN:S8-42: CPX of $30 with $20 sets C: masked P byte $01
        ldx #$30
        cpx #$20
        PMASK $C3
        CHECK_LIT 80
        SETP $01                ; check 81 cpx-lt MAN:S8-42: CPX of $10 with $20 clears C (C was set) and sets N: masked P byte $80
        ldx #$10
        cpx #$20
        PMASK $C3
        CHECK_LIT 81
        SETP $00                ; check 82 cpx-abs MAN:S8-42: CPX absolute with X = $06 against the byte $06 at $2500 sets Z and C: masked P byte $03
        ldx #$06
        cpx a:RAM_ABS
        PMASK $C3
        CHECK_LIT 82
        SETP $01                ; check 83 cpy-lt MAN:S8-43: CPY of $10 with $20 clears C (C was set) and sets N: masked P byte $80
        ldy #$10
        cpy #$20
        PMASK $C3
        CHECK_LIT 83
        SETP $00                ; check 84 cpy-zp MAN:S8-43: CPY zero page with Y = $01 against the cell $2010 ($01) sets Z and C: masked P byte $03
        ldy #$01
        cpy z:ZP_A
        PMASK $C3
        CHECK_LIT 84
        SETP $00                ; check 85 bit-imm MAN:S8-25: BIT immediate: $01 AND $C0 is zero (Z set) and N and V come from the operand bits 7 and 6 (the flag row of the page reads M7 and M6): masked P byte $C2
        lda #$01
        bit #$C0
        PMASK $C3
        CHECK_LIT 85
        lda #$40                ; check 86 bit-zp MAN:S8-25: BIT zero page: A = $0F with the cell $2050 = $40: Z set, N clear, V set from bit 6: masked P byte $42
        sta z:ZP_W
        SETP $00
        lda #$0F
        bit z:ZP_W
        PMASK $C3
        CHECK_LIT 86
        ldx #$02                ; check 87 bit-absx MAN:S8-25: BIT absolute,X: A = $FF with the cell $2702 = $80: Z clear, N set, V clear: masked P byte $80 (X = 2)
        lda #$80
        sta a:RAM_W,x
        SETP $00
        lda #$FF
        bit a:RAM_W,x
        PMASK $C3
        CHECK_LIT 87
        lda #$30                ; check 88 tsb-zp-mem MAN:S8-103: TSB zero page stores A OR M: A = $0F, the cell $2050 ($30) becomes $3F
        sta z:ZP_W
        lda #$0F
        tsb z:ZP_W
        lda z:ZP_W
        CHECK_LIT 88
        lda #$02                ; check 89 tsb-abs-mem MAN:S8-103: TSB absolute stores A OR M: A = $81, the cell $2700 ($02) becomes $83
        sta a:RAM_W
        lda #$81
        tsb a:RAM_W
        lda a:RAM_W
        CHECK_LIT 89
        lda #$F0                ; check 90 tsb-z-set ASSUMED:LED-CPU-0010: TSB: Z is set when A AND the old memory byte is zero (A = $0F, cell $F0). The page lists Z without saying where it comes from; N and V of TSB are not asserted (S8-103 against pcetech)
        sta z:ZP_W
        SETP $00
        lda #$0F
        tsb z:ZP_W
        PMASK $02
        CHECK_LIT 90
        lda #$FF                ; check 91 tsb-z-clear ASSUMED:LED-CPU-0010: TSB: Z is clear when A AND the old memory byte is not zero (A = $01, cell $FF)
        sta z:ZP_W
        SETP $02
        lda #$01
        tsb z:ZP_W
        PMASK $02
        CHECK_LIT 91
        lda #$FF                ; check 92 trb-zp-mem ASSUMED:LED-CPU-0010: TRB zero page clears the bits of A in memory (the page formula M <- not A AND M; its prose says the result is not stored, so the stored result is the project's reading): A = $0F, the cell $2050 ($FF) becomes $F0
        sta z:ZP_W
        lda #$0F
        trb z:ZP_W
        lda z:ZP_W
        CHECK_LIT 92
        lda #$F0                ; check 93 trb-z-set ASSUMED:LED-CPU-0010: TRB: Z is set when A AND the old memory byte is zero (A = $0F, cell $F0); N and V of TRB are not asserted
        sta z:ZP_W
        SETP $00
        lda #$0F
        trb z:ZP_W
        PMASK $02
        CHECK_LIT 93
        lda #$FE                ; check 94 tst-zp MAN:S8-104: TST #imm,zero page: $01 AND the cell $2050 ($FE) is zero (Z set); N and V come from the memory byte bits 7 and 6: masked P byte $C2
        sta z:ZP_W
        SETP $00
        tst #$01, z:ZP_W
        PMASK $C3
        CHECK_LIT 94
        ldx #$03                ; check 95 tst-zpx MAN:S8-104: TST #imm,zero page,X: $80 AND the cell $2053 ($80) is not zero; N set, V clear: masked P byte $80 (X = 3)
        lda #$80
        sta z:ZP_W,x
        SETP $00
        tst #$80, z:ZP_W,x
        PMASK $C3
        CHECK_LIT 95
        ldx #$02                ; check 96 tst-absx MAN:S8-104: TST #imm,absolute,X: $FF AND the cell $2702 ($40) is not zero; V set from bit 6, N clear: masked P byte $40 (X = 2)
        lda #$40
        sta a:RAM_W,x
        SETP $00
        tst #$FF, a:RAM_W,x
        PMASK $C3
        CHECK_LIT 96
        rts

; the model routines (GEXP is the expected byte of the CHECK_GUEST macro)
; model_add: GEXP = (MA + MB + MC) AND $FF, MCO = carry out (0 or 1), MV = signed overflow (0 or 1), one bit at a time from the mask table
model_add:
        stz a:MRES
        lda a:MC
        sta a:MCUR
        ldy #$00
ma_loop: lda a:masktab,y
        beq ma_done
        sta a:MMASK
        lda a:MCUR
        sta a:MCIN7             ; the carry into this bit; after the loop it is the carry into bit 7
        beq ma_c0
        lda a:MMASK
ma_c0:  sta a:MCI               ; the carry in as a mask or zero
        lda a:MMASK
        and a:MA
        sta a:MAI
        lda a:MMASK
        and a:MB
        sta a:MBI
        eor a:MAI
        sta a:MXOR
        eor a:MCI               ; the sum bit
        ora a:MRES
        sta a:MRES
        lda a:MAI
        and a:MBI
        sta a:MT
        lda a:MCI
        and a:MXOR
        ora a:MT                ; the carry out is the majority of the three bits
        beq ma_nc
        lda #$01
ma_nc:  sta a:MCUR
        iny
        bra ma_loop
ma_done: lda a:MRES
        sta a:GEXP
        lda a:MCUR
        sta a:MCO
        eor a:MCIN7
        sta a:MV
        rts
; model_flags: GFL = N, V, Z and C of the model result as a P mask ($C3), from GEXP, MV and MCO
model_flags:
        lda a:GEXP
        and #$80
        sta a:GFL
        lda a:GEXP
        bne mf_z
        lda a:GFL
        ora #$02
        sta a:GFL
mf_z:   lda a:MV
        beq mf_v
        lda a:GFL
        ora #$40
        sta a:GFL
mf_v:   lda a:MCO
        beq mf_c
        lda a:GFL
        ora #$01
        sta a:GFL
mf_c:   rts
; model_shl: GEXP = MA shifted left one bit with MFILL (0 or 1) at bit 0, MCO = the bit shifted out
model_shl:
        stz a:MRES
        stz a:MCO
        ldy #$00
msl:    lda a:masktab,y
        beq msl_d
        and a:MA
        beq msl_n
        lda a:masktab+1,y       ; the next mask up; the terminator after bit 7 stands for the carry out
        bne msl_s
        lda #$01
        sta a:MCO
        bra msl_n
msl_s:  ora a:MRES
        sta a:MRES
msl_n:  iny
        bra msl
msl_d:  lda a:MFILL
        beq msl_f
        lda a:MRES
        ora #$01
        sta a:MRES
msl_f:  lda a:MRES
        sta a:GEXP
        rts
; model_shr: GEXP = MA shifted right one bit with MFILL (0 or 1) at bit 7, MCO = the bit shifted out
model_shr:
        stz a:MRES
        stz a:MCO
        ldy #$00
msr:    lda a:masktab,y
        beq msr_d
        and a:MA
        beq msr_n
        lda a:masktab-1,y       ; the next mask down; the zero before bit 0 stands for the carry out
        bne msr_s
        lda #$01
        sta a:MCO
        bra msr_n
msr_s:  ora a:MRES
        sta a:MRES
msr_n:  iny
        bra msr
msr_d:  lda a:MFILL
        beq msr_f
        lda a:MRES
        ora #$80
        sta a:MRES
msr_f:  lda a:MRES
        sta a:GEXP
        rts
; agg_addsub: KIND 0 runs ADC and KIND 1 SBC over the 32 sums of vec_ac against model_add (SBC feeds the model the complement of M, A + not M + C, S8-74); MDR = the OR of the result differences, MDF = the OR of the flag differences
agg_addsub:
        stz a:MDR
        stz a:MDF
        ldx #$00
aa_loop: lda a:vec_ac,x
        sta a:MA
        lda a:vec_ac+1,x
        sta a:MM
        ldy a:KIND
        beq aa_noinv
        eor #$FF
aa_noinv: sta a:MB
        lda a:vec_ac+2,x
        sta a:MC
        jsr model_add
        jsr model_flags
        lda a:MC
        beq aa_c0
        sec
        bra aa_go
aa_c0:  clc
aa_go:  lda a:MA
        ldy a:KIND
        bne aa_sbc
        adc a:MB
        bra aa_res
aa_sbc: sbc a:MM
aa_res: sta a:MR
        php
        pla
        and #$C3
        sta a:RFL
        lda a:MR
        eor a:GEXP
        ora a:MDR
        sta a:MDR
        lda a:RFL
        eor a:GFL
        ora a:MDF
        sta a:MDF
        inx
        inx
        inx
        txa
        cmp #96
        bne aa_loop
        rts
; agg_shift: KIND 0 ASL, 1 LSR, 2 ROL, 3 ROR over the 32 inputs of vec_sh against model_shl or model_shr; the carry in (rotates) is MFILL; MDR and MDF as above, flags N Z C ($83)
agg_shift:
        stz a:MDR
        stz a:MDF
        ldx #$00
as_loop: lda a:vec_sh,x
        sta a:MA
        lda a:vec_sh+1,x
        sta a:MC
        stz a:MFILL
        lda a:KIND
        and #$02
        beq as_nof
        lda a:MC
        sta a:MFILL
as_nof: lda a:KIND
        and #$01
        bne as_r
        jsr model_shl
        bra as_m
as_r:   jsr model_shr
as_m:   stz a:MV
        jsr model_flags
        lda a:MC
        beq as_c0
        sec
        bra as_go
as_c0:  clc
as_go:  lda a:MA
        ldy a:KIND
        beq as_asl
        dey
        beq as_lsr
        dey
        beq as_rol
        ror a
        bra as_res
as_asl: asl a
        bra as_res
as_lsr: lsr a
        bra as_res
as_rol: rol a
as_res: sta a:MR
        php
        pla
        and #$83
        sta a:RFL
        lda a:MR
        eor a:GEXP
        ora a:MDR
        sta a:MDR
        lda a:RFL
        eor a:GFL
        ora a:MDF
        sta a:MDF
        inx
        inx
        txa
        cmp #64
        bne as_loop
        rts

        .byte $00               ; the zero before the first mask
masktab: .byte $01, $02, $04, $08, $10, $20, $40, $80, $00
; the sums: 16 edge pairs (A, M), each with C clear and then set; three bytes per vector
vec_ac:
        .byte $00, $00, $00, $00, $00, $01, $00, $01, $00, $00, $01, $01
        .byte $01, $FF, $00, $01, $FF, $01, $FF, $FF, $00, $FF, $FF, $01
        .byte $7F, $01, $00, $7F, $01, $01, $80, $80, $00, $80, $80, $01
        .byte $80, $FF, $00, $80, $FF, $01, $7F, $7F, $00, $7F, $7F, $01
        .byte $55, $AA, $00, $55, $AA, $01, $AA, $55, $00, $AA, $55, $01
        .byte $0F, $01, $00, $0F, $01, $01, $F0, $10, $00, $F0, $10, $01
        .byte $80, $01, $00, $80, $01, $01, $FF, $01, $00, $FF, $01, $01
        .byte $12, $34, $00, $12, $34, $01, $C3, $3C, $00, $C3, $3C, $01
; the shifts: 16 edge bytes, each with C clear and then set; two bytes per vector
vec_sh:
        .byte $00, $00, $00, $01, $01, $00, $01, $01, $02, $00, $02, $01, $7F, $00, $7F, $01
        .byte $80, $00, $80, $01, $81, $00, $81, $01, $40, $00, $40, $01, $FF, $00, $FF, $01
        .byte $55, $00, $55, $01, $AA, $00, $AA, $01, $FE, $00, $FE, $01, $C0, $00, $C0, $01
        .byte $0F, $00, $0F, $01, $F0, $00, $F0, $01, $3C, $00, $3C, $01, $C3, $00, $C3, $01
