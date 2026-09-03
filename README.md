# NBA Live '95 native C port

An in-progress native C99 port of the US SNES release of NBA Live '95. The
current playable route covers the Nintendo license, legal and EA intro screens,
title, Game Setup, Exhibition Team Select, Player Setup, introductions, tipoff,
and sustained CPU-versus-CPU gameplay.

Ordinary human offense and defense, several retail modes, complete match
orchestration, and whole-game native/C trajectory parity remain incomplete.
[STATUS.md](STATUS.md) is the maintained statement of implemented behavior,
known gaps, and measured verification.

Graphics and audio come from a user-supplied ROM-derived asset pack. The title,
Game Setup, Player Introduction, and gameplay audio paths use original indexed
resources, SPC700 state, DSP state, and BRR samples rather than rendered video
or mixed music files.

## Source of truth

The repository describes its current state through:

- production code under src/ and public contracts under include/;
- ROM addresses, ownership boundaries, and caveats beside the implementation;
- executable regressions and native-vector verifiers under tools/ and tests/;
- [STATUS.md](STATUS.md) for current product scope and remaining gaps;
- generated coverage reports in [docs/progress.md](docs/progress.md) and
  [docs/full-rom-instruction-census.md](docs/full-rom-instruction-census.md);
- the machine-readable
  [verified-routine ledger](docs/verified-routines.json).

Historical plans, task reports, audits, and screenshot narratives remain
available through Git history and are not maintained as parallel descriptions
of the code.

## Requirements

- Windows 10/11 and Visual Studio 2022 with the Desktop C++ workload
- Python 3.10+ with pip install -r requirements.txt
- The verified unheadered US ROM with normalized SHA-256
  2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870
- Mesen 2 when regenerating ignored native captures

CMake 3.16+ is optional. build.ps1 and CMake consume the same
nba95_sources.txt manifest.

## Build, test, and run

Build the executable:

~~~powershell
./build.ps1
~~~

Extract a fresh asset pack and run the complete configured regression suite:

~~~powershell
./build.ps1 -RomPath '<path-to-rom>' -ExtractAssets -Test
~~~

Run the GUI:

~~~powershell
./build.ps1 -RomPath '<path-to-rom>' -Run
~~~

The regular outputs are build/nba95_port.exe and build/nba95_assets.pak.
Capture and test artifacts stay under ignored build/ or .analysis/
directories.

## Headless visual smoke tests

A configured tipoff can start without replaying the frontend:

~~~powershell
./build/nba95_port.exe --headless --rom '<path-to-rom>' `
  --assets 'build/nba95_assets.pak' --tipoff-only `
  --tipoff-home-team 17 --tipoff-away-team 3 --frames 90 `
  --dump-frame 'build/new-york-tipoff.bmp'
~~~

Focused smoke tests press buttons or configure gameplay state, capture every
frame inside the renderer, and validate the resulting sequence:

~~~powershell
./tools/run_dribble_smoke.ps1 -RomPath '<path-to-rom>' -TipoffOnly
./tools/run_hoop_smoke.ps1 -RomPath '<path-to-rom>'
./tools/run_oob_smoke.ps1 -RomPath '<path-to-rom>'
python tools/test_cpu_reaction_smoke.py --rom '<path-to-rom>'
~~~

Use python tools/run_visible_smoke_checkpoints.py --help for the broader
frontend and gameplay capture matrix. See [tools/README.md](tools/README.md)
for current capture, replay, and census entry points.

## Controls

| SNES button | Keyboard |
|---|---|
| D-Pad Up | Up Arrow / W |
| D-Pad Down | Down Arrow / S |
| D-Pad Left | Left Arrow / A |
| D-Pad Right | Right Arrow / D |
| Button A | X / K |
| Button B | Z / J |
| Button X | V / I |
| Button Y | C / U |
| L Trigger | Q |
| R Trigger | E |
| Start | Enter |
| Select | Space / Shift |

F8 opens gameplay telemetry, F9 opens the ROM-backed Player Lab, F10 cycles
the live state HUD, F11 opens the audio debugger, and F12 opens the ROM asset
browser. Gameplay telemetry supports pause and single-frame stepping. Headless
runs expose the same state with --debug-state, --debug-every N,
--timing-debug, and --debug-hud-page 1|2.

On Game Setup, Left/Right changes Mode, Style, Level, and Quarter length. On
Team Select, Up/Down chooses the team name or a ranking category, Left/Right
cycles the selected order, and A/B/X/Y/L/R changes the active matchup side.
Enter confirms each settled frontend screen.
