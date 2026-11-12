; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; vdc_stat.s - the VDC status register and IRQ1 seen from the guest (plan 04-08; D-138, D-139, D-142, D-143, D-144, D-169, D-170, ER-10, F-40, H7-4, H7-7, H7-13). It is assembled by ca65 in the gate and never committed as a binary. The helpers are the macros of fixtures/lib/vdc.inc.
; The guest maps the hardware page at logical $0000 (MPR0 = $FF), so the VDC status and address register is at logical $0000 and the interrupt request register $1403 at logical $1403.
; It installs its own IRQ1 handler into the RAM vector slot before CLI (startup.inc, F-40), sets a vertical sum of 264 lines (VPR = $0F02, VDW = 239, VCR = 4, D-139: more than the frame length, so the cycle never repeats inside a frame)
; and RCR = 100, and never asserts an absolute line or frame number (D-136): the waits are counted loops, the rows judge status bytes and the order of events, not positions. The interrupt handler reads $1403, then the status, then $1403 again and the status a second time, and stores the four bytes in a log in work RAM; the foreground reads the log.
; This slice (the tracer of plan 04-08): the RR row.
; With CR bit 2 clear and the I flag set a counted wait of more than two frames passes, so an RCR match has happened with its enable clear (the status reads $00, H7-4).
; The I flag is set so that a flag the gate let through would stay in the status and not be taken by the handler first. Then CR bit 2 is set, the RCR match raises RR and the handler reads $04, and a second read returns $00 (H7-4: the read clears).
; Every cell the checks read is written by the fixture first, and the registers are 0 at power-on under every policy (D-167), so the check bytes are equal under every power-on policy. The classes: MAN (H7-4: flag layout, flag set only when its enable is set, cleared by the read). No HuC, PCEAS, emulator or cc65 library code.
        .setcpu "huc6280"

DSV_ID = 17
DSV_CHECKS = 3

        .include "startup.inc"
        .include "verdict.inc"
        .include "check.inc"
        .include "vdc.inc"

        CHECK_TABLE DSV_CHECKS
        LIT 1, $00                ; MAN:H7-4 with CR bit 2 clear an RCR match leaves the status at 0: more than two frames pass with RCR = 100 and the status read gives $00
        LIT 2, $04                ; MAN:H7-4 with CR bit 2 set the RCR match sets bit 2 (RR) of the status and nothing else: the handler reads $04
        LIT 3, $00                ; MAN:H7-4 a read of the status clears bits 0 to 5: the second read in the handler gives $00

IRQ_REQ = $1403                 ; $1403 with MPR0 = $FF: the request register
H_COUNT = $2440                 ; how often the IRQ1 handler ran
H_IDX = $2441                   ; the byte offset of the next log entry
LOG = $2442                     ; the log, 4 bytes per handler run: $1403 before the status read, the first status read, $1403 after it, the second status read (16 bytes)

; DELAY k: a counted delay of 6 * k CPU cycles (LDY # is 2, each DEY is 2 and each taken BNE is 4, the last BNE 2): k from 1 to 255. Clobbers Y
        .macro DELAY k
        .local l
        ldy #(k)
l:      dey
        bne l
        .endmacro

; WAIT_FRAMES: a counted wait of 205 delays of 1530 cycles plus the DEX and BNE of the outer loop, 314,880 CPU cycles: at the high speed (3 master clocks per cycle) 944,640 master clocks, more than two frames of 262 lines of 1365 master clocks (ASSUMED LED-VID-0001; the number is a wait bound, no row judges a position). Clobbers X and Y
        .macro WAIT_FRAMES
        .local w
        ldx #205
w:      DELAY 255
        dex
        bne w
        .endmacro

; WAIT_COUNT n: spin until the handler has run n times, bounded at about 1.6 frames at the high speed (a machine that raises no flag fails a check, it does not hang). The loop is lda abs, cmp #, bcs, dex, bne: its longest instruction is 5 cycles. Clobbers A, X and Y
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

fixture_main:
        lda #$FF
        tam #$01                        ; MPR0 = $FF: logical $0000-$1FFF is the hardware page
        stz a:H_COUNT
        stz a:H_IDX
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
        VDC_REG 5, $0000                ; CR = 0: no interrupt enable set
; ---- the RR row: the enable clear (the I flag stays set, so a flag that the gate let through would stay in the status for this read), then set ----
        WAIT_FRAMES
        lda a:VDC_AR                    ; the status after the RCR matches of more than two frames passed with CR bit 2 clear
        CHECK_LIT 1
        VDC_REG 5, $0004                ; CR bit 2 set: RR enabled
        cli                             ; the handler is installed: the next RCR match is taken as IRQ1
        WAIT_COUNT 1
        sei
        VDC_REG 5, $0000                ; no more interrupt enables before the checks and the seal
        lda a:LOG+1
        CHECK_LIT 2                     ; the first status read of the handler
        lda a:LOG+3
        CHECK_LIT 3                     ; the second status read of the handler
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
