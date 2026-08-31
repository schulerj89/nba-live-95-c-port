# Resumed completion mission — living continuation

Whole-game acceptance: **INCOMPLETE**. Resumed 2026-08-30; historical paused
documents are evidence, not instructions to stop. Owner task
`01a05629-d51a-7a00-bb98-a441b8ae518a`, Sol High. Single conditional read-only
Sol Max consultant `01a05634-5316-78c0-bb36-f9cdfd3b562e` (local) was created
after scheduler and independent auditor established the complex carried-phase
blocker. Reuse that task for qualifying later blockers; no other escalation.

Explicit user instruction (2026-08-31): preserve every bug present in the
original game and comment it. Confirm native behavior with ROM/routine/evidence
before labeling it an original bug; preserve quirks and document them near the
translation. Unresolved port/native differences are not permission to improve
the original or to excuse a port defect as a native bug.

## Source and artifacts verified at resume

- Primary repository `C:/Users/joshs/Projects/nba-live-95-c-port`: clean main
  `caa91344879d3bdb408da86dc34683c6ed01ca2c`; live remote main matched.
- Preserved WIP `c134f85de7e88b4ffd9017908322a84c295b0e42` remains on its
  original pushed branch. Integration worktree `.analysis/worktrees/completion-owner`,
  branch `work/completion-owner-20260830`, reconciliation commit `52c2899`.
  Main's pause document retained; STATUS conflict resolved to resumed status.
- Original ROM `F:/Games/SNES/NBA Live 95 (USA).sfc`, 1,572,864 bytes,
  SHA256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
- Mesen executable at handoff's WinGet path: SHA256
  `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
- Primary desktop EXE SHA256
  `18fea1fa239680de2337bf0c4bdfd97085fd40f6e315b1cf2a681e0c8e138694`;
  pack SHA256 `c7b90d9347c257e0746da7a6d5595e603ffd9d3a026666fe6e62c4f483e75a92`.
  Neither changed by resumed integration. No ROM, pack, capture or executable is published.
- No AGENTS.md found in repository or its absolute ancestor chain.
- All historical evidence remains in primary `.analysis`; worktrees must use
  explicit absolute evidence/pack paths, their own fresh build objects and private
  capture directories. Do not copy old worktree source over the committed WIP.

## Ownership and next checkpoint

| Owner/worktree | Exclusive scope | Next gate |
|---|---|---|
| Root / completion-owner | main.c headless driver, integration, source manifest, build/test orchestration, status; all commits/pushes | Actual held/release menu journeys, native configuration checks, unchanged native fixtures, independent review |
| scheduler / completion-scheduler | New scheduler modules; setup transition/resource portions in nba_setup_screen.c/.h; scoped probes/captures/docs | Source-derived work/epoch/DMA contract; first and repeated transitions; no empirical delays |
| controllers / completion-controllers | New controller modules, tipoff controller/ball-assignment portions, Player Setup ownership, scoped tests/docs | Full allocator and team-preserving transfer, normal left/neutral/right routes; preserve new-match reset |
| config_audit / completion-auditor | Independent audit docs and verifier-integrity tests; no production edits | UI-to-runtime inventory, legacy-test migration, independent acceptance review |

Agents do not commit/push, share object directories, or change main/desktop.
Root integrates only reviewed bounded changes and records remaining failures.

First reproduced inherited blocker: src/main.c scripts publish only pressed;
native Setup producer consumes held. Thus `--setup-menu rules` stays on Main.
Repair driver, not native input. Correct fresh state is Main[0,0,0,3]; Rules
[0,0,1,0,0,0,0,0,1,1,1,0,0]; Options[30,30,2,1,0,0,0]. Historical
Simulation/3min cases require real menu configuration and explicit phase mapping.

## Remaining-work table (acceptance categories stay separate)

I=implemented, W=production-wired, N=bounded ROM-verified, C=C-regression-tested,
V=visually/audibly reviewed. A=approximation present; M=missing/materially unverified.
Prior bounded results below are inherited evidence, not fresh combined acceptance.

| Area | Current categories and exact limits | Remaining observable behavior |
|---|---|---|
| Intro indexed renderer | I/W/N/C/V at A2:303 EA frames,5 text samples,31 brightness rasters | M full boot waits/input/audio/title handoff; A old audio sequence; decoder incomplete |
| Rules first open/hold/return | I/W/N/C/V at A1:147/208/342 frames; WIP publication changes not combined accepted | M reentry epochs/live VRAM, complete upload timing and arbitrary values |
| Configuration UI | I/W/N/C WIP:730 checkpoints,1770 adjustments,47 canvases; V sampled phase-misaligned screens | M combined CLI journeys, runtime effect of each option, disk semantics; A captured glyph resources |
| Team initialization | I/W/N/C WIP identity128 words/two pairs only | M full native initialization clocks/RNG/actors, asymmetric substitution, natural C journey |
| Human controls | M allocator/normal callers/ownership/actions; some dormant I/N helpers | Natural human-v-CPU left/right and neutral CPU lifecycle |
| CPU/ball/rules/lifecycle | I/W many bounded N/C slices; A/M whole-game differential62/449 initial words differ | Synchronize entry then first-divergence repair; complete all rule and lifecycle paths |
| HUD | I/N/C WIP leaf projections, W missing; asset286 absent | Native caller, shared NMI/RNG/event state, uploads, correct visible teams/clocks/scores |
| Audio | I/N/C WIP2612 event dispatches with injected captured return-A; W missing | Native allocator/queues/gain/sequence/shared RNG/NMI; audible review |
| Other transitions/resources | Mixed I/W/N/C; A/M timing/presentation and provenance gaps | All normal/repeated transitions and every graphics/audio consumer audited |
| Season/Playoffs/Load/persistence | M complete original modes and disk/SRAM semantics | Natural save/load/new-process journeys, full original feature behavior |

Next root action: repair input driver, fresh build with preserved candidate pack,
replay actual menus and bounded configuration gates, retain first failures, request
independent review. Main/desktop stays at A2 until a coherent checkpoint passes.

2026-08-31 input checkpoint update: driver repaired and frozen at
`build/headless-input-freeze.json` (main.c SHA256
`cb230e8f8b28ead7c589b976784b1e04f444b252aa8c55bd1c0808fa326080df`).
New CLI input-only native replay:730 configuration checkpoints and57391 actual
held/pressed/released frames pass; five automatic C menu journeys and ten malformed
script cases pass. `build/headless-input-v1/report.json`, log and exact scripts/CSV
retained. Existing bounded configuration gate freshly passes730/1770/47 in
`build/config-resume-v1/`. Candidate pack rehashed951f8233...; source binary
`c34b09ba6a23143b76ce85a4dc14d3dd31746559a23f71e0ce1d29678764f51a`.
Independent acceptance audit initially failed, then passed corrected freeze-v4
(`build/headless-input-freeze-v4.json`), main.c SHA256
`0f07289f` prefix (full value in freeze), test SHA256 `b0892ae` prefix.
Final owner replay is `build/headless-input-v5/`; exact independent review is
`docs/completion-headless-input-independent-audit.md`. No main acceptance.
Root next migrates legacy transition tests to explicit configuration and measured
input/dispatch phase, without changing native fixture bytes. First/repeated
transition scheduler and whole-game gates remain unresolved.

First Rules mappings restored with actual Simulation/3min menu taps:71 reveal,
137 hold,147 complete-opening and2x171 return RGB frames pass unchanged native
fixtures; PPU counts as documented in `docs/headless-input-contract.md`.
Full suite attemptv1 failed on CRLF conversion of hashed ball schema; explicit
LF checkout restores exact original bytes. Attemptv2 passes that native gate
and proceeds to `test_snes_mode1.py` historical frame1000 layer-winner counts.
Independent controlled2x2 legacy/current Tipoff versus fresh/legacy configuration
attribution is underway in completion-auditor. No golden changed; no gates skipped.
Root is preserving this accepted input checkpoint on the integration branch,
then continuing configuration/transition regression migration and gameplay work.
