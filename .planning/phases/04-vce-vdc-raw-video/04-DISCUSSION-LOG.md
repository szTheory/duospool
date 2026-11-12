# Phase 4: VCE/VDC Raw Video - Discussion Log

> Human reference only (audits, retrospectives). Not consumed by downstream agents; 04-CONTEXT.md is canonical.

**Date:** 2026-11-11
**Mode:** autonomous discuss under the standing fan-out protocol (AGENTS.md); no owner question asked.

## Method

- Six parallel advisors, one per cluster: A VDC register interface, VRAM and DMA; B raster timeline, scroll latch and interrupts; C background and sprite pixels; D VCE and the raw video record; E fixtures, evidence and gate; F core integration.
- One adversarial reconciliation pass over all six reports (about 28 cross-cluster conflicts), with the disputed manual pages re-read as page images, then one synthesized context.

## Gray areas and outcome

| Area | Outcome |
|------|---------|
| Raw video record layout (three competing layouts) | One variable-length record per scanline with a 64-byte header; index and colour arrays; digest over a domain string plus version plus records |
| Frame length versus the H7-13 composition sum (264) | Frame length is the bound profile field; the sum is recorded as not followed |
| Oracle policy (awk reference renderer vs hand-derived literals) | No committed reference renderer (ER-7); hand-derived rows, relations, record-fault sweep, ten toy mutants |
| Power-on of VRAM, SATB, palette | Additive seeded-prng-v3; registers and beam always zero |
| Snapshot and serial | One bump (serial 7 to 8, epoch 4 to 5) with the complete region list in the tracer plan |
| Model parameters | Two rows only (vd_irq_phase, rcr_origin); everything else is a constant with a disagreement note |
| Plan structure | 14 plans in 8 waves with an explicit cut line before sprites (Phase 4.1 option) |

## Owner gates

None crossed. Hardware captures stay optional and unrequested. C2, C3, C4 (encodings 10 and 11), C5 and C11 remain labelled modelling assumptions.

## Deferred ideas

See the deferred section of 04-CONTEXT.md.
