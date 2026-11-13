; SPDX-License-Identifier: 0BSD
; SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
;
; psg.inc - the PSG helpers of the authored fixtures (plan 05-04; PR-5, D-188, D-189, D-212). Equates and macros only: including this file emits no byte, so no image that does not expand a macro changes. It is created by the plan that lands the PSG judge and is never edited by a later
; plan, so every pinned image keeps its bytes; later shared code goes into its own include (psgmod.inc, psglfo.inc) or the fixture. Included after startup.inc, verdict.inc and check.inc. No HuC, PCEAS, emulator or cc65 library code.
; The PSG registers are at logical $0800 to $0809 while MPR0 = $FF maps the hardware page at logical $0000 (the fixture sets that); the register of an access is the offset & $0F (D-188), so $0810 + n is a mirror of register n, and $080A to $080F name no register.
; The registers are write only (H8-24): a read of the window returns the bus latch, never PSG state, so a fixture judges its writes through the runner's DSPE and DSPA streams, never by reading back.
        .setcpu "huc6280"

PSG_R0 = $0800                  ; channel select, bits 2-0 (MAN P8-2)
PSG_R1 = $0801                  ; main amplitude level: LMAL bits 7-4, RMAL bits 3-0 (MAN P8-3)
PSG_R2 = $0802                  ; frequency F, the low 8 bits, of the selected channel (MAN P8-3)
PSG_R3 = $0803                  ; frequency F, the high 4 bits (MAN P8-3)
PSG_R4 = $0804                  ; chON bit 7, DDA bit 6, AL bits 4-0 (MAN P8-4)
PSG_R5 = $0805                  ; balance: LAL bits 7-4, RAL bits 3-0 (MAN P8-4)
PSG_R6 = $0806                  ; waveform data, 5 bits (MAN P8-5)
PSG_R7 = $0807                  ; noise enable bit 7, noise frequency bits 4-0, select 4 and 5 only (MAN P8-6)
PSG_R8 = $0808                  ; LFO frequency (MAN P8-6)
PSG_R9 = $0809                  ; LFO trigger bit 7, LFO control bits 1-0 (MAN P8-6)
PSG_MIRROR = $0810              ; the window mirror of R0 ($0810 & $0F = 0); $0810 + n mirrors register n

; PSG_SELECT s: R0 = s (the channel the R2 to R7 writes go to)
        .macro PSG_SELECT s
        lda #(s)
        sta a:PSG_R0
        .endmacro

; PSG_WRITE reg, v: one write of the byte v to the register at address reg (one of the equates above)
        .macro PSG_WRITE reg, v
        lda #(v)
        sta a:reg
        .endmacro
