# Ownership checkpoint A1 — transition foundations

Date: 2026-08-30. Starting revision: `2723af610aab0ec63263a6449fa6a161a155f974`.
Whole-game acceptance remains **FAIL**. This checkpoint does not complete all
transitions or make human gameplay available.

## Bounded changes and evidence

| Change | Independent evidence and limits |
|---|---|
| Rules first opening |147 natural-ROM RGB frames470–616, including A dispatch;147 complete22-field PPU projections. C input idle717 is a declared background-phase alignment, not an added production delay |
| Rules held screen and selected values |137 further held RGB frames; four selected row/value snapshots. Native viewport/arrow palette lifetime and full glyph shadows restored |
| Rules first return |Two natural journeys, unchanged Simulation and row2/Custom:342 RGB frames830–1000,266 complete22-field PPU projections; full final13 Rules/4 main words and parent Mode cursor. Start dispatch now preserves its live viewport until the native wait, while clearing the selected highlight |
| Rules adjustment |Native style-to-Custom side effect on ordinary and clamped adjustments; controlled native prestates through actual C Setup input dispatch. Three cases/twelve snapshots; six protocol tests. Factory presets, persistence and runtime consumers are excluded |
| Brightness |Native five-bit quantization before RGB expansion;1,536 controlled native channel cases. C/Python shared eight-bit error removed. Previously changed hashes have independent conversion-only attribution |
| New match |Actual postgame→Setup→Team→Player→Intro→Tipoff callers reset match-owned terminal state and preserve session choices. Native first-court word projection; native second-match timing is not established |
| Inbound internal state |500 native dispatches,470 same-dispatch velocity changes,389 prepared arrivals/111 rejections. Exact four-word motion and ten-word arrival replay; controlled pregame setup disclosed |
| Verification methods |Lifecycle expected values are actually consumed; exact inbound direction enforced; private Mesen homes/settings/saves verified; synchronous RGB replaces asynchronous screenshot assumptions; mutation tests reject missing, altered or contradictory evidence |
| Closure regression |Historical digest reproduced; new `fdbdd69c21271f89` independently attributed. All6,000 gameplay updates and telemetry unchanged, with only native-backed Style1→2 session differences. One of66 images changed and matches all57,344 native pixels;65 images unchanged. This remains C-versus-C regression attribution |

Auditors independently read original-ROM bytes, Ghidra/recomp routines,
implementation and retained raw evidence. See `transition-independent-audit.md`,
`new-match-reset-audit.md`, `lifecycle-verifier-audit.md`,
`ppu-brightness-independent-audit.md`, `differential-launch-independent-audit.md`
and `closure-digest-attribution.md`. Final dispatch evidence lives in
`.analysis/rules-return-t0-audit-20260830/`; root closure replay is
`.analysis/closure-digest-audit-20260830/final-dispatch-root/`.

The side-by-side171-frame original/C video and visually inspected contact
sheet are under `.analysis/checkpoint-20260830/`. These are evidence only and
are not packed into production graphics.

## Build and checkpoint validation

The final dispatch executable SHA-256 is
`01902328061e031c51a36952fd84685dd4ddd6ee49e1c0c4b2ea3d65f5389d1e`.
The production pack SHA-256 is
`5d364ce926bbb8d7c12a51990e3a7409a17a5a45350b0cc6838db5ed16b1193f`.
Normal extraction reproduced that pack without special environment overrides.
Only assets144/145/154/155 differ from the preserved starting pack; no captured
RGB images were added by this checkpoint. Historical intro PNG assets still
exist and remain an explicit asset-policy failure.

The C Port shortcut on `C:\Users\joshs\OneDrive\Desktop` now targets this
executable and `build/nba95_assets.pak`, with the original ROM path. The separate
Recompiled shortcut was not changed.

`build.ps1 -Test` completed successfully; the retained log is
`build/ownership-checkpoint-suite.log`. This includes focused native projections,
core safety, UI/audio/asset regressions,63,800-frame lifecycle endurance,
48,000 multi-team gameplay updates,6,000-update closure regression and the
63,800-frame CPU gameplay/telemetry regression. Those C endurance runs are
regression checks, not synchronized native full matches.

The later Start-dispatch fix was rebuilt and separately rerun through all147
opening/342 return native frames,27 transition integrity tests, the full
Setup/Rules/Options regression and frozen6,000-update closure attribution.
Logs: `build/ownership-final-native-gates.log`,
`build/ownership-final-setup-regression.log`, and
`build/ownership-dispatch-closure.log`. Six Custom protocol tests and eight
configuration-evidence integrity tests also pass. No expected image/digest was
updated without the independent, bounded attribution described above.

## Still failing or unverified

- Repeated Rules entry has a one-frame builder-boundary difference, incorrect
  background cadence phase and stale default text. Its explicit nonblocking
  parity test retains the failure instead of weakening an assertion.
- Edited Rules return has intermediate offscreen VRAM differences despite
  exact rendered pixels/PPU fields. Full resource upload equivalence is not
  established. Options retains a legacy unverified construction guard.
- Cold boot/intro omits native waits and uses invalid PNG-derived production
  assets. The isolated indexed-render prototype matches135 native EA frames,
  but is not integrated or accepted as a completed intro implementation.
- Whole gameplay differential still fails62/449 baseline words with zero
  matched checkpoints; initialization/scheduling/RNG are not synchronized.
- Human ownership/control routing, complete CPU behavior and real match
  lifecycle, every rules/options consumer, audio sequence/mixer parity, dynamic
  HUD/presentation, non-Exhibition modes and original save semantics remain
  incomplete or materially unverified. See the subsystem inventories.
- Address accounting now observes29,438 positions with11,529 in eligible
  verified boundaries (39.16%). The lower-bound disassembly has11,526 verified
  starts of60,346 (19.10%). Neither measures game completion. The inherited
  55.50% weighted feature estimate remains historical only.

Next work remains dependency-aware: complete transition resource builders and
intro assets; integrate native team contexts/actor ordering and configuration
semantics from separate worktrees; then controller allocation and ordinary
human/CPU play, event consumers, remaining presentation, modes and persistence.
