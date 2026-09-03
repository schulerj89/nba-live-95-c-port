# Basket raster correction

The north/right basket no longer leaves a second backboard in the painted
lane as the camera moves. The opposite basket also retains its complete
support instead of losing everything below a fixed screen row.

## Reproduction and cause

Direct Tipoff, Orlando home, frame 1903 reproduces the defect at camera
`(200,-201)`. The old renderer submitted 172 extra BG1 pixels in that frame.
The 24-frame north sequence, 1891–1914, contained 5,023 extra pixels in total.
`build/hoop-20260902/north-basket-before-after.png` compares the same frame,
actors and camera before and after the correction.

The renderer had generalized scanline **123** from one historical Mesen
witness at camera Y=-220. The original game computes the split every frame.
At Y=-201, `$087E=-361`; `$00FF-$087E=616`, and the SNES nine-bit vertical
timer selects line **104**. Keeping BG1 on until row 123 exposed the wrapped
copy in its 256-row tilemap. This was a raster-state bug, not damaged artwork
or incorrect rim/net coordinates.

Fresh headless Ghidra and snesrecomp source generation cover seven ranges
across banks 80, 85 and 87. The decisive instructions are:

- `$80:8428-$8460`: north basket, initial TM selection and
  `$4209 = $00FF-$087E`; `$85:EF2E-$EF39` disables BG1 at the interrupt.
- `$80:8462-$849E`: opposite basket, initial enable at `$0004-$087E`.
  `$85:EEEE-$EF13` enables BG1 and schedules the next interrupt 76 rows later.
  `$85:EF14-$EF2D` narrows WH3 by 55, clamping negative results to zero.
- `$85:8E28-$8EE5` and `$87:A73B-$A845`: existing placement, OBJ resource
  submission and window publication. The rim/net resource remains unchanged.

`nba_court_goal_scanline` represents those per-row controls. The existing
indexed BG1 sampler now applies them over the full 224-row scanout. The
asset pack and gameplay simulation are unchanged. The complete 2,800-frame
old/new gameplay search traces are byte-identical.

## Verification

Evidence is retained under `build/hoop-20260902/` in the regular checkout.

| Gate | Result |
|---|---|
| Fresh Mesen CPU game, controller inputs only | 24 consecutive frames at each basket, no state injection |
| Independent reconstruction from VRAM/CGRAM/OAM and measured TM/WH3 writes | All 2,752,512 pixels match the 48 native scanouts |
| C raster helper versus measured native inputs and writes | 10,752 rows, 21,504 control values, zero differences |
| Old executable under the new smoke | Fails: 5,023 extra north pixels and 18,286 missing opposite-basket pixels |
| Direct Orlando smoke | 48 frames, zero missing, extra or incorrectly colored BG1 pixels |
| Direct New York smoke | 48 frames, same gate passes on the standard court layout |
| Menu route on the rebuilt executable | Nine verified button presses; 48 frames, zero pixel failures |
| Dribble regression | 68 frames, 134 physics and 67 draw replays pass; all 68 PNGs match the previously reviewed build |
| Existing Tipoff and court-asset tests | Pass |

Every frame in the three successful hoop sequences and the native sequence
was visually reviewed in contact sheets. Each run retains individual PNGs,
BMPs, winning-layer masks, input traces, gameplay traces, a slowed GIF and
a JSON report. The native scanout test and the C gameplay test are separate
gates: this does not claim matching whole-game trajectories or player poses
between the port and Mesen. Native scroll/window uploads can lag the working
camera, and the second interrupt can read an updated window source; the
fixture preserves the observed input at that handler rather than inferring
it from a screenshot.

The frozen numeric fixture is `tests/fixtures/hoop-raster-native.json`, SHA256
`db1b5c2985b64570ab482be6203929f5e6dfcb17c0466a421c9421973483ca4b`.
It records source inputs and raster writes, not captured artwork. The smoke
also checks that every frame exposes at least 50 expected basket pixels, so
disabling the layer cannot make the test pass.

## Repeat the quick smoke

```powershell
.\tools\run_hoop_smoke.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc'
.\tools\run_hoop_smoke.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc' -NoBuild -ThroughMenus
```

Direct Tipoff uses neutral controllers and naturally reaches both basket
views. The menu route presses through team/player setup into a Chicago–New
York game. The measured runs took about 11 and 17 seconds respectively,
excluding compilation. `--home-team` on `test_hoop_smoke.py` configures the
direct route's home court.

`capture_hoop.py` regenerates the native evidence in an isolated portable
Mesen. `verify_hoop_raster.py --native <capture> --fixture <new-output.json>`
reconstructs every scanout and checks fresh numeric cases without overwriting
the frozen fixture. `regenerate_hoop_reference.py` regenerates Ghidra and
snesrecomp listings from the verified USA ROM; generated C is source
cross-checking, not execution of a complete recompiled game.
