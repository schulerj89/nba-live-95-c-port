# Special-shot selector and complete launch

Implemented and verified on 2026-08-27, starting from `3b44215` on main.
Checkpoint publication is recorded in Git history.

## Result and routine map

| ROM boundary | Native implementation | Behavior |
|---|---|---|
| `$86:B625`, selector `$B629-$B6D2` | `cpu_start_rom_shot`, `nba_special_shot_select` | Fresh lane predicate; movement/distance/facing/appearance gates; upper 14/15, lower 1F; ordinary fallback |
| `$86:B979-$BAA1` | `nba_special_shot_step`, `cpu_update_rom_special_shooter` | Owner loss, phase-specific attachment, jump, turn, cancellation, phase-3 release and caller cleanup |
| `$86:9D6E` / `$9DA6`, return `$A476` | `nba_shot_launch` | Ordinary facing/pose versus retained special pose; detach, identity, quality, RNG, miss displacement, velocities, free throws, attempts and mode |
| `$86:A561-$A5AF` | `nba_gameplay_shot_value` | Origin-based arc test, including shared free-throw call |
| `$86:9CDB` attempt branches | `nba_shot_launch` | Player/controller attempts; made-stat branches excluded |

The four phases—map/capture, selector and mode 17, complete launch,
integration/evidence—are finished. The true return is A476, not the old
listing's A45E cutoff. Native typed state replaces gameplay behavior, not
emulator stack/register machinery or a running emulated game.

## Verification

- **79 selector/executor witnesses:** 21 natural ordinary fallbacks and 58
  controlled genuine-ROM calls cover selection gates, locks, hold/jump,
  owner loss, cancellation, airborne turn and release.
- **102 complete-launch witnesses:** 21 natural ordinary, 70 controlled
  ordinary and 11 controlled special-entry calls. All owned persistent
  outputs, preserved state, attempt statistics and RNG match.
- Controlled ROM tests change WRAM inputs on genuine calls, never ROM,
  PC, flags or stack. Natural and controlled evidence stays labeled.
- Full `build.ps1 -Test` passes, including all 181 new witnesses and the
  63,800-frame CPU test: final score 38–39, 2,140 exact-pass frame checks,
  94 automatic unlocks. Strategy/scoring and the 2,400-frame inbound guard
  remain intact.
- Controlled runtime tests on both basket sides enter mode 17, jump, retain
  the special animation, and physically release at frame 3446. There are
  **no natural mode-17 occurrences** in the 63,800-frame run; this proves
  reachable integration, not ROM-identical selection frequency.
- Asset 277 (`NBSHOT1`, 528 bytes) holds five ROM shot-table ranges.
  Byte-for-byte comparison passes. Animation, hand geometry, roster identity,
  ratings and graphics use the asset pack; no captured art/emulator runtime.
  F12 golden changes are proven confined to its index/count header.

Durable fixtures: `tests/fixtures/special-shot-witnesses.json` and
`tests/fixtures/complete-shot-witnesses.json`. Commands: `tools/README.md`.
Inline data and made-stat branches are excluded from the verified code ledger.

Coverage: 109 verified slices; **6,785 / 27,901 captured address positions =
24.32%**, up from 6,454 = 23.13% (+331 positions, +1.19 points).
Documented: 9,549 (34.2%). This is not whole-game completion or an instruction
census.

## Important source findings

- At 9FC1, STZ does not change Z; BNE is not taken after equal hot-team
  comparison. The default timing branch still executes.
- A02A doubles an already word-stepped timing index. Reads can extend into
  adjacent instruction bytes; the asset retains these literal bytes as data.
- Attachment preserves fractions. Launch uses 16.16 deltas, signed division,
  separate integer-Z SEC and the original ADC carry behavior.
- CPU free throws consume two RNG draws even for successful make rolls.
- Launch writes 1800 to 0930. Six natural captures were interrupted by
  `$85:EE30 DEC $0930` before return. Every countdown byte write is recorded
  and exactly reconstructed; C compares the launch-owned post-STA value.
  No tolerance or unexplained output masking is used.
- The C free-throw scene skipped common play control. Keeping
  `$87:8FA1 -> $85:AF5C/B128` active prevents a pending request surviving
  a made free throw. Scoring tests permit one point only for an active
  one-point free throw in the preceding frame.

## Local proof

Ghidra project `ghidra-projects/NbaLive95ShotAction`, bank86, has refreshed
labels and C-mapping comments. Scripts: `tools/ghidra/DumpShotCompletion.java`
and `DumpShotScheduler.java`; listings/logs:
`.analysis/shot-completion-ghidra-20260827/`.
Focused recomp: `.analysis/shot-completion-recomp-20260827/generated/`.

Final raw captures: `.analysis/special-shot-complete-20260827/`,
`.analysis/complete-shot-controlled-20260827/`,
`.analysis/shot-completion-natural-final-20260827/`.

Port evidence in `.analysis/shot-completion-proof-20260827/`:

- `gameplay-final.jsonl`: natural 63,800-frame run. Earlier `gameplay.jsonl`
  is diagnostic and is not final evidence.
- `cpu_3480.png`, `cpu_6932.png`, `cpu_6954.png`: inspected natural frames.
  Corrected quality/RNG changed later possessions and their goldens;
  frames 600/1300/3480 keep previous hashes.
- `controlled-special-shot.mp4`: 141 frames, 60 fps, 2.35 s, nearest-scaled
  768x672, no audio; clearly controlled C inputs at frame 3420.
- `special_3438.png`, `special_3444.png`, `special_3446.png`: inspected
  attachment, phase-3 pose and release. Both side traces are retained.

Full regression logs: `.analysis/shot-completion-regression.log` and the
final rebuilt run `.analysis/shot-completion-final-regression.log`.

## Remaining boundaries

This completes selector/launch, not all CPU gameplay. Upstream fatigue,
shot-modifier and hot-streak writers remain unported; runtime initializes
those explicit inputs to 7FFF, zero and FFFF. Their launch variations are
replay-verified, not their upstream writers. Human launch/free-throw branches
are verified helpers; user control remains out of scope. Natural special-shot
frequency, other contact/action callers and inbound animation adoption remain
follow-up work. Do not infer the entire scheduler is ROM-identical.
