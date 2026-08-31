# Independent completion-plan audit — 2026-08-31

Plan only. Inspected integration **f318478ef12586928c2e03b034a49ddf8bc508bd**, current production callers/source manifest, STATUS, the latest completion-continuation and HUD/period integration notes, and gameplay/configuration inventories. Historical inventories identify questions; current code and newer accepted evidence determine present status. No production edits or native captures were performed.

## Definition of done

“Finished NBA Live 95” means the original USA SNES game's complete reachable feature set works through the production application, with original rules, state, data and confirmed quirks preserved. It must include every original mode, human/CPU ownership, complete matches, postgame and persistence—not merely a CPU Exhibition demonstration. Any intentionally unsupported original feature means a partial release, not finished.

For each feature, require: **source contract → complete owned state/dependencies → production caller → original reachable input route → differential/observable test → independent review**. Track these as separate columns, not a single percentage. Controlled source cases supplement natural routes; they do not replace them. A subroutine that deliberately stops before a reachable child is unfinished at subsystem level.

Whole-game acceptance requires all matrix rows below green, all known current failures resolved by source-backed fixes or independently proved test corrections, and no reachable untranslated feature/child. A fresh release build must pass from real startup without debug scene entry, native after-state injection, manually preconfigured structs, or inherited build objects. Preserve confirmed original bugs with literal tests and comments; do not classify unexplained port failures as original behavior.

## Current facts that change the starting plan

- `nba_game.c:666–680` prints all four Mode routes but enters Team Select only for Exhibition. `nba_game.h:32–42` has no Season/Playoffs/Load scene states. Configuration remains process memory (`nba_session.h` explicitly disclaims disk persistence).
- Current Player Setup **does** represent pad0 left/neutral/right and its direction-table flag. Five native selection/allocator records now exist. However `nba_tipoff.c:8475,8630–8636` still forces CPU play and initializes effective selections `[1,1,1,1,1]`. Win32 has a single keyboard button mask. Ordinary human play and physical multiplayer are not complete. Do not repeat the old claim that no neutral UI or allocator exists.
- Current rule4 Traveling reaches the accepted controller publication helper (`nba_tipoff.c:186`), but normal human dispatch is gated. Rules3/6/9/10/12 still have no configuration reader found in production. Older “six missing rule readers” is stale; playable consumer coverage is still incomplete.
- The 40-file production manifest includes the new HUD and controller core. It excludes the accepted period-formation/draw-order and human action/audio continuation components. Those independent helper acceptances are useful prerequisites, not integrated feature credit.
- The bounded HUD repair is integrated: current scores/clock, expiration, pause refresh/return work. Statistics/ad/complex foul-clear children, full atomic clock-read/NMI/scanout behavior, native paused08DE and general new verifier mutation certification remain open.
- Latest CPU regression failure is **frame41876**, unreleased mode15 pass metadata with mode8/saved15/animation53. The older **49412** regulation-formation failure remains separate evidence. Period state aliases and the missing `86:AF66→B468` special-receiver producer require proof; the private alias experiment is not accepted.
- Current Rules reentry comparison still has **158 mismatches**, first native1176/C893 brightness. Its committed test migration is not a transition repair.
- STATUS's dated headline, 55.50% feature estimate, address coverage and older “full suite passes” are not present completion measures. The latest continuation/HUD/period notes supersede their stale implementation details.

## Release acceptance matrix

Every row needs an implementer and a different reviewer; “existing bounded evidence” never waives the route test.

| Required area | Present scope / remaining work | Testable exit gate |
|---|---|---|
| Retail feature inventory | Exhibition, Season, Playoffs, Load Series are confirmed; complete non-Exhibition screen/branch inventory absent | Enumerate original reachable menus/screens/actions, conditions, data owners, source PCs and persistence effects. Resolve each unknown below; assign every leaf to a matrix row. No unexplored mode is marked complete. |
| Boot/legal/EA/title/credits | Indexed assets and bounded renderers accepted; complete waits, skip/input/audio/hand-offs open | Cold startup, normal and all supported skip/return/attract routes reach Setup with matching owned state and consecutive visible/audio boundaries; repeat with fresh and existing saves. Inventory whether additional attract/credits routes exist before naming them implemented. |
| Setup/Rules/Options | Storage/presets/Custom/input and many bounded transitions accepted; repeated Rules entry fails | Real full-word input drives every page/value/preset/commit/cancel/return. Repeated edited and unchanged visits preserve correct values and resources with no stale cells; continuous frame/PPU checks close reentry. Save/restart retention follows original semantics. |
| Team/Player Setup/introductions | Canonical home/right vs visitor/left repaired; pad0 selection exists | All 29 currently modeled team IDs initialize/render, all841 pairings pass identity/resource checks, and native retail categories/rosters are verified. Real UI selection, neutral/CPU and supported multiplayer choices survive all intro, cancel and match boundaries. Names/scores/rosters/controllers never swap sides. |
| Controller/ordinary human play | Core allocation/transfer and numerous isolated action components accepted; startup remains neutral | Physical input → five native records → actor/context ownership → movement/turbo/pass/catch/shot/defense/inbound/FT and transfer all function from Player Setup. Human-left, human-right, versus, cooperative and neutral CPU arrangements supported by original hardware contract pass; release/hold/repeat, simultaneous buttons and disconnect/reconnect are tested. No forcedCPU override or pad0 reassignment shortcut. |
| CPU/ball/action graph | Substantial production gameplay; current pass failure and unobserved branches | Resolve41876 before accepting this trajectory. Audit live branches/dependencies and shared RNG/event/animation ownership; integrate missing receiver and aliases with controlled counterexamples plus natural routes. Sustained possessions, every shot/pass/catch/contact/dead-ball family recover without host safety nudges or silently skipped work. |
| Regulation/halftime/OT/final | Tables/expiry and standalone formation proof; runtime restart wrong | Integrate entry/formation/draw-order into canonical runtime owners, preserving carried state and source order. Complete uninterrupted regulation for every quarter setting; halftime reversal, tied regulation, repeated OT, horn with ball live, final and next match pass. Natural full-match state transitions plus controlled rare cases, not timer-seeded substitutes for the full route. |
| Rules/fouls/FT/injuries | Partial consumers; missing rules and caller/availability/stat propagation | Each of13 rules has an actual menu-to-consumer test (table below). Natural offensive/defensive foul, shooting/bonus/and-one/FT/rebound/inbound cycles, foul-out, injury and replacement paths complete with statistics/availability preserved. Inventory exact original edge conditions rather than impose modern NBA rules. |
| Pause/timeouts/substitution | Only timeout/resume named; native pause timing and three entries incomplete | Identify and implement all original entries, disabled states and requester routing. Manual/automatic substitutions, lineup/bench ordering, foul-out/injury handling and all returns work with human/CPU owners; correct clock/timer/stamina/counter effects and visible/audio transitions. Zero-timeouts and unavailable roster edges match original. |
| HUD/statistics/postgame | Score/clock bounded repair accepted; unknown overlays and structural final remain | All real score/stat/ad/foul-clear requests render original indexed content; no pending untranslated PCs. Complete original break/final summary, box scores, records and return/new-game paths; values agree with gameplay/season state and saves. Resolve clock-read/scanout ownership, not permanent-scoreboard substitution. |
| Season | Menu label only | Native-supported creation/configuration/team selection → schedule → played and any original simulated games → results/standings/statistics → season end and any native qualification/continuation complete. Test one full season and restart/resume mid-season; cover each native format and tie/qualification branch with additional controlled fixtures. Exact format/game counts come from inventory. |
| Playoffs/Load Series | Menu labels only | Start every supported original playoff/series format, play/simulate as supported, update bracket/results, advance rounds, eliminate/win and reach original terminal screens. Save, quit process, Load Series and finish; original invalid/empty-save responses work. Do not invent a modern bracket or assume Load Series loads Season. |
| SRAM/persistence | Session-only choices, no completed save/load | Map original SRAM fields/validation/slots, save triggers, custom rules and mode/stat data. Fresh-save, write/reload across process restart, overwrite/delete/reset and invalid/truncated data follow the source contract. Compare native saved payloads and host round trips; incompatible saves cannot silently reset successful progress. Separate transient match, session and persistent state. |
| Rendering/assets/audio | Indexed asset progress and standalone scheduling/SPC work; runtime gaps | All required retail resources are ROM-derived and reproducibly extracted, no runtime screenshots/native checkpoint playback. Implement source-defined publication/timing, shared CPU/APU ports/timers/events/RNG, voice priority/mixing and complete audio options. Continuous footage plus listened audio from startup through match/mode exits; bounded indexed/PCM/command checks catch cadence, seams, cutouts and incorrect triggers. |
| Release/verification | Many bounded tests; full current suite not green | Fresh all-source build, all maintained tests and meaningful negative verifier tests pass; current full journeys/endurance and independent visual/audio review pass. Package tested binary, correct pack and instructions; validate actual shortcut and clean-install save paths. Root owns reviewed commit/push/release and verifies remote/artifact identities. |

## Configuration gate: all24 exposed values

For each value: real UI changes working/committed state, actual gameplay/audio consumer reads the correct owner, an induced source-defined event demonstrates the effect, return/new-match/save behavior is checked. Boolean settings need both values; sliders all UI values plus consumer boundaries; enum values all branches. Use pairwise interaction coverage plus exhaustive known coupled branches rather than an untestable Cartesian-product promise.

| Values | Current state and required observable test |
|---|---|
| Mode, Style, Level, Quarter | Mode routing incomplete; presets/Custom now exist. Verify all four modes, Arcade/Simulation/retainedCustom effects, each difficulty's source branches, all regulation/OT clock tables through real matches. |
| Defensive Fouls, Offensive Fouls | Bounded readers: thresholds affect source-defined contact outcomes and downstream foul/stat/FT flows, including OFF/endpoints. |
| Out Of Bounds | Reader exists: ON dispatches appropriate restart, OFF suppresses the rule without freezing ball/actor flow. |
| Backcourt | No current reader found: complete possession/side/history gate and ensuing violation/restart. |
| Traveling | Bounded controller consumer exists but human route gated: grounded/moving owner and action gates produce/suppress violation correctly. |
| Goaltending | Existing contact/violation readers: valid/invalid contact and scoring/event outcome through gameplay. |
| 3 In The Key | No current reader found: source actor timer/commit/violation route, reset conditions and OFF behavior. |
| Foul Out | Existing bookkeeping/partial replacement: threshold, availability, full substitution and return. |
| Shot Clock | Existing clock/policy: ON expiration/reset notification/HUD and OFF event suppression, including ball-at-horn edges. |
| Inbound Clock, Half Court Clock | No current readers found: each distinct timer/gate/violation and OFF behavior; no conflation with shot clock. |
| Fatigue | Existing stamina consumer: enabled/disabled, all quarter lengths, timeout/bench/period grants and persistent roster effects. |
| Injuries | No current reader found: source contact, unavailable-player/stat/presentation/substitution flow and OFF behavior. |
| Music Volume, SFX Volume | Current Setup-only host gain approximations: complete original gain/priority semantics in frontend and gameplay, endpoints and retention. |
| Music Mode, Crowd Sound | Missing runtime readers: OFF/Mono/Stereo and crowd gates verified in actual output/commands, not glyph changes. |
| Slow Motion Dunks | Missing scheduler reader: original accumulator/cadence effect without breaking shared clocks/audio. |
| Shot Control, CPU Assistance | Bounded readers: actual human shot behavior and late-game source assistance gates, values and team ownership demonstrated. |

Do not add “Run Speed” as a fourteenth rule: the old original-label audit identifies it as unexposed text. Do not conflate Shot Control17BF with the distinct17C3 word.

## Manageable phase order and ownership

1. **Freeze a truthful release backlog now.** Root pinsf318478 and current failure witnesses. A modes/persistence implementation agent inventories original feature graph/SRAM immediately, in parallel with gameplay repair; this cannot be postponed until the supposed final week. Replace stale status numbers with this acceptance matrix.
2. **Close the next real Exhibition blockers.** Gameplay agent attributes41876, receiver producer/aliases, then adopts accepted period-entry/formation/draw-order with complete ownership and callers. Independent auditor reviews literal edge cases and natural integration. Root reruns the whole currently failing CPU test without weakening guards.
3. **Connect one complete human game, then original multiplayer.** Controls agent composes already accepted pieces and missing offense/defense/launch/cleanup into real production dispatch before lifting neutral gating. Extend host inputs and UI to the verified physical topology. First checkpoint is a full human Exhibition with ordinary inputs, not a collection of action probes.
4. **Close rules and full-match presentation.** Gameplay/rules agent implements missing consumers and complete foul/FT/substitution lifecycle. Presentation agent closes all pause/stat/final children and repeated frontend transitions. Audio/scheduler agent owns exact causal publication and CPU/APU state; collaborate on shared RNG/timers without duplicate ownership.
5. **Finish retail modes/persistence using the completed match service.** Modes agent implements the already inventoried Season/Playoffs/Load graph and durable records. Deliver vertical slices: create→one result→save→restart→load; then full season/series completion. Native source review precedes any invented data format or calendar/bracket logic.
6. **Integrated closure and release.** Root builds a fresh cross-feature executable and drives natural complete routes, every quarter preset, representative rule interactions, all team initializations and completed retail modes. Independent source/protocol reviewer plus separate visual/audio reviewer sign off. Fix first failures and rerun affected downstream gates; finish with clean-install/save/load and actual user-facing build validation.

Sub-agents implement bounded, separately reviewable source/caller contracts; root alone integrates shared files and manifest changes. Avoid spending successive checkpoints only expanding detached helper coverage while the same real match remains broken. Each phase must end with a stronger production route and a specific acceptance report.

Use the user-authorized **Max** consultant for concrete blockers after local reproduction/source inspection: provide exact revision, original PCs, minimal before-state, first differing outputs, attempted explanations and the bounded question. Ask for diagnosis or independent source derivation, not an unsupported approval. Root/implementer retain ownership, reproduce the proposed answer, and obtain independent review. Do not let escalation create a separate untracked implementation or replace source evidence.

## Facts to inventory, not invent

- Exact Season lengths/formats, team-selection limits, schedule/simulation/standings/tiebreak rules, qualification/end screens and whether original roster management/trades/editing exist. Only include such optional features as confirmed by the original graph/manual/resources.
- Exact Playoffs/series formats, round transitions, save/load scope and whether Load Series applies to other modes. Native mode labels alone do not establish any of these.
- SRAM size/layout/checksums/validity/slot counts, initialization/reset behavior, save triggers and what survives a new game or power cycle.
- The three unnamed pause entries, all disabled/cancel/return branches, original statistics/record screens and season-specific postgame children.
- Physical controller topology and simultaneous supported players. Five software selection records are proven; they alone do not prove five playable users or the host mapping. Verify device loss/neutral and requesting-pad pause behavior.
- Full unobserved human actions, rare CPU branches, fractional-state domains, runtime alias ownership and shared IRQ/NMI/APU ordering. Existing controlled preconditions must be proven at every production caller or expanded, not silently assumed.

## Reviewer responsibilities and hard stops

**Implementer:** frozen source/build closure, explicit state/API/preconditions, source references and real caller wiring; retain first failures and identify natural versus controlled evidence. **Source/state auditor:** original widths/wrapped arithmetic/aliases/quirks and complete children, independently reproduce counterexamples. **Evidence reviewer:** input route/isolation, all outputs consumed, exact source/pack identity, malformed/omitted/reordered responses rejected; no expected-afterstate driving C. **Visual/audio reviewer:** actual continuous production footage/listened audio, changing data, repeat routes and transitions—not only anchor screenshots. **Root:** cross-component ownership, fresh builds, mode coverage, full-suite triage and release claims.

Stop whole-game approval for any of: menu option only logs/stores; a setting has no proven effect; normal controller choice is overridden; reachable pending/untranslated child; unproved helper precondition; stale build or after-state seeding; fitted delays hiding causal divergence; first-failure oracle discarded; refreshed C hashes without controlled attribution; a whole-match claim based on direct scenes/forced clocks; session memory presented as persistence; static art or recorded command schedules replacing required state; unresolved full-suite failure; unreviewed original-mode branch; missing ordinary save/reload; or “original bug” asserted without source proof.

Primary current references: `docs/completion-continuation.md`, `docs/gameplay-hud-integration.md`, `docs/period-tipoff-state-mapping.md`, `docs/completion-controller-independent-audit.md`, `docs/completion-config-inventory-20260830.md`, `docs/gameplay-ownership-audit.md`, `docs/feature-capture-matrix.md`, `docs/ownership-plan.md`, `STATUS.md`, `nba95_sources.txt`, and the current callers cited above. This plan does not endorse archived dirty worktree snapshots or inherit their PASS claims.

## Review of root's proposed plan

Read `completion-owner/docs/completion-plan-20260831.md`. Its definition of done, separate acceptance columns, milestone0 retail inventory, blocker escalation and preservation of current failures cover the required scope. Three concrete refinements were sent to root:

1. Milestone5's exit must include a complete Season and complete playoff/series through their native terminal/progression screens, not merely create/save/load/continue. Additional formats/rare branches need declared coverage.
2. Milestone2 must explicitly include host physical input/multiplayer support; current Win32's one keyboard mask is not supplied by the five-record allocator.
3. Persistence must retain the fields the original actually saves, as established by SRAM inventory; listing all team/controller session fields as necessarily persistent would invent a contract.

Current-code corrections supplied to root: pad0 neutral/direction selection and the controller allocator now exist; Traveling now has a bounded reader but no normal enabled human journey; the remaining absent direct rule readers found are3/6/9/10/12. No further top-level required-scope omission was found in the draft. This is planning approval, not implementation or release acceptance.
