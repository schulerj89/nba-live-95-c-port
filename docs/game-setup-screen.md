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

Settled main-Setup state is main screen `$17`, sub screen `$04`. Main Setup's
sprites are parked at y=225, but Rules/Options populate OAM with both volume
bars and the Rules viewport arrow. Their OBJ CHR uses OBSEL word base `$6000`
(byte offset `$C000` in the packed VRAM image).

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

The captured settled reference frame is **100% pixel-identical** (0 of 57,344
pixels differ), including the gold highlight. Transition and cursor-row hashes
are enforced by `tools/test_setup_transition.py`.

## Main Setup values

The four value rows on the parent Game Setup page are live 16-bit working
values at `$7E:16FB`, `$7E:16FD`, `$7E:16FF`, and `$7E:1701`. Left/Right wraps
through the exact Mesen-observed cycles:

| Row | Values |
|---|---|
| Mode | Exhibition, Season, Playoffs, Load Series |
| Style | Arcade, Simulation, Custom |
| Level | Rookie, Starter, All-Star |
| Quarter | 3 Minutes, 5 Minutes, 8 Minutes, 12 Minutes |

`$80:9DEA` dispatches the input, `$80:A62D` selects the row state, and
`$80:A77C` selects the value passed to the proportional BG3 glyph writer.
Every accepted adjustment uses command `$49`/SRCN `$1A`, while row movement
uses `$4A`/SRCN `$1B`. The port stores these values in the session-owned
`NbaGameConfig`, so they survive Rules/Options round trips and complete Setup
scene reinitialization. A on a main value row returns the separate
`NBA_SETUP_ACTION_CONFIRM_MODE` navigation event with confirm SRCN `$1C`; the
scene dispatcher uses the persistent Mode value to choose Exhibition team
selection, Season, Playoffs, or Load Series.

## Set Rules and Set Options

`tools/mesen_setup_menus_capture.lua` records both submenus after their slide,
including settled VRAM/CGRAM, controller-driven WRAM writes, CPU execution
ranges, mirrored APU ports, and DSP writes. `DumpGameSetup.java` applies the
same entry-point labels during the headless Ghidra pass.

| Menu path | ROM routines | Working and committed storage |
|---|---|---|
| Set Rules | `$81:D318` frame, `$81:D3B1` move, `$81:D446` decrement, `$81:D4C0` increment, `$81:D47A` common write, `$81:D59B` redraw, `$81:D675` draw value | edits at `$7E:16FB + row*2`; `$81:D491` sets state `$7E:17AD = 2`; Start at `$81:D516` copies 26 bytes to `$7E:17D1` |
| Set Options | `$82:8CD1` frame, `$82:8D3C` move, `$82:8DA6` decrement, `$82:8E73` increment, `$82:8DC6` common write, `$82:8F9C` redraw, `$82:9028` draw value | edits at `$7E:16FB + row*2`; Start through `$82:8CD9/$82:8D0A` copies 14 bytes to `$7E:17B5` |

Both pages wrap their cursor and their discrete values. The two bar rows clamp
at 0 and 45 (`$81:D4A9-$D4B9/$81:D4FA-$D508` and
`$82:8E43-$8E54/$82:8E97-$8EA5`). Start confirms and returns; B is
intentionally ignored by the original handlers. Rules has 13 entries and a
seven-row scrolling viewport. The port scrolls the captured 64-row BG3 canvas,
so hidden rule labels and glyph pixels remain ROM-authentic rather than being
redrawn with a host font.

Opening either submenu is a shared screen transition, not a direct page swap.
The complete Mesen `$2100/$212C/$212D/$210D-$2112` trace shows BG3 scrolling
out by 14 pixels/frame, followed by opposing BG1/BG2 slides at 8 pixels/frame
while brightness falls 15→1. The ROM holds forced blank while `$80:A2BF`
builds the target, then `$80:A3B8` runs the 32-frame entrance and delayed BG3
staging. The builders are page-specific: Set Rules finishes after 146
transition frames, Set Options after 132, and Start returns to Game Setup
after 132. Packed PPU traces preserve the VRAM writes that continue while the
new BG3 canvas becomes visible instead of swapping directly to a settled page.
The trace's opening BG2 vertical coordinates are capture-time absolute values,
not a command to reset the live backdrop. `$80:A3B8` carries the current BG2
phase through the visible exit; `$80:A2BF` resets it only after forced blank is
active, and the rebuilt phase continues one pixel every three frames after the
new page settles. The port therefore rebases only the visible trace prefix and
hands the final trace phase to the steady updater instead of recomputing it from
the lifetime Setup frame counter.
Mesen's `screenBrightness` field contains only the low four INIDISP bits, so
the port separately restores bit 7 using edge-specific measured windows:
Rules open 51–80, Options open 51–76, Rules return 36–62, and Options return
52–78 (transition-frame numbering). The Rules `$81:A28E` visual scanout
trails its recorded BG3 vertical-register sweep by one 14-pixel step; Options
and both return edges do not. These differences now live in the directed
transition profiles instead of a global transition shortcut.

The shared sound dispatch is `$80:9DF3`: command `$49` selects SRCN `$1A` for
a value adjustment, `$4A` selects SRCN `$1B` for cursor movement, and `$4B`
selects SRCN `$1C` for confirm/open. These are asset IDs 120–122 in the F11
BRR catalog. Runtime playback keys S-DSP voice 1 from packed Setup ARAM/DIR
with the captured SRCN, pitch, `$8E/$E0` ADSR, and volume registers; it does
not play the undecorated F11 preview WAV. Options calls `$87:8C2D` from
`$82:8DDC` for rows 0 and 1 to
apply slider changes immediately; the port applies Music Volume to the
still-running Setup music stream and applies SFX Volume to the independently
synthesized menu voice.

Asset-pack version 10 retains the version 9 page-specific Rules/Options open and return snapshots
with PPU write traces. Each transition frame now
stores the ROM's brightness, main/sub layer designation, scroll positions,
tilemap/CHR bases, and map dimensions. The builder temporarily repoints
BG1/BG2/BG3 while VRAM is incomplete. Mesen's end-frame callback observes
VRAM/CGRAM prepared for the following scanout, so the packer delays those
deltas one frame. The return profiles also retain the outgoing page snapshot
across their short map/CHR DMA guard. Rendering those construction bytes with
the new addresses was the source of the transient garbage. The pack also carries exact,
independent Music Mode, Crowd Sound, Slow Motion Dunks, Shot Control, and CPU
Assistance BG3 states. `$82:8F9C -> $81:9FD4 -> $81:A1EE` uploads the redrawn
`$0800`-byte BG3 canvas as a unit. The port now composes the ROM's row-local
2bpp VRAM deltas into one mutable canvas, so every value keeps its complete
foreground and shadow pixels and no value is sampled from another row. Rules
follows the parallel `$81:D59B -> $81:9FD4 -> $81:A28E` path and uses the same
ROM-authored OFF glyph source rather than the retired, incorrectly labelled
row-6 capture.
Bars/arrows are decoded from captured OAM and OBJ tiles;
the full Rules bar objects provide the game's shared dynamic bar tiles, while
the Options OAM capture remains a packed, debugger-visible default-position
oracle. These values have no host-font or hardcoded-pixel fallback, and a
submenu will not open from an incomplete pack.
`tools/test_setup_transition.py` hashes the assets and rendered pages, compares
bars/arrows to Mesen pixel oracles, locks the open/return transition stages and
all six Options value states, proves their BG3 deltas do not overlap, and
exercises clamp, wrap, ignored-B, live SFX gain, and
working-versus-committed behavior. It also fingerprints the exact WAV output,
pitch, envelope, and shape for all three menu SRCNs.

`$81:D59B-$81:D5AB` enables the Rules slider objects while each logical foul
row (`$1693` below 2) is redrawn. `$81:D5AE-$81:D5C3` independently derives
the object's Y position from its visible viewport slot through `$1665`.
Consequently the first scroll removes Defensive Fouls but shifts the still
visible Offensive Fouls meter into slot 0; the second scroll removes both.

Version 9 also introduced ten main-page BG3 value states captured after the ROM's
own writer produced Season, Playoffs, Load Series, Custom, Arcade, Starter,
All-Star, and the 5/8/12-minute quarter choices. Rendering copies only those
game-authored glyph pixels, allowing independent combinations without a host
font or screenshot composite. Main values and submenu values share one
clear-cell/copy-span renderer: the entire previous value cell is restored from
BG2, then only the replacement word's measured foreground-and-shadow span is
sampled. This mirrors the bounded proportional writer and prevents a captured
Exhibition or Simulation tail from being copied back after the clear.
