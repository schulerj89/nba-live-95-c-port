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
| documented by ROM-address provenance | 8,855 | 31.7% |
| verified against live ROM calls | 5,064 | 18.15% |

These values are generated, not estimated. The detailed per-bank report is
`docs/progress.md`; the authoritative verified list and evidence paths are in
`docs/verified-routines.json`.

## Verified gameplay checkpoint

Forty-nine routine slices currently pass emulator-ground-truth replay. The most important
recent slices are:

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
