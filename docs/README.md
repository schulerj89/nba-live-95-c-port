# Documentation

This directory contains current project guidance, subsystem references, and
the evidence indexes consumed by repository tools. Historical completion
reports, dated work plans, one-off audits, migration notes, and task
checkpoints were removed from the working tree; Git history remains their
archive.

## Start here

- [Runtime architecture](architecture.md)
- [Build and live debugging](debugging.md)
- [Current parity gaps](parity-gap-report.md)
- [Pending gameplay work](gameplay-pending.md)
- [Measured implementation progress](progress.md)
- [Known original-game bugs and preserved quirks](known-original-game-bugs.md)

## Reproduction and verification

- [Asset capture locations](asset-capture-root.md)
- [Differential testing](differential-testing.md)
- [Headless input contract](headless-input-contract.md)
- [Visible smoke-test harness](visible-smoke-checkpoint-harness.md)
- [Native edge behavior](native-edge-parity.md)
- [Verified routine ledger](verified-routines.json)
- [Feature capture matrix](feature-capture-matrix.md)
- [Full-ROM instruction census](full-rom-instruction-census.md)

The JSON files beside the generated Markdown reports are required inputs for
the census and progress regression tools. Run artifacts, screenshots, native
captures, and temporary reports belong under ignored `build/` or `.analysis/`
directories.

## Frontend and presentation

- [EA intro](ea-intro.md)
- [Title and Setup handoff](title-to-setup-transition.md)
- [Game Setup screen](game-setup-screen.md) and [audio](game-setup-audio.md)
- [Team Select](team-select.md)
- [Player Setup](player-setup.md)
- [Player Introduction](player-introduction.md)
- [Player Lab](player-lab.md)
- [Gameplay HUD](gameplay-hud.md), [HUD assets](gameplay-hud-assets.md), and
  [gameplay audio](gameplay-audio.md)

## Gameplay and rendering

- [Tipoff](tipoff.md)
- [Match lifecycle](match-lifecycle.md) and [new-match reset](new-match-reset.md)
- [Dribble animation](dribble-animation.md)
- [Center-court layout correction](court-logo-complaint-audit.md)
- [Basket raster](hoop-raster.md)
- [Out-of-bounds overlay](out-of-bounds-hud.md)
- [CPU role-rebuild reaction delay](cpu-role-reaction-reload.md)
- [Gameplay telemetry](gameplay-debugging.md)
- [PPU pixel parity](ppu-pixel-parity.md)

Narrow evidence documents that remain outside this index are retained because
source files or verification tools reference their exact names. Extend an
existing subsystem document when possible instead of creating a new report for
each task.
