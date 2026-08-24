# CPU-versus-CPU gameplay implementation plan

## Goal and fidelity rule

The first playable gameplay milestone is a complete CPU-versus-CPU possession
loop after the jump ball. It must follow the original ROM's state transitions
for camera tracking, player assignments, action selection, animation, ball
ownership/flight, made baskets, score changes, and possession reset. Human
controller ownership is deliberately out of scope. Fouls are limited to data
structures and event hooks that can be tied to an original routine.

The recompilation, fresh headless Ghidra output, and live Mesen traces are
evidence sources. Generated recomp C is not copied blindly: every imported
behavior must be associated with its SNES entry point, relevant WRAM fields,
and a repeatable trace or regression. Captured screenshots are proof only and
never gameplay art.

## Current baseline

`nba_tipoff.c` already has ten world-space actors, ROM-derived sprite assets,
an attached/pass/shot/bounce ball model, CPU-only telemetry, and a moving
camera. Its post-tip policy is still a deterministic timed approximation:
possessions advance through fixed frame ranges, four play codes rotate in a
fixed order, shots always target a scripted rim point, scores are not stored,
and possessions reset after 430 frames. These shortcuts must be removed as the
corresponding original decisions are understood.

The baseline comparison contract is `tools/mesen_tipoff_capture.lua` plus
`tools/compare_gameplay_traces.py`. Raw fields remain present even when their
semantic name is not yet proven.

## Evidence ledger

| Area | Known original locations | Required proof before porting |
|---|---|---|
| World integration | `$85:963D-$990F` (`$9700` airborne subpath), actor records `$34EB + slot*$100` | actor write-PC trace and fixed-point before/after values |
| Direction/movement | `$85:F34F`, `$87:B832-$B952` | delta inputs, direction result, velocity/output cadence |
| Assignment/CPU branch | `$85:BC43-$BD7D`, `$85:B95C-$B9D1`, dispatch `$87:9245/$9BD0` | actor target, mode, action and reaction fields across both teams |
| Play selection | `$85:B100-$B28B`, globals `$0996-$099C` | fixed RNG-state trace through at least two possessions |
| Camera | `$85:9192-$93F4`; streamer `$85:8EE6-$90C3`; ROM map `$A0:8006` | subject proxy, bounds, step/easing, projection, source/destination mapping |
| Ball/possession | ball record `$3EEB`, owner/candidate `$0946`, `$86:CED6-$D43C`, `$87:B649/$B66A` | owner transitions and XYZ/velocity at attachment, pass and loose-ball edges |
| Shooting/scoring | to be resolved from recomp/Ghidra/Mesen | release action, rim test, made/miss branch, score writer and reset routine |
| Fouls | to be resolved adjacent to contact/collision dispatch | exact event code and handler only; no invented rules |

## Checkpoints

### 1. World and camera

- Extract the live camera routine into a reusable gameplay camera module.
- Preserve original fixed-point state separately from projected screen
  coordinates.
- Port subject selection, easing, clamping and court bounds from evidence.
- Extend JSONL/CLI telemetry with target, delta, clamp and routine fields.
- Lock multiple ROM/port camera samples and non-default home-court frames.

Exit gate: all ten actors remain in world space while the camera follows play
without exposing invalid court pixels; targeted and full regression suites
pass. Commit and push this checkpoint before CPU policy work.

Implementation evidence: `$85:9192` stores prior output in `$085E/$0862`,
limits requested motion to 22 pixels with a one-pixel dead zone, and permits
only two pixels more acceleration than the prior displacement. `$85:8EE6`
then computes coarse coordinates `(camera_x+$246)>>3` and
`(camera_y+$F2)>>3`; its source pointer is exactly
`$A0:8006 + coarse_x*104 + coarse_y*2`. The asset extractor decodes that
114x52 ROM table into 29 team-specific 912x416 court panoramas. A focused
Mesen frame-650 trace confirmed source `$A0:9F60` for coarse `(77,9)` and the
port regression guards the panorama schema, camera bounds, step cadence and
non-default home court.

### 2. CPU policy and animation

- Extract assignments, decision policy and movement integration from
  `nba_tipoff.c` into gameplay modules with raw-ROM names beside semantic names.
- Replace fixed frame-range play states with original action/condition
  transitions and deterministic RNG state.
- Port offense/defense target selection, reaction delay, direction quantizing,
  collision avoidance and scheduler cadence.
- Drive upper/lower animation resources from action and direction state rather
  than assigning generic walk/shot IDs.

Exit gate: an extended trace shows both teams continuously making decisions,
changing assignments and selecting the same action/animation families as the
ROM under the same seed. Commit and push.

Implementation evidence (increment 2A): `$87:A160` was disproven as a routine;
it is the high operand byte of `$A15E LDA actor+$5E`. The real 18-mode dispatch
is `$87:9245 -> $87:9BD3 -> $87:9BD0`. `$87:8F01-$8F8D` calls `$85:963D`
for all ten actors in one logical pass with `dt=2`; `$85:9700` is only its
airborne/Z subpath. Actor `+$64` is a modulo-47 cadence timer, not an action
ID. C now preserves those facts, the exact `$80:CEE7` LFSR and golden vector,
`$85:B95C` reaction thresholds, doubled assignment indices and paired facing,
and independent `$30/$32` animation states, clocks and ROM resource IDs.
Spatial actor/ball conditions replace the former fixed phase boundaries. The
90-tick rebound recovery remains explicitly temporary until Checkpoint 3 ports
the proven hoop/dead-ball/inbound path.

### 3. Ball, shooting and scoring

- Port possession acquisition, hand attachment, dribble, pass targeting,
  interception/loose-ball handling and shot release.
- Port ball gravity/collision and the rim/backboard made-or-miss decision.
- Add score and game-clock state to the session/gameplay model.
- Implement made-basket score changes, dead-ball transition, inbound/reset and
  alternating direction of play.

Exit gate: a deterministic CPU game produces passes, attempts, makes/misses,
score changes and post-basket possessions with no timed forced turnover.
Commit and push.

Implementation evidence (increment 3A): the final make classifier is
`$85:9D40-$A079`, using live state `$0936=1`, normal Z `74..82`, the correct
basket side, `$09F8=0`, and `$85:F1C1` weighted distance `<7`. Point value is
initialized at `$86:9DED-$9DFF` and upgraded to three by `$86:A561-$A5AF`
using the 179-word arc table at `$85:ABFB`. `$85:A1E9-$A1F7` adds `$094C`
to `$4711/$4791`; the post-make path sets `$0936=$82`, opponent group/actor
`$0952/$0954` (0/2 or 5/7), and timers `$092E/$0A04=300`. The inbound chain
is `$85:C37D-$C5C0 -> $86:F3D2-$F653 -> $86:AB2D`. C now retains the made
ball's downward physics, steers all ten actors during the inbound state, and
uses the proven 240/120/60 gates plus arrival/receiver readiness instead of
the disproven 48-frame pause. `nba_gameplay_ball` contains the ROM arc table,
classifier, point-value routine, and golden boundary fixtures.

### 4. Proven foul hooks and endurance QA

- Add foul/event enums and dispatch hooks only where original handlers and
  state fields are identified.
- Do not implement speculative foul probabilities or penalty rules.
- Run long CPU-vs-CPU simulations with invariants for bounds, ownership,
  possession progress, score monotonicity and animation/resource validity.
- Capture port video/screenshots and matching Mesen evidence at important
  transitions.

Exit gate: targeted tests, long simulation, full suite, visual review and an
independent evidence/code audit pass. Commit and push final documentation.

## Checkpoint discipline

Each checkpoint updates this document and the relevant Ghidra script comments,
has focused regression tests plus at least one visual oracle, runs the complete
build/test suite, and leaves `main` clean and synchronized with `origin/main`.
Unknown behavior is recorded as unknown rather than filled with a plausible
basketball shortcut.

## Independent audit and corrected CPU oracle (2026-08-24)

The first independent audit returned **FAIL** for the complete objective. The
older directories named `cpu-vs-cpu-live-20260823` and
`cpu-ai-extended-20260823` were actually player-vs-CPU: controller assignment
0 remained `2`. They remain useful for routine discovery but are not accepted
as CPU-only policy parity evidence.

`tools/mesen_tipoff_capture.lua` now supports `NBA95_CPU_VS_CPU=1`, clearing
all five assignments before actor initialization and exporting `$093E` as the
possession actor rather than a human-control marker. The fresh 1,201-frame
oracle in `.analysis/cpu-vs-cpu-oracle-20260824` has five `FFFF` assignments,
control actor `-1`, and zero human actor frames throughout. It also disproves
the former claim that behavior mode 11 is human-only: mode 11 appears in 704
CPU actor-frames.

The first core differential against checkpoint `eddc88c` fails with 106 field
mismatches in 202 comparable frames. Remaining blockers are therefore explicit:

- port the 18 behavior handlers rather than reporting their target addresses;
- replace render-parity scheduling and handcrafted target/speed/phase policy;
- replace fixed hand offsets and ballistic pass/shot constants;
- integrate the proven shot-quality/miss offset pipeline and collision-owned
  offensive/defensive rebounds;
- finish the event-driven inbound pass/catch executor; and
- trace the camera subject proxy writers for loose, shot and dead-ball states.

The exact score field path and isolated hoop/arc helpers remain valid partial
ports. No foul enum or callback is added because no foul event writer, handler,
or state mutation has been proven.
