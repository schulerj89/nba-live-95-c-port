# Gameplay PPU pixel-parity plan

## Contract

Gameplay rendering is verified in two independent stages:

1. Mesen runs the original ROM under the deterministic CPU-vs-CPU input route.
   The capture records the completed 256x224 scanout plus the VRAM, CGRAM,
   OAM, PPU registers, and raster-time `$212C` writes that produced it.
2. `snes_ppu_oracle.py` renders those raw bytes independently. The production
   C implementation renders the same snapshot through
   `nba_snes_mode1_render_snapshot`. Both outputs must match each other and the
   Mesen scanout at every pixel.

Captured pixels are comparison evidence only. They are never imported into the
asset pack or used as artwork.

Run the end-to-end capture and comparison with:

```powershell
.\tools\verify_ppu_parity.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc'
```

The report records whole-frame parity and separate witnesses for the right
goal and out-of-bounds/sideline regions.

## Audit finding

An end-of-frame snapshot initially differed from Mesen by exactly 1,210 pixels,
all inside the right backboard/support rectangle `(182,10)-(255,68)`. The court,
sideline, crowd, HUD, player, rim, and net pixels outside that rectangle were
already exact.

The mismatch was not a tile decoder, palette, OAM, or priority-ladder bug.
The ROM writes `$212C = $17` during the post-scanout upload and then calls
`$85:EF37`, which writes `$212C = $16` at scanline 123. It therefore enables
BG1 for the upper raster and disables it for the lower raster. In addition,
the ordinary end-frame VRAM dump may already contain the next upload. Capturing
at `$80:8188` preserves the bytes used by the completed scanout. Replaying that
memory with the scanline TM schedule produces a 57,344/57,344 match.

## Implementation phases

| Phase | Purpose | Gate |
|---|---|---|
| Raw PPU replay | BG1/BG2/BG3 tilemaps, CGRAM, OAM, windows, brightness, Mode-1 priority | C equals independent oracle |
| Raster state | Apply main-screen layer selection per scanline | C equals Mesen for goal and boundary witnesses |
| Runtime input parity | Replace guessed goal/court submissions with asset-pack tiles and the ROM-derived presentation state | Port trace has the same layer/source decisions as native |
| Camera-aligned frames | Match native and port by camera/actor state rather than arbitrary frame number | First divergent pixel and source are reported |
| Regression set | Left basket, right basket, center court, both sidelines, HUD overlap, moving players | All witness frames pass |

The first two phases are wired and passing. The remaining visual discrepancy is
now isolated to the runtime's inputs: it still submits a predecoded panorama and
separate guessed goal layers instead of replaying the exact asset-pack tilemap,
window, and raster state. That is the next correction; the PPU compositor itself
is no longer an unverified variable.
