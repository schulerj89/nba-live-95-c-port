# Native clock-expiry and period handoff evidence

This note scopes the next lifecycle increment. It records retail-ROM behavior;
it does not claim that the C port implements period expiry yet.

## Capture method

`tools/capture_match_lifecycle.ps1` launches the retail ROM in Mesen for four
CPU-vs-CPU cases. `tools/mesen_match_lifecycle_capture.lua` waits for natural
live play and a valid owner at the verified `$85:EDC6` clock writer, then seeds
only period, one remaining clock tick, scores, and the expiry latch/event bit.
The ROM performs the decrement, horn dispatch, presentation, period update,
and next-clock initialization. The checked-in compact expectations are in
`tests/fixtures/match-lifecycle-expiry-witnesses.json`; full event JSONL remains
an analysis artifact.

The captures used setting index 3, hence 43200 regulation ticks and 18000 OT
ticks. Increment A separately verifies all four regulation and overtime table
entries.

## Exact dispatcher path

1. `$86:97CD-$97F2` tests `$0928`. At expiry, if `$09B4` is clear,
   `$86:97E6-$97E9` stores 1 to `$09B4`, and `$86:97EC-$97F2` sets bit `$0800`
   in `$13E7`.
2. `$87:8EB2` gates the gameplay dispatcher on `$09B4`.
3. If `$093E >= 0` (the ball has an owner), `$87:8EBC` jumps to `$87:95E9`.
4. With no owner, `$87:8EBF-$8EC7` jumps when ball Z `$3EF7 < 8`.
5. With a high ownerless ball, `$87:8ECA-$8ECF` jumps only when `$0946 >= 0`.
   Otherwise normal actor/ball simulation continues and the gate is retried.

That last branch is the shot-in-flight-at-the-horn rule. The port must not
transition merely because the clock reached zero: an ownerless unresolved ball
with Z at least 8 remains live until resolution or descent. The exact meaning
of every `$0946` non-negative value is not yet labelled, so the implementation
should preserve the signed gate rather than inventing narrower shot types.

## Owned writes and observed outcomes

The common `$87:95E9` scene waits on `$0564`, snapshots presentation state,
performs cleanup, and reaches the period decision. `$87:96F6-$96FB` applies the
common `$1000` stamina grant through `$87:985D`. End of period 2 (raw period 1)
also applies `$6000` at `$87:9711-$9716`. A tied end of regulation applies
`$3000` at `$87:9746-$974B` and sets `$15BD=1`.

| Controlled case | Native branch witnesses | Owned final state |
| --- | --- | --- |
| Q1, 10-8 | `$96FB`, `$976E`, `$86:DD2D-$DD47` | period 0→1, latch cleared, clock 43200 |
| Halftime, 10-8 | `$96FB`, `$9716`, `$976E`, `$86:DD2D-$DD47` | period 1→2, latch cleared, clock 43200 |
| Regulation tie, 10-10 | `$96FB`, `$974B`, `$976E`, `$86:DD2D-$DD47` | period 3→4, latch cleared, OT clock 18000 |
| Regulation non-tie, 10-8 | `$96FB`, `$976E`, `$979D`, `$97A0` | period 3→4→5, clock 0, postgame path |

`$87:9766-$976E` increments `$0926` only while it is below 4. At raw period 4,
`$87:9779-$977F` compares `$4711` and `$4791`. A tie routes back to play; a
non-tie stores 5 at `$87:979A-$979D` and enters postgame cleanup at `$87:97A0`.
The next-play route eventually reaches `$86:DD2D-$DD44`: raw period >=4 selects
the OT table into `$0A0C`, then `$86:DD44` copies `$0A0C` to `$0928`.

## Implemented dependency order

1. Model the expiry latch plus the signed owner/Z/resolution horn gate. Done.
2. Add a period-scene orchestration phase so the C tick cannot mutate periods
   directly from clock code. Done; controlled native wait lengths replace the
   old arbitrary placeholder, while exact scene raster composition remains pending.
3. Port common, halftime, and tied-regulation stamina/event branches. Done.
4. Port raw-period increment and score-based OT/final selection. Done.
5. Re-enter gameplay through the existing table helper and reset the latch;
   keep postgame routing a separate explicit state. Done.
6. Add presentation/audio fidelity only after the state transition witnesses
   match, because `$87:95E9-$9765` owns substantial scene and audio work.

Focused tests should cover the three horn-gate branches and the one deferred
branch, all four period outcomes above, exact regulation/OT clock selection,
latch clearing on re-entry, and non-clearing at the `$87:97A0` postgame entry.
Captured presentation entries are `$87:C2F3` (halftime statistics, resource
`$A4:B2F7`), `$87:CC36` (end-regulation statistics, `$A4:B2F7`), `$87:D2AE`
(period-score presentation, `$AF:E478`, `$9F:8000`, music pointer `$82:8933`),
and `$83:FA91` (final summary, `$AF:E478`, `$A9:8000`). Audio commands `$24`
and `$41` accompany the horn; the children issue `$4B`, and `$D2AE` also `$49`.
`$82:DF52` performs five record-update calls before the Exhibition boundary at
`$87:985C`. Exact decompressed tile composition and the callee beyond that
boundary remain uncaptured; Season/playoff persistence branches must not be
inferred from the Exhibition route.
