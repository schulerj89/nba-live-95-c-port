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

Continuation checkpoints97c59c9,45dd555,659b4c1 are now committed/pushed on the
integration branch. Setup monolith migration has independent acceptance; the
PlayerLab driver now gives two actions three press/release/press frames while
preserving its images. CaptureRoot/RecompRoot arguments make worktree reports
consume the intended read-only evidence. Census totals/verified credit did
not increase; only the census tool identity changed. Primary/main and desktop
remain untouched.

Independently accepted switch repair1c3c60f preserves original wrapped F34F
comparisons and rejects all prior verifier corruptions. Root fresh results:
576 arithmetic guards,68480 native switch values,35 local/38 independent
integrity cases. Human pass selector/initializer checkpoint90045f3 is also
committed/pushed:42766 selector/metric and216466 initializer values,62/69 local
and39/39 independent integrity cases,609 independent initializer source cases.
These new components are not in the production manifest; human remains gated.
The original pointer/overflow/suffix/animation-lock quirks remain source-commented.

Full suitev10 failed fresh Arcade/12-minute endurance at46450; a private old-
F34F build reproduces the same stall, excluding the accepted arithmetic fix as
its cause. Root found a separate port dispatcher defect: C39C CMP#2's N flag
routes layout1 toC50B, but C had grouped1 with4 atC450. Nine fresh original-ROM
boundaries (one natural, eight explicitly controlled) verify54 target/play/RNG
words; the old helper fails exactly five layout1 cases. Failed capture attempts
are retained and rejected. The corrected source passes unchanged63800-frame
endurance, period1/post12026; no blanket target clamp is added, and layout4
retains its native404 case. See docs/inbound-layout-repair.md. Independent
review of freeze build/inbound-layout-freeze-v2.json is pending.

Fullv11 stops at changed C-only Mode1 counts. Private pre-fix-helper/matching
startup-check relink reproduces all old counts; telemetry is identical through
505 and first differs only in layout1 target/play/RNG fields at506. The updated
C census retains every57344-pixel assertion; no native fixture changes. Fullv12
then fails a controlled shot-assistance coverage window: the first unforced
basket now occurs at2356, after the fixture had reset to a long clock. Renewing
its controlled late-clock window until assistance is observed preserves all
per-frame oracles and16000 frames; fatigue-on/off cases pass with one assisted
make. This fixture-only change awaits review. Fullv13 is running; no full-suite
pass is claimed. Rules reentry still has158 mismatches and remains uncommitted.

Producer/header source work has fresh positive comparisons, but their first
verifiers accept impossible native chronology/PC metadata. Their v1 freezes
and rejection reports are retained; scheduler is preparing v2 verifiers.
Do not integrate or claim acceptance yet. AC50/B00B action11 is frozen pending
review, and the controller agent is continuing AD0E/F473 in new files.

The user reaffirmed that the separate Max task is for complex consultation,
not delegated implementation. Its record was checked: source/trace inspection
and in-memory diagnostic calculations only; no file changes, implementation,
commits, or takeover. The single consultation is complete and idle, and no
new request has been sent. Existing implementation/audit subagents remain
separate from that read-only consultant. Preserve this distinction.

The inbound source/Mode1 composite, strict v2 verifier and shot fixture have
independent acceptance and are committed as9f5b47c. Fullv13 passed those gates
and stopped at three old C-only gameplay85 hashes. Fresh matched-source
controls reproduce all old hashes; a configuration/team-context matrix and
pre-layout/current source comparison attribute the new three trajectories.
Only comments/three expected values change, preserving all48000 frames and
semantic guards. New standalone run passes; independent audit is queued.
See docs/gameplay85-regression-attribution.md and root freeze
build/gameplay85-regression-freeze-v1.json,627identities.

Producer/header v2 composites now have independent acceptance. Root checked
all51+49 freeze references, copied36 new source/audit files, compiled both
probes fresh and reproduced155750+126 states,40003+66 writes,27726+8206 DMA
bytes, both13Python/11C contracts, both9local/5independent mutation suites,
and10 shared protocol tests. Committed342f171; standalone only, no production
scheduler or DMA/phase acceptance. See producer-header-integration-checkpoint.

Fullv14 passes gameplay85 and the Rules Custom caller, then stops in closure
initialization. A private earlier run first found its four consecutive Down
frames were one held word (code12); the released-frame test repair was applied
before v14 compiled that probe. The resulting code60 exposes a separate port
startup defect: Philadelphia home has no original profile3E>=85, but the
synthetic boosted-pass self-test required one from the selected home team.
A local session copy using the known Orlando fixture preserves all lifecycle
assertions and original ratings. Old initialization fails29/841 pairs; new
initializes841/841, and all812 common complete owned states are byte-identical.
That repair and new all-team gate are frozen pending review. The revised
closure reaches6000 gameplay frames twice but its aggregate C digest remains
unaccepted pending separate attribution. Fullv14 is not a full-suite pass.

Controller action/aligned source slices pass positive native comparisons,
but independent audit rejected their original stderr protocols. Separate v2
verifier-only freezes repair that without changing source/native fixtures;
reviews continue. New AF1D pose/attachment work is separate. Sound prefix
reaches44 explicit pending SPC reads and two sequencer stops; independent
audit is queued, and scheduler is deriving normal initialization/upload.
Human play remains gated. Rules reentry retains158 unresolved mismatches.

The later independently accepted startup/closure/gameplay85/tipoff attribution
is committed through dcb1eb8. All841 team-pair initializations pass, with the812
common owned states unchanged. GameplayLab attribution364493f and the bounded
human pose component d143411 are accepted. Actor execution telemetry8c97b5f
records actual completed passes instead of inferring them during the period
wait. Across63800 rows, only1189 scheduler records change;31,305 actual actor
passes agree and all841 initializer states remain unchanged. Camera wait and
shot/scheduler fixture guards pass. These results do not establish native
hardware phase or whole-period timing.

The five CPU image hashes and three layout assertions now have independent
source attribution and are committed64f38d4. All prior controls and25 image
comparisons were retained. The current CPU test correctly stops at frame49412:
the resumed period still has the wrong inbounder/formation. That is an actual
port failure and has not been hidden by changing the test. No fullv16 suite
has passed or been claimed. Rules reentry still has158 mismatches.

Sound prefix/init/resident composites58cbc46 are independently accepted as
standalone components. Fresh root comparisons cover3673 CPU prefix states,
7055 CPU initialization states,182 SPC resident states, their ordered accesses,
full snapshots and transfer bytes. Normal CPU/SPC startup integration, clock
visibility, timers/DSP and hardware phase are still incomplete. The separate
SPC initializer/control C source passed independent ROM comparisons, but v3
verifiers accepted malformed pending/source metadata. Their unchanged v3
rejection remains recorded; scheduler has supplied separate v4 verifier freezes
for review. Production nba_spc/audio behavior is unchanged.

The catch component d007929 matches31788 natural values,93 local integrity
cases and24 independent cases; its corrected audit inventories287 identities.
The saved-frame return a7915e8 matches189100 values across25 calls and rejects
112 local/25 independent corruptions. Mode15 release daf35d3 now matches
2878464 values across517 calls/30 origins, with126 local/44 independent
integrity rejections. All have fresh root builds and bounded independent
source reviews. They remain outside the normal human-play gate. Launch99C4
is separate active work; the last right-side release cleanup is explicitly
unfinished at the captured endpoint. Root retry command mistakes are retained
in the respective integration directories, without changing accepted inputs.

Accepted commits through daf35d3 are pushed to work/completion-owner-20260830.
Primary/main and the desktop executable remain unchanged. The separate Max
consultant has stayed idle and read-only; it has not implemented any of these
changes. Existing controller/scheduler implementation agents and the reviewer
continue in their previously authorized worktrees.

Period repair now has ten isolated native expiry captures,336 full128KiB WRAM
snapshots and573 frozen identities, recorded in
build/period-restart-native-freeze-v1.json. Expiry uses only the declared six
WRAM seed words after real menu input and natural court ownership/readiness;
there are no ROM, PC, stack, formation or ready-latch injections. The original
period branch bypasses the new-game bulk clear. Consequently09BA and09B0/B2
survive; actor coordinate fractions also survive, and the formation loop writes
pair index0..4 to+A6 on both teams. These original quirks must remain and be
commented. The old foul-inbound parent is inappropriate for period restart.

The first standalone period-parent freeze passed28420 native typed words but
was independently rejected: opening/OT positive-anchor formation omitted the
source Y negation, and its verifier admitted decimal/entry-register metadata
corruption. That is a port mistake, not an original bug. Scheduler is preparing
separate v2 source and verifier files; frozen v1 and its failed expectations
remain unchanged. It is not integrated into production.

Root's unwired period AAB2 projection passes forty native child calls and5200
actor/RNG/owner-pointer words,14 protocol rejections and9 domain refusals. It
is frozen for review at build/period-appearance-freeze-v1.json (931identities).
The separate assignment/sort/attachment candidate passes eleven native calls
and34126 bytes,17 protocol/metadata rejections and9 domain refusals. Its freeze
is build/period-support-freeze-v1.json (856identities). The latter composes real
BC9B/cancel/install/AEC3/B649/B66A operations without native poststate seeding.
Neither candidate is accepted or wired yet. BC07 role suffix, video child,
integrated restart and whole-game/native phase remain outstanding.

### Latest accepted components and continuous formation checkpoint

This update supersedes the pending component statuses immediately above.
`5684b5e` accepts the corrected period-parent v2 as a standalone component:
28,420 native words, independent original-ROM cases and source CPU-domain
checks pass. Opening/OT positive-anchor Y negation is corrected as a port bug.
The original carried latches/fractions and A6 pair index remain documented.

`979c042` commits unchanged appearance/support C after independent final
verifier acceptance. Appearance v3 freeze is
`d61ea18b12a15d90ecfbba1358bbb90ecdb57b6e8fda2f4762b198eebd9f3ce1`
(1414 identities); support v2 is
`7a34dbcab0846133893dbd6d4c0a3b299f6bab94e17f408e455d4353d142a133`
(1683). Commands now derive from complete native rows, with exact raw-field
binding and no old C-report authority. Raw support adapters require canonical
actor IDs/carried roster tables and a zero leading collision sentinel.
A fresh thirteen-source root build with no borrowed objects passes all forty
appearance and eleven support native calls. Neither is production-wired.

`0328d13` accepts standalone SPC initializer v4. All 192,818 instruction states,
64,394 writes and full ARAM endpoints match; six binary files are unchanged.
Both 21-case source sets, 4 protocol, 19 evidence, 6 independent metadata and
44 schema/callback checks pass. The original $08FF clear omission remains
commented and survives nonzero tests. F1 control v4's separate PS.P callback
rejection remains visible; scheduler control v5 awaits final audit/integration.
Production SPC/audio remains unchanged.

The original $80:FBFF child is a draw-depth sort, not video DMA. Root's bounded
render-tail source and v2 verifier passed independent review, including 384
controlled original-ROM cases. Final freeze
`build/period-render-tail-freeze-v2.json` is
`b9362eab95705e0b0c4fcab5a8f6ce85b846de9f32a28a1dfc90dcdc79e90e96`
(2100 identities). Source and all old evidence remain unchanged; the repaired
verifier rejects duplicate/reordered/inserted hooks. Native periods0/1 cross
one frame during E1F7-E207 while2/3 do not. Owned bytes match without a timing,
CPU, DP or interrupt-parity claim. Root standalone integration is next.

Root's diagnostic DD97-to-E207 composition consumes one before-state per
capture and executes every included C child on the preceding C output.
All 125 native checkpoints / 504,500 owned bytes pass, with 27 malformed-output
checks and 8 explicit domain refusals. Freeze
`build/period-composition-freeze-v1.json` is
`075660a6bca1e2aac08683bd0f6b5f0267e8d9b016db8ebde3a09e5227e5c32c`
(2674 identities). Controllers is independently reviewing this root-authored
composition. It uses role-prefix v1 only for observed cadence early returns;
rebuild/planner paths are refused. It is diagnostic, not a production raw-WRAM
runtime. Scheduler is building a canonical typed composition for a subsequent
NbaTipoff adapter, including role v2 and explicit unresolved source stops.

Controllers supplied the inline DCA6-to-DD97 entry prefix, freeze
`a96b329445557aaa0e4d6976e97a14b9f518d0cbd55ccaaed4ff36893b3e3ea8`.
It resets source-owned animation fields, preserves queue contents and original
latches, selects the original OT clock table and flips anchors only for raw
period2. Native/source/protocol checks pass; independent review is pending.
The role continuation v2 source passed independent source cases but required
a separate byte-width verifier repair: role v3 freeze
`40f6762fa9310fa4ac83f2f8fc427e689594591952c45ce8e71bd1048f928667`
(1056 identities) is queued for final review. Human launch remains frozen
separately at `87326e441bd90f6e8b921075724683e40e7b1cf94ad69b6949c471a5cbfe45a8`
and awaits independent review.

No production period repair is accepted or wired yet. The known CPU failure
at frame49412, rules-reentry158 mismatches, HUD gaps, hardware scheduling and
full-game acceptance remain open. Main/desktop and the read-only Max consultant
are unchanged. Existing implementation agents continue only in their own
previously authorized worktrees; root owns all commits and integration.

### Accepted entry, roles, control and diagnostic composition

This update supersedes the pending statuses above. `73b04a5` integrates the
accepted standalone render tail with a fresh build and all four native cases.
`5cb2331` integrates SPC control v5 after independent acceptance, including
the direct-page callback constraint. Source-only non-callback API cases keep
their documented domain. Production SPC/audio is unchanged.

`f9bbd76` integrates the accepted DCA6-to-DD97 entry prefix. Fresh root checks
pass 786,432 full native words, 90 controlled source cases and 56 corruptions.
`ab44869` integrates role source v2 and the accepted v3 verifier. Fresh root
checks pass four native cases / 892 final fields, 116 controlled original-ROM
cases, 13 API contracts, 17 local protocol corruptions and 12 independent
malformed outputs. Both remain standalone, outside the game source manifest.
Their source comments preserve original carried latches, fractions, queues,
wrapped arithmetic and selection behavior. Old rejected evidence is retained.

Controllers independently accepts root's bounded diagnostic composition with
verifier v2, freeze `c187b92ab3a393899fa6a4b31f42b69bd6e811378fa2de38d330665aebd1548b`
(2925 identities). Its fresh 44-source build reproduces all 125 boundaries /
504,500 owned bytes and complete original C output files. V1's incomplete
build manifest attestation is rejected. V2 requires exact source/object key
sets; all 11 invalid manifests and 56 further protocol corruptions reject.
Before-state-only and carried-marker tests pass. This is still diagnostic
raw-state glue, not a production raw-WRAM runtime.

Scheduler has separately frozen the canonical typed composition at
`.analysis/period-formation-freeze-v1.json`, SHA256
`8265eb8e8e71e6c59186b2a0526d9ea75c02fe96a80ccf18bed5507dde41e244`
(1594 identities), for independent review. It reports 125 native checkpoints /
128,500 canonical values, controlled source tests and strict refusals. Each
overlapping child field has one owner; source alias slots09DA/09DE/09E2 are
shared. Unmodeled carried scratch and unresolved assignment children stop
explicitly. Root has not accepted or wired this candidate yet.

Root mapping found that basket X already has a runtime owner:
`court_presentation.basket_x_3fef`, updated by the original camera presentation
branch. The adapter must carry that value across the period-two anchor flip;
it must not derive it from the newly flipped anchor. The persistent 12-record
draw order and its ordinary FC80 adjacent pass still need runtime ownership.
Basket Y has an original explicit zero writer at86:DBC2. Further mapping must
distinguish actual initialization from fabricated default inputs.

Human launch C passes independent natural, controlled and original-opcode
checks. Its verifier remains rejected because return tag/PC/PS substitutions
can disagree with entry operand signs. Controllers will repair only the
verifier in a new packet, retaining the original freeze and rejection cases.

At the user's request, controllers also captured the actual current C build
into primary `.analysis/progress-screenshots/`. This path is already ignored
by `.gitignore`; there were no main, desktop or tracked source changes. The
local `latest/index.html` gallery links ten named views, dated runs, exact
build/capture manifests and a repeatable capture script. The static score
panel is labeled as a current port defect, not an original-game quirk.
The images do not claim separately audited components are production-wired.

The known CPU frame49412 failure, rules-reentry mismatches, HUD replacement,
hardware scheduling and full-game acceptance remain open. The separate Max
consultant remains idle and read-only. No production period repair is yet
claimed complete.
