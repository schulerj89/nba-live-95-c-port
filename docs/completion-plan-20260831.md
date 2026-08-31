# Plan to finish NBA Live '95

This is the earlier planning baseline. The current execution order, accepted
checkpoints and remaining full-game gates are in
[the updated full-game completion plan](full-game-completion-plan.md).

Date: 2026-08-31. Planning baseline: integration commit `f318478` on
`work/completion-owner-20260830`. **The game is not finished.** This plan replaces
the old next-action ordering; historical audits and frozen evidence remain valid
only within their stated scope. No completion percentage or delivery date is
claimed before the remaining original feature inventory is closed.

## What finished means

The delivered executable must support the original game's complete menu and
gameplay routes, human and CPU play, all match events, all original retail modes,
and persistent saves. Original rules, settings, visuals, audio, state ownership
and timing must be implemented and checked through real callers. A working
Exhibition match is an intermediate milestone, not the final product.

Every required feature needs five separately recorded results: implementation,
production wiring, original-source/native verification, C regression coverage,
and visual/audio review where applicable. A helper-only pass, image gallery,
instruction coverage count, successful compile or completed trace cannot satisfy
the other columns. No required feature may remain missing, approximated or
materially unverified when the game is declared complete.

Confirmed original bugs stay preserved and commented in the relevant code and
[original-bug catalog](known-original-game-bugs.md). An unexplained port failure
must first be attributed; it is not an original quirk by default.

## Team and ownership

Use the existing three sub-agents. They are already reviewing this plan's
gameplay, timing and completeness tracks; no additional user-owned task is needed.
Implementation tickets require fixes and working runtime integration proposals,
not investigation-only reports. The auditor remains independent of the code
being accepted.

| Owner | Implementation/review responsibility | Shared-file rule |
|---|---|---|
| Root | Integrate accepted patches; session/scene dispatch coordination; feature ledger; retail modes/save work; combined tests; commits, pushes and release candidate | Sole owner of `nba95_sources.txt`, final integration source, release/status documents and main/desktop promotion |
| Controllers sub-agent | CPU/ball/pass defects, state aliases, period restart adapter, human allocation/actions, match rules and lifecycle | Ticket-scoped ownership of gameplay files in a fresh integration-based worktree; request root coordination for session/scene interfaces |
| Scheduler sub-agent | CPU/NMI/DMA/SPC timing, transition/resource publication, HUD remaining children, boot/audio work | Ticket-scoped frontend/audio/timing files; no concurrent edits to controller-owned gameplay files |
| Independent audit sub-agent | Required-feature inventory, original/source attribution, verifier integrity, review of each candidate and final acceptance | No production edits to the candidate being reviewed; report precise failures rather than changing its expected outputs |
| Existing Max consultant | Read-only advice on demonstrated complex blockers | No implementation, commits, captures or ordinary task delegation; root/team implements and independently verifies recommendations |

The existing Max task is **Diagnose native NBA95 Rules scheduler…**,
`01a05634-5316-78c0-bb36-f9cdfd3b562e`. Keep its current settings and reuse it;
do not create another consultant. Escalate as soon as a minimized failure and
source analysis expose an unresolved architectural/timing contract or competing
interpretations that prevent a correct next change. Send the first divergence,
exact source/capture identities, rejected explanations and one falsifiable
question. Do not spend repeated turns guessing or send routine fixes to Max.
Continue independent unblocked tickets while advice is pending.

A read-only follow-up to that existing task returned a
[concrete timing recommendation](completion-timing-consultation-20260831.md):
derive normal reset/IPL/upload into a persistent game-level clock/bus owner,
then cross the accepted SPC F1 boundary without reseeding. Its prior diagnosis
resolved FB30 output/work but not complete phase prediction. The recommendation
now informs S1; implementation still needs independent verification. The C1
gameplay repair and original-mode inventory can proceed independently.

All older worker branches were preserved as archival snapshots. Start each new
implementation ticket from the current integration commit in a fresh worktree;
bring in only the reviewed component/dependency required. Never merge an entire
archival branch. Use private build directories and preserve frozen bytes.

## First implementation assignments

| Ticket | Assigned owner | Concrete deliverable | Dependency/review |
|---|---|---|---|
| C1 | Controllers | Fix the source-attributed pass/contact error at41876, or correct the test domain only if the original proves it wrong; verify the real actor loop | Can start now; auditor checks original behavior and new regression |
| S1 | Scheduler, root for game-level interface | Compose original reset/IPL/upload into a persistent CPU/SPC owner and reach the real F1 publication/0387/03DB boundaries without seeded state | Incorporate Max's bootstrap ordering; auditor checks state/cycle provenance; further DSP/acknowledgement work remains explicit |
| A0 | Independent auditor | Maintain required-feature/current-caller matrix and independently review C1/S1 packets | Start now; never treat old inventory findings as current without checking |
| M0 | Root | Inventory original Season/Playoffs/Load routes, terminal states, persistence layout and host controller topology; turn unknowns into source-backed tickets | Parallel with C1/S1; no invented formats, saves or multiplayer counts |
| C2 | Controllers | Implement86:B468 receiver producer, then canonical actor/scratch storage | C1 attributed; independent before-only contract tests and real caller checks |
| D1 | Root with scheduler | Establish persistent ordinary draw-order ownership and current-state period prerequisites | Separate from controller's tipoff edits; unlocks C3 |
| C3 | Controllers | Wire period restart through real typed adapters and close reached child refusals | C2 plus D1 and complete carry mapping |
| C4 | Controllers | Compose human caller/B route, repair rejected launch verification, then implement remaining actions before enablement | C2 plus launch audit; full enablement also needs match lifecycle |

These assignments include implementation, validation and handoff, not just
diagnosis. This request's completed work is the plan and independent reviews;
the new fixes are not claimed implemented. The detailed
[gameplay tickets](completion-gameplay-work-plan-20260831.md) and
[timing/audio tickets](completion-timing-work-plan-20260831.md), plus the
[independent coverage matrix](completion-plan-audit-20260831.md), are preserved
with the plan. Root owns ticket sequencing when an implementer finishes or
blocks, so the three agent slots remain useful without overlapping edits.

## Milestones and dependencies

Tracks 1A and 1B can proceed in parallel. Feature inventory and source mapping
for later milestones can also proceed early; acceptance cannot skip a dependency.
No agent should wait idle for an unrelated subsystem.

The numbered milestones group work, not a rule that every earlier track must
finish before a later line of code can be written. In particular, human action
implementation can proceed before full hardware timing is accepted. Complete
human enablement requires both milestone2's actions and milestone3's lifecycle;
strict timing and whole-game parity remain mandatory release gates.

### 0. Establish the executable backlog and preserve the baseline

Root and auditor enumerate every original top-level route, submenu, selectable
mode/rule/option, match event and save transaction. Cross-check the historical
inventories against current source; old claims of missing defaults or preset
logic must not be copied as current findings without checking. Each ticket records
owner, source/native evidence, state producers/consumers, implementation files,
dependencies, acceptance command/journey and outstanding limits.

Baseline failures: CPU pass-state assertion at frame41,876; Rules reentry158
mismatches, first native1176/C893 brightness; strict HUD two08F6 observations
at an interrupt crossing; older baseline period restart failure near49,412.
Keep each tied to its own executable, configuration and trace. HUD RNG restoration
changed the later C trajectory; do not impose an obsolete score/trajectory as an
original-game oracle. Current scoreboard display/expiry repair remains accepted
within [its bounded scope](gameplay-hud-integration.md).

Exit: every original route has a ledger row, every known failure has a reproducer
or retained witness, and no row equates a standalone helper with runtime completion.

### 1A. Repair gameplay state and complete period restart

Controllers owns the implementation; root integrates and auditor verifies.

1. Attribute the frame41,876 active-unreleased-pass/contact-mode case using
   original dispatch order. Determine whether code, assertion or both are wrong.
   Fix the proven error and add a meaningful regression that distinguishes the
   original behavior; never remove the assertion merely to make the suite green.
2. Translate the missing receiver entry child `$86:B468` and its real callers,
   including timer/animation/trajectory/RNG effects. Reconcile actor `+56/+58/+60`
   and the shared `09DA..09EC` scratch buffer only after their producers are owned.
   Preserve the original catch table-overrun result. Do not reuse an unrelated
   layup path or merge the earlier alias experiment without verification.
3. Wire the accepted period entry, formation, role, appearance, support and draw
   order components through a real runtime adapter. Establish basket, roster,
   queues, fractions, timing delta and carried-state ownership. Keep new-match
   reset distinct from next-period continuation.

The restart adapter depends on a live owner of the12-record permutation and
ordinary `$80:FC80` passes, separate from the period's `$80:FBFF` full sort.
Preserve all16 native fraction bits, not just the current host's eight. Reached
role/appearance `ROLE_STOP` or `REFUSED` branches (including BF51/BAE4) become
explicit child implementation tickets; do not coerce their input into an
already accepted standalone domain.

Exit: fresh production build passes the attributed pass/contact regression and
long CPU test; real gameplay crosses regulation periods, halftime side change,
overtime and final without stale formation/inbound ownership. Bounded controlled
expiry fixtures remain useful but cannot replace a naturally played full-match
journey. No copied native after-state or fabricated adapter inputs are allowed.

### 1B. Own the shared hardware timing and fix repeated transitions

Scheduler owns implementation, with the existing Max consultation for the
established CPU/SPC timing blocker. Complete the smallest source-derived next
owner, then connect it to the already reviewed codec/backdrop/header, interrupt,
sound and SPC components. Carry CPU/SPC clocks, DMA/refresh, interrupt entry/RTI,
timer/DSP work and CPU-port visibility from their actual initialization.

Reuse the already derived/audited550560-CPU-cycle backdrop work and440-cycle
header work; do not reopen the obsolete26487-cycle residual. Their work
descriptions are not a complete interrupt implementation or master-clock model.
The first bounded implementation ticket composes the0380 SPC initializer and
accepted F1 control into one live typed state: charge the pending0384 write cycle
once, commit it, resume0387 and run to the explicit03DB DSP read. Preserve output
latches, timer-edge state and the original08FF omission. This removes one real
initialization stop without claiming DSP timing is solved. The next continuation
is03DB..043F into0447, with actual DSP/timer ownership rather than a canned read.

Source correction at implementation start:03DB bytes `C4 F3` are `MOV $F3,A`,
including a read-before-write bus cycle. The preserved timing work-plan and Max
consultation incorrectly name `OR A,$F3`; their mnemonic is superseded by the
actual ROM bytes and accepted initializer source. The pending DSP bus boundary
still exists. Preserve those reports as history; do not alter frozen source to
match their prose.

That composition must start with the original reset caller and80:AB06 IPL/upload
transactions, not a captured0380 prestate. Following Max's ownership correction,
place the CPU/SPC clock/bus state beside `NbaGame.session`, outside the scene union
that transitions clear. Carry it through menus, gameplay, pause and audio muting.
Derive the power-on profile, port writes/visibility,1,264-byte upload and entry
registers; only then cross the real post-cycle F1 boundary. The bounded first
gate ends at03DB if the DSP response is not yet owned. Do not clear a pending-read
stop flag or treat a visible-input commit helper as a CPU bus-write adapter.

No fixed visit delay, captured scanline, captured return-A, atomic bulk cost or
per-frame blanket upload may stand in for work that crosses an interrupt. A
general CPU interpreter in production is outside the agreed native C approach.

Exit: predict header loaded epochs72/15/71/15 and return epochs73/16/72/16 from
real state for the retained first/repeated Rules journey; pass its complete
RGB/PPU/VRAM comparisons, including hidden writes and changed Custom values.
Use the same runtime owner to resolve the HUD clock-read crossing and native
paused timer/callback behavior; keep strict failed reports until that succeeds.
Independent verifier integrity and fresh C journeys are required in addition
to bounded source replay.
Add a second natural menu dwell duration with independently derived native
expectations. The first four known epochs must not become a hard-coded timing
table. Strict gameplay must reproduce the39945-to39944 read crossing at its
source accesses, without one-tick correction or the current bounded exemptions.

### 2. Make ordinary human play complete

Controllers completes Player Setup ownership, original supported controller/side
arrangements, allocator and switch behavior, movement, offense, passing/catching,
shooting/dunk/layup, defense/steal/block and human free throws. Repair rejected
launch/verifier packets before accepting them. Root owns coordinated scene/session
interfaces; auditor checks real input paths and malformed evidence rejection.

First compose `$87:9106..929E`, requester9165..91BF, processed marking, action
return and physical commit. Connect the accepted B-pass/switch/movement pieces
using only C-produced intermediate state; then implement the non-B children at
`$84:E2F2..E3E9` and remaining lock/reallocation continuations. A successful B
pass is not permission to remove the forced-neutral production policy.

Exit: normal menus launch human-vs-CPU on either side, supported human-vs-human
arrangements and neutral CPU-vs-CPU. Each action reaches its real runtime caller,
and team ownership survives possession changes, pauses and substitutions.
Implement the physical host input path for the original supported arrangements;
multiple software controller records do not prove distinct usable input devices.
No debug ownership seed or forced-CPU fallback counts as a playable human path.
Full enablement also requires inbound/free throws, period restart, final and a
new match with correct control reallocation, using milestone3's lifecycle work.

### 3. Complete match rules, options and presentation

Controllers finishes gameplay rule and lifecycle consumers. Scheduler finishes
clock/HUD/statistics/ad/foul-clear children, pause/menu transitions and audio.
Root coordinates shared interfaces and the option-to-consumer ledger.

Cover every original Rules and Options value through the UI and its actual
effect: fouls, out-of-bounds, backcourt, traveling, goaltending, three-in-the-key,
foul-out, shot/inbound/half-court clocks, fatigue, injuries, difficulty, period
length, shot control, CPU assistance, slow-motion dunks, music/SFX volume,
off/mono/stereo and crowd sound. Verify current readers rather than assuming the
old audit still describes current code. Test ON/OFF, bounds and discrete modes.

Complete free throws/bonus and penalties, inbounds, timeout limits, automatic and
manual substitutions, disabled choices, quarter/halftime/overtime/final screens,
postgame statistics, replay/menu return and a second newly initialized match.
Keep original player/court/camera/OAM ordering and crowd/basket presentation.

Exit: the rule/option ledger has no effect-free menu setting or unowned consumer;
full match journeys cover every event family. No unsupported child fallback,
host-invented replacement panel or captured static gameplay scoreboard remains.

### 4. Complete boot, assets and audio

Scheduler and root finish cold boot through legal/EA/title/menu/game handoffs,
including original waits and input. Complete indexed resource decoders and runtime
publication; account for every graphics/audio resource and remaining captured
hardware-state dependency without substituting recorded rendered frames.

Audio acceptance covers shared command/RNG ordering, initialization and SPC
acknowledgement, sequence/voice priority, gain/options, continuity and audible
comparison. Command counts or BRR samples alone are insufficient.
Current production still uses the legacy gameplay-audio event adapter and private
audio RNG. Audit the standalone event candidate and close the actual80:9DF3
return/allocator contract before shared-RNG cutover. HUD's shared07F6 consumer
does not establish that gameplay audio shares it correctly.

Exit: continuous cold-boot journeys and menu/game transitions pass source/state
and consecutive-frame/audio review from a fresh build and reproducible user-ROM
asset preparation. No dependency on an agent's private executable or hidden save
state remains in the runnable product. Historical evidence stays local and intact.

### 5. Complete all retail modes and persistence

Root owns implementation once integration load permits, reassigning one freed
implementer explicitly if useful; auditor checks the original route inventory.
Implement Season, Playoffs and Load Series with every observed original submenu,
schedule/bracket/progression/results/record/statistics operation and return route.
Inventory exact original behavior before adding features not yet established.

Implement original save/load transactions and validation, including selected
configuration, separately saved Custom values, mode progress and only those
team/controller fields the original actually persists, established by the SRAM
inventory. Session memory is not persistence. Use isolated save files during tests;
never overwrite the user's existing saves. Preserve source-confirmed corrupt-save
quirks instead of silently adding friendlier behavior.

Exit: create, play/progress, save, exit the process, restart, load and continue
each original persistent mode. Compare the relevant serialized state and visible
progress to native behavior; cover normal, absent, invalid and interrupted host
save handling without changing original validation semantics. No log-only mode
route or placeholder menu can pass.
Also complete a full original-supported Season and each supported playoff/series
format through terminal/progression screens. Source/controlled format cases
supplement, rather than replace, end-to-end completion and reload journeys.

### 6. Independent release acceptance and delivery

Root builds one coherent release candidate; auditor reviews that exact source,
executable and pack, including every previous acceptance caveat. Run the full
existing regression suite, native differential checks from matched initialization,
normal input journeys and a declared endurance matrix spanning all period lengths,
difficulties, representative teams and all supported control arrangements.
Exercise complete matches, overtime, substitutions, second matches and persistent
mode progression. Rare controlled branch tests supplement normal journeys.

Capture and inspect consecutive frames at transitions/events and review audio;
check crashes, hangs, state leaks and sustained runtime performance. Record actual
measurements, not nominal60Hz or unsupported FPS claims. Investigate all new
failures; no fixture replacement, cropping, unexplained tolerance or source-less
exception may turn a failure into a pass.

Exit: all required ledger rows satisfy their applicable acceptance columns; no
known port defect or required missing/unverified original behavior remains.
The auditor signs the exact candidate, root produces reproducible build/run
instructions and a rollback path, and all source/documentation is committed and
pushed. Main/desktop stay unchanged until the final candidate is reviewable and
the standing restriction on promotion is explicitly lifted. Promotion then
includes a fresh launch smoke check of the actual installed executable.

## Integration and progress routine

For each ticket: reproduce the first failure, establish the original contract,
implement and wire the fix, run focused plus affected combined checks, freeze
the candidate, obtain independent review, integrate with a fresh build, then
commit/push and update the ledger. The implementation is not complete at the
investigation or helper stage. Record rejected attempts without rewriting them.

Reserve one native emulator capture slot at a time, using a private portable
configuration/save home and unique outputs. Coordinate through root; never stop
unrelated NBA Live97 or other emulator processes. CPU-only analysis/builds can
continue concurrently in separate directories.

After each visible integrated checkpoint, assign a bounded screenshot refresh
to an available sub-agent, then inspect and publish the completed dated gallery
under primary `.analysis/progress-screenshots/`. Update `latest/index.html` only
after success, retaining earlier runs. Screenshots remain gitignored. This is
checkpoint-based work, not a claim that an unattended recurring monitor exists.

At each checkpoint report what works in the actual executable, what failed,
what remains, and the next ticket. If a blocker reaches Max, record the question,
recommendation, implemented decision and independent verification result.

## Planning review record

Controllers reviewed the gameplay ordering and human enable gate. Scheduler
reviewed timing dependencies and checked the game-level owner location against
current source after the Max response. The independent auditor checked required
feature coverage and requested full Season/playoff terminal journeys, real host
multiplayer input and source-defined persisted fields; all were incorporated.
This is approval of the plan's scope and ordering, not acceptance of unbuilt fixes.

The three supporting work-plan files were copied unchanged from the agents'
ignored planning artifacts. Their reviewed SHA256 identities are:

- Gameplay: `65b5530c6c4a9d458b682966aac0327eac7b2485be5b783954573341f96d9942`.
- Timing/audio: `5bcc9d553952511507a0c319ab660fdb6fb6ca94a186fe777b6d3ff9047a885f`.
- Independent coverage review: `cf8eff368249cda549a1c7a2d4a90828b6669b1f851edb10ed68e4be43bd3486`.
