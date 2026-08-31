# Proposed shared NbaGame interface — not applied

Root owns this integration. The standalone checkpoint is not ready to replace
normal game initialization or to jump directly into the existing license scene.

In `include/nba_game.h`, forward-declare or include the bootstrap type and place
its persistent allocation beside `NbaSession session`, **outside** the scene
union. Root should allocate and release it with the game lifetime; it must not
reset on scene changes, pause, track changes or host-audio mute. A pointer avoids
adding its roughly200KiB memory bus directly to an already-large stack object.
The current anonymous struct typedef can be made a tagged declaration in a later
reviewed interface revision if root prefers a forward declaration.

```c
/* Proposal only: outside NbaGame.scene. */
NbaBootstrap *bootstrap;
```

After the existing validated ROM load, initialize the owner from that ROM and
the explicit profile. Do not accept any scene-owned channel/ARAM/port prestate.
Normal boot then needs a running/stopped boundary result owned by the same
driver. The current80BC stop must surface as incomplete bootstrap, not become
a successful license transition. Root's existing license start remains unchanged
until the required reset children are implemented and independently accepted.

The standalone step consumes no externally supplied bus value. Its observer is
optional diagnostic output and must not mutate the state. Future runtime
adapters must choose one owner for shared WRAM-equivalent game fields; they may
not keep independent bootstrap and Tipoff RNG/timer/channel copies and silently
switch between them. This checkpoint establishes hardware/source state, not the
later gameplay alias adapter.

Preserve the difference between cold power-on and soft reset. The current API
implements the named zero-RAM cold profile only; it is not permission to clear
timer prescalers,08FF, output latches or session state at a later scene boundary.
