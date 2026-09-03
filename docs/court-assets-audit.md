# Gameplay court asset audit

Audited 2026-08-28 against the original ROM with headless Ghidra and a fresh
1,000-frame Mesen CPU-vs-CPU trace.  Evidence is under
`.analysis/court-assets-audit-20260828e/`, with the decisive PPU layer dump in
`.analysis/court-assets-ppu-20260828/`. Emulator screenshots are comparison
evidence only. The pack stores structured 4bpp/map/palette state, not frames.

## Component census

Updated 2026-09-02: the original audit's map witnesses covered Orlando's
parquet branch. The [home-layout correction](court-logo-complaint-audit.md)
adds the standard `$A0:BC26` map, its shared CHR and native fan relocation.
The historical viewport counts below did not independently establish the
missing standard branch; the new every-frame smoke covers all 29 home teams.

| Component | Native ownership | Port/pack status | Pending decoded instructions |
|---|---|---|---:|
| Center-court logo and floor paint | `$85:8EE6-$90C3` streams the 148x52 column-major map from `$A0:8006`; its tile graphics are the gameplay BG2 CHR set | Complete in asset 273 (`NBCOURT2`, 29 x 1184x416) and raw map asset 279; 522 viewport checks and 12 native map witnesses pass | **0** in the already-censused map/render scope |
| Sidelines, baselines, lane/arc markings and stands rail | Same BG2 map/CHR layer as center court | Present across the full panorama, including both camera extremes; no separate sideline compositor was observed | **0** in the already-censused map/render scope |
| Backboard, rim, support and net | `$85:8E28-$8EDC` selects BG1 scroll/window and the basket record. `$87:A73B-$A7D2` projects/mirrors OBJ resource `$0822/$082C-$082F`; `$87:A7D5-$A845` submits objects and the window | Implemented in asset 282 (`NBGOAL2`) plus raw descriptor closure in asset 256. C applies BG1 `$087C/$087E`, clips `$0882..$0880`, then composes the OBJ rim/net | **0** in the bounded 114-instruction presentation scope; four alternate starts remain trace-coverage caveats, not missing C branches |
| Crowd/fan graphics | BG2 map entries use dynamic tile IDs 808-820 and 849-863. The shared `$80:82A3` PPU queue updates their physical CHR destinations `$5280-$55FF` during play | Implemented as asset 283 (`NBCROWD1`): three native 4bpp tile states, applied only to those 28 IDs while retaining map palette/flip bits | **Producer census remains open**; renderer/upload behavior is implemented, but shared producer instruction attribution is not claimed |

The former **114 known presentation instructions are now implemented**. The
only unbounded item in this audit is exact attribution of the shared fan queue
producer; it is not counted as completed reverse-engineering coverage.

## Goal proof

The goal is split across BG1 and OBJ, not part of the static BG2 panorama:

The [2026-09-02 basket raster correction](hoop-raster.md) fixes a later
runtime regression in that composition: the historical row-123 TM witness
had been applied as a constant. BG1 now follows the ROM's camera-dependent
enable/disable and window-narrowing interrupts at both baskets.

- The 12-entry on-court render list contains ten players plus the ball record
  at `$3EEB` and the visible basket record at `$3FEB`.
- `$87:A73B-$A7D2` reads the current effect resource from `$4015`, projects the
  basket record, mirrors it from the sign of basket X `$3FEF`, and submits the
  primary and supplemental resource groups.  Base resource `$0822` and net
  states `$082C-$082F` were observed naturally.
- `$87:A7D5-$A81B` drains the two ordered object queues through `$80:B346`.
  `$87:A81D-$A845` copies `$087C/$087E/$0878/$087A/$0880/$0882` into the OAM
  window state and commits it through `$80:AC89`.
- The 1,000-frame execution census observed 62/66 goal-composition starts and
  all 48 submission starts.  The four unobserved starts are the alternate
  off-screen/orientation calculation at `$87:A76F-$A776`; they remain pending
  and must receive controlled witnesses during implementation.

- Fresh PPU state proves BG1 map word `$0000`, CHR word `$1000`, Mode 1.
  BG1 h/v scroll exactly equal `$087C/$087E`; its active window edges are
  `$0882/$0880`. This layer owns the rigid board/support. OBJ owns rim/net.
- C uses the projected `$3FEB` basket origin to select the matching BG1 goal
  copy and excludes the layer's staging copies, then renders `$4015` through
  the same `$80:B344-$B498` descriptor compositor used for players.

The current C effect engine verifies `$87:A9E3-$AA01` starts and
`$87:AA02-$AAB1` steps. No host-drawn goal substitute or duplicated rim
physics is used.

## Crowd/fan proof and boundary

The existing static panorama is geometrically correct but cannot reproduce
all live crowd pixels.  Native map entries which differ from the static pack
are tile IDs 808-820 and 849-863.  At a 4bpp BG2 base of `$4000`, those tile
IDs map to the same physical CHR destinations observed in the live queued
uploads (`$5280-$55FF` in the PPU word-address trace).

The fresh trace records updates throughout play, including 16-word tile
updates and 192/240-word blocks.  `$80:82A3` is only the shared queue consumer;
counting that entire helper as a "fan animator" would also count player and
goal streaming.  The next crowd audit must trace the queue producer and ROM
resource descriptor for each write, then split fan-owned selection/cadence
from shared upload glue.  Until that split is made, the honest instruction
count is uncensused rather than zero.

## Regression boundary

`tools/test_court_assets.py` locks pack v31, the complete goal BG1 payload,
all five basket/net descriptors, all 28 fan tile IDs and distinct animation
states. `tools/test_court_presentation.py` continues to lock the complete
center/logo/sideline panorama and both camera extremes. Right-goal visual
proof is `.analysis/court-assets-port-final-right/frame_2733.bmp`.

The runtime no longer uses framebuffer call order as a substitute for SNES
priority. `nba_snes_ppu` resolves Mode-1 BG1/BG2/BG3/OBJ candidates, both BG
tile priorities, all OBJ priorities, OAM ties and the inverted BG1 basket
window. It also preserves the SNES OBJ quirk by selecting the lowest-index
opaque OAM pixel before comparing that object's priority with BG. The settled
frame proof `.analysis/ppu-mode1-verified.jsonl` reports
2,098 winning BG1, 51,845 BG2 and 3,401 OBJ pixels. The paired BMP is
`.analysis/ppu-mode1-verified.bmp`; generated evidence is not packed art.

Pack v31's `NBPPUIN1` input stores each home team's indexed gameplay VRAM and
CGRAM. The runtime now submits the native BG2 court, BG3 scoreboard/HUD,
backdrop, raster-limited BG1 goal, and goal OBJ through the Mode-1 compositor;
only composed player objects remain predecoded direct-color inputs. The exact
frame-989 gate compares 54,688 non-player pixels and all 182 pixels belonging
to the native goal OAM slots 33/34 with zero mismatches. Run
`tools/test_runtime_ppu_inputs.py` directly, or `tools/verify_ppu_parity.ps1`
to recapture the Mesen witness before testing it.
