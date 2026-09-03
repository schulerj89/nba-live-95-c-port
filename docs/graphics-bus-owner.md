# Canonical WRAM graphics queue view

This bounded component replaces the proposed persistent private queue with a
borrowed view of one128KiB game-lifetime WRAM owner. Queue records `$0100-$02FF`,
head `$35`, tail `$37`, budget `$39`, decoder scratch and the B468 word `$012C`
therefore alias the same bytes. It introduces no initialization or captured
state and is not yet production-wired.

The accepted queue consumer remains unchanged. `nba_graphics_bus_publish` is a
bounded **endpoint-only adapter**: it projects records/control words, runs the
consumer under a sink contract that forbids observing or mutating the bus, then
commits final head, budget and palette size together. It does not claim native
write visibility or source ordering. An unsupported record is found before any
palette/job publication and returns with no sink call or mutation, so a caller
cannot retry a partially published endpoint. Records are not copied back.
Palette descriptor addresses remain unintegrated and are an explicit borrowed
input/output rather than persistent duplicate state.

That preflight scans the entire queued interval even when the current budget
would stop before a later unsupported record. Refusing that mixed interval is a
deliberately conservative domain restriction, not native endpoint equivalence.
The future stepwise drain must replace it when the missing branches are owned.

The source-confirmed original quirk at `87:B7DA/B7E1` reads DBR `$7E:$012C`.
`nba_graphics_bus_receiver_word` reads that live word. It never substitutes
receiver+$A8, a boolean, a constant or an observed capture value.

The focused `/W4 /WX` probe compares supported randomized endpoint calls with
the accepted value consumer and reports explicit category counts for wraps,
each mode, palette low/high-byte edges and budget stops. Deterministic cases
check unsupported-first and unsupported-after-supported records make no sink or
state change, malformed/upper-WRAM reads, palette descriptor immutability, the
64-record head/tail ambiguity, and direct live `$012C` visibility.
The reported mode counters count generated queued records; an early budget stop
can prevent some of those records from executing. They establish adapter input
coverage alongside the accepted consumer, not new source-semantic coverage.

This does not implement queue producers, cache allocation, codecs, actual
palette owners, source-ordered writes, NMI/DMA timing, persistent placement in
`NbaGame`, B468 wiring or full draw order. The T1 NMI owner must call a future
stepwise drain at the reached source location; this endpoint adapter cannot be
used as timing/order evidence. Those production tasks remain required by the
current remaining-work inventory. The initialization-cut consultation remains in Git history.
