# Gameplay Sprite Jitter Audit

## Native census

Fresh headless Ghidra output and the union of gameplay execution traces bound
the final player-placement area as follows:

| Native slice | Purpose | starts | observed | C status |
|---|---|---:|---:|---|
| `$87:A3BB-$A3DC` | integer actor/camera projection | 18 | 18 | exact portable helper and native witnesses |
| `$87:A3DF-$A43B` | player culling/controlled-player route | 39 | 34 | exact result; five high-jump starts statically translated, not observed |
| `$87:A357-$A3B4` | queue/effect prelude | 39 | 20 | outside player-origin jitter |
| `$87:A43E-$A479` | ball presentation setup | 24 | 20 | outside player-origin jitter |
| **bounded `$87:A357-$A479`** |  | **120** | **92** | **28 unobserved; 5 in statically translated player culling** |

The broader `$87:A47A-$A98D` layer-selection census still has 92 unobserved
rare presentation/effect starts documented by historical native appearance evidence in Git history. The ordinary lower/upper/head/
number compositor and live animation resource paths used here already have
native evidence; those rare branches were not relabeled as jitter work.

## First divergence and fix

The first comparable court frame already diverged before any CPU decision:

| value | ROM | old C |
|---|---:|---:|
| actor world X/Y | `8 / 3` | `8 / 3` |
| camera Y | `-124` | `-124` |
| player screen Y | `122` | `123` |

`$87:A3BB-$A3C9` subtracts X from Y and performs two sign-preserving rotates,
which is floor division by four for a negative value. The old C expression
used signed `/ 4`, which truncates toward zero. It also rounded the combined
fixed-point actor position before rendering even though the native draw path
reads actor integer words `+$04/+$08/+$0C` directly.

`nba_court_project_actor` now reproduces the signed 16-bit arithmetic and is
shared by player rendering, ball rendering and screen-coordinate telemetry.
`nba_court_actor_visible` reproduces the native CPU and controlled-player
rectangles so actors do not pop at the port's former approximate bounds.
After the fix, all eight players submitted by the native opening frame match
the port exactly at their screen origins; the other two are intentionally
excluded from that pre-tip native submission list.

## Consecutive-frame OAM cadence

Sparse proof frames hid a second, independent presentation error. A fresh
Mesen capture saved every frame from 180 through 620 and traced every
`$87:A47A` submission. Native player OAM coordinates persist between scheduled
draw passes; the draw normally occurs on alternating frames, with occasional
NMI-split submissions that resume without clearing the previous OAM image.

The port previously reprojected every actor on every 60 Hz host render. Its
camera pass and actor pass occupy opposite cadence phases, so that recompute
created 51 small A -> B -> A screen-origin reversals in frames 180..620 even
though the underlying actor motion was monotonic. The same detector finds zero
in the native trace.

`latch_player_screen_origins` now stores the complete player origin/visibility
result on the native due actor/presentation pass. The renderer and telemetry
retain it on the intervening camera-only frame, matching persistent SNES OAM.
The identical port window now also contains zero A -> B -> A reversals. This is
presentation latching, not invented interpolation or a change to game physics.

## Verification

- `court_runtime_probe` checks native negative-remainder projection witnesses,
  exact half-open visibility edges, 16,000 gameplay updates, 522 full viewport
  renders across all 29 teams, and all four period-side scenarios.
- The 63,800-frame CPU regression retains gameplay/appearance/physics guards.
- Five reviewed screenshot anchors were refreshed only after their changed
  pixels were confirmed to be the corrected native player origins.
- The CPU regression examines every consecutive player origin from frames
  180..620 and rejects any small A -> B -> A reversal. The captured before/
  after counts are 51 and zero; the native count is zero.
- Before/after 60 Hz videos live under `.analysis/jitter-audit-20260828/`.

The ROM itself advances player logic on its scheduled cadence rather than
interpolating host pixels. No invented smoothing was added. Any remaining
two-frame hold is therefore evaluated separately from erroneous one-pixel
origin wobble.

## Honest remaining caveats

- The controlled-player off-screen indicator routine `$87:A846` is not part
  of the player sprite origin and remains a separate presentation task.
- Twenty-eight bounded starts remain unobserved: 19 in the optional queue/
  effect prelude, five in the statically translated high-jump culling branch
  `$87:A423-$A42D`, and four in the ball-side attribute branch
  `$87:A45F-$A466`. None is hidden behind a false execution-coverage claim.
- This closes the bounded placement error, not every rare branch in the broad
  `$87:A47A-$A98D` draw-preparation routine.
