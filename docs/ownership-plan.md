# Completion ownership and evidence plan

Established 2026-08-30 from the local checkout and references. This is the
active integration plan; older percentage-named plans are historical slices.
**The game is incomplete. No whole-game parity or completion percentage is
accepted as a release gate.**

## Verified workspace identity

- Checkout: `C:/Users/joshs/Projects/nba-live-95-c-port`.
- Starting branch/commit: `main`,
  `2723af610aab0ec63263a6449fa6a161a155f974`. Initially clean. `git fetch
  origin` confirmed the same `origin/main`; only this checkout is registered.
- Remote: `https://github.com/schulerj89/nba-live-95-c-port.git`.
- ROM: `F:/Games/SNES/NBA Live 95 (USA).sfc`, 1,572,864 bytes;
  independently rehashed SHA-256
  `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
- No `AGENTS.md` exists in this repository or its filesystem ancestors.
  Other projects' instructions do not govern this checkout.
- Fresh MSVC `/W4 /O2` build succeeded. Log:
  `build/ownership-baseline-build.log`; untouched prior executable retained
  as `build/nba95_port-before-ownership.exe` for reproducing defects.

## Artifact map

Paths below are relative to this checkout unless absolute. Existence was
checked, rather than inferred from old reports. The ignored machine-readable
inventory is `.analysis/ownership-20260830/inventory.json`.

| Artifact | Location and current meaning |
|---|---|
| Portable production implementation | `src/` (35 C files), `include/` (33 headers); exact build membership in `nba95_sources.txt` |
| Scene ownership and routing | `src/nba_game.c`, `include/nba_game.h`; persistent choices/match state in `src/nba_session.c`, `include/nba_session.h` |
| Frontend render/state | `nba_ea_intro`, `nba_title_sequence`, `nba_setup_screen`, `nba_team_select`, `nba_player_setup`, `nba_player_intro` under `src/` and `include/` |
| Gameplay integration | `src/nba_tipoff.c`; extracted AI, ball, camera, court, owner flow, shot action/launch/state, foul, free throw, effect, tip flow and jump/reach modules |
| Raw/indexed render and audio | `src/nba_snes_ppu.c`, `nba_renderer.c`, `nba_assets.c`, `nba_audio.c`, `nba_spc.c`; `nba_*_debugger.c` are diagnostic surfaces |
| Working recompilation | `C:/Users/joshs/Projects/NBA-Live-95-Recomp`; **not a Git repository**. `generated/` currently contains banks 00/80/81/82, a dispatch file and a manifest, not complete generated gameplay banks |
| Recompiler implementation | `../tools/snesrecomp-source-v0.2.0-alpha/recompiler`; the sibling recomp's `snesrecomp/` contains only its runner. Focused regeneration entrypoints are `tools/regenerate_*_reference.py` |
| Bounded gameplay recomp references | `.analysis/**.c` and `../NBA-Live-95-Recomp/.analysis/recomp_gameplay_extract/`; identify the exact file/range/hash for each change. Generated C is reference, not independent execution proof |
| Recomp runnable binaries | `../NBA-Live-95-Recomp/build/NBALive95Recompiled.exe`, `nba_live_95_probe.exe`; their existence does not prove full native gameplay translation (LLE remains) |
| Ghidra projects | `ghidra-projects/` (72 `.gpr` files at inventory), including `NbaLive95FullRom80..87`, focused Setup/PlayerSetup/Tipoff/Timeout/Substitution projects |
| Ghidra scripts and dumps | `tools/ghidra/`; ignored `.analysis/*ghidra*`, `.analysis/full-rom-census/`, and feature-specific dump directories |
| Asset source pipeline | `tools/capture_assets.ps1`, `tools/mesen_*_capture.lua`, `tools/extract_assets.py`, `tools/snes65816_decompressor.py`; raw VRAM/CGRAM/OAM/SPC/BRR and ROM decoding inputs. Follow-up found prohibited PNG sources for legal2, EA3–6/72/73; replacement required, see `intro-exact-audit.md` |
| Production pack | `build/nba95_assets.pak`, v31, 90,057,659 bytes; SHA-256 `d6adfe3ab8a49805a2cd10921281c33541135a332b4f2b174dfe25c093c2ebfd` |
| Other packs/audio | `build/` holds 13 other historical packs; `extracted_wavs/` and captured WAVs are not authority to substitute recorded songs for production sequencing |
| Native capture tools | `tools/mesen_func_vectors.lua`, dedicated `mesen_*.lua` and `capture_*.ps1`; Mesen resolves on PATH to the installed WinGet Mesen2 executable |
| Strict trajectory experiment | `tools/run_differential.py`, `mesen_differential_capture.lua`, `differential_runtime_probe.c`, `differential_fields.def`, `differential_compare.py` |
| Retained native fixtures | `tests/fixtures/` (69 files at inventory); provenance/shape checked by dedicated `verify_*.py` and `test_native_verifier_integrity.py` |
| C regression/integration probes | `tools/test_*.py`, `*_probe.c`, `build_vector_probe.ps1`; distinct from independent ROM evidence even when their names say closure or native |
| Existing strict failure evidence | `.analysis/differential-release-final-cpu/{run.json,report.json,rom.jsonl,port.jsonl,baseline.wram}`; capture made before starting commit with disclosed dirty tree |
| Transition evidence | Historical `.analysis/live_transition_debug_*`, `full_frame_oracle*`, `final_transition_recordings_*`; fresh work under `.analysis/transition-ownership-20260830/` |
| New ownership evidence | `.analysis/ownership-20260830/` and workstream-specific fresh directories; local images/video/raw memories stay ignored |
| Build and desktop | `build.ps1`, `CMakeLists.txt`, `nba95_sources.txt`, `build/nba95_port.exe`; shortcut must be inspected/refreshed at a playable release checkpoint |
| Status and evidence accounting | `STATUS.md`, `docs/parity-gap-report.md`, `verified-routines.json`, `progress.md`, `full-rom-instruction-census.*`, `feature-capture-matrix.*` |

The executable headless Ghidra installation was subsequently verified at
`C:/Users/joshs/Downloads/ghidra_11.3_PUBLIC_20250205/ghidra_11.3_PUBLIC/support/analyzeHeadless.bat`,
with JDK `C:/Program Files/Microsoft/jdk-21.0.12.8-hotspot`. The directory
`C:/Users/joshs/Projects/tools/ghidra` exists but is empty; existence of that
directory alone was not valid tool-location evidence.

The desktop resolves to `C:/Users/joshs/OneDrive/Desktop`. Its
`NBA Live '95 (C Port).lnk` targets this checkout's `build/nba95_port.exe`,
with the verified ROM and `build/nba95_assets.pak` arguments. The separate
`NBA Live 95 (Recompiled).lnk` targets
`../NBA-Live-95-Recomp/dist/NBA Live 95 Recompiled/NBALive95Recompiled.exe`;
that 644,608-byte reference executable hashes to
`ae219af8a2fee3d0186c14c19b884fb27cd2a81e93907296232b60d6b1dee31c`.
The reference shortcut is not the portable C build and will not be overwritten.

The local evidence store is substantial: approximately 59.7 GB in `.analysis`
and 17.5 GB in `build` at inventory. Do not duplicate it into worktrees or Git.
Copyrighted ROM data, packs, generated code, captures and large traces remain
ignored. A filename containing `final` or `PASS` is not a freshness certificate.

## What the old numbers actually measure

| Number at starting commit | Denominator and exclusions |
|---|---|
| 28,643 / 28,643 documented (100%) | Retained executed address positions covered by provenance comments; includes old coalesced intervals. Excludes uncaptured modes, branches and features. Does not count correct behavior |
| 11,529 / 28,643 eligible (40.25%) | Same capture-address denominator intersected with eligible ledger ranges; bounded represented-output evidence, not every caller or whole-routine proof |
| 11,526 / 60,346 decoded starts (19.10%) | Conservative recursive Ghidra instruction-start census. Unknown ROM bytes are not counted as code. Excludes undiscovered control flow; different units from the preceding two rows |
| 55.50% feature estimate | Hand-assigned weights and completion estimates across eleven features. No independent measurable completeness denominator; preserve only as historical planning input |
| Zero vector mismatches | Equality for declared outputs at recorded routine boundaries and sampled input cases. Does not establish unobserved branches, real caller reachability, timing or initialization |
| C trace/image golden PASS | Deterministic stability of the C implementation. A reviewed hash protects against change but cannot establish ROM equivalence |

There are 233 ledger entries, 207 eligible and 26 excluded at this baseline.
The fresh project-census test passes its accounting checks; that result does
not validate feature estimates or all evidence claims.

## Gap inventory and evidence labels

Use seven separate labels in the workstream tables: implemented, production
wired, bounded ROM-verified, C regression-tested, visually/audibly verified,
approximated, and missing. A feature can carry several labels. A dormant
helper is not a wired feature. The detailed inventories are maintained in
`transition-ownership-audit.md`, `gameplay-ownership-audit.md` and
`options-test-ownership-audit.md` as those audits complete.

| Area | Remaining release work identified from code and evidence |
|---|---|
| License/legal/EA/title | Consecutive normal and skipped transition captures; reconcile upload/fade/scroll ownership instead of accepting isolated endpoint hashes |
| Setup/Rules/Options | Every entry/return, repeated visits, altered values, native style presets/custom selection, dynamic value tilemaps and all runtime consumers |
| Team Select/Player Setup | Both team choices and return/confirmation flows, native outgoing transitions, neutral/human ownership and controller allocation |
| Matchup/lineups/tip-off | Full sequential PPU/OAM/resource handoffs, startup state and scheduler; current native/C baseline fails |
| Ordinary human gameplay | Movement/boost/stop, ball control/pass/shot, defense/switch/steal/block, input ownership and all downstream consumers. Currently CPU-only with isolated dormant human helpers |
| CPU gameplay | Align initialization/RNG/scheduler, remaining caller prefixes, AI decisions and action/resource timing; natural CPU-versus-CPU journey and exact differential |
| Ball/contact/scoring | Whole-call state/order, rim anchors, pending interrupt/event case, shared RNG/audio effects and rare contact/ball branches |
| Rules/free throws | Six rule settings have no direct gameplay config readers at audit start; trace indirect/native semantics before implementation. Natural foul-to-stripe, bonus/penalty, violations and replacement paths |
| Clock/period/OT | Bounded tables/expiry implemented; natural full-match comparison, exact break/final presentation and mode-specific side effects remain |
| Pause/timeouts/substitution | Native menu choices, disabled states, bench UI, automatic/manual replacement and precise resume transitions; current pause art is a host panel |
| Final/replay/return | Native postgame/stats/menu art and logic; second Exhibition currently inherits FINAL/period state and stalls, requiring a new-match reset at the real boundary |
| Court/players/camera | Crowd tile cadence, basket windows, edge/high-jump/rare OAM ordering, camera ownership and complete frame equivalence |
| Audio | All option semantics, shared command/RNG/event order, priority/mixing and long continuity; samples and commands alone do not prove timing |
| Season/Playoffs/Load Series | Inventory native screens/routes, schedules/brackets/results, records and save/load; no complete implementation or independent end-to-end tests |
| Persistence | Separate session choices from match reset and original SRAM/disk semantics. Retained menu labels do not prove saves |

## Testing critique and corrective actions

1. Keep native fixtures, C regressions and controlled state injection explicitly
   separate. The strict CPU capture also injects pre-game team/controller
   choices; it is not an untouched button-only journey.
2. At baseline the strict test compares 449 words, reports 62 CPU-only
   differences (63 with one native human), zero matching checkpoints and first
   sweep native frame 25 versus C frame 2. Clocks/configuration differ too.
   Later motion cannot be attributed as an equal-input trajectory failure.
3. Its projection excludes scores, full team/roster context, controller latches,
   rendering/APU state and some timers. A matching projection would still not
   prove the complete game. Capture missing state, do not widen tolerances.
4. The historical inbound compensation envelope was introduced at `a893dbb`
   but **already removed at the starting commit**. The current test checks raw
   `[-9,+8]`; inspect native pre-call state and all emitted probe outputs before
   accepting its proof. Do not report an obsolete envelope as current code.
5. Prior default-image captures are useful independent evidence, but replaying
   their tilemaps as production transitions can restore default labels when
   current choices differ. Test dynamic data as well as endpoint pixels.
6. Search for hardcoded expected outcomes and ignored fixture words. In
   particular, the lifecycle expiry probe checks fixture strings rather than
   interpreting its expected outputs, and the inbound motion verifier drops
   one emitted word. Neither should be presented as complete native replay.
7. Golden changes require an independently justified behavior change and
   visual review; no automatic update-to-pass path. Test natural production
   routes as well as direct scene starts and controlled edge cases.
8. Fresh baseline build, census regression, 15 verifier integrity tests and 12
   differential harness unit tests pass. These are method/build checks, not
   a newly passing native game comparison.

## Dependency-aware checkpoints

| Checkpoint | Work and acceptance dependency |
|---|---|
| A: transition foundations | Inventory every handoff; reproduce fresh C/native consecutive frames; fix first causal resource/state ordering mismatch; test changed values and repeated returns. No invented masking/fades |
| B: match launch and reset | Correct genuine new-match reset; synchronize declared configuration and native initialization/scheduler/RNG boundaries. Retain strict first-divergence failures until resolved |
| C: ownership and ordinary play | Native controller selections/allocation, human and CPU dispatch, movement/action/defense and real callers. Natural human/CPU and CPU/CPU journeys; preserve quirks |
| D: rules/options and match events | Every value-to-consumer mapping and event proof, fouls/free throws/inbounds/violations and audio behavior. Requires C ownership/input paths |
| E: complete match presentation | Pause/timeouts/substitutions, quarters/halftime/OT/final and second match, all exact transition/resource/audio contracts |
| F: retail modes/persistence | Season/Playoffs/Load Series, postgame/statistics/records and original persistence semantics; native capture precedes completion claims |
| G: release acceptance | Independent differential/regression/endurance plus consecutive-frame/audio review, fresh playable desktop build, clean tree and pushed main. Any unimplemented/unverified original feature keeps whole-game acceptance FAIL |

Checkpoint A2 replaces the inherited intro image sources with indexed pack
resources75/76 and the original font. Bounded independent renderer evidence
is in `intro-indexed-resources.md` and `intro-text-independent-audit.md`.
This supersedes the intro asset-row findings in the **initial** inventory
above; other historical paths/hashes remain identified as inventory snapshots.
Current source counts and pack identity belong to each checkpoint manifest.
Cold-boot scheduling, legal input/waits, intro audio and title handoff remain
open; no whole-intro PASS follows from303 aligned renderer frames.

At every coherent checkpoint: bounded implementation, native/recomp/Ghidra
evidence, focused and integration tests, visual/audio review, independent
PASS/FAIL audit with exact caveats, documentation, fresh build, root-owned
commit/push and remote confirmation. Do not stop after writing this plan.

## Parallel ownership

- Transition implementer owns frontend scene/render files and dedicated
  capture/test tools. Gameplay implementer owns `nba_tipoff` plus coordinated
  session/reset work. Options/test implementer owns the option mapping and
  dedicated inbound verification/capture tools.
- Root owns integration, build manifest, evidence ledger/status, release gates,
  commits and pushes. Request frontend dispatch insertions from its owner;
  no concurrent edits to shared files. No agent pushes independently.
- Native captures use unique directories and owned process IDs. An unrelated
  NBA Live 96 Mesen process is active on this host: never stop it or change its
  settings. Use bounded headless captures, not shared GUI assumptions.
- Independent auditors inspect source, native/recomp/Ghidra and actual evidence
  themselves. Implementer summaries are navigation aids, not approval proof.
