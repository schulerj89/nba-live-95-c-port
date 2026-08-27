# Event-driven tip-off plan

Goal requested 2026-08-27. Baseline main35a3d85, verified7300/27901 captured
address positions (26.16%). Complete these checkpoints in order, with native
replay, runtime regression and a separate commit/push after each:

1. Tip contact: replace the scheduled contact result with the ROM geometric
   contact/classification input path; retain later stages explicitly until
   their checkpoints are verified.
2. Receiver selection: execute native RNG/team-relative receiver selection,
   state writes and associated caller event instead of fixed actor8.
3. Ball deflection: use the native pass-launch tables/math and tip wrapper's
   height restoration/zero vertical velocity, then the shared physics.
4. Actual possession: use contact-triggered acquisition and native state
   transition, not a forced frame220 receiver/clock/gameplay reset.
5. Recount and report remaining pending instructions by area. Unknown counts
   must remain unknown rather than reusing the unrelated323 CPU backlog.

## Pre-implementation findings and provisional bounded census

Fresh `DumpTipoffFlow.java` produces contiguous code listings from existing
bank86 Ghidra. Focused instruction-level recomp C was regenerated separately
under `.analysis/tipoff-flow-recomp-20260827/generated/bank86_v2.c` from five
explicit roots (no changes to the reference project's normal generated code).

| Stage/body | ROM | Decoded instructions | Proof boundary |
|---|---|---:|---|
| Contact coarse gates | 86:CCFC-CD96 | 62 | Shared with existing contact paths |
| Contact geometry/reach gates | 86:CE88-CF9F | 116 | Pose/body result; later classifier separate |
| Receiver selection/caller | 86:B04C-B0E1 | 59 | Launch child counted separately |
| Deflection launch | 86:99C4-9C44 | 286 | Shared pass helpers/tables require re-verification |
| Tip wrapper | 86:D3C6-D43D | 60 | Saves/restores height around receiver/launch |
| Acquisition continuation | 86:D25A-D3C5 | 155 | Existing proof explicitly excludes state81 branch |
| Jump startup dependency | 86:EC32-ECF8 | 77 | Outside the four selected body boundaries; inspect binding |

These are decoded body sizes, NOT new unverified-instruction totals. Shared
acquisition BAA2-BC99 and collision/pose helpers already have proofs. Finish
the dependency/branch census before claiming complete instruction coverage.
Early toss, jumping presentation, startup clock and renderer frame gates are
also identified dependencies; report any not replaced rather than implying
complete ROM tip-off parity.

## Native evidence

Evidence root `.analysis/tipoff-flow-proof-20260827/`. `natural` captures
170 geometry calls, two acquisitions, one receiver, one deflection, one mode
restore and one tip wrapper. First contact/selection/deflection occurs at
native routine-entry frame200; a distinct acquisition occurs at220.
End-frame snapshot labels follow their executing routine labels by one.

Critical ordering: D35B calls BAA2 before D3C6; D3F9 calls B04C (not BAA2).
B04C consumes RNG, chooses team base+3 or+4, writes0946, installs mode10,
and calls99C4. The wrapper restores both original Z words and clears velocity
3EFD; it also inhibits the opposite center for20 ticks. Do not hardcode the
observed actor8 or confuse the initial temporary catch with final possession.

Verification requirements:

- Compare native inputs/outputs, preserved fields, RNG, branch PCs and child
  call boundaries. Expected outputs must never come from the port.
- Keep controlled-WRAM cases separate from natural gameplay. Never change
  ROM/PC/flags/stack to fabricate a witness.
- Add durable fixtures, semantic integration guards, screenshots/video and
  full CPU regression before each completed-stage checkpoint.
- Runtime sprites, animation descriptors and launch tables remain asset-pack
  data. Mesen screenshots are comparison evidence only.
- Keep the goal active until all four stages and the final pending-area
  report are actually complete.

## Checkpoint 1: contact

`nba_tip_contact_geometry` translates the integer coarse/point/body gates.
The live adapter reads asset-pack pose offsets/head height, scans the existing
stable actor order on the logical cadence, and records actor/frame/reach in
CLI JSONL. A far ball cannot cause the old scheduled hit; placing a ball in
the body window at frame42 does. The placeholder deflection now starts only
after the geometric event, not before it.

Native replay: 170 natural +170 controlled calls, zero mismatches; 106 calls
in the controlled capture vary WRAM inputs. 143/178 gate instruction starts
are witnessed; the remaining35 include owned/inbound/non-tip alternatives,
not a claim of full contact-classifier coverage. Native pose-child outputs
are inputs to these tests; they do not independently prove the pose decoder.
Durable fixtures are `tests/fixtures/tip-contact-{natural,controlled}.json`.
The runtime test also checks altered geometry, single-event and odd-tick gates.

Proof correction: the older CCCD-D548 ledger range included the expressly
excluded state81 wrapper. Split it around D3C6-D43D pending its own proof;
coverage7300 ->7242/27901 (25.96%), not an implementation regression.
Upstream toss/jump timing, reach-animation adoption, receiver and final catch
remain separate. Do not describe this checkpoint as whole-tip visual parity.

Release check: natural C contact is actor5/frame204 (native call frame200).
Do not fit a frame offset to conceal the remaining toss/pose difference.
`stage1-full-test.log` passed the routine/asset/menu/owner endurance sections,
then caught an experimental jump-clock change at frame170. That unrelated
change was removed. Rebuilt tip-off, gameplay-debugger, 63,800-frame CPU and
intro tests then pass; only the reviewed frame220 camera regression anchor
changed. Visual evidence: `stage1-frames`, `stage1-port.mp4`, and
`natural-visual/native_*.png`. They show the remaining pose/trajectory gap,
not a claim that both games are visually identical.

## Checkpoint 2: receiver

B04C prefix/suffix are separate reusable functions around the forthcoming
launch child. Native RNG chooses team base+3/+4; the live caller stores passer,
receiver, familyFFFF, band12, mode10 and the slot11 event descriptor. Five
native calls (one natural/four controlled) cover all59 starts with zero
mismatches. `tip_receiver_probe` tests both sides/bits and no repeat RNG draw.
The event descriptor is preserved, but its downstream scheduler is not yet
implemented or mislabeled as audio.

The changed RNG path exposed a latent ownerless-ball physics omission:
host LOOSE records skipped the shared 85:9A6A substeps. A foul froze the ball
at Z63 and stranded the inbounder indefinitely. Routing LOOSE through the
same physics as PASS/SHOT/BOUNCE fixes that, with a four-mode binding test
and 63,800-frame stall guard. No timeout teleport or RNG rollback was added.
Two ambiguous old test proxies were replaced with direct events: play request
consumption has a diagnostic serial (the flag may re-raise later that frame),
and scheduling is checked by the exact ten-actor mask/order/delta, not action
timers that can be reinstalled during a contact.

Release: 63,800-frame CPU regression passes, including all semantic guards;
tip-off and gameplay-debugger regressions pass. Five changed CPU image anchors
were inspected after the new RNG consumption; they are C regressions, not ROM
parity. Frame1300 still exposes loose-ball camera framing with players mostly
offscreen. Evidence: `stage2-{cpu,tip,debug}.log`, `stage2-frames`,
`stage2-port.mp4`. Coverage7297/27901 (26.15%),148 ledger slices.

## Checkpoint 3: ball deflection

`nba_tip_launch` implements99C4-9C44 and the20-instruction9C45-9C6E
offset helper. 126 native calls (2 natural/124 controlled), zero mismatches,
306/306 starts. Fixtures retain all35 input/output words, including preserved
fractions, opaque pointer copies, receiver velocity corrections at both court
limits, all table families and the mode/live-state alternatives. Runtime
tests check the natural (-1109,597) velocity, (-9,4) integer nudge, unchanged
Z fraction/height, zero VZ, both center inhibits and subsequent physics.
The old straight-line path and separate rendered deflection are removed.

The existing frame220 bridge now uses the full acquisition cleanup and event
bit; checkpoint4 still must replace its timing and fixed catcher entirely.
Native first-contact input also proves097E=FFFF, not the old zero default.
The default had falsely classified same-side acquisition as a turnover.
The source-pointer E0/E2 caller provenance is unrepresented at runtime (not
invented); its copy is verified in the isolated routine. Ordinary passes
still use their earlier launcher adapter; adopting this complete shared
implementation there is separate work.

Regression: launch/native and runtime guards pass; the63,800-frame CPU,
tip-off, and gameplay-debugger suites pass. Shot-launch diagnostics now retain
the pre-physics actor/value: native-order same-frame rim processing can clear
the public latch, so checking only its final value was an invalid proxy.
Inspected captures: `stage3-frames` and `stage3-port.mp4`; changed C visual
anchors are not ROM-parity claims. Coverage7527/27901 (26.98%),149 slices.

Expanded pending census: EC32-ECF8 is only a77-instruction prefix. Its
ECF9-EE75 loose-ball/jump continuation adds162 contiguous instructions.
Neither is certified by the four contact/handoff checkpoints; total239
decoded instructions require path-specific audit/adoption. E054-E0AB is
only a31-instruction ball-init prefix, not its full dispatcher/toss census.

## Checkpoint 4: actual possession

The live dispatcher now uses the actual geometric event, BAA2 temporary
catch, D365 branch, D3C6/B04C/99C4 launch, shared physics/contact sweep and
second acquisition. No frame220 ownership, fixed actor8, play rotation,
all-player mode reset, extra reaction-RNG draws or forced clock reset remains.
The native state/whistle branch controls clock continuation and camera input.

Full caller replay found0942 must receive the ball source10 (DP C2 remains
unchanged while the wrapper swaps96/9A), not the jumping player's index.
The adapter and both-team runtime tests now enforce that identity. Catch
mode's +60 is mirrored into the CPU decision timer rather than leaving the
old receiver countdown installed. Its probe no longer aliases +AA height
to the +60 timer.

Proof:164 native completion calls,28/28 starts;22 BAA2 calls (4 natural,
18 controlled);17 wrappers (2 natural,15 controlled),60/60 starts observed.
All represented outputs match. Previous14 core/21 non-tip continuation
replays also pass. Durable fixtures:tip-completion.json and tip-acquisition.json.
The wrapper's stack/DP preservation is represented by C locals, not CPU ABI
emulation. Launch pointer provenance and final pose attachment remain separate.

`tip_possession_runtime_probe` checks natural contact, withholding the ball
past220, a later physical catch, both teams/both RNG bits/whistle alternatives,
pass cleanup, +60 reset and preserved RNG/clock. Natural C contact/catch is
204/222 versus native200/220; the initial quadratic toss and jump frame/pose
scaffold is still a real visual limitation, not hidden by shifted captures.

Diagnostics now record contact and final possession frames and remove fake
frame-based collision reports. Exact actor24.8 Z is also in JSONL:96B5 only
clamps a negative post-step height, so landing exactly on zero preserves VZ
for one more step. The old rounded-height test incorrectly rejected that
native boundary. Strategy, camera and fatigue tests now use actual events
rather than assuming frame220 is initialization/ownership.

Visual evidence:stage4-frames/frame_0204.bmp,frame_0220.bmp,frame_0222.bmp,
frame_0400.bmp; reviewed CPU anchors600/1300/3480/6932/6954; stage4-port.mp4.
Native comparison:acquisition-natural/native_0200.png and native_0220.png.
These expose remaining pose/trajectory/framing differences, not full-ROM
visual parity. Runtime art and animation/physics tables remain asset-pack data.

Updated remaining work: `gameplay-pending.md`; fresh Ghidra census has602
bounded CPU/gameplay instructions plus19 optional human steering instructions.
Uncounted wider callers and integration gaps are explicitly separate.
Coverage7,585/27,901 (27.19%),151 ledger slices.

Release: `stage4-full-release.log`, final `build.ps1 -Test` exit0. All native
replays, asset safety/regeneration, camera/court, audio, menus, teams, player
lab/setup/intro, tip-off and gameplay tests pass. The63,800-frame CPU trace
checks139 selectors,28 made-run updates,2,398 exact-pass frames and115
automatic action unlocks. Two200,000-frame owner runs reach states13/18;
Chicago/Orlando selects/releases one special at76883/76910, reverse matchup
none. The latter is a distribution caveat, not hidden as proof of ROM frequency.
The separate delayed-contact and63,800-frame dead-ball endurance tests pass.
The existing desktop shortcut resolves to the rebuilt executable/asset pack.

Checkpoint commits: contact a7b8297; receiver55085cd; deflection c020692.
The actual-possession checkpoint contains this release report and the pending
table; use Git history for its hash (do not embed a self-referential hash).
