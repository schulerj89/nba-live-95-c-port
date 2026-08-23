# Center-court tip-off

The C scene begins after the tenth Starting Lineup card. It creates all ten
player actors, then renders the eight composites visible in the settled
jump-ball camera and the jump ball from asset-pack data. Mesen screenshots are
comparison oracles; player art and the ball tile are read from ROM resources.

## Ghidra and live-trace proof

The focused Mesen trace divides the sequence into formation (frames 0–139),
jump ball (140–219), possession (220–399), and live play. Headless Ghidra then
correlates instructions and calls in those windows:

- `$86:DDA7–$E053` initializes five paired `$100`-byte player records. The
  live WRAM trace proves writes at `$86:DEE3/$DEED` (X), `$86:DEF5/$DF04`
  (Y), and `$86:DF51/$DF54` (Z). The resulting ten world positions are stored
  beside their measured `$80:AD92` sprite origins in `nba_tipoff.c`.
- `$86:E054–$E0AA` initializes the eleventh actor, the ball, at world
  `(0,0,80)`. OBJ tile `$EA` has one exact ROM match at file offset `$0D9C27`;
  asset 262 stores that raw tile and its hardware palette entries.
- `$87:A47A–$A98D` prepares each player draw. At frames 89/91 it culls actors
  4 and 9, producing visible submission order `8,2,6,5,0,1,7,3`.
  `$80:AD92–$AEC1` attaches the
  lower body, upper body, head, and jersey-number overlay, then `$80:B348`
  queues the raw sprite parts. The C court calls the same shared asset-pack
  compositor used by Player Lab, at native scale 1.
- At frame 156, `$86:ECF4 -> $87:B3BD` installs jump animation state `$32`.
  The traced center Z values rise `7,15,18,20` and fall `20,18,12,3`.
- At frame 198, `$86:CF49 -> $87:B47A` changes the contact animation. At frame
  200, `$86:D3F9 -> $86:B04C` initializes the post-tip actor/task state;
  `$86:9B80 -> $86:9846` resets the selected player and `$85:B100–$B245`
  performs the ROM's randomized possession decision.
- `$87:B832` supplies the repeated directional movement calculation as the
  players begin breaking from the circle.

Regenerate the evidence with `tools/mesen_tipoff_capture.lua`, then run
`tools/ghidra/Run-TipoffAnalysis.ps1`. The ignored `.analysis` outputs include
per-frame actor state, routine hits, sprite origins, raw PPU states, and frames.

## Regression lock

`tools/test_tipoff.py` locks pack version 17, the raw ball/court assets, the
formation/jump/contact cadence, Ghidra-address diagnostics, and RGB hashes at
frames 90, 170, and 220.
