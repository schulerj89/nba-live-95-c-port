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
- active audio track, Setup music/SFX volumes, and last menu SRCN.

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
