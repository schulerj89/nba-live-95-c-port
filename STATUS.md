# Project status

Last updated 2026-08-26. This is a current-state handoff, not a milestone log.
Use Git history for older checkpoints and run
`python tools/progress.py --write docs/progress.md` for live measurements.

## Current state

The playable path runs from the Nintendo license through Player Setup,
starting-lineup cards, tip-off, and early CPU-vs-CPU gameplay. The port has
court/camera movement, ten-player composition, possession, passing, shooting,
bounces, and initial foul scaffolding. User-controlled gameplay is not in
scope yet.

Current measured coverage:

| metric | bytes | % of observed executed code |
|---|---:|---:|
| observed executed code | 27,901 | 100.0% |
| documented by ROM-address provenance | 9,240 | 33.1% |
| verified against live ROM calls | 6,159 | 22.07% |

These values are generated, not estimated. The detailed per-bank report is
`docs/progress.md`; the authoritative verified list and evidence paths are in
`docs/verified-routines.json`.

## Verified gameplay checkpoint

Eighty routine slices currently pass emulator-ground-truth replay. The most important
recent slices are:

- `$87:B37C-$B571`: independent action install/cancel/reverse helpers (471
  live calls). Locked completion/queued continuation slices of `$87:ABC2-$AD18`
  and mode-2 idle state 7 also pass a 6,000-call post-locomotion replay.
  Ordinary live-play mode-15 passes integrate exact locks, release phases
  and hand resources; inbound passes retain their compatibility path.
  Other callers and the idle RNG stream are **verified helpers, not yet fully
  adopted gameplay paths**; see the active gaps below.
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
- `$85:9192-$93F4`: complete camera targeting and acceleration, including
  the fixed-point X projection carry and no-team hold.
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

The current action/pass increment adds 258 observed-executed verified bytes:
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

Do not infer that a surrounding routine is verified from one verified slice.
Only ranges present in `docs/verified-routines.json` count as ground-truth
verified.

## Evidence rules

1. **Observed executed code** is the union of Mesen `exec_*.txt` captures in
   `.analysis/**`. Current captures emphasize gameplay, so title/menu execution
   is underrepresented in the denominator.
2. **Documented code** is the intersection of those addresses with
   `$XX:XXXX` provenance comments in `src/*.c`.
3. **Verified code** must have live Mesen entry/exit vectors replayed through
   the compiled C implementation with zero output mismatches, then be entered
   in `docs/verified-routines.json`.
4. **Regression coverage** (trace hashes, long simulations, and screenshots)
   protects integration behavior but does not by itself make a ROM routine
   ground-truth verified.

See `tools/README.md` for capture, replay, Ghidra, and regression commands.

## Active gaps and next work

- Migrate the next non-pass action caller together with its release/cancel
  branches. The command helpers and common queued completion now have live
  replay proof, but generic shot/contact callers still use compatibility
  setters; do not replace all of them blindly. Ordinary mode-15 adoption is
  done, but inbound stays on the compatibility path: adopting its changed
  release/hand geometry exposed a >2,400-frame dead-ball stall in endurance
  testing. Keep that regression guard; trace the inbound continuation next.
- Finish upper mode-2 states 13/18. State 7's randomized timer is verified,
  but adopting its RNG consumption in ordinary runtime locomotion is deferred
  until the whole idle path is connected. Ordinary dribble/contact physics
  still uses legacy tick-derived phases; exact rendering alone is not proof
  that those gates are ROM-identical.
- Capture enough calls to verify the rare latched owner/CPU pose branch at
  `$86:E4F5-$E544`; the existing six-call sample is insufficient.
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
