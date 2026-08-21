# Team Select reverse-engineering notes

This screen is reached by confirming Exhibition with Start on Game Setup. The
facts below come from `tools/mesen_team_select_capture.lua` against the verified
USA ROM and the reproducible headless Ghidra dump in
`tools/ghidra/Run-TeamSelectAnalysis.ps1`.

## Scene and input routines

| Address | Label | Observed responsibility |
|---|---|---|
| `$80:DBF6` | `main_dispatch_team_select` | Calls `$82:809A` after the Setup Exhibition route |
| `$82:809A` | `team_select_scene` | Scene construction and update entry |
| `$82:838E` | `team_select_frame` | Reads controller state and dispatches the live screen |
| `$82:83BC` | `team_select_side_input` | A/B/X/Y/L/R path; toggles active side `$7E:16B5` between 1 and 2 |
| `$82:8406` | `team_select_direction_input` | Up/Down selects one of five ranking categories; Left/Right chooses the previous/next team in that ranking order |
| `$82:85D1` | `team_select_redraw` | Rebuilds team name, ROM logo objects, AI marker, and rankings |
| `$82:88D9` | `team_select_draw_matchup` | Draws both sides by temporarily selecting side 1 and side 2 |

The selected team IDs are 16-bit working values at `$7E:16FB` (left) and
`$7E:16FD` (right). `$7E:1693` is the current category row, `$7E:1695` is the
team being redrawn, and `$7E:16B5` owns the active side. The side handler at
`$82:83D9-$8404` compares `$16B5` with 1 and stores the opposite value. The
Left/Right paths at `$82:8477-$8548` update the team and write it back through
`$16FB + side*2` before jumping to `$82:85D1`.

This explains the original interaction: Up/Down does not itself change the
matchup. It chooses Scoring, Rebounds, Ball Control, Defense, or Overall;
Left/Right then walks teams by that ranking. Any face or shoulder button swaps
which matchup side is active.

## Team data

Team IDs 0 through 26 are alphabetical. The ROM table at `$80:D9AF` contains
27 records of five one-based ranks in this order: Scoring, Rebounds, Ball
Control, Defense, Overall. The headless dump writes the exact table to
`.analysis/team_select_ghidra/team_rankings.txt`.

The C port keeps these small semantic records in source control. Team logos
remain SNES graphics assets: they are extracted from ROM-driven Mesen PPU
captures into the asset pack and are not recreated with host artwork or fonts.

Asset-pack version 12 stores each team logo as a transparent 48x56 ARGB canvas
decoded from the capture's first six OBJ entries and their original 4bpp VRAM
tiles/CGRAM colors. It also stores per-team raw VRAM/CGRAM so names, ordinal
ranks, shadows, wallpaper, and palette changes continue to come from the ROM.
The runtime composites those assets with the captured OBJ panel pieces; no host
menu font or replacement team artwork participates in Team Select.

## Port controls and transition

Enter/Start on Exhibition enters Team Select. The original Setup SPC stream is
kept alive. A/B/X/Y/L/R toggles the active side, Up/Down selects the ranking
category, and Left/Right moves to the adjacent one-based rank with 1/27 wrap.
Both team IDs live in `NbaSession`.

`NbaTeamSelect.transition_frame` reproduces the 176-frame captured edge from
Setup confirm through the outgoing fade, forced blank, opposing BG1/BG2 slide,
BG3 vertical staging, and OBJ release. `--dump-sequence-dir` exports every
headless rendered frame for video/regression inspection; `--team-demo` scripts
right-team cycling, the ROM side toggle, then left-team cycling.

## Reproduction

```powershell
tools\ghidra\Run-TeamSelectAnalysis.ps1 `
  -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc' `
  -GhidraHome 'C:\path\to\ghidra' `
  -JdkHome 'C:\path\to\jdk-21'
```

Set `NBA95_TEAM_NAV=1` for the isolated directional capture or
`NBA95_TEAM_PROBE_BUTTON=l` to prove the side-toggle path from a clean boot.
