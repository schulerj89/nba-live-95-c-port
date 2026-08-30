# Free-throw completion differential

## Evidence boundaries

- `$87:9CBF-$A017`: composite stripe state machine. CPU completion witnesses retain
  states 1, the nested transient 2, 3, 9, 10, and the between-attempt 11..24
  cadence. A separate genuine-controller capture retains represented aim words
  for human states 3/4/5/9. Because unrepresented setup, artwork and common-
  launch effects and their same-call ordering are internal to this boundary,
  its aggregate ledger row grants no address credit.
- `$87:A018-$A045`: controller-owned two-axis aim oscillator; see
  `docs/human-free-throw-differential.md`. This exact subrange is independently
  credit-eligible.
- `$87:A15C-$A2FD`: lane target/movement helper and its carry-ready return.
  `$A2FE-$A356` is table data; `$A357-$A360` begins the following presentation
  routine and is not claimed as lane-helper code.
- `$85:9530-$9597`: transient state-two presentation/timer consumer.

The controlled script waits for the real Exhibition gameplay draw path, then
requests a CPU attempt by writing only the documented shooter, controller,
free-throw state/count, and whistle words. ROM, PC, stack, processor flags,
and RNG are not patched. Every captured call enters and returns through the
native routines.

## Native witnesses

`.analysis/free-throw-native-20260829-v5` contains a complete two-shot
sequence: lane arrival, 1 -> 2 -> 3, CPU aim 3 -> 9, release 9 -> 10, a made
first attempt and score increment, 10 -> 11 -> ... -> 24 -> 1, a second CPU
attempt, and a final miss that clears state ten for a live rebound.

`.analysis/free-throw-native-20260829-v6-one` contains the one-shot/and-one
shape. Its final make increments the shooting team score, establishes inbound
state `$0952=5`, then clears state ten. Both captures retain ball owner,
`$07F6` RNG, `$08DE/$09BE` timers, scores, `$08E6/$08E8` audio words,
`$180B/$180C` upload words, and the `$3EEF-$3EFD` resolution object.

The durable normalized subset is
`tests/fixtures/free-throw-completion-witnesses.json`; the build invokes
`tools/verify_free_throw_completion_vectors.py` against the typed C probe.
It requires eight cases: presentation state two, CPU state three RNG/aim,
two-shot and one-shot releases, between-attempt ball setup, state 24 wrap,
final make/inbound-ready acknowledgement, and final miss/live-rebound
acknowledgement.

The separate canonical human-input capture is
`.analysis/human-free-throw-native-20260829-v4`: 1,556 native vectors with
first-press delay 60, zero orphan exits, and zero shared-exit callbacks. Its
vector-corpus SHA-256 is
`a1c252ab961d6e72d4159553706a16176dd151ba4f26e5343d39e4808486dabd`.
Seven durable human witnesses include a distinct cursor wrap.

## Adopted C behavior

- CPU aim uses the exact delay mask, rating thresholds, RNG consumption, and
  fixed/miss aim words from `$87:9DA6-$9E26`.
- State one now publishes transient state two and runs the retained `$85:9530`
  timer/audio gate rather than jumping directly to state three.
- State nine decrements the native remaining-attempt word and restores the
  `$180B/$180C=$800C/$8080` upload state.
- Between attempts apply the native `$3EEF-$3EFD` gameplay-ball placement:
  shooter X/Y, Z 32, and zero velocity, then enter state 11. Fractional
  coordinate bytes remain intact.
- The 11..24 cadence wraps to state one exactly.

## Deliberate exclusions

Human aiming states 3/4/5 and the oscillator are implemented and independently
locked by seven witnesses in `docs/human-free-throw-differential.md`. Their aim
artwork words `$0988-$098E`, global player assignment, and a complete playable
human-control path remain separate; the ordinary runtime adapter remains
dormant. Complete common-launch effects and same-call ordering remain excluded.
The alternate roster-dependent commentary
selection producing `$1F/$23` is also excluded; the retained CPU witnesses
select `$1B`. The C final-flight gate uses the typed gameplay-ball mapping for
native `$3EEF-$3EFD`.
