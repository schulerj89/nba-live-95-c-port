# Preserved work at the user's stop request — 2026-08-30

This branch is **work in progress, not a release checkpoint**. The user asked
to stop, commit/push the current work, and summarize it. No game-completion
claim is made. The verified runtime remains main's A2 commit
`0d3a4207805aa7bea307cc1709e1f808acf7e54b`; the desktop executable and pack
were not replaced with this branch's candidate.

The branch consolidates the root configuration/resource/team-identity merge
and the agents' frozen HUD, audio-event and scheduler work. Agents did not
push independently. Captures, ROM, packs, WAVs, binaries, Ghidra output and
large traces remain local and ignored. Only source, tools, documentation and
compact native witness fixtures are committed.

## Implemented and checked in this branch

- Configuration: corrected fresh defaults (Exhibition/Arcade/Rookie/12 min),
  working versus committed Main values, separate Custom Rules, actual menu
  confirmation/cycling/repeat contracts, and value canvases. The integrated
  configuration gates pass 730 checkpoints, 1,770 entry/exit adjustment
  observations, 40 Main canvases and seven Rules canvases. These are bounded
  state/resource checks, not proof of every runtime consumer or persistence.
- Rules resources: preserve every native DMA publication, including writes
  whose baseline value was already zero; publish current value glyphs at the
  observed font-upload jobs. The independently reviewed patch passed the
  prior first-entry/return/hold gates separately. Root's combined parser
  checks pass 40 valid/corrupt native-stream cases and 11 integrity groups.
  Repeated entry still fails exact timing; combined transition replay is
  incomplete.
- Exhibition identity: publish home/right context0 and visitor/left context1,
  route roster/rating/appearance consumers through those identities, and
  publish sorted actor ranks. Root's fresh integrated probes match 128/128
  native identity words over two natural native captures. Seven verifier
  integrity groups pass. This is a direct initializer projection, not a
  full normal C menu-to-gameplay comparison.
- HUD: preserve the new original-font/resource publisher and clock formatters.
  The agent reports exact 35 controlled formatter cases and two five-child
  resource projections, with six verifier groups. The module compiles in the
  combined build but has no production caller. Asset286 is reserved but is
  not in the desktop pack. Shared task/NMI state, visibility and timing fail
  the full gate. See `gameplay-hud.md`.
- Audio events: preserve the isolated native event-consumption translation,
  probe and witness tooling. The frozen agent run passes 2,612 dispatches and
  173 ordered operations, plus seven verifier groups. The module is outside
  the production source manifest. Native playback-callee return values are
  explicit inputs; voice allocation, production NMI/RNG wiring, gain and
  audible sequence parity remain open. Independent module audit is pending.
- Scheduler: fresh read-only native evidence explains the repeated-entry
  mismatch through the real `$0564` epoch wait and DMA work crossing NMI.
  No timing fix was invented or implemented. See
  `setup-resource-scheduler-contract.md`.

Independent bounded reviews retained here are
`setup-config-independent-audit.md`, `rules-publications-independent-audit.md`
and `team-context-independent-audit.md`. They approve their named isolated
scopes, **not this combined branch or the entire game**.

## Known integration blocker at the stop

The old `src/main.c` headless menu scripts set `input.pressed` without producing
held/released controller states. The new native menu-input producer correctly
consumes `input.held`. Consequently the old `--setup-menu rules` script remains
on Main. Root reproduced this in `build/frontend-cli-stop-check.log`.
Do not weaken the native input implementation to make that script pass.
Migrate the script to real press/release input, then re-establish declared
dispatch/scroll alignment through normal menu operations. Existing native
fixtures must not be regenerated from C output. The corrected factory
defaults also invalidate older tests that silently assumed Simulation/3 min.

The full combined regression suite and independent combined audit have not
passed. The branch is intentionally not merged into main.

## Local continuation artifacts

- Integrated build: `build/nba95_frontend_checkpoint.exe`;
  final compile log: `build/ownership-stop-wip-build.log` (PASS).
- Config gates: `build/frontend-config-combined.log` and
  `.analysis/frontend-integration-20260830/config-combined/` (PASS).
- Identity: `identity-default.json` and `identity-alternative.json` under
  `.analysis/frontend-integration-20260830/` (both zero differences).
- Candidate pack: `.analysis/frontend-integration-20260830/nba95_assets_candidate.pak`,
  SHA256 `951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df`.
  Only IDs145/155 differ from A2; all other263-entry-pack metadata/payloads
  remain unchanged. Asset286 is not included. The root normal extractor
  changes were merged but a complete extraction of this candidate was not
  rerun before stopping.
- Resource freeze: `.analysis/transition-ownership-20260830/resource-freeze-v2.json`.
- HUD freeze: `.analysis/worktrees/team-context/build/hud-wip-freeze.json`.
- Audio freeze: `.analysis/worktrees/audio-events/build/audio-events/freeze.json`.
- Scheduler capture: `.analysis/setup-scheduler-20260830/native-v1/`.

All captures had stopped when this handoff was written. No background
implementation or monitoring task was scheduled.
