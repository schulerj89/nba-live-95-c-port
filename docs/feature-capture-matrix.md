# Whole-game feature/capture matrix

This is the planning view of the retail game. It is deliberately separate from
the native instruction census: feature completion is a weighted engineering
estimate, while capture/Ghidra/recomp/differential columns describe evidence
strength. `strong` does not mean a feature is complete.

**Historical weighted planning estimate: 55.50%**

The 2026-08-30 ownership audit does not endorse these inherited completion estimates as current proof. Missing option consumers, human gameplay, retail modes and unreliable historical consecutive-frame captures are recorded in docs/ownership-plan.md and its workstream inventories. No whole-game completion percentage is accepted as a release gate.

| feature | weight | completion | native | Ghidra | recomp | differential | tests |
|---|---:|---:|---|---|---|---|---|
| Boot, legal, EA intro, title and credits | 7% | 95% | strong | strong | strong | partial | strong |
| Game Setup, Rules and Options | 8% | 90% | strong | strong | partial | partial | strong |
| Team select, player setup and introductions | 8% | 90% | strong | strong | partial | partial | strong |
| CPU-vs-CPU gameplay core | 25% | 55% | strong | strong | strong | partial | strong |
| Gameplay rules, fouls and free throws | 10% | 48% | partial | strong | partial | partial | partial |
| Clock, quarters, timeout, substitutions and end game | 10% | 55% | strong | strong | partial | partial | strong |
| Human offense, defense and inbound control | 10% | 5% | partial | partial | partial | partial | partial |
| Court, sprites, PPU composition and camera fidelity | 7% | 70% | strong | strong | partial | partial | strong |
| Music and in-game sound fidelity | 5% | 75% | strong | partial | partial | partial | strong |
| Season, playoffs, schedules and persistence | 5% | 25% | partial | partial | untriaged | none | none |
| Postgame, statistics, records and save flow | 5% | 0% | none | none | untriaged | none | partial |

## Remaining work and evidence

### Boot, legal, EA intro, title and credits

Reverify consecutive-frame boundaries with synchronous non-skipped Mesen output and isolated save state; historical screenshot timing and broad routine-address claims are insufficient.

Evidence: [`docs/ea-intro.md`](../docs/ea-intro.md), [`docs/title-to-setup-transition.md`](../docs/title-to-setup-transition.md), [`docs/post-ea-title-audio.md`](../docs/post-ea-title-audio.md)

### Game Setup, Rules and Options

Fix resource-state timing, native raster IRQ visibility, changed-value transitions, Style presets/Custom propagation, and all runtime option consumers. Six rules and multiple audio/presentation settings have no production reader.

Evidence: [`docs/game-setup-screen.md`](../docs/game-setup-screen.md), [`docs/game-setup-audio.md`](../docs/game-setup-audio.md), [`docs/options-test-ownership-audit.md`](../docs/options-test-ownership-audit.md)

### Team select, player setup and introductions

Expand team/roster combinations and full transition comparisons.

Evidence: [`docs/team-select.md`](../docs/team-select.md), [`docs/player-setup.md`](../docs/player-setup.md), [`docs/player-introduction.md`](../docs/player-introduction.md)

### CPU-vs-CPU gameplay core

Align the strict native/C launch baseline and scheduler before closing unobserved decisions and longer-possession behavior.

Evidence: [`docs/parity-gap-report.md`](../docs/parity-gap-report.md), [`docs/differential-testing.md`](../docs/differential-testing.md), [`docs/gameplay-pending.md`](../docs/gameplay-pending.md), [`docs/native-edge-parity.md`](../docs/native-edge-parity.md), [`docs/inbound-cancel-recovery-differential.md`](../docs/inbound-cancel-recovery-differential.md)

### Gameplay rules, fouls and free throws

Complete bonus/penalty callers, natural foul-to-stripe orchestration, substitutions and rare rule edge cases.

Evidence: [`docs/foul-classifier-differential.md`](../docs/foul-classifier-differential.md), [`docs/foul-consumer-differential.md`](../docs/foul-consumer-differential.md), [`docs/free-throw-completion-differential.md`](../docs/free-throw-completion-differential.md), [`docs/human-free-throw-differential.md`](../docs/human-free-throw-differential.md), [`docs/native-edge-parity.md`](../docs/native-edge-parity.md)

### Clock, quarters, timeout, substitutions and end game

Pixel-exact break/final scenes, all pause/substitution choices and persistence modes. Second-match reset now has a native startup projection plus separate C return journeys; lifecycle terminal-word replay is repaired but does not prove timing or complete callers.

Evidence: [`STATUS.md`](../STATUS.md), [`docs/match-lifecycle.md`](../docs/match-lifecycle.md), [`docs/match-lifecycle-native-expiry.md`](../docs/match-lifecycle-native-expiry.md), [`docs/new-match-reset.md`](../docs/new-match-reset.md), [`docs/lifecycle-verifier-audit.md`](../docs/lifecycle-verifier-audit.md)

### Human offense, defense and inbound control

Carry real Player Setup ownership into complete movement, offense, defense, passing and ordinary shooting paths; current inbound and free-throw helpers are isolated.

Evidence: [`STATUS.md`](../STATUS.md), [`docs/human-free-throw-differential.md`](../docs/human-free-throw-differential.md), [`docs/gameplay-pending.md`](../docs/gameplay-pending.md)

### Court, sprites, PPU composition and camera fidelity

Reach pixel-order parity for remaining court edges, overlays and camera cases.

Evidence: [`docs/ppu-pixel-parity.md`](../docs/ppu-pixel-parity.md), [`docs/court-assets-audit.md`](../docs/court-assets-audit.md), [`docs/gameplay-sprite-jitter.md`](../docs/gameplay-sprite-jitter.md)

### Music and in-game sound fidelity

Refine triggering, priority, mixing and long-run music continuity.

Evidence: [`docs/gameplay-audio.md`](../docs/gameplay-audio.md), [`docs/game-setup-audio.md`](../docs/game-setup-audio.md), [`docs/post-ea-title-audio.md`](../docs/post-ea-title-audio.md)

### Season, playoffs, schedules and persistence

Inventory, capture and implement non-exhibition modes.

Evidence: [`STATUS.md`](../STATUS.md)

### Postgame, statistics, records and save flow

Replace structural Exhibition final panel with exact assets; capture Season/playoff/save persistence.

Evidence: [`STATUS.md`](../STATUS.md), [`docs/match-lifecycle-native-expiry.md`](../docs/match-lifecycle-native-expiry.md)

## Updating this matrix

Edit `docs/feature-capture-matrix.json`, cite retained evidence, then run:

```powershell
python tools/feature_capture_matrix.py
```

The generator rejects duplicate IDs, missing evidence, invalid levels, completion
outside 0-100, and weights that do not total 100.
