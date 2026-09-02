# Center-court layout correction

The jumbled center-court logo was a port error. The old renderer applied
Orlando's parquet map and shared characters to every home team, then replaced
only the team-specific tile block. New York and Chicago require the other
layout. Correct individual logo bytes do not establish correct placement.
This supersedes the earlier conclusion that the fragmentation was original
artwork.

## Original source and correction

Fresh headless Ghidra listings and bounded C emitted by the game's
`snesrecomp` toolchain agree on `$85:8BBF-$8C4E`:

| Home selector | Descriptor | Map, including header | Shared compressed CHR | Team CHR destination, bytes |
|---|---|---|---|---|
| Boston 1, Milwaukee 16, Orlando 18 | `$AF:E4F8` | `$A0:8000` | `$A1:A8ED`, decoded `$5340` bytes | `$B520` |
| Other exposed teams 0..28 | `$89:FF81` | `$A0:BC26` | `$A2:8000`, decoded `$52C0` bytes | `$B4A0` |

The original also sends selector 33 to parquet; that selector is outside the
port's 29-team catalog. Both maps contain a six-byte header and 148x52
column-major words. Both shared CHR resources upload at VRAM byte `$51C0`.
`$84:E55D-$E592` selects the team's graphics through `$84:E6B5`, expands
`$8C0` bytes, then reads the destination from the selected descriptor's `+$10`.
`$85:9117` adds the selected map base when initializing the circular stream.
The streaming implementation is `$85:8EE6-$90C3`.

Asset 279 remains the literal parquet map. New asset 288 stores the literal
standard map. The extractor rebuilds catalogs 272/273/284 using the selected
map and native court CHR, retaining the independent BG1, HUD and OBJ inputs
below `$51C0`. Runtime BG2 drawing and the stream initializer select the same
home layout. The existing fan samples also use the native four-tile relocation
on standard courts; their cadence remains the existing bounded approximation.

Both shared resources are independently decompressed from the ROM and checked
against all 29 native presentation VRAM inputs. Boston and Milwaukee use FB30
team streams unsupported by the bounded offline decompressor. Their final
native PPU bytes are the asset inputs, and fresh original-game captures verify
the full 2240-byte upload, correct destination and visible floor. They are not
treated as all-zero delta streams.

## Evidence and smoke tests

The correction's ignored evidence lives in `build/court-logo-20260902/` in the
regular checkout. The accepted implementation was fast-forwarded into `main`
before this work; new builds and captures do not run from a nested worktree.

- `reference-complete/`: fresh Ghidra instruction listings for banks 80/84/85,
  generated recomp C, original-byte hashes, commands and tool-source hashes.
  The sibling `NBA-Live-95-Recomp` generated banks do not cover this gameplay
  routine; the fresh bounded C is a second source translation, not an
  independently running full-game recompile.
- `native-new-york-state/`, `native-boston/`, `native-milwaukee/`, and
  `native-orlando/`: private portable Mesen runs, each with 180 consecutive
  renderer frames, seven raw PPU checkpoints and the original upload hook.
  Inputs navigate the menus; Exhibition mode is explicitly seeded. The
  scripts verify the requested home selector and isolate saves/settings.
- `smoke-final/`: all 240 frames for every one of 29 configured home teams,
  plus 241 frames from the real button route through Team Select, Player Setup,
  introductions and tipoff. All 7201 frames are retained as PNGs with compressed
  winning-layer masks and per-frame state/hash records. Every visible BG2
  floor/logo pixel in world rectangle `(470,160)-(710,322)` is compared against
  an independent decode of the selected ROM map and raw native team VRAM.
  Fade, camera, home/visitor identity, formation and first possession are
  checked; invalid state seeds must fail. The test keeps players and animation
  enabled and uses their actual layer ownership to identify exposed floor.
- Seven original PPU checkpoints per native team have zero floor differences
  against that independent decode. Six New York/Orlando checkpoints also show
  identical bytes for all 28 fan tiles after applying the four-tile relocation.
- `court-runtime-final.log`: 16000 production caller updates, 812 static
  indexed viewports across 29 teams and four period scenarios pass. This
  isolated map fixture hides actors and disables the sampled fan overlay;
  the separate frame smoke runs the unmodified production scene.
- `reproduced/nba95_assets.receipt.json`: rebuilding the pack produces SHA256
  `c2d1a79ba0f8384a54622fef7744a64cbf606470759aca8eaf5476855f183f22`.
  Only 272/273/284 change and 288 is added; all 262 unrelated resources and the
  established Orlando entries remain byte-identical.

The frame gallery includes a scrubber, consecutive New York contact sheets
and every home court at frame 90. This is a floor/layout proof through tipoff,
not whole-frame native gameplay parity. Player timing, HUD coverage and the
full animated-crowd producer keep their separately documented limits.

`visible-smoke-release/` passes all 28 commands and 10 regression groups from
the installed `build/nba95_port.exe` and `build/nba95_assets.pak`. The default
Orlando Tipoff image regression also passes without changing its hashes.

Appending 288 changes the F12 count from 265 to 266. The three debugger guards
for assets 126/128/160 were updated only after comparing both packs: exactly
18 pixels in the count's final digit change in each view, and their existing
stable-canvas guards remain unchanged. Game menu artwork hashes were retained.
The two New York Tipoff reveal hashes in the frontend route were updated after
the old executable reproduced their former values and the corrected route
matched every frame of the independently checked Tipoff smoke. Captures before
court entry remain identical; `before-after.json` records that comparison.

## Reproduce from the regular checkout

The full asset extractor uses the corrected shared builder. An existing
265-resource preview pack can also be upgraded without regenerating unrelated
capture inputs:

```powershell
python tools/upgrade_court_pack.py --base-pack '<old-pack>' `
  --rom '<verified-rom>' --capture-root '.analysis' `
  --output 'build/court-proof/nba95_assets.pak'
.\build.ps1 -RomPath '<verified-rom>' `
  -AssetPack 'build/court-proof/nba95_assets.pak' `
  -OutputExe 'build/court-proof/nba95_port.exe'
```

Generate source references and four fresh native input captures (all output
directories must be new):

```powershell
python tools/regenerate_court_logo_reference.py --rom '<verified-rom>' `
  --recompiler '<snesrecomp-source/recompiler>' --ghidra '<analyzeHeadless.bat>' `
  --jdk '<jdk-directory>' --output 'build/court-proof/reference'
python tools/capture_court_logo.py --rom '<verified-rom>' --mesen '<Mesen.exe>' `
  --team 17 --output 'build/court-proof/native-new-york-state'
python tools/capture_court_logo.py --rom '<verified-rom>' --mesen '<Mesen.exe>' `
  --team 1 --output 'build/court-proof/native-boston'
python tools/capture_court_logo.py --rom '<verified-rom>' --mesen '<Mesen.exe>' `
  --team 16 --output 'build/court-proof/native-milwaukee'
python tools/capture_court_logo.py --rom '<verified-rom>' --mesen '<Mesen.exe>' `
  --team 18 --output 'build/court-proof/native-orlando'
python tools/test_tipoff_court_smoke.py --rom '<verified-rom>' `
  --exe 'build/court-proof/nba95_port.exe' --pack 'build/court-proof/nba95_assets.pak' `
  --capture-root '.analysis' --native-root 'build/court-proof' `
  --output 'build/court-proof/smoke'
```

The capture root supplies the existing raw team VRAM files, not screenshots.
The smoke test's direct-entry controls are `--headless --tipoff-only
--tipoff-home-team N --tipoff-away-team N`; sequence capture and button scripts
run inside the executable without desktop interaction.
