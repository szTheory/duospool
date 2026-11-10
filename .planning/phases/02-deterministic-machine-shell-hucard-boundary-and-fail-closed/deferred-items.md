# Phase 2 deferred items

Out-of-scope discoveries logged by executors (scope boundary: only issues caused by the current task are fixed in that task).

## From plan 02-19 (found by a one-off Linux container run of the whole gate, not by CI)

### D-19-1. record-lint fails under mawk: lvalue-first evaluation in `scripts/lib/record-rules.awk` line 45

- **Status: FIXED in plan 02-20 (commit da17509, Rule 1).** The proposed `if/else` form is applied. An audit of every awk program under `scripts/` and `scripts/lib/` found no other statement that tests `(x in a)` and assigns the same `a[x]`. A one-off local run of the whole gate in an Ubuntu 24.04 container passed under mawk 1.3.4 and under gawk 5.2.1 (not a CI claim; see the 02-20 SUMMARY). The WINDOWS.md fixme is marked fixed.

- **What:** `sh scripts/check` on Ubuntu 24.04 (container, default awk mawk 1.3.4) gives `CHECK record-lint FAIL` with `REC_FIELDS` and `REC_TABLES` lines such as `record keys are ' format_version', want 'format_version'` (a leading space on every first key). macOS (BWK awk 20200816) and gawk 5.2.1 do not show it.
- **Cause:** `kseq[t] = (t in kseq) ? kseq[t] " " k : k` (line 45). mawk evaluates the assignment target `kseq[t]` first, which creates the element, so `(t in kseq)` is already true for the first key of every table. Which operand an awk evaluates first is not defined by POSIX, so this is an awk dialect trap, exactly the class that the CI job's second awk run exists to catch (D-61, D-74).
- **Owner of the fix:** the plan that wrote `record-lint` (earlier in this phase), not plan 19. Plan 19 only wraps the gate.
- **Proposed fix (verified in the container, not applied to the repository):** replace the line with `if (t in kseq) kseq[t] = kseq[t] " " k; else kseq[t] = k`. With only that change the whole gate passed on Ubuntu 24.04 (gcc 13.3.0, clang 18.1.3, cmake 3.28.3) under mawk 1.3.4 and again under gawk 5.2.1. A grep for the shape `x[k] = (k in x) ? ...` over `scripts/` and `tests/` finds this one line only. A control that runs the record checker under mawk would keep it from coming back (the CI second-awk run is the existing guard).
- **Consequence if left:** the first real CI run fails at the gate step under the runner's default awk (Ubuntu's default is mawk unless gawk is installed). Success criterion 5 stays unverified-pending-push either way (D-64).
