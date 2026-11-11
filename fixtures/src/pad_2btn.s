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
; The correct script holds RUN from time 0 (checks 1 to 6: the first read at about 37000 master clocks) and then changes the held set at 140000, 320000 and 510000 master clocks; the guest waits GAP 10 (about 185000 master clocks at the low speed) before each pair of reads, so the reads of the three phases sit near
; 228000, 415000 and 601000, each about 90000 master clocks from the nearest event: the placement does not depend on the exact cost of the code between the waits (the device log of the run shows the stamps, E0). The level a read sees is the level at its own access stamp (D-117), so a script event is seen by the first read at or after it.
; Every cell the checks read is written by the fixture first (D-127). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 14
DSV_CHECKS = 12

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $F7                      ; ASSUMED:LED-IO-0002: RUN held from time 0, SEL 0 CLR 0: nibble ~$8 = $7 under d6 = d7 = 1
        LIT 2, $F0                      ; ASSUMED:LED-IO-0003: the first read, before any write, has CLR set (port O is high at power-on): nibble 0
        LIT 3, $00                      ; GUEST: the table entry is an XOR mask of $00 (the expected byte is the read of check 1, kept in GEXP)
        LIT 4, $FF                      ; ASSUMED:LED-IO-0002: SEL 1, CLR 0 with only RUN held: no direction is pressed, nibble $F
        LIT 5, $F0                      ; ASSUMED:LED-IO-0003: CLR 1 (SEL 0): the nibble is 0 although RUN is held
        LIT 6, $F0                      ; ASSUMED:LED-IO-0003: CLR 1 (SEL 1): the nibble is 0
        LIT 7, $FE                      ; ASSUMED:LED-IO-0002: I and RIGHT held, buttons: ~$1 = $E
        LIT 8, $FD                      ; ASSUMED:LED-IO-0002: I and RIGHT held, directions: RIGHT is D1, ~$2 = $D
        LIT 9, $FD                      ; ASSUMED:LED-IO-0002: II, UP and LEFT held, buttons: ~$2 = $D
        LIT 10, $F6                     ; ASSUMED:LED-IO-0002: II, UP and LEFT held, directions: UP is D0 and LEFT is D3, ~$9 = $6
        LIT 11, $FB                     ; ASSUMED:LED-IO-0002: SELECT, RIGHT and DOWN held, buttons: ~$4 = $B
        LIT 12, $F9                     ; ASSUMED:LED-IO-0002: SELECT, RIGHT and DOWN held, directions: RIGHT is D1 and DOWN is D2, ~$6 = $9

SL3 = $08                       ; the TAM operand of MPR3 (logical $6000-$7FFF)
PAD = $7000                     ; the I/O window with MPR3 = $FF: a write is the latch of port O, a read is port K

; the cells of the checks (work RAM $2410 and up, outside the verdict block, the stack and the vector slots)
PWR_READ = $2410                ; the raw byte of the first read, taken before any write

; GAP n: a counted wait of n * 1536 CPU cycles (n times: LDY # 2, 255 * (DEY 2 + BNE 4) - 2, DEX 2, BNE 4), 18432 master clocks at the power-up low speed; n from 1 to 255. The events of the script are placed in the middle of these waits (see the header of the case)
        .macro GAP n
        .local outer, inner
        ldx #(n)
outer:  ldy #$FF
inner:  dey
        bne inner
        dex
        bne outer
        .endmacro

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
        sta a:GEXP                      ; kept for check 3
        CHECK_LIT 1                     ; check 1 ASSUMED:LED-IO-0002: the script holds RUN from time 0, so the buttons nibble is $7 (D3 low) and the raw byte is $F7
        lda a:PWR_READ
        CHECK_LIT 2                     ; check 2 ASSUMED:LED-IO-0003: the latch of port O is $FF at power-on (H8-18, H8-19), so SEL and CLR are both set and the nibble is 0, whatever is held
        lda a:PAD                       ; a second read with no write between: a read changes nothing, so it is the byte of check 1
        CHECK_GUEST 3                   ; check 3 GUEST:the-read-of-check-1: the expected byte is the first read, kept in GEXP
        PADRD $01                       ; SEL 1, CLR 0: the directions
        CHECK_LIT 4                     ; check 4 ASSUMED:LED-IO-0002: nothing but RUN is held, so no direction is pressed: nibble $F
        PADRD $02                       ; SEL 0, CLR 1
        CHECK_LIT 5                     ; check 5 ASSUMED:LED-IO-0003: CLR high reads the nibble as 0 although RUN is held
        PADRD $03                       ; SEL 1, CLR 1
        CHECK_LIT 6                     ; check 6 ASSUMED:LED-IO-0003: CLR high reads the nibble as 0 with SEL high too
; ---- the held set changes: each phase waits for the next event of the script, then reads the buttons and the directions ----
        GAP 10
        PADRD $00
        CHECK_LIT 7                     ; check 7 ASSUMED:LED-IO-0002: the buttons of phase 2: I held (D0 low), II SELECT RUN not: $FE
        PADRD $01
        CHECK_LIT 8                     ; check 8 ASSUMED:LED-IO-0002: the directions of phase 2: RIGHT held (D1 low): $FD
        GAP 10
        PADRD $00
        CHECK_LIT 9                     ; check 9 ASSUMED:LED-IO-0002: the buttons of phase 3: II held (D1 low): $FD
        PADRD $01
        CHECK_LIT 10                    ; check 10 ASSUMED:LED-IO-0002: the directions of phase 3: UP (D0) and LEFT (D3) held: $F6
        GAP 10
        PADRD $00
        CHECK_LIT 11                    ; check 11 ASSUMED:LED-IO-0002: the buttons of phase 4: SELECT held (D2 low): $FB
        PADRD $01
        CHECK_LIT 12                    ; check 12 ASSUMED:LED-IO-0002: the directions of phase 4: RIGHT (D1) and DOWN (D2) held: $F9
        rts
