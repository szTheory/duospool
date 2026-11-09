# DuoSpool project instructions

## Start here

Read .planning/bootstrap/START-HERE.md, then review the self-contained
.planning/bootstrap/PROJECT-SEED.md and .planning/bootstrap/EMULATOR-DNA.md
before initializing OpenGSD. These bootstrap files are kickoff context, not
generated GSD project artifacts. Let the installed $gsd-new-project workflow
create and validate .planning/PROJECT.md, REQUIREMENTS.md, ROADMAP.md, and
STATE.md. Preserve existing planning configuration and user work.

DuoSpool is the owner-selected working name for a PC Engine / TurboGrafx-16
emulator. The name is not trademark clearance. Keep the base PC Engine/TG-16
goal distinct from unresolved decisions about hardware revisions, regions,
SuperGrafx, CD-ROM²/Super CD-ROM², system cards, controllers, and delivery order.

## Engineering and workflow preferences

- Treat the project seed as a North Star and research input, not a frozen
  specification. Distinguish owner decisions, source-backed facts, code
  observations, historical reports, proposals, and unknowns.
- At every milestone discussion, fan out around the actual open decision.
  Bring in relevant technical, product, graphics/audio, CPU/bus, testing,
  security, performance, and DevOps lenses; compare options, tradeoffs,
  anti-patterns, and failure cases; use primary sources where they can change
  the decision; delegate independent questions to subagents when the session
  supports it and the parallel work saves time or context; challenge the
  recommendation once; then synthesize and stop when further passes no longer
  change the recommendation, risk, or evidence plan.
- Keep core machine behavior independent from filesystems, host clocks, physical
  device discovery, UI, windowing, and device output. Do not freeze a public
  API/ABI or assume a libfuturo dependency before a real consumer proof.
- Favor a small, flat dependency tree. A small understandable copied helper can
  be cheaper than another dependency; record provenance and maintenance cost
  when copying code.
- Automate repeatable verification as soon as it gives trustworthy feedback.
  Use CI only for distinct recurring failure classes at reasonable wait and
  maintenance cost. Keep expensive fuzzing, broad compatibility matrices, and
  noisy performance tests out of the fast PR path unless evidence justifies it.
- After each CI run, inspect measured low-risk ways to reduce critical-path
  time and cost without weakening distinct evidence. At milestone close, keep
  main green, triage open PRs, refresh the rolling roadmap and lessons, and
  release the exact tested artifact whenever provenance, rights, qualification,
  and publication authority permit.
- Let routine, reversible GSD work advance autonomously. Ask the owner only
  when the answer changes project direction, authority, material risk/cost, or
  a true external/irreversible action; do not turn each command or check into
  an approval gate.
- Use deterministic input scripts and fresh machine state for repeatable
  checks. Observe guest progress, machine time, raw video, raw audio, and host
  presentation separately. A matching hash proves repeatability only unless
  tied to an independent oracle.
- Treat game images, firmware, system cards, disc images, save files, and
  archive parsers as security and rights boundaries. Do not commit commercial
  ROMs, BIOS/system-card bytes, private saves, or credentials. A hash does not
  grant redistribution rights.
- Keep acceptance evidence honest: a test card, instruction vector, boot, title
  screen, or emulator-to-emulator agreement does not establish broad gameplay
  compatibility or hardware accuracy.

Use current PC Engine/TG-16 sources at each material decision. Sibling project
examples are engineering evidence, not PC Engine hardware facts or an
instruction to inherit another project's release, security, CI, or execution
stop. Do not rely on another private workspace for project context.
