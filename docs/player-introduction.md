# Player Introduction / Starting Lineup

Player Setup now hands off to a court-backed pregame presentation with matchup,
ratings, and ten Starting Lineup cards. The first five records belong to the
visitor and the next five belong to the home team. Cards advance every 434
frames, matching the measured ROM sequence, and the final home player waits for
Start/A.

## ROM and Ghidra proof

- `$87:BD7F-$87:BFE1` is the main Starting Lineup presentation routine and
  `$87:BE92` is its repeated card/palette/graphics loop. `$87:C01E-$87:C06E`
  draws the team label, `$87:C06F-$87:C0AB` allocates the two detail grids,
  and `$87:C0AC+` builds the current card strings.
- `$87:BE13-$87:BE1B` loads the large `$A6:BB16` descriptor before the
  centered STARTING/LINEUP calls at `$87:BE33/$87:BE4D`. The card builder at
  `$87:C0AC` keeps that font for jersey/name at x=$50/$70, then switches to
  `$A9:8000` at `$87:C13C` for the position. `$87:C01E` uses `$A9:8000` for
  the team name at x=$50.
- `$80:C62B-$80:C67D` dispatches compressed graphics commands. Every observed
  card swap reaches `$80:C633`; command `FB46` branches to the `$80:BD1B`
  expansion handler.
- `$81:A1E7-$81:A241` copies the three portrait palette groups after each swap.
- `$81:9756-$81:9FD3` renders the proportional BG3 presentation font described
  at `$A9:8000`; `$81:9FDF-$81:A05E` measures/centers strings and
  `$81:A05F-$81:A1ED` seeds the dynamic text-tile grid.
- `$80:9829` uploads the independent pregame ARAM/BRR bank (the live transfer
  loop is `$80:98CD`). `$80:A9E3` sends command `$0BFC`, then `$80:AACD`
  advances the APU queue. The first introduction voices key on after that path.
- Live Mesen changes occurred 434 frames apart. The visitor/home boundary came
  after roster slot four, and roster table `$84:E640` confirms that slots 0..4
  are the five starters.

The generated listings and decompilation are under the ignored
`.analysis/player_intro_ghidra` directory. Rebuild them with
`tools/ghidra/Run-PlayerIntroductionAnalysis.ps1`.

## Asset provenance

Asset 260 is the Mode-1 court BG2 reconstructed from raw VRAM/CGRAM using the
live map `$1000`, CHR `$4000`, and scroll `(6,6)`. Asset 261 contains keyed
72x72 RGBA portraits decoded from the raw palette-zero OBJ group. PNG files are
visual evidence only; the extractor does not read them.

The verified portrait catalog contains both visitor and home uniform variants
for all five starters on every ROM team: 29 teams and 290 decoded portraits,
including East and West. The
`mesen_player_intro_portraits.lua` harness pins Game Setup to Exhibition,
navigates Team Select until the requested ID is observed, and verifies that ID
again when `$87:BE92` begins the lineup loop. It saves raw PPU state only after
the ROM has built each home-team card.

Assets 265-268 contain the Player Introduction's 64 KiB ARAM bank, S-DSP
register snapshot, SPC state, and 5,560-frame cycle-timed DSP program. Asset
269 is the `$A9:8000` ROM font descriptor and its original 8x16 2bpp glyphs.
Asset 270 is the larger `$A6:BB16` 16x16 Starting Lineup font descriptor.
The C runtime synthesizes the BRR samples and does not store a mixed song WAV.

## Regression

`tools/test_player_intro.py` locks the visual assets, all 290 portrait keys,
ROM font format, SPC/DSP asset dimensions, a phase-tolerant runtime-audio
fingerprint, Player Setup handoff, first-card frame, visitor/home boundary,
and final-card cadence.
