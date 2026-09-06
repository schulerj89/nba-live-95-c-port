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

The host game now owns one zero-allocated 128 KiB canonical WRAM buffer and
binds borrowed views to the renderer and each active Tipoff scene. The bytes
persist across scene-union clears and new-match initialization and are released
on shutdown. This is lifecycle support for future graphics publication; no
native queue producer, consumer, or human action routine is credited by it.

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
- the complete native control-mode-two defensive parent, including exact
  countdown/reload, role pursuit, context/target routing, CPU and human
  controller gates, ordered arrival/acceleration/pose work, jump/reach gate,
  and final requested-direction commit;
- full 16-bit actor controller and paired-role words at the mode-two boundary,
  including the native wrapped signed `$8000/$8002/$8003` target split;
- native preservation of the unrelated mode-two behavior cadence and exact
  B37C lower-channel phase and scratch publication after defensive posing.
- the native control-mode-four role-flag gate for locomotion-base repair.
- the native control-mode-four role-flag gate between loose-ball pursuit and
  defensive-target refresh.
- the native control-mode-four base-assignment selector for defensive matchups.
- the native control-mode-four anticipation override for close and far
  matchups, including the tied/ahead team pose-contact comparison, its two
  extra RNG steps, animation choice, shared movement/distance gates, and the
  predicted mode-nine handoff.
- the native control-mode-six role-flag gate for locomotion-base repair.
- the native control-mode-six role-flag gate between loose-ball pursuit and
  paired defensive targeting.
- the native control-mode-six call into the shared defensive anticipation
  policy, including its reject and mode-nine handoff paths.
- the native control-mode-six base-assignment selector for paired defensive
  targeting, even while the mutable assignment names another opponent.
- the native control-mode-six negative role-result hold for human-controlled
  actors during ordinary live play.
- the native control-mode-seven dead-ball hold, including exact live-state,
  integer-height, and signed-timer gates, CPU origin steering, animation and
  direction-latch branches, and its post-global production dispatch order.
- the native control-mode-eight knockdown recovery, including fractional-Z
  landing, signed timer and presentation phases, actor-group restore, exact
  two-channel contact animation publication, and late cooldown/dispatch order.
- the native control-mode-nine timed target override, including late signed
  recovery countdown, signed timer boundaries, saved-target and final-window
  velocity selection, saved-mode restore, and post-global animation timing.
- the native control-mode-ten receiver, including mixed controller/actor table
  normalization, signed-wrap timer boundaries, complete pass cleanup, exact
  live-state handling, restore, post-global scheduler timing, and the shared
  30-Hz dead-ball behavior boundary required to keep receiver and passer
  progress synchronized.
- the native control-mode-twelve shooter parent, including ownership and pump
  gates, raw-pose ball attachment, human and CPU release thresholds, exact
  direction ownership, complete launch publication, post-global scheduling,
  and the directly calling free-throw state-nine handoff.

- the native control-mode-thirteen close-finish parent, including retained
  ownership, signed timer boundaries, pack-backed facing, pose/ball attachment,
  airborne fallback, terminal release, landing, and post-global scheduling.
- the native control-mode-fourteen special receiver, including exact signed
  timer boundaries, grounded queue setup, wrapped airborne disruption,
  ownership and relationship gates, pose/ball attachment, pass acquisition
  deferral, and post-global scheduling.

These paths are protected by test_tipoff_court_smoke.py,
test_dribble_smoke.py, test_hoop_smoke.py, test_oob_smoke.py, and
test_cpu_reaction_smoke.py, plus their native-vector verifiers, including the
six-case defense-context, eight-case mode-five, two-case mode-one and
mode-three role-flag, the 43-case complete mode-two parent, twelve-case
mode-four, and ten-case mode-six actor-parent replays, plus the nineteen-case
mode-seven,
  nineteen-case mode-eight, sixteen-case mode-nine, eighteen-case mode-ten,
  thirty-seven-case mode-twelve and mode-thirteen direct replays, the
  fifty-one-case mode-fourteen direct replay, and their full-scheduler phase
  checks, plus four free-throw mode-twelve caller witnesses.

## Major remaining gaps

- Ordinary human offense, defense, passing, shooting, inbound control, and
  controller ownership are not complete production paths. Bounded human
  helpers exist but do not make a normal matchup fully playable.
- Canonical graphics WRAM has a production lifetime owner, but the native
  jersey appender `$80:AD2B-$AD88`, remaining ordered producers/consumer, and
  `$012C` provenance required by `$87:B7D8` remain unimplemented.
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
| inside evidence-eligible verified ranges | 11,902 | 40.4% |

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
System workflow documentation allowed by AGENTS.md, such as
[docs/cpu-logic.md](docs/cpu-logic.md), records durable dispatch and evidence
boundaries needed to maintain the port.
