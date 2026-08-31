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
Its failure matrix reproduces the 109-frame all-black Player Setup hold,
ignored Start presses in matchup/ratings, `312 TURNER`, and an inbound that is
ready at frame 546 but has not transferred by frame 650. The longer production
trace transfers at frame 674, so the bounded baseline does not prove an
indefinite stall and the source timer remains unchanged pending native review. It partially
reproduces abrupt Team Select theme changes, whole-lineup skip, and the court
logo overlap. Neutral Start does enter Player Intro in the scripted path, and
the first bounded pass does not reproduce the hand offset; those two reports
remain open for broader interactive/directional coverage.

| Ticket | Current observed defect | Owner | Required regression and exit |
|---|---|---|---|
| F1 | Corrupt/glitching frames on entry to Team Select | `startup_nmi` | Game Setup confirm through settled Team Select, consecutive transition frames and team changes; no stale/foreign tiles, palette or OAM pieces |
| F2 | Incorrect fade into Player Setup | `startup_nmi` | Team Select Start through settled Player Setup compared at source transition boundaries; replace the current 109-frame fully black hold with the source transition sequence, retaining only source-demonstrated forced blanking |
| F3 | Centered controller cannot confirm CPU-vs-CPU | `startup_nmi` | Move pad0 to native neutral `$166D=1`, press the original confirm input, enter next scene and Tipoff with no human assignment or forced side |
| F4 | Team presentation and starting lineup cannot be skipped | `startup_nmi` | Press/release edges at every presentation phase; match original permitted skip destinations and handoff without repeated held-input activation |
| F5 | Intro/lineup text renders incorrectly | Root | Fixed at `b42350d`: render one or two ROM strips from each proportional descriptor width; focused regression passes and independent exact-build screenshot review remains |
| G1 | Court logo is corrupted | `runtime_graphics` | Multiple home courts and camera positions use source-produced indexed court/logo data; no flattened/captured substitute or broken tile seam |
| G2 | Jersey/player numbers display malformed values such as `312` | Root for reproduced lineup case; `runtime_graphics` for live jerseys | The roster is 31 and the extra 2 was adjacent-glyph bleed fixed by `b42350d`; independently replay all ten cards, then separately verify live jersey BCD/tile composition |
| G3 | Inbound presentation glitches | `runtime_graphics` | Consecutive frames before whistle/dead ball, inbound setup, throw/release/catch and return to live play; no stale sprite, teleport, queue residue or formation flash |
| G4 | Ball is not at the hand during pass/catch | `runtime_graphics` | Existing CPU-pass scenario plus held, windup, last attached, release, flight and catch frames; compare source hand point and ball OAM origin without changing ball physics/world coordinates |

Root alone integrates shared `NbaGame`, session, production manifest and final
asset-pack changes. Text fixes and each agent packet receive independent review
before merge. Any original quirk discovered during source comparison is kept,
commented and added to the known-bug catalog; port-only corruption is fixed.

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
