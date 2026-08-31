# Court Logo Complaint Audit

The visible Chicago and Orlando center-court logos in the current production
path are the original BG2 court tiles. No court-tile corruption was found, so
this checkpoint intentionally makes no renderer or asset change.

This complaint is separate from the Player Introduction gold plates. The
variable introduction logos were previously drawn at the plate origin; source
sites `$83:F8A0/$83:F8A3` and `$83:F8D8/$83:F8DB` establish their corrected
`+2,+4` offset. Gameplay center court has no corresponding overlay logo. Its
logo and floor paint are ordinary entries in the 148x52 column-major map at
`$A0:8006`, streamed by `$85:8EE6-$85:90C3`, with team graphics selected by
`$84:E55D-$84:E57A`.

## Exact attribution

The production pack's asset 279 is byte-for-byte equal to ROM
`$A0:8000-$A0:BC25`, including the 148x52 header and every map word. For both
teams below, the 2,240-byte graphics stream was independently decompressed from
the ROM's `$84:E6B5` table and compared with the production indexed PPU input
(asset 284) and the existing normal-route native PPU capture:

| Home team | ROM stream | VRAM destination | Tile SHA-256 | ROM/native/pack differences |
|---|---:|---:|---|---:|
| Chicago (3) | `$AB:E88E` | `$B4A0-$BD5F` | `eb960a5a6b10705396f8dfc35970142704d9ae8325a5d7135388355ac0675acb` | 0 |
| Orlando (18) | `$AB:EE0B` | `$B520-$BDDF` | `5a2f51d48a8ffb5cdb9b8a865d66e019fb4affae761f80ee0651e50534958fca` | 0 |

The four court-owned CGRAM ranges also match the ROM palette table and native
capture exactly for each team: bytes `0-1`, `34-37`, `64-191`, and `240-245`.
Differences elsewhere in the 64 KiB native VRAM are expected live player,
object, circular-map, and animated-crowd state; they do not belong to the court
logo stream.

The existing full renderer proof still passes 16,000 production update frames
and 812 indexed viewports across all 29 teams. The 12 Orlando native camera
witnesses have zero map/scroll geometry mismatches. Their small static-art
differences are confined to the already documented animated crowd tile IDs,
not the center logo.

The current integrated executable also reports `HOME:18` for the Orlando
direct scene and `HOME:03` after selecting Chicago through Team Select. Both
production screenshots display the same logo shapes as their normal-route
native captures. Chicago's long horizontal bull graphic and Orlando's dark
blue oval are therefore preserved original art, not defects to redraw.

## Pregame crowd-band correction

The gameplay animated-crowd limitation above does not apply to the static
Player Introduction background. The old pregame decoder emitted palette color
zero as opaque even though SNES background color index zero is transparent.
Assets 260 and 271 now show the native CGRAM[0] backdrop at those pixels. The
Orlando top 16 rows have zero differing pixels against native frame 2550, and
an independent frame-2300 mask matches all 9,696 safe crowd pixels. Native BG1
is enabled but wins no pixels in that witness. Gameplay asset 283 and its
broader crowd-producer limitation remain unchanged.

Run the focused attribution check with:

```powershell
python tools/test_court_logo_attribution.py `
  --rom '<verified-rom>' --pack '<production-pack>' `
  --native-root '.analysis/home-courts-gameplay-20260823'
```
