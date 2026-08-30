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

Within this old focused list, the hard lower bound is **14 isolated unverified
starts**, plus an unknown subset of the 152-start reach-launch child. It is not
a project-wide remainder. The regenerated conservative full-ROM census is now
available in `docs/full-rom-instruction-census.md` and reports 11,526 of 60,346
decoded starts as both observed and evidence-eligible; undecoded bytes remain
unknown code/data.

## Uncensused implementation areas

| Component | What remains |
|---|---|
| Match lifecycle | Regulation/OT tables, clock expiry, selected timeout resume and Exhibition final routing exist; uninterrupted regulation, halftime, all timeout/substitution paths, tied OT, postgame and persistence still need natural end-to-end proof |
| Human gameplay | Complete offense, defense, ordinary shooting, passing and movement ownership; human inbound steering and the native-backed free-throw aim slice do not yet form a playable control path, and the ordinary runtime free-throw adapter remains dormant |
| Modes | Season, Playoffs and Load Series routing, persistence and screens |
| Statistics/rules | Remaining personal-stat child, substitutions/bench promotion, rare foul/bonus/rule callers and wider made-stat branches |
| Generic action callers | Replace remaining compatibility setters only after native caller witnesses; ordinary pass and inbound paths are already adopted |
| Configured-start equivalence | Import/align complete native state and compare the first full-WRAM divergence under identical inputs |
| Gameplay presentation | Crowd CHR cadence, downstream BG1 basket/window composition, player/ball OBJ parity and more moving-camera witnesses |
| Audio fidelity | Supported gameplay families are table/pitch/source verified; complete shared-RNG ordering, crowd sequencing and cycle parity remain outside the current host boundary |
| Other presentation | Rare queue/effect/DMA paths where host-visible output is covered but SNES timing/ABI is intentionally not emulated |

## Current coverage interpretation

The live retained capture union is 28,643 address positions. All 28,643 have
source provenance, but only 11,529 (40.25%) fall inside precise,
evidence-eligible ledger boundaries. Broad whole-bank and `host equivalent`
rows remain useful documentation and now explicitly receive no address credit.
The conservative full-ROM census is 11,526/60,346 decoded starts (19.10%).
Neither percentage measures retail feature completion; use
`docs/feature-capture-matrix.md` and `docs/parity-gap-report.md` for planning.

The subsequent native edge checkpoint adds 66 exactly observed actor-edge
starts and 21 dispatch-wrapper starts, while removing unsupported older
credit. It also strengthens existing OOB, inbound and formation scopes without
granting a blanket new range. See `docs/native-edge-parity.md` for the 31-field
owned-ball projection, its one partial event case, and the remaining special-
mode, rim-context and shared-RNG gaps. A long C run is not native whole-match
equivalence.
