# Project status

Last updated 2026-08-29. This is a current-state handoff, not a milestone log.
Use Git history for older checkpoints and run
`python tools/progress.py --write docs/progress.md` for live measurements.

## Current state

The playable path runs from the Nintendo license through Player Setup,
starting-lineup cards, tip-off, and early CPU-vs-CPU gameplay. The port has
court/camera movement, ten-player composition, possession, passing, shooting,
bounces, and initial foul scaffolding. User-controlled gameplay is not in
scope yet.

Current measured coverage:

| metric | captured address positions | % of captured addresses |
|---|---:|---:|
| observed executed code | 27,901 | 100.0% |
| documented by ROM-address provenance | 27,901 | 100.0% |
| verified against ROM/recomp/Ghidra ground truth | 27,901 | 100.0% |

These values are generated, not estimated. The detailed per-bank report is
`docs/progress.md`; the authoritative verified list and evidence paths are in
`docs/verified-routines.json`.
The exec interval files mix instruction-start captures and older gap-coalesced
captures. This is an address-coverage metric, **not an instruction census or
percentage of the whole game completed**. Disassembled instruction counts
for a requested slice are reported separately.

Court presentation assets now use pack v31. Asset 282 stores the native BG1
basket map/4bpp CHR/CGRAM, asset 283 stores the 28 animated BG2 fan tiles
seen at `$5280-$55FF`, and asset 284 stores per-team indexed gameplay
VRAM/CGRAM inputs. The renderer applies `$087C/$087E` scroll,
`$0882..$0880` clipping, the projected `$3FEB` basket selection and raw OBJ
resources `$0822/$082C-$082F`. Center court, team paint, sidelines and stands
now render from asset 284's indexed state; asset 273 remains an offline
panorama oracle. See `docs/court-assets-audit.md` and
`tools/test_court_assets.py`; no screenshot pixels are runtime art.

Gameplay court composition now runs through the reusable Mode-1 library in
`nba_snes_ppu`: the complete BGMODE priority ladder (including high-priority
BG3), BG tile priority, four OBJ priorities, lower-OAM opaque-pixel selection,
single-window inversion and indexed CGRAM conversion are covered by a startup
self-test. The lowest-index opaque OBJ pixel is selected before its priority
is compared with BG, preserving the hardware OBJ-priority quirk.
`--ppu-trace FILE` writes one summary plus 256x224 JSONL pixel
records containing the winning source, priority rank, palette/color indexes,
OAM index and ARGB result. Predecoded panorama/player pixels are explicitly
marked palette/color 255, so the trace exposes rather than hides remaining
direct-color inputs. `tools/test_snes_mode1.py` locks the CLI contract.

## Verified gameplay checkpoint

Latest bounded work: `docs/gameplay-100-percent-plan.md`. A fresh headless
Ghidra closure classified the final 4,064 retained positions across Banks
`$00/$80-$84`; recomp cross-checks were used for Banks `$00/$80-$82` and the
indirect/table-owned Banks `$83/$84` remain Ghidra/native-grounded. The exact
closure gate reports 27,901 executed, 27,901 verified and zero pending.
`tools/progress.py` independently reports all 27,901 positions documented.

The permanent `gameplay100_closure_probe` traverses Rules and Options with
real commits and all four page transitions, Team Select, Player Setup,
matchup/ratings, ten lineup cards, physical tip possession and 6,000 CPU
frames. Two identical runs are pinned to digest `39974258c482a822` with eight
handoffs, 65 render changes, 2,910 actor-motion frames, 12,985 animation-
resource changes and 64 possession changes. The complete release suite also
retains the 48,000-frame three-matchup state/render digest and the 63,800-frame
dead-ball endurance test.

There are 227 verified ledger entries. The 100% figure is exact coverage of
the retained execution captures, not a claim about unobserved ROM branches,
unsupported modes, user-controlled gameplay or whole-game feature completion.
The most important bounded slices and caveats remain below:

Older "remaining" counts in those slice notes describe that slice's
then-current disassembly census or unobserved branches. They are preserved as
technical caveats and are not part of the current zero-pending captured-address
ledger.

- Ball initialization prefix: `docs/pending-gameplay-differential-plan.md`.
  Corrected native start E056 (E054 was a JMP operand),30 instructions.
  Natural and poisoned-input same-entry tests match all20 mapped words and
  all128KiB exit RAM. Runtime initialization now uses native VZ600, object
  bookkeeping and sentinel writes. Default whole-game baseline differences
  fell82->81; this is not a passing trajectory comparison. The later toss and
  jump/reach path is now native-state driven. Bounded core remaining:332 plus
  optional19.

- Tip-flow checkpoints: `docs/tipoff-flow-plan.md`. Contact geometry now drives
  the first hit instead of frame200. 340 native/controlled calls replay with
  zero mismatches. Receiver B04C now matches all59 starts across five native
  calls, with both-team/both-bit runtime guards. Launch now uses99C4 and
  shared physics:126 native calls, all306 starts, zero mismatches. The complete
  temporary/final possession caller now follows actual contact, not frame220.
  Completion164 calls, catch core22 and wrapper17 all replay without represented-
  output mismatches. The wrapper proof fixes0942's ball-source identity10 and
  restores its previously excluded ledger range. Initial toss/jump now uses
  the ROM countdown, EC32 launch and CF38 receiver reach; see
  `docs/gameplay-pending.md` for the current bounded CPU/gameplay census
  (332 remaining after the correction above),
  19 optional human instructions, and uncounted areas.

- Camera correction/callers: the remaining **99 instruction starts** now
  pass native replay: 62,243 calls across core/init/resolver/copy/cadence,
  zero mismatches, 1,133 durable witnesses. Initialization, edge fractions,
  basket orientation, no-team centering, alternate ball-height flags and
  before-wait latch/after-wait copy are integrated. Native tip state is
  `$0936=0081`, then `0000` on acquisition, not the old persistent `0001`.
  Correcting that connection reduces early post-tip vertical error. The
  initial toss/jump producer is now separately verified and wired; full
  configured-start trajectory equivalence is still not claimed.
  See `docs/camera-handoff-plan.md` for numeric/visual limits and reproduction.

- Camera/presentation: all **510 requested instruction starts** were audited
  in fresh Ghidra/native captures; 3,000 calls replay with zero mismatches.
  The wrapper's 72 state-writing instructions and the 220-instruction court
  streamer are translated; seven existing camera slices (212) are reverified.
  Six wrapper call/return sites are not credit for their external callees.
  Asset 273 now contains the full ROM-header **148x52** court (1184x416),
  fixing the old 34-column truncation and premature right-side scroll clamp.
  Asset 279 supplies the raw map; all runtime art remains asset-pack data.
  There are 480 durable call witnesses, 12 native viewport map/scroll checks,
  16,000 runtime binding frames and 812 rendered viewports across 29 teams.
  The old full-camera ledger claim was narrowed to exact slices; the later
  60 core corrections and 39 setup/caller instructions have separate proofs
  above. Animated crowd graphics and downstream BG1/backboard composition
  remain pending. See `docs/camera-presentation-plan.md`.

- The **145-instruction owner table** is implemented/re-verified: unlatched
  reversal `$86:E545-$E592` (31), idle cadence `$87:AD86-$ADBD` (22), and
  ordinary owner caller `$86:F34F-$F439` (92). All 145 instruction starts
  match Ghidra and appear in 2,095 zero-mismatch ROM call replays. The older
  two-word unlatched claim is replaced by full channel/resource proof.
  The caller stops held-ball velocity before posing, uses base pairing +74,
  returns immediately after losing ownership, and orders CPU/formation/pass
  calls with decision delta C8=32 (not physical C6=2).
  Three additional native contact calls confirm coarse F02D facing; using
  the fine pass quantizer had produced facing13 and stale attachment errors.
  There are 294 durable witnesses and runtime bindings/endurance guards.
  Idle cadence is integrated, but its **upstream defensive pose selector**
  E39A/E3E1 is not: the unforced C runs do not naturally select state7 yet.
  No full-game trajectory/frequency parity is claimed.
  Evidence, precise remaining boundaries and visuals: `docs/owner-flow-plan.md`.

- Held-ball states 13/18 and the latched owner pose branch are complete:
  `$87:AE89-$AEBC` (20 instructions), `$87:ADBE-$AE88` (78), and
  `$86:E4F5-$E544` (31). All **129 instruction starts** were observed in
  independent ROM calls and replayed; 12,049 calls match, with 326 durable
  witnesses. Actor `+$B0` now persists and appears in JSONL telemetry.
  State 13 resolves the lower resource before changing its shared phase;
  the selector writes desired facing `+$4E`, not eased display `+$52`.
  These are asset-pack animations, with controlled-ROM cadence coverage
  and natural latched-selector calls, not full-game frequency parity.
  Unforced C runs reach both poses; special-shot reachability/release is
  now guarded by two 200,000-frame matchups. With the event-driven tip flow,
  Chicago/Orlando selects/releases a special at76,883/76,910; the reverse
  matchup reaches no special in200,000 frames. Both reach states13/18 but
  neither naturally selects state7. This supersedes earlier trajectories.
  Detailed evidence, remaining boundaries and visual proof:
  `docs/owner-pose-animation-plan.md`.

- Shot-state writers now update the live CPU game instead of leaving stamina,
  made-run modifiers and assistance at defaults. The pre-code census was
  **239 decoded instructions**; all listed helper bodies and the pre-actor
  call binding are now translated/verified. 342 writer calls (176 natural,
  166 controlled) plus 47 additional natural selector calls replay exactly.
  `$09C0` is late-game trailing-team **CPU Assistance**, not a hot-streak flag.
  Actor `+$B2` is the made-run counter; opposing `+$B2/+$B4` clear on a make.
  Asset 278 and roster 251 supply stamina tables/ratings for all 24 slots.
  Its historical unforced 63,800-frame C run selected mode 17 at 50,338 and
  released at 50,366; the later held-pose RNG integration supersedes that
  trajectory (see above). This is not ROM frequency
  parity. Timeout/period grant helpers are replayed but their complete caller
  flows remain unimplemented. Evidence and exact boundaries:
  `docs/shot-state-plan.md`.

- Special selector `$86:B629-$B6D2`, mode-17 executor `$B979-$BAA1`, and
  complete launch `$86:9D6E/$9DA6` through `$A476` are integrated. 181 new
  ROM calls match exactly (42 natural, 139 controlled); both basket sides
  have controlled runtime release tests. Asset 277 provides literal shot
  tables. Full regression passes, including 63,800 CPU frames (38–39).
  Detailed evidence, code-only ledger boundaries, screenshots/video and
  caveats: `docs/shot-completion-plan.md`. The natural run did not select
  mode 17; controlled proof is not a natural-frequency claim.

- `$87:AEC3-$AF74` and `$87:AFA2-$B053`: immediate non-advancing pose
  resolution and ten-player appearance initialization. Live-pass installs
  now resolve their resources immediately, without a temporary tick-based
  fallback. Appearance uses asset-pack roster records. This does not adopt
  all shot/contact callers or emulate the SNES sprite-upload queue.
- Live ordinary-pose fallback now preserves `$87:AFA2-$B053` actor `+$A8`
  tall-lower selection and `$87:AC76-$AC95/$AD38-$AD57` actor `+$6C`
  direction-sensitive `base+$28` upper resources. The asset extractor now
  includes that complete derived upper family. Before the fix, 265 of 14,000
  actor-frames in the focused 1,400-frame audit selected an upper resource
  absent from the pack; afterward zero layers were absent, and the 63,800
  CPU trace crossed the new lower/upper/head/number guards. JSONL exposes
  per-layer resources/pixel counts/validity. Wider six-range census is
  1,779 starts, 1,485 observed and 294 not observed; only the B649/B832
  subranges retain strict instruction-level replay attribution. See
  `docs/gameplay-player-appearance-differential.md`.
- `$87:B37C-$B571`: independent action install/cancel/reverse helpers (471
  live calls). Locked completion/queued continuation slices of `$87:ABC2-$AD18`
  and mode-2 idle state 7 also pass a 6,000-call post-locomotion replay.
  Ordinary live-play mode-15 passes integrate exact locks, release phases
  and hand resources; inbound passes retain their compatibility path.
  State7 cadence now has a runtime binding; its defensive selection caller
  remains separate. Other action callers are not all adopted gameplay paths.
- `$87:8F13-$8F5E`, `$87:B572-$B648`, and owned common slices of
  `$87:AAB2-$AD5A`: facing easing, phase-preserving locomotion, exact
  accumulator cadence, and independent upper/lower resources. The replay
  compares resource IDs as well as phases; roster `+$08`/actor `+$6C` selects
  the low upper-body resource variant.
- `$85:AF5C-$B127` and `$85:B24C-$B353`: cached post-physics focal point,
  offense anchors, and the five-actor play barrier's live DP `$AA` counter.
- `$87:B649/$B66A/$B832/$B953`: exact actor-relative ball X/Y/Z from the
  asset pack. `$80:ADEB/$AE1E` and `$87:A6A9-$A6B4` prove contact height
  uses the composed head anchor, not the visible top of the head tiles.
- `$87:A9E3-$AA01/$AA02-$AAB1`: effect initialization and frame dispatch;
  active IDs 3/4 plus inactive/net paths are replayed, not every effect ID.

- `$86:D652-$D720`, `$86:C88F-$C91D`, `$86:CBC4-$CCCC`,
  `$86:BD41-$BF08`, `$86:C239-$C475`: the native sorted player-contact
  sweep, pair broadphase, teammate response, and ordinary opponent response
  across 293 live player-only sweeps with zero mismatches. The knockdown-only
  prefix remains outside the verified ranges.
- `$85:A4F2-$A517`, `$85:A532-$A597`: attached-ball vertical state,
  gravity, split fixed-point integration, and the distinct state-3/state-4
  impact responses across 292 live calls.
- `$85:C37D-$C5C0` plus its two projection helpers, and
  `$86:E7DC-$E7FC/$E8F7-$E922`: inbound and close-defense target formation.
- `$85:9A6A-$A4F1`, `$85:A598-$A7C7`: ownerless pass/shot/bounce
  physics, rim and score responses, gravity, fixed-point integration, ground
  restitution, settle, court clamp, and the two-substep scheduler tail across
  252 live ownerless calls.
- `$85:BC07-$C0F5`: complete defense-role cadence, rebuild, pairing cleanup,
  primary/help selection, and final context-ordered assignment pass across 26
  live calls and both observed exits.

- `$86:E4A7-$E592`: mode-11 owner/dribble gates, proximity selection, facing,
  and unlatched pose 9/11 selection.
- `$86:BAA2-$BB14`: player catch state and CPU-owner mode installation.
- `$85:F347-$F3BA`: target distance/direction calculation.
- `$85:A82C-$AB16`: native actor velocity damping, acceleration, boost, and
  cap behavior across 2,000 captured calls.
- `$85:B402-$B4B8`: predictive arrival, steering, and the coupled velocity
  application across 1,000 captured calls.
- `$85:B4B9-$B5FE`: cutter cadence and pass-receiver priority/order across
  1,187 captured calls and both stable selector exits.
- `$85:B60B-$B677`: CPU pass-receiver candidate rejection and acceptance.
- `$85:B734-$B820`: CPU mode-11 shot policy and its ordered RNG consumption.
- `$85:F5E4-$F727`: opponent lane obstruction used by the cutter and mode-11
  shot branches, including its half-open rectangle edges.
- `$85:9192-$93F4`: camera targeting/acceleration, first-placement history,
  fractional edges, orientation, no-team centering and alternate-height gate.
  Proof is the union of the older 212-instruction slices and the newer 60;
  setup/caller instructions have separate entries. No whole-game parity claim.
- `$87:A3BB-$A43B`: final player origin projection now uses the ROM's integer
  actor words, signed floor-by-four transform, camera subtraction and exact
  CPU/controlled visibility rectangles. This removes the opening-frame
  `123` versus native `122` Y divergence without non-native interpolation.
  The bounded `$87:A357-$A479` census is 120 starts, 92 observed. Of the 28
  unobserved starts, five are the statically translated high-jump culling
  branch; the other 23 are separate queue/effect/ball presentation setup.
  See `docs/gameplay-sprite-jitter.md`.
- `$87:A47A` player presentation cadence: consecutive Mesen frames prove that
  submitted OAM origins persist between alternating draw passes. The port now
  latches those origins on its due actor pass instead of reprojecting them on
  the intervening camera-only render. Frames 180..620 improve from 51 small
  A -> B -> A reversals to zero, matching the native trace's zero.
- `$85:A692-$A755`, `$85:B971-$B9D1`, `$85:F3C3-$F472`: court Y/clamp tail,
  reaction threshold/RNG, and fine pass direction.
- `$85:96B5-$9961` (three owned slices): live actor vertical and planar
  fixed-point commits, prior-position storage, movement distance,
  doubled speed, facing, and velocity direction across both observed exits.
- `$86:AB73-$AF4D`, `$86:A6B3-$A78F`: grounded pass initialization and the
  mode-15 release core, including native receiver timers and reaction cadence.
- `$86:E923-$E96E`, `$86:F0FD-$F1AF`: paired defensive target projection and
  loose-ball pursuit permission.

The velocity replay exposed a ROM-specific negative damping bias: for this
routine, `-128` contributes `-7`, not normal C truncation's `-8`. The port now
matches all captured outputs. The 63,800-frame CPU-vs-CPU regression and its
visual anchors pass at this checkpoint.

The latest gameplay-path increment adds 570 observed-executed verified bytes,
raising ground-truth coverage from 5.78% to 7.82% (+2.04 percentage points).
Its actor/pass replays cover 1,105 live ROM calls with zero owned-output
mismatches. They exposed and corrected actor 8.8-to-16.16 velocity scaling,
zero-velocity direction preservation, pass-coordinate truncation, the native
10-tick release reaction, pose-resource `+$66` ownership, and two pass-pose
selection branches.

The defense-role increment adds another 480 observed-executed verified bytes,
raising ground-truth coverage from 7.82% to 9.54% (+1.72 percentage points).
Its 26-call replay compares all represented planner globals and ten actor
records with zero mismatches. It corrected cached loose-ball focal input,
protected-basket versus assignment-anchor selection, the forced three-pass
role rebuild, `$09DA` scratch-counter lifetime, reciprocal release distance,
and the owner's post-repair assignment reload.
The integration regression also now keeps receiverless dead-ball recovery on
the active inbound side and scans for a side-gate-valid teammate at the final
retry threshold, preventing repeated five-second-violation loops.

The ownerless-ball increment adds 681 observed-executed verified bytes,
raising ground-truth coverage from 9.54% to 11.98% (+2.44 percentage points).
The replay selected 252 genuinely ownerless calls from 500 live entries and
matched every represented ball, rim, pass, score, RNG, and event output. It
corrected negative-Y miss recoil, gravity on the made-basket detection
substep, and an event-bit copyback that erased the ROM's score marker.

The attached-ball/target/contact increment adds 882 observed-executed verified
bytes, raising ground-truth coverage from 11.98% to 15.14% (+3.16 percentage
points). Its four replays cover 1,086 retained live calls with zero mismatches.
The contact replay exposed two exact integration details: `$86:D652` consumes
the integer-coordinate pointer list prepared by `$86:D5DB`, and
`$86:C2C1-$C300` reverses ordinary opponent recovery cooldowns from 8/2 to
2/8 for odd or high control modes. The production port now preserves both.

The collision/acquisition/launch increment adds 839 observed-executed verified
bytes, raising ground-truth coverage from 15.14% to 18.15% (+3.01 percentage
points). Its replays retain 1,475 player sweeps, 15 ball-contact sweeps, 14
direct catch installs, 21 acquisition continuations, 80 loose-ball pursuer
scans, and 12 shot launches with zero owned-output mismatches. They corrected
assigned-defender velocity/nudge ordering, catch timer `+$60` ownership,
same-side catch clock preservation, non-snapping acquisition state, and both
65816 carry quirks in fractional-Z shot velocity construction.

The preceding motion/pose increment added 837 observed-executed verified bytes,
raising coverage from 18.15% to 21.15% (+3.00 percentage points). Its 6,287
retained live calls pass with zero owned-output mismatches. It corrects
phase-zero snapping, low upper-resource variant selection, inflated contact
heights, cached offense focal timing, and the play barrier's scratch-counter
cadence. Seven cadence/resource witnesses and five head-anchor heights now
run in the normal C regression, independent of ignored local captures.

Motion proof is in `.analysis/motion-cadence-proof-20260826/`: 1,300 rendered
frames, `cpu-motion.mp4`, and gameplay JSONL. Frames 600/1300 were inspected
before updating the integration hashes. This is progress toward smoother
movement, not a claim that all gameplay motion now matches the ROM.

The preceding action/pass increment added 258 observed-executed verified bytes:
21.15% -> 22.07% (+0.92 percentage points, the bounded ~1% checkpoint).
All 6,471 new calls and the existing 15 pass-init/100 release calls replay
without owned-output mismatches. Forty-two checked-in WRAM witnesses run with
`build.ps1 -Test`; local Mesen captures are not required by that regression.
The `raw.animation_rom` trace object distinguishes literal resources, phases,
accumulators, locks and queue cursors from the older compatibility fields.
Ordinary pass start/release/completion and its hand use the same ROM phase.
The final `build.ps1 -Test` passes every suite, including 63,800 CPU frames,
1,848 exact-pass frame checks and 99 automatic action unlocks. The local
`regression.log` in the proof directory records that run.
Visual proof: `.analysis/action-animation-proof-20260826/pass-animation.mp4`,
1,300 source frames and gameplay JSONL. Broader movement fidelity is unfinished.

The preceding pose/appearance increment added 139 observed-executed verified bytes:
22.07% -> 22.57% (+0.50 percentage points). Both ten-player initialization
calls and 26 live action-pose refreshes match every owned output. The resolver
does not advance phases, accumulators, or locks. All 28 checked-in exit witnesses
run in `build.ps1 -Test`, independently of local Mesen captures. The prior
6,471 action/cadence, 15 pass-init, and 100 release calls still match exactly.
The 63,800-frame CPU test passes with 2,158 exact-pass frame checks and 99
automatic unlocks; visual hashes are unchanged. Fresh 1,300-frame video,
screenshots, JSONL, and the full-suite log live in
`.analysis/action-pose-proof-20260826/` (`pose-refresh.mp4`).
Ghidra labels/dump: `tools/ghidra/DumpActionPose.java`, with local output in
`.analysis/action-pose-ghidra-20260826/`. Low-resource variant/facing-8 branches
are not exhaustively exercised by these live action calls; do not confuse
this measured routine coverage with complete branch or whole-game fidelity.

The preceding shot-action increment added 140 captured verified address positions:
22.57% -> 23.07% (+0.50 percentage points). All 167 fresh live calls match:
14 recovery, two moving starts, 24 facing/release decisions, two cleanups,
123 wind-up timer decisions, and two lower-body jump installs. Checked-in
WRAM witnesses run in `build.ps1 -Test`. The gate replay includes five real
facing corrections; its `$85:F02D` quantizer uses strict signed comparisons,
not the nearby target-distance helper's tie rules. RNG is sampled, not stepped.

That checkpoint adopted facing/release decisions (`$86:B8CA-$B978`, excluding
launch calls), recovery (`$86:9846-$986C`), and cleanup (`$86:B8C0-$B8C8`).
Its startup/timer/jump helpers were initially helper-only; the current
shot-branch checkpoint below connects them. Its historical full-suite pass
included 63,800 CPU frames, 2,078 exact-pass frame checks and 94 unlocks.

Proof: `.analysis/shot-action-proof-20260827/` contains `shot-facing.mp4`,
3,600 source frames, screenshots, gameplay JSONL and `regression.log`.
Ghidra labels/comments are in `tools/ghidra/DumpShotAction.java`; fresh bank
$85/$86 dumps are in `.analysis/shot-action-ghidra-20260827/`, with focused
recomp output in `.analysis/shot-action-recomp-20260827/generated/`.

### Preceding stationary-shot / lost-possession checkpoint

The requested 35 stationary-shot/sidestep instructions (`$86:B7F7-$B849`)
and 22 lost-owner/pump-fake instructions (`$86:B867-$B86B`,
`$86:B886-$B88F`, `$86:B890-$B8BF`) are implemented. The existing shared
cleanup is reused. The missing button/CPU connector, its return, and the
owner/latch gate add 28 connecting instructions. This adds **16 captured
verified address positions**, 23.07% -> **23.13%**; it is not a 57-address
increase because most rare-path instructions were absent from the baseline
exec captures.

`tests/fixtures/shot-branch-witnesses.json` retains 118 passing ROM calls:
75 natural calls (five sidestep, 70 CPU wind-up) and 43 controlled-ROM calls
(19 sidestep, two owner restores, four cancels, four button gates, five
owner/latch gates, nine extra release-facing cases). The controlled harness
changes WRAM inputs on real calls, never ROM/PC/flags/stack. No natural
lost-owner or cancellation call occurred in the 30,000-frame capture.
The extra `$86:9D7A-$9D98` facing helper was replayed but not adopted in
that checkpoint; it is now part of the complete launch above.

Ordinary startup now distinguishes stationary wind-up from an already-moving
jump. Its persistent `$0948` counter reaches the native jump/sidestep gate.
Owner loss restores team-relative mode/cooldown without canceling locks or
touching the ball. Pump cancellation waits for BOTH upper phase 4 and
accumulator $600, cancels both channels, and lowers the ball's integer Z to
40 while preserving its fraction. CPU wind-up does not read human buttons.
Animation descriptors and hand geometry remain asset-pack data.

Integration also preserves ball fractions at `$86:B7AF-$B7CA` instead of
copying player fractions, and excludes attached stationary shots from the
ownerless rebound fallback. Loose-ball recovery now dispatches from ball
state instead of requiring the host REBOUND debug label: canceled-shot/free-
throw continuations could otherwise strand a loose ball under DRIVE/ATTACK.
The original contact predicates still decide acquisition; this is not an
automatic floor pickup. Runtime self-tests exercise these contracts through
the real mode-12 dispatcher. The final 63,800-frame trace sustains scoring
(74-70); its longest dead-ball stretch is 1,552 frames, below the unchanged
2,400-frame guard. A movement-only analyzer was insufficient to catch the
earlier stranded-ball diagnostic run; retain the full strategy/scoring test.
Final `build.ps1 -Test` passes every suite, including all 118 new ROM
witnesses, 63,800 CPU frames, 2,194 exact-pass frame checks, 112 automatic
action unlocks, and inspected screenshot anchors 600/1300/3480/6932/6954.

Fresh Ghidra: `.analysis/shot-branches-ghidra-20260827/shot_action_bank86.txt`.
Capture/replay commands and provenance are in `tools/README.md`. Final-build
visuals and regression output belong in `.analysis/shot-branches-proof-20260827/`;
use `final-frames`, `final-gameplay.jsonl`, `final-stationary-shot.mp4`,
`endurance-recovery.jsonl`, and `regression.log`. Frames 6932/6954 show the
stationary hold and subsequent jump. Earlier diagnostic `endurance-current`,
`endurance-facing`, and `endurance-fractions` are not the final build.

Do not infer that a surrounding routine is verified from one verified slice.
Only ranges present in `docs/verified-routines.json` count as ground-truth
verified.

## Evidence rules

1. **Observed executed code** is the union of Mesen `exec_*.txt` captures in
   `.analysis/**`. Current captures emphasize gameplay, so title/menu execution
   is underrepresented in the denominator.
2. **Documented code** is the intersection of those addresses with
   `$XX:XXXX` provenance comments in `src/*.c`.
3. **Verified code** must have Mesen entry/exit vectors replayed through
   the compiled C implementation with zero output mismatches, then be entered
   in `docs/verified-routines.json`. Distinguish natural gameplay calls from
   controlled-ROM input cases; neither is proof of all surrounding callers.
4. **Regression coverage** (trace hashes, long simulations, and screenshots)
   protects integration behavior but does not by itself make a ROM routine
   ground-truth verified.

See `tools/README.md` for capture, replay, Ghidra, and regression commands.

## Active gaps and next work

- Strict differential validation has started: `docs/differential-testing.md`.
  Fresh native/C captures compare 442 raw words at baseline and actual actor-
  sweep boundaries. Default native CPU-versus-human preserves the requested
  setup and currently reports81 baseline differences (previously82), including
  the known controller mismatch with the CPU-only port. First sweep cadence
  also differs. Full WRAM does not yet repeat across runs, despite matching
  selected-state traces. No full snapshot/configuration import or passing
  trajectory-equivalence claim exists. Align starting context and update order
  before interpreting later movement divergence. Per-routine replay coverage
  is 28.05%; the independently verified30-instruction initializer and239-start
  jump/reach parent add credit. The whole-game baseline still does not pass.
- Camera: the additional 99 correction/caller instructions are complete.
  Upstream camera/control selector ownership and wider configured-start
  trajectories remain separate; raw camera
  correctness does not make those input trajectories ROM-identical.
  Crowd CHR animation and the downstream basket/window compositor remain.
  The raw map geometry matches 12 native viewports; static art has 0-461
  crowd-tile pixel differences. See `docs/camera-presentation-plan.md`.
- Selector/launch and upstream made-run/fatigue/assistance writers are
  implemented; see `docs/shot-state-plan.md`. Remaining in this neighborhood:
  timeout/period caller integration (including stamina grants, period reset,
  quarter-clock initialization and end-of-period flow), substitutions and
  bench promotion, and wider natural-shot distribution comparison. The
  initial 43,200 clock seed remains a bounded scaffold; the forced frame220
  tip handoff has been removed. The
  verified clock-writer semantics do not certify all clock setup/callers.
  Human controls remain out of scope.
- The command helpers and common queued completion now have live replay
  proof, but generic shot/contact callers still use compatibility
  setters; do not replace all of them blindly. Ordinary mode-15 adoption is
  done, but inbound stays on the compatibility path: adopting its changed
  release/hand geometry exposed a >2,400-frame dead-ball stall in endurance
  testing. Keep that regression guard; trace the inbound continuation next.
- Upper mode-2 states 7/13/18 have verified, adopted cadence. Natural state7
  selection still needs the defensive `$86:E39A/$E3E1` pose callers; do not
  invent a random idle-selection rule. Ordinary dribble/contact physics
  still uses legacy tick-derived phases; exact rendering alone is not proof
  that those gates are ROM-identical.
- Latched/unlatched owner posing and the ordinary `$86:F34F-$F439` caller
  are verified/adopted. The `$F43A` inbound continuation and defensive pose
  callers remain distinct work; verified bodies do not certify the entire
  dispatcher or full-game trajectories. See `docs/owner-flow-plan.md`.
- Continue converting small post-tip CPU decision/animation slices from the
  recomp and Ghidra, re-verifying after each increment.
- Expand ball physics, scoring, and CPU decisions beyond the early-gameplay
  paths currently exercised; keep fouls scaffolded until their native branches
  are captured and verified.
- Add full-session execution coverage so title and menu code participate in
  the denominator.
- Use `docs/progress.md` to select larger undocumented regions only when they
  advance the active gameplay path; raw byte count alone is not priority.

## Resume checklist

1. Confirm `main` is clean and current.
2. Regenerate `docs/progress.md` rather than copying numbers into a new note.
3. Read the active gap above and the corresponding Ghidra listing/recomp code.
4. Capture live vectors, implement the smallest complete branch, and replay
   every vector with zero mismatches.
5. Run the relevant subsystem test plus `tools/test_cpu_gameplay.py` for
   gameplay changes, inspect visual anchors, then update the verified ledger.
6. Commit and push each verified checkpoint to `main`.
