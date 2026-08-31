# Native identity adapter migration

These changes repair controlled probe inputs after the native team-context
correction. They do not change gameplay production code or native expected
outputs. Raw `$46EB` is home/context0, and `$476B` is visitor/context1;
legacy UI right maps to home and left to visitor. See the original stores and
bounded initialization proof in `team-context-initializer.md`.

Full suitev3 first failed the court panorama fixture. That fixture changes the
team after match initialization, but changed only the UI session field. It now
also publishes the declared identity into the native home context consumed by
the renderer. All812indexed views across29teams,16,000caller frames and four
period scenarios pass unchanged pixel assertions. Original failure is retained
in `build/court-runtime-diagnostic-v1.log` (line94,exit10); the corrected run is
`build/court-runtime-context-v1.log`.

Suitev4 reached the new-match fixture. Its clean-state assertion compared native
home0/visitor1 pause side directly with legacy UI left0/right1. Comparing with
the same established boundary conversion restores both return journeys and
the unchanged native startup projection. See `build/new-match-context-v1.log`.
Neither change modifies the A1 new-match reset.

Suitev5 reached pass initialization and exposed the same inverse input adapter
in the pass probe. Correcting native-context-to-UI conversion restores all15
unchanged vectors, including the two previously failing calls7/13. A search
found eight other probes with the same inverse assignment. Their adapters are
corrected too. The Formation adapter bypasses initialization, so it explicitly
publishes both native context identity words from raw input as well.

All12 affected native gates pass after fresh probe builds: close-finish91,
pass-init15, pass-release16, player-contact38, ball-contact4, play-request61,
formation96, formation-override10, mode11=61, normal-actor64, violation10 and
out-of-bounds46. `build/context-adapters-v1/report.json` records every command,
source/probe/fixture/pack identity and log hash. Expected values, comparison
fields and tolerances are unchanged. This does not cure existing verifier
limitations or expand each gate's native claim.

The separate Mode1 five-count C baseline update follows the independently
accepted source/config/pack attribution in
`completion-mode1-attribution-audit.md`, not these input repairs. Historical
counts are retained, all57,344per-pixel checks remain, and the output label
now correctly says C-only. No native trajectory/HUD parity follows.

No unexplained behavior has been called an original-game bug. The entire
build test suite has not yet passed, and the complete game remains unfinished.
