# Game Setup screen — ROM facts

Everything here was measured from the running ROM (Mesen) or disassembled from
it (Ghidra headless). Nothing is inferred. The addresses the previous pass used
(`$80:DB37`, `$80:E01E`, `$80:DD36`, `$80:DD50`, `$82:809A`, `$82:91EC`,
`$87:8C6B`) do **not** execute on this screen and should not be trusted.

## Reproducing the measurements

Mesen's Lua sandbox blocks file I/O by default. Set
`Debug.ScriptWindow.AllowIoOsAccess = true` in `Mesen2/settings.json`
(a backup of the original is at `settings.json.bak-nba95`).

```bash
"C:\Users\joshs\AppData\Local\Microsoft\WinGet\Packages\SourMesen.Mesen2_Microsoft.Winget.Source_8wekyb3d8bbwe\Mesen.exe" "F:\Games\SNES\NBA Live 95 (USA).sfc" tools\mesen_setup_capture.lua
```

| script | produces |
|---|---|
| `tools/mesen_setup_capture.lua` | `vram.bin`, `cgram.bin`, `oam.bin`, `wram.bin`, `spcram.bin`, `ppu_state.txt`, screenshots |
| `tools/mesen_setup_dma.lua` | `setup_exec_addrs.txt` — every address executed on the live screen |
| `tools/mesen_decomp_trace.lua` | `decomp_trace.txt` — decompressor calls + every VRAM/CGRAM DMA |
| `tools/mesen_scroll_log.lua` | `scroll_log.txt` — per-frame BG scroll / brightness |
| `tools/ghidra/Run-GameSetupAnalysis.ps1` | listings + decompilation for the traced ranges |
| `tools/build_setup_screen.py` | replays the ROM decompressor offline; `--verify` diffs against the capture |

Mesen API notes that cost time to discover:

- `emu.setInput(inputTable, port)` — **table first**, not `(port, table)`.
- memory types are `snesVideoRam`, `snesCgRam`, `snesWorkRam`, `spcRam`,
  `snesSpriteRam` (not `snesVram` / `snesCgram` / `snesSpcRam`; a wrong name
  silently falls back to `snesMemory` and yields plausible-looking garbage).
- `emu.getState()` returns a **flat** table keyed by dotted paths, e.g.
  `st["ppu.layers[1].vscroll"]`. `st.ppu.layers` is nil.
- Always `f:flush()` — Mesen does not always exit on `emu.stop(0)`, and a
  killed process loses buffered writes.

## Routines that actually run (live exec trace)

Bank `$80` unless noted. Full list in `setup_exec_addrs.txt`.

| address | role |
|---|---|
| `$80:A2BF` | screen build / layer + scroll setup |
| `$80:A3B8` | per-frame update driving the backdrop scroll |
| `$80:A62D` | option row state |
| `$80:A77C` | option value dispatch |
| `$80:A9E3`, `$80:AA7B`, `$80:AACD` | APU ports `$2140`–`$2143` (music/SFX commands) |
| `$80:CB8F` | DMA/transfer helper |
| `$80:C62B` | ROM decompressor |
| `$81:F9F1` | HDMA table setup (writes `$420C` at `$81:FA72`/`$81:FA7E`) |

## PPU configuration

BG Mode 1. VRAM byte offsets (Mesen reports word addresses):

| layer | tilemap | chr | size | content |
|---|---|---|---|---|
| BG1 | `$1800` | `$6000` | 64×32, 4bpp | "Game Setup" banner |
| BG2 | `$1000` | `$2000` | 64×32, 4bpp | blue gradient, NBA watermark, EA SPORTS |
| BG3 | `$0000` | `$8000` | 32×64, 2bpp | menu text |

Settled state: main screen `$17` (BG1+BG2+BG3+OBJ), sub screen `$04` (BG3),
colour math enabled for BG3 in subtract mode, brightness 15. All sprites are
parked at y=225 (off screen), so OBJ contributes nothing.

## Scroll behaviour (measured per frame)

Entrance, 32 frames:

- BG1 hscroll `768 → 512`, 8 px/frame
- BG2 hscroll `768 → 0` (wraps), 8 px/frame
- brightness `1 → 15`, one step per frame
- main screen is `$03` during the slide, `$17` once settled

Steady state:

- BG1 `512 / 1023` fixed
- BG2 hscroll `0`, **vscroll +1 every 3 frames** (0.3333 px/frame, no drift
  over 210 frames) — this is the scrolling backdrop
- BG3 `0 / 0` fixed

## Asset pipeline

The ROM decompresses through `$80:C62B` into WRAM `$7F`, then DMAs to VRAM.
Pointers captured live:

| source | destination |
|---|---|
| `$AE:A0AF`, `$AE:C446`, `$A6:C5FC` | BG2 chr |
| `$AE:D153` | BG2 tilemap |
| `$AF:97AA`, `$AC:FD74` | BG1 chr/tilemap |
| `$97:FF6D`, `$AF:F2DC` | BG3 text canvas + glyphs |
| `$AE:FA10` (CGADD `$30`, `$A0` bytes) | palettes 3–7 |
| `$A8:FFF1` (CGADD `$49`, `$0E`) | palette 4 tail |
| `$AF:9072` (CGADD `$59`, `$0E`) | palette 5 tail |

`build_setup_screen.py` reproduces **BG2 chr byte-for-byte** from the ROM.
The other regions are finished by CPU writes whose results depend on
accumulated decompressor state, so the packed VRAM/CGRAM image is taken from
the Mesen state capture. Both are ROM data; neither is committed (`.analysis/`
and `*.bin` are ignored).

Menu text is **not** a tilemap font. BG3 is a canvas of sequentially numbered
tiles and the ROM renders glyphs into the tile data itself, which is why
changing an option value requires the ROM's glyph routine rather than a
character-to-tile table.

## Still outstanding

1. **Selected-row highlight.** The ROM draws the active row gold
   (`255,165,49`); every other row is white. It is not OBJ (sprites are off
   screen), not a tilemap palette (all BG3 entries use palette 0), and BG3
   palette 0 holds no gold. It is therefore a per-scanline palette change
   driven by the HDMA tables set up at `$81:F9F1`. Capturing those tables is
   the next step.
2. **Colour math / gradient.** The subtract-mode colour math with BG3 as
   subscreen is not implemented; the right side of the backdrop is the largest
   remaining pixel difference against the ROM.
3. **Option values.** Changing Mode/Style/Level/Quarter needs the ROM glyph
   renderer that writes into the BG3 tile canvas.
4. **Music.** The screen drives the SPC through `$2140`–`$2143` at `$80:A9E3`,
   `$80:AA7B` and `$80:AACD`. `spcram.bin` holds the live SPC image. The port
   should sequence those samples rather than play a captured WAV.

Current accuracy against the ROM frame: **67.9 % of pixels identical**, with
the differences concentrated in the gradient and the missing gold row.

## Title screen exit (Start during the build)

`$80:E5C7` is the routine that runs when the title is dismissed. It branches on
bit 7 of `$0A4C`, the "build already finished" flag:

```
80:E5C7  LDA !$0A4C
80:E5CA  BIT #$0080
80:E5CD  BNE $E5D9        ; already complete -> skip the snap
80:E5CF  JSL $80:F07E     ; snap the title to its finished state
80:E5D3  LDA #$0078       ; ...and hold 120 frames
80:E5D6  PHA
80:E5D7  BRA $E5DD
80:E5D9  LDA #$0028       ; already complete -> hold 40 frames
80:E5DC  PHA
80:E5DD  ...              ; load palette $80:E7D1 through $80:8A02
80:E5F9  JSL $80:86B0     ; wait one frame
80:E5FD  DEC A
80:E5FE  BPL $E5F9        ; runs count+1 times
80:E600  JSL $80:CF1B     ; fade out, then hand off to the next scene
```

`$80:F07E` does the snap by DMAing the finished title tilemap — 0x680 bytes
from `$7F:4006` — into VRAM in a single transfer, so the remaining pieces
appear at once instead of continuing to animate in.

`$80:CF1B` is the fade: `DEC $0562` once per frame until the brightness level
reaches zero.

These addresses came from a differential exec trace — the title screen traced
once with Start pressed and once without (`tools/mesen_title_trace.lua`); the
listed ranges are the ones that only execute on the pressed run.

### Measured against the ROM

| | ROM | port |
|---|---|---|
| press mid-build -> fade begins | 124 frames | 124 frames |
| press after build -> fade begins | 44 frames | 44 frames |
| snap latency | ~4 frames | 2 frames |
| fade length | 15 INIDISP steps | 15 steps |

The 124 and 44 figures are `#$0078`/`#$0028` plus one, because `DEC A / BPL`
runs the wait loop count+1 times, plus the ROM's input-detection latency.

**A second press during the hold does nothing.** Verified directly: pressing
Start at frame 1450 and again at 1500 produces a fade at exactly the same
frame as the single press. The hold is a fixed count, so the transition is
snap -> fixed hold -> fade, not snap -> wait for a second press.

The port needs one derived constant the ROM does not have. Its title is driven
by a reference frame stream rather than a tilemap, so the snap seeks that
stream to the point where the build has finished:
`NBA_TITLE_BUILD_COMPLETE_FRAMES = 965`, measured from the port's own stream
(the title scene starts at frame 649 and the build completes at 1614).
