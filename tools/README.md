# Reverse-engineering and asset tools

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
| `mesen_tipoff_capture.lua` | Drives into live tip-off, records raw actor/ball/controller/camera/AI state and emits `gameplay_rom.jsonl` for `compare_gameplay_traces.py` |
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
- `test_cpu_gameplay.py`: CPU-vs-CPU post-tip movement, assignment, camera, possession/pass, telemetry, and proof-frame regression
- `analyze_cpu_gameplay_trace.py`: reports recurring plays, offense/ball-mode changes, movement cadence, stationary windows, and owned-ball attachment distance
- `ghidra/Run-CpuGameplayAnalysis.ps1`: correlates the extended live trace with the player integrator, CPU dispatch, and ball attachment/free-physics routines

## Verification tools

- `mesen_func_vectors.lua`: generic per-function I/O vector capture. Set
  `NBA95_VEC_ENTRY` to a routine's 24-bit entry PC, `NBA95_VEC_EXITS` to its
  RTS/RTL addresses, and `NBA95_VEC_READS`/`NBA95_VEC_WRITES` to the WRAM
  ranges it consumes and produces (from Ghidra). Every real in-game call is
  recorded as an entry/exit CPU + WRAM snapshot in
  `<label>.vectors.jsonl` — ground truth for replaying through the C port
  function and diffing outputs. `NBA95_VEC_DRIVE=1` drives the verified
  Exhibition path into live gameplay first (with `NBA95_CPU_VS_CPU=1` for
  CPU-controlled teams) and records only on-court calls. A ported routine is
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
  `verify_velocity_step_vectors.py` resolves dynamic actor/profile pointers
  (including LoROM profile byte `[$E0]+$42`) and replays 2,000 calls through
  `$85:A82C-$AB16`, comparing velocity `+$0E/+$10` and boost `+$72`.
  `verify_predictive_arrival_step_vectors.py` continues through
  `$85:B402-$B4B8`, comparing native carry, preserved steering, velocity and
  boost across both arrival outcomes. `verify_receiver_candidate_vectors.py`
  covers all three `$85:B60B-$B677` candidate returns using live actor records.
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

## Investigation utilities

The remaining `mesen_*.lua`, Python render/decoder helpers, `spc_render_main.c`,
and `spc_replay_main.c` are diagnostic tools. They are not runtime dependencies
or part of the normal build. Set `NBA95_CAPTURE_DIR` to the desired output
directory before running a diagnostic Mesen script.

The Ghidra wrappers under `tools/ghidra` regenerate labeled listings and
decompilation notes. Supply their `-GhidraHome` and `-JdkHome` parameters on a
different machine.

`mesen_team_select_capture.lua` also accepts `NBA95_TEAM_PANEL_ANIM=1`. It
captures the settled gold-plate frames plus OAM/CGRAM and short executed-address
traces. `Run-TeamSelectAnalysis.ps1` dumps the corresponding `$82:8933-$8967`
palette-window routine and the separate `$87:89D5-$89E8` background divider.

`Run-PlayerSetupAnalysis.ps1` labels and dumps the Team Select confirmation,
shared transition interpreter, Player Setup dispatcher, object positioning and
redraw paths, selected-panel palette animation, and vertical-scroll IRQ handler.
