# Player Introduction / Starting Lineup

Player Setup now hands off to a court-backed pregame presentation with matchup,
ratings, and ten Starting Lineup cards. The first five records belong to the
visitor and the next five belong to the home team. Cards advance every 434
frames, matching the measured ROM sequence, and the final home player waits for
Start/A.

## ROM and Ghidra proof

- `$87:BD7F-$87:C0AB` is the complete Starting Lineup presentation routine.
  `$87:BE92` is its repeated card/palette/graphics loop; `$87:C0AB` is the final
  `RTL`, not the entry point.
- `$80:C62B-$80:C67D` dispatches compressed graphics commands. Every observed
  card swap reaches `$80:C633`; command `FB46` branches to the `$80:BD1B`
  expansion handler.
- `$81:A1E7-$81:A241` copies the three portrait palette groups after each swap.
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

## Regression

`tools/test_player_intro.py` locks the two asset payloads, all 290 portrait
keys, Player Setup handoff, first-card frame, visitor/home boundary, and
final-card cadence.
