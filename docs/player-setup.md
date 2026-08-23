# Player Setup

Player Setup is entered by pressing Start on the settled Exhibition Team Select
screen. The selected visitor and home IDs remain in `NbaSession`; the new scene
uses those IDs to fetch both 48x56 team-logo assets from the asset pack. The
screen is not a captured Mesen image. Its background, tile maps, palettes, and
objects are reconstructed from packed ROM-derived VRAM, CGRAM, and OAM state,
with the same reusable logo assets as Team Select.

## ROM control flow

Live Mesen execution and the headless Ghidra dump establish this path:

| Address | Role |
|---|---|
| `$82:8553` | Team Select Start confirmation and selected-team commit |
| `$81:C41E` | outgoing fade/forced-blank setup |
| `$80:E95B` | shared transition-script interpreter, invoked with `$81:B901` |
| `$81:A489` | Player Setup scene dispatcher |
| `$81:B404` | patches Player Setup object-pool markers during construction |
| `$81:B493` | positions one controller assignment relative to its team side |
| `$81:B546` | five-frame selected-panel CGRAM animation |
| `$81:B62C` | synchronizes ownership and rebuilds assignment objects |
| `$81:B719` | object-builder call site inside that rebuild |
| `$81:B7C1` | Player Setup vertical-scroll IRQ handler |

The reproducible trace reaches Team Select confirmation at frame 1650, the
Player Setup dispatcher at 1701, object construction at 1795, and the vertical
scroll handler at 1816. The reference screen is settled by frame 1850. The C
handoff therefore preserves the measured 200-frame cadence: outgoing fade,
forced black, opposed background slide, title/label reveal, and final object
release.

The selected gold plate uses the existing 26-byte Team Select palette-cycle
asset. Moving Player 1 left or right moves the entire arrow/controller OAM group
and swaps the gold/silver plate palettes; it does not retain stale object pieces.

## Reproduction and tests

`tools/mesen_gameplay_player_capture.lua` records the live routine hits and raw
PPU memories. `tools/ghidra/Run-PlayerSetupAnalysis.ps1` regenerates the labeled
bank listings and decompilation notes. `tools/test_player_setup.py` locks the
asset hashes, OAM geometry, transition checkpoints, selected-team persistence,
and left-side assignment rendering.

For a focused headless render:

```powershell
.\build\nba95_port.exe --rom '<path-to-rom>' --assets build\nba95_assets.pak `
  --player-setup-only --frames 225 --dump-frame player_setup.bmp
```
