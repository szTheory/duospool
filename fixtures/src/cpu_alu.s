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
DSV_CHECKS = 25

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
        LIT 20, $14
        LIT 21, $15
        LIT 22, $16
        LIT 23, $17
        LIT 24, $18
        LIT 25, $19

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
        sec                     ; check 20 adc-ind MAN:S8-17: ADC (zz) with C set: $10 + the cell value $03 + 1 gives $14
        lda #$10
        adc (<ZP_PTR)
        CHECK_LIT 20
        sec                     ; check 21 adc-indx MAN:S8-17: ADC (zz,X) (X = 4) with C set: $10 + the cell value $04 + 1 gives $15
        ldx #$04
        lda #$10
        adc (<ZP_PTR,x)
        CHECK_LIT 21
        sec                     ; check 22 adc-indy MAN:S8-17: ADC (zz),Y (Y = 3) with C set: $10 + the cell value $05 + 1 gives $16
        ldy #$03
        lda #$10
        adc (<ZP_PTR),y
        CHECK_LIT 22
        sec                     ; check 23 adc-abs MAN:S8-17: ADC absolute with C set: $10 + the cell value $06 + 1 gives $17
        lda #$10
        adc a:RAM_ABS
        CHECK_LIT 23
        sec                     ; check 24 adc-absx MAN:S8-17: ADC absolute,X (X = 2) with C set: $10 + the cell value $07 + 1 gives $18
        ldx #$02
        lda #$10
        adc a:RAM_ABS,x
        CHECK_LIT 24
        sec                     ; check 25 adc-absy MAN:S8-17: ADC absolute,Y (Y = 5) with C set: $10 + the cell value $08 + 1 gives $19
        ldy #$05
        lda #$10
        adc a:RAM_ABS,y
        CHECK_LIT 25
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
