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

## Confirmed ROM map

The initial Ghidra/Mesen audit established these hooks. Raw values remain in
the JSONL alongside interpreted C fields so later labels can be corrected
without changing the comparison contract.

| ROM/WRAM location | Evidence-backed role |
|---|---|
| `$80:CB8F–$CD7D` | controller auto-read into held masks `$0576–$0580` |
| `$86:8000–$8212` | controller ownership, assignment, and repeat timers |
| `$87:9B38` | gameplay controller-mask accessor |
| `$093E/$0940` | controlled player index and resolved actor pointer |
| `$0952/$0954` | human side group and initial controlled slot (2 or 7) |
| `$34EB + slot*$100` | ten player actor records |
| `$3EEB` | ball actor; position `$3EEF/$3EF3/$3EF7` |
| `$0946` | transient possession/ball-owner player index |
| `$87:A160–$A2CE` | human-versus-CPU steering split |
| `$85:BC43–$BD7D` | assignment target, direction, distance, control mode |
| `$85:B95C–$B9D1` | AI reaction threshold using ball distance and RNG |
| `$87:B832–$B952` | directional movement calculation |
| `$86:CED6–$D43C` | tip contact and possession resolution |
| `$85:B100–$B28B` | randomized initial possession/play decision |
| `$85:8EE6–$9191` | provisional camera/court streaming state |

The longer CPU-only capture now covers 651 frames. It confirms the `$35`
post-tip play, CPU reaction staggering, live matchup reassignment, the
eight-direction movement map, ballhandler changes, and camera projection.
Complete pass/shoot/switch policy and scoring remain later gameplay work; the
current C state machine implements the traced first possession far enough to
exercise ten autonomous actors and a CPU ball transfer.

`tools/mesen_tipoff_capture.lua` writes the ROM-side equivalent to
`gameplay_rom.jsonl`. Compare it with the port trace using:

```powershell
python tools/compare_gameplay_traces.py `
  --rom-trace .analysis/<capture>/gameplay_rom.jsonl `
  --port-trace gameplay-port.jsonl --report comparison.json
```

Core mode locks phase plus all ten actors' identity, visibility, world
coordinates and visible sprite origins/directions/animation. `--mode all`
reports every common non-unknown field, making unfinished controller, camera,
ball and AI behavior visible without incorrectly passing it as implemented.

## Regression contract

`tools/test_gameplay_debugger.py` locks the F8 mapping, ten actor records,
eight-player settled-camera visibility, CPU-only mapping, controller,
ball, camera, collision and AI raw fields, overlay pixels, JSON parsing, and
paused single-frame stepping, and both comparator pass and intentional-failure
paths. `tools/test_cpu_gameplay.py` separately protects the live CPU behavior.
