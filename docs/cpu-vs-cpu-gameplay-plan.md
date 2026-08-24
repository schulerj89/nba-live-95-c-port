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
| World integration | `$85:9700-$985F`, actor records `$34EB + slot*$100` | actor write-PC trace and fixed-point before/after values |
| Direction/movement | `$85:F34F`, `$87:B832-$B952` | delta inputs, direction result, velocity/output cadence |
| Assignment/CPU branch | `$85:BC43-$BD7D`, `$85:B95C-$B9D1`, `$87:A160-$A2CE` | actor target, mode, action and reaction fields across both teams |
| Play selection | `$85:B100-$B28B`, globals `$0996-$099C` | fixed RNG-state trace through at least two possessions |
| Camera | `$85:8EE6-$9191`, globals `$085C-$087A` | subject selection, bounds, step/easing, court projection |
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
