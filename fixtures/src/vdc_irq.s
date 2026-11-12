; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_irq.s - guest-recorded interrupt measurements (plan 04-17; D-174, D-136, D-139, D-142, D-143, D-169, ER-3, ER-10, F-19, F-38, F-41, H7-4, H7-7). It is assembled by ca65 in the gate and never committed as a binary; -D LINES=263 builds the 263-line variant (the default is 262), which differs only in the frame-period literal.
; The VDC has no readable scanline counter, so the guest measures time with the spin counter of spin.inc (inc a, bra: 6 cycles, 18 master clocks per iteration) and an IRQ1 handler that samples A first and then reads the status (the acknowledge). No absolute line or frame number is asserted (D-136): the recorded values are iteration counts between handler entries,
; and the "phase" of an event is those iterations converted by the printed formula iterations * 18 master clocks. With VPR = $0F02, VDW = 239, VCR = 4, HDS = 2, HDW = 31 and divider 4 the RCR counter is 64 on the first display line, 303 on the last one and keeps counting through the vertical blank and the frame wrap up to 64 + frame_lines - 1; every line has an EV_RCR (D-142).
; The chain. The IRQ1 handlers program RCR for the next event from a table (NXT_LO, NXT_HI), so that two samples are always less than 256 iterations apart. One cycle of the chain is 95 events: RCR 64 to 97 every third line, 100 to 108 on consecutive lines (the ratio rows), 110, 113 to 200 every third line, 203 to 299 every third line, 301, 303 (the last display line),
; then the VBlank interrupt (the VD event, which reprograms RCR to 306) and 306 to 324 every third line, after which RCR is set to 64 again. The handler of the last display line is the minimal variant h_last: it samples, reads the status, logs both and selects the next handler, and does not program RCR (the VD handler does), so that it returns before VD (below).
; Each handler stores its sample (A) and its status byte at the event index Y of the current cycle (Y is not touched by the foreground; the logs are 256-byte pages, one per cycle: samples at $2800, statuses at $2D00). The handler of the first event of every cycle (h_end) advances the pages and counts the cycle; after the fifth cycle starts it disables the VDC
; interrupts and redirects its return
; address to fin (a stack write), so the foreground loop, which has no exit, ends. After that the foreground sums the deltas of the samples (24 bits, clc/adc), judges the guest checks and writes the raw measurements as observe rows (D-169).
; Cycle sums (each is the table cycle of tests/oracle/huc6280-opcodes.tsv plus one penalty cycle for every VDC window access; entry 8, jmp (VEC_IRQ1) 7, RTI 7, see spin.inc; the sum is asserted to be a multiple of 6):
;   h_gen  sta (zp),y 7; tax 2; lda VDC_AR 5+1; sta (zp),y 7; st0 4+1; lda abs,y 5; sta VDC_LO 5+1; lda abs,y 5; sta VDC_HI 5+1; lda abs,y 5; sta abs 5; sax 3; clc 2; adc # 2; iny 2 = 68; with 22 of fixed cost = 90 = 15 * 6 (K = 15)
;   h_last sta (zp),y 7; tax 2; lda VDC_AR 5+1; sta (zp),y 7; lda abs,y 5; sta abs 5; txa 2; clc 2; adc # 2; iny 2; nop 2; nop 2 = 44; with 22 = 66 = 11 * 6 (K = 11). The wait before the entry is under 5 cycles (the sample points of the loop are 1 and 5 cycles into an iteration and an event is taken at the first sample point at or after it, then
;          the instruction ends 1 cycle later), so the RTI ends under 71 cycles after the event, before the next event of the chain, VD, which is 245 master clocks = 81.7 cycles after the RCR event by default: the last-display-line handler returns first (A10 of 04-VALIDATION.md).
;   h_end  sta zp 4; tax 2; sxy 3; lda VDC_AR 5+1; inc zp 6; inc zp 6; inc zp 6; ldy # 2; sta (zp),y 7; lda zp 4; sta (zp),y 7; st0 4+1; lda abs,y 5; sta VDC_LO 5+1; lda abs,y 5; sta VDC_HI 5+1; lda abs,y 5; sta abs 5; lda zp 4; clc 2; adc # 2; iny 2; ldx zp 4; cpx # 2; beq (not taken) 2; nop 2 = 110; with 22 = 132 = 22 * 6 (K = 22)
; The status read is 9 cycles after the first instruction of h_gen, h_last and h_end (tax 2 after the first store, or tax and sxy after the store to SMP), so the dev log read stamps of two handlers differ by the difference of their boundaries.
; The classes: MAN (H7-4: flag layout, RR and VD alone, cleared by the read), GUEST (the ratio: no constant is assumed), ASSUMED LED-VID-0001 (the 1365-clock line and the frame length enter the line and frame period rows and the two-line bound; ER-3: the 3-clock cycle, LED-CPU-0009 entry cycles and LED-CPU-0017 window penalty enter K), ASSUMED LED-IRQ-0009
; (level re-entry), LED-VID-0015 (bits 6 and 7).
; Every cell the checks read is written by the fixture first, and the registers are 0 at power-on under every policy (D-167), so the check bytes are equal under every power-on policy. No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 18
DSV_CHECKS = 16

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"
        .include "spin.inc"

        .ifndef LINES
LINES = 262                     ; the bound frame length of the image (F-38); -D LINES=263 builds the other
        .endif

NE = 95                         ; events per cycle
IA = 12                         ; the event of RCR 100 (a), a + 1 is event 13 and a + 8 event 20
IB = 51                         ; the event of RCR 200 (a + 100)
IL = 86                         ; the event of RCR 303, the last display line
IV = 87                         ; the VD event
NCYC = 4                        ; cycles judged; the fifth cycle start ends the run
LOGBASE = $2800                 ; sample log: one 256-byte page per cycle (five pages)
STBASE = $2D00                  ; status log
RATIO_TOL = 12                  ; iterations: 1.33 (sample jitter) + 8 * 1.33 (the one-line interval scaled by 8) = 12, derived in the check rows below

LP_EXP = (100 * 1365 + 9) / 18  ; iterations in 100 lines (ASSUMED LED-VID-0001), rounded to the nearest
FP_EXP = (4 * LINES * 1365 + 9) / 18 ; iterations in 4 frames (ASSUMED LED-VID-0001), rounded

; zero page (logical $2000 + n)
LOGP = $00                      ; pointer to the sample page of the cycle
STP = $02                       ; pointer to the status page
SP = $04                        ; analysis: sample pointer
SQ = $06                        ; analysis: status pointer
SMP = $08
FRC = $09                       ; cycle counter
NA = $0A                        ; no-acknowledge entries
ERR = $0B
ORS = $0C
YEND = $0D
TT = $0E
BADG = $0F
DV1 = $10
FF = $11
G1A = $12
G1B = $13
G2L = $14
G2H = $15
C0 = $18
C1 = $19
C2 = $1A
E0 = $1B
E1 = $1C
E2 = $1D
D0 = $1E
D1 = $1F
D2 = $20

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                ; GUEST:ratio-8-lines (frame 0) the iterations from RCR = 100 to RCR = 108 equal 8 times those from RCR = 100 to 101 within 12 iterations; the guest sets the expected byte 0 itself
        LIT 2, $00                ; GUEST:ratio-8-lines (frame 1) the same in the second cycle
        LIT 3, $00                ; ASSUMED:LED-VID-0001 the iterations from RCR = 100 to RCR = 200 (100 lines) are 100 * 1365 / 18 within 2 (frame 0)
        LIT 4, $00                ; ASSUMED:LED-VID-0001 the same in frame 1
        LIT 5, $00                ; ASSUMED:LED-VID-0001 the iterations from the first RCR = 64 to the fifth (4 frames) are 4 * frame_lines * 1365 / 18 within 2
        LIT 6, $00                ; ASSUMED:LED-VID-0001 in each of four frames the handler of the last display line read RR alone ($04), the next handler read VD alone ($20), and its sample is 1 to 151 iterations (under two lines) later
        LIT 7, $00                ; MAN:H7-4 every handler of four cycles read the one flag of its event: $04 (RR) for the RCR events and $20 (VD) for the VBlank event, the OR of the differences is 0
        LIT 8, $04                ; MAN:H7-4 the status read in the handler of the last display line RCR (frame 0) is $04: RR alone
        LIT 9, $20                ; MAN:H7-4 the status read in the next handler (frame 0) is $20: VD alone
        LIT 10, $00               ; ASSUMED:LED-VID-0015 bits 6 and 7 of every status byte read in the four cycles are 0
        LIT 11, $00               ; GUEST:cycle-count the guest ended the chain itself at the start of the fifth cycle: the cycle counter equals the 4 the guest expects (the table entry of a GUEST row is the XOR mask $00)
        LIT 12, $05               ; ASSUMED:LED-IRQ-0009 a handler that does not read the status is entered again and again (the level stays): five entries, the fifth acknowledges
        LIT 13, $00               ; observe: iterations from the last-display-line RCR sample to the VD sample, frame 0 (judged by vdc_irq_agree)
        LIT 14, $00               ; observe: the same, frame 1
        LIT 15, $00               ; observe: iterations from the VD sample of frame 0 to the first RCR = 64 sample of frame 1, low byte
        LIT 16, $00               ; observe: the same, high byte

IRQ_REQ = $1403
NA_LIMIT = 5

; G1 page, dst: the iterations from the sample of the last display line RCR handler to that of the VD handler of the page
        .macro G1 page, dst
        stz z:SP
        lda #(page)
        sta z:SP+1
        ldy #IL
        lda (SP),y
        sta z:TT
        iny
        lda (SP),y
        sec
        sbc z:TT
        sta z:dst
        .endmacro

; RATIO page: C = the iterations of 8 lines from event IA, E = 8 times the one line interval; A = 0 when they agree within RATIO_TOL
        .macro RATIO page
        stz z:SP
        lda #(page)
        sta z:SP+1
        ldy #IA
        lda (SP),y
        sta z:TT
        iny
        lda (SP),y
        sec
        sbc z:TT
        sta z:DV1                       ; the one-line interval d1
        stz z:C0
        stz z:C1
        stz z:C2
        ldy #IA
        lda #IA + 8
        sta z:YEND
        jsr sumd                        ; C = the interval of 8 lines
        lda z:DV1
        sta z:E0
        stz z:E1
        stz z:E2
        asl z:E0
        rol z:E1
        asl z:E0
        rol z:E1
        asl z:E0
        rol z:E1                        ; E = 8 * d1
        ldx #RATIO_TOL
        jsr win
        .endmacro

; LPER page: the iterations from RCR = 100 to RCR = 200 against LP_EXP within 2
        .macro LPER page
        stz z:SP
        lda #(page)
        sta z:SP+1
        stz z:C0
        stz z:C1
        stz z:C2
        ldy #IA
        lda #IB
        sta z:YEND
        jsr sumd
        lda #<LP_EXP
        sta z:E0
        lda #>LP_EXP
        sta z:E1
        lda #^LP_EXP
        sta z:E2
        ldx #2
        jsr win
        .endmacro


fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        stz z:NA
        stz z:LOGP
        stz z:STP
        lda #>(LOGBASE - $100)
        sta z:LOGP+1
        lda #>(STBASE - $100)
        sta z:STP+1
        lda #$FF
        sta z:FRC                       ; the first h_end brings it to 0
        lda #<h_sync
        sta a:VEC_IRQ1
        lda #>h_sync
        sta a:VEC_IRQ1+1                ; the handlers share one page: only the low byte of the vector is written afterwards
        csh                             ; the high speed: 3 master clocks per CPU cycle
        VDC_REG 12, $0F02               ; VPR: VSW = 2, VDS = 15
        VDC_REG 13, 239                 ; VDW = 239
        VDC_REG 14, 4                   ; VCR = 4
        VDC_REG 10, $0200               ; HSR: HSW = 0, HDS = 2
        VDC_REG 11, $001F               ; HDR: HDW = 31, HDE = 0
        VDC_REG 6, 64                   ; RCR = 64: the first display line (the counter is 16384 until then)
        VDC_REG 5, $0004                ; CR: RR (bit 2) enabled; VD follows in h_sync
        cla
        cli
        SPIN_LOOP

; ---- the handlers ----
h_gen:
        sta (LOGP),y                    ; 7  the sample
        tax                             ; 2
        lda a:VDC_AR                    ; 6  the status read (5 + 1 window penalty), the acknowledge
        sta (STP),y                     ; 7
        st0 #6                          ; 5  AR = RCR
        lda a:NXT_LO,y                  ; 5
        sta a:VDC_LO                    ; 6
        lda a:NXT_HI,y                  ; 5
        sta a:VDC_HI                    ; 6  the RCR of the next event
        lda a:NXT_VEC,y                 ; 5
        sta a:VEC_IRQ1                  ; 5  the handler of the next event
        sax                             ; 3  A = the sample (X = scratch)
        clc                             ; 2
        adc #K_GEN                      ; 2
        iny                             ; 2
        rti
BODY_GEN = 68
K_GEN = (SPIN_FIXED + BODY_GEN) / SPIN_P
        .assert ((SPIN_FIXED + BODY_GEN) .mod SPIN_P) = 0, error, "h_gen is not a multiple of 6 cycles"

h_last:
        sta (LOGP),y                    ; 7
        tax                             ; 2
        lda a:VDC_AR                    ; 6
        sta (STP),y                     ; 7
        lda a:NXT_VEC,y                 ; 5
        sta a:VEC_IRQ1                  ; 5
        txa                             ; 2
        clc                             ; 2
        adc #K_LAST                     ; 2
        iny                             ; 2
        nop                             ; 2
        nop                             ; 2
        rti
BODY_LAST = 44
K_LAST = (SPIN_FIXED + BODY_LAST) / SPIN_P
        .assert ((SPIN_FIXED + BODY_LAST) .mod SPIN_P) = 0, error, "h_last is not a multiple of 6 cycles"
        .assert (SPIN_FIXED + BODY_LAST) <= 72, error, "h_last must return before VD: entry + jmp + body + RTI at most 72 cycles"

h_end:
        sta z:SMP                       ; 4
        tax                             ; 2
        sxy                             ; 3  (the odd pad: the status read is 9 cycles into every measured handler)
        lda a:VDC_AR                    ; 6
        inc z:LOGP+1                    ; 6  the next pages
        inc z:STP+1                     ; 6
        inc z:FRC                       ; 6
        ldy #0                          ; 2
        sta (STP),y                     ; 7
        lda z:SMP                       ; 4
        sta (LOGP),y                    ; 7
        st0 #6                          ; 5
        lda a:NXT_LO,y                  ; 5
        sta a:VDC_LO                    ; 6
        lda a:NXT_HI,y                  ; 5
        sta a:VDC_HI                    ; 6
        lda a:NXT_VEC,y                 ; 5
        sta a:VEC_IRQ1                  ; 5
        lda z:SMP                       ; 4
        clc                             ; 2
        adc #K_END                      ; 2
        iny                             ; 2
        ldx z:FRC                       ; 4
        cpx #NCYC                       ; 2
        beq h_fin                       ; 2 not taken
        nop                             ; 2
        rti
BODY_END = 110
K_END = (SPIN_FIXED + BODY_END) / SPIN_P
        .assert ((SPIN_FIXED + BODY_END) .mod SPIN_P) = 0, error, "h_end is not a multiple of 6 cycles"

; h_fin: the end of the chain. The VDC interrupts are disabled, the vector is made harmless and the return address on the stack (P, PCL, PCH from S + 1) is replaced by fin
h_fin:
        VDC_REG 5, $0000
        lda #<h_dead
        sta a:VEC_IRQ1
        tsx
        lda #<fin
        sta a:$2102,x
        lda #>fin
        sta a:$2103,x
        rti

h_dead:
        rti

; h_sync: the first RCR = 64 match. The vertical registers are written when the machine is already some lines into frame 0 (the reset code runs at the low speed first), so the first display window is cut by the frame wrap and its counter run is not a whole frame: this match is consumed here,
; RCR stays 64, and the next match (the first display line of the next frame) is the first event of cycle 0. VD is enabled from here on. Nothing is measured across this handler: the first sample of the chain is taken by h_end
h_sync:
        lda a:VDC_AR
        VDC_REG 5, $000C
        lda #<h_end
        sta a:VEC_IRQ1
        rti

; h_noack: the handler of the level experiment: it does not read the status until it has been entered NA_LIMIT times
h_noack:
        inc z:NA
        lda z:NA
        cmp #NA_LIMIT
        bcc hn_r
        lda a:VDC_AR
hn_r:   rti

        .assert (>h_gen) = (>h_last), lderror, "the handlers must share a page"
        .assert (>h_gen) = (>h_end), lderror, "the handlers must share a page"
        .assert (>h_gen) = (>h_dead), lderror, "the handlers must share a page"
        .assert (>h_gen) = (>h_sync), lderror, "the handlers must share a page"
        .assert (>h_gen) = (>h_noack), lderror, "the handlers must share a page"

; ---- the foreground after the chain ----
fin:
; the level experiment first (the counter is at the first display line): RR enabled, RCR = 68 four lines later, the handler does not acknowledge until the fifth entry
        lda #<h_noack
        sta a:VEC_IRQ1
        stz z:NA
        VDC_REG 6, 68
        VDC_REG 5, $0004
        ldx #0
        ldy #3
fw:     lda z:NA
        cmp #NA_LIMIT
        bcs fwd
        dex
        bne fw
        dey
        bne fw
fwd:    sei
        VDC_REG 5, $0000

; the sums of the deltas
        RATIO >LOGBASE
        stz a:GEXP
        CHECK_GUEST 1
        RATIO (>LOGBASE) + 1
        stz a:GEXP
        CHECK_GUEST 2
        LPER >LOGBASE
        CHECK_LIT 3
        LPER (>LOGBASE) + 1
        CHECK_LIT 4
; four frames
        stz z:C0
        stz z:C1
        stz z:C2
        stz z:FF
        stz z:SP
        lda #>LOGBASE
        sta z:SP+1
fp_lp:  ldy #0
        lda #NE - 1
        sta z:YEND
        jsr sumd
        lda (SP),y                      ; the last sample of the cycle
        sta z:TT
        inc z:SP+1
        ldy #0
        lda (SP),y                      ; the first sample of the next cycle
        sec
        sbc z:TT
        ADD24 C0
        inc z:FF
        lda z:FF
        cmp #4
        bcc fp_lp
        lda #<FP_EXP
        sta z:E0
        lda #>FP_EXP
        sta z:E1
        lda #^FP_EXP
        sta z:E2
        ldx #2
        jsr win
        CHECK_LIT 5
; the order of the last display line RCR and VD in four frames
        stz z:BADG
        stz z:SP
        lda #>LOGBASE
        sta z:SP+1
        stz z:SQ
        lda #>STBASE
        sta z:SQ+1
        stz z:FF
g_lp:   ldy #IL
        lda (SP),y
        sta z:TT
        iny
        lda (SP),y                      ; the sample of the VD handler
        sec
        sbc z:TT                        ; the iterations since the sample of the last display line RCR handler
        beq g_bad
        cmp #152                        ; 2 * 1365 / 18 = 151.7: under two lines (ASSUMED LED-VID-0001)
        bcs g_bad
        lda (SQ),y
        eor #$20                        ; the VD handler read VD alone
        bne g_bad
        dey
        lda (SQ),y
        eor #$04                        ; the handler before it read RR alone
        beq g_ok
g_bad:  lda #1
        sta z:BADG
g_ok:   inc z:SP+1
        inc z:SQ+1
        inc z:FF
        lda z:FF
        cmp #4
        bcc g_lp
        lda z:BADG
        CHECK_LIT 6
; every status byte of four cycles
        stz z:ERR
        stz z:ORS
        stz z:SQ
        lda #>STBASE
        sta z:SQ+1
st_lp:  ldy #0
st_in:  lda (SQ),y
        sta z:TT
        ora z:ORS
        sta z:ORS
        lda z:TT
        eor a:EXPT,y
        ora z:ERR
        sta z:ERR
        iny
        cpy #NE
        bne st_in
        inc z:SQ+1
        lda z:SQ+1
        cmp #(>STBASE) + 4
        bcc st_lp
        lda z:ERR
        CHECK_LIT 7
        lda a:STBASE + IL
        CHECK_LIT 8
        lda a:STBASE + IV
        CHECK_LIT 9
        lda z:ORS
        and #$C0
        CHECK_LIT 10
        lda #NCYC
        sta a:GEXP
        lda z:FRC                       ; the counter of the cycle start that ended the chain
        CHECK_GUEST 11
        lda z:NA
        CHECK_LIT 12
; the raw measurements (observe rows)
        G1 >LOGBASE, G1A
        G1 (>LOGBASE) + 1, G1B
        stz z:C0
        stz z:C1
        stz z:C2
        stz z:SP
        lda #>LOGBASE
        sta z:SP+1
        ldy #IV
        lda #NE - 1
        sta z:YEND
        jsr sumd
        lda (SP),y
        sta z:TT
        inc z:SP+1
        ldy #0
        lda (SP),y
        sec
        sbc z:TT
        ADD24 C0
        lda z:C0
        sta z:G2L
        lda z:C1
        sta z:G2H
        CHECK_OBSERVE 13, z:G1A
        CHECK_OBSERVE 14, z:G1B
        CHECK_OBSERVE 15, z:G2L
        CHECK_OBSERVE 16, z:G2H
        rts

; sumd: adds to C the deltas (s(n) - s(n-1)) mod 256 of the samples at the indices Y + 1 to YEND of the page SP. Out: Y = YEND
sumd:   lda (SP),y
        sta z:TT
        iny
        lda (SP),y
        sec
        sbc z:TT
        ADD24 C0
        cpy z:YEND
        bne sumd
        rts

; win: A = 0 when |C - E| <= X, else 1 (C and E are 24-bit values). Clobbers A and D
win:    sec
        lda z:C0
        sbc z:E0
        sta z:D0
        lda z:C1
        sbc z:E1
        sta z:D1
        lda z:C2
        sbc z:E2
        sta z:D2
        clc
        txa
        adc z:D0
        sta z:D0
        lda z:D1
        adc #0
        sta z:D1
        lda z:D2
        adc #0
        sta z:D2                        ; D = C - E + X: inside the tolerance when 0 <= D <= 2 * X
        lda z:D1
        ora z:D2
        bne w_bad
        txa
        asl a
        cmp z:D0
        bcc w_bad
        lda #0
        rts
w_bad:  lda #1
        rts

; ---- the event tables: index = the current event, value = what the handler of this event programs for the next one ----
        .segment "RODATA"

; EMIT which, nxt, vec, ex: one byte of table which (0 NXT_LO, 1 NXT_HI, 2 NXT_VEC, 3 EXPT)
        .macro EMIT which, nxt, vec, ex
        .if (which) = 0
        .byte <(nxt)
        .elseif (which) = 1
        .byte >(nxt)
        .elseif (which) = 2
        .byte <(vec)
        .else
        .byte (ex)
        .endif
        .endmacro

        .macro TABLE which
        .repeat 12, i                   ; events 0 to 11: RCR 64 to 97, then 100 next
        EMIT which, 3 * (i) + 67, h_gen, $04
        .endrepeat
        .repeat 8, i                    ; events 12 to 19: RCR 100 to 107, then 101 to 108 next (consecutive lines)
        EMIT which, (i) + 101, h_gen, $04
        .endrepeat
        EMIT which, 110, h_gen, $04     ; event 20: RCR 108
        .repeat 63, i                   ; events 21 to 83: RCR 110, 113 to 296, then 113 to 299 next
        EMIT which, 3 * (i) + 113, h_gen, $04
        .endrepeat
        EMIT which, 301, h_gen, $04     ; event 84: RCR 299
        EMIT which, 303, h_last, $04    ; event 85: RCR 301, the next handler is the last display line one
        EMIT which, 303, h_gen, $04     ; event 86: RCR 303, the last display line: h_last programs nothing
        EMIT which, 306, h_gen, $20     ; event 87: the VBlank interrupt, RCR = 306 next
        .repeat 6, i                    ; events 88 to 93: RCR 306 to 321, then 309 to 324 next
        EMIT which, 3 * (i) + 309, h_gen, $04
        .endrepeat
        EMIT which, 64, h_end, $04      ; event 94: RCR 324, the first display line RCR = 64 next and the cycle start handler
        .endmacro

NXT_LO: TABLE 0
NXT_HI: TABLE 1
NXT_VEC: TABLE 2
EXPT:   TABLE 3
