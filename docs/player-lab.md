# Player Lab (F9)

The Player Lab validates roster, player art, and ROM gameplay animations before
gameplay is implemented. Press **F9** from any scene. Use Left/Right to cycle
all 29 teams, Up/Down for each 12-player roster, Q/E for animation states, U/I
for the eight directions, and K to pause/resume playback.

The eight direction slots are 45-degree steps (0 through 315 degrees). The HUD
shows the angle and the ROM mirror state; directions 0-2 use the horizontal
flip path selected at `$87:A5FB-$A609`, while directions 3-7 do not.

Runtime data comes from asset-pack v15:

- asset 251: 29 x 12 fixed records parsed from the pointer table at `$84:E640`;
- asset 252: precomposed camera-facing regression oracle (not used at runtime);
- asset 253: `NBPTILE2` manifest containing the raw pose tiles plus all 39
  five-direction head families, each with its ROM address and bytes;
- asset 254: `NBPALET2`, the 29 teams x two uniform sources x three player
  palette variants reconstructed from `$AB:FDE2`, `$AE:DB76`, and the
  `$AF:F022/$F042` overlays;
- asset 255: `NBPPOSE2` OBJ layout and hardware palette proven by the pre-tip frame;
- asset 256: `NBPANIM1` schema 4, containing all 57 state slots, the complete bank-$84
  descriptor data, 1,878 referenced raw sprite resources, and ROM attachment
  tables for composing lower body, upper body, roster-selected head, and the
  dynamically generated jersey-number overlay, including its per-upper-frame
  visibility table.

The pack build does not read screenshots, VRAM, CGRAM, or OAM captures for the
Player Lab pose. Mesen is an oracle for the live arrangement; the art itself is
copied byte-for-byte from the verified ROM. The C renderer decodes and composes
the packed 4bpp tiles at runtime; it does not display the precomposed oracle.
Animation art is copied from the resource table at `$89:8000`, not from Mesen
captures. At runtime the selected roster record supplies its head family, team
uniform palette, and player palette variant. Direction and animation state then
select the authentic body frames and cadence from bank `$84`.

## ROM and subroutine proof

- `$86:D7B8-$D85D` (`gameplay_build_player_record_pointers`) builds the ten
  active player-record pointers.
- `$86:D85E-$DA17` (`gameplay_build_player_appearance`) reads record offsets
  `+$36` and `+$37`, adds them into one appearance key, and builds five entries
  for each side.
- `$86:D73E-$D7B7` (`gameplay_resolve_player_sprite_parts`) sorts the five
  `(appearance key, player slot)` pairs.
- `$87:B01D-$B03A` reads roster `+$07`, normalizes selectors `>= $27` with
  `& $1F`, and computes the five-direction head family as
  `$049C + selector * 5`. Player Lab uses family entry `+2`, the straight-on
  camera-facing head. Selectors 0 through 38 occur in the roster data, so the
  asset pack carries all 39 families; this includes Rambis (37) and Workman
  (38).
- `$87:AFD4-$AFF4` converts roster `+$06` to palette offset `$000/$200/$400`
  (values above two clamp to two), and `$87:A47A-$A482` retrieves that exact
  player palette offset for rendering.
- `$80:8A02-$8A56` is the generic DMA path to CGRAM. Live Mesen records the
  `$2122` writes at `$80:8A3F`.
- `$80:8BA1-$8BCF` is the generic ROM/WRAM-to-VRAM DMA path used while gameplay
  graphics are staged.
- `$87:AB38-$AC3D` selects lower-body state `+$32` through `$84:C218` or
  `$84:C28A`, advances its ROM cadence, and writes the resource to player `+$2C`.
- `$87:AC3D-$AD5A` selects upper-body state `+$30` through `$84:C2FC`, handles
  fixed/per-frame/synchronized cadence modes, and writes player `+$2A`.
- `$80:AD92-$AEC1` attaches lower body, upper body, and head using the signed
  resource offsets at `$A9:D86E/$A9:D03E`, then queues each raw descriptor via
  `$80:B348`.
- `$80:B452-$B498` mirrors queued sprite parts around their final pixel index:
  it subtracts `7` for 8x8 parts and `15` for 16x16 parts. Player Lab uses the
  same `extent - 1` rule, including the direction-6 `$0591` number overlay.
- `$87:B357-$B378` reads roster byte `+$00` and maps the binary jersey number
  through `$80:859C`; `$87:B05B-$B354` composites its three directional glyph
  views from `$A6:AFD6`. `$87:A98E` selects overlay resource `$0591-$0593`, and
  `$80:AE20-$AE4E` positions it from `$AC:D07B/$AC:AE1B`. Side-on directions 1
  and 5 intentionally have no number layer. `$80:AE78-$80:AE86` applies the
  overlay X flip only when the selected resource is `$0591`; the number layer
  does not inherit the upper body's direction flip. `$87:A99E` maps directions
  0 and 6 to their perspective buffers at `$86F0/$8730`; Player Lab retains
  that exact direction-specific mapping.
- `$87:A506-$A51E` reads signed byte `$AC:C7E3[upper resource]`; a negative
  entry leaves `$D8` negative, and `$80:AE74-$AE76` skips the number overlay.
  Player Lab packs the complete `$830`-byte table and applies this gate before
  composing or drawing a jersey tile. Its CLI reports the selected upper
  resource, visibility state, and raw gate byte.
- `$85:8CAE-$8CB9` copies the 64-byte palette source at `$AF:E99F` to WRAM,
  `$85:8CBD-$8CE3` patches the two teams' colors, and `$85:8CF7` uploads OBJ
  palettes 6 and 7 starting at CGRAM `$E0`. `$80:AE86` selects palette 7 for
  the number overlay; Player Lab builds that palette from the same ROM inputs.
- The controlled-player Mesen trace confirms matching upper/lower states for
  ordinary motion and intentional overlays such as upper `$2B` + lower `$0C`,
  upper `$2F` + lower `$05`, and upper `$30` + lower `$0C`.

The headless Ghidra evidence is written to
`.analysis/gameplay_players_ghidra/`. The reproducible live trace is
`tools/mesen_gameplay_player_capture.lua`.

## Headless verification

```powershell
.\build\nba95_port.exe --headless --rom '<rom>' --assets build\nba95_assets.pak `
  --player-lab --player-team 18 --player-roster 4 --frames 1 `
  --player-animation 0x32 --player-direction 6 `
  --dump-frame player-lab.bmp
```

`tools/test_player_lab.py` locks the roster schema, known Chicago/West records,
ROM tile/resource manifests, all head families, distinct animation states,
animation/direction cycling, navigation wrap, CLI validation, and rendered
frame hashes.
