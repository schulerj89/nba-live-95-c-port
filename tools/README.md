# Reverse-engineering and asset tools

## Reproducible pipeline

`capture_assets.ps1` runs these authoritative Mesen captures into ignored
`.analysis` directories:

| Script | Output |
|---|---|
| `mesen_intro_capture.lua` | legal and four EA intro reference frames; set `NBA95_CAPTURE_MOTION=1` for the 123-frame motion oracle |
| `mesen_title_capture.lua` | title PPU/APU/cue state |
| `mesen_setup_capture.lua` | settled Setup VRAM/CGRAM and PPU state |
| `mesen_setup_transition_capture.lua` | Setup entrance VRAM deltas and cycle-timed APU writes; `NBA95_CAPTURE_MOTION=1` also saves frame oracles |
| `mesen_setup_menus_capture.lua` | Rules/Options VRAM/CGRAM/OAM, per-frame open/return VRAM writes and complete PPU layer states, WRAM commits and DSP sounds; set `NBA95_CAPTURE_MENU=rules` or `options`, `NBA95_CAPTURE_VALUES=1` for independent Music Mode/Crowd/Slow Motion/Shot/CPU Assistance canvases, `NBA95_CAPTURE_CALLS=1` for deduplicated JSR/JSL targets, or `NBA95_CAPTURE_EVERY_FRAME=1` for a lossless screenshot oracle |
| `mesen_setup_main_capture.lua` | Main Game Setup Mode/Style/Level/Quarter value cycles, exact BG3 VRAM states, `$7E:16FB` working values, and executing CPU paths |
| `mesen_team_select_capture.lua` | Start-only Exhibition Setup-to-Team-Select transition, settled PPU memories, WRAM writes, and execution ranges; asserts `$82:809A` scene entry, supports `NBA95_TEAM_NAV=1` for isolated navigation, and `NBA95_TEAM_ALIGNMENT=1` for GOLDEN STATE/PHILADELPHIA alignment and home-wallpaper evidence |
| `mesen_gameplay_player_capture.lua` | Drives through Player Setup into gameplay, records Player Setup raw PPU state/routine hits, then records executed player-loader paths and roster/appearance/palette reads used to prove the F9 Player Lab ROM extraction |

`extract_assets.py` validates those captures and the ROM, then writes the asset
pack. Its minimal 65816 decompressor lives in `snes65816_decompressor.py`.
Pass `-CaptureName intro_capture`, `title_capture`, `setup_capture`,
`setup_transition`, `setup_rules`, `setup_options`, `setup_main`, or
`team_select_logos` to refresh
one capture while investigating it. The orchestrator supplies the menu-specific
environment flags and validates every extractor-required output before it
reports success.

## Regression tools

- `test_intro_sequence.py`: license, legal, and EA intro frames
- `test_title_pipeline.py`: title hardware assets, cues, pixels, and PCM
- `test_setup_transition.py`: transition, cursor rows, exit paths, PCM,
  Rules/Options page hashes, edit/commit storage, and F11 menu SFX
- `test_core_safety.py`: pack/ROM validation, host-rate timing, and SPC vectors
- `test_team_select.py`: Start-only Exhibition handoff, seven-position ROM selector, alphabetical/ranked wrap, 27 league teams plus East/West ROM teams and dash ranks, and Team Select frame hashes
- `test_player_setup.py`: Team Select confirmation, measured 200-frame handoff, ROM PPU assets, selected-team persistence, controller-side movement, and settled frame hashes

## Investigation utilities

The remaining `mesen_*.lua`, Python render/decoder helpers, `spc_render_main.c`,
and `spc_replay_main.c` are diagnostic tools. They are not runtime dependencies
or part of the normal build. Set `NBA95_CAPTURE_DIR` to the desired output
directory before running a diagnostic Mesen script.

The Ghidra wrappers under `tools/ghidra` regenerate labeled listings and
decompilation notes. Supply their `-GhidraHome` and `-JdkHome` parameters on a
different machine.

`mesen_team_select_capture.lua` also accepts `NBA95_TEAM_PANEL_ANIM=1`. It
captures the settled gold-plate frames plus OAM/CGRAM and short executed-address
traces. `Run-TeamSelectAnalysis.ps1` dumps the corresponding `$82:8933-$8967`
palette-window routine and the separate `$87:89D5-$89E8` background divider.

`Run-PlayerSetupAnalysis.ps1` labels and dumps the Team Select confirmation,
shared transition interpreter, Player Setup dispatcher, object positioning and
redraw paths, selected-panel palette animation, and vertical-scroll IRQ handler.
