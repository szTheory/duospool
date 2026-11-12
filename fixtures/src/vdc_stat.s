; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_stat.s - the VDC status register and IRQ1 seen from the guest (plan 04-08; D-138, D-139, D-142, D-143, D-144, D-169, D-170, ER-10, F-40, H7-4, H7-7, H7-13). It is assembled by ca65 in the gate and never committed as a binary. The helpers are the macros of fixtures/lib/vdc.inc.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), so the VDC status and address register is at logical $0000 and the interrupt request register $1403 at logical $1403.
; It installs its own IRQ1 handler into the RAM vector slot before CLI (startup.inc, F-40), sets a vertical sum of 264 lines (VPR = $0F02, VDW = 239, VCR = 4, D-139: more than the frame length, so the cycle never repeats inside a frame), HDS = 2, HDW = 31 and RCR = 100, and never asserts an absolute line or frame number (D-136).
; The waits are counted loops or polls of the request register, the rows judge status bytes and the order of events, not positions. The interrupt handler reads $1403, then the status, then $1403 again and the status a second time, and stores the four bytes in a log in work RAM; the foreground reads the log.
; The RR and VD rows, then the DV and DS rows of plan 04-10 (the OR and CR rows of plan 04.1-02 follow in the sections H and I). A: with every enable clear and the I flag set a wait of more than two frames passes, so RCR matches and VBlanks happened with their enables clear: the status reads $00 (check 1).
; B: with only CR bit 3 set (VD) the request register polled in the foreground shows the IRQ1 level (check 2), the status shows bit 5 alone although RCR matches pass with CR bit 2 clear (check 3), the read ends the level (check 4) and clears the flag (check 5). C: the next VBlank is taken as IRQ1 and the handler reads the same status and runs once (checks 6, 7).
; D: synchronised on that VBlank, CR bit 2 (RR) is set after the earlier RCR matches passed and the status still reads $00 (check 8: enabling later never creates a flag); the next RCR match is taken as IRQ1 and the handler sees bit 2 alone, the level, the cleared second read and the dropped level (checks 9 to 13).
; E: with the I flag set a wait of 1.2 frames passes with RR enabled and VD not: the flag is raised again after the acknowledge and bit 5 stays clear although a VBlank passed (check 14). Check 15: bits 6 and 7 of every status byte read are 0.
; Every cell the checks read is written by the fixture first, and the registers are 0 at power-on under every policy (D-167), so the check bytes are equal under every power-on policy.
; The classes: MAN (H7-4: flag layout, a flag set only when its enable is set, cleared by the read), ASSUMED LED-IRQ-0009 (the IRQ1 level follows the status, delivered once), ASSUMED LED-VID-0001 (rows that need the frame to be longer than the sync distance: checks 8 and 14), ASSUMED LED-VID-0015 (BSY reads 0). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 17
DSV_CHECKS = 27

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"
        .include "vdma.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                ; ASSUMED:LED-VID-0008 (the converse of H7-4) with every enable clear an RCR match and a VBlank leave the status at 0: more than two frames pass with the I flag set and the status read gives $00
        LIT 2, $02                ; ASSUMED:LED-IRQ-0009 with VD set and not yet read, bit 1 of $1403 (the IRQ1 level) reads 1 and bit 0 reads 0 (the VDC drives no IRQ2)
        LIT 3, $20                ; ASSUMED:LED-VID-0008 (the converse of H7-4 for RR) the VBlank flag is bit 5 of the status and no other bit: CR bit 2 is clear, so the RCR matches that pass leave bit 2 clear
        LIT 4, $00                ; ASSUMED:LED-IRQ-0009 after the status read the IRQ1 level reads 0 again: bits 1 and 0 of $1403 are 0
        LIT 5, $00                ; MAN:H7-4 a read of the status clears bits 0 to 5: the second read gives $00
        LIT 6, $20                ; MAN:H7-4 the next VBlank is taken as IRQ1 and the first status read in the handler gives $20
        LIT 7, $01                ; ASSUMED:LED-IRQ-0009 the status read ends the level, so the handler ran once and was not entered again
        LIT 8, $00                ; ASSUMED:LED-VID-0001 CR bit 2 set after RCR matches passed with it clear: the status read at once gives $00 (enabling later creates no flag; the next match is far from the VBlank this fixture is synchronised on)
        LIT 9, $04                ; MAN:H7-4 with CR bit 2 set the next RCR match is taken as IRQ1 and the first status read in the handler gives $04: bit 2 (RR) alone
        LIT 10, $00               ; MAN:H7-4 the second status read in the handler gives $00
        LIT 11, $02               ; ASSUMED:LED-IRQ-0009 in the handler, before the status read, bit 1 of $1403 reads 1 and bit 0 reads 0
        LIT 12, $00               ; ASSUMED:LED-IRQ-0009 in the handler, after the status read, bits 1 and 0 of $1403 read 0
        LIT 13, $02               ; ASSUMED:LED-IRQ-0009 the handler has run twice in all: once for the VBlank and once for the RCR match, and was not entered again
        LIT 14, $04               ; ASSUMED:LED-VID-0001 after the acknowledge the next RCR match raises RR again, and a VBlank that passes with CR bit 3 clear leaves bit 5 clear: 1.2 frames later the status reads $04
        LIT 15, $00               ; ASSUMED:LED-VID-0015 bits 6 and 7 of every status byte read are 0: the OR of the foreground reads and of the four handler reads, masked with $C0
        LIT 16, $00               ; ASSUMED:LED-VID-0008 (the converse of H7-4) with DCR bit 1 clear the end of a VRAM-to-VRAM transfer sets no DV: with every CR enable clear and the flags cleared the status read after the transfer is $00
        LIT 17, $10               ; MAN:H7-4 with DCR bit 1 set the end of a transfer sets DV, bit 4 of the status, alone: $10
        LIT 18, $00               ; MAN:H7-4 the status read cleared DV: the next read is $00
        LIT 19, $20               ; ASSUMED:LED-VID-0013 a VRAM-to-SATB copy carried out at a VBlank with DCR bit 0 clear sets no DS: after the VBlank and a wait the status is $20, VD alone (bit 5)
        LIT 20, $28               ; ASSUMED:LED-VID-0013 with DCR bit 0 set the same copy sets DS, bit 3: the status is $28, VD and DS
        LIT 21, $00               ; MAN:H7-4 the status read cleared DS and VD: the next read is $00
        LIT 22, $00               ; ASSUMED:LED-VID-0008 (the converse of H7-4) with CR bit 1 clear 18 sprites on a line (more than 17, so too many for any slot count of this setup) set no OR flag: the status read after a wait of more than a frame is $00
        LIT 23, $02               ; MAN:H7-4 with CR bit 1 set the same line sets OR, bit 1 alone (the CR enable is clear): $02
        LIT 24, $00               ; MAN:H7-4 the status read cleared OR: the next read is $00
        LIT 25, $00               ; ASSUMED:LED-VID-0008 (the converse of H7-4) with CR bit 0 clear sprite 0 opaque over sprite 1 sets no CR flag: the status read after a wait of more than a frame is $00
        LIT 26, $01               ; MAN:H7-4 with CR bit 0 set the same collision sets CR, bit 0 alone (two sprites on the line, so no OR): $01
        LIT 27, $00               ; MAN:H7-4 the status read cleared CR: the next read is $00

IRQ_REQ = $1403                 ; $1403 with MPR0 = $FF: the request register
H_COUNT = $2440                 ; how often the IRQ1 handler ran
H_IDX = $2441                   ; the byte offset of the next log entry
ST_OR = $2442                   ; the OR of every status byte the foreground read
T_ST = $2443                    ; the status byte just read
LIM_N = $2460                   ; the SATB writer parameters of the sections H and I (plan 04.1-02): the entry count
LIM_I = $2461                   ; the entry being written
LIM_Y = $2462                   ; word 0 (Y), low then high byte
LIM_X = $2464                   ; word 1 (X), low then high byte, stepped by LIM_DX after each entry
LIM_PC = $2466                  ; word 2 (the pattern code)
LIM_ATTR = $2468                ; word 3 (the attributes; the palette of entry i is the low nibble of the base plus i, modulo 16)
LIM_DX = $246A                  ; the X step in dots
LOG = $2444                     ; the log, 4 bytes per handler run: $1403 before the status read, the first status read, $1403 after it, the second status read (16 bytes)

; DELAY k: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2): k from 1 to 255. Clobbers Y
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

; WAITK n: a counted wait of n delays of 1530 cycles plus the DEX and BNE of the outer loop (1536 cycles each): at the high speed (3 master clocks per cycle) 4608 master clocks per step. n = 205 is 944,640 master clocks, more than two frames of 262 lines of 1365 master clocks (ASSUMED LED-VID-0001);
; n = 95 is 437,760, 1.2 frames. The numbers are wait bounds, no row judges a position. Clobbers X and Y
        .macro WAITK n
        .local w
        ldx #(n)
w:      DELAY 255
        dex
        bne w
        .endmacro

; WAIT_COUNT n: spin until the handler has run n times, bounded at about 1.6 frames at the high speed (a machine that raises no flag fails a check, it does not hang). The loop is lda abs, cmp #, bcs, dex, bne: its longest instruction is 5 cycles (15 master clocks at the high speed). Clobbers A, X and Y
        .macro WAIT_COUNT n
        .local w, done
        ldx #0
        ldy #50
w:      lda a:H_COUNT
        cmp #(n)
        bcs done
        dex
        bne w
        dey
        bne w
done:
        .endmacro

; WAIT_LEVEL: poll the request register until the IRQ1 level reads 1, bounded at about 1.6 frames. A poll changes nothing (D-110). Clobbers A, X and Y
        .macro WAIT_LEVEL
        .local w, done
        ldx #0
        ldy #50
w:      lda a:IRQ_REQ
        and #$02
        bne done
        dex
        bne w
        dey
        bne w
done:
        .endmacro

; READ_STATUS: A = the status (a read clears bits 0 to 5); the byte is also ORed into ST_OR for check 15. Clobbers A
        .macro READ_STATUS
        lda a:VDC_AR
        sta a:T_ST
        ora a:ST_OR
        sta a:ST_OR
        lda a:T_ST
        .endmacro

; ZFILL nwords: nwords (1 to 256) zero words by VWR (AR = VWR). Clobbers X
        .macro ZFILL nwords
        .local zl
        ldx #((nwords) & 255)
zl:     st1 #0
        st2 #0
        dex
        bne zl
        .endmacro

; BAND n, ey, ex0, edx, epc, eattr: n entries (at most 255) at the VRAM write address: word 0 = ey, word 1 = ex0 + edx * i, word 2 = epc, word 3 = eattr + (i & 15). Clobbers A
        .macro BAND n, ey, ex0, edx, epc, eattr
        lda #(n)
        sta a:LIM_N
        lda #<(ey)
        sta a:LIM_Y
        lda #>(ey)
        sta a:LIM_Y+1
        lda #<(ex0)
        sta a:LIM_X
        lda #>(ex0)
        sta a:LIM_X+1
        lda #<(epc)
        sta a:LIM_PC
        lda #>(epc)
        sta a:LIM_PC+1
        lda #<(eattr)
        sta a:LIM_ATTR
        lda #>(eattr)
        sta a:LIM_ATTR+1
        lda #(edx)
        sta a:LIM_DX
        jsr lim_band
        .endmacro

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        stz a:H_COUNT
        stz a:H_IDX
        stz a:ST_OR
        ldx #15
zlog:   stz a:LOG,x                     ; the log is cleared: work RAM holds drawn bytes under a seeded power-on policy
        dex
        bpl zlog
        lda #<irq1_h
        sta a:VEC_IRQ1                  ; the IRQ1 vector jumps through this slot (startup.inc)
        lda #>irq1_h
        sta a:VEC_IRQ1+1
        csh                             ; the high speed: 3 master clocks per CPU cycle
        VDC_REG 12, $0F02               ; VPR: VSW = 2 (bits 4-0), VDS = 15 (bits 15-8)
        VDC_REG 13, 239                 ; VDW = 239
        VDC_REG 14, 4                   ; VCR = 4: the sum (2+1) + (15+2) + (239+1) + 4 = 264 lines is more than the frame length
        VDC_REG 10, $0200               ; HSR: HSW = 0, HDS = 2
        VDC_REG 11, $001F               ; HDR: HDW = 31, HDE = 0
        VDC_REG 6, 100                  ; RCR = 100
        VDC_REG 5, $0000                ; CR = 0: no interrupt enable set; the I flag stays set (reset code) until section C
; ---- A: every enable clear (the I flag stays set, so a flag that the gate let through would stay in the status for this read) ----
        WAITK 205
        READ_STATUS
        CHECK_LIT 1
; ---- B: VD enabled, RR not; the level and the status polled in the foreground ----
        VDC_REG 5, $0008                ; CR bit 3 set: VD enabled
        WAIT_LEVEL
        lda a:IRQ_REQ
        and #$03
        CHECK_LIT 2                     ; the level before the status read
        READ_STATUS
        CHECK_LIT 3
        lda a:IRQ_REQ
        and #$03
        CHECK_LIT 4                     ; the level after the status read
        READ_STATUS
        CHECK_LIT 5                     ; the second status read
; ---- C: the next VBlank is taken as IRQ1 ----
        cli                             ; the handler is installed: the next VBlank is taken as IRQ1
        WAIT_COUNT 1
        sei
        lda a:LOG+1
        CHECK_LIT 6                     ; the first status read of the handler
        lda a:H_COUNT
        CHECK_LIT 7                     ; the handler ran once
; ---- D: synchronised on that VBlank: RR enabled late, then taken as IRQ1 ----
        VDC_REG 5, $0004                ; CR bit 2 set and bit 3 clear: RR enabled, VD not
        READ_STATUS
        CHECK_LIT 8                     ; enabling later created no flag
        cli
        WAIT_COUNT 2
        sei
        lda a:LOG+5
        CHECK_LIT 9                     ; the first status read of the second handler run
        lda a:LOG+7
        CHECK_LIT 10                    ; the second status read of the second handler run
        lda a:LOG+4
        and #$03
        CHECK_LIT 11                    ; the level before the read, in the handler
        lda a:LOG+6
        and #$03
        CHECK_LIT 12                    ; the level after the read, in the handler
        lda a:H_COUNT
        CHECK_LIT 13                    ; the handler ran twice in all
; ---- E: the flag raised again after the acknowledge; VD gated by its enable ----
        WAITK 95
        READ_STATUS
        CHECK_LIT 14
; ---- bits 6 and 7 of every status byte read ----
        lda a:ST_OR
        ora a:LOG+1
        ora a:LOG+3
        ora a:LOG+5
        ora a:LOG+7
        and #$C0
        CHECK_LIT 15
; ---- F: DV gated by DCR bit 1, cleared by the read (plan 04-10): a transfer of 4 words in burst (CR bits 7 and 6 clear), judged by the status alone ----
        VDC_REG 5, $0000                ; CR = 0: no enable; the RR and VD flags that the sections above left are cleared by the read
        READ_STATUS
        VDC_REG VR_DCR, $0000
        VDMA_GO $1000, $1100, 3
        VDMA_DELAY 20
        READ_STATUS
        CHECK_LIT 16                    ; no DV with the enable clear
        VDC_REG VR_DCR, DCR_DV
        VDMA_GO $1000, $1100, 3
        VDMA_DELAY 20
        READ_STATUS
        CHECK_LIT 17                    ; DV alone
        READ_STATUS
        CHECK_LIT 18                    ; cleared by the read
; ---- G: DS gated by DCR bit 0, cleared by the read (plan 04-10): a VRAM-to-SATB copy armed by the DVSSR high byte is carried out at the next VBlank ----
        VDC_REG VR_DCR, $0000
        VDC_REG 5, $0008                ; CR bit 3: VD enabled, to find the VBlank
        WAIT_LEVEL
        READ_STATUS                     ; acknowledge: the next wait is for a new VBlank
        VDC_REG VR_DVSSR, $1000         ; the high byte write arms the copy of the next VBlank
        WAIT_LEVEL
        WAITK 2                         ; 9216 master clocks: more than the DS time of the copy
        READ_STATUS
        CHECK_LIT 19                    ; VD alone with DCR bit 0 clear
        VDC_REG VR_DCR, DCR_DS
        VDC_REG VR_DVSSR, $1000
        WAIT_LEVEL
        WAITK 2
        READ_STATUS
        CHECK_LIT 20                    ; VD and DS
        READ_STATUS
        CHECK_LIT 21                    ; cleared by the read
; ---- H: OR gated by CR bit 1 (plan 04.1-02): 18 sprites of 16 dots on display line 20 (SATB entries 0 to 17 at an X step of 14, one solid cell at VRAM $2000), the SATB copied at the next vertical blanking before SB is set. With HDE 0, HSW 0 and HDS 2 the slot count is 2 * (5 - 2) = 6, so the line overflows whatever the slot rule ----
        VDC_REG VR_DCR, $0000           ; no DS: the SATB copies below set no flag
        VDC_REG 5, $0000
        VDC_REG VR_MAWR, $2000
        VDC_SEL VR_VWR
        ldy #4
cell_pl:
        lda #$FF                        ; pixel 15: every plane is $FFFF
        ldx #16
cell_w: sta a:VDC_LO
        sta a:VDC_HI
        dex
        bne cell_w
        dey
        bne cell_pl
        VDC_REG VR_MAWR, $1400
        VDC_SEL VR_VWR
        BAND 18, 84, 32, 14, $100, 0
        ZFILL 184
        VDC_REG VR_DVSSR, $1400
        WAITK 95                        ; the copy runs at the next vertical blanking
        VDC_REG 5, $0040                ; SB set, no enable
        WAITK 95
        READ_STATUS
        CHECK_LIT 22                    ; OR gated by its enable
        VDC_REG 5, $0042                ; the OR enable (CR bit 1)
        READ_STATUS                     ; enabling later creates no flag: this read is not judged
        WAITK 95
        READ_STATUS
        CHECK_LIT 23                    ; OR alone
        READ_STATUS
        CHECK_LIT 24                    ; cleared by the read
; ---- I: CR gated by CR bit 0 (plan 04.1-02): sprite 0 (X 72) and sprite 1 (X 80) on display line 20 overlap at the dots 48 to 55, both opaque ----
        VDC_REG 5, $0040
        VDC_REG VR_MAWR, $1400
        VDC_SEL VR_VWR
        BAND 1, 84, 72, 0, $100, 1
        BAND 1, 84, 80, 0, $100, 2
        ZFILL 248
        VDC_REG VR_DVSSR, $1400
        WAITK 95
        READ_STATUS                     ; clears what the last frames set (not judged)
        WAITK 95
        READ_STATUS
        CHECK_LIT 25                    ; CR gated by its enable
        VDC_REG 5, $0041                ; the CR enable (CR bit 0)
        READ_STATUS                     ; enabling later creates no flag: this read is not judged
        WAITK 95
        READ_STATUS
        CHECK_LIT 26                    ; CR alone
        READ_STATUS
        CHECK_LIT 27                    ; cleared by the read
        rts

; the IRQ1 handler: the request register, the status (the acknowledge), the request register again, the status again; the four bytes go to the log. A and X are saved, Y is not used
irq1_h:
        pha
        phx
        ldx a:H_IDX
        lda a:IRQ_REQ
        sta a:LOG,x
        lda a:VDC_AR
        sta a:LOG+1,x
        lda a:IRQ_REQ
        sta a:LOG+2,x
        lda a:VDC_AR
        sta a:LOG+3,x
        inx
        inx
        inx
        inx
        stx a:H_IDX
        inc a:H_COUNT
        plx
        pla
        rti

; lim_band: LIM_N entries written by VWR from the parameters (the macro BAND fills them). Clobbers A
lim_band:
        stz a:LIM_I
lb_next:
        lda a:LIM_Y
        sta a:VDC_LO
        lda a:LIM_Y+1
        sta a:VDC_HI
        lda a:LIM_X
        sta a:VDC_LO
        lda a:LIM_X+1
        sta a:VDC_HI
        lda a:LIM_PC
        sta a:VDC_LO
        lda a:LIM_PC+1
        sta a:VDC_HI
        lda a:LIM_I
        and #$0F
        clc
        adc a:LIM_ATTR
        sta a:VDC_LO
        lda a:LIM_ATTR+1
        sta a:VDC_HI
        clc
        lda a:LIM_X
        adc a:LIM_DX
        sta a:LIM_X
        lda a:LIM_X+1
        adc #0
        sta a:LIM_X+1
        inc a:LIM_I
        lda a:LIM_I
        cmp a:LIM_N
        bne lb_next
        rts
