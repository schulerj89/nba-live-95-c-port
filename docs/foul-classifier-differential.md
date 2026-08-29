# Contact-foul classifier differential

`$86:C4FE-$C6AC` is the native contact-foul decision tree called from the
player/player collision sweep. Six direct natural ROM calls are retained in
`foul-classifier-witnesses.json`; their RNG, event, shooting-foul,
free-throw, whistle, team-foul and event-bit outputs replay with zero
differences through `nba_gameplay_foul_classify_contact`.

The direct natural population consists of rejection and RNG-masking paths.
Accepted defensive, charging and offensive branches are covered by forced
production self-tests, including detached-shot deferral, period-dependent
bonus thresholds and the six-personal-foul cap. More importantly, the
existing 1,475-call full player-contact sweep differential exercises this
helper at its real runtime call site and compares all persistent foul words;
175 state-changing collision sweeps match the original ROM.

The external personal-stat child `$86:C493` remains outside this range's
credit. Its portable outcome is represented by the capped per-player foul
counter, but its native profile-table writes require a separate direct
capture before that child can be credited.
