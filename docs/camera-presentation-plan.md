# Camera and court presentation

Started from clean main f62a84f, 2026-08-27. User selected the three
"Additional camera/presentation routines" rows, not the entire preceding
CPU/animation backlog. Baseline: 510 decoded instructions to audit/integrate:

| Slice | ROM | Count |
|---|---|---:|
| Presentation wrapper | 85:8E1C-8EE5 | 78 |
| Court streaming | 85:8EE6-90C3 | 220 |
| Remaining camera core | see below | 212 |

The 212 are 91CB-91DE (8), 91FB-9218 (18), 9230-92BF (68),
92CA-92E3 (10), 92F9-932E (25), 932F-9348 (10), 9352-93F4 (73).
The separately reported 60 camera correction instructions, 39 setup/caller
instructions, and 323 CPU/animation instructions are not automatically
included. Dependency corrections must be explained, never counted twice.

## Plan and acceptance

1. Read Ghidra/recomp and assets; retain exact instruction census and label
   C correspondences. Separate portable visual semantics from SNES DMA glue.
2. Capture native wrapper/stream/core inputs and outputs, including controlled
   boundary cases. Replay through production C and keep durable witnesses.
   Verify source/destination wrapping, both axes and changes of direction.
3. Correct asset extraction/rendering where ROM proves a mismatch. Use only
   asset-pack art at runtime; emulator pixels are comparison evidence only.
4. Add regression checks and inspect C screenshots/video. Run full suite.
5. Update status/provenance, rebuild, refresh/verify desktop shortcut, commit
   and push main. Do not claim full-game parity from bounded routine proofs.

## Initial findings

- ROM A0:8000 begins 94 00 34 00 09 03: dimensions 148 x 52. The old
  extractor hardcodes 114 x 52, truncating 34 columns. At camera X=328,
  a 256-pixel viewport needs pixels through X=1165, but the old 912-pixel
  panorama clamps its origin to 656. Native loader/stream captures confirm
  the full dimensions; this truncation is now corrected.
- Camera 9192-93F4 still passes the existing 500 replay vectors, but the
  recorded full-range label is not proof of all branches. Keep separately
  documented init, no-team, orientation and alternate-height gaps visible.
- Runtime need not emulate VRAM/DMA. A complete raw-ROM court panorama is
  acceptable only with a tested mapping to the ROM's circular tile streamer.

Evidence root: `.analysis/camera-presentation-proof-20260827/`.

## Implementation and proof

| Requested row | Result | Proof boundary |
|---|---|---|
| 78 wrapper instructions | Basket/window state translated and bound after the camera update | 72 state-writing instructions replay exactly; six call/return instructions are audited boundaries, not verification credit for their callees |
| 220 stream instructions | Column/row walking, circular wrapping, source/destination, scroll and 99-word scratch translated | Ordered transfer descriptors and all owned state replay exactly; no SNES DMA emulator is added |
| 212 camera instructions | Reverified existing C arithmetic against a fresh exact census | Seven precise slices replace the old overbroad whole-routine ledger entry |

Production entry points are in `src/nba_court_presentation.c` and are called
from `src/nba_tipoff.c`. The wrapper selects the basket from period/camera
side, calculates the left/right window bands, and preserves 087E in the
middle band. Stream columns advance before rows; vertical streaming has the
native three-row limit. JSONL exposes real 0874/0876/0878/087A stream state,
087C/087E/0880/0882 window state, 3FEF basket selection and row-byte count.
The renderer consumes the complete asset-pack panorama using the independently
verified pixel origin; it does not render captured screenshots or replay DMA.

Asset-pack version 29 contains asset 273 panorama schema v2 (29 x 1184x416)
and new asset 279 (the literal 15398-byte ROM map/header). The original court
art in each panorama's first 912 pixels is protected by its previous hash.
All 29 team extents and 522 real-renderer viewports exercise both camera
extremes and the former premature clamp boundary at camera X=74/75.

### Native routine checks

- 3,000 native calls replay with zero owned-output mismatches: 1,000 each
  for core, wrapper and stream. Controlled cases total 300; the other 2,700
  calls use natural inputs. Controlled WRAM inputs are supplied at native
  call boundaries, never by patching ROM, PC, processor flags or stack.
- All **510 distinct decoded instruction starts** match the fresh Ghidra
  census and occur in captures. Checked-in fixtures retain 480 calls:
  all controlled cases plus 60 natural calls per routine family.
- A 16,000-frame C runtime binding test and 522 viewport renders pass.
  Four additional scenarios deliberately disagree current period 0926 with
  the menu's quarter-length setting 1701. They caught a wrong new caller
  binding (exit 12 before the fix); the wrapper now reads the actual period.
  Existing CPU/endurance regressions remain required; vector replay alone
  is not evidence that gameplay trajectories match.
- Fresh persisted Ghidra labels/comments are generated by
  `tools/ghidra/DumpCourtPresentation.java`; the final successful dump is
  `ghidra/court_presentation_bank85.txt` under the evidence root. It identifies
  8E1C caller, 8E28 basket/window, 8EE6 streamer, 8FD4 row streamer,
  9192 camera and 9352 adaptive axis approach.

### Visual checks and their limits

The uncontrolled `natural-final/` capture contains 12 native screenshots plus
raw VRAM/CGRAM/PPU state. Each 256x224 BG2 viewport has **zero map/scroll
geometry differences** when rebuilt from the raw ROM map and that frame's
CHR/palette. The stored visible-map hashes also run without local captures.
WRAM may contain the next logical camera while the PPU still displays the
previous pass: compare published PPU scroll against the resident ring origin,
not current WRAM camera values. This distinction corrected a two-pixel false
failure in the first comparison; that diagnostic remains in local evidence.

Static pack art versus native BG2 differs by 0-461 pixels in these frames,
on crowd graphic tiles. This is an explicitly outstanding animated-art
difference, not a tolerated map mismatch or whole-frame parity claim.
Window/basket state is persisted and visible in telemetry; its downstream
BG1/backboard/window compositor is **not** implemented by this state model.
The wrapper's external audio/actor/render calls retain independent scopes.
In particular 83:CC10 is a whistle/audio countdown, not a crowd animator.

Port proof: `court-scroll.mp4` (frames 1800-2200, camera X -150 through 194),
`port-video/frame_1800.bmp`, `frame_2000.bmp`, `frame_2200.bmp`, and
`port-video.jsonl` in the evidence root. The recording crosses the old early
clamp. Native screenshots are comparison evidence only, never runtime art.

## Reproduce

```powershell
.\build.ps1 -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc'
.\tools\capture_camera_presentation.ps1 -OutputDir .analysis/camera-stream -Kind stream -Controlled
.\tools\capture_camera_presentation.ps1 -OutputDir .analysis/camera-core -Kind core -Controlled
.\tools\capture_camera_presentation.ps1 -OutputDir .analysis/camera-wrapper -Kind wrapper -Controlled
.\tools\build_vector_probe.ps1 -Name court_presentation_probe
python tools/verify_court_presentation.py --require-census --vectors tests/fixtures/camera-presentation-witnesses.json --probe build/court_presentation_probe.exe --pack build/nba95_assets.pak
python tools/test_court_presentation.py --pack build/nba95_assets.pak --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc'
.\tools\build_vector_probe.ps1 -Name court_runtime_probe
.\build\court_runtime_probe.exe build/nba95_assets.pak
.\build.ps1 -Test -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc'
```

For live pixel reconstruction add `--native-dir` pointing to `natural-final`
to the asset test. The normal regression depends on checked-in witnesses,
not an installed/running emulator or ignored local capture files.

## Remaining work (outside the selected three rows)

Historical census below: the first two rows were subsequently completed in
`camera-handoff-plan.md`, with separate raw native-call and runtime proofs.
Other rows remain pending; this checkpoint's original scope is unchanged.

| Area | Pending decoded instructions |
|---|---:|
| Camera init, fractional boundaries, orientation, no-team centering, alternate-height flags | 60 |
| Separate camera setup/caller integration | 39 |
| Previously reported CPU/animation backlog | 323 |
| Separate timeout slice | 9 |
| Crowd animation and downstream presentation callees | Not recounted here |

Do not add six audited wrapper call/return sites to verified callee coverage.
The ledger credits 504 instructions across the selected slices, including
the reverified 212, and removes the earlier unproved full-camera claim.
Captured-address coverage changes from 7,000/27,901 (25.09%) to
7,206/27,901 (25.83%): **+206 positions / +0.74 percentage points** after
correcting that old claim. This is not 510 new verified addresses, branch
exhaustiveness, or percentage of the whole game completed.

## Release verification

The final `build.ps1 -Test` exited 0; see `full-regression-release.log` in the
evidence directory. It includes the 63,800-frame CPU regression (2,005 exact
pass-frame checks, 91 automatic unlocks), both 200,000-frame owner endurance
matchups, all durable ROM replays, clean/headered asset regeneration, menus,
player lab/setup/intro, tip-off, debugger and intro tests. Earlier failed or
superseded logs are diagnostic history, not this release result.

The only F12 golden changes are the entry-count/index text row after adding
asset 279; comparison against reconstructed prior packs proves no asset-art
changes in those screens. Gameplay screenshot anchors remain unchanged.
Release-native replays and pixel checks are in `replay-release.log` and
`pixel-check-release.log`; the recording was regenerated with the release
executable. The standard OneDrive Desktop `NBA Live '95 (C Port).lnk` was
saved and read back: it targets `build/nba95_port.exe`, this repo's version-29
`build/nba95_assets.pak`, and the user's F-drive ROM. The recomp shortcut
was not changed.
