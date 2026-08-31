# Preserve wrapped direction comparisons and repair switch verification

The original switch checkpoint is rejected despite matching all40 retained
native calls. Its existing direction helper compares separately signed
operands; original `$85:F37D` and `$85:F399` branch on the sign of the wrapped
16-bit subtraction. The first comparison also accepts equality; the second
does not. This difference matters at full-word boundaries.

The global helper now performs those original wrapped comparisons, with source
PC comments. No other gameplay helper changed. For dx=$8000,dy=1 the negative-x
key plus swap selects ROM direction6 and distance$8000. In the independent
controlled switch case, the angular penalty produces$8080; the wrapped compare
to$0640 rejects that candidate and retains the current actor. The old C helper
instead transferred ownership. This is a controlled source-contract edge;
ordinary-court reachability has not been established.

The auditor's original manual prose said direction2, omitting the negative-x
key bit. Its independently written576-case executable guard always expected
direction6. The corrected report retains both counterexample JSON versions;
the score and switch-route discrepancy were unaffected. The root also retained
its first comment-only transcription in
`build/switch-repair-v1/helper-first-repair.c` before correcting that comment.

All11 original switch files and their dependency sources are retained in
`build/switch-repair-v1/original`. The controller agent's frozen files and native
captures are unchanged. The repaired switch body itself remains byte-identical;
only the shared direction helper, verifier and separate probe build change.

The old verifier accepted15 of38 independent corruptions. The repair bounds
every recorded word to16 bits; binds actor(C2), owner, live state, offense,
candidate, score and direction to their actual raw boundary words; and checks
the capture revision's Player/court clock origins, frame-to-court relationship,
global script stop bound and recorded completion. These clocks validate the
fixed diagnostic input route and never drive production timing. Existing exact
source/settings/artifact checks and all original C output comparisons remain.

Fresh `/W4 /WX` builds compile both the repaired global helper and switch source
into a new private directory. `build/switch-repair-v2` passes576 independent
arithmetic edge cases, the controlled retain-route case, all68480 original
native values across14 left and26 right calls,35 existing integrity mutations
and all38 independent corruption cases. The directional equal-score replacement
at entry95/event98 remains observed and unchanged.

Independent repair acceptance is pending. The broader suitev10 is testing the
global arithmetic repair; it has passed the earlier native gameplay/vector and
Setup gates so far. This component is not production-wired, and normal human
gameplay remains disabled. Full caller ordering, unsupported human actions,
native whole-game parity and Rules reentry remain separate work.

Independent review now accepts this bounded repair; see
`completion-human-switch-repair-independent-audit.md`. The exact reviewed
pre-acceptance note remains in `build/switch-repair-v2/reviewed-owner-repair.md`.
The broader suitev10 passed its preceding vector, Setup, core safety and Player
Lab gates, then failed the endurance journey at frame46450 with an inbound
target404/-224 and actor394/-219. A private counterfactual freshly compiling
the old helper against the same other objects fails at the same frame with
the same inbound state and score. Therefore the stall predates this repair;
it remains unresolved, not an established original-game bug or a suite pass.
Evidence: `build/tip-stall-attribution-v1/old-helper.log` and build/full-suite-v10.log.
