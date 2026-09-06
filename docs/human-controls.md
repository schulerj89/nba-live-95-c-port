# Human controls workflow

Human gameplay remains incomplete. The action parent `$84:E2AC-$E3E9`
naturally reaches the pass catch preinitializer at `$86:AF66`, whose pose
helper `$87:B7D8` reads canonical WRAM `$012C`. The game-lifetime WRAM owner,
shared renderer/Tipoff views, and native graphics allocator initialization are
present, but no production graphics producer currently establishes that word.

The dependency order is:

1. Implement `$87:AFA2` cache invalidation and `$87:B05B-$B354` jersey-buffer
   publication, then `$80:AD2B-$AD88` with its real renderer caller inputs.
2. Complete the remaining ordered graphics producers and consumer that give
   `$012C` its production provenance.
3. Resume the complete action parent and all naturally reachable action paths,
   including `$86:AF66` and the `$86:B335` layup/shot wrapper.
4. Integrate the full `$87:9106-$92A4` actor/controller sweep and only then
   replace Tipoff's neutral controller selections.

The current WRAM/allocator foundation does not make a normal matchup human
playable and does not claim any portion of the action parent as complete.
