# Dribble animation and ball placement

The September 2 follow-up fixes three independent causes of the ball looking
detached from the dribbling hand. The asset pack is unchanged.

| Symptom | Original source | Production correction |
|---|---|---|
| Ball returns to hand height during the falling pose | `$85:A50D-A516` reads the owner's literal `+$3A` phase before comparing against 3 | Owned-ball substeps consume `rom_upper_animation_phase_raw_3a`, rather than the legacy action/contact counter. |
| Ball appears below and beside the projected hand point | `$80:B10C-B11F` submits resource `$081D`; its descriptor at `$9B:9C16` has tile offset `(-3,-4)` | Render the existing ROM sprite descriptor at the projected origin. |
| Ball covers fingers that should be in front | `$80:AF1E-B0AB` interleaves the ball using signed `$AC:BAFF[upper]` and `$3F31` | Include the ball in the owner's ordered pose submissions, using the carried twelve-record draw list and native priority bits. |

The renderer handles the ordinary low-resource mode-11 owner with
`$4015 < $082C`. Negative table entries submit the ball after the upper/number
group when the sorted ball record was already visited, or after the body
otherwise. Nonnegative entries submit it first. The existing `NBPDRAW1`
capture includes the needed bytes of `$AC:BAFF`; no new palette, tile or
attachment offset is invented.

`NbaTipoff.draw_order` retains `$7E44` across scheduled submissions. Its depth
is the wrapped arithmetic `(Y-X)>>2`, minus camera Y, without ball height.
Initial and period placement use the original FBFF gap sort; ordinary object
origin latches use one FC80 reverse adjacent pass. Equal keys do not swap in
that ordinary pass. A new period sorts the carried list without resetting it
to identity. Existing player and ball screen coordinates still latch together.

## Quick visual check

From the regular checkout on `main`:

```powershell
.\tools\run_dribble_smoke.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc' -TipoffOnly
```

The script builds `build/nba95_port.exe` and the two native-vector probes.
It configures the normal CPU tipoff, advances headlessly to a visible full
dribble, then deterministically replays that journey while capturing every
frame around the bounce. `-NoBuild` skips rebuilding the game; omit
`-TipoffOnly` to press the setup/team/player buttons inside the test instead.
Neither route controls the desktop or injects a mid-dribble snapshot.

Each output directory includes an HTML gallery, a quarter-speed GIF, contact
sheets, individual BMP/PNG frames, winning-layer masks, input traces,
gameplay traces and a report with executable/pack/ROM/frame hashes. The gate
requires phases 0 through 7, a floor bounce, a return to hand height, continuous
frames and identical state between search and rendered replay. The default
menu route also verifies all nine held/pressed/released button transitions.

Ball pixels are independently decoded from the original ROM descriptor and
palette `$AF:E99F`, and checked at the retained projected origin. Displaced
pixels must be covered by another OBJ. Coverage also requires a completely
visible phase-4 bounce and a partial OBJ overlap during a hand phase; drawing
the ball on top of the hand in every frame fails that coverage check.

The reviewed regular-build runs are under `build/dribble-20260902`:

| Route | Captured frames | Full dribble | Runtime, excluding build |
|---|---|---|---|
| `verified-buttons` | 4083–4138, all 56 frames | actor 4, 4091–4130 | about 20 seconds |
| `verified-direct` | 2088–2155, all 68 frames | actor 7, 2096–2147 | about 15 seconds |

All 124 frames were checked, including the start/end margins. Together they
check 6,448 ball-pixel positions, including legitimate player occlusion.

## Original-game evidence

ROM SHA256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

`tools/regenerate_dribble_reference.py` produces fresh headless Ghidra
65816 listings and `snesrecomp` v2 generated C from the same verified ROM.
It includes the owned-ball driver, both attachment composers, animation
cadence/reversal, player/ball submission and both draw sorts. Ghidra's
`ctx_MF`, `ctx_XF` and `ctx_EF` are explicitly zero. Source bytes, tool hashes,
commands and outputs are recorded in a manifest. The reviewed complete run
is `build/dribble-20260902/reference-accepted-20260902`. The generator requires
the key instruction addresses to appear, including the separately seeded
`$87:B66A` attachment wrapper and `$80:FC69` depth helper.

The generated C is a second source representation, not an executed full-game
recomp. Actual original-game execution comes from private portable Mesen,
normal cold boot and controller input. The dribble capture does not patch
ROM, WRAM, registers, PC, stack or saved state. `tools/capture_dribble.py`
and `tools/mesen_dribble_capture.lua` retain native RGB frames, OAM, register
arguments and routine-boundary words. The 480-frame capture contains 139
dribble frames; all of those dribble frames were visually reviewed.

The committed fixtures retain ROM, emulator, capture-script and raw-call
hashes. `tools/verify_dribble_vectors.py` pins their exact file identities:

* `dribble-native.json`: 67 natural calls from `$85:9A37` to `$85:A7C7`,
  covering all eight literal phases. All 31 captured output words match,
  including fractions, velocity, history, attachment state and events.
* The same calls are replayed with the legacy counter deliberately placed
  on the wrong side of the phase-3 gate: 134 exact physics replays total.
  The previous implementation fails all 67 poisoned-counter cases.
* `dribble-draw-native.json`: 67 actual `$80:AF1E` calls with ordered `$80:B348`
  submissions through `$80:B0AB`. Every resource, attribute and X/Y word
  matches, including both observed `$3F31` branches and four/five-part poses.
  These natural calls cover negative draw-order entries; the positive-entry
  branch is established by the original code.

The previous released executable also fails the new pixel smoke at frame
3378, with exposed ball pixels at the wrong position. This negative control
is retained under `negative-previous-release`.

These checks establish bounded routine and rendering behavior. They do not
assert identical whole-game trajectories, full-frame emulator/port equality,
native graphics-queue allocation, DMA or interrupt timing.

## Surrounding regression checks

The existing source draw-order test passes 5,668 cases against original
opcodes; the native test passes 37 cases/888 words plus the carried sequence.
The new full-sort function matches all 96 order/depth words in four earlier
controlled period-expiry captures. Those period captures use declared expiry
seeds and are distinct from the unseeded dribble capture.

The player compositor passes 43 native geometry cases, 149,632 controlled
source cases and 9,976 legacy compatibility cases. The 20,000-frame gameplay
attachment run passes 1,059 dynamic observations, 21 reversals and seven
preserved-resource reversal witnesses. Draw preparation passes 2,000 calls;
period initialization, expiry and overtime regression probes also pass.

The older owned-ball suite passes with its already-declared single
cross-frame `$13E7` event caveat. The new 67-call dribble fixture has no field
exclusions. The unrelated historical whole-suite failures in the Mode-1
layer census and CPU gameplay image anchors are not claimed resolved here.

The tipoff image references were compared to the previous released executable
with both literal and fallback packs before updating them. Frame 90 is
identical. Frame 170 changes exactly 90 ball pixels in rectangle
`(177,19)..(187,30)`; frame 220 changes exactly 100 in
`(200,102)..(210,115)`. Every other pixel remains identical. The paired images
and hashes are retained in `build/dribble-20260902/tipoff-images`.
