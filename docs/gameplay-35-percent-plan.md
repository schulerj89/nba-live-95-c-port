# Gameplay 35% verification goal

Started 2026-08-29. This plan uses the existing captured-address metric so the
target is comparable with `STATUS.md` and `tools/progress.py`. It is not a
percentage of the whole ROM and it is not estimated completion.

## Starting point and target

| Metric | Captured addresses | Percent |
|---|---:|---:|
| Executed denominator | 27,901 | 100.00% |
| Verified starting point | 7,826 | 28.05% |
| Minimum 35% target | 9,766 | 35.00% |
| Required verified gain | 1,940 | 6.95 points |

Coverage credit requires all four of these: a decoded instruction census,
native/recomp ground truth, a portable implementation or explicit host-side
equivalent, and a production runtime-binding test. Source comments alone do
not move the verified ledger.

## Pending gameplay components

The instruction counts below come from the maintained Ghidra listings. The
coverage-gain column is the distinct captured-address gain available after
subtracting `docs/verified-routines.json`; it is the number that affects the
35% metric. Unobserved instruction starts remain pending even if a portable
outcome exists.

| Component/function | ROM boundary | Pending instruction starts | Available captured-address gain |
|---|---|---:|---:|
| Stationary defensive idle selector | `$86:E39A-$E3CA` | 20 | 20 |
| Wider defensive pose caller | `$86:E3E1-$E4A6` | 79 | 79 |
| CPU inbound owner continuation | `$86:F43A-$F4F1` | 89 | 89 |
| CPU inbound arrival/readiness/timing/candidates/launch/return | `$86:F4F2-$F51F`, `$86:F54F-$F668` | 135 | 119 |
| Appearance-record build | `$86:D85E-$DA17` | 12 unobserved (186 verified) | 0 remaining captured gain |
| Appearance upload-list build | `$86:E0B0-$E389` | 152 | 46 |
| Remaining body/head/jersey assignment | `$87:AF75-$B450` excluding the now-verified number compositor | 4 observed | 4 |
| Live frame/layer selection | `$87:A47A-$A98D` | 561 after existing small proofs | 472 |
| Projection/culling | `$87:A357-$A479` | 120 | 96 |
| Sprite-part composition | `$80:AD92-$AEC1` | 116 after existing helper proof | 116 |
| Player-number/jersey compositor | `$87:B05B-$B354` | 0 (native output complete) | 0 remaining captured gain |
| Foul/event/dead-ball/free-throw dispatch | `$85:93F5-$945E`, `$87:92A5-$95E6`, `$87:9B30-$A017`, `$87:BACB-$BAF4` | must be recounted per callable boundary | 272 |

The optional human inbound steering `$86:F520-$F54E` has 19 starts and remains
outside the CPU-vs-CPU goal. SNES DMA/upload plumbing is also excluded from
portable gameplay credit; the asset pack and PPU parity harness verify its
host-side outcome without pretending the C port executes SNES DMA machinery.

## Implementation sequence

1. **Defense and CPU inbound (321-address ceiling).** Translate the two
   defensive routines and the complete CPU-only inbound chain. Capture natural
   and controlled branches, compare all owned words, wire the helpers into live
   actor flow, and add an inbound endurance smoke test.
2. **Appearance preload and assignment (627-address ceiling).** Reuse the
   roster/asset-pack identities but compare native exit records for all ten
   actors. Fail closed on missing records/resources. Keep upload queue bytes as
   evidence outputs rather than emulating DMA.
3. **Live draw selection and composition (684-address ceiling).** Compare
   selected lower/upper/head/number resources, coordinates, flips, palette and
   layer visibility. Add consecutive-frame and all-team runtime probes plus
   player/ball OBJ pixel witnesses.
4. **Threshold closure.** Recount. If the preceding distinct verified gain is
   below 1,940, complete the player-number compositor first; use event/dead-ball
   dispatch only if additional observed coverage is still required.
5. **Release gate.** Run the native differential captures, all vector probes,
   `build.ps1 -Test`, the 63,800-frame CPU endurance test, PPU pixel parity,
   and `tools/progress.py`. Commit and push coherent checkpoints; do not claim
   completion until verified coverage is at least 9,766/27,901.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 7,826 (28.05%) | Recounted ledger before implementation |
| Defensive idle/pose | 99 | 7,925 (28.40%) | 12,265 native calls, all eight observed exits, zero mismatches; 250 retained witnesses; production adapter probe; reviewed five changed CPU visual anchors |
| Active appearance/matchup records | 186 | 8,111 (29.07%) | Exact ten-actor native output; all 348 ROM roster records byte-compared with pack; 29-team runtime sweep; corrected roster-slot/matchup-selector bug |
| Jersey-number compositor/BCD selector | 391 | 8,502 (30.47%) | Exact 1,920-byte native ten-player output; all numbers 0-99, both sides and six visible views; corrected visiting-side mask bug |
| Player projection/culling core | 50 | 8,552 (30.65%) | Native signed-coordinate witnesses, exact CPU/human visibility boundaries, retained OAM cadence and 16,000-update runtime binding |
| Sprite-part compositor | 116 | 8,668 (31.07%) | All 32 native opening resource/origin/order calls plus 5,568 team/roster/direction/side cases; fixed direction-dependent torso/number priority |
| CPU inbound motion core | 89 | 8,757 (31.39%) | 500 natural native calls; all 111 non-arrival motion calls retained and replayed with zero mismatches; fixed duplicate integration, integer-word gates, close-range damping and negative /16 bias |
| Player draw preparation / target-facing selector | 128 | 8,885 (31.84%) | 2,000 natural native compositor-input calls, all ten actors, 175 resource pairs, five modes and 30 movement/draw direction differences; zero mismatches; renderer now preserves presentation-facing separately from physics-facing |

### Route from checkpoint 2 to 35%

After the projection and compositor checkpoints, the remaining target is
1,098 captured address positions (8,668 -> 9,766). The bounded components
below still provide 1,109 uncredited positions, so the route retains an
11-position margin rather than relying on percentage rounding.

| Component / callable boundary | Newly available verified positions | Required evidence before ledger credit |
|---|---:|---|
| Remaining player projection presentation setup `$87:A357-$A479` | 46 | Native queue/effect/ball presentation branches beyond the verified 50-position player projection core |
| Player draw preparation `$87:A47A-$A98D` | 472 | Native per-layer resource/origin/attribute calls across directions and live animations, plus production telemetry agreement |
| Sprite-part compositor `$80:AD92-$AEC1` | 0 | Completed at checkpoint 4: 116 positions already credited |
| Appearance upload-list equivalent `$86:E0B0-$E389` | 46 | Native active-player outputs and exhaustive asset-pack resource closure; SNES DMA timing remains explicitly out of scope |
| CPU inbound continuation `$86:F43A-$F668` | 213 | Native arrival/timer/selector vectors and sustained live inbound completion with host fallback removed |
| Foul/dead-ball/free-throw dispatch | 272 | Native `$85:93F5`, `$87:92A5`, `$87:9B30`, `$87:BACB` state-output vectors plus live event/whistle/CPU free-throw paths |
| Remaining appearance/resource resolution | 48 | Close the four observed `$87:AF75-$B450` positions and 44 observed `$87:B649-$B952` positions with exact resource/attachment outputs |
| Whistle timer cadence `$85:EDB3` | 12 | Signed timer/gate vectors and runtime cadence assertion |
| **Remaining planned subtotal** | **1,109** | **Target requires 1,098** |

The CPU-inbound implementation checkpoint also removed the host-only
"first eligible teammate" scan. Ghidra/recompilation at `$85:AE35-$AE95`
showed the missing behavior was instead a descending role-4-to-role-0
formation scan that assigns `(-40,160)` to the first CPU teammate other than
provisional `$0954` during state `$82`, plays 6..9 and the left-baseline target
quadrant. The production runtime now performs that exact side effect and then
lets `$86:F5C7-$F653` retain sole authority over receiver selection. The
63,800-frame CPU regression and branch-predicate self-test pass. No inbound
ledger credit is taken until the dedicated native state vectors are retained.

The defensive checkpoint also repaired two stale smoke assumptions exposed by
the stronger release gate: a same-pass shot launch is identified by its actual
launch serial/actor rather than a prior-frame velocity proxy, and a naturally
installed inbound may validly expire after repeated collision displacement.
The deterministic inbound completion witness continues to require the
successful arrival/transfer path.

## Regression policy

- Every helper receives tamper/poison cases so a self-consistent C fixture
  cannot pass as native evidence.
- Every translated helper needs a runtime probe proving the production game
  calls it with live state.
- Inbound work must cover both teams, timeout/fallback, candidate ordering,
  launch, and no-receiver endurance.
- Appearance work must cover all 29 teams, all 12 roster entries, tall/short
  body families, skin/head families, jersey number digits, flips, and missing
  asset rejection.
- Visual hashes may change only after reviewed frames and must remain separate
  from native pixel-parity claims.
