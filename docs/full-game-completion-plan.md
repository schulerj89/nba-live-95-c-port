# Full-game completion plan

Updated2026-08-31 from integration `d0aa808` on
`work/completion-owner-20260830`. This is the current execution plan; the earlier
dated plan and frozen audits remain historical evidence. **The game is not
complete.** No completion percentage or delivery date is inferred from source
coverage, passing helper tests or screenshots.

User review of the current preview exposed release-blocking visible defects in
Team Select, Player Setup, intro/lineup control and text, court/logo rendering,
jersey numbers, inbounds and ball/hand attachment. The immediate execution
order is temporarily replaced by the
[visible defect recovery plan](visible-defect-recovery-plan-20260831.md). The
remaining phases in this document resume after that route passes.

## Current position

The integrated HUD score/clock/expiry repair, contact interruption/recovery,
culling, draw-direction and reported pass body-pose corrections are reviewed and
pushed. The latest runtime screenshot checkpoint is `744809a`:33 inspected
images and195 checked links. Later commits change documentation, C-only
expectations and dormant bootstrap/graphics prerequisites; they do not change
the visible runtime. Main and the standing release desktop executable remain
separate; a current isolated preview build is available for progress review.

Independent controls attributed the remaining3480/6932/6954 C-only image
expectations to the accepted HUD and pass-direction/body changes. After the
three reviewed strings were migrated, a fresh63,800-frame capture passed the
complete maintained CPU gameplay regression without trace reuse. This removes
the old combined-suite stop; it does not complete the CPU match lifecycle or
establish native frame parity.

Receiver B468/selected AF66 and many period/human components are accepted in
isolation, **not wired gameplay features**. Normal controls remain forced neutral.
Season/Playoffs/Load Series currently log their selection without entering their
retail scenes; postgame uses a host return flow and disk persistence is absent.
These current callers were rechecked for this update.

The normal reset/upload/F1 prefix through80BC is in the repository with its
repaired v4 verifier. First DMA fill through80C0 and reset tables through8145
have now passed bounded independent review with their repaired readers; root
has imported that accepted prefix as a dormant integration prerequisite without
adding it to the production manifest. New first-NMI work through pre-OAM8184 is
under development. None of these prefix stops is a complete boot, sound system
or Rules timing fix.

## Owners and work running now

| Track | Owner | Current deliverable | Next required integration |
|---|---|---|---|
| G1 runtime graphics agent | `runtime_graphics` | Separate body/head mirror and ordering; real animation-status stores; ROM-derived draw-input tables and preserving asset upgrade | Actual draw/telemetry callers consume source-produced inputs; all facing combinations and pass stages checked |
| T1 startup/NMI agent | `startup_nmi` | First NMI/early handler from normal power-on, then reached OAM/DMA/controller/SPC work | One game-lifetime bus/clock through real boot and transitions; no scene-level reset or canned read |
| V1 independent review | `independent_qa` | Attribute3480/6932/6954 expectations and review G1/T1/G2 candidates | Independent source/caller, integrity and visual/audio acceptance; maintain the feature ledger |
| G2 shared graphics and integration | Root | Integrate accepted startup prefix; implement the canonical graphics-bus aliases, semantic queue drain/writeback and reached producers | Real draw order, queue/cache/codec aliases and live B468 input, then receiver/period wiring |
| M1 modes and saves | Root, reassigned explicitly when capacity opens | Extend the reviewed retail/SRAM inventory into concrete transaction and scene tickets | Original configuration persistence first, then complete Season/Playoffs/Load journeys with real match results |
| Complex blocker advice | Existing Max task | Read-only source/architecture advice on minimized unresolved blockers | Implementer reproduces the answer; QA reviews the result |

Use the existing three agents. QA performs the bounded screenshot role at visible
checkpoints; it is not a fourth simultaneous worker or an unattended monitor.
Implementers fix and deliver code, real caller integration and tests. Root alone
merges shared `NbaGame`/session/manifest/asset changes. Isolated worktrees and exact
patches prevent concurrent edits to the accepted integration branch.

Max remains the existing task **Diagnose native NBA95 Rules scheduler…**,
`01a05634-5316-78c0-bb36-f9cdfd3b562e`. Reuse its current settings. Send exact source
PCs, revision, first divergence, competing explanations and a falsifiable question
when a real dependency blocks progress. Max does not implement, capture or commit.
The queue answer is recorded in
[the graphics consultation](completion-graphics-queue-consultation-20260831.md).

## Completion sequence and exit gates

### 1. Finish the live rendering and shared-state prerequisites

G1 completes the original distinct upper/lower/head flips, head/jersey ordering
and animation-status publication. The new versioned draw-input asset must derive
all2112 table bytes from the user ROM and preserve every existing payload.
Keep the already fixed body-resource/ball-coordinate relationship intact.

G2 establishes one persistent12-record draw permutation and source one-pass
FC80 ordering, keeping exceptional FBFF full sorts at their actual callers.
Canonical WRAM owns overlapping DP bytes, queue0100..02FF, cursors35/37 and
budget39. Codecs, graphics, NMI and gameplay use that same storage. Implement
reached cache/allocation/jersey/append and consumer branches with exact widths,
overlapping stores, zero-terminal writes, wrap and budget-stop behavior.

The shared-bus interface has one ownership split. G2 owns the aliased storage,
source-semantic queue drain and exact head/budget/palette writeback. T1 owns
interrupt cadence, NMI entry/return and calling that drain at the reached source
point. Neither lane creates a second queue or an independent NMI loop; root is
the only lane that merges the interface into `NbaGame`, the production manifest
and session lifetime.

Court decoder/cache cuts do not reset all ring state. Execute their source
writes and derive the carried cursor predecessor from boot/menu work; do not
seed012C or01B8, assume an overwrite after a fixed frame count, or invent a
per-frame drain. Typed predecessor tests can precede timing closure, but they
cannot establish production initialization.

**Exit:** ordinary production draw calls and B468 entries receive C-derived
state; pass/shot/receive/contact views, offscreen/culling, pause and new-match
transitions pass source/state and visual checks. No private duplicate queue,
physics nudge or snapshot-fed initialization substitutes for the missing owner.
On a clean checkout, the accepted asset table is regenerated from the user's
ROM, its byte identity is recorded, and every pre-upgrade pack payload identity
is proven unchanged before production code may consume the new table.

### 2. Wire receiver state and complete the CPU gameplay timeline

`runtime_graphics` and root compose the real catch gate, ordered lane inputs and accepted
B468/AF66 child/parent. Preserve the inherited passer profile while selecting the
receiver and the original DBR:$012C read. Consolidate actor+56/+58/+60 and shared
09DA..09EC only with all affected producers/consumers mapped.

Wire the accepted period entry/formation/role/appearance/support components into
the actual restart caller. Preserve full native fraction words, roster/controller
identity, basket orientation, channel queues and carried dead-ball/inbound state.
Implement newly reached child refusals; do not coerce their inputs into a tested
subset or rerun a complete new-game initializer at the next period.

**Exit:** a naturally elapsed production CPU game crosses Q1/Q2, halftime and
side change, Q3/Q4, tied regulation and repeated overtime, then reaches the
source final-game state and hands its real state to the still-open presentation
children in Step4. A second game repeats the gameplay timeline without leaked
state. Controlled expiry/rare cases supplement those journeys. No stuck
possession, unreleased pass, lost controller, stale formation or host-invented
final transition. This gate alone is not credit for the complete CPU match
route; final/statistics/break presentation must also pass Step4.

### 3. Make human play and all match events complete

Repair the rejected launch-verifier routes, then compose real input publication,
requester/processed marking, actor dispatch, B pass/switch/movement, release/catch,
shots/dunks/layups, defense/steal/block and all non-B branches. Human action work
can proceed before complete hardware timing, but normal enablement requires its
full action and lifecycle gates; one working B pass does not lift neutral policy.

Finish inbounds, free throws/bonus/and-one, fouls/foul-out, injury/replacements,
timeouts, every pause entry, manual/automatic substitutions and requester routing.
Audit all13 rules and all24 exposed settings from UI to actual consumer, including
all enum values, slider boundaries and known coupled cases. Inventory original
behavior rather than impose modern NBA rules or invent unnamed menu functions.

Implement usable physical host input for the original supported arrangements;
five software records do not establish five physical players. Include held,
released and simultaneous buttons, neutral control, device loss/reconnect and
ownership after pause/substitution/period/new-match changes.

**Exit:** whole human-left, human-right, neutral CPU and each source-supported
multiplayer arrangement complete real matches and all action/event families.
Every setting has a demonstrated effect; no silent CPU override or skipped child.

### 4. Complete continuous startup, transitions, HUD and audio

T1 continues the accepted normal power-on state across NMI entry/return, DMA,
controller polling, timers, DSP/SPC initialization, CPU/APU ports and sound
commands. Place the owner outside the cleared scene union. Preserve cycle and
pending-bus state across unsupported boundaries until the required owner exists.
Original03DB bytes C4 F3 are MOV $F3,A with a read-before-write bus cycle; old
planning prose calling it OR is superseded by the actual source.

Wire real boot/legal/EA/title/Setup paths and all original skip/cancel/return/
attract routes. Replace the legacy gameplay-audio adapter/private RNG only after
the real sound return/allocator/command ordering is owned. Finish voice priority,
mixing and music/SFX/off/mono/stereo/crowd options; listen to captured output.

Close repeated Rules entry (retained158 mismatches, first native1176/C893), the
HUD08F6 interrupt-crossing mismatch and paused timer behavior. Implement remaining
statistics/ad/foul-clear and break/final presentation children using original
indexed resources. Do not substitute a fixed delay, captured palette/frame,
recorded command schedule or permanent scoreboard.

**Exit:** continuous cold-boot/menu/game/pause/return journeys match their source
state and visible/audio boundaries across differing normal dwell durations.
Full source timing, port handshakes and sustained audio are verified separately
from standalone semantic tests and the software reference profile.

### 5. Deliver every retail mode and persistent save/load

Use [the retail-mode/SRAM inventory](retail-mode-state-inventory.md) as the source
map, not as mode-completion credit. Configuration/Custom/team confirmation have
separate original save transactions.17A7 is Season length26/52/82, not controller
selection; native left/right and saved right/left ordering must remain distinct.

Root first implements source-defined save validation/transactions and isolated
host storage, including matched original defaults. Test process restart, missing,
invalid, truncated and interrupted host writes without overwriting user saves or
changing original validity behavior. Preserve unrelated SRAM bytes and save only
what each original transaction owns; session memory is not persistence.

Then implement each mode's real setup, cancellation, selection, schedule/bracket,
match/result/progression, statistics/records and terminal screens. Confirm all
formats, tie/qualification rules and Load Series scope from source/normal input.
Add simulation/roster features only where the original actually provides them.

**Exit:** a full supported Season and every supported playoff/series format reach
their original terminal screens. Mid-progress save → process exit → restart →
load → continue works with matching state and presentation. Alternate formats and
rare branches receive declared additional coverage. No label-only route passes.

### 6. Clear every release gate and deliver the tested build

Maintain one feature ledger with distinct **implemented**, **runtime wired**,
**source/native checked**, **regression checked**, and **visual/audio reviewed**
columns. A subsystem becomes complete only when all applicable columns and its
whole-route exit pass. Audit all original menus, resources and reachable branches;
unexplored original behavior remains open rather than omitted from the scope.

Run a fresh all-source build, the entire maintained suite, matched native
differentials, complete CPU/human/mode/save journeys and an endurance matrix
covering quarter lengths, difficulty, team/roster resources and control modes.
Review consecutive event/transition frames and audible output, measure actual
performance and check crash/hang/state-leak behavior. Verify reproducible asset
extraction and clean-install build/run/save instructions using the user's ROM.

**Exit:** no known port defect, missing required original feature, unowned reachable
child, unresolved regression or material verification gap remains. Preserve and
comment source-confirmed original bugs in
[the catalog](known-original-game-bugs.md). Prepare one independently reviewed
release candidate and rollback instructions; only then resolve the standing
main/desktop promotion restriction and smoke-test the actual installed launch.

## Current feature ledger

"Bounded" means the cited component/route only; "open" is not a failure claim
about every part of that subsystem. Acceptance details remain in the receipts.

| Area | Implemented | Runtime wired | Source/native | Regression | Visual/audio |
|---|---|---|---|---|---|
| HUD score/clock/panel expiry | Bounded | Yes | Bounded; timing open | Bounded pass | Reviewed scenes |
| C1 contact, culling, pass body/direction fixes | Bounded | Yes | Bounded pass |63,800-frame guards pass | Exact-build gallery |
| Separate sprite mirrors/order | In progress G1 | Open | Literal component checks; runtime pending | Pending | Pending |
| Shared graphics queue/order | Components exist | Open G2 | Partial | Pending live closure | Pending |
| Receiver B468/AF66 | Accepted component | Open | Bounded pass | Component pass | Live route pending |
| Period/full match/postgame | Partial/components | Incomplete | Bounded only | Full journey open | Full journey open |
| Human/physical multiplayer | Partial/components | Normal play gated | Bounded only | Whole action/match open | Pending |
| Rules, pause, substitution, remaining HUD | Partial | Incomplete | Partial | Consumer matrix open | Incomplete |
| Normal startup through8145 | Accepted prefix; dormant in integration | No complete boot | Bounded pass | Prefix pass | Not full boot/audio |
| NMI/SPC/DSP, Rules transitions, audio | In progress T1 | Incomplete | Partial | Timing failures open | Full audible route open |
| Season/Playoffs/Load | Entry inventory | No retail scenes | First-entry evidence | Whole modes open | Pending |
| Disk persistence | Source inventory | No | Transactions mapped | Process restart open | Save/load routes open |
| Full regression/release | Partial | No release candidate | Incomplete | Maintained CPU suite passes; wider gates open | Endurance/audio open |

## Checkpoint routine and immediate work

1. Finish G1's actual runtime patch and accepted asset upgrade. Root advances G2
   against the canonical bus contract after importing the reviewed dormant
   startup prefix through8145. T1 continues pre-OAM/NMI/SPC work independently.
2. QA reviews the remaining image expectations and each candidate. A justified
   C image-anchor migration is separate from original-game parity; never edit
   a failing expectation merely to move past it.
3. Integrate only reviewed changes, rebuild from current source, run meaningful
   affected tests and compare actual state. Each checkpoint records what is
   wired, what remains dormant, the first remaining failure and the next ticket.
4. Commit and push each coherent accepted checkpoint. For visible changes, the
   screenshot agent captures that exact commit into dated gitignored
   `.analysis/progress-screenshots/` folders, inspects it and updates `latest`
   only after success. Preserve history and include pass/transition sequences,
   not only static overview images. Tool-only changes need no duplicate gallery.
5. Reserve the private native emulator slot through root. Never mutate the ROM,
   seed live afterstate, overwrite existing evidence/saves or stop unrelated
   emulators. Preserve rejected attempts and exact source/build/input identities.
6. When a dependency blocks one ticket, continue an independent assigned ticket
   and consult Max with concrete evidence. Do not repeatedly end with an offer
   to continue, treat a helper freeze as a finished feature, or silently relax
   the definition of a complete game.

This plan will be updated at accepted runtime milestones. New evidence can
change ticket ordering, but cannot remove a required original feature or its
release gate without an explicit scope change from the user.
