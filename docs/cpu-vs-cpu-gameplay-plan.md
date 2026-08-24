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
| Assignment/CPU branch | `$85:BC43-$BD7D`, `$85:B95C-$B9D1`, dispatch `$87:9244/$9BD0` | actor target, mode, action and reaction fields across both teams |
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
is `$87:9244 -> $87:9BD3 -> $87:9BD0`. `$87:8F01-$8F8D` calls `$85:963D`
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

Increment 4A ports the first safe miss/rebound boundary: asset-pack roster
bytes `+$36/+$37`, `$86:9F32/$9F38` rating tiers/difficulty adjustments, the
exact `$80:CEE7` outcome comparison at `$86:A110`, `$09F8`, and all 16 miss
offsets at `$86:A17D`. Missed balls retain free physics and all players pursue
the landing point; ownership changes only on the C equivalent of
`$86:BAA2/$86:BAEE`, so offensive rebounds remain possible. Raw contest and
timing inputs to `$86:9ED8-$A11D` are still explicit defaults, and rim response
is still a bounded/damped host approximation pending the full collision port.

The CPU-only Mesen oracle now exports the live `$07F6` LFSR state instead of
the former `FFFF` placeholder and directly probes `$87:9244/$87:9BD0` behavior
dispatch plus `$85:963D/$85:980B/$85:985F` actor physics/coordinate commits.
A fresh 1,801-frame capture validates 23 possession-owner changes, 994 CPU
mode-11 actor frames, and 1,404 distinct RNG states. This makes subsequent
same-seed policy and scheduler comparisons reproducible; the ignored raw
capture remains evidence rather than a runtime asset.

Increment 4B corrects the behavior entry to `$87:9244` and expands the
headless dump to all 18 `$87:9BD3` destinations. C now preserves the proven
CPU possession lifecycle modes (receiver 10, handler 11, shot 12, passer 15,
post-shot 16, recovery 7) without claiming that the remaining policy handlers
have been ported. Actor physics uses a global 30-Hz phase that survives
possession/inbound changes and integrates host 24.8 positions with the ROM's
`dt=2` bridge from `$87:9B0D/$85:963D`.

Normal shots now use the first two words of every `$86:A4AB` record and the
base launch formula at `$86:A1BD-$A292`. Free flight follows `$85:A3B7-$A4DA`:
gravity `$18` per due physics pass, 7/8 vertical restitution capped at `$0400`,
and impact-only 15/16 lateral damping. The unresolved `$0978` timing variants
and multi-branch rim shell remain disabled rather than receiving guessed raw
inputs. A 50,000-frame regression locks repeated makes, deterministic misses,
collision-owned rebounds, both-team scoring, and both early/timeout inbound
completion paths.

Increment 4C records actor +$56/+58 targets, +$62 saved behavior mode,
requested facing +$50, and the `$85:963D` actor order. The fresh CPU-only ROM
capture in `.analysis/cpu-scheduler-oracle-20260824` contains 863 completed
logical passes: 465 fit in one rendered frame and 398 cross an NMI boundary.
Coalescing adjacent slices proves every pass is exactly actors 0..9 with no
skip, duplicate, or reorder. This is hardware preemption, not cooperative AI
time slicing: `$80:8156-$859B` saves/restores the interrupted CPU context and
RTI resumes the actor loop. C therefore keeps the ten-actor pass atomic at
30 Hz, with fixed delta 2 from `$87:9B0D-$9B2F`; the comparator's
`--logical-passes` option performs the required ROM coalescing.

The exact `$86:E923-$E96E` paired-player target primitive is now isolated in
`nba_gameplay_target_from_pair`: target position equals paired position plus
signed velocity arithmetic-shifted right by three plus the caller's table
offset. Mode 16 no longer uses a fabricated 12-render-frame switch. Its
`+$60` counts 24..0 by two at `$86:B0F7-$B108`, then `$86:B10A-$B122` installs
mode 7 and 180; mode 7 uses the same logical-pass countdown before returning
to the planner boundary. Mode 9 has the proven saved-mode restore at +$62,
while its unobserved target-selection branches remain explicit future work.

Increment 4D traces the active-mode decision cadence and movement data rather
than inferring them from rendered motion. At the `$87:9244` dispatcher, live
CPU-only play has DP `$C6=2` for physics and DP `$C8=$20` for modes 1-6. Those
modes subtract `$20` from actor `+$60`; modes 1/3/5 reload from
`$40 + roster[$3F]`, mode 2 from `$30 + roster[$40]`, and modes 4/6 from
`$20 + roster[$40]`. The latter group conditionally adds another `$20` for
the same court half. Asset roster schema `NBPROST2` preserves these two raw
ROM bytes per player so decision frequency remains roster-driven.

The movement target source is the five-root pointer graph at `$85:C6A5` used
by `$85:AD6B-$ADE3`: 61 play codes select one of 1,595 signed coordinate pairs
by team-relative slot and play-step index. `$099C` mirrors Y; possession side
mirrors X and, for play codes `$0E+`, Y as well. The extracted records have
SHA-256 `e6c71d3e45e12c1f5bf691a23f7d952e6f989798d78024d77518ac7f7437941c`.
These are initial formation targets; the later `$85:AE35+` overrides and
mode-specific helpers still apply before velocity resolution.

Increment 4E ports the exact velocity boundary `$85:A82C-$AB16`, fed by the
`$85:F34F` direction/distance quantizer. It uses eight ROM direction vectors,
a 16-entry profile scale selected from roster `+$42`, two integer damping
iterations in observed play, and a 16-entry squared-speed cap. Direction 8
decelerates by `lowbyte(dt)*25`; no floating-point seek or host speed constant
remains in actor movement. The `+$72` boost countdown, boosted vectors and
1.5x cap are preserved and exported as `movement_boost_72` telemetry.

`$87:9127-$9136` proves DP `$E0/$E2` is selected through the active-record
pointer array at `$7E:3449`, built by `$86:D7B8-$D85D` from lineup arrays
`$46F9/$4779`. The proven default lineup order is `[2,0,1,3,4]` on each side,
so C actors now store an explicit roster slot instead of using `actor % 5`.
That slot drives roster `+$3F/+$40/+$42`, shot ratings and sprite identity.
Raw actor velocities in F8/JSON are now the signed 8.8 `+$0E/+$10` words,
making direct Mesen comparison possible.

The camera proxy is now exact at its subject-selection boundary.
`$87:A9D0-$A9E2` resolves signed `$093E`; `$87:95BB-$95D8` copies that actor
or substitutes the ball record when `$093E=FFFF`. Passes, shots, loose balls,
misses and made/dead states therefore follow the ball until `$86:BAA2-$BAEE`
commits a catcher; attached play follows that actor. The independent `$093A`
side group persists while the ball is free and only refreshes on actor/team
acquisition. F8/JSON expose both `camera.subject_raw` and
`camera.side_group_raw`, and the long test covers actor and free-ball paths.

The 61-play formation graph is packed and validated but not yet connected to
all modes. Live proof establishes that its coordinates already use C's world
origin—there is no `+140` or `280-x` host transform—and that index zero is the
first step. The port still needs `$B377/$B2DC-$B309` ownership of `$0998`,
the raw AD6B call gates, and later `$85:AE35+` overrides before replacing its
provisional role targets. The first-possession movement guard is likewise
explicit temporary parity scaffolding. These caveats prevent treating 4E as
the final parity gate.

Increment 4F connects the already-isolated `$85:9ACB-$A081` rim/backboard
shell to live shot integration. Gravity and XYZ integration run first, then a
unit-scale local bridge maps the rendered right/left hoop centers to the ROM's
signed `+336/-336` domain. Outer contacts copy the routine's exact position
and velocity mutations back once; makes enter `$85:A079-$A345`; classified
edge/miss contacts enter loose-ball recovery without inventing the still
unresolved contact impulse. Persistent `$092C/$0962/$096A/$097C/$096E/$13E7`
side effects now survive between frames. Pure self-tests lock both baskets,
boundary classifications, mutations, and bitwise preservation of `$13E7`.

The same trace corrects the conditional think-delay input for modes 2/4/6.
DP `$9E` is not the ball or a paired actor: `$87:8EFE/$8F11` selects the left
team context `$46EB` for slots 0..4 and right context `$476B` for slots 5..9.
Those routines compare signed-byte actor `+$04` with stable context `+$0A`
anchors `$B0` (-80) and `$50` (+80) before adding `$20`; C now performs that
exact comparison. Remaining mapped fields are actor `+$16` signed controller
assignment, `+$7A` recovery inhibit, `+$4C` movement magnitude, and `+$92`
lineup rank. Their planner branches remain pending rather than guessed.

The play-step audit also identifies `$85:C6AF` as a separate 61-entry control
stream graph. Its 320 eight-byte records end per play with a two-byte `$23BA`
sentinel and cannot reuse formation counts. `$85:B377` initializes
`$0998/$099A/$099C`; `$85:B2DC-$B352` loads countdown/event words and three
side-relative player selectors. `$87:8FA1 -> $85:AF5C` advances it once after
each completed ten-actor logical pass with `C6=2`, advancing only after the
countdown underflows. Packing this independent graph and replacing the host
play-state timeline is the next checkpoint.

Increment 4G packs that graph as ROM-derived asset `NBPLAY1` in asset-pack
version 25. The `$85:C6AF` pointer bytes hash to
`ec730707bd9a46518203b109c36d0ca7c76cbae1c1b20f6cf5bb8af22d451ade`;
following all 61 two-byte pointers and stopping only at each two-byte `$23BA`
sentinel yields 320 records (2,560 bytes) hashing to
`5d0775922793118a8e8d0b2b3c5e3a074d09f6a511742822111d057880aa48bd`.
The runtime validates independent counts, offsets, source pointers, and the
known `$35/$01/$0F/$26` streams before exposing a typed four-word accessor.
No Mesen frame or generated recompilation source is stored in the pack.

The available recompilation is useful as an executable hardware oracle but
does not presently provide native C for gameplay. Its generated dispatch table
contains discovered entries only in banks `$80-$82`, while `generic_host.c`
explicitly routes other addresses through `interp_bridge_run_until_quiescent`.
Gameplay semantics therefore continue to come from exact ROM bytes, Ghidra
control flow, and Mesen WRAM/execution traces. The recomp remains valuable for
repeatable end-to-end playback and future function-boundary discovery, not as
a false C-level reference for bank `$85-$87` gameplay routines.

Increment 4H installs the control stream's raw runtime registers and debugger
contract. New possessions execute the proven `$85:B377 -> $B2DC` reset/load,
translate nonnegative selectors through the offense side base, and decrement
positive `$099A` records by `C6=2` only after a completed ten-actor pass.
Underflow advances; `$23BA` cycles to record zero or repeats the final record
when `$09D0` is set. Negative countdown records set `$099E` and intentionally
hold: the prerequisite teammate/event scan is not yet ported, so no host phase
is allowed to impersonate it. F8, CLI, JSON, and the 50,000-frame regression
now expose and lock these boundaries before formation indices consume them.

Increment 4I ports the event barrier at `$87:8FA1 -> $85:AF5C/$B24C`.
After each completed ten-actor pass, `$099A` subtracts `C6=2`; a negative
event record scans exactly the five actors selected by DP `$9E`. An actor is
complete when signed byte `+$16 >= 0` or behavior flag `+$7E & $40` is set.
The first incomplete actor preserves the signed underflow and retries without
changing `$0998/$099E`; all five complete clears `$099E`, consumes bits
`$08/$40` for that side, and advances through `$85:B28B-$B326`. The runtime,
F8/CLI/JSON, and the 50,000-frame regression now retain actor `+$16`, movement
magnitude `+$4C`, recovery inhibit `+$7A`, and behavior flags `+$7E` so this
boundary is directly comparable with a ROM trace.

The same audit corrects the formation mirror contract: `$85:AD6B` tests the
team-context anchor at DP `$9E + $0A` (`-80` left, `+80` right), not ball X.
Modes 1/3/5 refresh persistent `+$56/+58` only after their signed timer and
`+$16/+$7A/+$7E` gates; `$85:AE35+` then distinguishes persistent targets
from local midcourt steering. This checkpoint fixes the asset API naming and
locks the transform, but intentionally leaves live target installation until
the mode gates, `$85:B402` arrival writer, and `$0994` reselect path are ported.
