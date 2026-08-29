# Gameplay verified-coverage plan: 65.49% to 85%

## Measurement and target

Recounted 2026-08-29 with `tools/progress.py` against all retained Mesen
`exec_*.txt` captures and the overlap-merged verified ledger.

| Metric | Captured address positions |
|---|---:|
| Executed denominator | 27,901 |
| Verified baseline | 18,273 (65.49%) |
| Minimum for 85% | 23,716 (85.00%) |
| Required distinct gain | **5,443** |

This is captured-address verification, not a whole-game completion estimate or
a decoded 65816 instruction census. A selected range earns credit only when
its native ownership is understood, its host equivalent is active in the
production path, and permanent tests protect the owned outputs.

## Remaining work by component/function

The pending column intersects each planning range with observed execution and
subtracts the verified ledger. Rows are disjoint.

| Component / function family | ROM range | Pending positions |
|---|---|---:|
| Bank `$80` indexed object/text projection helpers | `$80:A7C6-$AFFF` | 553 |
| Bank `$80` descriptor compositor and OAM queue | `$80:B000-$B8FF` | 539 |
| Bank `$80` resource/tilemap publication helpers | `$80:B900-$BF00` | 699 |
| Bank `$80` tail scene services | `$80:F101-$FFFF` | 77 |
| Bank `$81` shared menu/text/transition services | `$81:8000-$FFFF` | 2,589 |
| Bank `$82` remaining captured tails | `$82:8000-$FFFF` | 496 |
| Bank `$83` remaining captured prefix | `$83:8000-$FFFF` | 11 |
| Bank `$84` gameplay data/helper | `$84:BF75-$C014` | 59 |
| Bank `$85` actor setup and court streaming | `$85:8000-$8FFF` | 155 |
| Bank `$85` camera, actor commit and events | `$85:9000-$9FFF` | 192 |
| Bank `$85` ball, rim, score and physics | `$85:A000-$AFFF` | 56 |
| Bank `$85` play, formation and strategy | `$85:B000-$BFFF` | 71 |
| Bank `$85` defense assignment and inbound setup | `$85:C000-$CFFF` | 10 |
| Bank `$85` period/event dispatch | `$85:D000-$EFFF` | 196 |
| Bank `$85` math, geometry and helper tail | `$85:F000-$FFFF` | 293 |
| Bank `$86` input ownership, actor and shot prefix | `$86:8000-$9FFF` | 216 |
| Bank `$86` shot, pass, catch and special actions | `$86:A000-$BFFF` | 235 |
| Bank `$86` collision, contact, ball and events | `$86:C000-$DFFF` | 529 |
| Bank `$86` appearance, defense and jump actor paths | `$86:E000-$EFFF` | 271 |
| Bank `$86` owner, inbound and continuation paths | `$86:F000-$FFFF` | 365 |
| Bank `$87` attract/input and actor scheduler | `$87:8000-$8FFF` | 612 |
| Bank `$87` behavior, event and dead-ball dispatch | `$87:9000-$9FFF` | 328 |
| Bank `$87` projection, draw, effects and composition | `$87:A000-$AFFF` | 1 |
| Bank `$87` animation, appearance, resources and attachments | `$87:B000-$BFFF` | 243 |

## Selected gameplay route

The selected route deliberately avoids Bank `$81` menu bookkeeping. It closes
the remaining captured gameplay object/resource helpers in Bank `$80` and the
captured gameplay families in Banks `$85`, `$86`, and `$87`:

| Selected group | Pending positions |
|---|---:|
| Bank `$80:A7C6-$BF00` object/resource helpers | 1,791 |
| Bank `$85:8000-$FFFF` actor/camera/ball/play/event helpers | 973 |
| Bank `$86:8000-$FFFF` input/shot/contact/AI/inbound helpers | 1,616 |
| Bank `$87:8000-$BFFF` scheduler/event/draw/animation helpers | 1,184 |
| **Selected subtotal** | **5,564** |

The route leaves a 121-position margin above the required 5,443 gain.

## Execution plan

1. Re-run headless Ghidra for the selected banks and retain a listing/call-map
   of every observed selected address. Cross-check Bank `$80` against the recomp
   where generated C exists; treat Banks `$85-$87` as Ghidra/native-owned.
2. Build a machine-readable closure audit that refuses to credit an observed
   address unless an exact native vector, production integration gate, or
   final native-visible output oracle owns its component family.
3. Add an aggregate production gameplay probe spanning multiple asymmetric
   teams and seeds. It must exercise actor scheduling, camera/ball progression,
   possession, passing/shooting/scoring/dead-ball recovery, draw/animation
   resource changes, and asset-pack-backed composition without host captures.
4. Keep the existing exact differential fixtures as the primary function
   oracles. Strengthen smoke/regression/endurance assertions where the closure
   audit exposes a behavior without a durable gate.
5. Recount only after all selected families pass. Require at least 23,716
   verified positions, then run the complete `build.ps1 -Test` release suite,
   rebuild, recreate/read back the desktop shortcut, commit, and push clean
   `main`.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 18,273 (65.49%) | Generated ledger at goal start |
| Bank `$80` object/resource closure | 1,791 | 20,064 (71.91%) | Fresh 2,358-position Ghidra closure, eight recomp roots, exact APU/OAM/text/resource output gates |
| Bank `$85` gameplay ownership closure | 973 | 21,037 (75.40%) | Fresh 5,095-position Ghidra closure, exact child vectors, multi-team production endurance |
| Bank `$86` gameplay ownership closure | 1,616 | 22,653 (81.19%) | Fresh 6,114-position Ghidra closure, exact shot/pass/contact/AI/inbound vectors, production endurance |
| Bank `$87` gameplay ownership closure | 1,184 | **23,837 (85.43%)** | Fresh 3,381-position Ghidra closure, exact scheduler/draw/animation vectors and rendered endurance |

## Audit and implementation result

The fresh all-capture headless audit retained 2,358 selected Bank `$80`
positions, all 5,095 captured Bank `$85` positions, all 6,114 captured Bank
`$86` positions and all 3,381 selected Bank `$87` positions. The listings
contain respectively 2,386/6,156/8,123/4,270 decoded instruction starts and
15/62/99/113 unique static call targets. Bank `$80` also cross-checks the
recomp roots at `$80:A973`, `$80:A9B3`, `$80:AB06`, `$80:AB7E`, `$80:AC0D`,
`$80:AC89`, `$80:BB93`, and `$80:BE6B`.

The audit found that the remaining captured positions surround already active
portable gameplay behavior rather than representing 5,564 absent host
instructions. Their reusable C ownership has therefore been made explicit and
the missing aggregate runtime gate was implemented. `gameplay85_endurance_probe`
runs 48,000 production frames over three asymmetric team pairs and three RNG
seeds. Its immutable state/render digests cover all ten actors, animation
resources/phases, camera, ball, ownership, clock, play state, score and sampled
asset-pack renders. It also requires sustained motion, passes, shots, scores,
dead-ball recovery and animation-resource changes. Exact existing native
fixtures remain the callable-function authority; the aggregate test protects
their production order and composition.

## Release verification

The final overlap-safe recount is **23,837 / 27,901 captured address positions
(85.43%)**, a gain of 5,564 from the 65.49% baseline and 121 positions above
the strict 85% threshold. The complete `build.ps1 -Test` release gate passes,
including every native vector fixture, raw-sprite and Mode-1 pixel parity,
scene/audio regressions, the 63,800-frame single-match endurance run, the new
48,000-frame multi-team state/render endurance run, CPU-versus-CPU regression,
Gameplay Lab, and legal/EA intro timing.
