# Human controls workflow

Human gameplay remains incomplete. The action parent `$84:E2AC-$E3E9`
naturally reaches the pass catch preinitializer at `$86:AF66`, whose pose
helper `$87:B7D8` reads canonical WRAM `$012C`. The game-lifetime WRAM owner,
shared renderer/Tipoff views, and native graphics allocator initialization are
present, but no production graphics producer currently establishes that word.

The held-input sampler `$87:9B30-$9B37` is complete. The actor sweep enters it
from `$87:915D` with the actor's 16-bit controller index in A; the routine
doubles that index, reads one word from `$0576+2*pad`, stores it in DP `$AA`,
and returns to the existing `$85:EF3A` input publisher. Tipoff now retains five
native held words. The current frontend and headless runner continue to supply
pad zero, and pads one through four are cleared until a later frontend input
owner supplies them. During the actor sweep the sampler and publisher run
before normal actor behavior, and controller record `+$04` prevents a shared
pad from being sampled twice in the same sweep.

`tests/fixtures/controller-held-sampler-witnesses.json` retains one untouched
production call and five controlled calls at the genuine native entry. The
controlled cases change only A and `$0576-$057F`; stack, PC, return address,
ROM, and outputs remain native. `tools/test_controller_held_sampler.py` replays
all six calls through `nba_controller_sample_held`, rejects invalid host
indices atomically, and runs the host production behavior pass through held,
changed, pressed, direction, boost, and shared-pad cases. It also checks the
existing pad-zero host conversion and zero values for pads one through four.

The dependency order is:

1. `$87:AFA2-$B058` now invalidates the cache and publishes the complete
   `$87:B059-$B354` jersey buffer. Implement `$80:AD2B-$AD88` next with its
   real renderer caller inputs.
2. Complete the remaining ordered graphics producers and consumer that give
   `$012C` its production provenance.
3. Resume the complete action parent and all naturally reachable action paths,
   including `$86:AF66` and the `$86:B335` layup/shot wrapper.
4. Complete the remaining `$87:9106-$92A4` actor/controller sweep around the
   now-integrated held sampler, and only then replace Tipoff's neutral
   controller selections.

The current WRAM/allocator foundation does not make a normal matchup human
playable and does not claim any portion of the action parent as complete.
