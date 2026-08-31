# Reverse-engineering and asset tools

The 2026-08-30 [ownership audit](../docs/ownership-plan.md) found that maximum-speed
Mesen capture can skip PPU rendering and that `takeScreenshot()` may return a
previous presented frame. For consecutive-frame evidence, use the isolated
portable runner `capture_setup_transition_exact.ps1`, rendering skips disabled,
and synchronous `getScreenBuffer()` output. Its fixed seven-row SNES border
conversion retains all 256x224 active pixels. Historical PNG sequences are not
automatically valid frame-by-frame oracles.

Recent verification gates deliberately separate their claims:

- `verify_inbound_internal.py`: all represented internal outputs from 500 native
  inbound dispatches; includes pre-compensation and same-dispatch velocity.
- `verify_match_lifecycle.py`: twelve terminal words from four native expiry
  witnesses; the C adapter begins after the clock writer and skips presentation.
- `verify_new_match_reset.py`: a native first-court state projection plus two
  production C postgame-to-new-match journeys, not native whole-journey parity.
- `verify_ppu_brightness.py`: 1,536 controlled PPU/CGRAM cases with all rejected
  attempts retained; no natural scene-timing or color-math claim.
- `audit_brightness_golden_delta.py`: compares old committed Setup goldens to
  repaired output using the native brightness table for every pixel. Reports
  conversion-only changes; never rewrites expected hashes.
- `audit_menu_brightness_golden_delta.py`: applies that strict conversion-only
  check to the existing Options and Rules transition snapshots, rejecting any
  changed scroll state or unexplained pixel rather than authorizing a new hash.
- `run_differential.py`: now creates and verifies a private portable Mesen
  home through `mesen_portable.py`; its controlled launch and unsynchronized
  C/native baseline remain explicitly separate from natural menu journeys.

The corresponding `test_*verifier*.py` tests reject malformed, truncated, and
mutated evidence/probe output. A verifier protocol PASS is not gameplay parity.

## Reproducible pipeline

`capture_assets.ps1` runs these authoritative Mesen captures into ignored
`.analysis` directories:

| Script | Output |
|---|---|
| `mesen_intro_capture.lua` | legal and four EA intro reference frames, 303-frame motion oracle, and raw indexed E/A/SPORTS Mode 7 VRAM/CGRAM layers |
| `mesen_title_capture.lua` | title PPU/APU/cue state |
| `mesen_setup_capture.lua` | settled Setup VRAM/CGRAM and PPU state |
| `mesen_setup_transition_capture.lua` | Setup entrance VRAM deltas and cycle-timed APU writes; `NBA95_CAPTURE_MOTION=1` also saves frame oracles |
| `mesen_setup_menus_capture.lua` | Rules/Options VRAM/CGRAM/OAM, per-frame open/return VRAM writes and complete PPU layer states, WRAM commits and DSP sounds; set `NBA95_CAPTURE_MENU=rules` or `options`, `NBA95_CAPTURE_VALUES=1` for independent Music Mode/Crowd/Slow Motion/Shot/CPU Assistance canvases, `NBA95_CAPTURE_CALLS=1` for deduplicated JSR/JSL targets, or `NBA95_CAPTURE_EVERY_FRAME=1` for a lossless screenshot oracle |
| `mesen_setup_main_capture.lua` | Main Game Setup Mode/Style/Level/Quarter value cycles, exact BG3 VRAM states, `$7E:16FB` working values, and executing CPU paths |
| `mesen_team_select_capture.lua` | Start-only Exhibition Setup-to-Team-Select transition, settled PPU memories, WRAM writes, and execution ranges; asserts `$82:809A` scene entry, supports `NBA95_TEAM_NAV=1` for isolated navigation, and `NBA95_TEAM_ALIGNMENT=1` for GOLDEN STATE/PHILADELPHIA alignment and home-wallpaper evidence |
| `mesen_gameplay_player_capture.lua` | Drives through Player Setup into gameplay, records Player Setup raw PPU state/routine hits, then records executed player-loader paths and roster/appearance/palette reads used to prove the F9 Player Lab ROM extraction |
| `mesen_gameplay_audio_trace.lua` / `capture_gameplay_audio.ps1` | Reuses the verified CPU-vs-CPU driver and correlates `$80:9DF3` commands, `$2140-$2143` writes, DSP voice state, and gameplay event words; `-VariantProbe` forces all 60 randomized dispatcher cases as evidence only |
| `ghidra/DumpGameplayAudio.java` / `ghidra/Run-GameplayAudioAnalysis.ps1` | Labels `$82:FD65-$FF08`, dumps its nine command tables, and records the `$80:8930` RNG masks from headless Ghidra |
| `gameplay_audio_variant_probe.c` | Replays the 60 ROM table/RNG cases against production C and rejects command, SRCN, pitch, volume, or second-collision-bit drift |
| `mesen_tipoff_capture.lua` | Drives into live tip-off, records raw actor/ball/controller/camera/AI state and emits `gameplay_rom.jsonl` for `compare_gameplay_traces.py` |
| `verify_ppu_parity.ps1` | Captures a bounded native gameplay scanout and proves the production C PPU matches both an independent oracle and Mesen pixel-for-pixel |
| `snes_ppu_oracle.py` | Independent raw VRAM/CGRAM/OAM Mode-1 renderer used only as a differential oracle |
| `ppu_snapshot_probe.c` | Replays one captured raw PPU snapshot through the production C compositor |
| `mesen_player_intro_portraits.lua` | Drives the verified Exhibition path, state-walks Team Select to a requested team, verifies it again at `$87:BE92`, and saves five raw visitor or home portrait PPU states (`NBA95_INTRO_TEAM=0..28`, `NBA95_INTRO_SIDE=away|home`) |

`extract_assets.py` validates those captures and the ROM, then writes the asset
pack. Its minimal 65816 decompressor lives in `snes65816_decompressor.py`.
Pass `-CaptureName intro_capture`, `title_capture`, `setup_capture`,
`setup_transition`, `setup_rules`, `setup_options`, `setup_main`, or
`team_select_logos` to refresh
one capture while investigating it. The orchestrator supplies the menu-specific
environment flags and validates every extractor-required output before it
reports success.

## Regression tools

- `test_intro_sequence.py`: license, legal, and EA intro frames
- `test_title_pipeline.py`: title hardware assets, cues, pixels, and PCM
- `test_setup_transition.py`: transition, cursor rows, exit paths, PCM,
  Rules/Options page hashes, edit/commit storage, and F11 menu SFX
- `test_core_safety.py`: pack/ROM validation, host-rate timing, and SPC vectors
- `test_team_select.py`: Start-only Exhibition handoff, seven-position ROM selector, alphabetical/ranked wrap, 27 league teams plus East/West ROM teams and dash ranks, and Team Select frame hashes
- `test_player_setup.py`: Team Select confirmation, measured 200-frame handoff, ROM PPU assets, selected-team persistence, controller-side movement, and settled frame hashes
- `test_player_intro.py`: 290-key visitor/home portrait catalog, Player Setup handoff, card cadence, and non-default Golden State/San Antonio frame hashes
- `test_tipoff.py`: gameplay court/ball assets, shared ten-player compositor, ROM cadence, and formation/jump/contact frame hashes
- `test_gameplay_audio.py`: ROM BRR gameplay bank, persistent two-source crowd mix, overlapping effect voices, and live `$13E7/$13E9` command dispatch
- `test_cpu_gameplay.py`: CPU-vs-CPU post-tip movement, assignment, camera, possession/pass, telemetry, and proof-frame regression
- `analyze_cpu_gameplay_trace.py`: reports recurring plays, offense/ball-mode changes, movement cadence, stationary windows, and owned-ball attachment distance
- `ghidra/Run-CpuGameplayAnalysis.ps1`: correlates the extended live trace with the player integrator, CPU dispatch, and ball attachment/free-physics routines

## Verification tools

- `verify_ball_init_differential.py`: same-entry E056/E0AC native/C test through
  the production helper;20-word projection plus full128KiB unexpected-write
  checking. `run_differential.py --capture-ball-init` captures it; optional
  `--poison-ball-init` tests nonzero native entry values. Both durable fixtures
  and `ball_init_runtime_probe.c` run under `build.ps1 -Test`.
- `regenerate_pending_reference.py`: bounded recomp C for the requested
  gameplay slices. Reference-only output, not runtime code or native evidence.
- `run_differential.py`: fresh Mesen/C partial-state comparison (preserves the
  native CPU-versus-human setup; optional `--controllers cpu-vs-cpu`),
  using `mesen_differential_capture.lua`, `differential_runtime_probe.c`, the
  shared `differential_fields.def` schema and `differential_compare.py`.
  Rejects incomplete traces and reports the FIRST mismatching checkpoint;
  no sentinel skipping or fitted frame offset. Phase1 baseline currently
  fails; full initial-state/configuration import remains pending. See
  `docs/differential-testing.md` for commands, outputs and exact proof limits.
- `test_differential.py` and `differential_observer_probe.c`: comparator failure
  semantics and non-mutating actual-sweep observer tests, run by
  `build.ps1 -Test`. Synthetic unit PASS is not a native gameplay-equivalence PASS.
- Motion/pose replay tools: `verify_facing_ease_vectors.py`,
  `verify_locomotion_state_vectors.py`, `verify_animation_cadence_vectors.py`,
  `verify_pose_point_vectors.py`, `verify_ball_attachment_xy_vectors.py`,
  `verify_ball_attachment_z_vectors.py`, and `verify_contact_height_vectors.py`
  each invoke their matching compiled `*_vector_probe.c`. Animation replay
  compares accumulator/phase/resource outputs and reports upper mode-2 calls
  separately. Attachment wrappers resolve the actor from live DP `$96`.
- `verify_offense_normalize_vectors.py` and `verify_play_control_vectors.py`
  replay cached focal/anchor normalization and the five-actor event barrier.
  The play-control capture stops before `$85:B353`'s final store, so the
  verifier completes that store from captured DP `$AA`, not old `$099A`.
- `verify_effect_vectors.py --mode start|step` uses `effect_vector_probe.c`
  for `$87:A9E3/$AA02`; delayed captures are required to include active
  descriptors rather than only the presentation's inactive fallback.
- `nba_player_animation_self_test` retains seven live cadence/resource and
  five head-anchor witnesses in production C; the ordinary gameplay tests
  therefore protect them without needing local `.analysis` captures.
- `mesen_func_vectors.lua`: generic per-function I/O vector capture. Set
  `NBA95_VEC_ENTRY` to a routine's 24-bit entry PC, `NBA95_VEC_EXITS` to its
  RTS/RTL addresses, and `NBA95_VEC_READS`/`NBA95_VEC_WRITES` to the WRAM
  ranges it consumes and produces (from Ghidra). Every real in-game call is
  recorded as an entry/exit CPU + WRAM snapshot in
  `<label>.vectors.jsonl` — ground truth for replaying through the C port
  function and diffing outputs. `NBA95_VEC_DRIVE=1` drives the verified
  Exhibition path into live gameplay first (with `NBA95_CPU_VS_CPU=1` for
  CPU-controlled teams) and records only on-court calls. Set
  `NBA95_VEC_DELAY` to skip that many driven gameplay frames before arming a
  capture when presentation-idle calls would otherwise exhaust `MAX_CALLS`.
  A ported routine is
  *verified* when all its captured vectors pass.
- `verify_func_vectors.py`: replays a vector capture through a small C probe
  built against the real port sources and diffs each output word against the
  ROM's recorded exit state. `--word` handles one-word state transitions;
  `--input-words` plus `--output-word` covers pure routines with multiple
  captured inputs; `--output-words` verifies routines that produce multiple
  coupled words. `rng_vector_probe.c` is the one-word worked example:
  500 live `$80:CEE7` calls (including the `$07F6`-zero recovery path)
  verified `nba_gameplay_rng_next` with zero mismatches.
  `hoop_distance_vector_probe.c` independently replays signed DP `$AA/$AE`
  through `$85:F1C1-$F228`; 500 live calls cover all four RTL paths with zero
  mismatches against `nba_gameplay_hoop_distance`.
  `target_direction_vector_probe.c` replays those signed inputs through
  `$85:F347-$F3BA` and verifies both distance `$AA` and direction `$B2`.
  Dynamic-record routines can use a specialized normalizer:
  `verify_catch_prefix_vectors.py` resolves DP `$96/$9E`, then replays 19
  coupled outputs from `$86:BAA2-$BAFA` through
  `catch_prefix_vector_probe.c`.
  `verify_catch_mode_vectors.py` covers the first CPU-owner mode branch at
  `$86:BAFD-$BB14`; `verify_owner_dribble_pose_vectors.py` covers the
  terminal idle/moving dribble fallback at `$86:E593-$E5AA`.
  `verify_owner_dribble_gate_vectors.py` covers all three opening outcomes at
  `$86:E4A7-$E4C4`. Its internal exit PCs are shared by later control flow,
  so `NBA95_VEC_SHARED_EXITS=1` reports post-classification callbacks
  separately without mislabeling them as orphaned returns.
  `verify_owner_dribble_proximity_vectors.py` continues through
  `$86:E4C7-$E4F3`, verifying paired-side/speed/distance gates and the
  successful `+$86 -> +$50` requested-facing write.
  `verify_owner_unlatched_pose_vectors.py` covers `$86:E545-$E592`, replaying
  the relative-velocity 9/11 pose choice and `+$50 -> +$4E` facing write.
  `dynamic_dribble_attachment_probe.c` follows that selector through natural
  production cadence and requires `$87:B649/$B66A` to consume the published
  `+$2A/+$2C` pair across bases-9/11 reversals for 20,000 unforced frames.
  `verify_velocity_step_vectors.py` resolves dynamic actor/profile pointers
  (including LoROM profile byte `[$E0]+$42`) and replays 2,000 calls through
  `$85:A82C-$AB16`, comparing velocity `+$0E/+$10` and boost `+$72`.
  `verify_predictive_arrival_step_vectors.py` continues through
  `$85:B402-$B4B8`, comparing native carry, preserved steering, velocity and
  boost across both arrival outcomes. `verify_receiver_candidate_vectors.py`
  covers all three `$85:B60B-$B677` candidate returns using live actor records.
  `verify_special_actor_vectors.py` replays the preceding `$85:B4B9-$B50D`
  cutter timer and selection gates, resolving `$093E` as the possession actor
  rather than the current DP `$96` actor. `verify_pass_selector_vectors.py`
  then covers `$85:B50E-$B5FE` priority/order selection through both stable
  exits, and `verify_mode11_shot_policy_vectors.py` covers `$85:B734-$B820`
  while checking both the policy result and every resulting `$07F6` RNG state.
  `verify_mode11_parent_vectors.py` replays the complete `$85:B678-$B8CA`
  dispatcher through its normal, consumed-action and shot-start contracts.
  Its permanent corpus combines natural calls with urgent-clock, context-shot,
  clear-lane and blocked-lane controls; video-frame-interrupted calls are
  excluded because the NMI path can independently advance `$07F6`.
  `verify_court_presentation.py` replays 480 durable wrapper/stream/core
  witnesses and requires the exact 510-PC Ghidra census with `--require-census`.
  `test_court_presentation.py` checks the full 148x52 raw ROM map, 29 complete
  panoramas and 12 native viewport map hashes. `court_runtime_probe.c` checks
  live binding for 16,000 frames and 812 indexed viewports. Pack v31 asset 284
  supplies per-team raw VRAM/CGRAM rather than a flattened court screenshot;
  `test_runtime_ppu_inputs.py` compares an exact native camera witness and
  requires zero mismatch across 54,688 non-player and 182 goal OBJ pixels.
  `verify_ppu_parity.ps1` recaptures and runs both raw PPU and runtime-input
  parity gates. The compositor and 812-view runtime probes also run in
  `build.ps1 -Test`; the native recapture remains an explicit Mesen test. Use
  `capture_camera_presentation.ps1 -Kind core|wrapper|stream`
  (one kind per run) with `-Controlled` for boundary cases; detailed commands,
  Ghidra labels, pixel-proof limits and evidence are in
  `docs/camera-presentation-plan.md`.
  `verify_camera_vectors.py` retains the older camera target/easing replay.
  It does not alone prove the entire `$85:9192-$93F4` routine:
  `verify_court_presentation.py` covers the older 212 instructions;
  `verify_camera_handoff.py` adds the remaining 60 and 39 setup/caller
  instructions, with 1,133 saved native witnesses. `camera_handoff_runtime_probe.c`
  checks raw live-state/ball-height/orientation binding and subject latching.
  All run in `build.ps1 -Test`. Capture with `capture_camera_handoff.ps1`
  (optionally `-Controlled` or `-InitActor`); labels and commands are in
  `docs/camera-handoff-plan.md`. No-team hold is no longer a runtime shortcut.
  `verify_court_clamp_vectors.py` resumes at `$85:A692` after X
  integration and compares Y integration plus both clamp axes.
  `verify_reaction_core_vectors.py` checks the distance/RNG tail at
  `$85:B971-$B9D1`; `verify_pass_direction_vectors.py` checks the fine
  16-direction result at `$85:F3C3-$F472`.
  `verify_target_from_pair_vectors.py` resolves dynamic records and ROM
  formation tables for `$86:E923-$E96E`, including the X-only carried ADC.
  `verify_loose_pursuit_gate_vectors.py` covers both allow/reject exits of
  `$86:F0FD-$F1AF` and the offense-relative pursuit restriction.
  `verify_lane_clear_vectors.py` normalizes the linked-list player scan at
  `$85:F5E4-$F727` into ten actor records and compares the native blocked/clear
  result for both basket orientations.
  `verify_actor_commit_vectors.py` replays delayed moving-player calls through
  the split 16.16 `$85:96B5-$9961` commit. `verify_pass_init_vectors.py` and
  `verify_pass_release_vectors.py` cover the grounded `$86:AB73-$AF4D`
  initializer and `$86:A6B3-$A78F` mode-15 release core. The neighboring
  `verify_defense_refresh_vectors.py` covers `$85:BC07-$C0F5` across both the
  normal `$85:C0F5` return and exhausted-assignment `$85:C051` return. It
  compares cadence/rebuild state and every represented defense-planner actor
  output, including pair, anchor, and focal geometry.
  `verify_ownerless_ball_vectors.py` filters the shared `$85:9A6A` entry by
  negative `$093E`, then compares the represented ball/rim/pass/score core
  through `$85:A7C7`; owned-contact continuations and the separately scheduled
  `$87:AA02` graphics-effect dispatcher remain outside that replay.
  `verify_attached_ball_vectors.py` covers the complementary phase>=3 attached
  vertical response, including `$09F6`, gravity, split Z integration and the
  two rebound states. `verify_inbound_target_vectors.py` checks the inbound
  target constructor and its nested projection helpers, while
  `verify_defense_close_target_vectors.py` checks the mode-2/mode-4 close
  defender projection. `verify_player_contact_sweep_vectors.py` consumes the
  exact captured `$34D3` actor-pointer order and compares player-owned outputs
  from `$86:D652`; ball/event and separately scheduled animation-only calls
  are explicitly filtered rather than attributed to the player-pair replay.
  `verify_ball_contact_sweep_vectors.py` replays the complementary
  state-changing ball/pass/shot/loose-ball calls. Direct
  `verify_ball_acquisition_vectors.py` captures separate the shared BAA2
  ownership core from D25A's pass/live-state continuation;
  `verify_tip_acquisition.py` adds native BAA2/tip-wrapper fixtures and checks
  the ball-source10 caller identity. `verify_tip_completion.py` requires all28
  completion-branch starts; `tip_possession_runtime_probe` withholds the ball
  beyond the old frame220 gate and then allows physical acquisition. These
  run in `build.ps1 -Test`; capture via `capture_tipoff_flow.ps1`, and census
  with `ghidra/DumpTipoffFlow.java`. See `docs/tipoff-flow-plan.md` and the
  current `docs/gameplay-pending.md` table.
  `verify_no_owner_pursuer_vectors.py` checks the five-record loose-ball scan,
  and `verify_shot_launch_vectors.py` checks all three launch velocities,
  including fractional-Z borrow and final-ADC carry behavior.
- `trace_hash.py`: freezes a lockstep-passing gameplay JSONL trace as compact
  per-frame golden hashes (`--write-golden`), then re-verifies later runs
  cheaply (`--golden`), reporting the first divergent scene frame. Field-level
  diagnosis stays with `compare_gameplay_traces.py`.

- `progress.py`: quantifies port status with no hand-maintained state. Crosses
  Mesen `exec_*.txt` coverage (denominator: code observed executing),
  `$XX:XXXX` provenance comments in `src/` (documented), the
  `docs/verified-routines.json` ledger (verified), and the recomp's
  `bank_XX_YYYY` function set. Writes `docs/progress.md` with per-bank
  percentages and the largest undocumented executed regions.
  `evidence_ranges.py` is the shared strict ledger parser: semicolon-separated
  ranges are validated individually, and whole-bank or `host equivalent`
  aggregate rows must set `coverage_credit=false` so they cannot silently
  verify future captures.

### Human free-throw aim replay

`capture_human_free_throw.ps1` drives a real Exhibition gameplay boundary and
uses `mesen_human_free_throw_control.lua` to send B through Mesen's controller
API. It captures native dispatcher/oscillator calls without patching ROM, PC,
stack, processor flags, or RNG. The canonical
`.analysis/human-free-throw-native-20260829-v4` corpus uses first-press delay
60 and contains 1,556 vectors with zero orphan exits and zero shared-exit
callbacks. Its SHA-256 is
`a1c252ab961d6e72d4159553706a16176dd151ba4f26e5343d39e4808486dabd`.
Normalize the corpus and replay the seven durable state/input witnesses with:

```powershell
python tools\normalize_human_free_throw_vectors.py --vectors <capture>\human-free-throw.vectors.jsonl --capture-meta <capture>\human-free-throw.meta.json --output tests\fixtures\human-free-throw-aim-witnesses.json --rom '<path-to-rom>'
.\tools\build_vector_probe.ps1 -Name human_free_throw_vector_probe
python tools\verify_human_free_throw_vectors.py --vectors tests\fixtures\human-free-throw-aim-witnesses.json --probe build\human_free_throw_vector_probe.exe --rom '<path-to-rom>'
.\tools\ghidra\Run-HumanFreeThrow.ps1 -RomPath '<path-to-rom>' -GhidraHome '<ghidra>' -JdkHome '<jdk-21>'
```

The fixture locks `3 -> 3`, first press `3 -> 4`, held `4 -> 4`, release
fallthrough `4 -> 5`, wait `5 -> 5`, second press `5 -> 9`, and a distinct
cursor wrap. See `docs/human-free-throw-differential.md`; the ordinary runtime
adapter remains dormant, and complete common-launch effects/order remain
excluded. This is not a complete human-control claim.

## Investigation utilities

### Action/animation replay

`build.ps1 -Test` builds `action_animation_vector_probe` and replays 42 compact
WRAM witnesses from `tests/fixtures/action-animation-witnesses.json`. Expected
values come from live Mesen, not a duplicate Python animation implementation.

For a full local capture, use `mesen_func_vectors.lua` with comma-separated
`NBA95_VEC_ENTRY` addresses. Every selected routine must have its return PCs
in `NBA95_VEC_EXITS`; nested calls are paired LIFO and the verifier rejects
invalid entry/exit pairs. The capture stores each entry PC in its JSONL row.
`ghidra/DumpActionAnimation.java` dumps and labels the complete bank-$87 action
and cadence paths, including instructions absent from the older live listing.

```powershell
.\tools\build_vector_probe.ps1 -Name action_animation_vector_probe
python tools/verify_action_animation_vectors.py --vectors .analysis/func-vectors-action-install-full-20260826/action_install.vectors.jsonl --probe build/action_animation_vector_probe.exe --pack build/nba95_assets.pak
python tools/verify_action_animation_vectors.py --vectors .analysis/func-vectors-action-cadence-20260826/action_cadence.vectors.jsonl --probe build/action_animation_vector_probe.exe --pack build/nba95_assets.pak
```

The gameplay JSONL `raw.animation_rom` object is the literal channel state.
Historical `upper_phase`/`lower_phase` compatibility fields are not a substitute
for it outside the integrated pass path. Rendering still uses asset-pack data;
Mesen is only the verification oracle.

### Immediate pose and appearance replay

`ghidra/DumpActionPose.java` labels `$87:AEC3-$AF74` (pose refresh without a
cadence tick) and `$87:AFA2-$B053` (ten-player appearance/cache initialization).
The latter deliberately excludes pointer construction at `$86:D7B8` and
jersey composition at `$87:B059`. The recomp counterpart is
`bank_87_AEC3_M0X0` / the owned middle of `bank_87_AF95_M0X0`.

`action_pose_vector_probe` compares all eight pose outputs, or all 50
per-player appearance/cache words plus the overlapping `$180B-$180D` seed.
Input roster pointers must resolve to real records in the asset pack; missing
WRAM is an error, never an implicit zero. The native renderer does not emulate
the SNES upload queue; its seed and dirty words are retained as verification
outputs, while tip-off uses the body/upper variants from the same helper.

Capture with `NBA95_VEC_ENTRY=87AEC3,87AFA2`,
`NBA95_VEC_EXITS=87AF74,87B054`, and reads/writes
`0096-0097,180B-180D,3449-3470,34EB-3EEA,8E10-8E23`.
`NBA95_VEC_PREGAME=1` records initialization while the normal menu driver
continues into CPU-vs-CPU play. Expected fixture outputs are copied from live
ROM exits, not generated by a second implementation.

```powershell
.\tools\build_vector_probe.ps1 -Name action_pose_vector_probe
python tools/verify_action_pose_vectors.py --vectors .analysis/func-vectors-action-pose-long-20260826/action_pose.vectors.jsonl --probe build/action_pose_vector_probe.exe --pack build/nba95_assets.pak
```

`build.ps1 -Test` also replays all 28 calls in
`tests/fixtures/action-pose-witnesses.json` (26 poses, two ten-player setups).
Immediate refresh is adopted only for already-integrated live passes. The
inbound compatibility boundary and unconverted shot/contact callers remain.

### Shot-action replay

`ghidra/DumpShotAction.java` labels and dumps bank-$86 shot setup, wind-up,
facing/release and recovery, plus bank-$85 `$F02D-$F099` facing quantization.
The focused recomp inputs/outputs are retained locally under
`.analysis/shot-action-recomp-20260827/`.

```powershell
.\tools\build_vector_probe.ps1 -Name shot_action_vector_probe
python tools/verify_shot_action_vectors.py --vectors .analysis/func-vectors-shot-action-20260827/shot_action.vectors.jsonl --probe build/shot_action_vector_probe.exe --pack build/nba95_assets.pak
python tools/verify_shot_action_vectors.py --vectors .analysis/func-vectors-shot-delay-20260827/shot_delay.vectors.jsonl --probe build/shot_action_vector_probe.exe --pack build/nba95_assets.pak
```

The first capture has 42 recovery/start/gate/cleanup calls; the second has
123 timer decisions and two lower-jump installs. All 167 WRAM witnesses are
checked in at `tests/fixtures/shot-action-witnesses.json` and replayed by
`build.ps1 -Test`. Missing state or invalid entry/exit pairs are errors.
The gate compares facing and decision, and asserts represented actor state
and RNG remain otherwise unchanged. It stops before the ball-launch call.

Capture entry/exits for the first group:
`NBA95_VEC_ENTRY=86B6D3,86B8CA,869846,86B8C0,86B84C`,
`NBA95_VEC_EXITS=86B744,86B951,86B971,86B978,86B886,86986C,86B8C8,86B866`.
Reads/writes: `0000-00FF,07F6-07F7,0900-0980,34EB-3EEA,466B-47EA`.
Use `NBA95_VEC_SHARED_EXITS=1` for the shared gate exit.
The second group uses entries `86B7CD,86B84C`, exits
`86B8CA,86B7E4,86B7F7,86B866`, and reads/writes
`0000-00FF,0900-0980,34EB-3EEA`. Both use the CPU-vs-CPU menu driver.

Facing/release decisions, recovery and cleanup are adopted gameplay paths.
Ordinary startup, wind-up and lower-jump helpers are now connected by the
shot-branch checkpoint below. The full B625 selector and 9D6E launch are now
integrated by the complete-shot checkpoint below. Asset-pack descriptors
remain the runtime animation source; Mesen captures supply test data only.

### Stationary-shot, lost-owner and pump-fake replay

The requested disassembled slices are `$86:B7F7-$B849` (35 instructions)
and `$86:B867-$B86B`, `$86:B886-$B88F`, `$86:B890-$B8BF` (2+4+16).
The existing `$86:B8C0-$B8C8` cleanup is reused. The missing human/CPU
wind-up connector `$86:B86C-$B885` and owner/latch gate `$86:B769-$B790`
are also implemented. `DumpShotAction.java` names these boundaries; the
fresh dump is `.analysis/shot-branches-ghidra-20260827/shot_action_bank86.txt`.

The 30,000-frame natural capture retained 75 calls: five sidestep decisions
and 70 CPU wind-up returns. It did **not** naturally exercise lost ownership
or pump cancellation. `mesen_shot_branch_cases.lua` supplies 43 additional
controlled-ROM calls by changing WRAM inputs on genuine shot entries. It
does not patch ROM, CPU PC/flags or the stack. At real branch exits it records
outputs and restores the saved WRAM. Nineteen sidestep cases cover all nine
direction-table entries and rejection boundaries; two owner restores, four
cancellations, four button gates and five owner/latch gates cover the rare
paths. Nine extra release-facing cases verify `$86:9D7A-$9D98` as a helper
only: its caller is **not** integrated or counted in this checkpoint's ledger.

```powershell
.\tools\build_vector_probe.ps1 -Name shot_branch_vector_probe
python tools/verify_shot_branch_vectors.py --normalized --vectors tests/fixtures/shot-branch-witnesses.json --probe build/shot_branch_vector_probe.exe --pack build/nba95_assets.pak
```

All 118 retained witnesses run with `build.ps1 -Test`. Raw captures are at
`.analysis/func-vectors-shot-branches-headless-20260827/shot_branches.vectors.jsonl`
and `.analysis/shot-branch-release-cases-20260827/shot_branch_cases.vectors.jsonl`.
The verifier compares all represented actor/ball state and chosen exits,
including preserved RNG and fractional ball Z. The runtime self-test also
exercises stationary wind-up -> jump, both cancellation thresholds, and
lost ownership overriding a ready cancellation without touching the ball.
The integration trace must keep attached stationary shots out of the
ownerless rebound fallback. Shot attachment preserves ball fractions at
`$86:B7AF-$B7CA`. Loose-ball recovery must not depend on the host REBOUND
debug label after cancellation/free-throw continuations. The existing contact
predicates, full strategy/scoring checks, and 2,400-frame inbound guard are
retained; the movement-only analyzer does not prove sustained possession.

For headless controlled recapture, set `NBA95_CAPTURE_DIR` to a new directory,
`NBA95_VECTOR_DRIVER` to the absolute path of `mesen_func_vectors.lua`,
`NBA95_VEC_ENTRY=888888`, `NBA95_VEC_EXITS=888889`,
`NBA95_VEC_READS=0096-0097`, `NBA95_VEC_WRITES=0096-0097`,
`NBA95_VEC_LABEL=driver`, `NBA95_VEC_DRIVE=1`, `NBA95_CPU_VS_CPU=1`,
`NBA95_VEC_PREGAME=0`, and `NBA95_VEC_FRAMES=10000`.
Run `Mesen.exe --testrunner --timeout=300 <ROM> <mesen_shot_branch_cases.lua>`.
The deliberately unused driver entry/exit prevents duplicate captures; the
case script owns its real callback boundaries. Check the 43-case completion
sentinel and process exit before replaying. No desktop automation is needed.

### Special selector, mode 17 and complete launch

`docs/shot-completion-plan.md` maps exact routines, owned boundaries, findings,
capture provenance and remaining upstream gaps. `DumpShotCompletion.java`
refreshes bank86 labels/comments through the true launch return A476;
`DumpShotScheduler.java` documents the independent 85:EE30 countdown writer.

```powershell
.\tools\build_vector_probe.ps1 -Name special_shot_vector_probe
.\tools\build_vector_probe.ps1 -Name complete_shot_vector_probe
python tools/verify_special_shot_vectors.py --normalized --vectors tests/fixtures/special-shot-witnesses.json --probe build/special_shot_vector_probe.exe --pack build/nba95_assets.pak
python tools/verify_complete_shot_vectors.py --normalized --vectors tests/fixtures/complete-shot-witnesses.json --probe build/complete_shot_vector_probe.exe --pack build/nba95_assets.pak
python tools/test_special_shot_integration.py --exe build/nba95_port.exe --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --pack build/nba95_assets.pak
```

These 79 + 102 witnesses and both runtime basket sides run in `build.ps1 -Test`.
`test_shot_assets.py` also checks asset 277 against ROM and isolates F12 count-
header changes from asset pixels. Capture inputs are not production graphics.

### Close-finish modes

The retained corpus contains 91 natural CPU-vs-CPU calls across `$86:B0F7`,
`$86:B154`, and `$86:B34F`. It compares RNG, ball/ownership globals and the
entry actor through hold, expiry, receiver, terminal and initializer exits.
The runtime probe separately protects the basket-target attachment distinction
between `$3FEF/$3FF3` and the live ball at `$3EEF/$3EF3`.

```powershell
.\tools\build_vector_probe.ps1 -Name close_finish_vector_probe
python tools/verify_close_finish_vectors.py --vectors tests/fixtures/close-finish-witnesses.json --probe build/close_finish_vector_probe.exe --pack build/nba95_assets.pak
.\tools\build_vector_probe.ps1 -Name close_finish_runtime_probe
.\build\close_finish_runtime_probe.exe build/nba95_assets.pak
```

Headless recapture uses the unused driver entry/exit configuration described
above, `NBA95_VEC_FRAMES=100000`, a new `NBA95_CAPTURE_DIR` and the same
absolute `NBA95_VECTOR_DRIVER`. `mesen_special_shot_cases.lua` owns the 58
special cases. `mesen_complete_shot_capture.lua` owns full 9D6E/9DA6 entry to
A476 snapshots; `NBA95_LAUNCH_CONTROL=1` enables 70 ordinary cases, otherwise
it observes natural calls. It can wrap the special driver using
`NBA95_SHOT_CAPTURE_DRIVER` (absolute path to the special-case script). Confirm the
completion sentinel and process exit, then replay raw JSONL with `--rom`
instead of `--normalized`. Full-launch capture records the launch-owned
timeout and each asynchronous NMI countdown byte, not a timing tolerance.

The C-only diagnostic `--gameplay-special-shot-at 3420 --gameplay-actor 0`
prepares clearly labeled controlled inputs; it does not force a made basket.
Use actor 5 for the opposite basket. `--dump-sequence-from 3420` limits
capture to the selected interval without replaying a separate gameplay state.

```powershell
.\build\nba95_port.exe --headless --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --assets build/nba95_assets.pak --tipoff-only --frames 3560 --gameplay-special-shot-at 3420 --gameplay-actor 0 --gameplay-trace .analysis/special-proof.jsonl --dump-sequence-from 3420 --dump-sequence-dir .analysis/special-proof
ffmpeg -framerate 60 -start_number 3420 -i .analysis/special-proof/frame_%04d.bmp -vf scale=768:672:flags=neighbor -c:v libx264 -crf 18 -pix_fmt yuv420p .analysis/special-proof.mp4
```

### Shot-state writers and natural selection

`build.ps1 -Test` includes 342 ROM shot-state writer calls, 47 additional
natural selector calls, the runtime OFF/ON roster/clock binding probe, and
the unforced mode-17 lifecycle in the 63,800-frame CPU test. `shot_selection`
in gameplay JSONL now reports real selector inputs, assistance team, fatigue
timer, 24 stamina/playing-time words, and ten made/defensive-run counters.
`hot_team_09c0` remains only the historical launch-vector protocol name;
the field is trailing-team CPU Assistance, not a hot-streak switch.

```powershell
.\tools\build_vector_probe.ps1 -Name shot_state_vector_probe
python tools/verify_shot_state_vectors.py --normalized --vectors tests/fixtures/shot-state-witnesses.json --probe build/shot_state_vector_probe.exe --pack build/nba95_assets.pak
.\tools\build_vector_probe.ps1 -Name shot_state_runtime_probe
.\build\shot_state_runtime_probe.exe build/nba95_assets.pak
python tools/analyze_shot_selection.py .analysis/shot-state-proof-20260827/gameplay.jsonl
python tools/count_shot_state_instructions.py --listing-dir .analysis/shot-state-ghidra-20260827
```

For recapture, use `mesen_shot_state_capture.lua`, `NBA95_VECTOR_DRIVER`
pointing at `mesen_func_vectors.lua`, and unused driver entries `888888` /
`888889`, read/write range `0096-0097`. `NBA95_SHOT_STATE_CONTROL=0` observes
natural writers; `1` enables explicitly labeled boundary cases.
Set `NBA95_VEC_DRIVE=1`, `NBA95_CPU_VS_CPU=1`,
`NBA95_VEC_FRAMES=160000`, and invoke Mesen with `--testrunner --timeout=900`.
Use a fresh output directory and require `capture_complete.txt` before
normalizing with `--require-complete --rom ... --write-normalized ...`.
`NBA95_SHOT_MAKE_OFFSET=30` with 32,000 frames completes the nine remaining
make cases after the first period stops producing baskets. Reset this offset
to zero for other runs.

The fixed grant capture uses `NBA95_SHOT_STATE_MENU=1`, CONTROL=0,
CPU_VS_CPU=0 and 12,000 frames: real controller input opens the ROM pause
menu; a labeled controlled menu selection chooses timeout. Stamina boundary
values are injected at the helper entry, not by changing PC/ROM/stack.
Normal gameplay does not use any of these capture scripts. All stamina tables
and ratings come from the asset pack.

Ghidra `DumpShotStateMap.java` takes output directory, bank, **colon-separated**
ranges and seeds (headless splits commas). It labels/comments the C/ROM
correspondence and prints decoded instruction lengths. The 239-instruction
pre-code census stays fixed; `docs/shot-state-plan.md` separates finished
helpers from pending timeout/period/substitution caller integration.

### Owner caller, reversal, and idle cadence

`docs/owner-flow-plan.md` records the 145-instruction table, caller/callee
proof boundaries, raw capture locations, and remaining defensive idle
selection work. `build.ps1 -Test` replays all 294 durable witnesses, checks
the 31/22/92 instruction census, runtime bindings, and unforced special-shot
reachability in two 200,000-frame games.

```powershell
.\tools\capture_owner_flow.ps1 -OutputDir .analysis/owner-new/unlatched -Kind unlatched -Controlled
.\tools\capture_owner_flow.ps1 -OutputDir .analysis/owner-new/flow -Kind flow -Controlled
.\tools\capture_owner_flow.ps1 -OutputDir .analysis/owner-new/idle -Kind idle -Controlled
.\tools\build_vector_probe.ps1 -Name owner_flow_vector_probe
python tools/verify_owner_flow_vectors.py --normalized --require-census --vectors tests/fixtures/owner-flow-witnesses.json --probe build/owner_flow_vector_probe.exe --pack build/nba95_assets.pak
```

Use a fresh capture directory and require `capture_complete.txt`.
`DumpOwnerFlow.java` takes output-directory and bank86/87, labels/comments
the native routines, and prints instruction lengths. Supplying that output
to the verifier's `--listing-dir` checks exact PC sets and decode continuity.
Negative-lock reversal fixtures supply valid bank84 descriptor context;
arbitrary stale scratch-bank reads are not the portable asset-pack contract.
Recorded child outputs in the owner oracle verify caller ordering, not the
child implementation itself. No emulator process is needed by the port.

### Mode-1 compositor trace

Gameplay can dump the final PPU decision for every pixel:

```powershell
.\build\nba95_port.exe --headless --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --assets build/nba95_assets.pak --tipoff-only --frames 1000 --ppu-trace .analysis/ppu.jsonl
```

The first JSONL record summarizes winning layer counts. Each following record
identifies the final layer, BG/OBJ priority, Mode-1 rank, palette/color indexes
(`255` means a predecoded direct-color asset), OAM index, and ARGB result.

### Other utilities

Violation/dead-ball differential checkpoint: `mesen_violation_parent_control.lua`
drives ten labeled real-entry branch families through `$87:92A5-$949E`.
`normalize_violation_parent.py` retains only complete parent calls, and
`verify_violation_parent_vectors.py` replays the checked-in fixture through
the production dispatcher. The build gate also checks the exact Ghidra PC
census; `test_cpu_gameplay.py` protects live boundary-event consumption and
layout-1 inbound recovery.

Defensive target-family capture uses `capture_defense_target_families.ps1`.
Mode 6 enters at `$86:E9B3` and returns through the shared `$86:E82E`
continuation; `normalize_defense_target_families.py` retains spread witnesses
for each family and the build gate replays all target coordinates.

Normal CPU actor parents use `normal_actor_parent_vector_probe.c` and
`verify_normal_actor_parent_vectors.py` for `$86:F1B0-$F2C9`. The permanent
fixture retains timer-hold and decision-due calls from both entry points.
Nested formation/direction child writes are excluded only when the decision
is due; the parents' own F236/F2C1 direction-copy paths remain compared.
The nested `$86:E5AB-$E633` requested-direction finalizer has its own 300-call
natural corpus and `requested-direction-witnesses.json` build gate.

Jump/reach differential checkpoint: `mesen_jump_reach.lua` observes native
EC32 entry, parent child-call sites and returns. `run_differential.py
--capture-jump-reach` launches it; optional `--jump-cases` uses explicitly
controlled WRAM inputs. `verify_jump_reach.py` replays the production helper
and existing animation-channel API, while `test_jump_reach.py` checks durable
native fixtures, Ghidra PC census, corrupted-oracle rejection and ROM assets.
The production caller is exercised by `jump_runtime_probe`; its current
launch/rejection counts are integration evidence, not a complete starting-
state match. The far EAA8 branch and full pre-tip trajectory still have the
caveats in `docs/jump-reach-differential.md`. A bounded decision match must not
be reported as whole-game equivalence.

The remaining `mesen_*.lua`, Python render/decoder helpers, `spc_render_main.c`,
and `spc_replay_main.c` are diagnostic tools. They are not runtime dependencies
or part of the normal build. Set `NBA95_CAPTURE_DIR` to the desired output
directory before running a diagnostic Mesen script.

The Ghidra wrappers under `tools/ghidra` regenerate labeled listings and
decompilation notes. Supply their `-GhidraHome` and `-JdkHome` parameters on a
different machine.

### Full-ROM census and feature matrix

The full-ROM census is a conservative recursive disassembly, not a linear
sweep. It seeds the eight natively observed code banks from retained execution,
evidence-eligible ledger ranges, source provenance, recomp function entries and
SNES vectors. Direct cross-bank calls may discover more banks. A fresh prepare
deletes prior seed/listing/call artifacts so an older seed policy cannot leak
into the result. Unknown bytes remain
classified as data-or-undiscovered-code rather than being counted as pending
instructions.

```powershell
.\tools\ghidra\Run-FullRomCensus.ps1 -RomPath 'F:\Games\SNES\NBA Live 95 (USA).sfc' -GhidraHome 'C:\path\to\ghidra' -JdkHome 'C:\path\to\jdk-21'
python tools\feature_capture_matrix.py
```

The first command regenerates `docs/full-rom-instruction-census.md` and its
JSON source. The second validates the weighted planning data in
`docs/feature-capture-matrix.json` and regenerates its Markdown view. Do not
substitute either percentage for the other: one measures verified starts in a
lower-bound decoded universe; the other is an explicitly weighted feature
estimate.

Census provenance hashes include the ledger, census tool and evidence-range
policy. Checked-in text uses UTF-8/LF-normalized hashes so Git's CRLF checkout
conversion cannot make the same source appear stale; native capture, ROM and
executable identities continue to use raw-byte hashes. The report freshness
test still rejects actual source or content changes.

`mesen_team_select_capture.lua` also accepts `NBA95_TEAM_PANEL_ANIM=1`. It
captures the settled gold-plate frames plus OAM/CGRAM and short executed-address
traces. `Run-TeamSelectAnalysis.ps1` dumps the corresponding `$82:8933-$8967`
palette-window routine and the separate `$87:89D5-$89E8` background divider.

`Run-PlayerSetupAnalysis.ps1` labels and dumps the Team Select confirmation,
shared transition interpreter, Player Setup dispatcher, object positioning and
redraw paths, selected-panel palette animation, and vertical-scroll IRQ handler.

### Native edge-contract fixtures

`docs/native-edge-parity.md` records the Ghidra/recomp mappings, capture hashes,
reproduction commands and exact exclusions for the 2026-08-29 ball, actor,
OOB, inbound-side and formation fixes. The build replays durable native
fixtures separately from the inspected C-only image/state digests:

- `actor-commit-edge-witnesses.json`: 56 controlled native calls, 19 outputs;
  `capture_actor_commit_edges.ps1` / `verify_actor_commit_vectors.py`.
- `violation-oob-witnesses.json`: 46 controlled calls, including owned versus
  free predicates and X-priority corners; `capture_violation_oob_matrix.ps1`
  / `verify_violation_parent_vectors.py`.
- `inbound-side-gate.json`: 40 native signed-anchor cases;
  `capture_inbound_side_gate.ps1` / `verify_inbound_side_gate.py`.
- `formation-route-witnesses.json`: 64 retained plus 32 new native cases;
  `capture_formation_anchors.ps1` / `verify_formation_route_vectors.py`.
- `formation-override-witnesses.json`: ten supplemental native calls proving
  the conditional -40,+160 inbound teammate target and two sign rejections;
  `capture_formation_override.ps1` / `verify_formation_override_vectors.py`.
  Its `--self-test` checks malformed/corrupted fixture rejection separately.
- `ball-driver-owned-dispatch.json`: 31 output words, 323 complete projections
  plus one partial cross-frame event case; `normalize_ball_driver_owned_vectors.py`
  / `verify_ball_driver_owned_vectors.py`. The native event oracle is not changed
  to the isolated C producer's value.

`docs/inbound-cancel-recovery-differential.md` separates four controlled native
arrival projections from a host whole-update stale-ATTACHED/cache guard. The
generated header must match the compressed native capture exactly:

```powershell
python tools/verify_inbound_cancel_recovery.py --fixture tests/fixtures/inbound-cancel-recovery.json --header tests/fixtures/inbound-cancel-recovery.h --probe build/tip_flow_endurance_probe.exe --assets build/nba95_assets.pak --rom '<path-to-rom>'
build/tip_flow_endurance_probe.exe build/nba95_assets.pak
```

The deterministic recovery guard runs before the unchanged long-run period,
live-frame and maximum-stall requirements. Capture scripts are evidence-only;
none supplies production graphics, audio or an alternative gameplay runtime.
