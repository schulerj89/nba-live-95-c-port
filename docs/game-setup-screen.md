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

## Still outstanding

1. **Option values.** Changing Mode/Style/Level/Quarter needs the ROM glyph
   renderer that writes into the BG3 tile canvas.
2. **Music.** The screen drives the SPC through `$2140`–`$2143` at `$80:A9E3`,
   `$80:AA7B` and `$80:AACD`. `spcram.bin` holds the live SPC image. The port
   should sequence those samples rather than play a captured WAV.

Current accuracy against the ROM frame: **100 % of pixels identical** (0 of
57344 differing, max channel delta 0), with the gold highlight in place.

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

## Game Setup music (in progress — not yet playing)

The screen genuinely has a sequenced track. Polling the DSP every frame across
the settled screen (`tools/mesen_dsp_activity.lua`) shows **315 pitch/sample
changes and 233 note re-triggers over 700 frames**, across voices using sample
numbers 0, 4, 8, 13, 19, 20, 21, 23. It is not a held chord and not a stream.

The whole CPU-to-driver interface is four bytes. Capturing every 65816 write to
`$2140`-`$2143` (`tools/mesen_apu_ports.lua`) over the screen gives just **24
writes, all in one burst at frame 1637**: eight commands of the form
`port1 = N; port0 = $05; port0 = $00` for N = 7 down to 1. After that the CPU
says nothing and the driver sequences on its own — which is why an APU snapshot
is enough to resume the music without emulating the 65816 side.

### What is built

`src/nba_spc.c` is an SPC700 + S-DSP core that resumes the ROM's own driver
from a captured snapshot (`tools/mesen_spc_capture.lua` writes `spc_ram.bin`,
`spc_dsp.bin` and `spc_state.txt`). `tools/spc_render_main.c` renders it to a
WAV offline so it can be checked without the game loop:

```
build\spc_render.exe .analysis\setup_capture\spc_ram.bin .analysis\setup_capture\spc_dsp.bin 0x06B2 7 22 99 255 0 8 out.wav
```

Working so far:

- the SPC700 core executes the real driver, cycling through `$048B`, `$04DA`,
  `$044A`, `$0497`, `$044D`, `$0548`
- the DSP decodes BRR from the sample directory at `$0200` and produces clean
  tonal output (harmonics at 605/1210/1816 Hz, 97% of energy below 5 kHz)
- timers tick at the right rates — 1156 ticks/second across T0/T1/T2, matching
  500 + 432 + 182 Hz for targets `$10`/`$2C`/`$94`
- the driver polls the timer 38,340 times a second, so its wait loop is live

Two things had to be fixed to get that far. `$F4`-`$F7` are the CPU-facing
output latches: what the SPC reads back is whatever the 65816 last wrote, so
SPC writes must not change the read value. Without that the snapshot resumes
mid-handshake and the driver spins forever at `$0443`
(`MOV A,$F4` / `BNE $0443`). And a timer target of 0 means 256, not 0.

### What is wrong

The driver gets its tick but issues only **2 DSP writes per second**, so no
notes are keyed and the output is silent — or, before the port fix, one held
tone. The sequencer is not running its note-processing path after the tick,
which points at a CPU bug on that branch rather than anything in the DSP.

Next step is a differential trace: log the SPC700 PC and register file from
Mesen for a few thousand instructions from the snapshot point, run the same
from this core, and diff to find the first divergent instruction.

`nba_spc.c` is deliberately **not** in the game build until it plays correctly.
The port is silent on this screen rather than falling back to a recording.
