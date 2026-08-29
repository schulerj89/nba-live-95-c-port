# Foul-out substitution continuation audit

This is a read-only implementation audit. It identifies the native ownership
boundaries following the typed foul-out request (`$09CA=8`, `$0A08=1`) but does
not claim that the C port executes the lineup transaction yet.

## Native parent and return boundary

The foul/whistle presentation owner is `$83:EBD8-$ED46`. For presentation
selectors `$08E8=0x11` or `0x16`, `$83:ECA5` tests `$09CA XOR $09CC` and enters
the substitution refresh at `$83:ECB0` when exactly one request class is
pending. The refresh is:

1. `$83:ECB0-$ECC4`: suspend/clear presentation and emit the transition cue.
2. `$83:ECC7`: call lineup transaction `$83:ED73-$EE4F`.
3. `$83:ECD1`: call court-actor rebuild `$85:C0F6-$C37C`.
4. `$83:ECD5`: call `$87:8C66-$8C6A`, a wrapper around the active-player
   appearance builder `$86:D85E-$DA20`.
5. `$83:ECD9`: call resource-metadata rebuild `$87:AF95-$B058`.
6. `$83:ECDD`: call ten-actor resource binding `$87:AF75-$AF94`.
7. `$83:ECE1`: call draw/scheduler preparation `$87:A357-...`.
8. `$83:ECE6-$ECEA`: resume presentation and clear `$0A08`.
9. `$83:ED21-$ED46`: common audio/presentation tail and `RTL`.

`$86:F587-$F58C` is a separate consumer-side gate: a nonzero `$0A08` jumps to
the ordinary CPU continuation epilogue rather than selecting another inbound
receiver while the transaction is pending.

## Lineup transaction `$83:ED73-$EE4F`

`$09CA` is the request class used by the foul-out path. Its actor index is
`$492D`; values `0..4` address the first active five and `5..9` address the
second active five. The routine resolves that actor through `$46F9` or `$4779`,
marks its global `$4943` eligibility/status word as `1`, repairs that team's
lineup, clears `$09CA`, and writes `$492D=FFFF`.

`$09CC` is a parallel request class with actor index `$09CE`. It resolves the
same active-lineup tables, marks `$495B` as `FFFF`, repairs the affected lineup,
clears `$09CC`, and writes `$09CE=FFFF`. Its exact gameplay meaning is not
proven by the foul-out evidence and must not be named "foul-out" in typed C.

The team repair paths are exact:

| Team/selector path | Boundary | Owned mapping writes |
| --- | --- | --- |
| First lineup, incremental repair | `$83:939D-$9468` | swaps one active/bench pair in `$46F9`; writes selected record pointers to `$7E3C/$7E3E` and `$7E40/$7E42` |
| Second lineup, incremental repair | `$83:947D-$9548` | equivalent swap in `$4779` |
| First lineup, full automatic rebuild | `$83:9549-$95DA` | stages 12 entries in `$8FEE`, then copies all 12 words to `$46F9` |
| Second lineup, full automatic rebuild | `$83:95DB-$966C` | stages 12 entries in `$8FEE`, then copies all 12 words to `$4779` |

The incremental helpers search bench slots `4..11`, skip entries disqualified
by `$4943`/`$495B`, prefer the outgoing player's roster-position byte, and
fall back to another eligible bench entry. The full rebuild helpers use roster
record pointers `$3471/$3473` or `$34A1/$34A3` and child `$83:966D` to rebuild
the complete order.

`$4726` (first team) and `$47A6` (second team) choose full rebuild when zero,
or incremental swap followed by `$83:EE50-$EF47` when nonzero. The retained
evidence proves the branch and writes, but not a safe semantic name for these
selector words. `$83:EE50-$EF47` builds a substitution presentation from the
selected roster-record pointers. Input handling, cancellation, and any manual
lineup UI beyond this presentation builder remain unknown.

## Downstream actor/resource rebuild

- `$85:C0F6-$C37C` reconstructs the ten active court actors from the repaired
  lineup, resets position/velocity/action fields, invokes animation/resource
  children, advances actor bases by `$100`, and restores dead-ball timing.
- `$87:8C66-$8C6A` delegates to the already documented appearance builder
  `$86:D85E-$DA20`.
- `$87:AF95-$B058` reloads the ten roster pointers from `$3449/$344B` and
  repopulates body/skin/head/appearance metadata before its final child.
- `$87:AF75-$AF94` iterates ten `$100`-byte actor records and invokes
  `$87:AAB2` to bind their resources.
- `$87:A357-...` re-enters the normal draw preparation path. Its complete
  scheduler boundary is already owned by the gameplay renderer and should be
  called, not duplicated inside substitution code.

This call order is observable native behavior. A C implementation should make
the lineup replacement atomic, then invoke existing actor/appearance/resource
rebuild APIs in this order, and clear the typed request only after those calls
succeed. It must not invent a separate substitution executor.

## Capture status and required vectors

`tools/capture_substitution_continuation.ps1` is a natural-entry acquisition
harness for `$83:ED73-$EE4F`. The bounded CPU-vs-CPU run did not naturally foul
out a player, so it produced zero vectors. A controlled attempt to publish a
request after player-resource initialization found no later genuine
`$83:EBD8` parent opportunity in the bounded run. No PC, stack, ROM, or return
address was patched, and no synthetic fixture was retained.

Before production translation is considered verified, retain native vectors
for:

1. first-team full rebuild;
2. second-team full rebuild;
3. first-team incremental swap;
4. second-team incremental swap;
5. preferred-position candidate and fallback candidate;
6. unavailable/fouled bench entries skipped;
7. the parallel `$09CC/$09CE` class independently;
8. complete `$83:ECB0-$ED46` proof that mapping, actor state, appearance,
   bindings, draw preparation, `$0A08` clear, and return ordering agree.

The safest controlled capture is a real contact/classifier entry with the
selected player's persistent foul count prepared at five, followed through the
native whistle/presentation cadence. Publishing `$09CA/$0A08` in isolation is
insufficient because `$492D`, presentation latches, lineup state, and roster
pointers must all belong to the same live event.

## Implementation slices and dependencies

1. Typed lineup/status model for `$46F9`, `$4779`, `$4943`, `$495B`, `$492D`,
   and `$09CE`; no presentation work.
2. Evidence-complete automatic full rebuild (`$9549/$95DB` plus `$966D`).
3. Evidence-complete incremental replacement (`$939D/$947D`).
4. Atomic lifecycle parent that calls existing actor, appearance, resource,
   binding, and draw-preparation APIs, then clears `$0A08`.
5. Presentation-only `$EE50-$EF47` after its selector semantics and visible
   cadence have their own capture.

Unknown UI branches are intentionally separate from automatic foul-out
continuation. They must not block the typed request and automatic bench
promotion, but neither may they be replaced with a guessed menu.
