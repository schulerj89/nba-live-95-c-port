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
the `$85:EF2E` handler, which writes `$212C = $16` at scanline 123 in that
camera view. The split follows `$087E`; it is not a fixed row. The
[basket raster correction](hoop-raster.md) adds moving views at both baskets
and replaces the mistakenly fixed runtime cutoff. In addition,
the ordinary end-frame VRAM dump may already contain the next upload. Capturing
at `$80:8188` preserves the bytes used by the completed scanout. Replaying that
memory with the scanline TM schedule produces a 57,344/57,344 match.

## Implementation phases

| Phase | Purpose | Gate |
|---|---|---|
| Raw PPU replay | BG1/BG2/BG3 tilemaps, CGRAM, OAM, windows, brightness, Mode-1 priority | C equals independent oracle |
| Raster state | Apply main-screen layer selection per scanline | C equals Mesen for goal and boundary witnesses |
| Runtime input parity | Replace guessed goal/court submissions with asset-pack tiles and the ROM-derived presentation state | **PASS:** 54,688 background + 182 goal OBJ pixels match native |
| Camera-aligned frames | Match native and port by camera/actor state rather than arbitrary frame number | **PASS:** settled frame 989 at camera `(135,-220)` |
| Regression set | Left basket, right basket, center court, both sidelines, HUD overlap, moving players | **IN PROGRESS:** 812 indexed runtime viewports pass; additional native moving frames remain |

The runtime now consumes pack v31's `NBPPUIN1` entry for all 29 home teams.
BG2 is sampled as indexed ROM-map/CHR data, color zero remains transparent,
BG3 and backdrop are submitted, and BG1 uses the native hardware window plus
the camera-dependent TM/WH3 raster schedule. The `$0822` rim/net resource uses native OBJ priority
3 and OAM witness slots 33/34. At camera `(135,-220)`, every non-player pixel
and every native goal OBJ pixel matches the original scanout. Remaining parity
work is player/ball OBJ state and additional moving-camera witness frames.
