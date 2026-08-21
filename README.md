# NBA Live '95 native C port

An in-progress native C99 port of the US SNES release of NBA Live '95. The
current playable path covers the Nintendo license, NBA legal screen, EA SPORTS
intro, animated title, Game Setup, and Exhibition Team Select.

Graphics and audio come from a user-supplied ROM-derived asset pack. The title
and Game Setup music run the original SPC700 driver and BRR samples; the pack
does not contain rendered title video or mixed title/Setup songs.

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

Extract a fresh pack, compile at MSVC `/W4`, and run every regression:

```powershell
.\build.ps1 -RomPath '<path-to-rom>' -ExtractAssets -Test
```

Run the GUI:

```powershell
.\build.ps1 -RomPath '<path-to-rom>' -Run
```

The tests lock intro/title/Setup/Team Select pixels, both title-exit paths, all Setup cursor
rows, malformed-pack handling, ROM identity, 59.94/60 Hz equivalence, robust
runtime-PCM fingerprints, all 27 ROM-derived logos/ranking records, independent
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

F10 toggles the live state HUD, F11 opens the audio debugger, and F12 opens the ROM
asset browser for pack metadata, CGRAM palettes, paged SNES VRAM tiles, and
screen-positioned OAM/OBJ reconstruction.
Team logo assets also render directly in F12 as their decoded ROM OBJ pixels.

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

On Team Select, Up/Down chooses Scoring, Rebounds, Ball Control, Defense, or
Overall. Left/Right walks teams in that ranking order. A/B/X/Y/L/R toggles the
active matchup side exactly like `$82:83BC`; both selected teams persist in the
running session. Enter confirms Exhibition on Game Setup and keeps the Setup
SPC music running through the handoff.

## Technical notes

- [Title visuals and audio](docs/post-ea-title-audio.md)
- [Title-to-Setup transition](docs/title-to-setup-transition.md)
- [Game Setup rendering](docs/game-setup-screen.md)
- [Game Setup audio](docs/game-setup-audio.md)
- [Team Select routines, data, and controls](docs/team-select.md)
- [Reverse-engineering tools](tools/README.md)
- [Live and CLI debugging](docs/debugging.md)
- [Runtime architecture](docs/architecture.md)

Remaining fidelity work is documented in the subsystem notes. Setup's CPU-side
music decisions currently use a cycle-timed control trace rather than a direct
C port of the 65816 sequencer.
