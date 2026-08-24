# Gameplay Lab and ROM comparison telemetry

F8 opens the live Gameplay Lab after the tip-off scene begins. It overlays all
ten actor slots on the court without replacing the ROM-derived player/court
assets. Up/Down selects an actor, Left/Right changes the detail page, A pauses
or resumes simulation, and X advances exactly one simulation frame while
paused. F9–F12 retain their existing Player Lab, HUD, audio, and asset roles.

The CLI exposes the same state as newline-delimited JSON:

```powershell
.\build\nba95_port.exe --headless --rom '<rom>' `
  --assets .\build\nba95_assets.pak --tipoff-only --frames 220 `
  --gameplay-trace gameplay-port.jsonl
```

`--gameplay-lab --gameplay-actor 0..9 --gameplay-page 1..3` renders a lab
proof frame. `--gameplay-paused --gameplay-step-count N` verifies deterministic
single-frame stepping in headless runs.

F8 page 3 and the normal CLI debug state expose `PLAY:code/step T:count W:wait`.
JSON adds raw `$0998/$099A/$099C/$099E/$09A4/$09D0` values plus the three
side-relative `$09AA/$09AC/$09AE` selectors. Positive countdown records advance
once per completed 30-Hz actor pass; event records deliberately hold until the
ROM teammate/event scan that clears `$099E` is ported.

## Confirmed ROM map

The initial Ghidra/Mesen audit established these hooks. Raw values remain in
the JSONL alongside interpreted C fields so later labels can be corrected
without changing the comparison contract.

| ROM/WRAM location | Evidence-backed role |
|---|---|
| `$80:CB8F–$CD7D` | controller auto-read into held masks `$0576–$0580` |
| `$86:8000–$8212` | controller ownership, assignment, and repeat timers |
| `$87:9B38` | gameplay controller-mask accessor |
| `$093E/$0940` | current possession/catcher actor index and resolved actor pointer |
| `$0952/$0954` | context-dependent side group/slot; after a make, opponent inbound group and actor 2 or 7 |
| `$34EB + slot*$100` | ten player actor records |
| `$3EEB` | ball actor; position `$3EEF/$3EF3/$3EF7` |
| `$0946` | transient possession/ball-owner player index |
| `$87:9244/$9BD0` | actor `+$5E` behavior-mode dispatch |
| `$85:BC43–$BD7D` | assignment target, direction, distance, control mode |
| `$85:B95C–$B9D1` | AI reaction threshold using ball distance and RNG |
| `$87:B832–$B952` | directional movement calculation |
| `$86:CED6–$D43C` | tip contact and possession resolution |
| `$85:B100–$B28B` | randomized initial possession/play decision |
| `$85:9192–$93F4` | camera subject transform and adaptive approach |
| `$85:8EE6–$90C3` | circular court streamer sourced from `$A0:8006` |
| `$85:9D40–$A079` | final hoop/rim/made classification |
| `$85:F1C1` | weighted max/min hoop-distance helper |
| `$86:A561–$A5AF` / `$85:ABFB` | two/three-point arc classification/table |
| `$85:A1E9–$A26E` | score write and post-make inbound initialization |
| `$85:C37D–$C5C0`, `$86:F3D2–$F653` | inbound steering, receiver and pass path |

The extended CPU-only capture covers 1,801 frames. It confirms the `$35`
post-tip play, CPU reaction staggering, live matchup reassignment, the
eight-direction movement map, ballhandler and offense changes, camera
projection, and the actual coordinate writers. The C state machine therefore
continues across possessions and uses separate attached/pass/shot/bounce ball
modes rather than a timed first-pass script. Complete rules, scoring, and
collision policy remain later gameplay work.

`tools/mesen_tipoff_capture.lua` writes the ROM-side equivalent to
`gameplay_rom.jsonl`. Compare it with the port trace using:

```powershell
python tools/compare_gameplay_traces.py `
  --rom-trace .analysis/<capture>/gameplay_rom.jsonl `
  --port-trace gameplay-port.jsonl --report comparison.json
```

For scheduler/physics comparisons, use `--logical-passes`. An SNES NMI can
interrupt `$87:8EFB-$8F92` after any actor, so one logical 0..9 pass may appear
as prefix/suffix slices in adjacent ROM JSON rows. The option coalesces those
slices and compares the completed state with one atomic C simulation tick:

```powershell
python tools/compare_gameplay_traces.py `
  --rom-trace .analysis/<capture>/gameplay_rom.jsonl `
  --port-trace gameplay-port.jsonl --logical-passes --mode all
```

Core mode locks phase plus all ten actors' identity, visibility, world
coordinates and visible sprite origins/directions/animation. `--mode all`
reports every common non-unknown field, making unfinished controller, camera,
ball and AI behavior visible without incorrectly passing it as implemented.

For a movement-oriented summary rather than an exact-field diff, run:

```powershell
python tools/analyze_cpu_gameplay_trace.py gameplay-port.jsonl --require-sustained
python tools/analyze_cpu_gameplay_trace.py gameplay_rom.jsonl
```

It prints every play/offense and ball-mode transition, per-actor and per-team
movement counts for each time window, and the maximum ball-to-owner attachment
distance. `--require-sustained` fails on stationary teams, fewer than four
recurring play codes, missing pass/attach/shot/inbound physics, or a detached
owned ball.

## Regression contract

`tools/test_gameplay_debugger.py` locks the F8 mapping, ten actor records,
eight-player settled-camera visibility, CPU-only mapping, controller,
ball, camera, collision and AI raw fields, overlay pixels, JSON parsing, and
paused single-frame stepping, and both comparator pass and intentional-failure
paths. `tools/test_cpu_gameplay.py` separately protects 50,000 frames of live
CPU behavior, score monotonicity, made-ball Z, `$0952/$0954`, the 300-tick
inbound gates, and resumed possessions for both teams.
