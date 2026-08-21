# Live and CLI debugging

F10 is a non-pausing, compact state HUD. Repeated presses cycle **off → overview
(1/2) → scene detail (2/2) → off**. The overview contains scene, timing, input,
and audio telemetry; the Setup detail page contains its row, transition,
configuration, and PPU state. F11 and F12 remain pausing audio and ROM-asset
browsers.

The shared snapshot reports:

- scene name, global frame, scene-local frame, and scene timer;
- pressed, held, and released controller masks;
- Game Setup page, selected row, submenu row, transition phase/frame/blanking;
- Mode, Style, Level, and Quarter using human-readable values;
- captured brightness and BG1/BG2/BG3 scroll values;
- active audio track/status, Setup music/SFX volumes, and last menu SRCN;
- the last Setup navigation action, including the future team-selection handoff.

Print one snapshot after a deterministic headless run:

```powershell
.\build\nba95_port.exe --headless --setup-only --rom <rom> --assets <pack> `
  --frames 170 --debug-state
```

Print snapshots every 30 frames and render the compact overview into the frame:

```powershell
.\build\nba95_port.exe --headless --setup-only --rom <rom> --assets <pack> `
  --frames 180 --debug-every 30 --timing-debug --dump-frame debug.bmp
```

Every HUD line is emitted as `[DEBUG STATE] ...`; periodic groups begin with
`[DEBUG SAMPLE] stepped=N`. Use `--debug-hud-page 2` instead of
`--timing-debug` to capture the Setup-detail page. `tools/test_core_safety.py`
locks the Setup fields, sampling count, CLI validation, and exact bitmaps for
both compact F10 pages.

For a frame-by-frame Rules/Options transition audit, export a CSV:

```powershell
.\build\nba95_port.exe --headless --setup-only --setup-menu rules `
  --rom <rom> --assets <pack> --frames 320 `
  --setup-transition-trace build\rules-open.csv
```

The file includes the directed transition route, logical and packed-trace
frames, forced blank, brightness, layer masks, every BG scroll/map/CHR base,
and deterministic FNV-1a hashes of cumulative VRAM, CGRAM, and rendered pixels.
This makes missing INIDISP blanking or a layer reveal offset visible without
depending on a screen recording.

For BG2 specifically, the visible outgoing rows are rebased to the live
backdrop position. The packed absolute coordinate becomes authoritative during
forced blank, when `$80:A2BF` resets the rebuilt page. The final rows and a
subsequent `--debug-state` capture can be used to verify that `$80:A3B8` keeps
the same one-pixel-per-three-frame phase after the transition releases.
