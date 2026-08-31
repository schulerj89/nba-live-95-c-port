# Visible defect recovery plan

Date:2026-08-31. User-review baseline: preview executable
`b0c2f2f903e94ee61bb54de654628659665b8190bfee5540d545c93cf8ebb69a`,
264-resource pack
`f564c29612928984002ed3f0389d317de639fff122baf61a7bc9ecaef2a6be09`.
The preview accurately exposed that recent prerequisite/test checkpoints did
not repair several high-visibility routes. These defects now block further
release work. None is classified as an original-game bug without source proof.

## Reproduction gate

`independent_qa` owns test-only production-path scenarios and consecutive
screenshots for each ticket. A scenario may set an explicitly declared C test
entry or clock only when necessary; it may not seed normal runtime, alter the
ROM/assets, or be cited as native/natural parity. Unsupported quick entries
must refuse. Each report binds commit, executable, ROM, pack, input script,
trace and screenshots. The existing `tools/quick_live_scenario.py` already
covers CPU pass/catch, Q1 expiry and pause/resume and refuses unsupported live
routes honestly.

The exact failing baseline is frozen outside Git at
`.analysis/progress-screenshots/20260831T222202.578262Z-user-defect-baseline-preview-ee0eee9/`.
Its corrected failure matrix reproduces the port's frozen outgoing fade followed
by black frames 209..298, ignored Start presses in matchup/ratings, `312 TURNER`,
and an inbound that is ready at frame 546 but has not transferred by frame 650.
The longer production trace transfers at frame 674, so the bounded baseline does
not prove an indefinite stall and the source timer remains unchanged pending
native review. The earlier 109-frame all-black result was a screenshot-tool
warmup artifact and is retained as such. Scripted Right presses caused the
observed settled Team Select changes, so that route does not reproduce an entry
glitch. It does reproduce the incorrect one-card lineup Start behavior and the
ratings/logo overlap. Neutral Start enters Player Intro in the scripted path,
and the first bounded pass does not reproduce the hand offset; those two reports
remain open for broader interactive/directional coverage.

| Ticket | Current observed defect | Owner | Required regression and exit |
|---|---|---|---|
| F1 | Corrupt/glitching frames on entry to Team Select | `startup_nmi` | Source/native review at `ba52557` proves the clipped BG3 reveal fragments are original behavior; they are preserved and commented rather than hidden as a port bug |
| F2 | Incorrect transition into Player Setup | `startup_nmi` | Fixed at `ba52557`: outgoing Team Select layers withdraw independently, forced black begins at the source boundary, and Player Setup retains its destination reveal |
| F3 | Centered controller cannot confirm CPU-vs-CPU | `startup_nmi` | Fixed and production-routed at `ba52557`: native selection `$166D=1` confirms through Player Intro to Tipoff with zero human assignments; the debug overlay also reports `NEUTRAL` |
| F4 | Team presentation and starting lineup cannot be skipped | `startup_nmi` | Fixed at `ba52557`: source-backed Start edges skip Matchup, Ratings, and the complete lineup; Left/Right navigate lineup cards and the non-source A shortcut is removed |
| F5 | Intro/lineup text renders incorrectly | Root | Fixed at `b42350d`: render the ROM strip count from each proportional descriptor width; focused regression passes and independent exact-build screenshots prove `312 TURNER` is now `31 TURNER` and clipped I/M/W glyphs are complete |
| G1 | Court logo is corrupted | `runtime_graphics` | Multiple home courts and camera positions use source-produced indexed court/logo data; no flattened/captured substitute or broken tile seam |
| G2 | Jersey/player numbers display malformed values such as `312` | Root for reproduced lineup case; `runtime_graphics` for live jerseys | Lineup bleed is fixed/proven at `b42350d`; `9d5bc10` activates the literal number/body compositor with resource 287, pending final exact-build visual review across live jerseys |
| G3 | Inbound presentation glitches | `runtime_graphics` | Source timing is preserved and commented; the integrated 850-frame production route passes ready/transfer/retry/release milestones with no host timer shortcut using the resource-287 pack |
| G4 | Ball is not at the hand during pass/catch | `runtime_graphics` | Fixed by `9d5bc10` plus `a5a551e`: the submitted live body pose and ball attachment both consume literal actor `+$28`; the 20,000-frame probe passes 846 dynamic observations and 368 direction-only offset differences |

Root alone integrates shared `NbaGame`, session, production manifest and final
asset-pack changes. Text fixes and each agent packet receive independent review
before merge. Any original quirk discovered during source comparison is kept,
commented and added to the known-bug catalog; port-only corruption is fixed.

The graphics pair requires the append-only `NBPDRAW1` resource 287. The final
candidate pack preserves all 264 production payloads and has SHA-256
`acc4a436c990fd3a7beab9dadab47d40690ccedf912f7db90e283264ed0f299a`.
Running the attachment-mask patch against the old 264-resource pack is not a
valid release configuration because that pack leaves the literal compositor
inactive.

## Checkpoints

1. Freeze the current failing route matrix and screenshot sequences.
2. Integrate F1-F5, run the complete frontend scenario from Game Setup through
   Tipoff, commit/push, and publish an exact-build screenshot gallery.
3. Integrate G1-G4, rerun CPU pass/catch and inbound sequences across multiple
   teams/courts, commit/push, and publish a second exact-build gallery.
4. Build a new isolated desktop preview only from the accepted commit and pack;
   verify the shortcut target/arguments and smoke the same routes. The standing
   release shortcut remains untouched until the final release candidate.
5. Resume the remaining full-game plan only after these visible blockers pass.

No ticket passes on a static helper image, component-only probe, refreshed
golden without attribution, or a single hand-picked frame. Before/after route
evidence and the first remaining defect are recorded at every checkpoint.
