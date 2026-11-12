; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; verdict.inc - the DSV1 verdict block routines (plan 03-05; D-124). The runner (tests/conformance/guestkinds.c) judges the block from raw work RAM.
; Layout, one block of 256 bytes at work-RAM offset $1F00 (logical $5F00 while MPR2 = $F9, see startup.inc):
;   +00 "DSV1"   +04 state ($01 running, $A5 done)   +05 complement of the state   +06 version 1   +07 n_checks   +08 fixture id
;   +09 pass   +0A fail   +0B notrun   +0C progress   +0D error flags   +10..+6F result per check ($00 not run, $50 pass, $46 fail)
;   +80..+DF observed byte per check   +E0 +E1 16-bit little-endian sum of +06..+DF   +FE +FF trailer $5A $A5   every other byte zero.
; The sum is written before the state flips to done, and the state is written last; the guest then executes SEI and branches to itself.
;
; The protocol-control variants of ds_proto (plan 03-05, D-124) are ca65 -D switches read here and nowhere else; none is defined in a normal build:
;   DSV_PREDONE  the guest seals the block (checksum, then done) before its checks run, so the results written afterwards make the checksum stale
;   DSV_NODONE   the guest never writes the done state
;   DSV_BADSUM   the guest writes a checksum that is one too large
;   DSV_WRONGID  the guest writes the fixture id 2 instead of DSV_ID
; Requires DSV_ID and DSV_CHECKS (defined by the fixture before startup.inc) and the VERDICT address of startup.inc.
        .setcpu "huc6280"

        .ifdef DSV_WRONGID
DSV_ID_WRITTEN = $02
        .else
DSV_ID_WRITTEN = DSV_ID
        .endif

; dsv_init: the reset code has zeroed the 256-byte block; write the magic, version, id, counts, trailer and the RUNNING state (complement first, state last).
dsv_init:
        ldx #$FC                ; the magic: X runs $FC to $FF
dsv_ml: lda a:dsv_magic-$FC,x
        sta a:VERDICT-$FC,x
        inx
        bne dsv_ml
        lda #$01                ; version 1
        sta a:VERDICT+$06
        lda #DSV_ID_WRITTEN
        sta a:VERDICT+$08
        lda #DSV_CHECKS         ; n_checks, and notrun starts at n_checks
        sta a:VERDICT+$07
        sta a:VERDICT+$0B
        lda #$5A                ; trailer
        sta a:VERDICT+$FE
        lda #$A5
        sta a:VERDICT+$FF
        lda #$FE                ; state RUNNING: the complement $FE first, then $01
        sta a:VERDICT+$05
        lda #$01
        sta a:VERDICT+$04
        .ifdef DSV_PREDONE
        jsr dsv_seal            ; done before any check ran
        .endif
        rts

dsv_magic:
        .byte $44, $53, $56, $31

; dsv_seal: the checksum of +06..+DF (218 bytes, X runs $26 to $FF) and then the done state, last.
dsv_seal:
        ldx #$26
dsv_sl: lda a:VERDICT+$E0
        clc
        adc a:VERDICT+$06-$26,x
        sta a:VERDICT+$E0
        lda a:VERDICT+$E1
        adc #$00
        sta a:VERDICT+$E1
        inx
        bne dsv_sl
        .ifdef DSV_BADSUM
        inc a:VERDICT+$E0       ; a checksum that is one too large
        .endif
        .ifndef DSV_NODONE
        lda #$5A                ; the complement first, then the state
        sta a:VERDICT+$05
        lda #$A5
        sta a:VERDICT+$04
        .endif
        rts

; dsv_finish: seal the block (unless it was sealed early), then SEI and branch to itself.
dsv_finish:
        .ifndef DSV_PREDONE
        jsr dsv_seal
        .endif
        sei
dsv_spin:   bra dsv_spin
