; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; pad_2btn.s - the two-button pad seen from the guest through its own SEL and CLR reads of the I/O window (plan 03-27; D-115, D-116, D-117, D-118, D-124, D-125, ER-5, R-5, R-7; H8-18, H8-19, CDBIOS-p28). It is assembled by ca65 in the gate and never committed as a binary.
; The I/O window is logical $7000 with MPR3 = $FF (physical $1FF000, H8-19: a write cycle drives port O, a read cycle reads port K). The guest sees the input only through these reads: it writes the latch of port O, waits, and reads the byte. The protocol is a modelling assumption of
; this project, labelled ASSUMED LED-IO-0002 to LED-IO-0004 (evidence class E2: the pad core and this file are written by one party, ER-5): bit 0 of the written byte is SEL and bit 1 is CLR; a read is $30 | d6 << 6 | d7 << 7 | nibble; the nibble is 0 while CLR is 1, otherwise the inverted
; directions while SEL is 1 (D0 UP, D1 RIGHT, D2 DOWN, D3 LEFT) or the inverted buttons while SEL is 0 (D0 I, D1 II, D2 SELECT, D3 RUN), a pressed button reading 0; port O powers on high (H8-18, H8-19), so the first read before any write has CLR set. d6 and d7 are the values the run request
; binds (LED-IO-0001, open): every literal below assumes the default binding d6 = d7 = 1, so a raw byte is $F0 | nibble. One report orders the direction nibble differently (RIGHT and DOWN swapped, pcetech.txt); this file follows the other three sources and that disagreement is recorded in the
; ledger entry of the protocol, not decided here (ER-8). The logical button mask of a script event is the CD BIOS layout (CDBIOS-p28: I 0, II 1, SELECT 2, RUN 3, UP 4, RIGHT 5, DOWN 6, LEFT 7, 1 = pressed), the one fact of the pad that a manual page states.
; Every read is preceded by a write of the latch and a wait of at least nine CPU cycles (five NOPs, ten cycles), as the community says a real pad needs (no settling model in the core, D-115); a file that omitted the wait must not be labelled hardware-valid.
; The raw byte of every read is stored as the observed byte of its check, so the DSV1 block holds the guest-side pad-read log of the run (R-7); no byte is masked before it is stored. The input script is declared by the case (the key input, a file of tests/conformance/inputs): the machine runs this file
; unchanged against the correct script and, in plan 03-29, against a script with no input and one with the wrong input, whose different verdicts show that the guest really reads the pad.
; Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 14
DSV_CHECKS = 2

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $F7                      ; ASSUMED:LED-IO-0002: RUN held from time 0, SEL 0 CLR 0: nibble ~$8 = $7 under d6 = d7 = 1
        LIT 2, $F0                      ; ASSUMED:LED-IO-0003: the first read, before any write, has CLR set (port O is high at power-on): nibble 0

SL3 = $08                       ; the TAM operand of MPR3 (logical $6000-$7FFF)
PAD = $7000                     ; the I/O window with MPR3 = $FF: a write is the latch of port O, a read is port K

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
PWR_READ = $2410                ; the raw byte of the first read, taken before any write

; WAIT9: at least nine CPU cycles between the write of the latch and the read (five NOPs of two cycles each)
        .macro WAIT9
        nop
        nop
        nop
        nop
        nop
        .endmacro

; PADRD latch: write the latch of port O, wait, and read the window into A (the raw byte)
        .macro PADRD latch
        lda #(latch)
        sta a:PAD
        WAIT9
        lda a:PAD
        .endmacro

fixture_main:
        lda #$FF
        tam #SL3                        ; MPR3 = $FF: the hardware page at logical $6000, the I/O window at $7000
        lda a:PAD                       ; the power-on read, before any write: kept for check 2
        sta a:PWR_READ
        PADRD $00                       ; SEL 0, CLR 0: the buttons
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-IO-0002: the script holds RUN from time 0, so the buttons nibble is $7 (D3 low) and the raw byte is $F7
        lda a:PWR_READ
        CHECK_LIT 2                     ; check 2 ASSUMED:LED-IO-0003: the latch of port O is $FF at power-on (H8-18, H8-19), so SEL and CLR are both set and the nibble is 0, whatever is held
        rts
