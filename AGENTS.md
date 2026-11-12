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

## Local gate

The single local verification command is `sh scripts/check`. CI runs the same command (record 0006), so a green local
gate predicts a green CI gate. A green gate precedes any commit. `--stage NAME`, `--selftest [NAME]` and `--timing`
narrow or time a run; they do not replace the full run before a commit. README.md documents the flags, exit codes and
tool prerequisites, and `scripts/docs-check` keeps those claims true.

## Standing process protocols (owner preferences; apply by default)

These apply to every phase, including autonomous runs. They record the owner's
standing instructions so a later phase does not re-ask or re-derive them.

### Discuss phases: fan out, adversarially, then synthesize

- For every open decision point, fan out subagents (one per decision area, in
  parallel) instead of reasoning inline, to keep the main context small. Keep
  fanning out until a further pass no longer changes the recommendation, risk, or
  evidence plan.
- Each fan-out considers breadth and depth through every relevant stakeholder and
  technical lens for the decision (for this project: security, product,
  software architecture, CPU/bus and video/audio hardware, test and verification,
  DevOps/CI, build systems, C and any other language involved, API consumers). It
  weighs pros, cons, tradeoffs and examples; patterns and anti-patterns; best
  practices, footguns and lessons learned; and prior art from other emulators and
  ecosystems. Use online research where it helps and prefer primary sources (manuals,
  specifications, official documentation) over secondary ones; think independently
  where sources are thin or conflict.
- Run an adversarial pass on the leading recommendation, then synthesize one
  coherent recommendation per decision, consistent across decisions and with the
  project goals, and follow it. Ask the owner only at authority gates (licence,
  visibility, rights, hardware captures, publication, anything one-way) or where
  the owner's preference changes the direction.
- For UI/UX decisions, weigh existing patterns, conventions, the principle of least
  surprise and elegant design instead of the hardware lenses; do not let that drift
  into unrelated scope.
- Dependencies: another copy-and-paste with recorded provenance is better than
  another dependency. A real need for a dependency is fine, but prefer a smaller,
  flatter dependency tree (more secure, fewer imported abstractions).

### Verify phases: shift left, automate everything, zero human UAT

- Goal: no human verification or UAT. Automate the world with a DevOps mindset:
  integration, end-to-end, smoke and seam tests, run as early as they give useful
  feedback ("shift left"), and on CI only when the check has recurring value.
- Hand the owner only what cannot be automated (authority gates, one-way actions,
  judgment that only the owner can make). Take verification as far as possible
  without the owner, but only where it yields useful feedback. Do not build
  automation that corners the project. Throwaway verification is fine and does not
  need to live in CI.
- Record what was automated and what was handed off in the phase verification
  report, so a later phase does not loop back to re-litigate or re-churn the same
  verification. Reduce toil.
- When verification needs an environment this machine lacks, reproduce it
  (for example a Linux container for portability) rather than deferring it to a
  human, and label such runs by what they are (a one-off local run is not a CI claim).

### CI and release: review efficiency after every run

- Whenever a CI run is happening or has finished, look for low-hanging ways to make
  it faster and cheaper (iteration time, runner minutes, redundant jobs, caching,
  path filters) without weakening the evidence each job provides. Do not leave
  pointless bottlenecks that waste time or money. Apply the same review to release
  and publication steps. Record the measured result and any change in the phase
  notes. This extends the CI bullets above and is not an approval gate.
