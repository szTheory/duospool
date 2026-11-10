# Phase 1 — Linux awk portability evidence (closes the plan 01-01 gawk/mawk backstop)

Run on 2026-11-09 from the orchestrating session. A clean `git clone` of the phase branch (HEAD at the time) was mounted into a `debian:stable-slim` container (`/bin/sh` = dash). `gawk`, `mawk`, `git` and `perl` were installed with apt, then `sh scripts/check`, `sh tests/audit/synthetic.sh` and `sh tests/protocol/synthetic.sh` were run once per awk, switching `/usr/bin/awk` with `update-alternatives`.

Result: `CHECK RESULT PASS` (all 18 stages) and the synthetic suites passing under both mawk 1.3.4 20250131 and GNU Awk 5.2.1, with dash as `sh`. The Phase 1 checkers therefore give the same verdicts under macOS BWK awk 20200816, mawk and gawk. (Linux CI remains a Phase 2 job; this is a one-off local run, not a CI claim.)

Raw output (tail of each run):

```
=== awk=/usr/bin/mawk sh=/usr/bin/dash
mawk 1.3.4 20250131
CHECK RESULT PASS
AUDITTEST dup-blob-path PASS
AUDITTEST tag-only PASS
AUDITTEST shallow-clone PASS
SYNTH scorer-missing PASS
SYNTH git-failure-spikes PASS
SYNTH git-failure-protocol PASS
=== awk=/usr/bin/gawk sh=/usr/bin/dash
GNU Awk 5.2.1, API 3.2, PMA Avon 8-g1, (GNU MPFR 4.2.2, GNU MP 6.3.0)
CHECK RESULT PASS
AUDITTEST dup-blob-path PASS
AUDITTEST tag-only PASS
AUDITTEST shallow-clone PASS
SYNTH scorer-missing PASS
SYNTH git-failure-spikes PASS
SYNTH git-failure-protocol PASS
```
