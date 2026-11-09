# DuoSpool bootstrap

Updated: 2026-11-09. This is a pre-GSD startup pack for the owner-selected
DuoSpool PC Engine / TurboGrafx-16 emulator project. At review time, the target
contained only .planning/config.json: no source code, Git repository, or formal
GSD project, requirements, roadmap, or state documents.

## Read before kickoff

1. Root AGENTS.md for standing project policy.
2. .planning/bootstrap/PROJECT-SEED.md for product direction and open choices.
3. .planning/bootstrap/EMULATOR-DNA.md for transferable emulator practice,
   PC Engine research leads, and their evidence limits.
4. Current .planning/config.json. All existing settings were preserved except
   workflow.auto_advance, enabled to reflect the owner's preference for
   autonomous routine GSD progress. Material choices, rights, hardware
   qualification, and external publication/release authority still require
   their proper owner gates.

These bootstrap notes do not replace GSD's formal project artifacts. Do not
hand-write .planning/PROJECT.md, REQUIREMENTS.md, ROADMAP.md, or STATE.md; let
the installed GSD workflow create and validate them.

## Start the project

In a fresh LLM session with this repository as the working directory, run:

    $gsd-new-project --auto @.planning/bootstrap/PROJECT-SEED.md

Use syntax supported by that session's installed GSD version if it differs.
The owner has asked for an automated, well-researched kickoff, so use its
automatic path and existing research/plan-check/verifier/Nyquist preferences.
The seed includes open hardware/profile, API, platform, fixture-rights, license,
and release questions: do not present proposed defaults as accepted owner
decisions.

Finish project initialization and show the generated artifacts plus the next
GSD command. Do not start phase planning or implementation in this bootstrap
handoff. Future milestone discussions should refresh only the sibling/libfuturo
evidence that could change the live decision.

## Existing configuration

The existing configuration was preserved and only workflow.auto_advance was
changed from false to true. It already enabled research, plan checks,
verification, Nyquist validation, code review, pattern-mapping, parallel plans,
and end-of-phase human verification. The project policy keeps owner decisions,
legal rights, hardware and external publication gates explicit even when
routine workflow steps advance autonomously.
