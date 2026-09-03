# Project status

The full game remains incomplete. The current playable path covers the
Nintendo license, legal and EA intro screens, title, Game Setup, Exhibition
Team Select, Player Setup, introductions, tipoff, and sustained CPU-versus-CPU
gameplay.

## Current runtime

Implemented production behavior includes court and camera movement, ten-player
rendering, possession, passing, shooting, dribbling, rebounds, fouls, free
throws, clock and period transitions, timeout/resume support, out-of-bounds
presentation, and a structural final-game flow. Graphics and audio are derived
from the verified US ROM through the asset pack.

Recent source-verified visual and gameplay fixes include:

- all 29 selected home-team court layouts and center logos;
- native dribble animation timing and ball attachment;
- north and south basket raster clipping;
- the original out-of-bounds violation and possession overlay;
- native CPU role-rebuild reaction delays, flag clearing, and RNG order.

The associated evidence and frame captures are documented in
[center-court layout](docs/court-logo-complaint-audit.md),
[dribbling](docs/dribble-animation.md), [basket raster](docs/hoop-raster.md),
[out-of-bounds HUD](docs/out-of-bounds-hud.md), and
[CPU reaction reload](docs/cpu-role-reaction-reload.md).

## Major remaining gaps

- Ordinary human offense, defense, passing, shooting, inbound control, and
  controller ownership are not complete production paths. A few bounded human
  helpers exist but do not make a normal matchup fully playable.
- The strict native/C gameplay differential still begins from different launch
  state, scheduler timing, and RNG history. Passing isolated routine vectors
  does not establish an equivalent whole-game trajectory.
- Season, playoffs, schedules, records, save/load, complete postgame statistics,
  and several non-Exhibition flows remain incomplete.
- Some Setup rule and option values have no complete gameplay consumer.
- Complete native frame and audio timing remains open for several transitions,
  menus, breaks, substitutions, and end-game scenes.

The maintained gap inventory is [docs/parity-gap-report.md](docs/parity-gap-report.md).
Focused gameplay work is tracked in
[docs/gameplay-pending.md](docs/gameplay-pending.md).

## Evidence accounting

Current generated captured-address measurements are:

| metric | address positions | captured-address percentage |
|---|---:|---:|
| observed in retained execution captures | 29,438 | 100.0% |
| documented by source provenance | 29,101 | 98.9% |
| inside evidence-eligible verified ranges | 11,537 | 39.2% |

These are coverage measurements for retained captures. They are not a
percentage of the ROM, retail features, or game completion. The generated
[progress report](docs/progress.md),
[full-ROM instruction census](docs/full-rom-instruction-census.md),
[feature matrix](docs/feature-capture-matrix.md), and
[verified routine ledger](docs/verified-routines.json) are the authoritative
sources for their respective metrics.

## Build and verification

Build the regular executable with:

```powershell
.\build.ps1
```

Run the complete configured regression suite with the verified ROM and asset
pack:

```powershell
.\build.ps1 -RomPath '<path-to-rom>' -AssetPack 'build/nba95_assets.pak' -Test
```

Focused headless smoke tests press real buttons, capture frames inside the
renderer, and retain their output under ignored `build/` directories. Native
Mesen captures and other private reference material remain under ignored
`.analysis/` directories. See the [documentation index](docs/README.md) for
current subsystem and reproduction guides.
