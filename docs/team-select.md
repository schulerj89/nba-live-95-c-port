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
| `$82:8406` | `team_select_direction_input` | Moves the seven-position selector and routes Left/Right to alphabetical or ranking order |
| `$82:85D1` | `team_select_redraw` | Rebuilds team name, ROM logo objects, AI marker, and rankings |
| `$82:88D9` | `team_select_draw_matchup` | Draws both sides by temporarily selecting side 1 and side 2 |
| `$82:8933` | `team_select_animate_selected_plate` | Advances the selected gold plate through seven ROM palette windows at an eight-frame cadence |
| `$87:89D5` | `advance_mod3_frame_divider` | Separate modulo-three counter used by the screen's background-motion cadence |

The selected team IDs are 16-bit working values at `$7E:16FB` (left) and
`$7E:16FD` (right). `$7E:1693` is a seven-position selector, `$7E:1695` is the
team being redrawn, and `$7E:16B5` owns the active side. At `$82:83C3-$83D6`,
the side handler swaps selector 0/1 only when a name row is selected. It leaves
ranking selectors 2..6 unchanged, then `$82:83D9-$8404` toggles `$16B5`. The
Left/Right paths at `$82:8477-$8548` update the team and write it back through
`$16FB + side*2` before jumping to `$82:85D1`.

Selector values 0 and 1 are the left and right team-name rows; values 2 through
6 are Scoring, Rebounds, Ball Control, Defense, and Overall. The scene starts on
the right name row. Left/Right on a name row walks alphabetical team IDs, while
Left/Right on a ranking row walks that ranking. Up/Down wraps between the active
name row and the five rankings. Any face or shoulder button swaps the active
side. The corresponding name row follows the side when a name is selected;
a selected ranking remains selected across the side change.

The two name paths are deliberately asymmetric. The right/home branch at
`$82:863C-$8792` calls `$81:A01F` with `$18C6=$00B4`, producing a right-aligned
name whose visible pixels finish at X=177. The left/visitor branch at
`$82:8793-$88D8` calls `$81:9FD4` with `$18C6=$0050`, producing a left-aligned
name beginning at X=80. Long-name Mesen captures use GOLDEN STATE on the left
and PHILADELPHIA on the right so neither alignment can regress unnoticed.

## Team data

Team IDs 0 through 26 are alphabetical. The ROM table at `$80:D9AF` contains
34 records of five values in this order: Scoring, Rebounds, Ball Control,
Defense, Overall. IDs 0..26 hold the one-based league ranks. East and West are
native IDs 27 and 28 at `$80:D21D`/`$80:D222`; their records at
`$80:DA36`/`$80:DA3B` contain 28 and 29, which the ROM ordinal path displays
as dashes. The remaining bonus-team records stay unexposed for now. The
headless dump writes the exact table to
`.analysis/team_select_ghidra/team_rankings.txt`.

The C port keeps these small semantic records in source control. Team logos
remain SNES graphics assets: they are extracted from ROM-driven Mesen PPU
captures into the asset pack and are not recreated with host artwork or fonts.

Asset-pack version 14 stores each team logo as a transparent 48x56 ARGB canvas
decoded from its variable palette-4 OBJ entries and their original 4bpp VRAM
tiles/CGRAM colors. It also stores per-team raw VRAM/CGRAM so names, ordinal
ranks, shadows, wallpaper, and palette changes continue to come from the ROM.
The runtime composites those assets with the captured OBJ panel pieces; no host
menu font or replacement team artwork participates in Team Select.

The right matchup entry is the home team and owns the wallpaper and its base
palette. Changing the left visitor or toggling the active cursor does not
replace that background; changing the right home team does.

Logo object counts vary by team: the verified right-side captures use 6..13
palette-4 objects, immediately followed by the fixed 15-piece palette-2 plate.
The runtime's shared geometry asset is team 18: right logo 0..5, right plate
6..20, left logo 21..31, and left plate 32..46. Logo extraction therefore
selects the variable palette-4 objects instead of assuming a count; the runtime
draws the two exact 15-piece plate ranges. Keeping those classes separate
prevents upper silver/gold plate pieces from being omitted and prevents logo
objects from being repainted across the ranking columns.

Ghidra's object path agrees with that classification. `$82:8348-$8353` and
`$82:88D9-$88FF` wrap the redraw in `$81:A975`/`$81:A981` and call
`$81:AA73` with both-side mask 3. `$81:AA73-$AB56` calls `$80:B344` twice per
side: `A=$2800` draws the variable team logo, then object group `$22` is drawn
at `(teamX-2, teamY-4)`. The active-side test on `$16B5` chooses `A=$2200` for
inactive silver or `A=$2400` for selected gold. Thus silver and gold use the
same complete 15-piece geometry; only their object palette selection differs.

The five ordinal values are likewise redrawn as one field by the
`$82:85D1-$88D8` refresh path. Their visible ROM baselines are irregular
(Y=119, 135, 151, 167, and 187), so treating them as five equal 16-pixel crops
clips the top of Ball Control and Overall and can make adjacent values appear
to overlap. The port now copies the complete 44x90 ROM rank field for each
side, then applies selection highlighting within its five 18-row bands. The
regression test checks every ordinal's exact visible bounding box on both
sides, including the final Overall row.

The selected gold plate does not move its OBJ pieces. Captures at frames
620..700 keep OAM byte-identical while CGRAM colors `$A1-$A7` change every
eight frames. The executed-address captures for frames 624..629 reach
`$82:8933` every frame and reach the `$82:8947` wrap branch only when `$1805`
completes its seven-step cycle. Ghidra shows `$82:893D-$8951` masking `$1805`
with `$0038`, dividing by four to make source offsets 0,2,...12, then selecting
an overlapping 14-byte window at `$82:8968`. `$82:895A-$8964` queues that
window to CGRAM destination `$A1` with length 14. Asset 250 stores the exact
26-byte ROM source table, and the renderer applies the same window; it does not
synthesize host colors or ingest Mesen screenshots.

The WRAM write trace begins `$1805=1` at Setup-relative frame 572, exactly 172
frames after the Start pulse, and records `$1805=$38` followed by zero at frame
627. The port uses that same start frame, 56-frame period, and `$0E -> $00`
source-offset sentinel instead of indexing an eighth palette phase.

## Port controls and transition

Enter/Start on Exhibition enters Team Select; controller A does not confirm a
top-level Setup mode. The original Setup SPC stream is kept alive. A/B/X/Y/L/R
toggles the active side; a name-row selector follows the new side while a
ranking selector remains on that rank. Up/Down moves through the active name
row and the five rankings. Left/Right moves alphabetically on the name row
or to the adjacent one-based rank on a ranking row, with wrap in either case.
Both team IDs live in `NbaSession`.

The 176-frame edge is split at the same scene boundary as the ROM. Game Setup
keeps ownership through frame 51: `$80:A3B8` scrolls BG3 away, slides BG1/BG2
apart, and ramps INIDISP down. At forced blank, `$80:DBF6` dispatches
`$82:809A`; `NbaTeamSelect.transition_frame` resumes at frame 52 for the hidden
builder, opposing BG1/BG2 entrance, BG3 vertical staging, and OBJ release. No
captured host frame is faded. `--dump-sequence-dir` exports every
headless rendered frame for video/regression inspection; `--team-demo` scripts
right-team cycling, the ROM side toggle, then left-team cycling.

The outgoing renderer must also reproduce the source layer windows. Its static
VRAM snapshot contains BG1 construction cells and wrapped BG2 map data that the
SNES never presents during this edge. The port therefore limits outgoing BG1
to the palette-5 header and clips both horizontal layers at their moving screen
boundaries; BG3 uses the `$80:A3B8` vertical window. Without those windows,
continuous port frame 200 displayed colored tile garbage even though a direct
`--team-only` smoke run looked correct. `test_team_select.py` now hashes five
reviewed frames from the natural `--setup-only` handoff, and the visible smoke
harness publishes the complete boundary as its own contact sheet. This port fix
is separate from the source-authentic clipped Team Select reveal described in
`known-original-game-bugs.md`.

## Reproduction

```powershell
tools\ghidra\Run-TeamSelectAnalysis.ps1 `
  -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc' `
  -GhidraHome 'C:\path\to\ghidra' `
  -JdkHome 'C:\path\to\jdk-21'
```

Set `NBA95_TEAM_NAV=1` for the isolated directional capture or
`NBA95_TEAM_PROBE_BUTTON=l` to prove the side-toggle path from a clean boot.
Set `NBA95_TEAM_PANEL_ANIM=1` to emit the frame, OAM, CGRAM, and executed-address
evidence used to correlate the selected-plate animation with Ghidra.
