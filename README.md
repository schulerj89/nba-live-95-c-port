# NBA Live '95 native C port

An in-progress native C99 port of the US SNES release of NBA Live '95. The
current playable path covers the Nintendo license, NBA legal screen, EA SPORTS
intro, animated title, and Game Setup entrance/menu.

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

The tests lock intro/title/Setup pixels, both title-exit paths, all Setup cursor
rows, malformed-pack handling, ROM identity, 59.94/60 Hz equivalence, robust
runtime-PCM fingerprints, and focused SPC700/S-DSP vectors.

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

F10 toggles the timing HUD and F11 opens the audio debugger.

## Technical notes

- [Title visuals and audio](docs/post-ea-title-audio.md)
- [Title-to-Setup transition](docs/title-to-setup-transition.md)
- [Game Setup rendering](docs/game-setup-screen.md)
- [Game Setup audio](docs/game-setup-audio.md)
- [Reverse-engineering tools](tools/README.md)

Remaining fidelity work is documented in the subsystem notes. Setup option
editing still needs the ROM's BG3 glyph writer, and Setup's CPU-side music
decisions currently use a cycle-timed control trace rather than a direct C port
of the 65816 sequencer.
