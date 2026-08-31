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
| `$81:C41E` | outgoing layer withdrawal/forced-blank setup |
| `$80:EBF9-$80:EC53` | sixteen-step foreground withdrawal (`$1775 -= $000E`, `$0617 += $000E`) |
| `$80:EB27-$80:EB7B` | opposed eight-pixel background motion and brightness stepping |
| `$80:E95B` | shared transition-script interpreter, invoked with `$81:B901` |
| `$81:A489` | Player Setup scene dispatcher |
| `$81:B404` | patches Player Setup object-pool markers during construction |
| `$81:B493` | positions one controller assignment relative to its team side |
| `$81:B546` | five-frame selected-panel CGRAM animation |
| `$81:B62C` | synchronizes ownership and rebuilds assignment objects |
| `$81:B719` | object-builder call site inside that rebuild |
| `$81:B7C1` | Player Setup vertical-scroll IRQ handler |
| `$82:863C` | rebuilds the selected home graphics through `$80:D0E2`; its twenty-five wallpaper tiles remain at VRAM `$20A0-$23BF` |

The earlier targeted trace reaches Team Select confirmation at frame 1650, the
Player Setup dispatcher at 1701, object construction at 1795, and the vertical
scroll handler at 1816. A later normal-input, normal-power-on consecutive frame
capture pins the visible boundary more tightly: Start at setup frame 650,
outgoing layer motion at 651-672, brightness withdrawal at 673-700, forced
black at 701, and the first Player Setup reveal at 768. The C handoff composes
the ROM-derived Team Select layers through that 51-frame outgoing boundary;
it does not fade a frozen RGB screenshot. Its corresponding Start/reveal
frames are 178/296, with 67 fully-black frames at 229-295. It then preserves
the existing 200-frame destination cadence through opposed background slide,
title/label reveal, and final object release.

The selected gold plate uses the existing 26-byte Team Select palette-cycle
asset. Moving Player 1 left or right moves the entire arrow/controller OAM group
and swaps the gold/silver plate palettes; it does not retain stale object pieces.
The grayscale wallpaper is also team-dependent. The C scene copies the exact
twenty-five-tile block from the selected home team's raw Team Select VRAM asset,
matching the ROM's retained-VRAM handoff instead of baking in Orlando.

## Reproduction and tests

`tools/mesen_gameplay_player_capture.lua` records the live routine hits and raw
PPU memories. `tools/ghidra/Run-PlayerSetupAnalysis.ps1` regenerates the labeled
bank listings and decompilation notes. `tools/test_player_setup.py` locks the
asset hashes, OAM geometry, transition checkpoints, selected-team persistence,
and left-side assignment rendering. `tools/test_frontend_route.py` checks the
consecutive-frame exit, centered CPU-vs-CPU selection, and complete production
route through presentation skips to Tipoff.

For a focused headless render:

```powershell
.\build\nba95_port.exe --rom '<path-to-rom>' --assets build\nba95_assets.pak `
  --player-setup-only --frames 225 --dump-frame player_setup.bmp
```
