# Gameplay 40% verification goal

Started 2026-08-29. This goal continues the generated captured-address metric
used by `tools/progress.py`. It is not a percentage of the whole ROM or an
estimate of total game completion.

## Baseline and target

| Metric | Captured address positions | Percent |
|---|---:|---:|
| Executed denominator | 27,901 | 100.00% |
| Verified baseline | 9,767 | 35.01% |
| Minimum 40% target | 11,161 | 40.00% |
| Required verified gain | 1,394 | 4.99 points |

Credit still requires a decoded instruction census, native ROM/recomp ground
truth, a portable implementation or explicit host equivalent, a production
runtime binding, and a permanent regression gate. Existing C code and source
comments do not count by themselves.

## Recount by component and function

Instruction starts are unique decoded starts found in the maintained headless
Ghidra listings. Available gain is the distinct captured-address contribution
remaining after subtracting `docs/verified-routines.json`; it is the number
that changes the coverage metric.

| Component / function | Native boundary | Decoded starts | Available verified gain |
|---|---|---:|---:|
| Play request consumption | `$85:B128-$B24B` | 88 | 90 |
| Dead-ball and formation routing | `$85:AD6B-$AF5B` | 195 | 161 |
| Mode-11 shot/formation/pass dispatcher | `$85:B50E-$B8CA` | 368 | 67 |
| Defensive matchup assignment | `$85:B9D2-$BC06` | 133 | 210 |
| **Play/formation subtotal** |  | **784** | **528** |
| Defensive target and pose continuations | `$86:E635-$EA03` | 266 | 203 |
| Mode-13 carried-ball close finish | `$86:A7DA-$AA69` | 222 | 37 |
| Mode-14 special receiver and close-finish initializer | `$86:B0F7-$B624` | 488 | 326 |
| **Close-finish subtotal** |  | **710** | **363** |
| CPU pass setup and grounded release | `$86:A6B3-$A7D9`, `$86:AB2D-$B04A` | 606 | 116 |
| Player/ball contact orchestration | `$86:CCCD-$D728` | 1,125 | 129 |
| Violation/event dispatch | `$87:92A5-$95E6` | 295 | 142 |
| Shared dead-ball initializer/clear continuation | `$87:9B30-$9D20` | 112 | 32 |
| **Violation/dead-ball subtotal** |  | **407** | **174** |
| **Selected distinct route** |  | **3,898** | **1,513** |

The route exceeds the required gain by 119 positions. Counts deliberately do
not add already verified shot launch, inbound, compositor, foul-consumer, or
dead-ball reset ranges a second time.

## Implementation order and proof gates

1. **Close-finish modes (363 ceiling).** Recount internal callable boundaries,
   capture natural and controlled state transitions, replay animation/ball/
   ownership outputs, and prove live modes 13/14 use the helpers.
2. **Play, formation, and matchup assignment (528 ceiling).** Preserve native
   scan/tie ordering, play-request consumption, RNG order, team context, and
   actor targets. Add branch-census and multi-possession runtime tests.
3. **Defensive target/pose continuation (203 ceiling).** Differentially replay
   all table families, sign/half-court branches and pose outputs; require live
   CPU actors to exercise each supported family without invalid resources.
4. **CPU pass setup/release (116 ceiling).** Compare receiver selection inputs,
   resource/attachment state, ball launch and ownership handoff. Retain
   ordinary and inbound passes as separate witnesses.
5. **Player/ball contact orchestration (129 ceiling).** Verify broadphase,
   ordered candidate handling, attachment hitboxes and acquisition handoff.
   Strengthen collision and long-run possession invariants.
6. **Violation/dead-ball dispatch (174 ceiling).** Implement only decoded,
   witnessed branches; compare event, whistle, inbound, clock and free-throw
   outputs. Recount after each callable boundary and stop only at >=40%.
7. **Release gate.** Run every new vector probe, focused runtime smoke tests,
   `build.ps1 -Test`, the 63,800-frame CPU endurance suite, PPU/asset checks,
   and `tools/progress.py`. Rebuild and refresh the desktop shortcut, then push
   the final clean `main` checkpoint.

## Checkpoints

| Checkpoint | Newly verified | Running verified | Evidence |
|---|---:|---:|---|
| Baseline | - | 9,767 (35.01%) | Generated ledger at goal start |
| Close-finish modes and post-shot continuation | 402 | 10,169 (36.45%) | 91 natural real-entry calls, six entry/exit pairs, zero mismatches; basket-target hold and terminal mode branches protected by runtime probe |
| Defensive primary/help matchup helpers | 210 | 10,379 (37.20%) | 342 natural real-entry calls, nine entry/exit pairs, zero mismatches; permanent witnesses retain every rare one-way-help call and runtime probe covers one-way/symmetric/tie behavior |
| Pass/contact regression hardening | 24 | 10,403 (37.29%) | 115 pass calls and split player/ball contact corpus moved into permanent build gates; overlap recount prevents duplicate credit |
| Defensive target families | 158 | 10,561 (37.85%) | 600 natural calls across context-3, weak-side and positional families, exact ROM player-rating lookup, zero mismatches |
| Made-score play-request rewind | 48 | 10,609 (38.02%) | 100 controlled native caller-boundary calls; fixed active-context source and cutter preservation; live-play selector remains unclaimed |
| Live-play request selection | 73 | 10,682 (38.29%) | 36 controlled B120-B352 calls; exact score, strategy, range, override and RNG ordering |
| Formation target and steering | 147 | 10,829 (38.81%) | 400 natural plus five controlled families; 64 permanent full-state witnesses and exact nested child census |
| Mode-11 parent dispatcher | 67 | 10,896 (39.05%) | 246 natural calls and four controlled branch families; 61 permanent witnesses; fixed context-$3B route and direction-8 arrival deceleration |
| Violation/dead-ball parent dispatch | 98 | 10,994 (39.40%) | Ten controlled real-entry families and 61 permanent calls; exact event/dead-ball outputs, boundary scheduling, and parent/child split; CPU endurance covers live layout-1 inbound recovery |
| Mode-six defensive target family | 19 | 11,013 (39.47%) | Corrected the false EA03 capture exit to the shared E82E continuation; 200 natural calls and 20 permanent target witnesses, zero mismatches |
| Normal mode-one/mode-three actor parents | 112 | 11,125 (39.87%) | 579 uninterrupted natural parent calls; 64 permanent hold/due witnesses; fixed state-81 cadence, premature derived state, and loose-pursuit target corruption |
| Requested-direction finalizer | 58 | **11,183 (40.08%)** | 300 uninterrupted natural E5AB calls and 32 permanent witnesses cover CPU target-box, human velocity, ball-facing and preserve-current branches |
