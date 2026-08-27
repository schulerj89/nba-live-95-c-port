# Camera initialization and post-tip handoff

Baseline: main b331c01, 2026-08-27; 7206/27901 captured addresses verified
(25.83%). This implements the 99 camera correction/caller instructions from
the preceding report. The frame-based tip outcome itself is not silently
included in that count: trace its influence, identify any necessary bounded
connector, and report what remains instead of claiming complete tip-off parity.

| Slice | ROM | Instructions |
|---|---|---:|
| Initial subject copy and placement | 85:8B98-8BBE | 13 |
| First update/history | 85:9192-91CA,9349-9351 | 27 |
| Edge fraction clearing | 85:91DF-91FA | 12 |
| Basket orientation | 85:9219-922F | 9 |
| No-team centering | 85:92C0-92C9 | 4 |
| Alternate height branch | 85:92E4-92F8 | 8 |
| Actor/ball resolution | 87:A9D0-A9E2 | 9 |
| Presentation cadence | 87:95AC-95BA | 5 |
| Subject copy and dispatch | 87:95BB-95DE | 12 |
| Total | | 99 |

Plan: fresh Ghidra labels/census and recomp comparison; capture actual initial
placement, per-call inputs/outputs and early post-tip camera frames; add
controlled native cases for missing branches; translate portable state and
caller binding; replay and retain witnesses; inspect before/after port and
native footage; run regressions; update precise coverage, rebuild/refresh the
desktop and commit/push the verified checkpoint. No capture art at runtime.

Acceptance distinguishes correctness of a routine for native inputs from
full-game trajectories. A frame-based tip winner/ball motion can still feed
different camera inputs even when every camera instruction is translated.

## Result and exact behavior

All 99 requested starts are translated and appear in native call witnesses.
The runtime now uses the raw camera interface in `nba_gameplay_camera.c`:

- `$85:8B98-$8BBE`: copy selected actor/ball XY and clear initialization;
  `$85:9192` directly places the first camera without easing, preserving old
  position history and commanded-step words. Later calls update that history.
- `$85:91DF-$91FA`: exact integer court edges clear scratch fractions,
  not the persistent proxy. Projection retains both 16-bit fractional words.
- `$85:9219-$922F`: choose look-ahead from the selected basket anchor's sign;
  `$85:92C0-$92C9`: a negative team selector centers on projected X minus 128.
- `$85:92E4-$92F8`: use ball-height framing for live-state 1 or alternate
  `08BC!=0 && 08CC==1`. Height comes from BALL `$3EF7`, never actor Z.
- `$87:A9D0-$A9E2`: actor pointer lookup for selectors 0..9; negative means
  pointer 0/ball. The host guards invalid positive selectors; those are not
  claimed equivalent to out-of-table native memory reads.
- `$87:95AC-$95DE`: resolve before waiting for unsigned presentation ticks
  `0564>=2`, discard surplus credits, then copy the latched record's XY.
  The `$85:8E1C` second resolver may change 0940 but cannot replace the copy.
  The C host yields rather than emulating an IRQ busy-wait. Scene entry seeds
  one presentation credit to match the observed initial due phase; subsequent
  outer updates supply credits. This is not NMI/cycle/scheduler parity.

Removed camera-specific frame 200 enable/frame 220 subject substitutions.
The separate upstream tip sequence still supplies approximated ownership.
The old 8.8 camera adapter remains only for older probes; gameplay uses the
explicit subject, basket, flags, state and ball-height inputs.

### The post-tip vertical bug

Fresh Ghidra and natural Mesen writes agree:

```text
$86:E1A6  LDA #$0081
$86:E1A9  STA $0936       ; pre-tip state
$86:D392  LDA $09B6
$86:D395  BNE $D39D       ; preserve state if whistle active
$86:D397  LDA #$0000
$86:D39A  STA $0936       ; acquired possession
```

Mesen memory-write callbacks report the following PC (E1AC/D39D), not the
store's starting PC. `state-writers/state_writes.jsonl` records 0081 at frame 0
and 0000 at frame 220. The old C bridge left 0001 installed, selecting ball-height
framing. `nba_tipoff_init` now uses 0081; `cpu_begin_possession` performs the
guarded clear. These six writer instructions are documented dependency fixes,
NOT extra verified-ledger credit for their entire dispatchers.

## Evidence and regression boundaries

Local evidence root: `.analysis/camera-handoff-proof-20260827/`.
Fresh Ghidra labels/comments and gap-checked listings are in
`ghidra/camera_bank85.txt`, `camera_bank86.txt`, `camera_bank87.txt`;
`tools/ghidra/DumpCameraHandoff.java` reproduces them against the existing
`NbaLive95CpuGameplay85/86/87` projects, processing `bank_85/86/87.bin`.
The recomp camera reference is
`../NBA-Live-95-Recomp/.analysis/recomp_gameplay_extract/generated_violation_regen/bank85_v2.c`.

Full replay of `natural-final`, `controlled-final` and `init-actor-final`:

| Native call boundary | Replayed calls | Mismatches |
|---|---:|---:|
| Camera core | 3,877 | 0 |
| Initial placement | 3 | 0 |
| Actor/ball resolver | 50,055 | 0 |
| Subject copy | 4,164 | 0 |
| Presentation cadence | 4,144 | 0 |
| Total | 62,243 | 0 |

Controlled cases change saved/restored WRAM inputs at actual call boundaries;
they do not patch ROM, PC, flags or stack. Enclosing cadence witnesses tainted
by controlled resolver experiments are excluded. Expected outputs are from
Mesen, never the C implementation. Nine raw core outputs include history,
prior displacement, commanded steps and initialization. Copy/initialization
also compare the four-word proxy; cadence captures observed tick samples.

`tests/fixtures/camera-handoff-witnesses.json` retains 1,133 witnesses and
`camera-handoff-census.json` records the exact 99 requested PCs. New captures
observe 264/272 core PCs, not 272: the older independently replayed 212-slice
fixture supplies the remaining starts. The ledger keeps these proof sets
separate; it does not assert that one capture proves every branch combination.

`camera_handoff_runtime_probe.c` protects initial placement, state 0081 -> 0000,
pre-frame 200 dispatch, six actor-Z/basket-sign/alternate-flag scenarios,
latched-before-wait selection, second resolution, and surplus credit discard.
Debugger JSON now exposes 4A54/0940/0564/08BC/08CC and the four-word 4A56 proxy.
The shared court runtime probe remains a 16,000-frame/522-viewport check.
Only the frame 220 tip-off golden changes: corrected framing, no asset changes.

## Visual comparison and remaining work

Release recordings: `port-tip-camera.mp4` and `native-tip-camera.mp4`, each
frames 120-360 at 60 fps, nearest-neighbor enlargement, no camera interpolation
or capture art in the runtime. Source screenshots/frames are `port-frames/`
and `natural-final/native_*.png`. Inspected matching logical call example:
port frame 221 and native end-frame 222. Native PNG end-frame labels follow the
routine-entry label; do not silently equate them when comparing screenshots.

`camera-comparison.json` compares 19 native routine-call outputs from labels
201-239 against old/new C scene-frame labels, with no fitted frame offset:

| Absolute camera error (pixels) | Before mean / max | After mean / max |
|---|---:|---:|
| Vertical | 35.32 / 54 | 0.63 / 2 |
| Horizontal | 5.37 / 17 | 6.79 / 17 |

This is a bounded diagnostic sample, not whole-game or whole-frame parity.
Horizontal error did NOT improve. Removing the old camera gate exposes a
two-pixel left move at C frame 199; the native camera holds until 201. The
existing interpolated tip ball coordinates differ, and native call cadence
later shifts phase. Do not repair those differences by adding arbitrary
camera offsets or restoring frame gates.

Still pending:

- Replace quadratic tip toss/linear contact trajectory and forced receiver 8
  at frame 200/possession bridge at 220 with native event/physics progression.
- Separate upstream 093E camera/control selection from the current represented
  possession actor when the full dispatcher is ported.
- Integrate upstream writers of alternate 08BC/08CC flags; their camera
  consumers are implemented and tested, but gameplay currently initializes 0.
- Crowd CHR animation and downstream BG1/backboard/window composition.
- Previously reported 323 CPU/animation instructions, 9 timeout instructions,
  initial 43,200 clock seed and period/timeout integration remain separate.

Coverage: 7,206 -> 7,300 of 27,901 captured address positions, 25.83% -> 26.16%
(+94 positions/+0.33 percentage points), 146 ledger slices. This is not a
percentage of the whole game completed and not 99 new captured addresses.

## Reproduce

```powershell
.\tools\capture_camera_handoff.ps1 -OutputDir .analysis/camera-natural
.\tools\capture_camera_handoff.ps1 -OutputDir .analysis/camera-controlled -Controlled
.\tools\capture_camera_handoff.ps1 -OutputDir .analysis/camera-init -InitActor -Frames 600
.\tools\build_vector_probe.ps1 -Name camera_handoff_probe
python tools/verify_camera_handoff.py --probe build/camera_handoff_probe.exe --vectors tests/fixtures/camera-handoff-witnesses.json --require-census
.\tools\build_vector_probe.ps1 -Name camera_handoff_runtime_probe
.\build\camera_handoff_runtime_probe.exe build/nba95_assets.pak
.\build.ps1 -Test -RomPath 'F:/Games/SNES/NBA Live 95 (USA).sfc'
python tools/compare_camera_handoff.py --native .analysis/camera-handoff-proof-20260827/natural-final/camera_handoff.jsonl --before .analysis/camera-presentation-proof-20260827/port-video.jsonl --after .analysis/camera-handoff-proof-20260827/port.jsonl
```

The normal regression uses saved witnesses and does not require Mesen or
ignored local captures. The trajectory comparison requires the local captures
and is deliberately not an exact-match acceptance test.

## Release verification

`build.ps1 -Test` exited0: `full-test-release.log` under the evidence root.
This includes the new 1,133-call replay, camera/court binding, both200,000-frame
owner endurance runs, 63,800-frame CPU regression (2,005 exact pass frames,
91 automatic unlocks), asset regeneration/safety, menus, player lab/setup/
intro, tip-off, debugger, audio and license/legal/EA intro tests. Older logs
that failed obsolete source-name assertions are diagnostic history only.
The updated assertion requires the live raw camera step/place/ready functions;
it does not retain a dummy call to the retired adapter.

The release executable also passed all62,243 native replays in
`replay-release.log`. Release frames/recording were regenerated. The standard
OneDrive Desktop `NBA Live '95 (C Port).lnk` was refreshed and read back:
target `build/nba95_port.exe`, repo working directory, F-drive ROM and repo
`build/nba95_assets.pak`. The recomp shortcut was not changed.
Asset pack remains version29,87,836,941 bytes; no runtime artwork changed.
