# Project status

The full game remains incomplete. The current playable path covers the
Nintendo license, legal and EA intro screens, title, Game Setup, Exhibition
Team Select, Player Setup, introductions, tipoff, and sustained
CPU-versus-CPU gameplay.

This file is the maintained product-level status. Production behavior is
defined by src/ and include/; ROM boundaries and important limitations are
recorded beside the implementation. Native fixtures, verifiers, and headless
frame tests under tools/ and tests/ are the executable proof.

## Current runtime

Implemented production behavior includes court and camera movement, ten-player
rendering, possession, passing, shooting, dribbling, rebounds, fouls, free
throws, clock and period transitions, timeout/resume support, out-of-bounds
presentation, and a structural final-game flow. Graphics and audio are derived
from the verified US ROM through the asset pack.

Recent source-verified visual and gameplay fixes include:

- all 29 selected home-team court layouts and center logos;
- native dribble animation timing and ball attachment;
- north and south basket raster clipping;
- the original out-of-bounds violation and possession overlay;
- native CPU role-rebuild reaction delays, flag clearing, and RNG order;
- native late-game CPU defense-mode selection when consuming a play request;
- the native control-mode-five actor continuation, including its role-flagged
  loose-ball pursuit;
- native control-mode-one and control-mode-three loose-ball pursuit from the
  role-pass flag, independently of the possession record.
- the native control-mode-two owner/receiver gate for locomotion-base repair.
- the native control-mode-two signed half-court gate for decision-timer reloads.
- the native control-mode-two role-flag gate between loose-ball pursuit and
  defensive-target refresh.
- the native control-mode-two base-assignment selector for defensive matchups.

These paths are protected by test_tipoff_court_smoke.py,
test_dribble_smoke.py, test_hoop_smoke.py, test_oob_smoke.py, and
test_cpu_reaction_smoke.py, plus their native-vector verifiers, including the
six-case defense-context, eight-case mode-five, two-case mode-one and
mode-three role-flag, and eight-case mode-two actor-parent replays.

## Major remaining gaps

- Ordinary human offense, defense, passing, shooting, inbound control, and
  controller ownership are not complete production paths. Bounded human
  helpers exist but do not make a normal matchup fully playable.
- The strict native/C gameplay differential begins from different launch
  state, scheduler timing, and RNG history. Passing isolated routine vectors
  does not establish an equivalent whole-game trajectory.
- Season, playoffs, schedules, records, save/load, complete postgame
  statistics, and several non-Exhibition flows remain incomplete.
- Some Setup rule and option values have no complete gameplay consumer.
- Complete native frame and audio timing remains open for several transitions,
  menus, breaks, substitutions, and end-game scenes.
- Several exact raster and runtime checks are C regression anchors. Their names
  and comments distinguish them from native parity evidence.

## Evidence accounting

Current generated captured-address measurements are:

| metric | address positions | captured-address percentage |
|---|---:|---:|
| observed in retained execution captures | 29,438 | 100.0% |
| documented by source provenance | 29,101 | 98.9% |
| inside evidence-eligible verified ranges | 11,588 | 39.4% |

These are coverage measurements for retained captures. They are not a
percentage of the ROM, retail features, or game completion. The generated
[progress report](docs/progress.md),
[full-ROM instruction census](docs/full-rom-instruction-census.md), and
[verified routine ledger](docs/verified-routines.json) are the authoritative
artifacts for these measurements.

## Build and verification

Build the regular executable with:

~~~powershell
./build.ps1
~~~

Run the complete configured regression suite with the verified ROM and asset
pack:

~~~powershell
./build.ps1 -RomPath '<path-to-rom>' -AssetPack 'build/nba95_assets.pak' -Test
~~~

Focused headless smoke tests capture frames inside the renderer and retain
their output under ignored build/ directories. Native Mesen captures and other
private reference material remain under ignored .analysis/ directories.
[tools/README.md](tools/README.md) lists the maintained capture, replay, and
census entry points.

## Documentation policy

Plans, dated checkpoints, task audits, migration notes, and screenshot reports
are not maintained in the working tree. Git history remains their archive.
When behavior changes, update the implementation comments, executable tests,
this status, and the generated evidence artifacts that actually changed.
