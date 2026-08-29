# Gameplay verified-coverage plan: 56.63% to 65%

## Measurement and target

Recounted 2026-08-29 with `tools/progress.py` against all retained Mesen
`exec_*.txt` captures and the overlap-merged verified ledger.

| Metric | Captured address positions |
|---|---:|
| Executed denominator | 27,901 |
| Verified baseline | 15,800 (56.63%) |
| Minimum for 65% | 18,136 (65.00%) |
| Required distinct gain | **2,336** |

This is captured-address coverage, not whole-game completion or a decoded
instruction census. Broad ranges below are ceilings until their production
behavior and permanent evidence gates are identified.

## Remaining work by component/function

| Component / function family | ROM range | Pending ceiling |
|---|---|---:|
| Bank `$80` post-metasprite object/resource helpers | `$80:A7C6-$BF00` | 1,791 |
| Bank `$80` tail scene services | `$80:F101-$FFFF` | 77 |
| Bank `$81` shared menu/text/transition services | `$81:8000-$FFFF` | 2,589 |
| Team Select scene, input, redraw and Options handoff | `$82:809A-$91FF` | 1,199 |
| Post-EA resource/title handoff | `$82:A900-$ADFF` | 81 |
| Gameplay graphics-scratch producer | `$82:F000-$F15B` | 161 |
| Other captured Bank `$82` tails | disjoint remainder | 496 |
| Bank `$83` gameplay presentation/event helpers | `$83:C000-$EFFF` | 469 |
| Matchup, ratings and starting-lineup scene | `$83:F000-$FA90` | 563 |
| Other captured Bank `$83` prefix | `$83:8000-$BFFF` | 11 |
| Bank `$84` helper remainder | `$84:BF75-$C014` | 59 |
| Bank `$85` camera/ball/play/tail remainder | disjoint gameplay ranges | 973 |
| Bank `$86` shot/pass/collision/AI remainder | disjoint gameplay ranges | 1,616 |
| Bank `$87` event/draw/animation remainder | `$87:8000-$BFFF` | 1,184 |

The selected rows below are disjoint and total 2,473 positions, leaving a
137-position margin above the required gain.

## Selected route

1. **Team Select and Options ownership (`$82:809A-$91FF`, 1,199).** Fresh
   headless Ghidra listings prove the scene constructor, controller dispatcher,
   side/category paths, redraw, seven-window palette cadence, confirmation and
   committed-option paths. Protect all 29 teams, long-name alignment, five
   rankings, East/West dash ranks, navigation wrap, both active sides and the
   exact Team Select -> Player Setup transition.
2. **Post-EA handoff (`$82:A900-$ADFF`, 81).** Retain exact sequence/instrument/
   BRR resource identity and the EA -> title publication boundary.
3. **Graphics scratch (`$82:F000-$F15B`, 161).** Keep the existing 29-call
   native three-slot differential and production jump/reach binding as a
   release requirement.
4. **Gameplay event/presentation helpers (`$83:C000-$EFFF`, 469).** Keep
   collision/event ownership separated and require the existing foul,
   violation, whistle and live gameplay regressions.
5. **Matchup/ratings/lineups (`$83:F000-$FA90`, 563).** Fresh Ghidra proves the
   court scene driver, `$83:F790` cadence, `$83:F891` team panels and
   `$83:F901` rank-to-basketball rows. Protect both selected teams, home court,
   five rank rows, ten lineup cards, font/portrait assets, card cadence and the
   final gameplay handoff.
6. Add one end-to-end production probe that executes Team Select -> Player
   Setup -> matchup -> ratings -> all ten lineup cards for several asymmetric
   team pairs, verifies ownership/team persistence, and hashes each rendered
   phase. Keep exact existing screenshot/frame oracles independent from this
   state-machine probe.
7. Recount after the ledger checkpoint. Run the entire `build.ps1 -Test` gate,
   rebuild, recreate/read back the desktop shortcut, commit and push clean
   `main` only if verified coverage is at least 18,136.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 15,800 (56.63%) | Generated ledger at goal start |
| Team Select / Options | 1,199 | 16,999 (60.93%) | Fresh headless Ghidra ownership plus production flow, navigation, persistence, long-name, rank and transition gates |
| Post-EA handoff | 81 | 17,080 (61.22%) | Sequence/instrument/BRR identity and EA/title publication gates |
| Gameplay graphics scratch | 161 | 17,241 (61.79%) | 29-call native differential and production jump/reach binding |
| Gameplay event/presentation | 469 | 17,710 (63.47%) | Collision, foul, violation, whistle and live-gameplay regressions |
| Matchup / ratings / lineups | 563 | **18,273 (65.49%)** | Fresh headless Ghidra ownership and three-pair, five-phase, 30-lineup-card production probe |

## Release verification

The final recount is **18,273 / 27,901 captured address positions
(65.49%)**, a gain of 2,473 positions from the 56.63% baseline and 137
positions above the strict 65% threshold. The full `build.ps1 -Test` release
gate passed, including the new end-to-end gameplay-65 flow probe, all existing
native differential and scene regressions, the 63,800-frame tip-flow endurance
run, CPU-versus-CPU gameplay regression, Gameplay Lab, and the legal/EA intro
timing gate.
