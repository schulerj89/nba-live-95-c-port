# Gameplay, controller ownership, and lifecycle audit

Audited 2026-08-30 against repository HEAD `2723af6` before this workstream's
changes. The initial tracked working tree was clean. No `AGENTS.md` was found
in this repository or its ancestor directories. This is a code/evidence audit,
not a whole-game parity certificate.

## Reference and implementation map

| Artifact | Verified local location | Use and limitation |
| --- | --- | --- |
| Portable game | `src/nba_tipoff.c`, `include/nba_tipoff.h` | Production actor/ball/action/scheduler/lifecycle adapter; approximately 9,700 lines, including embedded C self-tests |
| CPU, ball, rules helpers | `src/nba_gameplay_ai.c`, `nba_gameplay_ball.c`, `nba_gameplay_foul.c`, `nba_gameplay_free_throw.c`, `nba_shot_*.c`, `nba_tipoff_flow.c` | Typed native slices; exact helper output does not prove complete caller order |
| Session and navigation | `src/nba_session.c`, `src/nba_game.c`, `src/nba_player_setup.c` | Stored configuration, scene routing and human side selection |
| Original ROM | `F:/Games/SNES/NBA Live 95 (USA).sfc` | Canonical SHA-256 `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`; authoritative behavior |
| Sibling recomp | `../NBA-Live-95-Recomp` | Exists, but is not a Git repository; `generated/` contains bank00/80/81/82, not a complete generated gameplay graph |
| Focused gameplay recomp | `.analysis/shot-state-recomp-20260827/generated`, `.analysis/tipoff-flow-recomp-20260827/generated`, `.analysis/human-free-throw-recomp-20260829` and other slice directories | Readable native translations; several generated graphs retain unresolved/LLE calls. Validate the exact slice rather than treating the sibling runner as a fully independent C implementation |
| Ghidra | `ghidra-projects/`; `.analysis/cpu_gameplay_ghidra/*_listing.txt`; `.analysis/differential-ghidra-20260828/shot_state_bank86.txt`; `.analysis/gameplay100-closure-ghidra/gameplay100_bank84_listing.txt` | Control-flow/address evidence; some older files include executed instructions only and omit unobserved branches |
| Native scalar evidence | `tests/fixtures/`, full ignored `.analysis/*native*/*.jsonl` and `*.meta.json` | Retained input/output witnesses; distinguish natural journeys from controlled state seeds |
| Strict trajectory harness | `tools/{run_differential.py,mesen_differential_capture.lua,differential_runtime_probe.c,differential_fields.def,differential_compare.py}` | Baseline plus actual actor-sweep boundaries; baseline currently fails |
| Production assets | `build/nba95_assets.pak`, extraction in `tools/`, native asset IDs in `include/nba_assets.h` | ROM-derived indexed graphics, SPC/BRR state; full `.analysis` screenshots are evidence, not runtime art |
| Integration tests | `tools/test_cpu_gameplay.py`, `gameplay100_closure_probe.c`, `gameplay85_endurance_probe.c`, `tip_flow_endurance_probe.c`, lifecycle/timeout probes | C regression coverage; matching repeat runs or image hashes do not establish ROM equivalence |

## What is actually playable

One Exhibition CPU-versus-CPU path runs through the frontend, tip, live play,
dead-ball handling, periods and a structural final screen. Ordinary human play
is absent despite the Player Setup screen assigning Player 1 to a side. CPU
gameplay is substantial but not synchronized with a matching native start.

The initial-state report in `.analysis/differential-release-final-cpu/report.json`
has 62 differences in 449 projected 16-bit fields and zero matching
checkpoints. Its paired `run.json` records the original ROM hash, executable,
pack, scripts and capture hashes. Native first sweep is relative frame 25;
the C sweep is frame 2. Native clock 43200 versus C 10800 is an unsynchronized
configuration, not evidence that the correctly translated clock table is wrong.
The default human run adds an actor-controller mismatch. None of these reports
can support post-baseline trajectory parity.

## Remaining-work inventory

`implemented` means code exists; `wired` means normal production calls it;
`ROM` below means a bounded retained native comparison, never full subsystem
equivalence; `regression` means C checks; `visual` means an inspected image or
component oracle, never automatically consecutive-frame equivalence.

| Subsystem / screen | Implemented and wired | ROM / regression / visual evidence | Approximation or missing acceptance work |
| --- | --- | --- | --- |
| Boot/legal/EA/title | Frontend production scenes | Strong retained component/scene evidence and C hashes | Transition agent must inspect every consecutive boundary frame, skip path and repeat path; no gameplay audit PASS is implied |
| Setup/Rules/Options | UI values, commit/re-entry and Exhibition route | Menu fixtures and C flow tests | Runtime consumer mapping is incomplete; Season/Playoffs/Load Series log the selected route without entering it |
| Team/Player Setup | Both team selectors, Player 1 left/right icon stored in session | Native menu witnesses and C rendering | No neutral/CPU slot, no five-controller selection records, no complete return/cancel/controller-loss flow |
| Introduction/lineups/tip | Native assets, lineup cards and physical tip helpers | Native bounded toss/contact/catch helpers, C journey | Whole launch context and start latency differ; entry raster/brightness timing still requires transition proof |
| Human movement/ownership | Pad publication and isolated inbound direction helper | Sixteen direction nibble witnesses; CPU-only regressions do not cover human play | Init forces all actors to CPU. Movement/acceleration/stopping, turbo, possession-aware selection, manual switch, loss/disconnect and pad routing are missing as a playable pipeline |
| Human offense | Some complete shot/pass helpers usable by CPU | Native arithmetic/animation slices | Real button dispatch, pass targeting, receiving control transfer, normal shot press/hold/release, close/special shot selection and off-ball actions missing |
| Human defense | Contact/knockdown helpers contain conditional human branches | Bounded contact vectors | Production steal, block, defensive stance, switching, assignments and human-vs-CPU journey missing |
| Human free throws | Two-axis oscillator and scene adapter | Seven retained state/aim witnesses; refreshed replay passes | Dormant: +$16 and context +$3B never set. Natural foul-to-stripe input journey, cursor assets/common-launch ordering, alternate pads and controller loss unproved |
| CPU movement and actor lifecycle | Ten-actor commit, derived velocity/facing, reaction timers, animations and common bounds | Many independent helper witnesses; multi-team C endurance | Native scheduling and common-prefix order unaligned. Special modes retain extra compatibility clamp when common commit is bypassed |
| CPU offense/defense | Play stream, assignment planner, pass/shot/layup/rebound/action selectors | Focused ROM slices plus long C trajectories | Full graph and decision/RNG order unknown; unobserved branches and natural frequencies unproved |
| Possession/dribble/pass/catch | Signed owner dispatch, resource-based attachment, pass animation, receiver acquisition | Independent owned-ball/pass/attachment/catch fixtures; C binding probes | Ball attach wipes all controllers, launch/shot wipes them again. Event/interrupt timing and all owned rim cases unproved |
| Shot/ball/rim/score | Launch arithmetic, wind-up, close finishes, flight, rim/backboard/net results, made baskets | Native helper and 31-field owned projections; C score/endurance guards | One owned event case is explicitly partial; unresolved whole-start state makes whole shot trajectory/audio comparison invalid |
| Fouls/violations/inbounds | Contact classification/consumer, OOB split, shot clock, inbound targets/arrival/transfer/cancellation | Native controlled/helper witnesses; C dead-ball/endurance | Bonus and shooter/bench/stat orchestration, rare violation callers and natural foul/FT cycles incomplete. Successful isolated inbound proof is not full per-dispatch proof |
| Fatigue/statistics | Active roster stamina, shot counters, fouls and some momentum/assistance writers | Native bounded writers; C actor/roster binding | Complete box scores, records, season/stat persistence and all consumers not implemented |
| Regulation/halftime/OT/horn | Clock tables, expiry latch/high-ball gate, period advancement, stamina grants, anchor reversal | Four controlled native expiry outcomes; C short/long lifecycle | Frozen prior-court presentation during measured waits. Native statistical/score scene raster and audio children not ported; natural complete-match parity absent |
| Timeout/pause | Timeout and Resume only; freeze, count decrement, stamina grant, saved live-state restore | Bounded state witnesses; C freeze tests | Native entries 1..3 missing; hand-drawn pause panel, fixed transition wait and correct disabled-item/menu-return behavior not fully verified |
| Substitution/foul-out | Atomic incremental replacement, roster/resource rebuild and pending-request gate | Disassembly and deterministic transaction/binding tests | Full native parent vector acquisition incomplete; `$966D` full reorder, unknown `$4726/$47A6` selector semantics, human lineup UI and parallel request `$09CC` missing |
| Final/replay/new match | Structural final panel and return to Setup; follow-up resets a newly confirmed Exhibition match | Native first-court state projection; two C production return journeys; native final entry witnessed | Host-created panel, native postgame records and exit unproved. The audited second-match FINAL-state freeze is fixed by the bounded follow-up; no natural native second-match/scene parity claim |
| Season/playoffs/save/load | Menu labels only for retail mode routes | No equivalent trajectories | Schedules, brackets, results, records, save/load and disk persistence missing. Existing config is process/session memory only |
| Court/camera/players | Indexed ROM court, ten sprites, appearance and camera helpers, Mode-1 composition | Native camera/PPU/component oracles; C image anchors | Baseline camera differs. Crowd cadence, high jumps/window/clipping, rare OAM priority, HUD/actor overlap and full moving-frame parity unresolved |
| Gameplay audio | ROM BRR/SPC and command/mixer paths | Sixty native command/RNG/source/pitch/volume cases, PCM regressions | Shared event/RNG/interrupt order, voice priority and long continuity unproved. Option effect must be checked through actual commands/samples |

## Exact human ownership gap

`nba_player_setup_update` changes `session.player_one_side`; only pause-side
selection consumes that field during a match. `nba_tipoff_init` unconditionally
sets `cpu_vs_cpu=true`, actor `controller_assignment_raw=-1`, team-context
controller fields to -1, and leaves `mode11_context_raw_3b` zero. The only
ordinary pad retained by Tipoff is pad 0's held mask. The current normal actor
dispatcher is a CPU behavior dispatcher; it is not the Bank84 human input
dispatcher.

Direct inspection of `$86:E285-$E386` shows five controller selections at
`$166D+pad*2`, previous selections at `$1677+pad*2`, and controller records
advancing by `$40`. Value 1 is neutral; value below 1 maps team group 5 and
above 1 maps group 0. `$4726/$47A6` count assignments; actor `+$16` receives
the pad index. Reused selections and the `$07F8` gate have separate branches.
The C two-valued side flag cannot represent this contract. `$84:E2AC-$E432`
and its human action children need a complete caller/dependency census.

Merely changing `cpu_vs_cpu` to false is unsafe: `ball_attach_to_actor` clears
every assignment and makes whichever actor owns the ball pad 0, including the
opposing team. `ball_launch` and shot-start code clear assignments again.
No partial human mode was enabled by this audit.

## Tests challenged by direct inspection

1. `gameplay100_closure_probe` runs C twice, compares digests and a hardcoded
   image/state hash. Its scene helpers are invoked directly and do not execute
   `nba_game_enter_state` or the production scene dispatcher. It changes menu
   values, but digest inclusion proves storage only, not intended runtime
   effect. It cannot catch the second-match navigation/reset bug.
2. `match_lifecycle_expiry_probe` checks only literal case names and PC strings
   using `strstr`; it never parses fixture input/result values. Its four
   outcomes are hardcoded C constants. Changing native fixture results would
   not change the comparison. Keep it as a C regression until a real fixture
   parser/driver makes native outputs authoritative.
3. `timeout_resume_runtime_probe` seeds structs and advances 60 calls. This
   protects freeze/restoration, but cannot prove the native menu selection,
   transition-resource ordering or fixed timing. The three unnamed retail menu
   items between the retained endpoints are not exercised at all.
4. Current `test_cpu_gameplay` already checks arrival against the exact raw
   `[-9,+8]` box. Do not reintroduce the older broad compensation envelope.
   End-of-frame seed/selector/timer allowances still miss internal caller state.
   Add pre-call/post-call witnesses for `$F43A`, velocity/steering `$F45F`,
   restored raw target `$F4E6`, arrival `$F4F2`, and ready/transfer continuations.
5. Substitution docs admit zero vectors from attempted complete native parent
   capture. Typed incremental C transaction tests must not be promoted into
   proof of the complete `$83:ECB0-$ED46` caller/graphics return.
6. Human free-throw evidence uses real Mesen input **after controlled state
   injection**. It is valuable and labeled in the script/docs; it is not a
   natural foul-to-stripe journey or proof of ownership propagation.
7. The strict differential is appropriately failing. Matching its 449-word
   projection would still exclude scores, full team/roster/controller/camera
   state, rendering inputs/output and APU state. No relaxed tolerances or
   fitted frame offsets should replace the failure.

Historical 100% was captured address-position documentation, including broad
whole-bank credits later withdrawn. The corrected 11,529/28,643 (40.25%) counts
positions inside evidence-eligible boundaries; 11,526/60,346 (19.10%) counts
decoded instruction starts. The latter denominator is a conservative code
lower bound, not all ROM bytes. The feature matrix's 55.50% is a weighted
planning estimate, excluding no evidence debt automatically. None measures
complete game behavior or playable human coverage.

## Priority and acceptance dependencies

1. Complete consecutive-frame frontend transition checkpoints first. In
   parallel, repair the bounded fresh-match reset without changing period
   restart behavior or session settings.
2. Align the native/C configured launch, controller records, scheduler and RNG.
   Add per-routine boundary telemetry before interpreting later differences.
3. Implement full single-human ownership/action flow from Player Setup through
   offense, defense, inbound, pass/receive and shooting, preserving CPU-only
   selection. Test left/right/neutral and repeated games through actual menus.
4. Consume every rule/option through real play and complete natural foul/FT,
   timeout/manual substitution, period/OT/final presentation and audio flows.
5. Complete non-Exhibition modes and persistence; separate disk/save semantics
   from configuration retained only while the process runs.
6. Require an independent native/recomp/code/evidence auditor, exact bounded
   comparisons, C regressions, endurance and viewed consecutive footage at
   each release checkpoint. No current whole-game completion claim is valid.

## Audit-time reruns

Existing August 29 probe binaries were rerun on August 30: human free-throw
retained fixtures 7/7 and inbound side-gate controlled fixtures 40/40 matched;
the differential harness's 12 synthetic tests passed. These establish only
those binaries' retained-fixture/unit behavior. They are not freshly captured
ROM results or a proof that rebuilt current sources pass. New source builds
and the session-reset regression are recorded in [new-match-reset.md](new-match-reset.md).
That follow-up fixes the second-match FINAL-state freeze through the actual
frontend caller and passed its independent startup projection plus two C
return journeys. It does not close the wider lifecycle/presentation gaps.
