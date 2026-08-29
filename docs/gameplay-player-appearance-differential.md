# Gameplay Player Appearance Differential

## Native scope

Fresh headless Ghidra listings and the union of existing native execution
traces give this bounded census:

| Native range | Purpose | starts | observed | not observed |
|---|---|---:|---:|---:|
| `$86:D85E-$DA20` | build active-player appearance records | 198 | 186 | 12 |
| `$86:E0B0-$E389` | build/deduplicate upload resource lists | 152 | 77 | 75 |
| `$87:AF75-$B450` | assign palettes, body/head families and jersey tiles | 579 | 564 | 15 |
| `$87:A47A-$A98D` | select live frame layers and submit compositor inputs | 567 | 475 | 92 |
| `$80:AD92-$AEC1` | attach and queue lower/upper/number/head layers | 132 | 132 | 0 |
| `$87:B649-$B952` | exact pose and attachment resources | 151 | 151 | 0 |
| **bounded total** |  | **1779** | **1485** | **294** |

Observed execution is not automatically C verification. The strict native
replay ledger currently covers `$87:B649-$B669` (14 starts) and
`$87:B832-$B952` (130 starts). The remaining preprocessing/draw instructions
are represented by asset extraction, pure C output witnesses and live layer
diagnostics; they are not falsely listed as instruction-by-instruction replay.

## First divergence and root cause

The old live fallback called `nba_player_animation_resources`, which always
used the ordinary lower-body descriptor table and returned only literal upper
resource IDs. Native `$87:AFA2-$B053` stores two appearance selectors per
actor:

- actor `+$A8` selects `$84:C218` or the tall-player `$84:C28A` lower table;
- actor `+$6C`, XOR the facing group, makes `$87:AC76-$AC95` or
  `$87:AD38-$AD57` add `$0028` to upper IDs below `$00F0`.

At the initial native `$87:A47A` witness, Orlando/West actor 0 is
upper/lower `$00F3/$068C`. The retired fallback produced the ordinary-table
counterpart `$00F3/$044F` for an analogous tall-player path.
`nba_player_animation_resources_for_appearance` now consumes both selectors
and reproduces the ten native initial resource pairs stored in the self-test.

The second fault was in the asset pack. Animation frame lists contain base
upper IDs, while the native runtime derives `base+$28`. The extractor packed
only literal frame-list IDs, so correct derived resources such as `$004A`
could be selected but were absent. `build_player_animation_asset` now closes
the resource set under the native `+$28` transform before packing it.

## Harness evidence

Gameplay JSONL now records an `appearance` object for every actor and frame:

- exact lower, upper, head and number resource IDs;
- opaque source-pixel counts for each layer;
- separate palette, resource-presence, number-gate and total-valid flags.

This distinguishes a valid native no-op layer from a missing packed resource.
The 1,400-frame pre-fix audit found 265 actor-frames with a selected upper
resource absent from the pack. After extraction, the same run exercised 81
derived upper resources with zero missing layers. The 63,800-frame CPU
regression crossed the new appearance assertions without a layer failure.

Permanent guards:

- `nba_player_animation_self_test` replays the ten native initial
  Orlando/West appearance/resource witnesses.
- `tools/test_player_lab.py` reconstructs all raw descriptor references and
  requires every native `base+$28` resource in the asset pack.
- `tools/test_cpu_gameplay.py` rejects any live lower/upper/head/visible-number
  layer that is absent or has no packed ROM pixels.

All visuals still come from ROM-derived asset-pack data. Emulator screenshots
are evidence only and are not shipped or rendered by the port.

## Remaining caveats

The 294 unobserved native starts are mostly setup/upload-list and draw-time
presentation/effect branches. They remain an honest census caveat: their
portable outcome is covered for ordinary gameplay, but rare callers need new
native witnesses before claiming instruction-level parity. Native PPU upload
queue timing is represented by the prebuilt asset pack rather than emulated
as WRAM/DMA traffic.
