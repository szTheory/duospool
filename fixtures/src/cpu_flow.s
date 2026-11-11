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
DSV_CHECKS = 8

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
        rts

; sub_a: the subroutine of the JSR checks: it reads the two bytes the JSR pushed (S points at the next free slot, so the low byte is at S+1 and the high byte at S+2)
sub_a:  tsx
        lda a:$2101,x
        sta a:SUBL
        lda a:$2102,x
        sta a:SUBH
        rts
