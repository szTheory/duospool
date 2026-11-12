; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; cpu_flow.s - control flow of the HuC6280 (plan 03-13; D-98, D-99, D-124, D-125, D-127, S8-20 to S8-24, S8-26 to S8-33, S8-35 to S8-38, S8-51, S8-52, S8-57, S8-70, S8-71, S8-75 to S8-77). It is assembled by ca65 in the gate and never committed as a binary.
; Checks run in ladder order: BEQ and BNE with JSR and RTS first (the tracer), then the other conditional branches, BRA with its extreme offsets, BBR and BBS for all eight bits, BSR, the three JMP forms, BRK and RTI through the RAM vector of startup.inc, the flag instructions and NOP.
; A branch row reads a marker byte (the path reached writes $01 or $02); a subroutine or handler reads the bytes pushed on the stack page. Nothing here counts cycles: the cycle checks arrive with the timer and speed plans (D-98).
; The BRK handler is installed in the IRQ2 and BRK RAM slot of startup.inc and the default (the error-flag handler) is put back afterwards, so a stray vector sets the DSV1 error flag. P is set from a byte by PLP and read as PHP, PLA and a mask (B reads 1 and T reads 0 through PHP, H8-5).
; The T bit that BRK pushes and the T load of RTI are ASSUMED rows (LED-CPU-0009): the open question of the T flag pushed at hardware interrupt entry (H8-4 and H8-5 against pcetech) is not decided here. No taken-branch page-cross penalty is asserted (LED-CPU-0010).
; The pages the checks read are filled with $FF first and every cell a check reads is written by the fixture before (D-127). This file is generated from a table of checks by a scratch script; the committed text is the source of record.
        .setcpu "huc6280"

DSV_ID = 5
DSV_CHECKS = 71

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
MARK    = $2410
ACC     = $2411
SV      = $2412                 ; S saved before a call or a BRK
SUBL    = $2413                 ; the low and high byte pushed by JSR or BSR, read by the subroutine
SUBH    = $2414
BRKP    = $2415                 ; the bytes the BRK handler found on the stack: P, PCL, PCH
BRKL    = $2416
BRKH    = $2417
BRKI    = $2418                 ; I and D inside the handler, masked $0C
BRKS    = $2419                 ; S inside the handler
BRKHIT  = $241A                 ; how often the handler ran
BRKMODE = $241B                 ; 1 = the handler sets bit 5 of the pushed P
MR1     = $2420
MR2     = $2421
MR3     = $2422
MR4     = $2423
; zero-page cells (logical $2000 + the offset) and pointers
ZPA     = $40
ZPB     = $41
ZPC     = $42
ZA      = $2000+ZPA
ZB      = $2000+ZPB
ZC      = $2000+ZPC
JP1     = $2520                 ; the pointer of JMP (abs)
JT      = $2540                 ; the table of JMP (abs,X)

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

        LIT 1, $02
        LIT 2, $01
        LIT 3, $02
        LIT 4, $01
        LIT 5, $00
        LIT 6, $00
        LIT 7, $01
        LIT 8, $00
        LIT 9, $02
        LIT 10, $01
        LIT 11, $02
        LIT 12, $01
        LIT 13, $02
        LIT 14, $01
        LIT 15, $02
        LIT 16, $01
        LIT 17, $02
        LIT 18, $01
        LIT 19, $02
        LIT 20, $01
        LIT 21, $01
        LIT 22, $10
        LIT 23, $10
        LIT 24, $05
        LIT 25, $C3
        LIT 26, $C3
        LIT 27, $06
        LIT 28, $06
        LIT 29, $06
        LIT 30, $06
        LIT 31, $06
        LIT 32, $06
        LIT 33, $06
        LIT 34, $06
        LIT 35, $00
        LIT 36, $00
        LIT 37, $01
        LIT 38, $01
        LIT 39, $C3
        LIT 40, $04
        LIT 41, $00
        LIT 42, $00
        LIT 43, $01
        LIT 44, $00
        LIT 45, $00
        LIT 46, $00
        LIT 47, $00
        LIT 48, $CB
        LIT 49, $00
        LIT 50, $00
        LIT 51, $01
        LIT 52, $00
        LIT 53, $00
        LIT 54, $10
        LIT 55, $C3
        LIT 56, $00
        LIT 57, $04
        LIT 58, $00
        LIT 59, $81
        LIT 60, $CE
        LIT 61, $C7
        LIT 62, $CB
        LIT 63, $8F
        LIT 64, $01
        LIT 65, $08
        LIT 66, $04
        LIT 67, $5A
        LIT 68, $A5
        LIT 69, $3C
        LIT 70, $C5
        LIT 71, $01

fixture_main:
        ldx #$00                ; the pages the checks read are filled with $FF, then every cell that a check reads is written
        lda #$FF
fill:   sta a:$2000,x
        sta a:$2500,x
        sta a:$2600,x
        inx
        bne fill
        SETP $02 ; check 1 beq-taken MAN:S8-24: BEQ branches when Z is 1: P = Z only (N, V, C clear), the target path is reached (marker $02)
        beq c1_t
        lda #$01
        bra c1_d
c1_t: lda #$02
c1_d: CHECK_LIT 1
        SETP $C1 ; check 2 beq-not-taken MAN:S8-24: BEQ does not branch when Z is 0: P = N V C set, Z clear, the fall-through path runs (marker $01)
        beq c2_t
        lda #$01
        bra c2_d
c2_t: lda #$02
c2_d: CHECK_LIT 2
        SETP $C1 ; check 3 bne-taken MAN:S8-27: BNE branches when Z is 0: P = N V C set, Z clear, the target path is reached (marker $02)
        bne c3_t
        lda #$01
        bra c3_d
c3_t: lda #$02
c3_d: CHECK_LIT 3
        SETP $02 ; check 4 bne-not-taken MAN:S8-27: BNE does not branch when Z is 1: P = Z only, the fall-through path runs (marker $01)
        bne c4_t
        lda #$01
        bra c4_d
c4_t: lda #$02
c4_d: CHECK_LIT 4
        tsx ; check 5 jsr-pushed-pcl GUEST:jsr-site-plus-2: JSR pushes the address of its last byte (the JSR address plus 2), PCH first then PCL (S8-52): the subroutine reads the pushed low byte from the stack page, the expected byte is computed from the assembler label of the JSR
        stx a:SV
        ldy #$00
js1:    jsr sub_a
        iny
        lda #<(js1+2)
        sta a:GEXP
        lda a:SUBL
        CHECK_GUEST 5
        lda #>(js1+2) ; check 6 jsr-pushed-pch GUEST:jsr-site-plus-2: JSR pushes PCH first (at the higher stack address) then PCL (S8-52): the pushed high byte read from the stack page is the high byte of the JSR address plus 2
        sta a:GEXP
        lda a:SUBH
        CHECK_GUEST 6
        tya ; check 7 rts-next-instruction MAN:S8-71: RTS pulls PCL then PCH and adds 1 (S8-71), so the instruction after the JSR runs exactly once: Y is $00 before the JSR and one INY follows it
        CHECK_LIT 7
        lda a:SV ; check 8 rts-s-balance GUEST:s-before-equals-s-after: JSR and RTS leave S as it was (two pushes, two pulls, S8-52, S8-71): S after the return equals the S saved before the JSR
        sta a:GEXP
        tsx
        txa
        CHECK_GUEST 8
        SETP $43 ; check 9 bpl-taken MAN:S8-28: BPL branches when N is 0: P byte $43 (the other flags in the opposite state), the target path is reached (marker $02)
        bpl c9_t
        lda #$01
        bra c9_d
c9_t: lda #$02
c9_d: CHECK_LIT 9
        SETP $80 ; check 10 bpl-not-taken MAN:S8-28: BPL does not branch when N is 1: P byte $80, the fall-through path runs (marker $01)
        bpl c10_t
        lda #$01
        bra c10_d
c10_t: lda #$02
c10_d: CHECK_LIT 10
        SETP $80 ; check 11 bmi-taken MAN:S8-26: BMI branches when N is 1: P byte $80 (the other flags in the opposite state), the target path is reached (marker $02)
        bmi c11_t
        lda #$01
        bra c11_d
c11_t: lda #$02
c11_d: CHECK_LIT 11
        SETP $43 ; check 12 bmi-not-taken MAN:S8-26: BMI does not branch when N is 0: P byte $43, the fall-through path runs (marker $01)
        bmi c12_t
        lda #$01
        bra c12_d
c12_t: lda #$02
c12_d: CHECK_LIT 12
        SETP $83 ; check 13 bvc-taken MAN:S8-32: BVC branches when V is 0: P byte $83 (the other flags in the opposite state), the target path is reached (marker $02)
        bvc c13_t
        lda #$01
        bra c13_d
c13_t: lda #$02
c13_d: CHECK_LIT 13
        SETP $40 ; check 14 bvc-not-taken MAN:S8-32: BVC does not branch when V is 1: P byte $40, the fall-through path runs (marker $01)
        bvc c14_t
        lda #$01
        bra c14_d
c14_t: lda #$02
c14_d: CHECK_LIT 14
        SETP $40 ; check 15 bvs-taken MAN:S8-33: BVS branches when V is 1: P byte $40 (the other flags in the opposite state), the target path is reached (marker $02)
        bvs c15_t
        lda #$01
        bra c15_d
c15_t: lda #$02
c15_d: CHECK_LIT 15
        SETP $83 ; check 16 bvs-not-taken MAN:S8-33: BVS does not branch when V is 0: P byte $83, the fall-through path runs (marker $01)
        bvs c16_t
        lda #$01
        bra c16_d
c16_t: lda #$02
c16_d: CHECK_LIT 16
        SETP $C2 ; check 17 bcc-taken MAN:S8-22: BCC branches when C is 0: P byte $C2 (the other flags in the opposite state), the target path is reached (marker $02)
        bcc c17_t
        lda #$01
        bra c17_d
c17_t: lda #$02
c17_d: CHECK_LIT 17
        SETP $01 ; check 18 bcc-not-taken MAN:S8-22: BCC does not branch when C is 1: P byte $01, the fall-through path runs (marker $01)
        bcc c18_t
        lda #$01
        bra c18_d
c18_t: lda #$02
c18_d: CHECK_LIT 18
        SETP $01 ; check 19 bcs-taken MAN:S8-23: BCS branches when C is 1: P byte $01 (the other flags in the opposite state), the target path is reached (marker $02)
        bcs c19_t
        lda #$01
        bra c19_d
c19_t: lda #$02
c19_d: CHECK_LIT 19
        SETP $C2 ; check 20 bcs-not-taken MAN:S8-23: BCS does not branch when C is 0: P byte $C2, the fall-through path runs (marker $01)
        bcs c20_t
        lda #$01
        bra c20_d
c20_t: lda #$02
c20_d: CHECK_LIT 20
        lda #$01 ; check 21 bra-taken MAN:S8-29: BRA always branches: PC <- PC+2+rr skips the following two-byte LDA (A stays $01)
        bra c21_t
        lda #$02
c21_t: CHECK_LIT 21
        ldx #$00 ; check 22 branch-fwd-127 MAN:S8-29: The offset rr is signed, -128 to 127, and the target is PC+2+rr (S8-29): BRA with rr = 127 skips 127 INX bytes and lands on the INY after them (X = 0, Y = 1 gives $10)
        ldy #$00
        bra c22_t
        .res 127, $E8
c22_t: iny
        tya
        asl a
        asl a
        asl a
        asl a
        sta a:MR1
        txa
        ora a:MR1
        CHECK_LIT 22
        ldx #$00 ; check 23 branch-back-128 MAN:S8-27: BNE with rr = -128 (sign extension of $80) branches back 126 bytes to the INY: reached once (Y = 1) and no INX filler executed (X = 0) gives $10
        ldy #$00
        lda #$01
        jmp a:c23_b
c23_t: iny
        jmp a:c23_o
        .res 122, $E8
c23_b: bne c23_t
c23_o: tya
        asl a
        asl a
        asl a
        asl a
        sta a:MR1
        txa
        ora a:MR1
        CHECK_LIT 23
        ldx #$00 ; check 24 branch-back-loop MAN:S8-27: A short backward BNE loop: INX, CPX #5, BNE back runs the body five times (X = 5)
c24_l: inx
        cpx #$05
        bne c24_l
        txa
        CHECK_LIT 24
        SETP $C3 ; check 25 taken-branch-keeps-flags MAN:S8-23: A taken branch changes no flag (the flag row of S8-23 is T = 0 and dashes): P = N V Z C set, BCS taken over a CLC, the masked P byte stays $C3
        bcs c25_t
        clc
c25_t: PMASK $C3
        CHECK_LIT 25
        SETP $C3 ; check 26 not-taken-keeps-flags MAN:S8-22: A branch that is not taken changes no flag (S8-22): P = N V Z C set, BCC not taken (C is 1), the masked P byte stays $C3
        bcc c26_t
        nop
c26_t: PMASK $C3
        CHECK_LIT 26
        lda #$01 ; check 27 bb-bit0 MAN:S8-20: Bit 0: with the cell $01 (only bit 0 set) BBR0 is not taken and BBS0 is taken, with the cell $FE (only bit 0 clear) BBR0 is taken and BBS0 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr0 <ZPA, c27_a1
        bra c27_a2
c27_a1: lda a:ACC
        ora #$01
        sta a:ACC
c27_a2: bbr0 <ZPB, c27_b1
        bra c27_b2
c27_b1: lda a:ACC
        ora #$02
        sta a:ACC
c27_b2: bbs0 <ZPA, c27_c1
        bra c27_c2
c27_c1: lda a:ACC
        ora #$04
        sta a:ACC
c27_c2: bbs0 <ZPB, c27_d1
        bra c27_d2
c27_d1: lda a:ACC
        ora #$08
        sta a:ACC
c27_d2: lda a:ACC
        CHECK_LIT 27
        lda #$02 ; check 28 bb-bit1 MAN:S8-20: Bit 1: with the cell $02 (only bit 1 set) BBR1 is not taken and BBS1 is taken, with the cell $FD (only bit 1 clear) BBR1 is taken and BBS1 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr1 <ZPA, c28_a1
        bra c28_a2
c28_a1: lda a:ACC
        ora #$01
        sta a:ACC
c28_a2: bbr1 <ZPB, c28_b1
        bra c28_b2
c28_b1: lda a:ACC
        ora #$02
        sta a:ACC
c28_b2: bbs1 <ZPA, c28_c1
        bra c28_c2
c28_c1: lda a:ACC
        ora #$04
        sta a:ACC
c28_c2: bbs1 <ZPB, c28_d1
        bra c28_d2
c28_d1: lda a:ACC
        ora #$08
        sta a:ACC
c28_d2: lda a:ACC
        CHECK_LIT 28
        lda #$04 ; check 29 bb-bit2 MAN:S8-20: Bit 2: with the cell $04 (only bit 2 set) BBR2 is not taken and BBS2 is taken, with the cell $FB (only bit 2 clear) BBR2 is taken and BBS2 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr2 <ZPA, c29_a1
        bra c29_a2
c29_a1: lda a:ACC
        ora #$01
        sta a:ACC
c29_a2: bbr2 <ZPB, c29_b1
        bra c29_b2
c29_b1: lda a:ACC
        ora #$02
        sta a:ACC
c29_b2: bbs2 <ZPA, c29_c1
        bra c29_c2
c29_c1: lda a:ACC
        ora #$04
        sta a:ACC
c29_c2: bbs2 <ZPB, c29_d1
        bra c29_d2
c29_d1: lda a:ACC
        ora #$08
        sta a:ACC
c29_d2: lda a:ACC
        CHECK_LIT 29
        lda #$08 ; check 30 bb-bit3 MAN:S8-20: Bit 3: with the cell $08 (only bit 3 set) BBR3 is not taken and BBS3 is taken, with the cell $F7 (only bit 3 clear) BBR3 is taken and BBS3 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr3 <ZPA, c30_a1
        bra c30_a2
c30_a1: lda a:ACC
        ora #$01
        sta a:ACC
c30_a2: bbr3 <ZPB, c30_b1
        bra c30_b2
c30_b1: lda a:ACC
        ora #$02
        sta a:ACC
c30_b2: bbs3 <ZPA, c30_c1
        bra c30_c2
c30_c1: lda a:ACC
        ora #$04
        sta a:ACC
c30_c2: bbs3 <ZPB, c30_d1
        bra c30_d2
c30_d1: lda a:ACC
        ora #$08
        sta a:ACC
c30_d2: lda a:ACC
        CHECK_LIT 30
        lda #$10 ; check 31 bb-bit4 MAN:S8-20: Bit 4: with the cell $10 (only bit 4 set) BBR4 is not taken and BBS4 is taken, with the cell $EF (only bit 4 clear) BBR4 is taken and BBS4 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr4 <ZPA, c31_a1
        bra c31_a2
c31_a1: lda a:ACC
        ora #$01
        sta a:ACC
c31_a2: bbr4 <ZPB, c31_b1
        bra c31_b2
c31_b1: lda a:ACC
        ora #$02
        sta a:ACC
c31_b2: bbs4 <ZPA, c31_c1
        bra c31_c2
c31_c1: lda a:ACC
        ora #$04
        sta a:ACC
c31_c2: bbs4 <ZPB, c31_d1
        bra c31_d2
c31_d1: lda a:ACC
        ora #$08
        sta a:ACC
c31_d2: lda a:ACC
        CHECK_LIT 31
        lda #$20 ; check 32 bb-bit5 MAN:S8-20: Bit 5: with the cell $20 (only bit 5 set) BBR5 is not taken and BBS5 is taken, with the cell $DF (only bit 5 clear) BBR5 is taken and BBS5 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr5 <ZPA, c32_a1
        bra c32_a2
c32_a1: lda a:ACC
        ora #$01
        sta a:ACC
c32_a2: bbr5 <ZPB, c32_b1
        bra c32_b2
c32_b1: lda a:ACC
        ora #$02
        sta a:ACC
c32_b2: bbs5 <ZPA, c32_c1
        bra c32_c2
c32_c1: lda a:ACC
        ora #$04
        sta a:ACC
c32_c2: bbs5 <ZPB, c32_d1
        bra c32_d2
c32_d1: lda a:ACC
        ora #$08
        sta a:ACC
c32_d2: lda a:ACC
        CHECK_LIT 32
        lda #$40 ; check 33 bb-bit6 MAN:S8-20: Bit 6: with the cell $40 (only bit 6 set) BBR6 is not taken and BBS6 is taken, with the cell $BF (only bit 6 clear) BBR6 is taken and BBS6 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr6 <ZPA, c33_a1
        bra c33_a2
c33_a1: lda a:ACC
        ora #$01
        sta a:ACC
c33_a2: bbr6 <ZPB, c33_b1
        bra c33_b2
c33_b1: lda a:ACC
        ora #$02
        sta a:ACC
c33_b2: bbs6 <ZPA, c33_c1
        bra c33_c2
c33_c1: lda a:ACC
        ora #$04
        sta a:ACC
c33_c2: bbs6 <ZPB, c33_d1
        bra c33_d2
c33_d1: lda a:ACC
        ora #$08
        sta a:ACC
c33_d2: lda a:ACC
        CHECK_LIT 33
        lda #$80 ; check 34 bb-bit7 MAN:S8-20: Bit 7: with the cell $80 (only bit 7 set) BBR7 is not taken and BBS7 is taken, with the cell $7F (only bit 7 clear) BBR7 is taken and BBS7 is not (S8-20, S8-21): outcome bits $02 and $04 set, $06
        sta a:ZA
        eor #$FF
        sta a:ZB
        stz a:ACC
        bbr7 <ZPA, c34_a1
        bra c34_a2
c34_a1: lda a:ACC
        ora #$01
        sta a:ACC
c34_a2: bbr7 <ZPB, c34_b1
        bra c34_b2
c34_b1: lda a:ACC
        ora #$02
        sta a:ACC
c34_b2: bbs7 <ZPA, c34_c1
        bra c34_c2
c34_c1: lda a:ACC
        ora #$04
        sta a:ACC
c34_c2: bbs7 <ZPB, c34_d1
        bra c34_d2
c34_d1: lda a:ACC
        ora #$08
        sta a:ACC
c34_d2: lda a:ACC
        CHECK_LIT 34
        stz a:ZA ; check 35 bbr-taken-skips MAN:S8-20: BBR0 on a cell with bit 0 clear branches, PC <- PC+3+rr skipping the one-byte INX (X stays 0, S8-20)
        ldx #$00
        bbr0 <ZPA, c35_t
        inx
c35_t: txa
        CHECK_LIT 35
        lda #$01 ; check 36 bbs-taken-skips MAN:S8-21: BBS0 on a cell with bit 0 set branches, PC <- PC+3+rr skipping the one-byte INX (X stays 0, S8-21)
        sta a:ZA
        ldx #$00
        bbs0 <ZPA, c36_t
        inx
c36_t: txa
        CHECK_LIT 36
        lda #$01 ; check 37 bbr-not-taken MAN:S8-20: BBR0 on a cell with bit 0 set does not branch: the program counter is increased by 3 (S8-20) and the INX runs (X = 1)
        sta a:ZA
        ldx #$00
        bbr0 <ZPA, c37_t
        inx
c37_t: txa
        CHECK_LIT 37
        stz a:ZA ; check 38 bbs-not-taken MAN:S8-21: BBS0 on a cell with bit 0 clear does not branch: the program counter is increased by 3 (S8-21) and the INX runs (X = 1)
        ldx #$00
        bbs0 <ZPA, c38_t
        inx
c38_t: txa
        CHECK_LIT 38
        stz a:ZA ; check 39 bb-keeps-flags MAN:S8-21: BBR and BBS change no flag (the flag row is T = 0 and dashes): P = N V Z C set, BBR0 taken over a CLC, the masked P byte stays $C3
        SETP $C3
        bbr0 <ZPA, c39_t
        clc
c39_t: PMASK $C3
        CHECK_LIT 39
        stz a:ZC ; check 40 bb-back-loop MAN:S8-20: A backward BBR (rr negative, target PC+3+rr, S8-20): INC of a zero-page cell with BBR2 back to the INC runs until bit 2 of the cell is set, the cell ends at $04
c40_l: inc a:ZC
        bbr2 <ZPC, c40_l
        lda a:ZC
        CHECK_LIT 40
        ldy #$00 ; check 41 bsr-pushed-pcl GUEST:bsr-site-plus-1: BSR pushes the address of the last byte of the BSR instruction (the BSR address plus 1), PCH first (S8-31): the pushed low byte read in the subroutine
c41_s: bsr c41_sub
        iny
        bra c41_o
c41_sub: tsx
        lda a:$2101,x
        sta a:SUBL
        lda a:$2102,x
        sta a:SUBH
        rts
c41_o: lda #<(c41_s+1)
        sta a:GEXP
        lda a:SUBL
        CHECK_GUEST 41
        lda #>(c41_s+1) ; check 42 bsr-pushed-pch GUEST:bsr-site-plus-1: BSR pushes PCH first (S8-31): the pushed high byte is the high byte of the BSR address plus 1
        sta a:GEXP
        lda a:SUBH
        CHECK_GUEST 42
        tya ; check 43 bsr-returns-next MAN:S8-31: RTS after BSR returns to the instruction after the BSR (S8-31, S8-71): the one INY after it ran once, Y = 1
        CHECK_LIT 43
        ldx #$00 ; check 44 jmp-abs MAN:S8-51: JMP absolute: PCL <- ll, PCH <- hh (S8-51): the jump skips the INX (X = 0)
        jmp a:c44_t
        inx
c44_t: txa
        CHECK_LIT 44
        lda #<c45_t ; check 45 jmp-ind MAN:S8-51: JMP (abs): PCL <- (hhll), PCH <- (hhll+1) (S8-51): the pointer in work RAM names the target, the INX is skipped (X = 0)
        sta a:JP1
        lda #>c45_t
        sta a:JP1+1
        ldx #$00
        jmp (JP1)
        inx
c45_t: txa
        CHECK_LIT 45
        lda #<c46_t ; check 46 jmp-ind-xxff MAN:S8-51: JMP (abs) with the pointer at $25FF: PCL comes from $25FF and PCH from (hhll+1) = $2600 (S8-51 prints hhll+1; there is no 6502 page wrap), the INX is skipped (X = 0)
        sta a:$25FF
        lda #>c46_t
        sta a:$2600
        ldx #$00
        jmp ($25FF)
        inx
c46_t: txa
        CHECK_LIT 46
        lda #<c47_t ; check 47 jmp-absx MAN:S8-51: JMP (abs,X) with X = 2: PCL <- (hhll+X), PCH <- (hhll+X+1) (S8-51): the entry at +2 is taken and the decoy entry at +0 is not (Y = 0, a decoy landing gives 1, no jump gives 2)
        sta a:JT+2
        lda #>c47_t
        sta a:JT+3
        lda #<c47_d
        sta a:JT
        lda #>c47_d
        sta a:JT+1
        ldy #$00
        ldx #$02
        jmp (JT,x)
        iny
c47_d: iny
c47_t: tya
        CHECK_LIT 47
        lda #<brk_h ; check 48 rti-restores-p MAN:S8-70: RTI pulls P first (the flag row of S8-70 reads RESTORED): P = N V D Z C set and I clear before a BRK, SET just before it, so after the RTI the masked P byte is $CB
        sta a:VEC_IRQ2
        lda #>brk_h
        sta a:VEC_IRQ2+1
        stz a:BRKMODE
        stz a:BRKHIT
        tsx
        stx a:SV
        ldy #$00
        lda #$CB
        pha
        plp
        set
c48_b: brk
        iny
        php
        pla
        and #$CF
        cld
        sei
        CHECK_LIT 48
        tya ; check 49 rti-returns-brk-plus-2 MAN:S8-30: BRK pushes the address BRK+2 and RTI returns there (S8-30, S8-70): the one-byte INY at BRK+1 is skipped (Y = 0)
        CHECK_LIT 49
        lda a:SV ; check 50 rti-s-balance GUEST:s-before-equals-s-after: BRK pushes three bytes and RTI pulls three (S8-30, S8-70): S after the RTI equals S before the BRK
        sta a:GEXP
        tsx
        txa
        CHECK_GUEST 50
        lda a:BRKHIT ; check 51 brk-vector-reached MAN:S8-30: BRK reads the vector at logical $FFF6 and $FFF7 (S8-30), which the startup code routes through the IRQ2 and BRK RAM slot: the installed handler ran once
        CHECK_LIT 51
        lda #<(c48_b+2) ; check 52 brk-pushed-pcl GUEST:brk-site-plus-2: BRK pushes PC+2, the address of the BRK plus 2, as PCH then PCL (S8-30): the pushed low byte read in the handler, the expected byte computed from the assembler label of the BRK
        sta a:GEXP
        lda a:BRKL
        CHECK_GUEST 52
        lda #>(c48_b+2) ; check 53 brk-pushed-pch GUEST:brk-site-plus-2: BRK pushes PCH first (S8-30): the pushed high byte is the high byte of the BRK address plus 2
        sta a:GEXP
        lda a:BRKH
        CHECK_GUEST 53
        lda a:BRKP ; check 54 brk-pushed-b MAN:S8-30: BRK pushes P with the B flag set to 1 (S8-30): bit 4 of the pushed byte
        and #$10
        CHECK_LIT 54
        lda a:BRKP ; check 55 brk-pushed-flags MAN:S8-30: BRK pushes the status register (Ms <- P, S8-30): N V Z C of the byte pushed with P = $CB are $C3
        and #$C3
        CHECK_LIT 55
        lda a:BRKP ; check 56 brk-pushed-t0 ASSUMED:LED-CPU-0009: BRK pushes the T flag as 0 (D-99 and the flag row of S8-30 read T = 0), also right after a SET: bit 5 of the pushed byte. Open conflict, not decided here: the pushed T at a hardware interrupt entry, H8-4 and H8-5 against pcetech, is a ledger disagreement (LED-CPU-0009)
        and #$20
        CHECK_LIT 56
        lda a:BRKI ; check 57 brk-handler-flags MAN:S8-30: BRK sets I and clears D (the flag row of S8-30, H8-17): D was 1 and I was 0 before the BRK, inside the handler the masked P byte is $04
        CHECK_LIT 57
        lda a:SV ; check 58 brk-handler-s GUEST:s-minus-3: BRK pushes three bytes (S8-30): S inside the handler is S before the BRK minus 3
        sec
        sbc #$03
        sta a:GEXP
        lda a:BRKS
        CHECK_GUEST 58
        lda #$01 ; check 59 rti-loads-t ASSUMED:LED-CPU-0009: RTI loads T from bit 5 of the pulled byte (D-99; the flag row of S8-70 reads RESTORED): the handler sets bit 5 of the pushed byte, so the ORA #$80 at the return address works in T mode on the cell $2007 (X = 7) and the cell $01 becomes $81. LED-CPU-0009 carries the open T question
        sta a:$2007
        lda #$01
        sta a:BRKMODE
        brk
        nop
        ora #$80
        stz a:BRKMODE
        lda #<err
        sta a:VEC_IRQ2
        lda #>err
        sta a:VEC_IRQ2+1
        lda a:$2007
        CHECK_LIT 59
        lda #$CF ; check 60 clc MAN:S8-35: CLC clears C and no other flag: P = N V D I Z C set, the P byte masked with $CF reads $CE
        pha
        plp
        clc
        PMASK $CF
        cld
        sei
        CHECK_LIT 60
        lda #$CF ; check 61 cld MAN:S8-36: CLD clears D and no other flag: P = N V D I Z C set, the masked P byte reads $C7
        pha
        plp
        cld
        PMASK $CF
        cld
        sei
        CHECK_LIT 61
        lda #$CF ; check 62 cli MAN:S8-37: CLI clears I and no other flag: P = N V D I Z C set, the masked P byte reads $CB
        pha
        plp
        cli
        PMASK $CF
        cld
        sei
        CHECK_LIT 62
        lda #$CF ; check 63 clv MAN:S8-38: CLV clears V and no other flag: P = N V D I Z C set, the masked P byte reads $8F
        pha
        plp
        clv
        PMASK $CF
        cld
        sei
        CHECK_LIT 63
        lda #$00 ; check 64 sec MAN:S8-75: SEC sets C and no other flag: P = 0, the masked P byte reads $01
        pha
        plp
        sec
        PMASK $CF
        cld
        sei
        CHECK_LIT 64
        lda #$00 ; check 65 sed MAN:S8-76: SED sets D and no other flag: P = 0, the masked P byte reads $08
        pha
        plp
        sed
        PMASK $CF
        cld
        sei
        CHECK_LIT 65
        lda #$00 ; check 66 sei MAN:S8-77: SEI sets I and no other flag: P = 0, the masked P byte reads $04
        pha
        plp
        sei
        PMASK $CF
        cld
        sei
        CHECK_LIT 66
        lda #$C5 ; check 67 nop-keeps-a MAN:S8-57: NOP changes no register (PC <- PC+1, S8-57): A = $5A before, $5A after
        pha
        ldx #$A5
        ldy #$3C
        lda #$5A
        plp
        nop
        sta a:MR1
        stx a:MR2
        sty a:MR3
        php
        pla
        and #$CF
        sta a:MR4
        lda a:MR1
        CHECK_LIT 67
        lda a:MR2 ; check 68 nop-keeps-x MAN:S8-57: NOP changes no register (S8-57): X = $A5 before, $A5 after
        CHECK_LIT 68
        lda a:MR3 ; check 69 nop-keeps-y MAN:S8-57: NOP changes no register (S8-57): Y = $3C before, $3C after
        CHECK_LIT 69
        lda a:MR4 ; check 70 nop-keeps-flags MAN:S8-57: NOP changes no flag (the flag row of S8-57 is T = 0 and dashes): P = N V I C ($C5) before, the masked P byte after is $C5
        CHECK_LIT 70
        ldy #$00 ; check 71 nop-one-byte MAN:S8-57: NOP is one byte (S8-57): the INY right after it runs, Y = 1
        nop
        iny
        tya
        CHECK_LIT 71
        rts

; sub_a: the subroutine of the JSR checks: it reads the two bytes the JSR pushed (S points at the next free slot, so the low byte is at S+1 and the high byte at S+2)
sub_a:  tsx
        lda a:$2101,x
        sta a:SUBL
        lda a:$2102,x
        sta a:SUBH
        rts

; brk_h: the BRK handler. It records I and D as the handler sees them, S, and the three bytes the BRK pushed (P, then PCL, then PCH at rising addresses), counts its entries,
; and in mode 1 sets bit 5 of the pushed P. It leaves Y alone and returns with X = 7 (the cell of the T-mode check).
brk_h:  php
        pla
        and #$0C
        sta a:BRKI
        tsx
        stx a:BRKS
        lda a:$2101,x
        sta a:BRKP
        lda a:$2102,x
        sta a:BRKL
        lda a:$2103,x
        sta a:BRKH
        inc a:BRKHIT
        lda a:BRKMODE
        beq bh_done
        lda a:BRKP
        ora #$20
        sta a:$2101,x
bh_done: ldx #$07
        rti
