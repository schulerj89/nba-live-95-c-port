# Gameplay instruction census and remaining work

Reaudited 2026-08-29 against `docs/verified-routines.json`, the current source
bindings, retained Mesen fixtures, and `tools/progress.py`. Counts are decoded
instruction starts for the named slice, not C lines, byte lengths, effort, or
a percentage of the whole game.

## Corrected bounded census

The former 602/332/252 pending tables are historical and must not be reused.
Every row from that bounded plan is now translated, runtime-bound, and covered
by a permanent native replay or differential fixture.

| Former bounded area | ROM boundary | Current status |
|---|---|---|
| Ball initialization prefix | `$86:E056-$E0AB` | 30 starts verified; full 128 KiB exit-RAM comparison |
| Jump/reach parent | `$86:EC32-$EE75` | 239 starts verified; child boundaries remain separate |
| Stationary defensive selector | `$86:E39A-$E3CA` | Verified and runtime-bound |
| Wider defensive pose caller | `$86:E3E1-$E4A6` | Verified and runtime-bound |
| Inbound motion core | `$86:F43A-$F4F1` | Verified and runtime-bound |
| Inbound arrival | `$86:F4F2-$F51F` | Verified exact signed boundary |
| Human inbound steering | `$86:F520-$F54E` | All 16 direction nibbles verified and production-bound |
| Readiness and first selector | `$86:F54F-$F5D0` | Verified and endurance-covered |
| Alternate/timeout selectors | `$86:F5D2-$F60A` | Verified second-selector and all-invalid fallback |
| Launch/orientation/return | `$86:F60B-$F668` | Verified and sustained both-team completion |

**Pending instructions in the former bounded table: 0.**

## Known decoded caveats outside that table

These rows must not be summed into a claimed whole-ROM remainder. Some are
partially verified routines and others are isolated branch-exhaustiveness
caveats.

| Area | Known count | Honest boundary |
|---|---:|---|
| Timeout confirmation prefix | 9 starts at `$86:844E-$8467` | Fixed stamina-grant child is verified; timeout UI/caller orchestration is not |
| High-jump player culling | 5 unobserved starts in `$87:A3BB-$A43B` | Statically translated, but no live native witness |
| Reach-launch child | 152 decoded starts at `$86:EAA8-$EC31` in the retained listing | Near-branch calls and production launch are tested; far branch/post-return equality is incomplete |
| Focused tip-contact alternatives | 35 of 178 gate starts absent from the focused tip fixture | Broader contact proofs exist; this is fixture exhaustiveness, not 35 missing C instructions |

The known hard lower bound is therefore **14 isolated unverified starts**, plus
an unknown subset of the 152-start reach-launch child. A larger grand total is
not defensible until a full-ROM code/data census exists.

## Uncensused implementation areas

| Component | What remains |
|---|---|
| Match lifecycle | Quarter initialization, period transitions, halftime, timeouts, substitutions, end-of-game and postgame |
| Human gameplay | Complete offense, defense, shooting, passing and movement ownership; isolated human helpers do not form a playable control path |
| Modes | Season, Playoffs and Load Series routing, persistence and screens |
| Statistics/rules | Remaining personal-stat child, substitutions/bench promotion, rare foul/bonus/rule callers and wider made-stat branches |
| Generic action callers | Replace remaining compatibility setters only after native caller witnesses; ordinary pass and inbound paths are already adopted |
| Configured-start equivalence | Import/align complete native state and compare the first full-WRAM divergence under identical inputs |
| Gameplay presentation | Crowd CHR cadence, downstream BG1 basket/window composition, player/ball OBJ parity and more moving-camera witnesses |
| Audio fidelity | Supported gameplay families are table/pitch/source verified; complete shared-RNG ordering, crowd sequencing and cycle parity remain outside the current host boundary |
| Other presentation | Rare queue/effect/DMA paths where host-visible output is covered but SNES timing/ABI is intentionally not emulated |

## Current coverage interpretation

The live retained capture union is 28,643 address positions and all 28,643 are
documented and represented by verified ledger boundaries. That is **100% of
captured addresses**, not 100% of the ROM or retail feature set. The next
measurement layer is the full-ROM census and feature/capture matrix; until
those exist, no exact whole-game instruction percentage should be published.
