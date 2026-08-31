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

Accepted input checkpoint is committed/pushed as77788abb3e26a0b5f2755248d52e67167d9d141e
on work/completion-owner-20260830 only. Primary main remains caa9134.

Later owner work (not yet accepted as a combined checkpoint): full asset extraction
using explicit retained capture root reproduces the saved candidate pack exactly,
SHA256951f82331c4bb6ce8f381da519ee8bfdf517bf8c13f2cd6f20cfa9c34d5ed4df;
only IDs145/155 differ from main, and HUD286 remains absent. See
build/full-extraction-v1/equivalence.json. Mode1 independent source/config/pack
attribution permits only the five C-only count updates; native trajectory/HUD
parity remains unproved. See docs/completion-mode1-attribution-audit.md.

Main text source bounds restored with unchanged47 native canvases and147 opening
frames; see docs/setup-main-span-restoration.md. Legacy Setup frame162 default
hash migration remains pending. Full suitev3 passes Mode1 and subsequent gameplay55/65
gates, then fails court_runtime_probe; exact cause is under investigation.
No failing gate was skipped or promoted to original-game behavior.

Scheduler primitives are independently accepted and committed/pushed as
ab27be9abd93c27f4a95a192543868dc69d98ee7 on the integration branch only;
the module is not production-wired. See docs/scheduler-integration-checkpoint.md
and docs/setup-scheduler-consultation-plan.md. Max consultation is complete:
codec component work is resolved, audio/SPC continuation and end-to-end epochs
are not. Reuse the single consultant task only for a qualifying future blocker.

Main span freezev1 was rejected by independent review for a newly introduced
16-byte raw-canvas alias defect. Revised freezev2 uses destination-owned map
addresses and passes the18independent cases plus47native canvases,730configs,
57391input frames and1770adjustments. See docs/completion-main-span-independent-audit.md.
The early frame162 difference is not yet attributed solely to defaults:
the value overlay colors the selected logical row while the HDMA band stays
in screen coordinates during scroll. Do not refresh that golden before fixing
and verifying the compositor contract.

Court regression fixture now publishes its explicitly changed home identity to
the initialized native context; all812views/29teams/16kcallerframes/4periods pass.
New-match fixture now compares native home0/visitor1 pause side with the mapped
legacy UI side; two return journeys and unchanged startup projection pass.
Production was unchanged for these fixture corrections. Full suitev5 continues
past those gates; a complete suite pass is not yet claimed.

2026-08-31 further checkpoints: Main source-span restoration is committed as
bb8b3c0; canonical team-context probe adapters and independently attributed
C-only Mode1 counts as0034f06, both pushed only to the integration branch.
All12 affected native probe gates pass with unchanged fixtures/comparison
fields; see docs/gameplay-adapter-migration.md. Full suitev6 reaches the legacy
Setup monolith and fails its old frame162 default hash. No gate was skipped.

The Main screen-coordinate highlight correction is independently accepted:
docs/setup-highlight-independent-audit.md records a private fresh build,
all9 old-image attributions and unchanged selected native Rules gates. Fresh
native HDMA evidence supports the fixed screen band but does not observe a
populated moving-label crossing. Do not call the C entrance crossing an
original-game bug; initial-entry timing remains unresolved. Only162/166 old
default images differ after correction, by1116 changed Style/Quarter pixels.

Main values wrap. The earlier audit prose incorrectly claimed Main quarter
clamped; actual preserved logs show3,0,1,2. Only prose/comments were corrected.
A suspected --setup-down driver defect was also disproved: its scene clock
starts at-105, ENTER_FRAMES is32, and three taps at139/147/155 reach rows1/2/3.
No production driver change was made for that hypothesis.

Controller19-file checkpoint plus repaired verifier independently passes
270020 native words,45 integrity cases,200 controlled original-ROM/quirk checks,
and3055 artifact attestations. Root integration is underway; the normal
human path remains disabled. The separate10-file human dispatch and codec
continuation work are not integrated or accepted yet. Scheduler and human
action implementation continue; whole-game completion remains incomplete.

Owner controller integration now passes a fresh all-object build and combined
270020 native words,45 integrity cases,200 ROM/quirk checks,61 Mode11 calls,
39 acquisition calls, runtime/timeout/Player Setup checks and730configuration
checkpoints/57391input frames. See docs/controller-integration-checkpoint.md.
The CLI reaches LEFT through two released taps; debug output reports neutral.
The old left image differs only because original OAM usesx40 instead ofC'sx42;
the exact-offset counterfactual reproduces the old whole image. Human remains
disabled. The separate human-stage audit found an original carried-X pointer
quirk that its movement wrapper did not yet preserve; that stage is rejected
pending repair, not integrated. Scheduler codec review is queued separately.

The bounded human-stage repair now has independent acceptance. It preserves
the original carried-X controller-relative timer bug, with PCs and a natural
L+X witness documented in source. All764650 native values,25 verifier-integrity
cases,12 independent alias cases and14 source-contract guards pass a fresh
auditor build. The old v1 rejection remains recorded; see
docs/completion-human-dispatch-repair-independent-audit.md and
docs/human-dispatch-repair.md. The14 quirk guards are now included in build-Test
through a separate component build; this does not add the module to the
production source manifest or enable human gameplay. Original-game bugs must
be kept and commented when established by evidence; unexplained port failures
must not be reclassified as original bugs.

The legacy Setup monolith migration now passes the previous frame162/default,
Main shadow-strip, real-button configuration, Main commit persistence, Rules
publication trace and Options route/cadence checks. The previous blocked Rules
slider SFX assertion counted the two real Main configuration adjustments.
The migrated check now requires exactly those two sounds on a blocked slider
and a third sound on a successful submenu adjustment; volume checks select
that third sound. The whole monolith passes in
build/setup-monolith-migration-v6.log. No native fixture was changed.
Full-suite and Rules reentry failures are still unresolved.

The accepted human repair is committed/pushed as e76babe. Capture-root extraction
support is b0d6c7c, and independently accepted FB46/FB30 source-work components
are fceb788. All are on the integration branch only; production source manifests
and normal human gate remain unchanged. The codec owner check passes112814+
36418 native instruction states/durations,28218+9935 write positions,20 complete
payloads,16+18 Python tests,12+14 C contracts and both independent six-case
corruption suites. See docs/codec-integration-checkpoint.md. Producer helpers
and forward NMI/audio/SPC timing are still separate work.

Full suitev7 found an obsolete one-press Player Setup assumption; its fixture
now explicitly checks right/neutral/left and passes three full matchup/lineup
journeys. Full suitev8 passes the Setup monolith and then fails the core safety
test's missing local capture path. With the explicit read-only capture root,
both clean/headered extractions pass; the next legacy debug-state assumption
is repaired through real buttons, reproducing both old BMP hashes exactly.
See docs/controller-regression-driver-migration.md. Full suitev9 is running;
no full-suite pass or Rules reentry repair is claimed.
