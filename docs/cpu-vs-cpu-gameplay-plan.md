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
Those routines compare signed-word actor `+$04` with stable context `+$0A`
anchors `$FEB0` (-336) and `$0150` (+336) before adding `$20`; C now performs
that exact comparison. Remaining mapped fields are actor `+$16` signed controller
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
team-context anchor at DP `$9E + $0A` (`-336` left, `+336` right), not ball X.
Modes 1/3/5 refresh persistent `+$56/+58` only after their signed timer and
`+$16/+$7A/+$7E` gates; `$85:AE35+` then distinguishes persistent targets
from local midcourt steering. This checkpoint fixes the asset API naming and
locks the transform, but intentionally leaves live target installation until
the mode gates, `$85:B402` arrival writer, and `$0994` reselect path are ported.

Increment 4J removes the fixed eight-direction hand offsets and fabricated
24-frame bounce from attached-ball play. Asset-pack v26 / `NBPANIM1` schema 5
adds the complete `$AC:A9CF`, `$AC:B267`, and `$AC:A583` upper-resource tables
beside the already-packed `$A9:D86E/$A9:D03E` lower tables. Their SHA-256
hashes are respectively `4e68d7d66128fca5e036897279235651826646d6ddbf4ebb9ea28fc57b2f9cb7`,
`d7b9409e98959dd57e5a74427e3f116804446b530d1ced1654566e8362599d8f`,
and `0b0e1bcac585f59d0f2a6fc03356cafbc9f2cd391af593f42403f758a6d82b58`.

`$87:B649/$B66A/$B832/$B953` now resolves the actor's independent current
upper/lower animation resources and composes exact X/Y/Z offsets. The
canonical `$87:AC13-$AC22` mirror writer maps animation direction `0..2` to
actor `+$28` bit 15; `$B832` XORs masks 2/1 when negative. Positive-up Z is
`AC:A9CF[upper] - A9:D03E[lower] - AC:A583[upper]`. Golden resource vectors
from ROM frames 422/464/599 are validated during tip-off initialization and
in the asset regression, so gameplay cannot silently fall back to host art or
directional constants.

Increment 4K ports the `$0994` play-request boundary at
`$85:B128-$B24B`. A made basket writes play `$01` and request `$01`; the next
completed actor pass consumes one discarded RNG result, clears
`$0994/$099E/$09A4`, preserves `$0996/$09D0` in live state `$82`, and reloads
record zero through the existing `$85:B377` stream reset. Outside that
dead-ball state, the represented normal context now follows the ROM's two
rejection-sampled `rng & 7` draws and coin draw. Team/coin strategy bytes at
`$85:C661` select one of seven base/count pairs at `$85:C729`; strategy five
sets the `$09D0` final-record hold.

Asset-pack v27 / `NBCAI1` contains those 58 strategy bytes and 28 range bytes,
plus the complete 108-byte pass-launch table at `$86:9C6F-$9CDA` and eight
release thresholds at `$86:A7A0-$A7A7`. The pass bytes hash to
`e8b2d2ec179a286a66e52707957c418a9463ba0edc4d87d28779bcfd4431071e`.
They are validated and exposed through typed accessors, but pass mode 15 is
not activated yet: `$86:AB2D-$AF65` still has unresolved raw state-selection
gates, so using the table prematurely would manufacture policy.

The exact predictive-arrival primitive at `$85:B402-$B4B8` is now isolated
and self-tested. It subtracts the actor position and the ROM's biased signed
`velocity >> 3`, measures distance with `$85:F34F`, and treats distance equal
to the caller tolerance as arrived. Live frame-400 vectors lock both the
distance-16 arrival and distance-63 steering cases. Formation modes will call
this helper through the live-covered `$85:AE35+` routes in the next increment;
unobserved special rewrites remain separately gated.

Increment 4L connects the safe, live-covered mode 1/3/5 formation lifecycle.
On the roster-driven signed `+$60` expiry, CPU actors with `+$7A==0` and
signed `+$16<0` install `NBFORM1[play $0996][role][step $0998]` once. Actor
`+$7E bit $08` is the install latch; `$85:B28B/$B359` clear bits `$08/$40`
when the stream advances or resets. The target persists between decision
ticks instead of being overwritten by the port's former per-frame offensive
shape.

`$85:AE35-$AF5B` now handles the normal live routes: opposite-X-sign front
roles steer through local X `+/-16` without changing the persistent target;
back roles under the represented `$097C` activity gate use local court-edge
`+/-338,+/-16`; all other actors use `$85:B402` with inclusive tolerance 16.
Only the normal arrival path sets `+$7E bit $40`, which releases the existing
`$099E` five-actor barrier. The runtime regression validates every observed
bit-$08 rising edge against the ROM asset's play/step/role/mirror/side
coordinate and requires sustained installs and completions by both teams.

The unobserved `$AE3B-$AE95` inbound-teammate rewrite and `$AE97-$AEBB`
DP-`$5C` short-timer rewrite remain disabled, as does the unresolved mode-1
pre-call `$86:E635` side effect and `$09A2` anchor-target writer. `$0948` is
not yet represented at that checkpoint, so the special activity route used
only proven persistent `$097C`. These were explicit next inputs, not
host-authored substitutes.

Increment 4M closes two live-required inputs that the first formation pass
left explicit. `$85:B4B9-$B50D` now advances actor `+$64` with signed
`C6=2` cadence. When `$09A4` is active, possession exists, the ROM
actor-to-basket rectangle at `$85:F5E4-$F715` contains no opponent, and the
actor is within 160 units of `$093E`, it writes that actor to `$09A2`.
`$85:AE1F-$AE32` then clears formation bit `$08` and persists the exact team
basket anchor `(-336,0)` or `(336,0)`. The 50,000-frame regression observes
1,192 bounded selector rows and more than 800 exact anchored rows.

Raw `$0948` is now represented independently from `$097C`. Canonical shot
detach writers `$86:AA01/$B16E` set it to one; actor attachment clears it.
The role-3/4 `$85:AEF5` route tests their OR, matching live execution where
`$097C` remains zero but `$0948` drives the edge clamp. Both raw values and
`$09A2` are exported by F8, CLI, port JSONL, and the Mesen capture script.

The pass audit resolves why the packed `$86:9C6F` table is not yet activated.
`$86:AB2D-$AF65` depends on the fine 16-direction helper `$85:F3C3`, distinct
passer fields `+$62/+$66/+$84/+$C0`, independent animation phase `+$3A`, and
roster bytes `+$39/+$3E`. The current immediate `38/704` host launch is still
explicit provisional behavior. The safe next pass increment must add those
raw fields and helper first, then enable only live-covered states
`$2D/$2E/$2F/$30/$31`; aligned `$2A-$2C`, catch pre-init, and airborne
specials remain gated until their own inputs exist.

Increment 4N establishes the exact pass-direction foundation before changing
live behavior. The repeatable headless Ghidra dump now emits complete
`$85:F3C3-$F472` and `$86:A6B3-$A7D9/$AB2D-$B04A` listings. The C helper
reproduces `$F3C3`'s 16-direction `$B2` and weighted-distance `$AA` outputs,
including its `$10` zero sentinel, wrapped-subtraction N-flag comparisons,
and the 16-bit overflow before the `3*x/2` slope test. Boundary and overflow
vectors lock those non-obvious 65816 semantics. Asset pack v28 also preserves
roster bytes `+$39/+$3E` in `NBPROST2` and exposes a typed accessor, so the
next mode-15 initializer can read the same per-player inputs as the ROM.

Increment 4O activates the first evidence-complete mode-15 slice. At the pass
boundary, C now predicts passer/receiver positions with the ROM's `/16` and
`/8` velocity terms, computes `$09DA`, installs actor `+$62/+$66/+$C0`, and
selects the live-covered grounded upper states `$2D/$2E/$2F/$30/$31` without
changing the lower channel. `$86:A6B3-$A790` keeps `$093E` and the asset-based
ball attachment intact while the upper `+$3A` phase is at or below the ROM
threshold. Only then does the packed `$86:9C6F` duration/vertical record lead
the receiver and detach the pass. JSON/F8 comparison state now distinguishes
the old mislabeled `+$62` from the true saved mode at `+$84` and exports
`$0942/$0946/$09C4/$09DA`. Unsupported aligned `$2A-$2C`, airborne `$AFC4`,
and `$AF66` catch-preinit branches remain explicit gates rather than guessed
fallback animations.
The release also restores `$0936=0` at `$86:9B84-$9B8F`; the shot initializer
then writes `$0936=1` at `$86:9DDB-$9DE4`. This previously hidden state pair
is regression-locked through successful rim makes, score writes, and the
next-pass `$0994` request/consume lifecycle.

Increment 4P establishes the foul-state boundary without inventing collision
policy. Ghidra identifies pending event `$0964`, deferred shooting-foul
`$09BC`, offender/victim `$492D/$492F`, side counters at team-context `+$28`,
personal counters at persistent player `+$14`, and free-throw state
`$0978/$097A`. The isolated C state hook accepts the proven defensive,
charging, and offensive event classes, preserves the six-foul personal cap,
applies the non-deferred team threshold to all three classes, and models the
later made-shot bit-15 transition as a separate operation. It is deliberately
not called by live
gameplay: the current emulator oracle does not exercise a deterministic foul
contact, so enabling it would require a guessed predicate. C and Mesen JSONL
now expose the common raw fields, while the 50,000-frame CPU regression locks
the scaffold dormant until a focused contact trace supplies that predicate.
The reproducible dump covers primary `$86:C4FE-$C6AC` and alternate
`$86:D12D-$D1D0` classification paths. The current CPU oracle reaches the
alternate entry but exits on `$093E<0`, and never reaches `$C4FE`; therefore
live activation remains gated on a focused trace that observes codes 1, 2,
13, and a detached-shot deferral through their complete latch lifecycle.
The hook also receives the offender team explicitly, matching actor `+$70`
rather than assuming slot/5. Literal `$86:C66E-$C684` uses threshold five
when `$0926<4` and threshold four otherwise.

Increment 4Q replaces two more host-authored animation/policy choices with
live ROM behavior. Mode 12 now holds upper/lower resources `$16/$32`, matching
all 26 observed CPU shot frames and the `$86:B769` executor, instead of the
provisional `$31/$03` pair. Receiver selection now follows
`$85:B50E-$B60A`: `$09A2` receives priority, then `$09AA`, with `$09AC` used
only if `$85:B60B-$B677` rejects the first candidate. The isolated validator
rejects self-selection, candidate modes seven and above, the direction-eight
zero vector, and both low-motion head-on conflicts using actor `+$86/+$8A`.
It also preserves the signed, team-normalized `<$00C9` forward-X window.
Pure golden cases lock primary, alternate, head-on rejection, distance, and
special-cutter priority. Live integration no longer fabricates the former
`handler+2` next receiver. A play record with no valid receiver returns from
the pass boundary without invoking `$86:AB2D`; the provisional global planner
then resumes its attack branch rather than deadlocking. That bridge remains
explicitly temporary until the complete mode-11 executor replaces the global
BREAK/DRIVE/ATTACK state machine. The 50,000-frame CPU regression continues
to cover recurring possessions, both-team scoring, passes, shots, rebounds,
inbounds, and bounded `$09A2` cutter lifetimes.

Increment 4R establishes the resource-accurate acquisition boundary. Asset
schema `NBPANIM1` version 6 adds the ROM's second `$87:B832` pose point from
`$AC:CC2F/$AC:BF4B/$AC:C397`; both points continue sharing lower-body tables
`$A9:D86E/$A9:D03E`. Golden vectors reproduce `(12,-20,14)/(20,-12,14)` and
`(17,-15,27)/(-8,8,26)` exactly. C now isolates the coarse
`$86:CCCD-$CCFB` box (X `-16..16`, asymmetric Y `-16..15`, Z `0..71` or
`0..95` for the designated receiver) and `$86:D549-$D5DA`'s strict per-axis
pose cubes. Loose rebounds scan actors 0 through 9 once and commit the first
qualifying contact, matching `$86:CCFC-$D43C`; the former Z-at-most-24,
planar-nearest, distance-14 shortcut is gone. Mode-10/14 receivers no longer
run the generic formation accelerator while `$86:99C4`'s velocity lead is in
flight.

Pass contact now attempts the exact designated-receiver 16-unit pose cube,
but retains a clearly labeled Z=0 completion bridge. The stricter gate exposed
an upstream mismatch: the current global play geometry produced a 451-unit
pass where the comparable ROM pass was about 197 units, so the host pass hit
the floor before reaching either hand point. Opponent interception/foul RNG at
`$86:D000-$D1CE` also remains disabled. Therefore this increment proves the
asset, primitive, and loose-rebound portions; it does not claim the complete
pass acquisition chain until mode-11 formation and `$86:99C4` flight parity
remove that fallback. The repeatable headless dump now includes the complete
`$86:BAA2-$BF0B` possession-install boundary, and the 50,000-frame regression
passes with updated visual hashes.

Increment 4S replaces the remaining escaped-ball and selector shortcuts with
their live ROM contracts. `$85:A3B7-$A656` now applies gravity before Z
integration, 7/8 ground restitution capped at `$0400`, signed 15/16 lateral
impact damping, and the small-bounce settle threshold `$18`. The shared
`$85:A656-$A755` record clamp is used by actors and free balls: integer X is
bounded to `-394..394`, Y to `-224..224`, only outward rectangular velocity is
cancelled, and the asymmetric isometric edge enforces `x >= -556-y` for
negative Y or `x <= 561-y` otherwise. Fractional coordinates survive every
clamp. Rectangular contact also takes `$86:A613-$A628` and clears the represented
`$0942/$0946/$0948` state; the mode-15 executor therefore retains its actor-local
pass authority rather than incorrectly treating those telemetry words as
durable ownership.

The complete `$85:B50E-$B60A` audit corrects Increment 4Q: `$09A2` has
priority, a negative or self `$09AA` returns immediately, and rejected normal
candidates fall through AA, AC, then AE. B50E and `$86:AB2D` never consume
`$09A2/$09AA/$09AC/$09AE`; play-record loading or ball acquisition replaces
them. A normal validated selector writes owner `+$60=1`. A C-only unsupported
AB2D animation family now retains mode 11 and the selectors for retry instead
of destroying the opportunity. Play `$0F`'s first two all-FFFF records are
therefore intentional formation barriers, followed by role-3 and role-1/0
receiver records.

`$86:B625/$B769/$B8CA-$B978` now owns the shot jump: mode 12 installs upper
`$16`, lower `$32`, velocity `$0210`, pose-attaches the ball, and releases only
through the signed/low-velocity RNG gate. `$86:9D6E-$A45E` detaches into upper
`$17`, launches the shot, returns the shooter to mode 11, and lets the following
non-owner dispatch fall to mode 1. Regression checks use that velocity-driven
sequence rather than the former unrelated mode-16 timer lifecycle.

Finally, catches and rebounds share the represented `$86:BAA2-$BC99`
acquisition contract. It installs the first pose-collision winner, clears
`$09A2/$09AA/$09AC/$09AE`, requests a new asset-backed team strategy through
`$0994`, conditionally resets `$092C` on a side change, and clears stale
`$0948/$094C/$096A/$097C` after old-shot classification. `$097C` clearing at
`$86:BC8A` is essential: leaving the rim value `$05A0` alive trapped roles 3/4
in the edge route forever. The old four-play host rotation is no longer used
after catches; the 50,000-frame regression now covers the ROM strategy ranges,
both-side scoring, mode-12 releases, boundary contacts, collision-owned
rebounds, repeated inbounds, and sustained camera/player movement.

Increment 4T replaces the post-score timer shortcut with the ROM inbound
target and pass executor. The former bridge attached the ball when a host
timer reached 120 and began a new possession at 60. Continuous Ghidra dumps
and the 1,800-frame Mesen oracle disprove that interpretation:
`$85:C37D-$C5C0` selects team slot 2, its target/facing, and a play; mode 11
then reaches `$86:F43A-$F653` while live state remains `$82`.

The native path now compensates its steering target by signed velocity `/16`
toward zero, accepts only raw X/Y deltas `[-9,+8]`, and reloads `$092E=300`
on every failed arrival. Timer 240 never attempts a pass, 239..120 uses
`RNG & $003C`, and below 120 always validates `$09AA/$09AC/$09AE` through
the represented `$85:B60B` predicate. Only below 60 may the side-slot-4/3
fallback be used. The context-side X guard precedes `$86:AB2D`, which installs
mode 15/10; release remains in `$86:A6B3-$A790`, and the shared
`$86:BAA2-$BC99` catch boundary alone completes live state `$82`.

The synthetic `NBA_BALL_INBOUND` timer phase is no longer part of this path.
Debug JSON and F10 now expose layout `$0956`, target `$0958/$095A/$095C`,
arrival state, transfer latch `$09B8`, and timer `$092E`. Pure golden vectors
lock target and arrival endpoints; the 50,000-frame runtime test locks the
300-reset sawtooth, transfer timing, selector handoff, pose attachment, catch,
repeated scoring, and both teams. Headless Ghidra now emits complete
`$85:C37D-$C600` and `$86:F3D2-$F669` sections. The expanded Mesen script
completed a 251-frame schema smoke capture, and native frames 1900/1966 show
arrival and catch completion without corrupt court/player composition.

Increment 4U corrects three assumptions exposed by direct component
differential testing. Initial `$0954` slot 2/7 is only provisional: the ROM
keeps `$093E=-1` until `$86:CCFC-$D43C` finds the first inbound-side pose
collision, which may replace it (the oracle changes 2 to 3 at frame 918).
The port now preserves that separation and no longer derives `$093E` from the
logical ball owner at the end of every host frame. During the dead-ball walk,
the ball uses the selected player's resource attachment coordinates while its
logical owner remains negative; `$86:AB2D` alone installs mode 15 and logical
attachment for release.

The regenerated listing also corrected layout state 2. `$85:C49E-$C4D1`
uses X `+/-226` from context `+$0A`, Y `+/-224` and direction 0/4 from
`$09B2`, then calls endline selector `$85:C602`. The previous `+/-160` clamp
and direction 2/6 belong to neighboring negative-layout path `$85:C50B`.
Golden cases now distinguish both states.

Finally, `$87:9AA6-$9BCA` is represented as expired-inbound recovery: reload
300, select layout 5, award the opposite `$093A ^ 5` side, demote the expired
carrier to mode 2, clear signed possession, restore the shot clock, and reseed
`$85:C37D`. The made-ball bridge also closes its host subpixel hoop overshoot
at the oracle's proven `+/-336,0` cylinder so a valid make cannot drift to a
court corner before collision visitation. A 50,000-frame run again sustains
both-team scoring, collision-selected inbounds, transfers, and catches.
Evidence is under `build/evidence/cpu-inbound-v2/`. This checkpoint does not
claim final half-court spacing or the remaining collision/rim/animation work.

Increment 4V uses the working recompilation as an instruction-exact gameplay
oracle instead of treating it only as a rendered reference. A focused
`snesrecomp` configuration for bank `$85` exposes the complete
`$85:9ACB-$A656` register/memory flow. Manifest AOT can lift the edge response
at `$9DAC`, miss response at `$9F01`, gravity at `$A3B7`, and clamp at `$A656`;
the monolithic entry remains LLE in that mode because the scoring call
`$85:A346->$83:CE36` has an unproven transitive exit. Direct regeneration still
emits the full body, making both paths useful translation evidence without
copying emulator dispatch or CPU-flag machinery into the port.

That evidence corrected the live 30-Hz ball order. `$85:9A78-$A345` tests the
current integer position and mutates rim velocity/state first; only then does
`$85:A3B7-$A5F1` apply gravity and integrate, followed by the `$85:A656+`
court clamp. The port formerly integrated and clamped before rim contact and
returned immediately, delaying a reflected impulse by one logical tick. It now
uses the ROM integer word for collision, applies the response before same-tick
integration, and replaces only collision-touched integer axes while retaining
all subpixel bytes. The make gate now compares the ball X sign with the selected
basket context sign as `$85:9D65-$9D7B` does, replacing the former tautological
handler-team check. A live-orchestration self-test locks that a `Y=-24` lip
contact moves with its reflected velocity on the contact tick.

Two outer-shell details are also corrected directly from `$85:9B8C-$9BB3`:
negative Y contact is inclusive at `-24`, and `$096E=15` is installed only
when the timer is zero rather than restarted on every contact. Exact inner
distance-7 and distance-8..10 velocity responses are now decoded, but remain
the next checkpoint because their `$0920/$0948/$094A/$0970/$0978/$09F8/`
`$1866/$07F6` inputs and forced-state oracle vectors must be represented
together. This checkpoint does not relabel those branches as generic bounce.

Increment 4W ports those two inner responses from the focused recomp output
and continuous Ghidra control flow. `$85:9F01-$A006` now performs the
distance-8..10 and `$09F8`-vetoed miss transition: common activity/timer
resets, optional `$1866` force, signed planar halving and reflection, the
zero-planar Y escape, reversed/damped VZ, live-state clear, response effect 3,
`$0970/$13E7` arming, and the conditional `$07F6/$80:CEE7` low-speed kicks.
`$85:9DAC-$9EFE` remains distinct and derives its twice-thresholded X/Y
impulses from rim geometry before applying its own low-speed kick constants.
Both paths tail into same-tick gravity/integration rather than becoming a
host-authored generic bounce.

The component API explicitly carries `$0920/$0936/$0948/$094A/$0970/$0978/`
`$09F8/$1866/$07F6` so no hidden host defaults can choose a branch. Forced
golden vectors cover distances 8, 9, and 10 at both baskets, signed damping,
event/global writes, and exact LFSR advancement. The existing ROM oracle also
proves `$85:9F01` executes in live play and records `$0970` counting 15 down to
0 across frames 1652..1683; the native 30-Hz pass now mirrors that countdown.
Gameplay JSON exposes `$0920`, `$0970`, and the `$87:A9E3` effect selection for
future differential traces. The effect's asset/animation dispatch is retained
as explicit state and remains part of the animation-dispatch work, not silently
discarded as emulation plumbing.

Increment 4X ports the made-ball physics handoff instead of freezing the ball
at the hoop. Ghidra/recomp control flow `$85:A079-$A345` performs scoring and
dead-ball setup, then `$85:A34A-$A3B3` anchors X at signed 336, clears Y and
planar velocity, retains signed VZ/8, clears `$0948/$094A/$0962/$096A/$09B8`,
reloads `$092C=$05A0`, and enters `$85:A3C8` after the normal gravity entry.
The native path now performs those represented writes, rebuilds the integer
ball coordinates so stale host fractions cannot survive the anchor, and runs
the reduced vertical motion through same-tick integration without applying
free-flight gravity. A forced negative-VZ golden vector protects 65816
arithmetic-shift behavior. The sustained CPU test now judges rebound ownership
only for misses with a complete 600-frame resolution horizon, preventing a
shot begun in the final capture tail from being mislabeled as an unresolved
physics regression.

Increment 4Y closes the post-impact settle omission exposed by the focused
native recomp. `$85:A5F4-$A655` runs only at Z zero with unsigned VZ below
`$0018`; before changing velocity it observes `$09B8` to clear `$0936`, calls
the exact `$86:A613-$A628` activity reset, and maps free-throw state
`$0978=$000A` to `$097C=1`. It then zeroes VZ and treats VX/VY independently:
signed magnitude below `$0018` becomes zero, after which each axis is always
arithmetic-shifted right once. The old C path stopped after thresholding and
therefore retained twice the ROM's lateral energy.

The isolated settle component now carries the represented `$0936/$0942/`
`$0944/$0946/$0948/$094A/$0978/$097C/$09B8` contract. Golden vectors protect
the signed `-101 -> -51` half, the pass/live resets, the free-throw latch, and
the exact Z/VZ entry gates. Gameplay, tip-off, and the sustained 50,000-frame
CPU regression all execute with the corrected settle physics.

Increment 4Z ports the gameplay-effect dispatcher that the ball/rim path
selects. Focused native recomp output and Ghidra agree that `$87:A9E3-$AA01`
writes effect `$401B`, reads one of six descriptors at `$85:8AB4-$8B0F` into
gate `$3F33`, and clears frame `$4025` and timer `$402D` without immediately
changing resource `$4015`. `$87:AA02-$AAB1` advances the descriptor with the
ROM's dt value 2, advances at most one frame per call, and returns to inactive
resource `$0822` when the descriptor ends. All six exact resource/duration
records are represented in C; selector 3 now emits `$082A` at timers 2, 4, 6,
and 8 before its timer-10 terminal reset.

The high-Z descending-net gate is also literal: unsigned Z at least `$004A`,
negative `(VZ-1)`, permitted latch/effect state, and signed Y distance below 8
select resource `$082D` and clear `$3F33` without advancing the frame timer.
The exact distance-8 boundary, duration carry, single-step large-dt behavior,
and inactive reset have forced component vectors. Scheduling follows
`$87:8F95-$8FA9`: ball physics runs first, then the effect step on that same
30-Hz pass, so a miss receives its first `$082A` resource immediately.
Gameplay JSON now exposes all five raw dispatcher words for future Mesen
differential traces. Rendering resource `$4015` still requires extracting its
ROM graphic records; the state is intentionally not mapped to captured art or
the existing static host ball tile.

Increment 5A closes the remaining represented output of the native ground
impact branch. `$85:A3B7-$A4DA` computes 7/8 vertical restitution (capped at
`$0400`) and 15/16 planar damping; `$85:A43A-$A44B` also stores the rebound
word in `$13E5` and raises bit 0 of `$13E7` only when the unsigned impulse is
at least `$0048`. The previous port used the correct velocities but discarded
both gameplay-visible writes. A shared component now owns the complete impact
response, telemetry exposes `$13E5`, and boundary vectors distinguish rebound
71 from 72 while protecting signed planar damping.

Increment 5B fixes a width error found by translating the native mode-2/4/6
countdown entries `$86:F6CD/$F794/$F8CD`. Their half-court delay test executes
with M=0 and XORs the full signed 16-bit actor X (`+$04`) with the full context
anchor (`+$0A`). The port had cast both to signed bytes, so actor X `+200`
against anchor `-336` incorrectly looked same-half and added `$20` to its next
decision interval. The shared predicate now preserves the native word XOR;
four sign combinations and the reproducing `+200/-336` case are locked by the
AI component self-test.

Increment 5C translates the portable target geometry shared by defensive
modes 2/4/6 without prematurely wiring guessed context. The five exact
20-word circular tables at `$86:E82F-$E8F6` encode radii 168, 64, 48, 24,
and 19 across 16 directions plus the four wrapped Y entries. The selector at
`$86:E8F7-$E922` consumes context `+$30/+$32`; `$86:E923-$E96E` adds the
selected X/Y offsets to paired position plus arithmetic velocity/8, with
16-bit wrapping. `$86:E7DC` may force the radius-19 table. A pure C component
now owns this exact behavior and forced vectors lock ordinary radius-64,
forced-close, and context-`+$30==2` targets. It remains deliberately unhooked
from live defenders until those two context words are added to the runtime.

Increment 5D adds the persistent left/right team-AI contexts rooted at
`$46EB/$476B`. Recomp initialization `$86:DBF1-$DC03` sets both represented
contexts to `+$30=4`, `+$32=1`, and activity `+$39=1`. Play-request handling
at `$85:B128-$B176` consumes its first `$80:CEE7` result to update the
opposing context's `+$30`: a signed 16-bit subtraction-N test selects mode 1
when trailing before period 3, otherwise RNG bit 0 selects mode 3 or 1. The
C runtime and JSON telemetry now retain these raw fields and `$0926`, and all
true team-context anchor consumers use the full `$FEB0/$0150` words. Forced
unit vectors cover the inactive-context guard, trailing branch, period gate,
and both RNG outcomes. The gameplay lab, tip-off, and sustained 50,000-frame
CPU regression pass with the new state lifecycle.

Increment 5E replaces the continuously executed host defensive shortcut with
the portable recomp target dispatcher. Actor `+$74` now remains a mutable even
offset into `$87:9C7B`, initialized from the active-lineup permutation rather
than same-index pairing; `+$76` base and reciprocal `+$78` remain distinct.
`$85:BC52-$BC81` coarse pair direction/distance (`+$86/$8A`) and
`$85:AFC2-$AFE5` fine basket direction/distance (`+$88/$8C`) feed exact C
translations of `$86:E7B3`, `$E7DC`, `$E96F`, `$E6B7`, and `$E9B3`, including
the five ROM spacing tables, three-point arc/rating branch, velocity eighths,
close-basket overrides, and mode-6 direction maps. The fixed `+/-18` shoulder
offset and host `first_move_start[10]` table are gone.

The same checkpoint restores the ROM's ownership boundary: `$86:BB3B-$BB4E`
copies every base `+$76` into current `+$74` and raises represented `$09D6`;
the end-of-ten-actor `$87:8FA1-$8FA9` hook then runs the represented
`$85:AF5C/$85:BC07` order, preserving modes 7+ while normalizing offense to
mode 1, defense to mode 2, and the owner's assigned primary defender to mode
4. Optional `$85:C018-$C036` help-defender selection that promotes mode 6 is
still pending; mode 6's target geometry itself is translated and unit-tested.
Gameplay JSON exposes team contexts, `$09D6`, `+$88/$8C`, and roster `+$92`.
Visually reviewed frame goldens 220/600/1300 and the Gameplay Lab were updated
after the 50,000-frame sustained CPU trace passed.

Increment 5F ports the complete portable team-defense coordinator rather than
leaving the mode 2/4/6 executors behind a host-authored matchup shortcut. A
focused native recomp oracle for `$85:BE06-$C0F5`, checked against the Ghidra
listing and the existing Mesen CPU trace, establishes the dependency boundary.
`$87:9090-$90A0` first snapshots all ten current modes into actor `+$84`;
`$85:BC07` then refreshes basket (`+$88/+$8C`), focal (`+$8E`), and assigned
pair (`+$86/+$8A`) geometry before applying the planner.

The C runtime now preserves the ROM's live-state and mode-9 bypasses, dynamic
assignment release tests, `+$72=$1E/$14` movement boosts, primary mode-4 repair
through `$85:BAE4/$BA1D`, and mode-6 help selection through `$85:BB6C/$BBBF`.
Help assignments remain deliberately one-way, last-candidate tie behavior is
retained where the ROM uses it, and the next pass recognizes the old helper
through saved mode 6 before rebuilding coverage. The no-owner `$85:C052`
fallback and final context-ordered `$85:C0B4-$C0F5` pairing pass are also
represented. Context `+$4E` is initialized to `$00A0`, confirmed in ROM WRAM
at frames 220, 400, and 1800; context `+$49..+$4D` uses the exact five actor
offsets from the same snapshots.

Forced native self-test vectors protect alternate help selection, last-tie
fallback selection, and one-way old-helper release. The sustained integration
test now treats immutable `+$76` lineup assignments separately from mutable
`+$74` current assignments and runs 60,000 frames so the corrected deterministic
movement covers a complete miss and rebound. Frames 600 and 1300 were visually
reviewed before their RGB hashes were updated. This checkpoint ports no PPU,
OAM, DMA, bank-dispatch, or CPU-flag machinery from the recomp.

Increment 5G replaces the receiver-only/floor-catch bridge with the portable
player/ball contact path recovered from focused native recomp output and the
headless Ghidra listing. `$86:D5DB` performs a stable signed-world-X insertion
sort of the actor list and `$86:D652-$D728` preserves that order when presenting
actor/ball pairs. `$86:CCCD-$CCFB` supplies the inclusive 16-unit broadphase;
`$86:D549-$D5DA` checks two asset-derived pose points with strict cube bounds.
C now uses those same ordering and bounds rules for loose balls, inbound
installation, pass catches, and opposing interceptions. The detached-pass
classifier rejects same-side nonreceivers, uses radius 16 for `$0946` and 12
for opponents, and preserves the original `$86:D11F-$D128` low-Z bug that
compares the random byte with pose-point selector DP `$00` instead of the
computed chance in DP `$AA`. The former `ball.z <= 0` receiver auto-catch is
gone.

The body fallback at `$86:CF23-$CF89` now derives actor `+$AA` from raw
asset-pack animation descriptors. This mirrors `$87:A60D-$A6B2` after
`$80:AD92` composes lower, upper, jersey-number, and head resources, including
the exact `foot_y - top_y + 11` extent. Telemetry exports `+$5A` contact
inhibit and `+$AA` height. The portable `$86:D4E3-$D544` deflection velocity
response is isolated and forced-vector tested; its remaining shot/strip call
predicates are not guessed and remain part of the next collision checkpoint.

The same audit found a separate receiver lifecycle error. Behavior table
`$87:9BD3` maps mode 10 through `$87:9C3A` to `$86:A5B0`. When `$0946` is
valid it decrements actor `+$60`; expiry or a negative `$0946` invokes
`$86:9846-$986C`, restoring mode 1/2 by actor `+$6E` versus `$093A` and
clearing the receiver temporaries. C previously coasted mode-10 actors
indefinitely, leaving stale receivers against a court boundary. The exact
lifecycle is now ported, and the 60,000-frame regression fails if an invalid
receiver remains in mode 10 beyond the two-rendered-frame 30-Hz scheduling
latency. Updated frames 600 and 1300 were visually reviewed before their RGB
goldens were accepted.

Increment 5H connects the owned-ball half of the same contact dispatcher.
Ghidra and the focused native recomp agree that `$86:CD16-$CD33` rejects the
carrier's teammates, `$86:CEBE-$CF0D` selects a strict pose-point radius of 12
for animation `$13` and 4 otherwise, and this path has no body-box fallback.
The random stages at `$86:D035-$D128` are translated literally: ordinary
animations first pass the 1-in-8 gate, animation `$13` uses the `$17AF`
difficulty bit gate, and the ROM revision compares the following random byte
to pose-point selector DP `$00` rather than the chance accumulated in DP
`$AA`. Deterministic RNG vectors protect both point-zero rejection and the
point-one foul/strip outcomes.

The final strip threshold at `$86:D1D9-$D200` comes from the active player's
ROM record byte `+$3A`; it is now stored in the asset-pack roster record and
validated for ordinary and all-star players. On success, `$86:D205-$D257`
puts contact inhibit 15 on the former owner, clears logical ownership, copies
the owner's signed 8.8 velocities into the ball, and restores the owner's
mode through the represented `$86:9846` boundary. Ordinary contacts commit
the opponent through the existing `$86:BAA2` acquisition translation;
animations `$32/$33` instead run the already-tested `$86:D43E-$D548`
deflection and continue as a loose rebound.

The proven `$86:D12D-$D1CE` branch now records defensive foul code 1 with the
candidate/owner pair when committed Rules word `$17D1` wins its random gate.
The downstream whistle/free-throw scene is not yet translated, so this
checkpoint exposes but does not claim that consumer. F8/JSON collision
telemetry no longer fabricates the jump-ball pair during live play: it emits
the actual actors and `$86:D12D`, `$86:D1D9`, or `$86:D43E` branch that won.
The 60,000-frame test accepts only opponent pairs and exact foul bookkeeping;
unsuccessful ROM contact rolls deliberately advance the shared RNG and thus
change later deterministic scoring without replacing ball or shot physics.

Increment 5I translates the detached descending-shot branch instead of
treating every miss as a generic floor rebound. The recomp and refreshed
headless Ghidra dump agree that `$86:9DBF/$9DFF` preserve the shooter in
`$09C8` and the one/two/three-point value in `$096A`. `$86:CD97-$CDBD` then
requires negative shot VZ and a strict eight-unit asset-derived pose point;
there is no body fallback. The candidate must be on the opposing five-actor
side and the real match clock `$0928` must be in the unsigned `[5,$FF00)`
window. C now owns those latches and the captured clock cadence: 43200 at
gameplay frame 220, 43020 at 400, and 41620 at 1800.

When committed rule `$17DB` (`rules[5]`) is enabled, `$096A` is live, the ball
is at least `$50` high, and no foul/inbound/whistle state blocks it,
`$86:CE1E-$CE65` emits basket-interference code 6 and awards `$094C` to the
original shooter. This event does not increment team or personal fouls. The
port also retains `$1403/$1405/$1407` leading-side and lead-change writes.
The same pose contact continues through `$86:D078-$D128`: nonzero `$097C`
acquires immediately; otherwise the ROM again compares the RNG low byte with
pose selector DP `$00`, so point zero never catches and point one catches only
on a zero byte. Forced component vectors and the sustained 60,000-frame test
lock the predicate, shot latches, clock, and ordinary physics lifecycle.

The exact `$85:93F5-$945E` pending-event consumer is now a tested reusable C
state transition: it moves `$0964` to `$08F0`, clears the pending/shooting
latches, raises `$09B6/$4937`, updates `$13E7/$13E9`, and installs the
300/120 timer plus `$08E6/$08E8=17`. Headless analysis of bank `$83` also
proves that `$83:EBD8` is only `STZ $08E2`; `$87:BACB` queues its presentation
resource only when the old signed `$08DE` is negative. Both outcomes are now
represented and regression-tested independently of the eventual host SFX
playback binding.

Code-6 interference now runs through the live dispatcher. `$87:92AD-$92E9`
selects the contact actor's side group, records the actor in team-context
`+$3F`, clears `$0956/$096C`, sets `$0966=FFFF`, and enters the shared
`$87:9B41-$9BC8` dead-ball initializer. The C translation sets `$0936=82`,
`$092E/$09D6=300`, `$092C/$09C6=05A0`, saves the ball position to
`$09B0/$09B2`, returns a prior owner to mode 2, zeroes its planar ball
velocity, cancels pass/shot ownership, and starts the asset-driven inbound
target flow. A refreshed whole-ROM write search found the missing lifecycle
edge at `$86:F56E-$F577`: when the inbound actor reaches its target, the ROM
clears `$09B6/$0964` immediately before setting `$09BA=1`. The port now does
the same, so the whistle neither remains stuck nor releases early.

### Recomp boundary

The locally verified recomp is useful as an execution oracle, but its current
generated AOT C contains only banks `$80-$82`. Gameplay in banks
`$83/$85/$86/$87` succeeds through the bundled interpreter fallback, so there
is not yet a high-level gameplay C routine available to copy verbatim. For
gameplay work, use its traces/state snapshots to identify executed entry
points, recover those exact blocks with the maintained headless Ghidra dump,
then translate portable rules, AI, animation, ball physics, camera, scoring,
and dead-ball state into the native model. Do not import 65816 dispatch,
memory-bus, PPU/DMA, scheduler, or SPC-emulation machinery.
