# NBA Live '95 native C port

An in-progress native C99 port of the US SNES release of NBA Live '95. The
current playable path covers the Nintendo license, NBA legal screen, EA SPORTS
intro, animated title, Game Setup, Exhibition Team Select, Player Setup,
introductions, tip-off, and CPU-vs-CPU gameplay. The gameplay port is
substantial but not at whole-game parity: ordinary human offense/defense and a
matching native/C launch context remain open. A bounded human free-throw aim
sequence is implemented in the scene adapter, but remains dormant until the
missing Player Setup ownership/context pipeline exists; it is not a complete
human-control path.

The current [parity gap report](docs/parity-gap-report.md) separates native
evidence from C-only regressions. The latest
[edge-contract checkpoint](docs/native-edge-parity.md) corrects ball ownership
and substeps, actor boundaries, inbound/OOB rules and halftime formation
anchors; the strict full-game differential still fails its initial-state
comparison. See [STATUS.md](STATUS.md) for verified scopes and remaining gaps.

The [out-of-bounds overlay](docs/out-of-bounds-hud.md) uses the original font,
message and possession-team label. Its headless smoke test captures every
frame through appearance and removal, with Ghidra/recomp and Mesen evidence.

The [documentation index](docs/README.md) links the current subsystem guides,
reproduction steps, evidence ledgers, and remaining-work reports. Historical
task reports and dated completion plans remain available in Git history.

The [known original-game bugs and preserved quirks](docs/known-original-game-bugs.md)
catalog separates demonstrated original defects from unusual behavior and port
errors. Each entry records its source evidence and whether the preserving
component is enabled in normal gameplay.

Graphics and audio come from a user-supplied ROM-derived asset pack. The title,
Game Setup, and Player Introduction music use the original SPC700/S-DSP state
and BRR samples; the pack does not contain rendered video or mixed music WAVs.

## Requirements

- Windows 10/11 and Visual Studio 2022 with the Desktop C++ workload
- Python 3.10+ with `pip install -r requirements.txt`
- The verified US ROM with normalized SHA-256
  `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`
- Mesen 2 for regenerating the ignored hardware-state captures

CMake 3.16+ is optional. `build.ps1` and CMake consume the same
`nba95_sources.txt` manifest.

## Reproduce, build, and test

Enable script file I/O in Mesen, then create the ignored `.analysis` captures:

```powershell
.\tools\capture_assets.ps1 -RomPath '<path-to-rom>' -MesenPath '<path-to-Mesen.exe>'
```

The corrected Rules entrance additionally requires a fresh isolated capture:

```powershell
.\tools\capture_setup_transition_exact.ps1 -RomPath '<path-to-rom>' `
  -OutputRoot '.analysis/setup_rules_exact' -SimulationThreeMinute
```

That output directory must be new. Preserve earlier evidence when recapturing;
use another new directory and set `NBA95_RULES_OPEN_CAPTURE` to its absolute
path for extraction. The recorded raw PPU resources are production inputs;
RGB frames remain comparison evidence and are not packed.

The extractor also requires the original indexed HUD capture under
`.analysis/gameplay-hud`, or the explicit `NBA95_HUD_NATIVE_CAPTURE` path.
See [HUD asset reproduction](docs/gameplay-hud-assets.md) for the capture
command and an upgrade that preserves every resource in an existing pack.

Press **F9** for the ROM-backed Player Lab. Left/Right cycles all teams and
Up/Down cycles the selected team's 12-player roster; see
[`docs/player-lab.md`](docs/player-lab.md) for asset and Ghidra provenance.

Extract a fresh pack, compile at MSVC `/W4`, and run every regression:

```powershell
.\build.ps1 -RomPath '<path-to-rom>' -ExtractAssets -Test
```

Run the GUI:

```powershell
.\build.ps1 -RomPath '<path-to-rom>' -Run
```

Use the regular `nba-live-95-c-port` checkout on `main`; its executable and
asset pack are `build/nba95_port.exe` and `build/nba95_assets.pak`. Headless
smoke tests can jump straight to a configured matchup and capture frames
inside the renderer:

```powershell
.\build\nba95_port.exe --headless --rom '<path-to-rom>' `
  --assets 'build/nba95_assets.pak' --tipoff-only `
  --tipoff-home-team 17 --tipoff-away-team 3 --frames 90 `
  --dump-frame 'build/new-york-tipoff.bmp'
```

The [court layout correction and frame smoke](docs/court-logo-complaint-audit.md)
documents the original-game source evidence, scripted button route, all 29
home-team sweep and per-frame floor/logo checks. Capture output can stay under
`build`; `.analysis` is only needed when reading older raw reference inputs.

To capture a complete dribble immediately from a configured CPU game:

```powershell
.\tools\run_dribble_smoke.ps1 -RomPath '<path-to-rom>' -TipoffOnly
```

This builds the regular executable, checks native Mesen physics and draw
vectors, and saves every frame, contact sheets and a slow-motion GIF under
`build/dribble-smoke-*`. Add `-NoBuild` for another quick run. Omit
`-TipoffOnly` to exercise the scripted menu buttons as well. The
[dribble animation audit](docs/dribble-animation.md) records the Ghidra,
generated recomp C and native frame evidence.

The tests lock intro/title/Setup/Team Select/Player Setup pixels, both title-exit paths, all Setup cursor
rows, malformed-pack handling, ROM identity, 59.94/60 Hz equivalence, robust
runtime-PCM fingerprints, all 29 currently exposed ROM-derived logos/ranking records, independent
left/right team cycling, and focused SPC700/S-DSP vectors.

## Controls

| SNES button | Keyboard |
|---|---|
| **D-Pad Up** | Up Arrow / W |
| **D-Pad Down** | Down Arrow / S |
| **D-Pad Left** | Left Arrow / A |
| **D-Pad Right** | Right Arrow / D |
| **Button A** | X / K |
| **Button B** | Z / J |
| **Button X** | V / I |
| **Button Y** | C / U |
| **L Trigger** | Q |
| **R Trigger** | E |
| **Start** | Enter |
| **Select** | Space / Shift |

F8 opens the gameplay telemetry lab after tip-off, F10 toggles the live state
HUD, F11 opens the audio debugger, and F12 opens the ROM
asset browser for pack metadata, CGRAM palettes, paged SNES VRAM tiles, and
screen-positioned OAM/OBJ reconstruction.
Team logo assets also render directly in F12 as their decoded ROM OBJ pixels.

Gameplay Lab marks all ten actor slots and exposes actor/ball physics,
controller ownership, camera, collision, possession, animation, and raw CPU/AI
state. It supports pause and single-frame stepping; see
[`docs/gameplay-debugging.md`](docs/gameplay-debugging.md). Remaining gameplay
work is tracked in [`docs/gameplay-pending.md`](docs/gameplay-pending.md).

F10 cycles through off, a compact scene/timing/input/audio overview, and a
compact scene-detail page. Together they report Setup page/row/transition and
choices plus PPU brightness/scroll state without covering most of the screen.
Headless runs expose the complete snapshot with
`--debug-state` or periodically with `--debug-every N`; `--timing-debug` draws
the overview into `--dump-frame`, while `--debug-hud-page 1|2` selects either
compact page explicitly.

On Game Setup, Left/Right changes Mode, Style, Level, and Quarter length using
the original game cycles and menu sounds. These choices belong to the running
game session rather than the Setup screen, so scene re-entry preserves them.

On Team Select, Left/Right walks teams alphabetically while a team name is
selected. Up/Down moves between the active team name and Scoring, Rebounds,
Ball Control, Defense, or Overall; Left/Right then walks that ranking order.
A/B/X/Y/L/R toggles the active matchup side; a selected name follows the new
side while a selected ranking remains selected, exactly like `$82:83BC`.
Both selected teams persist in the running session.
Enter confirms Exhibition on Game Setup and keeps the Setup SPC music running
through Team Select and Player Setup. Enter on settled Team Select opens Player
Setup; Left/Right assigns Player 1 to the visitor/home side.

## Status and progress

[`STATUS.md`](STATUS.md) records the measurement methodology and milestone
baselines; `python tools/progress.py --write docs/progress.md` regenerates
the live numbers from Mesen exec coverage, `src/` provenance comments, the
verified-routine ledger, and the recomp function set.

[`docs/parity-gap-report.md`](docs/parity-gap-report.md) is the current honest
gap inventory. [`docs/human-free-throw-differential.md`](docs/human-free-throw-differential.md)
documents the latest native-input gameplay slice and its explicit exclusions.

## Technical notes

- [Title visuals and audio](docs/post-ea-title-audio.md)
- [Title-to-Setup transition](docs/title-to-setup-transition.md)
- [Game Setup rendering](docs/game-setup-screen.md)
- [Game Setup audio](docs/game-setup-audio.md)
- [Team Select routines, data, and controls](docs/team-select.md)
- [Player Setup routines, assets, transition, and controls](docs/player-setup.md)
- [Reverse-engineering tools](tools/README.md)
- [Live and CLI debugging](docs/debugging.md)
- [Runtime architecture](docs/architecture.md)

Remaining fidelity work is documented in the subsystem notes. Setup's CPU-side
music decisions currently use a cycle-timed control trace rather than a direct
C port of the 65816 sequencer.
