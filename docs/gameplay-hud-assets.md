# Reproduce the original HUD resource

Resource286 contains original indexed HUD characters, tilemaps, palette and
font/string data. It contains no rendered screenshots. Version2 adds the eleven
original shot-clock maps to the nine existing sections. The 63 initial 2bpp
tiles remain attested native decompressor output: an independent format30
decoder has not been accepted for that route.

The full extractor requires a validated natural HUD capture. Its default is
`gameplay-hud` under `--capture-root` (normally repository `.analysis`). Set
`NBA95_HUD_NATIVE_CAPTURE` to use an existing accepted capture elsewhere. That
path is read-only; validation checks the original ROM, executed capture tools,
isolated emulator settings and raw resource identities.

To create a new capture, use the pinned capture tool and a fresh directory.
Coordinate the emulator slot if another capture is running:

```powershell
python tools/capture_gameplay_hud.py --rom '<original US ROM>' --mesen '<Mesen.exe>' --output .analysis/gameplay-hud
```

The supported ROM and emulator hashes are recorded in `tools/capture_gameplay_hud.py`.
The capture runner and Lua script retain their exact reviewed bytes through
`.gitattributes`; changing only line endings would otherwise invalidate the
resource builder's source-identity check. Do not relax that check or edit old
capture evidence to accommodate a different tool version.

Run the normal extraction/build commands after capture, or upgrade a previous
version31 pack into a **new** file without extracting all its other resources:

```powershell
python tools/upgrade_gameplay_hud_pack.py --base-pack '<existing pack without286>' --rom '<original US ROM>' --native '<accepted HUD capture>' --output build/hud-assets/nba95_assets.pak --manifest build/hud-assets/provenance.json
./build.ps1 -AssetPack build/hud-assets/nba95_assets.pak
```

The upgrade refuses existing output/receipt files, malformed pack directories
and an existing286 entry. Every old payload, ID, width, height and flag remains
identical; directory offsets change when the entry is appended. The receipt
records every preserved resource plus the new resource's provenance. To
replace an older HUD version, perform a fresh full extraction instead.

Root verification is retained in `build/hud-integration-v1` and
`build/hud-full-extraction-v1`. Both the upgrade and a complete ROM extraction
produce the same 264-entry, 89,442,736-byte pack, SHA256
`f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09`.
All 263 baseline resources remain unchanged. The new 3,926-byte HUD resource
has SHA256 `4a39e5d5464b676eed999823c178e41f3251a1a830b96221cfd3d3a50c1c0f2d`.
The pack helper also passes payload/metadata preservation checks and 13
malformed/existing-resource rejection cases.

This verifies asset production, not full HUD lifecycle or native publication
timing. The [bounded gameplay repair](gameplay-hud-integration.md) is reviewed
separately. Main's pack and the desktop executable are unchanged.
