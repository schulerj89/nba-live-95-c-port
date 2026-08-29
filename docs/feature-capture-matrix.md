# Whole-game feature/capture matrix

This is the planning view of the retail game. It is deliberately separate from
the native instruction census: feature completion is a weighted engineering
estimate, while capture/Ghidra/recomp/differential columns describe evidence
strength. `strong` does not mean a feature is complete.

**Current weighted whole-game estimate: 48.95%**

| feature | weight | completion | native | Ghidra | recomp | differential | tests |
|---|---:|---:|---|---|---|---|---|
| Boot, legal, EA intro, title and credits | 7% | 95% | strong | strong | strong | partial | strong |
| Game Setup, Rules and Options | 8% | 90% | strong | strong | partial | partial | strong |
| Team select, player setup and introductions | 8% | 90% | strong | strong | partial | partial | strong |
| CPU-vs-CPU gameplay core | 25% | 55% | strong | strong | strong | strong | strong |
| Gameplay rules, fouls and free throws | 10% | 45% | partial | strong | partial | strong | partial |
| Clock, quarters, timeout, substitutions and end game | 10% | 10% | weak | partial | partial | none | weak |
| Human offense, defense and inbound control | 10% | 0% | none | partial | partial | none | none |
| Court, sprites, PPU composition and camera fidelity | 7% | 70% | strong | strong | partial | partial | strong |
| Music and in-game sound fidelity | 5% | 75% | strong | partial | partial | partial | strong |
| Season, playoffs, schedules and persistence | 5% | 0% | none | none | untriaged | none | none |
| Postgame, statistics, records and save flow | 5% | 0% | none | none | untriaged | none | none |

## Remaining work and evidence

### Boot, legal, EA intro, title and credits

Finish frame/pixel equivalence for a few presentation boundaries.

Evidence: [`docs/ea-intro.md`](../docs/ea-intro.md), [`docs/title-to-setup-transition.md`](../docs/title-to-setup-transition.md), [`docs/post-ea-title-audio.md`](../docs/post-ea-title-audio.md)

### Game Setup, Rules and Options

Broaden transition and option-combination visual witnesses.

Evidence: [`docs/game-setup-screen.md`](../docs/game-setup-screen.md), [`docs/game-setup-audio.md`](../docs/game-setup-audio.md)

### Team select, player setup and introductions

Expand team/roster combinations and full transition comparisons.

Evidence: [`docs/team-select.md`](../docs/team-select.md), [`docs/player-setup.md`](../docs/player-setup.md), [`docs/player-introduction.md`](../docs/player-introduction.md)

### CPU-vs-CPU gameplay core

Close unobserved decisions, animation/ball attachment quirks and longer-possession behavior.

Evidence: [`docs/gameplay-100-percent-plan.md`](../docs/gameplay-100-percent-plan.md), [`docs/differential-testing.md`](../docs/differential-testing.md), [`docs/gameplay-pending.md`](../docs/gameplay-pending.md)

### Gameplay rules, fouls and free throws

Complete foul consumers, penalties, free throws and edge cases.

Evidence: [`docs/foul-classifier-differential.md`](../docs/foul-classifier-differential.md), [`docs/foul-consumer-differential.md`](../docs/foul-consumer-differential.md)

### Clock, quarters, timeout, substitutions and end game

Capture and implement complete regulation-game lifecycle.

Evidence: [`STATUS.md`](../STATUS.md)

### Human offense, defense and inbound control

Capture, map and implement the complete human input paths.

Evidence: [`STATUS.md`](../STATUS.md)

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

Inventory, capture and implement postgame and persistence flows.

Evidence: [`STATUS.md`](../STATUS.md)

## Updating this matrix

Edit `docs/feature-capture-matrix.json`, cite retained evidence, then run:

```powershell
python tools/feature_capture_matrix.py
```

The generator rejects duplicate IDs, missing evidence, invalid levels, completion
outside 0-100, and weights that do not total 100.
