; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; check.inc - the fixture check macros and the literal table (plan 03-05; D-124, D-125, D-126). Included after startup.inc and verdict.inc.
; Every check compares an observed byte with an expected value that is read from a literal table in ROM, at the fixed address $F000 (the
; segment CHECKLIT, placed by hucard.cfg: bank 0, file offset $1000). The mutation sweep patches one entry of that table in an in-memory
; copy of the image and expects exactly that check to turn to $46 (D-126), so every class of check has a table entry:
;   table  +00 "DCK1"  +04 count n  then n entries of two bytes, entry k at +05+2*(k-1):  +0 check number k (1-based), +1 value
;   CHECK_LIT k     A = the observed byte; the entry value is the expected byte itself (a MAN-class or ASSUMED-class literal)
;   CHECK_GUEST k   A = the observed byte; the guest has stored its own computed expected byte in GEXP; the entry value is an XOR mask
;                   ($00 in the shipped fixture) applied to GEXP, so a patched mask turns the check red (a GUEST-class check)
;   CHECK_OBSERVE k, actual   an observe-only row (plan 03-10): a check whose expected value legitimately differs by case (a model parameter value, a policy, a variant). It does NOT compare: it stores
;                   the observed byte (the operand `actual` loaded into A, or A itself when the operand is left out) at +80+(k-1) and $50 at +10+(k-1), and tallies a pass. The runner judges the byte
;                   from the case (the item observe=IDX:VALUE of the expect). The table still holds the entry for k (a fixture writes LIT k, $00 as for every check, so the table stays one entry
;                   per check and the build fails when it is missing), and the sweep skips the index (the fifth column of NAME.checks is the word observe). The macro emits nothing until it is used.
; Each macro stores the observed byte at +80+(k-1), then writes the result byte ($50 pass, $46 fail) at +10+(k-1) and tallies pass, fail,
; notrun and progress through ck_pass or ck_fail. X and A are clobbered. A fixture calls CHECK_TABLE n once, before its first LIT.
; Macro parameters are parenthesised where they enter an expression (Pitfall 5); the check number is a plain number so that .string can
; build the symbol cklit_N, and a LIT whose position in the table is not the one CHECK_* reads fails the build (.assert).
        .setcpu "huc6280"

CKLIT_ADDR = $F000              ; the logical address of the table while MPR7 = $00 (bank 0)

        .macro CHECK_TABLE n
        .pushseg
        .segment "CHECKLIT"
cklit_hdr:
        .byte $44, $43, $4B, $31        ; "DCK1"
        .byte (n)
        .popseg
        .endmacro

        .macro LIT k, value
        .pushseg
        .segment "CHECKLIT"
        .ident(.concat("cklit_", .string(k))):
        .byte (k), (value)
        .assert .ident(.concat("cklit_", .string(k))) - cklit_hdr = 5 + 2 * ((k) - 1), error, "LIT entries must be in check order"
        .popseg
        .endmacro

        .macro CHECK_RESULT
        .local pass, done
        beq pass
        jsr ck_fail
        bra done
pass:   jsr ck_pass
done:
        .endmacro

        .macro CHECK_LIT k
        ldx #((k) - 1)
        sta a:VERDICT+$80,x             ; the observed byte
        cmp a:.ident(.concat("cklit_", .string(k))) + 1
        CHECK_RESULT
        .endmacro

        .macro CHECK_GUEST k
        ldx #((k) - 1)
        sta a:VERDICT+$80,x             ; the observed byte
        lda a:GEXP                      ; the guest-computed expected byte, XOR the table mask
        eor a:.ident(.concat("cklit_", .string(k))) + 1
        cmp a:VERDICT+$80+(k)-1         ; equal when the observed byte is the expected byte
        CHECK_RESULT
        .endmacro

        .macro CHECK_OBSERVE k, actual
        .assert .ident(.concat("cklit_", .string(k))) - cklit_hdr = 5 + 2 * ((k) - 1), error, "an observe row still needs its LIT entry in check order"
        .ifnblank actual
        lda actual                      ; the observed byte
        .endif
        ldx #((k) - 1)
        sta a:VERDICT+$80,x             ; the observed byte
        jsr ck_pass                     ; result $50: nothing is compared here, the runner judges the byte
        .endmacro

; ck_pass and ck_fail: X = the zero-based check index. They write the result byte, count it, and tally progress.
ck_pass:
        lda #$50
        sta a:VERDICT+$10,x
        inc a:VERDICT+$09
        bra ck_tally
ck_fail:
        lda #$46
        sta a:VERDICT+$10,x
        inc a:VERDICT+$0A
ck_tally:
        lda a:VERDICT+$0B               ; notrun - 1, as an add of $FF
        clc
        adc #$FF
        sta a:VERDICT+$0B
        inc a:VERDICT+$0C               ; progress + 1
        rts
