# Game Setup screen — ROM facts

Everything here was measured from the running ROM (Mesen) or disassembled from
it (Ghidra headless). Nothing is inferred. The addresses the previous pass used
(`$80:DB37`, `$80:E01E`, `$80:DD36`, `$80:DD50`, `$82:809A`, `$82:91EC`,
`$87:8C6B`) do **not** execute on this screen and should not be trusted.

## Reproducing the measurements

Mesen's Lua sandbox blocks file I/O by default. Set
`Debug.ScriptWindow.AllowIoOsAccess = true` in `Mesen2/settings.json`, then use
the repository capture wrapper:

```powershell
.\tools\capture_assets.ps1 -RomPath '<path-to-rom>' -MesenPath '<path-to-Mesen.exe>'
```

| script | produces |
|---|---|
| `tools/mesen_setup_capture.lua` | `vram.bin`, `cgram.bin`, `oam.bin`, `wram.bin`, `spcram.bin`, `ppu_state.txt`, screenshots |
| `tools/mesen_setup_dma.lua` | `setup_exec_addrs.txt` — every address executed on the live screen |
| `tools/mesen_decomp_trace.lua` | `decomp_trace.txt` — decompressor calls + every VRAM/CGRAM DMA |
| `tools/mesen_scroll_log.lua` | `scroll_log.txt` — per-frame BG scroll / brightness |
| `tools/ghidra/Run-GameSetupAnalysis.ps1` | listings + decompilation for the traced ranges |
| `tools/build_setup_screen.py` | replays the ROM decompressor offline; `--verify` diffs against the capture |
| `tools/mesen_hdma_dump.lua` | HDMA channel config on the live screen |
| `tools/mesen_hdma_window.lua` | walks HDMA ch7's window table |
| `tools/mesen_wram_full.lua` | full 128 KiB WRAM dump (banks $7E and $7F) |
| `tools/mesen_row_bands.lua` | screenshots the cursor on each row |
| `tools/mesen_title_trace.lua` | differential exec trace of the title screen |

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

Handoff/loading (the capture now uses routine-relative frames; the historical
global frame numbers are included only to relate the original trace):

- `$80:E600` enters the 15-step fade; the brightness-1 frame is capture frame 0
- the following 105 frames are forced blank while the next scene is built
- `$80:A2BF` builds the Setup layers; forced blank then releases at brightness
  1 with BG1/BG2 scroll 768

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

BG3 is deliberately delayed after the 32-frame BG1/BG2 slide. Its vertical
scroll changes `280 -> 252 -> ... -> 14 -> 0` across frames 1782-1801 while
`$212C/$212D` stage the text layer. Releasing the final VRAM image immediately
was the source of the port's glitchy transition.

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

## Selected-row highlight (solved)

The active row is gold and the rest white, done with colour math rather than a
second palette:

- `$2131 CGADSUB` — colour math enabled for BG3 only, subtract mode
- `$2130 CGWSEL` — use the fixed colour, not the subscreen
- `$2132 COLDATA` — `25952` = R 0, G 11, B 25 in 5-bit channels
- window 1 gates the colour window (`ppu.window[0].activeLayers[5]=true`)
- **HDMA channel 7** rewrites `$2126`/`$2127` (window 1 left/right) per
  scanline from a table at `$7F:6800`, opening the window only over the
  active row

Subtracting (0,11,25) from white (31,31,31) gives (31,20,6) = RGB
(255,165,49); from the grey (22,22,22) it gives (22,11,0) = (181,90,0). Both
match the ROM's pixels exactly.

The HDMA table decodes to a 16-scanline band over the selected row and a
closed window everywhere else. Band tops for cursor rows 0–5: 70, 88, 106,
124, 156, 174 — an 18px pitch with an extra 14px gap before "Set Rules".
All six bands were verified against ROM captures and match exactly.

Note `$420C` is **write-only**: reading it returns open bus, which reads as
`00` and makes HDMA look disabled when it is running. Read the channel
registers at `$4300`–`$437F` instead, and watch the current-table pointer at
`$43x8`/`$43x9` advance to confirm a channel is live.

Two PPU details were needed to land this pixel-exactly:

- 5-bit to 8-bit colour is `(v << 3) | (v >> 2)`, not `v * 255 / 31`. The
  naive form is off by one on several levels and cost ~25% of the pixel match.
- Vertical scroll is offset by one — the first displayed scanline shows
  tilemap line `vscroll + 1`.

## Rendering status

1. **Option values.** Changing Mode/Style/Level/Quarter needs the ROM glyph
   renderer that writes into the BG3 tile canvas.

The captured settled reference frame is **100% pixel-identical** (0 of 57,344
pixels differ), including the gold highlight. Transition and cursor-row hashes
are enforced by `tools/test_setup_transition.py`.
