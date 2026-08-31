# Configuration candidate and bounded verification

Candidate branch `work/setup-config-20260830` starts at pushed checkpoint
`e1bc0d4db83c1e19998e025cf653650aedc62437`. Independent audit is **pending**.
This document does not declare the game or every option complete.

## Changes and caller contracts

`nba_session.c` now uses original factory Exhibition/Arcade/Rookie/12-minute
defaults (`$81:C19A-$C231`). Its separately stored Custom profile begins45/45
with Fatigue/Injuries OFF, unlike Simulation's all-ON rules. Selecting Style
immediately changes active Rules through `$81:BFAA-$C00A`; it leaves committed
Main alone. `NbaSetupScreen.working_main` represents Main's `$16FB` buffer and
`NbaGameConfig.main_values` represents `$17AB`. The normal submenu and match
callers commit all four Main words. Rules Start commits all13 and updates
Custom when the adjustment dispatcher marked Style2. Options commits all7
without changing Style. Both return to Main row0. Beginning a new match
preserves configuration and the separate session Custom profile.

Main/Options wrap their cursors. Rules clamps at0/12, including its no-move
arrow feedback. `nba_menu_input.c` translates the five-record producer
`$81:AB58-$AC03` and pending consumer `$81:AC04-$AC52`, preserving complete
button words, native delay/speed/fast state and input-change quirks. Production
Setup currently routes only host controller0; the other four records are
disconnected. The five-record unit test is not a claim of host/controller
ownership wiring. Main's face/shoulder mask and exact Start dispatch are
distinct from the exact-word Rules/Options dispatchers.

The optional adjustment observer copies state at translated entry/exit; it
does not inject inputs or expected results. The test process receives only
the controller schedule. Native values stay outside that process.

## Evidence, denominators and limits

ROM SHA-256 remains
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
All native journeys execute the original unpatched ROM in Mesen with normal
controller input. `input-v1` deliberately preserves a held word across some
adjacent actions; equal hold/wait durations mean **no invented release frame**.
Its retained home/output/script attestations agree. New `faces-v1` and
`main-v4` captures use `run_mesen_isolated.py` with explicit subprocess
environment dictionaries, private executable/settings/save directories and
observed-home verification. No CPU/ROM/PPU/WRAM injection occurred.

| Gate | Exact compared population | Excluded from this claim |
|---|---|---|
| Stable menu replay |730 checkpoints /33,277 compared words: presets30, Rules257, Options289, held27, Main45, combined-input34, face/shoulder48; committed24, decoded Custom13, active working4/7/13, cursor and source-interpreted menu page/scene | Unused shared-buffer tail; Team Select working state; disk reload; frame-by-frame menu construction |
| Adjustment entry/exit |1,770 observations =885 dispatched adjustments; primary1,532 + combined-input238; all active working words, committed24, selected/max value, controller, previous/pending, fast/delay/speed/acceleration, exact input-frame offset | Other routines; intermediate CPU registers; input queued during divergent builder timing; visual/audio effects |
| Main value canvas |40 natural Main configurations before first submenu, every65,536 VRAM bytes, covering every exposed value | Upload timing, final RGB parity, font writer translation, production provenance of existing captured glyph packs |
| Rules value canvas |Seven native factory Rules entries/reentries, every65,536 VRAM bytes, including multiple OFF rows and logical row12 | Upload timing, OAM bars, cursor animation, viewport scheduling, whole transition |
| Deterministic C regression |Full-word mapping, five-record priority, disconnected records, release retention, repeat cadence, changed-word fast retention, signed-counter restart | Independent ROM equivalence; production multi-controller routing |

The stable page interpretation uses native pre-action row and the independently
decoded Main/submenu dispatch contracts. Labels and C-reported pages do not
choose expected pages. Match handoff additionally requires captured native
`$81:BF59/$BF6A` callbacks. The C process starts at the actual Setup component;
the native process starts at boot. Therefore this is a production-menu caller
comparison, not a claim of intro/frame alignment or complete natural C boot.

Strict timing comparison initially found three acceleration-state mismatches
despite identical final values. Source review established Rules redraw's
bar flag and Options navigation's explicit clear; the implementation was
corrected. No timing window or tolerance was added. The stronger stable gate
also found Options returning to Main row5 in C, causing a subsequent Start
to reopen Options. The native row0 return now reaches Team Select correctly.

Independent review replayed the full bounded implementation and canvases, then
found that permanent compact replay did not validate native provenance metadata.
The verifier now shares strict manifest validation with the raw-capture reader
and binds each compact manifest to a separate immutable copy of the original
capture manifest. It rejects changed injection flags, classification, ROM,
source identities, exit status, private home, controller/filter settings and
arguments. The native action/state/canvas payloads were not changed.

Six original manifests omitted a numeric process exit field. Their original
bytes remain unchanged: they are accepted only by their pinned original
manifest identity and the retained runner's successful-exit publication path.
This is explicitly indirect exit evidence, not a newly invented recorded0.
New captures require a recorded integer exit0. The registry retains original
manifest/raw-manifest hashes and completion sentinels; it is not regenerated
from C output. Thirty protocol/evidence tests now include31 provenance/schema
mutations, alongside the separate deterministic C regression.

## Canvas and visual evidence

The previous Main overlay cleared16 lines, leaving the old value's shadow
visible after changing Simulation to Arcade. Main value cells now include
the full19-line shadow. All four rows compose upwards so the18-pixel row pitch
does not import a preceding value's shadow from another variant capture.
The raw-canvas helper matches all40 independently observed full VRAM images.

The Rules helper constructs all11 Boolean cells in the32x64 map, including
the second tilemap quadrant for lower rows. It clears all cells before painting
their overlapping shadows. Its ON source is the clean first Rules ON glyph
after the OAM bars; Options ON carried the preceding STEREO shadow and was
rejected. Factory Arcade's several OFF rows match the full native VRAM across
seven visits. Slider graphics remain owned by OAM.

Fresh synchronous native RGB and VRAM are retained under
`.analysis/setup-config-native-20260830/{faces-v1,main-v4}/visual_*` in the main
repository. `faces-v1/factory-main.png` and `factory-rules.png` are lossless
viewing conversions, not production assets. Actual C captures in the worktree
are `build/factory-main.bmp` (before shadow correction) and
`build/factory-main-fixed.bmp`. I inspected both against native factory Main.
Their background scroll phases differ; these screenshots are **not** reported
as whole-frame parity. No hash was rebaselined from the candidate output.

The existing asset pack still contains captured glyph/canvas resources. This
patch adds no production capture assets and does not resolve that provenance
violation. It verifies correct composition of the existing resources; the
original font/resource producer remains separate work.

## Reproduction and retained results

Build from the worktree using the explicitly supplied checkpoint pack; do not
extract or assume the old configured default is native factory state:

```powershell
.\build.ps1 -AssetPack C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\transition-ownership-20260830\nba95_assets_rules_return_candidate.pak
.\tools\run_setup_config_checks.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc' -AssetPack C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\transition-ownership-20260830\nba95_assets_rules_return_candidate.pak -OutputDir C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\setup-config\build\config-auditor-new
```

The output directory must be new. The runner compiles dedicated probes from
the current build objects, runs30 protocol/evidence tests and the separate C
regression, then executes the stable, adjustment and canvas native gates.
Retained candidate results: `build/config-final-v2/`. Earlier first-failure
adjustment report `build/config-adjustments-v1.json` remains available.
The source-labelled Ghidra decode is
`.analysis/setup-config-native-20260830/preparation/complete-input/setup_config_bank81.txt`;
recomp references are `reference/bank81-with-repeat.c` and `reference/bank82.c`.

New immutable fixtures and SHA-256:

| Fixture | SHA-256 |
|---|---|
| `setup-config-input-native-witnesses.json` | `3150105ed26c5455e1b6050b679686f7a25b123f831e79c35308bf8e61dd0cd3` |
| `setup-config-faces-native-witnesses.json` | `fd7ef6387e0957f2e7db7d3b795c414eeaa8e96851cfd76542173ce5e38fe5c2` |
| `setup-config-main-visual-native-witnesses.json` | `1a21d6c113993e4cfea995a1907e876127b391077940aea1f169a008846607f9` |
| `setup-config-manifest-witnesses.json` | `dc6bbc1cc202807aebd9f780395bd46f4ee0ecac54425d2deac517f557d8b34b` |

## Pending integration and game behavior

Root owns adapting legacy CLI telemetry and configured transition journeys:
they must explicitly drive Simulation/3-minute settings through real menus or
declare a controlled prestate. Factory defaults must not be reverted to keep
historical C hashes passing. The full regression/endurance suite and combined
transition/configuration integration have not yet passed for this candidate.

Session Custom retention is implemented; disk save validity/serialization and
fresh-process reload are not. Original SRAM team/control fields require the
gameplay owner's mapping. Options' live audio preview/gain/mode commands and
their intended match consumers still need completion. Backcourt, Traveling,
Three Seconds, Inbound Clock, Half Court Clock and Injuries were identified as
missing/unverified consumers in the ownership audit; preset correctness does
not prove them. Per-frame cursor-scroll animation and input consumption during
all builder/return boundaries remain separate from these stable/call gates.
Hidden button sequences are inventoried and deferred until core playability.
